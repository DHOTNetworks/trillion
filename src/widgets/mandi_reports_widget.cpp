#include "mandi_reports_widget.h"
#include "engine/accounting_engine.h"
#include "engine/fiscal_year_helper.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QHeaderView>
#include <QKeyEvent>
#include <QSplitter>
#include <QGroupBox>
#include <QDate>
#include <QLocale>

MandiReportsWidget::MandiReportsWidget(PrintExportController* printExportCtrl, QWidget* parent)
    : QWidget(parent)
    , m_printExportCtrl(printExportCtrl)
{
    setAttribute(Qt::WA_StyledBackground, true);
    setAutoFillBackground(true);
    setupUi();
    applyCustomStyles();
    refreshAllTabs();
}

MandiReportsWidget::~MandiReportsWidget() = default;

void MandiReportsWidget::setupUi() {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(12, 10, 12, 10);
    mainLayout->setSpacing(8);

    // Top Header Card
    auto* headerCard = new QFrame(this);
    headerCard->setObjectName("topHeaderCard");
    auto* topHeader = new QHBoxLayout(headerCard);
    topHeader->setContentsMargins(14, 8, 14, 8);

    auto* titleCol = new QVBoxLayout();
    titleCol->setSpacing(2);
    auto* titleLabel = new QLabel("MANDI OPERATIONS & STATUTORY REPORTING (HSAMB)", headerCard);
    titleLabel->setObjectName("reportTitle");
    auto* subTitleLabel = new QLabel("Statutory Form M Return, J-Form (Farmer Khata/Dheri), I-Form (Buyer Register), and Dami Book", headerCard);
    subTitleLabel->setObjectName("reportSubtitle");
    titleCol->addWidget(titleLabel);
    titleCol->addWidget(subTitleLabel);

    auto* backBtn = new QPushButton("Esc - Back to Dashboard", headerCard);
    backBtn->setObjectName("backButton");
    connect(backBtn, &QPushButton::clicked, this, [this]() { emit backRequested(); });

    topHeader->addLayout(titleCol);
    topHeader->addStretch();
    topHeader->addWidget(backBtn);
    mainLayout->addWidget(headerCard);

    // Tab Widget
    m_tabWidget = new QTabWidget(this);
    m_tabWidget->setObjectName("mandiTabWidget");
    setupFormMTab();
    setupJFormRegisterTab();
    setupIFormRegisterTab();
    setupFarmerStatementTab();
    setupDamiRegisterTab();

    mainLayout->addWidget(m_tabWidget, 1);

    const QList<QWidget*> childs = {
        backBtn, m_tabWidget, m_formMFromDate, m_formMToDate, m_formMTable,
        m_jfFromDate, m_jfToDate, m_jfTable, m_ifFromDate, m_ifToDate, m_ifTable,
        m_stmtFarmerCombo, m_stmtDheriesTable, m_stmtPaymentsTable,
        m_damiFromDate, m_damiToDate, m_damiTable
    };
    for (QWidget* w : childs) {
        if (w) w->installEventFilter(this);
    }
}

