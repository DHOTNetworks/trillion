import QtQuick
import QtQuick.Templates as T
import QtQuick.Layouts
import MahadevERP

ColumnLayout {
    id: root
    spacing: 16

    signal showNewModal()

    RowLayout {
        Layout.fillWidth: true
        spacing: 12

        ColumnLayout {
            spacing: 2
            Text {
                text: "Financial Vouchers & Cash/Bank Book"
                color: "#0F172A"
                font.pixelSize: 20
                font.bold: true
            }
            Text {
                text: "Post Cash & Bank receipts, Farmer payments, Transporter & Hamali expense vouchers."
                color: "#64748B"
                font.pixelSize: 12
            }
        }

        Item { Layout.fillWidth: true }

        T.Button {
            id: addVchBtn
            implicitWidth: contentItem.implicitWidth + 24
            implicitHeight: 32
            background: Rectangle { color: addVchBtn.hovered ? "#6D28D9" : "#7C3AED"; radius: 6 }
            contentItem: RowLayout {
                spacing: 6
                Item { Layout.fillWidth: true }
                Text { text: "+ Post Voucher"; color: "#FFFFFF"; font.bold: true; font.pixelSize: 13 }
                KbdBadge { text: "F2"; badgeColor: "#4C1D95"; textColor: "#C4B5FD"; borderColor: "#7C3AED" }
                Item { Layout.fillWidth: true }
            }
            onClicked: root.showNewModal()
        }
    }

    function openVoucher(rowIndex) {
        if (typeof vouchersModel === "undefined" || !vouchersModel || typeof window === "undefined") return
        var row = vouchersModel.get(rowIndex)
        if (!row) return

        var vType = row.voucher_type || ""
        var vNo = ("" + (row.voucher_no || "")).trim()
        var vId = row.id || 0
        var vDate = "" + (row.voucher_date || "")

        if (vType === "Sale" || vType === "Sales") {
            window.pendingEditInvoiceNo = vNo
            window.pendingEditVoucherNo = vNo
            window.pendingEditVoucherId = vId
            window.pendingEditVoucherDate = vDate
            window.navigateToView(14)
            return
        }

        if (vType === "Purc" || vType === "Purchase") {
            window.pendingEditInvoiceNo = vNo
            window.pendingEditVoucherNo = vNo
            window.pendingEditVoucherId = vId
            window.pendingEditVoucherDate = vDate
            window.navigateToView(15)
            return
        }

        if (vType === "ChPt" || vType === "Pymt" || vType === "Payment") {
            window.targetChequeMode = "PAYMENT"
            window.pendingEditVoucherId = vId
            window.pendingEditVoucherNo = vNo
            window.pendingEditVoucherDate = vDate
            window.navigateToView(16)
            return
        }

        if (vType === "ChRt" || vType === "Rcpt" || vType === "Receipt") {
            window.targetChequeMode = "RECEIPT"
            window.pendingEditVoucherId = vId
            window.pendingEditVoucherNo = vNo
            window.pendingEditVoucherDate = vDate
            window.navigateToView(16)
            return
        }

        if (vType === "Jrnl" || vType === "Journal" || vType === "Jour") {
            window.pendingEditVoucherId = vId
            window.pendingEditVoucherNo = vNo
            window.pendingEditVoucherDate = vDate
            window.navigateToView(17)
            return
        }

        if (vType === "Mill" || vType === "Milling" || vType === "Prod" || vType === "ML") {
            window.pendingEditVoucherId = vId
            window.pendingEditVoucherNo = vNo
            window.pendingEditVoucherDate = vDate
            window.navigateToView(18)
            return
        }

        if (vType === "JFrm" || vType === "J-Form" || vType === "JForm") {
            window.pendingEditVoucherId = vId
            window.pendingEditVoucherNo = vNo
            window.pendingEditVoucherDate = vDate
            window.navigateToView(23)
            return
        }

        if (vType === "TDS" || vType === "Tds") {
            window.targetTdsVoucherId = vId
            window.pendingEditVoucherId = vId
            window.pendingEditVoucherNo = vNo
            window.navigateToView(24)
            return
        }

        if (vType === "DbNt" || vType === "CrNt" || vType === "Debit Note" || vType === "Credit Note" || vType === "DN" || vType === "CN") {
            window.pendingEditVoucherId = vId
            window.pendingEditVoucherNo = vNo
            window.pendingEditVoucherDate = vDate
            window.navigateToView(28)
            return
        }

        window.pendingEditVoucherId = vId
        window.pendingEditVoucherNo = vNo
        window.pendingEditVoucherDate = vDate
        window.navigateToView(16)
    }

    FastTable {
        Layout.fillWidth: true
        Layout.fillHeight: true
        title: "Cash & Bank Vouchers Register"
        model: vouchersModel
        headers: ["Voucher No", "Date", "Type", "Party", "Account", "Amount ₹", "Narration"]
        roleKeys: ["voucher_no", "voucher_date", "voucher_type", "party_name", "account_type", "amount", "narration"]
        onNewEntryRequested: root.showNewModal()
        onRowClicked: function(rowIndex) {
            root.openVoucher(rowIndex)
        }
    }
}
