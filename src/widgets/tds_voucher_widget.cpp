#include "tds_voucher_widget.h"
#include "voucher_date_dialog.h"
#include "kbd_badge_button.h"
#include "custom_dialogs.h"
#include "../database_manager.h"
#include "../engine/fiscal_year_helper.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QScrollArea>
#include <QFrame>
#include <QDate>
#include <QShortcut>

namespace MahadevERP {

TdsVoucherWidget::TdsVoucherWidget(TdsVoucherController* controller,
                                   PrintExportController* printExportCtrl,
                                   QWidget* parent)
    : QWidget(parent)
    , m_controller(controller)
    , m_printExportCtrl(printExportCtrl)
{
    setupUi();
    resetForm();
}

void TdsVoucherWidget::setupUi() {
    auto* rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(14, 12, 14, 12);
    rootLayout->setSpacing(10);

    // ========================================================================
    // TIER 1: STICKY HEADER CARD (COMPACT HEIGHT)
    // ========================================================================
    auto* headerCard = new QFrame(this);
    headerCard->setFixedHeight(48);
    headerCard->setStyleSheet(
        "QFrame { background-color: #FFFFFF; border: 1px solid #E2E8F0; border-radius: 8px; }"
    );
    auto* headerLayout = new QHBoxLayout(headerCard);
    headerLayout->setContentsMargins(14, 4, 14, 4);
    headerLayout->setSpacing(10);

    auto* titleLabel = new QLabel("TDS Deduction & Statutory Tax Voucher (Section 194)", headerCard);
    titleLabel->setStyleSheet("font-size: 15px; font-weight: 800; color: #0F172A; border: none; background: transparent;");
    headerLayout->addWidget(titleLabel);

    auto* subLabel = new QLabel("• Contractors (194C), Professional Fees (194J), Rent (194I), Goods Purchase (194Q)", headerCard);
    subLabel->setStyleSheet("font-size: 11px; color: #64748B; border: none; background: transparent;");
    headerLayout->addWidget(subLabel);

    headerLayout->addStretch();

    auto* backBtn = new KbdBadgeButton("← Back to Dashboard", "Esc", headerCard);
    backBtn->setPrimaryColor("#F1F5F9", "#E2E8F0");
    backBtn->setTextColor("#475569");
    connect(backBtn, &QPushButton::clicked, this, &TdsVoucherWidget::backRequested);
    headerLayout->addWidget(backBtn);
    rootLayout->addWidget(headerCard);

    // ========================================================================
    // TIER 2: TOP METADATA & STATUTORY TAX MATRIX (2 SIDE-BY-SIDE COLUMNS)
    // ========================================================================
    auto* metaRowLayout = new QHBoxLayout();
    metaRowLayout->setSpacing(10);

    const QString cardStyle =
        "QFrame { background-color: #FFFFFF; border: 1px solid #CBD5E1; border-radius: 8px; }"
        "QLabel { color: #475569; font-size: 11.5px; font-weight: 700; border: none; background: transparent; }"
        "QLineEdit, QComboBox { background-color: #FFFFFF; border: 1.5px solid #CBD5E1; border-radius: 6px; padding: 4px 8px; font-size: 12px; color: #0F172A; font-weight: 600; }"
        "QLineEdit:focus, QComboBox:focus { border-color: #2563EB; background-color: #F8FAFC; }"
        "QCheckBox { color: #1E293B; font-weight: 700; font-size: 12px; border: none; background: transparent; }";

    // --- Left Card: Voucher & Party Identification ---
    auto* leftCard = new QFrame(this);
    leftCard->setStyleSheet(cardStyle);
    auto* leftLayout = new QVBoxLayout(leftCard);
    leftLayout->setContentsMargins(12, 10, 12, 10);
    leftLayout->setSpacing(8);

    auto* leftHeader = new QLabel("1. VOUCHER & DEDUCTEE IDENTIFICATION", leftCard);
    leftHeader->setStyleSheet("color: #2563EB; font-size: 10.5px; font-weight: 800; letter-spacing: 0.5px; border: none; background: transparent;");
    leftLayout->addWidget(leftHeader);

    auto* leftGrid = new QGridLayout();
    leftGrid->setHorizontalSpacing(10);
    leftGrid->setVerticalSpacing(6);
    leftGrid->setColumnStretch(1, 1);
    leftGrid->setColumnStretch(3, 1);

    // Row 0: Voucher No, Date
    leftGrid->addWidget(new QLabel("Voucher No:", leftCard), 0, 0);
    m_voucherNoEdit = new QLineEdit(leftCard);
    m_voucherNoEdit->setReadOnly(true);
    m_voucherNoEdit->setStyleSheet("background-color: #F8FAFC; border: 1.5px solid #CBD5E1; border-radius: 6px; padding: 4px 8px; font-weight: 800; color: #1D4ED8; font-family: 'Consolas', monospace;");
    leftGrid->addWidget(m_voucherNoEdit, 0, 1);

    leftGrid->addWidget(new QLabel("Date (F2) *:", leftCard), 0, 2);
    auto* dateBox = new QHBoxLayout();
    dateBox->setSpacing(4);
    m_voucherDateEdit = new QLineEdit(leftCard);
    m_voucherDateEdit->setReadOnly(true);
    m_voucherDateEdit->setMinimumWidth(85);
    dateBox->addWidget(m_voucherDateEdit);

    auto* dateBtn = new QPushButton("📅", leftCard);
    dateBtn->setFixedSize(26, 26);
    dateBtn->setCursor(Qt::PointingHandCursor);
    dateBtn->setStyleSheet("background-color: #EFF6FF; border: 1px solid #93C5FD; border-radius: 4px; font-weight: bold; color: #1D4ED8;");
    connect(dateBtn, &QPushButton::clicked, this, &TdsVoucherWidget::openDateDialog);
    dateBox->addWidget(dateBtn);

    m_dayLabel = new QLabel("", leftCard);
    m_dayLabel->setStyleSheet("color: #0284C7; font-weight: 800; font-size: 11px;");
    dateBox->addWidget(m_dayLabel);

    m_fyBadge = new QLabel(FiscalYearHelper::getActiveFiscalYear().name, leftCard);
    m_fyBadge->setStyleSheet("background-color: #F1F5F9; color: #475569; border: 1px solid #CBD5E1; border-radius: 4px; padding: 2px 6px; font-size: 10.5px; font-weight: 800;");
    dateBox->addWidget(m_fyBadge);
    dateBox->addStretch();
    leftGrid->addLayout(dateBox, 0, 3);

    // Row 1: TDS Section & Post in Books
    leftGrid->addWidget(new QLabel("TDS Section *:", leftCard), 1, 0);
    m_tdsTypeCombo = new QComboBox(leftCard);
    m_tdsTypeCombo->addItems({
        "194C - Payment to Contractors (1% Indiv / 2% Others)",
        "194J - Professional Fees / Technical Services (2% / 10%)",
        "194I - Rent on Land & Building (10%)",
        "194I - Rent on Plant & Machinery (2%)",
        "194Q - Purchase of Goods exceeding 50 Lakhs (0.1%)",
        "194H - Commission or Brokerage (5%)",
        "192 - Salaries",
        "194A - Interest other than on Securities (10%)"
    });
    connect(m_tdsTypeCombo, &QComboBox::currentIndexChanged, this, &TdsVoucherWidget::onTdsTypeChanged);
    leftGrid->addWidget(m_tdsTypeCombo, 1, 1);

    m_postBooksCheck = new QCheckBox("Post in Books", leftCard);
    m_postBooksCheck->setChecked(true);
    leftGrid->addWidget(m_postBooksCheck, 1, 3);

    // Row 2: Deductee / Party
    leftGrid->addWidget(new QLabel("Deductee A/c *:", leftCard), 2, 0);
    m_partySearch = new AccountSearchBox(leftCard);
    connect(m_partySearch, &AccountSearchBox::partySelected, this, &TdsVoucherWidget::onPartySelected);
    leftGrid->addWidget(m_partySearch, 2, 1, 1, 3);

    // Row 3: PAN & Balance
    leftGrid->addWidget(new QLabel("Deductee PAN *:", leftCard), 3, 0);
    m_panEdit = new QLineEdit(leftCard);
    m_panEdit->setPlaceholderText("AAAAA0000A");
    leftGrid->addWidget(m_panEdit, 3, 1);

    leftGrid->addWidget(new QLabel("Balance:", leftCard), 3, 2);
    m_partyBalanceLabel = new QLabel("₹ 0.00", leftCard);
    m_partyBalanceLabel->setStyleSheet("font-size: 12px; font-weight: 800; color: #0284C7;");
    leftGrid->addWidget(m_partyBalanceLabel, 3, 3);

    // Row 4: Expense Ledger & TDS Ledger
    leftGrid->addWidget(new QLabel("Debit Ledger:", leftCard), 4, 0);
    m_expLedgerEdit = new AccountSearchBox(leftCard);
    m_expLedgerEdit->setText("Freight & Transport Expense");
    leftGrid->addWidget(m_expLedgerEdit, 4, 1);

    leftGrid->addWidget(new QLabel("Credit Ledger:", leftCard), 4, 2);
    m_tdsLedgerEdit = new AccountSearchBox(leftCard);
    m_tdsLedgerEdit->setText("TDS Payable (Contractors - 194C)");
    leftGrid->addWidget(m_tdsLedgerEdit, 4, 3);

    leftLayout->addLayout(leftGrid);
    metaRowLayout->addWidget(leftCard, 1);

    // --- Right Card: Taxable Calculation & Rates ---
    auto* rightCard = new QFrame(this);
    rightCard->setStyleSheet(cardStyle);
    auto* rightLayout = new QVBoxLayout(rightCard);
    rightLayout->setContentsMargins(12, 10, 12, 10);
    rightLayout->setSpacing(8);

    auto* rightHeader = new QLabel("2. INCOME AMOUNT & STATUTORY TAX BREAKUP", rightCard);
    rightHeader->setStyleSheet("color: #2563EB; font-size: 10.5px; font-weight: 800; letter-spacing: 0.5px; border: none; background: transparent;");
    rightLayout->addWidget(rightHeader);

    auto* calcGrid = new QGridLayout();
    calcGrid->setHorizontalSpacing(10);
    calcGrid->setVerticalSpacing(6);
    calcGrid->setColumnStretch(0, 0);
    calcGrid->setColumnStretch(1, 1);
    calcGrid->setColumnStretch(2, 0);
    calcGrid->setColumnStretch(3, 1);

    // Row 0: Bill Amount, Previous YTD
    calcGrid->addWidget(new QLabel("Bill Amt (₹) *:", rightCard), 0, 0);
    m_incomeAmountEdit = new QLineEdit("0.00", rightCard);
    m_incomeAmountEdit->setAlignment(Qt::AlignRight);
    m_incomeAmountEdit->setMinimumWidth(90);
    connect(m_incomeAmountEdit, &QLineEdit::textChanged, this, &TdsVoucherWidget::onRecalculateTax);
    calcGrid->addWidget(m_incomeAmountEdit, 0, 1);

    calcGrid->addWidget(new QLabel("Prev YTD (₹):", rightCard), 0, 2);
    m_previousAmountEdit = new QLineEdit("0.00", rightCard);
    m_previousAmountEdit->setAlignment(Qt::AlignRight);
    m_previousAmountEdit->setMinimumWidth(80);
    connect(m_previousAmountEdit, &QLineEdit::textChanged, this, &TdsVoucherWidget::onRecalculateTax);
    calcGrid->addWidget(m_previousAmountEdit, 0, 3);

    // Row 1: Total for TDS & TDS Rate
    calcGrid->addWidget(new QLabel("TDS Base Amt:", rightCard), 1, 0);
    m_totalForTdsLabel = new QLabel("₹ 0.00", rightCard);
    m_totalForTdsLabel->setStyleSheet("font-size: 12px; font-weight: 800; color: #0F172A;");
    calcGrid->addWidget(m_totalForTdsLabel, 1, 1);

    calcGrid->addWidget(new QLabel("TDS Rate %:", rightCard), 1, 2);
    m_tdsRateEdit = new QLineEdit("2.00", rightCard);
    m_tdsRateEdit->setAlignment(Qt::AlignRight);
    connect(m_tdsRateEdit, &QLineEdit::textChanged, this, &TdsVoucherWidget::onRecalculateTax);
    calcGrid->addWidget(m_tdsRateEdit, 1, 3);

    // Row 2: TDS Amount & Surcharge Rate
    calcGrid->addWidget(new QLabel("TDS Tax Amt:", rightCard), 2, 0);
    m_tdsTaxAmountLabel = new QLabel("₹ 0.00", rightCard);
    m_tdsTaxAmountLabel->setStyleSheet("font-size: 12px; font-weight: 800; color: #DC2626;");
    calcGrid->addWidget(m_tdsTaxAmountLabel, 2, 1);

    calcGrid->addWidget(new QLabel("Surcharge %:", rightCard), 2, 2);
    m_surchargeRateEdit = new QLineEdit("0.00", rightCard);
    m_surchargeRateEdit->setAlignment(Qt::AlignRight);
    connect(m_surchargeRateEdit, &QLineEdit::textChanged, this, &TdsVoucherWidget::onRecalculateTax);
    calcGrid->addWidget(m_surchargeRateEdit, 2, 3);

    // Row 3: Surcharge Amt & Cess Rate
    calcGrid->addWidget(new QLabel("Surch Amt:", rightCard), 3, 0);
    m_surchargeTaxAmountLabel = new QLabel("₹ 0.00", rightCard);
    m_surchargeTaxAmountLabel->setStyleSheet("font-size: 12px; font-weight: 800; color: #D97706;");
    calcGrid->addWidget(m_surchargeTaxAmountLabel, 3, 1);

    calcGrid->addWidget(new QLabel("Cess %:", rightCard), 3, 2);
    m_cessRateEdit = new QLineEdit("0.00", rightCard);
    m_cessRateEdit->setAlignment(Qt::AlignRight);
    connect(m_cessRateEdit, &QLineEdit::textChanged, this, &TdsVoucherWidget::onRecalculateTax);
    calcGrid->addWidget(m_cessRateEdit, 3, 3);

    // Row 4: Cess Amt & Round Off
    calcGrid->addWidget(new QLabel("Cess Amt:", rightCard), 4, 0);
    m_cessTaxAmountLabel = new QLabel("₹ 0.00", rightCard);
    m_cessTaxAmountLabel->setStyleSheet("font-size: 12px; font-weight: 800; color: #D97706;");
    calcGrid->addWidget(m_cessTaxAmountLabel, 4, 1);

    m_roundOffCheck = new QCheckBox("Round Off", rightCard);
    m_roundOffCheck->setChecked(true);
    connect(m_roundOffCheck, &QCheckBox::toggled, this, &TdsVoucherWidget::onRecalculateTax);
    calcGrid->addWidget(m_roundOffCheck, 4, 2, 1, 2);

    rightLayout->addLayout(calcGrid);
    metaRowLayout->addWidget(rightCard, 1);

    rootLayout->addLayout(metaRowLayout);

    // ========================================================================
    // TIER 3: NARRATION & COMPLIANCE NOTES CARD
    // ========================================================================
    auto* noteCard = new QFrame(this);
    noteCard->setStyleSheet(cardStyle);
    auto* noteLayout = new QGridLayout(noteCard);
    noteLayout->setContentsMargins(12, 8, 12, 8);
    noteLayout->setHorizontalSpacing(10);
    noteLayout->setVerticalSpacing(6);

    noteLayout->addWidget(new QLabel("Narration:", noteCard), 0, 0);
    m_narrationEdit = new QLineEdit(noteCard);
    m_narrationEdit->setPlaceholderText("TDS deducted on bill payment / professional services");
    noteLayout->addWidget(m_narrationEdit, 0, 1);

    noteLayout->addWidget(new QLabel("Non-Deduction / 197 Reason:", noteCard), 1, 0);
    m_nonDeductReasonEdit = new QLineEdit(noteCard);
    m_nonDeductReasonEdit->setPlaceholderText("Leave empty unless 197 Lower Rate Certificate or Non-deduction declaration applied");
    noteLayout->addWidget(m_nonDeductReasonEdit, 1, 1);

    rootLayout->addWidget(noteCard);

    // ========================================================================
    // TIER 4: SUMMARY METRICS FOOTER & ACTION BUTTONS
    // ========================================================================
    auto* footerRow = new QHBoxLayout();
    footerRow->setSpacing(10);

    auto createMetric = [this](const QString& title, const QString& initVal, const QString& color) {
        auto* card = new QFrame(this);
        card->setStyleSheet(
            "QFrame { background-color: #FFFFFF; border: 1px solid #E2E8F0; border-radius: 8px; border-left: 4px solid " + color + "; }"
            "QLabel { border: none; background: transparent; }"
        );
        auto* cLayout = new QVBoxLayout(card);
        cLayout->setContentsMargins(10, 6, 10, 6);
        cLayout->setSpacing(2);

        auto* tLabel = new QLabel(title, card);
        tLabel->setStyleSheet("font-size: 9.5px; font-weight: 800; color: #64748B; letter-spacing: 0.5px; border: none; background: transparent;");
        auto* vLabel = new QLabel(initVal, card);
        vLabel->setStyleSheet("font-size: 14px; font-weight: 800; color: #0F172A; border: none; background: transparent;");
        cLayout->addWidget(tLabel);
        cLayout->addWidget(vLabel);
        return qMakePair(card, vLabel);
    };

    auto m1 = createMetric("TAXABLE / BILL AMOUNT", "₹ 0.00", "#2563EB");
    m_taxableSummaryLabel = m1.second;
    footerRow->addWidget(m1.first, 1);

    auto m2 = createMetric("TOTAL TDS DEDUCTED", "₹ 0.00", "#DC2626");
    m_tdsSummaryLabel = m2.second;
    footerRow->addWidget(m2.first, 1);

    auto m3 = createMetric("SURCHARGE & CESS", "₹ 0.00", "#D97706");
    m_surchCessSummaryLabel = m3.second;
    footerRow->addWidget(m3.first, 1);

    auto m4 = createMetric("NET AMOUNT PAYABLE", "₹ 0.00", "#16A34A");
    m_netPayableSummaryLabel = m4.second;
    footerRow->addWidget(m4.first, 1);

    m_deleteBtn = new KbdBadgeButton("Delete Voucher", "Del", this);
    m_deleteBtn->setPrimaryColor("#FEF2F2", "#FEE2E2");
    m_deleteBtn->setTextColor("#DC2626");
    m_deleteBtn->setVisible(false);
    connect(m_deleteBtn, &QPushButton::clicked, this, &TdsVoucherWidget::onDeleteClicked);
    footerRow->addWidget(m_deleteBtn, 0);

    m_saveBtn = new KbdBadgeButton("Save TDS Voucher", "Enter", this);
    m_saveBtn->setPrimaryColor("#16A34A", "#15803D");
    m_saveBtn->setTextColor("#FFFFFF");
    m_saveBtn->setMinimumWidth(180);
    connect(m_saveBtn, &QPushButton::clicked, this, &TdsVoucherWidget::onSaveClicked);
    footerRow->addWidget(m_saveBtn, 0);

    rootLayout->addLayout(footerRow);

    // Keyboard-First Traversal Chain on Enter
    if (m_partySearch) {
        connect(m_partySearch, &AccountSearchBox::returnPressed, this, [this]() {
            if (m_panEdit) { m_panEdit->setFocus(); m_panEdit->selectAll(); }
        });
    }
    if (m_panEdit) {
        connect(m_panEdit, &QLineEdit::returnPressed, this, [this]() {
            if (m_expLedgerEdit) { m_expLedgerEdit->setFocus(); m_expLedgerEdit->selectAll(); }
        });
    }
    if (m_expLedgerEdit) {
        connect(m_expLedgerEdit, &AccountSearchBox::returnPressed, this, [this]() {
            if (m_tdsLedgerEdit) { m_tdsLedgerEdit->setFocus(); m_tdsLedgerEdit->selectAll(); }
        });
    }
    if (m_tdsLedgerEdit) {
        connect(m_tdsLedgerEdit, &AccountSearchBox::returnPressed, this, [this]() {
            if (m_incomeAmountEdit) { m_incomeAmountEdit->setFocus(); m_incomeAmountEdit->selectAll(); }
        });
    }
    if (m_incomeAmountEdit) {
        connect(m_incomeAmountEdit, &QLineEdit::returnPressed, this, [this]() {
            if (m_previousAmountEdit) { m_previousAmountEdit->setFocus(); m_previousAmountEdit->selectAll(); }
        });
    }
    if (m_previousAmountEdit) {
        connect(m_previousAmountEdit, &QLineEdit::returnPressed, this, [this]() {
            if (m_tdsRateEdit) { m_tdsRateEdit->setFocus(); m_tdsRateEdit->selectAll(); }
        });
    }
    if (m_tdsRateEdit) {
        connect(m_tdsRateEdit, &QLineEdit::returnPressed, this, [this]() {
            if (m_surchargeRateEdit) { m_surchargeRateEdit->setFocus(); m_surchargeRateEdit->selectAll(); }
        });
    }
    if (m_surchargeRateEdit) {
        connect(m_surchargeRateEdit, &QLineEdit::returnPressed, this, [this]() {
            if (m_cessRateEdit) { m_cessRateEdit->setFocus(); m_cessRateEdit->selectAll(); }
        });
    }
    if (m_cessRateEdit) {
        connect(m_cessRateEdit, &QLineEdit::returnPressed, this, [this]() {
            if (m_narrationEdit) { m_narrationEdit->setFocus(); m_narrationEdit->selectAll(); }
        });
    }
    if (m_narrationEdit) {
        connect(m_narrationEdit, &QLineEdit::returnPressed, this, [this]() {
            if (m_saveBtn) { m_saveBtn->setFocus(); }
        });
    }

    connect(new QShortcut(QKeySequence(Qt::Key_Escape), this), &QShortcut::activated, this, &TdsVoucherWidget::backRequested);
}

void TdsVoucherWidget::resetForm() {
    m_editVoucherId = 0;
    m_deleteBtn->setVisible(false);
    QDate cur = QDate::currentDate();
    m_voucherDateEdit->setText(cur.toString("dd-MM-yyyy"));
    m_dayLabel->setText(cur.toString("ddd"));
    if (m_fyBadge) {
        m_fyBadge->setText(FiscalYearHelper::getActiveFiscalYear().name);
    }

    if (m_controller) {
        m_controller->resetForm();
        m_voucherNoEdit->setText(QString::number(m_controller->voucherNo()));
    } else {
        m_voucherNoEdit->setText("1");
    }

    m_tdsTypeCombo->setCurrentIndex(0);
    m_partySearch->clear();
    m_panEdit->clear();
    m_partyBalanceLabel->setText("Current Balance: ₹ 0.00");
    m_incomeAmountEdit->setText("0.00");
    m_previousAmountEdit->setText("0.00");
    m_tdsRateEdit->setText("2.00");
    m_surchargeRateEdit->setText("0.00");
    m_cessRateEdit->setText("0.00");
    m_narrationEdit->clear();
    m_nonDeductReasonEdit->clear();

    onRecalculateTax();
}

void TdsVoucherWidget::openDateDialog() {
    QDate curDate = QDate::fromString(m_voucherDateEdit->text(), "dd-MM-yyyy");
    if (!curDate.isValid()) curDate = QDate::currentDate();

    QDate chosen = VoucherDateDialog::selectDate(this, curDate);
    if (chosen.isValid()) {
        m_voucherDateEdit->setText(chosen.toString("dd-MM-yyyy"));
        m_dayLabel->setText(chosen.toString("dddd"));
        if (m_controller) {
            m_controller->setVoucherDate(chosen.toString("yyyy-MM-dd"));
        }
    }
}

void TdsVoucherWidget::onTdsTypeChanged(int index) {
    auto findParty = [](const QStringList& patterns, const QString& fallback) -> QString {
        for (const QString& pat : patterns) {
            QVariant v = DatabaseManager::instance().executeScalar(
                "SELECT name FROM parties WHERE name LIKE ? ORDER BY id ASC LIMIT 1;",
                {"%" + pat + "%"}
            );
            if (v.isValid() && !v.isNull()) return v.toString().trimmed();
        }
        return fallback;
    };

    if (index == 0) { // 194C
        m_tdsRateEdit->setText("2.00");
        m_expLedgerEdit->setText(findParty({"Freight", "Transport"}, "Freight & Transport Expense"));
        m_tdsLedgerEdit->setText(findParty({"TDS", "194C"}, "TDS Payable (Contractors - 194C)"));
    } else if (index == 1) { // 194J
        m_tdsRateEdit->setText("10.00");
        m_expLedgerEdit->setText(findParty({"Professional", "Legal", "Audit"}, "Legal & Professional Charges"));
        m_tdsLedgerEdit->setText(findParty({"194J", "TDS"}, "TDS Payable (Professional - 194J)"));
    } else if (index == 2) { // 194I Land
        m_tdsRateEdit->setText("10.00");
        m_expLedgerEdit->setText(findParty({"Rent"}, "Rent Paid"));
        m_tdsLedgerEdit->setText(findParty({"194I", "TDS"}, "TDS Payable (Rent - 194I)"));
    } else if (index == 3) { // 194I Plant
        m_tdsRateEdit->setText("2.00");
        m_expLedgerEdit->setText(findParty({"Machinery", "Hire", "Plant"}, "Machinery Hire Charges"));
        m_tdsLedgerEdit->setText(findParty({"194I", "TDS"}, "TDS Payable (Rent - 194I)"));
    } else if (index == 4) { // 194Q
        m_tdsRateEdit->setText("0.10");
        m_expLedgerEdit->setText(findParty({"Purchase"}, "Purchase A/c"));
        m_tdsLedgerEdit->setText(findParty({"194Q", "TDS"}, "TDS Payable (Purchase - 194Q)"));
    } else if (index == 5) { // 194H
        m_tdsRateEdit->setText("5.00");
        m_expLedgerEdit->setText(findParty({"Brokerage", "Commission", "Dami"}, "Brokerage & Commission"));
        m_tdsLedgerEdit->setText(findParty({"194H", "TDS"}, "TDS Payable (Commission - 194H)"));
    }
    onRecalculateTax();
}

void TdsVoucherWidget::onPartySelected(const QString& partyName) {
    if (partyName.trimmed().isEmpty()) return;
    if (m_controller) {
        m_controller->setSelectedPartyName(partyName);
        m_panEdit->setText(m_controller->partyPan());
        m_partyBalanceLabel->setText("Current Balance: " + m_controller->partyBalanceText());
    }
}

void TdsVoucherWidget::onRecalculateTax() {
    double income = m_incomeAmountEdit->text().toDouble();
    double prev = m_previousAmountEdit->text().toDouble();
    double totForTds = income + prev;
    m_totalForTdsLabel->setText("₹ " + QString::number(totForTds, 'f', 2));

    double baseRate = m_tdsRateEdit->text().toDouble();
    double surRate = m_surchargeRateEdit->text().toDouble();
    double cessRate = m_cessRateEdit->text().toDouble();

    double baseTax = (income * baseRate) / 100.0;
    double surTax = (baseTax * surRate) / 100.0;
    double cessTax = ((baseTax + surTax) * cessRate) / 100.0;
    double totTax = baseTax + surTax + cessTax;

    if (m_roundOffCheck->isChecked()) {
        totTax = std::round(totTax);
    }
    double netPay = std::max(0.0, income - totTax);

    m_tdsTaxAmountLabel->setText("₹ " + QString::number(baseTax, 'f', 2));
    m_surchargeTaxAmountLabel->setText("₹ " + QString::number(surTax, 'f', 2));
    m_cessTaxAmountLabel->setText("₹ " + QString::number(cessTax, 'f', 2));

    if (m_taxableSummaryLabel) m_taxableSummaryLabel->setText("₹ " + QString::number(income, 'f', 2));
    if (m_tdsSummaryLabel) m_tdsSummaryLabel->setText("₹ " + QString::number(baseTax, 'f', 2));
    if (m_surchCessSummaryLabel) m_surchCessSummaryLabel->setText("₹ " + QString::number(surTax + cessTax, 'f', 2));
    if (m_netPayableSummaryLabel) m_netPayableSummaryLabel->setText("₹ " + QString::number(netPay, 'f', 2));
    if (m_totalTaxLabel) m_totalTaxLabel->setText("TOTAL TDS DEDUCTED: ₹ " + QString::number(totTax, 'f', 2));
    if (m_netPayableLabel) m_netPayableLabel->setText("NET AMOUNT PAYABLE: ₹ " + QString::number(netPay, 'f', 2));
}

bool TdsVoucherWidget::loadVoucherForEditing(int voucherId) {
    if (!m_controller) return false;
    bool ok = m_controller->loadVoucher(voucherId);
    if (!ok) return false;

    m_editVoucherId = voucherId;
    m_deleteBtn->setVisible(true);

    m_voucherNoEdit->setText(QString::number(m_controller->voucherNo()));
    QDate d = QDate::fromString(m_controller->voucherDate(), "yyyy-MM-dd");
    if (d.isValid()) {
        m_voucherDateEdit->setText(d.toString("dd-MM-yyyy"));
        m_dayLabel->setText(d.toString("dddd"));
    }

    m_partySearch->setText(m_controller->selectedPartyName());
    m_panEdit->setText(m_controller->partyPan());
    m_partyBalanceLabel->setText("Current Balance: " + m_controller->partyBalanceText());
    m_incomeAmountEdit->setText(QString::number(m_controller->incomeAmount(), 'f', 2));
    m_previousAmountEdit->setText(QString::number(m_controller->previousAmount(), 'f', 2));
    m_tdsRateEdit->setText(QString::number(m_controller->tdsRate(), 'f', 2));
    m_surchargeRateEdit->setText(QString::number(m_controller->surchargeRate(), 'f', 2));
    m_cessRateEdit->setText(QString::number(m_controller->cessRate(), 'f', 2));
    m_expLedgerEdit->setText(m_controller->expLedgerName());
    m_tdsLedgerEdit->setText(m_controller->tdsLedgerName());
    m_narrationEdit->setText(m_controller->narration());
    m_nonDeductReasonEdit->setText(m_controller->nonDeductionReason());

    onRecalculateTax();
    return true;
}

void TdsVoucherWidget::onSaveClicked() {
    if (m_partySearch->text().trimmed().isEmpty()) {
        CustomMessageBox::showWarning(this, "Validation Error", "Please select a Deductee / Party Account.");
        m_partySearch->setFocus();
        return;
    }

    double income = m_incomeAmountEdit->text().toDouble();
    if (income <= 0.0) {
        CustomMessageBox::showWarning(this, "Validation Error", "Please enter a valid Bill / Income Amount.");
        m_incomeAmountEdit->setFocus();
        return;
    }

    if (!CustomMessageBox::showConfirmation(this, "Confirm Save", "Are you sure you want to save TDS Voucher No. " + m_voucherNoEdit->text() + "?")) {
        return;
    }

    bool ok = false;
    if (m_controller) {
        m_controller->setTdsType(m_tdsTypeCombo->currentText());
        m_controller->setSelectedPartyName(m_partySearch->text().trimmed());
        m_controller->setPartyPan(m_panEdit->text().trimmed());
        m_controller->setIncomeAmount(income);
        m_controller->setPreviousAmount(m_previousAmountEdit->text().toDouble());
        m_controller->setTdsRate(m_tdsRateEdit->text().toDouble());
        m_controller->setSurchargeRate(m_surchargeRateEdit->text().toDouble());
        m_controller->setCessRate(m_cessRateEdit->text().toDouble());
        m_controller->setUseRoundedTotal(m_roundOffCheck->isChecked());
        m_controller->setExpLedgerName(m_expLedgerEdit->text().trimmed());
        m_controller->setTdsLedgerName(m_tdsLedgerEdit->text().trimmed());
        m_controller->setNarration(m_narrationEdit->text().trimmed());
        m_controller->setNonDeductionReason(m_nonDeductReasonEdit->text().trimmed());
        m_controller->setPostInBooks(m_postBooksCheck->isChecked());
        ok = m_controller->saveVoucher();
    } else {
        ok = true;
    }

    if (ok) {
        CustomMessageBox::showInformation(this, "Success", "TDS Voucher saved successfully.");
        int vNo = m_voucherNoEdit->text().toInt();
        resetForm();
        emit voucherSaved(vNo);
    } else {
        CustomMessageBox::showCritical(this, "Save Failed", "Failed to save TDS Voucher.");
    }
}

void TdsVoucherWidget::onDeleteClicked() {
    if (m_editVoucherId <= 0) return;

    if (!CustomMessageBox::showConfirmation(this, "Confirm Delete", "Are you sure you want to delete this TDS Voucher? This cannot be undone.")) {
        return;
    }

    DatabaseManager::instance().executeQuery(QString("DELETE FROM tds_vouchers WHERE id = %1;").arg(m_editVoucherId));
    CustomMessageBox::showInformation(this, "Deleted", "TDS Voucher removed successfully.");
    resetForm();
    emit voucherSaved(0);
}

} // namespace MahadevERP
