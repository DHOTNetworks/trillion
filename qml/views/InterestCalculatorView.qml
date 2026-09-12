import QtQuick
import QtQuick.Templates as T
import QtQuick.Layouts
import MahadevERP

FocusScope {
    id: root
    focus: true

    signal cancelRequested()
    signal interestVoucherSaved()

    property string currentPartyName: ""
    property string fromDateText: "01-04-2025"
    property string toDateText: "31-03-2026"
    property string fromIsoText: "2025-04-01"
    property string toIsoText: "2026-03-31"

    property real globalCrRate: 12.0
    property real globalDrRate: 12.0
    property int globalCrGraceDays: 0
    property int globalDrGraceDays: 0

    property bool is360DaysMethod: true
    property int yearDivisor: 365
    property bool includeOpBal: true

    property real crTotalAmount: 0.0
    property real crTotalInterest: 0.0
    property real drTotalAmount: 0.0
    property real drTotalInterest: 0.0

    property real netInterest: 0.0
    property string netInterestType: "Dr (Receivable)"
    property real ledgerBalance: 0.0
    property string ledgerBalanceType: "Dr"
    property real finalNetBalance: 0.0
    property string finalNetBalanceType: "Dr"

    property string statusMessage: ""
    property bool isError: false

    GenericListModel { id: crItemsModel }
    GenericListModel { id: drItemsModel }

    Component.onCompleted: {
        initializeDates()
        if (partySearch.currentText) {
            loadInterestStatement(partySearch.currentText)
        }
        Qt.callLater(function() {
            partySearch.forceActiveFocus()
        })
    }

    function initializeDates() {
        if (typeof stockItemsModel !== "undefined" && stockItemsModel) {
            var fy = stockItemsModel.get_financial_year()
            if (fy.indexOf("2025-26") !== -1 || fy.indexOf("25-26") !== -1) {
                fromDateText = "01-04-2025"
                toDateText = "31-03-2026"
                fromIsoText = "2025-04-01"
                toIsoText = "2026-03-31"
            } else if (fy.indexOf("2024-25") !== -1 || fy.indexOf("24-25") !== -1) {
                fromDateText = "01-04-2024"
                toDateText = "31-03-2025"
                fromIsoText = "2024-04-01"
                toIsoText = "2025-03-31"
            } else if (fy.indexOf("2026-27") !== -1 || fy.indexOf("26-27") !== -1) {
                fromDateText = "01-04-2026"
                toDateText = "31-03-2027"
                fromIsoText = "2026-04-01"
                toIsoText = "2027-03-31"
            }
        }
    }

    function toIso(dStr) {
        if (!dStr) return ""
        if (typeof dateService !== "undefined" && dateService) {
            var res = dateService.toIso(dStr)
            if (res) return res
        }
        var s = dStr.trim()
        if (s.indexOf("-") !== -1) {
            var parts = s.split("-")
            if (parts.length === 3) {
                if (parts[0].length === 4) return parts[0] + "-" + parts[1] + "-" + parts[2]
                if (parts[2].length === 4) return parts[2] + "-" + parts[1] + "-" + parts[0]
            }
        }
        return s
    }

    function calcBahiKhataDays360(d1Iso, d2Iso) {
        if (!d1Iso || !d2Iso) return 0
        var p1 = d1Iso.split("-")
        var p2 = d2Iso.split("-")
        if (p1.length !== 3 || p2.length !== 3) return 0
        var y1 = parseInt(p1[0]), m1 = parseInt(p1[1]), d1 = parseInt(p1[2])
        var y2 = parseInt(p2[0]), m2 = parseInt(p2[1]), d2 = parseInt(p2[2])
        var days = (y2 - y1) * 360 + (m2 - m1) * 30 + (30 - d1)
        return Math.max(0, days)
    }

    function calcCalendarDays365(d1Iso, d2Iso) {
        if (!d1Iso || !d2Iso) return 0
        var dt1 = new Date(d1Iso)
        var dt2 = new Date(d2Iso)
        var diff = Math.round((dt2 - dt1) / (1000 * 60 * 60 * 24))
        return Math.max(0, diff)
    }

    function loadInterestStatement(partyName) {
        currentPartyName = partyName.trim()
        if (!currentPartyName) return

        if (typeof interestModel !== "undefined" && interestModel) {
            var data = interestModel.get_interest_data(
                currentPartyName,
                fromIsoText,
                toIsoText,
                globalCrRate,
                globalDrRate,
                is360DaysMethod,
                yearDivisor,
                includeOpBal
            )

            crItemsModel.clear()
            drItemsModel.clear()

            var crs = data.cr_items || []
            for (var i = 0; i < crs.length; i++) {
                var it = crs[i]
                it.dueDays = (typeof it.dueDays !== "undefined" && it.dueDays > 0) ? it.dueDays : root.globalCrGraceDays
                it.rate = it.rate || globalCrRate
                crItemsModel.append(it)
            }

            var drs = data.dr_items || []
            for (var j = 0; j < drs.length; j++) {
                var itm = drs[j]
                itm.dueDays = (typeof itm.dueDays !== "undefined" && itm.dueDays > 0) ? itm.dueDays : root.globalDrGraceDays
                itm.rate = itm.rate || globalDrRate
                drItemsModel.append(itm)
            }

            recalcAllTotals()
        }
    }

    function recalcAllTotals() {
        var divisor = yearDivisor > 0 ? yearDivisor : 365.0
        var crAmtSum = 0.0, crIntSum = 0.0

        for (var i = 0; i < crItemsModel.count; i++) {
            var ci = crItemsModel.get(i)
            var baseD = is360DaysMethod ? calcBahiKhataDays360(ci.vIso, toIsoText) : calcCalendarDays365(ci.vIso, toIsoText)
            var dueD = parseInt(ci.dueDays) || 0
            var effD = Math.max(0, baseD - dueD)
            var r = parseFloat(ci.rate) || globalCrRate
            var rowInt = (typeof mathService !== "undefined" && mathService) ? mathService.round2(ci.amount * (r / 100.0) * effD / divisor) : (Math.round(ci.amount * (r / 100.0) * effD / divisor * 100.0) / 100.0)

            crItemsModel.setProperty(i, "days", effD)
            crItemsModel.setProperty(i, "interestAmt", rowInt)
            crAmtSum += ci.amount
            crIntSum += rowInt
        }

        var drAmtSum = 0.0, drIntSum = 0.0
        for (var j = 0; j < drItemsModel.count; j++) {
            var di = drItemsModel.get(j)
            var baseDr = is360DaysMethod ? calcBahiKhataDays360(di.vIso, toIsoText) : calcCalendarDays365(di.vIso, toIsoText)
            var dueDr = parseInt(di.dueDays) || 0
            var effDr = Math.max(0, baseDr - dueDr)
            var drR = parseFloat(di.rate) || globalDrRate
            var rowDrInt = (typeof mathService !== "undefined" && mathService) ? mathService.round2(di.amount * (drR / 100.0) * effDr / divisor) : (Math.round(di.amount * (drR / 100.0) * effDr / divisor * 100.0) / 100.0)

            drItemsModel.setProperty(j, "days", effDr)
            drItemsModel.setProperty(j, "interestAmt", rowDrInt)
            drAmtSum += di.amount
            drIntSum += rowDrInt
        }

        crTotalAmount = (typeof mathService !== "undefined" && mathService) ? mathService.round2(crAmtSum) : crAmtSum
        crTotalInterest = (typeof mathService !== "undefined" && mathService) ? mathService.round2(crIntSum) : crIntSum
        drTotalAmount = (typeof mathService !== "undefined" && mathService) ? mathService.round2(drAmtSum) : drAmtSum
        drTotalInterest = (typeof mathService !== "undefined" && mathService) ? mathService.round2(drIntSum) : drIntSum

        var netI = (typeof mathService !== "undefined" && mathService) ? mathService.round2(drTotalInterest - crTotalInterest) : (drTotalInterest - crTotalInterest)
        netInterest = Math.abs(netI)
        netInterestType = netI >= 0 ? "Dr (Receivable)" : "Cr (Payable)"

        var ledgB = (typeof mathService !== "undefined" && mathService) ? mathService.round2(drTotalAmount - crTotalAmount) : (drTotalAmount - crTotalAmount)
        ledgerBalance = Math.abs(ledgB)
        ledgerBalanceType = ledgB >= 0 ? "Dr" : "Cr"

        var finB = (typeof mathService !== "undefined" && mathService) ? mathService.round2(ledgB + netI) : (ledgB + netI)
        finalNetBalance = Math.abs(finB)
        finalNetBalanceType = finB >= 0 ? "Dr" : "Cr"
    }

    function applyGlobalCrRate(newRate) {
        globalCrRate = newRate
        for (var i = 0; i < crItemsModel.count; i++) {
            crItemsModel.setProperty(i, "rate", newRate)
        }
        recalcAllTotals()
    }

    function applyGlobalDrRate(newRate) {
        globalDrRate = newRate
        for (var j = 0; j < drItemsModel.count; j++) {
            drItemsModel.setProperty(j, "rate", newRate)
        }
        recalcAllTotals()
    }

    function applyGlobalCrGrace(newGrace) {
        globalCrGraceDays = newGrace
        for (var i = 0; i < crItemsModel.count; i++) {
            crItemsModel.setProperty(i, "dueDays", newGrace)
        }
        recalcAllTotals()
    }

    function applyGlobalDrGrace(newGrace) {
        globalDrGraceDays = newGrace
        for (var j = 0; j < drItemsModel.count; j++) {
            drItemsModel.setProperty(j, "dueDays", newGrace)
        }
        recalcAllTotals()
    }

    function formatInr(val) {
        if (typeof dashboardCtrl !== "undefined" && dashboardCtrl) {
            return dashboardCtrl.format_inr(val)
        }
        return "₹" + (val || 0).toLocaleString("en-IN", { minimumFractionDigits: 2, maximumFractionDigits: 2 })
    }

    // MAIN CONTAINER
    Rectangle {
        anchors.fill: parent
        color: "#F1F5F9"

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 10
            spacing: 8

            // 1. TOP TOOLBAR & CONTROLS
            Rectangle {
                Layout.fillWidth: true
                height: 56
                color: "#FFFFFF"
                border.color: "#CBD5E1"
                radius: 8

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 12
                    anchors.rightMargin: 12
                    spacing: 10

                    // View Title & Icon
                    RowLayout {
                        spacing: 6
                        Text {
                            text: ""
                            font.pixelSize: 18
                        }
                        ColumnLayout {
                            spacing: 0
                            Text {
                                text: "INTEREST CALCULATOR"
                                font.pixelSize: 13
                                font.bold: true
                                color: "#0F172A"
                            }
                            Text {
                                text: "Interest & Due Engine"
                                font.pixelSize: 9
                                color: "#64748B"
                            }
                        }
                    }

                    Rectangle { width: 1; height: 32; color: "#E2E8F0" }

                    // Party / Account Search Dropdown
                    CustomWhiteCombo {
                        id: partySearch
                        Layout.preferredWidth: 340
                        model: (typeof partiesModel !== "undefined" && partiesModel) ? partiesModel.get_parties_list() : []
                        onCurrentTextChanged: root.loadInterestStatement(currentText)
                        onReturnPressed: {
                            if (crItemsModel.count > 0) {
                                crListView.forceActiveFocus()
                                crListView.currentIndex = 0
                            } else if (drItemsModel.count > 0) {
                                drListView.forceActiveFocus()
                                drListView.currentIndex = 0
                            }
                        }
                    }

                    // Date Range Display Badge
                    Rectangle {
                        height: 34
                        Layout.preferredWidth: 200
                        color: "#FEF9C3"
                        border.color: "#FDE047"
                        radius: 6

                        RowLayout {
                            anchors.centerIn: parent
                            spacing: 4
                            Text {
                                text: " " + root.fromDateText + "  →  " + root.toDateText
                                color: "#854D0E"
                                font.pixelSize: 11
                                font.bold: true
                            }
                        }
                    }

                    // Year Days (360 / 365) Selector
                    Rectangle {
                        height: 34
                        implicitWidth: yearDaysRow.implicitWidth + 16
                        color: "#F8FAFC"
                        border.color: "#CBD5E1"
                        radius: 6

                        RowLayout {
                            id: yearDaysRow
                            anchors.centerIn: parent
                            spacing: 6

                            Text {
                                text: "Year Days:"
                                font.pixelSize: 11
                                color: "#334155"
                                font.bold: true
                            }

                            // 360 Button
                            Rectangle {
                                width: 44
                                height: 26
                                radius: 4
                                color: root.is360DaysMethod ? "#2563EB" : "#FFFFFF"
                                border.color: root.is360DaysMethod ? "#1D4ED8" : "#CBD5E1"

                                Text {
                                    anchors.centerIn: parent
                                    text: "360"
                                    font.pixelSize: 11
                                    font.bold: true
                                    color: root.is360DaysMethod ? "#FFFFFF" : "#475569"
                                }

                                MouseArea {
                                    anchors.fill: parent
                                    cursorShape: Qt.PointingHandCursor
                                    onClicked: {
                                        root.is360DaysMethod = true
                                        root.yearDivisor = 365
                                        root.recalcAllTotals()
                                    }
                                }
                            }

                            // 365 Button
                            Rectangle {
                                width: 44
                                height: 26
                                radius: 4
                                color: !root.is360DaysMethod ? "#2563EB" : "#FFFFFF"
                                border.color: !root.is360DaysMethod ? "#1D4ED8" : "#CBD5E1"

                                Text {
                                    anchors.centerIn: parent
                                    text: "365"
                                    font.pixelSize: 11
                                    font.bold: true
                                    color: !root.is360DaysMethod ? "#FFFFFF" : "#475569"
                                }

                                MouseArea {
                                    anchors.fill: parent
                                    cursorShape: Qt.PointingHandCursor
                                    onClicked: {
                                        root.is360DaysMethod = false
                                        root.yearDivisor = 365
                                        root.recalcAllTotals()
                                    }
                                }
                            }
                        }
                    }

                    // Op Bal Checkbox
                    RowLayout {
                        spacing: 4
                        CustomCheckBox {
                            id: opBalCheck
                            checked: root.includeOpBal
                            onCheckedChanged: {
                                root.includeOpBal = checked
                                root.loadInterestStatement(root.currentPartyName)
                            }
                        }
                        Text {
                            text: "Op. Amt"
                            font.pixelSize: 11
                            color: "#334155"
                            font.bold: true
                        }
                    }

                    Item { Layout.fillWidth: true }

                    // Action Buttons
                    RowLayout {
                        spacing: 6

                        // Set Hisab Date
                        T.Button {
                            height: 32
                            implicitWidth: contentItem.implicitWidth + 16
                            background: Rectangle { color: "#EEF2FF"; radius: 6; border.color: "#C7D2FE" }
                            contentItem: RowLayout {
                                spacing: 4
                                Text { text: "Hisab Date"; color: "#3730A3"; font.bold: true; font.pixelSize: 11 }
                                KbdBadge { text: "F6"; badgeColor: "#C7D2FE"; textColor: "#312E81"; borderColor: "#818CF8" }
                            }
                            onClicked: hisabDatePopup.open()
                        }

                        // Done Interest Voucher
                        T.Button {
                            height: 32
                            implicitWidth: contentItem.implicitWidth + 16
                            background: Rectangle { color: "#16A34A"; radius: 6 }
                            contentItem: RowLayout {
                                spacing: 4
                                Text { text: " Done Voucher"; color: "#FFFFFF"; font.bold: true; font.pixelSize: 11 }
                                KbdBadge { text: "Ctrl+Enter"; badgeColor: "#15803D"; textColor: "#BBF7D0"; borderColor: "#16A34A" }
                            }
                            onClicked: postConfirmModal.open()
                        }

                        // Refresh
                        T.Button {
                            height: 32
                            implicitWidth: contentItem.implicitWidth + 14
                            background: Rectangle { color: "#2563EB"; radius: 6 }
                            contentItem: Text { text: " Refresh"; color: "#FFF"; font.bold: true; font.pixelSize: 11 }
                            onClicked: root.loadInterestStatement(root.currentPartyName)
                        }

                        // Back / Esc
                        T.Button {
                            height: 32
                            implicitWidth: contentItem.implicitWidth + 14
                            background: Rectangle { color: "#F1F5F9"; radius: 6; border.color: "#CBD5E1" }
                            contentItem: RowLayout {
                                spacing: 4
                                Text { text: "← Back"; color: "#475569"; font.bold: true; font.pixelSize: 11 }
                                KbdBadge { text: "Esc"; badgeColor: "#E2E8F0"; textColor: "#475569"; borderColor: "#CBD5E1" }
                            }
                            onClicked: root.cancelRequested()
                        }
                    }
                }
            }

            // 2. DUAL-COLUMN TABLES (CREDIT & DEBIT)
            RowLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                spacing: 10

                // ==========================================
                // LEFT TABLE: CREDIT SIDE (JAMA / CR)
                // ==========================================
                Rectangle {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    color: "#FFFFFF"
                    border.color: crListView.activeFocus ? "#16A34A" : "#CBD5E1"
                    border.width: crListView.activeFocus ? 2 : 1
                    radius: 8
                    clip: true

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 6
                        spacing: 4

                        // Section Top Banner with Rate and Grace Days Inputs
                        Rectangle {
                            Layout.fillWidth: true
                            height: 36
                            color: "#DCFCE7"
                            border.color: "#86EFAC"
                            radius: 6

                            RowLayout {
                                anchors.fill: parent
                                anchors.leftMargin: 8
                                anchors.rightMargin: 8
                                spacing: 8

                                Text {
                                    text: " Cr. Side (Jama)"
                                    color: "#15803D"
                                    font.pixelSize: 12
                                    font.bold: true
                                }
                                KbdBadge { text: "Alt+R"; badgeColor: "#BBF7D0"; textColor: "#14532D"; borderColor: "#86EFAC" }

                                Item { Layout.fillWidth: true }

                                // Global Rate Input
                                Text {
                                    text: "Rate %:"
                                    font.pixelSize: 11
                                    font.bold: true
                                    color: "#166534"
                                }

                                Rectangle {
                                    width: 46
                                    height: 24
                                    color: "#FFFFFF"
                                    border.color: "#86EFAC"
                                    radius: 4

                                    TextInput {
                                        id: crRateInput
                                        anchors.fill: parent
                                        text: root.globalCrRate.toString()
                                        font.pixelSize: 11
                                        font.bold: true
                                        color: "#14532D"
                                        horizontalAlignment: Text.AlignHCenter
                                        verticalAlignment: Text.AlignVCenter
                                        onEditingFinished: {
                                            var val = parseFloat(text) || 12.0
                                            root.applyGlobalCrRate(val)
                                        }
                                        Keys.onReturnPressed: {
                                            var val2 = parseFloat(text) || 12.0
                                            root.applyGlobalCrRate(val2)
                                            crGraceInput.forceActiveFocus()
                                            crGraceInput.selectAll()
                                        }
                                    }
                                }

                                // Global Grace Days Input
                                Text {
                                    text: "Grace (D):"
                                    font.pixelSize: 11
                                    font.bold: true
                                    color: "#92400E"
                                }

                                Rectangle {
                                    width: 46
                                    height: 24
                                    color: root.globalCrGraceDays > 0 ? "#FEF3C7" : "#FFFFFF"
                                    border.color: root.globalCrGraceDays > 0 ? "#D97706" : "#86EFAC"
                                    radius: 4

                                    TextInput {
                                        id: crGraceInput
                                        anchors.fill: parent
                                        text: root.globalCrGraceDays.toString()
                                        font.pixelSize: 11
                                        font.bold: true
                                        color: root.globalCrGraceDays > 0 ? "#92400E" : "#14532D"
                                        horizontalAlignment: Text.AlignHCenter
                                        verticalAlignment: Text.AlignVCenter
                                        onEditingFinished: {
                                            var gVal = parseInt(text) || 0
                                            root.applyGlobalCrGrace(gVal)
                                        }
                                        Keys.onReturnPressed: {
                                            var gVal2 = parseInt(text) || 0
                                            root.applyGlobalCrGrace(gVal2)
                                            if (crItemsModel.count > 0) {
                                                crListView.forceActiveFocus()
                                                crListView.currentIndex = 0
                                            }
                                        }
                                    }
                                }
                            }
                        }

                        // Column Headers Table Row
                        Rectangle {
                            Layout.fillWidth: true
                            height: 28
                            color: "#F1F5F9"
                            border.color: "#E2E8F0"
                            radius: 4

                            RowLayout {
                                anchors.fill: parent
                                anchors.leftMargin: 6
                                anchors.rightMargin: 6
                                spacing: 4

                                Text { text: "Cr. Date"; color: "#475569"; font.pixelSize: 11; font.bold: true; Layout.preferredWidth: 76 }
                                Text { text: "Type/Ref"; color: "#475569"; font.pixelSize: 11; font.bold: true; Layout.preferredWidth: 64 }
                                Text { text: "Amount (" + crItemsModel.count + ")"; color: "#475569"; font.pixelSize: 11; font.bold: true; Layout.preferredWidth: 105; horizontalAlignment: Text.AlignRight }
                                Text { text: "Rate"; color: "#475569"; font.pixelSize: 11; font.bold: true; Layout.preferredWidth: 42; horizontalAlignment: Text.AlignHCenter }
                                Text { text: "D (Grace)"; color: "#B45309"; font.pixelSize: 11; font.bold: true; Layout.preferredWidth: 54; horizontalAlignment: Text.AlignHCenter }
                                Text { text: "Days"; color: "#475569"; font.pixelSize: 11; font.bold: true; Layout.preferredWidth: 40; horizontalAlignment: Text.AlignHCenter }
                                Text { text: "Interest Amt (₹)"; color: "#15803D"; font.pixelSize: 11; font.bold: true; Layout.fillWidth: true; horizontalAlignment: Text.AlignRight }
                            }
                        }

                        // Scrollable List
                        ListView {
                            id: crListView
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            model: crItemsModel
                            clip: true
                            spacing: 1
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
                                if (crListView.currentIndex < crItemsModel.count - 1) {
                                    event.accepted = true
                                    crListView.currentIndex++
                                    crListView.positionViewAtIndex(crListView.currentIndex, ListView.Contain)
                                }
                            }
                            Keys.onRightPressed: function(event) {
                                event.accepted = true
                                drListView.forceActiveFocus()
                                if (drListView.currentIndex < 0 && drItemsModel.count > 0) {
                                    drListView.currentIndex = Math.min(Math.max(0, crListView.currentIndex), drItemsModel.count - 1)
                                }
                            }

                            T.ScrollBar.vertical: T.ScrollBar {
                                policy: T.ScrollBar.AsNeeded
                            }

                            delegate: Rectangle {
                                id: crDelegateRect
                                width: crListView.width
                                height: 30
                                color: (crListView.activeFocus && crListView.currentIndex === index) ? "#DCFCE7" : (index % 2 === 0 ? "#FFFFFF" : "#F8FAFC")
                                border.color: (crListView.activeFocus && crListView.currentIndex === index) ? "#16A34A" : "#E2E8F0"
                                border.width: (crListView.activeFocus && crListView.currentIndex === index) ? 1.5 : 1
                                radius: 3

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
                                    spacing: 4

                                    Text {
                                        text: model.vDate
                                        color: "#334155"
                                        font.pixelSize: 10
                                        Layout.preferredWidth: 76
                                    }

                                    Text {
                                        text: model.vType || "Vchr"
                                        color: "#64748B"
                                        font.pixelSize: 10
                                        elide: Text.ElideRight
                                        Layout.preferredWidth: 64
                                    }

                                    Text {
                                        text: root.formatInr(model.amount)
                                        color: "#0F172A"
                                        font.pixelSize: 11
                                        font.bold: true
                                        Layout.preferredWidth: 105
                                        horizontalAlignment: Text.AlignRight
                                    }

                                    // Editable Rate %
                                    Rectangle {
                                        Layout.preferredWidth: 42
                                        height: 22
                                        color: "#F1F5F9"
                                        border.color: "#CBD5E1"
                                        radius: 3

                                        TextInput {
                                            id: rowCrRate
                                            anchors.fill: parent
                                            text: model.rate ? model.rate.toString() : "12"
                                            font.pixelSize: 10
                                            font.bold: true
                                            color: "#0F172A"
                                            horizontalAlignment: Text.AlignHCenter
                                            verticalAlignment: Text.AlignVCenter
                                            onEditingFinished: {
                                                crItemsModel.setProperty(index, "rate", parseFloat(text) || 12.0)
                                                root.recalcAllTotals()
                                            }
                                            Keys.onReturnPressed: {
                                                crItemsModel.setProperty(index, "rate", parseFloat(text) || 12.0)
                                                root.recalcAllTotals()
                                                rowCrDue.forceActiveFocus()
                                                rowCrDue.selectAll()
                                            }
                                            Keys.onUpPressed: {
                                                if (index > 0) {
                                                    crListView.currentIndex = index - 1
                                                    crListView.positionViewAtIndex(index - 1, ListView.Contain)
                                                }
                                            }
                                            Keys.onDownPressed: {
                                                if (index < crItemsModel.count - 1) {
                                                    crListView.currentIndex = index + 1
                                                    crListView.positionViewAtIndex(index + 1, ListView.Contain)
                                                }
                                            }
                                        }
                                    }

                                    // Editable Grace/Due Days (D)
                                    Rectangle {
                                        Layout.preferredWidth: 54
                                        height: 22
                                        color: model.dueDays > 0 ? "#FEF3C7" : "#FFFFFF"
                                        border.color: model.dueDays > 0 ? "#D97706" : "#CBD5E1"
                                        border.width: model.dueDays > 0 ? 1.5 : 1
                                        radius: 3

                                        TextInput {
                                            id: rowCrDue
                                            anchors.fill: parent
                                            text: model.dueDays ? model.dueDays.toString() : "0"
                                            font.pixelSize: 10
                                            font.bold: model.dueDays > 0
                                            color: model.dueDays > 0 ? "#92400E" : "#475569"
                                            horizontalAlignment: Text.AlignHCenter
                                            verticalAlignment: Text.AlignVCenter
                                            onEditingFinished: {
                                                crItemsModel.setProperty(index, "dueDays", parseInt(text) || 0)
                                                root.recalcAllTotals()
                                            }
                                            Keys.onReturnPressed: {
                                                crItemsModel.setProperty(index, "dueDays", parseInt(text) || 0)
                                                root.recalcAllTotals()
                                                if (index < crItemsModel.count - 1) {
                                                    crListView.currentIndex = index + 1
                                                    crListView.positionViewAtIndex(index + 1, ListView.Contain)
                                                } else {
                                                    crListView.forceActiveFocus()
                                                }
                                            }
                                            Keys.onUpPressed: {
                                                if (index > 0) {
                                                    crListView.currentIndex = index - 1
                                                    crListView.positionViewAtIndex(index - 1, ListView.Contain)
                                                }
                                            }
                                            Keys.onDownPressed: {
                                                if (index < crItemsModel.count - 1) {
                                                    crListView.currentIndex = index + 1
                                                    crListView.positionViewAtIndex(index + 1, ListView.Contain)
                                                }
                                            }
                                            Keys.onEscapePressed: crListView.forceActiveFocus()
                                        }
                                    }

                                    Text {
                                        text: model.days.toString()
                                        color: "#475569"
                                        font.pixelSize: 11
                                        font.bold: true
                                        Layout.preferredWidth: 40
                                        horizontalAlignment: Text.AlignHCenter
                                    }

                                    Text {
                                        text: root.formatInr(model.interestAmt)
                                        color: "#15803D"
                                        font.pixelSize: 11
                                        font.bold: true
                                        Layout.fillWidth: true
                                        horizontalAlignment: Text.AlignRight
                                    }
                                }
                            }
                        }

                        // Cr Subtotals Footer Bar
                        Rectangle {
                            Layout.fillWidth: true
                            height: 32
                            color: "#DCFCE7"
                            border.color: "#86EFAC"
                            radius: 6

                            RowLayout {
                                anchors.fill: parent
                                anchors.leftMargin: 10
                                anchors.rightMargin: 10
                                Text {
                                    text: "Cr Total: " + root.formatInr(root.crTotalAmount)
                                    color: "#166534"
                                    font.pixelSize: 12
                                    font.bold: true
                                }
                                Item { Layout.fillWidth: true }
                                Text {
                                    text: "Cr Interest: " + root.formatInr(root.crTotalInterest)
                                    color: "#15803D"
                                    font.pixelSize: 13
                                    font.bold: true
                                }
                            }
                        }
                    }
                }

                // ==========================================
                // RIGHT TABLE: DEBIT SIDE (NAMA / DR)
                // ==========================================
                Rectangle {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    color: "#FFFFFF"
                    border.color: drListView.activeFocus ? "#2563EB" : "#CBD5E1"
                    border.width: drListView.activeFocus ? 2 : 1
                    radius: 8
                    clip: true

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 6
                        spacing: 4

                        // Section Top Banner with Rate and Grace Days Inputs
                        Rectangle {
                            Layout.fillWidth: true
                            height: 36
                            color: "#DBEAFE"
                            border.color: "#93C5FD"
                            radius: 6

                            RowLayout {
                                anchors.fill: parent
                                anchors.leftMargin: 8
                                anchors.rightMargin: 8
                                spacing: 8

                                Text {
                                    text: "Dr. Side (Name)"
                                    color: "#1E40AF"
                                    font.pixelSize: 12
                                    font.bold: true
                                }
                                KbdBadge { text: "Alt+B"; badgeColor: "#BFDBFE"; textColor: "#1E3A8A"; borderColor: "#93C5FD" }

                                Item { Layout.fillWidth: true }

                                // Global Rate Input
                                Text {
                                    text: "Rate %:"
                                    font.pixelSize: 11
                                    font.bold: true
                                    color: "#1E40AF"
                                }

                                Rectangle {
                                    width: 46
                                    height: 24
                                    color: "#FFFFFF"
                                    border.color: "#93C5FD"
                                    radius: 4

                                    TextInput {
                                        id: drRateInput
                                        anchors.fill: parent
                                        text: root.globalDrRate.toString()
                                        font.pixelSize: 11
                                        font.bold: true
                                        color: "#1E3A8A"
                                        horizontalAlignment: Text.AlignHCenter
                                        verticalAlignment: Text.AlignVCenter
                                        onEditingFinished: {
                                            var val = parseFloat(text) || 12.0
                                            root.applyGlobalDrRate(val)
                                        }
                                        Keys.onReturnPressed: {
                                            var val2 = parseFloat(text) || 12.0
                                            root.applyGlobalDrRate(val2)
                                            drGraceInput.forceActiveFocus()
                                            drGraceInput.selectAll()
                                        }
                                    }
                                }

                                // Global Grace Days Input
                                Text {
                                    text: "Grace (D):"
                                    font.pixelSize: 11
                                    font.bold: true
                                    color: "#92400E"
                                }

                                Rectangle {
                                    width: 46
                                    height: 24
                                    color: root.globalDrGraceDays > 0 ? "#FEF3C7" : "#FFFFFF"
                                    border.color: root.globalDrGraceDays > 0 ? "#D97706" : "#93C5FD"
                                    radius: 4

                                    TextInput {
                                        id: drGraceInput
                                        anchors.fill: parent
                                        text: root.globalDrGraceDays.toString()
                                        font.pixelSize: 11
                                        font.bold: true
                                        color: root.globalDrGraceDays > 0 ? "#92400E" : "#1E3A8A"
                                        horizontalAlignment: Text.AlignHCenter
                                        verticalAlignment: Text.AlignVCenter
                                        onEditingFinished: {
                                            var drGVal = parseInt(text) || 0
                                            root.applyGlobalDrGrace(drGVal)
                                        }
                                        Keys.onReturnPressed: {
                                            var drGVal2 = parseInt(text) || 0
                                            root.applyGlobalDrGrace(drGVal2)
                                            if (drItemsModel.count > 0) {
                                                drListView.forceActiveFocus()
                                                drListView.currentIndex = 0
                                            }
                                        }
                                    }
                                }
                            }
                        }

                        // Column Headers Table Row
                        Rectangle {
                            Layout.fillWidth: true
                            height: 28
                            color: "#F1F5F9"
                            border.color: "#E2E8F0"
                            radius: 4

                            RowLayout {
                                anchors.fill: parent
                                anchors.leftMargin: 6
                                anchors.rightMargin: 6
                                spacing: 4

                                Text { text: "Dr. Date"; color: "#475569"; font.pixelSize: 11; font.bold: true; Layout.preferredWidth: 76 }
                                Text { text: "Type/Ref"; color: "#475569"; font.pixelSize: 11; font.bold: true; Layout.preferredWidth: 64 }
                                Text { text: "Amount (" + drItemsModel.count + ")"; color: "#475569"; font.pixelSize: 11; font.bold: true; Layout.preferredWidth: 105; horizontalAlignment: Text.AlignRight }
                                Text { text: "Rate"; color: "#475569"; font.pixelSize: 11; font.bold: true; Layout.preferredWidth: 42; horizontalAlignment: Text.AlignHCenter }
                                Text { text: "D (Grace)"; color: "#B45309"; font.pixelSize: 11; font.bold: true; Layout.preferredWidth: 54; horizontalAlignment: Text.AlignHCenter }
                                Text { text: "Days"; color: "#475569"; font.pixelSize: 11; font.bold: true; Layout.preferredWidth: 40; horizontalAlignment: Text.AlignHCenter }
                                Text { text: "Interest Amt (₹)"; color: "#2563EB"; font.pixelSize: 11; font.bold: true; Layout.fillWidth: true; horizontalAlignment: Text.AlignRight }
                            }
                        }

                        // Scrollable List
                        ListView {
                            id: drListView
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            model: drItemsModel
                            clip: true
                            spacing: 1
                            boundsBehavior: Flickable.StopAtBounds
                            focus: true
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
                                if (drListView.currentIndex < drItemsModel.count - 1) {
                                    event.accepted = true
                                    drListView.currentIndex++
                                    drListView.positionViewAtIndex(drListView.currentIndex, ListView.Contain)
                                }
                            }
                            Keys.onLeftPressed: function(event) {
                                event.accepted = true
                                crListView.forceActiveFocus()
                                if (crListView.currentIndex < 0 && crItemsModel.count > 0) {
                                    crListView.currentIndex = Math.min(Math.max(0, drListView.currentIndex), crItemsModel.count - 1)
                                }
                            }

                            T.ScrollBar.vertical: T.ScrollBar {
                                policy: T.ScrollBar.AsNeeded
                            }

                            delegate: Rectangle {
                                id: drDelegateRect
                                width: drListView.width
                                height: 30
                                color: (drListView.activeFocus && drListView.currentIndex === index) ? "#DBEAFE" : (index % 2 === 0 ? "#FFFFFF" : "#F8FAFC")
                                border.color: (drListView.activeFocus && drListView.currentIndex === index) ? "#2563EB" : "#E2E8F0"
                                border.width: (drListView.activeFocus && drListView.currentIndex === index) ? 1.5 : 1
                                radius: 3

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
                                    spacing: 4

                                    Text {
                                        text: model.vDate
                                        color: "#334155"
                                        font.pixelSize: 10
                                        Layout.preferredWidth: 76
                                    }

                                    Text {
                                        text: model.vType || "Vchr"
                                        color: "#64748B"
                                        font.pixelSize: 10
                                        elide: Text.ElideRight
                                        Layout.preferredWidth: 64
                                    }

                                    Text {
                                        text: root.formatInr(model.amount)
                                        color: "#0F172A"
                                        font.pixelSize: 11
                                        font.bold: true
                                        Layout.preferredWidth: 105
                                        horizontalAlignment: Text.AlignRight
                                    }

                                    // Editable Rate %
                                    Rectangle {
                                        Layout.preferredWidth: 42
                                        height: 22
                                        color: "#F1F5F9"
                                        border.color: "#CBD5E1"
                                        radius: 3

                                        TextInput {
                                            id: rowDrRate
                                            anchors.fill: parent
                                            text: model.rate ? model.rate.toString() : "12"
                                            font.pixelSize: 10
                                            font.bold: true
                                            color: "#0F172A"
                                            horizontalAlignment: Text.AlignHCenter
                                            verticalAlignment: Text.AlignVCenter
                                            onEditingFinished: {
                                                drItemsModel.setProperty(index, "rate", parseFloat(text) || 12.0)
                                                root.recalcAllTotals()
                                            }
                                            Keys.onReturnPressed: {
                                                drItemsModel.setProperty(index, "rate", parseFloat(text) || 12.0)
                                                root.recalcAllTotals()
                                                rowDrDue.forceActiveFocus()
                                                rowDrDue.selectAll()
                                            }
                                            Keys.onUpPressed: {
                                                if (index > 0) {
                                                    drListView.currentIndex = index - 1
                                                    drListView.positionViewAtIndex(index - 1, ListView.Contain)
                                                }
                                            }
                                            Keys.onDownPressed: {
                                                if (index < drItemsModel.count - 1) {
                                                    drListView.currentIndex = index + 1
                                                    drListView.positionViewAtIndex(index + 1, ListView.Contain)
                                                }
                                            }
                                        }
                                    }

                                    // Editable Grace/Due Days (D)
                                    Rectangle {
                                        Layout.preferredWidth: 54
                                        height: 22
                                        color: model.dueDays > 0 ? "#FEF3C7" : "#FFFFFF"
                                        border.color: model.dueDays > 0 ? "#D97706" : "#CBD5E1"
                                        border.width: model.dueDays > 0 ? 1.5 : 1
                                        radius: 3

                                        TextInput {
                                            id: rowDrDue
                                            anchors.fill: parent
                                            text: model.dueDays ? model.dueDays.toString() : "0"
                                            font.pixelSize: 10
                                            font.bold: model.dueDays > 0
                                            color: model.dueDays > 0 ? "#92400E" : "#475569"
                                            horizontalAlignment: Text.AlignHCenter
                                            verticalAlignment: Text.AlignVCenter
                                            onEditingFinished: {
                                                drItemsModel.setProperty(index, "dueDays", parseInt(text) || 0)
                                                root.recalcAllTotals()
                                            }
                                            Keys.onReturnPressed: {
                                                drItemsModel.setProperty(index, "dueDays", parseInt(text) || 0)
                                                root.recalcAllTotals()
                                                if (index < drItemsModel.count - 1) {
                                                    drListView.currentIndex = index + 1
                                                    drListView.positionViewAtIndex(index + 1, ListView.Contain)
                                                } else {
                                                    drListView.forceActiveFocus()
                                                }
                                            }
                                            Keys.onUpPressed: {
                                                if (index > 0) {
                                                    drListView.currentIndex = index - 1
                                                    drListView.positionViewAtIndex(index - 1, ListView.Contain)
                                                }
                                            }
                                            Keys.onDownPressed: {
                                                if (index < drItemsModel.count - 1) {
                                                    drListView.currentIndex = index + 1
                                                    drListView.positionViewAtIndex(index + 1, ListView.Contain)
                                                }
                                            }
                                            Keys.onEscapePressed: drListView.forceActiveFocus()
                                        }
                                    }

                                    Text {
                                        text: model.days.toString()
                                        color: "#475569"
                                        font.pixelSize: 11
                                        font.bold: true
                                        Layout.preferredWidth: 40
                                        horizontalAlignment: Text.AlignHCenter
                                    }

                                    Text {
                                        text: root.formatInr(model.interestAmt)
                                        color: "#2563EB"
                                        font.pixelSize: 11
                                        font.bold: true
                                        Layout.fillWidth: true
                                        horizontalAlignment: Text.AlignRight
                                    }
                                }
                            }
                        }

                        // Dr Subtotals Footer Bar
                        Rectangle {
                            Layout.fillWidth: true
                            height: 32
                            color: "#DBEAFE"
                            border.color: "#93C5FD"
                            radius: 6

                            RowLayout {
                                anchors.fill: parent
                                anchors.leftMargin: 10
                                anchors.rightMargin: 10
                                Text {
                                    text: "Dr Total: " + root.formatInr(root.drTotalAmount)
                                    color: "#1E3A8A"
                                    font.pixelSize: 12
                                    font.bold: true
                                }
                                Item { Layout.fillWidth: true }
                                Text {
                                    text: "Dr Interest: " + root.formatInr(root.drTotalInterest)
                                    color: "#1D4ED8"
                                    font.pixelSize: 13
                                    font.bold: true
                                }
                            }
                        }
                    }
                }
            }

            // 3. PINNED BOTTOM GRAND SUMMARY RIBBON
            Rectangle {
                Layout.fillWidth: true
                height: 52
                color: "#0F172A"
                radius: 8

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 14
                    anchors.rightMargin: 14
                    spacing: 12

                    // Net Interest Result Header & Value
                    RowLayout {
                        spacing: 8
                        Text {
                            text: ""
                            font.pixelSize: 18
                        }
                        ColumnLayout {
                            spacing: 1
                            Text {
                                text: "NET INTEREST RESULT"
                                color: "#94A3B8"
                                font.pixelSize: 9
                                font.bold: true
                                font.letterSpacing: 0.8
                            }
                            Text {
                                text: (root.netInterestType.indexOf("Receivable") !== -1 ? "Net Interest Receivable: " : "Net Interest Payable: ") + root.formatInr(root.netInterest)
                                color: "#FACC15"
                                font.pixelSize: 14
                                font.bold: true
                            }
                        }
                    }

                    Item { Layout.fillWidth: true }

                    // Metrics Badges: Principal Balance | Net Interest | Grand Total Balance
                    RowLayout {
                        spacing: 8

                        // Ledger Principal Balance Pill
                        Rectangle {
                            height: 32
                            implicitWidth: balTxt.implicitWidth + 20
                            color: "#1E293B"
                            radius: 6
                            border.color: "#334155"
                            RowLayout {
                                anchors.centerIn: parent
                                spacing: 4
                                Text {
                                    id: balTxt
                                    text: "Ledger Bal: " + root.formatInr(root.ledgerBalance) + " " + root.ledgerBalanceType
                                    color: "#CBD5E1"
                                    font.pixelSize: 11
                                    font.bold: true
                                }
                            }
                        }

                        // Net Interest Pill
                        Rectangle {
                            height: 32
                            implicitWidth: intTxt.implicitWidth + 20
                            color: "#1E293B"
                            radius: 6
                            border.color: "#334155"
                            RowLayout {
                                anchors.centerIn: parent
                                spacing: 4
                                Text {
                                    id: intTxt
                                    text: "Intt Diff: " + root.formatInr(root.netInterest) + " " + (root.netInterestType.indexOf("Receivable") !== -1 ? "Dr" : "Cr")
                                    color: "#FDE047"
                                    font.pixelSize: 11
                                    font.bold: true
                                }
                            }
                        }

                        // Final Net Settlement Pill
                        Rectangle {
                            height: 36
                            implicitWidth: netBalTxt.implicitWidth + 24
                            color: root.finalNetBalanceType === "Dr" ? "#2563EB" : "#059669"
                            radius: 6
                            border.color: root.finalNetBalanceType === "Dr" ? "#60A5FA" : "#34D399"
                            RowLayout {
                                anchors.centerIn: parent
                                spacing: 6
                                Text {
                                    id: netBalTxt
                                    text: "FINAL NET BAL: " + root.formatInr(root.finalNetBalance) + " " + root.finalNetBalanceType
                                    color: "#FFFFFF"
                                    font.pixelSize: 12
                                    font.bold: true
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    // F6: SET HISAB DATE POPUP
    T.Popup {
        id: hisabDatePopup
        width: 320
        height: 200
        modal: true
        dim: true
        anchors.centerIn: parent
        focus: true
        closePolicy: T.Popup.CloseOnPressOutside | T.Popup.CloseOnEscape

        background: Rectangle {
            color: "#FFFFFF"
            border.color: "#2563EB"
            border.width: 2
            radius: 10
        }

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 16
            spacing: 12

            Text { text: "SET HISAB / SETTLEMENT DATE"; font.bold: true; color: "#0F172A"; font.pixelSize: 13 }
            
            Text { text: "Enter Cut-Off Date for Interest Calculation:"; color: "#64748B"; font.pixelSize: 11 }

            Rectangle {
                Layout.fillWidth: true
                height: 36
                color: "#F8FAFC"
                border.color: "#CBD5E1"
                radius: 6

                TextInput {
                    id: hisabDateInput
                    anchors.fill: parent
                    anchors.margins: 8
                    text: root.toDateText
                    font.pixelSize: 13
                    font.bold: true
                    color: "#0F172A"
                }
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: 10

                T.Button {
                    Layout.fillWidth: true
                    height: 34
                    background: Rectangle { color: "#F1F5F9"; radius: 6; border.color: "#CBD5E1" }
                    contentItem: Text { text: "Cancel"; color: "#475569"; font.bold: true; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                    onClicked: hisabDatePopup.close()
                }

                T.Button {
                    Layout.fillWidth: true
                    height: 34
                    background: Rectangle { color: "#2563EB"; radius: 6 }
                    contentItem: Text { text: "Apply Date"; color: "#FFFFFF"; font.bold: true; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                    onClicked: {
                        var dt = hisabDateInput.text.trim()
                        if (dt) {
                            root.toDateText = dt
                            root.toIsoText = root.toIso(dt)
                            root.recalcAllTotals()
                        }
                        hisabDatePopup.close()
                    }
                }
            }
        }
    }

    // CONFIRM POST INTEREST VOUCHER MODAL
    ConfirmationModal {
        id: postConfirmModal
        anchors.centerIn: parent
        titleText: "POST INTEREST JOURNAL VOUCHER"
        messageText: "Are you sure you want to create and post an Interest Journal Voucher for " + root.currentPartyName + " with Net Interest of " + root.formatInr(root.netInterest) + " (" + root.netInterestType + ")?"
        onConfirmed: {
            if (typeof interestModel !== "undefined" && interestModel) {
                var isRec = root.netInterestType.indexOf("Receivable") !== -1
                var ok = interestModel.post_interest_voucher(
                    root.currentPartyName,
                    root.toIsoText,
                    root.netInterest,
                    isRec,
                    "Interest up to " + root.toDateText
                )
                if (ok) {
                    root.statusMessage = "Interest Voucher posted successfully!"
                    root.isError = false
                    root.interestVoucherSaved()
                } else {
                    root.statusMessage = "Failed to post Interest Voucher."
                    root.isError = true
                }
            }
        }
    }

    // SHORTCUTS
    Shortcut { sequence: "Alt+S"; onActivated: partySearch.forceActiveFocus() }
    Shortcut { sequence: "Alt+P"; onActivated: partySearch.forceActiveFocus() }
    Shortcut { sequence: "Alt+R"; onActivated: { crRateInput.forceActiveFocus(); crRateInput.selectAll() } }
    Shortcut { sequence: "Alt+B"; onActivated: { drRateInput.forceActiveFocus(); drRateInput.selectAll() } }
    Shortcut { sequence: "Alt+G"; onActivated: { crGraceInput.forceActiveFocus(); crGraceInput.selectAll() } }
    Shortcut { sequence: "Alt+H"; onActivated: { drGraceInput.forceActiveFocus(); drGraceInput.selectAll() } }
    Shortcut { sequence: "F6"; onActivated: hisabDatePopup.open() }
    Shortcut { sequence: "Shift+Return"; onActivated: root.loadInterestStatement(root.currentPartyName) }
    Shortcut { sequence: "Ctrl+Return"; onActivated: postConfirmModal.open() }
    Shortcut { sequence: "Escape"; onActivated: root.cancelRequested() }
}



