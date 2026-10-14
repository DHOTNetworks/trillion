#include "tcs_receipt_voucher_widget.h"
#include "kbd_badge_button.h"
#include "erp_theme.h"
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
        m_statusLabel->setStyleSheet(QString("font-weight: 700; font-size: 12px; color: %1;").arg(m_controller->isError() ? ErpTheme::kRed.name() : ErpTheme::kGreen.name()));
    });

    onNewClicked();
}

void TcsReceiptVoucherWidget::setupUi()
{
    auto *mainLayout = ErpTheme::makePageLayout(this);

    // 1. Header Bar
    mainLayout->addWidget(createHeaderBar());

    // 2. Parameters Card
    mainLayout->addWidget(createParamsCard());

    // 3. Main Form Card
    mainLayout->addWidget(createMainFormCard(), 1);

    // Keyboard-first Enter chain across the whole form.
    connect(m_partySearch, &AccountSearchBox::returnPressed, this, [this]() {
        if (m_bankSearch) m_bankSearch->setFocus();
    });
    connect(m_bankSearch, &AccountSearchBox::returnPressed, this, [this]() {
        if (m_bankAmountEdit) { m_bankAmountEdit->setFocus(); m_bankAmountEdit->selectAll(); }
    });
    connect(m_bankAmountEdit, &QLineEdit::returnPressed, this, [this]() {
        if (m_withoutTcsEdit) { m_withoutTcsEdit->setFocus(); m_withoutTcsEdit->selectAll(); }
    });
    connect(m_withoutTcsEdit, &QLineEdit::returnPressed, this, [this]() {
        if (m_tcsRateEdit) { m_tcsRateEdit->setFocus(); m_tcsRateEdit->selectAll(); }
    });
    connect(m_tcsRateEdit, &QLineEdit::returnPressed, this, [this]() {
        if (m_interestAmtEdit) { m_interestAmtEdit->setFocus(); m_interestAmtEdit->selectAll(); }
    });
    connect(m_interestAmtEdit, &QLineEdit::returnPressed, this, [this]() {
        if (m_interestLedgerBox) m_interestLedgerBox->setFocus();
    });
    connect(m_interestLedgerBox, &AccountSearchBox::returnPressed, this, [this]() {
        if (m_discountAmtEdit) { m_discountAmtEdit->setFocus(); m_discountAmtEdit->selectAll(); }
    });
    connect(m_discountAmtEdit, &QLineEdit::returnPressed, this, [this]() {
        if (m_discountLedgerBox) m_discountLedgerBox->setFocus();
    });
    connect(m_discountLedgerBox, &AccountSearchBox::returnPressed, this, [this]() {
        if (m_otherAmtEdit) { m_otherAmtEdit->setFocus(); m_otherAmtEdit->selectAll(); }
    });
    connect(m_otherAmtEdit, &QLineEdit::returnPressed, this, [this]() {
        if (m_otherLedgerBox) m_otherLedgerBox->setFocus();
    });
    connect(m_otherLedgerBox, &AccountSearchBox::returnPressed, this, [this]() {
        if (m_narrationEdit) { m_narrationEdit->setFocus(); m_narrationEdit->selectAll(); }
    });
    connect(m_narrationEdit, &QLineEdit::returnPressed, this, [this]() {
        if (m_saveBtn) m_saveBtn->setFocus();
    });

    QWidget::setTabOrder(m_partySearch, m_bankSearch);
    QWidget::setTabOrder(m_bankSearch, m_bankAmountEdit);
    QWidget::setTabOrder(m_bankAmountEdit, m_withoutTcsEdit);
    QWidget::setTabOrder(m_withoutTcsEdit, m_tcsRateEdit);
    QWidget::setTabOrder(m_tcsRateEdit, m_tcsReceivedCombo);
    QWidget::setTabOrder(m_tcsReceivedCombo, m_interestAmtEdit);
    QWidget::setTabOrder(m_interestAmtEdit, m_interestLedgerBox);
    QWidget::setTabOrder(m_interestLedgerBox, m_discountAmtEdit);
    QWidget::setTabOrder(m_discountAmtEdit, m_discountLedgerBox);
    QWidget::setTabOrder(m_discountLedgerBox, m_otherAmtEdit);
    QWidget::setTabOrder(m_otherAmtEdit, m_otherLedgerBox);
    QWidget::setTabOrder(m_otherLedgerBox, m_narrationEdit);
    QWidget::setTabOrder(m_narrationEdit, m_saveBtn);

    // Shortcuts
    new QShortcut(QKeySequence(Qt::ALT | Qt::Key_S), this, SLOT(onSaveClicked()));
    new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_S), this, SLOT(onSaveClicked()));
    connect(&m_saveWatcher, &QFutureWatcher<bool>::finished, this, &TcsReceiptVoucherWidget::onSaveFinished);
    new QShortcut(QKeySequence(Qt::ALT | Qt::Key_N), this, SLOT(onNewClicked()));
    new QShortcut(QKeySequence(Qt::Key_F3), this, SLOT(onNewClicked()));
    new QShortcut(QKeySequence(Qt::Key_Escape), this, SIGNAL(backRequested()));
    auto* f2Shortcut = new QShortcut(QKeySequence(Qt::Key_F2), this);
    connect(f2Shortcut, &QShortcut::activated, this, [this]() {
        QDate cur = QDate::fromString(m_controller->receiptDate(), "yyyy-MM-dd");
        if (!cur.isValid()) cur = QDate::currentDate();
        QDate chosen = VoucherDateDialog::selectDate(this, cur);
        if (chosen.isValid()) {
            m_controller->setReceiptDate(chosen.toString("yyyy-MM-dd"));
            m_receiptDateEdit->setText(chosen.toString("dd-MM-yyyy"));
        }
    });
}

