#include "main_window.h"
#include "accounting_period_dialog.h"
#include "../engine/fiscal_year_helper.h"
#include "../services/app_keyboard_controller.h"
#include <QQuickItem>
#include <QTimer>
#include <QShortcut>
#include <QQmlEngine>
#include <QQmlContext>
#include <QDebug>

MainWindow::MainWindow(QQuickWindow* qmlWindow,
                       LedgerStatementController* ledgerCtrl,
                       PrintExportController* printExportCtrl,
                       DashboardController* dashCtrl,
                       FirmManager* firmMgr,
                       BahiKhataMigrator* migrator,
                       QWidget* parent)
    : QMainWindow(parent)
    , m_qmlWindow(qmlWindow)
    , m_ledgerCtrl(ledgerCtrl)
    , m_printExportCtrl(printExportCtrl)
    , m_dashCtrl(dashCtrl)
    , m_firmMgr(firmMgr)
    , m_bahiKhataMigrator(migrator)
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

    QQmlEngine* engine = m_qmlWindow ? qmlEngine(m_qmlWindow) : nullptr;
    QQmlContext* ctx = engine ? engine->rootContext() : nullptr;

    auto getProp = [ctx](const char* name) -> QVariant {
        if (!ctx) return QVariant();
        return ctx->contextProperty(name);
    };

    AccountGroupsModel* groupsModel = getProp("groupsModel").value<AccountGroupsModel*>();
    StockMasterController* stockMasterCtrl = getProp("stockMasterCtrl").value<StockMasterController*>();
    StockItemsModel* stockItemsModel = getProp("stockItemsModel").value<StockItemsModel*>();
    SalesRegisterController* salesRegisterCtrl = getProp("salesRegisterCtrl").value<SalesRegisterController*>();
    PurchaseRegisterController* purchaseRegisterCtrl = getProp("purchaseRegisterCtrl").value<PurchaseRegisterController*>();
    StockRegisterController* stockRegisterCtrl = getProp("stockRegisterCtrl").value<StockRegisterController*>();
    PaddyArrivalsModel* paddyModel = getProp("paddyModel").value<PaddyArrivalsModel*>();
    PaddyProcurementController* paddyProcurementCtrl = getProp("paddyProcurementCtrl").value<PaddyProcurementController*>();
    MillingBatchController* millingBatchCtrl = getProp("millingBatchCtrl").value<MillingBatchController*>();
    MillingModel* millingModel = getProp("millingModel").value<MillingModel*>();
    TdsVoucherController* tdsVoucherCtrl = getProp("tdsVoucherCtrl").value<TdsVoucherController*>();
    InterestModel* interestModel = getProp("interestModel").value<InterestModel*>();
    BankStatementController* bankStatementCtrl = getProp("bankStatementCtrl").value<BankStatementController*>();
    TransportDispatchController* transportDispatchCtrl = getProp("transportDispatchCtrl").value<TransportDispatchController*>();
    DebitCreditNoteController* debitCreditNoteCtrl = getProp("debitCreditNoteCtrl").value<DebitCreditNoteController*>();

    // Index 0: Native C++ Main Dashboard Widget (View 0)
    m_dashboardWidget = new DashboardWidget(m_dashCtrl, m_firmMgr, m_printExportCtrl, m_bahiKhataMigrator, this);
    connect(m_dashboardWidget, &DashboardWidget::openViewRequested, this, &MainWindow::navigateToView);
    connect(m_dashboardWidget, &DashboardWidget::requestAccountingPeriodDialog, this, &MainWindow::openAccountingPeriodDialog);
    m_stackedWidget->addWidget(m_dashboardWidget);

    // Embedded QML Application Window
    if (m_qmlWindow) {
        m_qmlContainer = QWidget::createWindowContainer(m_qmlWindow, this);
        m_qmlContainer->setFocusPolicy(Qt::StrongFocus);
        m_stackedWidget->addWidget(m_qmlContainer);
    }

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
        m_previousViewIndex = 29;
        if (m_stockDetailWidget) {
            // open stock detail
        }
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
    m_newGroupWidget = new MahadevERP::NewGroupWidget(groupsModel, this);
    connect(m_newGroupWidget, &MahadevERP::NewGroupWidget::backRequested, this, [this]() { navigateToView(0); });
    connect(m_newGroupWidget, &MahadevERP::NewGroupWidget::savedSuccess, this, [this]() { navigateToView(0); });
    m_stackedWidget->addWidget(m_newGroupWidget);

    // Index 19: Native C++ Modify Group Widget (View 10)
    m_modifyGroupWidget = new MahadevERP::ModifyGroupWidget(groupsModel, this);
    connect(m_modifyGroupWidget, &MahadevERP::ModifyGroupWidget::backRequested, this, [this]() { navigateToView(0); });
    connect(m_modifyGroupWidget, &MahadevERP::ModifyGroupWidget::savedSuccess, this, [this]() { navigateToView(0); });
    m_stackedWidget->addWidget(m_modifyGroupWidget);

    // Index 20: Native C++ New Stock Item Widget (View 11)
    m_newStockItemWidget = new MahadevERP::NewStockItemWidget(stockMasterCtrl, stockItemsModel, this);
    connect(m_newStockItemWidget, &MahadevERP::NewStockItemWidget::backRequested, this, [this]() { navigateToView(0); });
    connect(m_newStockItemWidget, &MahadevERP::NewStockItemWidget::savedSuccess, this, [this]() { navigateToView(0); });
    m_stackedWidget->addWidget(m_newStockItemWidget);

    // Index 21: Native C++ Modify Stock Item Widget (View 12)
    m_modifyStockItemWidget = new MahadevERP::ModifyStockItemWidget(stockMasterCtrl, stockItemsModel, this);
    connect(m_modifyStockItemWidget, &MahadevERP::ModifyStockItemWidget::backRequested, this, [this]() { navigateToView(0); });
    connect(m_modifyStockItemWidget, &MahadevERP::ModifyStockItemWidget::savedSuccess, this, [this]() { navigateToView(0); });
    m_stackedWidget->addWidget(m_modifyStockItemWidget);

    // Index 22: Native C++ Sales Register Widget (View 3)
    m_salesRegisterWidget = new MahadevERP::SalesRegisterWidget(salesRegisterCtrl, m_printExportCtrl, this);
    connect(m_salesRegisterWidget, &MahadevERP::SalesRegisterWidget::backRequested, this, [this]() { navigateToView(0); });
    connect(m_salesRegisterWidget, &MahadevERP::SalesRegisterWidget::alterInvoiceRequested, this, &MainWindow::onLedgerAlterVoucherRequested);
    connect(m_salesRegisterWidget, &MahadevERP::SalesRegisterWidget::newInvoiceRequested, this, [this]() { navigateToView(14); });
    m_stackedWidget->addWidget(m_salesRegisterWidget);

    // Index 23: Native C++ Purchase Register Widget (View 4 / 21)
    m_purchaseRegisterWidget = new MahadevERP::PurchaseRegisterWidget(purchaseRegisterCtrl, m_printExportCtrl, this);
    connect(m_purchaseRegisterWidget, &MahadevERP::PurchaseRegisterWidget::backRequested, this, [this]() { navigateToView(0); });
    connect(m_purchaseRegisterWidget, &MahadevERP::PurchaseRegisterWidget::alterBillRequested, this, &MainWindow::onLedgerAlterVoucherRequested);
    connect(m_purchaseRegisterWidget, &MahadevERP::PurchaseRegisterWidget::newBillRequested, this, [this]() { navigateToView(15); });
    m_stackedWidget->addWidget(m_purchaseRegisterWidget);

    // Index 24: Native C++ Stock Detail Widget (View 13)
    m_stockDetailWidget = new MahadevERP::StockDetailWidget(stockRegisterCtrl, m_printExportCtrl, this);
    connect(m_stockDetailWidget, &MahadevERP::StockDetailWidget::backRequested, this, [this]() { navigateToView(0); });
    m_stackedWidget->addWidget(m_stockDetailWidget);

    // Index 25: Native C++ Paddy Procurement Widget (View 1)
    m_paddyProcurementWidget = new MahadevERP::PaddyProcurementWidget(paddyModel, paddyProcurementCtrl, m_printExportCtrl, this);
    connect(m_paddyProcurementWidget, &MahadevERP::PaddyProcurementWidget::backRequested, this, [this]() { navigateToView(0); });
    m_stackedWidget->addWidget(m_paddyProcurementWidget);

    // Index 26: Native C++ Milling Voucher Widget (View 31)
    m_millingVoucherWidget = new MahadevERP::MillingVoucherWidget(millingBatchCtrl, millingModel, m_printExportCtrl, this);
    connect(m_millingVoucherWidget, &MahadevERP::MillingVoucherWidget::backRequested, this, [this]() { navigateToView(0); });
    connect(m_millingVoucherWidget, &MahadevERP::MillingVoucherWidget::batchSaved, this, [this](const QString& batchNo) {
        Q_UNUSED(batchNo);
        navigateToView(0);
    });
    m_stackedWidget->addWidget(m_millingVoucherWidget);

    // Index 27: Native C++ TDS Voucher Widget (View 24)
    m_tdsVoucherWidget = new MahadevERP::TdsVoucherWidget(tdsVoucherCtrl, m_printExportCtrl, this);
    connect(m_tdsVoucherWidget, &MahadevERP::TdsVoucherWidget::backRequested, this, [this]() { navigateToView(0); });
    connect(m_tdsVoucherWidget, &MahadevERP::TdsVoucherWidget::voucherSaved, this, [this](int voucherNo) {
        Q_UNUSED(voucherNo);
        navigateToView(0);
    });
    m_stackedWidget->addWidget(m_tdsVoucherWidget);

    // Index 28: Native C++ Interest Calculator Widget (View 25)
    m_interestCalcWidget = new MahadevERP::InterestCalculatorWidget(interestModel, m_printExportCtrl, this);
    connect(m_interestCalcWidget, &MahadevERP::InterestCalculatorWidget::backRequested, this, [this]() { navigateToView(0); });
    m_stackedWidget->addWidget(m_interestCalcWidget);

    // Index 29: Native C++ Bank Statement Import Widget (View 26)
    m_bankStatementWidget = new MahadevERP::BankStatementImportWidget(bankStatementCtrl, m_printExportCtrl, this);
    connect(m_bankStatementWidget, &MahadevERP::BankStatementImportWidget::backRequested, this, [this]() { navigateToView(0); });
    connect(m_bankStatementWidget, &MahadevERP::BankStatementImportWidget::importCompleted, this, [this]() { navigateToView(0); });
    m_stackedWidget->addWidget(m_bankStatementWidget);

    // Index 30: Native C++ Transport Dispatch Widget (View 27)
    m_transportDispatchWidget = new MahadevERP::TransportDispatchWidget(transportDispatchCtrl, m_printExportCtrl, this);
    connect(m_transportDispatchWidget, &MahadevERP::TransportDispatchWidget::backRequested, this, [this]() { navigateToView(0); });
    m_stackedWidget->addWidget(m_transportDispatchWidget);

    // Index 31: Native C++ Debit / Credit Note Widget (View 28)
    m_debitCreditNoteWidget = new MahadevERP::DebitCreditNoteWidget(debitCreditNoteCtrl, m_printExportCtrl, this);
    connect(m_debitCreditNoteWidget, &MahadevERP::DebitCreditNoteWidget::backRequested, this, [this]() { navigateToView(0); });
    connect(m_debitCreditNoteWidget, &MahadevERP::DebitCreditNoteWidget::noteSaved, this, [this](const QString& noteNo) {
        Q_UNUSED(noteNo);
        navigateToView(0);
    });
    m_stackedWidget->addWidget(m_debitCreditNoteWidget);

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

    if (m_qmlWindow) {
        connect(m_qmlWindow, SIGNAL(requestAccountingPeriodDialog()), this, SLOT(openAccountingPeriodDialog()));
        connect(m_qmlWindow, SIGNAL(currentViewIndexChanged()), this, SLOT(checkQmlView()));

        FiscalYearInfo activeFy = FiscalYearHelper::getActiveFiscalYear();
        QString s_fmt = FiscalYearHelper::formatDisplayDate(activeFy.startDate);
        QString e_fmt = FiscalYearHelper::formatDisplayDate(activeFy.endDate);
        QString label = QString("%1 To %2 (%3)").arg(s_fmt, e_fmt, activeFy.name);
        m_qmlWindow->setProperty("activePeriodLabel", label);
    }

    // Unified Application-Wide Keyboard Navigation Controller
    m_keyboardCtrl = new AppKeyboardController(this, this);
    if (m_qmlWindow) {
        if (engine && engine->rootContext()) {
            engine->rootContext()->setContextProperty("keyboardCtrl", m_keyboardCtrl);
        }
    }

    // Monitor QML View Index navigation via responsive check
    QTimer* viewMonitor = new QTimer(this);
    connect(viewMonitor, &QTimer::timeout, this, &MainWindow::checkQmlView);
    viewMonitor->start(50); // Fast 50ms responsive check

    // Explicitly start application on Firm Selector View (View 22)
    navigateToView(22);
}