void MandiReportsWidget::setupFormMTab() {
    auto* tab = new QWidget();
    auto* layout = new QVBoxLayout(tab);
    layout->setContentsMargins(8, 8, 8, 8);
    layout->setSpacing(8);

    // Filter Bar
    FiscalYearInfo fy = FiscalYearHelper::getActiveFiscalYear();
    QDate defStart = QDate::fromString(fy.startDate, "yyyy-MM-dd");
    if (!defStart.isValid()) defStart = QDate(QDate::currentDate().year(), 4, 1);
    QDate defEnd = QDate::fromString(fy.endDate, "yyyy-MM-dd");
    if (!defEnd.isValid()) defEnd = QDate::currentDate();

    auto* filterLayout = new QHBoxLayout();
    filterLayout->addWidget(new QLabel("From Date:", this));
    m_formMFromDate = new QDateEdit(defStart, this);
    m_formMFromDate->setCalendarPopup(true);
    m_formMFromDate->setDisplayFormat("dd-MM-yyyy");
    filterLayout->addWidget(m_formMFromDate);

    filterLayout->addWidget(new QLabel("To Date:", this));
    m_formMToDate = new QDateEdit(defEnd, this);
    m_formMToDate->setCalendarPopup(true);
    m_formMToDate->setDisplayFormat("dd-MM-yyyy");
    filterLayout->addWidget(m_formMToDate);

    auto* refreshBtn = new QPushButton("Generate Form M Return", this);
    refreshBtn->setObjectName("primaryButton");
    connect(refreshBtn, &QPushButton::clicked, this, &MandiReportsWidget::generateFormM);
    filterLayout->addWidget(refreshBtn);
    filterLayout->addStretch();
    layout->addLayout(filterLayout);

    // Form M Table
    m_formMTable = new QTableWidget(0, 8, this);
    m_formMTable->setHorizontalHeaderLabels({
        "Commodity / Item", "Total Bags", "Weight (Qtl)", "Gross Value (₹)",
        "M.Fee Rate (%)", "Market Fee (₹)", "HRDF Rate (%)", "HRDF Amount (₹)"
    });
    m_formMTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    for (int c = 1; c < 8; ++c) {
        m_formMTable->horizontalHeader()->setSectionResizeMode(c, QHeaderView::ResizeToContents);
    }
    layout->addWidget(m_formMTable, 1);

    // Summary Box
    auto* sumBox = new QGroupBox("Market Committee Statutory Levies Summary", this);
    auto* sumLayout = new QGridLayout(sumBox);
    sumLayout->setContentsMargins(8, 6, 8, 6);

    sumLayout->addWidget(new QLabel("Total Bags:", this), 0, 0);
    m_formMBagsSummary = new QLabel("0", this);
    m_formMBagsSummary->setObjectName("boldValue");
    sumLayout->addWidget(m_formMBagsSummary, 0, 1);

    sumLayout->addWidget(new QLabel("Total Weight:", this), 0, 2);
    m_formMWeightSummary = new QLabel("0.000 Qtl", this);
    m_formMWeightSummary->setObjectName("boldValue");
    sumLayout->addWidget(m_formMWeightSummary, 0, 3);

    sumLayout->addWidget(new QLabel("Gross Value:", this), 0, 4);
    m_formMGoodsSummary = new QLabel("₹ 0.00", this);
    m_formMGoodsSummary->setObjectName("boldValue");
    sumLayout->addWidget(m_formMGoodsSummary, 0, 5);

    sumLayout->addWidget(new QLabel("Total Market Fee (2%):", this), 1, 0);
    m_formMMFeeSummary = new QLabel("₹ 0.00", this);
    m_formMMFeeSummary->setObjectName("boldValue");
    sumLayout->addWidget(m_formMMFeeSummary, 1, 1);

    sumLayout->addWidget(new QLabel("Total HRDF:", this), 1, 2);
    m_formMHRDFSummary = new QLabel("₹ 0.00", this);
    m_formMHRDFSummary->setObjectName("boldValue");
    sumLayout->addWidget(m_formMHRDFSummary, 1, 3);

    sumLayout->addWidget(new QLabel("Grand Total Payable to Mandi Board:", this), 1, 4);
    m_formMTotLevySummary = new QLabel("₹ 0.00", this);
    m_formMTotLevySummary->setObjectName("grandTotalLabel");
    sumLayout->addWidget(m_formMTotLevySummary, 1, 5);

    layout->addWidget(sumBox);
    m_tabWidget->addTab(tab, "Form M (Mandi Board Return)");
}

void MandiReportsWidget::setupJFormRegisterTab() {
    auto* tab = new QWidget();
    auto* layout = new QVBoxLayout(tab);
    layout->setContentsMargins(8, 8, 8, 8);
    layout->setSpacing(8);

    FiscalYearInfo fy = FiscalYearHelper::getActiveFiscalYear();
    QDate defStart = QDate::fromString(fy.startDate, "yyyy-MM-dd");
    if (!defStart.isValid()) defStart = QDate(QDate::currentDate().year(), 4, 1);
    QDate defEnd = QDate::fromString(fy.endDate, "yyyy-MM-dd");
    if (!defEnd.isValid()) defEnd = QDate::currentDate();

    auto* filterLayout = new QHBoxLayout();
    filterLayout->addWidget(new QLabel("From Date:", this));
    m_jfFromDate = new QDateEdit(defStart, this);
    m_jfFromDate->setCalendarPopup(true);
    m_jfFromDate->setDisplayFormat("dd-MM-yyyy");
    filterLayout->addWidget(m_jfFromDate);

    filterLayout->addWidget(new QLabel("To Date:", this));
    m_jfToDate = new QDateEdit(defEnd, this);
    m_jfToDate->setCalendarPopup(true);
    m_jfToDate->setDisplayFormat("dd-MM-yyyy");
    filterLayout->addWidget(m_jfToDate);

    auto* refreshBtn = new QPushButton("Filter Register", this);
    refreshBtn->setObjectName("primaryButton");
    connect(refreshBtn, &QPushButton::clicked, this, &MandiReportsWidget::generateJFormRegister);
    filterLayout->addWidget(refreshBtn);
    filterLayout->addStretch();
    layout->addLayout(filterLayout);

    m_jfTable = new QTableWidget(0, 9, this);
    m_jfTable->setHorizontalHeaderLabels({
        "Vch No", "J-Form No", "Date", "Farmer (Zimidar) Name", "Crop Description",
        "Bags", "Weight (Qtl)", "Goods Value (₹)", "Net Payable (₹)"
    });
    m_jfTable->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Stretch);
    for (int c = 0; c < 9; ++c) {
        if (c != 3) m_jfTable->horizontalHeader()->setSectionResizeMode(c, QHeaderView::ResizeToContents);
    }
    layout->addWidget(m_jfTable, 1);

    m_jfSummaryLabel = new QLabel("Total Records: 0 | Total Weight: 0.000 Qtl | Total Net Payable: ₹ 0.00", this);
    m_jfSummaryLabel->setObjectName("boldValue");
    layout->addWidget(m_jfSummaryLabel);

    m_tabWidget->addTab(tab, "J-Form Farmer Register");
}

