#pragma once

#include "base_table_model.h"

class ManufacturingModel : public BaseTableModel {
    Q_OBJECT

public:
    explicit ManufacturingModel(QObject* parent = nullptr);
    Q_INVOKABLE void reload_data() override;

    Q_INVOKABLE QString get_next_batch_no(const QString& fy = "");
    Q_INVOKABLE bool add_manufacturing_voucher(
        const QString& batch_no, const QString& batch_date, const QString& particulars,
        const QVariantList& consumed_items, const QVariantList& produced_items,
        int edit_id = 0
    );

    Q_INVOKABLE bool add_manufacturing_voucher_full(
        const QString& batch_no, const QString& batch_date,
        const QString& input_item, int input_bags, double input_weight,
        const QVariantList& produced_items,
        double wastage_weight = 0.0, double wastage_pct = 0.0,
        double manufacturing_cost_per_qtl = 0.0, double total_manufacturing_cost = 0.0,
        const QString& operator_name = "", const QString& notes = ""
    );

    Q_INVOKABLE QVariantList get_manufacturing_statement(const QString& from_date = "", const QString& to_date = "", const QString& variety = "");
    Q_INVOKABLE QVariantMap get_manufacturing_totals(const QString& from_date = "", const QString& to_date = "");
    Q_INVOKABLE QVariantList get_batch_items(int batch_id = 0, const QString& batch_no = "");
    Q_INVOKABLE QVariantMap get_manufacturing_batch(const QVariant& batchNoOrId);

private:
    void auto_repair_manufacturing_batches();
};
