#include "bardana_widget.h"
#include "voucher_date_dialog.h"
#include "kbd_badge_button.h"
#include "custom_dialogs.h"
#include "../engine/accounting_engine.h"
#include "../engine/fiscal_year_helper.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QHeaderView>
#include <QMessageBox>
#include <QKeyEvent>
#include <QFileDialog>
#include <QFile>
#include <QTextStream>
#include <cmath>

BardanaWidget::BardanaWidget(BardanaController* controller, PrintExportController* printCtrl, QWidget* parent)
    : QWidget(parent)
    , m_controller(controller)
    , m_printCtrl(printCtrl)
{
    setupUi();
    applyCustomStyles();

    FiscalYearInfo activeFy = FiscalYearHelper::getActiveFiscalYear();
    QDate sDate = QDate::fromString(activeFy.startDate, "yyyy-MM-dd");
    QDate eDate = QDate::fromString(activeFy.endDate, "yyyy-MM-dd");
    if (!sDate.isValid()) sDate = QDate(QDate::currentDate().month() < 4 ? QDate::currentDate().year() - 1 : QDate::currentDate().year(), 4, 1);
    if (!eDate.isValid()) eDate = QDate::currentDate();

    m_filterFromDate->blockSignals(true);
    m_filterToDate->blockSignals(true);
    m_filterFromDate->setDate(sDate);
    m_filterToDate->setDate(eDate);
    m_filterFromDate->blockSignals(false);
    m_filterToDate->blockSignals(false);

    connect(m_controller, &BardanaController::bardanaDataChanged, this, &BardanaWidget::refreshData);
    refreshData();
}

void BardanaWidget::applyCustomStyles() {
    setAttribute(Qt::WA_StyledBackground, true);
    setStyleSheet(
        "BardanaWidget { background-color: #F8FAFC; }"
        "QFrame.kpiCard { background-color: #FFFFFF; border: 1px solid #E2E8F0; border-radius: 8px; }"
        "QFrame.entryCard { background-color: #FFFFFF; border: 1.5px solid #CBD5E1; border-radius: 8px; }"
        "QLabel { color: #0F172A; font-size: 12px; border: none; background: transparent; }"
        "QLabel.sectionHeader { font-size: 13px; font-weight: 800; color: #0F172A; border: none; background: transparent; }"
        "QLabel.fieldLabel { font-size: 11px; font-weight: 700; color: #475569; border: none; background: transparent; }"
        "QLineEdit, QComboBox, AccountingDateEdit, QDateEdit { background-color: #FFFFFF; border: 1.5px solid #CBD5E1; border-radius: 6px; padding: 5px 8px; font-size: 12px; color: #0F172A; font-weight: 600; }"
        "QLineEdit:focus, QComboBox:focus, AccountingDateEdit:focus, QDateEdit:focus { border-color: #2563EB; background-color: #F8FAFC; }"
        "QTableWidget { background-color: #FFFFFF; alternate-background-color: #F8FAFC; border: 1px solid #CBD5E1; border-radius: 8px; gridline-color: #F1F5F9; font-size: 12.5px; color: #0F172A; selection-background-color: #EFF6FF; selection-color: #1D4ED8; }"
        "QTableWidget::item { padding: 4px 8px; }"
        "QHeaderView::section { background-color: #F1F5F9; color: #1E293B; font-weight: 800; font-size: 12px; padding: 8px 10px; border: none; border-bottom: 2px solid #CBD5E1; border-right: 1px solid #E2E8F0; }"
        "QTabWidget::pane { border: 1px solid #CBD5E1; background: #FFFFFF; border-radius: 8px; top: -1px; }"
        "QTabBar::tab { background: #F1F5F9; color: #475569; padding: 8px 18px; font-weight: 700; font-size: 12px; border-top-left-radius: 6px; border-top-right-radius: 6px; margin-right: 4px; border: 1px solid #CBD5E1; border-bottom: none; }"
        "QTabBar::tab:selected { background: #2563EB; color: #FFFFFF; border-color: #2563EB; }"
    );
}

