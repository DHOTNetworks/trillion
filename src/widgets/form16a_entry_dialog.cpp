#include "form16a_entry_dialog.h"
#include "../database_manager.h"
#include "voucher_date_dialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QDate>
#include <QDoubleValidator>

namespace MahadevERP {

Form16AEntryDialog::Form16AEntryDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
    setAttribute(Qt::WA_TranslucentBackground, false);
    setAttribute(Qt::WA_StyledBackground, true);
    setupUi();
}

void Form16AEntryDialog::setupUi()
{
    setFixedWidth(540);
    setStyleSheet(
        "QDialog {"
        "  background-color: #FFFFFF;"
        "  border: 2px solid #2563EB;"
        "  border-radius: 8px;"
        "}"
        "QLineEdit, QComboBox {"
        "  background-color: #FFFFFF;"
        "  border: 1.5px solid #CBD5E1;"
        "  border-radius: 6px;"
        "  color: #0F172A;"
        "  padding: 5px 8px;"
        "  font-size: 13px;"
        "  min-height: 26px;"
        "}"
        "QLineEdit:focus, QComboBox:focus {"
        "  border: 1.5px solid #2563EB;"
        "}"
        "QLineEdit[readOnly=\"true\"] {"
        "  background-color: #F1F5F9;"
        "  color: #64748B;"
        "  border: 1.5px solid #E2E8F0;"
        "}"
        "QLabel {"
        "  color: #475569;"
        "  font-size: 12px;"
        "  font-weight: 700;"
        "}"
        "QComboBox QAbstractItemView {"
        "  background-color: #FFFFFF;"
        "  color: #0F172A;"
        "  selection-background-color: #EFF6FF;"
        "  selection-color: #1E3A8A;"
        "}"
    );

    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 16);
    mainLayout->setSpacing(12);

    auto *title = new QLabel("Receive Form-16A / Form-27D Certificate", this);
    title->setStyleSheet("color: #0F172A; font-size: 15px; font-weight: 800; padding: 12px; background-color: #F8FAFC; border-bottom: 1px solid #E2E8F0; border-top-left-radius: 6px; border-top-right-radius: 6px;");
    title->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(title);

    auto *formWidget = new QWidget(this);
    formWidget->setStyleSheet("background-color: #FFFFFF;");
    auto *grid = new QGridLayout(formWidget);
    grid->setContentsMargins(20, 6, 20, 0);
    grid->setHorizontalSpacing(14);
    grid->setVerticalSpacing(10);

    auto *val = new QDoubleValidator(0.0, 999999999.0, 2, this);
    val->setNotation(QDoubleValidator::StandardNotation);

    // Row 0: Certificate No & Date
    grid->addWidget(new QLabel("Certificate No. *:", formWidget), 0, 0);
    m_certNoEdit = new QLineEdit(formWidget);
    grid->addWidget(m_certNoEdit, 0, 1);

    grid->addWidget(new QLabel("Date:", formWidget), 0, 2);
    auto *dtRow = new QHBoxLayout();
    dtRow->setSpacing(4);
    m_dateEdit = new QLineEdit(formWidget);
    m_dateEdit->setReadOnly(true);
    m_dateEdit->setText(QDate::currentDate().toString("dd-MM-yyyy"));
    dtRow->addWidget(m_dateEdit);
    auto *btnDt = new QPushButton("📅", formWidget);
    btnDt->setFixedWidth(32);
    btnDt->setStyleSheet("background-color: #F8FAFC; border: 1.5px solid #CBD5E1; border-radius: 6px; padding: 4px; font-size: 13px;");
    btnDt->setCursor(Qt::PointingHandCursor);
    connect(btnDt, &QPushButton::clicked, this, [this]() {
        QDate cur = QDate::fromString(m_dateEdit->text(), "dd-MM-yyyy");
        if (!cur.isValid()) cur = QDate::currentDate();
        QDate chosen = VoucherDateDialog::selectDate(this, cur);
        if (chosen.isValid()) m_dateEdit->setText(chosen.toString("dd-MM-yyyy"));
    });
    dtRow->addWidget(btnDt);
    grid->addLayout(dtRow, 0, 3);

    // Row 1: Deductor Party & PAN
    grid->addWidget(new QLabel("Deductor (Customer) *:", formWidget), 1, 0);
    m_partySearch = new AccountSearchBox(formWidget);
    connect(m_partySearch, &AccountSearchBox::partySelectedWithId, this, [this](const QString &, int id) {
        DatabaseManager &db = DatabaseManager::instance();
        QVariant pan = db.executeScalar("SELECT pan FROM parties WHERE id = ?;", {id});
        m_panEdit->setText(pan.isValid() ? pan.toString().trimmed().toUpper() : "");
    });
    grid->addWidget(m_partySearch, 1, 1, 1, 3);

    // Row 2: PAN & Quarter
    grid->addWidget(new QLabel("Deductor PAN:", formWidget), 2, 0);
    m_panEdit = new QLineEdit(formWidget);
    m_panEdit->setReadOnly(true);
    grid->addWidget(m_panEdit, 2, 1);

    grid->addWidget(new QLabel("Quarter:", formWidget), 2, 2);
    m_quarterCombo = new QComboBox(formWidget);
    m_quarterCombo->addItems({"Q1 (Apr-Jun)", "Q2 (Jul-Sep)", "Q3 (Oct-Dec)", "Q4 (Jan-Mar)"});
    grid->addWidget(m_quarterCombo, 2, 3);

    // Row 3: Gross Amount & TDS Deducted
    grid->addWidget(new QLabel("Gross Amount:", formWidget), 3, 0);
    m_grossAmtEdit = new QLineEdit(formWidget);
    m_grossAmtEdit->setValidator(val);
    m_grossAmtEdit->setAlignment(Qt::AlignRight);
    grid->addWidget(m_grossAmtEdit, 3, 1);

    grid->addWidget(new QLabel("TDS Deducted *:", formWidget), 3, 2);
    m_tdsAmtEdit = new QLineEdit(formWidget);
    m_tdsAmtEdit->setValidator(val);
    m_tdsAmtEdit->setAlignment(Qt::AlignRight);
    grid->addWidget(m_tdsAmtEdit, 3, 3);

    // Row 4: Matched with 26AS & Remarks
    grid->addWidget(new QLabel("26AS / AIS Match:", formWidget), 4, 0);
    m_matchedCombo = new QComboBox(formWidget);
    m_matchedCombo->addItems({"Yes", "Pending / No"});
    grid->addWidget(m_matchedCombo, 4, 1);

    grid->addWidget(new QLabel("Remarks:", formWidget), 4, 2);
    m_remarksEdit = new QLineEdit(formWidget);
    grid->addWidget(m_remarksEdit, 4, 3);

    mainLayout->addWidget(formWidget);

    m_statusLabel = new QLabel("", this);
    m_statusLabel->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(m_statusLabel);

    // Buttons
    auto *btnRow = new QHBoxLayout();
    btnRow->setContentsMargins(20, 0, 20, 0);
    btnRow->setSpacing(12);

    auto *btnCancel = new QPushButton("Cancel [Esc]", this);
    btnCancel->setStyleSheet("QPushButton { background-color: #F1F5F9; border: 1px solid #CBD5E1; color: #475569; padding: 7px 18px; border-radius: 6px; font-weight: 600; font-size: 13px; } QPushButton:hover { background-color: #E2E8F0; }");
    btnCancel->setCursor(Qt::PointingHandCursor);
    connect(btnCancel, &QPushButton::clicked, this, &QDialog::reject);
    btnRow->addWidget(btnCancel);

    btnRow->addStretch(1);

    auto *btnSave = new QPushButton("Save Certificate", this);
    btnSave->setStyleSheet("QPushButton { background-color: #16A34A; color: white; border: none; padding: 7px 20px; border-radius: 6px; font-weight: 700; font-size: 13px; } QPushButton:hover { background-color: #15803D; }");
    btnSave->setCursor(Qt::PointingHandCursor);
    connect(btnSave, &QPushButton::clicked, this, &Form16AEntryDialog::onSaveClicked);
    btnRow->addWidget(btnSave);

    mainLayout->addLayout(btnRow);
}

