#include "gate_register_widget.h"
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
#include <QTime>
#include <QFileDialog>
#include <QFile>
#include <QTextStream>

GateRegisterWidget::GateRegisterWidget(GateRegisterController* controller, PrintExportController* printCtrl, QWidget* parent)
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

    m_histFromDate->blockSignals(true);
    m_histToDate->blockSignals(true);
    m_histFromDate->setDate(sDate);
    m_histToDate->setDate(eDate);
    m_histFromDate->blockSignals(false);
    m_histToDate->blockSignals(false);

    connect(m_controller, &GateRegisterController::gateDataChanged, this, &GateRegisterWidget::refreshData);
    refreshData();
}

void GateRegisterWidget::applyCustomStyles() {
    setAttribute(Qt::WA_StyledBackground, true);
    setStyleSheet(
        "GateRegisterWidget { background-color: #F8FAFC; }"
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

void GateRegisterWidget::setupUi() {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(16, 14, 16, 14);
    mainLayout->setSpacing(12);

    // ========================================================================
    // 1. TOP HEADER BAR
    // ========================================================================
    auto* headerLayout = new QHBoxLayout();
    auto* titleCol = new QVBoxLayout();
    titleCol->setSpacing(2);

    auto* titleLabel = new QLabel("Gate Inward & Outward Security Register", this);
    titleLabel->setStyleSheet("font-size: 19px; font-weight: 800; color: #0F172A;");
    auto* subLabel = new QLabel("Vehicle security check-in/out, gross & tare weighbridge capture, driver logs, and yard dispatch tracking.", this);
    subLabel->setStyleSheet("font-size: 11.5px; color: #64748B; font-weight: 500;");
    titleCol->addWidget(titleLabel);
    titleCol->addWidget(subLabel);
    headerLayout->addLayout(titleCol);
    headerLayout->addStretch();

    auto* btnNewPass = new KbdBadgeButton("+ New Gate Pass", "F2", this);
    btnNewPass->setPrimaryColor("#16A34A", "#15803D");
    btnNewPass->setTextColor("#FFFFFF");
    connect(btnNewPass, &QPushButton::clicked, this, &GateRegisterWidget::toggleEntryForm);
    headerLayout->addWidget(btnNewPass);

    auto* btnKanda = new KbdBadgeButton("Weighbridge (Kanda)", "F3", this);
    btnKanda->setPrimaryColor("#2563EB", "#1D4ED8");
    btnKanda->setTextColor("#FFFFFF");
    connect(btnKanda, &QPushButton::clicked, this, &GateRegisterWidget::onSendToWeighbridge);
    headerLayout->addWidget(btnKanda);

    auto* btnCsv = new QPushButton("Export CSV", this);
    btnCsv->setStyleSheet("background-color: #FFFFFF; border: 1px solid #CBD5E1; border-radius: 6px; padding: 6px 14px; font-weight: 700; color: #334155; font-size: 12px;");
    connect(btnCsv, &QPushButton::clicked, this, &GateRegisterWidget::onExportCsv);
    headerLayout->addWidget(btnCsv);

    auto* btnExportPdf = new QPushButton("Export PDF", this);
    btnExportPdf->setStyleSheet("background-color: #2563EB; border: none; border-radius: 6px; padding: 6px 14px; font-weight: 700; color: #FFFFFF; font-size: 12px;");
    connect(btnExportPdf, &QPushButton::clicked, this, &GateRegisterWidget::onExportPdf);
    headerLayout->addWidget(btnExportPdf);

    auto* btnBack = new KbdBadgeButton("← Back to Menu", "Esc", this);
    btnBack->setPrimaryColor("#F1F5F9", "#E2E8F0");
    btnBack->setTextColor("#475569");
    connect(btnBack, &QPushButton::clicked, this, &GateRegisterWidget::backRequested);
    headerLayout->addWidget(btnBack);

    mainLayout->addLayout(headerLayout);

    // ========================================================================
    // 2. KPI METRIC SUMMARY CARDS
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

    kpiLayout->addWidget(createKpiCard("Vehicles at Gate / Yard", "0 Trucks", "#D97706", m_lblActiveAtGate));
    kpiLayout->addWidget(createKpiCard("Inward Arrivals Today", "0 Trips", "#2563EB", m_lblInwardToday));
    kpiLayout->addWidget(createKpiCard("Outward Dispatches Today", "0 Trips", "#059669", m_lblOutwardToday));
    kpiLayout->addWidget(createKpiCard("Total Commodity Tonnage", "0.00 Qtl", "#7C3AED", m_lblTotalWeight));
    mainLayout->addLayout(kpiLayout);

    // ========================================================================
    // 3. COLLAPSIBLE NEW GATE PASS FORM CARD
    // ========================================================================
    m_entryCard = new QFrame(this);
    m_entryCard->setProperty("class", "entryCard");
    auto* formLayout = new QVBoxLayout(m_entryCard);
    formLayout->setContentsMargins(14, 12, 14, 12);
    formLayout->setSpacing(8);

    auto* fHeader = new QHBoxLayout();
    auto* fTitle = new QLabel("Register New Vehicle Security Gate Entry", m_entryCard);
    fTitle->setStyleSheet("font-size: 13px; font-weight: 800; color: #0F172A;");
    fHeader->addWidget(fTitle);
    fHeader->addStretch();
    auto* fCloseBtn = new QPushButton("✕ Close Panel", m_entryCard);
    fCloseBtn->setStyleSheet("border: none; background: transparent; color: #64748B; font-weight: 700; font-size: 11px;");
    connect(fCloseBtn, &QPushButton::clicked, this, &GateRegisterWidget::toggleEntryForm);
    fHeader->addWidget(fCloseBtn);
    formLayout->addLayout(fHeader);

    auto* grid = new QGridLayout();
    grid->setHorizontalSpacing(10);
    grid->setVerticalSpacing(6);

    auto addField = [](QGridLayout* g, const QString& label, QWidget* widget, int row, int col) {
        auto* lay = new QVBoxLayout();
        lay->setSpacing(2);
        auto* l = new QLabel(label);
        l->setProperty("class", "fieldLabel");
        lay->addWidget(l);
        lay->addWidget(widget);
        g->addLayout(lay, row, col);
    };

    // Row 0
    m_gatePassEdit = new QLineEdit(m_entryCard);
    m_gatePassEdit->setText(m_controller->generateNextGatePassNo());
    m_gatePassEdit->setReadOnly(true);
    m_gatePassEdit->setStyleSheet("background-color: #F8FAFC; font-weight: 700; color: #0F172A;");

    m_dateEdit = new AccountingDateEdit(QDate::currentDate(), m_entryCard);
    m_dateEdit->setCalendarPopup(true);
    m_dateEdit->setDisplayFormat("dd-MM-yyyy");

    m_timeEdit = new QLineEdit(QTime::currentTime().toString("HH:mm"), m_entryCard);

    m_directionCombo = new QComboBox(m_entryCard);
    m_directionCombo->addItems({"INWARD (Paddy/Raw Arrival)", "OUTWARD (Rice/Finished Dispatch)"});

    addField(grid, "Gate Pass No:", m_gatePassEdit, 0, 0);
    addField(grid, "Entry Date:", m_dateEdit, 0, 1);
    addField(grid, "Entry Time (HH:mm):", m_timeEdit, 0, 2);
    addField(grid, "Movement Direction:", m_directionCombo, 0, 3);

    // Row 1
    m_purposeCombo = new QComboBox(m_entryCard);
    m_purposeCombo->addItems({"Paddy Procurement Purchase", "Rice Sales Dispatch", "Bardana Packaging Movement", "Bran/Husk Sales", "Milling Jobwork", "General"});

    m_vehicleEdit = new QLineEdit(m_entryCard);
    m_vehicleEdit->setPlaceholderText("e.g. HR-57-A-1234");

    m_driverNameEdit = new QLineEdit(m_entryCard);
    m_driverNameEdit->setPlaceholderText("Driver Full Name");

    m_driverPhoneEdit = new QLineEdit(m_entryCard);
    m_driverPhoneEdit->setPlaceholderText("Driver Mobile (10-digits)");

    addField(grid, "Entry Purpose:", m_purposeCombo, 1, 0);
    addField(grid, "Vehicle / Truck No:", m_vehicleEdit, 1, 1);
    addField(grid, "Driver Name:", m_driverNameEdit, 1, 2);
    addField(grid, "Driver Phone:", m_driverPhoneEdit, 1, 3);

    // Row 2
    m_transporterSearchBox = new AccountSearchBox(m_entryCard);
    m_transporterSearchBox->setPlaceholderText("Search Transporter / Self Fleet...");

    m_partySearchBox = new AccountSearchBox(m_entryCard);
    m_partySearchBox->setPlaceholderText("Search Supplier / Farmer / Customer...");

    m_commodityEdit = new ItemSearchEditor(m_entryCard);
    m_commodityEdit->setPlaceholderText("Search Commodity / Stock Item...");
    m_commodityEdit->setText("Paddy Basmati 1121");

    m_bagCountEdit = new QLineEdit(m_entryCard);
    m_bagCountEdit->setPlaceholderText("e.g. 500");

    addField(grid, "Transporter Name:", m_transporterSearchBox, 2, 0);
    addField(grid, "Party / Consignee:", m_partySearchBox, 2, 1);
    addField(grid, "Commodity Item:", m_commodityEdit, 2, 2);
    addField(grid, "Bag Count:", m_bagCountEdit, 2, 3);

    // Row 3
    m_grossWtEdit = new QLineEdit(m_entryCard);
    m_grossWtEdit->setPlaceholderText("Gross Wt in Quintals");

    m_remarksEdit = new QLineEdit(m_entryCard);
    m_remarksEdit->setPlaceholderText("Gate notes / Security inspection remarks");

    m_btnSave = new QPushButton("Register Gate Entry (F12)", m_entryCard);
    m_btnSave->setStyleSheet("background-color: #16A34A; color: #FFFFFF; font-weight: 800; font-size: 12px; padding: 7px 16px; border-radius: 6px;");
    connect(m_btnSave, &QPushButton::clicked, this, &GateRegisterWidget::onSaveGateEntry);

    addField(grid, "Gross Weight (Qtl):", m_grossWtEdit, 3, 0);
    
    auto* rLay = new QHBoxLayout();
    rLay->setSpacing(10);
    auto* remBox = new QVBoxLayout();
    remBox->setSpacing(2);
    auto* rmLbl = new QLabel("Remarks / Notes:");
    rmLbl->setProperty("class", "fieldLabel");
    remBox->addWidget(rmLbl);
    remBox->addWidget(m_remarksEdit);
    rLay->addLayout(remBox, 2);

    auto* sBox = new QVBoxLayout();
    sBox->setSpacing(2);
    sBox->addWidget(new QLabel(" "));
    sBox->addWidget(m_btnSave);
    rLay->addLayout(sBox, 1);

    grid->addLayout(rLay, 3, 1, 1, 3);

    formLayout->addLayout(grid);
    m_entryCard->setVisible(false);
    mainLayout->addWidget(m_entryCard);

    // ========================================================================
    // 4. MAIN TAB WIDGET
    // ========================================================================
    m_tabWidget = new QTabWidget(this);

    // ------------------------------------------------------------------------
    // TAB 1: LIVE GATE PASS REGISTER
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
        "QDateEdit, QLineEdit, QComboBox { background-color: #FFFFFF; border: 1.5px solid #CBD5E1; border-radius: 6px; padding: 4px 8px; font-size: 12px; color: #0F172A; font-weight: 600; }"
        "QDateEdit:focus, QLineEdit:focus, QComboBox:focus { border-color: #2563EB; background-color: #F8FAFC; }"
    );
    auto* filterLayout = new QHBoxLayout(filterCard);
    filterLayout->setContentsMargins(12, 8, 12, 8);
    filterLayout->setSpacing(12);

    filterLayout->addWidget(new QLabel("Quick Search:", filterCard));
    m_searchLiveEdit = new QLineEdit(filterCard);
    m_searchLiveEdit->setPlaceholderText("Search vehicle, gate pass, driver, party...");
    m_searchLiveEdit->setClearButtonEnabled(true);
    connect(m_searchLiveEdit, &QLineEdit::textChanged, this, &GateRegisterWidget::populateLiveTable);
    filterLayout->addWidget(m_searchLiveEdit, 1);

    auto* btnMarkWeighed = new QPushButton("Mark Weighed", filterCard);
    btnMarkWeighed->setStyleSheet("background-color: #EFF6FF; color: #2563EB; border: 1px solid #BFDBFE; border-radius: 4px; padding: 4px 10px; font-weight: 700; font-size: 11px;");
    connect(btnMarkWeighed, &QPushButton::clicked, this, [this]() { onStatusUpdate("WEIGHED"); });
    filterLayout->addWidget(btnMarkWeighed);

    auto* btnMarkDispatched = new QPushButton("Mark Dispatched", filterCard);
    btnMarkDispatched->setStyleSheet("background-color: #ECFDF5; color: #059669; border: 1px solid #A7F3D0; border-radius: 4px; padding: 4px 10px; font-weight: 700; font-size: 11px;");
    connect(btnMarkDispatched, &QPushButton::clicked, this, [this]() { onStatusUpdate("DISPATCHED"); });
    filterLayout->addWidget(btnMarkDispatched);

    tab1Layout->addWidget(filterCard);

    m_liveTable = new QTableWidget(tab1);
    m_liveTable->setColumnCount(13);
    m_liveTable->setHorizontalHeaderLabels({
        "Pass No", "Date", "Time", "Dir", "Vehicle No", "Driver", "Party / Consignee",
        "Commodity", "Bags", "Gross (Qtl)", "Tare (Qtl)", "Net (Qtl)", "Status"
    });
    m_liveTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_liveTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_liveTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    m_liveTable->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    m_liveTable->horizontalHeader()->setSectionResizeMode(4, QHeaderView::ResizeToContents);
    m_liveTable->horizontalHeader()->setSectionResizeMode(5, QHeaderView::ResizeToContents);
    m_liveTable->horizontalHeader()->setSectionResizeMode(6, QHeaderView::Stretch);
    for (int c = 7; c < 13; ++c) {
        m_liveTable->horizontalHeader()->setSectionResizeMode(c, QHeaderView::ResizeToContents);
    }
    m_liveTable->verticalHeader()->setVisible(false);
    m_liveTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_liveTable->setSelectionMode(QAbstractItemView::SingleSelection);
    m_liveTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_liveTable->setAlternatingRowColors(true);
    tab1Layout->addWidget(m_liveTable);

    m_tabWidget->addTab(tab1, "Active Vehicles at Gate / Yard");

    // ------------------------------------------------------------------------
    // TAB 2: COMPLETED HISTORY & WEIGHBRIDGE OUT-TURN
    // ------------------------------------------------------------------------
    auto* tab2 = new QWidget(this);
    auto* tab2Layout = new QVBoxLayout(tab2);
    tab2Layout->setContentsMargins(8, 10, 8, 8);
    tab2Layout->setSpacing(8);

    auto* histFilterCard = new QFrame(tab2);
    histFilterCard->setStyleSheet(
        "QFrame { background-color: #FFFFFF; border: 1px solid #CBD5E1; border-radius: 8px; }"
        "QLabel { color: #475569; font-weight: 700; font-size: 11.5px; border: none; background: transparent; }"
        "AccountingDateEdit, QDateEdit, QLineEdit, QComboBox { background-color: #FFFFFF; border: 1.5px solid #CBD5E1; border-radius: 6px; padding: 4px 8px; font-size: 12px; color: #0F172A; font-weight: 600; }"
        "AccountingDateEdit:focus, QDateEdit:focus, QLineEdit:focus, QComboBox:focus { border-color: #2563EB; background-color: #F8FAFC; }"
    );
    auto* hFLay = new QHBoxLayout(histFilterCard);
    hFLay->setContentsMargins(12, 8, 12, 8);
    hFLay->setSpacing(12);

    hFLay->addWidget(new QLabel("From Date:", histFilterCard));
    m_histFromDate = new AccountingDateEdit(QDate::currentDate().addDays(-30), histFilterCard);
    m_histFromDate->setCalendarPopup(true);
    m_histFromDate->setDisplayFormat("dd-MM-yyyy");
    hFLay->addWidget(m_histFromDate);

    hFLay->addWidget(new QLabel("To Date:", histFilterCard));
    m_histToDate = new AccountingDateEdit(QDate::currentDate(), histFilterCard);
    m_histToDate->setCalendarPopup(true);
    m_histToDate->setDisplayFormat("dd-MM-yyyy");
    hFLay->addWidget(m_histToDate);

    auto* periodBtn = new QPushButton("Period (F2)", histFilterCard);
    periodBtn->setFixedHeight(32);
    periodBtn->setCursor(Qt::PointingHandCursor);
    periodBtn->setStyleSheet("QPushButton { background-color: #F1F5F9; color: #0F172A; border: 1.5px solid #CBD5E1; border-radius: 6px; padding: 0 10px; font-weight: 700; font-size: 11.5px; } QPushButton:hover { background-color: #E2E8F0; }");
    connect(periodBtn, &QPushButton::clicked, this, [this]() {
        QDate f = m_histFromDate->date();
        QDate t = m_histToDate->date();
        if (VoucherDateDialog::selectDateRange(this, &f, &t, f, t)) {
            m_histFromDate->setDate(f);
            m_histToDate->setDate(t);
            populateHistoryTable();
        }
    });
    hFLay->addWidget(periodBtn);

    hFLay->addWidget(new QLabel("Direction:", histFilterCard));
    m_histDirectionCombo = new QComboBox(histFilterCard);
    m_histDirectionCombo->addItems({"ALL", "INWARD", "OUTWARD"});
    hFLay->addWidget(m_histDirectionCombo);

    hFLay->addWidget(new QLabel("Search:", histFilterCard));
    m_searchHistEdit = new QLineEdit(histFilterCard);
    m_searchHistEdit->setPlaceholderText("Filter completed slips...");
    m_searchHistEdit->setClearButtonEnabled(true);
    connect(m_searchHistEdit, &QLineEdit::textChanged, this, &GateRegisterWidget::populateHistoryTable);
    hFLay->addWidget(m_searchHistEdit, 1);

    connect(m_histFromDate, &AccountingDateEdit::dateChanged, this, &GateRegisterWidget::populateHistoryTable);
    connect(m_histToDate, &AccountingDateEdit::dateChanged, this, &GateRegisterWidget::populateHistoryTable);
    connect(m_histDirectionCombo, &QComboBox::currentTextChanged, this, &GateRegisterWidget::populateHistoryTable);

    tab2Layout->addWidget(histFilterCard);

    m_historyTable = new QTableWidget(tab2);
    m_historyTable->setColumnCount(13);
    m_historyTable->setHorizontalHeaderLabels({
        "Pass No", "Date", "In Time", "Out Time", "Dir", "Vehicle No", "Driver", "Party / Consignee",
        "Commodity", "Bags", "Gross (Qtl)", "Tare (Qtl)", "Net Wt (Qtl)"
    });
    m_historyTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_historyTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_historyTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    m_historyTable->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    m_historyTable->horizontalHeader()->setSectionResizeMode(4, QHeaderView::ResizeToContents);
    m_historyTable->horizontalHeader()->setSectionResizeMode(5, QHeaderView::ResizeToContents);
    m_historyTable->horizontalHeader()->setSectionResizeMode(6, QHeaderView::ResizeToContents);
    m_historyTable->horizontalHeader()->setSectionResizeMode(7, QHeaderView::Stretch);
    for (int c = 8; c < 13; ++c) {
        m_historyTable->horizontalHeader()->setSectionResizeMode(c, QHeaderView::ResizeToContents);
    }
    m_historyTable->verticalHeader()->setVisible(false);
    m_historyTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_historyTable->setSelectionMode(QAbstractItemView::SingleSelection);
    m_historyTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_historyTable->setAlternatingRowColors(true);
    tab2Layout->addWidget(m_historyTable);

    m_tabWidget->addTab(tab2, "Completed Dispatches & Gate History");

    mainLayout->addWidget(m_tabWidget, 1);
}

void GateRegisterWidget::toggleEntryForm() {
    if (m_entryCard) {
        bool nowVisible = !m_entryCard->isVisible();
        m_entryCard->setVisible(nowVisible);
        if (nowVisible) {
            m_gatePassEdit->setText(m_controller->generateNextGatePassNo());
            m_timeEdit->setText(QTime::currentTime().toString("HH:mm"));
            m_vehicleEdit->setFocus();
        }
    }
}

void GateRegisterWidget::keyPressEvent(QKeyEvent* event) {
    if (event->key() == Qt::Key_Escape) {
        if (m_entryCard && m_entryCard->isVisible()) {
            m_entryCard->setVisible(false);
            return;
        }
        emit backRequested();
    } else if (event->key() == Qt::Key_F2) {
        toggleEntryForm();
    } else if (event->key() == Qt::Key_F3) {
        onSendToWeighbridge();
    } else if (event->key() == Qt::Key_F12) {
        if (m_entryCard && m_entryCard->isVisible()) {
            onSaveGateEntry();
        }
    } else {
        QWidget::keyPressEvent(event);
    }
}

void GateRegisterWidget::refreshData() {
    populateLiveTable();
    populateHistoryTable();
    updateSummaryKpis();
    m_isDirty = false;
}

void GateRegisterWidget::updateSummaryKpis() {
    QVariantList activeEntries = m_controller->getLiveGateEntries();
    QVariantList allEntries = m_controller->getGateRegisterHistory();

    int activeCount = activeEntries.size();
    int inwardCount = 0;
    int outwardCount = 0;
    double totalWeightQtl = 0.0;
    QString today = QDate::currentDate().toString("yyyy-MM-dd");

    for (const auto& rowVar : allEntries) {
        QVariantMap row = rowVar.toMap();
        QString dt = row.value("entry_date").toString();
        QString dir = row.value("direction").toString().toUpper();
        if (dt == today) {
            if (dir.contains("INWARD")) inwardCount++;
            else if (dir.contains("OUTWARD")) outwardCount++;
        }
        totalWeightQtl += row.value("net_weight_qtl").toDouble();
        if (row.value("net_weight_qtl").toDouble() <= 0.0) {
            totalWeightQtl += row.value("gross_weight_qtl").toDouble();
        }
    }

    if (m_lblActiveAtGate) m_lblActiveAtGate->setText(QString("%1 Trucks").arg(activeCount));
    if (m_lblInwardToday) m_lblInwardToday->setText(QString("%1 Trips").arg(inwardCount));
    if (m_lblOutwardToday) m_lblOutwardToday->setText(QString("%1 Trips").arg(outwardCount));
    if (m_lblTotalWeight) m_lblTotalWeight->setText(QString("%1 Qtl").arg(QString::number(totalWeightQtl, 'f', 2)));
}

void GateRegisterWidget::populateLiveTable() {
    QVariantList entries = m_controller->getLiveGateEntries();
    QString search = m_searchLiveEdit ? m_searchLiveEdit->text().trimmed().toLower() : "";
    m_liveTable->setRowCount(0);

    for (const auto& rowVar : entries) {
        QVariantMap row = rowVar.toMap();
        QString pNo = row.value("gate_pass_no").toString();
        QString veh = row.value("vehicle_no").toString();
        QString party = row.value("party_name").toString();
        QString driver = row.value("driver_name").toString();

        if (!search.isEmpty()) {
            if (!pNo.toLower().contains(search) && !veh.toLower().contains(search) &&
                !party.toLower().contains(search) && !driver.toLower().contains(search)) {
                continue;
            }
        }

        int r = m_liveTable->rowCount();
        m_liveTable->insertRow(r);

        auto createItem = [](const QString& text, Qt::Alignment align = Qt::AlignLeft | Qt::AlignVCenter) {
            auto* item = new QTableWidgetItem(text);
            item->setTextAlignment(align);
            return item;
        };

        QString dir = row.value("direction").toString();
        auto* dirItem = createItem(dir.startsWith("INWARD") ? "INWARD" : "OUTWARD", Qt::AlignCenter);
        if (dir.startsWith("INWARD")) dirItem->setForeground(QColor("#2563EB"));
        else dirItem->setForeground(QColor("#059669"));

        QString status = row.value("status").toString();
        auto* statusItem = createItem(status, Qt::AlignCenter);
        statusItem->setFont(QFont("", -1, QFont::Bold));
        if (status == "AT_GATE") statusItem->setForeground(QColor("#D97706"));
        else if (status == "WEIGHED") statusItem->setForeground(QColor("#2563EB"));
        else statusItem->setForeground(QColor("#059669"));

        m_liveTable->setItem(r, 0, createItem(pNo, Qt::AlignCenter));
        m_liveTable->setItem(r, 1, createItem(row.value("entry_date").toString(), Qt::AlignCenter));
        m_liveTable->setItem(r, 2, createItem(row.value("entry_time").toString(), Qt::AlignCenter));
        m_liveTable->setItem(r, 3, dirItem);
        m_liveTable->setItem(r, 4, createItem(veh));
        m_liveTable->setItem(r, 5, createItem(driver));
        m_liveTable->setItem(r, 6, createItem(party));
        m_liveTable->setItem(r, 7, createItem(row.value("commodity").toString()));
        m_liveTable->setItem(r, 8, createItem(QString::number(row.value("bag_count").toInt()), Qt::AlignRight | Qt::AlignVCenter));
        m_liveTable->setItem(r, 9, createItem(QString::number(row.value("gross_weight_qtl").toDouble(), 'f', 2), Qt::AlignRight | Qt::AlignVCenter));
        m_liveTable->setItem(r, 10, createItem(QString::number(row.value("tare_weight_qtl").toDouble(), 'f', 2), Qt::AlignRight | Qt::AlignVCenter));
        m_liveTable->setItem(r, 11, createItem(QString::number(row.value("net_weight_qtl").toDouble(), 'f', 2), Qt::AlignRight | Qt::AlignVCenter));
        m_liveTable->setItem(r, 12, statusItem);
    }
}

void GateRegisterWidget::populateHistoryTable() {
    QString fromDate = m_histFromDate ? m_histFromDate->date().toString("yyyy-MM-dd") : "";
    QString toDate = m_histToDate ? m_histToDate->date().toString("yyyy-MM-dd") : "";
    QString dirFilter = (m_histDirectionCombo && m_histDirectionCombo->currentText() != "ALL") ? m_histDirectionCombo->currentText() : "";
    QString search = m_searchHistEdit ? m_searchHistEdit->text().trimmed().toLower() : "";

    QVariantList entries = m_controller->getGateRegisterHistory(fromDate, toDate, dirFilter);
    m_historyTable->setRowCount(0);

    for (const auto& rowVar : entries) {
        QVariantMap row = rowVar.toMap();
        QString pNo = row.value("gate_pass_no").toString();
        QString veh = row.value("vehicle_no").toString();
        QString party = row.value("party_name").toString();

        if (!search.isEmpty()) {
            if (!pNo.toLower().contains(search) && !veh.toLower().contains(search) && !party.toLower().contains(search)) {
                continue;
            }
        }

        int r = m_historyTable->rowCount();
        m_historyTable->insertRow(r);

        auto createItem = [](const QString& text, Qt::Alignment align = Qt::AlignLeft | Qt::AlignVCenter) {
            auto* item = new QTableWidgetItem(text);
            item->setTextAlignment(align);
            return item;
        };

        m_historyTable->setItem(r, 0, createItem(pNo, Qt::AlignCenter));
        m_historyTable->setItem(r, 1, createItem(row.value("entry_date").toString(), Qt::AlignCenter));
        m_historyTable->setItem(r, 2, createItem(row.value("entry_time").toString(), Qt::AlignCenter));
        m_historyTable->setItem(r, 3, createItem(row.value("exit_time").toString(), Qt::AlignCenter));
        m_historyTable->setItem(r, 4, createItem(row.value("direction").toString().startsWith("INWARD") ? "IN" : "OUT", Qt::AlignCenter));
        m_historyTable->setItem(r, 5, createItem(veh));
        m_historyTable->setItem(r, 6, createItem(row.value("driver_name").toString()));
        m_historyTable->setItem(r, 7, createItem(party));
        m_historyTable->setItem(r, 8, createItem(row.value("commodity").toString()));
        m_historyTable->setItem(r, 9, createItem(QString::number(row.value("bag_count").toInt()), Qt::AlignRight | Qt::AlignVCenter));
        m_historyTable->setItem(r, 10, createItem(QString::number(row.value("gross_weight_qtl").toDouble(), 'f', 2), Qt::AlignRight | Qt::AlignVCenter));
        m_historyTable->setItem(r, 11, createItem(QString::number(row.value("tare_weight_qtl").toDouble(), 'f', 2), Qt::AlignRight | Qt::AlignVCenter));
        m_historyTable->setItem(r, 12, createItem(QString::number(row.value("net_weight_qtl").toDouble(), 'f', 2), Qt::AlignRight | Qt::AlignVCenter));
    }
}

void GateRegisterWidget::onSaveGateEntry() {
    QString veh = m_vehicleEdit->text().trimmed();
    if (veh.isEmpty()) {
        CustomMessageBox::critical(this, "Validation Error", "Please enter vehicle registration number.");
        m_vehicleEdit->setFocus();
        return;
    }

    QString passNo = m_gatePassEdit->text().trimmed();
    QString dt = m_dateEdit->date().toString("yyyy-MM-dd");
    QString tm = m_timeEdit->text().trimmed();
    QString dir = m_directionCombo->currentText().startsWith("INWARD") ? "INWARD" : "OUTWARD";
    QString purpose = m_purposeCombo->currentText();
    QString driver = m_driverNameEdit->text().trimmed();
    QString phone = m_driverPhoneEdit->text().trimmed();
    QString trans = m_transporterSearchBox ? m_transporterSearchBox->currentPartyName().trimmed() : "";
    if (trans.isEmpty() && m_transporterSearchBox) trans = m_transporterSearchBox->text().trimmed();
    QString party = m_partySearchBox ? m_partySearchBox->currentPartyName().trimmed() : "";
    if (party.isEmpty() && m_partySearchBox) party = m_partySearchBox->text().trimmed();
    int partyId = m_partySearchBox ? m_partySearchBox->selectedPartyId() : 0;
    QString commodity = m_commodityEdit ? m_commodityEdit->text().trimmed() : "";
    if (commodity.isEmpty()) commodity = "Paddy Basmati 1121";
    int bags = m_bagCountEdit->text().toInt();
    double gross = m_grossWtEdit->text().toDouble();
    QString rem = m_remarksEdit->text().trimmed();

    bool ok = m_controller->createGateEntry(
        passNo, dt, tm, dir, purpose, veh, driver, phone, trans, partyId, party, commodity, bags, gross, rem
    );

    if (ok) {
        CustomMessageBox::information(this, "Gate Entry Created", QString("Gate Pass %1 registered successfully!").arg(passNo));
        m_vehicleEdit->clear();
        m_driverNameEdit->clear();
        m_driverPhoneEdit->clear();
        if (m_transporterSearchBox) m_transporterSearchBox->clearParty();
        if (m_partySearchBox) m_partySearchBox->clearParty();
        m_bagCountEdit->clear();
        m_grossWtEdit->clear();
        m_remarksEdit->clear();
        m_entryCard->setVisible(false);
        refreshData();
    } else {
        CustomMessageBox::critical(this, "Error", "Failed to create gate entry.");
    }
}

void GateRegisterWidget::onStatusUpdate(const QString& newStatus) {
    int r = m_liveTable->currentRow();
    if (r < 0) {
        CustomMessageBox::information(this, "Selection Required", "Please select a vehicle row to update status.");
        return;
    }

    QString passNo = m_liveTable->item(r, 0)->text();
    QVariantMap entry = m_controller->getGateEntryByPassNo(passNo);
    int entryId = entry.value("id").toInt();

    QString exitTime = (newStatus == "DISPATCHED" || newStatus == "COMPLETED") ? QTime::currentTime().toString("HH:mm") : "";
    if (m_controller->updateGateStatus(entryId, newStatus, exitTime)) {
        refreshData();
    }
}

void GateRegisterWidget::onFilterChanged() {
    populateLiveTable();
    populateHistoryTable();
}

void GateRegisterWidget::onSendToWeighbridge() {
    int r = m_liveTable->currentRow();
    QString passNo = (r >= 0) ? m_liveTable->item(r, 0)->text() : "";
    QString vehNo = (r >= 0) ? m_liveTable->item(r, 4)->text() : "";
    QString party = (r >= 0) ? m_liveTable->item(r, 6)->text() : "";

    emit openWeighbridgeRequested(passNo, vehNo, party);
}

void GateRegisterWidget::onExportPdf() {
    if (!m_printCtrl) return;
    QString html = "<h2>Gate Inward & Outward Register</h2><table border='1' cellspacing='0' cellpadding='5'><tr>"
                   "<th>Pass No</th><th>Date</th><th>Time</th><th>Dir</th><th>Vehicle</th><th>Driver</th><th>Party</th><th>Commodity</th><th>Bags</th><th>Gross Wt</th></tr>";

    for (int r = 0; r < m_liveTable->rowCount(); ++r) {
        html += QString("<tr><td>%1</td><td>%2</td><td>%3</td><td>%4</td><td>%5</td><td>%6</td><td>%7</td><td>%8</td><td>%9</td><td>%10</td></tr>")
                .arg(m_liveTable->item(r, 0)->text())
                .arg(m_liveTable->item(r, 1)->text())
                .arg(m_liveTable->item(r, 2)->text())
                .arg(m_liveTable->item(r, 3)->text())
                .arg(m_liveTable->item(r, 4)->text())
                .arg(m_liveTable->item(r, 5)->text())
                .arg(m_liveTable->item(r, 6)->text())
                .arg(m_liveTable->item(r, 7)->text())
                .arg(m_liveTable->item(r, 8)->text())
                .arg(m_liveTable->item(r, 9)->text());
    }
    html += "</table>";
    m_printCtrl->exportHtmlToPdf(html, "Gate_Register.pdf");
}

void GateRegisterWidget::onExportCsv() {
    QString fileName = QFileDialog::getSaveFileName(this, "Export Gate Register CSV", "Gate_Register.csv", "CSV Files (*.csv)");
    if (fileName.isEmpty()) return;

    QFile file(fileName);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&file);
        out << "Pass No,Date,Time,Direction,Vehicle No,Driver,Party,Commodity,Bags,Gross Wt,Tare Wt,Net Wt,Status\n";
        for (int r = 0; r < m_liveTable->rowCount(); ++r) {
            QStringList cells;
            for (int c = 0; c < m_liveTable->columnCount(); ++c) {
                cells << QString("\"%1\"").arg(m_liveTable->item(r, c) ? m_liveTable->item(r, c)->text() : "");
            }
            out << cells.join(",") << "\n";
        }
        file.close();
        CustomMessageBox::information(this, "Export Complete", "CSV export completed successfully.");
    }
}
