import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../components"

FocusScope {
    id: root
    focus: true

    signal cancelRequested()
    signal openNewMillingRequested()

    property string searchQuery: ""
    property string selectedVariety: "All Varieties"
    property var allBatches: []
    property int selectedBatchIndex: -1
    property string activeBatchNo: ""
    property var activeBatchItems: []

    property int totalBatchesCount: 0
    property string totalPaddyMilledFmt: "0.00"
    property string totalHeadRiceProducedFmt: "0.00"
    property string totalBranProducedFmt: "0.00"
    property string totalBrokenProducedFmt: "0.00"
    property string totalHuskProducedFmt: "0.00"
    property string totalWastageMilledFmt: "0.00"
    property string avgYieldPctFmt: "0.00%"

    Component.onCompleted: {
        reloadStatementData()
        Qt.callLater(function() { root.forceActiveFocus() })
    }

    function reloadStatementData() {
        if (typeof millingModel !== "undefined" && millingModel) {
            allBatches = millingModel.get_milling_statement("", "", selectedVariety)
            var totals = millingModel.get_milling_totals("", "")
            totalBatchesCount = totals.total_batches || allBatches.length
            totalPaddyMilledFmt = totals.total_paddy_fmt || "0.00"
            totalHeadRiceProducedFmt = totals.total_head_rice_fmt || "0.00"
            totalBranProducedFmt = totals.total_bran_fmt || "0.00"
            totalBrokenProducedFmt = totals.total_broken_fmt || "0.00"
            totalHuskProducedFmt = totals.total_husk_fmt || "0.00"
            totalWastageMilledFmt = totals.total_wastage_fmt || "0.00"
            avgYieldPctFmt = totals.avg_yield_fmt || "0.00%"
        } else {
            allBatches = []
        }
        filterAndPopulateBatches()
    }

    function filterAndPopulateBatches() {
        batchListModel.clear()
        var query = searchQuery.trim().toLowerCase()

        for (var i = 0; i < allBatches.length; i++) {
            var b = allBatches[i]
            var bNo = b.batch_no ? b.batch_no.toString() : ""
            var bDate = b.batch_date ? b.batch_date.toString() : ""
            var bVar = b.paddy_variety ? b.paddy_variety.toString() : "Paddy Basmati"
            var bNarr = b.narration ? b.narration.toString() : ""

            if (query !== "") {
                if (bNo.toLowerCase().indexOf(query) === -1 &&
                    bDate.toLowerCase().indexOf(query) === -1 &&
                    bVar.toLowerCase().indexOf(query) === -1 &&
                    bNarr.toLowerCase().indexOf(query) === -1) {
                    continue
                }
            }

            batchListModel.append({
                "id": b.id || (i + 1),
                "batchNo": bNo,
                "batchDate": bDate,
                "paddyVariety": bVar,
                "paddyInput": b.paddy_input_fmt || (parseFloat(b.paddy_input_qtl) || 0.0).toFixed(3),
                "headRice": b.head_rice_fmt || (parseFloat(b.head_rice_qtl) || 0.0).toFixed(3),
                "brokenRice": b.broken_rice_fmt || (parseFloat(b.broken_rice_qtl) || 0.0).toFixed(3),
                "bran": b.bran_fmt || (parseFloat(b.bran_qtl) || 0.0).toFixed(3),
                "husk": b.husk_fmt || (parseFloat(b.husk_qtl) || 0.0).toFixed(3),
                "wastage": b.wastage_fmt || (parseFloat(b.wastage_qtl) || 0.0).toFixed(3),
                "yieldPct": b.yield_pct_fmt || ((parseFloat(b.yield_pct) || 0.0).toFixed(2) + "%"),
                "narration": bNarr
            })
        }

        if (batchListModel.count > 0) {
            selectBatch(0)
        } else {
            selectedBatchIndex = -1
            activeBatchNo = ""
            activeItemsModel.clear()
        }
    }

    function selectBatch(idx) {
        if (idx >= 0 && idx < batchListModel.count) {
            selectedBatchIndex = idx
            var row = batchListModel.get(idx)
            activeBatchNo = row.batchNo
            batchListView.currentIndex = idx
            batchListView.positionViewAtIndex(idx, ListView.Contain)
            loadBatchItems(activeBatchNo)
        }
    }

    function loadBatchItems(bNo) {
        activeItemsModel.clear()
        if (typeof millingModel !== "undefined" && millingModel && bNo) {
            var items = millingModel.get_batch_items(bNo)
            for (var i = 0; i < items.length; i++) {
                var it = items[i]
                var drcr = it.drcr || (it.row_no === 1 ? "Cr" : "Dr")
                var bg = parseInt(it.bags) || 0
                var pct = parseFloat(it.percentage) || 0.0

                activeItemsModel.append({
                    "rowNo": it.row_no || (i + 1),
                    "drcr": drcr,
                    "isInput": drcr === "Cr",
                    "itemName": it.item_name || ("Item #" + it.item_code),
                    "yieldPct": pct > 0 ? (pct.toFixed(2) + "%") : "-",
                    "bags": bg.toString(),
                    "weight": it.weight_fmt || (parseFloat(it.weight_qtl) || 0.0).toFixed(3),
                    "rate": it.rate_fmt || "-",
                    "amount": it.amount_fmt || "₹0.00",
                    "narration": it.narration || ""
                })
            }
        }
    }

    ListModel { id: batchListModel }
    ListModel { id: activeItemsModel }

    // Keyboard Shortcuts
    Shortcut { sequence: "F2"; onActivated: root.openNewMillingRequested() }
    Shortcut { sequence: "Ctrl+F"; onActivated: searchInput.focusInput = true }

    Keys.onUpPressed: function(event) {
        event.accepted = true
        if (selectedBatchIndex > 0) selectBatch(selectedBatchIndex - 1)
    }
    Keys.onDownPressed: function(event) {
        event.accepted = true
        if (selectedBatchIndex < batchListModel.count - 1) selectBatch(selectedBatchIndex + 1)
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 14
        spacing: 10

        // 1. TOP TITLE & ACTION HEADER
        Rectangle {
            Layout.fillWidth: true
            height: 48
            color: "#0F172A"
            radius: 8

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 14; anchors.rightMargin: 14
                spacing: 12

                Text {
                    text: "🌾 Milling Statement & Production Register"
                    color: "#FFFFFF"
                    font.pixelSize: 16
                    font.bold: true
                }

                Item { Layout.fillWidth: true }

                // Search Box
                CustomInput {
                    id: searchInput
                    Layout.preferredWidth: 260
                    placeholderText: "🔍 Search Batch No, Date, Variety..."
                    text: root.searchQuery
                    onTextChanged: {
                        root.searchQuery = text
                        root.filterAndPopulateBatches()
                    }
                }

                Button {
                    background: Rectangle { color: "#16A34A"; radius: 6 }
                    contentItem: RowLayout {
                        spacing: 6
                        Text { text: "+ Log Milling Batch"; color: "#FFFFFF"; font.pixelSize: 11; font.bold: true }
                        KbdBadge { text: "F2"; badgeColor: "#14532D"; textColor: "#86EFAC"; borderColor: "#16A34A" }
                    }
                    onClicked: root.openNewMillingRequested()
                }

                Button {
                    background: Rectangle { color: "#334155"; radius: 6 }
                    contentItem: Text { text: "← Back (Esc)"; color: "#F8FAFC"; font.pixelSize: 11; font.bold: true }
                    onClicked: root.cancelRequested()
                }
            }
        }

        // 2. SUMMARY METRICS STRIP (INDIAN NUMBERING SYSTEM)
        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            // Card 1: Total Batches
            Rectangle {
                Layout.fillWidth: true
                height: 52
                color: "#EFF6FF"
                border.color: "#BFDBFE"
                radius: 6
                ColumnLayout {
                    anchors.centerIn: parent
                    spacing: 2
                    Text { text: "TOTAL BATCHES"; color: "#1D4ED8"; font.pixelSize: 9; font.bold: true; font.letterSpacing: 0.5; Layout.alignment: Qt.AlignHCenter }
                    Text { text: root.totalBatchesCount + " Batches"; color: "#1E3A8A"; font.pixelSize: 14; font.bold: true; Layout.alignment: Qt.AlignHCenter }
                }
            }

            // Card 2: Total Paddy Input
            Rectangle {
                Layout.fillWidth: true
                height: 52
                color: "#FEF2F2"
                border.color: "#FECACA"
                radius: 6
                ColumnLayout {
                    anchors.centerIn: parent
                    spacing: 2
                    Text { text: "RAW PADDY INPUT"; color: "#DC2626"; font.pixelSize: 9; font.bold: true; font.letterSpacing: 0.5; Layout.alignment: Qt.AlignHCenter }
                    Text { text: root.totalPaddyMilledFmt + " Qtl"; color: "#991B1B"; font.pixelSize: 14; font.bold: true; Layout.alignment: Qt.AlignHCenter }
                }
            }

            // Card 3: Head Rice Output
            Rectangle {
                Layout.fillWidth: true
                height: 52
                color: "#ECFDF5"
                border.color: "#A7F3D0"
                radius: 6
                ColumnLayout {
                    anchors.centerIn: parent
                    spacing: 2
                    Text { text: "HEAD RICE RECOVERY"; color: "#059669"; font.pixelSize: 9; font.bold: true; font.letterSpacing: 0.5; Layout.alignment: Qt.AlignHCenter }
                    Text { text: root.totalHeadRiceProducedFmt + " Qtl (" + root.avgYieldPctFmt + ")"; color: "#065F46"; font.pixelSize: 14; font.bold: true; Layout.alignment: Qt.AlignHCenter }
                }
            }

            // Card 4: Rice Bran
            Rectangle {
                Layout.fillWidth: true
                height: 52
                color: "#FFFBEB"
                border.color: "#FDE68A"
                radius: 6
                ColumnLayout {
                    anchors.centerIn: parent
                    spacing: 2
                    Text { text: "RICE BRAN"; color: "#D97706"; font.pixelSize: 9; font.bold: true; font.letterSpacing: 0.5; Layout.alignment: Qt.AlignHCenter }
                    Text { text: root.totalBranProducedFmt + " Qtl"; color: "#92400E"; font.pixelSize: 14; font.bold: true; Layout.alignment: Qt.AlignHCenter }
                }
            }

            // Card 5: Rice Broken / Nakku
            Rectangle {
                Layout.fillWidth: true
                height: 52
                color: "#F5F3FF"
                border.color: "#DDD6FE"
                radius: 6
                ColumnLayout {
                    anchors.centerIn: parent
                    spacing: 2
                    Text { text: "BROKEN / NAKKU"; color: "#7C3AED"; font.pixelSize: 9; font.bold: true; font.letterSpacing: 0.5; Layout.alignment: Qt.AlignHCenter }
                    Text { text: root.totalBrokenProducedFmt + " Qtl"; color: "#5B21B6"; font.pixelSize: 14; font.bold: true; Layout.alignment: Qt.AlignHCenter }
                }
            }

            // Card 6: Wastage / Shortage
            Rectangle {
                Layout.fillWidth: true
                height: 52
                color: "#F8FAFC"
                border.color: "#E2E8F0"
                radius: 6
                ColumnLayout {
                    anchors.centerIn: parent
                    spacing: 2
                    Text { text: "MILLING LOSS / WASTAGE"; color: "#64748B"; font.pixelSize: 9; font.bold: true; font.letterSpacing: 0.5; Layout.alignment: Qt.AlignHCenter }
                    Text { text: root.totalWastageMilledFmt + " Qtl"; color: "#334155"; font.pixelSize: 14; font.bold: true; Layout.alignment: Qt.AlignHCenter }
                }
            }
        }

        // 3. EXCEL-LIKE SPLIT GRID UI TABLE VIEW (MASTER-DETAIL)
        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 10

            // ==================== LEFT MASTER TABLE: BATCHES REGISTER ====================
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredWidth: 6
                Layout.fillHeight: true
                color: "#FFFFFF"
                border.color: "#CBD5E1"
                radius: 8

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 8
                    spacing: 4

                    // Excel Grid Header
                    Rectangle {
                        Layout.fillWidth: true
                        height: 28
                        color: "#1E293B"
                        radius: 4

                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: 8; anchors.rightMargin: 8
                            spacing: 6

                            Text { Layout.preferredWidth: 70; text: "Batch No"; color: "#F8FAFC"; font.pixelSize: 11; font.bold: true }
                            Text { Layout.preferredWidth: 80; text: "Date"; color: "#F8FAFC"; font.pixelSize: 11; font.bold: true }
                            Text { Layout.fillWidth: true; text: "Paddy Variety"; color: "#F8FAFC"; font.pixelSize: 11; font.bold: true }
                            Text { Layout.preferredWidth: 75; text: "Input (Qtl)"; color: "#FCA5A5"; font.pixelSize: 11; font.bold: true; horizontalAlignment: Text.AlignRight }
                            Text { Layout.preferredWidth: 80; text: "Head Rice"; color: "#86EFAC"; font.pixelSize: 11; font.bold: true; horizontalAlignment: Text.AlignRight }
                            Text { Layout.preferredWidth: 60; text: "Yield %"; color: "#86EFAC"; font.pixelSize: 11; font.bold: true; horizontalAlignment: Text.AlignRight }
                            Text { Layout.preferredWidth: 60; text: "Bran"; color: "#FDE68A"; font.pixelSize: 11; font.bold: true; horizontalAlignment: Text.AlignRight }
                            Text { Layout.preferredWidth: 55; text: "Broken"; color: "#DDD6FE"; font.pixelSize: 11; font.bold: true; horizontalAlignment: Text.AlignRight }
                            Text { Layout.preferredWidth: 60; text: "Wastage"; color: "#CBD5E1"; font.pixelSize: 11; font.bold: true; horizontalAlignment: Text.AlignRight }
                        }
                    }

                    // Excel Grid Rows ListView
                    ListView {
                        id: batchListView
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        model: batchListModel
                        clip: true
                        spacing: 2

                        delegate: Rectangle {
                            id: batchRow
                            width: batchListView.width
                            height: 30
                            color: root.selectedBatchIndex === index ? "#EFF6FF" : (index % 2 === 0 ? "#FFFFFF" : "#F8FAFC")
                            border.color: root.selectedBatchIndex === index ? "#3B82F6" : "#E2E8F0"
                            border.width: root.selectedBatchIndex === index ? 1.5 : 1
                            radius: 4

                            MouseArea {
                                anchors.fill: parent
                                onClicked: root.selectBatch(index)
                            }

                            RowLayout {
                                anchors.fill: parent
                                anchors.leftMargin: 8; anchors.rightMargin: 8
                                spacing: 6

                                Text { Layout.preferredWidth: 70; text: model.batchNo; color: "#2563EB"; font.pixelSize: 11; font.bold: true }
                                Text { Layout.preferredWidth: 80; text: model.batchDate; color: "#334155"; font.pixelSize: 11 }
                                Text { Layout.fillWidth: true; text: model.paddyVariety; color: "#0F172A"; font.pixelSize: 11; font.bold: true; elide: Text.ElideRight }
                                Text { Layout.preferredWidth: 75; text: model.paddyInput; color: "#DC2626"; font.pixelSize: 11; font.bold: true; horizontalAlignment: Text.AlignRight }
                                Text { Layout.preferredWidth: 80; text: model.headRice; color: "#16A34A"; font.pixelSize: 11; font.bold: true; horizontalAlignment: Text.AlignRight }
                                Text { Layout.preferredWidth: 60; text: model.yieldPct; color: "#15803D"; font.pixelSize: 11; font.bold: true; horizontalAlignment: Text.AlignRight }
                                Text { Layout.preferredWidth: 60; text: model.bran; color: "#D97706"; font.pixelSize: 11; horizontalAlignment: Text.AlignRight }
                                Text { Layout.preferredWidth: 55; text: model.brokenRice; color: "#7C3AED"; font.pixelSize: 11; horizontalAlignment: Text.AlignRight }
                                Text { Layout.preferredWidth: 60; text: model.wastage; color: "#64748B"; font.pixelSize: 11; horizontalAlignment: Text.AlignRight }
                            }
                        }
                    }

                    // Status Bottom Info Bar
                    Rectangle {
                        Layout.fillWidth: true
                        height: 24
                        color: "#F1F5F9"
                        radius: 4
                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: 8; anchors.rightMargin: 8
                            Text { text: "Showing " + batchListModel.count + " batches (Use ↑ / ↓ arrow keys to navigate)"; color: "#64748B"; font.pixelSize: 10 }
                        }
                    }
                }
            }

            // ==================== RIGHT DETAIL TABLE: SELECTED BATCH LINE ITEMS ====================
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredWidth: 4
                Layout.fillHeight: true
                color: "#FFFFFF"
                border.color: "#CBD5E1"
                radius: 8

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 8
                    spacing: 4

                    // Detail Header Banner
                    Rectangle {
                        Layout.fillWidth: true
                        height: 28
                        color: "#047857"
                        radius: 4

                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: 8; anchors.rightMargin: 8
                            Text {
                                text: "📋 Batch Details: " + (root.activeBatchNo !== "" ? root.activeBatchNo : "Select a batch")
                                color: "#FFFFFF"
                                font.pixelSize: 12
                                font.bold: true
                            }
                            Item { Layout.fillWidth: true }
                            Text {
                                text: activeItemsModel.count + " Items"
                                color: "#A7F3D0"
                                font.pixelSize: 11
                                font.bold: true
                            }
                        }
                    }

                    // Detail Table Columns Header
                    Rectangle {
                        Layout.fillWidth: true
                        height: 24
                        color: "#F1F5F9"
                        radius: 4

                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: 6; anchors.rightMargin: 6
                            spacing: 6

                            Text { Layout.preferredWidth: 40; text: "Type"; color: "#475569"; font.pixelSize: 10; font.bold: true }
                            Text { Layout.fillWidth: true; text: "Item Name"; color: "#475569"; font.pixelSize: 10; font.bold: true }
                            Text { Layout.preferredWidth: 50; text: "Yield %"; color: "#475569"; font.pixelSize: 10; font.bold: true; horizontalAlignment: Text.AlignRight }
                            Text { Layout.preferredWidth: 40; text: "Bags"; color: "#475569"; font.pixelSize: 10; font.bold: true; horizontalAlignment: Text.AlignRight }
                            Text { Layout.preferredWidth: 65; text: "Weight (Qtl)"; color: "#475569"; font.pixelSize: 10; font.bold: true; horizontalAlignment: Text.AlignRight }
                            Text { Layout.preferredWidth: 65; text: "Amount"; color: "#475569"; font.pixelSize: 10; font.bold: true; horizontalAlignment: Text.AlignRight }
                        }
                    }

                    // Detail Items ListView
                    ListView {
                        id: activeItemsListView
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        model: activeItemsModel
                        clip: true
                        spacing: 2

                        delegate: Rectangle {
                            width: activeItemsListView.width
                            height: 28
                            color: model.isInput ? "#FEF2F2" : (index % 2 === 0 ? "#FFFFFF" : "#F0FDF4")
                            border.color: model.isInput ? "#FECACA" : "#BBF7D0"
                            radius: 4

                            RowLayout {
                                anchors.fill: parent
                                anchors.leftMargin: 6; anchors.rightMargin: 6
                                spacing: 6

                                Rectangle {
                                    Layout.preferredWidth: 40
                                    height: 18
                                    color: model.isInput ? "#FEE2E2" : "#DCFCE7"
                                    radius: 3
                                    border.color: model.isInput ? "#FCA5A5" : "#86EFAC"
                                    Text {
                                        anchors.centerIn: parent
                                        text: model.drcr === "Cr" ? "IN (Cr)" : "OUT (Dr)"
                                        color: model.isInput ? "#991B1B" : "#166534"
                                        font.pixelSize: 9
                                        font.bold: true
                                    }
                                }

                                Text { Layout.fillWidth: true; text: model.itemName; color: "#0F172A"; font.pixelSize: 11; font.bold: true; elide: Text.ElideRight }
                                Text { Layout.preferredWidth: 50; text: model.yieldPct; color: "#166534"; font.pixelSize: 11; font.bold: true; horizontalAlignment: Text.AlignRight }
                                Text { Layout.preferredWidth: 40; text: model.bags; color: "#334155"; font.pixelSize: 11; horizontalAlignment: Text.AlignRight }
                                Text { Layout.preferredWidth: 65; text: model.weight; color: model.isInput ? "#DC2626" : "#15803D"; font.pixelSize: 11; font.bold: true; horizontalAlignment: Text.AlignRight }
                                Text { Layout.preferredWidth: 65; text: model.amount; color: "#0F172A"; font.pixelSize: 11; font.bold: true; horizontalAlignment: Text.AlignRight }
                            }
                        }
                    }
                }
            }
        }
    }
}
