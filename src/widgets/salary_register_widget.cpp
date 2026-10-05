#include "salary_register_widget.h"
#include "kbd_badge_button.h"
#include "custom_dialogs.h"
#include "../engine/fiscal_year_helper.h"
#include "../services/financial_math_service.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QHeaderView>
#include <QShortcut>
#include <QFileDialog>
#include <QFile>
#include <QTextStream>
#include <QMessageBox>
#include <QCheckBox>
#include <QDebug>

namespace MahadevERP {

enum SalaryTableCol {
    ColCheck = 0,
    ColSr,
    ColCode,
    ColName,
    ColDesignation,
    ColMasterSalary,
    ColMonthDays,
    ColPresentDays,
    ColEarnedBasic,
    ColAllowances,
    ColPfDeduction,
    ColTdsDeduction,
    ColAdvanceDeduction,
    ColOtherDeduction,
    ColNetPayable,
    ColStatus,
    ColAction,
    ColCount
};

SalaryRegisterWidget::SalaryRegisterWidget(SalaryRegisterController* controller, PrintExportController* printExportCtrl, QWidget* parent)
    : QWidget(parent)
    , m_controller(controller)
    , m_printExportCtrl(printExportCtrl)
{
    setupUi();

    connect(m_controller, &SalaryRegisterController::dataLoaded, this, &SalaryRegisterWidget::populateTable);
    connect(m_controller, &SalaryRegisterController::summaryChanged, this, &SalaryRegisterWidget::onUpdateSummary);
    connect(m_controller, &SalaryRegisterController::postingFinished, this, [this](bool success, const QString& msg) {
        if (success) {
            QMessageBox::information(this, "Salary Posting Successful", msg);
        } else {
            QMessageBox::warning(this, "Salary Posting Error", msg);
        }
    });

    // Shortcuts
    new QShortcut(QKeySequence(Qt::Key_Escape), this, [this]() {
        emit backRequested();
    });
    new QShortcut(QKeySequence(Qt::Key_F8), this, [this]() {
        onPostSalariesClicked();
    });
    new QShortcut(QKeySequence(Qt::Key_F5), this, [this]() {
        loadData();
    });
}

void SalaryRegisterWidget::setupUi() {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(14, 12, 14, 12);
    mainLayout->setSpacing(10);

    // ========================================================================
    // TIER 1: STICKY HEADER CARD
    // ========================================================================
    auto* headerCard = new QFrame(this);
    headerCard->setStyleSheet("QFrame { background-color: #FFFFFF; border: 1px solid #E2E8F0; border-radius: 8px; }");
    auto* headerLayout = new QHBoxLayout(headerCard);
    headerLayout->setContentsMargins(14, 10, 14, 10);

    auto* titleCol = new QVBoxLayout();
    titleCol->setSpacing(2);
    auto* titleLabel = new QLabel("Employee Payroll & Monthly Salary Crediting Register", headerCard);
    titleLabel->setStyleSheet("font-size: 16px; font-weight: 800; color: #0F172A; border: none; background: transparent;");
    auto* subLabel = new QLabel("Batch Monthly Salary Adjustment, Working Days, Deductions, and 1-Click Journal Posting (Salary A/c Dr to Employee Cr).", headerCard);
    subLabel->setStyleSheet("font-size: 11px; color: #64748B; border: none; background: transparent;");
    titleCol->addWidget(titleLabel);
    titleCol->addWidget(subLabel);
    headerLayout->addLayout(titleCol);
    headerLayout->addStretch();

    auto* postBtn = new KbdBadgeButton("Post Selected Salaries", "F8", headerCard);
    postBtn->setPrimaryColor("#16A34A", "#15803D");
    postBtn->setTextColor("#FFFFFF");
    connect(postBtn, &QPushButton::clicked, this, &SalaryRegisterWidget::onPostSalariesClicked);
    headerLayout->addWidget(postBtn);

    auto* syncBtn = new KbdBadgeButton("Sync Master Salaries", "Alt+M", headerCard);
    syncBtn->setPrimaryColor("#2563EB", "#1D4ED8");
    syncBtn->setTextColor("#FFFFFF");
    connect(syncBtn, &QPushButton::clicked, this, &SalaryRegisterWidget::onSyncMasterClicked);
    headerLayout->addWidget(syncBtn);

    auto* csvBtn = new QPushButton("Export CSV", headerCard);
    csvBtn->setCursor(Qt::PointingHandCursor);
    csvBtn->setStyleSheet("QPushButton { background-color: #FFFFFF; border: 1.5px solid #CBD5E1; border-radius: 6px; padding: 6px 12px; font-weight: 800; font-size: 11.5px; color: #334155; } QPushButton:hover { background-color: #F8FAFC; }");
    connect(csvBtn, &QPushButton::clicked, this, &SalaryRegisterWidget::onExportCsv);
    headerLayout->addWidget(csvBtn);

    auto* backBtn = new KbdBadgeButton("Back", "Esc", headerCard);
    backBtn->setPrimaryColor("#F1F5F9", "#E2E8F0");
    backBtn->setTextColor("#475569");
    connect(backBtn, &QPushButton::clicked, this, &SalaryRegisterWidget::backRequested);
    headerLayout->addWidget(backBtn);

    mainLayout->addWidget(headerCard);

    // ========================================================================
    // TIER 2: FILTER & CONTROL CARD
    // ========================================================================
    auto* filterCard = new QFrame(this);
    filterCard->setStyleSheet(
        "QFrame { background-color: #FFFFFF; border: 1px solid #CBD5E1; border-radius: 8px; }"
        "QLabel { color: #475569; font-weight: 700; font-size: 11.5px; border: none; background: transparent; }"
        "QComboBox, QDateEdit, QLineEdit { background: #F8FAFC; border: 1px solid #CBD5E1; border-radius: 4px; padding: 4px 8px; font-size: 12px; font-weight: 600; color: #1E293B; height: 26px; }"
        "QComboBox:focus, QDateEdit:focus, QLineEdit:focus { border: 1.5px solid #3B82F6; background: #FFFFFF; }"
    );
    auto* filterLayout = new QHBoxLayout(filterCard);
    filterLayout->setContentsMargins(12, 8, 12, 8);
    filterLayout->setSpacing(10);

    // Month Selector
    filterLayout->addWidget(new QLabel("Month:", filterCard));
    m_monthCombo = new QComboBox(filterCard);
    m_monthCombo->addItems({
        "April", "May", "June", "July", "August", "September",
        "October", "November", "December", "January", "February", "March"
    });
    // Default to April or current month
    int curM = QDate::currentDate().month();
    int monthIdx = (curM >= 4) ? (curM - 4) : (curM + 8);
    m_monthCombo->setCurrentIndex(monthIdx);
    connect(m_monthCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &SalaryRegisterWidget::onPeriodChanged);
    filterLayout->addWidget(m_monthCombo);

    // Year Selector
    filterLayout->addWidget(new QLabel("Year:", filterCard));
    m_yearCombo = new QComboBox(filterCard);
    int curY = QDate::currentDate().year();
    for (int y = curY - 2; y <= curY + 2; ++y) {
        m_yearCombo->addItem(QString::number(y), y);
    }
    m_yearCombo->setCurrentText(QString::number(curY));
    connect(m_yearCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &SalaryRegisterWidget::onPeriodChanged);
    filterLayout->addWidget(m_yearCombo);

    // Voucher Posting Date
    filterLayout->addWidget(new QLabel("Voucher Date:", filterCard));
    m_voucherDateEdit = new QDateEdit(filterCard);
    m_voucherDateEdit->setCalendarPopup(true);
    m_voucherDateEdit->setDisplayFormat("dd/MM/yyyy");
    filterLayout->addWidget(m_voucherDateEdit);

    // Status Filter
    filterLayout->addWidget(new QLabel("Status:", filterCard));
    m_statusCombo = new QComboBox(filterCard);
    m_statusCombo->addItems({"ALL EMPLOYEES", "PENDING CREDITING ONLY", "ALREADY POSTED", "CONFIGURED SALARY > 0"});
    connect(m_statusCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &SalaryRegisterWidget::onFilterChanged);
    filterLayout->addWidget(m_statusCombo);

    // Search Box
    filterLayout->addWidget(new QLabel("Search:", filterCard));
    m_searchEdit = new QLineEdit(filterCard);
    m_searchEdit->setPlaceholderText("Filter by Name, Code, Phone, Station...");
    m_searchEdit->setClearButtonEnabled(true);
    m_searchEdit->setMinimumWidth(180);
    connect(m_searchEdit, &QLineEdit::textChanged, this, &SalaryRegisterWidget::onSearchChanged);
    filterLayout->addWidget(m_searchEdit);

    filterLayout->addStretch();

    // Quick selection buttons
    auto* btnSelAll = new QPushButton("Select All", filterCard);
    btnSelAll->setStyleSheet("QPushButton { background: #EFF6FF; border: 1px solid #BFDBFE; border-radius: 4px; padding: 4px 8px; font-weight: 700; color: #1E40AF; font-size: 11px; } QPushButton:hover { background: #DBEAFE; }");
    connect(btnSelAll, &QPushButton::clicked, this, &SalaryRegisterWidget::onSelectAllClicked);
    filterLayout->addWidget(btnSelAll);

    auto* btnDeselAll = new QPushButton("Deselect All", filterCard);
    btnDeselAll->setStyleSheet("QPushButton { background: #F8FAFC; border: 1px solid #E2E8F0; border-radius: 4px; padding: 4px 8px; font-weight: 700; color: #475569; font-size: 11px; } QPushButton:hover { background: #F1F5F9; }");
    connect(btnDeselAll, &QPushButton::clicked, this, &SalaryRegisterWidget::onDeselectAllClicked);
    filterLayout->addWidget(btnDeselAll);

    auto* btnSelPending = new QPushButton("Select Pending", filterCard);
    btnSelPending->setStyleSheet("QPushButton { background: #F0FDF4; border: 1px solid #BBF7D0; border-radius: 4px; padding: 4px 8px; font-weight: 700; color: #166534; font-size: 11px; } QPushButton:hover { background: #DCFCE7; }");
    connect(btnSelPending, &QPushButton::clicked, this, &SalaryRegisterWidget::onSelectPendingClicked);
    filterLayout->addWidget(btnSelPending);

    mainLayout->addWidget(filterCard);

    // ========================================================================
    // TIER 3: DATA GRID TABLE
    // ========================================================================
    m_table = new QTableWidget(this);
    m_table->setColumnCount(ColCount);
    m_table->setHorizontalHeaderLabels({
        "Select",
        "Sr.",
        "Code",
        "Employee Name",
        "Designation / Station",
        "Master Salary (INR)",
        "Month Days",
        "Present Days",
        "Earned Basic (INR)",
        "Bonus / Allow. (INR)",
        "PF Deduct (INR)",
        "TDS (INR)",
        "Adv. Deduct (INR)",
        "Other Deduct (INR)",
        "Net Payable (INR)",
        "Status / Voucher",
        "Action"
    });

    m_table->setStyleSheet(
        "QTableWidget { background-color: #FFFFFF; gridline-color: #E2E8F0; border: 1px solid #CBD5E1; border-radius: 8px; font-size: 12px; color: #1E293B; selection-background-color: #EFF6FF; selection-color: #1E293B; }"
        "QHeaderView::section { background-color: #F8FAFC; color: #475569; font-weight: 800; font-size: 11.5px; border: none; border-bottom: 1.5px solid #CBD5E1; border-right: 1px solid #E2E8F0; padding: 6px 4px; }"
        "QTableWidget::item { padding: 4px; border-bottom: 1px solid #F1F5F9; }"
        "QTableWidget::item:focus { background-color: #FEF3C7; color: #92400E; }"
    );
    m_table->verticalHeader()->setVisible(false);
    m_table->verticalHeader()->setDefaultSectionSize(36);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table->setAlternatingRowColors(true);

    // Column widths
    m_table->setColumnWidth(ColCheck, 45);
    m_table->setColumnWidth(ColSr, 45);
    m_table->setColumnWidth(ColCode, 60);
    m_table->setColumnWidth(ColName, 220);
    m_table->setColumnWidth(ColDesignation, 140);
    m_table->setColumnWidth(ColMasterSalary, 110);
    m_table->setColumnWidth(ColMonthDays, 75);
    m_table->setColumnWidth(ColPresentDays, 85);
    m_table->setColumnWidth(ColEarnedBasic, 110);
    m_table->setColumnWidth(ColAllowances, 100);
    m_table->setColumnWidth(ColPfDeduction, 90);
    m_table->setColumnWidth(ColTdsDeduction, 85);
    m_table->setColumnWidth(ColAdvanceDeduction, 95);
    m_table->setColumnWidth(ColOtherDeduction, 95);
    m_table->setColumnWidth(ColNetPayable, 120);
    m_table->setColumnWidth(ColStatus, 130);
    m_table->setColumnWidth(ColAction, 85);

    m_table->horizontalHeader()->setStretchLastSection(false);
    m_table->horizontalHeader()->setSectionResizeMode(ColName, QHeaderView::Stretch);

    connect(m_table, &QTableWidget::cellChanged, this, &SalaryRegisterWidget::onCellChanged);
    connect(m_table, &QTableWidget::cellDoubleClicked, this, &SalaryRegisterWidget::onCellDoubleClicked);

    mainLayout->addWidget(m_table, 1);

    // ========================================================================
    // TIER 4: STATS SUMMARY BAR (Scoped strictly to avoid child borders)
    // ========================================================================
    auto* statsBarFrame = new QFrame(this);
    statsBarFrame->setObjectName("statsBarFrame");
    statsBarFrame->setStyleSheet(
        "QFrame#statsBarFrame { background-color: #FFFFFF; border: 1.5px solid #CBD5E1; border-radius: 8px; min-height: 50px; }"
        "QFrame#statsBarFrame QLabel, QFrame#statsBarFrame QWidget { border: none; background: transparent; }"
    );
    auto* statsLayout = new QHBoxLayout(statsBarFrame);
    statsLayout->setContentsMargins(14, 8, 14, 8);
    statsLayout->setSpacing(20);

    auto makeStatBlock = [statsBarFrame](const QString& title, QLabel*& outValLabel, const QString& valColor = "#0F172A", bool isBoldLarge = false) -> QWidget* {
        auto* block = new QWidget(statsBarFrame);
        auto* l = new QVBoxLayout(block);
        l->setContentsMargins(0, 0, 0, 0);
        l->setSpacing(2);

        auto* titleLbl = new QLabel(title, block);
        titleLbl->setStyleSheet("font-size: 10.5px; font-weight: 700; color: #64748B; text-transform: uppercase; border: none; background: transparent;");

        outValLabel = new QLabel("-", block);
        if (isBoldLarge) {
            outValLabel->setStyleSheet(QString("font-size: 15px; font-weight: 900; color: %1; border: none; background: transparent;").arg(valColor));
        } else {
            outValLabel->setStyleSheet(QString("font-size: 12.5px; font-weight: 800; color: %1; border: none; background: transparent;").arg(valColor));
        }

        l->addWidget(titleLbl);
        l->addWidget(outValLabel);
        return block;
    };

    statsLayout->addWidget(makeStatBlock("Total Employees", m_lblTotalEmployees, "#1E293B"));
    statsLayout->addWidget(makeStatBlock("Selected To Credit", m_lblSelectedCount, "#2563EB"));
    statsLayout->addWidget(makeStatBlock("Total Master Base", m_lblTotalMaster, "#475569"));
    statsLayout->addWidget(makeStatBlock("Total Deductions", m_lblTotalDeductions, "#DC2626"));
    statsLayout->addWidget(makeStatBlock("Total Net Payable", m_lblTotalNetPayable, "#16A34A", true));
    statsLayout->addWidget(makeStatBlock("Already Posted", m_lblTotalPosted, "#059669"));
    statsLayout->addStretch();

    mainLayout->addWidget(statsBarFrame);

    updateVoucherDateForSelectedMonth();
}

int SalaryRegisterWidget::getSelectedMonthNumber() const {
    int idx = m_monthCombo->currentIndex();
    // 0 -> April (4), 1 -> May (5), ..., 8 -> December (12), 9 -> January (1), 10 -> February (2), 11 -> March (3)
    if (idx < 9) return idx + 4;
    return idx - 8;
}

int SalaryRegisterWidget::getSelectedYearNumber() const {
    int yr = m_yearCombo->currentData().toInt();
    if (yr <= 0) yr = m_yearCombo->currentText().toInt();
    int m = getSelectedMonthNumber();
    // For Jan, Feb, Mar in FY starting year yr, actual calendar year is yr or yr+1 depending on how FY is selected
    return yr;
}

void SalaryRegisterWidget::updateVoucherDateForSelectedMonth() {
    int m = getSelectedMonthNumber();
    int y = getSelectedYearNumber();
    QDate d(y, m, 1);
    QDate endOfMonth(y, m, d.daysInMonth());
    m_voucherDateEdit->setDate(endOfMonth);
}

void SalaryRegisterWidget::loadData() {
    m_isDirty = false;
    int m = getSelectedMonthNumber();
    int y = getSelectedYearNumber();
    m_controller->loadPayroll(y, m);
}

void SalaryRegisterWidget::focusTable() {
    if (m_table) m_table->setFocus();
}

void SalaryRegisterWidget::onPeriodChanged() {
    updateVoucherDateForSelectedMonth();
    loadData();
}

void SalaryRegisterWidget::populateTable() {
    m_isUpdatingTable = true;
    m_table->setRowCount(0);

    const auto& items = m_controller->items();
    m_table->setRowCount(items.size());

    for (int r = 0; r < items.size(); ++r) {
        const auto& item = items[r];

        // 0: Checkbox
        auto* chkItem = new QTableWidgetItem();
        chkItem->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled | Qt::ItemIsSelectable);
        chkItem->setCheckState(item.isSelected ? Qt::Checked : Qt::Unchecked);
        chkItem->setTextAlignment(Qt::AlignCenter);
        m_table->setItem(r, ColCheck, chkItem);

        // 1: Sr No
        auto* srItem = new QTableWidgetItem(QString::number(r + 1));
        srItem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
        srItem->setTextAlignment(Qt::AlignCenter);
        m_table->setItem(r, ColSr, srItem);

        // 2: Code
        auto* codeItem = new QTableWidgetItem(item.legacyCode > 0 ? QString::number(item.legacyCode) : QString::number(item.partyId));
        codeItem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
        codeItem->setTextAlignment(Qt::AlignCenter);
        m_table->setItem(r, ColCode, codeItem);

        // 3: Name
        auto* nameItem = new QTableWidgetItem(item.employeeName);
        nameItem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
        QFont nf = nameItem->font();
        nf.setBold(true);
        nameItem->setFont(nf);
        nameItem->setForeground(QBrush(QColor("#0F172A")));
        m_table->setItem(r, ColName, nameItem);

        // 4: Designation / Station
        QString desigCol;
        if (!item.designation.isEmpty() && !item.station.isEmpty()) {
            desigCol = QString("%1 [%2]").arg(item.designation, item.station);
        } else if (!item.designation.isEmpty()) {
            desigCol = item.designation;
        } else if (!item.station.isEmpty()) {
            desigCol = QString("[%1]").arg(item.station);
        } else {
            desigCol = "—";
        }
        auto* desigItem = new QTableWidgetItem(desigCol);
        desigItem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
        desigItem->setForeground(QBrush(QColor("#475569")));
        m_table->setItem(r, ColDesignation, desigItem);

        // 5: Master Salary
        auto* masterItem = new QTableWidgetItem(QString::number(item.masterSalary, 'f', 2));
        masterItem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
        masterItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        m_table->setItem(r, ColMasterSalary, masterItem);

        // 6: Month Days
        auto* mDaysItem = new QTableWidgetItem(QString::number(item.daysInMonth));
        mDaysItem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
        mDaysItem->setTextAlignment(Qt::AlignCenter);
        m_table->setItem(r, ColMonthDays, mDaysItem);

        // 7: Present Days (Editable)
        auto* pDaysItem = new QTableWidgetItem(QString::number(item.presentDays, 'f', 1));
        pDaysItem->setFlags(item.isPosted ? (Qt::ItemIsEnabled | Qt::ItemIsSelectable) : (Qt::ItemIsEditable | Qt::ItemIsEnabled | Qt::ItemIsSelectable));
        pDaysItem->setTextAlignment(Qt::AlignCenter);
        pDaysItem->setBackground(item.isPosted ? QBrush(QColor("#F8FAFC")) : QBrush(QColor("#FFFFFF")));
        m_table->setItem(r, ColPresentDays, pDaysItem);

        // 8: Earned Basic
        auto* earnedItem = new QTableWidgetItem(QString::number(item.earnedBasic, 'f', 2));
        earnedItem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
        earnedItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        m_table->setItem(r, ColEarnedBasic, earnedItem);

        // 9: Bonus / Allowances (Editable)
        auto* allowItem = new QTableWidgetItem(QString::number(item.allowances, 'f', 2));
        allowItem->setFlags(item.isPosted ? (Qt::ItemIsEnabled | Qt::ItemIsSelectable) : (Qt::ItemIsEditable | Qt::ItemIsEnabled | Qt::ItemIsSelectable));
        allowItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        allowItem->setBackground(item.isPosted ? QBrush(QColor("#F8FAFC")) : QBrush(QColor("#FFFFFF")));
        m_table->setItem(r, ColAllowances, allowItem);

        // 10: PF Deduction (Editable)
        auto* pfItem = new QTableWidgetItem(QString::number(item.pfDeduction, 'f', 2));
        pfItem->setFlags(item.isPosted ? (Qt::ItemIsEnabled | Qt::ItemIsSelectable) : (Qt::ItemIsEditable | Qt::ItemIsEnabled | Qt::ItemIsSelectable));
        pfItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        pfItem->setBackground(item.isPosted ? QBrush(QColor("#F8FAFC")) : QBrush(QColor("#FFFFFF")));
        m_table->setItem(r, ColPfDeduction, pfItem);

        // 11: TDS (Editable)
        auto* tdsItem = new QTableWidgetItem(QString::number(item.tdsDeduction, 'f', 2));
        tdsItem->setFlags(item.isPosted ? (Qt::ItemIsEnabled | Qt::ItemIsSelectable) : (Qt::ItemIsEditable | Qt::ItemIsEnabled | Qt::ItemIsSelectable));
        tdsItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        tdsItem->setBackground(item.isPosted ? QBrush(QColor("#F8FAFC")) : QBrush(QColor("#FFFFFF")));
        m_table->setItem(r, ColTdsDeduction, tdsItem);

        // 12: Advance (Editable)
        auto* advItem = new QTableWidgetItem(QString::number(item.advanceDeduction, 'f', 2));
        advItem->setFlags(item.isPosted ? (Qt::ItemIsEnabled | Qt::ItemIsSelectable) : (Qt::ItemIsEditable | Qt::ItemIsEnabled | Qt::ItemIsSelectable));
        advItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        advItem->setBackground(item.isPosted ? QBrush(QColor("#F8FAFC")) : QBrush(QColor("#FFFFFF")));
        m_table->setItem(r, ColAdvanceDeduction, advItem);

        // 13: Other Deductions (Editable)
        auto* otherItem = new QTableWidgetItem(QString::number(item.otherDeduction, 'f', 2));
        otherItem->setFlags(item.isPosted ? (Qt::ItemIsEnabled | Qt::ItemIsSelectable) : (Qt::ItemIsEditable | Qt::ItemIsEnabled | Qt::ItemIsSelectable));
        otherItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        otherItem->setBackground(item.isPosted ? QBrush(QColor("#F8FAFC")) : QBrush(QColor("#FFFFFF")));
        m_table->setItem(r, ColOtherDeduction, otherItem);

        // 14: Net Payable (Bold Green)
        auto* netItem = new QTableWidgetItem(QString::number(item.netPayable, 'f', 2));
        netItem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
        netItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        QFont bf = netItem->font();
        bf.setBold(true);
        netItem->setFont(bf);
        netItem->setForeground(QBrush(QColor("#15803D")));
        m_table->setItem(r, ColNetPayable, netItem);

        // 15: Status / Voucher
        QString statusText = item.isPosted ? QString("POSTED [%1]").arg(item.postedVoucherNo) : "PENDING";
        auto* statItem = new QTableWidgetItem(statusText);
        statItem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
        statItem->setTextAlignment(Qt::AlignCenter);
        statItem->setForeground(item.isPosted ? QBrush(QColor("#059669")) : QBrush(QColor("#D97706")));
        QFont sf = statItem->font();
        sf.setBold(true);
        statItem->setFont(sf);
        m_table->setItem(r, ColStatus, statItem);

        // 16: Action (Statement View button)
        auto* actBtn = new QPushButton("Statement", m_table);
        actBtn->setCursor(Qt::PointingHandCursor);
        actBtn->setStyleSheet("QPushButton { background: #F1F5F9; border: 1px solid #CBD5E1; border-radius: 4px; font-weight: 700; color: #334155; font-size: 10.5px; padding: 2px 4px; } QPushButton:hover { background: #E2E8F0; color: #0F172A; }");
        QString pName = item.employeeName;
        connect(actBtn, &QPushButton::clicked, this, [this, pName]() {
            emit statementRequested(pName);
        });
        m_table->setCellWidget(r, ColAction, actBtn);

        applyRowVisibility(r);
    }

