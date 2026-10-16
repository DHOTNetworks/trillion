#include "menu_tree_manager.h"
#include "../config/feature_flags.h"
#include "../config/settings_registry.h"
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
    applyFeatureFlags();
}

void MenuTreeManager::applyFeatureFlags() {
    // Per-item: OpenView into a flag-disabled view becomes unreachable.
    // Canonical positional indices are preserved here (the dialog gets a
    // filtered copy in resumeMenuStack), so all setItemCallback wirings
    // keep landing on the right rows while hidden options never appear.
    for (auto& node : m_menus) {
        for (auto& item : node.items) {
            if (item.actionType == MenuActionType::OpenView && item.targetViewIndex >= 0) {
                item.enabled = FeatureFlags::isViewEnabled(item.targetViewIndex);
            }
        }
    }
    // The mandi hub entry stays only while the hub keeps an actionable row.
    bool hubLive = false;
    auto hubIt = m_menus.find("mandi_trading_hub");
    if (hubIt != m_menus.end()) {
        for (const auto& item : hubIt->items) {
            if (item.actionType != MenuActionType::Back && item.enabled) {
                hubLive = true;
                break;
            }
        }
    }
    if (!hubLive) {
        for (auto& node : m_menus) {
            for (auto& item : node.items) {
                if (item.actionType == MenuActionType::OpenSubmenu
                    && item.targetSubmenuId == "mandi_trading_hub") {
                    item.enabled = false;
                }
            }
        }
    }
    // Mandi hub single-home (§23.4): the hub entry lives in `mandi` iff the
    // mandi flag is on, else in `other_voucher` iff the hub is live —
    // exactly one visible instance, canonical indices stable either way.
    // (Runs after the hubLive block above so it always wins.)
    const bool mandiOn = FeatureFlags::isOn(SettingsKeys::feature_mandi());
    auto setHubEntry = [&](const QString& menuId, bool enabled) {
        auto it = m_menus.find(menuId);
        if (it == m_menus.end()) return;
        for (auto& item : it->items) {
            if (item.actionType == MenuActionType::OpenSubmenu
                && item.targetSubmenuId == "mandi_trading_hub") {
                item.enabled = enabled;
            }
        }
    };
    if (mandiOn) {
        setHubEntry("mandi", true);
        setHubEntry("other_voucher", false);
    } else {
        setHubEntry("mandi", false);
        setHubEntry("other_voucher", hubLive);
    }
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

    // Flag-filtered: re-filter on every open so firm switches (and future
    // Settings toggles) apply immediately. Canonical indices never shift
    // (resumeMenuStack filters at display time), so lastSelectedIndex and
    // callback wirings stay valid while off-domain rows stay hidden.
    applyFeatureFlags();

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
        // Flag-filtered: show only enabled rows — off-domain options never
        // appear (no grey rows). canonIdx maps each visible row back to its
        // canonical index so callbacks, target views and lastSelectedIndex
        // keep their positional wiring.
        QVector<MenuItem> visible;
        QVector<int> canonIdx;
        visible.reserve(node.items.size());
        canonIdx.reserve(node.items.size());
        for (int i = 0; i < node.items.size(); ++i) {
            if (node.items[i].enabled) {
                visible.append(node.items[i]);
                canonIdx.append(i);
            }
        }
        if (visible.isEmpty()) {
            // Nothing actionable here (e.g. a domain hub with every row
            // off): pop and return to the parent menu instead of showing
            // an empty dialog.
            m_activeStack.removeLast();
            continue;
        }
        int initialFiltered = 0;
        for (int k = 0; k < canonIdx.size(); ++k) {
            if (canonIdx[k] == node.lastSelectedIndex) {
                initialFiltered = k;
                break;
            }
        }
        MenuNode shown(currentId, node.title, node.borderColor, node.parentId, visible);
        // NavigationMenuDialog(Node) starts at node.lastSelectedIndex, so
        // carry the mapped position through it.
        shown.lastSelectedIndex = initialFiltered;
        NavigationMenuDialog mappedDlg(shown, parent);

        int res = mappedDlg.exec();
        if (res == QDialog::Accepted) {
            int selectedFiltered = mappedDlg.selectedIndex();
            if (selectedFiltered < 0 || selectedFiltered >= canonIdx.size()) {
                m_activeStack.removeLast();
                continue;
            }
            int selectedIdx = canonIdx[selectedFiltered];
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
                    // Preserve m_activeStack so that when user presses Esc / Back from the target view,
                    // the menu stack can unwind back to this menu dialog.
                    QMetaObject::invokeMethod(this, [this, targetView]() {
                        emit openViewRequested(targetView);
                    }, Qt::QueuedConnection);
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
    // 1. LEDGER MASTER MENU (§23.6: no numeric prefixes; badge numbers and
    // visible-position digit keys keep working; one letter hotkey per row).
    MenuNode ledgerMenu("ledger_master", "LEDGER MASTER MENU", "#2563EB", "");
    ledgerMenu.items = {
        MenuItem("[N] New Ledger Entry", "F3 / Alt+N", Qt::Key_N, MenuActionType::OpenView, 6),
        MenuItem("[M] Modify Existing Ledger", "Enter", Qt::Key_M, MenuActionType::OpenView, 7),
        MenuItem("[L] View Ledger Statement (Account Book)", "F4 / Alt+L", Qt::Key_L, MenuActionType::OpenView, 8),
        MenuItem("[G] New Account Group", "", Qt::Key_G, MenuActionType::OpenView, 9),
        MenuItem("[R] Modify Account Group", "", Qt::Key_R, MenuActionType::OpenView, 10),
        MenuItem("[A] All Ledgers Directory / Master List", "Alt+6", Qt::Key_A, MenuActionType::OpenView, 5)
    };
    registerMenu(ledgerMenu);

    // 2. STOCK MASTER MENU (§23.6 titles where the mapping is 1:1; the three
    // category rows below use generic vocabulary per Doc 20 A2-A3 and stay
    // bound to core view 13, always on — reworded, not gated).
    MenuNode stockMenu("stock_master", "STOCK MASTER MENU", "#16A34A", "");
    stockMenu.items = {
        MenuItem("[I] New Stock Item Master", "Alt+I", Qt::Key_I, MenuActionType::OpenView, 11),
        MenuItem("[M] Modify Stock Item Master", "Enter", Qt::Key_M, MenuActionType::OpenView, 12),
        MenuItem("[R] Stock Register (View Options)", "Alt+4 / S", Qt::Key_R, MenuActionType::OpenSubmenu, -1, "stock_register_hub"),
        MenuItem("[C] Closing Stock & Valuation", "Alt+C", Qt::Key_C, MenuActionType::OpenView, 36),
        MenuItem("[P] Raw Material Stock Register", "", Qt::Key_P, MenuActionType::OpenView, 13),
        MenuItem("[F] Finished Goods Stock Register", "", Qt::Key_F, MenuActionType::OpenView, 13),
        MenuItem("[H] By-Products Stock Register", "", Qt::Key_H, MenuActionType::OpenView, 13)
    };
    registerMenu(stockMenu);

    // 3. ADD VOUCHER MENU (§23.6 usage-first titles).
    MenuNode addVoucherMenu("add_voucher", "ADD VOUCHER MENU", "#2563EB", "");
    addVoucherMenu.items = {
        MenuItem("[S] Sales – Tax Invoice", "F8", Qt::Key_S, MenuActionType::OpenView, 14),
        MenuItem("[P] Purchase – Bill", "F9", Qt::Key_P, MenuActionType::OpenView, 15),
        MenuItem("[J] Journal", "F5", Qt::Key_J, MenuActionType::OpenView, 17),
        MenuItem("[B] Bank – Payment / Receipt", "F3", Qt::Key_B, MenuActionType::OpenView, 16),
        MenuItem("[C] Cash – Payment / Receipt", "F6", Qt::Key_C, MenuActionType::OpenView, 60),
        MenuItem("[M] Manufacturing Entry", "", Qt::Key_M, MenuActionType::OpenView, 31),
        MenuItem("[D] Debit / Credit Notes", "Alt+D", Qt::Key_D, MenuActionType::OpenView, 28),
        MenuItem("[E] Bill Series (Sale Invoice Numbering)", "", Qt::Key_E, MenuActionType::ExecuteCustom)
    };
    registerMenu(addVoucherMenu);

    // 4. OTHER VOUCHERS MENU (§23.4 slim: J-Form + Paddy moved to `mandi`,
    // not copied; hub entry stays here only while mandi is off).
    MenuNode otherVoucherMenu("other_voucher", "OTHER VOUCHERS MENU", "#7C3AED", "");
    otherVoucherMenu.items = {
        MenuItem("[M] Mandi & Trading Operations", "M / Alt+M", Qt::Key_M, MenuActionType::OpenSubmenu, -1, "mandi_trading_hub"),
        MenuItem("[T] TDS / TCS Vouchers & Challans", "F12", Qt::Key_T, MenuActionType::OpenSubmenu, -1, "tds_tcs_hub"),
        MenuItem("[B] Bank Statement Auto-Import & Reconciliation", "Ctrl+B", Qt::Key_B, MenuActionType::OpenView, 26),
        MenuItem("[D] Transport Dispatch & Gate Pass Register", "Alt+T", Qt::Key_D, MenuActionType::OpenView, 27),
        MenuItem("[G] GSTR-2A Matching & ITC Reconciliation", "Alt+G", Qt::Key_G, MenuActionType::OpenView, 34),
        MenuItem("[S] Employee Payroll & Salary Register", "Alt+S", Qt::Key_S, MenuActionType::OpenView, 80)
    };
    registerMenu(otherVoucherMenu);

    // 4b. MANDI & PROCUREMENT MENU (§23.4 top-level isolation: J-Form,
    // I-Form and the paddy slip live here, nowhere else).
    MenuNode mandiMenu("mandi", "MANDI & PROCUREMENT MENU", "#7C3AED", "");
    mandiMenu.items = {
        MenuItem("[J] Mandi Purchase – J-Form", "F11 / J", Qt::Key_J, MenuActionType::OpenView, 18),
        MenuItem("[I] Mandi Sale – I-Form", "I", Qt::Key_I, MenuActionType::OpenView, 19),
        MenuItem("[P] Paddy Procurement Slip", "F2 / P", Qt::Key_P, MenuActionType::OpenView, 1),
        MenuItem("[M] Mandi Operations Hub", "M", Qt::Key_M, MenuActionType::OpenSubmenu, -1, "mandi_trading_hub")
    };
    registerMenu(mandiMenu);

    // 4b. MANDI, BARDANA & TRADING OPERATIONS HUB
    MenuNode mandiTradingHub("mandi_trading_hub", "MANDI & TRADING OPERATIONS", "#7C3AED", "other_voucher");
    mandiTradingHub.items = {
        MenuItem("[B] Bardana (Gunny Bag) Packaging Ledger", "B", Qt::Key_B, MenuActionType::OpenView, 70),
        MenuItem("[G] Gate Inward / Outward Register (Weighbridge)", "G", Qt::Key_G, MenuActionType::OpenView, 71),
        MenuItem("[S] Sauda (Forward Contracts) & Dalali Settlement", "S", Qt::Key_S, MenuActionType::OpenView, 72),
        MenuItem("[M] Mandi Form M & Statutory Returns (HSAMB)", "M / Alt+M", Qt::Key_M, MenuActionType::OpenView, 20),
        MenuItem("[Q] Quit / Back", "Esc / Q", Qt::Key_Q, MenuActionType::Back)
    };
    registerMenu(mandiTradingHub);

    // 5. BOOKS & REGISTERS MENU (§23.6: no numeric prefixes, one letter
    // hotkey per row; digit keys follow visible positions in the dialog).
    MenuNode reportsMenu("reports_register", "BOOKS & REGISTERS MENU", "#059669", "");
    reportsMenu.items = {
        MenuItem("[D] Day Book (Daily Audit & Transaction Register)", "Alt+D", Qt::Key_D, MenuActionType::OpenView, 33),
        MenuItem("[C] Cash Book & Flow Statements (Cash, Bank, Joint)", "Alt+C / C", Qt::Key_C, MenuActionType::OpenSubmenu, -1, "cash_book_hub"),
        MenuItem("[B] Bank Statement Parser & Auto-Reconciliation", "Ctrl+B", Qt::Key_B, MenuActionType::OpenView, 26),
        MenuItem("[S] Sales Register & Summary", "", Qt::Key_S, MenuActionType::OpenView, 3),
        MenuItem("[P] Purchase Register & Summary", "", Qt::Key_P, MenuActionType::OpenView, 4),
        MenuItem("[R] Stock Register (View Options)", "S", Qt::Key_R, MenuActionType::OpenSubmenu, -1, "stock_register_hub"),
        MenuItem("[T] Transport Dispatch & Gate Pass Register", "Alt+T", Qt::Key_T, MenuActionType::OpenView, 27),
        MenuItem("[M] Manufacturing Production & Out-turn Statement", "Alt+M", Qt::Key_M, MenuActionType::OpenView, 35),
        MenuItem("[E] Employee Payroll & Salary Register", "Alt+S", Qt::Key_E, MenuActionType::OpenView, 80),
        MenuItem("[A] Audit Trail (Edit Log & Refused Writes)", "", Qt::Key_A, MenuActionType::OpenView, 64)
    };
    registerMenu(reportsMenu);

    // 6. FINAL ACCOUNTS & FINANCIALS MENU (§23.6: same treatment).
    MenuNode finalAccountsMenu("final_accounts", "FINAL ACCOUNTS & FINANCIALS MENU", "#4F46E5", "");
    finalAccountsMenu.items = {
        MenuItem("[B] Balance Sheet (Liabilities & Assets)", "B / Ctrl+Alt+B", Qt::Key_B, MenuActionType::OpenView, 29),
        MenuItem("[P] Profit & Loss Statement (Trading & P&L)", "P / Ctrl+Alt+P", Qt::Key_P, MenuActionType::OpenView, 30),
        MenuItem("[T] Trial Balance Sheet (Group & Ledger Summaries)", "A / F7", Qt::Key_T, MenuActionType::OpenSubmenu, -1, "trial_balance_options_hub"),
        MenuItem("[G] GST Compliance Dashboard (GSTR-1, 2A Match, 3B)", "Alt+G", Qt::Key_G, MenuActionType::OpenView, 34),
        MenuItem("[I] Interest Calculator (Bulk & Party-wise)", "Alt+A", Qt::Key_I, MenuActionType::OpenView, 25),
        MenuItem("[C] Capital Accounts Summary", "C", Qt::Key_C, MenuActionType::OpenView, 40),
        MenuItem("[D] Depreciation Chart & Schedule", "E", Qt::Key_D, MenuActionType::OpenSubmenu, -1, "depreciation_options_hub"),
        MenuItem("[L] Closing Stock Valuation & Year-End Audit", "Alt+C", Qt::Key_L, MenuActionType::OpenView, 36),
        MenuItem("[Y] Close Financial Year (Guided Checklist)", "", Qt::Key_Y, MenuActionType::ExecuteCustom)
    };
    registerMenu(finalAccountsMenu);

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
        MenuItem("[O] Show Only Stock (Quantities)", "O", Qt::Key_O, MenuActionType::OpenSubmenu, -1, "stock_only_menu"),
        MenuItem("[A] Show Stock With Amount (Valuation)", "A", Qt::Key_A, MenuActionType::OpenSubmenu, -1, "stock_amount_menu"),
        MenuItem("[P] Show Item Wise Profit & Loss", "P", Qt::Key_P, MenuActionType::OpenSubmenu, -1, "stock_profit_menu"),
        MenuItem("[D] Item Monthly / Daily Stock", "D", Qt::Key_D, MenuActionType::OpenView, 13),
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
    MenuNode finalReportsHub("final_reports_hub", "FINAL REPORTS (ALT+F2: SET PERIOD)", "#4F46E5", "final_accounts");
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
        MenuItem("[S] Summarized Report", "S", Qt::Key_S, MenuActionType::OpenView, 41),
        MenuItem("[D] Detailed Report", "D", Qt::Key_D, MenuActionType::OpenView, 41),
        MenuItem("[Q] Quit / Back", "Esc / Q", Qt::Key_Q, MenuActionType::Back)
    };
    registerMenu(depOptionsHub);
}

} // namespace MahadevERP
