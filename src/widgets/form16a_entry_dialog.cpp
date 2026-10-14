#include "form16a_entry_dialog.h"
#include "../services/db_service.h"
#include "kbd_badge_button.h"
#include "erp_theme.h"
#include "../database_manager.h"
#include "voucher_date_dialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QDate>
#include <QDoubleValidator>
#include <QShortcut>

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
    setMinimumWidth(540);
setStyleSheet(
        QString("QDialog { background-color: %1; border: 2px solid %2; border-radius: 8px; }"
        "QLineEdit, QComboBox { background-color: %1; border: 1.5px solid %3; border-radius: 6px; color: %4; padding: 5px 8px; font-size: 13px; min-height: 26px; }"
        "QLineEdit:focus, QComboBox:focus { border: 1.5px solid %2; }"
        "QLineEdit[readOnly=\"true\"] { background-color: %5; color: %6; border: 1.5px solid %7; }"
        "QLabel { color: %8; font-size: 12px; font-weight: 700; }"
        "QComboBox QAbstractItemView { background-color: %1; color: %4; selection-background-color: %2; selection-color: #FFFFFF; }")
        .arg(ErpTheme::kPaper.name(), ErpTheme::kBlue.name(), ErpTheme::kLine.name(), ErpTheme::kNavy.name(),
             ErpTheme::kHeaderBg.name(), ErpTheme::kMuted.name(), ErpTheme::kCardBorder.name(), ErpTheme::kLabelSlate.name()));

    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 16);
    mainLayout->setSpacing(12);

    auto *title = new QLabel("Receive Form-16A / Form-27D Certificate", this);
    title->setStyleSheet(QString("color: %1; font-size: 15px; font-weight: 800; padding: 12px; background-color: %2; border-bottom: 1px solid %3; border-top-left-radius: 6px; border-top-right-radius: 6px;").arg(ErpTheme::kNavy.name(), ErpTheme::kCanvas.name(), ErpTheme::kCardBorder.name()));
    title->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(title);

    auto *formWidget = new QWidget(this);
    formWidget->setStyleSheet(QString("background-color: %1;").arg(ErpTheme::kPaper.name()));
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
    m_dateEdit->setFocusPolicy(Qt::ClickFocus);
    m_dateEdit->setText(QDate::currentDate().toString("dd-MM-yyyy"));
    dtRow->addWidget(m_dateEdit);
    auto *btnDt = new QPushButton("\u2026", formWidget);
    btnDt->setMinimumWidth(32);
    btnDt->setToolTip("Pick date");
    btnDt->setStyleSheet(QString("background-color: %1; border: 1.5px solid %2; border-radius: 6px; padding: 4px; font-size: 13px;").arg(ErpTheme::kCanvas.name(), ErpTheme::kLine.name()));
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

    auto *btnCancel = new KbdBadgeButton("Cancel", "Esc", this);
    connect(btnCancel, &QPushButton::clicked, this, &QDialog::reject);
    btnRow->addWidget(btnCancel);

    btnRow->addStretch(1);

    m_saveBtn = new KbdBadgeButton("Save Certificate", "Ctrl+S", this);
    m_saveBtn->setPrimaryColor(ErpTheme::kGreen, ErpTheme::kGreenDark);
    m_saveBtn->setTextColor(ErpTheme::kPaper);
    connect(m_saveBtn, &QPushButton::clicked, this, &Form16AEntryDialog::onSaveClicked);
    btnRow->addWidget(m_saveBtn);

    mainLayout->addLayout(btnRow);

    // Keyboard-first: full Enter chain + Tab order + Ctrl+S.
    connect(m_certNoEdit, &QLineEdit::returnPressed, this, [this]() {
        if (m_partySearch) m_partySearch->setFocus();
    });
    connect(m_partySearch, &AccountSearchBox::returnPressed, this, [this]() {
        if (m_grossAmtEdit) { m_grossAmtEdit->setFocus(); m_grossAmtEdit->selectAll(); }
    });
    connect(m_grossAmtEdit, &QLineEdit::returnPressed, this, [this]() {
        if (m_tdsAmtEdit) { m_tdsAmtEdit->setFocus(); m_tdsAmtEdit->selectAll(); }
    });
    connect(m_tdsAmtEdit, &QLineEdit::returnPressed, this, [this]() {
        if (m_remarksEdit) { m_remarksEdit->setFocus(); m_remarksEdit->selectAll(); }
    });
    connect(m_remarksEdit, &QLineEdit::returnPressed, this, [this]() {
        if (m_saveBtn) m_saveBtn->setFocus();
    });
    QWidget::setTabOrder(m_certNoEdit, m_partySearch);
    QWidget::setTabOrder(m_partySearch, m_panEdit);
    QWidget::setTabOrder(m_panEdit, m_quarterCombo);
    QWidget::setTabOrder(m_quarterCombo, m_grossAmtEdit);
    QWidget::setTabOrder(m_grossAmtEdit, m_tdsAmtEdit);
    QWidget::setTabOrder(m_tdsAmtEdit, m_matchedCombo);
    QWidget::setTabOrder(m_matchedCombo, m_remarksEdit);
    QWidget::setTabOrder(m_remarksEdit, m_saveBtn);
    new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_S), this, SLOT(onSaveClicked()));
    connect(&m_saveWatcher, &QFutureWatcher<bool>::finished, this, &Form16AEntryDialog::onSaveFinished);
    if (m_certNoEdit) m_certNoEdit->setFocus();
}

