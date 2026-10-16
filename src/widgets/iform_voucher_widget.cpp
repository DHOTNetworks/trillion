#include "iform_voucher_widget.h"
#include "kbd_badge_button.h"
#include "erp_theme.h"
#include "voucher_common.h"
#include "voucher_date_dialog.h"
#include "custom_dialogs.h"
#include "engine/accounting_engine.h"
#include "engine/fiscal_year_helper.h"
#include "database_manager.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QHeaderView>
#include <QKeyEvent>
#include <QMessageBox>
#include <QApplication>
#include <QBrush>
#include <QDate>
#include <QLocale>
#include <QTimer>

IFormVoucherWidget::IFormVoucherWidget(PrintExportController* printExportCtrl, QWidget* parent)
    : QWidget(parent)
    , m_printExportCtrl(printExportCtrl)
{
    setAttribute(Qt::WA_StyledBackground, true);
    setAutoFillBackground(true);
    setupUi();
    setupNavigationChains();
    applyCustomStyles();
    resetForm();
}

IFormVoucherWidget::~IFormVoucherWidget() = default;

void IFormVoucherWidget::setupUi() {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(12, 10, 12, 10);
    mainLayout->setSpacing(8);

    // 1. Top Bar
    auto* topBarFrame = new QFrame(this);
    topBarFrame->setObjectName("topBarFrame");
    auto* topBarLayout = new QHBoxLayout(topBarFrame);
    topBarLayout->setContentsMargins(8, 6, 8, 6);

    m_badgeLabel = new QLabel("I-FORM MANDI BUYER ISSUE", this);
    m_badgeLabel->setObjectName("voucherBadgePurple");

    m_voucherNoDisplay = new QLabel("Vch: IFrm-1", this);
    m_voucherNoDisplay->setObjectName("headerInfoLabel");

    auto* iformNoLabel = new QLabel("I-Form No:", this);
    m_iformNoEdit = new QLineEdit("1", this);
    m_iformNoEdit->setMinimumWidth(80);
    // Auto-generated and non-editable on both new and alter pages:
    // mouse-only, never a Tab/Enter stop.
    m_iformNoEdit->setReadOnly(true);
    m_iformNoEdit->setFocusPolicy(Qt::ClickFocus);

    auto* dateLabel = new QLabel("Date (F2):", this);
    m_dateEdit = new AccountingDateEdit(this);
    m_dayOfWeekLabel = new QLabel(this);
    m_dayOfWeekLabel->setObjectName("dayOfWeekLabel");

    m_fyBadgeLabel = new QLabel(this);
    m_fyBadgeLabel->setObjectName("fyBadge");

    topBarLayout->addWidget(m_badgeLabel);
    topBarLayout->addSpacing(16);
    topBarLayout->addWidget(m_voucherNoDisplay);
    topBarLayout->addSpacing(12);
    topBarLayout->addWidget(iformNoLabel);
    topBarLayout->addWidget(m_iformNoEdit);
    topBarLayout->addSpacing(16);
    topBarLayout->addWidget(dateLabel);
    topBarLayout->addWidget(m_dateEdit);
    topBarLayout->addWidget(m_dayOfWeekLabel);
    topBarLayout->addStretch();
    topBarLayout->addWidget(m_fyBadgeLabel);

    mainLayout->addWidget(topBarFrame);

    // 2. Buyer & Header Info
    auto* headerBox = new QGroupBox("Buyer (Manufacturer / Trader) & Dispatch Details", this);
    auto* headerLayout = new QGridLayout(headerBox);
    headerLayout->setContentsMargins(8, 6, 8, 6);
    headerLayout->setSpacing(8);

    headerLayout->addWidget(new QLabel("Buyer Name (Enter to Search / F3 New):", this), 0, 0);
    m_buyerSearch = new AccountSearchBox(this);
    headerLayout->addWidget(m_buyerSearch, 0, 1, 1, 2);

    m_buyerBalanceLabel = new QLabel("Date Bal. 0.00", this);
    m_buyerBalanceLabel->setObjectName("balanceLabel");
    headerLayout->addWidget(m_buyerBalanceLabel, 0, 3);

    headerLayout->addWidget(new QLabel("Broker / Commission Agent:", this), 1, 0);
    m_brokerNameEdit = new AccountSearchBox(this);
    m_brokerNameEdit->configureForBrokerSearch();
    headerLayout->addWidget(m_brokerNameEdit, 1, 1);

    headerLayout->addWidget(new QLabel("Due Days:", this), 1, 2);
    m_dueDaysEdit = new QLineEdit("0", this);
    m_dueDaysEdit->setMinimumWidth(60);
    headerLayout->addWidget(m_dueDaysEdit, 1, 3);

    // Row 2: Vehicle & GR No
    headerLayout->addWidget(new QLabel("Vehicle No:", this), 2, 0);
    m_vehicleNoEdit = new QLineEdit(this);
    VoucherCommon::setupUppercaseInput(m_vehicleNoEdit);
    headerLayout->addWidget(m_vehicleNoEdit, 2, 1);

    headerLayout->addWidget(new QLabel("GR / Bilty No:", this), 2, 2);
    m_grNoEdit = new QLineEdit(this);
    headerLayout->addWidget(m_grNoEdit, 2, 3);

    mainLayout->addWidget(headerBox);

    // 3. Line Items Table
    m_table = new QTableWidget(1, 9, this);
    m_table->setHorizontalHeaderLabels({
        "Item Description", "Bags", "Weight (Qtl)", "Rate (₹/Qtl)",
        "Goods Amt (₹)", "Dami (2.5%)", "M.Fee (2%)", "HRDF (0.5%)", "Line Total (₹)"
    });
    // Sale-voucher grid contract: the item column stretches, numeric
    // columns hold fixed widths (no ResizeToContents jumping).
    m_table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    for (int c = 1; c < 9; ++c) {
        m_table->horizontalHeader()->setSectionResizeMode(c, QHeaderView::Fixed);
    }
    m_table->setColumnWidth(1, 60);
    m_table->setColumnWidth(2, 85);
    m_table->setColumnWidth(3, 95);
    m_table->setColumnWidth(4, 110);
    m_table->setColumnWidth(5, 90);
    m_table->setColumnWidth(6, 90);
    m_table->setColumnWidth(7, 90);
    m_table->setColumnWidth(8, 115);
    m_table->verticalHeader()->setVisible(false);
    m_table->horizontalHeader()->setHighlightSections(false);
    m_table->setAlternatingRowColors(true);
    m_table->setSelectionBehavior(QAbstractItemView::SelectItems);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table->setShowGrid(true);

    // Table-wide delegate (sale pattern): every cell — item search or plain
    // numeric editor — gets the same Enter/Backspace/Left/Right/PgUp/PgDn
    // contract. Column-scoped delegates leave numeric cells on default keys.
    m_itemDelegate = new ItemSearchDelegate(this);
    m_table->setItemDelegate(m_itemDelegate);

    mainLayout->addWidget(m_table, 1);

    // 4. Calculations Summary Frame
    auto* summaryFrame = new QFrame(this);
    summaryFrame->setObjectName("summaryFrame");
    auto* summaryLayout = new QGridLayout(summaryFrame);
    summaryLayout->setContentsMargins(8, 6, 8, 6);
    summaryLayout->setSpacing(8);

    summaryLayout->addWidget(new QLabel("Total Bags:", this), 0, 0);
    m_totalBagsLabel = new QLabel("0", this);
    m_totalBagsLabel->setObjectName("boldValue");
    summaryLayout->addWidget(m_totalBagsLabel, 0, 1);

    summaryLayout->addWidget(new QLabel("Total Weight:", this), 0, 2);
    m_totalWeightLabel = new QLabel("0.000 Qtl", this);
    m_totalWeightLabel->setObjectName("boldValue");
    summaryLayout->addWidget(m_totalWeightLabel, 0, 3);

    summaryLayout->addWidget(new QLabel("Goods Amount:", this), 0, 4);
    m_goodsAmountLabel = new QLabel("₹ 0.00", this);
    m_goodsAmountLabel->setObjectName("boldValue");
    summaryLayout->addWidget(m_goodsAmountLabel, 0, 5);

    // Row 1: Levies (Dami, Mandi Fee, HRDF)
    summaryLayout->addWidget(new QLabel("Total Dami (2.5%):", this), 1, 0);
    m_damiLabel = new QLabel("₹ 0.00", this);
    m_damiLabel->setObjectName("boldValue");
    summaryLayout->addWidget(m_damiLabel, 1, 1);

    summaryLayout->addWidget(new QLabel("Mandi Fee (2%):", this), 1, 2);
    m_mandiFeeLabel = new QLabel("₹ 0.00", this);
    m_mandiFeeLabel->setObjectName("boldValue");
    summaryLayout->addWidget(m_mandiFeeLabel, 1, 3);

    summaryLayout->addWidget(new QLabel("HRDF (0.5%):", this), 1, 4);
    m_hrdfLabel = new QLabel("₹ 0.00", this);
    m_hrdfLabel->setObjectName("boldValue");
    summaryLayout->addWidget(m_hrdfLabel, 1, 5);

    // Row 2: Labour, Round Off, Grand Total
    summaryLayout->addWidget(new QLabel("Labour / Hamali (+):", this), 2, 0);
    m_labourEdit = new QLineEdit("0.00", this);
    summaryLayout->addWidget(m_labourEdit, 2, 1);

    summaryLayout->addWidget(new QLabel("Round Off:", this), 2, 2);
    m_roundOffEdit = new QLineEdit("0.00", this);
    summaryLayout->addWidget(m_roundOffEdit, 2, 3);

    summaryLayout->addWidget(new QLabel("Grand Total Receivable:", this), 2, 4);
    m_grandTotalLabel = new QLabel("₹ 0.00", this);
    m_grandTotalLabel->setObjectName("grandTotalLabel");
    summaryLayout->addWidget(m_grandTotalLabel, 2, 5);

    // Row 3: Narration
    summaryLayout->addWidget(new QLabel("Narration:", this), 3, 0);
    m_narrationEdit = new QLineEdit(this);
    summaryLayout->addWidget(m_narrationEdit, 3, 1, 1, 5);

    mainLayout->addWidget(summaryFrame);

    // 5. Bottom Action Bar
    auto* actionLayout = new QHBoxLayout();
    actionLayout->setSpacing(8);

    m_statusLabel = new QLabel(this);
    actionLayout->addWidget(m_statusLabel);
    actionLayout->addStretch();

    m_dateButton = new KbdBadgeButton("Change Date", "F2", this);
    m_saveButton = new KbdBadgeButton("Save I-Form", "Ctrl+S", this);
    m_saveButton->setPrimaryColor(ErpTheme::kBlue, ErpTheme::kBlueDark);
    m_saveButton->setTextColor(ErpTheme::kPaper);
    m_deleteButton = new KbdBadgeButton("Delete", "Ctrl+D", this);
    m_deleteButton->setPrimaryColor(ErpTheme::kRedBg, ErpTheme::kRedBorder);
    m_deleteButton->setTextColor(ErpTheme::kRed);
    m_deleteButton->setEnabled(false);
    m_printButton = new KbdBadgeButton("Print", "Ctrl+P", this);
    m_cancelButton = new KbdBadgeButton("Cancel", "Esc", this);

    actionLayout->addWidget(m_dateButton);
    actionLayout->addWidget(m_saveButton);
    actionLayout->addWidget(m_deleteButton);
    actionLayout->addWidget(m_printButton);
    actionLayout->addWidget(m_cancelButton);

    mainLayout->addLayout(actionLayout);

    // Connect signals
    connect(m_dateEdit, &AccountingDateEdit::dateChanged, this, &IFormVoucherWidget::onDateChanged);
    connect(m_buyerSearch, &AccountSearchBox::partyDataSelected, this, &IFormVoucherWidget::onBuyerSelected);
    connect(m_table, &QTableWidget::cellChanged, this, &IFormVoucherWidget::onTableCellChanged);
    connect(m_itemDelegate, &ItemSearchDelegate::stockItemConfigured, this, &IFormVoucherWidget::onStockItemConfigured);
    connect(m_itemDelegate, &ItemSearchDelegate::moveNextRequested, this, &IFormVoucherWidget::advanceCell);
    connect(m_itemDelegate, &ItemSearchDelegate::movePrevRequested, this, &IFormVoucherWidget::retreatCell);
    connect(m_itemDelegate, &ItemSearchDelegate::browsePrevRequested, this, &IFormVoucherWidget::loadPreviousVoucher);
    connect(m_itemDelegate, &ItemSearchDelegate::browseNextRequested, this, &IFormVoucherWidget::loadNextVoucher);
    connect(m_labourEdit, &QLineEdit::textChanged, this, &IFormVoucherWidget::recalculateTotals);
    connect(m_roundOffEdit, &QLineEdit::textChanged, this, &IFormVoucherWidget::recalculateTotals);

    connect(m_dateButton, &QPushButton::clicked, this, &IFormVoucherWidget::openDateDialog);
    connect(m_saveButton, &QPushButton::clicked, this, &IFormVoucherWidget::saveVoucher);
    connect(m_deleteButton, &QPushButton::clicked, this, &IFormVoucherWidget::deleteVoucher);
    connect(m_printButton, &QPushButton::clicked, this, &IFormVoucherWidget::printVoucher);
    connect(m_cancelButton, &QPushButton::clicked, this, [this]() { emit backRequested(); });
}

