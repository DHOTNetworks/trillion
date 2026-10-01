#pragma once

#include <QDialog>
#include <QLabel>
#include <QProgressBar>
#include <QPushButton>
#include <QRadioButton>
#include <QButtonGroup>
#include <QStackedWidget>
#include <QGridLayout>
#include "../engine/bahi_khata_migrator.h"
#include "../engine/busy_data_migrator.h"
#include "../engine/tally_data_migrator.h"
#include "../models/firm_manager.h"

enum class MigrationSourceType {
    AutoDetect,
    BahiKhata,
    Busy,
    Tally
};

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
    void onFormatRadioToggled(int id);
    void onStartMigrationClicked();
    void onMigrationProgress(int percent, const QString& currentStep);
    void onMigrationFinished(bool success, const QString& summaryMessage);

private:
    void setupUi();
    void inspectFile();
    QWidget* createStatCard(const QString& title, QLabel** outValLabel, QLabel** outTitleLabel, const QString& bgColor, const QString& borderColor, const QString& numColor, const QString& labelColor);

    BahiKhataMigrator* m_migrator = nullptr;
    MahadevERP::BusyDataMigrator* m_busyMigrator = nullptr;
    MahadevERP::TallyDataMigrator* m_tallyMigrator = nullptr;
    FirmManager* m_firmMgr = nullptr;
    QString m_filePath;
    QString m_firmName;
    QString m_firmId;

    bool m_hasCompleted = false;
    MigrationSourceType m_selectedSourceType = MigrationSourceType::AutoDetect;
    MigrationSourceType m_detectedSourceType = MigrationSourceType::BahiKhata;
    QVariantMap m_inspectionData;

    // Header & file selection
    QPushButton* m_headerCloseBtn = nullptr;
    QLabel* m_fileLabel = nullptr;
    QPushButton* m_browseBtn = nullptr;

    // Source Format Radio Group
    QButtonGroup* m_formatButtonGroup = nullptr;
    QRadioButton* m_radioAuto = nullptr;
    QRadioButton* m_radioBahiKhata = nullptr;
    QRadioButton* m_radioBusy = nullptr;
    QRadioButton* m_radioTally = nullptr;

    // Preview Stack (Empty/Error, Stats Grid, Success)
    QStackedWidget* m_previewStack = nullptr;
    QLabel* m_detectedInfoTitle = nullptr;
    QLabel* m_emptyPlaceholderLabel = nullptr;
    QLabel* m_stockTxLabel = nullptr;
    QLabel* m_stockTxTitleLabel = nullptr;
    QLabel* m_millingLabel = nullptr;
    QLabel* m_millingTitleLabel = nullptr;
    QLabel* m_stockItemsLabel = nullptr;
    QLabel* m_stockItemsTitleLabel = nullptr;
    QLabel* m_ledgersLabel = nullptr;
    QLabel* m_ledgersTitleLabel = nullptr;
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