void BardanaWidget::setupUi() {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(16, 14, 16, 14);
    mainLayout->setSpacing(12);

    // ========================================================================
    // 1. TOP HEADER BAR
    // ========================================================================
    auto* headerLayout = new QHBoxLayout();
    auto* titleCol = new QVBoxLayout();
    titleCol->setSpacing(2);

    auto* titleLabel = new QLabel("Bardana (Gunny Bag) Packaging Ledger", this);
    titleLabel->setStyleSheet("font-size: 19px; font-weight: 800; color: #0F172A;");
    auto* subLabel = new QLabel("Party & Godown packaging inventory, inward/outward bag accounting, and physical stock reconciliation.", this);
    subLabel->setStyleSheet("font-size: 11.5px; color: #64748B; font-weight: 500;");
    titleCol->addWidget(titleLabel);
    titleCol->addWidget(subLabel);
    headerLayout->addLayout(titleCol);
    headerLayout->addStretch();

    auto* btnNewEntry = new KbdBadgeButton("+ New Bag Entry", "F2", this);
    btnNewEntry->setPrimaryColor("#16A34A", "#15803D");
    btnNewEntry->setTextColor("#FFFFFF");
    connect(btnNewEntry, &QPushButton::clicked, this, &BardanaWidget::toggleEntryForm);
    headerLayout->addWidget(btnNewEntry);

    auto* btnCsv = new QPushButton("Export CSV", this);
    btnCsv->setStyleSheet("background-color: #FFFFFF; border: 1px solid #CBD5E1; border-radius: 6px; padding: 6px 14px; font-weight: 700; color: #334155; font-size: 12px;");
    connect(btnCsv, &QPushButton::clicked, this, &BardanaWidget::onExportCsv);
    headerLayout->addWidget(btnCsv);

    auto* btnExportPdf = new QPushButton("Export PDF", this);
    btnExportPdf->setStyleSheet("background-color: #2563EB; border: none; border-radius: 6px; padding: 6px 14px; font-weight: 700; color: #FFFFFF; font-size: 12px;");
    connect(btnExportPdf, &QPushButton::clicked, this, &BardanaWidget::onExportPdf);
    headerLayout->addWidget(btnExportPdf);

    auto* btnBack = new KbdBadgeButton("← Back to Menu", "Esc", this);
    btnBack->setPrimaryColor("#F1F5F9", "#E2E8F0");
    btnBack->setTextColor("#475569");
    connect(btnBack, &QPushButton::clicked, this, &BardanaWidget::backRequested);
    headerLayout->addWidget(btnBack);

    mainLayout->addLayout(headerLayout);

    // ========================================================================
    // 2. KPI SUMMARY METRIC CARDS
    // ========================================================================
    auto* kpiLayout = new QHBoxLayout();
    kpiLayout->setSpacing(10);

    auto createKpiCard = [this](const QString& title, const QString& initialVal, const QString& accentColor, QLabel*& valLabelRef) -> QFrame* {
        auto* card = new QFrame(this);
        card->setProperty("class", "kpiCard");
        auto* cLay = new QVBoxLayout(card);
        cLay->setContentsMargins(12, 10, 12, 10);
        cLay->setSpacing(4);

        auto* tLbl = new QLabel(title, card);
        tLbl->setStyleSheet("font-size: 11px; font-weight: 700; color: #64748B; text-transform: uppercase;");
        valLabelRef = new QLabel(initialVal, card);
        valLabelRef->setStyleSheet(QString("font-size: 18px; font-weight: 800; color: %1;").arg(accentColor));

        cLay->addWidget(tLbl);
        cLay->addWidget(valLabelRef);
        return card;
    };

    kpiLayout->addWidget(createKpiCard("Total Issued Bags (Dr)", "0 Bags", "#2563EB", m_lblTotalIssued));
    kpiLayout->addWidget(createKpiCard("Total Received Bags (Cr)", "0 Bags", "#059669", m_lblTotalReceived));
    kpiLayout->addWidget(createKpiCard("Net Party Balance", "0 Bags", "#D97706", m_lblNetPartyBalance));
    kpiLayout->addWidget(createKpiCard("Godown Physical Stock", "0 Bags", "#7C3AED", m_lblGodownStock));
    mainLayout->addLayout(kpiLayout);

    // ========================================================================
    // 3. COLLAPSIBLE NEW TRANSACTION FORM CARD
    // ========================================================================
    m_entryCard = new QFrame(this);
    m_entryCard->setProperty("class", "entryCard");
    auto* formLayout = new QVBoxLayout(m_entryCard);
    formLayout->setContentsMargins(14, 12, 14, 12);
    formLayout->setSpacing(8);

    auto* fHeader = new QHBoxLayout();
    auto* fTitle = new QLabel("Record New Bardana Packaging Voucher", m_entryCard);
    fTitle->setStyleSheet("font-size: 13px; font-weight: 800; color: #0F172A;");
    fHeader->addWidget(fTitle);
    fHeader->addStretch();
    auto* fCloseBtn = new QPushButton("✕ Close Panel", m_entryCard);
    fCloseBtn->setStyleSheet("border: none; background: transparent; color: #64748B; font-weight: 700; font-size: 11px;");
    connect(fCloseBtn, &QPushButton::clicked, this, &BardanaWidget::toggleEntryForm);
    fHeader->addWidget(fCloseBtn);
    formLayout->addLayout(fHeader);

    auto* grid = new QGridLayout();
    grid->setHorizontalSpacing(10);
    grid->setVerticalSpacing(6);

    // Row 0
    m_vchTypeCombo = new QComboBox(m_entryCard);
    m_vchTypeCombo->addItems({"ISSUE", "RECEIVE", "SALE", "PURCHASE", "OPENING", "TRANSFER"});

    m_dateEdit = new AccountingDateEdit(QDate::currentDate(), m_entryCard);
    m_dateEdit->setCalendarPopup(true);
    m_dateEdit->setDisplayFormat("dd-MM-yyyy");

    m_vchNoEdit = new QLineEdit(m_entryCard);
    m_vchNoEdit->setPlaceholderText("Auto or Slip No");

    m_partySearchBox = new AccountSearchBox(m_entryCard);
    m_partySearchBox->setPlaceholderText("Search Party / Farmer / Merchant...");

    auto addField = [](QGridLayout* g, const QString& label, QWidget* widget, int row, int col) {
        auto* lay = new QVBoxLayout();
        lay->setSpacing(2);
        auto* l = new QLabel(label);
        l->setProperty("class", "fieldLabel");
        lay->addWidget(l);
        lay->addWidget(widget);
        g->addLayout(lay, row, col);
    };

    addField(grid, "Voucher Type:", m_vchTypeCombo, 0, 0);
    addField(grid, "Date:", m_dateEdit, 0, 1);
    addField(grid, "Voucher No:", m_vchNoEdit, 0, 2);
    addField(grid, "Party / Farmer Name:", m_partySearchBox, 0, 3);

    // Row 1
    m_bardanaItemEdit = new ItemSearchEditor(m_entryCard);
    m_bardanaItemEdit->setPlaceholderText("Search Bardana / Bag Item...");
    m_bardanaItemEdit->setText("Jute 50kg (Pukka)");

    m_godownCombo = new QComboBox(m_entryCard);
    m_godownCombo->addItems({"Main Godown", "Godown No. 1", "Godown No. 2", "Mill Yard", "Shed A"});

    m_drCrCombo = new QComboBox(m_entryCard);
    m_drCrCombo->addItems({"Dr (Issue / Party Owes)", "Cr (Receive / Party Returned)"});

    m_qtyEdit = new QLineEdit(m_entryCard);
    m_qtyEdit->setPlaceholderText("e.g. 500");

    addField(grid, "Bardana Type / Item:", m_bardanaItemEdit, 1, 0);
    addField(grid, "Godown Location:", m_godownCombo, 1, 1);
    addField(grid, "Dr / Cr Direction:", m_drCrCombo, 1, 2);
    addField(grid, "Bags Quantity:", m_qtyEdit, 1, 3);

    // Row 2
    m_rateEdit = new QLineEdit(m_entryCard);
    m_rateEdit->setPlaceholderText("₹ Rate / Bag");

    m_amountEdit = new QLineEdit(m_entryCard);
    m_amountEdit->setReadOnly(true);
    m_amountEdit->setPlaceholderText("₹ 0.00");
    m_amountEdit->setStyleSheet("background-color: #F8FAFC; font-weight: 700; color: #0F172A;");

    m_vehicleEdit = new QLineEdit(m_entryCard);
    m_vehicleEdit->setPlaceholderText("e.g. HR-57-A-1234");

    m_billNoEdit = new QLineEdit(m_entryCard);
    m_billNoEdit->setPlaceholderText("Ref Bill / Challan No");

    connect(m_qtyEdit, &QLineEdit::textChanged, this, [this]() {
        double q = m_qtyEdit->text().toDouble();
        double r = m_rateEdit->text().toDouble();
        m_amountEdit->setText(QString("₹%1").arg(QString::number(q * r, 'f', 2)));
    });
    connect(m_rateEdit, &QLineEdit::textChanged, this, [this]() {
        double q = m_qtyEdit->text().toDouble();
        double r = m_rateEdit->text().toDouble();
        m_amountEdit->setText(QString("₹%1").arg(QString::number(q * r, 'f', 2)));
    });

    addField(grid, "Rate per Bag (₹):", m_rateEdit, 2, 0);
    addField(grid, "Total Amount (₹):", m_amountEdit, 2, 1);
    addField(grid, "Vehicle / Lorry No:", m_vehicleEdit, 2, 2);
    addField(grid, "Challan / Bill Ref:", m_billNoEdit, 2, 3);

    // Row 3
    m_narrationEdit = new QLineEdit(m_entryCard);
    m_narrationEdit->setPlaceholderText("Remarks / Narration / Purpose of issue");

    m_btnSave = new QPushButton("Save Bag Voucher (F12)", m_entryCard);
    m_btnSave->setStyleSheet("background-color: #16A34A; color: #FFFFFF; font-weight: 800; font-size: 12px; padding: 7px 16px; border-radius: 6px;");
    connect(m_btnSave, &QPushButton::clicked, this, &BardanaWidget::onSaveTransaction);

    auto* r3Lay = new QHBoxLayout();
    r3Lay->setSpacing(10);
    auto* nBox = new QVBoxLayout();
    nBox->setSpacing(2);
    auto* nLbl = new QLabel("Remarks / Narration:");
    nLbl->setProperty("class", "fieldLabel");
    nBox->addWidget(nLbl);
    nBox->addWidget(m_narrationEdit);
    r3Lay->addLayout(nBox, 3);

    auto* sBox = new QVBoxLayout();
    sBox->setSpacing(2);
    sBox->addWidget(new QLabel(" "));
    sBox->addWidget(m_btnSave);
    r3Lay->addLayout(sBox, 1);

    formLayout->addLayout(grid);
    formLayout->addLayout(r3Lay);

    m_entryCard->setVisible(false); // Initially hidden, toggled by F2
    mainLayout->addWidget(m_entryCard);

    // ========================================================================
    // 4. MAIN TAB WIDGET
    // ========================================================================
    m_tabWidget = new QTabWidget(this);
    connect(m_tabWidget, &QTabWidget::currentChanged, this, &BardanaWidget::onTabChanged);

    // ------------------------------------------------------------------------
    // TAB 1: TRANSACTIONS REGISTER
    // ------------------------------------------------------------------------
    auto* tab1 = new QWidget(this);
    auto* tab1Layout = new QVBoxLayout(tab1);
    tab1Layout->setContentsMargins(8, 10, 8, 8);
    tab1Layout->setSpacing(8);

    // Filter Bar Card
    auto* filterCard = new QFrame(tab1);
    filterCard->setStyleSheet(
        "QFrame { background-color: #FFFFFF; border: 1px solid #CBD5E1; border-radius: 8px; }"
        "QLabel { color: #475569; font-weight: 700; font-size: 11.5px; border: none; background: transparent; }"
        "AccountingDateEdit, QDateEdit, QLineEdit, QComboBox { background-color: #FFFFFF; border: 1.5px solid #CBD5E1; border-radius: 6px; padding: 4px 8px; font-size: 12px; color: #0F172A; font-weight: 600; }"
        "AccountingDateEdit:focus, QDateEdit:focus, QLineEdit:focus, QComboBox:focus { border-color: #2563EB; background-color: #F8FAFC; }"
    );
    auto* filterLayout = new QHBoxLayout(filterCard);
    filterLayout->setContentsMargins(12, 8, 12, 8);
    filterLayout->setSpacing(12);

    filterLayout->addWidget(new QLabel("From Date:", filterCard));
    m_filterFromDate = new AccountingDateEdit(QDate::currentDate().addDays(-30), filterCard);
    m_filterFromDate->setCalendarPopup(true);
    m_filterFromDate->setDisplayFormat("dd-MM-yyyy");
    filterLayout->addWidget(m_filterFromDate);

    filterLayout->addWidget(new QLabel("To Date:", filterCard));
    m_filterToDate = new AccountingDateEdit(QDate::currentDate(), filterCard);
    m_filterToDate->setCalendarPopup(true);
    m_filterToDate->setDisplayFormat("dd-MM-yyyy");
    filterLayout->addWidget(m_filterToDate);

    auto* periodBtn = new QPushButton("Period (F2)", filterCard);
    periodBtn->setFixedHeight(32);
    periodBtn->setCursor(Qt::PointingHandCursor);
    periodBtn->setStyleSheet("QPushButton { background-color: #F1F5F9; color: #0F172A; border: 1.5px solid #CBD5E1; border-radius: 6px; padding: 0 10px; font-weight: 700; font-size: 11.5px; } QPushButton:hover { background-color: #E2E8F0; }");
    connect(periodBtn, &QPushButton::clicked, this, [this]() {
        QDate f = m_filterFromDate->date();
        QDate t = m_filterToDate->date();
        if (VoucherDateDialog::selectDateRange(this, &f, &t, f, t)) {
            m_filterFromDate->setDate(f);
            m_filterToDate->setDate(t);
            onFilterChanged();
        }
    });
    filterLayout->addWidget(periodBtn);

    filterLayout->addWidget(new QLabel("Type:", filterCard));
    m_filterTypeCombo = new QComboBox(filterCard);
    m_filterTypeCombo->addItems({"ALL", "ISSUE", "RECEIVE", "SALE", "PURCHASE", "OPENING", "TRANSFER"});
    filterLayout->addWidget(m_filterTypeCombo);

    filterLayout->addWidget(new QLabel("Search:", filterCard));
    m_searchGeneralEdit = new QLineEdit(filterCard);
    m_searchGeneralEdit->setPlaceholderText("Search party, vehicle, slip no...");
    m_searchGeneralEdit->setClearButtonEnabled(true);
    connect(m_searchGeneralEdit, &QLineEdit::textChanged, this, &BardanaWidget::onFilterChanged);
    filterLayout->addWidget(m_searchGeneralEdit, 1);

    connect(m_filterFromDate, &AccountingDateEdit::dateChanged, this, &BardanaWidget::onFilterChanged);
    connect(m_filterToDate, &AccountingDateEdit::dateChanged, this, &BardanaWidget::onFilterChanged);
    connect(m_filterTypeCombo, &QComboBox::currentTextChanged, this, &BardanaWidget::onFilterChanged);

    auto* btnDelete = new QPushButton("Delete Selected", filterCard);
    btnDelete->setStyleSheet("background-color: #FEE2E2; color: #DC2626; border: 1px solid #FECACA; border-radius: 4px; padding: 4px 10px; font-weight: 700; font-size: 11px;");
    connect(btnDelete, &QPushButton::clicked, this, &BardanaWidget::onDeleteSelected);
    filterLayout->addWidget(btnDelete);

    tab1Layout->addWidget(filterCard);

    // Register Table
    m_registerTable = new QTableWidget(tab1);
    m_registerTable->setColumnCount(13);
    m_registerTable->setHorizontalHeaderLabels({
        "ID", "Date", "Vch No", "Type", "Party / Consignee", "Bardana Type",
        "Godown", "Dr/Cr", "Qty (Bags)", "Rate (₹)", "Amount (₹)", "Vehicle No", "Narration"
    });
    m_registerTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_registerTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_registerTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    m_registerTable->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    m_registerTable->horizontalHeader()->setSectionResizeMode(4, QHeaderView::Stretch);
    for (int c = 5; c < 13; ++c) {
        m_registerTable->horizontalHeader()->setSectionResizeMode(c, QHeaderView::ResizeToContents);
    }
    m_registerTable->verticalHeader()->setVisible(false);
    m_registerTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_registerTable->setSelectionMode(QAbstractItemView::SingleSelection);
    m_registerTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_registerTable->setAlternatingRowColors(true);
    tab1Layout->addWidget(m_registerTable);

    m_tabWidget->addTab(tab1, "Bardana Transactions Register");

    // ------------------------------------------------------------------------
    // TAB 2: PARTY-WISE BAG BALANCES
    // ------------------------------------------------------------------------
    auto* tab2 = new QWidget(this);
    auto* tab2Layout = new QVBoxLayout(tab2);
    tab2Layout->setContentsMargins(8, 10, 8, 8);
    tab2Layout->setSpacing(8);

    auto* partyFilterCard = new QFrame(tab2);
    partyFilterCard->setStyleSheet(
        "QFrame { background-color: #FFFFFF; border: 1px solid #CBD5E1; border-radius: 8px; }"
        "QLabel { color: #475569; font-weight: 700; font-size: 11.5px; border: none; background: transparent; }"
        "QLineEdit { background-color: #FFFFFF; border: 1.5px solid #CBD5E1; border-radius: 6px; padding: 4px 8px; font-size: 12px; color: #0F172A; font-weight: 600; }"
        "QLineEdit:focus { border-color: #2563EB; background-color: #F8FAFC; }"
    );
    auto* pFLay = new QHBoxLayout(partyFilterCard);
    pFLay->setContentsMargins(12, 8, 12, 8);
    pFLay->setSpacing(12);
    pFLay->addWidget(new QLabel("Filter Party:", partyFilterCard));
    m_searchPartyBox = new AccountSearchBox(partyFilterCard);
    m_searchPartyBox->setPlaceholderText("Search Party to filter balances...");
    connect(m_searchPartyBox, &AccountSearchBox::partySelected, this, [this](const QString& /*pName*/) {
        populatePartySummary();
    });
    connect(m_searchPartyBox, &QLineEdit::textChanged, this, &BardanaWidget::populatePartySummary);
    pFLay->addWidget(m_searchPartyBox, 1);
    tab2Layout->addWidget(partyFilterCard);

    m_partyTable = new QTableWidget(tab2);
    m_partyTable->setColumnCount(6);
    m_partyTable->setHorizontalHeaderLabels({
        "Party / Farmer Name", "Bardana Type", "Total Issued (Dr)", "Total Returned (Cr)", "Net Balance Bags", "Status / Settlement"
    });
    m_partyTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_partyTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_partyTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    m_partyTable->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    m_partyTable->horizontalHeader()->setSectionResizeMode(4, QHeaderView::ResizeToContents);
    m_partyTable->horizontalHeader()->setSectionResizeMode(5, QHeaderView::ResizeToContents);
    m_partyTable->verticalHeader()->setVisible(false);
    m_partyTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_partyTable->setSelectionMode(QAbstractItemView::SingleSelection);
    m_partyTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_partyTable->setAlternatingRowColors(true);
    tab2Layout->addWidget(m_partyTable);

    m_tabWidget->addTab(tab2, "Party-Wise Bag Balances");

    // ------------------------------------------------------------------------
    // TAB 3: GODOWN PHYSICAL STOCK
    // ------------------------------------------------------------------------
    auto* tab3 = new QWidget(this);
    auto* tab3Layout = new QVBoxLayout(tab3);
    tab3Layout->setContentsMargins(8, 10, 8, 8);
    tab3Layout->setSpacing(8);

    m_godownTable = new QTableWidget(tab3);
    m_godownTable->setColumnCount(6);
    m_godownTable->setHorizontalHeaderLabels({
        "Godown Location", "Bardana Type", "Opening Bags", "Inward Receipts", "Outward Issues", "Current Physical Stock"
    });
    m_godownTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_godownTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_godownTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    m_godownTable->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    m_godownTable->horizontalHeader()->setSectionResizeMode(4, QHeaderView::ResizeToContents);
    m_godownTable->horizontalHeader()->setSectionResizeMode(5, QHeaderView::ResizeToContents);
    m_godownTable->verticalHeader()->setVisible(false);
    m_godownTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_godownTable->setSelectionMode(QAbstractItemView::SingleSelection);
    m_godownTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_godownTable->setAlternatingRowColors(true);
    tab3Layout->addWidget(m_godownTable);

    m_tabWidget->addTab(tab3, "Godown Physical Stock");

    mainLayout->addWidget(m_tabWidget, 1);
}