void MainWindow::checkQmlView() {
    if (!m_qmlWindow) return;
    QObject* root = m_qmlWindow;

    int vIdx = root->property("currentViewIndex").toInt();
    if (vIdx == 0 && m_stackedWidget->currentWidget() != m_dashboardWidget) {
        navigateToView(0);
    } else if (vIdx == 1 && m_stackedWidget->currentWidget() != m_paddyProcurementWidget) {
        navigateToView(1);
    } else if (vIdx == 3 && m_stackedWidget->currentWidget() != m_salesRegisterWidget) {
        navigateToView(3);
    } else if ((vIdx == 4 || vIdx == 21) && m_stackedWidget->currentWidget() != m_purchaseRegisterWidget) {
        navigateToView(vIdx);
    } else if (vIdx == 8 && (m_stackedWidget->currentWidget() != m_ledgerWidget || !root->property("targetStatementParty").toString().isEmpty())) {
        navigateToView(8);
    } else if (vIdx == 9 && m_stackedWidget->currentWidget() != m_newGroupWidget) {
        navigateToView(9);
    } else if (vIdx == 10 && m_stackedWidget->currentWidget() != m_modifyGroupWidget) {
        navigateToView(10);
    } else if (vIdx == 11 && m_stackedWidget->currentWidget() != m_newStockItemWidget) {
        navigateToView(11);
    } else if (vIdx == 12 && m_stackedWidget->currentWidget() != m_modifyStockItemWidget) {
        navigateToView(12);
    } else if (vIdx == 13 && m_stackedWidget->currentWidget() != m_stockDetailWidget) {
        navigateToView(13);
    } else if (vIdx == 14 && (m_stackedWidget->currentWidget() != m_salesVoucherWidget || !root->property("pendingEditInvoiceNo").toString().isEmpty())) {
        navigateToView(14);
    } else if (vIdx == 15 && (m_stackedWidget->currentWidget() != m_purchaseVoucherWidget || !root->property("pendingEditInvoiceNo").toString().isEmpty())) {
        navigateToView(15);
    } else if (vIdx == 16 && (m_stackedWidget->currentWidget() != m_chequeVoucherWidget || !root->property("pendingEditVoucherNo").toString().isEmpty() || !root->property("targetChequeMode").toString().isEmpty())) {
        navigateToView(16);
    } else if (vIdx == 17 && (m_stackedWidget->currentWidget() != m_journalVoucherWidget || !root->property("pendingEditVoucherNo").toString().isEmpty())) {
        navigateToView(17);
    } else if (vIdx == 18 && (m_stackedWidget->currentWidget() != m_jformVoucherWidget || !root->property("pendingEditVoucherNo").toString().isEmpty())) {
        navigateToView(18);
    } else if (vIdx == 19 && (m_stackedWidget->currentWidget() != m_iformVoucherWidget || !root->property("pendingEditVoucherNo").toString().isEmpty())) {
        navigateToView(19);
    } else if (vIdx == 20 && m_stackedWidget->currentWidget() != m_mandiReportsWidget) {
        navigateToView(20);
    } else if (vIdx == 22 && m_stackedWidget->currentWidget() != m_firmSelectorWidget) {
        navigateToView(22);
    } else if (vIdx == 24 && m_stackedWidget->currentWidget() != m_tdsVoucherWidget) {
        navigateToView(24);
    } else if (vIdx == 25 && m_stackedWidget->currentWidget() != m_interestCalcWidget) {
        navigateToView(25);
    } else if (vIdx == 26 && m_stackedWidget->currentWidget() != m_bankStatementWidget) {
        navigateToView(26);
    } else if (vIdx == 27 && m_stackedWidget->currentWidget() != m_transportDispatchWidget) {
        navigateToView(27);
    } else if (vIdx == 28 && m_stackedWidget->currentWidget() != m_debitCreditNoteWidget) {
        navigateToView(28);
    } else if (vIdx == 29 && m_stackedWidget->currentWidget() != m_balanceSheetWidget) {
        navigateToView(29);
    } else if (vIdx == 30 && m_stackedWidget->currentWidget() != m_profitLossWidget) {
        navigateToView(30);
    } else if (vIdx == 31 && m_stackedWidget->currentWidget() != m_millingVoucherWidget) {
        navigateToView(31);
    } else if (vIdx == 33 && m_stackedWidget->currentWidget() != m_dayBookWidget) {
        navigateToView(33);
    } else if (vIdx == 34 && m_stackedWidget->currentWidget() != m_gstrReportsWidget) {
        navigateToView(34);
    } else if (vIdx == 35 && m_stackedWidget->currentWidget() != m_millingStatementWidget) {
        navigateToView(35);
    } else if (vIdx == 36 && m_stackedWidget->currentWidget() != m_customClosingStockWidget) {
        navigateToView(36);
    } else if (vIdx == 6 && m_stackedWidget->currentWidget() != m_newLedgerWidget) {
        navigateToView(6);
    } else if (vIdx == 7 && m_stackedWidget->currentWidget() != m_modifyLedgerWidget) {
        navigateToView(7);
    }
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
    if (m_qmlWindow) {
        return m_qmlWindow->property("currentViewIndex").toInt();
    }
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
        QString s_fmt = FiscalYearHelper::formatDisplayDate(fIso);
        QString e_fmt = FiscalYearHelper::formatDisplayDate(tIso);
        QString label = QString("%1 To %2 (%3)").arg(s_fmt, e_fmt, fyLabel);

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

    QObject* root = m_qmlWindow;
    QString pendingInv = root ? root->property("pendingEditInvoiceNo").toString() : "";
    QString pendingVNo = root ? root->property("pendingEditVoucherNo").toString() : "";
    int pendingId = root ? root->property("pendingEditVoucherId").toInt() : 0;
    QString pendingDate = root ? root->property("pendingEditVoucherDate").toString() : "";

    if (root) {
        root->setProperty("pendingEditInvoiceNo", "");
        root->setProperty("pendingEditVoucherNo", "");
        root->setProperty("pendingEditVoucherId", 0);
        root->setProperty("pendingEditVoucherDate", "");

        int curQmlIdx = root->property("currentViewIndex").toInt();
        if (curQmlIdx != viewIndex) {
            root->setProperty("currentViewIndex", viewIndex);
        }
    }

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
        QString party = root ? root->property("targetStatementParty").toString().trimmed() : "";
        QString fromDate = root ? root->property("lastViewedStatementFromDate").toString() : "";
        QString toDate = root ? root->property("lastViewedStatementToDate").toString() : "";
        QString side = root ? root->property("lastViewedStatementSide").toString() : "Dr";
        int rowIndex = root ? root->property("lastViewedStatementIndex").toInt() : 0;

        if (root) {
            root->setProperty("targetStatementParty", "");
        }

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
            QString mode = root ? root->property("targetChequeMode").toString() : "";
            if (root) root->setProperty("targetChequeMode", "");
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
    }

    m_isNavigating = false;
}

