import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../components"
import "../dialogs"

Item {
    id: root
    anchors.fill: parent

    signal cancelRequested()
    signal savedSuccess()

    property int currentGroupId: -1

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 14
        spacing: 12

        // Page Header Bar
        RowLayout {
            Layout.fillWidth: true
            spacing: 12

            ColumnLayout {
                spacing: 1
                Text {
                    text: "✏️ Modify Existing Account Group"
                    color: "#0F172A"
                    font.pixelSize: 18
                    font.bold: true
                }
                Text {
                    text: "Select any existing accounting group to modify name, parent group, nature, or reporting configuration."
                    color: "#64748B"
                    font.pixelSize: 11
                }
            }

            Item { Layout.fillWidth: true }

            Button {
                id: backBtn
                background: Rectangle { color: "#F1F5F9"; radius: 6; border.color: "#CBD5E1" }
                contentItem: RowLayout {
                    spacing: 6
                    Text { text: "← Back to Dashboard"; color: "#475569"; font.pixelSize: 12; font.bold: true }
                    KbdBadge { text: "Esc"; badgeColor: "#DC2626"; textColor: "#FFF"; borderColor: "#B91C1C" }
                }
                onClicked: root.cancelRequested()
            }
        }

        Rectangle { Layout.fillWidth: true; height: 1; color: "#E2E8F0" }

        // GROUP SELECTOR CARD
        Rectangle {
            Layout.fillWidth: true
            implicitHeight: selectCol.implicitHeight + 16
            color: "#EFF6FF"
            border.color: "#BFDBFE"
            border.width: 1
            radius: 8
            z: 100

            ColumnLayout {
                id: selectCol
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.top: parent.top
                anchors.margins: 10
                spacing: 6

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 12

                    CustomWhiteCombo {
                        id: selectGroupCombo
                        label: "🔍 CHOOSE ACCOUNT GROUP TO MODIFY (Type Name, ↑/↓ Arrows & Enter) *"
                        Layout.fillWidth: true
                        focusInput: true
                        model: (groupsModel && typeof groupsModel.get_parent_groups === 'function') ? groupsModel.get_parent_groups() : (partiesModel ? partiesModel.get_account_groups() : [])
                        onReturnPressed: root.loadSelectedGroup(selectGroupCombo.currentText)
                        onDownPressed: groupNameInput.focusInput = true
                    }
                }
            }
        }

        // GROUP DETAILS FORM CARD
        Rectangle {
            Layout.fillWidth: true
            implicitHeight: formCol.implicitHeight + 24
            color: "#FFFFFF"
            border.color: "#E2E8F0"
            border.width: 1
            radius: 8

            ColumnLayout {
                id: formCol
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.top: parent.top
                anchors.margins: 12
                spacing: 12

                Text {
                    text: "ACCOUNT GROUP EDIT DETAILS"
                    color: "#2563EB"
                    font.pixelSize: 11
                    font.bold: true
                    font.letterSpacing: 0.8
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 12

                    CustomInput {
                        id: groupNameInput
                        label: "Account Group Name *"
                        text: ""
                        isRequired: true
                        Layout.fillWidth: true
                        onReturnPressed: parentCombo.focusAndOpen()
                        onRightPressed: parentCombo.focusAndOpen()
                        onUpPressed: selectGroupCombo.focusAndOpen()
                        onDownPressed: natureCombo.focusAndOpen()
                    }

                    CustomWhiteCombo {
                        id: parentCombo
                        label: "Parent Group *"
                        Layout.fillWidth: true
                        model: (groupsModel && typeof groupsModel.get_parent_groups === 'function') ? groupsModel.get_parent_groups() : (partiesModel ? partiesModel.get_account_groups() : [])
                        onReturnPressed: natureCombo.focusAndOpen()
                        onLeftPressed: groupNameInput.focusInput = true
                        onUpPressed: selectGroupCombo.focusAndOpen()
                        onDownPressed: descInput.focusInput = true
                    }
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 12

                    CustomWhiteCombo {
                        id: natureCombo
                        label: "Primary Group Nature *"
                        Layout.preferredWidth: 240
                        model: ["Assets", "Liabilities", "Income", "Expense"]
                        onReturnPressed: descInput.focusInput = true
                        onRightPressed: descInput.focusInput = true
                        onUpPressed: groupNameInput.focusInput = true
                        onDownPressed: submitBtn.focus = true
                    }

                    CustomInput {
                        id: descInput
                        label: "Description / Notes"
                        text: ""
                        Layout.fillWidth: true
                        onReturnPressed: root.saveUpdateGroup()
                        onLeftPressed: natureCombo.focusAndOpen()
                        onUpPressed: parentCombo.focusAndOpen()
                        onDownPressed: submitBtn.focus = true
                    }
                }

                // Balance Sheet Extraction Checkbox
                RowLayout {
                    spacing: 10
                    Layout.topMargin: 4

                    Rectangle {
                        id: bsCheck
                        width: 22
                        height: 22
                        radius: 5
                        color: bsCheckMouse.checked ? "#2563EB" : "#FFFFFF"
                        border.color: bsCheckMouse.checked ? "#1D4ED8" : "#CBD5E1"
                        border.width: 1

                        property alias checked: bsCheckMouse.checked

                        Text {
                            anchors.centerIn: parent
                            text: "✓"
                            color: "#FFFFFF"
                            font.pixelSize: 13
                            font.bold: true
                            visible: bsCheckMouse.checked
                        }

                        MouseArea {
                            id: bsCheckMouse
                            anchors.fill: parent
                            property bool checked: true
                            onClicked: checked = !checked
                        }
                    }

                    Text {
                        text: "Extract in Financial Statements (Balance Sheet & Profit / Loss Statement)"
                        color: "#0F172A"
                        font.pixelSize: 12
                        font.bold: true
                    }
                }
            }
        }

        Item { Layout.fillHeight: true }

        // SAVE & CANCEL ACTION BAR
        RowLayout {
            Layout.fillWidth: true
            spacing: 14

            Button {
                id: cancelBottomBtn
                background: Rectangle { color: "#F1F5F9"; radius: 6; border.color: "#CBD5E1" }
                contentItem: RowLayout {
                    spacing: 6
                    Text { text: "Cancel"; color: "#475569"; font.pixelSize: 13 }
                    KbdBadge { text: "Esc"; badgeColor: "#DC2626"; textColor: "#FFF"; borderColor: "#B91C1C" }
                }
                onClicked: root.cancelRequested()
            }

            Item { Layout.fillWidth: true }

            Button {
                id: submitBtn
                background: Rectangle { color: (submitBtn.hovered || submitBtn.activeFocus) ? "#15803D" : "#16A34A"; radius: 6; border.color: submitBtn.activeFocus ? "#86EFAC" : "transparent"; border.width: 2 }
                contentItem: RowLayout {
                    spacing: 6
                    Text { text: "💾 Update Account Group"; color: "#FFFFFF"; font.bold: true; font.pixelSize: 14 }
                    KbdBadge { text: "Enter"; badgeColor: "#14532D"; textColor: "#86EFAC"; borderColor: "#16A34A" }
                }
                Keys.onReturnPressed: root.saveUpdateGroup()
                Keys.onEnterPressed: root.saveUpdateGroup()
                onClicked: root.saveUpdateGroup()
            }
        }
    }

    function loadSelectedGroup(groupName) {
        if (!groupName || !groupName.trim()) return
        var g = (typeof groupsModel !== "undefined" && groupsModel) ? groupsModel.get_group_by_name(groupName.trim()) : {}
        if (!g || !g.name) return

        root.currentGroupId = g.id ? g.id : -1
        groupNameInput.text = g.name ? g.name : ""
        parentCombo.editText = g.parent_group_name ? g.parent_group_name : ""
        natureCombo.editText = g.nature ? g.nature : ""
        descInput.text = g.description ? g.description : ""
        bsCheck.checked = (g.extract_in_balance_sheet !== 0)

        groupNameInput.focusInput = true
    }

    function saveUpdateGroup() {
        if (!groupNameInput.text.trim()) return
        var pName = parentCombo.currentText !== "" ? parentCombo.currentText : parentCombo.editText
        if (root.currentGroupId > 0) {
            groupsModel.update_group(
                root.currentGroupId,
                groupNameInput.text,
                pName !== "" ? pName : "Primary / Root Group",
                natureCombo.currentText,
                descInput.text,
                bsCheck.checked
            )
        }
        partiesModel.reload_data()
        root.savedSuccess()
    }

    Component.onCompleted: {
        Qt.callLater(function() {
            selectGroupCombo.focusAndOpen()
        })
    }
}
