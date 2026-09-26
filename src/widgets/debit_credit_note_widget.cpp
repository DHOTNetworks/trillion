#include "debit_credit_note_widget.h"
#include "voucher_date_dialog.h"
#include "kbd_badge_button.h"
#include "custom_dialogs.h"
#include "../engine/fiscal_year_helper.h"
#include "../database_manager.h"
#include "../models/parties_model.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QHeaderView>
#include <QFrame>
#include <QDate>
#include <QKeyEvent>
#include <cmath>

namespace MahadevERP {

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
    setAttribute(Qt::WA_StyledBackground, true);
    setStyleSheet("DebitCreditNoteWidget { background-color: #F8FAFC; font-family: 'Segoe UI', -apple-system, BlinkMacSystemFont, Roboto, sans-serif; }");

    auto* rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(16, 14, 16, 14);
    rootLayout->setSpacing(10);

    // ========================================================================
    // TIER 1: STICKY HEADER BAR CARD
    // ========================================================================
    auto* headerCard = new QFrame(this);
    headerCard->setFixedHeight(54);
    headerCard->setStyleSheet("QFrame { background-color: #FFFFFF; border: 1px solid #E2E8F0; border-radius: 8px; }");
    auto* headerLayout = new QHBoxLayout(headerCard);
    headerLayout->setContentsMargins(14, 6, 14, 6);
    headerLayout->setSpacing(12);

    auto* titleCol = new QVBoxLayout();
    titleCol->setSpacing(1);
    auto* titleLabel = new QLabel("GST Debit & Credit Notes Entry (Section 34)", headerCard);
    titleLabel->setStyleSheet("font-size: 15px; font-weight: 800; color: #0F172A; border: none; background: transparent;");
    auto* subLabel = new QLabel("Issue statutory GST Credit Notes (Sales Return / Rate Cut) & Debit Notes (Purchase Return / Rate Difference).", headerCard);
    subLabel->setStyleSheet("font-size: 11px; color: #64748B; border: none; background: transparent;");
    titleCol->addWidget(titleLabel);
    titleCol->addWidget(subLabel);
    headerLayout->addLayout(titleCol);
    headerLayout->addStretch(1);

    m_newBtn = new KbdBadgeButton("+ New Note", "F9", headerCard);
    m_newBtn->setPrimaryColor("#0284C7", "#0369A1");
    m_newBtn->setTextColor("#FFFFFF");
    headerLayout->addWidget(m_newBtn);

    auto* backBtn = new KbdBadgeButton("← Back to Dashboard", "Esc", headerCard);
    backBtn->setPrimaryColor("#F1F5F9", "#E2E8F0");
    backBtn->setTextColor("#475569");
    connect(backBtn, &QPushButton::clicked, this, &DebitCreditNoteWidget::backRequested);
    headerLayout->addWidget(backBtn);

    rootLayout->addWidget(headerCard);

    // ========================================================================
    // TIER 2: TOP METADATA & PARTY DETAILS CARDS (2 SIDE-BY-SIDE COLUMNS)
    // ========================================================================
    auto* metaRowLayout = new QHBoxLayout();
    metaRowLayout->setSpacing(10);

    const QString cardStyle =
        "QFrame { background-color: #FFFFFF; border: 1px solid #E2E8F0; border-radius: 8px; }"
        "QLabel { color: #475569; font-size: 11.5px; font-weight: 700; border: none; background: transparent; }"
        "QLineEdit, QComboBox { background-color: #FFFFFF; border: 1.5px solid #CBD5E1; border-radius: 6px; padding: 4px 8px; font-size: 12px; color: #0F172A; font-weight: 600; }"
        "QLineEdit:focus, QComboBox:focus { border-color: #2563EB; background-color: #F8FAFC; }";

    // --- Left Card: Note Identification & Invoice Reference ---
    auto* leftCard = new QFrame(this);
    leftCard->setStyleSheet(cardStyle);
    auto* leftLayout = new QVBoxLayout(leftCard);
    leftLayout->setContentsMargins(12, 10, 12, 10);
    leftLayout->setSpacing(8);

