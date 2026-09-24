#include "tcs_receipt_voucher_widget.h"
#include "voucher_date_dialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QDate>
#include <QShortcut>
#include <QDoubleValidator>
#include <QMessageBox>
#include <QKeyEvent>

namespace MahadevERP {

TcsReceiptVoucherWidget::TcsReceiptVoucherWidget(TcsReceiptVoucherController *controller, QWidget *parent)
    : QWidget(parent)
    , m_controller(controller)
{
    setupUi();

    connect(m_controller, &TcsReceiptVoucherController::totalsChanged, this, &TcsReceiptVoucherWidget::syncUiFromController);
    connect(m_controller, &TcsReceiptVoucherController::statusChanged, this, [this]() {
        m_statusLabel->setText(m_controller->statusMessage());
        m_statusLabel->setStyleSheet(QString("font-weight: 700; font-size: 12px; color: %1;").arg(m_controller->isError() ? "#dc2626" : "#16a34a"));
    });

    onNewClicked();
}

void TcsReceiptVoucherWidget::setupUi()
{
    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(14, 12, 14, 12);
    mainLayout->setSpacing(10);

    // 1. Header Bar
    mainLayout->addWidget(createHeaderBar());

    // 2. Parameters Card
    mainLayout->addWidget(createParamsCard());

    // 3. Main Form Card
    mainLayout->addWidget(createMainFormCard(), 1);

    // Shortcuts
    new QShortcut(QKeySequence("F2"), this, SLOT(onSaveClicked()));
    new QShortcut(QKeySequence("Ctrl+S"), this, SLOT(onSaveClicked()));
    new QShortcut(QKeySequence("F3"), this, SLOT(onNewClicked()));
    new QShortcut(QKeySequence("Escape"), this, SIGNAL(backRequested()));
}

QWidget *TcsReceiptVoucherWidget::createHeaderBar()
{
    auto *headerCard = new QFrame(this);
    headerCard->setStyleSheet("QFrame { background-color: #FFFFFF; border: 1px solid #E2E8F0; border-radius: 8px; }");
    auto *layout = new QHBoxLayout(headerCard);
    layout->setContentsMargins(14, 10, 14, 10);

    auto *titleCol = new QVBoxLayout();
    titleCol->setSpacing(2);
    m_titleLabel = new QLabel("Receipt Voucher With TCS (Creation)", headerCard);
    m_titleLabel->setStyleSheet("font-size: 16px; font-weight: 800; color: #0F172A; border: none; background: transparent;");
    auto *subLabel = new QLabel("Section 206C(1H) collection on customer sales realization with net ledger balancing.", headerCard);
    subLabel->setStyleSheet("font-size: 11px; color: #64748B; border: none; background: transparent;");
    titleCol->addWidget(m_titleLabel);
    titleCol->addWidget(subLabel);
    layout->addLayout(titleCol);

    m_statusLabel = new QLabel("", headerCard);
    m_statusLabel->setStyleSheet("margin-left: 15px; border: none; background: transparent;");
    layout->addWidget(m_statusLabel, 1);

    auto *btnNew = new QPushButton("[F3] New", headerCard);
    btnNew->setCursor(Qt::PointingHandCursor);
    btnNew->setStyleSheet(
        "QPushButton { background-color: #FFFFFF; border: 1.5px solid #CBD5E1; padding: 6px 14px; border-radius: 6px; font-weight: 700; font-size: 11.5px; color: #334155; }"
        "QPushButton:hover { background-color: #F8FAFC; }"
    );
    connect(btnNew, &QPushButton::clicked, this, &TcsReceiptVoucherWidget::onNewClicked);
    layout->addWidget(btnNew);

    auto *btnSave = new QPushButton("[F2] Save Voucher", headerCard);
    btnSave->setCursor(Qt::PointingHandCursor);
    btnSave->setStyleSheet(
        "QPushButton { background-color: #16A34A; color: #FFFFFF; border: none; padding: 6px 18px; border-radius: 6px; font-weight: 800; font-size: 12px; }"
        "QPushButton:hover { background-color: #15803D; }"
    );
    connect(btnSave, &QPushButton::clicked, this, &TcsReceiptVoucherWidget::onSaveClicked);
    layout->addWidget(btnSave);

    auto *btnBack = new QPushButton("← [Esc] Back", headerCard);
    btnBack->setCursor(Qt::PointingHandCursor);
    btnBack->setStyleSheet(
        "QPushButton { background-color: #F1F5F9; border: 1px solid #CBD5E1; border-radius: 6px; padding: 6px 12px; font-weight: 700; color: #475569; font-size: 11.5px; }"
        "QPushButton:hover { background-color: #E2E8F0; }"
    );
    connect(btnBack, &QPushButton::clicked, this, &TcsReceiptVoucherWidget::backRequested);
    layout->addWidget(btnBack);

    return headerCard;
}

QWidget *TcsReceiptVoucherWidget::createParamsCard()
{
    auto *card = new QFrame(this);
    card->setStyleSheet(
        "QFrame { background-color: #FFFFFF; border: 1px solid #CBD5E1; border-radius: 8px; }"
        "QLabel { color: #475569; font-weight: 700; font-size: 11.5px; border: none; background: transparent; }"
        "QLineEdit { background-color: #FFFFFF; border: 1.5px solid #CBD5E1; border-radius: 6px; padding: 4px 8px; font-size: 12px; color: #0F172A; font-weight: 600; }"
        "QLineEdit:focus { border-color: #2563EB; background-color: #F8FAFC; }"
    );
    auto *layout = new QHBoxLayout(card);
    layout->setContentsMargins(12, 10, 12, 10);
    layout->setSpacing(16);

    // Receipt No
    auto *rcBox = new QVBoxLayout();
    rcBox->setSpacing(4);
    rcBox->addWidget(new QLabel("Receipt No. *", card));
    m_receiptNoEdit = new QLineEdit(card);
    connect(m_receiptNoEdit, &QLineEdit::textChanged, this, [this](const QString &t) {
        m_controller->setReceiptNo(t.toInt());
    });
    rcBox->addWidget(m_receiptNoEdit);
    layout->addLayout(rcBox);

    // Date
    auto *dtBox = new QVBoxLayout();
    dtBox->setSpacing(4);
    dtBox->addWidget(new QLabel("Date (F2) *", card));
    auto *dtRow = new QHBoxLayout();
    m_receiptDateEdit = new QLineEdit(card);
    m_receiptDateEdit->setReadOnly(true);
    dtRow->addWidget(m_receiptDateEdit);
    auto *btnDate = new QPushButton("📅", card);
    btnDate->setFixedWidth(28);
    btnDate->setStyleSheet("QPushButton { background-color: #F8FAFC; border: 1.5px solid #CBD5E1; border-radius: 4px; }");
    connect(btnDate, &QPushButton::clicked, this, [this]() {
        QDate cur = QDate::fromString(m_controller->receiptDate(), "yyyy-MM-dd");
        if (!cur.isValid()) cur = QDate::currentDate();
        QDate chosen = VoucherDateDialog::selectDate(this, cur);
        if (chosen.isValid()) {
            m_controller->setReceiptDate(chosen.toString("yyyy-MM-dd"));
            m_receiptDateEdit->setText(chosen.toString("dd-MM-yyyy"));
        }
    });
    dtRow->addWidget(btnDate);
    dtBox->addLayout(dtRow);
    layout->addLayout(dtBox);

    layout->addStretch(1);
    return card;
}

QWidget *TcsReceiptVoucherWidget::createMainFormCard()
{
    auto *card = new QFrame(this);
    card->setStyleSheet(
        "QFrame { background-color: #FFFFFF; border: 1px solid #E2E8F0; border-radius: 8px; }"
        "QLabel { color: #475569; font-weight: 700; font-size: 11.5px; border: none; background: transparent; }"
        "QLineEdit, QComboBox { background-color: #FFFFFF; border: 1.5px solid #CBD5E1; border-radius: 6px; padding: 4px 8px; font-size: 12px; color: #0F172A; font-weight: 600; }"
        "QLineEdit:focus, QComboBox:focus { border-color: #2563EB; background-color: #F8FAFC; }"
    );
    auto *grid = new QGridLayout(card);
    grid->setContentsMargins(16, 14, 16, 14);
    grid->setHorizontalSpacing(16);
    grid->setVerticalSpacing(10);

    auto *val = new QDoubleValidator(0.0, 999999999.0, 2, this);
    val->setNotation(QDoubleValidator::StandardNotation);

    // Row 0: Party Name & PAN
    grid->addWidget(new QLabel("Party Name (Customer) *:", card), 0, 0);
    m_partySearch = new AccountSearchBox(card);
    connect(m_partySearch, &AccountSearchBox::partySelectedWithId, this, [this](const QString &, int id) {
        m_controller->setPartyId(id);
    });
    grid->addWidget(m_partySearch, 0, 1, 1, 2);

    grid->addWidget(new QLabel("Party PAN:", card), 0, 3);
    m_panEdit = new QLineEdit(card);
    m_panEdit->setReadOnly(true);
    m_panEdit->setStyleSheet("background-color: #F8FAFC; color: #334155; font-weight: 700;");
    grid->addWidget(m_panEdit, 0, 4);

    // Row 1: Bank Account & Bank/Cash Amount
    grid->addWidget(new QLabel("Bank / Cash Account *:", card), 1, 0);
    m_bankSearch = new AccountSearchBox(card);
    connect(m_bankSearch, &AccountSearchBox::partySelectedWithId, this, [this](const QString &, int id) {
        m_controller->setBankLedgerId(id);
    });
    grid->addWidget(m_bankSearch, 1, 1, 1, 2);

    grid->addWidget(new QLabel("Bank / Cash Amount:", card), 1, 3);
    m_bankAmountEdit = new QLineEdit(card);
    m_bankAmountEdit->setValidator(val);
    m_bankAmountEdit->setAlignment(Qt::AlignRight);
    connect(m_bankAmountEdit, &QLineEdit::textChanged, this, [this](const QString &t) {
        if (!m_isSyncing) m_controller->setBankAmount(t.toDouble());
    });
    grid->addWidget(m_bankAmountEdit, 1, 4);

    // Row 2: Without TCS Amount & TCS Rate
    grid->addWidget(new QLabel("Without T.C.S. Amount *:", card), 2, 0);
    m_withoutTcsEdit = new QLineEdit(card);
    m_withoutTcsEdit->setValidator(val);
    m_withoutTcsEdit->setAlignment(Qt::AlignRight);
    connect(m_withoutTcsEdit, &QLineEdit::textChanged, this, [this](const QString &t) {
        if (!m_isSyncing) m_controller->setWithoutTcsAmount(t.toDouble());
    });
    grid->addWidget(m_withoutTcsEdit, 2, 1);

    grid->addWidget(new QLabel("T.C.S. Rate (%):", card), 2, 2);
    auto *rateRow = new QHBoxLayout();
    m_tcsRateEdit = new QLineEdit("0.100", card);
    m_tcsRateEdit->setFixedWidth(70);
    connect(m_tcsRateEdit, &QLineEdit::textChanged, this, [this](const QString &t) {
        if (!m_isSyncing) m_controller->setTcsRate(t.toDouble());
    });
    rateRow->addWidget(m_tcsRateEdit);
    grid->addLayout(rateRow, 2, 3);

    grid->addWidget(new QLabel("T.C.S. Amount:", card), 2, 4);
    m_tcsAmountEdit = new QLineEdit(card);
    m_tcsAmountEdit->setReadOnly(true);
    m_tcsAmountEdit->setAlignment(Qt::AlignRight);
    m_tcsAmountEdit->setStyleSheet("background-color: #F8FAFC; color: #D97706; font-weight: 700;");
    grid->addWidget(m_tcsAmountEdit, 2, 4);

    // Row 3: TCS Received & Net Receipt In Bank
    grid->addWidget(new QLabel("T.C.S. Received:", card), 3, 0);
    m_tcsReceivedCombo = new QComboBox(card);
    m_tcsReceivedCombo->addItems({"Yes", "No"});
    connect(m_tcsReceivedCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int idx) {
        if (!m_isSyncing) m_controller->setTcsReceived(idx == 0);
    });
    grid->addWidget(m_tcsReceivedCombo, 3, 1);

