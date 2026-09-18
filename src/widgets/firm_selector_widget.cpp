#include "firm_selector_widget.h"
#include "new_firm_dialog.h"
#include "mdb_migration_dialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QFrame>
#include <QKeyEvent>
#include <QFileDialog>
#include <QMessageBox>
#include <QTimer>

FirmSelectorWidget::FirmSelectorWidget(FirmManager* firmMgr,
                                       BahiKhataMigrator* migrator,
                                       QWidget* parent)
    : QWidget(parent)
    , m_firmMgr(firmMgr)
    , m_migrator(migrator)
{
    m_appDataFolder = m_firmMgr ? m_firmMgr->get_app_data_folder() : "data";
    m_currentFolder = m_appDataFolder;

    setupUi();
    refreshFirms();
}

bool FirmSelectorWidget::isViewingAppData() const {
    return (m_currentFolder == m_appDataFolder ||
            m_currentFolder.endsWith("/data") ||
            m_currentFolder.endsWith("/data/") ||
            m_currentFolder == "data");
}

void FirmSelectorWidget::setupUi() {
    setAttribute(Qt::WA_StyledBackground, true);
    setStyleSheet("FirmSelectorWidget { background-color: #F8FAFC; }");

    QVBoxLayout* rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(20, 16, 20, 16);
    rootLayout->setSpacing(14);

    // ========================================================================
    // 1. TOP HEADER CARD
    // ========================================================================
    QFrame* headerCard = new QFrame(this);
    headerCard->setFixedHeight(64);
    headerCard->setStyleSheet("background-color: #FFFFFF; border: 1px solid #E2E8F0; border-radius: 10px;");
    QHBoxLayout* headerLayout = new QHBoxLayout(headerCard);
    headerLayout->setContentsMargins(18, 0, 18, 0);
    headerLayout->setSpacing(14);

    QFrame* iconBox = new QFrame(headerCard);
    iconBox->setFixedSize(40, 40);
    iconBox->setStyleSheet("background-color: #EFF6FF; border: 1px solid #BFDBFE; border-radius: 8px;");
    QLabel* iconLabel = new QLabel("🏢", iconBox);
    iconLabel->setAlignment(Qt::AlignCenter);
    iconLabel->setGeometry(0, 0, 40, 40);
    headerLayout->addWidget(iconBox);

    QVBoxLayout* titleBox = new QVBoxLayout();
    titleBox->setSpacing(1);
    QLabel* titleLabel = new QLabel("SELECT COMPANY / FIRM", headerCard);
    titleLabel->setStyleSheet("color: #0F172A; font-size: 16px; font-weight: 800; letter-spacing: 0.5px; border: none; background: transparent;");
    titleBox->addWidget(titleLabel);

    m_subtitleLabel = new QLabel("App Working Directory • Managing native SQLite company databases in data/", headerCard);
    m_subtitleLabel->setStyleSheet("color: #64748B; font-size: 11px; border: none; background: transparent;");
    titleBox->addWidget(m_subtitleLabel);
    headerLayout->addLayout(titleBox);

    headerLayout->addStretch(1);

    m_backBtn = new QPushButton("← Back to Dashboard (Esc)", headerCard);
    m_backBtn->setFixedHeight(32);
    m_backBtn->setCursor(Qt::PointingHandCursor);
    m_backBtn->setStyleSheet(
        "QPushButton { background-color: #F1F5F9; border: 1px solid #CBD5E1; border-radius: 6px; padding: 0px 14px; font-weight: 700; color: #475569; font-size: 12px; }"
        "QPushButton:hover { background-color: #E2E8F0; }"
    );
    connect(m_backBtn, &QPushButton::clicked, this, &FirmSelectorWidget::backRequested);
    headerLayout->addWidget(m_backBtn);

    rootLayout->addWidget(headerCard);

    // ========================================================================
    // 2. DIRECTORY CONTROLS & IMPORT BAR
    // ========================================================================
    m_folderBarFrame = new QFrame(this);
    m_folderBarFrame->setFixedHeight(52);
    m_folderBarFrame->setStyleSheet("background-color: #FFFFFF; border: 1px solid #E2E8F0; border-radius: 8px;");
    QHBoxLayout* folderLayout = new QHBoxLayout(m_folderBarFrame);
    folderLayout->setContentsMargins(16, 0, 16, 0);
    folderLayout->setSpacing(12);

    m_folderDirLabel = new QLabel("📁 App Data Directory:", m_folderBarFrame);
    m_folderDirLabel->setStyleSheet("color: #334155; font-size: 12px; font-weight: 700; border: none; background: transparent;");
    folderLayout->addWidget(m_folderDirLabel);

    m_folderInput = new QLineEdit(m_folderBarFrame);
    m_folderInput->setFixedHeight(34);
    m_folderInput->setText(m_currentFolder);
    m_folderInput->setStyleSheet(
        "QLineEdit {"
        "  background-color: #F8FAFC;"
        "  border: 1px solid #CBD5E1;"
        "  border-radius: 6px;"
        "  padding: 0px 10px;"
        "  font-size: 12px;"
        "  color: #0F172A;"
        "  font-family: 'Consolas', 'Menlo', 'Monaco', 'DejaVu Sans Mono', 'Liberation Mono', 'Courier New', monospace;"
        "}"
        "QLineEdit:focus {"
        "  border: 1.5px solid #2563EB;"
        "  background-color: #FFFFFF;"
        "}"
    );
    connect(m_folderInput, &QLineEdit::returnPressed, this, &FirmSelectorWidget::onFolderAccepted);
    folderLayout->addWidget(m_folderInput, 1);

    m_appDataBtn = new QPushButton("📁 App Data Folder", m_folderBarFrame);
    m_appDataBtn->setFixedHeight(34);
    m_appDataBtn->setCursor(Qt::PointingHandCursor);
    m_appDataBtn->setStyleSheet(
        "QPushButton { background-color: #F1F5F9; border: 1px solid #CBD5E1; border-radius: 6px; padding: 0px 14px; font-weight: 700; color: #1E293B; font-size: 12px; }"
        "QPushButton:hover { background-color: #E2E8F0; }"
    );
    connect(m_appDataBtn, &QPushButton::clicked, this, &FirmSelectorWidget::resetToAppData);
    folderLayout->addWidget(m_appDataBtn);

    m_importMdbBtn = new QPushButton("📥 Import External MDB (F4)", m_folderBarFrame);
    m_importMdbBtn->setFixedHeight(34);
    m_importMdbBtn->setCursor(Qt::PointingHandCursor);
    m_importMdbBtn->setStyleSheet(
        "QPushButton { background-color: #EFF6FF; border: 1px solid #BFDBFE; border-radius: 6px; padding: 0px 14px; font-weight: 700; color: #1D4ED8; font-size: 12px; }"
        "QPushButton:hover { background-color: #DBEAFE; }"
    );
    connect(m_importMdbBtn, &QPushButton::clicked, this, &FirmSelectorWidget::pickFolder);
    folderLayout->addWidget(m_importMdbBtn);

    m_browseBtn = new QPushButton("Browse (F3)", m_folderBarFrame);
    m_browseBtn->setFixedHeight(34);
    m_browseBtn->setCursor(Qt::PointingHandCursor);
    m_browseBtn->setStyleSheet(
        "QPushButton { background-color: #F1F5F9; border: 1px solid #CBD5E1; border-radius: 6px; padding: 0px 14px; font-weight: 700; color: #334155; font-size: 12px; }"
        "QPushButton:hover { background-color: #E2E8F0; }"
    );
    connect(m_browseBtn, &QPushButton::clicked, this, &FirmSelectorWidget::pickFolder);
    folderLayout->addWidget(m_browseBtn);

    m_rescanBtn = new QPushButton("🔄 Rescan (F5)", m_folderBarFrame);
    m_rescanBtn->setFixedHeight(34);
    m_rescanBtn->setCursor(Qt::PointingHandCursor);
    m_rescanBtn->setStyleSheet(
        "QPushButton { background-color: #F1F5F9; border: 1px solid #CBD5E1; border-radius: 6px; padding: 0px 14px; font-weight: 700; color: #334155; font-size: 12px; }"
        "QPushButton:hover { background-color: #E2E8F0; }"
    );
    connect(m_rescanBtn, &QPushButton::clicked, this, &FirmSelectorWidget::refreshFirms);
    folderLayout->addWidget(m_rescanBtn);

    rootLayout->addWidget(m_folderBarFrame);

    // ========================================================================
    // 3. TABLE OF FIRMS CONTAINER & EMPTY STATE
    // ========================================================================
    QFrame* tableContainer = new QFrame(this);
    tableContainer->setStyleSheet("background-color: #FFFFFF; border: 1px solid #E2E8F0; border-radius: 10px;");
    QVBoxLayout* tableContainerLayout = new QVBoxLayout(tableContainer);
    tableContainerLayout->setContentsMargins(1, 1, 1, 1);
    tableContainerLayout->setSpacing(0);

    m_centerStack = new QStackedWidget(tableContainer);

    // Page 0: High-Performance QTableWidget
    m_table = new QTableWidget(m_centerStack);
    m_table->setColumnCount(7);
    m_table->setHorizontalHeaderLabels({
        "Database File",
        "Company / Firm Name",
        "Period (Financial Years)",
        "GSTIN",
        "City / Station",
        "Entity Type",
        "Status"
    });
    m_table->horizontalHeader()->setStretchLastSection(false);
    m_table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Fixed);
    m_table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    m_table->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Fixed);
    m_table->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Fixed);
    m_table->horizontalHeader()->setSectionResizeMode(4, QHeaderView::Fixed);
    m_table->horizontalHeader()->setSectionResizeMode(5, QHeaderView::Fixed);
    m_table->horizontalHeader()->setSectionResizeMode(6, QHeaderView::Fixed);

    m_table->setColumnWidth(0, 160);
    m_table->setColumnWidth(2, 170);
    m_table->setColumnWidth(3, 140);
    m_table->setColumnWidth(4, 90);
    m_table->setColumnWidth(5, 130);
    m_table->setColumnWidth(6, 95);

    m_table->verticalHeader()->setVisible(false);
    m_table->verticalHeader()->setDefaultSectionSize(44);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setShowGrid(false);
    m_table->setAlternatingRowColors(true);
    m_table->setFocusPolicy(Qt::StrongFocus);
    m_table->setStyleSheet(
        "QTableWidget {"
        "  background-color: #FFFFFF;"
        "  alternate-background-color: #FAFAFA;"
        "  border: none;"
        "  font-size: 12px;"
        "  color: #0F172A;"
        "  outline: 0;"
        "}"
        "QTableWidget::item {"
        "  padding: 6px 12px;"
        "  border: none;"
        "  border-bottom: 1px solid #F1F5F9;"
        "  outline: 0;"
        "}"
        "QTableWidget::item:selected {"
        "  background-color: #EFF6FF;"
        "  color: #1D4ED8;"
        "  outline: 0;"
        "}"
        "QTableWidget::item:hover:!selected {"
        "  background-color: #F8FAFC;"
        "}"
        "QHeaderView::section {"
        "  background-color: #F8FAFC;"
        "  color: #475569;"
        "  font-weight: 800;"
        "  font-size: 11px;"
        "  padding: 8px 12px;"
        "  border: none;"
        "  border-bottom: 1.5px solid #E2E8F0;"
        "}"
    );
    connect(m_table, &QTableWidget::itemDoubleClicked, this, &FirmSelectorWidget::onTableItemDoubleClicked);
    connect(m_table, &QTableWidget::itemSelectionChanged, this, &FirmSelectorWidget::onTableRowSelectionChanged);
    m_centerStack->addWidget(m_table);

    // Page 1: Empty State
    m_emptyStateWidget = new QWidget(m_centerStack);
    QVBoxLayout* emptyLayout = new QVBoxLayout(m_emptyStateWidget);
    emptyLayout->setContentsMargins(40, 60, 40, 60);
    emptyLayout->setSpacing(12);
    emptyLayout->setAlignment(Qt::AlignCenter);

    QLabel* emptyIcon = new QLabel("📁", m_emptyStateWidget);
    emptyIcon->setStyleSheet("font-size: 40px; border: none; background: transparent;");
    emptyIcon->setAlignment(Qt::AlignCenter);
    emptyLayout->addWidget(emptyIcon);

    m_emptyStateLabel = new QLabel("No native databases found in data/ folder.", m_emptyStateWidget);
    m_emptyStateLabel->setStyleSheet("font-size: 14px; font-weight: 600; color: #64748B; border: none; background: transparent;");
    m_emptyStateLabel->setAlignment(Qt::AlignCenter);
    emptyLayout->addWidget(m_emptyStateLabel);

    m_emptyStateBtn = new QPushButton("Import External Data", m_emptyStateWidget);
    m_emptyStateBtn->setFixedSize(220, 36);
    m_emptyStateBtn->setCursor(Qt::PointingHandCursor);
    m_emptyStateBtn->setStyleSheet(
        "QPushButton { background-color: #EFF6FF; border: 1px solid #BFDBFE; border-radius: 6px; font-weight: 700; color: #1D4ED8; font-size: 12px; }"
        "QPushButton:hover { background-color: #DBEAFE; }"
    );
    connect(m_emptyStateBtn, &QPushButton::clicked, this, [this]() {
        if (isViewingAppData()) {
            pickFolder();
        } else {
            resetToAppData();
        }
    });
    emptyLayout->addWidget(m_emptyStateBtn);
    m_centerStack->addWidget(m_emptyStateWidget);

    tableContainerLayout->addWidget(m_centerStack);
    rootLayout->addWidget(tableContainer, 1);

    // ========================================================================
    // 4. ACTION BAR FOOTER
    // ========================================================================
    QHBoxLayout* footerLayout = new QHBoxLayout();
    footerLayout->setSpacing(12);

    m_newFirmBtn = new QPushButton("+ Create New Firm (F2)", this);
    m_newFirmBtn->setFixedHeight(36);
    m_newFirmBtn->setCursor(Qt::PointingHandCursor);
    m_newFirmBtn->setStyleSheet(
        "QPushButton { background-color: #16A34A; border: none; border-radius: 6px; padding: 0px 18px; font-weight: 700; color: #FFFFFF; font-size: 12px; }"
        "QPushButton:hover { background-color: #15803D; }"
    );
    connect(m_newFirmBtn, &QPushButton::clicked, this, &FirmSelectorWidget::openNewFirmDialog);
    footerLayout->addWidget(m_newFirmBtn);

    footerLayout->addStretch(1);

    m_openFirmBtn = new QPushButton("Open Selected Firm (Enter)", this);
    m_openFirmBtn->setFixedHeight(36);
    m_openFirmBtn->setCursor(Qt::PointingHandCursor);
    m_openFirmBtn->setStyleSheet(
        "QPushButton { background-color: #2563EB; border: none; border-radius: 6px; padding: 0px 22px; font-weight: 700; color: #FFFFFF; font-size: 12px; }"
        "QPushButton:hover { background-color: #1D4ED8; }"
    );
    connect(m_openFirmBtn, &QPushButton::clicked, this, &FirmSelectorWidget::openSelectedFirm);
    footerLayout->addWidget(m_openFirmBtn);

    rootLayout->addLayout(footerLayout);

    setFocusPolicy(Qt::StrongFocus);
    updateHeaderAndFolderStyles();
}

