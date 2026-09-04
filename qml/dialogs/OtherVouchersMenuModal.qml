import QtQuick
import QtQuick.Templates as T
import QtQuick.Layouts
import MahadevERP

T.Popup {
    id: root
    width: 460
    implicitHeight: mainCol.implicitHeight + 36
    modal: true
    dim: true
    focus: true
    anchors.centerIn: parent
    closePolicy: T.Popup.CloseOnPressOutside | T.Popup.CloseOnEscape

    signal optionSelected(int optionIndex, int selectedIndex)

    property int selectedIndex: 0

    onOpened: {
        item1.resetMouseTracking()
        item2.resetMouseTracking()
        Qt.callLater(function() { menuScope.forceActiveFocus() })
    }

    function triggerSelected() {
        var opt = selectedIndex + 1
        var sel = selectedIndex
        root.close()
        root.optionSelected(opt, sel)
    }

    background: Rectangle {
        color: "#FFFFFF"
        border.color: "#7C3AED"
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
            else root.selectedIndex = 1
        }
        Keys.onDownPressed: function(event) {
            event.accepted = true
            if (root.selectedIndex < 1) root.selectedIndex++
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

        ColumnLayout {
            id: mainCol
            anchors.fill: parent
            anchors.margins: 14
            spacing: 10

            // Header Title
            RowLayout {
                Layout.fillWidth: true
                Text {
                    text: "OTHER VOUCHERS MENU"
                    color: "#7C3AED"
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

            // Item 1: J-Form Voucher (Mandi Purchase Stub)
            NavMenuItem {
                id: item1
                index: 0
                selectedIndex: root.selectedIndex
                activeColor: "#16A34A"
                activeBorderColor: "#15803D"
                onItemHovered: root.selectedIndex = 0
                onItemClicked: { root.selectedIndex = 0; root.triggerSelected() }

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 14; anchors.rightMargin: 14
                    spacing: 10

                    Text {
                        text: "1. J-Form Mandi Procurement Voucher (Form J)"
                        color: root.selectedIndex === 0 ? "#FFFFFF" : "#000000"
                        font.pixelSize: 13
                        font.bold: true
                        Layout.fillWidth: true
                    }

                    Rectangle {
                        height: 22
                        width: 45
                        radius: 4
                        color: root.selectedIndex === 0 ? "#14532D" : "#DCFCE7"
                        Text {
                            anchors.centerIn: parent
                            text: "F11"
                            color: root.selectedIndex === 0 ? "#86EFAC" : "#15803D"
                            font.pixelSize: 10
                            font.bold: true
                        }
                    }
                }
            }

            // Item 2: TDS Voucher (Tax Deducted at Source)
            NavMenuItem {
                id: item2
                index: 1
                selectedIndex: root.selectedIndex
                activeColor: "#7C3AED"
                activeBorderColor: "#6D28D9"
                onItemHovered: root.selectedIndex = 1
                onItemClicked: { root.selectedIndex = 1; root.triggerSelected() }

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 14; anchors.rightMargin: 14
                    spacing: 10

                    Text {
                        text: "2. TDS Voucher (Tax Deducted at Source)"
                        color: root.selectedIndex === 1 ? "#FFFFFF" : "#000000"
                        font.pixelSize: 13
                        font.bold: true
                        Layout.fillWidth: true
                    }

                    Rectangle {
                        height: 22
                        width: 45
                        radius: 4
                        color: root.selectedIndex === 1 ? "#4C1D95" : "#EDE9FE"
                        Text {
                            anchors.centerIn: parent
                            text: "F12"
                            color: root.selectedIndex === 1 ? "#DDD6FE" : "#7C3AED"
                            font.pixelSize: 10
                            font.bold: true
                        }
                    }
                }
            }
        }
    }
}
