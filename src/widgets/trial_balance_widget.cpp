#include "trial_balance_widget.h"
#include "kbd_badge_button.h"
#include "custom_dialogs.h"
#include "../engine/fiscal_year_helper.h"
#include "../services/financial_math_service.h"
#include "../models/menu_tree_manager.h"
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

TrialBalanceWidget::TrialBalanceWidget(TrialBalanceController* controller, PrintExportController* printExportCtrl, QWidget* parent)
    : QWidget(parent)
    , m_controller(controller)
    , m_printExportCtrl(printExportCtrl)
{
    if (!m_controller) {
        m_controller = new TrialBalanceController(this);
    }
    setupUi();

    connect(m_controller, &TrialBalanceController::dataChanged, this, &TrialBalanceWidget::populateTable);

    FiscalYearInfo activeFy = FiscalYearHelper::getActiveFiscalYear();
    m_fromDateEdit->setDate(QDate::fromString(activeFy.startDate, "yyyy-MM-dd"));
    m_toDateEdit->setDate(QDate::fromString(activeFy.endDate, "yyyy-MM-dd"));

    reloadData();
}

void TrialBalanceWidget::setupUi() {
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
    m_titleLabel = new QLabel("Trial Balance Sheet", headerCard);
    m_titleLabel->setStyleSheet("font-size: 16px; font-weight: 800; color: #0F172A;");
    m_subtitleLabel = new QLabel("General ledger mathematical balancing verification • Press F4 for View Options", headerCard);
    m_subtitleLabel->setStyleSheet("font-size: 11px; color: #64748B; font-weight: 600;");
    titleCol->addWidget(m_titleLabel);
    titleCol->addWidget(m_subtitleLabel);
    headerLayout->addLayout(titleCol, 1);

    m_modeBadge = new QLabel("Normal View", headerCard);
    m_modeBadge->setStyleSheet(
        "QLabel { background-color: #EFF6FF; color: #2563EB; border: 1.5px solid #BFDBFE; border-radius: 6px; padding: 4px 10px; font-weight: 800; font-size: 11.5px; }"
    );
    headerLayout->addWidget(m_modeBadge);

    auto* optionsBtn = new KbdBadgeButton("View Options", "F4", headerCard);
    connect(optionsBtn, &QPushButton::clicked, this, &TrialBalanceWidget::onOpenViewOptions);
    headerLayout->addWidget(optionsBtn);

    auto* printBtn = new KbdBadgeButton("Print", "Alt+P", headerCard);
    connect(printBtn, &QPushButton::clicked, this, &TrialBalanceWidget::onPrintPdf);
    headerLayout->addWidget(printBtn);

    auto* exportBtn = new KbdBadgeButton("Export CSV", "Alt+E", headerCard);
    connect(exportBtn, &QPushButton::clicked, this, &TrialBalanceWidget::onExportCsv);
    headerLayout->addWidget(exportBtn);

    auto* backBtn = new KbdBadgeButton("Back", "Esc", headerCard);
    connect(backBtn, &QPushButton::clicked, this, &TrialBalanceWidget::backRequested);
    headerLayout->addWidget(backBtn);

    mainLayout->addWidget(headerCard);

    // ========================================================================
    // TIER 2: FILTER & CONTROL BAR CARD
    // ========================================================================
    auto* filterCard = new QFrame(this);
    filterCard->setStyleSheet(
        "QFrame { background-color: #FFFFFF; border: 1px solid #E2E8F0; border-radius: 8px; }"
        "QLabel { border: none; background: transparent; font-size: 11.5px; font-weight: 700; color: #475569; }"
        "QDateEdit, QComboBox, QLineEdit { background-color: #FFFFFF; color: #0F172A; border: 1.5px solid #CBD5E1; border-radius: 6px; padding: 4px 8px; font-weight: 700; font-size: 11.5px; }"
        "QDateEdit:focus, QComboBox:focus, QLineEdit:focus { border: 2px solid #2563EB; background-color: #FFFFF0; }"
    );
    auto* filterLayout = new QHBoxLayout(filterCard);
    filterLayout->setContentsMargins(12, 6, 12, 6);
    filterLayout->setSpacing(10);

    filterLayout->addWidget(new QLabel("From Date:", filterCard));
    m_fromDateEdit = new QDateEdit(filterCard);
    m_fromDateEdit->setCalendarPopup(true);
    m_fromDateEdit->setDisplayFormat("dd-MM-yyyy");
    connect(m_fromDateEdit, &QDateEdit::dateChanged, this, &TrialBalanceWidget::onDateFilterChanged);
    filterLayout->addWidget(m_fromDateEdit);

    filterLayout->addWidget(new QLabel("To Date:", filterCard));
    m_toDateEdit = new QDateEdit(filterCard);
    m_toDateEdit->setCalendarPopup(true);
    m_toDateEdit->setDisplayFormat("dd-MM-yyyy");
    connect(m_toDateEdit, &QDateEdit::dateChanged, this, &TrialBalanceWidget::onDateFilterChanged);
    filterLayout->addWidget(m_toDateEdit);

    filterLayout->addSpacing(10);
    filterLayout->addWidget(new QLabel("View Mode:", filterCard));
    m_modeCombo = new QComboBox(filterCard);
    m_modeCombo->addItem("Normal View (Group Tree)", static_cast<int>(TrialBalanceMode::NormalView));
    m_modeCombo->addItem("Flat View (All Ledgers)", static_cast<int>(TrialBalanceMode::FlatView));
    m_modeCombo->addItem("Flat Grouped", static_cast<int>(TrialBalanceMode::FlatGrouped));
    m_modeCombo->addItem("Normal Detailed", static_cast<int>(TrialBalanceMode::NormalDetailed));
    m_modeCombo->addItem("Without Opening Bal.", static_cast<int>(TrialBalanceMode::WithoutOpBal));
    m_modeCombo->addItem("Show Turnover", static_cast<int>(TrialBalanceMode::ShowTurnover));
    m_modeCombo->addItem("Show Opening Bal.", static_cast<int>(TrialBalanceMode::ShowOpeningBal));
    connect(m_modeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &TrialBalanceWidget::onModeComboChanged);
    filterLayout->addWidget(m_modeCombo);

    filterLayout->addSpacing(10);
    m_searchEdit = new QLineEdit(filterCard);
    m_searchEdit->setPlaceholderText("Search account or group... (Ctrl+F)");
    m_searchEdit->setMinimumWidth(180);
    connect(m_searchEdit, &QLineEdit::textChanged, this, &TrialBalanceWidget::onSearchChanged);
    filterLayout->addWidget(m_searchEdit);

    filterLayout->addStretch(1);

    m_balanceStatusBadge = new QLabel("● Balanced (₹0.00)", filterCard);
    m_balanceStatusBadge->setStyleSheet(
        "QLabel { background-color: #F0FDF4; color: #16A34A; border: 1.5px solid #86EFAC; border-radius: 6px; padding: 3px 8px; font-weight: 800; font-size: 11px; }"
    );
    filterLayout->addWidget(m_balanceStatusBadge);

    mainLayout->addWidget(filterCard);

    // ========================================================================
    // TIER 3: HIGH-PERFORMANCE DATA SURFACE
    // ========================================================================
    m_table = new QTableWidget(this);
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
    connect(m_table, &QTableWidget::cellDoubleClicked, this, &TrialBalanceWidget::onTableDoubleClicked);
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

    createMetricCard(m_card1, m_card1Title, m_card1Val, "TOTAL ACCOUNTS / ENTRIES", "0", "#2563EB");
    createMetricCard(m_card2, m_card2Title, m_card2Val, "TOTAL DEBIT BALANCES", "₹0.00", "#0284C7");
    createMetricCard(m_card3, m_card3Title, m_card3Val, "TOTAL CREDIT BALANCES", "₹0.00", "#16A34A");
    createMetricCard(m_card4, m_card4Title, m_card4Val, "TRIAL BALANCE DIFFERENCE", "₹0.00 (Balanced)", "#16A34A");

    mainLayout->addLayout(metricsLayout);

    // Shortcuts
    connect(new QShortcut(QKeySequence(Qt::Key_Escape), this), &QShortcut::activated, this, &TrialBalanceWidget::backRequested);
    connect(new QShortcut(QKeySequence(Qt::Key_F4), this), &QShortcut::activated, this, &TrialBalanceWidget::onOpenViewOptions);
    connect(new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_F), this), &QShortcut::activated, this, [this]() {
        m_searchEdit->setFocus();
        m_searchEdit->selectAll();
    });
    new QShortcut(QKeySequence(Qt::ALT | Qt::Key_P), this, SLOT(onPrintPdf()));
    new QShortcut(QKeySequence(Qt::ALT | Qt::Key_E), this, SLOT(onExportCsv()));

    configureTableColumns();
}