void FirmSelectorWidget::updateHeaderAndFolderStyles() {
    bool appData = isViewingAppData();
    if (appData) {
        m_subtitleLabel->setText("App Working Directory • Managing native SQLite company databases in data/");
        m_folderDirLabel->setText("📁 App Data Directory:");
        m_folderDirLabel->setStyleSheet("color: #334155; font-size: 12px; font-weight: 700; border: none; background: transparent;");
        m_folderBarFrame->setStyleSheet("background-color: #FFFFFF; border: 1px solid #E2E8F0; border-radius: 8px;");
        m_appDataBtn->setVisible(false);
        m_emptyStateLabel->setText("No native databases found in data/ folder.");
        m_emptyStateBtn->setText("Import External Data");
        m_table->horizontalHeaderItem(0)->setText("Database File");
    } else {
        m_subtitleLabel->setText("External Database Import • Select a firm to import into the app's working directory");
        m_folderDirLabel->setText("📁 External Data Folder:");
        m_folderDirLabel->setStyleSheet("color: #B45309; font-size: 12px; font-weight: 700; border: none; background: transparent;");
        m_folderBarFrame->setStyleSheet("background-color: #FFFFFF; border: 1px solid #FDE68A; border-radius: 8px;");
        m_appDataBtn->setVisible(true);
        m_emptyStateLabel->setText("No Data.* files found in selected directory.");
        m_emptyStateBtn->setText("Return to App Data Folder");
        m_table->horizontalHeaderItem(0)->setText("Source File");
    }

    if (m_firmMgr && !m_firmMgr->currentFirmName().isEmpty()) {
        m_backBtn->setVisible(true);
    } else {
        m_backBtn->setVisible(false);
    }
}

