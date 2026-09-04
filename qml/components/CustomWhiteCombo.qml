import QtQuick
import QtQuick.Templates as T
import QtQuick.Layouts

ColumnLayout {
    id: root
    implicitWidth: 200
    property alias comboHeight: combo.implicitHeight
    readonly property int count: rawItems.length
    property string label: ""
    property var model: []
    property int currentIndex: -1
    property alias text: comboField.text
    property alias editText: comboField.text
    property bool editable: true
    property alias focusInput: comboField.focus
    readonly property string currentText: comboField.text

    signal returnPressed()
    signal accepted()
    signal leftPressed()
    signal rightPressed()
    signal upPressed()
    signal downPressed()

    property var rawItems: []
    property var filteredItems: []

    function extractArray(m) {
        var res = []
        if (!m) return res
        function toStr(val) {
            if (val === undefined || val === null) return ""
            if (typeof val === "object") {
                if (val.name !== undefined) return val.name.toString()
                if (val.parent_group_name !== undefined) return val.parent_group_name.toString()
                if (val.group_name !== undefined) return val.group_name.toString()
                if (val.text !== undefined) return val.text.toString()
                var keys = Object.keys(val)
                if (keys.length > 0 && val[keys[0]] !== undefined) return val[keys[0]].toString()
                return ""
            }
            return val.toString()
        }
        if (Array.isArray(m)) {
            for (var i = 0; i < m.length; i++) {
                var s = toStr(m[i])
                if (s !== "") res.push(s)
            }
        } else if (typeof m.length === "number") {
            for (var j = 0; j < m.length; j++) {
                var s2 = toStr(m[j])
                if (s2 !== "") res.push(s2)
            }
        } else if (typeof m.count === "number") {
            for (var k = 0; k < m.count; k++) {
                var val = (typeof m.get === "function") ? m.get(k) : m[k]
                var s3 = toStr(val)
                if (s3 !== "") res.push(s3)
            }
        }
        return res
    }

    function updateRawItems() {
        rawItems = extractArray(root.model)
        filteredItems = rawItems.slice()
    }

    onModelChanged: {
        updateRawItems()
    }

    onCurrentIndexChanged: {
        if (currentIndex >= 0 && currentIndex < rawItems.length) {
            var txt = rawItems[currentIndex]
            if (txt !== undefined && txt !== null && comboField.text !== txt) {
                comboField.text = txt
            }
        }
    }

    function textAt(idx) {
        if (idx >= 0 && idx < rawItems.length) {
            return rawItems[idx].toString()
        }
        return ""
    }

    function find(val) {
        if (val === undefined || val === null) return -1
        var target = val.toString().trim().toLowerCase()
        for (var i = 0; i < rawItems.length; i++) {
            var itemStr = (rawItems[i] || "").toString().trim().toLowerCase()
            if (itemStr === target) return i
        }
        return -1
    }

    function focusAndOpen() {
        combo.forceActiveFocus()
        comboField.forceActiveFocus()
        comboField.selectAll()
    }

    function emitReturn() {
        root.returnPressed()
        root.accepted()
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
        if (comboPopup.visible && popupListView && popupListView.currentIndex >= 0 && popupListView.currentIndex < filteredItems.length) {
            var sel = filteredItems[popupListView.currentIndex]
            if (sel !== undefined && sel !== null) {
                comboField.text = sel.toString()
                root.currentIndex = root.find(sel)
                comboPopup.close()
                return
            }
        }

        var currentVal = (comboField.text || "").trim().toLowerCase()
        if (currentVal.length === 0) {
            root.currentIndex = -1
            return
        }

        var exactIdx = root.find(comboField.text)
        if (exactIdx >= 0) {
            root.currentIndex = exactIdx
            comboField.text = rawItems[exactIdx]
        } else {
            root.currentIndex = -1
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

    Item {
        id: combo
        implicitWidth: 200
        Layout.fillWidth: true
        implicitHeight: 34

        Rectangle {
            anchors.fill: parent
            color: "#FFFFFF"
            border.color: (combo.activeFocus || comboField.activeFocus) ? "#2563EB" : "#CBD5E1"
            border.width: (combo.activeFocus || comboField.activeFocus) ? 2 : 1
            radius: 5
        }

        T.TextField {
            id: comboField
            anchors.fill: parent
            anchors.rightMargin: 26
            verticalAlignment: TextInput.AlignVCenter
            leftPadding: 10
            rightPadding: 4
            topPadding: 0
            bottomPadding: 0
            font.pixelSize: 13
            font.bold: false
            color: "#0F172A"
            background: null
            selectByMouse: true

            onActiveFocusChanged: {
                if (activeFocus) {
                    comboField.selectAll()
                }
            }

            onTextEdited: {
                var typed = text.trim().toLowerCase()
                if (typed.length === 0) {
                    filteredItems = rawItems.slice()
                    if (popupListView) popupListView.currentIndex = -1
                    if (!comboPopup.visible && rawItems.length > 0) comboPopup.open()
                } else {
                    var pfx = []
                    var sub = []
                    for (var i = 0; i < rawItems.length; i++) {
                        var s = (rawItems[i] || "").toString()
                        var low = s.toLowerCase()
                        if (low === typed || low.startsWith(typed)) {
                            pfx.push(s)
                        } else if (low.indexOf(typed) !== -1) {
                            sub.push(s)
                        }
                    }
                    var matches = pfx.concat(sub)
                    filteredItems = matches
                    if (matches.length > 0) {
                        if (popupListView) popupListView.currentIndex = 0
                        if (!comboPopup.visible) comboPopup.open()
                    } else {
                        if (popupListView) popupListView.currentIndex = -1
                        if (comboPopup.visible) comboPopup.close()
                    }
                }
            }

            Keys.onReturnPressed: function(event) {
                event.accepted = true
                root.commitSelection()
                if (comboPopup.visible) comboPopup.close()
                Qt.callLater(root.emitReturn)
            }
            Keys.onEnterPressed: function(event) {
                event.accepted = true
                root.commitSelection()
                if (comboPopup.visible) comboPopup.close()
                Qt.callLater(root.emitReturn)
            }
            Keys.onUpPressed: function(event) {
                if (comboPopup.visible) {
                    event.accepted = true
                    if (popupListView && popupListView.count > 0) {
                        var nextIdx = Math.max(0, popupListView.currentIndex - 1)
                        popupListView.currentIndex = nextIdx
                        popupListView.positionViewAtIndex(nextIdx, ListView.Contain)
                    }
                } else {
                    event.accepted = true
                    Qt.callLater(root.emitUp)
                }
            }
            Keys.onDownPressed: function(event) {
                if (comboPopup.visible) {
                    event.accepted = true
                    if (popupListView && popupListView.count > 0) {
                        var nextIdx = Math.min(popupListView.count - 1, popupListView.currentIndex + 1)
                        popupListView.currentIndex = nextIdx
                        popupListView.positionViewAtIndex(nextIdx, ListView.Contain)
                    }
                } else {
                    event.accepted = true
                    filteredItems = rawItems.slice()
                    var curIdx = root.find(comboField.text)
                    if (curIdx >= 0) {
                        if (popupListView) popupListView.currentIndex = curIdx
                        if (popupListView) popupListView.positionViewAtIndex(curIdx, ListView.Center)
                    } else {
                        if (popupListView) popupListView.currentIndex = 0
                    }
                    comboPopup.open()
                }
            }
            Keys.onLeftPressed: function(event) {
                if (!comboPopup.visible && (comboField.cursorPosition === 0 || comboField.selectedText.length > 0 || comboField.text.length === 0)) {
                    event.accepted = true
                    Qt.callLater(root.emitLeft)
                } else {
                    event.accepted = false
                }
            }
            Keys.onRightPressed: function(event) {
                if (!comboPopup.visible && (comboField.cursorPosition === comboField.text.length || comboField.selectedText.length > 0 || comboField.text.length === 0)) {
                    event.accepted = true
                    Qt.callLater(root.emitRight)
                } else {
                    event.accepted = false
                }
            }
        }

        Item {
            id: arrowButton
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            width: 26

            Text {
                anchors.centerIn: parent
                text: "▼"
                font.pixelSize: 8
                color: (combo.activeFocus || comboField.activeFocus) ? "#2563EB" : "#64748B"
            }

            MouseArea {
                anchors.fill: parent
                cursorShape: Qt.PointingHandCursor
                onClicked: {
                    comboField.forceActiveFocus()
                    if (comboPopup.visible) {
                        comboPopup.close()
                    } else {
                        filteredItems = rawItems.slice()
                        var curIdx = root.find(comboField.text)
                        if (curIdx >= 0) {
                            if (popupListView) popupListView.currentIndex = curIdx
                            if (popupListView) popupListView.positionViewAtIndex(curIdx, ListView.Center)
                        } else {
                            if (popupListView) popupListView.currentIndex = 0
                        }
                        comboPopup.open()
                    }
                }
            }
        }

        T.Popup {
            id: comboPopup
            y: combo.height + 2
            width: Math.max(combo.width, 240)
            height: Math.min(220, Math.max(40, ((root.filteredItems && root.filteredItems.length) ? root.filteredItems.length * 32 + 8 : 40)))
            padding: 4
            clip: true
            closePolicy: T.Popup.CloseOnPressOutside | T.Popup.CloseOnEscape

            background: Rectangle {
                color: "#FFFFFF"
                border.color: "#2563EB"
                border.width: 1
                radius: 6
            }

            ListView {
                id: popupListView
                anchors.fill: parent
                clip: true
                model: comboPopup.visible ? root.filteredItems : []
                currentIndex: -1
                boundsBehavior: Flickable.StopAtBounds
                T.ScrollBar.vertical: T.ScrollBar { policy: T.ScrollBar.AsNeeded }

                delegate: T.ItemDelegate {
                    width: popupListView.width
                    implicitHeight: 30
                    highlighted: popupListView.currentIndex === index
                    contentItem: Text {
                        text: modelData ? modelData.toString() : ""
                        color: popupListView.currentIndex === index ? "#2563EB" : "#0F172A"
                        font.pixelSize: 12
                        font.bold: popupListView.currentIndex === index
                        verticalAlignment: Text.AlignVCenter
                        elide: Text.ElideRight
                        leftPadding: 8
                    }
                    background: Rectangle {
                        color: popupListView.currentIndex === index ? "#EFF6FF" : (hovered ? "#F8FAFC" : "#FFFFFF")
                        radius: 4
                    }
                    onClicked: {
                        comboField.text = modelData ? modelData.toString() : ""
                        root.currentIndex = root.find(comboField.text)
                        comboPopup.close()
                        Qt.callLater(root.emitReturn)
                    }
                }
            }
        }
    }

    Component.onCompleted: {
        updateRawItems()
        if (comboField.text === "" && root.currentIndex >= 0 && root.currentIndex < rawItems.length) {
            comboField.text = rawItems[root.currentIndex] || ""
        }
    }
}
