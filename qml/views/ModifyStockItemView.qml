import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../components"

ScrollView {
    id: root
    contentWidth: availableWidth
    clip: true

    signal cancelRequested()
    signal savedSuccess()

    property int currentItemId: -1

    ColumnLayout {
        width: root.availableWidth > 0 ? root.availableWidth : 1200
        spacing: 20

        // Page Header Bar
        RowLayout {
            Layout.fillWidth: true
            spacing: 12

            ColumnLayout {
                spacing: 2
                Text {
                    text: "✏️ Modify Existing Inventory Stock Item"
                    color: "#0F172A"
                    font.pixelSize: 20
                    font.bold: true
                }
                Text {
                    text: "Select any stock item to edit pricing, GST rate, packing weight, opening valuation & accounting ledgers."
                    color: "#64748B"
                    font.pixelSize: 12
                }
            }

            Item { Layout.fillWidth: true }

            Button {
                id: backBtn
                background: Rectangle { color: "#F1F5F9"; radius: 6; border.color: "#CBD5E1" }
                contentItem: RowLayout {
                    spacing: 6
                    Text { text: "← Back to Dashboard"; color: "#475569"; font.pixelSize: 13; font.bold: true }
                    KbdBadge { text: "Esc"; badgeColor: "#DC2626"; textColor: "#FFF"; borderColor: "#B91C1C" }
                }
                onClicked: root.cancelRequested()
            }
        }

        Rectangle { Layout.fillWidth: true; height: 1; color: "#E2E8F0" }

        // STOCK ITEM SELECTOR CARD
        Rectangle {
            Layout.fillWidth: true
            implicitHeight: selectCol.implicitHeight + 28
            color: "#EFF6FF"
            border.color: "#BFDBFE"
            border.width: 1
            radius: 10
            z: 100

            ColumnLayout {
                id: selectCol
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.top: parent.top
                anchors.margins: 14
                spacing: 10

                Text {
                    text: "SELECT INVENTORY STOCK ITEM TO MODIFY"
                    color: "#2563EB"
                    font.pixelSize: 11
                    font.bold: true
                    font.letterSpacing: 1.0
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 14

                    CustomWhiteCombo {
                        id: selectItemCombo
                        label: "Choose Stock Item *"
                        Layout.fillWidth: true
                        model: (typeof stockItemsModel !== "undefined" && stockItemsModel) ? stockItemsModel.get_items_list() : []
                        onCurrentTextChanged: {
                            if (!currentText) return
                            var item = stockItemsModel.get_item_by_name(currentText)
                            if (item && item.name) {
                                root.currentItemId = item.id
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
                            }
                        }
                    }
                }
            }
        }

        // SECTION 1: PRIMARY ITEM CLASSIFICATION
        Rectangle {
            Layout.fillWidth: true
            implicitHeight: secCol1.implicitHeight + 28
            color: "#FFFFFF"
            border.color: "#E2E8F0"
            border.width: 1
            radius: 10

            ColumnLayout {
                id: secCol1
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.top: parent.top
                anchors.margins: 14
                spacing: 12

                Text {
                    text: "1. PRIMARY ITEM CLASSIFICATION & BRANDING"
                    color: "#2563EB"
                    font.pixelSize: 11
                    font.bold: true
                    font.letterSpacing: 1.0
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 12

                    CustomInput {
                        id: nameInput
                        label: "Stock Item Name *"
                        text: ""
                        isRequired: true
                        focusInput: true
                        Layout.fillWidth: true
                        onReturnPressed: codeInput.focusInput = true
                    }

                    CustomInput {
                        id: codeInput
                        label: "Product Code / SKU"
                        text: ""
                        Layout.preferredWidth: 200
                        onReturnPressed: typeCombo.focusAndOpen()
                    }
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 12

                    CustomWhiteCombo {
                        id: typeCombo
                        label: "Item Type *"
                        Layout.fillWidth: true
                        model: (typeof stockItemsModel !== "undefined" && stockItemsModel) ? stockItemsModel.get_item_types() : []
                        onReturnPressed: companyInput.focusInput = true
                    }

                    CustomInput {
                        id: companyInput
                        label: "Brand / Manufacturer"
                        text: ""
                        Layout.fillWidth: true
                        onReturnPressed: unitCombo.focusAndOpen()
                    }

                    CustomWhiteCombo {
                        id: unitCombo
                        label: "Measurement Unit *"
                        Layout.preferredWidth: 150
                        model: (typeof stockItemsModel !== "undefined" && stockItemsModel) ? stockItemsModel.get_units() : []
                        onReturnPressed: packingInput.focusInput = true
                    }

                    CustomInput {
                        id: packingInput
                        label: "Packing Wt (kg/bag)"
                        text: ""
                        inputMethodHints: Qt.ImhFormattedNumbersOnly
                        Layout.preferredWidth: 150
                        onReturnPressed: purRateInput.focusInput = true
                    }
                }
            }
        }

        // SECTION 2: PRICING & GST TAX COMPLIANCE
        Rectangle {
            Layout.fillWidth: true
            implicitHeight: secCol2.implicitHeight + 28
            color: "#FFFFFF"
            border.color: "#E2E8F0"
            border.width: 1
            radius: 10

            ColumnLayout {
                id: secCol2
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.top: parent.top
                anchors.margins: 14
                spacing: 12

                Text {
                    text: "2. PRICING & GST TAX COMPLIANCE"
                    color: "#16A34A"
                    font.pixelSize: 11
                    font.bold: true
                    font.letterSpacing: 1.0
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 12

                    CustomInput {
                        id: purRateInput
                        label: "Purchase Rate (₹)"
                        text: ""
                        inputMethodHints: Qt.ImhFormattedNumbersOnly
                        Layout.fillWidth: true
                        onReturnPressed: saleRateInput.focusInput = true
                    }

                    CustomInput {
                        id: saleRateInput
                        label: "Selling Rate (₹)"
                        text: ""
                        inputMethodHints: Qt.ImhFormattedNumbersOnly
                        Layout.fillWidth: true
                        onReturnPressed: mrpInput.focusInput = true
                    }

                    CustomInput {
                        id: mrpInput
                        label: "MRP (₹)"
                        text: ""
                        inputMethodHints: Qt.ImhFormattedNumbersOnly
                        Layout.fillWidth: true
                        onReturnPressed: discountInput.focusInput = true
                    }

                    CustomInput {
                        id: discountInput
                        label: "Discount %"
                        text: ""
                        inputMethodHints: Qt.ImhFormattedNumbersOnly
                        Layout.preferredWidth: 120
                        onReturnPressed: hsnInput.focusInput = true
                    }
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 12

                    CustomInput {
                        id: hsnInput
                        label: "HSN / SAC Code"
                        text: ""
                        Layout.fillWidth: true
                        onReturnPressed: gstCombo.focusAndOpen()
                    }

                    CustomWhiteCombo {
                        id: gstCombo
                        label: "GST Rate % *"
                        Layout.preferredWidth: 150
                        model: (typeof stockItemsModel !== "undefined" && stockItemsModel) ? stockItemsModel.get_gst_rates() : []
                        onReturnPressed: cessInput.focusInput = true
                    }

                    CustomInput {
                        id: cessInput
                        label: "Cess %"
                        text: ""
                        inputMethodHints: Qt.ImhFormattedNumbersOnly
                        Layout.preferredWidth: 140
                        onReturnPressed: opBagsInput.focusInput = true
                    }
                }
            }
        }

        // SECTION 3: INITIAL STOCK & VALUATION
        Rectangle {
            Layout.fillWidth: true
            implicitHeight: secCol3.implicitHeight + 28
            color: "#FFFFFF"
            border.color: "#E2E8F0"
            border.width: 1
            radius: 10

            ColumnLayout {
                id: secCol3
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.top: parent.top
                anchors.margins: 14
                spacing: 12

                Text {
                    text: "3. INITIAL STOCK & VALUATION"
                    color: "#D97706"
                    font.pixelSize: 11
                    font.bold: true
                    font.letterSpacing: 1.0
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 12

                    CustomInput {
                        id: opBagsInput
                        label: "Opening Bags"
                        text: ""
                        inputMethodHints: Qt.ImhDigitsOnly
                        Layout.fillWidth: true
                        onReturnPressed: opQtyInput.focusInput = true
                    }

                    CustomInput {
                        id: opQtyInput
                        label: "Opening Quantity (Qtl/Units)"
                        text: ""
                        inputMethodHints: Qt.ImhFormattedNumbersOnly
                        Layout.fillWidth: true
                        onReturnPressed: opRateInput.focusInput = true
                    }

                    CustomInput {
                        id: opRateInput
                        label: "Opening Valuation Rate (₹)"
                        text: ""
                        inputMethodHints: Qt.ImhFormattedNumbersOnly
                        Layout.fillWidth: true
                        onReturnPressed: opValInput.focusInput = true
                    }

                    CustomInput {
                        id: opValInput
                        label: "Opening Total Value (₹)"
                        text: ""
                        inputMethodHints: Qt.ImhFormattedNumbersOnly
                        Layout.fillWidth: true
                        onReturnPressed: purLedgerCombo.focusAndOpen()
                    }
                }
            }
        }

        // SECTION 4: FINANCIAL LEDGER POSTINGS
        Rectangle {
            Layout.fillWidth: true
            implicitHeight: secCol4.implicitHeight + 28
            color: "#FFFFFF"
            border.color: "#E2E8F0"
            border.width: 1
            radius: 10

            ColumnLayout {
                id: secCol4
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.top: parent.top
                anchors.margins: 14
                spacing: 12

                Text {
                    text: "4. FINANCIAL LEDGER POSTINGS"
                    color: "#7C3AED"
                    font.pixelSize: 11
                    font.bold: true
                    font.letterSpacing: 1.0
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 12

                    CustomWhiteCombo {
                        id: purLedgerCombo
                        label: "Purchase Posting Account"
                        Layout.fillWidth: true
                        model: (typeof partiesModel !== "undefined" && partiesModel) ? partiesModel.get_account_groups() : []
                        onReturnPressed: saleLedgerCombo.focusAndOpen()
                    }

                    CustomWhiteCombo {
                        id: saleLedgerCombo
                        label: "Sales Posting Account"
                        Layout.fillWidth: true
                        model: (typeof partiesModel !== "undefined" && partiesModel) ? partiesModel.get_account_groups() : []
                        onReturnPressed: stockLedgerCombo.focusAndOpen()
                    }

                    CustomWhiteCombo {
                        id: stockLedgerCombo
                        label: "Stock Valuation Account"
                        Layout.fillWidth: true
                        model: (typeof partiesModel !== "undefined" && partiesModel) ? partiesModel.get_account_groups() : []
                        onReturnPressed: submitBtn.focus = true
                    }
                }
            }
        }

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
                background: Rectangle { color: submitBtn.hovered ? "#15803D" : "#16A34A"; radius: 6 }
                contentItem: RowLayout {
                    spacing: 6
                    Text { text: "💾 Update Stock Item"; color: "#FFFFFF"; font.bold: true; font.pixelSize: 14 }
                    KbdBadge { text: "Enter"; badgeColor: "#14532D"; textColor: "#86EFAC"; borderColor: "#16A34A" }
                }
                onClicked: {
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
            }
        }
    }
}
