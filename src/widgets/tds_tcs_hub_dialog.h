#pragma once

#include "navigation_menu_dialog.h"

namespace MahadevERP {

class TdsTcsHubMenuDialog : public NavigationMenuDialog {
    Q_OBJECT

public:
    enum Action {
        None,
        TdsVoucherEntry,
        TcsVoucherEntry
    };

    explicit TdsTcsHubMenuDialog(QWidget *parent = nullptr);
    Action selectedAction() const { return m_selectedAction; }

signals:
    void openTdsOptionsRequested();
    void openTcsOptionsRequested();

protected:
    void accept() override;

private:
    Action m_selectedAction = None;
};

} // namespace MahadevERP
