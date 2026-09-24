#pragma once

#include <QWidget>
#include <QTableWidget>
#include <QLineEdit>
#include <QDateEdit>
#include <QComboBox>
#include <QPushButton>
#include <QLabel>
#include <QFrame>
#include "../models/stock_register_model.h"
#include "../services/print_export_controller.h"
#include "../models/menu_tree_manager.h"

namespace MahadevERP {

class StockDetailWidget : public QWidget {
    Q_OBJECT

public:
    explicit StockDetailWidget(StockRegisterController* controller = nullptr, PrintExportController* printExportCtrl = nullptr, QWidget* parent = nullptr);
    ~StockDetailWidget() override = default;

    void reloadData(const QString& fromDate = "", const QString& toDate = "");
    void setViewConfiguration(StockViewMode mode, StockGrouping grouping, const QString& title = "");
    void focusTable();

signals:
    void backRequested();
    void openItemMovement(const QString& itemName);

protected:
    void keyPressEvent(QKeyEvent* event) override;

private slots:
    void onSearchChanged(const QString& query);
    void onDateFilterChanged();
    void onModeComboChanged(int index);
    void onGroupingComboChanged(int index);
    void onItemComboChanged(int index);
    void onTimelineComboChanged(int index);
    void onOpenOptionsDialog();
    void onTableDoubleClicked(int row, int col);
    void onPrintRegister();
    void onExportPdf();
    void onExportCsv();
    void updateSummaryMetrics();

private:
    void setupUi();
    void configureTableColumns();
    void populateTable();
    void populateItemDropdown();
    void updateHeaderLabels();

    StockRegisterController* m_controller = nullptr;
    PrintExportController* m_printExportCtrl = nullptr;

    // Tier 1 Header
    QLabel* m_titleLabel = nullptr;
    QLabel* m_subtitleLabel = nullptr;

    // Tier 2 Filter Controls
    QDateEdit* m_fromDateEdit = nullptr;
    QDateEdit* m_toDateEdit = nullptr;
    QComboBox* m_modeCombo = nullptr;
    QComboBox* m_groupingCombo = nullptr;
    QComboBox* m_itemSelectorCombo = nullptr;
    QComboBox* m_timelineCombo = nullptr;
    QLineEdit* m_searchEdit = nullptr;

    // Tier 3 Data Table
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

    bool m_isUpdatingUi = false;
    QString m_customTitle;
};

} // namespace MahadevERP
