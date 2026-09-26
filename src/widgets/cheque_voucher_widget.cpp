#include "cheque_voucher_widget.h"
#include "voucher_date_dialog.h"
#include "custom_dialogs.h"
#include "../engine/fiscal_year_helper.h"
#include "../services/accounting_date_service.h"
#include "../services/financial_math_service.h"
#include "../database_manager.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QHeaderView>
#include <QMessageBox>
#include <QKeyEvent>
#include <QDate>
#include <QApplication>
#include <QTimer>
#include <cmath>

ChequeVoucherWidget::ChequeVoucherWidget(PrintExportController* printExportCtrl, QWidget* parent)
    : QWidget(parent)
    , m_printExportCtrl(printExportCtrl)
{
    setupUi();
    applyCustomStyles();
    resetForm();
}

void ChequeVoucherWidget::setupUi() {
    setAttribute(Qt::WA_StyledBackground, true);
    setStyleSheet("ChequeVoucherWidget { background-color: #F8FAFC; }");

    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(16, 12, 16, 12);
    mainLayout->setSpacing(10);

    // ========================================================================
    // 1. TOP HEADER CARD
    // ========================================================================
    auto* headerCard = new QFrame(this);
    headerCard->setFixedHeight(58);
    headerCard->setStyleSheet("background-color: #FFFFFF; border: 1px solid #E2E8F0; border-radius: 8px;");
    auto* headerLayout = new QHBoxLayout(headerCard);
    headerLayout->setContentsMargins(16, 0, 16, 0);
    headerLayout->setSpacing(12);

    auto* titleBox = new QVBoxLayout();
    titleBox->setSpacing(1);
    m_modeBadge = new QLabel("CHEQUE PAYMENT VOUCHER", headerCard);
    m_modeBadge->setStyleSheet("color: #0F172A; font-size: 15px; font-weight: 800; border: none; background: transparent;");
    titleBox->addWidget(m_modeBadge);

    auto* subTitleLabel = new QLabel("Double-entry cheque voucher • Real-time balance verification (Dr = Cr)", headerCard);
    subTitleLabel->setStyleSheet("color: #64748B; font-size: 11px; border: none; background: transparent;");
    titleBox->addWidget(subTitleLabel);
    headerLayout->addLayout(titleBox);

    headerLayout->addStretch(1);

    m_paymentModeBtn = new QPushButton("F3: Payment", headerCard);
    m_paymentModeBtn->setFixedHeight(32);
    m_paymentModeBtn->setCursor(Qt::PointingHandCursor);
    connect(m_paymentModeBtn, &QPushButton::clicked, this, [this]() { setVoucherMode("Payment"); });
    headerLayout->addWidget(m_paymentModeBtn);

    m_receiptModeBtn = new QPushButton("F4: Receipt", headerCard);
    m_receiptModeBtn->setFixedHeight(32);
    m_receiptModeBtn->setCursor(Qt::PointingHandCursor);
    connect(m_receiptModeBtn, &QPushButton::clicked, this, [this]() { setVoucherMode("Receipt"); });
    headerLayout->addWidget(m_receiptModeBtn);

    m_alterBadge = new QLabel("", headerCard);
    m_alterBadge->setStyleSheet(
        "background-color: #FEE2E2; color: #DC2626; border: 1px solid #FCA5A5; "
        "padding: 4px 10px; border-radius: 6px; font-weight: 800; font-size: 11px;"
    );
    m_alterBadge->hide();
    headerLayout->addWidget(m_alterBadge);

    m_backBtn = new QPushButton("← Back to Dashboard (Esc)", headerCard);
    m_backBtn->setFixedHeight(32);
    m_backBtn->setCursor(Qt::PointingHandCursor);
    m_backBtn->setStyleSheet(
        "QPushButton { background-color: #F1F5F9; border: 1px solid #CBD5E1; border-radius: 6px; padding: 0px 14px; font-weight: 700; color: #475569; font-size: 12px; }"
        "QPushButton:hover { background-color: #E2E8F0; }"
    );
    connect(m_backBtn, &QPushButton::clicked, this, &ChequeVoucherWidget::backRequested);
    headerLayout->addWidget(m_backBtn);

    mainLayout->addWidget(headerCard);

    // ========================================================================
    // 2. VOUCHER META / CONTROLS CARD
    // ========================================================================
    auto* metaCard = new QFrame(this);
    metaCard->setFixedHeight(52);
    metaCard->setStyleSheet("background-color: #FFFFFF; border: 1px solid #E2E8F0; border-radius: 8px;");
    auto* metaLayout = new QHBoxLayout(metaCard);
    metaLayout->setContentsMargins(16, 0, 16, 0);
    metaLayout->setSpacing(14);

    auto* typeTagLabel = new QLabel("Type:", metaCard);
    typeTagLabel->setStyleSheet("color: #475569; font-size: 11px; font-weight: 700; border: none; background: transparent;");
    metaLayout->addWidget(typeTagLabel);

    auto* voucherTypePill = new QLabel("Cheque Payment", metaCard);
    voucherTypePill->setObjectName("voucherTypePill");
    voucherTypePill->setStyleSheet(
        "background-color: #FEF2F2; color: #DC2626; border: 1px solid #FCA5A5; "
        "padding: 4px 10px; border-radius: 6px; font-weight: 800; font-size: 11px;"
    );
    metaLayout->addWidget(voucherTypePill);

    m_voucherNoLabel = new QLabel("Voucher No:", metaCard);
    m_voucherNoLabel->setStyleSheet("color: #475569; font-size: 11px; font-weight: 700; border: none; background: transparent;");
    metaLayout->addWidget(m_voucherNoLabel);

    m_voucherNoInput = new QLineEdit(metaCard);
    m_voucherNoInput->setFixedSize(100, 30);
    m_voucherNoInput->setAlignment(Qt::AlignCenter);
    m_voucherNoInput->setStyleSheet(
        "QLineEdit { background-color: #F1F5F9; color: #2563EB; border: 1px solid #CBD5E1; "
        "border-radius: 6px; font-weight: 800; font-size: 12px; }"
    );
    m_voucherNoInput->installEventFilter(this);
    metaLayout->addWidget(m_voucherNoInput);

    auto* dateLabel = new QLabel("Date (F2):", metaCard);
    dateLabel->setStyleSheet("color: #475569; font-size: 11px; font-weight: 700; border: none; background: transparent;");
    metaLayout->addWidget(dateLabel);

    m_dateEdit = new AccountingDateEdit(metaCard);
    m_dateEdit->setFixedSize(110, 30);
    m_dateEdit->setStyleSheet(
        "QLineEdit { background-color: #FFFFFF; color: #0F172A; border: 1px solid #CBD5E1; "
        "border-radius: 6px; padding: 2px 6px; font-weight: 700; font-size: 12px; }"
        "QLineEdit:focus { border: 1.5px solid #2563EB; background-color: #EFF6FF; }"
    );
    m_dateEdit->installEventFilter(this);
    connect(m_dateEdit, &AccountingDateEdit::dateChanged, this, &ChequeVoucherWidget::onDateChanged);
    metaLayout->addWidget(m_dateEdit);

    m_dayOfWeekLabel = new QLabel("", metaCard);
    m_dayOfWeekLabel->setStyleSheet("color: #0284C7; font-weight: 700; font-size: 11.5px; border: none; background: transparent;");
    metaLayout->addWidget(m_dayOfWeekLabel);

    m_fyBadge = new QLabel(FiscalYearHelper::getActiveFiscalYear().name, metaCard);
    m_fyBadge->setStyleSheet(
        "background-color: #F1F5F9; color: #475569; border: 1px solid #CBD5E1; "
        "padding: 4px 8px; border-radius: 6px; font-weight: 700; font-size: 11px;"
    );
    metaLayout->addWidget(m_fyBadge);

    metaLayout->addStretch(1);
    mainLayout->addWidget(metaCard);

    // ========================================================================
    // 3. MULTI-ROW TRANSACTION GRID TABLE
    // ========================================================================
    m_table = new QTableWidget(0, 6, this);
    m_table->setObjectName("voucherTable");
    m_table->setHorizontalHeaderLabels({"TYPE", "PARTICULARS (ACCOUNT / LEDGER)", "DEBIT (₹)", "CREDIT (₹)", "CHEQUE / REF NO", ""});
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table->setShowGrid(true);
    m_table->verticalHeader()->setVisible(false);
    m_table->horizontalHeader()->setStretchLastSection(false);
    m_table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Fixed);
    m_table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    m_table->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Fixed);
    m_table->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Fixed);
    m_table->horizontalHeader()->setSectionResizeMode(4, QHeaderView::Fixed);
    m_table->horizontalHeader()->setSectionResizeMode(5, QHeaderView::Fixed);

    m_table->setColumnWidth(0, 80);
    m_table->setColumnWidth(2, 140);
    m_table->setColumnWidth(3, 140);
    m_table->setColumnWidth(4, 160);
    m_table->setColumnWidth(5, 40);

    m_table->setStyleSheet(
        "QTableWidget { background-color: #FFFFFF; border: 1px solid #E2E8F0; border-radius: 8px; gridline-color: #F1F5F9; }"
        "QHeaderView::section { background-color: #F1F5F9; color: #334155; font-weight: 800; font-size: 11px; padding: 6px; border: none; border-bottom: 2px solid #CBD5E1; }"
        "QTableWidget::item { padding: 2px; }"
    );

    mainLayout->addWidget(m_table, 1);

    // ========================================================================
    // 4. BOTTOM NARRATION & SUMMARY CARD
    // ========================================================================
    auto* bottomCard = new QFrame(this);
    bottomCard->setStyleSheet("background-color: #FFFFFF; border: 1px solid #E2E8F0; border-radius: 8px;");
    auto* bottomLayout = new QVBoxLayout(bottomCard);
    bottomLayout->setContentsMargins(14, 10, 14, 10);
    bottomLayout->setSpacing(8);

    auto* narrRow = new QHBoxLayout();
    narrRow->setSpacing(10);
    auto* narrLabel = new QLabel("Narration:", bottomCard);
    narrLabel->setStyleSheet("color: #334155; font-weight: 700; font-size: 12px; border: none; background: transparent;");
    narrRow->addWidget(narrLabel);

    m_narrationInput = new QLineEdit(bottomCard);
    m_narrationInput->setPlaceholderText("Enter voucher narration / remarks...");
    m_narrationInput->setFixedHeight(32);
    m_narrationInput->setStyleSheet(
        "QLineEdit { background-color: #FFFFFF; color: #0F172A; border: 1px solid #CBD5E1; "
        "border-radius: 6px; padding: 4px 10px; font-size: 12px; }"
        "QLineEdit:focus { border: 1.5px solid #2563EB; background-color: #EFF6FF; }"
    );
    m_narrationInput->installEventFilter(this);
    narrRow->addWidget(m_narrationInput, 1);
    bottomLayout->addLayout(narrRow);

    auto* totalsRow = new QHBoxLayout();
    totalsRow->setSpacing(14);

    m_statusLabel = new QLabel("", bottomCard);
    m_statusLabel->setStyleSheet("font-size: 12px; font-weight: 700; border: none; background: transparent;");
    totalsRow->addWidget(m_statusLabel, 1);

    auto* debitBox = new QHBoxLayout();
    auto* debitTitle = new QLabel("Total Debit:", bottomCard);
    debitTitle->setStyleSheet("color: #64748B; font-size: 12px; font-weight: 700; border: none; background: transparent;");
    m_totalDebitLabel = new QLabel("₹0.00", bottomCard);
    m_totalDebitLabel->setStyleSheet("color: #0F172A; font-size: 14px; font-weight: 800; border: none; background: transparent;");
    debitBox->addWidget(debitTitle);
    debitBox->addWidget(m_totalDebitLabel);
    totalsRow->addLayout(debitBox);

    auto* creditBox = new QHBoxLayout();
    auto* creditTitle = new QLabel("Total Credit:", bottomCard);
    creditTitle->setStyleSheet("color: #64748B; font-size: 12px; font-weight: 700; border: none; background: transparent;");
    m_totalCreditLabel = new QLabel("₹0.00", bottomCard);
    m_totalCreditLabel->setStyleSheet("color: #0F172A; font-size: 14px; font-weight: 800; border: none; background: transparent;");
    creditBox->addWidget(creditTitle);
    creditBox->addWidget(m_totalCreditLabel);
    totalsRow->addLayout(creditBox);

    m_balanceBadge = new QLabel("Balanced (₹0.00)", bottomCard);
    m_balanceBadge->setStyleSheet(
        "background-color: #DCFCE7; color: #166534; border: 1px solid #86EFAC; "
        "font-weight: 800; font-size: 12px; padding: 4px 12px; border-radius: 6px;"
    );
    totalsRow->addWidget(m_balanceBadge);

    bottomLayout->addLayout(totalsRow);
    mainLayout->addWidget(bottomCard);

    // ========================================================================
    // 5. ACTION BUTTONS BAR
    // ========================================================================
    auto* actionsRow = new QHBoxLayout();
    actionsRow->setSpacing(8);

    m_dateBtn = new QPushButton("F2: Date", this);
    m_dateBtn->setFixedHeight(34);
    m_dateBtn->setCursor(Qt::PointingHandCursor);
    m_dateBtn->setStyleSheet(
        "QPushButton { background-color: #F1F5F9; color: #334155; border: 1px solid #CBD5E1; "
        "padding: 0px 14px; border-radius: 6px; font-weight: 700; font-size: 12px; } "
        "QPushButton:hover { background-color: #E2E8F0; }"
    );
    connect(m_dateBtn, &QPushButton::clicked, this, &ChequeVoucherWidget::openDateDialog);
    actionsRow->addWidget(m_dateBtn);

    m_addRowBtn = new QPushButton("+ Add Row (Insert)", this);
    m_addRowBtn->setFixedHeight(34);
    m_addRowBtn->setCursor(Qt::PointingHandCursor);
    m_addRowBtn->setStyleSheet(
        "QPushButton { background-color: #EFF6FF; color: #2563EB; border: 1px solid #BFDBFE; "
        "padding: 0px 14px; border-radius: 6px; font-weight: 700; font-size: 12px; } "
        "QPushButton:hover { background-color: #DBEAFE; }"
    );
    connect(m_addRowBtn, &QPushButton::clicked, this, [this]() { addNewRow(); });
    actionsRow->addWidget(m_addRowBtn);

    actionsRow->addStretch();

    m_deleteBtn = new QPushButton("Delete Voucher (Ctrl+D)", this);
    m_deleteBtn->setFixedHeight(34);
    m_deleteBtn->setCursor(Qt::PointingHandCursor);
    m_deleteBtn->setStyleSheet(
        "QPushButton { background-color: #FEE2E2; color: #DC2626; border: 1px solid #FCA5A5; "
        "padding: 0px 14px; border-radius: 6px; font-weight: 700; font-size: 12px; } "
        "QPushButton:hover { background-color: #FECACA; }"
    );
    m_deleteBtn->hide();
    connect(m_deleteBtn, &QPushButton::clicked, this, &ChequeVoucherWidget::deleteVoucher);
    actionsRow->addWidget(m_deleteBtn);

    m_backBtn = new QPushButton("Esc: Back", this);
    m_backBtn->setFixedHeight(34);
    m_backBtn->setCursor(Qt::PointingHandCursor);
    m_backBtn->setStyleSheet(
        "QPushButton { background-color: #F1F5F9; color: #475569; border: 1px solid #CBD5E1; "
        "padding: 0px 14px; border-radius: 6px; font-weight: 700; font-size: 12px; } "
        "QPushButton:hover { background-color: #E2E8F0; }"
    );
    connect(m_backBtn, &QPushButton::clicked, this, &ChequeVoucherWidget::backRequested);
    actionsRow->addWidget(m_backBtn);

    m_saveBtn = new QPushButton("Ctrl+S: Save Voucher", this);
    m_saveBtn->setFixedHeight(34);
    m_saveBtn->setCursor(Qt::PointingHandCursor);
    m_saveBtn->setStyleSheet(
        "QPushButton { background-color: #2563EB; color: #FFFFFF; border: 1px solid #1D4ED8; "
        "padding: 0px 20px; border-radius: 6px; font-weight: 800; font-size: 13px; } "
        "QPushButton:hover { background-color: #1D4ED8; }"
    );
    connect(m_saveBtn, &QPushButton::clicked, this, &ChequeVoucherWidget::saveVoucher);
    actionsRow->addWidget(m_saveBtn);

    mainLayout->addLayout(actionsRow);
}

