#pragma once

#include <QDialog>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QProgressBar>
#include <QCheckBox>
#include <QComboBox>
#include "../engine/bahi_khata_exporter.h"
#include "../models/firm_manager.h"

namespace MahadevERP {

/**
 * @brief Worker that runs the export (and optional Jet verification) off the
 * UI thread. Lives in this header so AUTOMOC picks it up.
 */
class BahiKhataExportWorker : public QObject {
    Q_OBJECT

public:
    explicit BahiKhataExportWorker(BahiKhataExporter::ExportOptions opts, bool verify,
                                   QObject* parent = nullptr);

signals:
    void progress(int percent, const QString& status);
    void finished(BahiKhataExporter::ExportSummary summary, QString verifyText);

public slots:
    void run();

private:
    static QString runVerify(const QString& mdbPath);

    BahiKhataExporter::ExportOptions m_opts;
    bool m_verify = true;
};

/**
 * @brief High-fidelity modal dialog for exporting ERP data to Bahi-Khata JetDB files (Data.001, Data.002, etc.)
 *
 * Export runs on a worker thread (the UI stays responsive); progress and the
 * final result (including the post-export Jet verification report) are
 * delivered back over queued connections.
 */
class BahiKhataExportDialog : public QDialog {
    Q_OBJECT

public:
    explicit BahiKhataExportDialog(FirmManager* firmMgr, QWidget* parent = nullptr);
    ~BahiKhataExportDialog() override;

protected:
    void keyPressEvent(QKeyEvent* event) override;

private slots:
    void onBrowseClicked();
    void onDbSelectionChanged(int index);
    void onExportNameChanged(const QString& text);
    void onExportClicked();
    void onExportProgress(int percent, const QString& status);
    void onExportFinished(bool success, const QString& message);
    void onWorkerFinished(BahiKhataExporter::ExportSummary summary, const QString& verifyText);

private:
    void setupUi();
    QString suggestDefaultFileName() const;
    bool validateFileName(const QString& path, QString& outError) const;
    void teardownWorker();

    FirmManager* m_firmMgr = nullptr;
    BahiKhataExporter* m_exporter = nullptr;

    static QString bahiKhataDir();
    static QStringList scanDatabaseFiles();
    void refreshDbList();
    QString currentTargetDir() const;

    QLineEdit* m_pathEdit = nullptr;
    QLineEdit* m_nameEdit = nullptr;
    QComboBox* m_dbCombo = nullptr;
    QPushButton* m_browseBtn = nullptr;
    QComboBox* m_fyCombo = nullptr;
    QCheckBox* m_backupCheck = nullptr;
    QCheckBox* m_txCheck = nullptr;
    QCheckBox* m_millingStockCheck = nullptr;
    QCheckBox* m_verifyCheck = nullptr;
    QProgressBar* m_progressBar = nullptr;
    QLabel* m_statusLabel = nullptr;
    QPushButton* m_exportBtn = nullptr;
    QPushButton* m_closeBtn = nullptr;

    QThread* m_workerThread = nullptr;
    BahiKhataExportWorker* m_worker = nullptr;
    bool m_isExporting = false;
};

} // namespace MahadevERP

Q_DECLARE_METATYPE(MahadevERP::BahiKhataExporter::ExportSummary)
