import QtQuick
import QtQuick.Templates as T
import QtQuick.Layouts
import MahadevERP

T.Popup {
    id: root
    width: 440
    implicitHeight: mainCol.implicitHeight + 36
    modal: true
    dim: true
    focus: true
    anchors.centerIn: T.Overlay.overlay
    closePolicy: T.Popup.CloseOnPressOutside | T.Popup.CloseOnEscape

    signal actionSelected(string actionName, int selectedIndex)

    property int selectedIndex: 0

    onOpened: {
        item1.resetMouseTracking()
        item2.resetMouseTracking()
        item3.resetMouseTracking()
        item4.resetMouseTracking()
        item5.resetMouseTracking()
        item6.resetMouseTracking()
        Qt.callLater(function() { menuScope.forceActiveFocus() })
    }

    function triggerSelected() {
        var act = ""
        if (selectedIndex === 0) act = "Milling Statement"
        else if (selectedIndex === 1) act = "Stock Register"
        else if (selectedIndex === 2) act = "Item Movement"
        else if (selectedIndex === 3) act = "Ledger Statement"
        else if (selectedIndex === 4) act = "Sales Register"
        else if (selectedIndex === 5) act = "Purchase Register"
        var sel = selectedIndex
        root.close()
        root.actionSelected(act, sel)
    }

    background: Rectangle {
        color: "#FFFFFF"
        border.color: "#059669"
        border.width: 2.5
        radius: 12
    }

    FocusScope {
        id: menuScope
        anchors.fill: parent
        focus: true

        Keys.onUpPressed: function(event) {
            event.accepted = true
            if (root.selectedIndex > 0) root.selectedIndex--
            else root.selectedIndex = 5
        }
        Keys.onDownPressed: function(event) {
            event.accepted = true
            if (root.selectedIndex < 5) root.selectedIndex++
            else root.selectedIndex = 0
        }
        Keys.onReturnPressed: function(event) {
            event.accepted = true
            root.triggerSelected()
        }
        Keys.onEnterPressed: function(event) {
            event.accepted = true
            root.triggerSelected()
        }
        Keys.onEscapePressed: function(event) {
            event.accepted = true
            root.close()
        }
        Keys.onDigit1Pressed: function(event) { event.accepted = true; root.selectedIndex = 0; root.triggerSelected() }
        Keys.onDigit2Pressed: function(event) { event.accepted = true; root.selectedIndex = 1; root.triggerSelected() }
        Keys.onDigit3Pressed: function(event) { event.accepted = true; root.selectedIndex = 2; root.triggerSelected() }
        Keys.onDigit4Pressed: function(event) { event.accepted = true; root.selectedIndex = 3; root.triggerSelected() }
        Keys.onDigit5Pressed: function(event) { event.accepted = true; root.selectedIndex = 4; root.triggerSelected() }
        Keys.onDigit6Pressed: function(event) { event.accepted = true; root.selectedIndex = 5; root.triggerSelected() }

        ColumnLayout {
            id: mainCol
            anchors.fill: parent
            anchors.margins: 14
            spacing: 10

            // Header Title
            RowLayout {
                Layout.fillWidth: true
                Text {
                    text: "📊 FINANCIAL & MILLING REPORTS MENU"
                    color: "#059669"
                    font.pixelSize: 13
                    font.bold: true
                    font.letterSpacing: 1.0
                }
                Item { Layout.fillWidth: true }
                Text {
                    text: "Press ↑ / ↓ & Enter"
                    color: "#64748B"
                    font.pixelSize: 11
                    font.bold: true
                }
            }

            Rectangle { Layout.fillWidth: true; height: 1; color: "#CBD5E1" }

            // Item 1: Milling Statement
            NavMenuItem {
                id: item1
                index: 0
                selectedIndex: root.selectedIndex
                text: "1. Milling Statement & Yield Register"
                activeColor: "#059669"
                activeBorderColor: "#047857"
                onItemHovered: root.selectedIndex = 0
                onItemClicked: { root.selectedIndex = 0; root.triggerSelected() }
            }

            // Item 2: Stock Register
            NavMenuItem {
                id: item2
                index: 1
                selectedIndex: root.selectedIndex
                text: "2. Stock Summary Register"
                activeColor: "#059669"
                activeBorderColor: "#047857"
                onItemHovered: root.selectedIndex = 1
                onItemClicked: { root.selectedIndex = 1; root.triggerSelected() }
            }

            // Item 3: Item Movement
            NavMenuItem {
                id: item3
                index: 2
                selectedIndex: root.selectedIndex
                text: "3. Item Movement Analysis"
                activeColor: "#059669"
                activeBorderColor: "#047857"
                onItemHovered: root.selectedIndex = 2
                onItemClicked: { root.selectedIndex = 2; root.triggerSelected() }
            }

            // Item 4: Ledger Statement
            NavMenuItem {
                id: item4
                index: 3
                selectedIndex: root.selectedIndex
                text: "4. Party & Ledger Statements"
                activeColor: "#059669"
                activeBorderColor: "#047857"
                onItemHovered: root.selectedIndex = 3
                onItemClicked: { root.selectedIndex = 3; root.triggerSelected() }
            }

            // Item 5: Sales Register
            NavMenuItem {
                id: item5
                index: 4
                selectedIndex: root.selectedIndex
                text: "5. Sales Invoices Register"
                activeColor: "#059669"
                activeBorderColor: "#047857"
                onItemHovered: root.selectedIndex = 4
                onItemClicked: { root.selectedIndex = 4; root.triggerSelected() }
            }

            // Item 6: Purchase Register
            NavMenuItem {
                id: item6
                index: 5
                selectedIndex: root.selectedIndex
                text: "6. Purchase Invoices Register"
                activeColor: "#059669"
                activeBorderColor: "#047857"
                onItemHovered: root.selectedIndex = 5
                onItemClicked: { root.selectedIndex = 5; root.triggerSelected() }
            }
        }
    }
}
