#pragma once

#include <QWidget>
#include <QTableWidget>
#include <QLineEdit>
#include <QComboBox>
#include <QPushButton>
#include <QLabel>
#include "../models/tds_voucher_controller.h"

namespace MahadevERP {

class TdsVouchersListWidget : public QWidget {
    Q_OBJECT

public:
    explicit TdsVouchersListWidget(TdsVoucherController *controller, QWidget *parent = nullptr);

    void reloadData();

signals:
    void backRequested();
    void newVoucherRequested();
    void editVoucherRequested(int voucherId);

protected:
    void keyPressEvent(QKeyEvent *event) override;

private slots:
    void onNewClicked();
    void onEditClicked();
    void onDeleteClicked();
    void onRefreshClicked();
    void onFilterChanged();

private:
    void setupUi();
    void populateTable();

    TdsVoucherController *m_controller = nullptr;

    QLineEdit *m_searchEdit = nullptr;
    QComboBox *m_typeFilterCombo = nullptr;
    QTableWidget *m_table = nullptr;

    QLabel *m_totalVouchersVal = nullptr;
    QLabel *m_totalGrossVal = nullptr;
    QLabel *m_totalTaxVal = nullptr;
    QLabel *m_totalNetVal = nullptr;

    QVariantList m_currentVouchers;
};

} // namespace MahadevERP
