#include "main_window.h"
#include "accounting_period_dialog.h"
#include "../engine/fiscal_year_helper.h"
#include "../services/app_keyboard_controller.h"
#include <QTimer>
#include <QShortcut>
#include <QDebug>

MainWindow::MainWindow(const MainWindowDependencies& deps, QWidget* parent)
    : QMainWindow(parent)
    , m_ledgerCtrl(deps.ledgerCtrl)
    , m_printExportCtrl(deps.printExportCtrl)
    , m_dashCtrl(deps.dashCtrl)
    , m_firmMgr(deps.firmMgr)
    , m_bahiKhataMigrator(deps.migrator)
    , m_groupsModel(deps.groupsModel)
    , m_stockMasterCtrl(deps.stockMasterCtrl)
    , m_stockItemsModel(deps.stockItemsModel)
    , m_salesRegisterCtrl(deps.salesRegisterCtrl)
    , m_purchaseRegisterCtrl(deps.purchaseRegisterCtrl)
    , m_stockRegisterCtrl(deps.stockRegisterCtrl)
    , m_paddyModel(deps.paddyModel)
    , m_paddyProcurementCtrl(deps.paddyProcurementCtrl)
    , m_millingBatchCtrl(deps.millingBatchCtrl)
    , m_millingModel(deps.millingModel)
    , m_tdsVoucherCtrl(deps.tdsVoucherCtrl)
    , m_interestModel(deps.interestModel)
    , m_bankStatementCtrl(deps.bankStatementCtrl)
    , m_transportDispatchCtrl(deps.transportDispatchCtrl)
    , m_debitCreditNoteCtrl(deps.debitCreditNoteCtrl)
{
    setWindowTitle("Mahadev Rice Mill ERP & Accounting");
    resize(1280, 800);
    setMinimumSize(1024, 680);
    setStyleSheet(
        "QMainWindow { background-color: #F8FAFC; } "
        "QStackedWidget { background-color: #F8FAFC; } "
        "QComboBox QAbstractItemView, QComboBox QListView { "
        "  background-color: #FFFFFF; "
        "  color: #0F172A; "
        "  border: 1px solid #CBD5E1; "
        "  border-radius: 4px; "
        "  selection-background-color: #2563EB; "
        "  selection-color: #FFFFFF; "
        "  outline: none; "
        "  font-size: 12px; "
        "  padding: 2px; "
        "} "
        "QComboBox QAbstractItemView::item, QComboBox QListView::item { "
        "  color: #0F172A; "
        "  padding: 5px 8px; "
        "  min-height: 22px; "
        "} "
        "QComboBox QAbstractItemView::item:hover, QComboBox QListView::item:hover { "
        "  background-color: #EFF6FF; "
        "  color: #1D4ED8; "
        "} "
        "QComboBox QAbstractItemView::item:selected, QComboBox QListView::item:selected { "
        "  background-color: #2563EB; "
        "  color: #FFFFFF; "
        "}"
    );

    m_stackedWidget = new QStackedWidget(this);
    m_stackedWidget->setAttribute(Qt::WA_StyledBackground, true);
    setCentralWidget(m_stackedWidget);

    // Index 0: Native C++ Main Dashboard Widget (View 0)
    m_dashboardWidget = new DashboardWidget(m_dashCtrl, m_firmMgr, m_printExportCtrl, m_bahiKhataMigrator, this);
    connect(m_dashboardWidget, &DashboardWidget::openViewRequested, this, &MainWindow::navigateToView);
    connect(m_dashboardWidget, &DashboardWidget::requestAccountingPeriodDialog, this, &MainWindow::openAccountingPeriodDialog);
    m_stackedWidget->addWidget(m_dashboardWidget);

    // Index 1: Native C++ Ledger Statement Widget (View 8)
    m_ledgerWidget = new LedgerStatementWidget(m_ledgerCtrl, m_printExportCtrl, this);
    connect(m_ledgerWidget, &LedgerStatementWidget::backRequested, this, &MainWindow::onLedgerBackRequested);
    connect(m_ledgerWidget, &LedgerStatementWidget::alterVoucherRequested, this, &MainWindow::onLedgerAlterVoucherRequested);
    m_stackedWidget->addWidget(m_ledgerWidget);

    // Index 2: Native C++ Balance Sheet Widget (View 29)
    m_balanceSheetCtrl = new BalanceSheetController(m_printExportCtrl, this);
    m_balanceSheetWidget = new BalanceSheetWidget(m_balanceSheetCtrl, m_printExportCtrl, this);
    connect(m_balanceSheetWidget, &BalanceSheetWidget::backRequested, this, &MainWindow::onBalanceSheetBackRequested);
    connect(m_balanceSheetWidget, &BalanceSheetWidget::openPartyStatement, this, &MainWindow::onBalanceSheetPartyStatementRequested);
    connect(m_balanceSheetWidget, &BalanceSheetWidget::openStockRegisterRequested, this, [this](const QString& itemName) {
        Q_UNUSED(itemName);
        m_previousViewIndex = 29;
        navigateToView(13); // Stock Summary
    });
    connect(m_balanceSheetWidget, &BalanceSheetWidget::requestAccountingPeriodDialog, this, &MainWindow::openAccountingPeriodDialog);
    m_stackedWidget->addWidget(m_balanceSheetWidget);

    // Index 3: Native C++ Profit & Loss Widget (View 30)
    m_profitLossCtrl = new ProfitLossController(m_printExportCtrl, this);
    m_profitLossWidget = new ProfitLossWidget(m_profitLossCtrl, m_printExportCtrl, this);
    connect(m_profitLossWidget, &ProfitLossWidget::backRequested, this, &MainWindow::onProfitLossBackRequested);
    connect(m_profitLossWidget, &ProfitLossWidget::openPartyStatement, this, &MainWindow::onProfitLossPartyStatementRequested);
    connect(m_profitLossWidget, &ProfitLossWidget::openStockRegisterRequested, this, [this](const QString& itemName) {
        Q_UNUSED(itemName);
        m_previousViewIndex = 30;
        navigateToView(13); // Stock Summary
    });
    connect(m_profitLossWidget, &ProfitLossWidget::requestAccountingPeriodDialog, this, &MainWindow::openAccountingPeriodDialog);
    m_stackedWidget->addWidget(m_profitLossWidget);

    // Index 4: Native C++ Sales Voucher Widget (View 14)
    m_salesVoucherWidget = new SalesVoucherWidget(m_printExportCtrl, this);
    connect(m_salesVoucherWidget, &SalesVoucherWidget::backRequested, this, &MainWindow::onSalesVoucherBackRequested);
    connect(m_salesVoucherWidget, &SalesVoucherWidget::invoiceSaved, this, &MainWindow::onSalesVoucherSaved);
    m_stackedWidget->addWidget(m_salesVoucherWidget);

    // Index 5: Native C++ Purchase Voucher Widget (View 15)
    m_purchaseVoucherWidget = new PurchaseVoucherWidget(m_printExportCtrl, this);
    connect(m_purchaseVoucherWidget, &PurchaseVoucherWidget::backRequested, this, &MainWindow::onPurchaseVoucherBackRequested);
    connect(m_purchaseVoucherWidget, &PurchaseVoucherWidget::invoiceSaved, this, &MainWindow::onPurchaseVoucherSaved);
    m_stackedWidget->addWidget(m_purchaseVoucherWidget);

    // Index 6: Native C++ Firm Selector Widget (View 22)
    m_firmSelectorWidget = new FirmSelectorWidget(m_firmMgr, m_bahiKhataMigrator, this);
    connect(m_firmSelectorWidget, &FirmSelectorWidget::firmOpened, this, [this](const QString& firmId, const QString& firmName) {
        Q_UNUSED(firmId);
        Q_UNUSED(firmName);
        if (m_dashboardWidget) {
            m_dashboardWidget->refreshStats();
        }
        navigateToView(0);
    });
    connect(m_firmSelectorWidget, &FirmSelectorWidget::backRequested, this, [this]() {
        navigateToView(0);
    });
    m_stackedWidget->addWidget(m_firmSelectorWidget);

    // Index 7: Native C++ Cheque Voucher Widget (View 16 - Payment & Receipt)
    m_chequeVoucherWidget = new ChequeVoucherWidget(m_printExportCtrl, this);
    connect(m_chequeVoucherWidget, &ChequeVoucherWidget::backRequested, this, &MainWindow::onChequeVoucherBackRequested);
    connect(m_chequeVoucherWidget, &ChequeVoucherWidget::voucherSaved, this, &MainWindow::onChequeVoucherSaved);
    m_stackedWidget->addWidget(m_chequeVoucherWidget);

    // Index 8: Native C++ Journal Voucher Widget (View 17)
    m_journalVoucherWidget = new JournalVoucherWidget(m_printExportCtrl, this);
    connect(m_journalVoucherWidget, &JournalVoucherWidget::backRequested, this, &MainWindow::onJournalVoucherBackRequested);
    connect(m_journalVoucherWidget, &JournalVoucherWidget::voucherSaved, this, &MainWindow::onJournalVoucherSaved);
    m_stackedWidget->addWidget(m_journalVoucherWidget);

    // Index 9: Native C++ New Ledger Widget (View 6)
    m_newLedgerWidget = new NewLedgerWidget(this);
    connect(m_newLedgerWidget, &NewLedgerWidget::backRequested, this, &MainWindow::onNewLedgerBackRequested);
    connect(m_newLedgerWidget, &NewLedgerWidget::savedSuccess, this, &MainWindow::onNewLedgerSaved);
    m_stackedWidget->addWidget(m_newLedgerWidget);

    // Index 10: Native C++ Modify Ledger Widget (View 7)
    m_modifyLedgerWidget = new ModifyLedgerWidget(this);
    connect(m_modifyLedgerWidget, &ModifyLedgerWidget::backRequested, this, &MainWindow::onModifyLedgerBackRequested);
    connect(m_modifyLedgerWidget, &ModifyLedgerWidget::savedSuccess, this, &MainWindow::onModifyLedgerSaved);
    m_stackedWidget->addWidget(m_modifyLedgerWidget);

    // Index 11: Native C++ J-Form Mandi Procurement Widget (View 18)
    m_jformVoucherWidget = new JFormVoucherWidget(m_printExportCtrl, this);
    connect(m_jformVoucherWidget, &JFormVoucherWidget::backRequested, this, &MainWindow::onJFormVoucherBackRequested);
    connect(m_jformVoucherWidget, &JFormVoucherWidget::voucherSaved, this, &MainWindow::onJFormVoucherSaved);
    m_stackedWidget->addWidget(m_jformVoucherWidget);

    // Index 12: Native C++ I-Form Mandi Buyer Issue Widget (View 19)
    m_iformVoucherWidget = new IFormVoucherWidget(m_printExportCtrl, this);
    connect(m_iformVoucherWidget, &IFormVoucherWidget::backRequested, this, &MainWindow::onIFormVoucherBackRequested);
    connect(m_iformVoucherWidget, &IFormVoucherWidget::voucherSaved, this, &MainWindow::onIFormVoucherSaved);
    m_stackedWidget->addWidget(m_iformVoucherWidget);

    // Index 13: Native C++ Mandi Operations & Reports Widget (View 20)
    m_mandiReportsWidget = new MandiReportsWidget(m_printExportCtrl, this);
    connect(m_mandiReportsWidget, &MandiReportsWidget::backRequested, this, &MainWindow::onMandiReportsBackRequested);
    m_stackedWidget->addWidget(m_mandiReportsWidget);

    // Index 14: Native C++ Day Book Widget (View 33)
    m_dayBookWidget = new MahadevERP::DayBookWidget(m_printExportCtrl, this);
    connect(m_dayBookWidget, &MahadevERP::DayBookWidget::backRequested, this, [this]() { navigateToView(0); });
    connect(m_dayBookWidget, &MahadevERP::DayBookWidget::alterVoucherRequested, this, &MainWindow::onLedgerAlterVoucherRequested);
    m_stackedWidget->addWidget(m_dayBookWidget);

    // Index 15: Native C++ GST Compliance & Returns Widget (View 34)
    m_gstrReportsWidget = new MahadevERP::GstrReportsWidget(m_printExportCtrl, this);
    connect(m_gstrReportsWidget, &MahadevERP::GstrReportsWidget::backRequested, this, [this]() { navigateToView(0); });
    m_stackedWidget->addWidget(m_gstrReportsWidget);

    // Index 16: Native C++ Milling Statement & Out-turn Register (View 35)
    m_millingStatementWidget = new MahadevERP::MillingStatementWidget(m_printExportCtrl, this);
    connect(m_millingStatementWidget, &MahadevERP::MillingStatementWidget::backRequested, this, [this]() { navigateToView(0); });
    connect(m_millingStatementWidget, &MahadevERP::MillingStatementWidget::newBatchRequested, this, [this]() { navigateToView(31); });
    m_stackedWidget->addWidget(m_millingStatementWidget);

    // Index 17: Native C++ Custom Closing Stock & Valuation Register (View 36)
    m_customClosingStockWidget = new MahadevERP::CustomClosingStockWidget(m_printExportCtrl, this);
    connect(m_customClosingStockWidget, &MahadevERP::CustomClosingStockWidget::backRequested, this, [this]() { navigateToView(0); });
    m_stackedWidget->addWidget(m_customClosingStockWidget);

    // Index 18: Native C++ New Group Widget (View 9)
    m_newGroupWidget = new MahadevERP::NewGroupWidget(m_groupsModel, this);
    connect(m_newGroupWidget, &MahadevERP::NewGroupWidget::backRequested, this, [this]() { navigateToView(0); });
    connect(m_newGroupWidget, &MahadevERP::NewGroupWidget::savedSuccess, this, [this]() { navigateToView(0); });
    m_stackedWidget->addWidget(m_newGroupWidget);

    // Index 19: Native C++ Modify Group Widget (View 10)
    m_modifyGroupWidget = new MahadevERP::ModifyGroupWidget(m_groupsModel, this);
    connect(m_modifyGroupWidget, &MahadevERP::ModifyGroupWidget::backRequested, this, [this]() { navigateToView(0); });
    connect(m_modifyGroupWidget, &MahadevERP::ModifyGroupWidget::savedSuccess, this, [this]() { navigateToView(0); });
    m_stackedWidget->addWidget(m_modifyGroupWidget);

    // Index 20: Native C++ New Stock Item Widget (View 11)
    m_newStockItemWidget = new MahadevERP::NewStockItemWidget(m_stockMasterCtrl, m_stockItemsModel, this);
    connect(m_newStockItemWidget, &MahadevERP::NewStockItemWidget::backRequested, this, [this]() { navigateToView(0); });
    connect(m_newStockItemWidget, &MahadevERP::NewStockItemWidget::savedSuccess, this, [this]() { navigateToView(0); });
    m_stackedWidget->addWidget(m_newStockItemWidget);

    // Index 21: Native C++ Modify Stock Item Widget (View 12)
    m_modifyStockItemWidget = new MahadevERP::ModifyStockItemWidget(m_stockMasterCtrl, m_stockItemsModel, this);
    connect(m_modifyStockItemWidget, &MahadevERP::ModifyStockItemWidget::backRequested, this, [this]() { navigateToView(0); });
    connect(m_modifyStockItemWidget, &MahadevERP::ModifyStockItemWidget::savedSuccess, this, [this]() { navigateToView(0); });
    m_stackedWidget->addWidget(m_modifyStockItemWidget);

    // Index 22: Native C++ Sales Register Widget (View 3)
    m_salesRegisterWidget = new MahadevERP::SalesRegisterWidget(m_salesRegisterCtrl, m_printExportCtrl, this);
    connect(m_salesRegisterWidget, &MahadevERP::SalesRegisterWidget::backRequested, this, [this]() { navigateToView(0); });
    connect(m_salesRegisterWidget, &MahadevERP::SalesRegisterWidget::alterInvoiceRequested, this, &MainWindow::onLedgerAlterVoucherRequested);
    connect(m_salesRegisterWidget, &MahadevERP::SalesRegisterWidget::newInvoiceRequested, this, [this]() { navigateToView(14); });
    m_stackedWidget->addWidget(m_salesRegisterWidget);

    // Index 23: Native C++ Purchase Register Widget (View 4 / 21)
    m_purchaseRegisterWidget = new MahadevERP::PurchaseRegisterWidget(m_purchaseRegisterCtrl, m_printExportCtrl, this);
    connect(m_purchaseRegisterWidget, &MahadevERP::PurchaseRegisterWidget::backRequested, this, [this]() { navigateToView(0); });
    connect(m_purchaseRegisterWidget, &MahadevERP::PurchaseRegisterWidget::alterBillRequested, this, &MainWindow::onLedgerAlterVoucherRequested);
    connect(m_purchaseRegisterWidget, &MahadevERP::PurchaseRegisterWidget::newBillRequested, this, [this]() { navigateToView(15); });
    m_stackedWidget->addWidget(m_purchaseRegisterWidget);

    // Index 24: Native C++ Stock Detail Widget (View 13)
    m_stockDetailWidget = new MahadevERP::StockDetailWidget(m_stockRegisterCtrl, m_printExportCtrl, this);
    connect(m_stockDetailWidget, &MahadevERP::StockDetailWidget::backRequested, this, [this]() { navigateToView(0); });
    m_stackedWidget->addWidget(m_stockDetailWidget);

    // Index 25: Native C++ Paddy Procurement Widget (View 1)
    m_paddyProcurementWidget = new MahadevERP::PaddyProcurementWidget(m_paddyModel, m_paddyProcurementCtrl, m_printExportCtrl, this);
    connect(m_paddyProcurementWidget, &MahadevERP::PaddyProcurementWidget::backRequested, this, [this]() { navigateToView(0); });
    m_stackedWidget->addWidget(m_paddyProcurementWidget);

    // Index 26: Native C++ Milling Voucher Widget (View 31)
    m_millingVoucherWidget = new MahadevERP::MillingVoucherWidget(m_millingBatchCtrl, m_millingModel, m_printExportCtrl, this);
    connect(m_millingVoucherWidget, &MahadevERP::MillingVoucherWidget::backRequested, this, [this]() { navigateToView(0); });
    connect(m_millingVoucherWidget, &MahadevERP::MillingVoucherWidget::batchSaved, this, [this](const QString& batchNo) {
        Q_UNUSED(batchNo);
        navigateToView(0);
    });
    m_stackedWidget->addWidget(m_millingVoucherWidget);

    // Index 27: Native C++ TDS Voucher Widget (View 24)
    m_tdsVoucherWidget = new MahadevERP::TdsVoucherWidget(m_tdsVoucherCtrl, m_printExportCtrl, this);
    connect(m_tdsVoucherWidget, &MahadevERP::TdsVoucherWidget::backRequested, this, [this]() { navigateToView(0); });
    connect(m_tdsVoucherWidget, &MahadevERP::TdsVoucherWidget::voucherSaved, this, [this](int voucherNo) {
        Q_UNUSED(voucherNo);
        navigateToView(0);
    });
    m_stackedWidget->addWidget(m_tdsVoucherWidget);

    // Index 28: Native C++ Interest Calculator Widget (View 25)
    m_interestCalcWidget = new MahadevERP::InterestCalculatorWidget(m_interestModel, m_printExportCtrl, this);
    connect(m_interestCalcWidget, &MahadevERP::InterestCalculatorWidget::backRequested, this, [this]() { navigateToView(0); });
    m_stackedWidget->addWidget(m_interestCalcWidget);

    // Index 29: Native C++ Bank Statement Import Widget (View 26)
    m_bankStatementWidget = new MahadevERP::BankStatementImportWidget(m_bankStatementCtrl, m_printExportCtrl, this);
    connect(m_bankStatementWidget, &MahadevERP::BankStatementImportWidget::backRequested, this, [this]() { navigateToView(0); });
    connect(m_bankStatementWidget, &MahadevERP::BankStatementImportWidget::importCompleted, this, [this]() { navigateToView(0); });
    m_stackedWidget->addWidget(m_bankStatementWidget);

    // Index 30: Native C++ Transport Dispatch Widget (View 27)
    m_transportDispatchWidget = new MahadevERP::TransportDispatchWidget(m_transportDispatchCtrl, m_printExportCtrl, this);
    connect(m_transportDispatchWidget, &MahadevERP::TransportDispatchWidget::backRequested, this, [this]() { navigateToView(0); });
    m_stackedWidget->addWidget(m_transportDispatchWidget);

    // Index 31: Native C++ Debit / Credit Note Widget (View 28)
    m_debitCreditNoteWidget = new MahadevERP::DebitCreditNoteWidget(m_debitCreditNoteCtrl, m_printExportCtrl, this);
    connect(m_debitCreditNoteWidget, &MahadevERP::DebitCreditNoteWidget::backRequested, this, [this]() { navigateToView(0); });
    connect(m_debitCreditNoteWidget, &MahadevERP::DebitCreditNoteWidget::noteSaved, this, [this](const QString& noteNo) {
        Q_UNUSED(noteNo);
        navigateToView(0);
    });
    m_stackedWidget->addWidget(m_debitCreditNoteWidget);

    // TDS & TCS Subsystem Controllers
    m_taxChallanCtrl = new TaxChallanController(this);
    m_tcsReceiptVoucherCtrl = new TcsReceiptVoucherController(this);

    // Index 32: TDS Vouchers List (View 50)
    m_tdsVouchersListWidget = new MahadevERP::TdsVouchersListWidget(m_tdsVoucherCtrl, this);
    connect(m_tdsVouchersListWidget, &MahadevERP::TdsVouchersListWidget::backRequested, this, [this]() { navigateToView(0); });
    connect(m_tdsVouchersListWidget, &MahadevERP::TdsVouchersListWidget::newVoucherRequested, this, [this]() { navigateToView(24); });
    connect(m_tdsVouchersListWidget, &MahadevERP::TdsVouchersListWidget::editVoucherRequested, this, [this](int id) {
        m_pendingEditVoucherId = id;
        navigateToView(24);
    });
    m_stackedWidget->addWidget(m_tdsVouchersListWidget);

    // Index 33: Tax Challan Creation (View 51)
    m_taxChallanCreationWidget = new MahadevERP::TaxChallanCreationWidget(m_taxChallanCtrl, this);
    connect(m_taxChallanCreationWidget, &MahadevERP::TaxChallanCreationWidget::backRequested, this, [this]() { navigateToView(52); });
    connect(m_taxChallanCreationWidget, &MahadevERP::TaxChallanCreationWidget::challanSaved, this, [this]() { navigateToView(52); });
    m_stackedWidget->addWidget(m_taxChallanCreationWidget);

    // Index 34: Tax Challan Register (View 52)
    m_taxChallanRegisterWidget = new MahadevERP::TaxChallanRegisterWidget(m_taxChallanCtrl, "TDS", this);
    connect(m_taxChallanRegisterWidget, &MahadevERP::TaxChallanRegisterWidget::backRequested, this, [this]() { navigateToView(0); });
    connect(m_taxChallanRegisterWidget, &MahadevERP::TaxChallanRegisterWidget::newChallanRequested, this, [this](const QString& type) {
        m_taxChallanCreationWidget->setTaxType(type);
        navigateToView(51);
    });
    m_stackedWidget->addWidget(m_taxChallanRegisterWidget);

    // Index 35: TCS Receipt Voucher (View 53)
    m_tcsReceiptVoucherWidget = new MahadevERP::TcsReceiptVoucherWidget(m_tcsReceiptVoucherCtrl, this);
    connect(m_tcsReceiptVoucherWidget, &MahadevERP::TcsReceiptVoucherWidget::backRequested, this, [this]() { navigateToView(0); });
    connect(m_tcsReceiptVoucherWidget, &MahadevERP::TcsReceiptVoucherWidget::voucherSaved, this, [this]() { navigateToView(54); });
    m_stackedWidget->addWidget(m_tcsReceiptVoucherWidget);

    // Index 36: TCS Receipt List (View 54)
    m_tcsReceiptListWidget = new MahadevERP::TcsReceiptListWidget(m_tcsReceiptVoucherCtrl, this);
    connect(m_tcsReceiptListWidget, &MahadevERP::TcsReceiptListWidget::backRequested, this, [this]() { navigateToView(0); });
    connect(m_tcsReceiptListWidget, &MahadevERP::TcsReceiptListWidget::newReceiptRequested, this, [this]() { navigateToView(53); });
    m_stackedWidget->addWidget(m_tcsReceiptListWidget);

    // Index 37: Advance Payment 194-Q (View 55)
    m_advancePayment194QWidget = new MahadevERP::AdvancePayment194QWidget(this);
    connect(m_advancePayment194QWidget, &MahadevERP::AdvancePayment194QWidget::backRequested, this, [this]() { navigateToView(0); });
    connect(m_advancePayment194QWidget, &MahadevERP::AdvancePayment194QWidget::voucherSaved, this, [this]() { navigateToView(56); });
    m_stackedWidget->addWidget(m_advancePayment194QWidget);

    // Index 38: Advance Payment 194-Q List (View 56)
    m_advancePayment194QListWidget = new MahadevERP::AdvancePayment194QListWidget(this);
    connect(m_advancePayment194QListWidget, &MahadevERP::AdvancePayment194QListWidget::backRequested, this, [this]() { navigateToView(0); });
    connect(m_advancePayment194QListWidget, &MahadevERP::AdvancePayment194QListWidget::newVoucherRequested, this, [this]() { navigateToView(55); });
    m_stackedWidget->addWidget(m_advancePayment194QListWidget);

    // Index 39: Form-16A Received List (View 57)
    m_form16AListWidget = new MahadevERP::Form16AListWidget(this);
    connect(m_form16AListWidget, &MahadevERP::Form16AListWidget::backRequested, this, [this]() { navigateToView(0); });
    connect(m_form16AListWidget, &MahadevERP::Form16AListWidget::newCertificateRequested, this, [this]() {
        MahadevERP::Form16AEntryDialog dlg(this);
        if (dlg.exec() == QDialog::Accepted) {
            m_form16AListWidget->reloadData();
        }
    });
    m_stackedWidget->addWidget(m_form16AListWidget);

    // Global Shortcuts for Accounting Period (Alt+F2) and Context-Aware F2 (Date / Period)
    QShortcut* altF2Shortcut = new QShortcut(QKeySequence(Qt::ALT | Qt::Key_F2), this);
    connect(altF2Shortcut, &QShortcut::activated, this, &MainWindow::openAccountingPeriodDialog);

    QShortcut* f2Shortcut = new QShortcut(QKeySequence(Qt::Key_F2), this);
    connect(f2Shortcut, &QShortcut::activated, this, [this]() {
        int vIdx = currentViewIndex();
        if (vIdx == 14 && m_salesVoucherWidget) {
            m_salesVoucherWidget->openDateDialog();
        } else if (vIdx == 15 && m_purchaseVoucherWidget) {
            m_purchaseVoucherWidget->openDateDialog();
        } else if (vIdx == 16 && m_chequeVoucherWidget) {
            m_chequeVoucherWidget->openDateDialog();
        } else if (vIdx == 17 && m_journalVoucherWidget) {
            m_journalVoucherWidget->openDateDialog();
        } else if (vIdx == 18 && m_jformVoucherWidget) {
            m_jformVoucherWidget->openDateDialog();
        } else if (vIdx == 19 && m_iformVoucherWidget) {
            m_iformVoucherWidget->openDateDialog();
        } else if (vIdx == 24 && m_tdsVoucherWidget) {
            m_tdsVoucherWidget->openDateDialog();
        } else if (vIdx == 28 && m_debitCreditNoteWidget) {
            m_debitCreditNoteWidget->openDateDialog();
        } else if (vIdx == 31 && m_millingVoucherWidget) {
            m_millingVoucherWidget->openDateDialog();
        } else {
            openAccountingPeriodDialog();
        }
    });

    // Unified Application-Wide Keyboard Navigation Controller
    m_keyboardCtrl = new AppKeyboardController(this, this);

    // Global MenuTreeManager Integration
    connect(&MahadevERP::MenuTreeManager::instance(), &MahadevERP::MenuTreeManager::openViewRequested, this, &MainWindow::navigateToView);

    auto& menuMgr = MahadevERP::MenuTreeManager::instance();
    // TDS Deposit Challans (index 2 in tds_options)
    menuMgr.setItemCallback("tds_options", 2, [this]() {
        if (m_taxChallanRegisterWidget) m_taxChallanRegisterWidget->setTaxType("TDS");
    });
    // TDS Acknowledgement Nos. (index 3 in tds_options)
    menuMgr.setItemCallback("tds_options", 3, [this]() {
        MahadevERP::TdsAcknowledgementDialog ackDlg(this);
        ackDlg.exec();
    });
    // Receive Form-16A / 27D (index 6 in tds_options)
    menuMgr.setItemCallback("tds_options", 6, [this]() {
        MahadevERP::Form16AEntryDialog fDlg(this);
        if (fDlg.exec() == QDialog::Accepted && m_form16AListWidget) {
            m_form16AListWidget->reloadData();
        }
    });
    // TCS Deposit Challans (index 2 in tcs_options)
    menuMgr.setItemCallback("tcs_options", 2, [this]() {
        if (m_taxChallanRegisterWidget) m_taxChallanRegisterWidget->setTaxType("TCS");
    });
    // TCS Others (index 3 in tcs_options)
    menuMgr.setItemCallback("tcs_options", 3, [this]() {
        MahadevERP::TcsConfigDialog cDlg(this);
        cDlg.exec();
    });

    // Stock Register Hub - Item Monthly / Daily Stock (index 3 in stock_register_hub)
    menuMgr.setItemCallback("stock_register_hub", 3, [this]() {
        m_previousViewIndex = 0;
        if (m_stockDetailWidget) {
            m_stockDetailWidget->setViewConfiguration(StockViewMode::MonthlyDaily, StockGrouping::ItemWise, "Item Monthly/Daily Stock");
        }
    });

    // Show Only Stock Submenu
    menuMgr.setItemCallback("stock_only_menu", 0, [this]() {
        m_previousViewIndex = 0;
        if (m_stockDetailWidget) m_stockDetailWidget->setViewConfiguration(StockViewMode::OnlyStock, StockGrouping::ItemWise, "Item Stock");
    });
    menuMgr.setItemCallback("stock_only_menu", 1, [this]() {
        m_previousViewIndex = 0;
        if (m_stockDetailWidget) m_stockDetailWidget->setViewConfiguration(StockViewMode::OnlyStock, StockGrouping::GroupWise, "Group Stock");
    });
    menuMgr.setItemCallback("stock_only_menu", 2, [this]() {
        m_previousViewIndex = 0;
        if (m_stockDetailWidget) m_stockDetailWidget->setViewConfiguration(StockViewMode::OnlyStock, StockGrouping::CompanyWise, "Company Stock");
    });
    menuMgr.setItemCallback("stock_only_menu", 3, [this]() {
        m_previousViewIndex = 0;
        if (m_stockDetailWidget) m_stockDetailWidget->setViewConfiguration(StockViewMode::OnlyStock, StockGrouping::TotalSummary, "Total Stock");
    });
    menuMgr.setItemCallback("stock_only_menu", 4, [this]() {
        m_previousViewIndex = 0;
        if (m_stockDetailWidget) m_stockDetailWidget->setViewConfiguration(StockViewMode::OnlyStock, StockGrouping::HsnWise, "HSN Wise Total Stock");
    });

    // Show Stock With Amount Submenu
    menuMgr.setItemCallback("stock_amount_menu", 0, [this]() {
        m_previousViewIndex = 0;
        if (m_stockDetailWidget) m_stockDetailWidget->setViewConfiguration(StockViewMode::StockWithAmount, StockGrouping::ItemWise, "Item Stock With Amount");
    });
    menuMgr.setItemCallback("stock_amount_menu", 1, [this]() {
        m_previousViewIndex = 0;
        if (m_stockDetailWidget) m_stockDetailWidget->setViewConfiguration(StockViewMode::StockWithAmount, StockGrouping::GroupWise, "Group Stock With Amount");
    });
    menuMgr.setItemCallback("stock_amount_menu", 2, [this]() {
        m_previousViewIndex = 0;
        if (m_stockDetailWidget) m_stockDetailWidget->setViewConfiguration(StockViewMode::StockWithAmount, StockGrouping::CompanyWise, "Company Stock With Amount");
    });
    menuMgr.setItemCallback("stock_amount_menu", 3, [this]() {
        m_previousViewIndex = 0;
        if (m_stockDetailWidget) m_stockDetailWidget->setViewConfiguration(StockViewMode::StockWithAmount, StockGrouping::TotalSummary, "Total Stock With Amount");
    });
    menuMgr.setItemCallback("stock_amount_menu", 4, [this]() {
        m_previousViewIndex = 0;
        if (m_stockDetailWidget) m_stockDetailWidget->setViewConfiguration(StockViewMode::StockWithAmount, StockGrouping::HsnWise, "HSN Wise Total Stock With Amount");
    });

    // Item Wise Profit & Loss Submenu
    menuMgr.setItemCallback("stock_profit_menu", 0, [this]() {
        m_previousViewIndex = 0;
        if (m_stockDetailWidget) m_stockDetailWidget->setViewConfiguration(StockViewMode::ProfitLoss, StockGrouping::ItemWise, "Item Details (P&L)");
    });
    menuMgr.setItemCallback("stock_profit_menu", 1, [this]() {
        m_previousViewIndex = 0;
        if (m_stockDetailWidget) m_stockDetailWidget->setViewConfiguration(StockViewMode::ProfitLoss, StockGrouping::GroupWise, "Group Details (P&L)");
    });
    menuMgr.setItemCallback("stock_profit_menu", 2, [this]() {
        m_previousViewIndex = 0;
        if (m_stockDetailWidget) m_stockDetailWidget->setViewConfiguration(StockViewMode::ProfitLoss, StockGrouping::CompanyWise, "Company Details (P&L)");
    });
    menuMgr.setItemCallback("stock_profit_menu", 3, [this]() {
        m_previousViewIndex = 0;
        if (m_stockDetailWidget) m_stockDetailWidget->setViewConfiguration(StockViewMode::ProfitLoss, StockGrouping::TotalSummary, "Total Items Details (P&L)");
    });
    menuMgr.setItemCallback("stock_profit_menu", 4, [this]() {
        m_previousViewIndex = 0;
        if (m_stockDetailWidget) m_stockDetailWidget->setViewConfiguration(StockViewMode::ProfitLoss, StockGrouping::HsnWise, "HSN Wise Total Details (P&L)");
    });

    // Raw Paddy Stock Register (index 4 in stock_master)
    menuMgr.setItemCallback("stock_master", 4, [this]() {
        m_previousViewIndex = 0;
        if (m_stockDetailWidget) {
            m_stockDetailWidget->setViewConfiguration(StockViewMode::OnlyStock, StockGrouping::ItemWise, "Raw Paddy Stock");
        }
        navigateToView(13);
    });

    // Finished Rice Stock Register (index 5 in stock_master)
    menuMgr.setItemCallback("stock_master", 5, [this]() {
        m_previousViewIndex = 0;
        if (m_stockDetailWidget) {
            m_stockDetailWidget->setViewConfiguration(StockViewMode::OnlyStock, StockGrouping::ItemWise, "Finished Rice Stock");
        }
        navigateToView(13);
    });

    // By-Products & Husk Stock Register (index 6 in stock_master)
    menuMgr.setItemCallback("stock_master", 6, [this]() {
        m_previousViewIndex = 0;
        if (m_stockDetailWidget) {
            m_stockDetailWidget->setViewConfiguration(StockViewMode::OnlyStock, StockGrouping::ItemWise, "By-Products & Husk Stock");
        }
        navigateToView(13);
    });

    // Explicitly start application on Firm Selector View (View 22)
    navigateToView(22);
}

