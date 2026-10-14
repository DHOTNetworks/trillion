#include "trial_balance_widget.h"
#include "voucher_date_dialog.h"
#include "kbd_badge_button.h"
#include "erp_theme.h"
#include "custom_dialogs.h"
#include "../engine/fiscal_year_helper.h"
#include "../services/financial_math_service.h"
#include "../models/menu_tree_manager.h"
#include "reports/specs/trial_balance_report.h"
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

    {
        QSignalBlocker b1(m_fromDateEdit);
        QSignalBlocker b2(m_toDateEdit);
        FiscalYearInfo activeFy = FiscalYearHelper::getActiveFiscalYear();
        m_fromDateEdit->setDate(QDate::fromString(activeFy.startDate, "yyyy-MM-dd"));
        m_toDateEdit->setDate(QDate::fromString(activeFy.endDate, "yyyy-MM-dd"));
    }
}

void TrialBalanceWidget::setupUi() {
    // Standard page scaffold (ErpTheme): slate palette + T1-T4 margins/spacing.
    auto* mainLayout = ErpTheme::makePageLayout(this);

    // TIER 1: HEADER BAR CARD (ErpTheme builder; title contract from ReportSpec)
    auto* headerCard = ErpTheme::makeHeaderCard(this,
        ErpReports::trialBalanceSpec()->title, ErpReports::trialBalanceSpec()->subtitle,
        &m_titleLabel, &m_subtitleLabel);
    auto* headerLayout = qobject_cast<QHBoxLayout*>(headerCard->layout());

    m_modeBadge = new QLabel("Normal View", headerCard);
    m_modeBadge->setStyleSheet(ErpTheme::badgeStyle("info"));
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

    // TIER 2: FILTER & CONTROL BAR CARD (ErpTheme builder, spec height 52)
    QHBoxLayout* filterLayout = nullptr;
    auto* filterCard = ErpTheme::makeFilterBar(this, &filterLayout);

    filterLayout->addWidget(new QLabel("From Date:", filterCard));
    m_fromDateEdit = new AccountingDateDisplay(filterCard);
    m_fromDateEdit->setMinimumWidth(105);
    connect(m_fromDateEdit, &AccountingDateDisplay::dateChanged, this, &TrialBalanceWidget::onDateFilterChanged);
    filterLayout->addWidget(m_fromDateEdit);

    filterLayout->addWidget(new QLabel("To Date:", filterCard));
    m_toDateEdit = new AccountingDateDisplay(filterCard);
    m_toDateEdit->setMinimumWidth(105);
    connect(m_toDateEdit, &AccountingDateDisplay::dateChanged, this, &TrialBalanceWidget::onDateFilterChanged);
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
    periodBtn->setStyleSheet(ErpTheme::secondaryButtonStyle());
    connect(periodBtn, &QPushButton::clicked, this, openPeriodDlg);
    filterLayout->addWidget(periodBtn);

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
    m_balanceStatusBadge->setStyleSheet(ErpTheme::badgeStyle("ok"));
    filterLayout->addWidget(m_balanceStatusBadge);

    mainLayout->addWidget(filterCard);

    // TIER 3: HIGH-PERFORMANCE DATA SURFACE (ErpTheme: navy header, 29px rows,
    // SelectRows/Single, explicit foregrounds per cell in populateTable)
    m_table = new QTableWidget(this);
    ErpTheme::styleTable(m_table);
    m_table->horizontalHeader()->setDefaultAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    m_table->setShowGrid(true);
    connect(m_table, &QTableWidget::cellDoubleClicked, this, &TrialBalanceWidget::onTableDoubleClicked);
    // Enter opens the focused row's ledger (same as double-click).
    // Scoped to the grid so search boxes and buttons keep their own keys.
    auto openFocusedRow = [this]() {
        if (!m_table || m_table->currentRow() < 0) return;
        onTableDoubleClicked(m_table->currentRow(), m_table->currentColumn());
    };
    auto* enterShortcut = new QShortcut(QKeySequence(Qt::Key_Return), m_table);
    enterShortcut->setContext(Qt::WidgetWithChildrenShortcut);
    connect(enterShortcut, &QShortcut::activated, this, openFocusedRow);
    auto* numEnterShortcut = new QShortcut(QKeySequence(Qt::Key_Enter), m_table);
    numEnterShortcut->setContext(Qt::WidgetWithChildrenShortcut);
    connect(numEnterShortcut, &QShortcut::activated, this, openFocusedRow);
    mainLayout->addWidget(m_table, 1);

    // TIER 4: SUMMARY METRICS FOOTER CARDS (ErpTheme builders, 4px accent stripe)
    auto* metricsLayout = new QHBoxLayout();
    metricsLayout->setSpacing(10);

    m_card1Val = ErpTheme::makeMetricCard(this, metricsLayout, "TOTAL ACCOUNTS / ENTRIES", "0", ErpTheme::kBlue, &m_card1, &m_card1Title);
    m_card2Val = ErpTheme::makeMetricCard(this, metricsLayout, "TOTAL DEBIT BALANCES", "₹0.00", ErpTheme::kSky, &m_card2, &m_card2Title);
    m_card3Val = ErpTheme::makeMetricCard(this, metricsLayout, "TOTAL CREDIT BALANCES", "₹0.00", ErpTheme::kGreen, &m_card3, &m_card3Title);
    m_card4Val = ErpTheme::makeMetricCard(this, metricsLayout, "TRIAL BALANCE DIFFERENCE", "₹0.00 (Balanced)", ErpTheme::kGreen, &m_card4, &m_card4Title);

    mainLayout->addLayout(metricsLayout);

    // Shortcuts
    connect(new QShortcut(QKeySequence(Qt::Key_Escape), this), &QShortcut::activated, this, &TrialBalanceWidget::backRequested);
    connect(new QShortcut(QKeySequence(Qt::Key_F2), this), &QShortcut::activated, this, openPeriodDlg);
    connect(new QShortcut(QKeySequence(Qt::ALT | Qt::Key_X), this), &QShortcut::activated, this, &TrialBalanceWidget::onOpenViewOptions);
    connect(new QShortcut(QKeySequence(Qt::ALT | Qt::Key_Q), this), &QShortcut::activated, this, [this]() {
        m_searchEdit->setFocus();
        m_searchEdit->selectAll();
    });
    new QShortcut(QKeySequence(Qt::ALT | Qt::Key_P), this, SLOT(onPrintPdf()));
    new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_P), this, SLOT(onExportPdf()));
    new QShortcut(QKeySequence(Qt::ALT | Qt::Key_O), this, SLOT(onExportOdf()));
    new QShortcut(QKeySequence(Qt::ALT | Qt::Key_E), this, SLOT(onExportExcel()));

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

    // Align header items: Col 0 & 1 Left-aligned, numeric columns Right-aligned
    for (int col = 0; col < m_table->columnCount(); ++col) {
        if (auto* itm = m_table->horizontalHeaderItem(col)) {
            if (col == 0 || col == 1) {
                itm->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
            } else {
                itm->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
            }
        }
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
    if (!m_table) return;
    if (m_table->rowCount() > 0 && m_table->currentRow() < 0) m_table->selectRow(0);
    m_table->setFocus();
}

