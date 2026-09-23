#include "modify_ledger_widget.h"
#include "custom_dialogs.h"
#include "../database_manager.h"
#include "../engine/accounting_engine.h"
#include <QKeyEvent>
#include <QMessageBox>
#include <QDoubleValidator>
#include <QIntValidator>
#include <QTimer>
#include <QShortcut>
#include <QCompleter>
#include <QAbstractItemView>
#include <QDesktopServices>
#include <QUrl>
#include <QInputDialog>

ModifyLedgerWidget::ModifyLedgerWidget(QWidget* parent)
    : QWidget(parent)
{
    setupUi();
    populateDropdowns();

    // Global focus shortcuts for search
    auto* altS = new QShortcut(QKeySequence("Alt+S"), this);
    connect(altS, &QShortcut::activated, this, &ModifyLedgerWidget::focusSearch);

    auto* altL = new QShortcut(QKeySequence("Alt+L"), this);
    connect(altL, &QShortcut::activated, this, &ModifyLedgerWidget::focusSearch);

    auto* altP = new QShortcut(QKeySequence("Alt+P"), this);
    connect(altP, &QShortcut::activated, this, &ModifyLedgerWidget::focusSearch);

    auto* altD = new QShortcut(QKeySequence("Alt+D"), this);
    connect(altD, &QShortcut::activated, this, &ModifyLedgerWidget::onDeleteClicked);
}

static QString inputStyle() {
    return "QLineEdit { background-color: #FFFFFF; color: #0F172A; border: 1px solid #CBD5E1; "
           "border-radius: 5px; padding: 3px 7px; font-size: 11px; font-weight: 500; } "
           "QLineEdit:focus { border: 1.5px solid #2563EB; background-color: #F8FAFC; }";
}

static QString comboStyle() {
    return "QComboBox { "
           "  background-color: #FFFFFF; "
           "  color: #0F172A; "
           "  border: 1px solid #CBD5E1; "
           "  border-radius: 5px; "
           "  padding: 2px 18px 2px 6px; "
           "  font-size: 11px; "
           "  font-weight: 600; "
           "} "
           "QComboBox:focus, QComboBox:on { "
           "  border: 1.5px solid #2563EB; "
           "  background-color: #F8FAFC; "
           "} "
           "QComboBox:editable { "
           "  background-color: #FFFFFF; "
           "} "
           "QComboBox::drop-down { "
           "  subcontrol-origin: padding; "
           "  subcontrol-position: top right; "
           "  width: 18px; "
           "  border-left: 1px solid #CBD5E1; "
           "  border-top-right-radius: 4px; "
           "  border-bottom-right-radius: 4px; "
           "  background-color: #F1F5F9; "
           "} "
           "QComboBox::drop-down:hover { "
           "  background-color: #E2E8F0; "
           "} "
           "QComboBox::down-arrow { "
           "  width: 0px; "
           "  height: 0px; "
           "  border-left: 3.5px solid transparent; "
           "  border-right: 3.5px solid transparent; "
           "  border-top: 4.5px solid #475569; "
           "  margin-right: 2px; "
           "}";
}

static QString comboPopupStyle() {
    return "QListView { "
           "  background-color: #FFFFFF; "
           "  color: #0F172A; "
           "  border: 1px solid #CBD5E1; "
           "  border-radius: 5px; "
           "  padding: 3px; "
           "  outline: none; "
           "  font-size: 11px; "
           "  font-weight: 600; "
           "} "
           "QListView::item { "
           "  min-height: 22px; "
           "  padding: 3px 6px; "
           "  border-radius: 3px; "
           "} "
           "QListView::item:hover { "
           "  background-color: #EFF6FF; "
           "  color: #1D4ED8; "
           "} "
           "QListView::item:selected { "
           "  background-color: #2563EB; "
           "  color: #FFFFFF; "
           "}";
}

static QString checkStyle() {
    return "QCheckBox { color: #334155; font-size: 11px; font-weight: 600; spacing: 6px; } "
           "QCheckBox::indicator { width: 14px; height: 14px; border: 1.5px solid #94A3B8; border-radius: 3px; background: #FFFFFF; } "
           "QCheckBox::indicator:hover { border-color: #2563EB; } "
           "QCheckBox::indicator:checked { background-color: #2563EB; border-color: #1D4ED8; }";
}

void ModifyLedgerWidget::setupSearchableCombo(QComboBox* combo, const QStringList& items) {
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
        combo->lineEdit()->setStyleSheet("QLineEdit { background: transparent; border: none; padding: 0px 2px; font-size: 11px; font-weight: 600; color: #0F172A; }");
        combo->lineEdit()->installEventFilter(this);
    }
}

void ModifyLedgerWidget::setComboText(QComboBox* combo, const QString& text) {
    if (!combo) return;
    int idx = combo->findText(text.trimmed(), Qt::MatchFixedString);
    if (idx >= 0) {
        combo->setCurrentIndex(idx);
    } else {
        combo->setEditText(text.trimmed());
    }
}

QString ModifyLedgerWidget::getComboText(const QComboBox* combo) const {
    if (!combo) return "";
    return combo->currentText().trimmed();
}