void MandiReportsWidget::setupIFormRegisterTab() {
    auto* tab = new QWidget();
    auto* layout = new QVBoxLayout(tab);
    layout->setContentsMargins(8, 8, 8, 8);
    layout->setSpacing(8);

    FiscalYearInfo fy = FiscalYearHelper::getActiveFiscalYear();
    QDate defStart = QDate::fromString(fy.startDate, "yyyy-MM-dd");
    if (!defStart.isValid()) defStart = QDate(QDate::currentDate().year(), 4, 1);
    QDate defEnd = QDate::fromString(fy.endDate, "yyyy-MM-dd");
    if (!defEnd.isValid()) defEnd = QDate::currentDate();

    auto* filterLayout = new QHBoxLayout();
    filterLayout->addWidget(new QLabel("From Date:", this));
    m_ifFromDate = new QDateEdit(defStart, this);
    m_ifFromDate->setCalendarPopup(true);
    m_ifFromDate->setDisplayFormat("dd-MM-yyyy");
    filterLayout->addWidget(m_ifFromDate);

    filterLayout->addWidget(new QLabel("To Date:", this));
    m_ifToDate = new QDateEdit(defEnd, this);
    m_ifToDate->setCalendarPopup(true);
    m_ifToDate->setDisplayFormat("dd-MM-yyyy");
    filterLayout->addWidget(m_ifToDate);

    auto* refreshBtn = new QPushButton("Filter Register", this);
    refreshBtn->setObjectName("primaryButton");
    connect(refreshBtn, &QPushButton::clicked, this, &MandiReportsWidget::generateIFormRegister);
    filterLayout->addWidget(refreshBtn);
    filterLayout->addStretch();
    layout->addLayout(filterLayout);

    m_ifTable = new QTableWidget(0, 11, this);
    m_ifTable->setHorizontalHeaderLabels({
        "Vch No", "I-Form No", "Date", "Buyer Name", "Broker", "Crop Description",
        "Bags", "Weight (Qtl)", "Goods Value (₹)", "Dami (₹)", "Grand Total (₹)"
    });
    m_ifTable->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Stretch);
    for (int c = 0; c < 11; ++c) {
        if (c != 3) m_ifTable->horizontalHeader()->setSectionResizeMode(c, QHeaderView::ResizeToContents);
    }
    layout->addWidget(m_ifTable, 1);

    m_ifSummaryLabel = new QLabel("Total Records: 0 | Total Weight: 0.000 Qtl | Total Grand Receivable: ₹ 0.00", this);
    m_ifSummaryLabel->setObjectName("boldValue");
    layout->addWidget(m_ifSummaryLabel);

    m_tabWidget->addTab(tab, "I-Form Buyer Register");
}

void MandiReportsWidget::setupFarmerStatementTab() {
    auto* tab = new QWidget();
    auto* layout = new QVBoxLayout(tab);
    layout->setContentsMargins(8, 8, 8, 8);
    layout->setSpacing(8);

    auto* filterLayout = new QHBoxLayout();
    filterLayout->addWidget(new QLabel("Select Farmer:", this));
    m_stmtFarmerCombo = new QComboBox(this);
    m_stmtFarmerCombo->setMinimumWidth(250);
    filterLayout->addWidget(m_stmtFarmerCombo);

    auto* refreshBtn = new QPushButton("Fetch Farmer Statement", this);
    refreshBtn->setObjectName("primaryButton");
    connect(refreshBtn, &QPushButton::clicked, this, &MandiReportsWidget::generateFarmerStatement);
    filterLayout->addWidget(refreshBtn);
    filterLayout->addStretch();
    layout->addLayout(filterLayout);

    auto* splitter = new QSplitter(Qt::Vertical, this);

    // Section 1: Dheries Delivered (Credits)
    auto* dheriBox = new QGroupBox("Crop Dheries Delivered (J-Forms Credit to Farmer)", this);
    auto* dheriLayout = new QVBoxLayout(dheriBox);
    m_stmtDheriesTable = new QTableWidget(0, 7, this);
    m_stmtDheriesTable->setHorizontalHeaderLabels({
        "J-Form No", "Date", "Crop Item", "Bags", "Weight (Qtl)", "Rate (₹/Qtl)", "Net Credit (₹)"
    });
    m_stmtDheriesTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);
    dheriLayout->addWidget(m_stmtDheriesTable);
    splitter->addWidget(dheriBox);

    // Section 2: Payments Made (Debits)
    auto* payBox = new QGroupBox("Payments & Disbursements (Debits to Farmer)", this);
    auto* payLayout = new QVBoxLayout(payBox);
    m_stmtPaymentsTable = new QTableWidget(0, 5, this);
    m_stmtPaymentsTable->setHorizontalHeaderLabels({
        "Voucher No", "Date", "Voucher Type", "Amount (₹)", "Narration"
    });
    m_stmtPaymentsTable->horizontalHeader()->setSectionResizeMode(4, QHeaderView::Stretch);
    payLayout->addWidget(m_stmtPaymentsTable);
    splitter->addWidget(payBox);

    layout->addWidget(splitter, 1);

    // Bottom Summary
    auto* sumBox = new QGroupBox("Farmer Statement Balance", this);
    auto* sumLayout = new QHBoxLayout(sumBox);
    m_stmtCreditsLabel = new QLabel("Total Crop Credits: ₹ 0.00", this);
    m_stmtCreditsLabel->setObjectName("boldValue");
    m_stmtDebitsLabel = new QLabel("Total Payments Made: ₹ 0.00", this);
    m_stmtDebitsLabel->setObjectName("boldValue");
    m_stmtNetBalLabel = new QLabel("Net Balance: ₹ 0.00 Cr", this);
    m_stmtNetBalLabel->setObjectName("grandTotalLabel");

    sumLayout->addWidget(m_stmtCreditsLabel);
    sumLayout->addSpacing(20);
    sumLayout->addWidget(m_stmtDebitsLabel);
    sumLayout->addSpacing(20);
    sumLayout->addWidget(m_stmtNetBalLabel);
    sumLayout->addStretch();
    layout->addWidget(sumBox);

    m_tabWidget->addTab(tab, "Farmer / Zimidar Khata & Dheri");
}

