#include "paddy_arrivals_model.h"
#include "../database_manager.h"
#include <QDate>

PaddyArrivalsModel::PaddyArrivalsModel(QObject* parent)
    : BaseTableModel(
        {"Slip No", "Date", "Farmer", "Variety", "Bags", "Gross (Qtl)", "Moist %", "Net (Qtl)", "Rate (₹)", "Net Amt (₹)", "Status"},
        {"slip_no", "arrival_date", "farmer_name", "variety", "bag_count", "gross_weight_qtl", "moisture_pct", "net_weight_qtl", "rate_per_qtl", "net_amount", "payment_status"},
        parent
    )
{
}

void PaddyArrivalsModel::reload_data() {
    beginResetModel();
    // S7 unified: arrival slips live in paddy_slips (slip_type='arrival').
    m_data = DatabaseManager::instance().executeQuery("SELECT * FROM paddy_slips WHERE slip_type = 'arrival' ORDER BY id DESC;");
    endResetModel();
    emit dataChangedSignal();
    emit countChanged();
}

bool PaddyArrivalsModel::add_arrival(const QString& farmer_name, const QString& paddy_variety, const QString& arrival_date, int bags, double gross_qtl, double moisture_pct, double rate_qtl, double hamali, const QString& status) {
    QString dt = arrival_date.isEmpty() ? QDate::currentDate().toString("yyyy-MM-dd") : arrival_date;
    QVariant maxRow = DatabaseManager::instance().executeScalar("SELECT MAX(id) FROM paddy_slips WHERE slip_type = 'arrival';");
    long long maxId = maxRow.isValid() ? maxRow.toLongLong() : 1000;
    QString slipNo = QString("SLIP-%1").arg(maxId + 1);

    double moistDed = (gross_qtl * (moisture_pct - 17.0) * 0.01);
    if (moistDed < 0) moistDed = 0;
    double netQtl = qMax(0.1, gross_qtl - moistDed);
    double netAmt = (netQtl * rate_qtl) - (bags * hamali);

    // S7 accepted follow-up: the hardcoded farmer_id=1 violated the
    // parties FK on every DB whose ids don't start at 1 (fixture-migrated
    // charts start at 9). Resolve the farmer by name; NULL when unknown —
    // farmer_name remains the display key, and real farmers still link.
    QVariant farmerIdVar;
    const QString farmerKey = farmer_name.trimmed();
    if (!farmerKey.isEmpty()) {
        QVariant fRow = DatabaseManager::instance().executeScalar(
            "SELECT id FROM parties WHERE name = ? COLLATE NOCASE LIMIT 1;",
            {farmerKey});
        if (fRow.isValid() && !fRow.isNull()) farmerIdVar = fRow.toLongLong();
    }

    bool ok = DatabaseManager::instance().executeNonQuery(
        "INSERT INTO paddy_slips (slip_type, slip_no, arrival_date, farmer_id, farmer_name, variety, bag_count, gross_weight_qtl, moisture_pct, moisture_deduction_qtl, net_weight_qtl, rate_per_qtl, hamali_charges, net_amount, payment_status) "
        "VALUES ('arrival', ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);",
        {slipNo, dt, farmerIdVar, farmer_name, paddy_variety, bags, gross_qtl, moisture_pct, moistDed, netQtl, rate_qtl, hamali, netAmt, status}
    );
    if (ok) reload_data();
    return ok;
}
