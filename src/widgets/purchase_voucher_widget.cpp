#include "purchase_voucher_widget.h"
#include "voucher_date_dialog.h"
#include "custom_dialogs.h"
#include "voucher_common.h"
#include "../engine/fiscal_year_helper.h"
#include "../engine/accounting_engine.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QHeaderView>
#include <QMessageBox>
#include <QInputDialog>
#include <QScrollBar>
#include <QKeyEvent>
#include <QApplication>
#include <QDate>
#include <QTimer>
#include <cmath>
#include <QDebug>

// ============================================================================
// PurchaseVoucherWidget Implementation (Authentic Bahi-Khata Replication)
// ============================================================================

PurchaseVoucherWidget::PurchaseVoucherWidget(PrintExportController* printExportCtrl, QWidget* parent)
    : QWidget(parent)
    , m_printExportCtrl(printExportCtrl)
{
    setupUi();
    applyCustomStyles();
    setupNavigationChains();
    resetForm();
}

PurchaseVoucherWidget::~PurchaseVoucherWidget() {
}

void PurchaseVoucherWidget::setupUi() {
    setAttribute(Qt::WA_StyledBackground, true);
    setStyleSheet(
        "PurchaseVoucherWidget { background-color: #F8FAFC; font-family: 'Segoe UI', -apple-system, BlinkMacSystemFont, Roboto, 'Helvetica Neue', 'Noto Sans', 'Liberation Sans', Arial, sans-serif; }"
        "QLabel { border: none; background: transparent; color: #1E293B; font-size: 11px; font-weight: 700; }"
        "QLineEdit { background-color: #FFFFFF; color: #0F172A; border: 1px solid #CBD5E1; border-radius: 6px; padding: 2px 6px; font-size: 11.5px; font-weight: 700; }"
        "QLineEdit:focus { border: 1.5px solid #2563EB; background-color: #EFF6FF; }"
        "QComboBox { background-color: #FFFFFF; color: #0F172A; border: 1px solid #CBD5E1; border-radius: 6px; padding: 2px 6px; font-size: 11.5px; font-weight: 700; }"
        "QComboBox:focus { border: 1.5px solid #2563EB; background-color: #EFF6FF; }"
        "QRadioButton { font-size: 11px; font-weight: 700; color: #1E293B; spacing: 5px; background: transparent; }"
        "QRadioButton:hover { color: #0F172A; }"
        "QRadioButton::indicator { width: 14px; height: 14px; border-radius: 7px; border: 1.5px solid #94A3B8; background-color: #FFFFFF; }"
        "QRadioButton::indicator:hover { border-color: #16A34A; background-color: #F0FDF4; }"
        "QRadioButton::indicator:checked { border: 4.5px solid #16A34A; background-color: #FFFFFF; }"
        "QRadioButton::indicator:checked:hover { border-color: #15803D; }"
        "QCheckBox { font-size: 11px; font-weight: 700; color: #334155; spacing: 4px; background: transparent; }"
        "QCheckBox::indicator { width: 13px; height: 13px; background: #FFFFFF; border: 1px solid #CBD5E1; border-radius: 3px; }"
        "QGroupBox { font-size: 10.5px; font-weight: 800; color: #1E293B; border: 1px solid #CBD5E1; border-radius: 6px; margin-top: 6px; padding: 2px 8px 3px 8px; background-color: #F8FAFC; }"
        "QGroupBox::title { subcontrol-origin: margin; subcontrol-position: top left; padding: 0 4px; left: 8px; color: #16A34A; font-weight: 800; }"
        "QPushButton { background-color: #F1F5F9; border: 1px solid #CBD5E1; border-radius: 6px; font-size: 11px; font-weight: 700; color: #475569; padding: 3px 10px; }"
        "QPushButton:hover { background-color: #E2E8F0; }"
        "QPushButton:focus { border: 1.5px solid #2563EB; background-color: #EFF6FF; }"
    );

    QVBoxLayout* rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(10, 6, 10, 6);
    rootLayout->setSpacing(4);

    // ========================================================================
    // 1. TOP HEADER BAR: Modern Badge + No. + Title + Date & Day
    // ========================================================================
    QHBoxLayout* topHeaderLayout = new QHBoxLayout();
    topHeaderLayout->setSpacing(8);

    m_purchaseBadge = new QLabel("Purchase", this);
    m_purchaseBadge->setAlignment(Qt::AlignCenter);
    m_purchaseBadge->setFixedHeight(24);
    m_purchaseBadge->setStyleSheet("background-color: #FEF2F2; color: #DC2626; font-weight: 900; font-size: 12px; padding: 2px 10px; border: 1px solid #FECACA; border-radius: 6px;");
    topHeaderLayout->addWidget(m_purchaseBadge);

    m_voucherNoDisplay = new QLabel("No. 1", this);
    m_voucherNoDisplay->setStyleSheet("font-size: 13px; font-weight: 900; color: #0F172A;");
    topHeaderLayout->addWidget(m_voucherNoDisplay);

    topHeaderLayout->addStretch(1);

    m_titleHeaderLabel = new QLabel("F9 : Purchase Voucher", this);
    m_titleHeaderLabel->setAlignment(Qt::AlignCenter);
    m_titleHeaderLabel->setStyleSheet("font-size: 15px; font-weight: 900; color: #0F172A; font-family: 'Segoe UI', -apple-system, BlinkMacSystemFont, Roboto, 'Helvetica Neue', 'Noto Sans', 'Liberation Sans', Arial, sans-serif;");
    topHeaderLayout->addWidget(m_titleHeaderLabel);

    topHeaderLayout->addStretch(1);

    // Date & Day of Week
    QVBoxLayout* dateBox = new QVBoxLayout();
    dateBox->setSpacing(0);
    dateBox->setAlignment(Qt::AlignRight);

    m_invoiceDateEdit = new AccountingDateEdit(this);
    m_invoiceDateEdit->setFixedHeight(24);
    m_invoiceDateEdit->setFixedWidth(105);
    m_invoiceDateEdit->setStyleSheet("QLineEdit { background-color: #FFFFFF; color: #0F172A; font-size: 12px; font-weight: 800; border: 1px solid #CBD5E1; border-radius: 6px; }");
    connect(m_invoiceDateEdit, &AccountingDateEdit::dateChanged, this, &PurchaseVoucherWidget::onDateChanged);
    dateBox->addWidget(m_invoiceDateEdit);

    m_dayOfWeekLabel = new QLabel(this);
    m_dayOfWeekLabel->setAlignment(Qt::AlignRight);
    m_dayOfWeekLabel->setStyleSheet("font-size: 11px; font-weight: 700; color: #0284C7;");
    dateBox->addWidget(m_dayOfWeekLabel);
    topHeaderLayout->addLayout(dateBox);

    rootLayout->addLayout(topHeaderLayout);

    // Sub-Header Shortcut / Help Line + Rev.Charge
    QHBoxLayout* subBarLayout = new QHBoxLayout();
    subBarLayout->setSpacing(10);

    m_revChargeCheck = new QCheckBox("Rev.Charge By Recipient", this);
    subBarLayout->addWidget(m_revChargeCheck);

    m_helpHintLabel = new QLabel("Change Date - F9 : Change Purchase Mode - Shift+F9: Purchase Settings - Alt+L:Show Ledger - Ctrl+F1: Set Image)", this);
    m_helpHintLabel->setStyleSheet("color: #4A1500; font-size: 10.5px; font-weight: 700;");
    subBarLayout->addWidget(m_helpHintLabel);

    subBarLayout->addStretch(1);
    rootLayout->addLayout(subBarLayout);

    // ========================================================================
    // 2. META ROW 1: Market Type, Bill No., Due Days, Tax Status, Mandi Statuses
    // ========================================================================
    QHBoxLayout* row1 = new QHBoxLayout();
    row1->setSpacing(8);

    // Market Type Dropdown
    m_marketTypeCombo = new QComboBox(this);
    m_marketTypeCombo->addItems({"Market Type (With Stock)", "Market Type (Without Stock)", "Mandi Type"});
    m_marketTypeCombo->setFixedHeight(24);
    m_marketTypeCombo->setFixedWidth(190);
    connect(m_marketTypeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &PurchaseVoucherWidget::onMarketTypeChanged);
    row1->addWidget(m_marketTypeCombo);

    // Bill / Invoice No
    QLabel* invLbl = new QLabel("Bill No.:", this);
    row1->addWidget(invLbl);
    m_invoiceNoEdit = new QLineEdit(this);
    m_invoiceNoEdit->setFixedHeight(24);
    m_invoiceNoEdit->setFixedWidth(110);
    row1->addWidget(m_invoiceNoEdit);

    // Due Days
    QVBoxLayout* dueBox = new QVBoxLayout();
    dueBox->setSpacing(0);
    QLabel* dueHint = new QLabel("(Ctrl+D)", this);
    dueHint->setStyleSheet("font-size: 9px; font-weight: 700; color: #000000;");
    QLabel* dueLbl = new QLabel("Due Days :", this);
    dueBox->addWidget(dueHint);
    dueBox->addWidget(dueLbl);
    row1->addLayout(dueBox);

    m_dueDaysEdit = new QLineEdit("0", this);
    m_dueDaysEdit->setFixedHeight(24);
    m_dueDaysEdit->setFixedWidth(42);
    m_dueDaysEdit->setAlignment(Qt::AlignCenter);
    row1->addWidget(m_dueDaysEdit);

    // Mandi Sale Status (Mandi Only)
    m_saleStatusGroupBox = new QGroupBox("Purchase Status (Alt+S)", this);
    QHBoxLayout* saleStatLayout = new QHBoxLayout(m_saleStatusGroupBox);
    saleStatLayout->setContentsMargins(4, 2, 4, 2);
    saleStatLayout->setSpacing(6);

    m_saleButtonGroup = new QButtonGroup(this);
    m_saleSelfRadio = new QRadioButton("Self Sale", m_saleStatusGroupBox);
    m_saleTransferRadio = new QRadioButton("Stock Transfer", m_saleStatusGroupBox);
    m_saleLagatRadio = new QRadioButton("Lagat Bill", m_saleStatusGroupBox);
    m_saleThirdPartyRadio = new QRadioButton("Third Party", m_saleStatusGroupBox);

    m_saleSelfRadio->setChecked(true);
    m_saleButtonGroup->addButton(m_saleSelfRadio, 0);
    m_saleButtonGroup->addButton(m_saleTransferRadio, 1);
    m_saleButtonGroup->addButton(m_saleLagatRadio, 2);
    m_saleButtonGroup->addButton(m_saleThirdPartyRadio, 3);

    saleStatLayout->addWidget(m_saleSelfRadio);
    saleStatLayout->addWidget(m_saleTransferRadio);
    saleStatLayout->addWidget(m_saleLagatRadio);
    saleStatLayout->addWidget(m_saleThirdPartyRadio);
    connect(m_saleButtonGroup, &QButtonGroup::idClicked, this, &PurchaseVoucherWidget::onSaleStatusChanged);
    row1->addWidget(m_saleStatusGroupBox);

    // Market Fee Status (Mandi Only)
    m_marketFeeStatusGroupBox = new QGroupBox("Market Fee Status (Alt+F)", this);
    QHBoxLayout* feeStatLayout = new QHBoxLayout(m_marketFeeStatusGroupBox);
    feeStatLayout->setContentsMargins(6, 2, 6, 2);
    feeStatLayout->setSpacing(8);

    m_feeButtonGroup = new QButtonGroup(this);
    m_feePayableRadio = new QRadioButton("Payable", m_marketFeeStatusGroupBox);
    m_feePaidRadio = new QRadioButton("Paid", m_marketFeeStatusGroupBox);
    m_feePaidRadio->setChecked(true);
    m_feeButtonGroup->addButton(m_feePayableRadio, 0);
    m_feeButtonGroup->addButton(m_feePaidRadio, 1);

    feeStatLayout->addWidget(m_feePayableRadio);
    feeStatLayout->addWidget(m_feePaidRadio);
    connect(m_feeButtonGroup, &QButtonGroup::idClicked, this, &PurchaseVoucherWidget::onMarketFeeStatusChanged);
    row1->addWidget(m_marketFeeStatusGroupBox);

    // Push Tax Status to the right side corner
    row1->addStretch(1);

    // Tax Status (Always visible, aligned to right side corner)
    m_taxStatusGroupBox = new QGroupBox("Tax Status : (Alt+R / Alt+T)", this);
    QHBoxLayout* taxStatLayout = new QHBoxLayout(m_taxStatusGroupBox);
    taxStatLayout->setContentsMargins(8, 2, 8, 2);
    taxStatLayout->setSpacing(10);

    m_taxButtonGroup = new QButtonGroup(this);
    m_taxGstRadio = new QRadioButton("GST / Exempt", m_taxStatusGroupBox);
    m_taxIgstRadio = new QRadioButton("IGST", m_taxStatusGroupBox);
    m_taxExportRadio = new QRadioButton("Export", m_taxStatusGroupBox);

    m_taxGstRadio->setChecked(true);
    m_taxButtonGroup->addButton(m_taxGstRadio, 0);
    m_taxButtonGroup->addButton(m_taxIgstRadio, 1);
    m_taxButtonGroup->addButton(m_taxExportRadio, 2);

    taxStatLayout->addWidget(m_taxGstRadio);
    taxStatLayout->addWidget(m_taxIgstRadio);
    taxStatLayout->addWidget(m_taxExportRadio);
    connect(m_taxButtonGroup, &QButtonGroup::idClicked, this, &PurchaseVoucherWidget::onTaxStatusChanged);
    row1->addWidget(m_taxStatusGroupBox);

    rootLayout->addLayout(row1);

    // ========================================================================
    // 3. META ROW 2: Party Search + Date Bal + GSTIN
    // ========================================================================
    QHBoxLayout* row2 = new QHBoxLayout();
    row2->setSpacing(6);

    m_partyTagLabel = new QLabel("Purchase From Party :", this);
    m_partyTagLabel->setStyleSheet("color: #1E293B; font-size: 12px; font-weight: 800;");
    row2->addWidget(m_partyTagLabel);

    m_partySearchWidget = new AccountSearchBox(this);
    m_partySearchWidget->setFixedHeight(26);
    connect(m_partySearchWidget, &AccountSearchBox::partyDataSelected, this, &PurchaseVoucherWidget::onPartySelected);
    row2->addWidget(m_partySearchWidget, 1);

    rootLayout->addLayout(row2);

    // Sub-row under party: Date Bal & GSTIN
    QHBoxLayout* partySubRow = new QHBoxLayout();
    partySubRow->setSpacing(20);
    partySubRow->setContentsMargins(10, 0, 0, 0);

    m_partyBalLabel = new QLabel("Date Bal. 0.00", this);
    m_partyBalLabel->setStyleSheet("font-size: 11px; font-weight: 700; color: #64748B; font-style: italic;");
    partySubRow->addWidget(m_partyBalLabel);

    QHBoxLayout* gstinLayout = new QHBoxLayout();
    gstinLayout->setSpacing(4);
    QLabel* gstinTag = new QLabel("GSTIN :", this);
    gstinLayout->addWidget(gstinTag);
    m_gstinDisplay = new QLineEdit(this);
    m_gstinDisplay->setReadOnly(true);
    m_gstinDisplay->setFixedHeight(20);
    m_gstinDisplay->setFixedWidth(160);
    m_gstinDisplay->setStyleSheet("QLineEdit { background: transparent; border: none; font-weight: 800; color: #0F172A; }");
    gstinLayout->addWidget(m_gstinDisplay);
    partySubRow->addLayout(gstinLayout);

    partySubRow->addStretch(1);

    // Including Tax Checkbox (Above Table)
    m_inclTaxCheck = new QCheckBox("Including Tax (Alt+I)", this);
    m_inclTaxCheck->setStyleSheet("color: #2563EB; font-weight: 800;");
    connect(m_inclTaxCheck, &QCheckBox::toggled, this, &PurchaseVoucherWidget::recalculateTotals);
    partySubRow->addWidget(m_inclTaxCheck);

    rootLayout->addLayout(partySubRow);

    // ========================================================================
    // 4. LINE ITEMS TABLE & TOTAL SUMMARY ROW
    // ========================================================================
    m_tableWidget = new QTableWidget(this);
    m_tableWidget->setColumnCount(10);
    m_tableWidget->setHorizontalHeaderLabels({
        "No.", "Item Name", "Grade", "Bags", "Pkng.", "Weight", "GST", "Rate", "Amount", "Act"
    });

    m_tableWidget->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Fixed);
    m_tableWidget->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Interactive);
    m_tableWidget->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);
    m_tableWidget->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Fixed);
    m_tableWidget->horizontalHeader()->setSectionResizeMode(4, QHeaderView::Fixed);
    m_tableWidget->horizontalHeader()->setSectionResizeMode(5, QHeaderView::Fixed);
    m_tableWidget->horizontalHeader()->setSectionResizeMode(6, QHeaderView::Fixed);
    m_tableWidget->horizontalHeader()->setSectionResizeMode(7, QHeaderView::Fixed);
    m_tableWidget->horizontalHeader()->setSectionResizeMode(8, QHeaderView::Fixed);
    m_tableWidget->horizontalHeader()->setSectionResizeMode(9, QHeaderView::Fixed);

    m_tableWidget->setColumnWidth(0, 35);
    m_tableWidget->setColumnWidth(1, 230);
    m_tableWidget->setColumnWidth(3, 65);
    m_tableWidget->setColumnWidth(4, 65);
    m_tableWidget->setColumnWidth(5, 85);
    m_tableWidget->setColumnWidth(6, 50);
    m_tableWidget->setColumnWidth(7, 95);
    m_tableWidget->setColumnWidth(8, 115);
    m_tableWidget->setColumnWidth(9, 35);

    m_tableWidget->verticalHeader()->setVisible(false);
    m_tableWidget->setSelectionBehavior(QAbstractItemView::SelectItems);
    m_tableWidget->setSelectionMode(QAbstractItemView::SingleSelection);
    m_tableWidget->setShowGrid(true);
    m_tableWidget->setMinimumHeight(120);
    m_tableWidget->setMaximumHeight(220);
    m_tableWidget->setStyleSheet(
        "QTableWidget {"
        "  background-color: #FFFFFF;"
        "  gridline-color: #F1F5F9;"
        "  font-family: 'Segoe UI', -apple-system, BlinkMacSystemFont, Roboto, 'Helvetica Neue', 'Noto Sans', 'Liberation Sans', Arial, sans-serif;"
        "  font-size: 11.5px;"
        "  color: #0F172A;"
        "  border: 1px solid #E2E8F0;"
        "  border-radius: 8px;"
        "}"
        "QHeaderView::section {"
        "  background-color: #F1F5F9;"
        "  color: #334155;"
        "  font-weight: 800;"
        "  font-size: 11px;"
        "  padding: 4px 6px;"
        "  border: none;"
        "  border-bottom: 2px solid #CBD5E1;"
        "}"
        "QTableWidget::item:selected {"
        "  background-color: #EFF6FF;"
        "  color: #2563EB;"
        "  font-weight: 800;"
        "}"
    );

    m_itemDelegate = new ItemSearchDelegate(this);
    m_tableWidget->setItemDelegate(m_itemDelegate);

    connect(m_itemDelegate, &ItemSearchDelegate::stockItemConfigured, this, &PurchaseVoucherWidget::onStockItemConfigured);
    connect(m_itemDelegate, &ItemSearchDelegate::moveNextRequested, this, &PurchaseVoucherWidget::advanceCell);
    connect(m_itemDelegate, &ItemSearchDelegate::movePrevRequested, this, &PurchaseVoucherWidget::retreatCell);

    connect(m_tableWidget, &QTableWidget::cellChanged, this, &PurchaseVoucherWidget::onTableCellChanged);
    rootLayout->addWidget(m_tableWidget);

    // Table Total Summary Row
    QFrame* totalSummaryBar = new QFrame(this);
    totalSummaryBar->setFixedHeight(28);
    totalSummaryBar->setStyleSheet("background: #F8FAFC; border-top: 1px solid #E2E8F0; border-bottom: 1px solid #E2E8F0; border-radius: 4px;");
    QHBoxLayout* sumLayout = new QHBoxLayout(totalSummaryBar);
    sumLayout->setContentsMargins(6, 2, 6, 2);
    sumLayout->setSpacing(0);

    sumLayout->addStretch(1);
    QLabel* totTitle = new QLabel("Total :", totalSummaryBar);
    totTitle->setStyleSheet("font-size: 12px; font-weight: 900; color: #000000;");
    sumLayout->addWidget(totTitle);
    sumLayout->addSpacing(8);

    m_totalBagsLabel = new QLabel("0", totalSummaryBar);
    m_totalBagsLabel->setFixedWidth(65);
    m_totalBagsLabel->setAlignment(Qt::AlignCenter);
    m_totalBagsLabel->setStyleSheet("font-size: 12px; font-weight: 900; color: #000000;");
    sumLayout->addWidget(m_totalBagsLabel);

    sumLayout->addSpacing(65); // Pkng spacing

    m_totalWeightLabel = new QLabel("0.000", totalSummaryBar);
    m_totalWeightLabel->setFixedWidth(85);
    m_totalWeightLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    m_totalWeightLabel->setStyleSheet("font-size: 12px; font-weight: 900; color: #000000;");
    sumLayout->addWidget(m_totalWeightLabel);

    sumLayout->addSpacing(145); // GST & Rate spacing

    m_subtotalTaxableLabel = new QLabel("0.00", totalSummaryBar);
    m_subtotalTaxableLabel->setFixedWidth(115);
    m_subtotalTaxableLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    m_subtotalTaxableLabel->setStyleSheet("font-size: 12px; font-weight: 900; color: #000000;");
    sumLayout->addWidget(m_subtotalTaxableLabel);

    sumLayout->addSpacing(35);
    rootLayout->addWidget(totalSummaryBar);

    // ========================================================================
    // 5. MANDI EXPENSES STRIP (2 Rows, visible only in Mandi Type)
    // ========================================================================
    m_mandiExpensesFrame = new QFrame(this);
    m_mandiExpensesFrame->setStyleSheet("background: transparent; border: none;");
    QVBoxLayout* mandiVBox = new QVBoxLayout(m_mandiExpensesFrame);
    mandiVBox->setContentsMargins(0, 2, 0, 2);
    mandiVBox->setSpacing(3);

    auto makeMandiInput = [this](const QString& lbl, QLineEdit*& member, const QString& defVal = "") -> QHBoxLayout* {
        QHBoxLayout* box = new QHBoxLayout();
        box->setSpacing(3);
        QLabel* l = new QLabel(lbl, m_mandiExpensesFrame);
        l->setStyleSheet("font-size: 11px; font-weight: 700; color: #000000;");
        box->addWidget(l);
        member = new QLineEdit(defVal, m_mandiExpensesFrame);
        member->setFixedWidth(70);
        member->setFixedHeight(22);
        member->setAlignment(Qt::AlignRight);
        connect(member, &QLineEdit::textChanged, this, &PurchaseVoucherWidget::recalculateTotals);
        box->addWidget(member);
        return box;
    };

    // Mandi Row 1
    QHBoxLayout* mRow1 = new QHBoxLayout();
    mRow1->setSpacing(8);
    mRow1->addLayout(makeMandiInput("Dami :", m_damiEdit, "0.00"));
    mRow1->addLayout(makeMandiInput("Labour :", m_labourEdit));
    mRow1->addLayout(makeMandiInput("Auction :", m_auctionEdit));
    mRow1->addLayout(makeMandiInput("M. Fee :", m_mFeeEdit));
    mRow1->addLayout(makeMandiInput("H.R.D.F. :", m_hrdfEdit));
    mRow1->addStretch(1);
    mandiVBox->addLayout(mRow1);

    // Mandi Row 2
    QHBoxLayout* mRow2 = new QHBoxLayout();
    mRow2->setSpacing(8);
    mRow2->addLayout(makeMandiInput("Other Exp. :", m_mandiOtherExpEdit));
    mRow2->addLayout(makeMandiInput("Welfare :", m_welfareEdit));
    mRow2->addLayout(makeMandiInput("Dhrmd. :", m_dhrmdEdit));
    mRow2->addLayout(makeMandiInput("Sutli :", m_sutliEdit, "0.00"));
    mRow2->addLayout(makeMandiInput("(-) Less :", m_mandiLessEdit));
    mRow2->addStretch(1);
    mandiVBox->addLayout(mRow2);

    m_mandiExpensesFrame->setVisible(false);
    rootLayout->addWidget(m_mandiExpensesFrame);

    // ========================================================================
    // 6. BOTTOM SPLIT SECTION: Logistics Matrix (Left) + Summary Panel (Right)
    // ========================================================================
    QHBoxLayout* bottomSplitLayout = new QHBoxLayout();
    bottomSplitLayout->setSpacing(10);

    // Left Column: Logistics Matrix + Narration Directly Below It
    QVBoxLayout* leftColLayout = new QVBoxLayout();
    leftColLayout->setSpacing(4);

    QFrame* logMatrixFrame = new QFrame(this);
    logMatrixFrame->setStyleSheet("QFrame { background: transparent; border: 1px solid #7F9DB9; }");
    QVBoxLayout* logVBox = new QVBoxLayout(logMatrixFrame);
    logVBox->setContentsMargins(6, 6, 6, 6);
    logVBox->setSpacing(4);

    auto makeLogItem = [logMatrixFrame](const QString& lbl, QWidget* w, bool isVeh = false, bool isKanda = false) -> QHBoxLayout* {
        QHBoxLayout* row = new QHBoxLayout();
        row->setSpacing(3);
        QLabel* l = new QLabel(lbl, logMatrixFrame);
        if (isKanda) l->setStyleSheet("color: #0000CC; text-decoration: underline; font-weight: 800; font-size: 11px;");
        else l->setStyleSheet("color: #000000; font-weight: 700; font-size: 11px;");
        row->addWidget(l);
        if (isVeh) w->setStyleSheet("QLineEdit { background: #D5F5D5; color: #000000; border: 1px solid #7F9DB9; font-weight: 800; }");
        row->addWidget(w, 1);
        return row;
    };

    m_vehicleNoEdit = new QLineEdit(logMatrixFrame); m_vehicleNoEdit->setFixedHeight(24);
    m_grNoEdit = new QLineEdit(logMatrixFrame); m_grNoEdit->setFixedHeight(24);
    m_driverNameEdit = new QLineEdit(logMatrixFrame); m_driverNameEdit->setFixedHeight(24);
    m_ewayBillNoEdit = new QLineEdit(logMatrixFrame); m_ewayBillNoEdit->setFixedHeight(24);

    m_billTimeEdit = new QLineEdit(logMatrixFrame); m_billTimeEdit->setFixedHeight(24);
    m_saudaDateEdit = new QLineEdit(logMatrixFrame); m_saudaDateEdit->setFixedHeight(24);
    m_shippingAddressBtn = new QPushButton("Shipping Address", logMatrixFrame);
    m_shippingAddressBtn->setFixedHeight(24);
    m_shippingAddressEdit = new QLineEdit(logMatrixFrame);
    m_shippingAddressEdit->setVisible(false);
    connect(m_shippingAddressBtn, &QPushButton::clicked, this, [this]() {
        bool ok = false;
        QString text = CustomInputDialog::getText(this, "Shipping Address", "Enter Shipping Address / Origin:",
                                                  m_shippingAddressEdit->text(), &ok);
        if (ok) m_shippingAddressEdit->setText(text);
    });

    m_posCombo = new QComboBox(logMatrixFrame);
    m_posCombo->setFixedHeight(24);
    m_posCombo->setEditable(true);
    QStringList posList = {"Same as Supplier"};
    PartiesModel pModel;
    for (const QString& st : pModel.get_states()) {
        QString c = pModel.get_state_code_for_state(st);
        posList.append(c.isEmpty() ? st : QString("%1 (%2)").arg(st, c));
    }
    m_posCombo->addItems(posList);

    m_poNoEdit = new QLineEdit(logMatrixFrame); m_poNoEdit->setFixedHeight(24);
    m_gradeEdit = new QLineEdit(logMatrixFrame); m_gradeEdit->setFixedHeight(24);
    m_transportEdit = new QLineEdit(logMatrixFrame); m_transportEdit->setFixedHeight(24);

    m_challanNoEdit = new QLineEdit(logMatrixFrame); m_challanNoEdit->setFixedHeight(24);
    m_kandaWeightEdit = new QLineEdit(logMatrixFrame); m_kandaWeightEdit->setFixedHeight(24);
    m_brokerEdit = new QLineEdit(logMatrixFrame); m_brokerEdit->setFixedHeight(24);

    // Matrix Row 1
    QHBoxLayout* lmR1 = new QHBoxLayout();
    lmR1->setSpacing(6);
    lmR1->addLayout(makeLogItem("Veh.No.(F10):", m_vehicleNoEdit, true), 3);
    lmR1->addLayout(makeLogItem("GR No. :", m_grNoEdit), 2);
    lmR1->addLayout(makeLogItem("Driver :", m_driverNameEdit), 2);
    lmR1->addLayout(makeLogItem("E-Way No. :", m_ewayBillNoEdit), 3);
    logVBox->addLayout(lmR1);

    // Matrix Row 2
    QHBoxLayout* lmR2 = new QHBoxLayout();
    lmR2->setSpacing(6);
    lmR2->addLayout(makeLogItem("Bill Time :", m_billTimeEdit), 3);
    lmR2->addLayout(makeLogItem("Sauda Dt. :", m_saudaDateEdit), 2);
    lmR2->addWidget(m_shippingAddressBtn, 2);
    lmR2->addLayout(makeLogItem("POS :", m_posCombo), 3);
    logVBox->addLayout(lmR2);

    // Matrix Row 3
    QHBoxLayout* lmR3 = new QHBoxLayout();
    lmR3->setSpacing(6);
    lmR3->addLayout(makeLogItem("P.O. No. :", m_poNoEdit), 3);
    lmR3->addLayout(makeLogItem("Grade :", m_gradeEdit), 2);
    lmR3->addLayout(makeLogItem("Transport :", m_transportEdit), 5);
    logVBox->addLayout(lmR3);

    // Matrix Row 4
    QHBoxLayout* lmR4 = new QHBoxLayout();
    lmR4->setSpacing(6);
    lmR4->addLayout(makeLogItem("Challan No. :", m_challanNoEdit), 3);
    lmR4->addLayout(makeLogItem("Kanda Weight :", m_kandaWeightEdit, false, true), 2);
    lmR4->addLayout(makeLogItem("Broker Name :", m_brokerEdit), 5);
    logVBox->addLayout(lmR4);

    leftColLayout->addWidget(logMatrixFrame);

    // Narration row directly below Logistics Matrix
    QHBoxLayout* narrRow = new QHBoxLayout();
    narrRow->setSpacing(6);
    QLabel* narrLbl = new QLabel("Narration :", this);
    narrLbl->setStyleSheet("font-size: 11.5px; font-weight: 700; color: #000000; font-style: italic;");
    narrRow->addWidget(narrLbl);

    m_narrationEdit = new QLineEdit(this);
    m_narrationEdit->setFixedHeight(24);
    narrRow->addWidget(m_narrationEdit, 1);
    leftColLayout->addLayout(narrRow);

    bottomSplitLayout->addLayout(leftColLayout, 3);

    // Right Column: Financial Summary Box + G. Total Directly Below It
    QVBoxLayout* rightColLayout = new QVBoxLayout();
    rightColLayout->setSpacing(4);

    m_summaryFrame = new QFrame(this);
    m_summaryFrame->setStyleSheet("QFrame { background-color: #FFFFFF; border: 1px solid #E2E8F0; border-radius: 8px; }");
    QGridLayout* sGrid = new QGridLayout(m_summaryFrame);
    sGrid->setContentsMargins(8, 6, 8, 6);
    sGrid->setHorizontalSpacing(8);
    sGrid->setVerticalSpacing(3);

    // Row 0: Mandi Total (Visible only in Mandi Type)
    m_mandiTotalTitleLabel = new QLabel("Total :", m_summaryFrame);
    m_mandiTotalTitleLabel->setStyleSheet("color: #2563EB; text-decoration: underline; font-weight: 800;");
    m_mandiTotalValLabel = new QLabel("0.00", m_summaryFrame);
    m_mandiTotalValLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    m_mandiTotalValLabel->setStyleSheet("font-size: 11.5px; font-weight: 800; color: #0F172A;");
    sGrid->addWidget(m_mandiTotalTitleLabel, 0, 0);
    sGrid->addWidget(m_mandiTotalValLabel, 0, 1);

    // Row 1: Commission (Mandi) / Other Exp (Standard)
    m_commissionTitleLabel = new QLabel("Commission :", m_summaryFrame);
    m_commissionEdit = new QLineEdit("0.00", m_summaryFrame);
    m_commissionEdit->setFixedHeight(22);
    m_commissionEdit->setAlignment(Qt::AlignRight);
    connect(m_commissionEdit, &QLineEdit::textChanged, this, &PurchaseVoucherWidget::recalculateTotals);
    sGrid->addWidget(m_commissionTitleLabel, 1, 0);
    sGrid->addWidget(m_commissionEdit, 1, 1);

    m_otherExpTitleLabel = new QLabel("Other Exp. :", m_summaryFrame);
    m_otherExpTitleLabel->setStyleSheet("color: #475569; text-decoration: underline;");
    m_otherExpEdit = new QLineEdit("0.00", m_summaryFrame);
    m_otherExpEdit->setFixedHeight(22);
    m_otherExpEdit->setAlignment(Qt::AlignRight);
    connect(m_otherExpEdit, &QLineEdit::textChanged, this, &PurchaseVoucherWidget::recalculateTotals);
    sGrid->addWidget(m_otherExpTitleLabel, 1, 0);
    sGrid->addWidget(m_otherExpEdit, 1, 1);

    // Row 2: Less Amount
    m_lessTitleLabel = new QLabel("(-) Less :", m_summaryFrame);
    m_lessTitleLabel->setStyleSheet("color: #DC2626; text-decoration: underline;");
    m_lessAmountEdit = new QLineEdit("0.00", m_summaryFrame);
    m_lessAmountEdit->setFixedHeight(22);
    m_lessAmountEdit->setAlignment(Qt::AlignRight);
    connect(m_lessAmountEdit, &QLineEdit::textChanged, this, &PurchaseVoucherWidget::recalculateTotals);
    sGrid->addWidget(m_lessTitleLabel, 2, 0);
    sGrid->addWidget(m_lessAmountEdit, 2, 1);

    // Row 3: Taxes
    m_taxTitleLabel = new QLabel("SGST+CGST :", m_summaryFrame);
    m_taxTitleLabel->setStyleSheet("color: #475569;");
    m_taxAmountEdit = new QLineEdit("0.00", m_summaryFrame);
    m_taxAmountEdit->setFixedHeight(22);
    m_taxAmountEdit->setAlignment(Qt::AlignRight);
    connect(m_taxAmountEdit, &QLineEdit::textChanged, this, &PurchaseVoucherWidget::recalculateTotals);
    sGrid->addWidget(m_taxTitleLabel, 3, 0);
    sGrid->addWidget(m_taxAmountEdit, 3, 1);

    // Row 4: Freight
    m_freightTitleLabel = new QLabel("(+) Freight :", m_summaryFrame);
    m_freightTitleLabel->setStyleSheet("color: #475569;");
    m_freightChargesEdit = new QLineEdit("0.00", m_summaryFrame);
    m_freightChargesEdit->setFixedHeight(22);
    m_freightChargesEdit->setAlignment(Qt::AlignRight);
    connect(m_freightChargesEdit, &QLineEdit::textChanged, this, &PurchaseVoucherWidget::recalculateTotals);
    sGrid->addWidget(m_freightTitleLabel, 4, 0);
    sGrid->addWidget(m_freightChargesEdit, 4, 1);

    // Row 5: Round Off
    m_roundOffTitleLabel = new QLabel("Round +/-", m_summaryFrame);
    m_roundOffTitleLabel->setStyleSheet("color: #475569; text-decoration: underline;");
    m_roundOffLabel = new QLabel("0.00", m_summaryFrame);
    m_roundOffLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    m_roundOffLabel->setStyleSheet("font-size: 11.5px; font-weight: 800; color: #0F172A;");
    sGrid->addWidget(m_roundOffTitleLabel, 5, 0);
    sGrid->addWidget(m_roundOffLabel, 5, 1);

    // Row 6: TCS
    m_tcsTitleLabel = new QLabel("TCS @ 0.000% :", m_summaryFrame);
    m_tcsTitleLabel->setStyleSheet("color: #DC2626; font-weight: 800;");
    m_tcsAmountLabel = new QLabel("0.00", m_summaryFrame);
    m_tcsAmountLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    m_tcsAmountLabel->setStyleSheet("font-size: 11.5px; font-weight: 800; color: #0F172A;");
    m_tcsRateEdit = new QLineEdit("0.000", m_summaryFrame);
    m_tcsRateEdit->setVisible(false);
    sGrid->addWidget(m_tcsTitleLabel, 6, 0);
    sGrid->addWidget(m_tcsAmountLabel, 6, 1);

    rightColLayout->addWidget(m_summaryFrame);

    // G. Total Row directly below the Financial Summary Box
    QHBoxLayout* gtRow = new QHBoxLayout();
    gtRow->setSpacing(6);
    QLabel* gtTitle = new QLabel("G. Total :", this);
    gtTitle->setStyleSheet("font-size: 14px; font-weight: 900; color: #0F172A;");
    gtRow->addWidget(gtTitle);

    m_grandTotalLabel = new QLabel("0.00", this);
    m_grandTotalLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    m_grandTotalLabel->setStyleSheet("font-size: 16px; font-weight: 900; color: #0F172A; background-color: #FFFFFF; border: 1.5px solid #2563EB; border-radius: 6px; padding: 2px 6px;");
    gtRow->addWidget(m_grandTotalLabel, 1);
    rightColLayout->addLayout(gtRow);

    bottomSplitLayout->addLayout(rightColLayout, 1);
    rootLayout->addLayout(bottomSplitLayout);

    // ========================================================================
    // 7. FOOTER ACTION BUTTONS BAR
    // ========================================================================
    QHBoxLayout* footerLayout = new QHBoxLayout();
    footerLayout->setSpacing(8);
    footerLayout->addStretch(1);

    m_backBtn = new QPushButton("Back (Esc)", this);
    m_backBtn->setFixedHeight(30);
    m_backBtn->setStyleSheet("QPushButton { background-color: #F1F5F9; border: 1px solid #CBD5E1; border-radius: 6px; padding: 0px 14px; font-weight: 700; color: #475569; font-size: 12px; } QPushButton:hover { background-color: #E2E8F0; }");
    connect(m_backBtn, &QPushButton::clicked, this, &PurchaseVoucherWidget::backRequested);
    footerLayout->addWidget(m_backBtn);

    m_newBtn = new QPushButton("New (F9)", this);
    m_newBtn->setFixedHeight(30);
    m_newBtn->setStyleSheet("QPushButton { background-color: #F1F5F9; border: 1px solid #CBD5E1; border-radius: 6px; padding: 0px 14px; font-weight: 700; color: #0284C7; font-size: 12px; } QPushButton:hover { background-color: #E0F2FE; }");
    connect(m_newBtn, &QPushButton::clicked, this, &PurchaseVoucherWidget::resetForm);
    footerLayout->addWidget(m_newBtn);

    m_deleteBtn = new QPushButton("Delete", this);
    m_deleteBtn->setFixedHeight(30);
    m_deleteBtn->setStyleSheet("QPushButton { background-color: #FEE2E2; color: #DC2626; border: 1px solid #FCA5A5; border-radius: 6px; padding: 0px 14px; font-weight: 800; font-size: 12px; } QPushButton:hover { background-color: #FECACA; }");
    m_deleteBtn->setVisible(false);
    connect(m_deleteBtn, &QPushButton::clicked, this, &PurchaseVoucherWidget::deleteVoucher);
    footerLayout->addWidget(m_deleteBtn);

    m_printBtn = new QPushButton("Print (Ctrl+P)", this);
    m_printBtn->setFixedHeight(30);
    m_printBtn->setStyleSheet("QPushButton { background-color: #F1F5F9; border: 1px solid #CBD5E1; border-radius: 6px; padding: 0px 14px; font-weight: 700; color: #475569; font-size: 12px; } QPushButton:hover { background-color: #E2E8F0; }");
    connect(m_printBtn, &QPushButton::clicked, this, &PurchaseVoucherWidget::printInvoice);
    footerLayout->addWidget(m_printBtn);

    m_pdfBtn = new QPushButton("PDF (Ctrl+E)", this);
    m_pdfBtn->setFixedHeight(30);
    m_pdfBtn->setStyleSheet("QPushButton { background-color: #F1F5F9; border: 1px solid #CBD5E1; border-radius: 6px; padding: 0px 14px; font-weight: 700; color: #475569; font-size: 12px; } QPushButton:hover { background-color: #E2E8F0; }");
    connect(m_pdfBtn, &QPushButton::clicked, this, &PurchaseVoucherWidget::exportPdf);
    footerLayout->addWidget(m_pdfBtn);

    m_saveBtn = new QPushButton("Save Voucher (Ctrl+S)", this);
    m_saveBtn->setFixedHeight(30);
    m_saveBtn->setStyleSheet("QPushButton { background-color: #2563EB; border: 1px solid #1D4ED8; border-radius: 6px; padding: 0px 16px; font-weight: 800; color: #FFFFFF; font-size: 12px; } QPushButton:hover { background-color: #1D4ED8; }");
    connect(m_saveBtn, &QPushButton::clicked, this, &PurchaseVoucherWidget::saveVoucher);
    footerLayout->addWidget(m_saveBtn);

    rootLayout->addLayout(footerLayout);

    m_statusBanner = new QLabel(this);
    m_statusBanner->setVisible(false);
    rootLayout->addWidget(m_statusBanner);
}

