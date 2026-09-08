import QtQuick
import QtQuick.Templates as T
import QtQuick.Layouts
import MahadevERP

Rectangle {
    id: root
    color: "#F4F6F9"

    signal cancelRequested()

    property string currentPartyName: (typeof ledgerStatementCtrl !== "undefined" && ledgerStatementCtrl) ? ledgerStatementCtrl.currentPartyName : ""

    function focusSearch() {
        partySearchInput.forceActiveFocus()
        partySearchInput.selectAll()
        partySearchBox.updateSearch()
        if (!partySearchPopup.visible) {
            partySearchPopup.open()
        }
    }

    function resetSearchAndFocus() {
        partySearchInput.text = ""
        currentPartyName = ""
        if (typeof window !== "undefined") {
            window.lastViewedPartyName = ""
            window.targetStatementParty = ""
        }
        if (typeof ledgerStatementCtrl !== "undefined" && ledgerStatementCtrl) {
            ledgerStatementCtrl.loadPartyStatement("")
        }
        Qt.callLater(focusSearch)
    }

    Component.onCompleted: {
        syncDateInputsWithActivePeriod()
        var initParty = (typeof window !== "undefined" && typeof window.targetStatementParty !== "undefined" && window.targetStatementParty) ? window.targetStatementParty : ""
        if (initParty !== "") {
            window.targetStatementParty = ""
            partySearchInput.text = initParty
            loadPartyStatement(initParty)
        } else {
            resetSearchAndFocus()
        }
        Qt.callLater(focusSearch)
    }

    onVisibleChanged: {
        if (visible) {
            syncDateInputsWithActivePeriod()
            var pToLoad = (typeof window !== "undefined" && typeof window.targetStatementParty !== "undefined" && window.targetStatementParty) ? window.targetStatementParty : ""
            if (pToLoad !== "") {
                window.targetStatementParty = ""
                partySearchInput.text = pToLoad
                loadPartyStatement(pToLoad)
            } else if (!currentPartyName) {
                resetSearchAndFocus()
            }
            Qt.callLater(focusSearch)
        }
    }

    function syncDateInputsWithActivePeriod() {
        if (typeof stockItemsModel !== "undefined" && stockItemsModel) {
            var sd = stockItemsModel.get_from_date()
            var ed = stockItemsModel.get_to_date()
            if (sd && ed) {
                var sParts = sd.split("-")
                var eParts = ed.split("-")
                if (sParts.length === 3) fromDateInput.text = sParts[2] + "-" + sParts[1] + "-" + sParts[0]
                if (eParts.length === 3) toDateInput.text = eParts[2] + "-" + eParts[1] + "-" + eParts[0]
            } else {
                fromDateInput.text = ""
                toDateInput.text = ""
            }
        }
    }

    function toIso(dStr) {
        if (!dStr) return ""
        var parts = dStr.trim().split("-")
        if (parts.length === 3 && parts[2].length === 4) {
            return parts[2] + "-" + parts[1] + "-" + parts[0]
        }
        return dStr
    }

    function loadPartyStatement(pName) {
        if (pName && typeof window !== "undefined") {
            window.lastViewedPartyName = pName
        }
        var fIso = toIso(fromDateInput.text)
        var tIso = toIso(toDateInput.text)
        if (typeof ledgerStatementCtrl !== "undefined" && ledgerStatementCtrl) {
            ledgerStatementCtrl.loadPartyStatement(pName, fIso, tIso)
        }
    }

    function applyDateFilterAndSort() {
        var fIso = toIso(fromDateInput.text)
        var tIso = toIso(toDateInput.text)
        if (typeof ledgerStatementCtrl !== "undefined" && ledgerStatementCtrl) {
            ledgerStatementCtrl.applyDateFilter(fIso, tIso)
        }
    }

    function printStatement() {
        if (typeof printExportCtrl !== "undefined" && printExportCtrl && partySearchInput.text.trim() !== "") {
            var fIso = toIso(fromDateInput.text)
            var tIso = toIso(toDateInput.text)
            printExportCtrl.print_ledger_statement(partySearchInput.text.trim(), fIso, tIso)
        }
    }

    function exportPdf() {
        if (typeof printExportCtrl !== "undefined" && printExportCtrl && partySearchInput.text.trim() !== "") {
            var fIso = toIso(fromDateInput.text)
            var tIso = toIso(toDateInput.text)
            printExportCtrl.export_ledger_statement_pdf(partySearchInput.text.trim(), fIso, tIso)
        }
    }

    function exportCsv() {
        if (typeof printExportCtrl !== "undefined" && printExportCtrl && partySearchInput.text.trim() !== "") {
            var fIso = toIso(fromDateInput.text)
            var tIso = toIso(toDateInput.text)
            printExportCtrl.export_ledger_csv(partySearchInput.text.trim(), fIso, tIso)
        }
    }

    function openVoucherEntry(item) {
        if (!item) return
        var vType = item.voucher_type || item.trans_type || ""
        var rawType = item.legacy_type || item.trans_type || ""
        var vNoStr = (item.voucher_no || item.refNo || "").toString()
        var vNo = parseInt(vNoStr.replace(/\D/g, "")) || 0
        var vDate = item.vIso || toIso(item.vDate)

        // Check if it's a TDS voucher
        if (vType === "TDS" || rawType === "TDS" || (item.particulars && item.particulars.indexOf("T.D.S.") !== -1)) {
            if (typeof tdsModel !== "undefined" && tdsModel) {
                var tdsVch = tdsModel.get_tds_voucher_by_ref(vNo, vDate)
                if (tdsVch && tdsVch.id) {
                    if (typeof window !== "undefined") {
                        window.targetTdsVoucherId = tdsVch.id
                        if (typeof window.navigateToView === "function") {
                            window.navigateToView(24) // TDS Voucher View
                        } else {
                            window.currentViewIndex = 24
                        }
                    }
                    return
                }
            }
            if (typeof window !== "undefined") {
                if (typeof window.navigateToView === "function") {
                    window.navigateToView(24)
                } else {
                    window.currentViewIndex = 24
                }
            }
            return
        }

        if (vType === "Sales" || rawType === "Sale") {
            if (typeof window !== "undefined") {
                window.pendingEditInvoiceNo = item.invoice_no || vNoStr
                if (typeof window.navigateToView === "function") {
                    window.navigateToView(14) // Sales Voucher
                } else {
                    window.currentViewIndex = 14
                }
            }
            return
        }
        if (vType === "Purchase" || rawType === "Purc") {
            if (typeof window !== "undefined") {
                window.pendingEditInvoiceNo = item.invoice_no || vNoStr
                if (typeof window.navigateToView === "function") {
                    window.navigateToView(15) // Purchase Voucher
                } else {
                    window.currentViewIndex = 15
                }
            }
            return
        }
        if (vType === "Payment" || vType === "Receipt" || rawType === "ChPt" || rawType === "ChRt" || rawType === "Pymt" || rawType === "Rcpt") {
            if (typeof window !== "undefined") {
                if (typeof window.navigateToView === "function") {
                    window.navigateToView(16) // Cheque Voucher
                } else {
                    window.currentViewIndex = 16
                }
            }
            return
        }
        if (vType === "Journal" || rawType === "Jrnl") {
            if (typeof window !== "undefined") {
                if (typeof window.navigateToView === "function") {
                    window.navigateToView(17) // Journal Voucher
                } else {
                    window.currentViewIndex = 17
                }
            }
            return
        }
        if (vType === "J-Form" || rawType === "JFrm") {
            if (typeof window !== "undefined") {
                if (typeof window.navigateToView === "function") {
                    window.navigateToView(23) // J-Form Voucher
                } else {
                    window.currentViewIndex = 23
                }
            }
            return
        }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 12

        // 1. PAGE HEADER BAR
        RowLayout {
            Layout.fillWidth: true
            spacing: 12

            ColumnLayout {
                spacing: 2
                Text {
                    text: "Account Ledger Statement (2-Column Dr / Cr)"
                    color: "#0F172A"
                    font.pixelSize: 18
                    font.bold: true
                }
                Text {
                    text: "Side-by-side Credit (Cr) and Debit (Dr) accounting ledger with interactive reconciliation."
                    color: "#64748B"
                    font.pixelSize: 11
                }
            }

            Item { Layout.fillWidth: true }

            T.Button {
                id: printBtn
                implicitWidth: contentItem.implicitWidth + 20
                implicitHeight: 32
                background: Rectangle { color: printBtn.hovered ? "#1D4ED8" : "#2563EB"; radius: 6 }
                contentItem: RowLayout {
                    spacing: 6
                    Text { text: "Print Statement"; color: "#FFFFFF"; font.pixelSize: 12; font.bold: true }
                    KbdBadge { text: "Ctrl+P"; badgeColor: "#1E3A8A"; textColor: "#93C5FD"; borderColor: "#2563EB" }
                }
                onClicked: root.printStatement()
            }

            T.Button {
                id: pdfBtn
                implicitWidth: contentItem.implicitWidth + 20
                implicitHeight: 32
                background: Rectangle { color: pdfBtn.hovered ? "#047857" : "#059669"; radius: 6 }
                contentItem: RowLayout {
                    spacing: 6
                    Text { text: "Export PDF"; color: "#FFFFFF"; font.pixelSize: 12; font.bold: true }
                    KbdBadge { text: "Alt+P"; badgeColor: "#064E3B"; textColor: "#A7F3D0"; borderColor: "#059669" }
                }
                onClicked: root.exportPdf()
            }

            T.Button {
                id: csvBtn
                implicitWidth: contentItem.implicitWidth + 16
                implicitHeight: 32
                background: Rectangle { color: "#F8FAFC"; radius: 6; border.color: "#CBD5E1" }
                contentItem: RowLayout {
                    spacing: 6
                    Text { text: "Excel CSV"; color: "#334155"; font.pixelSize: 12; font.bold: true }
                }
                onClicked: root.exportCsv()
            }

            T.Button {
                id: backBtn
                implicitWidth: contentItem.implicitWidth + 24
                implicitHeight: 32
                background: Rectangle { color: "#F1F5F9"; radius: 6; border.color: "#CBD5E1" }
                contentItem: RowLayout {
                    spacing: 6
                    Text { text: "← Back to Dashboard"; color: "#475569"; font.pixelSize: 12; font.bold: true }
                    KbdBadge { text: "Esc"; badgeColor: "#DC2626"; textColor: "#FFF"; borderColor: "#B91C1C" }
                }
                onClicked: root.cancelRequested()
            }
        }

        // 2. FILTER & PARTY SELECTION BAR (FULL FINANCIAL YEAR BY DEFAULT)
        Rectangle {
            Layout.fillWidth: true
            height: 52
            color: "#FFFFFF"
            border.color: "#CBD5E1"
            radius: 8
            z: 10

            RowLayout {
                anchors.fill: parent
                anchors.margins: 10
                spacing: 12

                // Native Ultra-Fast Party Search Field
                Rectangle {
                    id: partySearchBox
                    Layout.preferredWidth: 520
                    height: 36
                    color: "#FFFFFF"
                    radius: 6
                    border.color: partySearchInput.activeFocus ? "#2563EB" : "#CBD5E1"
                    border.width: partySearchInput.activeFocus ? 2 : 1

                    property var searchResults: []

                    function updateSearch() {
                        var q = partySearchInput.text.trim()
                        if (typeof ledgerStatementCtrl !== "undefined" && ledgerStatementCtrl) {
                            searchResults = ledgerStatementCtrl.searchParties(q)
                            if (searchResults.length > 0) {
                                partySearchList.currentIndex = 0
                            }
                        }
                    }

                    function selectParty(pName) {
                        if (!pName) return
                        partySearchInput.text = pName
                        searchResults = []
                        partySearchPopup.close()
                        root.loadPartyStatement(pName)
                        crListView.forceActiveFocus()
                        if (typeof ledgerStatementCtrl !== "undefined" && ledgerStatementCtrl && ledgerStatementCtrl.crModel.count > 0) {
                            crListView.currentIndex = 0
                        }
                    }

                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: 10; anchors.rightMargin: 10
                        spacing: 8

                        Text { text: ""; font.pixelSize: 13 }

                        Item {
                            Layout.fillWidth: true
                            Layout.fillHeight: true

                            TextInput {
                                id: partySearchInput
                                anchors.fill: parent
                                verticalAlignment: TextInput.AlignVCenter
                                color: "#000000"
                                selectionColor: "#2563EB"
                                selectedTextColor: "#FFFFFF"
                                font.pixelSize: 13
                                font.bold: true
                                font.family: "Segoe UI, -apple-system, Roboto, sans-serif"
                                selectByMouse: true
                                clip: true
                                focus: true

                                onActiveFocusChanged: {
                                    if (activeFocus) {
                                        partySearchBox.updateSearch()
                                        partySearchPopup.open()
                                        partySearchInput.selectAll()
                                    }
                                }

                                onTextChanged: {
                                    if (activeFocus) {
                                        partySearchBox.updateSearch()
                                        if (!partySearchPopup.visible) {
                                            partySearchPopup.open()
                                        }
                                    }
                                }

                                Keys.onDownPressed: function(event) {
                                    event.accepted = true
                                    if (partySearchPopup.visible && partySearchBox.searchResults.length > 0) {
                                        partySearchList.currentIndex = Math.min(partySearchBox.searchResults.length - 1, partySearchList.currentIndex + 1)
                                    } else {
                                        crListView.forceActiveFocus()
                                        if (crListView.currentIndex < 0 && ledgerStatementCtrl && ledgerStatementCtrl.crModel.count > 0) {
                                            crListView.currentIndex = 0
                                        }
                                    }
                                }

                                Keys.onUpPressed: function(event) {
                                    event.accepted = true
                                    if (partySearchPopup.visible && partySearchBox.searchResults.length > 0) {
                                        partySearchList.currentIndex = Math.max(0, partySearchList.currentIndex - 1)
                                    }
                                }

                                Keys.onReturnPressed: function(event) {
                                    event.accepted = true
                                    if (partySearchPopup.visible && partySearchBox.searchResults.length > 0 && partySearchList.currentIndex >= 0) {
                                        var item = partySearchBox.searchResults[partySearchList.currentIndex]
                                        partySearchBox.selectParty(item.name)
                                    } else if (partySearchInput.text.trim() !== "") {
                                        partySearchBox.selectParty(partySearchInput.text.trim())
                                    }
                                }

                                Keys.onEnterPressed: function(event) {
                                    event.accepted = true
                                    if (partySearchPopup.visible && partySearchBox.searchResults.length > 0 && partySearchList.currentIndex >= 0) {
                                        var item = partySearchBox.searchResults[partySearchList.currentIndex]
                                        partySearchBox.selectParty(item.name)
                                    } else if (partySearchInput.text.trim() !== "") {
                                        partySearchBox.selectParty(partySearchInput.text.trim())
                                    }
                                }

                                Keys.onEscapePressed: function(event) {
                                    if (partySearchPopup.visible) {
                                        event.accepted = true
                                        partySearchPopup.close()
                                    } else {
                                        root.cancelRequested()
                                    }
                                }
                            }

                            Text {
                                anchors.fill: parent
                                verticalAlignment: Text.AlignVCenter
                                text: "Search Party Name / Ledger... (Alt+S)"
                                color: "#64748B"
                                font.pixelSize: 13
                                font.bold: false
                                font.family: "Segoe UI, -apple-system, Roboto, sans-serif"
                                visible: partySearchInput.text === "" && !partySearchInput.inputMethodComposing
                            }
                        }

                        T.Button {
                            implicitWidth: 20
                            implicitHeight: 20
                            flat: true
                            visible: partySearchInput.text.length > 0
                            contentItem: Text { text: "X"; color: "#94A3B8"; font.pixelSize: 11; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                            onClicked: {
                                root.resetSearchAndFocus()
                            }
                        }
                    }

                    T.Popup {
                        id: partySearchPopup
                        y: partySearchBox.height + 4
                        width: partySearchBox.width
                        height: Math.min(320, partySearchBox.searchResults.length * 44 + 10)
                        padding: 4
                        closePolicy: T.Popup.CloseOnPressOutside | T.Popup.CloseOnEscape
                        focus: false

                        background: Rectangle {
                            color: "#FFFFFF"
                            border.color: "#2563EB"
                            border.width: 1.5
                            radius: 8
                        }

                        ListView {
                            id: partySearchList
                            anchors.fill: parent
                            clip: true
                            model: partySearchBox.searchResults
                            currentIndex: 0
                            spacing: 2

                            delegate: Rectangle {
                                width: partySearchList.width
                                height: 40
                                radius: 4
                                color: index === partySearchList.currentIndex ? "#EFF6FF" : (hoverArea.containsMouse ? "#F8FAFC" : "#FFFFFF")
                                border.color: index === partySearchList.currentIndex ? "#BFDBFE" : "transparent"

                                MouseArea {
                                    id: hoverArea
                                    anchors.fill: parent
                                    hoverEnabled: true
                                    onClicked: {
                                        partySearchBox.selectParty(modelData.name)
                                    }
                                }

                                RowLayout {
                                    anchors.fill: parent
                                    anchors.leftMargin: 12; anchors.rightMargin: 12
                                    spacing: 8

                                    ColumnLayout {
                                        Layout.fillWidth: true
                                        spacing: 1
                                        Text {
                                            text: modelData.name || ""
                                            color: "#000000"
                                            font.pixelSize: 13
                                            font.bold: true
                                            elide: Text.ElideRight
                                        }
                                        Text {
                                            text: (modelData.group_name || "") + (modelData.city ? " • " + modelData.city : "")
                                            color: "#64748B"
                                            font.pixelSize: 11
                                            elide: Text.ElideRight
                                        }
                                    }
                                }
                            }
                        }
                    }
                }

                // Active Financial Year Badge Indicator
                Rectangle {
                    height: 32
                    Layout.preferredWidth: 180
                    color: "#EFF6FF"
                    border.color: "#93C5FD"
                    radius: 6

                    RowLayout {
                        anchors.centerIn: parent
                        spacing: 6
                        Text {
                            text: " " + ((typeof stockItemsModel !== "undefined" && stockItemsModel) ? stockItemsModel.get_financial_year() : "Active Period")
                            color: "#1D4ED8"
                            font.pixelSize: 11
                            font.bold: true
                        }
                    }
                }

                T.Button {
                    id: filterBtn
                    implicitWidth: contentItem.implicitWidth + 24
                    implicitHeight: 32
                    height: 32
                    background: Rectangle { color: filterPopup.visible ? "#1D4ED8" : "#2563EB"; radius: 6 }
                    contentItem: RowLayout {
                        spacing: 6
                        Text { text: "Filter Dates"; color: "#FFF"; font.bold: true; font.pixelSize: 12 }
                        KbdBadge { text: "Alt+F"; badgeColor: "#1E3A8A"; textColor: "#93C5FD"; borderColor: "#2563EB" }
                    }
                    onClicked: filterPopup.open()
                }

                Item { Layout.fillWidth: true }

                T.Button {
                    implicitWidth: contentItem.implicitWidth + 24
                    implicitHeight: 32
                    height: 32
                    background: Rectangle { color: "#F1F5F9"; radius: 6; border.color: "#CBD5E1" }
                    contentItem: Text { text: "Print PDF"; color: "#475569"; font.bold: true; font.pixelSize: 12; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                }
            }
        }

        // 3. MAIN 2-COLUMN SCROLLABLE GRID SECTION (CREDIT ON LEFT, DEBIT ON RIGHT)
        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 12

            // ==================== LEFT COLUMN: CREDIT SIDE (JAMA / Cr) ====================
            Rectangle {
                Layout.fillWidth: true
                Layout.fillHeight: true
                color: "#FFFFFF"
                border.color: crListView.activeFocus ? "#16A34A" : "#CBD5E1"
                border.width: crListView.activeFocus ? 2 : 1
                radius: 8

                MouseArea {
                    anchors.fill: parent
                    onClicked: {
                        crListView.forceActiveFocus()
                        if (ledgerStatementCtrl && ledgerStatementCtrl.crModel.count > 0 && crListView.currentIndex < 0) crListView.currentIndex = 0
                    }
                }

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 10
                    spacing: 8

                    // Side Header
                    Rectangle {
                        Layout.fillWidth: true
                        height: 32
                        color: "#DCFCE7"
                        radius: 6

                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: 10
                            anchors.rightMargin: 10
                            Text { text: "CREDIT SIDE (JAMA / Cr) - " + root.currentPartyName; color: "#15803D"; font.pixelSize: 12; font.bold: true; elide: Text.ElideRight; Layout.fillWidth: true }
                            Text { text: "Takes / Payables"; color: "#16A34A"; font.pixelSize: 11; font.bold: true }
                        }
                    }

                    // Table Header Grid
                    Rectangle {
                        Layout.fillWidth: true
                        height: 28
                        color: "#F1F5F9"
                        radius: 4

                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: 6
                            anchors.rightMargin: 6
                            spacing: 6

                            Text { text: "Sel"; color: "#475569"; font.pixelSize: 11; font.bold: true; width: 28; horizontalAlignment: Text.AlignHCenter }
                            Text { text: "Date"; color: "#475569"; font.pixelSize: 11; font.bold: true; width: 85 }
                            Text { text: "Ref No"; color: "#475569"; font.pixelSize: 11; font.bold: true; width: 75 }
                            Text { text: "Particulars"; color: "#475569"; font.pixelSize: 11; font.bold: true; Layout.fillWidth: true }
                            Text { text: "Amount (₹)"; color: "#475569"; font.pixelSize: 11; font.bold: true; horizontalAlignment: Text.AlignRight; width: 95 }
                        }
                    }

                    // Credit Entries Scrollable ListView
                    ListView {
                        id: crListView
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        model: (typeof ledgerStatementCtrl !== "undefined" && ledgerStatementCtrl) ? ledgerStatementCtrl.crModel : null
                        clip: true
                        spacing: 2
                        boundsBehavior: Flickable.StopAtBounds
                        focus: false
                        activeFocusOnTab: true
                        currentIndex: -1

                        Keys.onUpPressed: function(event) {
                            event.accepted = true
                            if (crListView.currentIndex > 0) {
                                crListView.currentIndex--
                            } else if (crListView.currentIndex === -1 && count > 0) {
                                crListView.currentIndex = 0
                            }
                            crListView.positionViewAtIndex(crListView.currentIndex, ListView.Contain)
                        }
                        Keys.onDownPressed: function(event) {
                            event.accepted = true
                            if (crListView.currentIndex < 0 && count > 0) {
                                crListView.currentIndex = 0
                            } else if (crListView.currentIndex < count - 1) {
                                crListView.currentIndex++
                            }
                            crListView.positionViewAtIndex(crListView.currentIndex, ListView.Contain)
                        }
                        Keys.onSpacePressed: function(event) {
                            event.accepted = true
                            if (crListView.currentIndex >= 0 && crListView.currentIndex < count) {
                                ledgerStatementCtrl.crModel.toggleSelection(crListView.currentIndex)
                            }
                        }
                        Keys.onRightPressed: function(event) {
                            event.accepted = true
                            drListView.forceActiveFocus()
                            if (drListView.count > 0) {
                                if (drListView.currentIndex < 0 || drListView.currentIndex >= drListView.count) {
                                    drListView.currentIndex = 0
                                }
                            }
                        }
                        Keys.onReturnPressed: function(event) {
                            event.accepted = true
                            if (crListView.currentIndex >= 0 && crListView.currentIndex < count) {
                                root.openVoucherEntry(ledgerStatementCtrl.crModel.get(crListView.currentIndex))
                            }
                        }
                        Keys.onEnterPressed: function(event) {
                            event.accepted = true
                            if (crListView.currentIndex >= 0 && crListView.currentIndex < count) {
                                root.openVoucherEntry(ledgerStatementCtrl.crModel.get(crListView.currentIndex))
                            }
                        }

                        delegate: Rectangle {
                            width: crListView.width
                            height: 36
                            color: (crListView.activeFocus && crListView.currentIndex === index) ? "#DCFCE7" : (index % 2 === 0 ? "#FFFFFF" : "#F8FAFC")
                            border.color: (crListView.activeFocus && crListView.currentIndex === index) ? "#16A34A" : "#E2E8F0"
                            border.width: (crListView.activeFocus && crListView.currentIndex === index) ? 2 : 1
                            radius: 4

                            MouseArea {
                                anchors.fill: parent
                                onClicked: {
                                    crListView.forceActiveFocus()
                                    crListView.currentIndex = index
                                }
                                onDoubleClicked: {
                                    crListView.forceActiveFocus()
                                    crListView.currentIndex = index
                                    root.openVoucherEntry(ledgerStatementCtrl.crModel.get(index))
                                }
                            }

                            RowLayout {
                                anchors.fill: parent
                                anchors.leftMargin: 6
                                anchors.rightMargin: 6
                                spacing: 6

                                CustomCheckBox {
                                    boxSize: 20
                                    checkedColor: "#16A34A"
                                    checked: model.isSelected
                                    onToggled: {
                                        crListView.forceActiveFocus()
                                        crListView.currentIndex = index
                                        ledgerStatementCtrl.crModel.toggleSelection(index)
                                    }
                                }

                                Text { text: model.vDate; color: "#334155"; font.pixelSize: 11; font.family: "Segoe UI, Consolas, Menlo, sans-serif"; width: 85 }
                                Text { text: model.refNo; color: "#16A34A"; font.pixelSize: 11; font.bold: true; font.family: "Segoe UI, Consolas, Menlo, sans-serif"; width: 75 }
                                Text { text: model.particulars; color: "#0F172A"; font.pixelSize: 11; Layout.fillWidth: true; elide: Text.ElideRight }
                                Text { text: model.amountFmt; color: "#15803D"; font.pixelSize: 12; font.bold: true; horizontalAlignment: Text.AlignRight; width: 95 }
                            }
                        }
                    }

                    Rectangle { Layout.fillWidth: true; height: 1; color: "#CBD5E1" }

                    // Credit Subtotals Bar
                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 4

                        RowLayout {
                            Layout.fillWidth: true
                            Text { text: "Total Cr Entries:"; color: "#475569"; font.pixelSize: 11; font.bold: true }
                            Item { Layout.fillWidth: true }
                            Text {
                                text: (typeof ledgerStatementCtrl !== "undefined" && ledgerStatementCtrl) ? ledgerStatementCtrl.crTotalFmt : "₹0.00"
                                color: "#0F172A"
                                font.pixelSize: 13
                                font.bold: true
                            }
                        }

                        Rectangle {
                            Layout.fillWidth: true
                            height: 28
                            color: (typeof ledgerStatementCtrl !== "undefined" && ledgerStatementCtrl && ledgerStatementCtrl.crSelectedTotal > 0) ? "#DCFCE7" : "#F8FAFC"
                            radius: 4
                            border.color: (typeof ledgerStatementCtrl !== "undefined" && ledgerStatementCtrl && ledgerStatementCtrl.crSelectedTotal > 0) ? "#86EFAC" : "#E2E8F0"

                            RowLayout {
                                anchors.fill: parent
                                anchors.leftMargin: 8
                                anchors.rightMargin: 8
                                Text { text: "Checked Cr Total:"; color: "#166534"; font.pixelSize: 11; font.bold: true }
                                Item { Layout.fillWidth: true }
                                Text {
                                    text: (typeof ledgerStatementCtrl !== "undefined" && ledgerStatementCtrl) ? ledgerStatementCtrl.crSelectedTotalFmt : "₹0.00"
                                    color: "#15803D"
                                    font.pixelSize: 12
                                    font.bold: true
                                }
                            }
                        }
                    }
                }
            }

            // ==================== RIGHT COLUMN: DEBIT SIDE (NAAME / Dr) ====================
            Rectangle {
                Layout.fillWidth: true
                Layout.fillHeight: true
                color: "#FFFFFF"
                border.color: drListView.activeFocus ? "#2563EB" : "#CBD5E1"
                border.width: drListView.activeFocus ? 2 : 1
                radius: 8

                MouseArea {
                    anchors.fill: parent
                    onClicked: {
                        drListView.forceActiveFocus()
                        if (ledgerStatementCtrl && ledgerStatementCtrl.drModel.count > 0 && drListView.currentIndex < 0) drListView.currentIndex = 0
                    }
                }

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 10
                    spacing: 8

                    // Side Header
                    Rectangle {
                        Layout.fillWidth: true
                        height: 32
                        color: "#EFF6FF"
                        radius: 6

                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: 10
                            anchors.rightMargin: 10
                            Text { text: "DEBIT SIDE (NAAME / Dr) - " + root.currentPartyName; color: "#1D4ED8"; font.pixelSize: 12; font.bold: true; elide: Text.ElideRight; Layout.fillWidth: true }
                            Text { text: "Gives / Receivables"; color: "#3B82F6"; font.pixelSize: 11; font.bold: true }
                        }
                    }

                    // Table Header Grid
                    Rectangle {
                        Layout.fillWidth: true
                        height: 28
                        color: "#F1F5F9"
                        radius: 4

                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: 6
                            anchors.rightMargin: 6
                            spacing: 6

                            Text { text: "Sel"; color: "#475569"; font.pixelSize: 11; font.bold: true; width: 28; horizontalAlignment: Text.AlignHCenter }
                            Text { text: "Date"; color: "#475569"; font.pixelSize: 11; font.bold: true; width: 85 }
                            Text { text: "Ref No"; color: "#475569"; font.pixelSize: 11; font.bold: true; width: 75 }
                            Text { text: "Particulars"; color: "#475569"; font.pixelSize: 11; font.bold: true; Layout.fillWidth: true }
                            Text { text: "Amount (₹)"; color: "#475569"; font.pixelSize: 11; font.bold: true; horizontalAlignment: Text.AlignRight; width: 95 }
                        }
                    }

                    // Debit Entries Scrollable ListView
                    ListView {
                        id: drListView
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        model: (typeof ledgerStatementCtrl !== "undefined" && ledgerStatementCtrl) ? ledgerStatementCtrl.drModel : null
                        clip: true
                        spacing: 2
                        boundsBehavior: Flickable.StopAtBounds
                        focus: false
                        activeFocusOnTab: true
                        currentIndex: -1

                        Keys.onUpPressed: function(event) {
                            event.accepted = true
                            if (drListView.currentIndex > 0) {
                                drListView.currentIndex--
                            } else if (drListView.currentIndex === -1 && count > 0) {
                                drListView.currentIndex = 0
                            }
                            drListView.positionViewAtIndex(drListView.currentIndex, ListView.Contain)
                        }
                        Keys.onDownPressed: function(event) {
                            event.accepted = true
                            if (drListView.currentIndex < 0 && count > 0) {
                                drListView.currentIndex = 0
                            } else if (drListView.currentIndex < count - 1) {
                                drListView.currentIndex++
                            }
                            drListView.positionViewAtIndex(drListView.currentIndex, ListView.Contain)
                        }
                        Keys.onSpacePressed: function(event) {
                            event.accepted = true
                            if (drListView.currentIndex >= 0 && drListView.currentIndex < count) {
                                ledgerStatementCtrl.drModel.toggleSelection(drListView.currentIndex)
                            }
                        }
                        Keys.onLeftPressed: function(event) {
                            event.accepted = true
                            crListView.forceActiveFocus()
                            if (crListView.count > 0) {
                                if (crListView.currentIndex < 0 || crListView.currentIndex >= crListView.count) {
                                    crListView.currentIndex = 0
                                }
                            }
                        }
                        Keys.onReturnPressed: function(event) {
                            event.accepted = true
                            if (drListView.currentIndex >= 0 && drListView.currentIndex < count) {
                                root.openVoucherEntry(ledgerStatementCtrl.drModel.get(drListView.currentIndex))
                            }
                        }
                        Keys.onEnterPressed: function(event) {
                            event.accepted = true
                            if (drListView.currentIndex >= 0 && drListView.currentIndex < count) {
                                root.openVoucherEntry(ledgerStatementCtrl.drModel.get(drListView.currentIndex))
                            }
                        }

                        delegate: Rectangle {
                            width: drListView.width
                            height: 36
                            color: (drListView.activeFocus && drListView.currentIndex === index) ? "#DBEAFE" : (index % 2 === 0 ? "#FFFFFF" : "#F8FAFC")
                            border.color: (drListView.activeFocus && drListView.currentIndex === index) ? "#2563EB" : "#E2E8F0"
                            border.width: (drListView.activeFocus && drListView.currentIndex === index) ? 2 : 1
                            radius: 4

                            MouseArea {
                                anchors.fill: parent
                                onClicked: {
                                    drListView.forceActiveFocus()
                                    drListView.currentIndex = index
                                }
                                onDoubleClicked: {
                                    drListView.forceActiveFocus()
                                    drListView.currentIndex = index
                                    root.openVoucherEntry(ledgerStatementCtrl.drModel.get(index))
                                }
                            }

                            RowLayout {
                                anchors.fill: parent
                                anchors.leftMargin: 6
                                anchors.rightMargin: 6
                                spacing: 6

                                // CUSTOM CLEAN WHITE CHECKBOX
                                CustomCheckBox {
                                    boxSize: 20
                                    checkedColor: "#2563EB"
                                    checked: model.isSelected
                                    onToggled: {
                                        drListView.forceActiveFocus()
                                        drListView.currentIndex = index
                                        ledgerStatementCtrl.drModel.toggleSelection(index)
                                    }
                                }

                                Text { text: model.vDate; color: "#334155"; font.pixelSize: 11; font.family: "Segoe UI, Consolas, Menlo, sans-serif"; width: 85 }
                                Text { text: model.refNo; color: "#2563EB"; font.pixelSize: 11; font.bold: true; font.family: "Segoe UI, Consolas, Menlo, sans-serif"; width: 75 }
                                Text { text: model.particulars; color: "#0F172A"; font.pixelSize: 11; Layout.fillWidth: true; elide: Text.ElideRight }
                                Text { text: model.amountFmt; color: "#1D4ED8"; font.pixelSize: 12; font.bold: true; horizontalAlignment: Text.AlignRight; width: 95 }
                            }
                        }
                    }

                    Rectangle { Layout.fillWidth: true; height: 1; color: "#CBD5E1" }

                    // Debit Subtotals Bar
                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 4

                        RowLayout {
                            Layout.fillWidth: true
                            Text { text: "Total Dr Entries:"; color: "#475569"; font.pixelSize: 11; font.bold: true }
                            Item { Layout.fillWidth: true }
                            Text {
                                text: (typeof ledgerStatementCtrl !== "undefined" && ledgerStatementCtrl) ? ledgerStatementCtrl.drTotalFmt : "₹0.00"
                                color: "#0F172A"
                                font.pixelSize: 13
                                font.bold: true
                            }
                        }

                        Rectangle {
                            Layout.fillWidth: true
                            height: 28
                            color: (typeof ledgerStatementCtrl !== "undefined" && ledgerStatementCtrl && ledgerStatementCtrl.drSelectedTotal > 0) ? "#DBEAFE" : "#F8FAFC"
                            radius: 4
                            border.color: (typeof ledgerStatementCtrl !== "undefined" && ledgerStatementCtrl && ledgerStatementCtrl.drSelectedTotal > 0) ? "#93C5FD" : "#E2E8F0"

                            RowLayout {
                                anchors.fill: parent
                                anchors.leftMargin: 8
                                anchors.rightMargin: 8
                                Text { text: "Checked Dr Total:"; color: "#1E40AF"; font.pixelSize: 11; font.bold: true }
                                Item { Layout.fillWidth: true }
                                Text {
                                    text: (typeof ledgerStatementCtrl !== "undefined" && ledgerStatementCtrl) ? ledgerStatementCtrl.drSelectedTotalFmt : "₹0.00"
                                    color: "#1D4ED8"
                                    font.pixelSize: 12
                                    font.bold: true
                                }
                            }
                        }
                    }
                }
            }
        }

        // 4. PINNED NET TOTAL & STATEMENT SUMMARY BAR AT BOTTOM
        Rectangle {
            Layout.fillWidth: true
            height: 50
            color: "#0F172A"
            radius: 8

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 16
                anchors.rightMargin: 16
                spacing: 20

                // Checked Reconciliation Summary
                ColumnLayout {
                    spacing: 2
                    Text { text: "CHECKED ENTRIES RECONCILIATION"; color: "#94A3B8"; font.pixelSize: 10; font.bold: true; font.letterSpacing: 0.8 }
                    Text {
                        text: "Selected Cr: " + ((typeof ledgerStatementCtrl !== "undefined" && ledgerStatementCtrl) ? ledgerStatementCtrl.crSelectedTotalFmt : "₹0.00") + "  |  Selected Dr: " + ((typeof ledgerStatementCtrl !== "undefined" && ledgerStatementCtrl) ? ledgerStatementCtrl.drSelectedTotalFmt : "₹0.00")
                        color: "#E2E8F0"
                        font.pixelSize: 13
                        font.bold: true
                    }
                }

                Item { Layout.fillWidth: true }

                // Final Net Balance and Dr/Cr Badge
                RowLayout {
                    spacing: 12
                    ColumnLayout {
                        spacing: 0
                        Text { text: "FINAL STATEMENT NET BALANCE"; color: "#94A3B8"; font.pixelSize: 10; font.bold: true; font.letterSpacing: 0.8; Layout.alignment: Qt.AlignRight }
                        Text {
                            text: (typeof ledgerStatementCtrl !== "undefined" && ledgerStatementCtrl) ? ledgerStatementCtrl.netBalanceFmt : "₹0.00"
                            color: "#38BDF8"
                            font.pixelSize: 20
                            font.bold: true
                        }
                    }

                    Rectangle {
                        height: 26
                        implicitWidth: balTypeTxt.implicitWidth + 14
                        radius: 6
                        color: ((typeof ledgerStatementCtrl !== "undefined" && ledgerStatementCtrl ? ledgerStatementCtrl.netBalanceType : "").indexOf("Nil") !== -1) ? "#16A34A" : "#0284C7"
                        Text {
                            id: balTypeTxt
                            anchors.centerIn: parent
                            text: (typeof ledgerStatementCtrl !== "undefined" && ledgerStatementCtrl) ? ledgerStatementCtrl.netBalanceType : "Nil Balance"
                            color: "#FFFFFF"
                            font.pixelSize: 11
                            font.bold: true
                        }
                    }
                }
            }
        }
    }

    Shortcut {
        sequence: "Alt+S"
        context: Qt.WindowShortcut
        onActivated: {
            partySearchInput.forceActiveFocus()
            partySearchInput.selectAll()
        }
    }

    Shortcut {
        sequence: "Alt+L"
        context: Qt.WindowShortcut
        onActivated: {
            partySearchInput.forceActiveFocus()
            partySearchInput.selectAll()
        }
    }

    Shortcut {
        sequence: "Alt+F"
        context: Qt.WindowShortcut
        onActivated: filterPopup.open()
    }

    Shortcut {
        sequence: "Ctrl+P"
        context: Qt.WindowShortcut
        onActivated: root.printStatement()
    }

    Shortcut {
        sequence: "Alt+P"
        context: Qt.WindowShortcut
        onActivated: root.exportPdf()
    }

    T.Popup {
        id: filterPopup
        width: 380
        height: 280
        modal: true
        dim: true
        x: Math.round((parent.width - width) / 2)
        y: Math.round((parent.height - height) / 2)
        focus: true
        closePolicy: T.Popup.CloseOnPressOutside | T.Popup.CloseOnEscape

        background: Rectangle {
            color: "#FFFFFF"
            border.color: "#2563EB"
            border.width: 2.5
            radius: 12
        }

        FocusScope {
            id: popScope
            anchors.fill: parent
            focus: true

            Keys.onReturnPressed: function(event) { event.accepted = true; popScope.applyFilter() }
            Keys.onEnterPressed: function(event) { event.accepted = true; popScope.applyFilter() }
            Keys.onEscapePressed: function(event) { event.accepted = true; filterPopup.close() }

            function applyFilter() {
                filterPopup.close()
                root.applyDateFilterAndSort()
                crListView.forceActiveFocus()
            }

            function clearFilter() {
                fromDateInput.text = "01-04-2026"
                toDateInput.text = "31-03-2027"
                filterPopup.close()
                root.applyDateFilterAndSort()
                crListView.forceActiveFocus()
            }

            ColumnLayout {
                id: filterCol
                anchors.fill: parent
                anchors.margins: 18
                spacing: 14

                RowLayout {
                    Layout.fillWidth: true
                    Text { text: "Filter Statement Date Range"; color: "#0F172A"; font.pixelSize: 15; font.bold: true }
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
                    spacing: 12

                    CustomInput {
                        id: fromDateInput
                        label: "From Date (DD-MM-YYYY)"
                        text: "01-04-2026"
                        focusInput: true
                        Layout.fillWidth: true
                        onReturnPressed: toDateInput.focusInput = true
                    }

                    CustomInput {
                        id: toDateInput
                        label: "To Date (DD-MM-YYYY)"
                        text: "31-03-2027"
                        Layout.fillWidth: true
                        onReturnPressed: popScope.applyFilter()
                    }
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 10

                    T.Button {
                        implicitWidth: contentItem.implicitWidth + 24
                        implicitHeight: 32
                        background: Rectangle { color: "#F1F5F9"; radius: 6; border.color: "#CBD5E1" }
                        contentItem: Text { text: "Clear (Full Year)"; color: "#475569"; font.bold: true; font.pixelSize: 12; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                        onClicked: popScope.clearFilter()
                    }

                    Item { Layout.fillWidth: true }

                    T.Button {
                        implicitWidth: contentItem.implicitWidth + 24
                        implicitHeight: 32
                        background: Rectangle { color: "#2563EB"; radius: 6 }
                        contentItem: Text { text: "Apply Filter (Enter)"; color: "#FFFFFF"; font.bold: true; font.pixelSize: 12; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                        onClicked: popScope.applyFilter()
                    }
                }
            }
        }
    }
}
