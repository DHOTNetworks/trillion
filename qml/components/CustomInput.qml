import QtQuick
import QtQuick.Templates as T
import QtQuick.Layouts

ColumnLayout {
    id: root
    property string label: ""
    property string placeholderText: ""
    property alias text: textInput.text
    property bool isRequired: false
    property bool focusInput: false
    onFocusInputChanged: {
        if (focusInput) {
            textInput.forceActiveFocus()
            textInput.selectAll()
            focusInput = false
        }
    }
    property alias isFocused: textInput.activeFocus
    property alias inputMethodHints: textInput.inputMethodHints
    property alias font: textInput.font
    property alias horizontalAlignment: textInput.horizontalAlignment

    signal returnPressed()
    signal leftPressed()
    signal rightPressed()
    signal upPressed()
    signal downPressed()
    signal editingFinished()

    function focusAndSelect() {
        textInput.forceActiveFocus()
        textInput.selectAll()
    }

    function forceActiveFocus() {
        textInput.forceActiveFocus()
    }

    spacing: 4

    RowLayout {
        spacing: 4
        visible: root.label !== ""
        Text {
            text: root.isRequired && root.label.endsWith("*") ? root.label.replace(/\s*\*+$/, "").trim() : root.label
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

        T.TextField {
            id: textInput
            anchors.fill: parent
            verticalAlignment: TextInput.AlignVCenter
            leftPadding: 10
            rightPadding: 10
            topPadding: 0
            bottomPadding: 0
            placeholderText: root.placeholderText
            color: "#0F172A"
            font.pixelSize: 13
            font.family: "Segoe UI, Consolas, Menlo, sans-serif"
            background: null
            selectByMouse: true
            onEditingFinished: root.editingFinished()

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
            Keys.onTabPressed: function(event) {
                event.accepted = true
                Qt.callLater(function() {
                    root.returnPressed()
                })
            }
            Keys.onBacktabPressed: function(event) {
                event.accepted = true
                Qt.callLater(function() {
                    root.leftPressed()
                })
            }
        }
    }
}