void PurchaseVoucherWidget::applyCustomStyles() {
    onMarketTypeChanged(0);
    onTaxStatusChanged();
}

void PurchaseVoucherWidget::onMarketTypeChanged(int index) {
    bool isMandi = (index == 2); // "Mandi Type"
    m_saleStatusGroupBox->setVisible(isMandi);
    m_marketFeeStatusGroupBox->setVisible(isMandi);
    m_mandiExpensesFrame->setVisible(isMandi);

    m_mandiTotalTitleLabel->setVisible(isMandi);
    m_mandiTotalValLabel->setVisible(isMandi);
    m_commissionTitleLabel->setVisible(isMandi);
    m_commissionEdit->setVisible(isMandi);

    m_otherExpTitleLabel->setVisible(!isMandi);
    m_otherExpEdit->setVisible(!isMandi);
    m_lessTitleLabel->setVisible(!isMandi);
    m_lessAmountEdit->setVisible(!isMandi);

    recalculateTotals();
}

void PurchaseVoucherWidget::onTaxStatusChanged() {
    if (m_taxGstRadio->isChecked()) {
        m_selectedTaxStatus = "GST / Exempt";
        m_taxTitleLabel->setText("SGST+CGST :");
    } else if (m_taxIgstRadio->isChecked()) {
        m_selectedTaxStatus = "IGST";
        m_taxTitleLabel->setText("IGST :");
    } else if (m_taxExportRadio->isChecked()) {
        m_selectedTaxStatus = "Export";
        m_taxTitleLabel->setText("Export Tax :");
    }
    recalculateTotals();
}

