#pragma once

#include <QWidget>
#include <QLineEdit>
#include <QComboBox>
#include <QCheckBox>
#include <QPushButton>
#include <QLabel>
#include "../models/account_groups_model.h"

class KbdBadgeButton;

namespace MahadevERP {

class NewGroupWidget : public QWidget {
    Q_OBJECT

public:
    explicit NewGroupWidget(AccountGroupsModel* groupsModel = nullptr, QWidget* parent = nullptr);

    void resetForm();
    void focusFirstField();

signals:
    void backRequested();
    void savedSuccess();

private slots:
    void onSaveClicked();
    void refreshParents();

private:
    void setupUi();

    AccountGroupsModel* m_groupsModel = nullptr;

    QLineEdit* m_groupNameEdit = nullptr;
    QComboBox* m_parentCombo = nullptr;
    QComboBox* m_natureCombo = nullptr;
    QLineEdit* m_descEdit = nullptr;
    QCheckBox* m_bsCheckBox = nullptr;

    KbdBadgeButton* m_saveBtn = nullptr;
    KbdBadgeButton* m_cancelBtn = nullptr;
};

} // namespace MahadevERP
