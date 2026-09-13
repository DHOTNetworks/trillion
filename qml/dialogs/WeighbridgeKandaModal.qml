import QtQuick
import QtQuick.Templates as T
import QtQuick.Layouts
import MahadevERP
import "../components"

Rectangle {
    id: root
    width: 820
    height: 680
    color: "#FFFFFF"
    radius: 12
    border.color: "#CBD5E1"
    border.width: 1

    property int editDispatchId: 0
    property var invoiceSearchList: []
    property real loadedNetWeight: 0.0
    property real loadedFreight: 0.0
    property real loadedAdvance: 0.0
    property real calculatedNetWeight: 0.0
    property real calculatedTotalFreight: 0.0
    property real calculatedBalanceFreight: 0.0

    signal closeRequested()
    signal savedSuccess()

    function loadDispatch(id) {
        root.editDispatchId = id
        if (id > 0 && typeof transportDispatchCtrl !== "undefined") {
            var data = transportDispatchCtrl.getDispatch(id)
            if (data && data.slipNo) {
                root.loadedNetWeight = (data.netWeightQtl && data.netWeightQtl > 0) ? data.netWeightQtl : 0.0
                root.loadedFreight = (data.totalFreight && data.totalFreight > 0) ? data.totalFreight : 0.0
                root.loadedAdvance = (data.advanceFreight && data.advanceFreight > 0) ? data.advanceFreight : 0.0

                slipNoInput.text = data.slipNo || ""
                dateInput.text = data.dispatchDate || ""
                timeInput.text = data.dispatchTime || ""
                invoiceInput.text = data.invoiceNo || ""
                partyInput.text = data.partyName || ""
                itemInput.text = data.itemName || ""
                gradeInput.text = data.grade || ""
                vehicleInput.text = data.vehicleNo || ""
                driverInput.text = data.driverName || ""
                phoneInput.text = data.driverPhone || ""
                transporterInput.text = data.transporterName || ""
                transporterGstinInput.text = data.transporterGstin || ""
                grNoInput.text = data.grNo || ""
                grDateInput.text = data.grDate || ""
                destInput.text = data.destination || ""
                distInput.text = data.distanceKm ? data.distanceKm.toString() : "0"
                bagCountInput.text = data.bagCount ? data.bagCount.toString() : "0"
                packingKgInput.text = data.packingKg ? data.packingKg.toString() : "50.0"
                grossWeightInput.text = (data.grossWeightQtl && data.grossWeightQtl > 0) ? data.grossWeightQtl.toString() : ""
                tareWeightInput.text = (data.tareWeightQtl && data.tareWeightQtl > 0) ? data.tareWeightQtl.toString() : ""
                bagTareInput.text = (data.bagTareKg && data.bagTareKg > 0) ? data.bagTareKg.toString() : "0.00"
                
                var cType = data.freightCalcType || "Per Qtl"
                if (cType === "Per Bag") calcTypeCombo.currentIndex = 1
                else if (cType === "Fixed") calcTypeCombo.currentIndex = 2
                else calcTypeCombo.currentIndex = 0

                freightRateInput.text = (data.freightRate && data.freightRate > 0) ? data.freightRate.toString() : "0.00"
                advanceFreightInput.text = (data.advanceFreight && data.advanceFreight > 0) ? data.advanceFreight.toString() : "0.00"
                ewayInput.text = data.ewayBillNo || ""
                irnInput.text = data.irnNo || ""
                notesInput.text = data.notes || ""
                
                recalcAll()
                return
            }
        }
        // New Record Defaults
        resetForm()
    }

    function resetForm() {
        root.editDispatchId = 0
        root.loadedNetWeight = 0.0
        root.loadedFreight = 0.0
        root.loadedAdvance = 0.0
        root.calculatedNetWeight = 0.0
        root.calculatedTotalFreight = 0.0
        root.calculatedBalanceFreight = 0.0

        if (typeof transportDispatchCtrl !== "undefined") {
            slipNoInput.text = transportDispatchCtrl.getNextSlipNo()
        } else {
            slipNoInput.text = "KND-0001"
        }
        
        var today = new Date()
        var yyyy = today.getFullYear()
        var mm = String(today.getMonth() + 1).padStart(2, '0')
        var dd = String(today.getDate()).padStart(2, '0')
        dateInput.text = yyyy + "-" + mm + "-" + dd
        
        var hh = String(today.getHours()).padStart(2, '0')
        var min = String(today.getMinutes()).padStart(2, '0')
        timeInput.text = hh + ":" + min

        invoiceInput.text = ""
        partyInput.text = ""
        itemInput.text = ""
        gradeInput.text = ""
        vehicleInput.text = ""
        driverInput.text = ""
        phoneInput.text = ""
        transporterInput.text = ""
        transporterGstinInput.text = ""
        grNoInput.text = ""
        grDateInput.text = yyyy + "-" + mm + "-" + dd
        destInput.text = ""
        distInput.text = "0"
        bagCountInput.text = "0"
        packingKgInput.text = "50.0"
        grossWeightInput.text = ""
        tareWeightInput.text = ""
        bagTareInput.text = "0.00"
        calcTypeCombo.currentIndex = 0
        freightRateInput.text = "0.00"
        advanceFreightInput.text = "0.00"
        ewayInput.text = ""
        irnInput.text = ""
        notesInput.text = ""
        recalcAll()
    }

    function recalcAll() {
        var gross = parseFloat(grossWeightInput.text) || 0.0
        var tare = parseFloat(tareWeightInput.text) || 0.0
        var bags = parseInt(bagCountInput.text) || 0
        var packKg = parseFloat(packingKgInput.text) || 50.0
        var bagTare = parseFloat(bagTareInput.text) || 0.0

        var netQtl = 0.0
        if (gross > 0 || tare > 0) {
            if (typeof transportDispatchCtrl !== "undefined") {
                netQtl = transportDispatchCtrl.calculateNetWeight(gross, tare, bags, bagTare)
            } else {
                var rawNet = gross - tare
                var totalBagTareQtl = (bags * bagTare) / 100.0
                netQtl = Math.max(0.0, rawNet - totalBagTareQtl)
            }
        } else if (root.loadedNetWeight > 0) {
            netQtl = root.loadedNetWeight
        } else if (bags > 0 && packKg > 0) {
            netQtl = (bags * packKg) / 100.0
        }
        root.calculatedNetWeight = netQtl
        netWeightDisplay.text = netQtl.toFixed(2) + " Qtl"

        var cType = calcTypeCombo.currentText || "Per Qtl"
        var rate = parseFloat(freightRateInput.text) || 0.0
        var adv = parseFloat(advanceFreightInput.text) || 0.0

        var tot = 0.0
        var bal = 0.0
        var st = "Unpaid"

        if (rate > 0) {
            if (typeof transportDispatchCtrl !== "undefined") {
                var fResult = transportDispatchCtrl.calculateFreight(cType, rate, netQtl, bags, adv)
                if (fResult) {
                    tot = fResult.totalFreight || 0.0
                    bal = fResult.balanceFreight || 0.0
                    st = fResult.paymentStatus || "Unpaid"
                }
            } else {
                if (cType === "Per Qtl") tot = netQtl * rate
                else if (cType === "Per Bag") tot = bags * rate
                else tot = rate
                bal = Math.max(0.0, tot - adv)
                st = (bal <= 0 && tot > 0) ? "Settled" : (adv > 0 ? "Partially Paid" : "Unpaid")
            }
        } else if (root.loadedFreight > 0) {
            tot = root.loadedFreight
            bal = Math.max(0.0, tot - adv)
            st = (bal <= 0) ? "Settled" : (adv > 0 ? "Partially Paid" : "Unpaid")
        } else {
            tot = 0.0
            bal = 0.0
            st = "Settled"
        }

        root.calculatedTotalFreight = tot
        root.calculatedBalanceFreight = bal
        totalFreightDisplay.text = "Rs. " + tot.toFixed(2)
        balanceFreightDisplay.text = "Rs. " + bal.toFixed(2)
        statusBadge.text = st
    }

    function doSave() {
        if (!vehicleInput.text.trim()) {
            vehicleInput.focusInput = true
            return
        }

        var gross = parseFloat(grossWeightInput.text) || 0.0
        var tare = parseFloat(tareWeightInput.text) || 0.0
        var bags = parseInt(bagCountInput.text) || 0
        var bagTare = parseFloat(bagTareInput.text) || 0.0
        var rate = parseFloat(freightRateInput.text) || 0.0
        var adv = parseFloat(advanceFreightInput.text) || 0.0
        var dist = parseInt(distInput.text) || 0
        var packKg = parseFloat(packingKgInput.text) || 50.0

        var payload = {
            "id": root.editDispatchId,
            "slipNo": slipNoInput.text.trim(),
            "dispatchDate": dateInput.text.trim(),
            "dispatchTime": timeInput.text.trim(),
            "invoiceNo": invoiceInput.text.trim(),
            "partyName": partyInput.text.trim(),
            "itemName": itemInput.text.trim(),
            "grade": gradeInput.text.trim(),
            "vehicleNo": vehicleInput.text.trim().toUpperCase(),
            "driverName": driverInput.text.trim(),
            "driverPhone": phoneInput.text.trim(),
            "transporterName": transporterInput.text.trim(),
            "transporterGstin": transporterGstinInput.text.trim().toUpperCase(),
            "grNo": grNoInput.text.trim(),
            "grDate": grDateInput.text.trim(),
            "destination": destInput.text.trim(),
            "distanceKm": dist,
            "bagCount": bags,
            "packingKg": packKg,
            "grossWeightQtl": gross,
            "tareWeightQtl": tare,
            "bagTareKg": bagTare,
            "netWeightQtl": root.calculatedNetWeight,
            "freightCalcType": calcTypeCombo.currentText,
            "freightRate": rate,
            "totalFreight": root.calculatedTotalFreight,
            "advanceFreight": adv,
            "balanceFreight": root.calculatedBalanceFreight,
            "freightPaymentStatus": statusBadge.text,
            "ewayBillNo": ewayInput.text.trim(),
            "irnNo": irnInput.text.trim(),
            "notes": notesInput.text.trim()
        }

        if (typeof transportDispatchCtrl !== "undefined") {
            var res = transportDispatchCtrl.saveDispatch(payload)
            if (res && res.success) {
                root.savedSuccess()
                root.closeRequested()
            }
        }
    }

    Component.onCompleted: {
        resetForm()
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 20
        spacing: 12

        // Modal Header
        RowLayout {
            Layout.fillWidth: true
            spacing: 12

            Rectangle {
                width: 36
                height: 36
                radius: 8
                color: "#EFF6FF"
                border.color: "#BFDBFE"
                border.width: 1
                Text {
                    anchors.centerIn: parent
                    text: "K"
                    color: "#2563EB"
                    font.bold: true
                    font.pixelSize: 18
                }
            }

            ColumnLayout {
                spacing: 2
                Text {
                    text: root.editDispatchId > 0 ? "Edit Weighbridge (Kanda) Slip" : "New Weighbridge (Kanda) & Transport Entry"
                    color: "#0F172A"
                    font.pixelSize: 17
                    font.bold: true
                }
                Text {
                    text: "Physical gross/tare weighment, transport logistics, and freight payable ledger"
                    color: "#64748B"
                    font.pixelSize: 12
                }
            }

            Item { Layout.fillWidth: true }

            T.Button {
                flat: true
                text: "X"
                font.pixelSize: 16
                font.bold: true
                onClicked: root.closeRequested()
            }
        }

        Rectangle { Layout.fillWidth: true; height: 1; color: "#E2E8F0" }

        // Live KPI Calculation Banner
        RowLayout {
            Layout.fillWidth: true
            spacing: 12

            Rectangle {
                Layout.fillWidth: true
                height: 64
                radius: 8
                color: "#F0FDF4"
                border.color: "#BBF7D0"
                border.width: 1

                RowLayout {
                    anchors.fill: parent
                    anchors.margins: 10
                    spacing: 8

                    ColumnLayout {
                        spacing: 2
                        Text { text: "NET WEIGHT (KANDA)"; color: "#166534"; font.pixelSize: 11; font.bold: true }
                        Text {
                            id: netWeightDisplay
                            text: "0.00 Qtl"
                            color: "#15803D"
                            font.pixelSize: 18
                            font.bold: true
                        }
                    }
                }
            }

            Rectangle {
                Layout.fillWidth: true
                height: 64
                radius: 8
                color: "#EFF6FF"
                border.color: "#BFDBFE"
                border.width: 1

                RowLayout {
                    anchors.fill: parent
                    anchors.margins: 10
                    spacing: 8

                    ColumnLayout {
                        spacing: 2
                        Text { text: "TOTAL FREIGHT"; color: "#1E40AF"; font.pixelSize: 11; font.bold: true }
                        Text {
                            id: totalFreightDisplay
                            text: "Rs. 0.00"
                            color: "#1D4ED8"
                            font.pixelSize: 18
                            font.bold: true
                        }
                    }
                }
            }

            Rectangle {
                Layout.fillWidth: true
                height: 64
                radius: 8
                color: "#FEF2F2"
                border.color: "#FECACA"
                border.width: 1

                RowLayout {
                    anchors.fill: parent
                    anchors.margins: 10
                    spacing: 8

                    ColumnLayout {
                        spacing: 2
                        Text { text: "BALANCE PAYABLE"; color: "#991B1B"; font.pixelSize: 11; font.bold: true }
                        Text {
                            id: balanceFreightDisplay
                            text: "Rs. 0.00"
                            color: "#DC2626"
                            font.pixelSize: 18
                            font.bold: true
                        }
                    }
                }
            }

            Rectangle {
                width: 110
                height: 64
                radius: 8
                color: "#F8FAFC"
                border.color: "#E2E8F0"
                border.width: 1

                ColumnLayout {
                    anchors.centerIn: parent
                    spacing: 4
                    Text { text: "STATUS"; color: "#64748B"; font.pixelSize: 10; font.bold: true; Layout.alignment: Qt.AlignHCenter }
                    Text {
                        id: statusBadge
                        text: "Unpaid"
                        color: "#0F172A"
                        font.pixelSize: 12
                        font.bold: true
                        Layout.alignment: Qt.AlignHCenter
                    }
                }
            }
        }

        // Main Form Scroll Area
        T.ScrollView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true

            T.ScrollBar.vertical: T.ScrollBar {
                policy: T.ScrollBar.AsNeeded
            }

            ColumnLayout {
                width: parent.width - 12
                spacing: 14

                // Section 1: Dispatch & Vehicle Identifiers
                Text { text: "1. Dispatch & Vehicle Details"; font.bold: true; font.pixelSize: 13; color: "#334155" }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 10

                    CustomInput {
                        id: slipNoInput
                        label: "Slip No *"
                        placeholderText: "KND-0001"
                        Layout.preferredWidth: 140
                    }

                    CustomInput {
                        id: dateInput
                        label: "Date (YYYY-MM-DD) *"
                        placeholderText: "YYYY-MM-DD"
                        Layout.preferredWidth: 140
                    }

                    CustomInput {
                        id: timeInput
                        label: "Time"
                        placeholderText: "HH:MM"
                        Layout.preferredWidth: 100
                    }

                    CustomInput {
                        id: invoiceInput
                        label: "Invoice No"
                        placeholderText: "Link Sale Bill"
                        Layout.fillWidth: true
                    }
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 10

                    CustomInput {
                        id: vehicleInput
                        label: "Vehicle Truck No *"
                        placeholderText: "e.g. PB03AJ1982"
                        isRequired: true
                        focusInput: true
                        Layout.preferredWidth: 200
                        onTextChanged: recalcAll()
                    }

                    CustomInput {
                        id: partyInput
                        label: "Consignee / Buyer Name"
                        placeholderText: "e.g. Haryana Food Corp"
                        Layout.fillWidth: true
                    }

                    CustomInput {
                        id: itemInput
                        label: "Item Dispatched"
                        placeholderText: "e.g. Basmati Sella 1121"
                        Layout.preferredWidth: 180
                    }

                    CustomInput {
                        id: gradeInput
                        label: "Grade / Brand"
                        placeholderText: "e.g. Platinum"
                        Layout.preferredWidth: 120
                    }
                }

                // Section 2: Transporter & Driver Information
                Text { text: "2. Transporter & Driver Information"; font.bold: true; font.pixelSize: 13; color: "#334155" }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 10

                    CustomInput {
                        id: transporterInput
                        label: "Transporter Agency"
                        placeholderText: "e.g. Royal Golden Transport"
                        Layout.fillWidth: true
                    }

                    CustomInput {
                        id: transporterGstinInput
                        label: "Transporter GSTIN"
                        placeholderText: "03AABCR1234F1Z1"
                        Layout.preferredWidth: 180
                    }

                    CustomInput {
                        id: grNoInput
                        label: "GR / Bilty No"
                        placeholderText: "GR-8921"
                        Layout.preferredWidth: 140
                    }

                    CustomInput {
                        id: grDateInput
                        label: "GR Date"
                        placeholderText: "YYYY-MM-DD"
                        Layout.preferredWidth: 130
                    }
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 10

                    CustomInput {
                        id: driverInput
                        label: "Driver Name"
                        placeholderText: "e.g. Gurpreet Singh"
                        Layout.fillWidth: true
                    }

                    CustomInput {
                        id: phoneInput
                        label: "Driver Phone"
                        placeholderText: "9876543210"
                        inputMethodHints: Qt.ImhDigitsOnly
                        Layout.preferredWidth: 180
                    }

                    CustomInput {
                        id: destInput
                        label: "Destination Station"
                        placeholderText: "e.g. Gandhidham Port"
                        Layout.fillWidth: true
                    }

                    CustomInput {
                        id: distInput
                        label: "Distance (KM)"
                        placeholderText: "0"
                        inputMethodHints: Qt.ImhDigitsOnly
                        Layout.preferredWidth: 110
                    }
                }

                // Section 3: Physical Weighbridge (Kanda) Math
                Text { text: "3. Physical Weighbridge (Kanda) Weights"; font.bold: true; font.pixelSize: 13; color: "#334155" }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 10

                    CustomInput {
                        id: bagCountInput
                        label: "Bag Count *"
                        placeholderText: "0"
                        inputMethodHints: Qt.ImhDigitsOnly
                        Layout.preferredWidth: 130
                        onTextChanged: recalcAll()
                    }

                    CustomInput {
                        id: packingKgInput
                        label: "Packing Size (Kg)"
                        placeholderText: "50.0"
                        inputMethodHints: Qt.ImhFormattedNumbersOnly
                        Layout.preferredWidth: 130
                        onTextChanged: recalcAll()
                    }

                    CustomInput {
                        id: grossWeightInput
                        label: "Gross Weight (Qtl) *"
                        placeholderText: "0.00"
                        inputMethodHints: Qt.ImhFormattedNumbersOnly
                        Layout.fillWidth: true
                        onTextChanged: recalcAll()
                    }

                    CustomInput {
                        id: tareWeightInput
                        label: "Tare Weight (Qtl) *"
                        placeholderText: "0.00"
                        inputMethodHints: Qt.ImhFormattedNumbersOnly
                        Layout.fillWidth: true
                        onTextChanged: recalcAll()
                    }

                    CustomInput {
                        id: bagTareInput
                        label: "Bag Tare Deduction (Kg/Bag)"
                        placeholderText: "0.00"
                        inputMethodHints: Qt.ImhFormattedNumbersOnly
                        Layout.preferredWidth: 160
                        onTextChanged: recalcAll()
                    }
                }

                // Section 4: Freight Payable & Statutory
                Text { text: "4. Freight Calculation & Statutory e-Way"; font.bold: true; font.pixelSize: 13; color: "#334155" }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 10

                    CustomWhiteCombo {
                        id: calcTypeCombo
                        label: "Freight Mode"
                        Layout.preferredWidth: 140
                        model: ["Per Qtl", "Per Bag", "Fixed"]
                        onCurrentTextChanged: recalcAll()
                    }

                    CustomInput {
                        id: freightRateInput
                        label: "Freight Rate (Rs.)"
                        placeholderText: "0.00"
                        inputMethodHints: Qt.ImhFormattedNumbersOnly
                        Layout.preferredWidth: 150
                        onTextChanged: recalcAll()
                    }

                    CustomInput {
                        id: advanceFreightInput
                        label: "Advance Paid (Rs.)"
                        placeholderText: "0.00"
                        inputMethodHints: Qt.ImhFormattedNumbersOnly
                        Layout.preferredWidth: 150
                        onTextChanged: recalcAll()
                    }

                    CustomInput {
                        id: ewayInput
                        label: "e-Way Bill No"
                        placeholderText: "12 Digits e-Way"
                        Layout.fillWidth: true
                    }

                    CustomInput {
                        id: irnInput
                        label: "e-Invoice IRN"
                        placeholderText: "64 Hex Digits"
                        Layout.fillWidth: true
                    }
                }

                CustomInput {
                    id: notesInput
                    label: "Remarks / Gate Pass Notes"
                    placeholderText: "e.g. Moisture 12.5%, Clean Truck, Verified by Gate Incharge"
                    Layout.fillWidth: true
                }
            }
        }

        Rectangle { Layout.fillWidth: true; height: 1; color: "#E2E8F0" }

        // Action Buttons Footer
        RowLayout {
            Layout.fillWidth: true
            spacing: 12

            T.Button {
                implicitWidth: contentItem.implicitWidth + 24
                implicitHeight: 34
                background: Rectangle { color: "#F1F5F9"; radius: 6; border.color: "#CBD5E1" }
                contentItem: Text { text: "Cancel (Esc)"; color: "#475569"; font.bold: true; font.pixelSize: 12; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                onClicked: root.closeRequested()
            }

            Item { Layout.fillWidth: true }

            T.Button {
                id: savePrintBtn
                background: Rectangle { color: savePrintBtn.hovered ? "#047857" : "#059669"; radius: 6 }
                contentItem: RowLayout {
                    spacing: 6
                    Text { text: "Save & Print Slip"; color: "#FFFFFF"; font.bold: true; font.pixelSize: 13 }
                    KbdBadge { text: "Ctrl+P"; badgeColor: "#064E3B"; textColor: "#6EE7B7"; borderColor: "#059669" }
                }
                onClicked: {
                    doSave()
                }
            }

            T.Button {
                id: saveBtn
                background: Rectangle { color: saveBtn.hovered ? "#1D4ED8" : "#2563EB"; radius: 6 }
                contentItem: RowLayout {
                    spacing: 6
                    Text { text: "Save Dispatch Record"; color: "#FFFFFF"; font.bold: true; font.pixelSize: 13 }
                    KbdBadge { text: "Ctrl+S"; badgeColor: "#1E3A8A"; textColor: "#93C5FD"; borderColor: "#2563EB" }
                }
                onClicked: {
                    doSave()
                }
            }
        }
    }

    Shortcut {
        sequence: "Ctrl+S"
        onActivated: doSave()
    }
    Shortcut {
        sequence: "Escape"
        onActivated: root.closeRequested()
    }
}