    auto* leftHeader = new QLabel("NOTE IDENTIFICATION & ORIGINAL INVOICE LINK", leftCard);
    leftHeader->setStyleSheet("color: #2563EB; font-size: 10.5px; font-weight: 800; letter-spacing: 0.5px; border: none; background: transparent;");
    leftLayout->addWidget(leftHeader);

    auto* leftGrid = new QGridLayout();
    leftGrid->setHorizontalSpacing(10);
    leftGrid->setVerticalSpacing(6);

    // Row 0: Note Type, Date
    leftGrid->addWidget(new QLabel("Note Type *:", leftCard), 0, 0);
    m_noteTypeCombo = new QComboBox(leftCard);
    m_noteTypeCombo->addItems({"Credit Note (Sales Return / Rate Cut)", "Debit Note (Purchase Return / Rate Difference)"});
    m_noteTypeCombo->setStyleSheet("background-color: #EFF6FF; border: 1.5px solid #93C5FD; border-radius: 6px; padding: 4px 8px; font-weight: bold; color: #1D4ED8;");
    leftGrid->addWidget(m_noteTypeCombo, 0, 1);

    leftGrid->addWidget(new QLabel("Date (F2) *:", leftCard), 0, 2);
    auto* dateBox = new QHBoxLayout();
    dateBox->setSpacing(5);
    dateBox->setContentsMargins(0, 0, 0, 0);
    m_noteDateEdit = new AccountingDateEdit(leftCard);
    m_noteDateEdit->setMinimumWidth(100);
    m_noteDateEdit->setMaximumWidth(110);
    dateBox->addWidget(m_noteDateEdit);

    auto* dateBtn = new QPushButton("📅", leftCard);
    dateBtn->setFixedSize(26, 26);
    dateBtn->setCursor(Qt::PointingHandCursor);
    dateBtn->setStyleSheet("background-color: #EFF6FF; border: 1px solid #93C5FD; border-radius: 4px; font-weight: bold; color: #1D4ED8;");
    connect(dateBtn, &QPushButton::clicked, this, &DebitCreditNoteWidget::openDateDialog);
    dateBox->addWidget(dateBtn);

    m_dayLabel = new QLabel("", leftCard);
    m_dayLabel->setStyleSheet("color: #0284C7; font-weight: 800; font-size: 11px;");
    dateBox->addWidget(m_dayLabel);

    m_fyBadge = new QLabel(FiscalYearHelper::getActiveFiscalYear().name, leftCard);
    m_fyBadge->setStyleSheet("background-color: #F1F5F9; color: #475569; border: 1px solid #CBD5E1; border-radius: 4px; padding: 2px 6px; font-size: 10.5px; font-weight: 800;");
    dateBox->addWidget(m_fyBadge);
    dateBox->addStretch();

    leftGrid->addLayout(dateBox, 0, 3);

    // Row 1: Note No, Orig Doc Type
    leftGrid->addWidget(new QLabel("Note No:", leftCard), 1, 0);
    m_noteNoEdit = new QLineEdit(leftCard);
    m_noteNoEdit->setReadOnly(true);
    m_noteNoEdit->setStyleSheet("background-color: #F8FAFC; border: 1.5px solid #CBD5E1; border-radius: 6px; padding: 4px 8px; font-weight: 800; color: #1D4ED8; font-family: 'Consolas', monospace;");
    leftGrid->addWidget(m_noteNoEdit, 1, 1);

    leftGrid->addWidget(new QLabel("Orig Doc Type:", leftCard), 1, 2);
    m_origInvoiceTypeCombo = new QComboBox(leftCard);
    m_origInvoiceTypeCombo->addItems({"Sale Invoice", "Purchase Bill", "Other / Manual"});
    leftGrid->addWidget(m_origInvoiceTypeCombo, 1, 3);

    // Row 2: Original Inv No, Original Inv Date
    leftGrid->addWidget(new QLabel("Orig Invoice No *:", leftCard), 2, 0);
    m_origInvoiceNoEdit = new QLineEdit(leftCard);
    m_origInvoiceNoEdit->setPlaceholderText("e.g. INV-0045");
    leftGrid->addWidget(m_origInvoiceNoEdit, 2, 1);

