#include "ledger_statement_widget.h"
#include "engine/accounting_engine.h"
#include "engine/fiscal_year_helper.h"
#include "database_manager.h"
#include <QPainter>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFrame>
#include <QShortcut>
#include <QKeyEvent>
#include <QDate>
#include <QTimer>
#include <QHeaderView>
#include <QMessageBox>
#include <cmath>

LedgerStatementWidget::LedgerStatementWidget(LedgerStatementController* controller,
                                             PrintExportController* printExportCtrl,
                                             QWidget* parent)
    : QWidget(parent)
    , m_controller(controller)
    , m_printExportCtrl(printExportCtrl)
{
    setAttribute(Qt::WA_StyledBackground, true);
    setAutoFillBackground(true);
    setupUi();

    if (m_controller) {
        connect(m_controller, &LedgerStatementController::statementTotalsChanged, this, &LedgerStatementWidget::onTotalsChanged);
        connect(m_controller, &LedgerStatementController::statementLoaded, this, &LedgerStatementWidget::onTotalsChanged);
        connect(m_controller, &LedgerStatementController::statementLoaded, this, &LedgerStatementWidget::recalculateAankStatement);
    }
}

void LedgerStatementWidget::setupUi() {
    setStyleSheet("background-color: #F8FAFC;");

    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(12, 10, 12, 10);
    mainLayout->setSpacing(8);

    // ================= 1. TOP HEADER BAR CARD =================
    QFrame* headerCard = new QFrame(this);
    headerCard->setFixedHeight(54);
    headerCard->setStyleSheet("background-color: #FFFFFF; border: 1px solid #E2E8F0; border-radius: 8px;");
    QHBoxLayout* headerLayout = new QHBoxLayout(headerCard);
    headerLayout->setContentsMargins(14, 6, 14, 6);
    headerLayout->setSpacing(10);

    QVBoxLayout* titleLayout = new QVBoxLayout();
    titleLayout->setSpacing(1);
    QLabel* titleLabel = new QLabel("Account Ledger Statement & Mandi Aank Rokka", headerCard);
    titleLabel->setStyleSheet("font-size: 16px; font-weight: bold; color: #0F172A; font-family: 'Segoe UI', -apple-system, BlinkMacSystemFont, Roboto, sans-serif; border: none; background: transparent;");
    QLabel* subtitleLabel = new QLabel("Side-by-side Credit (Cr) & Debit (Dr) double-entry ledger with Aank daily product interest calculation.", headerCard);
    subtitleLabel->setStyleSheet("font-size: 11px; color: #64748B; font-family: 'Segoe UI', -apple-system, BlinkMacSystemFont, Roboto, sans-serif; border: none; background: transparent;");
    titleLayout->addWidget(titleLabel);
    titleLayout->addWidget(subtitleLabel);
    headerLayout->addLayout(titleLayout);

    headerLayout->addStretch(1);

    m_toggleAankBtn = new KbdBadgeButton("Aank Interest (Rokka)", "Alt+A", QColor("#7C3AED"), QColor("#6D28D9"), QColor("#FFFFFF"), QColor("#7C3AED"), headerCard);
    connect(m_toggleAankBtn, &QPushButton::clicked, this, &LedgerStatementWidget::toggleAankMode);
    headerLayout->addWidget(m_toggleAankBtn);

    m_printBtn = new KbdBadgeButton("Print Statement", "Ctrl+P", QColor("#2563EB"), QColor("#1D4ED8"), QColor("#FFFFFF"), QColor("#2563EB"), headerCard);
    connect(m_printBtn, &QPushButton::clicked, this, &LedgerStatementWidget::printStatement);
    headerLayout->addWidget(m_printBtn);

    m_pdfBtn = new KbdBadgeButton("Export PDF", "Alt+P", QColor("#059669"), QColor("#047857"), QColor("#FFFFFF"), QColor("#059669"), headerCard);
    connect(m_pdfBtn, &QPushButton::clicked, this, &LedgerStatementWidget::exportPdf);
    headerLayout->addWidget(m_pdfBtn);

    m_csvBtn = new KbdBadgeButton("Export CSV", "", QColor("#FFFFFF"), QColor("#F1F5F9"), QColor("#334155"), QColor("#CBD5E1"), headerCard);
    connect(m_csvBtn, &QPushButton::clicked, this, &LedgerStatementWidget::exportCsv);
    headerLayout->addWidget(m_csvBtn);

    m_backBtn = new KbdBadgeButton("Back to Dashboard", "Esc", QColor("#EF4444"), QColor("#DC2626"), QColor("#FFFFFF"), QColor("#EF4444"), headerCard);
    connect(m_backBtn, &QPushButton::clicked, this, &LedgerStatementWidget::backRequested);
    headerLayout->addWidget(m_backBtn);

    mainLayout->addWidget(headerCard);

    // ================= 2. SEARCH & FILTER BAR CARD =================
    QFrame* filterCard = new QFrame(this);
    filterCard->setFixedHeight(50);
    filterCard->setStyleSheet("background-color: #FFFFFF; border: 1px solid #E2E8F0; border-radius: 8px;");
    QHBoxLayout* filterLayout = new QHBoxLayout(filterCard);
    filterLayout->setContentsMargins(12, 6, 12, 6);
    filterLayout->setSpacing(10);

    m_searchBox = new AccountSearchBox(filterCard);
    m_searchBox->setMinimumWidth(380);
    if (m_controller) {
        m_searchBox->setSearchFunction([this](const QString& q) {
            return m_controller->searchParties(q);
        });
    }
    connect(m_searchBox, &AccountSearchBox::partySelected, this, &LedgerStatementWidget::onPartySelected);
    filterLayout->addWidget(m_searchBox);

    FiscalYearInfo activeFy = FiscalYearHelper::getActiveFiscalYear();

    m_fyBadge = new QLabel(activeFy.name, filterCard);
    m_fyBadge->setAlignment(Qt::AlignCenter);
    m_fyBadge->setFixedHeight(34);
    m_fyBadge->setStyleSheet("background-color: #F8FAFC; color: #334155; border: 1px solid #CBD5E1; border-radius: 6px; padding: 4px 12px; font-weight: bold; font-size: 11px;");
    filterLayout->addWidget(m_fyBadge);

    QLabel* fromLabel = new QLabel("From:", filterCard);
    fromLabel->setStyleSheet("color: #475569; font-weight: bold; font-size: 11px; border: none; background: transparent;");
    filterLayout->addWidget(fromLabel);

    m_fromDateEdit = new AccountingDateEdit(filterCard);
    m_fromDateEdit->setIsoDate(activeFy.startDate);
    m_fromDateEdit->setFixedWidth(110);
    filterLayout->addWidget(m_fromDateEdit);

    QLabel* toLabel = new QLabel("To:", filterCard);
    toLabel->setStyleSheet("color: #475569; font-weight: bold; font-size: 11px; border: none; background: transparent;");
    filterLayout->addWidget(toLabel);

    m_toDateEdit = new AccountingDateEdit(filterCard);
    m_toDateEdit->setIsoDate(activeFy.endDate);
    m_toDateEdit->setFixedWidth(110);
    filterLayout->addWidget(m_toDateEdit);

    m_applyFilterBtn = new QPushButton("Filter Dates", filterCard);
    m_applyFilterBtn->setFixedHeight(34);
    m_applyFilterBtn->setCursor(Qt::PointingHandCursor);
    m_applyFilterBtn->setStyleSheet("background-color: #2563EB; color: #FFFFFF; border-radius: 6px; padding: 4px 16px; font-weight: bold; font-size: 11px; border: none;");
    connect(m_applyFilterBtn, &QPushButton::clicked, this, &LedgerStatementWidget::onDateFilterApplied);
    filterLayout->addWidget(m_applyFilterBtn);

    filterLayout->addStretch(1);
    mainLayout->addWidget(filterCard);

    // ================= 3. TABS CONTAINER =================
    m_viewTabs = new QTabWidget(this);
    m_viewTabs->setStyleSheet(
        "QTabWidget::pane { border: 1px solid #CBD5E1; background: #FFFFFF; border-radius: 8px; top: -1px; }"
        "QTabBar::tab { background: #E2E8F0; color: #1E293B; font-size: 12px; font-weight: 700; padding: 7px 18px; margin-right: 4px; border-top-left-radius: 6px; border-top-right-radius: 6px; border: 1px solid #CBD5E1; border-bottom: none; }"
        "QTabBar::tab:selected { background: #2563EB; color: #FFFFFF; }"
    );

    // ---------- TAB 1: 2-COLUMN LEDGER VIEW ----------
    m_twoColumnWidget = new QWidget(m_viewTabs);
    auto* twoColLayout = new QVBoxLayout(m_twoColumnWidget);
    twoColLayout->setContentsMargins(8, 8, 8, 8);
    twoColLayout->setSpacing(8);

    QHBoxLayout* tablesLayout = new QHBoxLayout();
    tablesLayout->setSpacing(10);

    // --- Left: Credit Side (Cr) ---
    QVBoxLayout* crSideLayout = new QVBoxLayout();
    crSideLayout->setSpacing(4);

    QFrame* crBanner = new QFrame(m_twoColumnWidget);
    crBanner->setFixedHeight(32);
    crBanner->setStyleSheet("background-color: #DCFCE7; border: 1px solid #86EFAC; border-radius: 6px;");
    QHBoxLayout* crBannerLayout = new QHBoxLayout(crBanner);
    crBannerLayout->setContentsMargins(10, 0, 10, 0);
    m_crHeaderLabel = new QLabel("CREDIT SIDE (JAMA / Cr)", crBanner);
    m_crHeaderLabel->setStyleSheet("color: #15803D; font-weight: bold; font-size: 11px; border: none; background: transparent;");
    QLabel* crBadge = new QLabel("Takes / Payables", crBanner);
    crBadge->setStyleSheet("color: #166534; background-color: #BBF7D0; border-radius: 4px; padding: 2px 6px; font-size: 10px; font-weight: bold; border: none;");
    crBannerLayout->addWidget(m_crHeaderLabel);
    crBannerLayout->addStretch(1);
    crBannerLayout->addWidget(crBadge);
    crSideLayout->addWidget(crBanner);

    m_crTable = new LedgerTableView("Cr", m_controller ? m_controller->crModel() : nullptr, m_twoColumnWidget);
    connect(m_crTable, &LedgerTableView::voucherActivated, this, [this](const QVariantMap& entry) {
        m_lastSide = "Cr";
        int row = m_crTable->selectedRowIndex();
        m_lastIndex = (row >= 0) ? row : 0;
        openVoucherForEntry(entry);
    });
    connect(m_crTable, &LedgerTableView::switchSideRequested, this, &LedgerStatementWidget::onSwitchSideRequested);
    connect(m_crTable, &LedgerTableView::focusSearchRequested, this, &LedgerStatementWidget::focusSearch);
    crSideLayout->addWidget(m_crTable, 1);

    // Cr Footer
    QFrame* crFooter = new QFrame(m_twoColumnWidget);
    crFooter->setFixedHeight(32);
    crFooter->setStyleSheet("background-color: #F8FAFC; border: 1px solid #CBD5E1; border-radius: 6px;");
    QHBoxLayout* crFooterLayout = new QHBoxLayout(crFooter);
    crFooterLayout->setContentsMargins(10, 0, 10, 0);
    m_crTotalLabel = new QLabel("Total Cr: ₹0.00 (0 entries)", crFooter);
    m_crTotalLabel->setStyleSheet("color: #15803D; font-weight: bold; font-size: 11px; border: none; background: transparent;");
    m_crCheckedLabel = new QLabel("Checked: ₹0.00", crFooter);
    m_crCheckedLabel->setStyleSheet("color: #475569; font-weight: bold; font-size: 11px; border: none; background: transparent;");
    crFooterLayout->addWidget(m_crTotalLabel);
    crFooterLayout->addStretch(1);
    crFooterLayout->addWidget(m_crCheckedLabel);
    crSideLayout->addWidget(crFooter);

    tablesLayout->addLayout(crSideLayout, 1);

    // --- Right: Debit Side (Dr) ---
    QVBoxLayout* drSideLayout = new QVBoxLayout();
    drSideLayout->setSpacing(4);

    QFrame* drBanner = new QFrame(m_twoColumnWidget);
    drBanner->setFixedHeight(32);
    drBanner->setStyleSheet("background-color: #DBEAFE; border: 1px solid #93C5FD; border-radius: 6px;");
    QHBoxLayout* drBannerLayout = new QHBoxLayout(drBanner);
    drBannerLayout->setContentsMargins(10, 0, 10, 0);
    m_drHeaderLabel = new QLabel("DEBIT SIDE (NAAME / Dr)", drBanner);
    m_drHeaderLabel->setStyleSheet("color: #1D4ED8; font-weight: bold; font-size: 11px; border: none; background: transparent;");
    QLabel* drBadge = new QLabel("Gives / Receivables", drBanner);
    drBadge->setStyleSheet("color: #1E40AF; background-color: #BFDBFE; border-radius: 4px; padding: 2px 6px; font-size: 10px; font-weight: bold; border: none;");
    drBannerLayout->addWidget(m_drHeaderLabel);
    drBannerLayout->addStretch(1);
    drBannerLayout->addWidget(drBadge);
    drSideLayout->addWidget(drBanner);

    m_drTable = new LedgerTableView("Dr", m_controller ? m_controller->drModel() : nullptr, m_twoColumnWidget);
    connect(m_drTable, &LedgerTableView::voucherActivated, this, [this](const QVariantMap& entry) {
        m_lastSide = "Dr";
        int row = m_drTable->selectedRowIndex();
        m_lastIndex = (row >= 0) ? row : 0;
        openVoucherForEntry(entry);
    });
    connect(m_drTable, &LedgerTableView::switchSideRequested, this, &LedgerStatementWidget::onSwitchSideRequested);
    connect(m_drTable, &LedgerTableView::focusSearchRequested, this, &LedgerStatementWidget::focusSearch);
    drSideLayout->addWidget(m_drTable, 1);

    // Dr Footer
    QFrame* drFooter = new QFrame(m_twoColumnWidget);
    drFooter->setFixedHeight(32);
    drFooter->setStyleSheet("background-color: #F8FAFC; border: 1px solid #CBD5E1; border-radius: 6px;");
    QHBoxLayout* drFooterLayout = new QHBoxLayout(drFooter);
    drFooterLayout->setContentsMargins(10, 0, 10, 0);
    m_drTotalLabel = new QLabel("Total Dr: ₹0.00 (0 entries)", drFooter);
    m_drTotalLabel->setStyleSheet("color: #1D4ED8; font-weight: bold; font-size: 11px; border: none; background: transparent;");
    m_drCheckedLabel = new QLabel("Checked: ₹0.00", drFooter);
    m_drCheckedLabel->setStyleSheet("color: #475569; font-weight: bold; font-size: 11px; border: none; background: transparent;");
    drFooterLayout->addWidget(m_drTotalLabel);
    drFooterLayout->addStretch(1);
    drFooterLayout->addWidget(m_drCheckedLabel);
    drSideLayout->addWidget(drFooter);

    tablesLayout->addLayout(drSideLayout, 1);
    twoColLayout->addLayout(tablesLayout, 1);

    // Bottom Net Reconciliation Summary
    QFrame* summaryBar = new QFrame(m_twoColumnWidget);
    summaryBar->setFixedHeight(44);
    summaryBar->setStyleSheet("background-color: #F8FAFC; border: 1px solid #CBD5E1; border-radius: 6px;");
    QHBoxLayout* summaryLayout = new QHBoxLayout(summaryBar);
    summaryLayout->setContentsMargins(16, 0, 16, 0);

    m_netDiffLabel = new QLabel("Difference: ₹0.00", summaryBar);
    m_netDiffLabel->setStyleSheet("color: #334155; font-weight: bold; font-size: 12px; border: none; background: transparent;");

    m_netBalanceLabel = new QLabel("Closing Balance: ₹0.00", summaryBar);
    m_netBalanceLabel->setStyleSheet("color: #0F172A; font-weight: bold; font-size: 13px; border: none; background: transparent;");

    m_checkedDiffLabel = new QLabel("Checked Difference: ₹0.00", summaryBar);
    m_checkedDiffLabel->setStyleSheet("color: #64748B; font-weight: bold; font-size: 11px; border: none; background: transparent;");

    summaryLayout->addWidget(m_netDiffLabel);
    summaryLayout->addStretch(1);
    summaryLayout->addWidget(m_netBalanceLabel);
    summaryLayout->addStretch(1);
    summaryLayout->addWidget(m_checkedDiffLabel);

    twoColLayout->addWidget(summaryBar);
    m_viewTabs->addTab(m_twoColumnWidget, "  2-Column Ledger (Dr / Cr)  ");

    // ---------- TAB 2: AANK DAILY PRODUCT & INTEREST STATEMENT ----------
    m_aankWidget = new QWidget(m_viewTabs);
    auto* aankLayout = new QVBoxLayout(m_aankWidget);
    aankLayout->setContentsMargins(8, 8, 8, 8);
    aankLayout->setSpacing(8);

    // Aank Control Card
    QFrame* aankControlCard = new QFrame(m_aankWidget);
    aankControlCard->setFixedHeight(46);
    aankControlCard->setStyleSheet("background-color: #F8FAFC; border: 1px solid #E2E8F0; border-radius: 6px;");
    QHBoxLayout* aankControlLayout = new QHBoxLayout(aankControlCard);
    aankControlLayout->setContentsMargins(12, 4, 12, 4);
    aankControlLayout->setSpacing(12);

    QLabel* drRateLbl = new QLabel("Dr Interest Rate (%):", aankControlCard);
    drRateLbl->setStyleSheet("font-size: 11px; font-weight: 700; color: #1D4ED8;");
    aankControlLayout->addWidget(drRateLbl);

    m_drInterestRateSpin = new QDoubleSpinBox(aankControlCard);
    m_drInterestRateSpin->setRange(0.0, 100.0);
    m_drInterestRateSpin->setValue(12.0);
    m_drInterestRateSpin->setSingleStep(0.25);
    m_drInterestRateSpin->setFixedHeight(28);
    m_drInterestRateSpin->setMinimumWidth(75);
    connect(m_drInterestRateSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &LedgerStatementWidget::recalculateAankStatement);
    aankControlLayout->addWidget(m_drInterestRateSpin);

    QLabel* crRateLbl = new QLabel("Cr Interest Rate (%):", aankControlCard);
    crRateLbl->setStyleSheet("font-size: 11px; font-weight: 700; color: #15803D;");
    aankControlLayout->addWidget(crRateLbl);

    m_crInterestRateSpin = new QDoubleSpinBox(aankControlCard);
    m_crInterestRateSpin->setRange(0.0, 100.0);
    m_crInterestRateSpin->setValue(12.0);
    m_crInterestRateSpin->setSingleStep(0.25);
    m_crInterestRateSpin->setFixedHeight(28);
    m_crInterestRateSpin->setMinimumWidth(75);
    connect(m_crInterestRateSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &LedgerStatementWidget::recalculateAankStatement);
    aankControlLayout->addWidget(m_crInterestRateSpin);

    m_leapYearDivisorCheck = new QCheckBox("366-Day Leap Year Basis", aankControlCard);
    m_leapYearDivisorCheck->setStyleSheet("font-size: 11px; font-weight: 600; color: #475569;");
    connect(m_leapYearDivisorCheck, &QCheckBox::toggled, this, &LedgerStatementWidget::recalculateAankStatement);
    aankControlLayout->addWidget(m_leapYearDivisorCheck);

    aankControlLayout->addStretch(1);

    m_recomputeAankBtn = new KbdBadgeButton("Recalculate", "F5", QColor("#2563EB"), QColor("#1D4ED8"), QColor("#FFFFFF"), QColor("#2563EB"), aankControlCard);
    connect(m_recomputeAankBtn, &QPushButton::clicked, this, &LedgerStatementWidget::recalculateAankStatement);
    aankControlLayout->addWidget(m_recomputeAankBtn);

    m_postInterestVoucherBtn = new KbdBadgeButton("Post Interest Voucher", "Alt+I", QColor("#059669"), QColor("#047857"), QColor("#FFFFFF"), QColor("#059669"), aankControlCard);
    connect(m_postInterestVoucherBtn, &QPushButton::clicked, this, &LedgerStatementWidget::onPostInterestVoucherClicked);
    aankControlLayout->addWidget(m_postInterestVoucherBtn);

    aankLayout->addWidget(aankControlCard);

    // Aank Table
    m_aankTable = new QTableWidget(m_aankWidget);
    m_aankTable->setColumnCount(12);
    m_aankTable->setHorizontalHeaderLabels({
        "Date", "Vch No", "Type", "Particulars / Narration", "Debit (₹)", "Credit (₹)",
        "Running Bal (₹)", "Side", "Days", "Dr Aank (Product)", "Cr Aank (Product)", "Interest (₹)"
    });
    m_aankTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_aankTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_aankTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    m_aankTable->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Stretch);
    m_aankTable->horizontalHeader()->setSectionResizeMode(4, QHeaderView::ResizeToContents);
    m_aankTable->horizontalHeader()->setSectionResizeMode(5, QHeaderView::ResizeToContents);
    m_aankTable->horizontalHeader()->setSectionResizeMode(6, QHeaderView::ResizeToContents);
    m_aankTable->horizontalHeader()->setSectionResizeMode(7, QHeaderView::ResizeToContents);
    m_aankTable->horizontalHeader()->setSectionResizeMode(8, QHeaderView::ResizeToContents);
    m_aankTable->horizontalHeader()->setSectionResizeMode(9, QHeaderView::ResizeToContents);
    m_aankTable->horizontalHeader()->setSectionResizeMode(10, QHeaderView::ResizeToContents);
    m_aankTable->horizontalHeader()->setSectionResizeMode(11, QHeaderView::ResizeToContents);
    m_aankTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_aankTable->setSelectionMode(QAbstractItemView::SingleSelection);
    m_aankTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_aankTable->setAlternatingRowColors(true);
    m_aankTable->verticalHeader()->setVisible(false);
    m_aankTable->verticalHeader()->setDefaultSectionSize(26);
    m_aankTable->setStyleSheet(
        "QTableWidget { background-color: #FFFFFF; alternate-background-color: #F8FAFC; border: 1px solid #CBD5E1; border-radius: 6px; font-size: 11px; }"
        "QHeaderView::section { background-color: #0F172A; color: #FFFFFF; font-weight: 700; font-size: 11px; padding: 5px 6px; border: none; }"
    );
    aankLayout->addWidget(m_aankTable, 1);

    // Aank Summary Bar Card
    QFrame* aankSummaryBar = new QFrame(m_aankWidget);
    aankSummaryBar->setFixedHeight(46);
    aankSummaryBar->setStyleSheet("background-color: #F8FAFC; border: 1px solid #CBD5E1; border-radius: 6px;");
    QHBoxLayout* aankSummaryLayout = new QHBoxLayout(aankSummaryBar);
    aankSummaryLayout->setContentsMargins(14, 4, 14, 4);
    aankSummaryLayout->setSpacing(14);

    m_aankTotalDrLabel = new QLabel("Dr Aank: 0.00", aankSummaryBar);
    m_aankTotalDrLabel->setStyleSheet("color: #1D4ED8; font-weight: 700; font-size: 11px;");
    aankSummaryLayout->addWidget(m_aankTotalDrLabel);

    m_aankDrInterestLabel = new QLabel("Dr Int: ₹0.00", aankSummaryBar);
    m_aankDrInterestLabel->setStyleSheet("color: #1D4ED8; font-weight: 800; font-size: 12px;");
    aankSummaryLayout->addWidget(m_aankDrInterestLabel);

    aankSummaryLayout->addSpacing(15);

    m_aankTotalCrLabel = new QLabel("Cr Aank: 0.00", aankSummaryBar);
    m_aankTotalCrLabel->setStyleSheet("color: #15803D; font-weight: 700; font-size: 11px;");
    aankSummaryLayout->addWidget(m_aankTotalCrLabel);

    m_aankCrInterestLabel = new QLabel("Cr Int: ₹0.00", aankSummaryBar);
    m_aankCrInterestLabel->setStyleSheet("color: #15803D; font-weight: 800; font-size: 12px;");
    aankSummaryLayout->addWidget(m_aankCrInterestLabel);

    aankSummaryLayout->addStretch(1);

    m_aankNetInterestLabel = new QLabel("Net Accrued Interest: ₹0.00", aankSummaryBar);
    m_aankNetInterestLabel->setStyleSheet("color: #0F172A; font-weight: 800; font-size: 13px;");
    aankSummaryLayout->addWidget(m_aankNetInterestLabel);

    aankLayout->addWidget(aankSummaryBar);
    m_viewTabs->addTab(m_aankWidget, "  Aank Daily Product & Interest Statement (Rokka)  ");

    mainLayout->addWidget(m_viewTabs, 1);

    // Tab Order Chain
    setTabOrder(m_searchBox, m_fromDateEdit);
    setTabOrder(m_fromDateEdit, m_toDateEdit);
    setTabOrder(m_toDateEdit, m_applyFilterBtn);
    setTabOrder(m_applyFilterBtn, m_crTable);
    setTabOrder(m_crTable, m_drTable);

    // Global Shortcuts
    new QShortcut(QKeySequence("Alt+S"), this, SLOT(focusSearch()));
    new QShortcut(QKeySequence("Alt+L"), this, SLOT(focusSearch()));
    new QShortcut(QKeySequence("Ctrl+F"), this, SLOT(focusSearch()));
    new QShortcut(QKeySequence("F3"), this, SLOT(focusSearch()));
    new QShortcut(QKeySequence("Ctrl+E"), this, SLOT(openSelectedVoucher()));
    new QShortcut(QKeySequence("Ctrl+P"), this, SLOT(printStatement()));
    new QShortcut(QKeySequence("Alt+P"), this, SLOT(exportPdf()));
    new QShortcut(QKeySequence("Alt+E"), this, SLOT(exportCsv()));
    new QShortcut(QKeySequence("Alt+F"), this, SLOT(onDateFilterApplied()));
    new QShortcut(QKeySequence("Alt+A"), this, SLOT(toggleAankMode()));
    new QShortcut(QKeySequence("Alt+I"), this, SLOT(onPostInterestVoucherClicked()));
}