void PurchaseVoucherWidget::onSaleStatusChanged() {
    if (m_saleSelfRadio->isChecked()) m_selectedSaleStatus = "Self Sale";
    else if (m_saleTransferRadio->isChecked()) m_selectedSaleStatus = "Stock Transfer";
    else if (m_saleLagatRadio->isChecked()) m_selectedSaleStatus = "Lagat Bill";
    else if (m_saleThirdPartyRadio->isChecked()) m_selectedSaleStatus = "Third Party";
}

void PurchaseVoucherWidget::onMarketFeeStatusChanged() {
    if (m_feePayableRadio->isChecked()) m_selectedMarketFeeStatus = "Payable";
    else if (m_feePaidRadio->isChecked()) m_selectedMarketFeeStatus = "Paid";
}

void PurchaseVoucherWidget::onDateChanged(const QDate& date) {
    updateDayOfWeek(date);
    updateFiscalYearBadge();
    if (!isEditMode()) {
        updateNextNumbers();
    }
}

void PurchaseVoucherWidget::updateDayOfWeek(const QDate& date) {
    if (m_dayOfWeekLabel) {
        m_dayOfWeekLabel->setText(date.isValid() ? date.toString("dddd") : QDate::currentDate().toString("dddd"));
    }
}

void PurchaseVoucherWidget::updateFiscalYearBadge() {
}

