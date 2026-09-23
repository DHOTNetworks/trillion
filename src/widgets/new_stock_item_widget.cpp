#include "new_stock_item_widget.h"
#include "custom_dialogs.h"
#include "kbd_badge_button.h"
#include "../database_manager.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QShortcut>
#include <QInputDialog>
#include <QListView>
#include <QCompleter>

namespace MahadevERP {

static QString comboStyle() {
    return "QComboBox {"
           "  background-color: #FFFFFF;"
           "  color: #0F172A;"
           "  border: 1.5px solid #CBD5E1;"
           "  border-radius: 4px;"
           "  padding: 4px 8px;"
           "  font-size: 12px;"
           "  font-weight: 600;"
           "} "
           "QComboBox:focus {"
           "  border: 2px solid #2563EB;"
           "  background-color: #F8FAFC;"
           "} "
           "QComboBox::drop-down {"
           "  subcontrol-origin: padding;"
           "  subcontrol-position: top right;"
           "  width: 20px;"
           "  border-left: 1px solid #CBD5E1;"
           "  background-color: #F1F5F9;"
           "  border-top-right-radius: 3px;"
           "  border-bottom-right-radius: 3px;"
           "} "
           "QComboBox::down-arrow {"
           "  width: 8px;"
           "  height: 8px;"
           "}";
}

static QString comboPopupStyle() {
    return "QListView, QAbstractItemView {"
           "  background-color: #FFFFFF;"
           "  color: #0F172A;"
           "  border: 1px solid #CBD5E1;"
           "  border-radius: 4px;"
           "  padding: 2px;"
           "  outline: none;"
           "  font-size: 11px;"
           "  font-weight: 600;"
           "} "
           "QListView::item, QAbstractItemView::item {"
           "  min-height: 22px;"
           "  padding: 4px 8px;"
           "  border-radius: 3px;"
           "  color: #0F172A;"
           "} "
           "QListView::item:hover, QAbstractItemView::item:hover {"
           "  background-color: #EFF6FF;"
           "  color: #1D4ED8;"
           "} "
           "QListView::item:selected, QAbstractItemView::item:selected {"
           "  background-color: #2563EB;"
           "  color: #FFFFFF;"
           "}";
}

NewStockItemWidget::NewStockItemWidget(StockMasterController* controller, StockItemsModel* stockModel, QWidget* parent)
    : QWidget(parent)
    , m_controller(controller)
    , m_stockModel(stockModel)
{
    setupUi();
    refreshStockGroups();
    refreshUnits();
    refreshLedgers();
    resetForm();
}

void NewStockItemWidget::setupSearchableCombo(QComboBox* combo, const QStringList& items) {
    if (!combo) return;
    combo->setEditable(true);
    combo->setInsertPolicy(QComboBox::NoInsert);
    combo->setStyleSheet(comboStyle());
    combo->clear();
    combo->addItems(items);

    if (combo->view()) {
        combo->view()->setStyleSheet(comboPopupStyle());
    }

    auto* completer = new QCompleter(items, combo);
    completer->setCaseSensitivity(Qt::CaseInsensitive);
    completer->setFilterMode(Qt::MatchContains);
    completer->setCompletionMode(QCompleter::PopupCompletion);
    if (completer->popup()) {
        completer->popup()->setStyleSheet(comboPopupStyle());
    }
    combo->setCompleter(completer);

    if (combo->lineEdit()) {
        combo->lineEdit()->setStyleSheet("QLineEdit { background: transparent; border: none; padding: 0px 2px; font-size: 12px; font-weight: 600; color: #0F172A; }");
    }
}

void NewStockItemWidget::setComboText(QComboBox* combo, const QString& text) {
    if (!combo) return;
    int idx = combo->findText(text.trimmed(), Qt::MatchFixedString);
    if (idx >= 0) {
        combo->setCurrentIndex(idx);
    } else {
        combo->setEditText(text.trimmed());
    }
}

void NewStockItemWidget::setupUi() {
    auto* rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(16, 12, 16, 12);
    rootLayout->setSpacing(8);

    // 1. Top Header Bar
    auto* headerLayout = new QHBoxLayout();
    auto* titleCol = new QVBoxLayout();
    auto* titleLabel = new QLabel("STOCK ITEM CREATION (Single Slate Master)", this);
    titleLabel->setStyleSheet("font-size: 16px; font-weight: bold; color: #0F172A;");
    auto* subLabel = new QLabel("Mandi Type, Market Type & Both Classification • Dual Accounting & Labour Matrix", this);
    subLabel->setStyleSheet("font-size: 11px; color: #64748B;");
    titleCol->addWidget(titleLabel);
    titleCol->addWidget(subLabel);
    headerLayout->addLayout(titleCol);
    headerLayout->addStretch();

    headerLayout->addWidget(new QLabel("Goods Type:", this));
    m_goodsTypeCombo = new QComboBox(this);
    m_goodsTypeCombo->addItems({"Goods", "Services", "Capital Goods"});
    m_goodsTypeCombo->setStyleSheet(comboStyle());
    if (m_goodsTypeCombo->view()) {
        m_goodsTypeCombo->view()->setStyleSheet(comboPopupStyle());
    }
    headerLayout->addWidget(m_goodsTypeCombo);

    headerLayout->addWidget(new QLabel("Classification:", this));
    m_itemTypeCombo = new QComboBox(this);
    m_itemTypeCombo->addItems({"Both (Mandi & Market)", "Mandi Type", "Market Type"});
    m_itemTypeCombo->setStyleSheet(comboStyle());
    if (m_itemTypeCombo->view()) {
        m_itemTypeCombo->view()->setStyleSheet(comboPopupStyle());
    }
    headerLayout->addWidget(m_itemTypeCombo);

    auto* backBtn = new KbdBadgeButton("← Back", "Esc", this);
    backBtn->setPrimaryColor("#F1F5F9", "#E2E8F0");
    backBtn->setTextColor("#475569");
    connect(backBtn, &QPushButton::clicked, this, &NewStockItemWidget::backRequested);
    headerLayout->addWidget(backBtn);
    rootLayout->addLayout(headerLayout);

    // Separator
    auto* sep = new QFrame(this);
    sep->setFrameShape(QFrame::HLine);
    sep->setStyleSheet("color: #E2E8F0;");
    rootLayout->addWidget(sep);

    // 2. Main 3-Column Horizontal Form
    auto* columnsLayout = new QHBoxLayout();
    columnsLayout->setSpacing(12);

    const QString cardStyle =
        "QFrame.CardFrame { background-color: #FFFFFF; border: 1px solid #CBD5E1; border-radius: 8px; }"
        "QLabel.CardHeader { font-size: 12px; font-weight: bold; color: #1E40AF; background-color: #F8FAFC; border-bottom: 1px solid #E2E8F0; border-top-left-radius: 7px; border-top-right-radius: 7px; padding: 6px 10px; }"
        "QLabel { color: #334155; font-size: 11px; font-weight: 600; }"
        "QLineEdit { background-color: #FFFFFF; color: #0F172A; border: 1.5px solid #CBD5E1; border-radius: 4px; padding: 4px 8px; font-size: 12px; font-weight: 600; }"
        "QLineEdit:focus { border: 2px solid #2563EB; background-color: #F8FAFC; }"
        "QCheckBox { font-size: 11px; color: #1E293B; font-weight: 600; spacing: 6px; min-height: 20px; }"
        "QCheckBox::indicator { width: 15px; height: 15px; border: 1.5px solid #94A3B8; border-radius: 3px; background-color: #FFFFFF; }"
        "QCheckBox::indicator:hover { border-color: #2563EB; }"
        "QCheckBox::indicator:checked { background-color: #2563EB; border-color: #1D4ED8; }";

    // --- Column 1: Identity & Classification ---
    auto* col1Frame = new QFrame(this);
    col1Frame->setProperty("class", "CardFrame");
    col1Frame->setStyleSheet(cardStyle);
    auto* col1Layout = new QVBoxLayout(col1Frame);
    col1Layout->setContentsMargins(0, 0, 0, 8);
    col1Layout->setSpacing(4);

    auto* col1Header = new QLabel("1. IDENTITY & CLASSIFICATION", col1Frame);
    col1Header->setProperty("class", "CardHeader");
    col1Layout->addWidget(col1Header);

    auto* col1Grid = new QGridLayout();
    col1Grid->setContentsMargins(10, 6, 10, 6);
    col1Grid->setSpacing(6);

    m_nameEdit = new QLineEdit(col1Frame);
    m_nameEdit->setPlaceholderText("e.g. Paddy 1121 Raw / Rice 1509");
    col1Grid->addWidget(new QLabel("Stock Item Name *:"), 0, 0);
    col1Grid->addWidget(m_nameEdit, 0, 1);

    m_codeEdit = new QLineEdit(col1Frame);
    m_codeEdit->setPlaceholderText("Item Code / SKU");
    col1Grid->addWidget(new QLabel("Item Code:"), 1, 0);
    col1Grid->addWidget(m_codeEdit, 1, 1);

    // Stock / Trading Group Row with + Group button
    auto* groupRowLayout = new QHBoxLayout();
    groupRowLayout->setContentsMargins(0, 0, 0, 0);
    groupRowLayout->setSpacing(4);
    m_stockGroupCombo = new QComboBox(col1Frame);
    m_stockGroupCombo->setStyleSheet(comboStyle());
    m_createGroupBtn = new QPushButton("+ Group", col1Frame);
    m_createGroupBtn->setStyleSheet("QPushButton { background-color: #EFF6FF; color: #1D4ED8; border: 1px solid #BFDBFE; border-radius: 4px; padding: 4px 6px; font-weight: bold; font-size: 10px; } QPushButton:hover { background-color: #DBEAFE; }");
    connect(m_createGroupBtn, &QPushButton::clicked, this, &NewStockItemWidget::onCreateStockGroup);
    groupRowLayout->addWidget(m_stockGroupCombo, 1);
    groupRowLayout->addWidget(m_createGroupBtn);
    col1Grid->addWidget(new QLabel("Trading Group *:"), 2, 0);
    col1Grid->addLayout(groupRowLayout, 2, 1);

    m_unitCombo = new QComboBox(col1Frame);
    m_unitCombo->setStyleSheet(comboStyle());
    col1Grid->addWidget(new QLabel("Base Unit *:"), 3, 0);
    col1Grid->addWidget(m_unitCombo, 3, 1);

    m_companyEdit = new QLineEdit(col1Frame);
    m_companyEdit->setPlaceholderText("Manufacturer / Mill Brand");
    col1Grid->addWidget(new QLabel("Company / Brand:"), 4, 0);
    col1Grid->addWidget(m_companyEdit, 4, 1);

    m_packingKgEdit = new QLineEdit("50.0", col1Frame);
    connect(m_packingKgEdit, &QLineEdit::textChanged, this, &NewStockItemWidget::onRecalculateOpening);
    col1Grid->addWidget(new QLabel("Packing Weight (Kg):"), 5, 0);
    col1Grid->addWidget(m_packingKgEdit, 5, 1);

    col1Layout->addLayout(col1Grid);

    // Checkboxes 2-column grid to ensure perfect alignment
    auto* flagsBox = new QFrame(col1Frame);
    flagsBox->setStyleSheet("QFrame { background-color: #F8FAFC; border: 1px solid #E2E8F0; border-radius: 6px; margin: 4px 8px; padding: 6px; }");
    auto* flagsGrid = new QGridLayout(flagsBox);
    flagsGrid->setContentsMargins(6, 4, 6, 4);
    flagsGrid->setHorizontalSpacing(14);
    flagsGrid->setVerticalSpacing(6);

    m_autoAdjustNameCheck = new QCheckBox("Auto Adjust Name", flagsBox);
    m_autoAdjustNameCheck->setChecked(true);
    flagsGrid->addWidget(m_autoAdjustNameCheck, 0, 0, 1, 2);

    m_stockCalculateCheck = new QCheckBox("Stock Calculate", flagsBox);
    m_stockCalculateCheck->setChecked(true);
    flagsGrid->addWidget(m_stockCalculateCheck, 1, 0);

    m_calcInTradingCheck = new QCheckBox("Calculate in Trading A/c", flagsBox);
    m_calcInTradingCheck->setChecked(true);
    flagsGrid->addWidget(m_calcInTradingCheck, 1, 1);

    m_capitalGoodsCheck = new QCheckBox("Capital Goods", flagsBox);
    flagsGrid->addWidget(m_capitalGoodsCheck, 2, 0);

    m_taxOnQtyCheck = new QCheckBox("Tax on Qty", flagsBox);
    flagsGrid->addWidget(m_taxOnQtyCheck, 2, 1);

    col1Layout->addWidget(flagsBox);
    col1Layout->addStretch();
    columnsLayout->addWidget(col1Frame, 34);

    // --- Column 2: Pricing & Tax Matrix ---
    auto* col2Frame = new QFrame(this);
    col2Frame->setProperty("class", "CardFrame");
    col2Frame->setStyleSheet(cardStyle);
    auto* col2Layout = new QVBoxLayout(col2Frame);
    col2Layout->setContentsMargins(0, 0, 0, 8);
    col2Layout->setSpacing(4);

    auto* col2Header = new QLabel("2. PRICING & GST TAX MATRIX", col2Frame);
    col2Header->setProperty("class", "CardHeader");
    col2Layout->addWidget(col2Header);

    auto* col2Grid = new QGridLayout();
    col2Grid->setContentsMargins(10, 6, 10, 6);
    col2Grid->setSpacing(7);

    m_purchaseRateEdit = new QLineEdit("0.00", col2Frame);
    col2Grid->addWidget(new QLabel("Purchase Rate (₹):"), 0, 0);
    col2Grid->addWidget(m_purchaseRateEdit, 0, 1);

    m_saleRateEdit = new QLineEdit("0.00", col2Frame);
    col2Grid->addWidget(new QLabel("Sale Rate (₹):"), 1, 0);
    col2Grid->addWidget(m_saleRateEdit, 1, 1);

    m_mrpEdit = new QLineEdit("0.00", col2Frame);
    col2Grid->addWidget(new QLabel("MRP (₹):"), 2, 0);
    col2Grid->addWidget(m_mrpEdit, 2, 1);

    m_discountEdit = new QLineEdit("0.00", col2Frame);
    col2Grid->addWidget(new QLabel("Trade Discount %:"), 3, 0);
    col2Grid->addWidget(m_discountEdit, 3, 1);

    m_hsnEdit = new QLineEdit("1006", col2Frame);
    col2Grid->addWidget(new QLabel("HSN / SAC Code:"), 4, 0);
    col2Grid->addWidget(m_hsnEdit, 4, 1);

    m_gstRateCombo = new QComboBox(col2Frame);
    m_gstRateCombo->addItems({"5.0 % (GST 5%)", "0.0 % (Exempt/Nil)", "12.0 % (GST 12%)", "18.0 % (GST 18%)", "28.0 % (GST 28%)"});
    m_gstRateCombo->setStyleSheet(comboStyle());
    if (m_gstRateCombo->view()) {
        m_gstRateCombo->view()->setStyleSheet(comboPopupStyle());
    }
    col2Grid->addWidget(new QLabel("GST Rate %:"), 5, 0);
    col2Grid->addWidget(m_gstRateCombo, 5, 1);

    m_cessRateEdit = new QLineEdit("0.00", col2Frame);
    col2Grid->addWidget(new QLabel("Cess Rate %:"), 6, 0);
    col2Grid->addWidget(m_cessRateEdit, 6, 1);

    col2Layout->addLayout(col2Grid);
    col2Layout->addStretch();
    columnsLayout->addWidget(col2Frame, 32);

    // --- Column 3: Opening Stock & Database Ledgers ---
    auto* col3Frame = new QFrame(this);
    col3Frame->setProperty("class", "CardFrame");
    col3Frame->setStyleSheet(cardStyle);
    auto* col3Layout = new QVBoxLayout(col3Frame);
    col3Layout->setContentsMargins(0, 0, 0, 8);
    col3Layout->setSpacing(4);

    auto* col3Header = new QLabel("3. OPENING STOCK & LEDGERS (DATABASE)", col3Frame);
    col3Header->setProperty("class", "CardHeader");
    col3Layout->addWidget(col3Header);

    auto* col3Grid = new QGridLayout();
    col3Grid->setContentsMargins(10, 6, 10, 6);
    col3Grid->setSpacing(5);

    m_openingBagsEdit = new QLineEdit("0", col3Frame);
    connect(m_openingBagsEdit, &QLineEdit::textChanged, this, &NewStockItemWidget::onRecalculateOpening);
    col3Grid->addWidget(new QLabel("Opening Bags:"), 0, 0);
    col3Grid->addWidget(m_openingBagsEdit, 0, 1);

    m_openingQtyEdit = new QLineEdit("0.00", col3Frame);
    connect(m_openingQtyEdit, &QLineEdit::textChanged, this, &NewStockItemWidget::onRecalculateOpening);
    col3Grid->addWidget(new QLabel("Opening Qty (Qtl):"), 1, 0);
    col3Grid->addWidget(m_openingQtyEdit, 1, 1);

    m_openingRateEdit = new QLineEdit("0.00", col3Frame);
    connect(m_openingRateEdit, &QLineEdit::textChanged, this, &NewStockItemWidget::onRecalculateOpening);
    col3Grid->addWidget(new QLabel("Opening Rate (₹):"), 2, 0);
    col3Grid->addWidget(m_openingRateEdit, 2, 1);

    m_openingValueLabel = new QLabel("₹ 0.00", col3Frame);
    m_openingValueLabel->setStyleSheet("font-size: 13px; font-weight: bold; color: #16A34A; background-color: #DCFCE7; padding: 4px 8px; border-radius: 4px; border: 1px solid #BBF7D0;");
    col3Grid->addWidget(new QLabel("Opening Value:"), 3, 0);
    col3Grid->addWidget(m_openingValueLabel, 3, 1);

    // Database Ledgers mapping using AccountSearchBox
    m_purchaseLedgerBox = new AccountSearchBox(col3Frame);
    col3Grid->addWidget(new QLabel("Purchase A/c *:"), 4, 0);
    col3Grid->addWidget(m_purchaseLedgerBox, 4, 1);

    m_purcReturnLedgerBox = new AccountSearchBox(col3Frame);
    col3Grid->addWidget(new QLabel("Purc. Return A/c:"), 5, 0);
    col3Grid->addWidget(m_purcReturnLedgerBox, 5, 1);

    m_saleLedgerBox = new AccountSearchBox(col3Frame);
    col3Grid->addWidget(new QLabel("Sale A/c *:"), 6, 0);
    col3Grid->addWidget(m_saleLedgerBox, 6, 1);

    m_saleReturnLedgerBox = new AccountSearchBox(col3Frame);
    col3Grid->addWidget(new QLabel("Sale Return A/c:"), 7, 0);
    col3Grid->addWidget(m_saleReturnLedgerBox, 7, 1);

    m_stockLedgerBox = new AccountSearchBox(col3Frame);
    col3Grid->addWidget(new QLabel("Stock A/c (Self):"), 8, 0);
    col3Grid->addWidget(m_stockLedgerBox, 8, 1);

    col3Layout->addLayout(col3Grid);
    col3Layout->addStretch();
    columnsLayout->addWidget(col3Frame, 34);

    rootLayout->addLayout(columnsLayout, 1);

    // 3. Bottom Action Bar
    auto* bottomLayout = new QHBoxLayout();
    auto* cancelBottomBtn = new KbdBadgeButton("Cancel", "Esc", this);
    cancelBottomBtn->setPrimaryColor("#F1F5F9", "#E2E8F0");
    cancelBottomBtn->setTextColor("#475569");
    connect(cancelBottomBtn, &QPushButton::clicked, this, &NewStockItemWidget::backRequested);
    bottomLayout->addWidget(cancelBottomBtn);

    auto* copyLedgerBtn = new KbdBadgeButton("Copy All Ledgers From Purchase A/c", "F5", this);
    copyLedgerBtn->setPrimaryColor("#EDE9FE", "#DDD6FE");
    copyLedgerBtn->setTextColor("#6D28D9");
    connect(copyLedgerBtn, &QPushButton::clicked, this, &NewStockItemWidget::onCopyAllLedgersFromPurchase);
    bottomLayout->addWidget(copyLedgerBtn);

    auto* addGrpBtn = new KbdBadgeButton("New Stock Group", "Alt+C", this);
    addGrpBtn->setPrimaryColor("#EFF6FF", "#DBEAFE");
    addGrpBtn->setTextColor("#1D4ED8");
    connect(addGrpBtn, &QPushButton::clicked, this, &NewStockItemWidget::onCreateStockGroup);
    bottomLayout->addWidget(addGrpBtn);

    bottomLayout->addStretch();

    auto* saveBtn = new KbdBadgeButton("Save Stock Item", "Ctrl+S / F2", this);
    saveBtn->setPrimaryColor("#16A34A", "#15803D");
    saveBtn->setTextColor("#FFFFFF");
    saveBtn->setMinimumWidth(220);
    connect(saveBtn, &QPushButton::clicked, this, &NewStockItemWidget::onSaveClicked);
    bottomLayout->addWidget(saveBtn);
    rootLayout->addLayout(bottomLayout);

    connect(new QShortcut(QKeySequence(Qt::Key_Escape), this), &QShortcut::activated, this, &NewStockItemWidget::backRequested);
    connect(new QShortcut(QKeySequence(Qt::Key_F2), this), &QShortcut::activated, this, &NewStockItemWidget::onSaveClicked);
    connect(new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_S), this), &QShortcut::activated, this, &NewStockItemWidget::onSaveClicked);
    connect(new QShortcut(QKeySequence(Qt::Key_F5), this), &QShortcut::activated, this, &NewStockItemWidget::onCopyAllLedgersFromPurchase);
    connect(new QShortcut(QKeySequence(Qt::ALT | Qt::Key_C), this), &QShortcut::activated, this, &NewStockItemWidget::onCreateStockGroup);
}