void ModifyLedgerWidget::setupUi() {
    setAttribute(Qt::WA_StyledBackground, true);
    setStyleSheet("ModifyLedgerWidget { background-color: #F8FAFC; }");

    auto* rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(12, 6, 12, 6);
    rootLayout->setSpacing(5);

    // 1. TOP HEADER & TRIAL BALANCE SUMMARY
    rootLayout->addWidget(createHeaderSection());

    // 2. SEARCH STRIP
    rootLayout->addWidget(createSearchCard());

    // 3. UNIFIED SINGLE SLATE
    rootLayout->addWidget(createSlateSection(), 1);

    // 4. BOTTOM ACTION BAR
    auto* bottomCard = new QFrame(this);
    bottomCard->setFixedHeight(42);
    bottomCard->setStyleSheet("background-color: #FFFFFF; border: 1px solid #E2E8F0; border-radius: 8px;");
    auto* bottomLayout = new QHBoxLayout(bottomCard);
    bottomLayout->setContentsMargins(12, 0, 12, 0);
    bottomLayout->setSpacing(10);

    m_cancelBtn = new QPushButton("Cancel (Esc)", bottomCard);
    m_cancelBtn->setFixedSize(100, 30);
    m_cancelBtn->setCursor(Qt::PointingHandCursor);
    m_cancelBtn->setStyleSheet("QPushButton { background-color: #F1F5F9; color: #475569; border: 1px solid #CBD5E1; border-radius: 5px; font-weight: 700; font-size: 11px; } QPushButton:hover { background-color: #E2E8F0; }");
    connect(m_cancelBtn, &QPushButton::clicked, this, &ModifyLedgerWidget::backRequested);
    bottomLayout->addWidget(m_cancelBtn);

    m_deleteBtn = new QPushButton("Delete Ledger (Alt+D)", bottomCard);
    m_deleteBtn->setFixedSize(150, 30);
    m_deleteBtn->setCursor(Qt::PointingHandCursor);
    m_deleteBtn->setStyleSheet("QPushButton { background-color: #FEE2E2; color: #DC2626; border: 1px solid #FCA5A5; border-radius: 5px; font-weight: 700; font-size: 11px; } QPushButton:hover { background-color: #FECACA; }");
    connect(m_deleteBtn, &QPushButton::clicked, this, &ModifyLedgerWidget::onDeleteClicked);
    bottomLayout->addWidget(m_deleteBtn);

    bottomLayout->addStretch();

    m_updateBtn = new QPushButton("Update Complete Ledger Account (Enter)", bottomCard);
    m_updateBtn->setFixedSize(270, 30);
    m_updateBtn->setCursor(Qt::PointingHandCursor);
    m_updateBtn->setStyleSheet("QPushButton { background-color: #059669; color: #FFFFFF; border: 1px solid #047857; border-radius: 5px; font-weight: 800; font-size: 12px; } QPushButton:hover { background-color: #047857; } QPushButton:focus { border: 2px solid #6EE7B7; }");
    connect(m_updateBtn, &QPushButton::clicked, this, &ModifyLedgerWidget::onUpdateClicked);
    bottomLayout->addWidget(m_updateBtn);

    rootLayout->addWidget(bottomCard);

    // Keyboard Navigation Order
    m_navOrder = {
        m_prefixCombo,
        m_nameInput,
        m_aliasInput,
        m_groupCombo,
        m_stationCombo,
        m_stateCombo,
        m_stateCodeInput,
        m_pincodeInput,
        m_useRoutesCheck,
        m_routeCombo,
        m_booksFromInput,
        m_opBalInput,
        m_balTypeCombo,
        m_openFromInput,
        m_gstinInput,
        m_gstPartyTypeCombo,
        m_panInput,
        m_tinInput,
        m_urnInput,
        m_addressInput,
        m_phoneInput,
        m_whatsappInput,
        m_emailInput,
        m_aadhaarInput,
        m_bankAccountInput,
        m_ifscInput,
        m_bankNameInput,
        m_shopNoInput,
        m_creditLimitInput,
        m_applyTcsCheck,
        m_tcsExemptCheck,
        m_stockNotCalcCheck,
        m_calcDirectExpenseCheck,
        m_showDateTotalsCheck,
        m_useCreditLimitCheck,
        m_setTitleCaseCheck,
        m_updateBtn
    };

    for (QWidget* w : m_navOrder) {
        if (w) {
            w->installEventFilter(this);
        }
    }

    // Auto-formatting and reactive triggers
    connect(m_nameInput, &QLineEdit::editingFinished, this, &ModifyLedgerWidget::onNameEditingFinished);
    connect(m_opBalInput, &QLineEdit::textChanged, this, &ModifyLedgerWidget::updateTotalOpeningBalDisplay);
    connect(m_balTypeCombo, &QComboBox::currentTextChanged, this, &ModifyLedgerWidget::updateTotalOpeningBalDisplay);
    connect(m_gstinInput, &QLineEdit::textChanged, this, &ModifyLedgerWidget::onGstinChanged);
    connect(m_stateCombo, &QComboBox::currentTextChanged, this, &ModifyLedgerWidget::onStateChanged);

    // Group Selection -> Parent Group Hint
    connect(m_groupCombo, &QComboBox::currentTextChanged, this, [this](const QString& gText) {
        QString gt = gText.trimmed().toLower();
        if (gt.contains("debtor") || gt.contains("buyer") || gt.contains("customer")) {
            m_parentGroupLbl->setText("(Sundry Debtors)");
        } else if (gt.contains("creditor") || gt.contains("supplier") || gt.contains("vendor")) {
            m_parentGroupLbl->setText("(Sundry Creditors)");
        } else if (gt.contains("bank")) {
            m_parentGroupLbl->setText("(Bank Accounts)");
        } else if (gt.contains("expense")) {
            m_parentGroupLbl->setText("(Direct/Indirect Expenses)");
        } else if (gt.contains("income")) {
            m_parentGroupLbl->setText("(Direct/Indirect Incomes)");
        } else {
            m_parentGroupLbl->setText(QString("(%1)").arg(gText.trimmed()));
        }
    });
}

QWidget* ModifyLedgerWidget::createHeaderSection() {
    auto* card = new QFrame(this);
    card->setFixedHeight(44);
    card->setStyleSheet("background-color: #0F172A; border: 1px solid #1E293B; border-radius: 8px;");
    auto* layout = new QHBoxLayout(card);
    layout->setContentsMargins(12, 2, 12, 2);
    layout->setSpacing(10);

    // Left: Prefix + Create New
    auto* pfxRow = new QHBoxLayout();
    pfxRow->setSpacing(4);
    auto* pfxTitle = new QLabel("Prefix :", card);
    pfxTitle->setStyleSheet("color: #94A3B8; font-size: 10px; font-weight: 700; border: none; background: transparent;");
    pfxRow->addWidget(pfxTitle);

    m_prefixCombo = new QComboBox(card);
    m_prefixCombo->setFixedHeight(26);
    m_prefixCombo->setFixedWidth(80);
    pfxRow->addWidget(m_prefixCombo);

    m_createPrefixBtn = new QPushButton("(Alt+C : Create New)", card);
    m_createPrefixBtn->setStyleSheet("QPushButton { background: transparent; color: #60A5FA; font-size: 10px; font-weight: 700; border: none; padding: 0px 2px; text-decoration: underline; } QPushButton:hover { color: #93C5FD; }");
    m_createPrefixBtn->setCursor(Qt::PointingHandCursor);
    connect(m_createPrefixBtn, &QPushButton::clicked, this, [this]() {
        bool ok = false;
        QString p = QInputDialog::getText(this, "New Prefix", "Enter title prefix (e.g. M/s, Sh., Dr.):", QLineEdit::Normal, "", &ok);
        if (ok && !p.trimmed().isEmpty()) {
            m_prefixCombo->addItem(p.trimmed());
            m_prefixCombo->setCurrentText(p.trimmed());
        }
    });
    pfxRow->addWidget(m_createPrefixBtn);
    layout->addLayout(pfxRow);

    layout->addStretch(1);

    // Center: Banner Title
    auto* titleLbl = new QLabel("Ledger Alteration / Modification", card);
    titleLbl->setStyleSheet("color: #FFFFFF; font-size: 15px; font-weight: 800; letter-spacing: 0.5px; border: none; background: transparent;");
    layout->addWidget(titleLbl, 0, Qt::AlignCenter);

    layout->addStretch(1);

    // Right: Total Opening Bal. Monitor
    auto* balCard = new QFrame(card);
    balCard->setFixedHeight(36);
    balCard->setFixedWidth(270);
    balCard->setStyleSheet("background-color: #1E293B; border: 1px solid #334155; border-radius: 5px;");
    auto* balLayout = new QVBoxLayout(balCard);
    balLayout->setContentsMargins(6, 2, 6, 2);
    balLayout->setSpacing(0);

    auto* drCrRow = new QHBoxLayout();
    drCrRow->setSpacing(4);
    auto* totLbl = new QLabel("OPENING BAL:", balCard);
    totLbl->setStyleSheet("color: #F87171; font-size: 9px; font-weight: 800; border: none; background: transparent;");
    m_totalDrLbl = new QLabel("Dr. 0.00", balCard);
    m_totalDrLbl->setStyleSheet("color: #F8FAFC; font-size: 10px; font-weight: 700; border: none; background: transparent;");
    m_totalCrLbl = new QLabel("Cr. 0.00", balCard);
    m_totalCrLbl->setStyleSheet("color: #F8FAFC; font-size: 10px; font-weight: 700; border: none; background: transparent;");
    m_totalCrLbl->setAlignment(Qt::AlignRight);
    drCrRow->addWidget(totLbl);
    drCrRow->addWidget(m_totalDrLbl);
    drCrRow->addWidget(m_totalCrLbl);
    balLayout->addLayout(drCrRow);

    m_diffBalLbl = new QLabel("Diff. 0.00 (Balanced)", balCard);
    m_diffBalLbl->setStyleSheet("color: #4ADE80; font-size: 9px; font-weight: 800; border: none; background: transparent;");
    m_diffBalLbl->setAlignment(Qt::AlignCenter);
    balLayout->addWidget(m_diffBalLbl);

    layout->addWidget(balCard);

    return card;
}

