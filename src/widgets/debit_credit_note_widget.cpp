#include "debit_credit_note_widget.h"
#include "voucher_common.h"
#include "voucher_date_dialog.h"
#include "kbd_badge_button.h"
#include "erp_theme.h"
#include "custom_dialogs.h"
#include "../engine/fiscal_year_helper.h"
#include "../engine/stock_pipeline.h"
#include "../database_manager.h"
#include "../models/parties_model.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QHeaderView>
#include <QFrame>
#include <QCompleter>
#include <QApplication>
#include <QDate>
#include <QKeyEvent>
#include <cmath>

namespace MahadevERP {

// Non-editable combo text is painted from the widget palette on some
// styles (macOS dark mode), ignoring stylesheet color — so white-on-white
// happens no matter what the stylesheet says. Belt and suspenders: force
// the palette roles AND an explicit focus policy on every header combo.
static void hardenComboText(QComboBox* cb, const QColor& textColor, bool lockTyping = true) {
    if (!cb) return;
    QPalette pal = cb->palette();
    pal.setColor(QPalette::WindowText, textColor);
    pal.setColor(QPalette::ButtonText, textColor);
    pal.setColor(QPalette::Text, textColor);
    pal.setColor(QPalette::BrightText, textColor);
    cb->setPalette(pal);
    cb->setFocusPolicy(Qt::StrongFocus);
    // Display-locked editable: the value paints through a real line edit
    // (fully styleable on every platform theme) while staying untypable —
    // the same rendering path as the invoice combo, which reads fine.
    // The dropdown, Enter/Backspace/arrow contract is unchanged.
    // (The invoice combo keeps typing: it resolves typed numbers.)
    if (!lockTyping) return;
    cb->setEditable(true);
    if (QLineEdit* le = cb->lineEdit()) {
        le->setReadOnly(true);
        le->setFocusPolicy(Qt::StrongFocus);
        QPalette lp = le->palette();
        lp.setColor(QPalette::Text, textColor);
        le->setPalette(lp);
    }
}

DebitCreditNoteWidget::DebitCreditNoteWidget(DebitCreditNoteController* controller,
                                             PrintExportController* printExportCtrl,
                                             QWidget* parent)
    : QWidget(parent)
    , m_controller(controller)
    , m_printExportCtrl(printExportCtrl)
{
    setupUi();
    setupConnections();
    resetForm();
}

void DebitCreditNoteWidget::setupUi() {
    setAttribute(Qt::WA_StyledBackground, true);    setStyleSheet(
        "DebitCreditNoteWidget { background-color: #F8FAFC; font-family: 'Segoe UI', -apple-system, BlinkMacSystemFont, Roboto, sans-serif; }"
        "QLabel { border: none; background: transparent; color: #1E293B; font-size: 11px; font-weight: 700; }"
        "QLineEdit { background-color: #FFFFFF; color: #0F172A; border: 1px solid #CBD5E1; border-radius: 5px; padding: 2px 6px; font-size: 11.5px; font-weight: 700; }"
        "QLineEdit:focus { border: 1.5px solid #0284C7; background-color: #F0F9FF; color: #0F172A; }"
    );

    auto* rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(10, 6, 10, 6);
    rootLayout->setSpacing(6);

    // ========================================================================
    // 1. TOP HEADER BAR (Sale-voucher mirror): badge + No. + title + date/day
    // ========================================================================
    QHBoxLayout* topHeaderLayout = new QHBoxLayout();
    topHeaderLayout->setContentsMargins(0, 0, 0, 0);
    topHeaderLayout->setSpacing(6);

    m_noteBadge = new QLabel("Credit Note", this);
    m_noteBadge->setAlignment(Qt::AlignCenter);
    m_noteBadge->setMinimumHeight(22);
    m_noteBadge->setStyleSheet(ErpTheme::badgeStyle("info"));
    topHeaderLayout->addWidget(m_noteBadge);

    m_voucherNoDisplay = new QLabel("No. CN-0001", this);
    m_voucherNoDisplay->setStyleSheet(ErpTheme::headerTitleStyle(13));
    topHeaderLayout->addWidget(m_voucherNoDisplay);

    topHeaderLayout->addStretch(1);

    auto* titleLabel = new QLabel("F9 : Debit & Credit Note", this);
    titleLabel->setAlignment(Qt::AlignCenter);
    titleLabel->setStyleSheet(ErpTheme::headerTitleStyle(15));
    topHeaderLayout->addWidget(titleLabel);

    topHeaderLayout->addStretch(1);

    // Date & Day of Week (stacked, right-aligned — exactly like Sale).
    QVBoxLayout* dateBox = new QVBoxLayout();
    dateBox->setSpacing(0);
    dateBox->setAlignment(Qt::AlignRight);

    auto* dateRow = new QHBoxLayout();
    dateRow->setSpacing(4);
    dateRow->setContentsMargins(0, 0, 0, 0);
    m_noteDateEdit = new AccountingDateEdit(this);
    m_noteDateEdit->setMinimumHeight(22);
    m_noteDateEdit->setMinimumWidth(105);
    dateRow->addWidget(m_noteDateEdit);

    auto* dateBtn = new QPushButton("…", this);
    dateBtn->setFixedSize(26, 26);
    dateBtn->setCursor(Qt::PointingHandCursor);
    dateBtn->setToolTip("Pick date (F2)");
    dateBtn->setStyleSheet("background-color: #EFF6FF; border: 1px solid #93C5FD; border-radius: 4px; font-weight: bold; color: #1D4ED8;");
    connect(dateBtn, &QPushButton::clicked, this, [this]() { openDateDialog(false); });
    dateRow->addWidget(dateBtn);
    dateBox->addLayout(dateRow);

    m_dayLabel = new QLabel("", this);
    m_dayLabel->setAlignment(Qt::AlignRight);
    m_dayLabel->setStyleSheet(QString("font-size: 10.5px; font-weight: 700; color: %1; border: none; background: transparent;").arg(ErpTheme::kSky.name()));
    dateBox->addWidget(m_dayLabel);
    topHeaderLayout->addLayout(dateBox);

    rootLayout->addLayout(topHeaderLayout);

    // Sub-header shortcut / help line + FY badge (sale mirror).
    QHBoxLayout* subBarLayout = new QHBoxLayout();
    subBarLayout->setContentsMargins(0, 0, 0, 0);
    subBarLayout->setSpacing(16);

    auto* helpHintLabel = new QLabel("(Change Date - F2 : Alter Note - F4 : Prev/Next - PgUp/PgDn : New Note - F9 : Print - Ctrl+P : PDF - Ctrl+E)", this);
    helpHintLabel->setStyleSheet(ErpTheme::mutedLabelStyle(10));
    subBarLayout->addWidget(helpHintLabel);

    subBarLayout->addStretch(1);

    m_fyBadge = new QLabel(FiscalYearHelper::getActiveFiscalYear().name, this);
    m_fyBadge->setStyleSheet("background-color: #F1F5F9; color: #475569; border: 1px solid #CBD5E1; border-radius: 4px; padding: 2px 6px; font-size: 10.5px; font-weight: 800;");
    subBarLayout->addWidget(m_fyBadge);

    rootLayout->addLayout(subBarLayout);

    // ========================================================================
    // 2. PARTY ROW + SUB-ROW — entry starts here: party first, the
    //    invoice-link fields below all react to it.
    // ========================================================================
    QHBoxLayout* partyRow = new QHBoxLayout();
    partyRow->setSpacing(6);

    auto* partyTagLabel = new QLabel("Party / Account *:", this);
    partyTagLabel->setStyleSheet(QString("color: %1; font-size: 12px; font-weight: 800; border: none; background: transparent;").arg(ErpTheme::kNavy.name()));
    partyRow->addWidget(partyTagLabel);

    m_partySearch = new AccountSearchBox(this);
    m_partySearch->setMinimumHeight(26);
    partyRow->addWidget(m_partySearch, 1);

    rootLayout->addLayout(partyRow);

    // Sub-row under party: Date Bal + GSTIN (display mirrors, sale pattern).
    QHBoxLayout* partySubRow = new QHBoxLayout();
    partySubRow->setSpacing(20);
    partySubRow->setContentsMargins(10, 0, 0, 0);

    m_partyBalLabel = new QLabel("Date Bal. 0.00", this);
    m_partyBalLabel->setStyleSheet(ErpTheme::mutedLabelStyle(11));
    partySubRow->addWidget(m_partyBalLabel);

    QHBoxLayout* gstinLayout = new QHBoxLayout();
    gstinLayout->setSpacing(4);
    gstinLayout->addWidget(new QLabel("GSTIN :", this));
    m_partyGstinEdit = new QLineEdit(this);
    m_partyGstinEdit->setReadOnly(true);
    // Display mirror of the party master GSTIN: mouse-only, no Tab stop.
    m_partyGstinEdit->setFocusPolicy(Qt::ClickFocus);
    m_partyGstinEdit->setMinimumHeight(20);
    m_partyGstinEdit->setMinimumWidth(160);
    m_partyGstinEdit->setStyleSheet(QString("QLineEdit { background: transparent; border: none; font-weight: 800; color: %1; }").arg(ErpTheme::kNavy.name()));
    gstinLayout->addWidget(m_partyGstinEdit);
    partySubRow->addLayout(gstinLayout);

    partySubRow->addStretch(1);

    rootLayout->addLayout(partySubRow);

    // ========================================================================
    // 3. META ROW: note type, numbers, orig-invoice link, reason, POS
    //    (single flow row below the party — all party-driven fields).
    QHBoxLayout* metaRow = new QHBoxLayout();
    metaRow->setContentsMargins(0, 0, 0, 0);
    metaRow->setSpacing(6);

    metaRow->addWidget(new QLabel("Note Type *:", this));
    m_noteTypeCombo = new QComboBox(this);
    m_noteTypeCombo->addItems({"Credit Note (Sales Return / Rate Cut)", "Debit Note (Purchase Return / Rate Difference)"});
    m_noteTypeCombo->setMinimumHeight(24);
    m_noteTypeCombo->setStyleSheet("background-color: #EFF6FF; border: 1.5px solid #93C5FD; border-radius: 6px; padding: 4px 8px; font-weight: bold; color: #1D4ED8;"
        " QComboBox:focus { border: 2px solid #1D4ED8; background-color: #FFFFF0; }"
        " QComboBox QLineEdit { background-color: #EFF6FF; color: #1D4ED8; border: none; font-weight: bold; }"
        " QComboBox QAbstractItemView { background-color: #FFFFFF; color: #0F172A; selection-background-color: #2563EB; selection-color: #FFFFFF; }");
    hardenComboText(m_noteTypeCombo, QColor("#1D4ED8"));
    metaRow->addWidget(m_noteTypeCombo);

    metaRow->addWidget(new QLabel("Note No:", this));
    m_noteNoEdit = new QLineEdit(this);
    m_noteNoEdit->setReadOnly(true);
    // Auto-generated: never a keyboard stop (click still selects).
    m_noteNoEdit->setFocusPolicy(Qt::ClickFocus);
    m_noteNoEdit->setMinimumHeight(24);
    m_noteNoEdit->setMinimumWidth(90);
    m_noteNoEdit->setStyleSheet("background-color: #F8FAFC; border: 1.5px solid #CBD5E1; border-radius: 6px; padding: 4px 8px; font-weight: 800; color: #1D4ED8; font-family: 'Consolas', monospace;");
    metaRow->addWidget(m_noteNoEdit);

    metaRow->addWidget(new QLabel("Orig Type:", this));
    m_origInvoiceTypeCombo = new QComboBox(this);
    m_origInvoiceTypeCombo->addItems({"Sale Invoice", "Purchase Bill", "Other / Manual"});
    m_origInvoiceTypeCombo->setMinimumHeight(24);
    // Explicit text color: non-editable combos otherwise paint with the
    // system palette (white-on-white in dark mode) instead of the widget
    // stylesheet. Same treatment for Reason / POS below.
    m_origInvoiceTypeCombo->setStyleSheet(
        "QComboBox { background-color: #FFFFFF; color: #0F172A; border: 1.5px solid #CBD5E1; border-radius: 6px; padding: 4px 8px; font-size: 12px; font-weight: 600; }"
        "QComboBox:focus { border: 2px solid #2563EB; background-color: #FFFFF0; }"
        "QComboBox QAbstractItemView { background-color: #FFFFFF; color: #0F172A; selection-background-color: #2563EB; selection-color: #FFFFFF; border: 1px solid #CBD5E1; outline: none; }");
    hardenComboText(m_origInvoiceTypeCombo, QColor("#0F172A"));
    metaRow->addWidget(m_origInvoiceTypeCombo);

    metaRow->addWidget(new QLabel("Orig Invoice *:", this));
    m_origInvoiceCombo = new QComboBox(this);
    m_origInvoiceCombo->setEditable(true);
    m_origInvoiceCombo->setMinimumHeight(24);
    if (m_origInvoiceCombo->lineEdit())
        m_origInvoiceCombo->lineEdit()->setPlaceholderText("Select party first, then pick invoice");
    m_origInvoiceCombo->setStyleSheet("background-color: #EFF6FF; border: 1.5px solid #93C5FD; border-radius: 6px; padding: 4px 8px; font-weight: bold; color: #1D4ED8;"
        " QComboBox:focus { border: 2px solid #1D4ED8; background-color: #FFFFF0; }"
        " QComboBox QLineEdit { background-color: #EFF6FF; color: #1D4ED8; border: none; font-weight: bold; }"
        " QComboBox QAbstractItemView { background-color: #FFFFFF; color: #0F172A; selection-background-color: #2563EB; selection-color: #FFFFFF; }");
    hardenComboText(m_origInvoiceCombo, QColor("#1D4ED8"), false);
    metaRow->addWidget(m_origInvoiceCombo, 1);

    metaRow->addWidget(new QLabel("Orig Date:", this));
    m_origInvoiceDateEdit = new QLineEdit(this);
    m_origInvoiceDateEdit->setPlaceholderText("auto from invoice");
    m_origInvoiceDateEdit->setReadOnly(true);
    // Display-only mirror of the linked invoice date: no keyboard stop.
    m_origInvoiceDateEdit->setFocusPolicy(Qt::ClickFocus);
    m_origInvoiceDateEdit->setMinimumHeight(24);
    m_origInvoiceDateEdit->setMinimumWidth(90);
    metaRow->addWidget(m_origInvoiceDateEdit);

    metaRow->addWidget(new QLabel("Reason *:", this));
    m_reasonCodeCombo = new QComboBox(this);
    m_reasonCodeCombo->setMinimumHeight(24);
    m_reasonCodeCombo->setStyleSheet(
        "QComboBox { background-color: #FFFFFF; color: #0F172A; border: 1.5px solid #CBD5E1; border-radius: 6px; padding: 4px 8px; font-size: 12px; font-weight: 600; }"
        "QComboBox:focus { border: 2px solid #2563EB; background-color: #FFFFF0; }"
        "QComboBox QAbstractItemView { background-color: #FFFFFF; color: #0F172A; selection-background-color: #2563EB; selection-color: #FFFFFF; border: 1px solid #CBD5E1; outline: none; }");
    m_reasonCodeCombo->addItems({
        "01 - Sales / Purchase Return",
        "02 - Post-Sale / Purchase Rate Difference (Rate Cut)",
        "03 - Deficiency in Services / Quantity Shortage",
        "04 - Correction in Original Invoice",
        "05 - Change in Place of Supply (POS)"
    });
    hardenComboText(m_reasonCodeCombo, QColor("#0F172A"));
    metaRow->addWidget(m_reasonCodeCombo);

    metaRow->addWidget(new QLabel("POS:", this));
    m_posCombo = new QComboBox(this);
    m_posCombo->setMinimumHeight(24);
    m_posCombo->setStyleSheet(
        "QComboBox { background-color: #FFFFFF; color: #0F172A; border: 1.5px solid #CBD5E1; border-radius: 6px; padding: 4px 8px; font-size: 12px; font-weight: 600; }"
        "QComboBox:focus { border: 2px solid #2563EB; background-color: #FFFFF0; }"
        "QComboBox QAbstractItemView { background-color: #FFFFFF; color: #0F172A; selection-background-color: #2563EB; selection-color: #FFFFFF; border: 1px solid #CBD5E1; outline: none; }");
    populateStates();
    hardenComboText(m_posCombo, QColor("#0F172A"));
    metaRow->addWidget(m_posCombo);

    rootLayout->addLayout(metaRow);

    // (Party row lives above the meta row — entry starts with the party.)

    // ========================================================================
    // TIER 3: ITEM PARTICULARS & TAX BREAKUP MATRIX TABLE
    // ========================================================================
    auto* tableCard = new QFrame(this);
    tableCard->setStyleSheet("QFrame { background-color: #FFFFFF; border: 1px solid #CBD5E1; border-radius: 8px; }");
    auto* tableCardLayout = new QVBoxLayout(tableCard);
    tableCardLayout->setContentsMargins(10, 8, 10, 8);
    tableCardLayout->setSpacing(6);

    auto* tableTopRow = new QHBoxLayout();
    auto* tableTitle = new QLabel("ITEM PARTICULARS & TAX ADJUSTMENT MATRIX", tableCard);
    tableTitle->setStyleSheet("color: #1E293B; font-size: 11px; font-weight: 800; letter-spacing: 0.5px; border: none; background: transparent;");
    tableTopRow->addWidget(tableTitle);

    tableTopRow->addStretch(1);

    m_addRowBtn = new KbdBadgeButton("+ Add Item Row", "", tableCard);
    connect(m_addRowBtn, &QPushButton::clicked, this, &DebitCreditNoteWidget::onAddItemRow);
    tableTopRow->addWidget(m_addRowBtn);

    m_remRowBtn = new KbdBadgeButton("- Remove Item", "Del", tableCard);
    m_remRowBtn->setPrimaryColor(ErpTheme::kRedBg, ErpTheme::kRedBorder);
    m_remRowBtn->setTextColor(ErpTheme::kRed);
    connect(m_remRowBtn, &QPushButton::clicked, this, &DebitCreditNoteWidget::onRemoveItemRow);
    tableTopRow->addWidget(m_remRowBtn);

    tableCardLayout->addLayout(tableTopRow);

    m_itemsTable = new QTableWidget(0, 10, tableCard);
    m_itemsTable->setHorizontalHeaderLabels({
        "Commodity / Item Name (Press Space to Search)", "HSN Code", "Unit", "Bags",
        "Weight (Qtl)", "Rate (₹)", "Taxable Amount (₹)", "GST %", "GST Amt (₹)", "Total (₹)"
    });
    m_itemsTable->setSelectionBehavior(QAbstractItemView::SelectItems);
    m_itemsTable->setSelectionMode(QAbstractItemView::SingleSelection);
    m_itemsTable->setAlternatingRowColors(true);
    m_itemsTable->setShowGrid(true);
    m_itemsTable->verticalHeader()->setVisible(false);
    m_itemsTable->verticalHeader()->setDefaultSectionSize(28);

    m_itemsTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_itemsTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Fixed);
    m_itemsTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Fixed);
    m_itemsTable->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Fixed);
    m_itemsTable->horizontalHeader()->setSectionResizeMode(4, QHeaderView::Fixed);
    m_itemsTable->horizontalHeader()->setSectionResizeMode(5, QHeaderView::Fixed);
    m_itemsTable->horizontalHeader()->setSectionResizeMode(6, QHeaderView::Fixed);
    m_itemsTable->horizontalHeader()->setSectionResizeMode(7, QHeaderView::Fixed);
    m_itemsTable->horizontalHeader()->setSectionResizeMode(8, QHeaderView::Fixed);
    m_itemsTable->horizontalHeader()->setSectionResizeMode(9, QHeaderView::Fixed);
    m_itemsTable->setColumnWidth(1, 90);
    m_itemsTable->setColumnWidth(2, 60);
    m_itemsTable->setColumnWidth(3, 60);
    m_itemsTable->setColumnWidth(4, 90);
    m_itemsTable->setColumnWidth(5, 95);
    m_itemsTable->setColumnWidth(6, 110);
    m_itemsTable->setColumnWidth(7, 55);
    m_itemsTable->setColumnWidth(8, 95);
    m_itemsTable->setColumnWidth(9, 110);

    m_itemsTable->setStyleSheet(
        "QTableWidget {"
        "  background-color: #FFFFFF;"
        "  alternate-background-color: #F8FAFC;"
        "  border: 1px solid #CBD5E1;"
        "  border-radius: 6px;"
        "  gridline-color: #E2E8F0;"
        "  font-size: 12px;"
        "  color: #0F172A;"
        "  selection-background-color: #2563EB;"
        "  selection-color: #1E3A8A;"
        "}"
        "QTableWidget::item {"
        "  padding: 4px 6px;"
        "  color: #0F172A;"
        "}"
        "QTableWidget::item:selected {"
        "  background-color: #2563EB;"
        "  color: #FFFFFF;"
        "  font-weight: bold;"
        "}"
        "QHeaderView::section {"
        "  background-color: #0F172A;"
        "  color: #FFFFFF;"
        "  font-weight: 800;"
        "  font-size: 12px;"
        "  padding: 8px 10px;"
        "  border: none;"
        "  "
        "  border-right: 1px solid #334155;"
        "}"
    );

    m_itemDelegate = new ItemSearchDelegate(this);
    // Table-wide delegate (sale pattern): every cell — item search or plain
    // numeric editor — gets the same Enter/Backspace/Left/Right/PgUp/PgDn
    // contract. Column-scoped delegates leave numeric cells on default keys.
    m_itemsTable->setItemDelegate(m_itemDelegate);
    connect(m_itemDelegate, &ItemSearchDelegate::stockItemConfigured, this, &DebitCreditNoteWidget::onStockItemConfigured);
    connect(m_itemDelegate, &ItemSearchDelegate::moveNextRequested, this, &DebitCreditNoteWidget::advanceCell);
    connect(m_itemDelegate, &ItemSearchDelegate::movePrevRequested, this, &DebitCreditNoteWidget::retreatCell);
    connect(m_itemDelegate, &ItemSearchDelegate::browsePrevRequested, this, &DebitCreditNoteWidget::loadPreviousNote);
    connect(m_itemDelegate, &ItemSearchDelegate::browseNextRequested, this, &DebitCreditNoteWidget::loadNextNote);

    tableCardLayout->addWidget(m_itemsTable, 1);
    rootLayout->addWidget(tableCard, 1);

    // ========================================================================
    // TIER 4: 4-TIER SUMMARY METRICS CARDS (DESIGN SYSTEM STANDARD)
    // ========================================================================
    auto* summaryRowLayout = new QHBoxLayout();
    summaryRowLayout->setSpacing(10);

    auto createMetricCard = [](const QString& labelText, const QString& initVal, const QString& accentColor, int stretch = 1, bool isGrand = false) {
        auto* card = new QFrame();
        card->setStyleSheet(QString(
            "QFrame { background-color: %1; border: 1px solid #E2E8F0; border-left: 4px solid %2; border-radius: 6px; padding: 6px 12px; }"
        ).arg(isGrand ? "#F0FDF4" : "#FFFFFF", accentColor));

        auto* layout = new QVBoxLayout(card);
        layout->setContentsMargins(4, 4, 4, 4);
        layout->setSpacing(2);

        auto* topLabel = new QLabel(labelText, card);
        topLabel->setStyleSheet("font-size: 10px; font-weight: 800; color: #64748B; letter-spacing: 0.5px; text-transform: uppercase; border: none; background: transparent;");
        layout->addWidget(topLabel);

        auto* valLabel = new QLabel(initVal, card);
        valLabel->setStyleSheet(QString("font-size: %1px; font-weight: 800; color: %2; border: none; background: transparent;").arg(isGrand ? 18 : 15).arg(accentColor));
        layout->addWidget(valLabel);

        return std::make_pair(card, valLabel);
    };

    auto card1 = createMetricCard("TOTAL TAXABLE VALUE", "₹ 0.00", "#2563EB", 1, false);
    m_taxableValLabel = card1.second;
    summaryRowLayout->addWidget(card1.first, 1);

    auto card2 = createMetricCard("CGST (2.5%) & SGST (2.5%)", "₹ 0.00", "#0284C7", 1, false);
    m_cgstSgstValLabel = card2.second;
    summaryRowLayout->addWidget(card2.first, 1);

    auto card3 = createMetricCard("INTEGRATED GST (IGST)", "₹ 0.00", "#F59E0B", 1, false);
    m_igstValLabel = card3.second;
    summaryRowLayout->addWidget(card3.first, 1);

    auto card4 = createMetricCard("NET ADJUSTMENT GRAND TOTAL", "₹ 0.00", "#16A34A", 2, true);
    m_grandTotalValLabel = card4.second;
    summaryRowLayout->addWidget(card4.first, 2);

    rootLayout->addLayout(summaryRowLayout);

    // ========================================================================
    // TIER 5: NARRATION & ACTION FOOTER BAR
    // ========================================================================
    auto* footerCard = new QFrame(this);
    footerCard->setStyleSheet("QFrame { background-color: #FFFFFF; border: 1px solid #E2E8F0; border-radius: 8px; }");
    auto* footerLayout = new QHBoxLayout(footerCard);
    footerLayout->setContentsMargins(12, 8, 12, 8);
    footerLayout->setSpacing(12);

    auto* narrLabel = new QLabel("Narration / Remarks:", footerCard);
    narrLabel->setStyleSheet("font-weight: 700; font-size: 12px; color: #475569; border: none; background: transparent;");
    footerLayout->addWidget(narrLabel);

    m_narrationEdit = new QLineEdit(footerCard);
    m_narrationEdit->setPlaceholderText("Enter statutory remarks, reason for debit/credit adjustment in party ledger...");
    m_narrationEdit->setStyleSheet("background-color: #F8FAFC; border: 1.5px solid #CBD5E1; border-radius: 6px; padding: 6px 10px; font-size: 12px; color: #0F172A; font-weight: 600;");
    footerLayout->addWidget(m_narrationEdit, 1);

    m_statusLabel = new QLabel(footerCard);
    m_statusLabel->setStyleSheet("font-weight: 700; font-size: 11px; color: #475569; border: none; background: transparent;");
    footerLayout->addWidget(m_statusLabel);

    m_newBtn = new KbdBadgeButton("New", "F9", footerCard);
    m_newBtn->setPrimaryColor("#0284C7", "#0369A1");
    m_newBtn->setTextColor("#FFFFFF");
    connect(m_newBtn, &QPushButton::clicked, this, &DebitCreditNoteWidget::resetForm);
    footerLayout->addWidget(m_newBtn);

    m_cancelBtn = new KbdBadgeButton("Cancel", "Esc", footerCard);
    m_cancelBtn->setPrimaryColor("#F1F5F9", "#E2E8F0");
    m_cancelBtn->setTextColor("#475569");
    connect(m_cancelBtn, &QPushButton::clicked, this, &DebitCreditNoteWidget::backRequested);
    footerLayout->addWidget(m_cancelBtn);

    m_printBtn = new KbdBadgeButton("Print", "Ctrl+P", footerCard);
    connect(m_printBtn, &QPushButton::clicked, this, &DebitCreditNoteWidget::printNote);
    footerLayout->addWidget(m_printBtn);

    m_pdfBtn = new KbdBadgeButton("PDF", "Ctrl+E", footerCard);
    connect(m_pdfBtn, &QPushButton::clicked, this, &DebitCreditNoteWidget::exportPdf);
    footerLayout->addWidget(m_pdfBtn);

    m_deleteBtn = new KbdBadgeButton("Delete Note", "Ctrl+D", footerCard);
    m_deleteBtn->setPrimaryColor("#FEF2F2", "#FEE2E2");
    m_deleteBtn->setTextColor("#DC2626");
    m_deleteBtn->setVisible(false);
    connect(m_deleteBtn, &QPushButton::clicked, this, &DebitCreditNoteWidget::onDeleteClicked);
    footerLayout->addWidget(m_deleteBtn);

    m_saveBtn = new KbdBadgeButton("Save Note", "Ctrl+S", footerCard);
    m_saveBtn->setPrimaryColor("#16A34A", "#15803D");
    m_saveBtn->setTextColor("#FFFFFF");
    m_saveBtn->setMinimumWidth(200);
    connect(m_saveBtn, &QPushButton::clicked, this, &DebitCreditNoteWidget::onSaveClicked);
    footerLayout->addWidget(m_saveBtn);

    rootLayout->addWidget(footerCard);
}

