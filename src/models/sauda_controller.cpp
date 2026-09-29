#include "sauda_controller.h"
#include "../database_manager.h"
#include "../engine/accounting_engine.h"
#include <QDate>
#include <cmath>

SaudaController::SaudaController(QObject* parent)
    : QObject(parent)
{
}

QString SaudaController::generateNextSaudaNo(const QString& prefix) {
    QVariant maxVal = DatabaseManager::instance().executeScalar(
        "SELECT MAX(id) FROM sauda_contracts;"
    );
    int nextId = (maxVal.isValid() && !maxVal.isNull()) ? (maxVal.toInt() + 1) : 1;
    return QString("%1%2").arg(prefix).arg(nextId, 5, 10, QChar('0'));
}

bool SaudaController::createSaudaContract(
    const QString& saudaNo,
    const QString& saudaDate,
    const QString& saudaType,
    int partyId,
    const QString& partyName,
    int brokerId,
    const QString& brokerName,
    int itemId,
    const QString& itemName,
    const QString& grade,
    int contractedBags,
    double contractedWeightQtl,
    double ratePerQtl,
    double dalaliRatePerQtl,
    double dalaliPct,
    const QString& deliveryFrom,
    const QString& deliveryTo,
    const QString& paymentTerms,
    const QString& conditionNotes
) {
    QString fyLabel = AccountingEngine::getActiveFyLabel();
    if (fyLabel.isEmpty()) fyLabel = AccountingEngine::resolveFinancialYear(saudaDate);

    QString sNo = saudaNo.trimmed().isEmpty() ? generateNextSaudaNo() : saudaNo;

    QVariant pId;
    if (partyId > 0) {
        QVariant chk = DatabaseManager::instance().executeScalar("SELECT id FROM parties WHERE id = ? LIMIT 1;", {partyId});
        if (chk.isValid() && !chk.isNull()) pId = partyId;
    }
    QVariant bId;
    if (brokerId > 0) {
        QVariant chk = DatabaseManager::instance().executeScalar("SELECT id FROM parties WHERE id = ? LIMIT 1;", {brokerId});
        if (chk.isValid() && !chk.isNull()) bId = brokerId;
    }
    QVariant iId;
    if (itemId > 0) {
        QVariant chk = DatabaseManager::instance().executeScalar("SELECT id FROM stock_items WHERE id = ? LIMIT 1;", {itemId});
        if (chk.isValid() && !chk.isNull()) iId = itemId;
    }

    bool ok = DatabaseManager::instance().executeNonQuery(
        "INSERT INTO sauda_contracts ("
        "  financial_year, sauda_no, sauda_date, sauda_type, party_id, party_name, "
        "  broker_id, broker_name, item_id, item_name, grade, contracted_bags, "
        "  contracted_weight_qtl, rate_per_qtl, dalali_rate_per_qtl, dalali_pct, "
        "  delivery_from, delivery_to, payment_terms, condition_notes, status"
        ") VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, 'PENDING');",
        {
            fyLabel, sNo, saudaDate, saudaType, pId, partyName,
            bId, brokerName, iId, itemName, grade, contractedBags,
            contractedWeightQtl, ratePerQtl, dalaliRatePerQtl, dalaliPct,
            deliveryFrom, deliveryTo, paymentTerms, conditionNotes
        }
    );

    if (ok) {
        emit saudaDataChanged();
    }
    return ok;
}

bool SaudaController::fulfillSauda(int saudaId, int bags, double weightQtl) {
    QVariantMap sauda = getSaudaById(saudaId);
    if (sauda.isEmpty()) return false;

    double curWeight = sauda.value("fulfilled_weight_qtl").toDouble() + weightQtl;
    int curBags = sauda.value("fulfilled_bags").toInt() + bags;
    double totalWeight = sauda.value("contracted_weight_qtl").toDouble();

    QString newStatus = (curWeight >= totalWeight && totalWeight > 0) ? "FULFILLED" : "PARTIAL";

    bool ok = DatabaseManager::instance().executeNonQuery(
        "UPDATE sauda_contracts "
        "SET fulfilled_weight_qtl = ?, fulfilled_bags = ?, status = ? "
        "WHERE id = ?;",
        {curWeight, curBags, newStatus, saudaId}
    );

    if (ok) {
        emit saudaDataChanged();
    }
    return ok;
}

