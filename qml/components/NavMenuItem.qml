import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: root

    property int index: 0
    property int selectedIndex: -1
    property bool isSelected: selectedIndex === index

    property string text: ""
    property int fontPixelSize: 15
    property bool fontBold: true

    property color activeColor: "#2563EB"
    property color activeBorderColor: "#1D4ED8"
    property color activeTextColor: "#FFFFFF"

    property color normalColor: "#F8FAFC"
    property color normalBorderColor: "#CBD5E1"
    property color normalTextColor: "#000000"

    property real itemHeight: 48
    property real itemRadius: 8
    default property alias contentData: container.data

    signal itemHovered()
    signal itemClicked()

    Layout.fillWidth: true
    implicitHeight: itemHeight
    Layout.preferredHeight: itemHeight
    height: itemHeight
    radius: itemRadius

    color: isSelected ? activeColor : normalColor
    border.color: isSelected ? activeBorderColor : normalBorderColor
    border.width: isSelected ? 2 : 1

    // Default container for custom children (if provided)
    Item {
        id: container
        anchors.fill: parent
    }

    // Default text item (if property text is specified)
    Text {
        anchors.fill: parent
        anchors.leftMargin: 16
        anchors.rightMargin: 16
        verticalAlignment: Text.AlignVCenter
        visible: root.text !== ""
        text: root.text
        color: root.isSelected ? root.activeTextColor : root.normalTextColor
        font.pixelSize: root.fontPixelSize
        font.bold: root.fontBold
    }

    MouseArea {
        id: mouseArea
        anchors.fill: parent
        hoverEnabled: true
        property point lastPos: Qt.point(-1, -1)

        onPositionChanged: function(mouse) {
            if (lastPos.x >= 0 && lastPos.y >= 0) {
                var dx = Math.abs(mouse.x - lastPos.x)
                var dy = Math.abs(mouse.y - lastPos.y)
                if (dx > 3 || dy > 3) {
                    root.itemHovered()
                }
            }
            lastPos = Qt.point(mouse.x, mouse.y)
        }

        onClicked: {
            root.itemClicked()
        }
    }

    function resetMouseTracking() {
        mouseArea.lastPos = Qt.point(-1, -1)
    }
}
