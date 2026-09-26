#include "cash_bank_flow_widget.h"
#include "accounting_period_dialog.h"
#include "custom_dialogs.h"
#include "../engine/accounting_engine.h"
#include "../engine/fiscal_year_helper.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QKeyEvent>
#include <QFileDialog>
#include <QFile>
#include <QTextStream>
#include <QDesktopServices>
#include <QUrl>
#include <QPainter>
#include <QPrinter>
#include <QTextDocument>

namespace MahadevERP {

CashBankFlowWidget::CashBankFlowWidget(PrintExportController* printExportCtrl, QWidget* parent)
    : QWidget(parent)
    , m_printExportCtrl(printExportCtrl)
{
    setupUi();
    connect(&m_controller, &CashBankFlowController::dataChanged, this, &CashBankFlowWidget::updateTableData);
    refreshData();
}

void CashBankFlowWidget::setupUi() {
    setAttribute(Qt::WA_StyledBackground, true);
    setStyleSheet("CashBankFlowWidget { background-color: #F8FAFC; } QLabel { border: none; background: transparent; }");

    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(16, 12, 16, 12);
    mainLayout->setSpacing(10);

    // ========================================================================
    // TIER 1: HEADER BAR CARD
    // ========================================================================
    auto* headerCard = new QFrame(this);
    headerCard->setFixedHeight(58);
    headerCard->setStyleSheet("background-color: #FFFFFF; border: 1px solid #E2E8F0; border-radius: 8px;");
    auto* headerLayout = new QHBoxLayout(headerCard);
    headerLayout->setContentsMargins(16, 0, 16, 0);
    headerLayout->setSpacing(10);

    auto* titleBox = new QVBoxLayout();
    titleBox->setSpacing(1);
    m_titleLabel = new QLabel("CASH FLOW STATEMENT", headerCard);
    m_titleLabel->setStyleSheet("color: #0F172A; font-size: 15px; font-weight: 800; letter-spacing: 0.5px; border: none; background: transparent;");
    titleBox->addWidget(m_titleLabel);

    m_subTitleLabel = new QLabel("Cash & Bank Ledger Flow • Double-Entry Verification", headerCard);
    m_subTitleLabel->setStyleSheet("color: #64748B; font-size: 11px; border: none; background: transparent;");
    titleBox->addWidget(m_subTitleLabel);
    headerLayout->addLayout(titleBox);

    headerLayout->addStretch(1);

    // Mode Switcher Buttons
    m_cashModeBtn = new QPushButton("Alt+A: Cash Flow", headerCard);
    m_cashModeBtn->setFixedHeight(32);
    m_cashModeBtn->setCursor(Qt::PointingHandCursor);
    m_cashModeBtn->setStyleSheet(
        "QPushButton { background-color: #EFF6FF; border: 1.5px solid #3B82F6; border-radius: 6px; padding: 0px 12px; font-weight: 700; color: #1D4ED8; font-size: 11.5px; }"
        "QPushButton:hover { background-color: #DBEAFE; }"
    );
    connect(m_cashModeBtn, &QPushButton::clicked, this, [this]() { setStatementMode(FlowStatementType::CashFlow); });
    headerLayout->addWidget(m_cashModeBtn);

    m_bankModeBtn = new QPushButton("Alt+N: Bank Flow", headerCard);
    m_bankModeBtn->setFixedHeight(32);
    m_bankModeBtn->setCursor(Qt::PointingHandCursor);
    m_bankModeBtn->setStyleSheet(
        "QPushButton { background-color: #F8FAFC; border: 1px solid #CBD5E1; border-radius: 6px; padding: 0px 12px; font-weight: 700; color: #475569; font-size: 11.5px; }"
        "QPushButton:hover { background-color: #F1F5F9; }"
    );
    connect(m_bankModeBtn, &QPushButton::clicked, this, [this]() { setStatementMode(FlowStatementType::BankFlow); });
    headerLayout->addWidget(m_bankModeBtn);

    m_jointModeBtn = new QPushButton("Alt+J: Joint Flow", headerCard);
    m_jointModeBtn->setFixedHeight(32);
    m_jointModeBtn->setCursor(Qt::PointingHandCursor);
    m_jointModeBtn->setStyleSheet(
        "QPushButton { background-color: #F8FAFC; border: 1px solid #CBD5E1; border-radius: 6px; padding: 0px 12px; font-weight: 700; color: #475569; font-size: 11.5px; }"
        "QPushButton:hover { background-color: #F1F5F9; }"
    );
    connect(m_jointModeBtn, &QPushButton::clicked, this, [this]() { setStatementMode(FlowStatementType::JointFlow); });
    headerLayout->addWidget(m_jointModeBtn);

    // Action Buttons (PDF, CSV, Back)
    m_pdfBtn = new QPushButton("PDF (Alt+P)", headerCard);
    m_pdfBtn->setFixedHeight(32);
    m_pdfBtn->setCursor(Qt::PointingHandCursor);
    m_pdfBtn->setStyleSheet(
        "QPushButton { background-color: #FEF2F2; border: 1px solid #FCA5A5; border-radius: 6px; padding: 0px 12px; font-weight: 700; color: #DC2626; font-size: 11.5px; }"
        "QPushButton:hover { background-color: #FEE2E2; }"
    );
    connect(m_pdfBtn, &QPushButton::clicked, this, &CashBankFlowWidget::onExportPdf);
    headerLayout->addWidget(m_pdfBtn);

    m_csvBtn = new QPushButton("CSV (Alt+E)", headerCard);
    m_csvBtn->setFixedHeight(32);
    m_csvBtn->setCursor(Qt::PointingHandCursor);
    m_csvBtn->setStyleSheet(
        "QPushButton { background-color: #F0FDF4; border: 1px solid #86EFAC; border-radius: 6px; padding: 0px 12px; font-weight: 700; color: #16A34A; font-size: 11.5px; }"
        "QPushButton:hover { background-color: #DCFCE7; }"
    );
    connect(m_csvBtn, &QPushButton::clicked, this, &CashBankFlowWidget::onExportCsv);
    headerLayout->addWidget(m_csvBtn);

    m_backBtn = new QPushButton("← Back (Esc)", headerCard);
    m_backBtn->setFixedHeight(32);
    m_backBtn->setCursor(Qt::PointingHandCursor);
    m_backBtn->setStyleSheet(
        "QPushButton { background-color: #F1F5F9; border: 1px solid #CBD5E1; border-radius: 6px; padding: 0px 14px; font-weight: 700; color: #475569; font-size: 11.5px; }"
        "QPushButton:hover { background-color: #E2E8F0; }"
    );
    connect(m_backBtn, &QPushButton::clicked, this, &CashBankFlowWidget::backRequested);
    headerLayout->addWidget(m_backBtn);

    mainLayout->addWidget(headerCard);

    // ========================================================================
    // TIER 2: FILTER & CONTROL BAR CARD
    // ========================================================================
    auto* filterCard = new QFrame(this);
    filterCard->setFixedHeight(50);
    filterCard->setStyleSheet("background-color: #FFFFFF; border: 1px solid #E2E8F0; border-radius: 8px;");
    auto* filterLayout = new QHBoxLayout(filterCard);
    filterLayout->setContentsMargins(16, 0, 16, 0);
    filterLayout->setSpacing(12);

    auto* fromLabel = new QLabel("From:", filterCard);
    fromLabel->setStyleSheet("color: #475569; font-size: 11.5px; font-weight: 700;");
    filterLayout->addWidget(fromLabel);

    m_fromDateEdit = new AccountingDateEdit(filterCard);
    m_fromDateEdit->setFixedSize(110, 30);
    m_fromDateEdit->setStyleSheet(
        "QLineEdit { background-color: #FFFFFF; color: #0F172A; border: 1.5px solid #CBD5E1; "
        "border-radius: 6px; padding: 2px 6px; font-weight: 700; font-size: 12px; }"
        "QLineEdit:focus { border: 2px solid #2563EB; background-color: #FFFFF0; }"
    );
    connect(m_fromDateEdit, &AccountingDateEdit::dateChanged, this, &CashBankFlowWidget::onDateChanged);
    filterLayout->addWidget(m_fromDateEdit);

    auto* toLabel = new QLabel("To:", filterCard);
    toLabel->setStyleSheet("color: #475569; font-size: 11.5px; font-weight: 700;");
    filterLayout->addWidget(toLabel);

    m_toDateEdit = new AccountingDateEdit(filterCard);
    m_toDateEdit->setFixedSize(110, 30);
    m_toDateEdit->setStyleSheet(
        "QLineEdit { background-color: #FFFFFF; color: #0F172A; border: 1.5px solid #CBD5E1; "
        "border-radius: 6px; padding: 2px 6px; font-weight: 700; font-size: 12px; }"
        "QLineEdit:focus { border: 2px solid #2563EB; background-color: #FFFFF0; }"
    );
    connect(m_toDateEdit, &AccountingDateEdit::dateChanged, this, &CashBankFlowWidget::onDateChanged);
    filterLayout->addWidget(m_toDateEdit);

    m_periodBtn = new QPushButton("Period (F2)", filterCard);
    m_periodBtn->setFixedHeight(30);
    m_periodBtn->setCursor(Qt::PointingHandCursor);
    m_periodBtn->setStyleSheet(
        "QPushButton { background-color: #F8FAFC; border: 1px solid #CBD5E1; border-radius: 6px; padding: 0px 10px; font-weight: 700; color: #334155; font-size: 11px; }"
        "QPushButton:hover { background-color: #F1F5F9; }"
    );
    connect(m_periodBtn, &QPushButton::clicked, this, &CashBankFlowWidget::onAccountingPeriod);
    filterLayout->addWidget(m_periodBtn);

    filterLayout->addSpacing(8);

    auto* searchLabel = new QLabel("Search (Ctrl+F):", filterCard);
    searchLabel->setStyleSheet("color: #475569; font-size: 11.5px; font-weight: 700;");
    filterLayout->addWidget(searchLabel);

    m_searchBox = new QLineEdit(filterCard);
    m_searchBox->setPlaceholderText("Filter ledger name...");
    m_searchBox->setFixedHeight(30);
    m_searchBox->setStyleSheet(
        "QLineEdit { background-color: #FFFFFF; color: #0F172A; border: 1.5px solid #CBD5E1; "
        "border-radius: 6px; padding: 2px 10px; font-weight: 600; font-size: 12px; }"
        "QLineEdit:focus { border: 2px solid #2563EB; background-color: #FFFFF0; }"
    );
    connect(m_searchBox, &QLineEdit::textChanged, this, &CashBankFlowWidget::onSearchTextChanged);
    filterLayout->addWidget(m_searchBox, 1);

    m_fyBadge = new QLabel(FiscalYearHelper::getActiveFiscalYear().name, filterCard);
    m_fyBadge->setStyleSheet(
        "background-color: #F0F9FF; color: #0284C7; border: 1.5px solid #BAE6FD; "
        "padding: 4px 10px; border-radius: 6px; font-weight: 700; font-size: 11px;"
    );
    filterLayout->addWidget(m_fyBadge);

    mainLayout->addWidget(filterCard);

    // ========================================================================
    // TIER 3: HIGH-PERFORMANCE DATA SURFACE
    // ========================================================================
    m_table = new QTableWidget(this);
    m_table->setColumnCount(5);
    m_table->setHorizontalHeaderLabels({"Ledger Name", "Op.Balance", "Receipts", "Payments", "Cl.Balance"});
    m_table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Fixed);
    m_table->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Fixed);
    m_table->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Fixed);
    m_table->horizontalHeader()->setSectionResizeMode(4, QHeaderView::Fixed);
    m_table->setColumnWidth(1, 140);
    m_table->setColumnWidth(2, 150);
    m_table->setColumnWidth(3, 150);
    m_table->setColumnWidth(4, 150);

    m_table->setAlternatingRowColors(true);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->verticalHeader()->setVisible(false);
    m_table->verticalHeader()->setDefaultSectionSize(28);

    m_table->setStyleSheet(
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
        "  padding: 8px 8px;"
        "  border: none;"
        "  border-right: 1px solid #334155;"
        "}"
    );

    connect(m_table, &QTableWidget::cellDoubleClicked, this, &CashBankFlowWidget::onRowDoubleClicked);
    mainLayout->addWidget(m_table, 1);

    // ========================================================================
    // TIER 4: SUMMARY METRICS FOOTER CARDS
    // ========================================================================
    auto* footerContainer = new QHBoxLayout();
    footerContainer->setSpacing(12);

    auto createFooterCard = [this, footerContainer](const QString& title, QLabel** outVal, const QString& accentColor, int stretch = 1) {
        auto* card = new QFrame(this);
        card->setFixedHeight(68);
        card->setStyleSheet(QString(
            "QFrame { background-color: #FFFFFF; border: 1px solid #E2E8F0; border-left: 4px solid %1; border-radius: 8px; }"
            "QLabel { border: none; background: transparent; }"
        ).arg(accentColor));

        auto* layout = new QVBoxLayout(card);
        layout->setContentsMargins(12, 6, 12, 6);
        layout->setSpacing(2);

        auto* tLabel = new QLabel(title, card);
        tLabel->setObjectName("cardTitle");
        tLabel->setStyleSheet("font-size: 10px; font-weight: 800; color: #64748B; letter-spacing: 0.5px; text-transform: uppercase;");
        layout->addWidget(tLabel);

        *outVal = new QLabel("₹ 0.00", card);
        (*outVal)->setStyleSheet(QString("font-size: 16px; font-weight: 800; color: %1; font-family: 'Segoe UI', -apple-system, Roboto, sans-serif;").arg(accentColor));
        layout->addWidget(*outVal);

        footerContainer->addWidget(card, stretch);
        return card;
    };

    createFooterCard("Opening Balance", &m_opBalValLabel, "#475569", 1);
    createFooterCard("Total Receipts", &m_receiptsValLabel, "#16A34A", 1);
    createFooterCard("Total Payments", &m_paymentsValLabel, "#DC2626", 1);
    auto* clCard = createFooterCard("Closing Cash / Bank Balance", &m_clBalValLabel, "#16A34A", 2);
    m_clBalTitleLabel = clCard->findChild<QLabel*>("cardTitle");

    mainLayout->addLayout(footerContainer);

    // Initialize Dates from Fiscal Year
    FiscalYearInfo fy = FiscalYearHelper::getActiveFiscalYear();
    m_fromDateEdit->setDate(QDate::fromString(fy.startDate, "yyyy-MM-dd"));
    m_toDateEdit->setDate(QDate::fromString(fy.endDate, "yyyy-MM-dd"));
}