    leftGrid->addWidget(new QLabel("Orig Inv Date:", leftCard), 2, 2);
    m_origInvoiceDateEdit = new QLineEdit(leftCard);
    m_origInvoiceDateEdit->setPlaceholderText("dd-MM-yyyy");
    leftGrid->addWidget(m_origInvoiceDateEdit, 2, 3);

    leftLayout->addLayout(leftGrid);
    metaRowLayout->addWidget(leftCard, 1);

    // --- Right Card: Party Details & GST Reason ---
    auto* rightCard = new QFrame(this);
    rightCard->setStyleSheet(cardStyle);
    auto* rightLayout = new QVBoxLayout(rightCard);
    rightLayout->setContentsMargins(12, 10, 12, 10);
    rightLayout->setSpacing(8);

    auto* rightHeader = new QLabel("PARTY DETAILS & GST STATUTORY MATRIX", rightCard);
    rightHeader->setStyleSheet("color: #2563EB; font-size: 10.5px; font-weight: 800; letter-spacing: 0.5px; border: none; background: transparent;");
    rightLayout->addWidget(rightHeader);

    auto* rightGrid = new QGridLayout();
    rightGrid->setHorizontalSpacing(10);
    rightGrid->setVerticalSpacing(6);
    rightGrid->setColumnStretch(0, 0);
    rightGrid->setColumnStretch(1, 1);
    rightGrid->setColumnStretch(2, 0);
    rightGrid->setColumnStretch(3, 1);

    leftGrid->setColumnStretch(0, 0);
    leftGrid->setColumnStretch(1, 1);
    leftGrid->setColumnStretch(2, 0);
    leftGrid->setColumnStretch(3, 1);

    // Row 0: Party Account & GSTIN
    rightGrid->addWidget(new QLabel("Party / Account *:", rightCard), 0, 0);
    m_partySearch = new AccountSearchBox(rightCard);
    rightGrid->addWidget(m_partySearch, 0, 1);

    rightGrid->addWidget(new QLabel("Party GSTIN:", rightCard), 0, 2);
    m_partyGstinEdit = new QLineEdit(rightCard);
    m_partyGstinEdit->setPlaceholderText("Unregistered / GSTIN");
    m_partyGstinEdit->setStyleSheet("background-color: #F8FAFC; border: 1.5px solid #CBD5E1; border-radius: 6px; padding: 4px 8px; font-weight: bold; font-family: 'Consolas', monospace; color: #334155;");
    rightGrid->addWidget(m_partyGstinEdit, 0, 3);

    // Row 1: Reason Code & Place of Supply
    rightGrid->addWidget(new QLabel("Reason Code *:", rightCard), 1, 0);
    m_reasonCodeCombo = new QComboBox(rightCard);
    m_reasonCodeCombo->addItems({
        "01 - Sales / Purchase Return",
        "02 - Post-Sale / Purchase Rate Difference (Rate Cut)",
        "03 - Deficiency in Services / Quantity Shortage",
        "04 - Correction in Original Invoice",
        "05 - Change in Place of Supply (POS)"
    });
    rightGrid->addWidget(m_reasonCodeCombo, 1, 1);

    rightGrid->addWidget(new QLabel("Place of Supply:", rightCard), 1, 2);
    m_posCombo = new QComboBox(rightCard);
    populateStates();
    rightGrid->addWidget(m_posCombo, 1, 3);

    rightLayout->addLayout(rightGrid);
    metaRowLayout->addWidget(rightCard, 1);

    rootLayout->addLayout(metaRowLayout);

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

    m_addRowBtn = new QPushButton("+ Add Item Row (F4)", tableCard);
    m_addRowBtn->setCursor(Qt::PointingHandCursor);
    m_addRowBtn->setStyleSheet(
        "QPushButton { background-color: #EFF6FF; border: 1.5px solid #93C5FD; border-radius: 6px; padding: 4px 12px; font-weight: 800; font-size: 11.5px; color: #1D4ED8; }"
        "QPushButton:hover { background-color: #DBEAFE; }"
    );
    connect(m_addRowBtn, &QPushButton::clicked, this, &DebitCreditNoteWidget::onAddItemRow);
    tableTopRow->addWidget(m_addRowBtn);

