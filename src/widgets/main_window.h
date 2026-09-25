#pragma once

#include <QMainWindow>
#include <QStackedWidget>
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
#include "jform_voucher_widget.h"
#include "iform_voucher_widget.h"
#include "mandi_reports_widget.h"
#include "day_book_widget.h"
#include "gstr_reports_widget.h"
#include "milling_statement_widget.h"
#include "custom_closing_stock_widget.h"
#include "new_group_widget.h"
#include "modify_group_widget.h"
#include "new_stock_item_widget.h"
#include "modify_stock_item_widget.h"
#include "sales_register_widget.h"
#include "purchase_register_widget.h"
#include "stock_detail_widget.h"
#include "paddy_procurement_widget.h"
#include "milling_voucher_widget.h"
#include "tds_voucher_widget.h"
#include "interest_calculator_widget.h"
#include "bank_statement_import_widget.h"
#include "transport_dispatch_widget.h"
#include "debit_credit_note_widget.h"
#include "tds_tcs_hub_dialog.h"
#include "tds_options_dialog.h"
#include "tcs_options_dialog.h"
#include "../models/menu_tree_manager.h"
#include "tax_challan_creation_widget.h"
#include "tax_challan_register_widget.h"
#include "tcs_receipt_voucher_widget.h"
#include "tcs_receipt_list_widget.h"
#include "tds_vouchers_list_widget.h"
#include "advance_payment_194q_widget.h"
#include "advance_payment_194q_list_widget.h"
#include "form16a_entry_dialog.h"
#include "form16a_list_widget.h"
#include "tds_acknowledgement_dialog.h"
#include "tcs_config_dialog.h"
#include "trial_balance_widget.h"
#include "capital_accounts_widget.h"
#include "depreciation_chart_widget.h"
#include "joint_reports_dialog.h"
#include "../models/trial_balance_controller.h"
#include "../models/capital_accounts_controller.h"
#include "../engine/depreciation_calculator.h"
#include "../models/tax_challan_controller.h"
#include "../models/tcs_receipt_voucher_controller.h"
#include "../models/ledger_statement_model.h"
#include "../models/balance_sheet_controller.h"
#include "../models/profit_loss_controller.h"
#include "../models/dashboard_controller.h"
#include "../models/firm_manager.h"
#include "../models/account_groups_model.h"
#include "../models/stock_master_controller.h"
#include "../models/stock_items_model.h"
#include "../models/sales_register_model.h"
#include "../models/purchase_register_model.h"
#include "../models/stock_register_model.h"
#include "../models/paddy_arrivals_model.h"
#include "../models/paddy_procurement_controller.h"
#include "../models/milling_batch_controller.h"
#include "../models/milling_model.h"
#include "../models/tds_voucher_controller.h"
#include "../models/interest_model.h"
#include "../models/bank_statement_controller.h"
#include "../models/transport_dispatch_controller.h"
#include "../models/debit_credit_note_controller.h"
#include "../engine/bahi_khata_migrator.h"
#include "../services/print_export_controller.h"

class AppKeyboardController;