void PurchaseVoucherWidget::updateNextNumbers() {
    if (isEditMode()) return;
    FiscalYearInfo fy = FiscalYearHelper::getActiveFiscalYear();
    QString dt = m_invoiceDateEdit ? m_invoiceDateEdit->formattedDate() : "";

    QString invNo = m_purchaseModel.get_next_invoice_no(dt);
    QString vNo = m_purchaseModel.get_next_voucher_no(dt);

    if (m_invoiceNoEdit && m_invoiceNoEdit->text().trimmed().isEmpty()) {
        m_invoiceNoEdit->setText(invNo);
    }
    if (m_voucherNoDisplay) {
        m_voucherNoDisplay->setText("No. " + vNo.replace("Pur-", ""));
    }
}

void PurchaseVoucherWidget::onPartySelected(const QVariantMap& partyData) {
    if (m_gstinDisplay) {
        m_gstinDisplay->setText(partyData.value("gstin").toString());
    }
    if (m_partyBalLabel) {
        double bal = partyData.value("current_balance", 0.0).toDouble();
        m_partyBalLabel->setText(QString("Date Bal. %1").arg(QString::number(bal, 'f', 2)));
    }
    QString state = partyData.value("state").toString();
    if (!state.isEmpty() && m_posCombo) {
        for (int i = 0; i < m_posCombo->count(); ++i) {
            if (m_posCombo->itemText(i).contains(state, Qt::CaseInsensitive)) {
                m_posCombo->setCurrentIndex(i);
                break;
            }
        }
    }
}