QWidget* ModifyLedgerWidget::createSearchCard() {
    auto* card = new QFrame(this);
    card->setFixedHeight(36);
    card->setStyleSheet("QFrame { background-color: #FFFFFF; border: 1px solid #CBD5E1; border-radius: 6px; }");
    auto* layout = new QHBoxLayout(card);
    layout->setContentsMargins(10, 2, 10, 2);
    layout->setSpacing(8);

    auto* searchTitle = new QLabel("SELECT PARTY TO MODIFY (Alt+S) :", card);
    searchTitle->setStyleSheet("color: #1D4ED8; font-size: 10px; font-weight: 800; border: none;");
    layout->addWidget(searchTitle);

    m_partySearchWidget = new AccountSearchBox(card);
    m_partySearchWidget->setPlaceholderText("Start typing party or ledger name...");
    connect(m_partySearchWidget, &AccountSearchBox::partyDataSelected, this, &ModifyLedgerWidget::onPartySelected);
    layout->addWidget(m_partySearchWidget, 1);

    auto* hintLbl = new QLabel("(↑/↓ Arrows & Enter)", card);
    hintLbl->setStyleSheet("color: #64748B; font-size: 10px; font-style: italic; border: none;");
    layout->addWidget(hintLbl);

    return card;
}

QWidget* ModifyLedgerWidget::createSlateSection() {
    auto* card = new QFrame(this);
    card->setStyleSheet("QFrame { background-color: #FFFFFF; border: 1px solid #E2E8F0; border-radius: 8px; }");
    auto* layout = new QVBoxLayout(card);
    layout->setContentsMargins(14, 8, 14, 8);
    layout->setSpacing(6);

    // ==========================================
    // 1. IDENTITY & OPENING BALANCE
    // ==========================================
    auto* sec1Lbl = new QLabel("1. Identity & Opening Balance", card);
    sec1Lbl->setStyleSheet("color: #1E293B; font-size: 11px; font-weight: 800; border: none;");
    layout->addWidget(sec1Lbl);

    // Row 1: Name, Alias, Under (Group)
    auto* row1 = new QHBoxLayout();
    row1->setSpacing(8);

    auto* nameBox = new QVBoxLayout();
    nameBox->setSpacing(1);
    auto* nameLbl = new QLabel("Ledger Name : *", card);
    nameLbl->setStyleSheet("color: #0F172A; font-size: 10px; font-weight: 700; border: none;");
    m_nameInput = new QLineEdit(card);
    m_nameInput->setPlaceholderText("e.g. KRISHNA FOODS");
    m_nameInput->setStyleSheet(inputStyle());
    m_nameInput->setFixedHeight(26);
    nameBox->addWidget(nameLbl);
    nameBox->addWidget(m_nameInput);
    row1->addLayout(nameBox, 3);

    auto* aliasBox = new QVBoxLayout();
    aliasBox->setSpacing(1);
    auto* aliasLbl = new QLabel("(In Short) / Alias :", card);
    aliasLbl->setStyleSheet("color: #475569; font-size: 10px; font-weight: 600; border: none;");
    m_aliasInput = new QLineEdit(card);
    m_aliasInput->setPlaceholderText("Alias / Code");
    m_aliasInput->setStyleSheet(inputStyle());
    m_aliasInput->setFixedHeight(26);
    aliasBox->addWidget(aliasLbl);
    aliasBox->addWidget(m_aliasInput);
    row1->addLayout(aliasBox, 1);

    auto* underBox = new QVBoxLayout();
    underBox->setSpacing(1);
    auto* underLbl = new QLabel("Under (Group) : *", card);
    underLbl->setStyleSheet("color: #0F172A; font-size: 10px; font-weight: 700; border: none;");
    auto* underRow = new QHBoxLayout();
    underRow->setSpacing(4);
    m_groupCombo = new QComboBox(card);
    m_groupCombo->setFixedHeight(26);
    underRow->addWidget(m_groupCombo, 2);

    m_parentGroupLbl = new QLabel("(Sundry Debtors)", card);
    m_parentGroupLbl->setStyleSheet("color: #2563EB; font-size: 10px; font-weight: 800; border: none;");
    underRow->addWidget(m_parentGroupLbl, 1);
    underBox->addWidget(underLbl);
    underBox->addLayout(underRow);
    row1->addLayout(underBox, 2);

    layout->addLayout(row1);

    // Row 2: Station, State, State Code, PIN Code, [ ] Use Route
    auto* row2 = new QHBoxLayout();
    row2->setSpacing(8);

    auto* stBox = new QVBoxLayout();
    stBox->setSpacing(1);
    auto* stLbl = new QLabel("Station :", card);
    stLbl->setStyleSheet("color: #0F172A; font-size: 10px; font-weight: 700; border: none;");
    m_stationCombo = new QComboBox(card);
    m_stationCombo->setFixedHeight(26);
    stBox->addWidget(stLbl);
    stBox->addWidget(m_stationCombo);
    row2->addLayout(stBox, 2);

    auto* stateBox = new QVBoxLayout();
    stateBox->setSpacing(1);
    auto* stateLbl = new QLabel("State :", card);
    stateLbl->setStyleSheet("color: #0F172A; font-size: 10px; font-weight: 700; border: none;");
    m_stateCombo = new QComboBox(card);
    m_stateCombo->setFixedHeight(26);
    stateBox->addWidget(stateLbl);
    stateBox->addWidget(m_stateCombo);
    row2->addLayout(stateBox, 2);

    auto* scBox = new QVBoxLayout();
    scBox->setSpacing(1);
    auto* scLbl = new QLabel("State Code :", card);
    scLbl->setStyleSheet("color: #475569; font-size: 10px; font-weight: 600; border: none;");
    m_stateCodeInput = new QLineEdit(card);
    m_stateCodeInput->setPlaceholderText("06");
    m_stateCodeInput->setStyleSheet(inputStyle());
    m_stateCodeInput->setFixedHeight(26);
    m_stateCodeInput->setFixedWidth(65);
    scBox->addWidget(scLbl);
    scBox->addWidget(m_stateCodeInput);
    row2->addLayout(scBox);

    auto* pinBox = new QVBoxLayout();
    pinBox->setSpacing(1);
    auto* pinLbl = new QLabel("PIN Code :", card);
    pinLbl->setStyleSheet("color: #475569; font-size: 10px; font-weight: 600; border: none;");
    m_pincodeInput = new QLineEdit(card);
    m_pincodeInput->setPlaceholderText("125055");
    m_pincodeInput->setStyleSheet(inputStyle());
    m_pincodeInput->setFixedHeight(26);
    m_pincodeInput->setFixedWidth(85);
    pinBox->addWidget(pinLbl);
    pinBox->addWidget(m_pincodeInput);
    row2->addLayout(pinBox);

    m_routeContainer = new QWidget(card);
    auto* rcLayout = new QHBoxLayout(m_routeContainer);
    rcLayout->setContentsMargins(0, 0, 0, 0);
    rcLayout->setSpacing(4);

    m_useRoutesCheck = new QCheckBox("Use Route", m_routeContainer);
    m_useRoutesCheck->setStyleSheet(checkStyle());
    rcLayout->addWidget(m_useRoutesCheck);

    m_routeCombo = new QComboBox(m_routeContainer);
    m_routeCombo->setFixedHeight(26);
    m_routeCombo->setFixedWidth(120);
    m_routeCombo->setEnabled(false);
    rcLayout->addWidget(m_routeCombo);

    connect(m_useRoutesCheck, &QCheckBox::toggled, this, [this](bool checked) {
        m_routeCombo->setEnabled(checked);
    });

    row2->addWidget(m_routeContainer);
    layout->addLayout(row2);

    // Row 3: Books Start From, Opening Balance, Type (Dr/Cr), Ledger Open Always From
    auto* row3 = new QHBoxLayout();
    row3->setSpacing(8);

    auto* booksBox = new QVBoxLayout();
    booksBox->setSpacing(1);
    auto* booksLbl = new QLabel("Books Start From (DD-MM-YYYY) :", card);
    booksLbl->setStyleSheet("color: #0F172A; font-size: 10px; font-weight: 700; border: none;");
    m_booksFromInput = new QLineEdit(card);
    m_booksFromInput->setPlaceholderText("01-04-2023");
    m_booksFromInput->setStyleSheet(inputStyle());
    m_booksFromInput->setFixedHeight(26);
    booksBox->addWidget(booksLbl);
    booksBox->addWidget(m_booksFromInput);
    row3->addLayout(booksBox, 2);

    auto* opBox = new QVBoxLayout();
    opBox->setSpacing(1);
    auto* opLbl = new QLabel("Opening Balance (₹) :", card);
    opLbl->setStyleSheet("color: #0F172A; font-size: 10px; font-weight: 700; border: none;");
    auto* opRow = new QHBoxLayout();
    opRow->setSpacing(4);
    m_opBalInput = new QLineEdit(card);
    m_opBalInput->setPlaceholderText("0.00");
    m_opBalInput->setAlignment(Qt::AlignRight);
    m_opBalInput->setValidator(new QDoubleValidator(0.0, 999999999.0, 2, this));
    m_opBalInput->setStyleSheet(inputStyle());
    m_opBalInput->setFixedHeight(26);
    opRow->addWidget(m_opBalInput, 2);

    m_balTypeCombo = new QComboBox(card);
    m_balTypeCombo->addItems({"Dr", "Cr"});
    m_balTypeCombo->setStyleSheet(comboStyle());
    m_balTypeCombo->setFixedHeight(26);
    m_balTypeCombo->setFixedWidth(55);
    opRow->addWidget(m_balTypeCombo);

    opBox->addWidget(opLbl);
    opBox->addLayout(opRow);
    row3->addLayout(opBox, 2);

    auto* openFromBox = new QVBoxLayout();
    openFromBox->setSpacing(1);
    auto* openFromLbl = new QLabel("Ledger Open Always From :", card);
    openFromLbl->setStyleSheet("color: #475569; font-size: 10px; font-weight: 600; border: none;");
    m_openFromInput = new QLineEdit(card);
    m_openFromInput->setPlaceholderText("e.g. Sales A/c, Head Office");
    m_openFromInput->setStyleSheet(inputStyle());
    m_openFromInput->setFixedHeight(26);
    openFromBox->addWidget(openFromLbl);
    openFromBox->addWidget(m_openFromInput);
    row3->addLayout(openFromBox, 2);

    layout->addLayout(row3);

    // ==========================================
    // DIVIDER 1
    // ==========================================
    auto* div1 = new QFrame(card);
    div1->setFixedHeight(1);
    div1->setStyleSheet("background-color: #F1F5F9; border: none; margin: 1px 0px;");
    layout->addWidget(div1);

    // ==========================================
    // 2. STATUTORY, CONTACT & BANKING DETAILS
    // ==========================================
    auto* sec2Lbl = new QLabel("2. Statutory, Contact & Banking Details", card);
    sec2Lbl->setStyleSheet("color: #1E293B; font-size: 11px; font-weight: 800; border: none;");
    layout->addWidget(sec2Lbl);

    auto* gridLayout = new QHBoxLayout();
    gridLayout->setSpacing(12);

    auto createFormField = [](QWidget* parent, const QString& label, QLineEdit*& edit, const QString& placeholder) -> QVBoxLayout* {
        auto* box = new QVBoxLayout();
        box->setSpacing(1);
        auto* lbl = new QLabel(label, parent);
        lbl->setStyleSheet("color: #334155; font-size: 10px; font-weight: 600; border: none;");
        edit = new QLineEdit(parent);
        edit->setPlaceholderText(placeholder);
        edit->setStyleSheet(inputStyle());
        edit->setFixedHeight(25);
        box->addWidget(lbl);
        box->addWidget(edit);
        return box;
    };

    // COLUMN 1: GST & Tax Identification
    auto* col1 = new QVBoxLayout();
    col1->setSpacing(3);

    col1->addLayout(createFormField(card, "GSTIN :", m_gstinInput, "06AJAPP3837B1ZK"));

    auto* ptBox = new QVBoxLayout();
    ptBox->setSpacing(1);
    auto* ptLbl = new QLabel("Party Type (GST) :", card);
    ptLbl->setStyleSheet("color: #334155; font-size: 10px; font-weight: 600; border: none;");
    m_gstPartyTypeCombo = new QComboBox(card);
    m_gstPartyTypeCombo->setFixedHeight(25);
    ptBox->addWidget(ptLbl);
    ptBox->addWidget(m_gstPartyTypeCombo);
    col1->addLayout(ptBox);

    col1->addLayout(createFormField(card, "Party PAN :", m_panInput, "AJAPP3837B"));
    col1->addLayout(createFormField(card, "Party TIN / VAT :", m_tinInput, "Sales Tax / VAT TIN"));
    col1->addLayout(createFormField(card, "Udyam Reg. (URN) :", m_urnInput, "UDYAM-XX-00-0000000"));
    gridLayout->addLayout(col1, 1);

    // COLUMN 2: Address & Contact Details
    auto* col2 = new QVBoxLayout();
    col2->setSpacing(3);

    col2->addLayout(createFormField(card, "Address :", m_addressInput, "Shop / Street / Mandi Address"));
    col2->addLayout(createFormField(card, "Phone / Mobile :", m_phoneInput, "08532-234567, 98765..."));
    col2->addLayout(createFormField(card, "Whatsapp No. :", m_whatsappInput, "9876543210"));
    col2->addLayout(createFormField(card, "E-mail ID :", m_emailInput, "accounts@party.com"));
    col2->addLayout(createFormField(card, "Aadhar No. :", m_aadhaarInput, "12-digit Aadhaar"));
    gridLayout->addLayout(col2, 1);

    // COLUMN 3: Banking & Credit Terms
    auto* col3 = new QVBoxLayout();
    col3->setSpacing(3);

    col3->addLayout(createFormField(card, "Bank A/c No. :", m_bankAccountInput, "Account Number"));
    col3->addLayout(createFormField(card, "IFSC Code :", m_ifscInput, "CNRB0002058"));
    col3->addLayout(createFormField(card, "Bank Name :", m_bankNameInput, "HDFC / Canara Bank"));
    col3->addLayout(createFormField(card, "Shop / Premise No. :", m_shopNoInput, "Shop #"));
    col3->addLayout(createFormField(card, "Credit Limit (₹) :", m_creditLimitInput, "0.00"));
    gridLayout->addLayout(col3, 1);

    layout->addLayout(gridLayout);

    // ==========================================
    // DIVIDER 2
    // ==========================================
    auto* div2 = new QFrame(card);
    div2->setFixedHeight(1);
    div2->setStyleSheet("background-color: #F1F5F9; border: none; margin: 1px 0px;");
    layout->addWidget(div2);

    // ==========================================
    // 3. ACCOUNTING & BUSINESS RULES
    // ==========================================
    auto* sec3Lbl = new QLabel("3. Accounting & Business Rules", card);
    sec3Lbl->setStyleSheet("color: #1E293B; font-size: 11px; font-weight: 800; border: none;");
    layout->addWidget(sec3Lbl);

    auto* rulesLayout = new QHBoxLayout();
    rulesLayout->setSpacing(12);

    // Col 1: TCS rules
    auto* rCol1 = new QVBoxLayout();
    rCol1->setSpacing(3);
    m_applyTcsCheck = new QCheckBox("Apply TCS on Purchases (u/s 206C)", card);
    m_applyTcsCheck->setStyleSheet(checkStyle());
    rCol1->addWidget(m_applyTcsCheck);

    m_tcsExemptCheck = new QCheckBox("TCS Not Applicable in Sales", card);
    m_tcsExemptCheck->setStyleSheet(checkStyle());
    rCol1->addWidget(m_tcsExemptCheck);
    rulesLayout->addLayout(rCol1, 1);

    // Col 2: Ledger calculation behaviors
    auto* rCol2 = new QVBoxLayout();
    rCol2->setSpacing(3);
    m_stockNotCalcCheck = new QCheckBox("Stock Not Calculate in Ledger", card);
    m_stockNotCalcCheck->setStyleSheet(checkStyle());
    rCol2->addWidget(m_stockNotCalcCheck);

    m_calcDirectExpenseCheck = new QCheckBox("Calculate as Direct Expenses", card);
    m_calcDirectExpenseCheck->setStyleSheet(checkStyle());
    rCol2->addWidget(m_calcDirectExpenseCheck);

    m_showDateTotalsCheck = new QCheckBox("Show Date Totals in Ledger (Alt+T)", card);
    m_showDateTotalsCheck->setStyleSheet(checkStyle());
    rCol2->addWidget(m_showDateTotalsCheck);
    rulesLayout->addLayout(rCol2, 1);

    // Col 3: Credit limit & casing
    auto* rCol3 = new QVBoxLayout();
    rCol3->setSpacing(3);
    m_useCreditLimitCheck = new QCheckBox("Use Credit Limit for Ledgers", card);
    m_useCreditLimitCheck->setChecked(true);
    m_useCreditLimitCheck->setStyleSheet(checkStyle());
    rCol3->addWidget(m_useCreditLimitCheck);

    m_setTitleCaseCheck = new QCheckBox("Auto-format Name in Title Case", card);
    m_setTitleCaseCheck->setChecked(true);
    m_setTitleCaseCheck->setStyleSheet(checkStyle());
    rCol3->addWidget(m_setTitleCaseCheck);
    rulesLayout->addLayout(rCol3, 1);

    layout->addLayout(rulesLayout);

    return card;
}