void CashBankFlowWidget::setStatementMode(FlowStatementType mode) {
    m_controller.setStatementType(mode);
    updateHeaderLabels();

    // Style active mode button
    auto applyBtnStyle = [](QPushButton* btn, bool active) {
        if (active) {
            btn->setStyleSheet(
                "QPushButton { background-color: #EFF6FF; border: 1.5px solid #3B82F6; border-radius: 6px; padding: 0px 12px; font-weight: 700; color: #1D4ED8; font-size: 11.5px; }"
            );
        } else {
            btn->setStyleSheet(
                "QPushButton { background-color: #F8FAFC; border: 1px solid #CBD5E1; border-radius: 6px; padding: 0px 12px; font-weight: 700; color: #475569; font-size: 11.5px; }"
                "QPushButton:hover { background-color: #F1F5F9; }"
            );
        }
    };

    applyBtnStyle(m_cashModeBtn, mode == FlowStatementType::CashFlow);
    applyBtnStyle(m_bankModeBtn, mode == FlowStatementType::BankFlow);
    applyBtnStyle(m_jointModeBtn, mode == FlowStatementType::JointFlow);
}

void CashBankFlowWidget::updateHeaderLabels() {
    QString title = m_controller.statementTitle();
    m_titleLabel->setText(title.toUpper());

    QString sDate = FiscalYearHelper::formatDisplayDate(m_controller.fromDate());
    QString eDate = FiscalYearHelper::formatDisplayDate(m_controller.toDate());
    m_subTitleLabel->setText(QString("(%1 To %2) • Real-time double-entry flow audit").arg(sDate, eDate));

    if (m_clBalTitleLabel) {
        if (m_controller.statementType() == FlowStatementType::CashFlow) {
            m_clBalTitleLabel->setText("CLOSING CASH BALANCE");
        } else if (m_controller.statementType() == FlowStatementType::BankFlow) {
            m_clBalTitleLabel->setText("CLOSING BANK BALANCE");
        } else {
            m_clBalTitleLabel->setText("CLOSING CASH & BANK BALANCE");
        }
    }
}

