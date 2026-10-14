#include "advance_payment_194q_widget.h"
#include "kbd_badge_button.h"
#include "erp_theme.h"
#include "voucher_date_dialog.h"
#include "../database_manager.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QDate>
#include <QShortcut>
#include <QDoubleValidator>
#include <QMessageBox>
#include <QKeyEvent>

namespace MahadevERP {

AdvancePayment194QWidget::AdvancePayment194QWidget(QWidget *parent)
    : QWidget(parent)
{
    setupUi();
    onNewClicked();
}

void AdvancePayment194QWidget::setupUi()
{
    auto *mainLayout = ErpTheme::makePageLayout(this);

    mainLayout->addWidget(createHeaderBar());
    mainLayout->addWidget(createFormCard(), 1);

    // Keyboard-first Enter chain across the whole form.
    connect(m_supplierSearch, &AccountSearchBox::returnPressed, this, [this]() {
        if (m_bankSearch) m_bankSearch->setFocus();
    });
    connect(m_bankSearch, &AccountSearchBox::returnPressed, this, [this]() {
        if (m_grossAdvanceEdit) { m_grossAdvanceEdit->setFocus(); m_grossAdvanceEdit->selectAll(); }
    });
    connect(m_grossAdvanceEdit, &QLineEdit::returnPressed, this, [this]() {
        if (m_tdsRateEdit) { m_tdsRateEdit->setFocus(); m_tdsRateEdit->selectAll(); }
    });
    connect(m_tdsRateEdit, &QLineEdit::returnPressed, this, [this]() {
        if (m_chequeNoEdit) { m_chequeNoEdit->setFocus(); m_chequeNoEdit->selectAll(); }
    });
    connect(m_chequeNoEdit, &QLineEdit::returnPressed, this, [this]() {
        if (m_narrationEdit) { m_narrationEdit->setFocus(); m_narrationEdit->selectAll(); }
    });

    QWidget::setTabOrder(m_voucherNoEdit, m_postInBooksCombo);
    QWidget::setTabOrder(m_postInBooksCombo, m_supplierSearch);
    QWidget::setTabOrder(m_supplierSearch, m_bankSearch);
    QWidget::setTabOrder(m_bankSearch, m_grossAdvanceEdit);
    QWidget::setTabOrder(m_grossAdvanceEdit, m_tdsRateEdit);
    QWidget::setTabOrder(m_tdsRateEdit, m_chequeNoEdit);
    QWidget::setTabOrder(m_chequeNoEdit, m_narrationEdit);

    new QShortcut(QKeySequence(Qt::ALT | Qt::Key_S), this, SLOT(onSaveClicked()));
    new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_S), this, SLOT(onSaveClicked()));
    new QShortcut(QKeySequence(Qt::ALT | Qt::Key_N), this, SLOT(onNewClicked()));
    new QShortcut(QKeySequence("Escape"), this, SIGNAL(backRequested()));
}

QWidget *AdvancePayment194QWidget::createHeaderBar()
{
    auto *headerCard = new QFrame(this);
    headerCard->setStyleSheet(ErpTheme::headerCardStyle());
    auto *layout = new QHBoxLayout(headerCard);
    layout->setContentsMargins(14, 10, 14, 10);

    auto *titleCol = new QVBoxLayout();
    titleCol->setSpacing(2);
    m_titleLabel = new QLabel("Advance Payment Voucher U/S 194-Q (Creation)", headerCard);
    m_titleLabel->setStyleSheet(ErpTheme::headerTitleStyle(15));
    auto *subLabel = new QLabel("Withhold tax at 0.1% on purchase advance payments above statutory limits.", headerCard);
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

    auto *btnNew = new KbdBadgeButton("New", "Alt+N", headerCard);
    connect(btnNew, &QPushButton::clicked, this, &AdvancePayment194QWidget::onNewClicked);
    layout->addWidget(btnNew);

    auto *btnSave = new KbdBadgeButton("Save Voucher", "Alt+S", headerCard);
    btnSave->setPrimaryColor(ErpTheme::kGreen, ErpTheme::kGreenDark);
    btnSave->setTextColor(ErpTheme::kPaper);
    connect(btnSave, &QPushButton::clicked, this, &AdvancePayment194QWidget::onSaveClicked);
    layout->addWidget(btnSave);

    auto *btnBack = new KbdBadgeButton("Back", "Esc", headerCard);
    connect(btnBack, &QPushButton::clicked, this, &AdvancePayment194QWidget::backRequested);
    layout->addWidget(btnBack);

    return headerCard;
}