    m_isUpdatingTable = false;
    onUpdateSummary();
}

void SalaryRegisterWidget::onCellChanged(int row, int column) {
    if (m_isUpdatingTable) return;
    const auto& items = m_controller->items();
    if (row < 0 || row >= items.size()) return;

    m_isUpdatingTable = true;

    if (column == ColCheck) {
        auto* it = m_table->item(row, ColCheck);
        bool checked = (it && it->checkState() == Qt::Checked);
        m_controller->setItemSelected(row, checked);
    } else if (column == ColPresentDays) {
        double d = m_table->item(row, ColPresentDays)->text().toDouble();
        m_controller->setItemPresentDays(row, d);
        // Update read-only computed cells
        const auto& item = m_controller->items()[row];
        m_table->item(row, ColEarnedBasic)->setText(QString::number(item.earnedBasic, 'f', 2));
        m_table->item(row, ColNetPayable)->setText(QString::number(item.netPayable, 'f', 2));
    } else if (column == ColAllowances) {
        double a = m_table->item(row, ColAllowances)->text().toDouble();
        m_controller->setItemAllowances(row, a);
        const auto& item = m_controller->items()[row];
        m_table->item(row, ColNetPayable)->setText(QString::number(item.netPayable, 'f', 2));
    } else if (column == ColPfDeduction) {
        double pf = m_table->item(row, ColPfDeduction)->text().toDouble();
        m_controller->setItemPf(row, pf);
        const auto& item = m_controller->items()[row];
        m_table->item(row, ColNetPayable)->setText(QString::number(item.netPayable, 'f', 2));
    } else if (column == ColTdsDeduction) {
        double tds = m_table->item(row, ColTdsDeduction)->text().toDouble();
        m_controller->setItemTds(row, tds);
        const auto& item = m_controller->items()[row];
        m_table->item(row, ColNetPayable)->setText(QString::number(item.netPayable, 'f', 2));
    } else if (column == ColAdvanceDeduction) {
        double adv = m_table->item(row, ColAdvanceDeduction)->text().toDouble();
        m_controller->setItemAdvance(row, adv);
        const auto& item = m_controller->items()[row];
        m_table->item(row, ColNetPayable)->setText(QString::number(item.netPayable, 'f', 2));
    } else if (column == ColOtherDeduction) {
        double oth = m_table->item(row, ColOtherDeduction)->text().toDouble();
        m_controller->setItemOther(row, oth);
        const auto& item = m_controller->items()[row];
        m_table->item(row, ColNetPayable)->setText(QString::number(item.netPayable, 'f', 2));
    }

    m_isUpdatingTable = false;
    onUpdateSummary();
}

