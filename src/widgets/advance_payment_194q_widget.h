#pragma once

#include <QWidget>
#include <QLineEdit>
#include <QComboBox>
#include <QPushButton>
#include <QLabel>
#include "account_search_box.h"

namespace MahadevERP {

class AdvancePayment194QWidget : public QWidget {
    Q_OBJECT

public:
    explicit AdvancePayment194QWidget(QWidget *parent = nullptr);

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
    void recalculate();

private:
    void setupUi();
    QWidget *createHeaderBar();
    QWidget *createFormCard();
    int nextVoucherNumber() const;

    int m_editVoucherId = 0;
    QLabel *m_statusLabel = nullptr;

    QLineEdit *m_voucherNoEdit = nullptr;
    QLineEdit *m_voucherDateEdit = nullptr;
    QComboBox *m_postInBooksCombo = nullptr;

    AccountSearchBox *m_supplierSearch = nullptr;
    QLineEdit *m_supplierPanEdit = nullptr;
    AccountSearchBox *m_bankSearch = nullptr;

    QLineEdit *m_grossAdvanceEdit = nullptr;
    QLineEdit *m_tdsRateEdit = nullptr;
    QLineEdit *m_tdsAmountEdit = nullptr;
    QLineEdit *m_netPaymentEdit = nullptr;

    QLineEdit *m_chequeNoEdit = nullptr;
    QLineEdit *m_chequeDateEdit = nullptr;
    QLineEdit *m_narrationEdit = nullptr;
};

} // namespace MahadevERP