bool SaudaController::createDalaliSettlement(
    int brokerId,
    const QString& brokerName,
    int saudaId,
    const QString& saudaNo,
    const QString& settlementDate,
    const QString& voucherType,
    const QString& voucherNo,
    const QString& invoiceNo,
    const QString& partyName,
    const QString& itemName,
    double weightQtl,
    double ratePerQtl,
    double dalaliRate,
    double tdsPct,
    const QString& narration
) {
    QString fyLabel = AccountingEngine::getActiveFyLabel();
    if (fyLabel.isEmpty()) fyLabel = AccountingEngine::resolveFinancialYear(settlementDate);

    QVariant maxVal = DatabaseManager::instance().executeScalar("SELECT MAX(id) FROM dalali_settlements;");
    int nextId = (maxVal.isValid() && !maxVal.isNull()) ? (maxVal.toInt() + 1) : 1;
    QString settNo = QString("DL-%1").arg(nextId, 5, 10, QChar('0'));

    double dalaliAmount = std::round((weightQtl * dalaliRate) * 100.0) / 100.0;
    if (dalaliAmount == 0.0 && ratePerQtl > 0) {
        // If dalali rate was percentage
        dalaliAmount = std::round(((weightQtl * ratePerQtl) * (dalaliRate / 100.0)) * 100.0) / 100.0;
    }

    double tdsAmount = std::round((dalaliAmount * (tdsPct / 100.0)) * 100.0) / 100.0;
    double netDalali = dalaliAmount - tdsAmount;

    QVariant bId;
    if (brokerId > 0) {
        QVariant chk = DatabaseManager::instance().executeScalar("SELECT id FROM parties WHERE id = ? LIMIT 1;", {brokerId});
        if (chk.isValid() && !chk.isNull()) bId = brokerId;
    }
    QVariant sId;
    if (saudaId > 0) {
        QVariant chk = DatabaseManager::instance().executeScalar("SELECT id FROM sauda_contracts WHERE id = ? LIMIT 1;", {saudaId});
        if (chk.isValid() && !chk.isNull()) sId = saudaId;
    }

    bool ok = DatabaseManager::instance().executeNonQuery(
        "INSERT INTO dalali_settlements ("
        "  financial_year, settlement_no, settlement_date, broker_id, broker_name, "
        "  sauda_id, sauda_no, voucher_type, voucher_no, invoice_no, party_name, "
        "  item_name, weight_qtl, rate_per_qtl, dalali_rate, dalali_amount, "
        "  tds_pct, tds_amount, net_dalali_payable, is_posted_to_jv, narration"
        ") VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, 0, ?);",
        {
            fyLabel, settNo, settlementDate, bId, brokerName,
            sId, saudaNo, voucherType, voucherNo, invoiceNo, partyName,
            itemName, weightQtl, ratePerQtl, dalaliRate, dalaliAmount,
            tdsPct, tdsAmount, netDalali, narration
        }
    );

    if (ok) {
        emit dalaliDataChanged();
    }
    return ok;
}

