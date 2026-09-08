import QtQuick
import QtQuick.Templates as T
import QtQuick.Layouts
import MahadevERP

T.Popup {
    id: root
    width: 340
    implicitHeight: mainCol.implicitHeight + 36
    modal: true
    dim: true
    focus: true
    closePolicy: T.Popup.CloseOnPressOutside | T.Popup.CloseOnEscape

    signal dateConfirmed(string formattedDate, string isoDate)
    signal cancelled()

    property string workingDateText: ""
    property string initialDate: ""
    property string errorMessage: ""

    onOpened: {
        errorMessage = ""
        var wDate = (typeof financialYearsModel !== "undefined" && financialYearsModel) ? financialYearsModel.get_working_date() : Qt.formatDate(new Date(), "dd-MM-yyyy")
        workingDateText = wDate
        if (initialDate && initialDate.trim() !== "") {
            dateInputField.text = initialDate.trim()
        } else {
            dateInputField.text = wDate
        }
        Qt.callLater(function() {
            dateInputField.forceActiveFocus()
            dateInputField.selectAll()
        })
    }

    onClosed: {
        errorMessage = ""
    }

    function openWithDate(currentDate) {
        initialDate = currentDate || ""
        root.open()
    }

    function validateAndAccept() {
        errorMessage = ""
        var inputVal = dateInputField.text.trim()
        if (!inputVal) {
            errorMessage = "Please enter a date."
            dateInputField.forceActiveFocus()
            return
        }

        if (typeof financialYearsModel !== "undefined" && financialYearsModel) {
            var res = financialYearsModel.validate_voucher_date(inputVal, workingDateText)
            if (!res.valid) {
                errorMessage = res.error || "Date is outside active Financial Year."
                dateInputField.forceActiveFocus()
                dateInputField.selectAll()
                return
            }
            financialYearsModel.set_working_date(res.formattedDate)
            root.close()
            root.dateConfirmed(res.formattedDate, res.isoDate)
        } else {
            root.close()
            root.dateConfirmed(inputVal, inputVal)
        }
    }

    background: Rectangle {
        color: "#E0F7FA" // Soft cyan / aqua background matching Bahi Khata
        border.color: "#0284C7"
        border.width: 2.5
        radius: 10
    }

    ColumnLayout {
        id: mainCol
        anchors.fill: parent
        anchors.margins: 14
        spacing: 6

        // Label: (Current Working Date)
        Text {
            text: "(Current Working Date)"
            color: "#0369A1"
            font.pixelSize: 13
            font.bold: true
            Layout.alignment: Qt.AlignHCenter
        }

        // Current Working Date value
        Text {
            text: root.workingDateText || "01-04-2026"
            color: "#082F49"
            font.pixelSize: 15
            font.bold: true
            font.letterSpacing: 0.5
            Layout.alignment: Qt.AlignHCenter
        }

        Item { Layout.preferredHeight: 4 }

        // Label: New Voucher Date
        Text {
            text: "New Voucher Date"
            color: "#0369A1"
            font.pixelSize: 14
            font.bold: true
            font.underline: true
            Layout.alignment: Qt.AlignHCenter
        }

        // Date Input Box
        Rectangle {
            Layout.preferredWidth: 200
            Layout.preferredHeight: 38
            Layout.alignment: Qt.AlignHCenter
            color: "#FFFFFF"
            border.color: dateInputField.activeFocus ? "#EAB308" : "#2563EB"
            border.width: 2
            radius: 4

            T.TextField {
                id: dateInputField
                anchors.fill: parent
                horizontalAlignment: TextInput.AlignHCenter
                verticalAlignment: TextInput.AlignVCenter
                color: "#0F172A"
                font.pixelSize: 16
                font.bold: true
                selectByMouse: true
                inputMethodHints: Qt.ImhDigitsOnly

                Keys.onReturnPressed: function(event) {
                    event.accepted = true
                    root.validateAndAccept()
                }
                Keys.onEnterPressed: function(event) {
                    event.accepted = true
                    root.validateAndAccept()
                }
                Keys.onEscapePressed: function(event) {
                    event.accepted = true
                    root.close()
                    root.cancelled()
                }
            }
        }

        // Error message if date is invalid or out of active FY range
        Text {
            visible: root.errorMessage !== ""
            text: root.errorMessage
            color: "#DC2626"
            font.pixelSize: 11
            font.bold: true
            wrapMode: Text.Wrap
            horizontalAlignment: Text.AlignHCenter
            Layout.fillWidth: true
            Layout.topMargin: 2
        }

        // Instructions
        Text {
            text: "Press Enter to Accept • Esc to Cancel"
            color: "#475569"
            font.pixelSize: 10
            Layout.alignment: Qt.AlignHCenter
            Layout.topMargin: 2
        }
    }
}