struct MainWindowDependencies {
    DashboardController* dashCtrl = nullptr;
    FirmManager* firmMgr = nullptr;
    PrintExportController* printExportCtrl = nullptr;
    BahiKhataMigrator* migrator = nullptr;
    LedgerStatementController* ledgerCtrl = nullptr;
    AccountGroupsModel* groupsModel = nullptr;
    StockMasterController* stockMasterCtrl = nullptr;
    StockItemsModel* stockItemsModel = nullptr;
    SalesRegisterController* salesRegisterCtrl = nullptr;
    PurchaseRegisterController* purchaseRegisterCtrl = nullptr;
    StockRegisterController* stockRegisterCtrl = nullptr;
    PaddyArrivalsModel* paddyModel = nullptr;
    PaddyProcurementController* paddyProcurementCtrl = nullptr;
    MillingBatchController* millingBatchCtrl = nullptr;
    MillingModel* millingModel = nullptr;
    TdsVoucherController* tdsVoucherCtrl = nullptr;
    InterestModel* interestModel = nullptr;
    BankStatementController* bankStatementCtrl = nullptr;
    TransportDispatchController* transportDispatchCtrl = nullptr;
    DebitCreditNoteController* debitCreditNoteCtrl = nullptr;
};

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(const MainWindowDependencies& deps, QWidget* parent = nullptr);

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
    JFormVoucherWidget* jformVoucherWidget() const { return m_jformVoucherWidget; }
    IFormVoucherWidget* iformVoucherWidget() const { return m_iformVoucherWidget; }
    MandiReportsWidget* mandiReportsWidget() const { return m_mandiReportsWidget; }
    MahadevERP::DayBookWidget* dayBookWidget() const { return m_dayBookWidget; }
    MahadevERP::GstrReportsWidget* gstrReportsWidget() const { return m_gstrReportsWidget; }
    MahadevERP::MillingStatementWidget* millingStatementWidget() const { return m_millingStatementWidget; }
    MahadevERP::CustomClosingStockWidget* customClosingStockWidget() const { return m_customClosingStockWidget; }
    MahadevERP::NewGroupWidget* newGroupWidget() const { return m_newGroupWidget; }
    MahadevERP::ModifyGroupWidget* modifyGroupWidget() const { return m_modifyGroupWidget; }
    MahadevERP::NewStockItemWidget* newStockItemWidget() const { return m_newStockItemWidget; }
    MahadevERP::ModifyStockItemWidget* modifyStockItemWidget() const { return m_modifyStockItemWidget; }
    MahadevERP::SalesRegisterWidget* salesRegisterWidget() const { return m_salesRegisterWidget; }
    MahadevERP::PurchaseRegisterWidget* purchaseRegisterWidget() const { return m_purchaseRegisterWidget; }
    MahadevERP::StockDetailWidget* stockDetailWidget() const { return m_stockDetailWidget; }
    MahadevERP::PaddyProcurementWidget* paddyProcurementWidget() const { return m_paddyProcurementWidget; }
    MahadevERP::MillingVoucherWidget* millingVoucherWidget() const { return m_millingVoucherWidget; }
    MahadevERP::TdsVoucherWidget* tdsVoucherWidget() const { return m_tdsVoucherWidget; }
    MahadevERP::InterestCalculatorWidget* interestCalculatorWidget() const { return m_interestCalcWidget; }
    MahadevERP::BankStatementImportWidget* bankStatementWidget() const { return m_bankStatementWidget; }
    MahadevERP::TransportDispatchWidget* transportDispatchWidget() const { return m_transportDispatchWidget; }
    MahadevERP::DebitCreditNoteWidget* debitCreditNoteWidget() const { return m_debitCreditNoteWidget; }
    MahadevERP::TrialBalanceWidget* trialBalanceWidget() const { return m_trialBalanceWidget; }
    MahadevERP::CapitalAccountsWidget* capitalAccountsWidget() const { return m_capitalAccountsWidget; }
    MahadevERP::DepreciationChartWidget* depreciationChartWidget() const { return m_depreciationChartWidget; }

    QStackedWidget* stackedWidget() const { return m_stackedWidget; }

    int currentViewIndex() const;
    void restoreActiveViewFocus();
    AppKeyboardController* keyboardController() const { return m_keyboardCtrl; }

public slots:
    void navigateToView(int viewIndex);
    void openAccountingPeriodDialog();
    void openStatementForParty(const QString& partyName);
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
    void onJFormVoucherBackRequested();
    void onJFormVoucherSaved(const QString& jformNo);
    void onIFormVoucherBackRequested();
    void onIFormVoucherSaved(const QString& iformNo);
    void onMandiReportsBackRequested();

protected:
    void closeEvent(QCloseEvent* event) override;

