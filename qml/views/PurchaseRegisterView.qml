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

    property string searchQuery: ""
    property var allInvoices: []
    property int totalInvoicesCount: 0
    property int totalBagsCount: 0
    property real totalWeightQtl: 0.0
    property real totalTaxableAmt: 0.0
    property real totalGstAmt: 0.0
    property real totalGrossAmt: 0.0

    Component.onCompleted: {
        reloadPurchaseRegister()
        Qt.callLater(function() {
            purchaseListView.forceActiveFocus()
            if (purchaseRegisterModel.count > 0) purchaseListView.currentIndex = 0
        })
    }

    function reloadPurchaseRegister() {
        allInvoices = (typeof purchaseModel !== "undefined" && purchaseModel) ? purchaseModel.get_purchase_register() : []
        filterAndPopulateRegister()
    }

    function filterAndPopulateRegister() {
        purchaseRegisterModel.clear()
        totalInvoicesCount = 0
        totalBagsCount = 0
        totalWeightQtl = 0.0
        totalTaxableAmt = 0.0
        totalGstAmt = 0.0
        totalGrossAmt = 0.0

        var query = searchQuery.trim().toLowerCase()

        for (var i = 0; i < allInvoices.length; i++) {
            var inv = allInvoices[i]
            var invNo = inv.invoice_no ? String(inv.invoice_no) : ""
            var dt = inv.invoice_date ? String(inv.invoice_date) : ""
            var supp = inv.supplier_name ? String(inv.supplier_name) : ""
            var item = inv.item_name ? String(inv.item_name) : ""
            var veh = inv.vehicle_no ? String(inv.vehicle_no) : ""

            if (query !== "") {
                if (invNo.toLowerCase().indexOf(query) === -1 &&
                    supp.toLowerCase().indexOf(query) === -1 &&
                    item.toLowerCase().indexOf(query) === -1 &&
                    veh.toLowerCase().indexOf(query) === -1 &&
                    dt.toLowerCase().indexOf(query) === -1) {
                    continue
                }
            }

            var bags = inv.bag_count ? parseInt(inv.bag_count) : 0
            var w = inv.weight_qtl ? Number(inv.weight_qtl) : 0.0
            var r = inv.rate_per_qtl ? Number(inv.rate_per_qtl) : 0.0
            var tax = inv.taxable_amount ? Number(inv.taxable_amount) : 0.0
            var gAmt = inv.gst_amount ? Number(inv.gst_amount) : 0.0
            var tot = inv.total_amount ? Number(inv.total_amount) : 0.0

            purchaseRegisterModel.append({
                vchNoVal: inv.voucher_no ? String(inv.voucher_no) : ("Purc-" + (i + 1)),
                invNoVal: invNo,
                dateVal: dt,
                suppVal: supp,
                itemVal: item,
                bagsVal: bags > 0 ? bags : "-",
                weightVal: w > 0 ? w.toFixed(3) : "-",
                rateVal: r > 0 ? r.toFixed(2) : "-",
                taxableVal: tax > 0 ? tax.toFixed(2) : "-",
                gstVal: gAmt > 0 ? gAmt.toFixed(2) : "-",
                totalVal: tot.toFixed(2),
                vehVal: veh ? veh : "-"
            })

            totalInvoicesCount++
            totalBagsCount += bags
            totalWeightQtl += w
            totalTaxableAmt += tax
            totalGstAmt += gAmt
            totalGrossAmt += tot
        }
        if (purchaseRegisterModel.count > 0) {
            if (purchaseListView.currentIndex < 0 || purchaseListView.currentIndex >= purchaseRegisterModel.count) {
                purchaseListView.currentIndex = 0
            }
        }
    }

    GenericListModel { id: purchaseRegisterModel }

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
                    text: "📋 Purchase Invoices Register (Paddy & Raw Material Bills)"
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
                value: root.totalInvoicesCount.toString()
                accentColor: "#2563EB"
                Layout.fillWidth: true
            }

            StatCard {
                title: "TOTAL INWARD WEIGHT"
                value: (typeof dashboardCtrl !== "undefined" && dashboardCtrl) ? dashboardCtrl.format_qty(root.totalWeightQtl) : (root.totalWeightQtl.toFixed(2) + " Qtl")
                accentColor: "#16A34A"
                Layout.fillWidth: true
            }

            StatCard {
                title: "TAXABLE PROCUREMENT VALUE"
                value: (typeof dashboardCtrl !== "undefined" && dashboardCtrl) ? dashboardCtrl.format_inr(root.totalTaxableAmt) : ("₹" + root.totalTaxableAmt.toFixed(2))
                accentColor: "#D97706"
                Layout.fillWidth: true
            }

            StatCard {
                title: "GROSS PROCUREMENT TOTAL"
                value: (typeof dashboardCtrl !== "undefined" && dashboardCtrl) ? dashboardCtrl.format_inr(root.totalGrossAmt) : ("₹" + root.totalGrossAmt.toFixed(2))
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

                        Text { text: "🔍 Search Purchase Bills:"; color: "#334155"; font.pixelSize: 12; font.bold: true }

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
                                        root.searchQuery = text
                                        root.filterAndPopulateRegister()
                                    }
                                    Keys.onDownPressed: function(event) {
                                        event.accepted = true
                                        purchaseListView.forceActiveFocus()
                                        if (purchaseListView.currentIndex < 0 && purchaseRegisterModel.count > 0) purchaseListView.currentIndex = 0
                                    }
                                    Keys.onReturnPressed: function(event) {
                                        event.accepted = true
                                        purchaseListView.forceActiveFocus()
                                        if (purchaseListView.currentIndex < 0 && purchaseRegisterModel.count > 0) purchaseListView.currentIndex = 0
                                    }
                                    Keys.onEnterPressed: function(event) {
                                        event.accepted = true
                                        purchaseListView.forceActiveFocus()
                                        if (purchaseListView.currentIndex < 0 && purchaseRegisterModel.count > 0) purchaseListView.currentIndex = 0
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
                    model: purchaseRegisterModel
                    boundsBehavior: Flickable.StopAtBounds
                    focus: true
                    highlightFollowsCurrentItem: true
                    currentIndex: 0

                    Keys.onReturnPressed: function(event) {
                        event.accepted = true
                        if (currentIndex >= 0 && currentIndex < purchaseRegisterModel.count) {
                            var it = purchaseRegisterModel.get(currentIndex)
                            if (it && it.invNoVal) root.openInvoiceRequested(it.invNoVal)
                        }
                    }
                    Keys.onEnterPressed: function(event) {
                        event.accepted = true
                        if (currentIndex >= 0 && currentIndex < purchaseRegisterModel.count) {
                            var it = purchaseRegisterModel.get(currentIndex)
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
                        if (currentIndex < purchaseRegisterModel.count - 1) {
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
                            Text { text: Number(model.weightVal || 0).toFixed(2); color: "#16A34A"; font.pixelSize: 11; font.bold: true; Layout.preferredWidth: 70; horizontalAlignment: Text.AlignRight }
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

                        Text { text: "GRAND TOTAL (" + root.totalInvoicesCount.toString() + " Purchase Bills):"; color: "#166534"; font.pixelSize: 12; font.bold: true }
                        Text { text: root.totalBagsCount.toString() + " Bags (" + root.totalWeightQtl.toFixed(2) + " Qtl)"; color: "#15803D"; font.pixelSize: 12; font.bold: true }
                        Item { Layout.fillWidth: true }
                        Text { text: "Taxable: " + ((typeof dashboardCtrl !== "undefined" && dashboardCtrl) ? dashboardCtrl.format_inr(root.totalTaxableAmt) : ("₹" + root.totalTaxableAmt.toFixed(2))); color: "#9A3412"; font.pixelSize: 12; font.bold: true }
                        Item { implicitWidth: 16 }
                        Text { text: "Gross Procurement: " + ((typeof dashboardCtrl !== "undefined" && dashboardCtrl) ? dashboardCtrl.format_inr(root.totalGrossAmt) : ("₹" + root.totalGrossAmt.toFixed(2))); color: "#166534"; font.pixelSize: 13; font.bold: true }
                    }
                }
            }
        }
    }
}