QWidget *AdvancePayment194QWidget::createFormCard()
{
    auto *card = new QFrame(this);
    card->setStyleSheet(ErpTheme::filterBarStyle());
    auto *grid = new QGridLayout(card);
    grid->setContentsMargins(16, 14, 16, 14);
    grid->setHorizontalSpacing(16);
    grid->setVerticalSpacing(12);

    auto *val = new QDoubleValidator(0.0, 999999999.0, 2, this);
    val->setNotation(QDoubleValidator::StandardNotation);

    // Row 0: Voucher No & Date
    grid->addWidget(new QLabel("Voucher No. *:", card), 0, 0);
    m_voucherNoEdit = new QLineEdit(card);
    grid->addWidget(m_voucherNoEdit, 0, 1);

    grid->addWidget(new QLabel("Date (F2) *:", card), 0, 2);
    auto *dtRow = new QHBoxLayout();
    m_voucherDateEdit = new QLineEdit(card);
    m_voucherDateEdit->setReadOnly(true);
    dtRow->addWidget(m_voucherDateEdit);
    auto *btnDt = new QPushButton("\u2026", card);
    btnDt->setMinimumWidth(28);
    btnDt->setToolTip("Pick date (F2)");
    btnDt->setStyleSheet(QString("QPushButton { background-color: %1; border: 1.5px solid %2; border-radius: 4px; }").arg(ErpTheme::kCanvas.name(), ErpTheme::kLine.name()));
    connect(btnDt, &QPushButton::clicked, this, [this]() {
        QDate cur = QDate::fromString(m_voucherDateEdit->text(), "dd-MM-yyyy");
        if (!cur.isValid()) cur = QDate::currentDate();
        QDate chosen = VoucherDateDialog::selectDate(this, cur);
        if (chosen.isValid()) m_voucherDateEdit->setText(chosen.toString("dd-MM-yyyy"));
    });
    dtRow->addWidget(btnDt);
    grid->addLayout(dtRow, 0, 3);

    grid->addWidget(new QLabel("Post In Books:", card), 0, 4);
    m_postInBooksCombo = new QComboBox(card);
    m_postInBooksCombo->addItems({"Yes", "No"});
    grid->addWidget(m_postInBooksCombo, 0, 5);

    // Row 1: Supplier & PAN
    grid->addWidget(new QLabel("Supplier (Seller) *:", card), 1, 0);
    m_supplierSearch = new AccountSearchBox(card);
    connect(m_supplierSearch, &AccountSearchBox::partySelectedWithId, this, [this](const QString &, int id) {
        DatabaseManager &db = DatabaseManager::instance();
        QVariant pan = db.executeScalar("SELECT pan FROM parties WHERE id = ?;", {id});
        m_supplierPanEdit->setText(pan.isValid() ? pan.toString().trimmed().toUpper() : "");
    });
    grid->addWidget(m_supplierSearch, 1, 1, 1, 3);

    grid->addWidget(new QLabel("Supplier PAN:", card), 1, 4);
    m_supplierPanEdit = new QLineEdit(card);
    m_supplierPanEdit->setReadOnly(true);
    m_supplierPanEdit->setStyleSheet(QString("background-color: %1; color: %2; font-weight: 700;").arg(ErpTheme::kCanvas.name(), ErpTheme::kLabelSlate.name()));
    grid->addWidget(m_supplierPanEdit, 1, 5);

    // Row 2: Bank Account & Gross Advance
    grid->addWidget(new QLabel("Bank / Cash Account *:", card), 2, 0);
    m_bankSearch = new AccountSearchBox(card);
    grid->addWidget(m_bankSearch, 2, 1, 1, 3);

    grid->addWidget(new QLabel("Gross Advance Amount *:", card), 2, 4);
    m_grossAdvanceEdit = new QLineEdit(card);
    m_grossAdvanceEdit->setValidator(val);
    m_grossAdvanceEdit->setAlignment(Qt::AlignRight);
    connect(m_grossAdvanceEdit, &QLineEdit::textChanged, this, &AdvancePayment194QWidget::recalculate);
    grid->addWidget(m_grossAdvanceEdit, 2, 5);

    // Row 3: TDS Rate & Tax Amount
    grid->addWidget(new QLabel("TDS Rate u/s 194-Q (%):", card), 3, 0);
    m_tdsRateEdit = new QLineEdit("0.100", card);
    connect(m_tdsRateEdit, &QLineEdit::textChanged, this, &AdvancePayment194QWidget::recalculate);
    grid->addWidget(m_tdsRateEdit, 3, 1);

    grid->addWidget(new QLabel("TDS Deducted Amount:", card), 3, 2);
    m_tdsAmountEdit = new QLineEdit(card);
    m_tdsAmountEdit->setReadOnly(true);
    m_tdsAmountEdit->setAlignment(Qt::AlignRight);
    m_tdsAmountEdit->setStyleSheet(QString("background-color: %1; color: %2; font-weight: 700;").arg(ErpTheme::kCanvas.name(), ErpTheme::kAmber.name()));
    grid->addWidget(m_tdsAmountEdit, 3, 3);

    // Row 4: Net Payment Amount
    auto *netLbl = new QLabel("Net Payment Amount:", card);
    netLbl->setStyleSheet(QString("font-weight: 800; color: %1; font-size: 13px;").arg(ErpTheme::kGreenDark.name()));
    grid->addWidget(netLbl, 4, 0);
    m_netPaymentEdit = new QLineEdit(card);
    m_netPaymentEdit->setReadOnly(true);
    m_netPaymentEdit->setAlignment(Qt::AlignRight);
    m_netPaymentEdit->setStyleSheet(
        QString("QLineEdit { font-size: 15px; font-weight: 800; color: %1; background-color: %2; border: 1.5px solid %3; border-radius: 6px; padding: 4px 8px; }")
        .arg(ErpTheme::kGreenDark.name(), ErpTheme::kGreenBg.name(), ErpTheme::kGreenBorder.name()));
    grid->addWidget(m_netPaymentEdit, 4, 1, 1, 3);

    // Row 5: Cheque No & Date
    grid->addWidget(new QLabel("Cheque / Ref No:", card), 5, 0);
    m_chequeNoEdit = new QLineEdit(card);
    grid->addWidget(m_chequeNoEdit, 5, 1);

    grid->addWidget(new QLabel("Cheque Date:", card), 5, 2);
    auto *cqRow = new QHBoxLayout();
    m_chequeDateEdit = new QLineEdit(card);
    m_chequeDateEdit->setReadOnly(true);
    cqRow->addWidget(m_chequeDateEdit);
    auto *btnCq = new QPushButton("\u2026", card);
    btnCq->setMinimumWidth(28);
    btnCq->setToolTip("Pick cheque date");
    btnCq->setStyleSheet(QString("QPushButton { background-color: %1; border: 1.5px solid %2; border-radius: 4px; }").arg(ErpTheme::kCanvas.name(), ErpTheme::kLine.name()));
    connect(btnCq, &QPushButton::clicked, this, [this]() {
        QDate cur = QDate::fromString(m_chequeDateEdit->text(), "dd-MM-yyyy");
        if (!cur.isValid()) cur = QDate::currentDate();
        QDate chosen = VoucherDateDialog::selectDate(this, cur);
        if (chosen.isValid()) m_chequeDateEdit->setText(chosen.toString("dd-MM-yyyy"));
    });
    cqRow->addWidget(btnCq);
    grid->addLayout(cqRow, 5, 3);

    // Row 6: Narration
    grid->addWidget(new QLabel("Narration:", card), 6, 0);
    m_narrationEdit = new QLineEdit(card);
    grid->addWidget(m_narrationEdit, 6, 1, 1, 5);

    return card;
}