void NewStockItemWidget::refreshStockGroups() {
    QStringList groups;
    if (m_stockModel) {
        groups = m_stockModel->get_stock_groups();
    }
    setupSearchableCombo(m_stockGroupCombo, groups);
}

void NewStockItemWidget::refreshUnits() {
    QStringList units;
    if (m_stockModel) {
        units = m_stockModel->get_units();
    }
    setupSearchableCombo(m_unitCombo, units);
}

void NewStockItemWidget::refreshLedgers() {
    // AccountSearchBox queries dynamically from parties table as user types
}

void NewStockItemWidget::onCreateStockGroup() {
    bool ok = false;
    QString groupName = QInputDialog::getText(
        this, "Create New Stock / Trading Group",
        "Enter Stock Group Name (e.g. Paddy 1509, Rice Bran):",
        QLineEdit::Normal, "", &ok
    );
    if (ok && !groupName.trimmed().isEmpty()) {
        QString g = groupName.trimmed();
        if (m_stockModel) {
            m_stockModel->add_stock_group(g);
        }
        refreshStockGroups();
        setComboText(m_stockGroupCombo, g);
    }
}

void NewStockItemWidget::resetForm() {
    m_nameEdit->clear();
    m_codeEdit->clear();
    m_companyEdit->clear();
    if (m_stockGroupCombo->count() > 0) m_stockGroupCombo->setCurrentIndex(0);
    if (m_unitCombo->count() > 0) m_unitCombo->setCurrentIndex(0);
    m_goodsTypeCombo->setCurrentIndex(0);
    m_itemTypeCombo->setCurrentIndex(0);
    m_purchaseRateEdit->setText("0.00");
    m_saleRateEdit->setText("0.00");
    m_mrpEdit->setText("0.00");
    m_discountEdit->setText("0.00");
    m_hsnEdit->setText("1006");
    m_gstRateCombo->setCurrentIndex(0);
    m_cessRateEdit->setText("0.00");
    m_packingKgEdit->setText("50.0");
    m_openingBagsEdit->setText("0");
    m_openingQtyEdit->setText("0.00");
    m_openingRateEdit->setText("0.00");

    m_purchaseLedgerBox->clearParty();
    m_purcReturnLedgerBox->clearParty();
    m_saleLedgerBox->clearParty();
    m_saleReturnLedgerBox->clearParty();
    m_stockLedgerBox->clearParty();

    m_autoAdjustNameCheck->setChecked(true);
    m_stockCalculateCheck->setChecked(true);
    m_calcInTradingCheck->setChecked(true);
    m_capitalGoodsCheck->setChecked(false);
    m_taxOnQtyCheck->setChecked(false);

    onRecalculateOpening();
}