void BardanaWidget::toggleEntryForm() {
    if (m_entryCard) {
        bool nowVisible = !m_entryCard->isVisible();
        m_entryCard->setVisible(nowVisible);
        if (nowVisible) {
            m_partySearchBox->setFocus();
        }
    }
}

void BardanaWidget::keyPressEvent(QKeyEvent* event) {
    if (event->key() == Qt::Key_Escape) {
        if (m_entryCard && m_entryCard->isVisible()) {
            m_entryCard->setVisible(false);
            return;
        }
        emit backRequested();
    } else if (event->key() == Qt::Key_F2) {
        toggleEntryForm();
    } else if (event->key() == Qt::Key_F12) {
        if (m_entryCard && m_entryCard->isVisible()) {
            onSaveTransaction();
        }
    } else {
        QWidget::keyPressEvent(event);
    }
}

void BardanaWidget::refreshData() {
    populateRegisterTable();
    populateGodownSummary();
    populatePartySummary();
    updateSummaryKpis();
    m_isDirty = false;
}

void BardanaWidget::updateSummaryKpis() {
    QVariantList trans = m_controller->getBardanaRegister();
    long long totalIssued = 0;
    long long totalReceived = 0;

    for (const auto& rowVar : trans) {
        QVariantMap row = rowVar.toMap();
        QString drCr = row.value("dr_cr").toString();
        long long qty = row.value("qty").toLongLong();
        if (drCr == "Dr") {
            totalIssued += qty;
        } else {
            totalReceived += qty;
        }
    }

    long long netParty = totalIssued - totalReceived;
    long long godownStock = totalReceived - totalIssued; // If mill acts as holder
    if (godownStock < 0) godownStock = 0;

    if (m_lblTotalIssued) m_lblTotalIssued->setText(QString("%1 Bags").arg(QLocale(QLocale::English).toString(totalIssued)));
    if (m_lblTotalReceived) m_lblTotalReceived->setText(QString("%1 Bags").arg(QLocale(QLocale::English).toString(totalReceived)));
    if (m_lblNetPartyBalance) m_lblNetPartyBalance->setText(QString("%1 Bags").arg(QLocale(QLocale::English).toString(netParty)));
    if (m_lblGodownStock) m_lblGodownStock->setText(QString("%1 Bags").arg(QLocale(QLocale::English).toString(godownStock)));
}

