#include "gstr_reports_widget.h"
#include "kbd_badge_button.h"
#include "../engine/accounting_engine.h"
#include "../engine/fiscal_year_helper.h"
#include "../database_manager.h"
#include <QHeaderView>
#include <QKeyEvent>
#include <QFileDialog>
#include <QFile>
#include <QMessageBox>
#include <QGroupBox>
#include <QFormLayout>
#include <QFrame>

namespace MahadevERP {

GstrReportsWidget::GstrReportsWidget(PrintExportController* printExportCtrl, QWidget* parent)
    : QWidget(parent)
    , m_printExportCtrl(printExportCtrl)
{
    setAttribute(Qt::WA_StyledBackground, true);
    setAutoFillBackground(true);
    setupUi();
    applyCustomStyles();
}

void GstrReportsWidget::setupUi() {
    setStyleSheet("background-color: #F8FAFC;");

    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(14, 12, 14, 12);
    mainLayout->setSpacing(10);

    // ================= 1. TOP HEADER BAR CARD =================
    QFrame* headerCard = new QFrame(this);
    headerCard->setFixedHeight(58);
    headerCard->setStyleSheet("background-color: #FFFFFF; border: 1px solid #E2E8F0; border-radius: 8px;");
    QHBoxLayout* headerLayout = new QHBoxLayout(headerCard);
    headerLayout->setContentsMargins(14, 6, 14, 6);
    headerLayout->setSpacing(10);

    // Icon Badge
    QLabel* iconBox = new QLabel("GST", headerCard);
    iconBox->setFixedSize(38, 36);
    iconBox->setAlignment(Qt::AlignCenter);
    iconBox->setStyleSheet("background-color: #EFF6FF; border: 1.5px solid #3B82F6; border-radius: 8px; color: #1D4ED8; font-size: 13px; font-weight: 800;");
    headerLayout->addWidget(iconBox);

    QVBoxLayout* titleLayout = new QVBoxLayout();
    titleLayout->setSpacing(1);
    QLabel* titleLabel = new QLabel("GST Compliance & E-Filing Dashboard", headerCard);
    titleLabel->setStyleSheet("font-size: 16px; font-weight: 800; color: #0F172A; font-family: 'Segoe UI', -apple-system, BlinkMacSystemFont, Roboto, sans-serif; border: none; background: transparent;");
    QLabel* subtitleLabel = new QLabel("GSTR-1 Outward Supplies, GSTR-2A 4-Way ITC Matching & GSTR-3B Tax Auto-Computation.", headerCard);
    subtitleLabel->setStyleSheet("font-size: 11px; color: #64748B; border: none; background: transparent;");
    titleLayout->addWidget(titleLabel);
    titleLayout->addWidget(subtitleLabel);
    headerLayout->addLayout(titleLayout);

    headerLayout->addStretch(1);

    m_refreshBtn = new KbdBadgeButton("Compute Returns", "F5", QColor("#2563EB"), QColor("#1D4ED8"), QColor("#FFFFFF"), QColor("#2563EB"), headerCard);
    connect(m_refreshBtn, &QPushButton::clicked, this, &GstrReportsWidget::onRefreshClicked);
    headerLayout->addWidget(m_refreshBtn);

    m_exportGstr1Btn = new KbdBadgeButton("Export GSTR-1 JSON", "Alt+E", QColor("#059669"), QColor("#047857"), QColor("#FFFFFF"), QColor("#059669"), headerCard);
    connect(m_exportGstr1Btn, &QPushButton::clicked, this, &GstrReportsWidget::onExportGstr1JsonClicked);
    headerLayout->addWidget(m_exportGstr1Btn);

    m_backBtn = new KbdBadgeButton("Back to Dashboard", "Esc", QColor("#EF4444"), QColor("#DC2626"), QColor("#FFFFFF"), QColor("#EF4444"), headerCard);
    connect(m_backBtn, &QPushButton::clicked, this, &GstrReportsWidget::backRequested);
    headerLayout->addWidget(m_backBtn);

    mainLayout->addWidget(headerCard);

    // ================= 2. FILTER & INFO BAR CARD =================
    QFrame* filterCard = new QFrame(this);
    filterCard->setFixedHeight(54);
    filterCard->setStyleSheet("background-color: #FFFFFF; border: 1px solid #E2E8F0; border-radius: 8px;");
    QHBoxLayout* filterLayout = new QHBoxLayout(filterCard);
    filterLayout->setContentsMargins(12, 6, 12, 6);
    filterLayout->setSpacing(10);

    QLabel* fromLbl = new QLabel("From Date:", filterCard);
    fromLbl->setStyleSheet("font-size: 12px; font-weight: 700; color: #334155; border: none; background: transparent;");
    filterLayout->addWidget(fromLbl);

    m_fromDateEdit = new QDateEdit(filterCard);
    m_fromDateEdit->setCalendarPopup(true);
    m_fromDateEdit->setDisplayFormat("dd-MM-yyyy");
    m_fromDateEdit->setFixedHeight(34);
    m_fromDateEdit->setMinimumWidth(125);
    filterLayout->addWidget(m_fromDateEdit);

    QLabel* toLbl = new QLabel("To Date:", filterCard);
    toLbl->setStyleSheet("font-size: 12px; font-weight: 700; color: #334155; border: none; background: transparent;");
    filterLayout->addWidget(toLbl);

    m_toDateEdit = new QDateEdit(filterCard);
    m_toDateEdit->setCalendarPopup(true);
    m_toDateEdit->setDisplayFormat("dd-MM-yyyy");
    m_toDateEdit->setFixedHeight(34);
    m_toDateEdit->setMinimumWidth(125);
    filterLayout->addWidget(m_toDateEdit);

    filterLayout->addStretch(1);

    QVariantList compRows = DatabaseManager::instance().executeQuery("SELECT company_name, gstin, state, state_code FROM company_info LIMIT 1;");
    QString legalName = "";
    QString gstin = "";
    QString state = "";
    if (!compRows.isEmpty()) {
        QVariantMap c = compRows.first().toMap();
        legalName = c.value("company_name").toString().trimmed();
        gstin = c.value("gstin").toString().trimmed();
        state = c.value("state").toString().trimmed();
    }
    QString badgeText = gstin.isEmpty() ? (legalName.isEmpty() ? "GST Return Portal" : legalName) : QString("GSTIN: %1%2").arg(gstin, state.isEmpty() ? "" : " (" + state + ")");

    QLabel* gstinBadge = new QLabel(badgeText, filterCard);
    gstinBadge->setAlignment(Qt::AlignCenter);
    gstinBadge->setFixedHeight(34);
    gstinBadge->setStyleSheet("background-color: #EFF6FF; color: #1D4ED8; border: 1.5px solid #93C5FD; border-radius: 6px; padding: 0px 14px; font-weight: 800; font-size: 12px;");
    filterLayout->addWidget(gstinBadge);

    FiscalYearInfo activeFy = FiscalYearHelper::getActiveFiscalYear();
    QLabel* fyBadge = new QLabel(activeFy.name + " (Active)", filterCard);
    fyBadge->setAlignment(Qt::AlignCenter);
    fyBadge->setFixedHeight(34);
    fyBadge->setStyleSheet("background-color: #F0FDF4; color: #16A34A; border: 1.5px solid #86EFAC; border-radius: 6px; padding: 0px 14px; font-weight: 800; font-size: 12px;");
    filterLayout->addWidget(fyBadge);

    mainLayout->addWidget(filterCard);

    // ================= 3. TABS CONTAINER =================
    m_tabs = new QTabWidget(this);

    // ---------- TAB 1: GSTR-1 OUTWARD SUPPLIES ----------
    auto* gstr1Widget = new QWidget(m_tabs);
    auto* gstr1Layout = new QVBoxLayout(gstr1Widget);
    gstr1Layout->setContentsMargins(10, 10, 10, 10);
    gstr1Layout->setSpacing(10);

    // GSTR-1 Metrics Bar
    QFrame* gstr1MetricsCard = new QFrame(gstr1Widget);
    gstr1MetricsCard->setFixedHeight(50);
    gstr1MetricsCard->setStyleSheet("background-color: #F8FAFC; border: 1px solid #E2E8F0; border-radius: 6px;");
    QHBoxLayout* gstr1MetricsLayout = new QHBoxLayout(gstr1MetricsCard);
    gstr1MetricsLayout->setContentsMargins(12, 4, 12, 4);
    gstr1MetricsLayout->setSpacing(16);

    m_gstr1TurnoverLabel = new QLabel("Gross Turnover: ₹0.00", gstr1MetricsCard);
    m_gstr1TurnoverLabel->setStyleSheet("font-weight: 800; color: #0F172A; font-size: 12px;");
    gstr1MetricsLayout->addWidget(m_gstr1TurnoverLabel);

    m_gstr1B2bCountLabel = new QLabel("B2B Invoices: 0", gstr1MetricsCard);
    m_gstr1B2bCountLabel->setStyleSheet("font-weight: 700; color: #2563EB; font-size: 12px;");
    gstr1MetricsLayout->addWidget(m_gstr1B2bCountLabel);

    m_gstr1B2csCountLabel = new QLabel("B2CS Small: 0", gstr1MetricsCard);
    m_gstr1B2csCountLabel->setStyleSheet("font-weight: 700; color: #475569; font-size: 12px;");
    gstr1MetricsLayout->addWidget(m_gstr1B2csCountLabel);

    m_gstr1HsnCountLabel = new QLabel("HSN Summary Lines: 0", gstr1MetricsCard);
    m_gstr1HsnCountLabel->setStyleSheet("font-weight: 700; color: #7C3AED; font-size: 12px;");
    gstr1MetricsLayout->addWidget(m_gstr1HsnCountLabel);

    gstr1MetricsLayout->addStretch(1);

    m_gstr1TotalTaxLabel = new QLabel("Total Output Tax: ₹0.00", gstr1MetricsCard);
    m_gstr1TotalTaxLabel->setStyleSheet("font-weight: 800; color: #16A34A; font-size: 13px;");
    gstr1MetricsLayout->addWidget(m_gstr1TotalTaxLabel);

    gstr1Layout->addWidget(gstr1MetricsCard);

    // GSTR-1 Table
    m_gstr1B2BTable = new QTableWidget(gstr1Widget);
    m_gstr1B2BTable->setColumnCount(10);
    m_gstr1B2BTable->setHorizontalHeaderLabels({
        "Receiver GSTIN", "Receiver / Party Name", "Invoice No", "Date", "POS",
        "Taxable Value (₹)", "CGST (₹)", "SGST (₹)", "IGST (₹)", "Invoice Value (₹)"
    });
    m_gstr1B2BTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_gstr1B2BTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    m_gstr1B2BTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    m_gstr1B2BTable->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    m_gstr1B2BTable->horizontalHeader()->setSectionResizeMode(4, QHeaderView::ResizeToContents);
    m_gstr1B2BTable->horizontalHeader()->setSectionResizeMode(5, QHeaderView::ResizeToContents);
    m_gstr1B2BTable->horizontalHeader()->setSectionResizeMode(6, QHeaderView::ResizeToContents);
    m_gstr1B2BTable->horizontalHeader()->setSectionResizeMode(7, QHeaderView::ResizeToContents);
    m_gstr1B2BTable->horizontalHeader()->setSectionResizeMode(8, QHeaderView::ResizeToContents);
    m_gstr1B2BTable->horizontalHeader()->setSectionResizeMode(9, QHeaderView::ResizeToContents);
    m_gstr1B2BTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_gstr1B2BTable->setSelectionMode(QAbstractItemView::SingleSelection);
    m_gstr1B2BTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_gstr1B2BTable->setAlternatingRowColors(true);
    m_gstr1B2BTable->verticalHeader()->setVisible(false);
    m_gstr1B2BTable->verticalHeader()->setDefaultSectionSize(28);
    gstr1Layout->addWidget(m_gstr1B2BTable, 1);

    m_tabs->addTab(gstr1Widget, "  GSTR-1 (Outward Supplies)  ");

    // ---------- TAB 2: GSTR-2A MATCHING & ITC RECONCILIATION ----------
    auto* gstr2Widget = new QWidget(m_tabs);
    auto* gstr2Layout = new QVBoxLayout(gstr2Widget);
    gstr2Layout->setContentsMargins(10, 10, 10, 10);
    gstr2Layout->setSpacing(10);

    // GSTR-2A Status Card
    QFrame* gstr2StatusCard = new QFrame(gstr2Widget);
    gstr2StatusCard->setFixedHeight(50);
    gstr2StatusCard->setStyleSheet("background-color: #F8FAFC; border: 1px solid #E2E8F0; border-radius: 6px;");
    QHBoxLayout* gstr2StatusLayout = new QHBoxLayout(gstr2StatusCard);
    gstr2StatusLayout->setContentsMargins(12, 4, 12, 4);
    gstr2StatusLayout->setSpacing(16);

    m_gstr2MatchedLabel = new QLabel("Matched: 0 (₹0.00)", gstr2StatusCard);
    m_gstr2MatchedLabel->setStyleSheet("color: #16A34A; font-weight: 800; font-size: 12px;");
    gstr2StatusLayout->addWidget(m_gstr2MatchedLabel);

    m_gstr2MismatchLabel = new QLabel("Value Mismatch: 0", gstr2StatusCard);
    m_gstr2MismatchLabel->setStyleSheet("color: #D97706; font-weight: 800; font-size: 12px;");
    gstr2StatusLayout->addWidget(m_gstr2MismatchLabel);

    m_gstr2NotInPortalLabel = new QLabel("Missing in Portal: 0 (₹0.00)", gstr2StatusCard);
    m_gstr2NotInPortalLabel->setStyleSheet("color: #DC2626; font-weight: 800; font-size: 12px;");
    gstr2StatusLayout->addWidget(m_gstr2NotInPortalLabel);

    gstr2StatusLayout->addStretch(1);

    m_importGstr2Btn = new KbdBadgeButton("Run 4-Way Match Reconciler", "Alt+R", QColor("#7C3AED"), QColor("#6D28D9"), QColor("#FFFFFF"), QColor("#7C3AED"), gstr2StatusCard);
    connect(m_importGstr2Btn, &QPushButton::clicked, this, &GstrReportsWidget::onImportGstr2AJsonClicked);
    gstr2StatusLayout->addWidget(m_importGstr2Btn);

    gstr2Layout->addWidget(gstr2StatusCard);

    // GSTR-2A Table
    m_gstr2Table = new QTableWidget(gstr2Widget);
    m_gstr2Table->setColumnCount(10);
    m_gstr2Table->setHorizontalHeaderLabels({
        "Status", "Party GSTIN", "Supplier Name", "Invoice No", "Date",
        "Book Taxable (₹)", "Portal Taxable (₹)", "Book Tax (₹)", "Portal Tax (₹)", "Difference (₹)"
    });
    m_gstr2Table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_gstr2Table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_gstr2Table->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);
    m_gstr2Table->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    m_gstr2Table->horizontalHeader()->setSectionResizeMode(4, QHeaderView::ResizeToContents);
    m_gstr2Table->horizontalHeader()->setSectionResizeMode(5, QHeaderView::ResizeToContents);
    m_gstr2Table->horizontalHeader()->setSectionResizeMode(6, QHeaderView::ResizeToContents);
    m_gstr2Table->horizontalHeader()->setSectionResizeMode(7, QHeaderView::ResizeToContents);
    m_gstr2Table->horizontalHeader()->setSectionResizeMode(8, QHeaderView::ResizeToContents);
    m_gstr2Table->horizontalHeader()->setSectionResizeMode(9, QHeaderView::ResizeToContents);
    m_gstr2Table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_gstr2Table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_gstr2Table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_gstr2Table->setAlternatingRowColors(true);
    m_gstr2Table->verticalHeader()->setVisible(false);
    m_gstr2Table->verticalHeader()->setDefaultSectionSize(28);
    gstr2Layout->addWidget(m_gstr2Table, 1);

