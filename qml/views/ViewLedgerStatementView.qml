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
        partySearch.focusInput = true
        loadPartyStatement("")
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

        if (!pName) {
            applyDateFilterAndSort()
            return
        }

        if (pName.indexOf("Ramesh") !== -1) {
            drMasterModel.append({ isSelected: false, vDate: "10-08-2026", refNo: "SLIP-1001", particulars: "Paddy Arrival (Sona Masoori 112 Qtl)", amount: 275150.0 })
            crMasterModel.append({ isSelected: false, vDate: "15-08-2026", refNo: "VCH-9001", particulars: "HDFC Bank - Payment for Paddy Slip-1001", amount: 275150.0 })
        } else if (pName.indexOf("Suresh") !== -1) {
            drMasterModel.append({ isSelected: false, vDate: "24-08-2026", refNo: "SLIP-1002", particulars: "Paddy Arrival (IR64 147 Qtl)", amount: 317050.0 })
        } else if (pName.indexOf("Balaji") !== -1) {
            drMasterModel.append({ isSelected: false, vDate: "01-04-2026", refNo: "OP-BAL", particulars: "Opening Balance (Dr)", amount: 82000.0 })
            drMasterModel.append({ isSelected: false, vDate: "20-08-2026", refNo: "INV-5004", particulars: "Sona Masoori Steam Rice Grade-A", amount: 250000.0 })
            crMasterModel.append({ isSelected: false, vDate: "22-08-2026", refNo: "VCH-9004", particulars: "Bank Receipt - ICICI Bank", amount: 150000.0 })
        } else if (pName.indexOf("Golden") !== -1) {
            drMasterModel.append({ isSelected: false, vDate: "24-08-2026", refNo: "INV-5002", particulars: "Rice Bran (16% Oil) 60 Qtl", amount: 151200.0 })
            crMasterModel.append({ isSelected: false, vDate: "24-08-2026", refNo: "VCH-9002", particulars: "Cash Receipt against INV-5002", amount: 151200.0 })
        } else {
            drMasterModel.append({ isSelected: false, vDate: "01-04-2026", refNo: "OP-BAL", particulars: "Opening Balance (Dr)", amount: 45000.0 })
            drMasterModel.append({ isSelected: false, vDate: "18-08-2026", refNo: "INV-5001", particulars: "Sona Masoori Steam Rice Grade-A (200 Bags)", amount: 404250.0 })
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

        // 2. FILTER & PARTY SELECTION BAR (DD-MM-YYYY FORMAT)
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

                AutoCompletePartySearch {
                    id: partySearch
                    implicitWidth: 300
                    implicitHeight: 34
                    onPartySelected: function(party) {
                        root.loadPartyStatement(party.name)
                    }
                }

                Text { text: "From Date:"; color: "#0F172A"; font.pixelSize: 12; font.bold: true }
                Rectangle {
                    width: 120; height: 34; radius: 6; color: "#FFFFFF"
                    border.color: fromDateInput.activeFocus ? "#2563EB" : "#CBD5E1"
                    border.width: fromDateInput.activeFocus ? 2 : 1

                    TextField {
                        id: fromDateInput
                        anchors.fill: parent
                        anchors.leftMargin: 8
                        anchors.rightMargin: 8
                        text: "01-04-2026"
                        color: "#0F172A"
                        font.pixelSize: 12
                        font.bold: true
                        font.family: "Menlo, Consolas, sans-serif"
                        background: null
                        selectByMouse: true
                        onTextChanged: root.applyDateFilterAndSort()
                    }
                }

                Text { text: "To Date:"; color: "#0F172A"; font.pixelSize: 12; font.bold: true }
                Rectangle {
                    width: 120; height: 34; radius: 6; color: "#FFFFFF"
                    border.color: toDateInput.activeFocus ? "#2563EB" : "#CBD5E1"
                    border.width: toDateInput.activeFocus ? 2 : 1

                    TextField {
                        id: toDateInput
                        anchors.fill: parent
                        anchors.leftMargin: 8
                        anchors.rightMargin: 8
                        text: "25-08-2026"
                        color: "#0F172A"
                        font.pixelSize: 12
                        font.bold: true
                        font.family: "Menlo, Consolas, sans-serif"
                        background: null
                        selectByMouse: true
                        onTextChanged: root.applyDateFilterAndSort()
                    }
                }

                Item { Layout.fillWidth: true }

                Button {
                    height: 32
                    background: Rectangle { color: "#2563EB"; radius: 6 }
                    contentItem: Text { text: "⚡ Auto Sorted"; color: "#FFF"; font.bold: true; font.pixelSize: 12 }
                    onClicked: root.applyDateFilterAndSort()
                }

                Button {
                    height: 32
                    background: Rectangle { color: "#F1F5F9"; radius: 6; border.color: "#CBD5E1" }
                    contentItem: Text { text: "🖨️ Print PDF"; color: "#475569"; font.bold: true; font.pixelSize: 12 }
                }
            }
        }

        // 3. MAIN 2-COLUMN SCROLLABLE GRID SECTION
        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 12

            // ==================== LEFT COLUMN: DEBIT SIDE (JAMA / Dr) ====================
            Rectangle {
                Layout.fillWidth: true
                Layout.fillHeight: true
                color: "#FFFFFF"
                border.color: "#3B82F6"
                border.width: 2
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
                            Text { text: "🔴 DEBIT SIDE (JAMA / Dr) - " + root.currentPartyName; color: "#1D4ED8"; font.pixelSize: 12; font.bold: true; elide: Text.ElideRight; Layout.fillWidth: true }
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

                        delegate: Rectangle {
                            width: drListView.width
                            height: 36
                            color: index % 2 === 0 ? "#FFFFFF" : "#F8FAFC"
                            border.color: "#E2E8F0"
                            radius: 4

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
                                            drListModel.setProperty(index, "isSelected", !model.isSelected)
                                            root.recalcTotals()
                                        }
                                    }
                                }

                                Text { text: model.vDate; color: "#334155"; font.pixelSize: 11; font.family: "Menlo, Consolas, sans-serif"; width: 85 }
                                Text { text: model.refNo; color: "#2563EB"; font.pixelSize: 11; font.bold: true; font.family: "Menlo, Consolas, sans-serif"; width: 75 }
                                Text { text: model.particulars; color: "#0F172A"; font.pixelSize: 11; Layout.fillWidth: true; elide: Text.ElideRight }
                                Text { text: "₹" + model.amount.toLocaleString(Qt.locale(), 'f', 2); color: "#1D4ED8"; font.pixelSize: 12; font.bold: true; horizontalAlignment: Text.AlignRight; width: 95 }
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
                            Text { text: "₹" + root.drTotalAll.toLocaleString(Qt.locale(), 'f', 2); color: "#0F172A"; font.pixelSize: 13; font.bold: true }
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
                                Text { text: "₹" + root.drSelectedTotal.toLocaleString(Qt.locale(), 'f', 2); color: "#1D4ED8"; font.pixelSize: 12; font.bold: true }
                            }
                        }
                    }
                }
            }

            // ==================== RIGHT COLUMN: CREDIT SIDE (NAMA / Cr) ====================
            Rectangle {
                Layout.fillWidth: true
                Layout.fillHeight: true
                color: "#FFFFFF"
                border.color: "#16A34A"
                border.width: 2
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
                            Text { text: "🟢 CREDIT SIDE (NAMA / Cr) - " + root.currentPartyName; color: "#15803D"; font.pixelSize: 12; font.bold: true; elide: Text.ElideRight; Layout.fillWidth: true }
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

                        delegate: Rectangle {
                            width: crListView.width
                            height: 36
                            color: index % 2 === 0 ? "#FFFFFF" : "#F8FAFC"
                            border.color: "#E2E8F0"
                            radius: 4

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
                                            crListModel.setProperty(index, "isSelected", !model.isSelected)
                                            root.recalcTotals()
                                        }
                                    }
                                }

                                Text { text: model.vDate; color: "#334155"; font.pixelSize: 11; font.family: "Menlo, Consolas, sans-serif"; width: 85 }
                                Text { text: model.refNo; color: "#16A34A"; font.pixelSize: 11; font.bold: true; font.family: "Menlo, Consolas, sans-serif"; width: 75 }
                                Text { text: model.particulars; color: "#0F172A"; font.pixelSize: 11; Layout.fillWidth: true; elide: Text.ElideRight }
                                Text { text: "₹" + model.amount.toLocaleString(Qt.locale(), 'f', 2); color: "#15803D"; font.pixelSize: 12; font.bold: true; horizontalAlignment: Text.AlignRight; width: 95 }
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
                            Text { text: "₹" + root.crTotalAll.toLocaleString(Qt.locale(), 'f', 2); color: "#0F172A"; font.pixelSize: 13; font.bold: true }
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
                                Text { text: "₹" + root.crSelectedTotal.toLocaleString(Qt.locale(), 'f', 2); color: "#15803D"; font.pixelSize: 12; font.bold: true }
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
                        text: "Selected Dr: ₹" + root.drSelectedTotal.toLocaleString(Qt.locale(), 'f', 2) + "  |  Selected Cr: ₹" + root.crSelectedTotal.toLocaleString(Qt.locale(), 'f', 2)
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
                            text: "₹" + root.netBalance.toLocaleString(Qt.locale(), 'f', 2)
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
}
