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

QStringList AccountGroupsModel::get_parent_groups() const {
    QStringList result;
    result << "Primary";
    QVariantList rows = DatabaseManager::instance().executeQuery(
        "SELECT DISTINCT parent_group_name FROM account_groups WHERE parent_group_name IS NOT NULL AND parent_group_name != '' "
        "UNION "
        "SELECT DISTINCT name FROM account_groups WHERE name IS NOT NULL AND name != '' "
        "ORDER BY 1 COLLATE NOCASE ASC;");
    for (const QVariant& r : rows) {
        QString n = r.toMap().value("parent_group_name").toString().trimmed();
        if (n.isEmpty()) n = r.toMap().value("name").toString().trimmed();
        if (!n.isEmpty() && !result.contains(n)) result.append(n);
    }
    return result;
}

QStringList AccountGroupsModel::get_all_group_names() const {
    QStringList result;
    for (const QVariant& r : m_data) {
        QString n = r.toMap().value("name").toString().trimmed();
        if (!n.isEmpty() && !result.contains(n)) result.append(n);
    }
    return result;
}

QVariantMap AccountGroupsModel::get_group_by_name(const QString& name) const {
    QString cleanName = name.trimmed();
    if (cleanName.isEmpty()) return {};
    QVariantList rows = DatabaseManager::instance().executeQuery(
        "SELECT * FROM account_groups WHERE name = ? COLLATE NOCASE LIMIT 1;",
        {cleanName}
    );
    if (!rows.isEmpty()) {
        return rows.first().toMap();
    }
    return {};
}

bool AccountGroupsModel::add_group(const QString& name, const QString& parentGroup, const QString& nature, const QString& desc, bool balanceSheet) {
    bool ok = DatabaseManager::instance().executeNonQuery(
        "INSERT INTO account_groups (name, parent_group_name, nature, description, extract_in_balance_sheet, is_system) "
        "VALUES (?, ?, ?, ?, ?, 0);",
        {name.trimmed(), parentGroup.trimmed(), nature.trimmed(), desc.trimmed(), balanceSheet ? 1 : 0}
    );
    if (ok) reload_data();
    return ok;
}

bool AccountGroupsModel::update_group(int groupId, const QString& name, const QString& parentGroup, const QString& nature, const QString& desc, bool balanceSheet) {
    bool ok = DatabaseManager::instance().executeNonQuery(
        "UPDATE account_groups SET name = ?, parent_group_name = ?, nature = ?, description = ?, extract_in_balance_sheet = ? "
        "WHERE id = ?;",
        {name.trimmed(), parentGroup.trimmed(), nature.trimmed(), desc.trimmed(), balanceSheet ? 1 : 0, groupId}
    );
    if (ok) reload_data();
    return ok;
}
