#include "tax_challan_creation_widget.h"
#include "voucher_date_dialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QHeaderView>
#include <QDate>
#include <QShortcut>
#include <QDoubleValidator>
#include <QKeyEvent>

namespace MahadevERP {

TaxChallanCreationWidget::TaxChallanCreationWidget(TaxChallanController *controller, QWidget *parent)
    : QWidget(parent)
    , m_controller(controller)
{
    setupUi();

    connect(m_controller, &TaxChallanController::totalsChanged, this, &TaxChallanCreationWidget::syncUiFromController);
    connect(m_controller, &TaxChallanController::undepositedVouchersChanged, this, &TaxChallanCreationWidget::populateTable);
    connect(m_controller, &TaxChallanController::statusChanged, this, [this]() {
        m_statusLabel->setText(m_controller->statusMessage());
        m_statusLabel->setStyleSheet(QString("font-weight: 700; font-size: 12px; color: %1;").arg(m_controller->isError() ? "#dc2626" : "#16a34a"));
    });

    syncUiFromController();
    m_controller->fetchUndepositedVouchers();
}

void TaxChallanCreationWidget::setTaxType(const QString &taxType)
{
    m_controller->setTaxType(taxType);
    m_titleLabel->setText(QString("%1. Deposit Challan (Creation)").arg(taxType == "TDS" ? "T.D.S" : "T.C.S"));
    m_controller->fetchUndepositedVouchers();
}

void TaxChallanCreationWidget::setupUi()
{
    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(14, 12, 14, 12);
    mainLayout->setSpacing(10);

    // 1. Header Bar
    mainLayout->addWidget(createHeaderBar());

    // 2. Parameters Card
    mainLayout->addWidget(createParamsCard());

    // 3. Center Table & selection buttons
    auto *tableContainer = new QFrame(this);
    tableContainer->setStyleSheet("QFrame { background-color: #FFFFFF; border: 1px solid #CBD5E1; border-radius: 8px; }");
    auto *tableLayout = new QVBoxLayout(tableContainer);
    tableLayout->setContentsMargins(12, 10, 12, 10);
    tableLayout->setSpacing(8);

    auto *tableHeaderLayout = new QHBoxLayout();
    auto *tableTitle = new QLabel("Un-deposited Vouchers List", tableContainer);
    tableTitle->setStyleSheet("font-weight: 800; color: #0F172A; font-size: 13px; border: none; background: transparent;");
    tableHeaderLayout->addWidget(tableTitle);

    tableHeaderLayout->addStretch(1);

    auto *btnSelectAll = new QPushButton("Select All", tableContainer);
    btnSelectAll->setCursor(Qt::PointingHandCursor);
    btnSelectAll->setStyleSheet(
        "QPushButton { background-color: #FFFFFF; border: 1.5px solid #CBD5E1; border-radius: 4px; padding: 4px 10px; font-size: 11px; font-weight: 700; color: #334155; }"
        "QPushButton:hover { background-color: #F8FAFC; }"
    );
    connect(btnSelectAll, &QPushButton::clicked, this, &TaxChallanCreationWidget::onSelectAllClicked);
    tableHeaderLayout->addWidget(btnSelectAll);

    auto *btnSelectNone = new QPushButton("Clear All", tableContainer);
    btnSelectNone->setCursor(Qt::PointingHandCursor);
    btnSelectNone->setStyleSheet(
        "QPushButton { background-color: #FFFFFF; border: 1.5px solid #CBD5E1; border-radius: 4px; padding: 4px 10px; font-size: 11px; font-weight: 700; color: #334155; }"
        "QPushButton:hover { background-color: #F8FAFC; }"
    );
    connect(btnSelectNone, &QPushButton::clicked, this, &TaxChallanCreationWidget::onSelectNoneClicked);
    tableHeaderLayout->addWidget(btnSelectNone);

    tableLayout->addLayout(tableHeaderLayout);

    m_vouchersTable = new QTableWidget(tableContainer);
    m_vouchersTable->setColumnCount(10);
    m_vouchersTable->setHorizontalHeaderLabels({
        "Select", "Type", "Vch No", "Date", "Deductee / Party Name",
        "Gross Value (₹)", "Basic Tax (₹)", "Surcharge (₹)", "Cess (₹)", "Total Tax (₹)"
    });
    m_vouchersTable->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    m_vouchersTable->horizontalHeader()->setSectionResizeMode(4, QHeaderView::Stretch);
    m_vouchersTable->verticalHeader()->setVisible(false);
    m_vouchersTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_vouchersTable->setAlternatingRowColors(true);
    m_vouchersTable->setStyleSheet(
        "QTableWidget { background-color: #FFFFFF; border: 1px solid #CBD5E1; border-radius: 6px; gridline-color: #F1F5F9; font-size: 12px; color: #0F172A; }"
        "QTableWidget::item { padding: 5px 8px; }"
        "QTableWidget::item:selected { background-color: #EFF6FF; color: #1E3A8A; font-weight: bold; }"
        "QHeaderView::section { background-color: #0F172A; color: #FFFFFF; font-weight: 800; font-size: 11px; padding: 6px 8px; border: none; }"
    );
    connect(m_vouchersTable, &QTableWidget::itemChanged, this, &TaxChallanCreationWidget::onTableItemChanged);
    tableLayout->addWidget(m_vouchersTable, 1);

    m_tableSummaryLabel = new QLabel("Selected: 0 vouchers | Total Tax: ₹0.00", tableContainer);
    m_tableSummaryLabel->setStyleSheet("color: #16A34A; font-weight: 700; font-size: 12px; border: none; background: transparent;");
    tableLayout->addWidget(m_tableSummaryLabel);

    mainLayout->addWidget(tableContainer, 1);

    // 4. Bottom Cards: Adjustments (Left) & Bank Allocation (Right)
    auto *bottomLayout = new QHBoxLayout();
    bottomLayout->setSpacing(12);

    bottomLayout->addWidget(createAdjustmentCard(), 1);
    bottomLayout->addWidget(createBankDetailsCard(), 1);

    mainLayout->addLayout(bottomLayout);

    // Shortcuts
    new QShortcut(QKeySequence("F2"), this, SLOT(onSaveClicked()));
    new QShortcut(QKeySequence("Ctrl+S"), this, SLOT(onSaveClicked()));
    new QShortcut(QKeySequence("Escape"), this, SIGNAL(backRequested()));
}

QWidget *TaxChallanCreationWidget::createHeaderBar()
{
    auto *headerCard = new QFrame(this);
    headerCard->setStyleSheet("QFrame { background-color: #FFFFFF; border: 1px solid #E2E8F0; border-radius: 8px; }");
    auto *layout = new QHBoxLayout(headerCard);
    layout->setContentsMargins(14, 10, 14, 10);

    auto *titleCol = new QVBoxLayout();
    titleCol->setSpacing(2);
    m_titleLabel = new QLabel(QString("%1. Deposit Challan (Creation)").arg(m_controller->taxType() == "TDS" ? "T.D.S" : "T.C.S"), headerCard);
    m_titleLabel->setStyleSheet("font-size: 16px; font-weight: 800; color: #0F172A; border: none; background: transparent;");
    auto *subLabel = new QLabel("Prepare ITNS 281 single deposit challan for selected withholding vouchers.", headerCard);
    subLabel->setStyleSheet("font-size: 11px; color: #64748B; border: none; background: transparent;");
    titleCol->addWidget(m_titleLabel);
    titleCol->addWidget(subLabel);
    layout->addLayout(titleCol);

    m_statusLabel = new QLabel("", headerCard);
    m_statusLabel->setStyleSheet("margin-left: 15px; border: none; background: transparent;");
    layout->addWidget(m_statusLabel, 1);

    auto *btnSave = new QPushButton("[F2] Save Challan", headerCard);
    btnSave->setCursor(Qt::PointingHandCursor);
    btnSave->setStyleSheet(
        "QPushButton { background-color: #16A34A; color: #FFFFFF; border: none; border-radius: 6px; padding: 6px 18px; font-weight: 800; font-size: 12px; }"
        "QPushButton:hover { background-color: #15803D; }"
    );
    connect(btnSave, &QPushButton::clicked, this, &TaxChallanCreationWidget::onSaveClicked);
    layout->addWidget(btnSave);

    auto *btnBack = new QPushButton("← [Esc] Back", headerCard);
    btnBack->setCursor(Qt::PointingHandCursor);
    btnBack->setStyleSheet(
        "QPushButton { background-color: #F1F5F9; border: 1px solid #CBD5E1; border-radius: 6px; padding: 6px 12px; font-weight: 700; color: #475569; font-size: 11.5px; }"
        "QPushButton:hover { background-color: #E2E8F0; }"
    );
    connect(btnBack, &QPushButton::clicked, this, &TaxChallanCreationWidget::backRequested);
    layout->addWidget(btnBack);

    return headerCard;
}

QWidget *TaxChallanCreationWidget::createParamsCard()
{
    auto *card = new QFrame(this);
    card->setStyleSheet(
        "QFrame { background-color: #FFFFFF; border: 1px solid #CBD5E1; border-radius: 8px; }"
        "QLabel { color: #475569; font-weight: 700; font-size: 11.5px; border: none; background: transparent; }"
        "QLineEdit, QComboBox { background-color: #FFFFFF; border: 1.5px solid #CBD5E1; border-radius: 6px; padding: 4px 8px; font-size: 12px; color: #0F172A; font-weight: 600; }"
        "QLineEdit:focus, QComboBox:focus { border-color: #2563EB; background-color: #F8FAFC; }"
    );
    auto *layout = new QHBoxLayout(card);
    layout->setContentsMargins(12, 10, 12, 10);
    layout->setSpacing(14);

    // Challan No
    auto *chBox = new QVBoxLayout();
    chBox->setSpacing(4);
    chBox->addWidget(new QLabel("Challan No. *", card));
    m_challanNoEdit = new QLineEdit(card);
    m_challanNoEdit->setPlaceholderText("ITNS 281 No");
    connect(m_challanNoEdit, &QLineEdit::textChanged, m_controller, &TaxChallanController::setChallanNo);
    chBox->addWidget(m_challanNoEdit);
    layout->addLayout(chBox);

    // Date
    auto *dtBox = new QVBoxLayout();
    dtBox->setSpacing(4);
    dtBox->addWidget(new QLabel("Date (F2) *", card));
    auto *dtRow = new QHBoxLayout();
    m_challanDateEdit = new QLineEdit(card);
    m_challanDateEdit->setReadOnly(true);
    dtRow->addWidget(m_challanDateEdit);
    auto *btnDate = new QPushButton("📅", card);
    btnDate->setFixedWidth(28);
    btnDate->setStyleSheet("QPushButton { background-color: #F8FAFC; border: 1.5px solid #CBD5E1; border-radius: 4px; }");
    connect(btnDate, &QPushButton::clicked, this, [this]() {
        QDate cur = QDate::fromString(m_controller->challanDate(), "yyyy-MM-dd");
        if (!cur.isValid()) cur = QDate::currentDate();
        QDate chosen = VoucherDateDialog::selectDate(this, cur);
        if (chosen.isValid()) {
            m_controller->setChallanDate(chosen.toString("yyyy-MM-dd"));
            m_challanDateEdit->setText(chosen.toString("dd-MM-yyyy"));
        }
    });
    dtRow->addWidget(btnDate);
    dtBox->addLayout(dtRow);
    layout->addLayout(dtBox);

    // Post in Books
    auto *pbBox = new QVBoxLayout();
    pbBox->setSpacing(4);
    pbBox->addWidget(new QLabel("Post In Books", card));
    m_postInBooksCombo = new QComboBox(card);
    m_postInBooksCombo->addItems({"Yes", "No"});
    connect(m_postInBooksCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int idx) {
        m_controller->setPostInBooks(idx == 0);
    });
    pbBox->addWidget(m_postInBooksCombo);
    layout->addLayout(pbBox);