void IFormVoucherWidget::setupNavigationChains() {
    // Qt tab order (keyboard Tab follows the same visual order).
    setTabOrder(m_buyerSearch, m_brokerNameEdit);
    setTabOrder(m_brokerNameEdit, m_dueDaysEdit);
    setTabOrder(m_dueDaysEdit, m_vehicleNoEdit);
    setTabOrder(m_vehicleNoEdit, m_grNoEdit);
    setTabOrder(m_grNoEdit, m_table);
    setTabOrder(m_table, m_labourEdit);
    setTabOrder(m_labourEdit, m_roundOffEdit);
    setTabOrder(m_roundOffEdit, m_narrationEdit);
    setTabOrder(m_narrationEdit, m_saveButton);

    // Sale-voucher pattern: single Enter-forward declaration per field
    // (Backspace/Left derive backward automatically, popup-aware).
    auto go = [](QWidget* w) {
        if (!w) return;
        w->setFocus();
        if (auto* le = qobject_cast<QLineEdit*>(w)) le->selectAll();
        else if (auto* asb = qobject_cast<AccountSearchBox*>(w)) asb->selectAll();
    };
    auto stepBackToGrid = [this]() {
        if (!m_table || m_table->rowCount() == 0) return;
        m_table->setCurrentCell(m_table->rowCount() - 1, 3);
        m_table->edit(m_table->currentIndex());
    };
    VoucherCommon::installGridNavigation(m_buyerSearch, nullptr, m_brokerNameEdit,
        [this, go]() { go(m_brokerNameEdit); });
    VoucherCommon::installGridNavigation(m_brokerNameEdit, m_buyerSearch, m_dueDaysEdit,
        [this, go]() { go(m_dueDaysEdit); });
    VoucherCommon::installGridNavigation(m_dueDaysEdit, m_brokerNameEdit, m_vehicleNoEdit,
        [this, go]() { go(m_vehicleNoEdit); });
    VoucherCommon::installGridNavigation(m_vehicleNoEdit, m_dueDaysEdit, m_grNoEdit,
        [this, go]() { go(m_grNoEdit); });
    VoucherCommon::installGridNavigation(m_grNoEdit, m_vehicleNoEdit, nullptr,
        [this]() {
            if (m_table->rowCount() == 0) addNewLineRow();
            focusTableAt(0, 0);
        });
    VoucherCommon::installGridNavigation(m_labourEdit, nullptr, m_roundOffEdit,
        [this, go]() { go(m_roundOffEdit); }, stepBackToGrid);
    VoucherCommon::installGridNavigation(m_roundOffEdit, m_labourEdit, m_narrationEdit,
        [this, go]() { go(m_narrationEdit); });
    VoucherCommon::installGridNavigation(m_narrationEdit, m_roundOffEdit, nullptr,
        [this]() { if (m_saveButton) m_saveButton->setFocus(); });

    // Global keys (F2/F4/PgUp/PgDn/Ctrl+S/D/P/E, Esc). The grid filters
    // above ignore function keys, so both coexist peacefully.
    const QList<QWidget*> inputs = {
        m_buyerSearch, m_brokerNameEdit, m_dueDaysEdit,
        m_vehicleNoEdit, m_grNoEdit, m_table, m_labourEdit,
        m_roundOffEdit, m_narrationEdit, m_dateButton,
        m_saveButton, m_deleteButton, m_printButton, m_cancelButton
    };
    for (QWidget* w : inputs) {
        if (w) w->installEventFilter(this);
    }
}