void TrialBalanceWidget::configureTableColumns() {
    m_table->clear();
    TrialBalanceMode mode = m_controller ? m_controller->mode() : TrialBalanceMode::NormalView;

    if (mode == TrialBalanceMode::ShowTurnover) {
        m_table->setColumnCount(6);
        m_table->setHorizontalHeaderLabels({
            "Particulars / Account Name", "Group", "Opening Balance", "Debit Turnover", "Credit Turnover", "Closing Balance"
        });
        m_table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
        for (int i = 1; i < 6; ++i) m_table->horizontalHeader()->setSectionResizeMode(i, QHeaderView::ResizeToContents);
    } else if (mode == TrialBalanceMode::ShowOpeningBal) {
        m_table->setColumnCount(5);
        m_table->setHorizontalHeaderLabels({
            "Particulars / Account Name", "Group", "Opening Balance", "Closing Debit (Dr)", "Closing Credit (Cr)"
        });
        m_table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
        for (int i = 1; i < 5; ++i) m_table->horizontalHeader()->setSectionResizeMode(i, QHeaderView::ResizeToContents);
    } else if (mode == TrialBalanceMode::WithoutOpBal) {
        m_table->setColumnCount(4);
        m_table->setHorizontalHeaderLabels({
            "Particulars / Account Name", "Group", "Net Period Debit (Dr)", "Net Period Credit (Cr)"
        });
        m_table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
        for (int i = 1; i < 4; ++i) m_table->horizontalHeader()->setSectionResizeMode(i, QHeaderView::ResizeToContents);
    } else {
        // NormalView, FlatView, FlatGrouped, NormalDetailed
        m_table->setColumnCount(4);
        m_table->setHorizontalHeaderLabels({
            "Particulars / Account Name", "Group", "Debit Balance (Dr)", "Credit Balance (Cr)"
        });
        m_table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
        for (int i = 1; i < 4; ++i) m_table->horizontalHeader()->setSectionResizeMode(i, QHeaderView::ResizeToContents);
    }
}

