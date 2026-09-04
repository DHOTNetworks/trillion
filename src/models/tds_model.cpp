#include "tds_model.h"
#include "../database_manager.h"
#include <QDate>
#include <QLocale>
#include <cmath>
#include <iostream>

TdsModel::TdsModel(QObject* parent) : QObject(parent) {}

QVariantMap TdsModel::get_next_voucher_info(const QString& tdsType) {
    QVariantMap res;
    auto& db = DatabaseManager::instance();

    QVariant maxVch = db.executeScalar("SELECT MAX(voucher_no) FROM tds_vouchers;");
    int nextVch = (maxVch.isValid() && !maxVch.isNull()) ? maxVch.toInt() + 1 : 1;

    QDate today = QDate::currentDate();
    res["next_voucher_no"] = nextVch;
    res["date_iso"] = today.toString("yyyy-MM-dd");
    res["date_display"] = today.toString("dd/MM/yyyy");
    res["day_name"] = today.toString("dddd");

    // Standard default TDS rates by type in India
    QString typeUpper = tdsType.trimmed().toUpper();
    double defaultRate = 10.0;
    if (typeUpper == "FREIGHT" || typeUpper == "CONTRACTOR" || typeUpper == "DAMI") {
        defaultRate = 2.0;
    } else if (typeUpper == "LABOUR") {
        defaultRate = 1.0;
    } else if (typeUpper == "COMMISSION" || typeUpper == "BROKERAGE") {
        defaultRate = 5.0;
    } else if (typeUpper == "INTEREST" || typeUpper == "RENT" || typeUpper == "PROFESSIONAL") {
        defaultRate = 10.0;
    } else if (typeUpper == "OTHER") {
        defaultRate = 0.0;
    }
    res["default_tds_rate"] = defaultRate;
    res["default_surcharge_rate"] = 0.0;
    res["default_cess_rate"] = 0.0;

    // Resolve default Expense ledger based on TDS Type
    QString expPattern = "%" + typeUpper.toLower() + "%";
    QVariantList expRows = db.executeQuery(
        "SELECT id, name FROM parties WHERE LOWER(name) LIKE ? OR LOWER(group_name) LIKE '%expense%' LIMIT 1;",
        {expPattern}
    );
    if (!expRows.isEmpty()) {
        res["default_exp_ledger_id"] = expRows.first().toMap().value("id").toInt();
        res["default_exp_ledger_name"] = expRows.first().toMap().value("name").toString();
    } else {
        if (typeUpper == "RENT") res["default_exp_ledger_name"] = "Godawn Rent";
        else if (typeUpper == "FREIGHT") res["default_exp_ledger_name"] = "Freight Outward";
        else if (typeUpper == "LABOUR") res["default_exp_ledger_name"] = "Labour Charges";
        else if (typeUpper == "INTEREST") res["default_exp_ledger_name"] = "Interest Account";
        else if (typeUpper == "BROKERAGE") res["default_exp_ledger_name"] = "Brokerage Account";
        else if (typeUpper == "COMMISSION") res["default_exp_ledger_name"] = "Commission Account";
        else res["default_exp_ledger_name"] = typeUpper.toLower().replace(0, 1, typeUpper.left(1).toUpper()) + " Expense";
        res["default_exp_ledger_id"] = 0;
    }

    // Resolve default TDS Cr. Ledger (e.g. "T D S PEYABAL" or "TDS Payable")
    QVariantList tdsLedgerRows = db.executeQuery(
        "SELECT id, name FROM parties WHERE LOWER(name) LIKE '%tds%' OR LOWER(name) LIKE '%t d s%' LIMIT 1;"
    );
    if (!tdsLedgerRows.isEmpty()) {
        res["default_tds_ledger_id"] = tdsLedgerRows.first().toMap().value("id").toInt();
        res["default_tds_ledger_name"] = tdsLedgerRows.first().toMap().value("name").toString();
    } else {
        res["default_tds_ledger_id"] = 0;
        res["default_tds_ledger_name"] = "T D S PEYABAL";
    }

    return res;
}

