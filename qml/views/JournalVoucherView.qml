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
            autoVchCode = vouchersModel.get_next_voucher_no("Jrnl")
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

        for (var i = 0; i < voucherRowsModel.count; i++) {
            var row = voucherRowsModel.get(i)
            if (!row.ledgerName.trim()) {
                statusMessage = "❌ Please select a Ledger Account for Row " + (i + 1) + "."
                isError = true
                return
            }
        }

        saveConfirmModal.open()
    }

    function executeSaveVoucher() {
        var vchDate = voucherDateInput.text.trim()
        
        var drParty = ""
        var crParty = ""
        var refNote = ""

        for (var i = 0; i < voucherRowsModel.count; i++) {
            var row = voucherRowsModel.get(i)
            if (row.drcr === "Dr" && !drParty) drParty = row.ledgerName.trim()
            if (row.drcr === "Cr" && !crParty) crParty = row.ledgerName.trim()
            if (row.refNo.trim() && !refNote) refNote = row.refNo.trim()
        }

        if (typeof vouchersModel !== "undefined" && vouchersModel) {
            var ok = vouchersModel.add_journal_voucher(drParty, crParty, totalDebit, refNote, narrationInput.text.trim(), vchDate, "Journal")

            if (ok) {
                statusMessage = "✅ Journal Voucher " + autoVchCode + " saved & posted successfully!"
                isError = false
                resetForm()
                root.voucherSaved()
            } else {
                statusMessage = "❌ Failed to save Journal Voucher."
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
        titleText: "CONFIRM JOURNAL VOUCHER SAVE"
        messageText: "Are you sure you want to save & post Journal Voucher " + root.autoVchCode + " for ₹" + root.totalDebit.toFixed(2) + "?"
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
                        text: "📓 Journal Voucher Entry (F5)"
                        color: "#0F172A"
                        font.pixelSize: 18
                        font.bold: true
                    }
                    Text {
                        text: "General journal voucher for posting adjustments between ANY debit and credit ledger accounts."
                        color: "#64748B"
                        font.pixelSize: 11
                    }
                }

                Item { Layout.fillWidth: true }

                // Dedicated Mode Badge
                Rectangle {
                    width: 130; height: 32; radius: 6
                    color: "#7C3AED"
                    border.color: "#6D28D9"
                    RowLayout {
                        anchors.centerIn: parent
                        spacing: 4
                        Text { text: "F5 : Journal Voucher"; color: "#FFF"; font.pixelSize: 11; font.bold: true }
                    }
                }

                Button {
                    background: Rectangle { color: "#F1F5F9"; radius: 6; border.color: "#CBD5E1" }
                    contentItem: RowLayout {
                        spacing: 6
                        Text { text: "← Dashboard"; color: "#475569"; font.pixelSize: 12; font.bold: true }
                        KbdBadge { text: "Esc"; badgeColor: "#DC2626"; textColor: "#FFF"; borderColor: "#B91C1C" }
                    }
                    onClicked: root.cancelRequested()
                }
            }

            Rectangle { Layout.fillWidth: true; height: 1; color: "#E2E8F0" }

            // 2. VOUCHER HEADER FIELDS BAR
            RowLayout {
                Layout.fillWidth: true
                spacing: 12

                // Voucher Type Field (Dedicated "Journal Voucher" Badge)
                ColumnLayout {
                    spacing: 4
                    Layout.preferredWidth: 160
                    Text { text: "Voucher Type"; color: "#334155"; font.pixelSize: 12; font.bold: true }
                    Rectangle {
                        Layout.fillWidth: true
                        height: 36
                        radius: 6
                        color: "#F3E8FF"
                        border.color: "#7C3AED"
                        border.width: 1.5

                        Text {
                            anchors.centerIn: parent
                            text: "Journal Voucher"
                            color: "#7C3AED"
                            font.pixelSize: 13
                            font.bold: true
                        }
                    }
                }

                // Voucher No Auto Field
                CustomInput {
                    id: vchNoInput
                    label: "Voucher No (Auto)"
                    text: root.autoVchCode
                    enabled: false
                    Layout.preferredWidth: 140
                }

                // Date Picker Input
                CustomInput {
                    id: voucherDateInput
                    label: "Voucher Date (DD-MM-YYYY)"
                    text: Qt.formatDate(new Date(), "dd-MM-yyyy")
                    isRequired: true
                    Layout.preferredWidth: 160
                    onReturnPressed: rowsListView.focusRowItem(0, "ledger")
                    onDownPressed: rowsListView.focusRowItem(0, "ledger")
                }

                Item { Layout.fillWidth: true }

                // Status Notification Message Bar
                Rectangle {
                    visible: root.statusMessage !== ""
                    Layout.fillWidth: true
                    Layout.maximumWidth: 380
                    height: 36
                    radius: 6
                    color: root.isError ? "#FEF2F2" : "#F0FDF4"
                    border.color: root.isError ? "#FCA5A5" : "#86EFAC"

                    Text {
                        anchors.centerIn: parent
                        text: root.statusMessage
                        color: root.isError ? "#DC2626" : "#16A34A"
                        font.pixelSize: 12
                        font.bold: true
                        elide: Text.ElideRight
                    }
                }
            }

            Rectangle { Layout.fillWidth: true; height: 1; color: "#CBD5E1" }

            // 3. MULTI-ROW TRANSACTION GRID CONTAINER
            ColumnLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                spacing: 4

                // Table Header Row
                Rectangle {
                    Layout.fillWidth: true
                    height: 30
                    color: "#F1F5F9"
                    radius: 4

                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: 8; anchors.rightMargin: 8
                        spacing: 8

                        Text { Layout.preferredWidth: 65; text: "Dr / Cr"; color: "#475569"; font.pixelSize: 11; font.bold: true }
                        Text { Layout.fillWidth: true; Layout.preferredWidth: 230; text: "Ledger Account Name (Any Ledger)"; color: "#475569"; font.pixelSize: 11; font.bold: true }
                        Text { Layout.preferredWidth: 140; text: "Debit (₹)"; color: "#475569"; font.pixelSize: 11; font.bold: true; horizontalAlignment: Text.AlignRight }
                        Text { Layout.preferredWidth: 140; text: "Credit (₹)"; color: "#475569"; font.pixelSize: 11; font.bold: true; horizontalAlignment: Text.AlignRight }
                        Text { Layout.preferredWidth: 160; text: "Ref / Narration Note"; color: "#475569"; font.pixelSize: 11; font.bold: true }
                        Text { Layout.preferredWidth: 130; text: "Live Date Bal."; color: "#475569"; font.pixelSize: 11; font.bold: true; horizontalAlignment: Text.AlignRight }
                        Item { Layout.preferredWidth: 28 } // Delete action spacer
                    }
                }

                // Dynamic ListView for Multiple Voucher Rows
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

                            // Ledger Account ComboBox (ANY Ledger in Journal mode!)
                            CustomWhiteCombo {
                                id: itemLedgerCombo
                                Layout.fillWidth: true
                                Layout.preferredWidth: 230
                                model: (typeof partiesModel !== "undefined" && partiesModel) ? partiesModel.get_parties_list() : []
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

                            // Debit Amount Input (Enabled when Dr)
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

                            // Credit Amount Input (Enabled when Cr)
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

                            // Ref / Narration Note Input
                            CustomInput {
                                id: refInput
                                Layout.preferredWidth: 160
                                text: refNo
                                placeholderText: "Ref Note / Adj"
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

                            // Live Date Balance Badge
                            Rectangle {
                                Layout.preferredWidth: 130; height: 28; radius: 4
                                color: "#F1F5F9"
                                border.color: "#CBD5E1"
                                Text {
                                    anchors.centerIn: parent
                                    text: (typeof partiesModel !== "undefined" && partiesModel && ledgerName.trim()) ? partiesModel.get_ledger_live_balance(ledgerName.trim()) : "0.00 Dr"
                                    color: "#334155"; font.pixelSize: 11; font.bold: true; font.family: "Segoe UI, Consolas, Menlo, sans-serif"
                                }
                            }

                            // Delete Action Button
                            Button {
                                Layout.preferredWidth: 28; height: 28
                                enabled: voucherRowsModel.count > 2
                                opacity: enabled ? 1.0 : 0.4
                                background: Rectangle { color: parent.hovered ? "#FEF2F2" : "#FFFFFF"; radius: 4; border.color: parent.hovered ? "#DC2626" : "#E2E8F0" }
                                contentItem: Text { text: "✕"; color: parent.hovered ? "#DC2626" : "#94A3B8"; font.pixelSize: 12; font.bold: true; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                                onClicked: root.removeRow(index)
                            }
                        }
                    }
                }

                // Add Row Action Button
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 12

                    Button {
                        height: 32
                        background: Rectangle { color: "#EFF6FF"; radius: 6; border.color: "#BFDBFE" }
                        contentItem: RowLayout {
                            spacing: 6
                            Text { text: "➕ Add Journal Row"; color: "#2563EB"; font.bold: true; font.pixelSize: 12 }
                        }
                        onClicked: root.addNewRow()
                    }

                    Item { Layout.fillWidth: true }
                }
            }

            Rectangle { Layout.fillWidth: true; height: 1; color: "#CBD5E1" }

            // 4. FOOTER NARRATION & BALANCING TOTALS BAR
            RowLayout {
                Layout.fillWidth: true
                spacing: 12

                CustomInput {
                    id: narrationInput
                    label: "General Journal Narration / Particulars Remarks"
                    placeholderText: "Enter general journal adjustment narration notes..."
                    Layout.fillWidth: true
                }

                // Balanced Status Indicator Badge
                Rectangle {
                    height: 36
                    implicitWidth: 150
                    radius: 6
                    color: Math.abs(root.totalDebit - root.totalCredit) < 0.01 && root.totalDebit > 0 ? "#F0FDF4" : "#FEF2F2"
                    border.color: Math.abs(root.totalDebit - root.totalCredit) < 0.01 && root.totalDebit > 0 ? "#86EFAC" : "#FCA5A5"

                    Text {
                        anchors.centerIn: parent
                        text: Math.abs(root.totalDebit - root.totalCredit) < 0.01 && root.totalDebit > 0 ? "✓ BALANCED" : "⚠️ UNBALANCED"
                        color: Math.abs(root.totalDebit - root.totalCredit) < 0.01 && root.totalDebit > 0 ? "#16A34A" : "#DC2626"
                        font.pixelSize: 12
                        font.bold: true
                    }
                }

                // Total Debit Badge
                ColumnLayout {
                    spacing: 1
                    Text { text: "Total Debit (₹)"; color: "#64748B"; font.pixelSize: 10; font.bold: true }
                    Text { text: (typeof dashboardCtrl !== "undefined" && dashboardCtrl) ? dashboardCtrl.format_inr(root.totalDebit) : ("₹" + root.totalDebit.toFixed(2)); color: "#0F172A"; font.pixelSize: 15; font.bold: true }
                }

                // Total Credit Badge
                ColumnLayout {
                    spacing: 1
                    Text { text: "Total Credit (₹)"; color: "#64748B"; font.pixelSize: 10; font.bold: true }
                    Text { text: (typeof dashboardCtrl !== "undefined" && dashboardCtrl) ? dashboardCtrl.format_inr(root.totalCredit) : ("₹" + root.totalCredit.toFixed(2)); color: "#0F172A"; font.pixelSize: 15; font.bold: true }
                }

                // Save Action Button
                Button {
                    id: saveVchBtn
                    height: 38
                    implicitWidth: 130
                    background: Rectangle {
                        color: saveVchBtn.hovered ? "#15803D" : "#16A34A"
                        radius: 6
                    }
                    contentItem: RowLayout {
                        spacing: 6
                        Text { text: "💾 Save (Ctrl+S)"; color: "#FFFFFF"; font.bold: true; font.pixelSize: 13 }
                    }
                    onClicked: root.saveVoucher()
                }
            }
        }
    }
}
