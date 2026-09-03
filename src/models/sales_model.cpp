#include "sales_model.h"
#include "../database_manager.h"
#include "../engine/accounting_engine.h"
#include <QDate>
#include <QRegularExpression>

SalesModel::SalesModel(QObject* parent)
    : BaseTableModel(
        {"Voucher No", "Invoice No", "Date", "Customer", "Item", "Bags", "Weight (Qtl)", "Rate (₹)", "Taxable (₹)", "GST %", "Total (₹)", "Mode"},
        {"voucher_no", "invoice_no", "invoice_date", "customer_name", "item_name", "bag_count", "weight_qtl", "rate_per_qtl", "taxable_amount", "gst_pct", "total_amount", "payment_mode"},
        parent
    )
{
    reload_data();
}

void SalesModel::reload_data() {
    beginResetModel();
    m_data = DatabaseManager::instance().executeQuery("SELECT * FROM sales_invoices ORDER BY id DESC;");
    endResetModel();
    emit dataChangedSignal();
    emit countChanged();
}

static QString incrementInvoiceStr(const QString& invStr, const QString& defaultPrefix = "MRI/2627-") {
    if (invStr.trimmed().isEmpty()) return defaultPrefix + "1";
    QString invClean = invStr.trimmed();
    QRegularExpression re("^(.*?)(\\d+)$");
    QRegularExpressionMatch m = re.match(invClean);
    if (m.hasMatch()) {
        QString prefix = m.captured(1);
        QString numStr = m.captured(2);
        long long nextVal = numStr.toLongLong() + 1;
        QString nextNumStr;
        if (numStr.startsWith('0') && numStr.length() > 1) {
            nextNumStr = QString::number(nextVal).rightJustified(numStr.length(), '0');
        } else {
            nextNumStr = QString::number(nextVal);
        }
        return prefix + nextNumStr;
    }
    return invClean + "-1";
}

QString SalesModel::get_next_voucher_no(const QString& fy) {
    QString targetFy = fy;
    if (targetFy.isEmpty()) {
        QVariant fyVal = DatabaseManager::instance().executeScalar("SELECT year_name FROM financial_years WHERE is_active = 1 LIMIT 1;");
        targetFy = fyVal.isValid() ? fyVal.toString() : "FY 2026-27";
    }

    QVariantList rows = DatabaseManager::instance().executeQuery(
        "SELECT voucher_no FROM vouchers WHERE voucher_no LIKE 'Sale-%' AND financial_year = ?;",
        {targetFy}
    );

    long long maxId = 0;
    QRegularExpression re("-(\\d+)$");
    for (const QVariant& r : rows) {
        QString v = r.toMap().value("voucher_no").toString();
        QRegularExpressionMatch m = re.match(v);
        if (m.hasMatch()) {
            long long num = m.captured(1).toLongLong();
            if (num > maxId) maxId = num;
        }
    }
    return QString("Sale-%1").arg(maxId + 1);
}

QString SalesModel::get_next_invoice_no(const QString& fy) {
    QString targetFy = fy;
    if (targetFy.isEmpty()) {
        QVariant fyVal = DatabaseManager::instance().executeScalar("SELECT year_name FROM financial_years WHERE is_active = 1 LIMIT 1;");
        targetFy = fyVal.isValid() ? fyVal.toString() : "FY 2026-27";
    }

    QVariant lastInv = DatabaseManager::instance().executeScalar(
        "SELECT invoice_no FROM sales_invoices WHERE financial_year = ? AND invoice_no IS NOT NULL AND invoice_no != '' ORDER BY id DESC LIMIT 1;",
        {targetFy}
    );

    QString defaultPrefix = "MRI/2627-";
    QRegularExpression fyRe("(\\d{2})(\\d{2})-(\\d{2})");
    QRegularExpressionMatch m = fyRe.match(targetFy);
    if (m.hasMatch()) {
        defaultPrefix = QString("MRI/%1%2-").arg(m.captured(2), m.captured(3));
    }

    if (lastInv.isValid() && !lastInv.toString().isEmpty()) {
        return incrementInvoiceStr(lastInv.toString(), defaultPrefix);
    }
    return defaultPrefix + "1";
}

QString SalesModel::increment_invoice(const QString& invStr) {
    return incrementInvoiceStr(invStr);
}

