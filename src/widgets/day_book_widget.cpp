#include "day_book_widget.h"
#include "voucher_date_dialog.h"
#include "kbd_badge_button.h"
#include "erp_theme.h"
#include "custom_dialogs.h"
#include "voucher_ops.h"
#include "../database_manager.h"
#include "../engine/accounting_engine.h"
#include "../engine/fiscal_year_helper.h"
#include <QHeaderView>
#include <QKeyEvent>
#include <QShortcut>
#include <QtGlobal>
#include <QMessageBox>
#include <QFileDialog>
#include <QFrame>
#include <QDate>
#include <cmath>

namespace MahadevERP {

DayBookWidget::DayBookWidget(PrintExportController* printExportCtrl, QWidget* parent)
    : QWidget(parent)
    , m_printExportCtrl(printExportCtrl)
{
    setAttribute(Qt::WA_StyledBackground, true);
    setAutoFillBackground(true);
    setupUi();
    applyCustomStyles();
}

void DayBookWidget::setupUi() {
    // Pin the light slate palette first: any sub-control without an explicit
    // stylesheet (scroll corners, viewports, future children) inherits light
    // colors instead of falling back to the OS dark theme (= black boxes).
    ErpTheme::applySlate(this);
    setStyleSheet("background-color: #F8FAFC;");

    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(14, 12, 14, 12);
    mainLayout->setSpacing(10);

    // ================= 1. TOP HEADER BAR CARD =================
    QFrame* headerCard = new QFrame(this);
    headerCard->setObjectName("headerCard");
    headerCard->setMinimumHeight(58);
    headerCard->setStyleSheet(
        "QFrame#headerCard {"
        "  background-color: #FFFFFF;"
        "  border: 1px solid #E2E8F0;"
        "  border-radius: 8px;"
        "}"
        "QFrame#headerCard QLabel {"
        "  border: none;"
        "  background: transparent;"
        "}"
    );
    QHBoxLayout* headerLayout = new QHBoxLayout(headerCard);
    headerLayout->setContentsMargins(14, 6, 14, 6);
    headerLayout->setSpacing(10);

    // Icon Badge
    QLabel* iconBox = new QLabel("DB", headerCard);
    iconBox->setFixedSize(36, 36);
    iconBox->setAlignment(Qt::AlignCenter);
    iconBox->setStyleSheet("background-color: #EFF6FF; border: 1.5px solid #3B82F6; border-radius: 8px; color: #1D4ED8; font-size: 13px; font-weight: 800;");
    headerLayout->addWidget(iconBox);

    QVBoxLayout* titleLayout = new QVBoxLayout();
    titleLayout->setSpacing(1);
    QLabel* titleLabel = new QLabel("Day Book / Daily Transaction Audit Register", headerCard);
    titleLabel->setStyleSheet("font-size: 16px; font-weight: 800; color: #0F172A; font-family: 'Segoe UI', -apple-system, BlinkMacSystemFont, Roboto, sans-serif; border: none; background: transparent;");
    QLabel* subtitleLabel = new QLabel("Chronological journal & voucher register across all sales, purchases, bank, cash, and mandi postings.", headerCard);
    subtitleLabel->setStyleSheet("font-size: 11px; color: #64748B; border: none; background: transparent;");
    titleLayout->addWidget(titleLabel);
    titleLayout->addWidget(subtitleLabel);
    headerLayout->addLayout(titleLayout);

    headerLayout->addStretch(1);

    m_refreshBtn = new KbdBadgeButton("Refresh Register", "F5", QColor("#2563EB"), QColor("#1D4ED8"), QColor("#FFFFFF"), QColor("#2563EB"), headerCard);
    connect(m_refreshBtn, &QPushButton::clicked, this, &DayBookWidget::onRefreshClicked);
    headerLayout->addWidget(m_refreshBtn);

    m_pdfBtn = new KbdBadgeButton("Export PDF", "Alt+P", QColor("#059669"), QColor("#047857"), QColor("#FFFFFF"), QColor("#059669"), headerCard);
    connect(m_pdfBtn, &QPushButton::clicked, this, &DayBookWidget::onExportPdfClicked);
    headerLayout->addWidget(m_pdfBtn);

    m_backBtn = new KbdBadgeButton("Back to Dashboard", "Esc", QColor("#EF4444"), QColor("#DC2626"), QColor("#FFFFFF"), QColor("#EF4444"), headerCard);
    connect(m_backBtn, &QPushButton::clicked, this, &DayBookWidget::backRequested);
    headerLayout->addWidget(m_backBtn);

    mainLayout->addWidget(headerCard);

    // ================= 2. FILTER & CONTROL BAR CARD =================
    QFrame* filterCard = new QFrame(this);
    filterCard->setObjectName("filterCard");
    filterCard->setMinimumHeight(54);
    filterCard->setStyleSheet(
        "QFrame#filterCard {"
        "  background-color: #FFFFFF;"
        "  border: 1px solid #E2E8F0;"
        "  border-radius: 8px;"
        "}"
        "QFrame#filterCard QLabel {"
        "  border: none;"
        "  background: transparent;"
        "}"
    );
    QHBoxLayout* filterLayout = new QHBoxLayout(filterCard);
    filterLayout->setContentsMargins(12, 6, 12, 6);
    filterLayout->setSpacing(10);

    // From Date
    QLabel* fromLbl = new QLabel("From Date:", filterCard);
    fromLbl->setStyleSheet("font-size: 12px; font-weight: 700; color: #334155; border: none; background: transparent;");
    filterLayout->addWidget(fromLbl);

    FiscalYearInfo initFy = FiscalYearHelper::getActiveFiscalYear();
    QDate initStart = QDate::fromString(initFy.startDate, "yyyy-MM-dd");
    QDate initEnd = QDate::fromString(initFy.endDate, "yyyy-MM-dd");
    if (!initStart.isValid()) initStart = QDate(QDate::currentDate().month() < 4 ? QDate::currentDate().year() - 1 : QDate::currentDate().year(), 4, 1);
    if (!initEnd.isValid()) initEnd = QDate::currentDate();

    m_fromDateEdit = new AccountingDateDisplay(filterCard);
    m_fromDateEdit->setMinimumHeight(34);
    m_fromDateEdit->setMinimumWidth(115);
    m_fromDateEdit->setDate(initStart);
    connect(m_fromDateEdit, &AccountingDateDisplay::dateChanged, this, &DayBookWidget::onDateChanged);
    filterLayout->addWidget(m_fromDateEdit);

    // To Date
    QLabel* toLbl = new QLabel("To Date:", filterCard);
    toLbl->setStyleSheet("font-size: 12px; font-weight: 700; color: #334155; border: none; background: transparent;");
    filterLayout->addWidget(toLbl);

    m_toDateEdit = new AccountingDateDisplay(filterCard);
    m_toDateEdit->setMinimumHeight(34);
    m_toDateEdit->setMinimumWidth(115);
    m_toDateEdit->setDate(initEnd);
    connect(m_toDateEdit, &AccountingDateDisplay::dateChanged, this, &DayBookWidget::onDateChanged);
    filterLayout->addWidget(m_toDateEdit);

    auto openPeriodDlg = [this]() {
        QDate f = m_fromDateEdit->date();
        QDate t = m_toDateEdit->date();
        if (VoucherDateDialog::selectDateRange(this, &f, &t, f, t)) {
            m_fromDateEdit->setDate(f);
            m_toDateEdit->setDate(t);
        }
    };
    connect(m_fromDateEdit, &AccountingDateDisplay::clicked, this, openPeriodDlg);
    connect(m_toDateEdit, &AccountingDateDisplay::clicked, this, openPeriodDlg);

    auto* periodBtn = new QPushButton("Period (F2)", filterCard);
    periodBtn->setStyleSheet(
        "QPushButton { background-color: #EFF6FF; color: #1D4ED8; border: 1px solid #BFDBFE; font-weight: 700; font-size: 11px; border-radius: 6px; padding: 4px 10px; }"
        "QPushButton:hover { background-color: #DBEAFE; color: #1E40AF; }"
    );
    connect(periodBtn, &QPushButton::clicked, this, openPeriodDlg);
    filterLayout->addWidget(periodBtn);

    // Type Filter
    QLabel* typeLbl = new QLabel("Voucher Type:", filterCard);
    typeLbl->setStyleSheet("font-size: 12px; font-weight: 700; color: #334155; border: none; background: transparent;");
    filterLayout->addWidget(typeLbl);

    m_typeFilterCombo = new QComboBox(filterCard);
    m_typeFilterCombo->setMinimumHeight(34);
    m_typeFilterCombo->setMinimumWidth(170);
    m_typeFilterCombo->addItems({"ALL Vouchers", "Sales (SL)", "Purchases (PU)", "Journal (JV)", "Bank / Cheque (BK)", "Cash Book (CB)", "J-Form (JFrm)", "I-Form (IFrm)", "Milling Production", "TDS Deductions"});
    connect(m_typeFilterCombo, &QComboBox::currentTextChanged, this, &DayBookWidget::onRefreshClicked);
    filterLayout->addWidget(m_typeFilterCombo);

    // Search Box (debounced: one reload per typing pause, not per keystroke)
    m_searchBox = new QLineEdit(filterCard);
    m_searchBox->setPlaceholderText("Filter by Party, Narration, Opposing A/c...");
    m_searchBox->setMinimumHeight(34);
    m_searchBox->setMinimumWidth(260);
    m_searchDebounce = new QTimer(this);
    m_searchDebounce->setSingleShot(true);
    m_searchDebounce->setInterval(250);
    connect(m_searchDebounce, &QTimer::timeout, this, &DayBookWidget::onRefreshClicked);
    connect(m_searchBox, &QLineEdit::textChanged, this, [this]() { m_searchDebounce->start(); });
    filterLayout->addWidget(m_searchBox);

    filterLayout->addStretch(1);

    // Period Badge
    FiscalYearInfo activeFy = FiscalYearHelper::getActiveFiscalYear();
    QLabel* fyBadge = new QLabel(activeFy.name + " (Active)", filterCard);
    fyBadge->setAlignment(Qt::AlignCenter);
    fyBadge->setMinimumHeight(34);
    fyBadge->setStyleSheet("background-color: #F0FDF4; color: #16A34A; border: 1px solid #BBF7D0; border-radius: 6px; padding: 0px 14px; font-weight: 700; font-size: 11.5px;");
    filterLayout->addWidget(fyBadge);

    mainLayout->addWidget(filterCard);

    // ================= 3. DAY BOOK TABLE =================
    m_table = new QTableWidget(this);
    m_table->setColumnCount(7);
    m_table->setHorizontalHeaderLabels({"Date", "Vch No", "Type", "Particulars / Account Name", "Opposing Account / Narration", "Debit (₹)", "Credit (₹)"});
    m_table->horizontalHeader()->setMinimumHeight(36);
    m_table->horizontalHeader()->setDefaultAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    // Fixed widths (NOT ResizeToContents): with tens of thousands of rows,
    // ResizeToContents re-measures + HarfBuzz-shapes every cell on EVERY
    // setItem (profiled: 100% of a 1s+ main-thread hang in sizeHintForColumn).
    // Columns 3/4 stretch to absorb the remaining width; no per-row measuring.
    m_table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Fixed);
    m_table->setColumnWidth(0, 95);
    m_table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Fixed);
    m_table->setColumnWidth(1, 140);
    m_table->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Fixed);
    m_table->setColumnWidth(2, 110);
    m_table->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Stretch);
    m_table->horizontalHeader()->setSectionResizeMode(4, QHeaderView::Stretch);
    m_table->horizontalHeader()->setSectionResizeMode(5, QHeaderView::Fixed);
    m_table->setColumnWidth(5, 130);
    m_table->horizontalHeader()->setSectionResizeMode(6, QHeaderView::Fixed);
    m_table->setColumnWidth(6, 130);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setAlternatingRowColors(true);
    m_table->verticalHeader()->setVisible(false);
    m_table->verticalHeader()->setDefaultSectionSize(ErpTheme::kTableRowH);
    m_table->setShowGrid(true);
    m_table->setStyleSheet(
        "QTableWidget { background-color: #FFFFFF; alternate-background-color: #F8FAFC; border: 1px solid #CBD5E1; border-radius: 8px; gridline-color: #F1F5F9; font-size: 12.5px; color: #0F172A; }"
        "QTableWidget::item { padding: 5px 12px; }"
        "QTableWidget::item:selected { background-color: #2563EB; color: #FFFFFF; }"
        "QHeaderView::section { background-color: #0F172A; color: #FFFFFF; font-weight: 800; font-size: 12px; min-height: 28px; height: 34px; padding: 6px 12px; border: none;  border-right: 1px solid #334155; }"
        // Corner square between the headers: unstyled it falls back to the OS
        // theme (black in macOS dark mode). Pin to the navy header contract.
        "QTableWidget QTableCornerButton::section { background: #0F172A; border: none; }"
        // Scrollbars: same story — pin light slate so dark mode can't paint them black.
        "QScrollBar:vertical { background: #F1F5F9; width: 12px; margin: 0; border: none; }"
        "QScrollBar::handle:vertical { background: #CBD5E1; min-height: 24px; border-radius: 6px; }"
        "QScrollBar::handle:vertical:hover { background: #94A3B8; }"
        "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0; background: none; border: none; }"
        "QScrollBar:horizontal { background: #F1F5F9; height: 12px; margin: 0; border: none; }"
        "QScrollBar::handle:horizontal { background: #CBD5E1; min-width: 24px; border-radius: 6px; }"
        "QScrollBar::handle:horizontal:hover { background: #94A3B8; }"
        "QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal { width: 0; background: none; border: none; }"
    );

    // Align header items
    for (int col = 0; col < m_table->columnCount(); ++col) {
        if (auto* itm = m_table->horizontalHeaderItem(col)) {
            if (col == 0 || col == 1 || col == 2) itm->setTextAlignment(Qt::AlignCenter | Qt::AlignVCenter);
            else if (col == 3 || col == 4) itm->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
            else itm->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        }
    }

    connect(m_table, &QTableWidget::cellDoubleClicked, this, &DayBookWidget::onRowDoubleClicked);
    // Enter opens the focused row's alteration (same as double-click).
    // Scoped to the grid so search boxes and buttons keep their own keys.
    auto openFocusedRow = [this]() {
        if (!m_table || m_table->currentRow() < 0) return;
        onRowDoubleClicked(m_table->currentRow(), m_table->currentColumn());
    };
    auto* enterShortcut = new QShortcut(QKeySequence(Qt::Key_Return), m_table);
    enterShortcut->setContext(Qt::WidgetWithChildrenShortcut);
    connect(enterShortcut, &QShortcut::activated, this, openFocusedRow);
    auto* numEnterShortcut = new QShortcut(QKeySequence(Qt::Key_Enter), m_table);
    numEnterShortcut->setContext(Qt::WidgetWithChildrenShortcut);
    connect(numEnterShortcut, &QShortcut::activated, this, openFocusedRow);
    // Alt+D deletes the focused voucher (shared VoucherOps confirm + delete).
    auto* delShortcut = new QShortcut(QKeySequence(Qt::ALT | Qt::Key_D), m_table);
    delShortcut->setContext(Qt::WidgetWithChildrenShortcut);
    connect(delShortcut, &QShortcut::activated, this, &DayBookWidget::deleteFocusedRow);
    mainLayout->addWidget(m_table, 1);

    // ================= 4. SUMMARY FOOTER BAR CARD =================
    QFrame* footerCard = new QFrame(this);
    footerCard->setObjectName("footerCard");
    footerCard->setMinimumHeight(44);
    footerCard->setStyleSheet(
        "QFrame#footerCard {"
        "  background-color: #FFFFFF;"
        "  border: 1px solid #CBD5E1;"
        "  border-radius: 8px;"
        "}"
        "QFrame#footerCard QLabel {"
        "  border: none;"
        "  background: transparent;"
        "}"
    );
    QHBoxLayout* footerLayout = new QHBoxLayout(footerCard);
    footerLayout->setContentsMargins(14, 4, 14, 4);
    footerLayout->setSpacing(14);

    m_rowCountLabel = new QLabel("Total Records: 0", footerCard);
    m_rowCountLabel->setStyleSheet("font-weight: 700; color: #475569; font-size: 12px; border: none; background: transparent;");
    footerLayout->addWidget(m_rowCountLabel);

    footerLayout->addStretch(1);

    m_totalDrLabel = new QLabel("Total Debit: ₹0.00", footerCard);
    m_totalDrLabel->setStyleSheet("font-weight: 800; color: #16A34A; font-size: 13.5px; border: none; background: transparent;");
    footerLayout->addWidget(m_totalDrLabel);

    footerLayout->addSpacing(20);

    m_totalCrLabel = new QLabel("Total Credit: ₹0.00", footerCard);
    m_totalCrLabel->setStyleSheet("font-weight: 800; color: #DC2626; font-size: 13.5px; border: none; background: transparent;");
    footerLayout->addWidget(m_totalCrLabel);

    mainLayout->addWidget(footerCard);

    // Initialize with active financial year dates
    QDate sDate = QDate::fromString(activeFy.startDate, "yyyy-MM-dd");
    QDate eDate = QDate::fromString(activeFy.endDate, "yyyy-MM-dd");
    if (!sDate.isValid()) sDate = QDate(QDate::currentDate().month() < 4 ? QDate::currentDate().year() - 1 : QDate::currentDate().year(), 4, 1);
    if (!eDate.isValid()) eDate = sDate.addYears(1).addDays(-1);
    {
        QSignalBlocker b1(m_fromDateEdit);
        QSignalBlocker b2(m_toDateEdit);
        m_fromDateEdit->setDate(sDate);
        m_toDateEdit->setDate(eDate);
    }
    onRefreshClicked();
}

