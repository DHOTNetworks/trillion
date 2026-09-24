#include "tds_tcs_hub_dialog.h"
#include "../models/menu_tree_manager.h"

namespace MahadevERP {

TdsTcsHubMenuDialog::TdsTcsHubMenuDialog(QWidget *parent)
    : NavigationMenuDialog("TDS / TCS VOUCHER MENU", "#7C3AED",
        {
            MenuItem("[D] T.D.S. Voucher Entry", "D", Qt::Key_D, MenuActionType::OpenSubmenu, -1, "tds_options"),
            MenuItem("[C] T.C.S. Voucher Entry", "C", Qt::Key_C, MenuActionType::OpenSubmenu, -1, "tcs_options"),
            MenuItem("[Q] Quit", "Q", Qt::Key_Q, MenuActionType::Back)
        },
        MenuTreeManager::instance().lastSelectedIndex("tds_tcs_hub"),
        parent)
{
}

void TdsTcsHubMenuDialog::accept()
{
    int idx = selectedIndex();
    MenuTreeManager::instance().setLastSelectedIndex("tds_tcs_hub", idx);
    if (idx == 0) {
        m_selectedAction = TdsVoucherEntry;
        emit openTdsOptionsRequested();
    } else if (idx == 1) {
        m_selectedAction = TcsVoucherEntry;
        emit openTcsOptionsRequested();
    } else {
        m_selectedAction = None;
        reject();
        return;
    }
    NavigationMenuDialog::accept();
}

} // namespace MahadevERP
