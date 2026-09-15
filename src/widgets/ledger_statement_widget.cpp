#include "ledger_statement_widget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFrame>
#include <QShortcut>
#include <QKeyEvent>
#include <QDate>
#include <cmath>

LedgerStatementWidget::LedgerStatementWidget(LedgerStatementController* controller,
                                             PrintExportController* printExportCtrl,
                                             QWidget* parent)
    : QWidget(parent)
    , m_controller(controller)
    , m_printExportCtrl(printExportCtrl)
{
    setupUi();

    if (m_controller) {
        connect(m_controller, &LedgerStatementController::statementTotalsChanged, this, &LedgerStatementWidget::onTotalsChanged);
        connect(m_controller, &LedgerStatementController::statementLoaded, this, &LedgerStatementWidget::onTotalsChanged);
    }
}

void LedgerStatementWidget::setupUi() {
    setStyleSheet("background-color: #F4F6F9;");

    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(16, 12, 16, 12);
    mainLayout->setSpacing(10);

    // ================= 1. TOP HEADER BAR =================
    QHBoxLayout* headerLayout = new QHBoxLayout();
    headerLayout->setSpacing(10);

    QVBoxLayout* titleLayout = new QVBoxLayout();
    titleLayout->setSpacing(2);
    QLabel* titleLabel = new QLabel("Account Ledger Statement (2-Column Dr / Cr)", this);
    titleLabel->setStyleSheet("font-size: 18px; font-weight: bold; color: #0F172A; font-family: 'Segoe UI', sans-serif;");
    QLabel* subtitleLabel = new QLabel("Side-by-side Credit (Cr) and Debit (Dr) accounting ledger with interactive reconciliation.", this);
    subtitleLabel->setStyleSheet("font-size: 11px; color: #64748B; font-family: 'Segoe UI', sans-serif;");
    titleLayout->addWidget(titleLabel);
    titleLayout->addWidget(subtitleLabel);
    headerLayout->addLayout(titleLayout);

    headerLayout->addStretch(1);

    m_alterBtn = new KbdBadgeButton("Alter Voucher", "Ctrl+E", QColor("#D97706"), QColor("#B45309"), this);
    connect(m_alterBtn, &QPushButton::clicked, this, &LedgerStatementWidget::openSelectedVoucher);
    headerLayout->addWidget(m_alterBtn);

    m_printBtn = new KbdBadgeButton("Print Statement", "Ctrl+P", QColor("#2563EB"), QColor("#1D4ED8"), this);
    connect(m_printBtn, &QPushButton::clicked, this, &LedgerStatementWidget::printStatement);
    headerLayout->addWidget(m_printBtn);

    m_pdfBtn = new KbdBadgeButton("Export PDF", "Alt+P", QColor("#059669"), QColor("#047857"), this);
    connect(m_pdfBtn, &QPushButton::clicked, this, &LedgerStatementWidget::exportPdf);
    headerLayout->addWidget(m_pdfBtn);

    m_csvBtn = new KbdBadgeButton("Export CSV", "", QColor("#FFFFFF"), QColor("#F1F5F9"), this);
    m_csvBtn->setStyleSheet("color: #334155; border: 1px solid #CBD5E1;");
    connect(m_csvBtn, &QPushButton::clicked, this, &LedgerStatementWidget::exportCsv);
    headerLayout->addWidget(m_csvBtn);

    m_backBtn = new KbdBadgeButton("Back to Dashboard", "Esc", QColor("#EF4444"), QColor("#DC2626"), this);
    connect(m_backBtn, &QPushButton::clicked, this, &LedgerStatementWidget::backRequested);
    headerLayout->addWidget(m_backBtn);

    mainLayout->addLayout(headerLayout);

    // ================= 2. SEARCH & FILTER BAR =================
    QHBoxLayout* filterLayout = new QHBoxLayout();
    filterLayout->setSpacing(8);

    m_searchBox = new AccountSearchBox(this);
    m_searchBox->setMinimumWidth(420);
    if (m_controller) {
        m_searchBox->setSearchFunction([this](const QString& q) {
            return m_controller->searchParties(q);
        });
    }
    connect(m_searchBox, &AccountSearchBox::partySelected, this, &LedgerStatementWidget::onPartySelected);
    filterLayout->addWidget(m_searchBox);

    m_fyBadge = new QLabel("FY 2026-27", this);
    m_fyBadge->setAlignment(Qt::AlignCenter);
    m_fyBadge->setFixedHeight(34);
    m_fyBadge->setStyleSheet("background-color: #FFFFFF; color: #1E293B; border: 1px solid #CBD5E1; border-radius: 6px; padding: 4px 12px; font-weight: bold; font-size: 12px;");
    filterLayout->addWidget(m_fyBadge);

    QLabel* fromLabel = new QLabel("From:", this);
    fromLabel->setStyleSheet("color: #475569; font-weight: bold; font-size: 12px;");
    filterLayout->addWidget(fromLabel);

    m_fromDateEdit = new AccountingDateEdit(this);
    m_fromDateEdit->setIsoDate("2026-04-01");
    filterLayout->addWidget(m_fromDateEdit);

    QLabel* toLabel = new QLabel("To:", this);
    toLabel->setStyleSheet("color: #475569; font-weight: bold; font-size: 12px;");
    filterLayout->addWidget(toLabel);

    m_toDateEdit = new AccountingDateEdit(this);
    m_toDateEdit->setIsoDate("2027-03-31");
    filterLayout->addWidget(m_toDateEdit);

    m_applyFilterBtn = new QPushButton("Filter Dates", this);
    m_applyFilterBtn->setFixedHeight(34);
    m_applyFilterBtn->setCursor(Qt::PointingHandCursor);
    m_applyFilterBtn->setStyleSheet("background-color: #2563EB; color: #FFFFFF; border-radius: 6px; padding: 4px 14px; font-weight: bold; font-size: 12px;");
    connect(m_applyFilterBtn, &QPushButton::clicked, this, &LedgerStatementWidget::onDateFilterApplied);
    filterLayout->addWidget(m_applyFilterBtn);

    filterLayout->addStretch(1);
    mainLayout->addLayout(filterLayout);

    // ================= 3. 2-COLUMN TABLES =================
    QHBoxLayout* tablesLayout = new QHBoxLayout();
    tablesLayout->setSpacing(12);

    // --- Left: Credit Side (Cr) ---
    QVBoxLayout* crSideLayout = new QVBoxLayout();
    crSideLayout->setSpacing(4);

    QFrame* crBanner = new QFrame(this);
    crBanner->setFixedHeight(32);
    crBanner->setStyleSheet("background-color: #DCFCE7; border: 1px solid #86EFAC; border-radius: 6px;");
    QHBoxLayout* crBannerLayout = new QHBoxLayout(crBanner);
    crBannerLayout->setContentsMargins(10, 0, 10, 0);
    m_crHeaderLabel = new QLabel("CREDIT SIDE (JAMA / Cr)", crBanner);
    m_crHeaderLabel->setStyleSheet("color: #15803D; font-weight: bold; font-size: 12px; border: none;");
    QLabel* crBadge = new QLabel("Takes / Payables", crBanner);
    crBadge->setStyleSheet("color: #166534; background-color: #BBF7D0; border-radius: 4px; padding: 2px 6px; font-size: 10px; font-weight: bold; border: none;");
    crBannerLayout->addWidget(m_crHeaderLabel);
    crBannerLayout->addStretch(1);
    crBannerLayout->addWidget(crBadge);
    crSideLayout->addWidget(crBanner);

    m_crTable = new LedgerTableView("Cr", m_controller ? m_controller->crModel() : nullptr, this);
    connect(m_crTable, &LedgerTableView::voucherActivated, this, &LedgerStatementWidget::onVoucherActivated);
    connect(m_crTable, &LedgerTableView::switchSideRequested, this, &LedgerStatementWidget::onSwitchSideRequested);
    crSideLayout->addWidget(m_crTable, 1);

    // Cr Footer
    QFrame* crFooter = new QFrame(this);
    crFooter->setFixedHeight(32);
    crFooter->setStyleSheet("background-color: #F8FAFC; border: 1px solid #E2E8F0; border-radius: 6px;");
    QHBoxLayout* crFooterLayout = new QHBoxLayout(crFooter);
    crFooterLayout->setContentsMargins(10, 0, 10, 0);
    m_crTotalLabel = new QLabel("Total Cr: ₹0.00 (0 entries)", crFooter);
    m_crTotalLabel->setStyleSheet("color: #15803D; font-weight: bold; font-size: 11px; border: none;");
    m_crCheckedLabel = new QLabel("Checked: ₹0.00", crFooter);
    m_crCheckedLabel->setStyleSheet("color: #475569; font-weight: bold; font-size: 11px; border: none;");
    crFooterLayout->addWidget(m_crTotalLabel);
    crFooterLayout->addStretch(1);
    crFooterLayout->addWidget(m_crCheckedLabel);
    crSideLayout->addWidget(crFooter);

    tablesLayout->addLayout(crSideLayout, 1);

    // --- Right: Debit Side (Dr) ---
    QVBoxLayout* drSideLayout = new QVBoxLayout();
    drSideLayout->setSpacing(4);

    QFrame* drBanner = new QFrame(this);
    drBanner->setFixedHeight(32);
    drBanner->setStyleSheet("background-color: #DBEAFE; border: 1px solid #93C5FD; border-radius: 6px;");
    QHBoxLayout* drBannerLayout = new QHBoxLayout(drBanner);
    drBannerLayout->setContentsMargins(10, 0, 10, 0);
    m_drHeaderLabel = new QLabel("DEBIT SIDE (NAAME / Dr)", drBanner);
    m_drHeaderLabel->setStyleSheet("color: #1D4ED8; font-weight: bold; font-size: 12px; border: none;");
    QLabel* drBadge = new QLabel("Gives / Receivables", drBanner);
    drBadge->setStyleSheet("color: #1E40AF; background-color: #BFDBFE; border-radius: 4px; padding: 2px 6px; font-size: 10px; font-weight: bold; border: none;");
    drBannerLayout->addWidget(m_drHeaderLabel);
    drBannerLayout->addStretch(1);
    drBannerLayout->addWidget(drBadge);
    drSideLayout->addWidget(drBanner);

    m_drTable = new LedgerTableView("Dr", m_controller ? m_controller->drModel() : nullptr, this);
    connect(m_drTable, &LedgerTableView::voucherActivated, this, &LedgerStatementWidget::onVoucherActivated);
    connect(m_drTable, &LedgerTableView::switchSideRequested, this, &LedgerStatementWidget::onSwitchSideRequested);
    drSideLayout->addWidget(m_drTable, 1);

    // Dr Footer
    QFrame* drFooter = new QFrame(this);
    drFooter->setFixedHeight(32);
    drFooter->setStyleSheet("background-color: #F8FAFC; border: 1px solid #E2E8F0; border-radius: 6px;");
    QHBoxLayout* drFooterLayout = new QHBoxLayout(drFooter);
    drFooterLayout->setContentsMargins(10, 0, 10, 0);
    m_drTotalLabel = new QLabel("Total Dr: ₹0.00 (0 entries)", drFooter);
    m_drTotalLabel->setStyleSheet("color: #1D4ED8; font-weight: bold; font-size: 11px; border: none;");
    m_drCheckedLabel = new QLabel("Checked: ₹0.00", drFooter);
    m_drCheckedLabel->setStyleSheet("color: #475569; font-weight: bold; font-size: 11px; border: none;");
    drFooterLayout->addWidget(m_drTotalLabel);
    drFooterLayout->addStretch(1);
    drFooterLayout->addWidget(m_drCheckedLabel);
    drSideLayout->addWidget(drFooter);

    tablesLayout->addLayout(drSideLayout, 1);

    mainLayout->addLayout(tablesLayout, 1);

    // ================= 4. BOTTOM NET RECONCILIATION SUMMARY =================
    QFrame* summaryBar = new QFrame(this);
    summaryBar->setFixedHeight(40);
    summaryBar->setStyleSheet("background-color: #FFFFFF; border: 1px solid #CBD5E1; border-radius: 8px;");
    QHBoxLayout* summaryLayout = new QHBoxLayout(summaryBar);
    summaryLayout->setContentsMargins(16, 0, 16, 0);

    m_netDiffLabel = new QLabel("Difference: ₹0.00", summaryBar);
    m_netDiffLabel->setStyleSheet("color: #334155; font-weight: bold; font-size: 12px; border: none;");

    m_netBalanceLabel = new QLabel("Closing Balance: ₹0.00", summaryBar);
    m_netBalanceLabel->setStyleSheet("color: #0F172A; font-weight: bold; font-size: 13px; border: none;");

    m_checkedDiffLabel = new QLabel("Checked Difference: ₹0.00", summaryBar);
    m_checkedDiffLabel->setStyleSheet("color: #64748B; font-weight: bold; font-size: 11px; border: none;");

    summaryLayout->addWidget(m_netDiffLabel);
    summaryLayout->addStretch(1);
    summaryLayout->addWidget(m_netBalanceLabel);
    summaryLayout->addStretch(1);
    summaryLayout->addWidget(m_checkedDiffLabel);

    mainLayout->addWidget(summaryBar);

    // Global Shortcuts
    new QShortcut(QKeySequence("Alt+S"), this, SLOT(focusSearch()));
    new QShortcut(QKeySequence("Alt+L"), this, SLOT(focusSearch()));
    new QShortcut(QKeySequence("Ctrl+E"), this, SLOT(openSelectedVoucher()));
    new QShortcut(QKeySequence("Ctrl+P"), this, SLOT(printStatement()));
    new QShortcut(QKeySequence("Alt+P"), this, SLOT(exportPdf()));
    new QShortcut(QKeySequence("Alt+F"), this, SLOT(onDateFilterApplied()));
}

