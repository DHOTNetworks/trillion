#pragma once

#include <QWidget>
#include <QTableWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include "../models/tcs_receipt_voucher_controller.h"

namespace MahadevERP {

class TcsReceiptListWidget : public QWidget {
    Q_OBJECT

public:
    explicit TcsReceiptListWidget(TcsReceiptVoucherController *controller, QWidget *parent = nullptr);

    void reloadData();

signals:
    void backRequested();
    void newReceiptRequested();

protected:
    void keyPressEvent(QKeyEvent *event) override;

private slots:
    void onNewClicked();
    void onDeleteClicked();
    void onRefreshClicked();
    void onFilterChanged();

private:
    void setupUi();
    void populateTable();

    TcsReceiptVoucherController *m_controller = nullptr;

    QLineEdit *m_searchEdit = nullptr;
    QTableWidget *m_table = nullptr;

    QLabel *m_totalVouchersVal = nullptr;
    QLabel *m_totalBankReceiptVal = nullptr;
    QLabel *m_totalTcsVal = nullptr;
    QLabel *m_totalCreditPartyVal = nullptr;

    QVariantList m_currentReceipts;
};

} // namespace MahadevERP
