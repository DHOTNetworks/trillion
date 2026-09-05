#include "milling_model.h"
#include "../database_manager.h"
#include "../engine/accounting_engine.h"
#include <QDate>
#include <QRegularExpression>

MillingModel::MillingModel(QObject* parent)
    : BaseTableModel(
        {"Batch No", "Date", "Paddy Input (Qtl)", "Rice Output (Qtl)", "Outturn %", "Byproducts (Qtl)", "Total Cost (₹)"},
        {"batch_no", "batch_date", "paddy_weight_qtl", "rice_weight_qtl", "outturn_pct", "total_byproduct_weight_qtl", "total_milling_cost"},
        parent
    )
{
    reload_data();
}

void MillingModel::auto_repair_milling_batches() {
    QVariant check = DatabaseManager::instance().executeScalar(
        "SELECT COUNT(*) FROM milling_batches WHERE head_rice_qtl = 0.0 AND paddy_input_qtl > 0.0;"
    );
    if (check.isValid() && check.toInt() > 0) {
        DatabaseManager::instance().executeNonQuery(
            "UPDATE milling_batches AS mb "
            "SET "
            "  head_rice_qtl = IFNULL((SELECT SUM(weight_qtl) FROM milling_voucher_items WHERE batch_id = mb.id AND drcr = 'Dr' AND NOT (LOWER(item_name) LIKE '%bran%' AND LOWER(item_name) NOT LIKE '%brand%') AND LOWER(item_name) NOT LIKE '%broken%' AND LOWER(item_name) NOT LIKE '%nakku%' AND LOWER(item_name) NOT LIKE '%tibar%' AND LOWER(item_name) NOT LIKE '%dubar%' AND LOWER(item_name) NOT LIKE '%mogra%' AND LOWER(item_name) NOT LIKE '%kinki%' AND LOWER(item_name) NOT LIKE '%husk%' AND LOWER(item_name) NOT LIKE '%phak%' AND LOWER(item_name) NOT LIKE '%bhusa%'), 0.0), "
            "  bran_qtl = IFNULL((SELECT SUM(weight_qtl) FROM milling_voucher_items WHERE batch_id = mb.id AND drcr = 'Dr' AND LOWER(item_name) LIKE '%bran%' AND LOWER(item_name) NOT LIKE '%brand%'), 0.0), "
            "  broken_rice_qtl = IFNULL((SELECT SUM(weight_qtl) FROM milling_voucher_items WHERE batch_id = mb.id AND drcr = 'Dr' AND (LOWER(item_name) LIKE '%broken%' OR LOWER(item_name) LIKE '%nakku%' OR LOWER(item_name) LIKE '%tibar%' OR LOWER(item_name) LIKE '%dubar%' OR LOWER(item_name) LIKE '%mogra%' OR LOWER(item_name) LIKE '%kinki%')), 0.0), "
            "  husk_qtl = IFNULL((SELECT SUM(weight_qtl) FROM milling_voucher_items WHERE batch_id = mb.id AND drcr = 'Dr' AND (LOWER(item_name) LIKE '%husk%' OR LOWER(item_name) LIKE '%phak%' OR LOWER(item_name) LIKE '%bhusa%')), 0.0);"
        );
        DatabaseManager::instance().executeNonQuery(
            "UPDATE milling_batches SET "
            "  wastage_qtl = ROUND(MAX(0.0, paddy_input_qtl - (head_rice_qtl + broken_rice_qtl + bran_qtl + husk_qtl)), 3), "
            "  yield_pct = CASE WHEN paddy_input_qtl > 0 THEN ROUND((head_rice_qtl / paddy_input_qtl) * 100.0, 2) ELSE 0.0 END;"
        );
    }
}

void MillingModel::reload_data() {
    auto_repair_milling_batches();
    beginResetModel();
    m_data = DatabaseManager::instance().executeQuery("SELECT * FROM milling_batches ORDER BY id DESC;");
    endResetModel();
    emit dataChangedSignal();
    emit countChanged();
}

