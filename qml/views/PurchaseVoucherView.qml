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

    // Aggregated Totals (Direct Reactive Bindings to C++ Controller)
    readonly property int totalBags: (typeof purchaseVoucherCtrl !== "undefined" && purchaseVoucherCtrl) ? purchaseVoucherCtrl.totalBags : 0
    readonly property real totalWeight: (typeof purchaseVoucherCtrl !== "undefined" && purchaseVoucherCtrl) ? purchaseVoucherCtrl.totalWeightQtl : 0.0
    readonly property real taxableAmount: (typeof purchaseVoucherCtrl !== "undefined" && purchaseVoucherCtrl) ? purchaseVoucherCtrl.taxableAmount : 0.0
    readonly property real gstTaxAmount: (typeof purchaseVoucherCtrl !== "undefined" && purchaseVoucherCtrl) ? purchaseVoucherCtrl.totalTaxAmount : 0.0
    readonly property real otherExpAmount: (typeof purchaseVoucherCtrl !== "undefined" && purchaseVoucherCtrl) ? purchaseVoucherCtrl.otherExp : 0.0
    readonly property real lessAmount: (typeof purchaseVoucherCtrl !== "undefined" && purchaseVoucherCtrl) ? purchaseVoucherCtrl.lessAmount : 0.0
    readonly property real freightAmount: (typeof purchaseVoucherCtrl !== "undefined" && purchaseVoucherCtrl) ? purchaseVoucherCtrl.freightCharges : 0.0
    readonly property real roundOffAmount: (typeof purchaseVoucherCtrl !== "undefined" && purchaseVoucherCtrl) ? purchaseVoucherCtrl.roundOff : 0.0
    readonly property real tcsAmount: (typeof purchaseVoucherCtrl !== "undefined" && purchaseVoucherCtrl) ? purchaseVoucherCtrl.tcsAmount : 0.0
    readonly property real grandTotal: (typeof purchaseVoucherCtrl !== "undefined" && purchaseVoucherCtrl) ? purchaseVoucherCtrl.grandTotal : 0.0
    
    property string statusMessage: ""
    property bool isError: false
    property string selectedTaxStatus: "GST / Exempt"
    property bool isManualGst: false

    Shortcut {
        sequence: "F2"
        context: Qt.WindowShortcut
        onActivated: root.openDateModal()
    }

    Shortcut {
        sequence: "Alt+S"
        context: Qt.WindowShortcut
        onActivated: partyCombo.focusAndOpen()
    }

    Shortcut {
        sequence: "Alt+L"
        context: Qt.WindowShortcut
        onActivated: partyCombo.focusAndOpen()
    }

    Shortcut {
        sequence: "Alt+P"
        context: Qt.WindowShortcut
        onActivated: partyCombo.focusAndOpen()
    }

    function openDateModal() {
        voucherDateModal.openWithDate(invoiceDateInput.text)
    }

    Component.onCompleted: {
        resetForm()
        if (typeof window !== "undefined" && window && (window.pendingEditInvoiceNo !== "" || window.pendingEditVoucherNo !== "" || window.pendingEditVoucherId > 0)) {
            var purcIdOrNo = window.pendingEditInvoiceNo !== "" ? window.pendingEditInvoiceNo : (window.pendingEditVoucherId > 0 ? window.pendingEditVoucherId : window.pendingEditVoucherNo)
            var pDate = window.pendingEditVoucherDate || ""
            window.pendingEditInvoiceNo = ""
            window.pendingEditVoucherNo = ""
            window.pendingEditVoucherId = 0
            window.pendingEditVoucherDate = ""
            Qt.callLater(function() {
                root.loadInvoiceForEditing(purcIdOrNo, pDate)
            })
        } else {
            Qt.callLater(function() {
                root.openDateModal()
            })
        }
    }

    function updateNextNumbers(dateStr) {
        if (root.isEditMode) return
        var d = dateStr || invoiceDateInput.text.trim()
        if (typeof purchaseModel !== "undefined" && purchaseModel) {
            autoVchCode = purchaseModel.get_next_voucher_no(d)
            autoVoucherNo = autoVchCode
            var nextInv = purchaseModel.get_next_invoice_no(d)
            if (!invNoInput.text.trim()) {
                invNoInput.placeholderText = nextInv ? nextInv : "e.g. SMRI/25-26/328"
            }
        }
    }

    function resetForm() {
        editingInvoiceId = 0
        var wDate = (typeof financialYearsModel !== "undefined" && financialYearsModel) ? financialYearsModel.get_working_date() : Qt.formatDate(new Date(), "dd-MM-yyyy")
        if (typeof purchaseVoucherCtrl !== "undefined" && purchaseVoucherCtrl) {
            purchaseVoucherCtrl.resetForm(wDate)
        }
        invoiceDateInput.text = wDate
        invNoInput.text = ""
        updateNextNumbers(wDate)
        dueDaysInput.text = "0"
        marketTypeCombo.currentIndex = 0
        posCombo.currentIndex = 0
        posCombo.editText = "Same as Buyer"
        
        partyCombo.currentIndex = -1
        partyCombo.editText = ""
        gstinInput.text = ""
        
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

    function loadInvoiceForEditing(invNoOrId, dateHint) {
        if (voucherDateModal && voucherDateModal.opened) {
            voucherDateModal.close()
        }
        if (typeof purchaseVoucherCtrl === "undefined" || !purchaseVoucherCtrl) return

        var ok = purchaseVoucherCtrl.loadInvoiceForEditing(invNoOrId, dateHint || "")
        if (!ok) {
            statusMessage = purchaseVoucherCtrl.statusMessage || "Purchase invoice not found"
            isError = true
            return
        }

        editingInvoiceId = purchaseVoucherCtrl.editingInvoiceId
        autoVchCode = purchaseVoucherCtrl.voucherNo
        autoVoucherNo = autoVchCode
        invNoInput.text = purchaseVoucherCtrl.invoiceNo
        
        if (purchaseVoucherCtrl.invoiceDate) {
            var raw = String(purchaseVoucherCtrl.invoiceDate).trim()
            if (raw.indexOf("-") !== -1 || raw.indexOf(".") !== -1 || raw.indexOf("/") !== -1) {
                var clean = raw.replace(/[.\/]/g, "-")
                var parts = clean.split("-")
                if (parts.length === 3) {
                    if (parts[0].length === 4) {
                        invoiceDateInput.text = parts[2] + "-" + parts[1] + "-" + parts[0]
                    } else {
                        invoiceDateInput.text = parts[0] + "-" + parts[1] + "-" + parts[2]
                    }
                } else {
                    invoiceDateInput.text = raw
                }
            } else {
                invoiceDateInput.text = raw
            }
        }
        partyCombo.editText = purchaseVoucherCtrl.partyLedger
        gstinInput.text = purchaseVoucherCtrl.gstin
        vehNoInput.text = purchaseVoucherCtrl.vehicleNo
        grNoInput.text = purchaseVoucherCtrl.grNo
        driverInput.text = purchaseVoucherCtrl.driverName
        ewayInput.text = purchaseVoucherCtrl.ewayBillNo
        billTimeInput.text = purchaseVoucherCtrl.billTime
        saudaDtInput.text = purchaseVoucherCtrl.saudaDate
        shippingInput.text = purchaseVoucherCtrl.shippingAddress
        poNoInput.text = purchaseVoucherCtrl.poNo
        gradeInput.text = purchaseVoucherCtrl.grade
        transportInput.text = purchaseVoucherCtrl.transportName
        brokerInput.text = purchaseVoucherCtrl.brokerName
        kandaWeightInput.text = purchaseVoucherCtrl.kandaWeight
        narrationInput.text = purchaseVoucherCtrl.narration

        damiAmount = purchaseVoucherCtrl.dami
        labourAmount = purchaseVoucherCtrl.labour
        auctionAmount = purchaseVoucherCtrl.auction
        mFeeAmount = purchaseVoucherCtrl.marketFee
        hrdfAmount = purchaseVoucherCtrl.hrdf
        welfareAmount = purchaseVoucherCtrl.welfare
        dhrmdAmount = purchaseVoucherCtrl.dhrmd
        sutliAmount = purchaseVoucherCtrl.sutli

        otherExpInput.text = purchaseVoucherCtrl.otherExp.toFixed(2)
        lessInput.text = purchaseVoucherCtrl.lessAmount.toFixed(2)
        freightInput.text = purchaseVoucherCtrl.freightCharges.toFixed(2)
        tcsInput.text = purchaseVoucherCtrl.tcsAmount.toFixed(2)
        challanInput.text = purchaseVoucherCtrl.challanNo
        dueDaysInput.text = String(purchaseVoucherCtrl.dueDays)

        if (purchaseVoucherCtrl.marketType === "Mandi Type") {
            marketTypeCombo.currentIndex = 1
        } else if (purchaseVoucherCtrl.marketType === "Market Type (Without Stock)") {
            marketTypeCombo.currentIndex = 2
        } else {
            marketTypeCombo.currentIndex = 0
        }

        if (purchaseVoucherCtrl.placeOfSupply) {
            posCombo.editText = purchaseVoucherCtrl.placeOfSupply
        }
        if (purchaseVoucherCtrl.taxStatus === "IGST") {
            root.selectedTaxStatus = "IGST"
        } else if (purchaseVoucherCtrl.taxStatus === "Export") {
            root.selectedTaxStatus = "Export"
        } else {
            root.selectedTaxStatus = "GST / Exempt"
        }

        clearItemInputRow()
        recalculateTotals()
    }

    function clearItemInputRow() {
        itemCombo.currentIndex = -1
        itemCombo.editText = ""
        bagsInput.text = ""
        pkngInput.text = ""
        weightInput.text = ""
        gstInput.text = ""
        rateInput.text = ""
        amountInput.text = ""
    }

    function addCurrentItemRow() {
        addLineItem()
    }

    function addLineItem() {
        var itemName = itemCombo.currentText.trim()
        if (itemName === "") {
            statusMessage = "Please select or enter an Item."
            isError = true
            return
        }

        var bCount = parseInt(bagsInput.text) || 0
        var pkng = pkngInput.text.trim() || "0.500"
        var weightVal = parseFloat(weightInput.text) || 0.0
        var gstPct = parseFloat(gstInput.text) || 0.0
        var rateVal = parseFloat(rateInput.text) || 0.0
        var userAmount = parseFloat(amountInput.text) || 0.0

        if (!root.isWithoutStock && weightVal <= 0 && bCount > 0) {
            var pkKg = parseFloat(pkng) || 0.500
            weightVal = Math.round(bCount * pkKg * 1000.0) / 1000.0
        }

        if (!root.isWithoutStock && weightVal <= 0 && bCount <= 0) {
            statusMessage = "Please enter valid Bags or Weight."
            isError = true
            return
        }

        var amount = userAmount > 0 ? userAmount : (root.isWithoutStock ? rateVal : Math.round(weightVal * rateVal * 100.0) / 100.0)

        if (typeof purchaseVoucherCtrl !== "undefined" && purchaseVoucherCtrl) {
            purchaseVoucherCtrl.lineItemsModel.appendRow(
                itemName,
                "",
                "QTL",
                root.isWithoutStock ? 0 : bCount,
                parseFloat(pkng) || 0.500,
                root.isWithoutStock ? 0.0 : weightVal,
                rateVal,
                amount,
                gstPct
            )
        }

        clearItemInputRow()
        statusMessage = ""
        isError = false
        recalculateTotals()
        Qt.callLater(function() {
            itemCombo.focusAndOpen()
        })
    }

    function removeLineItem(index) {
        if (typeof purchaseVoucherCtrl !== "undefined" && purchaseVoucherCtrl) {
            purchaseVoucherCtrl.lineItemsModel.removeRowAt(index)
            recalculateTotals()
        }
    }

    function recalculateTotals() {
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

        if (typeof purchaseVoucherCtrl !== "undefined" && purchaseVoucherCtrl) {
            purchaseVoucherCtrl.setDami(damiAmount)
            purchaseVoucherCtrl.setLabour(labourAmount)
            purchaseVoucherCtrl.setAuction(auctionAmount)
            purchaseVoucherCtrl.setMarketFee(mFeeAmount)
            purchaseVoucherCtrl.setHrdf(hrdfAmount)
            purchaseVoucherCtrl.setWelfare(welfareAmount)
            purchaseVoucherCtrl.setDhrmd(dhrmdAmount)
            purchaseVoucherCtrl.setSutli(sutliAmount)
            purchaseVoucherCtrl.setOtherExp(otherExpAmount)
            purchaseVoucherCtrl.setLessAmount(lessAmount)
            purchaseVoucherCtrl.setFreightCharges(freightAmount)
            purchaseVoucherCtrl.setTcsRate(parseFloat(tcsInput.text) || 0.0)
            purchaseVoucherCtrl.setIsInterstate(selectedTaxStatus === "IGST")
            purchaseVoucherCtrl.recalculateTotals()

            if (!isManualGst && typeof gstTaxInput !== "undefined" && gstTaxInput) {
                gstTaxInput.text = purchaseVoucherCtrl.totalTaxAmount > 0 ? purchaseVoucherCtrl.totalTaxAmount.toFixed(2) : "0.00"
            }
        }
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
            if (item.purchase_rate !== undefined && item.purchase_rate !== null && item.purchase_rate !== "" && parseFloat(item.purchase_rate) > 0) {
                rateInput.text = item.purchase_rate.toString()
            } else if (item.sale_rate !== undefined && item.sale_rate !== null && item.sale_rate !== "" && parseFloat(item.sale_rate) > 0) {
                rateInput.text = item.sale_rate.toString()
            } else {
                rateInput.text = ""
            }
            if (item.gst_rate !== undefined && item.gst_rate !== null && item.gst_rate !== "") {
                var gVal = parseFloat(item.gst_rate)
                if (isNaN(gVal)) gVal = 0.0
                gstInput.text = gVal > 0 ? (gVal.toString() + "%") : "0%"
            } else {
                gstInput.text = ""
            }
            if (item.packing_kg !== undefined && item.packing_kg !== null && item.packing_kg !== "") {
                var pVal = parseFloat(item.packing_kg) || 0.0
                if (pVal > 0) {
                    var pQtl = pVal > 2.0 ? pVal / 100.0 : pVal
                    pkngInput.text = pQtl.toFixed(3)
                } else {
                    pkngInput.text = ""
                }
            } else {
                pkngInput.text = ""
            }
            isManualGst = false
            recalculateRowAmount(true)
        }
    }

    function recalculateRowAmount(forceRecalcWeight) {
        if (!weightInput || !bagsInput || !pkngInput || !amountInput || !rateInput) return
        if (root.isWithoutStock) {
            var rWithout = parseFloat(rateInput.text) || 0.0
            amountInput.text = rWithout > 0 ? rWithout.toFixed(2) : ""
            return
        }
        var b = parseInt(bagsInput.text) || 0
        var pVal = parseFloat(pkngInput.text) || 0.0
        var pQtl = pVal > 2.0 ? pVal / 100.0 : pVal
        var autoWeight = (b > 0 && pQtl > 0) ? (Math.round((b * pQtl) * 1000.0) / 1000.0) : 0.0

        if (forceRecalcWeight || weightInput.text.trim() === "" || (bagsInput.activeFocus || pkngInput.activeFocus)) {
            if (autoWeight > 0) {
                weightInput.text = autoWeight.toFixed(3)
            } else if (b === 0 && pVal === 0.0 && (bagsInput.activeFocus || pkngInput.activeFocus)) {
                // Keep empty if bags/pkng are not set
            }
        }

        var w = parseFloat(weightInput.text) || (autoWeight > 0 ? autoWeight : 0.0)
        var r = parseFloat(rateInput.text) || 0.0
        if (w > 0 && r > 0) {
            var a = Math.round(w * r * 100.0) / 100.0
            amountInput.text = a.toFixed(2)
        } else {
            amountInput.text = ""
        }
    }

    function saveInvoice() {
        statusMessage = ""
        var partyLedger = partyCombo.currentText.trim()

        if (!partyLedger) {
            statusMessage = "Please select a Supplier / Party Ledger Account."
            isError = true
            return
        }

        var itemsCount = (typeof purchaseVoucherCtrl !== "undefined" && purchaseVoucherCtrl) ? purchaseVoucherCtrl.lineItemsModel.count : 0
        if (itemsCount === 0 && itemCombo.currentText.trim() !== "") {
            addCurrentItemRow()
            itemsCount = (typeof purchaseVoucherCtrl !== "undefined" && purchaseVoucherCtrl) ? purchaseVoucherCtrl.lineItemsModel.count : 0
        }

        if (itemsCount === 0) {
            statusMessage = "Please enter at least one Stock Item in the grid."
            isError = true
            return
        }

        saveConfirmModal.open()
    }

    function executeSaveInvoice() {
        if (typeof financialYearsModel !== "undefined" && financialYearsModel) {
            var valCheck = financialYearsModel.validate_voucher_date(invoiceDateInput.text)
            if (!valCheck.valid) {
                statusMessage = "" + valCheck.error
                isError = true
                invoiceDateInput.focusAndSelect()
                return
            }
            invoiceDateInput.text = valCheck.formattedDate
            financialYearsModel.set_working_date(valCheck.formattedDate)
        }

        var partyLedger = partyCombo.currentText.trim()
        var invNo = invNoInput.text.trim()
        if (!invNo && typeof purchaseModel !== "undefined" && purchaseModel) {
            invNo = purchaseModel.get_next_invoice_no()
        }
        var vehicle = vehNoInput.text.trim()
        var eway = ewayInput.text.trim()
        var narr = narrationInput.text.trim()

        if (typeof purchaseVoucherCtrl !== "undefined" && purchaseVoucherCtrl) {
            purchaseVoucherCtrl.editingInvoiceId = root.editingInvoiceId
            purchaseVoucherCtrl.invoiceNo = invNo
            purchaseVoucherCtrl.voucherNo = autoVoucherNo
            purchaseVoucherCtrl.invoiceDate = invoiceDateInput.text.trim()
            purchaseVoucherCtrl.partyLedger = partyLedger
            purchaseVoucherCtrl.gstin = gstinInput.text.trim()
            purchaseVoucherCtrl.dueDays = parseInt(dueDaysInput.text) || 30
            purchaseVoucherCtrl.vehicleNo = vehicle
            purchaseVoucherCtrl.ewayBillNo = eway
            purchaseVoucherCtrl.grNo = grNoInput.text.trim()
            purchaseVoucherCtrl.driverName = driverInput.text.trim()
            purchaseVoucherCtrl.billTime = billTimeInput.text.trim()
            purchaseVoucherCtrl.saudaDate = saudaDtInput.text.trim()
            purchaseVoucherCtrl.grade = gradeInput.text.trim()
            purchaseVoucherCtrl.kandaWeight = kandaWeightInput.text.trim()
            purchaseVoucherCtrl.brokerName = brokerInput.text.trim()
            purchaseVoucherCtrl.challanNo = challanInput.text.trim()
            purchaseVoucherCtrl.placeOfSupply = posCombo.editText.trim()
            purchaseVoucherCtrl.transportName = transportInput.text.trim()
            purchaseVoucherCtrl.shippingAddress = shippingInput.text.trim()
            purchaseVoucherCtrl.poNo = poNoInput.text.trim()
            purchaseVoucherCtrl.narration = narr
            purchaseVoucherCtrl.freightCharges = freightAmount
            purchaseVoucherCtrl.otherExp = otherExpAmount
            purchaseVoucherCtrl.welfare = welfareAmount
            purchaseVoucherCtrl.dhrmd = dhrmdAmount
            purchaseVoucherCtrl.sutli = sutliAmount
            purchaseVoucherCtrl.lessAmount = lessAmount
            purchaseVoucherCtrl.tcsRate = parseFloat(tcsInput.text) || 0.0
            purchaseVoucherCtrl.dami = damiAmount
            purchaseVoucherCtrl.labour = labourAmount
            purchaseVoucherCtrl.auction = auctionAmount
            purchaseVoucherCtrl.marketFee = mFeeAmount
            purchaseVoucherCtrl.hrdf = hrdfAmount
            purchaseVoucherCtrl.marketType = marketTypeCombo.currentText
            purchaseVoucherCtrl.saleStatus = selectedSaleStatus
            purchaseVoucherCtrl.taxStatus = selectedTaxStatus

            var ok = purchaseVoucherCtrl.saveVoucher()
            if (ok) {
                savedInvoiceNo = invNo
                savedInvoiceDate = invoiceDateInput.text.trim()
                savedCustomerName = partyLedger
                savedGrandTotal = grandTotal
                savedTotalBags = totalBags
                savedTotalWeight = totalWeight
                savedIsEdit = root.editingInvoiceId > 0

                statusMessage = root.editingInvoiceId > 0 ? ("Purchase Invoice " + invNo + " updated successfully!") : ("Purchase Invoice " + invNo + " saved successfully!")
                isError = false
                postSaveDialog.open()
                resetForm()
                root.invoiceSaved()
                return
            } else {
                statusMessage = purchaseVoucherCtrl.statusMessage || "Failed to save purchase invoice"
                isError = true
                return
            }
        }

        if (typeof purchaseModel !== "undefined" && purchaseModel) {
            var ok = false
            var mktType = marketTypeCombo.currentText
            var dDays = parseInt(dueDaysInput.text) || 0
            var chNo = challanInput.text.trim()
            var posVal = posCombo.editText || ""
            if (root.editingInvoiceId > 0) {
                ok = purchaseModel.update_purchase_invoice_full(
                    root.editingInvoiceId,
                    invNo, invDate, partyLedger, gstinInput.text.trim(), mainItemName, "", totalBags, totalWeight, 0.0,
                    taxableAmount, mainGstPct, cgstVal, sgstVal, igstVal, roundOffAmount, grandTotal,
                    "Credit", vehicle, eway, narr,
                    selectedSaleStatus, selectedMarketFeeStatus, damiAmount, labourAmount, auctionAmount, mFeeAmount, hrdfAmount, otherExpAmount, welfareAmount, dhrmdAmount, sutliAmount, lessAmount,
                    grNoInput.text.trim(), driverInput.text.trim(), billTimeInput.text.trim(), saudaDtInput.text.trim(), shippingInput.text.trim(), poNoInput.text.trim(), gradeInput.text.trim(), kandaWeightInput.text.trim(), transportInput.text.trim(), brokerInput.text.trim(),
                    autoVoucherNo, itemsList,
                    mktType, dDays, chNo, freightAmount, tcsAmount, 0.0, selectedTaxStatus, posVal
                )
            } else {
                ok = purchaseModel.add_purchase_invoice_full(
                    invNo, invDate, partyLedger, gstinInput.text.trim(), mainItemName, "", totalBags, totalWeight, 0.0,
                    taxableAmount, mainGstPct, cgstVal, sgstVal, igstVal, roundOffAmount, grandTotal,
                    "Credit", vehicle, eway, narr,
                    selectedSaleStatus, selectedMarketFeeStatus, damiAmount, labourAmount, auctionAmount, mFeeAmount, hrdfAmount, otherExpAmount, welfareAmount, dhrmdAmount, sutliAmount, lessAmount,
                    grNoInput.text.trim(), driverInput.text.trim(), billTimeInput.text.trim(), saudaDtInput.text.trim(), shippingInput.text.trim(), poNoInput.text.trim(), gradeInput.text.trim(), kandaWeightInput.text.trim(), transportInput.text.trim(), brokerInput.text.trim(),
                    autoVoucherNo, itemsList,
                    mktType, dDays, chNo, freightAmount, tcsAmount, 0.0, selectedTaxStatus, posVal
                )
            }
            if (ok) {
                statusMessage = root.editingInvoiceId > 0 ? ("Purchase Voucher " + invNo + " updated successfully!") : ("Purchase Voucher " + (invNo ? invNo : autoVchCode) + " saved & posted successfully!")
                isError = false
                resetForm()
                root.invoiceSaved()
            } else {
                statusMessage = "Failed to save Purchase Voucher."
                isError = true
            }
        }
    }

    function hasActivePopup() {
        return saveConfirmModal.opened || voucherDateModal.opened
    }

    function closeActivePopup() {
        if (saveConfirmModal.opened) saveConfirmModal.close()
        if (voucherDateModal.opened) voucherDateModal.close()
    }

    VoucherDateModal {
        id: voucherDateModal
        anchors.centerIn: parent
        onDateConfirmed: function(fmtDate, isoDate) {
            invoiceDateInput.text = fmtDate
            root.updateNextNumbers(fmtDate)
            Qt.callLater(function() {
                partyCombo.focusAndOpen()
            })
        }
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
                        Item { Layout.fillWidth: true }
                        Text { text: "← Back to Dashboard"; color: "#475569"; font.pixelSize: 11; font.bold: true }
                        KbdBadge { text: "Esc"; badgeColor: "#DC2626"; textColor: "#FFF"; borderColor: "#B91C1C" }
                        Item { Layout.fillWidth: true }
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
                                Text { text: ""; font.pixelSize: 9 }
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
                        Text { text: "Bill Date (F2)"; color: "#0F172A"; font.pixelSize: 10; font.bold: true }
                        CustomInput {
                            id: invoiceDateInput
                            text: (typeof financialYearsModel !== "undefined" && financialYearsModel) ? financialYearsModel.get_working_date() : Qt.formatDate(new Date(), "dd-MM-yyyy")
                            placeholderText: "DD-MM-YYYY"
                            Layout.preferredWidth: 110
                            onReturnPressed: function() {
                                if (typeof financialYearsModel !== "undefined" && financialYearsModel) {
                                    var valRes = financialYearsModel.validate_voucher_date(invoiceDateInput.text)
                                    if (!valRes.valid) {
                                        statusMessage = "" + valRes.error
                                        isError = true
                                        invoiceDateInput.focusAndSelect()
                                        return
                                    }
                                    invoiceDateInput.text = valRes.formattedDate
                                    financialYearsModel.set_working_date(valRes.formattedDate)
                                    statusMessage = ""
                                    isError = false
                                }
                                dueDaysInput.focusInput = true
                            }
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
                        Text { text: "Sale Status :"; color: "#D97706"; font.pixelSize: 10; font.bold: true }
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
                        Text { text: "Purchase From Party / Supplier Ledger Account (Alt+S) *"; color: "#0F172A"; font.pixelSize: 10; font.bold: true }
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

                            Item { Layout.preferredWidth: 35; Layout.minimumWidth: 35; Layout.maximumWidth: 35; Text { anchors.verticalCenter: parent.verticalCenter; text: "No."; color: "#0F172A"; font.pixelSize: 11; font.bold: true } }
                            Item { Layout.fillWidth: true; Layout.minimumWidth: 160; Text { anchors.verticalCenter: parent.verticalCenter; text: "Item Name *"; color: "#0F172A"; font.pixelSize: 11; font.bold: true } }
                            
                            Item { visible: !root.isWithoutStock; Layout.preferredWidth: 70; Layout.minimumWidth: 70; Layout.maximumWidth: 70; Text { anchors.right: parent.right; anchors.verticalCenter: parent.verticalCenter; text: "Bags"; color: "#0F172A"; font.pixelSize: 11; font.bold: true } }
                            Item { visible: !root.isWithoutStock; Layout.preferredWidth: 65; Layout.minimumWidth: 65; Layout.maximumWidth: 65; Text { anchors.centerIn: parent; text: "Pkng."; color: "#0F172A"; font.pixelSize: 11; font.bold: true } }
                            Item { visible: !root.isWithoutStock; Layout.preferredWidth: 95; Layout.minimumWidth: 95; Layout.maximumWidth: 95; Text { anchors.right: parent.right; anchors.verticalCenter: parent.verticalCenter; text: "Weight (Qtl)"; color: "#0F172A"; font.pixelSize: 11; font.bold: true } }
                            
                            Item { Layout.preferredWidth: 60; Layout.minimumWidth: 60; Layout.maximumWidth: 60; Text { anchors.right: parent.right; anchors.verticalCenter: parent.verticalCenter; text: "GST %"; color: "#0F172A"; font.pixelSize: 11; font.bold: true } }
                            Item { Layout.preferredWidth: 95; Layout.minimumWidth: 95; Layout.maximumWidth: 95; Text { anchors.right: parent.right; anchors.verticalCenter: parent.verticalCenter; text: "Rate (₹)"; color: "#0F172A"; font.pixelSize: 11; font.bold: true } }
                            Item { Layout.preferredWidth: 120; Layout.minimumWidth: 120; Layout.maximumWidth: 120; Text { anchors.right: parent.right; anchors.verticalCenter: parent.verticalCenter; text: "Amount (₹)"; color: "#0F172A"; font.pixelSize: 11; font.bold: true } }
                            Item { Layout.preferredWidth: 38; Layout.minimumWidth: 38; Layout.maximumWidth: 38; Text { anchors.centerIn: parent; text: "Act"; color: "#0F172A"; font.pixelSize: 11; font.bold: true } }
                        }
                    }

                    // 3B. LISTVIEW FOR COMMITTED REGISTERED ITEMS
                    ListView {
                        id: itemsListView
                        Layout.fillWidth: true
                        Layout.preferredHeight: contentHeight
                        implicitHeight: contentHeight
                        interactive: false
                        clip: true
                        model: (typeof purchaseVoucherCtrl !== "undefined" && purchaseVoucherCtrl) ? purchaseVoucherCtrl.lineItemsModel : null
                        delegate: Rectangle {
                            width: itemsListView.width
                            height: 32
                            color: index % 2 === 0 ? "#FFFFFF" : "#F8FAFC"
                            border.color: "#E2E8F0"

                            RowLayout {
                                anchors.fill: parent
                                anchors.leftMargin: 8; anchors.rightMargin: 8
                                spacing: 6

                                Item { Layout.preferredWidth: 35; Layout.minimumWidth: 35; Layout.maximumWidth: 35; Text { anchors.verticalCenter: parent.verticalCenter; text: (index + 1) + "."; color: "#0F172A"; font.pixelSize: 12; font.bold: true } }
                                Item { Layout.fillWidth: true; Layout.minimumWidth: 160; Text { anchors.verticalCenter: parent.verticalCenter; text: (model.itemName || model.item_name || ""); color: "#0F172A"; font.pixelSize: 12; font.bold: true; elide: Text.ElideRight } }
                                
                                Item { visible: !root.isWithoutStock; Layout.preferredWidth: 70; Layout.minimumWidth: 70; Layout.maximumWidth: 70; Text { anchors.right: parent.right; anchors.verticalCenter: parent.verticalCenter; text: ((model.bags || model.bag_count || 0) > 0 ? (model.bags || model.bag_count).toString() : ""); color: "#0F172A"; font.pixelSize: 12 } }
                                Item { visible: !root.isWithoutStock; Layout.preferredWidth: 65; Layout.minimumWidth: 65; Layout.maximumWidth: 65; Text { anchors.centerIn: parent; text: (model.packing || model.pkng || "0.500"); color: "#475569"; font.pixelSize: 12 } }
                                Item { visible: !root.isWithoutStock; Layout.preferredWidth: 95; Layout.minimumWidth: 95; Layout.maximumWidth: 95; Text { anchors.right: parent.right; anchors.verticalCenter: parent.verticalCenter; text: ((model.weight || model.weight_qtl || 0) > 0 ? Number(model.weight || model.weight_qtl).toFixed(3) : ""); color: "#0F172A"; font.pixelSize: 12; font.bold: true } }
                                
                                Item { Layout.preferredWidth: 60; Layout.minimumWidth: 60; Layout.maximumWidth: 60; Text { anchors.right: parent.right; anchors.verticalCenter: parent.verticalCenter; text: ((model.gstPct !== undefined ? model.gstPct : (model.gst_pct || 0)) + "%"); color: "#475569"; font.pixelSize: 12 } }
                                Item { Layout.preferredWidth: 95; Layout.minimumWidth: 95; Layout.maximumWidth: 95; Text { anchors.right: parent.right; anchors.verticalCenter: parent.verticalCenter; text: ((model.rate || model.rate_per_qtl || 0) > 0 ? Number(model.rate || model.rate_per_qtl).toFixed(2) : ""); color: "#0F172A"; font.pixelSize: 12; font.bold: true } }
                                Item { Layout.preferredWidth: 120; Layout.minimumWidth: 120; Layout.maximumWidth: 120; Text { anchors.right: parent.right; anchors.verticalCenter: parent.verticalCenter; text: "₹" + Number(model.amount || model.total_amount || 0).toFixed(2); color: "#16A34A"; font.pixelSize: 12; font.bold: true } }
                                
                                Item {
                                    Layout.preferredWidth: 38
                                    Layout.minimumWidth: 38
                                    Layout.maximumWidth: 38
                                    T.Button {
                                        anchors.centerIn: parent
                                        implicitWidth: 28
                                        implicitHeight: 20
                                        width: 28; height: 20
                                        background: Rectangle { color: "#FEE2E2"; radius: 4 }
                                        contentItem: Text { text: "X"; color: "#DC2626"; font.bold: true; font.pixelSize: 10; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
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

                            Item { Layout.preferredWidth: 35; Layout.minimumWidth: 35; Layout.maximumWidth: 35; Text { anchors.verticalCenter: parent.verticalCenter; text: (itemsListView.count + 1) + "."; color: "#16A34A"; font.pixelSize: 12; font.bold: true } }

                            CustomWhiteCombo {
                                id: itemCombo
                                model: (typeof stockItemsModel !== "undefined" && stockItemsModel) ? stockItemsModel.get_items_list(root.isMandiType ? "Mandi" : "Market") : []
                                Layout.fillWidth: true
                                Layout.minimumWidth: 160
                                placeholderText: "Select Item..."
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
                                placeholderText: "Bags"
                                horizontalAlignment: TextInput.AlignRight
                                Layout.preferredWidth: 70
                                Layout.minimumWidth: 70
                                Layout.maximumWidth: 70
                                onTextChanged: root.recalculateRowAmount(true)
                                onReturnPressed: pkngInput.focusInput = true
                                onRightPressed: pkngInput.focusInput = true
                                onLeftPressed: itemCombo.focusAndOpen()
                            }

                            CustomInput {
                                id: pkngInput
                                visible: !root.isWithoutStock
                                text: ""
                                placeholderText: "0.500"
                                horizontalAlignment: TextInput.AlignHCenter
                                Layout.preferredWidth: 65
                                Layout.minimumWidth: 65
                                Layout.maximumWidth: 65
                                onTextChanged: root.recalculateRowAmount(true)
                                onReturnPressed: weightInput.focusInput = true
                                onRightPressed: weightInput.focusInput = true
                                onLeftPressed: bagsInput.focusInput = true
                            }

                            CustomInput {
                                id: weightInput
                                visible: !root.isWithoutStock
                                placeholderText: "0.000"
                                horizontalAlignment: TextInput.AlignRight
                                Layout.preferredWidth: 95
                                Layout.minimumWidth: 95
                                Layout.maximumWidth: 95
                                onTextChanged: root.recalculateRowAmount(false)
                                onReturnPressed: gstInput.focusInput = true
                                onRightPressed: gstInput.focusInput = true
                                onLeftPressed: pkngInput.focusInput = true
                            }

                            CustomInput {
                                id: gstInput
                                text: ""
                                placeholderText: "0%"
                                horizontalAlignment: TextInput.AlignRight
                                Layout.preferredWidth: 60
                                Layout.minimumWidth: 60
                                Layout.maximumWidth: 60
                                onReturnPressed: rateInput.focusInput = true
                                onRightPressed: rateInput.focusInput = true
                                onLeftPressed: root.isWithoutStock ? itemCombo.focusAndOpen() : weightInput.focusInput = true
                            }

                            CustomInput {
                                id: rateInput
                                placeholderText: "0.00"
                                horizontalAlignment: TextInput.AlignRight
                                Layout.preferredWidth: 95
                                Layout.minimumWidth: 95
                                Layout.maximumWidth: 95
                                onTextChanged: root.recalculateRowAmount(false)
                                onReturnPressed: amountInput.focusInput = true
                                onRightPressed: amountInput.focusInput = true
                                onLeftPressed: gstInput.focusInput = true
                            }

                            CustomInput {
                                id: amountInput
                                text: ""
                                placeholderText: "0.00"
                                horizontalAlignment: TextInput.AlignRight
                                Layout.preferredWidth: 120
                                Layout.minimumWidth: 120
                                Layout.maximumWidth: 120
                                onReturnPressed: root.addCurrentItemRow()
                                onRightPressed: root.addCurrentItemRow()
                                onLeftPressed: rateInput.focusInput = true
                            }

                            Item {
                                Layout.preferredWidth: 38
                                Layout.minimumWidth: 38
                                Layout.maximumWidth: 38
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
                    contentItem: Text {
                        text: root.isEditMode ? "Update Purchase Voucher (F2)" : "Save & Post Purchase Voucher (F9 / F2)"
                        color: "#FFF"
                        font.bold: true
                        font.pixelSize: 12
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
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
