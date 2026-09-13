import QtQuick
import QtQuick.Templates as T
import QtQuick.Layouts
import MahadevERP

T.ApplicationWindow {
    id: window
    width: 1380
    height: 850
    visible: true
    title: (typeof firmManager !== "undefined" && firmManager && firmManager.currentFirmName !== "" ? (firmManager.currentFirmName + " • ") : "") + "ERP & Accounting System"
    background: Rectangle { color: "#F4F6F9" }

    // Active View Index: 
    // 0=Dashboard, 1=Paddy, 2=Milling/Stock, 3=Sales, 4=Vouchers, 5=Reports/LedgerList, 6=NewLedgerPage, 7=ModifyLedgerPage, 8=ViewStatementPage, 9=NewGroup, 10=ModifyGroup, 11=NewStockItem, 12=ModifyStockItem, 13=StockDetail, 14=SalesVoucher, 15=PurchaseVoucher, 22=FirmSelector
    property int currentViewIndex: 22
    property var navigationHistory: []
    property string lastViewedPartyName: ""
    property int lastDashboardMenuIndex: 0
    property int lastActiveMenuType: 0 // 0=Dashboard Root, 1=Ledger Menu, 2=Stock Menu, 3=AddVoucher Menu
    property int lastLedgerSubmenuIndex: 0
    property int lastStockSubmenuIndex: 0
    property int lastVoucherSubmenuIndex: 0
    property int lastOtherSubmenuIndex: 0
    property int lastReportsSubmenuIndex: 0
    property bool isShortcutsModalOpen: false
    property bool isPaddyModalOpen: false
    property bool isItemMovementModalOpen: false
    property bool isPeriodModalOpen: false
    property bool isMdbModalOpen: false
    property string activePeriodLabel: "FY 2026-27"
    property string pendingEditInvoiceNo: ""
    property int targetTdsVoucherId: 0
    property string targetStatementParty: ""

    function navigateToView(targetIndex) {
        if (window.currentViewIndex !== targetIndex) {
            if (window.currentViewIndex !== 22) {
                window.navigationHistory.push(window.currentViewIndex)
            }
            window.currentViewIndex = targetIndex
        }
    }

    function navigateBack() {
        if (window.navigationHistory.length > 0) {
            var prevIndex = window.navigationHistory.pop()
            window.currentViewIndex = prevIndex
        } else {
            window.currentViewIndex = 0
        }
    }

    Component.onCompleted: {
        var fy = (typeof stockItemsModel !== "undefined" && stockItemsModel) ? stockItemsModel.get_financial_year() : ""
        var sd = (typeof stockItemsModel !== "undefined" && stockItemsModel) ? stockItemsModel.get_from_date() : ""
        var ed = (typeof stockItemsModel !== "undefined" && stockItemsModel) ? stockItemsModel.get_to_date() : ""
        if (fy && sd && ed) {
            var s_fmt = sd.split("-").reverse().join("-")
            var e_fmt = ed.split("-").reverse().join("-")
            window.activePeriodLabel = s_fmt + " To " + e_fmt + " (" + fy + ")"
            if (typeof dashboardCtrl !== "undefined" && dashboardCtrl) {
                dashboardCtrl.refresh_stats(sd, ed, fy)
            }
        }
    }

    Connections {
        target: typeof stockItemsModel !== "undefined" ? stockItemsModel : null
        function onPeriodChanged(fy, fromD, toD) {
            var s_fmt = fromD.indexOf("-") !== -1 ? fromD.split("-").reverse().join("-") : fromD
            var e_fmt = toD.indexOf("-") !== -1 ? toD.split("-").reverse().join("-") : toD
            window.activePeriodLabel = s_fmt + " To " + e_fmt + " (" + fy + ")"
            if (typeof dashboardCtrl !== "undefined" && dashboardCtrl) {
                dashboardCtrl.refresh_stats(fromD, toD, fy)
            }
            if (mainLoader.item && typeof mainLoader.item.activePeriodText !== "undefined") {
                mainLoader.item.activePeriodText = s_fmt + " To " + e_fmt + " (" + fy + ")"
            }
            if (mainLoader.item && typeof mainLoader.item.loadDashboardStats !== "undefined") {
                mainLoader.item.loadDashboardStats()
            }
            if (mainLoader.item && typeof mainLoader.item.loadStockItems !== "undefined") {
                mainLoader.item.loadStockItems()
            }
        }
    }

    function openLedgerMasterMenu() {
        window.lastActiveMenuType = 0
        window.lastLedgerSubmenuIndex = 0
        ledgerMenu.selectedIndex = 0
        ledgerMenu.open()
    }

    function openStockMasterMenu() {
        window.lastActiveMenuType = 0
        window.lastStockSubmenuIndex = 0
        stockMenu.selectedIndex = 0
        stockMenu.open()
    }

    function openAddVoucherMenu() {
        window.lastActiveMenuType = 0
        window.lastVoucherSubmenuIndex = 0
        addVoucherMenu.selectedIndex = 0
        addVoucherMenu.open()
    }

    function openOtherVoucherMenu() {
        window.lastActiveMenuType = 0
        window.lastOtherSubmenuIndex = 0
        otherVoucherMenu.selectedIndex = 0
        otherVoucherMenu.open()
    }

    function openReportsMenu() {
        window.lastActiveMenuType = 0
        window.lastReportsSubmenuIndex = 0
        reportsMenu.selectedIndex = 0
        reportsMenu.open()
    }

    // GLOBAL KEYBOARD SHORTCUTS WITH APPLICATION SCOPE
    Shortcut { sequence: "Alt+1"; context: Qt.ApplicationShortcut; onActivated: { window.navigationHistory = []; window.currentViewIndex = 0; } }
    Shortcut { sequence: "Alt+2"; context: Qt.ApplicationShortcut; onActivated: window.navigateToView(1) }
    Shortcut { sequence: "Alt+3"; context: Qt.ApplicationShortcut; onActivated: window.navigateToView(14) } // Sales Voucher
    Shortcut { sequence: "Alt+4"; context: Qt.ApplicationShortcut; onActivated: window.navigateToView(2) } // Stock/Milling
    Shortcut { sequence: "Alt+5"; context: Qt.ApplicationShortcut; onActivated: window.openOtherVoucherMenu() } // Other Vouchers Menu
    Shortcut { sequence: "Alt+6"; context: Qt.ApplicationShortcut; onActivated: window.navigateToView(5) } // Ledger List / Reports
    Shortcut { sequence: "Alt+7"; context: Qt.ApplicationShortcut; onActivated: window.openReportsMenu() } // Reports Menu
    Shortcut { 
        sequence: "Alt+F1"
        context: Qt.ApplicationShortcut
        onActivated: {
            if (window.currentViewIndex === 22) {
                window.navigateBack()
            } else {
                window.navigateToView(22)
            }
        }
    }
    function triggerSaveActiveView() {
        if (!mainLoader.item) return false
        var itm = mainLoader.item
        if (typeof itm.saveVoucher === "function") { itm.saveVoucher(); return true }
        if (typeof itm.saveInvoice === "function") { itm.saveInvoice(); return true }
        if (typeof itm.saveMillingVoucher === "function") { itm.saveMillingVoucher(); return true }
        if (typeof itm.executeSave === "function") { itm.executeSave(); return true }
        if (typeof itm.saveLedger === "function") { itm.saveLedger(); return true }
        if (typeof itm.saveUpdateLedger === "function") { itm.saveUpdateLedger(); return true }
        if (typeof itm.saveStockItem === "function") { itm.saveStockItem(); return true }
        if (typeof itm.saveUpdateItem === "function") { itm.saveUpdateItem(); return true }
        return false
    }

    Shortcut { 
        sequence: "F2"
        context: Qt.ApplicationShortcut
        onActivated: {
            if (window.currentViewIndex !== 0 && triggerSaveActiveView()) {
                // saved
            } else {
                window.openAddVoucherMenu()
            }
        }
    }
    Shortcut { sequence: "Alt+F2"; context: Qt.ApplicationShortcut; onActivated: window.isPeriodModalOpen = true }
    Shortcut { sequence: "Option+F2"; context: Qt.ApplicationShortcut; onActivated: window.isPeriodModalOpen = true }
    property string targetChequeMode: "Payment"

    Shortcut { 
        sequence: "F3"
        context: Qt.ApplicationShortcut
        onActivated: {
            window.targetChequeMode = "Payment"
            window.navigateToView(16)
            if (mainLoader.item && typeof mainLoader.item.voucherMode !== "undefined") {
                mainLoader.item.voucherMode = "Payment"
            }
        } 
    }
    Shortcut { 
        sequence: "F4"
        context: Qt.ApplicationShortcut
        onActivated: {
            window.targetChequeMode = "Receipt"
            window.navigateToView(16)
            if (mainLoader.item && typeof mainLoader.item.voucherMode !== "undefined") {
                mainLoader.item.voucherMode = "Receipt"
            }
        } 
    }
    Shortcut { 
        sequence: "F5"
        context: Qt.ApplicationShortcut
        onActivated: {
            window.navigateToView(17)
        } 
    }
    Shortcut { sequence: "F8"; context: Qt.ApplicationShortcut; onActivated: window.navigateToView(14) } // Sales Voucher Entry
    Shortcut { sequence: "F9"; context: Qt.ApplicationShortcut; onActivated: window.navigateToView(15) } // Purchase Voucher Entry
    Shortcut { sequence: "F11"; context: Qt.ApplicationShortcut; onActivated: window.navigateToView(23) } // J-Form Mandi Procurement Voucher
    Shortcut { sequence: "F12"; context: Qt.ApplicationShortcut; onActivated: window.navigateToView(24) } // TDS Voucher Entry
    Shortcut { sequence: "Ctrl+B"; context: Qt.ApplicationShortcut; onActivated: window.navigateToView(26) } // Bank Statement Auto-Entry
    Shortcut { sequence: "Alt+T"; context: Qt.ApplicationShortcut; onActivated: window.navigateToView(27) } // Transport & Weighbridge Register
    Shortcut { sequence: "Alt+D"; context: Qt.ApplicationShortcut; onActivated: window.navigateToView(28) } // GST Debit Notes & Credit Notes
    Shortcut { 
        sequences: ["Ctrl+S", "Ctrl+s", "StandardKey.Save"]
        context: Qt.ApplicationShortcut
        onActivated: {
            triggerSaveActiveView()
        }
    }

    function handleUniversalEscape() {
        if (ledgerMenu.opened) {
            window.lastActiveMenuType = 0
            ledgerMenu.close()
        } else if (stockMenu.opened) {
            window.lastActiveMenuType = 0
            stockMenu.close()
        } else if (addVoucherMenu.opened) {
            window.lastActiveMenuType = 0
            addVoucherMenu.close()
        } else if (otherVoucherMenu.opened) {
            window.lastActiveMenuType = 0
            otherVoucherMenu.close()
        } else if (reportsMenu.opened) {
            window.lastActiveMenuType = 0
            reportsMenu.close()
        } else if (window.isShortcutsModalOpen || window.isPaddyModalOpen || window.isItemMovementModalOpen || window.isPeriodModalOpen) {
            window.isShortcutsModalOpen = false
            window.isPaddyModalOpen = false
            window.isItemMovementModalOpen = false
            window.isPeriodModalOpen = false
        } else if (mainLoader.item && typeof mainLoader.item.hasActivePopup === "function" && mainLoader.item.hasActivePopup()) {
            mainLoader.item.closeActivePopup()
        } else if (mainLoader.item && typeof mainLoader.item.handleEscape === "function") {
            mainLoader.item.handleEscape()
        } else if (mainLoader.item && typeof mainLoader.item.cancelRequested !== "undefined") {
            mainLoader.item.cancelRequested()
        } else if (window.currentViewIndex !== 0) {
            window.navigateBack()
        }
    }

    Connections {
        target: typeof globalKeyFilter !== "undefined" ? globalKeyFilter : null
        function onEscapePressed() {
            window.handleUniversalEscape()
        }
    }

    FocusScope {
        anchors.fill: parent
        focus: true
        Keys.onEscapePressed: function(event) {
            event.accepted = true
            window.handleUniversalEscape()
        }
        Keys.onPressed: function(event) {
            if (event.key === Qt.Key_Escape) {
                event.accepted = true
                window.handleUniversalEscape()
            }
        }

        ColumnLayout {
            anchors.fill: parent
            spacing: 0

            // Top Navigation Header - VISIBLE ONLY ON DASHBOARD (Index 0)
            AppHeader {
                Layout.fillWidth: true
                visible: window.currentViewIndex === 0
                activePeriodText: window.activePeriodLabel
                onShowHelpRequested: window.isShortcutsModalOpen = true
                onOpenAccountingPeriodRequested: window.isPeriodModalOpen = true
                onOpenMdbMigrationRequested: window.isMdbModalOpen = true
                onSwitchFirmRequested: window.currentViewIndex = 22
            }

        // Main View Loader Framework
        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true

            Loader {
                id: mainLoader
                anchors.fill: parent
                anchors.margins: window.currentViewIndex === 0 ? 14 : 10
                source: {
                    switch (window.currentViewIndex) {
                        case 0: return "views/DashboardView.qml"
                        case 1: return "views/PaddyProcurementView.qml"
                        case 2: return "views/MillingView.qml"
                        case 3: return "views/SalesInvoicingView.qml"
                        case 4: return "views/VoucherLedgerView.qml"
                        case 5: return "views/ReportsView.qml"
                        case 6: return "views/NewLedgerView.qml"
                        case 7: return "views/ModifyLedgerView.qml"
                        case 8: return "views/ViewLedgerStatementView.qml"
                        case 9: return "views/NewGroupView.qml"
                        case 10: return "views/ModifyGroupView.qml"
                        case 11: return "views/NewStockItemView.qml"
                        case 12: return "views/ModifyStockItemView.qml"
                        case 13: return "views/StockDetailView.qml"
                        case 14: return "views/SalesVoucherView.qml"
                        case 15: return "views/PurchaseVoucherView.qml"
                        case 16: return "views/ChequeVoucherView.qml"
                        case 17: return "views/JournalVoucherView.qml"
                        case 18: return "views/MillingVoucherView.qml"
                        case 19: return "views/MillingStatementView.qml"
                        case 20: return "views/SalesRegisterView.qml"
                        case 21: return "views/PurchaseRegisterView.qml"
                        case 22: return "views/FirmSelecterView.qml"
                        case 23: return "views/JFormVoucherView.qml"
                        case 24: return "views/TdsVoucherView.qml"
                        case 25: return "views/InterestCalculatorView.qml"
                        case 26: return "views/BankStatementImportView.qml"
                        case 27: return "views/TransportDispatchRegisterView.qml"
                        case 28: return "views/DebitCreditNoteView.qml"
                        default: return "views/DashboardView.qml"
                    }
                }

                onLoaded: {
                    if (window.currentViewIndex === 8 && item && typeof item.focusSearch === "function") {
                        Qt.callLater(item.focusSearch)
                    }

                    if (window.currentViewIndex === 16 && item && typeof item.voucherMode !== "undefined") {
                        item.voucherMode = window.targetChequeMode
                    }

                    if (window.currentViewIndex === 24 && item) {
                        if (window.targetTdsVoucherId > 0) {
                            if (typeof item.loadVoucher !== "undefined" && typeof tdsModel !== "undefined" && tdsModel) {
                                var vch = tdsModel.get_tds_voucher_by_id(window.targetTdsVoucherId)
                                item.loadVoucher(vch)
                            }
                            window.targetTdsVoucherId = 0
                        }
                    }

                    if (window.currentViewIndex === 0 && item) {
                        if (typeof item.selectedMenuIndex !== "undefined") {
                            item.selectedMenuIndex = window.lastDashboardMenuIndex
                        }

                        // Restore 2-Level Menu Tree Memory (Only once when coming back from a page)
                        if (window.lastActiveMenuType === 1) {
                            ledgerMenu.selectedIndex = window.lastLedgerSubmenuIndex
                            ledgerMenu.open()
                            window.lastActiveMenuType = 0
                            window.lastLedgerSubmenuIndex = 0
                        } else if (window.lastActiveMenuType === 2) {
                            stockMenu.selectedIndex = window.lastStockSubmenuIndex
                            stockMenu.open()
                            window.lastActiveMenuType = 0
                            window.lastStockSubmenuIndex = 0
                        } else if (window.lastActiveMenuType === 3) {
                            addVoucherMenu.selectedIndex = window.lastVoucherSubmenuIndex
                            addVoucherMenu.open()
                            window.lastActiveMenuType = 0
                            window.lastVoucherSubmenuIndex = 0
                        } else if (window.lastActiveMenuType === 4) {
                            otherVoucherMenu.selectedIndex = window.lastOtherSubmenuIndex
                            otherVoucherMenu.open()
                            window.lastActiveMenuType = 0
                            window.lastOtherSubmenuIndex = 0
                        } else if (window.lastActiveMenuType === 5) {
                            reportsMenu.selectedIndex = window.lastReportsSubmenuIndex
                            reportsMenu.open()
                            window.lastActiveMenuType = 0
                            window.lastReportsSubmenuIndex = 0
                        } else {
                            item.forceActiveFocus()
                        }
                    }

                    // Automatically load invoice for editing if pending
                    if (window.pendingEditInvoiceNo !== "") {
                        var invToLoad = window.pendingEditInvoiceNo
                        window.pendingEditInvoiceNo = ""
                        Qt.callLater(function() {
                            if (item && typeof item.loadInvoiceForEditing === "function") {
                                item.loadInvoiceForEditing(invToLoad)
                            }
                        })
                    }
                }
            }

            Connections {
                target: mainLoader.item
                ignoreUnknownSignals: true
                function onOpenAddVoucherMenu() { window.openAddVoucherMenu() }
                function onOpenOtherVoucherMenu() { window.openOtherVoucherMenu() }
                function onOpenReportsMenu() { window.openReportsMenu() }
                function onOpenLedgers() { window.openLedgerMasterMenu() }
                function onOpenStock() { window.openStockMasterMenu() }
                function onOpenPaddy() { window.navigateToView(1) }
                function onOpenLedgerMenu() { window.openLedgerMasterMenu() }
                function onOpenStockMenu() { window.openStockMasterMenu() }
                function onOpenPeriodModal() { window.isPeriodModalOpen = true }
                function onCancelRequested() { window.navigateBack() }
                function onSavedSuccess() { window.navigateBack() }
                function onInvoiceSaved() { window.navigateBack() }
                function onVoucherSaved() { 
                    window.navigateBack()
                    if (typeof dashboardCtrl !== "undefined" && dashboardCtrl) dashboardCtrl.refresh_stats()
                }
                function onInterestVoucherSaved() {
                    window.navigateBack()
                    if (typeof dashboardCtrl !== "undefined" && dashboardCtrl) dashboardCtrl.refresh_stats()
                }
                function onShowNewModal() {
                    window.navigateToView(18)
                }
                function onOpenNewMillingRequested() {
                    window.navigateToView(18)
                }
                function onSelectedMenuIndexChanged() {
                    if (mainLoader.item && typeof mainLoader.item.selectedMenuIndex !== "undefined") {
                        window.lastDashboardMenuIndex = mainLoader.item.selectedMenuIndex
                    }
                }
                function onOpenInvoiceRequested(invNo) {
                    window.pendingEditInvoiceNo = invNo
                    if (window.currentViewIndex === 21) {
                        window.navigateToView(15)
                    } else {
                        window.navigateToView(14)
                    }
                }
                function onOpenItemMovement(itemName) {
                    var fDate = typeof stockItemsModel !== "undefined" && stockItemsModel ? stockItemsModel.get_from_date() : ""
                    var tDate = typeof stockItemsModel !== "undefined" && stockItemsModel ? stockItemsModel.get_to_date() : ""
                    itemMovementModal.loadItemMovements(itemName, fDate, tDate)
                    window.isItemMovementModalOpen = true
                }
                function onFirmOpened(firmId, firmName) {
                    window.navigationHistory = []
                    window.currentViewIndex = 0
                    var fy = (typeof stockItemsModel !== "undefined" && stockItemsModel) ? stockItemsModel.get_financial_year() : ""
                    var sd = (typeof stockItemsModel !== "undefined" && stockItemsModel) ? stockItemsModel.get_from_date() : ""
                    var ed = (typeof stockItemsModel !== "undefined" && stockItemsModel) ? stockItemsModel.get_to_date() : ""
                    if (fy && sd && ed) {
                        var s_fmt = sd.split("-").reverse().join("-")
                        var e_fmt = ed.split("-").reverse().join("-")
                        window.activePeriodLabel = s_fmt + " To " + e_fmt + " (" + fy + ")"
                        if (typeof dashboardCtrl !== "undefined" && dashboardCtrl) {
                            dashboardCtrl.refresh_stats(sd, ed, fy)
                        }
                    }
                }
            }
        }
    }

    // TREE-LIKE STATE MEMORY POPUP SUBMENUS
    LedgerMasterMenuModal {
        id: ledgerMenu
        anchors.centerIn: parent
        onClosed: {
            if (window.lastActiveMenuType === 0) ledgerMenu.selectedIndex = 0
            if (window.lastActiveMenuType === 0 && mainLoader.item) mainLoader.item.forceActiveFocus()
        }
        onActionSelected: function(act, selIdx) {
            window.lastActiveMenuType = 1
            window.lastLedgerSubmenuIndex = selIdx
            window.lastDashboardMenuIndex = 0
            if (act === "New Ledger") {
                window.navigateToView(6)
            } else if (act === "Modify Ledger") {
                window.navigateToView(7)
            } else if (act === "View Ledger") {
                window.navigateToView(8)
            } else if (act === "New Group") {
                window.navigateToView(9)
            } else if (act === "Modify Group") {
                window.navigateToView(10)
            } else {
                window.navigateToView(5)
            }
        }
    }

    StockMasterMenuModal {
        id: stockMenu
        anchors.centerIn: parent
        onClosed: {
            if (window.lastActiveMenuType === 0) stockMenu.selectedIndex = 0
            if (window.lastActiveMenuType === 0 && mainLoader.item) mainLoader.item.forceActiveFocus()
        }
        onActionSelected: function(act, selIdx) {
            window.lastActiveMenuType = 2
            window.lastStockSubmenuIndex = selIdx
            window.lastDashboardMenuIndex = 1
            if (act === "New Stock Item") {
                window.navigateToView(11)
            } else if (act === "Modify Stock Item") {
                window.navigateToView(12)
            } else if (act === "Stock Details") {
                window.navigateToView(13)
            } else {
                window.navigateToView(2)
            }
        }
    }

    AddVoucherMenuModal {
        id: addVoucherMenu
        anchors.centerIn: parent
        onClosed: {
            if (window.lastActiveMenuType === 0) addVoucherMenu.selectedIndex = 0
            if (window.lastActiveMenuType === 0 && mainLoader.item) mainLoader.item.forceActiveFocus()
        }
        onOptionSelected: function(opt, selIdx) {
            window.lastActiveMenuType = 3
            window.lastVoucherSubmenuIndex = selIdx
            window.lastDashboardMenuIndex = 2
            if (opt === 1) {
                window.navigateToView(14)
            } else if (opt === 2) {
                window.navigateToView(15)
            } else if (opt === 3) {
                window.targetChequeMode = "Payment"
                window.navigateToView(16)
                if (mainLoader.item && typeof mainLoader.item.voucherMode !== "undefined") mainLoader.item.voucherMode = "Payment"
            } else if (opt === 4) {
                window.targetChequeMode = "Receipt"
                window.navigateToView(16)
                if (mainLoader.item && typeof mainLoader.item.voucherMode !== "undefined") mainLoader.item.voucherMode = "Receipt"
            } else if (opt === 5) {
                window.navigateToView(17)
            } else if (opt === 6) {
                window.navigateToView(18)
            }
        }
    }

    OtherVouchersMenuModal {
        id: otherVoucherMenu
        anchors.centerIn: parent
        onClosed: {
            if (window.lastActiveMenuType === 0) otherVoucherMenu.selectedIndex = 0
            if (window.lastActiveMenuType === 0 && mainLoader.item) mainLoader.item.forceActiveFocus()
        }
        onOptionSelected: function(opt, selIdx) {
            window.lastActiveMenuType = 4
            window.lastOtherSubmenuIndex = selIdx
            window.lastDashboardMenuIndex = 3
            if (opt === 1) {
                window.navigateToView(23)
            } else if (opt === 2) {
                window.navigateToView(24)
            } else if (opt === 3) {
                window.navigateToView(26)
            } else if (opt === 4) {
                window.navigateToView(27)
            } else if (opt === 5) {
                window.navigateToView(28)
            }
        }
    }

    ReportsMenuModal {
        id: reportsMenu
        anchors.centerIn: parent
        onClosed: {
            if (window.lastActiveMenuType === 0) reportsMenu.selectedIndex = 0
            if (window.lastActiveMenuType === 0 && mainLoader.item) mainLoader.item.forceActiveFocus()
        }
        onActionSelected: function(act, selIdx) {
            window.lastActiveMenuType = 5
            window.lastReportsSubmenuIndex = selIdx
            window.lastDashboardMenuIndex = 4
            if (act === "Milling Statement") {
                window.navigateToView(19)
            } else if (act === "Stock Register") {
                window.navigateToView(13)
            } else if (act === "Item Movement") {
                window.navigateToView(13)
            } else if (act === "Ledger Statement") {
                window.navigateToView(8)
            } else if (act === "Sales Register") {
                window.navigateToView(20)
            } else if (act === "Purchase Register") {
                window.navigateToView(21)
            } else if (act === "Interest Calculator") {
                window.navigateToView(25)
            } else if (act === "Transport Register") {
                window.navigateToView(27)
            } else if (act === "Debit Credit Notes") {
                window.navigateToView(28)
            }
        }
    }

    // Modal Overlays Container for custom full dialogs
    Rectangle {
        anchors.fill: parent
        color: "#66000000"
        visible: window.isShortcutsModalOpen || window.isPaddyModalOpen || window.isItemMovementModalOpen || window.isPeriodModalOpen || window.isMdbModalOpen

        MouseArea {
            anchors.fill: parent
            onClicked: {
                window.isShortcutsModalOpen = false
                window.isPaddyModalOpen = false
                window.isItemMovementModalOpen = false
                window.isPeriodModalOpen = false
                window.isMdbModalOpen = false
            }
        }

        // Bahi Khata In-App MDB Migration Modal
        MdbMigrationModal {
            id: mdbMigrationModal
            anchors.centerIn: parent
            visible: window.isMdbModalOpen
            onCloseRequested: window.isMdbModalOpen = false
            onMigrationSuccess: {
                if (typeof dashboardCtrl !== "undefined" && dashboardCtrl) {
                    dashboardCtrl.refresh_stats()
                }
                if (mainLoader.item && typeof mainLoader.item.loadDashboardStats !== "undefined") {
                    mainLoader.item.loadDashboardStats()
                }
                if (mainLoader.item && typeof mainLoader.item.loadStockItems !== "undefined") {
                    mainLoader.item.loadStockItems()
                }
            }
        }

        // Accounting Period / Financial Year Modal (Bahi-Khata Style)
        AccountingPeriodModal {
            id: accountingPeriodModal
            anchors.centerIn: parent
            visible: window.isPeriodModalOpen
            onCloseRequested: window.isPeriodModalOpen = false
            onPeriodSelected: function(fromIso, toIso, fyLabel) {
                if (typeof stockItemsModel !== "undefined" && stockItemsModel) {
                    stockItemsModel.set_accounting_period(fromIso, toIso, fyLabel)
                }
                var s_fmt = fromIso.indexOf("-") !== -1 ? fromIso.split("-").reverse().join("-") : fromIso
                var e_fmt = toIso.indexOf("-") !== -1 ? toIso.split("-").reverse().join("-") : toIso
                window.activePeriodLabel = s_fmt + " To " + e_fmt + " (" + fyLabel + ")"
                if (typeof dashboardCtrl !== "undefined" && dashboardCtrl) {
                    dashboardCtrl.refresh_stats(fromIso, toIso, fyLabel)
                }
                if (mainLoader.item && typeof mainLoader.item.activePeriodText !== "undefined") {
                    mainLoader.item.activePeriodText = s_fmt + " To " + e_fmt + " (" + fyLabel + ")"
                }
                if (mainLoader.item && typeof mainLoader.item.loadDashboardStats !== "undefined") {
                    mainLoader.item.loadDashboardStats()
                }
                if (mainLoader.item && typeof mainLoader.item.loadStockItems !== "undefined") {
                    mainLoader.item.loadStockItems()
                }
                if (mainLoader.item && typeof mainLoader.item.syncDateInputsWithActivePeriod === "function") {
                    mainLoader.item.syncDateInputsWithActivePeriod()
                }
                if (mainLoader.item && typeof mainLoader.item.syncWithActivePeriod === "function") {
                    mainLoader.item.syncWithActivePeriod()
                }
                if (mainLoader.item && typeof mainLoader.item.reloadStatementData === "function") {
                    mainLoader.item.reloadStatementData()
                }
                if (mainLoader.item && typeof mainLoader.item.loadPartyStatement !== "undefined") {
                    mainLoader.item.loadPartyStatement(mainLoader.item.currentPartyName || "")
                }
            }
        }

        // Shortcuts Cheatsheet
        KeyboardShortcutsModal {
            anchors.centerIn: parent
            visible: window.isShortcutsModalOpen
            onCloseRequested: window.isShortcutsModalOpen = false
        }

        // New Paddy Arrival Entry
        NewPaddyModal {
            anchors.centerIn: parent
            visible: window.isPaddyModalOpen
            onCloseRequested: window.isPaddyModalOpen = false
            onSavedSuccess: window.isPaddyModalOpen = false
        }

        // SIDE-BY-SIDE ITEM MOVEMENT POPUP MODAL
        ItemMovementModal {
            id: itemMovementModal
            anchors.centerIn: parent
            visible: window.isItemMovementModalOpen
            onCloseRequested: window.isItemMovementModalOpen = false
            onOpenInvoiceRequested: function(invNo, invType) {
                window.isItemMovementModalOpen = false
                window.pendingEditInvoiceNo = invNo
                if (invType === "Purchase") {
                    window.currentViewIndex = 15 // PurchaseVoucherView
                } else {
                    window.currentViewIndex = 14 // SalesVoucherView
                }
            }
        }
    }
}
}
