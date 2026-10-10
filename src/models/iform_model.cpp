#include "iform_model.h"
#include "../database_manager.h"
#include "../engine/accounting_engine.h"
#include "../engine/fiscal_year_helper.h"
#include <QDate>
#include <QLocale>
#include <cmath>
#include <iostream>

IFormModel::IFormModel(QObject* parent) : QObject(parent) {}

QVariantMap IFormModel::get_next_voucher_info(const QString& fy) {
    QVariantMap res;
    auto& db = DatabaseManager::instance();

    QString targetFy = AccountingEngine::resolveFinancialYear(fy);
    QString fyPattern = "%" + targetFy.mid(3).trimmed() + "%";

    QVariant maxVch = db.executeScalar(
        "SELECT MAX(voucher_no) FROM iform_vouchers WHERE financial_year = ? OR financial_year LIKE ?;",
        {targetFy, fyPattern}
    );
    int nextVch = maxVch.isValid() && !maxVch.isNull() ? maxVch.toInt() + 1 : 1;

    QVariant maxIForm = db.executeScalar(
        "SELECT MAX(CAST(iform_no AS INTEGER)) FROM iform_vouchers WHERE financial_year = ? OR financial_year LIKE ?;",
        {targetFy, fyPattern}
    );
    int nextIForm = maxIForm.isValid() && !maxIForm.isNull() ? maxIForm.toInt() + 1 : 1;

    QDate today = QDate::currentDate();
    res["next_voucher_no"] = nextVch;
    res["next_iform_no"] = QString::number(nextIForm);
    res["date_iso"] = today.toString("yyyy-MM-dd");
    res["date_display"] = today.toString("dd-MM-yyyy");
    res["day_name"] = today.toString("dddd");

    return res;
}

