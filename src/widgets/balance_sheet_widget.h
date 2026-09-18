#pragma once

#include <QWidget>
#include <QTreeWidget>
#include <QLabel>
#include <QPushButton>
#include <QSplitter>
#include "accounting_date_edit.h"
#include "kbd_badge_button.h"
#include "../models/balance_sheet_controller.h"
#include "../services/print_export_controller.h"

class BalanceSheetWidget : public QWidget {
    Q_OBJECT

public:
    explicit BalanceSheetWidget(BalanceSheetController* controller,
                                PrintExportController* printExportCtrl,
                                QWidget* parent = nullptr);

    void refreshData(const QString& asOnDateIso = "");
    void setAsOnDate(const QString& asOnDateIso);

    QTreeWidget* liabilitiesTree() const { return m_liabilitiesTree; }
    QTreeWidget* assetsTree() const { return m_assetsTree; }
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
    void onLiabilitiesItemActivated(QTreeWidgetItem* item, int column);
    void onAssetsItemActivated(QTreeWidgetItem* item, int column);
    void populateTrees();

private:
    void setupUi();
    void applyCustomStyles();
    void handleItemDrillDown(QTreeWidgetItem* item);

    BalanceSheetController* m_controller = nullptr;
    PrintExportController* m_printExportCtrl = nullptr;

    // Header controls
    QLabel* m_titleLabel = nullptr;
    QLabel* m_firmLabel = nullptr;
    QLabel* m_fyBadge = nullptr;
    AccountingDateEdit* m_asOnDateEdit = nullptr;
    KbdBadgeButton* m_periodBtn = nullptr;
    KbdBadgeButton* m_expandBtn = nullptr;
    KbdBadgeButton* m_collapseBtn = nullptr;
    KbdBadgeButton* m_pdfBtn = nullptr;
    KbdBadgeButton* m_printBtn = nullptr;
    KbdBadgeButton* m_csvBtn = nullptr;
    KbdBadgeButton* m_backBtn = nullptr;

    // Main split trees
    QLabel* m_liabHeaderLbl = nullptr;
    QLabel* m_assetHeaderLbl = nullptr;
    QTreeWidget* m_liabilitiesTree = nullptr;
    QTreeWidget* m_assetsTree = nullptr;
    QSplitter* m_splitter = nullptr;

    // Footer summary
    QLabel* m_liabilitiesTotalLbl = nullptr;
    QLabel* m_assetsTotalLbl = nullptr;
    QLabel* m_balanceStatusBadge = nullptr;
    QLabel* m_keyLegendLbl = nullptr;

    int m_lastLiabIndex = 0;
    int m_lastAssetIndex = 0;
};