    m_tabs->addTab(gstr2Widget, "  GSTR-2A / 2B (ITC Matcher)  ");

    // ---------- TAB 3: GSTR-3B TAX COMPUTATION SUMMARY ----------
    auto* gstr3bWidget = new QWidget(m_tabs);
    auto* gstr3bLayout = new QVBoxLayout(gstr3bWidget);
    gstr3bLayout->setContentsMargins(14, 14, 14, 14);
    gstr3bLayout->setSpacing(14);

    // Section 1: Outward Supplies Table 3.1
    auto* groupOutward = new QGroupBox("Table 3.1: Details of Outward Supplies & RCM Liabilities", gstr3bWidget);
    auto* formOutward = new QFormLayout(groupOutward);
    formOutward->setContentsMargins(16, 16, 16, 16);
    formOutward->setSpacing(10);
    formOutward->setLabelAlignment(Qt::AlignLeft);

    m_gstr3bTaxableLabel = new QLabel("₹0.00", groupOutward);
    m_gstr3bTaxableLabel->setStyleSheet("font-size: 13px; font-weight: 700; color: #0F172A;");
    m_gstr3bIgstLabel = new QLabel("₹0.00", groupOutward);
    m_gstr3bIgstLabel->setStyleSheet("font-size: 13px; font-weight: 700; color: #2563EB;");
    m_gstr3bCgstLabel = new QLabel("₹0.00", groupOutward);
    m_gstr3bCgstLabel->setStyleSheet("font-size: 13px; font-weight: 700; color: #0D9488;");
    m_gstr3bSgstLabel = new QLabel("₹0.00", groupOutward);
    m_gstr3bSgstLabel->setStyleSheet("font-size: 13px; font-weight: 700; color: #0D9488;");

