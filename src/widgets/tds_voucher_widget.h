#pragma once

#include <QWidget>
#include <QLineEdit>
#include <QComboBox>
#include <QCheckBox>
#include <QPushButton>
#include <QLabel>
#include "../models/tds_voucher_controller.h"
#include "../services/print_export_controller.h"
#include "account_search_box.h"

class KbdBadgeButton;

namespace MahadevERP {

class TdsVoucherWidget : public QWidget {
    Q_OBJECT

public:
    explicit TdsVoucherWidget(TdsVoucherController* controller = nullptr,
                              PrintExportController* printExportCtrl = nullptr,
                              QWidget* parent = nullptr);

public slots:
    void resetForm();
    bool loadVoucherForEditing(int voucherId);
    void openDateDialog();
    void onSaveClicked();
    void onDeleteClicked();
    void saveVoucher() { onSaveClicked(); }
    void deleteVoucher() { onDeleteClicked(); }

signals:
    void backRequested();
    void voucherSaved(int voucherNo);

private slots:
    void onTdsTypeChanged(int index);
    void onPartySelected(const QString& partyName);
    void onRecalculateTax();

private:
    void setupUi();

    TdsVoucherController* m_controller = nullptr;
    PrintExportController* m_printExportCtrl = nullptr;

    int m_editVoucherId = 0;

    // Controls
    QLineEdit* m_voucherNoEdit = nullptr;
    QLineEdit* m_voucherDateEdit = nullptr;
    QLabel* m_dayLabel = nullptr;
    QCheckBox* m_postBooksCheck = nullptr;

    QComboBox* m_tdsTypeCombo = nullptr;
    AccountSearchBox* m_partySearch = nullptr;
    QLineEdit* m_panEdit = nullptr;
    QLabel* m_partyBalanceLabel = nullptr;

    QLineEdit* m_incomeAmountEdit = nullptr;
    QLineEdit* m_previousAmountEdit = nullptr;
    QLabel* m_totalForTdsLabel = nullptr;

    QLineEdit* m_tdsRateEdit = nullptr;
    QLabel* m_tdsTaxAmountLabel = nullptr;
    QLineEdit* m_surchargeRateEdit = nullptr;
    QLabel* m_surchargeTaxAmountLabel = nullptr;
    QLineEdit* m_cessRateEdit = nullptr;
    QLabel* m_cessTaxAmountLabel = nullptr;
    QCheckBox* m_roundOffCheck = nullptr;

    QLabel* m_totalTaxLabel = nullptr;
    QLabel* m_netPayableLabel = nullptr;
    QLabel* m_taxableSummaryLabel = nullptr;
    QLabel* m_tdsSummaryLabel = nullptr;
    QLabel* m_surchCessSummaryLabel = nullptr;
    QLabel* m_netPayableSummaryLabel = nullptr;
    QLabel* m_fyBadge = nullptr;

    AccountSearchBox* m_expLedgerEdit = nullptr;
    AccountSearchBox* m_tdsLedgerEdit = nullptr;
    QLineEdit* m_narrationEdit = nullptr;
    QLineEdit* m_nonDeductReasonEdit = nullptr;

    KbdBadgeButton* m_saveBtn = nullptr;
    KbdBadgeButton* m_deleteBtn = nullptr;
    KbdBadgeButton* m_cancelBtn = nullptr;
};

} // namespace MahadevERP
