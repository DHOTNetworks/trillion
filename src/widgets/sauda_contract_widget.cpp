#include "sauda_contract_widget.h"
#include "kbd_badge_button.h"
#include "custom_dialogs.h"
#include "../engine/accounting_engine.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QHeaderView>
#include <QMessageBox>
#include <QKeyEvent>
#include <QInputDialog>
#include <QFileDialog>
#include <QFile>
#include <QTextStream>
#include <cmath>

SaudaContractWidget::SaudaContractWidget(SaudaController* controller, PrintExportController* printCtrl, QWidget* parent)
    : QWidget(parent)
    , m_controller(controller)
    , m_printCtrl(printCtrl)
{
    setupUi();
    applyCustomStyles();

    connect(m_controller, &SaudaController::saudaDataChanged, this, &SaudaContractWidget::refreshData);
    connect(m_controller, &SaudaController::dalaliDataChanged, this, &SaudaContractWidget::refreshData);
    refreshData();
}

void SaudaContractWidget::applyCustomStyles() {
    setAttribute(Qt::WA_StyledBackground, true);
    setStyleSheet(
        "SaudaContractWidget { background-color: #F8FAFC; }"
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

void SaudaContractWidget::setupUi() {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(16, 14, 16, 14);
    mainLayout->setSpacing(12);

    // ========================================================================
    // 1. TOP HEADER BAR
    // ========================================================================
    auto* headerLayout = new QHBoxLayout();
    auto* titleCol = new QVBoxLayout();
    titleCol->setSpacing(2);

    auto* titleLabel = new QLabel("Sauda (Forward Contracts) & Brokerage Ledger", this);
    titleLabel->setStyleSheet("font-size: 19px; font-weight: 800; color: #0F172A;");
    auto* subLabel = new QLabel("Trade forward agreements, delivery schedules, fulfillment quotas, and Section 194-H brokerage double-entry settlements.", this);
    subLabel->setStyleSheet("font-size: 11.5px; color: #64748B; font-weight: 500;");
    titleCol->addWidget(titleLabel);
    titleCol->addWidget(subLabel);
    headerLayout->addLayout(titleCol);
    headerLayout->addStretch();

    auto* btnNewSauda = new KbdBadgeButton("+ Book New Sauda", "F2", this);
    btnNewSauda->setPrimaryColor("#16A34A", "#15803D");
    btnNewSauda->setTextColor("#FFFFFF");
    connect(btnNewSauda, &QPushButton::clicked, this, &SaudaContractWidget::toggleEntryForm);
    headerLayout->addWidget(btnNewSauda);

    auto* btnCsv = new QPushButton("Export CSV", this);
    btnCsv->setStyleSheet("background-color: #FFFFFF; border: 1px solid #CBD5E1; border-radius: 6px; padding: 6px 14px; font-weight: 700; color: #334155; font-size: 12px;");
    connect(btnCsv, &QPushButton::clicked, this, &SaudaContractWidget::onExportCsv);
    headerLayout->addWidget(btnCsv);

    auto* btnExportPdf = new QPushButton("Export PDF", this);
    btnExportPdf->setStyleSheet("background-color: #2563EB; border: none; border-radius: 6px; padding: 6px 14px; font-weight: 700; color: #FFFFFF; font-size: 12px;");
    connect(btnExportPdf, &QPushButton::clicked, this, &SaudaContractWidget::onExportPdf);
    headerLayout->addWidget(btnExportPdf);

    auto* btnBack = new KbdBadgeButton("← Back to Menu", "Esc", this);
    btnBack->setPrimaryColor("#F1F5F9", "#E2E8F0");
    btnBack->setTextColor("#475569");
    connect(btnBack, &QPushButton::clicked, this, &SaudaContractWidget::backRequested);
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

    kpiLayout->addWidget(createKpiCard("Pending Sauda Volume", "0.00 Qtl", "#2563EB", m_lblPendingVolume));
    kpiLayout->addWidget(createKpiCard("Fulfilled Volume", "0.00 Qtl", "#059669", m_lblFulfilledVolume));
    kpiLayout->addWidget(createKpiCard("Total Booked Value", "₹ 0.00", "#0F172A", m_lblTotalValue));
    kpiLayout->addWidget(createKpiCard("Unposted Dalali Payable", "₹ 0.00", "#DC2626", m_lblUnpostedDalali));
    mainLayout->addLayout(kpiLayout);

    // ========================================================================
    // 3. COLLAPSIBLE NEW SAUDA CONTRACT FORM CARD
    // ========================================================================
    m_entryCard = new QFrame(this);
    m_entryCard->setProperty("class", "entryCard");
    auto* formLayout = new QVBoxLayout(m_entryCard);
    formLayout->setContentsMargins(14, 12, 14, 12);
    formLayout->setSpacing(8);

    auto* fHeader = new QHBoxLayout();
    auto* fTitle = new QLabel("Book New Forward Sauda Contract", m_entryCard);
    fTitle->setStyleSheet("font-size: 13px; font-weight: 800; color: #0F172A;");
    fHeader->addWidget(fTitle);
    fHeader->addStretch();
    auto* fCloseBtn = new QPushButton("✕ Close Panel", m_entryCard);
    fCloseBtn->setStyleSheet("border: none; background: transparent; color: #64748B; font-weight: 700; font-size: 11px;");
    connect(fCloseBtn, &QPushButton::clicked, this, &SaudaContractWidget::toggleEntryForm);
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
    m_saudaNoEdit = new QLineEdit(m_entryCard);
    m_saudaNoEdit->setText(m_controller->generateNextSaudaNo());
    m_saudaNoEdit->setReadOnly(true);
    m_saudaNoEdit->setStyleSheet("background-color: #F8FAFC; font-weight: 700; color: #0F172A;");

    m_dateEdit = new AccountingDateEdit(QDate::currentDate(), m_entryCard);
    m_dateEdit->setCalendarPopup(true);
    m_dateEdit->setDisplayFormat("dd-MM-yyyy");

    m_typeCombo = new QComboBox(m_entryCard);
    m_typeCombo->addItems({"SALE", "BUY"});

    m_partySearchBox = new AccountSearchBox(m_entryCard);
    m_partySearchBox->setPlaceholderText("Search Buyer / Seller Party...");

    addField(grid, "Sauda No:", m_saudaNoEdit, 0, 0);
    addField(grid, "Contract Date:", m_dateEdit, 0, 1);
    addField(grid, "Contract Type:", m_typeCombo, 0, 2);
    addField(grid, "Party Name:", m_partySearchBox, 0, 3);

    // Row 1
    m_brokerSearchBox = new AccountSearchBox(m_entryCard);
    m_brokerSearchBox->setPlaceholderText("Search Broker / Dalal Name...");

    m_itemEdit = new ItemSearchEditor(m_entryCard);
    m_itemEdit->setPlaceholderText("Search Commodity / Stock Item...");
    m_itemEdit->setText("Basmati Rice 1121 Steam");

    m_gradeEdit = new QLineEdit(m_entryCard);
    m_gradeEdit->setPlaceholderText("e.g. Grade-A / Premium");

    m_bagsEdit = new QLineEdit(m_entryCard);
    m_bagsEdit->setPlaceholderText("Bags Quota");

    addField(grid, "Broker (Dalal):", m_brokerSearchBox, 1, 0);
    addField(grid, "Commodity:", m_itemEdit, 1, 1);
    addField(grid, "Grade:", m_gradeEdit, 1, 2);
    addField(grid, "Bags Count:", m_bagsEdit, 1, 3);

    // Row 2
    m_weightEdit = new QLineEdit(m_entryCard);
    m_weightEdit->setPlaceholderText("Quintals");

    m_rateEdit = new QLineEdit(m_entryCard);
    m_rateEdit->setPlaceholderText("₹ / Qtl");

    m_dalaliRateEdit = new QLineEdit(m_entryCard);
    m_dalaliRateEdit->setPlaceholderText("₹ / Qtl (e.g. 10.00)");

    m_dalaliPctEdit = new QLineEdit(m_entryCard);
    m_dalaliPctEdit->setPlaceholderText("or Dalali % (e.g. 1.0)");

    addField(grid, "Contract Weight (Qtl):", m_weightEdit, 2, 0);
    addField(grid, "Rate per Qtl (₹):", m_rateEdit, 2, 1);
    addField(grid, "Dalali Rate (₹/Qtl):", m_dalaliRateEdit, 2, 2);
    addField(grid, "Dalali Rate (%):", m_dalaliPctEdit, 2, 3);

    // Row 3
    m_deliveryFromEdit = new QLineEdit(m_entryCard);
    m_deliveryFromEdit->setText(QDate::currentDate().toString("yyyy-MM-dd"));

    m_deliveryToEdit = new QLineEdit(m_entryCard);
    m_deliveryToEdit->setText(QDate::currentDate().addDays(15).toString("yyyy-MM-dd"));

    m_notesEdit = new QLineEdit(m_entryCard);
    m_notesEdit->setPlaceholderText("Moisture <= 12%, Payment within 10 days...");

    m_btnSaveSauda = new QPushButton("Book Sauda Contract (F12)", m_entryCard);
    m_btnSaveSauda->setStyleSheet("background-color: #16A34A; color: #FFFFFF; font-weight: 800; font-size: 12px; padding: 7px 16px; border-radius: 6px;");
    connect(m_btnSaveSauda, &QPushButton::clicked, this, &SaudaContractWidget::onSaveSauda);

    addField(grid, "Delivery From:", m_deliveryFromEdit, 3, 0);
    addField(grid, "Delivery To:", m_deliveryToEdit, 3, 1);

    auto* nLay = new QHBoxLayout();
    nLay->setSpacing(10);
    auto* nb = new QVBoxLayout();
    nb->setSpacing(2);
    auto* nL = new QLabel("Condition / Terms:");
    nL->setProperty("class", "fieldLabel");
    nb->addWidget(nL);
    nb->addWidget(m_notesEdit);
    nLay->addLayout(nb, 2);

    auto* sb = new QVBoxLayout();
    sb->setSpacing(2);
    sb->addWidget(new QLabel(" "));
    sb->addWidget(m_btnSaveSauda);
    nLay->addLayout(sb, 1);

    grid->addLayout(nLay, 3, 2, 1, 2);

    formLayout->addLayout(grid);
    m_entryCard->setVisible(false);
    mainLayout->addWidget(m_entryCard);

    // ========================================================================
    // 4. MAIN TAB WIDGET
    // ========================================================================
    m_tabWidget = new QTabWidget(this);

    // ------------------------------------------------------------------------
    // TAB 1: SAUDA CONTRACTS REGISTER
    // ------------------------------------------------------------------------
    auto* tab1 = new QWidget(this);
    auto* tab1Layout = new QVBoxLayout(tab1);
    tab1Layout->setContentsMargins(8, 10, 8, 8);
    tab1Layout->setSpacing(8);

    auto* filterCard = new QFrame(tab1);
    filterCard->setStyleSheet(
        "QFrame { background-color: #FFFFFF; border: 1px solid #CBD5E1; border-radius: 8px; }"
        "QLabel { color: #475569; font-weight: 700; font-size: 11.5px; border: none; background: transparent; }"
        "AccountingDateEdit, QDateEdit, QLineEdit, QComboBox { background-color: #FFFFFF; border: 1.5px solid #CBD5E1; border-radius: 6px; padding: 4px 8px; font-size: 12px; color: #0F172A; font-weight: 600; }"
        "AccountingDateEdit:focus, QDateEdit:focus, QLineEdit:focus, QComboBox:focus { border-color: #2563EB; background-color: #F8FAFC; }"
    );
    auto* fLay = new QHBoxLayout(filterCard);
    fLay->setContentsMargins(12, 8, 12, 8);
    fLay->setSpacing(12);

    fLay->addWidget(new QLabel("Type:", filterCard));
    m_filterTypeCombo = new QComboBox(filterCard);
    m_filterTypeCombo->addItems({"ALL", "SALE", "BUY"});
    connect(m_filterTypeCombo, &QComboBox::currentTextChanged, this, &SaudaContractWidget::onFilterChanged);
    fLay->addWidget(m_filterTypeCombo);

    fLay->addWidget(new QLabel("Status:", filterCard));
    m_filterStatusCombo = new QComboBox(filterCard);
    m_filterStatusCombo->addItems({"ALL", "PENDING", "PARTIAL", "FULFILLED"});
    connect(m_filterStatusCombo, &QComboBox::currentTextChanged, this, &SaudaContractWidget::onFilterChanged);
    fLay->addWidget(m_filterStatusCombo);

    fLay->addWidget(new QLabel("Search:", filterCard));
    m_searchSaudaEdit = new QLineEdit(filterCard);
    m_searchSaudaEdit->setPlaceholderText("Search Sauda No, party, broker, commodity...");
    m_searchSaudaEdit->setClearButtonEnabled(true);
    connect(m_searchSaudaEdit, &QLineEdit::textChanged, this, &SaudaContractWidget::populateSaudaTable);
    fLay->addWidget(m_searchSaudaEdit, 1);

    auto* btnFulfill = new QPushButton("Fulfill Selected Sauda", filterCard);
    btnFulfill->setStyleSheet("background-color: #EFF6FF; color: #2563EB; border: 1px solid #BFDBFE; border-radius: 4px; padding: 4px 10px; font-weight: 700; font-size: 11px;");
    connect(btnFulfill, &QPushButton::clicked, this, &SaudaContractWidget::onFulfillSaudaSelected);
    fLay->addWidget(btnFulfill);

    tab1Layout->addWidget(filterCard);

    m_saudaTable = new QTableWidget(tab1);
    m_saudaTable->setColumnCount(14);
    m_saudaTable->setHorizontalHeaderLabels({
        "Sauda No", "Date", "Type", "Party Name", "Broker (Dalal)", "Commodity",
        "Grade", "Bags", "Contract Wt (Qtl)", "Rate (₹)", "Dalali Rate", "Fulfilled (Qtl)", "Pending (Qtl)", "Status"
    });
    m_saudaTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_saudaTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_saudaTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    m_saudaTable->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Stretch);
    m_saudaTable->horizontalHeader()->setSectionResizeMode(4, QHeaderView::ResizeToContents);
    for (int c = 5; c < 14; ++c) {
        m_saudaTable->horizontalHeader()->setSectionResizeMode(c, QHeaderView::ResizeToContents);
    }
    m_saudaTable->verticalHeader()->setVisible(false);
    m_saudaTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_saudaTable->setSelectionMode(QAbstractItemView::SingleSelection);
    m_saudaTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_saudaTable->setAlternatingRowColors(true);
    tab1Layout->addWidget(m_saudaTable);

    m_tabWidget->addTab(tab1, "Sauda Forward Contracts");

    // ------------------------------------------------------------------------
    // TAB 2: DALALI SETTLEMENTS & DOUBLE-ENTRY JV
    // ------------------------------------------------------------------------
    auto* tab2 = new QWidget(this);
    auto* tab2Layout = new QVBoxLayout(tab2);
    tab2Layout->setContentsMargins(8, 10, 8, 8);
    tab2Layout->setSpacing(8);

    auto* dFilterCard = new QFrame(tab2);
    dFilterCard->setStyleSheet(
        "QFrame { background-color: #FFFFFF; border: 1px solid #CBD5E1; border-radius: 8px; }"
        "QLabel { color: #475569; font-weight: 700; font-size: 11.5px; border: none; background: transparent; }"
        "QLineEdit { background-color: #FFFFFF; border: 1.5px solid #CBD5E1; border-radius: 6px; padding: 4px 8px; font-size: 12px; color: #0F172A; font-weight: 600; }"
        "QLineEdit:focus { border-color: #2563EB; background-color: #F8FAFC; }"
    );
    auto* dFLay = new QHBoxLayout(dFilterCard);
    dFLay->setContentsMargins(12, 8, 12, 8);
    dFLay->setSpacing(12);

    dFLay->addWidget(new QLabel("Search Broker:", dFilterCard));
    m_searchDalaliBox = new AccountSearchBox(dFilterCard);
    m_searchDalaliBox->setPlaceholderText("Filter by broker name...");
    connect(m_searchDalaliBox, &AccountSearchBox::partySelected, this, [this](const QString& /*pName*/) {
        populateDalaliTable();
    });
    connect(m_searchDalaliBox, &QLineEdit::textChanged, this, &SaudaContractWidget::populateDalaliTable);
    dFLay->addWidget(m_searchDalaliBox, 1);

    auto* btnSettle = new QPushButton("+ Settle Dalali on Dispatch", dFilterCard);
    btnSettle->setStyleSheet("background-color: #16A34A; color: #FFFFFF; border: none; border-radius: 4px; padding: 4px 12px; font-weight: 700; font-size: 11px;");
    connect(btnSettle, &QPushButton::clicked, this, &SaudaContractWidget::onSettleDalali);
    dFLay->addWidget(btnSettle);

    auto* btnPostJv = new QPushButton("Post Selected to JV", dFilterCard);
    btnPostJv->setStyleSheet("background-color: #2563EB; color: #FFFFFF; border: none; border-radius: 4px; padding: 4px 12px; font-weight: 700; font-size: 11px;");
    connect(btnPostJv, &QPushButton::clicked, this, &SaudaContractWidget::onPostJournalVoucher);
    dFLay->addWidget(btnPostJv);

    tab2Layout->addWidget(dFilterCard);

    m_dalaliTable = new QTableWidget(tab2);
    m_dalaliTable->setColumnCount(12);
    m_dalaliTable->setHorizontalHeaderLabels({
        "Settlement No", "Date", "Broker (Dalal)", "Sauda Ref", "Voucher/Inv",
        "Dispatched (Qtl)", "Rate", "Gross Dalali (₹)", "TDS 194-H (₹)", "Net Payable (₹)", "JV Status", "JV Voucher No"
    });
    m_dalaliTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_dalaliTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_dalaliTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);
    for (int c = 3; c < 12; ++c) {
        m_dalaliTable->horizontalHeader()->setSectionResizeMode(c, QHeaderView::ResizeToContents);
    }
    m_dalaliTable->verticalHeader()->setVisible(false);
    m_dalaliTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_dalaliTable->setSelectionMode(QAbstractItemView::SingleSelection);
    m_dalaliTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_dalaliTable->setAlternatingRowColors(true);
    tab2Layout->addWidget(m_dalaliTable);

    m_tabWidget->addTab(tab2, "Brokerage (Dalali) Settlement & JV Postings");

    mainLayout->addWidget(m_tabWidget, 1);
}

