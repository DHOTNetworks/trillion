import QtQuick
import QtQuick.Templates as T
import QtQuick.Layouts
import MahadevERP

Item {
    id: root

    signal cancelRequested()
    signal invoiceSaved()

    property int editingInvoiceId: 0
    readonly property bool isEditMode: editingInvoiceId > 0

    property string autoVoucherNo: ""
    property string autoVchCode: ""
    
    // Toggle for Without Stock Market Type
    readonly property bool isWithoutStock: marketTypeCombo.currentText.indexOf("Without Stock") !== -1
    readonly property bool isMandiType: marketTypeCombo.currentText.indexOf("Mandi") !== -1

    // Mandi Status Selections
    property string selectedSaleStatus: "Self Sale"
    property string selectedMarketFeeStatus: "Paid"

    // Mandi Charges Amounts
    property real damiAmount: 0.0
    property real labourAmount: 0.0
    property real auctionAmount: 0.0
    property real mFeeAmount: 0.0
    property real hrdfAmount: 0.0
    property real welfareAmount: 0.0
    property real dhrmdAmount: 0.0
    property real sutliAmount: 0.0

    // Aggregated Totals
    property int totalBags: 0
    property real totalWeight: 0.0
    property real taxableAmount: 0.0
    property real gstTaxAmount: 0.0
    property real otherExpAmount: 0.0
    property real lessAmount: 0.0
    property real freightAmount: 0.0
    property real roundOffAmount: 0.0
    property real tcsAmount: 0.0
    property real grandTotal: 0.0
    
    property string statusMessage: ""
    property bool isError: false
    property string selectedTaxStatus: "GST / Exempt"
    property bool isManualGst: false

    // Registered Items List Model
    GenericListModel {
        id: lineItemsModel
    }

    Component.onCompleted: {
        resetForm()
        Qt.callLater(function() {
            partyCombo.focusAndOpen()
        })
    }

    function resetForm() {
        editingInvoiceId = 0
        var nextInv = ""
        if (typeof purchaseModel !== "undefined" && purchaseModel) {
            autoVchCode = purchaseModel.get_next_voucher_no()
            autoVoucherNo = autoVchCode
            nextInv = purchaseModel.get_next_invoice_no()
        } else {
            autoVchCode = ""
            autoVoucherNo = ""
        }

        invNoInput.text = ""
        invNoInput.placeholderText = nextInv ? nextInv : "e.g. SMRI/25-26/328"
        dueDaysInput.text = "0"
        marketTypeCombo.currentIndex = 0
        posCombo.currentIndex = 0
        posCombo.editText = "Same as Buyer"
        
        partyCombo.currentIndex = -1
        partyCombo.editText = ""
        gstinInput.text = ""
        
        lineItemsModel.clear()
        clearItemInputRow()
        
        vehNoInput.text = ""
        grNoInput.text = ""
        driverInput.text = ""
        ewayInput.text = ""
        billTimeInput.text = ""
        saudaDtInput.text = ""
        shippingInput.text = ""
        poNoInput.text = ""
        gradeInput.text = ""
        transportInput.text = ""
        brokerInput.text = ""
        challanInput.text = ""
        kandaWeightInput.text = ""
        narrationInput.text = ""

        gstTaxInput.text = "0.00"
        otherExpInput.text = "0.00"
        lessInput.text = "0.00"
        freightInput.text = "0.00"
        tcsInput.text = "0.00"
        
        isManualGst = false
        statusMessage = ""
        isError = false
        recalculateTotals()
    }

    function loadInvoiceForEditing(invNoOrId) {
        if (typeof purchaseModel === "undefined" || !purchaseModel) return
        var inv = purchaseModel.get_purchase_invoice(invNoOrId)
        if (!inv || !inv.id) return

        editingInvoiceId = inv.id
        autoVchCode = inv.voucher_no || ("Purc-" + inv.id)
        autoVoucherNo = autoVchCode
        invNoInput.text = inv.invoice_no || ""
        if (inv.invoice_date) {
            var parts = String(inv.invoice_date).split("-")
            invoiceDateInput.text = parts.length === 3 ? (parts[2] + "-" + parts[1] + "-" + parts[0]) : inv.invoice_date
        }
        partyCombo.editText = inv.supplier_name || ""
        gstinInput.text = inv.gstin || ""
        vehNoInput.text = inv.vehicle_no || ""
        grNoInput.text = inv.gr_no || ""
        driverInput.text = inv.driver || inv.driver_name || ""
        ewayInput.text = inv.eway_bill_no || ""
        billTimeInput.text = inv.bill_time || ""
        saudaDtInput.text = inv.sauda_date || ""
        shippingInput.text = inv.shipping_address || ""
        poNoInput.text = inv.po_no || ""
        gradeInput.text = inv.grade || ""
        transportInput.text = inv.transport || ""
        brokerInput.text = inv.broker_name || ""
        kandaWeightInput.text = inv.kanda_weight || ""
        narrationInput.text = inv.narration || ""

        damiAmount = parseFloat(inv.dami) || 0.0
        labourAmount = parseFloat(inv.labour) || 0.0
        auctionAmount = parseFloat(inv.auction) || 0.0
        mFeeAmount = parseFloat(inv.m_fee) || 0.0
        hrdfAmount = parseFloat(inv.hrdf) || 0.0
        welfareAmount = parseFloat(inv.welfare) || 0.0
        dhrmdAmount = parseFloat(inv.dhrmd) || 0.0
        sutliAmount = parseFloat(inv.sutli) || 0.0

        otherExpInput.text = (parseFloat(inv.other_exp) || 0.0).toFixed(2)
        lessInput.text = (parseFloat(inv.less_amount) || 0.0).toFixed(2)
        
        lineItemsModel.clear()
        var items = inv.items || []
        for (var i = 0; i < items.length; i++) {
            var itm = items[i]
            var w = parseFloat(itm.weight || itm.weight_qtl || 0.0)
            var r = parseFloat(itm.rate || itm.rate_per_qtl || 0.0)
            var a = parseFloat(itm.amount || itm.total_amount || (w * r))
            lineItemsModel.append({
                itemName: itm.item_name || "",
                bags: parseInt(itm.bags || itm.bag_count) || 0,
                packing: (parseFloat(itm.packing) || 0.5).toFixed(3),
                weight: w,
                rate: r,
                gstPct: parseFloat(itm.gst_pct || 5.0),
                amount: a
            })
        }
        recalculateTotals()
    }

    function clearItemInputRow() {
        itemCombo.currentIndex = -1
        itemCombo.editText = ""
        bagsInput.text = ""
        pkngInput.text = "0.500"
        weightInput.text = ""
        gstInput.text = "5%"
        rateInput.text = ""
        amountInput.text = ""
    }

    function addCurrentItemRow() {
        var itemName = itemCombo.currentText.trim()
        var bCount = parseInt(bagsInput.text) || 0
        var weightVal = weightInput.text.trim() !== "" ? parseFloat(weightInput.text) : (parseFloat(weightInput.placeholderText) || 0.0)
        var rateVal = rateInput.text.trim() !== "" ? parseFloat(rateInput.text) : (parseFloat(rateInput.placeholderText) || 0.0)
        var gstStr = gstInput.text.replace("%", "").trim()
        var gstPct = parseFloat(gstStr) || 5.0
        var pkng = pkngInput.text.trim() !== "" ? pkngInput.text.trim() : "0.500"
        var userAmount = parseFloat(amountInput.text) || 0.0

        if (!itemName) {
            statusMessage = "❌ Please select an Item."
            isError = true
            return
        }

        if (!root.isWithoutStock && weightVal <= 0 && bCount <= 0) {
            statusMessage = "❌ Please enter valid Bags or Weight."
            isError = true
            return
        }

        var amount = userAmount > 0 ? userAmount : (root.isWithoutStock ? rateVal : Math.round(weightVal * rateVal * 100.0) / 100.0)

        lineItemsModel.append({
            "itemName": itemName,
            "bags": root.isWithoutStock ? 0 : bCount,
            "packing": root.isWithoutStock ? "" : pkng,
            "weight": root.isWithoutStock ? 0.0 : weightVal,
            "gstPct": gstPct,
            "rate": rateVal,
            "amount": amount
        })

        clearItemInputRow()
        statusMessage = ""
        isError = false
        recalculateTotals()
        Qt.callLater(function() {
            itemCombo.focusAndOpen()
        })
    }

    function removeLineItem(index) {
        if (index >= 0 && index < lineItemsModel.count) {
            lineItemsModel.remove(index)
            recalculateTotals()
        }
    }

    function recalculateTotals() {
        var sumBags = 0
        var sumWeight = 0.0
        var sumTaxable = 0.0
        var sumGst = 0.0

        for (var i = 0; i < lineItemsModel.count; i++) {
            var item = lineItemsModel.get(i)
            sumBags += item.bags
            sumWeight += item.weight
            sumTaxable += item.amount
            if (selectedTaxStatus !== "Export") {
                sumGst += Math.round(item.amount * (item.gstPct / 100.0) * 100.0) / 100.0
            }
        }

        totalBags = sumBags
        totalWeight = Math.round(sumWeight * 1000.0) / 1000.0
        taxableAmount = Math.round(sumTaxable * 100.0) / 100.0

        if (!isManualGst) {
            gstTaxAmount = Math.round(sumGst * 100.0) / 100.0
            gstTaxInput.text = gstTaxAmount > 0 ? gstTaxAmount.toFixed(2) : "0.00"
        } else {
            gstTaxAmount = parseFloat(gstTaxInput.text) || 0.0
        }

        if (root.isMandiType && typeof damiInput !== "undefined" && damiInput) {
            damiAmount = parseFloat(damiInput.text) || 0.0
            labourAmount = parseFloat(labourInput.text) || 0.0
            auctionAmount = parseFloat(auctionInput.text) || 0.0
            mFeeAmount = parseFloat(mFeeInput.text) || 0.0
            hrdfAmount = parseFloat(hrdfInput.text) || 0.0
            welfareAmount = parseFloat(welfareInput.text) || 0.0
            dhrmdAmount = parseFloat(dhrmdInput.text) || 0.0
            sutliAmount = parseFloat(sutliInput.text) || 0.0
            otherExpAmount = parseFloat(mandiOtherExpInput.text) || 0.0
            lessAmount = parseFloat(mandiLessInput.text) || 0.0
        } else {
            damiAmount = 0.0; labourAmount = 0.0; auctionAmount = 0.0; mFeeAmount = 0.0
            hrdfAmount = 0.0; welfareAmount = 0.0; dhrmdAmount = 0.0; sutliAmount = 0.0
            otherExpAmount = (typeof otherExpInput !== "undefined" && otherExpInput) ? (parseFloat(otherExpInput.text) || 0.0) : 0.0
            lessAmount = (typeof lessInput !== "undefined" && lessInput) ? (parseFloat(lessInput.text) || 0.0) : 0.0
        }

        freightAmount = (typeof freightInput !== "undefined" && freightInput) ? (parseFloat(freightInput.text) || 0.0) : 0.0
        tcsAmount = (typeof tcsInput !== "undefined" && tcsInput) ? (parseFloat(tcsInput.text) || 0.0) : 0.0

        var mandiChargesSum = damiAmount + labourAmount + auctionAmount + mFeeAmount + hrdfAmount + welfareAmount + dhrmdAmount + sutliAmount
        var gross = taxableAmount + mandiChargesSum + gstTaxAmount + otherExpAmount + freightAmount + tcsAmount - lessAmount
        var rounded = Math.round(gross)
        roundOffAmount = Math.round((rounded - gross) * 100.0) / 100.0
        grandTotal = rounded
    }

    function onPartySelected(partyName) {
        var cleanName = partyName ? partyName.trim() : ""
        if (!cleanName) {
            gstinInput.text = ""
            return
        }
        var party = (typeof partiesModel !== "undefined" && partiesModel) ? partiesModel.get_party_by_name(cleanName) : null
        if (party && party.gstin) {
            gstinInput.text = party.gstin
        } else {
            gstinInput.text = ""
        }
    }

    function onItemSelected(itemName) {
        if (!itemName) return
        var item = (typeof stockItemsModel !== "undefined" && stockItemsModel) ? stockItemsModel.get_item_by_name(itemName) : null
        if (item) {
            if (item.purchase_rate) rateInput.text = item.purchase_rate.toString()
            else if (item.sale_rate) rateInput.text = item.sale_rate.toString()
            if (item.gst_rate) gstInput.text = item.gst_rate
            if (item.packing_kg) {
                var pVal = parseFloat(item.packing_kg) || 50.0
                var pQtl = pVal > 2.0 ? pVal / 100.0 : pVal
                pkngInput.text = pQtl.toFixed(3)
            }
            isManualGst = false
            recalculateRowAmount(true)
        }
    }

    function recalculateRowAmount(forceRecalcWeight) {
        if (!weightInput || !bagsInput || !pkngInput || !amountInput || !rateInput) return
        if (root.isWithoutStock) {
            var rWithout = parseFloat(rateInput.text) || 0.0
            amountInput.text = rWithout > 0 ? rWithout.toFixed(2) : "0.00"
            return
        }
        var b = parseInt(bagsInput.text) || 0
        var pVal = parseFloat(pkngInput.text) || 0.500
        var pQtl = pVal > 2.0 ? pVal / 100.0 : pVal
        var autoWeight = Math.round((b * pQtl) * 1000.0) / 1000.0

        if (forceRecalcWeight || weightInput.text.trim() === "" || (bagsInput.activeFocus || pkngInput.activeFocus)) {
            weightInput.text = autoWeight > 0 ? autoWeight.toFixed(3) : (b > 0 ? "0.000" : "")
        }

        var w = parseFloat(weightInput.text) || autoWeight
        var r = parseFloat(rateInput.text) || 0.0
        var a = Math.round(w * r * 100.0) / 100.0
        amountInput.text = a > 0 ? a.toFixed(2) : "0.00"
    }

    function saveInvoice() {
        statusMessage = ""
        var partyLedger = partyCombo.currentText.trim()

        if (!partyLedger) {
            statusMessage = "❌ Please select a Supplier / Party Ledger Account."
            isError = true
            return
        }

        if (lineItemsModel.count === 0 && itemCombo.currentText.trim() !== "") {
            addCurrentItemRow()
        }

        if (lineItemsModel.count === 0) {
            statusMessage = "❌ Please enter at least one Stock Item in the grid."
            isError = true
            return
        }

        saveConfirmModal.open()
    }

    function executeSaveInvoice() {
        var partyLedger = partyCombo.currentText.trim()
        var invNo = invNoInput.text.trim()
        if (!invNo && typeof purchaseModel !== "undefined" && purchaseModel) {
            invNo = purchaseModel.get_next_invoice_no()
        }
        var dParts = invoiceDateInput.text.trim().split("-")
        var invDate = dParts.length === 3 ? (dParts[2] + "-" + dParts[1] + "-" + dParts[0]) : Qt.formatDate(new Date(), "yyyy-MM-dd")
        var vehicle = vehNoInput.text.trim()
        var eway = ewayInput.text.trim()
        var narr = narrationInput.text.trim()

        var firstItem = lineItemsModel.get(0)
        var mainItemName = firstItem ? firstItem.itemName : ""

        var cgstVal = selectedTaxStatus === "IGST" ? 0.0 : gstTaxAmount / 2.0
        var sgstVal = selectedTaxStatus === "IGST" ? 0.0 : gstTaxAmount / 2.0
        var igstVal = selectedTaxStatus === "IGST" ? gstTaxAmount : 0.0

        var itemsList = []
        for (var i = 0; i < lineItemsModel.count; i++) {
            var it = lineItemsModel.get(i)
            itemsList.push({
                item_name: it.itemName,
                bags: it.bags,
                packing: parseFloat(it.packing) || 0.5,
                weight: it.weight,
                rate: it.rate,
                gst_pct: it.gstPct,
                amount: it.amount
            })
        }

        if (typeof purchaseModel !== "undefined" && purchaseModel) {
            var ok = false
            if (root.editingInvoiceId > 0) {
                ok = purchaseModel.update_purchase_invoice_full(
                    root.editingInvoiceId,
                    invNo, invDate, partyLedger, gstinInput.text.trim(), mainItemName, "", totalBags, totalWeight, 0.0,
                    taxableAmount, 5.0, cgstVal, sgstVal, igstVal, roundOffAmount, grandTotal,
                    "Credit", vehicle, eway, narr,
                    selectedSaleStatus, selectedMarketFeeStatus, damiAmount, labourAmount, auctionAmount, mFeeAmount, hrdfAmount, otherExpAmount, welfareAmount, dhrmdAmount, sutliAmount, lessAmount,
                    grNoInput.text.trim(), driverInput.text.trim(), billTimeInput.text.trim(), saudaDtInput.text.trim(), shippingInput.text.trim(), poNoInput.text.trim(), gradeInput.text.trim(), kandaWeightInput.text.trim(), transportInput.text.trim(), brokerInput.text.trim(),
                    autoVoucherNo, itemsList
                )
            } else {
                ok = purchaseModel.add_purchase_invoice_full(
                    invNo, invDate, partyLedger, gstinInput.text.trim(), mainItemName, "", totalBags, totalWeight, 0.0,
                    taxableAmount, 5.0, cgstVal, sgstVal, igstVal, roundOffAmount, grandTotal,
                    "Credit", vehicle, eway, narr,
                    selectedSaleStatus, selectedMarketFeeStatus, damiAmount, labourAmount, auctionAmount, mFeeAmount, hrdfAmount, otherExpAmount, welfareAmount, dhrmdAmount, sutliAmount, lessAmount,
                    grNoInput.text.trim(), driverInput.text.trim(), billTimeInput.text.trim(), saudaDtInput.text.trim(), shippingInput.text.trim(), poNoInput.text.trim(), gradeInput.text.trim(), kandaWeightInput.text.trim(), transportInput.text.trim(), brokerInput.text.trim(),
                    autoVoucherNo
                )
            }
            if (ok) {
                statusMessage = root.editingInvoiceId > 0 ? ("✅ Purchase Voucher " + invNo + " updated successfully!") : ("✅ Purchase Voucher " + (invNo ? invNo : autoVchCode) + " saved & posted successfully!")
                isError = false
                resetForm()
                root.invoiceSaved()
            } else {
                statusMessage = "❌ Failed to save Purchase Voucher."
                isError = true
            }
        }
    }

    function hasActivePopup() {
        return saveConfirmModal.opened
    }

    function closeActivePopup() {
        if (saveConfirmModal.opened) saveConfirmModal.close()
    }

    ConfirmationModal {
        id: saveConfirmModal
        anchors.centerIn: parent
        titleText: "CONFIRM PURCHASE VOUCHER SAVE"
        messageText: "Are you sure you want to save & post Purchase Voucher " + (invNoInput.text.trim() || (typeof purchaseModel !== "undefined" ? purchaseModel.get_next_invoice_no() : autoVchCode)) + " for ₹" + grandTotal.toFixed(2) + "?"
        onConfirmed: root.executeSaveInvoice()
    }

    // MAIN SINGLE SLATE CARD CONTAINER
    Rectangle {
        anchors.fill: parent
        color: "#FFFFFF"
        border.color: "#CBD5E1"
        border.width: 1
        radius: 8

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 10
            spacing: 6

            // 1. TOP TITLE HEADER BAR
            RowLayout {
                Layout.fillWidth: true
                spacing: 10

                ColumnLayout {
                    spacing: 0
                    Text {
                        text: "Purchase Voucher Entry (F9)"
                        color: "#0F172A"
                        font.pixelSize: 18
                        font.bold: true
                    }
                    Text {
                        text: "In-grid accounting spreadsheet entry with continuous Enter key navigation & automatic row creation."
                        color: "#64748B"
                        font.pixelSize: 11
                    }
                }

                Item { Layout.fillWidth: true }

                T.Button {
                    id: backBtn
                    implicitWidth: contentItem.implicitWidth + 24
                    implicitHeight: 30
                    background: Rectangle { color: "#F1F5F9"; radius: 6; border.color: "#CBD5E1" }
                    contentItem: RowLayout {
                        spacing: 6
                        Text { text: "← Back to Dashboard"; color: "#475569"; font.pixelSize: 11; font.bold: true }
                        KbdBadge { text: "Esc"; badgeColor: "#DC2626"; textColor: "#FFF"; borderColor: "#B91C1C" }
                    }
                    onClicked: root.cancelRequested()
                }
            }

            // Status Notification Banner (if active)
            Rectangle {
                Layout.fillWidth: true
                height: 24
                color: isError ? "#FEF2F2" : "#F0FDF4"
                border.color: isError ? "#FCA5A5" : "#86EFAC"
                border.width: 1
                radius: 4
                visible: statusMessage !== ""

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 10; anchors.rightMargin: 10
                    Text { text: root.statusMessage; color: isError ? "#991B1B" : "#166534"; font.pixelSize: 11; font.bold: true }
                }
            }

            // 2. VOUCHER & PARTY CONTROLS SECTION
            ColumnLayout {
                Layout.fillWidth: true
                spacing: 4

                // Row 1: Market Type, Voucher No, Invoice No, Date, Due Days, Tax Status Badges
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 8

                    ColumnLayout {
                        spacing: 1
                        Text { text: "Market Type"; color: "#475569"; font.pixelSize: 10; font.bold: true }
                        CustomWhiteCombo {
                            id: marketTypeCombo
                            model: ["Market Type (With Stock)", "Mandi Type", "Market Type (Without Stock)"]
                            Layout.preferredWidth: 190
                            onReturnPressed: invNoInput.focusInput = true
                            onRightPressed: invNoInput.focusInput = true
                        }
                    }

                    ColumnLayout {
                        spacing: 1
                        Text { text: "Voucher No (Auto)"; color: "#64748B"; font.pixelSize: 10; font.bold: true }
                        Rectangle {
                            implicitWidth: 110
                            implicitHeight: 34
                            color: "#F1F5F9"
                            border.color: "#CBD5E1"
                            radius: 6
                            RowLayout {
                                anchors.fill: parent
                                anchors.leftMargin: 8; anchors.rightMargin: 8
                                Text { text: root.autoVchCode; color: "#16A34A"; font.pixelSize: 11; font.bold: true }
                                Item { Layout.fillWidth: true }
                                Text { text: "🔒"; font.pixelSize: 9 }
                            }
                        }
                    }

                    ColumnLayout {
                        spacing: 1
                        Text { text: "Purchase Bill No (Editable)"; color: "#0F172A"; font.pixelSize: 10; font.bold: true }
                        CustomInput {
                            id: invNoInput
                            placeholderText: "PUR-1"
                            Layout.preferredWidth: 140
                            onReturnPressed: invoiceDateInput.focusInput = true
                            onRightPressed: invoiceDateInput.focusInput = true
                            onLeftPressed: marketTypeCombo.focusAndOpen()
                        }
                    }

                    ColumnLayout {
                        spacing: 1
                        Text { text: "Bill Date"; color: "#0F172A"; font.pixelSize: 10; font.bold: true }
                        CustomInput {
                            id: invoiceDateInput
                            text: Qt.formatDate(new Date(), "dd-MM-yyyy")
                            placeholderText: "DD-MM-YYYY"
                            Layout.preferredWidth: 110
                            onReturnPressed: dueDaysInput.focusInput = true
                            onRightPressed: dueDaysInput.focusInput = true
                            onLeftPressed: invNoInput.focusInput = true
                        }
                    }

                    ColumnLayout {
                        spacing: 1
                        Text { text: "Due Days"; color: "#475569"; font.pixelSize: 10; font.bold: true }
                        CustomInput {
                            id: dueDaysInput
                            placeholderText: "0"
                            Layout.preferredWidth: 60
                            onReturnPressed: partyCombo.focusAndOpen()
                            onRightPressed: partyCombo.focusAndOpen()
                            onLeftPressed: invoiceDateInput.focusInput = true
                        }
                    }

                    Item { Layout.fillWidth: true }

                    // Sale Status Badge Selectors (Visible in Mandi Type)
                    ColumnLayout {
                        spacing: 1
                        visible: root.isMandiType
                        Text { text: "Sale Status : (Alt+S)"; color: "#D97706"; font.pixelSize: 10; font.bold: true }
                        RowLayout {
                            spacing: 4

                            Rectangle {
                                width: 70; height: 28; radius: 5
                                color: root.selectedSaleStatus === "Self Sale" ? "#D97706" : "#F1F5F9"
                                border.color: root.selectedSaleStatus === "Self Sale" ? "#B45309" : "#CBD5E1"
                                Text { anchors.centerIn: parent; text: "Self Sale"; color: root.selectedSaleStatus === "Self Sale" ? "#FFF" : "#475569"; font.pixelSize: 10; font.bold: true }
                                MouseArea { anchors.fill: parent; onClicked: root.selectedSaleStatus = "Self Sale" }
                            }

                            Rectangle {
                                width: 90; height: 28; radius: 5
                                color: root.selectedSaleStatus === "Stock Transfer" ? "#D97706" : "#F1F5F9"
                                border.color: root.selectedSaleStatus === "Stock Transfer" ? "#B45309" : "#CBD5E1"
                                Text { anchors.centerIn: parent; text: "Stock Transfer"; color: root.selectedSaleStatus === "Stock Transfer" ? "#FFF" : "#475569"; font.pixelSize: 10; font.bold: true }
                                MouseArea { anchors.fill: parent; onClicked: root.selectedSaleStatus = "Stock Transfer" }
                            }

                            Rectangle {
                                width: 70; height: 28; radius: 5
                                color: root.selectedSaleStatus === "Lagat Bill" ? "#D97706" : "#F1F5F9"
                                border.color: root.selectedSaleStatus === "Lagat Bill" ? "#B45309" : "#CBD5E1"
                                Text { anchors.centerIn: parent; text: "Lagat Bill"; color: root.selectedSaleStatus === "Lagat Bill" ? "#FFF" : "#475569"; font.pixelSize: 10; font.bold: true }
                                MouseArea { anchors.fill: parent; onClicked: root.selectedSaleStatus = "Lagat Bill" }
                            }

                            Rectangle {
                                width: 75; height: 28; radius: 5
                                color: root.selectedSaleStatus === "Third Party" ? "#D97706" : "#F1F5F9"
                                border.color: root.selectedSaleStatus === "Third Party" ? "#B45309" : "#CBD5E1"
                                Text { anchors.centerIn: parent; text: "Third Party"; color: root.selectedSaleStatus === "Third Party" ? "#FFF" : "#475569"; font.pixelSize: 10; font.bold: true }
                                MouseArea { anchors.fill: parent; onClicked: root.selectedSaleStatus = "Third Party" }
                            }
                        }
                    }

                    // Market Fee Status Badge Selectors (Visible in Mandi Type)
                    ColumnLayout {
                        spacing: 1
                        visible: root.isMandiType
                        Text { text: "Market Fee Status : (Alt+F)"; color: "#16A34A"; font.pixelSize: 10; font.bold: true }
                        RowLayout {
                            spacing: 4

                            Rectangle {
                                width: 65; height: 28; radius: 5
                                color: root.selectedMarketFeeStatus === "Payable" ? "#16A34A" : "#F1F5F9"
                                border.color: root.selectedMarketFeeStatus === "Payable" ? "#15803D" : "#CBD5E1"
                                Text { anchors.centerIn: parent; text: "Payable"; color: root.selectedMarketFeeStatus === "Payable" ? "#FFF" : "#475569"; font.pixelSize: 10; font.bold: true }
                                MouseArea { anchors.fill: parent; onClicked: root.selectedMarketFeeStatus = "Payable" }
                            }

                            Rectangle {
                                width: 55; height: 28; radius: 5
                                color: root.selectedMarketFeeStatus === "Paid" ? "#16A34A" : "#F1F5F9"
                                border.color: root.selectedMarketFeeStatus === "Paid" ? "#15803D" : "#CBD5E1"
                                Text { anchors.centerIn: parent; text: "Paid"; color: root.selectedMarketFeeStatus === "Paid" ? "#FFF" : "#475569"; font.pixelSize: 10; font.bold: true }
                                MouseArea { anchors.fill: parent; onClicked: root.selectedMarketFeeStatus = "Paid" }
                            }
                        }
                    }

                    // Tax Status Badge Selectors
                    ColumnLayout {
                        spacing: 1
                        Text { text: "Tax Status : (Alt+R / Alt+T)"; color: "#16A34A"; font.pixelSize: 10; font.bold: true }
                        RowLayout {
                            spacing: 4

                            Rectangle {
                                width: 90; height: 28; radius: 5
                                color: root.selectedTaxStatus === "GST / Exempt" ? "#16A34A" : "#F1F5F9"
                                border.color: root.selectedTaxStatus === "GST / Exempt" ? "#15803D" : "#CBD5E1"
                                Text { anchors.centerIn: parent; text: "GST / Exempt"; color: root.selectedTaxStatus === "GST / Exempt" ? "#FFF" : "#475569"; font.pixelSize: 10; font.bold: true }
                                MouseArea { anchors.fill: parent; onClicked: { root.selectedTaxStatus = "GST / Exempt"; root.isManualGst = false; root.recalculateTotals() } }
                            }

                            Rectangle {
                                width: 55; height: 28; radius: 5
                                color: root.selectedTaxStatus === "IGST" ? "#16A34A" : "#F1F5F9"
                                border.color: root.selectedTaxStatus === "IGST" ? "#15803D" : "#CBD5E1"
                                Text { anchors.centerIn: parent; text: "IGST"; color: root.selectedTaxStatus === "IGST" ? "#FFF" : "#475569"; font.pixelSize: 10; font.bold: true }
                                MouseArea { anchors.fill: parent; onClicked: { root.selectedTaxStatus = "IGST"; root.isManualGst = false; root.recalculateTotals() } }
                            }

                            Rectangle {
                                width: 60; height: 28; radius: 5
                                color: root.selectedTaxStatus === "Export" ? "#16A34A" : "#F1F5F9"
                                border.color: root.selectedTaxStatus === "Export" ? "#15803D" : "#CBD5E1"
                                Text { anchors.centerIn: parent; text: "Export"; color: root.selectedTaxStatus === "Export" ? "#FFF" : "#475569"; font.pixelSize: 10; font.bold: true }
                                MouseArea { anchors.fill: parent; onClicked: { root.selectedTaxStatus = "Export"; root.isManualGst = false; root.recalculateTotals() } }
                            }
                        }
                    }
                }

                // Row 2: Purchase From Party Ledger Account * & Party GSTIN
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 8

                    ColumnLayout {
                        spacing: 1
                        Layout.fillWidth: true
                        Layout.preferredWidth: 600
                        Text { text: "Purchase From Party / Supplier Ledger Account *"; color: "#0F172A"; font.pixelSize: 10; font.bold: true }
                        CustomWhiteCombo {
                            id: partyCombo
                            model: (typeof partiesModel !== "undefined" && partiesModel) ? partiesModel.get_parties_list() : []
                            Layout.fillWidth: true
                            onCurrentTextChanged: root.onPartySelected(currentText)
                            onReturnPressed: gstinInput.focusInput = true
                            onRightPressed: gstinInput.focusInput = true
                            onLeftPressed: dueDaysInput.focusInput = true
                        }
                    }

                    ColumnLayout {
                        spacing: 1
                        Layout.preferredWidth: 240
                        Text { text: "Supplier GSTIN"; color: "#475569"; font.pixelSize: 10; font.bold: true }
                        CustomInput {
                            id: gstinInput
                            placeholderText: "29AAAAA0000A1Z5"
                            Layout.fillWidth: true
                            Layout.preferredWidth: 240
                            onReturnPressed: itemCombo.focusAndOpen()
                            onRightPressed: itemCombo.focusAndOpen()
                            onLeftPressed: partyCombo.focusAndOpen()
                        }
                    }
                }
            }

            // 3. IN-GRID ITEM ENTRY TABLE (SUPPORTING WITH-STOCK & WITHOUT-STOCK DYNAMIC COLUMNS)
            Rectangle {
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.minimumHeight: 180
                color: "#FFFFFF"
                border.color: "#CBD5E1"
                border.width: 1
                radius: 6

                ColumnLayout {
                    anchors.fill: parent
                    spacing: 0

                    // 3A. HEADER ROW (DYNAMICALLY HIDES BAGS/PKNG/WEIGHT FOR WITHOUT STOCK)
                    Rectangle {
                        Layout.fillWidth: true
                        height: 28
                        color: "#E2E8F0"

                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: 8; anchors.rightMargin: 8
                            spacing: 6

                            Item { Layout.preferredWidth: 30; Text { anchors.verticalCenter: parent.verticalCenter; text: "No."; color: "#0F172A"; font.pixelSize: 11; font.bold: true } }
                            Item { Layout.fillWidth: true; Layout.preferredWidth: 240; Text { anchors.verticalCenter: parent.verticalCenter; text: "Item Name *"; color: "#0F172A"; font.pixelSize: 11; font.bold: true } }
                            
                            Item { visible: !root.isWithoutStock; Layout.preferredWidth: 60; Text { anchors.right: parent.right; anchors.verticalCenter: parent.verticalCenter; text: "Bags"; color: "#0F172A"; font.pixelSize: 11; font.bold: true } }
                            Item { visible: !root.isWithoutStock; Layout.preferredWidth: 70; Text { anchors.centerIn: parent; text: "Pkng."; color: "#0F172A"; font.pixelSize: 11; font.bold: true } }
                            Item { visible: !root.isWithoutStock; Layout.preferredWidth: 90; Text { anchors.right: parent.right; anchors.verticalCenter: parent.verticalCenter; text: "Weight (Qtl)"; color: "#0F172A"; font.pixelSize: 11; font.bold: true } }
                            
                            Item { Layout.preferredWidth: 50; Text { anchors.right: parent.right; anchors.verticalCenter: parent.verticalCenter; text: "GST %"; color: "#0F172A"; font.pixelSize: 11; font.bold: true } }
                            Item { Layout.preferredWidth: 95; Text { anchors.right: parent.right; anchors.verticalCenter: parent.verticalCenter; text: "Rate (₹)"; color: "#0F172A"; font.pixelSize: 11; font.bold: true } }
                            Item { Layout.preferredWidth: 110; Text { anchors.right: parent.right; anchors.verticalCenter: parent.verticalCenter; text: "Amount (₹)"; color: "#0F172A"; font.pixelSize: 11; font.bold: true } }
                            Item { Layout.preferredWidth: 35; Text { anchors.centerIn: parent; text: "Act"; color: "#0F172A"; font.pixelSize: 11; font.bold: true } }
                        }
                    }

                    // 3B. LISTVIEW FOR COMMITTED REGISTERED ITEMS
                    ListView {
                        id: itemsListView
                        Layout.fillWidth: true
                        implicitHeight: contentHeight
                        clip: true
                        model: lineItemsModel
                        delegate: Rectangle {
                            width: itemsListView.width
                            height: 32
                            color: index % 2 === 0 ? "#FFFFFF" : "#F8FAFC"
                            border.color: "#E2E8F0"

                            RowLayout {
                                anchors.fill: parent
                                anchors.leftMargin: 8; anchors.rightMargin: 8
                                spacing: 6

                                Item { Layout.preferredWidth: 30; Text { anchors.verticalCenter: parent.verticalCenter; text: (index + 1) + "."; color: "#0F172A"; font.pixelSize: 12; font.bold: true } }
                                Item { Layout.fillWidth: true; Layout.preferredWidth: 240; Text { anchors.verticalCenter: parent.verticalCenter; text: model.itemName; color: "#0F172A"; font.pixelSize: 12; font.bold: true; elide: Text.ElideRight } }
                                
                                Item { visible: !root.isWithoutStock; Layout.preferredWidth: 60; Text { anchors.right: parent.right; anchors.verticalCenter: parent.verticalCenter; text: model.bags > 0 ? model.bags.toString() : ""; color: "#0F172A"; font.pixelSize: 12 } }
                                Item { visible: !root.isWithoutStock; Layout.preferredWidth: 70; Text { anchors.centerIn: parent; text: model.packing; color: "#475569"; font.pixelSize: 12 } }
                                Item { visible: !root.isWithoutStock; Layout.preferredWidth: 90; Text { anchors.right: parent.right; anchors.verticalCenter: parent.verticalCenter; text: model.weight > 0 ? model.weight.toFixed(3) : ""; color: "#0F172A"; font.pixelSize: 12; font.bold: true } }
                                
                                Item { Layout.preferredWidth: 50; Text { anchors.right: parent.right; anchors.verticalCenter: parent.verticalCenter; text: model.gstPct + "%"; color: "#475569"; font.pixelSize: 12 } }
                                Item { Layout.preferredWidth: 95; Text { anchors.right: parent.right; anchors.verticalCenter: parent.verticalCenter; text: model.rate > 0 ? model.rate.toFixed(2) : ""; color: "#0F172A"; font.pixelSize: 12; font.bold: true } }
                                Item { Layout.preferredWidth: 110; Text { anchors.right: parent.right; anchors.verticalCenter: parent.verticalCenter; text: "₹" + model.amount.toFixed(2); color: "#16A34A"; font.pixelSize: 12; font.bold: true } }
                                
                                Item {
                                    Layout.preferredWidth: 35
                                    T.Button {
                                        anchors.centerIn: parent
                                        implicitWidth: 28
                                        implicitHeight: 20
                                        width: 28; height: 20
                                        background: Rectangle { color: "#FEE2E2"; radius: 4 }
                                        contentItem: Text { text: "✕"; color: "#DC2626"; font.bold: true; font.pixelSize: 10; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                                        onClicked: root.removeLineItem(index)
                                    }
                                }
                            }
                        }
                    }

                    // 3C. ACTIVE IN-GRID ENTRY ROW
                    Rectangle {
                        Layout.fillWidth: true
                        height: 36
                        color: "#F0FDF4"
                        border.color: "#86EFAC"
                        border.width: 1

                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: 8; anchors.rightMargin: 8
                            spacing: 6

                            Item { Layout.preferredWidth: 30; Text { anchors.verticalCenter: parent.verticalCenter; text: (lineItemsModel.count + 1) + "."; color: "#16A34A"; font.pixelSize: 12; font.bold: true } }

                            CustomWhiteCombo {
                                id: itemCombo
                                model: (typeof stockItemsModel !== "undefined" && stockItemsModel) ? stockItemsModel.get_items_list(root.isMandiType ? "Mandi" : "Market") : []
                                Layout.fillWidth: true
                                Layout.preferredWidth: 240
                                onCurrentTextChanged: root.onItemSelected(currentText)
                                onReturnPressed: {
                                    if (itemCombo.currentText.trim() === "") {
                                        vehNoInput.focusInput = true
                                    } else if (root.isWithoutStock) {
                                        gstInput.focusInput = true
                                    } else {
                                        bagsInput.focusInput = true
                                    }
                                }
                                onRightPressed: {
                                    if (itemCombo.currentText.trim() === "") {
                                        vehNoInput.focusInput = true
                                    } else if (root.isWithoutStock) {
                                        gstInput.focusInput = true
                                    } else {
                                        bagsInput.focusInput = true
                                    }
                                }
                                onLeftPressed: gstinInput.focusInput = true
                            }

                            CustomInput {
                                id: bagsInput
                                visible: !root.isWithoutStock
                                placeholderText: "100"
                                Layout.preferredWidth: 60
                                onTextChanged: root.recalculateRowAmount(true)
                                onReturnPressed: pkngInput.focusInput = true
                                onRightPressed: pkngInput.focusInput = true
                                onLeftPressed: itemCombo.focusAndOpen()
                            }

                            CustomInput {
                                id: pkngInput
                                visible: !root.isWithoutStock
                                text: "0.500"
                                placeholderText: "0.500"
                                Layout.preferredWidth: 70
                                onTextChanged: root.recalculateRowAmount(true)
                                onReturnPressed: weightInput.focusInput = true
                                onRightPressed: weightInput.focusInput = true
                                onLeftPressed: bagsInput.focusInput = true
                            }

                            CustomInput {
                                id: weightInput
                                visible: !root.isWithoutStock
                                placeholderText: "50.000"
                                Layout.preferredWidth: 90
                                onTextChanged: root.recalculateRowAmount(false)
                                onReturnPressed: gstInput.focusInput = true
                                onRightPressed: gstInput.focusInput = true
                                onLeftPressed: pkngInput.focusInput = true
                            }

                            CustomInput {
                                id: gstInput
                                text: "5%"
                                placeholderText: "5%"
                                Layout.preferredWidth: 50
                                onReturnPressed: rateInput.focusInput = true
                                onRightPressed: rateInput.focusInput = true
                                onLeftPressed: root.isWithoutStock ? itemCombo.focusAndOpen() : weightInput.focusInput = true
                            }

                            CustomInput {
                                id: rateInput
                                placeholderText: "2800"
                                Layout.preferredWidth: 95
                                onTextChanged: root.recalculateRowAmount(false)
                                onReturnPressed: amountInput.focusInput = true
                                onRightPressed: amountInput.focusInput = true
                                onLeftPressed: gstInput.focusInput = true
                            }

                            CustomInput {
                                id: amountInput
                                text: "0.00"
                                placeholderText: "0.00"
                                Layout.preferredWidth: 110
                                onReturnPressed: root.addCurrentItemRow()
                                onRightPressed: root.addCurrentItemRow()
                                onLeftPressed: rateInput.focusInput = true
                            }

                            Item {
                                Layout.preferredWidth: 35
                                T.Button {
                                    anchors.centerIn: parent
                                    implicitWidth: 28
                                    implicitHeight: 22
                                    width: 28; height: 22
                                    background: Rectangle { color: "#16A34A"; radius: 4 }
                                    contentItem: Text { text: "+"; color: "#FFF"; font.bold: true; font.pixelSize: 12; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                                    onClicked: root.addCurrentItemRow()
                                }
                            }
                        }
                    }

                    // EMPTY FLEX FILLER (PUSHES SUMMARY TOTAL BAR TO BOTTOM)
                    Item { Layout.fillHeight: true }

                    // 3D. BAHI-KHATA STYLE SUMMARY TOTAL BAR
                    Rectangle {
                        Layout.fillWidth: true
                        height: 28
                        color: "#FED7AA"

                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: 10; anchors.rightMargin: 10
                            spacing: 12

                            Text { text: "Total :"; color: "#0F172A"; font.pixelSize: 12; font.bold: true }
                            Item { Layout.fillWidth: true }
                            
                            Text { visible: !root.isWithoutStock; text: root.totalBags + " Bags"; color: "#0F172A"; font.pixelSize: 12; font.bold: true }
                            Item { visible: !root.isWithoutStock; implicitWidth: 16 }
                            
                            Text { visible: !root.isWithoutStock; text: root.totalWeight.toFixed(3) + " Qtl."; color: "#0F172A"; font.pixelSize: 12; font.bold: true }
                            Item { visible: !root.isWithoutStock; implicitWidth: 16 }

                            Text { text: (typeof dashboardCtrl !== "undefined" && dashboardCtrl) ? dashboardCtrl.format_inr(root.taxableAmount) : ("₹" + root.taxableAmount.toFixed(2)); color: "#9A3412"; font.pixelSize: 13; font.bold: true }
                        }
                    }

                    // 10 MANDI EXPENSES CHARGES BAR (VISIBLE ONLY WHEN MANDI TYPE IS SELECTED)
                    Rectangle {
                        Layout.fillWidth: true
                        implicitHeight: 76
                        color: "#FFF7ED"
                        border.color: "#FDBA74"
                        border.width: 1
                        radius: 6
                        visible: root.isMandiType

                        ColumnLayout {
                            anchors.fill: parent
                            anchors.margins: 6
                            spacing: 6

                            // Row 1: Dami, Labour, Auction, M. Fee, H.R.D.F.
                            RowLayout {
                                Layout.fillWidth: true
                                spacing: 8

                                RowLayout {
                                    spacing: 4
                                    Text { text: "Dami :"; color: "#9A3412"; font.pixelSize: 11; font.bold: true; font.italic: true }
                                    CustomInput {
                                        id: damiInput
                                        placeholderText: "0.00"
                                        Layout.preferredWidth: 80
                                        onTextChanged: root.recalculateTotals()
                                        onReturnPressed: labourInput.focusInput = true
                                        onRightPressed: labourInput.focusInput = true
                                    }
                                }

                                RowLayout {
                                    spacing: 4
                                    Text { text: "Labour :"; color: "#9A3412"; font.pixelSize: 11; font.bold: true; font.italic: true }
                                    CustomInput {
                                        id: labourInput
                                        placeholderText: "0.00"
                                        Layout.preferredWidth: 80
                                        onTextChanged: root.recalculateTotals()
                                        onReturnPressed: auctionInput.focusInput = true
                                        onRightPressed: auctionInput.focusInput = true
                                        onLeftPressed: damiInput.focusInput = true
                                    }
                                }

                                RowLayout {
                                    spacing: 4
                                    Text { text: "Auction :"; color: "#9A3412"; font.pixelSize: 11; font.bold: true; font.italic: true }
                                    CustomInput {
                                        id: auctionInput
                                        placeholderText: "0.00"
                                        Layout.preferredWidth: 80
                                        onTextChanged: root.recalculateTotals()
                                        onReturnPressed: mFeeInput.focusInput = true
                                        onRightPressed: mFeeInput.focusInput = true
                                        onLeftPressed: labourInput.focusInput = true
                                    }
                                }

                                RowLayout {
                                    spacing: 4
                                    Text { text: "M. Fee :"; color: "#9A3412"; font.pixelSize: 11; font.bold: true; font.italic: true }
                                    CustomInput {
                                        id: mFeeInput
                                        placeholderText: "0.00"
                                        Layout.preferredWidth: 80
                                        onTextChanged: root.recalculateTotals()
                                        onReturnPressed: hrdfInput.focusInput = true
                                        onRightPressed: hrdfInput.focusInput = true
                                        onLeftPressed: auctionInput.focusInput = true
                                    }
                                }

                                RowLayout {
                                    spacing: 4
                                    Text { text: "H.R.D.F. :"; color: "#9A3412"; font.pixelSize: 11; font.bold: true; font.italic: true }
                                    CustomInput {
                                        id: hrdfInput
                                        placeholderText: "0.00"
                                        Layout.preferredWidth: 80
                                        onTextChanged: root.recalculateTotals()
                                        onReturnPressed: mandiOtherExpInput.focusInput = true
                                        onRightPressed: mandiOtherExpInput.focusInput = true
                                        onLeftPressed: mFeeInput.focusInput = true
                                    }
                                }
                            }

                            // Row 2: Other Exp., Welfare, Dhrmd., Sutli, (-) Less
                            RowLayout {
                                Layout.fillWidth: true
                                spacing: 8

                                RowLayout {
                                    spacing: 4
                                    Text { text: "Other Exp. :"; color: "#9A3412"; font.pixelSize: 11; font.bold: true; font.italic: true }
                                    CustomInput {
                                        id: mandiOtherExpInput
                                        placeholderText: "0.00"
                                        Layout.preferredWidth: 80
                                        onTextChanged: root.recalculateTotals()
                                        onReturnPressed: welfareInput.focusInput = true
                                        onRightPressed: welfareInput.focusInput = true
                                        onLeftPressed: hrdfInput.focusInput = true
                                    }
                                }

                                RowLayout {
                                    spacing: 4
                                    Text { text: "Welfare :"; color: "#9A3412"; font.pixelSize: 11; font.bold: true; font.italic: true }
                                    CustomInput {
                                        id: welfareInput
                                        placeholderText: "0.00"
                                        Layout.preferredWidth: 80
                                        onTextChanged: root.recalculateTotals()
                                        onReturnPressed: dhrmdInput.focusInput = true
                                        onRightPressed: dhrmdInput.focusInput = true
                                        onLeftPressed: mandiOtherExpInput.focusInput = true
                                    }
                                }

                                RowLayout {
                                    spacing: 4
                                    Text { text: "Dhrmd. :"; color: "#9A3412"; font.pixelSize: 11; font.bold: true; font.italic: true }
                                    CustomInput {
                                        id: dhrmdInput
                                        placeholderText: "0.00"
                                        Layout.preferredWidth: 80
                                        onTextChanged: root.recalculateTotals()
                                        onReturnPressed: sutliInput.focusInput = true
                                        onRightPressed: sutliInput.focusInput = true
                                        onLeftPressed: welfareInput.focusInput = true
                                    }
                                }

                                RowLayout {
                                    spacing: 4
                                    Text { text: "Sutli :"; color: "#9A3412"; font.pixelSize: 11; font.bold: true; font.italic: true }
                                    CustomInput {
                                        id: sutliInput
                                        placeholderText: "0.00"
                                        Layout.preferredWidth: 80
                                        onTextChanged: root.recalculateTotals()
                                        onReturnPressed: mandiLessInput.focusInput = true
                                        onRightPressed: mandiLessInput.focusInput = true
                                        onLeftPressed: dhrmdInput.focusInput = true
                                    }
                                }

                                RowLayout {
                                    spacing: 4
                                    Text { text: "(-) Less :"; color: "#9A3412"; font.pixelSize: 11; font.bold: true; font.italic: true }
                                    CustomInput {
                                        id: mandiLessInput
                                        placeholderText: "0.00"
                                        Layout.preferredWidth: 80
                                        onTextChanged: root.recalculateTotals()
                                        onReturnPressed: vehNoInput.focusInput = true
                                        onRightPressed: vehNoInput.focusInput = true
                                        onLeftPressed: sutliInput.focusInput = true
                                    }
                                }
                            }
                        }
                    }
                }
            }

            // 4. BOTTOM LOGISTICS MATRIX & CLEAN LIGHT TAX RECONCILIATION CARD
            RowLayout {
                Layout.fillWidth: true
                Layout.preferredHeight: 175
                spacing: 10

                // Left Side: 4x4 Logistics Matrix Grid + Narration
                Rectangle {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    color: "#FFFFFF"
                    border.color: "#E2E8F0"
                    border.width: 1
                    radius: 8

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 6
                        spacing: 2

                        Text { text: "LOGISTICS & TRANSPORTATION MATRIX"; color: "#475569"; font.pixelSize: 10; font.bold: true; font.letterSpacing: 1.0 }

                        // Row 1: Veh.No.(F10), GR No., Driver, E-Way No.
                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 6

                            ColumnLayout {
                                spacing: 1; Layout.fillWidth: true
                                Text { text: "Veh.No.(F10):"; color: "#475569"; font.pixelSize: 9; font.bold: true }
                                CustomInput {
                                    id: vehNoInput
                                    placeholderText: "KA-36-EA-4589"
                                    Layout.fillWidth: true
                                    onReturnPressed: grNoInput.focusInput = true
                                    onRightPressed: grNoInput.focusInput = true
                                    onLeftPressed: amountInput.focusInput = true
                                }
                            }
                            ColumnLayout {
                                spacing: 1; Layout.fillWidth: true
                                Text { text: "GR No. :"; color: "#475569"; font.pixelSize: 9; font.bold: true }
                                CustomInput {
                                    id: grNoInput
                                    placeholderText: "GR-1029"
                                    Layout.fillWidth: true
                                    onReturnPressed: driverInput.focusInput = true
                                    onRightPressed: driverInput.focusInput = true
                                    onLeftPressed: vehNoInput.focusInput = true
                                }
                            }
                            ColumnLayout {
                                spacing: 1; Layout.fillWidth: true
                                Text { text: "Driver :"; color: "#475569"; font.pixelSize: 9; font.bold: true }
                                CustomInput {
                                    id: driverInput
                                    placeholderText: "Ramesh"
                                    Layout.fillWidth: true
                                    onReturnPressed: ewayInput.focusInput = true
                                    onRightPressed: ewayInput.focusInput = true
                                    onLeftPressed: grNoInput.focusInput = true
                                }
                            }
                            ColumnLayout {
                                spacing: 1; Layout.fillWidth: true
                                Text { text: "E-Way No. :"; color: "#475569"; font.pixelSize: 9; font.bold: true }
                                CustomInput {
                                    id: ewayInput
                                    placeholderText: "181002938475"
                                    Layout.fillWidth: true
                                    onReturnPressed: billTimeInput.focusInput = true
                                    onRightPressed: billTimeInput.focusInput = true
                                    onLeftPressed: driverInput.focusInput = true
                                }
                            }
                        }

                        // Row 2: Bill Time, Sauda Dt., Shipping Address, POS (Same as Buyer)
                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 6

                            ColumnLayout {
                                spacing: 1; Layout.preferredWidth: 80
                                Text { text: "Bill Time :"; color: "#475569"; font.pixelSize: 9; font.bold: true }
                                CustomInput {
                                    id: billTimeInput
                                    placeholderText: "18:44:00"
                                    Layout.fillWidth: true
                                    onReturnPressed: saudaDtInput.focusInput = true
                                    onRightPressed: saudaDtInput.focusInput = true
                                    onLeftPressed: ewayInput.focusInput = true
                                }
                            }
                            ColumnLayout {
                                spacing: 1; Layout.preferredWidth: 90
                                Text { text: "Sauda Dt. :"; color: "#475569"; font.pixelSize: 9; font.bold: true }
                                CustomInput {
                                    id: saudaDtInput
                                    placeholderText: "25-08-2026"
                                    Layout.fillWidth: true
                                    onReturnPressed: shippingInput.focusInput = true
                                    onRightPressed: shippingInput.focusInput = true
                                    onLeftPressed: billTimeInput.focusInput = true
                                }
                            }
                            ColumnLayout {
                                spacing: 1; Layout.fillWidth: true
                                Text { text: "Shipping Address"; color: "#475569"; font.pixelSize: 9; font.bold: true }
                                CustomInput {
                                    id: shippingInput
                                    placeholderText: "APMC Yard, Raichur"
                                    Layout.fillWidth: true
                                    onReturnPressed: posCombo.focusAndOpen()
                                    onRightPressed: posCombo.focusAndOpen()
                                    onLeftPressed: saudaDtInput.focusInput = true
                                }
                            }
                            ColumnLayout {
                                spacing: 1; Layout.preferredWidth: 120
                                Text { text: "POS :"; color: "#475569"; font.pixelSize: 9; font.bold: true }
                                CustomWhiteCombo {
                                    id: posCombo
                                    model: ["Same as Buyer", "Inter-State"]
                                    currentIndex: 0
                                    editText: "Same as Buyer"
                                    Layout.fillWidth: true
                                    onReturnPressed: poNoInput.focusInput = true
                                    onRightPressed: poNoInput.focusInput = true
                                    onLeftPressed: shippingInput.focusInput = true
                                }
                            }
                        }

                        // Row 3: P.O. No., Grade, Transport Name, Broker Name
                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 6

                            ColumnLayout {
                                spacing: 1; Layout.preferredWidth: 80
                                Text { text: "P.O. No. :"; color: "#475569"; font.pixelSize: 9; font.bold: true }
                                CustomInput {
                                    id: poNoInput
                                    placeholderText: "PO-402"
                                    Layout.fillWidth: true
                                    onReturnPressed: gradeInput.focusInput = true
                                    onRightPressed: gradeInput.focusInput = true
                                    onLeftPressed: posCombo.focusAndOpen()
                                }
                            }
                            ColumnLayout {
                                spacing: 1; Layout.preferredWidth: 90
                                Text { text: "Grade :"; color: "#475569"; font.pixelSize: 9; font.bold: true }
                                CustomInput {
                                    id: gradeInput
                                    placeholderText: "Grade-A"
                                    Layout.fillWidth: true
                                    onReturnPressed: transportInput.focusInput = true
                                    onRightPressed: transportInput.focusInput = true
                                    onLeftPressed: poNoInput.focusInput = true
                                }
                            }
                            ColumnLayout {
                                spacing: 1; Layout.fillWidth: true
                                Text { text: "Transport"; color: "#475569"; font.pixelSize: 9; font.bold: true }
                                CustomInput {
                                    id: transportInput
                                    placeholderText: "Venkateswara Transport"
                                    Layout.fillWidth: true
                                    onReturnPressed: brokerInput.focusInput = true
                                    onRightPressed: brokerInput.focusInput = true
                                    onLeftPressed: gradeInput.focusInput = true
                                }
                            }
                            ColumnLayout {
                                spacing: 1; Layout.fillWidth: true
                                Text { text: "Broker Name :"; color: "#475569"; font.pixelSize: 9; font.bold: true }
                                CustomInput {
                                    id: brokerInput
                                    placeholderText: "Sri Rama Traders"
                                    Layout.fillWidth: true
                                    onReturnPressed: challanInput.focusInput = true
                                    onRightPressed: challanInput.focusInput = true
                                    onLeftPressed: transportInput.focusInput = true
                                }
                            }
                        }

                        // Row 4: Challan No., Kanda Weight, Narration
                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 6

                            ColumnLayout {
                                spacing: 1; Layout.preferredWidth: 80
                                Text { text: "Challan No. :"; color: "#475569"; font.pixelSize: 9; font.bold: true }
                                CustomInput {
                                    id: challanInput
                                    placeholderText: "CH-901"
                                    Layout.fillWidth: true
                                    onReturnPressed: kandaWeightInput.focusInput = true
                                    onRightPressed: kandaWeightInput.focusInput = true
                                    onLeftPressed: brokerInput.focusInput = true
                                }
                            }
                            ColumnLayout {
                                spacing: 1; Layout.preferredWidth: 90
                                Text { text: "Kanda Weight :"; color: "#475569"; font.pixelSize: 9; font.bold: true }
                                CustomInput {
                                    id: kandaWeightInput
                                    placeholderText: "102.50 Qtl"
                                    Layout.fillWidth: true
                                    onReturnPressed: narrationInput.focusInput = true
                                    onRightPressed: narrationInput.focusInput = true
                                    onLeftPressed: challanInput.focusInput = true
                                }
                            }
                            ColumnLayout {
                                spacing: 1; Layout.fillWidth: true
                                Text { text: "Narration :"; color: "#475569"; font.pixelSize: 9; font.bold: true }
                                CustomInput {
                                    id: narrationInput
                                    placeholderText: "Purchase voucher entry against Order #4029"
                                    Layout.fillWidth: true
                                    onReturnPressed: gstTaxInput.forceActiveFocus()
                                    onRightPressed: gstTaxInput.forceActiveFocus()
                                    onLeftPressed: kandaWeightInput.focusInput = true
                                }
                            }
                        }
                    }
                }

                // Right Side: CLEAN LIGHT TAX RECONCILIATION CARD
                Rectangle {
                    width: 370
                    Layout.fillHeight: true
                    color: "#F8FAFC"
                    border.color: "#CBD5E1"
                    border.width: 1
                    radius: 8

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 10
                        spacing: 4

                        RowLayout {
                            Layout.fillWidth: true
                            Text { text: "Subtotal Taxable:"; color: "#475569"; font.pixelSize: 11; font.bold: true }
                            Item { Layout.fillWidth: true }
                            Text { text: (typeof dashboardCtrl !== "undefined" && dashboardCtrl) ? dashboardCtrl.format_inr(root.taxableAmount) : ("₹" + root.taxableAmount.toFixed(2)); color: "#0F172A"; font.pixelSize: 11; font.bold: true }
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            Text { text: "SGST+CGST / IGST Tax:"; color: "#0F172A"; font.pixelSize: 11; font.bold: true }
                            Item { Layout.fillWidth: true }
                            T.TextField {
                                id: gstTaxInput
                                text: "0.00"
                                implicitWidth: 100; implicitHeight: 24
                                font.pixelSize: 11; font.bold: true
                                color: "#0F172A"
                                horizontalAlignment: Text.AlignRight
                                verticalAlignment: TextInput.AlignVCenter
                                topPadding: 0
                                bottomPadding: 0
                                rightPadding: 6
                                background: Rectangle { color: "#FFFFFF"; border.color: "#CBD5E1"; radius: 4 }
                                onTextChanged: { root.isManualGst = true; root.recalculateTotals() }
                                Keys.onReturnPressed: freightInput.forceActiveFocus()
                                Keys.onEnterPressed: freightInput.forceActiveFocus()
                                Keys.onRightPressed: freightInput.forceActiveFocus()
                                Keys.onLeftPressed: narrationInput.focusInput = true
                            }
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            Text { text: "(+) Freight Charges:"; color: "#0F172A"; font.pixelSize: 11; font.bold: true }
                            Item { Layout.fillWidth: true }
                            T.TextField {
                                id: freightInput
                                text: "0.00"
                                implicitWidth: 100; implicitHeight: 24
                                font.pixelSize: 11; font.bold: true
                                color: "#0F172A"
                                horizontalAlignment: Text.AlignRight
                                verticalAlignment: TextInput.AlignVCenter
                                topPadding: 0
                                bottomPadding: 0
                                rightPadding: 6
                                background: Rectangle { color: "#FFFFFF"; border.color: "#CBD5E1"; radius: 4 }
                                onTextChanged: root.recalculateTotals()
                                Keys.onReturnPressed: otherExpInput.forceActiveFocus()
                                Keys.onEnterPressed: otherExpInput.forceActiveFocus()
                                Keys.onRightPressed: otherExpInput.forceActiveFocus()
                                Keys.onLeftPressed: gstTaxInput.forceActiveFocus()
                            }
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            Text { text: "(+) Other Expenses:"; color: "#0F172A"; font.pixelSize: 11; font.bold: true }
                            Item { Layout.fillWidth: true }
                            T.TextField {
                                id: otherExpInput
                                text: "0.00"
                                implicitWidth: 100; implicitHeight: 24
                                font.pixelSize: 11; font.bold: true
                                color: "#0F172A"
                                horizontalAlignment: Text.AlignRight
                                verticalAlignment: TextInput.AlignVCenter
                                topPadding: 0
                                bottomPadding: 0
                                rightPadding: 6
                                background: Rectangle { color: "#FFFFFF"; border.color: "#CBD5E1"; radius: 4 }
                                onTextChanged: root.recalculateTotals()
                                Keys.onReturnPressed: lessInput.forceActiveFocus()
                                Keys.onEnterPressed: lessInput.forceActiveFocus()
                                Keys.onRightPressed: lessInput.forceActiveFocus()
                                Keys.onLeftPressed: freightInput.forceActiveFocus()
                            }
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            Text { text: "(-) Discount / Less:"; color: "#0F172A"; font.pixelSize: 11; font.bold: true }
                            Item { Layout.fillWidth: true }
                            T.TextField {
                                id: lessInput
                                text: "0.00"
                                implicitWidth: 100; implicitHeight: 24
                                font.pixelSize: 11; font.bold: true
                                color: "#0F172A"
                                horizontalAlignment: Text.AlignRight
                                verticalAlignment: TextInput.AlignVCenter
                                topPadding: 0
                                bottomPadding: 0
                                rightPadding: 6
                                background: Rectangle { color: "#FFFFFF"; border.color: "#CBD5E1"; radius: 4 }
                                onTextChanged: root.recalculateTotals()
                                Keys.onReturnPressed: tcsInput.forceActiveFocus()
                                Keys.onEnterPressed: tcsInput.forceActiveFocus()
                                Keys.onRightPressed: tcsInput.forceActiveFocus()
                                Keys.onLeftPressed: otherExpInput.forceActiveFocus()
                            }
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            Text { text: "(+) TCS @ 0.100%:"; color: "#0F172A"; font.pixelSize: 11; font.bold: true }
                            Item { Layout.fillWidth: true }
                            T.TextField {
                                id: tcsInput
                                text: "0.00"
                                implicitWidth: 100; implicitHeight: 24
                                font.pixelSize: 11; font.bold: true
                                color: "#0F172A"
                                horizontalAlignment: Text.AlignRight
                                verticalAlignment: TextInput.AlignVCenter
                                topPadding: 0
                                bottomPadding: 0
                                rightPadding: 6
                                background: Rectangle { color: "#FFFFFF"; border.color: "#CBD5E1"; radius: 4 }
                                onTextChanged: root.recalculateTotals()
                                Keys.onReturnPressed: saveBtn.forceActiveFocus()
                                Keys.onEnterPressed: saveBtn.forceActiveFocus()
                                Keys.onRightPressed: saveBtn.forceActiveFocus()
                                Keys.onLeftPressed: lessInput.forceActiveFocus()
                            }
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            Text { text: "Round Off (+/-):"; color: "#64748B"; font.pixelSize: 11; font.bold: true }
                            Item { Layout.fillWidth: true }
                            Text { text: "₹" + root.roundOffAmount.toFixed(2); color: "#475569"; font.pixelSize: 11; font.bold: true }
                        }

                        Rectangle { Layout.fillWidth: true; height: 1; color: "#CBD5E1" }

                        // GRAND TOTAL HIGHLIGHT CONTAINER CARD
                        Rectangle {
                            Layout.fillWidth: true
                            implicitHeight: 30
                            color: "#F0FDF4"
                            border.color: "#16A34A"
                            border.width: 1.5
                            radius: 6

                            RowLayout {
                                anchors.fill: parent
                                anchors.leftMargin: 10; anchors.rightMargin: 10
                                Text { text: "GRAND TOTAL:"; color: "#166534"; font.pixelSize: 11; font.bold: true }
                                Item { Layout.fillWidth: true }
                                Text { text: (typeof dashboardCtrl !== "undefined" && dashboardCtrl) ? dashboardCtrl.format_inr(root.grandTotal) : ("₹" + root.grandTotal.toFixed(2)); color: "#15803D"; font.pixelSize: 15; font.bold: true }
                            }
                        }
                    }
                }
            }

            // 5. ACTION BUTTONS FOOTER BAR
            RowLayout {
                Layout.fillWidth: true
                height: 36
                spacing: 12

                T.Button {
                    implicitWidth: contentItem.implicitWidth + 24
                    implicitHeight: 32
                    height: 32
                    background: Rectangle { color: "#F1F5F9"; radius: 6; border.color: "#CBD5E1" }
                    contentItem: Text { text: "Reset Form"; color: "#475569"; font.bold: true; font.pixelSize: 11; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                    onClicked: root.resetForm()
                }

                Item { Layout.fillWidth: true }

                T.Button {
                    id: saveBtn
                    implicitWidth: 260
                    implicitHeight: 34
                    Layout.preferredWidth: 260
                    Layout.preferredHeight: 34
                    height: 34
                    background: Rectangle { color: saveBtn.activeFocus ? "#15803D" : "#16A34A"; radius: 6; border.color: saveBtn.activeFocus ? "#86EFAC" : "transparent"; border.width: 2 }
                    contentItem: RowLayout {
                        anchors.centerIn: parent
                        spacing: 6
                        Text { text: root.isEditMode ? "💾 Update Purchase Voucher (F2)" : "💾 Save & Post Purchase Voucher (F9 / F2)"; color: "#FFF"; font.bold: true; font.pixelSize: 12 }
                    }
                    onClicked: root.saveInvoice()
                    Keys.onReturnPressed: root.saveInvoice()
                    Keys.onEnterPressed: root.saveInvoice()
                    Keys.onLeftPressed: tcsInput.forceActiveFocus()
                }
            }
        }
    }
}