void LedgerStatementWidget::toggleAankMode() {
    if (m_viewTabs->currentIndex() == 0) {
        m_viewTabs->setCurrentIndex(1);
        recalculateAankStatement();
    } else {
        m_viewTabs->setCurrentIndex(0);
    }
}

void LedgerStatementWidget::recalculateAankStatement() {
    QString partyName = m_searchBox->currentPartyName().trimmed();
    if (partyName.isEmpty()) {
        m_aankTable->setRowCount(0);
        m_aankTotalDrLabel->setText("Dr Aank: 0.00");
        m_aankTotalCrLabel->setText("Cr Aank: 0.00");
        m_aankDrInterestLabel->setText("Dr Int: ₹0.00");
        m_aankCrInterestLabel->setText("Cr Int: ₹0.00");
        m_aankNetInterestLabel->setText("Net Accrued Interest: ₹0.00");
        return;
    }

    QDate fromDt = QDate::fromString(m_fromDateEdit->isoDate(), "yyyy-MM-dd");
    QDate toDt = QDate::fromString(m_toDateEdit->isoDate(), "yyyy-MM-dd");
    if (!fromDt.isValid()) {
        FiscalYearInfo activeFy = FiscalYearHelper::getActiveFiscalYear();
        fromDt = QDate::fromString(activeFy.startDate, "yyyy-MM-dd");
        if (!fromDt.isValid()) fromDt = QDate(QDate::currentDate().month() < 4 ? QDate::currentDate().year() - 1 : QDate::currentDate().year(), 4, 1);
    }
    if (!toDt.isValid()) toDt = QDate::currentDate();

    // Query party opening balance and interest rate
    QVariantList pRows = DatabaseManager::instance().executeQuery(
        "SELECT id, opening_balance, balance_type, interest_rate FROM parties WHERE name = ? LIMIT 1;",
        {partyName}
    );

    double opBal = 0.0;
    QString opDrCr = "Cr";
    int partyId = 0;
    if (!pRows.isEmpty()) {
        QVariantMap pr = pRows.first().toMap();
        partyId = pr.value("id").toInt();
        opBal = pr.value("opening_balance").toDouble();
        opDrCr = pr.value("balance_type").toString();
        double savedRate = pr.value("interest_rate").toDouble();
        if (savedRate > 0.0 && m_drInterestRateSpin->value() == 12.0) {
            m_drInterestRateSpin->setValue(savedRate);
            m_crInterestRateSpin->setValue(savedRate);
        }
    }

    // Partition prior transactions before fromDt to determine opening balance on fromDt
    PartitionedLedgerData partData = FiscalYearHelper::partitionPartyTransactions(partyName, m_fromDateEdit->isoDate(), m_toDateEdit->isoDate());
    if (std::abs(partData.netOpeningBalance) > 0.001) {
        opBal = std::abs(partData.netOpeningBalance);
        opDrCr = partData.openingBalanceType;
    }

    // Query all transaction vouchers for this party in date range
    QString sql = QString(
        "SELECT voucher_date AS date, voucher_no, voucher_type, narration, "
        "CASE WHEN dr_cr = 'Dr' THEN amount ELSE 0.0 END AS debit, "
        "CASE WHEN dr_cr = 'Cr' THEN amount ELSE 0.0 END AS credit "
        "FROM transactions WHERE party_name = ? AND voucher_date >= ? AND voucher_date <= ? "
        "ORDER BY voucher_date ASC, id ASC;"
    );
    QVariantList rawRows = DatabaseManager::instance().executeQuery(sql, {partyName, m_fromDateEdit->isoDate(), m_toDateEdit->isoDate()});

    QList<QVariantMap> rawVouchers;
    for (const auto& var : rawRows) rawVouchers.append(var.toMap());

    m_currentAankStatement = MahadevERP::AankInterestEngine::calculateStatement(
        partyId, partyName, opBal, opDrCr, fromDt, toDt,
        m_drInterestRateSpin->value(), m_crInterestRateSpin->value(),
        rawVouchers, m_leapYearDivisorCheck->isChecked()
    );

    // Populate Aank Table
    m_aankTable->setRowCount(0);
    int row = 0;
    for (const auto& e : m_currentAankStatement.entries) {
        m_aankTable->insertRow(row);
        m_aankTable->setItem(row, 0, new QTableWidgetItem(e.date.toString("dd-MM-yyyy")));
        m_aankTable->setItem(row, 1, new QTableWidgetItem(e.voucherNo));
        m_aankTable->setItem(row, 2, new QTableWidgetItem(e.voucherType));
        m_aankTable->setItem(row, 3, new QTableWidgetItem(e.narration));

        auto* drItem = new QTableWidgetItem(e.debitAmount > 0.0 ? AccountingEngine::formatCurrency(e.debitAmount) : "");
        drItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        if (e.debitAmount > 0.0) drItem->setForeground(QColor("#1D4ED8"));
        m_aankTable->setItem(row, 4, drItem);

        auto* crItem = new QTableWidgetItem(e.creditAmount > 0.0 ? AccountingEngine::formatCurrency(e.creditAmount) : "");
        crItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        if (e.creditAmount > 0.0) crItem->setForeground(QColor("#15803D"));
        m_aankTable->setItem(row, 5, crItem);

        double absBal = std::abs(e.runningBalance);
        auto* balItem = new QTableWidgetItem(AccountingEngine::formatCurrency(absBal));
        balItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        m_aankTable->setItem(row, 6, balItem);

        QString side = e.runningBalance >= 0 ? "Dr" : "Cr";
        auto* sideItem = new QTableWidgetItem(side);
        sideItem->setTextAlignment(Qt::AlignCenter);
        sideItem->setForeground(e.runningBalance >= 0 ? QColor("#1D4ED8") : QColor("#15803D"));
        m_aankTable->setItem(row, 7, sideItem);

        auto* daysItem = new QTableWidgetItem(QString::number(e.days));
        daysItem->setTextAlignment(Qt::AlignCenter);
        m_aankTable->setItem(row, 8, daysItem);

        double drAank = (e.runningBalance >= 0) ? e.productAank : 0.0;
        auto* drAankItem = new QTableWidgetItem(drAank > 0.0 ? AccountingEngine::formatCurrency(drAank) : "");
        drAankItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        drAankItem->setForeground(QColor("#1D4ED8"));
        m_aankTable->setItem(row, 9, drAankItem);

        double crAank = (e.runningBalance < 0) ? std::abs(e.productAank) : 0.0;
        auto* crAankItem = new QTableWidgetItem(crAank > 0.0 ? AccountingEngine::formatCurrency(crAank) : "");
        crAankItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        crAankItem->setForeground(QColor("#15803D"));
        m_aankTable->setItem(row, 10, crAankItem);

        auto* intItem = new QTableWidgetItem(AccountingEngine::formatCurrency(std::abs(e.interestAmount)));
        intItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        if (e.interestAmount > 0.0) intItem->setForeground(QColor("#1D4ED8"));
        else if (e.interestAmount < 0.0) intItem->setForeground(QColor("#15803D"));
        m_aankTable->setItem(row, 11, intItem);

        row++;
    }

    // Update Summary Labels
    m_aankTotalDrLabel->setText(QString("Dr Aank: %1").arg(AccountingEngine::formatIndianCurrency(m_currentAankStatement.totalDrAank)));
    m_aankTotalCrLabel->setText(QString("Cr Aank: %1").arg(AccountingEngine::formatIndianCurrency(m_currentAankStatement.totalCrAank)));
    m_aankDrInterestLabel->setText(QString("Dr Int (@%1%): %2").arg(m_drInterestRateSpin->value()).arg(AccountingEngine::formatIndianCurrency(m_currentAankStatement.totalDrInterest)));
    m_aankCrInterestLabel->setText(QString("Cr Int (@%1%): %2").arg(m_crInterestRateSpin->value()).arg(AccountingEngine::formatIndianCurrency(m_currentAankStatement.totalCrInterest)));

    QString netType = m_currentAankStatement.netInterest >= 0 ? "Dr (Receivable)" : "Cr (Payable)";
    QString netColor = m_currentAankStatement.netInterest >= 0 ? "#1D4ED8" : "#15803D";
    m_aankNetInterestLabel->setText(QString("Net Accrued Interest: %1 %2").arg(AccountingEngine::formatIndianCurrency(std::abs(m_currentAankStatement.netInterest))).arg(netType));
    m_aankNetInterestLabel->setStyleSheet(QString("color: %1; font-weight: 800; font-size: 13px;").arg(netColor));
}