    m_remRowBtn = new QPushButton("- Remove Item (Del)", tableCard);
    m_remRowBtn->setCursor(Qt::PointingHandCursor);
    m_remRowBtn->setStyleSheet(
        "QPushButton { background-color: #FEF2F2; border: 1.5px solid #FECACA; border-radius: 6px; padding: 4px 12px; font-weight: 800; font-size: 11.5px; color: #DC2626; }"
        "QPushButton:hover { background-color: #FEE2E2; }"
    );
    connect(m_remRowBtn, &QPushButton::clicked, this, &DebitCreditNoteWidget::onRemoveItemRow);
    tableTopRow->addWidget(m_remRowBtn);

    tableCardLayout->addLayout(tableTopRow);

    m_itemsTable = new QTableWidget(0, 10, tableCard);
    m_itemsTable->setHorizontalHeaderLabels({
        "Commodity / Item Name (Press Space to Search)", "HSN Code", "Unit", "Bags",
        "Weight (Qtl)", "Rate (₹)", "Taxable Amount (₹)", "GST %", "GST Amt (₹)", "Total (₹)"
    });
    m_itemsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_itemsTable->setSelectionMode(QAbstractItemView::SingleSelection);
    m_itemsTable->setAlternatingRowColors(true);
    m_itemsTable->setShowGrid(true);
    m_itemsTable->verticalHeader()->setVisible(false);
    m_itemsTable->verticalHeader()->setDefaultSectionSize(28);

    m_itemsTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_itemsTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_itemsTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    m_itemsTable->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    m_itemsTable->horizontalHeader()->setSectionResizeMode(4, QHeaderView::ResizeToContents);
    m_itemsTable->horizontalHeader()->setSectionResizeMode(5, QHeaderView::ResizeToContents);
    m_itemsTable->horizontalHeader()->setSectionResizeMode(6, QHeaderView::ResizeToContents);
    m_itemsTable->horizontalHeader()->setSectionResizeMode(7, QHeaderView::ResizeToContents);
    m_itemsTable->horizontalHeader()->setSectionResizeMode(8, QHeaderView::ResizeToContents);
    m_itemsTable->horizontalHeader()->setSectionResizeMode(9, QHeaderView::ResizeToContents);

    m_itemsTable->setStyleSheet(
        "QTableWidget {"
        "  background-color: #FFFFFF;"
        "  alternate-background-color: #F8FAFC;"
        "  border: 1px solid #CBD5E1;"
        "  border-radius: 6px;"
        "  gridline-color: #E2E8F0;"
        "  font-size: 12px;"
        "  color: #0F172A;"
        "  selection-background-color: #EFF6FF;"
        "  selection-color: #1E3A8A;"
        "}"
        "QTableWidget::item {"
        "  padding: 4px 6px;"
        "  color: #0F172A;"
        "}"
        "QTableWidget::item:selected {"
        "  background-color: #EFF6FF;"
        "  color: #1E3A8A;"
        "  font-weight: bold;"
        "}"
        "QHeaderView::section {"
        "  background-color: #0F172A;"
        "  color: #FFFFFF;"
        "  font-weight: 800;"
        "  font-size: 11px;"
        "  padding: 6px 8px;"
        "  border: none;"
        "  border-right: 1px solid #334155;"
        "}"
    );

    m_itemDelegate = new ItemSearchDelegate(this);
    m_itemsTable->setItemDelegateForColumn(0, m_itemDelegate);
    connect(m_itemDelegate, &ItemSearchDelegate::stockItemConfigured, this, &DebitCreditNoteWidget::onStockItemConfigured);

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

    m_cancelBtn = new KbdBadgeButton("Cancel", "Esc", footerCard);
    m_cancelBtn->setPrimaryColor("#F1F5F9", "#E2E8F0");
    m_cancelBtn->setTextColor("#475569");
    connect(m_cancelBtn, &QPushButton::clicked, this, &DebitCreditNoteWidget::backRequested);
    footerLayout->addWidget(m_cancelBtn);

    m_deleteBtn = new KbdBadgeButton("Delete Note", "Del", footerCard);
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

