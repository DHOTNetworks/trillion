#include "gate_register_controller.h"
#include "../database_manager.h"
#include "../engine/accounting_engine.h"
#include <QDate>
#include <QTime>

GateRegisterController::GateRegisterController(QObject* parent)
    : QObject(parent)
{
}

QString GateRegisterController::generateNextGatePassNo(const QString& prefix) {
    QVariant maxVal = DatabaseManager::instance().executeScalar(
        "SELECT MAX(id) FROM gate_register;"
    );
    int nextId = (maxVal.isValid() && !maxVal.isNull()) ? (maxVal.toInt() + 1) : 1;
    return QString("%1%2").arg(prefix).arg(nextId, 5, 10, QChar('0'));
}

bool GateRegisterController::createGateEntry(
    const QString& gatePassNo,
    const QString& entryDate,
    const QString& entryTime,
    const QString& direction,
    const QString& purpose,
    const QString& vehicleNo,
    const QString& driverName,
    const QString& driverPhone,
    const QString& transporterName,
    int partyId,
    const QString& partyName,
    const QString& commodity,
    int bagCount,
    double grossWeightQtl,
    const QString& remarks
) {
    QString fyLabel = AccountingEngine::getActiveFyLabel();
    if (fyLabel.isEmpty()) fyLabel = AccountingEngine::resolveFinancialYear(entryDate);

    QString passNo = gatePassNo.trimmed().isEmpty() ? generateNextGatePassNo() : gatePassNo;
    QString eTime = entryTime.trimmed().isEmpty() ? QTime::currentTime().toString("HH:mm") : entryTime;

    QVariant pId;
    if (partyId > 0) {
        QVariant chk = DatabaseManager::instance().executeScalar("SELECT id FROM parties WHERE id = ? LIMIT 1;", {partyId});
        if (chk.isValid() && !chk.isNull()) pId = partyId;
    }

    bool ok = DatabaseManager::instance().executeNonQuery(
        "INSERT INTO gate_register ("
        "  financial_year, gate_pass_no, entry_date, entry_time, direction, purpose, "
        "  vehicle_no, driver_name, driver_phone, transporter_name, party_id, party_name, "
        "  commodity, bag_count, gross_weight_qtl, status, remarks"
        ") VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, 'AT_GATE', ?);",
        {
            fyLabel, passNo, entryDate, eTime, direction, purpose,
            vehicleNo, driverName, driverPhone, transporterName, pId, partyName,
            commodity, bagCount, grossWeightQtl, remarks
        }
    );

    if (ok) {
        emit gateDataChanged();
    }
    return ok;
}

bool GateRegisterController::updateWeighbridgeWeights(
    const QString& gatePassNo,
    const QString& weighbridgeSlipNo,
    double grossWeightQtl,
    double tareWeightQtl,
    double netWeightQtl
) {
    bool ok = DatabaseManager::instance().executeNonQuery(
        "UPDATE gate_register "
        "SET weighbridge_slip_no = ?, gross_weight_qtl = ?, tare_weight_qtl = ?, net_weight_qtl = ?, "
        "    status = CASE WHEN ? > 0 AND ? > 0 THEN 'COMPLETED' ELSE 'WEIGHED_GROSS' END "
        "WHERE gate_pass_no = ?;",
        {weighbridgeSlipNo, grossWeightQtl, tareWeightQtl, netWeightQtl, grossWeightQtl, tareWeightQtl, gatePassNo}
    );

    if (ok) {
        emit gateDataChanged();
    }
    return ok;
}

bool GateRegisterController::updateGateStatus(int id, const QString& newStatus, const QString& exitTime) {
    QString eTime = exitTime;
    if (newStatus == "DISPATCHED" && eTime.isEmpty()) {
        eTime = QTime::currentTime().toString("HH:mm");
    }

    bool ok = false;
    if (!eTime.isEmpty()) {
        ok = DatabaseManager::instance().executeNonQuery(
            "UPDATE gate_register SET status = ?, exit_time = ? WHERE id = ?;",
            {newStatus, eTime, id}
        );
    } else {
        ok = DatabaseManager::instance().executeNonQuery(
            "UPDATE gate_register SET status = ? WHERE id = ?;",
            {newStatus, id}
        );
    }

    if (ok) {
        emit gateDataChanged();
    }
    return ok;
}

bool GateRegisterController::linkVoucher(const QString& gatePassNo, const QString& voucherNo) {
    bool ok = DatabaseManager::instance().executeNonQuery(
        "UPDATE gate_register SET linked_voucher_no = ? WHERE gate_pass_no = ?;",
        {voucherNo, gatePassNo}
    );
    if (ok) {
        emit gateDataChanged();
    }
    return ok;
}

QVariantList GateRegisterController::getLiveGateEntries(const QString& filterDirection, const QString& filterStatus) {
    QString sql = "SELECT * FROM gate_register WHERE status != 'DISPATCHED'";
    QVariantList params;
    if (!filterDirection.isEmpty() && filterDirection != "ALL") {
        sql += " AND direction = ?";
        params.append(filterDirection);
    }
    if (!filterStatus.isEmpty() && filterStatus != "ALL") {
        sql += " AND status = ?";
        params.append(filterStatus);
    }
    sql += " ORDER BY entry_date DESC, id DESC;";
    return DatabaseManager::instance().executeQuery(sql, params);
}

QVariantList GateRegisterController::getGateRegisterHistory(const QString& fromDate, const QString& toDate, const QString& direction) {
    QString sql = "SELECT * FROM gate_register WHERE 1=1";
    QVariantList params;
    if (!fromDate.isEmpty() && !toDate.isEmpty()) {
        sql += " AND entry_date >= ? AND entry_date <= ?";
        params.append(fromDate);
        params.append(toDate);
    }
    if (!direction.isEmpty() && direction != "ALL") {
        sql += " AND direction = ?";
        params.append(direction);
    }
    sql += " ORDER BY entry_date DESC, id DESC;";
    return DatabaseManager::instance().executeQuery(sql, params);
}

QVariantMap GateRegisterController::getGateEntryByPassNo(const QString& gatePassNo) {
    QVariantList rows = DatabaseManager::instance().executeQuery(
        "SELECT * FROM gate_register WHERE gate_pass_no = ? LIMIT 1;",
        {gatePassNo}
    );
    return rows.isEmpty() ? QVariantMap() : rows.first().toMap();
}
