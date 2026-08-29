import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ColumnLayout {
    id: root
    implicitWidth: 200
    property string label: ""
    property alias model: combo.model
    property alias currentIndex: combo.currentIndex
    property alias editText: combo.editText
    property alias editable: combo.editable
    property alias focusInput: comboField.focus
    readonly property string currentText: combo.editText

    signal returnPressed()
    signal leftPressed()
    signal rightPressed()
    signal upPressed()
    signal downPressed()

    property bool userNavigatedPopup: false

    function focusAndOpen() {
        comboField.forceActiveFocus()
        comboField.selectAll()
    }

    function emitReturn() {
        root.returnPressed()
    }

    function emitLeft() {
        root.leftPressed()
    }

    function emitRight() {
        root.rightPressed()
    }

    function emitUp() {
        root.upPressed()
    }

    function emitDown() {
        root.downPressed()
    }

    function commitSelection() {
        if (combo.popup.visible && root.userNavigatedPopup) {
            var hIdx = combo.highlightedIndex
            if (hIdx >= 0 && hIdx < combo.count) {
                combo.currentIndex = hIdx
                var selText = combo.textAt(hIdx)
                if (selText !== undefined && selText !== null) {
                    combo.editText = selText
                    comboField.text = selText
                }
            } else if (comboField.text !== "") {
                combo.editText = comboField.text
            }
        } else {
            var currentVal = (comboField.text || "").trim().toLowerCase()
            var matchIdx = -1
            if (currentVal.length > 0) {
                for (var i = 0; i < combo.count; i++) {
                    var itemStr = (combo.textAt(i) || "").trim().toLowerCase()
                    if (itemStr === currentVal) {
                        matchIdx = i
                        break
                    }
                }
                if (matchIdx < 0) {
                    for (var j = 0; j < combo.count; j++) {
                        var itemStr2 = (combo.textAt(j) || "").trim().toLowerCase()
                        if (itemStr2.startsWith(currentVal)) {
                            matchIdx = j
                            break
                        }
                    }
                }
            }

            if (matchIdx >= 0) {
                combo.currentIndex = matchIdx
                var matchedText = combo.textAt(matchIdx)
                combo.editText = matchedText
                comboField.text = matchedText
            } else if (comboField.text !== "") {
                combo.editText = comboField.text
            }
        }
    }

    spacing: 4

    Text {
        text: root.label
        color: "#334155"
        font.pixelSize: 12
        font.bold: true
        visible: root.label !== ""
    }

    ComboBox {
        id: combo
        implicitWidth: 200
        Layout.fillWidth: true
        implicitHeight: 34
        editable: true
        currentIndex: -1
        editText: ""
        font.pixelSize: 12
        font.bold: true

        onActiveFocusChanged: {
            if (activeFocus && !comboField.activeFocus) {
                comboField.forceActiveFocus()
            }
        }

        Keys.onReturnPressed: function(event) {
            event.accepted = true
            root.commitSelection()
            combo.popup.close()
            Qt.callLater(root.emitReturn)
        }
        Keys.onEnterPressed: function(event) {
            event.accepted = true
            root.commitSelection()
            combo.popup.close()
            Qt.callLater(root.emitReturn)
        }
        Keys.onLeftPressed: function(event) {
            if (!combo.popup.visible) {
                event.accepted = true
                Qt.callLater(root.emitLeft)
            }
        }
        Keys.onRightPressed: function(event) {
            if (!combo.popup.visible) {
                event.accepted = true
                Qt.callLater(root.emitRight)
            }
        }

        background: Rectangle {
            color: "#FFFFFF"
            border.color: (combo.activeFocus || comboField.activeFocus) ? "#2563EB" : "#CBD5E1"
            border.width: (combo.activeFocus || comboField.activeFocus) ? 2 : 1
            radius: 5
        }

        contentItem: TextField {
            id: comboField
            leftPadding: 8
            rightPadding: 20
            text: combo.editText
            font.pixelSize: 12
            font.bold: true
            color: "#0F172A"
            background: null
            selectByMouse: true

            onActiveFocusChanged: {
                if (activeFocus) {
                    comboField.selectAll()
                }
            }

            onTextEdited: {
                root.userNavigatedPopup = false
                combo.editText = text
                var typed = text.trim().toLowerCase()

                if (typed.length > 0) {
                    if (!combo.popup.visible) combo.popup.open()

                    var matchIdx = -1
                    for (var i = 0; i < combo.count; i++) {
                        var itemStr = (combo.textAt(i) || "").trim().toLowerCase()
                        if (itemStr === typed || itemStr.startsWith(typed)) {
                            matchIdx = i
                            break
                        }
                    }
                    if (matchIdx >= 0) {
                        if (combo.popup && combo.popup.contentItem && typeof combo.popup.contentItem.currentIndex !== "undefined") {
                            combo.popup.contentItem.currentIndex = matchIdx
                        }
                    }
                } else {
                    combo.currentIndex = -1
                    if (combo.popup.visible) combo.popup.close()
                }
            }

            Keys.onReturnPressed: function(event) {
                event.accepted = true
                root.commitSelection()
                combo.popup.close()
                Qt.callLater(root.emitReturn)
            }
            Keys.onEnterPressed: function(event) {
                event.accepted = true
                root.commitSelection()
                combo.popup.close()
                Qt.callLater(root.emitReturn)
            }
            Keys.onUpPressed: function(event) {
                if (combo.popup.visible) {
                    root.userNavigatedPopup = true
                }
                if (!combo.popup.visible) {
                    event.accepted = true
                    Qt.callLater(root.emitUp)
                    if (combo.currentIndex > 0) {
                        combo.currentIndex--
                        var selText = combo.textAt(combo.currentIndex)
                        if (selText !== undefined && selText !== null) {
                            combo.editText = selText
                            comboField.text = selText
                        }
                    }
                } else {
                    event.accepted = false
                }
            }
            Keys.onDownPressed: function(event) {
                if (combo.popup.visible) {
                    root.userNavigatedPopup = true
                }
                if (!combo.popup.visible) {
                    event.accepted = true
                    Qt.callLater(root.emitDown)
                    if (combo.currentIndex < combo.count - 1) {
                        combo.currentIndex++
                        var selText = combo.textAt(combo.currentIndex)
                        if (selText !== undefined && selText !== null) {
                            combo.editText = selText
                            comboField.text = selText
                        }
                    }
                } else {
                    event.accepted = false
                }
            }
            Keys.onLeftPressed: function(event) {
                if (!combo.popup.visible && (comboField.cursorPosition === 0 || comboField.selectedText.length > 0 || comboField.text.length === 0)) {
                    event.accepted = true
                    Qt.callLater(root.emitLeft)
                } else {
                    event.accepted = false
                }
            }
            Keys.onRightPressed: function(event) {
                if (!combo.popup.visible && (comboField.cursorPosition === comboField.text.length || comboField.selectedText.length > 0 || comboField.text.length === 0)) {
                    event.accepted = true
                    Qt.callLater(root.emitRight)
                } else {
                    event.accepted = false
                }
            }
        }

        delegate: ItemDelegate {
            width: combo.width - 12
            height: modelData !== "" ? 30 : 0
            visible: modelData !== "" && modelData !== undefined
            highlighted: combo.highlightedIndex === index
            contentItem: Text {
                text: modelData ? modelData : ""
                color: combo.highlightedIndex === index ? "#2563EB" : "#0F172A"
                font.pixelSize: 12
                font.bold: combo.highlightedIndex === index
                verticalAlignment: Text.AlignVCenter
                elide: Text.ElideRight
            }
            background: Rectangle {
                color: combo.highlightedIndex === index ? "#EFF6FF" : "#FFFFFF"
                radius: 4
            }
            onClicked: {
                combo.currentIndex = index
                combo.editText = modelData
                comboField.text = modelData
                combo.popup.close()
                Qt.callLater(root.emitReturn)
            }
        }

        popup: Popup {
            y: combo.height + 2
            width: combo.width
            implicitHeight: Math.min(180, popupListView.contentHeight + 8)
            padding: 4
            clip: true
            closePolicy: Popup.CloseOnPressOutside | Popup.CloseOnEscape

            background: Rectangle {
                color: "#FFFFFF"
                border.color: "#2563EB"
                border.width: 1
                radius: 6
            }

            contentItem: ListView {
                id: popupListView
                clip: true
                implicitHeight: contentHeight
                model: combo.popup.visible ? combo.delegateModel : null
                currentIndex: combo.highlightedIndex
                ScrollIndicator.vertical: ScrollIndicator { }
            }
        }
    }
}