void SaudaContractWidget::toggleEntryForm() {
    if (m_entryCard) {
        bool nowVisible = !m_entryCard->isVisible();
        m_entryCard->setVisible(nowVisible);
        if (nowVisible) {
            m_saudaNoEdit->setText(m_controller->generateNextSaudaNo());
            m_partySearchBox->setFocus();
        }
    }
}

void SaudaContractWidget::keyPressEvent(QKeyEvent* event) {
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
            onSaveSauda();
        }
    } else {
        QWidget::keyPressEvent(event);
    }
}

void SaudaContractWidget::refreshData() {
    populateSaudaTable();
    populateDalaliTable();
    updateSummaryKpis();
    m_isDirty = false;
}

void SaudaContractWidget::updateSummaryKpis() {
    QVariantList saudas = m_controller->getSaudaContracts();
    double pendingVol = 0.0;
    double fulfilledVol = 0.0;
    double totalValue = 0.0;

    for (const auto& rowVar : saudas) {
        QVariantMap row = rowVar.toMap();
        double cWt = row.value("contracted_weight_qtl").toDouble();
        double fWt = row.value("fulfilled_weight_qtl").toDouble();
        double rate = row.value("rate_per_qtl").toDouble();

        fulfilledVol += fWt;
        double pWt = cWt - fWt;
        if (pWt > 0) pendingVol += pWt;
        totalValue += (cWt * rate);
    }

    QVariantList dalalis = m_controller->getDalaliSettlements();
    double unpostedDalali = 0.0;
    for (const auto& rowVar : dalalis) {
        QVariantMap row = rowVar.toMap();
        if (row.value("is_posted_to_jv").toInt() == 0) {
            unpostedDalali += row.value("net_dalali_payable").toDouble();
        }
    }

    if (m_lblPendingVolume) m_lblPendingVolume->setText(QString("%1 Qtl").arg(QString::number(pendingVol, 'f', 2)));
    if (m_lblFulfilledVolume) m_lblFulfilledVolume->setText(QString("%1 Qtl").arg(QString::number(fulfilledVol, 'f', 2)));
    if (m_lblTotalValue) m_lblTotalValue->setText(QString("₹%1").arg(QLocale(QLocale::English).toString(totalValue, 'f', 2)));
    if (m_lblUnpostedDalali) m_lblUnpostedDalali->setText(QString("₹%1").arg(QLocale(QLocale::English).toString(unpostedDalali, 'f', 2)));
}

