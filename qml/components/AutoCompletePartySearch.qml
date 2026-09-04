import QtQuick
import QtQuick.Templates as T
import QtQuick.Layouts

Rectangle {
    id: root
    implicitWidth: 420
    implicitHeight: 36
    radius: 6
    color: "#FFFFFF"
    border.color: searchInput.activeFocus ? "#2563EB" : "#CBD5E1"
    border.width: searchInput.activeFocus ? 2 : 1

    property string selectedPartyName: ""
    property alias placeholderText: searchInput.placeholderText
    property alias text: searchInput.text
    property alias focusInput: searchInput.focus
    property var searchResults: []
    signal partySelected(var party)

    function updateFilteredList() {
        if (!partiesModel || typeof partiesModel.search_parties !== 'function') {
            searchResults = []
            return
        }
        var q = searchInput.text.trim()
        if (q === "") {
            searchResults = []
            return
        }
        searchResults = partiesModel.search_parties(q)
        searchList.currentIndex = 0
    }

    function selectCurrentItem() {
        if (searchResults && searchResults.length > 0 && searchList.currentIndex >= 0 && searchList.currentIndex < searchResults.length) {
            var item = searchResults[searchList.currentIndex]
            if (item) {
                var pName = item.name ? String(item.name) : ""
                searchInput.text = pName
                root.selectedPartyName = pName
                root.searchResults = []
                root.partySelected({
                    name: pName,
                    group_name: item.group_name ? String(item.group_name) : "Sundry Debtors",
                    phone: item.phone ? String(item.phone) : "",
                    city: item.city ? String(item.city) : "Raichur"
                })
            }
        }
    }

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: 10
        anchors.rightMargin: 10
        spacing: 8

        Text {
            text: "🔍"
            font.pixelSize: 13
        }

        T.TextField {
            id: searchInput
            placeholderText: "Type party name to search..."
            color: "#0F172A"
            font.pixelSize: 13
            font.bold: true
            font.family: "Segoe UI, -apple-system, Roboto, sans-serif"
            verticalAlignment: TextInput.AlignVCenter
            topPadding: 0
            bottomPadding: 0
            background: null
            Layout.fillWidth: true
            selectByMouse: true

            onTextChanged: root.updateFilteredList()

            // Arrow keys navigation bounded strictly to searchResults.length
            Keys.onDownPressed: function(event) {
                if (dropdownBox.visible && root.searchResults && root.searchResults.length > 0) {
                    event.accepted = true
                    searchList.currentIndex = Math.min(root.searchResults.length - 1, searchList.currentIndex + 1)
                }
            }

            Keys.onUpPressed: function(event) {
                if (dropdownBox.visible && root.searchResults && root.searchResults.length > 0) {
                    event.accepted = true
                    searchList.currentIndex = Math.max(0, searchList.currentIndex - 1)
                }
            }

            Keys.onReturnPressed: function(event) {
                if (dropdownBox.visible && root.searchResults && root.searchResults.length > 0) {
                    event.accepted = true
                    root.selectCurrentItem()
                }
            }

            Keys.onEnterPressed: function(event) {
                if (dropdownBox.visible && root.searchResults && root.searchResults.length > 0) {
                    event.accepted = true
                    root.selectCurrentItem()
                }
            }
        }

        Text {
            text: "▼"
            color: "#64748B"
            font.pixelSize: 10
        }
    }

    // FLOATING DROPDOWN OVERLAY (LIGHTWEIGHT RECTANGLE PREVENTING WIN32 POPUP GRAB HANGS)
    Rectangle {
        id: dropdownBox
        z: 9999
        y: root.height + 4
        width: root.width
        height: Math.min(200, (root.searchResults ? root.searchResults.length : 0) * 36 + 8)
        visible: searchInput.activeFocus && root.searchResults && root.searchResults.length > 0 && searchInput.text.trim().length > 0
        color: "#FFFFFF"
        border.color: "#2563EB"
        border.width: 1
        radius: 6

        ListView {
            id: searchList
            anchors.fill: parent
            anchors.margins: 4
            clip: true
            model: root.searchResults
            currentIndex: 0

            delegate: Rectangle {
                width: searchList.width
                height: 34
                radius: 4
                color: index === searchList.currentIndex ? "#EFF6FF" : (mouseArea.containsMouse ? "#F8FAFC" : "#FFFFFF")
                border.color: index === searchList.currentIndex ? "#BFDBFE" : "transparent"

                MouseArea {
                    id: mouseArea
                    anchors.fill: parent
                    hoverEnabled: true
                    onClicked: {
                        searchList.currentIndex = index
                        root.selectCurrentItem()
                    }
                }

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 8
                    anchors.rightMargin: 8
                    spacing: 8

                    Text {
                        text: (modelData && modelData.name) ? String(modelData.name) : ""
                        color: index === searchList.currentIndex ? "#2563EB" : "#0F172A"
                        font.pixelSize: 12
                        font.bold: true
                        font.family: "Segoe UI, -apple-system, Roboto, sans-serif"
                        Layout.fillWidth: true
                        elide: Text.ElideRight
                    }

                    Rectangle {
                        height: 18
                        implicitWidth: groupTxt.implicitWidth + 8
                        radius: 3
                        color: index === searchList.currentIndex ? "#DBEAFE" : "#F1F5F9"
                        Text {
                            id: groupTxt
                            anchors.centerIn: parent
                            text: (modelData && modelData.group_name) ? String(modelData.group_name) : "Sundry Debtors"
                            color: index === searchList.currentIndex ? "#1D4ED8" : "#64748B"
                            font.pixelSize: 10
                            font.family: "Segoe UI, -apple-system, Roboto, sans-serif"
                        }
                    }
                }
            }
        }
    }
}
