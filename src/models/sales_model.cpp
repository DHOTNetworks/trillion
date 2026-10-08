#include "sales_model.h"
#include "../database_manager.h"
#include "../engine/accounting_engine.h"
#include "../engine/fiscal_year_helper.h"
#include <QDate>
#include <QRegularExpression>

SalesModel::SalesModel(QObject* parent)
    : BaseTableModel(
        {"Voucher No", "Invoice No", "Date", "Customer", "Item", "Bags", "Weight (Qtl)", "Rate (₹)", "Taxable (₹)", "GST %", "Total (₹)", "Mode"},
        {"voucher_no", "invoice_no", "invoice_date", "customer_name", "item_name", "bag_count", "weight_qtl", "rate_per_qtl", "taxable_amount", "gst_pct", "total_amount", "payment_mode"},
        parent
    )
{
}

void SalesModel::reload_data() {
    beginResetModel();
    m_data = DatabaseManager::instance().executeQuery("SELECT * FROM sales_invoices ORDER BY id DESC;");
    endResetModel();
    emit dataChangedSignal();
    emit countChanged();
}

static QString incrementInvoiceStr(const QString& invStr, const QString& defaultPrefix = "") {
    QString pfx = defaultPrefix;
    if (pfx.isEmpty()) {
        FiscalYearInfo activeFy = FiscalYearHelper::getActiveFiscalYear();
        QString shortFy = activeFy.name.mid(3).remove('-').remove(' ');
        pfx = "INV/" + shortFy + "-";
    }
    if (invStr.trimmed().isEmpty()) return pfx + "1";
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
    QString targetFy = AccountingEngine::resolveFinancialYear(fy);
    QString fyPattern = "%" + targetFy.mid(3).trimmed() + "%";

    QVariantList siRows = DatabaseManager::instance().executeQuery(
        "SELECT voucher_no FROM sales_invoices WHERE financial_year = ? OR financial_year LIKE ?;",
        {targetFy, fyPattern}
    );

    QVariantList vRows = DatabaseManager::instance().executeQuery(
        "SELECT voucher_no FROM vouchers WHERE (voucher_type = 'Sales' OR voucher_no LIKE 'Sale-%') AND (financial_year = ? OR financial_year LIKE ?);",
        {targetFy, fyPattern}
    );

    long long maxId = 0;
    QRegularExpression re("(\\d+)$");
    for (const QVariantList* list : {&siRows, &vRows}) {
        for (const QVariant& r : *list) {
            QString v = r.toMap().value("voucher_no").toString().trimmed();
            if (v.isEmpty()) continue;
            QRegularExpressionMatch m = re.match(v);
            if (m.hasMatch()) {
                long long num = m.captured(1).toLongLong();
                if (num > maxId) maxId = num;
            }
        }
    }
    return QString("Sale-%1").arg(maxId + 1);
}

