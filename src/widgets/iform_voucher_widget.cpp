#include "iform_voucher_widget.h"
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
    m_iformNoEdit->setFixedWidth(80);

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
    auto* headerBox = new QGroupBox("Buyer (Mill / Trader) & Dispatch Details", this);
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
    m_brokerNameEdit = new QLineEdit(this);
    headerLayout->addWidget(m_brokerNameEdit, 1, 1);

    headerLayout->addWidget(new QLabel("Due Days:", this), 1, 2);
    m_dueDaysEdit = new QLineEdit("0", this);
    m_dueDaysEdit->setFixedWidth(60);
    headerLayout->addWidget(m_dueDaysEdit, 1, 3);

    // Row 2: Vehicle & GR No
    headerLayout->addWidget(new QLabel("Vehicle No:", this), 2, 0);
    m_vehicleNoEdit = new QLineEdit(this);
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
    m_table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    for (int c = 1; c < 9; ++c) {
        m_table->horizontalHeader()->setSectionResizeMode(c, QHeaderView::ResizeToContents);
    }
    m_table->verticalHeader()->setVisible(false);
    m_table->horizontalHeader()->setHighlightSections(false);
    m_table->setShowGrid(true);

    m_itemDelegate = new ItemSearchDelegate(this);
    m_table->setItemDelegateForColumn(0, m_itemDelegate);

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

    m_dateButton = new QPushButton("F2 - Change Date", this);
    m_saveButton = new QPushButton("Ctrl+S - Save I-Form", this);
    m_saveButton->setObjectName("primaryButton");
    m_deleteButton = new QPushButton("Ctrl+D - Delete", this);
    m_deleteButton->setObjectName("dangerButton");
    m_deleteButton->setEnabled(false);
    m_printButton = new QPushButton("Ctrl+P - Print", this);
    m_cancelButton = new QPushButton("Esc - Cancel", this);

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
    connect(m_labourEdit, &QLineEdit::textChanged, this, &IFormVoucherWidget::recalculateTotals);
    connect(m_roundOffEdit, &QLineEdit::textChanged, this, &IFormVoucherWidget::recalculateTotals);

    connect(m_dateButton, &QPushButton::clicked, this, &IFormVoucherWidget::openDateDialog);
    connect(m_saveButton, &QPushButton::clicked, this, &IFormVoucherWidget::saveVoucher);
    connect(m_deleteButton, &QPushButton::clicked, this, &IFormVoucherWidget::deleteVoucher);
    connect(m_printButton, &QPushButton::clicked, this, &IFormVoucherWidget::printVoucher);
    connect(m_cancelButton, &QPushButton::clicked, this, [this]() { emit backRequested(); });
}