void MandiReportsWidget::setupDamiRegisterTab() {
    auto* tab = new QWidget();
    auto* layout = new QVBoxLayout(tab);
    layout->setContentsMargins(8, 8, 8, 8);
    layout->setSpacing(8);

    FiscalYearInfo fy = FiscalYearHelper::getActiveFiscalYear();
    QDate defStart = QDate::fromString(fy.startDate, "yyyy-MM-dd");
    if (!defStart.isValid()) defStart = QDate(QDate::currentDate().year(), 4, 1);
    QDate defEnd = QDate::fromString(fy.endDate, "yyyy-MM-dd");
    if (!defEnd.isValid()) defEnd = QDate::currentDate();

    auto* filterLayout = new QHBoxLayout();
    filterLayout->addWidget(new QLabel("From Date:", this));
    m_damiFromDate = new QDateEdit(defStart, this);
    m_damiFromDate->setCalendarPopup(true);
    m_damiFromDate->setDisplayFormat("dd-MM-yyyy");
    filterLayout->addWidget(m_damiFromDate);

    filterLayout->addWidget(new QLabel("To Date:", this));
    m_damiToDate = new QDateEdit(defEnd, this);
    m_damiToDate->setCalendarPopup(true);
    m_damiToDate->setDisplayFormat("dd-MM-yyyy");
    filterLayout->addWidget(m_damiToDate);

    auto* refreshBtn = new QPushButton("Refresh Dami Book", this);
    refreshBtn->setObjectName("primaryButton");
    connect(refreshBtn, &QPushButton::clicked, this, &MandiReportsWidget::generateDamiRegister);
    filterLayout->addWidget(refreshBtn);
    filterLayout->addStretch();
    layout->addLayout(filterLayout);

    m_damiTable = new QTableWidget(0, 7, this);
    m_damiTable->setHorizontalHeaderLabels({
        "Vch No", "I-Form No", "Date", "Buyer Name", "Weight (Qtl)", "Goods Value (₹)", "Dami Earned (₹)"
    });
    m_damiTable->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Stretch);
    for (int c = 0; c < 7; ++c) {
        if (c != 3) m_damiTable->horizontalHeader()->setSectionResizeMode(c, QHeaderView::ResizeToContents);
    }
    layout->addWidget(m_damiTable, 1);

    m_damiSummaryLabel = new QLabel("Total Commission / Dami Earned: ₹ 0.00", this);
    m_damiSummaryLabel->setObjectName("grandTotalLabel");
    layout->addWidget(m_damiSummaryLabel);

    m_tabWidget->addTab(tab, "Dami & Commission Book");
}

