#pragma once

#include "base_table_model.h"

class MillingModel : public BaseTableModel {
    Q_OBJECT

public:
    explicit MillingModel(QObject* parent = nullptr);
    Q_INVOKABLE void reload_data() override;

    Q_INVOKABLE QString get_next_batch_no(const QString& fy = "");
    Q_INVOKABLE bool add_milling_voucher_full(
        const QString& batch_no, const QString& batch_date, const QString& paddy_item,
        int paddy_bags, double paddy_weight, const QString& rice_item, int rice_bags,
        double rice_weight, double outturn_pct, double rice_cost = 0.0,
        const QString& bran_item = "", int bran_bags = 0, double bran_weight = 0.0, double bran_cost = 0.0,
        const QString& husk_item = "", int husk_bags = 0, double husk_weight = 0.0, double husk_cost = 0.0,
        const QString& nakku_item = "", int nakku_bags = 0, double nakku_weight = 0.0, double nakku_cost = 0.0,
        const QString& other_byproduct_item = "", int other_byproduct_bags = 0, double other_byproduct_weight = 0.0, double other_byproduct_cost = 0.0,
        int total_byproduct_bags = 0, double total_byproduct_weight = 0.0, double total_byproduct_cost = 0.0,
        double loss_weight = 0.0, double loss_pct = 0.0, double milling_cost_per_qtl = 0.0, double total_milling_cost = 0.0,
        const QString& operator_name = "", const QString& notes = ""
    );

    Q_INVOKABLE QVariantList get_milling_statement(const QString& fy = "");
};
