#pragma once

#include <QWidget>
#include <QTableWidget>
#include <QLineEdit>
#include <QDateEdit>
#include <QComboBox>
#include <QPushButton>
#include <QLabel>
#include <QFrame>
#include "../models/trial_balance_controller.h"
#include "../services/print_export_controller.h"

namespace MahadevERP {

class TrialBalanceWidget : public QWidget {
    Q_OBJECT

public:
    explicit TrialBalanceWidget(TrialBalanceController* controller = nullptr, PrintExportController* printExportCtrl = nullptr, QWidget* parent = nullptr);
    ~TrialBalanceWidget() override = default;

    void reloadData();
    void setMode(TrialBalanceMode mode);
    void focusTable();

signals:
    void backRequested();
    void openLedgerRequested(const QString& ledgerName);
    void openViewOptionsRequested();

protected:
    void keyPressEvent(QKeyEvent* event) override;

private slots:
    void onSearchChanged(const QString& text);
    void onDateFilterChanged();
    void onModeComboChanged(int index);
    void onTableDoubleClicked(int row, int col);
    void onOpenViewOptions();
    void onExportCsv();
    void onPrintPdf();

private:
    void setupUi();
    void configureTableColumns();
    void populateTable();
    void updateSummaryMetrics();

    TrialBalanceController* m_controller = nullptr;
    PrintExportController* m_printExportCtrl = nullptr;

    // Tier 1 Header
    QLabel* m_titleLabel = nullptr;
    QLabel* m_subtitleLabel = nullptr;
    QLabel* m_modeBadge = nullptr;

    // Tier 2 Filter Bar
    QDateEdit* m_fromDateEdit = nullptr;
    QDateEdit* m_toDateEdit = nullptr;
    QComboBox* m_modeCombo = nullptr;
    QLineEdit* m_searchEdit = nullptr;
    QLabel* m_balanceStatusBadge = nullptr;

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
