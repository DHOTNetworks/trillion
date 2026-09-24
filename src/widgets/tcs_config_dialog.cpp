#include "tcs_config_dialog.h"
#include "../database_manager.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QDoubleValidator>

namespace MahadevERP {

TcsConfigDialog::TcsConfigDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
    setAttribute(Qt::WA_TranslucentBackground, false);
    setAttribute(Qt::WA_StyledBackground, true);
    setupUi();
    loadSettings();
}

void TcsConfigDialog::setupUi()
{
    setFixedWidth(480);
    setStyleSheet(
        "QDialog {"
        "  background-color: #FFFFFF;"
        "  border: 2px solid #2563EB;"
        "  border-radius: 8px;"
        "}"
        "QLineEdit {"
        "  background-color: #FFFFFF;"
        "  border: 1.5px solid #CBD5E1;"
        "  border-radius: 6px;"
        "  color: #0F172A;"
        "  padding: 5px 8px;"
        "  font-size: 13px;"
        "  min-height: 26px;"
        "}"
        "QLineEdit:focus {"
        "  border: 1.5px solid #2563EB;"
        "}"
        "QLabel {"
        "  color: #475569;"
        "  font-size: 12px;"
        "  font-weight: 700;"
        "}"
    );

    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 16);
    mainLayout->setSpacing(12);

    auto *title = new QLabel("TCS Other Settings & Thresholds", this);
    title->setStyleSheet("color: #0F172A; font-size: 15px; font-weight: 800; padding: 12px; background-color: #F8FAFC; border-bottom: 1px solid #E2E8F0; border-top-left-radius: 6px; border-top-right-radius: 6px;");
    title->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(title);

    auto *formCard = new QWidget(this);
    formCard->setStyleSheet("background-color: #FFFFFF;");
    auto *grid = new QGridLayout(formCard);
    grid->setContentsMargins(20, 6, 20, 6);
    grid->setHorizontalSpacing(14);
    grid->setVerticalSpacing(12);

    auto *val = new QDoubleValidator(0.0, 999999999.0, 3, this);
    val->setNotation(QDoubleValidator::StandardNotation);

    grid->addWidget(new QLabel("TCS Threshold u/s 206C(1H) (₹):", formCard), 0, 0);
    m_thresholdEdit = new QLineEdit(formCard);
    m_thresholdEdit->setValidator(val);
    grid->addWidget(m_thresholdEdit, 0, 1);

    grid->addWidget(new QLabel("Standard Rate with PAN (%):", formCard), 1, 0);
    m_rateWithPanEdit = new QLineEdit(formCard);
    m_rateWithPanEdit->setValidator(val);
    grid->addWidget(m_rateWithPanEdit, 1, 1);

    grid->addWidget(new QLabel("Higher Rate without PAN (%):", formCard), 2, 0);
    m_rateWithoutPanEdit = new QLineEdit(formCard);
    m_rateWithoutPanEdit->setValidator(val);
    grid->addWidget(m_rateWithoutPanEdit, 2, 1);

    grid->addWidget(new QLabel("Default TCS Payable A/c:", formCard), 3, 0);
    m_tcsLedgerSearch = new AccountSearchBox(formCard);
    grid->addWidget(m_tcsLedgerSearch, 3, 1);

    mainLayout->addWidget(formCard);

    m_statusLabel = new QLabel("", this);
    m_statusLabel->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(m_statusLabel);

    auto *btnRow = new QHBoxLayout();
    btnRow->setContentsMargins(20, 0, 20, 0);

    auto *btnCancel = new QPushButton("Cancel [Esc]", this);
    btnCancel->setStyleSheet("QPushButton { background-color: #F1F5F9; border: 1px solid #CBD5E1; color: #475569; padding: 7px 18px; border-radius: 6px; font-weight: 600; font-size: 13px; } QPushButton:hover { background-color: #E2E8F0; }");
    btnCancel->setCursor(Qt::PointingHandCursor);
    connect(btnCancel, &QPushButton::clicked, this, &QDialog::reject);
    btnRow->addWidget(btnCancel);

    btnRow->addStretch(1);

    auto *btnSave = new QPushButton("Save Settings", this);
    btnSave->setStyleSheet("QPushButton { background-color: #16A34A; color: white; border: none; padding: 7px 20px; border-radius: 6px; font-weight: 700; font-size: 13px; } QPushButton:hover { background-color: #15803D; }");
    btnSave->setCursor(Qt::PointingHandCursor);
    connect(btnSave, &QPushButton::clicked, this, &TcsConfigDialog::onSaveClicked);
    btnRow->addWidget(btnSave);

    mainLayout->addLayout(btnRow);
}

void TcsConfigDialog::loadSettings()
{
    DatabaseManager &db = DatabaseManager::instance();
    m_thresholdEdit->setText(db.getSetting("tcs_threshold_limit", "5000000.00"));
    m_rateWithPanEdit->setText(db.getSetting("tcs_rate_with_pan", "0.100"));
    m_rateWithoutPanEdit->setText(db.getSetting("tcs_rate_without_pan", "1.000"));

    int tcsLedgerId = db.getSetting("tcs_payable_ledger_id", "0").toInt();
    if (tcsLedgerId > 0) {
        m_tcsLedgerSearch->setSelectedPartyId(tcsLedgerId);
    } else {
        QVariant def = db.executeScalar("SELECT id FROM parties WHERE name = 'TCS Payable A/c' LIMIT 1;");
        if (def.isValid() && !def.isNull()) m_tcsLedgerSearch->setSelectedPartyId(def.toInt());
    }
}

void TcsConfigDialog::onSaveClicked()
{
    DatabaseManager &db = DatabaseManager::instance();
    db.setSetting("tcs_threshold_limit", m_thresholdEdit->text().trimmed());
    db.setSetting("tcs_rate_with_pan", m_rateWithPanEdit->text().trimmed());
    db.setSetting("tcs_rate_without_pan", m_rateWithoutPanEdit->text().trimmed());
    db.setSetting("tcs_payable_ledger_id", QString::number(m_tcsLedgerSearch->selectedPartyId()));

    accept();
}

} // namespace MahadevERP