void IFormVoucherWidget::applyCustomStyles() {
    setStyleSheet(
        "IFormVoucherWidget {"
        "  background-color: #F8FAFC;"
        "}"
        "QFrame#topBarFrame, QFrame#summaryFrame {"
        "  background-color: #FFFFFF;"
        "  border: 1px solid #E2E8F0;"
        "  border-radius: 8px;"
        "}"
        "QGroupBox {"
        "  background-color: #FFFFFF;"
        "  border: 1px solid #E2E8F0;"
        "  border-radius: 8px;"
        "  margin-top: 14px;"
        "  padding-top: 14px;"
        "  font-weight: 700;"
        "  font-size: 11px;"
        "  color: #475569;"
        "}"
        "QGroupBox::title {"
        "  subcontrol-origin: margin;"
        "  subcontrol-position: top left;"
        "  left: 12px;"
        "  padding: 0 6px;"
        "  background-color: #FFFFFF;"
        "  color: #1E293B;"
        "  font-weight: 800;"
        "  font-size: 12px;"
        "}"
        "QLabel {"
        "  color: #334155;"
        "  font-size: 11.5px;"
        "  font-weight: 600;"
        "  background: transparent;"
        "}"
        "QLineEdit {"
        "  background-color: #FFFFFF;"
        "  color: #0F172A;"
        "  border: 1px solid #CBD5E1;"
        "  border-radius: 5px;"
        "  padding: 4px 8px;"
        "  font-size: 12px;"
        "  font-weight: 600;"
        "}"
        "QLineEdit:focus {"
        "  border: 1.5px solid #7C3AED;"
        "  background-color: #FAF5FF;"
        "}"
        "QLineEdit:disabled, QLineEdit:read-only {"
        "  background-color: #F8FAFC;"
        "  color: #64748B;"
        "  border: 1px solid #E2E8F0;"
        "}"
        "QComboBox {"
        "  background-color: #FFFFFF;"
        "  color: #0F172A;"
        "  border: 1px solid #CBD5E1;"
        "  border-radius: 5px;"
        "  padding: 4px 8px;"
        "  font-size: 12px;"
        "  font-weight: 600;"
        "}"
        "QComboBox:focus {"
        "  border: 1.5px solid #7C3AED;"
        "  background-color: #FAF5FF;"
        "}"
        "QComboBox::drop-down {"
        "  border: none;"
        "  width: 22px;"
        "}"
        "QComboBox QAbstractItemView {"
        "  background-color: #FFFFFF;"
        "  color: #0F172A;"
        "  selection-background-color: #7C3AED;"
        "  selection-color: #FFFFFF;"
        "  border: 1px solid #CBD5E1;"
        "  outline: none;"
        "}"
        "AccountingDateEdit, QDateEdit {"
        "  background-color: #FFFFFF;"
        "  color: #0F172A;"
        "  border: 1px solid #CBD5E1;"
        "  border-radius: 5px;"
        "  padding: 4px 8px;"
        "  font-size: 12px;"
        "  font-weight: 600;"
        "}"
        "AccountingDateEdit:focus, QDateEdit:focus {"
        "  border: 1.5px solid #7C3AED;"
        "  background-color: #FAF5FF;"
        "}"
        "QTableWidget {"
        "  background-color: #FFFFFF;"
        "  alternate-background-color: #F8FAFC;"
        "  color: #0F172A;"
        "  gridline-color: #E2E8F0;"
        "  border: 1px solid #CBD5E1;"
        "  border-radius: 6px;"
        "  selection-background-color: #2563EB;"
        "  selection-color: #FFFFFF;"
        "  font-size: 12px;"
        "}"
        "QHeaderView::section {"
        "  background-color: #0F172A;"
        "  color: #FFFFFF;"
        "  font-weight: 700;"
        "  font-size: 11px;"
        "  padding: 6px 4px;"
        "  border: none;"
        "  border-right: 1px solid #CBD5E1;"
        "  border-bottom: 1px solid #CBD5E1;"
        "}"
        "QTableCornerButton::section {"
        "  background-color: #F1F5F9;"
        "  border: none;"
        "  border-right: 1px solid #CBD5E1;"
        "  border-bottom: 1px solid #CBD5E1;"
        "}"
        "QScrollBar:vertical {"
        "  background: #F1F5F9;"
        "  width: 10px;"
        "  margin: 0px;"
        "  border-radius: 4px;"
        "}"
        "QScrollBar::handle:vertical {"
        "  background: #CBD5E1;"
        "  min-height: 20px;"
        "  border-radius: 4px;"
        "}"
        "QScrollBar::handle:vertical:hover {"
        "  background: #94A3B8;"
        "}"
        "QScrollBar:horizontal {"
        "  background: #F1F5F9;"
        "  height: 10px;"
        "  margin: 0px;"
        "  border-radius: 4px;"
        "}"
        "QScrollBar::handle:horizontal {"
        "  background: #CBD5E1;"
        "  min-width: 20px;"
        "  border-radius: 4px;"
        "}"
        "QScrollBar::handle:horizontal:hover {"
        "  background: #94A3B8;"
        "}"
        "QPushButton {"
        "  background-color: #FFFFFF;"
        "  color: #334155;"
        "  border: 1px solid #CBD5E1;"
        "  border-radius: 5px;"
        "  padding: 6px 14px;"
        "  font-weight: 700;"
        "  font-size: 11.5px;"
        "}"
        "QPushButton:hover {"
        "  background-color: #F1F5F9;"
        "  border-color: #94A3B8;"
        "}"
        "QPushButton:pressed {"
        "  background-color: #E2E8F0;"
        "}"
        "#voucherBadgePurple {"
        "  background-color: #7C3AED;"
        "  color: #FFFFFF;"
        "  font-weight: 800;"
        "  font-size: 12px;"
        "  padding: 5px 12px;"
        "  border-radius: 5px;"
        "  letter-spacing: 0.5px;"
        "}"
        "#headerInfoLabel {"
        "  font-size: 12px;"
        "  font-weight: 700;"
        "  color: #1E293B;"
        "  background-color: #F1F5F9;"
        "  border: 1px solid #CBD5E1;"
        "  border-radius: 4px;"
        "  padding: 4px 10px;"
        "}"
        "#fyBadge {"
        "  background-color: #FAF5FF;"
        "  color: #7C3AED;"
        "  font-weight: 700;"
        "  font-size: 11.5px;"
        "  padding: 4px 10px;"
        "  border: 1px solid #E9D5FF;"
        "  border-radius: 5px;"
        "}"
        "#balanceLabel {"
        "  font-weight: 800;"
        "  color: #0369A1;"
        "  font-size: 11.5px;"
        "  background-color: #F0F9FF;"
        "  border: 1px solid #BAE6FD;"
        "  border-radius: 4px;"
        "  padding: 3px 8px;"
        "}"
        "#boldValue {"
        "  font-weight: 800;"
        "  font-size: 12px;"
        "  color: #0F172A;"
        "}"
        "#grandTotalLabel {"
        "  font-weight: 900;"
        "  font-size: 16px;"
        "  color: #7C3AED;"
        "  background-color: #FAF5FF;"
        "  border: 1px solid #E9D5FF;"
        "  border-radius: 6px;"
        "  padding: 4px 12px;"
        "}"
    );
}

