#pragma once

#include "base_table_model.h"
#include <QStringList>
#include <QVariantMap>

class AccountGroupsModel : public BaseTableModel {
    Q_OBJECT

public:
    explicit AccountGroupsModel(QObject* parent = nullptr);
    Q_INVOKABLE void reload_data() override;

    Q_INVOKABLE QVariantList get_groups_list() const;
    Q_INVOKABLE QStringList get_parent_groups() const;
    Q_INVOKABLE QStringList get_all_group_names() const;
    Q_INVOKABLE QVariantMap get_group_by_name(const QString& name) const;
    Q_INVOKABLE bool add_group(const QString& name, const QString& parentGroup, const QString& nature, const QString& desc = "", bool balanceSheet = false);
    Q_INVOKABLE bool update_group(int groupId, const QString& name, const QString& parentGroup, const QString& nature, const QString& desc = "", bool balanceSheet = false);
};