void DebitCreditNoteWidget::setupConnections() {
    connect(m_noteTypeCombo, &QComboBox::currentIndexChanged, this, &DebitCreditNoteWidget::onNoteTypeChanged);
    connect(m_noteDateEdit, &AccountingDateEdit::dateChanged, this, [this](const QDate& dt) {
        m_dayLabel->setText(dt.toString("dddd"));
        FiscalYearInfo fy = FiscalYearHelper::getFiscalYearForDate(dt.toString("yyyy-MM-dd"));
        m_fyBadge->setText(fy.name);
        if (m_controller && m_editNoteId == 0) {
            m_controller->setDraftNoteDate(dt.toString("yyyy-MM-dd"));
            m_noteNoEdit->setText(m_controller->draftNoteNo());
        }
    });

    connect(m_partySearch, &AccountSearchBox::partyDataSelected, this, &DebitCreditNoteWidget::onPartyDataSelected);
    connect(m_origInvoiceTypeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this]() { refreshOrigInvoiceCombo(); });
    connect(m_origInvoiceCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &DebitCreditNoteWidget::onOrigInvoiceChosen);
    // Tab-out commits typed numbers too (Tab never fires currentIndexChanged).
    if (m_origInvoiceCombo->lineEdit()) {
        connect(m_origInvoiceCombo->lineEdit(), &QLineEdit::editingFinished, this, &DebitCreditNoteWidget::commitOrigInvoiceText);
    }
    connect(m_itemsTable, &QTableWidget::cellChanged, this, &DebitCreditNoteWidget::onGridCellChanged);
    connect(m_posCombo, &QComboBox::currentIndexChanged, this, &DebitCreditNoteWidget::onRecalculateTotals);
    if (m_saveBtn) m_saveBtn->installEventFilter(this);
    // Function keys must be caught here: QComboBox and the grid natively
    // swallow PgUp/PgDn (and F4 toggles combo dropdowns), so propagation
    // to keyPressEvent never happens from those controls.
    const QList<QWidget*> fnInputs = {
        m_noteTypeCombo, m_partySearch, m_origInvoiceTypeCombo, m_origInvoiceCombo,
        m_reasonCodeCombo, m_posCombo, m_partyGstinEdit, m_itemsTable, m_narrationEdit
    };
    for (QWidget* w : fnInputs) {
        if (!w) continue;
        w->installEventFilter(this);
        // Typing focus lives in a combo's line edit (all header combos
        // are editable now) — propagated keys never consult the combo's
        // own filters, so cover the edit directly.
        if (auto* cb = qobject_cast<QComboBox*>(w)) {
            if (cb->lineEdit()) cb->lineEdit()->installEventFilter(this);
        }
    }

    setupNavigationChains();
}

