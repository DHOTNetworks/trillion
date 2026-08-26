import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../components"

Rectangle {
    id: root
    width: 1080
    height: 620
    color: "#FFFFFF"
    radius: 12
    border.color: "#CBD5E1"
    border.width: 1

    signal closeRequested()

    property string itemName: ""
    property real totalInwardQty: 0.0
    property real totalInwardVal: 0.0
    property real totalOutwardQty: 0.0
    property real totalOutwardVal: 0.0
    
    property real selectedInwardQty: 0.0
    property real selectedInwardVal: 0.0
    property real selectedOutwardQty: 0.0
    property real selectedOutwardVal: 0.0
    
    property int selectedInwardCount: 0
    property int selectedOutwardCount: 0

    function loadItemMovements(name) {
        itemName = name
        inwardModel.clear()
        outwardModel.clear()
        totalInwardQty = 0.0
        totalInwardVal = 0.0
        totalOutwardQty = 0.0
        totalOutwardVal = 0.0

        if (!name) {
            recalculateInwardTotals()
            recalculateOutwardTotals()
            return
        }

        var movs = (typeof stockItemsModel !== "undefined" && stockItemsModel) ? stockItemsModel.get_item_movements(name) : []
        for (var i = 0; i < movs.length; i++) {
            var m = movs[i]
            if (m.isInward) {
                inwardModel.append({
                    isSelected: false,
                    vDate: m.vDate,
                    refNo: m.refNo,
                    party: m.party,
                    bags: m.bags,
                    qty: m.qty,
                    rate: m.rate,
                    amount: m.amount
                })
                totalInwardQty += m.qty
                totalInwardVal += m.amount
            } else {
                outwardModel.append({
                    isSelected: false,
                    vDate: m.vDate,
                    refNo: m.refNo,
                    party: m.party,
                    bags: m.bags,
                    qty: m.qty,
                    rate: m.rate,
                    amount: m.amount
                })
                totalOutwardQty += m.qty
                totalOutwardVal += m.amount
            }
        }
        recalculateInwardTotals()
        recalculateOutwardTotals()
    }

    function toggleSelectAllInwards(state) {
        for (var i = 0; i < inwardModel.count; i++) {
            inwardModel.setProperty(i, "isSelected", state)
        }
        recalculateInwardTotals()
    }

    function toggleSelectAllOutwards(state) {
        for (var i = 0; i < outwardModel.count; i++) {
            outwardModel.setProperty(i, "isSelected", state)
        }
        recalculateOutwardTotals()
    }

    function recalculateInwardTotals() {
        var count = 0
        var qty = 0.0
        var val = 0.0
        var hasSelections = false

        for (var i = 0; i < inwardModel.count; i++) {
            var row = inwardModel.get(i)
            if (row.isSelected) {
                hasSelections = true
                count++
                qty += row.qty
                val += row.amount
            }
        }
        selectedInwardCount = count

        if (hasSelections) {
            selectedInwardQty = qty
            selectedInwardVal = val
        } else {
            selectedInwardQty = totalInwardQty
            selectedInwardVal = totalInwardVal
        }
    }

    function recalculateOutwardTotals() {
        var count = 0
        var qty = 0.0
        var val = 0.0
        var hasSelections = false

        for (var i = 0; i < outwardModel.count; i++) {
            var row = outwardModel.get(i)
            if (row.isSelected) {
                hasSelections = true
                count++
                qty += row.qty
                val += row.amount
            }
        }
        selectedOutwardCount = count

        if (hasSelections) {
            selectedOutwardQty = qty
            selectedOutwardVal = val
        } else {
            selectedOutwardQty = totalOutwardQty
            selectedOutwardVal = totalOutwardVal
        }
    }

    ListModel { id: inwardModel }
    ListModel { id: outwardModel }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 18
        spacing: 12

        // Modal Header Bar
        RowLayout {
            Layout.fillWidth: true

            RowLayout {
                spacing: 10
                Rectangle {
                    width: 36; height: 36; radius: 8; color: "#EFF6FF"
                    Text { anchors.centerIn: parent; text: "📦"; font.pixelSize: 18 }
                }
                ColumnLayout {
                    spacing: 0
                    Text { text: "ITEM MOVEMENT & TRANSACTION REGISTER"; color: "#2563EB"; font.pixelSize: 11; font.bold: true; font.letterSpacing: 1.0 }
                    Text { text: root.itemName !== "" ? root.itemName : "Stock Item Details"; color: "#0F172A"; font.pixelSize: 18; font.bold: true }
                }
            }

            Item { Layout.fillWidth: true }

            Button {
                id: closeBtn
                width: 32
                height: 32
                background: Rectangle { color: closeBtn.hovered ? "#DC2626" : "#F1F5F9"; radius: 16 }
                contentItem: Text { text: "✕"; color: closeBtn.hovered ? "#FFF" : "#475569"; font.bold: true; horizontalAlignment: Text.AlignHCenter }
                onClicked: root.closeRequested()
            }
        }

        Rectangle { Layout.fillWidth: true; height: 1; color: "#E2E8F0" }

        // SIDE-BY-SIDE 2 SECTIONS: INWARDS & OUTWARDS WITH CHECKBOXES
        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 14

            // LEFT COLUMN: INWARDS / ARRIVALS & PURCHASES
            Rectangle {
                Layout.fillWidth: true
                Layout.fillHeight: true
                color: "#FFFFFF"
                border.color: "#BFDBFE"
                border.width: 1.5
                radius: 10
                clip: true

                ColumnLayout {
                    anchors.fill: parent
                    spacing: 0

                    // Section Title Header
                    Rectangle {
                        Layout.fillWidth: true
                        height: 36
                        color: "#EFF6FF"
                        border.color: "#BFDBFE"
                        border.width: 1

                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: 12; anchors.rightMargin: 12
                            Text { text: "📥 INWARDS / ARRIVALS & PURCHASES"; color: "#1D4ED8"; font.pixelSize: 12; font.bold: true }
                            Item { Layout.fillWidth: true }
                            Text { text: inwardModel.count.toString() + " Entries"; color: "#2563EB"; font.pixelSize: 11; font.bold: true }
                        }
                    }

                    // Inwards Table Header Grid with Master Checkbox
                    Rectangle {
                        Layout.fillWidth: true
                        height: 30
                        color: "#F8FAFC"
                        border.color: "#E2E8F0"
                        border.width: 1

                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: 6; anchors.rightMargin: 6
                            spacing: 4

                            // Master Inwards Checkbox
                            Rectangle {
                                width: 26; height: parent.height
                                color: "transparent"
                                Rectangle {
                                    anchors.centerIn: parent
                                    width: 16; height: 16; radius: 3
                                    color: inMasterCheck.containsMouse ? "#DBEAFE" : "#FFFFFF"
                                    border.color: "#2563EB"; border.width: 1.5

                                    MouseArea {
                                        id: inMasterCheck
                                        anchors.fill: parent
                                        hoverEnabled: true
                                        onClicked: {
                                            var newState = !(root.selectedInwardCount === inwardModel.count && inwardModel.count > 0)
                                            root.toggleSelectAllInwards(newState)
                                        }
                                    }
                                    Text {
                                        anchors.centerIn: parent; text: "✓"; color: "#2563EB"; font.bold: true; font.pixelSize: 11
                                        visible: root.selectedInwardCount === inwardModel.count && inwardModel.count > 0
                                    }
                                }
                            }

                            Text { text: "Date"; color: "#475569"; font.pixelSize: 11; font.bold: true; Layout.preferredWidth: 75 }
                            Text { text: "Ref No"; color: "#475569"; font.pixelSize: 11; font.bold: true; Layout.preferredWidth: 85 }
                            Text { text: "Supplier / Farmer"; color: "#475569"; font.pixelSize: 11; font.bold: true; Layout.fillWidth: true; elide: Text.ElideRight }
                            Text { text: "Bags"; color: "#475569"; font.pixelSize: 11; font.bold: true; Layout.preferredWidth: 40; horizontalAlignment: Text.AlignRight }
                            Text { text: "Qtl"; color: "#475569"; font.pixelSize: 11; font.bold: true; Layout.preferredWidth: 55; horizontalAlignment: Text.AlignRight }
                            Text { text: "Amount ₹"; color: "#475569"; font.pixelSize: 11; font.bold: true; Layout.preferredWidth: 85; horizontalAlignment: Text.AlignRight }
                        }
                    }

                    // ListView Body
                    ListView {
                        id: inwardListView
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        clip: true
                        model: inwardModel
                        spacing: 1

                        delegate: Rectangle {
                            width: inwardListView.width
                            height: 32
                            color: model.isSelected ? "#EFF6FF" : (index % 2 === 0 ? "#FFFFFF" : "#F8FAFC")
                            border.color: "#F1F5F9"

                            RowLayout {
                                anchors.fill: parent
                                anchors.leftMargin: 6; anchors.rightMargin: 6
                                spacing: 4

                                // Row Checkbox
                                Rectangle {
                                    width: 26; height: parent.height
                                    color: "transparent"
                                    Rectangle {
                                        anchors.centerIn: parent
                                        width: 16; height: 16; radius: 3
                                        color: model.isSelected ? "#2563EB" : "#FFFFFF"
                                        border.color: "#2563EB"; border.width: 1.5

                                        MouseArea {
                                            anchors.fill: parent
                                            onClicked: {
                                                inwardModel.setProperty(index, "isSelected", !model.isSelected)
                                                root.recalculateInwardTotals()
                                            }
                                        }
                                        Text {
                                            anchors.centerIn: parent; text: "✓"; color: "#FFFFFF"; font.bold: true; font.pixelSize: 11
                                            visible: model.isSelected
                                        }
                                    }
                                }

                                Text { text: model.vDate ? model.vDate : ""; color: "#334155"; font.pixelSize: 11; Layout.preferredWidth: 75 }
                                Text { text: model.refNo ? model.refNo : ""; color: "#2563EB"; font.pixelSize: 11; font.bold: true; Layout.preferredWidth: 85 }
                                Text { text: model.party ? model.party : ""; color: "#0F172A"; font.pixelSize: 11; Layout.fillWidth: true; elide: Text.ElideRight }
                                Text { text: model.bags ? model.bags.toString() : "0"; color: "#334155"; font.pixelSize: 11; Layout.preferredWidth: 40; horizontalAlignment: Text.AlignRight }
                                Text { text: model.qty ? model.qty.toFixed(2) : "0.00"; color: "#16A34A"; font.pixelSize: 11; font.bold: true; Layout.preferredWidth: 55; horizontalAlignment: Text.AlignRight }
                                Text { text: model.amount ? "₹" + model.amount.toLocaleString(Qt.locale(), "f", 2) : "₹0.00"; color: "#0F172A"; font.pixelSize: 11; font.bold: true; Layout.preferredWidth: 85; horizontalAlignment: Text.AlignRight }
                            }
                        }
                    }

                    // Inward Section Bottom Summary Bar (DYNAMICALLY UPDATED)
                    Rectangle {
                        Layout.fillWidth: true
                        height: 34
                        color: root.selectedInwardCount > 0 ? "#DCFCE7" : "#F0FDF4"
                        border.color: root.selectedInwardCount > 0 ? "#22C55E" : "#BBF7D0"
                        border.width: 1

                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: 12; anchors.rightMargin: 12
                            Text {
                                text: root.selectedInwardCount > 0 ? "Selected Inward Qty (" + root.selectedInwardCount.toString() + "):" : "Total Inward Quantity:"
                                color: "#166534"; font.pixelSize: 11; font.bold: true
                            }
                            Text { text: root.selectedInwardQty.toFixed(2) + " Qtl"; color: "#15803D"; font.pixelSize: 12; font.bold: true }
                            Item { Layout.fillWidth: true }
                            Text { text: "Total: ₹" + root.selectedInwardVal.toLocaleString(Qt.locale(), "f", 2); color: "#166534"; font.pixelSize: 11; font.bold: true }
                        }
                    }
                }
            }

            // RIGHT COLUMN: OUTWARDS / DISPATCHES & SALES
            Rectangle {
                Layout.fillWidth: true
                Layout.fillHeight: true
                color: "#FFFFFF"
                border.color: "#FED7AA"
                border.width: 1.5
                radius: 10
                clip: true

                ColumnLayout {
                    anchors.fill: parent
                    spacing: 0

                    // Section Title Header
                    Rectangle {
                        Layout.fillWidth: true
                        height: 36
                        color: "#FFF7ED"
                        border.color: "#FED7AA"
                        border.width: 1

                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: 12; anchors.rightMargin: 12
                            Text { text: "📤 OUTWARDS / SALES & DISPATCHES"; color: "#C2410C"; font.pixelSize: 12; font.bold: true }
                            Item { Layout.fillWidth: true }
                            Text { text: outwardModel.count.toString() + " Entries"; color: "#EA580C"; font.pixelSize: 11; font.bold: true }
                        }
                    }

                    // Outwards Table Header Grid with Master Checkbox
                    Rectangle {
                        Layout.fillWidth: true
                        height: 30
                        color: "#F8FAFC"
                        border.color: "#E2E8F0"
                        border.width: 1

                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: 6; anchors.rightMargin: 6
                            spacing: 4

                            // Master Outwards Checkbox
                            Rectangle {
                                width: 26; height: parent.height
                                color: "transparent"
                                Rectangle {
                                    anchors.centerIn: parent
                                    width: 16; height: 16; radius: 3
                                    color: outMasterCheck.containsMouse ? "#FFEDD5" : "#FFFFFF"
                                    border.color: "#EA580C"; border.width: 1.5

                                    MouseArea {
                                        id: outMasterCheck
                                        anchors.fill: parent
                                        hoverEnabled: true
                                        onClicked: {
                                            var newState = !(root.selectedOutwardCount === outwardModel.count && outwardModel.count > 0)
                                            root.toggleSelectAllOutwards(newState)
                                        }
                                    }
                                    Text {
                                        anchors.centerIn: parent; text: "✓"; color: "#EA580C"; font.bold: true; font.pixelSize: 11
                                        visible: root.selectedOutwardCount === outwardModel.count && outwardModel.count > 0
                                    }
                                }
                            }

                            Text { text: "Date"; color: "#475569"; font.pixelSize: 11; font.bold: true; Layout.preferredWidth: 75 }
                            Text { text: "Ref No"; color: "#475569"; font.pixelSize: 11; font.bold: true; Layout.preferredWidth: 85 }
                            Text { text: "Buyer / Customer"; color: "#475569"; font.pixelSize: 11; font.bold: true; Layout.fillWidth: true; elide: Text.ElideRight }
                            Text { text: "Bags"; color: "#475569"; font.pixelSize: 11; font.bold: true; Layout.preferredWidth: 40; horizontalAlignment: Text.AlignRight }
                            Text { text: "Qtl"; color: "#475569"; font.pixelSize: 11; font.bold: true; Layout.preferredWidth: 55; horizontalAlignment: Text.AlignRight }
                            Text { text: "Amount ₹"; color: "#475569"; font.pixelSize: 11; font.bold: true; Layout.preferredWidth: 85; horizontalAlignment: Text.AlignRight }
                        }
                    }

                    // ListView Body
                    ListView {
                        id: outwardListView
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        clip: true
                        model: outwardModel
                        spacing: 1

                        delegate: Rectangle {
                            width: outwardListView.width
                            height: 32
                            color: model.isSelected ? "#FFF7ED" : (index % 2 === 0 ? "#FFFFFF" : "#F8FAFC")
                            border.color: "#F1F5F9"

                            RowLayout {
                                anchors.fill: parent
                                anchors.leftMargin: 6; anchors.rightMargin: 6
                                spacing: 4

                                // Row Checkbox
                                Rectangle {
                                    width: 26; height: parent.height
                                    color: "transparent"
                                    Rectangle {
                                        anchors.centerIn: parent
                                        width: 16; height: 16; radius: 3
                                        color: model.isSelected ? "#EA580C" : "#FFFFFF"
                                        border.color: "#EA580C"; border.width: 1.5

                                        MouseArea {
                                            anchors.fill: parent
                                            onClicked: {
                                                outwardModel.setProperty(index, "isSelected", !model.isSelected)
                                                root.recalculateOutwardTotals()
                                            }
                                        }
                                        Text {
                                            anchors.centerIn: parent; text: "✓"; color: "#FFFFFF"; font.bold: true; font.pixelSize: 11
                                            visible: model.isSelected
                                        }
                                    }
                                }

                                Text { text: model.vDate ? model.vDate : ""; color: "#334155"; font.pixelSize: 11; Layout.preferredWidth: 75 }
                                Text { text: model.refNo ? model.refNo : ""; color: "#EA580C"; font.pixelSize: 11; font.bold: true; Layout.preferredWidth: 85 }
                                Text { text: model.party ? model.party : ""; color: "#0F172A"; font.pixelSize: 11; Layout.fillWidth: true; elide: Text.ElideRight }
                                Text { text: model.bags ? model.bags.toString() : "0"; color: "#334155"; font.pixelSize: 11; Layout.preferredWidth: 40; horizontalAlignment: Text.AlignRight }
                                Text { text: model.qty ? model.qty.toFixed(2) : "0.00"; color: "#DC2626"; font.pixelSize: 11; font.bold: true; Layout.preferredWidth: 55; horizontalAlignment: Text.AlignRight }
                                Text { text: model.amount ? "₹" + model.amount.toLocaleString(Qt.locale(), "f", 2) : "₹0.00"; color: "#0F172A"; font.pixelSize: 11; font.bold: true; Layout.preferredWidth: 85; horizontalAlignment: Text.AlignRight }
                            }
                        }
                    }

                    // Outward Section Bottom Summary Bar (DYNAMICALLY UPDATED)
                    Rectangle {
                        Layout.fillWidth: true
                        height: 34
                        color: root.selectedOutwardCount > 0 ? "#FFEDD5" : "#FFF7ED"
                        border.color: root.selectedOutwardCount > 0 ? "#F97316" : "#FFEDD5"
                        border.width: 1

                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: 12; anchors.rightMargin: 12
                            Text {
                                text: root.selectedOutwardCount > 0 ? "Selected Outward Qty (" + root.selectedOutwardCount.toString() + "):" : "Total Outward Quantity:"
                                color: "#9A3412"; font.pixelSize: 11; font.bold: true
                            }
                            Text { text: root.selectedOutwardQty.toFixed(2) + " Qtl"; color: "#C2410C"; font.pixelSize: 12; font.bold: true }
                            Item { Layout.fillWidth: true }
                            Text { text: "Total: ₹" + root.selectedOutwardVal.toLocaleString(Qt.locale(), "f", 2); color: "#9A3412"; font.pixelSize: 11; font.bold: true }
                        }
                    }
                }
            }
        }

        // FOOTER NET CLOSING BALANCE RECONCILIATION BAR (DYNAMICALLY UPDATED BASED ON SELECTED ENTRIES)
        Rectangle {
            Layout.fillWidth: true
            height: 42
            color: "#0F172A"
            radius: 8

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 16; anchors.rightMargin: 16

                Text { text: (root.selectedInwardCount > 0 || root.selectedOutwardCount > 0) ? "SELECTED ENTRIES NET BALANCE:" : "ITEM NET CLOSING BALANCE:"; color: "#94A3B8"; font.pixelSize: 12; font.bold: true }
                Text {
                    text: (root.selectedInwardQty - root.selectedOutwardQty).toFixed(2) + " Qtl"
                    color: (root.selectedInwardQty - root.selectedOutwardQty) >= 0 ? "#4ADE80" : "#F87171"
                    font.pixelSize: 15
                    font.bold: true
                }

                Item { Layout.fillWidth: true }

                Text { text: "NET VALUATION:"; color: "#94A3B8"; font.pixelSize: 12; font.bold: true }
                Text {
                    text: "₹" + (root.selectedInwardVal - root.selectedOutwardVal).toLocaleString(Qt.locale(), "f", 2)
                    color: "#60A5FA"
                    font.pixelSize: 15
                    font.bold: true
                }
            }
        }
    }
}