QVariantMap IFormModel::get_buyer_balance(int buyerId) {
    QVariantMap res;
    if (buyerId <= 0) {
        res["formatted_balance"] = "Date Bal. 0.00";
        res["balance"] = 0.0;
        res["drcr"] = "Dr";
        return res;
    }

    auto& db = DatabaseManager::instance();
    QVariant balVar = db.executeScalar(
        "SELECT SUM(CASE WHEN UPPER(drcr) = 'DR' THEN amount ELSE -amount END) "
        "FROM vouchers WHERE ledger_id = ?;",
        {buyerId}
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

bool IFormModel::save_iform_voucher(const QVariantMap& data, const QVariantList& items) {
    QString buyerName = data.value("buyer_name").toString().trimmed();
    if (buyerName.isEmpty()) return false;
    if (items.isEmpty()) return false;

    auto& db = DatabaseManager::instance();
    db.beginTransaction();

    QString vchDate = data.value("voucher_date").toString().trimmed();
    if (vchDate.isEmpty()) vchDate = QDate::currentDate().toString("yyyy-MM-dd");
    QString fy = AccountingEngine::resolveFinancialYear(vchDate);
    QString fyPattern = "%" + fy.mid(3).trimmed() + "%";

    int vchNo = data.value("voucher_no").toInt();
    if (vchNo <= 0) {
        QVariant maxVch = db.executeScalar(
            "SELECT MAX(voucher_no) FROM iform_vouchers WHERE financial_year = ? OR financial_year LIKE ?;",
            {fy, fyPattern}
        );
        vchNo = maxVch.isValid() && !maxVch.isNull() ? maxVch.toInt() + 1 : 1;
    }

    QString iformNo = data.value("iform_no").toString().trimmed();
    if (iformNo.isEmpty()) {
        iformNo = QString::number(vchNo);
    }

    int buyerId = data.value("buyer_id").toInt();
    if (buyerId <= 0) {
        QVariant foundId = db.executeScalar("SELECT id FROM parties WHERE party_name = ? LIMIT 1;", {buyerName});
        if (foundId.isValid() && !foundId.isNull()) {
            buyerId = foundId.toInt();
        }
    }

    QString brokerName = data.value("broker_name").toString().trimmed();
    int dueDays = data.value("due_days").toInt();
    QString vehicleNo = data.value("vehicle_no").toString().trimmed();
    QString driverName = data.value("driver_name").toString().trimmed();
    QString grNo = data.value("gr_no").toString().trimmed();
    QString gatePassNo = data.value("gate_pass_no").toString().trimmed();

    int totalBags = data.value("total_bags").toInt();
    double totalWeight = data.value("total_weight").toDouble();
    double goodsAmount = data.value("goods_amount").toDouble();
    double damiRate = data.value("dami_rate", 2.5).toDouble();
    double damiAmount = data.value("dami_amount").toDouble();
    double mandiFeeRate = data.value("mandi_fee_rate", 2.0).toDouble();
    double mandiFeeAmount = data.value("mandi_fee_amount").toDouble();
    double hrdfRate = data.value("hrdf_rate", 0.5).toDouble();
    double hrdfAmount = data.value("hrdf_amount").toDouble();
    double labourAmount = data.value("labour_amount").toDouble();
    double taxableAmount = data.value("taxable_amount").toDouble();
    double taxAmount = data.value("tax_amount").toDouble();
    double roundOff = data.value("round_off").toDouble();
    double grandTotal = data.value("grand_total").toDouble();
    QString narration = data.value("narration").toString().trimmed();

    int editId = data.value("id", 0).toInt();
    int voucherId = 0;

    if (editId > 0) {
        voucherId = editId;
        bool ok = db.executeNonQuery(
            "UPDATE iform_vouchers SET "
            "voucher_no = ?, voucher_date = ?, financial_year = ?, iform_no = ?, buyer_id = ?, buyer_name = ?, "
            "broker_name = ?, due_days = ?, vehicle_no = ?, driver_name = ?, gr_no = ?, gate_pass_no = ?, "
            "total_bags = ?, total_weight = ?, goods_amount = ?, dami_rate = ?, dami_amount = ?, "
            "mandi_fee_rate = ?, mandi_fee_amount = ?, hrdf_rate = ?, hrdf_amount = ?, labour_amount = ?, "
            "taxable_amount = ?, tax_amount = ?, round_off = ?, grand_total = ?, narration = ? "
            "WHERE id = ?;",
            {
                vchNo, vchDate, fy, iformNo, (buyerId > 0 ? QVariant(buyerId) : QVariant()), buyerName,
                brokerName, dueDays, vehicleNo, driverName, grNo, gatePassNo,
                totalBags, totalWeight, goodsAmount, damiRate, damiAmount,
                mandiFeeRate, mandiFeeAmount, hrdfRate, hrdfAmount, labourAmount,
                taxableAmount, taxAmount, roundOff, grandTotal, narration, voucherId
            }
        );
        if (!ok) {
            db.rollback();
            return false;
        }
        db.executeNonQuery("DELETE FROM iform_voucher_items WHERE voucher_id = ?;", {voucherId});
        db.executeNonQuery("DELETE FROM vouchers WHERE voucher_type = 'IFrm' AND voucher_no = ? AND voucher_date = ?;", {vchNo, vchDate});
    } else {
        bool ok = db.executeNonQuery(
            "INSERT INTO iform_vouchers ("
            "financial_year, voucher_no, voucher_date, iform_no, buyer_id, buyer_name, "
            "broker_name, due_days, vehicle_no, driver_name, gr_no, gate_pass_no, "
            "total_bags, total_weight, goods_amount, dami_rate, dami_amount, "
            "mandi_fee_rate, mandi_fee_amount, hrdf_rate, hrdf_amount, labour_amount, "
            "taxable_amount, tax_amount, round_off, grand_total, narration) "
            "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);",
            {
                fy, vchNo, vchDate, iformNo, (buyerId > 0 ? QVariant(buyerId) : QVariant()), buyerName,
                brokerName, dueDays, vehicleNo, driverName, grNo, gatePassNo,
                totalBags, totalWeight, goodsAmount, damiRate, damiAmount,
                mandiFeeRate, mandiFeeAmount, hrdfRate, hrdfAmount, labourAmount,
                taxableAmount, taxAmount, roundOff, grandTotal, narration
            }
        );
        if (!ok) {
            db.rollback();
            return false;
        }
        voucherId = db.executeScalar("SELECT last_insert_rowid();").toInt();
    }

    // Insert line items
    for (const auto& itemVar : items) {
        QVariantMap item = itemVar.toMap();
        QString itemName = item.value("item_name").toString().trimmed();
        int itemId = item.value("item_id").toInt();
        if (itemId <= 0 && !itemName.isEmpty()) {
            QVariant foundItemId = db.executeScalar("SELECT id FROM stock_items WHERE item_name = ? LIMIT 1;", {itemName});
            if (foundItemId.isValid() && !foundItemId.isNull()) itemId = foundItemId.toInt();
        }

        int bags = item.value("bags").toInt();
        double loose = item.value("loose_weight").toDouble();
        double packing = item.value("packing", 0.500).toDouble();
        double wt = item.value("weight").toDouble();
        double rate = item.value("rate").toDouble();
        double amt = item.value("amount").toDouble();
        double lineDami = item.value("dami_amount").toDouble();
        double lineMFee = item.value("mandi_fee_amount").toDouble();
        double lineHRDF = item.value("hrdf_amount").toDouble();
        double lineLabour = item.value("labour_amount").toDouble();
        double lineTaxRate = item.value("tax_rate").toDouble();
        double lineTaxAmt = item.value("tax_amount").toDouble();

        db.executeNonQuery(
            "INSERT INTO iform_voucher_items ("
            "voucher_id, voucher_no, item_id, item_name, bags, loose_weight, packing, weight, rate, amount, "
            "dami_rate, dami_amount, mandi_fee_rate, mandi_fee_amount, hrdf_rate, hrdf_amount, labour_amount, tax_rate, tax_amount) "
            "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);",
            {
                voucherId, vchNo, (itemId > 0 ? QVariant(itemId) : QVariant()), itemName,
                bags, loose, packing, wt, rate, amt,
                damiRate, lineDami, mandiFeeRate, lineMFee, hrdfRate, lineHRDF, lineLabour, lineTaxRate, lineTaxAmt
            }
        );
    }

    // Double Entry Postings:
    // 1. Debit Buyer Account with Grand Total
    if (buyerId > 0 && grandTotal > 0.001) {
        db.executeNonQuery(
            "INSERT INTO vouchers (financial_year, voucher_no, voucher_date, voucher_type, ledger_id, ledger_name, drcr, amount, narration) "
            "VALUES (?, ?, ?, 'IFrm', ?, ?, 'Dr', ?, ?);",
            {fy, vchNo, vchDate, buyerId, buyerName, grandTotal, narration.isEmpty() ? QString("I-Form No. %1").arg(iformNo) : narration}
        );
    }

    // 2. Credit Maal Khata / Sale with Goods Amount
    QVariant maalKhataId = db.executeScalar("SELECT id FROM parties WHERE party_name = 'Maal Khata A/c' OR party_name LIKE '%Maal Khata%' LIMIT 1;");
    if (maalKhataId.isValid() && !maalKhataId.isNull() && goodsAmount > 0.001) {
        db.executeNonQuery(
            "INSERT INTO vouchers (financial_year, voucher_no, voucher_date, voucher_type, ledger_id, ledger_name, drcr, amount, narration) "
            "VALUES (?, ?, ?, 'IFrm', ?, 'Maal Khata A/c', 'Cr', ?, ?);",
            {fy, vchNo, vchDate, maalKhataId.toInt(), goodsAmount, QString("I-Form No. %1 - %2").arg(iformNo, buyerName)}
        );
    }

    // 3. Credit Dami A/c
    if (damiAmount > 0.001) {
        QVariant damiId = db.executeScalar("SELECT id FROM parties WHERE party_name = 'Dami A/c' OR party_name LIKE '%Commission%' LIMIT 1;");
        if (damiId.isValid() && !damiId.isNull()) {
            db.executeNonQuery(
                "INSERT INTO vouchers (financial_year, voucher_no, voucher_date, voucher_type, ledger_id, ledger_name, drcr, amount, narration) "
                "VALUES (?, ?, ?, 'IFrm', ?, 'Dami A/c', 'Cr', ?, ?);",
                {fy, vchNo, vchDate, damiId.toInt(), damiAmount, QString("Dami on I-Form %1").arg(iformNo)}
            );
        }
    }

    // 4. Credit Market Fee A/c
    if (mandiFeeAmount > 0.001) {
        QVariant mfId = db.executeScalar("SELECT id FROM parties WHERE party_name = 'Market Fee A/c' OR party_name LIKE '%Market Fee%' LIMIT 1;");
        if (mfId.isValid() && !mfId.isNull()) {
            db.executeNonQuery(
                "INSERT INTO vouchers (financial_year, voucher_no, voucher_date, voucher_type, ledger_id, ledger_name, drcr, amount, narration) "
                "VALUES (?, ?, ?, 'IFrm', ?, 'Market Fee A/c', 'Cr', ?, ?);",
                {fy, vchNo, vchDate, mfId.toInt(), mandiFeeAmount, QString("Market Fee on I-Form %1").arg(iformNo)}
            );
        }
    }

    // 5. Credit H.R.D.F. A/c
    if (hrdfAmount > 0.001) {
        QVariant hrdfId = db.executeScalar("SELECT id FROM parties WHERE party_name = 'H.R.D.F. A/c' OR party_name LIKE '%HRDF%' LIMIT 1;");
        if (hrdfId.isValid() && !hrdfId.isNull()) {
            db.executeNonQuery(
                "INSERT INTO vouchers (financial_year, voucher_no, voucher_date, voucher_type, ledger_id, ledger_name, drcr, amount, narration) "
                "VALUES (?, ?, ?, 'IFrm', ?, 'H.R.D.F. A/c', 'Cr', ?, ?);",
                {fy, vchNo, vchDate, hrdfId.toInt(), hrdfAmount, QString("HRDF on I-Form %1").arg(iformNo)}
            );
        }
    }

    // 6. Credit Labour A/c
    if (labourAmount > 0.001) {
        QVariant labourId = db.executeScalar("SELECT id FROM parties WHERE party_name = 'Labour A/c' OR party_name LIKE '%Labour%' LIMIT 1;");
        if (labourId.isValid() && !labourId.isNull()) {
            db.executeNonQuery(
                "INSERT INTO vouchers (financial_year, voucher_no, voucher_date, voucher_type, ledger_id, ledger_name, drcr, amount, narration) "
                "VALUES (?, ?, ?, 'IFrm', ?, 'Labour A/c', 'Cr', ?, ?);",
                {fy, vchNo, vchDate, labourId.toInt(), labourAmount, QString("Labour on I-Form %1").arg(iformNo)}
            );
        }
    }

    db.commit();
    emit voucherSaved();
    return true;
}

QVariantMap IFormModel::get_iform_voucher(const QVariant& voucherIdOrNo) {
    QVariantMap res;
    auto& db = DatabaseManager::instance();

    int targetId = 0;
    QString vNo;
    QString ifNo;
    QString vDate;
    QString fy;

    if (voucherIdOrNo.typeId() == QMetaType::QVariantMap) {
        QVariantMap m = voucherIdOrNo.toMap();
        targetId = m.value("id").toInt();
        vNo = m.value("voucher_no", m.value("voucherNo")).toString().trimmed();
        ifNo = m.value("iform_no", m.value("iformNo", m.value("refNo", m.value("invoice_no")))).toString().trimmed();
        vDate = FiscalYearHelper::normalizeToIso(m.value("vIso", m.value("voucher_date", m.value("date"))).toString());
        fy = m.value("financial_year", m.value("financialYear")).toString().trimmed();
    } else {
        bool isNum = false;
        int parsedId = voucherIdOrNo.toInt(&isNum);
        if (isNum && parsedId > 0) {
            targetId = parsedId;
            vNo = QString::number(parsedId);
        } else {
            vNo = voucherIdOrNo.toString().trimmed();
        }
    }

    static const QRegularExpression prefixRe(QStringLiteral("^(Sale|Sales|Purc|Purchase|Pur|Jrnl|Journal|ChPt|ChRt|Pymt|Rcpt|TDS|IFrm|I-Form|IForm|IF)[-\\s#]*"), QRegularExpression::CaseInsensitiveOption);
    QString cleanVNo = vNo;
    cleanVNo = cleanVNo.remove(prefixRe).trimmed();
    if (cleanVNo.isEmpty()) cleanVNo = vNo;
    if (ifNo.isEmpty()) ifNo = cleanVNo;

    // 1. Direct search in iform_vouchers
    QVariantList rows;
    if (targetId > 0) {
        rows = db.executeQuery("SELECT * FROM iform_vouchers WHERE id = ? LIMIT 1;", {targetId});
    }
    if (rows.isEmpty() && (!vNo.isEmpty() || !cleanVNo.isEmpty() || !ifNo.isEmpty())) {
        rows = db.executeQuery(
            "SELECT * FROM iform_vouchers WHERE voucher_no = ? OR voucher_no = ? OR iform_no = ? OR iform_no = ? LIMIT 1;",
            {vNo, cleanVNo, ifNo, cleanVNo}
        );
    }
    if (rows.isEmpty() && !vDate.isEmpty() && (!vNo.isEmpty() || !cleanVNo.isEmpty())) {
        rows = db.executeQuery(
            "SELECT * FROM iform_vouchers WHERE (voucher_no = ? OR voucher_no = ?) AND voucher_date = ? LIMIT 1;",
            {vNo, cleanVNo, vDate}
        );
    }

    if (!rows.isEmpty()) {
        res = rows.first().toMap();
        int actualVoucherId = res.value("id").toInt();
        int vNumber = res.value("voucher_no").toInt();
        QVariantList itemRows = db.executeQuery(
            "SELECT * FROM iform_voucher_items WHERE voucher_id = ? OR voucher_no = ? ORDER BY id ASC;",
            {actualVoucherId, vNumber}
        );
        if (itemRows.isEmpty() && vNumber > 0) {
            QVariantList stRows = db.executeQuery(
                "SELECT * FROM stock_transactions WHERE (voucher_no = ? OR voucher_no = ?) AND trans_type IN ('IFrm', 'I-Form', 'IForm') ORDER BY id ASC;",
                {vNumber, QString::number(vNumber)}
            );
            for (const auto& stVar : stRows) {
                QVariantMap st = stVar.toMap();
                QVariantMap it;
                it["item_id"] = st.value("item_id");
                it["item_name"] = st.value("item_name");
                it["bags"] = st.value("bags");
                it["packing"] = 0.500;
                it["loose_weight"] = 0.0;
                it["weight"] = st.value("weight_qtl");
                it["rate"] = st.value("rate");
                it["amount"] = st.value("amount");
                itemRows.append(it);
            }
        }
        res["items"] = itemRows;
        return res;
    }

    // 2. Fallback to sales_invoices (Mandi I-Forms stored as sales invoices)
    QVariantList siRows;
    if (targetId > 0) {
        siRows = db.executeQuery("SELECT * FROM sales_invoices WHERE id = ? LIMIT 1;", {targetId});
    }
    if (siRows.isEmpty() && (!cleanVNo.isEmpty() || !vNo.isEmpty() || !ifNo.isEmpty())) {
        siRows = db.executeQuery(
            "SELECT * FROM sales_invoices WHERE invoice_no = ? OR invoice_no = ? OR voucher_no = ? OR voucher_no = ? OR invoice_no = ? LIMIT 1;",
            {vNo, cleanVNo, vNo, cleanVNo, ifNo}
        );
    }
    if (!siRows.isEmpty()) {
        QVariantMap si = siRows.first().toMap();
        res["id"] = si.value("id");
        res["voucher_no"] = si.value("voucher_no").toInt() > 0 ? si.value("voucher_no") : cleanVNo;
        res["iform_no"] = !si.value("invoice_no").toString().isEmpty() ? si.value("invoice_no") : cleanVNo;
        res["voucher_date"] = si.value("invoice_date");
        res["financial_year"] = si.value("financial_year");
        res["buyer_id"] = si.value("party_id");
        res["buyer_name"] = si.value("party_name");
        res["goods_amount"] = si.value("taxable_amount").toDouble() > 0.001 ? si.value("taxable_amount") : si.value("total_amount");
        res["labour_amount"] = si.value("labour_charges");
        res["dami_amount"] = si.value("brokerage_amount", si.value("commission_amount"));
        res["mandi_fee_amount"] = si.value("mandi_tax_amount");
        res["hrdf_amount"] = si.value("hrdf_amount");
        res["round_off"] = si.value("round_off");
        res["grand_total"] = si.value("grand_total").toDouble() > 0.001 ? si.value("grand_total") : si.value("total_amount");
        res["narration"] = si.value("remarks", si.value("narration"));
        res["vehicle_no"] = si.value("vehicle_no");
        res["broker_name"] = si.value("broker_name", si.value("broker"));

        int siId = si.value("id").toInt();
        QVariantList siItems = db.executeQuery("SELECT * FROM sales_invoice_items WHERE invoice_id = ? ORDER BY id ASC;", {siId});
        QVariantList items;
        for (const auto& sitVar : siItems) {
            QVariantMap sit = sitVar.toMap();
            QVariantMap it;
            it["item_id"] = sit.value("item_id");
            it["item_name"] = sit.value("item_name");
            it["bags"] = sit.value("quantity_bags", sit.value("bags"));
            it["packing"] = 0.500;
            it["loose_weight"] = 0.0;
            it["weight"] = sit.value("weight_quintals", sit.value("quantity"));
            it["rate"] = sit.value("rate");
            it["amount"] = sit.value("amount");
            it["dami_amount"] = sit.value("dami_amount");
            it["mandi_fee_amount"] = sit.value("mandi_fee_amount");
            it["hrdf_amount"] = sit.value("hrdf_amount");
            items.append(it);
        }
        res["items"] = items;
        return res;
    }

    // 3. Fallback to stock_transactions
    QVariantList stRows = db.executeQuery(
        "SELECT * FROM stock_transactions WHERE (voucher_no = ? OR voucher_no = ?) AND trans_type IN ('IFrm', 'I-Form', 'IForm') ORDER BY id ASC;",
        {vNo, cleanVNo}
    );
    if (!stRows.isEmpty()) {
        QVariantMap first = stRows.first().toMap();
        res["id"] = first.value("id");
        res["voucher_no"] = cleanVNo;
        res["iform_no"] = cleanVNo;
        res["voucher_date"] = first.value("voucher_date");
        res["buyer_id"] = first.value("party_id");
        res["buyer_name"] = first.value("party_name");
        res["narration"] = first.value("narration");

        double totalGoods = 0.0;
        int totalBags = 0;
        double totalWt = 0.0;
        QVariantList items;
        for (const auto& stVar : stRows) {
            QVariantMap st = stVar.toMap();
            QVariantMap it;
            it["item_id"] = st.value("item_id");
            it["item_name"] = st.value("item_name");
            it["bags"] = st.value("bags");
            it["packing"] = 0.500;
            it["loose_weight"] = 0.0;
            it["weight"] = st.value("weight_qtl");
            it["rate"] = st.value("rate");
            it["amount"] = st.value("amount");
            totalGoods += it["amount"].toDouble();
            totalBags += it["bags"].toInt();
            totalWt += it["weight"].toDouble();
            items.append(it);
        }
        res["goods_amount"] = totalGoods;
        res["grand_total"] = totalGoods;
        res["total_bags"] = totalBags;
        res["total_weight"] = totalWt;
        res["items"] = items;
        return res;
    }

    return res;
}

QVariantMap IFormModel::get_previous_iform_voucher(int currentId, const QString& currentIfOrVchNo) {
    auto& db = DatabaseManager::instance();
    QVariant targetId;
    if (currentId > 0) {
        targetId = db.executeScalar("SELECT id FROM iform_vouchers WHERE id < ? ORDER BY id DESC LIMIT 1;", {currentId});
    }
    if (!targetId.isValid() && !currentIfOrVchNo.trimmed().isEmpty()) {
        QVariant curRowId = db.executeScalar("SELECT id FROM iform_vouchers WHERE iform_no = ? OR voucher_no = ? LIMIT 1;",
                                             {currentIfOrVchNo, currentIfOrVchNo});
        if (curRowId.isValid()) {
            targetId = db.executeScalar("SELECT id FROM iform_vouchers WHERE id < ? ORDER BY id DESC LIMIT 1;", {curRowId.toInt()});
        }
    }
    if (!targetId.isValid()) {
        targetId = db.executeScalar("SELECT id FROM iform_vouchers ORDER BY id DESC LIMIT 1;");
    }
    if (targetId.isValid()) {
        return get_iform_voucher(targetId.toInt());
    }
    return {};
}

QVariantMap IFormModel::get_next_iform_voucher(int currentId, const QString& currentIfOrVchNo) {
    auto& db = DatabaseManager::instance();
    QVariant targetId;
    if (currentId > 0) {
        targetId = db.executeScalar("SELECT id FROM iform_vouchers WHERE id > ? ORDER BY id ASC LIMIT 1;", {currentId});
    }
    if (!targetId.isValid() && !currentIfOrVchNo.trimmed().isEmpty()) {
        QVariant curRowId = db.executeScalar("SELECT id FROM iform_vouchers WHERE iform_no = ? OR voucher_no = ? LIMIT 1;",
                                             {currentIfOrVchNo, currentIfOrVchNo});
        if (curRowId.isValid()) {
            targetId = db.executeScalar("SELECT id FROM iform_vouchers WHERE id > ? ORDER BY id ASC LIMIT 1;", {curRowId.toInt()});
        }
    }
    if (targetId.isValid()) {
        return get_iform_voucher(targetId.toInt());
    }
    return {};
}

QVariantList IFormModel::get_iform_register(const QString& fromDate, const QString& toDate) {
    auto& db = DatabaseManager::instance();
    QString query = "SELECT * FROM iform_vouchers ";
    QVariantList params;

    if (!fromDate.isEmpty() && !toDate.isEmpty()) {
        query += "WHERE voucher_date >= ? AND voucher_date <= ? ";
        params << fromDate << toDate;
    }
    query += "ORDER BY voucher_date ASC, voucher_no ASC;";

    return db.executeQuery(query, params);
}

bool IFormModel::delete_iform_voucher(int voucherId) {
    if (voucherId <= 0) return false;
    auto& db = DatabaseManager::instance();
    db.beginTransaction();

    QVariantMap vch = get_iform_voucher(voucherId);
    if (!vch.isEmpty()) {
        int vNo = vch.value("voucher_no").toInt();
        QString vDate = vch.value("voucher_date").toString();
        db.executeNonQuery("DELETE FROM vouchers WHERE voucher_type = 'IFrm' AND voucher_no = ? AND voucher_date = ?;", {vNo, vDate});
    }

    db.executeNonQuery("DELETE FROM iform_voucher_items WHERE voucher_id = ?;", {voucherId});
    bool ok = db.executeNonQuery("DELETE FROM iform_vouchers WHERE id = ?;", {voucherId});

    if (ok) {
        db.commit();
        emit voucherDeleted();
        return true;
    }
    db.rollback();
    return false;
}
