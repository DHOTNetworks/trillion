import QtQuick
import QtQuick.Templates as T
import QtQuick.Layouts
import MahadevERP

Item {
    id: root

    signal cancelRequested()
    signal voucherSaved()

    property int nextVchNo: 1
    property string nextJFormNo: "1"
    property string vchDate: ""
    property string dayName: ""
    property string zimidarBalanceText: "Date Bal. 0.00 Dr"
    property int selectedZimidarId: 0
    property int selectedPartyId: 0

    // Mandi Status Selection (Alt+S)
    property string selectedSaleStatus: "Zimidara Self Purchase"

    // Calculations
    property int totalBags: 0
    property real totalWeight: 0.0
    property real goodsAmount: 0.0
    property real bonusAmount: 0.0
    property real reliefAmount: 0.0
    property real subtotalAmount: 0.0
    property real labourAmount: 0.0
    property real roundOffAmount: 0.0
    property real grandTotal: 0.0

    property string statusMessage: ""
    property bool isError: false

    GenericListModel {
        id: lineItemsModel
    }

    Component.onCompleted: {
        resetForm()
        Qt.callLater(function() {
            zimidarCombo.focusAndOpen()
        })
    }

    function resetForm() {
        if (typeof jformModel !== "undefined" && jformModel) {
            var info = jformModel.get_next_voucher_info()
            root.nextVchNo = info.next_voucher_no || 1
            root.nextJFormNo = info.next_jform_no || "1"
            root.vchDate = info.date_display || Qt.formatDate(new Date(), "dd-MM-yyyy")
            root.dayName = info.day_name || ""
        } else {
            root.nextVchNo = 1
            root.nextJFormNo = "1"
            root.vchDate = Qt.formatDate(new Date(), "dd-MM-yyyy")
            root.dayName = ""
        }
        jformNoInput.text = root.nextJFormNo
        vchDateInput.text = root.vchDate
        dueDaysInput.text = "0"
        zimidarCombo.currentIndex = -1
        zimidarCombo.editText = ""
        partyCombo.currentIndex = 0
        partyCombo.editText = "Self Purchase"

        vehNoInput.text = ""
        grNoInput.text = ""
        driverInput.text = ""
        ewayInput.text = ""
        billTimeInput.text = ""
        saudaDtInput.text = ""
        shippingInput.text = ""
        posCombo.currentIndex = 0
        posCombo.editText = "Direct Farmer"
        poNoInput.text = ""
        gradeInput.text = ""
        transportInput.text = ""
        brokerInput.text = ""
        challanInput.text = ""
        kandaWeightInput.text = ""
        narrationInput.text = ""

        bonusInput.text = "0.00"
        reliefInput.text = "0.00"
        labourInput.text = "0.00"
        roundInput.text = "0.00"

        clearItemInputRow()
        lineItemsModel.clear()
        recalculateTotals()
        statusMessage = ""
        isError = false
    }

    function updateZimidarBalance(partyName) {
        if (typeof partiesModel !== "undefined" && partiesModel && partyName) {
            var party = partiesModel.get_party_by_name(partyName)
            if (party && party.id) {
                root.selectedZimidarId = party.id
                if (typeof jformModel !== "undefined" && jformModel) {
                    var res = jformModel.get_zimidar_balance(party.id)
                    root.zimidarBalanceText = res.formatted_balance || "Date Bal. 0.00 Dr"
                    return
                }
            }
        }
        root.zimidarBalanceText = "Date Bal. 0.00 Dr"
    }

    function onItemSelected(itemName) {
        if (!itemName) return
        var item = (typeof stockItemsModel !== "undefined" && stockItemsModel) ? stockItemsModel.get_item_by_name(itemName) : null
        if (item) {
            if (item.purchase_rate) rateInput.text = item.purchase_rate.toString()
            else if (item.sale_rate) rateInput.text = item.sale_rate.toString()
            if (item.packing_kg) {
                var pVal = parseFloat(item.packing_kg) || 50.0
                var pQtl = pVal > 2.0 ? pVal / 100.0 : pVal
                pkngInput.text = pQtl.toFixed(3)
            }
            recalculateRowAmount(true)
        }
    }

    function recalculateRowAmount(forceRecalcWeight) {
        var b = parseInt(bagsInput.text) || 0
        var l = parseFloat(looseInput.text) || 0.0
        var pVal = parseFloat(pkngInput.text) || 0.500
        var pQtl = pVal > 2.0 ? pVal / 100.0 : pVal
        var autoWeight = Math.round(((b * pQtl) + l) * 1000.0) / 1000.0

        if (forceRecalcWeight || weightInput.text.trim() === "" || bagsInput.focusInput || looseInput.focusInput || pkngInput.focusInput) {
            weightInput.text = autoWeight > 0 ? autoWeight.toFixed(3) : (b > 0 || l > 0 ? "0.000" : "")
        }

        var w = parseFloat(weightInput.text) || autoWeight
        var r = parseFloat(rateInput.text) || 0.0
        var autoAmt = Math.round(w * r * 100.0) / 100.0
        amountInput.text = autoAmt > 0 ? autoAmt.toFixed(2) : "0.00"
    }

    function addCurrentItemRow() {
        var itemName = itemCombo.currentText.trim()
        var bCount = parseInt(bagsInput.text) || 0
        var looseVal = parseFloat(looseInput.text) || 0.0
        var pkngVal = parseFloat(pkngInput.text) || 0.500
        var weightVal = parseFloat(weightInput.text) || ((bCount * pkngVal) + looseVal)
        var rateVal = parseFloat(rateInput.text) || 0.0
        var amountVal = parseFloat(amountInput.text) || (weightVal * rateVal)

        if (!itemName) {
            statusMessage = "❌ Please select an Item."
            isError = true
            itemCombo.focusAndOpen()
            return
        }

        if (weightVal <= 0 && bCount <= 0) {
            statusMessage = "❌ Please enter valid Bags, Loose or Weight."
            isError = true
            bagsInput.focusInput = true
            return
        }

        lineItemsModel.append({
            "itemName": itemName,
            "bags": bCount,
            "loose": looseVal,
            "packing": pkngVal,
            "weight": weightVal,
            "rate": rateVal,
            "amount": amountVal
        })

        clearItemInputRow()
        statusMessage = ""
        isError = false
        recalculateTotals()
        Qt.callLater(function() {
            itemCombo.focusAndOpen()
        })
    }

    function clearItemInputRow() {
        itemCombo.currentIndex = -1
        itemCombo.editText = ""
        bagsInput.text = ""
        looseInput.text = ""
        pkngInput.text = "0.500"
        weightInput.text = ""
        rateInput.text = ""
        amountInput.text = ""
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
        var sumAmount = 0.0

        for (var i = 0; i < lineItemsModel.count; i++) {
            var row = lineItemsModel.get(i)
            sumBags += row.bags
            sumWeight += row.weight
            sumAmount += row.amount
        }

        root.totalBags = sumBags
        root.totalWeight = Math.round(sumWeight * 1000.0) / 1000.0
        root.goodsAmount = Math.round(sumAmount * 100.0) / 100.0

        var bonus = parseFloat(bonusInput.text) || 0.0
        var relief = parseFloat(reliefInput.text) || 0.0
        root.bonusAmount = bonus
        root.reliefAmount = relief
        root.subtotalAmount = Math.round((root.goodsAmount + bonus + relief) * 100.0) / 100.0

        var labour = parseFloat(labourInput.text) || 0.0
        root.labourAmount = labour

        var netBeforeRound = root.subtotalAmount - labour
        var rounded = Math.round(netBeforeRound)
        var autoRound = Math.round((rounded - netBeforeRound) * 100.0) / 100.0

        if (roundInput.activeFocus) {
            root.roundOffAmount = parseFloat(roundInput.text) || 0.0
            root.grandTotal = Math.round((netBeforeRound + root.roundOffAmount) * 100.0) / 100.0
        } else {
            root.roundOffAmount = autoRound
            roundInput.text = autoRound !== 0 ? autoRound.toFixed(2) : "0.00"
            root.grandTotal = rounded
        }
    }

    function saveVoucher() {
        statusMessage = ""
        var zimidarName = zimidarCombo.currentText.trim()
        if (!zimidarName) {
            statusMessage = "❌ Please select a Zimidar (Farmer) Ledger Account."
            isError = true
            zimidarCombo.focusAndOpen()
            return
        }

        if (lineItemsModel.count === 0 && itemCombo.currentText.trim() !== "") {
            addCurrentItemRow()
        }

        if (lineItemsModel.count === 0) {
            statusMessage = "❌ Please enter at least one Item in the grid."
            isError = true
            itemCombo.focusAndOpen()
            return
        }

        saveConfirmModal.open()
    }

    function executeSaveVoucher() {
        if (typeof jformModel === "undefined" || !jformModel) {
            statusMessage = "❌ Error: JForm backend model not available."
            isError = true
            return
        }

        var dParts = vchDateInput.text.trim().split("-")
        var formattedDate = dParts.length === 3 ? (dParts[2] + "-" + dParts[1] + "-" + dParts[0]) : Qt.formatDate(new Date(), "yyyy-MM-dd")

        var headerData = {
            "voucher_no": root.nextVchNo,
            "voucher_date": formattedDate,
            "jform_no": jformNoInput.text.trim() || root.nextJFormNo,
            "zimidar_id": root.selectedZimidarId,
            "zimidar_name": zimidarCombo.currentText.trim(),
            "party_id": root.selectedPartyId,
            "party_name": partyCombo.currentText.trim() || "Self Purchase",
            "auction_sale_status": root.selectedSaleStatus,
            "due_days": parseInt(dueDaysInput.text) || 0,
            "vehicle_no": vehNoInput.text.trim(),
            "driver_name": driverInput.text.trim(),
            "gate_pass_no": grNoInput.text.trim(),
            "eway_bill_no": ewayInput.text.trim(),
            "bill_time": billTimeInput.text.trim(),
            "sauda_date": saudaDtInput.text.trim(),
            "mandi_place": shippingInput.text.trim(),
            "procurement_mode": posCombo.currentText.trim(),
            "lot_no": poNoInput.text.trim(),
            "grade": gradeInput.text.trim(),
            "transport_name": transportInput.text.trim(),
            "broker_name": brokerInput.text.trim(),
            "challan_no": challanInput.text.trim(),
            "kanda_weight": kandaWeightInput.text.trim(),
            "total_bags": root.totalBags,
            "total_weight": root.totalWeight,
            "goods_amount": root.goodsAmount,
            "bonus_amount": root.bonusAmount,
            "relief_amount": root.reliefAmount,
            "subtotal_amount": root.subtotalAmount,
            "labour_amount": root.labourAmount,
            "round_off": root.roundOffAmount,
            "grand_total": root.grandTotal,
            "narration": narrationInput.text.trim()
        }

        var items = []
        for (var i = 0; i < lineItemsModel.count; i++) {
            var row = lineItemsModel.get(i)
            items.push({
                "item_id": 0,
                "item_name": row.itemName,
                "bags": row.bags,
                "loose_weight": row.loose,
                "packing": row.packing,
                "weight": row.weight,
                "rate": row.rate,
                "amount": row.amount
            })
        }

        var ok = jformModel.save_jform_voucher(headerData, items)
        if (ok) {
            statusMessage = "✅ J-Form Voucher #" + root.nextVchNo + " (Form J: " + (jformNoInput.text.trim() || root.nextJFormNo) + ") posted successfully!"
            isError = false
            resetForm()
            root.voucherSaved()
        } else {
            statusMessage = "❌ Failed to save J-Form Voucher."
            isError = true
        }
    }

    // Keyboard Shortcuts
    Shortcut {
        sequence: "F2"
        context: Qt.WindowShortcut
        onActivated: root.saveVoucher()
    }
    Shortcut {
        sequence: StandardKey.Save
        context: Qt.WindowShortcut
        onActivated: root.saveVoucher()
    }
    Shortcut {
        sequence: "Alt+S"
        context: Qt.WindowShortcut
        onActivated: {
            root.selectedSaleStatus = root.selectedSaleStatus === "Zimidara Self Purchase" ? "Party" : "Zimidara Self Purchase"
        }
    }
    Shortcut {
        sequence: "Alt+C"
        context: Qt.WindowShortcut
        onActivated: {
            if (typeof window !== "undefined") window.currentViewIndex = 6 // New Ledger
        }
    }
    Shortcut {
        sequence: "Esc"
        context: Qt.WindowShortcut
        onActivated: {
            if (saveConfirmModal.opened) {
                saveConfirmModal.close()
            } else {
                root.cancelRequested()
            }
        }
    }

    ConfirmationModal {
        id: saveConfirmModal
        anchors.centerIn: parent
        titleText: "CONFIRM J-FORM VOUCHER SAVE"
        messageText: "Are you sure you want to save & post J-Form Voucher " + (jformNoInput.text.trim() || root.nextJFormNo) + " for ₹" + root.grandTotal.toFixed(2) + "?"
        onConfirmed: root.executeSaveVoucher()
    }

    // MAIN SINGLE SLATE CARD CONTAINER (IDENTICAL TO SALES VOUCHER)
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

            // -----------------------------------------------------------------
            // 1. TOP TITLE HEADER BAR (IDENTICAL TO SALES VOUCHER)
            // -----------------------------------------------------------------
            RowLayout {
                Layout.fillWidth: true
                spacing: 10

                ColumnLayout {
                    spacing: 0
                    Text {
                        text: "J-Form Mandi Procurement Voucher (Form J / F11)"
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
                color: root.isError ? "#FEF2F2" : "#F0FDF4"
                border.color: root.isError ? "#FCA5A5" : "#86EFAC"
                border.width: 1
                radius: 4
                visible: root.statusMessage !== ""

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 10; anchors.rightMargin: 10
                    Text { text: root.statusMessage; color: root.isError ? "#991B1B" : "#166534"; font.pixelSize: 11; font.bold: true }
                }
            }

            // -----------------------------------------------------------------
            // 2. VOUCHER & ZIMIDAR CONTROLS SECTION (IDENTICAL TO SALES VOUCHER ROWS)
            // -----------------------------------------------------------------
            ColumnLayout {
                Layout.fillWidth: true
                spacing: 4

                // Row 1: Voucher No (Auto), J. Form No, Voucher Date, Due Days, Auction Sale Status Badges
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 8

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
                                Text { text: "JFrm-" + root.nextVchNo; color: "#2563EB"; font.pixelSize: 11; font.bold: true }
                                Item { Layout.fillWidth: true }
                                Text { text: "🔒"; font.pixelSize: 9 }
                            }
                        }
                    }

                    ColumnLayout {
                        spacing: 1
                        Text { text: "J. Form No. (Editable)"; color: "#0F172A"; font.pixelSize: 10; font.bold: true }
                        CustomInput {
                            id: jformNoInput
                            placeholderText: "1"
                            Layout.preferredWidth: 140
                            onReturnPressed: vchDateInput.focusInput = true
                            onRightPressed: vchDateInput.focusInput = true
                        }
                    }

                    ColumnLayout {
                        spacing: 1
                        Text { text: "Voucher Date"; color: "#0F172A"; font.pixelSize: 10; font.bold: true }
                        CustomInput {
                            id: vchDateInput
                            text: root.vchDate
                            placeholderText: "DD-MM-YYYY"
                            Layout.preferredWidth: 110
                            onReturnPressed: dueDaysInput.focusInput = true
                            onRightPressed: dueDaysInput.focusInput = true
                            onLeftPressed: jformNoInput.focusInput = true
                        }
                    }

                    ColumnLayout {
                        spacing: 1
                        Text { text: "Due Days"; color: "#475569"; font.pixelSize: 10; font.bold: true }
                        CustomInput {
                            id: dueDaysInput
                            placeholderText: "0"
                            Layout.preferredWidth: 60
                            onReturnPressed: zimidarCombo.focusAndOpen()
                            onRightPressed: zimidarCombo.focusAndOpen()
                            onLeftPressed: vchDateInput.focusInput = true
                        }
                    }

                    Item { Layout.fillWidth: true }

                    // Auction Sale Status Badges (Alt+S)
                    ColumnLayout {
                        spacing: 1
                        Text { text: "Auction Sale Status : (Alt+S)"; color: "#D97706"; font.pixelSize: 10; font.bold: true }
                        RowLayout {
                            spacing: 4

                            Rectangle {
                                width: 140; height: 28; radius: 5
                                color: root.selectedSaleStatus === "Zimidara Self Purchase" ? "#D97706" : "#F1F5F9"
                                border.color: root.selectedSaleStatus === "Zimidara Self Purchase" ? "#B45309" : "#CBD5E1"
                                Text { anchors.centerIn: parent; text: "Zimidara Self Purchase"; color: root.selectedSaleStatus === "Zimidara Self Purchase" ? "#FFF" : "#475569"; font.pixelSize: 10; font.bold: true }
                                MouseArea { anchors.fill: parent; onClicked: root.selectedSaleStatus = "Zimidara Self Purchase" }
                            }

                            Rectangle {
                                width: 65; height: 28; radius: 5
                                color: root.selectedSaleStatus === "Party" ? "#D97706" : "#F1F5F9"
                                border.color: root.selectedSaleStatus === "Party" ? "#B45309" : "#CBD5E1"
                                Text { anchors.centerIn: parent; text: "Party"; color: root.selectedSaleStatus === "Party" ? "#FFF" : "#475569"; font.pixelSize: 10; font.bold: true }
                                MouseArea { anchors.fill: parent; onClicked: root.selectedSaleStatus = "Party" }
                            }
                        }
                    }
                }

                // Row 2: Zimidar Name * & Live Ledger Balance & Purchasing Party Name
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 8

                    ColumnLayout {
                        spacing: 1
                        Layout.fillWidth: true
                        Layout.preferredWidth: 600

                        RowLayout {
                            Layout.fillWidth: true
                            Text { text: "Zimidar (Farmer) Ledger Account *"; color: "#0F172A"; font.pixelSize: 10; font.bold: true }
                            Item { Layout.fillWidth: true }
                            Text {
                                text: root.zimidarBalanceText
                                color: root.zimidarBalanceText.indexOf("Dr") !== -1 ? "#B91C1C" : "#15803D"
                                font.pixelSize: 11
                                font.bold: true
                            }
                        }

                        CustomWhiteCombo {
                            id: zimidarCombo
                            model: (typeof partiesModel !== "undefined" && partiesModel) ? partiesModel.get_parties_list() : []
                            Layout.fillWidth: true
                            onCurrentTextChanged: root.updateZimidarBalance(currentText)
                            onReturnPressed: partyCombo.focusAndOpen()
                            onRightPressed: partyCombo.focusAndOpen()
                            onLeftPressed: dueDaysInput.focusInput = true
                        }
                    }

                    ColumnLayout {
                        spacing: 1
                        Layout.preferredWidth: 240
                        Text { text: "Purchasing Party Name (Default Self)"; color: "#475569"; font.pixelSize: 10; font.bold: true }
                        CustomWhiteCombo {
                            id: partyCombo
                            model: (typeof partiesModel !== "undefined" && partiesModel) ? partiesModel.get_parties_list() : []
                            Layout.fillWidth: true
                            onReturnPressed: itemCombo.focusAndOpen()
                            onRightPressed: itemCombo.focusAndOpen()
                            onLeftPressed: zimidarCombo.focusAndOpen()
                        }
                    }
                }
            }

            // -----------------------------------------------------------------
            // 3. IN-GRID ITEM ENTRY TABLE (IDENTICAL TO SALES VOUCHER TABLE)
            // -----------------------------------------------------------------
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

                    // 3A. HEADER ROW
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
                            Item { Layout.preferredWidth: 70; Text { anchors.right: parent.right; anchors.verticalCenter: parent.verticalCenter; text: "Bags"; color: "#0F172A"; font.pixelSize: 11; font.bold: true } }
                            Item { Layout.preferredWidth: 80; Text { anchors.right: parent.right; anchors.verticalCenter: parent.verticalCenter; text: "Loose"; color: "#0F172A"; font.pixelSize: 11; font.bold: true } }
                            Item { Layout.preferredWidth: 75; Text { anchors.centerIn: parent; text: "Pkng."; color: "#0F172A"; font.pixelSize: 11; font.bold: true } }
                            Item { Layout.preferredWidth: 100; Text { anchors.right: parent.right; anchors.verticalCenter: parent.verticalCenter; text: "Weight (Qtl)"; color: "#0F172A"; font.pixelSize: 11; font.bold: true } }
                            Item { Layout.preferredWidth: 100; Text { anchors.right: parent.right; anchors.verticalCenter: parent.verticalCenter; text: "Rate (₹)"; color: "#0F172A"; font.pixelSize: 11; font.bold: true } }
                            Item { Layout.preferredWidth: 120; Text { anchors.right: parent.right; anchors.verticalCenter: parent.verticalCenter; text: "Amount (₹)"; color: "#0F172A"; font.pixelSize: 11; font.bold: true } }
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
                                Item { Layout.preferredWidth: 70; Text { anchors.right: parent.right; anchors.verticalCenter: parent.verticalCenter; text: model.bags > 0 ? model.bags.toString() : ""; color: "#0F172A"; font.pixelSize: 12 } }
                                Item { Layout.preferredWidth: 80; Text { anchors.right: parent.right; anchors.verticalCenter: parent.verticalCenter; text: model.loose > 0 ? model.loose.toFixed(3) : "0.000"; color: "#475569"; font.pixelSize: 12 } }
                                Item { Layout.preferredWidth: 75; Text { anchors.centerIn: parent; text: model.packing.toFixed(3); color: "#475569"; font.pixelSize: 12 } }
                                Item { Layout.preferredWidth: 100; Text { anchors.right: parent.right; anchors.verticalCenter: parent.verticalCenter; text: model.weight.toFixed(3); color: "#0F172A"; font.pixelSize: 12; font.bold: true } }
                                Item { Layout.preferredWidth: 100; Text { anchors.right: parent.right; anchors.verticalCenter: parent.verticalCenter; text: model.rate > 0 ? model.rate.toFixed(2) : ""; color: "#0F172A"; font.pixelSize: 12; font.bold: true } }
                                Item { Layout.preferredWidth: 120; Text { anchors.right: parent.right; anchors.verticalCenter: parent.verticalCenter; text: "₹" + model.amount.toFixed(2); color: "#16A34A"; font.pixelSize: 12; font.bold: true } }

                                Item {
                                    Layout.preferredWidth: 35
                                    T.Button {
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

                    // 3C. ACTIVE IN-GRID ENTRY ROW
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
                                model: (typeof stockItemsModel !== "undefined" && stockItemsModel) ? stockItemsModel.get_items_list("Mandi") : []
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
                                onRightPressed: {
                                    if (itemCombo.currentText.trim() === "") {
                                        vehNoInput.focusInput = true
                                    } else {
                                        bagsInput.focusInput = true
                                    }
                                }
                                onLeftPressed: partyCombo.focusAndOpen()
                            }

                            CustomInput {
                                id: bagsInput
                                placeholderText: "100"
                                Layout.preferredWidth: 70
                                onTextChanged: root.recalculateRowAmount(true)
                                onReturnPressed: looseInput.focusInput = true
                                onRightPressed: looseInput.focusInput = true
                                onLeftPressed: itemCombo.focusAndOpen()
                            }

                            CustomInput {
                                id: looseInput
                                placeholderText: "0.000"
                                Layout.preferredWidth: 80
                                onTextChanged: root.recalculateRowAmount(true)
                                onReturnPressed: pkngInput.focusInput = true
                                onRightPressed: pkngInput.focusInput = true
                                onLeftPressed: bagsInput.focusInput = true
                            }

                            CustomInput {
                                id: pkngInput
                                text: "0.500"
                                placeholderText: "0.500"
                                Layout.preferredWidth: 75
                                onTextChanged: root.recalculateRowAmount(true)
                                onReturnPressed: weightInput.focusInput = true
                                onRightPressed: weightInput.focusInput = true
                                onLeftPressed: looseInput.focusInput = true
                            }

                            CustomInput {
                                id: weightInput
                                placeholderText: "50.000"
                                Layout.preferredWidth: 100
                                onTextChanged: root.recalculateRowAmount(false)
                                onReturnPressed: rateInput.focusInput = true
                                onRightPressed: rateInput.focusInput = true
                                onLeftPressed: pkngInput.focusInput = true
                            }

                            CustomInput {
                                id: rateInput
                                placeholderText: "3820"
                                Layout.preferredWidth: 100
                                onTextChanged: root.recalculateRowAmount(false)
                                onReturnPressed: amountInput.focusInput = true
                                onRightPressed: amountInput.focusInput = true
                                onLeftPressed: weightInput.focusInput = true
                            }

                            CustomInput {
                                id: amountInput
                                placeholderText: "0.00"
                                Layout.preferredWidth: 120
                                onReturnPressed: root.addCurrentItemRow()
                                onRightPressed: root.addCurrentItemRow()
                                onLeftPressed: rateInput.focusInput = true
                            }

                            Item {
                                Layout.preferredWidth: 35
                                T.Button {
                                    anchors.centerIn: parent
                                    width: 28; height: 22
                                    background: Rectangle { color: "#2563EB"; radius: 4 }
                                    contentItem: Text { text: "+"; color: "#FFF"; font.bold: true; font.pixelSize: 12; horizontalAlignment: Text.AlignHCenter }
                                    onClicked: root.addCurrentItemRow()
                                }
                            }
                        }
                    }

                    // EMPTY FLEX FILLER
                    Item { Layout.fillHeight: true }

                    // 3D. BAHI-KHATA STYLE SUMMARY TOTAL BAR (MATCHING SALES VOUCHER)
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

                            Text { text: "₹" + root.goodsAmount.toFixed(2); color: "#9A3412"; font.pixelSize: 13; font.bold: true }
                        }
                    }
                }
            }

            // -----------------------------------------------------------------
            // 4. BOTTOM 2-COLUMN SPLIT: LOGISTICS MATRIX (LEFT) + DEDUCTIONS CARD (RIGHT)
            // -----------------------------------------------------------------
            RowLayout {
                Layout.fillWidth: true
                Layout.preferredHeight: 155
                spacing: 10

                // Left Side: LOGISTICS & MANDI PROCUREMENT MATRIX (IDENTICAL 4-ROW TO SALES VOUCHER)
                Rectangle {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    color: "#FFFFFF"
                    border.color: "#E2E8F0"
                    border.width: 1
                    radius: 8

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 10
                        spacing: 6

                        Text { text: "LOGISTICS & MANDI PROCUREMENT MATRIX"; color: "#475569"; font.pixelSize: 10; font.bold: true; font.letterSpacing: 1.0 }

                        // Row 1: Veh.No., Gate Pass No., Driver, E-Way / Anugya
                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 6

                            ColumnLayout {
                                spacing: 1; Layout.preferredWidth: 110
                                Text { text: "Veh.No.(F10):"; color: "#475569"; font.pixelSize: 9; font.bold: true }
                                CustomInput {
                                    id: vehNoInput
                                    placeholderText: "HR-02-AB-1234"
                                    Layout.fillWidth: true
                                    onReturnPressed: grNoInput.focusInput = true
                                    onRightPressed: grNoInput.focusInput = true
                                    onLeftPressed: amountInput.focusInput = true
                                }
                            }
                            ColumnLayout {
                                spacing: 1; Layout.fillWidth: true
                                Text { text: "Gate Pass No. :"; color: "#475569"; font.pixelSize: 9; font.bold: true }
                                CustomInput {
                                    id: grNoInput
                                    placeholderText: "GP-1029"
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
                                Text { text: "E-Way / Anugya :"; color: "#475569"; font.pixelSize: 9; font.bold: true }
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

                        // Row 2: Entry Time, Sauda Dt., Mandi Yard, Procurement Mode
                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 6

                            ColumnLayout {
                                spacing: 1; Layout.preferredWidth: 80
                                Text { text: "Entry Time :"; color: "#475569"; font.pixelSize: 9; font.bold: true }
                                CustomInput {
                                    id: billTimeInput
                                    placeholderText: "10:30:00"
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
                                    placeholderText: "04-09-2026"
                                    Layout.fillWidth: true
                                    onReturnPressed: shippingInput.focusInput = true
                                    onRightPressed: shippingInput.focusInput = true
                                    onLeftPressed: billTimeInput.focusInput = true
                                }
                            }
                            ColumnLayout {
                                spacing: 1; Layout.fillWidth: true
                                Text { text: "Mandi Yard / Place"; color: "#475569"; font.pixelSize: 9; font.bold: true }
                                CustomInput {
                                    id: shippingInput
                                    placeholderText: "Grain Market, Ladwa"
                                    Layout.fillWidth: true
                                    onReturnPressed: posCombo.focusAndOpen()
                                    onRightPressed: posCombo.focusAndOpen()
                                    onLeftPressed: saudaDtInput.focusInput = true
                                }
                            }
                            ColumnLayout {
                                spacing: 1; Layout.preferredWidth: 140
                                Text { text: "Procurement Mode :"; color: "#475569"; font.pixelSize: 9; font.bold: true }
                                CustomWhiteCombo {
                                    id: posCombo
                                    model: ["Direct Farmer", "Commission Agent", "Govt Mandi MSP"]
                                    currentIndex: 0
                                    editText: "Direct Farmer"
                                    Layout.fillWidth: true
                                    onReturnPressed: poNoInput.focusInput = true
                                    onRightPressed: poNoInput.focusInput = true
                                    onLeftPressed: shippingInput.focusInput = true
                                }
                            }
                        }

                        // Row 3: Lot No. / Heap, Grade, Transport Name, Commission Agent (Dami)
                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 6

                            ColumnLayout {
                                spacing: 1; Layout.preferredWidth: 80
                                Text { text: "Lot No. / Heap :"; color: "#475569"; font.pixelSize: 9; font.bold: true }
                                CustomInput {
                                    id: poNoInput
                                    placeholderText: "LOT-402"
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
                                    placeholderText: "Grade-A 1509"
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
                                    placeholderText: "Local Tractor Trolley"
                                    Layout.fillWidth: true
                                    onReturnPressed: brokerInput.focusInput = true
                                    onRightPressed: brokerInput.focusInput = true
                                    onLeftPressed: gradeInput.focusInput = true
                                }
                            }
                            ColumnLayout {
                                spacing: 1; Layout.fillWidth: true
                                Text { text: "Commission Agent (Dami) :"; color: "#475569"; font.pixelSize: 9; font.bold: true }
                                CustomInput {
                                    id: brokerInput
                                    placeholderText: "Self / Gupta Traders"
                                    Layout.fillWidth: true
                                    onReturnPressed: challanInput.focusInput = true
                                    onRightPressed: challanInput.focusInput = true
                                    onLeftPressed: transportInput.focusInput = true
                                }
                            }
                        }

                        // Row 4: Challan / RST No., Kanda Weight, Narration / Remarks
                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 6

                            ColumnLayout {
                                spacing: 1; Layout.preferredWidth: 90
                                Text { text: "Challan / RST No. :"; color: "#475569"; font.pixelSize: 9; font.bold: true }
                                CustomInput {
                                    id: challanInput
                                    placeholderText: "RST-901"
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
                                Text { text: "Narration / Remarks :"; color: "#475569"; font.pixelSize: 9; font.bold: true }
                                CustomInput {
                                    id: narrationInput
                                    placeholderText: "Form J procurement against farmer mandi gate pass..."
                                    Layout.fillWidth: true
                                    onReturnPressed: bonusInput.forceActiveFocus()
                                    onRightPressed: bonusInput.forceActiveFocus()
                                    onLeftPressed: kandaWeightInput.focusInput = true
                                }
                            }
                        }
                    }
                }

                // Right Side: IDENTICAL CLEAN LIGHT DEDUCTIONS & GRAND TOTAL CARD
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
                            Text { text: "Goods Amount:"; color: "#475569"; font.pixelSize: 11; font.bold: true }
                            Item { Layout.fillWidth: true }
                            Text { text: (typeof dashboardCtrl !== "undefined" && dashboardCtrl) ? dashboardCtrl.format_inr(root.goodsAmount) : ("₹" + root.goodsAmount.toFixed(2)); color: "#0F172A"; font.pixelSize: 11; font.bold: true }
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            Text { text: "(+/-) Bonus:"; color: "#0F172A"; font.pixelSize: 11; font.bold: true }
                            Item { Layout.fillWidth: true }
                            T.TextField {
                                id: bonusInput
                                text: "0.00"
                                implicitWidth: 100; implicitHeight: 24
                                font.pixelSize: 11; font.bold: true
                                color: "#0F172A"
                                horizontalAlignment: Text.AlignRight
                                background: Rectangle { color: "#FFFFFF"; border.color: "#CBD5E1"; radius: 4 }
                                onTextChanged: root.recalculateTotals()
                                Keys.onReturnPressed: reliefInput.forceActiveFocus()
                                Keys.onEnterPressed: reliefInput.forceActiveFocus()
                                Keys.onRightPressed: reliefInput.forceActiveFocus()
                                Keys.onLeftPressed: narrationInput.focusInput = true
                            }
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            Text { text: "(+) Draught Relief:"; color: "#0F172A"; font.pixelSize: 11; font.bold: true }
                            Item { Layout.fillWidth: true }
                            T.TextField {
                                id: reliefInput
                                text: "0.00"
                                implicitWidth: 100; implicitHeight: 24
                                font.pixelSize: 11; font.bold: true
                                color: "#0F172A"
                                horizontalAlignment: Text.AlignRight
                                background: Rectangle { color: "#FFFFFF"; border.color: "#CBD5E1"; radius: 4 }
                                onTextChanged: root.recalculateTotals()
                                Keys.onReturnPressed: labourInput.forceActiveFocus()
                                Keys.onEnterPressed: labourInput.forceActiveFocus()
                                Keys.onRightPressed: labourInput.forceActiveFocus()
                                Keys.onLeftPressed: bonusInput.forceActiveFocus()
                            }
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            Text { text: "Subtotal (Before Labour):"; color: "#15803D"; font.pixelSize: 11; font.bold: true }
                            Item { Layout.fillWidth: true }
                            Text { text: (typeof dashboardCtrl !== "undefined" && dashboardCtrl) ? dashboardCtrl.format_inr(root.subtotalAmount) : ("₹" + root.subtotalAmount.toFixed(2)); color: "#15803D"; font.pixelSize: 11; font.bold: true }
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            Text { text: "(-) Labour Charges:"; color: "#991B1B"; font.pixelSize: 11; font.bold: true }
                            Item { Layout.fillWidth: true }
                            T.TextField {
                                id: labourInput
                                text: "0.00"
                                implicitWidth: 100; implicitHeight: 24
                                font.pixelSize: 11; font.bold: true
                                color: "#991B1B"
                                horizontalAlignment: Text.AlignRight
                                background: Rectangle { color: "#FEF2F2"; border.color: "#FCA5A5"; radius: 4 }
                                onTextChanged: root.recalculateTotals()
                                Keys.onReturnPressed: roundInput.forceActiveFocus()
                                Keys.onEnterPressed: roundInput.forceActiveFocus()
                                Keys.onRightPressed: roundInput.forceActiveFocus()
                                Keys.onLeftPressed: reliefInput.forceActiveFocus()
                            }
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            Text { text: "Round Off (+/-):"; color: "#475569"; font.pixelSize: 11; font.bold: true }
                            Item { Layout.fillWidth: true }
                            T.TextField {
                                id: roundInput
                                text: "0.00"
                                implicitWidth: 100; implicitHeight: 24
                                font.pixelSize: 11; font.bold: true
                                color: "#475569"
                                horizontalAlignment: Text.AlignRight
                                background: Rectangle { color: "#FFFFFF"; border.color: "#CBD5E1"; radius: 4 }
                                onTextChanged: root.recalculateTotals()
                                Keys.onReturnPressed: saveBtn.forceActiveFocus()
                                Keys.onEnterPressed: saveBtn.forceActiveFocus()
                                Keys.onRightPressed: saveBtn.forceActiveFocus()
                                Keys.onLeftPressed: labourInput.forceActiveFocus()
                            }
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

            // -----------------------------------------------------------------
            // 5. ACTION BUTTONS FOOTER BAR (IDENTICAL TO SALES VOUCHER)
            // -----------------------------------------------------------------
            RowLayout {
                Layout.fillWidth: true
                height: 36
                spacing: 12

                T.Button {
                    height: 32
                    background: Rectangle { color: "#F1F5F9"; radius: 6; border.color: "#CBD5E1" }
                    contentItem: Text { text: "Reset Form"; color: "#475569"; font.bold: true; font.pixelSize: 11; horizontalAlignment: Text.AlignHCenter }
                    onClicked: root.resetForm()
                }

                Item { Layout.fillWidth: true }

                T.Button {
                    id: saveBtn
                    height: 32
                    Layout.preferredWidth: 250
                    background: Rectangle {
                        color: saveBtn.activeFocus ? "#1D4ED8" : "#2563EB"
                        radius: 6
                        border.color: saveBtn.activeFocus ? "#60A5FA" : "transparent"
                        border.width: 2
                    }
                    contentItem: RowLayout {
                        anchors.centerIn: parent
                        spacing: 6
                        Text { text: "💾 Save & Post J-Form Voucher (F2)"; color: "#FFF"; font.bold: true; font.pixelSize: 12 }
                    }
                    onClicked: root.saveVoucher()
                    Keys.onReturnPressed: root.saveVoucher()
                    Keys.onEnterPressed: root.saveVoucher()
                    Keys.onLeftPressed: roundInput.forceActiveFocus()
                }
            }
        }
    }
}
