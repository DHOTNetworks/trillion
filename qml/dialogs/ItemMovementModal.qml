import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../components"

Rectangle {
    id: root
    width: parent ? Math.min(parent.width - 32, 1380) : 1380
    height: parent ? Math.min(parent.height - 40, 720) : 720
    color: "#FFFFFF"
    radius: 12
    border.color: "#CBD5E1"
    border.width: 1

    signal closeRequested()

    property string itemName: ""
    property int totalInwardBags: 0
    property real totalInwardQty: 0.0
    property real totalInwardVal: 0.0
    property int totalOutwardBags: 0
    property real totalOutwardQty: 0.0
    property real totalOutwardVal: 0.0
    
    property int selectedInwardBags: 0
    property real selectedInwardQty: 0.0
    property real selectedInwardVal: 0.0
    property int selectedOutwardBags: 0
    property real selectedOutwardQty: 0.0
    property real selectedOutwardVal: 0.0
    
    property int selectedInwardCount: 0
    property int selectedOutwardCount: 0

    function loadItemMovements(name) {
        itemName = name
        inwardModel.clear()
        outwardModel.clear()
        totalInwardBags = 0
        totalInwardQty = 0.0
        totalInwardVal = 0.0
        totalOutwardBags = 0
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
            var b = parseInt(m.bags) || 0
            var q = parseFloat(m.qty) || 0.0
            var r = parseFloat(m.rate) || 0.0
            var a = parseFloat(m.amount) || 0.0
            var p = m.party ? String(m.party).replace(/\u00a0/g, ' ') : ""

            if (m.isInward) {
                inwardModel.append({
                    isSelected: false,
                    vDate: m.vDate ? String(m.vDate) : "",
                    refNo: m.refNo ? String(m.refNo) : "",
                    party: p,
                    bags: b,
                    qty: q,
                    rate: r,
                    amount: a
                })
                totalInwardBags += b
                totalInwardQty += q
                totalInwardVal += a
            } else {
                outwardModel.append({
                    isSelected: false,
                    vDate: m.vDate ? String(m.vDate) : "",
                    refNo: m.refNo ? String(m.refNo) : "",
                    party: p,
                    bags: b,
                    qty: q,
                    rate: r,
                    amount: a
                })
                totalOutwardBags += b
                totalOutwardQty += q
                totalOutwardVal += a
            }
        }
        recalculateInwardTotals()
        recalculateOutwardTotals()

        if (inwardListView) {
            inwardListView.contentY = 0
            inwardListView.positionViewAtBeginning()
        }
        if (outwardListView) {
            outwardListView.contentY = 0
            outwardListView.positionViewAtBeginning()
        }
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
        var bTotal = 0
        var qty = 0.0
        var val = 0.0
        var hasSelections = false

        for (var i = 0; i < inwardModel.count; i++) {
            var row = inwardModel.get(i)
            if (row.isSelected) {
                hasSelections = true
                count++
                bTotal += (parseInt(row.bags) || 0)
                qty += (parseFloat(row.qty) || 0.0)
                val += (parseFloat(row.amount) || 0.0)
            }
        }
        selectedInwardCount = count

        if (hasSelections) {
            selectedInwardBags = bTotal
            selectedInwardQty = qty
            selectedInwardVal = val
        } else {
            selectedInwardBags = totalInwardBags
            selectedInwardQty = totalInwardQty
            selectedInwardVal = totalInwardVal
        }
    }

    function recalculateOutwardTotals() {
        var count = 0
        var bTotal = 0
        var qty = 0.0
        var val = 0.0
        var hasSelections = false

        for (var i = 0; i < outwardModel.count; i++) {
            var row = outwardModel.get(i)
            if (row.isSelected) {
                hasSelections = true
                count++
                bTotal += (parseInt(row.bags) || 0)
                qty += (parseFloat(row.qty) || 0.0)
                val += (parseFloat(row.amount) || 0.0)
            }
        }
        selectedOutwardCount = count

        if (hasSelections) {
            selectedOutwardBags = bTotal
            selectedOutwardQty = qty
            selectedOutwardVal = val
        } else {
            selectedOutwardBags = totalOutwardBags
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
                    RowLayout {
                        spacing: 8
                        Text { text: "ITEM MOVEMENT & TRANSACTION REGISTER"; color: "#2563EB"; font.pixelSize: 11; font.bold: true; font.letterSpacing: 1.0 }
                        Text {
                            text: (typeof stockItemsModel !== "undefined" && stockItemsModel && stockItemsModel.active_from_date) ? 
                                ("(From " + stockItemsModel.active_from_date + " To " + stockItemsModel.active_to_date + ")") : ""
                            color: "#64748B"
                            font.pixelSize: 11
                            font.bold: true
                        }
                    }
                    Text { text: root.itemName !== "" ? root.itemName : "Stock Item Details"; color: "#0F172A"; font.pixelSize: 18; font.bold: true }
                }
            }

            Item { Layout.fillWidth: true }

            Rectangle {
                id: closeBtn
                width: 32; height: 32; radius: 16
                color: closeBtnArea.containsMouse ? "#DC2626" : "#F1F5F9"
                Text { anchors.centerIn: parent; text: "✕"; color: closeBtnArea.containsMouse ? "#FFF" : "#475569"; font.bold: true }
                MouseArea {
                    id: closeBtnArea
                    anchors.fill: parent
                    hoverEnabled: true
                    onClicked: root.closeRequested()
                }
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
                            anchors.leftMargin: 8; anchors.rightMargin: 8
                            spacing: 6

                            // Master Inwards Checkbox
                            Rectangle {
                                width: 22; height: parent.height
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

                            Text { text: "Date"; color: "#475569"; font.pixelSize: 11; font.bold: true; Layout.preferredWidth: 80 }
                            Text { text: "Ref No"; color: "#475569"; font.pixelSize: 11; font.bold: true; Layout.preferredWidth: 125 }
                            Text { text: "Supplier / Farmer"; color: "#475569"; font.pixelSize: 11; font.bold: true; Layout.fillWidth: true; elide: Text.ElideRight }
                            Text { text: "Bags"; color: "#475569"; font.pixelSize: 11; font.bold: true; Layout.preferredWidth: 45; horizontalAlignment: Text.AlignRight }
                            Text { text: "Qtl"; color: "#475569"; font.pixelSize: 11; font.bold: true; Layout.preferredWidth: 65; horizontalAlignment: Text.AlignRight }
                            Text { text: "Amount ₹"; color: "#475569"; font.pixelSize: 11; font.bold: true; Layout.preferredWidth: 105; horizontalAlignment: Text.AlignRight }
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
                        boundsBehavior: Flickable.StopAtBounds

                        ScrollBar.vertical: ScrollBar {
                            policy: ScrollBar.AsNeeded
                            active: true
                        }

                        delegate: Rectangle {
                            width: inwardListView.width
                            height: 32
                            color: model.isSelected ? "#EFF6FF" : (index % 2 === 0 ? "#FFFFFF" : "#F8FAFC")
                            border.color: "#F1F5F9"

                            RowLayout {
                                anchors.fill: parent
                                anchors.leftMargin: 8; anchors.rightMargin: 8
                                spacing: 6

                                // Row Checkbox
                                Rectangle {
                                    width: 22; height: parent.height
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

                                Text { text: model.vDate ? String(model.vDate) : ""; color: "#334155"; font.pixelSize: 11; Layout.preferredWidth: 80 }
                                Text { text: model.refNo ? String(model.refNo) : ""; color: "#2563EB"; font.pixelSize: 11; font.bold: true; Layout.preferredWidth: 125; elide: Text.ElideRight }
                                Text { text: model.party ? String(model.party) : ""; color: "#0F172A"; font.pixelSize: 11; Layout.fillWidth: true; elide: Text.ElideRight }
                                Text { text: (parseInt(model.bags) || 0).toString(); color: "#334155"; font.pixelSize: 11; Layout.preferredWidth: 45; horizontalAlignment: Text.AlignRight }
                                Text { text: (parseFloat(model.qty) || 0.0).toFixed(2); color: "#16A34A"; font.pixelSize: 11; font.bold: true; Layout.preferredWidth: 65; horizontalAlignment: Text.AlignRight }
                                Text { text: (typeof dashboardCtrl !== "undefined" && dashboardCtrl) ? dashboardCtrl.format_inr(model.amount) : ("₹" + (parseFloat(model.amount) || 0.0).toFixed(2)); color: "#0F172A"; font.pixelSize: 11; font.bold: true; Layout.preferredWidth: 105; horizontalAlignment: Text.AlignRight }
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
                                text: root.selectedInwardCount > 0 ? "Selected Inwards (" + root.selectedInwardCount.toString() + "):" : "Total Inwards:"
                                color: "#166534"; font.pixelSize: 11; font.bold: true
                            }
                            Text { text: root.selectedInwardBags.toString() + " Bags (" + root.selectedInwardQty.toFixed(2) + " Qtl)"; color: "#15803D"; font.pixelSize: 12; font.bold: true }
                            Item { Layout.fillWidth: true }
                            Text { text: "Total: " + ((typeof dashboardCtrl !== "undefined" && dashboardCtrl) ? dashboardCtrl.format_inr(root.selectedInwardVal) : ("₹" + root.selectedInwardVal.toFixed(2))); color: "#166534"; font.pixelSize: 11; font.bold: true }
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
                            anchors.leftMargin: 8; anchors.rightMargin: 8
                            spacing: 6

                            // Master Outwards Checkbox
                            Rectangle {
                                width: 22; height: parent.height
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

                            Text { text: "Date"; color: "#475569"; font.pixelSize: 11; font.bold: true; Layout.preferredWidth: 80 }
                            Text { text: "Ref No"; color: "#475569"; font.pixelSize: 11; font.bold: true; Layout.preferredWidth: 125 }
                            Text { text: "Buyer / Customer"; color: "#475569"; font.pixelSize: 11; font.bold: true; Layout.fillWidth: true; elide: Text.ElideRight }
                            Text { text: "Bags"; color: "#475569"; font.pixelSize: 11; font.bold: true; Layout.preferredWidth: 45; horizontalAlignment: Text.AlignRight }
                            Text { text: "Qtl"; color: "#475569"; font.pixelSize: 11; font.bold: true; Layout.preferredWidth: 65; horizontalAlignment: Text.AlignRight }
                            Text { text: "Amount ₹"; color: "#475569"; font.pixelSize: 11; font.bold: true; Layout.preferredWidth: 105; horizontalAlignment: Text.AlignRight }
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
                        boundsBehavior: Flickable.StopAtBounds

                        ScrollBar.vertical: ScrollBar {
                            policy: ScrollBar.AsNeeded
                            active: true
                        }

                        delegate: Rectangle {
                            width: outwardListView.width
                            height: 32
                            color: model.isSelected ? "#FFF7ED" : (index % 2 === 0 ? "#FFFFFF" : "#F8FAFC")
                            border.color: "#F1F5F9"

                            RowLayout {
                                anchors.fill: parent
                                anchors.leftMargin: 8; anchors.rightMargin: 8
                                spacing: 6

                                // Row Checkbox
                                Rectangle {
                                    width: 22; height: parent.height
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

                                Text { text: model.vDate ? String(model.vDate) : ""; color: "#334155"; font.pixelSize: 11; Layout.preferredWidth: 80 }
                                Text { text: model.refNo ? String(model.refNo) : ""; color: "#EA580C"; font.pixelSize: 11; font.bold: true; Layout.preferredWidth: 125; elide: Text.ElideRight }
                                Text { text: model.party ? String(model.party) : ""; color: "#0F172A"; font.pixelSize: 11; Layout.fillWidth: true; elide: Text.ElideRight }
                                Text { text: (parseInt(model.bags) || 0).toString(); color: "#334155"; font.pixelSize: 11; Layout.preferredWidth: 45; horizontalAlignment: Text.AlignRight }
                                Text { text: (parseFloat(model.qty) || 0.0).toFixed(2); color: "#DC2626"; font.pixelSize: 11; font.bold: true; Layout.preferredWidth: 65; horizontalAlignment: Text.AlignRight }
                                Text { text: (typeof dashboardCtrl !== "undefined" && dashboardCtrl) ? dashboardCtrl.format_inr(model.amount) : ("₹" + (parseFloat(model.amount) || 0.0).toFixed(2)); color: "#0F172A"; font.pixelSize: 11; font.bold: true; Layout.preferredWidth: 105; horizontalAlignment: Text.AlignRight }
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
                                text: root.selectedOutwardCount > 0 ? "Selected Outwards (" + root.selectedOutwardCount.toString() + "):" : "Total Outwards:"
                                color: "#9A3412"; font.pixelSize: 11; font.bold: true
                            }
                            Text { text: root.selectedOutwardBags.toString() + " Bags (" + root.selectedOutwardQty.toFixed(2) + " Qtl)"; color: "#C2410C"; font.pixelSize: 12; font.bold: true }
                            Item { Layout.fillWidth: true }
                            Text { text: "Total: " + ((typeof dashboardCtrl !== "undefined" && dashboardCtrl) ? dashboardCtrl.format_inr(root.selectedOutwardVal) : ("₹" + root.selectedOutwardVal.toFixed(2))); color: "#9A3412"; font.pixelSize: 11; font.bold: true }
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
                
                RowLayout {
                    spacing: 6
                    Text {
                        text: (root.selectedInwardBags - root.selectedOutwardBags).toString() + " Bags"
                        color: (root.selectedInwardBags - root.selectedOutwardBags) >= 0 ? "#4ADE80" : "#F87171"
                        font.pixelSize: 14
                        font.bold: true
                    }
                    Text {
                        text: "(" + (root.selectedInwardQty - root.selectedOutwardQty).toFixed(2) + " Qtl)"
                        color: (root.selectedInwardQty - root.selectedOutwardQty) >= 0 ? "#4ADE80" : "#F87171"
                        font.pixelSize: 14
                        font.bold: true
                    }
                }

                Item { Layout.fillWidth: true }

                Text { text: "NET VALUATION:"; color: "#94A3B8"; font.pixelSize: 12; font.bold: true }
                Text {
                    text: (typeof dashboardCtrl !== "undefined" && dashboardCtrl) ? dashboardCtrl.format_inr(root.selectedInwardVal - root.selectedOutwardVal) : ("₹" + (root.selectedInwardVal - root.selectedOutwardVal).toFixed(2))
                    color: (root.selectedInwardVal - root.selectedOutwardVal) >= 0 ? "#60A5FA" : "#F87171"
                    font.pixelSize: 15
                    font.bold: true
                }
            }
        }
    }
}
