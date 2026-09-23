#include "mdb_migration_dialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QFrame>
#include <QFileInfo>
#include <QKeyEvent>
#include <QTimer>
#include <QFileDialog>
#include <QGraphicsDropShadowEffect>

MdbMigrationDialog::MdbMigrationDialog(BahiKhataMigrator* migrator,
                                       FirmManager* firmMgr,
                                       const QString& filePath,
                                       const QString& firmName,
                                       const QString& firmId,
                                       QWidget* parent)
    : QDialog(parent)
    , m_migrator(migrator)
    , m_firmMgr(firmMgr)
    , m_filePath(filePath)
    , m_firmName(firmName)
    , m_firmId(firmId)
{
    setWindowTitle("Database Migration & Sync");
    setModal(true);
    setFixedSize(620, 540);
    setupUi();
    if (!m_filePath.isEmpty()) {
        inspectFile();
    }
}

QWidget* MdbMigrationDialog::createStatCard(const QString& title, QLabel** outValLabel, const QString& bgColor, const QString& borderColor, const QString& numColor, const QString& labelColor) {
    QFrame* card = new QFrame(this);
    card->setStyleSheet(QString(
        "QFrame { background-color: %1; border: 1px solid %2; border-radius: 8px; }"
    ).arg(bgColor, borderColor));

    QVBoxLayout* layout = new QVBoxLayout(card);
    layout->setContentsMargins(12, 10, 12, 10);
    layout->setSpacing(2);
    layout->setAlignment(Qt::AlignCenter);

    QLabel* valLabel = new QLabel("0", card);
    valLabel->setAlignment(Qt::AlignCenter);
    valLabel->setStyleSheet(QString("font-size: 22px; font-weight: 800; color: %1; border: none; background: transparent;").arg(numColor));
    layout->addWidget(valLabel);

    QLabel* titleLabel = new QLabel(title, card);
    titleLabel->setAlignment(Qt::AlignCenter);
    titleLabel->setStyleSheet(QString("font-size: 12px; font-weight: 600; color: %1; border: none; background: transparent;").arg(labelColor));
    layout->addWidget(titleLabel);

    if (outValLabel) *outValLabel = valLabel;
    return card;
}

