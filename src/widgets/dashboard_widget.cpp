#include "dashboard_widget.h"
#include "accounting_period_dialog.h"
#include "mdb_migration_dialog.h"
#include "../engine/accounting_engine.h"
#include "../database_manager.h"
#include "../models/menu_tree_manager.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QDate>
#include <QDebug>
#include <QScrollArea>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QFrame>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QEnterEvent>
#include <QTimer>
#include <QFileDialog>
#include <QDir>
#include <QFile>

DashboardWidget::DashboardWidget(DashboardController* dashCtrl,
                                 FirmManager* firmMgr,
                                 PrintExportController* printExportCtrl,
                                 BahiKhataMigrator* migrator,
                                 QWidget* parent)
    : QWidget(parent)
    , m_dashCtrl(dashCtrl)
    , m_firmMgr(firmMgr)
    , m_printExportCtrl(printExportCtrl)
    , m_migrator(migrator)
{
    setupUi();
    if (m_dashCtrl) {
        connect(m_dashCtrl, &DashboardController::statsChanged, this, [this]() {
            if (m_paddyStockLabel) m_paddyStockLabel->setText(m_dashCtrl->paddyStock());
            if (m_riceStockLabel) m_riceStockLabel->setText(m_dashCtrl->riceStock());
            if (m_totalSalesLabel) m_totalSalesLabel->setText(m_dashCtrl->totalSales());
        });
    }
    if (m_firmMgr) {
        connect(m_firmMgr, &FirmManager::firmSwitched, this, [this](const QString&, const QString&) {
            refreshStats();
        });
    }

    connect(&MahadevERP::MenuTreeManager::instance(), &MahadevERP::MenuTreeManager::openViewRequested, this, [this](int viewIndex) {
        QString lastMenu = MahadevERP::MenuTreeManager::instance().lastTriggeredMenuId();
        if (lastMenu == "ledger_master") m_lastOpenedMenuIndex = 0;
        else if (lastMenu == "stock_master") m_lastOpenedMenuIndex = 1;
        else if (lastMenu == "add_voucher") m_lastOpenedMenuIndex = 2;
        else if (lastMenu == "other_voucher" || lastMenu == "tds_tcs_hub" || lastMenu == "tds_options" || lastMenu == "tcs_options") m_lastOpenedMenuIndex = 3;
        else if (lastMenu == "reports_register") m_lastOpenedMenuIndex = 4;
        m_lastSubmenuSelectedIndex = MahadevERP::MenuTreeManager::instance().lastTriggeredSubmenuIndex();
        m_reopenSubmenuOnReturn = true;
        emit openViewRequested(viewIndex);
    });

    refreshStats();
}

void DashboardWidget::setControllers(DashboardController* dashCtrl, FirmManager* firmMgr, BahiKhataMigrator* migrator) {
    m_dashCtrl = dashCtrl;
    m_firmMgr = firmMgr;
    if (migrator) m_migrator = migrator;
    if (m_dashCtrl) {
        connect(m_dashCtrl, &DashboardController::statsChanged, this, [this]() {
            if (m_paddyStockLabel) m_paddyStockLabel->setText(m_dashCtrl->paddyStock());
            if (m_riceStockLabel) m_riceStockLabel->setText(m_dashCtrl->riceStock());
            if (m_totalSalesLabel) m_totalSalesLabel->setText(m_dashCtrl->totalSales());
        });
    }
    if (m_firmMgr) {
        connect(m_firmMgr, &FirmManager::firmSwitched, this, [this](const QString&, const QString&) {
            refreshStats();
        });
    }
    refreshStats();
}

void DashboardWidget::onSyncClicked() {
    if (!m_firmMgr) return;
    QVariantMap firmInfo = m_firmMgr->currentFirmInfo();
    QString fullPath = firmInfo.value("full_path").toString();
    if (fullPath.isEmpty()) {
        QString src = firmInfo.value("source_file").toString();
        if (!src.isEmpty()) {
            fullPath = m_firmMgr->activeFolder() + "/" + src;
        }
    }
    if (fullPath.isEmpty() || !QFile::exists(fullPath)) {
        QString defaultFirmDir = m_firmMgr ? m_firmMgr->get_app_data_folder() : QDir::current().filePath("data");
        QDir firmDir(defaultFirmDir);
        if (firmDir.exists()) {
            for (const auto& f : firmDir.entryInfoList({"Data.*", "data.*", "DATA.*", "*.mdb", "*.accdb", "*.0*"}, QDir::Files)) {
                if (f.fileName().endsWith(".ldb", Qt::CaseInsensitive) || f.fileName().endsWith(".bak", Qt::CaseInsensitive)) continue;
                fullPath = f.absoluteFilePath();
                break;
            }
        }
        if (fullPath.isEmpty() || !QFile::exists(fullPath)) {
            fullPath = QFileDialog::getOpenFileName(this, "Select Bahi-Khata Data File to Sync", defaultFirmDir, "Bahi-Khata Databases (Data.* *.0* *.mdb *.accdb);;All Files (*.*)");
        }
    }

    if (!fullPath.isEmpty() && QFile::exists(fullPath)) {
        QString firmName = m_firmMgr->currentFirmName();
        QString firmId = m_firmMgr->currentFirmId();
        MdbMigrationDialog dlg(m_migrator, m_firmMgr, fullPath, firmName, firmId, this);
        connect(&dlg, &MdbMigrationDialog::migrationCompleted, this, [this](const QString&, const QString&) {
            refreshStats();
        });
        dlg.exec();
        refreshStats();
    }
}

