import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ColumnLayout {
    id: root
    property string label: ""
    property alias model: combo.model
    property alias currentIndex: combo.currentIndex
    property alias editText: combo.editText
    property alias editable: combo.editable
    property alias focusInput: combo.focus
    readonly property string currentText: combo.editText !== "" ? combo.editText : (combo.currentText !== "" ? combo.currentText : "")

    signal returnPressed()

    function focusAndOpen() {
        combo.focus = true
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
        Layout.fillWidth: true
        implicitHeight: 34
        editable: true
        currentIndex: -1
        editText: ""
        font.pixelSize: 12
        font.bold: true

        background: Rectangle {
            color: "#FFFFFF"
            border.color: combo.activeFocus ? "#2563EB" : "#CBD5E1"
            border.width: combo.activeFocus ? 2 : 1
            radius: 5
        }

        contentItem: TextField {
            leftPadding: 8
            rightPadding: 20
            text: combo.editText
            font.pixelSize: 12
            font.bold: true
            color: "#0F172A"
            background: null
            selectByMouse: true

            onTextEdited: {
                combo.editText = text
                if (text.trim().length > 0) {
                    if (!combo.popup.visible) combo.popup.open()
                } else {
                    if (combo.popup.visible) combo.popup.close()
                }
            }

            Keys.onReturnPressed: function(event) {
                event.accepted = true
                combo.popup.close()
                Qt.callLater(function() {
                    root.returnPressed()
                })
            }
            Keys.onEnterPressed: function(event) {
                event.accepted = true
                combo.popup.close()
                Qt.callLater(function() {
                    root.returnPressed()
                })
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
                combo.popup.close()
                Qt.callLater(function() {
                    root.returnPressed()
                })
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
