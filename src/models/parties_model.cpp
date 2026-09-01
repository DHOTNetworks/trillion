#include "parties_model.h"
#include "../database_manager.h"
#include "../engine/accounting_engine.h"

PartiesModel::PartiesModel(QObject* parent)
    : BaseTableModel(
        {"Party Name", "Group", "Station", "Mobile", "GSTIN", "Op Bal", "Bal Type"},
        {"party_name", "group_name", "station", "mobile", "gstin", "opening_balance", "balance_type"},
        parent
    )
{
    reload_data();
}

void PartiesModel::reload_data() {
    beginResetModel();
    m_data = DatabaseManager::instance().executeQuery("SELECT * FROM parties ORDER BY party_name ASC;");
    endResetModel();
    emit dataChangedSignal();
    emit countChanged();
}

QVariantMap PartiesModel::get_party_by_name(const QString& name) const {
    QString cleanName = name.trimmed();
    QVariantList rows = DatabaseManager::instance().executeQuery(
        "SELECT * FROM parties WHERE party_name = ? LIMIT 1;",
        {cleanName}
    );
    if (!rows.isEmpty()) {
        return rows.first().toMap();
    }
    return QVariantMap();
}

bool PartiesModel::add_party(const QString& name, const QString& group, const QString& station, const QString& mobile, const QString& gstin, double op_bal, const QString& bal_type, const QString& address) {
    bool ok = DatabaseManager::instance().executeNonQuery(
        "INSERT INTO parties (party_name, group_name, station, mobile, gstin, opening_balance, balance_type, address) "
        "VALUES (?, ?, ?, ?, ?, ?, ?, ?);",
        {name, group, station, mobile, gstin, op_bal, bal_type, address}
    );
    if (ok) reload_data();
    return ok;
}

QVariantMap PartiesModel::get_ledger_statement(const QString& partyName, const QString& fromDate, const QString& toDate, const QString& fy) {
    QVariantMap result;
    QVariantMap party = get_party_by_name(partyName);
    double opBal = party.value("opening_balance").toDouble();
    QString balType = party.value("balance_type").toString();

    QString sql = "SELECT voucher_no, instrument_no, voucher_date, voucher_type, legacy_type, party_name, account_type, amount, narration "
                  "FROM vouchers WHERE (party_name = ? OR account_type = ?) ";
    QVariantList params = {partyName, partyName};

    if (!fromDate.isEmpty() && !toDate.isEmpty()) {
        sql += "AND voucher_date >= ? AND voucher_date <= ? ";
        params << fromDate << toDate;
    } else if (!fy.isEmpty()) {
        sql += "AND financial_year = ? ";
        params << fy;
    }
    sql += "ORDER BY voucher_date ASC, id ASC;";

    QVariantList vchRows = DatabaseManager::instance().executeQuery(sql, params);
    QVariantList drRows, crRows;
    double totalDr = 0.0, totalCr = 0.0;

    for (const QVariant& r : vchRows) {
        QVariantMap v = r.toMap();
        QString vType = v.value("voucher_type").toString();
        QString pName = v.value("party_name").toString();
        double amt = v.value("amount").toDouble();

        if (vType == "Sales" || (vType == "ChPt" && pName == partyName)) {
            totalDr += amt;
            v["amount_fmt"] = AccountingEngine::formatIndianCurrency(amt);
            drRows.append(v);
        } else {
            totalCr += amt;
            v["amount_fmt"] = AccountingEngine::formatIndianCurrency(amt);
            crRows.append(v);
        }
    }

    result["party_name"] = partyName;
    result["opening_balance"] = opBal;
    result["balance_type"] = balType;
    result["total_dr"] = totalDr;
    result["total_cr"] = totalCr;
    result["dr_entries"] = drRows;
    result["cr_entries"] = crRows;
    return result;
}
