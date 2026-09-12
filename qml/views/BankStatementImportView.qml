import QtQuick
import QtQuick.Templates as T
import QtQuick.Layouts
import MahadevERP
import "../components"

Rectangle {
    id: root
    color: "#F8FAFC"

    signal cancelRequested()
    signal openLedgerRequested(string partyName)

    property string currentFilter: "ALL"
    property string searchQuery: ""
    property int selectedRowIdx: -1
    property bool isPartySelectorOpen: false
    property int editingRowIdx: -1
    property string editingField: "" // "Dr" or "Cr"

    Shortcut {
        sequence: "Ctrl+O"
        context: Qt.WindowShortcut
        onActivated: {
            if (typeof bankStatementCtrl !== "undefined" && bankStatementCtrl) {
                bankStatementCtrl.browseStatementFile()
            }
        }
    }

    Shortcut {
        sequence: "Ctrl+S"
        context: Qt.WindowShortcut
        onActivated: {
            postVouchersAction()
        }
    }

    Shortcut {
        sequence: "Escape"
        context: Qt.WindowShortcut
        onActivated: {
            if (root.isPartySelectorOpen) {
                root.isPartySelectorOpen = false
            } else {
                root.cancelRequested()
            }
        }
    }

    function postVouchersAction() {
        if (typeof bankStatementCtrl !== "undefined" && bankStatementCtrl) {
            if (bankStatementCtrl.selectedCount === 0) {
                postResultModal.title = "No Entries Selected"
                postResultModal.message = "Please select at least one transaction row to post into the database."
                postResultModal.isSuccess = false
                postResultModal.visible = true
                return
            }

            var res = bankStatementCtrl.postSelectedVouchers()
            if (res && res.success) {
                postResultModal.title = "Bank Vouchers Posted Successfully!"
                postResultModal.message = res.message || "All selected bank entries have been written to the accounting ledger."
                postResultModal.isSuccess = true
                postResultModal.visible = true
            } else {
                postResultModal.title = "Posting Failed"
                postResultModal.message = (res && res.message) ? res.message : "An error occurred while saving vouchers to database."
                postResultModal.isSuccess = false
                postResultModal.visible = true
            }
        }
    }

    function applyPartySelection(selectedPartyName) {
        if (root.editingRowIdx >= 0 && typeof bankStatementCtrl !== "undefined" && bankStatementCtrl) {
            if (root.editingField === "Dr") {
                bankStatementCtrl.rowsModel.setRowDrAccount(root.editingRowIdx, selectedPartyName)
            } else {
                bankStatementCtrl.rowsModel.setRowCrAccount(root.editingRowIdx, selectedPartyName)
            }

            // Save alias memory
            var row = bankStatementCtrl.rowsModel.getRow(root.editingRowIdx)
            if (row && row.extractedParty && row.extractedParty.length > 0) {
                bankStatementCtrl.saveAlias(row.extractedParty, selectedPartyName)
            }
        }
        root.isPartySelectorOpen = false
        statementListView.forceActiveFocus()
    }

    Component.onCompleted: {
        if (typeof bankStatementCtrl !== "undefined" && bankStatementCtrl) {
            if (bankStatementCtrl.totalCount === 0) {
                bankStatementCtrl.loadStatement("322157558_unlocked.pdf")
            }
        }
    }

    // Drag and Drop PDF support
    DropArea {
        anchors.fill: parent
        onEntered: function(drag) {
            drag.acceptProposedAction()
        }
        onDropped: function(drop) {
            if (drop.hasUrls && drop.urls.length > 0) {
                var url = drop.urls[0].toString()
                if (url.toLowerCase().endsWith(".pdf")) {
                    if (typeof bankStatementCtrl !== "undefined" && bankStatementCtrl) {
                        bankStatementCtrl.loadStatement(url)
                    }
                }
            }
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 12
        spacing: 8

        // 1. TOP HEADER & CONTROLS (2-ROW RESPONSIVE HEADER)
        ColumnLayout {
            Layout.fillWidth: true
            spacing: 6

            // Row 1: Title, Badges, and Action Buttons
            RowLayout {
                Layout.fillWidth: true
                spacing: 8

                Rectangle {
                    width: 32; height: 32; radius: 6
                    color: "#1E3A8A"
                    Text {
                        anchors.centerIn: parent
                        text: "BANK"
                        color: "#FFFFFF"
                        font.pixelSize: 9
                        font.bold: true
                    }
                }

                Text {
                    text: "CANARA BANK STATEMENT AUTO-ENTRY & RECONCILIATION"
                    color: "#1D4ED8"
                    font.pixelSize: 12
                    font.bold: true
                    font.letterSpacing: 0.5
                }

                Rectangle {
                    visible: typeof bankStatementCtrl !== "undefined" && bankStatementCtrl && bankStatementCtrl.fileName !== ""
                    height: 22; radius: 4; color: "#F1F5F9"; border.color: "#CBD5E1"
                    implicitWidth: fileTxt.implicitWidth + 10
                    Text {
                        id: fileTxt
                        anchors.centerIn: parent
                        text: "PDF: " + (bankStatementCtrl ? bankStatementCtrl.fileName : "")
                        color: "#334155"
                        font.pixelSize: 10
                        font.bold: true
                    }
                }

                Rectangle {
                    visible: typeof bankStatementCtrl !== "undefined" && bankStatementCtrl && bankStatementCtrl.accountNo !== ""
                    height: 22; radius: 4; color: "#DBEAFE"; border.color: "#93C5FD"
                    implicitWidth: acctTxt.implicitWidth + 10
                    Text {
                        id: acctTxt
                        anchors.centerIn: parent
                        text: "A/c: " + (bankStatementCtrl ? bankStatementCtrl.accountNo : "")
                        color: "#1E40AF"
                        font.pixelSize: 10
                        font.bold: true
                    }
                }

                Rectangle {
                    visible: typeof bankStatementCtrl !== "undefined" && bankStatementCtrl && bankStatementCtrl.dateRange !== ""
                    height: 22; radius: 4; color: "#FEF3C7"; border.color: "#FDE68A"
                    implicitWidth: dateRangeTxt.implicitWidth + 10
                    Text {
                        id: dateRangeTxt
                        anchors.centerIn: parent
                        text: "Period: " + (bankStatementCtrl ? bankStatementCtrl.dateRange : "")
                        color: "#92400E"
                        font.pixelSize: 10
                        font.bold: true
                    }
                }

                Item { Layout.fillWidth: true }

                // Select PDF Statement Button
                T.Button {
                    id: browseBtn
                    implicitHeight: 28
                    implicitWidth: contentItem.implicitWidth + 16
                    background: Rectangle { color: browseBtn.hovered ? "#1D4ED8" : "#2563EB"; radius: 4 }
                    contentItem: RowLayout {
                        spacing: 4
                        Text { text: "Select PDF Statement"; color: "#FFFFFF"; font.pixelSize: 11; font.bold: true }
                        KbdBadge { text: "Ctrl+O"; badgeColor: "#1E40AF"; textColor: "#DBEAFE"; borderColor: "#2563EB" }
                    }
                    onClicked: {
                        if (typeof bankStatementCtrl !== "undefined" && bankStatementCtrl) {
                            bankStatementCtrl.browseStatementFile()
                        }
                    }
                }

                // Back Button
                T.Button {
                    id: backBtn
                    implicitHeight: 28
                    implicitWidth: contentItem.implicitWidth + 14
                    background: Rectangle { color: backBtn.hovered ? "#475569" : "#334155"; radius: 4 }
                    contentItem: Text { text: "Back (Esc)"; color: "#F8FAFC"; font.pixelSize: 11; font.bold: true }
                    onClicked: root.cancelRequested()
                }
            }

            // Row 2: Subtitle and Bank Ledger Dropdown / Box
            RowLayout {
                Layout.fillWidth: true
                spacing: 8

                Text {
                    text: "Automated Double-Entry Voucher Importer & Party Matcher"
                    color: "#64748B"
                    font.pixelSize: 11
                }

                Item { Layout.fillWidth: true }

                Text { text: "Post To Bank Ledger:"; color: "#475569"; font.pixelSize: 11; font.bold: true }
                Rectangle {
                    width: 200; height: 26; radius: 4; color: "#FFFFFF"; border.color: "#CBD5E1"; border.width: 1
                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: 6; anchors.rightMargin: 6
                        Text {
                            text: (typeof bankStatementCtrl !== "undefined" && bankStatementCtrl && bankStatementCtrl.bankLedgerName) ? bankStatementCtrl.bankLedgerName : "Canara Bank Cc"
                            color: "#1E293B"; font.pixelSize: 11; font.bold: true; Layout.fillWidth: true; elide: Text.ElideRight
                        }
                    }
                }
            }
        }

        Rectangle { Layout.fillWidth: true; height: 1; color: "#E2E8F0" }

        // 2. STATS & SUMMARY KPI CARDS (RESPONSIVE VALUE + SUBTITLE)
        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            StatCard {
                title: "TOTAL TRANSACTIONS"
                value: (typeof bankStatementCtrl !== "undefined" && bankStatementCtrl) ? (bankStatementCtrl.totalCount.toString() + " Entries") : "0 Entries"
                subtext: "Extracted from PDF"
                accentColor: "#2563EB"
                implicitHeight: 82
                Layout.fillWidth: true
            }

            StatCard {
                title: "TOTAL STATEMENT VALUE"
                value: (typeof bankStatementCtrl !== "undefined" && bankStatementCtrl) ? ("W: " + bankStatementCtrl.totalWithdrawalsFmt) : "0.00"
                subtext: (typeof bankStatementCtrl !== "undefined" && bankStatementCtrl) ? ("D: " + bankStatementCtrl.totalDepositsFmt) : ""
                accentColor: "#0284C7"
                implicitHeight: 82
                Layout.fillWidth: true
            }

            StatCard {
                title: "SELECTED TO POST"
                value: (typeof bankStatementCtrl !== "undefined" && bankStatementCtrl) ? (bankStatementCtrl.selectedCount.toString() + " Entries") : "0 Entries"
                subtext: (typeof bankStatementCtrl !== "undefined" && bankStatementCtrl) ? ("W: " + bankStatementCtrl.selectedWithdrawalsFmt + " | D: " + bankStatementCtrl.selectedDepositsFmt) : ""
                accentColor: "#16A34A"
                implicitHeight: 82
                Layout.fillWidth: true
            }

            StatCard {
                title: "REVIEW / UNMATCHED"
                value: (typeof bankStatementCtrl !== "undefined" && bankStatementCtrl) ? (bankStatementCtrl.unmatchedCount.toString() + " Unmatched") : "0"
                subtext: (typeof bankStatementCtrl !== "undefined" && bankStatementCtrl) ? (bankStatementCtrl.duplicateCount.toString() + " Duplicates") : "0 Duplicates"
                accentColor: (typeof bankStatementCtrl !== "undefined" && bankStatementCtrl && bankStatementCtrl.unmatchedCount > 0) ? "#DC2626" : "#059669"
                implicitHeight: 82
                Layout.fillWidth: true
            }
        }

        // 3. FILTER TABS & SEARCH BAR
        Rectangle {
            Layout.fillWidth: true
            height: 36
            color: "#FFFFFF"
            radius: 6
            border.color: "#E2E8F0"
            border.width: 1

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 8; anchors.rightMargin: 8
                spacing: 6

                // Filter Buttons
                RowLayout {
                    spacing: 3
                    Repeater {
                        model: [
                            { id: "ALL", label: "All Entries" },
                            { id: "RECEIPTS", label: "Deposits / Receipts" },
                            { id: "PAYMENTS", label: "Withdrawals / Payments" },
                            { id: "CHARGES", label: "Bank Charges" },
                            { id: "INTEREST", label: "Interest Debits" },
                            { id: "UNMATCHED", label: "Unmatched" },
                            { id: "DUPLICATES", label: "Duplicates" }
                        ]

                        Rectangle {
                            height: 24
                            implicitWidth: tabTxt.implicitWidth + 12
                            radius: 3
                            color: root.currentFilter === modelData.id ? "#1E3A8A" : (tabMouse.containsMouse ? "#F1F5F9" : "transparent")
                            border.color: root.currentFilter === modelData.id ? "#1E3A8A" : "#CBD5E1"
                            border.width: 1

                            Text {
                                id: tabTxt
                                anchors.centerIn: parent
                                text: modelData.label
                                color: root.currentFilter === modelData.id ? "#FFFFFF" : "#475569"
                                font.pixelSize: 10
                                font.bold: true
                            }

                            MouseArea {
                                id: tabMouse
                                anchors.fill: parent
                                hoverEnabled: true
                                cursorShape: Qt.PointingHandCursor
                                onClicked: root.currentFilter = modelData.id
                            }
                        }
                    }
                }

                Item { Layout.fillWidth: true }

                // Search Input
                Rectangle {
                    width: 160; height: 24; radius: 3; color: "#F8FAFC"; border.color: searchInput.activeFocus ? "#2563EB" : "#CBD5E1"; border.width: 1
                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: 5; anchors.rightMargin: 5
                        spacing: 3
                        Text { text: "Search:"; color: "#64748B"; font.pixelSize: 10; font.bold: true }
                        TextInput {
                            id: searchInput
                            Layout.fillWidth: true
                            font.pixelSize: 10
                            color: "#0F172A"
                            selectByMouse: true
                            onTextChanged: root.searchQuery = text.toLowerCase()
                        }
                        Text {
                            text: "x"; color: "#94A3B8"; font.bold: true; visible: searchInput.text.length > 0
                            MouseArea { anchors.fill: parent; onClicked: searchInput.text = "" }
                        }
                    }
                }

                // Batch Selection Helpers
                T.Button {
                    implicitHeight: 24
                    implicitWidth: contentItem.implicitWidth + 8
                    background: Rectangle { color: "#F1F5F9"; radius: 3; border.color: "#CBD5E1" }
                    contentItem: Text { text: "Select All"; color: "#334155"; font.pixelSize: 10; font.bold: true }
                    onClicked: {
                        if (typeof bankStatementCtrl !== "undefined" && bankStatementCtrl) {
                            bankStatementCtrl.rowsModel.selectAll(true)
                        }
                    }
                }

                T.Button {
                    implicitHeight: 24
                    implicitWidth: contentItem.implicitWidth + 8
                    background: Rectangle { color: "#F1F5F9"; radius: 3; border.color: "#CBD5E1" }
                    contentItem: Text { text: "Deselect Duplicates"; color: "#B45309"; font.pixelSize: 10; font.bold: true }
                    onClicked: {
                        if (typeof bankStatementCtrl !== "undefined" && bankStatementCtrl) {
                            bankStatementCtrl.rowsModel.deselectDuplicates()
                        }
                    }
                }
            }
        }

        // 4. 100% TRANSPARENT INTERACTIVE DOUBLE-ENTRY PREVIEW TABLE
        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            color: "#FFFFFF"
            radius: 6
            border.color: "#CBD5E1"
            border.width: 1
            clip: true

            ColumnLayout {
                anchors.fill: parent
                spacing: 0

                // Table Header Grid
                Rectangle {
                    Layout.fillWidth: true
                    height: 30
                    color: "#F1F5F9"
                    border.color: "#CBD5E1"
                    border.width: 1

                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: 8; anchors.rightMargin: 8
                        spacing: 6

                        // Master Checkbox
                        CustomCheckBox {
                            Layout.preferredWidth: 24
                            Layout.minimumWidth: 24
                            boxSize: 15
                            boxRadius: 3
                            checkedColor: "#2563EB"
                            checked: (typeof bankStatementCtrl !== "undefined" && bankStatementCtrl && bankStatementCtrl.selectedCount === bankStatementCtrl.totalCount && bankStatementCtrl.totalCount > 0)
                            onToggled: {
                                if (typeof bankStatementCtrl !== "undefined" && bankStatementCtrl) {
                                    var allSel = (bankStatementCtrl.selectedCount === bankStatementCtrl.totalCount)
                                    bankStatementCtrl.rowsModel.selectAll(!allSel)
                                }
                            }
                        }

                        Text { text: "#"; color: "#475569"; font.pixelSize: 10; font.bold: true; Layout.preferredWidth: 28; Layout.minimumWidth: 28 }
                        Text { text: "Date"; color: "#475569"; font.pixelSize: 10; font.bold: true; Layout.preferredWidth: 72; Layout.minimumWidth: 72 }
                        Text { text: "Type"; color: "#475569"; font.pixelSize: 10; font.bold: true; Layout.preferredWidth: 75; Layout.minimumWidth: 75 }
                        Text {
                            text: "Bank Narration & UTR Ref"
                            color: "#475569"
                            font.pixelSize: 10
                            font.bold: true
                            Layout.fillWidth: true
                            Layout.minimumWidth: 100
                            Layout.preferredWidth: 0
                            elide: Text.ElideRight
                        }
                        Text { text: "DEBIT LEG (Dr Account)"; color: "#1D4ED8"; font.pixelSize: 10; font.bold: true; Layout.preferredWidth: 145; Layout.minimumWidth: 145 }
                        Text { text: "CREDIT LEG (Cr Account)"; color: "#C2410C"; font.pixelSize: 10; font.bold: true; Layout.preferredWidth: 145; Layout.minimumWidth: 145 }
                        Text { text: "Amount"; color: "#475569"; font.pixelSize: 10; font.bold: true; Layout.preferredWidth: 90; Layout.minimumWidth: 90; horizontalAlignment: Text.AlignRight }
                        Text { text: "Status"; color: "#475569"; font.pixelSize: 10; font.bold: true; Layout.preferredWidth: 85; Layout.minimumWidth: 85; horizontalAlignment: Text.AlignHCenter }
                    }
                }

                // Table Rows ListView
                ListView {
                    id: statementListView
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true
                    model: (typeof bankStatementCtrl !== "undefined" && bankStatementCtrl) ? bankStatementCtrl.rowsModel : null
                    spacing: 1
                    boundsBehavior: Flickable.StopAtBounds
                    focus: true
                    activeFocusOnTab: true
                    currentIndex: 0
                    highlightFollowsCurrentItem: true

                    Keys.onUpPressed: function(event) {
                        event.accepted = true
                        if (currentIndex > 0) currentIndex--
                        positionViewAtIndex(currentIndex, ListView.Contain)
                    }

                    Keys.onDownPressed: function(event) {
                        event.accepted = true
                        if (currentIndex < count - 1) currentIndex++
                        positionViewAtIndex(currentIndex, ListView.Contain)
                    }

                    Keys.onSpacePressed: function(event) {
                        event.accepted = true
                        if (currentIndex >= 0 && currentIndex < count) {
                            var r = bankStatementCtrl.rowsModel.getRow(currentIndex)
                            bankStatementCtrl.rowsModel.setRowSelected(currentIndex, !r.isSelected)
                        }
                    }

                    delegate: Rectangle {
                        id: rowDelegate
                        width: statementListView.width
                        height: 34
                        visible: {
                            if (root.currentFilter === "RECEIPTS" && model.deposit <= 0.001) return false
                            if (root.currentFilter === "PAYMENTS" && model.withdrawal <= 0.001) return false
                            if (root.currentFilter === "CHARGES" && model.category !== "BANK_CHARGES") return false
                            if (root.currentFilter === "INTEREST" && model.category !== "INTEREST_DEBIT") return false
                            if (root.currentFilter === "UNMATCHED" && model.confidence !== "UNMATCHED") return false
                            if (root.currentFilter === "DUPLICATES" && !model.isDuplicate) return false

                            if (root.searchQuery !== "") {
                                var q = root.searchQuery
                                var n = (model.narration || "").toLowerCase()
                                var u = (model.utrRef || "").toLowerCase()
                                var dr = (model.drAccount || "").toLowerCase()
                                var cr = (model.crAccount || "").toLowerCase()
                                if (!n.includes(q) && !u.includes(q) && !dr.includes(q) && !cr.includes(q)) return false
                            }
                            return true
                        }

                        property bool isCurrent: statementListView.currentIndex === index && statementListView.activeFocus
                        color: isCurrent ? "#DBEAFE" : (model.isSelected ? (model.deposit > 0.001 ? "#F0FDF4" : "#FFF7ED") : (index % 2 === 0 ? "#FFFFFF" : "#F8FAFC"))
                        border.color: isCurrent ? "#2563EB" : (model.isSelected ? (model.deposit > 0.001 ? "#86EFAC" : "#FDBA74") : "#F1F5F9")
                        border.width: isCurrent ? 2 : 1
                        radius: 3

                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: 8; anchors.rightMargin: 8
                            spacing: 6

                            // Row Checkbox
                            CustomCheckBox {
                                Layout.preferredWidth: 24
                                Layout.minimumWidth: 24
                                boxSize: 15
                                boxRadius: 3
                                checkedColor: "#2563EB"
                                checked: Boolean(model.isSelected)
                                onToggled: {
                                    bankStatementCtrl.rowsModel.setRowSelected(index, !model.isSelected)
                                }
                            }

                            Text { text: model.rowIndex ? model.rowIndex.toString() : (index + 1).toString(); color: "#64748B"; font.pixelSize: 10; Layout.preferredWidth: 28; Layout.minimumWidth: 28 }
                            Text { text: model.vDate ? model.vDate : ""; color: "#334155"; font.pixelSize: 10; Layout.preferredWidth: 72; Layout.minimumWidth: 72 }

                            // Voucher Type Badge
                            Rectangle {
                                Layout.preferredWidth: 75
                                Layout.minimumWidth: 75
                                height: 20
                                radius: 3
                                color: model.deposit > 0.001 ? "#DCFCE7" : "#FEE2E2"
                                border.color: model.deposit > 0.001 ? "#86EFAC" : "#FCA5A5"
                                Text {
                                    anchors.centerIn: parent
                                    text: model.deposit > 0.001 ? "Receipt" : "Payment"
                                    color: model.deposit > 0.001 ? "#15803D" : "#B91C1C"
                                    font.pixelSize: 9; font.bold: true
                                }
                            }

                            // Bank Narration with UTR pill (Critical: Layout.preferredWidth: 0 to allow responsive elision)
                            RowLayout {
                                Layout.fillWidth: true
                                Layout.minimumWidth: 100
                                Layout.preferredWidth: 0
                                spacing: 4
                                clip: true

                                Rectangle {
                                    visible: Boolean(model.utrRef && model.utrRef.length > 0)
                                    height: 16; radius: 3; color: "#EFF6FF"; border.color: "#BFDBFE"
                                    implicitWidth: utrTxt.implicitWidth + 6
                                    Text { id: utrTxt; anchors.centerIn: parent; text: model.utrRef || ""; color: "#1D4ED8"; font.pixelSize: 9; font.bold: true }
                                }
                                Text {
                                    text: model.narration ? model.narration : ""
                                    color: "#0F172A"
                                    font.pixelSize: 10
                                    Layout.fillWidth: true
                                    Layout.minimumWidth: 50
                                    Layout.preferredWidth: 0
                                    elide: Text.ElideRight
                                }
                            }

                            // DEBIT ACCOUNT (Dr Leg)
                            Rectangle {
                                Layout.preferredWidth: 145
                                Layout.minimumWidth: 145
                                height: 24
                                radius: 3
                                color: model.drAccount ? "#EFF6FF" : "#FEF2F2"
                                border.color: model.drAccount ? "#93C5FD" : "#FCA5A5"
                                border.width: 1
                                RowLayout {
                                    anchors.fill: parent
                                    anchors.leftMargin: 5; anchors.rightMargin: 5
                                    Text {
                                        text: model.drAccount ? model.drAccount : "Select Dr..."
                                        color: model.drAccount ? "#1E40AF" : "#DC2626"
                                        font.pixelSize: 9
                                        font.bold: true
                                        Layout.fillWidth: true
                                        elide: Text.ElideRight
                                    }
                                    Text { text: "Edit"; color: "#60A5FA"; font.pixelSize: 9; font.bold: true }
                                }
                                MouseArea {
                                    anchors.fill: parent
                                    cursorShape: Qt.PointingHandCursor
                                    onClicked: {
                                        statementListView.currentIndex = index
                                        partyPopupScope.openForEdit(index, "Dr", model.drAccount || "")
                                    }
                                }
                            }

                            // CREDIT ACCOUNT (Cr Leg)
                            Rectangle {
                                Layout.preferredWidth: 145
                                Layout.minimumWidth: 145
                                height: 24
                                radius: 3
                                color: model.crAccount ? "#FFF7ED" : "#FEF2F2"
                                border.color: model.crAccount ? "#FDBA74" : "#FCA5A5"
                                border.width: 1
                                RowLayout {
                                    anchors.fill: parent
                                    anchors.leftMargin: 5; anchors.rightMargin: 5
                                    Text {
                                        text: model.crAccount ? model.crAccount : "Select Cr..."
                                        color: model.crAccount ? "#9A3412" : "#DC2626"
                                        font.pixelSize: 9
                                        font.bold: true
                                        Layout.fillWidth: true
                                        elide: Text.ElideRight
                                    }
                                    Text { text: "Edit"; color: "#F97316"; font.pixelSize: 9; font.bold: true }
                                }
                                MouseArea {
                                    anchors.fill: parent
                                    cursorShape: Qt.PointingHandCursor
                                    onClicked: {
                                        statementListView.currentIndex = index
                                        partyPopupScope.openForEdit(index, "Cr", model.crAccount || "")
                                    }
                                }
                            }

                            // Amount
                            Text {
                                text: (typeof dashboardCtrl !== "undefined" && dashboardCtrl) ? dashboardCtrl.format_inr(model.amount) : ((parseFloat(model.amount) || 0.0).toFixed(2))
                                color: model.deposit > 0.001 ? "#16A34A" : "#DC2626"
                                font.pixelSize: 10
                                font.bold: true
                                Layout.preferredWidth: 90
                                Layout.minimumWidth: 90
                                horizontalAlignment: Text.AlignRight
                            }

                            // Confidence & Audit Status Badge
                            Rectangle {
                                Layout.preferredWidth: 85
                                Layout.minimumWidth: 85
                                height: 20
                                radius: 3
                                color: model.isDuplicate ? "#F1F5F9" : (model.confidence === "ALIAS_MATCH" ? "#F5F3FF" : (model.confidence === "HIGH" ? "#ECFDF5" : (model.confidence === "MEDIUM" ? "#EFF6FF" : "#FEF2F2")))
                                border.color: model.isDuplicate ? "#CBD5E1" : (model.confidence === "ALIAS_MATCH" ? "#DDD6FE" : (model.confidence === "HIGH" ? "#A7F3D0" : (model.confidence === "MEDIUM" ? "#BFDBFE" : "#FECACA")))

                                RowLayout {
                                    anchors.centerIn: parent
                                    Text {
                                        text: model.isDuplicate ? "Duplicate" : (model.confidence === "ALIAS_MATCH" ? "Learned" : (model.confidence === "HIGH" ? "Matched" : (model.confidence === "MEDIUM" ? "Suggestion" : "Unmatched")))
                                        color: model.isDuplicate ? "#64748B" : (model.confidence === "ALIAS_MATCH" ? "#6D28D9" : (model.confidence === "HIGH" ? "#047857" : (model.confidence === "MEDIUM" ? "#1D4ED8" : "#B91C1C")))
                                        font.pixelSize: 9; font.bold: true
                                    }
                                }
                            }
                        }

                        MouseArea {
                            anchors.fill: parent
                            z: -1
                            onClicked: {
                                statementListView.currentIndex = index
                                statementListView.forceActiveFocus()
                            }
                        }
                    }
                }

                // Empty State when no statement is loaded
                Rectangle {
                    visible: typeof bankStatementCtrl === "undefined" || !bankStatementCtrl || bankStatementCtrl.totalCount === 0
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    color: "#F8FAFC"

                    ColumnLayout {
                        anchors.centerIn: parent
                        spacing: 10

                        Rectangle {
                            Layout.alignment: Qt.AlignHCenter
                            width: 44; height: 44; radius: 8
                            color: "#DBEAFE"
                            border.color: "#93C5FD"
                            Text {
                                anchors.centerIn: parent
                                text: "PDF"
                                color: "#1E40AF"
                                font.pixelSize: 13
                                font.bold: true
                            }
                        }

                        Text {
                            Layout.alignment: Qt.AlignHCenter
                            text: "No Bank Statement Loaded"
                            color: "#0F172A"
                            font.pixelSize: 15
                            font.bold: true
                        }

                        Text {
                            Layout.alignment: Qt.AlignHCenter
                            text: "Select a Canara Bank Statement PDF or Drag & Drop it here"
                            color: "#64748B"
                            font.pixelSize: 11
                        }

                        T.Button {
                            id: emptySelectBtn
                            Layout.alignment: Qt.AlignHCenter
                            implicitHeight: 32
                            implicitWidth: 190
                            background: Rectangle {
                                color: emptySelectBtn.hovered ? "#1D4ED8" : "#2563EB"
                                radius: 4
                            }
                            contentItem: RowLayout {
                                spacing: 4
                                Text {
                                    text: "Select PDF Statement"
                                    color: "#FFFFFF"
                                    font.pixelSize: 11
                                    font.bold: true
                                }
                                KbdBadge { text: "Ctrl+O"; badgeColor: "#1E40AF"; textColor: "#DBEAFE"; borderColor: "#2563EB" }
                            }
                            onClicked: {
                                if (typeof bankStatementCtrl !== "undefined" && bankStatementCtrl) {
                                    bankStatementCtrl.browseStatementFile()
                                }
                            }
                        }
                    }
                }
            }
        }

        // 5. FOOTER RECONCILIATION BAR & PRIMARY POST BUTTON
        Rectangle {
            Layout.fillWidth: true
            height: 42
            color: "#0F172A"
            radius: 6

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 12; anchors.rightMargin: 12

                Text { text: "NET SELECTED:"; color: "#94A3B8"; font.pixelSize: 10; font.bold: true }

                RowLayout {
                    spacing: 6
                    Text { text: "Debits: " + (bankStatementCtrl ? bankStatementCtrl.selectedWithdrawalsFmt : "0.00"); color: "#F87171"; font.pixelSize: 11; font.bold: true }
                    Text { text: "|"; color: "#475569" }
                    Text { text: "Credits: " + (bankStatementCtrl ? bankStatementCtrl.selectedDepositsFmt : "0.00"); color: "#4ADE80"; font.pixelSize: 11; font.bold: true }
                    Text { text: "|"; color: "#475569" }
                    Text {
                        property real netDiff: (bankStatementCtrl ? (bankStatementCtrl.selectedDeposits - bankStatementCtrl.selectedWithdrawals) : 0.0)
                        text: "Net Delta: " + (typeof dashboardCtrl !== "undefined" && dashboardCtrl ? dashboardCtrl.format_inr(netDiff) : (netDiff.toFixed(2)))
                        color: netDiff >= 0 ? "#60A5FA" : "#F87171"
                        font.pixelSize: 11
                        font.bold: true
                    }
                }

                Item { Layout.fillWidth: true }

                // Post Button
                T.Button {
                    id: postBtn
                    implicitHeight: 30
                    implicitWidth: contentItem.implicitWidth + 20
                    background: Rectangle {
                        color: postBtn.hovered ? "#15803D" : "#16A34A"
                        radius: 4
                    }
                    contentItem: RowLayout {
                        spacing: 4
                        Text {
                            text: "Post " + (bankStatementCtrl ? bankStatementCtrl.selectedCount.toString() : "0") + " Vouchers to Ledger"
                            color: "#FFFFFF"
                            font.pixelSize: 11
                            font.bold: true
                        }
                        KbdBadge { text: "Ctrl+S"; badgeColor: "#14532D"; textColor: "#BBF7D0"; borderColor: "#16A34A" }
                    }
                    onClicked: postVouchersAction()
                }
            }
        }
    }

    // PARTY SEARCH & EDIT POPUP MODAL (NATIVE C++ DATABASE SEARCH)
    Item {
        id: partyPopupScope
        anchors.fill: parent
        visible: root.isPartySelectorOpen

        property var searchResults: []

        function updatePartySearch(q) {
            if (typeof bankStatementCtrl !== "undefined" && bankStatementCtrl) {
                searchResults = bankStatementCtrl.searchPartyLedgers(q ? q.trim() : "")
            } else {
                searchResults = []
            }
            if (searchResults.length > 0) {
                partyListView.currentIndex = 0
            }
        }

        function openForEdit(rowIdx, field, currentVal) {
            root.editingRowIdx = rowIdx
            root.editingField = field
            var searchSeed = (currentVal && !currentVal.startsWith("Select ")) ? currentVal : ""
            partyFilterInput.text = searchSeed
            updatePartySearch(searchSeed)
            root.isPartySelectorOpen = true
            Qt.callLater(function() {
                partyFilterInput.forceActiveFocus()
                partyFilterInput.selectAll()
            })
        }

        Rectangle {
            anchors.fill: parent
            color: "#80000000"
            MouseArea { anchors.fill: parent; onClicked: root.isPartySelectorOpen = false }
        }

        Rectangle {
            width: 500; height: 440; radius: 8
            anchors.centerIn: parent
            color: "#FFFFFF"
            border.color: "#CBD5E1"
            border.width: 1

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 14
                spacing: 10

                RowLayout {
                    Layout.fillWidth: true
                    Text {
                        text: "Select " + root.editingField + " Ledger Account for Row #" + (root.editingRowIdx + 1)
                        color: "#0F172A"
                        font.pixelSize: 13
                        font.bold: true
                    }
                    Item { Layout.fillWidth: true }
                    Text {
                        text: "x"
                        color: "#64748B"
                        font.bold: true
                        font.pixelSize: 12
                        MouseArea { anchors.fill: parent; onClicked: root.isPartySelectorOpen = false }
                    }
                }

                Rectangle {
                    Layout.fillWidth: true; height: 34; radius: 4; color: "#F8FAFC"; border.color: "#2563EB"; border.width: 1.5
                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: 8; anchors.rightMargin: 8
                        spacing: 4
                        Text { text: "Search:"; color: "#64748B"; font.pixelSize: 11; font.bold: true }
                        TextInput {
                            id: partyFilterInput
                            Layout.fillWidth: true
                            font.pixelSize: 12
                            font.bold: true
                            color: "#0F172A"
                            selectByMouse: true
                            onTextChanged: partyPopupScope.updatePartySearch(text)
                            Keys.onDownPressed: function(event) {
                                event.accepted = true
                                if (partyPopupScope.searchResults.length > 0) {
                                    partyListView.currentIndex = Math.min(partyPopupScope.searchResults.length - 1, partyListView.currentIndex + 1)
                                }
                            }
                            Keys.onUpPressed: function(event) {
                                event.accepted = true
                                if (partyPopupScope.searchResults.length > 0) {
                                    partyListView.currentIndex = Math.max(0, partyListView.currentIndex - 1)
                                }
                            }
                            Keys.onReturnPressed: function(event) {
                                event.accepted = true
                                if (partyPopupScope.searchResults.length > 0 && partyListView.currentIndex >= 0 && partyListView.currentIndex < partyPopupScope.searchResults.length) {
                                    var item = partyPopupScope.searchResults[partyListView.currentIndex]
                                    if (item && item.name) root.applyPartySelection(item.name)
                                } else if (partyFilterInput.text.trim() !== "") {
                                    root.applyPartySelection(partyFilterInput.text.trim())
                                }
                            }
                            Keys.onEnterPressed: function(event) {
                                event.accepted = true
                                if (partyPopupScope.searchResults.length > 0 && partyListView.currentIndex >= 0 && partyListView.currentIndex < partyPopupScope.searchResults.length) {
                                    var item = partyPopupScope.searchResults[partyListView.currentIndex]
                                    if (item && item.name) root.applyPartySelection(item.name)
                                } else if (partyFilterInput.text.trim() !== "") {
                                    root.applyPartySelection(partyFilterInput.text.trim())
                                }
                            }
                            Keys.onEscapePressed: function(event) {
                                event.accepted = true
                                root.isPartySelectorOpen = false
                            }
                        }
                    }
                }

                // Match count indicator
                RowLayout {
                    Layout.fillWidth: true
                    Text {
                        text: partyPopupScope.searchResults.length + " Ledgers Found"
                        color: "#64748B"
                        font.pixelSize: 10
                    }
                    Item { Layout.fillWidth: true }
                    Text {
                        text: "Use Up/Down and Enter to select"
                        color: "#94A3B8"
                        font.pixelSize: 10
                    }
                }

                ListView {
                    id: partyListView
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true
                    model: partyPopupScope.searchResults
                    spacing: 2
                    currentIndex: 0
                    boundsBehavior: Flickable.StopAtBounds

                    Keys.onUpPressed: function(event) {
                        event.accepted = true
                        if (currentIndex > 0) {
                            currentIndex--
                        } else {
                            partyFilterInput.forceActiveFocus()
                        }
                    }

                    Keys.onDownPressed: function(event) {
                        event.accepted = true
                        if (currentIndex < partyPopupScope.searchResults.length - 1) currentIndex++
                    }

                    Keys.onReturnPressed: function(event) {
                        event.accepted = true
                        if (partyPopupScope.searchResults.length > 0 && currentIndex >= 0 && currentIndex < partyPopupScope.searchResults.length) {
                            var item = partyPopupScope.searchResults[currentIndex]
                            if (item && item.name) root.applyPartySelection(item.name)
                        }
                    }

                    Keys.onEnterPressed: function(event) {
                        event.accepted = true
                        if (partyPopupScope.searchResults.length > 0 && currentIndex >= 0 && currentIndex < partyPopupScope.searchResults.length) {
                            var item = partyPopupScope.searchResults[currentIndex]
                            if (item && item.name) root.applyPartySelection(item.name)
                        }
                    }

                    Keys.onEscapePressed: function(event) {
                        event.accepted = true
                        root.isPartySelectorOpen = false
                    }

                    delegate: Rectangle {
                        width: partyListView.width
                        height: 36
                        radius: 4
                        color: partyListView.currentIndex === index ? "#DBEAFE" : (pMouse.containsMouse ? "#F1F5F9" : "#FFFFFF")
                        border.color: partyListView.currentIndex === index ? "#2563EB" : "#E2E8F0"
                        border.width: partyListView.currentIndex === index ? 1.5 : 1

                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: 10; anchors.rightMargin: 10
                            spacing: 8

                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: 1
                                Text {
                                    text: (modelData && modelData.name) ? modelData.name : ""
                                    color: partyListView.currentIndex === index ? "#1E40AF" : "#0F172A"
                                    font.pixelSize: 12
                                    font.bold: true
                                    Layout.fillWidth: true
                                    elide: Text.ElideRight
                                }
                                Text {
                                    visible: Boolean(modelData && (modelData.group_name || modelData.city))
                                    text: modelData ? ((modelData.group_name || "") + (modelData.city ? " • " + modelData.city : "")) : ""
                                    color: "#64748B"
                                    font.pixelSize: 10
                                    Layout.fillWidth: true
                                    elide: Text.ElideRight
                                }
                            }
                        }

                        MouseArea {
                            id: pMouse
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: {
                                if (modelData && modelData.name) {
                                    root.applyPartySelection(modelData.name)
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    // SUCCESS / CONFIRMATION MODAL
    Rectangle {
        id: postResultModal
        anchors.fill: parent
        color: "#80000000"
        visible: false

        property string title: ""
        property string message: ""
        property bool isSuccess: true

        Rectangle {
            width: 400; height: 200; radius: 8
            anchors.centerIn: parent
            color: "#FFFFFF"
            border.color: "#CBD5E1"

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 16
                spacing: 8

                RowLayout {
                    spacing: 8
                    Rectangle {
                        width: 26; height: 26; radius: 4
                        color: postResultModal.isSuccess ? "#DCFCE7" : "#FEE2E2"
                        Text {
                            anchors.centerIn: parent
                            text: postResultModal.isSuccess ? "OK" : "!"
                            color: postResultModal.isSuccess ? "#15803D" : "#B91C1C"
                            font.bold: true
                            font.pixelSize: 11
                        }
                    }
                    Text { text: postResultModal.title; color: "#0F172A"; font.pixelSize: 14; font.bold: true; Layout.fillWidth: true }
                }

                Text {
                    text: postResultModal.message
                    color: "#475569"
                    font.pixelSize: 11
                    wrapMode: Text.Wrap
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                }

                RowLayout {
                    Layout.fillWidth: true
                    Item { Layout.fillWidth: true }
                    T.Button {
                        implicitHeight: 28
                        implicitWidth: 80
                        background: Rectangle { color: "#2563EB"; radius: 4 }
                        contentItem: Text { text: "OK"; color: "#FFFFFF"; font.bold: true; font.pixelSize: 11; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                        onClicked: postResultModal.visible = false
                    }
                }
            }
        }
    }
}
