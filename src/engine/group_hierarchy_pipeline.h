#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QVariantList>
#include <QVariantMap>
#include <QVector>
#include <QHash>
#include <QMutex>

/**
 * @brief Dynamic Group Hierarchy Pipeline
 * 
 * Handles Chart of Accounts Groups, recursive tree structure, parent-child relations,
 * group nature ('Asset', 'Liability', 'Income', 'Expense', 'Capital'), and hierarchy traversal.
 * Zero hardcoding — all loaded dynamically from database.
 */
struct AccountGroupNode {
    int id = 0;
    int legacyCode = 0;
    int code1 = 0;
    int code2 = 0;
    int code3 = 0;
    int code4 = 0;
    QString name;
    QString parentName;
    int parentId = 0;
    QString nature;
    int extractInBalanceSheet = 1; // 0=Rollup group total, 1=Itemized individual ledgers, 2=Schedule
    bool isSystem = false;
    int depth = 0;
    QVector<int> childGroupIds;
};

class GroupHierarchyPipeline : public QObject {
    Q_OBJECT

public:
    static GroupHierarchyPipeline& instance();

    void invalidateCache();
    QVector<AccountGroupNode> getHierarchyTree();
    QHash<int, AccountGroupNode> getAllGroupsById();
    QHash<QString, AccountGroupNode> getAllGroupsByName();
    AccountGroupNode getGroupById(int groupId);
    AccountGroupNode getGroupByName(const QString& name);
    AccountGroupNode getGroupByCode(int code1st);

    QVector<int> getSubtreeGroupIds(int rootGroupId);
    QVector<int> getSubtreeGroupIdsByName(const QString& rootGroupName);
    QString getEffectiveNature(int groupId);
    QString getEffectiveNatureByName(const QString& groupName);

    QStringList getAllGroupNames(const QString& filterNature = "");
    QStringList getParentGroupNames();
    QVariantList getGroupsSummaryList();

    bool saveGroup(int id, const QString& name, const QString& parentName, const QString& nature,
                   int extractInBs = 1, bool isSystem = false, QString* outError = nullptr);

signals:
    void groupsChanged();

private:
    GroupHierarchyPipeline(QObject* parent = nullptr);
    ~GroupHierarchyPipeline() override = default;
    GroupHierarchyPipeline(const GroupHierarchyPipeline&) = delete;
    GroupHierarchyPipeline& operator=(const GroupHierarchyPipeline&) = delete;

    void ensureLoaded();

    QMutex m_mutex;
    bool m_valid = false;
    QHash<int, AccountGroupNode> m_byId;
    QHash<QString, AccountGroupNode> m_byName;
    QHash<int, AccountGroupNode> m_byCode;
    QVector<AccountGroupNode> m_roots;
};
