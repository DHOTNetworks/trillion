import QtQuick
import QtQuick.Templates as T
import QtQuick.Layouts
import MahadevERP

T.Popup {
    id: root
    width: Math.min(parent.width - 40, 920)
    height: Math.min(parent.height - 40, 680)
    modal: true
    focus: true
    closePolicy: T.Popup.CloseOnEscape

    signal firmCreated(string firmId, string firmName)

    background: Rectangle {
        color: "#FFFFFF"
        radius: 12
        border.color: "#CBD5E1"
        border.width: 1
    }

    onOpened: {
        compNameInput.focusInput = true
    }

    function createFirm() {
        var name = compNameInput.text.trim()
        if (!name) {
            errorText.text = "❌ Company / Firm Name is required."
            return
        }
        errorText.text = ""

        var info = {
            company_name: name,
            firm_type: firmTypeCombo.currentText,
            business_type: businessInput.text.trim(),
            gstin: gstinInput.text.trim(),
            pan_no: panInput.text.trim(),
            ml_no: mlNoInput.text.trim(),
            fssai_no: fssaiInput.text.trim(),
            address: addressInput.text.trim(),
            city: cityInput.text.trim() || "Sirsa",
            state: stateInput.text.trim() || "Haryana",
            state_code: "06",
            pincode: pinInput.text.trim() || "125055",
            phone: phoneInput.text.trim(),
            mobile: mobileInput.text.trim(),
            bank_name: bankNameInput.text.trim(),
            bank_account: bankAccInput.text.trim(),
            ifsc_code: ifscInput.text.trim(),
            books_from: booksFromInput.text.trim() || "2026-04-01",
            acc_year_from: "2026-04-01",
            acc_year_to: "2027-03-31",
            fy_name: "FY 2026-27"
        }

        if (typeof firmManager !== "undefined" && firmManager) {
            var ok = firmManager.create_new_firm(info)
            if (ok) {
                root.close()
                root.firmCreated(firmManager.currentFirmId, name)
            } else {
                errorText.text = "❌ Failed to create firm. Check if database already exists."
            }
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 20
        spacing: 12

        // Header Bar
        RowLayout {
            Layout.fillWidth: true
            Rectangle {
                width: 36; height: 36; radius: 8
                color: "#EFF6FF"; border.color: "#BFDBFE"
                Text { anchors.centerIn: parent; text: "🏢"; font.pixelSize: 18 }
            }
            ColumnLayout {
                spacing: 2
                Text { text: "CREATE NEW COMPANY / FIRM"; color: "#0F172A"; font.pixelSize: 16; font.bold: true }
                Text { text: "Initializes a dedicated SQLite database and master ledger accounts."; color: "#64748B"; font.pixelSize: 11 }
            }
            Item { Layout.fillWidth: true }
            Text {
                id: errorText
                color: "#DC2626"; font.pixelSize: 12; font.bold: true
            }
            T.Button {
                background: Rectangle { color: "#F1F5F9"; radius: 6 }
                contentItem: Text { text: "✕"; color: "#64748B"; font.bold: true }
                onClicked: root.close()
            }
        }

        Rectangle { Layout.fillWidth: true; height: 1; color: "#E2E8F0" }

        // Form Fields in ScrollView
        T.ScrollView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            contentWidth: availableWidth
            clip: true

            ColumnLayout {
                width: parent.width
                spacing: 14

                // Section 1: Basic Identity
                Text { text: "1. FIRM IDENTITY & LEGAL TYPE"; color: "#2563EB"; font.pixelSize: 11; font.bold: true; font.letterSpacing: 0.5 }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 12

                    CustomInput {
                        id: compNameInput
                        Layout.fillWidth: true
                        label: "Company / Firm Full Name *"
                        placeholderText: "e.g. M/s Shree Ganesh Rice Mills"
                    }

                    ColumnLayout {
                        spacing: 4
                        Layout.preferredWidth: 240
                        Text { text: "Legal Entity Type"; color: "#0F172A"; font.pixelSize: 11; font.bold: true }
                        T.ComboBox {
                            id: firmTypeCombo
                            Layout.fillWidth: true
                            implicitHeight: 32
                            model: ["Partnership Firm", "Proprietorship Firm", "Private Limited Company", "Limited Liability Partnership (LLP)", "Individual"]
                        }
                    }
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 12

                    CustomInput {
                        id: businessInput
                        Layout.fillWidth: true
                        label: "Nature of Business / Trade"
                        placeholderText: "e.g. Rice Mill & Grain Processing, Commission Agent"
                    }

                    CustomInput {
                        id: booksFromInput
                        Layout.preferredWidth: 160
                        label: "Books Beginning Date"
                        text: "2026-04-01"
                    }
                }

                // Section 2: Taxation & Licenses
                Text { text: "2. GST, PAN & STATUTORY LICENSES"; color: "#2563EB"; font.pixelSize: 11; font.bold: true; font.letterSpacing: 0.5 }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 12

                    CustomInput {
                        id: gstinInput
                        Layout.fillWidth: true
                        label: "GSTIN Number (15 Digits)"
                        placeholderText: "06AAAAA0000A1Z5"
                    }

                    CustomInput {
                        id: panInput
                        Layout.fillWidth: true
                        label: "PAN Number (10 Digits)"
                        placeholderText: "AAAAA0000A"
                    }

                    CustomInput {
                        id: mlNoInput
                        Layout.fillWidth: true
                        label: "Mandi License No (ML No)"
                        placeholderText: "5025/SRS/BOARD"
                    }

                    CustomInput {
                        id: fssaiInput
                        Layout.fillWidth: true
                        label: "FSSAI License No"
                        placeholderText: "10822019000152"
                    }
                }

                // Section 3: Address & Contact
                Text { text: "3. ADDRESS & COMMUNICATIONS"; color: "#2563EB"; font.pixelSize: 11; font.bold: true; font.letterSpacing: 0.5 }

                CustomInput {
                    id: addressInput
                    Layout.fillWidth: true
                    label: "Factory / Office Address"
                    placeholderText: "Plot / Shop No, Industrial Area / Mandi Road"
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 12

                    CustomInput {
                        id: cityInput
                        Layout.fillWidth: true
                        label: "City / Station"
                        text: "Sirsa"
                    }

                    CustomInput {
                        id: stateInput
                        Layout.fillWidth: true
                        label: "State"
                        text: "Haryana"
                    }

                    CustomInput {
                        id: pinInput
                        Layout.preferredWidth: 120
                        label: "Pincode"
                        text: "125055"
                    }

                    CustomInput {
                        id: mobileInput
                        Layout.fillWidth: true
                        label: "Mobile Number"
                        placeholderText: "9876543210"
                    }

                    CustomInput {
                        id: phoneInput
                        Layout.fillWidth: true
                        label: "Office Phone"
                        placeholderText: "01666-xxxxxx"
                    }
                }

                // Section 4: Banking Details
                Text { text: "4. BANK ACCOUNT DETAILS"; color: "#2563EB"; font.pixelSize: 11; font.bold: true; font.letterSpacing: 0.5 }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 12

                    CustomInput {
                        id: bankNameInput
                        Layout.fillWidth: true
                        label: "Bank Name & Branch"
                        placeholderText: "e.g. State Bank of India, Sirsa Main Branch"
                    }

                    CustomInput {
                        id: bankAccInput
                        Layout.fillWidth: true
                        label: "Account Number"
                        placeholderText: "128001400717"
                    }

                    CustomInput {
                        id: ifscInput
                        Layout.preferredWidth: 160
                        label: "IFSC Code"
                        placeholderText: "SBIN0001234"
                    }
                }
            }
        }

        Rectangle { Layout.fillWidth: true; height: 1; color: "#E2E8F0" }

        // Action Buttons Footer
        RowLayout {
            Layout.fillWidth: true
            spacing: 12

            T.Button {
                background: Rectangle { color: "#F1F5F9"; radius: 6 }
                contentItem: Text { text: "Cancel (Esc)"; color: "#475569"; font.bold: true }
                onClicked: root.close()
            }

            Item { Layout.fillWidth: true }

            T.Button {
                Layout.preferredWidth: 260
                height: 36
                background: Rectangle { color: "#16A34A"; radius: 6 }
                contentItem: Text { text: "💾 Create & Open Firm (F2)"; color: "#FFFFFF"; font.bold: true; font.pixelSize: 12; horizontalAlignment: Text.AlignHCenter }
                onClicked: root.createFirm()
            }
        }
    }
}