void ModifyLedgerWidget::populateDropdowns() {
    setupSearchableCombo(m_prefixCombo, m_partiesModel.get_prefixes());
    setupSearchableCombo(m_stationCombo, m_partiesModel.get_stations());
    setupSearchableCombo(m_stateCombo, m_partiesModel.get_states());
    setupSearchableCombo(m_routeCombo, m_partiesModel.get_routes());
    setupSearchableCombo(m_groupCombo, m_partiesModel.get_account_groups());
    setupSearchableCombo(m_gstPartyTypeCombo, m_partiesModel.get_gst_party_types());
}

void ModifyLedgerWidget::resetForm() {
    populateDropdowns();
    m_selectedPartyId = -1;

    QVariantList compRows = DatabaseManager::instance().executeQuery("SELECT state, state_code, pincode FROM company_info LIMIT 1;");
    QString defState = "";
    QString defStateCode = "";
    QString defPincode = "";
    if (!compRows.isEmpty()) {
        QVariantMap c = compRows.first().toMap();
        defState = c.value("state").toString().trimmed();
        defStateCode = c.value("state_code").toString().trimmed();
        defPincode = c.value("pincode").toString().trimmed();
    }

    m_partySearchWidget->clear();
    setComboText(m_prefixCombo, "None");
    m_nameInput->clear();
    m_aliasInput->clear();
    setComboText(m_stationCombo, "");
    setComboText(m_stateCombo, defState);
    m_stateCodeInput->setText(defStateCode.isEmpty() && !defState.isEmpty() ? m_partiesModel.get_state_code_for_state(defState) : defStateCode);
    m_pincodeInput->setText(defPincode);
    m_useRoutesCheck->setChecked(false);
    setComboText(m_routeCombo, "None");

    setComboText(m_groupCombo, "");
    m_parentGroupLbl->setText("");

    QString fyStart = m_partiesModel.get_financial_year_start();
    m_booksFromInput->setText(fyStart);
    m_opBalInput->clear();
    m_balTypeCombo->setCurrentIndex(0);

    m_openFromInput->clear();
    m_gstinInput->clear();
    setComboText(m_gstPartyTypeCombo, "Normal Dealer");
    m_panInput->clear();
    m_tinInput->clear();
    m_urnInput->clear();

    m_addressInput->clear();
    m_phoneInput->clear();
    m_whatsappInput->clear();
    m_emailInput->clear();
    m_aadhaarInput->clear();

    m_bankAccountInput->clear();
    m_ifscInput->clear();
    m_bankNameInput->clear();
    m_shopNoInput->clear();
    m_creditLimitInput->clear();

    m_applyTcsCheck->setChecked(false);
    m_tcsExemptCheck->setChecked(false);
    m_stockNotCalcCheck->setChecked(false);
    m_calcDirectExpenseCheck->setChecked(false);
    m_showDateTotalsCheck->setChecked(false);
    m_useCreditLimitCheck->setChecked(true);
    m_setTitleCaseCheck->setChecked(true);

    updateTotalOpeningBalDisplay();
    focusSearch();
}

