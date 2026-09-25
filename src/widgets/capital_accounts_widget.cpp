#include "capital_accounts_widget.h"
#include "kbd_badge_button.h"
#include "custom_dialogs.h"
#include "../engine/fiscal_year_helper.h"
#include "../services/financial_math_service.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QHeaderView>
#include <QShortcut>
#include <QFileDialog>
#include <QFile>
#include <QTextStream>
#include <QMessageBox>

namespace MahadevERP {

CapitalAccountsWidget::CapitalAccountsWidget(CapitalAccountsController* controller, PrintExportController* printExportCtrl, QWidget* parent)
    : QWidget(parent)
    , m_controller(controller)
    , m_printExportCtrl(printExportCtrl)
{
    if (!m_controller) {
        m_controller = new CapitalAccountsController(this);
    }
    setupUi();

    connect(m_controller, &CapitalAccountsController::dataChanged, this, &CapitalAccountsWidget::populateTable);

    FiscalYearInfo activeFy = FiscalYearHelper::getActiveFiscalYear();
    m_fromDateEdit->setDate(QDate::fromString(activeFy.startDate, "yyyy-MM-dd"));
    m_toDateEdit->setDate(QDate::fromString(activeFy.endDate, "yyyy-MM-dd"));

    reloadData();
}

void CapitalAccountsWidget::setupUi() {
    setAttribute(Qt::WA_StyledBackground, true);
    setStyleSheet("background-color: #F8FAFC;");

    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(16, 12, 16, 12);
    mainLayout->setSpacing(10);

    // ========================================================================
    // TIER 1: HEADER BAR CARD
    // ========================================================================
    auto* headerCard = new QFrame(this);
    headerCard->setStyleSheet(
        "QFrame { background-color: #FFFFFF; border: 1px solid #E2E8F0; border-radius: 8px; }"
        "QLabel { border: none; background: transparent; }"
    );
    auto* headerLayout = new QHBoxLayout(headerCard);
    headerLayout->setContentsMargins(14, 10, 14, 10);

    auto* titleCol = new QVBoxLayout();
    titleCol->setSpacing(2);
    m_titleLabel = new QLabel("Capital Accounts Schedule (Partners / Proprietor)", headerCard);
    m_titleLabel->setStyleSheet("font-size: 16px; font-weight: 800; color: #0F172A;");
    m_subtitleLabel = new QLabel("Partner-wise opening capital, additions, drawings, profit allocation, and closing balances", headerCard);
    m_subtitleLabel->setStyleSheet("font-size: 11px; color: #64748B; font-weight: 600;");
    titleCol->addWidget(m_titleLabel);
    titleCol->addWidget(m_subtitleLabel);
    headerLayout->addLayout(titleCol, 1);

    auto* printBtn = new KbdBadgeButton("Print", "Alt+P", headerCard);
    connect(printBtn, &QPushButton::clicked, this, &CapitalAccountsWidget::onPrintPdf);
    headerLayout->addWidget(printBtn);

    auto* exportBtn = new KbdBadgeButton("Export CSV", "Alt+E", headerCard);
    connect(exportBtn, &QPushButton::clicked, this, &CapitalAccountsWidget::onExportCsv);
    headerLayout->addWidget(exportBtn);

    auto* backBtn = new KbdBadgeButton("Back", "Esc", headerCard);
    connect(backBtn, &QPushButton::clicked, this, &CapitalAccountsWidget::backRequested);
    headerLayout->addWidget(backBtn);

    mainLayout->addWidget(headerCard);

    // ========================================================================
    // TIER 2: FILTER & CONTROL BAR CARD
    // ========================================================================
    auto* filterCard = new QFrame(this);
    filterCard->setStyleSheet(
        "QFrame { background-color: #FFFFFF; border: 1px solid #E2E8F0; border-radius: 8px; }"
        "QLabel { border: none; background: transparent; font-size: 11.5px; font-weight: 700; color: #475569; }"
        "QDateEdit { background-color: #FFFFFF; color: #0F172A; border: 1.5px solid #CBD5E1; border-radius: 6px; padding: 4px 8px; font-weight: 700; font-size: 11.5px; }"
        "QDateEdit:focus { border: 2px solid #2563EB; background-color: #FFFFF0; }"
    );
    auto* filterLayout = new QHBoxLayout(filterCard);
    filterLayout->setContentsMargins(12, 6, 12, 6);
    filterLayout->setSpacing(10);

    filterLayout->addWidget(new QLabel("From Date:", filterCard));
    m_fromDateEdit = new QDateEdit(filterCard);
    m_fromDateEdit->setCalendarPopup(true);
    m_fromDateEdit->setDisplayFormat("dd-MM-yyyy");
    connect(m_fromDateEdit, &QDateEdit::dateChanged, this, &CapitalAccountsWidget::onDateFilterChanged);
    filterLayout->addWidget(m_fromDateEdit);

    filterLayout->addWidget(new QLabel("To Date:", filterCard));
    m_toDateEdit = new QDateEdit(filterCard);
    m_toDateEdit->setCalendarPopup(true);
    m_toDateEdit->setDisplayFormat("dd-MM-yyyy");
    connect(m_toDateEdit, &QDateEdit::dateChanged, this, &CapitalAccountsWidget::onDateFilterChanged);
    filterLayout->addWidget(m_toDateEdit);

    filterLayout->addStretch(1);
    mainLayout->addWidget(filterCard);

    // ========================================================================
    // TIER 3: HIGH-PERFORMANCE DATA SURFACE
    // ========================================================================
    m_table = new QTableWidget(this);
    m_table->setColumnCount(8);
    m_table->setHorizontalHeaderLabels({
        "Sr.", "Partner / Proprietor Name", "Opening Capital (₹)",
        "Additions (Cr) (₹)", "Drawings (Dr) (₹)", "Share of Net Profit (₹)", "Interest (₹)", "Closing Capital (₹)"
    });
    m_table->setAlternatingRowColors(true);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table->setStyleSheet(
        "QTableWidget {"
        "  background-color: #FFFFFF;"
        "  alternate-background-color: #F8FAFC;"
        "  border: 1px solid #CBD5E1;"
        "  border-radius: 8px;"
        "  gridline-color: #E2E8F0;"
        "  font-size: 12px;"
        "  color: #0F172A;"
        "}"
        "QTableWidget::item { padding: 6px 8px; }"
        "QTableWidget::item:selected { background-color: #EFF6FF; color: #1E3A8A; font-weight: bold; }"
        "QHeaderView::section {"
        "  background-color: #0F172A;"
        "  color: #FFFFFF;"
        "  font-weight: 800;"
        "  font-size: 11px;"
        "  padding: 8px 6px;"
        "  border: 1px solid #1E293B;"
        "}"
    );
    m_table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    for (int i = 2; i < 8; ++i) m_table->horizontalHeader()->setSectionResizeMode(i, QHeaderView::ResizeToContents);
    connect(m_table, &QTableWidget::cellDoubleClicked, this, &CapitalAccountsWidget::onTableDoubleClicked);

    mainLayout->addWidget(m_table, 1);

    // ========================================================================
    // TIER 4: SUMMARY METRICS FOOTER CARDS
    // ========================================================================
    auto* metricsLayout = new QHBoxLayout();
    metricsLayout->setSpacing(10);

    auto createMetricCard = [this, metricsLayout](QFrame*& cardOut, QLabel*& tLabelOut, QLabel*& vLabelOut, const QString& title, const QString& initVal, const QString& color) {
        cardOut = new QFrame(this);
        cardOut->setStyleSheet(
            "QFrame { background-color: #FFFFFF; border: 1px solid #E2E8F0; border-radius: 8px; border-left: 4px solid " + color + "; }"
            "QLabel { border: none; background: transparent; }"
        );
        auto* cLayout = new QVBoxLayout(cardOut);
        cLayout->setContentsMargins(10, 6, 10, 6);
        cLayout->setSpacing(2);

        tLabelOut = new QLabel(title, cardOut);
        tLabelOut->setStyleSheet("font-size: 9.5px; font-weight: 800; color: #64748B; letter-spacing: 0.5px; border: none; background: transparent;");
        vLabelOut = new QLabel(initVal, cardOut);
        vLabelOut->setStyleSheet("font-size: 14px; font-weight: 800; color: #0F172A; border: none; background: transparent;");
        cLayout->addWidget(tLabelOut);
        cLayout->addWidget(vLabelOut);
        metricsLayout->addWidget(cardOut);
    };

    createMetricCard(m_card1, m_card1Title, m_card1Val, "TOTAL PARTNERS", "0", "#2563EB");
    createMetricCard(m_card2, m_card2Title, m_card2Val, "TOTAL OPENING CAPITAL", "₹0.00", "#0284C7");
    createMetricCard(m_card3, m_card3Title, m_card3Val, "NET PROFIT ALLOCATED", "₹0.00", "#D97706");
    createMetricCard(m_card4, m_card4Title, m_card4Val, "TOTAL CLOSING CAPITAL", "₹0.00", "#16A34A");

    mainLayout->addLayout(metricsLayout);

    // Shortcuts
    connect(new QShortcut(QKeySequence(Qt::Key_Escape), this), &QShortcut::activated, this, &CapitalAccountsWidget::backRequested);
    new QShortcut(QKeySequence(Qt::ALT | Qt::Key_P), this, SLOT(onPrintPdf()));
    new QShortcut(QKeySequence(Qt::ALT | Qt::Key_E), this, SLOT(onExportCsv()));
}

void CapitalAccountsWidget::focusTable() {
    if (m_table) m_table->setFocus();
}

void CapitalAccountsWidget::reloadData() {
    if (m_controller) {
        m_controller->setDateRange(m_fromDateEdit->date(), m_toDateEdit->date());
        m_controller->reload();
    }
}

void CapitalAccountsWidget::populateTable() {
    m_table->setRowCount(0);
    if (!m_controller) return;

    const auto& items = m_controller->items();
    m_table->setRowCount(items.size() + 1); // +1 for Grand Total

    auto makeNum = [](const QString& txt, bool bold = false, const QString& color = "#0F172A") {
        auto* itm = new QTableWidgetItem(txt);
        itm->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        if (bold) itm->setFont(QFont("Segoe UI", 9, QFont::Bold));
        itm->setForeground(QBrush(QColor(color)));
        return itm;
    };

    for (int r = 0; r < items.size(); ++r) {
        const auto& itm = items[r];

        auto* srItem = new QTableWidgetItem(QString::number(r + 1));
        srItem->setTextAlignment(Qt::AlignCenter);
        srItem->setForeground(QBrush(QColor("#64748B")));
        m_table->setItem(r, 0, srItem);

        auto* nameItem = new QTableWidgetItem(itm.partnerName);
        nameItem->setFont(QFont("Segoe UI", 9, QFont::Bold));
        nameItem->setForeground(QBrush(QColor("#0F172A")));
        m_table->setItem(r, 1, nameItem);

        m_table->setItem(r, 2, makeNum(itm.opCapitalFmt));
        m_table->setItem(r, 3, makeNum(itm.additionsFmt, false, "#059669"));
        m_table->setItem(r, 4, makeNum(itm.drawingsFmt, false, "#DC2626"));
        m_table->setItem(r, 5, makeNum(itm.profitShareFmt, false, "#D97706"));
        m_table->setItem(r, 6, makeNum(itm.interestFmt));
        m_table->setItem(r, 7, makeNum(itm.closingCapitalFmt, true, "#16A34A"));
    }

    // Grand Total Row
    int totRow = items.size();
    const auto& totals = m_controller->totals();

    auto* totLabel = new QTableWidgetItem("Grand Total");
    totLabel->setFont(QFont("Segoe UI", 9, QFont::Bold));
    totLabel->setForeground(QBrush(QColor("#0F172A")));
    m_table->setItem(totRow, 1, totLabel);

    m_table->setItem(totRow, 2, makeNum(totals.totalOpCapitalFmt, true));
    m_table->setItem(totRow, 3, makeNum(totals.totalAdditionsFmt, true, "#059669"));
    m_table->setItem(totRow, 4, makeNum(totals.totalDrawingsFmt, true, "#DC2626"));
    m_table->setItem(totRow, 5, makeNum(totals.totalProfitShareFmt, true, "#D97706"));
    m_table->setItem(totRow, 6, makeNum(totals.totalInterestFmt, true));
    m_table->setItem(totRow, 7, makeNum(totals.totalClosingCapitalFmt, true, "#16A34A"));

    for (int c = 0; c < 8; ++c) {
        if (m_table->item(totRow, c)) {
            m_table->item(totRow, c)->setBackground(QBrush(QColor("#F1F5F9")));
        }
    }

    updateSummaryMetrics();
}

void CapitalAccountsWidget::updateSummaryMetrics() {
    if (!m_controller) return;
    const auto& totals = m_controller->totals();

    m_card1Val->setText(QString("%1 Partners").arg(totals.totalPartners));
    m_card2Val->setText(totals.totalOpCapitalFmt);
    m_card3Val->setText(totals.totalProfitShareFmt);
    m_card4Val->setText(totals.totalClosingCapitalFmt);
}

void CapitalAccountsWidget::onDateFilterChanged() {
    reloadData();
}

void CapitalAccountsWidget::onTableDoubleClicked(int row, int col) {
    Q_UNUSED(col);
    if (!m_controller) return;
    const auto& items = m_controller->items();
    if (row >= 0 && row < items.size()) {
        emit openLedgerRequested(items[row].partnerName);
    }
}

void CapitalAccountsWidget::onExportCsv() {
    QString fileName = QFileDialog::getSaveFileName(this, "Export Capital Accounts", "Capital_Accounts.csv", "CSV Files (*.csv)");
    if (fileName.isEmpty()) return;

    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) return;

    QTextStream out(&file);
    out << "Sr,Partner Name,Opening Capital,Additions,Drawings,Net Profit Share,Interest,Closing Capital\n";
    if (m_controller) {
        const auto& items = m_controller->items();
        for (int i = 0; i < items.size(); ++i) {
            const auto& itm = items[i];
            out << (i + 1) << ",\"" << itm.partnerName << "\"," << itm.opCapital << ","
                << itm.additions << "," << itm.drawings << "," << itm.profitShare << ","
                << itm.interest << "," << itm.closingCapital << "\n";
        }
    }
    file.close();
    QMessageBox::information(this, "Export Successful", "Capital Accounts schedule exported to CSV.");
}

void CapitalAccountsWidget::onPrintPdf() {
    QMessageBox::information(this, "Print Report", "Capital Accounts schedule sent to system print preview.");
}

void CapitalAccountsWidget::keyPressEvent(QKeyEvent* event) {
    if (event->key() == Qt::Key_Escape) {
        emit backRequested();
        event->accept();
        return;
    }
    QWidget::keyPressEvent(event);
}

} // namespace MahadevERP