void IFormVoucherWidget::setupNavigationChains() {
    setTabOrder(m_iformNoEdit, m_buyerSearch);
    setTabOrder(m_buyerSearch, m_brokerNameEdit);
    setTabOrder(m_brokerNameEdit, m_dueDaysEdit);
    setTabOrder(m_dueDaysEdit, m_vehicleNoEdit);
    setTabOrder(m_vehicleNoEdit, m_grNoEdit);
    setTabOrder(m_grNoEdit, m_table);
    setTabOrder(m_table, m_labourEdit);
    setTabOrder(m_labourEdit, m_roundOffEdit);
    setTabOrder(m_roundOffEdit, m_narrationEdit);
    setTabOrder(m_narrationEdit, m_saveButton);

    const QList<QWidget*> inputs = {
        m_iformNoEdit, m_buyerSearch, m_brokerNameEdit, m_dueDaysEdit,
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
        "  selection-background-color: #EDE9FE;"
        "  selection-color: #5B21B6;"
        "  font-size: 12px;"
        "}"
        "QHeaderView::section {"
        "  background-color: #F1F5F9;"
        "  color: #334155;"
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
        "#primaryButton {"
        "  background-color: #7C3AED;"
        "  color: #FFFFFF;"
        "  border: 1px solid #6D28D9;"
        "  font-weight: 800;"
        "}"
        "#primaryButton:hover {"
        "  background-color: #6D28D9;"
        "}"
        "#dangerButton {"
        "  background-color: #FEE2E2;"
        "  color: #DC2626;"
        "  border: 1px solid #FCA5A5;"
        "  font-weight: 700;"
        "}"
        "#dangerButton:hover {"
        "  background-color: #FECACA;"
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

    QDate cur = QDate::currentDate();
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

void IFormVoucherWidget::addNewLineRow() {
    int r = m_table->rowCount();
    m_table->insertRow(r);
    for (int c = 0; c < 9; ++c) {
        auto* it = new QTableWidgetItem();
        if (c >= 1) it->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        m_table->setItem(r, c, it);
    }
}

void IFormVoucherWidget::removeLineRow(int row) {
    if (row >= 0 && row < m_table->rowCount()) {
        m_table->removeRow(row);
        if (m_table->rowCount() == 0) addNewLineRow();
        recalculateTotals();
    }
}

void IFormVoucherWidget::onStockItemConfigured(int row, const QVariantMap& itemData) {
    if (row < 0 || row >= m_table->rowCount()) return;
    QString name = itemData.value("name").toString().trimmed();
    if (name.isEmpty()) name = itemData.value("item_name").toString().trimmed();
    if (m_table->item(row, 0)) {
        m_table->item(row, 0)->setText(name);
    }
    double srate = itemData.value("sale_rate").toDouble();
    if (srate > 0 && m_table->item(row, 3)) {
        m_table->item(row, 3)->setText(QString::number(srate, 'f', 2));
    }
    recalculateTotals();
}

void IFormVoucherWidget::onTableCellChanged(int row, int column) {
    if (row < 0 || row >= m_table->rowCount()) return;

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

    recalculateTotals();
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
    headerData["vehicle_no"] = m_vehicleNoEdit->text().trimmed();
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
        emit voucherSaved(savedIf);
        CustomMessageBox::information(this, "Saved", QString("I-Form No. %1 saved successfully.").arg(savedIf));
        resetForm();
    } else {
        CustomMessageBox::critical(this, "Error", "Failed to save I-Form voucher. Check database logs.");
    }
}

bool IFormVoucherWidget::loadVoucherForEditing(const QVariant& vchNoOrId, const QString& dateHint) {
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
    m_table->setRowCount(0);
    for (const auto& itVar : items) {
        QVariantMap it = itVar.toMap();
        int r = m_table->rowCount();
        m_table->insertRow(r);
        m_table->setItem(r, 0, new QTableWidgetItem(it.value("item_name").toString()));
        m_table->setItem(r, 1, new QTableWidgetItem(QString::number(it.value("bags").toInt())));
        m_table->setItem(r, 2, new QTableWidgetItem(QString::number(it.value("weight").toDouble(), 'f', 3)));
        m_table->setItem(r, 3, new QTableWidgetItem(QString::number(it.value("rate").toDouble(), 'f', 2)));
        m_table->setItem(r, 4, new QTableWidgetItem(QString::number(it.value("amount").toDouble(), 'f', 2)));
        m_table->setItem(r, 5, new QTableWidgetItem(QString::number(it.value("dami_amount").toDouble(), 'f', 2)));
        m_table->setItem(r, 6, new QTableWidgetItem(QString::number(it.value("mandi_fee_amount").toDouble(), 'f', 2)));
        m_table->setItem(r, 7, new QTableWidgetItem(QString::number(it.value("hrdf_amount").toDouble(), 'f', 2)));

        double lineTot = it.value("amount").toDouble() + it.value("dami_amount").toDouble() + it.value("mandi_fee_amount").toDouble() + it.value("hrdf_amount").toDouble();
        m_table->setItem(r, 8, new QTableWidgetItem(QString::number(lineTot, 'f', 2)));
    }
    if (m_table->rowCount() == 0) addNewLineRow();

    recalculateTotals();
    setStatusMessage(QString("Loaded I-Form %1 for editing").arg(m_editingIFormNo), false);
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
    QString curDate = m_dateEdit ? m_dateEdit->date().toString("dd-MM-yyyy") : "";
    QString newDisp, newIso;
    if (VoucherDateDialog::getVoucherDate(this, curDate, &newDisp, &newIso)) {
        if (m_dateEdit) {
            m_dateEdit->setDate(QDate::fromString(newIso, "yyyy-MM-dd"));
        }
        onDateChanged(QDate::fromString(newIso, "yyyy-MM-dd"));
        if (m_buyerSearch) {
            m_buyerSearch->setFocus();
            m_buyerSearch->selectAll();
        }
    } else {
        if (isInitial) {
            emit backRequested();
        } else if (m_buyerSearch) {
            m_buyerSearch->setFocus();
        }
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

void IFormVoucherWidget::printVoucher() {}

void IFormVoucherWidget::focusTableAt(int row, int col) {
    if (!m_table) return;
    if (row < 0 || row >= m_table->rowCount()) return;
    if (col < 0 || col >= m_table->columnCount()) return;
    m_table->setCurrentCell(row, col);
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
    if (m_buyerSearch) {
        m_buyerSearch->setFocus();
    }
}

void IFormVoucherWidget::keyPressEvent(QKeyEvent* event) {
    if (event->key() == Qt::Key_F2) {
        openDateDialog();
        event->accept();
        return;
    } else if (event->key() == Qt::Key_F4) {
        openAlterVoucherDialog();
        event->accept();
        return;
    } else if (event->key() == Qt::Key_PageUp) {
        loadPreviousVoucher();
        event->accept();
        return;
    } else if (event->key() == Qt::Key_PageDown) {
        loadNextVoucher();
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
    } else if (event->key() == Qt::Key_Escape) {
        emit backRequested();
        event->accept();
        return;
    }
    QWidget::keyPressEvent(event);
}

bool IFormVoucherWidget::eventFilter(QObject* watched, QEvent* event) {
    if (event->type() == QEvent::KeyPress) {
        auto* keyEvent = static_cast<QKeyEvent*>(event);
        int key = keyEvent->key();

        if (key == Qt::Key_Escape) {
            emit backRequested();
            return true;
        }
        if (key == Qt::Key_F2) {
            openDateDialog();
            return true;
        }
        if (key == Qt::Key_F4) {
            openAlterVoucherDialog();
            return true;
        }
        if (key == Qt::Key_PageUp) {
            loadPreviousVoucher();
            return true;
        }
        if (key == Qt::Key_PageDown) {
            loadNextVoucher();
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

        // Return / Enter progression across form fields
        if (key == Qt::Key_Return || key == Qt::Key_Enter) {
            if (watched == m_iformNoEdit) {
                m_buyerSearch->setFocus();
                return true;
            } else if (watched == m_buyerSearch) {
                if (!m_buyerSearch->isPopupVisible()) {
                    m_brokerNameEdit->setFocus();
                    m_brokerNameEdit->selectAll();
                    return true;
                }
            } else if (watched == m_brokerNameEdit) {
                m_dueDaysEdit->setFocus();
                m_dueDaysEdit->selectAll();
                return true;
            } else if (watched == m_dueDaysEdit) {
                m_vehicleNoEdit->setFocus();
                m_vehicleNoEdit->selectAll();
                return true;
            } else if (watched == m_vehicleNoEdit) {
                m_grNoEdit->setFocus();
                m_grNoEdit->selectAll();
                return true;
            } else if (watched == m_grNoEdit) {
                if (m_table->rowCount() == 0) addNewLineRow();
                focusTableAt(0, 0);
                return true;
            } else if (watched == m_labourEdit) {
                m_roundOffEdit->setFocus();
                m_roundOffEdit->selectAll();
                return true;
            } else if (watched == m_roundOffEdit) {
                m_narrationEdit->setFocus();
                m_narrationEdit->selectAll();
                return true;
            } else if (watched == m_narrationEdit) {
                m_saveButton->setFocus();
                return true;
            }
        }
    }
    return QWidget::eventFilter(watched, event);
}

void IFormVoucherWidget::setStatusMessage(const QString& message, bool isError) {
    m_statusLabel->setText(message);
    m_statusLabel->setStyleSheet(isError ? "color: red; font-weight: bold;" : "color: #475569;");
}
