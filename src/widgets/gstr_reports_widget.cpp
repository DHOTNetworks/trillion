#include "gstr_reports_widget.h"
#include "accounting_period_dialog.h"
#include "voucher_date_dialog.h"
#include "kbd_badge_button.h"
#include "gst_portal_sync_dialog.h"
#include "../engine/accounting_engine.h"
#include "../engine/fiscal_year_helper.h"
#include "../engine/gstr1_excel_writer.h"
#include "../database_manager.h"
#include <QHeaderView>
#include <QKeyEvent>
#include <QFileDialog>
#include <QFile>
#include <QMessageBox>
#include <QGroupBox>
#include <QFormLayout>
#include <QFrame>
#include <QScrollArea>
#include <QStyleFactory>

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
    setStyleSheet("background-color: #FFFFFF;");
    // White palette inherited by every child: auto-filled surfaces can never
    // fall back to a dark system theme color (plain QWidgets + QTabWidget
    // pages do not reliably paint stylesheet backgrounds undergeath the
    // native macOS style in dark mode).
    setAutoFillBackground(true);
    {
        QPalette whitePal = palette();
        whitePal.setColor(QPalette::Window, QColor("#FFFFFF"));
        whitePal.setColor(QPalette::Base, QColor("#FFFFFF"));
        whitePal.setColor(QPalette::Button, QColor("#FFFFFF"));
        whitePal.setColor(QPalette::AlternateBase, QColor("#F8FAFC"));
        setPalette(whitePal);
    }

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

    // NOTE: export actions live on their own tabs (GSTR-1 / GSTR-3B), not here —
    // the header stays neutral: Compute (F5) + Back (Esc) only.

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

    m_fromDateEdit = new AccountingDateDisplay(filterCard);
    m_fromDateEdit->setFixedHeight(34);
    m_fromDateEdit->setFixedWidth(115);
    connect(m_fromDateEdit, &AccountingDateDisplay::dateChanged, this, &GstrReportsWidget::onRefreshClicked);
    filterLayout->addWidget(m_fromDateEdit);

    QLabel* toLbl = new QLabel("To Date:", filterCard);
    toLbl->setStyleSheet("font-size: 12px; font-weight: 700; color: #334155; border: none; background: transparent;");
    filterLayout->addWidget(toLbl);

    m_toDateEdit = new AccountingDateDisplay(filterCard);
    m_toDateEdit->setFixedHeight(34);
    m_toDateEdit->setFixedWidth(115);
    connect(m_toDateEdit, &AccountingDateDisplay::dateChanged, this, &GstrReportsWidget::onRefreshClicked);
    filterLayout->addWidget(m_toDateEdit);

    auto openPeriodDlg = [this]() {
        QDate f = m_fromDateEdit->date();
        QDate t = m_toDateEdit->date();
        if (VoucherDateDialog::selectDateRange(this, &f, &t, f, t)) {
            m_fromDateEdit->setDate(f);
            m_toDateEdit->setDate(t);
            loadReturns(f, t);
        }
    };
    connect(m_fromDateEdit, &AccountingDateDisplay::clicked, this, openPeriodDlg);
    connect(m_toDateEdit, &AccountingDateDisplay::clicked, this, openPeriodDlg);

    auto* periodBtn = new QPushButton("Period (F2)", filterCard);
    periodBtn->setStyleSheet(
        "QPushButton { background-color: #EFF6FF; color: #1D4ED8; border: 1px solid #BFDBFE; font-weight: 700; font-size: 11px; border-radius: 6px; padding: 4px 10px; cursor: pointer; }"
        "QPushButton:hover { background-color: #DBEAFE; color: #1E40AF; }"
    );
    connect(periodBtn, &QPushButton::clicked, this, openPeriodDlg);
    filterLayout->addWidget(periodBtn);

    // Month-wise division shortcut (same pattern as Salary register):
    // Month in FY order (April..March) + calendar year -> full calendar month.
    QLabel* monthLbl = new QLabel("Month:", filterCard);
    monthLbl->setStyleSheet("font-size: 12px; font-weight: 700; color: #334155; border: none; background: transparent;");
    filterLayout->addWidget(monthLbl);

    m_monthCombo = new QComboBox(filterCard);
    m_monthCombo->setFixedHeight(34);
    m_monthCombo->setStyleSheet(
        "QComboBox { background-color: #FFFFFF; color: #0F172A; border: 1.5px solid #CBD5E1; border-radius: 6px; padding: 4px 10px; font-weight: 700; font-size: 12px; }"
        "QComboBox:focus { border: 2px solid #2563EB; }");
    // Calendar order (January..December): this is a MONTHLY return view, so
    // the FY-ordered April-first list only caused FY confusion. Index == month-1.
    m_monthCombo->addItems({
        "January", "February", "March", "April", "May", "June",
        "July", "August", "September", "October", "November", "December"
    });
    {
        QSignalBlocker b(m_monthCombo);
        m_monthCombo->setCurrentIndex(QDate::currentDate().month() - 1);
    }
    connect(m_monthCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &GstrReportsWidget::onMonthYearChanged);
    filterLayout->addWidget(m_monthCombo);

    QLabel* yearLbl = new QLabel("Year:", filterCard);
    yearLbl->setStyleSheet("font-size: 12px; font-weight: 700; color: #334155; border: none; background: transparent;");
    filterLayout->addWidget(yearLbl);

    m_yearCombo = new QComboBox(filterCard);
    m_yearCombo->setFixedHeight(34);
    m_yearCombo->setStyleSheet(m_monthCombo->styleSheet());
    {
        int curY = QDate::currentDate().year();
        for (int y = curY - 2; y <= curY + 2; ++y) {
            m_yearCombo->addItem(QString::number(y), y);
        }
        QSignalBlocker b(m_yearCombo);
        m_yearCombo->setCurrentText(QString::number(curY));
    }
    connect(m_yearCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &GstrReportsWidget::onMonthYearChanged);
    filterLayout->addWidget(m_yearCombo);

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

    // FY of the SELECTED period (derived label, not a switcher): a monthly
    // return view must never offer to flip the firm's global active FY — that
    // is what kept this header contradicting the month combos.
    FiscalYearInfo activeFy = FiscalYearHelper::getActiveFiscalYear();
    m_fyBadge = new QLabel(activeFy.isValid() ? activeFy.name : "--", filterCard);
    m_fyBadge->setAlignment(Qt::AlignCenter);
    m_fyBadge->setFixedHeight(34);
    m_fyBadge->setStyleSheet("background-color: #F0FDF4; color: #16A34A; border: 1.5px solid #86EFAC; border-radius: 6px; padding: 0px 14px; font-weight: 800; font-size: 12px;");
    filterLayout->addWidget(m_fyBadge);

    mainLayout->addWidget(filterCard);

    // ================= 3. TABS CONTAINER =================
    m_tabs = new QTabWidget(this);
    // The native macOS tab bar ignores stylesheets/palette in dark mode, so
    // pin it to Fusion (fully stylesheet-driven) — scoped to this bar only.
    if (QStyle* fusion = QStyleFactory::create("Fusion")) {
        fusion->setParent(m_tabs->tabBar());
        m_tabs->tabBar()->setStyle(fusion);
    }
    // Belt-and-suspenders white: stylesheet QTabBar rule plus explicit palette
    // so the tab strip can never fall back to a dark system theme color.
    m_tabs->setAutoFillBackground(true);
    m_tabs->tabBar()->setAutoFillBackground(true);
    {
        QPalette whitePal = m_tabs->palette();
        whitePal.setColor(QPalette::Window, QColor("#FFFFFF"));
        whitePal.setColor(QPalette::Base, QColor("#FFFFFF"));
        whitePal.setColor(QPalette::Button, QColor("#FFFFFF"));
        m_tabs->setPalette(whitePal);
        m_tabs->tabBar()->setPalette(whitePal);
    }

    // ---------- TAB 1: GSTR-1 OUTWARD SUPPLIES ----------
    auto* gstr1Widget = new QWidget(m_tabs);
    gstr1Widget->setStyleSheet("background-color: #FFFFFF;");
    gstr1Widget->setAutoFillBackground(true);
    auto* gstr1Layout = new QVBoxLayout(gstr1Widget);
    gstr1Layout->setContentsMargins(10, 10, 10, 10);
    gstr1Layout->setSpacing(10);

    // GSTR-1 Action Bar (own header: title + its own exports)
    QFrame* gstr1ActionBar = new QFrame(gstr1Widget);
    gstr1ActionBar->setFixedHeight(50);
    gstr1ActionBar->setStyleSheet("background-color: #FFFFFF; border: 1px solid #E2E8F0; border-radius: 8px;");
    QHBoxLayout* gstr1ActionLayout = new QHBoxLayout(gstr1ActionBar);
    gstr1ActionLayout->setContentsMargins(14, 6, 14, 6);
    gstr1ActionLayout->setSpacing(12);
    auto* gstr1Banner = new QLabel("GSTR-1: Outward Supplies — B2B / B2CL / B2CS / Notes / HSN / Docs", gstr1ActionBar);
    gstr1Banner->setStyleSheet("font-weight: 800; color: #0F172A; font-size: 13px; border: none; background: transparent;");
    gstr1ActionLayout->addWidget(gstr1Banner);
    gstr1ActionLayout->addStretch(1);
    auto* expJsonBtn = new KbdBadgeButton("Export JSON", "Alt+E", QColor("#059669"), QColor("#047857"), QColor("#FFFFFF"), QColor("#059669"), gstr1ActionBar);
    connect(expJsonBtn, &QPushButton::clicked, this, &GstrReportsWidget::onExportGstr1JsonClicked);
    gstr1ActionLayout->addWidget(expJsonBtn);
    auto* expExcelBtn = new KbdBadgeButton("Export Excel (Govt Format)", "Alt+G", QColor("#1D4ED8"), QColor("#1E40AF"), QColor("#FFFFFF"), QColor("#1D4ED8"), gstr1ActionBar);
    connect(expExcelBtn, &QPushButton::clicked, this, &GstrReportsWidget::onExportGstr1ExcelClicked);
    gstr1ActionLayout->addWidget(expExcelBtn);
    gstr1Layout->addWidget(gstr1ActionBar);

    // GSTR-1 Metrics Bar
    QFrame* gstr1MetricsCard = new QFrame(gstr1Widget);
    gstr1MetricsCard->setFixedHeight(50);
    gstr1MetricsCard->setStyleSheet("background-color: #FFFFFF; border: 1px solid #E2E8F0; border-radius: 6px;");
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

    m_gstr1ExemptLabel = new QLabel("Exempted (T8): ₹0.00", gstr1MetricsCard);
    m_gstr1ExemptLabel->setStyleSheet("font-weight: 700; color: #0D9488; font-size: 12px;");
    gstr1MetricsLayout->addWidget(m_gstr1ExemptLabel);

    m_gstr1QuarLabel = new QLabel("Quarantined: 0", gstr1MetricsCard);
    m_gstr1QuarLabel->setStyleSheet("font-weight: 700; color: #B45309; font-size: 12px;");
    gstr1MetricsLayout->addWidget(m_gstr1QuarLabel);

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
    gstr2Widget->setStyleSheet("background-color: #FFFFFF;");
    gstr2Widget->setAutoFillBackground(true);
    auto* gstr2Layout = new QVBoxLayout(gstr2Widget);
    gstr2Layout->setContentsMargins(10, 10, 10, 10);
    gstr2Layout->setSpacing(10);

    // GSTR-2A Action Bar (own header, mirrors the other tabs)
    QFrame* gstr2ActionBar = new QFrame(gstr2Widget);
    gstr2ActionBar->setFixedHeight(50);
    gstr2ActionBar->setStyleSheet("background-color: #FFFFFF; border: 1px solid #E2E8F0; border-radius: 8px;");
    QHBoxLayout* gstr2ActionLayout = new QHBoxLayout(gstr2ActionBar);
    gstr2ActionLayout->setContentsMargins(14, 6, 14, 6);
    gstr2ActionLayout->setSpacing(12);
    auto* gstr2Banner = new QLabel("GSTR-2A/2B: ITC Matching & Reconciliation with Portal Data", gstr2ActionBar);
    gstr2Banner->setStyleSheet("font-weight: 800; color: #0F172A; font-size: 13px; border: none; background: transparent;");
    gstr2ActionLayout->addWidget(gstr2Banner);
    gstr2ActionLayout->addStretch(1);
    gstr2Layout->addWidget(gstr2ActionBar);

    // GSTR-2A Status Card
    QFrame* gstr2StatusCard = new QFrame(gstr2Widget);
    gstr2StatusCard->setFixedHeight(50);
    gstr2StatusCard->setStyleSheet("background-color: #FFFFFF; border: 1px solid #E2E8F0; border-radius: 6px;");
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

    m_autoDownloadGstr2Btn = new KbdBadgeButton("Auto-Download Portal", "Alt+D", QColor("#16A34A"), QColor("#15803D"), QColor("#FFFFFF"), QColor("#16A34A"), gstr2ActionBar);
    connect(m_autoDownloadGstr2Btn, &QPushButton::clicked, this, &GstrReportsWidget::onAutoDownloadGstr2Clicked);
    gstr2ActionLayout->addWidget(m_autoDownloadGstr2Btn);

    m_importGstr2Btn = new KbdBadgeButton("Import GSTR-2B & Match", "Alt+R", QColor("#7C3AED"), QColor("#6D28D9"), QColor("#FFFFFF"), QColor("#7C3AED"), gstr2ActionBar);
    connect(m_importGstr2Btn, &QPushButton::clicked, this, &GstrReportsWidget::onImportGstr2AJsonClicked);
    gstr2ActionLayout->addWidget(m_importGstr2Btn);

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
    auto* gstr3bScroll = new QScrollArea(m_tabs);
    gstr3bScroll->setWidgetResizable(true);
    gstr3bScroll->setFrameShape(QFrame::NoFrame);
    gstr3bScroll->setStyleSheet("background-color: #FFFFFF; border: none;");
    gstr3bScroll->viewport()->setAutoFillBackground(true);
    QPalette g3pal = gstr3bScroll->viewport()->palette();
    g3pal.setColor(QPalette::Window, QColor("#FFFFFF"));
    gstr3bScroll->viewport()->setPalette(g3pal);

    auto* gstr3bWidget = new QWidget();
    gstr3bWidget->setStyleSheet("background-color: #FFFFFF;");
    gstr3bWidget->setAutoFillBackground(true);
    auto* gstr3bLayout = new QVBoxLayout(gstr3bWidget);
    gstr3bLayout->setContentsMargins(10, 10, 10, 14);
    gstr3bLayout->setSpacing(12);

    // GSTR-3B Actions Bar
    QFrame* gstr3bActionBar = new QFrame(gstr3bWidget);
    gstr3bActionBar->setFixedHeight(50);
    gstr3bActionBar->setStyleSheet("background-color: #FFFFFF; border: 1px solid #E2E8F0; border-radius: 8px;");
    QHBoxLayout* gstr3bActionLayout = new QHBoxLayout(gstr3bActionBar);
    gstr3bActionLayout->setContentsMargins(14, 6, 14, 6);
    gstr3bActionLayout->setSpacing(12);

    m_gstr3bBanner = new QLabel("Form GSTR-3B: Monthly Summary Return & Tax Computation", gstr3bActionBar);
    m_gstr3bBanner->setStyleSheet("font-weight: 800; color: #0F172A; font-size: 13px; font-family: 'Segoe UI', -apple-system, sans-serif; border: none; background: transparent;");
    gstr3bActionLayout->addWidget(m_gstr3bBanner);

    gstr3bActionLayout->addStretch(1);

    m_gstr3bNetPayableBanner = new QLabel("Net Cash Tax Payable: ₹0.00", gstr3bActionBar);
    m_gstr3bNetPayableBanner->setStyleSheet("background-color: #FEF2F2; color: #DC2626; border: 1px solid #FCA5A5; border-radius: 6px; padding: 4px 12px; font-weight: 800; font-size: 12px;");
    gstr3bActionLayout->addWidget(m_gstr3bNetPayableBanner);

    m_exportGstr3BBtn = new KbdBadgeButton("Export GSTR-3B Excel (Govt Format)", "Alt+X", QColor("#16A34A"), QColor("#15803D"), QColor("#FFFFFF"), QColor("#16A34A"), gstr3bActionBar);
    connect(m_exportGstr3BBtn, &QPushButton::clicked, this, &GstrReportsWidget::onExportGstr3BExcelClicked);
    gstr3bActionLayout->addWidget(m_exportGstr3BBtn);

    gstr3bLayout->addWidget(gstr3bActionBar);

    auto styleTable = [](QTableWidget* table, int rowHeight = 28) {
        table->setSelectionBehavior(QAbstractItemView::SelectRows);
        table->setSelectionMode(QAbstractItemView::SingleSelection);
        table->setEditTriggers(QAbstractItemView::NoEditTriggers);
        table->setAlternatingRowColors(true);
        table->verticalHeader()->setVisible(false);
        table->verticalHeader()->setDefaultSectionSize(rowHeight);
        table->setShowGrid(true);
        table->setStyleSheet(
            "QTableWidget { background-color: #FFFFFF; alternate-background-color: #F8FAFC; border: 1px solid #E2E8F0; border-radius: 6px; gridline-color: #F1F5F9; font-size: 11.5px; color: #0F172A; }"
            "QHeaderView::section { background-color: #F1F5F9; color: #1E293B; font-weight: 800; font-size: 11.5px; padding: 6px 8px; border: none; border-bottom: 2px solid #CBD5E1; border-right: 1px solid #E2E8F0; }"
        );
    };

    // --- 1. Table 3.1: Details of Outward Supplies & RCM Liabilities ---
    auto* card31 = new QFrame(gstr3bWidget);
    card31->setStyleSheet("background-color: #FFFFFF; border: 1px solid #CBD5E1; border-radius: 8px;");
    auto* lay31 = new QVBoxLayout(card31);
    lay31->setContentsMargins(12, 10, 12, 10);
    lay31->setSpacing(6);
    auto* lbl31 = new QLabel("Table 3.1: Details of Outward Supplies and Inward Supplies Liable to Reverse Charge", card31);
    lbl31->setStyleSheet("font-size: 12.5px; font-weight: 800; color: #1D4ED8; border: none; background: transparent;");
    lay31->addWidget(lbl31);

    m_gstr3bTable31 = new QTableWidget(6, 6, card31);
    m_gstr3bTable31->setHorizontalHeaderLabels({
        "Nature of Supplies", "Total Taxable Value (₹)", "Integrated Tax (₹)", "Central Tax (₹)", "State/UT Tax (₹)", "Cess (₹)"
    });
    m_gstr3bTable31->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    for (int c = 1; c < 6; ++c) m_gstr3bTable31->horizontalHeader()->setSectionResizeMode(c, QHeaderView::ResizeToContents);
    m_gstr3bTable31->setFixedHeight(200);
    styleTable(m_gstr3bTable31);
    lay31->addWidget(m_gstr3bTable31);
    gstr3bLayout->addWidget(card31);

    // --- 1b. Table 3.2: POS-wise inter-state supplies (ties to GSTR-1) ---
    auto* card32 = new QFrame(gstr3bWidget);
    card32->setStyleSheet("background-color: #FFFFFF; border: 1px solid #CBD5E1; border-radius: 8px;");
    auto* lay32 = new QVBoxLayout(card32);
    lay32->setContentsMargins(12, 10, 12, 10);
    lay32->setSpacing(6);
    auto* lbl32 = new QLabel("Table 3.2: Inter-State Supplies to Unregistered Persons (from GSTR-1 Tables 5/7B)", card32);
    lbl32->setStyleSheet("font-size: 12.5px; font-weight: 800; color: #7C3AED; border: none; background: transparent;");
    lay32->addWidget(lbl32);

    m_gstr3bTable32 = new QTableWidget(0, 4, card32);
    m_gstr3bTable32->setHorizontalHeaderLabels({
        "Description", "Place of Supply (State/UT)", "Total Taxable Value (₹)", "Amount of Integrated Tax (₹)"
    });
    m_gstr3bTable32->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    for (int c = 1; c < 4; ++c) m_gstr3bTable32->horizontalHeader()->setSectionResizeMode(c, QHeaderView::ResizeToContents);
    m_gstr3bTable32->setFixedHeight(92);
    styleTable(m_gstr3bTable32);
    lay32->addWidget(m_gstr3bTable32);
    gstr3bLayout->addWidget(card32);

    // --- 2. Table 4: Eligible Input Tax Credit ---
    auto* card4 = new QFrame(gstr3bWidget);
    card4->setStyleSheet("background-color: #FFFFFF; border: 1px solid #CBD5E1; border-radius: 8px;");
    auto* lay4 = new QVBoxLayout(card4);
    lay4->setContentsMargins(12, 10, 12, 10);
    lay4->setSpacing(6);
    auto* lbl4 = new QLabel("Table 4: Eligible Input Tax Credit (ITC Available from Purchases)", card4);
    lbl4->setStyleSheet("font-size: 12.5px; font-weight: 800; color: #0D9488; border: none; background: transparent;");
    lay4->addWidget(lbl4);

    m_gstr3bTable4 = new QTableWidget(4, 5, card4);
    m_gstr3bTable4->setHorizontalHeaderLabels({
        "Details", "Integrated Tax (₹)", "Central Tax (₹)", "State/UT Tax (₹)", "Cess (₹)"
    });
    m_gstr3bTable4->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    for (int c = 1; c < 5; ++c) m_gstr3bTable4->horizontalHeader()->setSectionResizeMode(c, QHeaderView::ResizeToContents);
    m_gstr3bTable4->setFixedHeight(148);
    styleTable(m_gstr3bTable4);
    lay4->addWidget(m_gstr3bTable4);
    gstr3bLayout->addWidget(card4);

    // --- 3. Table 5: Values of Exempt, Nil-Rated and Non-GST Inward Supplies ---
    auto* card5 = new QFrame(gstr3bWidget);
    card5->setStyleSheet("background-color: #FFFFFF; border: 1px solid #CBD5E1; border-radius: 8px;");
    auto* lay5 = new QVBoxLayout(card5);
    lay5->setContentsMargins(12, 10, 12, 10);
    lay5->setSpacing(6);
    auto* lbl5 = new QLabel("Table 5: Values of Exempt, Nil-Rated and Non-GST Inward Supplies", card5);
    lbl5->setStyleSheet("font-size: 12.5px; font-weight: 800; color: #16A34A; border: none; background: transparent;");
    lay5->addWidget(lbl5);

    m_gstr3bTable5 = new QTableWidget(2, 3, card5);
    m_gstr3bTable5->setHorizontalHeaderLabels({
        "Nature of Supplies", "Inter-State Supplies (₹)", "Intra-State Supplies (₹)"
    });
    m_gstr3bTable5->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_gstr3bTable5->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_gstr3bTable5->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    m_gstr3bTable5->setFixedHeight(92);
    styleTable(m_gstr3bTable5);
    lay5->addWidget(m_gstr3bTable5);
    gstr3bLayout->addWidget(card5);

    // --- 4. Table 6.1: Payment of Tax (Cash Payable) ---
    auto* card61 = new QFrame(gstr3bWidget);
    card61->setStyleSheet("background-color: #FFFFFF; border: 1px solid #CBD5E1; border-radius: 8px;");
    auto* lay61 = new QVBoxLayout(card61);
    lay61->setContentsMargins(12, 10, 12, 10);
    lay61->setSpacing(6);
    auto* lbl61 = new QLabel("Table 6.1: Payment of Tax (Net Cash Liability - After ITC Offset)", card61);
    lbl61->setStyleSheet("font-size: 12.5px; font-weight: 800; color: #DC2626; border: none; background: transparent;");
    lay61->addWidget(lbl61);

    m_gstr3bTable61 = new QTableWidget(4, 5, card61);
    m_gstr3bTable61->setHorizontalHeaderLabels({
        "Description", "Total Tax Payable (₹)", "Paid Through ITC (₹)", "Tax Paid in Cash (₹)", "Interest / Late Fee (₹)"
    });
    m_gstr3bTable61->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    for (int c = 1; c < 5; ++c) m_gstr3bTable61->horizontalHeader()->setSectionResizeMode(c, QHeaderView::ResizeToContents);
    m_gstr3bTable61->setFixedHeight(148);
    styleTable(m_gstr3bTable61);
    lay61->addWidget(m_gstr3bTable61);
    gstr3bLayout->addWidget(card61);

    gstr3bScroll->setWidget(gstr3bWidget);
    m_tabs->addTab(gstr3bScroll, "  GSTR-3B (Monthly Summary)  ");

    mainLayout->addWidget(m_tabs, 1);

    // Initialize with the CURRENT MONTH's dates (combos already default to
    // the current month/year above) — never a full-year range on open.
    QDate today = QDate::currentDate();
    QDate sDate(today.year(), today.month(), 1);
    QDate eDate(today.year(), today.month(), today.daysInMonth());
    if (!sDate.isValid() || !eDate.isValid()) {
        sDate = QDate::fromString(activeFy.startDate, "yyyy-MM-dd");
        eDate = QDate::fromString(activeFy.endDate, "yyyy-MM-dd");
    }
    if (!sDate.isValid()) sDate = today;
    if (!eDate.isValid()) eDate = today;
    {
        QSignalBlocker b1(m_fromDateEdit);
        QSignalBlocker b2(m_toDateEdit);
        m_fromDateEdit->setDate(sDate);
        m_toDateEdit->setDate(eDate);
    }
    syncMonthYearCombos(sDate, eDate);
}

