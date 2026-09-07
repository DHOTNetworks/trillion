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
                it.dueDays = it.dueDays || 0
                it.rate = it.rate || globalCrRate
                crItemsModel.append(it)
            }

            var drs = data.dr_items || []
            for (var j = 0; j < drs.length; j++) {
                var itm = drs[j]
                itm.dueDays = itm.dueDays || 0
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
            var rowInt = Math.round(ci.amount * (r / 100.0) * effD / divisor * 100.0) / 100.0

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
            var rowDrInt = Math.round(di.amount * (drR / 100.0) * effDr / divisor * 100.0) / 100.0

            drItemsModel.setProperty(j, "days", effDr)
            drItemsModel.setProperty(j, "interestAmt", rowDrInt)
            drAmtSum += di.amount
            drIntSum += rowDrInt
        }

        crTotalAmount = crAmtSum
        crTotalInterest = crIntSum
        drTotalAmount = drAmtSum
        drTotalInterest = drIntSum

        var netI = drTotalInterest - crTotalInterest
        netInterest = Math.abs(netI)
        netInterestType = netI >= 0 ? "Dr (Receivable)" : "Cr (Payable)"

        var ledgB = drTotalAmount - crTotalAmount
        ledgerBalance = Math.abs(ledgB)
        ledgerBalanceType = ledgB >= 0 ? "Dr" : "Cr"

        var finB = ledgB + netI
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

    function formatInr(val) {
        if (typeof dashboardCtrl !== "undefined" && dashboardCtrl) {
            return dashboardCtrl.format_inr(val)
        }
        return "₹" + val.toFixed(2)
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 12
        spacing: 10

        // 1. TOP HEADER & SETTINGS TOOLBAR
        Rectangle {
            Layout.fillWidth: true
            height: 56
            color: "#FFFFFF"
            border.color: "#CBD5E1"
            radius: 8

            RowLayout {
                anchors.fill: parent
                anchors.margins: 10
                spacing: 12

                Text {
                    text: "📊 INTEREST CALCULATOR"
                    font.pixelSize: 14
                    font.bold: true
                    color: "#0F172A"
                }

                // Party Selector
                CustomWhiteCombo {
                    id: partySearch
                    Layout.preferredWidth: 300
                    model: (typeof partiesModel !== "undefined" && partiesModel) ? partiesModel.get_parties_list() : []
                    onCurrentTextChanged: root.loadInterestStatement(currentText)
                }

                // Date Criteria Badge
                Rectangle {
                    height: 32
                    Layout.preferredWidth: 220
                    color: "#FEF9C3"
                    border.color: "#FDE047"
                    radius: 6

                    RowLayout {
                        anchors.centerIn: parent
                        spacing: 6
                        Text {
                            text: "📅 Date: (" + root.fromDateText + " To " + root.toDateText + ")"
                            color: "#854D0E"
                            font.pixelSize: 11
                            font.bold: true
                        }
                    }
                }

                // Year Days Selection
                Rectangle {
                    height: 32
                    Layout.preferredWidth: 150
                    color: "#F1F5F9"
                    border.color: "#CBD5E1"
                    radius: 6

                    RowLayout {
                        anchors.fill: parent
                        anchors.margins: 4
                        spacing: 4

                        Text {
                            text: "Year Days:"
                            font.pixelSize: 11
                            color: "#475569"
                            font.bold: true
                        }

                        T.ComboBox {
                            id: yearDaysCombo
                            Layout.fillWidth: true
                            model: ["360 / 365", "365 / 365", "360 / 360", "365 / 360"]
                            currentIndex: 0
                            onCurrentIndexChanged: {
                                if (currentIndex === 0) { root.is360DaysMethod = true; root.yearDivisor = 365 }
                                else if (currentIndex === 1) { root.is360DaysMethod = false; root.yearDivisor = 365 }
                                else if (currentIndex === 2) { root.is360DaysMethod = true; root.yearDivisor = 360 }
                                else if (currentIndex === 3) { root.is360DaysMethod = false; root.yearDivisor = 360 }
                                root.recalcAllTotals()
                            }
                        }
                    }
                }

                // Op. Bal Checkbox
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
                        text: "Op. Amt Include"
                        font.pixelSize: 11
                        color: "#334155"
                        font.bold: true
                    }
                }

                Item { Layout.fillWidth: true }

                // Set Hisab Date Button
                T.Button {
                    height: 32
                    implicitWidth: contentItem.implicitWidth + 20
                    background: Rectangle { color: "#E0E7FF"; radius: 6; border.color: "#A5B4FC" }
                    contentItem: RowLayout {
                        spacing: 4
                        Text { text: "🗓️ Set Hisab Date"; color: "#3730A3"; font.bold: true; font.pixelSize: 11 }
                        KbdBadge { text: "F6"; badgeColor: "#C7D2FE"; textColor: "#312E81"; borderColor: "#818CF8" }
                    }
                    onClicked: hisabDatePopup.open()
                }

                // Done Interest Voucher Button
                T.Button {
                    height: 32
                    implicitWidth: contentItem.implicitWidth + 20
                    background: Rectangle { color: "#16A34A"; radius: 6 }
                    contentItem: RowLayout {
                        spacing: 4
                        Text { text: "⚡ Done Interest Voucher"; color: "#FFFFFF"; font.bold: true; font.pixelSize: 11 }
                        KbdBadge { text: "Ctrl+Enter"; badgeColor: "#15803D"; textColor: "#BBF7D0"; borderColor: "#16A34A" }
                    }
                    onClicked: postConfirmModal.open()
                }

                // Refresh Button
                T.Button {
                    height: 32
                    implicitWidth: contentItem.implicitWidth + 16
                    background: Rectangle { color: "#2563EB"; radius: 6 }
                    contentItem: Text { text: "🔄 Refresh"; color: "#FFF"; font.bold: true; font.pixelSize: 11 }
                    onClicked: root.loadInterestStatement(root.currentPartyName)
                }
            }
        }

        // 2. MAIN 2-COLUMN INTEREST CALCULATION SECTION (Cr on Left, Dr on Right)
        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 12

            // ==================== LEFT COLUMN: CREDIT SIDE (JAMA) ====================
            Rectangle {
                Layout.fillWidth: true
                Layout.fillHeight: true
                color: "#FFFFFF"
                border.color: "#CBD5E1"
                radius: 8

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 8
                    spacing: 6

                    // Side Header Bar with Global Rate
                    Rectangle {
                        Layout.fillWidth: true
                        height: 36
                        color: "#DCFCE7"
                        radius: 6

                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: 10
                            anchors.rightMargin: 10
                            spacing: 8

                            Text {
                                text: "🟢 Cr. Side (Jama)"
                                color: "#15803D"
                                font.pixelSize: 13
                                font.bold: true
                            }
                            KbdBadge { text: "Alt+R"; badgeColor: "#BBF7D0"; textColor: "#14532D"; borderColor: "#86EFAC" }

                            Item { Layout.fillWidth: true }

                            Text {
                                text: "Interest Rate (Annual) %:"
                                font.pixelSize: 11
                                font.bold: true
                                color: "#166534"
                            }

                            Rectangle {
                                width: 50
                                height: 26
                                color: "#FFFFFF"
                                border.color: "#86EFAC"
                                radius: 4

                                TextInput {
                                    id: crRateInput
                                    anchors.centerIn: parent
                                    width: 42
                                    text: root.globalCrRate.toString()
                                    font.pixelSize: 12
                                    font.bold: true
                                    color: "#14532D"
                                    horizontalAlignment: Text.AlignHCenter
                                    onEditingFinished: {
                                        var val = parseFloat(text) || 12.0
                                        root.applyGlobalCrRate(val)
                                    }
                                }
                            }
                        }
                    }

                    // Table Columns Header
                    Rectangle {
                        Layout.fillWidth: true
                        height: 26
                        color: "#F8FAFC"
                        radius: 4
                        border.color: "#E2E8F0"

                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: 6
                            anchors.rightMargin: 6
                            spacing: 4

                            Text { text: "Cr.Date"; color: "#475569"; font.pixelSize: 11; font.bold: true; width: 75 }
                            Text { text: "Cr.Amount (" + crItemsModel.count + ")"; color: "#475569"; font.pixelSize: 11; font.bold: true; width: 110; horizontalAlignment: Text.AlignRight }
                            Text { text: "Rate"; color: "#475569"; font.pixelSize: 11; font.bold: true; width: 36; horizontalAlignment: Text.AlignHCenter }
                            Text { text: "D (Grace)"; color: "#B45309"; font.pixelSize: 11; font.bold: true; width: 50; horizontalAlignment: Text.AlignHCenter }
                            Text { text: "Days"; color: "#475569"; font.pixelSize: 11; font.bold: true; width: 40; horizontalAlignment: Text.AlignHCenter }
                            Text { text: "Interest Amt (₹)"; color: "#15803D"; font.pixelSize: 11; font.bold: true; Layout.fillWidth: true; horizontalAlignment: Text.AlignRight }
                        }
                    }

                    // Credit Rows ListView
                    ListView {
                        id: crListView
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        model: crItemsModel
                        clip: true
                        spacing: 2
                        boundsBehavior: Flickable.StopAtBounds

                        delegate: Rectangle {
                            width: crListView.width
                            height: 32
                            color: index % 2 === 0 ? "#FFFFFF" : "#F8FAFC"
                            border.color: "#E2E8F0"
                            radius: 4

                            RowLayout {
                                anchors.fill: parent
                                anchors.leftMargin: 6
                                anchors.rightMargin: 6
                                spacing: 4

                                Text { text: model.vDate; color: "#334155"; font.pixelSize: 11; width: 75 }
                                Text { text: (typeof dashboardCtrl !== "undefined" && dashboardCtrl) ? dashboardCtrl.format_inr(model.amount) : model.amount.toFixed(2); color: "#0F172A"; font.pixelSize: 11; font.bold: true; width: 110; horizontalAlignment: Text.AlignRight }
                                
                                // Rate Input per row
                                Rectangle {
                                    width: 36
                                    height: 22
                                    color: "#F1F5F9"
                                    border.color: "#CBD5E1"
                                    radius: 3
                                    TextInput {
                                        anchors.centerIn: parent
                                        text: model.rate ? model.rate.toString() : "12"
                                        font.pixelSize: 11
                                        color: "#0F172A"
                                        horizontalAlignment: Text.AlignHCenter
                                        onEditingFinished: {
                                            crItemsModel.setProperty(index, "rate", parseFloat(text) || 12.0)
                                            root.recalcAllTotals()
                                        }
                                    }
                                }

                                // Due Days (D) input per row
                                Rectangle {
                                    width: 50
                                    height: 22
                                    color: model.dueDays > 0 ? "#FEF3C7" : "#FFFFFF"
                                    border.color: model.dueDays > 0 ? "#D97706" : "#CBD5E1"
                                    border.width: model.dueDays > 0 ? 1.5 : 1
                                    radius: 3

                                    TextInput {
                                        anchors.centerIn: parent
                                        text: model.dueDays ? model.dueDays.toString() : "0"
                                        font.pixelSize: 11
                                        font.bold: model.dueDays > 0
                                        color: model.dueDays > 0 ? "#92400E" : "#475569"
                                        horizontalAlignment: Text.AlignHCenter
                                        onEditingFinished: {
                                            crItemsModel.setProperty(index, "dueDays", parseInt(text) || 0)
                                            root.recalcAllTotals()
                                        }
                                    }
                                }

                                Text { text: model.days.toString(); color: "#475569"; font.pixelSize: 11; width: 40; horizontalAlignment: Text.AlignHCenter }
                                Text { text: (typeof dashboardCtrl !== "undefined" && dashboardCtrl) ? dashboardCtrl.format_inr(model.interestAmt) : model.interestAmt.toFixed(2); color: "#15803D"; font.pixelSize: 11; font.bold: true; Layout.fillWidth: true; horizontalAlignment: Text.AlignRight }
                            }
                        }
                    }

                    // Cr Subtotals
                    Rectangle {
                        Layout.fillWidth: true
                        height: 32
                        color: "#DCFCE7"
                        radius: 6

                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: 8
                            anchors.rightMargin: 8
                            Text { text: "Cr Total: " + root.formatInr(root.crTotalAmount); color: "#166534"; font.pixelSize: 12; font.bold: true }
                            Item { Layout.fillWidth: true }
                            Text { text: "Cr Interest: " + root.formatInr(root.crTotalInterest); color: "#15803D"; font.pixelSize: 13; font.bold: true }
                        }
                    }
                }
            }

            // ==================== RIGHT COLUMN: DEBIT SIDE (NAME) ====================
            Rectangle {
                Layout.fillWidth: true
                Layout.fillHeight: true
                color: "#FFFFFF"
                border.color: "#CBD5E1"
                radius: 8

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 8
                    spacing: 6

                    // Side Header Bar with Global Rate
                    Rectangle {
                        Layout.fillWidth: true
                        height: 36
                        color: "#DBEAFE"
                        radius: 6

                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: 10
                            anchors.rightMargin: 10
                            spacing: 8

                            Text {
                                text: "🔵 Dr. Side (Name)"
                                color: "#1E40AF"
                                font.pixelSize: 13
                                font.bold: true
                            }
                            KbdBadge { text: "Alt+B"; badgeColor: "#BFDBFE"; textColor: "#1E3A8A"; borderColor: "#93C5FD" }

                            Item { Layout.fillWidth: true }

                            Text {
                                text: "Interest Rate (Annual) %:"
                                font.pixelSize: 11
                                font.bold: true
                                color: "#1E40AF"
                            }

                            Rectangle {
                                width: 50
                                height: 26
                                color: "#FFFFFF"
                                border.color: "#93C5FD"
                                radius: 4

                                TextInput {
                                    id: drRateInput
                                    anchors.centerIn: parent
                                    width: 42
                                    text: root.globalDrRate.toString()
                                    font.pixelSize: 12
                                    font.bold: true
                                    color: "#1E3A8A"
                                    horizontalAlignment: Text.AlignHCenter
                                    onEditingFinished: {
                                        var val = parseFloat(text) || 12.0
                                        root.applyGlobalDrRate(val)
                                    }
                                }
                            }
                        }
                    }

                    // Table Columns Header
                    Rectangle {
                        Layout.fillWidth: true
                        height: 26
                        color: "#F8FAFC"
                        radius: 4
                        border.color: "#E2E8F0"

                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: 6
                            anchors.rightMargin: 6
                            spacing: 4

                            Text { text: "Dr.Date"; color: "#475569"; font.pixelSize: 11; font.bold: true; width: 75 }
                            Text { text: "Dr.Amount (" + drItemsModel.count + ")"; color: "#475569"; font.pixelSize: 11; font.bold: true; width: 110; horizontalAlignment: Text.AlignRight }
                            Text { text: "Rate"; color: "#475569"; font.pixelSize: 11; font.bold: true; width: 36; horizontalAlignment: Text.AlignHCenter }
                            Text { text: "D (Grace)"; color: "#B45309"; font.pixelSize: 11; font.bold: true; width: 50; horizontalAlignment: Text.AlignHCenter }
                            Text { text: "Days"; color: "#475569"; font.pixelSize: 11; font.bold: true; width: 40; horizontalAlignment: Text.AlignHCenter }
                            Text { text: "Interest Amt (₹)"; color: "#2563EB"; font.pixelSize: 11; font.bold: true; Layout.fillWidth: true; horizontalAlignment: Text.AlignRight }
                        }
                    }

                    // Debit Rows ListView
                    ListView {
                        id: drListView
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        model: drItemsModel
                        clip: true
                        spacing: 2
                        boundsBehavior: Flickable.StopAtBounds

                        delegate: Rectangle {
                            width: drListView.width
                            height: 32
                            color: index % 2 === 0 ? "#FFFFFF" : "#F8FAFC"
                            border.color: "#E2E8F0"
                            radius: 4

                            RowLayout {
                                anchors.fill: parent
                                anchors.leftMargin: 6
                                anchors.rightMargin: 6
                                spacing: 4

                                Text { text: model.vDate; color: "#334155"; font.pixelSize: 11; width: 75 }
                                Text { text: (typeof dashboardCtrl !== "undefined" && dashboardCtrl) ? dashboardCtrl.format_inr(model.amount) : model.amount.toFixed(2); color: "#0F172A"; font.pixelSize: 11; font.bold: true; width: 110; horizontalAlignment: Text.AlignRight }
                                
                                // Rate Input per row
                                Rectangle {
                                    width: 36
                                    height: 22
                                    color: "#F1F5F9"
                                    border.color: "#CBD5E1"
                                    radius: 3
                                    TextInput {
                                        anchors.centerIn: parent
                                        text: model.rate ? model.rate.toString() : "12"
                                        font.pixelSize: 11
                                        color: "#0F172A"
                                        horizontalAlignment: Text.AlignHCenter
                                        onEditingFinished: {
                                            drItemsModel.setProperty(index, "rate", parseFloat(text) || 12.0)
                                            root.recalcAllTotals()
                                        }
                                    }
                                }

                                // Due Days (D) input per row
                                Rectangle {
                                    width: 50
                                    height: 22
                                    color: model.dueDays > 0 ? "#FEF3C7" : "#FFFFFF"
                                    border.color: model.dueDays > 0 ? "#D97706" : "#CBD5E1"
                                    border.width: model.dueDays > 0 ? 1.5 : 1
                                    radius: 3

                                    TextInput {
                                        anchors.centerIn: parent
                                        text: model.dueDays ? model.dueDays.toString() : "0"
                                        font.pixelSize: 11
                                        font.bold: model.dueDays > 0
                                        color: model.dueDays > 0 ? "#92400E" : "#475569"
                                        horizontalAlignment: Text.AlignHCenter
                                        onEditingFinished: {
                                            drItemsModel.setProperty(index, "dueDays", parseInt(text) || 0)
                                            root.recalcAllTotals()
                                        }
                                    }
                                }

                                Text { text: model.days.toString(); color: "#475569"; font.pixelSize: 11; width: 40; horizontalAlignment: Text.AlignHCenter }
                                Text { text: (typeof dashboardCtrl !== "undefined" && dashboardCtrl) ? dashboardCtrl.format_inr(model.interestAmt) : model.interestAmt.toFixed(2); color: "#2563EB"; font.pixelSize: 11; font.bold: true; Layout.fillWidth: true; horizontalAlignment: Text.AlignRight }
                            }
                        }
                    }

                    // Dr Subtotals
                    Rectangle {
                        Layout.fillWidth: true
                        height: 32
                        color: "#DBEAFE"
                        radius: 6

                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: 8
                            anchors.rightMargin: 8
                            Text { text: "Dr Total: " + root.formatInr(root.drTotalAmount); color: "#1E3A8A"; font.pixelSize: 12; font.bold: true }
                            Item { Layout.fillWidth: true }
                            Text { text: "Dr Interest: " + root.formatInr(root.drTotalInterest); color: "#1D4ED8"; font.pixelSize: 13; font.bold: true }
                        }
                    }
                }
            }
        }

        // 3. PINNED BOTTOM SUMMARY RIBBON
        Rectangle {
            Layout.fillWidth: true
            height: 52
            color: "#0F172A"
            radius: 8

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 16
                anchors.rightMargin: 16
                spacing: 16

                // Net Interest Receivable / Payable
                ColumnLayout {
                    spacing: 2
                    Text { text: "NET INTEREST RESULT"; color: "#94A3B8"; font.pixelSize: 10; font.bold: true; font.letterSpacing: 0.8 }
                    Text {
                        text: (root.netInterestType.indexOf("Receivable") !== -1 ? "Net Interest Receivable: " : "Net Interest Payable: ") + root.formatInr(root.netInterest)
                        color: "#FACC15"
                        font.pixelSize: 14
                        font.bold: true
                    }
                }

                Item { Layout.fillWidth: true }

                // Summary Pills: (Bal = ... | Intt = ... | Net Bal = ...)
                RowLayout {
                    spacing: 10

                    Rectangle {
                        height: 30
                        implicitWidth: balTxt.implicitWidth + 16
                        color: "#1E293B"
                        radius: 6
                        border.color: "#334155"
                        Text {
                            id: balTxt
                            anchors.centerIn: parent
                            text: "Bal. = " + root.formatInr(root.ledgerBalance) + " " + root.ledgerBalanceType
                            color: "#94A3B8"
                            font.pixelSize: 11
                            font.bold: true
                        }
                    }

                    Rectangle {
                        height: 30
                        implicitWidth: intTxt.implicitWidth + 16
                        color: "#1E293B"
                        radius: 6
                        border.color: "#334155"
                        Text {
                            id: intTxt
                            anchors.centerIn: parent
                            text: "Intt. = " + root.formatInr(root.netInterest) + " " + (root.netInterestType.indexOf("Receivable") !== -1 ? "Dr" : "Cr")
                            color: "#FDE047"
                            font.pixelSize: 11
                            font.bold: true
                        }
                    }

                    Rectangle {
                        height: 32
                        implicitWidth: netBalTxt.implicitWidth + 20
                        color: root.finalNetBalanceType === "Dr" ? "#1E3A8A" : "#065F46"
                        radius: 6
                        border.color: root.finalNetBalanceType === "Dr" ? "#60A5FA" : "#34D399"
                        Text {
                            id: netBalTxt
                            anchors.centerIn: parent
                            text: "Net Bal. = " + root.formatInr(root.finalNetBalance) + " " + root.finalNetBalanceType
                            color: "#FFFFFF"
                            font.pixelSize: 12
                            font.bold: true
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

            Text { text: "🗓️ SET HISAB / SETTLEMENT DATE"; font.bold: true; color: "#0F172A"; font.pixelSize: 13 }
            
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
                    root.statusMessage = "✅ Interest Voucher posted successfully!"
                    root.isError = false
                    root.interestVoucherSaved()
                } else {
                    root.statusMessage = "❌ Failed to post Interest Voucher."
                    root.isError = true
                }
            }
        }
    }

    // SHORTCUTS
    Shortcut { sequence: "Alt+R"; onActivated: crRateInput.forceActiveFocus() }
    Shortcut { sequence: "Alt+B"; onActivated: drRateInput.forceActiveFocus() }
    Shortcut { sequence: "F6"; onActivated: hisabDatePopup.open() }
    Shortcut { sequence: "Shift+Return"; onActivated: root.loadInterestStatement(root.currentPartyName) }
    Shortcut { sequence: "Ctrl+Return"; onActivated: postConfirmModal.open() }
    Shortcut { sequence: "Escape"; onActivated: root.cancelRequested() }
}
