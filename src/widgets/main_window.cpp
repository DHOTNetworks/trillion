#include "main_window.h"
#include <QQuickItem>
#include <QTimer>
#include <QDebug>

MainWindow::MainWindow(QQuickWindow* qmlWindow,
                       LedgerStatementController* ledgerCtrl,
                       PrintExportController* printExportCtrl,
                       QWidget* parent)
    : QMainWindow(parent)
    , m_qmlWindow(qmlWindow)
    , m_ledgerCtrl(ledgerCtrl)
    , m_printExportCtrl(printExportCtrl)
{
    setWindowTitle("Mahadev Rice Mill ERP & Accounting");
    resize(1280, 800);
    setMinimumSize(1024, 680);

    m_stackedWidget = new QStackedWidget(this);
    setCentralWidget(m_stackedWidget);

    // Index 0: Embedded QML Application Window
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

    // Monitor QML View Index changes via a polling timer / property check
    QTimer* viewMonitor = new QTimer(this);
    connect(viewMonitor, &QTimer::timeout, this, &MainWindow::checkQmlView);
    viewMonitor->start(50); // Fast 50ms responsive check
}

void MainWindow::checkQmlView() {
    if (!m_qmlWindow) return;
    QObject* root = m_qmlWindow;
    int vIdx = root->property("currentViewIndex").toInt();

    if (vIdx == 8 && m_stackedWidget->currentWidget() != m_ledgerWidget) {
        navigateToView(8);
    } else if (vIdx != 8 && m_stackedWidget->currentWidget() == m_ledgerWidget) {
        m_stackedWidget->setCurrentWidget(m_qmlContainer);
    }
}

void MainWindow::navigateToView(int viewIndex) {
    if (viewIndex == 8) {
        // Show C++ Ledger Statement Widget
        QObject* root = m_qmlWindow;
        QString party = root ? root->property("targetStatementParty").toString() : "";
        if (party.isEmpty() && root) {
            party = root->property("lastViewedStatementParty").toString();
        }
        if (party.isEmpty() && root) {
            party = root->property("lastViewedPartyName").toString();
        }

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
        }
    } else {
        // Show QML view stack
        if (m_qmlContainer) {
            m_stackedWidget->setCurrentWidget(m_qmlContainer);
        }
        if (m_qmlWindow) {
            QMetaObject::invokeMethod(m_qmlWindow, "navigateToView", Q_ARG(QVariant, viewIndex));
        }
    }
}

void MainWindow::onLedgerBackRequested() {
    if (m_qmlContainer) {
        m_stackedWidget->setCurrentWidget(m_qmlContainer);
    }
    if (m_qmlWindow) {
        QMetaObject::invokeMethod(m_qmlWindow, "navigateBack");
    }
}

void MainWindow::onLedgerAlterVoucherRequested(int targetViewIndex, const QVariantMap& entry) {
    if (m_qmlWindow && m_ledgerWidget) {
        QString pName = m_ledgerWidget->currentParty();
        m_qmlWindow->setProperty("lastViewedStatementParty", pName);
        m_qmlWindow->setProperty("targetStatementParty", pName);
        m_qmlWindow->setProperty("lastViewedStatementFromDate", m_ledgerWidget->fromDate());
        m_qmlWindow->setProperty("lastViewedStatementToDate", m_ledgerWidget->toDate());

        QString vNoStr = entry.value("voucherNo").toString();
        if (vNoStr.isEmpty()) vNoStr = entry.value("refNo").toString();
        int itemId = entry.value("id").toInt();
        QString vDate = entry.value("vIso").toString();
        QString invNo = entry.value("invoiceNo").toString();
        if (invNo.isEmpty()) invNo = vNoStr;

        m_qmlWindow->setProperty("pendingEditInvoiceNo", invNo);
        m_qmlWindow->setProperty("pendingEditVoucherNo", vNoStr);
        m_qmlWindow->setProperty("pendingEditVoucherId", itemId);
        m_qmlWindow->setProperty("pendingEditVoucherDate", vDate);

        if (targetViewIndex == 24) {
            m_qmlWindow->setProperty("targetTdsVoucherId", itemId);
        } else if (targetViewIndex == 16) {
            QString vType = entry.value("voucherType").toString();
            QString rawType = entry.value("legacyType").toString();
            bool isReceipt = (vType == "Receipt" || rawType == "ChRt" || rawType == "Rcpt" || entry.value("side").toString() == "Cr");
            m_qmlWindow->setProperty("targetChequeMode", isReceipt ? "Receipt" : "Payment");
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
