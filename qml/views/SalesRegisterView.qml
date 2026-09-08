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
        if (typeof salesRegisterCtrl !== "undefined" && salesRegisterCtrl) {
            salesRegisterCtrl.reload()
        }
        Qt.callLater(function() {
            salesListView.forceActiveFocus()
            if (salesRegisterCtrl && salesRegisterCtrl.model.count > 0) {
                salesListView.currentIndex = 0
            }
        })
    }

    onVisibleChanged: {
        if (visible && typeof salesRegisterCtrl !== "undefined" && salesRegisterCtrl) {
            salesRegisterCtrl.reload()
        }
    }

    function openPrintModal() {
        if (!salesRegisterCtrl || salesListView.currentIndex < 0) return
        var it = salesRegisterCtrl.model.get(salesListView.currentIndex)
        if (!it) return
        regCopyModal.mode = "print"
        regCopyModal.invoiceNo = it.invNoVal || ""
        regCopyModal.customerName = it.partyNameVal || ""
        regCopyModal.open()
    }

    function openPdfModal() {
        if (!salesRegisterCtrl || salesListView.currentIndex < 0) return
        var it = salesRegisterCtrl.model.get(salesListView.currentIndex)
        if (!it) return
        regCopyModal.mode = "pdf"
        regCopyModal.invoiceNo = it.invNoVal || ""
        regCopyModal.customerName = it.partyNameVal || ""
        regCopyModal.open()
    }

    PrintCopyOptionsModal {
        id: regCopyModal
        onSelected: function(copyType) {
            if (!salesRegisterCtrl || salesListView.currentIndex < 0) return
            var it = salesRegisterCtrl.model.get(salesListView.currentIndex)
            if (!it || !it.invNoVal || typeof printExportCtrl === "undefined" || !printExportCtrl) return
            if (regCopyModal.mode === "print") {
                printExportCtrl.print_sales_invoice(it.invNoVal, copyType)
            } else {
                printExportCtrl.export_sales_invoice_pdf(it.invNoVal, "", copyType)
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
                    text: " Sales Invoices Register (Commercial Sales Bills)"
                    color: "#0F172A"
                    font.pixelSize: 20
                    font.bold: true
                }
                Text {
                    text: "Complete audit ledger of customer invoices, rice & byproduct dispatch billing, and output GST."
                    color: "#64748B"
                    font.pixelSize: 12
                }
            }

            Item { Layout.fillWidth: true }

            T.Button {
                id: printSelectedBtn
                implicitWidth: contentItem.implicitWidth + 20
                implicitHeight: 32
                background: Rectangle { color: printSelectedBtn.hovered ? "#047857" : "#059669"; radius: 6 }
                contentItem: RowLayout {
                    spacing: 6
                    Text { text: "Print Invoice"; color: "#FFFFFF"; font.pixelSize: 12; font.bold: true }
                    KbdBadge { text: "Ctrl+P"; badgeColor: "#064E3B"; textColor: "#A7F3D0"; borderColor: "#059669" }
                }
                onClicked: root.openPrintModal()
            }

            T.Button {
                id: exportPdfSelectedBtn
                implicitWidth: contentItem.implicitWidth + 20
                implicitHeight: 32
                background: Rectangle { color: exportPdfSelectedBtn.hovered ? "#0284C7" : "#0EA5E9"; radius: 6 }
                contentItem: RowLayout {
                    spacing: 6
                    Text { text: "Save PDF"; color: "#FFFFFF"; font.pixelSize: 12; font.bold: true }
                    KbdBadge { text: "Alt+P"; badgeColor: "#075985"; textColor: "#BAE6FD"; borderColor: "#0EA5E9" }
                }
                onClicked: root.openPdfModal()
            }

            T.Button {
                id: exportCsvBtn
                implicitWidth: contentItem.implicitWidth + 16
                implicitHeight: 32
                background: Rectangle { color: "#F8FAFC"; radius: 6; border.color: "#CBD5E1" }
                contentItem: RowLayout {
                    spacing: 6
                    Text { text: "Excel CSV"; color: "#334155"; font.pixelSize: 12; font.bold: true }
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
                title: "TOTAL SALES BILLS"
                value: (typeof salesRegisterCtrl !== "undefined" && salesRegisterCtrl) ? salesRegisterCtrl.totalInvoicesCount.toString() : "0"
                accentColor: "#2563EB"
                Layout.fillWidth: true
            }

            StatCard {
                title: "TOTAL OUTWARD WEIGHT"
                value: (typeof salesRegisterCtrl !== "undefined" && salesRegisterCtrl) ? salesRegisterCtrl.totalWeightFmt : "0.00 Qtl"
                accentColor: "#16A34A"
                Layout.fillWidth: true
            }

            StatCard {
                title: "TAXABLE BILL VALUE"
                value: (typeof salesRegisterCtrl !== "undefined" && salesRegisterCtrl) ? salesRegisterCtrl.totalTaxableFmt : "₹0.00"
                accentColor: "#D97706"
                Layout.fillWidth: true
            }

            StatCard {
                title: "GROSS REVENUE TOTAL"
                value: (typeof salesRegisterCtrl !== "undefined" && salesRegisterCtrl) ? salesRegisterCtrl.totalGrossFmt : "₹0.00"
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

                        Text { text: "Search Sales Bills:"; color: "#334155"; font.pixelSize: 12; font.bold: true }

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
                                        if (typeof salesRegisterCtrl !== "undefined" && salesRegisterCtrl) {
                                            salesRegisterCtrl.searchQuery = text
                                        }
                                    }
                                    Keys.onDownPressed: function(event) {
                                        event.accepted = true
                                        salesListView.forceActiveFocus()
                                        if (salesListView.currentIndex < 0 && salesListView.count > 0) salesListView.currentIndex = 0
                                    }
                                    Keys.onReturnPressed: function(event) {
                                        event.accepted = true
                                        salesListView.forceActiveFocus()
                                        if (salesListView.currentIndex < 0 && salesListView.count > 0) salesListView.currentIndex = 0
                                    }
                                    Keys.onEnterPressed: function(event) {
                                        event.accepted = true
                                        salesListView.forceActiveFocus()
                                        if (salesListView.currentIndex < 0 && salesListView.count > 0) salesListView.currentIndex = 0
                                    }
                                }

                                Text {
                                    visible: searchInput.text === ""
                                    text: "Filter by invoice no, customer name, commodity item, date..."
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
                        Text { text: "Invoice No"; color: "#475569"; font.pixelSize: 11; font.bold: true; Layout.preferredWidth: 120 }
                        Text { text: "Date"; color: "#475569"; font.pixelSize: 11; font.bold: true; Layout.preferredWidth: 80 }
                        Text { text: "Customer / Buyer"; color: "#475569"; font.pixelSize: 11; font.bold: true; Layout.fillWidth: true; elide: Text.ElideRight }
                        Text { text: "Commodity Item"; color: "#475569"; font.pixelSize: 11; font.bold: true; Layout.preferredWidth: 150; elide: Text.ElideRight }
                        Text { text: "Bags"; color: "#475569"; font.pixelSize: 11; font.bold: true; Layout.preferredWidth: 45; horizontalAlignment: Text.AlignRight }
                        Text { text: "Weight Qtl"; color: "#475569"; font.pixelSize: 11; font.bold: true; Layout.preferredWidth: 70; horizontalAlignment: Text.AlignRight }
                        Text { text: "Rate ₹"; color: "#475569"; font.pixelSize: 11; font.bold: true; Layout.preferredWidth: 70; horizontalAlignment: Text.AlignRight }
                        Text { text: "Taxable ₹"; color: "#475569"; font.pixelSize: 11; font.bold: true; Layout.preferredWidth: 95; horizontalAlignment: Text.AlignRight }
                        Text { text: "Taxes ₹"; color: "#475569"; font.pixelSize: 11; font.bold: true; Layout.preferredWidth: 75; horizontalAlignment: Text.AlignRight }
                        Text { text: "Total Amount ₹"; color: "#475569"; font.pixelSize: 11; font.bold: true; Layout.preferredWidth: 110; horizontalAlignment: Text.AlignRight }
                        Text { text: "Vehicle / Ref"; color: "#475569"; font.pixelSize: 11; font.bold: true; Layout.preferredWidth: 85 }
                    }
                }

                // Table List View
                ListView {
                    id: salesListView
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true
                    model: (typeof salesRegisterCtrl !== "undefined" && salesRegisterCtrl) ? salesRegisterCtrl.model : null
                    boundsBehavior: Flickable.StopAtBounds
                    focus: true
                    highlightFollowsCurrentItem: true
                    currentIndex: 0

                    Keys.onReturnPressed: function(event) {
                        event.accepted = true
                        if (currentIndex >= 0 && currentIndex < count) {
                            var it = salesRegisterCtrl.model.get(currentIndex)
                            if (it && it.invNoVal) root.openInvoiceRequested(it.invNoVal)
                        }
                    }
                    Keys.onEnterPressed: function(event) {
                        event.accepted = true
                        if (currentIndex >= 0 && currentIndex < count) {
                            var it = salesRegisterCtrl.model.get(currentIndex)
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
                        width: salesListView.width
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
                                salesListView.currentIndex = index
                                salesListView.forceActiveFocus()
                            }
                            onDoubleClicked: {
                                salesListView.currentIndex = index
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
                            Text { text: model.vchNoVal ? model.vchNoVal : ("Sale-" + (index+1)); color: "#64748B"; font.pixelSize: 11; Layout.preferredWidth: 75 }
                            Text { text: model.invNoVal; color: "#2563EB"; font.pixelSize: 11; font.bold: true; Layout.preferredWidth: 120 }
                            Text { text: model.dateVal; color: "#334155"; font.pixelSize: 11; Layout.preferredWidth: 80 }
                            Text { text: model.custVal; color: "#0F172A"; font.pixelSize: 11; font.bold: true; Layout.fillWidth: true; elide: Text.ElideRight }
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

                        Text { text: "GRAND TOTAL (" + ((typeof salesRegisterCtrl !== "undefined" && salesRegisterCtrl) ? salesRegisterCtrl.totalInvoicesCount.toString() : "0") + " Invoices):"; color: "#166534"; font.pixelSize: 12; font.bold: true }
                        Text { text: ((typeof salesRegisterCtrl !== "undefined" && salesRegisterCtrl) ? (salesRegisterCtrl.totalBagsCount.toString() + " Bags (" + salesRegisterCtrl.totalWeightFmt + ")") : ""); color: "#15803D"; font.pixelSize: 12; font.bold: true }
                        Item { Layout.fillWidth: true }
                        Text { text: "Taxable: " + ((typeof salesRegisterCtrl !== "undefined" && salesRegisterCtrl) ? salesRegisterCtrl.totalTaxableFmt : "₹0.00"); color: "#9A3412"; font.pixelSize: 12; font.bold: true }
                        Item { implicitWidth: 16 }
                        Text { text: "Gross Revenue: " + ((typeof salesRegisterCtrl !== "undefined" && salesRegisterCtrl) ? salesRegisterCtrl.totalGrossFmt : "₹0.00"); color: "#166534"; font.pixelSize: 13; font.bold: true }
                    }
                }
            }
        }
    }
}
