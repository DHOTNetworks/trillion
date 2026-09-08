import QtQuick
import QtQuick.Templates as T
import QtQuick.Layouts
import MahadevERP

FocusScope {
    id: root
    focus: true

    signal cancelRequested()
    signal openNewMillingRequested()
    signal openPeriodModal()

    function handleEscape() {
        root.cancelRequested()
    }

    property string activePeriodText: ""
    property string fromDateText: ""
    property string toDateText: ""

    function toIsoDate(dStr) {
        if (!dStr) return ""
        var str = dStr.trim()
        if (str === "ALL" || str === "All") return "ALL"
        if (str.indexOf("-") !== -1) {
            var parts = str.split("-")
            if (parts.length === 3) {
                if (parts[0].length === 4) return str
                if (parts[2].length === 4) return parts[2] + "-" + parts[1] + "-" + parts[0]
            }
        }
        return str
    }

    function toDisplayDate(dStr) {
        if (!dStr) return ""
        var str = dStr.trim()
        if (str === "ALL" || str === "All") return "All Financial Years"
        if (str.indexOf("-") !== -1) {
            var parts = str.split("-")
            if (parts.length === 3) {
                if (parts[0].length === 4) return parts[2] + "-" + parts[1] + "-" + parts[0]
                if (parts[2].length === 4) return str
            }
        }
        return str
    }

    function syncWithActivePeriod() {
        if (typeof stockItemsModel !== "undefined" && stockItemsModel) {
            var fIso = stockItemsModel.get_from_date()
            var tIso = stockItemsModel.get_to_date()
            var fy = stockItemsModel.get_financial_year()
            if (fIso && tIso) {
                fromDateText = toDisplayDate(fIso)
                toDateText = toDisplayDate(tIso)
                activePeriodText = fromDateText + " To " + toDateText + " (" + fy + ")"
            } else {
                activePeriodText = fy || "Active Period"
            }
        }
    }

    Component.onCompleted: {
        syncWithActivePeriod()
        reloadStatementData()
        Qt.callLater(function() { root.forceActiveFocus() })
    }

    onVisibleChanged: {
        if (visible) {
            syncWithActivePeriod()
            reloadStatementData()
            Qt.callLater(function() { root.forceActiveFocus() })
        }
    }

    function reloadStatementData() {
        var fIso = toIsoDate(fromDateText)
        var tIso = toIsoDate(toDateText)
        if (typeof millingStatementCtrl !== "undefined" && millingStatementCtrl) {
            millingStatementCtrl.reload(fIso, tIso)
        }
    }

    // Keyboard Shortcuts
    Shortcut { sequence: "F2"; onActivated: root.openNewMillingRequested() }
    Shortcut { sequence: "Ctrl+F"; onActivated: searchInput.focusInput = true }
    Shortcut {
        sequence: "Alt+F"
        context: Qt.WindowShortcut
        onActivated: filterPopup.open()
    }

    Keys.onEscapePressed: function(event) {
        event.accepted = true
        root.cancelRequested()
    }
    Keys.onUpPressed: function(event) {
        event.accepted = true
        if (typeof millingStatementCtrl !== "undefined" && millingStatementCtrl) {
            var idx = millingStatementCtrl.selectedBatchIndex
            if (idx > 0) {
                millingStatementCtrl.selectBatch(idx - 1)
                batchListView.positionViewAtIndex(idx - 1, ListView.Contain)
            }
        }
    }
    Keys.onDownPressed: function(event) {
        event.accepted = true
        if (typeof millingStatementCtrl !== "undefined" && millingStatementCtrl) {
            var idx = millingStatementCtrl.selectedBatchIndex
            if (idx < millingStatementCtrl.batchModel.count - 1) {
                millingStatementCtrl.selectBatch(idx + 1)
                batchListView.positionViewAtIndex(idx + 1, ListView.Contain)
            }
        }
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
                    text: " Milling Statement & Production Register"
                    color: "#FFFFFF"
                    font.pixelSize: 16
                    font.bold: true
                }

                Item { Layout.fillWidth: true }

                // Search Box
                CustomInput {
                    id: searchInput
                    Layout.preferredWidth: 240
                    placeholderText: "Search Batch No, Date..."
                    text: (typeof millingStatementCtrl !== "undefined" && millingStatementCtrl) ? millingStatementCtrl.searchQuery : ""
                    onTextChanged: {
                        if (typeof millingStatementCtrl !== "undefined" && millingStatementCtrl) {
                            millingStatementCtrl.searchQuery = text
                        }
                    }
                }

                // Active Financial Year Badge
                Rectangle {
                    implicitWidth: periodRow.implicitWidth + 20
                    implicitHeight: 30
                    color: "#1E293B"
                    radius: 6
                    border.color: "#38BDF8"
                    border.width: 1
                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        onClicked: {
                            root.openPeriodModal()
                        }
                    }
                    RowLayout {
                        id: periodRow
                        anchors.centerIn: parent
                        spacing: 6
                        Text {
                            text: " " + (root.activePeriodText !== "" ? root.activePeriodText : ((typeof stockItemsModel !== "undefined" && stockItemsModel) ? stockItemsModel.get_financial_year() : "Active Period"))
                            color: "#38BDF8"
                            font.pixelSize: 11
                            font.bold: true
                        }
                    }
                }

                // Filter Dates Button
                T.Button {
                    id: filterBtn
                    implicitWidth: contentItem.implicitWidth + 20
                    implicitHeight: 30
                    background: Rectangle { color: filterPopup.visible ? "#1D4ED8" : "#2563EB"; radius: 6 }
                    contentItem: RowLayout {
                        spacing: 6
                        Text { text: "Filter Dates"; color: "#FFF"; font.bold: true; font.pixelSize: 11 }
                        KbdBadge { text: "Alt+F"; badgeColor: "#1E3A8A"; textColor: "#93C5FD"; borderColor: "#2563EB" }
                    }
                    onClicked: filterPopup.open()
                }

                T.Button {
                    implicitWidth: contentItem.implicitWidth + 24
                    implicitHeight: 30
                    background: Rectangle { color: "#16A34A"; radius: 6 }
                    contentItem: RowLayout {
                        spacing: 6
                        Text { text: "+ Log Milling Batch"; color: "#FFFFFF"; font.pixelSize: 11; font.bold: true }
                        KbdBadge { text: "F2"; badgeColor: "#14532D"; textColor: "#86EFAC"; borderColor: "#16A34A" }
                    }
                    onClicked: root.openNewMillingRequested()
                }

                T.Button {
                    implicitWidth: contentItem.implicitWidth + 24
                    implicitHeight: 30
                    background: Rectangle { color: "#334155"; radius: 6 }
                    contentItem: Text {
                        text: "← Back (Esc)"
                        color: "#F8FAFC"
                        font.pixelSize: 11
                        font.bold: true
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                    onClicked: root.cancelRequested()
                }
            }
        }

        // 2. SUMMARY METRICS STRIP
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
                    Text { text: ((typeof millingStatementCtrl !== "undefined" && millingStatementCtrl) ? millingStatementCtrl.totalBatchesCount.toString() : "0") + " Batches"; color: "#1E3A8A"; font.pixelSize: 14; font.bold: true; Layout.alignment: Qt.AlignHCenter }
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
                    Text { text: ((typeof millingStatementCtrl !== "undefined" && millingStatementCtrl) ? millingStatementCtrl.totalPaddyMilledFmt : "0.00") + " Qtl"; color: "#991B1B"; font.pixelSize: 14; font.bold: true; Layout.alignment: Qt.AlignHCenter }
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
                    Text { text: ((typeof millingStatementCtrl !== "undefined" && millingStatementCtrl) ? (millingStatementCtrl.totalHeadRiceProducedFmt + " Qtl (" + millingStatementCtrl.avgYieldPctFmt + ")") : "0.00 Qtl (0.00%)"); color: "#065F46"; font.pixelSize: 14; font.bold: true; Layout.alignment: Qt.AlignHCenter }
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
                    Text { text: ((typeof millingStatementCtrl !== "undefined" && millingStatementCtrl) ? millingStatementCtrl.totalBranProducedFmt : "0.00") + " Qtl"; color: "#92400E"; font.pixelSize: 14; font.bold: true; Layout.alignment: Qt.AlignHCenter }
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
                    Text { text: ((typeof millingStatementCtrl !== "undefined" && millingStatementCtrl) ? millingStatementCtrl.totalBrokenProducedFmt : "0.00") + " Qtl"; color: "#5B21B6"; font.pixelSize: 14; font.bold: true; Layout.alignment: Qt.AlignHCenter }
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
                    Text { text: ((typeof millingStatementCtrl !== "undefined" && millingStatementCtrl) ? millingStatementCtrl.totalWastageMilledFmt : "0.00") + " Qtl"; color: "#334155"; font.pixelSize: 14; font.bold: true; Layout.alignment: Qt.AlignHCenter }
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
                        model: (typeof millingStatementCtrl !== "undefined" && millingStatementCtrl) ? millingStatementCtrl.batchModel : null
                        clip: true
                        spacing: 2

                        delegate: Rectangle {
                            id: batchRow
                            width: batchListView.width
                            height: 30
                            color: ((typeof millingStatementCtrl !== "undefined" && millingStatementCtrl) && millingStatementCtrl.selectedBatchIndex === index) ? "#EFF6FF" : (index % 2 === 0 ? "#FFFFFF" : "#F8FAFC")
                            border.color: ((typeof millingStatementCtrl !== "undefined" && millingStatementCtrl) && millingStatementCtrl.selectedBatchIndex === index) ? "#3B82F6" : "#E2E8F0"
                            border.width: ((typeof millingStatementCtrl !== "undefined" && millingStatementCtrl) && millingStatementCtrl.selectedBatchIndex === index) ? 1.5 : 1
                            radius: 4

                            MouseArea {
                                anchors.fill: parent
                                onClicked: {
                                    if (typeof millingStatementCtrl !== "undefined" && millingStatementCtrl) {
                                        millingStatementCtrl.selectBatch(index)
                                    }
                                }
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
                            Text { text: "Showing " + ((typeof millingStatementCtrl !== "undefined" && millingStatementCtrl) ? millingStatementCtrl.batchModel.count : 0) + " batches (Use ↑ / ↓ arrow keys to navigate)"; color: "#64748B"; font.pixelSize: 10 }
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
                                text: "Batch Details: " + ((typeof millingStatementCtrl !== "undefined" && millingStatementCtrl && millingStatementCtrl.activeBatchNo !== "") ? millingStatementCtrl.activeBatchNo : "Select a batch")
                                color: "#FFFFFF"
                                font.pixelSize: 12
                                font.bold: true
                            }
                            Item { Layout.fillWidth: true }
                            Text {
                                text: ((typeof millingStatementCtrl !== "undefined" && millingStatementCtrl) ? millingStatementCtrl.itemModel.count : 0) + " Items"
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
                        model: (typeof millingStatementCtrl !== "undefined" && millingStatementCtrl) ? millingStatementCtrl.itemModel : null
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

    T.Popup {
        id: filterPopup
        width: 420
        height: 320
        modal: true
        dim: true
        x: Math.round((parent.width - width) / 2)
        y: Math.round((parent.height - height) / 2)
        focus: true
        closePolicy: T.Popup.CloseOnPressOutside | T.Popup.CloseOnEscape

        background: Rectangle {
            color: "#FFFFFF"
            border.color: "#2563EB"
            border.width: 2
            radius: 12
        }

        onOpened: {
            filterFromDateInput.text = root.fromDateText !== "" ? root.fromDateText : (typeof stockItemsModel !== "undefined" && stockItemsModel ? root.toDisplayDate(stockItemsModel.get_from_date()) : "01-04-2026")
            filterToDateInput.text = root.toDateText !== "" ? root.toDateText : (typeof stockItemsModel !== "undefined" && stockItemsModel ? root.toDisplayDate(stockItemsModel.get_to_date()) : "31-03-2027")
            Qt.callLater(function() { filterFromDateInput.focusInput = true })
        }

        FocusScope {
            id: filterScope
            anchors.fill: parent
            focus: true

            Keys.onReturnPressed: function(event) { event.accepted = true; filterScope.applyFilter() }
            Keys.onEnterPressed: function(event) { event.accepted = true; filterScope.applyFilter() }
            Keys.onEscapePressed: function(event) { event.accepted = true; filterPopup.close() }

            function applyFilter() {
                root.fromDateText = filterFromDateInput.text.trim()
                root.toDateText = filterToDateInput.text.trim()
                root.activePeriodText = root.fromDateText + " To " + root.toDateText
                filterPopup.close()
                root.reloadStatementData()
                root.forceActiveFocus()
            }

            function resetToActiveFY() {
                root.syncWithActivePeriod()
                filterFromDateInput.text = root.fromDateText
                filterToDateInput.text = root.toDateText
                filterPopup.close()
                root.reloadStatementData()
                root.forceActiveFocus()
            }

            function showAllBatches() {
                root.fromDateText = "ALL"
                root.toDateText = "ALL"
                root.activePeriodText = "All Financial Years (All Batches)"
                filterPopup.close()
                root.reloadStatementData()
                root.forceActiveFocus()
            }

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 18
                spacing: 12

                RowLayout {
                    Layout.fillWidth: true
                    Text { text: " Filter Milling Statement Dates"; color: "#0F172A"; font.pixelSize: 15; font.bold: true }
                    Item { Layout.fillWidth: true }
                    T.Button {
                        implicitWidth: 28
                        implicitHeight: 28
                        flat: true
                        contentItem: Text { text: "X"; color: "#64748B"; font.bold: true; font.pixelSize: 12; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                        onClicked: filterPopup.close()
                    }
                }

                Rectangle { Layout.fillWidth: true; height: 1; color: "#E2E8F0" }

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 10

                    CustomInput {
                        id: filterFromDateInput
                        label: "From Date (DD-MM-YYYY)"
                        text: "01-04-2026"
                        Layout.fillWidth: true
                        onReturnPressed: filterToDateInput.focusInput = true
                    }

                    CustomInput {
                        id: filterToDateInput
                        label: "To Date (DD-MM-YYYY)"
                        text: "31-03-2027"
                        Layout.fillWidth: true
                        onReturnPressed: filterScope.applyFilter()
                    }
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 8

                    T.Button {
                        implicitWidth: contentItem.implicitWidth + 16
                        implicitHeight: 30
                        background: Rectangle { color: "#F1F5F9"; radius: 6; border.color: "#CBD5E1" }
                        contentItem: Text { text: "Active FY"; color: "#1E293B"; font.bold: true; font.pixelSize: 11; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                        onClicked: filterScope.resetToActiveFY()
                    }

                    T.Button {
                        implicitWidth: contentItem.implicitWidth + 16
                        implicitHeight: 30
                        background: Rectangle { color: "#F1F5F9"; radius: 6; border.color: "#CBD5E1" }
                        contentItem: Text { text: "All Batches (All FYs)"; color: "#6366F1"; font.bold: true; font.pixelSize: 11; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                        onClicked: filterScope.showAllBatches()
                    }

                    Item { Layout.fillWidth: true }

                    T.Button {
                        implicitWidth: contentItem.implicitWidth + 20
                        implicitHeight: 30
                        background: Rectangle { color: "#2563EB"; radius: 6 }
                        contentItem: Text { text: "Apply (Enter)"; color: "#FFFFFF"; font.bold: true; font.pixelSize: 11; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                        onClicked: filterScope.applyFilter()
                    }
                }
            }
        }
    }
}