void FirmSelectorWidget::refreshFirms() {
    updateHeaderAndFolderStyles();
    m_folderInput->setText(m_currentFolder);

    if (m_firmMgr) {
        m_firmsList = m_firmMgr->scan_folder_for_firms(m_currentFolder);
        if (m_firmsList.isEmpty() && !isViewingAppData()) {
            m_firmsList = m_firmMgr->get_registered_firms();
        }
    }

    if (m_firmsList.isEmpty()) {
        m_centerStack->setCurrentWidget(m_emptyStateWidget);
        m_table->setRowCount(0);
        return;
    }

    m_centerStack->setCurrentWidget(m_table);
    m_table->setRowCount(m_firmsList.size());

    int activeRow = 0;
    for (int i = 0; i < m_firmsList.size(); ++i) {
        QVariantMap map = m_firmsList[i].toMap();
        bool isAct = map.value("isActive").toBool();
        if (isAct) activeRow = i;

        // Col 0: File
        QString fileStr = isViewingAppData() ?
            map.value("db_name", map.value("id").toString() + ".db").toString() :
            map.value("source_file", "Data.*").toString();
        QTableWidgetItem* fileItem = new QTableWidgetItem(fileStr);
        fileItem->setForeground(QColor("#0284C7"));
        QFont f = fileItem->font();
        f.setBold(true);
        f.setFamily("Consolas");
        fileItem->setFont(f);
        m_table->setItem(i, 0, fileItem);

        // Col 1: Company / Firm Name (Plain text with [ACTIVE] tag)
        QString nameStr = map.value("name", "Unnamed Firm").toString();
        if (isAct) {
            nameStr += "  [ACTIVE]";
        }
        QTableWidgetItem* nameItem = new QTableWidgetItem(nameStr);
        nameItem->setForeground(isAct ? QColor("#16A34A") : QColor("#0F172A"));
        QFont nFont = nameItem->font();
        nFont.setBold(true);
        nameItem->setFont(nFont);
        m_table->setItem(i, 1, nameItem);

        // Col 2: Period
        QTableWidgetItem* periodItem = new QTableWidgetItem(map.value("period", "All Fiscal Years").toString());
        periodItem->setForeground(QColor("#475569"));
        m_table->setItem(i, 2, periodItem);

        // Col 3: GSTIN
        QTableWidgetItem* gstinItem = new QTableWidgetItem(map.value("gstin", "-").toString());
        gstinItem->setForeground(QColor("#64748B"));
        QFont gFont = gstinItem->font();
        gFont.setFamily("Consolas");
        gstinItem->setFont(gFont);
        m_table->setItem(i, 3, gstinItem);

        // Col 4: City
        QTableWidgetItem* cityItem = new QTableWidgetItem(map.value("city", "Sirsa").toString());
        cityItem->setForeground(QColor("#334155"));
        m_table->setItem(i, 4, cityItem);

        // Col 5: Entity Type
        QTableWidgetItem* typeItem = new QTableWidgetItem(map.value("firm_type", "Partnership").toString());
        typeItem->setForeground(QColor("#64748B"));
        m_table->setItem(i, 5, typeItem);

        // Col 6: Status / Action (Plain text)
        bool isImported = map.value("is_imported", true).toBool();
        QString statusStr;
        QColor statusColor;
        if (isAct) {
            statusStr = "Active";
            statusColor = QColor("#16A34A");
        } else if (!isViewingAppData() && !isImported) {
            statusStr = "Import";
            statusColor = QColor("#2563EB");
        } else if (isImported) {
            statusStr = "Ready";
            statusColor = QColor("#16A34A");
        } else {
            statusStr = "Import";
            statusColor = QColor("#D97706");
        }
        QTableWidgetItem* statusItem = new QTableWidgetItem(statusStr);
        statusItem->setForeground(statusColor);
        QFont sFont = statusItem->font();
        sFont.setBold(true);
        statusItem->setFont(sFont);
        m_table->setItem(i, 6, statusItem);
    }

    if (m_table->rowCount() > 0) {
        m_table->selectRow(activeRow);
    }
}

