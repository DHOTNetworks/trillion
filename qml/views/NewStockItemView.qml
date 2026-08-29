import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../components"
import "../dialogs"

ScrollView {
    id: root
    contentWidth: availableWidth
    clip: true

    signal cancelRequested()
    signal savedSuccess()

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
                    text: "📦 Define New Inventory Stock Item"
                    color: "#0F172A"
                    font.pixelSize: 20
                    font.bold: true
                }
                Text {
                    text: "Define rice milling product details, GST rate, packing weight, opening valuation & accounting ledgers."
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
                        placeholderText: "e.g. Sona Masoori Steam Rice 26kg"
                        isRequired: true
                        focusInput: true
                        Layout.fillWidth: true
                        onReturnPressed: codeInput.focusInput = true
                    }

                    CustomInput {
                        id: codeInput
                        label: "Product Code / SKU"
                        placeholderText: "e.g. RICE-SONA-26"
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
                        placeholderText: "e.g. Mahadev Brand, Supreme Gold"
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
                        placeholderText: "26.0"
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
                        placeholderText: "0.00"
                        inputMethodHints: Qt.ImhFormattedNumbersOnly
                        Layout.fillWidth: true
                        onReturnPressed: saleRateInput.focusInput = true
                    }

                    CustomInput {
                        id: saleRateInput
                        label: "Selling Rate (₹)"
                        placeholderText: "0.00"
                        inputMethodHints: Qt.ImhFormattedNumbersOnly
                        Layout.fillWidth: true
                        onReturnPressed: mrpInput.focusInput = true
                    }

                    CustomInput {
                        id: mrpInput
                        label: "MRP (₹)"
                        placeholderText: "0.00"
                        inputMethodHints: Qt.ImhFormattedNumbersOnly
                        Layout.fillWidth: true
                        onReturnPressed: discountInput.focusInput = true
                    }

                    CustomInput {
                        id: discountInput
                        label: "Discount %"
                        placeholderText: "0.0"
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
                        placeholderText: "100630"
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
                        placeholderText: "0.0"
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
                        placeholderText: "0"
                        inputMethodHints: Qt.ImhDigitsOnly
                        Layout.fillWidth: true
                        onReturnPressed: opQtyInput.focusInput = true
                    }

                    CustomInput {
                        id: opQtyInput
                        label: "Opening Quantity (Qtl/Units)"
                        placeholderText: "0.0"
                        inputMethodHints: Qt.ImhFormattedNumbersOnly
                        Layout.fillWidth: true
                        onReturnPressed: opRateInput.focusInput = true
                    }

                    CustomInput {
                        id: opRateInput
                        label: "Opening Valuation Rate (₹)"
                        placeholderText: "0.00"
                        inputMethodHints: Qt.ImhFormattedNumbersOnly
                        Layout.fillWidth: true
                        onReturnPressed: opValInput.focusInput = true
                    }

                    CustomInput {
                        id: opValInput
                        label: "Opening Total Value (₹)"
                        placeholderText: "0.00"
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
                background: Rectangle { color: submitBtn.hovered ? "#1D4ED8" : "#2563EB"; radius: 6 }
                contentItem: RowLayout {
                    spacing: 6
                    Text { text: "💾 Save Stock Item"; color: "#FFFFFF"; font.bold: true; font.pixelSize: 14 }
                    KbdBadge { text: "Enter"; badgeColor: "#1E3A8A"; textColor: "#93C5FD"; borderColor: "#2563EB" }
                }
                onClicked: root.saveStockItem()
            }
        }
    }

    function saveStockItem() {
        if (!nameInput.text.trim()) return
        saveConfirmModal.open()
    }

    function executeSaveStockItem() {
        var gstVal = parseFloat(gstCombo.currentText.replace("%", "")) || 0.0
        var success = stockItemsModel.add_stock_item(
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
        if (success) {
            root.savedSuccess()
        }
    }

    ConfirmationModal {
        id: saveConfirmModal
        anchors.centerIn: parent
        titleText: "CONFIRM STOCK ITEM SAVE"
        messageText: "Are you sure you want to save & create Stock Item '" + nameInput.text.trim() + "' under type '" + typeCombo.currentText + "'?"
        onConfirmed: root.executeSaveStockItem()
    }
}
