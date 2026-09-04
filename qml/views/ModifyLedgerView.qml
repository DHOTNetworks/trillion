import QtQuick
import QtQuick.Templates as T
import QtQuick.Layouts
import MahadevERP

Item {
    id: root
    anchors.fill: parent

    signal cancelRequested()
    signal savedSuccess()

    property int selectedPartyId: -1

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
                    text: "✏️ Modify Existing Ledger Account"
                    color: "#0F172A"
                    font.pixelSize: 18
                    font.bold: true
                }
                Text {
                    text: "Single-slate ledger search-ahead, accounting, tax compliance, bank & credit term editor"
                    color: "#64748B"
                    font.pixelSize: 11
                }
            }

            Item { Layout.fillWidth: true }

            T.Button {
                id: backBtn
                implicitWidth: contentItem.implicitWidth + 24
                implicitHeight: 32
                background: Rectangle { color: backBtn.hovered ? "#E2E8F0" : "#F1F5F9"; radius: 6; border.color: "#CBD5E1" }
                contentItem: RowLayout {
                    spacing: 6
                    anchors.centerIn: parent
                    Text { text: "← Back to Dashboard"; color: "#475569"; font.pixelSize: 12; font.bold: true }
                    KbdBadge { text: "Esc"; badgeColor: "#DC2626"; textColor: "#FFF"; borderColor: "#B91C1C" }
                }
                onClicked: root.cancelRequested()
            }
        }

        Rectangle { Layout.fillWidth: true; height: 1; color: "#E2E8F0" }

        // SEARCH-AHEAD PARTY SELECTOR CARD WITH CustomWhiteCombo
        Rectangle {
            Layout.fillWidth: true
            implicitHeight: selectCol.implicitHeight + 16
            color: "#EFF6FF"
            border.color: "#BFDBFE"
            border.width: 1
            radius: 8
            z: 100

            ColumnLayout {
                id: selectCol
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.top: parent.top
                anchors.margins: 10
                spacing: 6

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 12

                    CustomWhiteCombo {
                        id: partySearchCombo
                        label: "🔍 SEARCH & SELECT PARTY ACCOUNT TO MODIFY (Type Name, ↑/↓ Arrows & Enter) *"
                        Layout.fillWidth: true
                        focusInput: true
                        model: (typeof partiesModel !== "undefined" && partiesModel) ? partiesModel.get_party_list() : []
                        onReturnPressed: root.loadSelectedParty(partySearchCombo.currentText)
                        onDownPressed: nameInput.focusInput = true
                    }
                }
            }
        }

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
                                text: ""
                                isRequired: true
                                Layout.fillWidth: true
                                onReturnPressed: aliasInput.focusInput = true
                                onRightPressed: aliasInput.focusInput = true
                                onUpPressed: partySearchCombo.focusAndOpen()
                                onDownPressed: groupCombo.focusAndOpen()
                            }

                            CustomInput {
                                id: aliasInput
                                label: "Short Alias / Code"
                                text: ""
                                Layout.preferredWidth: 150
                                onReturnPressed: groupCombo.focusAndOpen()
                                onLeftPressed: nameInput.focusInput = true
                                onRightPressed: mailingNameInput.focusInput = true
                                onUpPressed: partySearchCombo.focusAndOpen()
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
                                text: ""
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
                                text: ""
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
                                text: ""
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
                                text: ""
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
                                text: ""
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
                                text: ""
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
                                text: ""
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
                                text: ""
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
                                text: ""
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
                                text: ""
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
                                text: ""
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
                            text: ""
                            Layout.fillWidth: true
                            onReturnPressed: addressInput.focusInput = true
                            onLeftPressed: aliasInput.focusInput = true
                            onDownPressed: addressInput.focusInput = true
                            onUpPressed: creditDaysInput.focusInput = true
                        }

                        CustomInput {
                            id: addressInput
                            label: "Full Street Address"
                            text: ""
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
                                text: ""
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
                                text: ""
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
                                text: ""
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
                                text: ""
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
                                text: ""
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
                            text: ""
                            Layout.fillWidth: true
                            onReturnPressed: root.saveUpdateLedger()
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

            T.Button {
                id: cancelBottomBtn
                implicitWidth: 110
                implicitHeight: 38
                background: Rectangle { color: cancelBottomBtn.hovered ? "#E2E8F0" : "#F1F5F9"; radius: 6; border.color: "#CBD5E1" }
                contentItem: RowLayout {
                    spacing: 6
                    anchors.centerIn: parent
                    Text { text: "Cancel"; color: "#475569"; font.pixelSize: 13; font.bold: true }
                    KbdBadge { text: "Esc"; badgeColor: "#DC2626"; textColor: "#FFF"; borderColor: "#B91C1C" }
                }
                onClicked: root.cancelRequested()
            }

            Item { Layout.fillWidth: true }

            T.Button {
                id: submitBtn
                implicitWidth: 300
                implicitHeight: 38
                background: Rectangle { color: (submitBtn.hovered || submitBtn.activeFocus) ? "#15803D" : "#16A34A"; radius: 6; border.color: submitBtn.activeFocus ? "#86EFAC" : "transparent"; border.width: 2 }
                contentItem: RowLayout {
                    spacing: 8
                    anchors.centerIn: parent
                    Text { text: "💾 Update Complete Ledger Account"; color: "#FFFFFF"; font.bold: true; font.pixelSize: 14 }
                    KbdBadge { text: "Enter"; badgeColor: "#14532D"; textColor: "#86EFAC"; borderColor: "#16A34A" }
                }
                Keys.onReturnPressed: root.saveUpdateLedger()
                Keys.onEnterPressed: root.saveUpdateLedger()
                onClicked: root.saveUpdateLedger()
            }
        }
    }

    function loadSelectedParty(name) {
        if (!name || !name.trim()) return
        var cleanName = name.trim()
        var p = (typeof partiesModel !== "undefined" && partiesModel) ? partiesModel.get_party_by_name(cleanName) : {}
        if (!p || !p.name) return

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

        nameInput.focusInput = true
    }

    function saveUpdateLedger() {
        if (!nameInput.text.trim()) {
            return
        }
        updateConfirmModal.open()
    }

    function executeUpdateLedger() {
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
                parseInt(creditDaysInput.text) || 30,
                bankNameInput.text,
                bankAccountInput.text,
                ifscInput.text
            )
        }
        partiesModel.reload_data()
        root.savedSuccess()
    }

    ConfirmationModal {
        id: updateConfirmModal
        anchors.centerIn: parent
        titleText: "CONFIRM LEDGER UPDATE"
        messageText: "Are you sure you want to update Ledger Account '" + nameInput.text.trim() + "' under group '" + groupCombo.currentText + "'?"
        onConfirmed: root.executeUpdateLedger()
    }

    Component.onCompleted: {
        Qt.callLater(function() {
            partySearchCombo.focusAndOpen()
        })
    }
}