void IFormVoucherWidget::resetForm() {
    m_editingVoucherId = 0;
    m_editingIFormNo.clear();
    m_editingVoucherNo.clear();
    m_hasInitialDateOpened = false;
    m_deleteButton->setEnabled(false);

    // Strictly inside the selected fiscal year (sale pattern): today when
    // it falls in-year, otherwise the year's first day.
    QDate cur = QDate::currentDate();
    FiscalYearInfo afy = FiscalYearHelper::getActiveFiscalYear();
    QDate fyStart = QDate::fromString(afy.startDate, "yyyy-MM-dd");
    QDate fyEnd = QDate::fromString(afy.endDate, "yyyy-MM-dd");
    if (afy.isValid() && fyStart.isValid() && fyEnd.isValid()
        && (cur < fyStart || cur > fyEnd)) {
        cur = fyStart;
    }
    m_dateEdit->setDate(cur);
    updateDayOfWeek(cur);
    updateFiscalYearBadge();
    updateNextNumbers();

    m_buyerSearch->clear();
    m_buyerBalanceLabel->setText("Date Bal. 0.00");
    m_selectedBuyerId = 0;
    m_selectedBuyerName.clear();

    m_dueDaysEdit->setText("0");
    m_vehicleNoEdit->clear();
    m_grNoEdit->clear();
    m_brokerNameEdit->clear();
    m_narrationEdit->clear();

    m_table->setRowCount(0);
    addNewLineRow();
    recalculateTotals();
    setStatusMessage("Ready for new I-Form entry", false);
}

void IFormVoucherWidget::updateNextNumbers() {
    QDate d = m_dateEdit->date();
    QString fy = AccountingEngine::resolveFinancialYear(d.toString("yyyy-MM-dd"));
    QVariantMap info = m_iformModel.get_next_voucher_info(fy);
    int nextVch = info.value("next_voucher_no", 1).toInt();
    QString nextIf = info.value("next_iform_no", "1").toString();

    m_voucherNoDisplay->setText(QString("Vch: IFrm-%1").arg(nextVch));
    if (!isEditMode()) {
        m_iformNoEdit->setText(nextIf);
    }
}

void IFormVoucherWidget::updateFiscalYearBadge() {
    QDate d = m_dateEdit->date();
    FiscalYearInfo fy = FiscalYearHelper::getFiscalYearForDate(d.toString("yyyy-MM-dd"));
    if (fy.isValid()) {
        m_fyBadgeLabel->setText(fy.name);
    } else {
        m_fyBadgeLabel->setText(AccountingEngine::resolveFinancialYear(d.toString("yyyy-MM-dd")));
    }
}

void IFormVoucherWidget::updateDayOfWeek(const QDate& date) {
    m_dayOfWeekLabel->setText(date.toString("dddd"));
}

void IFormVoucherWidget::setWorkingDate(const QString& dateStr) {
    QDate d = QDate::fromString(dateStr, "yyyy-MM-dd");
    if (!d.isValid()) d = QDate::fromString(dateStr, "dd-MM-yyyy");
    if (d.isValid()) {
        m_dateEdit->setDate(d);
        onDateChanged(d);
    }
}