void BardanaWidget::populateRegisterTable() {
    QString fromDate = m_filterFromDate ? m_filterFromDate->date().toString("yyyy-MM-dd") : "";
    QString toDate = m_filterToDate ? m_filterToDate->date().toString("yyyy-MM-dd") : "";
    QString type = (m_filterTypeCombo && m_filterTypeCombo->currentText() != "ALL") ? m_filterTypeCombo->currentText() : "";
    QString search = m_searchGeneralEdit ? m_searchGeneralEdit->text().trimmed().toLower() : "";

    QVariantList trans = m_controller->getBardanaRegister(fromDate, toDate, type);
    m_registerTable->setRowCount(0);

    for (const auto& rowVar : trans) {
        QVariantMap row = rowVar.toMap();
        QString party = row.value("party_name").toString();
        QString veh = row.value("vehicle_no").toString();
        QString vNo = row.value("voucher_no").toString();

        if (!search.isEmpty()) {
            if (!party.toLower().contains(search) && !veh.toLower().contains(search) && !vNo.toLower().contains(search)) {
                continue;
            }
        }

        int r = m_registerTable->rowCount();
        m_registerTable->insertRow(r);

        auto createItem = [](const QString& text, Qt::Alignment align = Qt::AlignLeft | Qt::AlignVCenter) {
            auto* item = new QTableWidgetItem(text);
            item->setTextAlignment(align);
            return item;
        };

        QString drCr = row.value("dr_cr").toString();
        auto* drCrItem = createItem(drCr, Qt::AlignCenter);
        if (drCr == "Dr") {
            drCrItem->setForeground(QColor("#2563EB"));
            drCrItem->setFont(QFont("", -1, QFont::Bold));
        } else {
            drCrItem->setForeground(QColor("#059669"));
            drCrItem->setFont(QFont("", -1, QFont::Bold));
        }

        m_registerTable->setItem(r, 0, createItem(row.value("id").toString(), Qt::AlignCenter));
        m_registerTable->setItem(r, 1, createItem(row.value("voucher_date").toString(), Qt::AlignCenter));
        m_registerTable->setItem(r, 2, createItem(row.value("voucher_no").toString(), Qt::AlignCenter));
        m_registerTable->setItem(r, 3, createItem(row.value("vch_type").toString(), Qt::AlignCenter));
        m_registerTable->setItem(r, 4, createItem(party));
        m_registerTable->setItem(r, 5, createItem(row.value("bardana_type").toString()));
        m_registerTable->setItem(r, 6, createItem(row.value("godown_name").toString()));
        m_registerTable->setItem(r, 7, drCrItem);
        m_registerTable->setItem(r, 8, createItem(QString::number(row.value("qty").toInt()), Qt::AlignRight | Qt::AlignVCenter));
        m_registerTable->setItem(r, 9, createItem(QString("₹%1").arg(QString::number(row.value("rate").toDouble(), 'f', 2)), Qt::AlignRight | Qt::AlignVCenter));
        m_registerTable->setItem(r, 10, createItem(QString("₹%1").arg(QString::number(row.value("amount").toDouble(), 'f', 2)), Qt::AlignRight | Qt::AlignVCenter));
        m_registerTable->setItem(r, 11, createItem(veh));
        m_registerTable->setItem(r, 12, createItem(row.value("narration").toString()));
    }
}