void ChequeVoucherWidget::applyCustomStyles() {
    setVoucherMode(m_voucherMode);
}

void ChequeVoucherWidget::setVoucherMode(const QString& mode) {
    m_voucherMode = mode.contains("Receipt", Qt::CaseInsensitive) ? "Receipt" : "Payment";

    auto* pill = findChild<QLabel*>("voucherTypePill");
    if (m_voucherMode == "Payment") {
        m_modeBadge->setText("CHEQUE PAYMENT VOUCHER");
        if (pill) {
            pill->setText("Cheque Payment");
            pill->setStyleSheet("background-color: #FEF2F2; color: #DC2626; border: 1px solid #FCA5A5; padding: 4px 10px; border-radius: 6px; font-weight: 800; font-size: 11px;");
        }
        m_paymentModeBtn->setStyleSheet("QPushButton { background-color: #DC2626; color: #FFFFFF; border: 1px solid #B91C1C; padding: 0px 12px; border-radius: 6px; font-weight: 700; font-size: 12px; }");
        m_receiptModeBtn->setStyleSheet("QPushButton { background-color: #F1F5F9; color: #475569; border: 1px solid #CBD5E1; padding: 0px 12px; border-radius: 6px; font-weight: 700; font-size: 12px; } QPushButton:hover { background-color: #E2E8F0; }");
    } else {
        m_modeBadge->setText("CHEQUE RECEIPT VOUCHER");
        if (pill) {
            pill->setText("Cheque Receipt");
            pill->setStyleSheet("background-color: #F0FDF4; color: #16A34A; border: 1px solid #86EFAC; padding: 4px 10px; border-radius: 6px; font-weight: 800; font-size: 11px;");
        }
        m_receiptModeBtn->setStyleSheet("QPushButton { background-color: #16A34A; color: #FFFFFF; border: 1px solid #15803D; padding: 0px 12px; border-radius: 6px; font-weight: 700; font-size: 12px; }");
        m_paymentModeBtn->setStyleSheet("QPushButton { background-color: #F1F5F9; color: #475569; border: 1px solid #CBD5E1; padding: 0px 12px; border-radius: 6px; font-weight: 700; font-size: 12px; } QPushButton:hover { background-color: #E2E8F0; }");
    }

    if (!isEditMode()) {
        updateFiscalYearAndVoucherNo();
    }
}