void LedgerStatementWidget::onPostInterestVoucherClicked() {
    QString partyName = m_searchBox->currentPartyName().trimmed();
    if (partyName.isEmpty() || std::abs(m_currentAankStatement.netInterest) < 0.01) {
        QMessageBox::warning(this, "Interest Posting", "No interest amount available to post.");
        return;
    }

    double netInt = m_currentAankStatement.netInterest;
    QString isDr = (netInt >= 0) ? "Dr (Receivable from Party)" : "Cr (Payable to Party)";
    QString msg = QString("Do you want to post an Interest Voucher for %1?\n\nParty: %2\nPeriod: %3 to %4\nNet Interest: %5 %6")
        .arg(partyName)
        .arg(partyName)
        .arg(m_fromDateEdit->isoDate())
        .arg(m_toDateEdit->isoDate())
        .arg(AccountingEngine::formatIndianCurrency(std::abs(netInt)))
        .arg(isDr);

    if (QMessageBox::question(this, "Confirm Interest Posting", msg, QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes) {
        return;
    }

    // Post Interest Journal Voucher in SQLite
    FiscalYearInfo fy = FiscalYearHelper::getActiveFiscalYear();
    QString vDate = m_toDateEdit->isoDate();
    QString vNo = QString("INT-%1").arg(QDate::fromString(vDate, "yyyy-MM-dd").toString("yyyyMMdd"));
    QString narr = QString("Interest accrued from %1 to %2 @ Dr %3% / Cr %4%")
        .arg(m_fromDateEdit->isoDate())
        .arg(m_toDateEdit->isoDate())
        .arg(m_drInterestRateSpin->value())
        .arg(m_crInterestRateSpin->value());

    DatabaseManager::instance().beginTransaction();

    // 1. Header Voucher
    DatabaseManager::instance().executeNonQuery(
        "INSERT INTO vouchers (fy_id, financial_year, voucher_no, voucher_date, voucher_type, legacy_type, party_name, amount, narration) "
        "VALUES (?, ?, ?, ?, 'Journal', 'Jrnl', ?, ?, ?);",
        {1, fy.name, vNo, vDate, partyName, std::abs(netInt), narr}
    );

    // 2. Double-Entry Posting
    if (netInt >= 0) {
        // Debit Party, Credit Interest Received
        DatabaseManager::instance().executeNonQuery(
            "INSERT INTO transactions (financial_year, voucher_no, voucher_date, voucher_type, trans_type, party_name, opposing_account, dr_cr, amount, narration) "
            "VALUES (?, ?, ?, 'Journal', 'Jrnl', ?, 'Interest Received A/c', 'Dr', ?, ?);",
            {fy.name, vNo, vDate, partyName, std::abs(netInt), narr}
        );
        DatabaseManager::instance().executeNonQuery(
            "INSERT INTO transactions (financial_year, voucher_no, voucher_date, voucher_type, trans_type, party_name, opposing_account, dr_cr, amount, narration) "
            "VALUES (?, ?, ?, 'Journal', 'Jrnl', 'Interest Received A/c', ?, 'Cr', ?, ?);",
            {fy.name, vNo, vDate, partyName, std::abs(netInt), narr}
        );
    } else {
        // Debit Interest Paid, Credit Party
        DatabaseManager::instance().executeNonQuery(
            "INSERT INTO transactions (financial_year, voucher_no, voucher_date, voucher_type, trans_type, party_name, opposing_account, dr_cr, amount, narration) "
            "VALUES (?, ?, ?, 'Journal', 'Jrnl', 'Interest Paid A/c', ?, 'Dr', ?, ?);",
            {fy.name, vNo, vDate, partyName, std::abs(netInt), narr}
        );
        DatabaseManager::instance().executeNonQuery(
            "INSERT INTO transactions (financial_year, voucher_no, voucher_date, voucher_type, trans_type, party_name, opposing_account, dr_cr, amount, narration) "
            "VALUES (?, ?, ?, 'Journal', 'Jrnl', ?, 'Interest Paid A/c', 'Cr', ?, ?);",
            {fy.name, vNo, vDate, partyName, std::abs(netInt), narr}
        );
    }

    DatabaseManager::instance().commit();

    QMessageBox::information(this, "Success", "Interest Voucher posted successfully!\nVoucher No: " + vNo);
    if (m_controller) m_controller->refresh();
    recalculateAankStatement();
}

void LedgerStatementWidget::loadParty(const QString& partyName, const QString& fromDate, const QString& toDate) {
    if (partyName.trimmed().isEmpty()) {
        resetSearch();
        return;
    }

    FiscalYearInfo activeFy = FiscalYearHelper::getActiveFiscalYear();
    m_fyBadge->setText(activeFy.name);

    QString fIso = FiscalYearHelper::normalizeToIso(fromDate);
    QString tIso = FiscalYearHelper::normalizeToIso(toDate);
    FiscalYearHelper::clampDateRangeToFiscalYear(fIso, tIso, activeFy);

    m_searchBox->setPartyName(partyName.trimmed());
    m_fromDateEdit->setIsoDate(fIso);
    m_toDateEdit->setIsoDate(tIso);

    if (m_controller) {
        m_controller->loadPartyStatement(partyName.trimmed(), fIso, tIso);
    }

    updateHeadersAndTotals();
    recalculateAankStatement();

    // Default focus to Dr table if entries exist, else Cr
    if (m_controller && m_controller->drModel()->rowCount() > 0) {
        m_drTable->setFocus();
        m_drTable->selectRowIndex(0);
    } else if (m_controller && m_controller->crModel()->rowCount() > 0) {
        m_crTable->setFocus();
        m_crTable->selectRowIndex(0);
    }
}

void LedgerStatementWidget::restoreState(const QString& partyName, const QString& fromDate, const QString& toDate,
                                         const QString& side, int rowIndex) {
    if (partyName.trimmed().isEmpty()) {
        resetSearch();
        return;
    }

    FiscalYearInfo activeFy = FiscalYearHelper::getActiveFiscalYear();
    m_fyBadge->setText(activeFy.name);

    QString fIso = FiscalYearHelper::normalizeToIso(fromDate);
    QString tIso = FiscalYearHelper::normalizeToIso(toDate);
    FiscalYearHelper::clampDateRangeToFiscalYear(fIso, tIso, activeFy);

    m_searchBox->setPartyName(partyName.trimmed());
    m_fromDateEdit->setIsoDate(fIso);
    m_toDateEdit->setIsoDate(tIso);

    if (m_controller) {
        m_controller->loadPartyStatement(partyName.trimmed(), fIso, tIso);
    }

    updateHeadersAndTotals();
    recalculateAankStatement();

    m_lastSide = side;
    m_lastIndex = rowIndex;

    QTimer::singleShot(20, this, [this, side, rowIndex]() {
        if (side == "Cr" && m_controller && m_controller->crModel()->rowCount() > 0) {
            int target = qBound(0, rowIndex, m_controller->crModel()->rowCount() - 1);
            m_crTable->setFocus();
            m_crTable->selectRowIndex(target);
        } else if (m_controller && m_controller->drModel()->rowCount() > 0) {
            int target = qBound(0, rowIndex, m_controller->drModel()->rowCount() - 1);
            m_drTable->setFocus();
            m_drTable->selectRowIndex(target);
        } else if (m_controller && m_controller->crModel()->rowCount() > 0) {
            m_crTable->setFocus();
            m_crTable->selectRowIndex(0);
        }
    });
}

void LedgerStatementWidget::resetSearch() {
    FiscalYearInfo activeFy = FiscalYearHelper::getActiveFiscalYear();
    m_fyBadge->setText(activeFy.name);
    m_fromDateEdit->setIsoDate(activeFy.startDate);
    m_toDateEdit->setIsoDate(activeFy.endDate);

    m_searchBox->setPartyName("");
    if (m_controller) {
        m_controller->loadPartyStatement("", activeFy.startDate, activeFy.endDate);
    }
    updateHeadersAndTotals();
    recalculateAankStatement();
    m_searchBox->setFocus();
    m_searchBox->selectAll();
}

QString LedgerStatementWidget::currentParty() const {
    return m_searchBox->currentPartyName();
}

QString LedgerStatementWidget::fromDate() const {
    return m_fromDateEdit->isoDate();
}

QString LedgerStatementWidget::toDate() const {
    return m_toDateEdit->isoDate();
}

void LedgerStatementWidget::focusSearch() {
    m_searchBox->setFocus();
    m_searchBox->selectAll();
    m_searchBox->openSearchPopup();
}

void LedgerStatementWidget::onPartySelected(const QString& partyName) {
    loadParty(partyName, m_fromDateEdit->isoDate(), m_toDateEdit->isoDate());
}

void LedgerStatementWidget::onDateFilterApplied() {
    if (m_controller && !m_searchBox->currentPartyName().isEmpty()) {
        FiscalYearInfo activeFy = FiscalYearHelper::getActiveFiscalYear();
        m_fyBadge->setText(activeFy.name);

        QString fIso = m_fromDateEdit->isoDate();
        QString tIso = m_toDateEdit->isoDate();
        FiscalYearHelper::clampDateRangeToFiscalYear(fIso, tIso, activeFy);

        // Update UI inputs with clamped valid dates
        m_fromDateEdit->setIsoDate(fIso);
        m_toDateEdit->setIsoDate(tIso);

        m_controller->applyDateFilter(fIso, tIso);
        updateHeadersAndTotals();
        recalculateAankStatement();
    }
}

void LedgerStatementWidget::onTotalsChanged() {
    updateHeadersAndTotals();
}

void LedgerStatementWidget::updateHeadersAndTotals() {
    QString pName = m_searchBox->currentPartyName();
    m_crHeaderLabel->setText(pName.isEmpty() ? "CREDIT SIDE (JAMA / Cr)" : QString("CREDIT SIDE (JAMA / Cr) - %1").arg(pName));
    m_drHeaderLabel->setText(pName.isEmpty() ? "DEBIT SIDE (NAAME / Dr)" : QString("DEBIT SIDE (NAAME / Dr) - %1").arg(pName));

    if (!m_controller) return;

    int crCount = m_controller->crModel()->rowCount();
    int drCount = m_controller->drModel()->rowCount();

    m_crTotalLabel->setText(QString("Total Cr: %1 (%2 entries)").arg(m_controller->crTotalFmt()).arg(crCount));
    m_crCheckedLabel->setText(QString("Checked: %1").arg(m_controller->crSelectedTotalFmt()));

    m_drTotalLabel->setText(QString("Total Dr: %1 (%2 entries)").arg(m_controller->drTotalFmt()).arg(drCount));
    m_drCheckedLabel->setText(QString("Checked: %1").arg(m_controller->drSelectedTotalFmt()));

    double drTot = m_controller->drTotal();
    double crTot = m_controller->crTotal();
    double diff = std::abs(drTot - crTot);
    m_netDiffLabel->setText(QString("Difference: %1").arg(AccountingEngine::formatIndianCurrency(diff, true)));

    QString balType = "Nil";
    QString balColor = "#475569";
    if (crTot > drTot + 0.001) {
        balType = "Cr";
        balColor = "#047857"; // Emerald Green for Cr closing balance
    } else if (drTot > crTot + 0.001) {
        balType = "Dr";
        balColor = "#1D4ED8"; // Royal Blue for Dr closing balance
    }

    m_netBalanceLabel->setText(QString("Closing Balance: %1 %2").arg(AccountingEngine::formatIndianCurrency(diff, true), balType));
    m_netBalanceLabel->setStyleSheet(QString("color: %1; font-weight: bold; font-size: 14px; border: none; background: transparent; font-family: 'Segoe UI', -apple-system, BlinkMacSystemFont, Roboto, sans-serif;").arg(balColor));

    double checkedDiff = std::abs(m_controller->drSelectedTotal() - m_controller->crSelectedTotal());
    m_checkedDiffLabel->setText(QString("Checked Difference: %1").arg(AccountingEngine::formatIndianCurrency(checkedDiff, true)));
}

void LedgerStatementWidget::openSelectedVoucher() {
    if (m_crTable->hasFocus() || (m_crTable->selectedRowIndex() >= 0 && !m_drTable->hasFocus())) {
        int row = m_crTable->selectedRowIndex();
        if (row >= 0 && m_controller) {
            m_lastSide = "Cr";
            m_lastIndex = row;
            onVoucherActivated(m_controller->crModel()->get(row));
            return;
        }
    }
    if (m_drTable->hasFocus() || m_drTable->selectedRowIndex() >= 0) {
        int row = m_drTable->selectedRowIndex();
        if (row >= 0 && m_controller) {
            m_lastSide = "Dr";
            m_lastIndex = row;
            onVoucherActivated(m_controller->drModel()->get(row));
            return;
        }
    }
    if (m_crTable->selectedRowIndex() >= 0 && m_controller) {
        m_lastSide = "Cr";
        m_lastIndex = m_crTable->selectedRowIndex();
        onVoucherActivated(m_controller->crModel()->get(m_crTable->selectedRowIndex()));
    }
}

void LedgerStatementWidget::onVoucherActivated(const QVariantMap& entry) {
    openVoucherForEntry(entry);
}

void LedgerStatementWidget::openVoucherForEntry(const QVariantMap& entry) {
    if (entry.isEmpty()) return;

    QString part = entry.value("particulars").toString().toLower();
    if (part.contains("opening balance") || part.contains("closing balance") || part.contains("b/f")) {
        return;
    }

    QString vType = entry.value("voucherType").toString();
    if (vType.isEmpty()) vType = entry.value("voucher_type").toString();
    QString rawType = entry.value("legacyType").toString();
    if (rawType.isEmpty()) rawType = entry.value("legacy_type").toString();
    if (vType.isEmpty()) vType = entry.value("transType").toString();
    if (vType.isEmpty()) vType = entry.value("trans_type").toString();
    if (rawType.isEmpty()) rawType = entry.value("trans_type").toString();

    QString refNo = entry.value("refNo").toString().trimmed();
    if (vType.isEmpty() && rawType.isEmpty() && !refNo.isEmpty()) {
        QString prefix = refNo.split(' ').first();
        rawType = prefix;
        vType = prefix;
    }

    int targetViewIndex = -1;

    // 1. TDS Voucher (View 24)
    if (vType == "TDS" || rawType == "TDS" || refNo.startsWith("TDS") || part.contains("t.d.s.")) {
        targetViewIndex = 24;
    }
    // 2. Sales Voucher (View 14)
    else if (vType == "Sales" || rawType == "Sale" || rawType == "Sales" || vType == "Sale" || refNo.startsWith("Sale")) {
        targetViewIndex = 14;
    }
    // 3. Purchase Voucher (View 15)
    else if (vType == "Purchase" || rawType == "Purc" || rawType == "Purchase" || vType == "Purc" || refNo.startsWith("Purc")) {
        targetViewIndex = 15;
    }
    // 4. Cash Payment / Receipt (View 60)
    else if (rawType == "Pymt" || rawType == "Rcpt" || refNo.startsWith("Pymt") || refNo.startsWith("Rcpt") || vType == "Cash Payment" || vType == "Cash Receipt") {
        targetViewIndex = 60;
    }
    // 4b. Cheque / Bank Payment / Receipt (View 16)
    else if (vType == "Payment" || vType == "Receipt" || rawType == "ChPt" || rawType == "ChRt" || rawType == "Bank" ||
             refNo.startsWith("ChPt") || refNo.startsWith("ChRt")) {
        targetViewIndex = 16;
    }
    // 5. Journal Voucher (View 17)
    else if (vType == "Journal" || rawType == "Jrnl" || rawType == "Journal" || vType == "Jrnl" || refNo.startsWith("Jrnl")) {
        targetViewIndex = 17;
    }
    // 6. Milling Voucher (View 18)
    else if (vType == "Milling" || rawType == "Mill" || rawType == "Prod" || rawType == "ML" || refNo.startsWith("Mill") || refNo.startsWith("ML")) {
        targetViewIndex = 18;
    }
    // 7. J-Form Voucher (View 23)
    else if (vType == "J-Form" || rawType == "JFrm" || rawType == "J-Form" || vType == "JFrm" || refNo.startsWith("JFrm") || refNo.startsWith("J-Form")) {
        targetViewIndex = 23;
    }
    // 8. Debit / Credit Note (View 28)
    else if (vType == "Debit Note" || vType == "Credit Note" || rawType == "DbNt" || rawType == "CrNt" || rawType == "DN" || rawType == "CN" || refNo.startsWith("DbNt") || refNo.startsWith("CrNt")) {
        targetViewIndex = 28;
    }

    if (targetViewIndex != -1) {
        emit alterVoucherRequested(targetViewIndex, entry);
    }
}

void LedgerStatementWidget::onSwitchSideRequested(const QString& targetSide) {
    if (targetSide == "Cr" && m_controller && m_controller->crModel()->rowCount() > 0) {
        int existingCrRow = m_crTable->selectedRowIndex();
        int targetRow = (existingCrRow >= 0) ? qBound(0, existingCrRow, m_controller->crModel()->rowCount() - 1) : 0;
        m_crTable->setFocus();
        m_crTable->selectRowIndex(targetRow);
        m_lastSide = "Cr";
        m_lastIndex = targetRow;
    } else if (targetSide == "Dr" && m_controller && m_controller->drModel()->rowCount() > 0) {
        int existingDrRow = m_drTable->selectedRowIndex();
        int targetRow = (existingDrRow >= 0) ? qBound(0, existingDrRow, m_controller->drModel()->rowCount() - 1) : 0;
        m_drTable->setFocus();
        m_drTable->selectRowIndex(targetRow);
        m_lastSide = "Dr";
        m_lastIndex = targetRow;
    }
}

void LedgerStatementWidget::printStatement() {
    if (m_printExportCtrl && !m_searchBox->currentPartyName().isEmpty()) {
        m_printExportCtrl->print_ledger_statement(m_searchBox->currentPartyName(), m_fromDateEdit->isoDate(), m_toDateEdit->isoDate());
    }
}

void LedgerStatementWidget::exportPdf() {
    if (m_printExportCtrl && !m_searchBox->currentPartyName().isEmpty()) {
        m_printExportCtrl->export_ledger_statement_pdf(m_searchBox->currentPartyName(), m_fromDateEdit->isoDate(), m_toDateEdit->isoDate());
    }
}

void LedgerStatementWidget::exportCsv() {
    if (m_printExportCtrl && !m_searchBox->currentPartyName().isEmpty()) {
        m_printExportCtrl->export_ledger_csv(m_searchBox->currentPartyName(), m_fromDateEdit->isoDate(), m_toDateEdit->isoDate());
    }
}

void LedgerStatementWidget::keyPressEvent(QKeyEvent* event) {
    if (event->key() == Qt::Key_Escape) {
        if (m_searchBox && m_searchBox->isPopupVisible()) {
            event->ignore();
            return;
        }
        emit backRequested();
        event->accept();
        return;
    }
    if (event->key() == Qt::Key_F2) {
        m_fromDateEdit->setFocus();
        m_fromDateEdit->selectAll();
        event->accept();
        return;
    }
    if (event->key() == Qt::Key_F3 || (event->key() == Qt::Key_F && (event->modifiers() & Qt::ControlModifier)) || (event->key() == Qt::Key_S && (event->modifiers() & Qt::AltModifier))) {
        focusSearch();
        event->accept();
        return;
    }
    if (event->key() == Qt::Key_Slash && !(event->modifiers() & (Qt::ControlModifier | Qt::AltModifier))) {
        focusSearch();
        event->accept();
        return;
    }
    QWidget::keyPressEvent(event);
}

void LedgerStatementWidget::showEvent(QShowEvent* event) {
    QWidget::showEvent(event);
    FiscalYearInfo activeFy = FiscalYearHelper::getActiveFiscalYear();
    m_fyBadge->setText(activeFy.name);
    
    QString curFrom = m_fromDateEdit->isoDate();
    QString curTo = m_toDateEdit->isoDate();
    if (m_searchBox->currentPartyName().isEmpty() || curFrom < activeFy.startDate || curTo > activeFy.endDate) {
        m_fromDateEdit->setIsoDate(activeFy.startDate);
        m_toDateEdit->setIsoDate(activeFy.endDate);
    }
    
    // Automatically focus the search box on open with text selected and popup ready
    QTimer::singleShot(0, this, [this]() {
        focusSearch();
    });
}

void LedgerStatementWidget::paintEvent(QPaintEvent* event) {
    QPainter painter(this);
    painter.fillRect(rect(), QColor("#F8FAFC"));
    QWidget::paintEvent(event);
}