    // Period From
    auto *pfBox = new QVBoxLayout();
    pfBox->setSpacing(4);
    pfBox->addWidget(new QLabel("Period From", card));
    auto *pfRow = new QHBoxLayout();
    m_periodFromEdit = new QLineEdit(card);
    m_periodFromEdit->setReadOnly(true);
    pfRow->addWidget(m_periodFromEdit);
    auto *btnPf = new QPushButton("📅", card);
    btnPf->setFixedWidth(28);
    btnPf->setStyleSheet("QPushButton { background-color: #F8FAFC; border: 1.5px solid #CBD5E1; border-radius: 4px; }");
    connect(btnPf, &QPushButton::clicked, this, [this]() {
        QDate cur = QDate::fromString(m_controller->periodFrom(), "yyyy-MM-dd");
        if (!cur.isValid()) cur = QDate::currentDate();
        QDate chosen = VoucherDateDialog::selectDate(this, cur);
        if (chosen.isValid()) {
            m_controller->setPeriodFrom(chosen.toString("yyyy-MM-dd"));
            m_periodFromEdit->setText(chosen.toString("dd-MM-yyyy"));
        }
    });
    pfRow->addWidget(btnPf);
    pfBox->addLayout(pfRow);
    layout->addLayout(pfBox);

    // Period To
    auto *ptBox = new QVBoxLayout();
    ptBox->setSpacing(4);
    ptBox->addWidget(new QLabel("Period To", card));
    auto *ptRow = new QHBoxLayout();
    m_periodToEdit = new QLineEdit(card);
    m_periodToEdit->setReadOnly(true);
    ptRow->addWidget(m_periodToEdit);
    auto *btnPt = new QPushButton("📅", card);
    btnPt->setFixedWidth(28);
    btnPt->setStyleSheet("QPushButton { background-color: #F8FAFC; border: 1.5px solid #CBD5E1; border-radius: 4px; }");
    connect(btnPt, &QPushButton::clicked, this, [this]() {
        QDate cur = QDate::fromString(m_controller->periodTo(), "yyyy-MM-dd");
        if (!cur.isValid()) cur = QDate::currentDate();
        QDate chosen = VoucherDateDialog::selectDate(this, cur);
        if (chosen.isValid()) {
            m_controller->setPeriodTo(chosen.toString("yyyy-MM-dd"));
            m_periodToEdit->setText(chosen.toString("dd-MM-yyyy"));
        }
    });
    ptRow->addWidget(btnPt);
    ptBox->addLayout(ptRow);
    layout->addLayout(ptBox);