void ChequeVoucherWidget::setWorkingDate(const QString& dateStr) {
    if (m_dateEdit) {
        m_dateEdit->setIsoDate(dateStr);
        updateDayOfWeek(m_dateEdit->date());
        updateFiscalYearAndVoucherNo();
    }
}

void ChequeVoucherWidget::updateFiscalYearAndVoucherNo() {
    QDate curDate = m_dateEdit ? m_dateEdit->date() : QDate::currentDate();
    if (!curDate.isValid()) curDate = QDate::currentDate();

    QString isoDate = curDate.toString("yyyy-MM-dd");
    FiscalYearInfo fy = FiscalYearHelper::getFiscalYearForDate(isoDate);
    QString fyStr = fy.name;
    m_fyBadge->setText(fyStr);

    if (!isEditMode()) {
        QString prefix = (m_voucherMode == "Receipt") ? "ChRt" : "ChPt";
        QString nextNo = m_vouchersModel.get_next_voucher_no(prefix, fyStr);
        m_voucherNoInput->setText(nextNo);
    }
}

void ChequeVoucherWidget::updateDayOfWeek(const QDate& date) {
    if (date.isValid() && m_dayOfWeekLabel) {
        m_dayOfWeekLabel->setText(QString("(%1)").arg(date.toString("dddd")));
    }
}

