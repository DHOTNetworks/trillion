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

QString BahiKhataExportDialog::bahiKhataDir() {
    QDir d(QDir::current().filePath("Bahi-Khata"));
    if (!d.exists()) d.mkpath(".");
    return d.absolutePath();
}

QStringList BahiKhataExportDialog::scanDatabaseFiles() {
    QDir d(bahiKhataDir());
    static const QRegularExpression dataRegex(R"(^Data\.\d{3}$)", QRegularExpression::CaseInsensitiveOption);
    QStringList out;
    const QStringList found = d.entryList({"Data.*"}, QDir::Files, QDir::Name);
    for (const QString& f : found) {
        if (dataRegex.match(f).hasMatch()) out << f;
    }
    return out;
}

QString BahiKhataExportDialog::resolvedTargetPath() const {
    QString name = m_nameEdit ? m_nameEdit->text().trimmed() : QString();
    if (name.isEmpty()) return QString();
    name = QFileInfo(name).fileName();
    QString dir;
    if (m_seedPathEdit && !m_seedPathEdit->text().trimmed().isEmpty()) {
        dir = QFileInfo(m_seedPathEdit->text().trimmed()).absolutePath();
    }
    if (dir.isEmpty() || !QDir(dir).exists()) {
        dir = bahiKhataDir();
    }
    return QDir::toNativeSeparators(QDir(dir).filePath(name));
}

void BahiKhataExportDialog::updateDestinationPreview() {
    if (!m_destPathLabel) return;
    QString target = resolvedTargetPath();
    if (target.isEmpty()) {
        m_destPathLabel->setText("Destination: <font color='#94A3B8'><i>Enter file name above</i></font>");
    } else {
        bool exists = QFile::exists(target);
        QString statusTag = exists
            ? "<font color='#D97706'>[Existing file — will update / overwrite]</font>"
            : "<font color='#16A34A'>[New file — will clone from seed template]</font>";
        m_destPathLabel->setText(QString("Destination: <b>%1</b> %2").arg(target, statusTag));
    }
}

