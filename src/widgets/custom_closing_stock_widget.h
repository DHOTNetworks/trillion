#pragma once

#include <QWidget>
#include <QTableWidget>
#include <QLabel>
#include <QPushButton>
#include <QLineEdit>
#include <QDateEdit>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QFrame>
#include "kbd_badge_button.h"
#include "../services/print_export_controller.h"
#include "../engine/stock_valuation_engine.h"

namespace MahadevERP {

class CustomClosingStockWidget : public QWidget {
    Q_OBJECT

public:
    explicit CustomClosingStockWidget(PrintExportController* printCtrl, QWidget* parent = nullptr);
    void reloadData();

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

private:
    void setupUi();
    void applyCustomStyles();
    void updateSummaryCards();
    void populateTable(const StockValuationReport& report);
    QWidget* createMetricCard(const QString& title, QLabel*& valueLabel, const QString& accentColor);

    PrintExportController* m_printCtrl = nullptr;
    QDateEdit* m_dateEdit = nullptr;
    QLabel* m_statusBadge = nullptr;
    QLabel* m_fyBadge = nullptr;
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
};

} // namespace MahadevERP