void MainWindow::onLedgerBackRequested() {
    if (m_qmlWindow) {
        m_qmlWindow->setProperty("lastViewedStatementParty", "");
        m_qmlWindow->setProperty("targetStatementParty", "");
        m_qmlWindow->setProperty("lastViewedPartyName", "");
    }
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
    if (m_qmlWindow) {
        m_qmlWindow->setProperty("targetStatementParty", partyName);
        m_qmlWindow->setProperty("lastViewedStatementFromDate", fromDate);
        m_qmlWindow->setProperty("lastViewedStatementToDate", toDate);
    }
    navigateToView(8);
}

void MainWindow::onProfitLossBackRequested() {
    navigateToView(0);
}

void MainWindow::onProfitLossPartyStatementRequested(const QString& partyName, const QString& fromDate, const QString& toDate) {
    m_previousViewIndex = 30;
    if (m_qmlWindow) {
        m_qmlWindow->setProperty("targetStatementParty", partyName);
        m_qmlWindow->setProperty("lastViewedStatementFromDate", fromDate);
        m_qmlWindow->setProperty("lastViewedStatementToDate", toDate);
    }
    navigateToView(8);
}

void MainWindow::onLedgerAlterVoucherRequested(int targetViewIndex, const QVariantMap& entry) {
    m_previousViewIndex = currentViewIndex();
    if (m_qmlWindow) {
        m_qmlWindow->setProperty("pendingEditInvoiceNo", entry.value("invoiceNo", entry.value("voucherNo", "")).toString());
        m_qmlWindow->setProperty("pendingEditVoucherNo", entry.value("voucherNo", "").toString());
        m_qmlWindow->setProperty("pendingEditVoucherId", entry.value("id", 0).toInt());
        m_qmlWindow->setProperty("pendingEditVoucherDate", entry.value("date", "").toString());
        if (entry.contains("type")) {
            m_qmlWindow->setProperty("targetChequeMode", entry.value("type").toString());
        }
    }
    navigateToView(targetViewIndex);
}

void MainWindow::closeEvent(QCloseEvent* event) {
    QMainWindow::closeEvent(event);
}