void SaudaContractWidget::populateSaudaTable() {
    QString typeFilter = (m_filterTypeCombo && m_filterTypeCombo->currentText() != "ALL") ? m_filterTypeCombo->currentText() : "";
    QString statusFilter = (m_filterStatusCombo && m_filterStatusCombo->currentText() != "ALL") ? m_filterStatusCombo->currentText() : "";
    QString search = m_searchSaudaEdit ? m_searchSaudaEdit->text().trimmed().toLower() : "";

    QVariantList saudas = m_controller->getSaudaContracts(typeFilter, statusFilter);
    m_saudaTable->setRowCount(0);

    for (const auto& rowVar : saudas) {
        QVariantMap row = rowVar.toMap();
        QString sNo = row.value("sauda_no").toString();
        QString party = row.value("party_name").toString();
        QString broker = row.value("broker_name").toString();
        QString item = row.value("item_name").toString();

        if (!search.isEmpty()) {
            if (!sNo.toLower().contains(search) && !party.toLower().contains(search) &&
                !broker.toLower().contains(search) && !item.toLower().contains(search)) {
                continue;
            }
        }

        int r = m_saudaTable->rowCount();
        m_saudaTable->insertRow(r);

        auto createItem = [](const QString& text, Qt::Alignment align = Qt::AlignLeft | Qt::AlignVCenter) {
            auto* item = new QTableWidgetItem(text);
            item->setTextAlignment(align);
            return item;
        };

        double cWt = row.value("contracted_weight_qtl").toDouble();
        double fWt = row.value("fulfilled_weight_qtl").toDouble();
        double pWt = std::max(0.0, cWt - fWt);

        QString status = row.value("status").toString();
        auto* statusItem = createItem(status, Qt::AlignCenter);
        statusItem->setFont(QFont("", -1, QFont::Bold));
        if (status == "PENDING") statusItem->setForeground(QColor("#2563EB"));
        else if (status == "PARTIAL") statusItem->setForeground(QColor("#D97706"));
        else statusItem->setForeground(QColor("#059669"));

        m_saudaTable->setItem(r, 0, createItem(sNo, Qt::AlignCenter));
        m_saudaTable->setItem(r, 1, createItem(row.value("sauda_date").toString(), Qt::AlignCenter));
        m_saudaTable->setItem(r, 2, createItem(row.value("sauda_type").toString(), Qt::AlignCenter));
        m_saudaTable->setItem(r, 3, createItem(party));
        m_saudaTable->setItem(r, 4, createItem(broker));
        m_saudaTable->setItem(r, 5, createItem(item));
        m_saudaTable->setItem(r, 6, createItem(row.value("grade").toString()));
        m_saudaTable->setItem(r, 7, createItem(QString::number(row.value("contracted_bags").toInt()), Qt::AlignRight | Qt::AlignVCenter));
        m_saudaTable->setItem(r, 8, createItem(QString::number(cWt, 'f', 2), Qt::AlignRight | Qt::AlignVCenter));
        m_saudaTable->setItem(r, 9, createItem(QString("₹%1").arg(QString::number(row.value("rate_per_qtl").toDouble(), 'f', 2)), Qt::AlignRight | Qt::AlignVCenter));
        
        double dRate = row.value("dalali_rate_per_qtl").toDouble();
        double dPct = row.value("dalali_pct").toDouble();
        QString dStr = (dRate > 0) ? QString("₹%1/Q").arg(dRate) : QString("%1%").arg(dPct);
        m_saudaTable->setItem(r, 10, createItem(dStr, Qt::AlignCenter));

        m_saudaTable->setItem(r, 11, createItem(QString::number(fWt, 'f', 2), Qt::AlignRight | Qt::AlignVCenter));
        m_saudaTable->setItem(r, 12, createItem(QString::number(pWt, 'f', 2), Qt::AlignRight | Qt::AlignVCenter));
        m_saudaTable->setItem(r, 13, statusItem);
    }
}

