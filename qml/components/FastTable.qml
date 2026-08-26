import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: root
    property alias model: listView.model
    property var headers: []
    property var roleKeys: []
    property var columnWidths: []
    property string searchFilter: ""
    property string title: "Records"

    signal newEntryRequested()
    signal rowClicked(int rowIndex)

    color: "#FFFFFF"
    border.color: "#E2E8F0"
    border.width: 1
    radius: 8
    clip: true

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 12
        spacing: 10

        // Header & Search Control Bar
        RowLayout {
            Layout.fillWidth: true
            height: 34
            spacing: 12

            Text {
                text: root.title
                color: "#0F172A"
                font.pixelSize: 15
                font.bold: true
                Layout.fillWidth: true
                elide: Text.ElideRight
            }

            // Search Bar
            Rectangle {
                width: 200
                height: 32
                radius: 6
                color: "#F8FAFC"
                border.color: "#CBD5E1"

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 8
                    anchors.rightMargin: 8
                    spacing: 6

                    Text {
                        text: "🔍"
                        font.pixelSize: 12
                    }

                    TextField {
                        id: searchInput
                        placeholderText: "Search records..."
                        color: "#0F172A"
                        font.pixelSize: 12
                        background: null
                        Layout.fillWidth: true
                        onTextChanged: root.searchFilter = text.toLowerCase()
                    }
                }
            }

            // Action Button
            Button {
                id: actionBtn
                height: 32
                background: Rectangle {
                    color: actionBtn.hovered ? "#15803D" : "#16A34A"
                    radius: 6
                }
                contentItem: RowLayout {
                    spacing: 6
                    Text { text: "+ Add Entry"; color: "#FFFFFF"; font.pixelSize: 12; font.bold: true }
                    KbdBadge { text: "F2"; badgeColor: "#14532D"; textColor: "#86EFAC"; borderColor: "#16A34A" }
                }
                onClicked: root.newEntryRequested()
            }
        }

        // Column Headers Header Bar
        Rectangle {
            Layout.fillWidth: true
            height: 32
            color: "#F1F5F9"
            radius: 4
            clip: true

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 10
                anchors.rightMargin: 10
                spacing: 4

                Repeater {
                    model: root.headers
                    delegate: Text {
                        property int colIndex: index
                        text: modelData
                        color: "#475569"
                        font.pixelSize: 11
                        font.bold: true
                        Layout.preferredWidth: (root.columnWidths && colIndex < root.columnWidths.length) ? root.columnWidths[colIndex] : 100
                        Layout.fillWidth: (root.columnWidths && colIndex < root.columnWidths.length) ? false : true
                        horizontalAlignment: (colIndex >= 4) ? Text.AlignRight : Text.AlignLeft
                        elide: Text.ElideRight
                    }
                }
            }
        }

        // ListView Body
        ListView {
            id: listView
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            boundsBehavior: Flickable.StopAtBounds
            spacing: 2

            delegate: Rectangle {
                id: rowRect
                property int rowIndexVal: index
                property var rowData: (listView.model && listView.model.get) ? listView.model.get(index) : null
                width: listView.width
                height: 36
                radius: 4
                color: index % 2 === 0 ? "#FFFFFF" : "#F8FAFC"
                border.color: mouseArea.containsMouse ? "#2563EB" : "#F1F5F9"

                MouseArea {
                    id: mouseArea
                    anchors.fill: parent
                    hoverEnabled: true
                    onClicked: root.rowClicked(rowRect.rowIndexVal)
                }

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 10
                    anchors.rightMargin: 10
                    spacing: 4

                    Repeater {
                        model: root.roleKeys
                        delegate: Text {
                            property int colIndex: index
                            property string roleKey: modelData
                            text: {
                                if (!roleKey) return ""
                                try {
                                    if (rowRect.rowData && rowRect.rowData[roleKey] !== undefined && rowRect.rowData[roleKey] !== null) {
                                        return String(rowRect.rowData[roleKey])
                                    }
                                    if (typeof model !== "undefined" && model && model[roleKey] !== undefined && model[roleKey] !== null) {
                                        return String(model[roleKey])
                                    }
                                } catch(e) {}
                                return ""
                            }
                            color: {
                                if (roleKey === "payment_status" || roleKey === "status") {
                                    return text === "Paid" ? "#16A34A" : "#DC2626"
                                }
                                return "#1E293B"
                            }
                            font.pixelSize: 12
                            font.family: "Menlo, Consolas, sans-serif"
                            font.bold: roleKey === "net_amount" || roleKey === "total_amount" || roleKey === "slip_no" || roleKey === "invoice_no" || roleKey === "closeValVal"
                            Layout.preferredWidth: (root.columnWidths && colIndex < root.columnWidths.length) ? root.columnWidths[colIndex] : 100
                            Layout.fillWidth: (root.columnWidths && colIndex < root.columnWidths.length) ? false : true
                            horizontalAlignment: (colIndex >= 4) ? Text.AlignRight : Text.AlignLeft
                            elide: Text.ElideRight
                        }
                    }
                }
            }
        }
    }
}