void MandiReportsWidget::applyCustomStyles() {
    setStyleSheet(
        "MandiReportsWidget {"
        "  background-color: #F8FAFC;"
        "}"
        "QFrame#topHeaderCard {"
        "  background-color: #FFFFFF;"
        "  border: 1px solid #E2E8F0;"
        "  border-radius: 8px;"
        "}"
        "#reportTitle {"
        "  font-size: 15px;"
        "  font-weight: 800;"
        "  color: #0F172A;"
        "  background: transparent;"
        "}"
        "#reportSubtitle {"
        "  font-size: 11px;"
        "  color: #64748B;"
        "  background: transparent;"
        "}"
        "#backButton {"
        "  background-color: #FEE2E2;"
        "  color: #DC2626;"
        "  border: 1px solid #FCA5A5;"
        "  font-weight: 700;"
        "  font-size: 11.5px;"
        "  border-radius: 5px;"
        "  padding: 6px 14px;"
        "}"
        "#backButton:hover {"
        "  background-color: #FECACA;"
        "}"
        "QTabWidget::pane {"
        "  border: 1px solid #CBD5E1;"
        "  background-color: #FFFFFF;"
        "  border-radius: 8px;"
        "  top: -1px;"
        "}"
        "QTabBar::tab {"
        "  background-color: #E2E8F0;"
        "  color: #475569;"
        "  font-weight: 700;"
        "  font-size: 11.5px;"
        "  padding: 8px 16px;"
        "  border-top-left-radius: 6px;"
        "  border-top-right-radius: 6px;"
        "  margin-right: 3px;"
        "  border: 1px solid #CBD5E1;"
        "  border-bottom: none;"
        "}"
        "QTabBar::tab:selected {"
        "  background-color: #FFFFFF;"
        "  color: #1E40AF;"
        "  border: 1px solid #CBD5E1;"
        "  border-bottom: 2px solid #FFFFFF;"
        "  font-weight: 800;"
        "}"
        "QTabBar::tab:hover:!selected {"
        "  background-color: #F1F5F9;"
        "  color: #1E293B;"
        "}"
        "QGroupBox {"
        "  background-color: #FFFFFF;"
        "  border: 1px solid #E2E8F0;"
        "  border-radius: 8px;"
        "  margin-top: 14px;"
        "  padding-top: 14px;"
        "  font-weight: 700;"
        "  font-size: 11px;"
        "  color: #475569;"
        "}"
        "QGroupBox::title {"
        "  subcontrol-origin: margin;"
        "  subcontrol-position: top left;"
        "  left: 12px;"
        "  padding: 0 6px;"
        "  background-color: #FFFFFF;"
        "  color: #1E293B;"
        "  font-weight: 800;"
        "  font-size: 12px;"
        "}"
        "QLabel {"
        "  color: #334155;"
        "  font-size: 11.5px;"
        "  font-weight: 600;"
        "  background: transparent;"
        "}"
        "QLineEdit, QComboBox, QDateEdit {"
        "  background-color: #FFFFFF;"
        "  color: #0F172A;"
        "  border: 1px solid #CBD5E1;"
        "  border-radius: 5px;"
        "  padding: 4px 8px;"
        "  font-size: 12px;"
        "  font-weight: 600;"
        "}"
        "QLineEdit:focus, QComboBox:focus, QDateEdit:focus {"
        "  border: 1.5px solid #2563EB;"
        "  background-color: #EFF6FF;"
        "}"
        "QComboBox::drop-down {"
        "  border: none;"
        "  width: 22px;"
        "}"
        "QComboBox QAbstractItemView {"
        "  background-color: #FFFFFF;"
        "  color: #0F172A;"
        "  selection-background-color: #2563EB;"
        "  selection-color: #FFFFFF;"
        "  border: 1px solid #CBD5E1;"
        "  outline: none;"
        "}"
        "QTableWidget {"
        "  background-color: #FFFFFF;"
        "  alternate-background-color: #F8FAFC;"
        "  color: #0F172A;"
        "  gridline-color: #E2E8F0;"
        "  border: 1px solid #CBD5E1;"
        "  border-radius: 6px;"
        "  selection-background-color: #DBEAFE;"
        "  selection-color: #1E3A8A;"
        "  font-size: 12px;"
        "}"
        "QHeaderView::section {"
        "  background-color: #F1F5F9;"
        "  color: #334155;"
        "  font-weight: 700;"
        "  font-size: 11px;"
        "  padding: 6px 4px;"
        "  border: none;"
        "  border-right: 1px solid #CBD5E1;"
        "  border-bottom: 1px solid #CBD5E1;"
        "}"
        "QTableCornerButton::section {"
        "  background-color: #F1F5F9;"
        "  border: none;"
        "  border-right: 1px solid #CBD5E1;"
        "  border-bottom: 1px solid #CBD5E1;"
        "}"
        "QScrollBar:vertical {"
        "  background: #F1F5F9;"
        "  width: 10px;"
        "  margin: 0px;"
        "  border-radius: 4px;"
        "}"
        "QScrollBar::handle:vertical {"
        "  background: #CBD5E1;"
        "  min-height: 20px;"
        "  border-radius: 4px;"
        "}"
        "QScrollBar::handle:vertical:hover {"
        "  background: #94A3B8;"
        "}"
        "QScrollBar:horizontal {"
        "  background: #F1F5F9;"
        "  height: 10px;"
        "  margin: 0px;"
        "  border-radius: 4px;"
        "}"
        "QScrollBar::handle:horizontal {"
        "  background: #CBD5E1;"
        "  min-width: 20px;"
        "  border-radius: 4px;"
        "}"
        "QScrollBar::handle:horizontal:hover {"
        "  background: #94A3B8;"
        "}"
        "QSplitter::handle {"
        "  background-color: #CBD5E1;"
        "  height: 4px;"
        "}"
        "QPushButton {"
        "  background-color: #FFFFFF;"
        "  color: #334155;"
        "  border: 1px solid #CBD5E1;"
        "  border-radius: 5px;"
        "  padding: 6px 14px;"
        "  font-weight: 700;"
        "  font-size: 11.5px;"
        "}"
        "QPushButton:hover {"
        "  background-color: #F1F5F9;"
        "  border-color: #94A3B8;"
        "}"
        "QPushButton:pressed {"
        "  background-color: #E2E8F0;"
        "}"
        "#primaryButton {"
        "  background-color: #2563EB;"
        "  color: #FFFFFF;"
        "  border: 1px solid #1D4ED8;"
        "  font-weight: 800;"
        "}"
        "#primaryButton:hover {"
        "  background-color: #1D4ED8;"
        "}"
        "#boldValue {"
        "  font-weight: 800;"
        "  font-size: 12px;"
        "  color: #0F172A;"
        "}"
        "#grandTotalLabel {"
        "  font-weight: 900;"
        "  font-size: 15px;"
        "  color: #059669;"
        "  background-color: #ECFDF5;"
        "  border: 1px solid #A7F3D0;"
        "  border-radius: 6px;"
        "  padding: 4px 10px;"
        "}"
    );
}

