import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../components"

ScrollView {
    id: root
    contentWidth: availableWidth
    clip: true

    signal cancelRequested()
    signal savedSuccess()

    ColumnLayout {
        width: root.availableWidth > 0 ? root.availableWidth : 1100
        spacing: 20

        // Page Header Bar
        RowLayout {
            Layout.fillWidth: true
            spacing: 12

            ColumnLayout {
                spacing: 2
                Text {
                    text: "📁 Create New Account Group"
                    color: "#0F172A"
                    font.pixelSize: 20
                    font.bold: true
                }
                Text {
                    text: "Define new accounting hierarchy sub-groups under Assets, Liabilities, Income, or Expenses."
                    color: "#64748B"
                    font.pixelSize: 12
                }
            }

            Item { Layout.fillWidth: true }

            Button {
                id: backBtn
                background: Rectangle { color: "#F1F5F9"; radius: 6; border.color: "#CBD5E1" }
                contentItem: RowLayout {
                    spacing: 6
                    Text { text: "← Back to Dashboard"; color: "#475569"; font.pixelSize: 13; font.bold: true }
                    KbdBadge { text: "Esc"; badgeColor: "#DC2626"; textColor: "#FFF"; borderColor: "#B91C1C" }
                }
                onClicked: root.cancelRequested()
            }
        }

        Rectangle { Layout.fillWidth: true; height: 1; color: "#E2E8F0" }

        // GROUP DETAILS FORM CARD
        Rectangle {
            Layout.fillWidth: true
            implicitHeight: formCol.implicitHeight + 32
            color: "#FFFFFF"
            border.color: "#E2E8F0"
            border.width: 1
            radius: 10

            ColumnLayout {
                id: formCol
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.top: parent.top
                anchors.margins: 16
                spacing: 16

                Text {
                    text: "ACCOUNT GROUP DEFINITION & CLASSIFICATION"
                    color: "#2563EB"
                    font.pixelSize: 11
                    font.bold: true
                    font.letterSpacing: 1.0
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 14

                    CustomInput {
                        id: groupNameInput
                        label: "Account Group Name *"
                        placeholderText: "e.g. Direct Expenses - APMC Hamali"
                        isRequired: true
                        focusInput: true
                        Layout.fillWidth: true
                        onReturnPressed: parentCombo.focusAndOpen()
                    }

                    CustomWhiteCombo {
                        id: parentCombo
                        label: "Parent Group *"
                        Layout.fillWidth: true
                        model: (groupsModel && typeof groupsModel.get_parent_groups === 'function') ? groupsModel.get_parent_groups() : (partiesModel ? partiesModel.get_account_groups() : [])
                        onReturnPressed: natureCombo.focusAndOpen()
                    }
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 14

                    CustomWhiteCombo {
                        id: natureCombo
                        label: "Primary Group Nature *"
                        Layout.preferredWidth: 240
                        model: ["Assets", "Liabilities", "Income", "Expense"]
                        onReturnPressed: descInput.focusInput = true
                    }

                    CustomInput {
                        id: descInput
                        label: "Description / Notes"
                        placeholderText: "Sub-group for tracking specific accounting transactions and reporting"
                        Layout.fillWidth: true
                        onReturnPressed: bsCheck.focus = true
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

                        Keys.onReturnPressed: function(event) {
                            event.accepted = true
                            bsCheckMouse.checked = !bsCheckMouse.checked
                            submitBtn.focus = true
                        }
                        Keys.onSpacePressed: function(event) {
                            event.accepted = true
                            bsCheckMouse.checked = !bsCheckMouse.checked
                        }
                    }

                    Text {
                        text: "Extract in Financial Statements (Balance Sheet & Profit / Loss Statement)"
                        color: "#0F172A"
                        font.pixelSize: 13
                        font.bold: true
                    }
                }
            }
        }

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
                background: Rectangle { color: submitBtn.hovered ? "#1D4ED8" : "#2563EB"; radius: 6 }
                contentItem: RowLayout {
                    spacing: 6
                    Text { text: "💾 Save Account Group"; color: "#FFFFFF"; font.bold: true; font.pixelSize: 14 }
                    KbdBadge { text: "Enter"; badgeColor: "#1E3A8A"; textColor: "#93C5FD"; borderColor: "#2563EB" }
                }
                onClicked: {
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
            }
        }
    }
}