bool SalesModel::add_sales_invoice_full(
    const QString& invoice_no, const QString& invoice_date, const QString& party_ledger,
    const QString& gstin, const QString& item_name, const QString& hsn_code,
    int bag_count, double weight_qtl, double rate_per_qtl, double taxable_amount,
    double gst_pct, double cgst_amount, double sgst_amount, double igst_amount,
    double round_off, double total_amount, const QString& payment_mode,
    const QString& vehicle_no, const QString& eway_bill_no, const QString& narration,
    const QString& sale_status, const QString& market_fee_status,
    double dami, double labour, double auction, double m_fee,
    double hrdf, double other_exp, double welfare, double dhrmd,
    double sutli, double less_amount, const QString& gr_no,
    const QString& driver, const QString& bill_time, const QString& sauda_date,
    const QString& shipping_address, const QString& po_no, const QString& grade,
    const QString& kanda_weight, const QString& transport, const QString& broker_name,
    const QString& voucher_no
) {
    QString dt = invoice_date.isEmpty() ? QDate::currentDate().toString("yyyy-MM-dd") : invoice_date;
    
    // Resolve Financial Year
    QVariantList fyRows = DatabaseManager::instance().executeQuery(
        "SELECT id, year_name FROM financial_years WHERE start_date <= ? AND end_date >= ? LIMIT 1;",
        {dt, dt}
    );
    int fyId = 28;
    QString fyLabel = "FY 2026-27";
    if (!fyRows.isEmpty()) {
        fyId = fyRows.first().toMap().value("id").toInt();
        fyLabel = fyRows.first().toMap().value("year_name").toString();
    }

    QString vchNo = voucher_no.isEmpty() ? get_next_voucher_no(fyLabel) : voucher_no;
    QString invNo = invoice_no.isEmpty() ? get_next_invoice_no(fyLabel) : invoice_no;
    double gst_amount = cgst_amount + sgst_amount + igst_amount;

    // Resolve Item ID
    QVariant itemRow = DatabaseManager::instance().executeScalar(
        "SELECT id FROM stock_items WHERE name = ? LIMIT 1;",
        {item_name}
    );
    int itemId = itemRow.isValid() ? itemRow.toInt() : 1;

    // Resolve Customer ID
    QVariant custRow = DatabaseManager::instance().executeScalar(
        "SELECT id FROM parties WHERE name = ? LIMIT 1;",
        {party_ledger}
    );
    int customerId = custRow.isValid() ? custRow.toInt() : 1;

    // --- ATOMIC ACID TRANSACTION ---
    DatabaseManager::instance().beginTransaction();

    // 1. Insert Sales Invoice Record
    bool okInv = DatabaseManager::instance().executeNonQuery(
        "INSERT INTO sales_invoices ("
        "fy_id, financial_year, voucher_no, invoice_no, invoice_date, customer_id, customer_name, gstin, item_id, item_name, hsn_code, "
        "bag_count, weight_qtl, rate_per_qtl, taxable_amount, gst_pct, cgst_amount, sgst_amount, igst_amount, round_off, gst_amount, "
        "total_amount, payment_mode, vehicle_no, eway_bill_no, narration, sale_status, market_fee_status, dami, labour, auction, "
        "m_fee, hrdf, other_exp, welfare, dhrmd, sutli, less_amount, gr_no, driver, bill_time, sauda_date, shipping_address, "
        "po_no, grade, kanda_weight, transport, broker_name"
        ") VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);",
        {
            fyId, fyLabel, vchNo, invNo, dt, customerId, party_ledger, gstin, itemId, item_name, hsn_code,
            bag_count, weight_qtl, rate_per_qtl, taxable_amount, gst_pct, cgst_amount, sgst_amount, igst_amount, round_off, gst_amount,
            total_amount, payment_mode, vehicle_no, eway_bill_no, narration, sale_status, market_fee_status,
            dami, labour, auction, m_fee, hrdf, other_exp, welfare, dhrmd, sutli, less_amount,
            gr_no, driver, bill_time, sauda_date, shipping_address, po_no, grade, kanda_weight, transport, broker_name
        }
    );

    if (!okInv) {
        DatabaseManager::instance().rollback();
        return false;
    }

    // 2. Guaranteed Double-Entry Ledger Posting:
    // Debit: Customer Account for Total Amount
    // Credit: Sales Account for Taxable Amount + GST Output Accounts
    QString vchNarr = QString("Sales Invoice %1 - %2 (%3 Qtl @ ₹%4)").arg(invNo, item_name, QString::number(weight_qtl), QString::number(rate_per_qtl));
    if (!narration.isEmpty()) vchNarr += " | " + narration;

    bool okVch = DatabaseManager::instance().executeNonQuery(
        "INSERT INTO vouchers ("
        "fy_id, financial_year, voucher_no, instrument_no, voucher_date, voucher_type, legacy_type, ledger_id, party_id, party_name, "
        "account_type, amount, taxable_amount, gst_pct, cgst_amount, sgst_amount, igst_amount, round_off, vehicle_no, eway_bill_no, "
        "broker_name, sauda_date, dami, labour, auction, m_fee, hrdf, other_exp, welfare, dhrmd, sutli, less_amount, narration"
        ") VALUES (?, ?, ?, ?, ?, 'Sales', 'Sale', ?, ?, ?, 'Sales Account', ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);",
        {
            fyId, fyLabel, vchNo, invNo, dt, customerId, customerId, party_ledger,
            total_amount, taxable_amount, gst_pct, cgst_amount, sgst_amount, igst_amount, round_off, vehicle_no, eway_bill_no,
            broker_name, sauda_date, dami, labour, auction, m_fee, hrdf, other_exp, welfare, dhrmd, sutli, less_amount, vchNarr
        }
    );

    if (!okVch) {
        DatabaseManager::instance().rollback();
        return false;
    }

    // 3. Guaranteed Stock Transaction Outward Posting:
    bool okStock = DatabaseManager::instance().executeNonQuery(
        "INSERT INTO stock_transactions ("
        "fy_id, financial_year, voucher_no, voucher_date, trans_type, voucher_type, party_id, party_name, bill_no, "
        "item_id, item_code, item_name, bags, weight_qtl, rate, amount, taxable_amount, narration"
        ") VALUES (?, ?, ?, ?, 'Sale', 'Sales Invoice', ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);",
        {
            fyId, fyLabel, vchNo, dt, customerId, party_ledger, invNo,
            itemId, hsn_code, item_name, bag_count, weight_qtl, rate_per_qtl, total_amount, taxable_amount, vchNarr
        }
    );

    if (!okStock) {
        DatabaseManager::instance().rollback();
        return false;
    }

    // Commit Transaction (ACID Durability Guarantee)
    DatabaseManager::instance().commit();
    reload_data();
    return true;
}

