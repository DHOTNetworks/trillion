#include "account_groups_model.h"
#include "account_classifier.h"
#include "../database_manager.h"

AccountGroupsModel::AccountGroupsModel(QObject* parent)
    : BaseTableModel(
        {"Group Name", "Parent Group", "Nature", "Description", "Balance Sheet", "Type"},
        {"name", "parent_group_name", "nature", "description", "extract_in_balance_sheet", "is_system"},
        parent
    )
{
}

void AccountGroupsModel::reload_data() {
    beginResetModel();
    m_data = AccountClassifier::searchGroups("", 1000);
    endResetModel();
    emit dataChangedSignal();
    emit countChanged();
}

QVariantList AccountGroupsModel::get_groups_list() const {
    if (m_data.isEmpty()) {
        const_cast<AccountGroupsModel*>(this)->reload_data();
    }
    return m_data;
}

QStringList AccountGroupsModel::get_parent_groups() const {
    return AccountClassifier::getParentGroupNames();
}

QStringList AccountGroupsModel::get_all_group_names() const {
    return AccountClassifier::getAllGroupNames();
}

QVariantMap AccountGroupsModel::get_group_by_name(const QString& name) const {
    return AccountClassifier::getGroupDetails(name);
}

bool AccountGroupsModel::add_group(const QString& name, const QString& parentGroup, const QString& nature, const QString& desc, bool balanceSheet) {
    QString err;
    bool ok = AccountClassifier::createOrUpdateGroup(0, name, parentGroup, nature, desc, balanceSheet, &err);
    if (ok) {
        reload_data();
    }
    return ok;
}

bool AccountGroupsModel::update_group(int groupId, const QString& name, const QString& parentGroup, const QString& nature, const QString& desc, bool balanceSheet) {
    QString err;
    bool ok = AccountClassifier::createOrUpdateGroup(groupId, name, parentGroup, nature, desc, balanceSheet, &err);
    if (ok) {
        reload_data();
    }
    return ok;
}
