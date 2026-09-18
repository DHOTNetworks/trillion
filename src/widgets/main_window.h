#pragma once

#include <QMainWindow>
#include <QStackedWidget>
#include <QQuickWindow>
#include "dashboard_widget.h"
#include "ledger_statement_widget.h"
#include "balance_sheet_widget.h"
#include "profit_loss_widget.h"
#include "sales_voucher_widget.h"
#include "purchase_voucher_widget.h"
#include "firm_selector_widget.h"
#include "cheque_voucher_widget.h"
#include "journal_voucher_widget.h"
#include "new_ledger_widget.h"
#include "modify_ledger_widget.h"
#include "../models/ledger_statement_model.h"
#include "../models/balance_sheet_controller.h"
#include "../models/profit_loss_controller.h"
#include "../models/dashboard_controller.h"
#include "../models/firm_manager.h"
#include "../engine/bahi_khata_migrator.h"
#include "../services/print_export_controller.h"

class AppKeyboardController;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QQuickWindow* qmlWindow,
                        LedgerStatementController* ledgerCtrl,
                        PrintExportController* printExportCtrl,
                        DashboardController* dashCtrl = nullptr,
                        FirmManager* firmMgr = nullptr,
                        BahiKhataMigrator* migrator = nullptr,
                        QWidget* parent = nullptr);

    DashboardWidget* dashboardWidget() const { return m_dashboardWidget; }
    LedgerStatementWidget* ledgerWidget() const { return m_ledgerWidget; }
    BalanceSheetWidget* balanceSheetWidget() const { return m_balanceSheetWidget; }
    ProfitLossWidget* profitLossWidget() const { return m_profitLossWidget; }
    SalesVoucherWidget* salesVoucherWidget() const { return m_salesVoucherWidget; }
    PurchaseVoucherWidget* purchaseVoucherWidget() const { return m_purchaseVoucherWidget; }
    FirmSelectorWidget* firmSelectorWidget() const { return m_firmSelectorWidget; }
    ChequeVoucherWidget* chequeVoucherWidget() const { return m_chequeVoucherWidget; }
    JournalVoucherWidget* journalVoucherWidget() const { return m_journalVoucherWidget; }
    NewLedgerWidget* newLedgerWidget() const { return m_newLedgerWidget; }
    ModifyLedgerWidget* modifyLedgerWidget() const { return m_modifyLedgerWidget; }
    QStackedWidget* stackedWidget() const { return m_stackedWidget; }
    QWidget* qmlContainer() const { return m_qmlContainer; }
    QQuickWindow* qmlWindow() const { return m_qmlWindow; }

    int currentViewIndex() const;
    void restoreActiveViewFocus();
    AppKeyboardController* keyboardController() const { return m_keyboardCtrl; }

public slots:
    void navigateToView(int viewIndex);
    void checkQmlView();
    void openAccountingPeriodDialog();
    void onLedgerBackRequested();
    void onLedgerAlterVoucherRequested(int targetViewIndex, const QVariantMap& entry);
    void onBalanceSheetBackRequested();
    void onBalanceSheetPartyStatementRequested(const QString& partyName, const QString& fromDate, const QString& toDate);
    void onProfitLossBackRequested();
    void onProfitLossPartyStatementRequested(const QString& partyName, const QString& fromDate, const QString& toDate);
    void onSalesVoucherBackRequested();
    void onSalesVoucherSaved(const QString& invoiceNo);
    void onPurchaseVoucherBackRequested();
    void onPurchaseVoucherSaved(const QString& invoiceNo);
    void onChequeVoucherBackRequested();
    void onChequeVoucherSaved(const QString& voucherNo);
    void onJournalVoucherBackRequested();
    void onJournalVoucherSaved(const QString& voucherNo);
    void onNewLedgerBackRequested();
    void onNewLedgerSaved();
    void onModifyLedgerBackRequested();
    void onModifyLedgerSaved();

protected:
    void closeEvent(QCloseEvent* event) override;

private:
    QStackedWidget* m_stackedWidget = nullptr;
    DashboardWidget* m_dashboardWidget = nullptr;
    QWidget* m_qmlContainer = nullptr;
    QQuickWindow* m_qmlWindow = nullptr;
    LedgerStatementWidget* m_ledgerWidget = nullptr;
    BalanceSheetWidget* m_balanceSheetWidget = nullptr;
    ProfitLossWidget* m_profitLossWidget = nullptr;
    SalesVoucherWidget* m_salesVoucherWidget = nullptr;
    PurchaseVoucherWidget* m_purchaseVoucherWidget = nullptr;
    FirmSelectorWidget* m_firmSelectorWidget = nullptr;
    ChequeVoucherWidget* m_chequeVoucherWidget = nullptr;
    JournalVoucherWidget* m_journalVoucherWidget = nullptr;
    NewLedgerWidget* m_newLedgerWidget = nullptr;
    ModifyLedgerWidget* m_modifyLedgerWidget = nullptr;

    LedgerStatementController* m_ledgerCtrl = nullptr;
    BalanceSheetController* m_balanceSheetCtrl = nullptr;
    ProfitLossController* m_profitLossCtrl = nullptr;
    PrintExportController* m_printExportCtrl = nullptr;
    DashboardController* m_dashCtrl = nullptr;
    FirmManager* m_firmMgr = nullptr;
    BahiKhataMigrator* m_bahiKhataMigrator = nullptr;
    AppKeyboardController* m_keyboardCtrl = nullptr;
    int m_previousViewIndex = 0;
    bool m_isNavigating = false;
};

