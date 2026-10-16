#pragma once

#include <QDialog>
#include <QLineEdit>
#include <QComboBox>
#include <QLabel>
#include <QPushButton>
#include <QRadioButton>
#include <QCheckBox>
#include <QButtonGroup>
#include "kbd_badge_button.h"
#include "erp_theme.h"
#include "../models/firm_manager.h"

struct FeatureFlags; // config/feature_flags.h (included in the .cpp)

class NewFirmDialog : public QDialog {
    Q_OBJECT

public:
    explicit NewFirmDialog(FirmManager* firmMgr, QWidget* parent = nullptr);

signals:
    void firmCreated(const QString& firmId, const QString& firmName);

protected:
    void keyPressEvent(QKeyEvent* event) override;

private slots:
    void onGstinChanged(const QString& text);
    void onPanChanged(const QString& text);
    void onCreateClicked();

private:
    void setupUi();
    void autoDetectFromPan(const QString& panStr);
    void applyDerivedPan(const QString& gstinUpper);
    bool m_panAutoFilled = false;
    QString m_panAutoGstin;

    FirmManager* m_firmMgr = nullptr;

    QLineEdit* m_compNameEdit = nullptr;
    QComboBox* m_firmTypeCombo = nullptr;
    QLineEdit* m_booksFromEdit = nullptr;

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

    // Section 5: business package (S6 wizard). Radios pick the base
    // package; checkboxes add valid add-ons. Never restructured — the
    // section appends after banking and only feeds onCreateClicked.
    QButtonGroup* m_packageGroup = nullptr;
    QRadioButton* m_pkgAgent = nullptr;
    QRadioButton* m_pkgManufacturer = nullptr;
    QRadioButton* m_pkgTrader = nullptr;
    QRadioButton* m_pkgTransporter = nullptr;
    QRadioButton* m_pkgGeneral = nullptr;
    QCheckBox* m_addMandi = nullptr;
    QCheckBox* m_addManufacturer = nullptr;
    QCheckBox* m_addBardana = nullptr;
    QCheckBox* m_addTransport = nullptr;
    void updatePackageAddOns();
    QString selectedPackageKey() const;
    QString selectedPackageLabel() const;
    FeatureFlags selectedPackageFlags() const;

    QLabel* m_errorLabel = nullptr;
    KbdBadgeButton* m_createBtn = nullptr;
    KbdBadgeButton* m_cancelBtn = nullptr;
};
