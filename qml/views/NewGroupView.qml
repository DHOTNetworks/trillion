import QtQuick
import QtQuick.Templates as T
import QtQuick.Layouts
import MahadevERP

Item {
    id: root
    anchors.fill: parent

    signal cancelRequested()
    signal savedSuccess()

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
                    text: "📁 Create New Account Group"
                    color: "#0F172A"
                    font.pixelSize: 18
                    font.bold: true
                }
                Text {
                    text: "Define new accounting hierarchy sub-groups under Assets, Liabilities, Income, or Expenses."
                    color: "#64748B"
                    font.pixelSize: 11
                }
            }

            Item { Layout.fillWidth: true }

            T.Button {
                id: backBtn
                implicitWidth: contentItem.implicitWidth + 24
                implicitHeight: 32
                background: Rectangle { color: backBtn.hovered ? "#E2E8F0" : "#F1F5F9"; radius: 6; border.color: "#CBD5E1" }
                contentItem: RowLayout {
                    spacing: 6
                    anchors.centerIn: parent
                    Text { text: "← Back to Dashboard"; color: "#475569"; font.pixelSize: 12; font.bold: true }
                    KbdBadge { text: "Esc"; badgeColor: "#DC2626"; textColor: "#FFF"; borderColor: "#B91C1C" }
                }
                onClicked: root.cancelRequested()
            }
        }

        Rectangle { Layout.fillWidth: true; height: 1; color: "#E2E8F0" }

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
                    text: "ACCOUNT GROUP DEFINITION & CLASSIFICATION"
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
                        placeholderText: "e.g. Direct Expenses - APMC Hamali"
                        isRequired: true
                        focusInput: true
                        Layout.fillWidth: true
                        onReturnPressed: parentCombo.focusAndOpen()
                        onRightPressed: parentCombo.focusAndOpen()
                        onDownPressed: natureCombo.focusAndOpen()
                    }

                    CustomWhiteCombo {
                        id: parentCombo
                        label: "Parent Group *"
                        Layout.fillWidth: true
                        model: (groupsModel && typeof groupsModel.get_parent_groups === 'function') ? groupsModel.get_parent_groups() : (partiesModel ? partiesModel.get_account_groups() : [])
                        onReturnPressed: natureCombo.focusAndOpen()
                        onLeftPressed: groupNameInput.focusInput = true
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
                        placeholderText: "Sub-group for tracking specific accounting transactions and reporting"
                        Layout.fillWidth: true
                        onReturnPressed: root.saveGroup()
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

            T.Button {
                id: cancelBottomBtn
                implicitWidth: 110
                implicitHeight: 38
                background: Rectangle { color: cancelBottomBtn.hovered ? "#E2E8F0" : "#F1F5F9"; radius: 6; border.color: "#CBD5E1" }
                contentItem: RowLayout {
                    spacing: 6
                    anchors.centerIn: parent
                    Text { text: "Cancel"; color: "#475569"; font.pixelSize: 13; font.bold: true }
                    KbdBadge { text: "Esc"; badgeColor: "#DC2626"; textColor: "#FFF"; borderColor: "#B91C1C" }
                }
                onClicked: root.cancelRequested()
            }

            Item { Layout.fillWidth: true }

            T.Button {
                id: submitBtn
                implicitWidth: 240
                implicitHeight: 38
                background: Rectangle { color: (submitBtn.hovered || submitBtn.activeFocus) ? "#1D4ED8" : "#2563EB"; radius: 6; border.color: submitBtn.activeFocus ? "#93C5FD" : "transparent"; border.width: 2 }
                contentItem: RowLayout {
                    spacing: 8
                    anchors.centerIn: parent
                    Text { text: "💾 Save Account Group"; color: "#FFFFFF"; font.bold: true; font.pixelSize: 14 }
                    KbdBadge { text: "Enter"; badgeColor: "#1E3A8A"; textColor: "#93C5FD"; borderColor: "#2563EB" }
                }
                Keys.onReturnPressed: root.saveGroup()
                Keys.onEnterPressed: root.saveGroup()
                onClicked: root.saveGroup()
            }
        }
    }

    function saveGroup() {
        if (!groupNameInput.text.trim()) return
        saveConfirmModal.open()
    }

    function executeSaveGroup() {
        var pName = parentCombo.currentText !== "" ? parentCombo.currentText : parentCombo.editText
        var success = groupsModel.add_group(
            groupNameInput.text,
            pName !== "" ? pName : "Primary",
            natureCombo.currentText,
            descInput.text,
            bsCheck.checked
        )
        if (success) {
            partiesModel.reload_data()
            root.savedSuccess()
        }
    }

    ConfirmationModal {
        id: saveConfirmModal
        anchors.centerIn: parent
        titleText: "CONFIRM ACCOUNT GROUP SAVE"
        messageText: "Are you sure you want to save & create Account Group '" + groupNameInput.text.trim() + "'?"
        onConfirmed: root.executeSaveGroup()
    }

    Component.onCompleted: {
        Qt.callLater(function() {
            groupNameInput.focusInput = true
        })
    }
}
