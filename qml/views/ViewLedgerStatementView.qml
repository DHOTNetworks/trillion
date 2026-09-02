import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../components"

Rectangle {
    id: root
    color: "#F4F6F9"

    signal cancelRequested()

    property string currentPartyName: ""
    property real drTotalAll: 0.0
    property real crTotalAll: 0.0
    property real drSelectedTotal: 0.0
    property real crSelectedTotal: 0.0
    property real netBalance: 0.0
    property string netBalanceType: "Dr (Debit Balance)"

    Component.onCompleted: {
        syncDateInputsWithActivePeriod()
        loadPartyStatement("")
        Qt.callLater(function() {
            partySearch.focusAndOpen()
        })
    }

    onVisibleChanged: {
        if (visible) {
            syncDateInputsWithActivePeriod()
            loadPartyStatement(currentPartyName)
            Qt.callLater(function() {
                partySearch.focusAndOpen()
            })
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
        currentPartyName = pName
        drMasterModel.clear()
        crMasterModel.clear()
        drSelectedTotal = 0.0
        crSelectedTotal = 0.0

        if (typeof partiesModel !== "undefined" && partiesModel && partiesModel.get_party_statement) {
            var res = partiesModel.get_party_statement(pName)
            if (res && res.dr_items) {
                for (var d = 0; d < res.dr_items.length; d++) {
                    drMasterModel.append(res.dr_items[d])
                }
            }
            if (res && res.cr_items) {
                for (var c = 0; c < res.cr_items.length; c++) {
                    crMasterModel.append(res.cr_items[c])
                }
            }
        }

        applyDateFilterAndSort()
    }

    function applyDateFilterAndSort() {
        var fIso = toIso(fromDateInput.text)
        var tIso = toIso(toDateInput.text)

        drListModel.clear()
        crListModel.clear()

        var drTemp = []
        for (var i = 0; i < drMasterModel.count; i++) {
            var dItem = drMasterModel.get(i)
            var dIso = toIso(dItem.vDate)
            if ((!fIso || dIso >= fIso) && (!tIso || dIso <= tIso)) {
                drTemp.push(dItem)
            }
        }
        // Sort chronologically by date ISO
        drTemp.sort(function(a, b) { return toIso(a.vDate).localeCompare(toIso(b.vDate)) })
        for (var k = 0; k < drTemp.length; k++) {
            drListModel.append(drTemp[k])
        }

        var crTemp = []
        for (var j = 0; j < crMasterModel.count; j++) {
            var cItem = crMasterModel.get(j)
            var cIso = toIso(cItem.vDate)
            if ((!fIso || cIso >= fIso) && (!tIso || cIso <= tIso)) {
                crTemp.push(cItem)
            }
        }
        // Sort chronologically by date ISO
        crTemp.sort(function(a, b) { return toIso(a.vDate).localeCompare(toIso(b.vDate)) })
        for (var m = 0; m < crTemp.length; m++) {
            crListModel.append(crTemp[m])
        }

        recalcTotals()
    }

    function recalcTotals() {
        var drSum = 0.0
        var drSel = 0.0
        for (var i = 0; i < drListModel.count; i++) {
            var item = drListModel.get(i)
            drSum += item.amount
            if (item.isSelected) drSel += item.amount
        }
        drTotalAll = drSum
        drSelectedTotal = drSel

        var crSum = 0.0
        var crSel = 0.0
        for (var j = 0; j < crListModel.count; j++) {
            var cItem = crListModel.get(j)
            crSum += cItem.amount
            if (cItem.isSelected) crSel += cItem.amount
        }
        crTotalAll = crSum
        crSelectedTotal = crSel

        netBalance = Math.abs(drTotalAll - crTotalAll)
        if (drTotalAll > crTotalAll) {
            netBalanceType = "Dr (Debit Balance)"
        } else if (crTotalAll > drTotalAll) {
            netBalanceType = "Cr (Credit Balance)"
        } else {
            netBalanceType = "Nil Balance"
        }
    }

    ListModel { id: drMasterModel }
    ListModel { id: crMasterModel }
    ListModel { id: drListModel }
    ListModel { id: crListModel }

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
                    text: "👁️ Traditional Bahi Khata Ledger Statement (2-Column Dr / Cr)"
                    color: "#0F172A"
                    font.pixelSize: 18
                    font.bold: true
                }
                Text {
                    text: "Side-by-side Jama (Debit) and Nama (Credit) accounting ledger with interactive reconciliation."
                    color: "#64748B"
                    font.pixelSize: 11
                }
            }

            Item { Layout.fillWidth: true }

            Button {
                id: backBtn
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
            height: 48
            color: "#FFFFFF"
            border.color: "#2563EB"
            border.width: 2
            radius: 8
            z: 100

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 12
                anchors.rightMargin: 12
                spacing: 12

                Text { text: "Search Party:"; color: "#2563EB"; font.pixelSize: 12; font.bold: true }

                CustomWhiteCombo {
                    id: partySearch
                    Layout.preferredWidth: 320
                    model: (typeof partiesModel !== "undefined" && partiesModel) ? partiesModel.get_parties_list() : []
                    onCurrentTextChanged: root.loadPartyStatement(currentText)
                    onReturnPressed: {
                        crListView.forceActiveFocus()
                        if (crListView.count > 0 && crListView.currentIndex < 0) crListView.currentIndex = 0
                    }
                }

                // Active Financial Year Badge Indicator
                Rectangle {
                    height: 28
                    Layout.preferredWidth: 180
                    color: "#EFF6FF"
                    border.color: "#93C5FD"
                    radius: 6

                    RowLayout {
                        anchors.centerIn: parent
                        spacing: 6
                        Text {
                            text: "📅 " + ((typeof stockItemsModel !== "undefined" && stockItemsModel) ? stockItemsModel.get_financial_year() : "Active Period")
                            color: "#1D4ED8"
                            font.pixelSize: 11
                            font.bold: true
                        }
                    }
                }

                Button {
                    id: filterBtn
                    height: 32
                    background: Rectangle { color: filterPopup.visible ? "#1D4ED8" : "#2563EB"; radius: 6 }
                    contentItem: RowLayout {
                        spacing: 6
                        Text { text: "🔍 Filter Dates"; color: "#FFF"; font.bold: true; font.pixelSize: 12 }
                        KbdBadge { text: "Alt+F"; badgeColor: "#1E3A8A"; textColor: "#93C5FD"; borderColor: "#2563EB" }
                    }
                    onClicked: filterPopup.open()
                }

                Item { Layout.fillWidth: true }

                Button {
                    height: 32
                    background: Rectangle { color: "#F1F5F9"; radius: 6; border.color: "#CBD5E1" }
                    contentItem: Text { text: "🖨️ Print PDF"; color: "#475569"; font.bold: true; font.pixelSize: 12 }
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
                            Text { text: "🟢 CREDIT SIDE (JAMA / Cr) - " + root.currentPartyName; color: "#15803D"; font.pixelSize: 12; font.bold: true; elide: Text.ElideRight; Layout.fillWidth: true }
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
                        model: crListModel
                        clip: true
                        spacing: 2
                        boundsBehavior: Flickable.StopAtBounds
                        focus: true
                        activeFocusOnTab: true
                        currentIndex: -1

                        Keys.onUpPressed: function(event) {
                            if (crListView.currentIndex > 0) {
                                event.accepted = true
                                crListView.currentIndex--
                                crListView.positionViewAtIndex(crListView.currentIndex, ListView.Contain)
                            }
                        }
                        Keys.onDownPressed: function(event) {
                            if (crListView.currentIndex < crListModel.count - 1) {
                                event.accepted = true
                                crListView.currentIndex++
                                crListView.positionViewAtIndex(crListView.currentIndex, ListView.Contain)
                            }
                        }
                        Keys.onSpacePressed: function(event) {
                            if (crListView.currentIndex >= 0 && crListView.currentIndex < crListModel.count) {
                                event.accepted = true
                                var cur = crListModel.get(crListView.currentIndex)
                                crListModel.setProperty(crListView.currentIndex, "isSelected", !cur.isSelected)
                                root.recalcTotals()
                            }
                        }
                        Keys.onRightPressed: function(event) {
                            event.accepted = true
                            drListView.forceActiveFocus()
                            if (drListView.currentIndex < 0 && drListModel.count > 0) drListView.currentIndex = 0
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
                            }

                            RowLayout {
                                anchors.fill: parent
                                anchors.leftMargin: 6
                                anchors.rightMargin: 6
                                spacing: 6

                                // CUSTOM CLEAN WHITE CHECKBOX
                                Rectangle {
                                    width: 20
                                    height: 20
                                    radius: 4
                                    color: model.isSelected ? "#16A34A" : "#FFFFFF"
                                    border.color: model.isSelected ? "#15803D" : "#CBD5E1"
                                    border.width: 1

                                    Text {
                                        anchors.centerIn: parent
                                        text: "✓"
                                        color: "#FFFFFF"
                                        font.pixelSize: 12
                                        font.bold: true
                                        visible: model.isSelected
                                    }

                                    MouseArea {
                                        anchors.fill: parent
                                        onClicked: {
                                            crListView.forceActiveFocus()
                                            crListView.currentIndex = index
                                            crListModel.setProperty(index, "isSelected", !model.isSelected)
                                            root.recalcTotals()
                                        }
                                    }
                                }

                                Text { text: model.vDate; color: "#334155"; font.pixelSize: 11; font.family: "Segoe UI, Consolas, Menlo, sans-serif"; width: 85 }
                                Text { text: model.refNo; color: "#16A34A"; font.pixelSize: 11; font.bold: true; font.family: "Segoe UI, Consolas, Menlo, sans-serif"; width: 75 }
                                Text { text: model.particulars; color: "#0F172A"; font.pixelSize: 11; Layout.fillWidth: true; elide: Text.ElideRight }
                                Text { text: (typeof dashboardCtrl !== "undefined" && dashboardCtrl) ? dashboardCtrl.format_inr(model.amount) : ("₹" + model.amount.toFixed(2)); color: "#15803D"; font.pixelSize: 12; font.bold: true; horizontalAlignment: Text.AlignRight; width: 95 }
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
                            Text { text: (typeof dashboardCtrl !== "undefined" && dashboardCtrl) ? dashboardCtrl.format_inr(root.crTotalAll) : ("₹" + root.crTotalAll.toFixed(2)); color: "#0F172A"; font.pixelSize: 13; font.bold: true }
                        }

                        Rectangle {
                            Layout.fillWidth: true
                            height: 28
                            color: root.crSelectedTotal > 0 ? "#DCFCE7" : "#F8FAFC"
                            radius: 4
                            border.color: root.crSelectedTotal > 0 ? "#86EFAC" : "#E2E8F0"

                            RowLayout {
                                anchors.fill: parent
                                anchors.leftMargin: 8
                                anchors.rightMargin: 8
                                Text { text: "☑ Checked Cr Total:"; color: "#166534"; font.pixelSize: 11; font.bold: true }
                                Item { Layout.fillWidth: true }
                                Text { text: (typeof dashboardCtrl !== "undefined" && dashboardCtrl) ? dashboardCtrl.format_inr(root.crSelectedTotal) : ("₹" + root.crSelectedTotal.toFixed(2)); color: "#15803D"; font.pixelSize: 12; font.bold: true }
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
                            Text { text: "🔴 DEBIT SIDE (NAAME / Dr) - " + root.currentPartyName; color: "#1D4ED8"; font.pixelSize: 12; font.bold: true; elide: Text.ElideRight; Layout.fillWidth: true }
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
                        model: drListModel
                        clip: true
                        spacing: 2
                        boundsBehavior: Flickable.StopAtBounds
                        focus: false
                        activeFocusOnTab: true
                        currentIndex: -1

                        Keys.onUpPressed: function(event) {
                            if (drListView.currentIndex > 0) {
                                event.accepted = true
                                drListView.currentIndex--
                                drListView.positionViewAtIndex(drListView.currentIndex, ListView.Contain)
                            }
                        }
                        Keys.onDownPressed: function(event) {
                            if (drListView.currentIndex < drListModel.count - 1) {
                                event.accepted = true
                                drListView.currentIndex++
                                drListView.positionViewAtIndex(drListView.currentIndex, ListView.Contain)
                            }
                        }
                        Keys.onSpacePressed: function(event) {
                            if (drListView.currentIndex >= 0 && drListView.currentIndex < drListModel.count) {
                                event.accepted = true
                                var cur = drListModel.get(drListView.currentIndex)
                                drListModel.setProperty(drListView.currentIndex, "isSelected", !cur.isSelected)
                                root.recalcTotals()
                            }
                        }
                        Keys.onLeftPressed: function(event) {
                            event.accepted = true
                            crListView.forceActiveFocus()
                            if (crListView.currentIndex < 0 && crListModel.count > 0) crListView.currentIndex = 0
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
                            }

                            RowLayout {
                                anchors.fill: parent
                                anchors.leftMargin: 6
                                anchors.rightMargin: 6
                                spacing: 6

                                // CUSTOM CLEAN WHITE CHECKBOX
                                Rectangle {
                                    width: 20
                                    height: 20
                                    radius: 4
                                    color: model.isSelected ? "#2563EB" : "#FFFFFF"
                                    border.color: model.isSelected ? "#1D4ED8" : "#CBD5E1"
                                    border.width: 1

                                    Text {
                                        anchors.centerIn: parent
                                        text: "✓"
                                        color: "#FFFFFF"
                                        font.pixelSize: 12
                                        font.bold: true
                                        visible: model.isSelected
                                    }

                                    MouseArea {
                                        anchors.fill: parent
                                        onClicked: {
                                            drListView.forceActiveFocus()
                                            drListView.currentIndex = index
                                            drListModel.setProperty(index, "isSelected", !model.isSelected)
                                            root.recalcTotals()
                                        }
                                    }
                                }

                                Text { text: model.vDate; color: "#334155"; font.pixelSize: 11; font.family: "Segoe UI, Consolas, Menlo, sans-serif"; width: 85 }
                                Text { text: model.refNo; color: "#2563EB"; font.pixelSize: 11; font.bold: true; font.family: "Segoe UI, Consolas, Menlo, sans-serif"; width: 75 }
                                Text { text: model.particulars; color: "#0F172A"; font.pixelSize: 11; Layout.fillWidth: true; elide: Text.ElideRight }
                                Text { text: (typeof dashboardCtrl !== "undefined" && dashboardCtrl) ? dashboardCtrl.format_inr(model.amount) : ("₹" + model.amount.toFixed(2)); color: "#1D4ED8"; font.pixelSize: 12; font.bold: true; horizontalAlignment: Text.AlignRight; width: 95 }
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
                            Text { text: (typeof dashboardCtrl !== "undefined" && dashboardCtrl) ? dashboardCtrl.format_inr(root.drTotalAll) : ("₹" + root.drTotalAll.toFixed(2)); color: "#0F172A"; font.pixelSize: 13; font.bold: true }
                        }

                        Rectangle {
                            Layout.fillWidth: true
                            height: 28
                            color: root.drSelectedTotal > 0 ? "#DBEAFE" : "#F8FAFC"
                            radius: 4
                            border.color: root.drSelectedTotal > 0 ? "#93C5FD" : "#E2E8F0"

                            RowLayout {
                                anchors.fill: parent
                                anchors.leftMargin: 8
                                anchors.rightMargin: 8
                                Text { text: "☑ Checked Dr Total:"; color: "#1E40AF"; font.pixelSize: 11; font.bold: true }
                                Item { Layout.fillWidth: true }
                                Text { text: (typeof dashboardCtrl !== "undefined" && dashboardCtrl) ? dashboardCtrl.format_inr(root.drSelectedTotal) : ("₹" + root.drSelectedTotal.toFixed(2)); color: "#1D4ED8"; font.pixelSize: 12; font.bold: true }
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
                        text: "Selected Cr: " + ((typeof dashboardCtrl !== "undefined" && dashboardCtrl) ? dashboardCtrl.format_inr(root.crSelectedTotal) : ("₹" + root.crSelectedTotal.toFixed(2))) + "  |  Selected Dr: " + ((typeof dashboardCtrl !== "undefined" && dashboardCtrl) ? dashboardCtrl.format_inr(root.drSelectedTotal) : ("₹" + root.drSelectedTotal.toFixed(2)))
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
                            text: (typeof dashboardCtrl !== "undefined" && dashboardCtrl) ? dashboardCtrl.format_inr(root.netBalance) : ("₹" + root.netBalance.toFixed(2))
                            color: "#38BDF8"
                            font.pixelSize: 20
                            font.bold: true
                        }
                    }

                    Rectangle {
                        height: 26
                        implicitWidth: balTypeTxt.implicitWidth + 14
                        radius: 6
                        color: root.netBalanceType.indexOf("Nil") !== -1 ? "#16A34A" : "#0284C7"
                        Text {
                            id: balTypeTxt
                            anchors.centerIn: parent
                            text: root.netBalanceType
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
        sequence: "Alt+F"
        context: Qt.WindowShortcut
        onActivated: filterPopup.open()
    }

    Popup {
        id: filterPopup
        width: 380
        implicitHeight: filterCol.implicitHeight + 36
        modal: true
        dim: true
        anchors.centerIn: parent
        focus: true
        closePolicy: Popup.CloseOnPressOutside | Popup.CloseOnEscape

        background: Rectangle {
            color: "#FFFFFF"
            border.color: "#2563EB"
            border.width: 2.5
            radius: 12
        }

        contentItem: FocusScope {
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
                    Text { text: "📅 Filter Statement Date Range"; color: "#0F172A"; font.pixelSize: 15; font.bold: true }
                    Item { Layout.fillWidth: true }
                    Button { flat: true; text: "✕"; onClicked: filterPopup.close() }
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

                    Button {
                        background: Rectangle { color: "#F1F5F9"; radius: 6; border.color: "#CBD5E1" }
                        contentItem: Text { text: "Clear (Full Year)"; color: "#475569"; font.bold: true; font.pixelSize: 12 }
                        onClicked: popScope.clearFilter()
                    }

                    Item { Layout.fillWidth: true }

                    Button {
                        background: Rectangle { color: "#2563EB"; radius: 6 }
                        contentItem: Text { text: "Apply Filter (Enter)"; color: "#FFFFFF"; font.bold: true; font.pixelSize: 12 }
                        onClicked: popScope.applyFilter()
                    }
                }
            }
        }
    }
}
