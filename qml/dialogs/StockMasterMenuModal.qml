import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../components"

Popup {
    id: root
    width: 420
    implicitHeight: mainCol.implicitHeight + 36
    modal: true
    dim: true
    focus: true
    closePolicy: Popup.CloseOnPressOutside | Popup.CloseOnEscape

    signal actionSelected(string actionName, int selectedIndex)

    property int selectedIndex: 0

    onOpened: {
        item1.resetMouseTracking()
        item2.resetMouseTracking()
        item3.resetMouseTracking()
        Qt.callLater(function() { menuScope.forceActiveFocus() })
    }

    function triggerSelected() {
        var act = ""
        if (selectedIndex === 0) act = "New Stock Item"
        else if (selectedIndex === 1) act = "Modify Stock Item"
        else if (selectedIndex === 2) act = "Stock Details"
        var sel = selectedIndex
        root.close()
        root.actionSelected(act, sel)
    }

    background: Rectangle {
        color: "#FFFFFF"
        border.color: "#16A34A"
        border.width: 2.5
        radius: 12
    }

    contentItem: FocusScope {
        id: menuScope
        anchors.fill: parent
        focus: true

        Keys.onUpPressed: function(event) {
            event.accepted = true
            if (root.selectedIndex > 0) root.selectedIndex--
            else root.selectedIndex = 2
        }
        Keys.onDownPressed: function(event) {
            event.accepted = true
            if (root.selectedIndex < 2) root.selectedIndex++
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

        ColumnLayout {
            id: mainCol
            anchors.fill: parent
            anchors.margins: 14
            spacing: 10

            // Header Title
            RowLayout {
                Layout.fillWidth: true
                Text {
                    text: "STOCK MASTER MENU"
                    color: "#16A34A"
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

            // Item 1
            NavMenuItem {
                id: item1
                index: 0
                selectedIndex: root.selectedIndex
                text: "1. New Stock Item"
                activeColor: "#16A34A"
                activeBorderColor: "#15803D"
                onItemHovered: root.selectedIndex = 0
                onItemClicked: { root.selectedIndex = 0; root.triggerSelected() }
            }

            // Item 2
            NavMenuItem {
                id: item2
                index: 1
                selectedIndex: root.selectedIndex
                text: "2. Modify Stock Item"
                activeColor: "#16A34A"
                activeBorderColor: "#15803D"
                onItemHovered: root.selectedIndex = 1
                onItemClicked: { root.selectedIndex = 1; root.triggerSelected() }
            }

            // Item 3
            NavMenuItem {
                id: item3
                index: 2
                selectedIndex: root.selectedIndex
                text: "3. Stock Details & Register"
                activeColor: "#16A34A"
                activeBorderColor: "#15803D"
                onItemHovered: root.selectedIndex = 2
                onItemClicked: { root.selectedIndex = 2; root.triggerSelected() }
            }
        }
    }
}