void SalaryRegisterWidget::onCellDoubleClicked(int row, int column) {
    Q_UNUSED(column);
    const auto& items = m_controller->items();
    if (row >= 0 && row < items.size()) {
        emit statementRequested(items[row].employeeName);
    }
}

void SalaryRegisterWidget::applyRowVisibility(int row) {
    const auto& items = m_controller->items();
    if (row < 0 || row >= items.size()) return;
    const auto& item = items[row];

    QString query = m_searchEdit->text().trimmed().toLower();
    int statusFilter = m_statusCombo->currentIndex();

    bool matchSearch = true;
    if (!query.isEmpty()) {
        QString fullTxt = QString("%1 %2 %3 %4 %5")
                              .arg(item.employeeName)
                              .arg(item.legacyCode)
                              .arg(item.designation)
                              .arg(item.station)
                              .arg(item.phone)
                              .toLower();
        matchSearch = fullTxt.contains(query);
    }

    bool matchStatus = true;
    if (statusFilter == 1) { // PENDING CREDITING ONLY
        matchStatus = !item.isPosted && (item.netPayable > 0.0);
    } else if (statusFilter == 2) { // ALREADY POSTED
        matchStatus = item.isPosted;
    } else if (statusFilter == 3) { // CONFIGURED SALARY > 0
        matchStatus = (item.masterSalary > 0.0);
    }

    m_table->setRowHidden(row, !(matchSearch && matchStatus));
}