void LedgerStatementWidget::loadParty(const QString& partyName, const QString& fromDate, const QString& toDate) {
    if (partyName.trimmed().isEmpty()) {
        resetSearch();
        return;
    }

    m_searchBox->setPartyName(partyName.trimmed());

    if (!fromDate.isEmpty()) {
        m_fromDateEdit->setIsoDate(fromDate);
    }
    if (!toDate.isEmpty()) {
        m_toDateEdit->setIsoDate(toDate);
    }

    if (m_controller) {
        m_controller->loadPartyStatement(partyName.trimmed(), m_fromDateEdit->isoDate(), m_toDateEdit->isoDate());
    }

    updateHeadersAndTotals();

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

    m_searchBox->setPartyName(partyName.trimmed());

    if (!fromDate.isEmpty()) {
        m_fromDateEdit->setIsoDate(fromDate);
    }
    if (!toDate.isEmpty()) {
        m_toDateEdit->setIsoDate(toDate);
    }

    if (m_controller) {
        m_controller->loadPartyStatement(partyName.trimmed(), m_fromDateEdit->isoDate(), m_toDateEdit->isoDate());
    }

    updateHeadersAndTotals();

    m_lastSide = side;
    m_lastIndex = rowIndex;

    if (side == "Cr" && m_controller && m_controller->crModel()->rowCount() > 0) {
        m_crTable->setFocus();
        m_crTable->selectRowIndex(qMin(qMax(0, rowIndex), m_controller->crModel()->rowCount() - 1));
    } else if (m_controller && m_controller->drModel()->rowCount() > 0) {
        m_drTable->setFocus();
        m_drTable->selectRowIndex(qMin(qMax(0, rowIndex), m_controller->drModel()->rowCount() - 1));
    } else if (m_controller && m_controller->crModel()->rowCount() > 0) {
        m_crTable->setFocus();
        m_crTable->selectRowIndex(0);
    }
}