void BardanaWidget::populateGodownSummary() {
    QVariantList summary = m_controller->getGodownStockSummary();
    m_godownTable->setRowCount(0);

    for (const auto& rowVar : summary) {
        QVariantMap row = rowVar.toMap();
        int r = m_godownTable->rowCount();
        m_godownTable->insertRow(r);

        auto createItem = [](const QString& text, Qt::Alignment align = Qt::AlignLeft | Qt::AlignVCenter) {
            auto* item = new QTableWidgetItem(text);
            item->setTextAlignment(align);
            return item;
        };

        m_godownTable->setItem(r, 0, createItem(row.value("godown_name").toString()));
        m_godownTable->setItem(r, 1, createItem(row.value("bardana_type").toString()));
        m_godownTable->setItem(r, 2, createItem("0", Qt::AlignRight | Qt::AlignVCenter));
        m_godownTable->setItem(r, 3, createItem(QString::number(row.value("total_received").toInt()), Qt::AlignRight | Qt::AlignVCenter));
        m_godownTable->setItem(r, 4, createItem(QString::number(row.value("total_issued").toInt()), Qt::AlignRight | Qt::AlignVCenter));

        int netBal = row.value("balance_stock").toInt();
        auto* balItem = createItem(QString::number(netBal), Qt::AlignRight | Qt::AlignVCenter);
        balItem->setFont(QFont("", -1, QFont::Bold));
        if (netBal >= 0) balItem->setForeground(QColor("#059669"));
        else balItem->setForeground(QColor("#DC2626"));
        m_godownTable->setItem(r, 5, balItem);
    }
}