int MainWindow::currentViewIndex() const {
    if (!m_stackedWidget) return 0;
    QWidget* cur = m_stackedWidget->currentWidget();
    if (cur == m_dashboardWidget) return 0;
    if (cur == m_paddyProcurementWidget) return 1;
    if (cur == m_salesRegisterWidget) return 3;
    if (cur == m_purchaseRegisterWidget) return 4;
    if (cur == m_newLedgerWidget) return 6;
    if (cur == m_modifyLedgerWidget) return 7;
    if (cur == m_ledgerWidget) return 8;
    if (cur == m_newGroupWidget) return 9;
    if (cur == m_modifyGroupWidget) return 10;
    if (cur == m_newStockItemWidget) return 11;
    if (cur == m_modifyStockItemWidget) return 12;
    if (cur == m_stockDetailWidget) return 13;
    if (cur == m_salesVoucherWidget) return 14;
    if (cur == m_purchaseVoucherWidget) return 15;
    if (cur == m_chequeVoucherWidget) return 16;
    if (cur == m_journalVoucherWidget) return 17;
    if (cur == m_jformVoucherWidget) return 18;
    if (cur == m_iformVoucherWidget) return 19;
    if (cur == m_mandiReportsWidget) return 20;
    if (cur == m_firmSelectorWidget) return 22;
    if (cur == m_tdsVoucherWidget) return 24;
    if (cur == m_interestCalcWidget) return 25;
    if (cur == m_bankStatementWidget) return 26;
    if (cur == m_transportDispatchWidget) return 27;
    if (cur == m_debitCreditNoteWidget) return 28;
    if (cur == m_balanceSheetWidget) return 29;
    if (cur == m_profitLossWidget) return 30;
    if (cur == m_millingVoucherWidget) return 31;
    if (cur == m_dayBookWidget) return 33;
    if (cur == m_gstrReportsWidget) return 34;
    if (cur == m_millingStatementWidget) return 35;
    if (cur == m_customClosingStockWidget) return 36;
    if (cur == m_tdsVouchersListWidget) return 50;
    if (cur == m_taxChallanCreationWidget) return 51;
    if (cur == m_taxChallanRegisterWidget) return 52;
    if (cur == m_tcsReceiptVoucherWidget) return 53;
    if (cur == m_tcsReceiptListWidget) return 54;
    if (cur == m_advancePayment194QWidget) return 55;
    if (cur == m_advancePayment194QListWidget) return 56;
    if (cur == m_form16AListWidget) return 57;
    return 0;
}

