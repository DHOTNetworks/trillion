import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../components"
import "../dialogs"

ScrollView {
    id: root
    contentWidth: availableWidth
    clip: true

    signal cancelRequested()
    signal savedSuccess()

    ColumnLayout {
        width: root.availableWidth > 0 ? root.availableWidth : 1200
        spacing: 20

        // Page Header Bar
        RowLayout {
            Layout.fillWidth: true
            spacing: 12

            ColumnLayout {
                spacing: 2
                Text {
                    text: "📖 Create New Ledger Account"
                    color: "#0F172A"
                    font.pixelSize: 20
                    font.bold: true
                }
                Text {
                    text: "Enter accounting group, tax compliance, bank & credit term details."
                    color: "#64748B"
                    font.pixelSize: 12
                }
            }

            Item { Layout.fillWidth: true }

            Button {
                id: backBtn
                background: Rectangle { color: "#F1F5F9"; radius: 6; border.color: "#CBD5E1" }
                contentItem: RowLayout {
                    spacing: 6
                    Text { text: "← Back to Dashboard"; color: "#475569"; font.pixelSize: 13; font.bold: true }
                    KbdBadge { text: "Esc"; badgeColor: "#DC2626"; textColor: "#FFF"; borderColor: "#B91C1C" }
                }
                onClicked: root.cancelRequested()
            }
        }

        Rectangle { Layout.fillWidth: true; height: 1; color: "#E2E8F0" }

        // SECTION 1: ACCOUNT IDENTIFICATION & GROUPING
        Rectangle {
            Layout.fillWidth: true
            implicitHeight: secCol1.implicitHeight + 28
            color: "#FFFFFF"
            border.color: "#E2E8F0"
            border.width: 1
            radius: 10

            ColumnLayout {
                id: secCol1
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.top: parent.top
                anchors.margins: 14
                spacing: 12

                Text {
                    text: "1. ACCOUNT IDENTIFICATION & GROUPING"
                    color: "#2563EB"
                    font.pixelSize: 11
                    font.bold: true
                    font.letterSpacing: 1.0
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 12

                    CustomInput {
                        id: nameInput
                        label: "Ledger Account Name"
                        placeholderText: "e.g. Sri Venkatesh Traders"
                        isRequired: true
                        focusInput: true
                        Layout.fillWidth: true
                        onReturnPressed: aliasInput.focusInput = true
                    }

                    CustomInput {
                        id: aliasInput
                        label: "Short Alias / Code"
                        placeholderText: "SVT-BLR"
                        Layout.preferredWidth: 200
                        onReturnPressed: groupCombo.focusAndOpen()
                    }
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 12

                    CustomWhiteCombo {
                        id: groupCombo
                        label: "Account Group *"
                        Layout.fillWidth: true
                        model: (typeof partiesModel !== "undefined" && partiesModel) ? partiesModel.get_account_groups() : []
                        onReturnPressed: partyTypeInput.focusInput = true
                    }

                    CustomInput {
                        id: partyTypeInput
                        label: "Party Type"
                        placeholderText: "Farmer, Buyer, Vendor"
                        Layout.fillWidth: true
                        onReturnPressed: specialTypeInput.focusInput = true
                    }

                    CustomInput {
                        id: specialTypeInput
                        label: "Special Classification"
                        placeholderText: "Paddy Seller, Rice Buyer"
                        Layout.fillWidth: true
                        onReturnPressed: opBalInput.focusInput = true
                    }

                    CustomInput {
                        id: opBalInput
                        label: "Opening Balance (₹)"
                        placeholderText: "0.00"
                        inputMethodHints: Qt.ImhFormattedNumbersOnly
                        Layout.preferredWidth: 140
                        onReturnPressed: balTypeCombo.focusAndOpen()
                    }

                    CustomWhiteCombo {
                        id: balTypeCombo
                        label: "Dr / Cr *"
                        Layout.preferredWidth: 90
                        model: ["Dr", "Cr"]
                        onReturnPressed: mailingNameInput.focusInput = true
                    }
                }
            }
        }

        // SECTION 2: MAILING ADDRESS & CONTACT DETAILS
        Rectangle {
            Layout.fillWidth: true
            implicitHeight: secCol2.implicitHeight + 28
            color: "#FFFFFF"
            border.color: "#E2E8F0"
            border.width: 1
            radius: 10

            ColumnLayout {
                id: secCol2
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.top: parent.top
                anchors.margins: 14
                spacing: 12

                Text {
                    text: "2. MAILING ADDRESS & CONTACT DETAILS"
                    color: "#16A34A"
                    font.pixelSize: 11
                    font.bold: true
                    font.letterSpacing: 1.0
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 12

                    CustomInput {
                        id: mailingNameInput
                        label: "Mailing / Billing Name"
                        placeholderText: "e.g. Sri Venkatesh Traders Pvt Ltd"
                        Layout.fillWidth: true
                        onReturnPressed: addressInput.focusInput = true
                    }

                    CustomInput {
                        id: addressInput
                        label: "Full Street Address"
                        placeholderText: "APMC Market Yard, Main Gate"
                        Layout.fillWidth: true
                        onReturnPressed: cityCombo.focusAndOpen()
                    }
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 12

                    CustomWhiteCombo {
                        id: cityCombo
                        label: "City / Town"
                        Layout.fillWidth: true
                        editable: true
                        model: (typeof partiesModel !== "undefined" && partiesModel) ? partiesModel.get_cities() : []
                        onReturnPressed: districtCombo.focusAndOpen()
                    }

                    CustomWhiteCombo {
                        id: districtCombo
                        label: "District"
                        Layout.fillWidth: true
                        editable: true
                        model: (typeof partiesModel !== "undefined" && partiesModel) ? partiesModel.get_districts() : []
                        onReturnPressed: stateCombo.focusAndOpen()
                    }

                    CustomWhiteCombo {
                        id: stateCombo
                        label: "Station / State"
                        Layout.fillWidth: true
                        editable: true
                        model: (typeof partiesModel !== "undefined" && partiesModel) ? partiesModel.get_stations() : []
                        onReturnPressed: pinInput.focusInput = true
                    }

                    CustomInput {
                        id: pinInput
                        label: "PIN Code"
                        placeholderText: "584101"
                        inputMethodHints: Qt.ImhDigitsOnly
                        Layout.preferredWidth: 120
                        onReturnPressed: phoneInput.focusInput = true
                    }
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 12

                    CustomInput {
                        id: phoneInput
                        label: "Phone / Landline"
                        placeholderText: "08532-234567"
                        Layout.fillWidth: true
                        onReturnPressed: mobileInput.focusInput = true
                    }

                    CustomInput {
                        id: mobileInput
                        label: "Mobile Number"
                        placeholderText: "9876543210"
                        inputMethodHints: Qt.ImhDigitsOnly
                        Layout.fillWidth: true
                        onReturnPressed: whatsappInput.focusInput = true
                    }

                    CustomInput {
                        id: whatsappInput
                        label: "WhatsApp Number"
                        placeholderText: "9876543210"
                        inputMethodHints: Qt.ImhDigitsOnly
                        Layout.fillWidth: true
                        onReturnPressed: emailInput.focusInput = true
                    }

                    CustomInput {
                        id: emailInput
                        label: "Email Address"
                        placeholderText: "accounts@party.com"
                        Layout.fillWidth: true
                        onReturnPressed: contactPersonInput.focusInput = true
                    }

                    CustomInput {
                        id: contactPersonInput
                        label: "Contact Person"
                        placeholderText: "Mr. Venkatesh Rao"
                        Layout.fillWidth: true
                        onReturnPressed: gstinInput.focusInput = true
                    }
                }
            }
        }

        // SECTION 3: STATUTORY TAX & BANKING COMPLIANCE
        Rectangle {
            Layout.fillWidth: true
            implicitHeight: secCol3.implicitHeight + 28
            color: "#FFFFFF"
            border.color: "#E2E8F0"
            border.width: 1
            radius: 10

            ColumnLayout {
                id: secCol3
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.top: parent.top
                anchors.margins: 14
                spacing: 12

                Text {
                    text: "3. STATUTORY TAX, BANKING & CREDIT TERMS"
                    color: "#D97706"
                    font.pixelSize: 11
                    font.bold: true
                    font.letterSpacing: 1.0
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 12

                    CustomInput {
                        id: gstinInput
                        label: "GSTIN Number"
                        placeholderText: "29ABCDE1234F1Z5"
                        Layout.fillWidth: true
                        onReturnPressed: panInput.focusInput = true
                    }

                    CustomInput {
                        id: panInput
                        label: "PAN Number"
                        placeholderText: "ABCDE1234F"
                        Layout.fillWidth: true
                        onReturnPressed: aadhaarInput.focusInput = true
                    }

                    CustomInput {
                        id: aadhaarInput
                        label: "Aadhaar Number"
                        placeholderText: "9988-7766-5544"
                        inputMethodHints: Qt.ImhDigitsOnly
                        Layout.fillWidth: true
                        onReturnPressed: bankNameInput.focusInput = true
                    }
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 12

                    CustomInput {
                        id: bankNameInput
                        label: "Bank Name"
                        placeholderText: "HDFC Bank, SBI, Canara"
                        Layout.fillWidth: true
                        onReturnPressed: bankAccountInput.focusInput = true
                    }

                    CustomInput {
                        id: bankAccountInput
                        label: "Bank Account Number"
                        placeholderText: "5010099887766"
                        Layout.fillWidth: true
                        onReturnPressed: ifscInput.focusInput = true
                    }

                    CustomInput {
                        id: ifscInput
                        label: "Bank IFSC Code"
                        placeholderText: "HDFC0000123"
                        Layout.preferredWidth: 160
                        onReturnPressed: creditLimitInput.focusInput = true
                    }

                    CustomInput {
                        id: creditLimitInput
                        label: "Credit Limit (₹)"
                        placeholderText: "500000.00"
                        inputMethodHints: Qt.ImhFormattedNumbersOnly
                        Layout.fillWidth: true
                        onReturnPressed: creditDaysInput.focusInput = true
                    }

                    CustomInput {
                        id: creditDaysInput
                        label: "Credit Days"
                        placeholderText: "30"
                        inputMethodHints: Qt.ImhDigitsOnly
                        Layout.preferredWidth: 110
                        onReturnPressed: submitBtn.focus = true
                    }
                }
            }
        }

        // SAVE & CANCEL ACTION BAR
        RowLayout {
            Layout.fillWidth: true
            spacing: 14

            Button {
                id: cancelBottomBtn
                background: Rectangle { color: "#F1F5F9"; radius: 6; border.color: "#CBD5E1" }
                contentItem: RowLayout {
                    spacing: 6
                    Text { text: "Cancel"; color: "#475569"; font.pixelSize: 13 }
                    KbdBadge { text: "Esc"; badgeColor: "#DC2626"; textColor: "#FFF"; borderColor: "#B91C1C" }
                }
                onClicked: root.cancelRequested()
            }

            Item { Layout.fillWidth: true }

            Button {
                id: submitBtn
                background: Rectangle { color: submitBtn.hovered ? "#1D4ED8" : "#2563EB"; radius: 6 }
                contentItem: RowLayout {
                    spacing: 6
                    Text { text: "💾 Save Complete Ledger Account"; color: "#FFFFFF"; font.bold: true; font.pixelSize: 14 }
                    KbdBadge { text: "Enter"; badgeColor: "#1E3A8A"; textColor: "#93C5FD"; borderColor: "#2563EB" }
                }
                onClicked: root.saveLedger()
            }
        }
    }

    function saveLedger() {
        if (!nameInput.text.trim()) {
            return
        }
        saveConfirmModal.open()
    }

    function executeSaveLedger() {
        var success = partiesModel.add_ledger_full(
            nameInput.text,
            aliasInput.text,
            "",
            groupCombo.currentText,
            partyTypeInput.text,
            specialTypeInput.text,
            parseFloat(opBalInput.text) || 0.0,
            balTypeCombo.currentText,
            mailingNameInput.text !== "" ? mailingNameInput.text : nameInput.text,
            addressInput.text,
            cityCombo.editText !== "" ? cityCombo.editText : cityCombo.currentText,
            districtCombo.editText !== "" ? districtCombo.editText : districtCombo.currentText,
            stateCombo.editText !== "" ? stateCombo.editText : stateCombo.currentText,
            pinInput.text,
            phoneInput.text,
            mobileInput.text,
            whatsappInput.text,
            emailInput.text,
            contactPersonInput.text,
            gstinInput.text,
            panInput.text,
            aadhaarInput.text,
            parseFloat(creditLimitInput.text) || 0.0,
            parseInt(creditDaysInput.text) || 0,
            bankNameInput.text,
            bankAccountInput.text,
            ifscInput.text,
            "",
            ""
        )
        if (success) {
            root.savedSuccess()
        }
    }

    ConfirmationModal {
        id: saveConfirmModal
        anchors.centerIn: parent
        titleText: "CONFIRM LEDGER SAVE"
        messageText: "Are you sure you want to save & create Ledger Account '" + nameInput.text.trim() + "' under group '" + groupCombo.currentText + "'?"
        onConfirmed: root.executeSaveLedger()
    }
}