void Form16AEntryDialog::onSaveClicked()
{
    QString certNo = m_certNoEdit->text().trimmed();
    int partyId = m_partySearch->selectedPartyId();
    double tdsAmt = m_tdsAmtEdit->text().toDouble();
    double grossAmt = m_grossAmtEdit->text().toDouble();

    auto markError = [this](const QString &msg, QWidget *focus) {
        m_statusLabel->setText(msg);
        m_statusLabel->setStyleSheet(QString("color: %1; font-weight: 600;").arg(ErpTheme::kRed.name()));
        if (focus) focus->setFocus();
    };
    if (certNo.isEmpty()) {
        markError("Certificate Number cannot be empty.", m_certNoEdit);
        return;
    }
    if (partyId <= 0) {
        markError("Please select a Deductor (Customer).", m_partySearch);
        return;
    }
    if (tdsAmt <= 0.0) {
        markError("TDS Deducted Amount must be greater than zero.", m_tdsAmtEdit);
        return;
    }
    if (m_saving) return;

    QDate dt = QDate::fromString(m_dateEdit->text(), "dd-MM-yyyy");
    const QString dtIso = dt.isValid() ? dt.toString("yyyy-MM-dd") : QDate::currentDate().toString("yyyy-MM-dd");

    // Snapshot for the worker (no widgets past this line).
    const QString cCertNo = certNo;
    const int cPartyId = partyId;
    const QString cPan = m_panEdit->text().trimmed();
    const QString cQtr = m_quarterCombo->currentText().left(2);
    const double cGross = grossAmt, cTds = tdsAmt;
    const int cMatched = m_matchedCombo->currentIndex() == 0 ? 1 : 0;
    const QString cRemarks = m_remarksEdit->text().trimmed();

    m_saving = true;
    if (m_saveBtn) m_saveBtn->setEnabled(false);
    m_statusLabel->setText("Saving certificate...");
    m_statusLabel->setStyleSheet(QString("color: %1; font-weight: 600;").arg(ErpTheme::kMuted.name()));
    m_saveWatcher.setFuture(MahadevERP::DbService::instance().transact(
        [=](MahadevERP::DbTx& tx) {
            QVariantList fyRows = tx.query(
                "SELECT id FROM financial_years WHERE is_active = 1 LIMIT 1;", {});
            if (fyRows.isEmpty()) {
                fyRows = tx.query(
                    "SELECT id FROM financial_years ORDER BY id DESC LIMIT 1;", {});
            }
            if (fyRows.isEmpty()) return false;
            const int fyId = fyRows.first().toMap().value("id", 1).toInt();
            const int tdsRecId = tx.scalar(
                "SELECT id FROM parties WHERE name = 'TDS Receivable A/c' LIMIT 1;").toInt();
            return tx.exec(
                "INSERT INTO received_forms_16a ("
                "fy_id, certificate_no, receipt_date, party_id, pan_of_deductor, quarter, "
                "gross_amount, tds_amount, tds_receivable_ledger_id, matched_with_26as, remarks"
                ") VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);",
                {fyId, cCertNo, dtIso, cPartyId, cPan, cQtr,
                 cGross, cTds, tdsRecId > 0 ? tdsRecId : QVariant(), cMatched, cRemarks});
        }));
}

void Form16AEntryDialog::onSaveFinished() {
    m_saving = false;
    if (m_saveBtn) m_saveBtn->setEnabled(true);
    const bool ok = m_saveWatcher.future().resultCount() > 0 && m_saveWatcher.result();
    if (!ok) {
        m_statusLabel->setText("Save failed — certificate was not recorded.");
        m_statusLabel->setStyleSheet(QString("color: %1; font-weight: 600;").arg(ErpTheme::kRed.name()));
        return;
    }
    emit certificateSaved();
    accept();
}

} // namespace MahadevERP