void MandiReportsWidget::refreshAllTabs() {
    // Populate Farmer Combo
    QVariantList farmers = m_mandiCtrl.get_zimidar_list();
    m_stmtFarmerCombo->clear();
    for (const auto& fVar : farmers) {
        QVariantMap f = fVar.toMap();
        m_stmtFarmerCombo->addItem(f.value("party_name").toString(), f.value("id"));
    }

    generateFormM();
    generateJFormRegister();
    generateIFormRegister();
    generateFarmerStatement();
    generateDamiRegister();
}

void MandiReportsWidget::setWorkingPeriod(const QString& fromDate, const QString& toDate) {
    QDate f = QDate::fromString(fromDate, "yyyy-MM-dd");
    QDate t = QDate::fromString(toDate, "yyyy-MM-dd");
    if (f.isValid()) {
        m_formMFromDate->setDate(f);
        m_jfFromDate->setDate(f);
        m_ifFromDate->setDate(f);
        m_damiFromDate->setDate(f);
    }
    if (t.isValid()) {
        m_formMToDate->setDate(t);
        m_jfToDate->setDate(t);
        m_ifToDate->setDate(t);
        m_damiToDate->setDate(t);
    }
    refreshAllTabs();
}

void MandiReportsWidget::generateFormM() {
    QString from = m_formMFromDate->date().toString("yyyy-MM-dd");
    QString to = m_formMToDate->date().toString("yyyy-MM-dd");
    QVariantMap res = m_mandiCtrl.get_form_m_return(from, to);

    QVariantList items = res.value("items").toList();
    m_formMTable->setRowCount(0);

    for (const auto& itVar : items) {
        QVariantMap it = itVar.toMap();
        int r = m_formMTable->rowCount();
        m_formMTable->insertRow(r);

        m_formMTable->setItem(r, 0, new QTableWidgetItem(it.value("item_name").toString()));
        m_formMTable->setItem(r, 1, new QTableWidgetItem(QString::number(it.value("total_bags").toInt())));
        m_formMTable->setItem(r, 2, new QTableWidgetItem(QString::number(it.value("total_weight").toDouble(), 'f', 3)));
        m_formMTable->setItem(r, 3, new QTableWidgetItem(QString::number(it.value("total_goods_amount").toDouble(), 'f', 2)));
        m_formMTable->setItem(r, 4, new QTableWidgetItem(QString::number(it.value("avg_mfee_rate").toDouble(), 'f', 2)));
        m_formMTable->setItem(r, 5, new QTableWidgetItem(QString::number(it.value("total_mfee").toDouble(), 'f', 2)));
        m_formMTable->setItem(r, 6, new QTableWidgetItem(QString::number(it.value("avg_hrdf_rate").toDouble(), 'f', 2)));
        m_formMTable->setItem(r, 7, new QTableWidgetItem(QString::number(it.value("total_hrdf").toDouble(), 'f', 2)));
    }

    m_formMBagsSummary->setText(QString::number(res.value("total_bags").toInt()));
    m_formMWeightSummary->setText(QString("%1 Qtl").arg(QString::number(res.value("total_weight").toDouble(), 'f', 3)));
    m_formMGoodsSummary->setText(QString("₹ %1").arg(QString::number(res.value("total_goods_amount").toDouble(), 'f', 2)));
    m_formMMFeeSummary->setText(QString("₹ %1").arg(QString::number(res.value("total_mandi_fee").toDouble(), 'f', 2)));
    m_formMHRDFSummary->setText(QString("₹ %1").arg(QString::number(res.value("total_hrdf").toDouble(), 'f', 2)));
    m_formMTotLevySummary->setText(QString("₹ %1").arg(QString::number(res.value("total_levy").toDouble(), 'f', 2)));
}