void DebitCreditNoteWidget::setupNavigationChains() {
    // Sale-voucher pattern: single Enter-forward declaration per field
    // (Backspace/Left derive backward automatically). Dates never take
    // keyboard focus — the dialog flow owns them.
    auto go = [](QWidget* w) {
        if (!w) return;
        w->setFocus();
        if (auto* le = qobject_cast<QLineEdit*>(w)) le->selectAll();
        else if (auto* cb = qobject_cast<QComboBox*>(w)) { if (cb->lineEdit()) cb->lineEdit()->selectAll(); }
        else if (auto* asb = qobject_cast<AccountSearchBox*>(w)) asb->selectAll();
    };
    auto stepBackToGrid = [this]() {
        if (!m_itemsTable || m_itemsTable->rowCount() == 0) return;
        int r = m_itemsTable->rowCount() - 1;
        m_itemsTable->setCurrentCell(r, 6);
        m_itemsTable->edit(m_itemsTable->currentIndex());
    };
    // Entry starts at the party (top row); the link fields below all
    // react to it. Single Enter-forward declaration per field.
    VoucherCommon::installGridNavigation(m_partySearch, nullptr, m_noteTypeCombo,
        [this, go]() { go(m_noteTypeCombo); });
    VoucherCommon::installGridNavigation(m_noteTypeCombo, m_partySearch, m_origInvoiceTypeCombo,
        [this, go]() { go(m_origInvoiceTypeCombo); });
    VoucherCommon::installGridNavigation(m_origInvoiceTypeCombo, m_noteTypeCombo, m_origInvoiceCombo,
        [this, go]() { go(m_origInvoiceCombo); });
    VoucherCommon::installGridNavigation(m_origInvoiceCombo, m_origInvoiceTypeCombo, m_reasonCodeCombo,
        [this]() { commitOrigInvoiceText(); if (m_reasonCodeCombo) { m_reasonCodeCombo->setFocus(); } });
    VoucherCommon::installGridNavigation(m_reasonCodeCombo, m_origInvoiceCombo, m_posCombo,
        [this, go]() { go(m_posCombo); });
    VoucherCommon::installGridNavigation(m_posCombo, m_reasonCodeCombo, nullptr,
        [this]() { focusTableAt(0, 0); }, stepBackToGrid);
    VoucherCommon::installGridNavigation(m_narrationEdit, nullptr, nullptr,
        [this]() { if (m_saveBtn) m_saveBtn->setFocus(); }, stepBackToGrid);
}