QVariantList SalesModel::get_sales_register(const QString& param1, const QString& param2) {
    QString sql;
    QVariantList params;
    QString fromDate, toDate, fyLabel;

    if (!param1.isEmpty() && !param2.isEmpty()) {
        fromDate = param1;
        toDate = param2;
    } else if (!param1.isEmpty() && param1 != "All") {
        fyLabel = param1;
    } else if (param1 != "All") {
        QVariantList fyRows = DatabaseManager::instance().executeQuery("SELECT year_name, start_date, end_date FROM financial_years WHERE is_active = 1 LIMIT 1;");
        if (!fyRows.isEmpty()) {
            QVariantMap r = fyRows.first().toMap();
            fyLabel = r.value("year_name").toString();
            fromDate = r.value("start_date").toString();
            toDate = r.value("end_date").toString();
        }
    }

    if (!fromDate.isEmpty() && !toDate.isEmpty()) {
        sql = "SELECT id, voucher_no, invoice_no, invoice_date, customer_name, item_name, bag_count, weight_qtl, rate_per_qtl, taxable_amount, gst_pct, cgst_amount, sgst_amount, igst_amount, gst_amount, round_off, total_amount, payment_mode, vehicle_no, eway_bill_no, financial_year, narration FROM sales_invoices WHERE invoice_date >= ? AND invoice_date <= ? ORDER BY invoice_date DESC, id DESC;";
        params << fromDate << toDate;
    } else if (!fyLabel.isEmpty()) {
        sql = "SELECT id, voucher_no, invoice_no, invoice_date, customer_name, item_name, bag_count, weight_qtl, rate_per_qtl, taxable_amount, gst_pct, cgst_amount, sgst_amount, igst_amount, gst_amount, round_off, total_amount, payment_mode, vehicle_no, eway_bill_no, financial_year, narration FROM sales_invoices WHERE financial_year = ? ORDER BY invoice_date DESC, id DESC;";
        params << fyLabel;
    } else {
        sql = "SELECT id, voucher_no, invoice_no, invoice_date, customer_name, item_name, bag_count, weight_qtl, rate_per_qtl, taxable_amount, gst_pct, cgst_amount, sgst_amount, igst_amount, gst_amount, round_off, total_amount, payment_mode, vehicle_no, eway_bill_no, financial_year, narration FROM sales_invoices ORDER BY invoice_date DESC, id DESC;";
    }

    QVariantList rawRows = DatabaseManager::instance().executeQuery(sql, params);
    QVariantList result;
    for (const QVariant& r : rawRows) {
        QVariantMap row = r.toMap();
        double bags = row.value("bag_count").toDouble();
        double wt = row.value("weight_qtl").toDouble();
        double rate = row.value("rate_per_qtl").toDouble();
        double taxable = row.value("taxable_amount").toDouble();
        double gstTot = row.value("gst_amount").toDouble();
        double total = row.value("total_amount").toDouble();

        row["bag_count_fmt"] = AccountingEngine::formatIndianNumber(bags, 0);
        row["weight_qtl_fmt"] = AccountingEngine::formatIndianNumber(wt, 2, "Qtl");
        row["rate_fmt"] = AccountingEngine::formatIndianCurrency(rate);
        row["taxable_amount_fmt"] = AccountingEngine::formatIndianCurrency(taxable);
        row["gst_amount_fmt"] = AccountingEngine::formatIndianCurrency(gstTot);
        row["total_amount_fmt"] = AccountingEngine::formatIndianCurrency(total);
        result.append(row);
    }
    return result;
}

