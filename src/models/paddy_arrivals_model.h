#pragma once

#include "base_table_model.h"

class PaddyArrivalsModel : public BaseTableModel {
    Q_OBJECT

public:
    explicit PaddyArrivalsModel(QObject* parent = nullptr);
    Q_INVOKABLE void reload_data() override;

    Q_INVOKABLE bool add_arrival(const QString& farmer_name, const QString& paddy_variety, const QString& arrival_date, int bags, double gross_qtl, double moisture_pct, double rate_qtl, double hamali, const QString& status);
};