void CashBankFlowWidget::refreshData() {
    QString fDate = m_fromDateEdit ? m_fromDateEdit->isoDate() : "";
    QString tDate = m_toDateEdit ? m_toDateEdit->isoDate() : "";
    QString q = m_searchBox ? m_searchBox->text().trimmed() : "";
    m_controller.loadData(fDate, tDate, q);
}

void CashBankFlowWidget::setDateRange(const QString& fromDate, const QString& toDate) {
    if (m_fromDateEdit) m_fromDateEdit->setDate(QDate::fromString(fromDate, "yyyy-MM-dd"));
    if (m_toDateEdit) m_toDateEdit->setDate(QDate::fromString(toDate, "yyyy-MM-dd"));
    refreshData();
}

void CashBankFlowWidget::onDateChanged() {
    refreshData();
}

void CashBankFlowWidget::onSearchTextChanged(const QString& text) {
    Q_UNUSED(text);
    refreshData();
}

void CashBankFlowWidget::onAccountingPeriod() {
    QString fIso, tIso, fyLabel;
    bool applied = AccountingPeriodDialog::selectAndApplyGlobalPeriod(this, &fIso, &tIso, &fyLabel);
    if (applied) {
        if (m_fromDateEdit) m_fromDateEdit->setDate(QDate::fromString(fIso, "yyyy-MM-dd"));
        if (m_toDateEdit) m_toDateEdit->setDate(QDate::fromString(tIso, "yyyy-MM-dd"));
        if (m_fyBadge) m_fyBadge->setText(fyLabel);
        refreshData();
    }
}