void ModifyLedgerWidget::updateTotalOpeningBalDisplay() {
    double amt = m_opBalInput ? m_opBalInput->text().trimmed().toDouble() : 0.0;
    QString bType = m_balTypeCombo ? m_balTypeCombo->currentText().trimmed() : "Dr";

    QVariantMap summary = m_partiesModel.get_total_opening_balance_summary(m_selectedPartyId, amt, bType);
    double dr = summary.value("total_dr").toDouble();
    double cr = summary.value("total_cr").toDouble();
    double diff = summary.value("diff").toDouble();
    QString diffType = summary.value("diff_type").toString();

    if (m_totalDrLbl) m_totalDrLbl->setText(QString("Dr. %1").arg(AccountingEngine::formatIndianCurrency(dr, false)));
    if (m_totalCrLbl) m_totalCrLbl->setText(QString("Cr. %1").arg(AccountingEngine::formatIndianCurrency(cr, false)));

    if (m_diffBalLbl) {
        if (diffType == "Balanced" || diff <= 0.001) {
            m_diffBalLbl->setText("Diff. 0.00 (Balanced)");
            m_diffBalLbl->setStyleSheet("color: #4ADE80; font-size: 10px; font-weight: 800; border: none; background: transparent;");
        } else {
            m_diffBalLbl->setText(QString("Diff. %1 %2").arg(AccountingEngine::formatIndianCurrency(diff, false), diffType));
            m_diffBalLbl->setStyleSheet("color: #38BDF8; font-size: 10px; font-weight: 800; border: none; background: transparent;");
        }
    }
}