void SaudaContractWidget::populateDalaliTable() {
    QVariantList dalalis = m_controller->getDalaliSettlements();
    QString search = m_searchDalaliBox ? m_searchDalaliBox->text().trimmed().toLower() : "";
    m_dalaliTable->setRowCount(0);

    for (const auto& rowVar : dalalis) {
        QVariantMap row = rowVar.toMap();
        QString sNo = row.value("settlement_no").toString();
        QString broker = row.value("broker_name").toString();

        if (!search.isEmpty()) {
            if (!sNo.toLower().contains(search) && !broker.toLower().contains(search)) {
                continue;
            }
        }

        int r = m_dalaliTable->rowCount();
        m_dalaliTable->insertRow(r);

        auto createItem = [](const QString& text, Qt::Alignment align = Qt::AlignLeft | Qt::AlignVCenter) {
            auto* item = new QTableWidgetItem(text);
            item->setTextAlignment(align);
            return item;
        };

        int isPosted = row.value("is_posted_to_jv").toInt();
        auto* jvItem = createItem(isPosted ? "POSTED" : "UNPOSTED", Qt::AlignCenter);
        jvItem->setFont(QFont("", -1, QFont::Bold));
        if (isPosted) jvItem->setForeground(QColor("#059669"));
        else jvItem->setForeground(QColor("#DC2626"));

        m_dalaliTable->setItem(r, 0, createItem(sNo, Qt::AlignCenter));
        m_dalaliTable->setItem(r, 1, createItem(row.value("settlement_date").toString(), Qt::AlignCenter));
        m_dalaliTable->setItem(r, 2, createItem(broker));
        m_dalaliTable->setItem(r, 3, createItem(row.value("sauda_no").toString(), Qt::AlignCenter));
        m_dalaliTable->setItem(r, 4, createItem(row.value("voucher_no").toString(), Qt::AlignCenter));
        m_dalaliTable->setItem(r, 5, createItem(QString::number(row.value("weight_qtl").toDouble(), 'f', 2), Qt::AlignRight | Qt::AlignVCenter));
        m_dalaliTable->setItem(r, 6, createItem(QString::number(row.value("dalali_rate").toDouble(), 'f', 2), Qt::AlignRight | Qt::AlignVCenter));
        m_dalaliTable->setItem(r, 7, createItem(QString("₹%1").arg(QString::number(row.value("dalali_amount").toDouble(), 'f', 2)), Qt::AlignRight | Qt::AlignVCenter));
        m_dalaliTable->setItem(r, 8, createItem(QString("₹%1").arg(QString::number(row.value("tds_amount").toDouble(), 'f', 2)), Qt::AlignRight | Qt::AlignVCenter));
        m_dalaliTable->setItem(r, 9, createItem(QString("₹%1").arg(QString::number(row.value("net_dalali_payable").toDouble(), 'f', 2)), Qt::AlignRight | Qt::AlignVCenter));
        m_dalaliTable->setItem(r, 10, jvItem);
        m_dalaliTable->setItem(r, 11, createItem(row.value("journal_voucher_no").toString(), Qt::AlignCenter));
    }
}

