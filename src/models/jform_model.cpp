#include "jform_model.h"
#include "../database_manager.h"
#include <QDate>
#include <QLocale>
#include <iostream>

JFormModel::JFormModel(QObject* parent) : QObject(parent) {}

QVariantMap JFormModel::get_next_voucher_info() {
    QVariantMap res;
    auto& db = DatabaseManager::instance();

    QVariant maxVch = db.executeScalar("SELECT MAX(voucher_no) FROM jform_vouchers;");
    int nextVch = maxVch.isValid() && !maxVch.isNull() ? maxVch.toInt() + 1 : 1;

    QVariant maxJForm = db.executeScalar("SELECT MAX(CAST(jform_no AS INTEGER)) FROM jform_vouchers;");
    int nextJForm = maxJForm.isValid() && !maxJForm.isNull() ? maxJForm.toInt() + 1 : 1;

    QDate today = QDate::currentDate();
    res["next_voucher_no"] = nextVch;
    res["next_jform_no"] = QString::number(nextJForm);
    res["date_iso"] = today.toString("yyyy-MM-dd");
    res["date_display"] = today.toString("dd-MM-yyyy");
    res["day_name"] = today.toString("dddd");

    return res;
}

QVariantMap JFormModel::get_zimidar_balance(int zimidarId) {
    QVariantMap res;
    if (zimidarId <= 0) {
        res["formatted_balance"] = "Date Bal. 0.00";
        res["balance"] = 0.0;
        res["drcr"] = "Dr";
        return res;
    }

    auto& db = DatabaseManager::instance();
    QVariant balVar = db.executeScalar(
        "SELECT SUM(CASE WHEN UPPER(drcr) = 'DR' THEN amount ELSE -amount END) "
        "FROM vouchers WHERE ledger_id = ?;",
        {zimidarId}
    );

    double bal = balVar.isValid() && !balVar.isNull() ? balVar.toDouble() : 0.0;
    QString drcr = bal >= 0.0 ? "Dr" : "Cr";
    double absBal = std::abs(bal);

    QLocale locale(QLocale::English, QLocale::India);
    QString numStr = locale.toString(absBal, 'f', 2);
    QString formatted = QString("Date Bal. %1 %2").arg(numStr, drcr);

    res["formatted_balance"] = formatted;
    res["balance"] = absBal;
    res["drcr"] = drcr;
    return res;
}

