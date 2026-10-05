#pragma once

#include <QWidget>
#include <QTableWidget>
#include <QLineEdit>
#include <QComboBox>
#include <QPushButton>
#include <QLabel>
#include <QVector>
#include <QVariantMap>
#include "../services/print_export_controller.h"

namespace MahadevERP {

struct LedgerDirectoryItem {
    int id = 0;
    QString name;
    QString alias;
    QString groupName;
    QString city;
    QString phone;
    QString mobile;
    QString gstin;
    QString pan;
    QString address;
    double opDebit = 0.0;
    double opCredit = 0.0;
    double closeDebit = 0.0;
    double closeCredit = 0.0;
    double netClosing = 0.0; // Positive for Dr, negative for Cr
    QString opBalFmt;
    QString closeBalFmt;
};

class LedgerDirectoryWidget : public QWidget {
    Q_OBJECT

public:
    explicit LedgerDirectoryWidget(PrintExportController* printExportCtrl = nullptr, QWidget* parent = nullptr);

    void loadData();
    void markDirty() { m_isDirty = true; }
    bool isDirty() const { return m_isDirty; }
    void focusTable();
    void focusSearch();

signals:
    void backRequested();
    void openStatementRequested(const QString& partyName);
    void modifyLedgerRequested(int partyId, const QString& partyName);
    void newLedgerRequested();

protected:
    void keyPressEvent(QKeyEvent* event) override;
    void showEvent(QShowEvent* event) override;
    void paintEvent(QPaintEvent* event) override;

private slots:
    void onSearchChanged(const QString& query);
    void onGroupFilterChanged(int index);
    void onBalanceFilterChanged(int index);
    void onTableDoubleClicked(int row, int col);
    void onPrintClicked();
    void onExportPdfClicked();
    void onExportCsvClicked();

private:
    void setupUi();
    void applyFilter();
    void populateTable();
    void updateSummaryCards();

    PrintExportController* m_printExportCtrl = nullptr;

    QLineEdit* m_searchEdit = nullptr;
    QComboBox* m_groupCombo = nullptr;
    QComboBox* m_balanceFilterCombo = nullptr;
    QLabel* m_fyBadge = nullptr;

    // Stat Cards
    QLabel* m_totalAccountsLabel = nullptr;
    QLabel* m_totalOpDrLabel = nullptr;
    QLabel* m_totalOpCrLabel = nullptr;
    QLabel* m_totalCloseDrLabel = nullptr;
    QLabel* m_totalCloseCrLabel = nullptr;
    QLabel* m_netDiffLabel = nullptr;

    QTableWidget* m_table = nullptr;

    QVector<LedgerDirectoryItem> m_allItems;
    QVector<LedgerDirectoryItem> m_filteredItems;

    bool m_isDirty = true;
};

} // namespace MahadevERP
