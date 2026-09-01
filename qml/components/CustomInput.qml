import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ColumnLayout {
    id: root
    property string label: ""
    property string placeholderText: ""
    property alias text: textInput.text
    property bool isRequired: false
    property alias focusInput: textInput.focus
    property alias isFocused: textInput.activeFocus
    property alias inputMethodHints: textInput.inputMethodHints

    signal returnPressed()
    signal leftPressed()
    signal rightPressed()
    signal upPressed()
    signal downPressed()

    function focusAndSelect() {
        textInput.forceActiveFocus()
        textInput.selectAll()
    }

    spacing: 4

    RowLayout {
        spacing: 4
        visible: root.label !== ""
        Text {
            text: root.label
            color: "#334155"
            font.pixelSize: 12
            font.bold: true
        }
        Text {
            text: "*"
            color: "#DC2626"
            font.pixelSize: 12
            font.bold: true
            visible: root.isRequired
        }
    }

    Rectangle {
        Layout.fillWidth: true
        implicitHeight: 36
        radius: 6
        color: "#FFFFFF"
        border.color: textInput.activeFocus ? "#2563EB" : "#CBD5E1"
        border.width: textInput.activeFocus ? 2 : 1

        TextField {
            id: textInput
            anchors.fill: parent
            anchors.leftMargin: 10
            anchors.rightMargin: 10
            placeholderText: root.placeholderText
            color: "#0F172A"
            font.pixelSize: 13
            font.family: "Menlo"
            background: null
            selectByMouse: true

            Keys.onReturnPressed: function(event) {
                event.accepted = true
                Qt.callLater(function() {
                    root.returnPressed()
                })
            }
            Keys.onEnterPressed: function(event) {
                event.accepted = true
                Qt.callLater(function() {
                    root.returnPressed()
                })
            }
            Keys.onLeftPressed: function(event) {
                if (textInput.cursorPosition === 0 || textInput.selectedText.length > 0) {
                    event.accepted = true
                    Qt.callLater(function() {
                        root.leftPressed()
                    })
                } else {
                    event.accepted = false
                }
            }
            Keys.onRightPressed: function(event) {
                if (textInput.cursorPosition === textInput.text.length || textInput.selectedText.length > 0) {
                    event.accepted = true
                    Qt.callLater(function() {
                        root.rightPressed()
                    })
                } else {
                    event.accepted = false
                }
            }
            Keys.onUpPressed: function(event) {
                event.accepted = true
                Qt.callLater(function() {
                    root.upPressed()
                })
            }
            Keys.onDownPressed: function(event) {
                event.accepted = true
                Qt.callLater(function() {
                    root.downPressed()
                })
            }
            Keys.onEscapePressed: function(event) {
                event.accepted = false
            }
        }
    }
}
