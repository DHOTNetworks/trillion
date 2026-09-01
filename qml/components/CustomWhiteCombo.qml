import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ColumnLayout {
    id: root
    implicitWidth: 200
    property string label: ""
    property alias model: combo.model
    property alias currentIndex: combo.currentIndex
    property alias text: comboField.text
    property alias editText: comboField.text
    property alias editable: combo.editable
    property alias focusInput: comboField.focus
    readonly property string currentText: comboField.text

    signal returnPressed()
    signal leftPressed()
    signal rightPressed()
    signal upPressed()
    signal downPressed()

    property bool userNavigatedPopup: false

    function focusAndOpen() {
        combo.forceActiveFocus()
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
            var hIdx = (popupListView && typeof popupListView.currentIndex !== "undefined") ? popupListView.currentIndex : -1
            if (hIdx >= 0 && hIdx < combo.count) {
                combo.currentIndex = hIdx
                var selText = combo.textAt(hIdx)
                if (selText !== undefined && selText !== null) {
                    comboField.text = selText
                }
                return
            }
        }

        var currentVal = (comboField.text || "").trim().toLowerCase()
        if (currentVal.length === 0) {
            combo.currentIndex = -1
            return
        }

        // Check if typed text exact-matches any item in the model
        var exactIdx = -1
        for (var i = 0; i < combo.count; i++) {
            var itemStr = (combo.textAt(i) || "").trim().toLowerCase()
            if (itemStr === currentVal) {
                exactIdx = i
                break
            }
        }

        if (exactIdx >= 0) {
            combo.currentIndex = exactIdx
            comboField.text = combo.textAt(exactIdx)
        } else {
            // Unmatched custom text (e.g. "Sirsa", custom station, or custom name):
            // KEEP typed custom text untouched and reset combo index!
            combo.currentIndex = -1
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
        Keys.onUpPressed: function(event) {
            if (combo.popup.visible) {
                event.accepted = true
                var list = popupListView
                if (list && list.count > 0) {
                    var nextIdx = Math.max(0, list.currentIndex - 1)
                    list.currentIndex = nextIdx
                    list.positionViewAtIndex(nextIdx, ListView.Contain)
                    var selText = combo.textAt(nextIdx)
                    if (selText !== undefined && selText !== null) {
                        comboField.text = selText
                    }
                }
            } else {
                event.accepted = true
                Qt.callLater(root.emitUp)
            }
        }
        Keys.onDownPressed: function(event) {
            if (combo.popup.visible) {
                event.accepted = true
                var list = popupListView
                if (list && list.count > 0) {
                    var nextIdx = Math.min(list.count - 1, list.currentIndex + 1)
                    list.currentIndex = nextIdx
                    list.positionViewAtIndex(nextIdx, ListView.Contain)
                    var selText = combo.textAt(nextIdx)
                    if (selText !== undefined && selText !== null) {
                        comboField.text = selText
                    }
                }
            } else {
                event.accepted = true
                Qt.callLater(root.emitDown)
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
                var typed = text.trim().toLowerCase()

                if (typed.length > 0) {
                    var matchIdx = -1
                    // 1. Exact or Prefix matching
                    for (var i = 0; i < combo.count; i++) {
                        var itemStr = (combo.textAt(i) || "").trim().toLowerCase()
                        if (itemStr === typed || itemStr.startsWith(typed)) {
                            matchIdx = i
                            break
                        }
                    }
                    // 2. Substring matching if >= 2 characters
                    if (matchIdx < 0 && typed.length >= 2) {
                        for (var j = 0; j < combo.count; j++) {
                            var itemStr2 = (combo.textAt(j) || "").trim().toLowerCase()
                            if (itemStr2.indexOf(typed) !== -1) {
                                matchIdx = j
                                break
                            }
                        }
                    }

                    if (matchIdx >= 0) {
                        if (!combo.popup.visible) combo.popup.open()
                        if (combo.popup && popupListView) {
                            popupListView.currentIndex = matchIdx
                            popupListView.positionViewAtIndex(matchIdx, ListView.Contain)
                        }
                    } else {
                        // NO matches found for custom text (e.g. "Sirsa"): Close popup immediately
                        if (combo.popup.visible) combo.popup.close()
                        if (popupListView) popupListView.currentIndex = -1
                    }
                } else {
                    combo.currentIndex = -1
                    if (combo.popup.visible) combo.popup.close()
                    if (popupListView) popupListView.currentIndex = -1
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
                    event.accepted = true
                    root.userNavigatedPopup = true
                    var list = popupListView
                    if (list && list.count > 0) {
                        var nextIdx = Math.max(0, list.currentIndex - 1)
                        list.currentIndex = nextIdx
                        list.positionViewAtIndex(nextIdx, ListView.Contain)
                        var selText = combo.textAt(nextIdx)
                        if (selText !== undefined && selText !== null) {
                            comboField.text = selText
                        }
                    }
                } else {
                    event.accepted = true
                    Qt.callLater(root.emitUp)
                }
            }
            Keys.onDownPressed: function(event) {
                if (combo.popup.visible) {
                    event.accepted = true
                    root.userNavigatedPopup = true
                    var list = popupListView
                    if (list && list.count > 0) {
                        var nextIdx = Math.min(list.count - 1, list.currentIndex + 1)
                        list.currentIndex = nextIdx
                        list.positionViewAtIndex(nextIdx, ListView.Contain)
                        var selText = combo.textAt(nextIdx)
                        if (selText !== undefined && selText !== null) {
                            comboField.text = selText
                        }
                    }
                } else {
                    event.accepted = true
                    Qt.callLater(root.emitDown)
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
            highlighted: popupListView ? (popupListView.currentIndex === index) : false
            contentItem: Text {
                text: modelData ? modelData : ""
                color: (popupListView && popupListView.currentIndex === index) ? "#2563EB" : "#0F172A"
                font.pixelSize: 12
                font.bold: popupListView ? (popupListView.currentIndex === index) : false
                verticalAlignment: Text.AlignVCenter
                elide: Text.ElideRight
            }
            background: Rectangle {
                color: (popupListView && popupListView.currentIndex === index) ? "#EFF6FF" : "#FFFFFF"
                radius: 4
            }
            onClicked: {
                combo.currentIndex = index
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