QString BahiKhataExportDialog::suggestDefaultFileName() const {
    if (m_firmMgr) {
        // 1. Explicit per-firm mapping persisted from previous exports.
        QVariantMap reg = m_firmMgr->currentFirmRegistryEntry();
        QString mapped = reg.value("bahi_khata_file").toString().trimmed();
        if (!mapped.isEmpty()) {
            QFileInfo fi(mapped);
            return fi.isAbsolute() ? QDir::toNativeSeparators(mapped)
                                   : QDir(bahiKhataDir()).filePath(fi.fileName());
        }
        // 2. Legacy source_file field when it already names a Data.* file.
        QString src = reg.value("source_file").toString();
        if (!src.isEmpty() && src.startsWith("Data.", Qt::CaseInsensitive)) {
            return QDir(bahiKhataDir()).filePath(QFileInfo(src).fileName());
        }
        // 3. Derive from sqlite db name / firm id ("*_data_002.db" -> Data.002).
        QString derived = FirmManager::deriveBahiKhataFileName(
            reg.value("db_name").toString(), m_firmMgr->currentFirmId());
        if (!derived.isEmpty()) {
            QString p = QDir(bahiKhataDir()).filePath(derived);
            if (QFile::exists(p)) return p;
        }
    }
    QString p002 = QDir(bahiKhataDir()).filePath("Data.002");
    if (QFile::exists(p002)) return p002;
    const QStringList files = scanDatabaseFiles();
    if (!files.isEmpty()) return QDir(bahiKhataDir()).filePath(files.first());
    return p002;
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

    // 1. Seed / Template Database File Selector (file selector with Browse button)
    QLabel* seedLabel = new QLabel("Seed / Template Database File:", formCard);
    seedLabel->setStyleSheet("font-size: 12px; font-weight: 700; color: #334155;");
    formLayout->addWidget(seedLabel);

    QHBoxLayout* seedRow = new QHBoxLayout();
    seedRow->setSpacing(8);

    m_seedPathEdit = new QLineEdit(formCard);
    QString defSeed = suggestDefaultFileName();
    m_seedPathEdit->setText(defSeed);
    m_seedPathEdit->setFixedHeight(36);
    m_seedPathEdit->setStyleSheet(
        "QLineEdit { background-color: #F8FAFC; border: 1.5px solid #CBD5E1; border-radius: 6px; padding: 0px 10px; font-size: 12px; font-weight: 600; color: #0F172A; }"
        "QLineEdit:focus { border-color: #2563EB; background-color: #FFFFFF; }"
    );
    seedRow->addWidget(m_seedPathEdit, 1);

    m_browseSeedBtn = new QPushButton("Browse...", formCard);
    m_browseSeedBtn->setFixedHeight(36);
    m_browseSeedBtn->setCursor(Qt::PointingHandCursor);
    m_browseSeedBtn->setStyleSheet(
        "QPushButton { background-color: #EFF6FF; border: 1.5px solid #93C5FD; border-radius: 6px; padding: 0px 14px; font-size: 12px; font-weight: 700; color: #1D4ED8; }"
        "QPushButton:hover { background-color: #DBEAFE; border-color: #3B82F6; }"
    );
    connect(m_browseSeedBtn, &QPushButton::clicked, this, &BahiKhataExportDialog::onBrowseSeedClicked);
    seedRow->addWidget(m_browseSeedBtn);
    formLayout->addLayout(seedRow);

    // 2. Export File Name Field (just the field to edit the name of the file being exported as)
    QLabel* nameLabel = new QLabel("Export As File Name (Data.NNN):", formCard);
    nameLabel->setStyleSheet("font-size: 12px; font-weight: 700; color: #334155;");
    formLayout->addWidget(nameLabel);

    m_nameEdit = new QLineEdit(formCard);
    m_nameEdit->setFixedHeight(36);
    m_nameEdit->setPlaceholderText("Data.002");
    if (!defSeed.isEmpty()) {
        m_nameEdit->setText(QFileInfo(defSeed).fileName());
    } else {
        m_nameEdit->setText("Data.002");
    }
    m_nameEdit->setStyleSheet(
        "QLineEdit { background-color: #F8FAFC; border: 1.5px solid #CBD5E1; border-radius: 6px; padding: 0px 10px; font-size: 12px; font-weight: 600; color: #0F172A; }"
        "QLineEdit:focus { border-color: #2563EB; background-color: #FFFFFF; }"
    );
    formLayout->addWidget(m_nameEdit);
    connect(m_nameEdit, &QLineEdit::textChanged,
            this, &BahiKhataExportDialog::onExportNameChanged);

    // Destination Path Preview
    m_destPathLabel = new QLabel(formCard);
    m_destPathLabel->setWordWrap(true);
    m_destPathLabel->setStyleSheet("font-size: 11px; color: #64748B; margin-top: -2px; margin-bottom: 2px;");
    formLayout->addWidget(m_destPathLabel);
    updateDestinationPreview();

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

    // 4. Interoperability & Legal Compliance Notice
    QFrame* legalCard = new QFrame(this);
    legalCard->setStyleSheet("QFrame { background-color: #F8FAFC; border: 1px dashed #CBD5E1; border-radius: 8px; }");
    QHBoxLayout* legalLayout = new QHBoxLayout(legalCard);
    legalLayout->setContentsMargins(12, 8, 12, 8);
    legalLayout->setSpacing(8);

    QLabel* legalIcon = new QLabel("⚖️", legalCard);
    legalIcon->setStyleSheet("font-size: 14px; border: none; background: transparent;");
    legalLayout->addWidget(legalIcon);

    QLabel* legalText = new QLabel(
        "<b>Interoperability Notice:</b> This utility processes accounting records solely for format interoperability "
        "and data portability as protected under <b>Section 52(1)(ab) & (ac) of the Indian Copyright Act, 1957</b>. "
        "All exported transactional and ledger data remains the exclusive property of the respective business entity.",
        legalCard
    );
    legalText->setWordWrap(true);
    legalText->setStyleSheet("font-size: 11px; color: #64748B; border: none; background: transparent;");
    legalLayout->addWidget(legalText, 1);

    rootLayout->addWidget(legalCard);

    // 5. Action Buttons
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

void BahiKhataExportDialog::onBrowseSeedClicked() {
    QString initial = m_seedPathEdit ? m_seedPathEdit->text().trimmed() : QString();
    QString dir = initial.isEmpty() ? bahiKhataDir() : QFileInfo(initial).absolutePath();
    QString chosen = QFileDialog::getOpenFileName(
        this,
        "Select Seed / Template Bahi-Khata Database File (Data.00x)",
        dir,
        "Bahi-Khata Files (Data.* *.mdb);;All Files (*.*)"
    );
    if (!chosen.isEmpty()) {
        chosen = QDir::toNativeSeparators(chosen);
        m_seedPathEdit->setText(chosen);
        QFileInfo fi(chosen);
        if (m_nameEdit) {
            QString curName = m_nameEdit->text().trimmed();
            if (curName.isEmpty() || curName.startsWith("Data.")) {
                m_nameEdit->blockSignals(true);
                m_nameEdit->setText(fi.fileName());
                m_nameEdit->blockSignals(false);
            }
        }
        updateDestinationPreview();
    }
}

void BahiKhataExportDialog::onExportNameChanged(const QString& text) {
    QString name = text.trimmed();
    if (!name.isEmpty()) {
        QString base = QFileInfo(name).fileName();
        if (base != text) {
            m_nameEdit->blockSignals(true);
            m_nameEdit->setText(base);
            m_nameEdit->blockSignals(false);
        }
    }
    updateDestinationPreview();
}

void BahiKhataExportDialog::onExportClicked() {
    QString targetPath = resolvedTargetPath();
    QString err;
    if (!validateFileName(targetPath, err)) {
        QMessageBox::warning(this, "Invalid File Name", err);
        return;
    }
    QString seedPath = m_seedPathEdit ? m_seedPathEdit->text().trimmed() : QString();
    if (!seedPath.isEmpty() && !QFile::exists(seedPath) && !QFile::exists(targetPath)) {
        QMessageBox::warning(this, "Seed File Not Found",
            QString("The specified seed file '%1' does not exist.\nPlease select an existing seed/template database file.").arg(seedPath));
        return;
    }
    if (m_isExporting) return;

    m_isExporting = true;
    m_exportBtn->setEnabled(false);
    m_browseSeedBtn->setEnabled(false);
    m_seedPathEdit->setEnabled(false);
    m_nameEdit->setEnabled(false);
    m_closeBtn->setEnabled(false);
    m_progressBar->setValue(2);
    m_statusLabel->setText("Starting export on background thread...");

    BahiKhataExporter::ExportOptions opts;
    opts.targetMdbPath = targetPath;
    opts.templateMdbPath = seedPath;
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
    m_browseSeedBtn->setEnabled(true);
    m_seedPathEdit->setEnabled(true);
    m_nameEdit->setEnabled(true);
    m_closeBtn->setEnabled(true);
    updateDestinationPreview();
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
    // Remember this target for the active firm so the next export defaults
    // to the same Data.NNN file.
    if (m_firmMgr && !m_firmMgr->currentFirmId().isEmpty()) {
        m_firmMgr->setFirmBahiKhataFile(m_firmMgr->currentFirmId(),
                                        QFileInfo(summary.targetFilePath).fileName());
    }
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
    m_browseSeedBtn->setEnabled(true);
    m_seedPathEdit->setEnabled(true);
    m_nameEdit->setEnabled(true);
    m_closeBtn->setEnabled(true);
    updateDestinationPreview();

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
