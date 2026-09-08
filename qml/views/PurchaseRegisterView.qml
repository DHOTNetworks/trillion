import QtQuick
import QtQuick.Templates as T
import QtQuick.Layouts
import MahadevERP

T.ScrollView {
    id: root
    contentWidth: availableWidth
    clip: true

    signal cancelRequested()
    signal openInvoiceRequested(string invoiceNo)

    function handleEscape() {
        root.cancelRequested()
    }

    Component.onCompleted: {
        if (typeof purchaseRegisterCtrl !== "undefined" && purchaseRegisterCtrl) {
            purchaseRegisterCtrl.reload()
        }
        Qt.callLater(function() {
            purchaseListView.forceActiveFocus()
            if (purchaseRegisterCtrl && purchaseRegisterCtrl.model.count > 0) {
                purchaseListView.currentIndex = 0
            }
        })
    }

    onVisibleChanged: {
        if (visible && typeof purchaseRegisterCtrl !== "undefined" && purchaseRegisterCtrl) {
            purchaseRegisterCtrl.reload()
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
                    text: "Purchase Invoices Register (Paddy & Raw Material Bills)"
                    color: "#0F172A"
                    font.pixelSize: 20
                    font.bold: true
                }
                Text {
                    text: "Complete audit ledger of supplier bills, paddy procurement purchases, and input tax credits."
                    color: "#64748B"
                    font.pixelSize: 12
                }
            }

            Item { Layout.fillWidth: true }

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
                title: "TOTAL PURCHASE BILLS"
                value: (typeof purchaseRegisterCtrl !== "undefined" && purchaseRegisterCtrl) ? purchaseRegisterCtrl.totalInvoicesCount.toString() : "0"
                accentColor: "#2563EB"
                Layout.fillWidth: true
            }

            StatCard {
                title: "TOTAL INWARD WEIGHT"
                value: (typeof purchaseRegisterCtrl !== "undefined" && purchaseRegisterCtrl) ? purchaseRegisterCtrl.totalWeightFmt : "0.00 Qtl"
                accentColor: "#16A34A"
                Layout.fillWidth: true
            }

            StatCard {
                title: "TAXABLE PROCUREMENT VALUE"
                value: (typeof purchaseRegisterCtrl !== "undefined" && purchaseRegisterCtrl) ? purchaseRegisterCtrl.totalTaxableFmt : "₹0.00"
                accentColor: "#D97706"
                Layout.fillWidth: true
            }

            StatCard {
                title: "GROSS PROCUREMENT TOTAL"
                value: (typeof purchaseRegisterCtrl !== "undefined" && purchaseRegisterCtrl) ? purchaseRegisterCtrl.totalGrossFmt : "₹0.00"
                accentColor: "#059669"
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

                // Search & Filter Toolbar
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

                        Text { text: "Search Purchase Bills:"; color: "#334155"; font.pixelSize: 12; font.bold: true }

                        Rectangle {
                            Layout.fillWidth: true
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
                                    onTextChanged: {
                                        if (typeof purchaseRegisterCtrl !== "undefined" && purchaseRegisterCtrl) {
                                            purchaseRegisterCtrl.searchQuery = text
                                        }
                                    }
                                    Keys.onDownPressed: function(event) {
                                        event.accepted = true
                                        purchaseListView.forceActiveFocus()
                                        if (purchaseListView.currentIndex < 0 && purchaseListView.count > 0) purchaseListView.currentIndex = 0
                                    }
                                    Keys.onReturnPressed: function(event) {
                                        event.accepted = true
                                        purchaseListView.forceActiveFocus()
                                        if (purchaseListView.currentIndex < 0 && purchaseListView.count > 0) purchaseListView.currentIndex = 0
                                    }
                                    Keys.onEnterPressed: function(event) {
                                        event.accepted = true
                                        purchaseListView.forceActiveFocus()
                                        if (purchaseListView.currentIndex < 0 && purchaseListView.count > 0) purchaseListView.currentIndex = 0
                                    }
                                }

                                Text {
                                    visible: searchInput.text === ""
                                    text: "Filter by bill no, supplier name, commodity item, date..."
                                    color: "#94A3B8"
                                    font.pixelSize: 12
                                }
                            }
                        }

                        T.Button {
                            implicitWidth: 60
                            implicitHeight: 28
                            text: "Clear"
                            visible: searchInput.text !== ""
                            onClicked: { searchInput.text = ""; searchInput.forceActiveFocus() }
                        }
                    }
                }

                // Table Header Bar
                Rectangle {
                    Layout.fillWidth: true
                    height: 36
                    color: "#F1F5F9"
                    border.color: "#CBD5E1"
                    border.width: 1

                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: 12; anchors.rightMargin: 12
                        spacing: 6

                        Text { text: "#"; color: "#475569"; font.pixelSize: 11; font.bold: true; Layout.preferredWidth: 30 }
                        Text { text: "Vch No"; color: "#475569"; font.pixelSize: 11; font.bold: true; Layout.preferredWidth: 75 }
                        Text { text: "Bill / Inv No"; color: "#475569"; font.pixelSize: 11; font.bold: true; Layout.preferredWidth: 120 }
                        Text { text: "Date"; color: "#475569"; font.pixelSize: 11; font.bold: true; Layout.preferredWidth: 80 }
                        Text { text: "Supplier / Farmer"; color: "#475569"; font.pixelSize: 11; font.bold: true; Layout.fillWidth: true; elide: Text.ElideRight }
                        Text { text: "Commodity Item"; color: "#475569"; font.pixelSize: 11; font.bold: true; Layout.preferredWidth: 150; elide: Text.ElideRight }
                        Text { text: "Bags"; color: "#475569"; font.pixelSize: 11; font.bold: true; Layout.preferredWidth: 45; horizontalAlignment: Text.AlignRight }
                        Text { text: "Weight Qtl"; color: "#475569"; font.pixelSize: 11; font.bold: true; Layout.preferredWidth: 70; horizontalAlignment: Text.AlignRight }
                        Text { text: "Rate ₹"; color: "#475569"; font.pixelSize: 11; font.bold: true; Layout.preferredWidth: 70; horizontalAlignment: Text.AlignRight }
                        Text { text: "Taxable ₹"; color: "#475569"; font.pixelSize: 11; font.bold: true; Layout.preferredWidth: 95; horizontalAlignment: Text.AlignRight }
                        Text { text: "Taxes ₹"; color: "#475569"; font.pixelSize: 11; font.bold: true; Layout.preferredWidth: 75; horizontalAlignment: Text.AlignRight }
                        Text { text: "Total Bill ₹"; color: "#475569"; font.pixelSize: 11; font.bold: true; Layout.preferredWidth: 110; horizontalAlignment: Text.AlignRight }
                        Text { text: "Vehicle / Ref"; color: "#475569"; font.pixelSize: 11; font.bold: true; Layout.preferredWidth: 85 }
                    }
                }

                // Table List View
                ListView {
                    id: purchaseListView
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true
                    model: (typeof purchaseRegisterCtrl !== "undefined" && purchaseRegisterCtrl) ? purchaseRegisterCtrl.model : null
                    boundsBehavior: Flickable.StopAtBounds
                    focus: true
                    highlightFollowsCurrentItem: true
                    currentIndex: 0

                    Keys.onReturnPressed: function(event) {
                        event.accepted = true
                        if (currentIndex >= 0 && currentIndex < count) {
                            var it = purchaseRegisterCtrl.model.get(currentIndex)
                            if (it && it.invNoVal) root.openInvoiceRequested(it.invNoVal)
                        }
                    }
                    Keys.onEnterPressed: function(event) {
                        event.accepted = true
                        if (currentIndex >= 0 && currentIndex < count) {
                            var it = purchaseRegisterCtrl.model.get(currentIndex)
                            if (it && it.invNoVal) root.openInvoiceRequested(it.invNoVal)
                        }
                    }
                    Keys.onUpPressed: function(event) {
                        event.accepted = true
                        if (currentIndex > 0) {
                            currentIndex--
                            positionViewAtIndex(currentIndex, ListView.Contain)
                        } else {
                            searchInput.forceActiveFocus()
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
                        width: purchaseListView.width
                        height: 34
                        color: ListView.isCurrentItem ? "#DBEAFE" : (rowMouseArea.containsMouse ? "#EFF6FF" : (index % 2 === 0 ? "#FFFFFF" : "#F8FAFC"))
                        border.color: ListView.isCurrentItem ? "#2563EB" : "#F1F5F9"
                        border.width: ListView.isCurrentItem ? 2 : 1

                        MouseArea {
                            id: rowMouseArea
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: {
                                purchaseListView.currentIndex = index
                                purchaseListView.forceActiveFocus()
                            }
                            onDoubleClicked: {
                                purchaseListView.currentIndex = index
                                if (model.invNoVal) {
                                    root.openInvoiceRequested(model.invNoVal)
                                }
                            }
                        }

                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: 12; anchors.rightMargin: 12
                            spacing: 6

                            Text { text: (index + 1).toString(); color: "#64748B"; font.pixelSize: 11; Layout.preferredWidth: 30 }
                            Text { text: model.vchNoVal ? model.vchNoVal : ("Purc-" + (index+1)); color: "#64748B"; font.pixelSize: 11; Layout.preferredWidth: 75 }
                            Text { text: model.invNoVal; color: "#2563EB"; font.pixelSize: 11; font.bold: true; Layout.preferredWidth: 120 }
                            Text { text: model.dateVal; color: "#334155"; font.pixelSize: 11; Layout.preferredWidth: 80 }
                            Text { text: model.suppVal; color: "#0F172A"; font.pixelSize: 11; font.bold: true; Layout.fillWidth: true; elide: Text.ElideRight }
                            Text { text: model.itemVal; color: "#475569"; font.pixelSize: 11; Layout.preferredWidth: 150; elide: Text.ElideRight }
                            Text { text: model.bagsVal.toString(); color: "#334155"; font.pixelSize: 11; Layout.preferredWidth: 45; horizontalAlignment: Text.AlignRight }
                            Text { text: model.weightVal ? model.weightVal.toString() : "-"; color: "#16A34A"; font.pixelSize: 11; font.bold: true; Layout.preferredWidth: 70; horizontalAlignment: Text.AlignRight }
                            Text { text: model.rateFmt; color: "#334155"; font.pixelSize: 11; Layout.preferredWidth: 70; horizontalAlignment: Text.AlignRight }
                            Text { text: model.taxableFmt; color: "#0F172A"; font.pixelSize: 11; font.bold: true; Layout.preferredWidth: 95; horizontalAlignment: Text.AlignRight }
                            Text { text: model.gstFmt; color: "#64748B"; font.pixelSize: 11; Layout.preferredWidth: 75; horizontalAlignment: Text.AlignRight }
                            Text { text: model.totalFmt; color: "#15803D"; font.pixelSize: 12; font.bold: true; Layout.preferredWidth: 110; horizontalAlignment: Text.AlignRight }
                            Text { text: model.vehVal ? model.vehVal : "-"; color: "#64748B"; font.pixelSize: 11; Layout.preferredWidth: 85; elide: Text.ElideRight }
                        }
                    }
                }

                // Bottom Grand Total Summary Bar
                Rectangle {
                    Layout.fillWidth: true
                    height: 40
                    color: "#F0FDF4"
                    border.color: "#86EFAC"
                    border.width: 1.5

                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: 16; anchors.rightMargin: 16
                        spacing: 12

                        Text { text: "GRAND TOTAL (" + ((typeof purchaseRegisterCtrl !== "undefined" && purchaseRegisterCtrl) ? purchaseRegisterCtrl.totalInvoicesCount.toString() : "0") + " Purchase Bills):"; color: "#166534"; font.pixelSize: 12; font.bold: true }
                        Text { text: ((typeof purchaseRegisterCtrl !== "undefined" && purchaseRegisterCtrl) ? (purchaseRegisterCtrl.totalBagsCount.toString() + " Bags (" + purchaseRegisterCtrl.totalWeightFmt + ")") : ""); color: "#15803D"; font.pixelSize: 12; font.bold: true }
                        Item { Layout.fillWidth: true }
                        Text { text: "Taxable: " + ((typeof purchaseRegisterCtrl !== "undefined" && purchaseRegisterCtrl) ? purchaseRegisterCtrl.totalTaxableFmt : "₹0.00"); color: "#9A3412"; font.pixelSize: 12; font.bold: true }
                        Item { implicitWidth: 16 }
                        Text { text: "Gross Procurement: " + ((typeof purchaseRegisterCtrl !== "undefined" && purchaseRegisterCtrl) ? purchaseRegisterCtrl.totalGrossFmt : "₹0.00"); color: "#166534"; font.pixelSize: 13; font.bold: true }
                    }
                }
            }
        }
    }
}
