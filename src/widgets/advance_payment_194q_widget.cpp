#include "advance_payment_194q_widget.h"
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
    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(14, 12, 14, 12);
    mainLayout->setSpacing(10);

    mainLayout->addWidget(createHeaderBar());
    mainLayout->addWidget(createFormCard(), 1);

    new QShortcut(QKeySequence("F2"), this, SLOT(onSaveClicked()));
    new QShortcut(QKeySequence("Ctrl+S"), this, SLOT(onSaveClicked()));
    new QShortcut(QKeySequence("F3"), this, SLOT(onNewClicked()));
    new QShortcut(QKeySequence("Escape"), this, SIGNAL(backRequested()));
}

QWidget *AdvancePayment194QWidget::createHeaderBar()
{
    auto *headerCard = new QFrame(this);
    headerCard->setStyleSheet("QFrame { background-color: #FFFFFF; border: 1px solid #E2E8F0; border-radius: 8px; }");
    auto *layout = new QHBoxLayout(headerCard);
    layout->setContentsMargins(14, 10, 14, 10);

    auto *titleCol = new QVBoxLayout();
    titleCol->setSpacing(2);
    auto *title = new QLabel("Advance Payment Voucher U/S 194-Q (Creation)", headerCard);
    title->setStyleSheet("font-size: 16px; font-weight: 800; color: #0F172A; border: none; background: transparent;");
    auto *subLabel = new QLabel("Withhold tax at 0.1% on purchase advance payments above statutory limits.", headerCard);
    subLabel->setStyleSheet("font-size: 11px; color: #64748B; border: none; background: transparent;");
    titleCol->addWidget(title);
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
    connect(btnNew, &QPushButton::clicked, this, &AdvancePayment194QWidget::onNewClicked);
    layout->addWidget(btnNew);

    auto *btnSave = new QPushButton("[F2] Save Voucher", headerCard);
    btnSave->setCursor(Qt::PointingHandCursor);
    btnSave->setStyleSheet(
        "QPushButton { background-color: #16A34A; color: #FFFFFF; border: none; padding: 6px 18px; border-radius: 6px; font-weight: 800; font-size: 12px; }"
        "QPushButton:hover { background-color: #15803D; }"
    );
    connect(btnSave, &QPushButton::clicked, this, &AdvancePayment194QWidget::onSaveClicked);
    layout->addWidget(btnSave);

    auto *btnBack = new QPushButton("← [Esc] Back", headerCard);
    btnBack->setCursor(Qt::PointingHandCursor);
    btnBack->setStyleSheet(
        "QPushButton { background-color: #F1F5F9; border: 1px solid #CBD5E1; border-radius: 6px; padding: 6px 12px; font-weight: 700; color: #475569; font-size: 11.5px; }"
        "QPushButton:hover { background-color: #E2E8F0; }"
    );
    connect(btnBack, &QPushButton::clicked, this, &AdvancePayment194QWidget::backRequested);
    layout->addWidget(btnBack);

    return headerCard;
}