void MainWindow::restoreActiveViewFocus() {
    int vIdx = currentViewIndex();
    if (vIdx == 0 && m_dashboardWidget) {
        m_dashboardWidget->focusMenu();
    } else if (vIdx == 1 && m_paddyProcurementWidget) {
        m_paddyProcurementWidget->setFocus(Qt::OtherFocusReason);
        m_paddyProcurementWidget->focusTable();
    } else if (vIdx == 3 && m_salesRegisterWidget) {
        m_salesRegisterWidget->setFocus(Qt::OtherFocusReason);
        m_salesRegisterWidget->focusTable();
    } else if ((vIdx == 4 || vIdx == 21) && m_purchaseRegisterWidget) {
        m_purchaseRegisterWidget->setFocus(Qt::OtherFocusReason);
        m_purchaseRegisterWidget->focusTable();
    } else if (vIdx == 13 && m_stockDetailWidget) {
        m_stockDetailWidget->setFocus(Qt::OtherFocusReason);
        m_stockDetailWidget->focusTable();
    } else if (vIdx == 14 && m_salesVoucherWidget) {
        m_salesVoucherWidget->setFocus(Qt::OtherFocusReason);
    } else if (vIdx == 15 && m_purchaseVoucherWidget) {
        m_purchaseVoucherWidget->setFocus(Qt::OtherFocusReason);
    } else if (vIdx == 16 && m_chequeVoucherWidget) {
        m_chequeVoucherWidget->setFocus(Qt::OtherFocusReason);
    } else if (vIdx == 17 && m_journalVoucherWidget) {
        m_journalVoucherWidget->setFocus(Qt::OtherFocusReason);
    } else if (vIdx == 18 && m_jformVoucherWidget) {
        m_jformVoucherWidget->setFocus(Qt::OtherFocusReason);
    } else if (vIdx == 19 && m_iformVoucherWidget) {
        m_iformVoucherWidget->setFocus(Qt::OtherFocusReason);
    } else if (vIdx == 20 && m_mandiReportsWidget) {
        m_mandiReportsWidget->setFocus(Qt::OtherFocusReason);
    } else if (vIdx == 24 && m_tdsVoucherWidget) {
        m_tdsVoucherWidget->setFocus(Qt::OtherFocusReason);
    } else if (vIdx == 25 && m_interestCalcWidget) {
        m_interestCalcWidget->setFocus(Qt::OtherFocusReason);
    } else if (vIdx == 26 && m_bankStatementWidget) {
        m_bankStatementWidget->setFocus(Qt::OtherFocusReason);
    } else if (vIdx == 27 && m_transportDispatchWidget) {
        m_transportDispatchWidget->setFocus(Qt::OtherFocusReason);
        m_transportDispatchWidget->focusTable();
    } else if (vIdx == 28 && m_debitCreditNoteWidget) {
        m_debitCreditNoteWidget->setFocus(Qt::OtherFocusReason);
    } else if (vIdx == 31 && m_millingVoucherWidget) {
        m_millingVoucherWidget->setFocus(Qt::OtherFocusReason);
    } else if (vIdx == 33 && m_dayBookWidget) {
        m_dayBookWidget->setFocus(Qt::OtherFocusReason);
    } else if (vIdx == 34 && m_gstrReportsWidget) {
        m_gstrReportsWidget->setFocus(Qt::OtherFocusReason);
    } else if (vIdx == 35 && m_millingStatementWidget) {
        m_millingStatementWidget->setFocus(Qt::OtherFocusReason);
    } else if (vIdx == 36 && m_customClosingStockWidget) {
        m_customClosingStockWidget->setFocus(Qt::OtherFocusReason);
    } else if (vIdx == 6 && m_newLedgerWidget) {
        m_newLedgerWidget->setFocus(Qt::OtherFocusReason);
        m_newLedgerWidget->focusFirstField();
    } else if (vIdx == 7 && m_modifyLedgerWidget) {
        m_modifyLedgerWidget->setFocus(Qt::OtherFocusReason);
        m_modifyLedgerWidget->focusSearch();
    } else if (vIdx == 9 && m_newGroupWidget) {
        m_newGroupWidget->setFocus(Qt::OtherFocusReason);
        m_newGroupWidget->focusFirstField();
    } else if (vIdx == 10 && m_modifyGroupWidget) {
        m_modifyGroupWidget->setFocus(Qt::OtherFocusReason);
        m_modifyGroupWidget->focusSearch();
    } else if (vIdx == 11 && m_newStockItemWidget) {
        m_newStockItemWidget->setFocus(Qt::OtherFocusReason);
        m_newStockItemWidget->focusFirstField();
    } else if (vIdx == 12 && m_modifyStockItemWidget) {
        m_modifyStockItemWidget->setFocus(Qt::OtherFocusReason);
        m_modifyStockItemWidget->focusSearch();
    } else if (vIdx == 22 && m_firmSelectorWidget) {
        m_firmSelectorWidget->setFocus(Qt::OtherFocusReason);
        m_firmSelectorWidget->focusTable();
    } else if (vIdx == 29 && m_balanceSheetWidget) {
        m_balanceSheetWidget->setFocus(Qt::OtherFocusReason);
    } else if (vIdx == 30 && m_profitLossWidget) {
        m_profitLossWidget->setFocus(Qt::OtherFocusReason);
    } else if (vIdx == 8 && m_ledgerWidget) {
        m_ledgerWidget->setFocus(Qt::OtherFocusReason);
    }
}