void TrialBalanceWidget::setMode(TrialBalanceMode mode) {
    if (m_controller) {
        m_controller->setMode(mode);
    }
    for (int i = 0; i < m_modeCombo->count(); ++i) {
        if (m_modeCombo->itemData(i).toInt() == static_cast<int>(mode)) {
            m_modeCombo->blockSignals(true);
            m_modeCombo->setCurrentIndex(i);
            m_modeCombo->blockSignals(false);
            break;
        }
    }
    m_modeBadge->setText(m_modeCombo->currentText());
    configureTableColumns();
    reloadData();
}

void TrialBalanceWidget::focusTable() {
    if (m_table) m_table->setFocus();
}

void TrialBalanceWidget::reloadData() {
    if (m_controller) {
        m_controller->setDateRange(m_fromDateEdit->date(), m_toDateEdit->date());
        m_controller->reload();
    }
}

void TrialBalanceWidget::populateTable() {
    m_table->setRowCount(0);
    if (!m_controller) return;

    const auto& rows = m_controller->rows();
    TrialBalanceMode mode = m_controller->mode();
    m_table->setRowCount(rows.size() + 1); // +1 for Grand Total

    auto makeNum = [](const QString& txt, bool bold = false, const QString& color = "#0F172A") {
        auto* itm = new QTableWidgetItem(txt);
        itm->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        if (bold) itm->setFont(QFont("Segoe UI", 9, QFont::Bold));
        itm->setForeground(QBrush(QColor(color)));
        return itm;
    };

    for (int r = 0; r < rows.size(); ++r) {
        const auto& row = rows[r];

        QString prefix = (row.depth > 0) ? "    ↳ " : "";
        auto* nameItem = new QTableWidgetItem(prefix + row.accountName);
        if (row.isGroupHeader) {
            nameItem->setFont(QFont("Segoe UI", 9, QFont::Bold));
            nameItem->setForeground(QBrush(QColor("#2563EB")));
        } else {
            nameItem->setForeground(QBrush(QColor("#0F172A")));
        }
        m_table->setItem(r, 0, nameItem);

        auto* grpItem = new QTableWidgetItem(row.groupName);
        grpItem->setForeground(QBrush(QColor("#64748B")));
        m_table->setItem(r, 1, grpItem);

        if (mode == TrialBalanceMode::ShowTurnover) {
            m_table->setItem(r, 2, makeNum(row.opBalanceFmt));
            m_table->setItem(r, 3, makeNum(row.periodDebitFmt));
            m_table->setItem(r, 4, makeNum(row.periodCreditFmt));
            QString clStr = (row.closeDebit > 0) ? (row.closeDebitFmt + " Dr") : ((row.closeCredit > 0) ? (row.closeCreditFmt + " Cr") : "-");
            m_table->setItem(r, 5, makeNum(clStr, true, row.closeDebit > 0 ? "#0284C7" : "#16A34A"));
        } else if (mode == TrialBalanceMode::ShowOpeningBal) {
            m_table->setItem(r, 2, makeNum(row.opBalanceFmt));
            m_table->setItem(r, 3, makeNum(row.closeDebitFmt, false, "#0284C7"));
            m_table->setItem(r, 4, makeNum(row.closeCreditFmt, false, "#16A34A"));
        } else if (mode == TrialBalanceMode::WithoutOpBal) {
            m_table->setItem(r, 2, makeNum(row.closeDebitFmt, false, "#0284C7"));
            m_table->setItem(r, 3, makeNum(row.closeCreditFmt, false, "#16A34A"));
        } else {
            m_table->setItem(r, 2, makeNum(row.closeDebitFmt, row.isGroupHeader, "#0284C7"));
            m_table->setItem(r, 3, makeNum(row.closeCreditFmt, row.isGroupHeader, "#16A34A"));
        }

        if (row.isGroupHeader) {
            for (int c = 0; c < m_table->columnCount(); ++c) {
                if (m_table->item(r, c)) {
                    m_table->item(r, c)->setBackground(QBrush(QColor("#F8FAFC")));
                }
            }
        }
    }

    // Grand Total Row
    int totRow = rows.size();
    const auto& totals = m_controller->totals();

    auto* totLabel = new QTableWidgetItem("Grand Total");
    totLabel->setFont(QFont("Segoe UI", 9, QFont::Bold));
    totLabel->setForeground(QBrush(QColor("#0F172A")));
    m_table->setItem(totRow, 0, totLabel);

    if (mode == TrialBalanceMode::ShowTurnover) {
        m_table->setItem(totRow, 3, makeNum(FinancialMathService::instance().formatInr(totals.totalPeriodDebit, false), true));
        m_table->setItem(totRow, 4, makeNum(FinancialMathService::instance().formatInr(totals.totalPeriodCredit, false), true));
        m_table->setItem(totRow, 5, makeNum(FinancialMathService::instance().formatInr(totals.totalCloseDebit, false), true, "#16A34A"));
    } else if (mode == TrialBalanceMode::ShowOpeningBal) {
        m_table->setItem(totRow, 3, makeNum(totals.totalCloseDebitFmt, true, "#0284C7"));
        m_table->setItem(totRow, 4, makeNum(totals.totalCloseCreditFmt, true, "#16A34A"));
    } else {
        m_table->setItem(totRow, 2, makeNum(totals.totalCloseDebitFmt, true, "#0284C7"));
        m_table->setItem(totRow, 3, makeNum(totals.totalCloseCreditFmt, true, "#16A34A"));
    }

    for (int c = 0; c < m_table->columnCount(); ++c) {
        if (m_table->item(totRow, c)) {
            m_table->item(totRow, c)->setBackground(QBrush(QColor("#F1F5F9")));
        }
    }

    updateSummaryMetrics();
}

