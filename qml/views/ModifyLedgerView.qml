import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../components"

ScrollView {
    id: root
    contentWidth: availableWidth
    clip: true

    signal cancelRequested()
    signal savedSuccess()

    property int selectedPartyId: -1

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
                    text: "✏️ Modify Existing Ledger Account"
                    color: "#0F172A"
                    font.pixelSize: 20
                    font.bold: true
                }
                Text {
                    text: "Search-ahead to load any party account, edit complete accounting, tax compliance, bank & credit term details."
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

        // SEARCH-AHEAD PARTY SELECTOR CARD
        Rectangle {
            Layout.fillWidth: true
            implicitHeight: selectCol.implicitHeight + 28
            color: "#EFF6FF"
            border.color: "#BFDBFE"
            border.width: 1
            radius: 10
            z: 100

            ColumnLayout {
                id: selectCol
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.top: parent.top
                anchors.margins: 14
                spacing: 10

                Text {
                    text: "SEARCH & SELECT PARTY ACCOUNT TO MODIFY"
                    color: "#2563EB"
                    font.pixelSize: 11
                    font.bold: true
                    font.letterSpacing: 1.0
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 14

                    AutoCompletePartySearch {
                        id: partySearchBox
                        Layout.fillWidth: true
                        onPartySelected: function(party) {
                            if (!party || !party.name) return
                            var p = (typeof partiesModel !== "undefined" && partiesModel) ? partiesModel.get_party_by_name(party.name) : {}
                            if (!p || !p.name) p = party
                            
                            root.selectedPartyId = p.id ? p.id : -1
                            nameInput.text = p.name ? p.name : ""
                            aliasInput.text = p.alias ? p.alias : ""
                            groupCombo.editText = p.group_name ? p.group_name : ""
                            partyTypeInput.text = p.party_type ? p.party_type : ""
                            specialTypeInput.text = p.special_type ? p.special_type : ""
                            opBalInput.text = (p.opening_balance !== undefined && p.opening_balance !== null) ? p.opening_balance.toString() : "0.00"
                            balTypeCombo.editText = p.balance_type ? p.balance_type : "Dr"
                            
                            mailingNameInput.text = p.mailing_name ? p.mailing_name : p.name
                            addressInput.text = p.address ? p.address : ""
                            cityCombo.editText = p.city ? p.city : ""
                            districtCombo.editText = p.district ? p.district : ""
                            stateCombo.editText = p.state ? p.state : ""
                            pinInput.text = p.pincode ? p.pincode : ""
                            
                            phoneInput.text = p.phone ? p.phone : ""
                            mobileInput.text = p.mobile ? p.mobile : ""
                            whatsappInput.text = p.whatsapp ? p.whatsapp : ""
                            emailInput.text = p.email ? p.email : ""
                            contactPersonInput.text = p.contact_person ? p.contact_person : ""
                            
                            gstinInput.text = p.gstin ? p.gstin : ""
                            panInput.text = p.pan ? p.pan : ""
                            aadhaarInput.text = p.aadhaar ? p.aadhaar : ""
                            
                            bankNameInput.text = p.bank_name ? p.bank_name : ""
                            bankAccountInput.text = p.bank_account ? p.bank_account : ""
                            ifscInput.text = p.ifsc_code ? p.ifsc_code : ""
                            creditLimitInput.text = (p.credit_limit !== undefined && p.credit_limit !== null) ? p.credit_limit.toString() : "0.00"
                            creditDaysInput.text = (p.credit_days !== undefined && p.credit_days !== null) ? p.credit_days.toString() : "30"
                        }
                    }
                }
            }
        }

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
                        text: ""
                        isRequired: true
                        Layout.fillWidth: true
                        onReturnPressed: aliasInput.focusInput = true
                    }

                    CustomInput {
                        id: aliasInput
                        label: "Short Alias / Code"
                        text: ""
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
                        text: ""
                        Layout.fillWidth: true
                        onReturnPressed: specialTypeInput.focusInput = true
                    }

                    CustomInput {
                        id: specialTypeInput
                        label: "Special Classification"
                        text: ""
                        Layout.fillWidth: true
                        onReturnPressed: opBalInput.focusInput = true
                    }

                    CustomInput {
                        id: opBalInput
                        label: "Opening Balance (₹)"
                        text: ""
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
                        text: ""
                        Layout.fillWidth: true
                        onReturnPressed: addressInput.focusInput = true
                    }

                    CustomInput {
                        id: addressInput
                        label: "Full Street Address"
                        text: ""
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
                        text: ""
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
                        text: ""
                        Layout.fillWidth: true
                        onReturnPressed: mobileInput.focusInput = true
                    }

                    CustomInput {
                        id: mobileInput
                        label: "Mobile Number"
                        text: ""
                        inputMethodHints: Qt.ImhDigitsOnly
                        Layout.fillWidth: true
                        onReturnPressed: whatsappInput.focusInput = true
                    }

                    CustomInput {
                        id: whatsappInput
                        label: "WhatsApp Number"
                        text: ""
                        inputMethodHints: Qt.ImhDigitsOnly
                        Layout.fillWidth: true
                        onReturnPressed: emailInput.focusInput = true
                    }

                    CustomInput {
                        id: emailInput
                        label: "Email Address"
                        text: ""
                        Layout.fillWidth: true
                        onReturnPressed: contactPersonInput.focusInput = true
                    }

                    CustomInput {
                        id: contactPersonInput
                        label: "Contact Person"
                        text: ""
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
                        text: ""
                        Layout.fillWidth: true
                        onReturnPressed: panInput.focusInput = true
                    }

                    CustomInput {
                        id: panInput
                        label: "PAN Number"
                        text: ""
                        Layout.fillWidth: true
                        onReturnPressed: aadhaarInput.focusInput = true
                    }

                    CustomInput {
                        id: aadhaarInput
                        label: "Aadhaar Number"
                        text: ""
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
                        text: ""
                        Layout.fillWidth: true
                        onReturnPressed: bankAccountInput.focusInput = true
                    }

                    CustomInput {
                        id: bankAccountInput
                        label: "Bank Account Number"
                        text: ""
                        Layout.fillWidth: true
                        onReturnPressed: ifscInput.focusInput = true
                    }

                    CustomInput {
                        id: ifscInput
                        label: "Bank IFSC Code"
                        text: ""
                        Layout.preferredWidth: 160
                        onReturnPressed: creditLimitInput.focusInput = true
                    }

                    CustomInput {
                        id: creditLimitInput
                        label: "Credit Limit (₹)"
                        text: ""
                        inputMethodHints: Qt.ImhFormattedNumbersOnly
                        Layout.fillWidth: true
                        onReturnPressed: creditDaysInput.focusInput = true
                    }

                    CustomInput {
                        id: creditDaysInput
                        label: "Credit Days"
                        text: ""
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
                background: Rectangle { color: submitBtn.hovered ? "#15803D" : "#16A34A"; radius: 6 }
                contentItem: RowLayout {
                    spacing: 6
                    Text { text: "💾 Update Complete Ledger Account"; color: "#FFFFFF"; font.bold: true; font.pixelSize: 14 }
                    KbdBadge { text: "Enter"; badgeColor: "#14532D"; textColor: "#86EFAC"; borderColor: "#16A34A" }
                }
                onClicked: {
                    if (root.selectedPartyId > 0) {
                        partiesModel.update_ledger_full(
                            root.selectedPartyId,
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
                            parseInt(creditDaysInput.text) || 30
                        )
                    }
                    partiesModel.reload_data()
                    root.savedSuccess()
                }
            }
        }
    }
}