    grid->addWidget(new QLabel("Net Receipt In Bank:", card), 3, 3);
    m_netBankReceiptEdit = new QLineEdit(card);
    m_netBankReceiptEdit->setReadOnly(true);
    m_netBankReceiptEdit->setAlignment(Qt::AlignRight);
    m_netBankReceiptEdit->setStyleSheet("background-color: #F8FAFC; color: #0F172A; font-weight: 700;");
    grid->addWidget(m_netBankReceiptEdit, 3, 4);

    // Row 4: Less Interest Received
    grid->addWidget(new QLabel("Less: Interest Received:", card), 4, 0);
    m_interestAmtEdit = new QLineEdit(card);
    m_interestAmtEdit->setValidator(val);
    m_interestAmtEdit->setAlignment(Qt::AlignRight);
    connect(m_interestAmtEdit, &QLineEdit::textChanged, this, [this](const QString &t) {
        if (!m_isSyncing) m_controller->setInterestReceived(t.toDouble());
    });
    grid->addWidget(m_interestAmtEdit, 4, 1);
    m_interestLedgerBox = new AccountSearchBox(card);
    connect(m_interestLedgerBox, &AccountSearchBox::partySelectedWithId, this, [this](const QString &, int id) {
        m_controller->setInterestLedgerId(id);
    });
    grid->addWidget(m_interestLedgerBox, 4, 2, 1, 3);

