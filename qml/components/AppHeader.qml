import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: root
    height: 44
    color: "#FFFFFF"
    border.color: "#E2E8F0"
    border.width: 1

    signal showHelpRequested()

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: 16
        anchors.rightMargin: 16
        spacing: 12

        // Logo & Title
        RowLayout {
            spacing: 10
            Rectangle {
                width: 28
                height: 28
                radius: 6
                color: "#16A34A"
                Text {
                    anchors.centerIn: parent
                    text: "🌾"
                    font.pixelSize: 15
                }
            }

            ColumnLayout {
                spacing: 0
                Text {
                    text: "MAHADEV RICE MILLING ERP"
                    color: "#0F172A"
                    font.pixelSize: 14
                    font.bold: true
                    font.letterSpacing: 0.5
                }
                Text {
                    text: "Professional Accounting & Inventory System | FY 2026-27"
                    color: "#64748B"
                    font.pixelSize: 10
                }
            }
        }

        Item { Layout.fillWidth: true }

        // Live Date / Time & System Status
        RowLayout {
            spacing: 12

            Rectangle {
                height: 24
                width: 130
                radius: 12
                color: "#F1F5F9"
                border.color: "#CBD5E1"

                RowLayout {
                    anchors.centerIn: parent
                    spacing: 6
                    Rectangle {
                        width: 7
                        height: 7
                        radius: 3.5
                        color: "#16A34A"
                    }
                    Text {
                        text: "Database Active"
                        color: "#1E293B"
                        font.pixelSize: 10
                        font.bold: true
                    }
                }
            }

            // Keyboard Help Button
            Button {
                id: helpBtn
                height: 26
                background: Rectangle {
                    color: helpBtn.hovered ? "#EFF6FF" : "#F8FAFC"
                    radius: 5
                    border.color: helpBtn.hovered ? "#2563EB" : "#CBD5E1"
                }
                contentItem: RowLayout {
                    spacing: 6
                    Text {
                        text: "⌨ Shortcuts"
                        color: "#2563EB"
                        font.pixelSize: 11
                        font.bold: true
                    }
                    KbdBadge {
                        text: "F1"
                        badgeColor: "#2563EB"
                        textColor: "#FFFFFF"
                        borderColor: "#1D4ED8"
                    }
                }
                onClicked: root.showHelpRequested()
            }
        }
    }
}
