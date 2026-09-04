import QtQuick
import QtQuick.Templates as T
import QtQuick.Layouts
import MahadevERP

FocusScope {
    id: root
    focus: true

    signal cancelRequested()
    signal showNewModal()

    Component.onCompleted: {
        root.forceActiveFocus()
    }

    Shortcut { sequence: "F2"; onActivated: root.showNewModal() }

    Keys.onEscapePressed: function(event) {
        event.accepted = true
        root.cancelRequested()
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 16

        // Title Bar
        RowLayout {
            Layout.fillWidth: true
            spacing: 12

            ColumnLayout {
                spacing: 2
                Text {
                    text: "🌾 Milling Process & Yield Analytics"
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

            T.Button {
                id: addBatchBtn
                implicitWidth: contentItem.implicitWidth + 24
                implicitHeight: 32
                background: Rectangle { color: addBatchBtn.hovered ? "#15803D" : "#16A34A"; radius: 6 }
                contentItem: RowLayout {
                    spacing: 6
                    Text { text: "+ Log Milling Batch"; color: "#FFFFFF"; font.bold: true; font.pixelSize: 13 }
                    KbdBadge { text: "F2"; badgeColor: "#14532D"; textColor: "#86EFAC"; borderColor: "#16A34A" }
                }
                onClicked: root.showNewModal()
            }

            T.Button {
                id: backBtn
                implicitWidth: contentItem.implicitWidth + 24
                implicitHeight: 32
                background: Rectangle { color: backBtn.hovered ? "#475569" : "#334155"; radius: 6 }
                contentItem: Text {
                    text: "← Back (Esc)"
                    color: "#F8FAFC"
                    font.pixelSize: 12
                    font.bold: true
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
                onClicked: root.cancelRequested()
            }
        }

        // Main Table
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
}
