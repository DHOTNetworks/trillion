#include "ledger_statement_widget.h"
#include "engine/accounting_engine.h"
#include <QPainter>
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
    setAttribute(Qt::WA_StyledBackground, true);
    setAutoFillBackground(true);
    setupUi();

    if (m_controller) {
        connect(m_controller, &LedgerStatementController::statementTotalsChanged, this, &LedgerStatementWidget::onTotalsChanged);
        connect(m_controller, &LedgerStatementController::statementLoaded, this, &LedgerStatementWidget::onTotalsChanged);
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
    QLabel* titleLabel = new QLabel("Account Ledger Statement (2-Column Dr / Cr)", headerCard);
    titleLabel->setStyleSheet("font-size: 16px; font-weight: bold; color: #0F172A; font-family: 'Segoe UI', sans-serif; border: none; background: transparent;");
    QLabel* subtitleLabel = new QLabel("Side-by-side Credit (Cr) and Debit (Dr) accounting ledger with interactive reconciliation.", headerCard);
    subtitleLabel->setStyleSheet("font-size: 11px; color: #64748B; font-family: 'Segoe UI', sans-serif; border: none; background: transparent;");
    titleLayout->addWidget(titleLabel);
    titleLayout->addWidget(subtitleLabel);
    headerLayout->addLayout(titleLayout);

    headerLayout->addStretch(1);

    m_alterBtn = new KbdBadgeButton("Alter Voucher", "Ctrl+E", QColor("#D97706"), QColor("#B45309"), QColor("#FFFFFF"), QColor("#D97706"), headerCard);
    connect(m_alterBtn, &QPushButton::clicked, this, &LedgerStatementWidget::openSelectedVoucher);
    headerLayout->addWidget(m_alterBtn);

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
    m_searchBox->setMinimumWidth(400);
    if (m_controller) {
        m_searchBox->setSearchFunction([this](const QString& q) {
            return m_controller->searchParties(q);
        });
    }
    connect(m_searchBox, &AccountSearchBox::partySelected, this, &LedgerStatementWidget::onPartySelected);
    filterLayout->addWidget(m_searchBox);

    m_fyBadge = new QLabel("FY 2026-27", filterCard);
    m_fyBadge->setAlignment(Qt::AlignCenter);
    m_fyBadge->setFixedHeight(34);
    m_fyBadge->setStyleSheet("background-color: #F8FAFC; color: #334155; border: 1px solid #CBD5E1; border-radius: 6px; padding: 4px 12px; font-weight: bold; font-size: 11px;");
    filterLayout->addWidget(m_fyBadge);

    QLabel* fromLabel = new QLabel("From:", filterCard);
    fromLabel->setStyleSheet("color: #475569; font-weight: bold; font-size: 11px; border: none; background: transparent;");
    filterLayout->addWidget(fromLabel);

    m_fromDateEdit = new AccountingDateEdit(filterCard);
    m_fromDateEdit->setIsoDate("2026-04-01");
    m_fromDateEdit->setFixedWidth(110);
    filterLayout->addWidget(m_fromDateEdit);

    QLabel* toLabel = new QLabel("To:", filterCard);
    toLabel->setStyleSheet("color: #475569; font-weight: bold; font-size: 11px; border: none; background: transparent;");
    filterLayout->addWidget(toLabel);

    m_toDateEdit = new AccountingDateEdit(filterCard);
    m_toDateEdit->setIsoDate("2027-03-31");
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

    // ================= 3. 2-COLUMN TABLES =================
    QHBoxLayout* tablesLayout = new QHBoxLayout();
    tablesLayout->setSpacing(10);

    // --- Left: Credit Side (Cr) ---
    QVBoxLayout* crSideLayout = new QVBoxLayout();
    crSideLayout->setSpacing(4);

    QFrame* crBanner = new QFrame(this);
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

    m_crTable = new LedgerTableView("Cr", m_controller ? m_controller->crModel() : nullptr, this);
    connect(m_crTable, &LedgerTableView::voucherActivated, this, &LedgerStatementWidget::onVoucherActivated);
    connect(m_crTable, &LedgerTableView::switchSideRequested, this, &LedgerStatementWidget::onSwitchSideRequested);
    crSideLayout->addWidget(m_crTable, 1);

    // Cr Footer
    QFrame* crFooter = new QFrame(this);
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

    QFrame* drBanner = new QFrame(this);
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

    m_drTable = new LedgerTableView("Dr", m_controller ? m_controller->drModel() : nullptr, this);
    connect(m_drTable, &LedgerTableView::voucherActivated, this, &LedgerStatementWidget::onVoucherActivated);
    connect(m_drTable, &LedgerTableView::switchSideRequested, this, &LedgerStatementWidget::onSwitchSideRequested);
    drSideLayout->addWidget(m_drTable, 1);

    // Dr Footer
    QFrame* drFooter = new QFrame(this);
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

    mainLayout->addLayout(tablesLayout, 1);

    // ================= 4. BOTTOM NET RECONCILIATION SUMMARY =================
    QFrame* summaryBar = new QFrame(this);
    summaryBar->setFixedHeight(44);
    summaryBar->setStyleSheet("background-color: #FFFFFF; border: 1px solid #CBD5E1; border-radius: 8px;");
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
    m_netBalanceLabel->setStyleSheet(QString("color: %1; font-weight: bold; font-size: 14px; border: none; background: transparent; font-family: 'Segoe UI', sans-serif;").arg(balColor));

    double checkedDiff = std::abs(m_controller->drSelectedTotal() - m_controller->crSelectedTotal());
    m_checkedDiffLabel->setText(QString("Checked Difference: %1").arg(AccountingEngine::formatIndianCurrency(checkedDiff, true)));
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
        int currentDrRow = m_drTable->selectedRowIndex();
        int targetRow = (currentDrRow >= 0) ? qMin(currentDrRow, m_controller->crModel()->rowCount() - 1) : 0;
        m_crTable->setFocus();
        m_crTable->selectRowIndex(targetRow);
    } else if (targetSide == "Dr" && m_controller && m_controller->drModel()->rowCount() > 0) {
        int currentCrRow = m_crTable->selectedRowIndex();
        int targetRow = (currentCrRow >= 0) ? qMin(currentCrRow, m_controller->drModel()->rowCount() - 1) : 0;
        m_drTable->setFocus();
        m_drTable->selectRowIndex(targetRow);
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

void LedgerStatementWidget::paintEvent(QPaintEvent* event) {
    QPainter painter(this);
    painter.fillRect(rect(), QColor("#F8FAFC"));
    QWidget::paintEvent(event);
}