void DashboardWidget::setupUi() {
    setAttribute(Qt::WA_StyledBackground, true);
    setStyleSheet("DashboardWidget { background-color: #F8FAFC; } QLabel { border: none; background: transparent; }");

    QVBoxLayout* rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(16, 14, 16, 14);
    rootLayout->setSpacing(14);

    // ========================================================================
    // 1. TOP ACTION & HEADER BAR (Matching DashboardView.qml lines 144-220)
    // ========================================================================
    QHBoxLayout* topHeaderLayout = new QHBoxLayout();
    topHeaderLayout->setSpacing(12);

    QVBoxLayout* titleBox = new QVBoxLayout();
    titleBox->setSpacing(2);
    QLabel* mainTitle = new QLabel("Executive Accounting Dashboard", this);
    mainTitle->setStyleSheet("font-size: 22px; font-weight: 800; color: #0F172A; font-family: 'Segoe UI', -apple-system, BlinkMacSystemFont, Roboto, 'Helvetica Neue', 'Noto Sans', 'Liberation Sans', Arial, sans-serif; border: none; background: transparent;");
    titleBox->addWidget(mainTitle);

    QLabel* subTitle = new QLabel("Mahadev Rice Milling ERP & Financial Control System", this);
    subTitle->setStyleSheet("font-size: 12px; font-weight: 500; color: #64748B; border: none; background: transparent;");
    titleBox->addWidget(subTitle);
    topHeaderLayout->addLayout(titleBox);

    topHeaderLayout->addStretch(1);

    // Header Buttons
    QHBoxLayout* btnBox = new QHBoxLayout();
    btnBox->setSpacing(8);

    // Open Firm Button (Alt+F1)
    m_openFirmBtn = new QPushButton("Open Firm  [Alt+F1]", this);
    m_openFirmBtn->setFixedHeight(36);
    m_openFirmBtn->setFocusPolicy(Qt::NoFocus);
    m_openFirmBtn->setCursor(Qt::PointingHandCursor);
    m_openFirmBtn->setStyleSheet(
        "QPushButton { background-color: #FFFFFF; border: 1.5px solid #CBD5E1; border-radius: 6px; padding: 0px 14px; font-weight: 700; color: #334155; font-size: 12px; }"
        "QPushButton:hover { background-color: #F1F5F9; border-color: #94A3B8; }"
    );
    connect(m_openFirmBtn, &QPushButton::clicked, this, [this]() { emit openViewRequested(22); });
    btnBox->addWidget(m_openFirmBtn);

    // Sync Bahi-Khata Data Button
    m_syncBtn = new QPushButton("Sync Data", this);
    m_syncBtn->setFixedHeight(36);
    m_syncBtn->setFocusPolicy(Qt::NoFocus);
    m_syncBtn->setCursor(Qt::PointingHandCursor);
    m_syncBtn->setStyleSheet(
        "QPushButton { background-color: #EFF6FF; border: 1.5px solid #3B82F6; border-radius: 6px; padding: 0px 14px; font-weight: 700; color: #1D4ED8; font-size: 12px; }"
        "QPushButton:hover { background-color: #DBEAFE; border-color: #2563EB; }"
    );
    connect(m_syncBtn, &QPushButton::clicked, this, &DashboardWidget::onSyncClicked);
    btnBox->addWidget(m_syncBtn);

    // Period Button
    m_periodBtn = new QPushButton(this);
    m_periodBtn->setFixedHeight(36);
    m_periodBtn->setFocusPolicy(Qt::NoFocus);
    m_periodBtn->setCursor(Qt::PointingHandCursor);
    m_periodBtn->setStyleSheet(
        "QPushButton { background-color: #F0FDF4; border: 1.5px solid #16A34A; border-radius: 6px; padding: 0px 14px; font-weight: 700; color: #15803D; font-size: 12px; }"
        "QPushButton:hover { background-color: #DCFCE7; }"
    );
    connect(m_periodBtn, &QPushButton::clicked, this, &DashboardWidget::requestAccountingPeriodDialog);
    btnBox->addWidget(m_periodBtn);

    // New Paddy Slip Button (with F2 badge)
    m_newPaddyBtn = new QPushButton("New Paddy Slip  [F2]", this);
    m_newPaddyBtn->setFixedHeight(36);
    m_newPaddyBtn->setFocusPolicy(Qt::NoFocus);
    m_newPaddyBtn->setCursor(Qt::PointingHandCursor);
    m_newPaddyBtn->setStyleSheet(
        "QPushButton { background-color: #16A34A; border: none; border-radius: 6px; padding: 0px 16px; font-weight: 700; color: #FFFFFF; font-size: 12px; }"
        "QPushButton:hover { background-color: #15803D; }"
    );
    connect(m_newPaddyBtn, &QPushButton::clicked, this, [this]() { emit openViewRequested(1); });
    btnBox->addWidget(m_newPaddyBtn);

    // New Invoice Button
    m_newInvoiceBtn = new QPushButton("New Invoice", this);
    m_newInvoiceBtn->setFixedHeight(36);
    m_newInvoiceBtn->setFocusPolicy(Qt::NoFocus);
    m_newInvoiceBtn->setCursor(Qt::PointingHandCursor);
    m_newInvoiceBtn->setStyleSheet(
        "QPushButton { background-color: #2563EB; border: none; border-radius: 6px; padding: 0px 18px; font-weight: 700; color: #FFFFFF; font-size: 12px; }"
        "QPushButton:hover { background-color: #1D4ED8; }"
    );
    connect(m_newInvoiceBtn, &QPushButton::clicked, this, [this]() { emit openViewRequested(14); });
    btnBox->addWidget(m_newInvoiceBtn);

    topHeaderLayout->addLayout(btnBox);
    rootLayout->addLayout(topHeaderLayout);

    // ========================================================================
    // 2. 3-SECTION HORIZONTAL LAYOUT (Left 1 : Middle 4 : Right 1)
    // ========================================================================
    QHBoxLayout* bodyLayout = new QHBoxLayout();
    bodyLayout->setSpacing(14);

    QWidget* leftSection = createLeftSection();
    QWidget* middleSection = createMiddleSection();
    QWidget* rightSection = createRightSection();

    bodyLayout->addWidget(leftSection, 1);
    bodyLayout->addWidget(middleSection, 4);
    bodyLayout->addWidget(rightSection, 1);

    rootLayout->addLayout(bodyLayout, 1);

    setFocusPolicy(Qt::StrongFocus);
    updateSelection(0);
}

