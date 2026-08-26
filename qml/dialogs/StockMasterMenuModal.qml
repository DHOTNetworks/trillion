import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../components"

Rectangle {
    id: root
    width: 520
    height: 340
    color: "#FFFFFF"
    border.color: "#E2E8F0"
    border.width: 1
    radius: 12

    signal closeRequested()
    signal actionSelected(string actionName)

    FocusScope {
        anchors.fill: parent
        focus: true
        Keys.onEscapePressed: root.closeRequested()
        Keys.onDigit1Pressed: root.actionSelected("New Stock Item")
        Keys.onDigit2Pressed: root.actionSelected("Modify Stock Item")
        Keys.onDigit3Pressed: root.actionSelected("Stock Details")
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 20
        spacing: 16

        // Header
        RowLayout {
            Layout.fillWidth: true
            RowLayout {
                spacing: 10
                Rectangle {
                    width: 36; height: 36; radius: 8; color: "#DCFCE7"
                    Text { anchors.centerIn: parent; text: "📦"; font.pixelSize: 18 }
                }
                ColumnLayout {
                    spacing: 0
                    Text { text: "STOCK MASTER MENU"; color: "#16A34A"; font.pixelSize: 11; font.bold: true; font.letterSpacing: 1.0 }
                    Text { text: "Select Stock Operation"; color: "#0F172A"; font.pixelSize: 16; font.bold: true }
                }
            }

            Item { Layout.fillWidth: true }

            Button {
                id: closeBtn
                width: 28
                height: 28
                background: Rectangle { color: closeBtn.hovered ? "#DC2626" : "#F1F5F9"; radius: 14 }
                contentItem: Text { text: "✕"; color: closeBtn.hovered ? "#FFF" : "#475569"; font.bold: true; horizontalAlignment: Text.AlignHCenter }
                onClicked: root.closeRequested()
            }
        }

        Rectangle { Layout.fillWidth: true; height: 1; color: "#E2E8F0" }

        // 3 Stock Master Options List
        ColumnLayout {
            Layout.fillWidth: true
            spacing: 8

            // 1. New Stock Item
            Rectangle {
                id: item1
                Layout.fillWidth: true
                height: 54
                radius: 8
                color: mouse1.containsMouse ? "#F0FDF4" : "#F8FAFC"
                border.color: mouse1.containsMouse ? "#16A34A" : "#E2E8F0"
                border.width: 1

                MouseArea {
                    id: mouse1
                    anchors.fill: parent
                    hoverEnabled: true
                    onClicked: root.actionSelected("New Stock Item")
                }

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 14; anchors.rightMargin: 14
                    spacing: 12
                    Text { text: "📦"; font.pixelSize: 18 }
                    ColumnLayout {
                        spacing: 1
                        Layout.fillWidth: true
                        Text { text: "1. New Stock Item"; color: "#0F172A"; font.pixelSize: 13; font.bold: true }
                        Text { text: "Add new Raw Paddy variety, Finished Rice grade or By-Product"; color: "#64748B"; font.pixelSize: 11 }
                    }
                    KbdBadge { text: "1"; badgeColor: "#F0FDF4"; textColor: "#16A34A"; borderColor: "#BBF7D0" }
                }
            }

            // 2. Modify Stock Item
            Rectangle {
                id: item2
                Layout.fillWidth: true
                height: 54
                radius: 8
                color: mouse2.containsMouse ? "#F0FDF4" : "#F8FAFC"
                border.color: mouse2.containsMouse ? "#16A34A" : "#E2E8F0"
                border.width: 1

                MouseArea {
                    id: mouse2
                    anchors.fill: parent
                    hoverEnabled: true
                    onClicked: root.actionSelected("Modify Stock Item")
                }

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 14; anchors.rightMargin: 14
                    spacing: 12
                    Text { text: "✏️"; font.pixelSize: 18 }
                    ColumnLayout {
                        spacing: 1
                        Layout.fillWidth: true
                        Text { text: "2. Modify Stock Item"; color: "#0F172A"; font.pixelSize: 13; font.bold: true }
                        Text { text: "Edit reorder level, unit of measure, or item description"; color: "#64748B"; font.pixelSize: 11 }
                    }
                    KbdBadge { text: "2"; badgeColor: "#F0FDF4"; textColor: "#16A34A"; borderColor: "#BBF7D0" }
                }
            }

            // 3. Stock Details
            Rectangle {
                id: item3
                Layout.fillWidth: true
                height: 54
                radius: 8
                color: mouse3.containsMouse ? "#F0FDF4" : "#F8FAFC"
                border.color: mouse3.containsMouse ? "#16A34A" : "#E2E8F0"
                border.width: 1

                MouseArea {
                    id: mouse3
                    anchors.fill: parent
                    hoverEnabled: true
                    onClicked: root.actionSelected("Stock Details")
                }

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 14; anchors.rightMargin: 14
                    spacing: 12
                    Text { text: "📊"; font.pixelSize: 18 }
                    ColumnLayout {
                        spacing: 1
                        Layout.fillWidth: true
                        Text { text: "3. Stock Details & Inventory Register"; color: "#0F172A"; font.pixelSize: 13; font.bold: true }
                        Text { text: "View live godown inventory, bag counts & stock balances"; color: "#64748B"; font.pixelSize: 11 }
                    }
                    KbdBadge { text: "3"; badgeColor: "#F0FDF4"; textColor: "#16A34A"; borderColor: "#BBF7D0" }
                }
            }
        }

        Item { Layout.fillHeight: true }
    }
}
