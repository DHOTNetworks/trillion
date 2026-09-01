#pragma once

#include "base_table_model.h"

class PartiesModel : public BaseTableModel {
    Q_OBJECT

public:
    explicit PartiesModel(QObject* parent = nullptr);
    Q_INVOKABLE void reload_data() override;

    Q_INVOKABLE QVariantMap get_party_by_name(const QString& name) const;
    Q_INVOKABLE bool add_party(const QString& name, const QString& group, const QString& station, const QString& mobile, const QString& gstin, double op_bal, const QString& bal_type, const QString& address = "");
    Q_INVOKABLE QVariantMap get_ledger_statement(const QString& partyName, const QString& fromDate = "", const QString& toDate = "", const QString& fy = "");
};
