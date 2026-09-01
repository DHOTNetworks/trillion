import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../components"
import "../dialogs"

Item {
    id: root
    anchors.fill: parent

    signal cancelRequested()
    signal savedSuccess()

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 14
        spacing: 10

        // Page Header Bar
        RowLayout {
            Layout.fillWidth: true
            spacing: 12

            ColumnLayout {
                spacing: 1
                Text {
                    text: "📖 Create New Ledger Account"
                    color: "#0F172A"
                    font.pixelSize: 18
                    font.bold: true
                }
                Text {
                    text: "Single-slate accounting, tax compliance, bank & credit term entry system (Use Enter & ↑/↓/←/→ Arrow Keys)"
                    color: "#64748B"
                    font.pixelSize: 11
                }
            }

            Item { Layout.fillWidth: true }

            Button {
                id: backBtn
                background: Rectangle { color: "#F1F5F9"; radius: 6; border.color: "#CBD5E1" }
                contentItem: RowLayout {
                    spacing: 6
                    Text { text: "← Back to Dashboard"; color: "#475569"; font.pixelSize: 12; font.bold: true }
                    KbdBadge { text: "Esc"; badgeColor: "#DC2626"; textColor: "#FFF"; borderColor: "#B91C1C" }
                }
                onClicked: root.cancelRequested()
            }
        }

        Rectangle { Layout.fillWidth: true; height: 1; color: "#E2E8F0" }

        // MAIN SINGLE-SLATE 2-COLUMN GRID
        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 12

            // ==============================================================
            // LEFT COLUMN: ACCOUNT IDENTIFICATION, GROUP & TAX/BANK DETAILS
            // ==============================================================
            ColumnLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.preferredWidth: 1
                spacing: 10

                // CARD 1: ACCOUNT IDENTIFICATION & GROUPING
                Rectangle {
                    Layout.fillWidth: true
                    implicitHeight: secCol1.implicitHeight + 20
                    color: "#FFFFFF"
                    border.color: "#E2E8F0"
                    border.width: 1
                    radius: 8

                    ColumnLayout {
                        id: secCol1
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.top: parent.top
                        anchors.margins: 10
                        spacing: 8

                        Text {
                            text: "1. ACCOUNT IDENTIFICATION & GROUPING"
                            color: "#2563EB"
                            font.pixelSize: 11
                            font.bold: true
                            font.letterSpacing: 0.8
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 8

                            CustomInput {
                                id: nameInput
                                label: "Ledger Account Name *"
                                placeholderText: "e.g. Sri Venkatesh Traders"
                                isRequired: true
                                focusInput: true
                                Layout.fillWidth: true
                                onReturnPressed: aliasInput.focusInput = true
                                onRightPressed: aliasInput.focusInput = true
                                onDownPressed: groupCombo.focusAndOpen()
                            }

                            CustomInput {
                                id: aliasInput
                                label: "Short Alias / Code"
                                placeholderText: "SVT-BLR"
                                Layout.preferredWidth: 150
                                onReturnPressed: groupCombo.focusAndOpen()
                                onLeftPressed: nameInput.focusInput = true
                                onRightPressed: mailingNameInput.focusInput = true
                                onDownPressed: groupCombo.focusAndOpen()
                            }
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 8

                            CustomWhiteCombo {
                                id: groupCombo
                                label: "Account Group *"
                                Layout.fillWidth: true
                                model: (typeof partiesModel !== "undefined" && partiesModel) ? partiesModel.get_account_groups() : []
                                onReturnPressed: balTypeCombo.focusAndOpen()
                                onRightPressed: balTypeCombo.focusAndOpen()
                                onLeftPressed: nameInput.focusInput = true
                                onUpPressed: nameInput.focusInput = true
                                onDownPressed: partyTypeInput.focusInput = true
                            }

                            CustomWhiteCombo {
                                id: balTypeCombo
                                label: "Dr / Cr *"
                                Layout.preferredWidth: 85
                                model: ["Dr", "Cr"]
                                onReturnPressed: opBalInput.focusInput = true
                                onRightPressed: opBalInput.focusInput = true
                                onLeftPressed: groupCombo.focusAndOpen()
                                onUpPressed: aliasInput.focusInput = true
                                onDownPressed: specialTypeInput.focusInput = true
                            }

                            CustomInput {
                                id: opBalInput
                                label: "Opening Balance (₹)"
                                placeholderText: "0.00"
                                inputMethodHints: Qt.ImhFormattedNumbersOnly
                                Layout.preferredWidth: 130
                                onReturnPressed: partyTypeInput.focusInput = true
                                onRightPressed: addressInput.focusInput = true
                                onLeftPressed: balTypeCombo.focusAndOpen()
                                onUpPressed: aliasInput.focusInput = true
                                onDownPressed: specialTypeInput.focusInput = true
                            }
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 8

                            CustomInput {
                                id: partyTypeInput
                                label: "Party Type"
                                placeholderText: "Farmer, Buyer, Vendor"
                                Layout.fillWidth: true
                                onReturnPressed: specialTypeInput.focusInput = true
                                onRightPressed: specialTypeInput.focusInput = true
                                onLeftPressed: groupCombo.focusAndOpen()
                                onUpPressed: groupCombo.focusAndOpen()
                                onDownPressed: gstinInput.focusInput = true
                            }

                            CustomInput {
                                id: specialTypeInput
                                label: "Special Classification"
                                placeholderText: "Paddy Seller, Rice Buyer"
                                Layout.fillWidth: true
                                onReturnPressed: gstinInput.focusInput = true
                                onRightPressed: cityCombo.focusAndOpen()
                                onLeftPressed: partyTypeInput.focusInput = true
                                onUpPressed: opBalInput.focusInput = true
                                onDownPressed: panInput.focusInput = true
                            }
                        }
                    }
                }

                // CARD 2: STATUTORY TAX & BANKING COMPLIANCE
                Rectangle {
                    Layout.fillWidth: true
                    implicitHeight: secCol2.implicitHeight + 20
                    color: "#FFFFFF"
                    border.color: "#E2E8F0"
                    border.width: 1
                    radius: 8

                    ColumnLayout {
                        id: secCol2
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.top: parent.top
                        anchors.margins: 10
                        spacing: 8

                        Text {
                            text: "2. STATUTORY TAX, BANKING & CREDIT TERMS"
                            color: "#D97706"
                            font.pixelSize: 11
                            font.bold: true
                            font.letterSpacing: 0.8
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 8

                            CustomInput {
                                id: gstinInput
                                label: "GSTIN Number"
                                placeholderText: "29ABCDE1234F1Z5"
                                Layout.fillWidth: true
                                onReturnPressed: panInput.focusInput = true
                                onRightPressed: panInput.focusInput = true
                                onLeftPressed: partyTypeInput.focusInput = true
                                onUpPressed: partyTypeInput.focusInput = true
                                onDownPressed: bankNameInput.focusInput = true
                            }

                            CustomInput {
                                id: panInput
                                label: "PAN Number"
                                placeholderText: "ABCDE1234F"
                                Layout.fillWidth: true
                                onReturnPressed: aadhaarInput.focusInput = true
                                onLeftPressed: gstinInput.focusInput = true
                                onRightPressed: aadhaarInput.focusInput = true
                                onUpPressed: specialTypeInput.focusInput = true
                                onDownPressed: bankAccountInput.focusInput = true
                            }

                            CustomInput {
                                id: aadhaarInput
                                label: "Aadhaar Number"
                                placeholderText: "9988-7766-5544"
                                inputMethodHints: Qt.ImhDigitsOnly
                                Layout.fillWidth: true
                                onReturnPressed: bankNameInput.focusInput = true
                                onLeftPressed: panInput.focusInput = true
                                onRightPressed: stateCombo.focusAndOpen()
                                onUpPressed: specialTypeInput.focusInput = true
                                onDownPressed: ifscInput.focusInput = true
                            }
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 8

                            CustomInput {
                                id: bankNameInput
                                label: "Bank Name & Branch"
                                placeholderText: "HDFC Bank, SBI Raichur Branch"
                                Layout.fillWidth: true
                                onReturnPressed: bankAccountInput.focusInput = true
                                onRightPressed: bankAccountInput.focusInput = true
                                onLeftPressed: aadhaarInput.focusInput = true
                                onUpPressed: gstinInput.focusInput = true
                                onDownPressed: creditLimitInput.focusInput = true
                            }

                            CustomInput {
                                id: bankAccountInput
                                label: "Bank Account No."
                                placeholderText: "5010099887766"
                                Layout.preferredWidth: 160
                                onReturnPressed: ifscInput.focusInput = true
                                onLeftPressed: bankNameInput.focusInput = true
                                onRightPressed: ifscInput.focusInput = true
                                onUpPressed: panInput.focusInput = true
                                onDownPressed: creditLimitInput.focusInput = true
                            }

                            CustomInput {
                                id: ifscInput
                                label: "IFSC Code"
                                placeholderText: "HDFC0000123"
                                Layout.preferredWidth: 120
                                onReturnPressed: creditLimitInput.focusInput = true
                                onLeftPressed: bankAccountInput.focusInput = true
                                onRightPressed: pinInput.focusInput = true
                                onUpPressed: aadhaarInput.focusInput = true
                                onDownPressed: creditDaysInput.focusInput = true
                            }
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 8

                            CustomInput {
                                id: creditLimitInput
                                label: "Credit Limit (₹)"
                                placeholderText: "500000.00"
                                inputMethodHints: Qt.ImhFormattedNumbersOnly
                                Layout.fillWidth: true
                                onReturnPressed: creditDaysInput.focusInput = true
                                onRightPressed: creditDaysInput.focusInput = true
                                onLeftPressed: ifscInput.focusInput = true
                                onUpPressed: bankNameInput.focusInput = true
                                onDownPressed: mailingNameInput.focusInput = true
                            }

                            CustomInput {
                                id: creditDaysInput
                                label: "Credit Days"
                                placeholderText: "30"
                                inputMethodHints: Qt.ImhDigitsOnly
                                Layout.preferredWidth: 110
                                onReturnPressed: mailingNameInput.focusInput = true
                                onLeftPressed: creditLimitInput.focusInput = true
                                onRightPressed: mailingNameInput.focusInput = true
                                onUpPressed: ifscInput.focusInput = true
                                onDownPressed: mailingNameInput.focusInput = true
                            }
                        }
                    }
                }

                Item { Layout.fillHeight: true }
            }

            // ==============================================================
            // RIGHT COLUMN: MAILING ADDRESS & CONTACT DETAILS
            // ==============================================================
            ColumnLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.preferredWidth: 1
                spacing: 10

                // CARD 3: MAILING ADDRESS & CONTACT DETAILS
                Rectangle {
                    Layout.fillWidth: true
                    implicitHeight: secCol3.implicitHeight + 20
                    color: "#FFFFFF"
                    border.color: "#E2E8F0"
                    border.width: 1
                    radius: 8

                    ColumnLayout {
                        id: secCol3
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.top: parent.top
                        anchors.margins: 10
                        spacing: 8

                        Text {
                            text: "3. MAILING ADDRESS & CONTACT DETAILS"
                            color: "#16A34A"
                            font.pixelSize: 11
                            font.bold: true
                            font.letterSpacing: 0.8
                        }

                        CustomInput {
                            id: mailingNameInput
                            label: "Mailing / Billing Name"
                            placeholderText: "e.g. Sri Venkatesh Traders Pvt Ltd"
                            Layout.fillWidth: true
                            onReturnPressed: addressInput.focusInput = true
                            onLeftPressed: aliasInput.focusInput = true
                            onDownPressed: addressInput.focusInput = true
                            onUpPressed: creditDaysInput.focusInput = true
                        }

                        CustomInput {
                            id: addressInput
                            label: "Full Street Address"
                            placeholderText: "APMC Market Yard, Main Gate"
                            Layout.fillWidth: true
                            onReturnPressed: cityCombo.focusAndOpen()
                            onLeftPressed: opBalInput.focusInput = true
                            onUpPressed: mailingNameInput.focusInput = true
                            onDownPressed: cityCombo.focusAndOpen()
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 8

                            CustomWhiteCombo {
                                id: cityCombo
                                label: "City / Town"
                                Layout.fillWidth: true
                                editable: true
                                model: (typeof partiesModel !== "undefined" && partiesModel) ? partiesModel.get_cities() : []
                                onReturnPressed: districtCombo.focusAndOpen()
                                onRightPressed: districtCombo.focusAndOpen()
                                onLeftPressed: specialTypeInput.focusInput = true
                                onUpPressed: addressInput.focusInput = true
                                onDownPressed: stateCombo.focusAndOpen()
                            }

                            CustomWhiteCombo {
                                id: districtCombo
                                label: "District"
                                Layout.fillWidth: true
                                editable: true
                                model: (typeof partiesModel !== "undefined" && partiesModel) ? partiesModel.get_districts() : []
                                onReturnPressed: stateCombo.focusAndOpen()
                                onLeftPressed: cityCombo.focusAndOpen()
                                onUpPressed: addressInput.focusInput = true
                                onDownPressed: pinInput.focusInput = true
                            }
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 8

                            CustomWhiteCombo {
                                id: stateCombo
                                label: "Station / State"
                                Layout.fillWidth: true
                                editable: true
                                model: (typeof partiesModel !== "undefined" && partiesModel) ? partiesModel.get_stations() : []
                                onReturnPressed: pinInput.focusInput = true
                                onRightPressed: pinInput.focusInput = true
                                onLeftPressed: aadhaarInput.focusInput = true
                                onUpPressed: cityCombo.focusAndOpen()
                                onDownPressed: phoneInput.focusInput = true
                            }

                            CustomInput {
                                id: pinInput
                                label: "PIN Code"
                                placeholderText: "584101"
                                inputMethodHints: Qt.ImhDigitsOnly
                                Layout.preferredWidth: 120
                                onReturnPressed: phoneInput.focusInput = true
                                onLeftPressed: stateCombo.focusAndOpen()
                                onUpPressed: districtCombo.focusAndOpen()
                                onDownPressed: mobileInput.focusInput = true
                            }
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 8

                            CustomInput {
                                id: phoneInput
                                label: "Phone / Landline"
                                placeholderText: "08532-234567"
                                Layout.fillWidth: true
                                onReturnPressed: mobileInput.focusInput = true
                                onRightPressed: mobileInput.focusInput = true
                                onLeftPressed: bankNameInput.focusInput = true
                                onUpPressed: stateCombo.focusAndOpen()
                                onDownPressed: whatsappInput.focusInput = true
                            }

                            CustomInput {
                                id: mobileInput
                                label: "Mobile Number"
                                placeholderText: "9876543210"
                                inputMethodHints: Qt.ImhDigitsOnly
                                Layout.fillWidth: true
                                onReturnPressed: whatsappInput.focusInput = true
                                onLeftPressed: phoneInput.focusInput = true
                                onUpPressed: pinInput.focusInput = true
                                onDownPressed: emailInput.focusInput = true
                            }
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 8

                            CustomInput {
                                id: whatsappInput
                                label: "WhatsApp Number"
                                placeholderText: "9876543210"
                                inputMethodHints: Qt.ImhDigitsOnly
                                Layout.fillWidth: true
                                onReturnPressed: emailInput.focusInput = true
                                onRightPressed: emailInput.focusInput = true
                                onLeftPressed: bankAccountInput.focusInput = true
                                onUpPressed: phoneInput.focusInput = true
                                onDownPressed: contactPersonInput.focusInput = true
                            }

                            CustomInput {
                                id: emailInput
                                label: "Email Address"
                                placeholderText: "accounts@party.com"
                                Layout.fillWidth: true
                                onReturnPressed: contactPersonInput.focusInput = true
                                onLeftPressed: whatsappInput.focusInput = true
                                onUpPressed: mobileInput.focusInput = true
                                onDownPressed: contactPersonInput.focusInput = true
                            }
                        }

                        CustomInput {
                            id: contactPersonInput
                            label: "Contact Person"
                            placeholderText: "Mr. Venkatesh Rao"
                            Layout.fillWidth: true
                            onReturnPressed: root.saveLedger()
                            onLeftPressed: ifscInput.focusInput = true
                            onUpPressed: whatsappInput.focusInput = true
                            onDownPressed: submitBtn.focus = true
                        }
                    }
                }

                Item { Layout.fillHeight: true }
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
                background: Rectangle { color: (submitBtn.hovered || submitBtn.activeFocus) ? "#1D4ED8" : "#2563EB"; radius: 6; border.color: submitBtn.activeFocus ? "#93C5FD" : "transparent"; border.width: 2 }
                contentItem: RowLayout {
                    spacing: 6
                    Text { text: "💾 Save Complete Ledger Account"; color: "#FFFFFF"; font.bold: true; font.pixelSize: 14 }
                    KbdBadge { text: "Enter"; badgeColor: "#1E3A8A"; textColor: "#93C5FD"; borderColor: "#2563EB" }
                }
                Keys.onReturnPressed: root.saveLedger()
                Keys.onEnterPressed: root.saveLedger()
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
            ifscInput.text
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

    Component.onCompleted: {
        Qt.callLater(function() {
            nameInput.focusInput = true
        })
    }
}