void ChequeVoucherWidget::onDateChanged(const QDate& date) {
    updateDayOfWeek(date);
    updateFiscalYearAndVoucherNo();
}

void ChequeVoucherWidget::openDateDialog(bool isInitial) {
    QString curIso = m_dateEdit ? m_dateEdit->isoDate() : QDate::currentDate().toString("yyyy-MM-dd");
    QString displayDate, isoDate;
    if (VoucherDateDialog::getVoucherDate(this, curIso, &displayDate, &isoDate)) {
        if (!isoDate.isEmpty() && m_dateEdit) {
            m_dateEdit->setIsoDate(isoDate);
            updateDayOfWeek(m_dateEdit->date());
            updateFiscalYearAndVoucherNo();
            focusFirstRow();
        }
    } else {
        if (isInitial) {
            emit backRequested();
        } else {
            focusFirstRow();
        }
    }
}

void ChequeVoucherWidget::resetForm() {
    m_editingVoucherId = 0;
    m_editingVoucherNo.clear();
    m_hasInitialDateOpened = false;
    m_alterBadge->hide();
    m_deleteBtn->hide();

    QDate today = QDate::currentDate();
    if (m_dateEdit) {
        m_dateEdit->setDate(today);
    }
    updateDayOfWeek(today);
    updateFiscalYearAndVoucherNo();

    if (m_narrationInput) m_narrationInput->clear();
    setStatusMessage("", false);

    m_table->setRowCount(0);
    addNewRow("Dr", "", 0.0, 0.0, "");
    addNewRow("Cr", "", 0.0, 0.0, "");

    recalculateTotals();
}

void ChequeVoucherWidget::focusCell(int row, int col) {
    if (row < 0 || row >= m_table->rowCount()) return;
    if (col < 0 || col >= m_table->columnCount()) return;

    QWidget* w = m_table->cellWidget(row, col);
    if (!w) return;

    m_table->setCurrentCell(row, col);
    m_table->scrollToItem(m_table->item(row, col), QAbstractItemView::EnsureVisible);
    w->setFocus(Qt::TabFocusReason);

    if (auto* lineEdit = qobject_cast<QLineEdit*>(w)) {
        lineEdit->selectAll();
    }
}