QVariantMap TdsModel::get_party_info(int partyId, const QString& tdsType) {
    QVariantMap res;
    if (partyId <= 0) {
        res["formatted_balance"] = "Date Bal. 0.00";
        res["balance"] = 0.0;
        res["drcr"] = "Dr";
        res["pan_no"] = "";
        res["previous_amount"] = 0.0;
        res["last_narration"] = "";
        return res;
    }

    auto& db = DatabaseManager::instance();

    // 1. Party master details
    QVariantList pRows = db.executeQuery("SELECT pan, name FROM parties WHERE id = ? LIMIT 1;", {partyId});
    QString pan = "";
    QString partyName = "";
    if (!pRows.isEmpty()) {
        pan = pRows.first().toMap().value("pan").toString().trimmed();
        partyName = pRows.first().toMap().value("name").toString().trimmed();
    }
    res["pan_no"] = pan;
    res["party_name"] = partyName;

    // 2. Live balance calculation
    QVariant balVar = db.executeScalar(
        "SELECT SUM(CASE WHEN UPPER(account_type) = 'CR' OR UPPER(account_type) = 'CREDIT' THEN -amount ELSE amount END) "
        "FROM vouchers WHERE ledger_id = ? OR party_id = ?;",
        {partyId, partyId}
    );
    double bal = (balVar.isValid() && !balVar.isNull()) ? balVar.toDouble() : 0.0;
    QString drcr = bal >= 0.0 ? "Dr" : "Cr";
    double absBal = std::abs(bal);

    QLocale locale(QLocale::English, QLocale::India);
    QString numStr = locale.toString(absBal, 'f', 2);
    res["formatted_balance"] = QString("Date Bal. %1 %2").arg(numStr, drcr);
    res["balance"] = absBal;
    res["drcr"] = drcr;

    // 3. Previous cumulative income in active financial year for this TDS type
    QVariant prevVar = db.executeScalar(
        "SELECT SUM(income_amount) FROM tds_vouchers WHERE (ledger_id = ? OR ledger_name = ?) AND UPPER(tds_type) = ?;",
        {partyId, partyName, tdsType.trimmed().toUpper()}
    );
    double prevAmt = (prevVar.isValid() && !prevVar.isNull()) ? prevVar.toDouble() : 0.0;
    res["previous_amount"] = prevAmt;

    // 4. Last narration
    res["last_narration"] = get_last_narration(partyId, tdsType);

    return res;
}

QString TdsModel::get_last_narration(int partyId, const QString& tdsType) {
    auto& db = DatabaseManager::instance();
    if (partyId > 0) {
        QVariant nVal = db.executeScalar(
            "SELECT narration FROM tds_vouchers WHERE ledger_id = ? AND narration IS NOT NULL AND narration != '' ORDER BY id DESC LIMIT 1;",
            {partyId}
        );
        if (nVal.isValid() && !nVal.toString().trimmed().isEmpty()) {
            return nVal.toString().trimmed();
        }
    }

    if (!tdsType.isEmpty()) {
        QVariant nVal2 = db.executeScalar(
            "SELECT narration FROM tds_vouchers WHERE UPPER(tds_type) = ? AND narration IS NOT NULL AND narration != '' ORDER BY id DESC LIMIT 1;",
            {tdsType.trimmed().toUpper()}
        );
        if (nVal2.isValid() && !nVal2.toString().trimmed().isEmpty()) {
            return nVal2.toString().trimmed();
        }
    }

    return tdsType.trimmed().toUpper();
}

