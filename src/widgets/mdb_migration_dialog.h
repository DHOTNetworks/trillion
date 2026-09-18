#pragma once

#include <QDialog>
#include <QLabel>
#include <QProgressBar>
#include <QPushButton>
#include <QStackedWidget>
#include <QGridLayout>
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
    void onBrowseClicked();
    void onStartMigrationClicked();
    void onMigrationProgress(int percent, const QString& currentStep);
    void onMigrationFinished(bool success, const QString& summaryMessage);

private:
    void setupUi();
    void inspectFile();
    QWidget* createStatCard(const QString& title, QLabel** outValLabel, const QString& bgColor, const QString& borderColor, const QString& numColor, const QString& labelColor);

    BahiKhataMigrator* m_migrator = nullptr;
    FirmManager* m_firmMgr = nullptr;
    QString m_filePath;
    QString m_firmName;
    QString m_firmId;

    bool m_hasCompleted = false;
    QVariantMap m_inspectionData;

    // Header & file selection
    QPushButton* m_headerCloseBtn = nullptr;
    QLabel* m_fileLabel = nullptr;
    QPushButton* m_browseBtn = nullptr;

    // Preview Stack (Empty/Error, Stats Grid, Success)
    QStackedWidget* m_previewStack = nullptr;
    QLabel* m_emptyPlaceholderLabel = nullptr;
    QLabel* m_stockTxLabel = nullptr;
    QLabel* m_millingLabel = nullptr;
    QLabel* m_stockItemsLabel = nullptr;
    QLabel* m_ledgersLabel = nullptr;
    QLabel* m_successMessageLabel = nullptr;

    // Progress Section
    QWidget* m_progressContainer = nullptr;
    QLabel* m_statusLabel = nullptr;
    QLabel* m_progressPercentLabel = nullptr;
    QProgressBar* m_progressBar = nullptr;

    // Footer Buttons
    QPushButton* m_startBtn = nullptr;
    QPushButton* m_closeBtn = nullptr;
};

