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
    property int editingNoteId: 0
    property string activeRegisterFilter: "ALL"

    property var currentItemsList: []

    function handleEscape() {
        if (root.currentViewMode === 0 && root.editingNoteId > 0) {
            resetForm()
            root.currentViewMode = 1
            return
        }
        root.cancelRequested()
    }

    function resetForm() {
        root.editingNoteId = 0
        var nType = noteTypeCombo.currentText || "Credit Note"
        if (typeof debitCreditNoteCtrl !== "undefined") {
            noteNoInput.text = debitCreditNoteCtrl.getNextNoteNo(nType)
        } else {
            noteNoInput.text = "CN-0001"
        }

        var today = new Date()
        var yyyy = today.getFullYear()
        var mm = String(today.getMonth() + 1).padStart(2, '0')
        var dd = String(today.getDate()).padStart(2, '0')
        noteDateInput.text = yyyy + "-" + mm + "-" + dd

        var hh = String(today.getHours()).padStart(2, '0')
        var min = String(today.getMinutes()).padStart(2, '0')
        noteTimeInput.text = hh + ":" + min

        origInvTypeCombo.currentIndex = 0
        origInvNoInput.text = ""
        origInvDateInput.text = yyyy + "-" + mm + "-" + dd
        partyInput.text = ""
        gstinInput.text = ""
        isInterstateCheck.checked = false
        gstSlabCombo.currentIndex = 0
        reasonCombo.currentIndex = 0
        adjTypeCombo.currentIndex = 0
        narrationInput.text = ""

        root.currentItemsList = []
        addNewItemRow("Basmati Rice", "10063020", 0, 0.0, 0.0, 0.0)
        recalculateTotals()
    }

    function addNewItemRow(iName, hsn, bags, wt, rate, amt) {
        var list = root.currentItemsList.slice()
        list.push({
            "itemName": iName || "Rice Byproduct",
            "hsnCode": hsn || "10063020",
            "unit": "QTL",
            "bags": bags || 0,
            "weightQtl": wt || 0.0,
            "rate": rate || 0.0,
            "taxableAmount": amt || 0.0,
            "gstPct": 5.0
        })
        root.currentItemsList = list
        recalculateTotals()
    }

    function removeItemRow(idx) {
        if (idx >= 0 && idx < root.currentItemsList.length) {
            var list = root.currentItemsList.slice()
            list.splice(idx, 1)
            if (list.length === 0) {
                list.push({
                    "itemName": "",
                    "hsnCode": "1006",
                    "unit": "QTL",
                    "bags": 0,
                    "weightQtl": 0.0,
                    "rate": 0.0,
                    "taxableAmount": 0.0,
                    "gstPct": 5.0
                })
            }
            root.currentItemsList = list
            recalculateTotals()
        }
    }

    function updateItemRow(idx, field, value) {
        if (idx >= 0 && idx < root.currentItemsList.length) {
            var list = root.currentItemsList.slice()
            var row = Object.assign({}, list[idx])
            row[field] = value

            if (field === "weightQtl" || field === "rate" || field === "bags") {
                var wt = parseFloat(row.weightQtl) || 0.0
                var rt = parseFloat(row.rate) || 0.0
                var bg = parseInt(row.bags) || 0
                if (wt > 0.0) {
                    row.taxableAmount = (wt * rt)
                } else if (bg > 0) {
                    row.taxableAmount = (bg * rt)
                }
            }
            list[idx] = row
            root.currentItemsList = list
            recalculateTotals()
        }
    }

    function doAutoFetchInvoice() {
        var invNo = origInvNoInput.text.trim()
        if (!invNo) return

        var iType = origInvTypeCombo.currentText === "Purchase Invoice" ? "Purchase" : "Sale"
        if (typeof debitCreditNoteCtrl !== "undefined") {
            var res = debitCreditNoteCtrl.fetchOriginalInvoice(iType, invNo)
            if (res && res.found) {
                origInvDateInput.text = res.invoiceDate || ""
                partyInput.text = res.partyName || ""
                gstinInput.text = res.partyGstin || ""
                isInterstateCheck.checked = res.isInterstate || false

                var slab = (res.gstPct || 5.0).toFixed(1)
                if (slab === "0.0") gstSlabCombo.currentIndex = 1
                else if (slab === "12.0") gstSlabCombo.currentIndex = 2
                else if (slab === "18.0") gstSlabCombo.currentIndex = 3
                else gstSlabCombo.currentIndex = 0

                if (res.items && res.items.length > 0) {
                    var newItems = []
                    for (var i = 0; i < res.items.length; ++i) {
                        var it = res.items[i]
                        newItems.push({
                            "itemName": it.itemName || "",
                            "hsnCode": it.hsnCode || "1006",
                            "unit": it.unit || "QTL",
                            "bags": it.bags || 0,
                            "weightQtl": it.weightQtl || 0.0,
                            "rate": it.rate || 0.0,
                            "taxableAmount": it.taxableAmount || 0.0,
                            "gstPct": it.gstPct || 5.0
                        })
                    }
                    root.currentItemsList = newItems
                }
                recalculateTotals()
            }
        }
    }

    function recalculateTotals() {
        var gstVal = 5.0
        if (gstSlabCombo.currentIndex === 1) gstVal = 0.0
        else if (gstSlabCombo.currentIndex === 2) gstVal = 12.0
        else if (gstSlabCombo.currentIndex === 3) gstVal = 18.0

        if (typeof debitCreditNoteCtrl !== "undefined") {
            var res = debitCreditNoteCtrl.calculateTotals(root.currentItemsList, gstVal, isInterstateCheck.checked)
            taxableDisplay.text = "Rs. " + (res.taxableAmount || 0.0).toFixed(2)
            cgstDisplay.text = "Rs. " + (res.cgstAmount || 0.0).toFixed(2)
            sgstDisplay.text = "Rs. " + (res.sgstAmount || 0.0).toFixed(2)
            igstDisplay.text = "Rs. " + (res.igstAmount || 0.0).toFixed(2)
            roundOffDisplay.text = "Rs. " + (res.roundOff || 0.0).toFixed(2)
            grandTotalDisplay.text = "Rs. " + (res.grandTotal || 0.0).toFixed(2)
            totalTaxDisplay.text = "Rs. " + (res.totalTaxAmount || 0.0).toFixed(2)
        }
    }

    function doSaveNote() {
        if (!partyInput.text.trim()) {
            partyInput.focusInput = true
            return
        }

        var gstVal = 5.0
        if (gstSlabCombo.currentIndex === 1) gstVal = 0.0
        else if (gstSlabCombo.currentIndex === 2) gstVal = 12.0
        else if (gstSlabCombo.currentIndex === 3) gstVal = 18.0

        var payload = {
            "id": root.editingNoteId,
            "noteType": noteTypeCombo.currentText,
            "noteNo": noteNoInput.text.trim(),
            "noteDate": noteDateInput.text.trim(),
            "noteTime": noteTimeInput.text.trim(),
            "originalInvoiceNo": origInvNoInput.text.trim(),
            "originalInvoiceDate": origInvDateInput.text.trim(),
            "originalInvoiceType": origInvTypeCombo.currentText === "Purchase Invoice" ? "Purchase" : "Sale",
            "partyName": partyInput.text.trim(),
            "partyGstin": gstinInput.text.trim().toUpperCase(),
            "isInterstate": isInterstateCheck.checked,
            "reasonCode": reasonCombo.currentText,
            "adjustmentType": adjTypeCombo.currentText,
            "gstPct": gstVal,
            "narration": narrationInput.text.trim(),
            "items": root.currentItemsList
        }

        if (typeof debitCreditNoteCtrl !== "undefined") {
            var res = debitCreditNoteCtrl.saveNote(payload)
            if (res && res.success) {
                root.currentViewMode = 1
                applyRegisterFilters()
            }
        }
    }

    function loadNoteForEditing(id) {
        if (id <= 0 || typeof debitCreditNoteCtrl === "undefined") return
        var data = debitCreditNoteCtrl.getNote(id)
        if (!data || !data.noteNo) return

        root.editingNoteId = id
        if (data.noteType === "Debit Note") noteTypeCombo.currentIndex = 1
        else noteTypeCombo.currentIndex = 0

        noteNoInput.text = data.noteNo || ""
        noteDateInput.text = data.noteDate || ""
        noteTimeInput.text = data.noteTime || ""

        if (data.originalInvoiceType === "Purchase") origInvTypeCombo.currentIndex = 1
        else origInvTypeCombo.currentIndex = 0

        origInvNoInput.text = data.originalInvoiceNo || ""
        origInvDateInput.text = data.originalInvoiceDate || ""
        partyInput.text = data.partyName || ""
        gstinInput.text = data.partyGstin || ""
        isInterstateCheck.checked = data.isInterstate || false

        var slab = (data.gstPct || 5.0).toFixed(1)
        if (slab === "0.0") gstSlabCombo.currentIndex = 1
        else if (slab === "12.0") gstSlabCombo.currentIndex = 2
        else if (slab === "18.0") gstSlabCombo.currentIndex = 3
        else gstSlabCombo.currentIndex = 0

        narrationInput.text = data.narration || ""
        root.currentItemsList = data.items || []
        recalculateTotals()
        root.currentViewMode = 0
    }

    function applyRegisterFilters() {
        if (typeof debitCreditNoteCtrl !== "undefined" && debitCreditNoteCtrl.model) {
            debitCreditNoteCtrl.model.setFilter(root.activeRegisterFilter, searchInput.text.trim())
        }
    }

    Component.onCompleted: {
        resetForm()
        if (typeof debitCreditNoteCtrl !== "undefined") {
            debitCreditNoteCtrl.reload()
        }
    }

    onVisibleChanged: {
        if (visible && typeof debitCreditNoteCtrl !== "undefined") {
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
                        root.resetForm()
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
                        onCurrentTextChanged: {
                            if (root.editingNoteId <= 0 && typeof debitCreditNoteCtrl !== "undefined") {
                                noteNoInput.text = debitCreditNoteCtrl.getNextNoteNo(currentText)
                            }
                        }
                    }

                    CustomInput {
                        id: noteNoInput
                        label: "Note Number *"
                        placeholderText: "CN-0001"
                        Layout.preferredWidth: 150
                    }

                    CustomInput {
                        id: noteDateInput
                        label: "Note Date (YYYY-MM-DD) *"
                        placeholderText: "YYYY-MM-DD"
                        Layout.preferredWidth: 150
                    }

                    CustomInput {
                        id: noteTimeInput
                        label: "Time"
                        placeholderText: "HH:MM"
                        Layout.preferredWidth: 110
                    }

                    CustomWhiteCombo {
                        id: origInvTypeCombo
                        label: "Original Bill Type"
                        Layout.preferredWidth: 160
                        model: ["Sales Invoice", "Purchase Invoice"]
                    }

                    CustomInput {
                        id: origInvNoInput
                        label: "Original Invoice No *"
                        placeholderText: "Type Bill No e.g. 12640"
                        Layout.fillWidth: true
                        onReturnPressed: doAutoFetchInvoice()
                    }

                    T.Button {
                        Layout.alignment: Qt.AlignBottom
                        implicitHeight: 36
                        background: Rectangle { color: "#EFF6FF"; radius: 6; border.color: "#BFDBFE" }
                        contentItem: Text { text: "Auto-Fetch"; color: "#2563EB"; font.pixelSize: 11; font.bold: true; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                        onClicked: doAutoFetchInvoice()
                    }

                    CustomInput {
                        id: origInvDateInput
                        label: "Orig Invoice Date"
                        placeholderText: "YYYY-MM-DD"
                        Layout.preferredWidth: 140
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
                    }

                    CustomInput {
                        id: partyInput
                        label: "Party / Customer / Vendor Name *"
                        placeholderText: "e.g. Haryana Food Corp"
                        isRequired: true
                        Layout.fillWidth: true
                    }

                    CustomInput {
                        id: gstinInput
                        label: "GSTIN"
                        placeholderText: "03AABCR1234F1Z1"
                        Layout.preferredWidth: 170
                    }

                    CustomCheckBox {
                        id: isInterstateCheck
                        Layout.alignment: Qt.AlignBottom
                        text: "Interstate IGST"
                        onCheckedChanged: recalculateTotals()
                    }

                    CustomWhiteCombo {
                        id: gstSlabCombo
                        label: "GST Slab"
                        Layout.preferredWidth: 120
                        model: ["5.0% (Standard)", "0.0% (Exempt)", "12.0%", "18.0%"]
                        onCurrentTextChanged: recalculateTotals()
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
                        onClicked: addNewItemRow("", "1006", 0, 0.0, 0.0, 0.0)
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

                // Line Items Repeater
                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 6

                    Repeater {
                        model: root.currentItemsList

                        Rectangle {
                            Layout.fillWidth: true
                            height: 38
                            color: index % 2 === 0 ? "#FFFFFF" : "#F8FAFC"
                            border.color: "#E2E8F0"
                            radius: 4

                            RowLayout {
                                anchors.fill: parent
                                anchors.leftMargin: 8; anchors.rightMargin: 8
                                spacing: 8

                                TextInput {
                                    text: modelData.itemName || ""
                                    font.pixelSize: 12
                                    color: "#0F172A"
                                    Layout.fillWidth: true
                                    selectByMouse: true
                                    onTextEdited: root.updateItemRow(index, "itemName", text)
                                }

                                TextInput {
                                    text: modelData.hsnCode || "1006"
                                    font.pixelSize: 12
                                    color: "#0F172A"
                                    Layout.preferredWidth: 90
                                    selectByMouse: true
                                    onTextEdited: root.updateItemRow(index, "hsnCode", text)
                                }

                                TextInput {
                                    text: (modelData.bags || 0).toString()
                                    font.pixelSize: 12
                                    color: "#0F172A"
                                    horizontalAlignment: Text.AlignRight
                                    Layout.preferredWidth: 70
                                    selectByMouse: true
                                    inputMethodHints: Qt.ImhDigitsOnly
                                    onTextEdited: root.updateItemRow(index, "bags", parseInt(text) || 0)
                                }

                                TextInput {
                                    text: (modelData.weightQtl || 0.0).toString()
                                    font.pixelSize: 12
                                    color: "#0F172A"
                                    horizontalAlignment: Text.AlignRight
                                    Layout.preferredWidth: 100
                                    selectByMouse: true
                                    inputMethodHints: Qt.ImhFormattedNumbersOnly
                                    onTextEdited: root.updateItemRow(index, "weightQtl", parseFloat(text) || 0.0)
                                }

                                TextInput {
                                    text: (modelData.rate || 0.0).toString()
                                    font.pixelSize: 12
                                    color: "#0F172A"
                                    horizontalAlignment: Text.AlignRight
                                    Layout.preferredWidth: 110
                                    selectByMouse: true
                                    inputMethodHints: Qt.ImhFormattedNumbersOnly
                                    onTextEdited: root.updateItemRow(index, "rate", parseFloat(text) || 0.0)
                                }

                                TextInput {
                                    text: (modelData.taxableAmount || 0.0).toFixed(2)
                                    font.pixelSize: 12
                                    color: "#1E40AF"
                                    font.bold: true
                                    horizontalAlignment: Text.AlignRight
                                    Layout.preferredWidth: 140
                                    selectByMouse: true
                                    inputMethodHints: Qt.ImhFormattedNumbersOnly
                                    onTextEdited: root.updateItemRow(index, "taxableAmount", parseFloat(text) || 0.0)
                                }

                                T.Button {
                                    implicitWidth: 28
                                    implicitHeight: 24
                                    background: Rectangle { color: "#FEF2F2"; radius: 4; border.color: "#FECACA" }
                                    contentItem: Text { text: "X"; color: "#DC2626"; font.pixelSize: 11; font.bold: true; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                                    onClicked: root.removeItemRow(index)
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
                        placeholderText: "e.g. Quality rate deduction due to 22% broken rice content approved by buyer"
                        Layout.fillWidth: true
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
                                Text { id: taxableDisplay; text: "Rs. 0.00"; color: "#0F172A"; font.pixelSize: 12; font.bold: true }
                            }

                            RowLayout {
                                Layout.fillWidth: true
                                Text { text: "Total GST Tax:"; color: "#475569"; font.pixelSize: 12 }
                                Item { Layout.fillWidth: true }
                                Text { id: totalTaxDisplay; text: "Rs. 0.00"; color: "#2563EB"; font.pixelSize: 12; font.bold: true }
                            }

                            RowLayout {
                                Layout.fillWidth: true
                                visible: isInterstateCheck.checked
                                Text { text: "  - Output/Input IGST:"; color: "#64748B"; font.pixelSize: 11 }
                                Item { Layout.fillWidth: true }
                                Text { id: igstDisplay; text: "Rs. 0.00"; color: "#64748B"; font.pixelSize: 11 }
                            }

                            RowLayout {
                                Layout.fillWidth: true
                                visible: !isInterstateCheck.checked
                                Text { text: "  - Output/Input CGST:"; color: "#64748B"; font.pixelSize: 11 }
                                Item { Layout.fillWidth: true }
                                Text { id: cgstDisplay; text: "Rs. 0.00"; color: "#64748B"; font.pixelSize: 11 }
                            }

                            RowLayout {
                                Layout.fillWidth: true
                                visible: !isInterstateCheck.checked
                                Text { text: "  - Output/Input SGST:"; color: "#64748B"; font.pixelSize: 11 }
                                Item { Layout.fillWidth: true }
                                Text { id: sgstDisplay; text: "Rs. 0.00"; color: "#64748B"; font.pixelSize: 11 }
                            }

                            RowLayout {
                                Layout.fillWidth: true
                                Text { text: "Round Off:"; color: "#64748B"; font.pixelSize: 11 }
                                Item { Layout.fillWidth: true }
                                Text { id: roundOffDisplay; text: "Rs. 0.00"; color: "#64748B"; font.pixelSize: 11 }
                            }

                            Rectangle { Layout.fillWidth: true; height: 1; color: "#CBD5E1" }

                            RowLayout {
                                Layout.fillWidth: true
                                Text { text: "GRAND TOTAL (LEDGER IMPACT):"; color: "#0F172A"; font.pixelSize: 13; font.bold: true }
                                Item { Layout.fillWidth: true }
                                Text { id: grandTotalDisplay; text: "Rs. 0.00"; color: "#059669"; font.pixelSize: 16; font.bold: true }
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
                        onClicked: root.resetForm()
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
                            Text { text: root.editingNoteId > 0 ? "Update Debit/Credit Note" : "Save & Post to Ledger"; color: "#FFFFFF"; font.bold: true; font.pixelSize: 13 }
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
                            onDoubleClicked: root.loadNoteForEditing(model.noteId)
                        }

                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: 12; anchors.rightMargin: 12
                            spacing: 8

                            Text {
                                text: model.noteNo || ""
                                color: "#2563EB"
                                font.pixelSize: 12
                                font.bold: true
                                Layout.preferredWidth: 90
                            }

                            Text {
                                text: model.noteDate || ""
                                color: "#64748B"
                                font.pixelSize: 11
                                Layout.preferredWidth: 85
                            }

                            Rectangle {
                                Layout.preferredWidth: 95
                                height: 22
                                radius: 4
                                color: model.noteType === "Credit Note" ? "#FEE2E2" : "#DCFCE7"
                                border.color: model.noteType === "Credit Note" ? "#FCA5A5" : "#86EFAC"
                                Text {
                                    anchors.centerIn: parent
                                    text: model.noteType || ""
                                    color: model.noteType === "Credit Note" ? "#991B1B" : "#166534"
                                    font.pixelSize: 10
                                    font.bold: true
                                }
                            }

                            Text {
                                text: model.originalInvoiceNo || "-"
                                color: "#0F172A"
                                font.pixelSize: 11
                                Layout.preferredWidth: 90
                            }

                            Text {
                                text: model.partyName || "-"
                                color: "#0F172A"
                                font.pixelSize: 12
                                elide: Text.ElideRight
                                Layout.fillWidth: true
                            }

                            Text {
                                text: model.adjustmentType || "-"
                                color: "#475569"
                                font.pixelSize: 11
                                elide: Text.ElideRight
                                Layout.preferredWidth: 160
                            }

                            Text {
                                text: model.totalBags ? model.totalBags.toString() : "0"
                                color: "#334155"
                                font.pixelSize: 12
                                Layout.preferredWidth: 60
                                horizontalAlignment: Text.AlignRight
                            }

                            Text {
                                text: model.totalWeightQtl ? model.totalWeightQtl.toFixed(2) : "0.00"
                                color: "#334155"
                                font.pixelSize: 12
                                Layout.preferredWidth: 80
                                horizontalAlignment: Text.AlignRight
                            }

                            Text {
                                text: model.taxableAmountFmt || "Rs. 0.00"
                                color: "#0F172A"
                                font.pixelSize: 12
                                Layout.preferredWidth: 100
                                horizontalAlignment: Text.AlignRight
                            }

                            Text {
                                text: model.totalTaxAmountFmt || "Rs. 0.00"
                                color: "#2563EB"
                                font.pixelSize: 12
                                Layout.preferredWidth: 90
                                horizontalAlignment: Text.AlignRight
                            }

                            Text {
                                text: model.grandTotalFmt || "Rs. 0.00"
                                color: model.noteType === "Credit Note" ? "#DC2626" : "#16A34A"
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
                                    onClicked: root.loadNoteForEditing(model.noteId)
                                }

                                T.Button {
                                    implicitWidth: 36
                                    implicitHeight: 26
                                    background: Rectangle { color: "#FEF2F2"; radius: 4; border.color: "#FECACA" }
                                    contentItem: Text { text: "Del"; color: "#DC2626"; font.pixelSize: 11; font.bold: true; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                                    onClicked: {
                                        if (typeof debitCreditNoteCtrl !== "undefined") {
                                            debitCreditNoteCtrl.deleteNote(model.noteId)
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
            root.resetForm()
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