bool JFormModel::save_jform_voucher(const QVariantMap& data, const QVariantList& items) {
    QString zimidarName = data.value("zimidar_name").toString().trimmed();
    if (zimidarName.isEmpty()) return false;
    if (items.isEmpty()) return false;

    auto& db = DatabaseManager::instance();
    db.beginTransaction();

    int vchNo = data.value("voucher_no").toInt();
    if (vchNo <= 0) {
        QVariant maxVch = db.executeScalar("SELECT MAX(voucher_no) FROM jform_vouchers;");
        vchNo = maxVch.isValid() && !maxVch.isNull() ? maxVch.toInt() + 1 : 1;
    }

    QString jformNo = data.value("jform_no").toString().trimmed();
    if (jformNo.isEmpty()) {
        jformNo = QString::number(vchNo);
    }

    QString vchDate = data.value("voucher_date").toString().trimmed();
    if (vchDate.isEmpty()) vchDate = QDate::currentDate().toString("yyyy-MM-dd");

    int zimidarId = data.value("zimidar_id").toInt();
    if (zimidarId <= 0) {
        QVariant foundId = db.executeScalar("SELECT id FROM parties WHERE party_name = ? LIMIT 1;", {zimidarName});
        if (foundId.isValid() && !foundId.isNull()) {
            zimidarId = foundId.toInt();
        }
    }

    int partyId = data.value("party_id").toInt();
    QString partyName = data.value("party_name").toString().trimmed();
    if (partyName.isEmpty()) partyName = "Self Purchase";
    if (partyId <= 0) {
        QVariant foundPId = db.executeScalar("SELECT id FROM parties WHERE party_name = ? LIMIT 1;", {partyName});
        if (foundPId.isValid() && !foundPId.isNull()) {
            partyId = foundPId.toInt();
        }
    }

    QString auctionSaleStatus = data.value("auction_sale_status").toString().trimmed();
    if (auctionSaleStatus.isEmpty()) auctionSaleStatus = "Zimidara Self Purchase";

    int dueDays = data.value("due_days").toInt();
    int totalBags = data.value("total_bags").toInt();
    double totalWeight = data.value("total_weight").toDouble();
    double goodsAmount = data.value("goods_amount").toDouble();
    double bonusAmount = data.value("bonus_amount").toDouble();
    double reliefAmount = data.value("relief_amount").toDouble();
    double subtotalAmount = data.value("subtotal_amount").toDouble();
    double labourAmount = data.value("labour_amount").toDouble();
    double roundOff = data.value("round_off").toDouble();
    double grandTotal = data.value("grand_total").toDouble();
    QString narration = data.value("narration").toString().trimmed();

    QString vehicleNo = data.value("vehicle_no").toString().trimmed();
    QString driverName = data.value("driver_name").toString().trimmed();
    QString gatePassNo = data.value("gate_pass_no").toString().trimmed();
    QString ewayBillNo = data.value("eway_bill_no").toString().trimmed();
    QString billTime = data.value("bill_time").toString().trimmed();
    QString saudaDate = data.value("sauda_date").toString().trimmed();
    QString mandiPlace = data.value("mandi_place").toString().trimmed();
    QString procurementMode = data.value("procurement_mode").toString().trimmed();
    QString lotNo = data.value("lot_no").toString().trimmed();
    QString grade = data.value("grade").toString().trimmed();
    QString transportName = data.value("transport_name").toString().trimmed();
    QString brokerName = data.value("broker_name").toString().trimmed();
    QString challanNo = data.value("challan_no").toString().trimmed();
    QString kandaWeight = data.value("kanda_weight").toString().trimmed();

    // 1. Insert into jform_vouchers
    bool ok = db.executeNonQuery(
        "INSERT INTO jform_vouchers ("
        "voucher_no, voucher_date, jform_no, zimidar_id, zimidar_name, party_id, party_name, "
        "auction_sale_status, due_days, vehicle_no, driver_name, gate_pass_no, eway_bill_no, "
        "bill_time, sauda_date, mandi_place, procurement_mode, lot_no, grade, transport_name, "
        "broker_name, challan_no, kanda_weight, total_bags, total_weight, goods_amount, bonus_amount, "
        "relief_amount, subtotal_amount, labour_amount, round_off, grand_total, narration"
        ") VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);",
        {
            vchNo, vchDate, jformNo, zimidarId, zimidarName, partyId, partyName,
            auctionSaleStatus, dueDays, vehicleNo, driverName, gatePassNo, ewayBillNo,
            billTime, saudaDate, mandiPlace, procurementMode, lotNo, grade, transportName,
            brokerName, challanNo, kandaWeight, totalBags, totalWeight, goodsAmount, bonusAmount,
            reliefAmount, subtotalAmount, labourAmount, roundOff, grandTotal, narration
        }
    );

    if (!ok) {
        db.rollback();
        return false;
    }

    QVariant newIdVar = db.executeScalar("SELECT last_insert_rowid();");
    int voucherId = newIdVar.toInt();

    // 2. Insert items into jform_voucher_items & stock_transactions
    for (const auto& itemVar : items) {
        QVariantMap it = itemVar.toMap();
        int itemId = it.value("item_id").toInt();
        QString itemName = it.value("item_name").toString().trimmed();
        int bags = it.value("bags").toInt();
        double loose = it.value("loose_weight").toDouble();
        double packing = it.value("packing").toDouble();
        if (packing <= 0.0) packing = 0.500;
        double weight = it.value("weight").toDouble();
        double rate = it.value("rate").toDouble();
        double amount = it.value("amount").toDouble();

        db.executeNonQuery(
            "INSERT INTO jform_voucher_items ("
            "voucher_id, voucher_no, item_id, item_name, bags, loose_weight, packing, weight, rate, amount"
            ") VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?);",
            {voucherId, vchNo, itemId, itemName, bags, loose, packing, weight, rate, amount}
        );

        // Add to stock transactions as IN (Procurement)
        db.executeNonQuery(
            "INSERT INTO stock_transactions ("
            "voucher_no, voucher_date, trans_type, item_id, item_name, bags, weight_qtl, rate, amount, "
            "party_id, party_name, narration"
            ") VALUES (?, ?, 'JFrm', ?, ?, ?, ?, ?, ?, ?, ?, ?);",
            {
                vchNo, vchDate, itemId, itemName, bags, weight, rate, amount,
                zimidarId, zimidarName, "J-Form No. " + jformNo + " - " + narration
            }
        );
    }

    // 3. Insert Double-Entry Bookkeeping Vouchers
    // A. Credit Farmer / Zimidar account with Grand Total payable
    db.executeNonQuery(
        "INSERT INTO vouchers ("
        "voucher_no, voucher_date, voucher_type, ledger_id, party_id, party_name, "
        "amount, drcr, narration"
        ") VALUES (?, ?, 'JFrm', ?, ?, ?, ?, 'Cr', ?);",
        {vchNo, vchDate, zimidarId, zimidarId, zimidarName, grandTotal, "J-Form No. " + jformNo + " - " + narration}
    );

    // B. Debit Purchase / Stock A/c with Goods Amount
    db.executeNonQuery(
        "INSERT INTO vouchers ("
        "voucher_no, voucher_date, voucher_type, ledger_id, party_id, party_name, "
        "amount, drcr, narration"
        ") VALUES (?, ?, 'JFrm', ?, ?, ?, ?, 'Dr', ?);",
        {vchNo, vchDate, partyId, partyId, partyName, goodsAmount, "J-Form No. " + jformNo + " Goods Amount"}
    );

    // C. Credit Labour deduction if > 0
    if (labourAmount > 0.001) {
        QVariant labourLedgerId = db.executeScalar(
            "SELECT id FROM parties WHERE party_name LIKE '%Labour%' OR party_name LIKE '%Majduri%' LIMIT 1;"
        );
        int lId = labourLedgerId.isValid() && !labourLedgerId.isNull() ? labourLedgerId.toInt() : 0;
        db.executeNonQuery(
            "INSERT INTO vouchers ("
            "voucher_no, voucher_date, voucher_type, ledger_id, party_id, party_name, "
            "amount, drcr, narration"
            ") VALUES (?, ?, 'JFrm', ?, ?, 'Labour Charges', ?, 'Cr', ?);",
            {vchNo, vchDate, lId, lId, labourAmount, "Labour deduction on J-Form No. " + jformNo}
        );
    }

    // D. Round Off if any
    if (std::abs(roundOff) > 0.001) {
        QString rDrcr = roundOff > 0 ? "Dr" : "Cr";
        QVariant roundLedgerId = db.executeScalar(
            "SELECT id FROM parties WHERE party_name LIKE '%Round Off%' LIMIT 1;"
        );
        int rId = roundLedgerId.isValid() && !roundLedgerId.isNull() ? roundLedgerId.toInt() : 0;
        db.executeNonQuery(
            "INSERT INTO vouchers ("
            "voucher_no, voucher_date, voucher_type, ledger_id, party_id, party_name, "
            "amount, drcr, narration"
            ") VALUES (?, ?, 'JFrm', ?, ?, 'Round Off', ?, ?, 'Round off on J-Form No. ' || ?);",
            {vchNo, vchDate, rId, rId, std::abs(roundOff), rDrcr, jformNo}
        );
    }

    db.commit();
    emit voucherSaved();
    std::cout << "[SUCCESS] J-Form Voucher #" << vchNo << " saved successfully!" << std::endl;
    return true;
}