void CashBankFlowWidget::updateTableData() {
    m_table->setRowCount(0);
    const auto& partyRows = m_controller.partyRows();
    const auto& bottomRows = m_controller.bankCashRows();
    const auto& summary = m_controller.summary();

    int row = 0;

    // 1. Party / Ledger Rows
    for (const auto& r : partyRows) {
        m_table->insertRow(row);

        auto* nameItem = new QTableWidgetItem(r.ledgerName);
        nameItem->setForeground(QBrush(QColor("#0F172A")));
        nameItem->setData(Qt::UserRole, r.ledgerName);
        nameItem->setData(Qt::UserRole + 1, r.partyId);
        m_table->setItem(row, 0, nameItem);

        auto* opItem = new QTableWidgetItem(r.opBalance != 0.0 ? QString::number(r.opBalance, 'f', 2) : "");
        opItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        opItem->setForeground(QBrush(QColor("#0F172A")));
        m_table->setItem(row, 1, opItem);

        auto* recItem = new QTableWidgetItem(r.receipts > 0.0 ? AccountingEngine::formatCurrency(r.receipts) : "--");
        recItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        recItem->setForeground(QBrush(r.receipts > 0.0 ? QColor("#16A34A") : QColor("#94A3B8")));
        m_table->setItem(row, 2, recItem);

        auto* payItem = new QTableWidgetItem(r.payments > 0.0 ? AccountingEngine::formatCurrency(r.payments) : "--");
        payItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        payItem->setForeground(QBrush(r.payments > 0.0 ? QColor("#0F172A") : QColor("#94A3B8")));
        m_table->setItem(row, 3, payItem);

        auto* clItem = new QTableWidgetItem(r.clBalance != 0.0 ? QString::number(r.clBalance, 'f', 2) : "");
        clItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        clItem->setForeground(QBrush(QColor("#0F172A")));
        m_table->setItem(row, 4, clItem);

        row++;
    }

    // 2. Bottom Cash / Bank Account Rows (e.g. Cash -> 10,35,864.00, Yes Bank -> 11,079.00)
    for (const auto& b : bottomRows) {
        m_table->insertRow(row);

        auto* nameItem = new QTableWidgetItem(b.ledgerName);
        nameItem->setForeground(QBrush(QColor("#2563EB")));
        nameItem->setFont(QFont(m_table->font().family(), 10, QFont::Bold));
        nameItem->setData(Qt::UserRole, b.ledgerName);
        nameItem->setData(Qt::UserRole + 1, b.partyId);
        m_table->setItem(row, 0, nameItem);

        auto* opItem = new QTableWidgetItem("");
        m_table->setItem(row, 1, opItem);

        auto* recItem = new QTableWidgetItem("");
        m_table->setItem(row, 2, recItem);

        auto* payItem = new QTableWidgetItem("");
        m_table->setItem(row, 3, payItem);

        auto* clItem = new QTableWidgetItem(AccountingEngine::formatCurrency(b.clBalance));
        clItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        clItem->setForeground(QBrush(QColor("#2563EB")));
        clItem->setFont(QFont(m_table->font().family(), 10, QFont::Bold));
        m_table->setItem(row, 4, clItem);

        for (int c = 0; c < 5; ++c) {
            if (m_table->item(row, c)) {
                m_table->item(row, c)->setBackground(QBrush(QColor("#F0F9FF")));
            }
        }
        row++;
    }

    // 3. Grand Total Row
    m_table->insertRow(row);

    auto* totName = new QTableWidgetItem("Grand Total");
    totName->setForeground(QBrush(QColor("#0F172A")));
    totName->setFont(QFont(m_table->font().family(), 10, QFont::Bold));
    m_table->setItem(row, 0, totName);

    auto* totOp = new QTableWidgetItem(QString::number(summary.totalOpBalance, 'f', 2));
    totOp->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
    totOp->setForeground(QBrush(QColor("#0F172A")));
    totOp->setFont(QFont(m_table->font().family(), 10, QFont::Bold));
    m_table->setItem(row, 1, totOp);

    auto* totRec = new QTableWidgetItem(AccountingEngine::formatCurrency(summary.totalReceipts));
    totRec->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
    totRec->setForeground(QBrush(QColor("#16A34A")));
    totRec->setFont(QFont(m_table->font().family(), 10, QFont::Bold));
    m_table->setItem(row, 2, totRec);

    auto* totPay = new QTableWidgetItem(AccountingEngine::formatCurrency(summary.totalPayments));
    totPay->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
    totPay->setForeground(QBrush(QColor("#0F172A")));
    totPay->setFont(QFont(m_table->font().family(), 10, QFont::Bold));
    m_table->setItem(row, 3, totPay);

    auto* totCl = new QTableWidgetItem(AccountingEngine::formatCurrency(summary.totalClBalance));
    totCl->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
    totCl->setForeground(QBrush(QColor("#16A34A")));
    totCl->setFont(QFont(m_table->font().family(), 10, QFont::Bold));
    m_table->setItem(row, 4, totCl);

    for (int c = 0; c < 5; ++c) {
        if (m_table->item(row, c)) {
            m_table->item(row, c)->setBackground(QBrush(QColor("#F1F5F9")));
        }
    }

    updateHeaderLabels();
    updateSummaryMetrics();
}

