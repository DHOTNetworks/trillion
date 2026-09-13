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
        item3.resetMouseTracking()
        item4.resetMouseTracking()
        item5.resetMouseTracking()
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
            else root.selectedIndex = 4
        }
        Keys.onDownPressed: function(event) {
            event.accepted = true
            if (root.selectedIndex < 4) root.selectedIndex++
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

            // Item 3: Bank Statement Auto-Entry & Reconciliation
            NavMenuItem {
                id: item3
                index: 2
                selectedIndex: root.selectedIndex
                activeColor: "#1D4ED8"
                activeBorderColor: "#1E40AF"
                onItemHovered: root.selectedIndex = 2
                onItemClicked: { root.selectedIndex = 2; root.triggerSelected() }

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 14; anchors.rightMargin: 14
                    spacing: 10

                    Text {
                        text: "3. Canara Bank Statement Auto-Entry (PDF)"
                        color: root.selectedIndex === 2 ? "#FFFFFF" : "#000000"
                        font.pixelSize: 13
                        font.bold: true
                        Layout.fillWidth: true
                    }

                    Rectangle {
                        height: 22
                        width: 45
                        radius: 4
                        color: root.selectedIndex === 2 ? "#1E3A8A" : "#DBEAFE"
                        Text {
                            anchors.centerIn: parent
                            text: "Ctrl+B"
                            color: root.selectedIndex === 2 ? "#BFDBFE" : "#1D4ED8"
                            font.pixelSize: 10
                            font.bold: true
                        }
                    }
                }
            }

            // Item 4: Transport, Weighbridge (Kanda) & e-Way Register
            NavMenuItem {
                id: item4
                index: 3
                selectedIndex: root.selectedIndex
                activeColor: "#059669"
                activeBorderColor: "#047857"
                onItemHovered: root.selectedIndex = 3
                onItemClicked: { root.selectedIndex = 3; root.triggerSelected() }

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 14; anchors.rightMargin: 14
                    spacing: 10

                    Text {
                        text: "4. Transport, Weighbridge (Kanda) & e-Way Register"
                        color: root.selectedIndex === 3 ? "#FFFFFF" : "#000000"
                        font.pixelSize: 13
                        font.bold: true
                        Layout.fillWidth: true
                    }

                    Rectangle {
                        height: 22
                        width: 45
                        radius: 4
                        color: root.selectedIndex === 3 ? "#064E3B" : "#D1FAE5"
                        Text {
                            anchors.centerIn: parent
                            text: "Alt+T"
                            color: root.selectedIndex === 3 ? "#A7F3D0" : "#059669"
                            font.pixelSize: 10
                            font.bold: true
                        }
                    }
                }
            }

            // Item 5: GST Debit Notes & Credit Notes
            NavMenuItem {
                id: item5
                index: 4
                selectedIndex: root.selectedIndex
                activeColor: "#DC2626"
                activeBorderColor: "#B91C1C"
                onItemHovered: root.selectedIndex = 4
                onItemClicked: { root.selectedIndex = 4; root.triggerSelected() }

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 14; anchors.rightMargin: 14
                    spacing: 10

                    Text {
                        text: "5. GST Debit Notes & Credit Notes (DebitCreditNotes)"
                        color: root.selectedIndex === 4 ? "#FFFFFF" : "#000000"
                        font.pixelSize: 13
                        font.bold: true
                        Layout.fillWidth: true
                    }

                    Rectangle {
                        height: 22
                        width: 45
                        radius: 4
                        color: root.selectedIndex === 4 ? "#7F1D1D" : "#FEE2E2"
                        Text {
                            anchors.centerIn: parent
                            text: "Alt+D"
                            color: root.selectedIndex === 4 ? "#FECACA" : "#DC2626"
                            font.pixelSize: 10
                            font.bold: true
                        }
                    }
                }
            }
        }
    }
}