void SaudaContractWidget::onSaveSauda() {
    QString party = m_partySearchBox ? m_partySearchBox->currentPartyName().trimmed() : "";
    if (party.isEmpty() && m_partySearchBox) party = m_partySearchBox->text().trimmed();
    if (party.isEmpty()) {
        CustomMessageBox::critical(this, "Validation Error", "Please select or enter party / buyer name.");
        if (m_partySearchBox) m_partySearchBox->setFocus();
        return;
    }
    int partyId = m_partySearchBox ? m_partySearchBox->selectedPartyId() : 0;

    double weight = m_weightEdit->text().toDouble();
    if (weight <= 0) {
        CustomMessageBox::critical(this, "Validation Error", "Contracted weight must be greater than zero.");
        m_weightEdit->setFocus();
        return;
    }

    double rate = m_rateEdit->text().toDouble();
    if (rate <= 0) {
        CustomMessageBox::critical(this, "Validation Error", "Rate per Quintal must be greater than zero.");
        m_rateEdit->setFocus();
        return;
    }

    QString sNo = m_saudaNoEdit->text().trimmed();
    QString sDate = m_dateEdit->date().toString("yyyy-MM-dd");
    QString sType = m_typeCombo->currentText();
    QString broker = m_brokerSearchBox ? m_brokerSearchBox->currentPartyName().trimmed() : "";
    if (broker.isEmpty() && m_brokerSearchBox) broker = m_brokerSearchBox->text().trimmed();
    int brokerId = m_brokerSearchBox ? m_brokerSearchBox->selectedPartyId() : 0;

    QString item = m_itemEdit ? m_itemEdit->text().trimmed() : "";
    if (item.isEmpty()) item = "Basmati Rice 1121 Steam";
    QString grade = m_gradeEdit->text().trimmed();
    int bags = m_bagsEdit->text().toInt();
    double dRate = m_dalaliRateEdit->text().toDouble();
    double dPct = m_dalaliPctEdit->text().toDouble();
    QString dFrom = m_deliveryFromEdit->text().trimmed();
    QString dTo = m_deliveryToEdit->text().trimmed();
    QString notes = m_notesEdit->text().trimmed();

    bool ok = m_controller->createSaudaContract(
        sNo, sDate, sType, partyId, party, brokerId, broker, 0, item, grade,
        bags, weight, rate, dRate, dPct, dFrom, dTo, "Immediate", notes
    );

    if (ok) {
        CustomMessageBox::information(this, "Sauda Booked", QString("Sauda contract %1 booked successfully!").arg(sNo));
        if (m_partySearchBox) m_partySearchBox->clearParty();
        if (m_brokerSearchBox) m_brokerSearchBox->clearParty();
        m_gradeEdit->clear();
        m_bagsEdit->clear();
        m_weightEdit->clear();
        m_rateEdit->clear();
        m_dalaliRateEdit->clear();
        m_dalaliPctEdit->clear();
        m_notesEdit->clear();
        m_entryCard->setVisible(false);
        refreshData();
    } else {
        CustomMessageBox::critical(this, "Error", "Failed to book Sauda contract.");
    }
}

