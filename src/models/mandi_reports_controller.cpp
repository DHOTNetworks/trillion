#include "mandi_reports_controller.h"
#include "../database_manager.h"
#include "../engine/accounting_engine.h"
#include <QLocale>
#include <cmath>
#include <iostream>

MandiReportsController::MandiReportsController(QObject* parent) : QObject(parent) {}

QVariantMap MandiReportsController::get_form_m_return(const QString& fromDate, const QString& toDate) {
    QVariantMap res;
    auto& db = DatabaseManager::instance();

    // Query aggregated commodity data from iform_voucher_items joined with iform_vouchers
    QString query =
        "SELECT "
        "  ivi.item_name, "
        "  SUM(ivi.bags) as total_bags, "
        "  SUM(ivi.weight) as total_weight, "
        "  SUM(ivi.amount) as total_goods_amount, "
        "  AVG(ivi.mandi_fee_rate) as avg_mfee_rate, "
        "  SUM(ivi.mandi_fee_amount) as total_mfee, "
        "  AVG(ivi.hrdf_rate) as avg_hrdf_rate, "
        "  SUM(ivi.hrdf_amount) as total_hrdf "
        "FROM iform_voucher_items ivi "
        "JOIN iform_vouchers iv ON ivi.voucher_id = iv.id ";

    QVariantList params;
    if (!fromDate.isEmpty() && !toDate.isEmpty()) {
        query += "WHERE iv.voucher_date >= ? AND iv.voucher_date <= ? ";
        params << fromDate << toDate;
    }
    query += "GROUP BY ivi.item_name ORDER BY ivi.item_name ASC;";

    QVariantList items = db.executeQuery(query, params);

    double grandWeight = 0.0;
    int grandBags = 0;
    double grandGoodsAmt = 0.0;
    double grandMFee = 0.0;
    double grandHRDF = 0.0;

    for (const auto& it : items) {
        QVariantMap m = it.toMap();
        grandBags += m.value("total_bags").toInt();
        grandWeight += m.value("total_weight").toDouble();
        grandGoodsAmt += m.value("total_goods_amount").toDouble();
        grandMFee += m.value("total_mfee").toDouble();
        grandHRDF += m.value("total_hrdf").toDouble();
    }

    res["from_date"] = fromDate;
    res["to_date"] = toDate;
    res["items"] = items;
    res["total_bags"] = grandBags;
    res["total_weight"] = grandWeight;
    res["total_goods_amount"] = grandGoodsAmt;
    res["total_mandi_fee"] = grandMFee;
    res["total_hrdf"] = grandHRDF;
    res["total_levy"] = grandMFee + grandHRDF;

    return res;
}

QVariantList MandiReportsController::get_jform_register(const QString& fromDate, const QString& toDate, int zimidarId) {
    auto& db = DatabaseManager::instance();
    QString query =
        "SELECT "
        "  jv.id, jv.voucher_no, jv.voucher_date, jv.jform_no, jv.zimidar_id, jv.zimidar_name, "
        "  jv.total_bags, jv.total_weight, jv.goods_amount, jv.labour_amount, jv.round_off, jv.grand_total, jv.narration, "
        "  GROUP_CONCAT(jvi.item_name, ', ') as crop_names "
        "FROM jform_vouchers jv "
        "LEFT JOIN jform_voucher_items jvi ON jv.id = jvi.voucher_id ";

    QStringList where;
    QVariantList params;
    if (!fromDate.isEmpty() && !toDate.isEmpty()) {
        where << "jv.voucher_date >= ? AND jv.voucher_date <= ?";
        params << fromDate << toDate;
    }
    if (zimidarId > 0) {
        where << "jv.zimidar_id = ?";
        params << zimidarId;
    }

    if (!where.isEmpty()) {
        query += "WHERE " + where.join(" AND ") + " ";
    }
    query += "GROUP BY jv.id ORDER BY jv.voucher_date ASC, jv.voucher_no ASC;";

    return db.executeQuery(query, params);
}

QVariantList MandiReportsController::get_iform_register(const QString& fromDate, const QString& toDate, int buyerId) {
    auto& db = DatabaseManager::instance();
    QString query =
        "SELECT "
        "  iv.id, iv.voucher_no, iv.voucher_date, iv.iform_no, iv.buyer_id, iv.buyer_name, iv.broker_name, "
        "  iv.total_bags, iv.total_weight, iv.goods_amount, iv.dami_rate, iv.dami_amount, "
        "  iv.mandi_fee_rate, iv.mandi_fee_amount, iv.hrdf_rate, iv.hrdf_amount, iv.labour_amount, "
        "  iv.round_off, iv.grand_total, iv.narration, "
        "  GROUP_CONCAT(ivi.item_name, ', ') as crop_names "
        "FROM iform_vouchers iv "
        "LEFT JOIN iform_voucher_items ivi ON iv.id = ivi.voucher_id ";

    QStringList where;
    QVariantList params;
    if (!fromDate.isEmpty() && !toDate.isEmpty()) {
        where << "iv.voucher_date >= ? AND iv.voucher_date <= ?";
        params << fromDate << toDate;
    }
    if (buyerId > 0) {
        where << "iv.buyer_id = ?";
        params << buyerId;
    }

    if (!where.isEmpty()) {
        query += "WHERE " + where.join(" AND ") + " ";
    }
    query += "GROUP BY iv.id ORDER BY iv.voucher_date ASC, iv.voucher_no ASC;";

    return db.executeQuery(query, params);
}

