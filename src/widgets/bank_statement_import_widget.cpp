#include "bank_statement_import_widget.h"
#include "account_search_delegate.h"
#include "kbd_badge_button.h"
#include "custom_dialogs.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QHeaderView>
#include <QGroupBox>
#include <QShortcut>
#include <QFileDialog>

#include "../database_manager.h"
#include <QSqlQuery>

namespace MahadevERP {

BankStatementImportWidget::BankStatementImportWidget(BankStatementController* controller,
                                                     PrintExportController* printExportCtrl,
                                                     QWidget* parent)
    : QWidget(parent)
    , m_controller(controller)
    , m_printExportCtrl(printExportCtrl)
{
    setupUi();
    resetForm();
}

void BankStatementImportWidget::setupUi() {
    auto* rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(14, 12, 14, 12);
    rootLayout->setSpacing(10);

    // ========================================================================
    // TIER 1: STICKY HEADER CARD
    // ========================================================================
    auto* headerCard = new QFrame(this);
    headerCard->setFixedHeight(50);
    headerCard->setStyleSheet(
        "QFrame { background-color: #FFFFFF; border: 1px solid #E2E8F0; border-radius: 8px; }"
    );
    auto* headerLayout = new QHBoxLayout(headerCard);
    headerLayout->setContentsMargins(14, 10, 14, 10);

    auto* titleCol = new QVBoxLayout();
    titleCol->setSpacing(2);
    auto* titleLabel = new QLabel("Bank Statement Automated Import & Reconciliation", headerCard);
    titleLabel->setStyleSheet("font-size: 16px; font-weight: 800; color: #0F172A; border: none; background: transparent;");
    auto* subLabel = new QLabel("Parse Bank PDF & Excel e-statements, auto-categorize parties, detect duplicate vouchers, and post batch entries.", headerCard);
    subLabel->setStyleSheet("font-size: 11px; color: #64748B; border: none; background: transparent;");
    titleCol->addWidget(titleLabel);
    titleCol->addWidget(subLabel);
    headerLayout->addLayout(titleCol);
    headerLayout->addStretch();

    auto* backBtn = new KbdBadgeButton("← Back to Dashboard", "Esc", headerCard);
    backBtn->setPrimaryColor("#F1F5F9", "#E2E8F0");
    backBtn->setTextColor("#475569");
    connect(backBtn, &QPushButton::clicked, this, &BankStatementImportWidget::backRequested);
    headerLayout->addWidget(backBtn);
    rootLayout->addWidget(headerCard);

    // ========================================================================
    // TIER 2: FILE SELECTION & FILTER CONTROLS CARD
    // ========================================================================
    auto* filterCard = new QFrame(this);
    filterCard->setStyleSheet(
        "QFrame { background-color: #FFFFFF; border: 1px solid #CBD5E1; border-radius: 8px; }"
        "QLabel { color: #475569; font-weight: 700; font-size: 11.5px; border: none; background: transparent; }"
        "QLineEdit, QComboBox { background-color: #FFFFFF; border: 1.5px solid #CBD5E1; border-radius: 6px; padding: 4px 8px; font-size: 12px; color: #0F172A; font-weight: 600; }"
        "QLineEdit:focus, QComboBox:focus { border-color: #2563EB; background-color: #F8FAFC; }"
        "QCheckBox { color: #1E293B; font-weight: 700; font-size: 12px; border: none; background: transparent; }"
    );
    auto* filterLayout = new QVBoxLayout(filterCard);
    filterLayout->setContentsMargins(12, 10, 12, 10);
    filterLayout->setSpacing(8);

    // Row 1: File selector & Destination Bank Ledger
    auto* row1 = new QHBoxLayout();
    row1->setSpacing(10);

    auto* lblFile = new QLabel("Bank Statement File *:", filterCard);
    row1->addWidget(lblFile);

    m_filePathEdit = new QLineEdit(filterCard);
    m_filePathEdit->setPlaceholderText("Select Bank Statement PDF or Excel (*.pdf, *.xls, *.xlsx, *.csv)...");
    m_filePathEdit->setReadOnly(true);
    m_filePathEdit->setStyleSheet("background-color: #F8FAFC; border: 1.5px solid #CBD5E1; border-radius: 6px; padding: 5px 8px; font-size: 12px; color: #0F172A;");
    row1->addWidget(m_filePathEdit, 1);

    auto* browseBtn = new QPushButton("Browse File 📁", filterCard);
    browseBtn->setCursor(Qt::PointingHandCursor);
    browseBtn->setStyleSheet("QPushButton { background-color: #2563EB; color: #FFFFFF; font-weight: 800; font-size: 12px; padding: 5px 14px; border-radius: 6px; border: none; } QPushButton:hover { background-color: #1D4ED8; }");
    connect(browseBtn, &QPushButton::clicked, this, &BankStatementImportWidget::onSelectFileClicked);
    row1->addWidget(browseBtn);

    row1->addSpacing(10);
    auto* lblLedger = new QLabel("Destination Bank Ledger *:", filterCard);
    row1->addWidget(lblLedger);

    m_bankLedgerCombo = new QComboBox(filterCard);
    m_bankLedgerCombo->setEditable(true);
    m_bankLedgerCombo->setMinimumWidth(220);
    populateBankLedgers();
    row1->addWidget(m_bankLedgerCombo);

    filterLayout->addLayout(row1);

    // Row 2: Search, Quick Filters & Selection Buttons
    auto* row2 = new QHBoxLayout();
    row2->setSpacing(10);

    m_selectAllCheck = new QCheckBox("Select All", filterCard);
    connect(m_selectAllCheck, &QCheckBox::toggled, this, &BankStatementImportWidget::onSelectAllToggled);
    row2->addWidget(m_selectAllCheck);

    auto* deselectDupBtn = new QPushButton("Deselect Duplicates ⚠️", filterCard);
    deselectDupBtn->setCursor(Qt::PointingHandCursor);
    deselectDupBtn->setStyleSheet("QPushButton { background-color: #FEF3C7; border: 1px solid #FCD34D; color: #92400E; font-weight: 800; font-size: 11.5px; padding: 4px 10px; border-radius: 6px; } QPushButton:hover { background-color: #FDE68A; }");
    connect(deselectDupBtn, &QPushButton::clicked, this, &BankStatementImportWidget::onDeselectDuplicatesClicked);
    row2->addWidget(deselectDupBtn);

    row2->addSpacing(10);

    auto* lblFilter = new QLabel("Filter View:", filterCard);
    row2->addWidget(lblFilter);

    m_filterTypeCombo = new QComboBox(filterCard);
    m_filterTypeCombo->addItems({
        "ALL",
        "UNMATCHED",
        "DEPOSITS (RECEIPTS)",
        "WITHDRAWALS (PAYMENTS)",
        "BANK CHARGES",
        "INTEREST DEBITS",
        "DUPLICATES"
    });
    connect(m_filterTypeCombo, &QComboBox::currentIndexChanged, this, &BankStatementImportWidget::onFilterChanged);
    row2->addWidget(m_filterTypeCombo);

    auto* lblSearch = new QLabel("Search Query:", filterCard);
    row2->addWidget(lblSearch);

    m_searchEdit = new QLineEdit(filterCard);
    m_searchEdit->setPlaceholderText("Search narration, UTR, party name...");
    m_searchEdit->setClearButtonEnabled(true);
    connect(m_searchEdit, &QLineEdit::textChanged, this, &BankStatementImportWidget::onFilterChanged);
    row2->addWidget(m_searchEdit, 1);

    filterLayout->addLayout(row2);
    rootLayout->addWidget(filterCard);

    // ========================================================================
    // TIER 3: HIGH PERFORMANCE DATA SURFACE (TABLE)
    // ========================================================================
    m_table = new QTableWidget(this);
    m_table->setColumnCount(10);
    m_table->setHorizontalHeaderLabels({
        "Select", "#", "Date", "Type", "Narration & UTR Ref",
        "Debit Leg (Dr Account)", "Credit Leg (Cr Account)", "Amount (₹)", "Balance (₹)", "Status"
    });

    m_accountDelegate = new AccountSearchDelegate(this);
    m_table->setItemDelegateForColumn(5, m_accountDelegate);
    m_table->setItemDelegateForColumn(6, m_accountDelegate);

    connect(m_accountDelegate, &AccountSearchDelegate::partyChosen, this, [this](int row, int col, const QString& partyName) {
        if (!m_controller) return;
        auto* mdl = m_controller->rowsModel();
        if (!mdl || row < 0 || row >= mdl->count()) return;

        auto rowMap = mdl->getRow(row);
        QString extracted = rowMap.value("extractedParty").toString();

        if (col == 5) {
            mdl->setRowDrAccount(row, partyName);
        } else if (col == 6) {
            mdl->setRowCrAccount(row, partyName);
        }

        if (!extracted.isEmpty() && !partyName.isEmpty()) {
            m_controller->saveAlias(extracted, partyName);
        }
        updateSummary();
    });

    m_table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(4, QHeaderView::Stretch);
    m_table->horizontalHeader()->setSectionResizeMode(5, QHeaderView::Interactive);
    m_table->horizontalHeader()->resizeSection(5, 175);
    m_table->horizontalHeader()->setSectionResizeMode(6, QHeaderView::Interactive);
    m_table->horizontalHeader()->resizeSection(6, 175);
    m_table->horizontalHeader()->setSectionResizeMode(7, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(8, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(9, QHeaderView::ResizeToContents);
    m_table->verticalHeader()->setVisible(false);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setAlternatingRowColors(true);
    m_table->setStyleSheet(
        "QTableWidget { background-color: #FFFFFF; border: 1px solid #CBD5E1; border-radius: 8px; gridline-color: #F1F5F9; font-size: 12px; color: #0F172A; }"
        "QTableWidget::item { padding: 4px 6px; }"
        "QTableWidget::item:selected { background-color: #EFF6FF; color: #1E3A8A; font-weight: bold; }"
        "QHeaderView::section { background-color: #0F172A; color: #FFFFFF; font-weight: 800; font-size: 11px; padding: 6px 8px; border: none; }"
    );
    connect(m_table, &QTableWidget::itemChanged, this, &BankStatementImportWidget::onTableItemChanged);
    rootLayout->addWidget(m_table, 1);

    // ========================================================================
    // TIER 4: SUMMARY METRICS FOOTER & ACTION BAR
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

    auto m1 = createMetric("PARSED TRANSACTIONS", "0", "#2563EB");
    m_totalCountLabel = m1.second;
    footerRow->addWidget(m1.first, 1);

    auto m2 = createMetric("TOTAL WITHDRAWALS (DR)", "₹ 0.00", "#DC2626");
    m_totalWithdrawalsLabel = m2.second;
    footerRow->addWidget(m2.first, 1);

    auto m3 = createMetric("TOTAL DEPOSITS (CR)", "₹ 0.00", "#16A34A");
    m_totalDepositsLabel = m3.second;
    footerRow->addWidget(m3.first, 1);

    auto m4 = createMetric("EXISTING / DUPLICATES", "0", "#D97706");
    m_duplicateCountLabel = m4.second;
    footerRow->addWidget(m4.first, 1);

    auto m5 = createMetric("SELECTED TO POST", "0", "#0284C7");
    m_selectedCountLabel = m5.second;
    footerRow->addWidget(m5.first, 1);

    m_postBtn = new KbdBadgeButton("Post Selected to Books", "Ctrl+S", this);
    m_postBtn->setPrimaryColor("#16A34A", "#15803D");
    m_postBtn->setTextColor("#FFFFFF");
    m_postBtn->setMinimumWidth(220);
    connect(m_postBtn, &QPushButton::clicked, this, &BankStatementImportWidget::onPostSelectedClicked);
    footerRow->addWidget(m_postBtn, 0);

    rootLayout->addLayout(footerRow);

    connect(new QShortcut(QKeySequence(Qt::Key_Escape), this), &QShortcut::activated, this, &BankStatementImportWidget::backRequested);
    connect(new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_S), this), &QShortcut::activated, this, &BankStatementImportWidget::onPostSelectedClicked);
}

void BankStatementImportWidget::showEvent(QShowEvent* event) {
    QWidget::showEvent(event);
    populateBankLedgers();
}

void BankStatementImportWidget::populateBankLedgers() {
    QString current = m_bankLedgerCombo->currentText();
    m_bankLedgerCombo->clear();
    QVariantList rows = DatabaseManager::instance().executeQuery(
        "SELECT name FROM parties WHERE group_name LIKE '%Bank%' OR group_name LIKE '%Cash%' OR name LIKE '%Bank%' OR name LIKE '%Cash%' ORDER BY name ASC;"
    );
    for (const QVariant& rowVar : rows) {
        QString name = rowVar.toMap().value("name").toString();
        if (!name.isEmpty()) {
            m_bankLedgerCombo->addItem(name);
        }
    }
    if (m_bankLedgerCombo->count() == 0) {
        QVariantList allParties = DatabaseManager::instance().executeQuery(
            "SELECT name FROM parties ORDER BY name ASC;"
        );
        for (const QVariant& rowVar : allParties) {
            QString name = rowVar.toMap().value("name").toString();
            if (!name.isEmpty()) {
                m_bankLedgerCombo->addItem(name);
            }
        }
    }
    if (!current.isEmpty()) {
        int idx = m_bankLedgerCombo->findText(current);
        if (idx >= 0) {
            m_bankLedgerCombo->setCurrentIndex(idx);
        } else {
            m_bankLedgerCombo->setEditText(current);
        }
    }
}

void BankStatementImportWidget::resetForm() {
    m_filePathEdit->clear();
    m_table->setRowCount(0);
    if (m_controller) {
        m_controller->rowsModel()->clear();
    }
    populateBankLedgers();
    updateSummary();
}

void BankStatementImportWidget::onSelectFileClicked() {
    QString path = QFileDialog::getOpenFileName(this, "Select Bank Statement", "", "Bank Statements (*.pdf *.xls *.xlsx *.csv);;All Files (*.*)");
    if (path.isEmpty()) return;

    m_filePathEdit->setText(path);

    if (m_controller) {
        m_controller->setBankLedger(m_bankLedgerCombo->currentText());
        bool ok = m_controller->loadStatement(path);
        if (ok) {
            refreshTable();
            updateSummary();
        } else {
            CustomMessageBox::showCritical(this, "Parse Failed", "Failed to parse bank statement file: " + m_controller->statusMessage());
        }
    }
}

void BankStatementImportWidget::refreshTable() {
    m_table->blockSignals(true);
    m_table->setRowCount(0);

    if (!m_controller) {
        m_table->blockSignals(false);
        return;
    }

    auto* mdl = m_controller->rowsModel();
    int count = mdl ? mdl->count() : 0;

    for (int i = 0; i < count; ++i) {
        auto rowMap = mdl->getRow(i);
        m_table->insertRow(i);

        bool isSelected = rowMap.value("isSelected").toBool();
        bool isDup = rowMap.value("isDuplicate").toBool();
        QString confidence = rowMap.value("confidence").toString();
        double wVal = rowMap.value("withdrawal").toDouble();
        double dVal = rowMap.value("deposit").toDouble();
        double bVal = rowMap.value("balance").toDouble();
        double amt = rowMap.value("amount").toDouble();
        int rIdx = rowMap.value("rowIndex").toInt();
        if (rIdx <= 0) rIdx = i + 1;

        // 0. Checkbox
        auto* checkItem = new QTableWidgetItem();
        checkItem->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled | Qt::ItemIsSelectable);
        checkItem->setCheckState(isSelected ? Qt::Checked : Qt::Unchecked);
        checkItem->setTextAlignment(Qt::AlignCenter);
        checkItem->setData(Qt::UserRole, i);
        m_table->setItem(i, 0, checkItem);

        // 1. # (Index)
        auto* numItem = new QTableWidgetItem(QString::number(rIdx));
        numItem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
        numItem->setTextAlignment(Qt::AlignCenter);
        numItem->setForeground(QBrush(QColor("#64748B")));
        m_table->setItem(i, 1, numItem);

        // 2. Date
        QString dateStr = rowMap.value("date").toString();
        if (dateStr.isEmpty()) dateStr = rowMap.value("vDate").toString();
        if (dateStr.isEmpty()) dateStr = rowMap.value("isoDate").toString();
        auto* dateItem = new QTableWidgetItem(dateStr);
        dateItem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
        dateItem->setForeground(QBrush(QColor("#1E293B")));
        dateItem->setFont(QFont("Segoe UI", 9, QFont::Bold));
        m_table->setItem(i, 2, dateItem);

        // 3. Type (Receipt / Payment)
        QString vType = dVal > 0.001 ? "Receipt" : "Payment";
        auto* typeItem = new QTableWidgetItem(vType);
        typeItem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
        typeItem->setTextAlignment(Qt::AlignCenter);
        if (dVal > 0.001) {
            typeItem->setForeground(QBrush(QColor("#15803D")));
            typeItem->setBackground(QBrush(QColor("#F0FDF4")));
        } else {
            typeItem->setForeground(QBrush(QColor("#B91C1C")));
            typeItem->setBackground(QBrush(QColor("#FEF2F2")));
        }
        typeItem->setFont(QFont("Segoe UI", 9, QFont::Bold));
        m_table->setItem(i, 3, typeItem);

        // 4. Narration & UTR
        QString narr = rowMap.value("narration").toString();
        QString utr = rowMap.value("utrRef").toString();
        QString fullNarr = narr;
        if (!utr.isEmpty() && !narr.contains(utr)) {
            fullNarr = QString("[%1] %2").arg(utr, narr);
        }
        auto* narrItem = new QTableWidgetItem(fullNarr);
        narrItem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
        narrItem->setForeground(QBrush(QColor("#0F172A")));
        m_table->setItem(i, 4, narrItem);

        // 5. Debit Leg (Dr Account)
        QString drAcc = rowMap.value("drAccount").toString();
        auto* drItem = new QTableWidgetItem(drAcc.isEmpty() ? "Select Dr..." : drAcc);
        drItem->setFlags(Qt::ItemIsEditable | Qt::ItemIsEnabled | Qt::ItemIsSelectable);
        drItem->setForeground(QBrush(drAcc.isEmpty() ? QColor("#DC2626") : QColor("#1D4ED8")));
        drItem->setFont(QFont("Segoe UI", 9, QFont::Bold));
        m_table->setItem(i, 5, drItem);

        // 6. Credit Leg (Cr Account)
        QString crAcc = rowMap.value("crAccount").toString();
        auto* crItem = new QTableWidgetItem(crAcc.isEmpty() ? "Select Cr..." : crAcc);
        crItem->setFlags(Qt::ItemIsEditable | Qt::ItemIsEnabled | Qt::ItemIsSelectable);
        crItem->setForeground(QBrush(crAcc.isEmpty() ? QColor("#DC2626") : QColor("#C2410C")));
        crItem->setFont(QFont("Segoe UI", 9, QFont::Bold));
        m_table->setItem(i, 6, crItem);

        // 7. Amount
        auto* amtItem = new QTableWidgetItem(QString::number(amt, 'f', 2));
        amtItem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
        amtItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        amtItem->setForeground(QBrush(dVal > 0.001 ? QColor("#16A34A") : QColor("#DC2626")));
        amtItem->setFont(QFont("Segoe UI", 9, QFont::Bold));
        m_table->setItem(i, 7, amtItem);

        // 8. Balance
        auto* bItem = new QTableWidgetItem(QString::number(bVal, 'f', 2));
        bItem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
        bItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        bItem->setForeground(QBrush(QColor("#475569")));
        m_table->setItem(i, 8, bItem);

        // 9. Status / Confidence
        QString statusText = "Unmatched";
        QColor statusColor("#DC2626");
        if (isDup) {
            statusText = "DUPLICATE ⚠️";
            statusColor = QColor("#D97706");
        } else if (confidence == "ALIAS_MATCH") {
            statusText = "Learned ⚡";
            statusColor = QColor("#7C3AED");
        } else if (confidence == "HIGH" || confidence == "ACCOUNT_MATCH") {
            statusText = "Matched ✓";
            statusColor = QColor("#16A34A");
        } else if (confidence == "MEDIUM") {
            statusText = "Suggestion ?";
            statusColor = QColor("#2563EB");
        }

        auto* dupItem = new QTableWidgetItem(statusText);
        dupItem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
        dupItem->setTextAlignment(Qt::AlignCenter);
        dupItem->setForeground(QBrush(statusColor));
        dupItem->setFont(QFont("Segoe UI", 9, QFont::Bold));
        m_table->setItem(i, 9, dupItem);
    }

