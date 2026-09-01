#pragma once

#include "base_table_model.h"

class StockItemsModel : public BaseTableModel {
    Q_OBJECT

public:
    explicit StockItemsModel(QObject* parent = nullptr);
    Q_INVOKABLE void reload_data() override;

    Q_INVOKABLE QVariantMap get_item_by_name(const QString& name) const;
    Q_INVOKABLE QVariantMap get_item_movement(const QString& itemName, const QString& fy = "");
    Q_INVOKABLE bool add_item(const QString& name, const QString& category, double opening_bags, double opening_weight, double sale_rate, double purchase_rate, const QString& gst_rate, const QString& hsn_code = "");
};