void DayBookWidget::applyCustomStyles() {
    // NOTE: QTableWidget/QHeaderView rules intentionally live ONLY on m_table's
    // own stylesheet in setupUi (a widget's own sheet wins over inherited ones,
    // so duplicates here would be dead weight that drifts). This block covers
    // the borderless inputs that carry no per-widget sheet.
    setStyleSheet(
        "DayBookWidget { background-color: #F8FAFC; }"
        "QDateEdit, QComboBox, QLineEdit {"
        "  background-color: #FFFFFF;"
        "  color: #0F172A;"
        "  border: 1.5px solid #CBD5E1;"
        "  border-radius: 6px;"
        "  padding: 4px 10px;"
        "  font-weight: 700;"
        "  font-size: 12px;"
        "}"
        "QDateEdit:focus, QComboBox:focus, QLineEdit:focus {"
        "  border: 2px solid #2563EB;"
        "  background-color: #FFFFF0;"
        "}"
    );
}

void DayBookWidget::loadDayBookData(const QDate& fromDate, const QDate& toDate, const QString& voucherTypeFilter) {
    {
        QSignalBlocker b1(m_fromDateEdit);
        QSignalBlocker b2(m_toDateEdit);
        if (fromDate.isValid()) m_fromDateEdit->setDate(fromDate);
        if (toDate.isValid()) m_toDateEdit->setDate(toDate);
    }

    QString fromStr = m_fromDateEdit->date().toString("yyyy-MM-dd");
    QString toStr = m_toDateEdit->date().toString("yyyy-MM-dd");

    // Robust query reading from transactions table containing all double-entry postings
    QString sql = QString(
        "SELECT "
        "  id, "
        "  voucher_date AS date, "
        "  voucher_no, "
        "  voucher_type, "
        "  trans_type, "
        "  COALESCE(financial_year, '') AS financial_year, "
        "  COALESCE(party_name, '') AS party_name, "
        "  COALESCE(opposing_account, '') AS opposing_account, "
        "  COALESCE(narration, '') AS narration, "
        "  CASE WHEN dr_cr = 'Dr' THEN amount ELSE 0.0 END AS debit, "
        "  CASE WHEN dr_cr = 'Cr' THEN amount ELSE 0.0 END AS credit "
        "FROM transactions "
        "WHERE voucher_date >= '%1' AND voucher_date <= '%2' "
        "ORDER BY voucher_date ASC, id ASC;"
    ).arg(fromStr, toStr);

    m_records = DatabaseManager::instance().executeQuery(sql);

    // Populate Table
    QString filterQuery = m_searchBox ? m_searchBox->text().trimmed().toLower() : "";
    QString selectedType = m_typeFilterCombo ? m_typeFilterCombo->currentText().toUpper() : "ALL";

    QVector<int> matchedIndices;
    matchedIndices.reserve(m_records.size());
    double totalDr = 0.0;
    double totalCr = 0.0;

    for (int i = 0; i < m_records.size(); ++i) {
        QVariantMap r = m_records.at(i).toMap();
        QString vType = r.value("voucher_type").toString();
        QString tType = r.value("trans_type").toString().toUpper();
        QString party = r.value("party_name").toString();
        QString opp = r.value("opposing_account").toString();
        QString narr = r.value("narration").toString();
        double dr = r.value("debit").toDouble();
        double cr = r.value("credit").toDouble();

        // Voucher Type Filter
        if (selectedType.contains("SALES") && !vType.contains("Sale", Qt::CaseInsensitive) && tType != "SL" && tType != "SALE") continue;
        if (selectedType.contains("PURCHASE") && !vType.contains("Purch", Qt::CaseInsensitive) && tType != "PU" && tType != "PURC") continue;
        if (selectedType.contains("JOURNAL") && !vType.contains("Journ", Qt::CaseInsensitive) && tType != "JRNL" && tType != "JV" && tType != "JRN") continue;
        if (selectedType.contains("BANK") && !vType.contains("Payment", Qt::CaseInsensitive) && !vType.contains("Receipt", Qt::CaseInsensitive) && !vType.contains("Bank", Qt::CaseInsensitive) && tType != "CHPT" && tType != "CHRT" && tType != "BKPT" && tType != "BKRT") continue;
        if (selectedType.contains("CASH") && !vType.contains("Cash", Qt::CaseInsensitive) && tType != "PYMT" && tType != "RCPT" && tType != "CSHP" && tType != "CSHR") continue;
        if (selectedType.contains("J-FORM") && !vType.contains("J-Form", Qt::CaseInsensitive) && tType != "JFRM") continue;
        if (selectedType.contains("I-FORM") && !vType.contains("I-Form", Qt::CaseInsensitive) && tType != "IFRM") continue;
        if (selectedType.contains("MILLING") && !vType.contains("Milling", Qt::CaseInsensitive) && tType != "MILL") continue;
        if (selectedType.contains("TDS") && !vType.contains("TDS", Qt::CaseInsensitive)) continue;

        // Search Query Filter
        if (!filterQuery.isEmpty()) {
            if (!party.toLower().contains(filterQuery) && !narr.toLower().contains(filterQuery) && !opp.toLower().contains(filterQuery)) {
                continue;
            }
        }

        matchedIndices.append(i);
        totalDr += dr;
        totalCr += cr;
    }

    m_table->setUpdatesEnabled(false);

    // State snapshot BEFORE the wipe: current voucher + scroll position, so a
    // dirty-gated reload (or a debounced search refresh) doesn't throw the
    // user back to the top. Search text / filters / dates live in controls
    // and already survive reload untouched.
    // NOTE: must run before setRowCount below — that call deletes all items.
    QString keepVoucherNo;
    int keepScroll = 0;
    if (m_table->currentRow() >= 0 && m_table->item(m_table->currentRow(), 1)) {
        keepVoucherNo = m_table->item(m_table->currentRow(), 1)->text();
    }
    if (m_table->verticalScrollBar()) keepScroll = m_table->verticalScrollBar()->value();

    m_table->setRowCount(matchedIndices.size());

    QColor typeColor("#2563EB");
    QColor partyColor("#0F172A");
    QColor descColor("#475569");
    QColor drColor("#16A34A");
    QColor crColor("#DC2626");

    for (int row = 0; row < matchedIndices.size(); ++row) {
        int idx = matchedIndices.at(row);
        QVariantMap r = m_records.at(idx).toMap();
        QString vType = r.value("voucher_type").toString();
        QString party = r.value("party_name").toString();
        QString opp = r.value("opposing_account").toString();
        QString narr = r.value("narration").toString();
        double dr = r.value("debit").toDouble();
        double cr = r.value("credit").toDouble();

        auto* dateItem = new QTableWidgetItem(
            FiscalYearHelper::formatDisplayDate(r.value("date").toString()));
        dateItem->setData(Qt::UserRole, QVariant::fromValue(r));
        m_table->setItem(row, 0, dateItem);

        auto* vchItem = new QTableWidgetItem(r.value("voucher_no").toString());
        m_table->setItem(row, 1, vchItem);

        auto* typeItem = new QTableWidgetItem(vType);
        typeItem->setForeground(typeColor);
        m_table->setItem(row, 2, typeItem);

        auto* partyItem = new QTableWidgetItem(party);
        partyItem->setForeground(partyColor);
        m_table->setItem(row, 3, partyItem);

        QString desc = opp.isEmpty() ? narr : (opp + (narr.isEmpty() ? "" : " - " + narr));
        auto* descItem = new QTableWidgetItem(desc);
        descItem->setForeground(descColor);
        m_table->setItem(row, 4, descItem);

        auto* drItem = new QTableWidgetItem(dr > 0.0 ? AccountingEngine::formatCurrency(dr) : "");
        drItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        if (dr > 0.0) drItem->setForeground(drColor);
        m_table->setItem(row, 5, drItem);

        auto* crItem = new QTableWidgetItem(cr > 0.0 ? AccountingEngine::formatCurrency(cr) : "");
        crItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        if (cr > 0.0) crItem->setForeground(crColor);
        m_table->setItem(row, 6, crItem);
    }

    m_table->setUpdatesEnabled(true);

    // State restore: re-select the same voucher if still visible, else hold
    // the scroll position (clamped) instead of jumping to the top.
    if (!keepVoucherNo.isEmpty()) {
        for (int row = 0; row < m_table->rowCount(); ++row) {
            auto* it = m_table->item(row, 1);
            if (it && it->text() == keepVoucherNo) {
                m_table->setCurrentCell(row, 0);
                break;
            }
        }
    }
    if (m_table->verticalScrollBar()) {
        m_table->verticalScrollBar()->setValue(
            qMin(keepScroll, m_table->verticalScrollBar()->maximum()));
    }

    m_rowCountLabel->setText(QString("Total Records: %1").arg(matchedIndices.size()));
    m_totalDrLabel->setText(QString("Total Debit: %1").arg(AccountingEngine::formatIndianCurrency(totalDr)));
    m_totalCrLabel->setText(QString("Total Credit: %1").arg(AccountingEngine::formatIndianCurrency(totalCr)));
    m_isDirty = false;
}

