#pragma once

#include "navigation_menu_dialog.h"

namespace MahadevERP {

class TdsOptionsMenuDialog : public NavigationMenuDialog {
    Q_OBJECT

public:
    enum Action {
        None,
        NewVoucher,
        VouchersList,
        DepositChallans,
        AcknowledgementNos,
        AdvancePymt194Q,
        AdvancePymtList,
        ReceiveForm16A,
        ReceivedForms16AList
    };

    explicit TdsOptionsMenuDialog(QWidget *parent = nullptr);
    Action selectedAction() const { return m_selectedAction; }

signals:
    void newVoucherRequested();
    void vouchersListRequested();
    void depositChallansRequested();
    void ackNosRequested();
    void advancePymt194QRequested();
    void advancePymtListRequested();
    void receiveForm16ARequested();
    void receivedForms16AListRequested();

protected:
    void accept() override;

private:
    Action m_selectedAction = None;
};

} // namespace MahadevERP
