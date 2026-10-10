#include "bill_series_dialog.h"
#include "voucher_common.h"
#include "../engine/fiscal_year_helper.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QShortcut>
#include <QKeySequence>

namespace MahadevERP {

BillSeriesDialog::BillSeriesDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
    setAttribute(Qt::WA_StyledBackground, true);
    setupUi();
    loadSettings();
}

void BillSeriesDialog::setupUi()
{
    setFixedWidth(520);
    setStyleSheet(
        "QDialog {"
        "  background-color: #FFFFFF;"
        "  border: 2px solid #2563EB;"
        "  border-radius: 8px;"
        "}"
        "QLineEdit, QComboBox, QSpinBox {"
        "  background-color: #FFFFFF;"
        "  border: 1.5px solid #CBD5E1;"
        "  border-radius: 6px;"
        "  color: #0F172A;"
        "  padding: 5px 8px;"
        "  font-size: 13px;"
        "  min-height: 26px;"
        "}"
        "QLineEdit:focus, QComboBox:focus, QSpinBox:focus {"
        "  border: 1.5px solid #2563EB;"
        "}"
        "QLabel {"
        "  color: #475569;"
        "  font-size: 12px;"
        "  font-weight: 700;"
        "}"
        "QCheckBox {"
        "  color: #0F172A;"
        "  font-size: 13px;"
        "  font-weight: 600;"
        "}"
    );

    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 16);
    mainLayout->setSpacing(12);

    auto* title = new QLabel("Bill Series — Sale Invoice Numbering", this);
    title->setStyleSheet("color: #0F172A; font-size: 15px; font-weight: 800; padding: 12px; background-color: #F8FAFC; border-bottom: 1px solid #E2E8F0; border-top-left-radius: 6px; border-top-right-radius: 6px;");
    title->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(title);

    auto* formCard = new QWidget(this);
    formCard->setStyleSheet("background-color: #FFFFFF;");
    auto* grid = new QGridLayout(formCard);
    grid->setContentsMargins(20, 6, 20, 6);
    grid->setHorizontalSpacing(14);
    grid->setVerticalSpacing(10);

    grid->addWidget(new QLabel("Prefix (e.g. MRI, STC):", formCard), 0, 0);
    m_prefixEdit = new QLineEdit(formCard);
    m_prefixEdit->setMaxLength(12);
    m_prefixEdit->setPlaceholderText("MRI");
    grid->addWidget(m_prefixEdit, 0, 1);

    grid->addWidget(new QLabel("Separator after prefix:", formCard), 1, 0);
    m_sepAEdit = new QLineEdit(formCard);
    m_sepAEdit->setMaxLength(3);
    m_sepAEdit->setPlaceholderText("/");
    grid->addWidget(m_sepAEdit, 1, 1);

    grid->addWidget(new QLabel("Financial-year token:", formCard), 2, 0);
    m_fyCombo = new QComboBox(formCard);
    m_fyCombo->addItem("2627  (yyYY)", "yyYY");
    m_fyCombo->addItem("26-27  (yy-yy)", "yy-yy");
    m_fyCombo->addItem("2026-27  (YYYY-YY)", "YYYY-YY");
    m_fyCombo->addItem("None (no year in bill no.)", "none");
    grid->addWidget(m_fyCombo, 2, 1);

    grid->addWidget(new QLabel("Separator before number:", formCard), 3, 0);
    m_sepBEdit = new QLineEdit(formCard);
    m_sepBEdit->setMaxLength(3);
    m_sepBEdit->setPlaceholderText("-");
    grid->addWidget(m_sepBEdit, 3, 1);

    grid->addWidget(new QLabel("Number width (1 = plain -11, 4 = -0001):", formCard), 4, 0);
    m_widthSpin = new QSpinBox(formCard);
    m_widthSpin->setRange(1, 8);
    grid->addWidget(m_widthSpin, 4, 1);

    grid->addWidget(new QLabel("Start number:", formCard), 5, 0);
    m_startSpin = new QSpinBox(formCard);
    m_startSpin->setRange(0, 999999999);
    grid->addWidget(m_startSpin, 5, 1);

    grid->addWidget(new QLabel("Suffix (optional):", formCard), 6, 0);
    m_suffixEdit = new QLineEdit(formCard);
    m_suffixEdit->setMaxLength(8);
    grid->addWidget(m_suffixEdit, 6, 1);

    m_resetCheck = new QCheckBox("Restart numbering every financial year (GST-correct)", formCard);
    m_resetCheck->setChecked(true);
    grid->addWidget(m_resetCheck, 7, 0, 1, 2);

    mainLayout->addWidget(formCard);

    m_previewLabel = new QLabel("", this);
    m_previewLabel->setAlignment(Qt::AlignCenter);
    m_previewLabel->setStyleSheet("color: #0F172A; font-size: 14px; font-weight: 800; background-color: #EFF6FF; border: 1px solid #BFDBFE; border-radius: 6px; padding: 8px;");
    mainLayout->addWidget(m_previewLabel);

    m_statusLabel = new QLabel("", this);
    m_statusLabel->setAlignment(Qt::AlignCenter);
    m_statusLabel->setStyleSheet("color: #DC2626; font-size: 12px; font-weight: 700;");
    m_statusLabel->setWordWrap(true);
    mainLayout->addWidget(m_statusLabel);

    auto* btnRow = new QHBoxLayout();
    btnRow->setContentsMargins(20, 0, 20, 0);

    auto* btnCancel = new QPushButton("Close [Esc]", this);
    btnCancel->setStyleSheet("QPushButton { background-color: #F1F5F9; border: 1px solid #CBD5E1; color: #475569; padding: 7px 18px; border-radius: 6px; font-weight: 600; font-size: 13px; } QPushButton:hover { background-color: #E2E8F0; }");
    btnCancel->setCursor(Qt::PointingHandCursor);
    connect(btnCancel, &QPushButton::clicked, this, &QDialog::reject);
    btnRow->addWidget(btnCancel);

    btnRow->addStretch(1);

    m_saveBtn = new QPushButton("Save [Ctrl+S]", this);
    m_saveBtn->setStyleSheet("QPushButton { background-color: #16A34A; color: white; border: none; padding: 7px 20px; border-radius: 6px; font-weight: 700; font-size: 13px; } QPushButton:hover { background-color: #15803D; }");
    m_saveBtn->setCursor(Qt::PointingHandCursor);
    connect(m_saveBtn, &QPushButton::clicked, this, &BillSeriesDialog::onSaveClicked);
    btnRow->addWidget(m_saveBtn);

    mainLayout->addLayout(btnRow);

    // House keyboard: Enter walks the chain to Save; Esc closes; Ctrl+S saves.
    auto go = [this](QWidget* w) { if (w) { w->setFocus(); if (auto* le = qobject_cast<QLineEdit*>(w)) le->selectAll(); } };
    VoucherCommon::installGridNavigation(m_prefixEdit, nullptr, m_sepAEdit, [this, go]() { go(m_sepAEdit); });
    VoucherCommon::installGridNavigation(m_sepAEdit, m_prefixEdit, m_fyCombo, [this, go]() { go(m_fyCombo); });
    VoucherCommon::installGridNavigation(m_fyCombo, m_sepAEdit, m_sepBEdit, [this, go]() { go(m_sepBEdit); });
    VoucherCommon::installGridNavigation(m_sepBEdit, m_fyCombo, m_widthSpin, [this, go]() { go(m_widthSpin); });
    VoucherCommon::installGridNavigation(m_widthSpin, m_sepBEdit, m_startSpin, [this, go]() { go(m_startSpin); });
    VoucherCommon::installGridNavigation(m_startSpin, m_widthSpin, m_suffixEdit, [this, go]() { go(m_suffixEdit); });
    VoucherCommon::installGridNavigation(m_suffixEdit, m_startSpin, m_resetCheck, [this, go]() { go(m_resetCheck); });
    VoucherCommon::installGridNavigation(m_resetCheck, m_suffixEdit, m_saveBtn, [this]() { if (m_saveBtn) m_saveBtn->setFocus(); });

    auto* saveShortcut = new QShortcut(QKeySequence("Ctrl+S"), this);
    connect(saveShortcut, &QShortcut::activated, this, &BillSeriesDialog::onSaveClicked);

    connect(m_prefixEdit, &QLineEdit::textChanged, this, &BillSeriesDialog::updatePreview);
    connect(m_sepAEdit, &QLineEdit::textChanged, this, &BillSeriesDialog::updatePreview);
    connect(m_fyCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &BillSeriesDialog::updatePreview);
    connect(m_sepBEdit, &QLineEdit::textChanged, this, &BillSeriesDialog::updatePreview);
    connect(m_widthSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, &BillSeriesDialog::updatePreview);
    connect(m_startSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, &BillSeriesDialog::updatePreview);
    connect(m_suffixEdit, &QLineEdit::textChanged, this, &BillSeriesDialog::updatePreview);
    connect(m_resetCheck, &QCheckBox::toggled, this, &BillSeriesDialog::updatePreview);
}

