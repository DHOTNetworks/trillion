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
    setAttribute(Qt::WA_StyledBackground, true);
    setStyleSheet("QMainWindow { background-color: #F8FAFC; } QStackedWidget { background-color: #F8FAFC; }");

    m_stackedWidget = new QStackedWidget(this);
    m_stackedWidget->setAttribute(Qt::WA_StyledBackground, true);
    setCentralWidget(m_stackedWidget);

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

    // Index 1: Native C++ Ledger Statement Widget
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
        if (m_qmlWindow) {
            QQmlEngine* engine = qmlEngine(m_qmlWindow);
            if (engine && engine->rootContext()) {
                QObject* stockCtrl = engine->rootContext()->contextProperty("stockRegisterCtrl").value<QObject*>();
                if (stockCtrl) {
                    QString filter = itemName.trimmed();
                    if (filter.contains("Closing Stock", Qt::CaseInsensitive) ||
                        filter.contains("Opening Stock", Qt::CaseInsensitive) ||
                        filter.contains("Trading Items", Qt::CaseInsensitive)) {
                        filter = "";
                    }
                    stockCtrl->setProperty("searchQuery", filter);
                }
            }
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
        if (m_qmlWindow) {
            QQmlEngine* engine = qmlEngine(m_qmlWindow);
            if (engine && engine->rootContext()) {
                QObject* stockCtrl = engine->rootContext()->contextProperty("stockRegisterCtrl").value<QObject*>();
                if (stockCtrl) {
                    QString filter = itemName.trimmed();
                    if (filter.contains("Closing Stock", Qt::CaseInsensitive) ||
                        filter.contains("Opening Stock", Qt::CaseInsensitive) ||
                        filter.contains("Trading Items", Qt::CaseInsensitive)) {
                        filter = "";
                    }
                    stockCtrl->setProperty("searchQuery", filter);
                }
            }
        }
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
        QQmlEngine* engine = qmlEngine(m_qmlWindow);
        if (engine && engine->rootContext()) {
            engine->rootContext()->setContextProperty("keyboardCtrl", m_keyboardCtrl);
        }
    }

    // Monitor QML View Index navigation via responsive check
    QTimer* viewMonitor = new QTimer(this);
    connect(viewMonitor, &QTimer::timeout, this, &MainWindow::checkQmlView);
    viewMonitor->start(50); // Fast 50ms responsive check
}

void MainWindow::checkQmlView() {
    if (!m_qmlWindow) return;
    QObject* root = m_qmlWindow;

    // View Index navigation
    QString pendingInv = root ? root->property("pendingEditInvoiceNo").toString() : "";
    QString pendingVNo = root ? root->property("pendingEditVoucherNo").toString() : "";
    int pendingId = root ? root->property("pendingEditVoucherId").toInt() : 0;
    bool hasPending = (pendingId > 0 || !pendingInv.isEmpty() || !pendingVNo.isEmpty());

    int vIdx = root->property("currentViewIndex").toInt();
    if (vIdx == 0 && m_stackedWidget->currentWidget() != m_dashboardWidget) {
        navigateToView(0);
    } else if (vIdx == 8 && (m_stackedWidget->currentWidget() != m_ledgerWidget || !root->property("targetStatementParty").toString().isEmpty())) {
        navigateToView(8);
    } else if (vIdx == 29 && m_stackedWidget->currentWidget() != m_balanceSheetWidget) {
        navigateToView(29);
    } else if (vIdx == 30 && m_stackedWidget->currentWidget() != m_profitLossWidget) {
        navigateToView(30);
    } else if (vIdx == 14 && (m_stackedWidget->currentWidget() != m_salesVoucherWidget || hasPending)) {
        navigateToView(14);
    } else if (vIdx == 15 && (m_stackedWidget->currentWidget() != m_purchaseVoucherWidget || hasPending)) {
        navigateToView(15);
    } else if (vIdx == 16 && (m_stackedWidget->currentWidget() != m_chequeVoucherWidget || hasPending || !root->property("targetChequeMode").toString().isEmpty())) {
        navigateToView(16);
    } else if (vIdx == 17 && (m_stackedWidget->currentWidget() != m_journalVoucherWidget || hasPending)) {
        navigateToView(17);
    } else if (vIdx == 22 && m_stackedWidget->currentWidget() != m_firmSelectorWidget) {
        navigateToView(22);
    } else if (vIdx == 6 && m_stackedWidget->currentWidget() != m_newLedgerWidget) {
        navigateToView(6);
    } else if (vIdx == 7 && m_stackedWidget->currentWidget() != m_modifyLedgerWidget) {
        navigateToView(7);
    } else if (vIdx != 0 && vIdx != 8 && vIdx != 29 && vIdx != 30 && vIdx != 14 && vIdx != 15 && vIdx != 16 && vIdx != 17 && vIdx != 22 && vIdx != 6 && vIdx != 7 && m_stackedWidget->currentWidget() != m_qmlContainer) {
        m_stackedWidget->setCurrentWidget(m_qmlContainer);
        if (m_qmlContainer) {
            m_qmlContainer->setFocus(Qt::OtherFocusReason);
        }
        if (m_qmlWindow) {
            m_qmlWindow->requestActivate();
            if (m_qmlWindow->contentItem()) {
                m_qmlWindow->contentItem()->forceActiveFocus(Qt::OtherFocusReason);
            }
        }
    }
}