private:
    QStackedWidget* m_stackedWidget = nullptr;
    DashboardWidget* m_dashboardWidget = nullptr;
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
    JFormVoucherWidget* m_jformVoucherWidget = nullptr;
    IFormVoucherWidget* m_iformVoucherWidget = nullptr;
    MandiReportsWidget* m_mandiReportsWidget = nullptr;
    MahadevERP::DayBookWidget* m_dayBookWidget = nullptr;
    MahadevERP::GstrReportsWidget* m_gstrReportsWidget = nullptr;
    MahadevERP::MillingStatementWidget* m_millingStatementWidget = nullptr;
    MahadevERP::CustomClosingStockWidget* m_customClosingStockWidget = nullptr;

    MahadevERP::NewGroupWidget* m_newGroupWidget = nullptr;
    MahadevERP::ModifyGroupWidget* m_modifyGroupWidget = nullptr;
    MahadevERP::NewStockItemWidget* m_newStockItemWidget = nullptr;
    MahadevERP::ModifyStockItemWidget* m_modifyStockItemWidget = nullptr;
    MahadevERP::SalesRegisterWidget* m_salesRegisterWidget = nullptr;
    MahadevERP::PurchaseRegisterWidget* m_purchaseRegisterWidget = nullptr;
    MahadevERP::StockDetailWidget* m_stockDetailWidget = nullptr;
    MahadevERP::PaddyProcurementWidget* m_paddyProcurementWidget = nullptr;
    MahadevERP::MillingVoucherWidget* m_millingVoucherWidget = nullptr;
    MahadevERP::TdsVoucherWidget* m_tdsVoucherWidget = nullptr;
    MahadevERP::InterestCalculatorWidget* m_interestCalcWidget = nullptr;
    MahadevERP::BankStatementImportWidget* m_bankStatementWidget = nullptr;
    MahadevERP::TransportDispatchWidget* m_transportDispatchWidget = nullptr;
    MahadevERP::DebitCreditNoteWidget* m_debitCreditNoteWidget = nullptr;
    MahadevERP::TdsVouchersListWidget* m_tdsVouchersListWidget = nullptr;
    MahadevERP::TaxChallanCreationWidget* m_taxChallanCreationWidget = nullptr;
    MahadevERP::TaxChallanRegisterWidget* m_taxChallanRegisterWidget = nullptr;
    MahadevERP::TcsReceiptVoucherWidget* m_tcsReceiptVoucherWidget = nullptr;
    MahadevERP::TcsReceiptListWidget* m_tcsReceiptListWidget = nullptr;
    MahadevERP::AdvancePayment194QWidget* m_advancePayment194QWidget = nullptr;
    MahadevERP::AdvancePayment194QListWidget* m_advancePayment194QListWidget = nullptr;
    MahadevERP::Form16AListWidget* m_form16AListWidget = nullptr;
    MahadevERP::TrialBalanceWidget* m_trialBalanceWidget = nullptr;
    MahadevERP::CapitalAccountsWidget* m_capitalAccountsWidget = nullptr;
    MahadevERP::DepreciationChartWidget* m_depreciationChartWidget = nullptr;

    TaxChallanController* m_taxChallanCtrl = nullptr;
    TcsReceiptVoucherController* m_tcsReceiptVoucherCtrl = nullptr;
    MahadevERP::TrialBalanceController* m_trialBalanceCtrl = nullptr;
    MahadevERP::CapitalAccountsController* m_capitalAccountsCtrl = nullptr;

    void openTdsTcsHub();
    void openTdsOptions();
    void openTcsOptions();

    LedgerStatementController* m_ledgerCtrl = nullptr;
    BalanceSheetController* m_balanceSheetCtrl = nullptr;
    ProfitLossController* m_profitLossCtrl = nullptr;
    PrintExportController* m_printExportCtrl = nullptr;
    DashboardController* m_dashCtrl = nullptr;
    FirmManager* m_firmMgr = nullptr;
    BahiKhataMigrator* m_bahiKhataMigrator = nullptr;
    AccountGroupsModel* m_groupsModel = nullptr;
    StockMasterController* m_stockMasterCtrl = nullptr;
    StockItemsModel* m_stockItemsModel = nullptr;
    SalesRegisterController* m_salesRegisterCtrl = nullptr;
    PurchaseRegisterController* m_purchaseRegisterCtrl = nullptr;
    StockRegisterController* m_stockRegisterCtrl = nullptr;
    PaddyArrivalsModel* m_paddyModel = nullptr;
    PaddyProcurementController* m_paddyProcurementCtrl = nullptr;
    MillingBatchController* m_millingBatchCtrl = nullptr;
    MillingModel* m_millingModel = nullptr;
    TdsVoucherController* m_tdsVoucherCtrl = nullptr;
    InterestModel* m_interestModel = nullptr;
    BankStatementController* m_bankStatementCtrl = nullptr;
    TransportDispatchController* m_transportDispatchCtrl = nullptr;
    DebitCreditNoteController* m_debitCreditNoteCtrl = nullptr;

    AppKeyboardController* m_keyboardCtrl = nullptr;
    int m_previousViewIndex = 0;
    bool m_isNavigating = false;

    // Navigation Transfer State
    QString m_pendingEditInvoiceNo;
    QString m_pendingEditVoucherNo;
    int m_pendingEditVoucherId = 0;
    QString m_pendingEditVoucherDate;
    QString m_targetChequeMode;
    QString m_targetStatementParty;
    QString m_lastViewedStatementFromDate;
    QString m_lastViewedStatementToDate;
    QString m_lastViewedStatementSide;
    int m_lastViewedStatementIndex = 0;
};