void AdvancePayment194QWidget::onNewClicked()
{
    m_editVoucherId = 0;
    if (m_titleLabel) m_titleLabel->setText("Advance Payment Voucher U/S 194-Q (Creation)");
    if (m_alterBadge) m_alterBadge->setVisible(false);
    m_supplierSearch->clear();
    m_bankSearch->clear();
    m_supplierPanEdit->clear();
    m_grossAdvanceEdit->clear();
    m_tdsRateEdit->setText("0.100");
    m_tdsAmountEdit->clear();
    m_netPaymentEdit->clear();
    m_chequeNoEdit->clear();
    m_narrationEdit->setText("Advance payment to supplier u/s 194-Q");
    m_postInBooksCombo->setCurrentIndex(0);

    QDate today = QDate::currentDate();
    m_voucherDateEdit->setText(today.toString("dd-MM-yyyy"));
    m_chequeDateEdit->setText(today.toString("dd-MM-yyyy"));

    DatabaseManager &db = DatabaseManager::instance();
    QVariant nextNo = db.executeScalar("SELECT COALESCE(MAX(voucher_no), 0) + 1 FROM advance_payments_194q;");
    m_voucherNoEdit->setText(nextNo.toString());

    m_statusLabel->clear();
    if (m_supplierSearch) m_supplierSearch->setFocus();
}

