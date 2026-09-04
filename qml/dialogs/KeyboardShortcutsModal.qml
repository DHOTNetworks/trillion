import QtQuick
import QtQuick.Templates as T
import QtQuick.Layouts
import MahadevERP

Rectangle {
    id: root
    width: 620
    height: 500
    color: "#FFFFFF"
    border.color: "#E2E8F0"
    border.width: 1
    radius: 12

    signal closeRequested()

    FocusScope {
        anchors.fill: parent
        focus: true
        Keys.onEscapePressed: root.closeRequested()
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 20
        spacing: 16

        // Modal Header
        RowLayout {
            Layout.fillWidth: true
            Text {
                text: "⌨ Keyboard Shortcuts Reference"
                color: "#0F172A"
                font.pixelSize: 18
                font.bold: true
                Layout.fillWidth: true
            }

            T.Button {
                id: closeBtn
                width: 28
                height: 28
                background: Rectangle { color: closeBtn.hovered ? "#DC2626" : "#F1F5F9"; radius: 14 }
                contentItem: Text {
                    text: "✕"
                    color: closeBtn.hovered ? "#FFF" : "#475569"
                    font.pixelSize: 13
                    font.bold: true
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
                onClicked: root.closeRequested()
            }
        }

        Rectangle { Layout.fillWidth: true; height: 1; color: "#E2E8F0" }

        // Shortcuts Table
        T.ScrollView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true

            ColumnLayout {
                width: parent.width
                spacing: 10

                Text {
                    text: "NAVIGATION HOTKEYS"
                    color: "#64748B"
                    font.pixelSize: 11
                    font.bold: true
                }

                GridLayout {
                    columns: 2
                    columnSpacing: 30
                    rowSpacing: 10

                    RowLayout {
                        spacing: 8
                        KbdBadge { text: "Alt + 1"; badgeColor: "#2563EB"; textColor: "#FFF"; borderColor: "#1D4ED8" }
                        Text { text: "Switch to Executive Dashboard"; color: "#1E293B"; font.pixelSize: 13 }
                    }
                    RowLayout {
                        spacing: 8
                        KbdBadge { text: "Alt + 2"; badgeColor: "#2563EB"; textColor: "#FFF"; borderColor: "#1D4ED8" }
                        Text { text: "Switch to Paddy Procurement"; color: "#1E293B"; font.pixelSize: 13 }
                    }
                    RowLayout {
                        spacing: 8
                        KbdBadge { text: "Alt + 3"; badgeColor: "#2563EB"; textColor: "#FFF"; borderColor: "#1D4ED8" }
                        Text { text: "Switch to Milling Process"; color: "#1E293B"; font.pixelSize: 13 }
                    }
                    RowLayout {
                        spacing: 8
                        KbdBadge { text: "Alt + 4"; badgeColor: "#2563EB"; textColor: "#FFF"; borderColor: "#1D4ED8" }
                        Text { text: "Switch to Sales & Invoicing"; color: "#1E293B"; font.pixelSize: 13 }
                    }
                    RowLayout {
                        spacing: 8
                        KbdBadge { text: "Alt + 5"; badgeColor: "#2563EB"; textColor: "#FFF"; borderColor: "#1D4ED8" }
                        Text { text: "Switch to Vouchers & Cash Book"; color: "#1E293B"; font.pixelSize: 13 }
                    }
                    RowLayout {
                        spacing: 8
                        KbdBadge { text: "Alt + 6"; badgeColor: "#2563EB"; textColor: "#FFF"; borderColor: "#1D4ED8" }
                        Text { text: "Switch to Reports & Ledgers"; color: "#1E293B"; font.pixelSize: 13 }
                    }
                }

                Rectangle { Layout.fillWidth: true; height: 1; color: "#E2E8F0"; Layout.topMargin: 5; Layout.bottomMargin: 5 }

                Text {
                    text: "ACTION & ENTRY SHORTCUTS"
                    color: "#64748B"
                    font.pixelSize: 11
                    font.bold: true
                }

                GridLayout {
                    columns: 2
                    columnSpacing: 30
                    rowSpacing: 10

                    RowLayout {
                        spacing: 8
                        KbdBadge { text: "F2"; badgeColor: "#16A34A"; textColor: "#FFF"; borderColor: "#15803D" }
                        Text { text: "Open New Voucher Entry Menu"; color: "#1E293B"; font.pixelSize: 13 }
                    }
                    RowLayout {
                        spacing: 8
                        KbdBadge { text: "F3 / F4 / F5"; badgeColor: "#7C3AED"; textColor: "#FFF"; borderColor: "#6D28D9" }
                        Text { text: "Payment (F3), Receipt (F4), Journal (F5)"; color: "#1E293B"; font.pixelSize: 13 }
                    }
                    RowLayout {
                        spacing: 8
                        KbdBadge { text: "F8 / F9"; badgeColor: "#2563EB"; textColor: "#FFF"; borderColor: "#1D4ED8" }
                        Text { text: "Sales Invoice (F8), Purchase Bill (F9)"; color: "#1E293B"; font.pixelSize: 13 }
                    }
                    RowLayout {
                        spacing: 8
                        KbdBadge { text: "F1 / ?"; badgeColor: "#2563EB"; textColor: "#FFF"; borderColor: "#1D4ED8" }
                        Text { text: "Open Keyboard Shortcuts Help"; color: "#1E293B"; font.pixelSize: 13 }
                    }
                    RowLayout {
                        spacing: 8
                        KbdBadge { text: "Enter / Tab"; badgeColor: "#CBD5E1"; textColor: "#0F172A"; borderColor: "#94A3B8" }
                        Text { text: "Advance focus to next input field"; color: "#1E293B"; font.pixelSize: 13 }
                    }
                    RowLayout {
                        spacing: 8
                        KbdBadge { text: "Esc"; badgeColor: "#DC2626"; textColor: "#FFF"; borderColor: "#B91C1C" }
                        Text { text: "Close Modal / Cancel Operation"; color: "#1E293B"; font.pixelSize: 13 }
                    }
                }
            }
        }
    }
}
