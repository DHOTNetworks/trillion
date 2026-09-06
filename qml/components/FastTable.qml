import QtQuick
import QtQuick.Templates as T
import QtQuick.Layouts
import MahadevERP

Rectangle {
    id: root
    property alias model: listView.model
    property list<string> headers: []
    property list<string> roleKeys: []
    property list<real> columnWidths: []
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

                    T.TextField {
                        id: searchInput
                        placeholderText: "Search records..."
                        color: "#0F172A"
                        font.pixelSize: 12
                        verticalAlignment: TextInput.AlignVCenter
                        topPadding: 0
                        bottomPadding: 0
                        background: null
                        Layout.fillWidth: true
                        onTextChanged: root.searchFilter = text.toLowerCase()
                        Keys.onDownPressed: function(event) {
                            event.accepted = true
                            listView.forceActiveFocus()
                            if (listView.currentIndex < 0 && listView.count > 0) listView.currentIndex = 0
                        }
                        Keys.onReturnPressed: function(event) {
                            event.accepted = true
                            listView.forceActiveFocus()
                            if (listView.currentIndex < 0 && listView.count > 0) listView.currentIndex = 0
                        }
                        Keys.onEnterPressed: function(event) {
                            event.accepted = true
                            listView.forceActiveFocus()
                            if (listView.currentIndex < 0 && listView.count > 0) listView.currentIndex = 0
                        }
                    }
                }
            }

            // Action Button
            T.Button {
                id: actionBtn
                implicitWidth: 120
                implicitHeight: 32
                background: Rectangle {
                    color: actionBtn.hovered ? "#15803D" : "#16A34A"
                    radius: 6
                }
                contentItem: RowLayout {
                    spacing: 6
                    anchors.centerIn: parent
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

        // Table Content Area
        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true

            // Empty State Placeholder
            Item {
                anchors.fill: parent
                visible: !listView.count || listView.count === 0

                ColumnLayout {
                    anchors.centerIn: parent
                    spacing: 6

                    Text {
                        text: "📂"
                        font.pixelSize: 28
                        Layout.alignment: Qt.AlignHCenter
                    }

                    Text {
                        text: "No records found in active books."
                        color: "#64748B"
                        font.pixelSize: 13
                        font.bold: true
                        Layout.alignment: Qt.AlignHCenter
                    }

                    Text {
                        text: "Click '+ Add Entry' or press F2 to create a new record."
                        color: "#94A3B8"
                        font.pixelSize: 11
                        Layout.alignment: Qt.AlignHCenter
                    }
                }
            }

            // ListView Body
            ListView {
                id: listView
                anchors.fill: parent
                visible: count > 0
                clip: true
                boundsBehavior: Flickable.StopAtBounds
                spacing: 2
                focus: false
                highlight: null
                highlightFollowsCurrentItem: false
                currentIndex: 0

                Keys.onReturnPressed: function(event) {
                    event.accepted = true
                    if (currentIndex >= 0 && currentIndex < count) {
                        root.rowClicked(currentIndex)
                    }
                }
                Keys.onEnterPressed: function(event) {
                    event.accepted = true
                    if (currentIndex >= 0 && currentIndex < count) {
                        root.rowClicked(currentIndex)
                    }
                }
                Keys.onUpPressed: function(event) {
                    event.accepted = true
                    if (currentIndex > 0) {
                        currentIndex--
                        positionViewAtIndex(currentIndex, ListView.Contain)
                    } else {
                        searchInput.forceActiveFocus()
                    }
                }
                Keys.onDownPressed: function(event) {
                    event.accepted = true
                    if (currentIndex < count - 1) {
                        currentIndex++
                        positionViewAtIndex(currentIndex, ListView.Contain)
                    }
                }

                delegate: Rectangle {
                    id: rowRect
                    property int rowIndexVal: index
                    property var rowData: (listView.model && listView.model.get) ? listView.model.get(index) : null
                    width: listView.width
                    height: 36
                    radius: 4
                    color: (ListView.isCurrentItem && listView.count > 0) ? "#DBEAFE" : (index % 2 === 0 ? "#FFFFFF" : "#F8FAFC")
                    border.color: (ListView.isCurrentItem && listView.count > 0) ? "#2563EB" : (mouseArea.containsMouse ? "#93C5FD" : "#F1F5F9")
                    border.width: (ListView.isCurrentItem && listView.count > 0) ? 2 : 1

                    MouseArea {
                        id: mouseArea
                        anchors.fill: parent
                        hoverEnabled: true
                        onClicked: {
                            listView.currentIndex = rowRect.rowIndexVal
                            listView.forceActiveFocus()
                            root.rowClicked(rowRect.rowIndexVal)
                        }
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
                                        if (listView.model && typeof listView.model.get_value === "function") {
                                            var val = listView.model.get_value(rowRect.rowIndexVal, roleKey)
                                            if (val !== undefined && val !== null) return String(val)
                                        }
                                        if (rowRect.rowData && rowRect.rowData[roleKey] !== undefined && rowRect.rowData[roleKey] !== null) {
                                            return String(rowRect.rowData[roleKey])
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
                                font.family: "Segoe UI, Consolas, Menlo, sans-serif"
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
}
