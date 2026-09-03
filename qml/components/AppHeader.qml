import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: root
    height: 44
    color: "#FFFFFF"
    border.color: "#E2E8F0"
    border.width: 1

    property string activePeriodText: ""

    signal showHelpRequested()
    signal openAccountingPeriodRequested()
    signal openMdbMigrationRequested()
    signal switchFirmRequested()

    Rectangle {
        anchors.fill: parent
        color: "#FFFFFF"
    }

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: 16
        anchors.rightMargin: 16
        spacing: 16

        // Brand & Application Title
        RowLayout {
            spacing: 10

            Rectangle {
                width: 28
                height: 28
                radius: 6
                color: "#1E3A8A"
                Text {
                    anchors.centerIn: parent
                    text: "🌾"
                    font.pixelSize: 16
                }
            }

            ColumnLayout {
                spacing: 1
                Text {
                    text: (typeof firmManager !== "undefined" && firmManager && firmManager.currentFirmName !== "") ? firmManager.currentFirmName : "MAHADEV RICE INDUSTRY"
                    color: "#0F172A"
                    font.pixelSize: 13
                    font.bold: true
                    font.letterSpacing: 0.5
                }
                Text {
                    text: "Enterprise Resource Planning & Bahi-Khata"
                    color: "#64748B"
                    font.pixelSize: 10
                }
            }

            // Switch Firm Pill
            Rectangle {
                height: 24
                Layout.preferredWidth: switchFirmRow.implicitWidth + 14
                radius: 12
                color: switchFirmMouse.containsMouse ? "#EFF6FF" : "#F1F5F9"
                border.color: switchFirmMouse.containsMouse ? "#3B82F6" : "#CBD5E1"
                border.width: 1

                RowLayout {
                    id: switchFirmRow
                    anchors.centerIn: parent
                    spacing: 4
                    Text { text: "🏛️"; font.pixelSize: 10 }
                    Text {
                        text: "Switch Firm (Alt+F1)"
                        color: switchFirmMouse.containsMouse ? "#2563EB" : "#475569"
                        font.pixelSize: 10
                        font.bold: true
                    }
                }

                MouseArea {
                    id: switchFirmMouse
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: root.switchFirmRequested()
                }
            }
        }

        // Active Accounting Period Selector Pill (Bahi-Khata FY Engine)
        Rectangle {
            id: periodPill
            height: 26
            Layout.preferredWidth: periodRow.implicitWidth + 20
            radius: 13
            color: periodMouseArea.containsMouse ? "#EFF6FF" : "#F8FAFC"
            border.color: periodMouseArea.containsMouse ? "#3B82F6" : "#E2E8F0"
            border.width: 1

            RowLayout {
                id: periodRow
                anchors.centerIn: parent
                spacing: 6

                Text {
                    text: "📅"
                    font.pixelSize: 11
                }

                Text {
                    text: root.activePeriodText !== "" ? root.activePeriodText : "Period: (2025-04-01 To 2026-03-31) FY 2025-26"
                    color: "#1E40AF"
                    font.pixelSize: 11
                    font.bold: true
                }

                KbdBadge {
                    text: "F2"
                    badgeColor: "#DBEAFE"
                    textColor: "#1E40AF"
                    borderColor: "#BFDBFE"
                }
            }

            MouseArea {
                id: periodMouseArea
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: root.openAccountingPeriodRequested()
            }
        }

        Item { Layout.fillWidth: true }

        // Live Date / Time & System Status
        RowLayout {
            spacing: 10

            // Sync / Import Bahi Khata Button
            Button {
                id: mdbBtn
                height: 26
                background: Rectangle {
                    color: mdbBtn.hovered ? "#EFF6FF" : "#F8FAFC"
                    radius: 5
                    border.color: mdbBtn.hovered ? "#2563EB" : "#CBD5E1"
                }
                contentItem: RowLayout {
                    spacing: 6
                    Text {
                        text: "🔄 Sync MDB"
                        color: "#2563EB"
                        font.pixelSize: 11
                        font.bold: true
                    }
                }
                onClicked: root.openMdbMigrationRequested()
            }

            Rectangle {
                height: 24
                width: 120
                radius: 12
                color: "#F1F5F9"
                border.color: "#CBD5E1"

                ToolTip.visible: dbMouseArea.containsMouse
                ToolTip.text: "Active SQLite DB:\n" + ((typeof dashboardCtrl !== "undefined" && dashboardCtrl) ? dashboardCtrl.dbPath : "")
                ToolTip.delay: 200

                MouseArea {
                    id: dbMouseArea
                    anchors.fill: parent
                    hoverEnabled: true
                }

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