    connect(m_partySearch, &AccountSearchBox::partySelected, this, &DebitCreditNoteWidget::onPartySelected);
    connect(m_itemsTable, &QTableWidget::cellChanged, this, &DebitCreditNoteWidget::onRecalculateTotals);
    connect(m_posCombo, &QComboBox::currentIndexChanged, this, &DebitCreditNoteWidget::onRecalculateTotals);
    connect(m_newBtn, &QPushButton::clicked, this, &DebitCreditNoteWidget::resetForm);
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
    if (event->key() == Qt::Key_Escape) {
        emit backRequested();
        event->accept();
        return;
    }
    if (event->key() == Qt::Key_F4) {
        onAddItemRow();
        event->accept();
        return;
    }
    if (event->key() == Qt::Key_F9) {
        resetForm();
        event->accept();
        return;
    }
    if (event->modifiers() & Qt::ControlModifier && event->key() == Qt::Key_S) {
        onSaveClicked();
        event->accept();
        return;
    }
    QWidget::keyPressEvent(event);
}

void DebitCreditNoteWidget::resetForm() {
    m_editNoteId = 0;
    m_deleteBtn->setVisible(false);

    FiscalYearInfo activeFy = FiscalYearHelper::getActiveFiscalYear();
    QDate cur = QDate::currentDate();
    m_noteDateEdit->setDate(cur);
    m_dayLabel->setText(cur.toString("dddd"));
    m_fyBadge->setText(activeFy.name);

    if (m_controller) {
        m_controller->resetDraft();
        m_noteNoEdit->setText(m_controller->draftNoteNo());
    } else {
        m_noteNoEdit->setText("CN-0001");
    }

    m_origInvoiceNoEdit->clear();
    m_origInvoiceDateEdit->clear();
    m_partySearch->clear();
    m_partyGstinEdit->clear();
    m_narrationEdit->clear();

    m_isUpdatingTable = true;
    m_itemsTable->setRowCount(0);
    m_isUpdatingTable = false;

    onAddItemRow();
    onRecalculateTotals();
}

void DebitCreditNoteWidget::openDateDialog() {
    QDate curDate = m_noteDateEdit->date();
    if (!curDate.isValid()) curDate = QDate::currentDate();

    QDate chosen = VoucherDateDialog::selectDate(this, curDate);
    if (chosen.isValid()) {
        m_noteDateEdit->setDate(chosen);
    }
}

void DebitCreditNoteWidget::onNoteTypeChanged(int index) {
    if (m_controller && m_editNoteId == 0) {
        m_controller->setDraftNoteType(index == 0 ? "Credit Note" : "Debit Note");
        m_noteNoEdit->setText(m_controller->draftNoteNo());
    }
    if (index == 0) {
        m_origInvoiceTypeCombo->setCurrentIndex(0); // Sale Invoice
    } else {
        m_origInvoiceTypeCombo->setCurrentIndex(1); // Purchase Bill
    }
    onRecalculateTotals();
}

void DebitCreditNoteWidget::onPartySelected(const QString& partyName) {
    if (m_controller) {
        m_controller->setDraftPartyName(partyName);
        m_partyGstinEdit->setText(m_controller->draftPartyGstin());
    } else {
        QVariant gstinVar = DatabaseManager::instance().executeScalar("SELECT gstin FROM parties WHERE name = ? LIMIT 1;", {partyName});
        if (gstinVar.isValid()) {
            m_partyGstinEdit->setText(gstinVar.toString().trimmed());
        }
    }

    // Auto update Place of Supply based on party GSTIN
    QString gstin = m_partyGstinEdit->text().trimmed();
    if (gstin.length() >= 2) {
        QString stateCode = gstin.left(2);
        for (int i = 0; i < m_posCombo->count(); ++i) {
            if (m_posCombo->itemText(i).startsWith(stateCode)) {
                m_posCombo->setCurrentIndex(i);
                break;
            }
        }
    }
    onRecalculateTotals();
}