bool DebitCreditNoteWidget::eventFilter(QObject* watched, QEvent* event) {
    // installGridNavigation owns every field's Enter/Left/Right/Backspace;
    // only the Save button needs a local contract (Enter/Space saves,
    // Left steps back to narration).
    if (event->type() == QEvent::KeyPress) {
        auto* keyEvent = static_cast<QKeyEvent*>(event);
        int key = keyEvent->key();
        // Combos and the grid swallow PgUp/PgDn/F4 natively — intercept
        // before them. Open popups keep the keys (list paging/close).
        if ((key == Qt::Key_PageUp || key == Qt::Key_PageDown || key == Qt::Key_F4) &&
            watched != m_saveBtn) {
            if (!VoucherCommon::browseBlockedByPopup()) {
                if (key == Qt::Key_PageUp) loadPreviousNote();
                else if (key == Qt::Key_PageDown) loadNextNote();
                else openAlterNoteDialog();
                return true;
            }
            return false;
        }
        if (watched == m_saveBtn) {
            if (key == Qt::Key_Return || key == Qt::Key_Enter || key == Qt::Key_Space) {
                onSaveClicked();
                return true;
            } else if (key == Qt::Key_Left || key == Qt::Key_Up) {
                if (m_narrationEdit) {
                    m_narrationEdit->setFocus();
                    m_narrationEdit->selectAll();
                }
                return true;
            }
        }
    }
    return QWidget::eventFilter(watched, event);
}

void DebitCreditNoteWidget::populateStates() {
    m_posCombo->clear();
    PartiesModel pModel;
    QStringList states = pModel.get_states();
    if (states.isEmpty()) {
        states = {
            "06 - Haryana", "03 - Punjab", "07 - Delhi", "08 - Rajasthan",
            "09 - Uttar Pradesh", "04 - Chandigarh", "02 - Himachal Pradesh"
        };
    }
    m_posCombo->addItems(states);

    // Match company state if available
    QVariant compState = DatabaseManager::instance().executeScalar("SELECT state_code || ' - ' || state FROM company_info LIMIT 1;");
    if (compState.isValid() && !compState.toString().trimmed().isEmpty()) {
        int idx = m_posCombo->findText(compState.toString().trimmed(), Qt::MatchContains);
        if (idx >= 0) m_posCombo->setCurrentIndex(idx);
    }
}

