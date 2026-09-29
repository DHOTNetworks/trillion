#include "bardana_controller.h"
#include "../database_manager.h"
#include "../engine/accounting_engine.h"
#include <QDate>

BardanaController::BardanaController(QObject* parent)
    : QObject(parent)
{
}

bool BardanaController::createBardanaTransaction(
    const QString& vchType,
    const QString& voucherDate,
    const QString& voucherNo,
    int partyId,
    const QString& partyName,
    const QString& bardanaType,
    const QString& godownName,
    const QString& drCr,
    int qty,
    double rate,
    const QString& vehicleNo,
    const QString& billNo,
    const QString& narration
) {
    QString fyLabel = AccountingEngine::getActiveFyLabel();
    if (fyLabel.isEmpty()) fyLabel = AccountingEngine::resolveFinancialYear(voucherDate);

    double amount = qty * rate;

    QVariant pId;
    if (partyId > 0) {
        QVariant chk = DatabaseManager::instance().executeScalar("SELECT id FROM parties WHERE id = ? LIMIT 1;", {partyId});
        if (chk.isValid() && !chk.isNull()) pId = partyId;
    }

    bool ok = DatabaseManager::instance().executeNonQuery(
        "INSERT INTO bardana_transactions ("
        "  financial_year, voucher_no, voucher_date, vch_type, party_id, party_name, "
        "  bardana_type, godown_name, dr_cr, qty, rate, amount, vehicle_no, bill_no, narration"
        ") VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);",
        {
            fyLabel, voucherNo, voucherDate, vchType, pId, partyName,
            bardanaType, godownName.isEmpty() ? "Main Godown" : godownName,
            drCr, qty, rate, amount, vehicleNo, billNo, narration
        }
    );

    if (ok) {
        emit bardanaDataChanged();
    }
    return ok;
}

int BardanaController::getPartyBagBalance(int partyId, const QString& bardanaType) {
    QString sql = "SELECT COALESCE(SUM(CASE WHEN dr_cr = 'Dr' THEN qty ELSE -qty END), 0) "
                  "FROM bardana_transactions WHERE party_id = ?";
    QVariantList params = {partyId};
    if (!bardanaType.isEmpty()) {
        sql += " AND bardana_type = ?";
        params.append(bardanaType);
    }
    QVariant val = DatabaseManager::instance().executeScalar(sql, params);
    return val.isValid() ? val.toInt() : 0;
}

int BardanaController::getGodownBagStock(const QString& godownName, const QString& bardanaType) {
    QString sql = "SELECT COALESCE(SUM(CASE "
                  "  WHEN vch_type IN ('RECEIVE', 'PURCHASE', 'OPENING') THEN qty "
                  "  WHEN vch_type IN ('ISSUE', 'SALE') THEN -qty "
                  "  ELSE 0 END), 0) "
                  "FROM bardana_transactions WHERE godown_name = ?";
    QVariantList params = {godownName};
    if (!bardanaType.isEmpty()) {
        sql += " AND bardana_type = ?";
        params.append(bardanaType);
    }
    QVariant val = DatabaseManager::instance().executeScalar(sql, params);
    return val.isValid() ? val.toInt() : 0;
}

QVariantList BardanaController::getBardanaLedger(int partyId, const QString& fromDate, const QString& toDate) {
    QString sql = "SELECT * FROM bardana_transactions WHERE party_id = ?";
    QVariantList params = {partyId};
    if (!fromDate.isEmpty() && !toDate.isEmpty()) {
        sql += " AND voucher_date >= ? AND voucher_date <= ?";
        params.append(fromDate);
        params.append(toDate);
    }
    sql += " ORDER BY voucher_date ASC, id ASC;";
    return DatabaseManager::instance().executeQuery(sql, params);
}

QVariantList BardanaController::getBardanaRegister(const QString& fromDate, const QString& toDate, const QString& vchType) {
    QString sql = "SELECT * FROM bardana_transactions WHERE 1=1";
    QVariantList params;
    if (!fromDate.isEmpty() && !toDate.isEmpty()) {
        sql += " AND voucher_date >= ? AND voucher_date <= ?";
        params.append(fromDate);
        params.append(toDate);
    }
    if (!vchType.isEmpty() && vchType != "ALL") {
        sql += " AND vch_type = ?";
        params.append(vchType);
    }
    sql += " ORDER BY voucher_date DESC, id DESC;";
    return DatabaseManager::instance().executeQuery(sql, params);
}

QVariantList BardanaController::getGodownStockSummary() {
    return DatabaseManager::instance().executeQuery(
        "SELECT "
        "  godown_name, "
        "  bardana_type, "
        "  SUM(CASE WHEN vch_type IN ('RECEIVE', 'PURCHASE', 'OPENING') THEN qty ELSE 0 END) as inward_bags, "
        "  SUM(CASE WHEN vch_type IN ('ISSUE', 'SALE') THEN qty ELSE 0 END) as outward_bags, "
        "  SUM(CASE "
        "    WHEN vch_type IN ('RECEIVE', 'PURCHASE', 'OPENING') THEN qty "
        "    WHEN vch_type IN ('ISSUE', 'SALE') THEN -qty "
        "    ELSE 0 END) as current_stock "
        "FROM bardana_transactions "
        "GROUP BY godown_name, bardana_type "
        "ORDER BY godown_name, bardana_type;"
    );
}

QVariantList BardanaController::getPartyBalanceSummary() {
    return DatabaseManager::instance().executeQuery(
        "SELECT "
        "  party_id, "
        "  party_name, "
        "  bardana_type, "
        "  SUM(CASE WHEN dr_cr = 'Dr' THEN qty ELSE 0 END) as issued_bags, "
        "  SUM(CASE WHEN dr_cr = 'Cr' THEN qty ELSE 0 END) as returned_bags, "
        "  SUM(CASE WHEN dr_cr = 'Dr' THEN qty ELSE -qty END) as balance_bags "
        "FROM bardana_transactions "
        "GROUP BY party_id, party_name, bardana_type "
        "HAVING balance_bags != 0 "
        "ORDER BY party_name, bardana_type;"
    );
}

bool BardanaController::deleteBardanaTransaction(int id) {
    bool ok = DatabaseManager::instance().executeNonQuery("DELETE FROM bardana_transactions WHERE id = ?;", {id});
    if (ok) {
        emit bardanaDataChanged();
    }
    return ok;
}