void DebitCreditNoteWidget::onStockItemConfigured(int row, const QVariantMap& itemData) {
    if (row < 0 || row >= m_itemsTable->rowCount()) return;

    m_isUpdatingTable = true;

    QString name = itemData.value("name").toString();
    QString hsn = itemData.value("hsn_code", "1006").toString();
    QString unit = itemData.value("unit", "QTL").toString();
    double gst = itemData.value("gst_rate", 5.0).toDouble();
    double rate = (m_noteTypeCombo->currentIndex() == 0)
        ? itemData.value("sale_rate", 0.0).toDouble()
        : itemData.value("purchase_rate", 0.0).toDouble();

    if (m_itemsTable->item(row, 0)) m_itemsTable->item(row, 0)->setText(name);
    if (m_itemsTable->item(row, 1)) m_itemsTable->item(row, 1)->setText(hsn);
    if (m_itemsTable->item(row, 2)) m_itemsTable->item(row, 2)->setText(unit);
    if (m_itemsTable->item(row, 5) && rate > 0.0) m_itemsTable->item(row, 5)->setText(QString::number(rate, 'f', 2));
    if (m_itemsTable->item(row, 7)) m_itemsTable->item(row, 7)->setText(QString::number(gst, 'f', 1));

    m_isUpdatingTable = false;
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
    m_itemsTable->setItem(r, 1, createCell("1006", Qt::AlignCenter, true)); // HSN
    m_itemsTable->setItem(r, 2, createCell("QTL", Qt::AlignCenter, true)); // Unit
    m_itemsTable->setItem(r, 3, createCell("0", Qt::AlignRight, true)); // Bags
    m_itemsTable->setItem(r, 4, createCell("0.00", Qt::AlignRight, true)); // Weight
    m_itemsTable->setItem(r, 5, createCell("0.00", Qt::AlignRight, true)); // Rate
    m_itemsTable->setItem(r, 6, createCell("0.00", Qt::AlignRight, true)); // Taxable
    m_itemsTable->setItem(r, 7, createCell("5.0", Qt::AlignRight, true)); // GST %
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

bool DebitCreditNoteWidget::loadNoteForEditing(int noteId) {
    if (!m_controller) return false;
    bool ok = m_controller->loadNoteIntoDraft(noteId);
    if (!ok) return false;

    m_editNoteId = noteId;
    m_deleteBtn->setVisible(true);

    m_noteTypeCombo->setCurrentIndex(m_controller->draftNoteType() == "Credit Note" ? 0 : 1);
    m_noteNoEdit->setText(m_controller->draftNoteNo());
    QDate d = QDate::fromString(m_controller->draftNoteDate(), "yyyy-MM-dd");
    if (d.isValid()) {
        m_noteDateEdit->setDate(d);
        m_dayLabel->setText(d.toString("dddd"));
        FiscalYearInfo fy = FiscalYearHelper::getFiscalYearForDate(d.toString("yyyy-MM-dd"));
        m_fyBadge->setText(fy.name);
    }

    m_origInvoiceNoEdit->setText(m_controller->draftOrigInvNo());
    m_origInvoiceDateEdit->setText(m_controller->draftOrigInvDate());
    m_partySearch->setText(m_controller->draftPartyName());
    m_partyGstinEdit->setText(m_controller->draftPartyGstin());
    m_narrationEdit->setText(m_controller->draftNarration());

    m_isUpdatingTable = true;
    m_itemsTable->setRowCount(0);

    auto* mdl = m_controller->itemsModel();
    if (mdl) {
        for (int i = 0; i < mdl->count(); ++i) {
            auto m = mdl->get(i);
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
            m_itemsTable->setItem(r, 7, createCell(QString::number(gst, 'f', 1), Qt::AlignRight, true));
            m_itemsTable->setItem(r, 8, createCell(QString::number(gstAmt, 'f', 2), Qt::AlignRight, false));
            m_itemsTable->setItem(r, 9, createCell(QString::number(tot, 'f', 2), Qt::AlignRight, false));
        }
    }

    m_isUpdatingTable = false;
    onRecalculateTotals();
    return true;
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
        m_controller->setDraftOrigInvNo(m_origInvoiceNoEdit->text().trimmed());
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
            QString hsn = m_itemsTable->item(r, 1) ? m_itemsTable->item(r, 1)->text().trimmed() : "1006";
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
