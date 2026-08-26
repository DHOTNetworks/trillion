import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../components"

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
                text: "Milling Process & Yield Analytics"
                color: "#0F172A"
                font.pixelSize: 20
                font.bold: true
            }
            Text {
                text: "Track Paddy milling batches, Head Rice recovery, Broken Rice (Nakku), Bran & Husk production."
                color: "#64748B"
                font.pixelSize: 12
            }
        }

        Item { Layout.fillWidth: true }

        Button {
            id: addBatchBtn
            background: Rectangle { color: addBatchBtn.hovered ? "#15803D" : "#16A34A"; radius: 6 }
            contentItem: RowLayout {
                spacing: 6
                Text { text: "+ Log Milling Batch"; color: "#FFFFFF"; font.bold: true; font.pixelSize: 13 }
                KbdBadge { text: "F2"; badgeColor: "#14532D"; textColor: "#86EFAC"; borderColor: "#16A34A" }
            }
            onClicked: root.showNewModal()
        }
    }

    FastTable {
        Layout.fillWidth: true
        Layout.fillHeight: true
        title: "Milling Batches Log"
        model: millingModel
        headers: ["Batch No", "Date", "Variety", "Paddy Input Qtl", "Head Rice Qtl", "Broken Qtl", "Bran Qtl", "Husk Qtl", "Wastage Qtl", "Yield %"]
        roleKeys: ["batch_no", "batch_date", "paddy_variety", "paddy_input_qtl", "head_rice_qtl", "broken_rice_qtl", "bran_qtl", "husk_qtl", "wastage_qtl", "yield_pct"]
        onNewEntryRequested: root.showNewModal()
    }
}
