import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs

Rectangle {
    id: root
    width: 620
    height: 540
    radius: 16
    color: "#FFFFFF"
    border.color: "#E2E8F0"
    border.width: 1.5
    clip: true

    signal closeRequested()
    signal migrationSuccess()

    property string selectedFilePath: ""
    property var inspectionData: ({})
    property bool isInspecting: false
    property bool hasCompleted: false
    property string completionMessage: ""

    Connections {
        target: (typeof bahiKhataMigrator !== "undefined" && bahiKhataMigrator) ? bahiKhataMigrator : null
        function onMigrationFinished(success, message) {
            hasCompleted = true
            completionMessage = message
            if (success) {
                root.migrationSuccess()
                if (typeof stockItemsModel !== "undefined" && stockItemsModel) {
                    stockItemsModel.reload_data()
                }
            }
        }
    }

    FileDialog {
        id: fileDialog
        title: "Select Bahi Khata Database File (Data.004 or *.mdb)"
        nameFilters: ["Bahi Khata / Access Databases (*.004 *.mdb *.accdb)", "All Files (*)"]
        onAccepted: {
            var raw = fileDialog.selectedFile.toString()
            if (raw.indexOf("file://") === 0) {
                raw = raw.substring(7)
            }
            selectedFilePath = raw
            inspectFile(selectedFilePath)
        }
    }

    function inspectFile(path) {
        if (!path || typeof bahiKhataMigrator === "undefined" || !bahiKhataMigrator) return
        isInspecting = true
        inspectionData = bahiKhataMigrator.inspect_mdb_file(path)
        isInspecting = false
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 20
        spacing: 14

        // Header
        RowLayout {
            Layout.fillWidth: true
            spacing: 12

            Rectangle {
                width: 40; height: 40; radius: 10; color: "#EFF6FF"
                Text { anchors.centerIn: parent; text: "🔄"; font.pixelSize: 20 }
            }

            ColumnLayout {
                spacing: 2
                Text { text: "BAHI KHATA MIGRATION & SYNC"; color: "#2563EB"; font.pixelSize: 11; font.bold: true; font.letterSpacing: 1.0 }
                Text { text: "In-App Database Importer (Data.004)"; color: "#0F172A"; font.pixelSize: 17; font.bold: true }
            }

            Item { Layout.fillWidth: true }

            Rectangle {
                width: 32; height: 32; radius: 16
                color: closeMouse.containsMouse ? "#FEE2E2" : "#F1F5F9"
                Text { anchors.centerIn: parent; text: "✕"; color: closeMouse.containsMouse ? "#EF4444" : "#64748B"; font.bold: true }
                MouseArea {
                    id: closeMouse
                    anchors.fill: parent
                    hoverEnabled: true
                    onClicked: root.closeRequested()
                }
            }
        }

        Rectangle { Layout.fillWidth: true; height: 1; color: "#E2E8F0" }

        // File Selection Section
        Rectangle {
            Layout.fillWidth: true
            height: 60
            color: "#F8FAFC"
            radius: 10
            border.color: "#CBD5E1"
            border.width: 1

            RowLayout {
                anchors.fill: parent
                anchors.margins: 12
                spacing: 10

                Text {
                    text: selectedFilePath !== "" ? selectedFilePath : "No file selected (Click browse to select Data.004)"
                    color: selectedFilePath !== "" ? "#0F172A" : "#94A3B8"
                    font.pixelSize: 13
                    elide: Text.ElideMiddle
                    Layout.fillWidth: true
                }

                Button {
                    text: "📁 Browse File..."
                    onClicked: fileDialog.open()
                }
            }
        }

        // Preview & Inspection Card
        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            color: "#FFFFFF"
            radius: 10
            border.color: "#E2E8F0"
            border.width: 1
            clip: true

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 14
                spacing: 10

                Text {
                    text: "📊 Detected Database Information"
                    color: "#334155"
                    font.pixelSize: 13
                    font.bold: true
                }

                Item {
                    Layout.fillWidth: true
                    Layout.fillHeight: true

                    ColumnLayout {
                        anchors.centerIn: parent
                        visible: !inspectionData.valid && !hasCompleted
                        spacing: 6
                        Text { Layout.alignment: Qt.AlignHCenter; text: "📂"; font.pixelSize: 32 }
                        Text {
                            Layout.alignment: Qt.AlignHCenter
                            text: "Select ~/Firm Data/Data.004 to inspect tables and rows"
                            color: "#94A3B8"
                            font.pixelSize: 13
                        }
                    }

                    // Inspection Stats Grid
                    GridLayout {
                        anchors.fill: parent
                        columns: 2
                        columnSpacing: 16
                        rowSpacing: 12
                        visible: (inspectionData && inspectionData.valid === true) && !hasCompleted

                        Rectangle {
                            Layout.fillWidth: true; Layout.fillHeight: true
                            color: "#F0FDF4"; radius: 8; border.color: "#BBF7D0"; border.width: 1
                            ColumnLayout {
                                anchors.centerIn: parent
                                Text { text: (inspectionData.stockTxCount || 0).toString(); color: "#166534"; font.pixelSize: 22; font.bold: true; Layout.alignment: Qt.AlignHCenter }
                                Text { text: "Stock Transactions"; color: "#15803D"; font.pixelSize: 12; Layout.alignment: Qt.AlignHCenter }
                            }
                        }

                        Rectangle {
                            Layout.fillWidth: true; Layout.fillHeight: true
                            color: "#EFF6FF"; radius: 8; border.color: "#BFDBFE"; border.width: 1
                            ColumnLayout {
                                anchors.centerIn: parent
                                Text { text: (inspectionData.millingCount || 0).toString(); color: "#1E40AF"; font.pixelSize: 22; font.bold: true; Layout.alignment: Qt.AlignHCenter }
                                Text { text: "Milling Vouchers"; color: "#1D4ED8"; font.pixelSize: 12; Layout.alignment: Qt.AlignHCenter }
                            }
                        }

                        Rectangle {
                            Layout.fillWidth: true; Layout.fillHeight: true
                            color: "#FAF5FF"; radius: 8; border.color: "#E9D5FF"; border.width: 1
                            ColumnLayout {
                                anchors.centerIn: parent
                                Text { text: (inspectionData.stockItemsCount || 0).toString(); color: "#6B21A8"; font.pixelSize: 22; font.bold: true; Layout.alignment: Qt.AlignHCenter }
                                Text { text: "Stock Items"; color: "#7E22CE"; font.pixelSize: 12; Layout.alignment: Qt.AlignHCenter }
                            }
                        }

                        Rectangle {
                            Layout.fillWidth: true; Layout.fillHeight: true
                            color: "#FFFBEB"; radius: 8; border.color: "#FDE68A"; border.width: 1
                            ColumnLayout {
                                anchors.centerIn: parent
                                Text { text: (inspectionData.ledgersCount || 0).toString(); color: "#92400E"; font.pixelSize: 22; font.bold: true; Layout.alignment: Qt.AlignHCenter }
                                Text { text: "Master Ledgers"; color: "#B45309"; font.pixelSize: 12; Layout.alignment: Qt.AlignHCenter }
                            }
                        }
                    }

                    // Success Result View
                    ColumnLayout {
                        anchors.centerIn: parent
                        visible: hasCompleted
                        spacing: 10
                        Text { Layout.alignment: Qt.AlignHCenter; text: "✅"; font.pixelSize: 40 }
                        Text {
                            Layout.alignment: Qt.AlignHCenter
                            text: completionMessage
                            color: "#166534"
                            font.pixelSize: 14
                            font.bold: true
                            wrapMode: Text.WordWrap
                            Layout.maximumWidth: 500
                            horizontalAlignment: Text.AlignHCenter
                        }
                    }
                }
            }
        }

        // Progress Section
        ColumnLayout {
            Layout.fillWidth: true
            spacing: 6
            visible: (typeof bahiKhataMigrator !== "undefined" && bahiKhataMigrator && bahiKhataMigrator.isMigrating)

            RowLayout {
                Layout.fillWidth: true
                Text { text: bahiKhataMigrator ? bahiKhataMigrator.statusText : ""; color: "#2563EB"; font.pixelSize: 12; font.bold: true }
                Item { Layout.fillWidth: true }
                Text { text: (bahiKhataMigrator ? bahiKhataMigrator.progressPercent : 0).toString() + "%"; color: "#2563EB"; font.pixelSize: 12; font.bold: true }
            }

            ProgressBar {
                Layout.fillWidth: true
                value: (bahiKhataMigrator ? bahiKhataMigrator.progressPercent : 0) / 100.0
            }
        }

        // Footer Action Buttons
        RowLayout {
            Layout.fillWidth: true
            spacing: 12

            Button {
                text: hasCompleted ? "Close" : "Cancel"
                onClicked: root.closeRequested()
            }

            Item { Layout.fillWidth: true }

            Button {
                text: "⚡ Start In-App Migration"
                visible: (!hasCompleted && inspectionData && inspectionData.valid === true) ? true : false
                enabled: (typeof bahiKhataMigrator !== "undefined" && bahiKhataMigrator && bahiKhataMigrator.isMigrating) ? false : true
                onClicked: {
                    if (typeof bahiKhataMigrator !== "undefined" && bahiKhataMigrator) {
                        bahiKhataMigrator.migrate_mdb_file(selectedFilePath)
                    }
                }
            }
        }
    }
}
