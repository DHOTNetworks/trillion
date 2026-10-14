#include "sales_voucher_widget.h"
#include "voucher_date_dialog.h"
#include "custom_dialogs.h"
#include "print_copy_options_dialog.h"
#include "voucher_common.h"
#include "erp_theme.h"
#include "kbd_badge_button.h"
#include <QBrush>
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
#include <QShortcut>
#include <QComboBox>
#include <QApplication>
#include <QCompleter>
#include <QDate>
#include <QTimer>
#include <cmath>
#include <QDebug>
#include "../database_manager.h"

// ============================================================================
// SalesVoucherWidget Implementation (Authentic Bahi-Khata Replication)
// ============================================================================

SalesVoucherWidget::SalesVoucherWidget(PrintExportController* printExportCtrl, QWidget* parent)
    : QWidget(parent)
    , m_printExportCtrl(printExportCtrl)
{
    setupUi();
    applyCustomStyles();
    setupNavigationChains();
    setupCompleters();
    resetForm();
}

SalesVoucherWidget::~SalesVoucherWidget() {
}

void SalesVoucherWidget::setupUi() {
    // Standard slate scaffold (ErpTheme): light palette pin + page margins.
    // Voucher grids stay editable — only the register/report T3 look is shared
    // (navy headers, explicit foregrounds), never styleTable's NoEditTriggers.
    ErpTheme::applySlate(this);
    setStyleSheet(
        "SalesVoucherWidget { background-color: #F8FAFC; font-family: 'Segoe UI', -apple-system, BlinkMacSystemFont, Roboto, 'Helvetica Neue', 'Noto Sans', 'Liberation Sans', Arial, sans-serif; }"
        "QLabel { border: none; background: transparent; color: #1E293B; font-size: 11px; font-weight: 700; }"
        "QLineEdit, QComboBox { background-color: #FFFFFF; color: #0F172A; border: 1.5px solid #CBD5E1; border-radius: 6px; padding: 4px 8px; font-size: 12px; font-weight: 700; }"
        "QLineEdit:focus, QComboBox:focus { border: 2px solid #2563EB; background-color: #FFFFF0; }"
        "QRadioButton { font-size: 11px; font-weight: 700; color: #1E293B; spacing: 5px; background: transparent; }"
        "QRadioButton:hover { color: #0F172A; }"
        "QRadioButton::indicator { width: 14px; height: 14px; border-radius: 7px; border: 1.5px solid #94A3B8; background-color: #FFFFFF; }"
        "QRadioButton::indicator:hover { border-color: #2563EB; background-color: #EFF6FF; }"
        "QRadioButton::indicator:checked { width: 14px; height: 14px; border-radius: 7px; border: 4.5px solid #2563EB; background-color: #FFFFFF; }"
        "QRadioButton::indicator:checked:hover { border-color: #1D4ED8; }"
        "QCheckBox { font-size: 11px; font-weight: 700; color: #334155; spacing: 4px; background: transparent; }"
        "QCheckBox::indicator { width: 13px; height: 13px; background: #FFFFFF; border: 1px solid #CBD5E1; border-radius: 3px; }"
        "QGroupBox { font-size: 10.5px; font-weight: 800; color: #1E293B; border: 1px solid #E2E8F0; border-radius: 8px; margin-top: 12px; padding: 6px 8px 4px 8px; background-color: #FFFFFF; }"
        "QGroupBox::title { subcontrol-origin: margin; subcontrol-position: top left; padding: 0 4px; left: 8px; top: 0px; color: #2563EB; font-weight: 800; background-color: #FFFFFF; }"
        "QPushButton { background-color: #F1F5F9; border: 1px solid #CBD5E1; border-radius: 6px; font-size: 11px; font-weight: 700; color: #475569; padding: 4px 10px; }"
        "QPushButton:hover { background-color: #E2E8F0; }"
        "QPushButton:focus { border: 2px solid #2563EB; background-color: #FFFFF0; }"
    );

    QVBoxLayout* rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(12, 10, 12, 10);
    rootLayout->setSpacing(8);

    // ========================================================================
    // 1. TOP HEADER BAR: Modern Badge + No. + Title + Date & Day
    // ========================================================================
    QHBoxLayout* topHeaderLayout = new QHBoxLayout();
    topHeaderLayout->setContentsMargins(0, 0, 0, 0);
    topHeaderLayout->setSpacing(6);

    m_saleBadge = new QLabel("Sale", this);
    m_saleBadge->setAlignment(Qt::AlignCenter);
    m_saleBadge->setMinimumHeight(22);
    m_saleBadge->setStyleSheet(ErpTheme::badgeStyle("info"));
    topHeaderLayout->addWidget(m_saleBadge);

    m_voucherNoDisplay = new QLabel("No. 1", this);
    m_voucherNoDisplay->setStyleSheet(ErpTheme::headerTitleStyle(13));
    topHeaderLayout->addWidget(m_voucherNoDisplay);

    topHeaderLayout->addStretch(1);

    m_titleHeaderLabel = new QLabel("F8 : Sale Voucher", this);
    m_titleHeaderLabel->setAlignment(Qt::AlignCenter);
    m_titleHeaderLabel->setStyleSheet(ErpTheme::headerTitleStyle(15));
    topHeaderLayout->addWidget(m_titleHeaderLabel);

    topHeaderLayout->addStretch(1);

    // Date & Day of Week
    QVBoxLayout* dateBox = new QVBoxLayout();
    dateBox->setSpacing(0);
    dateBox->setAlignment(Qt::AlignRight);

    m_invoiceDateEdit = new AccountingDateEdit(this);
    m_invoiceDateEdit->setMinimumHeight(22);
    m_invoiceDateEdit->setMinimumWidth(105);
    connect(m_invoiceDateEdit, &AccountingDateEdit::dateChanged, this, &SalesVoucherWidget::onDateChanged);
    dateBox->addWidget(m_invoiceDateEdit);

    m_dayOfWeekLabel = new QLabel(this);
    m_dayOfWeekLabel->setAlignment(Qt::AlignRight);
    m_dayOfWeekLabel->setStyleSheet(QString("font-size: 10.5px; font-weight: 700; color: %1; border: none; background: transparent;").arg(ErpTheme::kSky.name()));
    dateBox->addWidget(m_dayOfWeekLabel);
    topHeaderLayout->addLayout(dateBox);

    rootLayout->addLayout(topHeaderLayout);

    // Sub-Header Shortcut / Help Line + Rev.Charge
    QHBoxLayout* subBarLayout = new QHBoxLayout();
    subBarLayout->setContentsMargins(0, 0, 0, 0);
    subBarLayout->setSpacing(16);

    m_revChargeCheck = new QCheckBox("Rev.Charge By Recipient", this);
    m_revChargeCheck->setStyleSheet(QString("font-size: 11px; font-weight: 700; color: %1; spacing: 6px; background: transparent; padding-right: 8px;").arg(ErpTheme::kLabelSlate.name()));
    subBarLayout->addWidget(m_revChargeCheck);

    m_helpHintLabel = new QLabel("(Change Date - F8 : Change Sale Mode - Shift+F8: Sale Settings - Alt+L:Show Ledger - Ctrl+F1: Set Image)", this);
    m_helpHintLabel->setStyleSheet(ErpTheme::mutedLabelStyle(10));
    subBarLayout->addWidget(m_helpHintLabel);

    subBarLayout->addStretch(1);
    rootLayout->addLayout(subBarLayout);

    // ========================================================================
    // 2. META ROW 1: Market Type, Inv.No, Due Days, Sale Status, Tax Status, Market Fee Status
    // ========================================================================
    QHBoxLayout* row1 = new QHBoxLayout();
    row1->setContentsMargins(0, 0, 0, 0);
    row1->setSpacing(6);

    // Market Type Dropdown
    m_marketTypeCombo = new QComboBox(this);
    m_marketTypeCombo->addItems({"Market Type (With Stock)", "Market Type (Without Stock)", "Mandi Type"});
    m_marketTypeCombo->setMinimumHeight(24);
    m_marketTypeCombo->setMinimumWidth(190);
    connect(m_marketTypeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &SalesVoucherWidget::onMarketTypeChanged);
    row1->addWidget(m_marketTypeCombo);

    // Invoice No
    QLabel* invLbl = new QLabel("Inv.No.:", this);
    row1->addWidget(invLbl);
    m_invoiceNoEdit = new QLineEdit(this);
    m_invoiceNoEdit->setMinimumHeight(24);
    m_invoiceNoEdit->setMinimumWidth(110);
    row1->addWidget(m_invoiceNoEdit);

    // Due Days
    QVBoxLayout* dueBox = new QVBoxLayout();
    dueBox->setSpacing(0);
    QLabel* dueHint = new QLabel("(Ctrl+D)", this);
    dueHint->setStyleSheet(ErpTheme::mutedLabelStyle(9));
    QLabel* dueLbl = new QLabel("Due Days :", this);
    dueBox->addWidget(dueHint);
    dueBox->addWidget(dueLbl);
    row1->addLayout(dueBox);

    m_dueDaysEdit = new QLineEdit("0", this);
    m_dueDaysEdit->setMinimumHeight(24);
    m_dueDaysEdit->setMinimumWidth(42);
    m_dueDaysEdit->setAlignment(Qt::AlignCenter);
    row1->addWidget(m_dueDaysEdit);

    // Sale Status (Mandi Only)
    m_saleStatusGroupBox = new QGroupBox("Sale Status (Alt+S)", this);
    QHBoxLayout* saleStatLayout = new QHBoxLayout(m_saleStatusGroupBox);
    saleStatLayout->setContentsMargins(6, 4, 6, 4);
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
    connect(m_saleButtonGroup, &QButtonGroup::idClicked, this, &SalesVoucherWidget::onSaleStatusChanged);
    row1->addWidget(m_saleStatusGroupBox);

    // Market Fee Status (Mandi Only)
    m_marketFeeStatusGroupBox = new QGroupBox("Market Fee Status (Alt+F)", this);
    QHBoxLayout* feeStatLayout = new QHBoxLayout(m_marketFeeStatusGroupBox);
    feeStatLayout->setContentsMargins(6, 4, 6, 4);
    feeStatLayout->setSpacing(8);

    m_feeButtonGroup = new QButtonGroup(this);
    m_feePayableRadio = new QRadioButton("Payable", m_marketFeeStatusGroupBox);
    m_feePaidRadio = new QRadioButton("Paid", m_marketFeeStatusGroupBox);
    m_feePaidRadio->setChecked(true);
    m_feeButtonGroup->addButton(m_feePayableRadio, 0);
    m_feeButtonGroup->addButton(m_feePaidRadio, 1);

    feeStatLayout->addWidget(m_feePayableRadio);
    feeStatLayout->addWidget(m_feePaidRadio);
    connect(m_feeButtonGroup, &QButtonGroup::idClicked, this, &SalesVoucherWidget::onMarketFeeStatusChanged);
    row1->addWidget(m_marketFeeStatusGroupBox);

    // Push Tax Status to the right side corner
    row1->addStretch(1);

    // Tax Status (Always visible, aligned to right side corner)
    m_taxStatusGroupBox = new QGroupBox("Tax Status : (Alt+R / Alt+T)", this);
    QHBoxLayout* taxStatLayout = new QHBoxLayout(m_taxStatusGroupBox);
    taxStatLayout->setContentsMargins(8, 4, 8, 4);
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
    connect(m_taxButtonGroup, &QButtonGroup::idClicked, this, &SalesVoucherWidget::onTaxStatusChanged);
    row1->addWidget(m_taxStatusGroupBox);

    rootLayout->addLayout(row1);

    // ========================================================================
    // 3. META ROW 2: Party Search + Date Bal + GSTIN
    // ========================================================================
    QHBoxLayout* row2 = new QHBoxLayout();
    row2->setSpacing(6);

    m_partyTagLabel = new QLabel("Sale To Party :", this);
    m_partyTagLabel->setStyleSheet(QString("color: %1; font-size: 12px; font-weight: 800; border: none; background: transparent;").arg(ErpTheme::kNavy.name()));
    row2->addWidget(m_partyTagLabel);

    m_partySearchWidget = new AccountSearchBox(this);
    m_partySearchWidget->setMinimumHeight(26);
    connect(m_partySearchWidget, &AccountSearchBox::partyDataSelected, this, &SalesVoucherWidget::onPartySelected);
    row2->addWidget(m_partySearchWidget, 1);

    rootLayout->addLayout(row2);

    // Sub-row under party: Date Bal, GSTIN & Including Tax
    QHBoxLayout* partySubRow = new QHBoxLayout();
    partySubRow->setSpacing(20);
    partySubRow->setContentsMargins(10, 0, 0, 0);

    m_partyBalLabel = new QLabel("Date Bal. 0.00", this);
    m_partyBalLabel->setStyleSheet(ErpTheme::mutedLabelStyle(11));
    partySubRow->addWidget(m_partyBalLabel);

    QHBoxLayout* gstinLayout = new QHBoxLayout();
    gstinLayout->setSpacing(4);
    QLabel* gstinTag = new QLabel("GSTIN :", this);
    gstinLayout->addWidget(gstinTag);
    m_gstinDisplay = new QLineEdit(this);
    m_gstinDisplay->setReadOnly(true);
    m_gstinDisplay->setMinimumHeight(20);
    m_gstinDisplay->setMinimumWidth(160);
    m_gstinDisplay->setStyleSheet(QString("QLineEdit { background: transparent; border: none; font-weight: 800; color: %1; }").arg(ErpTheme::kNavy.name()));
    gstinLayout->addWidget(m_gstinDisplay);
    partySubRow->addLayout(gstinLayout);

    partySubRow->addStretch(1);

    // Including Tax Checkbox (Above Table)
    m_inclTaxCheck = new QCheckBox("Including Tax (Alt+I)", this);
    m_inclTaxCheck->setStyleSheet(QString("color: %1; font-weight: 800; background: transparent;").arg(ErpTheme::kBlue.name()));
    connect(m_inclTaxCheck, &QCheckBox::toggled, this, &SalesVoucherWidget::recalculateTotals);
    partySubRow->addWidget(m_inclTaxCheck);

    rootLayout->addLayout(partySubRow);

    // ========================================================================
    // 4. LINE ITEMS TABLE & TOTAL SUMMARY ROW
    // ========================================================================
    m_tableWidget = new QTableWidget(this);
    m_tableWidget->setColumnCount(10);
    m_tableWidget->setHorizontalHeaderLabels({
        "No.", "Item Name", "Grade", "Bags", "Pkng.", "Weight", "Rate", "Amount", "GST", "Act"
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
    m_tableWidget->setColumnWidth(6, 95);  // Rate
    m_tableWidget->setColumnWidth(7, 115); // Amount
    m_tableWidget->setColumnWidth(8, 50);  // GST
    m_tableWidget->setColumnWidth(9, 35);  // Act

    m_tableWidget->verticalHeader()->setVisible(false);
    m_tableWidget->setSelectionBehavior(QAbstractItemView::SelectItems);
    m_tableWidget->setSelectionMode(QAbstractItemView::SingleSelection);
    m_tableWidget->setShowGrid(true);
    m_tableWidget->setMinimumHeight(140);
    m_tableWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    // Voucher grid: editable entry surface with the Trial Balance look —
    // navy headers, alternating rows, spec selection. Edit triggers and
    // delegates are untouched; every cell gets an explicit foreground in
    // populateRow (dark-mode hardening).
    m_tableWidget->setStyleSheet(
        "QTableWidget {"
        "  background-color: #FFFFFF;"
        "  alternate-background-color: #F8FAFC;"
        "  gridline-color: #E2E8F0;"
        "  font-family: 'Segoe UI', -apple-system, BlinkMacSystemFont, Roboto, 'Helvetica Neue', 'Noto Sans', 'Liberation Sans', Arial, sans-serif;"
        "  font-size: 12px;"
        "  color: #0F172A;"
        "  border: 1px solid #CBD5E1;"
        "  border-radius: 6px;"
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
        "QTableWidget::item:selected {"
        "  background-color: #2563EB;"
        "  color: #FFFFFF;"
        "  font-weight: 700;"
        "}"
    );

    m_itemDelegate = new ItemSearchDelegate(this);
    m_tableWidget->setItemDelegate(m_itemDelegate);

    connect(m_itemDelegate, &ItemSearchDelegate::stockItemConfigured, this, &SalesVoucherWidget::onStockItemConfigured);
    connect(m_itemDelegate, &ItemSearchDelegate::moveNextRequested, this, &SalesVoucherWidget::advanceCell);
    connect(m_itemDelegate, &ItemSearchDelegate::movePrevRequested, this, &SalesVoucherWidget::retreatCell);
    connect(m_itemDelegate, &ItemSearchDelegate::browsePrevRequested, this, &SalesVoucherWidget::loadPreviousVoucher);
    connect(m_itemDelegate, &ItemSearchDelegate::browseNextRequested, this, &SalesVoucherWidget::loadNextVoucher);

    connect(m_tableWidget, &QTableWidget::cellChanged, this, &SalesVoucherWidget::onTableCellChanged);
    rootLayout->addWidget(m_tableWidget, 1);

    // Table Total Summary Row
    // Table Total Summary Row (white card, navy figures — was peach-tinted)
    QFrame* totalSummaryBar = new QFrame(this);
    totalSummaryBar->setMinimumHeight(26);
    totalSummaryBar->setStyleSheet(QString("background: #FFFFFF; border: 1px solid %1; border-radius: 6px;").arg(ErpTheme::kCardBorder.name()));
    QHBoxLayout* sumLayout = new QHBoxLayout(totalSummaryBar);
    sumLayout->setContentsMargins(6, 1, 6, 1);
    sumLayout->setSpacing(0);

    sumLayout->addStretch(1);
    QLabel* totTitle = new QLabel("Total :", totalSummaryBar);
    totTitle->setStyleSheet(QString("font-size: 12px; font-weight: 900; color: %1; border: none; background: transparent;").arg(ErpTheme::kNavy.name()));
    sumLayout->addWidget(totTitle);
    sumLayout->addSpacing(8);

    m_totalBagsLabel = new QLabel("0", totalSummaryBar);
    m_totalBagsLabel->setMinimumWidth(65);
    m_totalBagsLabel->setAlignment(Qt::AlignCenter);
    m_totalBagsLabel->setStyleSheet(QString("font-size: 12px; font-weight: 900; color: %1; border: none; background: transparent;").arg(ErpTheme::kNavy.name()));
    sumLayout->addWidget(m_totalBagsLabel);

    sumLayout->addSpacing(65); // Pkng spacing

    m_totalWeightLabel = new QLabel("0.000", totalSummaryBar);
    m_totalWeightLabel->setMinimumWidth(85);
    m_totalWeightLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    m_totalWeightLabel->setStyleSheet(QString("font-size: 12px; font-weight: 900; color: %1; border: none; background: transparent;").arg(ErpTheme::kNavy.name()));
    sumLayout->addWidget(m_totalWeightLabel);

    sumLayout->addSpacing(95); // Rate spacing

    m_subtotalTaxableLabel = new QLabel("0.00", totalSummaryBar);
    m_subtotalTaxableLabel->setMinimumWidth(115);
    m_subtotalTaxableLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    m_subtotalTaxableLabel->setStyleSheet(QString("font-size: 12px; font-weight: 900; color: %1; border: none; background: transparent;").arg(ErpTheme::kNavy.name()));
    sumLayout->addWidget(m_subtotalTaxableLabel);

    sumLayout->addSpacing(85); // GST (50) & Action (35) spacing
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
        l->setStyleSheet(QString("font-size: 11px; font-weight: 700; color: %1; border: none; background: transparent;").arg(ErpTheme::kNavy.name()));
        box->addWidget(l);
        member = new QLineEdit(defVal, m_mandiExpensesFrame);
        member->setMinimumWidth(70);
        member->setMinimumHeight(22);
        member->setAlignment(Qt::AlignRight);
        connect(member, &QLineEdit::textChanged, this, &SalesVoucherWidget::recalculateTotals);
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
    logMatrixFrame->setStyleSheet(QString("QFrame { background-color: #FFFFFF; border: 1px solid %1; border-radius: 8px; }").arg(ErpTheme::kLine.name()));
    QVBoxLayout* logVBox = new QVBoxLayout(logMatrixFrame);
    logVBox->setContentsMargins(6, 6, 6, 6);
    logVBox->setSpacing(4);

    auto makeLogItem = [logMatrixFrame](const QString& lbl, QWidget* w, bool isVeh = false, bool isKanda = false) -> QHBoxLayout* {
        QHBoxLayout* row = new QHBoxLayout();
        row->setSpacing(3);
        QLabel* l = new QLabel(lbl, logMatrixFrame);
        if (isKanda) l->setStyleSheet(QString("color: %1; text-decoration: underline; font-weight: 800; font-size: 11px; border: none; background: transparent;").arg(ErpTheme::kBlueDark.name()));
        else l->setStyleSheet(QString("color: %1; font-weight: 700; font-size: 11px; border: none; background: transparent;").arg(ErpTheme::kLabelSlate.name()));
        row->addWidget(l);
        if (isVeh) w->setStyleSheet(QString("QLineEdit { background: %1; color: %2; border: 1px solid %3; border-radius: 6px; font-weight: 800; }").arg(ErpTheme::kGreenBg.name(), ErpTheme::kNavy.name(), ErpTheme::kLine.name()));
        row->addWidget(w, 1);
        return row;
    };

    m_vehicleNoEdit = new QLineEdit(logMatrixFrame); m_vehicleNoEdit->setMinimumHeight(24);
    VoucherCommon::setupUppercaseInput(m_vehicleNoEdit);
    m_grNoEdit = new QLineEdit(logMatrixFrame); m_grNoEdit->setMinimumHeight(24);
    m_driverNameEdit = new QLineEdit(logMatrixFrame); m_driverNameEdit->setMinimumHeight(24);
    m_ewayBillNoEdit = new QLineEdit(logMatrixFrame); m_ewayBillNoEdit->setMinimumHeight(24);

    m_billTimeEdit = new QLineEdit(logMatrixFrame); m_billTimeEdit->setMinimumHeight(24);
    m_saudaDateEdit = new QLineEdit(logMatrixFrame); m_saudaDateEdit->setMinimumHeight(24);
    m_shippingAddressBtn = new QPushButton("Shipping Address", logMatrixFrame);
    m_shippingAddressBtn->setMinimumHeight(24);
    m_shippingAddressEdit = new QLineEdit(logMatrixFrame);
    m_shippingAddressEdit->setVisible(false);
    connect(m_shippingAddressBtn, &QPushButton::clicked, this, [this]() {
        bool ok = false;
        QString text = CustomInputDialog::getText(this, "Shipping Address", "Enter Shipping Address / Destination:",
                                                  m_shippingAddressEdit->text(), &ok);
        if (ok) m_shippingAddressEdit->setText(text);
    });

    m_posCombo = new QComboBox(logMatrixFrame);
    m_posCombo->setMinimumHeight(24);
    m_posCombo->setEditable(true);
    QStringList posList = {"Same as Buyer"};
    PartiesModel pModel;
    for (const QString& st : pModel.get_states()) {
        QString c = pModel.get_state_code_for_state(st);
        posList.append(c.isEmpty() ? st : QString("%1 (%2)").arg(st, c));
    }
    m_posCombo->addItems(posList);

    m_poNoEdit = new QLineEdit(logMatrixFrame); m_poNoEdit->setMinimumHeight(24);
    m_gradeEdit = new QLineEdit(logMatrixFrame); m_gradeEdit->setMinimumHeight(24);
    m_transportEdit = new AccountSearchBox(logMatrixFrame); m_transportEdit->setMinimumHeight(24);
    m_transportEdit->configureForTransportSearch();

    m_challanNoEdit = new QLineEdit(logMatrixFrame); m_challanNoEdit->setMinimumHeight(24);
    m_kandaWeightEdit = new QLineEdit(logMatrixFrame); m_kandaWeightEdit->setMinimumHeight(24);
    m_brokerEdit = new AccountSearchBox(logMatrixFrame); m_brokerEdit->setMinimumHeight(24);
    m_brokerEdit->configureForBrokerSearch();

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
    narrLbl->setStyleSheet(ErpTheme::mutedLabelStyle(11));
    narrRow->addWidget(narrLbl);

    m_narrationEdit = new QLineEdit(this);
    m_narrationEdit->setMinimumHeight(24);
    narrRow->addWidget(m_narrationEdit, 1);
    leftColLayout->addLayout(narrRow);

    bottomSplitLayout->addLayout(leftColLayout, 3);

    // Right Column: Financial Summary Box + G. Total Directly Below It
    QVBoxLayout* rightColLayout = new QVBoxLayout();
    rightColLayout->setSpacing(4);

    m_summaryFrame = new QFrame(this);
    m_summaryFrame->setStyleSheet(ErpTheme::headerCardStyle());
    QGridLayout* sGrid = new QGridLayout(m_summaryFrame);
    sGrid->setContentsMargins(8, 6, 8, 6);
    sGrid->setHorizontalSpacing(8);
    sGrid->setVerticalSpacing(3);

    // Row 0: Mandi Total (Visible only in Mandi Type)
    m_mandiTotalTitleLabel = new QLabel("Total :", m_summaryFrame);
    m_mandiTotalTitleLabel->setStyleSheet(QString("color: %1; text-decoration: underline; font-weight: 800; border: none; background: transparent;").arg(ErpTheme::kBlue.name()));
    m_mandiTotalValLabel = new QLabel("0.00", m_summaryFrame);
    m_mandiTotalValLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    m_mandiTotalValLabel->setStyleSheet(QString("font-size: 11.5px; font-weight: 800; color: %1; border: none; background: transparent;").arg(ErpTheme::kNavy.name()));
    sGrid->addWidget(m_mandiTotalTitleLabel, 0, 0);
    sGrid->addWidget(m_mandiTotalValLabel, 0, 1);

    // Row 1: Commission (Mandi) / Other Exp (Standard)
    m_commissionTitleLabel = new QLabel("Commission :", m_summaryFrame);
    m_commissionEdit = new QLineEdit("0.00", m_summaryFrame);
    m_commissionEdit->setMinimumHeight(22);
    m_commissionEdit->setAlignment(Qt::AlignRight);
    connect(m_commissionEdit, &QLineEdit::textChanged, this, &SalesVoucherWidget::recalculateTotals);
    sGrid->addWidget(m_commissionTitleLabel, 1, 0);
    sGrid->addWidget(m_commissionEdit, 1, 1);

    m_otherExpTitleLabel = new QLabel("Other Exp. :", m_summaryFrame);
    m_otherExpTitleLabel->setStyleSheet(QString("color: %1; text-decoration: underline; border: none; background: transparent;").arg(ErpTheme::kMuted.name()));
    m_otherExpEdit = new QLineEdit("0.00", m_summaryFrame);
    m_otherExpEdit->setMinimumHeight(22);
    m_otherExpEdit->setAlignment(Qt::AlignRight);
    connect(m_otherExpEdit, &QLineEdit::textChanged, this, &SalesVoucherWidget::recalculateTotals);
    sGrid->addWidget(m_otherExpTitleLabel, 1, 0);
    sGrid->addWidget(m_otherExpEdit, 1, 1);

    // Row 2: Less Amount
    m_lessTitleLabel = new QLabel("(-) Less :", m_summaryFrame);
    m_lessTitleLabel->setStyleSheet(QString("color: %1; text-decoration: underline; border: none; background: transparent;").arg(ErpTheme::kRed.name()));
    m_lessAmountEdit = new QLineEdit("0.00", m_summaryFrame);
    m_lessAmountEdit->setMinimumHeight(22);
    m_lessAmountEdit->setAlignment(Qt::AlignRight);
    connect(m_lessAmountEdit, &QLineEdit::textChanged, this, &SalesVoucherWidget::recalculateTotals);
    sGrid->addWidget(m_lessTitleLabel, 2, 0);
    sGrid->addWidget(m_lessAmountEdit, 2, 1);

    // Row 3: Taxes
    m_taxTitleLabel = new QLabel("SGST+CGST :", m_summaryFrame);
    m_taxTitleLabel->setStyleSheet(QString("color: %1; border: none; background: transparent;").arg(ErpTheme::kMuted.name()));
    m_taxAmountEdit = new QLineEdit("0.00", m_summaryFrame);
    m_taxAmountEdit->setMinimumHeight(22);
    m_taxAmountEdit->setAlignment(Qt::AlignRight);
    connect(m_taxAmountEdit, &QLineEdit::textChanged, this, &SalesVoucherWidget::recalculateTotals);
    sGrid->addWidget(m_taxTitleLabel, 3, 0);
    sGrid->addWidget(m_taxAmountEdit, 3, 1);

    // Row 4: Freight
    m_freightTitleLabel = new QLabel("(+) Freight :", m_summaryFrame);
    m_freightTitleLabel->setStyleSheet(QString("color: %1; border: none; background: transparent;").arg(ErpTheme::kMuted.name()));
    m_freightChargesEdit = new QLineEdit("0.00", m_summaryFrame);
    m_freightChargesEdit->setMinimumHeight(22);
    m_freightChargesEdit->setAlignment(Qt::AlignRight);
    connect(m_freightChargesEdit, &QLineEdit::textChanged, this, &SalesVoucherWidget::recalculateTotals);
    sGrid->addWidget(m_freightTitleLabel, 4, 0);
    sGrid->addWidget(m_freightChargesEdit, 4, 1);

    // Row 5: Round Off
    m_roundOffTitleLabel = new QLabel("Round +/-", m_summaryFrame);
    m_roundOffTitleLabel->setStyleSheet(QString("color: %1; text-decoration: underline; border: none; background: transparent;").arg(ErpTheme::kMuted.name()));
    m_roundOffEdit = new QLineEdit("0.00", m_summaryFrame);
    m_roundOffEdit->setMinimumHeight(22);
    m_roundOffEdit->setAlignment(Qt::AlignRight);
    connect(m_roundOffEdit, &QLineEdit::textEdited, this, [this]() {
        if (m_isUpdatingTotals) return;
        m_isUpdatingTotals = true;
        double raw = calculateRawTotal();
        double tcs = m_tcsAmountEdit ? m_tcsAmountEdit->text().toDouble() : 0.0;
        double ro = m_roundOffEdit->text().toDouble();
        double gt = raw + tcs + ro;
        if (m_grandTotalEdit) m_grandTotalEdit->setText(QString::number(gt, 'f', 2));
        m_isUpdatingTotals = false;
    });
    sGrid->addWidget(m_roundOffTitleLabel, 5, 0);
    sGrid->addWidget(m_roundOffEdit, 5, 1);

    // Row 6: TCS (mirrors Purchase TDS row: title + Calc dialog button)
    QHBoxLayout* tcsTitleLayout = new QHBoxLayout();
    tcsTitleLayout->setContentsMargins(0, 0, 0, 0);
    tcsTitleLayout->setSpacing(4);

    m_tcsTitleLabel = new QLabel("TCS @ 0.0% :", m_summaryFrame);
    m_tcsTitleLabel->setStyleSheet(QString("color: %1; font-weight: 800; border: none; background: transparent;").arg(ErpTheme::kRed.name()));
    tcsTitleLayout->addWidget(m_tcsTitleLabel);

    m_tcsCalcBtn = new QPushButton("Calc", m_summaryFrame);
    m_tcsCalcBtn->setMinimumSize(38, 20);
    m_tcsCalcBtn->setCursor(Qt::PointingHandCursor);
    m_tcsCalcBtn->setToolTip("Open TCS u/s 206C(1H) Calculation Dialog (TCS on bill amount, no turnover gate)");
    m_tcsCalcBtn->setStyleSheet(ErpTheme::secondaryButtonStyle());
    connect(m_tcsCalcBtn, &QPushButton::clicked, this, &SalesVoucherWidget::openTcsCalculationDialog);
    tcsTitleLayout->addWidget(m_tcsCalcBtn);

    sGrid->addLayout(tcsTitleLayout, 6, 0);
    m_tcsAmountEdit = new QLineEdit("0.00", m_summaryFrame);
    m_tcsAmountEdit->setMinimumHeight(22);
    m_tcsAmountEdit->setAlignment(Qt::AlignRight);
    connect(m_tcsAmountEdit, &QLineEdit::textEdited, this, [this]() {
        if (m_isUpdatingTotals) return;
        m_isUpdatingTotals = true;
        double raw = calculateRawTotal();
        double tcs = m_tcsAmountEdit->text().toDouble();
        double finalTotal = raw + tcs;
        double rounded = std::round(finalTotal);
        double ro = rounded - finalTotal;
        if (m_roundOffEdit) m_roundOffEdit->setText(QString::number(ro, 'f', 2));
        if (m_grandTotalEdit) m_grandTotalEdit->setText(QString::number(rounded, 'f', 2));
        m_isUpdatingTotals = false;
    });
    m_tcsRateEdit = new QLineEdit("0.000", m_summaryFrame);
    m_tcsRateEdit->setVisible(false);
    sGrid->addWidget(m_tcsTitleLabel, 6, 0);
    sGrid->addWidget(m_tcsAmountEdit, 6, 1);

    rightColLayout->addWidget(m_summaryFrame);

    // G. Total Row directly below the Financial Summary Box
    QHBoxLayout* gtRow = new QHBoxLayout();
    gtRow->setSpacing(6);
    QLabel* gtTitle = new QLabel("G. Total :", this);
    gtTitle->setStyleSheet(ErpTheme::headerTitleStyle(14));
    gtRow->addWidget(gtTitle);

    m_grandTotalEdit = new QLineEdit("0.00", this);
    m_grandTotalEdit->setMinimumHeight(28);
    m_grandTotalEdit->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    m_grandTotalEdit->setStyleSheet(QString("font-size: 16px; font-weight: 900; color: %1; background-color: #FFFFFF; border: 2px solid %2; border-radius: 6px; padding: 2px 6px;").arg(ErpTheme::kNavy.name(), ErpTheme::kBlue.name()));
    connect(m_grandTotalEdit, &QLineEdit::textEdited, this, [this]() {
        if (m_isUpdatingTotals) return;
        m_isUpdatingTotals = true;
        double raw = calculateRawTotal();
        double tcs = m_tcsAmountEdit ? m_tcsAmountEdit->text().toDouble() : 0.0;
        double gt = m_grandTotalEdit->text().toDouble();
        double ro = gt - (raw + tcs);
        if (m_roundOffEdit) m_roundOffEdit->setText(QString::number(ro, 'f', 2));
        m_isUpdatingTotals = false;
    });
    gtRow->addWidget(m_grandTotalEdit, 1);
    rightColLayout->addLayout(gtRow);

    bottomSplitLayout->addLayout(rightColLayout, 1);
    rootLayout->addLayout(bottomSplitLayout);

    // ========================================================================
    // 7. FOOTER ACTION BUTTONS BAR
    // ========================================================================
    QHBoxLayout* footerLayout = new QHBoxLayout();
    footerLayout->setSpacing(8);
    footerLayout->addStretch(1);

    // Footer actions use badge buttons so shortcuts are visible and truthful:
    // Esc/F8/Ctrl+S are owned by keyPressEvent; Ctrl+P/Ctrl+E were added there
    // in this pass (previously the badges promised shortcuts that did nothing).
    m_backBtn = new KbdBadgeButton("Back", "Esc", this);
    m_backBtn->setMinimumHeight(30);
    connect(m_backBtn, &QPushButton::clicked, this, &SalesVoucherWidget::backRequested);
    footerLayout->addWidget(m_backBtn);

    m_newBtn = new KbdBadgeButton("New", "F8", this);
    m_newBtn->setMinimumHeight(30);
    connect(m_newBtn, &QPushButton::clicked, this, &SalesVoucherWidget::resetForm);
    footerLayout->addWidget(m_newBtn);

    m_deleteBtn = new KbdBadgeButton("Delete", "Del", this);
    m_deleteBtn->setMinimumHeight(30);
    m_deleteBtn->setPrimaryColor(ErpTheme::kRedBg, ErpTheme::kRedBorder);
    m_deleteBtn->setTextColor(ErpTheme::kRed);
    m_deleteBtn->setVisible(false);
    connect(m_deleteBtn, &QPushButton::clicked, this, &SalesVoucherWidget::deleteVoucher);
    footerLayout->addWidget(m_deleteBtn);

    m_printBtn = new KbdBadgeButton("Print", "Ctrl+P", this);
    m_printBtn->setMinimumHeight(30);
    connect(m_printBtn, &QPushButton::clicked, this, &SalesVoucherWidget::printInvoice);
    footerLayout->addWidget(m_printBtn);

    m_pdfBtn = new KbdBadgeButton("PDF", "Ctrl+E", this);
    m_pdfBtn->setMinimumHeight(30);
    connect(m_pdfBtn, &QPushButton::clicked, this, &SalesVoucherWidget::exportPdf);
    footerLayout->addWidget(m_pdfBtn);

    m_saveBtn = new KbdBadgeButton("Save Voucher", "Ctrl+S", this);
    m_saveBtn->setMinimumHeight(30);
    m_saveBtn->setPrimaryColor(ErpTheme::kBlue, ErpTheme::kBlueDark);
    m_saveBtn->setTextColor(ErpTheme::kPaper);
    m_saveBtn->setFocusPolicy(Qt::StrongFocus);
    m_saveBtn->installEventFilter(this);
    connect(m_saveBtn, &QPushButton::clicked, this, &SalesVoucherWidget::saveVoucher);
    footerLayout->addWidget(m_saveBtn);

    rootLayout->addLayout(footerLayout);

    m_statusBanner = new QLabel(this);
    m_statusBanner->setVisible(false);
    rootLayout->addWidget(m_statusBanner);
}

void SalesVoucherWidget::applyCustomStyles() {
    onMarketTypeChanged(0);
    onTaxStatusChanged();
}

void SalesVoucherWidget::onMarketTypeChanged(int index) {
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

void SalesVoucherWidget::onTaxStatusChanged() {
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

void SalesVoucherWidget::onSaleStatusChanged() {
    if (m_saleSelfRadio->isChecked()) m_selectedSaleStatus = "Self Sale";
    else if (m_saleTransferRadio->isChecked()) m_selectedSaleStatus = "Stock Transfer";
    else if (m_saleLagatRadio->isChecked()) m_selectedSaleStatus = "Lagat Bill";
    else if (m_saleThirdPartyRadio->isChecked()) m_selectedSaleStatus = "Third Party";
}

void SalesVoucherWidget::onMarketFeeStatusChanged() {
    if (m_feePayableRadio->isChecked()) m_selectedMarketFeeStatus = "Payable";
    else if (m_feePaidRadio->isChecked()) m_selectedMarketFeeStatus = "Paid";
}

void SalesVoucherWidget::onDateChanged(const QDate& date) {
    updateDayOfWeek(date);
    updateFiscalYearBadge();
    if (!isEditMode()) {
        updateNextNumbers();
    }
}

void SalesVoucherWidget::updateDayOfWeek(const QDate& date) {
    if (m_dayOfWeekLabel) {
        m_dayOfWeekLabel->setText(date.isValid() ? date.toString("dddd") : QDate::currentDate().toString("dddd"));
    }
}

void SalesVoucherWidget::updateFiscalYearBadge() {
}

void SalesVoucherWidget::updateNextNumbers() {
    if (isEditMode()) return;
    FiscalYearInfo fy = FiscalYearHelper::getActiveFiscalYear();
    QString dt = m_invoiceDateEdit ? m_invoiceDateEdit->formattedDate() : "";

    QString invNo = m_salesModel.get_next_invoice_no(dt);
    QString vNo = m_salesModel.get_next_voucher_no(dt);

    if (m_invoiceNoEdit && m_invoiceNoEdit->text().trimmed().isEmpty()) {
        m_invoiceNoEdit->setText(invNo);
    }
    if (m_voucherNoDisplay) {
        // Show the full canonical identity ("2627/Sale-248"); never strip the
        // family token and never mutate the working copy.
        m_voucherNoDisplay->setText("No. " + vNo);
    }
}

void SalesVoucherWidget::onPartySelected(const QVariantMap& partyData) {
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

void SalesVoucherWidget::setupNavigationChains() {
    // Shared helpers: focus-with-select, and step back into the item grid
    // (the grid holds no widget target, so grid-boundary installs carry it).
    auto go = [](QWidget* w) {
        if (!w) return;
        w->setFocus();
        if (auto* le = qobject_cast<QLineEdit*>(w)) le->selectAll();
        else if (auto* cb = qobject_cast<QComboBox*>(w)) { if (cb->lineEdit()) cb->lineEdit()->selectAll(); }
    };
    auto stepBackToGrid = [this]() {
        if (!m_tableWidget || m_tableWidget->rowCount() == 0) return;
        int r = m_tableWidget->rowCount() - 1, c = 7;
        while (c > 1 && m_tableWidget->isColumnHidden(c)) --c;
        m_tableWidget->setCurrentCell(r, c);
        m_tableWidget->edit(m_tableWidget->currentIndex());
    };
    // Left/Right arrow navigation across fields
    VoucherCommon::installGridNavigation(m_invoiceNoEdit, m_marketTypeCombo, m_dueDaysEdit, [this]() {
        QString text = m_invoiceNoEdit ? m_invoiceNoEdit->text().trimmed() : "";
        if (!text.isEmpty()) {
            QString curDate = m_invoiceDateEdit ? m_invoiceDateEdit->date().toString("yyyy-MM-dd") : "";
            QVariantMap existing = m_salesModel.get_sales_invoice(text, curDate);
            if (!existing.isEmpty() && existing.value("id").toInt() != m_editingInvoiceId) {
                loadInvoiceForEditing(existing.value("id"), curDate);
                return;
            }
        }
        if (m_dueDaysEdit) { m_dueDaysEdit->setFocus(); m_dueDaysEdit->selectAll(); }
    });
    VoucherCommon::installGridNavigation(m_dueDaysEdit, m_invoiceNoEdit, m_partySearchWidget, [this]() {
        if (m_partySearchWidget) { m_partySearchWidget->setFocus(); m_partySearchWidget->selectAll(); }
    });
    VoucherCommon::installGridNavigation(m_partySearchWidget, m_dueDaysEdit, m_posCombo, [this]() {
        focusTableAt(0, 1);
    });
    VoucherCommon::installGridNavigation(m_posCombo, m_partySearchWidget, m_vehicleNoEdit, [this]() {
        focusTableAt(0, 1);
    });
    // Market-type dropdown: Enter advances instead of opening the list
    // (list still opens natively with Down/Space).
    VoucherCommon::installGridNavigation(m_marketTypeCombo, m_invoiceNoEdit, m_dueDaysEdit, [this, go]() { go(m_dueDaysEdit); });

    // NOTE: no QShortcuts on the grid: a window-context shortcut swallows
    // grid keys before editors/delegate ever see them. The no-editor-open
    // case is handled in keyPressEvent below (events only propagate here
    // when no child consumed them, so open editors are never disturbed).

    // Single key contract per field (Enter/Left/Right/Backspace): the forward
    // order lives here once — Backspace/Left derive backward automatically,
    // so no parallel returnPressed wiring exists to drift apart.
    // Mandi charges chain.
    VoucherCommon::installGridNavigation(m_damiEdit, nullptr, nullptr, [this, go]() { go(m_labourEdit); }, stepBackToGrid);
    VoucherCommon::installGridNavigation(m_labourEdit, m_damiEdit, nullptr, [this, go]() { go(m_auctionEdit); });
    VoucherCommon::installGridNavigation(m_auctionEdit, m_labourEdit, nullptr, [this, go]() { go(m_mFeeEdit); });
    VoucherCommon::installGridNavigation(m_mFeeEdit, m_auctionEdit, nullptr, [this, go]() { go(m_hrdfEdit); });
    VoucherCommon::installGridNavigation(m_hrdfEdit, m_mFeeEdit, nullptr, [this, go]() { go(m_mandiOtherExpEdit); });
    VoucherCommon::installGridNavigation(m_mandiOtherExpEdit, m_hrdfEdit, nullptr, [this, go]() { go(m_welfareEdit); });
    VoucherCommon::installGridNavigation(m_welfareEdit, m_mandiOtherExpEdit, nullptr, [this, go]() { go(m_dhrmdEdit); });
    VoucherCommon::installGridNavigation(m_dhrmdEdit, m_welfareEdit, nullptr, [this, go]() { go(m_sutliEdit); });
    VoucherCommon::installGridNavigation(m_sutliEdit, m_dhrmdEdit, nullptr, [this, go]() { go(m_mandiLessEdit); });
    VoucherCommon::installGridNavigation(m_mandiLessEdit, m_sutliEdit, nullptr, [this, go]() { go(m_vehicleNoEdit); });

    // Logistics chain.
    VoucherCommon::installGridNavigation(m_vehicleNoEdit, m_mandiLessEdit, m_grNoEdit, [this, go]() { go(m_grNoEdit); }, stepBackToGrid);
    VoucherCommon::installGridNavigation(m_grNoEdit, m_vehicleNoEdit, m_driverNameEdit, [this, go]() { go(m_driverNameEdit); });
    VoucherCommon::installGridNavigation(m_driverNameEdit, m_grNoEdit, m_ewayBillNoEdit, [this, go]() { go(m_ewayBillNoEdit); });
    VoucherCommon::installGridNavigation(m_ewayBillNoEdit, m_driverNameEdit, m_billTimeEdit, [this, go]() { go(m_billTimeEdit); });
    VoucherCommon::installGridNavigation(m_billTimeEdit, m_ewayBillNoEdit, m_saudaDateEdit, [this, go]() { go(m_saudaDateEdit); });
    VoucherCommon::installGridNavigation(m_saudaDateEdit, m_billTimeEdit, m_poNoEdit, [this, go]() { go(m_poNoEdit); });
    VoucherCommon::installGridNavigation(m_poNoEdit, m_saudaDateEdit, m_gradeEdit, [this, go]() { go(m_gradeEdit); });
    VoucherCommon::installGridNavigation(m_gradeEdit, m_poNoEdit, m_transportEdit, [this, go]() { go(m_transportEdit); });
    VoucherCommon::installGridNavigation(m_transportEdit, m_gradeEdit, m_challanNoEdit, [this, go]() { go(m_challanNoEdit); });
    VoucherCommon::installGridNavigation(m_challanNoEdit, m_transportEdit, m_kandaWeightEdit, [this, go]() { go(m_kandaWeightEdit); });
    VoucherCommon::installGridNavigation(m_kandaWeightEdit, m_challanNoEdit, m_brokerEdit, [this]() {
        double kw = m_kandaWeightEdit ? m_kandaWeightEdit->text().trimmed().toDouble() : 0.0;
        if (kw > 0.0) {
            int totalBags = 0;
            if (m_tableWidget) {
                for (int r = 0; r < m_tableWidget->rowCount(); ++r) {
                    auto* item = m_tableWidget->item(r, 3); // Bags column
                    if (item) totalBags += item->text().toInt();
                }
            }
            double calcFreight = 0.0;
            if (FreightCalculationDialog::promptFreight(this, kw, totalBags, &calcFreight)) {
                if (m_freightChargesEdit) {
                    m_freightChargesEdit->setText(QString::number(calcFreight, 'f', 2));
                    recalculateTotals();
                }
            }
        }
        if (m_brokerEdit) {
            m_brokerEdit->setFocus();
            m_brokerEdit->selectAll();
        }
    });
    VoucherCommon::installGridNavigation(m_brokerEdit, m_kandaWeightEdit, m_narrationEdit, [this, go]() { go(m_narrationEdit); });

    // Summary & Save chain: single declaration per field (Enter/Left/Right/Backspace).
    VoucherCommon::installGridNavigation(m_narrationEdit, m_brokerEdit, nullptr, [this, go]() {
        if (m_otherExpEdit && m_otherExpEdit->isVisible()) {
            go(m_otherExpEdit);
        } else if (m_commissionEdit && m_commissionEdit->isVisible()) {
            go(m_commissionEdit);
        } else if (m_taxAmountEdit && m_taxAmountEdit->isVisible()) {
            go(m_taxAmountEdit);
        } else if (m_saveBtn) {
            m_saveBtn->setFocus();
        }
    });

    // Summary & Save chain: single declaration per field (see above).
    VoucherCommon::installGridNavigation(m_commissionEdit, m_narrationEdit, nullptr,
        [this, go]() { go(m_freightChargesEdit); });
    VoucherCommon::installGridNavigation(m_otherExpEdit, m_narrationEdit, nullptr,
        [this, go]() { go(m_lessAmountEdit); });
    VoucherCommon::installGridNavigation(m_lessAmountEdit, m_otherExpEdit, nullptr,
        [this, go]() { go(m_taxAmountEdit); });
    VoucherCommon::installGridNavigation(m_taxAmountEdit, m_lessAmountEdit, nullptr,
        [this, go]() { go(m_freightChargesEdit); });
    VoucherCommon::installGridNavigation(m_freightChargesEdit, m_taxAmountEdit, nullptr,
        [this, go]() { go(m_roundOffEdit); });
    VoucherCommon::installGridNavigation(m_roundOffEdit, m_freightChargesEdit, nullptr,
        [this, go]() {
            go(m_tcsAmountEdit);
            openTcsCalculationDialog();
        });
    VoucherCommon::installGridNavigation(m_tcsAmountEdit, m_roundOffEdit, nullptr,
        [this, go]() {
            openTcsCalculationDialog();
        });
    VoucherCommon::installGridNavigation(m_grandTotalEdit, m_tcsAmountEdit, nullptr,
        [this]() { if (m_saveBtn) m_saveBtn->setFocus(); });
}

void SalesVoucherWidget::setupCompleters() {
    if (m_brokerEdit) {
        m_brokerEdit->configureForBrokerSearch();
    }
    if (m_transportEdit) {
        m_transportEdit->configureForTransportSearch();
    }
}

void SalesVoucherWidget::focusTableAt(int row, int col) {
    if (!m_tableWidget || m_tableWidget->rowCount() == 0) return;
    int r = qBound(0, row, m_tableWidget->rowCount() - 1);
    int c = qBound(0, col, m_tableWidget->columnCount() - 1);
    m_tableWidget->setCurrentCell(r, c);
    m_tableWidget->setFocus(Qt::OtherFocusReason);
    m_tableWidget->edit(m_tableWidget->currentIndex());
}

void SalesVoucherWidget::advanceCell() {
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
    // Col 8 is GST (read-only); skip past it so advance finishes row
    if (nextCol == 8) {
        nextCol = 9;
    }
    while (nextCol < 9 && m_tableWidget->isColumnHidden(nextCol)) {
        nextCol++;
    }

    if (nextCol <= 7) {
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

void SalesVoucherWidget::retreatCell() {
    if (!m_tableWidget) return;
    int curRow = m_tableWidget->currentRow();
    int curCol = m_tableWidget->currentColumn();

    int prevCol = curCol - 1;
    if (prevCol == 8) { // Skip GST (read-only)
        prevCol = 7;
    }
    while (prevCol >= 1 && m_tableWidget->isColumnHidden(prevCol)) {
        prevCol--;
    }

    if (prevCol >= 1) {
        m_tableWidget->setCurrentCell(curRow, prevCol);
        m_tableWidget->edit(m_tableWidget->currentIndex());
    } else if (curRow > 0) {
        m_tableWidget->setCurrentCell(curRow - 1, 7);
        m_tableWidget->edit(m_tableWidget->currentIndex());
    } else {
        if (m_partySearchWidget) {
            m_partySearchWidget->setFocus();
            m_partySearchWidget->selectAll();
        }
    }
}

void SalesVoucherWidget::moveCell(int row, int col) {
    focusTableAt(row, col);
}

void SalesVoucherWidget::onStockItemConfigured(int row, const QVariantMap& itemData) {
    if (row < 0 || row >= m_tableWidget->rowCount()) return;
    m_isUpdatingTable = true;
    m_tableWidget->item(row, 1)->setText(itemData.value("name").toString());
    double gst = itemData.value("gst_rate", 0.0).toDouble();
    if (gst == 0.0) gst = itemData.value("tax_rate", 0.0).toDouble();
    if (m_tableWidget->item(row, 8)) m_tableWidget->item(row, 8)->setText(QString::number(gst, 'f', 0));
    double rate = itemData.value("sale_rate", 0.0).toDouble();
    if (rate == 0.0) rate = itemData.value("sales_rate", 0.0).toDouble();
    if (rate > 0 && m_tableWidget->item(row, 6)) m_tableWidget->item(row, 6)->setText(QString::number(rate, 'f', 2));
    m_isUpdatingTable = false;
    focusTableAt(row, 2);
}

void SalesVoucherWidget::openDateDialog(bool isInitial) {
    Q_UNUSED(isInitial);
    QString curDate = m_invoiceDateEdit ? m_invoiceDateEdit->formattedDate() : "";
    QString curIso = m_invoiceDateEdit ? m_invoiceDateEdit->isoDate() : "";
    FiscalYearInfo fyCtx = isEditMode()
        ? FiscalYearHelper::getFiscalYearForDate(curIso)
        : FiscalYearHelper::getActiveFiscalYear();
    if (!fyCtx.isValid()) fyCtx = FiscalYearHelper::getActiveFiscalYear();
    QString newDisp, newIso;
    if (VoucherDateDialog::getVoucherDate(this, curDate, &newDisp, &newIso, fyCtx)) {
        if (m_invoiceDateEdit) {
            m_invoiceDateEdit->setIsoDate(newIso);
        }
        onDateChanged(QDate::fromString(newIso, "yyyy-MM-dd"));
    }
    if (m_partySearchWidget) {
        m_partySearchWidget->setFocus();
        m_partySearchWidget->selectAll();
    }
}

void SalesVoucherWidget::showEvent(QShowEvent* event) {
    QWidget::showEvent(event);
}

void SalesVoucherWidget::resetForm() {
    m_editingInvoiceId = 0;
    m_editingInvoiceNo.clear();
    m_editingVoucherNo.clear();
    m_hasInitialDateOpened = false;

    m_titleHeaderLabel->setText("F8 : Sale Voucher");
    m_titleHeaderLabel->setStyleSheet(ErpTheme::headerTitleStyle(15));
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
    if (m_tcsAmountEdit) m_tcsAmountEdit->setText("0.00");
    if (m_roundOffEdit) m_roundOffEdit->setText("0.00");
    if (m_grandTotalEdit) m_grandTotalEdit->setText("0.00");

    updateNextNumbers();
    recalculateTotals();
}

void SalesVoucherWidget::setWorkingDate(const QString& dateStr) {
    if (m_invoiceDateEdit) {
        m_invoiceDateEdit->setIsoDate(dateStr);
        updateDayOfWeek(m_invoiceDateEdit->date());
    }
    updateNextNumbers();
}

void SalesVoucherWidget::addNewLineRow() {
    int row = m_tableWidget->rowCount();
    m_isUpdatingTable = true;
    m_tableWidget->insertRow(row);
    populateRow(row, "", "", 0, 0.500, 0.0, 0.0, 0.0, 0.0);
    m_isUpdatingTable = false;
}

void SalesVoucherWidget::removeLineRow(int row) {
    if (row >= 0 && row < m_tableWidget->rowCount()) {
        m_isUpdatingTable = true;
        m_tableWidget->removeRow(row);
        m_isUpdatingTable = false;
        recalculateTotals();
    }
}

void SalesVoucherWidget::populateRow(int row, const QString& itemName, const QString& grade, int bags, double packing, double weight, double rate, double amount, double gstPct) {
    if (row >= m_tableWidget->rowCount()) {
        m_tableWidget->insertRow(row);
    }
    m_isUpdatingTable = true;

    // Explicit foregrounds on every cell (spec §2 dark-mode hardening).
    // Read-only No. + GST stay white; editable cells get the subtle tint.
    const QBrush navyFg(ErpTheme::kNavy);
    const QBrush grayFg(ErpTheme::kMuted);
    const QBrush editBg(ErpTheme::kEditableBg);
    const QBrush whiteBg(ErpTheme::kPaper);

    QTableWidgetItem* snoItem = new QTableWidgetItem(QString::number(row + 1));
    snoItem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
    snoItem->setTextAlignment(Qt::AlignCenter);
    snoItem->setForeground(grayFg);
    snoItem->setBackground(whiteBg);
    m_tableWidget->setItem(row, 0, snoItem);

    QTableWidgetItem* nameItem = new QTableWidgetItem(itemName);
    nameItem->setForeground(navyFg);
    nameItem->setBackground(editBg);
    m_tableWidget->setItem(row, 1, nameItem);

    QTableWidgetItem* gradeItem = new QTableWidgetItem(grade);
    gradeItem->setForeground(navyFg);
    gradeItem->setBackground(editBg);
    m_tableWidget->setItem(row, 2, gradeItem);

    QTableWidgetItem* bagsItem = new QTableWidgetItem(bags > 0 ? QString::number(bags) : "");
    bagsItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
    bagsItem->setForeground(navyFg);
    bagsItem->setBackground(editBg);
    m_tableWidget->setItem(row, 3, bagsItem);

    QTableWidgetItem* pkgItem = new QTableWidgetItem(packing > 0.0 ? QString::number(packing, 'f', 3) : "0.500");
    pkgItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
    pkgItem->setForeground(navyFg);
    pkgItem->setBackground(editBg);
    m_tableWidget->setItem(row, 4, pkgItem);

    QTableWidgetItem* wtItem = new QTableWidgetItem(weight > 0.0 ? QString::number(weight, 'f', 3) : "");
    wtItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
    wtItem->setForeground(navyFg);
    wtItem->setBackground(editBg);
    m_tableWidget->setItem(row, 5, wtItem);

    QTableWidgetItem* rateItem = new QTableWidgetItem(rate > 0.0 ? QString::number(rate, 'f', 2) : "");
    rateItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
    rateItem->setForeground(navyFg);
    rateItem->setBackground(editBg);
    m_tableWidget->setItem(row, 6, rateItem);

    QTableWidgetItem* amtItem = new QTableWidgetItem(amount > 0.0 ? QString::number(amount, 'f', 2) : "");
    amtItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
    amtItem->setForeground(QBrush(ErpTheme::kGreenDark));
    amtItem->setBackground(editBg);
    m_tableWidget->setItem(row, 7, amtItem);

    QTableWidgetItem* gstItem = new QTableWidgetItem(gstPct > 0.0 ? QString::number(gstPct, 'f', 0) : "0");
    gstItem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
    gstItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
    gstItem->setForeground(grayFg);
    gstItem->setBackground(whiteBg);
    m_tableWidget->setItem(row, 8, gstItem);

    m_isUpdatingTable = false;
}

void SalesVoucherWidget::onTableCellChanged(int row, int column) {
    if (m_isUpdatingTable) return;
    m_isUpdatingTable = true;

    if (column == 3 || column == 4 || column == 5 || column == 6) {
        int bags = m_tableWidget->item(row, 3) ? m_tableWidget->item(row, 3)->text().toInt() : 0;
        double pkg = m_tableWidget->item(row, 4) ? m_tableWidget->item(row, 4)->text().toDouble() : 0.500;
        double wt = m_tableWidget->item(row, 5) ? m_tableWidget->item(row, 5)->text().toDouble() : 0.0;
        double rate = m_tableWidget->item(row, 6) ? m_tableWidget->item(row, 6)->text().toDouble() : 0.0;

        if (column == 3 && pkg > 0.0001 && wt <= 0.0001) {
            wt = (bags * pkg);
            if (m_tableWidget->item(row, 5)) m_tableWidget->item(row, 5)->setText(QString::number(wt, 'f', 3));
        }

        double amt = wt * rate;
        if (m_tableWidget->item(row, 7)) m_tableWidget->item(row, 7)->setText(QString::number(amt, 'f', 2));
    }

    m_isUpdatingTable = false;
    recalculateTotals();

    // Auto-append the next blank row only when the user actually typed an
    // amount here (non-empty new text with an item set).
    if (row == m_tableWidget->rowCount() - 1 && column == 7) {
        auto* amtIt = m_tableWidget->item(row, 7);
        if (amtIt && !amtIt->text().trimmed().isEmpty() &&
            m_tableWidget->item(row, 1) && !m_tableWidget->item(row, 1)->text().isEmpty()) {
            addNewLineRow();
        }
    }
}

double SalesVoucherWidget::calculateRawTotal() const {
    double taxableSubtotal = m_subtotalTaxableLabel ? m_subtotalTaxableLabel->text().toDouble() : 0.0;
    bool isMandi = (m_marketTypeCombo && m_marketTypeCombo->currentIndex() == 2);
    double otherExp = 0.0;
    double lessAmt = 0.0;

    if (isMandi) {
        double dami = m_damiEdit ? m_damiEdit->text().toDouble() : 0.0;
        double labour = m_labourEdit ? m_labourEdit->text().toDouble() : 0.0;
        double auction = m_auctionEdit ? m_auctionEdit->text().toDouble() : 0.0;
        double mfee = m_mFeeEdit ? m_mFeeEdit->text().toDouble() : 0.0;
        double hrdf = m_hrdfEdit ? m_hrdfEdit->text().toDouble() : 0.0;
        double mOther = m_mandiOtherExpEdit ? m_mandiOtherExpEdit->text().toDouble() : 0.0;
        double welfare = m_welfareEdit ? m_welfareEdit->text().toDouble() : 0.0;
        double dhrmd = m_dhrmdEdit ? m_dhrmdEdit->text().toDouble() : 0.0;
        double sutli = m_sutliEdit ? m_sutliEdit->text().toDouble() : 0.0;
        lessAmt = m_mandiLessEdit ? m_mandiLessEdit->text().toDouble() : 0.0;
        otherExp = dami + labour + auction + mfee + hrdf + mOther + welfare + dhrmd + sutli;
    } else {
        otherExp = m_otherExpEdit ? m_otherExpEdit->text().toDouble() : 0.0;
        lessAmt = m_lessAmountEdit ? m_lessAmountEdit->text().toDouble() : 0.0;
    }

    double freight = m_freightChargesEdit ? m_freightChargesEdit->text().toDouble() : 0.0;
    double comm = (isMandi && m_commissionEdit) ? m_commissionEdit->text().toDouble() : 0.0;
    double tax = m_taxAmountEdit ? m_taxAmountEdit->text().toDouble() : 0.0;

    return taxableSubtotal + otherExp + comm + freight + tax - lessAmt;
}

void SalesVoucherWidget::openTcsCalculationDialog() {
    // Commit any currently focused table editor before computing
    if (m_tableWidget) {
        auto* edit = m_tableWidget->findChild<QLineEdit*>();
        if (edit) edit->clearFocus();
    }
    recalculateTotals();

    QString party = m_partySearchWidget ? m_partySearchWidget->currentPartyName() : "";

    double currentBill = calculateRawTotal();

    double curTcs = m_tcsAmountEdit ? m_tcsAmountEdit->text().toDouble() : 0.0;
    double curRate = m_tcsRateEdit ? m_tcsRateEdit->text().toDouble() : 0.100;
    if (curRate <= 0.0001) curRate = 0.100;

    double outTcs = curTcs;
    double outRate = curRate;
    double outTaxable = 0.0;

    if (Tcs206CCalculationDialog::promptTcs206C(this, party, currentBill, &outTcs, &outRate, &outTaxable, curRate)) {
        if (m_tcsRateEdit) m_tcsRateEdit->setText(QString::number(outRate, 'f', 3));
        if (m_tcsTitleLabel) m_tcsTitleLabel->setText(QString("TCS @ %1% :").arg(QString::number(outRate, 'f', 1)));
        if (m_tcsAmountEdit) {
            m_tcsAmountEdit->setText(QString::number(outTcs, 'f', 2));
        }
        recalculateTotals();
    }
    if (m_grandTotalEdit) {
        m_grandTotalEdit->setFocus();
        m_grandTotalEdit->selectAll();
    }
}

void SalesVoucherWidget::recalculateTotals() {
    if (m_isUpdatingTotals) return;
    m_isUpdatingTotals = true;

    int totalBags = 0;
    double totalWeight = 0.0;
    double taxableSubtotal = 0.0;
    double totalGst = 0.0;

    for (int r = 0; r < m_tableWidget->rowCount(); ++r) {
        QString name = m_tableWidget->item(r, 1) ? m_tableWidget->item(r, 1)->text() : "";
        if (name.isEmpty()) continue;
        int b = m_tableWidget->item(r, 3) ? m_tableWidget->item(r, 3)->text().toInt() : 0;
        double w = m_tableWidget->item(r, 5) ? m_tableWidget->item(r, 5)->text().toDouble() : 0.0;
        double amt = m_tableWidget->item(r, 7) ? m_tableWidget->item(r, 7)->text().toDouble() : 0.0;
        double gst = m_tableWidget->item(r, 8) ? m_tableWidget->item(r, 8)->text().toDouble() : 0.0;

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

    if (m_taxAmountEdit->text().toDouble() == 0.0 && totalGst > 0) {
        m_taxAmountEdit->setText(QString::number(totalGst, 'f', 2));
    }

    double rawTotal = calculateRawTotal();
    double tcsRate = m_tcsRateEdit ? m_tcsRateEdit->text().toDouble() : 0.0;
    double tcsAmt = (tcsRate > 0) ? (rawTotal * tcsRate / 100.0) : (m_tcsAmountEdit ? m_tcsAmountEdit->text().toDouble() : 0.0);
    if (m_tcsAmountEdit) m_tcsAmountEdit->setText(QString::number(tcsAmt, 'f', 2));

    double finalTotal = rawTotal + tcsAmt;
    double rounded = std::round(finalTotal);
    double roundOff = rounded - finalTotal;

    if (m_roundOffEdit) m_roundOffEdit->setText(QString::number(roundOff, 'f', 2));
    if (m_grandTotalEdit) m_grandTotalEdit->setText(QString::number(rounded, 'f', 2));

    m_isUpdatingTotals = false;
}

void SalesVoucherWidget::saveVoucher() {
    QString party = m_partySearchWidget->currentPartyName();
    if (party.isEmpty()) {
        CustomMessageBox::warning(this, "Validation", "Please select a valid Party / Customer Account.");
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
        itm["rate"] = m_tableWidget->item(r, 6) ? m_tableWidget->item(r, 6)->text().toDouble() : 0.0;
        itm["amount"] = m_tableWidget->item(r, 7) ? m_tableWidget->item(r, 7)->text().toDouble() : 0.0;
        itm["gst_pct"] = m_tableWidget->item(r, 8) ? m_tableWidget->item(r, 8)->text().toDouble() : 0.0;
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

    double roundOff = m_roundOffEdit ? m_roundOffEdit->text().toDouble() : 0.0;
    double grandTotal = m_grandTotalEdit ? m_grandTotalEdit->text().toDouble() : 0.0;
    double freight = m_freightChargesEdit->text().toDouble();
    double tcsAmt = m_tcsAmountEdit ? m_tcsAmountEdit->text().toDouble() : 0.0;
    double tcsRate = m_tcsRateEdit->text().toDouble();

    bool ok = false;
    if (isEditMode()) {
        ok = m_salesModel.update_sales_invoice_full(
            m_editingInvoiceId, invNo, invDate, party, m_gstinDisplay->text(),
            items.first().toMap().value("item_name").toString(), "",
            m_totalBagsLabel->text().toInt(), m_totalWeightLabel->text().toDouble(),
            items.first().toMap().value("rate").toDouble(), taxable,
            items.first().toMap().value("gst_pct").toDouble(), cgst, sgst, igst,
            roundOff, grandTotal, "Credit", m_vehicleNoEdit->text().trimmed().toUpper(), m_ewayBillNoEdit->text(),
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
        ok = m_salesModel.add_sales_invoice_full(
            invNo, invDate, party, m_gstinDisplay->text(),
            items.first().toMap().value("item_name").toString(), "",
            m_totalBagsLabel->text().toInt(), m_totalWeightLabel->text().toDouble(),
            items.first().toMap().value("rate").toDouble(), taxable,
            items.first().toMap().value("gst_pct").toDouble(), cgst, sgst, igst,
            roundOff, grandTotal, "Credit", m_vehicleNoEdit->text().trimmed().toUpper(), m_ewayBillNoEdit->text(),
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
        CustomMessageBox::information(this, "Success", QString("Sales Invoice %1 saved successfully.").arg(invNo));
        bool editMode = isEditMode();
        emit invoiceSaved(invNo);
        if (!editMode) {
            resetForm();
            if (m_invoiceDateEdit) m_invoiceDateEdit->setFocus();
        }
    } else {
        CustomMessageBox::critical(this, "Error", "Failed to save Sales Invoice. Database error.");
    }
}

void SalesVoucherWidget::deleteVoucher() {
    if (!isEditMode()) return;
    if (CustomMessageBox::question(this, "Confirm Delete", "Are you sure you want to delete this invoice?")) {
        if (m_salesModel.delete_sales_invoice(m_editingInvoiceId)) {
            emit invoiceDeleted(m_editingInvoiceNo);
            resetForm();
        }
    }
}

bool SalesVoucherWidget::loadInvoiceForEditing(const QVariant& invNoOrId, const QString& dateHint, bool popupDateOnLoad) {
    resetForm();

    QVariantMap inv = m_salesModel.get_sales_invoice(invNoOrId, dateHint);
    if (inv.isEmpty()) {
        QString displayIdent;
        if (invNoOrId.typeId() == QMetaType::QVariantMap) {
            QVariantMap m = invNoOrId.toMap();
            displayIdent = m.value("invoiceNo", m.value("invoice_no", m.value("voucherNo", m.value("voucher_no")))).toString();
        } else {
            displayIdent = invNoOrId.toString();
        }
        if (displayIdent.isEmpty()) displayIdent = "Requested voucher";
        CustomMessageBox::warning(
            this,
            "Voucher Not Found",
            QString("Sales Voucher \"%1\" was not found or has been deleted in this financial year.").arg(displayIdent)
        );
        return false;
    }

    m_hasInitialDateOpened = true;
    m_editingInvoiceId = inv.value("id").toInt();
    m_editingInvoiceNo = inv.value("invoice_no").toString();
    m_editingVoucherNo = inv.value("voucher_no").toString();
    m_deleteBtn->setVisible(true);

    m_titleHeaderLabel->setText(QString("ALTERATION : F8 : Sale Voucher (%1)").arg(m_editingInvoiceNo));
    if (m_voucherNoDisplay) m_voucherNoDisplay->setText("No. " + m_editingVoucherNo);
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

    QString custName = inv.value("customer_name").toString();
    if (custName.isEmpty()) custName = inv.value("party_ledger").toString();
    m_partySearchWidget->setPartyName(custName);
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
    m_driverNameEdit->setText(inv.value("driver_name").toString());
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
    if (inv.contains("round_off")) m_roundOffEdit->setText(QString::number(inv.value("round_off").toDouble(), 'f', 2));
    if (inv.contains("tcs_amount")) m_tcsAmountEdit->setText(QString::number(inv.value("tcs_amount").toDouble(), 'f', 2));
    if (inv.contains("grand_total")) m_grandTotalEdit->setText(QString::number(inv.value("grand_total").toDouble(), 'f', 2));
    // Drill-down entry: date dialog first (dates take no keyboard focus),
    // then party — openDateDialog already lands there.
    if (popupDateOnLoad) openDateDialog(false);
    return true;
}

void SalesVoucherWidget::printInvoice() {
    if (!m_printExportCtrl) return;
    QString invNo = m_invoiceNoEdit ? m_invoiceNoEdit->text().trimmed() : "";
    if (invNo.isEmpty()) {
        CustomMessageBox::warning(this, "No Invoice", "Save the voucher first, then print its invoice.");
        return;
    }
    QString cust = m_partySearchWidget ? m_partySearchWidget->currentPartyName() : "";
    QString copyType = MahadevERP::PrintCopyOptionsDialog::selectCopyType(
        MahadevERP::PrintCopyOptionsDialog::Mode::Print, invNo, cust, this);
    if (!copyType.isEmpty()) {
        m_printExportCtrl->print_sales_invoice(invNo, copyType);
    }
}

void SalesVoucherWidget::exportPdf() {
    if (!m_printExportCtrl) return;
    QString invNo = m_invoiceNoEdit ? m_invoiceNoEdit->text().trimmed() : "";
    if (invNo.isEmpty()) {
        CustomMessageBox::warning(this, "No Invoice", "Save the voucher first, then export its invoice PDF.");
        return;
    }
    QString cust = m_partySearchWidget ? m_partySearchWidget->currentPartyName() : "";
    QString copyType = MahadevERP::PrintCopyOptionsDialog::selectCopyType(
        MahadevERP::PrintCopyOptionsDialog::Mode::Pdf, invNo, cust, this);
    if (!copyType.isEmpty()) {
        m_printExportCtrl->export_sales_invoice_pdf(invNo, "", copyType);
    }
}

void SalesVoucherWidget::loadPreviousVoucher() {
    QVariantMap prev = m_salesModel.get_previous_sales_invoice(m_editingInvoiceId, m_invoiceNoEdit ? m_invoiceNoEdit->text().trimmed() : "");
    if (!prev.isEmpty()) {
        loadInvoiceForEditing(prev.value("id"));
    }
}

void SalesVoucherWidget::loadNextVoucher() {
    if (m_editingInvoiceId <= 0) return;
    QVariantMap next = m_salesModel.get_next_sales_invoice(m_editingInvoiceId, m_invoiceNoEdit ? m_invoiceNoEdit->text().trimmed() : "");
    if (!next.isEmpty()) {
        loadInvoiceForEditing(next.value("id"));
    } else {
        resetForm();
    }
}

void SalesVoucherWidget::openAlterVoucherDialog() {
    bool ok = false;
    QString text = CustomInputDialog::getText(this, "Find & Alter Sales Voucher", "Enter Invoice No or Voucher No to alter:", m_editingInvoiceNo, &ok);
    if (ok && !text.trimmed().isEmpty()) {
        QString curDate = m_invoiceDateEdit ? m_invoiceDateEdit->date().toString("yyyy-MM-dd") : "";
        QVariantMap inv = m_salesModel.get_sales_invoice(text.trimmed(), curDate);
        if (!inv.isEmpty()) {
            loadInvoiceForEditing(inv.value("id"), inv.value("invoice_date").toString());
        } else {
            CustomMessageBox::warning(this, "Voucher Not Found", QString("No sales voucher found matching \"%1\" in this financial year.").arg(text.trimmed()));
        }
    }
}

void SalesVoucherWidget::keyPressEvent(QKeyEvent* event) {
    // Grid with focus but no cell editor open (unhandled keys propagate here
    // only in that state): Enter opens the current cell's editor; Backspace
    // on a filled cell clears it, on an empty cell steps back.
    if ((event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter ||
         event->key() == Qt::Key_Backspace) &&
        m_tableWidget && QApplication::focusWidget() == m_tableWidget) {
        int r = m_tableWidget->currentRow(), c = m_tableWidget->currentColumn();
        if (r >= 0 && c >= 0 && r < m_tableWidget->rowCount() && c < m_tableWidget->columnCount()) {
            if (event->key() == Qt::Key_Backspace) {
                if (QTableWidgetItem* it = m_tableWidget->item(r, c)) {
                    if (!it->text().isEmpty()) {
                        it->setText("");
                        recalculateTotals();
                        event->accept();
                        return;
                    }
                }
                retreatCell();
            } else {
                m_tableWidget->edit(m_tableWidget->currentIndex());
            }
            event->accept();
            return;
        }
    }
    if (event->key() == Qt::Key_F4) {
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
    } else if (event->key() == Qt::Key_F8) {
        resetForm();
        event->accept();
        return;
    } else if (event->key() == Qt::Key_F7 || (event->modifiers() & Qt::AltModifier && event->key() == Qt::Key_D)) {
        openTcsCalculationDialog();
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
    } else if (event->modifiers() & Qt::ControlModifier && event->key() == Qt::Key_P) {
        printInvoice();
        event->accept();
        return;
    } else if (event->modifiers() & Qt::ControlModifier && event->key() == Qt::Key_E) {
        exportPdf();
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

bool SalesVoucherWidget::eventFilter(QObject* watched, QEvent* event) {
    if (event->type() == QEvent::KeyPress) {
        auto* keyEvent = static_cast<QKeyEvent*>(event);
        int key = keyEvent->key();
        if (watched == m_saveBtn) {
            if (key == Qt::Key_Return || key == Qt::Key_Enter || key == Qt::Key_Space) {
                saveVoucher();
                return true;
            } else if (key == Qt::Key_Left || key == Qt::Key_Up) {
                if (m_grandTotalEdit && m_grandTotalEdit->isVisible()) {
                    m_grandTotalEdit->setFocus();
                    m_grandTotalEdit->selectAll();
                } else if (m_freightChargesEdit && m_freightChargesEdit->isVisible()) {
                    m_freightChargesEdit->setFocus();
                    m_freightChargesEdit->selectAll();
                } else if (m_narrationEdit) {
                    m_narrationEdit->setFocus();
                    m_narrationEdit->selectAll();
                }
                return true;
            }
        }
    }
    return QWidget::eventFilter(watched, event);
}

void SalesVoucherWidget::syncModelFromTable() {}
void SalesVoucherWidget::syncTableFromModel() {}
void SalesVoucherWidget::setStatusMessage(const QString& message, bool isError) {}