void ChequeVoucherWidget::setupRowWidgets(int row, const QString& drcr, const QString& ledger, double debit, double credit, const QString& ref) {
    // 0. Type (Dr/Cr)
    auto* typeCombo = new QComboBox(m_table);
    typeCombo->addItems({"Dr", "Cr"});
    typeCombo->setCurrentText(drcr.isEmpty() ? "Dr" : drcr);
    typeCombo->setStyleSheet(
        "QComboBox { background-color: #FFFFFF; color: #0F172A; border: 1px solid #CBD5E1; "
        "border-radius: 4px; padding: 2px 6px; font-weight: 800; font-size: 11px; }"
        "QComboBox:focus { border: 1.5px solid #2563EB; background-color: #EFF6FF; }"
    );
    typeCombo->installEventFilter(this);
    m_table->setCellWidget(row, 0, typeCombo);

    // 1. Particulars (AccountSearchBox)
    auto* searchWidget = new AccountSearchBox(m_table);
    searchWidget->setPartyName(ledger);
    searchWidget->installEventFilter(this);
    connect(searchWidget, &QLineEdit::returnPressed, this, [this]() {
        for (int r = 0; r < m_table->rowCount(); ++r) {
            if (m_table->cellWidget(r, 1) == sender()) {
                auto* tc = qobject_cast<QComboBox*>(m_table->cellWidget(r, 0));
                int nextCol = (tc && tc->currentText() == "Cr") ? 3 : 2;
                QTimer::singleShot(0, this, [this, r, nextCol]() {
                    focusCell(r, nextCol);
                });
                break;
            }
        }
    });
    m_table->setCellWidget(row, 1, searchWidget);

    // 2. Debit Amount
    auto* debitInput = new QLineEdit(m_table);
    debitInput->setAlignment(Qt::AlignRight);
    debitInput->setText(debit > 0.001 ? QString::number(debit, 'f', 2) : "");
    debitInput->setPlaceholderText("0.00");
    debitInput->setStyleSheet(
        "QLineEdit { background-color: #FFFFFF; color: #0F172A; border: 1px solid #CBD5E1; "
        "border-radius: 4px; padding: 2px 8px; font-size: 12px; font-weight: 700; }"
        "QLineEdit:focus { border: 1.5px solid #2563EB; background-color: #EFF6FF; }"
    );
    debitInput->installEventFilter(this);
    connect(debitInput, &QLineEdit::textChanged, this, &ChequeVoucherWidget::onTableRowChanged);
    m_table->setCellWidget(row, 2, debitInput);

    // 3. Credit Amount
    auto* creditInput = new QLineEdit(m_table);
    creditInput->setAlignment(Qt::AlignRight);
    creditInput->setText(credit > 0.001 ? QString::number(credit, 'f', 2) : "");
    creditInput->setPlaceholderText("0.00");
    creditInput->setStyleSheet(
        "QLineEdit { background-color: #FFFFFF; color: #0F172A; border: 1px solid #CBD5E1; "
        "border-radius: 4px; padding: 2px 8px; font-size: 12px; font-weight: 700; }"
        "QLineEdit:focus { border: 1.5px solid #2563EB; background-color: #EFF6FF; }"
    );
    creditInput->installEventFilter(this);
    connect(creditInput, &QLineEdit::textChanged, this, &ChequeVoucherWidget::onTableRowChanged);
    m_table->setCellWidget(row, 3, creditInput);

    // 4. Cheque / Ref No
    auto* refInput = new QLineEdit(m_table);
    refInput->setText(ref);
    refInput->setPlaceholderText("Chq / Ref No");
    refInput->setStyleSheet(
        "QLineEdit { background-color: #FFFFFF; color: #0F172A; border: 1px solid #CBD5E1; "
        "border-radius: 4px; padding: 2px 8px; font-size: 12px; }"
        "QLineEdit:focus { border: 1.5px solid #2563EB; background-color: #EFF6FF; }"
    );
    refInput->installEventFilter(this);
    m_table->setCellWidget(row, 4, refInput);

    // 5. Delete Action Button
    auto* delBtn = new QPushButton("×", m_table);
    delBtn->setCursor(Qt::PointingHandCursor);
    delBtn->setStyleSheet(
        "QPushButton { background-color: transparent; color: #94A3B8; border: none; font-size: 16px; font-weight: bold; } "
        "QPushButton:hover { background-color: #FEE2E2; color: #DC2626; border-radius: 4px; }"
    );
    connect(delBtn, &QPushButton::clicked, this, [this]() {
        for (int r = 0; r < m_table->rowCount(); ++r) {
            if (m_table->cellWidget(r, 5) == sender()) {
                removeRow(r);
                break;
            }
        }
    });
    m_table->setCellWidget(row, 5, delBtn);

    // Connect Dr/Cr type toggle logic
    auto updateColumnStates = [typeCombo, debitInput, creditInput]() {
        bool isDr = (typeCombo->currentText() == "Dr");
        debitInput->setEnabled(isDr);
        creditInput->setEnabled(!isDr);
        if (isDr) {
            creditInput->clear();
            debitInput->setStyleSheet(
                "QLineEdit { background-color: #FFFFFF; color: #0F172A; border: 1px solid #CBD5E1; "
                "border-radius: 4px; padding: 2px 8px; font-size: 12px; font-weight: 700; }"
                "QLineEdit:focus { border: 1.5px solid #2563EB; background-color: #EFF6FF; }"
            );
            creditInput->setStyleSheet(
                "QLineEdit { background-color: #F8FAFC; color: #94A3B8; border: 1px solid #E2E8F0; "
                "border-radius: 4px; padding: 2px 8px; font-size: 12px; }"
            );
        } else {
            debitInput->clear();
            creditInput->setStyleSheet(
                "QLineEdit { background-color: #FFFFFF; color: #0F172A; border: 1px solid #CBD5E1; "
                "border-radius: 4px; padding: 2px 8px; font-size: 12px; font-weight: 700; }"
                "QLineEdit:focus { border: 1.5px solid #2563EB; background-color: #EFF6FF; }"
            );
            debitInput->setStyleSheet(
                "QLineEdit { background-color: #F8FAFC; color: #94A3B8; border: 1px solid #E2E8F0; "
                "border-radius: 4px; padding: 2px 8px; font-size: 12px; }"
            );
        }
    };

    connect(typeCombo, &QComboBox::currentTextChanged, this, [this, updateColumnStates]() {
        updateColumnStates();
        recalculateTotals();
    });

    updateColumnStates();
}

void ChequeVoucherWidget::addNewRow(const QString& drcr, const QString& ledger, double debit, double credit, const QString& ref) {
    int row = m_table->rowCount();
    m_table->insertRow(row);

    QString defaultType = drcr;
    if (defaultType.isEmpty()) {
        if (row > 0) {
            auto* prevCombo = qobject_cast<QComboBox*>(m_table->cellWidget(row - 1, 0));
            defaultType = (prevCombo && prevCombo->currentText() == "Dr") ? "Cr" : "Dr";
        } else {
            defaultType = "Dr";
        }
    }

    setupRowWidgets(row, defaultType, ledger, debit, credit, ref);
    recalculateTotals();
}

void ChequeVoucherWidget::removeRow(int row) {
    if (m_table->rowCount() > 2 && row >= 0 && row < m_table->rowCount()) {
        m_table->removeRow(row);
        recalculateTotals();
    }
}

void ChequeVoucherWidget::onTableRowChanged() {
    recalculateTotals();
}

void ChequeVoucherWidget::recalculateTotals() {
    double totalDr = 0.0;
    double totalCr = 0.0;

    for (int r = 0; r < m_table->rowCount(); ++r) {
        auto* typeCombo = qobject_cast<QComboBox*>(m_table->cellWidget(r, 0));
        auto* debitInput = qobject_cast<QLineEdit*>(m_table->cellWidget(r, 2));
        auto* creditInput = qobject_cast<QLineEdit*>(m_table->cellWidget(r, 3));

        if (!typeCombo) continue;

        if (typeCombo->currentText() == "Dr" && debitInput) {
            totalDr += debitInput->text().toDouble();
        } else if (typeCombo->currentText() == "Cr" && creditInput) {
            totalCr += creditInput->text().toDouble();
        }
    }

    totalDr = FinancialMathService::instance().round2(totalDr);
    totalCr = FinancialMathService::instance().round2(totalCr);

    m_totalDebitLabel->setText(FinancialMathService::instance().formatInr(totalDr));
    m_totalCreditLabel->setText(FinancialMathService::instance().formatInr(totalCr));

    double diff = std::abs(totalDr - totalCr);
    if (diff < 0.005) {
        m_balanceBadge->setText("Balanced (₹0.00)");
        m_balanceBadge->setStyleSheet(
            "background-color: #DCFCE7; color: #166534; border: 1px solid #86EFAC; "
            "font-weight: 800; font-size: 12px; padding: 4px 12px; border-radius: 6px;"
        );
    } else {
        m_balanceBadge->setText(QString("Diff: %1").arg(FinancialMathService::instance().formatInr(diff)));
        m_balanceBadge->setStyleSheet(
            "background-color: #FEE2E2; color: #991B1B; border: 1px solid #FCA5A5; "
            "font-weight: 800; font-size: 12px; padding: 4px 12px; border-radius: 6px;"
        );
    }
}