    // Fetch Button
    auto *btnFetch = new QPushButton("Fetch Vouchers", card);
    btnFetch->setFixedHeight(30);
    btnFetch->setCursor(Qt::PointingHandCursor);
    btnFetch->setStyleSheet(
        "QPushButton { background-color: #2563EB; color: #FFFFFF; font-weight: 700; font-size: 11.5px; padding: 0 16px; border-radius: 6px; border: none; }"
        "QPushButton:hover { background-color: #1D4ED8; }"
    );
    connect(btnFetch, &QPushButton::clicked, this, &TaxChallanCreationWidget::onFetchClicked);
    layout->addWidget(btnFetch);

    return card;
}

QWidget *TaxChallanCreationWidget::createAdjustmentCard()
{
    auto *card = new QFrame(this);
    card->setStyleSheet(
        "QFrame { background-color: #FFFFFF; border: 1px solid #E2E8F0; border-radius: 8px; }"
        "QLabel { color: #475569; font-weight: 700; font-size: 11.5px; border: none; background: transparent; }"
        "QLineEdit { background-color: #FFFFFF; border: 1.5px solid #CBD5E1; border-radius: 6px; padding: 4px 8px; font-size: 12px; color: #0F172A; font-weight: 600; }"
        "QLineEdit:focus { border-color: #2563EB; background-color: #F8FAFC; }"
    );
    auto *grid = new QGridLayout(card);
    grid->setContentsMargins(14, 12, 14, 12);
    grid->setHorizontalSpacing(10);
    grid->setVerticalSpacing(8);

    auto *val = new QDoubleValidator(0.0, 999999999.0, 2, this);
    val->setNotation(QDoubleValidator::StandardNotation);

    // Row 0: Basic Tax
    grid->addWidget(new QLabel("Basic Tax:", card), 0, 0);
    m_basicTaxEdit = new QLineEdit(card);
    m_basicTaxEdit->setReadOnly(true);
    m_basicTaxEdit->setAlignment(Qt::AlignRight);
    grid->addWidget(m_basicTaxEdit, 0, 1, 1, 2);

    // Row 1: Interest
    grid->addWidget(new QLabel("Interest:", card), 1, 0);
    m_interestAmtEdit = new QLineEdit(card);
    m_interestAmtEdit->setValidator(val);
    m_interestAmtEdit->setAlignment(Qt::AlignRight);
    connect(m_interestAmtEdit, &QLineEdit::textChanged, this, [this](const QString &t) {
        m_controller->setInterestAmount(t.toDouble());
    });
    grid->addWidget(m_interestAmtEdit, 1, 1);
    m_interestLedgerBox = new AccountSearchBox(card);
    connect(m_interestLedgerBox, &AccountSearchBox::partySelectedWithId, this, [this](const QString &, int id) {
        m_controller->setInterestLedgerId(id);
    });
    grid->addWidget(m_interestLedgerBox, 1, 2);

    // Row 2: Penalty
    grid->addWidget(new QLabel("Penalty:", card), 2, 0);
    m_penaltyAmtEdit = new QLineEdit(card);
    m_penaltyAmtEdit->setValidator(val);
    m_penaltyAmtEdit->setAlignment(Qt::AlignRight);
    connect(m_penaltyAmtEdit, &QLineEdit::textChanged, this, [this](const QString &t) {
        m_controller->setPenaltyAmount(t.toDouble());
    });
    grid->addWidget(m_penaltyAmtEdit, 2, 1);
    m_penaltyLedgerBox = new AccountSearchBox(card);
    connect(m_penaltyLedgerBox, &AccountSearchBox::partySelectedWithId, this, [this](const QString &, int id) {
        m_controller->setPenaltyLedgerId(id);
    });
    grid->addWidget(m_penaltyLedgerBox, 2, 2);

    // Row 3: Other
    grid->addWidget(new QLabel("Other Charges:", card), 3, 0);
    m_otherAmtEdit = new QLineEdit(card);
    m_otherAmtEdit->setValidator(val);
    m_otherAmtEdit->setAlignment(Qt::AlignRight);
    connect(m_otherAmtEdit, &QLineEdit::textChanged, this, [this](const QString &t) {
        m_controller->setOtherAmount(t.toDouble());
    });
    grid->addWidget(m_otherAmtEdit, 3, 1);
    m_otherLedgerBox = new AccountSearchBox(card);
    connect(m_otherLedgerBox, &AccountSearchBox::partySelectedWithId, this, [this](const QString &, int id) {
        m_controller->setOtherLedgerId(id);
    });
    grid->addWidget(m_otherLedgerBox, 3, 2);

    // Row 4: Total Challan Amount
    auto *totalLabel = new QLabel("Total Challan Amount:", card);
    totalLabel->setStyleSheet("font-weight: 800; color: #15803D; font-size: 13px;");
    grid->addWidget(totalLabel, 4, 0);
    m_totalChallanAmtEdit = new QLineEdit(card);
    m_totalChallanAmtEdit->setReadOnly(true);
    m_totalChallanAmtEdit->setAlignment(Qt::AlignRight);
    m_totalChallanAmtEdit->setStyleSheet(
        "QLineEdit {"
        "  font-size: 15px;"
        "  font-weight: 800;"
        "  color: #15803D;"
        "  background-color: #F0FDF4;"
        "  border: 1.5px solid #86EFAC;"
        "  border-radius: 6px;"
        "  padding: 4px 8px;"
        "}"
    );
    grid->addWidget(m_totalChallanAmtEdit, 4, 1, 1, 2);

    return card;
}

