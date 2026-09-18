#pragma once

#include <QWidget>
#include <QTreeWidget>
#include <QLabel>
#include <QPushButton>
#include <QSplitter>
#include "accounting_date_edit.h"
#include "kbd_badge_button.h"
#include "../models/profit_loss_controller.h"
#include "../services/print_export_controller.h"

class ProfitLossWidget : public QWidget {
    Q_OBJECT

public:
    explicit ProfitLossWidget(ProfitLossController* controller,
                              PrintExportController* printExportCtrl,
                              QWidget* parent = nullptr);

    void refreshData(const QString& fromDateIso = "", const QString& toDateIso = "");
    void setDateRange(const QString& fromDateIso, const QString& toDateIso);

    QTreeWidget* expensesTree() const { return m_expensesTree; }
    QTreeWidget* incomesTree() const { return m_incomesTree; }
    void triggerDrillDownOnCurrentItem();

signals:
    void backRequested();
    void openPartyStatement(const QString& partyName, const QString& fromDate, const QString& toDate);
    void openStockRegisterRequested(const QString& itemName = "");
    void requestAccountingPeriodDialog();

public slots:
    void expandAllGroups();
    void collapseAllGroups();
    void exportPdf();
    void exportCsv();
    void printReport();
    void onDateFilterChanged();

protected:
    void keyPressEvent(QKeyEvent* event) override;
    bool eventFilter(QObject* watched, QEvent* event) override;
    void showEvent(QShowEvent* event) override;
    void paintEvent(QPaintEvent* event) override;

private slots:
    void onExpensesItemActivated(QTreeWidgetItem* item, int column);
    void onIncomesItemActivated(QTreeWidgetItem* item, int column);
    void populateTrees();

private:
    void setupUi();
    void applyCustomStyles();
    void handleItemDrillDown(QTreeWidgetItem* item);

    ProfitLossController* m_controller = nullptr;
    PrintExportController* m_printExportCtrl = nullptr;

    // Header controls
    QLabel* m_titleLabel = nullptr;
    QLabel* m_firmLabel = nullptr;
    QLabel* m_fyBadge = nullptr;
    AccountingDateEdit* m_fromDateEdit = nullptr;
    AccountingDateEdit* m_toDateEdit = nullptr;
    KbdBadgeButton* m_periodBtn = nullptr;
    KbdBadgeButton* m_expandBtn = nullptr;
    KbdBadgeButton* m_collapseBtn = nullptr;
    KbdBadgeButton* m_pdfBtn = nullptr;
    KbdBadgeButton* m_printBtn = nullptr;
    KbdBadgeButton* m_csvBtn = nullptr;
    KbdBadgeButton* m_backBtn = nullptr;

    // Main split trees
    QTreeWidget* m_expensesTree = nullptr;
    QTreeWidget* m_incomesTree = nullptr;
    QSplitter* m_splitter = nullptr;

    // Footer summary
    QLabel* m_grossProfitBadge = nullptr;
    QLabel* m_netProfitBadge = nullptr;
    QLabel* m_keyLegendLbl = nullptr;

    int m_lastExpIndex = 0;
    int m_lastIncIndex = 0;
};