bool ChequeVoucherWidget::loadVoucherForEditing(const QVariant& vchNoOrId, const QString& dateHint) {
    QVariantMap data = m_vouchersModel.get_cheque_voucher(vchNoOrId, dateHint);
    if (data.isEmpty()) {
        setStatusMessage("Voucher not found for alteration.", true);
        return false;
    }

    m_editingVoucherId = data.value("id").toInt();
    m_editingVoucherNo = data.value("voucher_no").toString();

    m_voucherNoInput->setText(m_editingVoucherNo);
    QString vDate = data.value("voucher_date").toString();
    if (!vDate.isEmpty()) {
        m_dateEdit->setIsoDate(vDate);
    }

    QString vType = data.value("voucher_type").toString();
    setVoucherMode(vType.contains("Receipt", Qt::CaseInsensitive) ? "Receipt" : "Payment");

    m_narrationInput->setText(data.value("narration").toString());

    m_alterBadge->setText(QString("ALTERATION MODE (#%1)").arg(m_editingVoucherNo));
    m_alterBadge->show();
    m_deleteBtn->show();

    // Populate rows
    m_table->setRowCount(0);
    QVariantList rows = data.value("rows").toList();
    if (rows.isEmpty()) {
        addNewRow("Dr", data.value("party_name").toString(), data.value("amount").toDouble(), 0.0, data.value("instrument_no").toString());
        addNewRow("Cr", data.value("account_type").toString(), 0.0, data.value("amount").toDouble(), data.value("instrument_no").toString());
    } else {
        for (const auto& rVar : rows) {
            QVariantMap r = rVar.toMap();
            QString drcr = r.value("drcr").toString();
            QString ledger = r.value("ledgerName").toString();
            double dAmt = r.value("debitAmt").toDouble();
            double cAmt = r.value("creditAmt").toDouble();
            QString ref = r.value("refNo").toString();
            addNewRow(drcr, ledger, dAmt, cAmt, ref);
        }
    }

    recalculateTotals();
    setStatusMessage(QString("Loaded Voucher #%1 in Alteration Mode.").arg(m_editingVoucherNo), false);
    return true;
}

void ChequeVoucherWidget::saveVoucher() {
    double totalDr = 0.0;
    double totalCr = 0.0;
    QVariantList rows;

    for (int r = 0; r < m_table->rowCount(); ++r) {
        auto* typeCombo = qobject_cast<QComboBox*>(m_table->cellWidget(r, 0));
        auto* searchWidget = qobject_cast<AccountSearchBox*>(m_table->cellWidget(r, 1));
        auto* debitInput = qobject_cast<QLineEdit*>(m_table->cellWidget(r, 2));
        auto* creditInput = qobject_cast<QLineEdit*>(m_table->cellWidget(r, 3));
        auto* refInput = qobject_cast<QLineEdit*>(m_table->cellWidget(r, 4));

        if (!typeCombo || !searchWidget) continue;

        QString drcr = typeCombo->currentText();
        QString ledger = searchWidget->currentPartyName();
        double dAmt = debitInput ? debitInput->text().toDouble() : 0.0;
        double cAmt = creditInput ? creditInput->text().toDouble() : 0.0;
        QString ref = refInput ? refInput->text().trimmed() : "";

        if (ledger.isEmpty()) {
            if (dAmt > 0.0 || cAmt > 0.0) {
                setStatusMessage(QString("Please select an Account/Ledger for row %1.").arg(r + 1), true);
                focusCell(r, 1);
                return;
            }
            continue;
        }

        if (drcr == "Dr") {
            totalDr += dAmt;
        } else {
            totalCr += cAmt;
        }

        QVariantMap rowMap;
        rowMap["drcr"] = drcr;
        rowMap["ledgerName"] = ledger;
        rowMap["debitAmt"] = dAmt;
        rowMap["creditAmt"] = cAmt;
        rowMap["refNo"] = ref;
        rows.append(rowMap);
    }

    totalDr = FinancialMathService::instance().round2(totalDr);
    totalCr = FinancialMathService::instance().round2(totalCr);

    if (rows.size() < 2) {
        setStatusMessage("Please enter at least two transaction rows.", true);
        return;
    }

    if (std::abs(totalDr - totalCr) > 0.005) {
        setStatusMessage(QString("Total Debit (%1) does not equal Total Credit (%2).").arg(
            FinancialMathService::instance().formatInr(totalDr),
            FinancialMathService::instance().formatInr(totalCr)
        ), true);
        return;
    }

    if (totalDr <= 0.001) {
        setStatusMessage("Voucher amount must be greater than zero.", true);
        return;
    }

    QString vchNo = m_voucherNoInput->text().trimmed();
    QString vchDate = m_dateEdit->isoDate();
    QString narr = m_narrationInput->text().trimmed();
    QString vchType = (m_voucherMode == "Receipt") ? "Cheque Receipt" : "Cheque Payment";

    bool ok = m_vouchersModel.save_multi_row_voucher(
        m_editingVoucherId,
        vchType,
        vchNo,
        vchDate,
        narr,
        rows
    );

    if (ok) {
        QString savedNo = vchNo;
        CustomMessageBox::information(this, "Success", QString("%1 #%2 saved successfully!").arg(vchType, savedNo));
        emit voucherSaved(savedNo);
        resetForm();
    } else {
        setStatusMessage("Failed to save voucher in database.", true);
    }
}

void ChequeVoucherWidget::deleteVoucher() {
    if (m_editingVoucherId <= 0) return;

    if (CustomMessageBox::question(this, "Confirm Deletion",
            QString("Are you sure you want to permanently delete Voucher #%1?").arg(m_editingVoucherNo))) {
        bool ok = m_vouchersModel.delete_voucher(m_editingVoucherId);
        if (ok) {
            QString deletedNo = m_editingVoucherNo;
            CustomMessageBox::information(this, "Deleted", QString("Voucher #%1 deleted.").arg(deletedNo));
            emit voucherDeleted(deletedNo);
            resetForm();
        } else {
            setStatusMessage("Failed to delete voucher.", true);
        }
    }
}

