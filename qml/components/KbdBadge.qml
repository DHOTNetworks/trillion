import QtQuick
import QtQuick.Controls

Rectangle {
    id: root
    property alias text: label.text
    property color badgeColor: "#E2E8F0"
    property color textColor: "#1E293B"
    property color borderColor: "#CBD5E1"

    implicitWidth: label.implicitWidth + 12
    implicitHeight: 22
    radius: 4
    color: badgeColor
    border.color: borderColor
    border.width: 1

    Text {
        id: label
        anchors.centerIn: parent
        font.pixelSize: 10
        font.bold: true
        font.family: "Menlo"
        color: root.textColor
    }
}
