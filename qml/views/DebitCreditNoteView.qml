pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Templates as T
import QtQuick.Layouts
import MahadevERP
import "../components"
import "../dialogs"

T.ScrollView {
    id: root
    contentWidth: availableWidth
    clip: true

    signal cancelRequested()
    signal openInvoiceRequested(string invoiceNo)

    property int currentViewMode: 0 // 0: Form Entry, 1: Register Table
    property string activeRegisterFilter: "ALL"

    function handleEscape(): void {
        if (root.currentViewMode === 0 && (debitCreditNoteCtrl ? debitCreditNoteCtrl.draftId > 0 : false)) {
            if (debitCreditNoteCtrl) debitCreditNoteCtrl.resetDraft()
            root.currentViewMode = 1
            return
        }
        root.cancelRequested()
    }

    function doSaveNote(): void {
        if (!partyInput.text.trim()) {
            partyInput.focusInput = true
            return
        }
        if (debitCreditNoteCtrl) {
            debitCreditNoteCtrl.draftPartyName = partyInput.text.trim()
            debitCreditNoteCtrl.draftPartyGstin = gstinInput.text.trim().toUpperCase()
            debitCreditNoteCtrl.draftNoteNo = noteNoInput.text.trim()
            debitCreditNoteCtrl.draftNoteDate = noteDateInput.text.trim()
            debitCreditNoteCtrl.draftNoteTime = noteTimeInput.text.trim()
            debitCreditNoteCtrl.draftOrigInvNo = origInvNoInput.text.trim()
            debitCreditNoteCtrl.draftOrigInvDate = origInvDateInput.text.trim()
            debitCreditNoteCtrl.draftNarration = narrationInput.text.trim()

            var ok = debitCreditNoteCtrl.saveCurrentDraft()
            if (ok) {
                root.currentViewMode = 1
                applyRegisterFilters()
            }
        }
    }

    function loadNoteForEditing(id: int): void {
        if (id <= 0 || !debitCreditNoteCtrl) return
        if (debitCreditNoteCtrl.loadNoteIntoDraft(id)) {
            root.currentViewMode = 0
        }
    }

    function applyRegisterFilters(): void {
        if (debitCreditNoteCtrl && debitCreditNoteCtrl.model) {
            debitCreditNoteCtrl.model.setFilter(root.activeRegisterFilter, searchInput.text.trim())
        }
    }

    Component.onCompleted: {
        if (debitCreditNoteCtrl) {
            debitCreditNoteCtrl.resetDraft()
            debitCreditNoteCtrl.reload()
        }
    }

    onVisibleChanged: {
        if (visible && debitCreditNoteCtrl) {
            debitCreditNoteCtrl.reload()
            applyRegisterFilters()
        }
    }

    ColumnLayout {
        width: root.availableWidth > 0 ? root.availableWidth : 1200
        spacing: 14

        // 1. Header Bar
        RowLayout {
            Layout.fillWidth: true
            spacing: 12

            ColumnLayout {
                spacing: 2
                Text {
                    text: "GST Debit Notes & Credit Notes (DebitCreditNotes)"
                    color: "#0F172A"
                    font.pixelSize: 20
                    font.bold: true
                }
                Text {
                    text: "Commercial sales returns, quality rate cuts on broken rice/bran, purchase adjustments, and GST accounting."
                    color: "#64748B"
                    font.pixelSize: 12
                }
            }

            Item { Layout.fillWidth: true }

            // Mode Toggle Buttons
            RowLayout {
                spacing: 6

                T.Button {
                    implicitHeight: 34
                    background: Rectangle {
                        color: root.currentViewMode === 0 ? "#2563EB" : "#F1F5F9"
                        radius: 6
                        border.color: root.currentViewMode === 0 ? "#1D4ED8" : "#CBD5E1"
                    }
                    contentItem: RowLayout {
                        spacing: 6
                        Text {
                            text: "+ New Note Entry"
                            color: root.currentViewMode === 0 ? "#FFFFFF" : "#334155"
                            font.pixelSize: 12
                            font.bold: true
                        }
                        KbdBadge { text: "Ctrl+N"; badgeColor: root.currentViewMode === 0 ? "#1E3A8A" : "#E2E8F0"; textColor: root.currentViewMode === 0 ? "#93C5FD" : "#475569"; borderColor: "#CBD5E1" }
                    }
                    onClicked: {
                        if (debitCreditNoteCtrl) debitCreditNoteCtrl.resetDraft()
                        root.currentViewMode = 0
                    }
                }

                T.Button {
                    implicitHeight: 34
                    background: Rectangle {
                        color: root.currentViewMode === 1 ? "#2563EB" : "#F1F5F9"
                        radius: 6
                        border.color: root.currentViewMode === 1 ? "#1D4ED8" : "#CBD5E1"
                    }
                    contentItem: RowLayout {
                        spacing: 6
                        Text {
                            text: "Note Register & Audit Ledger"
                            color: root.currentViewMode === 1 ? "#FFFFFF" : "#334155"
                            font.pixelSize: 12
                            font.bold: true
                        }
                        KbdBadge { text: "Alt+R"; badgeColor: root.currentViewMode === 1 ? "#1E3A8A" : "#E2E8F0"; textColor: root.currentViewMode === 1 ? "#93C5FD" : "#475569"; borderColor: "#CBD5E1" }
                    }
                    onClicked: {
                        root.currentViewMode = 1
                        applyRegisterFilters()
                    }
                }
            }

            T.Button {
                id: backBtn
                implicitWidth: contentItem.implicitWidth + 24
                implicitHeight: 34
                background: Rectangle { color: backBtn.hovered ? "#475569" : "#334155"; radius: 6 }
                contentItem: Text {
                    text: "< Back (Esc)"
                    color: "#F8FAFC"
                    font.pixelSize: 12
                    font.bold: true
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
                onClicked: root.handleEscape()
            }
        }

        Rectangle { Layout.fillWidth: true; height: 1; color: "#E2E8F0" }

        // 2. STATS STRIP
        RowLayout {
            Layout.fillWidth: true
            spacing: 12

            StatCard {
                title: "TOTAL CREDIT NOTES (SALES ADJ)"
                value: (typeof debitCreditNoteCtrl !== "undefined" && debitCreditNoteCtrl) ? debitCreditNoteCtrl.totalCreditAmountFmt : "Rs. 0.00"
                accentColor: "#DC2626"
                Layout.fillWidth: true
            }

            StatCard {
                title: "TOTAL DEBIT NOTES (PURCHASE ADJ)"
                value: (typeof debitCreditNoteCtrl !== "undefined" && debitCreditNoteCtrl) ? debitCreditNoteCtrl.totalDebitAmountFmt : "Rs. 0.00"
                accentColor: "#16A34A"
                Layout.fillWidth: true
            }

            StatCard {
                title: "TOTAL GST ADJUSTED"
                value: (typeof debitCreditNoteCtrl !== "undefined" && debitCreditNoteCtrl) ? debitCreditNoteCtrl.totalGstAdjustedFmt : "Rs. 0.00"
                accentColor: "#2563EB"
                Layout.fillWidth: true
            }

            StatCard {
                title: "NET ADJUSTMENT IMPACT"
                value: (typeof debitCreditNoteCtrl !== "undefined" && debitCreditNoteCtrl) ? debitCreditNoteCtrl.netAdjustmentValueFmt : "Rs. 0.00"
                accentColor: "#7C3AED"
                Layout.fillWidth: true
            }
        }

        // 3. MAIN CONTENT: VOUCHER FORM OR REGISTER TABLE
        // =========================================================================
        // VIEW MODE 0: NOTE CREATION & EDIT FORM
        // =========================================================================
        Rectangle {
            Layout.fillWidth: true
            implicitHeight: formCol.implicitHeight + 36
            visible: root.currentViewMode === 0
            color: "#FFFFFF"
            border.color: "#CBD5E1"
            border.width: 1
            radius: 8

            ColumnLayout {
                id: formCol
                anchors.fill: parent
                anchors.margins: 18
                spacing: 16

                // Section 1: Note Identifiers & Original Invoice Linkage
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 12

                    Rectangle {
                        width: 28
                        height: 28
                        radius: 6
                        color: "#EFF6FF"
                        Text { anchors.centerIn: parent; text: "1"; color: "#2563EB"; font.bold: true; font.pixelSize: 13 }
                    }
                    Text { text: "Note Details & Original Commercial Bill Linkage"; font.bold: true; font.pixelSize: 14; color: "#1E293B" }
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 10

                    CustomWhiteCombo {
                        id: noteTypeCombo
                        label: "Note Type *"
                        Layout.preferredWidth: 200
                        model: ["Credit Note", "Debit Note"]
                        currentIndex: (debitCreditNoteCtrl && debitCreditNoteCtrl.draftNoteType === "Debit Note") ? 1 : 0
                        onCurrentTextChanged: {
                            if (debitCreditNoteCtrl) {
                                debitCreditNoteCtrl.draftNoteType = currentText
                            }
                        }
                    }

                    CustomInput {
                        id: noteNoInput
                        label: "Note Number *"
                        text: debitCreditNoteCtrl ? debitCreditNoteCtrl.draftNoteNo : "CN-0001"
                        placeholderText: "CN-0001"
                        Layout.preferredWidth: 150
                        onTextEdited: {
                            if (debitCreditNoteCtrl) debitCreditNoteCtrl.draftNoteNo = text
                        }
                    }

                    CustomInput {
                        id: noteDateInput
                        label: "Note Date (YYYY-MM-DD) *"
                        text: debitCreditNoteCtrl ? debitCreditNoteCtrl.draftNoteDate : ""
                        placeholderText: "YYYY-MM-DD"
                        Layout.preferredWidth: 150
                        onTextEdited: {
                            if (debitCreditNoteCtrl) debitCreditNoteCtrl.draftNoteDate = text
                        }
                    }

                    CustomInput {
                        id: noteTimeInput
                        label: "Time"
                        text: debitCreditNoteCtrl ? debitCreditNoteCtrl.draftNoteTime : ""
                        placeholderText: "HH:MM"
                        Layout.preferredWidth: 110
                        onTextEdited: {
                            if (debitCreditNoteCtrl) debitCreditNoteCtrl.draftNoteTime = text
                        }
                    }

                    CustomWhiteCombo {
                        id: origInvTypeCombo
                        label: "Original Bill Type"
                        Layout.preferredWidth: 160
                        model: ["Sales Invoice", "Purchase Invoice"]
                        currentIndex: (debitCreditNoteCtrl && debitCreditNoteCtrl.draftOrigInvType === "Purchase") ? 1 : 0
                        onCurrentTextChanged: {
                            if (debitCreditNoteCtrl) {
                                debitCreditNoteCtrl.draftOrigInvType = currentText === "Purchase Invoice" ? "Purchase" : "Sale"
                            }
                        }
                    }

                    CustomInput {
                        id: origInvNoInput
                        label: "Original Invoice No *"
                        text: debitCreditNoteCtrl ? debitCreditNoteCtrl.draftOrigInvNo : ""
                        placeholderText: "Type Bill No e.g. 12640"
                        Layout.fillWidth: true
                        onTextEdited: {
                            if (debitCreditNoteCtrl) debitCreditNoteCtrl.draftOrigInvNo = text
                        }
                        onReturnPressed: {
                            if (debitCreditNoteCtrl) {
                                var iType = origInvTypeCombo.currentText === "Purchase Invoice" ? "Purchase" : "Sale"
                                debitCreditNoteCtrl.fetchAndLoadOriginalInvoice(iType, text.trim())
                            }
                        }
                    }

                    T.Button {
                        Layout.alignment: Qt.AlignBottom
                        implicitHeight: 36
                        background: Rectangle { color: "#EFF6FF"; radius: 6; border.color: "#BFDBFE" }
                        contentItem: Text { text: "Auto-Fetch"; color: "#2563EB"; font.pixelSize: 11; font.bold: true; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                        onClicked: {
                            if (debitCreditNoteCtrl) {
                                var iType = origInvTypeCombo.currentText === "Purchase Invoice" ? "Purchase" : "Sale"
                                debitCreditNoteCtrl.fetchAndLoadOriginalInvoice(iType, origInvNoInput.text.trim())
                            }
                        }
                    }

                    CustomInput {
                        id: origInvDateInput
                        label: "Orig Invoice Date"
                        text: debitCreditNoteCtrl ? debitCreditNoteCtrl.draftOrigInvDate : ""
                        placeholderText: "YYYY-MM-DD"
                        Layout.preferredWidth: 140
                        onTextEdited: {
                            if (debitCreditNoteCtrl) debitCreditNoteCtrl.draftOrigInvDate = text
                        }
                    }
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 10

                    CustomWhiteCombo {
                        id: reasonCombo
                        label: "GST Statutory Reason Code *"
                        Layout.preferredWidth: 240
                        model: [
                            "01-Sales Return",
                            "02-Post Sale Discount",
                            "03-Deficiency in Value / Quality Cut",
                            "04-Correction in Invoice",
                            "05-Change in POS",
                            "06-Other"
                        ]
                        onCurrentTextChanged: {
                            if (debitCreditNoteCtrl) debitCreditNoteCtrl.draftReasonCode = currentText
                        }
                    }

                    CustomWhiteCombo {
                        id: adjTypeCombo
                        label: "Accounting Nature *"
                        Layout.preferredWidth: 240
                        model: [
                            "Sales Return",
                            "Rate Cut / Quality Deduction",
                            "Purchase Return",
                            "Purchase Rate Cut",
                            "Rebate / Discount",
                            "Price Differential"
                        ]
                        onCurrentTextChanged: {
                            if (debitCreditNoteCtrl) debitCreditNoteCtrl.draftAdjType = currentText
                        }
                    }

                    CustomInput {
                        id: partyInput
                        label: "Party / Customer / Vendor Name *"
                        text: debitCreditNoteCtrl ? debitCreditNoteCtrl.draftPartyName : ""
                        placeholderText: "e.g. Haryana Food Corp"
                        isRequired: true
                        Layout.fillWidth: true
                        onTextEdited: {
                            if (debitCreditNoteCtrl) debitCreditNoteCtrl.draftPartyName = text
                        }
                    }

                    CustomInput {
                        id: gstinInput
                        label: "GSTIN"
                        text: debitCreditNoteCtrl ? debitCreditNoteCtrl.draftPartyGstin : ""
                        placeholderText: "03AABCR1234F1Z1"
                        Layout.preferredWidth: 170
                        onTextEdited: {
                            if (debitCreditNoteCtrl) debitCreditNoteCtrl.draftPartyGstin = text.toUpperCase()
                        }
                    }

                    CustomCheckBox {
                        id: isInterstateCheck
                        Layout.alignment: Qt.AlignBottom
                        text: "Interstate IGST"
                        checked: debitCreditNoteCtrl ? debitCreditNoteCtrl.draftIsInterstate : false
                        onCheckedChanged: {
                            if (debitCreditNoteCtrl) debitCreditNoteCtrl.draftIsInterstate = checked
                        }
                    }

                    CustomWhiteCombo {
                        id: gstSlabCombo
                        label: "GST Slab"
                        Layout.preferredWidth: 120
                        model: ["5.0% (Standard)", "0.0% (Exempt)", "12.0%", "18.0%"]
                        currentIndex: (debitCreditNoteCtrl && debitCreditNoteCtrl.draftGstPct === 0.0) ? 1 : ((debitCreditNoteCtrl && debitCreditNoteCtrl.draftGstPct === 12.0) ? 2 : ((debitCreditNoteCtrl && debitCreditNoteCtrl.draftGstPct === 18.0) ? 3 : 0))
                        onCurrentTextChanged: {
                            if (debitCreditNoteCtrl) {
                                if (currentIndex === 1) debitCreditNoteCtrl.draftGstPct = 0.0
                                else if (currentIndex === 2) debitCreditNoteCtrl.draftGstPct = 12.0
                                else if (currentIndex === 3) debitCreditNoteCtrl.draftGstPct = 18.0
                                else debitCreditNoteCtrl.draftGstPct = 5.0
                            }
                        }
                    }
                }

                Rectangle { Layout.fillWidth: true; height: 1; color: "#E2E8F0" }

                // Section 2: Items & Rate Adjustments Grid
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 12

                    Rectangle {
                        width: 28
                        height: 28
                        radius: 6
                        color: "#EFF6FF"
                        Text { anchors.centerIn: parent; text: "2"; color: "#2563EB"; font.bold: true; font.pixelSize: 13 }
                    }
                    Text { text: "Items, Quantity & Rate Adjustment Specification"; font.bold: true; font.pixelSize: 14; color: "#1E293B" }

                    Item { Layout.fillWidth: true }

                    T.Button {
                        implicitHeight: 30
                        background: Rectangle { color: "#F0FDF4"; radius: 5; border.color: "#BBF7D0" }
                        contentItem: Text { text: "+ Add Item Row"; color: "#166534"; font.pixelSize: 11; font.bold: true; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                        onClicked: {
                            if (debitCreditNoteCtrl && debitCreditNoteCtrl.itemsModel) {
                                debitCreditNoteCtrl.itemsModel.addItem("Rice Byproduct", "1006", 0, 0.0, 0.0, 0.0, 5.0)
                            }
                        }
                    }
                }

                // Line Items Table Header
                Rectangle {
                    Layout.fillWidth: true
                    height: 32
                    color: "#0F172A"
                    radius: 4

                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: 10; anchors.rightMargin: 10
                        spacing: 8

                        Text { text: "ITEM DESCRIPTION"; color: "#94A3B8"; font.pixelSize: 11; font.bold: true; Layout.fillWidth: true }
                        Text { text: "HSN"; color: "#94A3B8"; font.pixelSize: 11; font.bold: true; Layout.preferredWidth: 90 }
                        Text { text: "BAGS"; color: "#94A3B8"; font.pixelSize: 11; font.bold: true; Layout.preferredWidth: 70; horizontalAlignment: Text.AlignRight }
                        Text { text: "WEIGHT (QTL)"; color: "#94A3B8"; font.pixelSize: 11; font.bold: true; Layout.preferredWidth: 100; horizontalAlignment: Text.AlignRight }
                        Text { text: "RATE / CUT (RS)"; color: "#94A3B8"; font.pixelSize: 11; font.bold: true; Layout.preferredWidth: 110; horizontalAlignment: Text.AlignRight }
                        Text { text: "TAXABLE VALUE (RS)"; color: "#94A3B8"; font.pixelSize: 11; font.bold: true; Layout.preferredWidth: 140; horizontalAlignment: Text.AlignRight }
                        Text { text: "ACTION"; color: "#94A3B8"; font.pixelSize: 11; font.bold: true; Layout.preferredWidth: 50; horizontalAlignment: Text.AlignHCenter }
                    }
                }

                // Line Items List View
                ListView {
                    id: itemsListView
                    Layout.fillWidth: true
                    implicitHeight: Math.max(42, count * 44)
                    interactive: false
                    model: debitCreditNoteCtrl ? debitCreditNoteCtrl.itemsModel : null

                    delegate: Rectangle {
                        required property int index
                        required property string itemName
                        required property string hsnCode
                        required property int bags
                        required property real weightQtl
                        required property real rate
                        required property real taxableAmount

                        width: itemsListView.width
                        height: 38
                        color: index % 2 === 0 ? "#FFFFFF" : "#F8FAFC"
                        border.color: "#E2E8F0"
                        radius: 4

                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: 8; anchors.rightMargin: 8
                            spacing: 8

                            TextInput {
                                text: itemName
                                font.pixelSize: 12
                                color: "#0F172A"
                                Layout.fillWidth: true
                                selectByMouse: true
                                onTextEdited: {
                                    if (debitCreditNoteCtrl && debitCreditNoteCtrl.itemsModel) {
                                        debitCreditNoteCtrl.itemsModel.updateItem(index, "itemName", text)
                                    }
                                }
                            }

                            TextInput {
                                text: hsnCode
                                font.pixelSize: 12
                                color: "#0F172A"
                                Layout.preferredWidth: 90
                                selectByMouse: true
                                onTextEdited: {
                                    if (debitCreditNoteCtrl && debitCreditNoteCtrl.itemsModel) {
                                        debitCreditNoteCtrl.itemsModel.updateItem(index, "hsnCode", text)
                                    }
                                }
                            }

                            TextInput {
                                text: bags.toString()
                                font.pixelSize: 12
                                color: "#0F172A"
                                horizontalAlignment: Text.AlignRight
                                Layout.preferredWidth: 70
                                selectByMouse: true
                                inputMethodHints: Qt.ImhDigitsOnly
                                onTextEdited: {
                                    if (debitCreditNoteCtrl && debitCreditNoteCtrl.itemsModel) {
                                        debitCreditNoteCtrl.itemsModel.updateItem(index, "bags", parseInt(text) || 0)
                                    }
                                }
                            }

                            TextInput {
                                text: weightQtl.toString()
                                font.pixelSize: 12
                                color: "#0F172A"
                                horizontalAlignment: Text.AlignRight
                                Layout.preferredWidth: 100
                                selectByMouse: true
                                inputMethodHints: Qt.ImhFormattedNumbersOnly
                                onTextEdited: {
                                    if (debitCreditNoteCtrl && debitCreditNoteCtrl.itemsModel) {
                                        debitCreditNoteCtrl.itemsModel.updateItem(index, "weightQtl", parseFloat(text) || 0.0)
                                    }
                                }
                            }

                            TextInput {
                                text: rate.toString()
                                font.pixelSize: 12
                                color: "#0F172A"
                                horizontalAlignment: Text.AlignRight
                                Layout.preferredWidth: 110
                                selectByMouse: true
                                inputMethodHints: Qt.ImhFormattedNumbersOnly
                                onTextEdited: {
                                    if (debitCreditNoteCtrl && debitCreditNoteCtrl.itemsModel) {
                                        debitCreditNoteCtrl.itemsModel.updateItem(index, "rate", parseFloat(text) || 0.0)
                                    }
                                }
                            }

                            TextInput {
                                text: taxableAmount.toFixed(2)
                                font.pixelSize: 12
                                color: "#1E40AF"
                                font.bold: true
                                horizontalAlignment: Text.AlignRight
                                Layout.preferredWidth: 140
                                selectByMouse: true
                                inputMethodHints: Qt.ImhFormattedNumbersOnly
                                onTextEdited: {
                                    if (debitCreditNoteCtrl && debitCreditNoteCtrl.itemsModel) {
                                        debitCreditNoteCtrl.itemsModel.updateItem(index, "taxableAmount", parseFloat(text) || 0.0)
                                    }
                                }
                            }

                            T.Button {
                                implicitWidth: 28
                                implicitHeight: 24
                                background: Rectangle { color: "#FEF2F2"; radius: 4; border.color: "#FECACA" }
                                contentItem: Text { text: "X"; color: "#DC2626"; font.pixelSize: 11; font.bold: true; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                                onClicked: {
                                    if (debitCreditNoteCtrl && debitCreditNoteCtrl.itemsModel) {
                                        debitCreditNoteCtrl.itemsModel.removeItem(index)
                                    }
                                }
                            }
                        }
                    }
                }

                Rectangle { Layout.fillWidth: true; height: 1; color: "#E2E8F0" }

                // Section 3: Summary Totals & Double-Entry Calculation Strip
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 16

                    CustomInput {
                        id: narrationInput
                        label: "Narration / Reference Remarks"
                        text: debitCreditNoteCtrl ? debitCreditNoteCtrl.draftNarration : ""
                        placeholderText: "e.g. Quality rate deduction due to 22% broken rice content approved by buyer"
                        Layout.fillWidth: true
                        onTextEdited: {
                            if (debitCreditNoteCtrl) debitCreditNoteCtrl.draftNarration = text
                        }
                    }

                    // Calculation Summary Card
                    Rectangle {
                        Layout.preferredWidth: 420
                        implicitHeight: summaryGrid.implicitHeight + 20
                        color: "#F8FAFC"
                        border.color: "#CBD5E1"
                        radius: 8

                        ColumnLayout {
                            id: summaryGrid
                            anchors.fill: parent
                            anchors.margins: 10
                            spacing: 4

                            RowLayout {
                                Layout.fillWidth: true
                                Text { text: "Taxable Adjustment:"; color: "#475569"; font.pixelSize: 12 }
                                Item { Layout.fillWidth: true }
                                Text { text: debitCreditNoteCtrl ? debitCreditNoteCtrl.formTaxableAmountFmt : "Rs. 0.00"; color: "#0F172A"; font.pixelSize: 12; font.bold: true }
                            }

                            RowLayout {
                                Layout.fillWidth: true
                                Text { text: "Total GST Tax:"; color: "#475569"; font.pixelSize: 12 }
                                Item { Layout.fillWidth: true }
                                Text { text: debitCreditNoteCtrl ? debitCreditNoteCtrl.formTotalTaxAmountFmt : "Rs. 0.00"; color: "#2563EB"; font.pixelSize: 12; font.bold: true }
                            }

                            RowLayout {
                                Layout.fillWidth: true
                                visible: isInterstateCheck.checked
                                Text { text: "  - Output/Input IGST:"; color: "#64748B"; font.pixelSize: 11 }
                                Item { Layout.fillWidth: true }
                                Text { text: debitCreditNoteCtrl ? debitCreditNoteCtrl.formIgstAmountFmt : "Rs. 0.00"; color: "#64748B"; font.pixelSize: 11 }
                            }

                            RowLayout {
                                Layout.fillWidth: true
                                visible: !isInterstateCheck.checked
                                Text { text: "  - Output/Input CGST:"; color: "#64748B"; font.pixelSize: 11 }
                                Item { Layout.fillWidth: true }
                                Text { text: debitCreditNoteCtrl ? debitCreditNoteCtrl.formCgstAmountFmt : "Rs. 0.00"; color: "#64748B"; font.pixelSize: 11 }
                            }

                            RowLayout {
                                Layout.fillWidth: true
                                visible: !isInterstateCheck.checked
                                Text { text: "  - Output/Input SGST:"; color: "#64748B"; font.pixelSize: 11 }
                                Item { Layout.fillWidth: true }
                                Text { text: debitCreditNoteCtrl ? debitCreditNoteCtrl.formSgstAmountFmt : "Rs. 0.00"; color: "#64748B"; font.pixelSize: 11 }
                            }

                            RowLayout {
                                Layout.fillWidth: true
                                Text { text: "Round Off:"; color: "#64748B"; font.pixelSize: 11 }
                                Item { Layout.fillWidth: true }
                                Text { text: debitCreditNoteCtrl ? debitCreditNoteCtrl.formRoundOffFmt : "Rs. 0.00"; color: "#64748B"; font.pixelSize: 11 }
                            }

                            Rectangle { Layout.fillWidth: true; height: 1; color: "#CBD5E1" }

                            RowLayout {
                                Layout.fillWidth: true
                                Text { text: "GRAND TOTAL (LEDGER IMPACT):"; color: "#0F172A"; font.pixelSize: 13; font.bold: true }
                                Item { Layout.fillWidth: true }
                                Text { text: debitCreditNoteCtrl ? debitCreditNoteCtrl.formGrandTotalFmt : "Rs. 0.00"; color: "#059669"; font.pixelSize: 16; font.bold: true }
                            }
                        }
                    }
                }

                // Form Action Buttons Footer
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 12

                    T.Button {
                        implicitWidth: contentItem.implicitWidth + 24
                        implicitHeight: 34
                        background: Rectangle { color: "#F1F5F9"; radius: 6; border.color: "#CBD5E1" }
                        contentItem: Text { text: "Reset / Cancel (Esc)"; color: "#475569"; font.bold: true; font.pixelSize: 12; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                        onClicked: {
                            if (debitCreditNoteCtrl) debitCreditNoteCtrl.resetDraft()
                        }
                    }

                    Item { Layout.fillWidth: true }

                    T.Button {
                        id: savePrintNoteBtn
                        implicitWidth: contentItem.implicitWidth + 24
                        implicitHeight: 34
                        background: Rectangle { color: savePrintNoteBtn.hovered ? "#047857" : "#059669"; radius: 6 }
                        contentItem: RowLayout {
                            spacing: 6
                            Text { text: "Save & Print Note"; color: "#FFFFFF"; font.bold: true; font.pixelSize: 13 }
                            KbdBadge { text: "Ctrl+P"; badgeColor: "#064E3B"; textColor: "#6EE7B7"; borderColor: "#059669" }
                        }
                        onClicked: root.doSaveNote()
                    }

                    T.Button {
                        id: saveNoteBtn
                        implicitWidth: contentItem.implicitWidth + 24
                        implicitHeight: 34
                        background: Rectangle { color: saveNoteBtn.hovered ? "#1D4ED8" : "#2563EB"; radius: 6 }
                        contentItem: RowLayout {
                            spacing: 6
                            Text {
                                text: (debitCreditNoteCtrl && debitCreditNoteCtrl.draftId > 0) ? "Update Debit/Credit Note" : "Save & Post to Ledger"
                                color: "#FFFFFF"
                                font.bold: true
                                font.pixelSize: 13
                            }
                            KbdBadge { text: "Ctrl+S"; badgeColor: "#1E3A8A"; textColor: "#93C5FD"; borderColor: "#2563EB" }
                        }
                        onClicked: root.doSaveNote()
                    }
                }
            }
        }

        // =========================================================================
        // VIEW MODE 1: REGISTER & AUDIT LEDGER TABLE
        // =========================================================================
        Rectangle {
            visible: root.currentViewMode === 1
            Layout.fillWidth: true
            implicitHeight: 640
            color: "#FFFFFF"
            border.color: "#CBD5E1"
            border.width: 1
            radius: 8

            ColumnLayout {
                anchors.fill: parent
                spacing: 0

                // Filter Tabs & Search Bar Toolbar
                Rectangle {
                    Layout.fillWidth: true
                    height: 52
                    color: "#F8FAFC"
                    border.color: "#E2E8F0"
                    border.width: 1
                    radius: 8

                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: 16; anchors.rightMargin: 16
                        spacing: 12

                        RowLayout {
                            spacing: 8

                            T.Button {
                                implicitWidth: contentItem.implicitWidth + 24
                                implicitHeight: 32
                                background: Rectangle { color: root.activeRegisterFilter === "ALL" ? "#2563EB" : "#E2E8F0"; radius: 5 }
                                contentItem: Text { text: "All Notes"; color: root.activeRegisterFilter === "ALL" ? "#FFFFFF" : "#334155"; font.pixelSize: 11; font.bold: true; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                                onClicked: { root.activeRegisterFilter = "ALL"; root.applyRegisterFilters() }
                            }

                            T.Button {
                                implicitWidth: contentItem.implicitWidth + 24
                                implicitHeight: 32
                                background: Rectangle { color: root.activeRegisterFilter === "CREDIT_NOTES" ? "#DC2626" : "#E2E8F0"; radius: 5 }
                                contentItem: Text { text: "Credit Notes (Sales)"; color: root.activeRegisterFilter === "CREDIT_NOTES" ? "#FFFFFF" : "#334155"; font.pixelSize: 11; font.bold: true; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                                onClicked: { root.activeRegisterFilter = "CREDIT_NOTES"; root.applyRegisterFilters() }
                            }

                            T.Button {
                                implicitWidth: contentItem.implicitWidth + 24
                                implicitHeight: 32
                                background: Rectangle { color: root.activeRegisterFilter === "DEBIT_NOTES" ? "#16A34A" : "#E2E8F0"; radius: 5 }
                                contentItem: Text { text: "Debit Notes (Purchases)"; color: root.activeRegisterFilter === "DEBIT_NOTES" ? "#FFFFFF" : "#334155"; font.pixelSize: 11; font.bold: true; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                                onClicked: { root.activeRegisterFilter = "DEBIT_NOTES"; root.applyRegisterFilters() }
                            }

                            T.Button {
                                implicitWidth: contentItem.implicitWidth + 24
                                implicitHeight: 32
                                background: Rectangle { color: root.activeRegisterFilter === "SALES_RETURNS" ? "#7C3AED" : "#E2E8F0"; radius: 5 }
                                contentItem: Text { text: "Sales Returns"; color: root.activeRegisterFilter === "SALES_RETURNS" ? "#FFFFFF" : "#334155"; font.pixelSize: 11; font.bold: true; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                                onClicked: { root.activeRegisterFilter = "SALES_RETURNS"; root.applyRegisterFilters() }
                            }

                            T.Button {
                                implicitWidth: contentItem.implicitWidth + 24
                                implicitHeight: 32
                                background: Rectangle { color: root.activeRegisterFilter === "RATE_CUTS" ? "#D97706" : "#E2E8F0"; radius: 5 }
                                contentItem: Text { text: "Quality Rate Cuts"; color: root.activeRegisterFilter === "RATE_CUTS" ? "#FFFFFF" : "#334155"; font.pixelSize: 11; font.bold: true; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                                onClicked: { root.activeRegisterFilter = "RATE_CUTS"; root.applyRegisterFilters() }
                            }
                        }

                        Item { Layout.fillWidth: true }

                        Text { text: "Search Notes:"; color: "#334155"; font.pixelSize: 12; font.bold: true }

                        Rectangle {
                            Layout.preferredWidth: 280
                            height: 34
                            color: "#FFFFFF"
                            border.color: searchInput.activeFocus ? "#2563EB" : "#CBD5E1"
                            border.width: searchInput.activeFocus ? 2 : 1
                            radius: 6

                            RowLayout {
                                anchors.fill: parent
                                anchors.leftMargin: 10; anchors.rightMargin: 10
                                spacing: 6

                                TextInput {
                                    id: searchInput
                                    Layout.fillWidth: true
                                    font.pixelSize: 12
                                    color: "#0F172A"
                                    selectByMouse: true
                                    onTextChanged: root.applyRegisterFilters()
                                }

                                Text {
                                    text: "Clear"
                                    color: "#64748B"
                                    font.pixelSize: 11
                                    visible: searchInput.text.length > 0
                                    MouseArea {
                                        anchors.fill: parent
                                        cursorShape: Qt.PointingHandCursor
                                        onClicked: {
                                            searchInput.text = ""
                                            root.applyRegisterFilters()
                                        }
                                    }
                                }
                            }
                        }
                    }
                }

                // Table Header
                Rectangle {
                    Layout.fillWidth: true
                    height: 38
                    color: "#0F172A"

                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: 12; anchors.rightMargin: 12
                        spacing: 8

                        Text { text: "NOTE #"; color: "#94A3B8"; font.pixelSize: 11; font.bold: true; Layout.preferredWidth: 90 }
                        Text { text: "DATE"; color: "#94A3B8"; font.pixelSize: 11; font.bold: true; Layout.preferredWidth: 85 }
                        Text { text: "TYPE"; color: "#94A3B8"; font.pixelSize: 11; font.bold: true; Layout.preferredWidth: 95 }
                        Text { text: "ORIG BILL"; color: "#94A3B8"; font.pixelSize: 11; font.bold: true; Layout.preferredWidth: 90 }
                        Text { text: "PARTY / CUSTOMER"; color: "#94A3B8"; font.pixelSize: 11; font.bold: true; Layout.fillWidth: true }
                        Text { text: "ADJUSTMENT NATURE"; color: "#94A3B8"; font.pixelSize: 11; font.bold: true; Layout.preferredWidth: 160 }
                        Text { text: "BAGS"; color: "#94A3B8"; font.pixelSize: 11; font.bold: true; Layout.preferredWidth: 60; horizontalAlignment: Text.AlignRight }
                        Text { text: "WEIGHT"; color: "#94A3B8"; font.pixelSize: 11; font.bold: true; Layout.preferredWidth: 80; horizontalAlignment: Text.AlignRight }
                        Text { text: "TAXABLE RS"; color: "#94A3B8"; font.pixelSize: 11; font.bold: true; Layout.preferredWidth: 100; horizontalAlignment: Text.AlignRight }
                        Text { text: "TOTAL TAX"; color: "#94A3B8"; font.pixelSize: 11; font.bold: true; Layout.preferredWidth: 90; horizontalAlignment: Text.AlignRight }
                        Text { text: "GRAND TOTAL"; color: "#94A3B8"; font.pixelSize: 11; font.bold: true; Layout.preferredWidth: 110; horizontalAlignment: Text.AlignRight }
                        Text { text: "ACTIONS"; color: "#94A3B8"; font.pixelSize: 11; font.bold: true; Layout.preferredWidth: 80; horizontalAlignment: Text.AlignHCenter }
                    }
                }

                // Table List View
                ListView {
                    id: noteListView
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true
                    model: (typeof debitCreditNoteCtrl !== "undefined" && debitCreditNoteCtrl) ? debitCreditNoteCtrl.model : null

                    T.ScrollBar.vertical: T.ScrollBar { policy: T.ScrollBar.AsNeeded }

                    delegate: Rectangle {
                        required property int index
                        required property int noteId
                        required property string noteNo
                        required property string noteDate
                        required property string noteType
                        required property string originalInvoiceNo
                        required property string partyName
                        required property string adjustmentType
                        required property int totalBags
                        required property real totalWeightQtl
                        required property string taxableAmountFmt
                        required property string totalTaxAmountFmt
                        required property string grandTotalFmt

                        width: noteListView.width
                        height: 42
                        color: index % 2 === 0 ? "#FFFFFF" : "#F8FAFC"

                        Rectangle {
                            anchors.bottom: parent.bottom
                            width: parent.width
                            height: 1
                            color: "#E2E8F0"
                        }

                        MouseArea {
                            anchors.fill: parent
                            hoverEnabled: true
                            onEntered: parent.color = "#EFF6FF"
                            onExited: parent.color = index % 2 === 0 ? "#FFFFFF" : "#F8FAFC"
                            onDoubleClicked: root.loadNoteForEditing(noteId)
                        }

                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: 12; anchors.rightMargin: 12
                            spacing: 8

                            Text {
                                text: noteNo
                                color: "#2563EB"
                                font.pixelSize: 12
                                font.bold: true
                                Layout.preferredWidth: 90
                            }

                            Text {
                                text: noteDate
                                color: "#64748B"
                                font.pixelSize: 11
                                Layout.preferredWidth: 85
                            }

                            Rectangle {
                                Layout.preferredWidth: 95
                                height: 22
                                radius: 4
                                color: noteType === "Credit Note" ? "#FEE2E2" : "#DCFCE7"
                                border.color: noteType === "Credit Note" ? "#FCA5A5" : "#86EFAC"
                                Text {
                                    anchors.centerIn: parent
                                    text: noteType
                                    color: noteType === "Credit Note" ? "#991B1B" : "#166534"
                                    font.pixelSize: 10
                                    font.bold: true
                                }
                            }

                            Text {
                                text: originalInvoiceNo.length > 0 ? originalInvoiceNo : "-"
                                color: "#0F172A"
                                font.pixelSize: 11
                                Layout.preferredWidth: 90
                            }

                            Text {
                                text: partyName.length > 0 ? partyName : "-"
                                color: "#0F172A"
                                font.pixelSize: 12
                                elide: Text.ElideRight
                                Layout.fillWidth: true
                            }

                            Text {
                                text: adjustmentType.length > 0 ? adjustmentType : "-"
                                color: "#475569"
                                font.pixelSize: 11
                                elide: Text.ElideRight
                                Layout.preferredWidth: 160
                            }

                            Text {
                                text: totalBags.toString()
                                color: "#334155"
                                font.pixelSize: 12
                                Layout.preferredWidth: 60
                                horizontalAlignment: Text.AlignRight
                            }

                            Text {
                                text: totalWeightQtl.toFixed(2)
                                color: "#334155"
                                font.pixelSize: 12
                                Layout.preferredWidth: 80
                                horizontalAlignment: Text.AlignRight
                            }

                            Text {
                                text: taxableAmountFmt
                                color: "#0F172A"
                                font.pixelSize: 12
                                Layout.preferredWidth: 100
                                horizontalAlignment: Text.AlignRight
                            }

                            Text {
                                text: totalTaxAmountFmt
                                color: "#2563EB"
                                font.pixelSize: 12
                                Layout.preferredWidth: 90
                                horizontalAlignment: Text.AlignRight
                            }

                            Text {
                                text: grandTotalFmt
                                color: noteType === "Credit Note" ? "#DC2626" : "#16A34A"
                                font.pixelSize: 12
                                font.bold: true
                                Layout.preferredWidth: 110
                                horizontalAlignment: Text.AlignRight
                            }

                            RowLayout {
                                Layout.preferredWidth: 80
                                spacing: 4

                                T.Button {
                                    implicitWidth: 36
                                    implicitHeight: 26
                                    background: Rectangle { color: "#EFF6FF"; radius: 4; border.color: "#BFDBFE" }
                                    contentItem: Text { text: "Edit"; color: "#2563EB"; font.pixelSize: 11; font.bold: true; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                                    onClicked: root.loadNoteForEditing(noteId)
                                }

                                T.Button {
                                    implicitWidth: 36
                                    implicitHeight: 26
                                    background: Rectangle { color: "#FEF2F2"; radius: 4; border.color: "#FECACA" }
                                    contentItem: Text { text: "Del"; color: "#DC2626"; font.pixelSize: 11; font.bold: true; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                                    onClicked: {
                                        if (typeof debitCreditNoteCtrl !== "undefined" && debitCreditNoteCtrl) {
                                            debitCreditNoteCtrl.deleteNote(noteId)
                                        }
                                    }
                                }
                            }
                        }
                    }

                    // Empty State
                    Rectangle {
                        anchors.centerIn: parent
                        visible: noteListView.count === 0
                        width: 320
                        height: 100
                        color: "transparent"

                        ColumnLayout {
                            anchors.centerIn: parent
                            spacing: 8
                            Text {
                                text: "No Debit / Credit Notes Found"
                                color: "#64748B"
                                font.pixelSize: 14
                                font.bold: true
                                Layout.alignment: Qt.AlignHCenter
                            }
                            Text {
                                text: "Click '+ New Note Entry' to create a GST credit/debit note."
                                color: "#94A3B8"
                                font.pixelSize: 12
                                Layout.alignment: Qt.AlignHCenter
                            }
                        }
                    }
                }
            }
        }
    }

    Shortcut {
        sequence: "Ctrl+N"
        onActivated: {
            if (debitCreditNoteCtrl) debitCreditNoteCtrl.resetDraft()
            root.currentViewMode = 0
        }
    }
    Shortcut {
        sequence: "Alt+R"
        onActivated: {
            root.currentViewMode = 1
            applyRegisterFilters()
        }
    }
    Shortcut {
        sequence: "Ctrl+S"
        onActivated: {
            if (root.currentViewMode === 0) {
                root.doSaveNote()
            }
        }
    }
    Shortcut {
        sequence: "Escape"
        onActivated: root.handleEscape()
    }
}