void Form16AEntryDialog::onSaveClicked()
{
    QString certNo = m_certNoEdit->text().trimmed();
    int partyId = m_partySearch->selectedPartyId();
    double tdsAmt = m_tdsAmtEdit->text().toDouble();
    double grossAmt = m_grossAmtEdit->text().toDouble();

    if (certNo.isEmpty()) {
        m_statusLabel->setText("Certificate Number cannot be empty.");
        m_statusLabel->setStyleSheet("color: #ef4444; font-weight: 600;");
        return;
    }
    if (partyId <= 0) {
        m_statusLabel->setText("Please select a Deductor (Customer).");
        m_statusLabel->setStyleSheet("color: #ef4444; font-weight: 600;");
        return;
    }
    if (tdsAmt <= 0.0) {
        m_statusLabel->setText("TDS Deducted Amount must be greater than zero.");
        m_statusLabel->setStyleSheet("color: #ef4444; font-weight: 600;");
        return;
    }

    DatabaseManager &db = DatabaseManager::instance();
    int fyId = 1;
    QVariant fyRow = db.executeScalar("SELECT id FROM financial_years WHERE is_active = 1 LIMIT 1;");
    if (fyRow.isValid() && !fyRow.isNull()) fyId = fyRow.toInt();

    QDate dt = QDate::fromString(m_dateEdit->text(), "dd-MM-yyyy");
    QString dtIso = dt.isValid() ? dt.toString("yyyy-MM-dd") : QDate::currentDate().toString("yyyy-MM-dd");

    QString qtr = m_quarterCombo->currentText().left(2);
    int matched = m_matchedCombo->currentIndex() == 0 ? 1 : 0;

    int tdsRecId = db.executeScalar("SELECT id FROM parties WHERE name = 'TDS Receivable A/c' LIMIT 1;").toInt();

    db.executeNonQuery(
        "INSERT INTO received_forms_16a ("
        "fy_id, certificate_no, receipt_date, party_id, pan_of_deductor, quarter, "
        "gross_amount, tds_amount, tds_receivable_ledger_id, matched_with_26as, remarks"
        ") VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);",
        {
            fyId, certNo, dtIso, partyId, m_panEdit->text().trimmed(), qtr,
            grossAmt, tdsAmt, tdsRecId > 0 ? tdsRecId : QVariant(), matched, m_remarksEdit->text().trimmed()
        }
    );

    emit certificateSaved();
    accept();
}

} // namespace MahadevERP
