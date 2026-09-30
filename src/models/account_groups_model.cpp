#include "account_groups_model.h"
#include "../engine/group_hierarchy_pipeline.h"
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
    m_data = GroupHierarchyPipeline::instance().getGroupsSummaryList();
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
    return GroupHierarchyPipeline::instance().getParentGroupNames();
}

QStringList AccountGroupsModel::get_all_group_names() const {
    return GroupHierarchyPipeline::instance().getAllGroupNames();
}

QVariantMap AccountGroupsModel::get_group_by_name(const QString& name) const {
    AccountGroupNode node = GroupHierarchyPipeline::instance().getGroupByName(name);
    if (node.id <= 0) return {};
    QVariantMap map;
    map["id"] = node.id;
    map["name"] = node.name;
    map["parent_group_name"] = node.parentName;
    map["nature"] = node.nature;
    map["extract_in_balance_sheet"] = node.extractInBalanceSheet;
    map["is_system"] = node.isSystem;
    map["depth"] = node.depth;
    return map;
}

bool AccountGroupsModel::add_group(const QString& name, const QString& parentGroup, const QString& nature, const QString& desc, bool balanceSheet) {
    Q_UNUSED(desc);
    QString err;
    bool ok = GroupHierarchyPipeline::instance().saveGroup(0, name, parentGroup, nature, balanceSheet ? 1 : 0, false, &err);
    if (ok) {
        reload_data();
    }
    return ok;
}

bool AccountGroupsModel::update_group(int groupId, const QString& name, const QString& parentGroup, const QString& nature, const QString& desc, bool balanceSheet) {
    Q_UNUSED(desc);
    QString err;
    bool ok = GroupHierarchyPipeline::instance().saveGroup(groupId, name, parentGroup, nature, balanceSheet ? 1 : 0, false, &err);
    if (ok) {
        reload_data();
    }
    return ok;
}