void DebitCreditNoteWidget::keyPressEvent(QKeyEvent* event) {
    // Grid with focus but no cell editor open (unhandled keys propagate here
    // only in that state): Enter opens the current cell's editor; Backspace
    // on a filled cell clears it, on an empty cell steps back.
    if ((event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter ||
         event->key() == Qt::Key_Backspace) &&
        m_itemsTable && QApplication::focusWidget() == m_itemsTable) {
        int r = m_itemsTable->currentRow(), c = m_itemsTable->currentColumn();
        if (r >= 0 && c >= 0 && r < m_itemsTable->rowCount() && c < m_itemsTable->columnCount()) {
            if (event->key() == Qt::Key_Backspace) {
                if (QTableWidgetItem* it = m_itemsTable->item(r, c)) {
                    if (!it->text().isEmpty()) {
                        it->setText("");
                        onRecalculateTotals();
                        event->accept();
                        return;
                    }
                }
                retreatCell();
            } else {
                m_itemsTable->edit(m_itemsTable->currentIndex());
            }
            event->accept();
            return;
        }
    }
    if (event->key() == Qt::Key_F2) {
        openDateDialog(false);
        event->accept();
        return;
    }
    if (event->key() == Qt::Key_F4) {
        if (!VoucherCommon::browseBlockedByPopup()) openAlterNoteDialog();
        event->accept();
        return;
    }
    if (event->key() == Qt::Key_PageUp) {
        if (!VoucherCommon::browseBlockedByPopup()) loadPreviousNote();
        event->accept();
        return;
    }
    if (event->key() == Qt::Key_PageDown) {
        if (!VoucherCommon::browseBlockedByPopup()) loadNextNote();
        event->accept();
        return;
    }
    if (event->key() == Qt::Key_F9) {
        resetForm();
        event->accept();
        return;
    }
    if (event->key() == Qt::Key_Delete) {
        // Row removal is a grid action only — never while typing elsewhere.
        if (m_itemsTable && QApplication::focusWidget() == m_itemsTable) {
            onRemoveItemRow();
            event->accept();
            return;
        }
    }
    if (event->modifiers() & Qt::ControlModifier && event->key() == Qt::Key_S) {
        onSaveClicked();
        event->accept();
        return;
    }
    if (event->modifiers() & Qt::ControlModifier && event->key() == Qt::Key_D) {
        onDeleteClicked();
        event->accept();
        return;
    }
    if (event->modifiers() & Qt::ControlModifier && event->key() == Qt::Key_P) {
        printNote();
        event->accept();
        return;
    }
    if (event->modifiers() & Qt::ControlModifier && event->key() == Qt::Key_E) {
        exportPdf();
        event->accept();
        return;
    }
    if (event->key() == Qt::Key_Escape) {
        emit backRequested();
        event->accept();
        return;
    }
    QWidget::keyPressEvent(event);
}

void DebitCreditNoteWidget::resetForm() {
    m_editNoteId = 0;
    m_deleteBtn->setVisible(false);

    FiscalYearInfo activeFy = FiscalYearHelper::getActiveFiscalYear();
    // Strictly inside the selected fiscal year (sale pattern): today when
    // it falls in-year, otherwise the year's first day.
    QDate cur = QDate::currentDate();
    QDate fyStart = QDate::fromString(activeFy.startDate, "yyyy-MM-dd");
    QDate fyEnd = QDate::fromString(activeFy.endDate, "yyyy-MM-dd");
    QDate activeDate = cur;
    if (activeFy.isValid() && fyStart.isValid() && fyEnd.isValid()
        && (cur < fyStart || cur > fyEnd)) {
        activeDate = fyStart;
    }
    m_noteDateEdit->setDate(activeDate);
    m_dayLabel->setText(activeDate.toString("dddd"));
    m_fyBadge->setText(activeFy.name);

    if (m_controller) {
        m_controller->resetDraft();
        m_noteNoEdit->setText(m_controller->draftNoteNo());
    } else {
        m_noteNoEdit->setText("CN-0001");
    }
    if (m_noteBadge) m_noteBadge->setText("Credit Note");
    if (m_voucherNoDisplay) m_voucherNoDisplay->setText(QString("No. %1").arg(m_noteNoEdit->text().trimmed()));
    if (m_noteTypeCombo && m_noteTypeCombo->currentIndex() != 0) {
        m_noteTypeCombo->setCurrentIndex(0);
    }

    m_suppressInvoiceFetch = true;
    m_origInvoiceCombo->clear();
    if (m_origInvoiceCombo->lineEdit()) m_origInvoiceCombo->lineEdit()->clear();
    m_suppressInvoiceFetch = false;
    m_origInvoiceDateEdit->clear();
    m_partySearch->clearParty();
    m_partyBalLabel->setText("Date Bal. 0.00");
    m_partyGstinEdit->clear();
    m_narrationEdit->clear();

    m_isUpdatingTable = true;
    m_itemsTable->setRowCount(0);
    m_isUpdatingTable = false;

    onAddItemRow();
    onRecalculateTotals();
    setStatusMessage("Ready for new note entry", false);
    if (m_partySearch) { m_partySearch->setFocus(); m_partySearch->selectAll(); }
}

void DebitCreditNoteWidget::showEvent(QShowEvent* event) {
    QWidget::showEvent(event);
}

void DebitCreditNoteWidget::openDateDialog(bool isInitial) {
    Q_UNUSED(isInitial);
    QDate curDate = m_noteDateEdit->date();
    if (!curDate.isValid()) curDate = QDate::currentDate();
    FiscalYearInfo fyCtx = FiscalYearHelper::getFiscalYearForDate(curDate.toString("yyyy-MM-dd"));
    if (!fyCtx.isValid()) fyCtx = FiscalYearHelper::getActiveFiscalYear();

    QDate chosen = VoucherDateDialog::selectDate(this, curDate, fyCtx);
    if (chosen.isValid()) {
        m_noteDateEdit->setDate(chosen);
    }
    if (m_partySearch) {
        m_partySearch->setFocus();
        m_partySearch->selectAll();
    }
}

void DebitCreditNoteWidget::setStatusMessage(const QString& message, bool isError) {
    if (!m_statusLabel) return;
    m_statusLabel->setText(message);
    m_statusLabel->setStyleSheet(QString("font-weight: 700; font-size: 11px; color: %1; border: none; background: transparent;")
        .arg(isError ? "#DC2626" : "#475569"));
}

void DebitCreditNoteWidget::onNoteTypeChanged(int index) {
    if (m_controller && m_editNoteId == 0) {
        m_controller->setDraftNoteType(index == 0 ? "Credit Note" : "Debit Note");
        m_noteNoEdit->setText(m_controller->draftNoteNo());
    }
    if (m_noteBadge) m_noteBadge->setText(index == 0 ? "Credit Note" : "Debit Note");
    if (m_voucherNoDisplay && m_noteNoEdit) {
        m_voucherNoDisplay->setText(QString("No. %1").arg(m_noteNoEdit->text().trimmed()));
    }
    if (index == 0) {
        m_origInvoiceTypeCombo->setCurrentIndex(0); // Sale Invoice
    } else {
        m_origInvoiceTypeCombo->setCurrentIndex(1); // Purchase Bill
    }
    refreshOrigInvoiceCombo();
    onRecalculateTotals();
}

void DebitCreditNoteWidget::onPartyDataSelected(const QVariantMap& partyData) {
    QString partyName = partyData.value("party_name").toString().trimmed();
    if (partyName.isEmpty()) partyName = partyData.value("name").toString().trimmed();
    if (partyName.isEmpty()) return;

    QString gstin = partyData.value("gstin").toString().trimmed();
    if (m_controller) {
        m_controller->setDraftPartyName(partyName);
        if (!gstin.isEmpty()) {
            m_controller->setDraftPartyGstin(gstin);
            m_partyGstinEdit->setText(gstin);
        } else {
            m_partyGstinEdit->setText(m_controller->draftPartyGstin());
        }
    } else if (!gstin.isEmpty()) {
        m_partyGstinEdit->setText(gstin);
    }

    double bal = partyData.value("current_balance", 0.0).toDouble();
    if (m_partyBalLabel) {
        m_partyBalLabel->setText(QString("Date Bal. %1").arg(QString::number(bal, 'f', 2)));
    }

    // Auto update Place of Supply based on party GSTIN / state.
    QString state = partyData.value("state").toString().trimmed();
    QString gstinTxt = m_partyGstinEdit->text().trimmed();
    QString stateCode = gstinTxt.length() >= 2 ? gstinTxt.left(2) : state.left(2);
    if (!stateCode.isEmpty()) {
        for (int i = 0; i < m_posCombo->count(); ++i) {
            if (m_posCombo->itemText(i).startsWith(stateCode)) {
                m_posCombo->setCurrentIndex(i);
                break;
            }
        }
    }
    // Party drives the invoice list: refill + auto-pick latest invoice date.
    refreshOrigInvoiceCombo();
    onRecalculateTotals();
}

QString DebitCreditNoteWidget::currentOrigInvType() const {
    QString t = m_origInvoiceTypeCombo ? m_origInvoiceTypeCombo->currentText() : "Sale Invoice";
    if (t.contains("Purchase")) return "Purchase";
    if (t.contains("Manual")) return "Manual";
    return "Sale";
}

void DebitCreditNoteWidget::refreshOrigInvoiceCombo(bool preserveSelection) {
    if (!m_origInvoiceCombo) return;
    QString keep = preserveSelection ? m_origInvoiceCombo->currentText().trimmed() : QString();
    // The list is being rebuilt — the applied-number guard restarts with it.
    m_lastAppliedInvNo.clear();
    m_suppressInvoiceFetch = true;
    m_origInvoiceCombo->clear();
    int keepIdx = -1;
    if (m_controller && !m_partySearch->text().trimmed().isEmpty()
        && currentOrigInvType() != "Manual") {
        QVariantList rows = m_controller->partyInvoices(currentOrigInvType(), m_partySearch->text().trimmed());
        for (int i = 0; i < rows.size(); ++i) {
            QVariantMap r = rows.at(i).toMap();
            QString invNo = r.value("invoice_no").toString();
            QString invDate = r.value("invoice_date").toString();
            // Display is the bare invoice number only; the ISO date rides
            // along as item data and auto-fills Orig Date on selection.
            m_origInvoiceCombo->addItem(invNo, invNo);
            m_origInvoiceCombo->setItemData(i, invDate, Qt::UserRole + 1);
            if (!keep.isEmpty() && invNo == keep) keepIdx = i;
        }
    }
    if (keepIdx >= 0) {
        m_origInvoiceCombo->setCurrentIndex(keepIdx);
    } else if (!keep.isEmpty()) {
        m_origInvoiceCombo->setCurrentText(keep);
    } else if (m_origInvoiceCombo->count() > 0) {
        m_origInvoiceCombo->setCurrentIndex(0);
    }
    m_suppressInvoiceFetch = false;

    // Auto-fill the linked date for the current row (items load only on
    // explicit user pick, never while refilling behind their back).
    int idx = m_origInvoiceCombo->currentIndex();
    if (idx >= 0) {
        QString iso = m_origInvoiceCombo->itemData(idx, Qt::UserRole + 1).toString();
        if (!iso.isEmpty()) {
            QDate d = QDate::fromString(iso, "yyyy-MM-dd");
            m_origInvoiceDateEdit->setText(d.isValid() ? d.toString("dd-MM-yyyy") : iso);
            if (m_controller) m_controller->setDraftOrigInvDate(iso);
        }
    } else {
        m_origInvoiceDateEdit->clear();
    }
}

