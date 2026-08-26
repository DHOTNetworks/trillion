import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../components"

Rectangle {
    id: root
    width: 620
    height: 560
    color: "#FFFFFF"
    radius: 12
    border.color: "#CBD5E1"
    border.width: 1

    signal closeRequested()
    signal savedSuccess()

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 20
        spacing: 16

        // Modal Header
        RowLayout {
            Layout.fillWidth: true

            Text {
                text: "🌾 New Paddy Arrival Entry"
                color: "#0F172A"
                font.pixelSize: 18
                font.bold: true
            }

            Item { Layout.fillWidth: true }

            Button {
                flat: true
                text: "✕"
                font.pixelSize: 16
                font.bold: true
                onClicked: root.closeRequested()
            }
        }

        Rectangle { Layout.fillWidth: true; height: 1; color: "#E2E8F0" }

        // Form Fields
        ColumnLayout {
            Layout.fillWidth: true
            spacing: 12

            CustomInput {
                id: farmerInput
                label: "Farmer / Supplier Name *"
                placeholderText: "Type farmer or vendor name..."
                isRequired: true
                focusInput: true
                Layout.fillWidth: true
                onReturnPressed: varietyCombo.focusAndOpen()
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: 12

                CustomWhiteCombo {
                    id: varietyCombo
                    label: "Paddy Variety *"
                    Layout.fillWidth: true
                    model: ["Sona Masoori", "BPT 5204", "RNR 15048", "IR 64", "Jeera Rice", "HMT Basmati"]
                    onReturnPressed: bagsInput.focusInput = true
                }

                CustomInput {
                    id: bagsInput
                    label: "Total Bags *"
                    placeholderText: "e.g. 200"
                    inputMethodHints: Qt.ImhDigitsOnly
                    Layout.fillWidth: true
                    onReturnPressed: grossInput.focusInput = true
                }
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: 12

                CustomInput {
                    id: grossInput
                    label: "Gross Weight (Qtl) *"
                    placeholderText: "e.g. 150.0"
                    inputMethodHints: Qt.ImhFormattedNumbersOnly
                    Layout.fillWidth: true
                    onReturnPressed: moistureInput.focusInput = true
                }

                CustomInput {
                    id: moistureInput
                    label: "Moisture %"
                    placeholderText: "14.0"
                    inputMethodHints: Qt.ImhFormattedNumbersOnly
                    Layout.fillWidth: true
                    onReturnPressed: rateInput.focusInput = true
                }

                CustomInput {
                    id: rateInput
                    label: "Purchase Rate (₹/Qtl) *"
                    placeholderText: "2450.00"
                    inputMethodHints: Qt.ImhFormattedNumbersOnly
                    Layout.fillWidth: true
                    onReturnPressed: saveBtn.focus = true
                }
            }
        }

        Item { Layout.fillHeight: true }

        // Action Buttons
        RowLayout {
            Layout.fillWidth: true
            spacing: 12

            Button {
                text: "Cancel"
                onClicked: root.closeRequested()
            }

            Item { Layout.fillWidth: true }

            Button {
                id: saveBtn
                background: Rectangle { color: saveBtn.hovered ? "#16A34A" : "#22C55E"; radius: 6 }
                contentItem: RowLayout {
                    spacing: 6
                    Text { text: "💾 Record Arrival Slip"; color: "#FFFFFF"; font.bold: true; font.pixelSize: 13 }
                    KbdBadge { text: "Enter"; badgeColor: "#14532D"; textColor: "#86EFAC"; borderColor: "#22C55E" }
                }
                onClicked: {
                    var success = paddyModel.add_arrival(
                        farmerInput.text,
                        varietyCombo.currentText,
                        parseInt(bagsInput.text) || 0,
                        parseFloat(grossInput.text) || 0.0,
                        parseFloat(moistureInput.text) || 14.0,
                        parseFloat(rateInput.text) || 0.0
                    )
                    if (success) {
                        root.savedSuccess()
                    }
                }
            }
        }
    }
}
