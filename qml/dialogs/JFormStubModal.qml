import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../components"

Popup {
    id: root
    width: 520
    implicitHeight: mainCol.implicitHeight + 36
    modal: true
    dim: true
    focus: true
    anchors.centerIn: parent
    closePolicy: Popup.CloseOnPressOutside | Popup.CloseOnEscape

    background: Rectangle {
        color: "#FFFFFF"
        border.color: "#16A34A"
        border.width: 2.5
        radius: 12
    }

    contentItem: FocusScope {
        id: stubScope
        anchors.fill: parent
        focus: true

        Keys.onEscapePressed: function(event) {
            event.accepted = true
            root.close()
        }
        Keys.onReturnPressed: function(event) {
            event.accepted = true
            root.close()
        }
        Keys.onEnterPressed: function(event) {
            event.accepted = true
            root.close()
        }

        ColumnLayout {
            id: mainCol
            anchors.fill: parent
            anchors.margins: 20
            spacing: 14

            // Header Banner
            RowLayout {
                Layout.fillWidth: true
                spacing: 10

                Rectangle {
                    width: 42; height: 42; radius: 10
                    color: "#DCFCE7"
                    border.color: "#86EFAC"
                    Text { anchors.centerIn: parent; text: "🌾"; font.pixelSize: 22 }
                }

                ColumnLayout {
                    spacing: 2
                    Layout.fillWidth: true
                    Text { text: "J-Form Mandi Procurement Voucher (Form J)"; color: "#0F172A"; font.pixelSize: 16; font.bold: true }
                    Text { text: "APMC Mandi Sale Slip & Grain Purchase Voucher"; color: "#16A34A"; font.pixelSize: 11; font.bold: true }
                }

                Button {
                    flat: true
                    text: "✕"
                    onClicked: root.close()
                }
            }

            Rectangle { Layout.fillWidth: true; height: 1; color: "#E2E8F0" }

            // Stub Notification Box
            Rectangle {
                Layout.fillWidth: true
                implicitHeight: stubInfoCol.implicitHeight + 24
                color: "#F0FDF4"
                border.color: "#86EFAC"
                radius: 8

                ColumnLayout {
                    id: stubInfoCol
                    anchors.fill: parent
                    anchors.margins: 14
                    spacing: 8

                    Text {
                        text: "📋 J-Form Module Stub Initialized"
                        color: "#15803D"
                        font.pixelSize: 13
                        font.bold: true
                    }

                    Text {
                        text: "Form J (APMC Mandi Sale Slip) is reserved for direct paddy/grain purchase recording from Mandi Farmers & Commission Agents (Katcha Arhtia)."
                        color: "#334155"
                        font.pixelSize: 12
                        wrapMode: Text.WordWrap
                        Layout.fillWidth: true
                    }

                    Text {
                        text: "⚡ Features configured for upcoming build:\n • Mandi Tax (1% / 2%) & Market Fee calculation\n • Rural Development Fund (RDF) & Infrastructure cess\n • Damami & Commission Agent charges\n • Net Bag Weight & Quality Grade deduction"
                        color: "#475569"
                        font.pixelSize: 11
                        wrapMode: Text.WordWrap
                        Layout.fillWidth: true
                    }
                }
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: 12

                Item { Layout.fillWidth: true }

                Button {
                    height: 36
                    background: Rectangle { color: "#16A34A"; radius: 6 }
                    contentItem: RowLayout {
                        spacing: 6
                        Text { text: "✓ Understood (Close)"; color: "#FFFFFF"; font.bold: true; font.pixelSize: 12 }
                        KbdBadge { text: "Esc"; badgeColor: "#14532D"; textColor: "#86EFAC"; borderColor: "#16A34A" }
                    }
                    onClicked: root.close()
                }
            }
        }
    }
}