void MdbMigrationDialog::setupUi() {
    setAttribute(Qt::WA_StyledBackground, true);
    setStyleSheet(
        "MdbMigrationDialog { background-color: #FFFFFF; border: 1.5px solid #E2E8F0; border-radius: 16px; }"
        "QLabel { border: none; background: transparent; }"
    );

    QVBoxLayout* root = new QVBoxLayout(this);
    root->setContentsMargins(20, 20, 20, 20);
    root->setSpacing(14);

    // ========================================================================
    // 1. Header (Matching MdbMigrationModal.qml lines 55-83)
    // ========================================================================
    QHBoxLayout* header = new QHBoxLayout();
    header->setSpacing(12);

    QFrame* iconFrame = new QFrame(this);
    iconFrame->setFixedSize(40, 40);
    iconFrame->setStyleSheet("background-color: #EFF6FF; border: 1px solid #BFDBFE; border-radius: 10px;");
    QLabel* iconLabel = new QLabel("📥", iconFrame);
    iconLabel->setAlignment(Qt::AlignCenter);
    iconLabel->setStyleSheet("font-size: 20px; border: none; background: transparent;");
    iconLabel->setGeometry(0, 0, 40, 40);
    header->addWidget(iconFrame);

    QVBoxLayout* titleBox = new QVBoxLayout();
    titleBox->setSpacing(2);
    QLabel* tagLabel = new QLabel("DATABASE MIGRATION & SYNC", this);
    tagLabel->setStyleSheet("font-size: 11px; font-weight: 800; color: #2563EB; letter-spacing: 1.0px;");
    titleBox->addWidget(tagLabel);

    QLabel* titleLabel = new QLabel("In-App Database Importer (Data.* / .mdb)", this);
    titleLabel->setStyleSheet("font-size: 17px; font-weight: 800; color: #0F172A;");
    titleBox->addWidget(titleLabel);
    header->addLayout(titleBox);

    header->addStretch(1);

    m_headerCloseBtn = new QPushButton("✕", this);
    m_headerCloseBtn->setFixedSize(32, 32);
    m_headerCloseBtn->setCursor(Qt::PointingHandCursor);
    m_headerCloseBtn->setStyleSheet(
        "QPushButton { background-color: #F1F5F9; border: none; border-radius: 16px; color: #64748B; font-size: 13px; font-weight: 800; }"
        "QPushButton:hover { background-color: #FEE2E2; color: #EF4444; }"
    );
    connect(m_headerCloseBtn, &QPushButton::clicked, this, &QDialog::reject);
    header->addWidget(m_headerCloseBtn);

    root->addLayout(header);

    // Divider
    QFrame* div1 = new QFrame(this);
    div1->setFixedHeight(1);
    div1->setStyleSheet("background-color: #E2E8F0; border: none;");
    root->addWidget(div1);

    // ========================================================================
    // 2. File Selection Section (Matching MdbMigrationModal.qml lines 88-125)
    // ========================================================================
    QFrame* fileSectionFrame = new QFrame(this);
    fileSectionFrame->setFixedHeight(60);
    fileSectionFrame->setStyleSheet("QFrame { background-color: #F8FAFC; border: 1px solid #CBD5E1; border-radius: 10px; }");
    QHBoxLayout* fileLayout = new QHBoxLayout(fileSectionFrame);
    fileLayout->setContentsMargins(12, 10, 12, 10);
    fileLayout->setSpacing(10);

    m_fileLabel = new QLabel(this);
    m_fileLabel->setStyleSheet("font-size: 13px; color: #0F172A; font-family: 'Consolas', 'Menlo', 'Monaco', 'DejaVu Sans Mono', 'Liberation Mono', 'Courier New', monospace; border: none; background: transparent;");
    if (!m_filePath.isEmpty()) {
        m_fileLabel->setText(m_filePath);
    } else {
        m_fileLabel->setText("No file selected (Click browse to select Data.*** or .mdb)");
        m_fileLabel->setStyleSheet("font-size: 13px; color: #94A3B8; border: none; background: transparent;");
    }
    fileLayout->addWidget(m_fileLabel, 1);

    m_browseBtn = new QPushButton("Browse File...", fileSectionFrame);
    m_browseBtn->setFixedSize(120, 34);
    m_browseBtn->setCursor(Qt::PointingHandCursor);
    m_browseBtn->setStyleSheet(
        "QPushButton { background-color: #FFFFFF; border: 1px solid #CBD5E1; border-radius: 6px; padding: 0px 12px; font-weight: 700; color: #0F172A; font-size: 12px; }"
        "QPushButton:hover { background-color: #F1F5F9; border-color: #94A3B8; }"
    );
    connect(m_browseBtn, &QPushButton::clicked, this, &MdbMigrationDialog::onBrowseClicked);
    fileLayout->addWidget(m_browseBtn);

    root->addWidget(fileSectionFrame);

    // ========================================================================
    // 3. Preview & Inspection Card (Matching MdbMigrationModal.qml lines 128-238)
    // ========================================================================
    QFrame* previewCard = new QFrame(this);
    previewCard->setStyleSheet("QFrame { background-color: #FFFFFF; border: 1px solid #E2E8F0; border-radius: 10px; }");
    QVBoxLayout* previewCardLayout = new QVBoxLayout(previewCard);
    previewCardLayout->setContentsMargins(14, 12, 14, 12);
    previewCardLayout->setSpacing(10);

    QLabel* detectedInfoTitle = new QLabel("📊 Detected Database Information", previewCard);
    detectedInfoTitle->setStyleSheet("font-size: 13px; font-weight: 700; color: #334155; border: none; background: transparent;");
    previewCardLayout->addWidget(detectedInfoTitle);

    m_previewStack = new QStackedWidget(previewCard);

    // Page 0: Empty / Placeholder / Error View
    QWidget* emptyPage = new QWidget(m_previewStack);
    QVBoxLayout* emptyLayout = new QVBoxLayout(emptyPage);
    emptyLayout->setContentsMargins(0, 20, 0, 20);
    emptyLayout->setAlignment(Qt::AlignCenter);
    m_emptyPlaceholderLabel = new QLabel("Select a Bahi-Khata database file (e.g. Data.***, *.mdb) to inspect and migrate", emptyPage);
    m_emptyPlaceholderLabel->setWordWrap(true);
    m_emptyPlaceholderLabel->setAlignment(Qt::AlignCenter);
    m_emptyPlaceholderLabel->setStyleSheet("font-size: 13px; color: #64748B; border: none; background: transparent;");
    emptyLayout->addWidget(m_emptyPlaceholderLabel);
    m_previewStack->addWidget(emptyPage);

    // Page 1: 4 Stat Cards in 2x2 Grid
    QWidget* statsPage = new QWidget(m_previewStack);
    QGridLayout* grid = new QGridLayout(statsPage);
    grid->setContentsMargins(0, 0, 0, 0);
    grid->setHorizontalSpacing(16);
    grid->setVerticalSpacing(12);

    grid->addWidget(createStatCard("Stock Transactions", &m_stockTxLabel, "#F0FDF4", "#BBF7D0", "#166534", "#15803D"), 0, 0);
    grid->addWidget(createStatCard("Milling Vouchers", &m_millingLabel, "#EFF6FF", "#BFDBFE", "#1E40AF", "#1D4ED8"), 0, 1);
    grid->addWidget(createStatCard("Stock Items", &m_stockItemsLabel, "#FAF5FF", "#E9D5FF", "#6B21A8", "#7E22CE"), 1, 0);
    grid->addWidget(createStatCard("Master Ledgers", &m_ledgersLabel, "#FFFBEB", "#FDE68A", "#92400E", "#B45309"), 1, 1);
    m_previewStack->addWidget(statsPage);

    // Page 2: Success Message View
    QWidget* successPage = new QWidget(m_previewStack);
    QVBoxLayout* successLayout = new QVBoxLayout(successPage);
    successLayout->setContentsMargins(0, 20, 0, 20);
    successLayout->setSpacing(10);
    successLayout->setAlignment(Qt::AlignCenter);

    QLabel* celebrationLabel = new QLabel("🎉", successPage);
    celebrationLabel->setAlignment(Qt::AlignCenter);
    celebrationLabel->setStyleSheet("font-size: 36px; border: none; background: transparent;");
    successLayout->addWidget(celebrationLabel);

    m_successMessageLabel = new QLabel("Migration completed successfully!", successPage);
    m_successMessageLabel->setWordWrap(true);
    m_successMessageLabel->setAlignment(Qt::AlignCenter);
    m_successMessageLabel->setStyleSheet("font-size: 14px; font-weight: 700; color: #166534; border: none; background: transparent;");
    successLayout->addWidget(m_successMessageLabel);
    m_previewStack->addWidget(successPage);

    previewCardLayout->addWidget(m_previewStack, 1);
    root->addWidget(previewCard, 1);

    // ========================================================================
    // 4. Progress Section (Matching MdbMigrationModal.qml lines 240-258)
    // ========================================================================
    m_progressContainer = new QWidget(this);
    QVBoxLayout* progLayout = new QVBoxLayout(m_progressContainer);
    progLayout->setContentsMargins(0, 0, 0, 0);
    progLayout->setSpacing(6);

    QHBoxLayout* progHeader = new QHBoxLayout();
    m_statusLabel = new QLabel("Migrating transactions...", m_progressContainer);
    m_statusLabel->setStyleSheet("font-size: 12px; font-weight: 700; color: #2563EB; border: none; background: transparent;");
    progHeader->addWidget(m_statusLabel);

    progHeader->addStretch(1);

    m_progressPercentLabel = new QLabel("0%", m_progressContainer);
    m_progressPercentLabel->setStyleSheet("font-size: 12px; font-weight: 700; color: #2563EB; border: none; background: transparent;");
    progHeader->addWidget(m_progressPercentLabel);
    progLayout->addLayout(progHeader);

    m_progressBar = new QProgressBar(m_progressContainer);
    m_progressBar->setFixedHeight(14);
    m_progressBar->setRange(0, 100);
    m_progressBar->setValue(0);
    m_progressBar->setTextVisible(false);
    m_progressBar->setStyleSheet(
        "QProgressBar {"
        "  background-color: #F1F5F9;"
        "  border: 1px solid #CBD5E1;"
        "  border-radius: 7px;"
        "}"
        "QProgressBar::chunk {"
        "  background-color: #2563EB;"
        "  border-radius: 6px;"
        "}"
    );
    progLayout->addWidget(m_progressBar);
    m_progressContainer->setVisible(false);
    root->addWidget(m_progressContainer);

    // ========================================================================
    // 5. Footer Action Buttons (Matching MdbMigrationModal.qml lines 260-290)
    // ========================================================================
    QHBoxLayout* footer = new QHBoxLayout();
    footer->setSpacing(12);

    m_closeBtn = new QPushButton("Cancel", this);
    m_closeBtn->setFixedSize(100, 36);
    m_closeBtn->setCursor(Qt::PointingHandCursor);
    m_closeBtn->setStyleSheet(
        "QPushButton { background-color: #F1F5F9; border: 1px solid #CBD5E1; border-radius: 6px; padding: 0px 16px; font-weight: 700; color: #475569; font-size: 12px; }"
        "QPushButton:hover { background-color: #E2E8F0; }"
    );
    connect(m_closeBtn, &QPushButton::clicked, this, &QDialog::reject);
    footer->addWidget(m_closeBtn);

    footer->addStretch(1);

    m_startBtn = new QPushButton("Start In-App Migration", this);
    m_startBtn->setFixedSize(180, 36);
    m_startBtn->setCursor(Qt::PointingHandCursor);
    m_startBtn->setStyleSheet(
        "QPushButton { background-color: #2563EB; border: none; border-radius: 6px; padding: 0px 18px; font-weight: 700; color: #FFFFFF; font-size: 12px; }"
        "QPushButton:hover { background-color: #1D4ED8; }"
        "QPushButton:disabled { background-color: #94A3B8; }"
    );
    connect(m_startBtn, &QPushButton::clicked, this, &MdbMigrationDialog::onStartMigrationClicked);
    footer->addWidget(m_startBtn);

    root->addLayout(footer);

    if (m_migrator) {
        connect(m_migrator, &BahiKhataMigrator::migrationProgress, this, &MdbMigrationDialog::onMigrationProgress);
        connect(m_migrator, &BahiKhataMigrator::migrationFinished, this, &MdbMigrationDialog::onMigrationFinished);
    }
}