void IFormVoucherWidget::onDateChanged(const QDate& date) {
    updateDayOfWeek(date);
    updateFiscalYearBadge();
    if (!isEditMode()) {
        updateNextNumbers();
    }
}

void IFormVoucherWidget::onBuyerSelected(const QVariantMap& partyData) {
    m_selectedBuyerId = partyData.value("id").toInt();
    m_selectedBuyerName = partyData.value("party_name").toString().trimmed();

    if (m_selectedBuyerId > 0) {
        QVariantMap balInfo = m_iformModel.get_buyer_balance(m_selectedBuyerId);
        m_buyerBalanceLabel->setText(balInfo.value("formatted_balance").toString());
    } else {
        m_buyerBalanceLabel->setText("Date Bal. 0.00");
    }
}

void IFormVoucherWidget::populateRow(int row, const QString& itemName, int bags, double weight, double rate, double goodsAmt, double dami, double mfee, double hrdf, double lineTotal) {
    if (row >= m_table->rowCount()) {
        m_table->insertRow(row);
    }
    m_isUpdatingTable = true;

    // Sale-voucher cell contract: explicit foregrounds on every cell
    // (dark-mode hardening); computed levy columns are display-only.
    const QBrush navyFg(ErpTheme::kNavy);
    const QBrush grayFg(ErpTheme::kMuted);
    const QBrush greenFg(ErpTheme::kGreenDark);
    const QBrush editBg(ErpTheme::kEditableBg);
    const QBrush whiteBg(ErpTheme::kPaper);

    auto editCell = [&](int col, const QString& text) {
        auto* it = new QTableWidgetItem(text);
        it->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        it->setForeground(navyFg);
        it->setBackground(editBg);
        m_table->setItem(row, col, it);
    };
    auto numCell = [&](int col, const QString& text) {
        auto* it = new QTableWidgetItem(text);
        it->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        it->setForeground(navyFg);
        it->setBackground(editBg);
        m_table->setItem(row, col, it);
    };
    auto roCell = [&](int col, const QString& text, const QBrush& fg) {
        auto* it = new QTableWidgetItem(text);
        it->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        it->setForeground(fg);
        it->setBackground(whiteBg);
        it->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
        m_table->setItem(row, col, it);
    };

    editCell(0, itemName);
    numCell(1, bags > 0 ? QString::number(bags) : "");
    numCell(2, weight > 0.0 ? QString::number(weight, 'f', 3) : "");
    numCell(3, rate > 0.0 ? QString::number(rate, 'f', 2) : "");
    roCell(4, goodsAmt > 0.0 ? QString::number(goodsAmt, 'f', 2) : "", navyFg);
    roCell(5, dami > 0.0 ? QString::number(dami, 'f', 2) : "", grayFg);
    roCell(6, mfee > 0.0 ? QString::number(mfee, 'f', 2) : "", grayFg);
    roCell(7, hrdf > 0.0 ? QString::number(hrdf, 'f', 2) : "", grayFg);
    roCell(8, lineTotal > 0.0 ? QString::number(lineTotal, 'f', 2) : "", greenFg);

    m_isUpdatingTable = false;
}

void IFormVoucherWidget::addNewLineRow() {
    int r = m_table->rowCount();
    m_isUpdatingTable = true;
    m_table->insertRow(r);
    m_isUpdatingTable = false;
    populateRow(r, "", 0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0);
}

void IFormVoucherWidget::removeLineRow(int row) {
    if (row >= 0 && row < m_table->rowCount()) {
        m_isUpdatingTable = true;
        m_table->removeRow(row);
        m_isUpdatingTable = false;
        if (m_table->rowCount() == 0) addNewLineRow();
        recalculateTotals();
    }
}

void IFormVoucherWidget::onStockItemConfigured(int row, const QVariantMap& itemData) {
    if (row < 0 || row >= m_table->rowCount()) return;
    m_isUpdatingTable = true;
    QString name = itemData.value("name").toString().trimmed();
    if (name.isEmpty()) name = itemData.value("item_name").toString().trimmed();
    if (m_table->item(row, 0)) {
        m_table->item(row, 0)->setText(name);
    }
    double srate = itemData.value("sale_rate").toDouble();
    if (srate > 0 && m_table->item(row, 3)) {
        m_table->item(row, 3)->setText(QString::number(srate, 'f', 2));
    }
    m_isUpdatingTable = false;
    recalculateTotals();
    focusTableAt(row, 1);
}

void IFormVoucherWidget::onTableCellChanged(int row, int column) {
    if (m_isUpdatingTable) return;
    if (row < 0 || row >= m_table->rowCount()) return;
    m_isUpdatingTable = true;

    if (column == 1 || column == 2 || column == 3) {
        int bags = m_table->item(row, 1) ? m_table->item(row, 1)->text().toInt() : 0;
        double wt = m_table->item(row, 2) ? m_table->item(row, 2)->text().toDouble() : 0.0;
        double rate = m_table->item(row, 3) ? m_table->item(row, 3)->text().toDouble() : 0.0;

        if (wt <= 0.0 && bags > 0) {
            wt = (bags * 50.0) / 100.0;
            if (m_table->item(row, 2)) m_table->item(row, 2)->setText(QString::number(wt, 'f', 3));
        }

        double goodsAmt = wt * rate;
        double lineDami = (goodsAmt * 2.5) / 100.0;
        double lineMFee = (goodsAmt * 2.0) / 100.0;
        double lineHRDF = (goodsAmt * 0.5) / 100.0;
        double lineTotal = goodsAmt + lineDami + lineMFee + lineHRDF;

        if (m_table->item(row, 4)) m_table->item(row, 4)->setText(QString::number(goodsAmt, 'f', 2));
        if (m_table->item(row, 5)) m_table->item(row, 5)->setText(QString::number(lineDami, 'f', 2));
        if (m_table->item(row, 6)) m_table->item(row, 6)->setText(QString::number(lineMFee, 'f', 2));
        if (m_table->item(row, 7)) m_table->item(row, 7)->setText(QString::number(lineHRDF, 'f', 2));
        if (m_table->item(row, 8)) m_table->item(row, 8)->setText(QString::number(lineTotal, 'f', 2));
    }

    m_isUpdatingTable = false;
    recalculateTotals();

    // Auto-append the next blank row only when the user actually typed the
    // rate here (sale pattern: last row + committed rate + item set).
    if (row == m_table->rowCount() - 1 && column == 3) {
        auto* rateIt = m_table->item(row, 3);
        if (rateIt && !rateIt->text().trimmed().isEmpty() &&
            m_table->item(row, 0) && !m_table->item(row, 0)->text().trimmed().isEmpty()) {
            addNewLineRow();
        }
    }
}

