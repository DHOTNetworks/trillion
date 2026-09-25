#pragma once

#include <QWidget>
#include <QTableWidget>
#include <QDateEdit>
#include <QComboBox>
#include <QPushButton>
#include <QLabel>
#include <QFrame>
#include "../engine/depreciation_calculator.h"
#include "../services/print_export_controller.h"

namespace MahadevERP {

class DepreciationChartWidget : public QWidget {
    Q_OBJECT

public:
    explicit DepreciationChartWidget(PrintExportController* printExportCtrl = nullptr, QWidget* parent = nullptr);
    ~DepreciationChartWidget() override = default;

    void reloadData();
    void setDetailedMode(bool isDetailed);
    void focusTable();

signals:
    void backRequested();
    void openLedgerRequested(const QString& ledgerName);
    void makeNewLedgerRequested();

protected:
    void keyPressEvent(QKeyEvent* event) override;

private slots:
    void onDateFilterChanged();
    void onModeToggleChanged();
    void onMakeDepreciationClicked();
    void onDeleteDepreciationClicked();
    void onOpenLedgerClicked();
    void onMakeNewLedgerClicked();
    void onExportCsv();
    void onPrintPdf();

private:
    void setupUi();
    void populateTable();
    void populateLedgerCombo();
    void updateSummaryMetrics();

    PrintExportController* m_printExportCtrl = nullptr;
    DepreciationCalculator m_calculator;
    QVector<DepreciationAssetItem> m_items;
    bool m_isDetailed = true;

    // Tier 1 Header
    QLabel* m_titleLabel = nullptr;
    QLabel* m_subtitleLabel = nullptr;
    QPushButton* m_modeToggleBtn = nullptr;

    // Tier 2 Filters
    QDateEdit* m_fromDateEdit = nullptr;
    QDateEdit* m_toDateEdit = nullptr;
    QComboBox* m_depLedgerCombo = nullptr;
    QLabel* m_statusBadge = nullptr;

    // Tier 3 Table
    QTableWidget* m_table = nullptr;

    // Bottom Action Buttons
    QPushButton* m_makeDepBtn = nullptr;
    QPushButton* m_deleteDepBtn = nullptr;
    QPushButton* m_openLedgerBtn = nullptr;

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