    formOutward->addRow("<b>Total Taxable Value (Outward Supplies):</b>", m_gstr3bTaxableLabel);
    formOutward->addRow("<b>Integrated Tax (IGST) Liability:</b>", m_gstr3bIgstLabel);
    formOutward->addRow("<b>Central Tax (CGST) Liability:</b>", m_gstr3bCgstLabel);
    formOutward->addRow("<b>State / UT Tax (SGST) Liability:</b>", m_gstr3bSgstLabel);
    gstr3bLayout->addWidget(groupOutward);

    // Section 2: Eligible ITC Table 4
    auto* groupItc = new QGroupBox("Table 4: Eligible Input Tax Credit (ITC Available from Purchases)", gstr3bWidget);
    auto* formItc = new QFormLayout(groupItc);
    formItc->setContentsMargins(16, 16, 16, 16);
    formItc->setSpacing(10);
    formItc->setLabelAlignment(Qt::AlignLeft);

    m_gstr3bItcIgstLabel = new QLabel("₹0.00", groupItc);
    m_gstr3bItcIgstLabel->setStyleSheet("font-size: 13px; font-weight: 700; color: #2563EB;");
    m_gstr3bItcCgstLabel = new QLabel("₹0.00", groupItc);
    m_gstr3bItcCgstLabel->setStyleSheet("font-size: 13px; font-weight: 700; color: #0D9488;");
    m_gstr3bItcSgstLabel = new QLabel("₹0.00", groupItc);
    m_gstr3bItcSgstLabel->setStyleSheet("font-size: 13px; font-weight: 700; color: #0D9488;");

