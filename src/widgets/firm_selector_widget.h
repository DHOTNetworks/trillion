#pragma once

#include <QWidget>
#include <QTableWidget>
#include <QLineEdit>
#include <QPushButton>
#include "kbd_badge_button.h"
#include "erp_theme.h"
#include <QLabel>
#include <QStackedWidget>
#include <QVariantList>
#include "../models/firm_manager.h"
#include "../engine/bahi_khata_migrator.h"

class FirmSelectorWidget : public QWidget {
    Q_OBJECT

public:
    explicit FirmSelectorWidget(FirmManager* firmMgr,
                                BahiKhataMigrator* migrator = nullptr,
                                QWidget* parent = nullptr);

    void refreshFirms();
    void resetToAppData();
    void pickFolder();
    void focusTable();
    QTableWidget* table() const { return m_table; }

signals:
    void firmOpened(const QString& firmId, const QString& firmName);
    void backRequested();

public slots:
    void openSelectedFirm();
    void openNewFirmDialog();
    void openEditFirmDialog();

protected:
    void keyPressEvent(QKeyEvent* event) override;
    void showEvent(QShowEvent* event) override;

private slots:
    void onFolderAccepted();
    void onTableItemDoubleClicked(QTableWidgetItem* item);
    void onTableRowSelectionChanged();

private:
    void setupUi();
    void updateHeaderAndFolderStyles();
    bool isViewingAppData() const;
    void startImportForFirm(const QVariantMap& firmMap);

    FirmManager* m_firmMgr = nullptr;
    BahiKhataMigrator* m_migrator = nullptr;

    QString m_appDataFolder;
    QString m_currentFolder;
    QVariantList m_firmsList;

    // Header
    QLabel* m_subtitleLabel = nullptr;
    KbdBadgeButton* m_backBtn = nullptr;

    // Folder controls
    QFrame* m_folderBarFrame = nullptr;
    QLabel* m_folderDirLabel = nullptr;
    QLineEdit* m_folderInput = nullptr;
    KbdBadgeButton* m_appDataBtn = nullptr;
    KbdBadgeButton* m_importMdbBtn = nullptr;
    KbdBadgeButton* m_browseBtn = nullptr;
    KbdBadgeButton* m_rescanBtn = nullptr;

    // Central stack (Table vs Empty State)
    QStackedWidget* m_centerStack = nullptr;
    QTableWidget* m_table = nullptr;
    QWidget* m_emptyStateWidget = nullptr;
    QLabel* m_emptyStateLabel = nullptr;
    KbdBadgeButton* m_emptyStateBtn = nullptr;

    // Footer
    KbdBadgeButton* m_newFirmBtn = nullptr;
    KbdBadgeButton* m_editFirmBtn = nullptr;
    KbdBadgeButton* m_openFirmBtn = nullptr;
};