QVariantMap SalesModel::get_sales_invoice(const QString& invoiceNoOrId) {
    QVariantList rows = DatabaseManager::instance().executeQuery(
        "SELECT * FROM sales_invoices WHERE invoice_no = ? OR id = ? LIMIT 1;",
        {invoiceNoOrId, invoiceNoOrId}
    );
    if (rows.isEmpty()) return {};
    QVariantMap inv = rows.first().toMap();
    int invId = inv.value("id").toInt();

    // Fetch line items from sales_invoice_items
    QVariantList itemRows = DatabaseManager::instance().executeQuery(
        "SELECT * FROM sales_invoice_items WHERE invoice_id = ? ORDER BY id ASC;",
        {invId}
    );
    // If no sales_invoice_items, create single line item from header
    if (itemRows.isEmpty() && !inv.value("item_name").toString().isEmpty()) {
        QVariantMap itm;
        itm["item_name"] = inv.value("item_name");
        itm["bags"] = inv.value("bag_count");
        itm["packing"] = 0.5;
        itm["weight"] = inv.value("weight_qtl");
        itm["rate"] = inv.value("rate_per_qtl");
        itm["gst_pct"] = inv.value("gst_pct");
        itm["amount"] = inv.value("taxable_amount");
        itm["hsn_code"] = inv.value("hsn_code");
        itemRows.append(itm);
    } else {
        for (int i = 0; i < itemRows.size(); ++i) {
            QVariantMap itm = itemRows[i].toMap();
            itm["bags"] = itm.value("bag_count");
            itm["weight"] = itm.value("weight_qtl");
            itm["rate"] = itm.value("rate_per_qtl");
            itm["amount"] = itm.value("total_amount").toDouble() > 0 ? itm.value("total_amount") : itm.value("taxable_amount");
            itemRows[i] = itm;
        }
    }
    inv["items"] = itemRows;
    return inv;
}

