#include "mdb_migration_dialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFrame>
#include <QFileInfo>
#include <QKeyEvent>
#include <QTimer>

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
    setWindowTitle("Bahi-Khata Database Migration");
    setModal(true);
    setFixedSize(620, 380);
    setupUi();
    inspectFile();
}

void MdbMigrationDialog::setupUi() {
    setAttribute(Qt::WA_StyledBackground, true);
    setStyleSheet("MdbMigrationDialog { background-color: #FFFFFF; }");

    QVBoxLayout* root = new QVBoxLayout(this);
    root->setContentsMargins(20, 18, 20, 18);
    root->setSpacing(14);

    // 1. Header
    QHBoxLayout* header = new QHBoxLayout();
    header->setSpacing(12);

    QFrame* iconFrame = new QFrame(this);
    iconFrame->setFixedSize(36, 36);
    iconFrame->setStyleSheet("background-color: #EFF6FF; border: 1px solid #BFDBFE; border-radius: 8px;");
    QLabel* iconLabel = new QLabel("📥", iconFrame);
    iconLabel->setAlignment(Qt::AlignCenter);
    iconLabel->setGeometry(0, 0, 36, 36);
    header->addWidget(iconFrame);

    QVBoxLayout* titleBox = new QVBoxLayout();
    titleBox->setSpacing(2);
    QLabel* titleLabel = new QLabel("IMPORT BAHI-KHATA DATABASE", this);
    titleLabel->setStyleSheet("font-size: 15px; font-weight: 800; color: #0F172A; letter-spacing: 0.5px;");
    titleBox->addWidget(titleLabel);

    QLabel* subLabel = new QLabel("Directly converts MS Access Data.* into native SQLite ledger records.", this);
    subLabel->setStyleSheet("font-size: 11px; color: #64748B;");
    titleBox->addWidget(subLabel);
    header->addLayout(titleBox);
    root->addLayout(header);

    QFrame* div1 = new QFrame(this);
    div1->setFixedHeight(1);
    div1->setStyleSheet("background-color: #E2E8F0; border: none;");
    root->addWidget(div1);

    // 2. Info Card
    QFrame* card = new QFrame(this);
    card->setStyleSheet("background-color: #F8FAFC; border: 1px solid #E2E8F0; border-radius: 8px;");
    QVBoxLayout* cardLayout = new QVBoxLayout(card);
    cardLayout->setContentsMargins(14, 12, 14, 12);
    cardLayout->setSpacing(8);

    m_fileLabel = new QLabel(QString("Source File: %1").arg(QFileInfo(m_filePath).fileName()), card);
    m_fileLabel->setStyleSheet("font-size: 12px; font-weight: 700; color: #0F172A; font-family: 'Consolas', monospace; border: none; background: transparent;");
    cardLayout->addWidget(m_fileLabel);

    m_tablesInfoLabel = new QLabel("Inspecting database structure...", card);
    m_tablesInfoLabel->setStyleSheet("font-size: 11px; color: #475569; border: none; background: transparent;");
    cardLayout->addWidget(m_tablesInfoLabel);

    root->addWidget(card);

    // 3. Progress Section
    QVBoxLayout* progLayout = new QVBoxLayout();
    progLayout->setSpacing(6);

    QHBoxLayout* statusHeader = new QHBoxLayout();
    m_statusLabel = new QLabel("Ready to migrate.", this);
    m_statusLabel->setStyleSheet("font-size: 12px; font-weight: 700; color: #2563EB; border: none; background: transparent;");
    statusHeader->addWidget(m_statusLabel);
    progLayout->addLayout(statusHeader);

    m_progressBar = new QProgressBar(this);
    m_progressBar->setFixedHeight(20);
    m_progressBar->setRange(0, 100);
    m_progressBar->setValue(0);
    m_progressBar->setTextVisible(true);
    m_progressBar->setStyleSheet(
        "QProgressBar {"
        "  background-color: #F1F5F9;"
        "  border: 1px solid #CBD5E1;"
        "  border-radius: 6px;"
        "  text-align: center;"
        "  font-size: 11px;"
        "  font-weight: 700;"
        "  color: #1E293B;"
        "}"
        "QProgressBar::chunk {"
        "  background-color: #2563EB;"
        "  border-radius: 5px;"
        "}"
    );
    progLayout->addWidget(m_progressBar);
    root->addLayout(progLayout);

    root->addStretch(1);

    // 4. Buttons
    QHBoxLayout* footer = new QHBoxLayout();
    footer->setSpacing(12);

    m_closeBtn = new QPushButton("Cancel (Esc)", this);
    m_closeBtn->setFixedHeight(36);
    m_closeBtn->setCursor(Qt::PointingHandCursor);
    m_closeBtn->setStyleSheet(
        "QPushButton { background-color: #F1F5F9; border: 1px solid #CBD5E1; border-radius: 6px; padding: 0px 16px; font-weight: 700; color: #475569; font-size: 12px; }"
        "QPushButton:hover { background-color: #E2E8F0; }"
    );
    connect(m_closeBtn, &QPushButton::clicked, this, &QDialog::reject);
    footer->addWidget(m_closeBtn);

    footer->addStretch(1);

    m_startBtn = new QPushButton("Start Migration (Enter)", this);
    m_startBtn->setFixedHeight(36);
    m_startBtn->setCursor(Qt::PointingHandCursor);
    m_startBtn->setStyleSheet(
        "QPushButton { background-color: #2563EB; border: none; border-radius: 6px; padding: 0px 20px; font-weight: 700; color: #FFFFFF; font-size: 12px; }"
        "QPushButton:hover { background-color: #1D4ED8; }"
    );
    connect(m_startBtn, &QPushButton::clicked, this, &MdbMigrationDialog::onStartMigrationClicked);
    footer->addWidget(m_startBtn);

    root->addLayout(footer);

    if (m_migrator) {
        connect(m_migrator, &BahiKhataMigrator::migrationProgress, this, &MdbMigrationDialog::onMigrationProgress);
        connect(m_migrator, &BahiKhataMigrator::migrationFinished, this, &MdbMigrationDialog::onMigrationFinished);
    }
}

void MdbMigrationDialog::inspectFile() {
    if (!m_migrator || m_filePath.isEmpty()) return;

    QVariantMap res = m_migrator->inspect_mdb_file(m_filePath);
    int accCount = res.value("accounts_count").toInt();
    int vouCount = res.value("vouchers_count").toInt();
    int itmCount = res.value("items_count").toInt();

    m_tablesInfoLabel->setText(
        QString("Detected: %1 Ledger Accounts • %2 Vouchers & Transactions • %3 Stock Items")
            .arg(accCount).arg(vouCount).arg(itmCount)
    );
}

void MdbMigrationDialog::onStartMigrationClicked() {
    if (!m_migrator) return;

    m_startBtn->setEnabled(false);
    m_closeBtn->setEnabled(false);
    m_statusLabel->setText("Starting database migration...");
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
    m_statusLabel->setText(currentStep);
}

void MdbMigrationDialog::onMigrationFinished(bool success, const QString& summaryMessage) {
    m_progressBar->setValue(100);
    m_closeBtn->setEnabled(true);
    m_closeBtn->setText("Close (Esc)");

    if (success) {
        m_statusLabel->setStyleSheet("font-size: 12px; font-weight: 700; color: #16A34A; border: none; background: transparent;");
        m_statusLabel->setText("Migration completed successfully!");
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
        if (m_startBtn->isEnabled()) {
            m_startBtn->click();
        }
        event->accept();
        return;
    }
    QDialog::keyPressEvent(event);
}
