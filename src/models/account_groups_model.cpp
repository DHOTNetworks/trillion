#include "account_groups_model.h"
#include "../database_manager.h"

AccountGroupsModel::AccountGroupsModel(QObject* parent)
    : BaseTableModel(
        {"Group Name", "Parent Group", "Nature", "Description", "Balance Sheet", "Type"},
        {"name", "parent_group_name", "nature", "description", "extract_in_balance_sheet", "is_system"},
        parent
    )
{
    reload_data();
}

void AccountGroupsModel::reload_data() {
    beginResetModel();
    m_data = DatabaseManager::instance().executeQuery("SELECT * FROM account_groups ORDER BY name ASC;");
    endResetModel();
    emit dataChangedSignal();
    emit countChanged();
}

QVariantList AccountGroupsModel::get_groups_list() const {
    return m_data;
}

QVariantList AccountGroupsModel::get_parent_groups() const {
    return DatabaseManager::instance().executeQuery("SELECT DISTINCT parent_group_name FROM account_groups WHERE parent_group_name IS NOT NULL AND parent_group_name != '';");
}

bool AccountGroupsModel::add_group(const QString& name, const QString& parentGroup, const QString& nature, const QString& desc, bool balanceSheet) {
    bool ok = DatabaseManager::instance().executeNonQuery(
        "INSERT INTO account_groups (name, parent_group_name, nature, description, extract_in_balance_sheet, is_system) "
        "VALUES (?, ?, ?, ?, ?, 0);",
        {name, parentGroup, nature, desc, balanceSheet ? 1 : 0}
    );
    if (ok) reload_data();
    return ok;
}