    // Row 5: Add Discount / CD
    grid->addWidget(new QLabel("Add: Discount / CD:", card), 5, 0);
    m_discountAmtEdit = new QLineEdit(card);
    m_discountAmtEdit->setValidator(val);
    m_discountAmtEdit->setAlignment(Qt::AlignRight);
    connect(m_discountAmtEdit, &QLineEdit::textChanged, this, [this](const QString &t) {
        if (!m_isSyncing) m_controller->setDiscountAllowed(t.toDouble());
    });
    grid->addWidget(m_discountAmtEdit, 5, 1);
    m_discountLedgerBox = new AccountSearchBox(card);
    connect(m_discountLedgerBox, &AccountSearchBox::partySelectedWithId, this, [this](const QString &, int id) {
        m_controller->setDiscountLedgerId(id);
    });
    grid->addWidget(m_discountLedgerBox, 5, 2, 1, 3);

    // Row 6: Others
    grid->addWidget(new QLabel("Others Adjustment:", card), 6, 0);
    m_otherAmtEdit = new QLineEdit(card);
    m_otherAmtEdit->setValidator(val);
    m_otherAmtEdit->setAlignment(Qt::AlignRight);
    connect(m_otherAmtEdit, &QLineEdit::textChanged, this, [this](const QString &t) {
        if (!m_isSyncing) m_controller->setOtherAmount(t.toDouble());
    });
    grid->addWidget(m_otherAmtEdit, 6, 1);
    m_otherLedgerBox = new AccountSearchBox(card);
    connect(m_otherLedgerBox, &AccountSearchBox::partySelectedWithId, this, [this](const QString &, int id) {
        m_controller->setOtherLedgerId(id);
    });
    grid->addWidget(m_otherLedgerBox, 6, 2, 1, 3);

