import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../components"

Rectangle {
    id: root
    width: 520
    height: 440
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
        Keys.onDigit1Pressed: root.actionSelected("New Ledger")
        Keys.onDigit2Pressed: root.actionSelected("Modify Ledger")
        Keys.onDigit3Pressed: root.actionSelected("View Ledger")
        Keys.onDigit4Pressed: root.actionSelected("New Group")
        Keys.onDigit5Pressed: root.actionSelected("Modify Group")
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
                    width: 36; height: 36; radius: 8; color: "#DBEAFE"
                    Text { anchors.centerIn: parent; text: "📖"; font.pixelSize: 18 }
                }
                ColumnLayout {
                    spacing: 0
                    Text { text: "LEDGER MASTER MENU"; color: "#2563EB"; font.pixelSize: 11; font.bold: true; font.letterSpacing: 1.0 }
                    Text { text: "Select Ledger Operation"; color: "#0F172A"; font.pixelSize: 16; font.bold: true }
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

        // 5 Ledger Master Options List
        ColumnLayout {
            Layout.fillWidth: true
            spacing: 8

            // 1. New Ledger
            Rectangle {
                id: item1
                Layout.fillWidth: true
                height: 52
                radius: 8
                color: mouse1.containsMouse ? "#EFF6FF" : "#F8FAFC"
                border.color: mouse1.containsMouse ? "#2563EB" : "#E2E8F0"
                border.width: 1

                MouseArea {
                    id: mouse1
                    anchors.fill: parent
                    hoverEnabled: true
                    onClicked: root.actionSelected("New Ledger")
                }

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 14; anchors.rightMargin: 14
                    spacing: 12
                    Text { text: "➕"; font.pixelSize: 16 }
                    ColumnLayout {
                        spacing: 1
                        Layout.fillWidth: true
                        Text { text: "1. New Ledger Account"; color: "#0F172A"; font.pixelSize: 13; font.bold: true }
                        Text { text: "Create new Farmer, Buyer, Merchant or Expense party"; color: "#64748B"; font.pixelSize: 11 }
                    }
                    KbdBadge { text: "1"; badgeColor: "#EFF6FF"; textColor: "#2563EB"; borderColor: "#BFDBFE" }
                }
            }

            // 2. Modify Ledger
            Rectangle {
                id: item2
                Layout.fillWidth: true
                height: 52
                radius: 8
                color: mouse2.containsMouse ? "#EFF6FF" : "#F8FAFC"
                border.color: mouse2.containsMouse ? "#2563EB" : "#E2E8F0"
                border.width: 1

                MouseArea {
                    id: mouse2
                    anchors.fill: parent
                    hoverEnabled: true
                    onClicked: root.actionSelected("Modify Ledger")
                }

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 14; anchors.rightMargin: 14
                    spacing: 12
                    Text { text: "✏️"; font.pixelSize: 16 }
                    ColumnLayout {
                        spacing: 1
                        Layout.fillWidth: true
                        Text { text: "2. Modify Ledger Account"; color: "#0F172A"; font.pixelSize: 13; font.bold: true }
                        Text { text: "Edit party contact, address, GSTIN or opening balance"; color: "#64748B"; font.pixelSize: 11 }
                    }
                    KbdBadge { text: "2"; badgeColor: "#EFF6FF"; textColor: "#2563EB"; borderColor: "#BFDBFE" }
                }
            }

            // 3. View Ledger
            Rectangle {
                id: item3
                Layout.fillWidth: true
                height: 52
                radius: 8
                color: mouse3.containsMouse ? "#EFF6FF" : "#F8FAFC"
                border.color: mouse3.containsMouse ? "#2563EB" : "#E2E8F0"
                border.width: 1

                MouseArea {
                    id: mouse3
                    anchors.fill: parent
                    hoverEnabled: true
                    onClicked: root.actionSelected("View Ledger")
                }

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 14; anchors.rightMargin: 14
                    spacing: 12
                    Text { text: "👁️"; font.pixelSize: 16 }
                    ColumnLayout {
                        spacing: 1
                        Layout.fillWidth: true
                        Text { text: "3. View Ledger Statement"; color: "#0F172A"; font.pixelSize: 13; font.bold: true }
                        Text { text: "Open party balance statement & debit/credit ledger"; color: "#64748B"; font.pixelSize: 11 }
                    }
                    KbdBadge { text: "3"; badgeColor: "#EFF6FF"; textColor: "#2563EB"; borderColor: "#BFDBFE" }
                }
            }

            // 4. New Group
            Rectangle {
                id: item4
                Layout.fillWidth: true
                height: 52
                radius: 8
                color: mouse4.containsMouse ? "#EFF6FF" : "#F8FAFC"
                border.color: mouse4.containsMouse ? "#2563EB" : "#E2E8F0"
                border.width: 1

                MouseArea {
                    id: mouse4
                    anchors.fill: parent
                    hoverEnabled: true
                    onClicked: root.actionSelected("New Group")
                }

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 14; anchors.rightMargin: 14
                    spacing: 12
                    Text { text: "📁"; font.pixelSize: 16 }
                    ColumnLayout {
                        spacing: 1
                        Layout.fillWidth: true
                        Text { text: "4. New Account Group"; color: "#0F172A"; font.pixelSize: 13; font.bold: true }
                        Text { text: "Add custom chart of accounts group under Debtors/Creditors"; color: "#64748B"; font.pixelSize: 11 }
                    }
                    KbdBadge { text: "4"; badgeColor: "#EFF6FF"; textColor: "#2563EB"; borderColor: "#BFDBFE" }
                }
            }

            // 5. Modify Group
            Rectangle {
                id: item5
                Layout.fillWidth: true
                height: 52
                radius: 8
                color: mouse5.containsMouse ? "#EFF6FF" : "#F8FAFC"
                border.color: mouse5.containsMouse ? "#2563EB" : "#E2E8F0"
                border.width: 1

                MouseArea {
                    id: mouse5
                    anchors.fill: parent
                    hoverEnabled: true
                    onClicked: root.actionSelected("Modify Group")
                }

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 14; anchors.rightMargin: 14
                    spacing: 12
                    Text { text: "📝"; font.pixelSize: 16 }
                    ColumnLayout {
                        spacing: 1
                        Layout.fillWidth: true
                        Text { text: "5. Modify Account Group"; color: "#0F172A"; font.pixelSize: 13; font.bold: true }
                        Text { text: "Update account group settings and tax classifications"; color: "#64748B"; font.pixelSize: 11 }
                    }
                    KbdBadge { text: "5"; badgeColor: "#EFF6FF"; textColor: "#2563EB"; borderColor: "#BFDBFE" }
                }
            }
        }

        Item { Layout.fillHeight: true }
    }
}