void DebitCreditNoteWidget::onOrigInvoiceChosen(int index) {
    if (m_suppressInvoiceFetch || !m_controller || index < 0) return;
    QString invNo = m_origInvoiceCombo->itemData(index).toString();
    if (invNo.isEmpty()) invNo = m_origInvoiceCombo->currentText().trimmed();
    applyOrigInvoice(invNo);
}

void DebitCreditNoteWidget::commitOrigInvoiceText() {
    if (m_suppressInvoiceFetch || !m_controller) return;
    QString typed = m_origInvoiceCombo ? m_origInvoiceCombo->currentText().trimmed() : "";
    if (typed.isEmpty() || currentOrigInvType() == "Manual") return;
    if (typed.compare(m_lastAppliedInvNo, Qt::CaseInsensitive) == 0) return;
    // Exact list match first (selects the row + its stashed date) ...
    for (int i = 0; i < m_origInvoiceCombo->count(); ++i) {
        QString invNo = m_origInvoiceCombo->itemData(i).toString();
        if (invNo.isEmpty()) invNo = m_origInvoiceCombo->itemText(i).trimmed();
        if (invNo.compare(typed, Qt::CaseInsensitive) == 0) {
            if (i != m_origInvoiceCombo->currentIndex()) {
                m_origInvoiceCombo->setCurrentIndex(i);
            } else {
                applyOrigInvoice(invNo);
            }
            return;
        }
    }
    // ... otherwise fetch the typed number straight from the books.
    applyOrigInvoice(typed);
}

void DebitCreditNoteWidget::applyOrigInvoice(const QString& invNo) {
    QString bareNo = invNo.trimmed();
    if (bareNo.isEmpty() || !m_controller || currentOrigInvType() == "Manual") return;

    if (!m_controller->fetchAndLoadOriginalInvoice(currentOrigInvType(), bareNo)) {
        setStatusMessage(QString("Invoice \"%1\" not found — kept manual entry").arg(bareNo), true);
        return;
    }
    // Remember the applied number so focus churn (Enter advance + Tab-out
    // editingFinished) doesn't re-fetch the same invoice twice.
    m_lastAppliedInvNo = bareNo;
    QDate d = QDate::fromString(m_controller->draftOrigInvDate(), "yyyy-MM-dd");
    m_origInvoiceDateEdit->setText(d.isValid() ? d.toString("dd-MM-yyyy") : m_controller->draftOrigInvDate());
    if (!m_controller->draftPartyName().isEmpty() && m_partySearch->text().trimmed().isEmpty()) {
        m_partySearch->setPartyName(m_controller->draftPartyName());
    }
    if (!m_controller->draftPartyGstin().isEmpty()) {
        m_partyGstinEdit->setText(m_controller->draftPartyGstin());
    }
    // The invoice's state drives Place of Supply (and therefore the
    // CGST+SGST vs IGST split) — no manual POS correction needed.
    QString invState = m_controller->draftStateCode().trimmed();
    if (invState.isEmpty() && m_controller->draftPartyGstin().length() >= 2) {
        invState = m_controller->draftPartyGstin().left(2);
    }
    if (!invState.isEmpty()) {
        int posIdx = -1;
        for (int i = 0; i < m_posCombo->count(); ++i) {
            if (m_posCombo->itemText(i).startsWith(invState)) { posIdx = i; break; }
        }
        if (posIdx < 0) {
            for (int i = 0; i < m_posCombo->count(); ++i) {
                if (m_posCombo->itemText(i).contains(invState, Qt::CaseInsensitive)) { posIdx = i; break; }
            }
        }
        if (posIdx >= 0) m_posCombo->setCurrentIndex(posIdx);
    }
    syncTableFromDraft();
    onRecalculateTotals();
    setStatusMessage(QString("Linked %1 with %2 line(s)").arg(invNo).arg(m_controller->itemsModel()->count()), false);
}

void DebitCreditNoteWidget::syncTableFromDraft() {
    if (!m_controller || !m_controller->itemsModel()) return;
    m_isUpdatingTable = true;
    m_itemsTable->setRowCount(0);

    auto createCell = [](const QString& text, Qt::Alignment align = Qt::AlignLeft, bool editable = true) {
        auto* it = new QTableWidgetItem(text);
        it->setTextAlignment(align | Qt::AlignVCenter);
        it->setForeground(QBrush(QColor("#0F172A")));
        if (editable) {
            it->setBackground(QBrush(QColor("#FFFFFF")));
            it->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable);
        } else {
            it->setBackground(QBrush(QColor("#F8FAFC")));
            it->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
        }
        return it;
    };

    auto* mdl = m_controller->itemsModel();
    for (int i = 0; i < mdl->count(); ++i) {
        auto m = mdl->get(i);
        int r = m_itemsTable->rowCount();
        m_itemsTable->insertRow(r);
        double taxable = m.value("taxableAmount").toDouble();
        double gst = m.value("gstPct").toDouble();
        double gstAmt = (taxable * gst) / 100.0;
        double tot = taxable + gstAmt;
        m_itemsTable->setItem(r, 0, createCell(m.value("itemName").toString(), Qt::AlignLeft, true));
        m_itemsTable->setItem(r, 1, createCell(m.value("hsnCode").toString(), Qt::AlignCenter, true));
        m_itemsTable->setItem(r, 2, createCell(m.value("unit").toString(), Qt::AlignCenter, true));
        m_itemsTable->setItem(r, 3, createCell(QString::number(m.value("bags").toInt()), Qt::AlignRight, true));
        m_itemsTable->setItem(r, 4, createCell(QString::number(m.value("weightQtl").toDouble(), 'f', 2), Qt::AlignRight, true));
        m_itemsTable->setItem(r, 5, createCell(QString::number(m.value("rate").toDouble(), 'f', 2), Qt::AlignRight, true));
        m_itemsTable->setItem(r, 6, createCell(QString::number(taxable, 'f', 2), Qt::AlignRight, true));
        m_itemsTable->setItem(r, 7, createCell(QString::number(gst, 'f', 1), Qt::AlignRight, false));
        m_itemsTable->setItem(r, 8, createCell(QString::number(gstAmt, 'f', 2), Qt::AlignRight, false));
        m_itemsTable->setItem(r, 9, createCell(QString::number(tot, 'f', 2), Qt::AlignRight, false));
    }
    m_isUpdatingTable = false;
}

void DebitCreditNoteWidget::onGridCellChanged(int row, int column) {
    if (m_isUpdatingTable) return;
    // Typed-and-committed item names fetch master details exactly like a
    // popup pick does (Tab-out commits without any signal otherwise).
    if (column == 0) fillItemDetailsFromMaster(row, false);
    onRecalculateTotals();
}

void DebitCreditNoteWidget::fillItemDetailsFromMaster(int row, bool focusNext) {
    if (!m_itemsTable || row < 0 || row >= m_itemsTable->rowCount()) return;
    QString name = m_itemsTable->item(row, 0) ? m_itemsTable->item(row, 0)->text().trimmed() : "";
    if (name.isEmpty()) return;

    QVariantMap found;
    const QVariantList rows = StockPipeline::instance().searchItems(name, 10);
    for (const QVariant& v : rows) {
        QVariantMap m = v.toMap();
        if (m.value("name").toString().compare(name, Qt::CaseInsensitive) == 0) {
            found = m;
            break;
        }
    }
    if (found.isEmpty()) return;

    double rate = (m_noteTypeCombo && m_noteTypeCombo->currentIndex() == 0)
        ? found.value("sale_rate", 0.0).toDouble()
        : found.value("purchase_rate", 0.0).toDouble();

    m_isUpdatingTable = true;
    auto setIf = [&](int col, const QString& text) {
        QTableWidgetItem* it = m_itemsTable->item(row, col);
        if (it && it->text() != text) it->setText(text);
    };
    setIf(1, found.value("hsn_code").toString());
    QString unit = found.value("unit").toString().trimmed();
    setIf(2, unit.isEmpty() ? "QTL" : unit);
    if (rate > 0.0) setIf(5, QString::number(rate, 'f', 2));
    // GST always follows the item master (the column is display-only).
    setIf(7, QString::number(found.value("gst_rate", 0.0).toDouble(), 'f', 1));
    m_isUpdatingTable = false;

    if (focusNext) focusTableAt(row, 1);
}