void IFormVoucherWidget::recalculateTotals() {
    int grandBags = 0;
    double grandWt = 0.0;
    double grandGoods = 0.0;
    double grandDami = 0.0;
    double grandMFee = 0.0;
    double grandHRDF = 0.0;

    for (int r = 0; r < m_table->rowCount(); ++r) {
        if (m_table->item(r, 1)) grandBags += m_table->item(r, 1)->text().toInt();
        if (m_table->item(r, 2)) grandWt += m_table->item(r, 2)->text().toDouble();
        if (m_table->item(r, 4)) grandGoods += m_table->item(r, 4)->text().toDouble();
        if (m_table->item(r, 5)) grandDami += m_table->item(r, 5)->text().toDouble();
        if (m_table->item(r, 6)) grandMFee += m_table->item(r, 6)->text().toDouble();
        if (m_table->item(r, 7)) grandHRDF += m_table->item(r, 7)->text().toDouble();
    }

    m_totalBagsLabel->setText(QString::number(grandBags));
    m_totalWeightLabel->setText(QString("%1 Qtl").arg(QString::number(grandWt, 'f', 3)));
    m_goodsAmountLabel->setText(QString("₹ %1").arg(QString::number(grandGoods, 'f', 2)));
    m_damiLabel->setText(QString("₹ %1").arg(QString::number(grandDami, 'f', 2)));
    m_mandiFeeLabel->setText(QString("₹ %1").arg(QString::number(grandMFee, 'f', 2)));
    m_hrdfLabel->setText(QString("₹ %1").arg(QString::number(grandHRDF, 'f', 2)));

    double labour = m_labourEdit ? m_labourEdit->text().toDouble() : 0.0;
    double roundOff = m_roundOffEdit ? m_roundOffEdit->text().toDouble() : 0.0;

    double grandTotal = grandGoods + grandDami + grandMFee + grandHRDF + labour + roundOff;
    m_grandTotalLabel->setText(QString("₹ %1").arg(QString::number(grandTotal, 'f', 2)));
}

void IFormVoucherWidget::saveVoucher() {
    if (m_selectedBuyerName.isEmpty()) {
        CustomMessageBox::warning(this, "Missing Buyer", "Please select a Buyer.");
        m_buyerSearch->setFocus();
        return;
    }

    QVariantList items;
    for (int r = 0; r < m_table->rowCount(); ++r) {
        QString iName = m_table->item(r, 0) ? m_table->item(r, 0)->text().trimmed() : "";
        if (iName.isEmpty()) continue;
        int bags = m_table->item(r, 1) ? m_table->item(r, 1)->text().toInt() : 0;
        double wt = m_table->item(r, 2) ? m_table->item(r, 2)->text().toDouble() : 0.0;
        double rate = m_table->item(r, 3) ? m_table->item(r, 3)->text().toDouble() : 0.0;
        double amt = m_table->item(r, 4) ? m_table->item(r, 4)->text().toDouble() : 0.0;
        double dami = m_table->item(r, 5) ? m_table->item(r, 5)->text().toDouble() : 0.0;
        double mfee = m_table->item(r, 6) ? m_table->item(r, 6)->text().toDouble() : 0.0;
        double hrdf = m_table->item(r, 7) ? m_table->item(r, 7)->text().toDouble() : 0.0;

        QVariantMap rowMap;
        rowMap["item_name"] = iName;
        rowMap["bags"] = bags;
        rowMap["weight"] = wt;
        rowMap["rate"] = rate;
        rowMap["amount"] = amt;
        rowMap["dami_amount"] = dami;
        rowMap["mandi_fee_amount"] = mfee;
        rowMap["hrdf_amount"] = hrdf;
        items.append(rowMap);
    }

    if (items.isEmpty()) {
        CustomMessageBox::warning(this, "Empty Items", "Please add at least one line item.");
        return;
    }

    QVariantMap headerData;
    if (isEditMode()) headerData["id"] = m_editingVoucherId;
    headerData["voucher_date"] = m_dateEdit->date().toString("yyyy-MM-dd");
    headerData["iform_no"] = m_iformNoEdit->text().trimmed();
    headerData["buyer_id"] = m_selectedBuyerId;
    headerData["buyer_name"] = m_selectedBuyerName;
    headerData["broker_name"] = m_brokerNameEdit->text().trimmed();
    headerData["due_days"] = m_dueDaysEdit->text().toInt();
    headerData["vehicle_no"] = m_vehicleNoEdit->text().trimmed().toUpper();
    headerData["gr_no"] = m_grNoEdit->text().trimmed();
    headerData["narration"] = m_narrationEdit->text().trimmed();

    int grandBags = 0;
    double grandWt = 0.0;
    double grandGoods = 0.0;
    double grandDami = 0.0;
    double grandMFee = 0.0;
    double grandHRDF = 0.0;

    for (const auto& itVar : items) {
        QVariantMap it = itVar.toMap();
        grandBags += it.value("bags").toInt();
        grandWt += it.value("weight").toDouble();
        grandGoods += it.value("amount").toDouble();
        grandDami += it.value("dami_amount").toDouble();
        grandMFee += it.value("mandi_fee_amount").toDouble();
        grandHRDF += it.value("hrdf_amount").toDouble();
    }

    double labour = m_labourEdit->text().toDouble();
    double roundOff = m_roundOffEdit->text().toDouble();
    double grandTotal = grandGoods + grandDami + grandMFee + grandHRDF + labour + roundOff;

    headerData["total_bags"] = grandBags;
    headerData["total_weight"] = grandWt;
    headerData["goods_amount"] = grandGoods;
    headerData["dami_amount"] = grandDami;
    headerData["mandi_fee_amount"] = grandMFee;
    headerData["hrdf_amount"] = grandHRDF;
    headerData["labour_amount"] = labour;
    headerData["round_off"] = roundOff;
    headerData["grand_total"] = grandTotal;

    bool ok = m_iformModel.save_iform_voucher(headerData, items);
    if (ok) {
        QString savedIf = headerData["iform_no"].toString();
        CustomMessageBox::information(this, "Saved", QString("I-Form No. %1 saved successfully.").arg(savedIf));
        bool editMode = isEditMode();
        emit voucherSaved(savedIf);
        if (!editMode) {
            resetForm();
            if (m_buyerSearch) { m_buyerSearch->setFocus(); m_buyerSearch->selectAll(); }
        }
    } else {
        CustomMessageBox::critical(this, "Error", "Failed to save I-Form voucher. Check database logs.");
    }
}