    m_table->blockSignals(false);
}

void BankStatementImportWidget::updateSummary() {
    if (!m_controller) return;

    m_totalCountLabel->setText(QString::number(m_controller->totalCount()));
    m_totalWithdrawalsLabel->setText(m_controller->totalWithdrawalsFmt());
    m_totalDepositsLabel->setText(m_controller->totalDepositsFmt());
    m_duplicateCountLabel->setText(QString::number(m_controller->duplicateCount()));
    m_selectedCountLabel->setText(QString::number(m_controller->selectedCount()));
}

void BankStatementImportWidget::onFilterChanged() {
    if (!m_controller) return;

    QString fType = m_filterTypeCombo->currentText();
    QString query = m_searchEdit->text();

    m_controller->rowsModel()->setFilter(fType, query);
    refreshTable();
    updateSummary();
}

void BankStatementImportWidget::onSelectAllToggled(bool checked) {
    if (m_controller) {
        m_controller->rowsModel()->selectAll(checked);
        refreshTable();
        updateSummary();
    }
}

void BankStatementImportWidget::onDeselectDuplicatesClicked() {
    if (m_controller) {
        m_controller->rowsModel()->deselectDuplicates();
        refreshTable();
        updateSummary();
    }
}

void BankStatementImportWidget::onTableItemChanged(QTableWidgetItem* item) {
    if (!item || !m_controller) return;

    int row = item->row();
    int col = item->column();
    auto* mdl = m_controller->rowsModel();
    if (!mdl || row < 0 || row >= mdl->count()) return;

    if (col == 0) {
        bool checked = (item->checkState() == Qt::Checked);
        mdl->setRowSelected(row, checked);
        updateSummary();
    } else if (col == 5) {
        QString acc = item->text().trimmed();
        mdl->setRowDrAccount(row, acc);
        auto r = mdl->getRow(row);
        QString extracted = r.value("extractedParty").toString();
        if (!extracted.isEmpty() && !acc.isEmpty()) {
            m_controller->saveAlias(extracted, acc);
        }
        updateSummary();
    } else if (col == 6) {
        QString acc = item->text().trimmed();
        mdl->setRowCrAccount(row, acc);
        auto r = mdl->getRow(row);
        QString extracted = r.value("extractedParty").toString();
        if (!extracted.isEmpty() && !acc.isEmpty()) {
            m_controller->saveAlias(extracted, acc);
        }
        updateSummary();
    }
}

void BankStatementImportWidget::onPostSelectedClicked() {
    if (!m_controller || m_controller->selectedCount() == 0) {
        CustomMessageBox::showWarning(this, "No Selection", "Please select at least one transaction to post.");
        return;
    }

    QString msg = QString("Are you sure you want to post %1 selected bank transactions into '%2'?")
        .arg(QString::number(m_controller->selectedCount()), m_bankLedgerCombo->currentText());

    if (!CustomMessageBox::showConfirmation(this, "Confirm Post to Books", msg)) {
        return;
    }

    m_controller->setBankLedger(m_bankLedgerCombo->currentText());
    QVariantMap res = m_controller->postSelectedVouchers();
    int posted = res.value("postedCount", 0).toInt();
    bool ok = res.value("success", false).toBool() || (posted > 0);

    if (ok) {
        CustomMessageBox::showInformation(this, "Success", QString("Successfully posted %1 vouchers into the books.").arg(posted));
        emit importCompleted();
        resetForm();
    } else {
        QString err = res.value("error", m_controller->statusMessage()).toString();
        CustomMessageBox::showCritical(this, "Post Error", "Failed to post transactions: " + err);
    }
}

} // namespace MahadevERP
