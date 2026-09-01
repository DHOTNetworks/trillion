import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../components"
import "../dialogs"

Item {
    id: root
    anchors.fill: parent

    signal cancelRequested()
    signal savedSuccess()

    property int currentItemId: -1

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 14
        spacing: 10

        // Page Header Bar
        RowLayout {
            Layout.fillWidth: true
            spacing: 12

            ColumnLayout {
                spacing: 1
                Text {
                    text: "✏️ Modify Existing Inventory Stock Item"
                    color: "#0F172A"
                    font.pixelSize: 18
                    font.bold: true
                }
                Text {
                    text: "Single-slate item selector, pricing, GST rates, packing weight, opening valuation & accounting ledgers"
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

        Rectangle { Layout.fillWidth: true; height: 1; color: "#E2E8F0" }

        // STOCK ITEM SELECTOR CARD
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
                        label: "🔍 CHOOSE STOCK ITEM TO MODIFY (Type Name, ↑/↓ Arrows & Enter) *"
                        Layout.fillWidth: true
                        focusInput: true
                        model: (typeof stockItemsModel !== "undefined" && stockItemsModel) ? stockItemsModel.get_items_list() : []
                        onReturnPressed: root.loadSelectedItem(selectItemCombo.currentText)
                        onDownPressed: nameInput.focusInput = true
                    }
                }
            }
        }

        // CARD 1: PRIMARY ITEM CLASSIFICATION & PRICING
        Rectangle {
            Layout.fillWidth: true
            implicitHeight: secCol1.implicitHeight + 20
            color: "#FFFFFF"
            border.color: "#E2E8F0"
            border.width: 1
            radius: 8

            ColumnLayout {
                id: secCol1
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.top: parent.top
                anchors.margins: 10
                spacing: 8

                Text {
                    text: "1. PRIMARY ITEM CLASSIFICATION & PRICING"
                    color: "#2563EB"
                    font.pixelSize: 11
                    font.bold: true
                    font.letterSpacing: 0.8
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 8

                    CustomInput {
                        id: nameInput
                        label: "Stock Item Name *"
                        text: ""
                        isRequired: true
                        Layout.fillWidth: true
                        onReturnPressed: codeInput.focusInput = true
                        onRightPressed: codeInput.focusInput = true
                        onUpPressed: selectItemCombo.focusAndOpen()
                        onDownPressed: unitCombo.focusAndOpen()
                    }

                    CustomInput {
                        id: codeInput
                        label: "Product Code / SKU"
                        text: ""
                        Layout.preferredWidth: 150
                        onReturnPressed: typeCombo.focusAndOpen()
                        onLeftPressed: nameInput.focusInput = true
                        onRightPressed: typeCombo.focusAndOpen()
                        onUpPressed: selectItemCombo.focusAndOpen()
                        onDownPressed: packingInput.focusInput = true
                    }

                    CustomWhiteCombo {
                        id: typeCombo
                        label: "Item Type *"
                        Layout.preferredWidth: 180
                        model: (typeof stockItemsModel !== "undefined" && stockItemsModel) ? stockItemsModel.get_item_types() : []
                        onReturnPressed: companyInput.focusInput = true
                        onLeftPressed: codeInput.focusInput = true
                        onRightPressed: companyInput.focusInput = true
                        onUpPressed: selectItemCombo.focusAndOpen()
                        onDownPressed: purRateInput.focusInput = true
                    }

                    CustomInput {
                        id: companyInput
                        label: "Brand / Manufacturer"
                        text: ""
                        Layout.preferredWidth: 220
                        onReturnPressed: unitCombo.focusAndOpen()
                        onLeftPressed: typeCombo.focusAndOpen()
                        onUpPressed: selectItemCombo.focusAndOpen()
                        onDownPressed: saleRateInput.focusInput = true
                    }
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 8

                    CustomWhiteCombo {
                        id: unitCombo
                        label: "Measurement Unit *"
                        Layout.preferredWidth: 130
                        model: (typeof stockItemsModel !== "undefined" && stockItemsModel) ? stockItemsModel.get_units() : []
                        onReturnPressed: packingInput.focusInput = true
                        onRightPressed: packingInput.focusInput = true
                        onUpPressed: nameInput.focusInput = true
                        onDownPressed: mrpInput.focusInput = true
                    }

                    CustomInput {
                        id: packingInput
                        label: "Packing Wt (kg/bag)"
                        text: ""
                        inputMethodHints: Qt.ImhFormattedNumbersOnly
                        Layout.preferredWidth: 140
                        onReturnPressed: purRateInput.focusInput = true
                        onLeftPressed: unitCombo.focusAndOpen()
                        onRightPressed: purRateInput.focusInput = true
                        onUpPressed: codeInput.focusInput = true
                        onDownPressed: discountInput.focusInput = true
                    }

                    CustomInput {
                        id: purRateInput
                        label: "Purchase Rate (₹)"
                        text: ""
                        inputMethodHints: Qt.ImhFormattedNumbersOnly
                        Layout.fillWidth: true
                        onReturnPressed: saleRateInput.focusInput = true
                        onLeftPressed: packingInput.focusInput = true
                        onRightPressed: saleRateInput.focusInput = true
                        onUpPressed: typeCombo.focusAndOpen()
                        onDownPressed: hsnInput.focusInput = true
                    }

                    CustomInput {
                        id: saleRateInput
                        label: "Selling Rate (₹)"
                        text: ""
                        inputMethodHints: Qt.ImhFormattedNumbersOnly
                        Layout.fillWidth: true
                        onReturnPressed: mrpInput.focusInput = true
                        onLeftPressed: purRateInput.focusInput = true
                        onUpPressed: companyInput.focusInput = true
                        onDownPressed: gstCombo.focusAndOpen()
                    }
                }
            }
        }

        // CARD 2: GST TAX COMPLIANCE & INITIAL STOCK VALUATION
        Rectangle {
            Layout.fillWidth: true
            implicitHeight: secCol2.implicitHeight + 20
            color: "#FFFFFF"
            border.color: "#E2E8F0"
            border.width: 1
            radius: 8

            ColumnLayout {
                id: secCol2
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.top: parent.top
                anchors.margins: 10
                spacing: 8

                Text {
                    text: "2. GST TAX COMPLIANCE & INITIAL STOCK VALUATION"
                    color: "#16A34A"
                    font.pixelSize: 11
                    font.bold: true
                    font.letterSpacing: 0.8
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 8

                    CustomInput {
                        id: mrpInput
                        label: "MRP (₹)"
                        text: ""
                        inputMethodHints: Qt.ImhFormattedNumbersOnly
                        Layout.fillWidth: true
                        onReturnPressed: discountInput.focusInput = true
                        onRightPressed: discountInput.focusInput = true
                        onUpPressed: unitCombo.focusAndOpen()
                        onDownPressed: opBagsInput.focusInput = true
                    }

                    CustomInput {
                        id: discountInput
                        label: "Discount %"
                        text: ""
                        inputMethodHints: Qt.ImhFormattedNumbersOnly
                        Layout.preferredWidth: 110
                        onReturnPressed: hsnInput.focusInput = true
                        onLeftPressed: mrpInput.focusInput = true
                        onRightPressed: hsnInput.focusInput = true
                        onUpPressed: packingInput.focusInput = true
                        onDownPressed: opQtyInput.focusInput = true
                    }

                    CustomInput {
                        id: hsnInput
                        label: "HSN / SAC Code"
                        text: ""
                        Layout.preferredWidth: 150
                        onReturnPressed: gstCombo.focusAndOpen()
                        onLeftPressed: discountInput.focusInput = true
                        onRightPressed: gstCombo.focusAndOpen()
                        onUpPressed: purRateInput.focusInput = true
                        onDownPressed: opRateInput.focusInput = true
                    }

                    CustomWhiteCombo {
                        id: gstCombo
                        label: "GST Rate % *"
                        Layout.preferredWidth: 130
                        model: (typeof stockItemsModel !== "undefined" && stockItemsModel) ? stockItemsModel.get_gst_rates() : []
                        onReturnPressed: cessInput.focusInput = true
                        onLeftPressed: hsnInput.focusInput = true
                        onRightPressed: cessInput.focusInput = true
                        onUpPressed: saleRateInput.focusInput = true
                        onDownPressed: opValInput.focusInput = true
                    }

                    CustomInput {
                        id: cessInput
                        label: "Cess %"
                        text: ""
                        inputMethodHints: Qt.ImhFormattedNumbersOnly
                        Layout.preferredWidth: 100
                        onReturnPressed: opBagsInput.focusInput = true
                        onLeftPressed: gstCombo.focusAndOpen()
                        onUpPressed: saleRateInput.focusInput = true
                        onDownPressed: opValInput.focusInput = true
                    }
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 8

                    CustomInput {
                        id: opBagsInput
                        label: "Opening Bags"
                        text: ""
                        inputMethodHints: Qt.ImhDigitsOnly
                        Layout.fillWidth: true
                        onReturnPressed: opQtyInput.focusInput = true
                        onRightPressed: opQtyInput.focusInput = true
                        onUpPressed: mrpInput.focusInput = true
                        onDownPressed: purLedgerCombo.focusAndOpen()
                    }

                    CustomInput {
                        id: opQtyInput
                        label: "Opening Quantity (Qtl)"
                        text: ""
                        inputMethodHints: Qt.ImhFormattedNumbersOnly
                        Layout.fillWidth: true
                        onReturnPressed: opRateInput.focusInput = true
                        onLeftPressed: opBagsInput.focusInput = true
                        onRightPressed: opRateInput.focusInput = true
                        onUpPressed: discountInput.focusInput = true
                        onDownPressed: saleLedgerCombo.focusAndOpen()
                    }

                    CustomInput {
                        id: opRateInput
                        label: "Opening Rate (₹)"
                        text: ""
                        inputMethodHints: Qt.ImhFormattedNumbersOnly
                        Layout.fillWidth: true
                        onReturnPressed: opValInput.focusInput = true
                        onLeftPressed: opQtyInput.focusInput = true
                        onRightPressed: opValInput.focusInput = true
                        onUpPressed: hsnInput.focusInput = true
                        onDownPressed: stockLedgerCombo.focusAndOpen()
                    }

                    CustomInput {
                        id: opValInput
                        label: "Opening Valuation (₹)"
                        text: ""
                        inputMethodHints: Qt.ImhFormattedNumbersOnly
                        Layout.fillWidth: true
                        onReturnPressed: purLedgerCombo.focusAndOpen()
                        onLeftPressed: opRateInput.focusInput = true
                        onUpPressed: gstCombo.focusAndOpen()
                        onDownPressed: stockLedgerCombo.focusAndOpen()
                    }
                }
            }
        }

        // CARD 3: FINANCIAL LEDGER POSTINGS
        Rectangle {
            Layout.fillWidth: true
            implicitHeight: secCol3.implicitHeight + 20
            color: "#FFFFFF"
            border.color: "#E2E8F0"
            border.width: 1
            radius: 8

            ColumnLayout {
                id: secCol3
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.top: parent.top
                anchors.margins: 10
                spacing: 8

                Text {
                    text: "3. FINANCIAL LEDGER POSTINGS"
                    color: "#7C3AED"
                    font.pixelSize: 11
                    font.bold: true
                    font.letterSpacing: 0.8
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 8

                    CustomWhiteCombo {
                        id: purLedgerCombo
                        label: "Purchase Posting Account"
                        Layout.fillWidth: true
                        model: (typeof partiesModel !== "undefined" && partiesModel) ? partiesModel.get_account_groups() : []
                        onReturnPressed: saleLedgerCombo.focusAndOpen()
                        onRightPressed: saleLedgerCombo.focusAndOpen()
                        onUpPressed: opBagsInput.focusInput = true
                        onDownPressed: submitBtn.focus = true
                    }

                    CustomWhiteCombo {
                        id: saleLedgerCombo
                        label: "Sales Posting Account"
                        Layout.fillWidth: true
                        model: (typeof partiesModel !== "undefined" && partiesModel) ? partiesModel.get_account_groups() : []
                        onReturnPressed: stockLedgerCombo.focusAndOpen()
                        onLeftPressed: purLedgerCombo.focusAndOpen()
                        onRightPressed: stockLedgerCombo.focusAndOpen()
                        onUpPressed: opQtyInput.focusInput = true
                        onDownPressed: submitBtn.focus = true
                    }

                    CustomWhiteCombo {
                        id: stockLedgerCombo
                        label: "Stock Valuation Account"
                        Layout.fillWidth: true
                        model: (typeof partiesModel !== "undefined" && partiesModel) ? partiesModel.get_account_groups() : []
                        onReturnPressed: root.saveUpdateStockItem()
                        onLeftPressed: saleLedgerCombo.focusAndOpen()
                        onUpPressed: opValInput.focusInput = true
                        onDownPressed: submitBtn.focus = true
                    }
                }
            }
        }

        Item { Layout.fillHeight: true }

        // SAVE & CANCEL ACTION BAR
        RowLayout {
            Layout.fillWidth: true
            spacing: 14

            Button {
                id: cancelBottomBtn
                background: Rectangle { color: "#F1F5F9"; radius: 6; border.color: "#CBD5E1" }
                contentItem: RowLayout {
                    spacing: 6
                    Text { text: "Cancel"; color: "#475569"; font.pixelSize: 13 }
                    KbdBadge { text: "Esc"; badgeColor: "#DC2626"; textColor: "#FFF"; borderColor: "#B91C1C" }
                }
                onClicked: root.cancelRequested()
            }

            Item { Layout.fillWidth: true }

            Button {
                id: submitBtn
                background: Rectangle { color: (submitBtn.hovered || submitBtn.activeFocus) ? "#15803D" : "#16A34A"; radius: 6; border.color: submitBtn.activeFocus ? "#86EFAC" : "transparent"; border.width: 2 }
                contentItem: RowLayout {
                    spacing: 6
                    Text { text: "💾 Update Complete Stock Item"; color: "#FFFFFF"; font.bold: true; font.pixelSize: 14 }
                    KbdBadge { text: "Enter"; badgeColor: "#14532D"; textColor: "#86EFAC"; borderColor: "#16A34A" }
                }
                Keys.onReturnPressed: root.saveUpdateStockItem()
                Keys.onEnterPressed: root.saveUpdateStockItem()
                onClicked: root.saveUpdateStockItem()
            }
        }
    }

    function loadSelectedItem(itemName) {
        if (!itemName || !itemName.trim()) return
        var item = (typeof stockItemsModel !== "undefined" && stockItemsModel) ? stockItemsModel.get_item_by_name(itemName.trim()) : {}
        if (!item || !item.name) return

        root.currentItemId = item.id ? item.id : -1
        nameInput.text = item.name ? item.name : ""
        codeInput.text = item.code ? item.code : ""
        typeCombo.editText = item.item_type ? item.item_type : ""
        companyInput.text = item.company_name ? item.company_name : ""
        unitCombo.editText = item.unit ? item.unit : ""
        packingInput.text = (item.packing_kg !== undefined && item.packing_kg !== null) ? item.packing_kg.toString() : "26.0"
        
        purRateInput.text = (item.purchase_rate !== undefined && item.purchase_rate !== null) ? item.purchase_rate.toString() : "0.00"
        saleRateInput.text = (item.sale_rate !== undefined && item.sale_rate !== null) ? item.sale_rate.toString() : "0.00"
        mrpInput.text = (item.mrp !== undefined && item.mrp !== null) ? item.mrp.toString() : "0.00"
        discountInput.text = (item.discount !== undefined && item.discount !== null) ? item.discount.toString() : "0.0"
        
        hsnInput.text = item.hsn_code ? item.hsn_code : ""
        gstCombo.editText = (item.gst_rate !== undefined && item.gst_rate !== null) ? item.gst_rate.toString() + "%" : "0%"
        cessInput.text = (item.cess_rate !== undefined && item.cess_rate !== null) ? item.cess_rate.toString() : "0.0"
        
        opBagsInput.text = (item.opening_bags !== undefined && item.opening_bags !== null) ? item.opening_bags.toString() : "0"
        opQtyInput.text = (item.opening_qty !== undefined && item.opening_qty !== null) ? item.opening_qty.toString() : "0.0"
        opRateInput.text = (item.opening_rate !== undefined && item.opening_rate !== null) ? item.opening_rate.toString() : "0.00"
        opValInput.text = (item.opening_value !== undefined && item.opening_value !== null) ? item.opening_value.toString() : "0.00"
        
        purLedgerCombo.editText = item.purchase_ledger ? item.purchase_ledger : ""
        saleLedgerCombo.editText = item.sale_ledger ? item.sale_ledger : ""
        stockLedgerCombo.editText = item.stock_ledger ? item.stock_ledger : ""

        nameInput.focusInput = true
    }

    function saveUpdateStockItem() {
        if (!nameInput.text.trim()) return
        if (root.currentItemId > 0) {
            var gstVal = parseFloat(gstCombo.currentText.replace("%", "")) || 0.0
            stockItemsModel.update_stock_item(
                root.currentItemId,
                nameInput.text,
                codeInput.text,
                typeCombo.currentText,
                companyInput.text,
                unitCombo.currentText,
                parseFloat(purRateInput.text) || 0.0,
                parseFloat(saleRateInput.text) || 0.0,
                parseFloat(mrpInput.text) || 0.0,
                parseFloat(discountInput.text) || 0.0,
                hsnInput.text,
                gstVal,
                parseFloat(cessInput.text) || 0.0,
                parseFloat(packingInput.text) || 26.0,
                parseInt(opBagsInput.text) || 0,
                parseFloat(opQtyInput.text) || 0.0,
                parseFloat(opRateInput.text) || 0.0,
                parseFloat(opValInput.text) || 0.0,
                purLedgerCombo.currentText,
                saleLedgerCombo.currentText,
                stockLedgerCombo.currentText
            )
        }
        root.savedSuccess()
    }

    Component.onCompleted: {
        Qt.callLater(function() {
            selectItemCombo.focusAndOpen()
        })
    }
}