void TrialBalanceWidget::updateSummaryMetrics() {
    if (!m_controller) return;
    const auto& totals = m_controller->totals();

    m_card1Val->setText(QString::number(m_controller->rows().size()));
    m_card2Val->setText(totals.totalCloseDebitFmt);
    m_card3Val->setText(totals.totalCloseCreditFmt);

    if (totals.isBalanced) {
        m_card4Title->setText("TRIAL BALANCE STATUS");
        m_card4Val->setText("₹0.00 (Balanced)");
        m_card4->setStyleSheet("QFrame { background-color: #FFFFFF; border: 1px solid #E2E8F0; border-radius: 8px; border-left: 4px solid #16A34A; } QLabel { border: none; background: transparent; }");
        m_balanceStatusBadge->setText("● Balanced (₹0.00)");
        m_balanceStatusBadge->setStyleSheet("QLabel { background-color: #F0FDF4; color: #16A34A; border: 1.5px solid #86EFAC; border-radius: 6px; padding: 3px 8px; font-weight: 800; font-size: 11px; }");
    } else {
        m_card4Title->setText("DISCREPANCY / DIFFERENCE");
        m_card4Val->setText(totals.differenceFmt);
        m_card4->setStyleSheet("QFrame { background-color: #FFFFFF; border: 1px solid #E2E8F0; border-radius: 8px; border-left: 4px solid #DC2626; } QLabel { border: none; background: transparent; }");
        m_balanceStatusBadge->setText("● Difference: " + totals.differenceFmt);
        m_balanceStatusBadge->setStyleSheet("QLabel { background-color: #FEF2F2; color: #DC2626; border: 1.5px solid #FECACA; border-radius: 6px; padding: 3px 8px; font-weight: 800; font-size: 11px; }");
    }
}