void BillSeriesDialog::loadSettings()
{
    BillSeriesConfig cfg = BillSeriesManager::load("sale", "sales_invoices");
    m_prefixEdit->setText(cfg.prefix);
    m_sepAEdit->setText(cfg.sepAfterPrefix);
    int fyIdx = m_fyCombo->findData(cfg.fyStyle);
    m_fyCombo->setCurrentIndex(fyIdx >= 0 ? fyIdx : 0);
    m_sepBEdit->setText(cfg.sepBeforeSeq);
    m_widthSpin->setValue(qBound(1, cfg.seqWidth, 8));
    m_startSpin->setValue((int)qBound(0LL, cfg.startNumber, 999999999LL));
    m_suffixEdit->setText(cfg.suffix);
    m_resetCheck->setChecked(cfg.resetEachFy);
    updatePreview();
}

BillSeriesConfig BillSeriesDialog::collect() const
{
    BillSeriesConfig cfg;
    cfg.prefix = m_prefixEdit->text().trimmed();
    cfg.sepAfterPrefix = m_sepAEdit->text();
    cfg.fyStyle = m_fyCombo->currentData().toString();
    cfg.sepBeforeSeq = m_sepBEdit->text();
    cfg.seqWidth = m_widthSpin->value();
    cfg.startNumber = m_startSpin->value();
    cfg.resetEachFy = m_resetCheck->isChecked();
    cfg.suffix = m_suffixEdit->text().trimmed();
    return cfg;
}