void ModifyLedgerWidget::onNameEditingFinished() {
    if (!m_setTitleCaseCheck || !m_setTitleCaseCheck->isChecked()) return;
    QString cur = m_nameInput->text().trimmed();
    if (cur.isEmpty()) return;

    QString result;
    bool inBracket = false;
    bool newWord = true;

    for (int i = 0; i < cur.length(); ++i) {
        QChar c = cur.at(i);
        if (c == '[') {
            inBracket = true;
            newWord = true;
            result.append(c);
        } else if (c == ']') {
            inBracket = false;
            newWord = true;
            result.append(c);
        } else if (c.isSpace() || c == '.' || c == '-' || c == '/' || c == '&' || c == '(' || c == ')') {
            newWord = true;
            result.append(c);
        } else {
            if (newWord) {
                result.append(c.toUpper());
                newWord = false;
            } else {
                result.append(c.toLower());
            }
        }
    }

    if (result != cur) {
        m_nameInput->setText(result);
    }
}

void ModifyLedgerWidget::onGstinChanged(const QString& text) {
    QString clean = text.trimmed().toUpper();
    if (clean.length() >= 2 && clean.at(0).isDigit() && clean.at(1).isDigit()) {
        QString sc = clean.left(2);
        m_stateCodeInput->setText(sc);
        QString st = m_partiesModel.get_state_for_gstin(clean);
        if (!st.isEmpty()) {
            setComboText(m_stateCombo, st);
        }
    }
    if (clean.length() >= 10) {
        QString extractedPan = clean.mid(2, 10);
        if (m_panInput->text().isEmpty()) {
            m_panInput->setText(extractedPan);
        }
        setComboText(m_gstPartyTypeCombo, "Normal Dealer");
    }
}

void ModifyLedgerWidget::onStateChanged(const QString& stateName) {
    if (stateName.trimmed().isEmpty()) return;
    QString sc = m_partiesModel.get_state_code_for_state(stateName);
    if (!sc.isEmpty() && m_stateCodeInput) {
        m_stateCodeInput->setText(sc);
    }
}

void ModifyLedgerWidget::focusSearch() {
    if (m_partySearchWidget) {
        m_partySearchWidget->setFocus();
        m_partySearchWidget->selectAll();
        m_partySearchWidget->openSearchPopup();
    }
}

void ModifyLedgerWidget::showEvent(QShowEvent* event) {
    QWidget::showEvent(event);
    updateTotalOpeningBalDisplay();
    QTimer::singleShot(50, this, [this]() {
        focusSearch();
    });
}

void ModifyLedgerWidget::onPartySelected(const QVariantMap& partyData) {
    if (partyData.isEmpty()) return;

    int partyId = partyData.value("id").toInt();
    if (partyId <= 0) {
        QString name = partyData.value("name").toString();
        loadPartyByName(name);
    } else {
        loadParty(partyId);
    }
}

void ModifyLedgerWidget::loadParty(int partyId) {
    if (partyId <= 0) return;
    QVariantMap p = m_partiesModel.get_party_by_id(partyId);
    if (p.isEmpty()) return;

    m_selectedPartyId = partyId;
    m_partySearchWidget->setPartyName(p.value("name").toString());

    setComboText(m_prefixCombo, p.value("prefix", "None").toString());
    m_nameInput->setText(p.value("name").toString());
    m_aliasInput->setText(p.value("alias").toString());

    // Single Station resolution
    QString pStation = p.value("party_station").toString();
    if (pStation.isEmpty()) pStation = p.value("city").toString();
    setComboText(m_stationCombo, pStation);

    // State & PIN Code
    QString state = p.value("state").toString();
    if (state.isEmpty()) {
        QVariant v = DatabaseManager::instance().executeScalar("SELECT state FROM company_info LIMIT 1;");
        if (v.isValid() && !v.isNull()) state = v.toString().trimmed();
    }
    setComboText(m_stateCombo, state);
    QString stateCode = p.value("state_code").toString();
    if (stateCode.isEmpty() && !state.isEmpty()) stateCode = m_partiesModel.get_state_code_for_state(state);
    m_stateCodeInput->setText(stateCode);
    m_pincodeInput->setText(p.value("pincode").toString());

    // Routes
    bool useRt = p.value("use_routes").toInt() > 0;
    m_useRoutesCheck->setChecked(useRt);
    setComboText(m_routeCombo, p.value("route", "None").toString());

    // Group
    setComboText(m_groupCombo, p.value("group_name").toString());

    // Books Start From Date
    QString booksFrom = p.value("books_start_from").toString().trimmed();
    if (booksFrom.isEmpty()) {
        QString rawMailing = p.value("mailing_name").toString().trimmed();
        static const QRegularExpression dateReg(R"(^\d{1,2}[/-]\d{1,2}[/-]\d{2,4}$)");
        if (dateReg.match(rawMailing).hasMatch()) {
            booksFrom = rawMailing;
            booksFrom.replace('/', '-');
        } else {
            booksFrom = m_partiesModel.get_financial_year_start();
        }
    }
    m_booksFromInput->setText(booksFrom);

    // Opening balance & type
    QString bType = p.value("balance_type").toString();
    if (bType.isEmpty()) bType = "Dr";
    int bIdx = m_balTypeCombo->findText(bType);
    if (bIdx >= 0) m_balTypeCombo->setCurrentIndex(bIdx);
    else m_balTypeCombo->setEditText(bType);

    m_opBalInput->setText(QString::number(p.value("opening_balance").toDouble(), 'f', 2));
    m_openFromInput->setText(p.value("ledger_open_from").toString());

    // Statutory, GST & Contact
    m_gstinInput->setText(p.value("gstin").toString());
    setComboText(m_gstPartyTypeCombo, p.value("gst_party_type", "Normal Dealer").toString());
    m_panInput->setText(p.value("pan").toString());
    m_tinInput->setText(p.value("tin").toString());
    m_urnInput->setText(p.value("urn").toString());

    m_addressInput->setText(p.value("address").toString());
    m_phoneInput->setText(p.value("phone").toString());
    m_whatsappInput->setText(p.value("whatsapp").toString());
    m_emailInput->setText(p.value("email").toString());
    m_aadhaarInput->setText(p.value("aadhaar").toString());

    // Banking & Terms
    m_bankAccountInput->setText(p.value("bank_account").toString());
    m_ifscInput->setText(p.value("ifsc_code").toString());
    m_bankNameInput->setText(p.value("bank_name").toString());
    m_shopNoInput->setText(p.value("shop_no").toString());
    m_creditLimitInput->setText(QString::number(p.value("credit_limit").toDouble(), 'f', 2));

    // Checkboxes
    m_applyTcsCheck->setChecked(p.value("apply_tcs").toInt() > 0);
    m_tcsExemptCheck->setChecked(p.value("tcs_exempt").toInt() > 0);
    m_stockNotCalcCheck->setChecked(p.value("stock_not_calc").toInt() > 0);
    m_calcDirectExpenseCheck->setChecked(p.value("calc_direct_expense").toInt() > 0);
    m_showDateTotalsCheck->setChecked(p.value("show_date_totals").toInt() > 0);
    m_useCreditLimitCheck->setChecked(p.value("use_credit_limit", 1).toInt() > 0);
    m_setTitleCaseCheck->setChecked(p.value("set_title_case", 1).toInt() > 0);

    updateTotalOpeningBalDisplay();
    m_nameInput->setFocus();
    m_nameInput->selectAll();
}

