import QtQuick
import QtQuick.Templates as T
import QtQuick.Layouts
import MahadevERP

T.Popup {
    id: root
    width: 520
    implicitHeight: mainCol.implicitHeight + 36
    modal: true
    dim: true
    focus: true
    anchors.centerIn: T.Overlay.overlay
    closePolicy: T.Popup.CloseOnPressOutside | T.Popup.CloseOnEscape

    property string mode: "print" // "print" or "pdf"
    property string invoiceNo: ""
    property string customerName: ""

    signal selected(string copyType)
    signal cancelled()

    onOpened: {
        Qt.callLater(function() { focusScope.forceActiveFocus() })
    }

    function chooseCopy(copyType) {
        root.close()
        root.selected(copyType)
    }

    function doCancel() {
        root.close()
        root.cancelled()
    }

    background: Rectangle {
        color: "#FFFFFF"
        border.color: root.mode === "print" ? "#059669" : "#0284C7"
        border.width: 2
        radius: 12
    }

    FocusScope {
        id: focusScope
        anchors.fill: parent
        focus: true

        Keys.onEscapePressed: function(event) { event.accepted = true; root.doCancel() }
        Keys.onPressed: function(event) {
            if (event.key === Qt.Key_1 || event.key === Qt.Key_O) {
                event.accepted = true
                root.chooseCopy("ORIGINAL FOR RECIPIENT")
            } else if (event.key === Qt.Key_2 || event.key === Qt.Key_D) {
                event.accepted = true
                root.chooseCopy("DUPLICATE FOR TRANSPORTER")
            } else if (event.key === Qt.Key_3 || event.key === Qt.Key_T) {
                event.accepted = true
                root.chooseCopy("TRIPLICATE FOR SUPPLIER")
            } else if (event.key === Qt.Key_4 || event.key === Qt.Key_A || event.key === Qt.Key_Return || event.key === Qt.Key_Enter) {
                event.accepted = true
                root.chooseCopy("ALL")
            }
        }

        ColumnLayout {
            id: mainCol
            anchors.fill: parent
            anchors.margins: 18
            spacing: 12

            // Header Row
            RowLayout {
                Layout.fillWidth: true
                spacing: 10

                Rectangle {
                    width: 38; height: 38; radius: 8
                    color: root.mode === "print" ? "#ECFDF5" : "#F0F9FF"
                    border.color: root.mode === "print" ? "#A7F3D0" : "#BAE6FD"
                    Text {
                        anchors.centerIn: parent
                        text: root.mode === "print" ? "" : ""
                        font.pixelSize: 18
                    }
                }

                ColumnLayout {
                    spacing: 2
                    Layout.fillWidth: true
                    Text {
                        text: root.mode === "print" ? "SELECT INVOICE COPY TO PRINT" : "SELECT INVOICE COPY TO SAVE AS PDF"
                        color: "#0F172A"
                        font.pixelSize: 15
                        font.bold: true
                    }
                    Text {
                        text: (root.invoiceNo ? "Invoice No: " + root.invoiceNo : "") + (root.customerName ? " | Buyer: " + root.customerName : "")
                        color: "#64748B"
                        font.pixelSize: 12
                        visible: root.invoiceNo !== ""
                        elide: Text.ElideRight
                        Layout.fillWidth: true
                    }
                }

                Rectangle {
                    width: 28; height: 28; radius: 6
                    color: closeMouse.containsMouse ? "#F1F5F9" : "transparent"
                    Text { anchors.centerIn: parent; text: "X"; color: "#64748B"; font.pixelSize: 14 }
                    MouseArea {
                        id: closeMouse
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: root.doCancel()
                    }
                }
            }

            Rectangle { Layout.fillWidth: true; height: 1; color: "#E2E8F0" }

            // Option 1: Original Copy (Buyer)
            Rectangle {
                Layout.fillWidth: true
                height: 48
                radius: 8
                color: opt1Mouse.containsMouse ? "#F8FAFC" : "#FFFFFF"
                border.color: opt1Mouse.containsMouse ? "#0284C7" : "#CBD5E1"
                border.width: opt1Mouse.containsMouse ? 1.5 : 1

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 12
                    anchors.rightMargin: 12
                    spacing: 12

                    Rectangle {
                        width: 26; height: 26; radius: 5
                        color: "#EFF6FF"
                        border.color: "#93C5FD"
                        Text { anchors.centerIn: parent; text: "1"; font.bold: true; color: "#1D4ED8"; font.pixelSize: 12 }
                    }

                    ColumnLayout {
                        spacing: 1
                        Layout.fillWidth: true
                        Text { text: "ORIGINAL FOR RECIPIENT"; font.bold: true; color: "#0F172A"; font.pixelSize: 13 }
                        Text { text: "Original tax invoice copy for the buyer / customer"; color: "#64748B"; font.pixelSize: 11 }
                    }

                    KbdBadge { text: "Key: 1"; badgeColor: "#F1F5F9"; textColor: "#475569"; borderColor: "#CBD5E1" }
                }

                MouseArea {
                    id: opt1Mouse
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: root.chooseCopy("ORIGINAL FOR RECIPIENT")
                }
            }

            // Option 2: Duplicate Copy (Transporter)
            Rectangle {
                Layout.fillWidth: true
                height: 48
                radius: 8
                color: opt2Mouse.containsMouse ? "#F8FAFC" : "#FFFFFF"
                border.color: opt2Mouse.containsMouse ? "#0284C7" : "#CBD5E1"
                border.width: opt2Mouse.containsMouse ? 1.5 : 1

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 12
                    anchors.rightMargin: 12
                    spacing: 12

                    Rectangle {
                        width: 26; height: 26; radius: 5
                        color: "#EFF6FF"
                        border.color: "#93C5FD"
                        Text { anchors.centerIn: parent; text: "2"; font.bold: true; color: "#1D4ED8"; font.pixelSize: 12 }
                    }

                    ColumnLayout {
                        spacing: 1
                        Layout.fillWidth: true
                        Text { text: "DUPLICATE FOR TRANSPORTER"; font.bold: true; color: "#0F172A"; font.pixelSize: 13 }
                        Text { text: "Duplicate copy for the driver / transporter / vehicle"; color: "#64748B"; font.pixelSize: 11 }
                    }

                    KbdBadge { text: "Key: 2"; badgeColor: "#F1F5F9"; textColor: "#475569"; borderColor: "#CBD5E1" }
                }

                MouseArea {
                    id: opt2Mouse
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: root.chooseCopy("DUPLICATE FOR TRANSPORTER")
                }
            }

            // Option 3: Triplicate Copy (Supplier)
            Rectangle {
                Layout.fillWidth: true
                height: 48
                radius: 8
                color: opt3Mouse.containsMouse ? "#F8FAFC" : "#FFFFFF"
                border.color: opt3Mouse.containsMouse ? "#0284C7" : "#CBD5E1"
                border.width: opt3Mouse.containsMouse ? 1.5 : 1

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 12
                    anchors.rightMargin: 12
                    spacing: 12

                    Rectangle {
                        width: 26; height: 26; radius: 5
                        color: "#EFF6FF"
                        border.color: "#93C5FD"
                        Text { anchors.centerIn: parent; text: "3"; font.bold: true; color: "#1D4ED8"; font.pixelSize: 12 }
                    }

                    ColumnLayout {
                        spacing: 1
                        Layout.fillWidth: true
                        Text { text: "TRIPLICATE FOR SUPPLIER"; font.bold: true; color: "#0F172A"; font.pixelSize: 13 }
                        Text { text: "Triplicate copy for firm / office / accounts records"; color: "#64748B"; font.pixelSize: 11 }
                    }

                    KbdBadge { text: "Key: 3"; badgeColor: "#F1F5F9"; textColor: "#475569"; borderColor: "#CBD5E1" }
                }

                MouseArea {
                    id: opt3Mouse
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: root.chooseCopy("TRIPLICATE FOR SUPPLIER")
                }
            }

            // Option 4: Both / All Copies
            Rectangle {
                Layout.fillWidth: true
                height: 48
                radius: 8
                color: opt4Mouse.containsMouse ? (root.mode === "print" ? "#047857" : "#0284C7") : (root.mode === "print" ? "#059669" : "#0EA5E9")

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 12
                    anchors.rightMargin: 12
                    spacing: 12

                    Rectangle {
                        width: 26; height: 26; radius: 5
                        color: "#FFFFFF"
                        Text { anchors.centerIn: parent; text: ""; font.pixelSize: 14 }
                    }

                    ColumnLayout {
                        spacing: 1
                        Layout.fillWidth: true
                        Text { text: root.mode === "print" ? "PRINT BOTH (ORIGINAL + DUPLICATE)" : "SAVE ALL COPIES (MULTI-PAGE PDF)"; font.bold: true; color: "#FFFFFF"; font.pixelSize: 13 }
                        Text { text: root.mode === "print" ? "Sends both Original and Duplicate to printer in 1 job" : "Saves Original + Duplicate together into a single PDF document"; color: "#E0F2FE"; font.pixelSize: 11 }
                    }

                    KbdBadge { text: "Enter ↵"; badgeColor: "#0F172A"; textColor: "#F8FAFC"; borderColor: "#334155" }
                }

                MouseArea {
                    id: opt4Mouse
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: root.chooseCopy("ALL")
                }
            }

            Rectangle { Layout.fillWidth: true; height: 1; color: "#E2E8F0" }

            // Bottom Action Row
            RowLayout {
                Layout.fillWidth: true
                Item { Layout.fillWidth: true }

                T.Button {
                    implicitWidth: 100
                    implicitHeight: 32
                    background: Rectangle {
                        color: parent.hovered ? "#F1F5F9" : "#FFFFFF"
                        border.color: "#CBD5E1"
                        radius: 6
                    }
                    contentItem: Text {
                        text: "Cancel (Esc)"
                        color: "#475569"
                        font.pixelSize: 12
                        font.bold: true
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                    onClicked: root.doCancel()
                }
            }
        }
    }
}