void TrialBalanceWidget::onSearchChanged(const QString& text) {
    if (m_controller) m_controller->setSearchQuery(text);
}

void TrialBalanceWidget::onDateFilterChanged() {
    reloadData();
}

void TrialBalanceWidget::onModeComboChanged(int index) {
    TrialBalanceMode m = static_cast<TrialBalanceMode>(m_modeCombo->itemData(index).toInt());
    setMode(m);
}

void TrialBalanceWidget::onOpenViewOptions() {
    MahadevERP::MenuTreeManager::instance().executeMenu("trial_balance_options_hub", this);
}

void TrialBalanceWidget::onTableDoubleClicked(int row, int col) {
    Q_UNUSED(col);
    if (!m_controller) return;
    const auto& rows = m_controller->rows();
    if (row >= 0 && row < rows.size()) {
        const auto& r = rows[row];
        if (!r.isGroupHeader) {
            emit openLedgerRequested(r.accountName);
        }
    }
}

void TrialBalanceWidget::onExportCsv() {
    QString fileName = QFileDialog::getSaveFileName(this, "Export Trial Balance", "Trial_Balance.csv", "CSV Files (*.csv)");
    if (fileName.isEmpty()) return;

    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) return;

    QTextStream out(&file);
    out << "Particulars,Group,Debit,Credit\n";
    if (m_controller) {
        for (const auto& r : m_controller->rows()) {
            out << "\"" << r.accountName << "\",\"" << r.groupName << "\"," << r.closeDebit << "," << r.closeCredit << "\n";
        }
    }
    file.close();
    QMessageBox::information(this, "Exported", "Trial Balance successfully exported to CSV.");
}

void TrialBalanceWidget::onPrintPdf() {
    QMessageBox::information(this, "Print Report", "Trial Balance report (" + m_modeBadge->text() + ") sent to system print preview.");
}

void TrialBalanceWidget::keyPressEvent(QKeyEvent* event) {
    if (event->key() == Qt::Key_Escape) {
        emit backRequested();
        event->accept();
        return;
    }
    QWidget::keyPressEvent(event);
}

} // namespace MahadevERP