void ModifyLedgerWidget::loadPartyByName(const QString& name) {
    QString cleanName = name.trimmed();
    if (cleanName.isEmpty()) return;
    QVariantMap p = m_partiesModel.get_party_by_name(cleanName);
    if (!p.isEmpty() && p.contains("id")) {
        loadParty(p.value("id").toInt());
    }
}

void ModifyLedgerWidget::keyPressEvent(QKeyEvent* event) {
    if (event->key() == Qt::Key_Escape) {
        emit backRequested();
        return;
    }
    QWidget::keyPressEvent(event);
}

bool ModifyLedgerWidget::eventFilter(QObject* watched, QEvent* event) {
    if (event->type() == QEvent::KeyPress) {
        auto* keyEvent = static_cast<QKeyEvent*>(event);

        auto* targetWidget = qobject_cast<QWidget*>(watched);
        if (targetWidget) {
            if (auto* parentCombo = qobject_cast<QComboBox*>(targetWidget->parent())) {
                targetWidget = parentCombo;
            } else if (auto* parentWidget = targetWidget->parentWidget()) {
                if (m_navOrder.contains(parentWidget)) {
                    targetWidget = parentWidget;
                }
            }
        }

        int idx = m_navOrder.indexOf(targetWidget);

        if (keyEvent->key() == Qt::Key_Return || keyEvent->key() == Qt::Key_Enter) {
            if (targetWidget == m_updateBtn) {
                onUpdateClicked();
                return true;
            }

            if (idx >= 0) {
                for (int nextIdx = idx + 1; nextIdx < m_navOrder.size(); ++nextIdx) {
                    QWidget* next = m_navOrder.at(nextIdx);
                    if (next && next->isEnabled() && next->isVisible()) {
                        next->setFocus();
                        if (auto* le = qobject_cast<QLineEdit*>(next)) {
                            le->selectAll();
                        } else if (auto* cb = qobject_cast<QComboBox*>(next)) {
                            if (cb->lineEdit()) cb->lineEdit()->selectAll();
                        }
                        return true;
                    }
                }
                onUpdateClicked();
                return true;
            }
        } else if (keyEvent->key() == Qt::Key_Left) {
            if (auto* le = qobject_cast<QLineEdit*>(watched)) {
                if (le->cursorPosition() == 0 && !le->hasSelectedText() && idx > 0) {
                    for (int prevIdx = idx - 1; prevIdx >= 0; --prevIdx) {
                        QWidget* prev = m_navOrder.at(prevIdx);
                        if (prev && prev->isEnabled() && prev->isVisible()) {
                            prev->setFocus();
                            if (auto* ple = qobject_cast<QLineEdit*>(prev)) {
                                ple->selectAll();
                            } else if (auto* cb = qobject_cast<QComboBox*>(prev)) {
                                if (cb->lineEdit()) cb->lineEdit()->selectAll();
                            }
                            return true;
                        }
                    }
                }
            }
        } else if (keyEvent->key() == Qt::Key_Right) {
            if (auto* le = qobject_cast<QLineEdit*>(watched)) {
                if (le->cursorPosition() >= le->text().length() && !le->hasSelectedText() && idx >= 0) {
                    for (int nextIdx = idx + 1; nextIdx < m_navOrder.size(); ++nextIdx) {
                        QWidget* next = m_navOrder.at(nextIdx);
                        if (next && next->isEnabled() && next->isVisible()) {
                            next->setFocus();
                            if (auto* nle = qobject_cast<QLineEdit*>(next)) {
                                nle->selectAll();
                            } else if (auto* cb = qobject_cast<QComboBox*>(next)) {
                                if (cb->lineEdit()) cb->lineEdit()->selectAll();
                            }
                            return true;
                        }
                    }
                }
            }
        }
    }
    return QWidget::eventFilter(watched, event);
}

void ModifyLedgerWidget::onUpdateClicked() {
    if (m_selectedPartyId <= 0) {
        CustomMessageBox::warning(this, "No Ledger Selected", "Please select a valid Ledger Account to modify first.");
        focusSearch();
        return;
    }

    QString name = m_nameInput->text().trimmed();
    if (name.isEmpty()) {
        CustomMessageBox::warning(this, "Validation Error", "Party/Ledger Name is required.");
        m_nameInput->setFocus();
        return;
    }

    QString group = getComboText(m_groupCombo);
    if (group.isEmpty()) {
        CustomMessageBox::warning(this, "Validation Error", "Account Group ('Under') must be selected.");
        m_groupCombo->setFocus();
        return;
    }

    bool confirmed = CustomMessageBox::question(
        this,
        "CONFIRM LEDGER UPDATE",
        QString("Are you sure you want to update Ledger Account '%1' under group '%2'?").arg(name, group),
        "Update Ledger",
        "Cancel"
    );

    if (confirmed) {
        executeUpdate();
    }
}

