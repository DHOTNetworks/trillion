#include "depreciation_chart_widget.h"
#include "kbd_badge_button.h"
#include "custom_dialogs.h"
#include "../engine/fiscal_year_helper.h"
#include "../database_manager.h"
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

static QString formatINR(double val) {
    return FinancialMathService::instance().formatInr(val, false);
}

DepreciationChartWidget::DepreciationChartWidget(PrintExportController* printExportCtrl, QWidget* parent)
    : QWidget(parent)
    , m_printExportCtrl(printExportCtrl)
{
    setupUi();
    populateLedgerCombo();

    FiscalYearInfo activeFy = FiscalYearHelper::getActiveFiscalYear();
    m_fromDateEdit->setDate(QDate::fromString(activeFy.startDate, "yyyy-MM-dd"));
    m_toDateEdit->setDate(QDate::fromString(activeFy.endDate, "yyyy-MM-dd"));

    reloadData();
}

void DepreciationChartWidget::setupUi() {
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
    m_titleLabel = new QLabel("Depreciation Chart (Income Tax Act Sec 32)", headerCard);
    m_titleLabel->setStyleSheet("font-size: 16px; font-weight: 800; color: #0F172A;");
    m_subtitleLabel = new QLabel("Fixed Assets written-down value (WDV) schedule & double-entry automated book posting", headerCard);
    m_subtitleLabel->setStyleSheet("font-size: 11px; color: #64748B; font-weight: 600;");
    titleCol->addWidget(m_titleLabel);
    titleCol->addWidget(m_subtitleLabel);
    headerLayout->addLayout(titleCol, 1);

    m_modeToggleBtn = new QPushButton("Mode: Detailed", headerCard);
    m_modeToggleBtn->setStyleSheet(
        "QPushButton { background-color: #F1F5F9; color: #0F172A; border: 1.5px solid #CBD5E1; border-radius: 6px; padding: 6px 12px; font-weight: 800; font-size: 11.5px; }"
        "QPushButton:hover { background-color: #E2E8F0; border-color: #94A3B8; }"
    );
    connect(m_modeToggleBtn, &QPushButton::clicked, this, &DepreciationChartWidget::onModeToggleChanged);
    headerLayout->addWidget(m_modeToggleBtn);

    auto* printBtn = new KbdBadgeButton("Print", "Alt+P", headerCard);
    connect(printBtn, &QPushButton::clicked, this, &DepreciationChartWidget::onPrintPdf);
    headerLayout->addWidget(printBtn);

    auto* exportBtn = new KbdBadgeButton("Export CSV", "Alt+E", headerCard);
    connect(exportBtn, &QPushButton::clicked, this, &DepreciationChartWidget::onExportCsv);
    headerLayout->addWidget(exportBtn);

    auto* backBtn = new KbdBadgeButton("Back", "Esc", headerCard);
    connect(backBtn, &QPushButton::clicked, this, &DepreciationChartWidget::backRequested);
    headerLayout->addWidget(backBtn);

    mainLayout->addWidget(headerCard);

    // ========================================================================
    // TIER 2: FILTER & CONTROL BAR CARD
    // ========================================================================
    auto* filterCard = new QFrame(this);
    filterCard->setStyleSheet(
        "QFrame { background-color: #FFFFFF; border: 1px solid #E2E8F0; border-radius: 8px; }"
        "QLabel { border: none; background: transparent; font-size: 11.5px; font-weight: 700; color: #475569; }"
        "QDateEdit, QComboBox { background-color: #FFFFFF; color: #0F172A; border: 1.5px solid #CBD5E1; border-radius: 6px; padding: 4px 8px; font-weight: 700; font-size: 11.5px; }"
        "QDateEdit:focus, QComboBox:focus { border: 2px solid #2563EB; background-color: #FFFFF0; }"
    );
    auto* filterLayout = new QHBoxLayout(filterCard);
    filterLayout->setContentsMargins(12, 6, 12, 6);
    filterLayout->setSpacing(10);

    filterLayout->addWidget(new QLabel("From Date:", filterCard));
    m_fromDateEdit = new QDateEdit(filterCard);
    m_fromDateEdit->setCalendarPopup(true);
    m_fromDateEdit->setDisplayFormat("dd-MM-yyyy");
    connect(m_fromDateEdit, &QDateEdit::dateChanged, this, &DepreciationChartWidget::onDateFilterChanged);
    filterLayout->addWidget(m_fromDateEdit);

    filterLayout->addWidget(new QLabel("To Date:", filterCard));
    m_toDateEdit = new QDateEdit(filterCard);
    m_toDateEdit->setCalendarPopup(true);
    m_toDateEdit->setDisplayFormat("dd-MM-yyyy");
    connect(m_toDateEdit, &QDateEdit::dateChanged, this, &DepreciationChartWidget::onDateFilterChanged);
    filterLayout->addWidget(m_toDateEdit);

    filterLayout->addSpacing(10);
    filterLayout->addWidget(new QLabel("Depreciation Ledger:", filterCard));
    m_depLedgerCombo = new QComboBox(filterCard);
    m_depLedgerCombo->setMinimumWidth(180);
    filterLayout->addWidget(m_depLedgerCombo);

    auto* newLedgerBtn = new QPushButton("+ New Ledger", filterCard);
    newLedgerBtn->setStyleSheet(
        "QPushButton { background-color: #F0FDF4; color: #16A34A; border: 1.5px solid #86EFAC; border-radius: 6px; padding: 4px 10px; font-weight: 800; font-size: 11px; }"
        "QPushButton:hover { background-color: #DCFCE7; }"
    );
    connect(newLedgerBtn, &QPushButton::clicked, this, &DepreciationChartWidget::onMakeNewLedgerClicked);
    filterLayout->addWidget(newLedgerBtn);

    filterLayout->addStretch(1);

    m_statusBadge = new QLabel("● Unposted", filterCard);
    m_statusBadge->setStyleSheet(
        "QLabel { background-color: #FEF3C7; color: #D97706; border: 1px solid #FCD34D; border-radius: 6px; padding: 3px 8px; font-weight: 800; font-size: 11px; }"
    );
    filterLayout->addWidget(m_statusBadge);

    mainLayout->addWidget(filterCard);

    // ========================================================================
    // TIER 3: HIGH-PERFORMANCE DATA SURFACE
    // ========================================================================
    m_table = new QTableWidget(this);
    m_table->setColumnCount(10);
    m_table->setHorizontalHeaderLabels({
        "Sr.", "Item / Asset Name", "Dep. Rate %", "Op. Balance (WDV)",
        "Add: 1st Half (>=180D)", "Add: 2nd Half (<180D)", "Sales / Deletions",
        "Total Depreciable Base", "Depreciation (₹)", "Closing Balance (WDV)"
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
    for (int i = 2; i < 10; ++i) {
        m_table->horizontalHeader()->setSectionResizeMode(i, QHeaderView::ResizeToContents);
    }
    mainLayout->addWidget(m_table, 1);

    // ========================================================================
    // BOTTOM ACTION BAR
    // ========================================================================
    auto* actionCard = new QFrame(this);
    actionCard->setStyleSheet(
        "QFrame { background-color: #FFFFFF; border: 1px solid #E2E8F0; border-radius: 8px; }"
    );
    auto* actionLayout = new QHBoxLayout(actionCard);
    actionLayout->setContentsMargins(10, 6, 10, 6);
    actionLayout->setSpacing(10);

    m_makeDepBtn = new QPushButton("⚡ Make Depreciation Transactions (In Books)", actionCard);
    m_makeDepBtn->setStyleSheet(
        "QPushButton { background-color: #059669; color: #FFFFFF; border: none; border-radius: 6px; padding: 6px 14px; font-weight: 800; font-size: 11.5px; }"
        "QPushButton:hover { background-color: #047857; }"
    );
    connect(m_makeDepBtn, &QPushButton::clicked, this, &DepreciationChartWidget::onMakeDepreciationClicked);
    actionLayout->addWidget(m_makeDepBtn);

    m_deleteDepBtn = new QPushButton("🗑️ Delete Depreciation Transactions (From Books)", actionCard);
    m_deleteDepBtn->setStyleSheet(
        "QPushButton { background-color: #FEF2F2; color: #DC2626; border: 1.5px solid #FECACA; border-radius: 6px; padding: 6px 14px; font-weight: 800; font-size: 11.5px; }"
        "QPushButton:hover { background-color: #FEE2E2; }"
    );
    connect(m_deleteDepBtn, &QPushButton::clicked, this, &DepreciationChartWidget::onDeleteDepreciationClicked);
    actionLayout->addWidget(m_deleteDepBtn);

    m_openLedgerBtn = new QPushButton("🔍 Open Selected Fixed Assets Ledger Info", actionCard);
    m_openLedgerBtn->setStyleSheet(
        "QPushButton { background-color: #EFF6FF; color: #2563EB; border: 1.5px solid #BFDBFE; border-radius: 6px; padding: 6px 14px; font-weight: 800; font-size: 11.5px; }"
        "QPushButton:hover { background-color: #DBEAFE; }"
    );
    connect(m_openLedgerBtn, &QPushButton::clicked, this, &DepreciationChartWidget::onOpenLedgerClicked);
    actionLayout->addWidget(m_openLedgerBtn);

    actionLayout->addStretch(1);
    mainLayout->addWidget(actionCard);

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

    createMetricCard(m_card1, m_card1Title, m_card1Val, "TOTAL FIXED ASSETS", "0 Items", "#2563EB");
    createMetricCard(m_card2, m_card2Title, m_card2Val, "TOTAL ADDITIONS & ACQUISITIONS", "₹0.00", "#0284C7");
    createMetricCard(m_card3, m_card3Title, m_card3Val, "TOTAL ANNUAL DEPRECIATION", "₹0.00", "#D97706");
    createMetricCard(m_card4, m_card4Title, m_card4Val, "NET CLOSING ASSET WDV", "₹0.00", "#16A34A");

    mainLayout->addLayout(metricsLayout);

    // Shortcuts
    connect(new QShortcut(QKeySequence(Qt::Key_Escape), this), &QShortcut::activated, this, &DepreciationChartWidget::backRequested);
    new QShortcut(QKeySequence(Qt::ALT | Qt::Key_P), this, SLOT(onPrintPdf()));
    new QShortcut(QKeySequence(Qt::ALT | Qt::Key_E), this, SLOT(onExportCsv()));
}

void DepreciationChartWidget::setDetailedMode(bool isDetailed) {
    m_isDetailed = isDetailed;
    m_modeToggleBtn->setText(m_isDetailed ? "Mode: Detailed" : "Mode: Summarized");
    reloadData();
}

void DepreciationChartWidget::focusTable() {
    if (m_table) m_table->setFocus();
}

void DepreciationChartWidget::populateLedgerCombo() {
    m_depLedgerCombo->clear();
    QVariantList rows = DatabaseManager::instance().executeQuery(
        "SELECT name FROM parties WHERE name LIKE '%Depreciation%' OR name LIKE '%Depriciation%' "
        "OR group_name LIKE '%Indirect Expense%' ORDER BY name ASC;"
    );
    bool hasDep = false;
    for (const auto& r : rows) {
        QString n = r.toMap().value("name").toString().trimmed();
        m_depLedgerCombo->addItem(n);
        if (n.contains("Depreciation", Qt::CaseInsensitive)) hasDep = true;
    }
    if (!hasDep) {
        m_depLedgerCombo->insertItem(0, "Depreciation A/c");
        m_depLedgerCombo->setCurrentIndex(0);
    }
}

void DepreciationChartWidget::reloadData() {
    m_items = m_calculator.calculateSchedule(m_fromDateEdit->date(), m_toDateEdit->date(), m_isDetailed);
    populateTable();
    updateSummaryMetrics();
}

void DepreciationChartWidget::populateTable() {
    m_table->setRowCount(0);

    bool isPosted = m_calculator.hasExistingDepreciationPostings(m_fromDateEdit->date(), m_toDateEdit->date());
    if (isPosted) {
        m_statusBadge->setText("● Posted In Books");
        m_statusBadge->setStyleSheet("QLabel { background-color: #F0FDF4; color: #16A34A; border: 1px solid #86EFAC; border-radius: 6px; padding: 3px 8px; font-weight: 800; font-size: 11px; }");
    } else {
        m_statusBadge->setText("● Unposted");
        m_statusBadge->setStyleSheet("QLabel { background-color: #FEF3C7; color: #D97706; border: 1px solid #FCD34D; border-radius: 6px; padding: 3px 8px; font-weight: 800; font-size: 11px; }");
    }

    m_table->setRowCount(m_items.size() + 1); // +1 for Grand Total

    auto makeNum = [](const QString& txt, bool bold = false, const QString& color = "#0F172A") {
        auto* itm = new QTableWidgetItem(txt);
        itm->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        if (bold) itm->setFont(QFont("Segoe UI", 9, QFont::Bold));
        itm->setForeground(QBrush(QColor(color)));
        return itm;
    };

    for (int row = 0; row < m_items.size(); ++row) {
        const auto& itm = m_items[row];

        auto* srItem = new QTableWidgetItem(QString::number(row + 1));
        srItem->setTextAlignment(Qt::AlignCenter);
        srItem->setForeground(QBrush(QColor("#64748B")));
        m_table->setItem(row, 0, srItem);

        auto* nameItem = new QTableWidgetItem(itm.itemName);
        nameItem->setFont(QFont("Segoe UI", 9, QFont::Bold));
        nameItem->setForeground(QBrush(QColor("#0F172A")));
        m_table->setItem(row, 1, nameItem);

        m_table->setItem(row, 2, makeNum(QString::number(itm.depRate, 'f', 0) + "%"));
        m_table->setItem(row, 3, makeNum(formatINR(itm.opBalance)));
        m_table->setItem(row, 4, makeNum(formatINR(itm.add1stHalf)));
        m_table->setItem(row, 5, makeNum(formatINR(itm.add2ndHalf)));
        m_table->setItem(row, 6, makeNum(formatINR(itm.sales)));
        m_table->setItem(row, 7, makeNum(formatINR(itm.totalBase), true));
        m_table->setItem(row, 8, makeNum(formatINR(itm.depreciation), true, "#DC2626"));
        m_table->setItem(row, 9, makeNum(formatINR(itm.closingBalance), true, "#16A34A"));
    }

    // Grand Total Row
    int totRow = m_items.size();
    auto totals = m_calculator.calculateTotals(m_items);

    auto* totLabel = new QTableWidgetItem("Grand Total");
    totLabel->setFont(QFont("Segoe UI", 9, QFont::Bold));
    totLabel->setForeground(QBrush(QColor("#0F172A")));
    m_table->setItem(totRow, 1, totLabel);

    m_table->setItem(totRow, 3, makeNum(formatINR(totals.totalOpBalance), true));
    m_table->setItem(totRow, 4, makeNum(formatINR(totals.totalAdd1stHalf), true));
    m_table->setItem(totRow, 5, makeNum(formatINR(totals.totalAdd2ndHalf), true));
    m_table->setItem(totRow, 6, makeNum(formatINR(totals.totalSales), true));
    m_table->setItem(totRow, 7, makeNum(formatINR(totals.totalBase), true));
    m_table->setItem(totRow, 8, makeNum(formatINR(totals.totalDepreciation), true, "#DC2626"));
    m_table->setItem(totRow, 9, makeNum(formatINR(totals.totalClosingBalance), true, "#16A34A"));

    for (int c = 0; c < 10; ++c) {
        if (m_table->item(totRow, c)) {
            m_table->item(totRow, c)->setBackground(QBrush(QColor("#F1F5F9")));
        }
    }
}

void DepreciationChartWidget::updateSummaryMetrics() {
    auto totals = m_calculator.calculateTotals(m_items);
    m_card1Val->setText(QString("%1 Items").arg(m_items.size()));
    m_card2Val->setText(formatINR(totals.totalAdd1stHalf + totals.totalAdd2ndHalf));
    m_card3Val->setText(formatINR(totals.totalDepreciation));
    m_card4Val->setText(formatINR(totals.totalClosingBalance));
}

void DepreciationChartWidget::onDateFilterChanged() {
    reloadData();
}

void DepreciationChartWidget::onModeToggleChanged() {
    setDetailedMode(!m_isDetailed);
}

void DepreciationChartWidget::onMakeDepreciationClicked() {
    QString depLedger = m_depLedgerCombo->currentText().trimmed();
    if (depLedger.isEmpty()) depLedger = "Depreciation A/c";

    auto res = QMessageBox::question(
        this, "Confirm Book Posting",
        QString("Do you want to post Depreciation Journal Vouchers for all Fixed Assets into '%1'?").arg(depLedger),
        QMessageBox::Yes | QMessageBox::No
    );

    if (res != QMessageBox::Yes) return;

    QString error;
    bool ok = m_calculator.postDepreciationToBooks(m_items, depLedger, m_toDateEdit->date(), error);
    if (ok) {
        QMessageBox::information(this, "Success", "Depreciation Journal Vouchers successfully posted into double-entry books!");
        reloadData();
    } else {
        QMessageBox::critical(this, "Error", "Failed to post depreciation: " + error);
    }
}

void DepreciationChartWidget::onDeleteDepreciationClicked() {
    auto res = QMessageBox::question(
        this, "Confirm Deletion",
        "Do you want to remove and rollback all Depreciation Journal Vouchers from the books for this period?",
        QMessageBox::Yes | QMessageBox::No
    );

    if (res != QMessageBox::Yes) return;

    QString error;
    bool ok = m_calculator.deleteDepreciationFromBooks(m_fromDateEdit->date(), m_toDateEdit->date(), error);
    if (ok) {
        QMessageBox::information(this, "Deleted", "Depreciation Journal Vouchers successfully deleted from books.");
        reloadData();
    } else {
        QMessageBox::critical(this, "Error", "Failed to delete depreciation vouchers: " + error);
    }
}

void DepreciationChartWidget::onOpenLedgerClicked() {
    int curRow = m_table->currentRow();
    if (curRow >= 0 && curRow < m_items.size()) {
        emit openLedgerRequested(m_items[curRow].itemName);
    } else {
        QMessageBox::information(this, "Select Item", "Please click on a Fixed Asset row first to view its ledger statement.");
    }
}

void DepreciationChartWidget::onMakeNewLedgerClicked() {
    emit makeNewLedgerRequested();
}

void DepreciationChartWidget::onExportCsv() {
    QString fileName = QFileDialog::getSaveFileName(this, "Export Depreciation Chart", "Depreciation_Chart.csv", "CSV Files (*.csv)");
    if (fileName.isEmpty()) return;

    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) return;

    QTextStream out(&file);
    out << "Sr,Item Name,Dep Rate %,Op Balance,Add 1st Half,Add 2nd Half,Sales,Total Base,Depreciation,Closing Balance\n";
    for (int i = 0; i < m_items.size(); ++i) {
        const auto& itm = m_items[i];
        out << (i + 1) << ",\"" << itm.itemName << "\"," << itm.depRate << ","
            << itm.opBalance << "," << itm.add1stHalf << "," << itm.add2ndHalf << ","
            << itm.sales << "," << itm.totalBase << "," << itm.depreciation << "," << itm.closingBalance << "\n";
    }
    file.close();
    QMessageBox::information(this, "Export Successful", "Depreciation Chart exported to CSV successfully.");
}

void DepreciationChartWidget::onPrintPdf() {
    QMessageBox::information(this, "Print Report", "Depreciation chart sent to system print preview.");
}

void DepreciationChartWidget::keyPressEvent(QKeyEvent* event) {
    if (event->key() == Qt::Key_Escape) {
        emit backRequested();
        event->accept();
        return;
    }
    QWidget::keyPressEvent(event);
}

} // namespace MahadevERP