QString MillingModel::get_next_batch_no(const QString& fy) {
    QString targetFy = fy;
    if (targetFy.isEmpty()) {
        QVariant fyVal = DatabaseManager::instance().executeScalar("SELECT year_name FROM financial_years WHERE is_active = 1 LIMIT 1;");
        targetFy = fyVal.isValid() ? fyVal.toString() : "FY 2026-27";
    }

    QVariantList rows = DatabaseManager::instance().executeQuery(
        "SELECT batch_no FROM milling_batches WHERE financial_year = ?;",
        {targetFy}
    );

    long long maxId = 0;
    QRegularExpression re("(\\d+)$");
    for (const QVariant& r : rows) {
        QString b = r.toMap().value("batch_no").toString();
        QRegularExpressionMatch m = re.match(b);
        if (m.hasMatch()) {
            long long num = m.captured(1).toLongLong();
            if (num > maxId) maxId = num;
        }
    }
    return QString("Mill-%1").arg(maxId + 1);
}

bool MillingModel::add_milling_voucher_full(
    const QString& batch_no, const QString& batch_date, const QString& paddy_item,
    int paddy_bags, double paddy_weight, const QString& rice_item, int rice_bags,
    double rice_weight, double outturn_pct, double rice_cost,
    const QString& bran_item, int bran_bags, double bran_weight, double bran_cost,
    const QString& husk_item, int husk_bags, double husk_weight, double husk_cost,
    const QString& nakku_item, int nakku_bags, double nakku_weight, double nakku_cost,
    const QString& other_byproduct_item, int other_byproduct_bags, double other_byproduct_weight, double other_byproduct_cost,
    int total_byproduct_bags, double total_byproduct_weight, double total_byproduct_cost,
    double loss_weight, double loss_pct, double milling_cost_per_qtl, double total_milling_cost,
    const QString& operator_name, const QString& notes
) {
    QString dt = batch_date.isEmpty() ? QDate::currentDate().toString("yyyy-MM-dd") : batch_date;
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

    QString bNo = batch_no.isEmpty() ? get_next_batch_no(fyLabel) : batch_no;

    // --- ATOMIC ACID TRANSACTION ---
    DatabaseManager::instance().beginTransaction();

    // 1. Insert Milling Batch Record
    bool okBatch = DatabaseManager::instance().executeNonQuery(
        "INSERT INTO milling_batches ("
        "fy_id, financial_year, batch_no, batch_date, paddy_variety, paddy_input_qtl, "
        "head_rice_qtl, broken_rice_qtl, bran_qtl, husk_qtl, wastage_qtl, yield_pct, narration"
        ") VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);",
        {
            fyId, fyLabel, bNo, dt, paddy_item, paddy_weight,
            rice_weight, nakku_weight, bran_weight, husk_weight, loss_weight, outturn_pct, notes
        }
    );

    if (!okBatch) {
        DatabaseManager::instance().rollback();
        return false;
    }

    qint64 batchId = DatabaseManager::instance().lastInsertedId();

    // 2. Line Items: Raw Paddy Consumed (Cr)
    DatabaseManager::instance().executeNonQuery(
        "INSERT INTO milling_voucher_items ("
        "batch_id, batch_no, batch_date, row_no, drcr, item_name, weight_qtl, bags, narration"
        ") VALUES (?, ?, ?, 1, 'Cr', ?, ?, ?, 'Raw Paddy Consumed in Milling');",
        {batchId, bNo, dt, paddy_item, paddy_weight, paddy_bags}
    );

    // 3. Line Items: Finished Rice & By-products Produced (Dr)
    int rowIdx = 2;
    DatabaseManager::instance().executeNonQuery(
        "INSERT INTO milling_voucher_items (batch_id, batch_no, batch_date, row_no, drcr, item_name, weight_qtl, bags, percentage, narration) "
        "VALUES (?, ?, ?, ?, 'Dr', ?, ?, ?, ?, 'Head Rice Produced');",
        {batchId, bNo, dt, rowIdx++, rice_item, rice_weight, rice_bags, outturn_pct}
    );

    if (bran_weight > 0) {
        DatabaseManager::instance().executeNonQuery(
            "INSERT INTO milling_voucher_items (batch_id, batch_no, batch_date, row_no, drcr, item_name, weight_qtl, bags, narration) "
            "VALUES (?, ?, ?, ?, 'Dr', ?, ?, ?, 'Rice Bran Produced');",
            {batchId, bNo, dt, rowIdx++, bran_item.isEmpty() ? "Rice Bran" : bran_item, bran_weight, bran_bags}
        );
    }

    if (husk_weight > 0) {
        DatabaseManager::instance().executeNonQuery(
            "INSERT INTO milling_voucher_items (batch_id, batch_no, batch_date, row_no, drcr, item_name, weight_qtl, bags, narration) "
            "VALUES (?, ?, ?, ?, 'Dr', ?, ?, ?, 'Rice Husk Produced');",
            {batchId, bNo, dt, rowIdx++, husk_item.isEmpty() ? "Rice Husk" : husk_item, husk_weight, husk_bags}
        );
    }

    if (nakku_weight > 0) {
        DatabaseManager::instance().executeNonQuery(
            "INSERT INTO milling_voucher_items (batch_id, batch_no, batch_date, row_no, drcr, item_name, weight_qtl, bags, narration) "
            "VALUES (?, ?, ?, ?, 'Dr', ?, ?, ?, 'Rice Nakku / Broken Produced');",
            {batchId, bNo, dt, rowIdx++, nakku_item.isEmpty() ? "Rice Nakku" : nakku_item, nakku_weight, nakku_bags}
        );
    }

    if (other_byproduct_weight > 0) {
        DatabaseManager::instance().executeNonQuery(
            "INSERT INTO milling_voucher_items (batch_id, batch_no, batch_date, row_no, drcr, item_name, weight_qtl, bags, narration) "
            "VALUES (?, ?, ?, ?, 'Dr', ?, ?, ?, 'Other Byproduct Produced');",
            {batchId, bNo, dt, rowIdx++, other_byproduct_item.isEmpty() ? "Other Byproduct" : other_byproduct_item, other_byproduct_weight, other_byproduct_bags}
        );
    }

    // 4. Double-Entry Accounting Voucher
    QString vchNarr = QString("Milling Batch %1 - In: %2 (%3 Qtl), Out: %4 (%5 Qtl, %6%)").arg(bNo, paddy_item, QString::number(paddy_weight), rice_item, QString::number(rice_weight), QString::number(outturn_pct));
    if (!notes.isEmpty()) vchNarr += " | " + notes;

    bool okVch = DatabaseManager::instance().executeNonQuery(
        "INSERT INTO vouchers (fy_id, financial_year, voucher_no, voucher_date, voucher_type, legacy_type, party_name, account_type, amount, narration) "
        "VALUES (?, ?, ?, ?, 'Milling', 'Mill', 'Milling Production Account', 'Inventory Account', ?, ?);",
        {fyId, fyLabel, bNo, dt, total_milling_cost, vchNarr}
    );

    if (!okVch) {
        DatabaseManager::instance().rollback();
        return false;
    }

    // Commit Transaction
    DatabaseManager::instance().commit();
    reload_data();
    return true;
}

