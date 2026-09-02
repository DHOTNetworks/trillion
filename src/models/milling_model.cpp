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

void MillingModel::reload_data() {
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

QVariantList MillingModel::get_milling_statement(const QString& fy) {
    QString targetFy = fy;
    if (targetFy.isEmpty()) {
        QVariant fyVal = DatabaseManager::instance().executeScalar("SELECT year_name FROM financial_years WHERE is_active = 1 LIMIT 1;");
        targetFy = fyVal.isValid() ? fyVal.toString() : "FY 2026-27";
    }

    QVariantList rawRows = DatabaseManager::instance().executeQuery(
        "SELECT * FROM milling_batches WHERE financial_year = ? ORDER BY batch_date DESC, id DESC;",
        {targetFy}
    );

    QVariantList result;
    for (const QVariant& r : rawRows) {
        QVariantMap row = r.toMap();
        double pWeight = row.value("paddy_input_qtl").toDouble();
        double rWeight = row.value("head_rice_qtl").toDouble();
        double outturn = row.value("yield_pct").toDouble();
        double cost = row.value("wastage_qtl").toDouble();

        row["paddy_weight_fmt"] = AccountingEngine::formatIndianNumber(pWeight, 2, "Qtl");
        row["rice_weight_fmt"] = AccountingEngine::formatIndianNumber(rWeight, 2, "Qtl");
        row["outturn_fmt"] = QString::number(outturn, 'f', 2) + "%";
        row["total_cost_fmt"] = AccountingEngine::formatIndianNumber(cost, 2, "Qtl");
        result.append(row);
    }
    return result;
}