void BillSeriesDialog::updatePreview()
{
    clearError();
    BillSeriesConfig cfg = collect();
    QString err = cfg.validate();
    if (!err.isEmpty()) {
        m_previewLabel->setText("—");
        showError(err);
        return;
    }
    FiscalYearInfo fy = FiscalYearHelper::getActiveFiscalYear();
    QString fyName = fy.isValid() ? fy.name : QString();
    // Preview the DRAFT config's true next number from the live DB
    // (same computation the voucher uses, with the on-screen values).
    QString next = BillSeriesManager::nextForConfig(cfg, "sales_invoices", fyName);
    m_previewLabel->setText(QString("Next sale bill will be:  %1").arg(next.isEmpty() ? "—" : next));
    if (!cfg.resetEachFy) {
        showError("Note: numbering will NOT restart each year. GST requires consecutive unique bills per FY.");
    } else if (cfg.fyStyle == "none") {
        showError("Note: no year in the bill number — the FY is taken from the bill date.");
    }
}

void BillSeriesDialog::showError(const QString& msg)
{
    m_statusLabel->setText(msg);
}

void BillSeriesDialog::clearError()
{
    m_statusLabel->clear();
}

void BillSeriesDialog::onSaveClicked()
{
    BillSeriesConfig cfg = collect();
    QString err = cfg.validate();
    if (!err.isEmpty()) {
        showError(err);
        return;
    }
    if (!BillSeriesManager::save("sale", cfg)) {
        showError("Could not save — please retry.");
        return;
    }
    accept();
}

} // namespace MahadevERP