void DayBookWidget::focusTable() {
    if (m_table->rowCount() > 0) {
        m_table->setFocus();
        // Don't clobber: only seed row 0 when nothing is selected (first
        // visit / keyboard entry). Round-trips from the alteration page go
        // through restoreActiveViewFocus -> here, and wiping the restored
        // selection would defeat the reload state-save above.
        if (m_table->currentRow() < 0) {
            m_table->selectRow(0);
        }
    } else {
        m_searchBox->setFocus();
    }
}

// Alt+D: delete the focused voucher through the shared VoucherOps path,
// then invalidate every statement and reload in place (filters preserved).
void DayBookWidget::deleteFocusedRow() {
    const int row = m_table ? m_table->currentRow() : -1;
    if (row < 0 || row >= m_table->rowCount()) return;
    auto* itm = m_table->item(row, 0);
    if (!itm) return;
    const QVariantMap entry = itm->data(Qt::UserRole).toMap();
    if (entry.isEmpty()) return;

    QString error;
    if (VoucherOps::deleteEntry(this, entry, &error)) {
        emit entriesChanged();
        loadDayBookData(m_fromDateEdit->date(), m_toDateEdit->date(),
                        m_typeFilterCombo ? m_typeFilterCombo->currentText() : "ALL");
        focusTable();
    } else if (!error.isEmpty()) {
        CustomMessageBox::showCritical(this, "Delete Failed", error);
    }
}

