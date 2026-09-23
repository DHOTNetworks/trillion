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

class ModifyGroupWidget : public QWidget {
    Q_OBJECT

public:
    explicit ModifyGroupWidget(AccountGroupsModel* groupsModel = nullptr, QWidget* parent = nullptr);

    void resetForm();
    void focusSearch();

signals:
    void backRequested();
    void savedSuccess();

private slots:
    void onGroupSelected(const QString& groupName);
    void onUpdateClicked();
    void refreshGroupsList();

private:
    void setupUi();

    AccountGroupsModel* m_groupsModel = nullptr;
    int m_currentGroupId = -1;

    QComboBox* m_selectGroupCombo = nullptr;
    QLineEdit* m_groupNameEdit = nullptr;
    QComboBox* m_parentCombo = nullptr;
    QComboBox* m_natureCombo = nullptr;
    QLineEdit* m_descEdit = nullptr;
    QCheckBox* m_bsCheckBox = nullptr;

    KbdBadgeButton* m_updateBtn = nullptr;
    KbdBadgeButton* m_cancelBtn = nullptr;
};

} // namespace MahadevERP
