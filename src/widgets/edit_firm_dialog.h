#pragma once

// EditFirmDialog: edit an existing firm's info from the firm selector
// page (user-ordered). Reads/writes the firm's own DB file through a
// switch dance (remember current → switch → work → switch back +
// DbService restart); the live DB is never left pointing elsewhere.
// Package change is additive-only (base flags OR-merged, never cleared).
// Add-on boxes mirror the creation matrix; boxes for already-ON flags
// come locked-checked so the UI never promises a removal it cannot do.
// Company rename syncs the firms registry via FirmManager::rename_firm.

#include <QDialog>
#include <QLineEdit>
#include <QComboBox>
#include <QLabel>
#include <QPushButton>
#include <QRadioButton>
#include <QCheckBox>
#include <QButtonGroup>
#include "../config/feature_flags.h" // FeatureFlags member (additive-only package edits)
#include "kbd_badge_button.h"
#include "erp_theme.h"
#include "../models/firm_manager.h"

class EditFirmDialog : public QDialog {
    Q_OBJECT

public:
    explicit EditFirmDialog(FirmManager* firmMgr, const QString& firmId,
                            const QString& dbPath, QWidget* parent = nullptr);

signals:
    void firmUpdated(const QString& firmId, const QString& firmName);

protected:
    void keyPressEvent(QKeyEvent* event) override;

private slots:
    void onSaveClicked();

private:
    void setupUi();
    bool loadFirm();
    QString selectedPackageKey() const;
    QString selectedPackageLabel() const;

    FirmManager* m_firmMgr = nullptr;
    QString m_firmId;
    QString m_dbPath;

    QLineEdit* m_compNameEdit = nullptr;
    QLineEdit* m_gstinEdit = nullptr;
    QLineEdit* m_panEdit = nullptr;
    QLineEdit* m_mlNoEdit = nullptr;
    QLineEdit* m_fssaiEdit = nullptr;
    QLineEdit* m_addressEdit = nullptr;
    QLineEdit* m_cityEdit = nullptr;
    QLineEdit* m_stateEdit = nullptr;
    QLineEdit* m_pinEdit = nullptr;
    QLineEdit* m_phoneEdit = nullptr;
    QLineEdit* m_mobileEdit = nullptr;
    QLineEdit* m_bankNameEdit = nullptr;
    QLineEdit* m_bankAccEdit = nullptr;
    QLineEdit* m_ifscEdit = nullptr;

    QButtonGroup* m_packageGroup = nullptr;
    QRadioButton* m_pkgAgent = nullptr;
    QRadioButton* m_pkgManufacturer = nullptr;
    QRadioButton* m_pkgTrader = nullptr;
    QRadioButton* m_pkgTransporter = nullptr;
    QRadioButton* m_pkgGeneral = nullptr;
    // Add-ons mirror the creation matrix; boxes for already-ON flags come
    // locked-checked (additive-only: nothing here can clear a flag).
    QCheckBox* m_addMandi = nullptr;
    QCheckBox* m_addManufacturer = nullptr;
    QCheckBox* m_addBardana = nullptr;
    QCheckBox* m_addTransport = nullptr;
    FeatureFlags m_loadedFlags;
    void updatePackageAddOns();

    QLabel* m_errorLabel = nullptr;
    KbdBadgeButton* m_saveBtn = nullptr;
    KbdBadgeButton* m_cancelBtn = nullptr;
};
