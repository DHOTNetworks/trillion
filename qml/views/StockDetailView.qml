import QtQuick
import QtQuick.Templates as T
import QtQuick.Layouts
import MahadevERP

T.ScrollView {
    id: root
    contentWidth: availableWidth
    clip: true

    signal cancelRequested()
    signal openItemMovement(string itemName)

    function handleEscape() {
        root.cancelRequested()
    }

    Shortcut {
        sequence: "Ctrl+P"
        context: Qt.WindowShortcut
        onActivated: {
            if (typeof printExportCtrl !== "undefined" && printExportCtrl) {
                printExportCtrl.print_stock_register()
            }
        }
    }

    Shortcut {
        sequence: "Alt+P"
        context: Qt.WindowShortcut
        onActivated: {
            if (typeof printExportCtrl !== "undefined" && printExportCtrl) {
                printExportCtrl.export_stock_register_pdf()
            }
        }
    }

    Component.onCompleted: {
        if (typeof stockRegisterCtrl !== "undefined" && stockRegisterCtrl) {
            stockRegisterCtrl.reload()
        }
        Qt.callLater(function() {
            gridListView.forceActiveFocus()
            if (stockRegisterCtrl && stockRegisterCtrl.model.count > 0) {
                gridListView.currentIndex = 0
            }
        })
    }

    onVisibleChanged: {
        if (visible && typeof stockRegisterCtrl !== "undefined" && stockRegisterCtrl) {
            stockRegisterCtrl.reload()
        }
    }

    ColumnLayout {
        width: root.availableWidth > 0 ? root.availableWidth : 1200
        spacing: 16

        // 1. PAGE HEADER BAR
        RowLayout {
            Layout.fillWidth: true
            spacing: 12

            ColumnLayout {
                spacing: 2
                Text {
                    text: "Comprehensive Stock Detail & Register"
                    color: "#0F172A"
                    font.pixelSize: 20
                    font.bold: true
                }
                Text {
                    text: "Real-time itemized physical inventory ledger with opening, inward, outward, and closing stock valuation."
                    color: "#64748B"
                    font.pixelSize: 12
                }
            }

            Item { Layout.fillWidth: true }

            T.Button {
                id: printStockBtn
                implicitWidth: 144
                implicitHeight: 32
                background: Rectangle { color: printStockBtn.hovered ? "#047857" : "#059669"; radius: 6 }
                contentItem: RowLayout {
                    spacing: 6
                    Item { Layout.fillWidth: true }
                    Text { text: "Print Register"; color: "#FFFFFF"; font.pixelSize: 12; font.bold: true }
                    KbdBadge { text: "Ctrl+P"; badgeColor: "#064E3B"; textColor: "#A7F3D0"; borderColor: "#059669" }
                    Item { Layout.fillWidth: true }
                }
                onClicked: {
                    if (typeof printExportCtrl !== "undefined" && printExportCtrl) {
                        printExportCtrl.print_stock_register()
                    }
                }
            }

            T.Button {
                id: exportStockPdfBtn
                implicitWidth: 130
                implicitHeight: 32
                background: Rectangle { color: exportStockPdfBtn.hovered ? "#0284C7" : "#0EA5E9"; radius: 6 }
                contentItem: RowLayout {
                    spacing: 6
                    Item { Layout.fillWidth: true }
                    Text { text: "Export PDF"; color: "#FFFFFF"; font.pixelSize: 12; font.bold: true }
                    KbdBadge { text: "Alt+P"; badgeColor: "#075985"; textColor: "#BAE6FD"; borderColor: "#0EA5E9" }
                    Item { Layout.fillWidth: true }
                }
                onClicked: {
                    if (typeof printExportCtrl !== "undefined" && printExportCtrl) {
                        printExportCtrl.export_stock_register_pdf()
                    }
                }
            }

            T.Button {
                id: exportStockCsvBtn
                implicitWidth: contentItem.implicitWidth + 24
                implicitHeight: 32
                background: Rectangle { color: exportStockCsvBtn.hovered ? "#F1F5F9" : "#FFFFFF"; radius: 6; border.color: "#CBD5E1" }
                contentItem: Text {
                    text: "Export CSV"
                    color: "#334155"
                    font.pixelSize: 12
                    font.bold: true
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
                onClicked: {
                    if (typeof printExportCtrl !== "undefined" && printExportCtrl) {
                        printExportCtrl.export_stock_csv()
                    }
                }
            }

            T.Button {
                id: backBtn
                implicitWidth: contentItem.implicitWidth + 24
                implicitHeight: 32
                background: Rectangle { color: backBtn.hovered ? "#475569" : "#334155"; radius: 6 }
                contentItem: Text {
                    text: "← Back (Esc)"
                    color: "#F8FAFC"
                    font.pixelSize: 12
                    font.bold: true
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
                onClicked: root.cancelRequested()
            }
        }

        Rectangle { Layout.fillWidth: true; height: 1; color: "#E2E8F0" }

        // 2. STATS STRIP
        RowLayout {
            Layout.fillWidth: true
            spacing: 12

            StatCard {
                title: "TOTAL INVENTORY ITEMS"
                value: (typeof stockRegisterCtrl !== "undefined" && stockRegisterCtrl) ? stockRegisterCtrl.totalItemsCount.toString() : "0"
                accentColor: "#2563EB"
                Layout.fillWidth: true
            }

            StatCard {
                title: "TOTAL CLOSING QUANTITY"
                value: (typeof stockRegisterCtrl !== "undefined" && stockRegisterCtrl) ? stockRegisterCtrl.totalClosingQtyFmt : "0.00 Qtl"
                accentColor: "#16A34A"
                Layout.fillWidth: true
            }

            StatCard {
                title: "TOTAL STOCK VALUATION"
                value: (typeof stockRegisterCtrl !== "undefined" && stockRegisterCtrl) ? stockRegisterCtrl.totalClosingValFmt : "₹0.00"
                accentColor: "#D97706"
                Layout.fillWidth: true
            }
        }

        // 3. TABLE GRID CONTAINER WITH SEARCH BAR
        Rectangle {
            Layout.fillWidth: true
            implicitHeight: 520
            color: "#FFFFFF"
            border.color: "#CBD5E1"
            border.width: 1
            radius: 8

            ColumnLayout {
                anchors.fill: parent
                spacing: 0

                // Search Toolbar
                Rectangle {
                    Layout.fillWidth: true
                    height: 52
                    color: "#F8FAFC"
                    border.color: "#E2E8F0"
                    border.width: 1
                    radius: 8

                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: 16; anchors.rightMargin: 16
                        spacing: 12

                        Text { text: "Search Stock Items:"; color: "#334155"; font.pixelSize: 12; font.bold: true }

                        Rectangle {
                            Layout.fillWidth: true
                            height: 34
                            color: "#FFFFFF"
                            border.color: searchField.activeFocus ? "#2563EB" : "#CBD5E1"
                            border.width: searchField.activeFocus ? 2 : 1
                            radius: 6

                            RowLayout {
                                anchors.fill: parent
                                anchors.leftMargin: 10; anchors.rightMargin: 10
                                spacing: 6

                                TextInput {
                                    id: searchField
                                    Layout.fillWidth: true
                                    font.pixelSize: 12
                                    color: "#0F172A"
                                    selectByMouse: true
                                    onTextChanged: {
                                        if (typeof stockRegisterCtrl !== "undefined" && stockRegisterCtrl) {
                                            stockRegisterCtrl.searchQuery = text
                                        }
                                    }
                                    Keys.onDownPressed: function(event) {
                                        event.accepted = true
                                        gridListView.forceActiveFocus()
                                        if (gridListView.currentIndex < 0 && gridListView.count > 0) gridListView.currentIndex = 0
                                    }
                                    Keys.onReturnPressed: function(event) {
                                        event.accepted = true
                                        gridListView.forceActiveFocus()
                                        if (gridListView.currentIndex < 0 && gridListView.count > 0) gridListView.currentIndex = 0
                                    }
                                    Keys.onEnterPressed: function(event) {
                                        event.accepted = true
                                        gridListView.forceActiveFocus()
                                        if (gridListView.currentIndex < 0 && gridListView.count > 0) gridListView.currentIndex = 0
                                    }
                                }

                                Text {
                                    text: "X"
                                    color: "#94A3B8"
                                    font.pixelSize: 12
                                    font.bold: true
                                    visible: searchField.text.length > 0
                                    MouseArea {
                                        anchors.fill: parent
                                        onClicked: {
                                            searchField.text = ""
                                            if (typeof stockRegisterCtrl !== "undefined" && stockRegisterCtrl) {
                                                stockRegisterCtrl.searchQuery = ""
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }

                // GRID TABLE HEADER
                Rectangle {
                    Layout.fillWidth: true
                    height: 32
                    color: "#F1F5F9"
                    border.color: "#CBD5E1"
                    border.width: 1
                    radius: 4

                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: 10; anchors.rightMargin: 10
                        spacing: 0

                        Text { text: "Item Name"; color: "#475569"; font.pixelSize: 11; font.bold: true; Layout.fillWidth: true; Layout.leftMargin: 4 }
                        Rectangle { width: 1; height: parent.height; color: "#CBD5E1" }

                        Text { text: "Code"; color: "#475569"; font.pixelSize: 11; font.bold: true; Layout.preferredWidth: 100; Layout.leftMargin: 8 }
                        Rectangle { width: 1; height: parent.height; color: "#CBD5E1" }

                        Text { text: "Type"; color: "#475569"; font.pixelSize: 11; font.bold: true; Layout.preferredWidth: 100; Layout.leftMargin: 8 }
                        Rectangle { width: 1; height: parent.height; color: "#CBD5E1" }

                        Text { text: "Unit"; color: "#475569"; font.pixelSize: 11; font.bold: true; Layout.preferredWidth: 55; horizontalAlignment: Text.AlignHCenter }
                        Rectangle { width: 1; height: parent.height; color: "#CBD5E1" }

                        Text { text: "Opening Qtl"; color: "#475569"; font.pixelSize: 11; font.bold: true; Layout.preferredWidth: 90; horizontalAlignment: Text.AlignRight; Layout.rightMargin: 6 }
                        Rectangle { width: 1; height: parent.height; color: "#CBD5E1" }

                        Text { text: "Inward Qtl"; color: "#475569"; font.pixelSize: 11; font.bold: true; Layout.preferredWidth: 90; horizontalAlignment: Text.AlignRight; Layout.rightMargin: 6 }
                        Rectangle { width: 1; height: parent.height; color: "#CBD5E1" }

                        Text { text: "Outward Qtl"; color: "#475569"; font.pixelSize: 11; font.bold: true; Layout.preferredWidth: 90; horizontalAlignment: Text.AlignRight; Layout.rightMargin: 6 }
                        Rectangle { width: 1; height: parent.height; color: "#CBD5E1" }

                        Text { text: "Closing Stock"; color: "#475569"; font.pixelSize: 11; font.bold: true; Layout.preferredWidth: 100; horizontalAlignment: Text.AlignRight; Layout.rightMargin: 6 }
                        Rectangle { width: 1; height: parent.height; color: "#CBD5E1" }

                        Text { text: "Rate (₹)"; color: "#475569"; font.pixelSize: 11; font.bold: true; Layout.preferredWidth: 90; horizontalAlignment: Text.AlignRight; Layout.rightMargin: 6 }
                        Rectangle { width: 1; height: parent.height; color: "#CBD5E1" }

                        Text { text: "Closing Value (₹)"; color: "#475569"; font.pixelSize: 11; font.bold: true; Layout.preferredWidth: 120; horizontalAlignment: Text.AlignRight; Layout.rightMargin: 8 }
                    }
                }

                // GRID TABLE BODY LISTVIEW
                ListView {
                    id: gridListView
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    model: (typeof stockRegisterCtrl !== "undefined" && stockRegisterCtrl) ? stockRegisterCtrl.model : null
                    clip: true
                    spacing: 1
                    boundsBehavior: Flickable.StopAtBounds
                    focus: true
                    highlightFollowsCurrentItem: true
                    currentIndex: 0

                    Keys.onReturnPressed: function(event) {
                        event.accepted = true
                        if (currentIndex >= 0 && currentIndex < count) {
                            var it = stockRegisterCtrl.model.get(currentIndex)
                            if (it && it.nameVal) root.openItemMovement(it.nameVal)
                        }
                    }
                    Keys.onEnterPressed: function(event) {
                        event.accepted = true
                        if (currentIndex >= 0 && currentIndex < count) {
                            var it = stockRegisterCtrl.model.get(currentIndex)
                            if (it && it.nameVal) root.openItemMovement(it.nameVal)
                        }
                    }
                    Keys.onUpPressed: function(event) {
                        event.accepted = true
                        if (currentIndex > 0) {
                            currentIndex--
                            positionViewAtIndex(currentIndex, ListView.Contain)
                        } else {
                            searchField.forceActiveFocus()
                        }
                    }
                    Keys.onDownPressed: function(event) {
                        event.accepted = true
                        if (currentIndex < count - 1) {
                            currentIndex++
                            positionViewAtIndex(currentIndex, ListView.Contain)
                        }
                    }

                    delegate: Rectangle {
                        id: rowRect
                        width: gridListView.width
                        height: 34
                        color: ListView.isCurrentItem ? "#DBEAFE" : (index % 2 === 0 ? "#FFFFFF" : "#F8FAFC")
                        border.color: (ListView.isCurrentItem || mouseArea.containsMouse) ? "#2563EB" : "#E2E8F0"
                        border.width: ListView.isCurrentItem ? 2 : 1
                        radius: 2

                        MouseArea {
                            id: mouseArea
                            anchors.fill: parent
                            hoverEnabled: true
                            onClicked: {
                                gridListView.currentIndex = index
                                gridListView.forceActiveFocus()
                            }
                            onDoubleClicked: {
                                gridListView.currentIndex = index
                                root.openItemMovement(model.nameVal)
                            }
                        }

                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: 10; anchors.rightMargin: 10
                            spacing: 0

                            // Item Name
                            Text {
                                text: model.nameVal ? model.nameVal : ""
                                color: "#0F172A"
                                font.pixelSize: 12
                                font.bold: true
                                Layout.fillWidth: true
                                Layout.leftMargin: 4
                                elide: Text.ElideRight
                            }

                            Rectangle { width: 1; height: parent.height; color: "#E2E8F0" }

                            // Code
                            Text {
                                text: model.codeVal ? model.codeVal : "-"
                                color: "#2563EB"
                                font.pixelSize: 11
                                font.bold: true
                                Layout.preferredWidth: 100
                                Layout.leftMargin: 8
                                elide: Text.ElideRight
                            }

                            Rectangle { width: 1; height: parent.height; color: "#E2E8F0" }

                            // Item Type
                            Text {
                                text: model.typeVal ? model.typeVal : "-"
                                color: "#475569"
                                font.pixelSize: 11
                                Layout.preferredWidth: 100
                                Layout.leftMargin: 8
                                elide: Text.ElideRight
                            }

                            Rectangle { width: 1; height: parent.height; color: "#E2E8F0" }

                            // Unit
                            Text {
                                text: model.unitVal ? model.unitVal : "Qtl"
                                color: "#64748B"
                                font.pixelSize: 11
                                Layout.preferredWidth: 55
                                horizontalAlignment: Text.AlignHCenter
                            }

                            Rectangle { width: 1; height: parent.height; color: "#E2E8F0" }

                            // Opening Qtl
                            Text {
                                text: model.opQtyVal ? model.opQtyVal : "0.00"
                                color: "#334155"
                                font.pixelSize: 11
                                Layout.preferredWidth: 90
                                horizontalAlignment: Text.AlignRight
                                Layout.rightMargin: 6
                            }

                            Rectangle { width: 1; height: parent.height; color: "#E2E8F0" }

                            // Inward Qtl
                            Text {
                                text: model.inQtyVal ? model.inQtyVal : "0.00"
                                color: "#16A34A"
                                font.pixelSize: 11
                                font.bold: true
                                Layout.preferredWidth: 90
                                horizontalAlignment: Text.AlignRight
                                Layout.rightMargin: 6
                            }

                            Rectangle { width: 1; height: parent.height; color: "#E2E8F0" }

                            // Outward Qtl
                            Text {
                                text: model.outQtyVal ? model.outQtyVal : "0.00"
                                color: "#DC2626"
                                font.pixelSize: 11
                                font.bold: true
                                Layout.preferredWidth: 90
                                horizontalAlignment: Text.AlignRight
                                Layout.rightMargin: 6
                            }

                            Rectangle { width: 1; height: parent.height; color: "#E2E8F0" }

                            // Net Closing Stock
                            Text {
                                text: model.closeQtyVal ? model.closeQtyVal : "0.00"
                                color: (model.closeQtyNum >= 0) ? "#15803D" : "#C2410C"
                                font.pixelSize: 12
                                font.bold: true
                                Layout.preferredWidth: 100
                                horizontalAlignment: Text.AlignRight
                                Layout.rightMargin: 6
                            }

                            Rectangle { width: 1; height: parent.height; color: "#E2E8F0" }

                            // Rate
                            Text {
                                text: model.rateVal ? model.rateVal : "₹0.00"
                                color: "#334155"
                                font.pixelSize: 11
                                Layout.preferredWidth: 90
                                horizontalAlignment: Text.AlignRight
                                Layout.rightMargin: 6
                            }

                            Rectangle { width: 1; height: parent.height; color: "#E2E8F0" }

                            // Closing Valuation Value ₹
                            Text {
                                text: model.closeValVal ? model.closeValVal : "₹0.00"
                                color: "#1D4ED8"
                                font.pixelSize: 12
                                font.bold: true
                                Layout.preferredWidth: 120
                                horizontalAlignment: Text.AlignRight
                                Layout.rightMargin: 8
                            }
                        }
                    }
                }
            }
        }
    }
}