bool TdsModel::save_tds_voucher(const QVariantMap& data) {
    auto& db = DatabaseManager::instance();

    int voucherNo = data.value("voucher_no").toInt();
    QString voucherDate = data.value("voucher_date").toString().trimmed();
    if (voucherDate.isEmpty()) voucherDate = QDate::currentDate().toString("yyyy-MM-dd");
    // Ensure ISO yyyy-MM-dd
    if (voucherDate.contains("/")) {
        QStringList parts = voucherDate.split("/");
        if (parts.size() == 3) {
            voucherDate = QString("%1-%2-%3").arg(parts[2], parts[1], parts[0]);
        }
    }

    QString dayOfWeek = data.value("day_of_week").toString().trimmed();
    if (dayOfWeek.isEmpty()) {
        QDate dt = QDate::fromString(voucherDate, "yyyy-MM-dd");
        dayOfWeek = dt.toString("dddd");
    }

    int postInBooks = data.value("post_in_books", 1).toInt();
    QString tdsType = data.value("tds_type").toString().trimmed().toUpper();
    if (tdsType.isEmpty()) tdsType = "RENT";

    int ledgerId = data.value("ledger_id").toInt();
    QString ledgerName = data.value("ledger_name").toString().trimmed();

    double incomeAmount = data.value("income_amount").toDouble();
    double previousAmount = data.value("previous_amount").toDouble();
    double totalForTds = data.value("total_for_tds").toDouble();
    if (totalForTds <= 0.0) totalForTds = incomeAmount + previousAmount;

    QString narration = data.value("narration").toString().trimmed();
    double rateTds = data.value("rate_tds").toDouble();
    double taxAmountTds = data.value("tax_amount_tds").toDouble();
    double rateSurcharge = data.value("rate_surcharge").toDouble();
    double taxAmountSurcharge = data.value("tax_amount_surcharge").toDouble();
    double rateCess = data.value("rate_cess").toDouble();
    double taxAmountCess = data.value("tax_amount_cess").toDouble();

    int useRoundedTotal = data.value("use_rounded_total", 1).toInt();
    double totalTaxRate = data.value("total_tax_rate").toDouble();
    double totalTaxAmount = data.value("total_tax_amount").toDouble();
    double netAmount = data.value("net_amount").toDouble();

    QString nonDeductionReason = data.value("non_deduction_reason").toString().trimmed();
    int expLedgerId = data.value("exp_ledger_id").toInt();
    QString expLedgerName = data.value("exp_ledger_name").toString().trimmed();
    int tdsLedgerId = data.value("tds_ledger_id").toInt();
    QString tdsLedgerName = data.value("tds_ledger_name").toString().trimmed();

    // Determine FY
    QVariantList fyRows = db.executeQuery(
        "SELECT id, year_name FROM financial_years WHERE start_date <= ? AND end_date >= ? LIMIT 1;",
        {voucherDate, voucherDate}
    );
    int fyId = 1;
    QString fyLabel = "FY 2025-26";
    if (!fyRows.isEmpty()) {
        fyId = fyRows.first().toMap().value("id").toInt();
        fyLabel = fyRows.first().toMap().value("year_name").toString();
    }

    if (voucherNo <= 0) {
        QVariant maxVch = db.executeScalar("SELECT MAX(voucher_no) FROM tds_vouchers WHERE financial_year = ?;", {fyLabel});
        voucherNo = (maxVch.isValid() && !maxVch.isNull()) ? maxVch.toInt() + 1 : 1;
    }

    db.beginTransaction();

    // 1. Insert into tds_vouchers
    bool ok = db.executeNonQuery(
        "INSERT INTO tds_vouchers ("
        "fy_id, financial_year, voucher_no, voucher_date, day_of_week, post_in_books, tds_type, "
        "ledger_id, ledger_name, income_amount, previous_amount, total_for_tds, narration, "
        "rate_tds, tax_amount_tds, rate_surcharge, tax_amount_surcharge, rate_cess, tax_amount_cess, "
        "use_rounded_total, total_tax_rate, total_tax_amount, net_amount, non_deduction_reason, "
        "exp_ledger_id, exp_ledger_name, tds_ledger_id, tds_ledger_name"
        ") VALUES ("
        "?, ?, ?, ?, ?, ?, ?, "
        "?, ?, ?, ?, ?, ?, "
        "?, ?, ?, ?, ?, ?, "
        "?, ?, ?, ?, ?, "
        "?, ?, ?, ?"
        ");",
        {
            fyId, fyLabel, voucherNo, voucherDate, dayOfWeek, postInBooks, tdsType,
            ledgerId, ledgerName, incomeAmount, previousAmount, totalForTds, narration,
            rateTds, taxAmountTds, rateSurcharge, taxAmountSurcharge, rateCess, taxAmountCess,
            useRoundedTotal, totalTaxRate, totalTaxAmount, netAmount, nonDeductionReason,
            expLedgerId, expLedgerName, tdsLedgerId, tdsLedgerName
        }
    );

    if (!ok) {
        db.rollback();
        std::cerr << "[ERROR] Failed to insert TDS voucher record" << std::endl;
        return false;
    }

    // 2. If post_in_books == 1, create double-entry postings in vouchers table
    if (postInBooks == 1 && incomeAmount > 0.001) {
        QString vchCode = QString("TDS-%1").arg(voucherNo);

        // Ensure Expense Ledger party ID
        if (expLedgerId <= 0 && !expLedgerName.isEmpty()) {
            QVariant expRow = db.executeScalar("SELECT id FROM parties WHERE name = ? LIMIT 1;", {expLedgerName});
            if (expRow.isValid() && !expRow.isNull()) {
                expLedgerId = expRow.toInt();
            } else {
                db.executeNonQuery(
                    "INSERT INTO parties (name, group_name, party_type) VALUES (?, 'Direct Expenses', 'Expense');",
                    {expLedgerName}
                );
                expLedgerId = db.executeScalar("SELECT last_insert_rowid();").toInt();
            }
        }

        // Ensure TDS Cr Ledger party ID
        if (tdsLedgerId <= 0 && !tdsLedgerName.isEmpty()) {
            QVariant tdsRow = db.executeScalar("SELECT id FROM parties WHERE name = ? LIMIT 1;", {tdsLedgerName});
            if (tdsRow.isValid() && !tdsRow.isNull()) {
                tdsLedgerId = tdsRow.toInt();
            } else {
                db.executeNonQuery(
                    "INSERT INTO parties (name, group_name, party_type) VALUES (?, 'Duties & Taxes', 'Tax');",
                    {tdsLedgerName}
                );
                tdsLedgerId = db.executeScalar("SELECT last_insert_rowid();").toInt();
            }
        }

        // Entry 1: Exp. Dr. Ledger [Debit Income Amount]
        db.executeNonQuery(
            "INSERT INTO vouchers ("
            "fy_id, financial_year, voucher_no, voucher_date, voucher_type, legacy_type, "
            "party_id, ledger_id, party_name, account_type, amount, narration"
            ") VALUES (?, ?, ?, ?, 'TDS', ?, ?, ?, ?, 'Dr', ?, ?);",
            {
                fyId, fyLabel, vchCode, voucherDate, QString("TDS%1").arg(tdsType),
                expLedgerId, expLedgerId, expLedgerName, incomeAmount,
                narration.isEmpty() ? ledgerName : narration
            }
        );

        // Entry 2: Deductee/Party Ledger [Credit Gross Income Amount]
        db.executeNonQuery(
            "INSERT INTO vouchers ("
            "fy_id, financial_year, voucher_no, voucher_date, voucher_type, legacy_type, "
            "party_id, ledger_id, party_name, account_type, amount, narration"
            ") VALUES (?, ?, ?, ?, 'TDS', ?, ?, ?, ?, 'Cr', ?, ?);",
            {
                fyId, fyLabel, vchCode, voucherDate, QString("TDS%1").arg(tdsType),
                ledgerId, ledgerId, ledgerName, incomeAmount,
                tdsType
            }
        );

        // Entries 3 & 4: If Tax was deducted, Debit Party & Credit TDS Cr Ledger
        if (totalTaxAmount > 0.001) {
            QString taxNarr = QString("T.D.S. %1 @%2% on Amt. %3")
                                  .arg(tdsType)
                                  .arg(totalTaxRate, 0, 'f', 2)
                                  .arg(totalForTds, 0, 'f', 2);

            // Entry 3: Deductee/Party Ledger [Debit TDS Tax Amount]
            db.executeNonQuery(
                "INSERT INTO vouchers ("
                "fy_id, financial_year, voucher_no, voucher_date, voucher_type, legacy_type, "
                "party_id, ledger_id, party_name, account_type, amount, narration"
                ") VALUES (?, ?, ?, ?, 'TDS', ?, ?, ?, ?, 'Dr', ?, ?);",
                {
                    fyId, fyLabel, vchCode, voucherDate, QString("TDS%1").arg(tdsType),
                    ledgerId, ledgerId, ledgerName, totalTaxAmount,
                    taxNarr
                }
            );

            // Entry 4: TDS Cr. Ledger [Credit TDS Tax Amount]
            db.executeNonQuery(
                "INSERT INTO vouchers ("
                "fy_id, financial_year, voucher_no, voucher_date, voucher_type, legacy_type, "
                "party_id, ledger_id, party_name, account_type, amount, narration"
                ") VALUES (?, ?, ?, ?, 'TDS', ?, ?, ?, ?, 'Cr', ?, ?);",
                {
                    fyId, fyLabel, vchCode, voucherDate, QString("TDS%1").arg(tdsType),
                    tdsLedgerId, tdsLedgerId, tdsLedgerName, totalTaxAmount,
                    ledgerName
                }
            );
        }
    }

    db.commit();
    emit voucherSaved();
    std::cout << "[INFO] Successfully saved TDS Voucher #" << voucherNo << " (" << tdsType.toStdString() << ")" << std::endl;
    return true;
}

