import QtQuick
import QtQuick.Templates as T
import QtQuick.Layouts
import MahadevERP

FocusScope {
    id: root
    focus: true

    signal cancelRequested()
    signal voucherSaved()

    property string autoVoucherNo: ""
    property string autoVchCode: ""

    property real totalConsumedBags: 0
    property real totalConsumedWeight: 0.0
    property real totalConsumedAmount: 0.0

    property real totalProducedYieldPct: 0.0
    property real totalProducedBags: 0
    property real totalProducedWeight: 0.0
    property real totalProducedAmount: 0.0

    property real shortagePct: 100.0
    property real shortageWeight: 0.0
    property real shortageBags: 0

    property string statusMessage: ""
    property bool isError: false

    GenericListModel {
        id: consumedModel
    }

    GenericListModel {
        id: producedModel
    }

    Component.onCompleted: {
        resetForm()
        Qt.callLater(function() {
            vchDateInput.focusInput = true
        })
    }

    function resetForm() {
        if (typeof millingModel !== "undefined" && millingModel) {
            autoVchCode = millingModel.get_next_batch_no()
            autoVoucherNo = autoVchCode
        } else {
            autoVchCode = ""
            autoVoucherNo = ""
        }

        vchDateInput.text = Qt.formatDate(new Date(), "dd-MM-yyyy")
        particularsInput.text = ""
        
        consumedModel.clear()
        producedModel.clear()

        // Clean empty initial rows with manual amount entry
        consumedModel.append({ "itemName": "", "bags": "", "weight": "", "amount": "" })
        producedModel.append({ "itemName": "", "yieldPct": "", "bags": "", "weight": "", "amount": "" })

        statusMessage = ""
        isError = false
        recalculateTotals()
    }

    function autoPickStandardItems() {
        producedModel.clear()
        var consWt = totalConsumedWeight > 0 ? totalConsumedWeight : 0.0
        var bTotal = totalConsumedBags > 0 ? totalConsumedBags : 0

        producedModel.append({ "itemName": "Rice Basmati(Non Branded)", "yieldPct": "65.00", "bags": bTotal > 0 ? Math.round(bTotal * 0.65).toString() : "", "weight": consWt > 0 ? (consWt * 0.65).toFixed(3) : "", "amount": "" })
        producedModel.append({ "itemName": "Rice Bran", "yieldPct": "15.00", "bags": bTotal > 0 ? Math.round(bTotal * 0.15).toString() : "", "weight": consWt > 0 ? (consWt * 0.15).toFixed(3) : "", "amount": "" })
        producedModel.append({ "itemName": "Rice Broken", "yieldPct": "10.00", "bags": bTotal > 0 ? Math.round(bTotal * 0.10).toString() : "", "weight": consWt > 0 ? (consWt * 0.10).toFixed(3) : "", "amount": "" })
        producedModel.append({ "itemName": "Rice Nakku", "yieldPct": "5.00", "bags": bTotal > 0 ? Math.round(bTotal * 0.05).toString() : "", "weight": consWt > 0 ? (consWt * 0.05).toFixed(3) : "", "amount": "" })
        producedModel.append({ "itemName": "Paddy Husk", "yieldPct": "5.00", "bags": bTotal > 0 ? Math.round(bTotal * 0.05).toString() : "", "weight": consWt > 0 ? (consWt * 0.05).toFixed(3) : "", "amount": "" })
        
        recalculateTotals()
    }

    function addConsumedRow() {
        consumedModel.append({ "itemName": "", "bags": "", "weight": "", "amount": "" })
        recalculateTotals()
        Qt.callLater(function() {
            consumedListView.focusRowItem(consumedModel.count - 1, "item")
        })
    }

    function addProducedRow() {
        producedModel.append({ "itemName": "", "yieldPct": "", "bags": "", "weight": "", "amount": "" })
        recalculateTotals()
        Qt.callLater(function() {
            producedListView.focusRowItem(producedModel.count - 1, "item")
        })
    }

    function removeConsumedRow(idx) {
        if (consumedModel.count > 1 && idx >= 0 && idx < consumedModel.count) {
            consumedModel.remove(idx)
            recalculateTotals()
        }
    }

    function removeProducedRow(idx) {
        if (producedModel.count > 1 && idx >= 0 && idx < producedModel.count) {
            producedModel.remove(idx)
            recalculateTotals()
        }
    }

    function getItemPackingQtl(itemName) {
        if (!itemName) return 0.500
        if (typeof stockItemsModel !== "undefined" && stockItemsModel) {
            var item = stockItemsModel.get_item_by_name(itemName)
            if (item && item.packing_kg) {
                var pkg = parseFloat(item.packing_kg) || 50.0
                return pkg > 2.0 ? pkg / 100.0 : pkg
            }
        }
        return 0.500
    }

    function updateProducedFromYields() {
        if (totalConsumedWeight <= 0) return
        for (var i = 0; i < producedModel.count; i++) {
            var pr = producedModel.get(i)
            var yVal = parseFloat(pr.yieldPct) || 0.0
            if (yVal > 0) {
                var autoW = (totalConsumedWeight * yVal / 100.0)
                var pkgQtl = root.getItemPackingQtl(pr.itemName)
                producedModel.setProperty(i, "weight", autoW.toFixed(3))
                producedModel.setProperty(i, "bags", Math.round(autoW / pkgQtl).toString())
            }
        }
        recalculateTotals()
    }

    function recalculateTotals() {
        var cBags = 0
        var cWt = 0.0
        var cAmt = 0.0

        for (var i = 0; i < consumedModel.count; i++) {
            var cr = consumedModel.get(i)
            var b = parseInt(cr.bags) || 0
            var w = parseFloat(cr.weight) || 0.0
            var a = parseFloat(cr.amount) || 0.0
            cBags += b
            cWt += w
            cAmt += a
        }

        totalConsumedBags = cBags
        totalConsumedWeight = cWt
        totalConsumedAmount = cAmt

        var pPct = 0.0
        var pBags = 0
        var pWt = 0.0
        var pAmt = 0.0

        for (var j = 0; j < producedModel.count; j++) {
            var pr = producedModel.get(j)
            var pb = parseInt(pr.bags) || 0
            var pw = parseFloat(pr.weight) || 0.0
            var pa = parseFloat(pr.amount) || 0.0
            var yVal = parseFloat(pr.yieldPct) || 0.0

            if (cWt > 0 && pw > 0) {
                yVal = (pw / cWt * 100.0)
            }

            pPct += yVal
            pBags += pb
            pWt += pw
            pAmt += pa
        }

        totalProducedYieldPct = Math.round(pPct * 1000.0) / 1000.0
        totalProducedBags = pBags
        totalProducedWeight = Math.round(pWt * 1000.0) / 1000.0
        totalProducedAmount = pAmt

        shortagePct = Math.max(0.0, Math.round((100.0 - totalProducedYieldPct) * 1000.0) / 1000.0)
        shortageWeight = Math.max(0.0, Math.round((totalConsumedWeight - totalProducedWeight) * 1000.0) / 1000.0)
        shortageBags = Math.max(0, totalConsumedBags - totalProducedBags)
    }

    function saveMillingVoucher() {
        statusMessage = ""
        if (totalConsumedWeight <= 0) {
            statusMessage = "❌ Please enter valid Consumed Paddy Weight."
            isError = true
            return
        }

        if (totalProducedWeight <= 0) {
            statusMessage = "❌ Please enter valid Produced Items Weight."
            isError = true
            return
        }

        saveConfirmModal.open()
    }

    function executeSave() {
        var cList = []
        for (var i = 0; i < consumedModel.count; i++) {
            var cr = consumedModel.get(i)
            if (cr.itemName && cr.itemName.trim() !== "") {
                cList.push({
                    "itemName": cr.itemName,
                    "bags": parseInt(cr.bags) || 0,
                    "weight": parseFloat(cr.weight) || 0.0,
                    "amount": parseFloat(cr.amount) || 0.0
                })
            }
        }

        var pList = []
        for (var j = 0; j < producedModel.count; j++) {
            var pr = producedModel.get(j)
            if (pr.itemName && pr.itemName.trim() !== "") {
                pList.push({
                    "itemName": pr.itemName,
                    "yieldPct": parseFloat(pr.yieldPct) || 0.0,
                    "bags": parseInt(pr.bags) || 0,
                    "weight": parseFloat(pr.weight) || 0.0,
                    "amount": parseFloat(pr.amount) || 0.0
                })
            }
        }

        var ok = (typeof millingModel !== "undefined" && millingModel) ? millingModel.add_milling_voucher_full(
            root.autoVchCode,
            vchDateInput.text.trim(),
            particularsInput.text.trim(),
            cList,
            pList
        ) : false

        if (ok) {
            statusMessage = "✅ Milling Production Voucher " + root.autoVchCode + " saved & posted successfully!"
            isError = false
            root.voucherSaved()
            resetForm()
        } else {
            statusMessage = "❌ Error saving Milling Voucher. Please verify entries."
            isError = true
        }
    }

    function hasActivePopup() {
        return saveConfirmModal.opened
    }

    function closeActivePopup() {
        if (saveConfirmModal.opened) {
            saveConfirmModal.close()
            return true
        }
        return false
    }

    function handleBackOrCancel() {
        if (saveConfirmModal.opened) {
            saveConfirmModal.close()
        } else {
            root.cancelRequested()
        }
    }

    // Keyboard Shortcuts
    Shortcut { sequence: "F2"; onActivated: saveMillingVoucher() }
    Shortcut { sequence: "Ctrl+S"; onActivated: saveMillingVoucher() }
    Shortcut { sequence: "Alt+P"; onActivated: autoPickStandardItems() }
    Shortcut { sequence: "Alt+A"; onActivated: addConsumedRow() }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 12
        spacing: 8

        // 1. TOP HEADER BANNER
        Rectangle {
            Layout.fillWidth: true
            height: 44
            color: "#0F172A"
            radius: 8

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 14; anchors.rightMargin: 14
                spacing: 12

                Text {
                    text: "🌾 Milling Voucher Entry (Production & Stock Movement)"
                    color: "#FFFFFF"
                    font.pixelSize: 15
                    font.bold: true
                }

                Item { Layout.fillWidth: true }

                Rectangle {
                    height: 26
                    implicitWidth: dateTxt.implicitWidth + 16
                    color: "#1E293B"
                    radius: 4
                    border.color: "#334155"
                    Text {
                        id: dateTxt
                        anchors.centerIn: parent
                        text: Qt.formatDate(new Date(), "dddd, dd MMMM yyyy")
                        color: "#94A3B8"
                        font.pixelSize: 11
                        font.bold: true
                    }
                }

                T.Button {
                    background: Rectangle { color: "#334155"; radius: 6 }
                    contentItem: Text { text: "← Back (Esc)"; color: "#F8FAFC"; font.pixelSize: 11; font.bold: true }
                    onClicked: root.handleBackOrCancel()
                }
            }
        }

        // Status Notification Banner (if active)
        Rectangle {
            Layout.fillWidth: true
            height: 28
            color: isError ? "#FEF2F2" : "#F0FDF4"
            border.color: isError ? "#FCA5A5" : "#86EFAC"
            border.width: 1
            radius: 6
            visible: statusMessage !== ""

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 12; anchors.rightMargin: 12
                Text { text: root.statusMessage; color: isError ? "#991B1B" : "#166534"; font.pixelSize: 11; font.bold: true }
            }
        }

        // 2. HEADER CONTROLS CARD
        Rectangle {
            Layout.fillWidth: true
            height: 68
            color: "#FFFFFF"
            border.color: "#CBD5E1"
            radius: 8

            RowLayout {
                anchors.fill: parent
                anchors.margins: 10
                spacing: 12

                // Voucher No
                ColumnLayout {
                    spacing: 2
                    Text { text: "Voucher No (Auto)"; color: "#64748B"; font.pixelSize: 10; font.bold: true }
                    Rectangle {
                        implicitWidth: 100
                        height: 32
                        color: "#F1F5F9"
                        border.color: "#CBD5E1"
                        radius: 6
                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: 8; anchors.rightMargin: 8
                            Text { text: root.autoVchCode; color: "#2563EB"; font.pixelSize: 12; font.bold: true }
                            Item { Layout.fillWidth: true }
                            Text { text: "🔒"; font.pixelSize: 10 }
                        }
                    }
                }

                // Voucher Date
                CustomInput {
                    id: vchDateInput
                    label: "Voucher Date"
                    text: Qt.formatDate(new Date(), "dd-MM-yyyy")
                    Layout.preferredWidth: 120
                    focusInput: true
                    onReturnPressed: particularsInput.focusInput = true
                }

                // Particulars / Narration
                CustomInput {
                    id: particularsInput
                    label: "Particulars / Batch Narration"
                    placeholderText: "Enter optional batch narration or processing details..."
                    Layout.fillWidth: true
                    onReturnPressed: consumedListView.focusRowItem(0, "item")
                }

                // Auto Pick Items Shortcut Button
                T.Button {
                    background: Rectangle { color: "#EFF6FF"; border.color: "#93C5FD"; radius: 6 }
                    contentItem: RowLayout {
                        spacing: 6
                        Text { text: "⚡ Auto Pick Recipe"; color: "#1D4ED8"; font.pixelSize: 11; font.bold: true }
                        KbdBadge { text: "Alt+P"; badgeColor: "#1E40AF"; textColor: "#93C5FD"; borderColor: "#2563EB" }
                    }
                    onClicked: root.autoPickStandardItems()
                }
            }
        }

        // 3. MAIN 2-COLUMN SPLIT GRIDS (ITEMS TO CONSUME & ITEMS TO PRODUCE)
        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 12

            // ==================== LEFT COLUMN: ITEMS TO CONSUME (RAW PADDY) ====================
            Rectangle {
                Layout.fillWidth: true
                Layout.fillHeight: true
                color: "#FFFFFF"
                border.color: "#CBD5E1"
                radius: 8

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 10
                    spacing: 6

                    // Side Header
                    Rectangle {
                        Layout.fillWidth: true
                        height: 30
                        color: "#FEF2F2"
                        radius: 6

                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: 10; anchors.rightMargin: 10
                            Text { text: "🔻 ITEMS TO BE CONSUMED (Raw Material Input)"; color: "#DC2626"; font.pixelSize: 12; font.bold: true }
                            Item { Layout.fillWidth: true }
                            T.Button {
                                background: Rectangle { color: "#FEE2E2"; radius: 4 }
                                contentItem: Text { text: "+ Add Row (Alt+A)"; color: "#991B1B"; font.pixelSize: 10; font.bold: true }
                                onClicked: root.addConsumedRow()
                            }
                        }
                    }

                    // Table Header
                    Rectangle {
                        Layout.fillWidth: true
                        height: 26
                        color: "#F1F5F9"
                        radius: 4

                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: 6; anchors.rightMargin: 6
                            spacing: 6

                            Text { Layout.fillWidth: true; text: "Item Name"; color: "#475569"; font.pixelSize: 11; font.bold: true }
                            Text { Layout.preferredWidth: 65; text: "Bags"; color: "#475569"; font.pixelSize: 11; font.bold: true; horizontalAlignment: Text.AlignRight }
                            Text { Layout.preferredWidth: 95; text: "Weight (Qtl)"; color: "#475569"; font.pixelSize: 11; font.bold: true; horizontalAlignment: Text.AlignRight }
                            Text { Layout.preferredWidth: 110; text: "Amount (₹)"; color: "#475569"; font.pixelSize: 11; font.bold: true; horizontalAlignment: Text.AlignRight }
                            Item { Layout.preferredWidth: 24 } // Action space
                        }
                    }

                    // Consumed Rows ListView
                    ListView {
                        id: consumedListView
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        model: consumedModel
                        clip: true
                        spacing: 4

                        function focusRowItem(rowIdx, field) {
                            if (rowIdx >= 0 && rowIdx < consumedModel.count) {
                                consumedListView.currentIndex = rowIdx
                                consumedListView.positionViewAtIndex(rowIdx, ListView.Contain)
                                var item = consumedListView.itemAtIndex(rowIdx)
                                if (item && typeof item.focusField !== "undefined") {
                                    item.focusField(field)
                                }
                            }
                        }

                        delegate: Rectangle {
                            id: cDelegate
                            width: consumedListView.width
                            height: 36
                            color: index % 2 === 0 ? "#FFFFFF" : "#F8FAFC"
                            border.color: "#E2E8F0"
                            radius: 4

                            Connections {
                                target: consumedModel
                                function onDataChanged() {
                                    if (index >= 0 && index < consumedModel.count) {
                                        var a = consumedModel.get(index).amount
                                        if (a !== undefined && !cAmtIn.isFocused) cAmtIn.text = a.toString()
                                        var w = consumedModel.get(index).weight
                                        if (w !== undefined && !cWeightIn.isFocused) cWeightIn.text = w.toString()
                                        var b = consumedModel.get(index).bags
                                        if (b !== undefined && !cBagsIn.isFocused) cBagsIn.text = b.toString()
                                    }
                                }
                            }

                            function focusField(f) {
                                if (f === "item") cItemCombo.focusAndOpen()
                                else if (f === "bags") cBagsIn.focusInput = true
                                else if (f === "weight") cWeightIn.focusInput = true
                                else if (f === "amount") cAmtIn.focusInput = true
                            }

                            RowLayout {
                                anchors.fill: parent
                                anchors.leftMargin: 6; anchors.rightMargin: 6
                                spacing: 6

                                CustomWhiteCombo {
                                    id: cItemCombo
                                    Layout.fillWidth: true
                                    model: (typeof stockItemsModel !== "undefined" && stockItemsModel) ? stockItemsModel.get_items_list() : ["Paddy Basmati", "Paddy 1509", "Paddy PR-14"]
                                    editText: (typeof model.itemName !== "undefined" && model.itemName) ? model.itemName : ""
                                    onCurrentTextChanged: {
                                        consumedModel.setProperty(index, "itemName", currentText)
                                    }
                                    onReturnPressed: cBagsIn.focusInput = true
                                    onRightPressed: cBagsIn.focusInput = true
                                    onDownPressed: {
                                        if (index < consumedModel.count - 1) consumedListView.focusRowItem(index + 1, "item")
                                    }
                                    onUpPressed: {
                                        if (index > 0) consumedListView.focusRowItem(index - 1, "item")
                                    }
                                }

                                CustomInput {
                                    id: cBagsIn
                                    Layout.preferredWidth: 65
                                    text: model.bags
                                    placeholderText: "0"
                                    onTextChanged: {
                                        consumedModel.setProperty(index, "bags", text)
                                        var b = parseInt(text) || 0
                                        if (b > 0 && cBagsIn.isFocused) {
                                            var pkgQtl = root.getItemPackingQtl(model.itemName)
                                            var autoW = (b * pkgQtl)
                                            cWeightIn.text = autoW.toFixed(3)
                                            consumedModel.setProperty(index, "weight", cWeightIn.text)
                                        }
                                        root.recalculateTotals()
                                        root.updateProducedFromYields()
                                    }
                                    onReturnPressed: cWeightIn.focusInput = true
                                    onRightPressed: cWeightIn.focusInput = true
                                    onLeftPressed: cItemCombo.focusAndOpen()
                                    onDownPressed: {
                                        if (index < consumedModel.count - 1) consumedListView.focusRowItem(index + 1, "bags")
                                    }
                                    onUpPressed: {
                                        if (index > 0) consumedListView.focusRowItem(index - 1, "bags")
                                    }
                                }

                                CustomInput {
                                    id: cWeightIn
                                    Layout.preferredWidth: 95
                                    text: model.weight
                                    placeholderText: "0.000"
                                    onTextChanged: {
                                        if (cWeightIn.isFocused) {
                                            consumedModel.setProperty(index, "weight", text)
                                            var w = parseFloat(text) || 0.0
                                            if (w > 0) {
                                                var pkgQtl = root.getItemPackingQtl(model.itemName)
                                                cBagsIn.text = Math.round(w / pkgQtl).toString()
                                                consumedModel.setProperty(index, "bags", cBagsIn.text)
                                            }
                                            root.recalculateTotals()
                                            root.updateProducedFromYields()
                                        }
                                    }
                                    onReturnPressed: cAmtIn.focusInput = true
                                    onRightPressed: cAmtIn.focusInput = true
                                    onLeftPressed: cBagsIn.focusInput = true
                                    onDownPressed: {
                                        if (index < consumedModel.count - 1) consumedListView.focusRowItem(index + 1, "weight")
                                    }
                                    onUpPressed: {
                                        if (index > 0) consumedListView.focusRowItem(index - 1, "weight")
                                    }
                                }

                                CustomInput {
                                    id: cAmtIn
                                    Layout.preferredWidth: 110
                                    text: model.amount
                                    placeholderText: "0.00"
                                    onTextChanged: {
                                        consumedModel.setProperty(index, "amount", text)
                                        root.recalculateTotals()
                                    }
                                    onLeftPressed: cWeightIn.focusInput = true
                                    onDownPressed: {
                                        if (index < consumedModel.count - 1) consumedListView.focusRowItem(index + 1, "amount")
                                    }
                                    onUpPressed: {
                                        if (index > 0) consumedListView.focusRowItem(index - 1, "amount")
                                    }
                                    onReturnPressed: {
                                        if (index < consumedModel.count - 1) {
                                            consumedListView.focusRowItem(index + 1, "item")
                                        } else {
                                            producedListView.focusRowItem(0, "item")
                                        }
                                    }
                                }

                                T.Button {
                                    Layout.preferredWidth: 24
                                    height: 24
                                    flat: true
                                    contentItem: Text { text: "✕"; color: "#DC2626"; font.bold: true; font.pixelSize: 11; anchors.centerIn: parent }
                                    onClicked: root.removeConsumedRow(index)
                                }
                            }
                        }
                    }

                    Rectangle { Layout.fillWidth: true; height: 1; color: "#CBD5E1" }

                    // Consumed Subtotals Bar
                    RowLayout {
                        Layout.fillWidth: true
                        Text { text: "Total Consumed:"; color: "#475569"; font.pixelSize: 11; font.bold: true }
                        Item { Layout.fillWidth: true }
                        Text { text: root.totalConsumedBags + " Bags  |  " + root.totalConsumedWeight.toFixed(3) + " Qtl  |  " + ((typeof dashboardCtrl !== "undefined" && dashboardCtrl) ? dashboardCtrl.format_inr(root.totalConsumedAmount) : ("₹" + root.totalConsumedAmount.toFixed(2))); color: "#DC2626"; font.pixelSize: 12; font.bold: true }
                    }
                }
            }

            // ==================== RIGHT COLUMN: ITEMS TO PRODUCE (FINISHED GOODS & BY-PRODUCTS) ====================
            Rectangle {
                Layout.fillWidth: true
                Layout.fillHeight: true
                color: "#FFFFFF"
                border.color: "#CBD5E1"
                radius: 8

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 10
                    spacing: 6

                    // Side Header
                    Rectangle {
                        Layout.fillWidth: true
                        height: 30
                        color: "#DCFCE7"
                        radius: 6

                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: 10; anchors.rightMargin: 10
                            Text { text: "🔺 ITEMS TO BE PRODUCED (Output Products & Yield)"; color: "#15803D"; font.pixelSize: 12; font.bold: true }
                            Item { Layout.fillWidth: true }
                            T.Button {
                                background: Rectangle { color: "#BBF7D0"; radius: 4 }
                                contentItem: Text { text: "+ Add Row"; color: "#166534"; font.pixelSize: 10; font.bold: true }
                                onClicked: root.addProducedRow()
                            }
                        }
                    }

                    // Table Header
                    Rectangle {
                        Layout.fillWidth: true
                        height: 26
                        color: "#F1F5F9"
                        radius: 4

                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: 6; anchors.rightMargin: 6
                            spacing: 6

                            Text { Layout.fillWidth: true; text: "Item Name"; color: "#475569"; font.pixelSize: 11; font.bold: true }
                            Text { Layout.preferredWidth: 65; text: "Yield %"; color: "#475569"; font.pixelSize: 11; font.bold: true; horizontalAlignment: Text.AlignRight }
                            Text { Layout.preferredWidth: 65; text: "Bags"; color: "#475569"; font.pixelSize: 11; font.bold: true; horizontalAlignment: Text.AlignRight }
                            Text { Layout.preferredWidth: 95; text: "Weight (Qtl)"; color: "#475569"; font.pixelSize: 11; font.bold: true; horizontalAlignment: Text.AlignRight }
                            Text { Layout.preferredWidth: 110; text: "Amount (₹)"; color: "#475569"; font.pixelSize: 11; font.bold: true; horizontalAlignment: Text.AlignRight }
                            Item { Layout.preferredWidth: 24 } // Action space
                        }
                    }

                    // Produced Rows ListView
                    ListView {
                        id: producedListView
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        model: producedModel
                        clip: true
                        spacing: 4

                        function focusRowItem(rowIdx, field) {
                            if (rowIdx >= 0 && rowIdx < producedModel.count) {
                                producedListView.currentIndex = rowIdx
                                producedListView.positionViewAtIndex(rowIdx, ListView.Contain)
                                var item = producedListView.itemAtIndex(rowIdx)
                                if (item && typeof item.focusField !== "undefined") {
                                    item.focusField(field)
                                }
                            }
                        }

                        delegate: Rectangle {
                            id: pDelegate
                            width: producedListView.width
                            height: 36
                            color: index % 2 === 0 ? "#FFFFFF" : "#F8FAFC"
                            border.color: "#E2E8F0"
                            radius: 4

                            Connections {
                                target: producedModel
                                function onDataChanged() {
                                    if (index >= 0 && index < producedModel.count) {
                                        var a = producedModel.get(index).amount
                                        if (a !== undefined && !pAmtIn.isFocused) {
                                            pAmtIn.text = a.toString()
                                        }
                                        var w = producedModel.get(index).weight
                                        if (w !== undefined && !pWeightIn.isFocused) {
                                            pWeightIn.text = w.toString()
                                        }
                                        var b = producedModel.get(index).bags
                                        if (b !== undefined && !pBagsIn.isFocused) {
                                            pBagsIn.text = b.toString()
                                        }
                                    }
                                }
                            }

                            function focusField(f) {
                                if (f === "item") pItemCombo.focusAndOpen()
                                else if (f === "yield") pYieldIn.focusInput = true
                                else if (f === "bags") pBagsIn.focusInput = true
                                else if (f === "weight") pWeightIn.focusInput = true
                                else if (f === "amount") pAmtIn.focusInput = true
                            }

                            RowLayout {
                                anchors.fill: parent
                                anchors.leftMargin: 6; anchors.rightMargin: 6
                                spacing: 6

                                CustomWhiteCombo {
                                    id: pItemCombo
                                    Layout.fillWidth: true
                                    model: (typeof stockItemsModel !== "undefined" && stockItemsModel) ? stockItemsModel.get_items_list() : ["Rice Basmati(Non Branded)", "Rice Bran", "Rice Broken", "Rice Nakku", "Paddy Husk"]
                                    editText: (typeof model.itemName !== "undefined" && model.itemName) ? model.itemName : ""
                                    onCurrentTextChanged: {
                                        producedModel.setProperty(index, "itemName", currentText)
                                    }
                                    onReturnPressed: {
                                        if (!currentText || currentText.trim() === "") {
                                            root.saveMillingVoucher()
                                        } else {
                                            pYieldIn.focusInput = true
                                        }
                                    }
                                    onRightPressed: pYieldIn.focusInput = true
                                    onDownPressed: {
                                        if (index < producedModel.count - 1) producedListView.focusRowItem(index + 1, "item")
                                    }
                                    onUpPressed: {
                                        if (index > 0) producedListView.focusRowItem(index - 1, "item")
                                    }
                                }

                                CustomInput {
                                    id: pYieldIn
                                    Layout.preferredWidth: 65
                                    text: model.yieldPct
                                    placeholderText: "0.00"
                                    onTextChanged: {
                                        if (pYieldIn.isFocused) {
                                            producedModel.setProperty(index, "yieldPct", text)
                                            var yVal = parseFloat(text) || 0.0
                                            if (root.totalConsumedWeight > 0 && yVal > 0) {
                                                var autoW = (root.totalConsumedWeight * yVal / 100.0)
                                                pWeightIn.text = autoW.toFixed(3)
                                                producedModel.setProperty(index, "weight", pWeightIn.text)
                                                var pkgQtl = root.getItemPackingQtl(model.itemName)
                                                pBagsIn.text = Math.round(autoW / pkgQtl).toString()
                                                producedModel.setProperty(index, "bags", pBagsIn.text)
                                            } else if (yVal <= 0) {
                                                pWeightIn.text = "0.000"
                                                producedModel.setProperty(index, "weight", "0.000")
                                                pBagsIn.text = "0"
                                                producedModel.setProperty(index, "bags", "0")
                                            }
                                            root.recalculateTotals()
                                        }
                                    }
                                    onReturnPressed: pBagsIn.focusInput = true
                                    onRightPressed: pBagsIn.focusInput = true
                                    onLeftPressed: pItemCombo.focusAndOpen()
                                    onDownPressed: {
                                        if (index < producedModel.count - 1) producedListView.focusRowItem(index + 1, "yield")
                                    }
                                    onUpPressed: {
                                        if (index > 0) producedListView.focusRowItem(index - 1, "yield")
                                    }
                                }

                                CustomInput {
                                    id: pBagsIn
                                    Layout.preferredWidth: 65
                                    text: model.bags
                                    placeholderText: "0"
                                    onTextChanged: {
                                        if (pBagsIn.isFocused) {
                                            producedModel.setProperty(index, "bags", text)
                                            var b = parseInt(text) || 0
                                            if (b > 0) {
                                                var pkgQtl = root.getItemPackingQtl(model.itemName)
                                                var autoW = (b * pkgQtl)
                                                pWeightIn.text = autoW.toFixed(3)
                                                producedModel.setProperty(index, "weight", pWeightIn.text)
                                                if (root.totalConsumedWeight > 0) {
                                                    var yVal = (autoW / root.totalConsumedWeight * 100.0)
                                                    pYieldIn.text = yVal.toFixed(2)
                                                    producedModel.setProperty(index, "yieldPct", pYieldIn.text)
                                                }
                                            } else {
                                                pWeightIn.text = "0.000"
                                                producedModel.setProperty(index, "weight", "0.000")
                                                pYieldIn.text = "0.00"
                                                producedModel.setProperty(index, "yieldPct", "0.00")
                                            }
                                            root.recalculateTotals()
                                        }
                                    }
                                    onReturnPressed: pWeightIn.focusInput = true
                                    onRightPressed: pWeightIn.focusInput = true
                                    onLeftPressed: pYieldIn.focusInput = true
                                    onDownPressed: {
                                        if (index < producedModel.count - 1) producedListView.focusRowItem(index + 1, "bags")
                                    }
                                    onUpPressed: {
                                        if (index > 0) producedListView.focusRowItem(index - 1, "bags")
                                    }
                                }

                                CustomInput {
                                    id: pWeightIn
                                    Layout.preferredWidth: 95
                                    text: model.weight
                                    placeholderText: "0.000"
                                    onTextChanged: {
                                        if (pWeightIn.isFocused) {
                                            producedModel.setProperty(index, "weight", text)
                                            var w = parseFloat(text) || 0.0
                                            if (w > 0) {
                                                var pkgQtl = root.getItemPackingQtl(model.itemName)
                                                cBagsIn.text = Math.round(w / pkgQtl).toString()
                                                consumedModel.setProperty(index, "bags", cBagsIn.text)
                                                if (root.totalConsumedWeight > 0) {
                                                    var yVal = (w / root.totalConsumedWeight * 100.0)
                                                    pYieldIn.text = yVal.toFixed(2)
                                                    producedModel.setProperty(index, "yieldPct", pYieldIn.text)
                                                }
                                            } else {
                                                pBagsIn.text = "0"
                                                producedModel.setProperty(index, "bags", "0")
                                                pYieldIn.text = "0.00"
                                                producedModel.setProperty(index, "yieldPct", "0.00")
                                            }
                                            root.recalculateTotals()
                                        }
                                    }
                                    onReturnPressed: pAmtIn.focusInput = true
                                    onRightPressed: pAmtIn.focusInput = true
                                    onLeftPressed: pBagsIn.focusInput = true
                                    onDownPressed: {
                                        if (index < producedModel.count - 1) producedListView.focusRowItem(index + 1, "weight")
                                    }
                                    onUpPressed: {
                                        if (index > 0) producedListView.focusRowItem(index - 1, "weight")
                                    }
                                }

                                CustomInput {
                                    id: pAmtIn
                                    Layout.preferredWidth: 110
                                    text: model.amount
                                    placeholderText: "0.00"
                                    onTextChanged: {
                                        producedModel.setProperty(index, "amount", text)
                                        root.recalculateTotals()
                                    }
                                    onLeftPressed: pWeightIn.focusInput = true
                                    onDownPressed: {
                                        if (index < producedModel.count - 1) producedListView.focusRowItem(index + 1, "amount")
                                    }
                                    onUpPressed: {
                                        if (index > 0) producedListView.focusRowItem(index - 1, "amount")
                                    }
                                    onReturnPressed: {
                                        if (index === producedModel.count - 1) {
                                            if (model.itemName && model.itemName.trim() !== "" && parseFloat(model.weight) > 0) {
                                                root.addProducedRow()
                                            } else {
                                                root.saveMillingVoucher()
                                            }
                                        } else {
                                            producedListView.focusRowItem(index + 1, "item")
                                        }
                                    }
                                }

                                T.Button {
                                    Layout.preferredWidth: 24
                                    height: 24
                                    flat: true
                                    contentItem: Text { text: "✕"; color: "#DC2626"; font.bold: true; font.pixelSize: 11; anchors.centerIn: parent }
                                    onClicked: root.removeProducedRow(index)
                                }
                            }
                        }
                    }

                    Rectangle { Layout.fillWidth: true; height: 1; color: "#CBD5E1" }

                    // Produced Subtotals Bar
                    RowLayout {
                        Layout.fillWidth: true
                        Text { text: "Total Produced:"; color: "#475569"; font.pixelSize: 11; font.bold: true }
                        Item { Layout.fillWidth: true }
                        Text { text: root.totalProducedYieldPct.toFixed(2) + "% Yield  |  " + root.totalProducedBags + " Bags  |  " + root.totalProducedWeight.toFixed(3) + " Qtl  |  " + ((typeof dashboardCtrl !== "undefined" && dashboardCtrl) ? dashboardCtrl.format_inr(root.totalProducedAmount) : ("₹" + root.totalProducedAmount.toFixed(2))); color: "#15803D"; font.pixelSize: 12; font.bold: true }
                    }
                }
            }
        }

        // 4. PINNED GRAND TOTAL & SHORTAGE / WASTAGE SUMMARY FOOTER
        Rectangle {
            Layout.fillWidth: true
            height: 50
            color: "#0F172A"
            radius: 8

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 16; anchors.rightMargin: 16
                spacing: 20

                // Total Input vs Total Output
                ColumnLayout {
                    spacing: 2
                    Text { text: "MILLING YIELD RECOVERY RATIO"; color: "#94A3B8"; font.pixelSize: 10; font.bold: true; font.letterSpacing: 0.8 }
                    Text {
                        text: "Input: " + root.totalConsumedWeight.toFixed(3) + " Qtl (" + ((typeof dashboardCtrl !== "undefined" && dashboardCtrl) ? dashboardCtrl.format_inr(root.totalConsumedAmount) : ("₹" + root.totalConsumedAmount.toFixed(2))) + ")  →  Output: " + root.totalProducedWeight.toFixed(3) + " Qtl (" + root.totalProducedYieldPct.toFixed(2) + "%)"
                        color: "#E2E8F0"
                        font.pixelSize: 13
                        font.bold: true
                    }
                }

                Item { Layout.fillWidth: true }

                // Shortage / Wastage Indicator
                RowLayout {
                    spacing: 12

                    ColumnLayout {
                        spacing: 0
                        Text { text: "MILLING SHORTAGE / WASTAGE"; color: "#94A3B8"; font.pixelSize: 10; font.bold: true; font.letterSpacing: 0.8; Layout.alignment: Qt.AlignRight }
                        Text {
                            text: root.shortageWeight.toFixed(3) + " Qtl (" + root.shortagePct.toFixed(2) + "%)"
                            color: root.shortagePct > 10.0 ? "#F87171" : "#FBBF24"
                            font.pixelSize: 18
                            font.bold: true
                        }
                    }

                    T.Button {
                        height: 34
                        background: Rectangle { color: "#16A34A"; radius: 6 }
                        contentItem: RowLayout {
                            spacing: 6
                            Text { text: "💾 Save Voucher"; color: "#FFFFFF"; font.bold: true; font.pixelSize: 12 }
                            KbdBadge { text: "F2"; badgeColor: "#14532D"; textColor: "#86EFAC"; borderColor: "#16A34A" }
                        }
                        onClicked: root.saveMillingVoucher()
                    }
                }
            }
        }
    }

    // Confirmation Modal
    ConfirmationModal {
        id: saveConfirmModal
        titleText: "Post Milling Production Batch"
        messageText: "Are you sure you want to post Milling Batch " + root.autoVchCode + " for " + root.totalConsumedWeight.toFixed(3) + " Qtl Paddy Input (" + root.totalProducedYieldPct.toFixed(2) + "% yield)?"
        confirmBtnText: "Save & Post Batch (Enter)"
        onConfirmed: root.executeSave()
    }
}
