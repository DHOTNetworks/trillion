import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../components"

Popup {
    id: root
    width: 440
    implicitHeight: mainCol.implicitHeight + 36
    modal: true
    dim: true
    focus: true
    anchors.centerIn: Overlay.overlay
    closePolicy: Popup.CloseOnPressOutside | Popup.CloseOnEscape

    property string titleText: "CONFIRM SAVE ENTRY"
    property string messageText: "Are you sure you want to save & post this entry into the database?"
    property string confirmBtnText: "Yes (Enter)"
    property string cancelBtnText: "No (Esc)"

    signal confirmed()
    signal cancelled()

    onOpened: {
        Qt.callLater(function() { focusScope.forceActiveFocus() })
    }

    function doConfirm() {
        root.close()
        root.confirmed()
    }

    function doCancel() {
        root.close()
        root.cancelled()
    }

    background: Rectangle {
        color: "#FFFFFF"
        border.color: "#2563EB"
        border.width: 2.5
        radius: 12
    }

    contentItem: FocusScope {
        id: focusScope
        anchors.fill: parent
        focus: true

        Keys.onReturnPressed: function(event) { event.accepted = true; root.doConfirm() }
        Keys.onEnterPressed: function(event) { event.accepted = true; root.doConfirm() }
        Keys.onEscapePressed: function(event) { event.accepted = true; root.doCancel() }
        Keys.onPressed: function(event) {
            if (event.key === Qt.Key_Y) {
                event.accepted = true
                root.doConfirm()
            } else if (event.key === Qt.Key_N) {
                event.accepted = true
                root.doCancel()
            }
        }

        ColumnLayout {
            id: mainCol
            anchors.fill: parent
            anchors.margins: 18
            spacing: 14

            // Header Row
            RowLayout {
                Layout.fillWidth: true
                spacing: 8

                Rectangle {
                    width: 32; height: 32; radius: 6
                    color: "#EFF6FF"
                    border.color: "#93C5FD"
                    Text { anchors.centerIn: parent; text: "💾"; font.pixelSize: 16 }
                }

                Text {
                    text: root.titleText
                    color: "#0F172A"
                    font.pixelSize: 15
                    font.bold: true
                    font.letterSpacing: 0.5
                }

                Item { Layout.fillWidth: true }
            }

            Rectangle { Layout.fillWidth: true; height: 1; color: "#E2E8F0" }

            // Message Body
            Text {
                Layout.fillWidth: true
                text: root.messageText
                color: "#334155"
                font.pixelSize: 13
                font.bold: true
                wrapMode: Text.WordWrap
            }

            Item { Layout.preferredHeight: 4 }

            // Action Buttons Row
            RowLayout {
                Layout.fillWidth: true
                spacing: 12

                Item { Layout.fillWidth: true }

                Button {
                    id: cancelBtn
                    background: Rectangle { color: cancelBtn.hovered ? "#CBD5E1" : "#E2E8F0"; radius: 6; border.color: "#94A3B8" }
                    contentItem: RowLayout {
                        spacing: 6
                        Text { text: root.cancelBtnText; color: "#334155"; font.bold: true; font.pixelSize: 12 }
                        KbdBadge { text: "Esc / N"; badgeColor: "#64748B"; textColor: "#FFF"; borderColor: "#475569" }
                    }
                    onClicked: root.doCancel()
                }

                Button {
                    id: confirmBtn
                    background: Rectangle { color: confirmBtn.hovered ? "#15803D" : "#16A34A"; radius: 6 }
                    contentItem: RowLayout {
                        spacing: 6
                        Text { text: root.confirmBtnText; color: "#FFFFFF"; font.bold: true; font.pixelSize: 12 }
                        KbdBadge { text: "Enter / Y"; badgeColor: "#15803D"; textColor: "#FFF"; borderColor: "#166534" }
                    }
                    onClicked: root.doConfirm()
                }
            }
        }
    }
}
