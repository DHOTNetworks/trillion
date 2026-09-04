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
    property int currentItemId: -1
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
        onActivated: root.updateStockItem()
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
    Shortcut {
        sequence: "Delete"
        onActivated: {
            if (root.currentItemId > 0) deleteConfirmModal.open()
        }
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
                    text: "✏️ STOCK ITEM ALTERATION (Single Slate Master)"
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
            KbdBadge { text: "F2: Update"; badgeColor: "#16A34A"; textColor: "#FFF"; borderColor: "#15803D" }
            KbdBadge { text: "Del: Delete"; badgeColor: "#DC2626"; textColor: "#FFF"; borderColor: "#B91C1C" }
        }

        Rectangle { Layout.fillWidth: true; height: 1; color: "#E2E8F0" }

        // =========================================================
        // STOCK ITEM SELECTOR CARD
        // =========================================================
        Rectangle {
            Layout.fillWidth: true
            implicitHeight: selectCol.implicitHeight + 16
            color: "#EFF6FF"
            border.color: "#BFDBFE"
            border.width: 1
            radius: 8
            z: 100

            ColumnLayout {
                id: selectCol
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.top: parent.top
                anchors.margins: 10
                spacing: 6

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 12

                    CustomWhiteCombo {
                        id: selectItemCombo
                        label: "🔍 CHOOSE STOCK ITEM TO ALTER / MODIFY (Type Name, ↑/↓ Arrows & Enter) *"
                        Layout.fillWidth: true
                        focusInput: true
                        model: (typeof stockItemsModel !== "undefined" && stockItemsModel) ? stockItemsModel.get_items_list() : []
                        onReturnPressed: root.loadSelectedItem(selectItemCombo.currentText)
                        onDownPressed: nameInput.focusInput = true
                    }

                    T.Button {
                        id: loadBtn
                        Layout.preferredWidth: 100
                        Layout.preferredHeight: 38
                        Layout.alignment: Qt.AlignBottom
                        background: Rectangle { color: (loadBtn.hovered || loadBtn.activeFocus) ? "#1D4ED8" : "#2563EB"; radius: 6 }
                        contentItem: Text {
                            text: "Load Item"
                            color: "#FFFFFF"
                            font.bold: true
                            font.pixelSize: 12
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }
                        onClicked: root.loadSelectedItem(selectItemCombo.currentText)
                    }
                }
            }
        }

        // =========================================================
        // SCROLLABLE FORM BODY
        // =========================================================
        T.ScrollView {
            id: formScroll
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
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
                                    model: (typeof stockItemsModel !== "undefined" && stockItemsModel) ? stockItemsModel.get_stock_groups() : ["Primary", "Paddy", "Rice", "Bardana"]
                                }
                            }

                            ColumnLayout {
                                Layout.preferredWidth: 150
                                spacing: 4
                                Text { text: "Unit *"; color: "#713F12"; font.pixelSize: 11; font.bold: true }
                                CustomWhiteCombo {
                                    id: unitCombo
                                    Layout.fillWidth: true
                                    comboHeight: 34
                                    model: (typeof stockItemsModel !== "undefined" && stockItemsModel) ? stockItemsModel.get_units() : ["Qtl.", "Bags", "Kgs", "Nos", "Pcs"]
                                }
                            }

                            ColumnLayout {
                                Layout.preferredWidth: 160
                                spacing: 4
                                Text { text: "Rate Calculated @"; color: "#713F12"; font.pixelSize: 11; font.bold: true }
                                CustomWhiteCombo {
                                    id: rateCalcCombo
                                    Layout.fillWidth: true
                                    comboHeight: 34
                                    model: ["N/A", "Weight", "Packing", "Bags"]
                                }
                            }

                            CustomInput {
                                id: narrationInput
                                label: "Item Narration / Specification"
                                placeholderText: "Optional item description"
                                Layout.fillWidth: true
                            }

                            CustomCheckBox {
                                id: capitalGoodsCheck
                                text: "Capital Goods"
                                textColor: "#713F12"
                            }
                        }
                    }
                }

                // -------------------------------------------------
                // CARD 2: GST & SALES TAX CLASSIFICATION
                // -------------------------------------------------
                Rectangle {
                    Layout.fillWidth: true
                    implicitHeight: taxCol.implicitHeight + 16
                    color: "#F8FAFC"
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

                        Text {
                            text: "GST & CENTRAL / STATE SALES TAX CLASSIFICATION"
                            color: "#0F172A"
                            font.pixelSize: 11
                            font.bold: true
                            font.letterSpacing: 0.8
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 10

                            CustomInput {
                                id: hsnInput
                                label: "HSN / SAC Code"
                                text: "1006"
                                placeholderText: "1006"
                                Layout.preferredWidth: 120
                            }

                            ColumnLayout {
                                Layout.preferredWidth: 120
                                spacing: 4
                                Text { text: "GST Rate %"; color: "#475569"; font.pixelSize: 11; font.bold: true }
                                CustomWhiteCombo {
                                    id: gstRateCombo
                                    Layout.fillWidth: true
                                    comboHeight: 34
                                    model: (typeof stockItemsModel !== "undefined" && stockItemsModel) ? stockItemsModel.get_gst_rates() : ["0%", "5%", "12%", "18%", "28%"]
                                    currentIndex: 1
                                }
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
                                label: "VAT Rate %"
                                text: "0.00"
                                placeholderText: "0.00"
                                inputMethodHints: Qt.ImhFormattedNumbersOnly
                                Layout.preferredWidth: 90
                            }

                            CustomWhiteCombo {
                                id: vatLedgerCombo
                                label: "VAT Ledger"
                                Layout.preferredWidth: 180
                                comboHeight: 34
                                model: (typeof partiesModel !== "undefined" && partiesModel) ? partiesModel.get_parties_list() : ["VAT A/c"]
                                text: "VAT A/c"
                            }

                            CustomInput {
                                id: surVatInput
                                label: "Surchg on VAT %"
                                text: "0.00"
                                placeholderText: "0.00"
                                inputMethodHints: Qt.ImhFormattedNumbersOnly
                                Layout.preferredWidth: 110
                            }

                            CustomInput {
                                id: cstInput
                                label: "CST Rate %"
                                text: "0.00"
                                placeholderText: "0.00"
                                inputMethodHints: Qt.ImhFormattedNumbersOnly
                                Layout.preferredWidth: 90
                            }

                            CustomWhiteCombo {
                                id: cstLedgerCombo
                                label: "CST Ledger"
                                Layout.fillWidth: true
                                comboHeight: 34
                                model: (typeof partiesModel !== "undefined" && partiesModel) ? partiesModel.get_parties_list() : ["CST A/c"]
                                text: "CST A/c"
                            }
                        }
                    }
                }

                // -------------------------------------------------
                // CARD 3: MANDI & MARKET COMMITTEE PARAMETERS
                // Visible on Mandi Type and Both
                // -------------------------------------------------
                Rectangle {
                    Layout.fillWidth: true
                    visible: root.isMandiOrBoth
                    implicitHeight: mandiCol.implicitHeight + 16
                    color: "#FFFBEB" // Warm Amber
                    border.color: "#FDE68A"
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
                            text: "MANDI & MARKET COMMITTEE PARAMETERS (MANDI TYPE / BOTH)"
                            color: "#B45309"
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
                                text: "0.00"
                                placeholderText: "2.50"
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
                                label: "Market Fee %"
                                text: "0.00"
                                placeholderText: "2.00"
                                inputMethodHints: Qt.ImhFormattedNumbersOnly
                                Layout.preferredWidth: 90
                            }

                            CustomWhiteCombo {
                                id: mFeeLedgerCombo
                                label: "Market Fee A/c *"
                                Layout.fillWidth: true
                                comboHeight: 34
                                model: (typeof partiesModel !== "undefined" && partiesModel) ? partiesModel.get_parties_list() : ["Market Fee A/c"]
                                text: "Market Fee A/c"
                            }

                            CustomInput {
                                id: hrdfInput
                                label: "H.R.D.F. %"
                                text: "0.00"
                                placeholderText: "2.00"
                                inputMethodHints: Qt.ImhFormattedNumbersOnly
                                Layout.preferredWidth: 90
                            }

                            CustomWhiteCombo {
                                id: hrdfLedgerCombo
                                label: "H.R.D.F. A/c *"
                                Layout.fillWidth: true
                                comboHeight: 34
                                model: (typeof partiesModel !== "undefined" && partiesModel) ? partiesModel.get_parties_list() : ["H.R.D.F. A/c"]
                                text: "H.R.D.F. A/c"
                            }
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 20

                            CustomCheckBox {
                                id: mktCommttFormCheck
                                text: "Market Commtt Form Applicable"
                                checked: false
                            }

                            CustomCheckBox {
                                id: mktCommttCouponCheck
                                text: "Market Commtt Coupon Applicable"
                                checked: false
                            }

                            CustomCheckBox {
                                id: damiWeightCheck
                                text: "Dami Calculate on Weight"
                                checked: false
                            }

                            CustomCheckBox {
                                id: taxOnQtyCheck
                                text: "Tax on Quantity"
                                checked: false
                            }
                        }
                    }
                }

                // -------------------------------------------------
                // CARD 4: TRADING & VALUATION ACCOUNTS
                // -------------------------------------------------
                Rectangle {
                    Layout.fillWidth: true
                    implicitHeight: acctCol.implicitHeight + 16
                    color: "#F1F5F9"
                    border.color: "#CBD5E1"
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
                                text: "TRADING & VALUATION ACCOUNTING LEDGERS"
                                color: "#334155"
                                font.pixelSize: 11
                                font.bold: true
                                font.letterSpacing: 0.8
                            }
                            Item { Layout.fillWidth: true }
                            T.Button {
                                id: allSameBtn
                                background: Rectangle { color: "#EDE9FE"; radius: 4; border.color: "#C4B5FD" }
                                contentItem: RowLayout {
                                    spacing: 4
                                    Text { text: "F5: All Same Accounts"; color: "#6D28D9"; font.pixelSize: 11; font.bold: true }
                                }
                                onClicked: root.copyAllLedgersFromPurchase()
                            }
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 8

                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: 4
                                Text { text: "Purchase A/c *"; color: "#475569"; font.pixelSize: 11; font.bold: true }
                                CustomWhiteCombo {
                                    id: purcLedgerCombo
                                    Layout.fillWidth: true
                                    comboHeight: 34
                                    model: (typeof partiesModel !== "undefined" && partiesModel) ? partiesModel.get_account_groups() : ["Purchase Accounts", "Trading A/c"]
                                }
                            }

                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: 4
                                Text { text: "Purc. Return A/c"; color: "#475569"; font.pixelSize: 11; font.bold: true }
                                CustomWhiteCombo {
                                    id: purcRetLedgerCombo
                                    Layout.fillWidth: true
                                    comboHeight: 34
                                    model: (typeof partiesModel !== "undefined" && partiesModel) ? partiesModel.get_account_groups() : ["Purchase Accounts", "Trading A/c"]
                                }
                            }

                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: 4
                                Text { text: "Sales A/c *"; color: "#475569"; font.pixelSize: 11; font.bold: true }
                                CustomWhiteCombo {
                                    id: saleLedgerCombo
                                    Layout.fillWidth: true
                                    comboHeight: 34
                                    model: (typeof partiesModel !== "undefined" && partiesModel) ? partiesModel.get_account_groups() : ["Sales Accounts", "Trading A/c"]
                                }
                            }

                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: 4
                                Text { text: "Sale Return A/c"; color: "#475569"; font.pixelSize: 11; font.bold: true }
                                CustomWhiteCombo {
                                    id: saleRetLedgerCombo
                                    Layout.fillWidth: true
                                    comboHeight: 34
                                    model: (typeof partiesModel !== "undefined" && partiesModel) ? partiesModel.get_account_groups() : ["Sales Accounts", "Trading A/c"]
                                }
                            }

                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: 4
                                Text { text: "Stock A/c *"; color: "#475569"; font.pixelSize: 11; font.bold: true }
                                CustomWhiteCombo {
                                    id: stockLedgerCombo
                                    Layout.fillWidth: true
                                    comboHeight: 34
                                    model: (typeof partiesModel !== "undefined" && partiesModel) ? partiesModel.get_account_groups() : ["Stock-in-Hand", "Trading A/c"]
                                }
                            }
                        }
                    }
                }

                // -------------------------------------------------
                // CARD 5: RATES, PACKING, OPENING STOCK & VALUATION
                // -------------------------------------------------
                Rectangle {
                    Layout.fillWidth: true
                    implicitHeight: ratesCol.implicitHeight + 16
                    color: "#FFFFFF"
                    border.color: "#E2E8F0"
                    border.width: 1
                    radius: 8

                    ColumnLayout {
                        id: ratesCol
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.top: parent.top
                        anchors.margins: 12
                        spacing: 8

                        Text {
                            text: "RATES, PACKING, OPENING STOCK & VALUATION"
                            color: "#0F172A"
                            font.pixelSize: 11
                            font.bold: true
                            font.letterSpacing: 0.8
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 10

                            CustomInput {
                                id: packingInput
                                label: "Packing (Kg)"
                                text: "50.0"
                                placeholderText: "50.0"
                                inputMethodHints: Qt.ImhFormattedNumbersOnly
                                Layout.preferredWidth: 110
                            }

                            CustomCheckBox {
                                id: stockCalcCheck
                                text: "Calculate Stock"
                                checked: true
                            }

                            CustomCheckBox {
                                id: calcTradingCheck
                                text: "Include In Trading"
                                checked: true
                            }

                            CustomInput {
                                id: purcRateInput
                                label: "Purc Rate (₹)"
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

            T.Button {
                id: deleteBtn
                visible: root.currentItemId > 0
                implicitWidth: 140
                implicitHeight: 38
                background: Rectangle {
                    color: deleteBtn.hovered ? "#B91C1C" : "#DC2626"
                    radius: 6
                }
                contentItem: RowLayout {
                    spacing: 6
                    anchors.centerIn: parent
                    Text { text: "🗑️ Delete Item"; color: "#FFFFFF"; font.bold: true; font.pixelSize: 13 }
                    KbdBadge { text: "Del"; badgeColor: "#7F1D1D"; textColor: "#FECACA"; borderColor: "#991B1B" }
                }
                onClicked: deleteConfirmModal.open()
            }

            Item { Layout.fillWidth: true }

            T.Button {
                id: updateBtn
                implicitWidth: 280
                implicitHeight: 38
                background: Rectangle {
                    color: (updateBtn.hovered || updateBtn.activeFocus) ? "#15803D" : "#16A34A"
                    radius: 6
                    border.color: updateBtn.activeFocus ? "#86EFAC" : "transparent"
                    border.width: 2
                }
                contentItem: RowLayout {
                    spacing: 8
                    anchors.centerIn: parent
                    Text { text: "💾 Update Complete Stock Item"; color: "#FFFFFF"; font.bold: true; font.pixelSize: 14 }
                    KbdBadge { text: "F2"; badgeColor: "#14532D"; textColor: "#86EFAC"; borderColor: "#166534" }
                }
                onClicked: root.updateStockItem()
            }
        }
    }

    // -------------------------------------------------------------
    // HELPER FUNCTIONS
    // -------------------------------------------------------------
    function loadSelectedItem(itemName) {
        if (!itemName || !itemName.trim()) return
        var item = (typeof stockItemsModel !== "undefined" && stockItemsModel) ? stockItemsModel.get_item_by_name(itemName.trim()) : {}
        if (!item || !item.name) return

        root.currentItemId = item.id ? item.id : -1
        nameInput.text = item.name ? item.name : ""
        autoAdjustCheck.checked = (item.auto_adjust_name !== 0)

        // Type
        var t = item.item_type ? item.item_type.trim() : "Both"
        if (t.toLowerCase().indexOf("mandi") !== -1) root.selectedItemType = "Mandi"
        else if (t.toLowerCase().indexOf("market") !== -1) root.selectedItemType = "Market"
        else root.selectedItemType = "Both"

        // Goods type
        var gt = item.goods_type ? item.goods_type.trim() : "Goods"
        var idx = goodsTypeCombo.find(gt)
        if (idx !== -1) goodsTypeCombo.currentIndex = idx

        // Trading group & unit
        if (item.trading_group) groupCombo.editText = item.trading_group
        if (item.unit) unitCombo.editText = item.unit

        if (item.rate_calc_on) {
            var rIdx = rateCalcCombo.find(item.rate_calc_on)
            if (rIdx !== -1) rateCalcCombo.currentIndex = rIdx
        }

        narrationInput.text = item.item_narration ? item.item_narration : ""
        capitalGoodsCheck.checked = (item.capital_goods === 1)

        // Tax
        hsnInput.text = item.hsn_code ? item.hsn_code : "1006"
        var gstVal = (item.gst_rate !== undefined && item.gst_rate !== null) ? item.gst_rate : 5.0
        var gstStr = gstVal.toString() + "%"
        var gIdx = gstRateCombo.find(gstStr)
        if (gIdx !== -1) gstRateCombo.currentIndex = gIdx
        else gstRateCombo.editText = gstStr

        cessInput.text = (item.cess_rate !== undefined) ? item.cess_rate.toString() : "0.00"
        gstLedgerCombo.editText = item.gst_ledger ? item.gst_ledger : "Duties & Taxes"
        vatInput.text = (item.vat_rate !== undefined) ? item.vat_rate.toString() : "0.00"
        vatLedgerCombo.editText = item.vat_ledger ? item.vat_ledger : "VAT A/c"
        surVatInput.text = (item.surcharge_on_vat !== undefined) ? item.surcharge_on_vat.toString() : "0.00"
        cstInput.text = (item.cst_rate !== undefined) ? item.cst_rate.toString() : "0.00"
        cstLedgerCombo.editText = item.cst_ledger ? item.cst_ledger : "CST A/c"

        // Mandi fees
        damiInput.text = (item.dami_rate !== undefined) ? item.dami_rate.toString() : "0.00"
        damiLedgerCombo.editText = item.dami_ledger ? item.dami_ledger : "Dami A/c"
        mktFeeInput.text = (item.market_fee_rate !== undefined) ? item.market_fee_rate.toString() : "0.00"
        mFeeLedgerCombo.editText = item.market_fee_ledger ? item.market_fee_ledger : "Market Fee A/c"
        hrdfInput.text = (item.hrdf_rate !== undefined) ? item.hrdf_rate.toString() : "0.00"
        hrdfLedgerCombo.editText = item.hrdf_ledger ? item.hrdf_ledger : "H.R.D.F. A/c"

        mktCommttFormCheck.checked = (item.market_commtt_form_apply === 1)
        mktCommttCouponCheck.checked = (item.market_commtt_coupon_apply === 1)
        damiWeightCheck.checked = (item.dami_calc_on_weight === 1)
        taxOnQtyCheck.checked = (item.tax_on_qty === 1)

        // Rates & Packing
        packingInput.text = (item.packing_kg !== undefined) ? item.packing_kg.toString() : "50.0"
        stockCalcCheck.checked = (item.calculate_stock !== 0)
        calcTradingCheck.checked = (item.include_in_trading !== 0)
        purcRateInput.text = (item.purchase_rate !== undefined) ? item.purchase_rate.toString() : "0.00"
        saleRateInput.text = (item.sale_rate !== undefined) ? item.sale_rate.toString() : "0.00"
        bonusInput.text = (item.bonus_approved !== undefined) ? item.bonus_approved.toString() : "0.00"
        opBagsInput.text = (item.opening_bags !== undefined) ? item.opening_bags.toString() : "0"
        opQtyInput.text = (item.opening_qty !== undefined) ? item.opening_qty.toString() : "0.000"

        // Ledgers
        if (item.purchase_ledger) purcLedgerCombo.editText = item.purchase_ledger
        if (item.purchase_return_ledger) purcRetLedgerCombo.editText = item.purchase_return_ledger
        if (item.sale_ledger) saleLedgerCombo.editText = item.sale_ledger
        if (item.sale_return_ledger) saleRetLedgerCombo.editText = item.sale_return_ledger
        if (item.stock_ledger) stockLedgerCombo.editText = item.stock_ledger

        // Labour Matrix
        if (item.labour_rate_unit) {
            var lIdx = labourUnitCombo.find(item.labour_rate_unit)
            if (lIdx !== -1) labourUnitCombo.currentIndex = lIdx
        }

        u1.text = (item.utrai_rate_1 !== undefined) ? item.utrai_rate_1.toString() : "0.00"
        j1.text = (item.jharai_rate_1 !== undefined) ? item.jharai_rate_1.toString() : "0.00"
        b1.text = (item.bharai_rate_1 !== undefined) ? item.bharai_rate_1.toString() : "0.00"
        t1.text = (item.tulai_rate_1 !== undefined) ? item.tulai_rate_1.toString() : "0.00"
        k1.text = (item.khichai_rate_1 !== undefined) ? item.khichai_rate_1.toString() : "0.00"
        s1.text = (item.silai_rate_1 !== undefined) ? item.silai_rate_1.toString() : "0.00"
        l1.text = (item.loading_rate_1 !== undefined) ? item.loading_rate_1.toString() : "0.00"

        u2.text = (item.utrai_rate_2 !== undefined) ? item.utrai_rate_2.toString() : "0.00"
        j2.text = (item.jharai_rate_2 !== undefined) ? item.jharai_rate_2.toString() : "0.00"
        b2.text = (item.bharai_rate_2 !== undefined) ? item.bharai_rate_2.toString() : "0.00"
        t2.text = (item.tulai_rate_2 !== undefined) ? item.tulai_rate_2.toString() : "0.00"
        k2.text = (item.khichai_rate_2 !== undefined) ? item.khichai_rate_2.toString() : "0.00"
        s2.text = (item.silai_rate_2 !== undefined) ? item.silai_rate_2.toString() : "0.00"
        l2.text = (item.loading_rate_2 !== undefined) ? item.loading_rate_2.toString() : "0.00"

        u3.text = (item.utrai_rate_3 !== undefined) ? item.utrai_rate_3.toString() : "0.00"
        j3.text = (item.jharai_rate_3 !== undefined) ? item.jharai_rate_3.toString() : "0.00"
        b3.text = (item.bharai_rate_3 !== undefined) ? item.bharai_rate_3.toString() : "0.00"
        t3.text = (item.tulai_rate_3 !== undefined) ? item.tulai_rate_3.toString() : "0.00"
        k3.text = (item.khichai_rate_3 !== undefined) ? item.khichai_rate_3.toString() : "0.00"
        s3.text = (item.silai_rate_3 !== undefined) ? item.silai_rate_3.toString() : "0.00"
        l3.text = (item.loading_rate_3 !== undefined) ? item.loading_rate_3.toString() : "0.00"

        statusToast.show("Loaded '" + item.name + "' (" + root.selectedItemType + " Type)")
    }

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

    function updateStockItem() {
        if (!nameInput.text.trim()) {
            statusToast.show("Please enter a valid Stock Item Name.")
            nameInput.forceActiveFocus()
            return
        }
        if (root.currentItemId <= 0) {
            statusToast.show("Please choose an existing stock item first.")
            selectItemCombo.focusAndOpen()
            return
        }
        confirmModal.open()
    }

    function executeUpdateStockItem() {
        var gstVal = parseFloat(gstRateCombo.currentText.replace("%", "")) || 5.0
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

        var success = stockItemsModel.update_stock_item_full(root.currentItemId, payload)
        if (success) {
            statusToast.show("Stock item updated successfully.")
            selectItemCombo.model = stockItemsModel.get_items_list()
            root.savedSuccess()
        } else {
            statusToast.show("Failed to update stock item. Please check the values.")
        }
    }

    function executeDeleteItem() {
        if (root.currentItemId > 0) {
            var ok = stockItemsModel.delete_stock_item(root.currentItemId)
            if (ok) {
                statusToast.show("Stock item deleted successfully.")
                root.currentItemId = -1
                nameInput.text = ""
                selectItemCombo.model = stockItemsModel.get_items_list()
                root.savedSuccess()
            } else {
                statusToast.show("Failed to delete stock item.")
            }
        }
    }

    // -------------------------------------------------------------
    // MODALS & POPUPS
    // -------------------------------------------------------------
    ConfirmationModal {
        id: confirmModal
        anchors.centerIn: parent
        titleText: "CONFIRM STOCK ITEM UPDATE"
        messageText: "Update stock item '" + nameInput.text.trim() + "' (" + selectedItemType + " Type)?"
        onConfirmed: root.executeUpdateStockItem()
    }

    ConfirmationModal {
        id: deleteConfirmModal
        anchors.centerIn: parent
        titleText: "CONFIRM DELETE STOCK ITEM"
        messageText: "Are you sure you want to permanently delete stock item '" + nameInput.text.trim() + "'?"
        onConfirmed: root.executeDeleteItem()
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

    Component.onCompleted: {
        Qt.callLater(function() {
            selectItemCombo.focusAndOpen()
        })
    }
}