bool IFormVoucherWidget::loadVoucherForEditing(const QVariant& vchNoOrId, const QString& dateHint, bool popupDateOnLoad) {
    Q_UNUSED(dateHint);
    QVariantMap vch = m_iformModel.get_iform_voucher(vchNoOrId);
    if (vch.isEmpty()) return false;

    m_editingVoucherId = vch.value("id").toInt();
    m_editingIFormNo = vch.value("iform_no").toString();
    m_editingVoucherNo = QString::number(vch.value("voucher_no").toInt());
    m_deleteButton->setEnabled(true);

    QDate d = QDate::fromString(vch.value("voucher_date").toString(), "yyyy-MM-dd");
    if (d.isValid()) m_dateEdit->setDate(d);

    m_voucherNoDisplay->setText(QString("Vch: IFrm-%1").arg(m_editingVoucherNo));
    m_iformNoEdit->setText(m_editingIFormNo);

    m_selectedBuyerId = vch.value("buyer_id").toInt();
    m_selectedBuyerName = vch.value("buyer_name").toString();
    m_buyerSearch->setText(m_selectedBuyerName);

    QVariantMap bal = m_iformModel.get_buyer_balance(m_selectedBuyerId);
    m_buyerBalanceLabel->setText(bal.value("formatted_balance").toString());

    m_dueDaysEdit->setText(QString::number(vch.value("due_days").toInt()));
    m_vehicleNoEdit->setText(vch.value("vehicle_no").toString());
    m_grNoEdit->setText(vch.value("gr_no").toString());
    m_brokerNameEdit->setText(vch.value("broker_name").toString());
    m_narrationEdit->setText(vch.value("narration").toString());

    m_labourEdit->setText(QString::number(vch.value("labour_amount").toDouble(), 'f', 2));
    m_roundOffEdit->setText(QString::number(vch.value("round_off").toDouble(), 'f', 2));

    QVariantList items = vch.value("items").toList();
    m_isUpdatingTable = true;
    m_table->setRowCount(0);
    m_isUpdatingTable = false;
    for (const auto& itVar : items) {
        QVariantMap it = itVar.toMap();
        int r = m_table->rowCount();
        double amt = it.value("amount").toDouble();
        double dami = it.value("dami_amount").toDouble();
        double mfee = it.value("mandi_fee_amount").toDouble();
        double hrdf = it.value("hrdf_amount").toDouble();
        populateRow(r,
            it.value("item_name").toString(),
            it.value("bags").toInt(),
            it.value("weight").toDouble(),
            it.value("rate").toDouble(),
            amt, dami, mfee, hrdf,
            amt + dami + mfee + hrdf);
    }
    if (m_table->rowCount() == 0) addNewLineRow();

    recalculateTotals();
    setStatusMessage(QString("Loaded I-Form %1 for editing").arg(m_editingIFormNo), false);
    // Drill-down entry: date dialog first (dates take no keyboard focus),
    // then the buyer field — openDateDialog already lands there.
    if (popupDateOnLoad) openDateDialog(false);
    return true;
}

void IFormVoucherWidget::deleteVoucher() {
    if (!isEditMode()) return;
    if (CustomMessageBox::question(this, "Confirm Delete", QString("Delete I-Form No. %1 permanently?").arg(m_editingIFormNo))) {
        bool ok = m_iformModel.delete_iform_voucher(m_editingVoucherId);
        if (ok) {
            emit voucherDeleted(m_editingIFormNo);
            CustomMessageBox::information(this, "Deleted", "I-Form voucher deleted.");
            resetForm();
        }
    }
}

void IFormVoucherWidget::openDateDialog(bool isInitial) {
    Q_UNUSED(isInitial);
    QString curDate = m_dateEdit ? m_dateEdit->date().toString("dd-MM-yyyy") : "";
    QString curIso = m_dateEdit ? m_dateEdit->date().toString("yyyy-MM-dd") : "";
    FiscalYearInfo fyCtx = isEditMode()
        ? FiscalYearHelper::getFiscalYearForDate(curIso)
        : FiscalYearHelper::getActiveFiscalYear();
    if (!fyCtx.isValid()) fyCtx = FiscalYearHelper::getActiveFiscalYear();
    QString newDisp, newIso;
    if (VoucherDateDialog::getVoucherDate(this, curDate, &newDisp, &newIso, fyCtx)) {
        if (m_dateEdit) {
            m_dateEdit->setDate(QDate::fromString(newIso, "yyyy-MM-dd"));
        }
        onDateChanged(QDate::fromString(newIso, "yyyy-MM-dd"));
    }
    if (m_buyerSearch) {
        m_buyerSearch->setFocus();
        m_buyerSearch->selectAll();
    }
}

void IFormVoucherWidget::openAlterVoucherDialog() {
    bool ok = false;
    QString text = CustomInputDialog::getText(this, "Find & Alter I-Form Voucher", "Enter I-Form No or Voucher No to alter:", m_editingIFormNo, &ok);
    if (ok && !text.trimmed().isEmpty()) {
        if (!loadVoucherForEditing(text.trimmed())) {
            CustomMessageBox::warning(this, "Voucher Not Found", QString("No I-Form voucher found matching \"%1\".").arg(text.trimmed()));
        }
    }
}

void IFormVoucherWidget::loadPreviousVoucher() {
    QVariantMap prev = m_iformModel.get_previous_iform_voucher(m_editingVoucherId, m_iformNoEdit ? m_iformNoEdit->text().trimmed() : "");
    if (!prev.isEmpty()) {
        loadVoucherForEditing(prev.value("id"));
    }
}

void IFormVoucherWidget::loadNextVoucher() {
    if (m_editingVoucherId <= 0) return;
    QVariantMap next = m_iformModel.get_next_iform_voucher(m_editingVoucherId, m_iformNoEdit ? m_iformNoEdit->text().trimmed() : "");
    if (!next.isEmpty()) {
        loadVoucherForEditing(next.value("id"));
    } else {
        resetForm();
    }
}

void IFormVoucherWidget::printVoucher() {
    if (!isEditMode() || !m_printExportCtrl) {
        CustomMessageBox::warning(this, "Nothing to Print", "Save the I-Form before printing.");
        return;
    }
    if (!m_printExportCtrl->print_iform_voucher(m_editingVoucherNo)) {
        CustomMessageBox::critical(this, "Print Failed", QString("Could not print I-Form %1.").arg(m_editingIFormNo));
    }
}

void IFormVoucherWidget::exportPdf() {
    if (!isEditMode() || !m_printExportCtrl) {
        CustomMessageBox::warning(this, "Nothing to Export", "Save the I-Form before exporting.");
        return;
    }
    QString path = m_printExportCtrl->export_iform_voucher_pdf(m_editingVoucherNo, "");
    if (path.isEmpty()) {
        CustomMessageBox::critical(this, "Export Failed", QString("Could not export I-Form %1 to PDF.").arg(m_editingIFormNo));
    }
}

void IFormVoucherWidget::focusTableAt(int row, int col) {
    if (!m_table || m_table->rowCount() == 0) return;
    int r = qBound(0, row, m_table->rowCount() - 1);
    int c = qBound(0, col, m_table->columnCount() - 1);
    m_table->setCurrentCell(r, c);
    m_table->setFocus(Qt::OtherFocusReason);
    m_table->edit(m_table->currentIndex());
}

void IFormVoucherWidget::moveCell(int row, int col) {
    focusTableAt(row, col);
}

