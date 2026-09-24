#pragma once

#include <QWidget>
#include <QTableWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>

namespace MahadevERP {

class AdvancePayment194QListWidget : public QWidget {
    Q_OBJECT

public:
    explicit AdvancePayment194QListWidget(QWidget *parent = nullptr);

    void reloadData();

signals:
    void backRequested();
    void newVoucherRequested();

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

    QLineEdit *m_searchEdit = nullptr;
    QTableWidget *m_table = nullptr;

    QLabel *m_totalVouchersVal = nullptr;
    QLabel *m_totalGrossVal = nullptr;
    QLabel *m_totalTaxVal = nullptr;
    QLabel *m_totalNetVal = nullptr;

    QVariantList m_currentVouchers;
};

} // namespace MahadevERP