QVariantMap TdsModel::get_tds_voucher(int voucherId) {
    auto& db = DatabaseManager::instance();
    QVariantList rows = db.executeQuery("SELECT * FROM tds_vouchers WHERE id = ? LIMIT 1;", {voucherId});
    if (!rows.isEmpty()) {
        return rows.first().toMap();
    }
    return {};
}

QVariantList TdsModel::get_tds_register(const QString& fromDate, const QString& toDate) {
    auto& db = DatabaseManager::instance();
    QString sql = "SELECT * FROM tds_vouchers";
    QVariantList args;
    if (!fromDate.isEmpty() && !toDate.isEmpty()) {
        sql += " WHERE voucher_date >= ? AND voucher_date <= ?";
        args.append(fromDate);
        args.append(toDate);
    }
    sql += " ORDER BY voucher_date DESC, voucher_no DESC;";
    return db.executeQuery(sql, args);
}

bool TdsModel::delete_tds_voucher(int voucherId) {
    auto& db = DatabaseManager::instance();
    QVariantMap vch = get_tds_voucher(voucherId);
    if (vch.isEmpty()) return false;

    int vchNo = vch.value("voucher_no").toInt();
    QString vchCode = QString("TDS-%1").arg(vchNo);

    db.beginTransaction();
    db.executeNonQuery("DELETE FROM tds_vouchers WHERE id = ?;", {voucherId});
    db.executeNonQuery("DELETE FROM vouchers WHERE voucher_type = 'TDS' AND voucher_no = ?;", {vchCode});
    db.commit();

    emit voucherDeleted();
    return true;
}
