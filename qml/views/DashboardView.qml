import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../components"

ScrollView {
    id: root
    contentWidth: availableWidth
    focus: true

    signal openNewPaddy()
    signal openNewMilling()
    signal openNewSale()
    signal openNewVoucher()
    signal openLedgers()
    signal openStock()
    signal openPaddy()
    signal openLedgerMenu()
    signal openStockMenu()
    signal openAddVoucherMenu()
    signal openOtherVoucherMenu()
    signal openReportsMenu()
    signal openPeriodModal()

    property int selectedMenuIndex: 0
    property string activePeriodText: ""

    Component.onCompleted: {
        root.forceActiveFocus()
        loadDashboardStats()
    }

    function loadDashboardStats() {
        if (typeof stockItemsModel !== "undefined" && stockItemsModel) {
            var fy = stockItemsModel.get_financial_year()
            var sd = stockItemsModel.get_from_date()
            var ed = stockItemsModel.get_to_date()
            var s_fmt = sd.indexOf("-") !== -1 ? sd.split("-").reverse().join("-") : sd
            var e_fmt = ed.indexOf("-") !== -1 ? ed.split("-").reverse().join("-") : ed
            if (s_fmt && e_fmt) {
                root.activePeriodText = s_fmt + " To " + e_fmt + " (" + fy + ")"
            } else if (fy) {
                root.activePeriodText = fy
            }
        }
        if (typeof dashboardCtrl !== "undefined" && dashboardCtrl) {
            dashboardCtrl.refresh_stats()
        }
    }

    Keys.onUpPressed: function(event) {
        event.accepted = true
        if (selectedMenuIndex > 0) {
            selectedMenuIndex--
        } else {
            selectedMenuIndex = 4
        }
    }

    Keys.onDownPressed: function(event) {
        event.accepted = true
        if (selectedMenuIndex < 4) {
            selectedMenuIndex++
        } else {
            selectedMenuIndex = 0
        }
    }

    Keys.onReturnPressed: function(event) {
        event.accepted = true
        triggerSelectedMenu()
    }

    Keys.onEnterPressed: function(event) {
        event.accepted = true
        triggerSelectedMenu()
    }

    function triggerSelectedMenu() {
        if (selectedMenuIndex === 0) {
            root.openLedgerMenu()
        } else if (selectedMenuIndex === 1) {
            root.openStockMenu()
        } else if (selectedMenuIndex === 2) {
            root.openAddVoucherMenu()
        } else if (selectedMenuIndex === 3) {
            root.openOtherVoucherMenu()
        } else if (selectedMenuIndex === 4) {
            root.openReportsMenu()
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 14
        spacing: 14

        // Top Action & Header Bar
        RowLayout {
            Layout.fillWidth: true
            spacing: 12

            ColumnLayout {
                spacing: 2
                Text {
                    text: "Executive Accounting Dashboard"
                    color: "#0F172A"
                    font.pixelSize: 22
                    font.bold: true
                }
                Text {
                    text: "Mahadev Rice Milling ERP & Financial Control System"
                    color: "#64748B"
                    font.pixelSize: 12
                }
            }

            Item { Layout.fillWidth: true }

            RowLayout {
                spacing: 8

                // Bahi-Khata Financial Year / Accounting Period Selector
                Button {
                    id: btnPeriod
                    background: Rectangle {
                        color: "#F0FDF4"
                        border.color: "#16A34A"
                        border.width: 1.5
                        radius: 6
                    }
                    contentItem: RowLayout {
                        spacing: 6
                        Text { text: "📅 Period:"; color: "#166534"; font.pixelSize: 11; font.bold: true }
                        Text { text: root.activePeriodText; color: "#15803D"; font.pixelSize: 12; font.bold: true }
                        Rectangle {
                            width: 16; height: 16; radius: 8; color: "#16A34A"
                            Text { anchors.centerIn: parent; text: "⚙"; color: "#FFF"; font.pixelSize: 9 }
                        }
                    }
                    onClicked: root.openPeriodModal()
                }

                Button {
                    id: btnPaddy
                    background: Rectangle { color: "#16A34A"; radius: 6 }
                    contentItem: RowLayout {
                        spacing: 6
                        Text { text: "🌾 New Paddy Slip"; color: "#FFF"; font.bold: true; font.pixelSize: 12 }
                        KbdBadge { text: "F2"; badgeColor: "#14532D"; textColor: "#86EFAC"; borderColor: "#16A34A" }
                    }
                    onClicked: root.openNewPaddy()
                }

                Button {
                    id: btnSale
                    background: Rectangle { color: "#2563EB"; radius: 6 }
                    contentItem: RowLayout {
                        spacing: 6
                        Text { text: "🧾 New Invoice"; color: "#FFF"; font.bold: true; font.pixelSize: 12 }
                    }
                    onClicked: root.openNewSale()
                }
            }
        }

        // 3-SECTION HORIZONTAL LAYOUT
        // Ratio: Left = 1 (16.7%), Middle = 4 (66.7% = 2/3rd), Right = 1 (16.7%)
        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 14

            // ==============================================================
            // LEFT SECTION (1/6th Width): FIRM INFO, PAN, GST, FY SELECTED
            // ==============================================================
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredWidth: 1
                Layout.fillHeight: true
                color: "#FFFFFF"
                border.color: "#E2E8F0"
                border.width: 1
                radius: 10

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 14
                    spacing: 12

                    RowLayout {
                        spacing: 8
                        Rectangle {
                            width: 34
                            height: 34
                            radius: 8
                            color: "#EFF6FF"
                            border.color: "#BFDBFE"
                            Text { anchors.centerIn: parent; text: "🏛️"; font.pixelSize: 18 }
                        }
                        ColumnLayout {
                            spacing: 0
                            Text { text: "FIRM PROFILE"; color: "#2563EB"; font.pixelSize: 10; font.bold: true; font.letterSpacing: 1.0 }
                            Text { text: "Mahadev Rice Mill"; color: "#0F172A"; font.pixelSize: 14; font.bold: true; elide: Text.ElideRight }
                        }
                    }

                    Rectangle { Layout.fillWidth: true; height: 1; color: "#F1F5F9" }

                    // Details List
                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 10

                        // Financial Year Selected Badge
                        Rectangle {
                            Layout.fillWidth: true
                            height: 46
                            radius: 8
                            color: "#F0FDF4"
                            border.color: "#BBF7D0"

                            RowLayout {
                                anchors.fill: parent
                                anchors.leftMargin: 10
                                anchors.rightMargin: 10
                                spacing: 8
                                Text { text: "📅"; font.pixelSize: 16 }
                                ColumnLayout {
                                    spacing: 0
                                    Text { text: "Financial Year Selected"; color: "#166534"; font.pixelSize: 10 }
                                    Text { text: "FY 2026-2027 (Active)"; color: "#15803D"; font.pixelSize: 12; font.bold: true }
                                }
                            }
                        }

                        // GSTIN
                        ColumnLayout {
                            spacing: 2
                            Text { text: "GSTIN Number:"; color: "#64748B"; font.pixelSize: 11 }
                            Text { text: "29AAACM8899F1Z4"; color: "#0F172A"; font.pixelSize: 12; font.bold: true; font.family: "Segoe UI, Consolas, Menlo, sans-serif" }
                        }

                        // PAN
                        ColumnLayout {
                            spacing: 2
                            Text { text: "PAN Number:"; color: "#64748B"; font.pixelSize: 11 }
                            Text { text: "AAACM8899F"; color: "#0F172A"; font.pixelSize: 12; font.bold: true; font.family: "Segoe UI, Consolas, Menlo, sans-serif" }
                        }

                        // FSSAI
                        ColumnLayout {
                            spacing: 2
                            Text { text: "FSSAI License:"; color: "#64748B"; font.pixelSize: 11 }
                            Text { text: "11223344556677"; color: "#0F172A"; font.pixelSize: 12; font.bold: true; font.family: "Segoe UI, Consolas, Menlo, sans-serif" }
                        }

                        // Business Type
                        ColumnLayout {
                            spacing: 2
                            Text { text: "Business Type:"; color: "#64748B"; font.pixelSize: 11 }
                            Text { text: "Paddy Milling & Grain ERP"; color: "#0F172A"; font.pixelSize: 12; font.bold: true }
                        }

                        // Location
                        ColumnLayout {
                            spacing: 2
                            Text { text: "Location:"; color: "#64748B"; font.pixelSize: 11 }
                            Text { text: "Raichur, Karnataka"; color: "#0F172A"; font.pixelSize: 12; font.bold: true }
                        }
                    }

                    Item { Layout.fillHeight: true }

                    // Status Badge
                    Rectangle {
                        Layout.fillWidth: true
                        height: 30
                        radius: 6
                        color: "#F8FAFC"
                        border.color: "#E2E8F0"

                        RowLayout {
                            anchors.centerIn: parent
                            spacing: 6
                            Rectangle { width: 8; height: 8; radius: 4; color: "#16A34A" }
                            Text { text: "GST Tax Registered"; color: "#334155"; font.pixelSize: 11; font.bold: true }
                        }
                    }
                }
            }

            // ==============================================================
            // MIDDLE SECTION (4/6th = 2/3rd Width): VERTICAL UP-TO-DOWN MENU & METRICS
            // ==============================================================
            ColumnLayout {
                Layout.fillWidth: true
                Layout.preferredWidth: 4
                Layout.fillHeight: true
                spacing: 14

                // Top Metrics Cards Strip
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 12

                    StatCard {
                        title: "Raw Paddy Stock"
                        value: (typeof dashboardCtrl !== "undefined" && dashboardCtrl) ? dashboardCtrl.paddyStock : "0.0 Qtl"
                        subtext: "In Godowns A & B"
                        icon: "🌾"
                        accentColor: "#D97706"
                        Layout.fillWidth: true
                    }

                    StatCard {
                        title: "Finished Rice Stock"
                        value: (typeof dashboardCtrl !== "undefined" && dashboardCtrl) ? dashboardCtrl.riceStock : "0.0 Qtl"
                        subtext: "Ready for Dispatch"
                        icon: "🍚"
                        accentColor: "#16A34A"
                        Layout.fillWidth: true
                    }

                    StatCard {
                        title: "Total Revenue"
                        value: (typeof dashboardCtrl !== "undefined" && dashboardCtrl) ? dashboardCtrl.totalSales : "₹0.0"
                        subtext: "Sales Invoices"
                        icon: "🧾"
                        accentColor: "#2563EB"
                        Layout.fillWidth: true
                    }
                }

                // VERTICAL UP-TO-DOWN MENU SECTION (STACKED COLUMN)
                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 10

                    Text {
                        text: "MASTER & VOUCHER MENU (Use ↑ / ↓ Arrow Keys & Press Enter)"
                        color: "#2563EB"
                        font.pixelSize: 11
                        font.bold: true
                        font.letterSpacing: 1.0
                    }

                    // 1. Ledger Master
                    NavMenuItem {
                        id: cardLedger
                        index: 0
                        selectedIndex: root.selectedMenuIndex
                        itemHeight: 54
                        activeColor: "#EFF6FF"
                        activeBorderColor: "#2563EB"
                        normalColor: "#FFFFFF"
                        normalBorderColor: "#E2E8F0"
                        onItemHovered: root.selectedMenuIndex = 0
                        onItemClicked: { root.selectedMenuIndex = 0; root.openLedgerMenu() }

                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: 14
                            anchors.rightMargin: 14
                            spacing: 12

                            Rectangle {
                                width: 36; height: 36; radius: 8; color: "#DBEAFE"
                                Text { anchors.centerIn: parent; text: "📖"; font.pixelSize: 18 }
                            }

                            ColumnLayout {
                                spacing: 1
                                Layout.fillWidth: true
                                Text { text: "1. Ledger Master"; color: "#0F172A"; font.pixelSize: 13; font.bold: true }
                                Text { text: "New Ledger, Modify, View Ledger, Groups"; color: "#64748B"; font.pixelSize: 11 }
                            }

                            KbdBadge { text: "Alt+6"; badgeColor: "#EFF6FF"; textColor: "#2563EB"; borderColor: "#BFDBFE" }
                        }
                    }

                    // 2. Stock Master
                    NavMenuItem {
                        id: cardStock
                        index: 1
                        selectedIndex: root.selectedMenuIndex
                        itemHeight: 54
                        activeColor: "#F0FDF4"
                        activeBorderColor: "#16A34A"
                        normalColor: "#FFFFFF"
                        normalBorderColor: "#E2E8F0"
                        onItemHovered: root.selectedMenuIndex = 1
                        onItemClicked: { root.selectedMenuIndex = 1; root.openStockMenu() }

                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: 14
                            anchors.rightMargin: 14
                            spacing: 12

                            Rectangle {
                                width: 36; height: 36; radius: 8; color: "#DCFCE7"
                                Text { anchors.centerIn: parent; text: "📦"; font.pixelSize: 18 }
                            }

                            ColumnLayout {
                                spacing: 1
                                Layout.fillWidth: true
                                Text { text: "2. Stock Master"; color: "#0F172A"; font.pixelSize: 13; font.bold: true }
                                Text { text: "Raw Paddy, Rice & By-Product Inventory"; color: "#64748B"; font.pixelSize: 11 }
                            }

                            KbdBadge { text: "Alt+4"; badgeColor: "#F0FDF4"; textColor: "#16A34A"; borderColor: "#BBF7D0" }
                        }
                    }

                    // 3. Add Vouchers
                    NavMenuItem {
                        id: cardAddVch
                        index: 2
                        selectedIndex: root.selectedMenuIndex
                        itemHeight: 54
                        activeColor: "#FEF3C7"
                        activeBorderColor: "#D97706"
                        normalColor: "#FFFFFF"
                        normalBorderColor: "#E2E8F0"
                        onItemHovered: root.selectedMenuIndex = 2
                        onItemClicked: { root.selectedMenuIndex = 2; root.openAddVoucherMenu() }

                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: 14
                            anchors.rightMargin: 14
                            spacing: 12

                            Rectangle {
                                width: 36; height: 36; radius: 8; color: "#FEF3C7"
                                Text { anchors.centerIn: parent; text: "📝"; font.pixelSize: 18 }
                            }

                            ColumnLayout {
                                spacing: 1
                                Layout.fillWidth: true
                                Text { text: "3. Add Vouchers"; color: "#0F172A"; font.pixelSize: 13; font.bold: true }
                                Text { text: "Sales Invoices, Paddy Slips, Journal & Milling"; color: "#64748B"; font.pixelSize: 11 }
                            }

                            KbdBadge { text: "F2"; badgeColor: "#FEF3C7"; textColor: "#D97706"; borderColor: "#FDE68A" }
                        }
                    }

                    // 4. Other Vouchers
                    NavMenuItem {
                        id: cardMoreVch
                        index: 3
                        selectedIndex: root.selectedMenuIndex
                        itemHeight: 54
                        activeColor: "#F3E8FF"
                        activeBorderColor: "#7C3AED"
                        normalColor: "#FFFFFF"
                        normalBorderColor: "#E2E8F0"
                        onItemHovered: root.selectedMenuIndex = 3
                        onItemClicked: { root.selectedMenuIndex = 3; root.openOtherVoucherMenu() }

                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: 14
                            anchors.rightMargin: 14
                            spacing: 12

                            Rectangle {
                                width: 36; height: 36; radius: 8; color: "#F3E8FF"
                                Text { anchors.centerIn: parent; text: "📑"; font.pixelSize: 18 }
                            }

                            ColumnLayout {
                                spacing: 1
                                Layout.fillWidth: true
                                Text { text: "4. Other Vouchers"; color: "#0F172A"; font.pixelSize: 13; font.bold: true }
                                Text { text: "J-Form Mandi Procurement, Journal & Milling Vouchers"; color: "#64748B"; font.pixelSize: 11 }
                            }

                            KbdBadge { text: "Alt+5"; badgeColor: "#F3E8FF"; textColor: "#7C3AED"; borderColor: "#E9D5FF" }
                        }
                    }

                    // 5. Reports & Statements
                    NavMenuItem {
                        id: cardReports
                        index: 4
                        selectedIndex: root.selectedMenuIndex
                        itemHeight: 54
                        activeColor: "#ECFDF5"
                        activeBorderColor: "#059669"
                        normalColor: "#FFFFFF"
                        normalBorderColor: "#E2E8F0"
                        onItemHovered: root.selectedMenuIndex = 4
                        onItemClicked: { root.selectedMenuIndex = 4; root.openReportsMenu() }

                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: 14
                            anchors.rightMargin: 14
                            spacing: 12

                            Rectangle {
                                width: 36; height: 36; radius: 8; color: "#DCFCE7"
                                Text { anchors.centerIn: parent; text: "📊"; font.pixelSize: 18 }
                            }

                            ColumnLayout {
                                spacing: 1
                                Layout.fillWidth: true
                                Text { text: "5. Reports & Statements"; color: "#0F172A"; font.pixelSize: 13; font.bold: true }
                                Text { text: "Milling Statement, Stock Register, Party Ledger & Invoices"; color: "#64748B"; font.pixelSize: 11 }
                            }

                            KbdBadge { text: "Alt+7"; badgeColor: "#ECFDF5"; textColor: "#059669"; borderColor: "#A7F3D0" }
                        }
                    }
                }

                // Procurement Slips FastTable
                FastTable {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    title: "Recent Paddy Procurement Slips"
                    model: paddyModel
                    headers: ["Slip No", "Date", "Farmer", "Variety", "Bags", "Net (Qtl)", "Net Amt (₹)", "Status"]
                    roleKeys: ["slip_no", "arrival_date", "farmer_name", "paddy_variety", "bag_count", "net_weight_qtl", "net_amount", "payment_status"]
                    onNewEntryRequested: root.openNewPaddy()
                }
            }

            // ==============================================================
            // RIGHT SECTION (1/6th Width): KEPT EMPTY FOR NOW AS REQUESTED
            // ==============================================================
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredWidth: 1
                Layout.fillHeight: true
                color: "#FFFFFF"
                border.color: "#E2E8F0"
                border.width: 1
                radius: 10

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 16
                    spacing: 12

                    Text {
                        text: "QUICK WIDGETS"
                        color: "#94A3B8"
                        font.pixelSize: 10
                        font.bold: true
                        font.letterSpacing: 1.0
                    }

                    Rectangle { Layout.fillWidth: true; height: 1; color: "#F1F5F9" }

                    Item { Layout.fillHeight: true }

                    // Placeholder graphic / subtle text
                    ColumnLayout {
                        Layout.alignment: Qt.AlignHCenter
                        spacing: 8
                        Text {
                            text: "⚡"
                            font.pixelSize: 28
                            horizontalAlignment: Text.AlignHCenter
                            Layout.alignment: Qt.AlignHCenter
                        }
                        Text {
                            text: "Reserved Section"
                            color: "#94A3B8"
                            font.pixelSize: 12
                            font.bold: true
                            horizontalAlignment: Text.AlignHCenter
                        }
                    }

                    Item { Layout.fillHeight: true }
                }
            }
        }
    }
}
