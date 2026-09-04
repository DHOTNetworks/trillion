import QtQuick
import QtQuick.Layouts
import MahadevERP

ColumnLayout {
    id: root
    spacing: 16

    RowLayout {
        Layout.fillWidth: true
        spacing: 12

        ColumnLayout {
            spacing: 2
            Text {
                text: "Party Directory & Financial Ledger Statements"
                color: "#0F172A"
                font.pixelSize: 20
                font.bold: true
            }
            Text {
                text: "View opening balances, outstanding balances, and GST details for Farmers and Grain Merchants."
                color: "#64748B"
                font.pixelSize: 12
            }
        }
    }

    FastTable {
        Layout.fillWidth: true
        Layout.fillHeight: true
        title: "Party Directory & Balances"
        model: partiesModel
        headers: ["Party Name", "Type", "Phone", "Place", "GSTIN", "Opening Bal ₹", "Dr/Cr"]
        roleKeys: ["name", "party_type", "phone", "place", "gstin", "opening_balance", "balance_type"]
    }
}
