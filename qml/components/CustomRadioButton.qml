import QtQuick
import QtQuick.Templates as T

T.RadioButton {
    id: control
    property color activeColor: "#2563EB"
    property color textColor: "#1E293B"

    font.pixelSize: 11
    font.bold: true

    implicitWidth: Math.max(18, (contentItem ? contentItem.implicitWidth : 0) + leftPadding + rightPadding)
    implicitHeight: Math.max(22, indicator ? indicator.implicitHeight : 18, (contentItem ? contentItem.implicitHeight : 0)) + topPadding + bottomPadding

    indicator: Rectangle {
        implicitWidth: 18
        implicitHeight: 18
        x: control.leftPadding
        y: parent.height / 2 - height / 2
        radius: 9
        color: "#FFFFFF"
        border.color: control.checked ? control.activeColor : (control.hovered ? "#94A3B8" : "#CBD5E1")
        border.width: control.checked ? 2 : 1.5

        Rectangle {
            anchors.centerIn: parent
            width: 8
            height: 8
            radius: 4
            color: control.activeColor
            visible: control.checked
        }
    }

    contentItem: Text {
        text: control.text
        font: control.font
        color: control.checked ? control.activeColor : control.textColor
        leftPadding: control.indicator.width + 6
        verticalAlignment: Text.AlignVCenter
    }
}