QWidget *TaxChallanCreationWidget::createBankDetailsCard()
{
    auto *card = new QFrame(this);
    card->setStyleSheet(
        "QFrame { background-color: #FFFFFF; border: 1px solid #E2E8F0; border-radius: 8px; }"
        "QLabel { color: #475569; font-weight: 700; font-size: 11.5px; border: none; background: transparent; }"
        "QLineEdit { background-color: #FFFFFF; border: 1.5px solid #CBD5E1; border-radius: 6px; padding: 4px 8px; font-size: 12px; color: #0F172A; font-weight: 600; }"
        "QLineEdit:focus { border-color: #2563EB; background-color: #F8FAFC; }"
    );
    auto *grid = new QGridLayout(card);
    grid->setContentsMargins(14, 12, 14, 12);
    grid->setHorizontalSpacing(10);
    grid->setVerticalSpacing(8);

    // Row 0: Bank Account
    grid->addWidget(new QLabel("Bank Account *:", card), 0, 0);
    m_bankLedgerBox = new AccountSearchBox(card);
    connect(m_bankLedgerBox, &AccountSearchBox::partySelectedWithId, this, [this](const QString &, int id) {
        m_controller->setBankLedgerId(id);
    });
    grid->addWidget(m_bankLedgerBox, 0, 1, 1, 3);

    // Row 1: Cheque No & Cheque Date
    grid->addWidget(new QLabel("Cheque / Ref No:", card), 1, 0);
    m_chequeNoEdit = new QLineEdit(card);
    connect(m_chequeNoEdit, &QLineEdit::textChanged, m_controller, &TaxChallanController::setChequeNo);
    grid->addWidget(m_chequeNoEdit, 1, 1);

    grid->addWidget(new QLabel("Cheque Date:", card), 1, 2);
    auto *chqDateRow = new QHBoxLayout();
    m_chequeDateEdit = new QLineEdit(card);
    m_chequeDateEdit->setReadOnly(true);
    chqDateRow->addWidget(m_chequeDateEdit);
    auto *btnChqDate = new QPushButton("📅", card);
    btnChqDate->setFixedWidth(28);
    btnChqDate->setStyleSheet("QPushButton { background-color: #F8FAFC; border: 1.5px solid #CBD5E1; border-radius: 4px; }");
    connect(btnChqDate, &QPushButton::clicked, this, [this]() {
        QDate cur = QDate::fromString(m_controller->chequeDate(), "yyyy-MM-dd");
        if (!cur.isValid()) cur = QDate::currentDate();
        QDate chosen = VoucherDateDialog::selectDate(this, cur);
        if (chosen.isValid()) {
            m_controller->setChequeDate(chosen.toString("yyyy-MM-dd"));
            m_chequeDateEdit->setText(chosen.toString("dd-MM-yyyy"));
        }
    });
    chqDateRow->addWidget(btnChqDate);
    grid->addLayout(chqDateRow, 1, 3);

    // Row 2: BSR Code & Minor Head
    grid->addWidget(new QLabel("BSR Code (7 Digits):", card), 2, 0);
    m_bsrCodeEdit = new QLineEdit(card);
    m_bsrCodeEdit->setMaxLength(7);
    m_bsrCodeEdit->setPlaceholderText("e.g. 0510302");
    connect(m_bsrCodeEdit, &QLineEdit::textChanged, m_controller, &TaxChallanController::setBsrCode);
    grid->addWidget(m_bsrCodeEdit, 2, 1);

    grid->addWidget(new QLabel("Minor Head:", card), 2, 2);
    m_minorHeadEdit = new QLineEdit("200", card);
    connect(m_minorHeadEdit, &QLineEdit::textChanged, m_controller, &TaxChallanController::setMinorHead);
    grid->addWidget(m_minorHeadEdit, 2, 3);

    // Row 3: Major Head & Narration
    grid->addWidget(new QLabel("Major Head:", card), 3, 0);
    m_majorHeadEdit = new QLineEdit("0021", card);
    connect(m_majorHeadEdit, &QLineEdit::textChanged, m_controller, &TaxChallanController::setMajorHead);
    grid->addWidget(m_majorHeadEdit, 3, 1);

    grid->addWidget(new QLabel("Narration:", card), 3, 2);
    m_narrationEdit = new QLineEdit(card);
    connect(m_narrationEdit, &QLineEdit::textChanged, m_controller, &TaxChallanController::setNarration);
    grid->addWidget(m_narrationEdit, 3, 3);

    return card;
}

