import QtQuick
import QtQuick.Templates as T

T.CheckBox {
    id: control
    property color checkedColor: "#2563EB"
    property color checkmarkColor: "#FFFFFF"
    property color textColor: "#1E293B"

    font.pixelSize: 12
    font.bold: true

    indicator: Rectangle {
        implicitWidth: 18
        implicitHeight: 18
        x: control.leftPadding
        y: parent.height / 2 - height / 2
        radius: 4
        color: control.checked ? control.checkedColor : "#FFFFFF"
        border.color: control.checked ? control.checkedColor : (control.hovered ? "#94A3B8" : "#CBD5E1")
        border.width: 1.5

        Text {
            anchors.centerIn: parent
            text: "✓"
            font.pixelSize: 12
            font.bold: true
            color: control.checkmarkColor
            visible: control.checked
        }
    }

    contentItem: Text {
        text: control.text
        font: control.font
        color: control.textColor
        leftPadding: control.indicator.width + 6
        verticalAlignment: Text.AlignVCenter
    }
}
