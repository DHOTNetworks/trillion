#include "jform_voucher_widget.h"
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
#include <cmath>

JFormVoucherWidget::JFormVoucherWidget(PrintExportController* printExportCtrl, QWidget* parent)
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

JFormVoucherWidget::~JFormVoucherWidget() = default;

void JFormVoucherWidget::setupUi() {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(12, 10, 12, 10);
    mainLayout->setSpacing(8);

    // 1. Top Bar
    auto* topBarFrame = new QFrame(this);
    topBarFrame->setObjectName("topBarFrame");
    auto* topBarLayout = new QHBoxLayout(topBarFrame);
    topBarLayout->setContentsMargins(8, 6, 8, 6);

    m_badgeLabel = new QLabel("J-FORM MANDI PROCUREMENT", this);
    m_badgeLabel->setObjectName("voucherBadge");

    m_voucherNoDisplay = new QLabel("Vch: JFrm-1", this);
    m_voucherNoDisplay->setObjectName("headerInfoLabel");

    m_jformNoDisplay = new QLabel("J-Form No:", this);
    m_jformNoEdit = new QLineEdit("1", this);
    m_jformNoEdit->setFixedWidth(80);

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
    topBarLayout->addWidget(m_jformNoDisplay);
    topBarLayout->addWidget(m_jformNoEdit);
    topBarLayout->addSpacing(16);
    topBarLayout->addWidget(dateLabel);
    topBarLayout->addWidget(m_dateEdit);
    topBarLayout->addWidget(m_dayOfWeekLabel);
    topBarLayout->addStretch();
    topBarLayout->addWidget(m_fyBadgeLabel);

    mainLayout->addWidget(topBarFrame);

    // 2. Farmer & Header Info
    auto* headerBox = new QGroupBox("Farmer (Zimidar) & Auction Details", this);
    auto* headerLayout = new QGridLayout(headerBox);
    headerLayout->setContentsMargins(8, 6, 8, 6);
    headerLayout->setSpacing(8);

    headerLayout->addWidget(new QLabel("Farmer Name (Enter to Search / F3 New):", this), 0, 0);
    m_farmerSearch = new PartySearchWidget(this);
    headerLayout->addWidget(m_farmerSearch, 0, 1, 1, 2);

    m_farmerBalanceLabel = new QLabel("Date Bal. 0.00", this);
    m_farmerBalanceLabel->setObjectName("balanceLabel");
    headerLayout->addWidget(m_farmerBalanceLabel, 0, 3);

    headerLayout->addWidget(new QLabel("Auction Status (Alt+S):", this), 1, 0);
    m_saleStatusCombo = new QComboBox(this);
    m_saleStatusCombo->addItems({"Zimidara Self Purchase", "Kacha Arhat Mandi", "Direct Procurement", "Government MSP"});
    headerLayout->addWidget(m_saleStatusCombo, 1, 1);

    headerLayout->addWidget(new QLabel("Due Days:", this), 1, 2);
    m_dueDaysEdit = new QLineEdit("0", this);
    m_dueDaysEdit->setFixedWidth(60);
    headerLayout->addWidget(m_dueDaysEdit, 1, 3);

    // Row 2: Vehicle & Broker
    headerLayout->addWidget(new QLabel("Vehicle No:", this), 2, 0);
    m_vehicleNoEdit = new QLineEdit(this);
    headerLayout->addWidget(m_vehicleNoEdit, 2, 1);

    headerLayout->addWidget(new QLabel("Broker / Munim:", this), 2, 2);
    m_brokerNameEdit = new QLineEdit(this);
    headerLayout->addWidget(m_brokerNameEdit, 2, 3);

    mainLayout->addWidget(headerBox);

    m_table = new QTableWidget(1, 7, this);
    m_table->setHorizontalHeaderLabels({"Item Description", "Bags", "Packing (Kg)", "Loose (Qtl)", "Total Weight (Qtl)", "Rate (₹/Qtl)", "Goods Amount (₹)"});
    m_table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(4, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(5, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(6, QHeaderView::ResizeToContents);
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

    // Row 1: Labour Deductions, Bonus, Relief
    summaryLayout->addWidget(new QLabel("Less Labour (Tulai/Hamali):", this), 1, 0);
    m_labourEdit = new QLineEdit("0.00", this);
    summaryLayout->addWidget(m_labourEdit, 1, 1);

    summaryLayout->addWidget(new QLabel("Bonus / Relief (+):", this), 1, 2);
    m_bonusEdit = new QLineEdit("0.00", this);
    summaryLayout->addWidget(m_bonusEdit, 1, 3);

    summaryLayout->addWidget(new QLabel("Round Off:", this), 1, 4);
    m_roundOffEdit = new QLineEdit("0.00", this);
    summaryLayout->addWidget(m_roundOffEdit, 1, 5);

    // Row 2: Narration & Net Payable
    summaryLayout->addWidget(new QLabel("Narration:", this), 2, 0);
    m_narrationEdit = new QLineEdit(this);
    summaryLayout->addWidget(m_narrationEdit, 2, 1, 1, 3);

    summaryLayout->addWidget(new QLabel("Net Payable to Farmer:", this), 2, 4);
    m_grandTotalLabel = new QLabel("₹ 0.00", this);
    m_grandTotalLabel->setObjectName("grandTotalLabel");
    summaryLayout->addWidget(m_grandTotalLabel, 2, 5);

    mainLayout->addWidget(summaryFrame);

    // 5. Bottom Action Bar
    auto* actionLayout = new QHBoxLayout();
    actionLayout->setSpacing(8);

    m_statusLabel = new QLabel(this);
    actionLayout->addWidget(m_statusLabel);
    actionLayout->addStretch();

    m_dateButton = new QPushButton("F2 - Change Date", this);
    m_saveButton = new QPushButton("Ctrl+S - Save J-Form", this);
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
    connect(m_dateEdit, &AccountingDateEdit::dateChanged, this, &JFormVoucherWidget::onDateChanged);
    connect(m_farmerSearch, &PartySearchWidget::partySelected, this, &JFormVoucherWidget::onFarmerSelected);
    connect(m_table, &QTableWidget::cellChanged, this, &JFormVoucherWidget::onTableCellChanged);
    connect(m_itemDelegate, &ItemSearchDelegate::stockItemConfigured, this, &JFormVoucherWidget::onStockItemConfigured);
    connect(m_itemDelegate, &ItemSearchDelegate::moveNextRequested, this, &JFormVoucherWidget::advanceCell);
    connect(m_itemDelegate, &ItemSearchDelegate::movePrevRequested, this, &JFormVoucherWidget::retreatCell);
    connect(m_labourEdit, &QLineEdit::textChanged, this, &JFormVoucherWidget::recalculateTotals);
    connect(m_bonusEdit, &QLineEdit::textChanged, this, &JFormVoucherWidget::recalculateTotals);
    connect(m_roundOffEdit, &QLineEdit::textChanged, this, &JFormVoucherWidget::recalculateTotals);

    connect(m_dateButton, &QPushButton::clicked, this, &JFormVoucherWidget::openDateDialog);
    connect(m_saveButton, &QPushButton::clicked, this, &JFormVoucherWidget::saveVoucher);
    connect(m_deleteButton, &QPushButton::clicked, this, &JFormVoucherWidget::deleteVoucher);
    connect(m_printButton, &QPushButton::clicked, this, &JFormVoucherWidget::printVoucher);
    connect(m_cancelButton, &QPushButton::clicked, this, [this]() { emit backRequested(); });
}

void JFormVoucherWidget::setupNavigationChains() {
    setTabOrder(m_jformNoEdit, m_farmerSearch);
    setTabOrder(m_farmerSearch, m_saleStatusCombo);
    setTabOrder(m_saleStatusCombo, m_dueDaysEdit);
    setTabOrder(m_dueDaysEdit, m_vehicleNoEdit);
    setTabOrder(m_vehicleNoEdit, m_brokerNameEdit);
    setTabOrder(m_brokerNameEdit, m_table);
    setTabOrder(m_table, m_labourEdit);
    setTabOrder(m_labourEdit, m_bonusEdit);
    setTabOrder(m_bonusEdit, m_roundOffEdit);
    setTabOrder(m_roundOffEdit, m_narrationEdit);
    setTabOrder(m_narrationEdit, m_saveButton);

    const QList<QWidget*> inputs = {
        m_jformNoEdit, m_farmerSearch, m_saleStatusCombo, m_dueDaysEdit,
        m_vehicleNoEdit, m_brokerNameEdit, m_table, m_labourEdit,
        m_bonusEdit, m_roundOffEdit, m_narrationEdit, m_dateButton,
        m_saveButton, m_deleteButton, m_printButton, m_cancelButton
    };
    for (QWidget* w : inputs) {
        if (w) w->installEventFilter(this);
    }
}

void JFormVoucherWidget::applyCustomStyles() {
    setStyleSheet(
        "JFormVoucherWidget {"
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
        "  border: 1.5px solid #0284C7;"
        "  background-color: #F0F9FF;"
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
        "  border: 1.5px solid #0284C7;"
        "  background-color: #F0F9FF;"
        "}"
        "QComboBox::drop-down {"
        "  border: none;"
        "  width: 22px;"
        "}"
        "QComboBox QAbstractItemView {"
        "  background-color: #FFFFFF;"
        "  color: #0F172A;"
        "  selection-background-color: #0284C7;"
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
        "  border: 1.5px solid #0284C7;"
        "  background-color: #F0F9FF;"
        "}"
        "QTableWidget {"
        "  background-color: #FFFFFF;"
        "  alternate-background-color: #F8FAFC;"
        "  color: #0F172A;"
        "  gridline-color: #E2E8F0;"
        "  border: 1px solid #CBD5E1;"
        "  border-radius: 6px;"
        "  selection-background-color: #E0F2FE;"
        "  selection-color: #0369A1;"
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
        "  background-color: #0284C7;"
        "  color: #FFFFFF;"
        "  border: 1px solid #0369A1;"
        "  font-weight: 800;"
        "}"
        "#primaryButton:hover {"
        "  background-color: #0369A1;"
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
        "#voucherBadge {"
        "  background-color: #0284C7;"
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
        "  background-color: #EFF6FF;"
        "  color: #1D4ED8;"
        "  font-weight: 700;"
        "  font-size: 11.5px;"
        "  padding: 4px 10px;"
        "  border: 1px solid #BFDBFE;"
        "  border-radius: 5px;"
        "}"
        "#balanceLabel {"
        "  font-weight: 800;"
        "  color: #DC2626;"
        "  font-size: 11.5px;"
        "  background-color: #FEF2F2;"
        "  border: 1px solid #FECACA;"
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
        "  color: #059669;"
        "  background-color: #ECFDF5;"
        "  border: 1px solid #A7F3D0;"
        "  border-radius: 6px;"
        "  padding: 4px 12px;"
        "}"
    );
}

void JFormVoucherWidget::resetForm() {
    m_editingVoucherId = 0;
    m_editingJFormNo.clear();
    m_editingVoucherNo.clear();
    m_deleteButton->setEnabled(false);

    QDate cur = QDate::currentDate();
    m_dateEdit->setDate(cur);
    updateDayOfWeek(cur);
    updateFiscalYearBadge();
    updateNextNumbers();

    m_farmerSearch->clear();
    m_farmerBalanceLabel->setText("Date Bal. 0.00");
    m_selectedFarmerId = 0;
    m_selectedFarmerName.clear();

    m_dueDaysEdit->setText("0");
    m_vehicleNoEdit->clear();
    m_brokerNameEdit->clear();
    m_narrationEdit->clear();

    m_table->setRowCount(0);
    addNewLineRow();
    recalculateTotals();
    setStatusMessage("Ready for new J-Form entry", false);
}

void JFormVoucherWidget::updateNextNumbers() {
    QDate d = m_dateEdit->date();
    QString fy = AccountingEngine::resolveFinancialYear(d.toString("yyyy-MM-dd"));
    QVariantMap info = m_jformModel.get_next_voucher_info(fy);
    int nextVch = info.value("next_voucher_no", 1).toInt();
    QString nextJf = info.value("next_jform_no", "1").toString();

    m_voucherNoDisplay->setText(QString("Vch: JFrm-%1").arg(nextVch));
    if (!isEditMode()) {
        m_jformNoEdit->setText(nextJf);
    }
}

void JFormVoucherWidget::updateFiscalYearBadge() {
    QDate d = m_dateEdit->date();
    FiscalYearInfo fy = FiscalYearHelper::getFiscalYearForDate(d.toString("yyyy-MM-dd"));
    if (fy.isValid()) {
        m_fyBadgeLabel->setText(fy.name);
    } else {
        m_fyBadgeLabel->setText(AccountingEngine::resolveFinancialYear(d.toString("yyyy-MM-dd")));
    }
}

void JFormVoucherWidget::updateDayOfWeek(const QDate& date) {
    m_dayOfWeekLabel->setText(date.toString("dddd"));
}

void JFormVoucherWidget::setWorkingDate(const QString& dateStr) {
    QDate d = QDate::fromString(dateStr, "yyyy-MM-dd");
    if (!d.isValid()) d = QDate::fromString(dateStr, "dd-MM-yyyy");
    if (d.isValid()) {
        m_dateEdit->setDate(d);
        onDateChanged(d);
    }
}

void JFormVoucherWidget::onDateChanged(const QDate& date) {
    updateDayOfWeek(date);
    updateFiscalYearBadge();
    if (!isEditMode()) {
        updateNextNumbers();
    }
}

void JFormVoucherWidget::onFarmerSelected(const QVariantMap& partyData) {
    m_selectedFarmerId = partyData.value("id").toInt();
    m_selectedFarmerName = partyData.value("party_name").toString().trimmed();

    if (m_selectedFarmerId > 0) {
        QVariantMap balInfo = m_jformModel.get_zimidar_balance(m_selectedFarmerId);
        m_farmerBalanceLabel->setText(balInfo.value("formatted_balance").toString());
    } else {
        m_farmerBalanceLabel->setText("Date Bal. 0.00");
    }
}

void JFormVoucherWidget::addNewLineRow() {
    int r = m_table->rowCount();
    m_table->insertRow(r);
    for (int c = 0; c < 7; ++c) {
        auto* it = new QTableWidgetItem();
        if (c == 1 || c == 2) it->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        else if (c >= 3) it->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        m_table->setItem(r, c, it);
    }
    m_table->item(r, 2)->setText("0.500"); // Standard packing deduction
}

void JFormVoucherWidget::removeLineRow(int row) {
    if (row >= 0 && row < m_table->rowCount()) {
        m_table->removeRow(row);
        if (m_table->rowCount() == 0) addNewLineRow();
        recalculateTotals();
    }
}

void JFormVoucherWidget::onStockItemConfigured(int row, const QVariantMap& itemData) {
    if (row < 0 || row >= m_table->rowCount()) return;
    QString name = itemData.value("name").toString().trimmed();
    if (name.isEmpty()) name = itemData.value("item_name").toString().trimmed();
    if (m_table->item(row, 0)) {
        m_table->item(row, 0)->setText(name);
    }
    double pkg = itemData.value("packing_kg").toDouble();
    if (pkg > 0 && m_table->item(row, 2)) {
        m_table->item(row, 2)->setText(QString::number(pkg, 'f', 3));
    }
    double prate = itemData.value("purchase_rate").toDouble();
    if (prate > 0 && m_table->item(row, 5)) {
        m_table->item(row, 5)->setText(QString::number(prate, 'f', 2));
    }
    recalculateTotals();
}

void JFormVoucherWidget::onTableCellChanged(int row, int column) {
    if (row < 0 || row >= m_table->rowCount()) return;

    // Recalculate row weight & amount if Bags/Packing/Loose/Rate changed
    if (column == 1 || column == 2 || column == 3 || column == 5) {
        int bags = m_table->item(row, 1) ? m_table->item(row, 1)->text().toInt() : 0;
        double packing = m_table->item(row, 2) ? m_table->item(row, 2)->text().toDouble() : 0.500;
        double loose = m_table->item(row, 3) ? m_table->item(row, 3)->text().toDouble() : 0.0;
        double rate = m_table->item(row, 5) ? m_table->item(row, 5)->text().toDouble() : 0.0;

        // Weight = Loose + (Bags * 0.500 Qtl) - (Bags * packing / 100 Qtl)
        double totalWt = loose;
        if (totalWt <= 0.0 && bags > 0) {
            totalWt = (bags * 50.0) / 100.0; // Default 50kg bag
        }

        double amt = totalWt * rate;
        if (m_table->item(row, 4)) m_table->item(row, 4)->setText(QString::number(totalWt, 'f', 3));
        if (m_table->item(row, 6)) m_table->item(row, 6)->setText(QString::number(amt, 'f', 2));
    }

    recalculateTotals();
}

void JFormVoucherWidget::recalculateTotals() {
    int grandBags = 0;
    double grandWt = 0.0;
    double grandGoods = 0.0;

    for (int r = 0; r < m_table->rowCount(); ++r) {
        if (m_table->item(r, 1)) grandBags += m_table->item(r, 1)->text().toInt();
        if (m_table->item(r, 4)) grandWt += m_table->item(r, 4)->text().toDouble();
        if (m_table->item(r, 6)) grandGoods += m_table->item(r, 6)->text().toDouble();
    }

    m_totalBagsLabel->setText(QString::number(grandBags));
    m_totalWeightLabel->setText(QString("%1 Qtl").arg(QString::number(grandWt, 'f', 3)));
    m_goodsAmountLabel->setText(QString("₹ %1").arg(QString::number(grandGoods, 'f', 2)));

    double labour = m_labourEdit ? m_labourEdit->text().toDouble() : 0.0;
    double bonus = m_bonusEdit ? m_bonusEdit->text().toDouble() : 0.0;
    double roundOff = m_roundOffEdit ? m_roundOffEdit->text().toDouble() : 0.0;

    double netPayable = grandGoods - labour + bonus + roundOff;
    m_grandTotalLabel->setText(QString("₹ %1").arg(QString::number(netPayable, 'f', 2)));
}

void JFormVoucherWidget::saveVoucher() {
    if (m_selectedFarmerName.isEmpty()) {
        CustomMessageBox::warning(this, "Missing Farmer", "Please select a Farmer / Zimidar.");
        m_farmerSearch->setFocus();
        return;
    }

    QVariantList items;
    for (int r = 0; r < m_table->rowCount(); ++r) {
        QString iName = m_table->item(r, 0) ? m_table->item(r, 0)->text().trimmed() : "";
        if (iName.isEmpty()) continue;
        int bags = m_table->item(r, 1) ? m_table->item(r, 1)->text().toInt() : 0;
        double pack = m_table->item(r, 2) ? m_table->item(r, 2)->text().toDouble() : 0.500;
        double loose = m_table->item(r, 3) ? m_table->item(r, 3)->text().toDouble() : 0.0;
        double wt = m_table->item(r, 4) ? m_table->item(r, 4)->text().toDouble() : 0.0;
        double rate = m_table->item(r, 5) ? m_table->item(r, 5)->text().toDouble() : 0.0;
        double amt = m_table->item(r, 6) ? m_table->item(r, 6)->text().toDouble() : 0.0;

        QVariantMap rowMap;
        rowMap["item_name"] = iName;
        rowMap["bags"] = bags;
        rowMap["packing"] = pack;
        rowMap["loose_weight"] = loose;
        rowMap["weight"] = wt;
        rowMap["rate"] = rate;
        rowMap["amount"] = amt;
        items.append(rowMap);
    }

    if (items.isEmpty()) {
        CustomMessageBox::warning(this, "Empty Items", "Please add at least one line item.");
        return;
    }

    QVariantMap headerData;
    if (isEditMode()) headerData["id"] = m_editingVoucherId;
    headerData["voucher_date"] = m_dateEdit->date().toString("yyyy-MM-dd");
    headerData["jform_no"] = m_jformNoEdit->text().trimmed();
    headerData["zimidar_id"] = m_selectedFarmerId;
    headerData["zimidar_name"] = m_selectedFarmerName;
    headerData["auction_sale_status"] = m_saleStatusCombo->currentText();
    headerData["due_days"] = m_dueDaysEdit->text().toInt();
    headerData["vehicle_no"] = m_vehicleNoEdit->text().trimmed();
    headerData["broker_name"] = m_brokerNameEdit->text().trimmed();
    headerData["narration"] = m_narrationEdit->text().trimmed();

    int grandBags = 0;
    double grandWt = 0.0;
    double grandGoods = 0.0;
    for (const auto& itVar : items) {
        QVariantMap it = itVar.toMap();
        grandBags += it.value("bags").toInt();
        grandWt += it.value("weight").toDouble();
        grandGoods += it.value("amount").toDouble();
    }

    double labour = m_labourEdit->text().toDouble();
    double bonus = m_bonusEdit->text().toDouble();
    double roundOff = m_roundOffEdit->text().toDouble();
    double grandTotal = grandGoods - labour + bonus + roundOff;

    headerData["total_bags"] = grandBags;
    headerData["total_weight"] = grandWt;
    headerData["goods_amount"] = grandGoods;
    headerData["labour_amount"] = labour;
    headerData["bonus_amount"] = bonus;
    headerData["round_off"] = roundOff;
    headerData["grand_total"] = grandTotal;

    bool ok = m_jformModel.save_jform_voucher(headerData, items);
    if (ok) {
        QString savedJf = headerData["jform_no"].toString();
        emit voucherSaved(savedJf);
        CustomMessageBox::information(this, "Saved", QString("J-Form No. %1 saved successfully.").arg(savedJf));
        resetForm();
    } else {
        CustomMessageBox::critical(this, "Error", "Failed to save J-Form voucher. Check database error logs.");
    }
}

bool JFormVoucherWidget::loadVoucherForEditing(const QVariant& vchNoOrId, const QString& dateHint) {
    Q_UNUSED(dateHint);
    QVariantMap vch = m_jformModel.get_jform_voucher(vchNoOrId);
    if (vch.isEmpty()) return false;

    m_editingVoucherId = vch.value("id").toInt();
    m_editingJFormNo = vch.value("jform_no").toString();
    m_editingVoucherNo = QString::number(vch.value("voucher_no").toInt());
    m_deleteButton->setEnabled(true);

    QDate d = QDate::fromString(vch.value("voucher_date").toString(), "yyyy-MM-dd");
    if (d.isValid()) m_dateEdit->setDate(d);

    m_voucherNoDisplay->setText(QString("Vch: JFrm-%1").arg(m_editingVoucherNo));
    m_jformNoEdit->setText(m_editingJFormNo);

    m_selectedFarmerId = vch.value("zimidar_id").toInt();
    m_selectedFarmerName = vch.value("zimidar_name").toString();
    m_farmerSearch->setText(m_selectedFarmerName);

    QVariantMap bal = m_jformModel.get_zimidar_balance(m_selectedFarmerId);
    m_farmerBalanceLabel->setText(bal.value("formatted_balance").toString());

    m_dueDaysEdit->setText(QString::number(vch.value("due_days").toInt()));
    m_vehicleNoEdit->setText(vch.value("vehicle_no").toString());
    m_brokerNameEdit->setText(vch.value("broker_name").toString());
    m_narrationEdit->setText(vch.value("narration").toString());

    m_labourEdit->setText(QString::number(vch.value("labour_amount").toDouble(), 'f', 2));
    m_bonusEdit->setText(QString::number(vch.value("bonus_amount").toDouble(), 'f', 2));
    m_roundOffEdit->setText(QString::number(vch.value("round_off").toDouble(), 'f', 2));

    QVariantList items = vch.value("items").toList();
    m_table->setRowCount(0);
    for (const auto& itVar : items) {
        QVariantMap it = itVar.toMap();
        int r = m_table->rowCount();
        m_table->insertRow(r);
        m_table->setItem(r, 0, new QTableWidgetItem(it.value("item_name").toString()));
        m_table->setItem(r, 1, new QTableWidgetItem(QString::number(it.value("bags").toInt())));
        m_table->setItem(r, 2, new QTableWidgetItem(QString::number(it.value("packing").toDouble(), 'f', 3)));
        m_table->setItem(r, 3, new QTableWidgetItem(QString::number(it.value("loose_weight").toDouble(), 'f', 3)));
        m_table->setItem(r, 4, new QTableWidgetItem(QString::number(it.value("weight").toDouble(), 'f', 3)));
        m_table->setItem(r, 5, new QTableWidgetItem(QString::number(it.value("rate").toDouble(), 'f', 2)));
        m_table->setItem(r, 6, new QTableWidgetItem(QString::number(it.value("amount").toDouble(), 'f', 2)));
    }
    if (m_table->rowCount() == 0) addNewLineRow();

    recalculateTotals();
    setStatusMessage(QString("Loaded J-Form %1 for editing").arg(m_editingJFormNo), false);
    return true;
}

void JFormVoucherWidget::deleteVoucher() {
    if (!isEditMode()) return;
    if (CustomMessageBox::question(this, "Confirm Delete", QString("Delete J-Form No. %1 permanently?").arg(m_editingJFormNo))) {
        bool ok = m_jformModel.delete_jform_voucher(m_editingVoucherId);
        if (ok) {
            emit voucherDeleted(m_editingJFormNo);
            CustomMessageBox::information(this, "Deleted", "J-Form voucher deleted.");
            resetForm();
        }
    }
}

void JFormVoucherWidget::openDateDialog(bool isInitial) {
    QString curDate = m_dateEdit ? m_dateEdit->date().toString("dd-MM-yyyy") : "";
    QString newDisp, newIso;
    if (VoucherDateDialog::getVoucherDate(this, curDate, &newDisp, &newIso)) {
        if (m_dateEdit) {
            m_dateEdit->setDate(QDate::fromString(newIso, "yyyy-MM-dd"));
        }
        onDateChanged(QDate::fromString(newIso, "yyyy-MM-dd"));
        if (m_farmerSearch) {
            m_farmerSearch->setFocus();
            m_farmerSearch->selectAll();
        }
    } else {
        if (isInitial) {
            emit backRequested();
        } else if (m_farmerSearch) {
            m_farmerSearch->setFocus();
        }
    }
}

void JFormVoucherWidget::openAlterVoucherDialog() {
    bool ok = false;
    QString text = CustomInputDialog::getText(this, "Find & Alter J-Form Voucher", "Enter J-Form No or Voucher No to alter:", m_editingJFormNo, &ok);
    if (ok && !text.trimmed().isEmpty()) {
        if (!loadVoucherForEditing(text.trimmed())) {
            CustomMessageBox::warning(this, "Voucher Not Found", QString("No J-Form voucher found matching \"%1\".").arg(text.trimmed()));
        }
    }
}

void JFormVoucherWidget::loadPreviousVoucher() {
    QVariantMap prev = m_jformModel.get_previous_jform_voucher(m_editingVoucherId, m_jformNoEdit ? m_jformNoEdit->text().trimmed() : "");
    if (!prev.isEmpty()) {
        loadVoucherForEditing(prev.value("id"));
    }
}

void JFormVoucherWidget::loadNextVoucher() {
    if (m_editingVoucherId <= 0) return;
    QVariantMap next = m_jformModel.get_next_jform_voucher(m_editingVoucherId, m_jformNoEdit ? m_jformNoEdit->text().trimmed() : "");
    if (!next.isEmpty()) {
        loadVoucherForEditing(next.value("id"));
    } else {
        resetForm();
    }
}

void JFormVoucherWidget::printVoucher() {}
void JFormVoucherWidget::exportPdf() {}

void JFormVoucherWidget::focusTableAt(int row, int col) {
    if (!m_table) return;
    if (row < 0 || row >= m_table->rowCount()) return;
    if (col < 0 || col >= m_table->columnCount()) return;
    m_table->setCurrentCell(row, col);
    m_table->edit(m_table->currentIndex());
}

void JFormVoucherWidget::moveCell(int row, int col) {
    focusTableAt(row, col);
}

void JFormVoucherWidget::advanceCell() {
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
    // Skip read-only columns (column 4 Total Wt and column 6 Amount)
    if (nextCol == 4) nextCol = 5;
    if (nextCol == 6) nextCol = 7;

    if (nextCol <= 5) {
        m_table->setCurrentCell(curRow, nextCol);
        m_table->edit(m_table->currentIndex());
    } else {
        // End of row
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

void JFormVoucherWidget::retreatCell() {
    if (!m_table) return;
    int curRow = m_table->currentRow();
    int curCol = m_table->currentColumn();

    int prevCol = curCol - 1;
    if (prevCol == 4) prevCol = 3;

    if (prevCol >= 0) {
        m_table->setCurrentCell(curRow, prevCol);
        m_table->edit(m_table->currentIndex());
    } else if (curRow > 0) {
        m_table->setCurrentCell(curRow - 1, 5);
        m_table->edit(m_table->currentIndex());
    } else {
        if (m_brokerNameEdit) {
            m_brokerNameEdit->setFocus();
            m_brokerNameEdit->selectAll();
        }
    }
}

void JFormVoucherWidget::showEvent(QShowEvent* event) {
    QWidget::showEvent(event);
    if (m_farmerSearch) {
        m_farmerSearch->setFocus();
    }
}

void JFormVoucherWidget::keyPressEvent(QKeyEvent* event) {
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

bool JFormVoucherWidget::eventFilter(QObject* watched, QEvent* event) {
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
            if (watched == m_jformNoEdit) {
                m_farmerSearch->setFocus();
                return true;
            } else if (watched == m_farmerSearch) {
                if (!m_farmerSearch->isPopupVisible()) {
                    m_saleStatusCombo->setFocus();
                    return true;
                }
            } else if (watched == m_saleStatusCombo) {
                m_dueDaysEdit->setFocus();
                m_dueDaysEdit->selectAll();
                return true;
            } else if (watched == m_dueDaysEdit) {
                m_vehicleNoEdit->setFocus();
                m_vehicleNoEdit->selectAll();
                return true;
            } else if (watched == m_vehicleNoEdit) {
                m_brokerNameEdit->setFocus();
                m_brokerNameEdit->selectAll();
                return true;
            } else if (watched == m_brokerNameEdit) {
                if (m_table->rowCount() == 0) addNewLineRow();
                focusTableAt(0, 0);
                return true;
            } else if (watched == m_labourEdit) {
                m_bonusEdit->setFocus();
                m_bonusEdit->selectAll();
                return true;
            } else if (watched == m_bonusEdit) {
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

void JFormVoucherWidget::setStatusMessage(const QString& message, bool isError) {
    m_statusLabel->setText(message);
    m_statusLabel->setStyleSheet(isError ? "color: red; font-weight: bold;" : "color: #475569;");
}