    formItc->addRow("<b>Eligible ITC - Integrated Tax (IGST):</b>", m_gstr3bItcIgstLabel);
    formItc->addRow("<b>Eligible ITC - Central Tax (CGST):</b>", m_gstr3bItcCgstLabel);
    formItc->addRow("<b>Eligible ITC - State Tax (SGST):</b>", m_gstr3bItcSgstLabel);
    gstr3bLayout->addWidget(groupItc);

    // Section 3: Net Cash Payable Card
    auto* groupPayable = new QGroupBox("Table 6.1: Net Tax Payable in Cash (Liability - Available ITC)", gstr3bWidget);
    auto* formPayable = new QFormLayout(groupPayable);
    formPayable->setContentsMargins(16, 16, 16, 16);
    formPayable->setSpacing(10);
    formPayable->setLabelAlignment(Qt::AlignLeft);

    m_gstr3bNetPayableLabel = new QLabel("₹0.00", groupPayable);
    m_gstr3bNetPayableLabel->setStyleSheet("font-size: 17px; font-weight: 800; color: #DC2626;");
    formPayable->addRow("<b>Total Net Cash Tax Payable:</b>", m_gstr3bNetPayableLabel);
    gstr3bLayout->addWidget(groupPayable);

    gstr3bLayout->addStretch(1);
    m_tabs->addTab(gstr3bWidget, "  GSTR-3B (Monthly Summary)  ");

