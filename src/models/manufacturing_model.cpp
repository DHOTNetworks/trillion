#include "manufacturing_model.h"
#include "../database_manager.h"
#include "../engine/accounting_engine.h"
#include "../engine/fiscal_year_helper.h"
#include "../engine/stock_pipeline.h"
#include <QDate>
#include <QRegularExpression>

// Generic manufacturing domain (manufacturer flag). A batch consumes raw
// material (Cr lines) and produces finished goods + by-products (Dr lines).
// Item classification is DATA-DRIVEN via stock_items.manufacturing_role
// ('input' | 'output' | 'byproduct' | '') — never item-name heuristics, so a
// non-rice manufacturer ( dal, oil, plastic… ) records the same way.

namespace {
// Sum the weight of produced (Dr) lines whose item role is `role` (empty
// role = primary output). Lines without a matching master row count as
// primary output, so unmapped items never silently vanish.
double producedWeightForRole(const QVariantList& produced, const QString& role) {
    double total = 0.0;
    for (const QVariant& pVar : produced) {
        QVariantMap p = pVar.toMap();
        QString name = p.value("itemName").toString().trimmed();
        if (name.isEmpty()) name = p.value("item_name").toString().trimmed();
        if (name.isEmpty()) continue;
        double wt = p.value("weight").toDouble();
        if (wt == 0.0) wt = p.value("weight_qtl").toDouble();
        StockItemRecord itm = StockPipeline::instance().getItemByName(name);
        const QString r = itm.manufacturingRole;
        if (role.isEmpty() ? (r.isEmpty() || r == "output") : (r == role)) {
            total += wt;
        }
    }
    return total;
}

QString firstItemName(const QVariantList& items) {
    for (const QVariant& v : items) {
        QVariantMap m = v.toMap();
        QString name = m.value("itemName").toString().trimmed();
        if (name.isEmpty()) name = m.value("item_name").toString().trimmed();
        if (!name.isEmpty()) return name;
    }
    return QString();
}
} // namespace

ManufacturingModel::ManufacturingModel(QObject* parent)
    : BaseTableModel(
        {"Batch No", "Date", "Raw Material Input (Qtl)", "Finished Output (Qtl)", "Outturn %", "By-products (Qtl)", "Total Cost (₹)"},
        {"batch_no", "batch_date", "input_qtl", "output_qtl", "outturn_pct", "byproduct_qtl", "total_manufacturing_cost"},
        parent
    )
{
}

void ManufacturingModel::auto_repair_manufacturing_batches() {
    QVariant check = DatabaseManager::instance().executeScalar(
        "SELECT COUNT(*) FROM manufacturing_batches WHERE output_qtl = 0.0 AND input_qtl > 0.0;"
    );
    if (!check.isValid() || check.toInt() == 0) return;
    // Recompute from the line items: role-split produced lines, residual
    // wastage, output yield. No item-name assumptions.
    DatabaseManager::instance().executeNonQuery(
        "UPDATE manufacturing_batches AS mb SET "
        "  input_qtl = IFNULL((SELECT SUM(weight_qtl) FROM manufacturing_voucher_items WHERE batch_id = mb.id AND drcr = 'Cr'), 0.0), "
        "  output_qtl = IFNULL((SELECT SUM(v.weight_qtl) FROM manufacturing_voucher_items v "
        "        LEFT JOIN stock_items si ON si.name = v.item_name "
        "        WHERE v.batch_id = mb.id AND v.drcr = 'Dr' AND (si.manufacturing_role IS NULL OR si.manufacturing_role != 'byproduct')), 0.0), "
        "  byproduct_qtl = IFNULL((SELECT SUM(v.weight_qtl) FROM manufacturing_voucher_items v "
        "        LEFT JOIN stock_items si ON si.name = v.item_name "
        "        WHERE v.batch_id = mb.id AND v.drcr = 'Dr' AND si.manufacturing_role = 'byproduct'), 0.0);"
    );
    DatabaseManager::instance().executeNonQuery(
        "UPDATE manufacturing_batches SET "
        "  wastage_qtl = ROUND(MAX(0.0, input_qtl - (output_qtl + byproduct_qtl)), 3), "
        "  yield_pct = CASE WHEN input_qtl > 0 THEN ROUND((output_qtl / input_qtl) * 100.0, 2) ELSE 0.0 END;"
    );
}