QVariantMap JFormModel::get_jform_voucher(int voucherId) {
    QVariantMap res;
    auto& db = DatabaseManager::instance();

    QVariantList rows = db.executeQuery(
        "SELECT * FROM jform_vouchers WHERE id = ? LIMIT 1;",
        {voucherId}
    );
    if (rows.isEmpty()) return res;

    res = rows.first().toMap();
    QVariantList items = db.executeQuery(
        "SELECT * FROM jform_voucher_items WHERE voucher_id = ? ORDER BY id ASC;",
        {voucherId}
    );
    res["items"] = items;
    return res;
}

QVariantList JFormModel::get_jform_register(const QString& fromDate, const QString& toDate) {
    auto& db = DatabaseManager::instance();
    QString sql = "SELECT * FROM jform_vouchers ";
    QVariantList params;
    if (!fromDate.isEmpty() && !toDate.isEmpty()) {
        sql += "WHERE voucher_date >= ? AND voucher_date <= ? ";
        params << fromDate << toDate;
    }
    sql += "ORDER BY voucher_no DESC;";
    return db.executeQuery(sql, params);
}

bool JFormModel::delete_jform_voucher(int voucherId) {
    if (voucherId <= 0) return false;
    auto& db = DatabaseManager::instance();

    QVariant vchNoVar = db.executeScalar("SELECT voucher_no FROM jform_vouchers WHERE id = ?;", {voucherId});
    if (!vchNoVar.isValid()) return false;
    int vchNo = vchNoVar.toInt();

    db.beginTransaction();
    db.executeNonQuery("DELETE FROM jform_vouchers WHERE id = ?;", {voucherId});
    db.executeNonQuery("DELETE FROM jform_voucher_items WHERE voucher_id = ?;", {voucherId});
    db.executeNonQuery("DELETE FROM stock_transactions WHERE trans_type = 'JFrm' AND voucher_no = ?;", {vchNo});
    db.executeNonQuery("DELETE FROM vouchers WHERE voucher_type = 'JFrm' AND voucher_no = ?;", {vchNo});
    db.commit();

    emit voucherDeleted();
    return true;
}