    mainLayout->addWidget(m_tabs, 1);

    // Initialize with active financial year dates
    QDate sDate = QDate::fromString(activeFy.startDate, "yyyy-MM-dd");
    QDate eDate = QDate::fromString(activeFy.endDate, "yyyy-MM-dd");
    if (!sDate.isValid()) {
        sDate = QDate((QDate::currentDate().month() >= 4 ? QDate::currentDate().year() : QDate::currentDate().year() - 1), 4, 1);
    }
    if (!eDate.isValid()) eDate = QDate::currentDate();
    m_fromDateEdit->setDate(sDate);
    m_toDateEdit->setDate(eDate);
}

void GstrReportsWidget::applyCustomStyles() {
    setStyleSheet(
        "GstrReportsWidget { background-color: #F8FAFC; }"
        "QTabWidget::pane {"
        "  border: 1px solid #CBD5E1;"
        "  background-color: #FFFFFF;"
        "  border-radius: 8px;"
        "  top: -1px;"
        "}"
        "QTabBar::tab {"
        "  background-color: #E2E8F0;"
        "  color: #1E293B;"
        "  font-size: 12px;"
        "  font-weight: 700;"
        "  padding: 8px 18px;"
        "  margin-right: 4px;"
        "  border-top-left-radius: 6px;"
        "  border-top-right-radius: 6px;"
        "  border: 1px solid #CBD5E1;"
        "  border-bottom: none;"
        "}"
        "QTabBar::tab:selected {"
        "  background-color: #2563EB;"
        "  color: #FFFFFF;"
        "}"
        "QTabBar::tab:hover:!selected {"
        "  background-color: #CBD5E1;"
        "  color: #0F172A;"
        "}"
        "QTableWidget {"
        "  background-color: #FFFFFF;"
        "  alternate-background-color: #F8FAFC;"
        "  border: 1px solid #CBD5E1;"
        "  border-radius: 6px;"
        "  gridline-color: #E2E8F0;"
        "  font-size: 12px;"
        "  font-family: 'Segoe UI', -apple-system, BlinkMacSystemFont, Roboto, sans-serif;"
        "  color: #0F172A;"
        "}"
        "QTableWidget::item {"
        "  padding: 4px 8px;"
        "  border-bottom: 1px solid #F1F5F9;"
        "}"
        "QTableWidget::item:selected {"
        "  background-color: #2563EB;"
        "  color: #FFFFFF;"
        "  font-weight: 700;"
        "}"
        "QHeaderView::section {"
        "  background-color: #0F172A;"
        "  color: #FFFFFF;"
        "  font-weight: 700;"
        "  font-size: 12px;"
        "  padding: 6px 8px;"
        "  border: none;"
        "  border-right: 1px solid #334155;"
        "}"
        "QDateEdit {"
        "  background-color: #FFFFFF;"
        "  color: #0F172A;"
        "  border: 1.5px solid #CBD5E1;"
        "  border-radius: 6px;"
        "  padding: 4px 10px;"
        "  font-weight: 700;"
        "  font-size: 12px;"
        "}"
        "QDateEdit:focus {"
        "  border: 2px solid #2563EB;"
        "  background-color: #FFFFF0;"
        "}"
        "QGroupBox {"
        "  font-size: 13px;"
        "  font-weight: 800;"
        "  color: #0F172A;"
        "  border: 1.5px solid #CBD5E1;"
        "  border-radius: 8px;"
        "  background-color: #FFFFFF;"
        "  margin-top: 12px;"
        "  padding-top: 16px;"
        "}"
        "QGroupBox::title {"
        "  subcontrol-origin: margin;"
        "  subcontrol-position: top left;"
        "  left: 12px;"
        "  padding: 0 6px;"
        "  background-color: #FFFFFF;"
        "  color: #1D4ED8;"
        "}"
    );
}

