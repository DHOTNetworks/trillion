import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: root
    property string title: "Metric"
    property string value: "0.00"
    property string subtext: ""
    property string icon: "📈"
    property color accentColor: "#2563EB"

    implicitWidth: 200
    implicitHeight: 95
    radius: 8
    color: "#FFFFFF"
    border.color: "#E2E8F0"
    border.width: 1

    Rectangle {
        width: 4
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.topMargin: 8
        anchors.bottomMargin: 8
        radius: 2
        color: root.accentColor
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 14
        anchors.leftMargin: 18
        spacing: 4

        RowLayout {
            spacing: 8
            Text {
                text: root.icon
                font.pixelSize: 16
            }
            Text {
                text: root.title
                color: "#64748B"
                font.pixelSize: 12
                font.bold: true
                Layout.fillWidth: true
            }
        }

        Text {
            text: root.value
            color: "#0F172A"
            font.pixelSize: 20
            font.bold: true
            font.family: "Menlo, SF Pro Display, Segoe UI, sans-serif"
        }

        Text {
            text: root.subtext
            color: root.accentColor
            font.pixelSize: 11
            visible: root.subtext !== ""
        }
    }
}
