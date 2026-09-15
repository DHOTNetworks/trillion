#pragma once

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QVariantMap>
#include "account_search_box.h"
#include "accounting_date_edit.h"
#include "ledger_table_view.h"
#include "kbd_badge_button.h"
#include "../models/ledger_statement_model.h"
#include "../services/print_export_controller.h"

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

signals:
    void backRequested();
    void alterVoucherRequested(int targetViewIndex, const QVariantMap& entry);

public slots:
    void openSelectedVoucher();
    void printStatement();
    void exportPdf();
    void exportCsv();
    void focusSearch();

protected:
    void keyPressEvent(QKeyEvent* event) override;
    void showEvent(QShowEvent* event) override;
    void paintEvent(QPaintEvent* event) override;

private slots:
    void onPartySelected(const QString& partyName);
    void onDateFilterApplied();
    void onTotalsChanged();
    void onVoucherActivated(const QVariantMap& entry);
    void onSwitchSideRequested(const QString& targetSide);

private:
    void setupUi();
    void updateHeadersAndTotals();
    void openVoucherForEntry(const QVariantMap& entry);

    LedgerStatementController* m_controller = nullptr;
    PrintExportController* m_printExportCtrl = nullptr;

    // Header Actions
    KbdBadgeButton* m_alterBtn = nullptr;
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

    // Side Banners & Tables
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

    // State Tracking
    QString m_lastSide = "Dr";
    int m_lastIndex = 0;
};