void SaudaContractWidget::onFulfillSaudaSelected() {
    int r = m_saudaTable->currentRow();
    if (r < 0) {
        CustomMessageBox::information(this, "Selection Required", "Please select a Sauda row to record fulfillment.");
        return;
    }

    QString sNo = m_saudaTable->item(r, 0)->text();
    QVariantList list = m_controller->getSaudaContracts();
    int targetId = 0;
    for (const auto& rowVar : list) {
        QVariantMap row = rowVar.toMap();
        if (row.value("sauda_no").toString() == sNo) {
            targetId = row.value("id").toInt();
            break;
        }
    }

    if (targetId <= 0) return;

    bool ok = false;
    double weight = QInputDialog::getDouble(this, "Record Fulfillment", "Dispatched / Fulfilled Weight (Quintals):", 100.0, 0.01, 100000.0, 2, &ok);
    if (ok && weight > 0) {
        int bags = QInputDialog::getInt(this, "Record Bags", "Dispatched Bags Count:", static_cast<int>(weight * 2), 0, 100000, 1, &ok);
        if (ok) {
            if (m_controller->fulfillSauda(targetId, bags, weight)) {
                CustomMessageBox::information(this, "Fulfillment Recorded", QString("Fulfillment of %1 Qtl recorded!").arg(weight));
                refreshData();
            }
        }
    }
}