void BardanaWidget::populatePartySummary() {
    QVariantList summary = m_controller->getPartyBalanceSummary();
    QString filter = m_searchPartyBox ? m_searchPartyBox->text().trimmed().toLower() : "";
    m_partyTable->setRowCount(0);

    for (const auto& rowVar : summary) {
        QVariantMap row = rowVar.toMap();
        QString party = row.value("party_name").toString();

        if (!filter.isEmpty() && !party.toLower().contains(filter)) {
            continue;
        }

        int r = m_partyTable->rowCount();
        m_partyTable->insertRow(r);

        auto createItem = [](const QString& text, Qt::Alignment align = Qt::AlignLeft | Qt::AlignVCenter) {
            auto* item = new QTableWidgetItem(text);
            item->setTextAlignment(align);
            return item;
        };

        m_partyTable->setItem(r, 0, createItem(party));
        m_partyTable->setItem(r, 1, createItem(row.value("bardana_type").toString()));
        m_partyTable->setItem(r, 2, createItem(QString::number(row.value("total_issued").toInt()), Qt::AlignRight | Qt::AlignVCenter));
        m_partyTable->setItem(r, 3, createItem(QString::number(row.value("total_received").toInt()), Qt::AlignRight | Qt::AlignVCenter));

        int netBal = row.value("balance_bags").toInt();
        auto* balItem = createItem(QString::number(netBal), Qt::AlignRight | Qt::AlignVCenter);
        balItem->setFont(QFont("", -1, QFont::Bold));

        QString statusText = "Settled (0)";
        if (netBal > 0) {
            statusText = QString("Party Owes %1 Bags").arg(netBal);
            balItem->setForeground(QColor("#2563EB"));
        } else if (netBal < 0) {
            statusText = QString("Mill Owes %1 Bags").arg(-netBal);
            balItem->setForeground(QColor("#D97706"));
        } else {
            balItem->setForeground(QColor("#059669"));
        }

        m_partyTable->setItem(r, 4, balItem);
        m_partyTable->setItem(r, 5, createItem(statusText, Qt::AlignCenter));
    }
}