void SalaryRegisterWidget::onSearchChanged(const QString& text) {
    Q_UNUSED(text);
    for (int r = 0; r < m_table->rowCount(); ++r) {
        applyRowVisibility(r);
    }
}

void SalaryRegisterWidget::onFilterChanged() {
    for (int r = 0; r < m_table->rowCount(); ++r) {
        applyRowVisibility(r);
    }
}

void SalaryRegisterWidget::onSelectAllClicked() {
    m_isUpdatingTable = true;
    m_controller->selectAll(true);
    for (int r = 0; r < m_table->rowCount(); ++r) {
        if (m_table->item(r, ColCheck)) {
            const auto& item = m_controller->items()[r];
            m_table->item(r, ColCheck)->setCheckState(item.isSelected ? Qt::Checked : Qt::Unchecked);
        }
    }
    m_isUpdatingTable = false;
    onUpdateSummary();
}

void SalaryRegisterWidget::onDeselectAllClicked() {
    m_isUpdatingTable = true;
    m_controller->selectAll(false);
    for (int r = 0; r < m_table->rowCount(); ++r) {
        if (m_table->item(r, ColCheck)) {
            m_table->item(r, ColCheck)->setCheckState(Qt::Unchecked);
        }
    }
    m_isUpdatingTable = false;
    onUpdateSummary();
}

