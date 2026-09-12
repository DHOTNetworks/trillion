import QtQuick
import QtQuick.Templates as T
import QtQuick.Layouts
import MahadevERP
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
    signal openInvoiceRequested(string invNo, string invType)

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

    // Focus state management
    Keys.onEscapePressed: function(event) {
        event.accepted = true
        root.closeRequested()
    }

    onVisibleChanged: {
        if (visible) {
            Qt.callLater(function() {
                if (inwardListView && inwardModel.count > 0) {
                    inwardListView.currentIndex = 0
                    inwardListView.forceActiveFocus()
                } else if (outwardListView && outwardModel.count > 0) {
                    outwardListView.currentIndex = 0
                    outwardListView.forceActiveFocus()
                }
            })
        }
    }

    function loadItemMovements(name, fDate, tDate) {
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

        var movs = (typeof stockItemsModel !== "undefined" && stockItemsModel) ? stockItemsModel.get_item_movements(name, fDate || "", tDate || "") : []
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

        Qt.callLater(function() {
            if (inwardListView) {
                inwardListView.contentY = 0
                inwardListView.positionViewAtBeginning()
                if (inwardModel.count > 0) {
                    inwardListView.currentIndex = 0
                    inwardListView.forceActiveFocus()
                } else {
                    inwardListView.currentIndex = -1
                }
            }
            if (outwardListView) {
                outwardListView.contentY = 0
                outwardListView.positionViewAtBeginning()
                if (outwardModel.count > 0) {
                    outwardListView.currentIndex = 0
                    if (inwardModel.count === 0) {
                        outwardListView.forceActiveFocus()
                    }
                } else {
                    outwardListView.currentIndex = -1
                }
            }
        })
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

    GenericListModel { id: inwardModel }
    GenericListModel { id: outwardModel }

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

            // Keyboard hints
            RowLayout {
                spacing: 6
                Layout.alignment: Qt.AlignVCenter
                KbdBadge { text: "←/→ Switch Table"; badgeColor: "#F1F5F9"; textColor: "#475569"; borderColor: "#CBD5E1" }
                KbdBadge { text: "↑/↓ Move"; badgeColor: "#F1F5F9"; textColor: "#475569"; borderColor: "#CBD5E1" }
                KbdBadge { text: "Space: Select"; badgeColor: "#EFF6FF"; textColor: "#1D4ED8"; borderColor: "#BFDBFE" }
                KbdBadge { text: "Enter: Open Voucher"; badgeColor: "#F0FDF4"; textColor: "#15803D"; borderColor: "#BBF7D0" }
                KbdBadge { text: "Esc: Close"; badgeColor: "#FEF2F2"; textColor: "#B91C1C"; borderColor: "#FECACA" }
            }

            Rectangle {
                id: closeBtn
                width: 32; height: 32; radius: 16
                color: closeBtnArea.containsMouse ? "#DC2626" : "#F1F5F9"
                Text { anchors.centerIn: parent; text: "✕"; color: closeBtnArea.containsMouse ? "#FFF" : "#475569"; font.bold: true; font.pixelSize: 13 }
                MouseArea {
                    id: closeBtnArea
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
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
                id: inwardContainer
                Layout.fillWidth: true
                Layout.fillHeight: true
                color: "#FFFFFF"
                border.color: inwardListView.activeFocus ? "#2563EB" : "#BFDBFE"
                border.width: inwardListView.activeFocus ? 2 : 1.5
                radius: 10
                clip: true

                ColumnLayout {
                    anchors.fill: parent
                    spacing: 0

                    // Section Title Header
                    Rectangle {
                        Layout.fillWidth: true
                        height: 36
                        color: inwardListView.activeFocus ? "#DBEAFE" : "#EFF6FF"
                        border.color: "#BFDBFE"
                        border.width: 1

                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: 12; anchors.rightMargin: 12
                            Text { text: "INWARDS / ARRIVALS & PURCHASES"; color: "#1D4ED8"; font.pixelSize: 12; font.bold: true }
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
                            CustomCheckBox {
                                Layout.preferredWidth: 22
                                Layout.alignment: Qt.AlignVCenter
                                boxSize: 16
                                boxRadius: 3
                                checkedColor: "#2563EB"
                                checked: root.selectedInwardCount === inwardModel.count && inwardModel.count > 0
                                onToggled: {
                                    var newState = !(root.selectedInwardCount === inwardModel.count && inwardModel.count > 0)
                                    root.toggleSelectAllInwards(newState)
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
                        focus: true
                        activeFocusOnTab: true
                        currentIndex: 0
                        highlightFollowsCurrentItem: true

                        Keys.onUpPressed: function(event) {
                            event.accepted = true
                            if (inwardListView.currentIndex > 0) {
                                inwardListView.currentIndex--
                            } else if (inwardListView.currentIndex === -1 && count > 0) {
                                inwardListView.currentIndex = 0
                            }
                            inwardListView.positionViewAtIndex(inwardListView.currentIndex, ListView.Contain)
                        }

                        Keys.onDownPressed: function(event) {
                            event.accepted = true
                            if (inwardListView.currentIndex < 0 && count > 0) {
                                inwardListView.currentIndex = 0
                            } else if (inwardListView.currentIndex < count - 1) {
                                inwardListView.currentIndex++
                            }
                            inwardListView.positionViewAtIndex(inwardListView.currentIndex, ListView.Contain)
                        }

                        Keys.onSpacePressed: function(event) {
                            event.accepted = true
                            if (inwardListView.currentIndex >= 0 && inwardListView.currentIndex < inwardModel.count) {
                                var cur = inwardModel.get(inwardListView.currentIndex)
                                inwardModel.setProperty(inwardListView.currentIndex, "isSelected", !cur.isSelected)
                                root.recalculateInwardTotals()
                            }
                        }

                        Keys.onRightPressed: function(event) {
                            event.accepted = true
                            outwardListView.forceActiveFocus()
                            if (outwardModel.count > 0) {
                                if (outwardListView.currentIndex < 0 || outwardListView.currentIndex >= outwardModel.count) {
                                    outwardListView.currentIndex = 0
                                }
                            }
                        }

                        Keys.onReturnPressed: function(event) {
                            event.accepted = true
                            if (inwardListView.currentIndex >= 0 && inwardListView.currentIndex < inwardModel.count) {
                                var r = inwardModel.get(inwardListView.currentIndex)
                                if (r && r.refNo) root.openInvoiceRequested(r.refNo, "Purchase")
                            }
                        }

                        Keys.onEnterPressed: function(event) {
                            event.accepted = true
                            if (inwardListView.currentIndex >= 0 && inwardListView.currentIndex < inwardModel.count) {
                                var r = inwardModel.get(inwardListView.currentIndex)
                                if (r && r.refNo) root.openInvoiceRequested(r.refNo, "Purchase")
                            }
                        }

                        Keys.onEscapePressed: function(event) {
                            event.accepted = true
                            root.closeRequested()
                        }

                        delegate: Rectangle {
                            id: inwardRowRect
                            width: inwardListView.width
                            height: 32
                            property bool isCurrent: inwardListView.activeFocus && inwardListView.currentIndex === index
                            color: isCurrent ? (model.isSelected ? "#DBEAFE" : "#EFF6FF") : (model.isSelected ? "#EFF6FF" : (index % 2 === 0 ? "#FFFFFF" : "#F8FAFC"))
                            border.color: isCurrent ? "#2563EB" : (model.isSelected ? "#93C5FD" : "#F1F5F9")
                            border.width: isCurrent ? 2 : 1
                            radius: 3

                            RowLayout {
                                anchors.fill: parent
                                anchors.leftMargin: 8; anchors.rightMargin: 8
                                spacing: 6

                                // Row Checkbox
                                CustomCheckBox {
                                    Layout.preferredWidth: 22
                                    Layout.alignment: Qt.AlignVCenter
                                    boxSize: 16
                                    boxRadius: 3
                                    checkedColor: "#2563EB"
                                    checked: Boolean(model.isSelected)
                                    onToggled: {
                                        inwardModel.setProperty(index, "isSelected", !model.isSelected)
                                        root.recalculateInwardTotals()
                                    }
                                }

                                Text { text: model.vDate ? String(model.vDate) : ""; color: "#334155"; font.pixelSize: 11; Layout.preferredWidth: 80 }
                                Text { text: model.refNo ? String(model.refNo) : ""; color: "#2563EB"; font.pixelSize: 11; font.bold: true; Layout.preferredWidth: 125; elide: Text.ElideRight }
                                Text { text: model.party ? String(model.party) : ""; color: "#0F172A"; font.pixelSize: 11; Layout.fillWidth: true; elide: Text.ElideRight }
                                Text { text: (parseInt(model.bags) || 0).toString(); color: "#334155"; font.pixelSize: 11; Layout.preferredWidth: 45; horizontalAlignment: Text.AlignRight }
                                Text { text: (parseFloat(model.qty) || 0.0).toFixed(2); color: "#16A34A"; font.pixelSize: 11; font.bold: true; Layout.preferredWidth: 65; horizontalAlignment: Text.AlignRight }
                                Text { text: (typeof dashboardCtrl !== "undefined" && dashboardCtrl) ? dashboardCtrl.format_inr(model.amount) : ("₹" + (parseFloat(model.amount) || 0.0).toFixed(2)); color: "#0F172A"; font.pixelSize: 11; font.bold: true; Layout.preferredWidth: 105; horizontalAlignment: Text.AlignRight }
                            }

                            MouseArea {
                                anchors.fill: parent
                                anchors.leftMargin: 35
                                hoverEnabled: true
                                cursorShape: Qt.PointingHandCursor
                                onClicked: {
                                    inwardListView.currentIndex = index
                                    inwardListView.forceActiveFocus()
                                }
                                onDoubleClicked: {
                                    inwardListView.currentIndex = index
                                    inwardListView.forceActiveFocus()
                                    if (model.refNo) {
                                        root.openInvoiceRequested(model.refNo, "Purchase")
                                    }
                                }
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
                id: outwardContainer
                Layout.fillWidth: true
                Layout.fillHeight: true
                color: "#FFFFFF"
                border.color: outwardListView.activeFocus ? "#EA580C" : "#FED7AA"
                border.width: outwardListView.activeFocus ? 2 : 1.5
                radius: 10
                clip: true

                ColumnLayout {
                    anchors.fill: parent
                    spacing: 0

                    // Section Title Header
                    Rectangle {
                        Layout.fillWidth: true
                        height: 36
                        color: outwardListView.activeFocus ? "#FFEDD5" : "#FFF7ED"
                        border.color: "#FED7AA"
                        border.width: 1

                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: 12; anchors.rightMargin: 12
                            Text { text: "OUTWARDS / SALES & DISPATCHES"; color: "#C2410C"; font.pixelSize: 12; font.bold: true }
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
                            CustomCheckBox {
                                Layout.preferredWidth: 22
                                Layout.alignment: Qt.AlignVCenter
                                boxSize: 16
                                boxRadius: 3
                                checkedColor: "#EA580C"
                                checked: root.selectedOutwardCount === outwardModel.count && outwardModel.count > 0
                                onToggled: {
                                    var newState = !(root.selectedOutwardCount === outwardModel.count && outwardModel.count > 0)
                                    root.toggleSelectAllOutwards(newState)
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
                        focus: false
                        activeFocusOnTab: true
                        currentIndex: 0
                        highlightFollowsCurrentItem: true

                        Keys.onUpPressed: function(event) {
                            event.accepted = true
                            if (outwardListView.currentIndex > 0) {
                                outwardListView.currentIndex--
                            } else if (outwardListView.currentIndex === -1 && count > 0) {
                                outwardListView.currentIndex = 0
                            }
                            outwardListView.positionViewAtIndex(outwardListView.currentIndex, ListView.Contain)
                        }

                        Keys.onDownPressed: function(event) {
                            event.accepted = true
                            if (outwardListView.currentIndex < 0 && count > 0) {
                                outwardListView.currentIndex = 0
                            } else if (outwardListView.currentIndex < count - 1) {
                                outwardListView.currentIndex++
                            }
                            outwardListView.positionViewAtIndex(outwardListView.currentIndex, ListView.Contain)
                        }

                        Keys.onSpacePressed: function(event) {
                            event.accepted = true
                            if (outwardListView.currentIndex >= 0 && outwardListView.currentIndex < outwardModel.count) {
                                var cur = outwardModel.get(outwardListView.currentIndex)
                                outwardModel.setProperty(outwardListView.currentIndex, "isSelected", !cur.isSelected)
                                root.recalculateOutwardTotals()
                            }
                        }

                        Keys.onLeftPressed: function(event) {
                            event.accepted = true
                            inwardListView.forceActiveFocus()
                            if (inwardModel.count > 0) {
                                if (inwardListView.currentIndex < 0 || inwardListView.currentIndex >= inwardModel.count) {
                                    inwardListView.currentIndex = 0
                                }
                            }
                        }

                        Keys.onReturnPressed: function(event) {
                            event.accepted = true
                            if (outwardListView.currentIndex >= 0 && outwardListView.currentIndex < outwardModel.count) {
                                var r = outwardModel.get(outwardListView.currentIndex)
                                if (r && r.refNo) root.openInvoiceRequested(r.refNo, "Sale")
                            }
                        }

                        Keys.onEnterPressed: function(event) {
                            event.accepted = true
                            if (outwardListView.currentIndex >= 0 && outwardListView.currentIndex < outwardModel.count) {
                                var r = outwardModel.get(outwardListView.currentIndex)
                                if (r && r.refNo) root.openInvoiceRequested(r.refNo, "Sale")
                            }
                        }

                        Keys.onEscapePressed: function(event) {
                            event.accepted = true
                            root.closeRequested()
                        }

                        delegate: Rectangle {
                            id: outwardRowRect
                            width: outwardListView.width
                            height: 32
                            property bool isCurrent: outwardListView.activeFocus && outwardListView.currentIndex === index
                            color: isCurrent ? (model.isSelected ? "#FED7AA" : "#FFF7ED") : (model.isSelected ? "#FFF7ED" : (index % 2 === 0 ? "#FFFFFF" : "#F8FAFC"))
                            border.color: isCurrent ? "#EA580C" : (model.isSelected ? "#FDBA74" : "#F1F5F9")
                            border.width: isCurrent ? 2 : 1
                            radius: 3

                            RowLayout {
                                anchors.fill: parent
                                anchors.leftMargin: 8; anchors.rightMargin: 8
                                spacing: 6

                                // Row Checkbox
                                CustomCheckBox {
                                    Layout.preferredWidth: 22
                                    Layout.alignment: Qt.AlignVCenter
                                    boxSize: 16
                                    boxRadius: 3
                                    checkedColor: "#EA580C"
                                    checked: Boolean(model.isSelected)
                                    onToggled: {
                                        outwardModel.setProperty(index, "isSelected", !model.isSelected)
                                        root.recalculateOutwardTotals()
                                    }
                                }

                                Text { text: model.vDate ? String(model.vDate) : ""; color: "#334155"; font.pixelSize: 11; Layout.preferredWidth: 80 }
                                Text { text: model.refNo ? String(model.refNo) : ""; color: "#EA580C"; font.pixelSize: 11; font.bold: true; Layout.preferredWidth: 125; elide: Text.ElideRight }
                                Text { text: model.party ? String(model.party) : ""; color: "#0F172A"; font.pixelSize: 11; Layout.fillWidth: true; elide: Text.ElideRight }
                                Text { text: (parseInt(model.bags) || 0).toString(); color: "#334155"; font.pixelSize: 11; Layout.preferredWidth: 45; horizontalAlignment: Text.AlignRight }
                                Text { text: (parseFloat(model.qty) || 0.0).toFixed(2); color: "#DC2626"; font.pixelSize: 11; font.bold: true; Layout.preferredWidth: 65; horizontalAlignment: Text.AlignRight }
                                Text { text: (typeof dashboardCtrl !== "undefined" && dashboardCtrl) ? dashboardCtrl.format_inr(model.amount) : ("₹" + (parseFloat(model.amount) || 0.0).toFixed(2)); color: "#0F172A"; font.pixelSize: 11; font.bold: true; Layout.preferredWidth: 105; horizontalAlignment: Text.AlignRight }
                            }

                            MouseArea {
                                anchors.fill: parent
                                anchors.leftMargin: 35
                                hoverEnabled: true
                                cursorShape: Qt.PointingHandCursor
                                onClicked: {
                                    outwardListView.currentIndex = index
                                    outwardListView.forceActiveFocus()
                                }
                                onDoubleClicked: {
                                    outwardListView.currentIndex = index
                                    outwardListView.forceActiveFocus()
                                    if (model.refNo) {
                                        root.openInvoiceRequested(model.refNo, "Sale")
                                    }
                                }
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
