#include "menu_tree_manager.h"
#include "../widgets/navigation_menu_dialog.h"
#include <QDialog>

namespace MahadevERP {

MenuTreeManager& MenuTreeManager::instance()
{
    static MenuTreeManager s_instance;
    return s_instance;
}

MenuTreeManager::MenuTreeManager(QObject* parent)
    : QObject(parent)
{
    setupDefaultMenus();
}

void MenuTreeManager::registerMenu(const MenuNode& node)
{
    m_menus[node.id] = node;
}

MenuNode* MenuTreeManager::getMenu(const QString& id)
{
    auto it = m_menus.find(id);
    return (it != m_menus.end()) ? &(*it) : nullptr;
}

const MenuNode* MenuTreeManager::getMenu(const QString& id) const
{
    auto it = m_menus.find(id);
    return (it != m_menus.end()) ? &(*it) : nullptr;
}

bool MenuTreeManager::hasMenu(const QString& id) const
{
    return m_menus.contains(id);
}

void MenuTreeManager::setItemCallback(const QString& menuId, int itemIndex, std::function<void()> callback)
{
    if (m_menus.contains(menuId) && itemIndex >= 0 && itemIndex < m_menus[menuId].items.size()) {
        m_menus[menuId].items[itemIndex].callback = callback;
    }
}

void MenuTreeManager::setItemTargetView(const QString& menuId, int itemIndex, int viewIndex)
{
    if (m_menus.contains(menuId) && itemIndex >= 0 && itemIndex < m_menus[menuId].items.size()) {
        m_menus[menuId].items[itemIndex].targetViewIndex = viewIndex;
    }
}

int MenuTreeManager::lastSelectedIndex(const QString& menuId) const
{
    if (m_menus.contains(menuId)) {
        return m_menus[menuId].lastSelectedIndex;
    }
    return 0;
}

void MenuTreeManager::setLastSelectedIndex(const QString& menuId, int idx)
{
    if (m_menus.contains(menuId)) {
        m_menus[menuId].lastSelectedIndex = idx;
    }
}

void MenuTreeManager::executeMenu(const QString& startMenuId, QWidget* parent, int initialIndex)
{
    if (!m_menus.contains(startMenuId)) return;

    if (initialIndex >= 0) {
        m_menus[startMenuId].lastSelectedIndex = initialIndex;
    }

    // Build the full ancestor stack from root down to startMenuId
    m_activeStack.clear();
    QString curr = startMenuId;
    QVector<QString> chain;
    while (!curr.isEmpty() && m_menus.contains(curr)) {
        chain.prepend(curr);
        curr = m_menus[curr].parentId;
    }
    m_activeStack = chain;

    resumeMenuStack(parent);
}

void MenuTreeManager::resumeMenuStack(QWidget* parent)
{
    while (!m_activeStack.isEmpty()) {
        QString currentId = m_activeStack.last();
        if (!m_menus.contains(currentId)) {
            m_activeStack.removeLast();
            continue;
        }

        MenuNode& node = m_menus[currentId];
        NavigationMenuDialog dlg(node, parent);

        int res = dlg.exec();
        if (res == QDialog::Accepted) {
            int selectedIdx = dlg.selectedIndex();
            node.lastSelectedIndex = selectedIdx;

            if (selectedIdx >= 0 && selectedIdx < node.items.size()) {
                const MenuItem& item = node.items[selectedIdx];
                if (item.actionType == MenuActionType::Back) {
                    m_activeStack.removeLast();
                } else if (item.actionType == MenuActionType::OpenSubmenu) {
                    if (!item.targetSubmenuId.isEmpty() && m_menus.contains(item.targetSubmenuId)) {
                        m_activeStack.append(item.targetSubmenuId);
                    }
                } else if (item.actionType == MenuActionType::OpenView) {
                    if (item.callback) {
                        item.callback();
                    }
                    m_lastTriggeredMenuId = currentId;
                    m_lastTriggeredSubmenuIndex = selectedIdx;
                    int targetView = item.targetViewIndex;
                    emit openViewRequested(targetView);
                    return;
                } else if (item.actionType == MenuActionType::ExecuteCustom) {
                    if (item.callback) {
                        item.callback();
                    }
                }
            }
        } else {
            // Escape pressed or clicked outside: pop current menu so user returns to parent menu
            m_activeStack.removeLast();
        }
    }
}

void MenuTreeManager::setupDefaultMenus()
{
    // 1. LEDGER MASTER MENU
    MenuNode ledgerMenu("ledger_master", "LEDGER MASTER MENU", "#2563EB", "");
    ledgerMenu.items = {
        MenuItem("1. New Ledger Entry", "F3 / Alt+N", Qt::Key_1, MenuActionType::OpenView, 6),
        MenuItem("2. Modify Existing Ledger", "Enter", Qt::Key_2, MenuActionType::OpenView, 7),
        MenuItem("3. View Ledger Statement (Account Bahi-Khata)", "F4 / Alt+L", Qt::Key_3, MenuActionType::OpenView, 8),
        MenuItem("4. New Account Group", "", Qt::Key_4, MenuActionType::OpenView, 9),
        MenuItem("5. Modify Account Group", "", Qt::Key_5, MenuActionType::OpenView, 10),
        MenuItem("6. All Ledgers Directory / Master List", "Alt+6", Qt::Key_6, MenuActionType::OpenView, 5)
    };
    registerMenu(ledgerMenu);

    // 2. STOCK MASTER MENU
    MenuNode stockMenu("stock_master", "STOCK MASTER MENU", "#16A34A", "");
    stockMenu.items = {
        MenuItem("1. New Stock Item Master", "Alt+I", Qt::Key_1, MenuActionType::OpenView, 11),
        MenuItem("2. Modify Stock Item Master", "Enter", Qt::Key_2, MenuActionType::OpenView, 12),
        MenuItem("3. Stock Register (View Options)", "Alt+4 / S", Qt::Key_3, MenuActionType::OpenSubmenu, -1, "stock_register_hub"),
        MenuItem("4. Closing Stock Valuation & Year-End Audit", "Alt+C", Qt::Key_4, MenuActionType::OpenView, 36),
        MenuItem("5. Raw Paddy Stock Register", "", Qt::Key_5, MenuActionType::OpenView, 13),
        MenuItem("6. Finished Rice Stock Register", "", Qt::Key_6, MenuActionType::OpenView, 13),
        MenuItem("7. By-Products & Husk Stock Register", "", Qt::Key_7, MenuActionType::OpenView, 13)
    };
    registerMenu(stockMenu);

    // 3. ADD VOUCHER MENU
    MenuNode addVoucherMenu("add_voucher", "ADD VOUCHER MENU", "#2563EB", "");
    addVoucherMenu.items = {
        MenuItem("1. Sales Voucher Entry (Tax Invoice)", "F8", Qt::Key_1, MenuActionType::OpenView, 14),
        MenuItem("2. Purchase Voucher Entry (Purchase Bill)", "F9", Qt::Key_2, MenuActionType::OpenView, 15),
        MenuItem("3. Paddy Procurement Slip (Kachha / Mandi)", "F2", Qt::Key_3, MenuActionType::OpenView, 1),
        MenuItem("4. Milling Production Entry", "", Qt::Key_4, MenuActionType::OpenView, 31),
        MenuItem("5. Cheque / Bank Payment & Receipt", "F3", Qt::Key_5, MenuActionType::OpenView, 16),
        MenuItem("6. Cash Payment & Receipt Voucher", "F6", Qt::Key_6, MenuActionType::OpenView, 60),
        MenuItem("7. Journal Voucher Entry", "F5", Qt::Key_7, MenuActionType::OpenView, 17)
    };
    registerMenu(addVoucherMenu);

    // 4. OTHER VOUCHERS MENU
    MenuNode otherVoucherMenu("other_voucher", "OTHER VOUCHERS MENU", "#7C3AED", "");
    otherVoucherMenu.items = {
        MenuItem("1. J-Form Mandi Procurement Voucher", "F11", Qt::Key_1, MenuActionType::OpenView, 18),
        MenuItem("2. I-Form Mandi Buyer Issue Voucher", "Alt+I", Qt::Key_2, MenuActionType::OpenView, 19),
        MenuItem("3. Mandi Form M & Statutory Returns (HSAMB)", "Alt+M", Qt::Key_3, MenuActionType::OpenView, 20),
        MenuItem("4. TDS / TCS Vouchers & Challans", "F12", Qt::Key_4, MenuActionType::OpenSubmenu, -1, "tds_tcs_hub"),
        MenuItem("5. Bank Statement Auto-Import & Reconciliation", "Ctrl+B", Qt::Key_5, MenuActionType::OpenView, 26),
        MenuItem("6. Transport Dispatch & Gate Pass Register", "Alt+T", Qt::Key_6, MenuActionType::OpenView, 27),
        MenuItem("7. GST Debit Notes & Credit Notes", "Alt+D", Qt::Key_7, MenuActionType::OpenView, 28),
        MenuItem("8. GSTR-2A Matching & ITC Reconciliation", "Alt+G", Qt::Key_8, MenuActionType::OpenView, 34)
    };
    registerMenu(otherVoucherMenu);

    // 5. REPORTS & REGISTERS MENU
    MenuNode reportsMenu("reports_register", "REPORTS & REGISTERS MENU", "#059669", "");
    reportsMenu.items = {
        MenuItem("1. Day Book (Daily Audit & Transaction Register)", "Alt+D", Qt::Key_1, MenuActionType::OpenView, 33),
        MenuItem("2. Cash Book & Flow Statements (Cash, Bank, Joint)", "Alt+C / C", Qt::Key_2, MenuActionType::OpenSubmenu, -1, "cash_book_hub"),
        MenuItem("3. GST Compliance Dashboard (GSTR-1, 2A Match, 3B)", "Alt+G", Qt::Key_3, MenuActionType::OpenView, 34),
        MenuItem("4. Mandi Form M & Statutory Returns (HSAMB)", "Alt+M", Qt::Key_4, MenuActionType::OpenView, 20),
        MenuItem("5. Sales Register & Summary", "", Qt::Key_5, MenuActionType::OpenView, 3),
        MenuItem("6. Purchase Register & Summary", "", Qt::Key_6, MenuActionType::OpenView, 4),
        MenuItem("7. Stock Register (View Options)", "S", Qt::Key_7, MenuActionType::OpenSubmenu, -1, "stock_register_hub"),
        MenuItem("8. Milling Production & Out-turn Statement", "Alt+M", Qt::Key_8, MenuActionType::OpenView, 35),
        MenuItem("9. Final Reports (Balance Sheet, P&L, Trading, Trial Bal)", "F / F7", Qt::Key_9, MenuActionType::OpenSubmenu, -1, "final_reports_hub"),
        MenuItem("10. Profit & Loss Statement (Trading & P&L)", "F6", 0, MenuActionType::OpenView, 30),
        MenuItem("11. Closing Stock Valuation & Year-End Audit", "Alt+C", 0, MenuActionType::OpenView, 36),
        MenuItem("12. Interest Calculation & Register (Aank / Rokka)", "Alt+A", 0, MenuActionType::OpenView, 8)
    };
    registerMenu(reportsMenu);

    // 5b. CASH BOOK & FLOW STATEMENTS HUB
    MenuNode cashBookHub("cash_book_hub", "CASH BOOK & FLOW STATEMENTS", "#2563EB", "reports_register");
    cashBookHub.items = {
        MenuItem("[A] Cash Flow Statement", "A", Qt::Key_A, MenuActionType::OpenView, 61),
        MenuItem("[N] Bank Flow Statement", "N", Qt::Key_N, MenuActionType::OpenView, 62),
        MenuItem("[J] Cash & Bank Joint Flow", "J", Qt::Key_J, MenuActionType::OpenView, 63),
        MenuItem("[D] Day-Wise Cash Book", "D", Qt::Key_D, MenuActionType::OpenView, 33),
        MenuItem("[Q] Quit / Back", "Esc / Q", Qt::Key_Q, MenuActionType::Back)
    };
    registerMenu(cashBookHub);

    // 6. TDS / TCS HUB MENU
    MenuNode tdsTcsHub("tds_tcs_hub", "TDS / TCS VOUCHER MENU", "#7C3AED", "other_voucher");
    tdsTcsHub.items = {
        MenuItem("[D] T.D.S. Voucher Entry", "D", Qt::Key_D, MenuActionType::OpenSubmenu, -1, "tds_options"),
        MenuItem("[C] T.C.S. Voucher Entry", "C", Qt::Key_C, MenuActionType::OpenSubmenu, -1, "tcs_options"),
        MenuItem("[Q] Quit", "Q", Qt::Key_Q, MenuActionType::Back)
    };
    registerMenu(tdsTcsHub);

    // 7. TDS OPTIONS MENU
    MenuNode tdsOptions("tds_options", "T.D.S. OPTIONS MENU", "#2563EB", "tds_tcs_hub");
    tdsOptions.items = {
        MenuItem("[N] New TDS Voucher", "N", Qt::Key_N, MenuActionType::OpenView, 24),
        MenuItem("[L] TDS Vouchers List", "L", Qt::Key_L, MenuActionType::OpenView, 50),
        MenuItem("[T] TDS Deposit Challans", "T", Qt::Key_T, MenuActionType::OpenView, 52),
        MenuItem("[A] Acknowledgement Nos.", "A", Qt::Key_A, MenuActionType::ExecuteCustom),
        MenuItem("[P] Advance Pymt Vch. U/S 194-Q", "P", Qt::Key_P, MenuActionType::OpenView, 55),
        MenuItem("[V] Advance Pymt Vouchers List", "V", Qt::Key_V, MenuActionType::OpenView, 56),
        MenuItem("[R] Receive Form-16A / 27D", "R", Qt::Key_R, MenuActionType::ExecuteCustom),
        MenuItem("[F] Received Forms-16A List", "F", Qt::Key_F, MenuActionType::OpenView, 57),
        MenuItem("[Q] Quit", "Q", Qt::Key_Q, MenuActionType::Back)
    };
    registerMenu(tdsOptions);

    // 8. TCS OPTIONS MENU
    MenuNode tcsOptions("tcs_options", "T.C.S. OPTIONS MENU", "#16A34A", "tds_tcs_hub");
    tcsOptions.items = {
        MenuItem("[R] Receipt Voucher For TCS", "R", Qt::Key_R, MenuActionType::OpenView, 53),
        MenuItem("[L] Receipt Vouchers List", "L", Qt::Key_L, MenuActionType::OpenView, 54),
        MenuItem("[T] TCS Deposit Challans", "T", Qt::Key_T, MenuActionType::OpenView, 52),
        MenuItem("[O] Others (TCS Settings)", "O", Qt::Key_O, MenuActionType::ExecuteCustom),
        MenuItem("[Q] Quit", "Q", Qt::Key_Q, MenuActionType::Back)
    };
    registerMenu(tcsOptions);

    // 9. STOCK REGISTER HUB MENU
    MenuNode stockHub("stock_register_hub", "STOCK REGISTER OPTIONS", "#059669", "reports_register");
    stockHub.items = {
        MenuItem("1. Show Only Stock (Quantities)", "1 / O", Qt::Key_1, MenuActionType::OpenSubmenu, -1, "stock_only_menu"),
        MenuItem("2. Show Stock With Amount (Valuation)", "2 / A", Qt::Key_2, MenuActionType::OpenSubmenu, -1, "stock_amount_menu"),
        MenuItem("3. Show Item Wise Profit & Loss", "3 / P", Qt::Key_3, MenuActionType::OpenSubmenu, -1, "stock_profit_menu"),
        MenuItem("4. Item Monthly / Daily Stock", "4 / D", Qt::Key_4, MenuActionType::OpenView, 13),
        MenuItem("[Q] Quit / Back", "Esc / Q", Qt::Key_Q, MenuActionType::Back)
    };
    registerMenu(stockHub);

    // 10. SHOW ONLY STOCK MENU
    MenuNode stockOnly("stock_only_menu", "SHOW ONLY STOCK", "#2563EB", "stock_register_hub");
    stockOnly.items = {
        MenuItem("[I] Item Stock", "I", Qt::Key_I, MenuActionType::OpenView, 13),
        MenuItem("[G] Group Stock", "G", Qt::Key_G, MenuActionType::OpenView, 13),
        MenuItem("[C] Company Stock", "C", Qt::Key_C, MenuActionType::OpenView, 13),
        MenuItem("[T] Total Stock", "T", Qt::Key_T, MenuActionType::OpenView, 13),
        MenuItem("[H] HSN Wise Total Stock", "H", Qt::Key_H, MenuActionType::OpenView, 13),
        MenuItem("[Q] Quit / Back", "Esc / Q", Qt::Key_Q, MenuActionType::Back)
    };
    registerMenu(stockOnly);

    // 11. SHOW STOCK WITH AMOUNT MENU
    MenuNode stockAmount("stock_amount_menu", "SHOW STOCK WITH AMOUNT", "#D97706", "stock_register_hub");
    stockAmount.items = {
        MenuItem("[S] Item Stock (With Amount)", "S", Qt::Key_S, MenuActionType::OpenView, 13),
        MenuItem("[R] Group Stock (With Amount)", "R", Qt::Key_R, MenuActionType::OpenView, 13),
        MenuItem("[M] Company Stock (With Amount)", "M", Qt::Key_M, MenuActionType::OpenView, 13),
        MenuItem("[O] Total Stock (With Amount)", "O", Qt::Key_O, MenuActionType::OpenView, 13),
        MenuItem("[W] HSN Wise Total Stock (With Amount)", "W", Qt::Key_W, MenuActionType::OpenView, 13),
        MenuItem("[Q] Quit / Back", "Esc / Q", Qt::Key_Q, MenuActionType::Back)
    };
    registerMenu(stockAmount);

    // 12. ITEM WISE PROFIT & LOSS MENU
    MenuNode stockProfit("stock_profit_menu", "ITEM WISE PROFIT & LOSS", "#7C3AED", "stock_register_hub");
    stockProfit.items = {
        MenuItem("[E] Item Details (P&L)", "E", Qt::Key_E, MenuActionType::OpenView, 13),
        MenuItem("[U] Group Details (P&L)", "U", Qt::Key_U, MenuActionType::OpenView, 13),
        MenuItem("[P] Company Details (P&L)", "P", Qt::Key_P, MenuActionType::OpenView, 13),
        MenuItem("[L] Total Items Details (P&L)", "L", Qt::Key_L, MenuActionType::OpenView, 13),
        MenuItem("[N] HSN Wise Total Details (P&L)", "N", Qt::Key_N, MenuActionType::OpenView, 13),
        MenuItem("[Q] Quit / Back", "Esc / Q", Qt::Key_Q, MenuActionType::Back)
    };
    registerMenu(stockProfit);

    // 13. FINAL REPORTS HUB MENU
    MenuNode finalReportsHub("final_reports_hub", "FINAL REPORTS (ALT+F2: SET PERIOD)", "#059669", "reports_register");
    finalReportsHub.items = {
        MenuItem("[B] Balance Sheet", "B", Qt::Key_B, MenuActionType::OpenView, 29),
        MenuItem("[C] Capital A/cs", "C", Qt::Key_C, MenuActionType::OpenView, 40),
        MenuItem("[P] Profit & Loss", "P", Qt::Key_P, MenuActionType::OpenView, 30),
        MenuItem("[T] Trading A/cs", "T", Qt::Key_T, MenuActionType::OpenSubmenu, -1, "trading_reports_hub"),
        MenuItem("[O] Auto Closing Stock Options", "O", Qt::Key_O, MenuActionType::OpenSubmenu, -1, "auto_closing_stock_hub"),
        MenuItem("[J] Print Joint Reports", "J", Qt::Key_J, MenuActionType::ExecuteCustom),
        MenuItem("[D] Done/Undone B/Sheet", "D", Qt::Key_D, MenuActionType::OpenSubmenu, -1, "done_undone_bsheet_hub"),
        MenuItem("[A] Trial Balance Sheet", "A", Qt::Key_A, MenuActionType::OpenSubmenu, -1, "trial_balance_options_hub"),
        MenuItem("[E] Depreciation Chart", "E", Qt::Key_E, MenuActionType::OpenSubmenu, -1, "depreciation_options_hub"),
        MenuItem("[Q] Quit / Back", "Esc / Q", Qt::Key_Q, MenuActionType::Back)
    };
    registerMenu(finalReportsHub);

    // 14. TRADING REPORTS HUB
    MenuNode tradingHub("trading_reports_hub", "REPORT OPTIONS...", "#2563EB", "final_reports_hub");
    tradingHub.items = {
        MenuItem("[I] Item Wise", "I", Qt::Key_I, MenuActionType::OpenView, 13),
        MenuItem("[J] Joint Report (Without Qty.)", "J", Qt::Key_J, MenuActionType::OpenView, 30),
        MenuItem("[P] Joint Report (With Qty.)", "P", Qt::Key_P, MenuActionType::OpenView, 30),
        MenuItem("[Y] Production Yield Chart", "Y", Qt::Key_Y, MenuActionType::OpenView, 35),
        MenuItem("[Q] Quit / Back", "Esc / Q", Qt::Key_Q, MenuActionType::Back)
    };
    registerMenu(tradingHub);

    // 15. AUTO CLOSING STOCK OPTIONS HUB
    MenuNode autoClosingHub("auto_closing_stock_hub", "CLOSING STOCK OPTIONS...", "#D97706", "final_reports_hub");
    autoClosingHub.items = {
        MenuItem("[A] Auto Fill Closing Stock", "A", Qt::Key_A, MenuActionType::ExecuteCustom),
        MenuItem("[R] Remove Filled Closing Stock", "R", Qt::Key_R, MenuActionType::ExecuteCustom),
        MenuItem("[F] Resave Filled Closing Stock", "F", Qt::Key_F, MenuActionType::ExecuteCustom),
        MenuItem("[Q] Quit / Back", "Esc / Q", Qt::Key_Q, MenuActionType::Back)
    };
    registerMenu(autoClosingHub);

    // 16. DONE / UNDONE B/SHEET HUB
    MenuNode doneUndoneHub("done_undone_bsheet_hub", "B/SHEET OPTIONS...", "#7C3AED", "final_reports_hub");
    doneUndoneHub.items = {
        MenuItem("[D] Done B/Sheet (Lock Audit)", "D", Qt::Key_D, MenuActionType::ExecuteCustom),
        MenuItem("[U] UnDone B/Sheet (Unlock Draft)", "U", Qt::Key_U, MenuActionType::ExecuteCustom),
        MenuItem("[Q] Quit / Back", "Esc / Q", Qt::Key_Q, MenuActionType::Back)
    };
    registerMenu(doneUndoneHub);

    // 17. TRIAL BALANCE VIEW OPTIONS HUB
    MenuNode tbOptionsHub("trial_balance_options_hub", "VIEW OPTIONS", "#0284C7", "final_reports_hub");
    tbOptionsHub.items = {
        MenuItem("[N] Normal View", "N", Qt::Key_N, MenuActionType::OpenView, 39),
        MenuItem("[F] Flat View", "F", Qt::Key_F, MenuActionType::OpenView, 39),
        MenuItem("[G] Flat Grouped", "G", Qt::Key_G, MenuActionType::OpenView, 39),
        MenuItem("[O] Normal Detailed", "O", Qt::Key_O, MenuActionType::OpenView, 39),
        MenuItem("[W] Without Op.Bal.", "W", Qt::Key_W, MenuActionType::OpenView, 39),
        MenuItem("[T] Show Turnover", "T", Qt::Key_T, MenuActionType::OpenView, 39),
        MenuItem("[B] Show Opening Bal.", "B", Qt::Key_B, MenuActionType::OpenView, 39),
        MenuItem("[Q] Quit / Back", "Esc / Q", Qt::Key_Q, MenuActionType::Back)
    };
    registerMenu(tbOptionsHub);

    // 18. DEPRECIATION OPTIONS HUB
    MenuNode depOptionsHub("depreciation_options_hub", "DEPRECIATION OPTIONS", "#059669", "final_reports_hub");
    depOptionsHub.items = {
        MenuItem("1. Summarized Report", "1", Qt::Key_1, MenuActionType::OpenView, 41),
        MenuItem("2. Detailed Report", "2", Qt::Key_2, MenuActionType::OpenView, 41),
        MenuItem("[Q] Quit / Back", "Esc / Q", Qt::Key_Q, MenuActionType::Back)
    };
    registerMenu(depOptionsHub);
}

} // namespace MahadevERP