    // Row 7: Net Amount Credit to Party
    auto *creditPartyLabel = new QLabel("Net Amount Credit to Party:", card);
    creditPartyLabel->setStyleSheet("font-weight: 800; color: #15803D; font-size: 13px;");
    grid->addWidget(creditPartyLabel, 7, 0);
    m_netCreditPartyEdit = new QLineEdit(card);
    m_netCreditPartyEdit->setReadOnly(true);
    m_netCreditPartyEdit->setAlignment(Qt::AlignRight);
    m_netCreditPartyEdit->setStyleSheet(
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
    grid->addWidget(m_netCreditPartyEdit, 7, 1, 1, 4);

    // Row 8: Narration
    grid->addWidget(new QLabel("Narration:", card), 8, 0);
    m_narrationEdit = new QLineEdit(card);
    connect(m_narrationEdit, &QLineEdit::textChanged, m_controller, &TcsReceiptVoucherController::setNarration);
    grid->addWidget(m_narrationEdit, 8, 1, 1, 4);

    return card;
}

void TcsReceiptVoucherWidget::onNewClicked()
{
    m_controller->resetForm();
    m_partySearch->clear();
    m_bankSearch->clear();
    m_interestLedgerBox->clear();
    m_discountLedgerBox->clear();
    m_otherLedgerBox->clear();
    syncUiFromController();
}

void TcsReceiptVoucherWidget::onSaveClicked()
{
    if (m_controller->saveVoucher()) {
        emit voucherSaved(m_controller->receiptNo());
        onNewClicked();
    }
}

void TcsReceiptVoucherWidget::syncUiFromController()
{
    m_isSyncing = true;

    m_receiptNoEdit->setText(QString::number(m_controller->receiptNo()));

    QDate dt = QDate::fromString(m_controller->receiptDate(), "yyyy-MM-dd");
    m_receiptDateEdit->setText(dt.isValid() ? dt.toString("dd-MM-yyyy") : m_controller->receiptDate());

    m_panEdit->setText(m_controller->partyPan());
    m_bankAmountEdit->setText(m_controller->bankAmount() > 0.0 ? QString::number(m_controller->bankAmount(), 'f', 2) : "");
    m_withoutTcsEdit->setText(m_controller->withoutTcsAmount() > 0.0 ? QString::number(m_controller->withoutTcsAmount(), 'f', 2) : "");
    m_tcsRateEdit->setText(QString::number(m_controller->tcsRate(), 'f', 3));
    m_tcsAmountEdit->setText(QString::number(m_controller->tcsAmount(), 'f', 2));
    m_tcsReceivedCombo->setCurrentIndex(m_controller->tcsReceived() ? 0 : 1);
    m_netBankReceiptEdit->setText(QString::number(m_controller->netBankReceipt(), 'f', 2));

    m_interestAmtEdit->setText(m_controller->interestReceived() > 0.0 ? QString::number(m_controller->interestReceived(), 'f', 2) : "");
    m_discountAmtEdit->setText(m_controller->discountAllowed() > 0.0 ? QString::number(m_controller->discountAllowed(), 'f', 2) : "");
    m_otherAmtEdit->setText(m_controller->otherAmount() > 0.0 ? QString::number(m_controller->otherAmount(), 'f', 2) : "");

    m_netCreditPartyEdit->setText(QString::number(m_controller->netCreditToParty(), 'f', 2));
    m_narrationEdit->setText(m_controller->narration());

    if (m_controller->partyId() > 0) m_partySearch->setSelectedPartyId(m_controller->partyId());
    if (m_controller->bankLedgerId() > 0) m_bankSearch->setSelectedPartyId(m_controller->bankLedgerId());
    if (m_controller->interestLedgerId() > 0) m_interestLedgerBox->setSelectedPartyId(m_controller->interestLedgerId());
    if (m_controller->discountLedgerId() > 0) m_discountLedgerBox->setSelectedPartyId(m_controller->discountLedgerId());
    if (m_controller->otherLedgerId() > 0) m_otherLedgerBox->setSelectedPartyId(m_controller->otherLedgerId());

    m_isSyncing = false;
}

void TcsReceiptVoucherWidget::resetForm()
{
    onNewClicked();
}

void TcsReceiptVoucherWidget::loadVoucher(int voucherId)
{
    if (m_controller) {
        m_controller->loadVoucher(voucherId);
        syncUiFromController();
    }
}

void TcsReceiptVoucherWidget::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Escape) {
        emit backRequested();
        return;
    }
    QWidget::keyPressEvent(event);
}

} // namespace MahadevERP
