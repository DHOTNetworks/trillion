#pragma once

#include "base_table_model.h"

class FinancialYearsModel : public BaseTableModel {
    Q_OBJECT

public:
    explicit FinancialYearsModel(QObject* parent = nullptr);
    Q_INVOKABLE void reload_data() override;

    Q_INVOKABLE QVariantMap get_active_year() const;
    Q_INVOKABLE bool set_active_year(const QString& yearName);
    Q_INVOKABLE QVariantList get_all_years() const;
};
