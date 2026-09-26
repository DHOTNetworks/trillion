#pragma once

#include <QWidget>
#include <QTableWidget>
#include <QDateEdit>
#include <QPushButton>
#include <QLabel>
#include <QFrame>
#include "../models/capital_accounts_controller.h"
#include "../services/print_export_controller.h"

namespace MahadevERP {

class CapitalAccountsWidget : public QWidget {
    Q_OBJECT

public:
    explicit CapitalAccountsWidget(CapitalAccountsController* controller = nullptr, PrintExportController* printExportCtrl = nullptr, QWidget* parent = nullptr);
    ~CapitalAccountsWidget() override = default;

    void reloadData();
    void focusTable();

signals:
    void backRequested();
    void openLedgerRequested(const QString& ledgerName);

protected:
    void keyPressEvent(QKeyEvent* event) override;

private slots:
    void onDateFilterChanged();
    void onTableDoubleClicked(int row, int col);
    void onExportCsv();
    void onExportPdf();
    void onExportOdf();
    void onExportExcel();
    void onPrintPdf();

private:
    void setupUi();
    void populateTable();
    void updateSummaryMetrics();

    CapitalAccountsController* m_controller = nullptr;
    PrintExportController* m_printExportCtrl = nullptr;

    // Tier 1 Header
    QLabel* m_titleLabel = nullptr;
    QLabel* m_subtitleLabel = nullptr;

    // Tier 2 Filter Bar
    QDateEdit* m_fromDateEdit = nullptr;
    QDateEdit* m_toDateEdit = nullptr;

    // Tier 3 Table
    QTableWidget* m_table = nullptr;

    // Tier 4 Metric Cards
    QFrame* m_card1 = nullptr;
    QLabel* m_card1Title = nullptr;
    QLabel* m_card1Val = nullptr;

    QFrame* m_card2 = nullptr;
    QLabel* m_card2Title = nullptr;
    QLabel* m_card2Val = nullptr;

    QFrame* m_card3 = nullptr;
    QLabel* m_card3Title = nullptr;
    QLabel* m_card3Val = nullptr;

    QFrame* m_card4 = nullptr;
    QLabel* m_card4Title = nullptr;
    QLabel* m_card4Val = nullptr;
};

} // namespace MahadevERP