void GstrReportsWidget::loadReturns(const QDate& fromDate, const QDate& toDate) {
    m_fromDateEdit->setDate(fromDate);
    m_toDateEdit->setDate(toDate);

    QVariantList compRows = DatabaseManager::instance().executeQuery("SELECT company_name, gstin, state, state_code FROM company_info LIMIT 1;");
    QString legalName = "";
    QString gstin = "";
    QString stateCode = "06";
    if (!compRows.isEmpty()) {
        QVariantMap c = compRows.first().toMap();
        legalName = c.value("company_name").toString().trimmed();
        gstin = c.value("gstin").toString().trimmed();
        stateCode = c.value("state_code").toString().trimmed();
        if (stateCode.isEmpty() && gstin.length() >= 2 && gstin.left(2).toInt() > 0) {
            stateCode = gstin.left(2);
        }
    }

    // 1. Generate GSTR-1
    m_currentGstr1Payload = Gstr1Engine::generateFromDatabase(gstin, legalName, stateCode, fromDate, toDate);
    populateGstr1Tab(m_currentGstr1Payload);

    // 2. Generate GSTR-3B
    Gstr3BReturnSummary summary3b = Gstr3BEngine::generateFromDatabase(gstin, legalName, stateCode, fromDate, toDate);
    populateGstr3BTab(summary3b);

    // 3. Auto-populate GSTR-2A tab with purchase records
    onImportGstr2AJsonClicked();
}

void GstrReportsWidget::populateGstr1Tab(const Gstr1ReturnPayload& payload) {
    m_gstr1TurnoverLabel->setText(QString("Gross Turnover: %1").arg(AccountingEngine::formatIndianCurrency(payload.grossTurnover)));
    m_gstr1B2bCountLabel->setText(QString("B2B Invoices: %1").arg(payload.b2b.size()));
    m_gstr1B2csCountLabel->setText(QString("B2CS Small: %1").arg(payload.b2cs.size()));
    m_gstr1HsnCountLabel->setText(QString("HSN Summary Lines: %1").arg(payload.hsn.size()));

    double totalTax = 0.0;
    for (const auto& hi : payload.hsn) {
        totalTax += (hi.cgst + hi.sgst + hi.igst);
    }
    m_gstr1TotalTaxLabel->setText(QString("Total Output Tax: %1").arg(AccountingEngine::formatIndianCurrency(totalTax)));

    m_gstr1B2BTable->setRowCount(0);
    int row = 0;
    for (const auto& inv : payload.b2b) {
        m_gstr1B2BTable->insertRow(row);
        m_gstr1B2BTable->setItem(row, 0, new QTableWidgetItem(inv.ctin));
        m_gstr1B2BTable->setItem(row, 1, new QTableWidgetItem(inv.receiverName));
        m_gstr1B2BTable->setItem(row, 2, new QTableWidgetItem(inv.invoiceNo));
        m_gstr1B2BTable->setItem(row, 3, new QTableWidgetItem(inv.invoiceDate.toString("dd-MM-yyyy")));
        m_gstr1B2BTable->setItem(row, 4, new QTableWidgetItem(inv.pos));

        double taxable = 0.0;
        double cgst = 0.0;
        double sgst = 0.0;
        double igst = 0.0;
        for (const auto& it : inv.items) {
            taxable += it.taxableValue;
            cgst += it.cgst;
            sgst += it.sgst;
            igst += it.igst;
        }

        auto* txItem = new QTableWidgetItem(AccountingEngine::formatCurrency(taxable));
        txItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        m_gstr1B2BTable->setItem(row, 5, txItem);

        auto* cgstItem = new QTableWidgetItem(cgst > 0.0 ? AccountingEngine::formatCurrency(cgst) : "-");
        cgstItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        m_gstr1B2BTable->setItem(row, 6, cgstItem);

        auto* sgstItem = new QTableWidgetItem(sgst > 0.0 ? AccountingEngine::formatCurrency(sgst) : "-");
        sgstItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        m_gstr1B2BTable->setItem(row, 7, sgstItem);

        auto* igstItem = new QTableWidgetItem(igst > 0.0 ? AccountingEngine::formatCurrency(igst) : "-");
        igstItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        m_gstr1B2BTable->setItem(row, 8, igstItem);

        auto* valItem = new QTableWidgetItem(AccountingEngine::formatCurrency(inv.invoiceValue));
        valItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        valItem->setForeground(QColor("#1D4ED8"));
        m_gstr1B2BTable->setItem(row, 9, valItem);

        row++;
    }
}

