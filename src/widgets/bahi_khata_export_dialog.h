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
 * @brief High-fidelity modal dialog for exporting ERP data to Bahi-Khata JetDB files (Data.001, Data.002, etc.)
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
    void onExportClicked();
    void onExportProgress(int percent, const QString& status);
    void onExportFinished(bool success, const QString& message);

private:
    void setupUi();
    QString suggestDefaultFileName() const;
    bool validateFileName(const QString& path, QString& outError) const;

    FirmManager* m_firmMgr = nullptr;
    BahiKhataExporter* m_exporter = nullptr;

    QLineEdit* m_pathEdit = nullptr;
    QPushButton* m_browseBtn = nullptr;
    QComboBox* m_fyCombo = nullptr;
    QCheckBox* m_backupCheck = nullptr;
    QCheckBox* m_txCheck = nullptr;
    QCheckBox* m_millingStockCheck = nullptr;
    QProgressBar* m_progressBar = nullptr;
    QLabel* m_statusLabel = nullptr;
    QPushButton* m_exportBtn = nullptr;
    QPushButton* m_closeBtn = nullptr;

    bool m_isExporting = false;
};

} // namespace MahadevERP