void CashBankFlowWidget::updateSummaryMetrics() {
    const auto& s = m_controller.summary();
    if (m_opBalValLabel) m_opBalValLabel->setText(AccountingEngine::formatIndianCurrency(s.totalOpBalance));
    if (m_receiptsValLabel) m_receiptsValLabel->setText(AccountingEngine::formatIndianCurrency(s.totalReceipts));
    if (m_paymentsValLabel) m_paymentsValLabel->setText(AccountingEngine::formatIndianCurrency(s.totalPayments));
    if (m_clBalValLabel) m_clBalValLabel->setText(AccountingEngine::formatIndianCurrency(s.totalClBalance));
}

void CashBankFlowWidget::onRowDoubleClicked(int row, int /*col*/) {
    if (row < 0 || row >= m_table->rowCount()) return;
    auto* item = m_table->item(row, 0);
    if (!item) return;

    QString partyName = item->data(Qt::UserRole).toString();
    if (partyName.isEmpty() || partyName == "Grand Total") return;

    QString fromDate = m_fromDateEdit ? m_fromDateEdit->isoDate() : "";
    QString toDate = m_toDateEdit ? m_toDateEdit->isoDate() : "";

    emit partyStatementRequested(partyName, fromDate, toDate);
}

void CashBankFlowWidget::onExportCsv() {
    QString defName = QString("%1_%2.csv").arg(m_controller.statementTitle().replace(" ", "_"), QDate::currentDate().toString("yyyyMMdd"));
    QString path = QFileDialog::getSaveFileName(this, "Export Statement to CSV", defName, "CSV Files (*.csv)");
    if (path.isEmpty()) return;

    QFile file(path);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&file);
        out << m_controller.generateCsvReport();
        file.close();
        CustomMessageBox::information(this, "Export Successful", QString("Statement exported successfully to:\n%1").arg(path));
    }
}