void SalaryRegisterWidget::onSelectPendingClicked() {
    m_isUpdatingTable = true;
    m_controller->selectPendingOnly();
    for (int r = 0; r < m_table->rowCount(); ++r) {
        if (m_table->item(r, ColCheck)) {
            const auto& item = m_controller->items()[r];
            m_table->item(r, ColCheck)->setCheckState(item.isSelected ? Qt::Checked : Qt::Unchecked);
        }
    }
    m_isUpdatingTable = false;
    onUpdateSummary();
}

void SalaryRegisterWidget::onPostSalariesClicked() {
    SalaryRegisterSummary s = m_controller->summary();
    if (s.selectedEmployees <= 0 || s.totalNetPayable <= 0.0) {
        QMessageBox::warning(this, "No Employees Selected", "Please select at least one employee with positive net payable salary to post.");
        return;
    }

    QDate vDate = m_voucherDateEdit->date();
    QString mLabel = m_controller->currentMonthLabel();

    QString confirmMsg = QString(
        "Are you sure you want to post salary journal entries for:\n\n"
        "• Selected Employees: %1\n"
        "• Total Amount to Post: INR %2\n"
        "• Payroll Month: %3\n"
        "• Voucher Date: %4\n\n"
        "This will generate double-entry Journal Vouchers (Salary A/c Dr to Employee A/c Cr) for all selected staff."
    ).arg(s.selectedEmployees)
     .arg(s.totalNetPayable, 0, 'f', 2)
     .arg(mLabel)
     .arg(vDate.toString("dd/MM/yyyy"));

    auto reply = QMessageBox::question(this, "Confirm Bulk Salary Posting", confirmMsg, QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes);
    if (reply != QMessageBox::Yes) {
        return;
    }

    m_controller->postSelectedSalaries(vDate);
}

