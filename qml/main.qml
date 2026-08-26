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
    // 0=Dashboard, 1=Paddy, 2=Milling/Stock, 3=Sales, 4=Vouchers, 5=Reports/LedgerList, 6=NewLedgerPage, 7=ModifyLedgerPage, 8=ViewStatementPage, 9=NewGroup, 10=ModifyGroup, 11=NewStockItem, 12=ModifyStockItem, 13=StockDetail, 14=SalesVoucher
    property int currentViewIndex: 0
    property bool isShortcutsModalOpen: false
    property bool isPaddyModalOpen: false
    property bool isLedgerMenuOpen: false
    property bool isStockMenuOpen: false
    property bool isItemMovementModalOpen: false
    property bool isAddVoucherMenuOpen: false

    // GLOBAL KEYBOARD SHORTCUTS WITH APPLICATION SCOPE
    Shortcut { sequence: "Alt+1"; context: Qt.ApplicationShortcut; onActivated: window.currentViewIndex = 0 }
    Shortcut { sequence: "Alt+2"; context: Qt.ApplicationShortcut; onActivated: window.currentViewIndex = 1 }
    Shortcut { sequence: "Alt+3"; context: Qt.ApplicationShortcut; onActivated: window.currentViewIndex = 14 } // Sales Voucher
    Shortcut { sequence: "Alt+4"; context: Qt.ApplicationShortcut; onActivated: window.currentViewIndex = 2 } // Stock/Milling
    Shortcut { sequence: "Alt+5"; context: Qt.ApplicationShortcut; onActivated: window.currentViewIndex = 4 } // Vouchers
    Shortcut { sequence: "Alt+6"; context: Qt.ApplicationShortcut; onActivated: window.currentViewIndex = 5 } // Ledger List / Reports
    Shortcut { sequence: "F1"; context: Qt.ApplicationShortcut; onActivated: window.isShortcutsModalOpen = !window.isShortcutsModalOpen }
    Shortcut { sequence: "F2"; context: Qt.ApplicationShortcut; onActivated: window.isAddVoucherMenuOpen = true }

    // UNIVERSAL ESCAPE KEY - RETURN TO DASHBOARD OR CLOSE OPEN MODALS
    Shortcut {
        sequence: "Esc"
        context: Qt.ApplicationShortcut
        onActivated: {
            if (window.isShortcutsModalOpen || window.isPaddyModalOpen || window.isLedgerMenuOpen || window.isStockMenuOpen || window.isItemMovementModalOpen || window.isAddVoucherMenuOpen) {
                window.isShortcutsModalOpen = false
                window.isPaddyModalOpen = false
                window.isLedgerMenuOpen = false
                window.isStockMenuOpen = false
                window.isItemMovementModalOpen = false
                window.isAddVoucherMenuOpen = false
            } else if (window.currentViewIndex !== 0) {
                window.currentViewIndex = 0 // Go back to Dashboard
            }
        }
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
                        default: return "views/DashboardView.qml"
                    }
                }

                onLoaded: {
                    if (item.openNewPaddy) item.openNewPaddy.connect(function() { window.isPaddyModalOpen = true })
                    if (item.showNewModal) item.showNewModal.connect(function() { window.isPaddyModalOpen = true })
                    if (item.openNewVoucher) item.openNewVoucher.connect(function() { window.isAddVoucherMenuOpen = true })
                    if (item.openNewSale) item.openNewSale.connect(function() { window.currentViewIndex = 14 })
                    if (item.openAddVoucherMenu) item.openAddVoucherMenu.connect(function() { window.isAddVoucherMenuOpen = true })
                    if (item.openLedgers) item.openLedgers.connect(function() { window.isLedgerMenuOpen = true })
                    if (item.openStock) item.openStock.connect(function() { window.isStockMenuOpen = true })
                    if (item.openPaddy) item.openPaddy.connect(function() { window.currentViewIndex = 1 })
                    if (item.openLedgerMenu) item.openLedgerMenu.connect(function() { window.isLedgerMenuOpen = true })
                    if (item.openStockMenu) item.openStockMenu.connect(function() { window.isStockMenuOpen = true })
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

    // Modal Overlays Container
    Rectangle {
        anchors.fill: parent
        color: "#66000000"
        visible: window.isShortcutsModalOpen || window.isPaddyModalOpen || window.isLedgerMenuOpen || window.isStockMenuOpen || window.isItemMovementModalOpen || window.isAddVoucherMenuOpen

        MouseArea {
            anchors.fill: parent
            onClicked: {
                window.isShortcutsModalOpen = false
                window.isPaddyModalOpen = false
                window.isLedgerMenuOpen = false
                window.isStockMenuOpen = false
                window.isItemMovementModalOpen = false
                window.isAddVoucherMenuOpen = false
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

        // LEDGER MASTER POPUP SUBMENU MODAL
        LedgerMasterMenuModal {
            anchors.centerIn: parent
            visible: window.isLedgerMenuOpen
            onCloseRequested: window.isLedgerMenuOpen = false
            onActionSelected: function(act) {
                window.isLedgerMenuOpen = false
                if (act === "New Ledger") {
                    window.currentViewIndex = 6 // Open Full-Screen New Ledger Page
                } else if (act === "Modify Ledger") {
                    window.currentViewIndex = 7 // Open Full-Screen Modify Ledger Page
                } else if (act === "View Ledger") {
                    window.currentViewIndex = 8 // Open Full-Screen Ledger Statement Page
                } else if (act === "New Group") {
                    window.currentViewIndex = 9 // Open Full-Screen New Group Page
                } else if (act === "Modify Group") {
                    window.currentViewIndex = 10 // Open Full-Screen Modify Group Page
                } else {
                    window.currentViewIndex = 5
                }
            }
        }

        // STOCK MASTER POPUP SUBMENU MODAL
        StockMasterMenuModal {
            anchors.centerIn: parent
            visible: window.isStockMenuOpen
            onCloseRequested: window.isStockMenuOpen = false
            onActionSelected: function(act) {
                window.isStockMenuOpen = false
                if (act === "New Stock Item") {
                    window.currentViewIndex = 11 // Open Full-Screen New Stock Item Page
                } else if (act === "Modify Stock Item") {
                    window.currentViewIndex = 12 // Open Full-Screen Modify Stock Item Page
                } else if (act === "Stock Details") {
                    window.currentViewIndex = 13 // Open Full-Screen Stock Details & Register Page
                } else {
                    window.currentViewIndex = 2 // Go to Stock & Milling Process Page
                }
            }
        }

        // ADD VOUCHER SUBMENU MODAL
        AddVoucherMenuModal {
            anchors.centerIn: parent
            visible: window.isAddVoucherMenuOpen
            onCloseRequested: window.isAddVoucherMenuOpen = false
            onOptionSelected: function(opt) {
                window.isAddVoucherMenuOpen = false
                if (opt === 1) {
                    window.currentViewIndex = 14 // SalesVoucherView.qml
                } else if (opt === 2) {
                    window.isPaddyModalOpen = true // Paddy procurement
                } else if (opt === 3) {
                    window.currentViewIndex = 4 // Voucher Ledger
                } else if (opt === 4) {
                    window.currentViewIndex = 2 // Milling Process
                }
            }
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