void TrialBalanceWidget::reloadData() {
    if (m_controller) {
        m_controller->setDateRange(m_fromDateEdit->date(), m_toDateEdit->date());
        m_controller->reload();
    }
    m_isDirty = false;
}

void TrialBalanceWidget::populateTable() {
    m_table->setRowCount(0);
    if (!m_controller) return;

    const auto& rows = m_controller->rows();
    TrialBalanceMode mode = m_controller->mode();
    m_table->setRowCount(rows.size() + 1); // +1 for Grand Total

    auto makeNum = [](const QString& txt, bool = false, const QString& color = "#0F172A") {
        auto* itm = new QTableWidgetItem(txt);
        itm->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        itm->setForeground(QBrush(QColor(color)));
        return itm;
    };

    for (int r = 0; r < rows.size(); ++r) {
        const auto& row = rows[r];

        QString prefix = (row.depth > 0) ? "    ↳ " : "";
        auto* nameItem = new QTableWidgetItem(prefix + row.accountName);
        if (row.isGroupHeader) {
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
        ErpTheme::restripeCard(m_card4, ErpTheme::kGreen);
        m_balanceStatusBadge->setText("● Balanced (₹0.00)");
        m_balanceStatusBadge->setStyleSheet(ErpTheme::badgeStyle("ok"));
    } else {
        m_card4Title->setText("DISCREPANCY / DIFFERENCE");
        m_card4Val->setText(totals.differenceFmt);
        ErpTheme::restripeCard(m_card4, ErpTheme::kRed);
        m_balanceStatusBadge->setText("● Difference: " + totals.differenceFmt);
        m_balanceStatusBadge->setStyleSheet(ErpTheme::badgeStyle("danger"));
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

void TrialBalanceWidget::onExportPdf() {
    if (!m_printExportCtrl) return;
    QString asOnDate = m_toDateEdit ? m_toDateEdit->date().toString("yyyy-MM-dd") : "";
    int mode = (m_modeBadge && m_modeBadge->text().contains("Hierarchy")) ? 1 : 0;
    m_printExportCtrl->export_trial_balance_pdf(asOnDate, mode);
}

void TrialBalanceWidget::onExportOdf() {
    if (!m_printExportCtrl) return;
    QString asOnDate = m_toDateEdit ? m_toDateEdit->date().toString("yyyy-MM-dd") : "";
    int mode = (m_modeBadge && m_modeBadge->text().contains("Hierarchy")) ? 1 : 0;
    m_printExportCtrl->export_trial_balance_odf(asOnDate, mode);
}

void TrialBalanceWidget::onExportExcel() {
    if (!m_printExportCtrl) return;
    QString asOnDate = m_toDateEdit ? m_toDateEdit->date().toString("yyyy-MM-dd") : "";
    int mode = (m_modeBadge && m_modeBadge->text().contains("Hierarchy")) ? 1 : 0;
    m_printExportCtrl->export_trial_balance_excel(asOnDate, mode);
}

void TrialBalanceWidget::onPrintPdf() {
    if (!m_printExportCtrl) return;
    QString asOnDate = m_toDateEdit ? m_toDateEdit->date().toString("yyyy-MM-dd") : "";
    int mode = (m_modeBadge && m_modeBadge->text().contains("Hierarchy")) ? 1 : 0;
    m_printExportCtrl->print_trial_balance(asOnDate, mode);
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
