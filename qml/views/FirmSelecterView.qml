import QtQuick
import QtQuick.Templates as T
import QtQuick.Layouts
import QtQuick.Dialogs
import MahadevERP

Rectangle {
    id: root
    anchors.fill: parent
    color: "#F8FAFC" // Clean, bright light theme background matching ERP

    signal firmOpened(string firmId, string firmName)
    signal cancelRequested()

    property string appDataFolder: (typeof firmManager !== "undefined" && firmManager) ? firmManager.get_app_data_folder() : "data"
    property string currentFolder: appDataFolder
    property bool isViewingAppData: (currentFolder === appDataFolder || currentFolder.endsWith("/data") || currentFolder.endsWith("/data/") || currentFolder === "data")

    property var firmsList: []
    property int selectedIndex: 0

    Component.onCompleted: {
        root.currentFolder = root.appDataFolder
        refreshFirms()
        listView.forceActiveFocus()
    }

    function refreshFirms() {
        if (typeof firmManager !== "undefined" && firmManager) {
            firmsList = firmManager.scan_folder_for_firms(currentFolder)
            if (firmsList.length === 0 && !root.isViewingAppData) {
                firmsList = firmManager.get_registered_firms()
            }
        }
        firmsModel.clear()
        for (var i = 0; i < firmsList.length; i++) {
            firmsModel.append(firmsList[i])
            if (firmsList[i].isActive === true) {
                root.selectedIndex = i
            }
        }
    }

    function openSelectedFirm() {
        if (selectedIndex >= 0 && selectedIndex < firmsModel.count) {
            var f = firmsModel.get(selectedIndex)
            if (!root.isViewingAppData) {
                // External Bahi-Khata mode: trigger full data import
                root.startImportForFirm(f)
                return
            }
            if (typeof firmManager !== "undefined" && firmManager) {
                var ok = firmManager.switch_to_firm(f.id)
                if (ok) {
                    root.firmOpened(f.id, f.name)
                }
            }
        }
    }

    function startImportForFirm(f) {
        if (!f) return
        var filePath = f.full_path || (root.currentFolder + "/" + f.source_file)
        if (typeof firmManager !== "undefined" && firmManager) {
            firmManager.prepare_firm_for_import(filePath, f.name, f.id)
        }
        firmMigrationModal.selectedFilePath = filePath
        firmMigrationModal.inspectFile(filePath)
        firmMigrationModal.visible = true
        if (typeof bahiKhataMigrator !== "undefined" && bahiKhataMigrator) {
            bahiKhataMigrator.migrate_mdb_file(filePath)
        }
    }

    function resetToAppData() {
        root.currentFolder = root.appDataFolder
        if (typeof firmManager !== "undefined" && firmManager) {
            firmManager.set_active_firm_folder(root.appDataFolder)
        }
        refreshFirms()
    }

    GenericListModel {
        id: firmsModel
    }

    FolderDialog {
        id: folderDialog
        title: "Select Bahi-Khata Firm Data Folder"
        currentFolder: "file://" + (root.isViewingAppData ? "/Users/karan/Firm Data" : root.currentFolder)
        onAccepted: {
            var path = selectedFolder.toString().replace("file://", "")
            root.currentFolder = path
            if (typeof firmManager !== "undefined" && firmManager) {
                firmManager.set_active_firm_folder(path)
            }
            root.refreshFirms()
        }
    }

    NewFirmModal {
        id: newFirmModal
        anchors.centerIn: parent
        onFirmCreated: function(firmId, firmName) {
            root.resetToAppData()
            root.firmOpened(firmId, firmName)
        }
    }

    MdbMigrationModal {
        id: firmMigrationModal
        anchors.centerIn: parent
        visible: false
        onCloseRequested: firmMigrationModal.visible = false
        onMigrationSuccess: {
            firmMigrationModal.visible = false
            root.resetToAppData()
            root.refreshFirms()
            root.firmOpened(firmManager.currentFirmId, firmManager.currentFirmName)
        }
    }

    // MAIN CONTENT CONTAINER
    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 20
        spacing: 14

        // 1. TOP HEADER CARD
        Rectangle {
            Layout.fillWidth: true
            height: 64
            radius: 10
            color: "#FFFFFF"
            border.color: "#E2E8F0"
            border.width: 1

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 18
                anchors.rightMargin: 18
                spacing: 14

                Rectangle {
                    width: 40; height: 40; radius: 8
                    color: "#EFF6FF"; border.color: "#BFDBFE"; border.width: 1
                    Text { anchors.centerIn: parent; text: "🏛️"; font.pixelSize: 20 }
                }

                ColumnLayout {
                    spacing: 1
                    Text {
                        text: "SELECT COMPANY / FIRM"
                        color: "#0F172A"
                        font.pixelSize: 17
                        font.bold: true
                        font.letterSpacing: 0.5
                    }
                    Text {
                        text: root.isViewingAppData ? 
                              "App Working Directory • Managing native SQLite company databases in data/" :
                              "Bahi-Khata External Import • Select a firm to import into the app's working directory"
                        color: "#64748B"
                        font.pixelSize: 11
                    }
                }

                Item { Layout.fillWidth: true }

                T.Button {
                    visible: (typeof firmManager !== "undefined" && firmManager && firmManager.currentFirmName !== "")
                    implicitWidth: 190
                    implicitHeight: 32
                    background: Rectangle { color: "#F1F5F9"; radius: 6; border.color: "#CBD5E1" }
                    contentItem: Text { 
                        text: "← Back to Dashboard (Esc)"
                        color: "#475569"
                        font.pixelSize: 12
                        font.bold: true
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                    onClicked: root.cancelRequested()
                }
            }
        }

        // 2. DIRECTORY CONTROLS & IMPORT BAR
        Rectangle {
            Layout.fillWidth: true
            height: 52
            radius: 8
            color: "#FFFFFF"
            border.color: root.isViewingAppData ? "#E2E8F0" : "#FDE68A"
            border.width: 1

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 16; anchors.rightMargin: 16
                spacing: 12

                Text {
                    text: root.isViewingAppData ? "📁 App Data Directory:" : "📂 Bahi-Khata Folder:"
                    color: root.isViewingAppData ? "#334155" : "#B45309"
                    font.pixelSize: 12
                    font.bold: true
                }

                T.TextField {
                    id: folderInput
                    Layout.fillWidth: true
                    implicitHeight: 34
                    verticalAlignment: TextInput.AlignVCenter
                    leftPadding: 10
                    rightPadding: 10
                    topPadding: 0
                    bottomPadding: 0
                    text: root.currentFolder
                    color: "#0F172A"
                    font.pixelSize: 12
                    font.family: "Segoe UI, Consolas, Menlo, monospace"
                    background: Rectangle { 
                        color: root.isViewingAppData ? "#F8FAFC" : "#FFFBEB"
                        radius: 6
                        border.color: root.isViewingAppData ? "#CBD5E1" : "#FCD34D"
                    }
                    onAccepted: {
                        root.currentFolder = text.trim()
                        if (typeof firmManager !== "undefined" && firmManager) {
                            firmManager.set_active_firm_folder(root.currentFolder)
                        }
                        root.refreshFirms()
                    }
                }

                // App Data Folder Button (when in external view)
                T.Button {
                    visible: !root.isViewingAppData
                    implicitWidth: 140
                    implicitHeight: 34
                    background: Rectangle { color: "#F1F5F9"; radius: 6; border.color: "#CBD5E1" }
                    contentItem: Text { 
                        text: "📁 App Data Folder"
                        color: "#1E293B"
                        font.bold: true
                        font.pixelSize: 12
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                    onClicked: root.resetToAppData()
                }

                // Import from Bahi-Khata Button
                T.Button {
                    implicitWidth: 200
                    implicitHeight: 34
                    background: Rectangle { 
                        color: root.isViewingAppData ? "#EFF6FF" : "#2563EB"
                        radius: 6
                        border.color: root.isViewingAppData ? "#BFDBFE" : "#2563EB"
                    }
                    contentItem: Text { 
                        text: "📥 Import from Bahi-Khata (F4)"
                        color: root.isViewingAppData ? "#1D4ED8" : "#FFFFFF"
                        font.bold: true
                        font.pixelSize: 12 
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                    onClicked: folderDialog.open()
                }

                T.Button {
                    implicitWidth: 100
                    implicitHeight: 34
                    background: Rectangle { color: "#F1F5F9"; radius: 6; border.color: "#CBD5E1" }
                    contentItem: Text { 
                        text: "Browse (F3)"
                        color: "#334155"
                        font.bold: true
                        font.pixelSize: 12
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                    onClicked: folderDialog.open()
                }

                T.Button {
                    implicitWidth: 115
                    implicitHeight: 34
                    background: Rectangle { color: "#F1F5F9"; radius: 6; border.color: "#CBD5E1" }
                    contentItem: Text { 
                        text: "🔄 Rescan (F5)"
                        color: "#334155"
                        font.bold: true
                        font.pixelSize: 12
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                    onClicked: root.refreshFirms()
                }
            }
        }

        // 3. TABLE OF FIRMS CONTAINER
        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            radius: 10
            color: "#FFFFFF"
            border.color: "#E2E8F0"
            border.width: 1
            clip: true

            ColumnLayout {
                anchors.fill: parent
                spacing: 0

                // Table Header
                Rectangle {
                    Layout.fillWidth: true
                    height: 38
                    color: "#F1F5F9"

                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: 16; anchors.rightMargin: 16
                        spacing: 8

                        Text { 
                            text: root.isViewingAppData ? "Database File" : "Source"
                            color: "#475569"; font.pixelSize: 11; font.bold: true; Layout.preferredWidth: 150 
                        }
                        Text { text: "Company / Firm Name"; color: "#475569"; font.pixelSize: 11; font.bold: true; Layout.fillWidth: true }
                        Text { text: "Period (Financial Years)"; color: "#475569"; font.pixelSize: 11; font.bold: true; Layout.preferredWidth: 170 }
                        Text { text: "GSTIN"; color: "#475569"; font.pixelSize: 11; font.bold: true; Layout.preferredWidth: 140 }
                        Text { text: "City / Station"; color: "#475569"; font.pixelSize: 11; font.bold: true; Layout.preferredWidth: 90 }
                        Text { text: "Entity Type"; color: "#475569"; font.pixelSize: 11; font.bold: true; Layout.preferredWidth: 130 }
                        Text { text: "Status"; color: "#475569"; font.pixelSize: 11; font.bold: true; Layout.preferredWidth: 80; horizontalAlignment: Text.AlignRight }
                    }
                }

                Rectangle { Layout.fillWidth: true; height: 1; color: "#E2E8F0" }

                // Empty State
                Item {
                    visible: firmsModel.count === 0
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    ColumnLayout {
                        anchors.centerIn: parent
                        spacing: 12
                        Text { text: "📂"; font.pixelSize: 36; Layout.alignment: Qt.AlignHCenter }
                        Text { 
                            text: root.isViewingAppData ? 
                                  "No native databases found in data/ folder." :
                                  "No Bahi-Khata Data.* files found in selected directory."
                            color: "#64748B"; font.pixelSize: 14; Layout.alignment: Qt.AlignHCenter 
                        }
                        T.Button {
                            Layout.alignment: Qt.AlignHCenter
                            implicitWidth: 220
                            implicitHeight: 36
                            background: Rectangle { color: "#EFF6FF"; radius: 6; border.color: "#BFDBFE" }
                            contentItem: Text { 
                                text: root.isViewingAppData ? "Import from Bahi-Khata" : "Return to App Data Folder"
                                color: "#1D4ED8"
                                font.bold: true
                                font.pixelSize: 12
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                            }
                            onClicked: root.isViewingAppData ? folderDialog.open() : root.resetToAppData()
                        }
                    }
                }

                // List View
                ListView {
                    id: listView
                    visible: firmsModel.count > 0
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true
                    model: firmsModel
                    currentIndex: root.selectedIndex
                    boundsBehavior: Flickable.StopAtBounds

                    T.ScrollBar.vertical: T.ScrollBar {
                        policy: T.ScrollBar.AsNeeded
                        active: true
                    }

                    Keys.onReturnPressed: root.openSelectedFirm()
                    Keys.onEnterPressed: root.openSelectedFirm()
                    Keys.onUpPressed: {
                        if (root.selectedIndex > 0) root.selectedIndex--
                    }
                    Keys.onDownPressed: {
                        if (root.selectedIndex < firmsModel.count - 1) root.selectedIndex++
                    }

                    delegate: Rectangle {
                        width: listView.width
                        height: 44
                        color: index === root.selectedIndex ? "#EFF6FF" : (index % 2 === 0 ? "#FFFFFF" : "#F8FAFC")
                        border.color: index === root.selectedIndex ? "#3B82F6" : "transparent"
                        border.width: index === root.selectedIndex ? 1.5 : 0

                        MouseArea {
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: {
                                root.selectedIndex = index
                                listView.forceActiveFocus()
                            }
                            onDoubleClicked: {
                                root.selectedIndex = index
                                root.openSelectedFirm()
                            }
                        }

                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: 16; anchors.rightMargin: 16
                            spacing: 8

                            Text {
                                text: root.isViewingAppData ? (model.db_name || model.id + ".db") : (model.source_file || "Data.*")
                                color: "#0284C7"
                                font.pixelSize: 12
                                font.bold: true
                                font.family: "Segoe UI, Consolas, monospace"
                                Layout.preferredWidth: 150
                                elide: Text.ElideRight
                            }

                            RowLayout {
                                Layout.fillWidth: true
                                spacing: 8
                                Text {
                                    text: model.name || "Unnamed Firm"
                                    color: index === root.selectedIndex ? "#1D4ED8" : "#0F172A"
                                    font.pixelSize: 13
                                    font.bold: true
                                    elide: Text.ElideRight
                                }
                                Rectangle {
                                    visible: model.isActive === true
                                    width: 54; height: 18; radius: 9
                                    color: "#DCFCE7"
                                    border.color: "#86EFAC"
                                    Text { anchors.centerIn: parent; text: "ACTIVE"; color: "#166534"; font.pixelSize: 9; font.bold: true }
                                }
                            }

                            Text {
                                text: model.period || "All Fiscal Years"
                                color: "#475569"
                                font.pixelSize: 11
                                Layout.preferredWidth: 170
                            }

                            Text {
                                text: model.gstin || "-"
                                color: "#64748B"
                                font.pixelSize: 11
                                font.family: "Segoe UI, Consolas, monospace"
                                Layout.preferredWidth: 140
                            }

                            Text {
                                text: model.city || "Sirsa"
                                color: "#334155"
                                font.pixelSize: 11
                                Layout.preferredWidth: 90
                            }

                            Text {
                                text: model.firm_type || "Partnership"
                                color: "#64748B"
                                font.pixelSize: 11
                                Layout.preferredWidth: 130
                                elide: Text.ElideRight
                            }

                            Rectangle {
                                Layout.preferredWidth: 80; height: 24; radius: 4
                                color: (!root.isViewingAppData && !model.is_imported) ? "#2563EB" : (model.is_imported ? "#DCFCE7" : "#FEF3C7")
                                border.color: (!root.isViewingAppData && !model.is_imported) ? "#1D4ED8" : (model.is_imported ? "#BBF7D0" : "#FDE68A")
                                Text {
                                    anchors.centerIn: parent
                                    text: (!root.isViewingAppData && !model.is_imported) ? "⚡ Import" : (model.is_imported ? "Ready" : "Import")
                                    color: (!root.isViewingAppData && !model.is_imported) ? "#FFFFFF" : (model.is_imported ? "#15803D" : "#B45309")
                                    font.pixelSize: 10
                                    font.bold: true
                                }
                                MouseArea {
                                    anchors.fill: parent
                                    cursorShape: Qt.PointingHandCursor
                                    onClicked: {
                                        root.selectedIndex = index
                                        if (!root.isViewingAppData) {
                                            root.startImportForFirm(firmsModel.get(index))
                                        } else {
                                            root.openSelectedFirm()
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }

        // 4. ACTION BAR FOOTER
        RowLayout {
            Layout.fillWidth: true
            height: 42
            spacing: 12

            T.Button {
                implicitWidth: 190
                implicitHeight: 36
                background: Rectangle { color: "#16A34A"; radius: 6 }
                contentItem: Text { 
                    text: "➕ Create New Firm (F2)"
                    color: "#FFFFFF"
                    font.bold: true
                    font.pixelSize: 12 
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
                onClicked: newFirmModal.open()
            }

            Item { Layout.fillWidth: true }

            T.Button {
                implicitWidth: 230
                implicitHeight: 36
                background: Rectangle { color: "#2563EB"; radius: 6 }
                contentItem: Text { 
                    text: (!root.isViewingAppData && selectedIndex >= 0 && selectedIndex < firmsModel.count && !firmsModel.get(selectedIndex).is_imported) ?
                          "✓ Import & Open Firm (Enter)" :
                          "✓ Open Selected Firm (Enter)"
                    color: "#FFFFFF"
                    font.bold: true
                    font.pixelSize: 12 
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
                onClicked: root.openSelectedFirm()
            }
        }
    }

    // Keyboard Shortcuts
    Shortcut {
        sequence: "F2"
        onActivated: newFirmModal.open()
    }
    Shortcut {
        sequence: "F3"
        onActivated: folderDialog.open()
    }
    Shortcut {
        sequence: "F4"
        onActivated: folderDialog.open()
    }
    Shortcut {
        sequence: "F5"
        onActivated: root.refreshFirms()
    }
    Shortcut {
        sequence: "Escape"
        onActivated: {
            if (!root.isViewingAppData) {
                root.resetToAppData()
            } else if (typeof firmManager !== "undefined" && firmManager && firmManager.currentFirmName !== "") {
                root.cancelRequested()
            }
        }
    }
}