void BardanaWidget::onSaveTransaction() {
    QString party = m_partySearchBox ? m_partySearchBox->currentPartyName().trimmed() : "";
    if (party.isEmpty() && m_partySearchBox) {
        party = m_partySearchBox->text().trimmed();
    }
    if (party.isEmpty()) {
        CustomMessageBox::critical(this, "Validation Error", "Please select or enter party name.");
        if (m_partySearchBox) m_partySearchBox->setFocus();
        return;
    }

    int partyId = m_partySearchBox ? m_partySearchBox->selectedPartyId() : 0;

    int qty = m_qtyEdit->text().toInt();
    if (qty <= 0) {
        CustomMessageBox::critical(this, "Validation Error", "Bags quantity must be greater than zero.");
        m_qtyEdit->setFocus();
        return;
    }

    double rate = m_rateEdit->text().toDouble();
    QString vchType = m_vchTypeCombo->currentText();
    QString vchDate = m_dateEdit->date().toString("yyyy-MM-dd");
    QString vchNo = m_vchNoEdit->text().trimmed();
    if (vchNo.isEmpty()) {
        vchNo = QString("BD-%1").arg(QDateTime::currentSecsSinceEpoch() % 100000);
    }

    QString bardanaType = m_bardanaItemEdit ? m_bardanaItemEdit->text().trimmed() : "";
    if (bardanaType.isEmpty()) bardanaType = "Jute 50kg (Pukka)";
    QString godown = m_godownCombo->currentText();
    QString drCr = m_drCrCombo->currentText().startsWith("Dr") ? "Dr" : "Cr";
    QString veh = m_vehicleEdit->text().trimmed();
    QString bill = m_billNoEdit->text().trimmed();
    QString narr = m_narrationEdit->text().trimmed();

    bool ok = m_controller->createBardanaTransaction(
        vchType, vchDate, vchNo, partyId, party, bardanaType, godown, drCr, qty, rate, veh, bill, narr
    );

    if (ok) {
        CustomMessageBox::information(this, "Voucher Saved", "Bardana packaging voucher saved successfully!");
        m_partySearchBox->clearParty();
        m_qtyEdit->clear();
        m_rateEdit->clear();
        m_amountEdit->clear();
        m_vehicleEdit->clear();
        m_billNoEdit->clear();
        m_narrationEdit->clear();
        m_vchNoEdit->clear();
        m_entryCard->setVisible(false);
        refreshData();
    } else {
        CustomMessageBox::critical(this, "Error", "Failed to save Bardana transaction.");
    }
}