void CashBankFlowWidget::onExportPdf() {
    QString defName = QString("%1_%2.pdf").arg(m_controller.statementTitle().replace(" ", "_"), QDate::currentDate().toString("yyyyMMdd"));
    QString path = QFileDialog::getSaveFileName(this, "Export Statement to PDF", defName, "PDF Files (*.pdf)");
    if (path.isEmpty()) return;

    QPrinter printer(QPrinter::HighResolution);
    printer.setOutputFormat(QPrinter::PdfFormat);
    printer.setOutputFileName(path);
    printer.setPageOrientation(QPageLayout::Portrait);

    QString html;
    html += "<html><head><style>";
    html += "body { font-family: 'Segoe UI', Arial, sans-serif; margin: 20px; color: #0F172A; }";
    html += "h2 { text-align: center; margin-bottom: 4px; color: #0F172A; }";
    html += "h4 { text-align: center; margin-top: 0px; color: #64748B; font-weight: 500; }";
    html += "table { width: 100%; border-collapse: collapse; margin-top: 15px; }";
    html += "th { background-color: #0F172A; color: #FFFFFF; font-weight: bold; font-size: 11px; padding: 6px; border: 1px solid #CBD5E1; }";
    html += "td { font-size: 10.5px; padding: 5px; border: 1px solid #E2E8F0; }";
    html += "tr:nth-child(even) { background-color: #F8FAFC; }";
    html += ".num { text-align: right; }";
    html += ".bold { font-weight: bold; }";
    html += ".grand { background-color: #F1F5F9; font-weight: bold; border-top: 2px solid #0F172A; }";
    html += ".bank { background-color: #F0F9FF; font-weight: bold; color: #2563EB; }";
    html += "</style></head><body>";

    html += QString("<h2>%1</h2>").arg(m_controller.statementTitle().toUpper());
    html += QString("<h4>Period: %1 To %2</h4>").arg(FiscalYearHelper::formatDisplayDate(m_controller.fromDate()), FiscalYearHelper::formatDisplayDate(m_controller.toDate()));

    html += "<table>";
    html += "<thead><tr>";
    html += "<th style='text-align: left;'>Ledger Name</th>";
    html += "<th style='text-align: right; width: 15%;'>Op.Balance</th>";
    html += "<th style='text-align: right; width: 18%;'>Receipts</th>";
    html += "<th style='text-align: right; width: 18%;'>Payments</th>";
    html += "<th style='text-align: right; width: 18%;'>Cl.Balance</th>";
    html += "</tr></thead><tbody>";

    for (const auto& r : m_controller.partyRows()) {
        QString recStr = r.receipts > 0.0 ? AccountingEngine::formatCurrency(r.receipts) : "--";
        QString payStr = r.payments > 0.0 ? AccountingEngine::formatCurrency(r.payments) : "--";
        html += QString("<tr><td>%1</td><td class='num'>%2</td><td class='num' style='color: #16A34A;'>%3</td><td class='num'>%4</td><td class='num'>%5</td></tr>")
                    .arg(r.ledgerName)
                    .arg(r.opBalance != 0.0 ? QString::number(r.opBalance, 'f', 2) : "")
                    .arg(recStr)
                    .arg(payStr)
                    .arg(r.clBalance != 0.0 ? QString::number(r.clBalance, 'f', 2) : "");
    }

    for (const auto& b : m_controller.bankCashRows()) {
        html += QString("<tr class='bank'><td>%1</td><td class='num'></td><td class='num'></td><td class='num'></td><td class='num'>%2</td></tr>")
                    .arg(b.ledgerName)
                    .arg(AccountingEngine::formatCurrency(b.clBalance));
    }

    const auto& s = m_controller.summary();
    html += QString("<tr class='grand'><td>Grand Total</td><td class='num'>%1</td><td class='num' style='color: #16A34A;'>%2</td><td class='num'>%3</td><td class='num' style='color: #16A34A;'>%4</td></tr>")
                .arg(QString::number(s.totalOpBalance, 'f', 2))
                .arg(AccountingEngine::formatCurrency(s.totalReceipts))
                .arg(AccountingEngine::formatCurrency(s.totalPayments))
                .arg(AccountingEngine::formatCurrency(s.totalClBalance));

    html += "</tbody></table></body></html>";

    QTextDocument doc;
    doc.setHtml(html);
    doc.print(&printer);

    CustomMessageBox::information(this, "Export PDF", QString("Statement PDF generated successfully at:\n%1").arg(path));
}