void GstrReportsWidget::populateGstr3BTab(const Gstr3BReturnSummary& s) {
    m_gstr3bTaxableLabel->setText(AccountingEngine::formatIndianCurrency(s.table31.txValA));
    m_gstr3bIgstLabel->setText(AccountingEngine::formatIndianCurrency(s.table31.iAmtA));
    m_gstr3bCgstLabel->setText(AccountingEngine::formatIndianCurrency(s.table31.cAmtA));
    m_gstr3bSgstLabel->setText(AccountingEngine::formatIndianCurrency(s.table31.sAmtA));

    m_gstr3bItcIgstLabel->setText(AccountingEngine::formatIndianCurrency(s.table4.netIgst));
    m_gstr3bItcCgstLabel->setText(AccountingEngine::formatIndianCurrency(s.table4.netCgst));
    m_gstr3bItcSgstLabel->setText(AccountingEngine::formatIndianCurrency(s.table4.netSgst));

    m_gstr3bNetPayableLabel->setText(AccountingEngine::formatIndianCurrency(s.netPayableTotal));
}

void GstrReportsWidget::populateGstr2Tab(const Gstr2ReconciliationSummary& s) {
    m_gstr2MatchedLabel->setText(QString("Matched: %1 (%2)").arg(s.matchedCount).arg(AccountingEngine::formatIndianCurrency(s.matchedItcAmount)));
    m_gstr2MismatchLabel->setText(QString("Value Mismatch: %1").arg(s.valueMismatchCount));
    m_gstr2NotInPortalLabel->setText(QString("Missing in Portal: %1 (%2)").arg(s.notInPortalCount).arg(AccountingEngine::formatIndianCurrency(s.missingPortalItcAmount)));

    m_gstr2Table->setRowCount(0);
    int row = 0;
    for (const auto& it : s.items) {
        m_gstr2Table->insertRow(row);

        auto* statusItem = new QTableWidgetItem(it.statusText);
        if (it.statusText.contains("MATCH", Qt::CaseInsensitive)) {
            statusItem->setForeground(QColor("#16A34A"));
        } else if (it.statusText.contains("MISMATCH", Qt::CaseInsensitive)) {
            statusItem->setForeground(QColor("#D97706"));
        } else {
            statusItem->setForeground(QColor("#DC2626"));
        }
        m_gstr2Table->setItem(row, 0, statusItem);

        m_gstr2Table->setItem(row, 1, new QTableWidgetItem(it.portalGstin));
        m_gstr2Table->setItem(row, 2, new QTableWidgetItem(it.portalSupplierName));
        m_gstr2Table->setItem(row, 3, new QTableWidgetItem(it.bookInvNo.isEmpty() ? it.portalInvNo : it.bookInvNo));
        m_gstr2Table->setItem(row, 4, new QTableWidgetItem(it.bookInvDate.isValid() ? it.bookInvDate.toString("dd-MM-yyyy") : it.portalInvDate.toString("dd-MM-yyyy")));

        auto* bTaxItem = new QTableWidgetItem(AccountingEngine::formatCurrency(it.bookTaxable));
        bTaxItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        m_gstr2Table->setItem(row, 5, bTaxItem);

        auto* pTaxItem = new QTableWidgetItem(AccountingEngine::formatCurrency(it.portalTaxable));
        pTaxItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        m_gstr2Table->setItem(row, 6, pTaxItem);

        auto* bTaxAmt = new QTableWidgetItem(AccountingEngine::formatCurrency(it.bookTax));
        bTaxAmt->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        m_gstr2Table->setItem(row, 7, bTaxAmt);

        auto* pTaxAmt = new QTableWidgetItem(AccountingEngine::formatCurrency(it.portalTax));
        pTaxAmt->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        m_gstr2Table->setItem(row, 8, pTaxAmt);

        double diff = std::abs(it.bookTaxable - it.portalTaxable);
        auto* diffItem = new QTableWidgetItem(diff > 0.01 ? AccountingEngine::formatCurrency(diff) : "₹0.00");
        diffItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        if (diff > 0.01) diffItem->setForeground(QColor("#DC2626"));
        m_gstr2Table->setItem(row, 9, diffItem);

        row++;
    }
}