QString SalesModel::get_next_invoice_no(const QString& fy) {
    QString targetFy = AccountingEngine::resolveFinancialYear(fy);
    QString fyPattern = "%" + targetFy.mid(3).trimmed() + "%";

    QVariantList rows = DatabaseManager::instance().executeQuery(
        "SELECT invoice_no FROM sales_invoices WHERE (financial_year = ? OR financial_year LIKE ?) AND invoice_no IS NOT NULL AND invoice_no != '';",
        {targetFy, fyPattern}
    );

    QString defaultPrefix = "MRI/2627-";
    QRegularExpression fyRe("(\\d{2})(\\d{2})-(\\d{2})");
    QRegularExpressionMatch mFy = fyRe.match(targetFy);
    if (mFy.hasMatch()) {
        defaultPrefix = QString("MRI/%1%2-").arg(mFy.captured(2), mFy.captured(3));
    }

    QString prefix = "";
    long long maxNum = 0;
    int numDigits = 0;

    QRegularExpression re("^(.*?)(\\d+)$");
    for (const QVariant& r : rows) {
        QString inv = r.toMap().value("invoice_no").toString().trimmed();
        if (inv.isEmpty()) continue;
        QRegularExpressionMatch m = re.match(inv);
        if (m.hasMatch()) {
            QString pfx = m.captured(1);
            QString digits = m.captured(2);
            long long num = digits.toLongLong();
            if (num > maxNum) {
                maxNum = num;
                prefix = pfx;
                numDigits = digits.length();
            }
        }
    }

    if (maxNum > 0) {
        if (prefix.isEmpty()) prefix = defaultPrefix;
        long long nextNum = maxNum + 1;
        QString nextNumStr = QString::number(nextNum);
        if (numDigits > 1 && nextNumStr.length() < numDigits) {
            nextNumStr = nextNumStr.rightJustified(numDigits, '0');
        }
        return prefix + nextNumStr;
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
    const QString& voucher_no,
    const QVariantList& items,
    const QString& market_type,
    int due_days,
    const QString& challan_no,
    double freight_charges,
    double tcs_amount,
    double tcs_rate,
    const QString& tax_status,
    const QString& place_of_supply
) {
    QString dt = invoice_date.isEmpty() ? QDate::currentDate().toString("yyyy-MM-dd") : invoice_date;
    
    // Resolve Financial Year
    FiscalYearInfo fy = FiscalYearHelper::getFiscalYearForDate(dt);
    int fyId = 1;
    QString fyLabel = fy.name;
    QVariant fyIdVar = DatabaseManager::instance().executeScalar(
        "SELECT id FROM financial_years WHERE year_name = ? LIMIT 1;",
        {fy.name}
    );
    if (fyIdVar.isValid() && !fyIdVar.isNull()) {
        fyId = fyIdVar.toInt();
    } else {
        QVariant anyFy = DatabaseManager::instance().executeScalar("SELECT id FROM financial_years WHERE is_active = 1 LIMIT 1;");
        if (anyFy.isValid() && !anyFy.isNull()) fyId = anyFy.toInt();
    }

    QString vchNo = voucher_no.isEmpty() ? get_next_voucher_no(fyLabel) : voucher_no;
    QString invNo = invoice_no.isEmpty() ? get_next_invoice_no(fyLabel) : invoice_no;
    double gst_amount = cgst_amount + sgst_amount + igst_amount;

    // Resolve Item ID
    int itemId = 1;
    QVariant itemRow = DatabaseManager::instance().executeScalar(
        "SELECT id FROM stock_items WHERE name = ? COLLATE NOCASE LIMIT 1;",
        {item_name}
    );
    if (!itemRow.isValid()) {
        itemRow = DatabaseManager::instance().executeScalar(
            "SELECT id FROM stock_items WHERE code = ? OR name LIKE ? LIMIT 1;",
            {item_name, "%" + item_name + "%"}
        );
    }
    if (itemRow.isValid()) itemId = itemRow.toInt();

    // Resolve Customer ID
    int customerId = 1;
    QVariant custRow = DatabaseManager::instance().executeScalar(
        "SELECT id FROM parties WHERE name = ? COLLATE NOCASE LIMIT 1;",
        {party_ledger}
    );
    if (!custRow.isValid()) {
        custRow = DatabaseManager::instance().executeScalar(
            "SELECT id FROM parties WHERE alias = ? COLLATE NOCASE OR name LIKE ? LIMIT 1;",
            {party_ledger, "%" + party_ledger + "%"}
        );
    }
    if (custRow.isValid()) customerId = custRow.toInt();

    // --- ATOMIC ACID TRANSACTION ---
    DatabaseManager::instance().beginTransaction();

    // 1. Insert Sales Invoice Record
    bool okInv = DatabaseManager::instance().executeNonQuery(
        "INSERT INTO sales_invoices ("
        "fy_id, financial_year, voucher_no, invoice_no, invoice_date, customer_id, customer_name, gstin, item_id, item_name, hsn_code, "
        "bag_count, weight_qtl, rate_per_qtl, taxable_amount, gst_pct, cgst_amount, sgst_amount, igst_amount, round_off, gst_amount, "
        "total_amount, payment_mode, vehicle_no, eway_bill_no, narration, sale_status, market_fee_status, dami, labour, auction, "
        "m_fee, hrdf, other_exp, welfare, dhrmd, sutli, less_amount, gr_no, driver, bill_time, sauda_date, shipping_address, "
        "po_no, grade, kanda_weight, transport, broker_name, market_type, due_days, tax_status, challan_no, freight_charges, "
        "tcs_amount, tcs_rate, place_of_supply"
        ") VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);",
        {
            fyId, fyLabel, vchNo, invNo, dt, customerId, party_ledger, gstin, itemId, item_name, hsn_code,
            bag_count, weight_qtl, rate_per_qtl, taxable_amount, gst_pct, cgst_amount, sgst_amount, igst_amount, round_off, gst_amount,
            total_amount, payment_mode, vehicle_no, eway_bill_no, narration, sale_status, market_fee_status,
            dami, labour, auction, m_fee, hrdf, other_exp, welfare, dhrmd, sutli, less_amount,
            gr_no, driver, bill_time, sauda_date, shipping_address, po_no, grade, kanda_weight, transport, broker_name,
            market_type, due_days, tax_status, challan_no, freight_charges, tcs_amount, tcs_rate, place_of_supply
        }
    );

    if (!okInv) {
        DatabaseManager::instance().rollback();
        return false;
    }

    long long newInvId = DatabaseManager::instance().lastInsertedId();

    // Insert line items if present
    if (!items.isEmpty()) {
        for (const QVariant& itmV : items) {
            QVariantMap itm = itmV.toMap();
            DatabaseManager::instance().executeNonQuery(
                "INSERT INTO sales_invoice_items (invoice_id, invoice_no, item_id, item_name, grade, bag_count, packing, weight_qtl, rate_per_qtl, taxable_amount, gst_pct, total_amount) "
                "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);",
                {
                    newInvId, invNo, itemId, itm.value("item_name").toString(),
                    itm.value("grade").toString(),
                    itm.value("bags").toInt(), itm.value("packing").toDouble(),
                    itm.value("weight").toDouble(), itm.value("rate").toDouble(),
                    itm.value("amount").toDouble(), itm.value("gst_pct").toDouble(),
                    itm.value("amount").toDouble()
                }
            );
        }
    }

    // 2. Guaranteed Double-Entry Ledger Posting:
    // Debit: Customer Account for Total Amount
    // Credit: Stock Item's Specific Sale Account for Taxable Amount + GST Output Accounts
    QString itemSaleLedger = "";
    QVariantList itemMeta = DatabaseManager::instance().executeQuery(
        "SELECT sale_ledger, sale_ledger_id FROM stock_items WHERE id = ? LIMIT 1;",
        {itemId}
    );
    if (!itemMeta.isEmpty()) {
        itemSaleLedger = itemMeta.first().toMap().value("sale_ledger").toString().trimmed();
    }

    QString vchNarr = narration.trimmed();

    bool okVch = DatabaseManager::instance().executeNonQuery(
        "INSERT INTO vouchers ("
        "fy_id, financial_year, voucher_no, instrument_no, voucher_date, voucher_type, legacy_type, ledger_id, party_id, party_name, "
        "account_type, amount, taxable_amount, gst_pct, cgst_amount, sgst_amount, igst_amount, round_off, vehicle_no, eway_bill_no, "
        "broker_name, sauda_date, dami, labour, auction, m_fee, hrdf, other_exp, welfare, dhrmd, sutli, less_amount, narration, "
        "due_days, market_type, tax_status, place_of_supply, challan_no"
        ") VALUES (?, ?, ?, ?, ?, 'Sales', 'Sale', ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);",
        {
            fyId, fyLabel, vchNo, invNo, dt, customerId, customerId, party_ledger,
            itemSaleLedger, total_amount, taxable_amount, gst_pct, cgst_amount, sgst_amount, igst_amount, round_off, vehicle_no, eway_bill_no,
            broker_name, sauda_date, dami, labour, auction, m_fee, hrdf, other_exp, welfare, dhrmd, sutli, less_amount, vchNarr,
            due_days, market_type, tax_status, place_of_supply, challan_no
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
        ") VALUES (?, ?, ?, ?, 'Sale', 'Sales', ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);",
        {
            fyId, fyLabel, vchNo, dt, customerId, party_ledger, invNo,
            itemId, hsn_code, item_name, bag_count, weight_qtl, rate_per_qtl, total_amount, taxable_amount, vchNarr
        }
    );

    if (!okStock) {
        DatabaseManager::instance().rollback();
        return false;
    }

    // 4. Guaranteed Double-Entry Ledger Posting into transactions:
    // Delete existing transaction splits if any for idempotency
    DatabaseManager::instance().executeNonQuery(
        "DELETE FROM transactions WHERE (voucher_no = ? OR invoice_no = ?) AND voucher_type IN ('Sales', 'Sale');",
        {vchNo, invNo}
    );

    QString effectiveSaleLedger = itemSaleLedger.isEmpty() ? "Sale A/c" : itemSaleLedger;
    // Leg 1: Debit Customer for total_amount
    DatabaseManager::instance().executeNonQuery(
        "INSERT INTO transactions ("
        "fy_id, financial_year, voucher_no, voucher_date, voucher_type, trans_type, "
        "account_code, party_id, party_name, opposing_account, dr_cr, amount, "
        "invoice_no, narration, taxable_amount, row_no"
        ") VALUES (?, ?, ?, ?, 'Sales', 'Sale', ?, ?, ?, ?, 'Dr', ?, ?, ?, ?, 1);",
        {
            fyId, fyLabel, vchNo, dt,
            customerId, customerId, party_ledger, effectiveSaleLedger, total_amount,
            invNo, vchNarr, taxable_amount
        }
    );

    int rNo = 2;
    // Leg 2: Credit Sales Account for taxable_amount
    DatabaseManager::instance().executeNonQuery(
        "INSERT INTO transactions ("
        "fy_id, financial_year, voucher_no, voucher_date, voucher_type, trans_type, "
        "party_name, opposing_account, dr_cr, amount, "
        "invoice_no, narration, taxable_amount, row_no"
        ") VALUES (?, ?, ?, ?, 'Sales', 'Sale', ?, ?, 'Cr', ?, ?, ?, ?, ?);",
        {
            fyId, fyLabel, vchNo, dt,
            effectiveSaleLedger, party_ledger, taxable_amount,
            invNo, vchNarr, taxable_amount, rNo++
        }
    );

    // Leg 3: Credit CGST Output if > 0
    if (cgst_amount > 0.001) {
        DatabaseManager::instance().executeNonQuery(
            "INSERT INTO transactions ("
            "fy_id, financial_year, voucher_no, voucher_date, voucher_type, trans_type, "
            "party_name, opposing_account, dr_cr, amount, "
            "invoice_no, narration, row_no"
            ") VALUES (?, ?, ?, ?, 'Sales', 'Sale', 'CGST Output', ?, 'Cr', ?, ?, ?, ?);",
            {fyId, fyLabel, vchNo, dt, party_ledger, cgst_amount, invNo, vchNarr, rNo++}
        );
    }
    // Leg 4: Credit SGST Output if > 0
    if (sgst_amount > 0.001) {
        DatabaseManager::instance().executeNonQuery(
            "INSERT INTO transactions ("
            "fy_id, financial_year, voucher_no, voucher_date, voucher_type, trans_type, "
            "party_name, opposing_account, dr_cr, amount, "
            "invoice_no, narration, row_no"
            ") VALUES (?, ?, ?, ?, 'Sales', 'Sale', 'SGST Output', ?, 'Cr', ?, ?, ?, ?);",
            {fyId, fyLabel, vchNo, dt, party_ledger, sgst_amount, invNo, vchNarr, rNo++}
        );
    }
    // Leg 5: Credit IGST Output if > 0
    if (igst_amount > 0.001) {
        DatabaseManager::instance().executeNonQuery(
            "INSERT INTO transactions ("
            "fy_id, financial_year, voucher_no, voucher_date, voucher_type, trans_type, "
            "party_name, opposing_account, dr_cr, amount, "
            "invoice_no, narration, row_no"
            ") VALUES (?, ?, ?, ?, 'Sales', 'Sale', 'IGST Output', ?, 'Cr', ?, ?, ?, ?);",
            {fyId, fyLabel, vchNo, dt, party_ledger, igst_amount, invNo, vchNarr, rNo++}
        );
    }
    // Leg 6: Round Off if != 0
    if (std::abs(round_off) > 0.001) {
        QString roDrCr = (round_off > 0) ? "Cr" : "Dr";
        DatabaseManager::instance().executeNonQuery(
            "INSERT INTO transactions ("
            "fy_id, financial_year, voucher_no, voucher_date, voucher_type, trans_type, "
            "party_name, opposing_account, dr_cr, amount, "
            "invoice_no, narration, row_no"
            ") VALUES (?, ?, ?, ?, 'Sales', 'Sale', 'Round Off', ?, ?, ?, ?, ?, ?);",
            {fyId, fyLabel, vchNo, dt, party_ledger, roDrCr, std::abs(round_off), invNo, vchNarr, rNo++}
        );
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
        sql = "SELECT id, voucher_no, invoice_no, invoice_date, customer_name, item_name, bag_count, weight_qtl, rate_per_qtl, taxable_amount, gst_pct, cgst_amount, sgst_amount, igst_amount, gst_amount, round_off, total_amount, payment_mode, vehicle_no, eway_bill_no, financial_year, narration, market_type, due_days, tax_status, challan_no, freight_charges, tcs_amount, place_of_supply FROM sales_invoices WHERE invoice_date >= ? AND invoice_date <= ? ORDER BY invoice_date DESC, id DESC;";
        params << fromDate << toDate;
    } else if (!fyLabel.isEmpty()) {
        sql = "SELECT id, voucher_no, invoice_no, invoice_date, customer_name, item_name, bag_count, weight_qtl, rate_per_qtl, taxable_amount, gst_pct, cgst_amount, sgst_amount, igst_amount, gst_amount, round_off, total_amount, payment_mode, vehicle_no, eway_bill_no, financial_year, narration, market_type, due_days, tax_status, challan_no, freight_charges, tcs_amount, place_of_supply FROM sales_invoices WHERE financial_year = ? ORDER BY invoice_date DESC, id DESC;";
        params << fyLabel;
    } else {
        sql = "SELECT id, voucher_no, invoice_no, invoice_date, customer_name, item_name, bag_count, weight_qtl, rate_per_qtl, taxable_amount, gst_pct, cgst_amount, sgst_amount, igst_amount, gst_amount, round_off, total_amount, payment_mode, vehicle_no, eway_bill_no, financial_year, narration, market_type, due_days, tax_status, challan_no, freight_charges, tcs_amount, place_of_supply FROM sales_invoices ORDER BY invoice_date DESC, id DESC;";
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

QVariantMap SalesModel::get_previous_sales_invoice(int currentId, const QString& currentInvOrVchNo) {
    QVariant targetId;
    if (currentId > 0) {
        targetId = DatabaseManager::instance().executeScalar(
            "SELECT id FROM sales_invoices WHERE id < ? ORDER BY id DESC LIMIT 1;", {currentId}
        );
    }
    if (!targetId.isValid() && !currentInvOrVchNo.trimmed().isEmpty()) {
        QVariant curRowId = DatabaseManager::instance().executeScalar(
            "SELECT id FROM sales_invoices WHERE invoice_no = ? OR voucher_no = ? LIMIT 1;",
            {currentInvOrVchNo, currentInvOrVchNo}
        );
        if (curRowId.isValid()) {
            targetId = DatabaseManager::instance().executeScalar(
                "SELECT id FROM sales_invoices WHERE id < ? ORDER BY id DESC LIMIT 1;", {curRowId.toInt()}
            );
        }
    }
    if (!targetId.isValid()) {
        targetId = DatabaseManager::instance().executeScalar(
            "SELECT id FROM sales_invoices ORDER BY id DESC LIMIT 1;"
        );
    }
    if (targetId.isValid()) {
        return get_sales_invoice(targetId.toInt());
    }

    // Fallback to vouchers table
    QVariant vId;
    if (currentId > 0) {
        vId = DatabaseManager::instance().executeScalar(
            "SELECT id FROM vouchers WHERE id < ? AND (voucher_type IN ('Sales', 'Sale') OR legacy_type IN ('Sale', 'Sales')) ORDER BY id DESC LIMIT 1;",
            {currentId}
        );
    }
    if (!vId.isValid()) {
        vId = DatabaseManager::instance().executeScalar(
            "SELECT id FROM vouchers WHERE (voucher_type IN ('Sales', 'Sale') OR legacy_type IN ('Sale', 'Sales')) ORDER BY id DESC LIMIT 1;"
        );
    }
    if (vId.isValid()) {
        return get_sales_invoice(vId.toInt());
    }
    return {};
}

QVariantMap SalesModel::get_next_sales_invoice(int currentId, const QString& currentInvOrVchNo) {
    if (currentId <= 0 && currentInvOrVchNo.trimmed().isEmpty()) return {};

    int effId = currentId;
    if (effId <= 0 && !currentInvOrVchNo.trimmed().isEmpty()) {
        QVariant curRowId = DatabaseManager::instance().executeScalar(
            "SELECT id FROM sales_invoices WHERE invoice_no = ? OR voucher_no = ? LIMIT 1;",
            {currentInvOrVchNo, currentInvOrVchNo}
        );
        if (curRowId.isValid()) effId = curRowId.toInt();
    }

    if (effId > 0) {
        QVariant targetId = DatabaseManager::instance().executeScalar(
            "SELECT id FROM sales_invoices WHERE id > ? ORDER BY id ASC LIMIT 1;", {effId}
        );
        if (targetId.isValid()) {
            return get_sales_invoice(targetId.toInt());
        }

        // Fallback to vouchers table
        QVariant vId = DatabaseManager::instance().executeScalar(
            "SELECT id FROM vouchers WHERE id > ? AND (voucher_type IN ('Sales', 'Sale') OR legacy_type IN ('Sale', 'Sales')) ORDER BY id ASC LIMIT 1;",
            {effId}
        );
        if (vId.isValid()) {
            return get_sales_invoice(vId.toInt());
        }
    }
    return {};
}

QVariantMap SalesModel::get_sales_invoice(const QVariant& invoiceNoOrId, const QString& dateHintParam, const QString& partyHintParam) {
    int targetId = 0;
    QString q;
    QString vNo;
    QString dateHint = FiscalYearHelper::normalizeToIso(dateHintParam);
    QString partyHint = partyHintParam.trimmed();

    if (invoiceNoOrId.typeId() == QMetaType::QVariantMap) {
        QVariantMap m = invoiceNoOrId.toMap();
        targetId = m.value("id").toInt();
        q = m.value("invoice_no", m.value("invoiceNo", m.value("refNo", m.value("voucher_no", m.value("voucherNo"))))).toString().trimmed();
        vNo = m.value("voucher_no", m.value("voucherNo")).toString().trimmed();
        if (dateHint.isEmpty()) dateHint = FiscalYearHelper::normalizeToIso(m.value("vIso", m.value("invoice_date", m.value("voucher_date", m.value("date")))).toString());
        if (partyHint.isEmpty()) partyHint = m.value("party_name", m.value("partyName", m.value("customer_name", m.value("customerName")))).toString().trimmed();
    } else {
        bool isNum = false;
        int parsedId = invoiceNoOrId.toInt(&isNum);
        if (isNum && parsedId > 0 && dateHint.isEmpty()) {
            QVariantList chk = DatabaseManager::instance().executeQuery("SELECT id FROM sales_invoices WHERE id = ? LIMIT 1;", {parsedId});
            if (!chk.isEmpty()) {
                targetId = parsedId;
            } else {
                q = QString::number(parsedId);
            }
        } else {
            q = invoiceNoOrId.toString().trimmed();
        }
    }

    qDebug() << "[SALES_MODEL] get_sales_invoice targetId:" << targetId << "q:" << q << "vNo:" << vNo << "dateHint:" << dateHint << "partyHint:" << partyHint;
    if (targetId <= 0 && q.isEmpty() && vNo.isEmpty() && dateHint.isEmpty() && partyHint.isEmpty()) return {};

    static const QRegularExpression prefixRe(QStringLiteral("^(Sale|Sales|Purc|Purchase|Pur|Jrnl|Journal|ChPt|ChRt|Pymt|Rcpt|TDS|JFrm|J-Form)[-\\s]*"), QRegularExpression::CaseInsensitiveOption);
    QString cleanQ = q;
    cleanQ = cleanQ.remove(prefixRe).trimmed();
    QString cleanVNo = vNo;
    cleanVNo = cleanVNo.remove(prefixRe).trimmed();
    if (cleanQ.isEmpty()) cleanQ = cleanVNo;
    if (cleanVNo.isEmpty()) cleanVNo = cleanQ;

    QVariantList rows;

    // 1. Direct search by ID if valid in sales_invoices
    if (targetId > 0) {
        rows = DatabaseManager::instance().executeQuery("SELECT * FROM sales_invoices WHERE id = ? LIMIT 1;", {targetId});
    }

    // 2. Match with dateHint and invoice_no / voucher_no
    if (rows.isEmpty() && !dateHint.isEmpty() && (!cleanQ.isEmpty() || !cleanVNo.isEmpty())) {
        rows = DatabaseManager::instance().executeQuery(
            "SELECT * FROM sales_invoices WHERE (invoice_no = ? OR voucher_no = ? OR invoice_no = ? OR voucher_no = ? OR invoice_no = ? OR voucher_no = ?) AND invoice_date = ? ORDER BY id DESC LIMIT 1;",
            {q, vNo, cleanQ, cleanVNo, ("Sale-" + cleanVNo), ("Sale-" + cleanQ), dateHint}
        );
    }

    // 3. Match with dateHint and partyHint
    if (rows.isEmpty() && !dateHint.isEmpty() && !partyHint.isEmpty()) {
        rows = DatabaseManager::instance().executeQuery(
            "SELECT * FROM sales_invoices WHERE invoice_date = ? AND (customer_name = ? OR customer_name LIKE ?) ORDER BY id DESC LIMIT 1;",
            {dateHint, partyHint, "%" + partyHint + "%"}
        );
    }

    // 4. Exact invoice_no or voucher_no match in sales_invoices
    if (rows.isEmpty() && (!cleanQ.isEmpty() || !cleanVNo.isEmpty())) {
        rows = DatabaseManager::instance().executeQuery(
            "SELECT * FROM sales_invoices WHERE invoice_no = ? OR voucher_no = ? OR invoice_no = ? OR voucher_no = ? OR voucher_no = ('Sale-' || ?) OR voucher_no = ('Sale-' || ?) ORDER BY id DESC LIMIT 1;",
            {q, vNo, cleanQ, cleanVNo, q, cleanVNo}
        );
    }

    // 3. Try lookup via vouchers table (if q/cleanQ is in vouchers table)
    if (rows.isEmpty() && (!q.isEmpty() || !cleanQ.isEmpty())) {
        QString sql = "SELECT id, voucher_no, instrument_no, voucher_date, party_name, amount, narration "
                      "FROM vouchers WHERE (id = ? OR voucher_no = ? OR instrument_no = ? OR voucher_no = ? OR instrument_no = ? OR voucher_no = ('Sale-' || ?) OR instrument_no = ('Sale-' || ?)) "
                      "AND (voucher_type IN ('Sales', 'Sale') OR legacy_type IN ('Sale', 'Sales')) ";
        QVariantList vParams = {q, q, q, cleanQ, cleanQ, q, q};
        if (!dateHint.isEmpty()) {
            sql += "AND voucher_date = ? ";
            vParams.append(dateHint);
        }
        sql += "ORDER BY id DESC LIMIT 1;";
        QVariantList vRows = DatabaseManager::instance().executeQuery(sql, vParams);
        if (vRows.isEmpty() && !dateHint.isEmpty()) {
            // Retry vouchers without strict dateHint
            vRows = DatabaseManager::instance().executeQuery(
                "SELECT id, voucher_no, instrument_no, voucher_date, party_name, amount, narration "
                "FROM vouchers WHERE (id = ? OR voucher_no = ? OR instrument_no = ? OR voucher_no = ? OR instrument_no = ? OR voucher_no = ('Sale-' || ?) OR instrument_no = ('Sale-' || ?)) "
                "AND (voucher_type IN ('Sales', 'Sale') OR legacy_type IN ('Sale', 'Sales')) ORDER BY id DESC LIMIT 1;",
                {q, q, q, cleanQ, cleanQ, q, q}
            );
        }

        if (!vRows.isEmpty()) {
            QVariantMap v = vRows.first().toMap();
            QString vRef = v.value("instrument_no").toString().trimmed();
            QString vNo = v.value("voucher_no").toString().trimmed();
            QString vDate = v.value("voucher_date").toString().trimmed();
            QString vParty = v.value("party_name").toString().trimmed();

            if (!vRef.isEmpty()) {
                rows = DatabaseManager::instance().executeQuery(
                    "SELECT * FROM sales_invoices WHERE invoice_no = ? ORDER BY id DESC LIMIT 1;", {vRef}
                );
            }
            if (rows.isEmpty() && !vNo.isEmpty() && !vParty.isEmpty()) {
                rows = DatabaseManager::instance().executeQuery(
                    "SELECT * FROM sales_invoices WHERE (voucher_no = ? OR invoice_no = ?) AND customer_name LIKE ? ORDER BY id DESC LIMIT 1;",
                    {vNo, vNo, "%" + vParty + "%"}
                );
            }
            if (rows.isEmpty() && !vNo.isEmpty()) {
                rows = DatabaseManager::instance().executeQuery(
                    "SELECT * FROM sales_invoices WHERE voucher_no = ? OR invoice_no = ? ORDER BY id DESC LIMIT 1;", {vNo, vNo}
                );
            }
            if (rows.isEmpty() && !vDate.isEmpty() && !vParty.isEmpty()) {
                rows = DatabaseManager::instance().executeQuery(
                    "SELECT * FROM sales_invoices WHERE invoice_date = ? AND customer_name = ? ORDER BY id DESC LIMIT 1;",
                    {vDate, vParty}
                );
            }
            if (rows.isEmpty()) {
                // Construct synthetic sales invoice directly from vouchers and stock_transactions
                QVariantMap synInv;
                synInv["id"] = v.value("id");
                synInv["voucher_no"] = vNo.isEmpty() ? ("Sale-" + v.value("id").toString()) : vNo;
                synInv["invoice_no"] = vRef.isEmpty() ? synInv["voucher_no"] : vRef;
                synInv["invoice_date"] = vDate;
                synInv["customer_name"] = vParty;
                synInv["party_ledger"] = vParty;
                synInv["total_amount"] = v.value("amount");
                synInv["taxable_amount"] = v.value("amount");
                synInv["narration"] = v.value("narration");

                QVariantList pRows = DatabaseManager::instance().executeQuery(
                    "SELECT gstin, address FROM parties WHERE name = ? LIMIT 1;", {vParty}
                );
                if (!pRows.isEmpty()) {
                    synInv["gstin"] = pRows.first().toMap().value("gstin");
                    synInv["shipping_address"] = pRows.first().toMap().value("address");
                }

                QVariantList stRows = DatabaseManager::instance().executeQuery(
                    "SELECT item_name, bags AS bag_count, packing, weight_qtl, rate AS rate_per_qtl, amount AS total_amount, taxable_amount, tax AS gst_pct "
                    "FROM stock_transactions WHERE (voucher_no = ? OR voucher_no = ? OR bill_no = ? OR (voucher_date = ? AND party_name = ?)) AND trans_type IN ('Sale', 'Sales') "
                    "ORDER BY row_no ASC, id ASC;",
                    {vNo, cleanQ, vRef, vDate, vParty}
                );
                synInv["items"] = stRows;
                return synInv;
            }
        }
    }

    // 3.5. Try lookup via transactions table
    if (rows.isEmpty() && (!q.isEmpty() || !cleanQ.isEmpty())) {
        QString sql = "SELECT id, voucher_no, invoice_no, voucher_date, party_name, amount, narration "
                      "FROM transactions WHERE (id = ? OR voucher_no = ? OR invoice_no = ? OR voucher_no = ? OR invoice_no = ? OR voucher_no = ('Sale-' || ?) OR invoice_no = ('Sale-' || ?)) "
                      "AND (trans_type IN ('Sale', 'Sales') OR voucher_type IN ('Sales', 'Sale')) ";
        QVariantList tParams = {q, q, q, cleanQ, cleanQ, q, q};
        if (!dateHint.isEmpty()) {
            sql += "AND voucher_date = ? ";
            tParams.append(dateHint);
        }
        sql += "ORDER BY id DESC LIMIT 1;";
        QVariantList tRows = DatabaseManager::instance().executeQuery(sql, tParams);
        if (tRows.isEmpty() && !dateHint.isEmpty()) {
            tRows = DatabaseManager::instance().executeQuery(
                "SELECT id, voucher_no, invoice_no, voucher_date, party_name, amount, narration "
                "FROM transactions WHERE (id = ? OR voucher_no = ? OR invoice_no = ? OR voucher_no = ? OR invoice_no = ? OR voucher_no = ('Sale-' || ?) OR invoice_no = ('Sale-' || ?)) "
                "AND (trans_type IN ('Sale', 'Sales') OR voucher_type IN ('Sales', 'Sale')) ORDER BY id DESC LIMIT 1;",
                {q, q, q, cleanQ, cleanQ, q, q}
            );
        }

        if (!tRows.isEmpty()) {
            QVariantMap t = tRows.first().toMap();
            QString tInv = t.value("invoice_no").toString().trimmed();
            QString tVNo = t.value("voucher_no").toString().trimmed();
            QString tDate = t.value("voucher_date").toString().trimmed();
            QString tParty = t.value("party_name").toString().trimmed();

            if (!tInv.isEmpty()) {
                rows = DatabaseManager::instance().executeQuery(
                    "SELECT * FROM sales_invoices WHERE invoice_no = ? ORDER BY id DESC LIMIT 1;", {tInv}
                );
            }
            if (rows.isEmpty() && !tVNo.isEmpty()) {
                rows = DatabaseManager::instance().executeQuery(
                    "SELECT * FROM sales_invoices WHERE voucher_no = ? OR voucher_no = ('Sale-' || ?) OR invoice_no = ? ORDER BY id DESC LIMIT 1;",
                    {tVNo, tVNo, tVNo}
                );
            }
            if (rows.isEmpty() && !tDate.isEmpty() && !tParty.isEmpty()) {
                rows = DatabaseManager::instance().executeQuery(
                    "SELECT * FROM sales_invoices WHERE invoice_date = ? AND customer_name LIKE ? ORDER BY id DESC LIMIT 1;",
                    {tDate, "%" + tParty + "%"}
                );
            }
            if (rows.isEmpty()) {
                // Construct synthetic sales invoice directly from transactions and stock_transactions
                QVariantMap synInv;
                synInv["id"] = t.value("id");
                synInv["voucher_no"] = tVNo.isEmpty() ? ("Sale-" + t.value("id").toString()) : tVNo;
                synInv["invoice_no"] = tInv.isEmpty() ? synInv["voucher_no"] : tInv;
                synInv["invoice_date"] = tDate;
                synInv["customer_name"] = tParty;
                synInv["party_ledger"] = tParty;
                synInv["total_amount"] = t.value("amount");
                synInv["taxable_amount"] = t.value("amount");
                synInv["narration"] = t.value("narration");

                QVariantList pRows = DatabaseManager::instance().executeQuery(
                    "SELECT gstin, address FROM parties WHERE name = ? LIMIT 1;", {tParty}
                );
                if (!pRows.isEmpty()) {
                    synInv["gstin"] = pRows.first().toMap().value("gstin");
                    synInv["shipping_address"] = pRows.first().toMap().value("address");
                }

                QVariantList stRows = DatabaseManager::instance().executeQuery(
                    "SELECT item_name, bags AS bag_count, packing, weight_qtl, rate AS rate_per_qtl, amount AS total_amount, taxable_amount, tax AS gst_pct "
                    "FROM stock_transactions WHERE (voucher_no = ? OR voucher_no = ? OR bill_no = ? OR (voucher_date = ? AND party_name = ?)) AND trans_type IN ('Sale', 'Sales') "
                    "ORDER BY row_no ASC, id ASC;",
                    {tVNo, cleanQ, tInv, tDate, tParty}
                );
                synInv["items"] = stRows;
                return synInv;
            }
        }
    }

    // 4. Try matching by dateHint and partyHint
    if (rows.isEmpty() && !dateHint.isEmpty() && !partyHint.isEmpty()) {
        rows = DatabaseManager::instance().executeQuery(
            "SELECT * FROM sales_invoices WHERE invoice_date = ? AND customer_name LIKE ? ORDER BY id DESC LIMIT 1;",
            {dateHint, "%" + partyHint + "%"}
        );
    }

    // 5. Loose substring match fallback
    if (rows.isEmpty() && !cleanQ.isEmpty()) {
        rows = DatabaseManager::instance().executeQuery(
            "SELECT * FROM sales_invoices WHERE invoice_no LIKE ? OR voucher_no LIKE ? ORDER BY id DESC LIMIT 1;",
            {"%" + cleanQ + "%", "%" + cleanQ + "%"}
        );
    }

    if (rows.isEmpty()) return {};
    QVariantMap inv = rows.first().toMap();
    int invId = inv.value("id").toInt();
    QString invNo = inv.value("invoice_no").toString().trimmed();
    vNo = inv.value("voucher_no").toString().trimmed();

    // Auto-resolve party metadata if gstin or address is missing
    QString custName = inv.value("customer_name").toString().trimmed();
    inv["party_ledger"] = custName;
    if (!custName.isEmpty()) {
        QVariantList pRows = DatabaseManager::instance().executeQuery(
            "SELECT gstin, address, city, state, phone FROM parties WHERE name = ? COLLATE NOCASE OR alias = ? COLLATE NOCASE LIMIT 1;",
            {custName, custName}
        );
        if (pRows.isEmpty() && custName.contains('[')) {
            QString cleanName = custName.left(custName.indexOf('[')).trimmed();
            pRows = DatabaseManager::instance().executeQuery(
                "SELECT gstin, address, city, state, phone FROM parties WHERE name LIKE ? LIMIT 1;",
                {cleanName + "%"}
            );
        }
        if (!pRows.isEmpty()) {
            QVariantMap pMap = pRows.first().toMap();
            if (inv.value("gstin").toString().trimmed().isEmpty()) {
                inv["gstin"] = pMap.value("gstin");
            }
            inv["party_address"] = pMap.value("address");
            inv["party_city"] = pMap.value("city");
            inv["party_state"] = pMap.value("state");
            inv["party_phone"] = pMap.value("phone");
        }
    }

    // Fetch line items from sales_invoice_items by invoice_id or invoice_no
    QVariantList itemRows = DatabaseManager::instance().executeQuery(
        "SELECT * FROM sales_invoice_items WHERE invoice_id = ? ORDER BY id ASC;",
        {invId}
    );
    if (itemRows.isEmpty() && !invNo.isEmpty()) {
        itemRows = DatabaseManager::instance().executeQuery(
            "SELECT * FROM sales_invoice_items WHERE invoice_no = ? ORDER BY id ASC;",
            {invNo}
        );
    }
    if (itemRows.isEmpty() && (!vNo.isEmpty() || !invNo.isEmpty())) {
        QVariantList stRows = DatabaseManager::instance().executeQuery(
            "SELECT item_id, item_name, dheri_bill_no AS grade, bags AS bag_count, packing, weight_qtl, rate AS rate_per_qtl, "
            "amount AS total_amount, taxable_amount, tax AS gst_pct "
            "FROM stock_transactions WHERE (voucher_no = ? OR bill_no = ?) AND trans_type IN ('Sale', 'Sales') "
            "ORDER BY row_no ASC, id ASC;",
            {vNo, invNo}
        );
        if (!stRows.isEmpty()) {
            itemRows = stRows;
        }
    }

    if (itemRows.isEmpty() && !inv.value("item_name").toString().isEmpty()) {
        QVariantMap itm;
        itm["itemName"] = inv.value("item_name");
        itm["item_name"] = inv.value("item_name");
        itm["grade"] = inv.value("grade");
        itm["bags"] = inv.value("bag_count").toInt();
        itm["bag_count"] = itm["bags"];
        itm["packing"] = 0.5;
        itm["weight"] = inv.value("weight_qtl").toDouble();
        itm["weight_qtl"] = itm["weight"];
        itm["rate"] = inv.value("rate_per_qtl").toDouble();
        itm["rate_per_qtl"] = itm["rate"];
        itm["gstPct"] = inv.value("gst_pct").toDouble();
        itm["gst_pct"] = itm["gstPct"];
        double amt = inv.value("taxable_amount").toDouble() > 0 ? inv.value("taxable_amount").toDouble() : inv.value("total_amount").toDouble();
        if (amt <= 0.001) amt = itm["weight"].toDouble() * itm["rate"].toDouble();
        itm["amount"] = amt;
        itm["taxable_amount"] = amt;
        itm["total_amount"] = inv.value("total_amount").toDouble() > 0 ? inv.value("total_amount").toDouble() : amt;
        itm["hsn_code"] = inv.value("hsn_code");
        itemRows.append(itm);
    } else {
        for (int i = 0; i < itemRows.size(); ++i) {
            QVariantMap itm = itemRows[i].toMap();
            QString iName = itm.contains("item_name") ? itm.value("item_name").toString() : itm.value("itemName").toString();
            QString gradeVal = itm.value("grade").toString().trimmed();
            if (gradeVal.isEmpty()) gradeVal = inv.value("grade").toString().trimmed();
            int bCount = itm.value("bag_count").toInt();
            if (bCount == 0) bCount = itm.value("bags").toInt();
            double pVal = itm.value("packing").toDouble();
            if (pVal <= 0.0001) pVal = 0.5;
            double wVal = itm.value("weight_qtl").toDouble();
            if (wVal <= 0.0001) wVal = itm.value("weight").toDouble();
            double rVal = itm.value("rate_per_qtl").toDouble();
            if (rVal <= 0.0001) rVal = itm.value("rate").toDouble();
            double gVal = itm.value("gst_pct").toDouble();
            if (gVal <= 0.0001) gVal = itm.value("gstPct").toDouble();
            double amt = itm.value("total_amount").toDouble();
            if (amt <= 0.0001) amt = itm.value("taxable_amount").toDouble();
            if (amt <= 0.0001) amt = itm.value("amount").toDouble();
            if (amt <= 0.001 && wVal > 0 && rVal > 0) amt = wVal * rVal;

            itm["itemName"] = iName;
            itm["item_name"] = iName;
            itm["grade"] = gradeVal;
            itm["bags"] = bCount;
            itm["bag_count"] = bCount;
            itm["packing"] = pVal;
            itm["weight"] = wVal;
            itm["weight_qtl"] = wVal;
            itm["rate"] = rVal;
            itm["rate_per_qtl"] = rVal;
            itm["gstPct"] = gVal;
            itm["gst_pct"] = gVal;
            itm["amount"] = amt;
            itm["taxable_amount"] = amt;
            itm["total_amount"] = amt;
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
    const QVariantList& items,
    const QString& market_type,
    int due_days,
    const QString& challan_no,
    double freight_charges,
    double tcs_amount,
    double tcs_rate,
    const QString& tax_status,
    const QString& place_of_supply
) {
    QString dt = invoice_date.isEmpty() ? QDate::currentDate().toString("yyyy-MM-dd") : invoice_date;
    FiscalYearInfo fy = FiscalYearHelper::getFiscalYearForDate(dt);
    int fyId = 1;
    QString fyLabel = fy.name;
    QVariant fyIdVar = DatabaseManager::instance().executeScalar(
        "SELECT id FROM financial_years WHERE year_name = ? LIMIT 1;",
        {fy.name}
    );
    if (fyIdVar.isValid() && !fyIdVar.isNull()) {
        fyId = fyIdVar.toInt();
    } else {
        QVariant anyFy = DatabaseManager::instance().executeScalar("SELECT id FROM financial_years WHERE is_active = 1 LIMIT 1;");
        if (anyFy.isValid() && !anyFy.isNull()) fyId = anyFy.toInt();
    }

    QVariant itemRow = DatabaseManager::instance().executeScalar(
        "SELECT id FROM stock_items WHERE name = ? COLLATE NOCASE LIMIT 1;", {item_name}
    );
    int itemId = itemRow.isValid() ? itemRow.toInt() : 1;

    QVariant custRow = DatabaseManager::instance().executeScalar(
        "SELECT id FROM parties WHERE name = ? COLLATE NOCASE LIMIT 1;", {party_ledger}
    );
    int customerId = custRow.isValid() ? custRow.toInt() : 1;
    double gst_amount = cgst_amount + sgst_amount + igst_amount;

    DatabaseManager::instance().beginTransaction();

    // Check if sales_invoices row exists by id, invoice_no, or voucher_no
    QVariant existingId = DatabaseManager::instance().executeScalar(
        "SELECT id FROM sales_invoices WHERE id = ? OR (invoice_no = ? AND invoice_no != '') OR (voucher_no = ? AND voucher_no != '') LIMIT 1;",
        {invoice_id, invoice_no, voucher_no}
    );

    int targetInvId = existingId.isValid() ? existingId.toInt() : invoice_id;

    if (existingId.isValid()) {
        DatabaseManager::instance().executeNonQuery(
            "UPDATE sales_invoices SET "
            "voucher_no = ?, invoice_no = ?, invoice_date = ?, customer_id = ?, customer_name = ?, gstin = ?, "
            "item_id = ?, item_name = ?, hsn_code = ?, bag_count = ?, weight_qtl = ?, rate_per_qtl = ?, "
            "taxable_amount = ?, gst_pct = ?, cgst_amount = ?, sgst_amount = ?, igst_amount = ?, round_off = ?, "
            "gst_amount = ?, total_amount = ?, payment_mode = ?, vehicle_no = ?, eway_bill_no = ?, narration = ?, "
            "sale_status = ?, market_fee_status = ?, dami = ?, labour = ?, auction = ?, m_fee = ?, hrdf = ?, "
            "other_exp = ?, welfare = ?, dhrmd = ?, sutli = ?, less_amount = ?, gr_no = ?, driver = ?, "
            "bill_time = ?, sauda_date = ?, shipping_address = ?, po_no = ?, grade = ?, kanda_weight = ?, "
            "transport = ?, broker_name = ?, market_type = ?, due_days = ?, tax_status = ?, challan_no = ?, "
            "freight_charges = ?, tcs_amount = ?, tcs_rate = ?, place_of_supply = ?, financial_year = ?, fy_id = ? "
            "WHERE id = ?;",
            {
                voucher_no, invoice_no, dt, customerId, party_ledger, gstin,
                itemId, item_name, hsn_code, bag_count, weight_qtl, rate_per_qtl,
                taxable_amount, gst_pct, cgst_amount, sgst_amount, igst_amount, round_off,
                gst_amount, total_amount, payment_mode, vehicle_no, eway_bill_no, narration,
                sale_status, market_fee_status, dami, labour, auction, m_fee, hrdf,
                other_exp, welfare, dhrmd, sutli, less_amount, gr_no, driver,
                bill_time, sauda_date, shipping_address, po_no, grade, kanda_weight,
                transport, broker_name, market_type, due_days, tax_status, challan_no,
                freight_charges, tcs_amount, tcs_rate, place_of_supply, fyLabel, fyId,
                targetInvId
            }
        );
    } else {
        DatabaseManager::instance().executeNonQuery(
            "INSERT INTO sales_invoices ("
            "fy_id, financial_year, voucher_no, invoice_no, invoice_date, customer_id, customer_name, gstin, item_id, item_name, hsn_code, "
            "bag_count, weight_qtl, rate_per_qtl, taxable_amount, gst_pct, cgst_amount, sgst_amount, igst_amount, round_off, gst_amount, "
            "total_amount, payment_mode, vehicle_no, eway_bill_no, narration, sale_status, market_fee_status, dami, labour, auction, "
            "m_fee, hrdf, other_exp, welfare, dhrmd, sutli, less_amount, gr_no, driver, bill_time, sauda_date, shipping_address, "
            "po_no, grade, kanda_weight, transport, broker_name, market_type, due_days, tax_status, challan_no, freight_charges, "
            "tcs_amount, tcs_rate, place_of_supply"
            ") VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);",
            {
                fyId, fyLabel, voucher_no, invoice_no, dt, customerId, party_ledger, gstin, itemId, item_name, hsn_code,
                bag_count, weight_qtl, rate_per_qtl, taxable_amount, gst_pct, cgst_amount, sgst_amount, igst_amount, round_off, gst_amount,
                total_amount, payment_mode, vehicle_no, eway_bill_no, narration, sale_status, market_fee_status,
                dami, labour, auction, m_fee, hrdf, other_exp, welfare, dhrmd, sutli, less_amount,
                gr_no, driver, bill_time, sauda_date, shipping_address, po_no, grade, kanda_weight, transport, broker_name,
                market_type, due_days, tax_status, challan_no, freight_charges, tcs_amount, tcs_rate, place_of_supply
            }
        );
        targetInvId = static_cast<int>(DatabaseManager::instance().lastInsertedId());
    }

    // Replace line items in sales_invoice_items
    DatabaseManager::instance().executeNonQuery(
        "DELETE FROM sales_invoice_items WHERE invoice_id = ? OR invoice_no = ?;",
        {targetInvId, invoice_no}
    );
    if (!items.isEmpty()) {
        for (const QVariant& itmV : items) {
            QVariantMap itm = itmV.toMap();
            DatabaseManager::instance().executeNonQuery(
                "INSERT INTO sales_invoice_items (invoice_id, invoice_no, item_id, item_name, grade, bag_count, packing, weight_qtl, rate_per_qtl, taxable_amount, gst_pct, total_amount) "
                "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);",
                {
                    targetInvId, invoice_no, itemId, itm.value("item_name").toString(),
                    itm.value("grade").toString(),
                    itm.value("bags").toInt(), itm.value("packing").toDouble(),
                    itm.value("weight").toDouble(), itm.value("rate").toDouble(),
                    itm.value("amount").toDouble(), itm.value("gst_pct").toDouble(),
                    itm.value("amount").toDouble()
                }
            );
        }
    }

    // Update or insert into vouchers table for financial reports
    QVariant vId = DatabaseManager::instance().executeScalar(
        "SELECT id FROM vouchers WHERE (instrument_no = ? OR voucher_no = ? OR id = ?) AND (voucher_type IN ('Sales', 'Sale') OR legacy_type IN ('Sale', 'Sales')) LIMIT 1;",
        {invoice_no, voucher_no, targetInvId}
    );

    if (vId.isValid()) {
        DatabaseManager::instance().executeNonQuery(
            "UPDATE vouchers SET "
            "voucher_no = ?, instrument_no = ?, voucher_date = ?, party_id = ?, party_name = ?, "
            "amount = ?, taxable_amount = ?, gst_pct = ?, cgst_amount = ?, sgst_amount = ?, igst_amount = ?, "
            "round_off = ?, vehicle_no = ?, eway_bill_no = ?, broker_name = ?, sauda_date = ?, "
            "dami = ?, labour = ?, auction = ?, m_fee = ?, hrdf = ?, other_exp = ?, welfare = ?, "
            "dhrmd = ?, sutli = ?, less_amount = ?, narration = ?, due_days = ?, market_type = ?, "
            "tax_status = ?, place_of_supply = ?, challan_no = ?, financial_year = ?, fy_id = ? "
            "WHERE id = ?;",
            {
                voucher_no, invoice_no, dt, customerId, party_ledger,
                total_amount, taxable_amount, gst_pct, cgst_amount, sgst_amount, igst_amount,
                round_off, vehicle_no, eway_bill_no, broker_name, sauda_date,
                dami, labour, auction, m_fee, hrdf, other_exp, welfare,
                dhrmd, sutli, less_amount, narration, due_days, market_type,
                tax_status, place_of_supply, challan_no, fyLabel, fyId,
                vId.toInt()
            }
        );
    } else {
        DatabaseManager::instance().executeNonQuery(
            "INSERT INTO vouchers ("
            "fy_id, financial_year, voucher_no, instrument_no, voucher_date, voucher_type, legacy_type, party_id, party_name, "
            "amount, taxable_amount, gst_pct, cgst_amount, sgst_amount, igst_amount, round_off, vehicle_no, eway_bill_no, broker_name, sauda_date, "
            "dami, labour, auction, m_fee, hrdf, other_exp, welfare, dhrmd, sutli, less_amount, narration, due_days, market_type, tax_status, place_of_supply, challan_no"
            ") VALUES (?, ?, ?, ?, ?, 'Sales', 'Sale', ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);",
            {
                fyId, fyLabel, voucher_no, invoice_no, dt, customerId, party_ledger,
                total_amount, taxable_amount, gst_pct, cgst_amount, sgst_amount, igst_amount,
                round_off, vehicle_no, eway_bill_no, broker_name, sauda_date,
                dami, labour, auction, m_fee, hrdf, other_exp, welfare,
                dhrmd, sutli, less_amount, narration, due_days, market_type,
                tax_status, place_of_supply, challan_no
            }
        );
    }

    // Delete and re-insert stock_transactions
    DatabaseManager::instance().executeNonQuery(
        "DELETE FROM stock_transactions WHERE (bill_no = ? OR voucher_no = ?) AND trans_type IN ('Sale', 'Sales', 'S');",
        {invoice_no, voucher_no}
    );

    QString vchNarr = narration.trimmed();
    if (!items.isEmpty()) {
        int rIdx = 1;
        for (const QVariant& itmV : items) {
            QVariantMap itm = itmV.toMap();
            DatabaseManager::instance().executeNonQuery(
                "INSERT INTO stock_transactions ("
                "fy_id, financial_year, voucher_no, voucher_date, trans_type, voucher_type, party_id, party_name, bill_no, "
                "item_id, item_code, item_name, bags, weight_qtl, rate, amount, taxable_amount, narration, row_no"
                ") VALUES (?, ?, ?, ?, 'Sale', 'Sales', ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);",
                {
                    fyId, fyLabel, voucher_no, dt, customerId, party_ledger, invoice_no,
                    itemId, hsn_code, itm.value("item_name").toString(),
                    itm.value("bags").toInt(), itm.value("weight").toDouble(), itm.value("rate").toDouble(),
                    itm.value("amount").toDouble(), itm.value("amount").toDouble(), vchNarr, rIdx++
                }
            );
        }
    } else {
        DatabaseManager::instance().executeNonQuery(
            "INSERT INTO stock_transactions ("
            "fy_id, financial_year, voucher_no, voucher_date, trans_type, voucher_type, party_id, party_name, bill_no, "
            "item_id, item_code, item_name, bags, weight_qtl, rate, amount, taxable_amount, narration, row_no"
            ") VALUES (?, ?, ?, ?, 'Sale', 'Sales', ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, 1);",
            {
                fyId, fyLabel, voucher_no, dt, customerId, party_ledger, invoice_no,
                itemId, hsn_code, item_name, bag_count, weight_qtl, rate_per_qtl, total_amount, taxable_amount, vchNarr
            }
        );
    }

    // Delete and re-insert transactions double-entry splits
    DatabaseManager::instance().executeNonQuery(
        "DELETE FROM transactions WHERE (voucher_no = ? OR invoice_no = ?) AND voucher_type IN ('Sales', 'Sale');",
        {voucher_no, invoice_no}
    );

    QString itemSaleLedger = "";
    QVariantList itemMeta = DatabaseManager::instance().executeQuery(
        "SELECT sale_ledger, sale_ledger_id FROM stock_items WHERE id = ? LIMIT 1;",
        {itemId}
    );
    if (!itemMeta.isEmpty()) {
        itemSaleLedger = itemMeta.first().toMap().value("sale_ledger").toString().trimmed();
    }
    QString effectiveSaleLedger = itemSaleLedger.isEmpty() ? "Sale A/c" : itemSaleLedger;

    auto resolveLedgerCode = [](const QString& name, int fallbackId) -> int {
        if (name.trimmed().isEmpty()) return fallbackId;
        QVariantList rows = DatabaseManager::instance().executeQuery(
            "SELECT id, legacy_id, account_code FROM parties WHERE replace(name, ' ', '') = replace(?, ' ', '') LIMIT 1;",
            {name.trimmed()}
        );
        if (!rows.isEmpty()) {
            QVariantMap m = rows.first().toMap();
            int legId = m.value("legacy_id").toInt();
            if (legId > 0) return legId;
            int ac = m.value("account_code").toInt();
            if (ac > 0) return ac;
            return m.value("id").toInt();
        }
        return fallbackId;
    };

    int custCode = resolveLedgerCode(party_ledger, customerId);
    int saleCode = resolveLedgerCode(effectiveSaleLedger, 1239);
    int cgstCode = resolveLedgerCode("CGST A/c", resolveLedgerCode("CGST Output", 944));
    int sgstCode = resolveLedgerCode("SGST A/c", resolveLedgerCode("SGST Output", 943));
    int igstCode = resolveLedgerCode("IGST A/c", resolveLedgerCode("IGST Output", 945));
    int roundOffCode = resolveLedgerCode("Round Off", resolveLedgerCode("Round Off A/c", 44));

    // Leg 1: Debit Customer for total_amount (Row 1 is ALWAYS the Party in Bahi-Khata)
    DatabaseManager::instance().executeNonQuery(
        "INSERT INTO transactions ("
        "fy_id, financial_year, voucher_no, voucher_date, voucher_type, trans_type, "
        "account_code, party_id, party_name, opposing_account, dr_cr, amount, "
        "invoice_no, narration, taxable_amount, row_no"
        ") VALUES (?, ?, ?, ?, 'Sales', 'Sale', ?, ?, ?, ?, 'Dr', ?, ?, ?, ?, 1);",
        {
            fyId, fyLabel, voucher_no, dt,
            custCode, customerId, party_ledger, effectiveSaleLedger, total_amount,
            invoice_no, vchNarr, taxable_amount
        }
    );

    int tRowNo = 2;
    // Leg 2: Credit Sales Account for taxable_amount
    DatabaseManager::instance().executeNonQuery(
        "INSERT INTO transactions ("
        "fy_id, financial_year, voucher_no, voucher_date, voucher_type, trans_type, "
        "account_code, party_name, opposing_account, dr_cr, amount, "
        "invoice_no, narration, taxable_amount, row_no"
        ") VALUES (?, ?, ?, ?, 'Sales', 'Sale', ?, ?, ?, 'Cr', ?, ?, ?, ?, ?);",
        {
            fyId, fyLabel, voucher_no, dt,
            saleCode, effectiveSaleLedger, party_ledger, taxable_amount,
            invoice_no, vchNarr, taxable_amount, tRowNo++
        }
    );

    // Leg 3: Credit CGST Output if > 0
    if (cgst_amount > 0.001) {
        DatabaseManager::instance().executeNonQuery(
            "INSERT INTO transactions ("
            "fy_id, financial_year, voucher_no, voucher_date, voucher_type, trans_type, "
            "account_code, party_name, opposing_account, dr_cr, amount, "
            "invoice_no, narration, row_no"
            ") VALUES (?, ?, ?, ?, 'Sales', 'Sale', ?, 'CGST Output', ?, 'Cr', ?, ?, ?, ?);",
            {fyId, fyLabel, voucher_no, dt, cgstCode, party_ledger, cgst_amount, invoice_no, vchNarr, tRowNo++}
        );
    }
    // Leg 4: Credit SGST Output if > 0
    if (sgst_amount > 0.001) {
        DatabaseManager::instance().executeNonQuery(
            "INSERT INTO transactions ("
            "fy_id, financial_year, voucher_no, voucher_date, voucher_type, trans_type, "
            "account_code, party_name, opposing_account, dr_cr, amount, "
            "invoice_no, narration, row_no"
            ") VALUES (?, ?, ?, ?, 'Sales', 'Sale', ?, 'SGST Output', ?, 'Cr', ?, ?, ?, ?);",
            {fyId, fyLabel, voucher_no, dt, sgstCode, party_ledger, sgst_amount, invoice_no, vchNarr, tRowNo++}
        );
    }
    // Leg 5: Credit IGST Output if > 0
    if (igst_amount > 0.001) {
        DatabaseManager::instance().executeNonQuery(
            "INSERT INTO transactions ("
            "fy_id, financial_year, voucher_no, voucher_date, voucher_type, trans_type, "
            "account_code, party_name, opposing_account, dr_cr, amount, "
            "invoice_no, narration, row_no"
            ") VALUES (?, ?, ?, ?, 'Sales', 'Sale', ?, 'IGST Output', ?, 'Cr', ?, ?, ?, ?);",
            {fyId, fyLabel, voucher_no, dt, igstCode, party_ledger, igst_amount, invoice_no, vchNarr, tRowNo++}
        );
    }
    // Leg 6: Round Off if != 0
    if (std::abs(round_off) > 0.001) {
        QString roDrCr = (round_off > 0) ? "Cr" : "Dr";
        DatabaseManager::instance().executeNonQuery(
            "INSERT INTO transactions ("
            "fy_id, financial_year, voucher_no, voucher_date, voucher_type, trans_type, "
            "account_code, party_name, opposing_account, dr_cr, amount, "
            "invoice_no, narration, row_no"
            ") VALUES (?, ?, ?, ?, 'Sales', 'Sale', ?, 'Round Off', ?, ?, ?, ?, ?, ?);",
            {fyId, fyLabel, voucher_no, dt, roundOffCode, party_ledger, roDrCr, std::abs(round_off), invoice_no, vchNarr, tRowNo++}
        );
    }

    DatabaseManager::instance().commit();
    reload_data();
    return true;
}

bool SalesModel::delete_sales_invoice(int invoice_id, const QString& invoice_no) {
    if (invoice_id <= 0 && invoice_no.trimmed().isEmpty()) {
        return false;
    }

    QString invNo = invoice_no.trimmed();
    QString vNo;
    if (invoice_id > 0) {
        QVariantMap invRow = DatabaseManager::instance().executeQuery(
            "SELECT invoice_no, voucher_no FROM sales_invoices WHERE id = ? LIMIT 1;", {invoice_id}
        ).value(0).toMap();
        if (invNo.isEmpty()) invNo = invRow.value("invoice_no").toString();
        vNo = invRow.value("voucher_no").toString();
    }

    DatabaseManager::instance().beginTransaction();

    if (invoice_id > 0) {
        DatabaseManager::instance().executeNonQuery("DELETE FROM sales_invoice_items WHERE invoice_id = ?;", {invoice_id});
        DatabaseManager::instance().executeNonQuery("DELETE FROM sales_invoices WHERE id = ?;", {invoice_id});
    }

    if (!invNo.isEmpty()) {
        DatabaseManager::instance().executeNonQuery("DELETE FROM sales_invoice_items WHERE invoice_no = ?;", {invNo});
        DatabaseManager::instance().executeNonQuery("DELETE FROM sales_invoices WHERE invoice_no = ?;", {invNo});
        DatabaseManager::instance().executeNonQuery("DELETE FROM vouchers WHERE instrument_no = ? AND (voucher_type IN ('Sales', 'Sale') OR legacy_type IN ('Sale', 'Sales'));", {invNo});
        DatabaseManager::instance().executeNonQuery("DELETE FROM stock_transactions WHERE bill_no = ? AND trans_type IN ('Sale', 'Sales', 'S');", {invNo});
        DatabaseManager::instance().executeNonQuery("DELETE FROM transactions WHERE (invoice_no = ? OR voucher_no = ?) AND voucher_type IN ('Sales', 'Sale');", {invNo, invNo});
    }

    if (!vNo.isEmpty()) {
        DatabaseManager::instance().executeNonQuery("DELETE FROM vouchers WHERE voucher_no = ? AND (voucher_type IN ('Sales', 'Sale') OR legacy_type IN ('Sale', 'Sales'));", {vNo});
        DatabaseManager::instance().executeNonQuery("DELETE FROM stock_transactions WHERE voucher_no = ? AND trans_type IN ('Sale', 'Sales', 'S');", {vNo});
        DatabaseManager::instance().executeNonQuery("DELETE FROM transactions WHERE voucher_no = ? AND voucher_type IN ('Sales', 'Sale');", {vNo});
    }

    DatabaseManager::instance().commit();
    reload_data();
    return true;
}

