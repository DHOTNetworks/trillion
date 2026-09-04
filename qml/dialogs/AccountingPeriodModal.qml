import QtQuick
import QtQuick.Templates as T
import QtQuick.Layouts
import MahadevERP

Rectangle {
    id: root
    width: 480
    height: 560
    color: "#FFFFFF"
    radius: 16
    border.color: "#E2E8F0"
    border.width: 1
    clip: true

    signal closeRequested()
    signal periodSelected(string fromDate, string toDate, string fyName)

    property string selectedFromDate: ""
    property string selectedFromIso: ""
    property string selectedToDate: ""
    property string selectedToIso: ""
    property string selectedFyLabel: "FY 2025-26"
    property var fyPresets: []

    function loadFromDatabase() {
        fromListModel.clear()
        toListModel.clear()
        
        var availableFys = (typeof stockItemsModel !== "undefined" && stockItemsModel) ? stockItemsModel.get_available_financial_years() : []
        fyPresets = availableFys
        
        var activeFromIso = (typeof stockItemsModel !== "undefined" && stockItemsModel) ? stockItemsModel.get_from_date() : ""
        var activeToIso = (typeof stockItemsModel !== "undefined" && stockItemsModel) ? stockItemsModel.get_to_date() : ""
        var currentFy = (typeof stockItemsModel !== "undefined" && stockItemsModel) ? stockItemsModel.get_financial_year() : ""

        // Fallback to active FY if no explicit dates
        if (!activeFromIso || !activeToIso) {
            for (var k = 0; k < availableFys.length; k++) {
                if (availableFys[k].name === currentFy || (!currentFy && availableFys[k].isActive)) {
                    activeFromIso = availableFys[k].startDate
                    activeToIso = availableFys[k].endDate
                    currentFy = availableFys[k].name
                    break
                }
            }
        }

        selectedFromIso = activeFromIso
        selectedToIso = activeToIso

        for (var i = 0; i < availableFys.length; i++) {
            var f = availableFys[i]
            if (f.name === "All") continue
            
            var isFromChecked = (f.startDate === activeFromIso)
            var isToChecked = (f.endDate === activeToIso)

            if (isFromChecked) {
                selectedFromDate = f.startFormatted
            }
            if (isToChecked) {
                selectedToDate = f.endFormatted
            }

            fromListModel.append({
                date: f.startFormatted,
                iso: f.startDate,
                fy: f.name,
                checked: isFromChecked
            })

            toListModel.append({
                date: f.endFormatted,
                iso: f.endDate,
                fy: f.name,
                checked: isToChecked
            })
        }

        // Match with presets dynamically to see if it matches an exact FY or is Custom Period
        var matched = false
        for (var m = 0; m < availableFys.length; m++) {
            if (availableFys[m].startDate === selectedFromIso && availableFys[m].endDate === selectedToIso) {
                selectedFyLabel = availableFys[m].name
                matched = true
                break
            }
        }
        if (!matched) {
            selectedFyLabel = "Custom Period"
        }
    }

    function setPreset(fromD, toD, fromIso, toIso, fy) {
        for (var i = 0; i < fromListModel.count; i++) {
            fromListModel.setProperty(i, "checked", fromListModel.get(i).iso === fromIso)
        }
        for (var j = 0; j < toListModel.count; j++) {
            toListModel.setProperty(j, "checked", toListModel.get(j).iso === toIso)
        }
        selectedFromDate = fromD
        selectedFromIso = fromIso
        selectedToDate = toD
        selectedToIso = toIso
        selectedFyLabel = fy
    }

    function updateSelection() {
        for (var i = 0; i < fromListModel.count; i++) {
            if (fromListModel.get(i).checked) {
                selectedFromDate = fromListModel.get(i).date
                selectedFromIso = fromListModel.get(i).iso
                break
            }
        }
        for (var j = 0; j < toListModel.count; j++) {
            if (toListModel.get(j).checked) {
                selectedToDate = toListModel.get(j).date
                selectedToIso = toListModel.get(j).iso
                break
            }
        }

        // Match with presets dynamically
        var matched = false
        for (var k = 0; k < fyPresets.length; k++) {
            if (fyPresets[k].startDate === selectedFromIso && fyPresets[k].endDate === selectedToIso) {
                selectedFyLabel = fyPresets[k].name
                matched = true
                break
            }
        }
        if (!matched) {
            selectedFyLabel = "Custom Period"
        }
    }

    onVisibleChanged: {
        if (visible) {
            loadFromDatabase()
        }
    }

    Component.onCompleted: {
        loadFromDatabase()
    }

    GenericListModel { id: fromListModel }
    GenericListModel { id: toListModel }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 20
        spacing: 16

        // 1. MODERN HEADER
        RowLayout {
            Layout.fillWidth: true
            spacing: 12

            Rectangle {
                width: 40; height: 40; radius: 10
                color: "#ECFDF5"
                border.color: "#A7F3D0"
                Text {
                    anchors.centerIn: parent
                    text: "📅"
                    font.pixelSize: 20
                }
            }

            ColumnLayout {
                spacing: 2
                Text {
                    text: "Accounting Period & Financial Year"
                    font.pixelSize: 16
                    font.bold: true
                    color: "#0F172A"
                }
                Text {
                    text: "Set active fiscal operating year for ledger books & stock registers"
                    font.pixelSize: 11
                    color: "#64748B"
                }
            }

            Item { Layout.fillWidth: true }

            Rectangle {
                width: 28; height: 28; radius: 14
                color: closeHover.containsMouse ? "#F1F5F9" : "transparent"
                Text {
                    anchors.centerIn: parent
                    text: "✕"
                    font.pixelSize: 13
                    font.bold: true
                    color: "#64748B"
                }
                MouseArea {
                    id: closeHover
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: root.closeRequested()
                }
            }
        }

        // 2. DYNAMIC PRESET CHIPS FROM DATABASE
        ColumnLayout {
            Layout.fillWidth: true
            spacing: 6

            Text {
                text: "FINANCIAL YEARS IN DATABASE"
                font.pixelSize: 10
                font.bold: true
                color: "#94A3B8"
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: 8

                Repeater {
                    model: root.fyPresets
                    delegate: Rectangle {
                        visible: modelData.name !== "All"
                        Layout.fillWidth: true
                        height: 34
                        radius: 8
                        color: root.selectedFyLabel === modelData.name ? "#047857" : "#F8FAFC"
                        border.color: root.selectedFyLabel === modelData.name ? "#047857" : "#E2E8F0"
                        border.width: 1

                        RowLayout {
                            anchors.centerIn: parent
                            spacing: 4
                            Text {
                                text: modelData.name
                                font.pixelSize: 12
                                font.bold: true
                                color: root.selectedFyLabel === modelData.name ? "#FFFFFF" : "#1E293B"
                            }
                            Rectangle {
                                width: 6; height: 6; radius: 3
                                color: root.selectedFyLabel === modelData.name ? "#34D399" : (modelData.isActive ? "#10B981" : "transparent")
                            }
                        }

                        MouseArea {
                            anchors.fill: parent
                            cursorShape: Qt.PointingHandCursor
                            onClicked: root.setPreset(modelData.startFormatted, modelData.endFormatted, modelData.startDate, modelData.endDate, modelData.name)
                        }
                    }
                }
            }
        }

        // 3. DATE SELECTION COLUMNS (YEAR FROM & YEAR TO)
        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 12

            // Year From Column
            Rectangle {
                Layout.fillWidth: true
                Layout.fillHeight: true
                radius: 10
                color: "#F8FAFC"
                border.color: "#E2E8F0"

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 10
                    spacing: 8

                    Text {
                        text: "From Date"
                        font.pixelSize: 11
                        font.bold: true
                        color: "#475569"
                    }

                    ListView {
                        id: fromListView
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        model: fromListModel
                        spacing: 6
                        delegate: Rectangle {
                            width: fromListView.width
                            height: 34
                            radius: 6
                            color: model.checked ? "#ECFDF5" : "#FFFFFF"
                            border.color: model.checked ? "#10B981" : "#E2E8F0"
                            border.width: model.checked ? 1.5 : 1

                            RowLayout {
                                anchors.fill: parent
                                anchors.leftMargin: 10; anchors.rightMargin: 10
                                spacing: 8

                                Rectangle {
                                    width: 16; height: 16; radius: 8
                                    color: model.checked ? "#10B981" : "#FFFFFF"
                                    border.color: model.checked ? "#10B981" : "#CBD5E1"
                                    border.width: 1.5
                                    Rectangle {
                                        anchors.centerIn: parent
                                        width: 6; height: 6; radius: 3
                                        color: "#FFFFFF"
                                        visible: model.checked
                                    }
                                }

                                Text {
                                    text: model.date
                                    font.pixelSize: 12
                                    font.bold: model.checked
                                    color: model.checked ? "#065F46" : "#1E293B"
                                }
                            }

                            MouseArea {
                                anchors.fill: parent
                                cursorShape: Qt.PointingHandCursor
                                onClicked: {
                                    for (var k = 0; k < fromListModel.count; k++) {
                                        fromListModel.setProperty(k, "checked", k === index)
                                    }
                                    root.updateSelection()
                                }
                            }
                        }
                    }
                }
            }

            // Year To Column
            Rectangle {
                Layout.fillWidth: true
                Layout.fillHeight: true
                radius: 10
                color: "#F8FAFC"
                border.color: "#E2E8F0"

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 10
                    spacing: 8

                    Text {
                        text: "To Date"
                        font.pixelSize: 11
                        font.bold: true
                        color: "#475569"
                    }

                    ListView {
                        id: toListView
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        model: toListModel
                        spacing: 6
                        delegate: Rectangle {
                            width: toListView.width
                            height: 34
                            radius: 6
                            color: model.checked ? "#ECFDF5" : "#FFFFFF"
                            border.color: model.checked ? "#10B981" : "#E2E8F0"
                            border.width: model.checked ? 1.5 : 1

                            RowLayout {
                                anchors.fill: parent
                                anchors.leftMargin: 10; anchors.rightMargin: 10
                                spacing: 8

                                Rectangle {
                                    width: 16; height: 16; radius: 8
                                    color: model.checked ? "#10B981" : "#FFFFFF"
                                    border.color: model.checked ? "#10B981" : "#CBD5E1"
                                    border.width: 1.5
                                    Rectangle {
                                        anchors.centerIn: parent
                                        width: 6; height: 6; radius: 3
                                        color: "#FFFFFF"
                                        visible: model.checked
                                    }
                                }

                                Text {
                                    text: model.date
                                    font.pixelSize: 12
                                    font.bold: model.checked
                                    color: model.checked ? "#065F46" : "#1E293B"
                                }
                            }

                            MouseArea {
                                anchors.fill: parent
                                cursorShape: Qt.PointingHandCursor
                                onClicked: {
                                    for (var k = 0; k < toListModel.count; k++) {
                                        toListModel.setProperty(k, "checked", k === index)
                                    }
                                    root.updateSelection()
                                }
                            }
                        }
                    }
                }
            }
        }

        // 4. SELECTED PERIOD SUMMARY BANNER
        Rectangle {
            Layout.fillWidth: true
            height: 40
            radius: 8
            color: "#0F172A"

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 14; anchors.rightMargin: 14
                spacing: 8

                Text {
                    text: "ACTIVE PERIOD:"
                    font.pixelSize: 11
                    font.bold: true
                    color: "#94A3B8"
                }

                Text {
                    text: (root.selectedFromDate || "—") + "  ➔  " + (root.selectedToDate || "—")
                    font.pixelSize: 13
                    font.bold: true
                    color: "#34D399"
                }

                Item { Layout.fillWidth: true }

                Rectangle {
                    height: 22
                    implicitWidth: fyTextBadge.implicitWidth + 14
                    radius: 4
                    color: "#1E293B"
                    Text {
                        id: fyTextBadge
                        anchors.centerIn: parent
                        text: root.selectedFyLabel
                        font.pixelSize: 11
                        font.bold: true
                        color: "#60A5FA"
                    }
                }
            }
        }

        // 5. ACTION BUTTONS
        RowLayout {
            Layout.fillWidth: true
            spacing: 10

            T.Button {
                Layout.fillWidth: true
                Layout.preferredHeight: 38
                background: Rectangle {
                    color: "#10B981"
                    radius: 8
                }
                contentItem: RowLayout {
                    spacing: 6
                    Item { Layout.fillWidth: true }
                    Text {
                        text: "✓ Apply Period"
                        font.pixelSize: 13
                        font.bold: true
                        color: "#FFFFFF"
                    }
                    KbdBadge {
                        text: "Enter"
                        badgeColor: "#047857"
                        textColor: "#A7F3D0"
                        borderColor: "#059669"
                    }
                    Item { Layout.fillWidth: true }
                }
                onClicked: {
                    if (typeof stockItemsModel !== "undefined" && stockItemsModel) {
                        stockItemsModel.set_accounting_period(root.selectedFromIso, root.selectedToIso, root.selectedFyLabel)
                    }
                    root.periodSelected(root.selectedFromIso, root.selectedToIso, root.selectedFyLabel)
                    root.closeRequested()
                }
            }

            T.Button {
                Layout.preferredWidth: 100
                Layout.preferredHeight: 38
                background: Rectangle {
                    color: "#F1F5F9"
                    border.color: "#CBD5E1"
                    radius: 8
                }
                contentItem: Text {
                    anchors.centerIn: parent
                    text: "Cancel"
                    font.pixelSize: 13
                    color: "#475569"
                }
                onClicked: root.closeRequested()
            }
        }
    }
}