void ModifyLedgerWidget::executeUpdate() {
    QString name = m_nameInput->text().trimmed();
    QString alias = m_aliasInput->text().trimmed();
    QString prefix = getComboText(m_prefixCombo);
    if (prefix.isEmpty() || prefix == "None") prefix = "M/s";
    QString group = getComboText(m_groupCombo);
    QString balType = m_balTypeCombo->currentText().trimmed();
    double opBal = m_opBalInput->text().trimmed().toDouble();

    QString station = getComboText(m_stationCombo);
    QString state = getComboText(m_stateCombo);
    if (state.isEmpty()) {
        QVariant v = DatabaseManager::instance().executeScalar("SELECT state FROM company_info LIMIT 1;");
        if (v.isValid() && !v.isNull()) state = v.toString().trimmed();
    }
    QString stateCode = m_stateCodeInput->text().trimmed();
    if (stateCode.isEmpty() && !state.isEmpty()) stateCode = m_partiesModel.get_state_code_for_state(state);
    QString pincode = m_pincodeInput->text().trimmed();
    if (pincode.isEmpty()) {
        QVariant v = DatabaseManager::instance().executeScalar("SELECT pincode FROM company_info LIMIT 1;");
        if (v.isValid() && !v.isNull()) pincode = v.toString().trimmed();
    }

    int useRoutes = m_useRoutesCheck->isChecked() ? 1 : 0;
    QString route = useRoutes ? getComboText(m_routeCombo) : "";

    QString booksStartFrom = m_booksFromInput->text().trimmed();
    if (booksStartFrom.isEmpty()) booksStartFrom = m_partiesModel.get_financial_year_start();
    QString openFrom = m_openFromInput->text().trimmed();

    QString gstin = m_gstinInput->text().trimmed().toUpper();
    QString gstPartyType = getComboText(m_gstPartyTypeCombo);
    QString pan = m_panInput->text().trimmed().toUpper();
    QString tin = m_tinInput->text().trimmed();
    QString urn = m_urnInput->text().trimmed();

    QString address = m_addressInput->text().trimmed();
    QString phone = m_phoneInput->text().trimmed();
    QString whatsapp = m_whatsappInput->text().trimmed();
    QString email = m_emailInput->text().trimmed();
    QString aadhaar = m_aadhaarInput->text().trimmed();

    QString bankAccount = m_bankAccountInput->text().trimmed();
    QString ifsc = m_ifscInput->text().trimmed().toUpper();
    QString bankName = m_bankNameInput->text().trimmed();
    QString shopNo = m_shopNoInput->text().trimmed();
    double creditLimit = m_creditLimitInput->text().trimmed().toDouble();

    int applyTcs = m_applyTcsCheck->isChecked() ? 1 : 0;
    int tcsExempt = m_tcsExemptCheck->isChecked() ? 1 : 0;
    int stockNotCalc = m_stockNotCalcCheck->isChecked() ? 1 : 0;
    int calcDirectExpense = m_calcDirectExpenseCheck->isChecked() ? 1 : 0;
    int showDateTotals = m_showDateTotalsCheck->isChecked() ? 1 : 0;
    int useCreditLimit = m_useCreditLimitCheck->isChecked() ? 1 : 0;
    int setTitleCase = m_setTitleCaseCheck->isChecked() ? 1 : 0;

    QString partyType = (group.contains("Debtor") || group.contains("Buyer")) ? "Buyer" : "Vendor";
    QString specialType = (partyType == "Buyer") ? "Rice Buyer" : "Paddy Seller";

    QVariantMap data;
    data["name"] = name;
    data["alias"] = alias;
    data["prefix"] = prefix;
    data["group_name"] = group;
    data["party_type"] = partyType;
    data["special_type"] = specialType;
    data["opening_balance"] = opBal;
    data["balance_type"] = balType;
    data["mailing_name"] = name;
    data["address"] = address;
    data["city"] = station;
    data["district"] = "";
    data["state"] = state;
    data["state_code"] = stateCode;
    data["pincode"] = pincode;
    data["route"] = route;
    data["mobile"] = phone;
    data["whatsapp"] = whatsapp;
    data["phone"] = phone;
    data["email"] = email;
    data["contact_person"] = "";
    data["pan"] = pan;
    data["aadhaar"] = aadhaar;
    data["tan"] = "";
    data["gstin"] = gstin;
    data["gst_party_type"] = gstPartyType;
    data["bank_name"] = bankName;
    data["bank_account"] = bankAccount;
    data["ifsc_code"] = ifsc;
    data["credit_limit"] = creditLimit;
    data["credit_days"] = 30;
    data["interest_rate"] = 0.0;
    data["commission_rate"] = 0.0;
    data["commission_on"] = "";
    data["apply_tcs"] = applyTcs;
    data["tcs_exempt"] = tcsExempt;
    data["party_station"] = station;
    data["use_routes"] = useRoutes;
    data["shop_no"] = shopNo;
    data["tin"] = tin;
    data["urn"] = urn;
    data["stock_not_calc"] = stockNotCalc;
    data["use_credit_limit"] = useCreditLimit;
    data["show_date_totals"] = showDateTotals;
    data["calc_direct_expense"] = calcDirectExpense;
    data["set_title_case"] = setTitleCase;
    data["ledger_open_from"] = openFrom;
    data["books_start_from"] = booksStartFrom;

    bool success = m_partiesModel.update_ledger_extended(m_selectedPartyId, data);

    if (success) {
        CustomMessageBox::information(this, "Success", QString("Ledger Account '%1' updated successfully.").arg(name));
        emit savedSuccess();
        emit backRequested();
    } else {
        CustomMessageBox::critical(this, "Error", "Failed to update ledger account in database.");
    }
}

void ModifyLedgerWidget::onDeleteClicked() {
    if (m_selectedPartyId <= 0) {
        CustomMessageBox::warning(this, "No Ledger Selected", "Please select a Ledger Account to delete.");
        focusSearch();
        return;
    }

    QString name = m_nameInput->text().trimmed();

    // Check if any transactions or invoices exist for this party
    QVariant txCount = DatabaseManager::instance().executeScalar(
        "SELECT COUNT(*) FROM transactions WHERE party_id = ? OR LOWER(party_name) = LOWER(?);",
        {m_selectedPartyId, name}
    );
    QVariant vCount = DatabaseManager::instance().executeScalar(
        "SELECT COUNT(*) FROM vouchers WHERE party_id = ? OR ledger_id = ? OR LOWER(party_name) = LOWER(?);",
        {m_selectedPartyId, m_selectedPartyId, name}
    );
    QVariant sCount = DatabaseManager::instance().executeScalar(
        "SELECT COUNT(*) FROM sales_invoices WHERE customer_id = ? OR LOWER(customer_name) = LOWER(?);",
        {m_selectedPartyId, name}
    );
    QVariant pCount = DatabaseManager::instance().executeScalar(
        "SELECT COUNT(*) FROM purchase_invoices WHERE supplier_id = ? OR LOWER(supplier_name) = LOWER(?);",
        {m_selectedPartyId, name}
    );

    int totalTx = txCount.toInt() + vCount.toInt() + sCount.toInt() + pCount.toInt();
    if (totalTx > 0) {
        CustomMessageBox::warning(
            this,
            "Cannot Delete Ledger",
            QString("Cannot delete Ledger Account '%1' because it has %2 associated transaction(s) / voucher(s).\n\nPlease delete the associated vouchers first before deleting this ledger.").arg(name, QString::number(totalTx))
        );
        return;
    }

    bool confirmed = CustomMessageBox::question(
        this,
        "CONFIRM LEDGER DELETION",
        QString("Are you sure you want to PERMANENTLY delete Ledger Account '%1'?\n\nThis action cannot be undone.").arg(name),
        "Delete Ledger",
        "Cancel"
    );

    if (confirmed) {
        executeDelete();
    }
}

void ModifyLedgerWidget::executeDelete() {
    QString name = m_nameInput->text().trimmed();
    bool ok = DatabaseManager::instance().executeNonQuery(
        "DELETE FROM parties WHERE id = ?;",
        {m_selectedPartyId}
    );

    if (ok) {
        m_partiesModel.reload_data();
        CustomMessageBox::information(this, "Deleted", QString("Ledger Account '%1' was successfully deleted.").arg(name));
        resetForm();
        emit savedSuccess();
        emit backRequested();
    } else {
        CustomMessageBox::critical(this, "Error", "Failed to delete ledger account from database.");
    }
}
