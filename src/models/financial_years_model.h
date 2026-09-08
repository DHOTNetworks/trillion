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

    Q_INVOKABLE QString get_working_date() const;
    Q_INVOKABLE void set_working_date(const QString& dateStr);

    Q_INVOKABLE QString parse_date_pattern(const QString& input, const QString& referenceDate = "") const;
    Q_INVOKABLE QVariantMap validate_voucher_date(const QString& input, const QString& referenceDate = "") const;

private:
    mutable QString m_workingDate;
};