// ============================================================================
// LEFT SECTION: FIRM PROFILE (1/6th Width)
// ============================================================================
QWidget* DashboardWidget::createLeftSection() {
    QFrame* card = new QFrame(this);
    card->setObjectName("firmProfileCard");
    card->setStyleSheet(
        "#firmProfileCard { background-color: #FFFFFF; border: 1px solid #E2E8F0; border-radius: 10px; }"
        "QLabel { border: none; background: transparent; }"
    );
    QVBoxLayout* layout = new QVBoxLayout(card);
    layout->setContentsMargins(14, 14, 14, 14);
    layout->setSpacing(12);

    // Firm Profile Header
    QHBoxLayout* header = new QHBoxLayout();
    header->setSpacing(8);

    QLabel* iconBox = new QLabel("FP", card);
    iconBox->setObjectName("fpIconBox");
    iconBox->setFixedSize(34, 34);
    iconBox->setAlignment(Qt::AlignCenter);
    iconBox->setStyleSheet("#fpIconBox { background-color: #EFF6FF; border: 1px solid #BFDBFE; border-radius: 8px; color: #2563EB; font-size: 12px; font-weight: 800; }");
    header->addWidget(iconBox);

    QVBoxLayout* nameBox = new QVBoxLayout();
    nameBox->setSpacing(0);
    QLabel* tag = new QLabel("FIRM PROFILE", card);
    tag->setStyleSheet("color: #2563EB; font-size: 10px; font-weight: 800; letter-spacing: 1px; border: none; background: transparent;");
    nameBox->addWidget(tag);

    m_firmNameLabel = new QLabel("—", card);
    m_firmNameLabel->setWordWrap(true);
    m_firmNameLabel->setStyleSheet("color: #0F172A; font-size: 13px; font-weight: 800; border: none; background: transparent;");
    nameBox->addWidget(m_firmNameLabel);
    header->addLayout(nameBox, 1);
    layout->addLayout(header);

    // Divider
    QFrame* div = new QFrame(card);
    div->setFixedHeight(1);
    div->setStyleSheet("background-color: #F1F5F9; border: none;");
    layout->addWidget(div);

    // Financial Year Selected Badge
    QFrame* fyBox = new QFrame(card);
    fyBox->setObjectName("fyBadgeBox");
    fyBox->setFixedHeight(46);
    fyBox->setStyleSheet(
        "#fyBadgeBox { background-color: #F0FDF4; border: 1px solid #BBF7D0; border-radius: 8px; }"
        "QLabel { border: none; background: transparent; }"
    );
    QHBoxLayout* fyLayout = new QHBoxLayout(fyBox);
    fyLayout->setContentsMargins(10, 4, 10, 4);
    fyLayout->setSpacing(8);

    QLabel* fyIcon = new QLabel("FY", fyBox);
    fyIcon->setStyleSheet("color: #16A34A; font-size: 11px; font-weight: 800; border: none; background: transparent;");
    fyLayout->addWidget(fyIcon);

    m_fyBadgeLabel = new QLabel(FiscalYearHelper::getActiveFiscalYear().name + " (Active)", fyBox);
    m_fyBadgeLabel->setWordWrap(true);
    m_fyBadgeLabel->setStyleSheet("color: #15803D; font-size: 11px; font-weight: 800; border: none; background: transparent;");
    fyLayout->addWidget(m_fyBadgeLabel, 1);
    layout->addWidget(fyBox);

    // Details List
    auto makeField = [card, layout](const QString& title, QLabel*& valLabel, const QString& defaultVal) {
        QVBoxLayout* l = new QVBoxLayout();
        l->setSpacing(2);
        QLabel* t = new QLabel(title, card);
        t->setStyleSheet("color: #64748B; font-size: 11px; font-weight: 500; border: none; background: transparent;");
        l->addWidget(t);
        valLabel = new QLabel(defaultVal, card);
        valLabel->setWordWrap(true);
        valLabel->setStyleSheet("color: #0F172A; font-size: 12px; font-weight: 700; border: none; background: transparent;");
        l->addWidget(valLabel);
        layout->addLayout(l);
    };

    makeField("GSTIN Number:", m_gstinLabel, "—");
    makeField("PAN Number:", m_panLabel, "—");
    makeField("Statutory / License:", m_statutoryLabel, "—");
    makeField("Business Type:", m_businessTypeLabel, "—");
    makeField("Location:", m_locationLabel, "—");

    layout->addStretch(1);

    // Bottom Status Badge
    QFrame* statusCard = new QFrame(card);
    statusCard->setObjectName("statusBadgeCard");
    statusCard->setFixedHeight(30);
    statusCard->setStyleSheet(
        "#statusBadgeCard { background-color: #F8FAFC; border: 1px solid #E2E8F0; border-radius: 6px; }"
        "QLabel { border: none; background: transparent; }"
    );
    QHBoxLayout* stLayout = new QHBoxLayout(statusCard);
    stLayout->setContentsMargins(8, 2, 8, 2);
    stLayout->setSpacing(6);
    stLayout->setAlignment(Qt::AlignCenter);

    QLabel* dot = new QLabel(statusCard);
    dot->setObjectName("statusDot");
    dot->setFixedSize(8, 8);
    dot->setStyleSheet("#statusDot { background-color: #16A34A; border-radius: 4px; border: none; }");
    stLayout->addWidget(dot);

    QLabel* stText = new QLabel("GST Tax Registered", statusCard);
    stText->setStyleSheet("color: #334155; font-size: 11px; font-weight: 700; border: none; background: transparent;");
    stLayout->addWidget(stText);
    layout->addWidget(statusCard);

    return card;
}

