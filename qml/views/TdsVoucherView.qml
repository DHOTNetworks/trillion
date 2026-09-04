import QtQuick
import QtQuick.Templates as T
import QtQuick.Layouts
import MahadevERP

Rectangle {
    id: root
    anchors.fill: parent
    color: "#F4F6F9"
    focus: true

    signal cancelRequested()
    signal voucherSaved()

    // -------------------------------------------------------------
    // PROPERTIES & STATE
    // -------------------------------------------------------------
    property int currentVoucherNo: 1
    property string voucherDate: ""
    property string dayOfWeek: ""
    property bool postInBooks: true
    property string currentTdsType: "RENT"
    property int selectedPartyId: 0
    property string selectedPartyName: ""
    property string partyBalanceText: "Date Bal. 0.00"
    property string partyPan: ""

    property double incomeAmount: 0.0
    property double previousAmount: 0.0
    property double totalForTds: 0.0

    property double tdsRate: 10.0
    property double tdsTaxAmount: 0.0
    property double surchargeRate: 0.0
    property double surchargeTaxAmount: 0.0
    property double cessRate: 0.0
    property double cessTaxAmount: 0.0
    property bool useRoundedTotal: true
    property double totalTaxRate: 10.0
    property double totalTaxAmount: 0.0
    property double netAmount: 0.0

    property string statusMessage: ""
    property bool isError: false

    // -------------------------------------------------------------
    // INITIALIZATION & CALCULATIONS
    // -------------------------------------------------------------
    Component.onCompleted: {
        Qt.callLater(function() {
            initializeVoucher()
            if (incomeInput) incomeInput.forceActiveFocus()
        })
    }

    function initializeVoucher() {
        if (typeof tdsModel !== "undefined" && tdsModel) {
            var info = tdsModel.get_next_voucher_info(currentTdsType)
            currentVoucherNo = info.next_voucher_no || 1
            if (vchNoInput) vchNoInput.text = currentVoucherNo.toString()
            voucherDate = info.date_display || ""
            if (vchDateInput) vchDateInput.text = voucherDate
            dayOfWeek = info.day_name || ""
            if (typeCombo) typeCombo.editText = currentTdsType

            tdsRate = info.default_tds_rate !== undefined ? info.default_tds_rate : 10.0
            if (tdsRateInput) tdsRateInput.text = tdsRate.toFixed(2)
            surchargeRate = info.default_surcharge_rate !== undefined ? info.default_surcharge_rate : 0.0
            if (surchargeRateInput) surchargeRateInput.text = surchargeRate.toFixed(2)
            cessRate = info.default_cess_rate !== undefined ? info.default_cess_rate : 0.0
            if (cessRateInput) cessRateInput.text = cessRate.toFixed(2)

            if (info.default_exp_ledger_name && expLedgerCombo && expLedgerCombo.currentText === "") {
                expLedgerCombo.editText = info.default_exp_ledger_name
            }
            if (info.default_tds_ledger_name && tdsLedgerCombo && tdsLedgerCombo.currentText === "") {
                tdsLedgerCombo.editText = info.default_tds_ledger_name
            }
        }
        if (deducteeCombo) updatePartyInfo(deducteeCombo.currentText)
        recalculateTotals()
    }

    function onTdsTypeChanged(newType) {
        currentTdsType = newType
        if (typeof tdsModel !== "undefined" && tdsModel) {
            var info = tdsModel.get_next_voucher_info(currentTdsType)
            tdsRate = info.default_tds_rate !== undefined ? info.default_tds_rate : 10.0
            if (tdsRateInput) tdsRateInput.text = tdsRate.toFixed(2)

            if (info.default_exp_ledger_name && expLedgerCombo) {
                expLedgerCombo.editText = info.default_exp_ledger_name
            }
            if (info.default_tds_ledger_name && tdsLedgerCombo) {
                tdsLedgerCombo.editText = info.default_tds_ledger_name
            }
        }
        if (deducteeCombo) updatePartyInfo(deducteeCombo.currentText)
        recalculateTotals()
    }

    function updatePartyInfo(partyName) {
        selectedPartyName = (partyName || "").trim()
        if (typeof partiesModel !== "undefined" && partiesModel && typeof tdsModel !== "undefined" && tdsModel) {
            var party = partiesModel.get_party_by_name(selectedPartyName)
            var pId = (party && party.id) ? party.id : 0
            selectedPartyId = pId
            var info = tdsModel.get_party_info(pId, currentTdsType)
            partyBalanceText = info.formatted_balance || "Date Bal. 0.00"
            partyPan = (party && party.pan) ? party.pan : (info.pan_no || "")
            previousAmount = info.previous_amount || 0.0
            if (prevAmtInput) {
                prevAmtInput.text = previousAmount > 0.001 ? previousAmount.toFixed(2) : "0.00"
            }

            if (info.last_narration && narrationInput && narrationInput.text.trim() === "") {
                narrationInput.text = info.last_narration
            }
        }
        recalculateTotals()
    }

    function fetchLastNarration() {
        if (typeof tdsModel !== "undefined" && tdsModel) {
            var narr = tdsModel.get_last_narration(selectedPartyId, currentTdsType)
            if (narr && narrationInput) {
                narrationInput.text = narr
            }
        }
    }

    function recalculateTotals() {
        var inc = (incomeInput && incomeInput.text) ? (parseFloat(incomeInput.text) || 0.0) : incomeAmount
        var prev = (prevAmtInput && prevAmtInput.text) ? (parseFloat(prevAmtInput.text) || 0.0) : previousAmount
        incomeAmount = inc
        previousAmount = prev
        totalForTds = inc + prev

        var rTds = (tdsRateInput && tdsRateInput.text) ? (parseFloat(tdsRateInput.text) || 0.0) : tdsRate
        var rSur = (surchargeRateInput && surchargeRateInput.text) ? (parseFloat(surchargeRateInput.text) || 0.0) : surchargeRate
        var rCess = (cessRateInput && cessRateInput.text) ? (parseFloat(cessRateInput.text) || 0.0) : cessRate

        tdsRate = rTds
        surchargeRate = rSur
        cessRate = rCess

        // Calculate tax on Total For TDS, or on current Income Amount
        // Standard Indian TDS calculation: Rate applied to Income Amount
        // (Previous amount establishes whether threshold limit is crossed)
        tdsTaxAmount = (inc * rTds) / 100.0
        surchargeTaxAmount = (tdsTaxAmount * rSur) / 100.0
        cessTaxAmount = ((tdsTaxAmount + surchargeTaxAmount) * rCess) / 100.0

        totalTaxRate = rTds + rSur + rCess
        var rawTotalTax = tdsTaxAmount + surchargeTaxAmount + cessTaxAmount

        if (useRoundedTotal) {
            totalTaxAmount = Math.round(rawTotalTax)
        } else {
            totalTaxAmount = Math.round(rawTotalTax * 100.0) / 100.0
        }

        netAmount = inc - totalTaxAmount
        if (netAmount < 0.0) netAmount = 0.0
    }

    function saveVoucher() {
        if (!selectedPartyName) {
            statusMessage = "⚠️ Please select a Deductee / Party Ledger!"
            isError = true
            deducteeCombo.focusAndOpen()
            return
        }
        if (incomeAmount <= 0.001) {
            statusMessage = "⚠️ Please enter a valid Income Amount greater than 0!"
            isError = true
            incomeInput.forceActiveFocus()
            return
        }

        confirmModal.titleText = "CONFIRM TDS VOUCHER SAVE"
        confirmModal.messageText = "Save & Post TDS Voucher #" + currentVoucherNo + "\n" +
                                   "Party: " + selectedPartyName + "\n" +
                                   "Type: " + currentTdsType + " | Income: ₹" + incomeAmount.toFixed(2) + "\n" +
                                   "Total Tax: ₹" + totalTaxAmount.toFixed(2) + " | Net: ₹" + netAmount.toFixed(2) + "?"
        confirmModal.open()
    }

    function executeSave() {
        var vchNo = parseInt(vchNoInput.text) || currentVoucherNo
        var vDate = vchDateInput.text.trim()

        var expId = 0
        var expName = expLedgerCombo.currentText.trim()
        if (typeof partiesModel !== "undefined" && partiesModel && expName) {
            var expParty = partiesModel.get_party_by_name(expName)
            if (expParty && expParty.id) expId = expParty.id
        }

        var tdsLedgerId = 0
        var tdsLedgerName = tdsLedgerCombo.currentText.trim()
        if (typeof partiesModel !== "undefined" && partiesModel && tdsLedgerName) {
            var tdsParty = partiesModel.get_party_by_name(tdsLedgerName)
            if (tdsParty && tdsParty.id) tdsLedgerId = tdsParty.id
        }

        var payload = {
            "voucher_no": vchNo,
            "voucher_date": vDate,
            "day_of_week": dayOfWeek,
            "post_in_books": postInBooks ? 1 : 0,
            "tds_type": currentTdsType,
            "ledger_id": selectedPartyId,
            "ledger_name": selectedPartyName,
            "income_amount": incomeAmount,
            "previous_amount": previousAmount,
            "total_for_tds": totalForTds,
            "narration": narrationInput.text.trim(),
            "rate_tds": tdsRate,
            "tax_amount_tds": tdsTaxAmount,
            "rate_surcharge": surchargeRate,
            "tax_amount_surcharge": surchargeTaxAmount,
            "rate_cess": cessRate,
            "tax_amount_cess": cessTaxAmount,
            "use_rounded_total": useRoundedTotal ? 1 : 0,
            "total_tax_rate": totalTaxRate,
            "total_tax_amount": totalTaxAmount,
            "net_amount": netAmount,
            "non_deduction_reason": nonDeductionInput.text.trim(),
            "exp_ledger_id": expId,
            "exp_ledger_name": expName,
            "tds_ledger_id": tdsLedgerId,
            "tds_ledger_name": tdsLedgerName
        }

        if (typeof tdsModel !== "undefined" && tdsModel) {
            var ok = tdsModel.save_tds_voucher(payload)
            if (ok) {
                statusMessage = "✅ TDS Voucher #" + vchNo + " saved & posted successfully!"
                isError = false
                root.voucherSaved()
            } else {
                statusMessage = "❌ Failed to save TDS Voucher. Please check inputs."
                isError = true
            }
        }
    }

    // -------------------------------------------------------------
    // KEYBOARD NAVIGATION
    // -------------------------------------------------------------
    Keys.onEscapePressed: function(event) {
        event.accepted = true
        if (confirmModal.opened) {
            confirmModal.close()
        } else if (newLedgerPopup.opened) {
            newLedgerPopup.close()
        } else {
            root.cancelRequested()
        }
    }

    Shortcut {
        sequence: "Alt+Z"
        onActivated: fetchLastNarration()
    }

    Shortcut {
        sequence: "Alt+C"
        onActivated: newLedgerPopup.open()
    }

    // -------------------------------------------------------------
    // UI LAYOUT
    // -------------------------------------------------------------
    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 14
        spacing: 10

        // =========================================================
        // TOP CONTROL / NAVIGATION BAR
        // =========================================================
        RowLayout {
            Layout.fillWidth: true
            spacing: 10

            T.Button {
                id: backBtn
                height: 32
                background: Rectangle {
                    color: backBtn.hovered ? "#E2E8F0" : "#FFFFFF"
                    border.color: "#CBD5E1"
                    radius: 6
                }
                contentItem: RowLayout {
                    spacing: 4
                    Text { text: "← Back (Esc)"; color: "#0F172A"; font.bold: true; font.pixelSize: 12 }
                }
                onClicked: root.cancelRequested()
            }

            ColumnLayout {
                spacing: 1
                Text {
                    text: "TDS VOUCHER (CREATION)"
                    font.pixelSize: 15
                    font.bold: true
                    color: "#7C3AED"
                    font.letterSpacing: 0.5
                }
                Text {
                    text: "Tax Deducted at Source Assessment & Double-Entry Journal Posting"
                    font.pixelSize: 10
                    color: "#64748B"
                }
            }

            Item { Layout.fillWidth: true }

            // Shortcuts Guide Badges
            RowLayout {
                spacing: 6
                KbdBadge { text: "Alt+Z: Last Narr." }
                KbdBadge { text: "Alt+C: New Ledger" }
                KbdBadge { text: "F2: Save Voucher" }
            }

            T.Button {
                id: saveTopBtn
                height: 32
                background: Rectangle {
                    color: saveTopBtn.hovered ? "#6D28D9" : "#7C3AED"
                    radius: 6
                }
                contentItem: RowLayout {
                    spacing: 6
                    Text { text: "💾 Save Voucher (F2)"; color: "#FFFFFF"; font.bold: true; font.pixelSize: 12 }
                }
                onClicked: root.saveVoucher()
            }
        }

        // =========================================================
        // MAIN SINGLE SLATE CARD
        // =========================================================
        Rectangle {
            id: mainSlate
            Layout.fillWidth: true
            Layout.fillHeight: true
            color: "#FFFFFF"
            radius: 10
            border.color: "#CBD5E1"
            border.width: 1

            T.ScrollView {
                anchors.fill: parent
                anchors.margins: 14
                clip: true
                contentWidth: availableWidth

                ColumnLayout {
                    width: parent.width
                    spacing: 12

                    // -------------------------------------------------
                    // HEADER STRIP: VOUCHER NO, DATE, DAY, TYPE, POST CHECKBOX
                    // -------------------------------------------------
                    Rectangle {
                        Layout.fillWidth: true
                        height: 48
                        color: "#F8FAFC"
                        radius: 8
                        border.color: "#E2E8F0"

                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: 10
                            anchors.rightMargin: 10
                            spacing: 12

                            // Crimson Badge: Voucher No.
                            Rectangle {
                                width: 100
                                height: 28
                                color: "#800000"
                                radius: 4
                                Text {
                                    anchors.centerIn: parent
                                    text: "Voucher  No."
                                    color: "#FFFFFF"
                                    font.bold: true
                                    font.pixelSize: 12
                                }
                            }

                            CustomInput {
                                id: vchNoInput
                                text: root.currentVoucherNo.toString()
                                Layout.preferredWidth: 70
                                horizontalAlignment: TextInput.AlignHCenter
                                font.bold: true
                                onReturnPressed: vchDateInput.focusInput = true
                            }

                            Rectangle { width: 1; height: 26; color: "#CBD5E1" }

                            // Voucher Date & Day of Week
                            Text {
                                text: "Voucher Dt. :"
                                color: "#0F172A"
                                font.bold: true
                                font.pixelSize: 12
                            }

                            CustomInput {
                                id: vchDateInput
                                text: root.voucherDate
                                Layout.preferredWidth: 105
                                placeholderText: "DD/MM/YYYY"
                                onReturnPressed: typeCombo.focusAndOpen()
                                onEditingFinished: {
                                    root.voucherDate = text.trim()
                                    // Update day of week if valid
                                    var parts = text.split("/")
                                    if (parts.length === 3) {
                                        var d = new Date(parseInt(parts[2]), parseInt(parts[1]) - 1, parseInt(parts[0]))
                                        var days = ["Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"]
                                        root.dayOfWeek = days[d.getDay()]
                                    }
                                }
                            }

                            Text {
                                text: root.dayOfWeek
                                color: "#64748B"
                                font.bold: true
                                font.pixelSize: 12
                            }

                            Rectangle { width: 1; height: 26; color: "#CBD5E1" }

                            // TDS Type Dropdown
                            Text {
                                text: "Type :"
                                color: "#0F172A"
                                font.bold: true
                                font.pixelSize: 12
                            }

                            CustomWhiteCombo {
                                id: typeCombo
                                Layout.preferredWidth: 160
                                model: [
                                    "RENT",
                                    "FREIGHT",
                                    "LABOUR",
                                    "INTEREST",
                                    "BROKERAGE",
                                    "DAMI",
                                    "COMMISSION",
                                    "CONTRACTOR",
                                    "PROFESSIONAL",
                                    "OTHER"
                                ]
                                onCurrentTextChanged: root.onTdsTypeChanged(currentText)
                                onReturnPressed: deducteeCombo.focusAndOpen()
                            }

                            Item { Layout.fillWidth: true }

                            // Transaction Post In Books Checkbox
                            RowLayout {
                                spacing: 6
                                CustomCheckBox {
                                    id: postCheck
                                    checked: root.postInBooks
                                    onCheckedChanged: root.postInBooks = checked
                                }
                                Text {
                                    text: "Transaction Post In Books"
                                    color: "#0F172A"
                                    font.bold: true
                                    font.pixelSize: 12
                                    MouseArea {
                                        anchors.fill: parent
                                        onClicked: postCheck.checked = !postCheck.checked
                                    }
                                }
                            }
                        }
                    }

                    // -------------------------------------------------
                    // DEDUCTEE / PARTY LEDGER SELECTION ROW
                    // -------------------------------------------------
                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 10

                        Text {
                            text: "Ledger Name :"
                            color: "#B91C1C"
                            font.bold: true
                            font.pixelSize: 13
                        }

                        CustomWhiteCombo {
                            id: deducteeCombo
                            Layout.fillWidth: true
                            model: (typeof partiesModel !== "undefined" && partiesModel) ? partiesModel.get_parties_list() : []
                            onCurrentTextChanged: root.updatePartyInfo(currentText)
                            onReturnPressed: incomeInput.forceActiveFocus()
                        }

                        // Live Date Balance Badge
                        Rectangle {
                            height: 30
                            radius: 5
                            color: root.partyBalanceText.indexOf("Dr") !== -1 ? "#FEF2F2" : "#F0FDF4"
                            border.color: root.partyBalanceText.indexOf("Dr") !== -1 ? "#FCA5A5" : "#86EFAC"
                            Layout.preferredWidth: balText.implicitWidth + 16
                            Text {
                                id: balText
                                anchors.centerIn: parent
                                text: root.partyBalanceText
                                color: root.partyBalanceText.indexOf("Dr") !== -1 ? "#B91C1C" : "#15803D"
                                font.bold: true
                                font.pixelSize: 11
                            }
                        }

                        // PAN Badge if available
                        Rectangle {
                            visible: root.partyPan.length > 0
                            height: 30
                            radius: 5
                            color: "#EFF6FF"
                            border.color: "#93C5FD"
                            Layout.preferredWidth: panText.implicitWidth + 16
                            Text {
                                id: panText
                                anchors.centerIn: parent
                                text: "PAN: " + root.partyPan
                                color: "#1D4ED8"
                                font.bold: true
                                font.pixelSize: 11
                            }
                        }
                    }

                    Text {
                        text: "(Alt+C : Create New Ledger  -  Alt+M : Ledger Alteration)"
                        color: "#64748B"
                        font.pixelSize: 10
                    }

                    // -------------------------------------------------
                    // COMPUTATION SLATE (LIGHT CHAMPAGNE / PEACH CARD)
                    // -------------------------------------------------
                    Rectangle {
                        Layout.fillWidth: true
                        radius: 8
                        color: "#FEF9EE"
                        border.color: "#FDE68A"
                        border.width: 1
                        implicitHeight: compRow.implicitHeight + 24

                        RowLayout {
                            id: compRow
                            anchors.fill: parent
                            anchors.margins: 12
                            spacing: 20

                            // ---------------------------------------------
                            // LEFT COLUMN: INCOME, PREVIOUS, TOTAL, NARRATION
                            // ---------------------------------------------
                            ColumnLayout {
                                Layout.fillWidth: true
                                Layout.preferredWidth: 420
                                spacing: 10

                                // Income + Previous = Total for TDS
                                RowLayout {
                                    spacing: 8

                                    ColumnLayout {
                                        spacing: 2
                                        Text { text: "Income Amount :"; color: "#0F172A"; font.bold: true; font.pixelSize: 12 }
                                        CustomInput {
                                            id: incomeInput
                                            placeholderText: "0.00"
                                            Layout.preferredWidth: 110
                                            horizontalAlignment: TextInput.AlignRight
                                            font.bold: true
                                            onTextChanged: root.recalculateTotals()
                                            onReturnPressed: prevAmtInput.focusInput = true
                                        }
                                    }

                                    Text { text: "+"; color: "#64748B"; font.bold: true; font.pixelSize: 14; Layout.alignment: Qt.AlignVCenter }

                                    ColumnLayout {
                                        spacing: 2
                                        Text { text: "Previous Amount"; color: "#64748B"; font.pixelSize: 11; font.bold: true }
                                        CustomInput {
                                            id: prevAmtInput
                                            text: "0.00"
                                            placeholderText: "0.00"
                                            Layout.preferredWidth: 110
                                            horizontalAlignment: TextInput.AlignRight
                                            onTextChanged: root.recalculateTotals()
                                            onReturnPressed: narrationInput.focusInput = true
                                        }
                                    }

                                    Text { text: "="; color: "#64748B"; font.bold: true; font.pixelSize: 14; Layout.alignment: Qt.AlignVCenter }

                                    ColumnLayout {
                                        spacing: 2
                                        Text { text: "Total For TDS"; color: "#0F172A"; font.pixelSize: 11; font.bold: true }
                                        Rectangle {
                                            width: 110; height: 30; radius: 5
                                            color: "#FFFFFF"; border.color: "#CBD5E1"
                                            Text {
                                                anchors.centerIn: parent
                                                text: root.totalForTds.toFixed(2)
                                                font.bold: true
                                                font.pixelSize: 12
                                                color: "#0F172A"
                                            }
                                        }
                                    }
                                }

                                // Narration For Credit Income
                                ColumnLayout {
                                    Layout.fillWidth: true
                                    spacing: 4

                                    RowLayout {
                                        Text {
                                            text: "Narration For Credit Income :-"
                                            color: "#991B1B"
                                            font.bold: true
                                            font.pixelSize: 11
                                        }
                                        Item { Layout.fillWidth: true }
                                        T.Button {
                                            id: lastNarrBtn
                                            height: 20
                                            background: Rectangle {
                                                color: lastNarrBtn.hovered ? "#DCFCE7" : "#F0FDF4"
                                                border.color: "#86EFAC"
                                                radius: 4
                                            }
                                            contentItem: Text {
                                                text: "(Alt+Z : Get Last Narration)"
                                                color: "#15803D"
                                                font.pixelSize: 9
                                                font.bold: true
                                            }
                                            onClicked: root.fetchLastNarration()
                                        }
                                    }

                                    CustomInput {
                                        id: narrationInput
                                        Layout.fillWidth: true
                                        placeholderText: "Enter narration for income credit..."
                                        onReturnPressed: tdsRateInput.focusInput = true
                                    }
                                }
                            }

                            // Vertical divider
                            Rectangle { width: 1; Layout.fillHeight: true; color: "#FDE68A" }

                            // ---------------------------------------------
                            // RIGHT COLUMN: TAX DEDUCTION TABLE
                            // ---------------------------------------------
                            ColumnLayout {
                                Layout.fillWidth: true
                                Layout.preferredWidth: 380
                                spacing: 6

                                // Table Header
                                RowLayout {
                                    Layout.fillWidth: true
                                    Text { text: "Tax Component"; color: "#475569"; font.pixelSize: 11; font.bold: true; Layout.preferredWidth: 120 }
                                    Text { text: "Rate (%)"; color: "#475569"; font.pixelSize: 11; font.bold: true; Layout.preferredWidth: 90; horizontalAlignment: Text.AlignHCenter }
                                    Text { text: "Tax Amount (₹)"; color: "#475569"; font.pixelSize: 11; font.bold: true; Layout.fillWidth: true; horizontalAlignment: Text.AlignRight }
                                }

                                Rectangle { Layout.fillWidth: true; height: 1; color: "#E2E8F0" }

                                // TDS Row
                                RowLayout {
                                    Layout.fillWidth: true
                                    Text { text: "TDS :"; color: "#0F172A"; font.bold: true; font.pixelSize: 12; Layout.preferredWidth: 120 }
                                    CustomInput {
                                        id: tdsRateInput
                                        text: root.tdsRate.toFixed(2)
                                        Layout.preferredWidth: 90
                                        horizontalAlignment: TextInput.AlignHCenter
                                        onTextChanged: root.recalculateTotals()
                                        onReturnPressed: surchargeRateInput.focusInput = true
                                    }
                                    Text {
                                        text: root.tdsTaxAmount.toFixed(2)
                                        color: "#0F172A"
                                        font.pixelSize: 12
                                        font.bold: true
                                        Layout.fillWidth: true
                                        horizontalAlignment: Text.AlignRight
                                    }
                                }

                                // Surcharge Row
                                RowLayout {
                                    Layout.fillWidth: true
                                    Text { text: "Surcharge :"; color: "#475569"; font.pixelSize: 11; Layout.preferredWidth: 120 }
                                    CustomInput {
                                        id: surchargeRateInput
                                        text: "0.00"
                                        Layout.preferredWidth: 90
                                        horizontalAlignment: TextInput.AlignHCenter
                                        onTextChanged: root.recalculateTotals()
                                        onReturnPressed: cessRateInput.focusInput = true
                                    }
                                    Text {
                                        text: root.surchargeTaxAmount.toFixed(2)
                                        color: "#475569"
                                        font.pixelSize: 11
                                        Layout.fillWidth: true
                                        horizontalAlignment: Text.AlignRight
                                    }
                                }

                                // Education (Cess) Row
                                RowLayout {
                                    Layout.fillWidth: true
                                    Text { text: "Education (Cess) :"; color: "#475569"; font.pixelSize: 11; Layout.preferredWidth: 120 }
                                    CustomInput {
                                        id: cessRateInput
                                        text: "0.00"
                                        Layout.preferredWidth: 90
                                        horizontalAlignment: TextInput.AlignHCenter
                                        onTextChanged: root.recalculateTotals()
                                        onReturnPressed: nonDeductionInput.focusInput = true
                                    }
                                    Text {
                                        text: root.cessTaxAmount.toFixed(2)
                                        color: "#475569"
                                        font.pixelSize: 11
                                        Layout.fillWidth: true
                                        horizontalAlignment: Text.AlignRight
                                    }
                                }

                                Rectangle { Layout.fillWidth: true; height: 1; color: "#CBD5E1" }

                                // Total TAX & Rounded Total Checkbox
                                RowLayout {
                                    Layout.fillWidth: true
                                    CustomCheckBox {
                                        id: roundCheck
                                        checked: root.useRoundedTotal
                                        onCheckedChanged: {
                                            root.useRoundedTotal = checked
                                            root.recalculateTotals()
                                        }
                                    }
                                    Text {
                                        text: "Use Rounded Total"
                                        color: "#475569"
                                        font.pixelSize: 10
                                        MouseArea {
                                            anchors.fill: parent
                                            onClicked: roundCheck.checked = !roundCheck.checked
                                        }
                                    }

                                    Item { Layout.fillWidth: true }

                                    Text {
                                        text: "Total TAX :"
                                        color: "#0F172A"
                                        font.bold: true
                                        font.pixelSize: 12
                                    }

                                    Rectangle {
                                        width: 70; height: 26; radius: 4
                                        color: "#FFFFFF"; border.color: "#CBD5E1"
                                        Text {
                                            anchors.centerIn: parent
                                            text: root.totalTaxRate.toFixed(2) + "%"
                                            font.bold: true
                                            font.pixelSize: 11
                                            color: "#0F172A"
                                        }
                                    }

                                    Text {
                                        text: root.totalTaxAmount.toFixed(2)
                                        color: "#0F172A"
                                        font.bold: true
                                        font.pixelSize: 13
                                        Layout.preferredWidth: 80
                                        horizontalAlignment: Text.AlignRight
                                    }
                                }

                                // Net Amount (Bold Red)
                                RowLayout {
                                    Layout.fillWidth: true
                                    Item { Layout.fillWidth: true }
                                    Text {
                                        text: "Net Amount :"
                                        color: "#DC2626"
                                        font.bold: true
                                        font.pixelSize: 13
                                    }
                                    Rectangle {
                                        height: 30
                                        radius: 5
                                        color: "#FEF2F2"
                                        border.color: "#FCA5A5"
                                        Layout.preferredWidth: 120
                                        Text {
                                            anchors.centerIn: parent
                                            text: "₹ " + root.netAmount.toFixed(2)
                                            color: "#B91C1C"
                                            font.bold: true
                                            font.pixelSize: 13
                                        }
                                    }
                                }
                            }
                        }
                    }

                    // -------------------------------------------------
                    // NON-DEDUCTION REASON SECTION
                    // -------------------------------------------------
                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 2

                        Text {
                            text: "Reason For Non Deduction :"
                            color: "#0F172A"
                            font.bold: true
                            font.pixelSize: 11
                        }

                        CustomInput {
                            id: nonDeductionInput
                            Layout.fillWidth: true
                            placeholderText: "Leave blank if payment has already been credited..."
                            onReturnPressed: expLedgerCombo.focusAndOpen()
                        }

                        Text {
                            text: "(नोट : अगर पैमेंट पहले जमाखर्च की जा चुकी हो तो इस कॉलम को खाली छोड़ें |)"
                            color: "#64748B"
                            font.pixelSize: 10
                        }
                    }

                    // -------------------------------------------------
                    // POSTING LEDGERS SECTION (VISIBLE WHEN POST IN BOOKS)
                    // -------------------------------------------------
                    Rectangle {
                        Layout.fillWidth: true
                        radius: 8
                        color: "#F8FAFC"
                        border.color: "#E2E8F0"
                        border.width: 1
                        visible: root.postInBooks
                        implicitHeight: postCol.implicitHeight + 20

                        ColumnLayout {
                            id: postCol
                            anchors.fill: parent
                            anchors.margins: 10
                            spacing: 8

                            Text {
                                text: "Double-Entry Accounting Posting Ledgers :"
                                color: "#475569"
                                font.bold: true
                                font.pixelSize: 11
                            }

                            RowLayout {
                                Layout.fillWidth: true
                                spacing: 10

                                Text {
                                    text: "Exp. Dr. Ledger :"
                                    color: "#1D4ED8"
                                    font.bold: true
                                    font.pixelSize: 12
                                    Layout.preferredWidth: 120
                                }

                                CustomWhiteCombo {
                                    id: expLedgerCombo
                                    Layout.fillWidth: true
                                    model: (typeof partiesModel !== "undefined" && partiesModel) ? partiesModel.get_parties_list() : []
                                    onReturnPressed: tdsLedgerCombo.focusAndOpen()
                                }
                            }

                            RowLayout {
                                Layout.fillWidth: true
                                spacing: 10

                                Text {
                                    text: "TDS Cr. Ledger :"
                                    color: "#7C3AED"
                                    font.bold: true
                                    font.pixelSize: 12
                                    Layout.preferredWidth: 120
                                }

                                CustomWhiteCombo {
                                    id: tdsLedgerCombo
                                    Layout.fillWidth: true
                                    model: (typeof partiesModel !== "undefined" && partiesModel) ? partiesModel.get_parties_list() : []
                                    onReturnPressed: saveBtn.forceActiveFocus()
                                }
                            }
                        }
                    }
                }
            }
        }

        // =========================================================
        // BOTTOM ACTION / STATUS BAR
        // =========================================================
        Rectangle {
            Layout.fillWidth: true
            height: 44
            color: "#FFFFFF"
            radius: 8
            border.color: "#CBD5E1"

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 12
                anchors.rightMargin: 12
                spacing: 10

                Text {
                    text: root.statusMessage
                    color: root.isError ? "#DC2626" : "#16A34A"
                    font.bold: true
                    font.pixelSize: 12
                    Layout.fillWidth: true
                    elide: Text.ElideRight
                }

                T.Button {
                    id: cancelBtn
                    height: 32
                    background: Rectangle {
                        color: cancelBtn.hovered ? "#F1F5F9" : "#FFFFFF"
                        border.color: "#CBD5E1"
                        radius: 6
                    }
                    contentItem: Text {
                        anchors.centerIn: parent
                        text: "Cancel (Esc)"
                        color: "#475569"
                        font.bold: true
                        font.pixelSize: 12
                    }
                    onClicked: root.cancelRequested()
                }

                T.Button {
                    id: saveBtn
                    height: 32
                    Layout.preferredWidth: 200
                    background: Rectangle {
                        color: saveBtn.hovered ? "#6D28D9" : "#7C3AED"
                        radius: 6
                    }
                    contentItem: Text {
                        anchors.centerIn: parent
                        text: "💾 Save TDS Voucher (F2)"
                        color: "#FFFFFF"
                        font.bold: true
                        font.pixelSize: 12
                    }
                    onClicked: root.saveVoucher()
                    Keys.onReturnPressed: root.saveVoucher()
                    Keys.onEnterPressed: root.saveVoucher()
                }
            }
        }
    }

    // -------------------------------------------------------------
    // MODALS
    // -------------------------------------------------------------
    ConfirmationModal {
        id: confirmModal
        onConfirmed: root.executeSave()
    }

    T.Popup {
        id: newLedgerPopup
        x: Math.round((parent.width - width) / 2)
        y: Math.round((parent.height - height) / 2)
        width: 600
        height: 520
        modal: true
        dim: true
        closePolicy: T.Popup.CloseOnEscape | T.Popup.CloseOnPressOutside
        NewLedgerModal {
            anchors.fill: parent
            onCloseRequested: newLedgerPopup.close()
            onSavedSuccess: {
                if (typeof partiesModel !== "undefined" && partiesModel) {
                    partiesModel.reload_data()
                }
                newLedgerPopup.close()
            }
        }
    }
}