void DebitCreditNoteWidget::onStockItemConfigured(int row, const QVariantMap& itemData) {
    if (row < 0 || row >= m_itemsTable->rowCount()) return;

    m_isUpdatingTable = true;
    if (m_itemsTable->item(row, 0)) {
        QString name = itemData.value("name").toString();
        if (m_itemsTable->item(row, 0)->text() != name) {
            m_itemsTable->item(row, 0)->setText(name);
        }
    }
    m_isUpdatingTable = false;
    // Unit/HSN/rate/GST all come from the same master lookup, so a popup
    // pick and a typed name converge on identical row content.
    fillItemDetailsFromMaster(row, true);
    onRecalculateTotals();
}

void DebitCreditNoteWidget::onAddItemRow() {
    m_isUpdatingTable = true;
    int r = m_itemsTable->rowCount();
    m_itemsTable->insertRow(r);

    auto createCell = [](const QString& text, Qt::Alignment align = Qt::AlignLeft, bool editable = true) {
        auto* it = new QTableWidgetItem(text);
        it->setTextAlignment(align | Qt::AlignVCenter);
        it->setForeground(QBrush(QColor("#0F172A")));
        if (editable) {
            it->setBackground(QBrush(QColor("#FFFFFF")));
            it->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable);
        } else {
            it->setBackground(QBrush(QColor("#F8FAFC")));
            it->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
        }
        return it;
    };

    m_itemsTable->setItem(r, 0, createCell("", Qt::AlignLeft, true)); // Item Name
    m_itemsTable->setItem(r, 1, createCell("", Qt::AlignCenter, true)); // HSN
    m_itemsTable->setItem(r, 2, createCell("QTL", Qt::AlignCenter, true)); // Unit
    m_itemsTable->setItem(r, 3, createCell("0", Qt::AlignRight, true)); // Bags
    m_itemsTable->setItem(r, 4, createCell("0.00", Qt::AlignRight, true)); // Weight
    m_itemsTable->setItem(r, 5, createCell("0.00", Qt::AlignRight, true)); // Rate
    m_itemsTable->setItem(r, 6, createCell("0.00", Qt::AlignRight, true)); // Taxable
    // GST % follows the item master and is display-only (sale pattern:
    // computed columns are never keyboard stops).
    m_itemsTable->setItem(r, 7, createCell("0.0", Qt::AlignRight, false)); // GST %
    m_itemsTable->setItem(r, 8, createCell("0.00", Qt::AlignRight, false)); // GST Amt (computed)
    m_itemsTable->setItem(r, 9, createCell("0.00", Qt::AlignRight, false)); // Total (computed)

    m_isUpdatingTable = false;
    m_itemsTable->setCurrentCell(r, 0);
}

void DebitCreditNoteWidget::onRemoveItemRow() {
    int r = m_itemsTable->currentRow();
    if (r >= 0 && m_itemsTable->rowCount() > 1) {
        m_itemsTable->removeRow(r);
        onRecalculateTotals();
    }
}

void DebitCreditNoteWidget::onRecalculateTotals() {
    if (m_isUpdatingTable) return;

    double totalTaxable = 0.0;
    double totalGst = 0.0;

    // Determine Interstate status from Place of Supply vs Company State
    QString compStateCode = "06";
    QVariant csc = DatabaseManager::instance().executeScalar("SELECT state_code FROM company_info LIMIT 1;");
    if (csc.isValid() && !csc.toString().trimmed().isEmpty()) {
        compStateCode = csc.toString().trimmed();
    }

    QString posStr = m_posCombo->currentText();
    QString posCode = posStr.left(2);
    bool isInterstate = (!posCode.isEmpty() && posCode != compStateCode);

    m_isUpdatingTable = true;

    for (int r = 0; r < m_itemsTable->rowCount(); ++r) {
        auto* wtItem = m_itemsTable->item(r, 4);
        auto* rateItem = m_itemsTable->item(r, 5);
        auto* amtItem = m_itemsTable->item(r, 6);
        auto* gstItem = m_itemsTable->item(r, 7);
        auto* gstAmtItem = m_itemsTable->item(r, 8);
        auto* totItem = m_itemsTable->item(r, 9);

        double wt = wtItem ? wtItem->text().toDouble() : 0.0;
        double rate = rateItem ? rateItem->text().toDouble() : 0.0;
        double amt = amtItem ? amtItem->text().toDouble() : 0.0;
        double gstPct = gstItem ? gstItem->text().toDouble() : 5.0;

        if (wt > 0.0 && rate > 0.0 && amt == 0.0) {
            amt = wt * rate;
            if (amtItem) amtItem->setText(QString::number(amt, 'f', 2));
        }

        double rowGstAmt = (amt * gstPct) / 100.0;
        double rowTotal = amt + rowGstAmt;

        if (gstAmtItem) gstAmtItem->setText(QString::number(rowGstAmt, 'f', 2));
        if (totItem) totItem->setText(QString::number(rowTotal, 'f', 2));

        totalTaxable += amt;
        totalGst += rowGstAmt;
    }

    m_isUpdatingTable = false;

    double cgst = isInterstate ? 0.0 : (totalGst / 2.0);
    double sgst = isInterstate ? 0.0 : (totalGst / 2.0);
    double igst = isInterstate ? totalGst : 0.0;
    double grandTotal = std::round(totalTaxable + totalGst);

    m_taxableValLabel->setText("₹ " + QString::number(totalTaxable, 'f', 2));
    m_cgstSgstValLabel->setText(isInterstate ? "₹ 0.00" : QString("₹ %1 (₹ %2 each)").arg(QString::number(cgst + sgst, 'f', 2), QString::number(cgst, 'f', 2)));
    m_igstValLabel->setText("₹ " + QString::number(igst, 'f', 2));
    m_grandTotalValLabel->setText("₹ " + QString::number(grandTotal, 'f', 2));
}

void DebitCreditNoteWidget::focusTableAt(int row, int col) {
    if (!m_itemsTable || m_itemsTable->rowCount() == 0) return;
    int r = qBound(0, row, m_itemsTable->rowCount() - 1);
    int c = qBound(0, col, m_itemsTable->columnCount() - 1);
    m_itemsTable->setCurrentCell(r, c);
    m_itemsTable->setFocus(Qt::OtherFocusReason);
    m_itemsTable->edit(m_itemsTable->currentIndex());
}

void DebitCreditNoteWidget::moveCell(int row, int col) {
    focusTableAt(row, col);
}

void DebitCreditNoteWidget::advanceCell() {
    if (!m_itemsTable) return;
    int curRow = m_itemsTable->currentRow();
    int curCol = m_itemsTable->currentColumn();
    if (curRow < 0) curRow = 0;
    if (curCol < 0) curCol = 0;

    QTableWidgetItem* nameIt = m_itemsTable->item(curRow, 0);
    bool isItemEmpty = (!nameIt || nameIt->text().trimmed().isEmpty());
    if (curCol == 0 && isItemEmpty) {
        if (curRow > 0 && curRow == m_itemsTable->rowCount() - 1) {
            m_itemsTable->removeRow(curRow);
            onRecalculateTotals();
        }
        if (m_narrationEdit) { m_narrationEdit->setFocus(); m_narrationEdit->selectAll(); }
        return;
    }

    // Columns 7 (GST %), 8 (GST Amt) and 9 (Total) are master-driven or
    // computed — never keyboard stops.
    int nextCol = curCol + 1;
    if (nextCol >= 7) nextCol = 10;
    if (nextCol <= 6) {
        m_itemsTable->setCurrentCell(curRow, nextCol);
        m_itemsTable->edit(m_itemsTable->currentIndex());
    } else if (!isItemEmpty) {
        if (curRow == m_itemsTable->rowCount() - 1) onAddItemRow();
        m_itemsTable->setCurrentCell(curRow + 1, 0);
        m_itemsTable->edit(m_itemsTable->currentIndex());
    } else {
        if (curRow > 0 && curRow == m_itemsTable->rowCount() - 1) {
            m_itemsTable->removeRow(curRow);
            onRecalculateTotals();
        }
        if (m_narrationEdit) { m_narrationEdit->setFocus(); m_narrationEdit->selectAll(); }
    }
}

void DebitCreditNoteWidget::retreatCell() {
    if (!m_itemsTable) return;
    int curRow = m_itemsTable->currentRow();
    int curCol = m_itemsTable->currentColumn();
    int prevCol = curCol - 1;
    if (prevCol >= 7) prevCol = 6; // GST % / GST Amt / Total are display-only
    if (prevCol >= 0) {
        m_itemsTable->setCurrentCell(curRow, prevCol);
        m_itemsTable->edit(m_itemsTable->currentIndex());
    } else if (curRow > 0) {
        m_itemsTable->setCurrentCell(curRow - 1, 6);
        m_itemsTable->edit(m_itemsTable->currentIndex());
    } else if (m_posCombo) {
        m_posCombo->setFocus();
    }
}

