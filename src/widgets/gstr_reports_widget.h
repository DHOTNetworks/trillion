#pragma once

#include <QWidget>
#include <QTabWidget>
#include <QTableWidget>
#include <QComboBox>
#include "accounting_date_edit.h"
#include <QLineEdit>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include "../engine/gstr1_engine.h"
#include "../engine/gstr2_reconciler.h"
#include "../engine/gstr3b_engine.h"
#include "../services/print_export_controller.h"

namespace MahadevERP {

class GstrReportsWidget : public QWidget {
    Q_OBJECT

public:
    explicit GstrReportsWidget(PrintExportController* printExportCtrl = nullptr, QWidget* parent = nullptr);

    void loadReturns(const QDate& fromDate, const QDate& toDate);
    void markDirty() { m_isDirty = true; }
    bool isDirty() const { return m_isDirty; }

signals:
    void backRequested();

protected:
    void keyPressEvent(QKeyEvent* event) override;

private slots:
    void onRefreshClicked();
    void onFyMonthChanged();
    void onExportGstr1JsonClicked();
    void onExportGstr1ExcelClicked();
    void onImportGstr2AJsonClicked();
    void onAutoDownloadGstr2Clicked();
    void onExportGstr3BExcelClicked();

private:
    void setupUi();
    void applyCustomStyles();
    void populateGstr1Tab(const Gstr1ReturnPayload& payload);
    void populateGstr3BTab(const Gstr3BReturnSummary& summary);
    void populateGstr2Tab(const Gstr2ReconciliationSummary& summary);

    QTabWidget* m_tabs = nullptr;
    AccountingDateDisplay* m_fromDateEdit = nullptr;
    AccountingDateDisplay* m_toDateEdit = nullptr;
    QComboBox* m_monthCombo = nullptr; // FY-ordered April..March (index 0..11)
    QComboBox* m_fyCombo = nullptr;    // selected FY owns the year (Jan-Mar auto-roll)
    void syncFyMonthCombos(const QDate& fromDate, const QDate& toDate);
    QPushButton* m_refreshBtn = nullptr;
    QPushButton* m_backBtn = nullptr;

    // GSTR-1 Widgets
    QLabel* m_gstr1TurnoverLabel = nullptr;
    QLabel* m_gstr1B2bCountLabel = nullptr;
    QLabel* m_gstr1B2csCountLabel = nullptr;
    QLabel* m_gstr1HsnCountLabel = nullptr;
    QLabel* m_gstr1TotalTaxLabel = nullptr;
    QTableWidget* m_gstr1B2BTable = nullptr;
    QLabel* m_gstr1ExemptLabel = nullptr;
    QLabel* m_gstr1QuarLabel = nullptr;
    QLabel* m_fyBadge = nullptr;

    // GSTR-2A Matching Widgets
    QTableWidget* m_gstr2Table = nullptr;
    QLabel* m_gstr2MatchedLabel = nullptr;
    QLabel* m_gstr2MismatchLabel = nullptr;
    QLabel* m_gstr2NotInPortalLabel = nullptr;
    QPushButton* m_importGstr2Btn = nullptr;
    QPushButton* m_autoDownloadGstr2Btn = nullptr;

    // GSTR-3B Widgets
    QLabel* m_gstr3bBanner = nullptr;
    QLabel* m_gstr3bNetPayableBanner = nullptr;
    QTableWidget* m_gstr3bTable31 = nullptr;
    QTableWidget* m_gstr3bTable32 = nullptr;
    QTableWidget* m_gstr3bTable4 = nullptr;
    QTableWidget* m_gstr3bTable5 = nullptr;
    QTableWidget* m_gstr3bTable61 = nullptr;
    QPushButton* m_exportGstr3BBtn = nullptr;

    PrintExportController* m_printExportCtrl = nullptr;
    Gstr1ReturnPayload m_currentGstr1Payload;
    Gstr3BReturnSummary m_currentGstr3BSummary;
    QList<Gstr2PortalRecord> m_loadedPortalRecords;
    QString m_loadedPortalPeriod;
    bool m_isDirty = true;
};

} // namespace MahadevERP