void MdbMigrationDialog::onBrowseClicked() {
    QString startDir = "/Users/karan/Firm Data";
    if (!m_filePath.isEmpty()) {
        startDir = QFileInfo(m_filePath).absolutePath();
    }
    QString picked = QFileDialog::getOpenFileName(this, "Select Bahi-Khata Database File", startDir, "Bahi-Khata Databases (Data.* *.0* *.mdb *.accdb);;All Files (*.*)");
    if (!picked.isEmpty()) {
        m_filePath = picked;
        m_fileLabel->setText(m_filePath);
        m_fileLabel->setStyleSheet("font-size: 13px; color: #0F172A; font-family: 'Consolas', 'Menlo', 'Monaco', 'DejaVu Sans Mono', 'Liberation Mono', 'Courier New', monospace; border: none; background: transparent;");
        inspectFile();
    }
}

void MdbMigrationDialog::inspectFile() {
    if (!m_migrator || m_filePath.isEmpty()) {
        m_previewStack->setCurrentIndex(0);
        m_startBtn->setVisible(false);
        return;
    }

    m_inspectionData = m_migrator->inspect_mdb_file(m_filePath);
    bool valid = m_inspectionData.value("valid", false).toBool();

    if (valid) {
        int txCount = m_inspectionData.value("stockTxCount", 0).toInt();
        int millingCount = m_inspectionData.value("millingCount", 0).toInt();
        int itmCount = m_inspectionData.value("stockItemsCount", 0).toInt();
        int ledgersCount = m_inspectionData.value("ledgersCount", 0).toInt();

        if (m_stockTxLabel) m_stockTxLabel->setText(QString::number(txCount));
        if (m_millingLabel) m_millingLabel->setText(QString::number(millingCount));
        if (m_stockItemsLabel) m_stockItemsLabel->setText(QString::number(itmCount));
        if (m_ledgersLabel) m_ledgersLabel->setText(QString::number(ledgersCount));

        m_previewStack->setCurrentIndex(1);
        m_startBtn->setVisible(true);
        m_startBtn->setEnabled(true);
    } else {
        QString err = m_inspectionData.value("error").toString();
        m_emptyPlaceholderLabel->setText(err.isEmpty() ? "Unable to open Jet database file. Ensure it is a valid .mdb / Data.*** file." : err);
        m_emptyPlaceholderLabel->setStyleSheet("font-size: 13px; font-weight: 700; color: #DC2626; border: none; background: transparent;");
        m_previewStack->setCurrentIndex(0);
        m_startBtn->setVisible(false);
    }
}

