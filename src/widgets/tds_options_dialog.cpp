#include "tds_options_dialog.h"
#include "../models/menu_tree_manager.h"

namespace MahadevERP {

TdsOptionsMenuDialog::TdsOptionsMenuDialog(QWidget *parent)
    : NavigationMenuDialog("T.D.S. OPTIONS MENU", "#2563EB",
        {
            MenuItem("[N] New TDS Voucher", "N", Qt::Key_N, MenuActionType::OpenView, 24),
            MenuItem("[L] TDS Vouchers List", "L", Qt::Key_L, MenuActionType::OpenView, 50),
            MenuItem("[T] TDS Deposit Challans", "T", Qt::Key_T, MenuActionType::OpenView, 52),
            MenuItem("[A] Acknowledgement Nos.", "A", Qt::Key_A, MenuActionType::ExecuteCustom),
            MenuItem("[P] Advance Pymt Vch. U/S 194-Q", "P", Qt::Key_P, MenuActionType::OpenView, 55),
            MenuItem("[V] Advance Pymt Vouchers List", "V", Qt::Key_V, MenuActionType::OpenView, 56),
            MenuItem("[R] Receive Form-16A / 27D", "R", Qt::Key_R, MenuActionType::ExecuteCustom),
            MenuItem("[F] Received Forms-16A List", "F", Qt::Key_F, MenuActionType::OpenView, 57),
            MenuItem("[Q] Quit", "Q", Qt::Key_Q, MenuActionType::Back)
        },
        MenuTreeManager::instance().lastSelectedIndex("tds_options"),
        parent)
{
}

void TdsOptionsMenuDialog::accept()
{
    int idx = selectedIndex();
    MenuTreeManager::instance().setLastSelectedIndex("tds_options", idx);
    switch (idx) {
    case 0:
        m_selectedAction = NewVoucher;
        emit newVoucherRequested();
        break;
    case 1:
        m_selectedAction = VouchersList;
        emit vouchersListRequested();
        break;
    case 2:
        m_selectedAction = DepositChallans;
        emit depositChallansRequested();
        break;
    case 3:
        m_selectedAction = AcknowledgementNos;
        emit ackNosRequested();
        break;
    case 4:
        m_selectedAction = AdvancePymt194Q;
        emit advancePymt194QRequested();
        break;
    case 5:
        m_selectedAction = AdvancePymtList;
        emit advancePymtListRequested();
        break;
    case 6:
        m_selectedAction = ReceiveForm16A;
        emit receiveForm16ARequested();
        break;
    case 7:
        m_selectedAction = ReceivedForms16AList;
        emit receivedForms16AListRequested();
        break;
    default:
        m_selectedAction = None;
        reject();
        return;
    }
    NavigationMenuDialog::accept();
}

} // namespace MahadevERP
