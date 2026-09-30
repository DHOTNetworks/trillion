#include "group_hierarchy_pipeline.h"
#include "../database_manager.h"
#include <QMutexLocker>
#include <QQueue>
#include <QDebug>

GroupHierarchyPipeline::GroupHierarchyPipeline(QObject* parent)
    : QObject(parent)
{
}

GroupHierarchyPipeline& GroupHierarchyPipeline::instance() {
    static GroupHierarchyPipeline s_instance;
    return s_instance;
}

void GroupHierarchyPipeline::invalidateCache() {
    QMutexLocker locker(&m_mutex);
    m_byId.clear();
    m_byName.clear();
    m_byCode.clear();
    m_roots.clear();
    m_valid = false;
    emit groupsChanged();
}

void GroupHierarchyPipeline::ensureLoaded() {
    if (m_valid) return;

    m_byId.clear();
    m_byName.clear();
    m_byCode.clear();
    m_roots.clear();

    QVariantList rows = DatabaseManager::instance().executeQuery(
        "SELECT id, name, parent_group_name, nature, extract_in_balance_sheet, is_system, "
        "       COALESCE(code1st, 0) AS c1, COALESCE(code2nd, 0) AS c2, "
        "       COALESCE(code3rd, 0) AS c3, COALESCE(code4th, 0) AS c4 "
        "FROM account_groups ORDER BY id ASC;"
    );

    for (const auto& rVar : rows) {
        QVariantMap r = rVar.toMap();
        AccountGroupNode node;
        node.id = r.value("id").toInt();
        node.name = r.value("name").toString().trimmed();
        node.parentName = r.value("parent_group_name").toString().trimmed();
        node.nature = r.value("nature").toString().trimmed();
        node.extractInBalanceSheet = r.value("extract_in_balance_sheet").toInt();
        node.isSystem = (r.value("is_system").toInt() == 1);
        node.code1 = r.value("c1").toInt();
        node.code2 = r.value("c2").toInt();
        node.code3 = r.value("c3").toInt();
        node.code4 = r.value("c4").toInt();
        node.legacyCode = node.code1;

        m_byId.insert(node.id, node);
        m_byName.insert(node.name.toLower(), node);
        if (node.code1 > 0) {
            m_byCode.insert(node.code1, node);
        }
    }

    // Resolve parent IDs and children links
    for (auto it = m_byId.begin(); it != m_byId.end(); ++it) {
        QString pName = it.value().parentName.toLower();
        if (!pName.isEmpty() && m_byName.contains(pName)) {
            int pId = m_byName.value(pName).id;
            it.value().parentId = pId;
            if (m_byId.contains(pId)) {
                m_byId[pId].childGroupIds.append(it.key());
            }
        }
    }

    // Determine root nodes and calculate tree depth
    for (auto it = m_byId.begin(); it != m_byId.end(); ++it) {
        if (it.value().parentId == 0) {
            it.value().depth = 0;
            m_roots.append(it.value());
        }
    }

    m_valid = true;
}

QVector<AccountGroupNode> GroupHierarchyPipeline::getHierarchyTree() {
    QMutexLocker locker(&m_mutex);
    ensureLoaded();
    return m_roots;
}

QHash<int, AccountGroupNode> GroupHierarchyPipeline::getAllGroupsById() {
    QMutexLocker locker(&m_mutex);
    ensureLoaded();
    return m_byId;
}

QHash<QString, AccountGroupNode> GroupHierarchyPipeline::getAllGroupsByName() {
    QMutexLocker locker(&m_mutex);
    ensureLoaded();
    return m_byName;
}

AccountGroupNode GroupHierarchyPipeline::getGroupById(int groupId) {
    QMutexLocker locker(&m_mutex);
    ensureLoaded();
    return m_byId.value(groupId);
}