QVariantList MillingModel::get_milling_statement(const QString& from_date, const QString& to_date, const QString& variety) {
    QString sql = "SELECT * FROM milling_batches WHERE 1=1";
    QVariantList args;
    if (!from_date.isEmpty()) {
        sql += " AND batch_date >= ?";
        args.append(from_date);
    }
    if (!to_date.isEmpty()) {
        sql += " AND batch_date <= ?";
        args.append(to_date);
    }
    if (!variety.isEmpty() && variety != "All" && variety != "All Varieties") {
        sql += " AND (paddy_variety = ? OR paddy_item = ?)";
        args.append(variety);
        args.append(variety);
    }
    sql += " ORDER BY batch_date DESC, id DESC;";

    QVariantList rawRows = DatabaseManager::instance().executeQuery(sql, args);
    QVariantList result;
    for (const QVariant& r : rawRows) {
        QVariantMap row = r.toMap();
        double pWeight = row.value("paddy_input_qtl").toDouble();
        double rWeight = row.value("head_rice_qtl").toDouble();
        double broken = row.value("broken_rice_qtl").toDouble();
        double bran = row.value("bran_qtl").toDouble();
        double husk = row.value("husk_qtl").toDouble();
        double wastage = row.value("wastage_qtl").toDouble();
        double outturn = row.value("yield_pct").toDouble();

        row["paddy_input_fmt"] = AccountingEngine::formatIndianNumber(pWeight, 2, "Qtl");
        row["paddy_weight_fmt"] = row["paddy_input_fmt"];
        row["head_rice_fmt"] = AccountingEngine::formatIndianNumber(rWeight, 2, "Qtl");
        row["rice_weight_fmt"] = row["head_rice_fmt"];
        row["broken_rice_fmt"] = AccountingEngine::formatIndianNumber(broken, 2, "Qtl");
        row["bran_fmt"] = AccountingEngine::formatIndianNumber(bran, 2, "Qtl");
        row["husk_fmt"] = AccountingEngine::formatIndianNumber(husk, 2, "Qtl");
        row["wastage_fmt"] = AccountingEngine::formatIndianNumber(wastage, 2, "Qtl");
        row["yield_pct_fmt"] = QString::number(outturn, 'f', 2) + "%";
        row["outturn_fmt"] = row["yield_pct_fmt"];
        row["total_cost_fmt"] = AccountingEngine::formatIndianNumber(wastage, 2, "Qtl");
        result.append(row);
    }
    return result;
}