void MainWindow::openAccountingPeriodDialog() {
    QString fIso, tIso, fyLabel;
    bool applied = AccountingPeriodDialog::selectAndApplyGlobalPeriod(this, &fIso, &tIso, &fyLabel);
    if (applied) {
        if (m_dashCtrl) {
            m_dashCtrl->refresh_stats(fIso, tIso, fyLabel);
        }

        if (m_dashboardWidget) {
            m_dashboardWidget->refreshStats();
        }

        if (m_ledgerWidget) {
            m_ledgerWidget->onTotalsChanged();
        }

        if (m_balanceSheetWidget) {
            m_balanceSheetWidget->refreshData(tIso);
        }

        if (m_profitLossWidget) {
            m_profitLossWidget->refreshData(fIso, tIso);
        }

        if (m_salesRegisterWidget) {
            m_salesRegisterWidget->loadData(QDate::fromString(fIso, "yyyy-MM-dd"), QDate::fromString(tIso, "yyyy-MM-dd"));
        }

        if (m_purchaseRegisterWidget) {
            m_purchaseRegisterWidget->loadData(QDate::fromString(fIso, "yyyy-MM-dd"), QDate::fromString(tIso, "yyyy-MM-dd"));
        }

        if (m_stockDetailWidget) {
            m_stockDetailWidget->reloadData(fIso, tIso);
        }

        if (m_paddyProcurementWidget) {
            m_paddyProcurementWidget->loadArrivals(QDate::fromString(fIso, "yyyy-MM-dd"), QDate::fromString(tIso, "yyyy-MM-dd"));
        }
    }

    QTimer::singleShot(10, this, [this]() {
        restoreActiveViewFocus();
    });
}

