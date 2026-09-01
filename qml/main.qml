import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "components"
import "views"
import "dialogs"

ApplicationWindow {
    id: window
    width: 1380
    height: 850
    visible: true
    title: "Mahadev Rice Mill ERP & Accounting"
    color: "#F4F6F9"

    // Active View Index: 
    // 0=Dashboard, 1=Paddy, 2=Milling/Stock, 3=Sales, 4=Vouchers, 5=Reports/LedgerList, 6=NewLedgerPage, 7=ModifyLedgerPage, 8=ViewStatementPage, 9=NewGroup, 10=ModifyGroup, 11=NewStockItem, 12=ModifyStockItem, 13=StockDetail, 14=SalesVoucher, 15=PurchaseVoucher
    property int currentViewIndex: 0
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
    property string activePeriodLabel: "01-04-2025 To 31-03-2026 (FY 2025-26)"

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
    Shortcut { sequence: "Alt+1"; context: Qt.ApplicationShortcut; onActivated: window.currentViewIndex = 0 }
    Shortcut { sequence: "Alt+2"; context: Qt.ApplicationShortcut; onActivated: window.currentViewIndex = 1 }
    Shortcut { sequence: "Alt+3"; context: Qt.ApplicationShortcut; onActivated: window.currentViewIndex = 14 } // Sales Voucher
    Shortcut { sequence: "Alt+4"; context: Qt.ApplicationShortcut; onActivated: window.currentViewIndex = 2 } // Stock/Milling
    Shortcut { sequence: "Alt+5"; context: Qt.ApplicationShortcut; onActivated: window.openOtherVoucherMenu() } // Other Vouchers Menu
    Shortcut { sequence: "Alt+6"; context: Qt.ApplicationShortcut; onActivated: window.currentViewIndex = 5 } // Ledger List / Reports
    Shortcut { sequence: "Alt+7"; context: Qt.ApplicationShortcut; onActivated: window.openReportsMenu() } // Reports Menu
    Shortcut { sequence: "F1"; context: Qt.ApplicationShortcut; onActivated: window.isShortcutsModalOpen = !window.isShortcutsModalOpen }
    Shortcut { sequence: "F2"; context: Qt.ApplicationShortcut; onActivated: window.openAddVoucherMenu() }
    Shortcut { sequence: "Alt+F2"; context: Qt.ApplicationShortcut; onActivated: window.isPeriodModalOpen = true }
    Shortcut { sequence: "Option+F2"; context: Qt.ApplicationShortcut; onActivated: window.isPeriodModalOpen = true }
    property string targetChequeMode: "Payment"

    Shortcut { 
        sequence: "F3"
        context: Qt.ApplicationShortcut
        onActivated: {
            window.targetChequeMode = "Payment"
            window.currentViewIndex = 16
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
            window.currentViewIndex = 16
            if (mainLoader.item && typeof mainLoader.item.voucherMode !== "undefined") {
                mainLoader.item.voucherMode = "Receipt"
            }
        } 
    }
    Shortcut { 
        sequence: "F5"
        context: Qt.ApplicationShortcut
        onActivated: {
            window.currentViewIndex = 17
        } 
    }
    Shortcut { sequence: "F8"; context: Qt.ApplicationShortcut; onActivated: window.currentViewIndex = 14 } // Sales Voucher Entry
    Shortcut { sequence: "F9"; context: Qt.ApplicationShortcut; onActivated: window.currentViewIndex = 15 } // Purchase Voucher Entry
    Shortcut { 
        sequence: "Ctrl+S"
        context: Qt.ApplicationShortcut
        onActivated: {
            if (mainLoader.item && typeof mainLoader.item.saveVoucher === "function") {
                mainLoader.item.saveVoucher()
            } else if (mainLoader.item && typeof mainLoader.item.saveInvoice === "function") {
                mainLoader.item.saveInvoice()
            }
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
        } else if (jformStubModal.opened) {
            jformStubModal.close()
        } else if (window.isShortcutsModalOpen || window.isPaddyModalOpen || window.isItemMovementModalOpen || window.isPeriodModalOpen) {
            window.isShortcutsModalOpen = false
            window.isPaddyModalOpen = false
            window.isItemMovementModalOpen = false
            window.isPeriodModalOpen = false
        } else if (mainLoader.item && typeof mainLoader.item.hasActivePopup === "function" && mainLoader.item.hasActivePopup()) {
            mainLoader.item.closeActivePopup()
        } else if (window.currentViewIndex !== 0) {
            window.currentViewIndex = 0 // Go back to Dashboard
        }
    }

    // UNIVERSAL ESCAPE KEY - CLOSE ACTIVE POPUPS, MODALS, OR RETURN TO DASHBOARD
    Shortcut {
        sequence: "Esc"
        context: Qt.ApplicationShortcut
        onActivated: window.handleUniversalEscape()
    }

    FocusScope {
        anchors.fill: parent
        focus: true
        Keys.onEscapePressed: function(event) {
            event.accepted = true
            window.handleUniversalEscape()
        }

        ColumnLayout {
            anchors.fill: parent
            spacing: 0

            // Top Navigation Header - VISIBLE ONLY ON DASHBOARD (Index 0)
            AppHeader {
                Layout.fillWidth: true
                visible: window.currentViewIndex === 0
                onShowHelpRequested: window.isShortcutsModalOpen = true
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
                        default: return "views/DashboardView.qml"
                    }
                }

                onLoaded: {
                    if (window.currentViewIndex === 2 && item) {
                        if (typeof item.showNewModal !== "undefined") {
                            item.showNewModal.connect(function() {
                                window.currentViewIndex = 18
                            })
                        }
                    }

                    if (window.currentViewIndex === 16 && item && typeof item.voucherMode !== "undefined") {
                        item.voucherMode = window.targetChequeMode
                    }

                    if (window.currentViewIndex === 18 && item) {
                        if (typeof item.cancelRequested !== "undefined") {
                            item.cancelRequested.connect(function() {
                                window.currentViewIndex = 2
                            })
                        }
                        if (typeof item.voucherSaved !== "undefined") {
                            item.voucherSaved.connect(function() {
                                window.currentViewIndex = 2
                            })
                        }
                    }

                    if (window.currentViewIndex === 19 && item) {
                        if (typeof item.cancelRequested !== "undefined") {
                            item.cancelRequested.connect(function() {
                                window.currentViewIndex = 0
                            })
                        }
                        if (typeof item.openNewMillingRequested !== "undefined") {
                            item.openNewMillingRequested.connect(function() {
                                window.currentViewIndex = 18
                            })
                        }
                    }

                    if (window.currentViewIndex === 0 && item) {
                        if (typeof item.selectedMenuIndex !== "undefined") {
                            item.selectedMenuIndex = window.lastDashboardMenuIndex
                            item.selectedMenuIndexChanged.connect(function() {
                                window.lastDashboardMenuIndex = item.selectedMenuIndex
                            })
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

                    if (item.openAddVoucherMenu) item.openAddVoucherMenu.connect(function() { window.openAddVoucherMenu() })
                    if (item.openOtherVoucherMenu) item.openOtherVoucherMenu.connect(function() { window.openOtherVoucherMenu() })
                    if (item.openReportsMenu) item.openReportsMenu.connect(function() { window.openReportsMenu() })
                    if (item.openLedgers) item.openLedgers.connect(function() { window.openLedgerMasterMenu() })
                    if (item.openStock) item.openStock.connect(function() { window.openStockMasterMenu() })
                    if (item.openPaddy) item.openPaddy.connect(function() { window.currentViewIndex = 1 })
                    if (item.openLedgerMenu) item.openLedgerMenu.connect(function() { window.openLedgerMasterMenu() })
                    if (item.openStockMenu) item.openStockMenu.connect(function() { window.openStockMasterMenu() })
                    if (item.openPeriodModal) item.openPeriodModal.connect(function() { window.isPeriodModalOpen = true })
                    if (item.cancelRequested) item.cancelRequested.connect(function() { window.currentViewIndex = 0 })
                    if (item.savedSuccess) item.savedSuccess.connect(function() { window.currentViewIndex = 0 })
                    if (item.invoiceSaved) item.invoiceSaved.connect(function() { window.currentViewIndex = 0 })
                    if (item.openItemMovement) item.openItemMovement.connect(function(itemName) {
                        itemMovementModal.loadItemMovements(itemName)
                        window.isItemMovementModalOpen = true
                    })
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
                window.currentViewIndex = 6
            } else if (act === "Modify Ledger") {
                window.currentViewIndex = 7
            } else if (act === "View Ledger") {
                window.currentViewIndex = 8
            } else if (act === "New Group") {
                window.currentViewIndex = 9
            } else if (act === "Modify Group") {
                window.currentViewIndex = 10
            } else {
                window.currentViewIndex = 5
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
                window.currentViewIndex = 11
            } else if (act === "Modify Stock Item") {
                window.currentViewIndex = 12
            } else if (act === "Stock Details") {
                window.currentViewIndex = 13
            } else {
                window.currentViewIndex = 2
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
                window.currentViewIndex = 14
            } else if (opt === 2) {
                window.currentViewIndex = 15
            } else if (opt === 3) {
                window.targetChequeMode = "Payment"
                window.currentViewIndex = 16
                if (mainLoader.item && typeof mainLoader.item.voucherMode !== "undefined") mainLoader.item.voucherMode = "Payment"
            } else if (opt === 4) {
                window.targetChequeMode = "Receipt"
                window.currentViewIndex = 16
                if (mainLoader.item && typeof mainLoader.item.voucherMode !== "undefined") mainLoader.item.voucherMode = "Receipt"
            } else if (opt === 5) {
                window.currentViewIndex = 17
            } else if (opt === 6) {
                window.currentViewIndex = 18
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
                jformStubModal.open()
            } else if (opt === 2) {
                window.currentViewIndex = 17
            } else if (opt === 3) {
                window.currentViewIndex = 18
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
                window.currentViewIndex = 19
            } else if (act === "Stock Register") {
                window.currentViewIndex = 13
            } else if (act === "Item Movement") {
                window.currentViewIndex = 13
            } else if (act === "Ledger Statement") {
                window.currentViewIndex = 8
            } else if (act === "Sales Register") {
                window.currentViewIndex = 20
            } else if (act === "Purchase Register") {
                window.currentViewIndex = 21
            }
        }
    }

    JFormStubModal {
        id: jformStubModal
        anchors.centerIn: parent
    }

    // Modal Overlays Container for custom full dialogs
    Rectangle {
        anchors.fill: parent
        color: "#66000000"
        visible: window.isShortcutsModalOpen || window.isPaddyModalOpen || window.isItemMovementModalOpen || window.isPeriodModalOpen

        MouseArea {
            anchors.fill: parent
            onClicked: {
                window.isShortcutsModalOpen = false
                window.isPaddyModalOpen = false
                window.isItemMovementModalOpen = false
                window.isPeriodModalOpen = false
            }
        }

        // Accounting Period / Financial Year Modal (Bahi-Khata Style)
        AccountingPeriodModal {
            id: accountingPeriodModal
            anchors.centerIn: parent
            visible: window.isPeriodModalOpen
            onCloseRequested: window.isPeriodModalOpen = false
            onPeriodSelected: function(fromIso, toIso, fyLabel) {
                window.activePeriodLabel = "(" + fromIso + " To " + toIso + ") " + fyLabel
                if (typeof dashboardCtrl !== "undefined" && dashboardCtrl) {
                    dashboardCtrl.refresh_stats(fromIso, toIso, fyLabel)
                }
                if (mainLoader.item && typeof mainLoader.item.activePeriodText !== "undefined") {
                    mainLoader.item.activePeriodText = fromIso + " To " + toIso + " (" + fyLabel + ")"
                }
                if (mainLoader.item && typeof mainLoader.item.loadDashboardStats !== "undefined") {
                    mainLoader.item.loadDashboardStats()
                }
                if (mainLoader.item && typeof mainLoader.item.loadStockItems !== "undefined") {
                    mainLoader.item.loadStockItems()
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
        }
    }
}