void SalaryRegisterWidget::onSyncMasterClicked() {
    int updated = m_controller->syncMasterSalariesFromBahiKhata();
    if (updated > 0) {
        QMessageBox::information(this, "Sync Complete", QString("Successfully imported and updated monthly salaries for %1 employees from Bahi-Khata.").arg(updated));
    } else {
        QMessageBox::information(this, "Sync Result", "No new employee master salary records found to sync, or all records are up to date.");
    }
}

void SalaryRegisterWidget::onExportCsv() {
    QString fileName = QFileDialog::getSaveFileName(this, "Export Payroll Register", QString("Payroll_%1_%2.csv").arg(m_monthCombo->currentText()).arg(m_yearCombo->currentText()), "CSV Files (*.csv)");
    if (fileName.isEmpty()) return;

    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(this, "Export Failed", "Could not create CSV file.");
        return;
    }

    QTextStream out(&file);
    out << "Sr,Code,Employee Name,Designation,Master Salary,Month Days,Present Days,Earned Basic,Allowances,PF Deduct,TDS Deduct,Advance Deduct,Other Deduct,Net Payable,Status,Voucher No\n";

    const auto& items = m_controller->items();
    for (int i = 0; i < items.size(); ++i) {
        const auto& item = items[i];
        out << (i + 1) << ","
            << (item.legacyCode > 0 ? item.legacyCode : item.partyId) << ","
            << "\"" << item.employeeName << "\","
            << "\"" << item.designation << "\","
            << item.masterSalary << ","
            << item.daysInMonth << ","
            << item.presentDays << ","
            << item.earnedBasic << ","
            << item.allowances << ","
            << item.pfDeduction << ","
            << item.tdsDeduction << ","
            << item.advanceDeduction << ","
            << item.otherDeduction << ","
            << item.netPayable << ","
            << (item.isPosted ? "POSTED" : "PENDING") << ","
            << "\"" << item.postedVoucherNo << "\"\n";
    }

    file.close();
    QMessageBox::information(this, "Export Successful", "Payroll register successfully exported to CSV.");
}

void SalaryRegisterWidget::onExportPdf() {
    onExportCsv();
}

void SalaryRegisterWidget::onUpdateSummary() {
    SalaryRegisterSummary s = m_controller->summary();

    m_lblTotalEmployees->setText(QString("%1 (%2 Pending)").arg(s.totalEmployees).arg(s.pendingEmployees));
    m_lblSelectedCount->setText(QString("%1 Staff").arg(s.selectedEmployees));
    m_lblTotalMaster->setText(FinancialMathService::instance().formatInr(s.totalMasterSalary));
    double totDed = s.totalPf + s.totalTds + s.totalAdvances + s.totalOther;
    m_lblTotalDeductions->setText(FinancialMathService::instance().formatInr(totDed));
    m_lblTotalNetPayable->setText(FinancialMathService::instance().formatInr(s.totalNetPayable));
    m_lblTotalPosted->setText(QString("%1 (%2 Posted)").arg(FinancialMathService::instance().formatInr(s.totalPostedAmount)).arg(s.postedEmployees));
}

} // namespace MahadevERP