bool SalesModel::update_sales_invoice_full(
    int invoice_id,
    const QString& invoice_no, const QString& invoice_date, const QString& party_ledger,
    const QString& gstin, const QString& item_name, const QString& hsn_code,
    int bag_count, double weight_qtl, double rate_per_qtl, double taxable_amount,
    double gst_pct, double cgst_amount, double sgst_amount, double igst_amount,
    double round_off, double total_amount, const QString& payment_mode,
    const QString& vehicle_no, const QString& eway_bill_no, const QString& narration,
    const QString& sale_status, const QString& market_fee_status,
    double dami, double labour, double auction, double m_fee,
    double hrdf, double other_exp, double welfare, double dhrmd,
    double sutli, double less_amount, const QString& gr_no,
    const QString& driver, const QString& bill_time, const QString& sauda_date,
    const QString& shipping_address, const QString& po_no, const QString& grade,
    const QString& kanda_weight, const QString& transport, const QString& broker_name,
    const QString& voucher_no,
    const QVariantList& items
) {
    QString dt = invoice_date.isEmpty() ? QDate::currentDate().toString("yyyy-MM-dd") : invoice_date;
    QVariant itemRow = DatabaseManager::instance().executeScalar(
        "SELECT id FROM stock_items WHERE name = ? LIMIT 1;", {item_name}
    );
    int itemId = itemRow.isValid() ? itemRow.toInt() : 1;
    QVariant custRow = DatabaseManager::instance().executeScalar(
        "SELECT id FROM parties WHERE name = ? LIMIT 1;", {party_ledger}
    );
    int customerId = custRow.isValid() ? custRow.toInt() : 1;
    double gst_amount = cgst_amount + sgst_amount + igst_amount;

    DatabaseManager::instance().beginTransaction();

    bool okInv = DatabaseManager::instance().executeNonQuery(
        "UPDATE sales_invoices SET "
        "voucher_no = ?, invoice_no = ?, invoice_date = ?, customer_id = ?, customer_name = ?, gstin = ?, "
        "item_id = ?, item_name = ?, hsn_code = ?, bag_count = ?, weight_qtl = ?, rate_per_qtl = ?, "
        "taxable_amount = ?, gst_pct = ?, cgst_amount = ?, sgst_amount = ?, igst_amount = ?, round_off = ?, "
        "gst_amount = ?, total_amount = ?, payment_mode = ?, vehicle_no = ?, eway_bill_no = ?, narration = ?, "
        "sale_status = ?, market_fee_status = ?, dami = ?, labour = ?, auction = ?, m_fee = ?, hrdf = ?, "
        "other_exp = ?, welfare = ?, dhrmd = ?, sutli = ?, less_amount = ?, gr_no = ?, driver = ?, "
        "bill_time = ?, sauda_date = ?, shipping_address = ?, po_no = ?, grade = ?, kanda_weight = ?, "
        "transport = ?, broker_name = ? WHERE id = ?;",
        {
            voucher_no, invoice_no, dt, customerId, party_ledger, gstin,
            itemId, item_name, hsn_code, bag_count, weight_qtl, rate_per_qtl,
            taxable_amount, gst_pct, cgst_amount, sgst_amount, igst_amount, round_off,
            gst_amount, total_amount, payment_mode, vehicle_no, eway_bill_no, narration,
            sale_status, market_fee_status, dami, labour, auction, m_fee, hrdf,
            other_exp, welfare, dhrmd, sutli, less_amount, gr_no, driver,
            bill_time, sauda_date, shipping_address, po_no, grade, kanda_weight,
            transport, broker_name, invoice_id
        }
    );

    if (!okInv) {
        DatabaseManager::instance().rollback();
        return false;
    }

    // Replace line items in sales_invoice_items
    DatabaseManager::instance().executeNonQuery("DELETE FROM sales_invoice_items WHERE invoice_id = ?;", {invoice_id});
    if (!items.isEmpty()) {
        for (const QVariant& itmV : items) {
            QVariantMap itm = itmV.toMap();
            DatabaseManager::instance().executeNonQuery(
                "INSERT INTO sales_invoice_items (invoice_id, invoice_no, item_id, item_name, bag_count, packing, weight_qtl, rate_per_qtl, taxable_amount, gst_pct, total_amount) "
                "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);",
                {
                    invoice_id, invoice_no, itemId, itm.value("item_name").toString(),
                    itm.value("bags").toInt(), itm.value("packing").toDouble(),
                    itm.value("weight").toDouble(), itm.value("rate").toDouble(),
                    itm.value("amount").toDouble(), itm.value("gst_pct").toDouble(),
                    itm.value("amount").toDouble()
                }
            );
        }
    }

    // Update stock_transactions
    DatabaseManager::instance().executeNonQuery(
        "UPDATE stock_transactions SET "
        "voucher_no = ?, voucher_date = ?, party_id = ?, party_name = ?, bill_no = ?, "
        "item_id = ?, item_name = ?, bags = ?, weight_qtl = ?, rate = ?, amount = ?, taxable_amount = ? "
        "WHERE bill_no = ? AND trans_type IN ('Sale', 'S');",
        {
            voucher_no, dt, customerId, party_ledger, invoice_no,
            itemId, item_name, bag_count, weight_qtl, rate_per_qtl, total_amount, taxable_amount,
            invoice_no
        }
    );

    DatabaseManager::instance().commit();
    reload_data();
    return true;
}