void AdvancePayment194QWidget::onSaveClicked()
{
    int vchNo = m_voucherNoEdit->text().toInt();
    if (vchNo <= 0) {
        QMessageBox::warning(this, "Validation Error", "Please provide a valid Voucher Number.");
        return;
    }

    int partyId = m_supplierSearch->selectedPartyId();
    if (partyId <= 0) {
        QMessageBox::warning(this, "Validation Error", "Please select a Supplier ledger.");
        return;
    }

    int bankId = m_bankSearch->selectedPartyId();
    if (bankId <= 0) {
        QMessageBox::warning(this, "Validation Error", "Please select a Bank / Cash ledger.");
        return;
    }

    double gross = m_grossAdvanceEdit->text().toDouble();
    if (gross <= 0.0) {
        QMessageBox::warning(this, "Validation Error", "Gross Advance Amount must be greater than zero.");
        return;
    }

    double rate = m_tdsRateEdit->text().toDouble();
    double tax = m_tdsAmountEdit->text().toDouble();
    double net = m_netPaymentEdit->text().toDouble();

    QDate vDate = QDate::fromString(m_voucherDateEdit->text(), "dd-MM-yyyy");
    QString vDateStr = vDate.isValid() ? vDate.toString("yyyy-MM-dd") : QDate::currentDate().toString("yyyy-MM-dd");

    QDate cqDate = QDate::fromString(m_chequeDateEdit->text(), "dd-MM-yyyy");
    QString cqDateStr = cqDate.isValid() ? cqDate.toString("yyyy-MM-dd") : vDateStr;

    bool postBooks = (m_postInBooksCombo->currentIndex() == 0);

    DatabaseManager &db = DatabaseManager::instance();

    // Resolve FY + TDS payable ledger up front (canonical schema requires
    // fy_id + tds_payable_ledger_id NOT NULL on every row).
    QVariant fyRow = db.executeScalar("SELECT id FROM financial_years WHERE is_active = 1 LIMIT 1;");
    if (!fyRow.isValid() || fyRow.isNull())
        fyRow = db.executeScalar("SELECT id FROM financial_years ORDER BY id DESC LIMIT 1;");
    int fyId = (fyRow.isValid() && !fyRow.isNull()) ? fyRow.toInt() : 1;

    int tdsLedgerId = 0;
    if (tax > 0.0) {
        QVariant tdsAcc = db.executeScalar("SELECT id FROM parties WHERE name = 'TDS u/s 194-Q Payable A/c' LIMIT 1;");
        if (tdsAcc.isValid() && !tdsAcc.isNull()) {
            tdsLedgerId = tdsAcc.toInt();
        } else {
            db.executeNonQuery(
                "INSERT INTO parties (name, group_name, party_type) VALUES ('TDS u/s 194-Q Payable A/c', 'Duties & Taxes (GST)', 'Tax');"
            );
            tdsLedgerId = db.executeScalar("SELECT last_insert_rowid();").toInt();
        }
    }
    if (tdsLedgerId <= 0) {
        // Zero-tax advances still need a valid ledger id for the NOT NULL
        // column — resolve-or-create the canonical ledger regardless.
        QVariant tdsAcc = db.executeScalar("SELECT id FROM parties WHERE name = 'TDS u/s 194-Q Payable A/c' LIMIT 1;");
        if (tdsAcc.isValid() && !tdsAcc.isNull()) {
            tdsLedgerId = tdsAcc.toInt();
        } else {
            db.executeNonQuery(
                "INSERT INTO parties (name, group_name, party_type) VALUES ('TDS u/s 194-Q Payable A/c', 'Duties & Taxes (GST)', 'Tax');"
            );
            tdsLedgerId = db.executeScalar("SELECT last_insert_rowid();").toInt();
        }
    }

    db.beginTransaction();

    int pId = 0;
    if (postBooks) {
        QVariant fyVal = db.executeScalar("SELECT id, year_name FROM financial_years WHERE is_active = 1 LIMIT 1;");
        if (!fyVal.isValid()) fyVal = db.executeScalar("SELECT id, year_name FROM financial_years ORDER BY id DESC LIMIT 1;");
        int fyId = fyVal.isValid() ? fyVal.toMap().value("id", 1).toInt() : 1;
        QString fyLabel = fyVal.isValid() ? fyVal.toMap().value("year_name", "").toString() : "";

        QString supplierName = db.executeScalar("SELECT name FROM parties WHERE id = ?;", {partyId}).toString();
        QString bankName = db.executeScalar("SELECT name FROM parties WHERE id = ?;", {bankId}).toString();
        QString vchFormatted = QString("ADV-%1").arg(vchNo, 4, 10, QChar('0'));
        QString vchNarration = m_narrationEdit->text().trimmed().isEmpty()
            ? QString("Advance Payment u/s 194-Q to %1").arg(supplierName)
            : m_narrationEdit->text().trimmed();

        // Resolve TDS 194-Q Payable Account
        int tdsId = 0;
        if (tax > 0.0) {
            QVariant tdsAcc = db.executeScalar("SELECT id FROM parties WHERE name = 'TDS u/s 194-Q Payable A/c' LIMIT 1;");
            if (tdsAcc.isValid() && !tdsAcc.isNull()) {
                tdsId = tdsAcc.toInt();
            } else {
                db.executeNonQuery(
                    "INSERT INTO parties (name, group_name, party_type) VALUES ('TDS u/s 194-Q Payable A/c', 'Duties & Taxes (GST)', 'Tax');"
                );
                tdsId = db.executeScalar("SELECT last_insert_rowid();").toInt();
            }
        }

        bool okH = db.executeNonQuery(
            "INSERT INTO vouchers (fy_id, financial_year, voucher_no, voucher_date, voucher_type, legacy_type, party_id, ledger_id, party_name, account_type, amount, narration) "
            "VALUES (?, ?, ?, ?, 'PAYMENT', 'BANK', ?, ?, ?, 'Dr', ?, ?);",
            {fyId, fyLabel, vchFormatted, vDateStr, partyId, partyId, supplierName, gross, vchNarration}
        );
        if (!okH) {
            db.rollback();
            QMessageBox::critical(this, "Database Error", "Failed to create payment voucher header.");
            return;
        }

        pId = db.lastInsertedId();

        int rowNo = 1;
        // 1. Debit Supplier with Gross Advance
        db.executeNonQuery(
            "INSERT INTO transactions (fy_id, financial_year, voucher_no, voucher_date, voucher_type, trans_type, account_code, party_id, party_name, opposing_account, dr_cr, amount, narration, taxable_amount, tds_amount, tds_rate, row_no) "
            "VALUES (?, ?, ?, ?, 'PAYMENT', 'BANK', ?, ?, ?, ?, 'Dr', ?, ?, ?, ?, ?, ?);",
            {fyId, fyLabel, vchFormatted, vDateStr, partyId, partyId, supplierName, bankName, gross, vchNarration, gross, tax, rate, rowNo++}
        );

        // 2. Credit TDS 194-Q Payable A/c
        if (tax > 0.0 && tdsId > 0) {
            db.executeNonQuery(
                "INSERT INTO transactions (fy_id, financial_year, voucher_no, voucher_date, voucher_type, trans_type, account_code, party_id, party_name, opposing_account, dr_cr, amount, narration, taxable_amount, tds_amount, tds_rate, row_no) "
                "VALUES (?, ?, ?, ?, 'PAYMENT', 'BANK', ?, ?, ?, ?, 'Cr', ?, ?, ?, ?, ?, ?);",
                {fyId, fyLabel, vchFormatted, vDateStr, tdsId, tdsId, "TDS u/s 194-Q Payable A/c", supplierName, tax, "TDS u/s 194-Q Withheld", gross, tax, rate, rowNo++}
            );
        }

        // 3. Credit Bank A/c with Net Payment
        db.executeNonQuery(
            "INSERT INTO transactions (fy_id, financial_year, voucher_no, voucher_date, voucher_type, trans_type, account_code, party_id, party_name, opposing_account, dr_cr, amount, narration, taxable_amount, tds_amount, tds_rate, row_no) "
            "VALUES (?, ?, ?, ?, 'PAYMENT', 'BANK', ?, ?, ?, ?, 'Cr', ?, ?, ?, 0, 0, ?);",
            {fyId, fyLabel, vchFormatted, vDateStr, bankId, bankId, bankName, supplierName, net, vchNarration, gross, rowNo++}
        );
    }

    bool okAdv = db.executeNonQuery(
        "INSERT INTO advance_payments_194q (fy_id, voucher_no, voucher_date, supplier_id, bank_ledger_id, gross_advance_amount, "
        "tds_rate, tds_amount, net_payment_amount, tds_payable_ledger_id, cheque_no, cheque_date, post_in_books, voucher_id, narration) "
        "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);",
        {fyId, vchNo, vDateStr, partyId, bankId, gross, rate, tax, net, tdsLedgerId, m_chequeNoEdit->text().trimmed(),
         cqDateStr, postBooks ? 1 : 0, pId > 0 ? pId : QVariant(), m_narrationEdit->text().trimmed()}
    );

    if (!okAdv) {
        db.rollback();
        QMessageBox::critical(this, "Database Error", "Failed to record Advance Payment 194-Q.");
        return;
    }

    db.commit();
    emit voucherSaved(vchNo);
    onNewClicked();
}