QWidget *TcsReceiptVoucherWidget::createHeaderBar()
{
    auto *headerCard = new QFrame(this);
    headerCard->setStyleSheet(ErpTheme::headerCardStyle());
    auto *layout = new QHBoxLayout(headerCard);
    layout->setContentsMargins(12, 4, 12, 4);

    auto *titleCol = new QVBoxLayout();
    titleCol->setSpacing(2);
    m_titleLabel = new QLabel("Receipt Voucher With TCS (Creation)", headerCard);
    m_titleLabel->setStyleSheet(ErpTheme::headerTitleStyle(15));
    auto *subLabel = new QLabel("Section 206C(1H) collection on customer sales realization with net ledger balancing.", headerCard);
    subLabel->setStyleSheet(ErpTheme::mutedLabelStyle(11));
    titleCol->addWidget(m_titleLabel);
    titleCol->addWidget(subLabel);
    layout->addLayout(titleCol);

    m_alterBadge = new QLabel("ALTERATION", headerCard);
    m_alterBadge->setStyleSheet(ErpTheme::badgeStyle("danger"));
    m_alterBadge->setVisible(false);
    layout->addWidget(m_alterBadge);

    m_statusLabel = new QLabel("", headerCard);
    m_statusLabel->setStyleSheet("margin-left: 15px; border: none; background: transparent;");
    layout->addWidget(m_statusLabel, 1);

    auto *btnNew = new KbdBadgeButton("New", "F3", headerCard);
    connect(btnNew, &QPushButton::clicked, this, &TcsReceiptVoucherWidget::onNewClicked);
    layout->addWidget(btnNew);

    m_saveBtn = new KbdBadgeButton("Save Voucher", "Ctrl+S", headerCard);
    m_saveBtn->setPrimaryColor(ErpTheme::kGreen, ErpTheme::kGreenDark);
    m_saveBtn->setTextColor(ErpTheme::kPaper);
    connect(m_saveBtn, &QPushButton::clicked, this, &TcsReceiptVoucherWidget::onSaveClicked);
    layout->addWidget(m_saveBtn);

    auto *btnBack = new KbdBadgeButton("Back", "Esc", headerCard);
    connect(btnBack, &QPushButton::clicked, this, &TcsReceiptVoucherWidget::backRequested);
    layout->addWidget(btnBack);

    return headerCard;
}

