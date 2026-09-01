import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../components"

ScrollView {
    id: root
    contentWidth: availableWidth
    clip: true

    signal cancelRequested()
    signal openItemMovement(string itemName)

    property int totalItemsCount: 0
    property real totalClosingQty: 0.0
    property real totalClosingVal: 0.0
    property string searchQuery: ""
    property var allStockItems: []

    Component.onCompleted: {
        reloadStockRegister()
    }

    function reloadStockRegister() {
        allStockItems = (typeof stockItemsModel !== "undefined" && stockItemsModel) ? stockItemsModel.get_stock_register() : []
        filterAndPopulateRegister()
    }

    function filterAndPopulateRegister() {
        stockRegisterModel.clear()
        totalClosingQty = 0.0
        totalClosingVal = 0.0
        var count = 0
        
        var query = searchQuery.trim().toLowerCase()
        
        for (var i = 0; i < allStockItems.length; i++) {
            var it = allStockItems[i]
            var name = it.name ? it.name : ""
            var code = it.code ? it.code : ""
            var typeStr = it.item_type ? it.item_type : ""
            
            if (query !== "") {
                if (name.toLowerCase().indexOf(query) === -1 &&
                    code.toLowerCase().indexOf(query) === -1 &&
                    typeStr.toLowerCase().indexOf(query) === -1) {
                    continue
                }
            }

            var opQ = (it.opening_qty !== undefined && it.opening_qty !== null) ? Number(it.opening_qty) : 0.0
            var inQ = (it.inward_qty !== undefined && it.inward_qty !== null) ? Number(it.inward_qty) : 0.0
            var outQ = (it.outward_qty !== undefined && it.outward_qty !== null) ? Number(it.outward_qty) : 0.0
            var closeQ = (it.closing_qty !== undefined && it.closing_qty !== null) ? Number(it.closing_qty) : 0.0
            var rVal = (it.rate !== undefined && it.rate !== null) ? Number(it.rate) : 0.0
            var closeV = (it.closing_value !== undefined && it.closing_value !== null) ? Number(it.closing_value) : 0.0

            stockRegisterModel.append({
                idVal: it.id,
                nameVal: name,
                codeVal: code ? code : "-",
                typeVal: typeStr ? typeStr : "-",
                unitVal: it.unit ? it.unit : "Qtl",
                opBags: it.opening_bags || 0,
                inBags: it.inward_bags || 0,
                outBags: it.outward_bags || 0,
                closeBags: it.closing_bags || 0,
                opQtyVal: it.opening_qty_fmt || opQ.toFixed(2),
                inQtyVal: it.inward_qty_fmt || inQ.toFixed(2),
                outQtyVal: it.outward_qty_fmt || outQ.toFixed(2),
                closeQtyVal: it.closing_qty_fmt || closeQ.toFixed(2),
                closeQtyNum: closeQ,
                rateVal: it.rate_fmt || ("₹" + rVal.toFixed(2)),
                closeValVal: it.closing_value_fmt || ("₹" + closeV.toFixed(2)),
                closeValNum: closeV
            })
            totalClosingQty += closeQ
            totalClosingVal += closeV
            count++
        }
        totalItemsCount = count
    }

    ListModel { id: stockRegisterModel }

    ColumnLayout {
        width: root.availableWidth > 0 ? root.availableWidth : 1200
        spacing: 16

        // Page Header Bar
        RowLayout {
            Layout.fillWidth: true
            spacing: 12

            ColumnLayout {
                spacing: 2
                Text {
                    text: "📊 Real-time Stock Register & Audited Inventory"
                    color: "#0F172A"
                    font.pixelSize: 20
                    font.bold: true
                }
                Text {
                    text: "Itemized tracking of Opening, Inward, Outward & Closing Balances across all grains and commodities."
                    color: "#64748B"
                    font.pixelSize: 12
                }
            }

            Item { Layout.fillWidth: true }

            Button {
                id: backBtn
                background: Rectangle { color: backBtn.hovered ? "#475569" : "#334155"; radius: 6 }
                contentItem: Text { text: "← Back (Esc)"; color: "#F8FAFC"; font.pixelSize: 12; font.bold: true }
                onClicked: root.cancelRequested()
            }
        }

        Rectangle { Layout.fillWidth: true; height: 1; color: "#E2E8F0" }

        // STATS STRIP
        RowLayout {
            Layout.fillWidth: true
            spacing: 12

            StatCard {
                title: "TOTAL COMMODITIES"
                value: root.totalItemsCount.toString()
                accentColor: "#2563EB"
                Layout.fillWidth: true
            }

            StatCard {
                title: "NET CLOSING STOCK"
                value: (typeof dashboardCtrl !== "undefined" && dashboardCtrl) ? dashboardCtrl.format_qty(root.totalClosingQty) : (root.totalClosingQty.toFixed(1) + " Qtl")
                accentColor: "#16A34A"
                Layout.fillWidth: true
            }

            StatCard {
                title: "TOTAL STOCK VALUATION"
                value: (typeof dashboardCtrl !== "undefined" && dashboardCtrl) ? dashboardCtrl.format_inr(root.totalClosingVal) : ("₹" + root.totalClosingVal.toFixed(2))
                accentColor: "#D97706"
                Layout.fillWidth: true
            }
        }

        // PROPER GRID TABLE CONTAINER CARD WITH LIVE SEARCH BAR
        Rectangle {
            Layout.fillWidth: true
            implicitHeight: 520
            color: "#FFFFFF"
            border.color: "#3B82F6"
            border.width: 2
            radius: 10

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 12
                spacing: 8

                // Grid Card Title & Live Search Bar
                Rectangle {
                    Layout.fillWidth: true
                    height: 42
                    color: "#EFF6FF"
                    radius: 6

                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: 12
                        anchors.rightMargin: 12
                        spacing: 12

                        Text {
                            text: "📊 INVENTORY STOCK REGISTER GRID"
                            color: "#1D4ED8"
                            font.pixelSize: 12
                            font.bold: true
                            font.letterSpacing: 1.0
                        }

                        Item { Layout.fillWidth: true }

                        // SEARCH BAR CONTROL
                        Rectangle {
                            width: 320
                            height: 32
                            radius: 6
                            color: "#FFFFFF"
                            border.color: searchField.activeFocus ? "#2563EB" : "#CBD5E1"
                            border.width: searchField.activeFocus ? 2 : 1

                            RowLayout {
                                anchors.fill: parent
                                anchors.leftMargin: 10
                                anchors.rightMargin: 8
                                spacing: 6

                                Text { text: "🔍"; font.pixelSize: 12 }

                                TextField {
                                    id: searchField
                                    placeholderText: "Search by item name, SKU code or type..."
                                    color: "#0F172A"
                                    placeholderTextColor: "#94A3B8"
                                    font.pixelSize: 12
                                    background: null
                                    Layout.fillWidth: true
                                    onTextChanged: {
                                        root.searchQuery = text
                                        root.filterAndPopulateRegister()
                                    }
                                }

                                Text {
                                    text: "✕"
                                    color: "#94A3B8"
                                    font.pixelSize: 12
                                    font.bold: true
                                    visible: searchField.text.length > 0
                                    MouseArea {
                                        anchors.fill: parent
                                        onClicked: {
                                            searchField.text = ""
                                            root.searchQuery = ""
                                            root.filterAndPopulateRegister()
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
                    model: stockRegisterModel
                    clip: true
                    spacing: 1
                    boundsBehavior: Flickable.StopAtBounds

                    delegate: Rectangle {
                        id: rowRect
                        width: gridListView.width
                        height: 34
                        color: index % 2 === 0 ? "#FFFFFF" : "#F8FAFC"
                        border.color: mouseArea.containsMouse ? "#2563EB" : "#E2E8F0"
                        border.width: 1
                        radius: 2

                        MouseArea {
                            id: mouseArea
                            anchors.fill: parent
                            hoverEnabled: true
                            onClicked: root.openItemMovement(model.nameVal)
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
