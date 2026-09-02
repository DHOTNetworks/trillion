#pragma once

#include "base_table_model.h"

class StockItemsModel : public BaseTableModel {
    Q_OBJECT

public:
    explicit StockItemsModel(QObject* parent = nullptr);
    Q_INVOKABLE void reload_data() override;

    Q_INVOKABLE QStringList get_items_list() const;
    Q_INVOKABLE QVariantMap get_item_by_name(const QString& name) const;
    Q_INVOKABLE QStringList get_item_types() const;
    Q_INVOKABLE QStringList get_units() const;
    Q_INVOKABLE QStringList get_gst_rates() const;

    Q_INVOKABLE bool add_stock_item(
        const QString& name, const QString& code, const QString& item_type,
        const QString& company_name, const QString& unit, double purchase_rate,
        double sale_rate, double mrp, double discount, const QString& hsn_code,
        double gst_rate, double cess_rate, double packing_kg, int opening_bags,
        double opening_qty, double opening_rate, double opening_value,
        const QString& purchase_ledger, const QString& sale_ledger, const QString& stock_ledger
    );

    Q_INVOKABLE bool update_stock_item(
        int item_id, const QString& name, const QString& code, const QString& item_type,
        const QString& company_name, const QString& unit, double purchase_rate,
        double sale_rate, double mrp, double discount, const QString& hsn_code,
        double gst_rate, double cess_rate, double packing_kg, int opening_bags,
        double opening_qty, double opening_rate, double opening_value,
        const QString& purchase_ledger, const QString& sale_ledger, const QString& stock_ledger
    );

    Q_INVOKABLE bool delete_stock_item(int item_id);

    Q_INVOKABLE QString get_financial_year() const;
    Q_INVOKABLE QString get_from_date() const;
    Q_INVOKABLE QString get_to_date() const;
    Q_INVOKABLE void set_financial_year(const QString& fy);
    Q_INVOKABLE void set_accounting_period(const QString& fromDate, const QString& toDate, const QString& fyLabel = "");
    Q_INVOKABLE QVariantList get_available_financial_years() const;

    Q_INVOKABLE QVariantList get_stock_register(const QString& param1 = "", const QString& param2 = "");
    Q_INVOKABLE QVariantList get_item_movements(const QString& itemName, const QString& param1 = "", const QString& param2 = "");
    Q_INVOKABLE QVariantMap get_item_movement(const QString& itemName, const QString& fy = "");

private:
    QString m_currentFinancialYear = "FY 2026-27";
    QString m_currentFromDate = "2026-04-01";
    QString m_currentToDate = "2027-03-31";

    void initActivePeriod();
};