void GstrReportsWidget::applyCustomStyles() {
    // Fully white, seamless: every surface (root, tab bar, pages, cards)
    // paints white like the other statement views — no dark bands.
    setStyleSheet(
        "GstrReportsWidget { background-color: #FFFFFF; }"
        "QTabWidget { background-color: #FFFFFF; }"
        "QTabWidget::pane {"
        "  border: 1px solid #E2E8F0;"
        "  background-color: #FFFFFF;"
        "  border-radius: 8px;"
        "  top: -1px;"
        "}"
        "QTabBar { background-color: #FFFFFF; }"
        "QTabBar::tab {"
        "  background-color: #F1F5F9;"
        "  color: #475569;"
        "  font-size: 12px;"
        "  font-weight: 700;"
        "  padding: 8px 18px;"
        "  margin-right: 4px;"
        "  border-top-left-radius: 6px;"
        "  border-top-right-radius: 6px;"
        "  border: 1px solid #E2E8F0;"
        "  border-bottom: none;"
        "}"
        "QTabBar::tab:selected {"
        "  background-color: #2563EB;"
        "  color: #FFFFFF;"
        "  border-color: #2563EB;"
        "}"
        "QTabBar::tab:hover:!selected {"
        "  background-color: #E2E8F0;"
        "  color: #0F172A;"
        "}"
        "QTableWidget {"
        "  background-color: #FFFFFF;"
        "  alternate-background-color: #F8FAFC;"
        "  border: 1px solid #CBD5E1;"
        "  border-radius: 8px;"
        "  gridline-color: #F1F5F9;"
        "  font-size: 12.5px;"
        "  font-family: 'Segoe UI', -apple-system, BlinkMacSystemFont, Roboto, sans-serif;"
        "  color: #0F172A;"
        "}"
        "QTableWidget::item {"
        "  padding: 4px 8px;"
        "}"
        "QTableWidget::item:selected {"
        "  background-color: #EFF6FF;"
        "  color: #1D4ED8;"
        "  font-weight: bold;"
        "}"
        "QHeaderView::section {"
        "  background-color: #F1F5F9;"
        "  color: #1E293B;"
        "  font-weight: 800;"
        "  font-size: 12px;"
        "  padding: 8px 10px;"
        "  border: none;"
        "  border-bottom: 2px solid #CBD5E1;"
        "  border-right: 1px solid #E2E8F0;"
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
    if (m_fromDateEdit && m_fromDateEdit->date() != fromDate) {
        QSignalBlocker b(m_fromDateEdit);
        m_fromDateEdit->setDate(fromDate);
    }
    if (m_toDateEdit && m_toDateEdit->date() != toDate) {
        QSignalBlocker b(m_toDateEdit);
        m_toDateEdit->setDate(toDate);
    }
    syncMonthYearCombos(fromDate, toDate);

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

    // 2. Generate GSTR-3B (same-period GSTR-1 payload feeds Table 3.2,
    // so 3.2 always ties to GSTR-1 Tables 5/7B).
    Gstr3BReturnSummary summary3b = Gstr3BEngine::generateFromDatabase(
        gstin, legalName, stateCode, fromDate, toDate, &m_currentGstr1Payload);
    populateGstr3BTab(summary3b);

    // 3. GSTR-2B ITC Matching: Strict filtering to selected date range and user-loaded portal statement
    QList<Gstr2BookRecord> books;
    if (!m_loadedPortalRecords.isEmpty()) {
        books = Gstr2Reconciler::loadBookPurchasesForReconciliation(fromDate, toDate, m_loadedPortalRecords);
    } else {
        books = Gstr2Reconciler::loadBookPurchasesFromDb(fromDate, toDate);
    }

    Gstr2ReconciliationSummary summary = Gstr2Reconciler::reconcile(books, m_loadedPortalRecords, 1.0, 30);
    populateGstr2Tab(summary);
    m_isDirty = false;
}

void GstrReportsWidget::populateGstr1Tab(const Gstr1ReturnPayload& payload) {
    m_gstr1TurnoverLabel->setText(QString("Gross Turnover: %1").arg(AccountingEngine::formatIndianCurrency(payload.grossTurnover)));
    m_gstr1B2bCountLabel->setText(QString("B2B Invoices: %1").arg(payload.b2b.size()));
    m_gstr1B2csCountLabel->setText(QString("B2CS Small: %1").arg(payload.b2cs.size()));
    m_gstr1HsnCountLabel->setText(QString("HSN Summary Lines: %1").arg(payload.hsnB2B.size() + payload.hsnB2C.size()));
    double exemptTotal = 0.0;
    for (const auto& e : payload.exemp) exemptTotal += (e.nilAmt + e.exmpAmt + e.ngsupAmt);
    m_gstr1ExemptLabel->setText(QString("Exempted (T8): %1").arg(AccountingEngine::formatIndianCurrency(exemptTotal)));
    m_gstr1QuarLabel->setText(QString("Quarantined: %1").arg(payload.quarantine.size()));
    QStringList quarTip = payload.quarantine.mid(0, 10);
    m_gstr1QuarLabel->setToolTip(quarTip.isEmpty() ? "No excluded rows." : quarTip.join("\n"));

    double totalTax = 0.0;
    for (const auto& hi : payload.hsnB2B) {
        totalTax += (hi.cgst + hi.sgst + hi.igst);
    }
    for (const auto& hi : payload.hsnB2C) {
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
    m_currentGstr3BSummary = s;
    if (m_gstr3bNetPayableBanner) {
        m_gstr3bNetPayableBanner->setText(QString("Net Cash Tax Payable: %1").arg(AccountingEngine::formatIndianCurrency(s.netPayableTotal)));
    }

    auto createNumItem = [](double val, bool bold = false, const QColor& color = QColor("#0F172A"), const QColor& bg = QColor()) -> QTableWidgetItem* {
        QString txt = (std::abs(val) > 0.001) ? AccountingEngine::formatCurrency(val) : "₹0.00";
        auto* it = new QTableWidgetItem(txt);
        it->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        if (bold) {
            QFont f = it->font();
            f.setBold(true);
            it->setFont(f);
        }
        it->setForeground(color);
        if (bg.isValid()) {
            it->setBackground(bg);
        }
        return it;
    };

    auto createLabelItem = [](const QString& txt, bool bold = false, const QColor& color = QColor("#0F172A"), const QColor& bg = QColor()) -> QTableWidgetItem* {
        auto* it = new QTableWidgetItem(txt);
        it->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        if (bold) {
            QFont f = it->font();
            f.setBold(true);
            it->setFont(f);
        }
        it->setForeground(color);
        if (bg.isValid()) {
            it->setBackground(bg);
        }
        return it;
    };

    // --- Populate Table 3.1 ---
    if (m_gstr3bTable31) {
        // Row 0: 3.1(a) Outward Taxable
        m_gstr3bTable31->setItem(0, 0, createLabelItem("(a) Outward taxable supplies (other than zero rated, nil rated and exempted)"));
        m_gstr3bTable31->setItem(0, 1, createNumItem(s.table31.txValA));
        m_gstr3bTable31->setItem(0, 2, createNumItem(s.table31.iAmtA));
        m_gstr3bTable31->setItem(0, 3, createNumItem(s.table31.cAmtA));
        m_gstr3bTable31->setItem(0, 4, createNumItem(s.table31.sAmtA));
        m_gstr3bTable31->setItem(0, 5, createNumItem(s.table31.csAmtA));

        // Row 1: 3.1(b) Zero rated
        m_gstr3bTable31->setItem(1, 0, createLabelItem("(b) Outward taxable supplies (zero rated)"));
        m_gstr3bTable31->setItem(1, 1, createNumItem(s.table31.txValB));
        m_gstr3bTable31->setItem(1, 2, createNumItem(s.table31.iAmtB));
        m_gstr3bTable31->setItem(1, 3, createNumItem(0.0));
        m_gstr3bTable31->setItem(1, 4, createNumItem(0.0));
        m_gstr3bTable31->setItem(1, 5, createNumItem(s.table31.csAmtB));

        // Row 2: 3.1(c) Other outward supplies (Nil rated, exempted)
        m_gstr3bTable31->setItem(2, 0, createLabelItem("(c) Other outward supplies (Nil rated, exempted)"));
        m_gstr3bTable31->setItem(2, 1, createNumItem(s.table31.txValC, false, QColor("#16A34A")));
        m_gstr3bTable31->setItem(2, 2, createNumItem(0.0));
        m_gstr3bTable31->setItem(2, 3, createNumItem(0.0));
        m_gstr3bTable31->setItem(2, 4, createNumItem(0.0));
        m_gstr3bTable31->setItem(2, 5, createNumItem(0.0));

        // Row 3: 3.1(d) Inward supplies liable to reverse charge
        m_gstr3bTable31->setItem(3, 0, createLabelItem("(d) Inward supplies (liable to reverse charge)"));
        m_gstr3bTable31->setItem(3, 1, createNumItem(s.table31.txValD));
        m_gstr3bTable31->setItem(3, 2, createNumItem(s.table31.iAmtD));
        m_gstr3bTable31->setItem(3, 3, createNumItem(s.table31.cAmtD));
        m_gstr3bTable31->setItem(3, 4, createNumItem(s.table31.sAmtD));
        m_gstr3bTable31->setItem(3, 5, createNumItem(s.table31.csAmtD));

        // Row 4: 3.1(e) Non-GST outward supplies
        m_gstr3bTable31->setItem(4, 0, createLabelItem("(e) Non-GST outward supplies"));
        m_gstr3bTable31->setItem(4, 1, createNumItem(s.table31.txValE));
        m_gstr3bTable31->setItem(4, 2, createNumItem(0.0));
        m_gstr3bTable31->setItem(4, 3, createNumItem(0.0));
        m_gstr3bTable31->setItem(4, 4, createNumItem(0.0));
        m_gstr3bTable31->setItem(4, 5, createNumItem(0.0));

        // Row 5: Total Table 3.1
        double tot31Taxable = s.table31.txValA + s.table31.txValB + s.table31.txValC + s.table31.txValD + s.table31.txValE;
        double tot31Igst = s.table31.iAmtA + s.table31.iAmtB + s.table31.iAmtD;
        double tot31Cgst = s.table31.cAmtA + s.table31.cAmtD;
        double tot31Sgst = s.table31.sAmtA + s.table31.sAmtD;
        double tot31Cess = s.table31.csAmtA + s.table31.csAmtB + s.table31.csAmtD;
        QColor totBg("#F1F5F9");
        QColor totFg("#1E3A8A");
        m_gstr3bTable31->setItem(5, 0, createLabelItem("TOTAL OUTWARD & RCM (3.1)", true, totFg, totBg));
        m_gstr3bTable31->setItem(5, 1, createNumItem(tot31Taxable, true, totFg, totBg));
        m_gstr3bTable31->setItem(5, 2, createNumItem(tot31Igst, true, totFg, totBg));
        m_gstr3bTable31->setItem(5, 3, createNumItem(tot31Cgst, true, totFg, totBg));
        m_gstr3bTable31->setItem(5, 4, createNumItem(tot31Sgst, true, totFg, totBg));
        m_gstr3bTable31->setItem(5, 5, createNumItem(tot31Cess, true, totFg, totBg));
    }

    // --- Populate Table 3.2 (POS-wise inter-state, ties to GSTR-1) ---
    if (m_gstr3bTable32) {
        m_gstr3bTable32->setRowCount(0);
        int r32 = 0;
        for (const auto& row : s.table32.rows) {
            m_gstr3bTable32->insertRow(r32);
            m_gstr3bTable32->setItem(r32, 0, createLabelItem(row.desc));
            m_gstr3bTable32->setItem(r32, 1, createLabelItem(row.pos));
            m_gstr3bTable32->setItem(r32, 2, createNumItem(row.txVal));
            m_gstr3bTable32->setItem(r32, 3, createNumItem(row.iAmt));
            r32++;
        }
        if (r32 == 0) {
            m_gstr3bTable32->insertRow(0);
            m_gstr3bTable32->setItem(0, 0, createLabelItem("No inter-state supplies to unregistered persons in this period"));
            m_gstr3bTable32->setItem(0, 1, createLabelItem("—"));
            m_gstr3bTable32->setItem(0, 2, createNumItem(0.0));
            m_gstr3bTable32->setItem(0, 3, createNumItem(0.0));
        }
    }

    // --- Populate Table 4 (Eligible ITC) ---
    if (m_gstr3bTable4) {
        // Row 0: 4(A)(5) All other ITC
        m_gstr3bTable4->setItem(0, 0, createLabelItem("(A)(5) All other ITC (Inward supplies from registered persons)"));
        m_gstr3bTable4->setItem(0, 1, createNumItem(s.table4.iAmtA5));
        m_gstr3bTable4->setItem(0, 2, createNumItem(s.table4.cAmtA5));
        m_gstr3bTable4->setItem(0, 3, createNumItem(s.table4.sAmtA5));
        m_gstr3bTable4->setItem(0, 4, createNumItem(s.table4.csAmtA5));

        // Row 1: 4(B) ITC Reversed
        double revIgst = s.table4.iAmtB1 + s.table4.iAmtB2;
        double revCgst = s.table4.cAmtB1 + s.table4.cAmtB2;
        double revSgst = s.table4.sAmtB1 + s.table4.sAmtB2;
        m_gstr3bTable4->setItem(1, 0, createLabelItem("(B) ITC Reversed (Rule 42, 43 / Other reversals)"));
        m_gstr3bTable4->setItem(1, 1, createNumItem(revIgst));
        m_gstr3bTable4->setItem(1, 2, createNumItem(revCgst));
        m_gstr3bTable4->setItem(1, 3, createNumItem(revSgst));
        m_gstr3bTable4->setItem(1, 4, createNumItem(0.0));

        // Row 2: 4(C) Net ITC Available
        QColor itcBg("#F0FDF4");
        QColor itcFg("#15803D");
        m_gstr3bTable4->setItem(2, 0, createLabelItem("(C) NET ITC AVAILABLE (A) - (B)", true, itcFg, itcBg));
        m_gstr3bTable4->setItem(2, 1, createNumItem(s.table4.netIgst, true, itcFg, itcBg));
        m_gstr3bTable4->setItem(2, 2, createNumItem(s.table4.netCgst, true, itcFg, itcBg));
        m_gstr3bTable4->setItem(2, 3, createNumItem(s.table4.netSgst, true, itcFg, itcBg));
        m_gstr3bTable4->setItem(2, 4, createNumItem(s.table4.netCess, true, itcFg, itcBg));

        // Row 3: 4(D) Ineligible ITC
        double inelIgst = s.table4.iAmtD1 + s.table4.iAmtD2;
        double inelCgst = s.table4.cAmtD1 + s.table4.cAmtD2;
        double inelSgst = s.table4.sAmtD1 + s.table4.sAmtD2;
        m_gstr3bTable4->setItem(3, 0, createLabelItem("(D) Ineligible ITC (Section 17(5) / Others)"));
        m_gstr3bTable4->setItem(3, 1, createNumItem(inelIgst));
        m_gstr3bTable4->setItem(3, 2, createNumItem(inelCgst));
        m_gstr3bTable4->setItem(3, 3, createNumItem(inelSgst));
        m_gstr3bTable4->setItem(3, 4, createNumItem(0.0));
    }

    // --- Populate Table 5 (Exempt Inward) ---
    if (m_gstr3bTable5) {
        m_gstr3bTable5->setItem(0, 0, createLabelItem("From supplier under composition scheme, exempt and nil rated supply"));
        m_gstr3bTable5->setItem(0, 1, createNumItem(s.table5.interExempt));
        m_gstr3bTable5->setItem(0, 2, createNumItem(s.table5.intraExempt, false, QColor("#16A34A")));

        m_gstr3bTable5->setItem(1, 0, createLabelItem("Non-GST supply"));
        m_gstr3bTable5->setItem(1, 1, createNumItem(s.table5.interNonGst));
        m_gstr3bTable5->setItem(1, 2, createNumItem(s.table5.intraNonGst));
    }

    // --- Populate Table 6.1 (Tax Payment / Cash Liability) ---
    if (m_gstr3bTable61) {
        m_gstr3bTable61->setItem(0, 0, createLabelItem("Integrated Tax (IGST)"));
        m_gstr3bTable61->setItem(0, 1, createNumItem(s.table61.taxPayableIgst));
        m_gstr3bTable61->setItem(0, 2, createNumItem(s.table61.itcPaidIgst));
        m_gstr3bTable61->setItem(0, 3, createNumItem(s.table61.cashPaidIgst, true, QColor("#DC2626")));
        m_gstr3bTable61->setItem(0, 4, createNumItem(s.table61.interestPaidIgst));

        m_gstr3bTable61->setItem(1, 0, createLabelItem("Central Tax (CGST)"));
        m_gstr3bTable61->setItem(1, 1, createNumItem(s.table61.taxPayableCgst));
        m_gstr3bTable61->setItem(1, 2, createNumItem(s.table61.itcPaidCgst));
        m_gstr3bTable61->setItem(1, 3, createNumItem(s.table61.cashPaidCgst, true, QColor("#DC2626")));
        m_gstr3bTable61->setItem(1, 4, createNumItem(s.table61.interestPaidCgst + s.table61.lateFeePaidCgst));

        m_gstr3bTable61->setItem(2, 0, createLabelItem("State / UT Tax (SGST)"));
        m_gstr3bTable61->setItem(2, 1, createNumItem(s.table61.taxPayableSgst));
        m_gstr3bTable61->setItem(2, 2, createNumItem(s.table61.itcPaidSgst));
        m_gstr3bTable61->setItem(2, 3, createNumItem(s.table61.cashPaidSgst, true, QColor("#DC2626")));
        m_gstr3bTable61->setItem(2, 4, createNumItem(s.table61.interestPaidSgst + s.table61.lateFeePaidSgst));

        double totPayable = s.table61.taxPayableIgst + s.table61.taxPayableCgst + s.table61.taxPayableSgst;
        double totItcOffset = s.table61.itcPaidIgst + s.table61.itcPaidCgst + s.table61.itcPaidSgst;
        double totCashPaid = s.table61.cashPaidIgst + s.table61.cashPaidCgst + s.table61.cashPaidSgst;
        double totInterestLate = s.table61.interestPaidIgst + s.table61.interestPaidCgst + s.table61.interestPaidSgst + s.table61.lateFeePaidCgst + s.table61.lateFeePaidSgst;
        QColor payBg("#FEF2F2");
        QColor payFg("#B91C1C");
        m_gstr3bTable61->setItem(3, 0, createLabelItem("TOTAL TAX CASH LIABILITY", true, payFg, payBg));
        m_gstr3bTable61->setItem(3, 1, createNumItem(totPayable, true, payFg, payBg));
        m_gstr3bTable61->setItem(3, 2, createNumItem(totItcOffset, true, payFg, payBg));
        m_gstr3bTable61->setItem(3, 3, createNumItem(totCashPaid, true, payFg, payBg));
        m_gstr3bTable61->setItem(3, 4, createNumItem(totInterestLate, true, payFg, payBg));
    }
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

        QString gstin = !it.portalGstin.isEmpty() ? it.portalGstin : it.bookGstin;
        QString name = !it.portalSupplierName.isEmpty() ? it.portalSupplierName : it.bookSupplierName;
        QString invNo = !it.bookInvNo.isEmpty() ? it.bookInvNo : it.portalInvNo;
        QString invDate = it.bookInvDate.isValid() ? it.bookInvDate.toString("dd-MM-yyyy")
                                                   : (it.portalInvDate.isValid() ? it.portalInvDate.toString("dd-MM-yyyy") : "-");

        m_gstr2Table->setItem(row, 1, new QTableWidgetItem(gstin));
        m_gstr2Table->setItem(row, 2, new QTableWidgetItem(name));
        m_gstr2Table->setItem(row, 3, new QTableWidgetItem(invNo));
        m_gstr2Table->setItem(row, 4, new QTableWidgetItem(invDate));

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

void GstrReportsWidget::onMonthYearChanged() {
    if (!m_monthCombo || !m_yearCombo) return;
    // Calendar order: combo index 0..11 == month 1..12.
    int idx = m_monthCombo->currentIndex();
    if (idx < 0 || idx > 11) return;
    int m = idx + 1;
    int y = m_yearCombo->currentData().toInt();
    if (y <= 0) y = m_yearCombo->currentText().toInt();
    if (y <= 0) return;
    QDate first(y, m, 1);
    if (!first.isValid()) return;
    QDate last(y, m, first.daysInMonth());
    {
        QSignalBlocker b1(m_fromDateEdit);
        QSignalBlocker b2(m_toDateEdit);
        m_fromDateEdit->setDate(first);
        m_toDateEdit->setDate(last);
    }
    loadReturns(first, last);
}

void GstrReportsWidget::syncMonthYearCombos(const QDate& fromDate, const QDate& toDate) {
    if (!m_monthCombo || !m_yearCombo) return;
    if (!fromDate.isValid() || !toDate.isValid()) return;
    // Combos ALWAYS follow the selection's start month — no stale shortcuts.
    // (Custom ranges via Period (F2) are still honored for computation.)
    int m = fromDate.month();
    int y = fromDate.year();
    {
        QSignalBlocker b1(m_monthCombo);
        QSignalBlocker b2(m_yearCombo);
        m_monthCombo->setCurrentIndex(m - 1);
        int yi = m_yearCombo->findData(y);
        if (yi < 0) {
            m_yearCombo->addItem(QString::number(y), y);
            yi = m_yearCombo->findData(y);
        }
        m_yearCombo->setCurrentIndex(yi);
    }
    // FY badge derives from the same selection (never the global active FY,
    // which is what made the old header contradict itself).
    if (m_fyBadge) {
        FiscalYearInfo selFy = FiscalYearHelper::getFiscalYearForDate(fromDate.toString("yyyy-MM-dd"));
        m_fyBadge->setText(selFy.isValid() ? selFy.name : "--");
    }
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

void GstrReportsWidget::onExportGstr1ExcelClicked() {
    QString savePath = QFileDialog::getSaveFileName(this, "Save GSTR-1 Excel (Govt Offline-Tool Format)",
        QString("GSTR1_%1.xlsx").arg(m_currentGstr1Payload.fp), "Excel Files (*.xlsx)");
    if (savePath.isEmpty()) return;

    MahadevERP::Gstr1ExcelResult res = MahadevERP::Gstr1ExcelWriter::write(m_currentGstr1Payload, savePath);
    if (!res.ok) {
        QMessageBox::warning(this, "GSTR-1 Export Failed", res.error);
        return;
    }
    QString msg = QString("GSTR-1 Excel written to:\n%1\n\nB2B lines: %2 | B2CL lines: %3 | B2CS rows: %4\n"
        "CDNR lines: %5 | CDNUR lines: %6 | HSN B2B: %7 | HSN B2C: %8 | Docs: %9\n\n"
        "Open it in the official Returns Offline Tool, Validate, generate JSON, upload on gst.gov.in.")
        .arg(savePath).arg(res.stats.b2bRows).arg(res.stats.b2clRows).arg(res.stats.b2csRows)
        .arg(res.stats.cdnrRows).arg(res.stats.cdnurRows)
        .arg(res.stats.hsnB2BRows).arg(res.stats.hsnB2CRows).arg(res.stats.docRows);
    if (!m_currentGstr1Payload.quarantine.isEmpty()) {
        msg += QString("\n\nExcluded %1 row(s) needing fixes:\n- %2")
            .arg(m_currentGstr1Payload.quarantine.size())
            .arg(m_currentGstr1Payload.quarantine.join("\n- "));
    }
    if (!m_currentGstr1Payload.infoNotes.isEmpty()) {
        msg += QString("\n\nNotes:\n- %1").arg(m_currentGstr1Payload.infoNotes.join("\n- "));
    }
    QMessageBox::information(this, "GSTR-1 Export", msg);
}

void GstrReportsWidget::onExportGstr3BExcelClicked() {
    QString defName = QString("GSTR-3B_%1_%2.xls").arg(m_currentGstr3BSummary.gstin, m_currentGstr3BSummary.returnPeriod);
    QString savePath = QFileDialog::getSaveFileName(this, "Save Form GSTR-3B Excel Return", defName, "Excel Files (*.xls *.xlsx)");
    if (savePath.isEmpty()) return;

    bool ok = Gstr3BEngine::exportToExcelTemplate(m_currentGstr3BSummary, savePath);
    if (ok) {
        QMessageBox::information(this, "Export Success", QString("Form GSTR-3B (Bahi-Khata / Excel Return) successfully exported to:\n%1").arg(savePath));
    } else {
        QMessageBox::warning(this, "Export Failed", "Could not locate the standard GSTR-3B Excel template or write the export file.");
    }
}

void GstrReportsWidget::onImportGstr2AJsonClicked() {
    // Prompt user to select downloaded GSTR-2B JSON or Excel file from any directory
    QString filePath = QFileDialog::getOpenFileName(this, "Select GSTR-2B Portal JSON or Excel File", "", "GST Returns (*.json *.xlsx *.xls *.zip);;JSON Files (*.json);;Excel Files (*.xlsx *.xls);;All Files (*.*)");
    if (filePath.isEmpty()) return;

    QString retPeriod = QString("%1%2").arg(m_fromDateEdit->date().month(), 2, 10, QChar('0')).arg(m_fromDateEdit->date().year());
    QList<Gstr2PortalRecord> portal = Gstr2Reconciler::loadPortalRecordsFromFile(filePath, retPeriod);
    if (portal.isEmpty()) {
        QMessageBox::warning(this, "Parse Failed", "Could not find valid B2B inward supply records in the selected file.");
        return;
    }

    m_loadedPortalRecords = portal;

    // If the portal file has a detected return period (e.g. "042026"), synchronize the active dates to that period
    if (!portal.first().returnPeriod.isEmpty() && portal.first().returnPeriod.length() == 6) {
        int m = portal.first().returnPeriod.left(2).toInt();
        int y = portal.first().returnPeriod.mid(2).toInt();
        if (m >= 1 && m <= 12 && y >= 2000) {
            QDate pStart(y, m, 1);
            QDate pEnd(y, m, pStart.daysInMonth());
            m_fromDateEdit->setDate(pStart);
            m_toDateEdit->setDate(pEnd);
        }
    }

    loadReturns(m_fromDateEdit->date(), m_toDateEdit->date());
    m_tabs->setCurrentIndex(1); // Switch to GSTR-2 tab
}

void GstrReportsWidget::onAutoDownloadGstr2Clicked() {
    auto* dlg = new GstPortalSyncDialog(m_fromDateEdit->date(), this);
    connect(dlg, &GstPortalSyncDialog::reconciliationCompleted, this, [this](const Gstr2ReconciliationSummary& s) {
        populateGstr2Tab(s);
        m_tabs->setCurrentIndex(1);
    });
    dlg->exec();
}

void GstrReportsWidget::keyPressEvent(QKeyEvent* event) {
    if (event->key() == Qt::Key_Escape) {
        emit backRequested();
        event->accept();
    } else if (event->key() == Qt::Key_F2) {
        QDate f = m_fromDateEdit->date();
        QDate t = m_toDateEdit->date();
        if (VoucherDateDialog::selectDateRange(this, &f, &t, f, t)) {
            m_fromDateEdit->setDate(f);
            m_toDateEdit->setDate(t);
            loadReturns(f, t);
        }
        event->accept();
    } else if (event->key() == Qt::Key_F5) {
        onRefreshClicked();
        event->accept();
    } else if (event->modifiers() & Qt::AltModifier && event->key() == Qt::Key_E) {
        if (m_tabs && m_tabs->currentIndex() == 0) { // GSTR-1 tab only
            onExportGstr1JsonClicked();
            event->accept();
        } else {
            QWidget::keyPressEvent(event);
        }
    } else if (event->modifiers() & Qt::AltModifier && event->key() == Qt::Key_G) {
        if (m_tabs && m_tabs->currentIndex() == 0) { // GSTR-1 tab only
            onExportGstr1ExcelClicked();
            event->accept();
        } else {
            QWidget::keyPressEvent(event);
        }
    } else if (event->modifiers() & Qt::AltModifier && event->key() == Qt::Key_X) {
        onExportGstr3BExcelClicked();
        event->accept();
    } else if (event->modifiers() & Qt::AltModifier && event->key() == Qt::Key_R) {
        onImportGstr2AJsonClicked();
        event->accept();
    } else if (event->modifiers() & Qt::AltModifier && event->key() == Qt::Key_D) {
        onAutoDownloadGstr2Clicked();
        event->accept();
    } else {
        QWidget::keyPressEvent(event);
    }
}

} // namespace MahadevERP
