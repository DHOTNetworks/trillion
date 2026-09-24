#include "tcs_options_dialog.h"
#include "../models/menu_tree_manager.h"

namespace MahadevERP {

TcsOptionsMenuDialog::TcsOptionsMenuDialog(QWidget *parent)
    : NavigationMenuDialog("T.C.S. OPTIONS MENU", "#16A34A",
        {
            MenuItem("[R] Receipt Voucher For TCS", "R", Qt::Key_R, MenuActionType::OpenView, 53),
            MenuItem("[L] Receipt Vouchers List", "L", Qt::Key_L, MenuActionType::OpenView, 54),
            MenuItem("[T] TCS Deposit Challans", "T", Qt::Key_T, MenuActionType::OpenView, 52),
            MenuItem("[O] Others (TCS Settings)", "O", Qt::Key_O, MenuActionType::ExecuteCustom),
            MenuItem("[Q] Quit", "Q", Qt::Key_Q, MenuActionType::Back)
        },
        MenuTreeManager::instance().lastSelectedIndex("tcs_options"),
        parent)
{
}

void TcsOptionsMenuDialog::accept()
{
    int idx = selectedIndex();
    MenuTreeManager::instance().setLastSelectedIndex("tcs_options", idx);
    switch (idx) {
    case 0:
        m_selectedAction = ReceiptVoucherForTcs;
        emit receiptVoucherForTcsRequested();
        break;
    case 1:
        m_selectedAction = ReceiptVouchersList;
        emit receiptVouchersListRequested();
        break;
    case 2:
        m_selectedAction = TcsDepositChallans;
        emit tcsDepositChallansRequested();
        break;
    case 3:
        m_selectedAction = Others;
        emit othersRequested();
        break;
    default:
        m_selectedAction = None;
        reject();
        return;
    }
    NavigationMenuDialog::accept();
}

} // namespace MahadevERP
