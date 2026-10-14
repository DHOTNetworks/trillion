#pragma once

#include <QWidget>
#include <QTableWidget>
#include <QLabel>
#include <QPushButton>
#include <QLineEdit>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QFrame>
#include "accounting_date_edit.h"
#include "kbd_badge_button.h"
#include "custom_dialogs.h"
#include "accounting_period_dialog.h"
#include "../services/print_export_controller.h"
#include "../engine/stock_valuation_engine.h"

namespace MahadevERP {

class CustomClosingStockWidget : public QWidget {
    Q_OBJECT

public:
    explicit CustomClosingStockWidget(PrintExportController* printCtrl, QWidget* parent = nullptr);
    void reloadData();
    void focusTable();
    void markDirty() { m_isDirty = true; }
    bool isDirty() const { return m_isDirty; }

signals:
    void backRequested();

protected:
    void keyPressEvent(QKeyEvent* event) override;

private slots:
    void onDateChanged();
    void onLoadLiveStockClicked();
    void onSaveLockStockClicked();
    void onDeleteStockClicked();
    void onExportPdfClicked();
    void onExportCsvClicked();
    void onTableItemChanged(QTableWidgetItem* item);
    void onSearchTextChanged(const QString& text);
    void onPeriodClicked();

private:
    void setupUi();
    void applyCustomStyles();
    void updateSummaryCards();
    void populateTable(const StockValuationReport& report);
    void filterTableRows();
    QWidget* createMetricCard(const QString& title, QLabel*& valueLabel, const QString& accentColor);

public:
    // Cell traversal 3(Bags)->4(Weight)->5(Rate)->6(Total, select-only)->
    // next visible row's 3. Enter commits+advances (auto-edits 3/4/5);
    // Left/Right move at text boundaries, skipping hidden filter rows.
    // row/col < 0 means "from the table's current cell".
    void advanceStockCell(int row, int col, int dir, bool editTarget);
    int nextVisibleRow(int row, int dir) const;
    QTableWidget* stockTable() const { return m_table; }

private:

    PrintExportController* m_printCtrl = nullptr;
    AccountingDateDisplay* m_dateEdit = nullptr;
    QLineEdit* m_searchBox = nullptr;
    QLabel* m_statusBadge = nullptr;
    QPushButton* m_btnPeriod = nullptr;
    QTableWidget* m_table = nullptr;

    QLabel* m_lblTotalItems = nullptr;
    QLabel* m_lblTotalBags = nullptr;
    QLabel* m_lblTotalWeight = nullptr;
    QLabel* m_lblTotalValuation = nullptr;

    KbdBadgeButton* m_btnAutoFill = nullptr;
    KbdBadgeButton* m_btnSaveLock = nullptr;
    KbdBadgeButton* m_btnDelete = nullptr;
    KbdBadgeButton* m_btnExportPdf = nullptr;
    KbdBadgeButton* m_btnExportCsv = nullptr;
    KbdBadgeButton* m_btnBack = nullptr;

    StockValuationReport m_currentReport;
    bool m_isUpdatingTable = false;
    bool m_isDirty = true;
};

} // namespace MahadevERP