bool DebitCreditNoteWidget::loadNoteForEditing(int noteId, bool popupDateOnLoad) {
    if (!m_controller) return false;
    bool ok = m_controller->loadNoteIntoDraft(noteId);
    if (!ok) return false;

    m_editNoteId = noteId;
    m_deleteBtn->setVisible(true);

    QString noteTitle = QString("%1 • %2").arg(m_controller->draftNoteNo(), m_controller->draftNoteType());
    m_voucherNoDisplay->setText(QString("No. %1").arg(m_controller->draftNoteNo()));
    if (m_noteBadge) m_noteBadge->setText(m_controller->draftNoteType());

    m_noteTypeCombo->setCurrentIndex(m_controller->draftNoteType() == "Credit Note" ? 0 : 1);
    m_noteNoEdit->setText(m_controller->draftNoteNo());
    QDate d = QDate::fromString(m_controller->draftNoteDate(), "yyyy-MM-dd");
    if (d.isValid()) {
        m_noteDateEdit->setDate(d);
        m_dayLabel->setText(d.toString("dddd"));
        FiscalYearInfo fy = FiscalYearHelper::getFiscalYearForDate(d.toString("yyyy-MM-dd"));
        m_fyBadge->setText(fy.name);
    }

    // Orig doc type first (it scopes the invoice list), then the party so
    // the combo refills around the stored invoice, preserved by text.
    QString oType = m_controller->draftOrigInvType();
    m_origInvoiceTypeCombo->setCurrentIndex(oType == "Purchase" ? 1 : 0);
    m_partySearch->setPartyName(m_controller->draftPartyName());
    m_suppressInvoiceFetch = true;
    m_origInvoiceCombo->clear();
    QString storedInv = m_controller->draftOrigInvNo();
    if (!storedInv.isEmpty()) m_origInvoiceCombo->addItem(storedInv, storedInv);
    m_lastAppliedInvNo = storedInv;
    m_suppressInvoiceFetch = false;
    refreshOrigInvoiceCombo(true);
    QDate od = QDate::fromString(m_controller->draftOrigInvDate(), "yyyy-MM-dd");
    m_origInvoiceDateEdit->setText(od.isValid() ? od.toString("dd-MM-yyyy") : m_controller->draftOrigInvDate());
    m_partyGstinEdit->setText(m_controller->draftPartyGstin());
    m_narrationEdit->setText(m_controller->draftNarration());

    syncTableFromDraft();
    onRecalculateTotals();
    setStatusMessage(QString("Loaded %1 for alteration").arg(noteTitle), false);
    // Drill-down entry: date dialog first (dates take no keyboard focus),
    // then the party field — openDateDialog already lands there.
    if (popupDateOnLoad) openDateDialog(false);
    return true;
}

int DebitCreditNoteWidget::findNoteRow(int noteId) const {
    if (!m_controller || !m_controller->model()) return -1;
    for (int i = 0; i < m_controller->model()->count(); ++i) {
        if (m_controller->model()->get(i).value("id").toInt() == noteId) return i;
    }
    return -1;
}

void DebitCreditNoteWidget::loadPreviousNote() {
    if (!m_controller || !m_controller->model()) return;
    int n = m_controller->model()->count();
    if (n == 0) return;
    int row = findNoteRow(m_editNoteId);
    int target = (row < 0) ? 0 : qMin(row + 1, n - 1);
    loadNoteForEditing(m_controller->model()->get(target).value("id").toInt());
}

void DebitCreditNoteWidget::loadNextNote() {
    if (!m_controller || !m_controller->model()) return;
    int n = m_controller->model()->count();
    if (n == 0) return;
    int row = findNoteRow(m_editNoteId);
    if (row < 0) { loadNoteForEditing(m_controller->model()->get(0).value("id").toInt()); return; }
    if (row == 0) { resetForm(); return; }
    loadNoteForEditing(m_controller->model()->get(row - 1).value("id").toInt());
}

void DebitCreditNoteWidget::openAlterNoteDialog() {
    bool ok = false;
    QString text = CustomInputDialog::getText(this, "Find & Alter Note",
        "Enter Note No (e.g. CN-0001) to alter:", m_noteNoEdit ? m_noteNoEdit->text().trimmed() : "", &ok);
    if (!ok || text.trimmed().isEmpty() || !m_controller || !m_controller->model()) return;
    for (int i = 0; i < m_controller->model()->count(); ++i) {
        QVariantMap r = m_controller->model()->get(i);
        if (r.value("noteNo").toString().compare(text.trimmed(), Qt::CaseInsensitive) == 0) {
            loadNoteForEditing(r.value("id").toInt());
            return;
        }
    }
    CustomMessageBox::warning(this, "Note Not Found", QString("No note found matching \"%1\".").arg(text.trimmed()));
}

void DebitCreditNoteWidget::printNote() {
    if (m_editNoteId <= 0) {
        CustomMessageBox::showWarning(this, "Nothing to Print", "Save the note before printing.");
        return;
    }
    QString noteNo = m_noteNoEdit ? m_noteNoEdit->text().trimmed() : "";
    if (noteNo.isEmpty() || !m_printExportCtrl) {
        CustomMessageBox::showWarning(this, "Nothing to Print", "Save the note before printing.");
        return;
    }
    if (!m_printExportCtrl->print_debit_credit_note(noteNo)) {
        CustomMessageBox::showCritical(this, "Print Failed", QString("Could not print note \"%1\".").arg(noteNo));
    }
}

void DebitCreditNoteWidget::exportPdf() {
    if (m_editNoteId <= 0) {
        CustomMessageBox::showWarning(this, "Nothing to Export", "Save the note before exporting.");
        return;
    }
    QString noteNo = m_noteNoEdit ? m_noteNoEdit->text().trimmed() : "";
    if (noteNo.isEmpty() || !m_printExportCtrl) {
        CustomMessageBox::showWarning(this, "Nothing to Export", "Save the note before exporting.");
        return;
    }
    QString path = m_printExportCtrl->export_debit_credit_note_pdf(noteNo, "");
    if (path.isEmpty()) {
        CustomMessageBox::showCritical(this, "Export Failed", QString("Could not export note \"%1\" to PDF.").arg(noteNo));
    }
}

void DebitCreditNoteWidget::onSaveClicked() {
    if (m_partySearch->text().trimmed().isEmpty()) {
        CustomMessageBox::showWarning(this, "Validation Error", "Please select a Party / Account Name.");
        m_partySearch->setFocus();
        return;
    }

    if (m_itemsTable->rowCount() == 0) {
        CustomMessageBox::showWarning(this, "Validation Error", "Please add at least one item row.");
        return;
    }

    if (!CustomMessageBox::showConfirmation(this, "Confirm Save", "Are you sure you want to save Note No. '" + m_noteNoEdit->text() + "'?")) {
        return;
    }

    bool ok = false;
    if (m_controller) {
        m_controller->setDraftNoteType(m_noteTypeCombo->currentIndex() == 0 ? "Credit Note" : "Debit Note");
        m_controller->setDraftNoteNo(m_noteNoEdit->text().trimmed());
        m_controller->setDraftNoteDate(m_noteDateEdit->isoDate());
        QString origInvNo = m_origInvoiceCombo ? m_origInvoiceCombo->currentText().trimmed() : "";
        if (origInvNo.contains("  |  ")) origInvNo = origInvNo.section("  |  ", 0, 0).trimmed();
        m_controller->setDraftOrigInvNo(origInvNo);
        m_controller->setDraftOrigInvDate(m_origInvoiceDateEdit->text().trimmed());
        m_controller->setDraftOrigInvType(m_origInvoiceTypeCombo->currentText().contains("Sale") ? "Sale" : "Purchase");
        m_controller->setDraftPartyName(m_partySearch->text().trimmed());
        m_controller->setDraftPartyGstin(m_partyGstinEdit->text().trimmed());
        m_controller->setDraftReasonCode(m_reasonCodeCombo->currentText());
        m_controller->setDraftNarration(m_narrationEdit->text().trimmed());

        m_controller->itemsModel()->clear();
        for (int r = 0; r < m_itemsTable->rowCount(); ++r) {
            QString iName = m_itemsTable->item(r, 0) ? m_itemsTable->item(r, 0)->text().trimmed() : "";
            if (iName.isEmpty()) continue;
            QString hsn = m_itemsTable->item(r, 1) ? m_itemsTable->item(r, 1)->text().trimmed() : "";
            int bags = m_itemsTable->item(r, 3) ? m_itemsTable->item(r, 3)->text().toInt() : 0;
            double wt = m_itemsTable->item(r, 4) ? m_itemsTable->item(r, 4)->text().toDouble() : 0.0;
            double rate = m_itemsTable->item(r, 5) ? m_itemsTable->item(r, 5)->text().toDouble() : 0.0;
            double taxable = m_itemsTable->item(r, 6) ? m_itemsTable->item(r, 6)->text().toDouble() : 0.0;
            double gst = m_itemsTable->item(r, 7) ? m_itemsTable->item(r, 7)->text().toDouble() : 5.0;

            m_controller->itemsModel()->addItem(iName, hsn, bags, wt, rate, taxable, gst);
        }

        ok = m_controller->saveCurrentDraft();
    } else {
        ok = true;
    }

    if (ok) {
        CustomMessageBox::showInformation(this, "Success", "Note saved successfully.");
        QString nNo = m_noteNoEdit->text();
        resetForm();
        emit noteSaved(nNo);
    } else {
        CustomMessageBox::showCritical(this, "Save Failed", "Failed to save Note.");
    }
}

void DebitCreditNoteWidget::onDeleteClicked() {
    if (m_editNoteId <= 0) return;

    if (!CustomMessageBox::showConfirmation(this, "Confirm Delete", "Are you sure you want to delete this Note? This action cannot be undone.")) {
        return;
    }

    bool ok = false;
    if (m_controller) {
        ok = m_controller->deleteNote(m_editNoteId);
    }

    if (ok) {
        CustomMessageBox::showInformation(this, "Deleted", "Note removed successfully.");
        resetForm();
        emit noteSaved("");
    } else {
        CustomMessageBox::showCritical(this, "Delete Failed", "Failed to delete Note.");
    }
}

} // namespace MahadevERP