void ChequeVoucherWidget::setStatusMessage(const QString& message, bool isError) {
    if (m_statusLabel) {
        m_statusLabel->setText(message);
        m_statusLabel->setStyleSheet(isError ? "color: #DC2626; font-weight: 700; border: none; background: transparent;" : "color: #16A34A; font-weight: 700; border: none; background: transparent;");
    }
}

void ChequeVoucherWidget::focusFirstRow() {
    if (m_table->rowCount() > 0) {
        focusCell(0, 1);
    }
}

void ChequeVoucherWidget::showEvent(QShowEvent* event) {
    QWidget::showEvent(event);
}

void ChequeVoucherWidget::keyPressEvent(QKeyEvent* event) {
    if (event->key() == Qt::Key_F3) {
        setVoucherMode("Payment");
        event->accept();
        return;
    }
    if (event->key() == Qt::Key_F4) {
        setVoucherMode("Receipt");
        event->accept();
        return;
    }
    if (event->key() == Qt::Key_S && (event->modifiers() & Qt::ControlModifier)) {
        saveVoucher();
        event->accept();
        return;
    }
    if (event->key() == Qt::Key_D && (event->modifiers() & Qt::ControlModifier)) {
        deleteVoucher();
        event->accept();
        return;
    }
    if (event->key() == Qt::Key_Insert) {
        addNewRow();
        event->accept();
        return;
    }
    if (event->key() == Qt::Key_Escape) {
        emit backRequested();
        event->accept();
        return;
    }
    QWidget::keyPressEvent(event);
}