void MainWindow::navigateToView(int viewIndex) {
    if (m_isNavigating) return;
    m_isNavigating = true;

    QString pendingInv = m_pendingEditInvoiceNo;
    QString pendingVNo = m_pendingEditVoucherNo;
    int pendingId = m_pendingEditVoucherId;
    QString pendingDate = m_pendingEditVoucherDate;

    m_pendingEditInvoiceNo.clear();
    m_pendingEditVoucherNo.clear();
    m_pendingEditVoucherId = 0;
    m_pendingEditVoucherDate.clear();

    qDebug() << "[NAV] navigateToView viewIndex:" << viewIndex << "pendingInv:" << pendingInv << "pendingVNo:" << pendingVNo << "pendingId:" << pendingId << "pendingDate:" << pendingDate;

    if (viewIndex == 0) {
        if (m_dashboardWidget) {
            m_stackedWidget->setCurrentWidget(m_dashboardWidget);
            m_dashboardWidget->setFocus();
            m_dashboardWidget->refreshStats();
        }
    } else if (viewIndex == 1) {
        if (m_paddyProcurementWidget) {
            m_stackedWidget->setCurrentWidget(m_paddyProcurementWidget);
            FiscalYearInfo activeFy = FiscalYearHelper::getActiveFiscalYear();
            m_paddyProcurementWidget->loadArrivals(QDate::fromString(activeFy.startDate, "yyyy-MM-dd"), QDate::fromString(activeFy.endDate, "yyyy-MM-dd"));
            m_paddyProcurementWidget->setFocus();
            m_paddyProcurementWidget->focusTable();
        }
    } else if (viewIndex == 3) {
        if (m_salesRegisterWidget) {
            m_stackedWidget->setCurrentWidget(m_salesRegisterWidget);
            FiscalYearInfo activeFy = FiscalYearHelper::getActiveFiscalYear();
            m_salesRegisterWidget->loadData(QDate::fromString(activeFy.startDate, "yyyy-MM-dd"), QDate::fromString(activeFy.endDate, "yyyy-MM-dd"));
            m_salesRegisterWidget->setFocus();
            m_salesRegisterWidget->focusTable();
        }
    } else if (viewIndex == 4 || viewIndex == 21) {
        if (m_purchaseRegisterWidget) {
            m_stackedWidget->setCurrentWidget(m_purchaseRegisterWidget);
            FiscalYearInfo activeFy = FiscalYearHelper::getActiveFiscalYear();
            m_purchaseRegisterWidget->loadData(QDate::fromString(activeFy.startDate, "yyyy-MM-dd"), QDate::fromString(activeFy.endDate, "yyyy-MM-dd"));
            m_purchaseRegisterWidget->setFocus();
            m_purchaseRegisterWidget->focusTable();
        }
    } else if (viewIndex == 6) {
        if (m_newLedgerWidget) {
            m_stackedWidget->setCurrentWidget(m_newLedgerWidget);
            m_newLedgerWidget->resetForm();
            m_newLedgerWidget->setFocus();
            m_newLedgerWidget->focusFirstField();
        }
    } else if (viewIndex == 7) {
        if (m_modifyLedgerWidget) {
            m_stackedWidget->setCurrentWidget(m_modifyLedgerWidget);
            m_modifyLedgerWidget->resetForm();
            m_modifyLedgerWidget->setFocus();
            m_modifyLedgerWidget->focusSearch();
        }
    } else if (viewIndex == 8) {
        QString party = m_targetStatementParty.trimmed();
        QString fromDate = m_lastViewedStatementFromDate;
        QString toDate = m_lastViewedStatementToDate;
        QString side = m_lastViewedStatementSide.isEmpty() ? "Dr" : m_lastViewedStatementSide;
        int rowIndex = m_lastViewedStatementIndex;

        m_targetStatementParty.clear();

        m_stackedWidget->setCurrentWidget(m_ledgerWidget);
        if (!party.isEmpty()) {
            m_ledgerWidget->restoreState(party, fromDate, toDate, side, rowIndex);
        } else {
            m_ledgerWidget->resetSearch();
            m_ledgerWidget->focusSearch();
        }
        if (m_ledgerWidget) {
            m_ledgerWidget->setFocus();
        }
    } else if (viewIndex == 9) {
        if (m_newGroupWidget) {
            m_stackedWidget->setCurrentWidget(m_newGroupWidget);
            m_newGroupWidget->resetForm();
            m_newGroupWidget->setFocus();
            m_newGroupWidget->focusFirstField();
        }
    } else if (viewIndex == 10) {
        if (m_modifyGroupWidget) {
            m_stackedWidget->setCurrentWidget(m_modifyGroupWidget);
            m_modifyGroupWidget->resetForm();
            m_modifyGroupWidget->setFocus();
            m_modifyGroupWidget->focusSearch();
        }
    } else if (viewIndex == 11) {
        if (m_newStockItemWidget) {
            m_stackedWidget->setCurrentWidget(m_newStockItemWidget);
            m_newStockItemWidget->resetForm();
            m_newStockItemWidget->setFocus();
            m_newStockItemWidget->focusFirstField();
        }
    } else if (viewIndex == 12) {
        if (m_modifyStockItemWidget) {
            m_stackedWidget->setCurrentWidget(m_modifyStockItemWidget);
            m_modifyStockItemWidget->resetForm();
            m_modifyStockItemWidget->setFocus();
            m_modifyStockItemWidget->focusSearch();
        }
    } else if (viewIndex == 13) {
        if (m_stockDetailWidget) {
            m_stackedWidget->setCurrentWidget(m_stockDetailWidget);
            FiscalYearInfo activeFy = FiscalYearHelper::getActiveFiscalYear();
            m_stockDetailWidget->reloadData(activeFy.startDate, activeFy.endDate);
            m_stockDetailWidget->setFocus();
            m_stockDetailWidget->focusTable();
        }
    } else if (viewIndex == 14) {
        if (m_salesVoucherWidget) {
            m_stackedWidget->setCurrentWidget(m_salesVoucherWidget);
            if (!pendingInv.isEmpty() || !pendingVNo.isEmpty() || pendingId > 0) {
                QVariant target = !pendingInv.isEmpty() ? QVariant(pendingInv) : (!pendingVNo.isEmpty() ? QVariant(pendingVNo) : QVariant(pendingId));
                m_salesVoucherWidget->loadInvoiceForEditing(target, pendingDate);
            } else {
                m_salesVoucherWidget->resetForm();
                QTimer::singleShot(0, this, [this]() {
                    if (m_salesVoucherWidget && m_stackedWidget->currentWidget() == m_salesVoucherWidget) {
                        m_salesVoucherWidget->openDateDialog(true);
                    }
                });
            }
            m_salesVoucherWidget->setFocus();
        }
    } else if (viewIndex == 15) {
        if (m_purchaseVoucherWidget) {
            m_stackedWidget->setCurrentWidget(m_purchaseVoucherWidget);
            if (!pendingInv.isEmpty() || !pendingVNo.isEmpty() || pendingId > 0) {
                QVariant target = !pendingInv.isEmpty() ? QVariant(pendingInv) : (!pendingVNo.isEmpty() ? QVariant(pendingVNo) : QVariant(pendingId));
                m_purchaseVoucherWidget->loadInvoiceForEditing(target, pendingDate);
            } else {
                m_purchaseVoucherWidget->resetForm();
                QTimer::singleShot(0, this, [this]() {
                    if (m_purchaseVoucherWidget && m_stackedWidget->currentWidget() == m_purchaseVoucherWidget) {
                        m_purchaseVoucherWidget->openDateDialog(true);
                    }
                });
            }
            m_purchaseVoucherWidget->setFocus();
        }
    } else if (viewIndex == 16) {
        if (m_chequeVoucherWidget) {
            m_stackedWidget->setCurrentWidget(m_chequeVoucherWidget);
            QString mode = m_targetChequeMode;
            m_targetChequeMode.clear();
            if (mode == "RECEIPT") m_chequeVoucherWidget->setVoucherType("Receipt");
            else if (mode == "PAYMENT") m_chequeVoucherWidget->setVoucherType("Payment");

            if (!pendingInv.isEmpty() || !pendingVNo.isEmpty() || pendingId > 0) {
                QVariant target = !pendingInv.isEmpty() ? QVariant(pendingInv) : (!pendingVNo.isEmpty() ? QVariant(pendingVNo) : QVariant(pendingId));
                m_chequeVoucherWidget->loadVoucherForEditing(target, pendingDate);
            } else {
                m_chequeVoucherWidget->resetForm();
                QTimer::singleShot(0, this, [this]() {
                    if (m_chequeVoucherWidget && m_stackedWidget->currentWidget() == m_chequeVoucherWidget) {
                        m_chequeVoucherWidget->openDateDialog(true);
                    }
                });
            }
            m_chequeVoucherWidget->setFocus();
        }
    } else if (viewIndex == 17) {
        if (m_journalVoucherWidget) {
            m_stackedWidget->setCurrentWidget(m_journalVoucherWidget);
            if (!pendingInv.isEmpty() || !pendingVNo.isEmpty() || pendingId > 0) {
                QVariant target = !pendingInv.isEmpty() ? QVariant(pendingInv) : (!pendingVNo.isEmpty() ? QVariant(pendingVNo) : QVariant(pendingId));
                m_journalVoucherWidget->loadVoucherForEditing(target, pendingDate);
            } else {
                m_journalVoucherWidget->resetForm();
                QTimer::singleShot(0, this, [this]() {
                    if (m_journalVoucherWidget && m_stackedWidget->currentWidget() == m_journalVoucherWidget) {
                        m_journalVoucherWidget->openDateDialog(true);
                    }
                });
            }
            m_journalVoucherWidget->setFocus();
        }
    } else if (viewIndex == 18) {
        if (m_jformVoucherWidget) {
            m_stackedWidget->setCurrentWidget(m_jformVoucherWidget);
            if (!pendingInv.isEmpty() || !pendingVNo.isEmpty() || pendingId > 0) {
                QVariant target = !pendingInv.isEmpty() ? QVariant(pendingInv) : (!pendingVNo.isEmpty() ? QVariant(pendingVNo) : QVariant(pendingId));
                m_jformVoucherWidget->loadVoucherForEditing(target, pendingDate);
            } else {
                m_jformVoucherWidget->resetForm();
                QTimer::singleShot(0, this, [this]() {
                    if (m_jformVoucherWidget && m_stackedWidget->currentWidget() == m_jformVoucherWidget) {
                        m_jformVoucherWidget->openDateDialog(true);
                    }
                });
            }
            m_jformVoucherWidget->setFocus();
        }
    } else if (viewIndex == 19) {
        if (m_iformVoucherWidget) {
            m_stackedWidget->setCurrentWidget(m_iformVoucherWidget);
            if (!pendingInv.isEmpty() || !pendingVNo.isEmpty() || pendingId > 0) {
                QVariant target = !pendingInv.isEmpty() ? QVariant(pendingInv) : (!pendingVNo.isEmpty() ? QVariant(pendingVNo) : QVariant(pendingId));
                m_iformVoucherWidget->loadVoucherForEditing(target, pendingDate);
            } else {
                m_iformVoucherWidget->resetForm();
                QTimer::singleShot(0, this, [this]() {
                    if (m_iformVoucherWidget && m_stackedWidget->currentWidget() == m_iformVoucherWidget) {
                        m_iformVoucherWidget->openDateDialog(true);
                    }
                });
            }
            m_iformVoucherWidget->setFocus();
        }
    } else if (viewIndex == 20) {
        if (m_mandiReportsWidget) {
            m_stackedWidget->setCurrentWidget(m_mandiReportsWidget);
            m_mandiReportsWidget->refreshAllTabs();
            m_mandiReportsWidget->setFocus();
        }
    } else if (viewIndex == 22) {
        if (m_firmSelectorWidget) {
            m_stackedWidget->setCurrentWidget(m_firmSelectorWidget);
            m_firmSelectorWidget->refreshFirms();
            m_firmSelectorWidget->setFocus();
            m_firmSelectorWidget->focusTable();
        }
    } else if (viewIndex == 24) {
        if (m_tdsVoucherWidget) {
            m_stackedWidget->setCurrentWidget(m_tdsVoucherWidget);
            if (pendingId > 0) {
                m_tdsVoucherWidget->loadVoucherForEditing(pendingId);
            } else {
                m_tdsVoucherWidget->resetForm();
            }
            m_tdsVoucherWidget->setFocus();
        }
    } else if (viewIndex == 25) {
        if (m_interestCalcWidget) {
            m_stackedWidget->setCurrentWidget(m_interestCalcWidget);
            m_interestCalcWidget->calculateInterest();
            m_interestCalcWidget->setFocus();
        }
    } else if (viewIndex == 26) {
        if (m_bankStatementWidget) {
            m_stackedWidget->setCurrentWidget(m_bankStatementWidget);
            m_bankStatementWidget->resetForm();
            m_bankStatementWidget->setFocus();
        }
    } else if (viewIndex == 27) {
        if (m_transportDispatchWidget) {
            m_stackedWidget->setCurrentWidget(m_transportDispatchWidget);
            FiscalYearInfo activeFy = FiscalYearHelper::getActiveFiscalYear();
            m_transportDispatchWidget->loadData(QDate::fromString(activeFy.startDate, "yyyy-MM-dd"), QDate::fromString(activeFy.endDate, "yyyy-MM-dd"));
            m_transportDispatchWidget->setFocus();
            m_transportDispatchWidget->focusTable();
        }
    } else if (viewIndex == 28) {
        if (m_debitCreditNoteWidget) {
            m_stackedWidget->setCurrentWidget(m_debitCreditNoteWidget);
            if (pendingId > 0) {
                m_debitCreditNoteWidget->loadNoteForEditing(pendingId);
            } else {
                m_debitCreditNoteWidget->resetForm();
            }
            m_debitCreditNoteWidget->setFocus();
        }
    } else if (viewIndex == 29) {
        FiscalYearInfo activeFy = FiscalYearHelper::getActiveFiscalYear();
        m_stackedWidget->setCurrentWidget(m_balanceSheetWidget);
        m_balanceSheetWidget->refreshData(activeFy.endDate);
        if (m_balanceSheetWidget) {
            m_balanceSheetWidget->setFocus();
        }
    } else if (viewIndex == 30) {
        FiscalYearInfo activeFy = FiscalYearHelper::getActiveFiscalYear();
        m_stackedWidget->setCurrentWidget(m_profitLossWidget);
        m_profitLossWidget->refreshData(activeFy.startDate, activeFy.endDate);
        if (m_profitLossWidget) {
            m_profitLossWidget->setFocus();
        }
    } else if (viewIndex == 31) {
        if (m_millingVoucherWidget) {
            m_stackedWidget->setCurrentWidget(m_millingVoucherWidget);
            if (!pendingInv.isEmpty() || !pendingVNo.isEmpty() || pendingId > 0) {
                QVariant target = !pendingInv.isEmpty() ? QVariant(pendingInv) : (!pendingVNo.isEmpty() ? QVariant(pendingVNo) : QVariant(pendingId));
                m_millingVoucherWidget->loadBatchForEditing(target);
            } else {
                m_millingVoucherWidget->resetForm();
            }
            m_millingVoucherWidget->setFocus();
        }
    } else if (viewIndex == 33) {
        if (m_dayBookWidget) {
            m_stackedWidget->setCurrentWidget(m_dayBookWidget);
            FiscalYearInfo activeFy = FiscalYearHelper::getActiveFiscalYear();
            QDate sDate = QDate::fromString(activeFy.startDate, "yyyy-MM-dd");
            QDate eDate = QDate::fromString(activeFy.endDate, "yyyy-MM-dd");
            if (!sDate.isValid()) sDate = QDate(QDate::currentDate().month() < 4 ? QDate::currentDate().year() - 1 : QDate::currentDate().year(), 4, 1);
            if (!eDate.isValid()) eDate = QDate::currentDate();
            m_dayBookWidget->loadDayBookData(sDate, eDate);
            m_dayBookWidget->setFocus();
        }
    } else if (viewIndex == 34) {
        if (m_gstrReportsWidget) {
            m_stackedWidget->setCurrentWidget(m_gstrReportsWidget);
            FiscalYearInfo activeFy = FiscalYearHelper::getActiveFiscalYear();
            QDate sDate = QDate::fromString(activeFy.startDate, "yyyy-MM-dd");
            QDate eDate = QDate::fromString(activeFy.endDate, "yyyy-MM-dd");
            if (!sDate.isValid()) sDate = QDate(QDate::currentDate().month() < 4 ? QDate::currentDate().year() - 1 : QDate::currentDate().year(), 4, 1);
            if (!eDate.isValid()) eDate = QDate::currentDate();
            m_gstrReportsWidget->loadReturns(sDate, eDate);
            m_gstrReportsWidget->setFocus();
        }
    } else if (viewIndex == 35) {
        if (m_millingStatementWidget) {
            m_stackedWidget->setCurrentWidget(m_millingStatementWidget);
            FiscalYearInfo activeFy = FiscalYearHelper::getActiveFiscalYear();
            QDate sDate = QDate::fromString(activeFy.startDate, "yyyy-MM-dd");
            QDate eDate = QDate::fromString(activeFy.endDate, "yyyy-MM-dd");
            if (!sDate.isValid()) sDate = QDate(QDate::currentDate().month() < 4 ? QDate::currentDate().year() - 1 : QDate::currentDate().year(), 4, 1);
            if (!eDate.isValid()) eDate = QDate::currentDate();
            m_millingStatementWidget->loadMillingData(sDate, eDate);
            m_millingStatementWidget->setFocus();
        }
    } else if (viewIndex == 36) {
        if (m_customClosingStockWidget) {
            m_stackedWidget->setCurrentWidget(m_customClosingStockWidget);
            m_customClosingStockWidget->reloadData();
            m_customClosingStockWidget->setFocus();
        }
    } else if (viewIndex == 50) {
        if (m_tdsVouchersListWidget) {
            m_stackedWidget->setCurrentWidget(m_tdsVouchersListWidget);
            m_tdsVouchersListWidget->reloadData();
            m_tdsVouchersListWidget->setFocus();
        }
    } else if (viewIndex == 51) {
        if (m_taxChallanCreationWidget) {
            m_stackedWidget->setCurrentWidget(m_taxChallanCreationWidget);
            m_taxChallanCreationWidget->setFocus();
        }
    } else if (viewIndex == 52) {
        if (m_taxChallanRegisterWidget) {
            m_stackedWidget->setCurrentWidget(m_taxChallanRegisterWidget);
            m_taxChallanRegisterWidget->reloadData();
            m_taxChallanRegisterWidget->setFocus();
        }
    } else if (viewIndex == 53) {
        if (m_tcsReceiptVoucherWidget) {
            m_stackedWidget->setCurrentWidget(m_tcsReceiptVoucherWidget);
            m_tcsReceiptVoucherWidget->resetForm();
            m_tcsReceiptVoucherWidget->setFocus();
        }
    } else if (viewIndex == 54) {
        if (m_tcsReceiptListWidget) {
            m_stackedWidget->setCurrentWidget(m_tcsReceiptListWidget);
            m_tcsReceiptListWidget->reloadData();
            m_tcsReceiptListWidget->setFocus();
        }
    } else if (viewIndex == 55) {
        if (m_advancePayment194QWidget) {
            m_stackedWidget->setCurrentWidget(m_advancePayment194QWidget);
            m_advancePayment194QWidget->resetForm();
            m_advancePayment194QWidget->setFocus();
        }
    } else if (viewIndex == 56) {
        if (m_advancePayment194QListWidget) {
            m_stackedWidget->setCurrentWidget(m_advancePayment194QListWidget);
            m_advancePayment194QListWidget->reloadData();
            m_advancePayment194QListWidget->setFocus();
        }
    } else if (viewIndex == 57) {
        if (m_form16AListWidget) {
            m_stackedWidget->setCurrentWidget(m_form16AListWidget);
            m_form16AListWidget->reloadData();
            m_form16AListWidget->setFocus();
        }
    } else if (viewIndex == 60) {
        openTdsTcsHub();
    }

    m_isNavigating = false;
}