void IFormVoucherWidget::advanceCell() {
    if (!m_table) return;
    int curRow = m_table->currentRow();
    int curCol = m_table->currentColumn();
    if (curRow < 0) curRow = 0;
    if (curCol < 0) curCol = 0;

    QTableWidgetItem* itemIt = m_table->item(curRow, 0);
    bool isItemEmpty = (!itemIt || itemIt->text().trimmed().isEmpty());

    // If empty item name on first column
    if (curCol == 0 && isItemEmpty) {
        if (curRow > 0 && curRow == m_table->rowCount() - 1) {
            m_table->removeRow(curRow);
            recalculateTotals();
        }
        if (m_labourEdit) {
            m_labourEdit->setFocus();
            m_labourEdit->selectAll();
        }
        return;
    }

    int nextCol = curCol + 1;
    // Editable columns: 0 (Item), 1 (Bags), 2 (Weight), 3 (Rate)
    // Columns 4 (Goods Amt), 5 (Dami), 6 (Mandi Fee), 7 (HRDF), 8 (Line Total) are calculated
    if (nextCol <= 3) {
        m_table->setCurrentCell(curRow, nextCol);
        m_table->edit(m_table->currentIndex());
    } else {
        // End of row: move to next row or add new line row
        if (!isItemEmpty) {
            if (curRow == m_table->rowCount() - 1) {
                addNewLineRow();
            }
            m_table->setCurrentCell(curRow + 1, 0);
            m_table->edit(m_table->currentIndex());
        } else {
            if (curRow > 0 && curRow == m_table->rowCount() - 1) {
                m_table->removeRow(curRow);
                recalculateTotals();
            }
            if (m_labourEdit) {
                m_labourEdit->setFocus();
                m_labourEdit->selectAll();
            }
        }
    }
}

void IFormVoucherWidget::retreatCell() {
    if (!m_table) return;
    int curRow = m_table->currentRow();
    int curCol = m_table->currentColumn();

    int prevCol = curCol - 1;
    if (prevCol >= 0) {
        m_table->setCurrentCell(curRow, prevCol);
        m_table->edit(m_table->currentIndex());
    } else if (curRow > 0) {
        m_table->setCurrentCell(curRow - 1, 3);
        m_table->edit(m_table->currentIndex());
    } else {
        if (m_grNoEdit) {
            m_grNoEdit->setFocus();
            m_grNoEdit->selectAll();
        }
    }
}

void IFormVoucherWidget::showEvent(QShowEvent* event) {
    QWidget::showEvent(event);
}

void IFormVoucherWidget::keyPressEvent(QKeyEvent* event) {
    // Grid with focus but no cell editor open (unhandled keys propagate here
    // only in that state): Enter opens the current cell's editor; Backspace
    // on a filled cell clears it, on an empty cell steps back.
    if ((event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter ||
         event->key() == Qt::Key_Backspace) &&
        m_table && QApplication::focusWidget() == m_table) {
        int r = m_table->currentRow(), c = m_table->currentColumn();
        if (r >= 0 && c >= 0 && r < m_table->rowCount() && c < m_table->columnCount()) {
            if (event->key() == Qt::Key_Backspace) {
                if (QTableWidgetItem* it = m_table->item(r, c)) {
                    if (!it->text().isEmpty()) {
                        it->setText("");
                        recalculateTotals();
                        event->accept();
                        return;
                    }
                }
                retreatCell();
            } else {
                m_table->edit(m_table->currentIndex());
            }
            event->accept();
            return;
        }
    }
    if (event->key() == Qt::Key_F2) {
        openDateDialog(false);
        event->accept();
        return;
    } else if (event->key() == Qt::Key_F4) {
        if (!VoucherCommon::browseBlockedByPopup()) openAlterVoucherDialog();
        event->accept();
        return;
    } else if (event->key() == Qt::Key_PageUp) {
        if (!VoucherCommon::browseBlockedByPopup()) loadPreviousVoucher();
        event->accept();
        return;
    } else if (event->key() == Qt::Key_PageDown) {
        if (!VoucherCommon::browseBlockedByPopup()) loadNextVoucher();
        event->accept();
        return;
    } else if (event->key() == Qt::Key_S && (event->modifiers() & Qt::ControlModifier)) {
        saveVoucher();
        event->accept();
        return;
    } else if (event->key() == Qt::Key_D && (event->modifiers() & Qt::ControlModifier)) {
        deleteVoucher();
        event->accept();
        return;
    } else if (event->key() == Qt::Key_P && (event->modifiers() & Qt::ControlModifier)) {
        printVoucher();
        event->accept();
        return;
    } else if (event->key() == Qt::Key_E && (event->modifiers() & Qt::ControlModifier)) {
        exportPdf();
        event->accept();
        return;
    } else if (event->key() == Qt::Key_Escape) {
        emit backRequested();
        event->accept();
        return;
    }
    QWidget::keyPressEvent(event);
}

bool IFormVoucherWidget::eventFilter(QObject* watched, QEvent* event) {
    Q_UNUSED(watched);
    if (event->type() == QEvent::KeyPress) {
        auto* keyEvent = static_cast<QKeyEvent*>(event);
        int key = keyEvent->key();

        if (key == Qt::Key_Escape) {
            emit backRequested();
            return true;
        }
        if (key == Qt::Key_F2) {
            openDateDialog(false);
            return true;
        }
        // Combos swallow PgUp/PgDn/F4 natively, so this filter (which runs
        // before the control's own handler) is the only reliable catch.
        // Open popups keep the keys for list paging/close instead.
        if ((key == Qt::Key_PageUp || key == Qt::Key_PageDown || key == Qt::Key_F4) &&
            !VoucherCommon::browseBlockedByPopup()) {
            if (key == Qt::Key_PageUp) loadPreviousVoucher();
            else if (key == Qt::Key_PageDown) loadNextVoucher();
            else openAlterVoucherDialog();
            return true;
        }
        if (key == Qt::Key_S && (keyEvent->modifiers() & Qt::ControlModifier)) {
            saveVoucher();
            return true;
        }
        if (key == Qt::Key_D && (keyEvent->modifiers() & Qt::ControlModifier)) {
            deleteVoucher();
            return true;
        }
        if (key == Qt::Key_P && (keyEvent->modifiers() & Qt::ControlModifier)) {
            printVoucher();
            return true;
        }
        if (key == Qt::Key_E && (keyEvent->modifiers() & Qt::ControlModifier)) {
            exportPdf();
            return true;
        }
        // Enter/Left/Right/Backspace belong to installGridNavigation
        // (setupNavigationChains); never handle field keys here.
    }
    return QWidget::eventFilter(watched, event);
}

void IFormVoucherWidget::setStatusMessage(const QString& message, bool isError) {
    m_statusLabel->setText(message);
    m_statusLabel->setStyleSheet(isError ? "color: red; font-weight: bold;" : "color: #475569;");
}