void LedgerStatementWidget::resetSearch() {
    m_searchBox->setPartyName("");
    if (m_controller) {
        m_controller->loadPartyStatement("");
    }
    updateHeadersAndTotals();
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
        m_controller->applyDateFilter(m_fromDateEdit->isoDate(), m_toDateEdit->isoDate());
        updateHeadersAndTotals();
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

    double diff = std::abs(m_controller->drTotal() - m_controller->crTotal());
    m_netDiffLabel->setText(QString("Difference: ₹%1").arg(QString::number(diff, 'f', 2)));

    QString balType = m_controller->netBalanceType();
    QString balFmt = m_controller->netBalanceFmt();
    m_netBalanceLabel->setText(QString("Closing Balance: %1 (%2)").arg(balFmt).arg(balType));

    double checkedDiff = std::abs(m_controller->drSelectedTotal() - m_controller->crSelectedTotal());
    m_checkedDiffLabel->setText(QString("Checked Difference: ₹%1").arg(QString::number(checkedDiff, 'f', 2)));
}

void LedgerStatementWidget::openSelectedVoucher() {
    if (m_crTable->hasFocus()) {
        int row = m_crTable->selectedRowIndex();
        if (row >= 0 && m_controller) {
            onVoucherActivated(m_controller->crModel()->get(row));
            return;
        }
    }
    if (m_drTable->hasFocus() || m_drTable->selectedRowIndex() >= 0) {
        int row = m_drTable->selectedRowIndex();
        if (row >= 0 && m_controller) {
            onVoucherActivated(m_controller->drModel()->get(row));
            return;
        }
    }
    if (m_crTable->selectedRowIndex() >= 0 && m_controller) {
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
    QString rawType = entry.value("legacyType").toString();
    if (vType.isEmpty()) vType = entry.value("transType").toString();
    if (rawType.isEmpty()) rawType = entry.value("transType").toString();

    int targetViewIndex = -1;

    // 1. TDS Voucher (View 24)
    if (vType == "TDS" || rawType == "TDS" || part.contains("t.d.s.")) {
        targetViewIndex = 24;
    }
    // 2. Sales Voucher (View 14)
    else if (vType == "Sales" || rawType == "Sale" || rawType == "Sales" || vType == "Sale") {
        targetViewIndex = 14;
    }
    // 3. Purchase Voucher (View 15)
    else if (vType == "Purchase" || rawType == "Purc" || rawType == "Purchase" || vType == "Purc") {
        targetViewIndex = 15;
    }
    // 4. Payment / Receipt (View 16)
    else if (vType == "Payment" || vType == "Receipt" || rawType == "ChPt" || rawType == "ChRt" || rawType == "Pymt" || rawType == "Rcpt" || rawType == "Bank") {
        targetViewIndex = 16;
    }
    // 5. Journal Voucher (View 17)
    else if (vType == "Journal" || rawType == "Jrnl" || rawType == "Journal" || vType == "Jrnl") {
        targetViewIndex = 17;
    }
    // 6. Milling Voucher (View 18)
    else if (vType == "Milling" || rawType == "Mill" || rawType == "Prod" || rawType == "ML") {
        targetViewIndex = 18;
    }
    // 7. J-Form Voucher (View 23)
    else if (vType == "J-Form" || rawType == "JFrm" || rawType == "J-Form" || vType == "JFrm") {
        targetViewIndex = 23;
    }
    // 8. Debit / Credit Note (View 28)
    else if (vType == "Debit Note" || vType == "Credit Note" || rawType == "DbNt" || rawType == "CrNt" || rawType == "DN" || rawType == "CN") {
        targetViewIndex = 28;
    }

    if (targetViewIndex != -1) {
        emit alterVoucherRequested(targetViewIndex, entry);
    }
}

void LedgerStatementWidget::onSwitchSideRequested(const QString& targetSide) {
    if (targetSide == "Cr" && m_controller && m_controller->crModel()->rowCount() > 0) {
        m_crTable->setFocus();
        if (m_crTable->selectedRowIndex() < 0) {
            m_crTable->selectRowIndex(0);
        }
    } else if (targetSide == "Dr" && m_controller && m_controller->drModel()->rowCount() > 0) {
        m_drTable->setFocus();
        if (m_drTable->selectedRowIndex() < 0) {
            m_drTable->selectRowIndex(0);
        }
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
        emit backRequested();
        event->accept();
        return;
    }
    QWidget::keyPressEvent(event);
}

void LedgerStatementWidget::showEvent(QShowEvent* event) {
    QWidget::showEvent(event);
    if (m_searchBox->currentPartyName().isEmpty()) {
        m_searchBox->setFocus();
        m_searchBox->selectAll();
    }
}