void PurchaseVoucherWidget::setupNavigationChains() {
    // Left/Right arrow navigation across fields
    VoucherCommon::installGridNavigation(m_invoiceNoEdit, m_marketTypeCombo, m_dueDaysEdit, [this]() {
        QString text = m_invoiceNoEdit ? m_invoiceNoEdit->text().trimmed() : "";
        if (!text.isEmpty()) {
            QVariantMap existing = m_purchaseModel.get_purchase_invoice(text);
            if (!existing.isEmpty() && existing.value("id").toInt() != m_editingInvoiceId) {
                loadInvoiceForEditing(existing.value("id"));
                return;
            }
        }
        if (m_dueDaysEdit) { m_dueDaysEdit->setFocus(); m_dueDaysEdit->selectAll(); }
    });
    VoucherCommon::installGridNavigation(m_dueDaysEdit, m_invoiceNoEdit, m_partySearchWidget, [this]() {
        if (m_partySearchWidget) { m_partySearchWidget->setFocus(); m_partySearchWidget->selectAll(); }
    });
    VoucherCommon::installGridNavigation(m_partySearchWidget, m_dueDaysEdit, m_posCombo, nullptr);
    connect(m_partySearchWidget, &AccountSearchBox::returnPressed, this, [this]() {
        focusTableAt(0, 1);
    });
    VoucherCommon::installGridNavigation(m_posCombo, m_partySearchWidget, m_vehicleNoEdit, [this]() {
        focusTableAt(0, 1);
    });

    // Mandi charges chain
    if (m_damiEdit) connect(m_damiEdit, &QLineEdit::returnPressed, this, [this]() { if (m_labourEdit) { m_labourEdit->setFocus(); m_labourEdit->selectAll(); } });
    if (m_labourEdit) connect(m_labourEdit, &QLineEdit::returnPressed, this, [this]() { if (m_auctionEdit) { m_auctionEdit->setFocus(); m_auctionEdit->selectAll(); } });
    if (m_auctionEdit) connect(m_auctionEdit, &QLineEdit::returnPressed, this, [this]() { if (m_mFeeEdit) { m_mFeeEdit->setFocus(); m_mFeeEdit->selectAll(); } });
    if (m_mFeeEdit) connect(m_mFeeEdit, &QLineEdit::returnPressed, this, [this]() { if (m_hrdfEdit) { m_hrdfEdit->setFocus(); m_hrdfEdit->selectAll(); } });
    if (m_hrdfEdit) connect(m_hrdfEdit, &QLineEdit::returnPressed, this, [this]() { if (m_mandiOtherExpEdit) { m_mandiOtherExpEdit->setFocus(); m_mandiOtherExpEdit->selectAll(); } });
    if (m_mandiOtherExpEdit) connect(m_mandiOtherExpEdit, &QLineEdit::returnPressed, this, [this]() { if (m_welfareEdit) { m_welfareEdit->setFocus(); m_welfareEdit->selectAll(); } });
    if (m_welfareEdit) connect(m_welfareEdit, &QLineEdit::returnPressed, this, [this]() { if (m_dhrmdEdit) { m_dhrmdEdit->setFocus(); m_dhrmdEdit->selectAll(); } });
    if (m_dhrmdEdit) connect(m_dhrmdEdit, &QLineEdit::returnPressed, this, [this]() { if (m_sutliEdit) { m_sutliEdit->setFocus(); m_sutliEdit->selectAll(); } });
    if (m_sutliEdit) connect(m_sutliEdit, &QLineEdit::returnPressed, this, [this]() { if (m_mandiLessEdit) { m_mandiLessEdit->setFocus(); m_mandiLessEdit->selectAll(); } });
    if (m_mandiLessEdit) connect(m_mandiLessEdit, &QLineEdit::returnPressed, this, [this]() { if (m_vehicleNoEdit) { m_vehicleNoEdit->setFocus(); m_vehicleNoEdit->selectAll(); } });

    // Logistics chain (Enter moves forward, Left/Right moves sideways)
    connect(m_vehicleNoEdit, &QLineEdit::returnPressed, this, [this]() { if (m_grNoEdit) { m_grNoEdit->setFocus(); m_grNoEdit->selectAll(); } });
    connect(m_grNoEdit, &QLineEdit::returnPressed, this, [this]() { if (m_driverNameEdit) { m_driverNameEdit->setFocus(); m_driverNameEdit->selectAll(); } });
    connect(m_driverNameEdit, &QLineEdit::returnPressed, this, [this]() { if (m_ewayBillNoEdit) { m_ewayBillNoEdit->setFocus(); m_ewayBillNoEdit->selectAll(); } });
    connect(m_ewayBillNoEdit, &QLineEdit::returnPressed, this, [this]() { if (m_billTimeEdit) { m_billTimeEdit->setFocus(); m_billTimeEdit->selectAll(); } });
    connect(m_billTimeEdit, &QLineEdit::returnPressed, this, [this]() { if (m_saudaDateEdit) { m_saudaDateEdit->setFocus(); m_saudaDateEdit->selectAll(); } });
    connect(m_saudaDateEdit, &QLineEdit::returnPressed, this, [this]() { if (m_poNoEdit) { m_poNoEdit->setFocus(); m_poNoEdit->selectAll(); } });
    connect(m_poNoEdit, &QLineEdit::returnPressed, this, [this]() { if (m_gradeEdit) { m_gradeEdit->setFocus(); m_gradeEdit->selectAll(); } });
    connect(m_gradeEdit, &QLineEdit::returnPressed, this, [this]() { if (m_transportEdit) { m_transportEdit->setFocus(); m_transportEdit->selectAll(); } });
    connect(m_transportEdit, &QLineEdit::returnPressed, this, [this]() { if (m_challanNoEdit) { m_challanNoEdit->setFocus(); m_challanNoEdit->selectAll(); } });
    connect(m_challanNoEdit, &QLineEdit::returnPressed, this, [this]() { if (m_kandaWeightEdit) { m_kandaWeightEdit->setFocus(); m_kandaWeightEdit->selectAll(); } });
    connect(m_kandaWeightEdit, &QLineEdit::returnPressed, this, [this]() { if (m_brokerEdit) { m_brokerEdit->setFocus(); m_brokerEdit->selectAll(); } });
    connect(m_brokerEdit, &QLineEdit::returnPressed, this, [this]() { if (m_narrationEdit) { m_narrationEdit->setFocus(); m_narrationEdit->selectAll(); } });

    // Left/Right navigation for logistics
    VoucherCommon::installGridNavigation(m_vehicleNoEdit, nullptr, m_grNoEdit);
    VoucherCommon::installGridNavigation(m_grNoEdit, m_vehicleNoEdit, m_driverNameEdit);
    VoucherCommon::installGridNavigation(m_driverNameEdit, m_grNoEdit, m_ewayBillNoEdit);
    VoucherCommon::installGridNavigation(m_ewayBillNoEdit, m_driverNameEdit, m_billTimeEdit);

    VoucherCommon::installGridNavigation(m_billTimeEdit, m_ewayBillNoEdit, m_saudaDateEdit);
    VoucherCommon::installGridNavigation(m_saudaDateEdit, m_billTimeEdit, m_poNoEdit);
    VoucherCommon::installGridNavigation(m_poNoEdit, m_saudaDateEdit, m_gradeEdit);
    VoucherCommon::installGridNavigation(m_gradeEdit, m_poNoEdit, m_transportEdit);

    VoucherCommon::installGridNavigation(m_transportEdit, m_gradeEdit, m_challanNoEdit);
    VoucherCommon::installGridNavigation(m_challanNoEdit, m_transportEdit, m_kandaWeightEdit);
    VoucherCommon::installGridNavigation(m_kandaWeightEdit, m_challanNoEdit, m_brokerEdit);
    VoucherCommon::installGridNavigation(m_brokerEdit, m_kandaWeightEdit, m_narrationEdit);
    VoucherCommon::installGridNavigation(m_narrationEdit, m_brokerEdit, nullptr);
}

void PurchaseVoucherWidget::focusTableAt(int row, int col) {
    if (!m_tableWidget || m_tableWidget->rowCount() == 0) return;
    int r = qBound(0, row, m_tableWidget->rowCount() - 1);
    int c = qBound(0, col, m_tableWidget->columnCount() - 1);
    m_tableWidget->setCurrentCell(r, c);
    m_tableWidget->setFocus(Qt::OtherFocusReason);
    m_tableWidget->edit(m_tableWidget->currentIndex());
}