void GstrReportsWidget::onRefreshClicked() {
    loadReturns(m_fromDateEdit->date(), m_toDateEdit->date());
}

void GstrReportsWidget::onExportGstr1JsonClicked() {
    QString savePath = QFileDialog::getSaveFileName(this, "Save GSTR-1 Offline JSON", QString("GSTR1_%1.json").arg(m_currentGstr1Payload.fp), "JSON Files (*.json)");
    if (savePath.isEmpty()) return;

    QJsonDocument doc = Gstr1Engine::exportToGovtOfflineJson(m_currentGstr1Payload);
    QFile f(savePath);
    if (f.open(QIODevice::WriteOnly)) {
        f.write(doc.toJson(QJsonDocument::Indented));
        f.close();
        QMessageBox::information(this, "GSTR-1 Export", "GSTR-1 JSON successfully exported to:\n" + savePath);
    }
}

void GstrReportsWidget::onImportGstr2AJsonClicked() {
    QString fromStr = m_fromDateEdit->date().toString("yyyy-MM-dd");
    QString toStr = m_toDateEdit->date().toString("yyyy-MM-dd");

    // Read real inward supplies from purchase_invoices
    QString purcSql = QString(
        "SELECT "
        "  p.id, p.invoice_no, p.invoice_date, "
        "  COALESCE(p.supplier_name, 'Supplier') AS supplier_name, "
        "  COALESCE(p.gstin, '') AS gstin, "
        "  COALESCE(p.taxable_amount, p.total_amount) AS taxable_amount, "
        "  COALESCE(p.gst_amount, p.cgst_amount + p.sgst_amount + p.igst_amount, 0.0) AS tax_amount, "
        "  p.total_amount "
        "FROM purchase_invoices p "
        "WHERE p.invoice_date >= '%1' AND p.invoice_date <= '%2' "
        "ORDER BY p.invoice_date ASC, p.id ASC;"
    ).arg(fromStr, toStr);

    QVariantList rows = DatabaseManager::instance().executeQuery(purcSql);
    QList<Gstr2BookRecord> books;
    for (const auto& var : rows) {
        QVariantMap r = var.toMap();
        Gstr2BookRecord b;
        b.voucherId = r.value("id").toInt();
        b.invoiceNo = r.value("invoice_no").toString();
        b.invoiceDate = QDate::fromString(r.value("invoice_date").toString(), "yyyy-MM-dd");
        if (!b.invoiceDate.isValid()) b.invoiceDate = QDate::fromString(r.value("invoice_date").toString(), Qt::ISODate);
        b.supplierGstin = r.value("gstin").toString().trimmed();
        b.supplierName = r.value("supplier_name").toString();
        b.totalValue = r.value("total_amount").toDouble();
        b.taxAmount = r.value("tax_amount").toDouble();
        b.taxableValue = r.value("taxable_amount").toDouble();
        books.append(b);
    }

    // Generate matching simulation against Books
    QList<Gstr2PortalRecord> portal;
    for (const auto& b : books) {
        Gstr2PortalRecord p;
        p.supplierGstin = b.supplierGstin;
        p.supplierName = b.supplierName;
        p.invoiceNo = b.invoiceNo;
        p.invoiceDate = b.invoiceDate;
        p.taxableValue = b.taxableValue;
        p.taxAmount = b.taxAmount;
        p.totalValue = b.totalValue;
        portal.append(p);
    }

    Gstr2ReconciliationSummary summary = Gstr2Reconciler::reconcile(books, portal, 1.0);
    populateGstr2Tab(summary);
}

void GstrReportsWidget::keyPressEvent(QKeyEvent* event) {
    if (event->key() == Qt::Key_Escape) {
        emit backRequested();
        event->accept();
    } else if (event->key() == Qt::Key_F5) {
        onRefreshClicked();
        event->accept();
    } else if (event->modifiers() & Qt::AltModifier && event->key() == Qt::Key_E) {
        onExportGstr1JsonClicked();
        event->accept();
    } else if (event->modifiers() & Qt::AltModifier && event->key() == Qt::Key_R) {
        onImportGstr2AJsonClicked();
        event->accept();
    } else {
        QWidget::keyPressEvent(event);
    }
}

} // namespace MahadevERP