void FirmSelectorWidget::resetToAppData() {
    m_currentFolder = m_appDataFolder;
    if (m_firmMgr) {
        m_firmMgr->set_active_firm_folder(m_appDataFolder);
    }
    refreshFirms();
    m_table->setFocus();
}

void FirmSelectorWidget::pickFolder() {
    QString startDir = m_currentFolder.isEmpty() ? m_appDataFolder : m_currentFolder;
    QString dir = QFileDialog::getExistingDirectory(this, "Select Directory with Bahi-Khata / SQLite Databases", startDir);
    if (!dir.isEmpty()) {
        m_currentFolder = dir;
        if (m_firmMgr) {
            m_firmMgr->set_active_firm_folder(dir);
        }
        refreshFirms();
        m_table->setFocus();
    }
}

void FirmSelectorWidget::onFolderAccepted() {
    QString text = m_folderInput->text().trimmed();
    if (!text.isEmpty()) {
        m_currentFolder = text;
        if (m_firmMgr) {
            m_firmMgr->set_active_firm_folder(text);
        }
        refreshFirms();
        m_table->setFocus();
    }
}

void FirmSelectorWidget::onTableRowSelectionChanged() {
    int row = m_table->currentRow();
    if (row >= 0 && row < m_firmsList.size()) {
        QVariantMap map = m_firmsList[row].toMap();
        bool isImported = map.value("is_imported", true).toBool();
        if (!isViewingAppData() && !isImported) {
            m_openFirmBtn->setText("Import & Open Firm (Enter)");
        } else {
            m_openFirmBtn->setText("Open Selected Firm (Enter)");
        }
    }
}