// ============================================================================
// MIDDLE SECTION: METRICS & VERTICAL MENU (4/6th Width)
// ============================================================================
QWidget* DashboardWidget::createMiddleSection() {
    QWidget* container = new QWidget(this);
    QVBoxLayout* layout = new QVBoxLayout(container);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(14);

    // 1. Top Metrics Cards Strip (3 StatCards)
    QHBoxLayout* statsStrip = new QHBoxLayout();
    statsStrip->setSpacing(12);

    statsStrip->addWidget(createStatCard("Raw Paddy Stock", &m_paddyStockLabel, "In Godowns A & B", "#D97706"));
    statsStrip->addWidget(createStatCard("Finished Rice Stock", &m_riceStockLabel, "Ready for Dispatch", "#16A34A"));
    statsStrip->addWidget(createStatCard("Total Revenue", &m_totalSalesLabel, "Sales Invoices", "#2563EB"));
    layout->addLayout(statsStrip);

    // 2. Vertical Master & Voucher Menu Section
    QVBoxLayout* menuSection = new QVBoxLayout();
    menuSection->setSpacing(8);

    QLabel* menuHeader = new QLabel("MASTER & VOUCHER MENU (Use ↑ / ↓ Arrow Keys & Press Enter)", container);
    menuHeader->setStyleSheet("color: #2563EB; font-size: 11px; font-weight: 800; letter-spacing: 1px;");
    menuSection->addWidget(menuHeader);

    // 5 Interactive Nav Cards
    m_menuCards.clear();

    auto addCard = [this, menuSection](int idx, const QString& num, const QString& title, const QString& sub,
                                       const QString& sc, const QString& bBg, const QString& bTxt,
                                       const QString& aBg, const QString& aBdr) {
        DashboardMenuCard* card = new DashboardMenuCard(idx, num, title, sub, sc, bBg, bTxt, aBg, aBdr, this);
        connect(card, &DashboardMenuCard::cardClicked, this, [this](int idx) { triggerMenuIndex(idx); });
        m_menuCards.append(card);
        menuSection->addWidget(card);
    };

    addCard(0, "1", "1. Ledger Master", "New Ledger, Modify, View Ledger, Groups", "Alt+6", "#DBEAFE", "#2563EB", "#EFF6FF", "#2563EB");
    addCard(1, "2", "2. Stock Master", "Raw Paddy, Rice & By-Product Inventory", "Alt+4", "#DCFCE7", "#16A34A", "#F0FDF4", "#16A34A");
    addCard(2, "3", "3. Add Vouchers", "Sales Invoices, Paddy Slips, Journal & Milling", "F2", "#FEF3C7", "#D97706", "#FEF3C7", "#D97706");
    addCard(3, "4", "4. Other Vouchers", "J-Form Mandi Procurement & TDS Vouchers", "Alt+5", "#F3E8FF", "#7C3AED", "#F3E8FF", "#7C3AED");
    addCard(4, "5", "5. Reports & Statements", "Milling Statement, Stock Register, Party Ledger & Invoices", "Alt+7", "#DCFCE7", "#059669", "#ECFDF5", "#059669");

    layout->addLayout(menuSection);
    layout->addStretch(1);

    return container;
}