void TaxChallanCreationWidget::populateTable()
{
    m_isUpdatingTable = true;
    m_vouchersTable->setRowCount(0);

    QVariantList list = m_controller->undepositedVouchers();
    m_vouchersTable->setRowCount(list.size());

    int selectedCount = 0;
    for (int i = 0; i < list.size(); ++i) {
        QVariantMap m = list[i].toMap();
        bool isSel = m.value("selected", true).toBool();
        if (isSel) selectedCount++;

        // 0: Checkbox
        auto *chkItem = new QTableWidgetItem();
        chkItem->setCheckState(isSel ? Qt::Checked : Qt::Unchecked);
        chkItem->setTextAlignment(Qt::AlignCenter);
        m_vouchersTable->setItem(i, 0, chkItem);

        // 1: Type
        auto *typeItem = new QTableWidgetItem(m.value("vch_type").toString());
        typeItem->setFlags(typeItem->flags() & ~Qt::ItemIsEditable);
        m_vouchersTable->setItem(i, 1, typeItem);

        // 2: Vch No
        auto *noItem = new QTableWidgetItem(m.value("voucher_no").toString());
        noItem->setFlags(noItem->flags() & ~Qt::ItemIsEditable);
        m_vouchersTable->setItem(i, 2, noItem);

        // 3: Date
        QString dStr = m.value("voucher_date").toString();
        QDate d = QDate::fromString(dStr, "yyyy-MM-dd");
        auto *dtItem = new QTableWidgetItem(d.isValid() ? d.toString("dd-MM-yyyy") : dStr);
        dtItem->setFlags(dtItem->flags() & ~Qt::ItemIsEditable);
        m_vouchersTable->setItem(i, 3, dtItem);

        // 4: Party Name
        auto *ptyItem = new QTableWidgetItem(m.value("party_name").toString());
        ptyItem->setFlags(ptyItem->flags() & ~Qt::ItemIsEditable);
        m_vouchersTable->setItem(i, 4, ptyItem);

        // 5: Gross Value
        auto *grItem = new QTableWidgetItem(QString::number(m.value("gross_amount").toDouble(), 'f', 2));
        grItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        grItem->setFlags(grItem->flags() & ~Qt::ItemIsEditable);
        m_vouchersTable->setItem(i, 5, grItem);

        // 6: Basic Tax
        auto *btItem = new QTableWidgetItem(QString::number(m.value("basic_tax").toDouble(), 'f', 2));
        btItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        btItem->setFlags(btItem->flags() & ~Qt::ItemIsEditable);
        m_vouchersTable->setItem(i, 6, btItem);

        // 7: Surcharge
        auto *surItem = new QTableWidgetItem(QString::number(m.value("surcharge").toDouble(), 'f', 2));
        surItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        surItem->setFlags(surItem->flags() & ~Qt::ItemIsEditable);
        m_vouchersTable->setItem(i, 7, surItem);

        // 8: Cess
        auto *cessItem = new QTableWidgetItem(QString::number(m.value("cess").toDouble(), 'f', 2));
        cessItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        cessItem->setFlags(cessItem->flags() & ~Qt::ItemIsEditable);
        m_vouchersTable->setItem(i, 8, cessItem);

        // 9: Total Tax
        auto *totItem = new QTableWidgetItem(QString::number(m.value("total_tax_amount").toDouble(), 'f', 2));
        totItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        totItem->setFlags(totItem->flags() & ~Qt::ItemIsEditable);
        totItem->setForeground(QBrush(QColor("#16A34A")));
        QFont f = totItem->font();
        f.setBold(true);
        totItem->setFont(f);
        m_vouchersTable->setItem(i, 9, totItem);
    }

    m_isUpdatingTable = false;

    m_tableSummaryLabel->setText(
        QString("Selected: %1 / %2 vouchers | Basic Tax: ₹%3")
            .arg(selectedCount)
            .arg(list.size())
            .arg(QString::number(m_controller->basicTax(), 'f', 2))
    );
}

