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
                text: "Sales Invoicing & GST Billing"
                color: "#0F172A"
                font.pixelSize: 20
                font.bold: true
            }
            Text {
                text: "Create tax invoices for Finished Rice (Steam, Parboiled, Raw), Rice Bran, and Paddy Husk sales."
                color: "#64748B"
                font.pixelSize: 12
            }
        }

        Item { Layout.fillWidth: true }

        T.Button {
            id: addInvBtn
            implicitWidth: contentItem.implicitWidth + 24
            implicitHeight: 32
            background: Rectangle { color: addInvBtn.hovered ? "#1D4ED8" : "#2563EB"; radius: 6 }
            contentItem: RowLayout {
                spacing: 6
                Text { text: "+ Create Sales Invoice"; color: "#FFFFFF"; font.bold: true; font.pixelSize: 13 }
                KbdBadge { text: "F2"; badgeColor: "#1E3A8A"; textColor: "#93C5FD"; borderColor: "#2563EB" }
            }
            onClicked: root.showNewModal()
        }
    }

    FastTable {
        Layout.fillWidth: true
        Layout.fillHeight: true
        title: "Sales Invoices History"
        model: salesModel
        headers: ["Invoice No", "Date", "Customer", "Item", "Bags", "Weight Qtl", "Rate ₹", "Taxable ₹", "GST %", "Total ₹", "Mode"]
        roleKeys: ["invoice_no", "invoice_date", "customer_name", "item_name", "bag_count", "weight_qtl", "rate_per_qtl", "taxable_amount", "gst_pct", "total_amount", "payment_mode"]
        onNewEntryRequested: root.showNewModal()
    }
}