AccountGroupNode GroupHierarchyPipeline::getGroupByName(const QString& name) {
    QMutexLocker locker(&m_mutex);
    ensureLoaded();
    return m_byName.value(name.trimmed().toLower());
}

AccountGroupNode GroupHierarchyPipeline::getGroupByCode(int code1st) {
    QMutexLocker locker(&m_mutex);
    ensureLoaded();
    return m_byCode.value(code1st);
}

QVector<int> GroupHierarchyPipeline::getSubtreeGroupIds(int rootGroupId) {
    QMutexLocker locker(&m_mutex);
    ensureLoaded();

    QVector<int> result;
    if (!m_byId.contains(rootGroupId)) return result;

    result.append(rootGroupId);
    QQueue<int> queue;
    queue.enqueue(rootGroupId);

    while (!queue.isEmpty()) {
        int cur = queue.dequeue();
        if (m_byId.contains(cur)) {
            for (int chId : m_byId[cur].childGroupIds) {
                if (!result.contains(chId)) {
                    result.append(chId);
                    queue.enqueue(chId);
                }
            }
        }
    }
    return result;
}

QVector<int> GroupHierarchyPipeline::getSubtreeGroupIdsByName(const QString& rootGroupName) {
    AccountGroupNode node = getGroupByName(rootGroupName);
    if (node.id > 0) {
        return getSubtreeGroupIds(node.id);
    }
    return QVector<int>();
}

QString GroupHierarchyPipeline::getEffectiveNature(int groupId) {
    QMutexLocker locker(&m_mutex);
    ensureLoaded();

    int curId = groupId;
    int guard = 0;
    while (curId > 0 && guard++ < 20) {
        if (!m_byId.contains(curId)) break;
        const AccountGroupNode& n = m_byId[curId];
        if (!n.nature.trimmed().isEmpty()) {
            return n.nature.trimmed();
        }
        curId = n.parentId;
    }
    return "Asset";
}

QString GroupHierarchyPipeline::getEffectiveNatureByName(const QString& groupName) {
    AccountGroupNode node = getGroupByName(groupName);
    if (node.id > 0) {
        return getEffectiveNature(node.id);
    }
    return "Asset";
}

QStringList GroupHierarchyPipeline::getAllGroupNames(const QString& filterNature) {
    QMutexLocker locker(&m_mutex);
    ensureLoaded();

    QStringList list;
    for (const auto& n : m_byId) {
        if (filterNature.isEmpty() || n.nature.compare(filterNature, Qt::CaseInsensitive) == 0) {
            list << n.name;
        }
    }
    list.sort(Qt::CaseInsensitive);
    return list;
}

QStringList GroupHierarchyPipeline::getParentGroupNames() {
    QMutexLocker locker(&m_mutex);
    ensureLoaded();

    QStringList list;
    for (const auto& r : m_roots) {
        list << r.name;
    }
    list.sort(Qt::CaseInsensitive);
    return list;
}

QVariantList GroupHierarchyPipeline::getGroupsSummaryList() {
    QMutexLocker locker(&m_mutex);
    ensureLoaded();

    QVariantList list;
    for (const auto& n : m_byId) {
        QVariantMap m;
        m["id"] = n.id;
        m["name"] = n.name;
        m["parent_group_name"] = n.parentName;
        m["nature"] = n.nature;
        m["extract_in_balance_sheet"] = n.extractInBalanceSheet;
        m["is_system"] = n.isSystem ? 1 : 0;
        m["code1st"] = n.code1;
        list.append(m);
    }
    return list;
}

#include "../models/account_classifier.h"

bool GroupHierarchyPipeline::saveGroup(int id, const QString& name, const QString& parentName,
                                       const QString& nature, int extractInBs, bool isSystem, QString* outError) {
    Q_UNUSED(isSystem);
    bool success = AccountClassifier::createOrUpdateGroup(id, name, parentName, nature, "", extractInBs != 0, outError);
    if (success) {
        invalidateCache();
    }
    return success;
}
