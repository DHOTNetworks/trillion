#pragma once

#include <QWidget>
#include <QLineEdit>
#include <QComboBox>
#include <QPushButton>
#include <QLabel>
#include "../models/tcs_receipt_voucher_controller.h"
#include "account_search_box.h"

namespace MahadevERP {

class TcsReceiptVoucherWidget : public QWidget {
    Q_OBJECT

public:
    explicit TcsReceiptVoucherWidget(TcsReceiptVoucherController *controller, QWidget *parent = nullptr);

    void resetForm();
    void loadVoucher(int voucherId);

signals:
    void backRequested();
    void voucherSaved(int voucherId);

protected:
    void keyPressEvent(QKeyEvent *event) override;

private slots:
    void onSaveClicked();
    void onNewClicked();
    void syncUiFromController();

private:
    void setupUi();
    QWidget *createHeaderBar();
    QWidget *createParamsCard();
    QWidget *createMainFormCard();

    TcsReceiptVoucherController *m_controller = nullptr;

    QLabel *m_titleLabel = nullptr;
    QLabel *m_statusLabel = nullptr;

    QLineEdit *m_receiptNoEdit = nullptr;
    QLineEdit *m_receiptDateEdit = nullptr;
    QComboBox *m_typeCombo = nullptr;
    QComboBox *m_postInBooksCombo = nullptr;

    AccountSearchBox *m_partySearch = nullptr;
    QLineEdit *m_panEdit = nullptr;
    AccountSearchBox *m_bankSearch = nullptr;

    QLineEdit *m_bankAmountEdit = nullptr;
    QLineEdit *m_withoutTcsEdit = nullptr;
    QLineEdit *m_tcsRateEdit = nullptr;
    QLineEdit *m_tcsAmountEdit = nullptr;
    QComboBox *m_tcsReceivedCombo = nullptr;
    QLineEdit *m_netBankReceiptEdit = nullptr;

    QLineEdit *m_interestAmtEdit = nullptr;
    AccountSearchBox *m_interestLedgerBox = nullptr;
    QLineEdit *m_discountAmtEdit = nullptr;
    AccountSearchBox *m_discountLedgerBox = nullptr;
    QLineEdit *m_otherAmtEdit = nullptr;
    AccountSearchBox *m_otherLedgerBox = nullptr;

    QLineEdit *m_netCreditPartyEdit = nullptr;
    QLineEdit *m_narrationEdit = nullptr;

    bool m_isSyncing = false;
};

} // namespace MahadevERP
