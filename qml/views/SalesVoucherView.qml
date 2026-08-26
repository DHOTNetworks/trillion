import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../components"

Item {
    id: root

    signal cancelRequested()
    signal invoiceSaved()

    property string autoVoucherNo: "1"
    property string autoVchCode: "VCH-9001"
    
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
    ListModel {
        id: lineItemsModel
    }

    Component.onCompleted: {
        resetForm()
        Qt.callLater(function() {
            partyCombo.focusAndOpen()
        })
    }

    function resetForm() {
        if (typeof salesModel !== "undefined" && salesModel) {
            autoVchCode = salesModel.get_next_voucher_no()
            autoVoucherNo = autoVchCode.replace("VCH-", "")
        } else {
            autoVchCode = "VCH-9001"
            autoVoucherNo = "1"
        }

        invNoInput.text = "INV-" + autoVoucherNo
        dueDaysInput.text = "0"
        marketTypeCombo.currentIndex = 0
        
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
        if (weightVal <= 0 && bCount <= 0) {
            statusMessage = "❌ Please enter valid Bags or Weight."
            isError = true
            return
        }

        var amount = userAmount > 0 ? userAmount : Math.round(weightVal * rateVal * 100.0) / 100.0

        lineItemsModel.append({
            "itemName": itemName,
            "bags": bCount,
            "packing": pkng,
            "weight": weightVal,
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

        otherExpAmount = parseFloat(otherExpInput.text) || 0.0
        lessAmount = parseFloat(lessInput.text) || 0.0
        freightAmount = parseFloat(freightInput.text) || 0.0
        tcsAmount = parseFloat(tcsInput.text) || 0.0

        var gross = taxableAmount + gstTaxAmount + otherExpAmount + freightAmount + tcsAmount - lessAmount
        var rounded = Math.round(gross)
        roundOffAmount = Math.round((rounded - gross) * 100.0) / 100.0
        grandTotal = rounded
    }

    function onPartySelected(partyName) {
        if (!partyName) return
        var party = (typeof partiesModel !== "undefined" && partiesModel) ? partiesModel.get_party_by_name(partyName) : null
        if (party && party.gstin) {
            gstinInput.text = party.gstin
        }
    }

    function onItemSelected(itemName) {
        if (!itemName) return
        var item = (typeof stockItemsModel !== "undefined" && stockItemsModel) ? stockItemsModel.get_item_by_name(itemName) : null
        if (item) {
            if (item.sale_rate) rateInput.text = item.sale_rate.toString()
            if (item.gst_rate) gstInput.text = item.gst_rate
            if (item.packing_kg) pkngInput.text = (item.packing_kg / 100.0).toFixed(3)
            isManualGst = false
            recalculateRowAmount()
        }
    }

    function recalculateRowAmount() {
        var b = parseInt(bagsInput.text) || 0
        var pKg = parseFloat(pkngInput.text) * 100.0 || 50.0
        var w = weightInput.text.trim() !== "" ? parseFloat(weightInput.text) : Math.round((b * pKg / 100.0) * 1000.0) / 1000.0
        if (weightInput.text.trim() === "" && w > 0) {
            weightInput.text = w.toFixed(3)
        }
        var r = parseFloat(rateInput.text) || 0.0
        var a = Math.round(w * r * 100.0) / 100.0
        amountInput.text = a > 0 ? a.toFixed(2) : "0.00"
    }

    function saveInvoice() {
        statusMessage = ""
        var partyLedger = partyCombo.currentText.trim()

        if (!partyLedger) {
            statusMessage = "❌ Please select a Party Ledger Account."
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

        var invNo = invNoInput.text.trim()
        var invDate = Qt.formatDate(new Date(), "dd-MM-yyyy")
        var vehicle = vehNoInput.text.trim()
        var eway = ewayInput.text.trim()
        var narr = narrationInput.text.trim()

        var firstItem = lineItemsModel.get(0)
        var mainItemName = firstItem.itemName

        var cgstVal = selectedTaxStatus === "IGST" ? 0.0 : gstTaxAmount / 2.0
        var sgstVal = selectedTaxStatus === "IGST" ? 0.0 : gstTaxAmount / 2.0
        var igstVal = selectedTaxStatus === "IGST" ? gstTaxAmount : 0.0

        if (typeof salesModel !== "undefined" && salesModel) {
            var ok = salesModel.add_sales_invoice_full(
                invNo, invDate, partyLedger, gstinInput.text.trim(), mainItemName, "", totalBags, totalWeight, 0.0,
                taxableAmount, 5.0, cgstVal, sgstVal, igstVal, roundOffAmount, grandTotal,
                "Credit", vehicle, eway, narr
            )
            if (ok) {
                statusMessage = "✅ Sales Voucher " + invNo + " saved & posted successfully!"
                isError = false
                resetForm()
                root.invoiceSaved()
            } else {
                statusMessage = "❌ Failed to save Sales Voucher."
                isError = true
            }
        }
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
                        text: "Sales Voucher Entry (F8)"
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

                Button {
                    id: backBtn
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
                                Text { text: root.autoVchCode; color: "#2563EB"; font.pixelSize: 11; font.bold: true }
                                Item { Layout.fillWidth: true }
                                Text { text: "🔒"; font.pixelSize: 9 }
                            }
                        }
                    }

                    ColumnLayout {
                        spacing: 1
                        Text { text: "Invoice No (Editable)"; color: "#0F172A"; font.pixelSize: 10; font.bold: true }
                        CustomInput {
                            id: invNoInput
                            placeholderText: "INV-1"
                            Layout.preferredWidth: 120
                        }
                    }

                    ColumnLayout {
                        spacing: 1
                        Text { text: "Invoice Date"; color: "#0F172A"; font.pixelSize: 10; font.bold: true }
                        CustomInput {
                            id: invoiceDateInput
                            text: Qt.formatDate(new Date(), "dd-MM-yyyy")
                            placeholderText: "DD-MM-YYYY"
                            Layout.preferredWidth: 110
                        }
                    }

                    ColumnLayout {
                        spacing: 1
                        Text { text: "Due Days"; color: "#475569"; font.pixelSize: 10; font.bold: true }
                        CustomInput {
                            id: dueDaysInput
                            placeholderText: "0"
                            Layout.preferredWidth: 60
                        }
                    }

                    Item { Layout.fillWidth: true }

                    // Tax Status Badge Selectors
                    ColumnLayout {
                        spacing: 1
                        Text { text: "Tax Status : (Alt+R / Alt+T)"; color: "#2563EB"; font.pixelSize: 10; font.bold: true }
                        RowLayout {
                            spacing: 4

                            Rectangle {
                                width: 90; height: 28; radius: 5
                                color: root.selectedTaxStatus === "GST / Exempt" ? "#2563EB" : "#F1F5F9"
                                border.color: root.selectedTaxStatus === "GST / Exempt" ? "#1D4ED8" : "#CBD5E1"
                                Text { anchors.centerIn: parent; text: "GST / Exempt"; color: root.selectedTaxStatus === "GST / Exempt" ? "#FFF" : "#475569"; font.pixelSize: 10; font.bold: true }
                                MouseArea { anchors.fill: parent; onClicked: { root.selectedTaxStatus = "GST / Exempt"; root.isManualGst = false; root.recalculateTotals() } }
                            }

                            Rectangle {
                                width: 55; height: 28; radius: 5
                                color: root.selectedTaxStatus === "IGST" ? "#2563EB" : "#F1F5F9"
                                border.color: root.selectedTaxStatus === "IGST" ? "#1D4ED8" : "#CBD5E1"
                                Text { anchors.centerIn: parent; text: "IGST"; color: root.selectedTaxStatus === "IGST" ? "#FFF" : "#475569"; font.pixelSize: 10; font.bold: true }
                                MouseArea { anchors.fill: parent; onClicked: { root.selectedTaxStatus = "IGST"; root.isManualGst = false; root.recalculateTotals() } }
                            }

                            Rectangle {
                                width: 60; height: 28; radius: 5
                                color: root.selectedTaxStatus === "Export" ? "#2563EB" : "#F1F5F9"
                                border.color: root.selectedTaxStatus === "Export" ? "#1D4ED8" : "#CBD5E1"
                                Text { anchors.centerIn: parent; text: "Export"; color: root.selectedTaxStatus === "Export" ? "#FFF" : "#475569"; font.pixelSize: 10; font.bold: true }
                                MouseArea { anchors.fill: parent; onClicked: { root.selectedTaxStatus = "Export"; root.isManualGst = false; root.recalculateTotals() } }
                            }
                        }
                    }
                }

                // Row 2: Sale To Party Ledger Account * & Party GSTIN
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 8

                    ColumnLayout {
                        spacing: 1
                        Layout.fillWidth: true
                        Text { text: "Sale To Party Ledger Account *"; color: "#0F172A"; font.pixelSize: 10; font.bold: true }
                        CustomWhiteCombo {
                            id: partyCombo
                            model: (typeof partiesModel !== "undefined" && partiesModel) ? partiesModel.get_parties_list() : []
                            Layout.fillWidth: true
                            onCurrentTextChanged: root.onPartySelected(currentText)
                            onReturnPressed: itemCombo.focusAndOpen()
                        }
                    }

                    ColumnLayout {
                        spacing: 1
                        Text { text: "Party GSTIN"; color: "#475569"; font.pixelSize: 10; font.bold: true }
                        CustomInput {
                            id: gstinInput
                            placeholderText: "29AAAAA0000A1Z5"
                            Layout.preferredWidth: 200
                        }
                    }
                }
            }

            // 3. IN-GRID ITEM ENTRY TABLE (EXACT ALIGNED COLUMNS ACROSS HEADER, ENTRY 1 & ENTRY 2)
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

                    // 3A. HEADER ROW (EXACT MATCHING PREFERRED WIDTHS)
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
                            Item { Layout.preferredWidth: 60; Text { anchors.right: parent.right; anchors.verticalCenter: parent.verticalCenter; text: "Bags"; color: "#0F172A"; font.pixelSize: 11; font.bold: true } }
                            Item { Layout.preferredWidth: 70; Text { anchors.centerIn: parent; text: "Pkng."; color: "#0F172A"; font.pixelSize: 11; font.bold: true } }
                            Item { Layout.preferredWidth: 90; Text { anchors.right: parent.right; anchors.verticalCenter: parent.verticalCenter; text: "Weight (Qtl)"; color: "#0F172A"; font.pixelSize: 11; font.bold: true } }
                            Item { Layout.preferredWidth: 50; Text { anchors.right: parent.right; anchors.verticalCenter: parent.verticalCenter; text: "GST %"; color: "#0F172A"; font.pixelSize: 11; font.bold: true } }
                            Item { Layout.preferredWidth: 95; Text { anchors.right: parent.right; anchors.verticalCenter: parent.verticalCenter; text: "Rate (₹)"; color: "#0F172A"; font.pixelSize: 11; font.bold: true } }
                            Item { Layout.preferredWidth: 110; Text { anchors.right: parent.right; anchors.verticalCenter: parent.verticalCenter; text: "Amount (₹)"; color: "#0F172A"; font.pixelSize: 11; font.bold: true } }
                            Item { Layout.preferredWidth: 35; Text { anchors.centerIn: parent; text: "Act"; color: "#0F172A"; font.pixelSize: 11; font.bold: true } }
                        }
                    }

                    // 3B. LISTVIEW FOR COMMITTED REGISTERED ITEMS (ENTRY 1, ETC.)
                    ListView {
                        id: itemsListView
                        Layout.fillWidth: true
                        implicitHeight: contentHeight // DYNAMIC HEIGHT SO ENTRY 2 SITS DIRECTLY BELOW!
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
                                Item { Layout.preferredWidth: 60; Text { anchors.right: parent.right; anchors.verticalCenter: parent.verticalCenter; text: model.bags > 0 ? model.bags.toString() : ""; color: "#0F172A"; font.pixelSize: 12 } }
                                Item { Layout.preferredWidth: 70; Text { anchors.centerIn: parent; text: model.packing; color: "#475569"; font.pixelSize: 12 } }
                                Item { Layout.preferredWidth: 90; Text { anchors.right: parent.right; anchors.verticalCenter: parent.verticalCenter; text: model.weight > 0 ? model.weight.toFixed(3) : ""; color: "#0F172A"; font.pixelSize: 12; font.bold: true } }
                                Item { Layout.preferredWidth: 50; Text { anchors.right: parent.right; anchors.verticalCenter: parent.verticalCenter; text: model.gstPct + "%"; color: "#475569"; font.pixelSize: 12 } }
                                Item { Layout.preferredWidth: 95; Text { anchors.right: parent.right; anchors.verticalCenter: parent.verticalCenter; text: model.rate > 0 ? model.rate.toFixed(2) : ""; color: "#0F172A"; font.pixelSize: 12; font.bold: true } }
                                Item { Layout.preferredWidth: 110; Text { anchors.right: parent.right; anchors.verticalCenter: parent.verticalCenter; text: "₹" + model.amount.toFixed(2); color: "#2563EB"; font.pixelSize: 12; font.bold: true } }
                                
                                Item {
                                    Layout.preferredWidth: 35
                                    Button {
                                        anchors.centerIn: parent
                                        width: 28; height: 20
                                        background: Rectangle { color: "#FEE2E2"; radius: 4 }
                                        contentItem: Text { text: "✕"; color: "#DC2626"; font.bold: true; font.pixelSize: 10; horizontalAlignment: Text.AlignHCenter }
                                        onClicked: root.removeLineItem(index)
                                    }
                                }
                            }
                        }
                    }

                    // 3C. ACTIVE IN-GRID ENTRY ROW (ENTRY 2 - SITS DIRECTLY BELOW COMMITTED ITEMS WITH 100% IDENTICAL COLUMN WIDTHS!)
                    Rectangle {
                        Layout.fillWidth: true
                        height: 36
                        color: "#EFF6FF"
                        border.color: "#93C5FD"
                        border.width: 1

                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: 8; anchors.rightMargin: 8
                            spacing: 6

                            Item { Layout.preferredWidth: 30; Text { anchors.verticalCenter: parent.verticalCenter; text: (lineItemsModel.count + 1) + "."; color: "#2563EB"; font.pixelSize: 12; font.bold: true } }

                            CustomWhiteCombo {
                                id: itemCombo
                                model: (typeof stockItemsModel !== "undefined" && stockItemsModel) ? stockItemsModel.get_items_list() : []
                                Layout.fillWidth: true
                                Layout.preferredWidth: 240
                                onCurrentTextChanged: root.onItemSelected(currentText)
                                onReturnPressed: {
                                    if (itemCombo.currentText.trim() === "") {
                                        vehNoInput.focusInput = true
                                    } else {
                                        bagsInput.focusInput = true
                                    }
                                }
                            }

                            CustomInput {
                                id: bagsInput
                                placeholderText: "100"
                                Layout.preferredWidth: 60
                                onTextChanged: root.recalculateRowAmount()
                                onReturnPressed: pkngInput.focusInput = true
                            }

                            CustomInput {
                                id: pkngInput
                                text: "0.500"
                                placeholderText: "0.500"
                                Layout.preferredWidth: 70
                                onTextChanged: root.recalculateRowAmount()
                                onReturnPressed: weightInput.focusInput = true
                            }

                            CustomInput {
                                id: weightInput
                                placeholderText: "50.000"
                                Layout.preferredWidth: 90
                                onTextChanged: root.recalculateRowAmount()
                                onReturnPressed: gstInput.focusInput = true
                            }

                            CustomInput {
                                id: gstInput
                                text: "5%"
                                placeholderText: "5%"
                                Layout.preferredWidth: 50
                                onReturnPressed: rateInput.focusInput = true
                            }

                            CustomInput {
                                id: rateInput
                                placeholderText: "2800"
                                Layout.preferredWidth: 95
                                onTextChanged: root.recalculateRowAmount()
                                onReturnPressed: amountInput.focusInput = true
                            }

                            CustomInput {
                                id: amountInput
                                text: "0.00"
                                placeholderText: "0.00"
                                Layout.preferredWidth: 110
                                onReturnPressed: root.addCurrentItemRow()
                            }

                            Item {
                                Layout.preferredWidth: 35
                                Button {
                                    anchors.centerIn: parent
                                    width: 28; height: 22
                                    background: Rectangle { color: "#2563EB"; radius: 4 }
                                    contentItem: Text { text: "+"; color: "#FFF"; font.bold: true; font.pixelSize: 12; horizontalAlignment: Text.AlignHCenter }
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
                            
                            Text { text: root.totalBags + " Bags"; color: "#0F172A"; font.pixelSize: 12; font.bold: true }
                            Item { implicitWidth: 16 }
                            
                            Text { text: root.totalWeight.toFixed(3) + " Qtl."; color: "#0F172A"; font.pixelSize: 12; font.bold: true }
                            Item { implicitWidth: 16 }

                            Text { text: "₹" + root.taxableAmount.toLocaleString(Qt.locale(), "f", 2); color: "#9A3412"; font.pixelSize: 13; font.bold: true }
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
                                CustomInput { id: vehNoInput; placeholderText: "KA-36-EA-4589"; Layout.fillWidth: true; onReturnPressed: grNoInput.focusInput = true }
                            }
                            ColumnLayout {
                                spacing: 1; Layout.fillWidth: true
                                Text { text: "GR No. :"; color: "#475569"; font.pixelSize: 9; font.bold: true }
                                CustomInput { id: grNoInput; placeholderText: "GR-1029"; Layout.fillWidth: true; onReturnPressed: driverInput.focusInput = true }
                            }
                            ColumnLayout {
                                spacing: 1; Layout.fillWidth: true
                                Text { text: "Driver :"; color: "#475569"; font.pixelSize: 9; font.bold: true }
                                CustomInput { id: driverInput; placeholderText: "Ramesh"; Layout.fillWidth: true; onReturnPressed: ewayInput.focusInput = true }
                            }
                            ColumnLayout {
                                spacing: 1; Layout.fillWidth: true
                                Text { text: "E-Way No. :"; color: "#475569"; font.pixelSize: 9; font.bold: true }
                                CustomInput { id: ewayInput; placeholderText: "181002938475"; Layout.fillWidth: true; onReturnPressed: billTimeInput.focusInput = true }
                            }
                        }

                        // Row 2: Bill Time, Sauda Dt., Shipping Address, POS (Same as Buyer)
                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 6

                            ColumnLayout {
                                spacing: 1; Layout.preferredWidth: 80
                                Text { text: "Bill Time :"; color: "#475569"; font.pixelSize: 9; font.bold: true }
                                CustomInput { id: billTimeInput; placeholderText: "18:44:00"; Layout.fillWidth: true; onReturnPressed: saudaDtInput.focusInput = true }
                            }
                            ColumnLayout {
                                spacing: 1; Layout.preferredWidth: 90
                                Text { text: "Sauda Dt. :"; color: "#475569"; font.pixelSize: 9; font.bold: true }
                                CustomInput { id: saudaDtInput; placeholderText: "25-08-2026"; Layout.fillWidth: true; onReturnPressed: shippingInput.focusInput = true }
                            }
                            ColumnLayout {
                                spacing: 1; Layout.fillWidth: true
                                Text { text: "Shipping Address"; color: "#475569"; font.pixelSize: 9; font.bold: true }
                                CustomInput { id: shippingInput; placeholderText: "APMC Yard, Raichur"; Layout.fillWidth: true; onReturnPressed: poNoInput.focusInput = true }
                            }
                            ColumnLayout {
                                spacing: 1; Layout.preferredWidth: 120
                                Text { text: "POS :"; color: "#475569"; font.pixelSize: 9; font.bold: true }
                                CustomWhiteCombo { model: ["Same as Buyer", "Inter-State"]; Layout.fillWidth: true }
                            }
                        }

                        // Row 3: P.O. No., Grade, Transport Name, Broker Name
                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 6

                            ColumnLayout {
                                spacing: 1; Layout.preferredWidth: 80
                                Text { text: "P.O. No. :"; color: "#475569"; font.pixelSize: 9; font.bold: true }
                                CustomInput { id: poNoInput; placeholderText: "PO-402"; Layout.fillWidth: true; onReturnPressed: gradeInput.focusInput = true }
                            }
                            ColumnLayout {
                                spacing: 1; Layout.preferredWidth: 90
                                Text { text: "Grade :"; color: "#475569"; font.pixelSize: 9; font.bold: true }
                                CustomInput { id: gradeInput; placeholderText: "Grade-A"; Layout.fillWidth: true; onReturnPressed: transportInput.focusInput = true }
                            }
                            ColumnLayout {
                                spacing: 1; Layout.fillWidth: true
                                Text { text: "Transport"; color: "#475569"; font.pixelSize: 9; font.bold: true }
                                CustomInput { id: transportInput; placeholderText: "Venkateswara Transport"; Layout.fillWidth: true; onReturnPressed: brokerInput.focusInput = true }
                            }
                            ColumnLayout {
                                spacing: 1; Layout.fillWidth: true
                                Text { text: "Broker Name :"; color: "#475569"; font.pixelSize: 9; font.bold: true }
                                CustomInput { id: brokerInput; placeholderText: "Sri Rama Traders"; Layout.fillWidth: true; onReturnPressed: challanInput.focusInput = true }
                            }
                        }

                        // Row 4: Challan No., Kanda Weight, Narration
                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 6

                            ColumnLayout {
                                spacing: 1; Layout.preferredWidth: 80
                                Text { text: "Challan No. :"; color: "#475569"; font.pixelSize: 9; font.bold: true }
                                CustomInput { id: challanInput; placeholderText: "CH-901"; Layout.fillWidth: true; onReturnPressed: kandaWeightInput.focusInput = true }
                            }
                            ColumnLayout {
                                spacing: 1; Layout.preferredWidth: 90
                                Text { text: "Kanda Weight :"; color: "#475569"; font.pixelSize: 9; font.bold: true }
                                CustomInput { id: kandaWeightInput; placeholderText: "102.50 Qtl"; Layout.fillWidth: true; onReturnPressed: narrationInput.focusInput = true }
                            }
                            ColumnLayout {
                                spacing: 1; Layout.fillWidth: true
                                Text { text: "Narration :"; color: "#475569"; font.pixelSize: 9; font.bold: true }
                                CustomInput { id: narrationInput; placeholderText: "Sales invoice entry against Order #4029"; Layout.fillWidth: true; onReturnPressed: gstTaxInput.forceActiveFocus() }
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
                            Text { text: "₹" + root.taxableAmount.toLocaleString(Qt.locale(), "f", 2); color: "#0F172A"; font.pixelSize: 11; font.bold: true }
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            Text { text: "SGST+CGST / IGST Tax:"; color: "#0F172A"; font.pixelSize: 11; font.bold: true }
                            Item { Layout.fillWidth: true }
                            TextField {
                                id: gstTaxInput
                                text: "0.00"
                                implicitWidth: 100; implicitHeight: 24
                                font.pixelSize: 11; font.bold: true
                                color: "#0F172A"
                                horizontalAlignment: Text.AlignRight
                                background: Rectangle { color: "#FFFFFF"; border.color: "#CBD5E1"; radius: 4 }
                                onTextChanged: { root.isManualGst = true; root.recalculateTotals() }
                                Keys.onReturnPressed: freightInput.forceActiveFocus()
                                Keys.onEnterPressed: freightInput.forceActiveFocus()
                            }
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            Text { text: "(+) Freight Charges:"; color: "#0F172A"; font.pixelSize: 11; font.bold: true }
                            Item { Layout.fillWidth: true }
                            TextField {
                                id: freightInput
                                text: "0.00"
                                implicitWidth: 100; implicitHeight: 24
                                font.pixelSize: 11; font.bold: true
                                color: "#0F172A"
                                horizontalAlignment: Text.AlignRight
                                background: Rectangle { color: "#FFFFFF"; border.color: "#CBD5E1"; radius: 4 }
                                onTextChanged: root.recalculateTotals()
                                Keys.onReturnPressed: otherExpInput.forceActiveFocus()
                                Keys.onEnterPressed: otherExpInput.forceActiveFocus()
                            }
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            Text { text: "(+) Other Expenses:"; color: "#0F172A"; font.pixelSize: 11; font.bold: true }
                            Item { Layout.fillWidth: true }
                            TextField {
                                id: otherExpInput
                                text: "0.00"
                                implicitWidth: 100; implicitHeight: 24
                                font.pixelSize: 11; font.bold: true
                                color: "#0F172A"
                                horizontalAlignment: Text.AlignRight
                                background: Rectangle { color: "#FFFFFF"; border.color: "#CBD5E1"; radius: 4 }
                                onTextChanged: root.recalculateTotals()
                                Keys.onReturnPressed: lessInput.forceActiveFocus()
                                Keys.onEnterPressed: lessInput.forceActiveFocus()
                            }
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            Text { text: "(-) Discount / Less:"; color: "#0F172A"; font.pixelSize: 11; font.bold: true }
                            Item { Layout.fillWidth: true }
                            TextField {
                                id: lessInput
                                text: "0.00"
                                implicitWidth: 100; implicitHeight: 24
                                font.pixelSize: 11; font.bold: true
                                color: "#0F172A"
                                horizontalAlignment: Text.AlignRight
                                background: Rectangle { color: "#FFFFFF"; border.color: "#CBD5E1"; radius: 4 }
                                onTextChanged: root.recalculateTotals()
                                Keys.onReturnPressed: tcsInput.forceActiveFocus()
                                Keys.onEnterPressed: tcsInput.forceActiveFocus()
                            }
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            Text { text: "(+) TCS @ 0.100%:"; color: "#0F172A"; font.pixelSize: 11; font.bold: true }
                            Item { Layout.fillWidth: true }
                            TextField {
                                id: tcsInput
                                text: "0.00"
                                implicitWidth: 100; implicitHeight: 24
                                font.pixelSize: 11; font.bold: true
                                color: "#0F172A"
                                horizontalAlignment: Text.AlignRight
                                background: Rectangle { color: "#FFFFFF"; border.color: "#CBD5E1"; radius: 4 }
                                onTextChanged: root.recalculateTotals()
                                Keys.onReturnPressed: saveBtn.forceActiveFocus()
                                Keys.onEnterPressed: saveBtn.forceActiveFocus()
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
                                Text { text: "₹" + root.grandTotal.toLocaleString(Qt.locale(), "f", 2); color: "#15803D"; font.pixelSize: 15; font.bold: true }
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

                Button {
                    height: 32
                    background: Rectangle { color: "#F1F5F9"; radius: 6; border.color: "#CBD5E1" }
                    contentItem: Text { text: "Reset Form"; color: "#475569"; font.bold: true; font.pixelSize: 11; horizontalAlignment: Text.AlignHCenter }
                    onClicked: root.resetForm()
                }

                Item { Layout.fillWidth: true }

                Button {
                    id: saveBtn
                    height: 32
                    Layout.preferredWidth: 250
                    background: Rectangle { color: saveBtn.activeFocus ? "#1D4ED8" : "#2563EB"; radius: 6; border.color: saveBtn.activeFocus ? "#60A5FA" : "transparent"; border.width: 2 }
                    contentItem: RowLayout {
                        anchors.centerIn: parent
                        spacing: 6
                        Text { text: "💾 Save & Post Sales Voucher (F2)"; color: "#FFF"; font.bold: true; font.pixelSize: 12 }
                    }
                    onClicked: root.saveInvoice()
                    Keys.onReturnPressed: root.saveInvoice()
                    Keys.onEnterPressed: root.saveInvoice()
                }
            }
        }
    }
}