void FirmSelectorWidget::onTableItemDoubleClicked(QTableWidgetItem* /*item*/) {
    openSelectedFirm();
}

void FirmSelectorWidget::openSelectedFirm() {
    int row = m_table->currentRow();
    if (row < 0 || row >= m_firmsList.size()) return;

    QVariantMap map = m_firmsList[row].toMap();
    if (!isViewingAppData()) {
        startImportForFirm(map);
        return;
    }

    QString firmId = map.value("id").toString();
    QString firmName = map.value("name").toString();
    if (m_firmMgr) {
        bool ok = m_firmMgr->switch_to_firm(firmId);
        if (ok) {
            emit firmOpened(firmId, firmName);
        }
    }
}

void FirmSelectorWidget::startImportForFirm(const QVariantMap& firmMap) {
    QString filePath = firmMap.value("full_path").toString();
    if (filePath.isEmpty()) {
        filePath = m_currentFolder + "/" + firmMap.value("source_file").toString();
    }
    QString firmName = firmMap.value("name").toString();
    QString firmId = firmMap.value("id").toString();

    MdbMigrationDialog dlg(m_migrator, m_firmMgr, filePath, firmName, firmId, this);
    connect(&dlg, &MdbMigrationDialog::migrationCompleted, this, [this](const QString& fid, const QString& fname) {
        resetToAppData();
        emit firmOpened(fid, fname);
    });
    dlg.exec();
}