void MainWindow::openTdsTcsHub() {
    MahadevERP::MenuTreeManager::instance().executeMenu("tds_tcs_hub", this);
}

void MainWindow::openTdsOptions() {
    MahadevERP::MenuTreeManager::instance().executeMenu("tds_options", this);
}

void MainWindow::openTcsOptions() {
    MahadevERP::MenuTreeManager::instance().executeMenu("tcs_options", this);
}

void MainWindow::onLedgerBackRequested() {
    m_targetStatementParty.clear();
    m_lastViewedStatementFromDate.clear();
    m_lastViewedStatementToDate.clear();
    if (m_previousViewIndex == 29) {
        m_previousViewIndex = 0;
        navigateToView(29);
        return;
    }
    if (m_previousViewIndex == 30) {
        m_previousViewIndex = 0;
        navigateToView(30);
        return;
    }
    navigateToView(0);
}

void MainWindow::onSalesVoucherBackRequested() {
    if (m_previousViewIndex == 8) {
        m_previousViewIndex = 0;
        navigateToView(8);
        return;
    }
    navigateToView(0);
}

void MainWindow::onSalesVoucherSaved(const QString& invoiceNo) {
    Q_UNUSED(invoiceNo);
    if (m_previousViewIndex == 8) {
        m_previousViewIndex = 0;
        navigateToView(8);
        return;
    }
    navigateToView(0);
}

