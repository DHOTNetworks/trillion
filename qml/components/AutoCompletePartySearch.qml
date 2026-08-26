import QtQuick
import QtQuick.Controls
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
    property alias focusInput: searchInput.focus
    signal partySelected(var party)

    ListModel {
        id: filteredModel
    }

    function updateFilteredList() {
        filteredModel.clear()
        if (!partiesModel || typeof partiesModel.get_party_list !== 'function') return
        
        var list = partiesModel.get_party_list()
        var q = searchInput.text.toLowerCase().trim()
        if (q === "" || !list) return

        for (var i = 0; i < list.length; i++) {
            var p = list[i]
            var n = p.name ? String(p.name) : ""
            var g = p.group_name ? String(p.group_name) : ""
            var ph = p.phone ? String(p.phone) : ""
            var ct = p.city ? String(p.city) : ""

            if (n.toLowerCase().indexOf(q) !== -1 || g.toLowerCase().indexOf(q) !== -1) {
                filteredModel.append({
                    pName: n,
                    pGroup: g ? g : "Sundry Debtors",
                    pPhone: ph,
                    pCity: ct ? ct : "Raichur"
                })
            }
        }
        searchList.currentIndex = 0
    }

    function selectCurrentItem() {
        if (filteredModel.count > 0 && searchList.currentIndex >= 0 && searchList.currentIndex < filteredModel.count) {
            var item = filteredModel.get(searchList.currentIndex)
            if (item) {
                var pName = item.pName
                searchInput.text = pName
                root.selectedPartyName = pName
                popup.close()
                root.partySelected({
                    name: pName,
                    group_name: item.pGroup,
                    phone: item.pPhone,
                    city: item.pCity
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

        TextField {
            id: searchInput
            placeholderText: "Type party name to search..."
            color: "#0F172A"
            font.pixelSize: 13
            font.bold: true
            background: null
            Layout.fillWidth: true
            selectByMouse: true

            onTextChanged: {
                root.updateFilteredList()
                if (filteredModel.count > 0 && text.trim().length > 0) {
                    popup.open()
                } else {
                    popup.close()
                }
            }

            // Arrow keys navigation bounded strictly to filteredModel.count
            Keys.onDownPressed: function(event) {
                if (popup.opened && filteredModel.count > 0) {
                    event.accepted = true
                    searchList.currentIndex = Math.min(filteredModel.count - 1, searchList.currentIndex + 1)
                }
            }

            Keys.onUpPressed: function(event) {
                if (popup.opened && filteredModel.count > 0) {
                    event.accepted = true
                    searchList.currentIndex = Math.max(0, searchList.currentIndex - 1)
                }
            }

            Keys.onReturnPressed: function(event) {
                if (popup.opened && filteredModel.count > 0) {
                    event.accepted = true
                    root.selectCurrentItem()
                }
            }

            Keys.onEnterPressed: function(event) {
                if (popup.opened && filteredModel.count > 0) {
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

    // FLOATING POPUP OVERLAY SIZED STRICTLY TO MATCHES COUNT
    Popup {
        id: popup
        y: root.height + 4
        width: root.width
        implicitHeight: Math.min(180, Math.max(38, filteredModel.count * 36 + 8))
        padding: 4
        closePolicy: Popup.CloseOnPressOutside | Popup.CloseOnEscape

        background: Rectangle {
            color: "#FFFFFF"
            border.color: "#2563EB"
            border.width: 1
            radius: 6
        }

        contentItem: ListView {
            id: searchList
            clip: true
            model: filteredModel
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
                        text: model.pName
                        color: index === searchList.currentIndex ? "#2563EB" : "#0F172A"
                        font.pixelSize: 12
                        font.bold: true
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
                            text: model.pGroup
                            color: index === searchList.currentIndex ? "#1D4ED8" : "#64748B"
                            font.pixelSize: 10
                        }
                    }
                }
            }
        }
    }
}