void DayBookWidget::onRefreshClicked() {
    loadDayBookData(m_fromDateEdit->date(), m_toDateEdit->date(), m_typeFilterCombo->currentText());
}

void DayBookWidget::onDateChanged() {
    loadDayBookData(m_fromDateEdit->date(), m_toDateEdit->date(), m_typeFilterCombo->currentText());
}

void DayBookWidget::onRowDoubleClicked(int row, int /*column*/) {
    if (row < 0 || row >= m_table->rowCount()) return;
    auto* itm = m_table->item(row, 0);
    if (!itm) return;
    const QVariantMap entry = itm->data(Qt::UserRole).toMap();
    if (entry.isEmpty()) return;

    // Single canonical router (VoucherOps) — alter and Alt+D delete agree.
    const int targetView = VoucherOps::viewIndexForKind(VoucherOps::kindForEntry(entry));
    if (targetView > 0) {
        emit alterVoucherRequested(targetView, entry);
    }
}

void DayBookWidget::onExportPdfClicked() {
    if (!m_printExportCtrl) return;
    QString fDate = m_fromDateEdit ? m_fromDateEdit->date().toString("yyyy-MM-dd") : "";
    QString tDate = m_toDateEdit ? m_toDateEdit->date().toString("yyyy-MM-dd") : "";
    m_printExportCtrl->export_day_book_pdf(fDate, tDate);
}