QVariantMap MillingModel::get_milling_totals(const QString& from_date, const QString& to_date) {
    QString sql = "SELECT COUNT(*) as cnt, SUM(paddy_input_qtl) as tot_paddy, SUM(head_rice_qtl) as tot_rice, "
                  "SUM(broken_rice_qtl) as tot_broken, SUM(bran_qtl) as tot_bran, SUM(husk_qtl) as tot_husk, "
                  "SUM(wastage_qtl) as tot_wastage FROM milling_batches WHERE 1=1";
    QVariantList args;
    if (!from_date.isEmpty()) {
        sql += " AND batch_date >= ?";
        args.append(from_date);
    }
    if (!to_date.isEmpty()) {
        sql += " AND batch_date <= ?";
        args.append(to_date);
    }

    QVariantList rows = DatabaseManager::instance().executeQuery(sql, args);
    QVariantMap res;
    if (!rows.isEmpty()) {
        QVariantMap r = rows.first().toMap();
        int cnt = r.value("cnt").toInt();
        double p = r.value("tot_paddy").toDouble();
        double rice = r.value("tot_rice").toDouble();
        double broken = r.value("tot_broken").toDouble();
        double bran = r.value("tot_bran").toDouble();
        double husk = r.value("tot_husk").toDouble();
        double wastage = r.value("tot_wastage").toDouble();
        double avgYield = p > 0 ? (rice / p * 100.0) : 0.0;

        res["total_batches"] = cnt;
        res["total_paddy_fmt"] = AccountingEngine::formatIndianNumber(p, 2, "Qtl");
        res["total_head_rice_fmt"] = AccountingEngine::formatIndianNumber(rice, 2, "Qtl");
        res["total_broken_fmt"] = AccountingEngine::formatIndianNumber(broken, 2, "Qtl");
        res["total_bran_fmt"] = AccountingEngine::formatIndianNumber(bran, 2, "Qtl");
        res["total_husk_fmt"] = AccountingEngine::formatIndianNumber(husk, 2, "Qtl");
        res["total_wastage_fmt"] = AccountingEngine::formatIndianNumber(wastage, 2, "Qtl");
        res["avg_yield_fmt"] = QString::number(avgYield, 'f', 2) + "%";
    }
    return res;
}

QVariantList MillingModel::get_batch_items(int batch_id, const QString& batch_no) {
    QVariantList rawRows;
    if (batch_id > 0) {
        rawRows = DatabaseManager::instance().executeQuery(
            "SELECT * FROM milling_voucher_items WHERE batch_id = ? ORDER BY row_no ASC, id ASC;",
            {batch_id}
        );
    }
    if (rawRows.isEmpty() && !batch_no.isEmpty()) {
        rawRows = DatabaseManager::instance().executeQuery(
            "SELECT * FROM milling_voucher_items WHERE batch_no = ? ORDER BY row_no ASC, id ASC;",
            {batch_no}
        );
    }

    QVariantList result;
    for (const QVariant& r : rawRows) {
        QVariantMap item = r.toMap();
        double wt = item.value("weight_qtl").toDouble();
        double rate = item.value("rate").toDouble();
        double amt = item.value("amount").toDouble();
        double pct = item.value("percentage").toDouble();

        item["weight_fmt"] = AccountingEngine::formatIndianNumber(wt, 2, "Qtl");
        item["rate_fmt"] = rate > 0.0 ? AccountingEngine::formatIndianCurrency(rate) : "-";
        item["amount_fmt"] = amt > 0.0 ? AccountingEngine::formatIndianCurrency(amt) : "₹0.00";
        item["percentage_fmt"] = pct > 0.0 ? QString::number(pct, 'f', 2) + "%" : "-";
        result.append(item);
    }
    return result;
}