int MainWindow::currentViewIndex() const {
    if (!m_stackedWidget) return 0;
    QWidget* cur = m_stackedWidget->currentWidget();
    if (cur == m_dashboardWidget) return 0;
    if (cur == m_ledgerWidget) return 8;
    if (cur == m_balanceSheetWidget) return 29;
    if (cur == m_profitLossWidget) return 30;
    if (cur == m_salesVoucherWidget) return 14;
    if (cur == m_purchaseVoucherWidget) return 15;
    if (cur == m_chequeVoucherWidget) return 16;
    if (cur == m_journalVoucherWidget) return 17;
    if (cur == m_firmSelectorWidget) return 22;
    if (cur == m_newLedgerWidget) return 6;
    if (cur == m_modifyLedgerWidget) return 7;
    if (m_qmlWindow) {
        return m_qmlWindow->property("currentViewIndex").toInt();
    }
    return 0;
}

void MainWindow::restoreActiveViewFocus() {
    int vIdx = currentViewIndex();
    if (vIdx == 0 && m_dashboardWidget) {
        m_dashboardWidget->setFocus(Qt::OtherFocusReason);
    } else if (vIdx == 14 && m_salesVoucherWidget) {
        m_salesVoucherWidget->setFocus(Qt::OtherFocusReason);
    } else if (vIdx == 15 && m_purchaseVoucherWidget) {
        m_purchaseVoucherWidget->setFocus(Qt::OtherFocusReason);
    } else if (vIdx == 16 && m_chequeVoucherWidget) {
        m_chequeVoucherWidget->setFocus(Qt::OtherFocusReason);
    } else if (vIdx == 17 && m_journalVoucherWidget) {
        m_journalVoucherWidget->setFocus(Qt::OtherFocusReason);
    } else if (vIdx == 6 && m_newLedgerWidget) {
        m_newLedgerWidget->setFocus(Qt::OtherFocusReason);
        m_newLedgerWidget->focusFirstField();
    } else if (vIdx == 7 && m_modifyLedgerWidget) {
        m_modifyLedgerWidget->setFocus(Qt::OtherFocusReason);
        m_modifyLedgerWidget->focusSearch();
    } else if (vIdx == 22 && m_firmSelectorWidget) {
        m_firmSelectorWidget->setFocus(Qt::OtherFocusReason);
        m_firmSelectorWidget->focusTable();
    } else if (vIdx == 29 && m_balanceSheetWidget) {
        m_balanceSheetWidget->setFocus(Qt::OtherFocusReason);
        if (m_balanceSheetWidget->liabilitiesTree() && m_balanceSheetWidget->liabilitiesTree()->topLevelItemCount() > 0) {
            m_balanceSheetWidget->liabilitiesTree()->setFocus(Qt::OtherFocusReason);
            if (!m_balanceSheetWidget->liabilitiesTree()->currentItem()) {
                m_balanceSheetWidget->liabilitiesTree()->setCurrentItem(m_balanceSheetWidget->liabilitiesTree()->topLevelItem(0));
            }
        }
    } else if (vIdx == 30 && m_profitLossWidget) {
        m_profitLossWidget->setFocus(Qt::OtherFocusReason);
        if (m_profitLossWidget->expensesTree() && m_profitLossWidget->expensesTree()->topLevelItemCount() > 0) {
            m_profitLossWidget->expensesTree()->setFocus(Qt::OtherFocusReason);
            if (!m_profitLossWidget->expensesTree()->currentItem()) {
                m_profitLossWidget->expensesTree()->setCurrentItem(m_profitLossWidget->expensesTree()->topLevelItem(0));
            }
        }
    } else if (vIdx == 8 && m_ledgerWidget) {
        m_ledgerWidget->setFocus(Qt::OtherFocusReason);
        if (m_ledgerWidget->drTable() && m_ledgerWidget->drTable()->model() && m_ledgerWidget->drTable()->model()->rowCount() > 0) {
            m_ledgerWidget->drTable()->setFocus(Qt::OtherFocusReason);
        } else if (m_ledgerWidget->crTable() && m_ledgerWidget->crTable()->model() && m_ledgerWidget->crTable()->model()->rowCount() > 0) {
            m_ledgerWidget->crTable()->setFocus(Qt::OtherFocusReason);
        } else if (m_ledgerWidget->searchBox()) {
            m_ledgerWidget->searchBox()->setFocus(Qt::OtherFocusReason);
        }
    } else {
        if (m_qmlContainer) {
            m_qmlContainer->setFocus(Qt::OtherFocusReason);
        }
        if (m_qmlWindow) {
            m_qmlWindow->requestActivate();
            if (m_qmlWindow->contentItem()) {
                m_qmlWindow->contentItem()->forceActiveFocus(Qt::OtherFocusReason);
            }
            QMetaObject::invokeMethod(m_qmlWindow, "restoreActiveViewFocus");
        }
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

        if (m_qmlWindow) {
            m_qmlWindow->setProperty("activePeriodLabel", label);

            QQmlEngine* engine = qmlEngine(m_qmlWindow);
            if (engine && engine->rootContext()) {
                QObject* stockObj = engine->rootContext()->contextProperty("stockItemsModel").value<QObject*>();
                if (stockObj) {
                    QMetaObject::invokeMethod(stockObj, "set_accounting_period",
                                              Q_ARG(QString, fIso),
                                              Q_ARG(QString, tIso),
                                              Q_ARG(QString, fyLabel));
                }
                QObject* dashObj = engine->rootContext()->contextProperty("dashboardCtrl").value<QObject*>();
                if (dashObj) {
                    QMetaObject::invokeMethod(dashObj, "refresh_stats",
                                              Q_ARG(QString, fIso),
                                              Q_ARG(QString, tIso),
                                              Q_ARG(QString, fyLabel));
                }
            }
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
    }

    if (m_stackedWidget->currentWidget() == m_qmlContainer) {
        if (m_qmlContainer) m_qmlContainer->setFocus(Qt::OtherFocusReason);
        if (m_qmlWindow) {
            m_qmlWindow->requestActivate();
            if (m_qmlWindow->contentItem()) {
                m_qmlWindow->contentItem()->forceActiveFocus(Qt::OtherFocusReason);
            }
        }
    } else if (m_stackedWidget->currentWidget() == m_ledgerWidget) {
        if (m_ledgerWidget) m_ledgerWidget->setFocus();
    } else if (m_stackedWidget->currentWidget() == m_balanceSheetWidget) {
        if (m_balanceSheetWidget) m_balanceSheetWidget->setFocus();
    } else if (m_stackedWidget->currentWidget() == m_profitLossWidget) {
        if (m_profitLossWidget) m_profitLossWidget->setFocus();
    } else if (m_stackedWidget->currentWidget() == m_chequeVoucherWidget) {
        if (m_chequeVoucherWidget) m_chequeVoucherWidget->setFocus();
    } else if (m_stackedWidget->currentWidget() == m_journalVoucherWidget) {
        if (m_journalVoucherWidget) m_journalVoucherWidget->setFocus();
    }
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
        // Show Native C++ Dashboard Widget
        if (m_dashboardWidget) {
            m_stackedWidget->setCurrentWidget(m_dashboardWidget);
            m_dashboardWidget->setFocus();
            m_dashboardWidget->refreshStats();
        }
    } else if (viewIndex == 8) {
        // Show C++ Ledger Statement Widget
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
            if (!party.isEmpty()) {
                if (m_ledgerWidget->drTable() && m_ledgerWidget->drTable()->model() && m_ledgerWidget->drTable()->model()->rowCount() > 0) {
                    m_ledgerWidget->drTable()->setFocus(Qt::OtherFocusReason);
                } else if (m_ledgerWidget->crTable() && m_ledgerWidget->crTable()->model() && m_ledgerWidget->crTable()->model()->rowCount() > 0) {
                    m_ledgerWidget->crTable()->setFocus(Qt::OtherFocusReason);
                }
            } else if (m_ledgerWidget->searchBox()) {
                m_ledgerWidget->searchBox()->setFocus(Qt::OtherFocusReason);
            }
        }
    } else if (viewIndex == 29) {
        // Show C++ Balance Sheet Widget
        FiscalYearInfo activeFy = FiscalYearHelper::getActiveFiscalYear();
        m_stackedWidget->setCurrentWidget(m_balanceSheetWidget);
        m_balanceSheetWidget->refreshData(activeFy.endDate);
        if (m_balanceSheetWidget) {
            m_balanceSheetWidget->setFocus();
            if (m_balanceSheetWidget->liabilitiesTree()) {
                m_balanceSheetWidget->liabilitiesTree()->setFocus(Qt::OtherFocusReason);
                if (m_balanceSheetWidget->liabilitiesTree()->topLevelItemCount() > 0 && !m_balanceSheetWidget->liabilitiesTree()->currentItem()) {
                    m_balanceSheetWidget->liabilitiesTree()->setCurrentItem(m_balanceSheetWidget->liabilitiesTree()->topLevelItem(0));
                }
            }
        }
    } else if (viewIndex == 30) {
        // Show C++ Profit & Loss Widget
        FiscalYearInfo activeFy = FiscalYearHelper::getActiveFiscalYear();
        m_stackedWidget->setCurrentWidget(m_profitLossWidget);
        m_profitLossWidget->refreshData(activeFy.startDate, activeFy.endDate);
        if (m_profitLossWidget) {
            m_profitLossWidget->setFocus();
            if (m_profitLossWidget->expensesTree()) {
                m_profitLossWidget->expensesTree()->setFocus(Qt::OtherFocusReason);
                if (m_profitLossWidget->expensesTree()->topLevelItemCount() > 0 && !m_profitLossWidget->expensesTree()->currentItem()) {
                    m_profitLossWidget->expensesTree()->setCurrentItem(m_profitLossWidget->expensesTree()->topLevelItem(0));
                }
            }
        }
    } else if (viewIndex == 14) {
        // Show C++ Sales Voucher Widget
        if (m_salesVoucherWidget) {
            m_stackedWidget->setCurrentWidget(m_salesVoucherWidget);
            if (!pendingInv.isEmpty() || !pendingVNo.isEmpty() || pendingId > 0) {
                QVariant target = !pendingInv.isEmpty() ? QVariant(pendingInv) : (!pendingVNo.isEmpty() ? QVariant(pendingVNo) : QVariant(pendingId));
                bool ok = m_salesVoucherWidget->loadInvoiceForEditing(target, pendingDate);
                if (!ok && !pendingVNo.isEmpty() && pendingVNo != pendingInv) {
                    ok = m_salesVoucherWidget->loadInvoiceForEditing(pendingVNo, pendingDate);
                }
                if (!ok && pendingId > 0) {
                    m_salesVoucherWidget->loadInvoiceForEditing(pendingId, pendingDate);
                }
            } else {
                m_salesVoucherWidget->resetForm();
            }
            m_salesVoucherWidget->setFocus();
        }
    } else if (viewIndex == 15) {
        // Show C++ Purchase Voucher Widget
        if (m_purchaseVoucherWidget) {
            m_stackedWidget->setCurrentWidget(m_purchaseVoucherWidget);
            if (!pendingInv.isEmpty() || !pendingVNo.isEmpty() || pendingId > 0) {
                QVariant target = !pendingInv.isEmpty() ? QVariant(pendingInv) : (!pendingVNo.isEmpty() ? QVariant(pendingVNo) : QVariant(pendingId));
                bool ok = m_purchaseVoucherWidget->loadInvoiceForEditing(target, pendingDate);
                if (!ok && !pendingVNo.isEmpty() && pendingVNo != pendingInv) {
                    ok = m_purchaseVoucherWidget->loadInvoiceForEditing(pendingVNo, pendingDate);
                }
                if (!ok && pendingId > 0) {
                    m_purchaseVoucherWidget->loadInvoiceForEditing(pendingId, pendingDate);
                }
            } else {
                m_purchaseVoucherWidget->resetForm();
            }
            m_purchaseVoucherWidget->setFocus();
        }
    } else if (viewIndex == 16) {
        // Show C++ Cheque Voucher Widget (Payment & Receipt)
        if (m_chequeVoucherWidget) {
            m_stackedWidget->setCurrentWidget(m_chequeVoucherWidget);
            QString mode = root ? root->property("targetChequeMode").toString() : "";
            if (root) {
                root->setProperty("targetChequeMode", "");
            }
            if (mode == "RECEIPT") {
                m_chequeVoucherWidget->setVoucherType("Receipt");
            } else if (mode == "PAYMENT") {
                m_chequeVoucherWidget->setVoucherType("Payment");
            }

            if (!pendingInv.isEmpty() || !pendingVNo.isEmpty() || pendingId > 0) {
                QVariant target = !pendingInv.isEmpty() ? QVariant(pendingInv) : (!pendingVNo.isEmpty() ? QVariant(pendingVNo) : QVariant(pendingId));
                bool ok = m_chequeVoucherWidget->loadVoucherForEditing(target, pendingDate);
                if (!ok && !pendingVNo.isEmpty() && pendingVNo != pendingInv) {
                    ok = m_chequeVoucherWidget->loadVoucherForEditing(pendingVNo, pendingDate);
                }
                if (!ok && pendingId > 0) {
                    m_chequeVoucherWidget->loadVoucherForEditing(pendingId, pendingDate);
                }
            } else {
                m_chequeVoucherWidget->resetForm();
            }
            m_chequeVoucherWidget->setFocus();
        }
    } else if (viewIndex == 17) {
        // Show C++ Journal Voucher Widget
        if (m_journalVoucherWidget) {
            m_stackedWidget->setCurrentWidget(m_journalVoucherWidget);
            if (!pendingInv.isEmpty() || !pendingVNo.isEmpty() || pendingId > 0) {
                QVariant target = !pendingInv.isEmpty() ? QVariant(pendingInv) : (!pendingVNo.isEmpty() ? QVariant(pendingVNo) : QVariant(pendingId));
                bool ok = m_journalVoucherWidget->loadVoucherForEditing(target, pendingDate);
                if (!ok && !pendingVNo.isEmpty() && pendingVNo != pendingInv) {
                    ok = m_journalVoucherWidget->loadVoucherForEditing(pendingVNo, pendingDate);
                }
                if (!ok && pendingId > 0) {
                    m_journalVoucherWidget->loadVoucherForEditing(pendingId, pendingDate);
                }
            } else {
                m_journalVoucherWidget->resetForm();
            }
            m_journalVoucherWidget->setFocus();
        }
    } else if (viewIndex == 22) {
        // Show C++ Firm Selector Widget
        if (m_firmSelectorWidget) {
            m_stackedWidget->setCurrentWidget(m_firmSelectorWidget);
            m_firmSelectorWidget->refreshFirms();
            m_firmSelectorWidget->setFocus();
            m_firmSelectorWidget->focusTable();
        }
    } else if (viewIndex == 6) {
        // Show C++ New Ledger Widget
        if (m_newLedgerWidget) {
            m_stackedWidget->setCurrentWidget(m_newLedgerWidget);
            m_newLedgerWidget->resetForm();
            m_newLedgerWidget->setFocus();
            m_newLedgerWidget->focusFirstField();
        }
    } else if (viewIndex == 7) {
        // Show C++ Modify Ledger Widget
        if (m_modifyLedgerWidget) {
            m_stackedWidget->setCurrentWidget(m_modifyLedgerWidget);
            m_modifyLedgerWidget->resetForm();
            m_modifyLedgerWidget->setFocus();
            m_modifyLedgerWidget->focusSearch();
        }
    } else {
        // Show QML view stack
        if (m_qmlContainer) {
            m_stackedWidget->setCurrentWidget(m_qmlContainer);
            m_qmlContainer->setFocus(Qt::OtherFocusReason);
        }
        if (m_qmlWindow) {
            m_qmlWindow->requestActivate();
            if (m_qmlWindow->contentItem()) {
                m_qmlWindow->contentItem()->forceActiveFocus(Qt::OtherFocusReason);
            }
            QMetaObject::invokeMethod(m_qmlWindow, "navigateToView", Q_ARG(QVariant, viewIndex));
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
    if (m_ledgerWidget) {
        m_ledgerWidget->onTotalsChanged();
    }
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
    if (m_ledgerWidget) {
        m_ledgerWidget->onTotalsChanged();
    }
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
    if (m_ledgerWidget) {
        m_ledgerWidget->onTotalsChanged();
    }
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
    if (m_ledgerWidget) {
        m_ledgerWidget->onTotalsChanged();
    }
}

void MainWindow::onNewLedgerBackRequested() {
    navigateToView(0);
}

void MainWindow::onNewLedgerSaved() {
    if (m_ledgerWidget) {
        m_ledgerWidget->onTotalsChanged();
    }
    if (m_dashboardWidget) {
        m_dashboardWidget->refreshStats();
    }
}

void MainWindow::onModifyLedgerBackRequested() {
    navigateToView(0);
}

void MainWindow::onModifyLedgerSaved() {
    if (m_ledgerWidget) {
        m_ledgerWidget->onTotalsChanged();
    }
    if (m_dashboardWidget) {
        m_dashboardWidget->refreshStats();
    }
}

void MainWindow::onBalanceSheetBackRequested() {
    navigateToView(0);
}

void MainWindow::onBalanceSheetPartyStatementRequested(const QString& partyName, const QString& fromDate, const QString& toDate) {
    m_previousViewIndex = 29;
    if (m_qmlWindow) {
        m_qmlWindow->setProperty("currentViewIndex", 8);
    }
    m_stackedWidget->setCurrentWidget(m_ledgerWidget);
    if (m_ledgerWidget) {
        m_ledgerWidget->loadParty(partyName, fromDate, toDate);
        m_ledgerWidget->setFocus();
    }
}

void MainWindow::onProfitLossBackRequested() {
    navigateToView(0);
}

void MainWindow::onProfitLossPartyStatementRequested(const QString& partyName, const QString& fromDate, const QString& toDate) {
    m_previousViewIndex = 30;
    if (m_qmlWindow) {
        m_qmlWindow->setProperty("currentViewIndex", 8);
    }
    m_stackedWidget->setCurrentWidget(m_ledgerWidget);
    if (m_ledgerWidget) {
        m_ledgerWidget->loadParty(partyName, fromDate, toDate);
        m_ledgerWidget->setFocus();
    }
}

void MainWindow::onLedgerAlterVoucherRequested(int targetViewIndex, const QVariantMap& entry) {
    if (m_qmlWindow && m_ledgerWidget) {
        QString pName = m_ledgerWidget->currentParty();
        m_qmlWindow->setProperty("lastViewedStatementParty", pName);
        m_qmlWindow->setProperty("targetStatementParty", pName);
        m_qmlWindow->setProperty("lastViewedStatementFromDate", m_ledgerWidget->fromDate());
        m_qmlWindow->setProperty("lastViewedStatementToDate", m_ledgerWidget->toDate());

        bool isCr = (m_ledgerWidget->lastSide() == "Cr");
        m_qmlWindow->setProperty("lastViewedStatementSide", isCr ? "Cr" : "Dr");
        m_qmlWindow->setProperty("lastViewedStatementIndex", m_ledgerWidget->lastIndex());

        QString vNoStr = entry.value("voucherNo").toString();
        if (vNoStr.isEmpty()) vNoStr = entry.value("voucher_no").toString();
        if (vNoStr.isEmpty()) vNoStr = entry.value("refNo").toString();

        int itemId = entry.value("id").toInt();
        QString vDate = entry.value("vIso").toString();
        if (vDate.isEmpty()) vDate = entry.value("voucher_date").toString();
        if (vDate.isEmpty()) vDate = entry.value("date").toString();
        QString invNo = entry.value("invoiceNo").toString();
        if (invNo.isEmpty()) invNo = entry.value("invoice_no").toString();
        if (invNo.isEmpty()) invNo = vNoStr;

        m_qmlWindow->setProperty("pendingEditInvoiceNo", invNo);
        m_qmlWindow->setProperty("pendingEditVoucherNo", vNoStr);
        m_qmlWindow->setProperty("pendingEditVoucherId", itemId);
        m_qmlWindow->setProperty("pendingEditVoucherDate", vDate);

        if (targetViewIndex == 14) {
            m_previousViewIndex = 8;
            navigateToView(14);
            return;
        } else if (targetViewIndex == 15) {
            m_previousViewIndex = 8;
            navigateToView(15);
            return;
        } else if (targetViewIndex == 16) {
            m_previousViewIndex = 8;
            QString vType = entry.value("voucherType").toString();
            if (vType.isEmpty()) vType = entry.value("voucher_type").toString();
            QString rawType = entry.value("legacyType").toString();
            if (rawType.isEmpty()) rawType = entry.value("legacy_type").toString();
            bool isReceipt = (vType == "Receipt" || rawType == "ChRt" || rawType == "Rcpt" || entry.value("side").toString() == "Cr");
            m_qmlWindow->setProperty("targetChequeMode", isReceipt ? "RECEIPT" : "PAYMENT");
            navigateToView(16);
            return;
        } else if (targetViewIndex == 17) {
            m_previousViewIndex = 8;
            navigateToView(17);
            return;
        } else if (targetViewIndex == 24) {
            m_qmlWindow->setProperty("targetTdsVoucherId", itemId);
        }

        if (m_qmlContainer) {
            m_stackedWidget->setCurrentWidget(m_qmlContainer);
        }
        QMetaObject::invokeMethod(m_qmlWindow, "navigateToView", Q_ARG(QVariant, targetViewIndex));
    }
}

void MainWindow::closeEvent(QCloseEvent* event) {
    QMainWindow::closeEvent(event);
}

