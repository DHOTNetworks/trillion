import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../components"
import "../dialogs"

FocusScope {
    id: root
    focus: true

    signal cancelRequested()
    signal voucherSaved()

    // Mode: "Payment" (F3) or "Receipt" (F4)
    property string voucherMode: "Payment"

    property string autoVoucherNo: ""
    property string autoVchCode: ""
    
    property real totalDebit: 0.0
    property real totalCredit: 0.0

    property string statusMessage: ""
    property bool isError: false

    ListModel {
        id: voucherRowsModel
    }

    Component.onCompleted: {
        resetForm()
        Qt.callLater(function() {
            voucherDateInput.focusInput = true
        })
    }

    onVoucherModeChanged: {
        resetForm()
    }

    function resetForm() {
        cursorMax()
        voucherRowsModel.clear()
        // Clean empty entries (zero prefilled data)
        voucherRowsModel.append({ drcr: "Dr", ledgerName: "", debitAmt: "", creditAmt: "", refNo: "" })
        voucherRowsModel.append({ drcr: "Cr", ledgerName: "", debitAmt: "", creditAmt: "", refNo: "" })

        statusMessage = ""
        isError = false
        recalculateTotals()
    }

    function cursorMax() {
        if (typeof vouchersModel !== "undefined" && vouchersModel) {
            var prefix = (root.voucherMode === "Receipt" || root.voucherMode === "ChRt") ? "ChRt" : "ChPt"
            autoVchCode = vouchersModel.get_next_voucher_no(prefix)
            autoVoucherNo = autoVchCode
        } else {
            autoVchCode = ""
            autoVoucherNo = ""
        }
    }

    function addNewRow() {
        var defaultType = "Cr"
        if (voucherRowsModel.count > 0) {
            var lastType = voucherRowsModel.get(voucherRowsModel.count - 1).drcr
            defaultType = lastType === "Dr" ? "Cr" : "Dr"
        }
        voucherRowsModel.append({ drcr: defaultType, ledgerName: "", debitAmt: "", creditAmt: "", refNo: "" })
        recalculateTotals()
        Qt.callLater(function() {
            rowsListView.positionViewAtIndex(voucherRowsModel.count - 1, ListView.Contain)
        })
    }

    function removeRow(idx) {
        if (voucherRowsModel.count > 2) {
            voucherRowsModel.remove(idx)
            recalculateTotals()
        }
    }

    function recalculateTotals() {
        var sumDr = 0.0
        var sumCr = 0.0

        for (var i = 0; i < voucherRowsModel.count; i++) {
            var r = voucherRowsModel.get(i)
            if (r.drcr === "Dr") {
                sumDr += parseFloat(r.debitAmt) || 0.0
            } else {
                sumCr += parseFloat(r.creditAmt) || 0.0
            }
        }

        totalDebit = sumDr
        totalCredit = sumCr
    }

    function saveVoucher() {
        statusMessage = ""

        if (voucherRowsModel.count < 2) {
            statusMessage = "❌ Please enter at least two transaction rows."
            isError = true
            return
        }

        if (Math.abs(totalDebit - totalCredit) > 0.01) {
            statusMessage = "❌ Total Debit (₹" + totalDebit.toFixed(2) + ") does not equal Total Credit (₹" + totalCredit.toFixed(2) + ")."
            isError = true
            return
        }

        if (totalDebit <= 0) {
            statusMessage = "❌ Please enter a valid Voucher Amount."
            isError = true
            return
        }

        var vchType = voucherMode === "Journal" ? "Journal" : (voucherMode === "Payment" ? "Cheque Payment" : "Cheque Receipt")
        var vchDate = voucherDateInput.text.trim()
        
        var drParty = ""
        var crParty = ""
        var chqNo = ""

        for (var i = 0; i < voucherRowsModel.count; i++) {
            var row = voucherRowsModel.get(i)
            if (!row.ledgerName.trim()) {
                statusMessage = "❌ Please select a Ledger Account for Row " + (i + 1) + "."
                isError = true
                return
            }
            if (row.drcr === "Dr" && !drParty) drParty = row.ledgerName.trim()
            if (row.drcr === "Cr" && !crParty) crParty = row.ledgerName.trim()
            if (row.refNo.trim() && !chqNo) chqNo = row.refNo.trim()
        }

        saveConfirmModal.open()
    }

    function executeSaveVoucher() {
        var vchType = voucherMode === "Journal" ? "Journal" : (voucherMode === "Payment" ? "Cheque Payment" : "Cheque Receipt")
        var vchDate = voucherDateInput.text.trim()
        
        var drParty = ""
        var crParty = ""
        var chqNo = ""

        for (var i = 0; i < voucherRowsModel.count; i++) {
            var row = voucherRowsModel.get(i)
            if (row.drcr === "Dr" && !drParty) drParty = row.ledgerName.trim()
            if (row.drcr === "Cr" && !crParty) crParty = row.ledgerName.trim()
            if (row.refNo.trim() && !chqNo) chqNo = row.refNo.trim()
        }

        if (typeof vouchersModel !== "undefined" && vouchersModel) {
            var ok = vouchersModel.add_cheque_voucher(vchType, drParty, crParty, totalDebit, chqNo, "", vchDate)
            if (ok) {
                statusMessage = "✅ " + vchType + " " + autoVchCode + " saved & posted successfully!"
                isError = false
                resetForm()
                root.voucherSaved()
            } else {
                statusMessage = "❌ Failed to save Voucher."
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
        titleText: "CONFIRM VOUCHER SAVE"
        messageText: "Are you sure you want to save & post " + (root.voucherMode === "Journal" ? "Journal Voucher " : (root.voucherMode === "Payment" ? "Cheque Payment " : "Cheque Receipt ")) + root.autoVchCode + " for ₹" + root.totalDebit.toFixed(2) + "?"
        onConfirmed: root.executeSaveVoucher()
    }

    Keys.onEscapePressed: function(event) {
        event.accepted = true
        root.cancelRequested()
    }

    // MODERN SINGLE SLATE CARD CONTAINER
    Rectangle {
        anchors.fill: parent
        color: "#FFFFFF"
        border.color: "#CBD5E1"
        border.width: 1
        radius: 8

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 12
            spacing: 8

            // 1. TOP TITLE HEADER BAR
            RowLayout {
                Layout.fillWidth: true
                spacing: 10

                ColumnLayout {
                    spacing: 1
                    Text {
                        text: root.voucherMode === "Payment" ? "💳 Cheque / Bank Payment Voucher (F3)" : "🏦 Cheque / Bank Receipt Voucher (F4)"
                        color: "#0F172A"
                        font.pixelSize: 18
                        font.bold: true
                    }
                    Text {
                        text: "Multi-entry double-entry cheque voucher with full Enter, Esc, and Arrow Key focus navigation."
                        color: "#64748B"
                        font.pixelSize: 11
                    }
                }

                Item { Layout.fillWidth: true }

                // Mode Badges
                RowLayout {
                    spacing: 6

                    Rectangle {
                        width: 105; height: 32; radius: 6
                        color: root.voucherMode === "Payment" ? "#DC2626" : "#F1F5F9"
                        border.color: root.voucherMode === "Payment" ? "#991B1B" : "#CBD5E1"
                        Text { anchors.centerIn: parent; text: "F3 : Payment"; color: root.voucherMode === "Payment" ? "#FFF" : "#475569"; font.pixelSize: 11; font.bold: true }
                        MouseArea { anchors.fill: parent; onClicked: root.voucherMode = "Payment" }
                    }

                    Rectangle {
                        width: 100; height: 32; radius: 6
                        color: root.voucherMode === "Receipt" ? "#16A34A" : "#F1F5F9"
                        border.color: root.voucherMode === "Receipt" ? "#15803D" : "#CBD5E1"
                        Text { anchors.centerIn: parent; text: "F4 : Receipt"; color: root.voucherMode === "Receipt" ? "#FFF" : "#475569"; font.pixelSize: 11; font.bold: true }
                        MouseArea { anchors.fill: parent; onClicked: root.voucherMode = "Receipt" }
                    }
                }

                Button {
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
                height: 26
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

            // 2. VOUCHER CONTROLS HEADER BAR
            RowLayout {
                Layout.fillWidth: true
                spacing: 12

                ColumnLayout {
                    spacing: 1
                    Text { text: "Voucher Type"; color: "#475569"; font.pixelSize: 10; font.bold: true }
                    Rectangle {
                        implicitWidth: 160
                        implicitHeight: 34
                        color: root.voucherMode === "Payment" ? "#FEF2F2" : "#F0FDF4"
                        border.color: root.voucherMode === "Payment" ? "#FCA5A5" : "#86EFAC"
                        radius: 6
                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: 8; anchors.rightMargin: 8
                            Text { text: root.voucherMode === "Payment" ? "Cheque Payment" : "Cheque Receipt"; color: root.voucherMode === "Payment" ? "#DC2626" : "#16A34A"; font.pixelSize: 11; font.bold: true }
                        }
                    }
                }

                ColumnLayout {
                    spacing: 1
                    Text { text: "Voucher No (Auto)"; color: "#64748B"; font.pixelSize: 10; font.bold: true }
                    Rectangle {
                        implicitWidth: 120
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
                    Text { text: "Voucher Date"; color: "#0F172A"; font.pixelSize: 10; font.bold: true }
                    CustomInput {
                        id: voucherDateInput
                        text: Qt.formatDate(new Date(), "dd-MM-yyyy")
                        placeholderText: "DD-MM-YYYY"
                        Layout.preferredWidth: 110
                        onReturnPressed: rowsListView.focusRowItem(0, "ledger")
                        onRightPressed: rowsListView.focusRowItem(0, "ledger")
                    }
                }

                Item { Layout.fillWidth: true }

                Button {
                    background: Rectangle { color: "#EFF6FF"; radius: 6; border.color: "#93C5FD" }
                    contentItem: RowLayout {
                        spacing: 4
                        Text { text: "+ Add Ledger Line"; color: "#2563EB"; font.bold: true; font.pixelSize: 11 }
                    }
                    onClicked: {
                        root.addNewRow()
                        Qt.callLater(function() {
                            rowsListView.focusRowItem(voucherRowsModel.count - 1, "ledger")
                        })
                    }
                }
            }

            // 3. MAIN DYNAMIC MULTI-ROW PARTICULARS TABLE (FILLS 100% VERTICAL HEIGHT!)
            Rectangle {
                Layout.fillWidth: true
                Layout.fillHeight: true
                color: "#FFFFFF"
                border.color: "#CBD5E1"
                border.width: 1
                radius: 6

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 6
                    spacing: 4

                    // Table Header Bar
                    Rectangle {
                        Layout.fillWidth: true
                        height: 32
                        color: "#F8FAFC"
                        border.color: "#E2E8F0"

                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: 8; anchors.rightMargin: 8
                            spacing: 8

                            Text { Layout.preferredWidth: 65; text: "Dr/Cr"; color: "#475569"; font.pixelSize: 11; font.bold: true }
                            Text { Layout.fillWidth: true; Layout.preferredWidth: 230; text: "Ledger Account Name"; color: "#475569"; font.pixelSize: 11; font.bold: true }
                            Text { Layout.preferredWidth: 140; text: "Debit (₹)"; color: "#475569"; font.pixelSize: 11; font.bold: true; horizontalAlignment: Text.AlignRight }
                            Text { Layout.preferredWidth: 140; text: "Credit (₹)"; color: "#475569"; font.pixelSize: 11; font.bold: true; horizontalAlignment: Text.AlignRight }
                            Text { Layout.preferredWidth: 160; text: "Cheque / Ref No."; color: "#475569"; font.pixelSize: 11; font.bold: true }
                            Text { Layout.preferredWidth: 130; text: "Live Date Bal."; color: "#475569"; font.pixelSize: 11; font.bold: true; horizontalAlignment: Text.AlignRight }
                            Item { Layout.preferredWidth: 28 } // Delete action spacer
                        }
                    }

                    // Dynamic ListView for Multiple Voucher Rows (Takes FULL height of table container!)
                    ListView {
                        id: rowsListView
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        clip: true
                        spacing: 4
                        model: voucherRowsModel

                        function focusRowItem(rowIdx, targetField) {
                            if (rowIdx >= 0 && rowIdx < voucherRowsModel.count) {
                                rowsListView.currentIndex = rowIdx
                                rowsListView.positionViewAtIndex(rowIdx, ListView.Contain)
                                var item = rowsListView.itemAtIndex(rowIdx)
                                if (item && typeof item.focusField !== "undefined") {
                                    item.focusField(targetField)
                                }
                            }
                        }

                        delegate: Rectangle {
                            id: rowItem
                            width: rowsListView.width
                            height: 40
                            color: index % 2 === 0 ? "#FFFFFF" : "#F8FAFC"
                            border.color: "#E2E8F0"

                            function focusField(field) {
                                if (field === "drcr") drcrCombo.focusAndOpen()
                                else if (field === "ledger") itemLedgerCombo.focusAndOpen()
                                else if (field === "amount") {
                                    if (drcr === "Dr") dbInput.focusInput = true
                                    else crInput.focusInput = true
                                }
                                else if (field === "ref") refInput.focusInput = true
                            }

                            RowLayout {
                                anchors.fill: parent
                                anchors.leftMargin: 8; anchors.rightMargin: 8
                                spacing: 8

                                // Selectable Dr / Cr Dropdown
                                CustomWhiteCombo {
                                    id: drcrCombo
                                    Layout.preferredWidth: 65
                                    model: ["Dr", "Cr"]
                                    currentIndex: drcr === "Cr" ? 1 : 0
                                    onCurrentTextChanged: {
                                        if (currentText !== drcr) {
                                            var oldDrCr = drcr
                                            var oldDb = debitAmt
                                            var oldCr = creditAmt
                                            voucherRowsModel.setProperty(index, "drcr", currentText)

                                            if (currentText === "Cr" && oldDrCr === "Dr") {
                                                voucherRowsModel.setProperty(index, "creditAmt", oldDb)
                                                voucherRowsModel.setProperty(index, "debitAmt", "")
                                            } else if (currentText === "Dr" && oldDrCr === "Cr") {
                                                voucherRowsModel.setProperty(index, "debitAmt", oldCr)
                                                voucherRowsModel.setProperty(index, "creditAmt", "")
                                            }
                                            root.recalculateTotals()
                                        }
                                    }
                                    onReturnPressed: itemLedgerCombo.focusAndOpen()
                                    onRightPressed: itemLedgerCombo.focusAndOpen()
                                    onLeftPressed: {
                                        if (index > 0) {
                                            rowsListView.focusRowItem(index - 1, "ref")
                                        }
                                    }
                                    onUpPressed: {
                                        if (index > 0) {
                                            rowsListView.focusRowItem(index - 1, "drcr")
                                        }
                                    }
                                    onDownPressed: {
                                        if (index < voucherRowsModel.count - 1) {
                                            rowsListView.focusRowItem(index + 1, "drcr")
                                        }
                                    }
                                }

                                // Ledger Account ComboBox (Width 230)
                                CustomWhiteCombo {
                                    id: itemLedgerCombo
                                    Layout.fillWidth: true
                                    Layout.preferredWidth: 230
                                    model: (typeof partiesModel !== "undefined" && partiesModel) ? 
                                           (root.voucherMode === "Journal" ? partiesModel.get_parties_list() : 
                                            (((drcr === "Dr" && root.voucherMode === "Receipt") || (drcr === "Cr" && root.voucherMode === "Payment")) ? 
                                             partiesModel.get_bank_accounts_list() : partiesModel.get_parties_list())) : []
                                    editText: ledgerName
                                    onCurrentTextChanged: {
                                        if (currentText !== ledgerName) {
                                            voucherRowsModel.setProperty(index, "ledgerName", currentText)
                                        }
                                    }
                                    onReturnPressed: {
                                        if (!ledgerName.trim()) {
                                            if (voucherRowsModel.count >= 2 && root.totalDebit > 0 && Math.abs(root.totalDebit - root.totalCredit) < 0.01) {
                                                if (index === voucherRowsModel.count - 1 && voucherRowsModel.count > 2) {
                                                    voucherRowsModel.remove(index)
                                                }
                                                root.saveVoucher()
                                            }
                                        } else {
                                            if (drcr === "Dr") dbInput.focusInput = true
                                            else crInput.focusInput = true
                                        }
                                    }
                                    onRightPressed: {
                                        if (drcr === "Dr") dbInput.focusInput = true
                                        else crInput.focusInput = true
                                    }
                                    onLeftPressed: drcrCombo.focusAndOpen()
                                    onUpPressed: {
                                        if (index > 0) {
                                            rowsListView.focusRowItem(index - 1, "ledger")
                                        }
                                    }
                                    onDownPressed: {
                                        if (index < voucherRowsModel.count - 1) {
                                            rowsListView.focusRowItem(index + 1, "ledger")
                                        }
                                    }
                                }

                                // Debit Amount Input (Enabled when Dr, width 140)
                                CustomInput {
                                    id: dbInput
                                    Layout.preferredWidth: 140
                                    text: debitAmt
                                    placeholderText: "0.00"
                                    enabled: drcr === "Dr"
                                    visible: drcr === "Dr"
                                    onTextChanged: {
                                        if (text !== debitAmt) {
                                            voucherRowsModel.setProperty(index, "debitAmt", text)
                                            root.recalculateTotals()
                                        }
                                    }
                                    onReturnPressed: refInput.focusInput = true
                                    onRightPressed: refInput.focusInput = true
                                    onLeftPressed: itemLedgerCombo.focusAndOpen()
                                    onUpPressed: {
                                        if (index > 0) {
                                            rowsListView.focusRowItem(index - 1, "amount")
                                        }
                                    }
                                    onDownPressed: {
                                        if (index < voucherRowsModel.count - 1) {
                                            rowsListView.focusRowItem(index + 1, "amount")
                                        }
                                    }
                                }

                                Item {
                                    visible: drcr !== "Dr"
                                    Layout.preferredWidth: 140
                                    Text { anchors.centerIn: parent; text: "-"; color: "#CBD5E1" }
                                }

                                // Credit Amount Input (Enabled when Cr, width 140)
                                CustomInput {
                                    id: crInput
                                    Layout.preferredWidth: 140
                                    text: creditAmt
                                    placeholderText: "0.00"
                                    enabled: drcr === "Cr"
                                    visible: drcr === "Cr"
                                    onTextChanged: {
                                        if (text !== creditAmt) {
                                            voucherRowsModel.setProperty(index, "creditAmt", text)
                                            root.recalculateTotals()
                                        }
                                    }
                                    onReturnPressed: refInput.focusInput = true
                                    onRightPressed: refInput.focusInput = true
                                    onLeftPressed: itemLedgerCombo.focusAndOpen()
                                    onUpPressed: {
                                        if (index > 0) {
                                            rowsListView.focusRowItem(index - 1, "amount")
                                        }
                                    }
                                    onDownPressed: {
                                        if (index < voucherRowsModel.count - 1) {
                                            rowsListView.focusRowItem(index + 1, "amount")
                                        }
                                    }
                                }

                                Item {
                                    visible: drcr !== "Cr"
                                    Layout.preferredWidth: 140
                                    Text { anchors.centerIn: parent; text: "-"; color: "#CBD5E1" }
                                }

                                // Cheque / Ref No. Input (Width 160)
                                CustomInput {
                                    id: refInput
                                    Layout.preferredWidth: 160
                                    text: refNo
                                    placeholderText: "Ch. No. / Ref"
                                    onTextChanged: {
                                        if (text !== refNo) {
                                            voucherRowsModel.setProperty(index, "refNo", text)
                                        }
                                    }
                                    onReturnPressed: {
                                        if (index < voucherRowsModel.count - 1) {
                                            rowsListView.focusRowItem(index + 1, "ledger")
                                        } else {
                                            root.addNewRow()
                                            Qt.callLater(function() {
                                                rowsListView.focusRowItem(voucherRowsModel.count - 1, "ledger")
                                            })
                                        }
                                    }
                                    onRightPressed: {
                                        if (index < voucherRowsModel.count - 1) {
                                            rowsListView.focusRowItem(index + 1, "drcr")
                                        }
                                    }
                                    onLeftPressed: {
                                        if (drcr === "Dr") dbInput.focusInput = true
                                        else crInput.focusInput = true
                                    }
                                    onUpPressed: {
                                        if (index > 0) {
                                            rowsListView.focusRowItem(index - 1, "ref")
                                        }
                                    }
                                    onDownPressed: {
                                        if (index < voucherRowsModel.count - 1) {
                                            rowsListView.focusRowItem(index + 1, "ref")
                                        }
                                    }
                                }

                                // Live Date Balance Badge (Width 130)
                                Rectangle {
                                    Layout.preferredWidth: 130; height: 28; radius: 4
                                    color: "#F1F5F9"
                                    border.color: "#CBD5E1"
                                    Text {
                                        anchors.centerIn: parent
                                        text: (typeof partiesModel !== "undefined" && partiesModel && ledgerName) ? partiesModel.get_ledger_live_balance(ledgerName) : "0.00 Dr"
                                        color: "#0F172A"
                                        font.pixelSize: 11
                                        font.bold: true
                                    }
                                }

                                // Row Remove Button
                                Item {
                                    Layout.preferredWidth: 28
                                    Button {
                                        anchors.centerIn: parent
                                        width: 24; height: 22
                                        visible: voucherRowsModel.count > 2
                                        background: Rectangle { color: "#FEE2E2"; radius: 4 }
                                        contentItem: Text { text: "✕"; color: "#DC2626"; font.bold: true; font.pixelSize: 10; horizontalAlignment: Text.AlignHCenter }
                                        onClicked: root.removeRow(index)
                                    }
                                }
                            }
                        }
                    }

                    // 4. SUMMARY TOTAL BAR
                    Rectangle {
                        Layout.fillWidth: true
                        height: 34
                        color: "#EFF6FF"
                        border.color: "#BFDBFE"
                        radius: 4

                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: 12; anchors.rightMargin: 12
                            spacing: 8

                            Text {
                                text: "Grand Total :"
                                color: "#0F172A"
                                font.pixelSize: 12
                                font.bold: true
                            }

                            Item { Layout.fillWidth: true }

                            // Difference / Balance Indicator
                            Rectangle {
                                width: 140; height: 24; radius: 4
                                color: Math.abs(root.totalDebit - root.totalCredit) < 0.01 ? "#DCFCE7" : "#FEE2E2"
                                border.color: Math.abs(root.totalDebit - root.totalCredit) < 0.01 ? "#86EFAC" : "#FCA5A5"
                                Text {
                                    anchors.centerIn: parent
                                    text: Math.abs(root.totalDebit - root.totalCredit) < 0.01 ? "Balanced ✓" : ("Diff: ₹" + Math.abs(root.totalDebit - root.totalCredit).toFixed(2))
                                    color: Math.abs(root.totalDebit - root.totalCredit) < 0.01 ? "#166534" : "#991B1B"
                                    font.pixelSize: 10
                                    font.bold: true
                                }
                            }

                            Text {
                                Layout.preferredWidth: 140
                                text: (typeof dashboardCtrl !== "undefined" && dashboardCtrl) ? dashboardCtrl.format_inr(root.totalDebit) : ("₹" + root.totalDebit.toFixed(2))
                                color: "#2563EB"
                                font.pixelSize: 13
                                font.bold: true
                                horizontalAlignment: Text.AlignRight
                            }

                            Text {
                                Layout.preferredWidth: 140
                                text: (typeof dashboardCtrl !== "undefined" && dashboardCtrl) ? dashboardCtrl.format_inr(root.totalCredit) : ("₹" + root.totalCredit.toFixed(2))
                                color: "#2563EB"
                                font.pixelSize: 13
                                font.bold: true
                                horizontalAlignment: Text.AlignRight
                            }

                            Item { Layout.preferredWidth: 328 }
                        }
                    }
                }
            }

            // 5. BOTTOM ACTIONS BAR
            RowLayout {
                Layout.fillWidth: true
                spacing: 10

                Item { Layout.fillWidth: true }

                Button {
                    id: saveBtn
                    background: Rectangle { color: saveBtn.hovered ? "#1D4ED8" : "#2563EB"; radius: 6 }
                    contentItem: RowLayout {
                        spacing: 6
                        Text { text: "Save Voucher"; color: "#FFFFFF"; font.bold: true; font.pixelSize: 12 }
                        KbdBadge { text: "Ctrl+S"; badgeColor: "#1E40AF"; textColor: "#93C5FD"; borderColor: "#2563EB" }
                    }
                    onClicked: root.saveVoucher()
                }

                Button {
                    id: cancelBtn
                    background: Rectangle { color: cancelBtn.hovered ? "#E2E8F0" : "#F1F5F9"; radius: 6; border.color: "#CBD5E1" }
                    contentItem: RowLayout {
                        spacing: 6
                        Text { text: "Cancel"; color: "#475569"; font.bold: true; font.pixelSize: 12 }
                        KbdBadge { text: "Esc"; badgeColor: "#DC2626"; textColor: "#FFF"; borderColor: "#B91C1C" }
                    }
                    onClicked: root.cancelRequested()
                }
            }
        }
    }
}