QWidget* DashboardWidget::createStatCard(const QString& title, QLabel** outValLabel, const QString& subtext, const QString& accentColor) {
    QFrame* card = new QFrame(this);
    card->setObjectName("statCard");
    card->setFixedHeight(95);
    card->setStyleSheet(
        "#statCard { background-color: #FFFFFF; border: 1px solid #E2E8F0; border-radius: 8px; }"
        "QLabel { border: none; background: transparent; }"
    );

    QHBoxLayout* root = new QHBoxLayout(card);
    root->setContentsMargins(0, 8, 14, 8);
    root->setSpacing(14);

    // Left accent bar
    QFrame* bar = new QFrame(card);
    bar->setObjectName("accentBar");
    bar->setFixedWidth(4);
    bar->setStyleSheet(QString("#accentBar { background-color: %1; border-radius: 2px; border: none; }").arg(accentColor));
    root->addWidget(bar);

    QVBoxLayout* content = new QVBoxLayout();
    content->setContentsMargins(0, 4, 0, 4);
    content->setSpacing(3);

    QLabel* tLabel = new QLabel(title, card);
    tLabel->setStyleSheet("color: #64748B; font-size: 12px; font-weight: 700; border: none; background: transparent;");
    content->addWidget(tLabel);

    *outValLabel = new QLabel("0.0 Qtl", card);
    (*outValLabel)->setStyleSheet("color: #0F172A; font-size: 20px; font-weight: 800; font-family: 'Segoe UI', -apple-system, BlinkMacSystemFont, Roboto, 'Helvetica Neue', 'Noto Sans', 'Liberation Sans', Arial, sans-serif; border: none; background: transparent;");
    content->addWidget(*outValLabel);

    if (!subtext.isEmpty()) {
        QLabel* sLabel = new QLabel(subtext, card);
        sLabel->setStyleSheet(QString("color: %1; font-size: 11px; font-weight: 600; border: none; background: transparent;").arg(accentColor));
        content->addWidget(sLabel);
    }

    root->addLayout(content, 1);
    return card;
}

// ============================================================================
// RIGHT SECTION: QUICK WIDGETS (1/6th Width)
// ============================================================================
QWidget* DashboardWidget::createRightSection() {
    QFrame* card = new QFrame(this);
    card->setObjectName("rightCard");
    card->setStyleSheet(
        "#rightCard { background-color: #FFFFFF; border: 1px solid #E2E8F0; border-radius: 10px; }"
        "QLabel { border: none; background: transparent; }"
    );
    QVBoxLayout* layout = new QVBoxLayout(card);
    layout->setContentsMargins(16, 16, 16, 16);
    layout->setSpacing(12);

    QLabel* title = new QLabel("QUICK WIDGETS", card);
    title->setStyleSheet("color: #94A3B8; font-size: 10px; font-weight: 800; letter-spacing: 1px; border: none; background: transparent;");
    layout->addWidget(title);

    QFrame* div = new QFrame(card);
    div->setFixedHeight(1);
    div->setStyleSheet("background-color: #F1F5F9; border: none;");
    layout->addWidget(div);

    layout->addStretch(1);

    QVBoxLayout* centerBox = new QVBoxLayout();
    centerBox->setSpacing(8);
    centerBox->setAlignment(Qt::AlignCenter);

    QLabel* resLabel = new QLabel("Reserved Section", card);
    resLabel->setAlignment(Qt::AlignCenter);
    resLabel->setStyleSheet("color: #94A3B8; font-size: 12px; font-weight: 700; border: none; background: transparent;");
    centerBox->addWidget(resLabel);

    layout->addLayout(centerBox);
    layout->addStretch(1);

    return card;
}

// ============================================================================
// SELECTION & TRIGGERING
// ============================================================================
void DashboardWidget::updateSelection(int index) {
    if (index < 0 || index >= m_menuCards.size()) return;
    m_selectedMenuIndex = index;
    for (int i = 0; i < m_menuCards.size(); ++i) {
        m_menuCards[i]->setSelected(i == m_selectedMenuIndex);
    }
}

void DashboardWidget::triggerMenuIndex(int index, int initialSubmenuIndex) {
    updateSelection(index);
    if (index == 0) openLedgerMenu(initialSubmenuIndex);
    else if (index == 1) openStockMenu(initialSubmenuIndex);
    else if (index == 2) openAddVoucherMenu(initialSubmenuIndex);
    else if (index == 3) openOtherVoucherMenu(initialSubmenuIndex);
    else if (index == 4) openReportsMenu(initialSubmenuIndex);
}

void DashboardWidget::openLedgerMenu(int initialIndex) {
    m_selectedMenuIndex = 0;
    updateSelection(0);
    MahadevERP::MenuTreeManager::instance().executeMenu("ledger_master", this, initialIndex);
}

void DashboardWidget::openStockMenu(int initialIndex) {
    m_selectedMenuIndex = 1;
    updateSelection(1);
    MahadevERP::MenuTreeManager::instance().executeMenu("stock_master", this, initialIndex);
}

void DashboardWidget::openAddVoucherMenu(int initialIndex) {
    m_selectedMenuIndex = 2;
    updateSelection(2);
    MahadevERP::MenuTreeManager::instance().executeMenu("add_voucher", this, initialIndex);
}

void DashboardWidget::openOtherVoucherMenu(int initialIndex) {
    m_selectedMenuIndex = 3;
    updateSelection(3);
    MahadevERP::MenuTreeManager::instance().executeMenu("other_voucher", this, initialIndex);
}

void DashboardWidget::openReportsMenu(int initialIndex) {
    m_selectedMenuIndex = 4;
    updateSelection(4);
    MahadevERP::MenuTreeManager::instance().executeMenu("reports_register", this, initialIndex);
}