void AdvancePayment194QWidget::recalculate()
{
    double gross = m_grossAdvanceEdit->text().toDouble();
    double rate = m_tdsRateEdit->text().toDouble();
    double tax = (gross * rate) / 100.0;
    double net = gross - tax;

    m_tdsAmountEdit->setText(QString::number(tax, 'f', 2));
    m_netPaymentEdit->setText(QString::number(net, 'f', 2));
}

void AdvancePayment194QWidget::resetForm()
{
    onNewClicked();
}

void AdvancePayment194QWidget::loadVoucher(int voucherId)
{
    DatabaseManager &db = DatabaseManager::instance();
    QVariantList rows = db.executeQuery(
        "SELECT * FROM advance_payments_194q WHERE id = ? LIMIT 1;", {voucherId}
    );
    if (rows.isEmpty()) return;
    QVariantMap m = rows.first().toMap();
    m_editVoucherId = voucherId;
    if (m_titleLabel) m_titleLabel->setText("Advance Payment Voucher U/S 194-Q (Alteration)");
    if (m_alterBadge) m_alterBadge->setVisible(true);
    m_voucherNoEdit->setText(m.value("voucher_no").toString());
    QDate dt = QDate::fromString(m.value("voucher_date").toString(), "yyyy-MM-dd");
    m_voucherDateEdit->setText(dt.isValid() ? dt.toString("dd-MM-yyyy") : m.value("voucher_date").toString());
    m_postInBooksCombo->setCurrentIndex(m.value("post_in_books").toInt() == 1 ? 0 : 1);
    m_supplierSearch->setSelectedPartyId(m.value("supplier_id").toInt());
    m_bankSearch->setSelectedPartyId(m.value("bank_ledger_id").toInt());
    m_grossAdvanceEdit->setText(QString::number(m.value("gross_advance_amount").toDouble(), 'f', 2));
    m_tdsRateEdit->setText(QString::number(m.value("tds_rate").toDouble(), 'f', 3));
    m_chequeNoEdit->setText(m.value("cheque_no").toString());
    QDate cqDt = QDate::fromString(m.value("cheque_date").toString(), "yyyy-MM-dd");
    m_chequeDateEdit->setText(cqDt.isValid() ? cqDt.toString("dd-MM-yyyy") : m.value("cheque_date").toString());
    m_narrationEdit->setText(m.value("narration").toString());
    recalculate();
    if (m_supplierSearch) m_supplierSearch->setFocus();
}

void AdvancePayment194QWidget::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Escape) {
        emit backRequested();
        return;
    }
    if (event->key() == Qt::Key_F3) {
        onNewClicked();
        return;
    }
    if ((event->key() == Qt::Key_S && event->modifiers() & Qt::ControlModifier)) {
        onSaveClicked();
        return;
    }
    if (event->key() == Qt::Key_F2) {
        // F2 = date field (label says "Date (F2)"), never save.
        if (m_voucherDateEdit) m_voucherDateEdit->setFocus();
        return;
    }
    QWidget::keyPressEvent(event);
}

} // namespace MahadevERP