void MdbMigrationDialog::onStartMigrationClicked() {
    if (!m_migrator || m_filePath.isEmpty()) return;

    m_startBtn->setEnabled(false);
    m_closeBtn->setEnabled(false);
    m_browseBtn->setEnabled(false);
    m_headerCloseBtn->setEnabled(false);
    m_progressContainer->setVisible(true);
    m_statusLabel->setText("Starting database migration...");
    m_progressPercentLabel->setText("5%");
    m_progressBar->setValue(5);

    if (m_firmMgr) {
        m_firmMgr->prepare_firm_for_import(m_filePath, m_firmName, m_firmId);
    }

    QTimer::singleShot(50, this, [this]() {
        bool ok = m_migrator->migrate_mdb_file(m_filePath);
        if (!ok) {
            onMigrationFinished(false, "Migration failed during data processing.");
        }
    });
}

void MdbMigrationDialog::onMigrationProgress(int percent, const QString& currentStep) {
    m_progressBar->setValue(percent);
    m_progressPercentLabel->setText(QString::number(percent) + "%");
    m_statusLabel->setText(currentStep);
}

void MdbMigrationDialog::onMigrationFinished(bool success, const QString& summaryMessage) {
    m_hasCompleted = true;
    m_progressBar->setValue(100);
    m_progressPercentLabel->setText("100%");
    m_closeBtn->setEnabled(true);
    m_closeBtn->setText("Close");
    m_headerCloseBtn->setEnabled(true);

    if (success) {
        m_previewStack->setCurrentIndex(2);
        m_successMessageLabel->setText(summaryMessage.isEmpty() ? "Migration completed successfully!" : summaryMessage);
        m_progressContainer->setVisible(false);

        m_startBtn->setText("Open Company Now");
        m_startBtn->setStyleSheet(
            "QPushButton { background-color: #16A34A; border: none; border-radius: 6px; padding: 0px 20px; font-weight: 700; color: #FFFFFF; font-size: 12px; }"
            "QPushButton:hover { background-color: #15803D; }"
        );
        m_startBtn->setEnabled(true);
        disconnect(m_startBtn, &QPushButton::clicked, this, &MdbMigrationDialog::onStartMigrationClicked);
        connect(m_startBtn, &QPushButton::clicked, this, [this]() {
            emit migrationCompleted(m_firmId, m_firmName);
            accept();
        });
    } else {
        m_statusLabel->setStyleSheet("font-size: 12px; font-weight: 700; color: #DC2626; border: none; background: transparent;");
        m_statusLabel->setText(summaryMessage.isEmpty() ? "Migration failed." : summaryMessage);
        m_startBtn->setEnabled(true);
        m_startBtn->setText("Retry Migration");
        m_browseBtn->setEnabled(true);
    }
}

void MdbMigrationDialog::keyPressEvent(QKeyEvent* event) {
    if (event->key() == Qt::Key_Escape) {
        if (m_closeBtn->isEnabled()) {
            reject();
        }
        event->accept();
        return;
    } else if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
        if (m_startBtn->isEnabled() && m_startBtn->isVisible()) {
            m_startBtn->click();
        }
        event->accept();
        return;
    }
    QDialog::keyPressEvent(event);
}