void DashboardWidget::refreshStats() {
    updatePeriodBadge();

    // 1. Dynamic Firm Info
    QVariantList cRows = DatabaseManager::instance().executeQuery("SELECT company_name, gstin, pan_no, fssai_no, ml_no, business_type, city, state FROM company_info LIMIT 1;");
    if (!cRows.isEmpty()) {
        QVariantMap info = cRows.first().toMap();
        QString name = info.value("company_name").toString().trimmed();
        if (m_firmNameLabel) m_firmNameLabel->setText(name.isEmpty() ? "—" : name);
        if (m_gstinLabel) m_gstinLabel->setText(info.value("gstin").toString().trimmed().isEmpty() ? "—" : info.value("gstin").toString().trimmed());
        if (m_panLabel) m_panLabel->setText(info.value("pan_no").toString().trimmed().isEmpty() ? "—" : info.value("pan_no").toString().trimmed());
        if (m_statutoryLabel) {
            QString stat = info.value("fssai_no").toString().trimmed();
            if (!stat.isEmpty()) m_statutoryLabel->setText("FSSAI: " + stat);
            else if (!info.value("ml_no").toString().trimmed().isEmpty()) m_statutoryLabel->setText("ML: " + info.value("ml_no").toString().trimmed());
            else m_statutoryLabel->setText("—");
        }
        if (m_businessTypeLabel) m_businessTypeLabel->setText(info.value("business_type").toString().trimmed().isEmpty() ? "—" : info.value("business_type").toString().trimmed());
        if (m_locationLabel) {
            QString city = info.value("city").toString().trimmed();
            QString state = info.value("state").toString().trimmed();
            QString loc = city;
            if (!state.isEmpty()) loc += (loc.isEmpty() ? "" : ", ") + state;
            m_locationLabel->setText(loc.isEmpty() ? "—" : loc);
        }
    } else if (m_firmMgr) {
        if (m_firmNameLabel) {
            QString name = m_firmMgr->currentFirmName();
            m_firmNameLabel->setText(name.isEmpty() ? "—" : name);
        }
    }

    // 2. Metrics from DashboardController / Database
    if (m_dashCtrl) {
        FiscalYearInfo fy = FiscalYearHelper::getActiveFiscalYear();
        m_dashCtrl->refresh_stats(fy.startDate, fy.endDate, fy.name);
        if (m_paddyStockLabel) m_paddyStockLabel->setText(m_dashCtrl->paddyStock());
        if (m_riceStockLabel) m_riceStockLabel->setText(m_dashCtrl->riceStock());
        if (m_totalSalesLabel) m_totalSalesLabel->setText(m_dashCtrl->totalSales());
    } else {
        // Direct Database fallback
        QVariant vSales = DatabaseManager::instance().executeScalar("SELECT COALESCE(SUM(grand_total), 0) FROM sales_vouchers WHERE is_cancelled = 0");
        if (vSales.isValid() && m_totalSalesLabel) {
            m_totalSalesLabel->setText(AccountingEngine::formatCurrency(vSales.toDouble()));
        }
    }
}

void DashboardWidget::updatePeriodBadge() {
    FiscalYearInfo fy = FiscalYearHelper::getActiveFiscalYear();
    QString s_fmt = FiscalYearHelper::formatDisplayDate(fy.startDate);
    QString e_fmt = FiscalYearHelper::formatDisplayDate(fy.endDate);
    QString periodText = QString("%1 To %2 (%3)").arg(s_fmt, e_fmt, fy.name);

    if (m_periodBtn) {
        m_periodBtn->setText("Period: " + periodText);
    }
    if (m_fyBadgeLabel) {
        m_fyBadgeLabel->setText(periodText + " (Active)");
    }
}

void DashboardWidget::focusMenu() {
    setFocus(Qt::OtherFocusReason);
    activateWindow();
    updateSelection(m_selectedMenuIndex);
}

void DashboardWidget::showEvent(QShowEvent* event) {
    QWidget::showEvent(event);
    refreshStats();
    focusMenu();

    if (m_reopenSubmenuOnReturn && m_lastOpenedMenuIndex >= 0) {
        int menuIdx = m_lastOpenedMenuIndex;
        int subIdx = m_lastSubmenuSelectedIndex;
        m_reopenSubmenuOnReturn = false;
        updateSelection(menuIdx);
        QTimer::singleShot(10, this, [this, menuIdx, subIdx]() {
            triggerMenuIndex(menuIdx, subIdx);
        });
    } else {
        m_selectedMenuIndex = 0;
        updateSelection(0);
    }
}

void DashboardWidget::focusInEvent(QFocusEvent* event) {
    QWidget::focusInEvent(event);
    updateSelection(m_selectedMenuIndex);
}

void DashboardWidget::keyPressEvent(QKeyEvent* event) {
    if (event->key() == Qt::Key_Up) {
        int next = (m_selectedMenuIndex > 0) ? m_selectedMenuIndex - 1 : m_menuCards.size() - 1;
        updateSelection(next);
        event->accept();
        return;
    } else if (event->key() == Qt::Key_Down) {
        int next = (m_selectedMenuIndex < m_menuCards.size() - 1) ? m_selectedMenuIndex + 1 : 0;
        updateSelection(next);
        event->accept();
        return;
    } else if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
        triggerMenuIndex(m_selectedMenuIndex);
        event->accept();
        return;
    } else if (event->key() == Qt::Key_1) {
        triggerMenuIndex(0);
        event->accept();
        return;
    } else if (event->key() == Qt::Key_2) {
        triggerMenuIndex(1);
        event->accept();
        return;
    } else if (event->key() == Qt::Key_3) {
        triggerMenuIndex(2);
        event->accept();
        return;
    } else if (event->key() == Qt::Key_4) {
        triggerMenuIndex(3);
        event->accept();
        return;
    } else if (event->key() == Qt::Key_5) {
        triggerMenuIndex(4);
        event->accept();
        return;
    } else if (event->key() == Qt::Key_F2) {
        emit requestAccountingPeriodDialog();
        event->accept();
        return;
    } else if (event->key() == Qt::Key_F8) {
        m_lastOpenedMenuIndex = 2;
        m_lastSubmenuSelectedIndex = 0;
        m_reopenSubmenuOnReturn = true;
        emit openViewRequested(14);
        event->accept();
        return;
    } else if (event->key() == Qt::Key_F9) {
        m_lastOpenedMenuIndex = 2;
        m_lastSubmenuSelectedIndex = 1;
        m_reopenSubmenuOnReturn = true;
        emit openViewRequested(15);
        event->accept();
        return;
    }

    QWidget::keyPressEvent(event);
}

