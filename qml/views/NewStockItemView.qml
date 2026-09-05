import QtQuick
import QtQuick.Templates as T
import QtQuick.Layouts
import MahadevERP

Item {
    id: root
    anchors.fill: parent

    signal cancelRequested()
    signal savedSuccess()

    // -------------------------------------------------------------
    // PROPERTIES & TOGGLES
    // -------------------------------------------------------------
    property string selectedItemType: "Both" // "Mandi", "Market", "Both"
    readonly property bool isMandiOrBoth: selectedItemType === "Mandi" || selectedItemType === "Both"
    readonly property bool isMarketOrBoth: selectedItemType === "Market" || selectedItemType === "Both"

    // -------------------------------------------------------------
    // KEYBOARD SHORTCUTS
    // -------------------------------------------------------------
    Shortcut {
        sequence: "Esc"
        onActivated: root.cancelRequested()
    }
    Shortcut {
        sequence: "F2"
        onActivated: root.saveStockItem()
    }
    Shortcut {
        sequence: "F5"
        onActivated: root.copyAllLedgersFromPurchase()
    }
    Shortcut {
        sequence: "Alt+T"
        onActivated: {
            if (selectedItemType === "Both") selectedItemType = "Market"
            else if (selectedItemType === "Market") selectedItemType = "Mandi"
            else selectedItemType = "Both"
        }
    }
    Shortcut {
        sequence: "Alt+C"
        onActivated: newGroupPopup.open()
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 14
        spacing: 8

        // =========================================================
        // TOP HEADER BAR
        // =========================================================
        RowLayout {
            Layout.fillWidth: true
            spacing: 10

            T.Button {
                id: backBtn
                implicitWidth: contentItem.implicitWidth + 24
                implicitHeight: 32
                background: Rectangle { color: backBtn.hovered ? "#E2E8F0" : "#F1F5F9"; radius: 6; border.color: "#CBD5E1" }
                contentItem: RowLayout {
                    spacing: 6
                    anchors.centerIn: parent
                    Text { text: "← Back"; color: "#475569"; font.pixelSize: 12; font.bold: true }
                    KbdBadge { text: "Esc"; badgeColor: "#DC2626"; textColor: "#FFF"; borderColor: "#B91C1C" }
                }
                onClicked: root.cancelRequested()
            }

            ColumnLayout {
                spacing: 1
                Text {
                    text: "📦 STOCK ITEM CREATION (Single Slate Master)"
                    color: "#0F172A"
                    font.pixelSize: 16
                    font.bold: true
                }
                Text {
                    text: "Mandi Type, Market Type & Both Classification • Dual Accounting & Labour Matrix"
                    color: "#64748B"
                    font.pixelSize: 11
                }
            }

            Item { Layout.fillWidth: true }

            // Top Goods Type Dropdown
            RowLayout {
                spacing: 6
                Text { text: "Item Type :"; color: "#334155"; font.pixelSize: 12; font.bold: true }
                CustomWhiteCombo {
                    id: goodsTypeCombo
                    model: (typeof stockItemsModel !== "undefined" && stockItemsModel) ? stockItemsModel.get_goods_types() : ["Goods", "Services", "Capital Goods"]
                    Layout.preferredWidth: 140
                    comboHeight: 32
                    currentIndex: 0
                }
            }

            T.Button {
                id: autoClStockBtn
                implicitWidth: 100
                implicitHeight: 32
                background: Rectangle { color: autoClStockBtn.hovered ? "#E0E7FF" : "#EEF2FF"; radius: 6; border.color: "#C7D2FE" }
                contentItem: Text {
                    text: "Auto Cl.Stock"
                    color: "#4338CA"
                    font.pixelSize: 11
                    font.bold: true
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
                onClicked: statusToast.show("Auto Closing Stock updated according to active books.")
            }

            // Quick Badges
            KbdBadge { text: "Alt+T: Type"; badgeColor: "#0284C7"; textColor: "#FFF"; borderColor: "#0369A1" }
            KbdBadge { text: "F5: All Same"; badgeColor: "#7C3AED"; textColor: "#FFF"; borderColor: "#6D28D9" }
            KbdBadge { text: "F2: Save"; badgeColor: "#16A34A"; textColor: "#FFF"; borderColor: "#15803D" }
        }

        Rectangle { Layout.fillWidth: true; height: 1; color: "#E2E8F0" }

        // =========================================================
        // SCROLLABLE FORM BODY
        // =========================================================
        T.ScrollView {
            id: formScroll
            Layout.fillWidth: true
            Layout.fillHeight: true
            T.ScrollBar.vertical: T.ScrollBar { policy: T.ScrollBar.AsNeeded }

            ColumnLayout {
                width: formScroll.width - 12
                spacing: 10

                // -------------------------------------------------
                // CARD 1: PRIMARY IDENTIFICATION & 3-TYPE SELECTION
                // -------------------------------------------------
                Rectangle {
                    Layout.fillWidth: true
                    implicitHeight: primaryCol.implicitHeight + 20
                    color: "#FEFCE8" // Warm Bahi-Khata champagne accent
                    border.color: "#FDE047"
                    border.width: 1
                    radius: 8

                    ColumnLayout {
                        id: primaryCol
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.top: parent.top
                        anchors.margins: 12
                        spacing: 10

                        // Row 1: Stock Item Name & Type Radio Buttons
                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 12

                            CustomInput {
                                id: nameInput
                                label: "Stock Item Name *"
                                placeholderText: "e.g. Paddy Basmati 1401, Sona Masoori, Rice Broken"
                                isRequired: true
                                focusInput: true
                                Layout.fillWidth: true
                                onReturnPressed: autoAdjustCheck.focus = true
                            }

                            CustomCheckBox {
                                id: autoAdjustCheck
                                text: "Auto Adjust Name"
                                checked: true
                                textColor: "#713F12"
                            }

                            // 3-Type Radio Box matching Bahi-Khata
                            Rectangle {
                                Layout.preferredWidth: typeBoxRow.implicitWidth + 24
                                Layout.preferredHeight: 38
                                color: "#FFFFFF"
                                border.color: "#CA8A04"
                                border.width: 1.5
                                radius: 6

                                RowLayout {
                                    id: typeBoxRow
                                    anchors.centerIn: parent
                                    spacing: 12

                                    Text {
                                        text: "Item Type (Alt+T):"
                                        font.pixelSize: 11
                                        font.bold: true
                                        color: "#854D0E"
                                    }

                                    CustomRadioButton {
                                        id: rbMandi
                                        text: "Mandi Type"
                                        checked: root.selectedItemType === "Mandi"
                                        activeColor: "#D97706"
                                        textColor: "#854D0E"
                                        onCheckedChanged: if (checked) root.selectedItemType = "Mandi"
                                    }

                                    CustomRadioButton {
                                        id: rbMarket
                                        text: "Market Type"
                                        checked: root.selectedItemType === "Market"
                                        activeColor: "#D97706"
                                        textColor: "#854D0E"
                                        onCheckedChanged: if (checked) root.selectedItemType = "Market"
                                    }

                                    CustomRadioButton {
                                        id: rbBoth
                                        text: "Both"
                                        checked: root.selectedItemType === "Both"
                                        activeColor: "#D97706"
                                        textColor: "#854D0E"
                                        onCheckedChanged: if (checked) root.selectedItemType = "Both"
                                    }
                                }
                            }
                        }

                        // Row 2: Trading Group, Unit, Rate@, Calculation Toggles
                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 10

                            ColumnLayout {
                                Layout.preferredWidth: 260
                                spacing: 4
                                RowLayout {
                                    spacing: 4
                                    Text { text: "Trading Group *"; color: "#713F12"; font.pixelSize: 11; font.bold: true }
                                    Text {
                                        text: "(Alt+C: Create New)"
                                        color: "#2563EB"
                                        font.pixelSize: 10
                                        font.underline: true
                                        MouseArea {
                                            anchors.fill: parent
                                            cursorShape: Qt.PointingHandCursor
                                            onClicked: newGroupPopup.open()
                                        }
                                    }
                                }
                                CustomWhiteCombo {
                                    id: groupCombo
                                    Layout.fillWidth: true
                                    comboHeight: 34
                                    model: (typeof stockItemsModel !== "undefined" && stockItemsModel) ? stockItemsModel.get_stock_groups() : ["Paddy Basmati 1401", "Paddy Parmal", "Rice"]
                                    onAccepted: {
                                        if (editText.trim() && find(editText.trim()) === -1) {
                                            stockItemsModel.add_stock_group(editText.trim())
                                            model = stockItemsModel.get_stock_groups()
                                        }
                                    }
                                }
                            }

                            ColumnLayout {
                                Layout.preferredWidth: 120
                                spacing: 4
                                Text { text: "Unit Name *"; color: "#713F12"; font.pixelSize: 11; font.bold: true }
                                CustomWhiteCombo {
                                    id: unitCombo
                                    Layout.fillWidth: true
                                    comboHeight: 34
                                    model: (typeof stockItemsModel !== "undefined" && stockItemsModel) ? stockItemsModel.get_units() : ["Qtl.", "Bags", "Kg", "Nos"]
                                    currentIndex: 0
                                }
                            }

                            ColumnLayout {
                                Layout.preferredWidth: 100
                                spacing: 4
                                Text { text: "Rate@"; color: "#713F12"; font.pixelSize: 11; font.bold: true }
                                CustomWhiteCombo {
                                    id: rateCalcCombo
                                    Layout.fillWidth: true
                                    comboHeight: 34
                                    model: ["N/A", "Weight", "Unit", "Bags"]
                                    currentIndex: 0
                                }
                            }

                            CustomCheckBox {
                                id: stockCalcCheck
                                text: "Stock Calculate"
                                checked: true
                            }

                            CustomCheckBox {
                                id: calcTradingCheck
                                text: "Calculate In Trading A/c"
                                checked: true
                            }

                            Item { Layout.fillWidth: true }
                        }

                        // Row 3: Printing Narration
                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 8
                            CustomInput {
                                id: narrationInput
                                label: "Printing Narration (Prints on Bills / Invoices)"
                                placeholderText: "Leave blank or add custom printing remark for this item..."
                                Layout.fillWidth: true
                            }
                        }
                    }
                }

                // -------------------------------------------------
                // CARD 2: GST & TAXATION (Single Slate)
                // -------------------------------------------------
                Rectangle {
                    Layout.fillWidth: true
                    implicitHeight: taxCol.implicitHeight + 16
                    color: "#FFFFFF"
                    border.color: "#E2E8F0"
                    border.width: 1
                    radius: 8

                    ColumnLayout {
                        id: taxCol
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.top: parent.top
                        anchors.margins: 12
                        spacing: 8

                        RowLayout {
                            Layout.fillWidth: true
                            Text {
                                text: "TAXATION & STATUTORY RATES (GST / VAT / CST)"
                                color: "#0F766E"
                                font.pixelSize: 11
                                font.bold: true
                                font.letterSpacing: 0.8
                            }
                            Item { Layout.fillWidth: true }
                            CustomCheckBox {
                                id: capitalGoodsCheck
                                text: "Capital Goods"
                                textColor: "#0F766E"
                            }
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 10

                            ColumnLayout {
                                Layout.preferredWidth: 110
                                spacing: 4
                                Text { text: "GST Rate % *"; color: "#334155"; font.pixelSize: 11; font.bold: true }
                                CustomWhiteCombo {
                                    id: gstRateCombo
                                    Layout.fillWidth: true
                                    comboHeight: 34
                                    model: (typeof stockItemsModel !== "undefined" && stockItemsModel) ? stockItemsModel.get_gst_rates() : ["0%", "5%", "12%", "18%", "28%"]
                                    currentIndex: 0 // default 0%
                                }
                            }

                            CustomInput {
                                id: hsnInput
                                label: "HSN / SAC Code *"
                                text: "1006"
                                placeholderText: "1006"
                                Layout.preferredWidth: 120
                            }

                            CustomInput {
                                id: cessInput
                                label: "Cess %"
                                text: "0.00"
                                placeholderText: "0.00"
                                inputMethodHints: Qt.ImhFormattedNumbersOnly
                                Layout.preferredWidth: 90
                            }

                            CustomWhiteCombo {
                                id: gstLedgerCombo
                                label: "GST / Duties & Taxes Ledger *"
                                Layout.fillWidth: true
                                comboHeight: 34
                                model: (typeof partiesModel !== "undefined" && partiesModel) ? partiesModel.get_parties_list() : ["Duties & Taxes"]
                                text: "Duties & Taxes"
                            }
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 10

                            CustomInput {
                                id: vatInput
                                label: "VAT %"
                                text: "0.00"
                                inputMethodHints: Qt.ImhFormattedNumbersOnly
                                Layout.preferredWidth: 80
                            }

                            CustomWhiteCombo {
                                id: vatLedgerCombo
                                label: "VAT A/c"
                                Layout.preferredWidth: 180
                                comboHeight: 34
                                model: (typeof partiesModel !== "undefined" && partiesModel) ? partiesModel.get_parties_list() : ["VAT A/c"]
                                text: "VAT A/c"
                            }

                            CustomInput {
                                id: surVatInput
                                label: "Surch. on VAT %"
                                text: "0.00"
                                inputMethodHints: Qt.ImhFormattedNumbersOnly
                                Layout.preferredWidth: 95
                            }

                            CustomInput {
                                id: cstInput
                                label: "CST %"
                                text: "0.00"
                                inputMethodHints: Qt.ImhFormattedNumbersOnly
                                Layout.preferredWidth: 80
                            }

                            CustomWhiteCombo {
                                id: cstLedgerCombo
                                label: "CST A/c"
                                Layout.fillWidth: true
                                comboHeight: 34
                                model: (typeof partiesModel !== "undefined" && partiesModel) ? partiesModel.get_parties_list() : ["CST A/c"]
                                text: "CST A/c"
                            }
                        }
                    }
                }

                // -------------------------------------------------
                // CARD 3: MANDI COMMITTEE & FEES (Visible on Mandi / Both)
                // -------------------------------------------------
                Rectangle {
                    Layout.fillWidth: true
                    visible: root.isMandiOrBoth
                    implicitHeight: mandiCol.implicitHeight + 16
                    color: "#FFF7ED" // Soft peach/orange
                    border.color: "#FDBA74"
                    border.width: 1
                    radius: 8

                    ColumnLayout {
                        id: mandiCol
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.top: parent.top
                        anchors.margins: 12
                        spacing: 8

                        Text {
                            text: "MANDI COMMITTEE SETTINGS & FEES (MANDI TYPE / BOTH)"
                            color: "#C2410C"
                            font.pixelSize: 11
                            font.bold: true
                            font.letterSpacing: 0.8
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 10

                            CustomInput {
                                id: damiInput
                                label: "Dami %"
                                text: "2.50"
                                inputMethodHints: Qt.ImhFormattedNumbersOnly
                                Layout.preferredWidth: 90
                            }

                            CustomWhiteCombo {
                                id: damiLedgerCombo
                                label: "Dami A/c *"
                                Layout.fillWidth: true
                                comboHeight: 34
                                model: (typeof partiesModel !== "undefined" && partiesModel) ? partiesModel.get_parties_list() : ["Dami A/c"]
                                text: "Dami A/c"
                            }

                            CustomInput {
                                id: mktFeeInput
                                label: "Mkt. Fee %"
                                text: "2.00"
                                inputMethodHints: Qt.ImhFormattedNumbersOnly
                                Layout.preferredWidth: 90
                            }

                            CustomWhiteCombo {
                                id: mFeeLedgerCombo
                                label: "Mkt. Fee A/c *"
                                Layout.fillWidth: true
                                comboHeight: 34
                                model: (typeof partiesModel !== "undefined" && partiesModel) ? partiesModel.get_parties_list() : ["Market Fee A/c"]
                                text: "Market Fee A/c"
                            }

                            CustomInput {
                                id: hrdfInput
                                label: "HRDF %"
                                text: "2.00"
                                inputMethodHints: Qt.ImhFormattedNumbersOnly
                                Layout.preferredWidth: 90
                            }

                            CustomWhiteCombo {
                                id: hrdfLedgerCombo
                                label: "HRDF A/c *"
                                Layout.fillWidth: true
                                comboHeight: 34
                                model: (typeof partiesModel !== "undefined" && partiesModel) ? partiesModel.get_parties_list() : ["H.R.D.F. A/c"]
                                text: "H.R.D.F. A/c"
                            }
                        }

                        // Market Committee Flags Row
                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 14

                            Text { text: "Market Committee Settings :"; color: "#9A3412"; font.pixelSize: 11; font.bold: true }

                            CustomCheckBox {
                                id: mktCommttFormCheck
                                text: "Apply Mkt.Commtt. Form"
                                textColor: "#9A3412"
                            }

                            CustomCheckBox {
                                id: mktCommttCouponCheck
                                text: "Apply Mkt.Comt.Coupon"
                                textColor: "#9A3412"
                            }

                            CustomCheckBox {
                                id: damiWeightCheck
                                text: "Dami Calculate on Weight"
                                textColor: "#9A3412"
                            }

                            CustomCheckBox {
                                id: taxOnQtyCheck
                                text: "Tax on Qty."
                                textColor: "#9A3412"
                            }

                            Item { Layout.fillWidth: true }
                        }
                    }
                }

                // -------------------------------------------------
                // CARD 4: FINANCIAL & TRADING ACCOUNTS
                // -------------------------------------------------
                Rectangle {
                    Layout.fillWidth: true
                    implicitHeight: acctCol.implicitHeight + 16
                    color: "#FFFFFF"
                    border.color: "#E2E8F0"
                    border.width: 1
                    radius: 8

                    ColumnLayout {
                        id: acctCol
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.top: parent.top
                        anchors.margins: 12
                        spacing: 8

                        RowLayout {
                            Layout.fillWidth: true
                            Text {
                                text: "TRADING & INVENTORY POSTING LEDGER ACCOUNTS"
                                color: "#4F46E5"
                                font.pixelSize: 11
                                font.bold: true
                                font.letterSpacing: 0.8
                            }
                            Item { Layout.fillWidth: true }
                            T.Button {
                                background: Rectangle { color: "#EEF2FF"; radius: 4; border.color: "#C7D2FE" }
                                contentItem: RowLayout {
                                    spacing: 4
                                    Text { text: "Copy Purchase A/c to All"; color: "#4338CA"; font.pixelSize: 11; font.bold: true }
                                    KbdBadge { text: "F5"; badgeColor: "#4338CA"; textColor: "#FFF"; borderColor: "#3730A3" }
                                }
                                onClicked: root.copyAllLedgersFromPurchase()
                            }
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 12

                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: 4
                                Text { text: "Purchase A/c *"; color: "#334155"; font.pixelSize: 11; font.bold: true }
                                CustomWhiteCombo {
                                    id: purcLedgerCombo
                                    Layout.fillWidth: true
                                    comboHeight: 34
                                    model: (typeof partiesModel !== "undefined" && partiesModel) ? partiesModel.get_parties_list() : ["Purchase Accounts"]
                                    text: "Purchase Accounts"
                                    onEditTextChanged: {
                                        if (typeof purcRetLedgerCombo !== "undefined" && purcRetLedgerCombo && (purcRetLedgerCombo.editText === "" || purcRetLedgerCombo.editText === "Purchase Accounts")) {
                                            purcRetLedgerCombo.editText = editText
                                        }
                                    }
                                }
                            }

                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: 4
                                Text { text: "Purc. Return A/c *"; color: "#334155"; font.pixelSize: 11; font.bold: true }
                                CustomWhiteCombo {
                                    id: purcRetLedgerCombo
                                    Layout.fillWidth: true
                                    comboHeight: 34
                                    model: (typeof partiesModel !== "undefined" && partiesModel) ? partiesModel.get_parties_list() : ["Purchase Accounts"]
                                    text: "Purchase Accounts"
                                }
                            }
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 12

                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: 4
                                Text { text: "Sale A/c *"; color: "#334155"; font.pixelSize: 11; font.bold: true }
                                CustomWhiteCombo {
                                    id: saleLedgerCombo
                                    Layout.fillWidth: true
                                    comboHeight: 34
                                    model: (typeof partiesModel !== "undefined" && partiesModel) ? partiesModel.get_parties_list() : ["Sales Accounts"]
                                    text: "Sales Accounts"
                                    onEditTextChanged: {
                                        if (typeof saleRetLedgerCombo !== "undefined" && saleRetLedgerCombo && (saleRetLedgerCombo.editText === "" || saleRetLedgerCombo.editText === "Sales Accounts")) {
                                            saleRetLedgerCombo.editText = editText
                                        }
                                    }
                                }
                            }

                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: 4
                                Text { text: "Sale Return A/c *"; color: "#334155"; font.pixelSize: 11; font.bold: true }
                                CustomWhiteCombo {
                                    id: saleRetLedgerCombo
                                    Layout.fillWidth: true
                                    comboHeight: 34
                                    model: (typeof partiesModel !== "undefined" && partiesModel) ? partiesModel.get_parties_list() : ["Sales Accounts"]
                                    text: "Sales Accounts"
                                }
                            }
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 12

                            ColumnLayout {
                                Layout.preferredWidth: 380
                                spacing: 4
                                Text { text: "Stock A/c (Self Trading) *"; color: "#334155"; font.pixelSize: 11; font.bold: true }
                                CustomWhiteCombo {
                                    id: stockLedgerCombo
                                    Layout.fillWidth: true
                                    comboHeight: 34
                                    model: (typeof partiesModel !== "undefined" && partiesModel) ? partiesModel.get_parties_list() : ["Stock-in-Hand"]
                                    text: "Stock-in-Hand"
                                }
                            }

                            Item { Layout.fillWidth: true }
                        }
                    }
                }

                // -------------------------------------------------
                // CARD 5: PACKING & PRICING
                // -------------------------------------------------
                Rectangle {
                    Layout.fillWidth: true
                    implicitHeight: priceCol.implicitHeight + 16
                    color: "#FFFFFF"
                    border.color: "#E2E8F0"
                    border.width: 1
                    radius: 8

                    ColumnLayout {
                        id: priceCol
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.top: parent.top
                        anchors.margins: 12
                        spacing: 8

                        Text {
                            text: "PACKING, INITIAL VALUATION & PRICING"
                            color: "#0369A1"
                            font.pixelSize: 11
                            font.bold: true
                            font.letterSpacing: 0.8
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 10

                            CustomInput {
                                id: packingInput
                                label: "Packing (Kg/Bag)"
                                text: "50.0"
                                placeholderText: "50.0"
                                inputMethodHints: Qt.ImhFormattedNumbersOnly
                                Layout.preferredWidth: 130
                            }

                            CustomInput {
                                id: purcRateInput
                                label: "Purc. Rate (₹)"
                                text: "0.00"
                                placeholderText: "0.00"
                                inputMethodHints: Qt.ImhFormattedNumbersOnly
                                Layout.fillWidth: true
                            }

                            CustomInput {
                                id: saleRateInput
                                label: "Sale Rate (₹)"
                                text: "0.00"
                                placeholderText: "0.00"
                                inputMethodHints: Qt.ImhFormattedNumbersOnly
                                Layout.fillWidth: true
                            }

                            CustomInput {
                                id: bonusInput
                                label: "Bonus Approved (₹)"
                                text: "0.00"
                                placeholderText: "0.00"
                                inputMethodHints: Qt.ImhFormattedNumbersOnly
                                Layout.fillWidth: true
                            }

                            CustomInput {
                                id: opBagsInput
                                label: "Opening Bags"
                                text: "0"
                                inputMethodHints: Qt.ImhDigitsOnly
                                Layout.preferredWidth: 100
                            }

                            CustomInput {
                                id: opQtyInput
                                label: "Opening Qtl."
                                text: "0.000"
                                inputMethodHints: Qt.ImhFormattedNumbersOnly
                                Layout.preferredWidth: 110
                            }
                        }
                    }
                }

                // -------------------------------------------------
                // CARD 6: LABOUR RATE SETTINGS (Visible on Mandi / Both)
                // -------------------------------------------------
                Rectangle {
                    Layout.fillWidth: true
                    visible: root.isMandiOrBoth
                    implicitHeight: labourCol.implicitHeight + 16
                    color: "#F0FDF4" // Soft mint/green
                    border.color: "#86EFAC"
                    border.width: 1
                    radius: 8

                    ColumnLayout {
                        id: labourCol
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.top: parent.top
                        anchors.margins: 12
                        spacing: 8

                        RowLayout {
                            Layout.fillWidth: true
                            Text {
                                text: "LABOUR RATE SLABS & OPERATION CHARGES (MANDI TYPE / BOTH)"
                                color: "#15803D"
                                font.pixelSize: 11
                                font.bold: true
                                font.letterSpacing: 0.8
                            }
                            Item { Layout.fillWidth: true }
                            RowLayout {
                                spacing: 6
                                Text { text: "Labour Rate Unit :"; color: "#166534"; font.pixelSize: 11; font.bold: true }
                                CustomWhiteCombo {
                                    id: labourUnitCombo
                                    model: ["Packing", "Weight", "Qtl"]
                                    Layout.preferredWidth: 110
                                    comboHeight: 30
                                    currentIndex: 0
                                }
                            }
                        }

                        // Labour Rates Matrix Grid
                        GridLayout {
                            Layout.fillWidth: true
                            columns: 8
                            columnSpacing: 6
                            rowSpacing: 6

                            // Header Row
                            Text { text: "Slab"; color: "#166534"; font.pixelSize: 11; font.bold: true; Layout.preferredWidth: 90 }
                            Text { text: "उतराई"; color: "#166534"; font.pixelSize: 11; font.bold: true; horizontalAlignment: Text.AlignHCenter; Layout.fillWidth: true }
                            Text { text: "झराई"; color: "#166534"; font.pixelSize: 11; font.bold: true; horizontalAlignment: Text.AlignHCenter; Layout.fillWidth: true }
                            Text { text: "भराई"; color: "#166534"; font.pixelSize: 11; font.bold: true; horizontalAlignment: Text.AlignHCenter; Layout.fillWidth: true }
                            Text { text: "तुलाई"; color: "#166534"; font.pixelSize: 11; font.bold: true; horizontalAlignment: Text.AlignHCenter; Layout.fillWidth: true }
                            Text { text: "खिंचाई"; color: "#166534"; font.pixelSize: 11; font.bold: true; horizontalAlignment: Text.AlignHCenter; Layout.fillWidth: true }
                            Text { text: "सिलाई"; color: "#166534"; font.pixelSize: 11; font.bold: true; horizontalAlignment: Text.AlignHCenter; Layout.fillWidth: true }
                            Text { text: "चढ़वाई"; color: "#166534"; font.pixelSize: 11; font.bold: true; horizontalAlignment: Text.AlignHCenter; Layout.fillWidth: true }

                            // Slab 1: 01 - 40 Kg
                            Text { text: "01 - 40 Kg. @"; color: "#334155"; font.pixelSize: 11; font.bold: true }
                            CustomInput { id: u1; text: "0.00"; Layout.fillWidth: true }
                            CustomInput { id: j1; text: "0.00"; Layout.fillWidth: true }
                            CustomInput { id: b1; text: "0.00"; Layout.fillWidth: true }
                            CustomInput { id: t1; text: "0.00"; Layout.fillWidth: true }
                            CustomInput { id: k1; text: "0.00"; Layout.fillWidth: true }
                            CustomInput { id: s1; text: "0.00"; Layout.fillWidth: true }
                            CustomInput { id: l1; text: "0.00"; Layout.fillWidth: true }

                            // Slab 2: 41 - 70 Kg
                            Text { text: "41 - 70 Kg. @"; color: "#334155"; font.pixelSize: 11; font.bold: true }
                            CustomInput { id: u2; text: "0.00"; Layout.fillWidth: true }
                            CustomInput { id: j2; text: "0.00"; Layout.fillWidth: true }
                            CustomInput { id: b2; text: "0.00"; Layout.fillWidth: true }
                            CustomInput { id: t2; text: "0.00"; Layout.fillWidth: true }
                            CustomInput { id: k2; text: "0.00"; Layout.fillWidth: true }
                            CustomInput { id: s2; text: "0.00"; Layout.fillWidth: true }
                            CustomInput { id: l2; text: "0.00"; Layout.fillWidth: true }

                            // Slab 3: 71 - 100 Kg
                            Text { text: "71 - 100 Kg. @"; color: "#334155"; font.pixelSize: 11; font.bold: true }
                            CustomInput { id: u3; text: "0.00"; Layout.fillWidth: true }
                            CustomInput { id: j3; text: "0.00"; Layout.fillWidth: true }
                            CustomInput { id: b3; text: "0.00"; Layout.fillWidth: true }
                            CustomInput { id: t3; text: "0.00"; Layout.fillWidth: true }
                            CustomInput { id: k3; text: "0.00"; Layout.fillWidth: true }
                            CustomInput { id: s3; text: "0.00"; Layout.fillWidth: true }
                            CustomInput { id: l3; text: "0.00"; Layout.fillWidth: true }
                        }
                    }
                }

                Item { Layout.preferredHeight: 12 }
            }
        }

        // =========================================================
        // BOTTOM ACTION BAR
        // =========================================================
        Rectangle { Layout.fillWidth: true; height: 1; color: "#E2E8F0" }

        RowLayout {
            Layout.fillWidth: true
            spacing: 12

            T.Button {
                id: cancelBtn
                implicitWidth: 110
                implicitHeight: 38
                background: Rectangle { color: cancelBtn.hovered ? "#E2E8F0" : "#F1F5F9"; radius: 6; border.color: "#CBD5E1" }
                contentItem: RowLayout {
                    spacing: 6
                    anchors.centerIn: parent
                    Text { text: "Cancel"; color: "#475569"; font.pixelSize: 13; font.bold: true }
                    KbdBadge { text: "Esc"; badgeColor: "#DC2626"; textColor: "#FFF"; borderColor: "#B91C1C" }
                }
                onClicked: root.cancelRequested()
            }

            Item { Layout.fillWidth: true }

            T.Button {
                id: saveBtn
                implicitWidth: 260
                implicitHeight: 38
                background: Rectangle {
                    color: (saveBtn.hovered || saveBtn.activeFocus) ? "#15803D" : "#16A34A"
                    radius: 6
                    border.color: saveBtn.activeFocus ? "#86EFAC" : "transparent"
                    border.width: 2
                }
                contentItem: RowLayout {
                    spacing: 8
                    anchors.centerIn: parent
                    Text { text: "💾 Save Stock Item"; color: "#FFFFFF"; font.bold: true; font.pixelSize: 14 }
                    KbdBadge { text: "F2"; badgeColor: "#14532D"; textColor: "#86EFAC"; borderColor: "#166534" }
                }
                onClicked: root.saveStockItem()
            }
        }
    }

    // -------------------------------------------------------------
    // HELPER FUNCTIONS
    // -------------------------------------------------------------
    function copyAllLedgersFromPurchase() {
        var pLedger = purcLedgerCombo.editText.trim()
        if (pLedger === "") pLedger = purcLedgerCombo.currentText.trim()
        if (pLedger === "") pLedger = nameInput.text.trim()
        if (pLedger === "") return

        purcLedgerCombo.editText = pLedger
        purcRetLedgerCombo.editText = pLedger
        saleLedgerCombo.editText = pLedger
        saleRetLedgerCombo.editText = pLedger
        stockLedgerCombo.editText = pLedger
        statusToast.show("Copied '" + pLedger + "' to all 4 trading accounts.")
    }

    function saveStockItem() {
        if (!nameInput.text.trim()) {
            statusToast.show("Please enter a valid Stock Item Name.")
            nameInput.forceActiveFocus()
            return
        }
        confirmModal.open()
    }

    function executeSaveStockItem() {
        var gstText = gstRateCombo.currentText.replace("%", "").trim()
        var gstVal = isNaN(parseFloat(gstText)) ? 0.0 : parseFloat(gstText)
        var payload = {
            "name": nameInput.text.trim(),
            "code": "",
            "item_type": selectedItemType,
            "goods_type": goodsTypeCombo.currentText,
            "trading_group": groupCombo.editText.trim() || groupCombo.currentText || "Primary",
            "unit": unitCombo.editText.trim() || unitCombo.currentText || "Qtl.",
            "rate_calc_on": rateCalcCombo.currentText,
            "auto_adjust_name": autoAdjustCheck.checked ? 1 : 0,
            "item_narration": narrationInput.text.trim(),
            "capital_goods": capitalGoodsCheck.checked ? 1 : 0,

            "hsn_code": hsnInput.text.trim(),
            "gst_rate": gstVal,
            "gst_ledger": gstLedgerCombo.editText.trim() || gstLedgerCombo.currentText,
            "cess_rate": parseFloat(cessInput.text) || 0.0,
            "vat_rate": parseFloat(vatInput.text) || 0.0,
            "vat_ledger": vatLedgerCombo.editText.trim() || vatLedgerCombo.currentText,
            "surcharge_on_vat": parseFloat(surVatInput.text) || 0.0,
            "cst_rate": parseFloat(cstInput.text) || 0.0,
            "cst_ledger": cstLedgerCombo.editText.trim() || cstLedgerCombo.currentText,

            "dami_rate": parseFloat(damiInput.text) || 0.0,
            "dami_ledger": damiLedgerCombo.editText.trim() || damiLedgerCombo.currentText,
            "market_fee_rate": parseFloat(mktFeeInput.text) || 0.0,
            "market_fee_ledger": mFeeLedgerCombo.editText.trim() || mFeeLedgerCombo.currentText,
            "hrdf_rate": parseFloat(hrdfInput.text) || 0.0,
            "hrdf_ledger": hrdfLedgerCombo.editText.trim() || hrdfLedgerCombo.currentText,

            "market_commtt_form_apply": mktCommttFormCheck.checked ? 1 : 0,
            "market_commtt_coupon_apply": mktCommttCouponCheck.checked ? 1 : 0,
            "dami_calc_on_weight": damiWeightCheck.checked ? 1 : 0,
            "tax_on_qty": taxOnQtyCheck.checked ? 1 : 0,

            "purchase_rate": parseFloat(purcRateInput.text) || 0.0,
            "sale_rate": parseFloat(saleRateInput.text) || 0.0,
            "bonus_approved": parseFloat(bonusInput.text) || 0.0,
            "packing_kg": parseFloat(packingInput.text) || 50.0,
            "opening_bags": parseInt(opBagsInput.text) || 0,
            "opening_qty": parseFloat(opQtyInput.text) || 0.0,

            "purchase_ledger": purcLedgerCombo.editText.trim() || purcLedgerCombo.currentText,
            "purchase_return_ledger": purcRetLedgerCombo.editText.trim() || purcRetLedgerCombo.currentText,
            "sale_ledger": saleLedgerCombo.editText.trim() || saleLedgerCombo.currentText,
            "sale_return_ledger": saleRetLedgerCombo.editText.trim() || saleRetLedgerCombo.currentText,
            "stock_ledger": stockLedgerCombo.editText.trim() || stockLedgerCombo.currentText,

            "calculate_stock": stockCalcCheck.checked ? 1 : 0,
            "include_in_trading": calcTradingCheck.checked ? 1 : 0,
            "labour_rate_unit": labourUnitCombo.currentText,

            "utrai_rate_1": parseFloat(u1.text) || 0.0,
            "jharai_rate_1": parseFloat(j1.text) || 0.0,
            "bharai_rate_1": parseFloat(b1.text) || 0.0,
            "tulai_rate_1": parseFloat(t1.text) || 0.0,
            "khichai_rate_1": parseFloat(k1.text) || 0.0,
            "silai_rate_1": parseFloat(s1.text) || 0.0,
            "loading_rate_1": parseFloat(l1.text) || 0.0,

            "utrai_rate_2": parseFloat(u2.text) || 0.0,
            "jharai_rate_2": parseFloat(j2.text) || 0.0,
            "bharai_rate_2": parseFloat(b2.text) || 0.0,
            "tulai_rate_2": parseFloat(t2.text) || 0.0,
            "khichai_rate_2": parseFloat(k2.text) || 0.0,
            "silai_rate_2": parseFloat(s2.text) || 0.0,
            "loading_rate_2": parseFloat(l2.text) || 0.0,

            "utrai_rate_3": parseFloat(u3.text) || 0.0,
            "jharai_rate_3": parseFloat(j3.text) || 0.0,
            "bharai_rate_3": parseFloat(b3.text) || 0.0,
            "tulai_rate_3": parseFloat(t3.text) || 0.0,
            "khichai_rate_3": parseFloat(k3.text) || 0.0,
            "silai_rate_3": parseFloat(s3.text) || 0.0,
            "loading_rate_3": parseFloat(l3.text) || 0.0
        }

        var success = stockItemsModel.save_stock_item_full(payload)
        if (success) {
            root.savedSuccess()
        } else {
            statusToast.show("Failed to save stock item. Please check the values.")
        }
    }

    // -------------------------------------------------------------
    // MODALS & POPUPS
    // -------------------------------------------------------------
    ConfirmationModal {
        id: confirmModal
        anchors.centerIn: parent
        titleText: "CONFIRM STOCK ITEM SAVE"
        messageText: "Save stock item '" + nameInput.text.trim() + "' (" + selectedItemType + " Type)?"
        onConfirmed: root.executeSaveStockItem()
    }

    T.Popup {
        id: newGroupPopup
        x: Math.round((parent.width - width) / 2)
        y: Math.round((parent.height - height) / 2)
        width: 380
        height: 180
        modal: true
        focus: true
        closePolicy: T.Popup.CloseOnEscape | T.Popup.CloseOnPressOutside
        background: Rectangle { color: "#FFFFFF"; radius: 8; border.color: "#CBD5E1"; border.width: 1 }

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 14
            spacing: 10

            Text { text: "Create New Trading Group"; color: "#0F172A"; font.pixelSize: 14; font.bold: true }

            CustomInput {
                id: newGroupNameInput
                label: "Group Name"
                placeholderText: "e.g. Paddy Basmati 1401"
                Layout.fillWidth: true
                focusInput: true
            }

            RowLayout {
                Layout.fillWidth: true
                Item { Layout.fillWidth: true }
                T.Button {
                    text: "Cancel"
                    onClicked: newGroupPopup.close()
                }
                T.Button {
                    text: "Create Group"
                    background: Rectangle { color: "#2563EB"; radius: 4 }
                    contentItem: Text { text: "Create Group"; color: "#FFF"; font.bold: true }
                    onClicked: {
                        var g = newGroupNameInput.text.trim()
                        if (g) {
                            stockItemsModel.add_stock_group(g)
                            groupCombo.model = stockItemsModel.get_stock_groups()
                            groupCombo.editText = g
                            newGroupPopup.close()
                        }
                    }
                }
            }
        }
    }

    // Toast banner
    Rectangle {
        id: statusToast
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 20
        width: toastText.implicitWidth + 28
        height: 36
        radius: 18
        color: "#1E293B"
        opacity: 0.0
        z: 9999

        property alias message: toastText.text
        function show(msg) {
            message = msg
            toastAnim.restart()
        }

        Text {
            id: toastText
            anchors.centerIn: parent
            color: "#FFFFFF"
            font.pixelSize: 12
            font.bold: true
        }

        SequentialAnimation {
            id: toastAnim
            NumberAnimation { target: statusToast; property: "opacity"; to: 0.95; duration: 150 }
            PauseAnimation { duration: 2500 }
            NumberAnimation { target: statusToast; property: "opacity"; to: 0.0; duration: 250 }
        }
    }
}
