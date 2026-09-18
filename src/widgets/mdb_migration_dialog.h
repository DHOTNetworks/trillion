#pragma once

#include <QDialog>
#include <QLabel>
#include <QProgressBar>
#include <QPushButton>
#include "../engine/bahi_khata_migrator.h"
#include "../models/firm_manager.h"

class MdbMigrationDialog : public QDialog {
    Q_OBJECT

public:
    explicit MdbMigrationDialog(BahiKhataMigrator* migrator,
                                FirmManager* firmMgr,
                                const QString& filePath,
                                const QString& firmName = "",
                                const QString& firmId = "",
                                QWidget* parent = nullptr);

signals:
    void migrationCompleted(const QString& firmId, const QString& firmName);

protected:
    void keyPressEvent(QKeyEvent* event) override;

private slots:
    void onStartMigrationClicked();
    void onMigrationProgress(int percent, const QString& currentStep);
    void onMigrationFinished(bool success, const QString& summaryMessage);

private:
    void setupUi();
    void inspectFile();

    BahiKhataMigrator* m_migrator = nullptr;
    FirmManager* m_firmMgr = nullptr;
    QString m_filePath;
    QString m_firmName;
    QString m_firmId;

    QLabel* m_fileLabel = nullptr;
    QLabel* m_tablesInfoLabel = nullptr;
    QLabel* m_statusLabel = nullptr;
    QProgressBar* m_progressBar = nullptr;
    QPushButton* m_startBtn = nullptr;
    QPushButton* m_closeBtn = nullptr;
};
