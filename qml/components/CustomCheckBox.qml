import QtQuick
import QtQuick.Templates as T

T.CheckBox {
    id: control
    property color checkedColor: "#2563EB"
    property color checkmarkColor: "#FFFFFF"
    property color textColor: "#1E293B"
    property int boxSize: 18
    property int boxRadius: 4

    font.pixelSize: 12
    font.bold: true

    implicitWidth: control.text !== "" ? (control.boxSize + (contentItem ? contentItem.implicitWidth : 0) + 8 + leftPadding + rightPadding) : (control.boxSize + leftPadding + rightPadding)
    implicitHeight: control.text !== "" ? Math.max(control.boxSize, (contentItem ? contentItem.implicitHeight : 0)) : (control.boxSize + topPadding + bottomPadding)

    indicator: Rectangle {
        implicitWidth: control.boxSize
        implicitHeight: control.boxSize
        x: control.leftPadding
        y: Math.round((control.height - height) / 2)
        radius: control.boxRadius
        color: control.checked ? control.checkedColor : "#FFFFFF"
        border.color: control.checked ? control.checkedColor : (control.hovered ? "#94A3B8" : "#CBD5E1")
        border.width: 1.5

        Canvas {
            id: checkCanvas
            anchors.fill: parent
            visible: control.checked
            onPaint: {
                var ctx = getContext("2d")
                ctx.reset()
                ctx.strokeStyle = control.checkmarkColor
                ctx.lineWidth = 2
                ctx.lineCap = "round"
                ctx.lineJoin = "round"
                ctx.beginPath()
                ctx.moveTo(width * 0.22, height * 0.52)
                ctx.lineTo(width * 0.44, height * 0.74)
                ctx.lineTo(width * 0.78, height * 0.28)
                ctx.stroke()
            }
            Connections {
                target: control
                function onCheckedChanged() {
                    checkCanvas.requestPaint()
                }
            }
        }
    }

    contentItem: Text {
        visible: control.text !== ""
        text: control.text
        font: control.font
        color: control.textColor
        leftPadding: control.indicator.width + 6
        verticalAlignment: Text.AlignVCenter
    }
}
