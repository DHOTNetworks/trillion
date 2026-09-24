#pragma once

#include "navigation_menu_dialog.h"

namespace MahadevERP {

class TcsOptionsMenuDialog : public NavigationMenuDialog {
    Q_OBJECT

public:
    enum Action {
        None,
        ReceiptVoucherForTcs,
        ReceiptVouchersList,
        TcsDepositChallans,
        Others
    };

    explicit TcsOptionsMenuDialog(QWidget *parent = nullptr);
    Action selectedAction() const { return m_selectedAction; }

signals:
    void receiptVoucherForTcsRequested();
    void receiptVouchersListRequested();
    void tcsDepositChallansRequested();
    void othersRequested();

protected:
    void accept() override;

private:
    Action m_selectedAction = None;
};

} // namespace MahadevERP