void PurchaseVoucherWidget::advanceCell() {
    if (!m_tableWidget) return;
    int curRow = m_tableWidget->currentRow();
    int curCol = m_tableWidget->currentColumn();

    if (curRow < 0) curRow = 0;
    if (curCol < 0) curCol = 1;

    QTableWidgetItem* nameIt = m_tableWidget->item(curRow, 1);
    bool isItemEmpty = (!nameIt || nameIt->text().isEmpty());

    if (curCol == 1 && isItemEmpty) {
        if (curRow > 0 && curRow == m_tableWidget->rowCount() - 1) {
            m_tableWidget->removeRow(curRow);
            recalculateTotals();
        }
        if (m_mandiExpensesFrame && m_mandiExpensesFrame->isVisible() && m_damiEdit) {
            m_damiEdit->setFocus();
            m_damiEdit->selectAll();
        } else if (m_vehicleNoEdit) {
            m_vehicleNoEdit->setFocus();
            m_vehicleNoEdit->selectAll();
        }
        return;
    }

    int nextCol = curCol + 1;
    while (nextCol < 9 && m_tableWidget->isColumnHidden(nextCol)) {
        nextCol++;
    }

    if (nextCol <= 8) {
        m_tableWidget->setCurrentCell(curRow, nextCol);
        m_tableWidget->edit(m_tableWidget->currentIndex());
    } else {
        if (nameIt && !nameIt->text().isEmpty()) {
            if (curRow == m_tableWidget->rowCount() - 1) {
                addNewLineRow();
            }
            m_tableWidget->setCurrentCell(curRow + 1, 1);
            m_tableWidget->edit(m_tableWidget->currentIndex());
        } else {
            if (curRow > 0 && curRow == m_tableWidget->rowCount() - 1) {
                m_tableWidget->removeRow(curRow);
                recalculateTotals();
            }
            if (m_mandiExpensesFrame && m_mandiExpensesFrame->isVisible() && m_damiEdit) {
                m_damiEdit->setFocus();
                m_damiEdit->selectAll();
            } else if (m_vehicleNoEdit) {
                m_vehicleNoEdit->setFocus();
                m_vehicleNoEdit->selectAll();
            }
        }
    }
}

void PurchaseVoucherWidget::retreatCell() {
    if (!m_tableWidget) return;
    int curRow = m_tableWidget->currentRow();
    int curCol = m_tableWidget->currentColumn();

    int prevCol = curCol - 1;
    while (prevCol >= 1 && m_tableWidget->isColumnHidden(prevCol)) {
        prevCol--;
    }

    if (prevCol >= 1) {
        m_tableWidget->setCurrentCell(curRow, prevCol);
        m_tableWidget->edit(m_tableWidget->currentIndex());
    } else if (curRow > 0) {
        m_tableWidget->setCurrentCell(curRow - 1, 8);
        m_tableWidget->edit(m_tableWidget->currentIndex());
    } else {
        if (m_partySearchWidget) {
            m_partySearchWidget->setFocus();
            m_partySearchWidget->selectAll();
        }
    }
}

void PurchaseVoucherWidget::moveCell(int row, int col) {
    focusTableAt(row, col);
}

void PurchaseVoucherWidget::onStockItemConfigured(int row, const QVariantMap& itemData) {
    if (row < 0 || row >= m_tableWidget->rowCount()) return;
    m_isUpdatingTable = true;
    m_tableWidget->item(row, 1)->setText(itemData.value("name").toString());
    double gst = itemData.value("gst_rate", 0.0).toDouble();
    if (gst == 0.0) gst = itemData.value("tax_rate", 0.0).toDouble();
    if (m_tableWidget->item(row, 6)) m_tableWidget->item(row, 6)->setText(QString::number(gst, 'f', 0));
    double rate = itemData.value("purchase_rate", 0.0).toDouble();
    if (rate <= 0) rate = itemData.value("sale_rate", 0.0).toDouble();
    if (rate <= 0) rate = itemData.value("sales_rate", 0.0).toDouble();
    if (rate > 0 && m_tableWidget->item(row, 7)) m_tableWidget->item(row, 7)->setText(QString::number(rate, 'f', 2));
    m_isUpdatingTable = false;
    focusTableAt(row, 2);
}

void PurchaseVoucherWidget::openDateDialog(bool isInitial) {
    QString curDate = m_invoiceDateEdit ? m_invoiceDateEdit->formattedDate() : "";
    QString newDisp, newIso;
    if (VoucherDateDialog::getVoucherDate(this, curDate, &newDisp, &newIso)) {
        if (m_invoiceDateEdit) {
            m_invoiceDateEdit->setIsoDate(newIso);
        }
        onDateChanged(QDate::fromString(newIso, "yyyy-MM-dd"));
        if (m_partySearchWidget) {
            m_partySearchWidget->setFocus();
            m_partySearchWidget->selectAll();
        }
    } else {
        if (isInitial) {
            emit backRequested();
        } else if (m_partySearchWidget) {
            m_partySearchWidget->setFocus();
        }
    }
}

void PurchaseVoucherWidget::showEvent(QShowEvent* event) {
    QWidget::showEvent(event);
    if (m_partySearchWidget) {
        m_partySearchWidget->setFocus();
    }
}

void PurchaseVoucherWidget::resetForm() {
    m_editingInvoiceId = 0;
    m_editingInvoiceNo.clear();
    m_editingVoucherNo.clear();
    m_hasInitialDateOpened = false;

    m_titleHeaderLabel->setText("F9 : Purchase Voucher");
    m_titleHeaderLabel->setStyleSheet("font-size: 16px; font-weight: 900; color: #FF0000; font-family: 'Segoe UI', -apple-system, BlinkMacSystemFont, Roboto, 'Helvetica Neue', 'Noto Sans', 'Liberation Sans', Arial, sans-serif;");
    m_deleteBtn->setVisible(false);
    m_statusBanner->setVisible(false);

    FiscalYearInfo fy = FiscalYearHelper::getActiveFiscalYear();
    QDate curDate = QDate::currentDate();
    QDate fyStart = QDate::fromString(fy.startDate, "yyyy-MM-dd");
    QDate fyEnd = QDate::fromString(fy.endDate, "yyyy-MM-dd");

    QDate activeDate = curDate;
    if (curDate < fyStart || curDate > fyEnd) {
        activeDate = fyStart;
    }

    if (m_invoiceDateEdit) {
        m_invoiceDateEdit->setDate(activeDate);
        updateDayOfWeek(activeDate);
    }

    if (m_invoiceNoEdit) m_invoiceNoEdit->clear();
    if (m_dueDaysEdit) m_dueDaysEdit->setText("0");

    if (m_partySearchWidget) m_partySearchWidget->clearSelection();
    if (m_gstinDisplay) m_gstinDisplay->clear();
    if (m_partyBalLabel) m_partyBalLabel->setText("Date Bal. 0.00");
    if (m_brokerEdit) m_brokerEdit->clear();

    m_isUpdatingTable = true;
    m_tableWidget->setRowCount(0);
    m_isUpdatingTable = false;

    addNewLineRow();

    // Reset Mandi & Logistics
    if (m_damiEdit) m_damiEdit->setText("0.00");
    if (m_labourEdit) m_labourEdit->clear();
    if (m_auctionEdit) m_auctionEdit->clear();
    if (m_mFeeEdit) m_mFeeEdit->clear();
    if (m_hrdfEdit) m_hrdfEdit->clear();
    if (m_mandiOtherExpEdit) m_mandiOtherExpEdit->clear();
    if (m_welfareEdit) m_welfareEdit->clear();
    if (m_dhrmdEdit) m_dhrmdEdit->clear();
    if (m_sutliEdit) m_sutliEdit->setText("0.00");
    if (m_mandiLessEdit) m_mandiLessEdit->clear();

    if (m_vehicleNoEdit) m_vehicleNoEdit->clear();
    if (m_grNoEdit) m_grNoEdit->clear();
    if (m_driverNameEdit) m_driverNameEdit->clear();
    if (m_ewayBillNoEdit) m_ewayBillNoEdit->clear();
    if (m_billTimeEdit) m_billTimeEdit->clear();
    if (m_saudaDateEdit) m_saudaDateEdit->clear();
    if (m_shippingAddressEdit) m_shippingAddressEdit->clear();
    if (m_poNoEdit) m_poNoEdit->clear();
    if (m_gradeEdit) m_gradeEdit->clear();
    if (m_transportEdit) m_transportEdit->clear();
    if (m_challanNoEdit) m_challanNoEdit->clear();
    if (m_kandaWeightEdit) m_kandaWeightEdit->clear();
    if (m_narrationEdit) m_narrationEdit->clear();

    if (m_commissionEdit) m_commissionEdit->setText("0.00");
    if (m_taxAmountEdit) m_taxAmountEdit->setText("0.00");
    if (m_freightChargesEdit) m_freightChargesEdit->setText("0.00");
    if (m_otherExpEdit) m_otherExpEdit->setText("0.00");
    if (m_lessAmountEdit) m_lessAmountEdit->setText("0.00");
    if (m_tcsRateEdit) m_tcsRateEdit->setText("0.00");

    updateNextNumbers();
    recalculateTotals();
}

void PurchaseVoucherWidget::setWorkingDate(const QString& dateStr) {
    if (m_invoiceDateEdit) {
        m_invoiceDateEdit->setIsoDate(dateStr);
        updateDayOfWeek(m_invoiceDateEdit->date());
    }
    updateNextNumbers();
}

void PurchaseVoucherWidget::addNewLineRow() {
    int row = m_tableWidget->rowCount();
    m_isUpdatingTable = true;
    m_tableWidget->insertRow(row);
    populateRow(row, "", "", 0, 0.500, 0.0, 0.0, 0.0, 0.0);
    m_isUpdatingTable = false;
}

void PurchaseVoucherWidget::removeLineRow(int row) {
    if (row >= 0 && row < m_tableWidget->rowCount()) {
        m_isUpdatingTable = true;
        m_tableWidget->removeRow(row);
        m_isUpdatingTable = false;
        recalculateTotals();
    }
}

void PurchaseVoucherWidget::populateRow(int row, const QString& itemName, const QString& grade, int bags, double packing, double weight, double rate, double amount, double gstPct) {
    if (row >= m_tableWidget->rowCount()) {
        m_tableWidget->insertRow(row);
    }
    m_isUpdatingTable = true;

    QTableWidgetItem* snoItem = new QTableWidgetItem(QString::number(row + 1));
    snoItem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
    snoItem->setTextAlignment(Qt::AlignCenter);
    m_tableWidget->setItem(row, 0, snoItem);

    QTableWidgetItem* nameItem = new QTableWidgetItem(itemName);
    m_tableWidget->setItem(row, 1, nameItem);

    QTableWidgetItem* gradeItem = new QTableWidgetItem(grade);
    m_tableWidget->setItem(row, 2, gradeItem);

    QTableWidgetItem* bagsItem = new QTableWidgetItem(bags > 0 ? QString::number(bags) : "");
    bagsItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
    m_tableWidget->setItem(row, 3, bagsItem);

    QTableWidgetItem* pkgItem = new QTableWidgetItem(packing > 0.0 ? QString::number(packing, 'f', 3) : "0.500");
    pkgItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
    m_tableWidget->setItem(row, 4, pkgItem);

    QTableWidgetItem* wtItem = new QTableWidgetItem(weight > 0.0 ? QString::number(weight, 'f', 3) : "");
    wtItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
    m_tableWidget->setItem(row, 5, wtItem);

    QTableWidgetItem* gstItem = new QTableWidgetItem(gstPct > 0.0 ? QString::number(gstPct, 'f', 0) : "0");
    gstItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
    m_tableWidget->setItem(row, 6, gstItem);

    QTableWidgetItem* rateItem = new QTableWidgetItem(rate > 0.0 ? QString::number(rate, 'f', 2) : "");
    rateItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
    m_tableWidget->setItem(row, 7, rateItem);

    QTableWidgetItem* amtItem = new QTableWidgetItem(amount > 0.0 ? QString::number(amount, 'f', 2) : "");
    amtItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
    m_tableWidget->setItem(row, 8, amtItem);

    m_isUpdatingTable = false;
}