void CashBankFlowWidget::showEvent(QShowEvent* event) {
    QWidget::showEvent(event);
    refreshData();
    if (m_table) m_table->setFocus();
}

void CashBankFlowWidget::keyPressEvent(QKeyEvent* event) {
    if (event->key() == Qt::Key_Escape) {
        emit backRequested();
        event->accept();
        return;
    }
    if (event->key() == Qt::Key_F2) {
        onAccountingPeriod();
        event->accept();
        return;
    }
    if (event->key() == Qt::Key_F5) {
        refreshData();
        event->accept();
        return;
    }
    if (event->key() == Qt::Key_A && (event->modifiers() & Qt::AltModifier)) {
        setStatementMode(FlowStatementType::CashFlow);
        event->accept();
        return;
    }
    if (event->key() == Qt::Key_N && (event->modifiers() & Qt::AltModifier)) {
        setStatementMode(FlowStatementType::BankFlow);
        event->accept();
        return;
    }
    if (event->key() == Qt::Key_J && (event->modifiers() & Qt::AltModifier)) {
        setStatementMode(FlowStatementType::JointFlow);
        event->accept();
        return;
    }
    if (event->key() == Qt::Key_P && (event->modifiers() & Qt::AltModifier)) {
        onExportPdf();
        event->accept();
        return;
    }
    if (event->key() == Qt::Key_E && (event->modifiers() & Qt::AltModifier)) {
        onExportCsv();
        event->accept();
        return;
    }
    if (event->key() == Qt::Key_F && (event->modifiers() & Qt::ControlModifier)) {
        if (m_searchBox) {
            m_searchBox->setFocus();
            m_searchBox->selectAll();
        }
        event->accept();
        return;
    }
    if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
        int r = m_table->currentRow();
        if (r >= 0) {
            onRowDoubleClicked(r, 0);
            event->accept();
            return;
        }
    }
    QWidget::keyPressEvent(event);
}

} // namespace MahadevERP
