#include "purchase_model.h"
#include "../database_manager.h"
#include "../engine/accounting_engine.h"
#include "../engine/fiscal_year_helper.h"
#include <QDate>
#include <QRegularExpression>

PurchaseModel::PurchaseModel(QObject* parent)
    : BaseTableModel(
        {"Voucher No", "Bill No", "Date", "Supplier", "Item", "Bags", "Weight (Qtl)", "Rate (₹)", "Taxable (₹)", "GST %", "Total (₹)", "Mode"},
        {"voucher_no", "invoice_no", "invoice_date", "supplier_name", "item_name", "bag_count", "weight_qtl", "rate_per_qtl", "taxable_amount", "gst_pct", "total_amount", "payment_mode"},
        parent
    )
{
}

void PurchaseModel::reload_data() {
    beginResetModel();
    m_data = DatabaseManager::instance().executeQuery("SELECT * FROM purchase_invoices ORDER BY id DESC;");
    endResetModel();
    emit dataChangedSignal();
    emit countChanged();
}

QString PurchaseModel::get_next_voucher_no(const QString& fy) {
    QString targetFy = AccountingEngine::resolveFinancialYear(fy);
    QString fyPattern = "%" + targetFy.mid(3).trimmed() + "%";

    QVariantList piRows = DatabaseManager::instance().executeQuery(
        "SELECT voucher_no FROM purchase_invoices WHERE financial_year = ? OR financial_year LIKE ?;",
        {targetFy, fyPattern}
    );

    QVariantList vRows = DatabaseManager::instance().executeQuery(
        "SELECT voucher_no FROM vouchers WHERE (voucher_type = 'Purchase' OR voucher_no LIKE 'Purc-%' OR voucher_no LIKE '%/Purc-%') AND (financial_year = ? OR financial_year LIKE ?);",
        {targetFy, fyPattern}
    );

    long long maxId = 0;
    QRegularExpression re("(\\d+)$");
    for (const QVariantList* list : {&piRows, &vRows}) {
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
    QString next = FiscalYearHelper::canonicalVoucherNo(targetFy, "Purc", QString::number(maxId + 1));
    if (!next.contains('/')) next = "Purc-" + next; // degenerate fallback: no FY resolvable, keep legacy shape
    return next;
}

// NOTE: no get_next_invoice_no here by design. A purchase invoice number is
// the OPPOSING PARTY's bill number — typed in by the user, never generated.
// The widget requires it (see PurchaseVoucherWidget::saveVoucher) and the
// save path below stores it verbatim.

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
    QString invNo = invoice_no; // supplier's bill number, verbatim — never auto-generated
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

    // Resolve Supplier ID
    int supplierId = 1;
    QVariant suppRow = DatabaseManager::instance().executeScalar(
        "SELECT id FROM parties WHERE name = ? COLLATE NOCASE LIMIT 1;",
        {party_ledger}
    );
    if (!suppRow.isValid()) {
        suppRow = DatabaseManager::instance().executeScalar(
            "SELECT id FROM parties WHERE alias = ? COLLATE NOCASE OR name LIKE ? LIMIT 1;",
            {party_ledger, "%" + party_ledger + "%"}
        );
    }
    if (suppRow.isValid()) supplierId = suppRow.toInt();

    // --- ATOMIC ACID TRANSACTION ---
    DatabaseManager::instance().beginTransaction();

    // 1. Insert Purchase Invoice Record
    bool okInv = DatabaseManager::instance().executeNonQuery(
        "INSERT INTO purchase_invoices ("
        "fy_id, financial_year, voucher_no, invoice_no, invoice_date, supplier_id, supplier_name, gstin, item_id, item_name, hsn_code, "
        "bag_count, weight_qtl, rate_per_qtl, taxable_amount, gst_pct, cgst_amount, sgst_amount, igst_amount, round_off, gst_amount, "
        "total_amount, payment_mode, vehicle_no, eway_bill_no, narration, sale_status, market_fee_status, dami, labour, auction, "
        "m_fee, hrdf, other_exp, welfare, dhrmd, sutli, less_amount, gr_no, driver, bill_time, sauda_date, shipping_address, "
        "po_no, grade, kanda_weight, transport, broker_name, market_type, due_days, tax_status, challan_no, freight_charges, "
        "tcs_amount, tcs_rate, place_of_supply"
        ") VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);",
        {
            fyId, fyLabel, vchNo, invNo, dt, supplierId, party_ledger, gstin, itemId, item_name, hsn_code,
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
                "INSERT INTO purchase_invoice_items (invoice_id, invoice_no, item_id, item_name, grade, bag_count, packing, weight_qtl, rate_per_qtl, taxable_amount, gst_pct, total_amount) "
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
    // Debit: Stock Item's Specific Purchase Account for Taxable Amount + GST Input Accounts
    // Credit: Supplier Account for Total Amount
    QString itemPurchaseLedger = "";
    QVariantList itemMeta = DatabaseManager::instance().executeQuery(
        "SELECT purchase_ledger, purchase_ledger_id FROM stock_items WHERE id = ? LIMIT 1;",
        {itemId}
    );
    if (!itemMeta.isEmpty()) {
        itemPurchaseLedger = itemMeta.first().toMap().value("purchase_ledger").toString().trimmed();
    }

    QString vchNarr = QString("Purchase Bill %1 - %2 (%3 Qtl @ ₹%4)").arg(invNo, item_name, QString::number(weight_qtl), QString::number(rate_per_qtl));
    if (!narration.isEmpty()) vchNarr += " | " + narration;

    bool okVch = DatabaseManager::instance().executeNonQuery(
        "INSERT INTO vouchers ("
        "fy_id, financial_year, voucher_no, instrument_no, voucher_date, voucher_type, legacy_type, ledger_id, party_id, party_name, "
        "account_type, amount, taxable_amount, gst_pct, cgst_amount, sgst_amount, igst_amount, round_off, vehicle_no, eway_bill_no, "
        "broker_name, sauda_date, dami, labour, auction, m_fee, hrdf, other_exp, welfare, dhrmd, sutli, less_amount, narration, "
        "due_days, market_type, tax_status, place_of_supply, challan_no"
        ") VALUES (?, ?, ?, ?, ?, 'Purchase', 'Purc', ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);",
        {
            fyId, fyLabel, vchNo, invNo, dt, supplierId, supplierId, party_ledger,
            itemPurchaseLedger, total_amount, taxable_amount, gst_pct, cgst_amount, sgst_amount, igst_amount, round_off, vehicle_no, eway_bill_no,
            broker_name, sauda_date, dami, labour, auction, m_fee, hrdf, other_exp, welfare, dhrmd, sutli, less_amount, vchNarr,
            due_days, market_type, tax_status, place_of_supply, challan_no
        }
    );

    if (!okVch) {
        DatabaseManager::instance().rollback();
        return false;
    }

    // 3. Guaranteed Stock Transaction Inward Posting:
    bool okStock = DatabaseManager::instance().executeNonQuery(
        "INSERT INTO stock_transactions ("
        "fy_id, financial_year, voucher_no, voucher_date, trans_type, voucher_type, party_id, party_name, bill_no, "
        "item_id, item_code, item_name, bags, weight_qtl, rate, amount, taxable_amount, narration"
        ") VALUES (?, ?, ?, ?, 'Purc', 'Purchase Bill', ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);",
        {
            fyId, fyLabel, vchNo, dt, supplierId, party_ledger, invNo,
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
        "DELETE FROM transactions WHERE (voucher_no = ? OR invoice_no = ?) AND voucher_type IN ('Purchase', 'Purc');",
        {vchNo, invNo}
    );

    QString effectivePurcLedger = itemPurchaseLedger.isEmpty() ? "Purchase A/c" : itemPurchaseLedger;

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

    int supCode = resolveLedgerCode(party_ledger, supplierId);
    int purcCode = resolveLedgerCode(effectivePurcLedger, 1050);
    int cgstCode = resolveLedgerCode("CGST A/c", resolveLedgerCode("CGST Input", 944));
    int sgstCode = resolveLedgerCode("SGST A/c", resolveLedgerCode("SGST Input", 943));
    int igstCode = resolveLedgerCode("IGST A/c", resolveLedgerCode("IGST Input", 945));
    int roundOffCode = resolveLedgerCode("Round Off", resolveLedgerCode("Round Off A/c", 1177));

    // Leg 1: Credit Supplier for total_amount (Row 1 is ALWAYS the Party in Bahi-Khata)
    DatabaseManager::instance().executeNonQuery(
        "INSERT INTO transactions ("
        "fy_id, financial_year, voucher_no, voucher_date, voucher_type, trans_type, "
        "account_code, party_id, party_name, opposing_account, dr_cr, amount, "
        "invoice_no, narration, taxable_amount, row_no"
        ") VALUES (?, ?, ?, ?, 'Purchase', 'Purc', ?, ?, ?, ?, 'Cr', ?, ?, ?, ?, 1);",
        {
            fyId, fyLabel, vchNo, dt,
            supCode, supplierId, party_ledger, effectivePurcLedger, total_amount,
            invNo, vchNarr, taxable_amount
        }
    );

    int rNo = 2;
    // Leg 2: Debit Purchase Ledger for taxable_amount
    DatabaseManager::instance().executeNonQuery(
        "INSERT INTO transactions ("
        "fy_id, financial_year, voucher_no, voucher_date, voucher_type, trans_type, "
        "account_code, party_name, opposing_account, dr_cr, amount, "
        "invoice_no, narration, taxable_amount, row_no"
        ") VALUES (?, ?, ?, ?, 'Purchase', 'Purc', ?, ?, ?, 'Dr', ?, ?, ?, ?, ?);",
        {
            fyId, fyLabel, vchNo, dt,
            purcCode, effectivePurcLedger, party_ledger, taxable_amount,
            invNo, vchNarr, taxable_amount, rNo++
        }
    );

    // Leg 3: Debit CGST Input if > 0
    if (cgst_amount > 0.001) {
        DatabaseManager::instance().executeNonQuery(
            "INSERT INTO transactions ("
            "fy_id, financial_year, voucher_no, voucher_date, voucher_type, trans_type, "
            "account_code, party_name, opposing_account, dr_cr, amount, "
            "invoice_no, narration, row_no"
            ") VALUES (?, ?, ?, ?, 'Purchase', 'Purc', ?, 'CGST Input', ?, 'Dr', ?, ?, ?, ?);",
            {fyId, fyLabel, vchNo, dt, cgstCode, party_ledger, cgst_amount, invNo, vchNarr, rNo++}
        );
    }
    // Leg 4: Debit SGST Input if > 0
    if (sgst_amount > 0.001) {
        DatabaseManager::instance().executeNonQuery(
            "INSERT INTO transactions ("
            "fy_id, financial_year, voucher_no, voucher_date, voucher_type, trans_type, "
            "account_code, party_name, opposing_account, dr_cr, amount, "
            "invoice_no, narration, row_no"
            ") VALUES (?, ?, ?, ?, 'Purchase', 'Purc', ?, 'SGST Input', ?, 'Dr', ?, ?, ?, ?);",
            {fyId, fyLabel, vchNo, dt, sgstCode, party_ledger, sgst_amount, invNo, vchNarr, rNo++}
        );
    }
    // Leg 5: Debit IGST Input if > 0
    if (igst_amount > 0.001) {
        DatabaseManager::instance().executeNonQuery(
            "INSERT INTO transactions ("
            "fy_id, financial_year, voucher_no, voucher_date, voucher_type, trans_type, "
            "account_code, party_name, opposing_account, dr_cr, amount, "
            "invoice_no, narration, row_no"
            ") VALUES (?, ?, ?, ?, 'Purchase', 'Purc', ?, 'IGST Input', ?, 'Dr', ?, ?, ?, ?);",
            {fyId, fyLabel, vchNo, dt, igstCode, party_ledger, igst_amount, invNo, vchNarr, rNo++}
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
            ") VALUES (?, ?, ?, ?, 'Purchase', 'Purc', ?, 'Round Off', ?, ?, ?, ?, ?, ?);",
            {fyId, fyLabel, vchNo, dt, roundOffCode, party_ledger, roDrCr, std::abs(round_off), invNo, vchNarr, rNo++}
        );
    }

    // Commit Transaction (ACID Durability Guarantee)
    DatabaseManager::instance().commit();
    reload_data();
    return true;
}

QVariantList PurchaseModel::get_purchase_register(const QString& param1, const QString& param2) {
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
        sql = "SELECT id, voucher_no, invoice_no, invoice_date, supplier_name, item_name, bag_count, weight_qtl, rate_per_qtl, taxable_amount, gst_pct, cgst_amount, sgst_amount, igst_amount, gst_amount, round_off, total_amount, payment_mode, vehicle_no, eway_bill_no, financial_year, narration, market_type, due_days, tax_status, challan_no, freight_charges, tcs_amount, place_of_supply FROM purchase_invoices WHERE invoice_date >= ? AND invoice_date <= ? ORDER BY invoice_date DESC, id DESC;";
        params << fromDate << toDate;
    } else if (!fyLabel.isEmpty()) {
        sql = "SELECT id, voucher_no, invoice_no, invoice_date, supplier_name, item_name, bag_count, weight_qtl, rate_per_qtl, taxable_amount, gst_pct, cgst_amount, sgst_amount, igst_amount, gst_amount, round_off, total_amount, payment_mode, vehicle_no, eway_bill_no, financial_year, narration, market_type, due_days, tax_status, challan_no, freight_charges, tcs_amount, place_of_supply FROM purchase_invoices WHERE financial_year = ? ORDER BY invoice_date DESC, id DESC;";
        params << fyLabel;
    } else {
        sql = "SELECT id, voucher_no, invoice_no, invoice_date, supplier_name, item_name, bag_count, weight_qtl, rate_per_qtl, taxable_amount, gst_pct, cgst_amount, sgst_amount, igst_amount, gst_amount, round_off, total_amount, payment_mode, vehicle_no, eway_bill_no, financial_year, narration, market_type, due_days, tax_status, challan_no, freight_charges, tcs_amount, place_of_supply FROM purchase_invoices ORDER BY invoice_date DESC, id DESC;";
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

QVariantMap PurchaseModel::get_previous_purchase_invoice(int currentId, const QString& currentInvOrVchNo) {
    FiscalYearInfo fy;
    if (currentId > 0) {
        QVariant curD = DatabaseManager::instance().executeScalar(
            "SELECT invoice_date FROM purchase_invoices WHERE id = ? LIMIT 1;", {currentId}
        );
        if (curD.isValid() && !curD.toString().isEmpty()) {
            fy = FiscalYearHelper::getFiscalYearForDate(curD.toString());
        }
    }
    if (!fy.isValid() && !currentInvOrVchNo.trimmed().isEmpty()) {
        fy = FiscalYearHelper::resolveFiscalYear(currentInvOrVchNo);
    }
    if (!fy.isValid()) {
        fy = FiscalYearHelper::getActiveFiscalYear();
    }

    QVariant targetId;
    if (currentId > 0) {
        if (fy.isValid()) {
            targetId = DatabaseManager::instance().executeScalar(
                "SELECT id FROM purchase_invoices WHERE id < ? AND invoice_date >= ? AND invoice_date <= ? ORDER BY id DESC LIMIT 1;",
                {currentId, fy.startDate, fy.endDate}
            );
        } else {
            targetId = DatabaseManager::instance().executeScalar(
                "SELECT id FROM purchase_invoices WHERE id < ? ORDER BY id DESC LIMIT 1;", {currentId}
            );
        }
    }
    if (!targetId.isValid() && !currentInvOrVchNo.trimmed().isEmpty()) {
        QVariant curRowId = DatabaseManager::instance().executeScalar(
            "SELECT id FROM purchase_invoices WHERE invoice_no = ? OR voucher_no = ? LIMIT 1;",
            {currentInvOrVchNo, currentInvOrVchNo}
        );
        if (curRowId.isValid()) {
            if (fy.isValid()) {
                targetId = DatabaseManager::instance().executeScalar(
                    "SELECT id FROM purchase_invoices WHERE id < ? AND invoice_date >= ? AND invoice_date <= ? ORDER BY id DESC LIMIT 1;",
                    {curRowId.toInt(), fy.startDate, fy.endDate}
                );
            } else {
                targetId = DatabaseManager::instance().executeScalar(
                    "SELECT id FROM purchase_invoices WHERE id < ? ORDER BY id DESC LIMIT 1;", {curRowId.toInt()}
                );
            }
        }
    }
    if (!targetId.isValid()) {
        if (fy.isValid()) {
            targetId = DatabaseManager::instance().executeScalar(
                "SELECT id FROM purchase_invoices WHERE invoice_date >= ? AND invoice_date <= ? ORDER BY id DESC LIMIT 1;",
                {fy.startDate, fy.endDate}
            );
        } else {
            targetId = DatabaseManager::instance().executeScalar(
                "SELECT id FROM purchase_invoices ORDER BY id DESC LIMIT 1;"
            );
        }
    }
    if (targetId.isValid()) {
        return get_purchase_invoice(targetId.toInt());
    }

    // No vouchers-table fallback: vouchers.id lives in a different id-space
    // than purchase_invoices.id (resolving one as the other opened unrelated
    // bills instead of reporting none).
    return {};
}

QVariantMap PurchaseModel::get_next_purchase_invoice(int currentId, const QString& currentInvOrVchNo) {
    if (currentId <= 0 && currentInvOrVchNo.trimmed().isEmpty()) return {};

    FiscalYearInfo fy;
    if (currentId > 0) {
        QVariant curD = DatabaseManager::instance().executeScalar(
            "SELECT invoice_date FROM purchase_invoices WHERE id = ? LIMIT 1;", {currentId}
        );
        if (curD.isValid() && !curD.toString().isEmpty()) {
            fy = FiscalYearHelper::getFiscalYearForDate(curD.toString());
        }
    }
    if (!fy.isValid() && !currentInvOrVchNo.trimmed().isEmpty()) {
        fy = FiscalYearHelper::resolveFiscalYear(currentInvOrVchNo);
    }
    if (!fy.isValid()) {
        fy = FiscalYearHelper::getActiveFiscalYear();
    }

    int effId = currentId;
    if (effId <= 0 && !currentInvOrVchNo.trimmed().isEmpty()) {
        QVariant curRowId = DatabaseManager::instance().executeScalar(
            "SELECT id FROM purchase_invoices WHERE invoice_no = ? OR voucher_no = ? LIMIT 1;",
            {currentInvOrVchNo, currentInvOrVchNo}
        );
        if (curRowId.isValid()) effId = curRowId.toInt();
    }

    if (effId > 0) {
        QVariant targetId;
        if (fy.isValid()) {
            targetId = DatabaseManager::instance().executeScalar(
                "SELECT id FROM purchase_invoices WHERE id > ? AND invoice_date >= ? AND invoice_date <= ? ORDER BY id ASC LIMIT 1;",
                {effId, fy.startDate, fy.endDate}
            );
        } else {
            targetId = DatabaseManager::instance().executeScalar(
                "SELECT id FROM purchase_invoices WHERE id > ? ORDER BY id ASC LIMIT 1;", {effId}
            );
        }
        if (targetId.isValid()) {
            return get_purchase_invoice(targetId.toInt());
        }
        // No vouchers-table fallback (different id-space; see get_previous).
    }
    return {};
}

QVariantMap PurchaseModel::get_purchase_invoice(const QVariant& invoiceNoOrId, const QString& dateHintParam, const QString& partyHintParam) {
    int targetId = 0;
    QString q;
    QString vNo;
    QString dateHint = FiscalYearHelper::normalizeToIso(dateHintParam);
    QString partyHint = partyHintParam.trimmed();
    QString explicitFy;

    if (invoiceNoOrId.typeId() == QMetaType::QVariantMap) {
        QVariantMap m = invoiceNoOrId.toMap();
        targetId = m.value("id").toInt();
        q = m.value("invoice_no", m.value("invoiceNo", m.value("refNo", m.value("voucher_no", m.value("voucherNo"))))).toString().trimmed();
        vNo = m.value("voucher_no", m.value("voucherNo")).toString().trimmed();
        if (dateHint.isEmpty()) dateHint = FiscalYearHelper::normalizeToIso(m.value("vIso", m.value("invoice_date", m.value("voucher_date", m.value("date")))).toString());
        if (partyHint.isEmpty()) partyHint = m.value("party_name", m.value("partyName", m.value("supplier_name", m.value("supplierName")))).toString().trimmed();
        explicitFy = m.value("financialYear", m.value("financial_year")).toString().trimmed();
    } else {
        bool isNum = false;
        int parsedId = invoiceNoOrId.toInt(&isNum);
        if (isNum && parsedId > 0 && dateHint.isEmpty()) {
            QVariantList chk = DatabaseManager::instance().executeQuery("SELECT id FROM purchase_invoices WHERE id = ? LIMIT 1;", {parsedId});
            if (!chk.isEmpty()) {
                targetId = parsedId;
            } else {
                q = QString::number(parsedId);
            }
        } else {
            q = invoiceNoOrId.toString().trimmed();
        }
    }

    qDebug() << "[PURCHASE_MODEL] get_purchase_invoice targetId:" << targetId << "q:" << q << "vNo:" << vNo << "dateHint:" << dateHint << "partyHint:" << partyHint << "explicitFy:" << explicitFy;
    if (targetId <= 0 && q.isEmpty() && vNo.isEmpty() && dateHint.isEmpty() && partyHint.isEmpty()) return {};

    FiscalYearInfo targetFy = FiscalYearHelper::resolveFiscalYear(!q.isEmpty() ? q : vNo, dateHint, explicitFy);

    // Canonical identity: exact equality only (see SalesModel). No prefix
    // stripping, no variant expansion, no substring LIKE.
    const QString cleanQ = q;
    const QString cleanVNo = vNo;

    QVariantList rows;

    // STRICT IDENTITY RULES (FY-qualified, exact matches only): same contract
    // as SalesModel::get_sales_invoice — ids are unique so an id miss reports
    // not-found instead of cascading into fuzzy matches; numbers match only
    // by exact equality inside one FY (2526/2627-style tokens select the FY);
    // substring LIKE is banned; nothing is fabricated from vouchers ledgers.

    // 1. Direct search by ID; FY-verified when the FY is determinable.
    if (targetId > 0) {
        if (targetFy.isValid()) {
            rows = DatabaseManager::instance().executeQuery("SELECT * FROM purchase_invoices WHERE id = ? AND invoice_date >= ? AND invoice_date <= ? LIMIT 1;", {targetId, targetFy.startDate, targetFy.endDate});
        } else {
            rows = DatabaseManager::instance().executeQuery("SELECT * FROM purchase_invoices WHERE id = ? LIMIT 1;", {targetId});
        }
        if (!rows.isEmpty()) {
            // exact identity hit; hydrated below
        } else if (cleanQ.isEmpty() && cleanVNo.isEmpty()) {
            return {};
        }
        // else: bogus id but numbers present (typed input) -> numbers strictly.
    }

    // 2. Exact invoice_no / voucher_no on an exact date, else inside exactly
    // one FY (never cross-FY, never LIKE).
    if (rows.isEmpty() && (!cleanQ.isEmpty() || !cleanVNo.isEmpty())) {
        if (!dateHint.isEmpty()) {
            rows = DatabaseManager::instance().executeQuery(
                "SELECT * FROM purchase_invoices WHERE (invoice_no = ? OR voucher_no = ?) AND invoice_date = ? ORDER BY id DESC LIMIT 1;",
                {cleanQ, cleanVNo, dateHint}
            );
        } else {
            const FiscalYearInfo scopeFy = targetFy.isValid() ? targetFy : FiscalYearHelper::getActiveFiscalYear();
            if (scopeFy.isValid()) {
                rows = DatabaseManager::instance().executeQuery(
                    "SELECT * FROM purchase_invoices WHERE (invoice_no = ? OR voucher_no = ?) "
                    "AND invoice_date >= ? AND invoice_date <= ? ORDER BY id DESC LIMIT 1;",
                    {cleanQ, cleanVNo, scopeFy.startDate, scopeFy.endDate}
                );
            }
        }
    }

    if (rows.isEmpty()) return {};
    Q_UNUSED(partyHint);

    QVariantMap inv = rows.first().toMap();
    int invId = inv.value("id").toInt();
    QString invNo = inv.value("invoice_no").toString().trimmed();
    vNo = inv.value("voucher_no").toString().trimmed();

    // Auto-resolve party metadata if gstin or address is missing
    QString suppName = inv.value("supplier_name").toString().trimmed();
    inv["party_ledger"] = suppName;
    if (!suppName.isEmpty()) {
        QVariantList pRows = DatabaseManager::instance().executeQuery(
            "SELECT gstin, address, city, state, phone FROM parties WHERE name = ? COLLATE NOCASE OR alias = ? COLLATE NOCASE LIMIT 1;",
            {suppName, suppName}
        );
        if (pRows.isEmpty() && suppName.contains('[')) {
            QString cleanName = suppName.left(suppName.indexOf('[')).trimmed();
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

    // Fetch line items from purchase_invoice_items by invoice_id or invoice_no
    QVariantList itemRows = DatabaseManager::instance().executeQuery(
        "SELECT * FROM purchase_invoice_items WHERE invoice_id = ? ORDER BY id ASC;",
        {invId}
    );
    if (itemRows.isEmpty() && !invNo.isEmpty()) {
        itemRows = DatabaseManager::instance().executeQuery(
            "SELECT * FROM purchase_invoice_items WHERE invoice_no = ? ORDER BY id ASC;",
            {invNo}
        );
    }
    if (itemRows.isEmpty() && (!vNo.isEmpty() || !invNo.isEmpty())) {
        QVariantList stRows = DatabaseManager::instance().executeQuery(
            "SELECT item_id, item_name, dheri_bill_no AS grade, bags AS bag_count, packing, weight_qtl, rate AS rate_per_qtl, "
            "amount AS total_amount, taxable_amount, tax AS gst_pct "
            "FROM stock_transactions WHERE (voucher_no = ? OR bill_no = ?) AND trans_type IN ('Purc', 'Purchase') "
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

bool PurchaseModel::update_purchase_invoice_full(
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

    QVariant suppRow = DatabaseManager::instance().executeScalar(
        "SELECT id FROM parties WHERE name = ? COLLATE NOCASE LIMIT 1;", {party_ledger}
    );
    int supplierId = suppRow.isValid() ? suppRow.toInt() : 1;
    double gst_amount = cgst_amount + sgst_amount + igst_amount;

    DatabaseManager::instance().beginTransaction();

    // Check if purchase_invoices row exists by id, else by exact invoice/
    // voucher identity WITHIN this row's financial year (same bare number
    // may legally exist in another FY).
    QVariant existingId = DatabaseManager::instance().executeScalar(
        "SELECT id FROM purchase_invoices WHERE id = ? OR ((invoice_no = ? AND invoice_no != '') OR (voucher_no = ? AND voucher_no != '')) AND financial_year = ? LIMIT 1;",
        {invoice_id, invoice_no, voucher_no, fyLabel}
    );

    int targetInvId = existingId.isValid() ? existingId.toInt() : invoice_id;

    if (existingId.isValid()) {
        DatabaseManager::instance().executeNonQuery(
            "UPDATE purchase_invoices SET "
            "voucher_no = ?, invoice_no = ?, invoice_date = ?, supplier_id = ?, supplier_name = ?, gstin = ?, "
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
                voucher_no, invoice_no, dt, supplierId, party_ledger, gstin,
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
            "INSERT INTO purchase_invoices ("
            "fy_id, financial_year, voucher_no, invoice_no, invoice_date, supplier_id, supplier_name, gstin, item_id, item_name, hsn_code, "
            "bag_count, weight_qtl, rate_per_qtl, taxable_amount, gst_pct, cgst_amount, sgst_amount, igst_amount, round_off, gst_amount, "
            "total_amount, payment_mode, vehicle_no, eway_bill_no, narration, sale_status, market_fee_status, dami, labour, auction, "
            "m_fee, hrdf, other_exp, welfare, dhrmd, sutli, less_amount, gr_no, driver, bill_time, sauda_date, shipping_address, "
            "po_no, grade, kanda_weight, transport, broker_name, market_type, due_days, tax_status, challan_no, freight_charges, "
            "tcs_amount, tcs_rate, place_of_supply"
            ") VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);",
            {
                fyId, fyLabel, voucher_no, invoice_no, dt, supplierId, party_ledger, gstin, itemId, item_name, hsn_code,
                bag_count, weight_qtl, rate_per_qtl, taxable_amount, gst_pct, cgst_amount, sgst_amount, igst_amount, round_off, gst_amount,
                total_amount, payment_mode, vehicle_no, eway_bill_no, narration, sale_status, market_fee_status,
                dami, labour, auction, m_fee, hrdf, other_exp, welfare, dhrmd, sutli, less_amount,
                gr_no, driver, bill_time, sauda_date, shipping_address, po_no, grade, kanda_weight, transport, broker_name,
                market_type, due_days, tax_status, challan_no, freight_charges, tcs_amount, tcs_rate, place_of_supply
            }
        );
        targetInvId = static_cast<int>(DatabaseManager::instance().lastInsertedId());
    }

    // Replace line items in purchase_invoice_items
    DatabaseManager::instance().executeNonQuery(
        "DELETE FROM purchase_invoice_items WHERE invoice_id = ? OR invoice_no = ?;",
        {targetInvId, invoice_no}
    );
    if (!items.isEmpty()) {
        for (const QVariant& itmV : items) {
            QVariantMap itm = itmV.toMap();
            DatabaseManager::instance().executeNonQuery(
                "INSERT INTO purchase_invoice_items (invoice_id, invoice_no, item_id, item_name, grade, bag_count, packing, weight_qtl, rate_per_qtl, taxable_amount, gst_pct, total_amount) "
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
        "SELECT id FROM vouchers WHERE (instrument_no = ? OR voucher_no = ? OR id = ?) AND (voucher_type IN ('Purchase', 'Purc') OR legacy_type IN ('Purc', 'Purchase')) LIMIT 1;",
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
                voucher_no, invoice_no, dt, supplierId, party_ledger,
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
            ") VALUES (?, ?, ?, ?, ?, 'Purchase', 'Purc', ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);",
            {
                fyId, fyLabel, voucher_no, invoice_no, dt, supplierId, party_ledger,
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
        "DELETE FROM stock_transactions WHERE (bill_no = ? OR voucher_no = ?) AND trans_type IN ('Purc', 'Purchase', 'P');",
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
                ") VALUES (?, ?, ?, ?, 'Purc', 'Purchase', ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);",
                {
                    fyId, fyLabel, voucher_no, dt, supplierId, party_ledger, invoice_no,
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
            ") VALUES (?, ?, ?, ?, 'Purc', 'Purchase', ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, 1);",
            {
                fyId, fyLabel, voucher_no, dt, supplierId, party_ledger, invoice_no,
                itemId, hsn_code, item_name, bag_count, weight_qtl, rate_per_qtl, total_amount, taxable_amount, vchNarr
            }
        );
    }

    // Delete and re-insert transactions double-entry splits
    DatabaseManager::instance().executeNonQuery(
        "DELETE FROM transactions WHERE (voucher_no = ? OR invoice_no = ?) AND voucher_type IN ('Purchase', 'Purc');",
        {voucher_no, invoice_no}
    );

    QString itemPurchaseLedger = "";
    QVariantList itemMeta = DatabaseManager::instance().executeQuery(
        "SELECT purchase_ledger, purchase_ledger_id FROM stock_items WHERE id = ? LIMIT 1;",
        {itemId}
    );
    if (!itemMeta.isEmpty()) {
        itemPurchaseLedger = itemMeta.first().toMap().value("purchase_ledger").toString().trimmed();
    }
    QString effectivePurcLedger = itemPurchaseLedger.isEmpty() ? "Purchase A/c" : itemPurchaseLedger;

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

    int supCode = resolveLedgerCode(party_ledger, supplierId);
    int purcCode = resolveLedgerCode(effectivePurcLedger, 1050);
    int cgstCode = resolveLedgerCode("CGST A/c", resolveLedgerCode("CGST Input", 944));
    int sgstCode = resolveLedgerCode("SGST A/c", resolveLedgerCode("SGST Input", 943));
    int igstCode = resolveLedgerCode("IGST A/c", resolveLedgerCode("IGST Input", 945));
    int roundOffCode = resolveLedgerCode("Round Off", resolveLedgerCode("Round Off A/c", 1177));

    // Leg 1: Credit Supplier for total_amount (Row 1 is ALWAYS the Party in Bahi-Khata)
    DatabaseManager::instance().executeNonQuery(
        "INSERT INTO transactions ("
        "fy_id, financial_year, voucher_no, voucher_date, voucher_type, trans_type, "
        "account_code, party_id, party_name, opposing_account, dr_cr, amount, "
        "invoice_no, narration, taxable_amount, row_no"
        ") VALUES (?, ?, ?, ?, 'Purchase', 'Purc', ?, ?, ?, ?, 'Cr', ?, ?, ?, ?, 1);",
        {
            fyId, fyLabel, voucher_no, dt,
            supCode, supplierId, party_ledger, effectivePurcLedger, total_amount,
            invoice_no, vchNarr, taxable_amount
        }
    );

    int tRowNo = 2;
    // Leg 2: Debit Purchase Ledger for taxable_amount
    DatabaseManager::instance().executeNonQuery(
        "INSERT INTO transactions ("
        "fy_id, financial_year, voucher_no, voucher_date, voucher_type, trans_type, "
        "account_code, party_name, opposing_account, dr_cr, amount, "
        "invoice_no, narration, taxable_amount, row_no"
        ") VALUES (?, ?, ?, ?, 'Purchase', 'Purc', ?, ?, ?, 'Dr', ?, ?, ?, ?, ?);",
        {
            fyId, fyLabel, voucher_no, dt,
            purcCode, effectivePurcLedger, party_ledger, taxable_amount,
            invoice_no, vchNarr, taxable_amount, tRowNo++
        }
    );

    // Leg 3: Debit CGST Input if > 0
    if (cgst_amount > 0.001) {
        DatabaseManager::instance().executeNonQuery(
            "INSERT INTO transactions ("
            "fy_id, financial_year, voucher_no, voucher_date, voucher_type, trans_type, "
            "account_code, party_name, opposing_account, dr_cr, amount, "
            "invoice_no, narration, row_no"
            ") VALUES (?, ?, ?, ?, 'Purchase', 'Purc', ?, 'CGST Input', ?, 'Dr', ?, ?, ?, ?);",
            {fyId, fyLabel, voucher_no, dt, cgstCode, party_ledger, cgst_amount, invoice_no, vchNarr, tRowNo++}
        );
    }
    // Leg 4: Debit SGST Input if > 0
    if (sgst_amount > 0.001) {
        DatabaseManager::instance().executeNonQuery(
            "INSERT INTO transactions ("
            "fy_id, financial_year, voucher_no, voucher_date, voucher_type, trans_type, "
            "account_code, party_name, opposing_account, dr_cr, amount, "
            "invoice_no, narration, row_no"
            ") VALUES (?, ?, ?, ?, 'Purchase', 'Purc', ?, 'SGST Input', ?, 'Dr', ?, ?, ?, ?);",
            {fyId, fyLabel, voucher_no, dt, sgstCode, party_ledger, sgst_amount, invoice_no, vchNarr, tRowNo++}
        );
    }
    // Leg 5: Debit IGST Input if > 0
    if (igst_amount > 0.001) {
        DatabaseManager::instance().executeNonQuery(
            "INSERT INTO transactions ("
            "fy_id, financial_year, voucher_no, voucher_date, voucher_type, trans_type, "
            "account_code, party_name, opposing_account, dr_cr, amount, "
            "invoice_no, narration, row_no"
            ") VALUES (?, ?, ?, ?, 'Purchase', 'Purc', ?, 'IGST Input', ?, 'Dr', ?, ?, ?, ?);",
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
            ") VALUES (?, ?, ?, ?, 'Purchase', 'Purc', ?, 'Round Off', ?, ?, ?, ?, ?, ?);",
            {fyId, fyLabel, voucher_no, dt, roundOffCode, party_ledger, roDrCr, std::abs(round_off), invoice_no, vchNarr, tRowNo++}
        );
    }

    DatabaseManager::instance().commit();
    reload_data();
    return true;
}

bool PurchaseModel::delete_purchase_invoice(int invoice_id, const QString& invoice_no) {
    if (invoice_id <= 0 && invoice_no.trimmed().isEmpty()) {
        return false;
    }

    QString invNo = invoice_no.trimmed();
    QString vNo;
    if (invoice_id > 0) {
        QVariantMap invRow = DatabaseManager::instance().executeQuery(
            "SELECT invoice_no, voucher_no FROM purchase_invoices WHERE id = ? LIMIT 1;", {invoice_id}
        ).value(0).toMap();
        if (invNo.isEmpty()) invNo = invRow.value("invoice_no").toString();
        vNo = invRow.value("voucher_no").toString();
    }

    DatabaseManager::instance().beginTransaction();

    if (invoice_id > 0) {
        DatabaseManager::instance().executeNonQuery("DELETE FROM purchase_invoice_items WHERE invoice_id = ?;", {invoice_id});
        DatabaseManager::instance().executeNonQuery("DELETE FROM purchase_invoices WHERE id = ?;", {invoice_id});
    }

    if (!invNo.isEmpty()) {
        DatabaseManager::instance().executeNonQuery("DELETE FROM purchase_invoice_items WHERE invoice_no = ?;", {invNo});
        DatabaseManager::instance().executeNonQuery("DELETE FROM purchase_invoices WHERE invoice_no = ?;", {invNo});
        DatabaseManager::instance().executeNonQuery("DELETE FROM vouchers WHERE instrument_no = ? AND (voucher_type IN ('Purchase', 'Purc') OR legacy_type IN ('Purc', 'Purchase'));", {invNo});
        DatabaseManager::instance().executeNonQuery("DELETE FROM stock_transactions WHERE bill_no = ? AND trans_type IN ('Purc', 'Purchase', 'P');", {invNo});
        DatabaseManager::instance().executeNonQuery("DELETE FROM transactions WHERE (invoice_no = ? OR voucher_no = ?) AND voucher_type IN ('Purchase', 'Purc');", {invNo, invNo});
    }

    if (!vNo.isEmpty()) {
        DatabaseManager::instance().executeNonQuery("DELETE FROM vouchers WHERE voucher_no = ? AND (voucher_type IN ('Purchase', 'Purc') OR legacy_type IN ('Purc', 'Purchase'));", {vNo});
        DatabaseManager::instance().executeNonQuery("DELETE FROM stock_transactions WHERE voucher_no = ? AND trans_type IN ('Purc', 'Purchase', 'P');", {vNo});
        DatabaseManager::instance().executeNonQuery("DELETE FROM transactions WHERE voucher_no = ? AND voucher_type IN ('Purchase', 'Purc');", {vNo});
    }

    DatabaseManager::instance().commit();
    reload_data();
    return true;
}

