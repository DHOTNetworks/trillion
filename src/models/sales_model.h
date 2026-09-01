#pragma once

#include "base_table_model.h"

class SalesModel : public BaseTableModel {
    Q_OBJECT

public:
    explicit SalesModel(QObject* parent = nullptr);
    Q_INVOKABLE void reload_data() override;

    Q_INVOKABLE QString get_next_voucher_no(const QString& fy = "");
    Q_INVOKABLE QString get_next_invoice_no(const QString& fy = "");
    Q_INVOKABLE QString increment_invoice(const QString& invStr);

    Q_INVOKABLE bool add_sales_invoice_full(
        const QString& invoice_no, const QString& invoice_date, const QString& party_ledger,
        const QString& gstin, const QString& item_name, const QString& hsn_code,
        int bag_count, double weight_qtl, double rate_per_qtl, double taxable_amount,
        double gst_pct, double cgst_amount, double sgst_amount, double igst_amount,
        double round_off, double total_amount, const QString& payment_mode,
        const QString& vehicle_no, const QString& eway_bill_no, const QString& narration,
        const QString& sale_status = "Self Sale", const QString& market_fee_status = "Paid",
        double dami = 0.0, double labour = 0.0, double auction = 0.0, double m_fee = 0.0,
        double hrdf = 0.0, double other_exp = 0.0, double welfare = 0.0, double dhrmd = 0.0,
        double sutli = 0.0, double less_amount = 0.0, const QString& gr_no = "",
        const QString& driver = "", const QString& bill_time = "", const QString& sauda_date = "",
        const QString& shipping_address = "", const QString& po_no = "", const QString& grade = "",
        const QString& kanda_weight = "", const QString& transport = "", const QString& broker_name = "",
        const QString& voucher_no = ""
    );

    Q_INVOKABLE QVariantList get_sales_register(const QString& param1 = "", const QString& param2 = "");
};