void FirmSelectorWidget::openNewFirmDialog() {
    NewFirmDialog dlg(m_firmMgr, this);
    connect(&dlg, &NewFirmDialog::firmCreated, this, [this](const QString& firmId, const QString& firmName) {
        resetToAppData();
        emit firmOpened(firmId, firmName);
    });
    dlg.exec();
}

void FirmSelectorWidget::keyPressEvent(QKeyEvent* event) {
    if (event->key() == Qt::Key_Escape) {
        if (!isViewingAppData()) {
            resetToAppData();
        } else if (m_firmMgr && !m_firmMgr->currentFirmName().isEmpty()) {
            emit backRequested();
        }
        event->accept();
        return;
    } else if (event->key() == Qt::Key_F2) {
        openNewFirmDialog();
        event->accept();
        return;
    } else if (event->key() == Qt::Key_F3 || event->key() == Qt::Key_F4) {
        pickFolder();
        event->accept();
        return;
    } else if (event->key() == Qt::Key_F5) {
        refreshFirms();
        event->accept();
        return;
    } else if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
        openSelectedFirm();
        event->accept();
        return;
    }
    QWidget::keyPressEvent(event);
}

void FirmSelectorWidget::focusTable() {
    if (m_table && m_table->rowCount() > 0) {
        int r = m_table->currentRow() >= 0 ? m_table->currentRow() : 0;
        m_table->selectRow(r);
        m_table->setCurrentCell(r, 1);
        m_table->setFocus(Qt::OtherFocusReason);
    }
}

void FirmSelectorWidget::showEvent(QShowEvent* event) {
    QWidget::showEvent(event);
    refreshFirms();
    QTimer::singleShot(20, this, &FirmSelectorWidget::focusTable);
}
