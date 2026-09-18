#pragma once

#include <QDialog>
#include <QLineEdit>
#include <QComboBox>
#include <QLabel>
#include <QPushButton>
#include "../models/firm_manager.h"

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

    FirmManager* m_firmMgr = nullptr;

    QLineEdit* m_compNameEdit = nullptr;
    QComboBox* m_firmTypeCombo = nullptr;
    QLineEdit* m_businessEdit = nullptr;
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

    QLabel* m_errorLabel = nullptr;
    QPushButton* m_createBtn = nullptr;
    QPushButton* m_cancelBtn = nullptr;
};
