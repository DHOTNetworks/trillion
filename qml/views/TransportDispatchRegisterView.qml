import QtQuick
import QtQuick.Templates as T
import QtQuick.Layouts
import MahadevERP
import "../components"
import "../dialogs"

T.ScrollView {
    id: root
    contentWidth: availableWidth
    clip: true

    signal cancelRequested()
    signal openInvoiceRequested(string invoiceNo)

    property string activeTab: "ALL"

    function handleEscape() {
        if (kandaModal.visible) {
            kandaModal.close()
            return
        }
        root.cancelRequested()
    }

    function openNewModal() {
        kandaModalObj.resetForm()
        kandaModal.open()
    }

    function openEditModal(id) {
        kandaModalObj.loadDispatch(id)
        kandaModal.open()
    }

    function applyFilters() {
        if (typeof transportDispatchCtrl !== "undefined" && transportDispatchCtrl.model) {
            transportDispatchCtrl.model.setFilter(root.activeTab, searchInput.text.trim())
        }
    }

    Component.onCompleted: {
        if (typeof transportDispatchCtrl !== "undefined") {
            transportDispatchCtrl.reload()
        }
    }

    onVisibleChanged: {
        if (visible && typeof transportDispatchCtrl !== "undefined") {
            transportDispatchCtrl.reload()
            applyFilters()
        }
    }

    // Weighbridge Kanda Entry Modal
    T.Popup {
        id: kandaModal
        anchors.centerIn: T.Overlay.overlay
        width: Math.min(840, T.Overlay.overlay ? T.Overlay.overlay.width - 40 : 840)
        height: Math.min(680, T.Overlay.overlay ? T.Overlay.overlay.height - 40 : 680)
        padding: 0
        modal: true
        dim: true
        focus: true
        closePolicy: T.Popup.CloseOnEscape

        background: Rectangle {
            color: "transparent"
        }

        WeighbridgeKandaModal {
            id: kandaModalObj
            anchors.fill: parent
            onCloseRequested: kandaModal.close()
            onSavedSuccess: {
                if (typeof transportDispatchCtrl !== "undefined") {
                    transportDispatchCtrl.reload()
                }
                applyFilters()
            }
        }
    }

    ColumnLayout {
        width: root.availableWidth > 0 ? root.availableWidth : 1200
        spacing: 16

        // 1. Page Header Bar
        RowLayout {
            Layout.fillWidth: true
            spacing: 12

            ColumnLayout {
                spacing: 2
                Text {
                    text: "Transport, Weighbridge (Kanda) & e-Way Register"
                    color: "#0F172A"
                    font.pixelSize: 20
                    font.bold: true
                }
                Text {
                    text: "Physical weighbridge gross/tare weights, truck dispatches, freight advance/balance, and e-Way logistics."
                    color: "#64748B"
                    font.pixelSize: 12
                }
            }

            Item { Layout.fillWidth: true }

            T.Button {
                id: newSlipBtn
                implicitWidth: contentItem.implicitWidth + 24
                implicitHeight: 34
                background: Rectangle { color: newSlipBtn.hovered ? "#1D4ED8" : "#2563EB"; radius: 6 }
                contentItem: RowLayout {
                    spacing: 6
                    Item { Layout.fillWidth: true }
                    Text { text: "+ New Weighbridge Slip"; color: "#FFFFFF"; font.pixelSize: 12; font.bold: true }
                    KbdBadge { text: "Ctrl+N"; badgeColor: "#1E3A8A"; textColor: "#93C5FD"; borderColor: "#2563EB" }
                    Item { Layout.fillWidth: true }
                }
                onClicked: root.openNewModal()
            }

            T.Button {
                id: exportCsvBtn
                implicitWidth: contentItem.implicitWidth + 24
                implicitHeight: 34
                background: Rectangle { color: exportCsvBtn.hovered ? "#F1F5F9" : "#FFFFFF"; radius: 6; border.color: "#CBD5E1" }
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
                        printExportCtrl.export_sales_register_csv()
                    }
                }
            }

            T.Button {
                id: backBtn
                implicitWidth: contentItem.implicitWidth + 24
                implicitHeight: 34
                background: Rectangle { color: backBtn.hovered ? "#475569" : "#334155"; radius: 6 }
                contentItem: Text {
                    text: "< Back (Esc)"
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
                title: "TOTAL DISPATCHES"
                value: (typeof transportDispatchCtrl !== "undefined" && transportDispatchCtrl && transportDispatchCtrl.model) ? transportDispatchCtrl.model.filteredCount.toString() : "0"
                accentColor: "#2563EB"
                Layout.fillWidth: true
            }

            StatCard {
                title: "TOTAL NET WEIGHT"
                value: (typeof transportDispatchCtrl !== "undefined" && transportDispatchCtrl && transportDispatchCtrl.model) ? transportDispatchCtrl.model.filteredNetWeightFmt : "0.00 Qtl"
                accentColor: "#16A34A"
                Layout.fillWidth: true
            }

            StatCard {
                title: "TOTAL FREIGHT PAYABLE"
                value: (typeof transportDispatchCtrl !== "undefined" && transportDispatchCtrl && transportDispatchCtrl.model) ? transportDispatchCtrl.model.filteredFreightAmountFmt : "Rs. 0.00"
                accentColor: "#D97706"
                Layout.fillWidth: true
            }

            StatCard {
                title: "OUTSTANDING FREIGHT BALANCE"
                value: (typeof transportDispatchCtrl !== "undefined" && transportDispatchCtrl && transportDispatchCtrl.model) ? transportDispatchCtrl.model.filteredBalancePayableFmt : "Rs. 0.00"
                accentColor: "#DC2626"
                Layout.fillWidth: true
            }
        }

        // 3. TABLE GRID CONTAINER WITH SEARCH AND FILTER TABS
        Rectangle {
            Layout.fillWidth: true
            implicitHeight: 560
            color: "#FFFFFF"
            border.color: "#CBD5E1"
            border.width: 1
            radius: 8

            ColumnLayout {
                anchors.fill: parent
                spacing: 0

                // Filter Tabs & Search Bar Toolbar
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

                        // Tab buttons
                        RowLayout {
                            spacing: 8

                            T.Button {
                                implicitWidth: contentItem.implicitWidth + 24
                                implicitHeight: 32
                                background: Rectangle {
                                    color: root.activeTab === "ALL" ? "#2563EB" : "#E2E8F0"
                                    radius: 5
                                }
                                contentItem: Text {
                                    text: "All Dispatches"
                                    color: root.activeTab === "ALL" ? "#FFFFFF" : "#334155"
                                    font.pixelSize: 11
                                    font.bold: true
                                    horizontalAlignment: Text.AlignHCenter
                                    verticalAlignment: Text.AlignVCenter
                                }
                                onClicked: {
                                    root.activeTab = "ALL"
                                    root.applyFilters()
                                }
                            }

                            T.Button {
                                implicitWidth: contentItem.implicitWidth + 24
                                implicitHeight: 32
                                background: Rectangle {
                                    color: root.activeTab === "PENDING" ? "#DC2626" : "#E2E8F0"
                                    radius: 5
                                }
                                contentItem: Text {
                                    text: "Pending Freight"
                                    color: root.activeTab === "PENDING" ? "#FFFFFF" : "#334155"
                                    font.pixelSize: 11
                                    font.bold: true
                                    horizontalAlignment: Text.AlignHCenter
                                    verticalAlignment: Text.AlignVCenter
                                }
                                onClicked: {
                                    root.activeTab = "PENDING"
                                    root.applyFilters()
                                }
                            }

                            T.Button {
                                implicitWidth: contentItem.implicitWidth + 24
                                implicitHeight: 32
                                background: Rectangle {
                                    color: root.activeTab === "SETTLED" ? "#16A34A" : "#E2E8F0"
                                    radius: 5
                                }
                                contentItem: Text {
                                    text: "Settled Freight"
                                    color: root.activeTab === "SETTLED" ? "#FFFFFF" : "#334155"
                                    font.pixelSize: 11
                                    font.bold: true
                                    horizontalAlignment: Text.AlignHCenter
                                    verticalAlignment: Text.AlignVCenter
                                }
                                onClicked: {
                                    root.activeTab = "SETTLED"
                                    root.applyFilters()
                                }
                            }

                            T.Button {
                                implicitWidth: contentItem.implicitWidth + 24
                                implicitHeight: 32
                                background: Rectangle {
                                    color: root.activeTab === "TODAY" ? "#4F46E5" : "#E2E8F0"
                                    radius: 5
                                }
                                contentItem: Text {
                                    text: "Today"
                                    color: root.activeTab === "TODAY" ? "#FFFFFF" : "#334155"
                                    font.pixelSize: 11
                                    font.bold: true
                                    horizontalAlignment: Text.AlignHCenter
                                    verticalAlignment: Text.AlignVCenter
                                }
                                onClicked: {
                                    root.activeTab = "TODAY"
                                    root.applyFilters()
                                }
                            }
                        }

                        Item { Layout.fillWidth: true }

                        Text { text: "Search Dispatches:"; color: "#334155"; font.pixelSize: 12; font.bold: true }

                        Rectangle {
                            Layout.preferredWidth: 280
                            height: 34
                            color: "#FFFFFF"
                            border.color: searchInput.activeFocus ? "#2563EB" : "#CBD5E1"
                            border.width: searchInput.activeFocus ? 2 : 1
                            radius: 6

                            RowLayout {
                                anchors.fill: parent
                                anchors.leftMargin: 10; anchors.rightMargin: 10
                                spacing: 6

                                TextInput {
                                    id: searchInput
                                    Layout.fillWidth: true
                                    font.pixelSize: 12
                                    color: "#0F172A"
                                    selectByMouse: true
                                    onTextChanged: root.applyFilters()
                                }

                                Text {
                                    text: "Clear"
                                    color: "#64748B"
                                    font.pixelSize: 11
                                    visible: searchInput.text.length > 0
                                    MouseArea {
                                        anchors.fill: parent
                                        cursorShape: Qt.PointingHandCursor
                                        onClicked: {
                                            searchInput.text = ""
                                            root.applyFilters()
                                        }
                                    }
                                }
                            }
                        }
                    }
                }

                // Table Header
                Rectangle {
                    Layout.fillWidth: true
                    height: 38
                    color: "#0F172A"

                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: 12; anchors.rightMargin: 12
                        spacing: 8

                        Text { text: "SLIP #"; color: "#94A3B8"; font.pixelSize: 11; font.bold: true; Layout.preferredWidth: 160 }
                        Text { text: "DATE / TIME"; color: "#94A3B8"; font.pixelSize: 11; font.bold: true; Layout.preferredWidth: 95 }
                        Text { text: "VEHICLE NO"; color: "#94A3B8"; font.pixelSize: 11; font.bold: true; Layout.preferredWidth: 105 }
                        Text { text: "CONSIGNEE / BUYER"; color: "#94A3B8"; font.pixelSize: 11; font.bold: true; Layout.fillWidth: true }
                        Text { text: "TRANSPORTER / DRIVER"; color: "#94A3B8"; font.pixelSize: 11; font.bold: true; Layout.preferredWidth: 165 }
                        Text { text: "ITEM & GRADE"; color: "#94A3B8"; font.pixelSize: 11; font.bold: true; Layout.preferredWidth: 125 }
                        Text { text: "BAGS"; color: "#94A3B8"; font.pixelSize: 11; font.bold: true; Layout.preferredWidth: 55; horizontalAlignment: Text.AlignRight }
                        Text { text: "NET QTL"; color: "#94A3B8"; font.pixelSize: 11; font.bold: true; Layout.preferredWidth: 75; horizontalAlignment: Text.AlignRight }
                        Text { text: "TOTAL FREIGHT"; color: "#94A3B8"; font.pixelSize: 11; font.bold: true; Layout.preferredWidth: 95; horizontalAlignment: Text.AlignRight }
                        Text { text: "BALANCE DUE"; color: "#94A3B8"; font.pixelSize: 11; font.bold: true; Layout.preferredWidth: 95; horizontalAlignment: Text.AlignRight }
                        Text { text: "STATUS"; color: "#94A3B8"; font.pixelSize: 11; font.bold: true; Layout.preferredWidth: 80; horizontalAlignment: Text.AlignHCenter }
                        Text { text: "ACTIONS"; color: "#94A3B8"; font.pixelSize: 11; font.bold: true; Layout.preferredWidth: 75; horizontalAlignment: Text.AlignHCenter }
                    }
                }

                // Table List View
                ListView {
                    id: dispatchListView
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true
                    model: (typeof transportDispatchCtrl !== "undefined" && transportDispatchCtrl) ? transportDispatchCtrl.model : null

                    T.ScrollBar.vertical: T.ScrollBar {
                        policy: T.ScrollBar.AsNeeded
                    }

                    delegate: Rectangle {
                        width: dispatchListView.width
                        height: 42
                        color: index % 2 === 0 ? "#FFFFFF" : "#F8FAFC"

                        Rectangle {
                            anchors.bottom: parent.bottom
                            width: parent.width
                            height: 1
                            color: "#E2E8F0"
                        }

                        MouseArea {
                            anchors.fill: parent
                            hoverEnabled: true
                            onEntered: parent.color = "#EFF6FF"
                            onExited: parent.color = index % 2 === 0 ? "#FFFFFF" : "#F8FAFC"
                            onDoubleClicked: {
                                root.openEditModal(model.dispatchId)
                            }
                        }

                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: 12; anchors.rightMargin: 12
                            spacing: 8

                            Text {
                                text: model.slipNo || ""
                                color: "#2563EB"
                                font.pixelSize: 12
                                font.bold: true
                                elide: Text.ElideRight
                                Layout.preferredWidth: 160
                            }

                            Text {
                                text: (model.dispatchDate || "") + "\n" + (model.dispatchTime || "")
                                color: "#64748B"
                                font.pixelSize: 11
                                elide: Text.ElideRight
                                Layout.preferredWidth: 95
                            }

                            Text {
                                text: model.vehicleNo || ""
                                color: "#0F172A"
                                font.pixelSize: 12
                                font.bold: true
                                elide: Text.ElideRight
                                Layout.preferredWidth: 105
                            }

                            Text {
                                text: model.partyName || "-"
                                color: "#0F172A"
                                font.pixelSize: 12
                                elide: Text.ElideRight
                                Layout.fillWidth: true
                            }

                            ColumnLayout {
                                Layout.preferredWidth: 165
                                spacing: 1
                                Text {
                                    text: model.transporterName || "-"
                                    color: "#334155"
                                    font.pixelSize: 11
                                    font.bold: true
                                    elide: Text.ElideRight
                                    Layout.fillWidth: true
                                }
                                Text {
                                    text: (model.driverName ? model.driverName + " " : "") + (model.driverPhone || "")
                                    color: "#64748B"
                                    font.pixelSize: 10
                                    elide: Text.ElideRight
                                    Layout.fillWidth: true
                                }
                            }

                            Text {
                                text: (model.itemName || "-") + (model.grade ? " (" + model.grade + ")" : "")
                                color: "#475569"
                                font.pixelSize: 11
                                elide: Text.ElideRight
                                Layout.preferredWidth: 125
                            }

                            Text {
                                text: model.bagCount ? model.bagCount.toString() : "0"
                                color: "#334155"
                                font.pixelSize: 12
                                Layout.preferredWidth: 55
                                horizontalAlignment: Text.AlignRight
                            }

                            Text {
                                text: model.netWeightQtl ? model.netWeightQtl.toFixed(2) : "0.00"
                                color: "#15803D"
                                font.pixelSize: 12
                                font.bold: true
                                Layout.preferredWidth: 75
                                horizontalAlignment: Text.AlignRight
                            }

                            Text {
                                text: model.totalFreightFmt || "Rs. 0.00"
                                color: "#1D4ED8"
                                font.pixelSize: 12
                                font.bold: true
                                Layout.preferredWidth: 95
                                horizontalAlignment: Text.AlignRight
                            }

                            Text {
                                text: model.balanceFreightFmt || "Rs. 0.00"
                                color: model.balanceFreight > 0 ? "#DC2626" : "#15803D"
                                font.pixelSize: 12
                                font.bold: true
                                Layout.preferredWidth: 95
                                horizontalAlignment: Text.AlignRight
                            }

                            Rectangle {
                                Layout.preferredWidth: 80
                                height: 22
                                radius: 4
                                color: model.freightPaymentStatus === "Settled" ? "#DCFCE7" : (model.freightPaymentStatus === "Partially Paid" ? "#FEF3C7" : "#FEE2E2")
                                border.color: model.freightPaymentStatus === "Settled" ? "#86EFAC" : (model.freightPaymentStatus === "Partially Paid" ? "#FDE68A" : "#FCA5A5")
                                border.width: 1

                                Text {
                                    anchors.centerIn: parent
                                    text: model.freightPaymentStatus || "Unpaid"
                                    color: model.freightPaymentStatus === "Settled" ? "#166534" : (model.freightPaymentStatus === "Partially Paid" ? "#92400E" : "#991B1B")
                                    font.pixelSize: 10
                                    font.bold: true
                                }
                            }

                            RowLayout {
                                Layout.preferredWidth: 75
                                spacing: 4

                                T.Button {
                                    implicitWidth: 34
                                    implicitHeight: 26
                                    background: Rectangle { color: "#EFF6FF"; radius: 4; border.color: "#BFDBFE" }
                                    contentItem: Text { text: "Edit"; color: "#2563EB"; font.pixelSize: 11; font.bold: true; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                                    onClicked: root.openEditModal(model.dispatchId)
                                }

                                T.Button {
                                    implicitWidth: 34
                                    implicitHeight: 26
                                    background: Rectangle { color: "#FEF2F2"; radius: 4; border.color: "#FECACA" }
                                    contentItem: Text { text: "Del"; color: "#DC2626"; font.pixelSize: 11; font.bold: true; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                                    onClicked: {
                                        if (typeof transportDispatchCtrl !== "undefined") {
                                            transportDispatchCtrl.deleteDispatch(model.dispatchId)
                                        }
                                    }
                                }
                            }
                        }
                    }

                    // Empty state
                    Rectangle {
                        anchors.centerIn: parent
                        visible: dispatchListView.count === 0
                        width: 300
                        height: 100
                        color: "transparent"

                        ColumnLayout {
                            anchors.centerIn: parent
                            spacing: 8
                            Text {
                                text: "No Transport Dispatches Found"
                                color: "#64748B"
                                font.pixelSize: 14
                                font.bold: true
                                Layout.alignment: Qt.AlignHCenter
                            }
                            Text {
                                text: "Press 'Ctrl+N' or click '+ New Weighbridge Slip' to create an entry."
                                color: "#94A3B8"
                                font.pixelSize: 12
                                Layout.alignment: Qt.AlignHCenter
                            }
                        }
                    }
                }
            }
        }
    }

    Shortcut {
        sequence: "Ctrl+N"
        onActivated: root.openNewModal()
    }
    Shortcut {
        sequence: "Escape"
        onActivated: root.handleEscape()
    }
}