void SaudaContractWidget::onSettleDalali() {
    bool ok = false;
    QString broker = QInputDialog::getText(this, "Settle Dalali", "Broker (Dalal) Name:", QLineEdit::Normal, "", &ok);
    if (!ok || broker.trimmed().isEmpty()) return;

    QString sNo = QInputDialog::getText(this, "Settle Dalali", "Sauda No (Optional):", QLineEdit::Normal, "", &ok);
    double weight = QInputDialog::getDouble(this, "Settle Dalali", "Fulfilled Weight (Quintals):", 500.0, 0.01, 100000.0, 2, &ok);
    if (!ok || weight <= 0) return;

    double dRate = QInputDialog::getDouble(this, "Settle Dalali", "Dalali Rate (₹ per Qtl):", 10.0, 0.0, 1000.0, 2, &ok);
    if (!ok) return;

    double tdsPct = 5.0; // Standard 194-H TDS rate

    QString settDate = QDate::currentDate().toString("yyyy-MM-dd");
    bool created = m_controller->createDalaliSettlement(
        0, broker.trimmed(), 0, sNo.trimmed(), settDate, "SALE", "VCH-AUTO", "INV-AUTO",
        "Party", "Basmati Rice", weight, 6500.0, dRate, tdsPct, "Brokerage settlement on dispatched volume"
    );

    if (created) {
        CustomMessageBox::information(this, "Settlement Created", "Dalali settlement generated! You can now post it to JV.");
        refreshData();
        m_tabWidget->setCurrentIndex(1);
    }
}

