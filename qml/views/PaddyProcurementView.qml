import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../components"

ColumnLayout {
    id: root
    spacing: 16

    signal showNewModal()

    // Title Bar
    RowLayout {
        Layout.fillWidth: true
        spacing: 12

        ColumnLayout {
            spacing: 2
            Text {
                text: "Paddy Procurement & Farmer Purchase"
                color: "#0F172A"
                font.pixelSize: 20
                font.bold: true
            }
            Text {
                text: "Record Paddy arrivals, calculate moisture & weight deductions, and process farmer settlements."
                color: "#64748B"
                font.pixelSize: 12
            }
        }

        Item { Layout.fillWidth: true }

        Button {
            id: addPaddyBtn
            background: Rectangle { color: addPaddyBtn.hovered ? "#15803D" : "#16A34A"; radius: 6 }
            contentItem: RowLayout {
                spacing: 6
                Text { text: "+ New Paddy Arrival Slip"; color: "#FFFFFF"; font.bold: true; font.pixelSize: 13 }
                KbdBadge { text: "F2"; badgeColor: "#14532D"; textColor: "#86EFAC"; borderColor: "#16A34A" }
            }
            onClicked: root.showNewModal()
        }
    }

    // Main Table
    FastTable {
        Layout.fillWidth: true
        Layout.fillHeight: true
        title: "Paddy Procurement History"
        model: paddyModel
        headers: ["Slip No", "Date", "Farmer", "Variety", "Bags", "Gross Qtl", "Moist %", "Deduct Qtl", "Net Qtl", "Rate ₹", "Net Amt ₹", "Status"]
        roleKeys: ["slip_no", "arrival_date", "farmer_name", "paddy_variety", "bag_count", "gross_weight_qtl", "moisture_pct", "moisture_deduction_qtl", "net_weight_qtl", "rate_per_qtl", "net_amount", "payment_status"]
        onNewEntryRequested: root.showNewModal()
    }
}