void NewStockItemWidget::focusFirstField() {
    m_nameEdit->setFocus();
}

void NewStockItemWidget::onRecalculateOpening() {
    int bags = m_openingBagsEdit->text().toInt();
    double packKg = m_packingKgEdit->text().toDouble();
    double qty = m_openingQtyEdit->text().toDouble();

    if (bags > 0 && packKg > 0 && qty == 0.0) {
        qty = (bags * packKg) / 100.0;
        m_openingQtyEdit->blockSignals(true);
        m_openingQtyEdit->setText(QString::number(qty, 'f', 2));
        m_openingQtyEdit->blockSignals(false);
    }

    double rate = m_openingRateEdit->text().toDouble();
    double val = qty * rate;
    m_openingValueLabel->setText("₹ " + QString::number(val, 'f', 2));
}

void NewStockItemWidget::onCopyAllLedgersFromPurchase() {
    QString pLedger = m_purchaseLedgerBox->currentPartyName();
    int pId = m_purchaseLedgerBox->currentPartyId();
    if (!pLedger.isEmpty()) {
        m_purcReturnLedgerBox->setParty(pLedger, pId);
        m_saleLedgerBox->setParty(pLedger, pId);
        m_saleReturnLedgerBox->setParty(pLedger, pId);
        m_stockLedgerBox->setParty(pLedger, pId);
    }
}

