import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../components"

Rectangle {
    id: root
    width: 600
    height: 520
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
                text: "👤 Quick Create Ledger Account"
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
                id: nameInput
                label: "Party Name *"
                placeholderText: "e.g. Ramesh Kumar (Farmer)"
                isRequired: true
                focusInput: true
                Layout.fillWidth: true
                onReturnPressed: typeCombo.focusAndOpen()
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: 12

                CustomWhiteCombo {
                    id: typeCombo
                    label: "Party Type *"
                    Layout.fillWidth: true
                    model: ["Farmer", "Buyer", "Vendor", "Transporter", "Broker"]
                    onReturnPressed: phoneInput.focusInput = true
                }

                CustomInput {
                    id: phoneInput
                    label: "Phone Number"
                    placeholderText: "9876543210"
                    inputMethodHints: Qt.ImhDigitsOnly
                    Layout.fillWidth: true
                    onReturnPressed: placeCombo.focusAndOpen()
                }
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: 12

                CustomWhiteCombo {
                    id: placeCombo
                    label: "City / Place"
                    Layout.fillWidth: true
                    editable: true
                    model: (typeof partiesModel !== "undefined" && partiesModel) ? partiesModel.get_cities() : ["Raichur", "Koppal", "Bengaluru", "Hyderabad"]
                    onReturnPressed: gstinInput.focusInput = true
                }

                CustomInput {
                    id: gstinInput
                    label: "GSTIN (Optional)"
                    placeholderText: "29ABCDE1234F1Z5"
                    Layout.fillWidth: true
                    onReturnPressed: balInput.focusInput = true
                }
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: 12

                CustomInput {
                    id: balInput
                    label: "Opening Balance ₹"
                    placeholderText: "0.0"
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
                background: Rectangle { color: saveBtn.hovered ? "#1D4ED8" : "#2563EB"; radius: 6 }
                contentItem: RowLayout {
                    spacing: 6
                    Text { text: "💾 Save Party Ledger"; color: "#FFFFFF"; font.bold: true; font.pixelSize: 13 }
                    KbdBadge { text: "Enter"; badgeColor: "#1E3A8A"; textColor: "#93C5FD"; borderColor: "#2563EB" }
                }
                onClicked: {
                    var success = partiesModel.add_party(
                        nameInput.text,
                        typeCombo.currentText,
                        phoneInput.text,
                        placeCombo.currentText,
                        gstinInput.text,
                        parseFloat(balInput.text) || 0.0
                    )
                    if (success) {
                        root.savedSuccess()
                    }
                }
            }
        }
    }
}
