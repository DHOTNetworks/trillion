#pragma once

#include <QWidget>
#include <QLineEdit>
#include <QComboBox>
#include <QPushButton>
#include <QTableWidget>
#include <QLabel>
#include "../models/tax_challan_controller.h"
#include "account_search_box.h"

namespace MahadevERP {

class TaxChallanCreationWidget : public QWidget {
    Q_OBJECT

public:
    explicit TaxChallanCreationWidget(TaxChallanController *controller, QWidget *parent = nullptr);

    void setTaxType(const QString &taxType);
    void resetForm();

signals:
    void backRequested();
    void challanSaved(int challanId);

protected:
    void keyPressEvent(QKeyEvent *event) override;

private slots:
    void onFetchClicked();
    void onSaveClicked();
    void onTableItemChanged(QTableWidgetItem *item);
    void onSelectAllClicked();
    void onSelectNoneClicked();
    void syncUiFromController();

private:
    void setupUi();
    void populateTable();
    QWidget *createHeaderBar();
    QWidget *createParamsCard();
    QWidget *createAdjustmentCard();
    QWidget *createBankDetailsCard();

    TaxChallanController *m_controller = nullptr;

    QLabel *m_titleLabel = nullptr;
    QLineEdit *m_challanNoEdit = nullptr;
    QLineEdit *m_challanDateEdit = nullptr;
    QComboBox *m_postInBooksCombo = nullptr;
    QLineEdit *m_periodFromEdit = nullptr;
    QLineEdit *m_periodToEdit = nullptr;

    QTableWidget *m_vouchersTable = nullptr;
    QLabel *m_tableSummaryLabel = nullptr;

    QLineEdit *m_basicTaxEdit = nullptr;
    QLineEdit *m_interestAmtEdit = nullptr;
    AccountSearchBox *m_interestLedgerBox = nullptr;
    QLineEdit *m_penaltyAmtEdit = nullptr;
    AccountSearchBox *m_penaltyLedgerBox = nullptr;
    QLineEdit *m_otherAmtEdit = nullptr;
    AccountSearchBox *m_otherLedgerBox = nullptr;
    QLineEdit *m_totalChallanAmtEdit = nullptr;

    AccountSearchBox *m_bankLedgerBox = nullptr;
    QLineEdit *m_chequeNoEdit = nullptr;
    QLineEdit *m_chequeDateEdit = nullptr;
    QLineEdit *m_bsrCodeEdit = nullptr;
    QLineEdit *m_minorHeadEdit = nullptr;
    QLineEdit *m_majorHeadEdit = nullptr;
    QLineEdit *m_narrationEdit = nullptr;

    QLabel *m_statusLabel = nullptr;
    bool m_isUpdatingTable = false;
};

} // namespace MahadevERP
