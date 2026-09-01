#pragma once

#include "base_table_model.h"

class AccountGroupsModel : public BaseTableModel {
    Q_OBJECT

public:
    explicit AccountGroupsModel(QObject* parent = nullptr);
    Q_INVOKABLE void reload_data() override;

    Q_INVOKABLE QVariantList get_groups_list() const;
    Q_INVOKABLE QVariantList get_parent_groups() const;
    Q_INVOKABLE bool add_group(const QString& name, const QString& parentGroup, const QString& nature, const QString& desc = "", bool balanceSheet = false);
};