void SaudaContractWidget::onPostJournalVoucher() {
    int r = m_dalaliTable->currentRow();
    if (r < 0) {
        CustomMessageBox::information(this, "Selection Required", "Please select a Dalali settlement row to post to Journal Voucher.");
        return;
    }

    QString sNo = m_dalaliTable->item(r, 0)->text();
    QVariantList list = m_controller->getDalaliSettlements();
    int targetId = 0;
    for (const auto& rowVar : list) {
        QVariantMap row = rowVar.toMap();
        if (row.value("settlement_no").toString() == sNo) {
            targetId = row.value("id").toInt();
            break;
        }
    }

    if (targetId <= 0) return;

    if (CustomMessageBox::question(this, "Confirm Double-Entry Posting", "Post this brokerage settlement to Double-Entry Journal Voucher (Debit Brokerage Expense, Credit Broker & TDS 194H)?")) {
        if (m_controller->postDalaliToJournalVoucher(targetId)) {
            CustomMessageBox::information(this, "Journal Voucher Posted", "Double-Entry JV generated and transactions posted successfully!");
            refreshData();
        } else {
            CustomMessageBox::critical(this, "Error", "Failed to post Journal Voucher.");
        }
    }
}

void SaudaContractWidget::onFilterChanged() {
    populateSaudaTable();
}

void SaudaContractWidget::onExportPdf() {
    if (!m_printCtrl) return;
    QString html = "<h2>Sauda Forward Contracts Register</h2><table border='1' cellspacing='0' cellpadding='5'><tr>"
                   "<th>Sauda No</th><th>Date</th><th>Type</th><th>Party</th><th>Broker</th><th>Commodity</th><th>Contract Qtl</th><th>Rate</th><th>Fulfilled Qtl</th><th>Status</th></tr>";

    for (int r = 0; r < m_saudaTable->rowCount(); ++r) {
        html += QString("<tr><td>%1</td><td>%2</td><td>%3</td><td>%4</td><td>%5</td><td>%6</td><td>%7</td><td>%8</td><td>%9</td><td>%10</td></tr>")
                .arg(m_saudaTable->item(r, 0)->text())
                .arg(m_saudaTable->item(r, 1)->text())
                .arg(m_saudaTable->item(r, 2)->text())
                .arg(m_saudaTable->item(r, 3)->text())
                .arg(m_saudaTable->item(r, 4)->text())
                .arg(m_saudaTable->item(r, 5)->text())
                .arg(m_saudaTable->item(r, 8)->text())
                .arg(m_saudaTable->item(r, 9)->text())
                .arg(m_saudaTable->item(r, 11)->text())
                .arg(m_saudaTable->item(r, 13)->text());
    }
    html += "</table>";
    m_printCtrl->exportHtmlToPdf(html, "Sauda_Contracts_Register.pdf");
}

void SaudaContractWidget::onExportCsv() {
    QString fileName = QFileDialog::getSaveFileName(this, "Export Sauda Register CSV", "Sauda_Register.csv", "CSV Files (*.csv)");
    if (fileName.isEmpty()) return;

    QFile file(fileName);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&file);
        out << "Sauda No,Date,Type,Party,Broker,Commodity,Grade,Bags,Contract Wt,Rate,Dalali,Fulfilled Wt,Pending Wt,Status\n";
        for (int r = 0; r < m_saudaTable->rowCount(); ++r) {
            QStringList cells;
            for (int c = 0; c < m_saudaTable->columnCount(); ++c) {
                cells << QString("\"%1\"").arg(m_saudaTable->item(r, c) ? m_saudaTable->item(r, c)->text() : "");
            }
            out << cells.join(",") << "\n";
        }
        file.close();
        CustomMessageBox::information(this, "Export Complete", "CSV export completed successfully.");
    }
}