QWidget *TcsReceiptVoucherWidget::createParamsCard()
{
    auto *card = new QFrame(this);
    card->setStyleSheet(ErpTheme::filterBarStyle());
    auto *layout = new QHBoxLayout(card);
    layout->setContentsMargins(12, 10, 12, 10);
    layout->setSpacing(16);

    // Receipt No
    auto *rcBox = new QVBoxLayout();
    rcBox->setSpacing(4);
    rcBox->addWidget(new QLabel("Receipt No. *", card));
    m_receiptNoEdit = new QLineEdit(card);
    // Auto-generated and non-editable on both new and alter pages (the
    // textChanged binding into the controller still fires on programmatic set).
    m_receiptNoEdit->setReadOnly(true);
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
m_receiptDateEdit->setFocusPolicy(Qt::ClickFocus);
    dtRow->addWidget(m_receiptDateEdit);
    auto *btnDate = new QPushButton("\u2026", card);
    btnDate->setMinimumWidth(28);
    btnDate->setToolTip("Pick date (F2)");
    btnDate->setStyleSheet(QString("QPushButton { background-color: %1; border: 1.5px solid %2; border-radius: 4px; }").arg(ErpTheme::kCanvas.name(), ErpTheme::kLine.name()));
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
    card->setStyleSheet(ErpTheme::filterBarStyle());
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
    m_panEdit->setStyleSheet(QString("background-color: %1; color: %2; font-weight: 700;").arg(ErpTheme::kCanvas.name(), ErpTheme::kLabelSlate.name()));
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
    m_tcsRateEdit = new QLineEdit("0.100", card);
    m_tcsRateEdit->setMinimumWidth(70);
    connect(m_tcsRateEdit, &QLineEdit::textChanged, this, [this](const QString &t) {
        if (!m_isSyncing) m_controller->setTcsRate(t.toDouble());
    });
    grid->addWidget(m_tcsRateEdit, 2, 3);

    grid->addWidget(new QLabel("T.C.S. Amount:", card), 2, 4);
    m_tcsAmountEdit = new QLineEdit(card);
    m_tcsAmountEdit->setReadOnly(true);
    m_tcsAmountEdit->setAlignment(Qt::AlignRight);
    m_tcsAmountEdit->setStyleSheet(QString("background-color: %1; color: %2; font-weight: 700;").arg(ErpTheme::kCanvas.name(), ErpTheme::kAmber.name()));
    grid->addWidget(m_tcsAmountEdit, 2, 5);

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
    m_netBankReceiptEdit->setStyleSheet(QString("background-color: %1; color: %2; font-weight: 700;").arg(ErpTheme::kCanvas.name(), ErpTheme::kNavy.name()));
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
    creditPartyLabel->setStyleSheet(QString("font-weight: 800; color: %1; font-size: 13px;").arg(ErpTheme::kGreenDark.name()));
    grid->addWidget(creditPartyLabel, 7, 0);
    m_netCreditPartyEdit = new QLineEdit(card);
    m_netCreditPartyEdit->setReadOnly(true);
    m_netCreditPartyEdit->setAlignment(Qt::AlignRight);
    m_netCreditPartyEdit->setStyleSheet(
        QString("QLineEdit { font-size: 15px; font-weight: 800; color: %1; background-color: %2; border: 1.5px solid %3; border-radius: 6px; padding: 4px 8px; }")
        .arg(ErpTheme::kGreenDark.name(), ErpTheme::kGreenBg.name(), ErpTheme::kGreenBorder.name()));
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
    if (m_saving) return; // write flight pending: receipt_no not committed yet
    if (m_titleLabel) m_titleLabel->setText("Receipt Voucher With TCS (Creation)");
    if (m_alterBadge) m_alterBadge->setVisible(false);
    m_controller->resetForm();
    m_partySearch->clear();
    m_bankSearch->clear();
    m_interestLedgerBox->clear();
    m_discountLedgerBox->clear();
    m_otherLedgerBox->clear();
    syncUiFromController();
    if (m_partySearch) m_partySearch->setFocus();
}

void TcsReceiptVoucherWidget::onSaveClicked()
{
    if (m_saving) return; // write flight already posted
    if (!m_controller) return;
    m_saving = true;
    if (m_saveBtn) m_saveBtn->setEnabled(false);
    m_saveWatcher.setFuture(m_controller->saveVoucherAsync());
}

void TcsReceiptVoucherWidget::onSaveFinished() {
    m_saving = false;
    if (m_saveBtn) m_saveBtn->setEnabled(true);
    const bool ok = m_saveWatcher.future().resultCount() > 0 && m_saveWatcher.result();
    if (ok) {
        emit voucherSaved(m_controller->receiptNo());
        onNewClicked();
    }
    // On failure the controller status label already explains (sync
    // validation) or the books are untouched (worker refusal) — the form
    // keeps its data so nothing is lost.
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
        if (m_titleLabel) m_titleLabel->setText("Receipt Voucher With TCS (Alteration)");
        if (m_alterBadge) m_alterBadge->setVisible(true);
        if (m_partySearch) m_partySearch->setFocus();
    }
}

void TcsReceiptVoucherWidget::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Escape) {
        emit backRequested();
        return;
    }
    if (event->key() == Qt::Key_F3) {
        onNewClicked();
        return;
    }
    if (event->key() == Qt::Key_S && (event->modifiers() & Qt::ControlModifier)) {
        onSaveClicked();
        return;
    }
    QWidget::keyPressEvent(event);
}

} // namespace MahadevERP