// ============================================================================
// DASHBOARD MENU CARD IMPLEMENTATION
// ============================================================================
DashboardMenuCard::DashboardMenuCard(int index, const QString& numStr, const QString& title, const QString& subtext,
                                     const QString& shortcutKey, const QString& badgeBg, const QString& badgeText,
                                     const QString& activeBg, const QString& activeBorder, QWidget* parent)
    : QFrame(parent)
    , m_index(index)
    , m_activeBg(activeBg)
    , m_activeBorder(activeBorder)
{
    setObjectName("dashboardMenuCard");
    setFixedHeight(54);
    setCursor(Qt::PointingHandCursor);
    setStyleSheet(
        "#dashboardMenuCard { background-color: #FFFFFF; border: 1px solid #E2E8F0; border-radius: 8px; }"
        "QLabel { border: none; background: transparent; }"
    );

    QHBoxLayout* layout = new QHBoxLayout(this);
    layout->setContentsMargins(14, 6, 14, 6);
    layout->setSpacing(12);

    // Number Badge
    QLabel* numBadge = new QLabel(numStr, this);
    numBadge->setObjectName("numBadge");
    numBadge->setFixedSize(36, 36);
    numBadge->setAlignment(Qt::AlignCenter);
    numBadge->setStyleSheet(QString("#numBadge { background-color: %1; border: none; border-radius: 8px; color: %2; font-size: 15px; font-weight: 800; }").arg(badgeBg, badgeText));
    layout->addWidget(numBadge);

    // Title and Subtext
    QVBoxLayout* textLayout = new QVBoxLayout();
    textLayout->setContentsMargins(0, 2, 0, 2);
    textLayout->setSpacing(1);

    QLabel* tLabel = new QLabel(title, this);
    tLabel->setStyleSheet("color: #0F172A; font-size: 13px; font-weight: 800; border: none; background: transparent;");
    textLayout->addWidget(tLabel);

    QLabel* sLabel = new QLabel(subtext, this);
    sLabel->setStyleSheet("color: #64748B; font-size: 11px; font-weight: 500; border: none; background: transparent;");
    textLayout->addWidget(sLabel);

    layout->addLayout(textLayout, 1);

    // Shortcut Badge
    if (!shortcutKey.isEmpty()) {
        QLabel* scBadge = new QLabel(shortcutKey, this);
        scBadge->setObjectName("scBadge");
        scBadge->setFixedHeight(24);
        scBadge->setAlignment(Qt::AlignCenter);
        scBadge->setStyleSheet(QString("#scBadge { background-color: %1; border: 1px solid %2; border-radius: 4px; padding: 2px 8px; color: %2; font-size: 11px; font-weight: 700; }").arg(badgeBg, badgeText));
        layout->addWidget(scBadge);
    }
}

void DashboardMenuCard::setSelected(bool sel) {
    m_isSelected = sel;
    if (m_isSelected) {
        setStyleSheet(QString(
            "#dashboardMenuCard { background-color: %1; border: 2px solid %2; border-radius: 8px; }"
            "QLabel { border: none; background: transparent; }"
        ).arg(m_activeBg, m_activeBorder));
    } else {
        setStyleSheet(QString(
            "#dashboardMenuCard { background-color: %1; border: 1px solid %2; border-radius: 8px; }"
            "QLabel { border: none; background: transparent; }"
        ).arg(m_normalBg, m_normalBorder));
    }
}

void DashboardMenuCard::mousePressEvent(QMouseEvent* event) {
    emit cardClicked(m_index);
    QFrame::mousePressEvent(event);
}

void DashboardMenuCard::enterEvent(QEnterEvent* event) {
    emit cardHovered(m_index);
    QFrame::enterEvent(event);
}

