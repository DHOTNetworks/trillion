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
            background: Rectangle { color: addVchBtn.hovered ? "#6D28D9" : "#7C3AED"; radius: 6 }
            contentItem: RowLayout {
                spacing: 6
                Text { text: "+ Post Voucher"; color: "#FFFFFF"; font.bold: true; font.pixelSize: 13 }
                KbdBadge { text: "F2"; badgeColor: "#4C1D95"; textColor: "#C4B5FD"; borderColor: "#7C3AED" }
            }
            onClicked: root.showNewModal()
        }
    }

    FastTable {
        Layout.fillWidth: true
        Layout.fillHeight: true
        title: "Cash & Bank Vouchers Register"
        model: vouchersModel
        headers: ["Voucher No", "Date", "Type", "Party", "Account", "Amount ₹", "Narration"]
        roleKeys: ["voucher_no", "voucher_date", "voucher_type", "party_name", "account_type", "amount", "narration"]
        onNewEntryRequested: root.showNewModal()
    }
}