bool SaudaController::postDalaliToJournalVoucher(int settlementId) {
    QVariantList rows = DatabaseManager::instance().executeQuery(
        "SELECT * FROM dalali_settlements WHERE id = ? LIMIT 1;", {settlementId}
    );
    if (rows.isEmpty()) return false;
    QVariantMap sett = rows.first().toMap();

    if (sett.value("is_posted_to_jv").toInt() == 1) return true; // Already posted

    QString fyLabel = sett.value("financial_year").toString();
    QString settDate = sett.value("settlement_date").toString();
    QString brokerName = sett.value("broker_name").toString();
    int brokerId = sett.value("broker_id").toInt();
    double dalaliAmt = sett.value("dalali_amount").toDouble();
    double tdsAmt = sett.value("tds_amount").toDouble();
    double netPayable = sett.value("net_dalali_payable").toDouble();
    QString settNo = sett.value("settlement_no").toString();

    // Generate Journal Voucher Sequence
    QVariant maxVch = DatabaseManager::instance().executeScalar(
        "SELECT MAX(id) FROM vouchers WHERE voucher_type = 'Journal';"
    );
    int nextVchNo = (maxVch.isValid() && !maxVch.isNull()) ? (maxVch.toInt() + 1) : 1;
    QString vchNoStr = QString("JV-%1").arg(nextVchNo);

    QVariant brokerPartyId;
    if (brokerId > 0) {
        QVariant chk = DatabaseManager::instance().executeScalar("SELECT id FROM parties WHERE id = ? LIMIT 1;", {brokerId});
        if (chk.isValid() && !chk.isNull()) brokerPartyId = brokerId;
    }

    DatabaseManager::instance().beginTransaction();

    // 1. Insert Voucher Header
    DatabaseManager::instance().executeNonQuery(
        "INSERT INTO vouchers ("
        "  financial_year, voucher_no, voucher_date, voucher_type, legacy_type, "
        "  party_id, party_name, account_type, amount, narration"
        ") VALUES (?, ?, ?, 'Journal', 'JV', ?, ?, 'Brokerage Expense', ?, ?);",
        {fyLabel, vchNoStr, settDate, brokerPartyId, brokerName, dalaliAmt,
         QString("Brokerage settled against %1 for party %2").arg(settNo, sett.value("party_name").toString())}
    );

    // 2. Double-Entry Postings in transactions
    // Line 1: Debit Dalali / Brokerage Expense A/c
    DatabaseManager::instance().executeNonQuery(
        "INSERT INTO transactions ("
        "  financial_year, voucher_no, voucher_date, voucher_type, trans_type, "
        "  party_name, opposing_account, dr_cr, amount, narration, row_no"
        ") VALUES (?, ?, ?, 'Journal', 'JV', 'Dalali Expense A/c', ?, 'Dr', ?, ?, 1);",
        {fyLabel, vchNoStr, settDate, brokerName, dalaliAmt, QString("Dalali expense against %1").arg(settNo)}
    );

    // Line 2: Credit Broker Personal A/c (Net Payable)
    DatabaseManager::instance().executeNonQuery(
        "INSERT INTO transactions ("
        "  financial_year, voucher_no, voucher_date, voucher_type, trans_type, "
        "  party_id, party_name, opposing_account, dr_cr, amount, narration, row_no"
        ") VALUES (?, ?, ?, 'Journal', 'JV', ?, ?, 'Dalali Expense A/c', 'Cr', ?, ?, 2);",
        {fyLabel, vchNoStr, settDate, brokerPartyId, brokerName, netPayable, QString("Dalali credit against %1").arg(settNo)}
    );

    // Line 3: Credit TDS Payable 194H A/c (if applicable)
    if (tdsAmt > 0) {
        DatabaseManager::instance().executeNonQuery(
            "INSERT INTO transactions ("
            "  financial_year, voucher_no, voucher_date, voucher_type, trans_type, "
            "  party_name, opposing_account, dr_cr, amount, narration, tds_amount, tds_rate, row_no"
            ") VALUES (?, ?, ?, 'Journal', 'JV', 'TDS Payable 194H A/c', ?, 'Cr', ?, ?, ?, ?, 3);",
            {fyLabel, vchNoStr, settDate, brokerName, tdsAmt, QString("TDS 194H on Dalali %1").arg(settNo), tdsAmt, sett.value("tds_pct").toDouble()}
        );
    }

    // 3. Mark Dalali Settlement as Posted
    DatabaseManager::instance().executeNonQuery(
        "UPDATE dalali_settlements "
        "SET is_posted_to_jv = 1, journal_voucher_no = ? "
        "WHERE id = ?;",
        {vchNoStr, settlementId}
    );

    DatabaseManager::instance().commit();

    emit dalaliDataChanged();
    return true;
}

QVariantList SaudaController::getSaudaContracts(const QString& saudaType, const QString& status) {
    QString sql = "SELECT * FROM sauda_contracts WHERE 1=1";
    QVariantList params;
    if (!saudaType.isEmpty() && saudaType != "ALL") {
        sql += " AND sauda_type = ?";
        params.append(saudaType);
    }
    if (!status.isEmpty() && status != "ALL") {
        sql += " AND status = ?";
        params.append(status);
    }
    sql += " ORDER BY sauda_date DESC, id DESC;";
    return DatabaseManager::instance().executeQuery(sql, params);
}

QVariantList SaudaController::getDalaliSettlements(int brokerId, const QString& fromDate, const QString& toDate) {
    QString sql = "SELECT * FROM dalali_settlements WHERE 1=1";
    QVariantList params;
    if (brokerId > 0) {
        sql += " AND broker_id = ?";
        params.append(brokerId);
    }
    if (!fromDate.isEmpty() && !toDate.isEmpty()) {
        sql += " AND settlement_date >= ? AND settlement_date <= ?";
        params.append(fromDate);
        params.append(toDate);
    }
    sql += " ORDER BY settlement_date DESC, id DESC;";
    return DatabaseManager::instance().executeQuery(sql, params);
}

QVariantMap SaudaController::getSaudaById(int id) {
    QVariantList rows = DatabaseManager::instance().executeQuery(
        "SELECT * FROM sauda_contracts WHERE id = ? LIMIT 1;", {id}
    );
    return rows.isEmpty() ? QVariantMap() : rows.first().toMap();
}