void BardanaWidget::onFilterChanged() {
    populateRegisterTable();
}

void BardanaWidget::onTabChanged(int index) {
    if (index == 0) populateRegisterTable();
    else if (index == 1) populatePartySummary();
    else if (index == 2) populateGodownSummary();
}

void BardanaWidget::onDeleteSelected() {
    int r = m_registerTable->currentRow();
    if (r < 0) {
        CustomMessageBox::information(this, "Selection Required", "Please select a transaction to delete.");
        return;
    }

    int transId = m_registerTable->item(r, 0)->text().toInt();
    if (CustomMessageBox::question(this, "Confirm Deletion", "Are you sure you want to delete this Bardana transaction?")) {
        if (m_controller->deleteBardanaTransaction(transId)) {
            refreshData();
        }
    }
}

void BardanaWidget::onExportPdf() {
    if (!m_printCtrl) return;
    QString html = "<h2>Bardana Packaging Register</h2><table border='1' cellspacing='0' cellpadding='5'><tr>"
                   "<th>Date</th><th>Vch No</th><th>Type</th><th>Party</th><th>Bardana Type</th><th>Godown</th><th>Dr/Cr</th><th>Qty</th><th>Rate</th><th>Amount</th></tr>";

    for (int r = 0; r < m_registerTable->rowCount(); ++r) {
        html += QString("<tr><td>%1</td><td>%2</td><td>%3</td><td>%4</td><td>%5</td><td>%6</td><td>%7</td><td>%8</td><td>%9</td><td>%10</td></tr>")
                .arg(m_registerTable->item(r, 1)->text())
                .arg(m_registerTable->item(r, 2)->text())
                .arg(m_registerTable->item(r, 3)->text())
                .arg(m_registerTable->item(r, 4)->text())
                .arg(m_registerTable->item(r, 5)->text())
                .arg(m_registerTable->item(r, 6)->text())
                .arg(m_registerTable->item(r, 7)->text())
                .arg(m_registerTable->item(r, 8)->text())
                .arg(m_registerTable->item(r, 9)->text())
                .arg(m_registerTable->item(r, 10)->text());
    }
    html += "</table>";
    m_printCtrl->exportHtmlToPdf(html, "Bardana_Register.pdf");
}

void BardanaWidget::onExportCsv() {
    QString fileName = QFileDialog::getSaveFileName(this, "Export Bardana Register CSV", "Bardana_Register.csv", "CSV Files (*.csv)");
    if (fileName.isEmpty()) return;

    QFile file(fileName);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&file);
        out << "ID,Date,Vch No,Type,Party,Bardana Type,Godown,Dr/Cr,Qty,Rate,Amount,Vehicle,Narration\n";
        for (int r = 0; r < m_registerTable->rowCount(); ++r) {
            QStringList cells;
            for (int c = 0; c < m_registerTable->columnCount(); ++c) {
                cells << QString("\"%1\"").arg(m_registerTable->item(r, c) ? m_registerTable->item(r, c)->text() : "");
            }
            out << cells.join(",") << "\n";
        }
        file.close();
        CustomMessageBox::information(this, "Export Complete", "CSV export completed successfully.");
    }
}