void MainWindow::onPurchaseVoucherBackRequested() {
    if (m_previousViewIndex == 8) {
        m_previousViewIndex = 0;
        navigateToView(8);
        return;
    }
    navigateToView(0);
}

void MainWindow::onPurchaseVoucherSaved(const QString& invoiceNo) {
    Q_UNUSED(invoiceNo);
    if (m_previousViewIndex == 8) {
        m_previousViewIndex = 0;
        navigateToView(8);
        return;
    }
    navigateToView(0);
}

void MainWindow::onChequeVoucherBackRequested() {
    if (m_previousViewIndex == 8) {
        m_previousViewIndex = 0;
        navigateToView(8);
        return;
    }
    navigateToView(0);
}

void MainWindow::onChequeVoucherSaved(const QString& voucherNo) {
    Q_UNUSED(voucherNo);
    if (m_previousViewIndex == 8) {
        m_previousViewIndex = 0;
        navigateToView(8);
        return;
    }
    navigateToView(0);
}

void MainWindow::onJournalVoucherBackRequested() {
    if (m_previousViewIndex == 8) {
        m_previousViewIndex = 0;
        navigateToView(8);
        return;
    }
    navigateToView(0);
}

void MainWindow::onJournalVoucherSaved(const QString& voucherNo) {
    Q_UNUSED(voucherNo);
    if (m_previousViewIndex == 8) {
        m_previousViewIndex = 0;
        navigateToView(8);
        return;
    }
    navigateToView(0);
}