void DayBookWidget::onExportOdfClicked() {
    if (!m_printExportCtrl) return;
    QString fDate = m_fromDateEdit ? m_fromDateEdit->date().toString("yyyy-MM-dd") : "";
    QString tDate = m_toDateEdit ? m_toDateEdit->date().toString("yyyy-MM-dd") : "";
    m_printExportCtrl->export_day_book_odf(fDate, tDate);
}

void DayBookWidget::onExportExcelClicked() {
    if (!m_printExportCtrl) return;
    QString fDate = m_fromDateEdit ? m_fromDateEdit->date().toString("yyyy-MM-dd") : "";
    QString tDate = m_toDateEdit ? m_toDateEdit->date().toString("yyyy-MM-dd") : "";
    m_printExportCtrl->export_day_book_excel(fDate, tDate);
}

void DayBookWidget::onPrintClicked() {
    if (!m_printExportCtrl) return;
    QString fDate = m_fromDateEdit ? m_fromDateEdit->date().toString("yyyy-MM-dd") : "";
    QString tDate = m_toDateEdit ? m_toDateEdit->date().toString("yyyy-MM-dd") : "";
    m_printExportCtrl->print_day_book(fDate, tDate);
}

void DayBookWidget::keyPressEvent(QKeyEvent* event) {
    if (event->key() == Qt::Key_Escape) {
        emit backRequested();
        event->accept();
    } else if (event->key() == Qt::Key_F2) {
        QDate f = m_fromDateEdit->date();
        QDate t = m_toDateEdit->date();
        if (VoucherDateDialog::selectDateRange(this, &f, &t, f, t)) {
            m_fromDateEdit->setDate(f);
            m_toDateEdit->setDate(t);
        }
        event->accept();
    } else if (event->key() == Qt::Key_F5) {
        onRefreshClicked();
        event->accept();
    } else if (event->modifiers() & Qt::AltModifier && event->key() == Qt::Key_P) {
        onPrintClicked();
        event->accept();
    } else if (event->modifiers() & Qt::ControlModifier && event->key() == Qt::Key_P) {
        onExportPdfClicked();
        event->accept();
    } else if (event->modifiers() & Qt::AltModifier && event->key() == Qt::Key_O) {
        onExportOdfClicked();
        event->accept();
    } else if (event->modifiers() & Qt::AltModifier && event->key() == Qt::Key_E) {
        onExportExcelClicked();
        event->accept();
    } else {
        QWidget::keyPressEvent(event);
    }
}