void NewStockItemWidget::onSaveClicked() {
    QString name = m_nameEdit->text().trimmed();
    if (name.isEmpty()) {
        CustomMessageBox::showWarning(this, "Validation Error", "Please enter a Stock Item Name.");
        m_nameEdit->setFocus();
        return;
    }

    QString group = m_stockGroupCombo->currentText().trimmed();
    if (group.isEmpty()) {
        CustomMessageBox::showWarning(this, "Validation Error", "Please select or create a Trading / Stock Group.");
        m_stockGroupCombo->setFocus();
        return;
    }

    if (!CustomMessageBox::showConfirmation(this, "Confirm Save", "Are you sure you want to create Stock Item '" + name + "'?")) {
        return;
    }

    QVariantMap data;
    data["name"] = name;
    data["code"] = m_codeEdit->text().trimmed();
    data["item_type"] = m_itemTypeCombo->currentText().contains("Mandi") ? "Mandi Type" : (m_itemTypeCombo->currentText().contains("Market") ? "Market Type" : "Both");
    data["goods_type"] = m_goodsTypeCombo->currentText();
    data["company_name"] = m_companyEdit->text();
    data["trading_group"] = group;
    data["stock_group"] = group;
    data["unit"] = m_unitCombo->currentText().trimmed();

    data["purchase_rate"] = m_purchaseRateEdit->text().toDouble();
    data["sale_rate"] = m_saleRateEdit->text().toDouble();
    data["mrp"] = m_mrpEdit->text().toDouble();
    data["discount"] = m_discountEdit->text().toDouble();
    data["hsn_code"] = m_hsnEdit->text().trimmed();

    double gst = 5.0;
    if (m_gstRateCombo->currentIndex() == 1) gst = 0.0;
    else if (m_gstRateCombo->currentIndex() == 2) gst = 12.0;
    else if (m_gstRateCombo->currentIndex() == 3) gst = 18.0;
    else if (m_gstRateCombo->currentIndex() == 4) gst = 28.0;
    data["gst_rate"] = gst;

    data["cess_rate"] = m_cessRateEdit->text().toDouble();
    data["packing_kg"] = m_packingKgEdit->text().toDouble();
    data["opening_bags"] = m_openingBagsEdit->text().toInt();
    data["opening_qty"] = m_openingQtyEdit->text().toDouble();
    data["opening_rate"] = m_openingRateEdit->text().toDouble();
    data["opening_value"] = m_openingQtyEdit->text().toDouble() * m_openingRateEdit->text().toDouble();

    data["purchase_ledger"] = m_purchaseLedgerBox->currentPartyName();
    data["purchase_ledger_id"] = m_purchaseLedgerBox->currentPartyId();

    data["purchase_return_ledger"] = m_purcReturnLedgerBox->currentPartyName();
    data["purchase_return_ledger_id"] = m_purcReturnLedgerBox->currentPartyId();

    data["sale_ledger"] = m_saleLedgerBox->currentPartyName();
    data["sale_ledger_id"] = m_saleLedgerBox->currentPartyId();

    data["sale_return_ledger"] = m_saleReturnLedgerBox->currentPartyName();
    data["sale_return_ledger_id"] = m_saleReturnLedgerBox->currentPartyId();

    data["stock_ledger"] = m_stockLedgerBox->currentPartyName();
    data["stock_ledger_id"] = m_stockLedgerBox->currentPartyId();

    data["auto_adjust_name"] = m_autoAdjustNameCheck->isChecked() ? 1 : 0;
    data["capital_goods"] = m_capitalGoodsCheck->isChecked() ? 1 : 0;
    data["tax_on_qty"] = m_taxOnQtyCheck->isChecked() ? 1 : 0;

    bool ok = false;
    if (m_stockModel) {
        ok = m_stockModel->save_stock_item_full(data);
    } else {
        ok = true;
    }

    if (ok) {
        CustomMessageBox::showInformation(this, "Success", "Stock Item '" + name + "' created successfully.");
        if (m_stockModel) m_stockModel->reload_data();
        resetForm();
        emit savedSuccess();
    } else {
        CustomMessageBox::showCritical(this, "Save Failed", "Failed to save Stock Item in database.");
    }
}

} // namespace MahadevERP