bool ChequeVoucherWidget::eventFilter(QObject* watched, QEvent* event) {
    if (event->type() == QEvent::KeyPress) {
        auto* keyEvent = static_cast<QKeyEvent*>(event);
        int key = keyEvent->key();

        if (key == Qt::Key_Escape) {
            emit backRequested();
            return true;
        }

        if (key == Qt::Key_S && (keyEvent->modifiers() & Qt::ControlModifier)) {
            saveVoucher();
            return true;
        }

        if (key == Qt::Key_D && (keyEvent->modifiers() & Qt::ControlModifier)) {
            deleteVoucher();
            return true;
        }

        if (key == Qt::Key_F3) {
            setVoucherMode("Payment");
            return true;
        }

        if (key == Qt::Key_F4) {
            setVoucherMode("Receipt");
            return true;
        }

        if (key == Qt::Key_Insert) {
            addNewRow();
            return true;
        }

        // Header controls: Date & Voucher No
        if (watched == m_dateEdit) {
            if (key == Qt::Key_Return || key == Qt::Key_Enter || key == Qt::Key_Tab || key == Qt::Key_Down || key == Qt::Key_Right) {
                if (m_voucherNoInput) {
                    m_voucherNoInput->setFocus();
                    m_voucherNoInput->selectAll();
                } else if (m_table->rowCount() > 0) {
                    focusCell(0, 1);
                }
                return true;
            }
        }

        if (watched == m_voucherNoInput) {
            if (key == Qt::Key_Return || key == Qt::Key_Enter || key == Qt::Key_Tab || key == Qt::Key_Down || key == Qt::Key_Right) {
                if (m_table->rowCount() > 0) {
                    focusCell(0, 1);
                }
                return true;
            } else if (key == Qt::Key_Left || key == Qt::Key_Up) {
                if (m_dateEdit) {
                    m_dateEdit->setFocus();
                }
                return true;
            }
        }

        // Footer: Narration Input
        if (watched == m_narrationInput) {
            if (key == Qt::Key_Return || key == Qt::Key_Enter) {
                saveVoucher();
                return true;
            } else if (key == Qt::Key_Up) {
                if (m_table->rowCount() > 0) {
                    focusCell(m_table->rowCount() - 1, 4);
                }
                return true;
            } else if (key == Qt::Key_Left && (m_narrationInput->cursorPosition() == 0 || m_narrationInput->hasSelectedText() || m_narrationInput->text().isEmpty())) {
                if (m_table->rowCount() > 0) {
                    focusCell(m_table->rowCount() - 1, 4);
                }
                return true;
            }
            return QWidget::eventFilter(watched, event);
        }

        // Find cell row and column
        int targetRow = -1;
        int targetCol = -1;
        for (int r = 0; r < m_table->rowCount(); ++r) {
            for (int c = 0; c < m_table->columnCount(); ++c) {
                if (m_table->cellWidget(r, c) == watched) {
                    targetRow = r;
                    targetCol = c;
                    break;
                }
            }
            if (targetRow != -1) break;
        }

        if (targetRow != -1) {
            auto* typeCombo = qobject_cast<QComboBox*>(m_table->cellWidget(targetRow, 0));
            bool isDr = !typeCombo || (typeCombo->currentText() == "Dr");
            int amtCol = isDr ? 2 : 3;

            // 1. Column 0: Type Combo (Dr/Cr)
            if (targetCol == 0) {
                auto* combo = qobject_cast<QComboBox*>(watched);
                if (key == Qt::Key_Return || key == Qt::Key_Enter || key == Qt::Key_Tab || key == Qt::Key_Right) {
                    focusCell(targetRow, 1);
                    return true;
                } else if (key == Qt::Key_Left) {
                    if (targetRow > 0) {
                        focusCell(targetRow - 1, 4);
                    } else if (m_voucherNoInput) {
                        m_voucherNoInput->setFocus();
                        m_voucherNoInput->selectAll();
                    }
                    return true;
                } else if (key == Qt::Key_Space) {
                    if (combo) {
                        combo->setCurrentText(combo->currentText() == "Dr" ? "Cr" : "Dr");
                    }
                    return true;
                } else if (key == Qt::Key_D) {
                    if (combo) combo->setCurrentText("Dr");
                    focusCell(targetRow, 1);
                    return true;
                } else if (key == Qt::Key_C) {
                    if (combo) combo->setCurrentText("Cr");
                    focusCell(targetRow, 1);
                    return true;
                } else if (key == Qt::Key_Up && targetRow > 0) {
                    focusCell(targetRow - 1, 0);
                    return true;
                } else if (key == Qt::Key_Down && targetRow + 1 < m_table->rowCount()) {
                    focusCell(targetRow + 1, 0);
                    return true;
                }
            }
            // 2. Column 1: Particulars (AccountSearchBox)
            else if (targetCol == 1) {
                auto* search = qobject_cast<AccountSearchBox*>(watched);

                if (key == Qt::Key_Return || key == Qt::Key_Enter || key == Qt::Key_Tab) {
                    if (search && !search->text().trimmed().isEmpty()) {
                        if (search->isPopupVisible()) {
                            search->selectCurrentListItem();
                        }
                        focusCell(targetRow, amtCol);
                        return true;
                    } else {
                        // Empty ledger field: finish rows and move to narration
                        if (search) search->closeSearchPopup();
                        if (targetRow >= 2 && m_table->rowCount() > 2) {
                            removeRow(targetRow);
                            if (m_narrationInput) {
                                m_narrationInput->setFocus();
                                m_narrationInput->selectAll();
                            }
                            return true;
                        } else if (m_narrationInput) {
                            m_narrationInput->setFocus();
                            m_narrationInput->selectAll();
                            return true;
                        }
                    }
                } else if (key == Qt::Key_Right) {
                    if (!search || search->cursorPosition() >= search->text().length() || search->hasSelectedText() || search->text().isEmpty()) {
                        if (search && search->isPopupVisible() && !search->text().trimmed().isEmpty()) {
                            search->selectCurrentListItem();
                        }
                        focusCell(targetRow, amtCol);
                        return true;
                    }
                } else if (key == Qt::Key_Left) {
                    if (!search || search->cursorPosition() == 0 || search->hasSelectedText() || search->text().isEmpty()) {
                        if (search) search->closeSearchPopup();
                        focusCell(targetRow, 0);
                        return true;
                    }
                } else if (key == Qt::Key_Up) {
                    if (search && search->isPopupVisible()) {
                        return false;
                    }
                    if (targetRow > 0) {
                        focusCell(targetRow - 1, 1);
                        return true;
                    }
                } else if (key == Qt::Key_Down) {
                    if (search && search->isPopupVisible()) {
                        return false;
                    }
                    if (targetRow + 1 < m_table->rowCount()) {
                        focusCell(targetRow + 1, 1);
                        return true;
                    }
                }
            }
            // 3. Column 2 / 3: Debit or Credit Amount
            else if (targetCol == 2 || targetCol == 3) {
                auto* edit = qobject_cast<QLineEdit*>(watched);
                if (key == Qt::Key_Return || key == Qt::Key_Enter || key == Qt::Key_Tab) {
                    focusCell(targetRow, 4);
                    return true;
                } else if (key == Qt::Key_Right) {
                    if (!edit || edit->cursorPosition() >= edit->text().length() || edit->hasSelectedText() || edit->text().isEmpty()) {
                        focusCell(targetRow, 4);
                        return true;
                    }
                } else if (key == Qt::Key_Left) {
                    if (!edit || edit->cursorPosition() == 0 || edit->hasSelectedText() || edit->text().isEmpty()) {
                        focusCell(targetRow, 1);
                        return true;
                    }
                } else if (key == Qt::Key_Up && targetRow > 0) {
                    auto* prevCombo = qobject_cast<QComboBox*>(m_table->cellWidget(targetRow - 1, 0));
                    int prevAmtCol = (prevCombo && prevCombo->currentText() == "Cr") ? 3 : 2;
                    focusCell(targetRow - 1, prevAmtCol);
                    return true;
                } else if (key == Qt::Key_Down && targetRow + 1 < m_table->rowCount()) {
                    auto* nextCombo = qobject_cast<QComboBox*>(m_table->cellWidget(targetRow + 1, 0));
                    int nextAmtCol = (nextCombo && nextCombo->currentText() == "Cr") ? 3 : 2;
                    focusCell(targetRow + 1, nextAmtCol);
                    return true;
                }
            }
            // 4. Column 4: Cheque / Ref No
            else if (targetCol == 4) {
                auto* edit = qobject_cast<QLineEdit*>(watched);
                if (key == Qt::Key_Return || key == Qt::Key_Enter || key == Qt::Key_Tab) {
                    auto* search = qobject_cast<AccountSearchBox*>(m_table->cellWidget(targetRow, 1));
                    bool hasLedger = search && !search->currentPartyName().isEmpty();

                    if (!hasLedger) {
                        if (m_narrationInput) {
                            m_narrationInput->setFocus();
                            m_narrationInput->selectAll();
                        }
                        return true;
                    }

                    if (targetRow + 1 < m_table->rowCount()) {
                        focusCell(targetRow + 1, 1);
                    } else {
                        double totalDr = m_totalDebitLabel ? m_totalDebitLabel->text().remove("₹").remove(",").trimmed().toDouble() : 0.0;
                        double totalCr = m_totalCreditLabel ? m_totalCreditLabel->text().remove("₹").remove(",").trimmed().toDouble() : 0.0;
                        if (std::abs(totalDr - totalCr) < 0.005 && totalDr > 0.0) {
                            if (m_narrationInput) {
                                m_narrationInput->setFocus();
                                m_narrationInput->selectAll();
                            }
                        } else {
                            addNewRow();
                            focusCell(targetRow + 1, 1);
                        }
                    }
                    return true;
                } else if (key == Qt::Key_Left) {
                    if (!edit || edit->cursorPosition() == 0 || edit->hasSelectedText() || edit->text().isEmpty()) {
                        focusCell(targetRow, amtCol);
                        return true;
                    }
                } else if (key == Qt::Key_Right) {
                    if (!edit || edit->cursorPosition() >= edit->text().length() || edit->hasSelectedText() || edit->text().isEmpty()) {
                        if (targetRow + 1 < m_table->rowCount()) {
                            focusCell(targetRow + 1, 0);
                        } else if (m_narrationInput) {
                            m_narrationInput->setFocus();
                            m_narrationInput->selectAll();
                        }
                        return true;
                    }
                } else if (key == Qt::Key_Up && targetRow > 0) {
                    focusCell(targetRow - 1, 4);
                    return true;
                } else if (key == Qt::Key_Down && targetRow + 1 < m_table->rowCount()) {
                    focusCell(targetRow + 1, 4);
                    return true;
                } else if (key == Qt::Key_Down && targetRow + 1 == m_table->rowCount() && m_narrationInput) {
                    m_narrationInput->setFocus();
                    m_narrationInput->selectAll();
                    return true;
                }
            }
        }
    }
    return QWidget::eventFilter(watched, event);
}