void PurchaseVoucherWidget::onTableCellChanged(int row, int column) {
    if (m_isUpdatingTable) return;
    m_isUpdatingTable = true;

    if (column == 3 || column == 4 || column == 5 || column == 7) {
        int bags = m_tableWidget->item(row, 3) ? m_tableWidget->item(row, 3)->text().toInt() : 0;
        double pkg = m_tableWidget->item(row, 4) ? m_tableWidget->item(row, 4)->text().toDouble() : 0.500;
        double wt = m_tableWidget->item(row, 5) ? m_tableWidget->item(row, 5)->text().toDouble() : 0.0;
        double rate = m_tableWidget->item(row, 7) ? m_tableWidget->item(row, 7)->text().toDouble() : 0.0;

        if (column == 3 && pkg > 0.0001 && wt <= 0.0001) {
            wt = (bags * pkg);
            if (m_tableWidget->item(row, 5)) m_tableWidget->item(row, 5)->setText(QString::number(wt, 'f', 3));
        }

        double amt = wt * rate;
        if (m_tableWidget->item(row, 8)) m_tableWidget->item(row, 8)->setText(QString::number(amt, 'f', 2));
    }

    m_isUpdatingTable = false;
    recalculateTotals();

    if (row == m_tableWidget->rowCount() - 1 && column == 8) {
        if (m_tableWidget->item(row, 1) && !m_tableWidget->item(row, 1)->text().isEmpty()) {
            addNewLineRow();
        }
    }
}

void PurchaseVoucherWidget::recalculateTotals() {
    int totalBags = 0;
    double totalWeight = 0.0;
    double taxableSubtotal = 0.0;
    double totalGst = 0.0;

    for (int r = 0; r < m_tableWidget->rowCount(); ++r) {
        QString name = m_tableWidget->item(r, 1) ? m_tableWidget->item(r, 1)->text() : "";
        if (name.isEmpty()) continue;
        int b = m_tableWidget->item(r, 3) ? m_tableWidget->item(r, 3)->text().toInt() : 0;
        double w = m_tableWidget->item(r, 5) ? m_tableWidget->item(r, 5)->text().toDouble() : 0.0;
        double amt = m_tableWidget->item(r, 8) ? m_tableWidget->item(r, 8)->text().toDouble() : 0.0;
        double gst = m_tableWidget->item(r, 6) ? m_tableWidget->item(r, 6)->text().toDouble() : 0.0;

        totalBags += b;
        totalWeight += w;
        taxableSubtotal += amt;
        if (gst > 0) {
            totalGst += (amt * gst / 100.0);
        }
    }

    m_totalBagsLabel->setText(QString::number(totalBags));
    m_totalWeightLabel->setText(QString::number(totalWeight, 'f', 3));
    m_subtotalTaxableLabel->setText(QString::number(taxableSubtotal, 'f', 2));
    m_mandiTotalValLabel->setText(QString::number(taxableSubtotal, 'f', 2));

    bool isMandi = (m_marketTypeCombo->currentIndex() == 2);
    double otherExp = 0.0;
    double lessAmt = 0.0;

    if (isMandi) {
        double dami = m_damiEdit->text().toDouble();
        double labour = m_labourEdit->text().toDouble();
        double auction = m_auctionEdit->text().toDouble();
        double mfee = m_mFeeEdit->text().toDouble();
        double hrdf = m_hrdfEdit->text().toDouble();
        double mOther = m_mandiOtherExpEdit->text().toDouble();
        double welfare = m_welfareEdit->text().toDouble();
        double dhrmd = m_dhrmdEdit->text().toDouble();
        double sutli = m_sutliEdit->text().toDouble();
        lessAmt = m_mandiLessEdit->text().toDouble();
        otherExp = dami + labour + auction + mfee + hrdf + mOther + welfare + dhrmd + sutli;
    } else {
        otherExp = m_otherExpEdit->text().toDouble();
        lessAmt = m_lessAmountEdit->text().toDouble();
    }

    double freight = m_freightChargesEdit->text().toDouble();
    double comm = isMandi ? m_commissionEdit->text().toDouble() : 0.0;

    if (m_taxAmountEdit->text().toDouble() == 0.0 && totalGst > 0) {
        m_taxAmountEdit->setText(QString::number(totalGst, 'f', 2));
    }
    double tax = m_taxAmountEdit->text().toDouble();

    double rawTotal = taxableSubtotal + otherExp + comm + freight + tax - lessAmt;
    double tcsRate = m_tcsRateEdit->text().toDouble();
    double tcsAmt = (tcsRate > 0) ? (rawTotal * tcsRate / 100.0) : 0.0;
    m_tcsAmountLabel->setText(QString::number(tcsAmt, 'f', 2));

    double finalTotal = rawTotal + tcsAmt;
    double rounded = std::round(finalTotal);
    double roundOff = rounded - finalTotal;

    m_roundOffLabel->setText(QString::number(roundOff, 'f', 2));
    m_grandTotalLabel->setText(QString::number(rounded, 'f', 2));
}

void PurchaseVoucherWidget::saveVoucher() {
    QString party = m_partySearchWidget->currentPartyName();
    if (party.isEmpty()) {
        CustomMessageBox::warning(this, "Validation", "Please select a valid Party / Supplier Account.");
        m_partySearchWidget->setFocus();
        return;
    }

    QVariantList items;
    for (int r = 0; r < m_tableWidget->rowCount(); ++r) {
        QString name = m_tableWidget->item(r, 1) ? m_tableWidget->item(r, 1)->text() : "";
        if (name.isEmpty()) continue;
        QString grade = m_tableWidget->item(r, 2) ? m_tableWidget->item(r, 2)->text() : "";
        QVariantMap itm;
        itm["item_name"] = name;
        itm["grade"] = grade;
        itm["bags"] = m_tableWidget->item(r, 3) ? m_tableWidget->item(r, 3)->text().toInt() : 0;
        itm["packing"] = m_tableWidget->item(r, 4) ? m_tableWidget->item(r, 4)->text().toDouble() : 0.500;
        itm["weight"] = m_tableWidget->item(r, 5) ? m_tableWidget->item(r, 5)->text().toDouble() : 0.0;
        itm["rate"] = m_tableWidget->item(r, 7) ? m_tableWidget->item(r, 7)->text().toDouble() : 0.0;
        itm["amount"] = m_tableWidget->item(r, 8) ? m_tableWidget->item(r, 8)->text().toDouble() : 0.0;
        itm["gst_pct"] = m_tableWidget->item(r, 6) ? m_tableWidget->item(r, 6)->text().toDouble() : 0.0;
        items.append(itm);
    }

    if (items.isEmpty()) {
        CustomMessageBox::warning(this, "Validation", "Please add at least one line item.");
        focusTableAt(0, 1);
        return;
    }

    QString invNo = m_invoiceNoEdit->text().trimmed();
    QString invDate = m_invoiceDateEdit->date().toString("yyyy-MM-dd");
    QString vchNo = isEditMode() ? m_editingVoucherNo : "";

    double taxable = m_subtotalTaxableLabel->text().toDouble();
    double tax = m_taxAmountEdit->text().toDouble();
    double cgst = 0, sgst = 0, igst = 0;
    if (m_selectedTaxStatus == "IGST") igst = tax;
    else { cgst = tax / 2.0; sgst = tax / 2.0; }

    double roundOff = m_roundOffLabel->text().toDouble();
    double grandTotal = m_grandTotalLabel->text().toDouble();
    double freight = m_freightChargesEdit->text().toDouble();
    double tcsAmt = m_tcsAmountLabel->text().toDouble();
    double tcsRate = m_tcsRateEdit->text().toDouble();

    bool ok = false;
    if (isEditMode()) {
        ok = m_purchaseModel.update_purchase_invoice_full(
            m_editingInvoiceId, invNo, invDate, party, m_gstinDisplay->text(),
            items.first().toMap().value("item_name").toString(), "",
            m_totalBagsLabel->text().toInt(), m_totalWeightLabel->text().toDouble(),
            items.first().toMap().value("rate").toDouble(), taxable,
            items.first().toMap().value("gst_pct").toDouble(), cgst, sgst, igst,
            roundOff, grandTotal, "Credit", m_vehicleNoEdit->text(), m_ewayBillNoEdit->text(),
            m_narrationEdit->text(), m_selectedSaleStatus, m_selectedMarketFeeStatus,
            m_damiEdit->text().toDouble(), m_labourEdit->text().toDouble(), m_auctionEdit->text().toDouble(),
            m_mFeeEdit->text().toDouble(), m_hrdfEdit->text().toDouble(), m_otherExpEdit->text().toDouble(),
            m_welfareEdit->text().toDouble(), m_dhrmdEdit->text().toDouble(), m_sutliEdit->text().toDouble(),
            m_lessAmountEdit->text().toDouble(), m_grNoEdit->text(), m_driverNameEdit->text(),
            m_billTimeEdit->text(), m_saudaDateEdit->text(), m_shippingAddressEdit->text(),
            m_poNoEdit->text(), m_gradeEdit->text(), m_kandaWeightEdit->text(),
            m_transportEdit->text(), m_brokerEdit->text(), m_editingVoucherNo, items,
            m_marketTypeCombo->currentText(), m_dueDaysEdit->text().toInt(), m_challanNoEdit->text(),
            freight, tcsAmt, tcsRate, m_selectedTaxStatus, m_posCombo->currentText()
        );
    } else {
        ok = m_purchaseModel.add_purchase_invoice_full(
            invNo, invDate, party, m_gstinDisplay->text(),
            items.first().toMap().value("item_name").toString(), "",
            m_totalBagsLabel->text().toInt(), m_totalWeightLabel->text().toDouble(),
            items.first().toMap().value("rate").toDouble(), taxable,
            items.first().toMap().value("gst_pct").toDouble(), cgst, sgst, igst,
            roundOff, grandTotal, "Credit", m_vehicleNoEdit->text(), m_ewayBillNoEdit->text(),
            m_narrationEdit->text(), m_selectedSaleStatus, m_selectedMarketFeeStatus,
            m_damiEdit->text().toDouble(), m_labourEdit->text().toDouble(), m_auctionEdit->text().toDouble(),
            m_mFeeEdit->text().toDouble(), m_hrdfEdit->text().toDouble(), m_otherExpEdit->text().toDouble(),
            m_welfareEdit->text().toDouble(), m_dhrmdEdit->text().toDouble(), m_sutliEdit->text().toDouble(),
            m_lessAmountEdit->text().toDouble(), m_grNoEdit->text(), m_driverNameEdit->text(),
            m_billTimeEdit->text(), m_saudaDateEdit->text(), m_shippingAddressEdit->text(),
            m_poNoEdit->text(), m_gradeEdit->text(), m_kandaWeightEdit->text(),
            m_transportEdit->text(), m_brokerEdit->text(), vchNo, items,
            m_marketTypeCombo->currentText(), m_dueDaysEdit->text().toInt(), m_challanNoEdit->text(),
            freight, tcsAmt, tcsRate, m_selectedTaxStatus, m_posCombo->currentText()
        );
    }

    if (ok) {
        emit invoiceSaved(invNo);
        CustomMessageBox::information(this, "Success", QString("Purchase Invoice %1 saved successfully.").arg(invNo));
        resetForm();
    } else {
        CustomMessageBox::critical(this, "Error", "Failed to save Purchase Invoice. Database error.");
    }
}