void MandiReportsWidget::generateJFormRegister() {
    QString from = m_jfFromDate->date().toString("yyyy-MM-dd");
    QString to = m_jfToDate->date().toString("yyyy-MM-dd");
    QVariantList list = m_mandiCtrl.get_jform_register(from, to);

    m_jfTable->setRowCount(0);
    double totWt = 0.0;
    double totPay = 0.0;

    for (const auto& rVar : list) {
        QVariantMap r = rVar.toMap();
        int row = m_jfTable->rowCount();
        m_jfTable->insertRow(row);

        m_jfTable->setItem(row, 0, new QTableWidgetItem(QString("JFrm-%1").arg(r.value("voucher_no").toInt())));
        m_jfTable->setItem(row, 1, new QTableWidgetItem(r.value("jform_no").toString()));
        m_jfTable->setItem(row, 2, new QTableWidgetItem(r.value("voucher_date").toString()));
        m_jfTable->setItem(row, 3, new QTableWidgetItem(r.value("zimidar_name").toString()));
        m_jfTable->setItem(row, 4, new QTableWidgetItem(r.value("crop_names").toString()));
        m_jfTable->setItem(row, 5, new QTableWidgetItem(QString::number(r.value("total_bags").toInt())));
        m_jfTable->setItem(row, 6, new QTableWidgetItem(QString::number(r.value("total_weight").toDouble(), 'f', 3)));
        m_jfTable->setItem(row, 7, new QTableWidgetItem(QString::number(r.value("goods_amount").toDouble(), 'f', 2)));
        m_jfTable->setItem(row, 8, new QTableWidgetItem(QString::number(r.value("grand_total").toDouble(), 'f', 2)));

        totWt += r.value("total_weight").toDouble();
        totPay += r.value("grand_total").toDouble();
    }

    m_jfSummaryLabel->setText(QString("Total Records: %1 | Total Weight: %2 Qtl | Total Net Payable: ₹ %3")
                              .arg(list.size())
                              .arg(QString::number(totWt, 'f', 3))
                              .arg(QString::number(totPay, 'f', 2)));
}

void MandiReportsWidget::generateIFormRegister() {
    QString from = m_ifFromDate->date().toString("yyyy-MM-dd");
    QString to = m_ifToDate->date().toString("yyyy-MM-dd");
    QVariantList list = m_mandiCtrl.get_iform_register(from, to);

    m_ifTable->setRowCount(0);
    double totWt = 0.0;
    double totRec = 0.0;

    for (const auto& rVar : list) {
        QVariantMap r = rVar.toMap();
        int row = m_ifTable->rowCount();
        m_ifTable->insertRow(row);

        m_ifTable->setItem(row, 0, new QTableWidgetItem(QString("IFrm-%1").arg(r.value("voucher_no").toInt())));
        m_ifTable->setItem(row, 1, new QTableWidgetItem(r.value("iform_no").toString()));
        m_ifTable->setItem(row, 2, new QTableWidgetItem(r.value("voucher_date").toString()));
        m_ifTable->setItem(row, 3, new QTableWidgetItem(r.value("buyer_name").toString()));
        m_ifTable->setItem(row, 4, new QTableWidgetItem(r.value("broker_name").toString()));
        m_ifTable->setItem(row, 5, new QTableWidgetItem(r.value("crop_names").toString()));
        m_ifTable->setItem(row, 6, new QTableWidgetItem(QString::number(r.value("total_bags").toInt())));
        m_ifTable->setItem(row, 7, new QTableWidgetItem(QString::number(r.value("total_weight").toDouble(), 'f', 3)));
        m_ifTable->setItem(row, 8, new QTableWidgetItem(QString::number(r.value("goods_amount").toDouble(), 'f', 2)));
        m_ifTable->setItem(row, 9, new QTableWidgetItem(QString::number(r.value("dami_amount").toDouble(), 'f', 2)));
        m_ifTable->setItem(row, 10, new QTableWidgetItem(QString::number(r.value("grand_total").toDouble(), 'f', 2)));

        totWt += r.value("total_weight").toDouble();
        totRec += r.value("grand_total").toDouble();
    }

    m_ifSummaryLabel->setText(QString("Total Records: %1 | Total Weight: %2 Qtl | Total Grand Receivable: ₹ %3")
                              .arg(list.size())
                              .arg(QString::number(totWt, 'f', 3))
                              .arg(QString::number(totRec, 'f', 2)));
}

