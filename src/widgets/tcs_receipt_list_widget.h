#pragma once

#include <QWidget>
#include <QTableWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QFutureWatcher>
#include "../models/tcs_receipt_voucher_controller.h"

namespace MahadevERP {

class TcsReceiptListWidget : public QWidget {
    Q_OBJECT

public:
    explicit TcsReceiptListWidget(TcsReceiptVoucherController *controller, QWidget *parent = nullptr);

    void reloadData();
    void markDirty() { m_isDirty = true; }
    bool isDirty() const { return m_isDirty; }
    void focusTable();

signals:
    void backRequested();
    void newReceiptRequested();

protected:
    void keyPressEvent(QKeyEvent *event) override;

private slots:
    void onNewClicked();
    void onDeleteClicked();
    void onDeleteFinished();
    void onQueryReady();
    void onRefreshClicked();
    void onFilterChanged();

private:
    void setupUi();
    void populateTable();

    TcsReceiptVoucherController *m_controller = nullptr;

    QLineEdit *m_searchEdit = nullptr;
    QTableWidget *m_table = nullptr;
    QFutureWatcher<bool> m_deleteWatcher;
    QFutureWatcher<QVariantList> m_queryWatcher;
    bool m_deleting = false;

    QLabel *m_totalVouchersVal = nullptr;
    QLabel *m_totalBankReceiptVal = nullptr;
    QLabel *m_totalTcsVal = nullptr;
    QLabel *m_totalCreditPartyVal = nullptr;

    QVariantList m_currentReceipts;
    bool m_isDirty = true;
};

} // namespace MahadevERP