void PurchaseVoucherWidget::deleteVoucher() {
    if (!isEditMode()) return;
    if (CustomMessageBox::question(this, "Confirm Delete", "Are you sure you want to delete this invoice?")) {
        if (m_purchaseModel.delete_purchase_invoice(m_editingInvoiceId)) {
            emit invoiceDeleted(m_editingInvoiceNo);
            resetForm();
        }
    }
}

bool PurchaseVoucherWidget::loadInvoiceForEditing(const QVariant& invNoOrId, const QString& dateHint) {
    resetForm();

    QVariantMap inv = m_purchaseModel.get_purchase_invoice(invNoOrId, dateHint);
    if (inv.isEmpty()) return false;

    m_hasInitialDateOpened = true;
    m_editingInvoiceId = inv.value("id").toInt();
    m_editingInvoiceNo = inv.value("invoice_no").toString();
    m_editingVoucherNo = inv.value("voucher_no").toString();
    m_deleteBtn->setVisible(true);

    m_titleHeaderLabel->setText(QString("ALTERATION : F9 : Purchase Voucher (%1)").arg(m_editingInvoiceNo));
    if (m_voucherNoDisplay) m_voucherNoDisplay->setText("No. " + m_editingVoucherNo.replace("Purchase-", "").replace("Pur-", ""));
    m_invoiceNoEdit->setText(m_editingInvoiceNo);

    QDate dt = QDate::fromString(inv.value("invoice_date").toString(), "yyyy-MM-dd");
    if (dt.isValid()) {
        m_invoiceDateEdit->setDate(dt);
        updateDayOfWeek(dt);
    }

    m_dueDaysEdit->setText(inv.value("due_days", 0).toString());

    QString mt = inv.value("market_type").toString();
    for (int i = 0; i < m_marketTypeCombo->count(); ++i) {
        if (m_marketTypeCombo->itemText(i).compare(mt, Qt::CaseInsensitive) == 0) {
            m_marketTypeCombo->setCurrentIndex(i);
            break;
        }
    }

    QString ts = inv.value("tax_status").toString();
    if (ts.contains("IGST", Qt::CaseInsensitive)) m_taxIgstRadio->setChecked(true);
    else if (ts.contains("Export", Qt::CaseInsensitive)) m_taxExportRadio->setChecked(true);
    else m_taxGstRadio->setChecked(true);
    onTaxStatusChanged();

    QString ss = inv.value("sale_status").toString();
    if (ss.contains("Stock Transfer", Qt::CaseInsensitive)) m_saleTransferRadio->setChecked(true);
    else if (ss.contains("Lagat", Qt::CaseInsensitive)) m_saleLagatRadio->setChecked(true);
    else if (ss.contains("Third Party", Qt::CaseInsensitive)) m_saleThirdPartyRadio->setChecked(true);
    else m_saleSelfRadio->setChecked(true);
    onSaleStatusChanged();

    QString fs = inv.value("market_fee_status").toString();
    if (fs.contains("Payable", Qt::CaseInsensitive)) m_feePayableRadio->setChecked(true);
    else m_feePaidRadio->setChecked(true);
    onMarketFeeStatusChanged();

    QString partyName = inv.value("party_ledger").toString();
    if (partyName.isEmpty()) partyName = inv.value("supplier_name").toString();
    m_partySearchWidget->setPartyName(partyName);
    m_gstinDisplay->setText(inv.value("gstin").toString());

    // Populate line items
    QVariantList items = inv.value("items").toList();
    m_tableWidget->setRowCount(0);
    if (!items.isEmpty()) {
        for (int r = 0; r < items.size(); ++r) {
            QVariantMap itm = items[r].toMap();
            QString grade = itm.value("grade").toString();
            if (grade.isEmpty()) grade = inv.value("grade").toString();
            populateRow(
                r, itm.value("item_name").toString(),
                grade,
                itm.value("bag_count", itm.value("bags")).toInt(),
                itm.value("packing", 0.500).toDouble(),
                itm.value("weight_qtl", itm.value("weight")).toDouble(),
                itm.value("rate_per_qtl", itm.value("rate")).toDouble(),
                itm.value("total_amount", itm.value("amount")).toDouble(),
                itm.value("gst_pct", itm.value("tax")).toDouble()
            );
        }
    } else {
        populateRow(
            0, inv.value("item_name").toString(),
            inv.value("grade").toString(),
            inv.value("bag_count").toInt(), 0.500,
            inv.value("weight_qtl").toDouble(),
            inv.value("rate_per_qtl").toDouble(),
            inv.value("taxable_amount").toDouble(),
            inv.value("gst_pct").toDouble()
        );
    }
    if (m_tableWidget->rowCount() == 0) {
        addNewLineRow();
    }

    // Populate logistics
    m_vehicleNoEdit->setText(inv.value("vehicle_no").toString());
    m_grNoEdit->setText(inv.value("gr_no").toString());
    m_driverNameEdit->setText(inv.value("driver").toString());
    m_ewayBillNoEdit->setText(inv.value("eway_bill_no").toString());
    m_billTimeEdit->setText(inv.value("bill_time").toString());
    m_saudaDateEdit->setText(inv.value("sauda_date").toString());
    m_shippingAddressEdit->setText(inv.value("shipping_address").toString());
    m_poNoEdit->setText(inv.value("po_no").toString());
    m_gradeEdit->setText(inv.value("grade").toString());
    m_transportEdit->setText(inv.value("transport").toString());
    m_challanNoEdit->setText(inv.value("challan_no").toString());
    m_kandaWeightEdit->setText(inv.value("kanda_weight").toString());
    m_brokerEdit->setText(inv.value("broker_name").toString());

    // Populate Mandi expenses
    m_damiEdit->setText(inv.value("dami").toString());
    m_labourEdit->setText(inv.value("labour").toString());
    m_auctionEdit->setText(inv.value("auction").toString());
    m_mFeeEdit->setText(inv.value("m_fee").toString());
    m_hrdfEdit->setText(inv.value("hrdf").toString());
    m_mandiOtherExpEdit->setText(inv.value("other_exp").toString());
    m_welfareEdit->setText(inv.value("welfare").toString());
    m_dhrmdEdit->setText(inv.value("dhrmd").toString());
    m_sutliEdit->setText(inv.value("sutli").toString());
    m_mandiLessEdit->setText(inv.value("less_amount").toString());

    // Populate summary
    m_otherExpEdit->setText(inv.value("other_exp").toString());
    m_lessAmountEdit->setText(inv.value("less_amount").toString());
    m_freightChargesEdit->setText(inv.value("freight_charges").toString());
    m_tcsRateEdit->setText(inv.value("tcs_rate").toString());
    m_narrationEdit->setText(inv.value("narration").toString());

    recalculateTotals();
    return true;
}

void PurchaseVoucherWidget::printInvoice() {
    if (m_printExportCtrl) {
        m_printExportCtrl->print_purchase_invoice(m_invoiceNoEdit->text());
    }
}

void PurchaseVoucherWidget::exportPdf() {
    if (m_printExportCtrl) {
        m_printExportCtrl->export_purchase_invoice_pdf(m_invoiceNoEdit->text());
    }
}

void PurchaseVoucherWidget::loadPreviousVoucher() {
    QVariantMap prev = m_purchaseModel.get_previous_purchase_invoice(m_editingInvoiceId, m_invoiceNoEdit ? m_invoiceNoEdit->text().trimmed() : "");
    if (!prev.isEmpty()) {
        loadInvoiceForEditing(prev.value("id"));
    }
}

void PurchaseVoucherWidget::loadNextVoucher() {
    if (m_editingInvoiceId <= 0) return;
    QVariantMap next = m_purchaseModel.get_next_purchase_invoice(m_editingInvoiceId, m_invoiceNoEdit ? m_invoiceNoEdit->text().trimmed() : "");
    if (!next.isEmpty()) {
        loadInvoiceForEditing(next.value("id"));
    } else {
        resetForm();
    }
}

void PurchaseVoucherWidget::openAlterVoucherDialog() {
    bool ok = false;
    QString text = CustomInputDialog::getText(this, "Find & Alter Purchase Voucher", "Enter Invoice No or Voucher No to alter:", m_editingInvoiceNo, &ok);
    if (ok && !text.trimmed().isEmpty()) {
        QVariantMap inv = m_purchaseModel.get_purchase_invoice(text.trimmed());
        if (!inv.isEmpty()) {
            loadInvoiceForEditing(inv.value("id"));
        } else {
            CustomMessageBox::warning(this, "Voucher Not Found", QString("No purchase voucher found matching \"%1\".").arg(text.trimmed()));
        }
    }
}

void PurchaseVoucherWidget::keyPressEvent(QKeyEvent* event) {
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
    } else if (event->key() == Qt::Key_F9) {
        resetForm();
        event->accept();
        return;
    } else if (event->key() == Qt::Key_Escape) {
        emit backRequested();
        event->accept();
        return;
    } else if (event->modifiers() & Qt::ControlModifier && event->key() == Qt::Key_S) {
        saveVoucher();
        event->accept();
        return;
    } else if (event->modifiers() & Qt::AltModifier && event->key() == Qt::Key_T) {
        int next = (m_taxButtonGroup->checkedId() + 1) % 3;
        m_taxButtonGroup->button(next)->setChecked(true);
        onTaxStatusChanged();
        event->accept();
        return;
    } else if (event->modifiers() & Qt::AltModifier && event->key() == Qt::Key_S) {
        if (m_marketTypeCombo->currentIndex() == 2) {
            int next = (m_saleButtonGroup->checkedId() + 1) % 4;
            m_saleButtonGroup->button(next)->setChecked(true);
            onSaleStatusChanged();
        } else {
            m_partySearchWidget->setFocus();
        }
        event->accept();
        return;
    } else if (event->modifiers() & Qt::AltModifier && event->key() == Qt::Key_F) {
        if (m_marketTypeCombo->currentIndex() == 2) {
            int next = (m_feeButtonGroup->checkedId() + 1) % 2;
            m_feeButtonGroup->button(next)->setChecked(true);
            onMarketFeeStatusChanged();
        }
        event->accept();
        return;
    }
    QWidget::keyPressEvent(event);
}

bool PurchaseVoucherWidget::eventFilter(QObject* watched, QEvent* event) {
    return QWidget::eventFilter(watched, event);
}

void PurchaseVoucherWidget::syncModelFromTable() {}
void PurchaseVoucherWidget::syncTableFromModel() {}
void PurchaseVoucherWidget::setStatusMessage(const QString& message, bool isError) {}
