import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../components"

Rectangle {
    id: root
    width: 600
    height: 440
    color: "#FFFFFF"
    radius: 12
    border.color: "#CBD5E1"
    border.width: 1

    signal closeRequested()
    signal optionSelected(int optionIndex)

    function handleKeyPress(event) {
        if (event.key === Qt.Key_1 || event.key === Qt.Key_S) {
            root.optionSelected(1)
            event.accepted = true
        } else if (event.key === Qt.Key_2 || event.key === Qt.Key_P) {
            root.optionSelected(2)
            event.accepted = true
        } else if (event.key === Qt.Key_3 || event.key === Qt.Key_J) {
            root.optionSelected(3)
            event.accepted = true
        } else if (event.key === Qt.Key_4 || event.key === Qt.Key_M) {
            root.optionSelected(4)
            event.accepted = true
        } else if (event.key === Qt.Key_Escape) {
            root.closeRequested()
            event.accepted = true
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 20
        spacing: 16

        // Modal Header
        RowLayout {
            Layout.fillWidth: true
            spacing: 10

            Rectangle {
                width: 38; height: 38; radius: 8; color: "#EFF6FF"
                Text { anchors.centerIn: parent; text: "📑"; font.pixelSize: 20 }
            }

            ColumnLayout {
                spacing: 0
                Text { text: "SELECT VOUCHER ENTRY TYPE"; color: "#2563EB"; font.pixelSize: 11; font.bold: true; font.letterSpacing: 1.0 }
                Text { text: "Add New Accounting & Inventory Voucher"; color: "#0F172A"; font.pixelSize: 17; font.bold: true }
            }

            Item { Layout.fillWidth: true }

            Button {
                id: closeBtn
                width: 32; height: 32
                background: Rectangle { color: closeBtn.hovered ? "#DC2626" : "#F1F5F9"; radius: 16 }
                contentItem: Text { text: "✕"; color: closeBtn.hovered ? "#FFF" : "#475569"; font.bold: true; horizontalAlignment: Text.AlignHCenter }
                onClicked: root.closeRequested()
            }
        }

        Rectangle { Layout.fillWidth: true; height: 1; color: "#E2E8F0" }

        Text {
            text: "Choose the voucher category to record new transaction entries:"
            color: "#64748B"
            font.pixelSize: 12
        }

        // 4 VOUCHER SUBMENU CARDS
        ColumnLayout {
            Layout.fillWidth: true
            spacing: 10

            // 1. Sales Voucher
            Rectangle {
                Layout.fillWidth: true
                height: 58
                color: mouse1.containsMouse ? "#EFF6FF" : "#F8FAFC"
                border.color: mouse1.containsMouse ? "#2563EB" : "#E2E8F0"
                border.width: mouse1.containsMouse ? 2 : 1
                radius: 8

                MouseArea {
                    id: mouse1
                    anchors.fill: parent
                    hoverEnabled: true
                    onClicked: root.optionSelected(1)
                }

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 14; anchors.rightMargin: 14
                    spacing: 12

                    KbdBadge { text: "1"; badgeColor: "#2563EB"; textColor: "#FFF"; borderColor: "#1D4ED8" }

                    ColumnLayout {
                        spacing: 2
                        Text { text: "🛍️ Sales Voucher (Tax Invoice)"; color: "#0F172A"; font.pixelSize: 14; font.bold: true }
                        Text { text: "Record sales invoices for finished rice, broken rice, rice bran & by-products"; color: "#64748B"; font.pixelSize: 11 }
                    }

                    Item { Layout.fillWidth: true }
                    Text { text: "→"; color: "#2563EB"; font.pixelSize: 16; font.bold: true }
                }
            }

            // 2. Purchase Voucher / Paddy Slip
            Rectangle {
                Layout.fillWidth: true
                height: 58
                color: mouse2.containsMouse ? "#F0FDF4" : "#F8FAFC"
                border.color: mouse2.containsMouse ? "#16A34A" : "#E2E8F0"
                border.width: mouse2.containsMouse ? 2 : 1
                radius: 8

                MouseArea {
                    id: mouse2
                    anchors.fill: parent
                    hoverEnabled: true
                    onClicked: root.optionSelected(2)
                }

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 14; anchors.rightMargin: 14
                    spacing: 12

                    KbdBadge { text: "2"; badgeColor: "#16A34A"; textColor: "#FFF"; borderColor: "#15803D" }

                    ColumnLayout {
                        spacing: 2
                        Text { text: "🌾 Purchase Voucher / Paddy Arrival Slip"; color: "#0F172A"; font.pixelSize: 14; font.bold: true }
                        Text { text: "Record raw paddy arrivals, gate passes, moisture deduction & purchase bills"; color: "#64748B"; font.pixelSize: 11 }
                    }

                    Item { Layout.fillWidth: true }
                    Text { text: "→"; color: "#16A34A"; font.pixelSize: 16; font.bold: true }
                }
            }

            // 3. Journal Voucher / Contra
            Rectangle {
                Layout.fillWidth: true
                height: 58
                color: mouse3.containsMouse ? "#FEF3C7" : "#F8FAFC"
                border.color: mouse3.containsMouse ? "#D97706" : "#E2E8F0"
                border.width: mouse3.containsMouse ? 2 : 1
                radius: 8

                MouseArea {
                    id: mouse3
                    anchors.fill: parent
                    hoverEnabled: true
                    onClicked: root.optionSelected(3)
                }

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 14; anchors.rightMargin: 14
                    spacing: 12

                    KbdBadge { text: "3"; badgeColor: "#D97706"; textColor: "#FFF"; borderColor: "#B45309" }

                    ColumnLayout {
                        spacing: 2
                        Text { text: "📑 Journal Voucher / Bank & Cash Transfer"; color: "#0F172A"; font.pixelSize: 14; font.bold: true }
                        Text { text: "Record cash receipts, bank payments, contra transfers & adjustment journal entries"; color: "#64748B"; font.pixelSize: 11 }
                    }

                    Item { Layout.fillWidth: true }
                    Text { text: "→"; color: "#D97706"; font.pixelSize: 16; font.bold: true }
                }
            }

            // 4. Milling Process Voucher
            Rectangle {
                Layout.fillWidth: true
                height: 58
                color: mouse4.containsMouse ? "#F3E8FF" : "#F8FAFC"
                border.color: mouse4.containsMouse ? "#9333EA" : "#E2E8F0"
                border.width: mouse4.containsMouse ? 2 : 1
                radius: 8

                MouseArea {
                    id: mouse4
                    anchors.fill: parent
                    hoverEnabled: true
                    onClicked: root.optionSelected(4)
                }

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 14; anchors.rightMargin: 14
                    spacing: 12

                    KbdBadge { text: "4"; badgeColor: "#9333EA"; textColor: "#FFF"; borderColor: "#7E22CE" }

                    ColumnLayout {
                        spacing: 2
                        Text { text: "⚙️ Milling Process Yield Production Voucher"; color: "#0F172A"; font.pixelSize: 14; font.bold: true }
                        Text { text: "Record raw paddy input milling batches, head rice, broken rice, bran & husk yields"; color: "#64748B"; font.pixelSize: 11 }
                    }

                    Item { Layout.fillWidth: true }
                    Text { text: "→"; color: "#9333EA"; font.pixelSize: 16; font.bold: true }
                }
            }
        }
    }
}