void DayBookWidget::showEvent(QShowEvent* event) {
    QWidget::showEvent(event);
    FiscalYearInfo activeFy = FiscalYearHelper::getActiveFiscalYear();
    QDate sDate = QDate::fromString(activeFy.startDate, "yyyy-MM-dd");
    QDate eDate = QDate::fromString(activeFy.endDate, "yyyy-MM-dd");
    if (!sDate.isValid()) sDate = QDate(QDate::currentDate().month() < 4 ? QDate::currentDate().year() - 1 : QDate::currentDate().year(), 4, 1);
    if (!eDate.isValid()) eDate = QDate::currentDate();

    if (!m_fromDateEdit->date().isValid() || !m_toDateEdit->date().isValid()) {
        QSignalBlocker b1(m_fromDateEdit);
        QSignalBlocker b2(m_toDateEdit);
        m_fromDateEdit->setDate(sDate);
        m_toDateEdit->setDate(eDate);
    }
    // Reload only when dirty: navigateToView already loads fresh FY dates on
    // entry, and every mutation marks statements dirty. Unconditional reload
    // here doubled every full populate (profiled hang is per-setItem header
    // measuring, so each redundant load costs seconds at scale).
    if (m_isDirty) {
        onRefreshClicked();
    }
}

} // namespace MahadevERP
