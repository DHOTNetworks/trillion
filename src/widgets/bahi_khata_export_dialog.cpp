#include "bahi_khata_export_dialog.h"
#include "../engine/fiscal_year_helper.h"
#include "../engine/jet4_writer.h"
#include "../database_manager.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QFrame>
#include <QFileDialog>
#include <QFileInfo>
#include <QDir>
#include <QRegularExpression>
#include <QKeyEvent>
#include <QMessageBox>
#include <QThread>
#include <QDebug>
#if defined(HAS_LIBMDB) || __has_include("mdbtools.h")
#include "mdbtools.h"
#endif

namespace MahadevERP {

BahiKhataExportWorker::BahiKhataExportWorker(BahiKhataExporter::ExportOptions opts, bool verify,
                                             QObject* parent)
    : QObject(parent)
    , m_opts(std::move(opts))
    , m_verify(verify)
{
}

QString BahiKhataExportWorker::runVerify(const QString& mdbPath) {
#if defined(HAS_LIBMDB) || __has_include("mdbtools.h")
    QByteArray pathBytes = QFile::encodeName(mdbPath);
    MdbHandle* mdb = mdb_open(pathBytes.constData(), MDB_NOFLAGS);
    if (!mdb) mdb = mdb_open(mdbPath.toUtf8().constData(), MDB_NOFLAGS);
    if (!mdb) return QString("Verify skipped: cannot open %1").arg(mdbPath);
    Jet4Writer::VerifyReport rep = Jet4Writer::verifyDatabase(mdb);
    mdb_close(mdb);
    if (!rep.error.empty())
        return QString("Verify FAILED: %1").arg(QString::fromStdString(rep.error));
    QStringList badIdx;
    for (const auto& vi : rep.indexes) {
        if (!vi.sorted || vi.walked != vi.dataRows)
            badIdx << QString("%1.%2").arg(QString::fromStdString(vi.table),
                                           QString::fromStdString(vi.name));
    }
    if (rep.badRows > 0 || !badIdx.isEmpty() || !rep.ok) {
        return QString("Verify FAILED: %1 bad rows, indexes off: %2")
            .arg(rep.badRows).arg(badIdx.join(", "));
    }
    long idxCount = static_cast<long>(rep.indexes.size());
    return QString("Verified: %1 tables, %2 rows, %3 indexes (walked == rows, sorted).")
        .arg(rep.tablesChecked).arg(rep.dataRows).arg(idxCount);
#else
    Q_UNUSED(mdbPath);
    return QString("Verify skipped: libmdb unavailable.");
#endif
}

void BahiKhataExportWorker::run() {
    BahiKhataExporter exporter; // parentless: lives on this worker thread
    connect(&exporter, &BahiKhataExporter::exportProgress,
            this, &BahiKhataExportWorker::progress);
    BahiKhataExporter::ExportSummary summary = exporter.exportToBahiKhata(m_opts);
    QString verifyText;
    if (summary.success && m_verify) {
        emit progress(98, "Verifying Jet indexes...");
        verifyText = runVerify(summary.targetFilePath);
    }
    emit finished(summary, verifyText);
}

BahiKhataExportDialog::BahiKhataExportDialog(FirmManager* firmMgr, QWidget* parent)
    : QDialog(parent)
    , m_firmMgr(firmMgr)
    , m_exporter(new BahiKhataExporter(this))
{
    qRegisterMetaType<BahiKhataExporter::ExportSummary>();
    setWindowTitle("Export Database to Bahi-Khata (Data.00x)");
    setFixedWidth(640);
    setModal(true);
    setStyleSheet("QDialog { background-color: #F8FAFC; } QLabel { border: none; background: transparent; }");

    // NOTE: actual exports run on BahiKhataExportWorker (off the UI thread);
    // m_exporter is retained for API compatibility only.
    connect(m_exporter, &BahiKhataExporter::exportProgress, this, &BahiKhataExportDialog::onExportProgress);
    connect(m_exporter, &BahiKhataExporter::exportFinished, this, &BahiKhataExportDialog::onExportFinished);

    setupUi();
}

BahiKhataExportDialog::~BahiKhataExportDialog() {
    teardownWorker();
}

void BahiKhataExportDialog::teardownWorker() {
    if (m_workerThread) {
        m_workerThread->quit();
        if (!m_workerThread->wait(60000))
            m_workerThread->terminate();
        // Worker deletes itself via deleteLater; thread deletes via connection.
        m_workerThread = nullptr;
        m_worker = nullptr;
    }
}

void BahiKhataExportDialog::keyPressEvent(QKeyEvent* event) {
    if (event->key() == Qt::Key_Escape && !m_isExporting) {
        reject();
        return;
    }
    QDialog::keyPressEvent(event);
}

QString BahiKhataExportDialog::suggestDefaultFileName() const {
    QString defName = "Data.002";
    if (m_firmMgr) {
        QVariantMap info = m_firmMgr->currentFirmInfo();
        QString src = info.value("source_file").toString();
        if (!src.isEmpty() && src.startsWith("Data.", Qt::CaseInsensitive)) {
            defName = src;
        } else {
            QString firmId = m_firmMgr->currentFirmId();
            if (firmId.contains("004")) defName = "Data.004";
            else if (firmId.contains("001")) defName = "Data.001";
            else if (firmId.contains("005")) defName = "Data.005";
            else if (firmId.contains("018")) defName = "Data.018";
        }
    }

    // Default target directory
    QString targetDir = "Bahi-Khata-Data";
    if (!QDir(targetDir).exists()) {
        targetDir = QDir::homePath();
    }
    return QDir(targetDir).filePath(defName);
}

bool BahiKhataExportDialog::validateFileName(const QString& path, QString& outError) const {
    if (path.trimmed().isEmpty()) {
        outError = "Please select a valid target destination path.";
        return false;
    }

    QFileInfo fi(path);
    QString base = fi.fileName();

    // Must match Data.001, Data.002, etc. or .mdb
    static const QRegularExpression dataRegex(R"(^Data\.\d{3}$)", QRegularExpression::CaseInsensitiveOption);
    if (!dataRegex.match(base).hasMatch() && !base.endsWith(".mdb", Qt::CaseInsensitive)) {
        outError = QString("Invalid file naming: '%1'.\nBahi-Khata requires database files named in the standard pattern: 'Data.001', 'Data.002', 'Data.004', etc.").arg(base);
        return false;
    }

    return true;
}

void BahiKhataExportDialog::setupUi() {
    QVBoxLayout* rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(20, 20, 20, 20);
    rootLayout->setSpacing(16);

    // 1. Header Card
    QFrame* headerCard = new QFrame(this);
    headerCard->setStyleSheet("QFrame { background-color: #FFFFFF; border: 1px solid #E2E8F0; border-radius: 10px; }");
    QVBoxLayout* headerLayout = new QVBoxLayout(headerCard);
    headerLayout->setContentsMargins(16, 14, 16, 14);
    headerLayout->setSpacing(4);

    QLabel* title = new QLabel("Export Database to Bahi-Khata JetDB", headerCard);
    title->setStyleSheet("font-size: 16px; font-weight: 800; color: #0F172A; border: none; background: transparent;");
    headerLayout->addWidget(title);

    QString firmDesc = "Active Firm: ";
    if (m_firmMgr) {
        firmDesc += m_firmMgr->currentFirmName();
    } else {
        firmDesc += "Mahadev Rice Mill";
    }
    QLabel* sub = new QLabel(firmDesc, headerCard);
    sub->setStyleSheet("font-size: 12px; font-weight: 600; color: #64748B; border: none; background: transparent;");
    headerLayout->addWidget(sub);

    rootLayout->addWidget(headerCard);

    // 2. Settings & File Path Card
    QFrame* formCard = new QFrame(this);
    formCard->setStyleSheet("QFrame { background-color: #FFFFFF; border: 1px solid #E2E8F0; border-radius: 10px; }");
    QVBoxLayout* formLayout = new QVBoxLayout(formCard);
    formLayout->setContentsMargins(16, 16, 16, 16);
    formLayout->setSpacing(12);

    // Target File Picker
    QLabel* pathLabel = new QLabel("Target Bahi-Khata Database File (Data.***):", formCard);
    pathLabel->setStyleSheet("font-size: 12px; font-weight: 700; color: #334155;");
    formLayout->addWidget(pathLabel);

    QHBoxLayout* pathRow = new QHBoxLayout();
    pathRow->setSpacing(8);

    m_pathEdit = new QLineEdit(formCard);
    m_pathEdit->setText(suggestDefaultFileName());
    m_pathEdit->setFixedHeight(36);
    m_pathEdit->setStyleSheet(
        "QLineEdit { background-color: #F8FAFC; border: 1.5px solid #CBD5E1; border-radius: 6px; padding: 0px 10px; font-size: 12px; font-weight: 600; color: #0F172A; }"
        "QLineEdit:focus { border-color: #2563EB; background-color: #FFFFFF; }"
    );
    pathRow->addWidget(m_pathEdit, 1);

    m_browseBtn = new QPushButton("Browse...", formCard);
    m_browseBtn->setFixedHeight(36);
    m_browseBtn->setCursor(Qt::PointingHandCursor);
    m_browseBtn->setStyleSheet(
        "QPushButton { background-color: #EFF6FF; border: 1.5px solid #93C5FD; border-radius: 6px; padding: 0px 14px; font-size: 12px; font-weight: 700; color: #1D4ED8; }"
        "QPushButton:hover { background-color: #DBEAFE; border-color: #3B82F6; }"
    );
    connect(m_browseBtn, &QPushButton::clicked, this, &BahiKhataExportDialog::onBrowseClicked);
    pathRow->addWidget(m_browseBtn);
    formLayout->addLayout(pathRow);

    // Financial Year Selector
    QHBoxLayout* fyRow = new QHBoxLayout();
    fyRow->setSpacing(8);

    QLabel* fyLabel = new QLabel("Financial Year Scope:", formCard);
    fyLabel->setStyleSheet("font-size: 12px; font-weight: 700; color: #334155;");
    fyRow->addWidget(fyLabel);

    m_fyCombo = new QComboBox(formCard);
    m_fyCombo->setFixedHeight(32);
    m_fyCombo->setStyleSheet(
        "QComboBox { background-color: #F8FAFC; border: 1.5px solid #CBD5E1; border-radius: 6px; padding: 0px 10px; font-size: 12px; font-weight: 700; color: #0F172A; }"
    );
    m_fyCombo->addItem("All Financial Years", "");
    
    // Load financial years
    auto& db = DatabaseManager::instance();
    QVariantList fyRows = db.executeQuery("SELECT year_name FROM financial_years ORDER BY start_date DESC;");
    for (const auto& r : fyRows) {
        QString yn = r.toMap().value("year_name").toString();
        m_fyCombo->addItem(yn, yn);
    }
    fyRow->addWidget(m_fyCombo, 1);
    formLayout->addLayout(fyRow);

    // Checkbox Options
    m_backupCheck = new QCheckBox("Create timestamped safety backup (.bak) of target file", formCard);
    m_backupCheck->setChecked(true);
    m_backupCheck->setStyleSheet("QCheckBox { font-size: 12px; font-weight: 600; color: #334155; }");
    formLayout->addWidget(m_backupCheck);

    m_txCheck = new QCheckBox("Export all Vouchers & Double-Entry Transactions", formCard);
    m_txCheck->setChecked(true);
    m_txCheck->setStyleSheet("QCheckBox { font-size: 12px; font-weight: 600; color: #334155; }");
    formLayout->addWidget(m_txCheck);

    m_millingStockCheck = new QCheckBox("Export Stock Items, Commodity Masters & Milling Batches", formCard);
    m_millingStockCheck->setChecked(true);
    m_millingStockCheck->setStyleSheet("QCheckBox { font-size: 12px; font-weight: 600; color: #334155; }");
    formLayout->addWidget(m_millingStockCheck);

    m_verifyCheck = new QCheckBox("Verify Jet indexes after export (walk + sort check, recommended)", formCard);
    m_verifyCheck->setChecked(true);
    m_verifyCheck->setStyleSheet("QCheckBox { font-size: 12px; font-weight: 600; color: #334155; }");
    formLayout->addWidget(m_verifyCheck);

    rootLayout->addWidget(formCard);

    // 3. Progress Card
    QFrame* progCard = new QFrame(this);
    progCard->setStyleSheet("QFrame { background-color: #FFFFFF; border: 1px solid #E2E8F0; border-radius: 10px; }");
    QVBoxLayout* progLayout = new QVBoxLayout(progCard);
    progLayout->setContentsMargins(16, 14, 16, 14);
    progLayout->setSpacing(8);

    m_progressBar = new QProgressBar(progCard);
    m_progressBar->setFixedHeight(12);
    m_progressBar->setRange(0, 100);
    m_progressBar->setValue(0);
    m_progressBar->setTextVisible(false);
    m_progressBar->setStyleSheet(
        "QProgressBar { background-color: #F1F5F9; border-radius: 6px; border: none; }"
        "QProgressBar::chunk { background-color: #2563EB; border-radius: 6px; }"
    );
    progLayout->addWidget(m_progressBar);

    m_statusLabel = new QLabel("Ready to export database to Bahi-Khata JetDB format.", progCard);
    m_statusLabel->setStyleSheet("font-size: 11px; font-weight: 600; color: #64748B;");
    progLayout->addWidget(m_statusLabel);

    rootLayout->addWidget(progCard);

    // 4. Action Buttons
    QHBoxLayout* btnRow = new QHBoxLayout();
    btnRow->setSpacing(10);
    btnRow->addStretch(1);

    m_closeBtn = new QPushButton("Cancel", this);
    m_closeBtn->setFixedHeight(36);
    m_closeBtn->setCursor(Qt::PointingHandCursor);
    m_closeBtn->setStyleSheet(
        "QPushButton { background-color: #FFFFFF; border: 1.5px solid #CBD5E1; border-radius: 8px; padding: 0px 16px; font-size: 12px; font-weight: 700; color: #475569; }"
        "QPushButton:hover { background-color: #F8FAFC; border-color: #94A3B8; color: #0F172A; }"
    );
    connect(m_closeBtn, &QPushButton::clicked, this, &QDialog::reject);
    btnRow->addWidget(m_closeBtn);

    m_exportBtn = new QPushButton("Export to Bahi-Khata", this);
    m_exportBtn->setFixedHeight(36);
    m_exportBtn->setCursor(Qt::PointingHandCursor);
    m_exportBtn->setStyleSheet(
        "QPushButton { background-color: #2563EB; border: 1.5px solid #1D4ED8; border-radius: 8px; padding: 0px 20px; font-size: 12px; font-weight: 800; color: #FFFFFF; }"
        "QPushButton:hover { background-color: #1D4ED8; border-color: #1E40AF; }"
        "QPushButton:pressed { background-color: #1E40AF; }"
    );
    connect(m_exportBtn, &QPushButton::clicked, this, &BahiKhataExportDialog::onExportClicked);
    btnRow->addWidget(m_exportBtn);

    rootLayout->addLayout(btnRow);
}

void BahiKhataExportDialog::onBrowseClicked() {
    QString initial = m_pathEdit->text();
    QString dir = initial.isEmpty() ? QDir::homePath() : QFileInfo(initial).absolutePath();
    QString chosen = QFileDialog::getSaveFileName(
        this,
        "Select Bahi-Khata JetDB Destination File (Data.00x)",
        dir,
        "Bahi-Khata Files (Data.* *.001 *.002 *.004 *.005 *.018 *.mdb);;All Files (*.*)"
    );
    if (!chosen.isEmpty()) {
        m_pathEdit->setText(QDir::toNativeSeparators(chosen));
    }
}

void BahiKhataExportDialog::onExportClicked() {
    QString targetPath = m_pathEdit->text().trimmed();
    QString err;
    if (!validateFileName(targetPath, err)) {
        QMessageBox::warning(this, "Invalid File Name", err);
        return;
    }
    if (m_isExporting) return;

    m_isExporting = true;
    m_exportBtn->setEnabled(false);
    m_browseBtn->setEnabled(false);
    m_pathEdit->setEnabled(false);
    m_closeBtn->setEnabled(false);
    m_progressBar->setValue(2);
    m_statusLabel->setText("Starting export on background thread...");

    BahiKhataExporter::ExportOptions opts;
    opts.targetMdbPath = targetPath;
    opts.financialYear = m_fyCombo->currentData().toString();
    opts.createBackup = m_backupCheck->isChecked();
    opts.exportTransactions = m_txCheck->isChecked();
    opts.exportMastersOnly = !m_txCheck->isChecked();
    opts.exportMillingAndStock = m_millingStockCheck->isChecked();
    const bool doVerify = m_verifyCheck->isChecked();

    teardownWorker();
    m_workerThread = new QThread(this);
    m_worker = new BahiKhataExportWorker(opts, doVerify);
    m_worker->moveToThread(m_workerThread);
    connect(m_workerThread, &QThread::started, m_worker, &BahiKhataExportWorker::run);
    connect(m_worker, &BahiKhataExportWorker::progress,
            this, &BahiKhataExportDialog::onExportProgress);
    connect(m_worker, &BahiKhataExportWorker::finished,
            this, &BahiKhataExportDialog::onWorkerFinished);
    connect(m_worker, &BahiKhataExportWorker::finished,
            m_worker, &QObject::deleteLater);
    connect(m_workerThread, &QThread::finished,
            m_workerThread, &QObject::deleteLater);
    m_workerThread->start();
}

void BahiKhataExportDialog::onWorkerFinished(BahiKhataExporter::ExportSummary summary,
                                             const QString& verifyText) {
    m_isExporting = false;
    m_exportBtn->setEnabled(true);
    m_browseBtn->setEnabled(true);
    m_pathEdit->setEnabled(true);
    m_closeBtn->setEnabled(true);
    m_worker = nullptr;
    if (m_workerThread) {
        m_workerThread->quit();
        m_workerThread = nullptr; // deletes itself via deleteLater
    }

    if (!summary.success) {
        m_statusLabel->setText("Export Failed: " + summary.errorMessage);
        QMessageBox::critical(this, "Export Failed", summary.errorMessage);
        return;
    }
    QString msg = QString("Exported %1 ledgers and %2 transactions to Bahi-Khata JetDB at %3")
        .arg(summary.ledgersExported).arg(summary.transactionsExported).arg(summary.targetFilePath);
    if (!verifyText.isEmpty()) {
        if (verifyText.startsWith("Verify FAILED")) {
            m_progressBar->setValue(100);
            m_statusLabel->setText("Export done, but verification failed!");
            QMessageBox::warning(this, "Export Completed With Warnings",
                                 msg + "\n\n" + verifyText);
            return;
        }
        msg += "\n\n" + verifyText;
    }
    m_progressBar->setValue(100);
    m_statusLabel->setText("Export Completed Successfully!");
    QMessageBox::information(this, "Export Completed", msg);
    accept();
}

void BahiKhataExportDialog::onExportProgress(int percent, const QString& status) {
    m_progressBar->setValue(percent);
    m_statusLabel->setText(status);
}

void BahiKhataExportDialog::onExportFinished(bool success, const QString& message) {
    m_isExporting = false;
    m_exportBtn->setEnabled(true);
    m_browseBtn->setEnabled(true);
    m_pathEdit->setEnabled(true);
    m_closeBtn->setEnabled(true);

    if (success) {
        m_progressBar->setValue(100);
        m_statusLabel->setText("Export Completed Successfully!");
        QMessageBox::information(this, "Export Completed", message);
        accept();
    } else {
        m_statusLabel->setText("Export Failed: " + message);
        QMessageBox::critical(this, "Export Failed", message);
    }
}

} // namespace MahadevERP