QVariantMap MandiReportsController::get_farmer_dheri_statement(int zimidarId, const QString& fromDate, const QString& toDate) {
    QVariantMap res;
    if (zimidarId <= 0) return res;

    auto& db = DatabaseManager::instance();
    QVariant zName = db.executeScalar("SELECT party_name FROM parties WHERE id = ?;", {zimidarId});
    QString farmerName = zName.isValid() ? zName.toString() : "Farmer";
    res["farmer_id"] = zimidarId;
    res["farmer_name"] = farmerName;

    // 1. Dheries / J-Forms delivered by farmer (Credits to Farmer)
    QString jfQuery =
        "SELECT jv.id, jv.voucher_no, jv.voucher_date, jv.jform_no, "
        "  jvi.item_name, jvi.bags, jvi.weight, jvi.rate, jvi.amount, "
        "  jv.labour_amount, jv.grand_total "
        "FROM jform_vouchers jv "
        "JOIN jform_voucher_items jvi ON jv.id = jvi.voucher_id "
        "WHERE jv.zimidar_id = ? ";
    QVariantList jfParams = {zimidarId};
    if (!fromDate.isEmpty() && !toDate.isEmpty()) {
        jfQuery += "AND jv.voucher_date >= ? AND jv.voucher_date <= ? ";
        jfParams << fromDate << toDate;
    }
    jfQuery += "ORDER BY jv.voucher_date ASC, jv.voucher_no ASC;";
    QVariantList dheries = db.executeQuery(jfQuery, jfParams);
    res["dheries"] = dheries;

    // 2. Payments / Vouchers made to farmer (Debits from Farmer)
    QString payQuery =
        "SELECT voucher_no, voucher_date, voucher_type, drcr, amount, narration "
        "FROM vouchers "
        "WHERE ledger_id = ? ";
    QVariantList payParams = {zimidarId};
    if (!fromDate.isEmpty() && !toDate.isEmpty()) {
        payQuery += "AND voucher_date >= ? AND voucher_date <= ? ";
        payParams << fromDate << toDate;
    }
    payQuery += "ORDER BY voucher_date ASC, voucher_no ASC;";
    QVariantList payments = db.executeQuery(payQuery, payParams);
    res["transactions"] = payments;

    // Calculate Totals & Net Balance
    double totalCredits = 0.0;
    double totalDebits = 0.0;
    for (const auto& pVar : payments) {
        QVariantMap p = pVar.toMap();
        QString drcr = p.value("drcr").toString().toUpper();
        double amt = p.value("amount").toDouble();
        if (drcr == "CR") totalCredits += amt;
        else totalDebits += amt;
    }

    double netBalance = totalCredits - totalDebits; // Positive = Payable to Farmer (Cr)
    res["total_credits"] = totalCredits;
    res["total_debits"] = totalDebits;
    res["net_balance"] = std::abs(netBalance);
    res["net_balance_drcr"] = netBalance >= 0.0 ? "Cr" : "Dr";

    return res;
}

QVariantMap MandiReportsController::get_dami_commission_register(const QString& fromDate, const QString& toDate) {
    QVariantMap res;
    auto& db = DatabaseManager::instance();

    QString query =
        "SELECT iv.id, iv.voucher_no, iv.voucher_date, iv.iform_no, iv.buyer_name, iv.broker_name, "
        "  iv.total_bags, iv.total_weight, iv.goods_amount, iv.dami_rate, iv.dami_amount "
        "FROM iform_vouchers iv "
        "WHERE iv.dami_amount > 0 ";
    QVariantList params;
    if (!fromDate.isEmpty() && !toDate.isEmpty()) {
        query += "AND iv.voucher_date >= ? AND iv.voucher_date <= ? ";
        params << fromDate << toDate;
    }
    query += "ORDER BY iv.voucher_date ASC, iv.voucher_no ASC;";

    QVariantList rows = db.executeQuery(query, params);
    double totalDami = 0.0;
    double totalGoods = 0.0;
    double totalWeight = 0.0;

    for (const auto& r : rows) {
        QVariantMap m = r.toMap();
        totalDami += m.value("dami_amount").toDouble();
        totalGoods += m.value("goods_amount").toDouble();
        totalWeight += m.value("total_weight").toDouble();
    }

    res["entries"] = rows;
    res["total_dami"] = totalDami;
    res["total_goods"] = totalGoods;
    res["total_weight"] = totalWeight;

    return res;
}

QVariantList MandiReportsController::get_zimidar_list() {
    auto& db = DatabaseManager::instance();
    return db.executeQuery(
        "SELECT p.id, p.party_name, p.station, p.mobile_no, p.current_balance, p.balance_type "
        "FROM parties p "
        "LEFT JOIN account_groups g ON p.group_id = g.id "
        "WHERE g.group_name LIKE '%Zimidar%' OR g.group_name LIKE '%Farmer%' OR p.party_type = 'Farmer' "
        "ORDER BY p.party_name ASC;"
    );
}

QVariantList MandiReportsController::get_buyer_list() {
    auto& db = DatabaseManager::instance();
    return db.executeQuery(
        "SELECT p.id, p.party_name, p.station, p.gstin, p.current_balance, p.balance_type "
        "FROM parties p "
        "LEFT JOIN account_groups g ON p.group_id = g.id "
        "WHERE g.group_name LIKE '%Sundry Debtors%' OR g.group_name LIKE '%Mandi Debtors%' OR g.group_name LIKE '%Debtors%' "
        "ORDER BY p.party_name ASC;"
    );
}
