#include "account_groups_model.h"
#include "../engine/group_hierarchy_pipeline.h"
#include "../database_manager.h"
#include "account_classifier.h"

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

void AccountGroupsModel::ensureDomainGroups(const QString& domain) {
    const QString d = domain.trimmed().toLower();
    struct DomainGroup {
        const char* name; const char* parent; const char* nature;
        int c1, c2, c3, c4; int extractBs;
    };
    static const DomainGroup mandi[] = {
        {"Zimidara Debtors", "Sundry Debtors", "Assets", 32, 20, 6, 0, 1},
        {"Zimidara Creditors", "Sundry Creditors", "Liabilities", 33, 11, 9, 0, 1},
        {"Mandi Debtors", "Sundry Debtors", "Assets", 34, 20, 6, 0, 1},
        {"Local Mandi Creditors", "Sundry Creditors", "Liabilities", 31, 11, 9, 0, 1},
    };
    static const DomainGroup manufacturer[] = {
        {"Manufacturing Group", "Primary", "Expense", 22, 0, 0, 0, 0},
        {"Manufacturing Exp.", "Manufacturing Group", "Expense", 23, 22, 0, 0, 0},
    };
    const DomainGroup* list = nullptr;
    int count = 0;
    QString tag;
    if (d == "mandi") { list = mandi; count = 4; tag = "Domain: mandi"; }
    else if (d == "manufacturer") { list = manufacturer; count = 2; tag = "Domain: manufacturer"; }
    else return;
    auto& db = DatabaseManager::instance();
    for (int i = 0; i < count; ++i) {
        const DomainGroup& g = list[i];
        QVariantList rows = db.executeQuery(
            "SELECT id FROM account_groups WHERE name = ? LIMIT 1;", {g.name});
        if (!rows.isEmpty()) continue;
        db.executeNonQuery(
            "INSERT INTO account_groups (name, parent_group_name, nature, description, "
            "extract_in_balance_sheet, is_system, code1st, code2nd, code3rd, code4th) "
            "VALUES (?, ?, ?, ?, ?, 0, ?, ?, ?, ?);",
            {g.name, g.parent, g.nature, tag, g.extractBs, g.c1, g.c2, g.c3, g.c4});
    }
    AccountClassifier::invalidateCache();
    // get_group_by_name reads through GroupHierarchyPipeline's own cache
    // (m_byName/m_valid), not the classifier's — invalidate both, or fresh
    // top-up rows are invisible until restart (broke testDomainGroupsTopUp).
    GroupHierarchyPipeline::instance().invalidateCache();
}

