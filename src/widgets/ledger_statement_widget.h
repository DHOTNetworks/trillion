#pragma once

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QVariantMap>
#include <QTabWidget>
#include <QTableWidget>
#include <QDoubleSpinBox>
#include <QCheckBox>
#include "account_search_box.h"
#include "accounting_date_edit.h"
#include "ledger_table_view.h"
#include "kbd_badge_button.h"
#include "../models/ledger_statement_model.h"
#include "../services/print_export_controller.h"
#include "../engine/aank_interest_engine.h"

class LedgerStatementWidget : public QWidget {
    Q_OBJECT

public:
    explicit LedgerStatementWidget(LedgerStatementController* controller,
                                  PrintExportController* printExportCtrl,
                                  QWidget* parent = nullptr);

    void loadParty(const QString& partyName, const QString& fromDate = "", const QString& toDate = "");
    void restoreState(const QString& partyName, const QString& fromDate, const QString& toDate,
                      const QString& side, int rowIndex);
    void resetSearch();

    QString currentParty() const;
    QString fromDate() const;
    QString toDate() const;
    QString lastSide() const { return m_lastSide; }
    int lastIndex() const { return m_lastIndex; }

    LedgerTableView* drTable() const { return m_drTable; }
    LedgerTableView* crTable() const { return m_crTable; }
    AccountSearchBox* searchBox() const { return m_searchBox; }

signals:
    void backRequested();
    void alterVoucherRequested(int targetViewIndex, const QVariantMap& entry);

public slots:
    void openSelectedVoucher();
    void printStatement();
    void exportPdf();
    void exportOdf();
    void exportExcel();
    void exportCsv();
    void focusSearch();
    void onTotalsChanged();
    void onSwitchSideRequested(const QString& targetSide);
    void recalculateAankStatement();
    void onPostInterestVoucherClicked();
    void toggleAankMode();

protected:
    void keyPressEvent(QKeyEvent* event) override;
    void showEvent(QShowEvent* event) override;
    void paintEvent(QPaintEvent* event) override;

private slots:
    void onPartySelected(const QString& partyName);
    void onDateFilterApplied();
    void onVoucherActivated(const QVariantMap& entry);

private:
    void setupUi();
    void updateHeadersAndTotals();
    void openVoucherForEntry(const QVariantMap& entry);

    LedgerStatementController* m_controller = nullptr;
    PrintExportController* m_printExportCtrl = nullptr;

    // Header Actions
    KbdBadgeButton* m_toggleAankBtn = nullptr;
    KbdBadgeButton* m_printBtn = nullptr;
    KbdBadgeButton* m_pdfBtn = nullptr;
    KbdBadgeButton* m_csvBtn = nullptr;
    KbdBadgeButton* m_backBtn = nullptr;

    // Search & Filter
    AccountSearchBox* m_searchBox = nullptr;
    QLabel* m_fyBadge = nullptr;
    AccountingDateEdit* m_fromDateEdit = nullptr;
    AccountingDateEdit* m_toDateEdit = nullptr;
    QPushButton* m_applyFilterBtn = nullptr;

    // Tab Container
    QTabWidget* m_viewTabs = nullptr;

    // --- Tab 1: 2-Column Ledger ---
    QWidget* m_twoColumnWidget = nullptr;
    QLabel* m_crHeaderLabel = nullptr;
    QLabel* m_drHeaderLabel = nullptr;
    LedgerTableView* m_crTable = nullptr;
    LedgerTableView* m_drTable = nullptr;

    // Footers
    QLabel* m_crTotalLabel = nullptr;
    QLabel* m_crCheckedLabel = nullptr;
    QLabel* m_drTotalLabel = nullptr;
    QLabel* m_drCheckedLabel = nullptr;

    // Bottom Net Bar
    QLabel* m_netDiffLabel = nullptr;
    QLabel* m_netBalanceLabel = nullptr;
    QLabel* m_checkedDiffLabel = nullptr;

    // --- Tab 2: Aank Statement ---
    QWidget* m_aankWidget = nullptr;
    QTableWidget* m_aankTable = nullptr;
    QDoubleSpinBox* m_drInterestRateSpin = nullptr;
    QDoubleSpinBox* m_crInterestRateSpin = nullptr;
    QCheckBox* m_leapYearDivisorCheck = nullptr;
    KbdBadgeButton* m_recomputeAankBtn = nullptr;
    KbdBadgeButton* m_postInterestVoucherBtn = nullptr;

    QLabel* m_aankTotalDrLabel = nullptr;
    QLabel* m_aankTotalCrLabel = nullptr;
    QLabel* m_aankDrInterestLabel = nullptr;
    QLabel* m_aankCrInterestLabel = nullptr;
    QLabel* m_aankNetInterestLabel = nullptr;

    MahadevERP::AankInterestStatement m_currentAankStatement;

    // State Tracking
    QString m_lastSide = "Dr";
    int m_lastIndex = 0;
};