QWidget *AdvancePayment194QWidget::createFormCard()
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
    auto *btnDt = new QPushButton("📅", card);
    btnDt->setFixedWidth(28);
    btnDt->setStyleSheet("QPushButton { background-color: #F8FAFC; border: 1.5px solid #CBD5E1; border-radius: 4px; }");
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
    m_supplierPanEdit->setStyleSheet("background-color: #F8FAFC; color: #334155; font-weight: 700;");
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
    m_tdsAmountEdit->setStyleSheet("background-color: #F8FAFC; color: #D97706; font-weight: 700;");
    grid->addWidget(m_tdsAmountEdit, 3, 3);

    // Row 4: Net Payment Amount
    auto *netLbl = new QLabel("Net Payment Amount:", card);
    netLbl->setStyleSheet("font-weight: 800; color: #15803D; font-size: 13px;");
    grid->addWidget(netLbl, 4, 0);
    m_netPaymentEdit = new QLineEdit(card);
    m_netPaymentEdit->setReadOnly(true);
    m_netPaymentEdit->setAlignment(Qt::AlignRight);
    m_netPaymentEdit->setStyleSheet(
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
    auto *btnCq = new QPushButton("📅", card);
    btnCq->setFixedWidth(28);
    btnCq->setStyleSheet("QPushButton { background-color: #F8FAFC; border: 1.5px solid #CBD5E1; border-radius: 4px; }");
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
    db.beginTransaction();

    int pId = 0;
    if (postBooks) {
        QVariant fyVal = db.executeScalar("SELECT id FROM financial_years WHERE is_active = 1 LIMIT 1;");
        if (!fyVal.isValid()) fyVal = db.executeScalar("SELECT id FROM financial_years ORDER BY id DESC LIMIT 1;");
        int fyId = fyVal.isValid() ? fyVal.toInt() : 1;

        QVariant nextVchNo = db.executeScalar("SELECT COALESCE(MAX(voucher_no), 0) + 1 FROM vouchers WHERE voucher_type = 'PAYMENT';");
        int newNo = nextVchNo.toInt();

        bool okH = db.executeNonQuery(
            "INSERT INTO vouchers (voucher_type, voucher_no, voucher_date, financial_year_id, narration, total_amount) "
            "VALUES ('PAYMENT', ?, ?, ?, ?, ?);",
            {newNo, vDateStr, fyId, m_narrationEdit->text().trimmed(), gross}
        );
        if (!okH) {
            db.rollback();
            QMessageBox::critical(this, "Database Error", "Failed to create journal voucher header.");
            return;
        }

        pId = db.lastInsertedId();

        // 1. Debit Supplier with Gross Advance
        db.executeNonQuery("INSERT INTO voucher_items (voucher_id, party_id, debit_amount, credit_amount, particulars) VALUES (?, ?, ?, 0.0, 'Advance Payment u/s 194-Q');",
                      {pId, partyId, gross});

        // 2. Credit TDS 194-Q Payable A/c
        if (tax > 0.0) {
            QVariant tdsAcc = db.executeScalar("SELECT id FROM parties WHERE name = 'TDS u/s 194-Q Payable A/c' LIMIT 1;");
            int tdsId = tdsAcc.isValid() ? tdsAcc.toInt() : 0;
            if (tdsId > 0) {
                db.executeNonQuery("INSERT INTO voucher_items (voucher_id, party_id, debit_amount, credit_amount, particulars) VALUES (?, ?, 0.0, ?, 'TDS u/s 194-Q Withheld');",
                              {pId, tdsId, tax});
            }
        }

        // 3. Credit Bank A/c with Net Payment
        db.executeNonQuery("INSERT INTO voucher_items (voucher_id, party_id, debit_amount, credit_amount, particulars) VALUES (?, ?, 0.0, ?, 'Bank Payment');",
                      {pId, bankId, net});
    }

    bool okAdv = db.executeNonQuery(
        "INSERT INTO advance_payments_194q (voucher_no, voucher_date, supplier_id, bank_id, gross_advance, "
        "tds_rate, tds_amount, net_payment, cheque_no, cheque_date, post_in_books, voucher_id, narration) "
        "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);",
        {vchNo, vDateStr, partyId, bankId, gross, rate, tax, net, m_chequeNoEdit->text().trimmed(),
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
    m_voucherNoEdit->setText(m.value("voucher_no").toString());
    QDate dt = QDate::fromString(m.value("voucher_date").toString(), "yyyy-MM-dd");
    m_voucherDateEdit->setText(dt.isValid() ? dt.toString("dd-MM-yyyy") : m.value("voucher_date").toString());
    m_postInBooksCombo->setCurrentIndex(m.value("post_in_books").toInt() == 1 ? 0 : 1);
    m_supplierSearch->setSelectedPartyId(m.value("supplier_id").toInt());
    m_bankSearch->setSelectedPartyId(m.value("bank_id").toInt());
    m_grossAdvanceEdit->setText(QString::number(m.value("gross_advance").toDouble(), 'f', 2));
    m_tdsRateEdit->setText(QString::number(m.value("tds_rate").toDouble(), 'f', 3));
    m_chequeNoEdit->setText(m.value("cheque_no").toString());
    QDate cqDt = QDate::fromString(m.value("cheque_date").toString(), "yyyy-MM-dd");
    m_chequeDateEdit->setText(cqDt.isValid() ? cqDt.toString("dd-MM-yyyy") : m.value("cheque_date").toString());
    m_narrationEdit->setText(m.value("narration").toString());
    recalculate();
}

void AdvancePayment194QWidget::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Escape) {
        emit backRequested();
        return;
    }
    QWidget::keyPressEvent(event);
}

} // namespace MahadevERP