// ============================================================================
// POPUP SUBMENU DIALOG IMPLEMENTATION
// ============================================================================
DashboardSubmenuDialog::DashboardSubmenuDialog(const QString& menuTitle, const QString& borderColor,
                                               const QVector<SubmenuItem>& items, int initialSelectedIndex, QWidget* parent)
    : QDialog(parent, Qt::Popup | Qt::FramelessWindowHint)
    , m_items(items)
    , m_selectedIndex(initialSelectedIndex)
    , m_borderColor(borderColor)
    , m_initialMousePos(QCursor::pos())
    , m_hasMouseMoved(false)
{
    setAttribute(Qt::WA_StyledBackground, true);
    setObjectName("submenuDialog");
    setFixedWidth(460);
    setStyleSheet(QString(
        "#submenuDialog { background-color: #FFFFFF; border: 2.5px solid %1; border-radius: 12px; }"
        "QLabel { border: none; background: transparent; }"
    ).arg(m_borderColor));

    QVBoxLayout* root = new QVBoxLayout(this);
    root->setContentsMargins(14, 14, 14, 14);
    root->setSpacing(10);

    // Header
    QHBoxLayout* header = new QHBoxLayout();
    QLabel* titleLabel = new QLabel(menuTitle, this);
    titleLabel->setStyleSheet(QString("color: %1; font-size: 13px; font-weight: 800; letter-spacing: 1px; border: none; background: transparent;").arg(m_borderColor));
    header->addWidget(titleLabel);

    header->addStretch(1);
    QLabel* hint = new QLabel("Press ↑ / ↓ & Enter", this);
    hint->setStyleSheet("color: #64748B; font-size: 11px; font-weight: 700; border: none; background: transparent;");
    header->addWidget(hint);
    root->addLayout(header);

    // Items
    for (int i = 0; i < m_items.size(); ++i) {
        const SubmenuItem& itm = m_items[i];
        QFrame* row = new QFrame(this);
        row->setObjectName("submenuRow");
        row->setFixedHeight(46);
        row->setCursor(Qt::PointingHandCursor);
        row->installEventFilter(this);

        QHBoxLayout* rowLayout = new QHBoxLayout(row);
        rowLayout->setContentsMargins(12, 4, 12, 4);

        QLabel* label = new QLabel(itm.title, row);
        label->setAttribute(Qt::WA_TransparentForMouseEvents, true);
        label->setStyleSheet("font-size: 13px; font-weight: 700; color: #0F172A; border: none; background: transparent;");
        rowLayout->addWidget(label, 1);

        if (!itm.shortcut.isEmpty()) {
            QLabel* sc = new QLabel(itm.shortcut, row);
            sc->setObjectName("scLabel");
            sc->setAttribute(Qt::WA_TransparentForMouseEvents, true);
            sc->setStyleSheet("#scLabel { background-color: #F1F5F9; border: 1px solid #CBD5E1; border-radius: 4px; padding: 2px 6px; font-size: 10px; font-weight: 700; color: #475569; }");
            rowLayout->addWidget(sc);
        }

        m_itemFrames.append(row);
        root->addWidget(row);
    }

    if (m_selectedIndex < 0 || m_selectedIndex >= m_itemFrames.size()) {
        m_selectedIndex = 0;
    }
    updateSelection(m_selectedIndex);
}

void DashboardSubmenuDialog::showEvent(QShowEvent* event) {
    m_initialMousePos = QCursor::pos();
    m_hasMouseMoved = false;
    QDialog::showEvent(event);
    if (parentWidget()) {
        QPoint center = parentWidget()->mapToGlobal(parentWidget()->rect().center());
        move(center.x() - width() / 2, center.y() - height() / 2);
    }
    updateSelection(m_selectedIndex);
}

bool DashboardSubmenuDialog::eventFilter(QObject* watched, QEvent* event) {
    for (int i = 0; i < m_itemFrames.size(); ++i) {
        if (m_itemFrames[i] == watched) {
            if (event->type() == QEvent::MouseButtonPress) {
                m_selectedIndex = i;
                triggerCurrent();
                return true;
            } else if (event->type() == QEvent::MouseMove || event->type() == QEvent::Enter) {
                QPoint currentPos = QCursor::pos();
                if (!m_hasMouseMoved) {
                    if ((currentPos - m_initialMousePos).manhattanLength() > 5) {
                        m_hasMouseMoved = true;
                    }
                }
                if (m_hasMouseMoved) {
                    updateSelection(i);
                }
                return true;
            }
        }
    }
    return QDialog::eventFilter(watched, event);
}

void DashboardSubmenuDialog::updateSelection(int idx) {
    if (idx < 0 || idx >= m_itemFrames.size()) return;
    m_selectedIndex = idx;
    for (int i = 0; i < m_itemFrames.size(); ++i) {
        if (i == m_selectedIndex) {
            m_itemFrames[i]->setStyleSheet(QString(
                "#submenuRow { background-color: #EFF6FF; border: 1.5px solid %1; border-radius: 8px; }"
                "QLabel { border: none; background: transparent; }"
            ).arg(m_borderColor));
        } else {
            m_itemFrames[i]->setStyleSheet(
                "#submenuRow { background-color: #FFFFFF; border: 1px solid #E2E8F0; border-radius: 8px; }"
                "QLabel { border: none; background: transparent; }"
            );
        }
    }
}

void DashboardSubmenuDialog::triggerCurrent() {
    if (m_selectedIndex >= 0 && m_selectedIndex < m_items.size()) {
        m_selectedViewIndex = m_items[m_selectedIndex].targetViewIndex;
        accept();
    }
}

void DashboardSubmenuDialog::keyPressEvent(QKeyEvent* event) {
    if (event->key() == Qt::Key_Up) {
        int next = (m_selectedIndex > 0) ? m_selectedIndex - 1 : m_itemFrames.size() - 1;
        updateSelection(next);
        event->accept();
        return;
    } else if (event->key() == Qt::Key_Down) {
        int next = (m_selectedIndex < m_itemFrames.size() - 1) ? m_selectedIndex + 1 : 0;
        updateSelection(next);
        event->accept();
        return;
    } else if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
        triggerCurrent();
        event->accept();
        return;
    } else if (event->key() >= Qt::Key_1 && event->key() <= Qt::Key_9) {
        int num = event->key() - Qt::Key_1;
        if (num < m_itemFrames.size()) {
            m_selectedIndex = num;
            triggerCurrent();
        }
        event->accept();
        return;
    } else if (event->key() == Qt::Key_Escape) {
        reject();
        event->accept();
        return;
    }
    QDialog::keyPressEvent(event);
}