void ManufacturingModel::reload_data() {
    auto_repair_manufacturing_batches();
    beginResetModel();
    m_data = DatabaseManager::instance().executeQuery("SELECT * FROM manufacturing_batches ORDER BY id DESC;");
    endResetModel();
    emit dataChangedSignal();
    emit countChanged();
}

QString ManufacturingModel::get_next_batch_no(const QString& fy) {
    QString targetFy = AccountingEngine::resolveFinancialYear(fy);
    QString fyPattern = "%" + targetFy.mid(3).trimmed() + "%";

    QVariantList rows = DatabaseManager::instance().executeQuery(
        "SELECT batch_no FROM manufacturing_batches WHERE financial_year = ? OR financial_year LIKE ?;",
        {targetFy, fyPattern}
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
    return QString("MANU-%1").arg(maxId + 1);
}

bool ManufacturingModel::add_manufacturing_voucher(
    const QString& batch_no,
    const QString& batch_date,
    const QString& particulars,
    const QVariantList& consumed_items,
    const QVariantList& produced_items,
    int edit_id
) {
    QString dt = batch_date.trimmed();
    if (dt.isEmpty()) dt = QDate::currentDate().toString("yyyy-MM-dd");
    if (dt.contains("-")) {
        QStringList parts = dt.split("-");
        if (parts.size() == 3 && parts[2].length() == 4) {
            dt = parts[2] + "-" + parts[1] + "-" + parts[0];
        }
    }

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

    QString bNo = batch_no.trimmed();
    if (bNo.isEmpty()) {
        bNo = get_next_batch_no(fyLabel);
    }

    double inputWeight = 0.0;
    int inputBags = 0;
    double consumedAmount = 0.0;
    for (const QVariant& cVar : consumed_items) {
        QVariantMap c = cVar.toMap();
        inputBags += c.value("bags").toInt();
        double wt = c.value("weight").toDouble();
        if (wt == 0.0) wt = c.value("weight_qtl").toDouble();
        inputWeight += wt;
        consumedAmount += c.value("amount").toDouble();
    }

    double outputWeight = producedWeightForRole(produced_items, QString());
    double byproductWeight = producedWeightForRole(produced_items, QString("byproduct"));
    double totalProducedWeight = 0.0;
    for (const QVariant& pVar : produced_items) {
        QVariantMap p = pVar.toMap();
        double wt = p.value("weight").toDouble();
        if (wt == 0.0) wt = p.value("weight_qtl").toDouble();
        totalProducedWeight += wt;
    }
    double yieldPct = inputWeight > 0.0 ? (outputWeight / inputWeight * 100.0) : 0.0;
    double wastageWeight = std::max(0.0, inputWeight - totalProducedWeight);

    const QString inputItem = firstItemName(consumed_items);

    DatabaseManager::instance().beginTransaction();

    qint64 batchId = edit_id;
    if (edit_id > 0) {
        QVariant oldNoVar = DatabaseManager::instance().executeScalar(
            "SELECT batch_no FROM manufacturing_batches WHERE id = ?;", {edit_id}
        );
        QString oldNo = oldNoVar.isValid() ? oldNoVar.toString() : bNo;

        DatabaseManager::instance().executeNonQuery(
            "DELETE FROM manufacturing_voucher_items WHERE batch_id = ?;", {edit_id}
        );
        DatabaseManager::instance().executeNonQuery(
            "DELETE FROM vouchers WHERE (voucher_no = ? OR voucher_no = ?) AND voucher_type = 'Manufacturing';",
            {oldNo, bNo}
        );

        DatabaseManager::instance().executeNonQuery(
            "UPDATE manufacturing_batches SET "
            "fy_id = ?, financial_year = ?, batch_no = ?, batch_date = ?, input_variety = ?, "
            "input_qtl = ?, output_qtl = ?, byproduct_qtl = ?, wastage_qtl = ?, yield_pct = ?, narration = ? "
            "WHERE id = ?;",
            {
                fyId, fyLabel, bNo, dt, inputItem,
                inputWeight, outputWeight, byproductWeight, wastageWeight, yieldPct, particulars, edit_id
            }
        );
    } else {
        bool okBatch = DatabaseManager::instance().executeNonQuery(
            "INSERT INTO manufacturing_batches ("
            "fy_id, financial_year, batch_no, batch_date, input_variety, input_qtl, "
            "output_qtl, byproduct_qtl, wastage_qtl, yield_pct, narration"
            ") VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);",
            {
                fyId, fyLabel, bNo, dt, inputItem, inputWeight,
                outputWeight, byproductWeight, wastageWeight, yieldPct, particulars
            }
        );

        if (!okBatch) {
            DatabaseManager::instance().rollback();
            return false;
        }
        batchId = DatabaseManager::instance().lastInsertedId();
    }

    int rowIdx = 1;
    for (const QVariant& cVar : consumed_items) {
        QVariantMap c = cVar.toMap();
        QString name = c.value("itemName").toString().trimmed();
        if (name.isEmpty()) name = c.value("item_name").toString().trimmed();
        if (name.isEmpty()) continue;

        int bags = c.value("bags").toInt();
        double wt = c.value("weight").toDouble();
        if (wt == 0.0) wt = c.value("weight_qtl").toDouble();
        double amt = c.value("amount").toDouble();
        double rate = c.value("rate").toDouble();
        if (rate == 0.0 && wt > 0.0 && amt > 0.0) rate = amt / wt;

        DatabaseManager::instance().executeNonQuery(
            "INSERT INTO manufacturing_voucher_items ("
            "batch_id, batch_no, batch_date, row_no, drcr, item_name, weight_qtl, bags, rate, amount, narration"
            ") VALUES (?, ?, ?, ?, 'Cr', ?, ?, ?, ?, ?, 'Raw material consumed in production');",
            {batchId, bNo, dt, rowIdx++, name, wt, bags, rate, amt}
        );
    }

    for (const QVariant& pVar : produced_items) {
        QVariantMap p = pVar.toMap();
        QString name = p.value("itemName").toString().trimmed();
        if (name.isEmpty()) name = p.value("item_name").toString().trimmed();
        if (name.isEmpty()) continue;

        int bags = p.value("bags").toInt();
        double wt = p.value("weight").toDouble();
        if (wt == 0.0) wt = p.value("weight_qtl").toDouble();
        double amt = p.value("amount").toDouble();
        double yieldP = p.value("yieldPct").toDouble();
        if (yieldP == 0.0 && inputWeight > 0.0 && wt > 0.0) {
            yieldP = (wt / inputWeight) * 100.0;
        }
        double rate = p.value("rate").toDouble();
        if (rate == 0.0 && wt > 0.0 && amt > 0.0) rate = amt / wt;

        DatabaseManager::instance().executeNonQuery(
            "INSERT INTO manufacturing_voucher_items ("
            "batch_id, batch_no, batch_date, row_no, drcr, item_name, percentage, weight_qtl, bags, rate, amount, narration"
            ") VALUES (?, ?, ?, ?, 'Dr', ?, ?, ?, ?, ?, ?, 'Finished goods / by-product produced');",
            {batchId, bNo, dt, rowIdx++, name, yieldP, wt, bags, rate, amt}
        );
    }

    QString vchNarr = QString("Manufacturing Batch %1 - In: %2 (%3 Qtl), Out: %4 Qtl, %5%")
        .arg(bNo, inputItem, QString::number(inputWeight, 'f', 2),
             QString::number(outputWeight, 'f', 2), QString::number(yieldPct, 'f', 2));
    if (!particulars.isEmpty()) vchNarr += " | " + particulars;

    bool okVch = DatabaseManager::instance().executeNonQuery(
        "INSERT INTO vouchers (fy_id, financial_year, voucher_no, voucher_date, voucher_type, legacy_type, party_name, account_type, amount, narration) "
        "VALUES (?, ?, ?, ?, 'Manufacturing', 'Manu', 'Manufacturing Production Account', 'Inventory Account', ?, ?);",
        {fyId, fyLabel, bNo, dt, consumedAmount, vchNarr}
    );

    if (!okVch) {
        DatabaseManager::instance().rollback();
        return false;
    }

    DatabaseManager::instance().commit();
    reload_data();
    return true;
}

bool ManufacturingModel::add_manufacturing_voucher_full(
    const QString& batch_no, const QString& batch_date,
    const QString& input_item, int input_bags, double input_weight,
    const QVariantList& produced_items,
    double wastage_weight, double wastage_pct,
    double manufacturing_cost_per_qtl, double total_manufacturing_cost,
    const QString& operator_name, const QString& notes
) {
    QString dt = batch_date.isEmpty() ? QDate::currentDate().toString("yyyy-MM-dd") : batch_date;
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

    QString bNo = batch_no.isEmpty() ? get_next_batch_no(fyLabel) : batch_no;
    double yieldPct = input_weight > 0.0 ? (producedWeightForRole(produced_items, QString()) / input_weight * 100.0) : 0.0;

    DatabaseManager::instance().beginTransaction();

    bool okBatch = DatabaseManager::instance().executeNonQuery(
        "INSERT INTO manufacturing_batches ("
        "fy_id, financial_year, batch_no, batch_date, input_variety, input_qtl, "
        "output_qtl, byproduct_qtl, wastage_qtl, yield_pct, narration"
        ") VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);",
        {
            fyId, fyLabel, bNo, dt, input_item, input_weight,
            producedWeightForRole(produced_items, QString()), producedWeightForRole(produced_items, QString("byproduct")), wastage_weight, yieldPct, notes
        }
    );

    if (!okBatch) {
        DatabaseManager::instance().rollback();
        return false;
    }

    qint64 batchId = DatabaseManager::instance().lastInsertedId();

    DatabaseManager::instance().executeNonQuery(
        "INSERT INTO manufacturing_voucher_items ("
        "batch_id, batch_no, batch_date, row_no, drcr, item_name, weight_qtl, bags, narration"
        ") VALUES (?, ?, ?, 1, 'Cr', ?, ?, ?, 'Raw material consumed in production');",
        {batchId, bNo, dt, input_item, input_weight, input_bags}
    );

    int rowIdx = 2;
    for (const QVariant& pVar : produced_items) {
        QVariantMap p = pVar.toMap();
        QString name = p.value("itemName").toString().trimmed();
        if (name.isEmpty()) name = p.value("item_name").toString().trimmed();
        if (name.isEmpty()) continue;
        int bags = p.value("bags").toInt();
        double wt = p.value("weight").toDouble();
        if (wt == 0.0) wt = p.value("weight_qtl").toDouble();
        double y = p.value("yieldPct").toDouble();
        if (y == 0.0 && input_weight > 0.0 && wt > 0.0) y = (wt / input_weight) * 100.0;
        DatabaseManager::instance().executeNonQuery(
            "INSERT INTO manufacturing_voucher_items (batch_id, batch_no, batch_date, row_no, drcr, item_name, weight_qtl, bags, percentage, narration) "
            "VALUES (?, ?, ?, ?, 'Dr', ?, ?, ?, ?, 'Finished goods / by-product produced');",
            {batchId, bNo, dt, rowIdx++, name, wt, bags, y}
        );
    }

    QString vchNarr = QString("Manufacturing Batch %1 - In: %2 (%3 Qtl), Out: %4 Qtl, %5%")
        .arg(bNo, input_item, QString::number(input_weight),
             QString::number(producedWeightForRole(produced_items, QString())), QString::number(yieldPct));
    if (!notes.isEmpty()) vchNarr += " | " + notes;

    bool okVch = DatabaseManager::instance().executeNonQuery(
        "INSERT INTO vouchers (fy_id, financial_year, voucher_no, voucher_date, voucher_type, legacy_type, party_name, account_type, amount, narration) "
        "VALUES (?, ?, ?, ?, 'Manufacturing', 'Manu', 'Manufacturing Production Account', 'Inventory Account', ?, ?);",
        {fyId, fyLabel, bNo, dt, total_manufacturing_cost, vchNarr}
    );

    if (!okVch) {
        DatabaseManager::instance().rollback();
        return false;
    }

    DatabaseManager::instance().commit();
    reload_data();
    return true;
}

QVariantList ManufacturingModel::get_manufacturing_statement(const QString& from_date, const QString& to_date, const QString& variety) {
    QString sql = "SELECT * FROM manufacturing_batches WHERE 1=1";
    QVariantList args;

    QString fDate = from_date.trimmed();
    QString tDate = to_date.trimmed();

    if (fDate.isEmpty() && tDate.isEmpty()) {
        QVariantList fyRows = DatabaseManager::instance().executeQuery("SELECT start_date, end_date FROM financial_years WHERE is_active = 1 LIMIT 1;");
        if (!fyRows.isEmpty()) {
            fDate = fyRows.first().toMap().value("start_date").toString();
            tDate = fyRows.first().toMap().value("end_date").toString();
        }
    }

    if (fDate != "ALL" && fDate != "All" && !fDate.isEmpty()) {
        sql += " AND batch_date >= ?";
        args.append(fDate);
    }
    if (tDate != "ALL" && tDate != "All" && !tDate.isEmpty()) {
        sql += " AND batch_date <= ?";
        args.append(tDate);
    }
    if (!variety.isEmpty() && variety != "All" && variety != "All Varieties") {
        sql += " AND (input_variety = ? OR input_variety LIKE ?)";
        args.append(variety);
        args.append("%" + variety + "%");
    }
    sql += " ORDER BY batch_date DESC, id DESC;";

    QVariantList rawRows = DatabaseManager::instance().executeQuery(sql, args);
    QVariantList result;
    for (const QVariant& r : rawRows) {
        QVariantMap row = r.toMap();
        double inWt = row.value("input_qtl").toDouble();
        double outWt = row.value("output_qtl").toDouble();
        double byp = row.value("byproduct_qtl").toDouble();
        double wastage = row.value("wastage_qtl").toDouble();
        double outturn = row.value("yield_pct").toDouble();

        row["input_qtl_fmt"] = AccountingEngine::formatIndianNumber(inWt, 2, "Qtl");
        row["input_weight_fmt"] = row["input_qtl_fmt"];
        row["output_qtl_fmt"] = AccountingEngine::formatIndianNumber(outWt, 2, "Qtl");
        row["output_weight_fmt"] = row["output_qtl_fmt"];
        row["byproduct_qtl_fmt"] = AccountingEngine::formatIndianNumber(byp, 2, "Qtl");
        row["byproduct_weight_fmt"] = row["byproduct_qtl_fmt"];
        row["wastage_fmt"] = AccountingEngine::formatIndianNumber(wastage, 2, "Qtl");
        row["yield_pct_fmt"] = QString::number(outturn, 'f', 2) + "%";
        row["outturn_fmt"] = row["yield_pct_fmt"];
        row["total_cost_fmt"] = AccountingEngine::formatIndianNumber(wastage, 2, "Qtl");
        result.append(row);
    }
    return result;
}

QVariantMap ManufacturingModel::get_manufacturing_totals(const QString& from_date, const QString& to_date) {
    QString sql = "SELECT COUNT(*) as cnt, SUM(input_qtl) as tot_input, SUM(output_qtl) as tot_output, "
                  "SUM(byproduct_qtl) as tot_byproduct, SUM(wastage_qtl) as tot_wastage FROM manufacturing_batches WHERE 1=1";
    QVariantList args;

    QString fDate = from_date.trimmed();
    QString tDate = to_date.trimmed();

    if (fDate.isEmpty() && tDate.isEmpty()) {
        QVariantList fyRows = DatabaseManager::instance().executeQuery("SELECT start_date, end_date FROM financial_years WHERE is_active = 1 LIMIT 1;");
        if (!fyRows.isEmpty()) {
            fDate = fyRows.first().toMap().value("start_date").toString();
            tDate = fyRows.first().toMap().value("end_date").toString();
        }
    }

    if (fDate != "ALL" && fDate != "All" && !fDate.isEmpty()) {
        sql += " AND batch_date >= ?";
        args.append(fDate);
    }
    if (tDate != "ALL" && tDate != "All" && !tDate.isEmpty()) {
        sql += " AND batch_date <= ?";
        args.append(tDate);
    }

    QVariantList rows = DatabaseManager::instance().executeQuery(sql, args);
    QVariantMap res;
    if (!rows.isEmpty()) {
        QVariantMap r = rows.first().toMap();
        int cnt = r.value("cnt").toInt();
        double in = r.value("tot_input").toDouble();
        double out = r.value("tot_output").toDouble();
        double byp = r.value("tot_byproduct").toDouble();
        double wastage = r.value("tot_wastage").toDouble();
        double avgYield = in > 0 ? (out / in * 100.0) : 0.0;

        res["total_batches"] = cnt;
        res["total_input_fmt"] = AccountingEngine::formatIndianNumber(in, 2, "Qtl");
        res["total_output_fmt"] = AccountingEngine::formatIndianNumber(out, 2, "Qtl");
        res["total_byproduct_fmt"] = AccountingEngine::formatIndianNumber(byp, 2, "Qtl");
        res["total_wastage_fmt"] = AccountingEngine::formatIndianNumber(wastage, 2, "Qtl");
        res["avg_yield_fmt"] = QString::number(avgYield, 'f', 2) + "%";
    }
    return res;
}

QVariantList ManufacturingModel::get_batch_items(int batch_id, const QString& batch_no) {
    QVariantList rawRows;
    if (batch_id > 0) {
        rawRows = DatabaseManager::instance().executeQuery(
            "SELECT * FROM manufacturing_voucher_items WHERE batch_id = ? ORDER BY row_no ASC, id ASC;",
            {batch_id}
        );
    }
    if (rawRows.isEmpty() && !batch_no.isEmpty()) {
        rawRows = DatabaseManager::instance().executeQuery(
            "SELECT * FROM manufacturing_voucher_items WHERE batch_no = ? ORDER BY row_no ASC, id ASC;",
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

QVariantMap ManufacturingModel::get_manufacturing_batch(const QVariant& batchNoOrId) {
    int targetId = 0;
    QString batchNo = "";
    QString dateHint = "";

    if (batchNoOrId.userType() == QMetaType::QVariantMap) {
        QVariantMap m = batchNoOrId.toMap();
        if (m.contains("targetId")) targetId = m.value("targetId").toInt();
        else if (m.contains("id")) targetId = m.value("id").toInt();
        else if (m.contains("batch_id")) targetId = m.value("batch_id").toInt();

        if (m.contains("batch_no")) batchNo = m.value("batch_no").toString().trimmed();
        else if (m.contains("voucher_no")) batchNo = m.value("voucher_no").toString().trimmed();
        else if (m.contains("invoice_no")) batchNo = m.value("invoice_no").toString().trimmed();

        if (m.contains("dateHint")) dateHint = m.value("dateHint").toString().trimmed();
        else if (m.contains("batch_date")) dateHint = m.value("batch_date").toString().trimmed();
        else if (m.contains("voucher_date")) dateHint = m.value("voucher_date").toString().trimmed();
        else if (m.contains("vIso")) dateHint = m.value("vIso").toString().trimmed();
    } else {
        QString qVal = batchNoOrId.toString().trimmed();
        if (qVal.isEmpty()) return {};
        bool isNum = false;
        int num = qVal.toInt(&isNum);
        if (isNum && num > 0) {
            targetId = num;
        }
        batchNo = qVal;
    }

    QVariantList rows;
    if (targetId > 0) {
        rows = DatabaseManager::instance().executeQuery(
            "SELECT * FROM manufacturing_batches WHERE id = ? LIMIT 1;",
            {targetId}
        );
    }

    if (rows.isEmpty() && !batchNo.isEmpty() && !dateHint.isEmpty()) {
        rows = DatabaseManager::instance().executeQuery(
            "SELECT * FROM manufacturing_batches WHERE batch_no = ? AND batch_date = ? LIMIT 1;",
            {batchNo, dateHint}
        );
    }

    if (rows.isEmpty() && !batchNo.isEmpty()) {
        rows = DatabaseManager::instance().executeQuery(
            "SELECT * FROM manufacturing_batches WHERE batch_no = ? ORDER BY id DESC LIMIT 1;",
            {batchNo}
        );
    }

    if (rows.isEmpty()) return {};

    QVariantMap batch = rows.first().toMap();
    int bId = batch.value("id").toInt();
    QString bNo = batch.value("batch_no").toString();

    QVariantList items = get_batch_items(bId, bNo);
    QVariantList consumedList;
    QVariantList producedList;

    for (const auto& itVar : items) {
        QVariantMap it = itVar.toMap();
        QString drcr = it.value("drcr").toString();
        if (drcr == "Cr") {
            consumedList.append(it);
        } else {
            producedList.append(it);
        }
    }

    batch["consumed_items"] = consumedList;
    batch["produced_items"] = producedList;
    return batch;
}