void TaxChallanCreationWidget::onTableItemChanged(QTableWidgetItem *item)
{
    if (m_isUpdatingTable || !item || item->column() != 0) return;
    int row = item->row();
    bool checked = (item->checkState() == Qt::Checked);
    m_controller->toggleVoucherSelection(row, checked);
}

void TaxChallanCreationWidget::onSelectAllClicked()
{
    m_controller->selectAllVouchers(true);
}

void TaxChallanCreationWidget::onSelectNoneClicked()
{
    m_controller->selectAllVouchers(false);
}

void TaxChallanCreationWidget::onFetchClicked()
{
    m_controller->fetchUndepositedVouchers();
}

void TaxChallanCreationWidget::onSaveClicked()
{
    if (m_controller->saveChallan()) {
        emit challanSaved(0);
    }
}

void TaxChallanCreationWidget::syncUiFromController()
{
    m_challanNoEdit->setText(m_controller->challanNo());

    QDate chDate = QDate::fromString(m_controller->challanDate(), "yyyy-MM-dd");
    m_challanDateEdit->setText(chDate.isValid() ? chDate.toString("dd-MM-yyyy") : m_controller->challanDate());

    m_postInBooksCombo->setCurrentIndex(m_controller->postInBooks() ? 0 : 1);

    QDate pf = QDate::fromString(m_controller->periodFrom(), "yyyy-MM-dd");
    m_periodFromEdit->setText(pf.isValid() ? pf.toString("dd-MM-yyyy") : m_controller->periodFrom());

    QDate pt = QDate::fromString(m_controller->periodTo(), "yyyy-MM-dd");
    m_periodToEdit->setText(pt.isValid() ? pt.toString("dd-MM-yyyy") : m_controller->periodTo());

    m_basicTaxEdit->setText(QString::number(m_controller->totalTax(), 'f', 2));
    m_interestAmtEdit->setText(m_controller->interestAmount() > 0.0 ? QString::number(m_controller->interestAmount(), 'f', 2) : "");
    m_penaltyAmtEdit->setText(m_controller->penaltyAmount() > 0.0 ? QString::number(m_controller->penaltyAmount(), 'f', 2) : "");
    m_otherAmtEdit->setText(m_controller->otherAmount() > 0.0 ? QString::number(m_controller->otherAmount(), 'f', 2) : "");
    m_totalChallanAmtEdit->setText(QString::number(m_controller->totalChallanAmount(), 'f', 2));

    if (m_controller->interestLedgerId() > 0) m_interestLedgerBox->setSelectedPartyId(m_controller->interestLedgerId());
    if (m_controller->penaltyLedgerId() > 0) m_penaltyLedgerBox->setSelectedPartyId(m_controller->penaltyLedgerId());
    if (m_controller->bankLedgerId() > 0) m_bankLedgerBox->setSelectedPartyId(m_controller->bankLedgerId());

    m_chequeNoEdit->setText(m_controller->chequeNo());
    QDate cq = QDate::fromString(m_controller->chequeDate(), "yyyy-MM-dd");
    m_chequeDateEdit->setText(cq.isValid() ? cq.toString("dd-MM-yyyy") : m_controller->chequeDate());

    m_bsrCodeEdit->setText(m_controller->bsrCode());
    m_minorHeadEdit->setText(m_controller->minorHead());
    m_majorHeadEdit->setText(m_controller->majorHead());
    m_narrationEdit->setText(m_controller->narration());
}

void TaxChallanCreationWidget::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Space && m_vouchersTable->hasFocus()) {
        int row = m_vouchersTable->currentRow();
        if (row >= 0) {
            auto *item = m_vouchersTable->item(row, 0);
            if (item) {
                item->setCheckState(item->checkState() == Qt::Checked ? Qt::Unchecked : Qt::Checked);
                return;
            }
        }
    }
    QWidget::keyPressEvent(event);
}

} // namespace MahadevERP
