#include "purchase_model.h"
#include "../database_manager.h"
#include "../engine/accounting_engine.h"
#include <QDate>
#include <QRegularExpression>

PurchaseModel::PurchaseModel(QObject* parent)
    : BaseTableModel(
        {"Voucher No", "Bill No", "Date", "Supplier", "Item", "Bags", "Weight (Qtl)", "Rate (₹)", "Taxable (₹)", "GST %", "Total (₹)", "Mode"},
        {"voucher_no", "invoice_no", "invoice_date", "supplier_name", "item_name", "bag_count", "weight_qtl", "rate_per_qtl", "taxable_amount", "gst_pct", "total_amount", "payment_mode"},
        parent
    )
{
    reload_data();
}

void PurchaseModel::reload_data() {
    beginResetModel();
    m_data = DatabaseManager::instance().executeQuery("SELECT * FROM purchase_invoices ORDER BY id DESC;");
    endResetModel();
    emit dataChangedSignal();
    emit countChanged();
}

static QString incrementInvoiceStr(const QString& invStr, const QString& defaultPrefix = "PUR/") {
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

QString PurchaseModel::get_next_voucher_no(const QString& fy) {
    QString targetFy = fy;
    if (targetFy.isEmpty()) {
        QVariant fyVal = DatabaseManager::instance().executeScalar("SELECT year_name FROM financial_years WHERE is_active = 1 LIMIT 1;");
        targetFy = fyVal.isValid() ? fyVal.toString() : "FY 2026-27";
    }

    QVariantList rows = DatabaseManager::instance().executeQuery(
        "SELECT voucher_no FROM vouchers WHERE voucher_no LIKE 'Purc-%' AND financial_year = ?;",
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
    return QString("Purc-%1").arg(maxId + 1);
}

QString PurchaseModel::get_next_invoice_no(const QString& fy) {
    QString targetFy = fy;
    if (targetFy.isEmpty()) {
        QVariant fyVal = DatabaseManager::instance().executeScalar("SELECT year_name FROM financial_years WHERE is_active = 1 LIMIT 1;");
        targetFy = fyVal.isValid() ? fyVal.toString() : "FY 2026-27";
    }

    QVariant lastInv = DatabaseManager::instance().executeScalar(
        "SELECT invoice_no FROM purchase_invoices WHERE financial_year = ? AND invoice_no IS NOT NULL AND invoice_no != '' ORDER BY id DESC LIMIT 1;",
        {targetFy}
    );

    if (lastInv.isValid() && !lastInv.toString().isEmpty()) {
        return incrementInvoiceStr(lastInv.toString(), "PUR/");
    }
    return "PUR/1";
}

QString PurchaseModel::increment_invoice(const QString& invStr) {
    return incrementInvoiceStr(invStr, "PUR/");
}

bool PurchaseModel::add_purchase_invoice_full(
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
    
    // Resolve FY
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

    bool ok = DatabaseManager::instance().executeNonQuery(
        "INSERT INTO purchase_invoices ("
        "fy_id, financial_year, voucher_no, invoice_no, invoice_date, supplier_id, supplier_name, gstin, item_name, hsn_code, bag_count, weight_qtl, rate_per_qtl, taxable_amount, gst_pct, cgst_amount, sgst_amount, igst_amount, round_off, gst_amount, total_amount, payment_mode, vehicle_no, eway_bill_no, narration, sale_status, market_fee_status, dami, labour, auction, m_fee, hrdf, other_exp, welfare, dhrmd, sutli, less_amount, gr_no, driver, bill_time, sauda_date, shipping_address, po_no, grade, kanda_weight, transport, broker_name"
        ") VALUES (?, ?, ?, ?, ?, 1, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);",
        {
            fyId, fyLabel, vchNo, invNo, dt, party_ledger, gstin, item_name, hsn_code,
            bag_count, weight_qtl, rate_per_qtl, taxable_amount, gst_pct, cgst_amount, sgst_amount, igst_amount, round_off, gst_amount, total_amount,
            payment_mode, vehicle_no, eway_bill_no, narration, sale_status, market_fee_status,
            dami, labour, auction, m_fee, hrdf, other_exp, welfare, dhrmd, sutli, less_amount,
            gr_no, driver, bill_time, sauda_date, shipping_address, po_no, grade, kanda_weight, transport, broker_name
        }
    );

    if (ok) {
        QString vchNarr = QString("Purchase Bill %1 - %2 (%3 Qtl @ ₹%4)").arg(invNo, item_name, QString::number(weight_qtl), QString::number(rate_per_qtl));
        if (!narration.isEmpty()) vchNarr += " | " + narration;

        DatabaseManager::instance().executeNonQuery(
            "INSERT INTO vouchers (fy_id, financial_year, voucher_no, instrument_no, voucher_date, voucher_type, legacy_type, party_name, account_type, amount, narration) "
            "VALUES (?, ?, ?, ?, ?, 'Purchase', 'Purc', ?, 'Purchase Account', ?, ?);",
            {fyId, fyLabel, vchNo, invNo, dt, party_ledger, total_amount, vchNarr}
        );
        reload_data();
    }
    return ok;
}

QVariantList PurchaseModel::get_purchase_register(const QString& param1, const QString& param2) {
    QString sql;
    QVariantList params;
    if (!param1.isEmpty() && !param2.isEmpty()) {
        sql = "SELECT id, voucher_no, invoice_no, invoice_date, supplier_name, item_name, bag_count, weight_qtl, rate_per_qtl, taxable_amount, gst_pct, cgst_amount, sgst_amount, igst_amount, gst_amount, round_off, total_amount, payment_mode, vehicle_no, eway_bill_no, financial_year, narration FROM purchase_invoices WHERE invoice_date >= ? AND invoice_date <= ? ORDER BY invoice_date DESC, id DESC;";
        params << param1 << param2;
    } else if (!param1.isEmpty() && param1 != "All") {
        sql = "SELECT id, voucher_no, invoice_no, invoice_date, supplier_name, item_name, bag_count, weight_qtl, rate_per_qtl, taxable_amount, gst_pct, cgst_amount, sgst_amount, igst_amount, gst_amount, round_off, total_amount, payment_mode, vehicle_no, eway_bill_no, financial_year, narration FROM purchase_invoices WHERE financial_year = ? ORDER BY invoice_date DESC, id DESC;";
        params << param1;
    } else {
        sql = "SELECT id, voucher_no, invoice_no, invoice_date, supplier_name, item_name, bag_count, weight_qtl, rate_per_qtl, taxable_amount, gst_pct, cgst_amount, sgst_amount, igst_amount, gst_amount, round_off, total_amount, payment_mode, vehicle_no, eway_bill_no, financial_year, narration FROM purchase_invoices ORDER BY invoice_date DESC, id DESC;";
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