void MandiReportsWidget::generateFarmerStatement() {
    int zId = m_stmtFarmerCombo->currentData().toInt();
    if (zId <= 0) return;

    QVariantMap stmt = m_mandiCtrl.get_farmer_dheri_statement(zId);

    // Section 1: Dheries
    QVariantList dheries = stmt.value("dheries").toList();
    m_stmtDheriesTable->setRowCount(0);
    for (const auto& dVar : dheries) {
        QVariantMap d = dVar.toMap();
        int r = m_stmtDheriesTable->rowCount();
        m_stmtDheriesTable->insertRow(r);
        m_stmtDheriesTable->setItem(r, 0, new QTableWidgetItem(d.value("jform_no").toString()));
        m_stmtDheriesTable->setItem(r, 1, new QTableWidgetItem(d.value("voucher_date").toString()));
        m_stmtDheriesTable->setItem(r, 2, new QTableWidgetItem(d.value("item_name").toString()));
        m_stmtDheriesTable->setItem(r, 3, new QTableWidgetItem(QString::number(d.value("bags").toInt())));
        m_stmtDheriesTable->setItem(r, 4, new QTableWidgetItem(QString::number(d.value("weight").toDouble(), 'f', 3)));
        m_stmtDheriesTable->setItem(r, 5, new QTableWidgetItem(QString::number(d.value("rate").toDouble(), 'f', 2)));
        m_stmtDheriesTable->setItem(r, 6, new QTableWidgetItem(QString::number(d.value("grand_total").toDouble(), 'f', 2)));
    }

    // Section 2: Payments
    QVariantList payments = stmt.value("transactions").toList();
    m_stmtPaymentsTable->setRowCount(0);
    for (const auto& pVar : payments) {
        QVariantMap p = pVar.toMap();
        int r = m_stmtPaymentsTable->rowCount();
        m_stmtPaymentsTable->insertRow(r);
        m_stmtPaymentsTable->setItem(r, 0, new QTableWidgetItem(QString("%1-%2").arg(p.value("voucher_type").toString(), QString::number(p.value("voucher_no").toInt()))));
        m_stmtPaymentsTable->setItem(r, 1, new QTableWidgetItem(p.value("voucher_date").toString()));
        m_stmtPaymentsTable->setItem(r, 2, new QTableWidgetItem(p.value("voucher_type").toString()));
        m_stmtPaymentsTable->setItem(r, 3, new QTableWidgetItem(QString::number(p.value("amount").toDouble(), 'f', 2)));
        m_stmtPaymentsTable->setItem(r, 4, new QTableWidgetItem(p.value("narration").toString()));
    }

    m_stmtCreditsLabel->setText(QString("Total Crop Credits: ₹ %1").arg(QString::number(stmt.value("total_credits").toDouble(), 'f', 2)));
    m_stmtDebitsLabel->setText(QString("Total Payments Made: ₹ %1").arg(QString::number(stmt.value("total_debits").toDouble(), 'f', 2)));
    m_stmtNetBalLabel->setText(QString("Net Balance: ₹ %1 %2")
                               .arg(QString::number(stmt.value("net_balance").toDouble(), 'f', 2))
                               .arg(stmt.value("net_balance_drcr").toString()));
}

void MandiReportsWidget::generateDamiRegister() {
    QString from = m_damiFromDate->date().toString("yyyy-MM-dd");
    QString to = m_damiToDate->date().toString("yyyy-MM-dd");
    QVariantMap res = m_mandiCtrl.get_dami_commission_register(from, to);

    QVariantList rows = res.value("entries").toList();
    m_damiTable->setRowCount(0);

    for (const auto& rVar : rows) {
        QVariantMap r = rVar.toMap();
        int row = m_damiTable->rowCount();
        m_damiTable->insertRow(row);

        m_damiTable->setItem(row, 0, new QTableWidgetItem(QString("IFrm-%1").arg(r.value("voucher_no").toInt())));
        m_damiTable->setItem(row, 1, new QTableWidgetItem(r.value("iform_no").toString()));
        m_damiTable->setItem(row, 2, new QTableWidgetItem(r.value("voucher_date").toString()));
        m_damiTable->setItem(row, 3, new QTableWidgetItem(r.value("buyer_name").toString()));
        m_damiTable->setItem(row, 4, new QTableWidgetItem(QString::number(r.value("total_weight").toDouble(), 'f', 3)));
        m_damiTable->setItem(row, 5, new QTableWidgetItem(QString::number(r.value("goods_amount").toDouble(), 'f', 2)));
        m_damiTable->setItem(row, 6, new QTableWidgetItem(QString::number(r.value("dami_amount").toDouble(), 'f', 2)));
    }

    m_damiSummaryLabel->setText(QString("Total Commission / Dami Earned: ₹ %1").arg(QString::number(res.value("total_dami").toDouble(), 'f', 2)));
}

void MandiReportsWidget::keyPressEvent(QKeyEvent* event) {
    if (event->key() == Qt::Key_Escape) {
        emit backRequested();
        event->accept();
    } else {
        QWidget::keyPressEvent(event);
    }
}

bool MandiReportsWidget::eventFilter(QObject* watched, QEvent* event) {
    if (event->type() == QEvent::KeyPress) {
        auto* keyEvent = static_cast<QKeyEvent*>(event);
        if (keyEvent->key() == Qt::Key_Escape) {
            emit backRequested();
            return true;
        }
    }
    return QWidget::eventFilter(watched, event);
}