void MainWindow::onNewLedgerBackRequested() {
    navigateToView(0);
}

void MainWindow::onNewLedgerSaved() {
    navigateToView(0);
}

void MainWindow::onModifyLedgerBackRequested() {
    navigateToView(0);
}

void MainWindow::onModifyLedgerSaved() {
    navigateToView(0);
}

void MainWindow::onJFormVoucherBackRequested() {
    navigateToView(0);
}

void MainWindow::onJFormVoucherSaved(const QString& jformNo) {
    Q_UNUSED(jformNo);
    navigateToView(0);
}

void MainWindow::onIFormVoucherBackRequested() {
    navigateToView(0);
}

void MainWindow::onIFormVoucherSaved(const QString& iformNo) {
    Q_UNUSED(iformNo);
    navigateToView(0);
}

void MainWindow::onMandiReportsBackRequested() {
    navigateToView(0);
}

void MainWindow::onBalanceSheetBackRequested() {
    navigateToView(0);
}

void MainWindow::onBalanceSheetPartyStatementRequested(const QString& partyName, const QString& fromDate, const QString& toDate) {
    m_previousViewIndex = 29;
    m_targetStatementParty = partyName;
    m_lastViewedStatementFromDate = fromDate;
    m_lastViewedStatementToDate = toDate;
    navigateToView(8);
}

void MainWindow::onProfitLossBackRequested() {
    navigateToView(0);
}

void MainWindow::onProfitLossPartyStatementRequested(const QString& partyName, const QString& fromDate, const QString& toDate) {
    m_previousViewIndex = 30;
    m_targetStatementParty = partyName;
    m_lastViewedStatementFromDate = fromDate;
    m_lastViewedStatementToDate = toDate;
    navigateToView(8);
}

void MainWindow::onLedgerAlterVoucherRequested(int targetViewIndex, const QVariantMap& entry) {
    m_previousViewIndex = currentViewIndex();
    m_pendingEditInvoiceNo = entry.value("invoiceNo", entry.value("voucherNo", "")).toString();
    m_pendingEditVoucherNo = entry.value("voucherNo", "").toString();
    m_pendingEditVoucherId = entry.value("id", 0).toInt();
    m_pendingEditVoucherDate = entry.value("date", "").toString();
    if (entry.contains("type")) {
        m_targetChequeMode = entry.value("type").toString();
    }
    navigateToView(targetViewIndex);
}

void MainWindow::closeEvent(QCloseEvent* event) {
    QMainWindow::closeEvent(event);
}
