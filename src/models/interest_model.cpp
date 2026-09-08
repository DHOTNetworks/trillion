#include "interest_model.h"
#include "../database_manager.h"
#include "../engine/accounting_engine.h"
#include <QDate>
#include <cmath>
#include <algorithm>
#include <iostream>

InterestModel::InterestModel(QObject* parent) : QObject(parent) {}

static int calcBahiKhataDays360(const QString& d1Str, const QString& d2Str) {
    if (d1Str.isEmpty() || d2Str.isEmpty()) return 0;
    QStringList p1 = d1Str.split('-');
    QStringList p2 = d2Str.split('-');
    if (p1.size() != 3 || p2.size() != 3) return 0;
    int y1 = p1[0].toInt(), m1 = p1[1].toInt(), d1 = p1[2].toInt();
    int y2 = p2[0].toInt(), m2 = p2[1].toInt(), d2 = p2[2].toInt();
    int days = (y2 - y1) * 360 + (m2 - m1) * 30 + (30 - d1);
    return std::max(0, days);
}

static int calcCalendarDays365(const QString& d1Str, const QString& d2Str) {
    if (d1Str.isEmpty() || d2Str.isEmpty()) return 0;
    QDate qd1 = QDate::fromString(d1Str, "yyyy-MM-dd");
    QDate qd2 = QDate::fromString(d2Str, "yyyy-MM-dd");
    if (!qd1.isValid() || !qd2.isValid()) return 0;
    return std::max(0, static_cast<int>(qd1.daysTo(qd2)));
}

static std::pair<QString, QString> parseDateFormatted(const QString& dStr) {
    if (dStr.trimmed().isEmpty()) return {"9999-12-31", ""};
    QString s = dStr.trimmed();
    if (s.contains('-')) {
        QStringList p = s.split('-');
        if (p.size() == 3) {
            if (p[0].length() == 4) {
                int y = p[0].toInt(), m = p[1].toInt(), d = p[2].toInt();
                return {
                    QString("%1-%2-%3").arg(y, 4, 10, QChar('0')).arg(m, 2, 10, QChar('0')).arg(d, 2, 10, QChar('0')),
                    QString("%1-%2-%3").arg(d, 2, 10, QChar('0')).arg(m, 2, 10, QChar('0')).arg(y, 4, 10, QChar('0'))
                };
            } else if (p[2].length() == 4) {
                int d = p[0].toInt(), m = p[1].toInt(), y = p[2].toInt();
                return {
                    QString("%1-%2-%3").arg(y, 4, 10, QChar('0')).arg(m, 2, 10, QChar('0')).arg(d, 2, 10, QChar('0')),
                    QString("%1-%2-%3").arg(d, 2, 10, QChar('0')).arg(m, 2, 10, QChar('0')).arg(y, 4, 10, QChar('0'))
                };
            }
        }
    }
    return {s, s};
}

QVariantMap InterestModel::get_interest_data(
    const QString& partyName,
    const QString& fromDate,
    const QString& toDate,
    double crRate,
    double drRate,
    bool dayMethod360,
    int yearDivisor,
    bool includeOpBal
) {
    QVariantMap res;
    QString cleanName = partyName.trimmed();
    if (cleanName.isEmpty()) return res;

    // Resolve active dates if not supplied
    QString fDate = fromDate.trimmed();
    QString tDate = toDate.trimmed();
    QString activeFyName = AccountingEngine::getActiveFyLabel();

    if (fDate.isEmpty() && tDate.isEmpty()) {
        QVariantList fyActiveRows = DatabaseManager::instance().executeQuery(
            "SELECT year_name, start_date, end_date FROM financial_years WHERE is_active = 1 LIMIT 1;"
        );
        if (!fyActiveRows.isEmpty()) {
            QVariantMap act = fyActiveRows.first().toMap();
            activeFyName = act.value("year_name").toString();
            fDate = act.value("start_date").toString();
            tDate = act.value("end_date").toString();
        } else {
            fDate = "2025-04-01";
            tDate = "2026-03-31";
        }
    } else if (tDate.isEmpty()) {
        tDate = "2026-03-31";
    }

    // Convert fDate & tDate to ISO (yyyy-MM-dd)
    auto [fIso, fFmt] = parseDateFormatted(fDate);
    auto [tIso, tFmt] = parseDateFormatted(tDate);

    // Resolve party info
    QString pattern = cleanName;
    pattern.replace(QChar(0x00A0), '%');
    pattern.replace(' ', '%');
    QString wildcard = "%" + pattern + "%";

    QVariantList pRows = DatabaseManager::instance().executeQuery(
        "SELECT id, legacy_id, opening_balance, balance_type FROM parties WHERE name = ? OR name LIKE ? OR name LIKE ? LIMIT 1;",
        {cleanName, "%" + cleanName + "%", wildcard}
    );
    int partyId = 0;
    int legacyCode = 0;
    double initialOp = 0.0;
    QString initialOpType = "Cr";
    if (!pRows.isEmpty()) {
        partyId = pRows.first().toMap().value("id").toInt();
        legacyCode = pRows.first().toMap().value("legacy_id").toInt();
        initialOp = pRows.first().toMap().value("opening_balance").toDouble();
        initialOpType = pRows.first().toMap().value("balance_type").toString();
    }

    // 1. Calculate Prior Opening Balance
    double priorDr = 0.0;
    double priorCr = 0.0;
    if (initialOpType == "Dr") priorDr += initialOp;
    else priorCr += initialOp;

    if (!fIso.isEmpty()) {
        QVariantList priorRows = DatabaseManager::instance().executeQuery(
            "SELECT dr_cr, SUM(amount) as total_amt FROM transactions "
            "WHERE (party_name = ? OR party_name LIKE ? OR party_name LIKE ? OR party_id = ? OR (account_code > 0 AND account_code = ?)) "
            "AND voucher_date < ? GROUP BY dr_cr;",
            {cleanName, "%" + cleanName + "%", wildcard, partyId, legacyCode, fIso}
        );
        for (const auto& pr : priorRows) {
            QVariantMap m = pr.toMap();
            if (m.value("dr_cr").toString().compare("Dr", Qt::CaseInsensitive) == 0) {
                priorDr += m.value("total_amt").toDouble();
            } else {
                priorCr += m.value("total_amt").toDouble();
            }
        }
    }

    double netOp = priorDr - priorCr;

    QVariantList crItems;
    QVariantList drItems;

    double divisor = yearDivisor > 0 ? static_cast<double>(yearDivisor) : 365.0;

    // Add Opening Balance Row if present and requested
    if (includeOpBal && std::abs(netOp) > 0.001) {
        int opBaseDays = dayMethod360 ? calcBahiKhataDays360(fIso, tIso) : calcCalendarDays365(fIso, tIso);
        if (netOp > 0) {
            double opInt = std::round(netOp * (drRate / 100.0) * opBaseDays / divisor * 100.0) / 100.0;
            QVariantMap opItem;
            opItem["vIso"] = fIso;
            opItem["vDate"] = fFmt;
            opItem["refNo"] = "OP-BAL";
            opItem["particulars"] = "Opening Balance (Dr)";
            opItem["amount"] = netOp;
            opItem["rate"] = drRate;
            opItem["dueDays"] = 0;
            opItem["days"] = opBaseDays;
            opItem["interestAmt"] = opInt;
            drItems.append(opItem);
        } else {
            double opAmt = std::abs(netOp);
            double opInt = std::round(opAmt * (crRate / 100.0) * opBaseDays / divisor * 100.0) / 100.0;
            QVariantMap opItem;
            opItem["vIso"] = fIso;
            opItem["vDate"] = fFmt;
            opItem["refNo"] = "OP-BAL";
            opItem["particulars"] = "Opening Balance (Cr)";
            opItem["amount"] = opAmt;
            opItem["rate"] = crRate;
            opItem["dueDays"] = 0;
            opItem["days"] = opBaseDays;
            opItem["interestAmt"] = opInt;
            crItems.append(opItem);
        }
    }

    // 2. Query Period Transactions (excluding TDS entries as TDS is not part of interest principal)
    QString sql = "SELECT voucher_no, voucher_date, voucher_type, trans_type, opposing_account, dr_cr, amount, invoice_no, narration, financial_year, broker_name, vehicle_no "
                  "FROM transactions WHERE (party_name = ? OR party_name LIKE ? OR party_name LIKE ? OR party_id = ? OR (account_code > 0 AND account_code = ?)) "
                  "AND trans_type != 'TDS' AND voucher_type != 'TDS'";
    QVariantList params = {cleanName, "%" + cleanName + "%", wildcard, partyId, legacyCode};
    if (!fIso.isEmpty() && !tIso.isEmpty()) {
        sql += " AND voucher_date >= ? AND voucher_date <= ?";
        params << fIso << tIso;
    }
    sql += " ORDER BY voucher_date ASC, CAST(voucher_no AS INTEGER) ASC, id ASC;";

    QVariantList txRows = DatabaseManager::instance().executeQuery(sql, params);

    for (const auto& tr : txRows) {
        QVariantMap t = tr.toMap();
        QString vNo = t.value("voucher_no").toString();
        QString rawType = t.value("trans_type").toString();
        QString vType = t.value("voucher_type").toString();
        QString opposing = t.value("opposing_account").toString();
        QString drCr = t.value("dr_cr").toString();
        double amt = t.value("amount").toDouble();
        QString invNo = t.value("invoice_no").toString();
        QString narr = t.value("narration").toString().trimmed();
        auto [vIso, vFmt] = parseDateFormatted(t.value("voucher_date").toString());

        int baseDays = dayMethod360 ? calcBahiKhataDays360(vIso, tIso) : calcCalendarDays365(vIso, tIso);
        int dueDays = 0;
        int effDays = std::max(0, baseDays - dueDays);

        QString displayRef = !rawType.isEmpty() ? QString("%1 %2").arg(rawType, vNo).trimmed() : vNo;
        QString desc;
        if (!invNo.isEmpty() && (rawType == "Sale" || rawType == "Purc")) {
            desc = QString("B.No. %1").arg(invNo);
        } else if (!opposing.isEmpty()) {
            desc = opposing;
        } else {
            desc = !narr.isEmpty() ? narr : vType;
        }

        if (drCr.compare("Dr", Qt::CaseInsensitive) == 0) {
            double intAmt = std::round(amt * (drRate / 100.0) * effDays / divisor * 100.0) / 100.0;
            QVariantMap item;
            item["vIso"] = vIso;
            item["vDate"] = vFmt;
            item["refNo"] = displayRef;
            item["voucher_no"] = vNo;
            item["invoice_no"] = invNo;
            item["particulars"] = desc;
            item["amount"] = amt;
            item["rate"] = drRate;
            item["dueDays"] = dueDays;
            item["days"] = effDays;
            item["baseDays"] = baseDays;
            item["interestAmt"] = intAmt;
            drItems.append(item);
        } else {
            double intAmt = std::round(amt * (crRate / 100.0) * effDays / divisor * 100.0) / 100.0;
            QVariantMap item;
            item["vIso"] = vIso;
            item["vDate"] = vFmt;
            item["refNo"] = displayRef;
            item["voucher_no"] = vNo;
            item["invoice_no"] = invNo;
            item["particulars"] = desc;
            item["amount"] = amt;
            item["rate"] = crRate;
            item["dueDays"] = dueDays;
            item["days"] = effDays;
            item["baseDays"] = baseDays;
            item["interestAmt"] = intAmt;
            crItems.append(item);
        }
    }

    // Totals
    double crTotAmt = 0.0, crTotInt = 0.0;
    for (const auto& ci : crItems) {
        QVariantMap m = ci.toMap();
        crTotAmt += m.value("amount").toDouble();
        crTotInt += m.value("interestAmt").toDouble();
    }

    double drTotAmt = 0.0, drTotInt = 0.0;
    for (const auto& di : drItems) {
        QVariantMap m = di.toMap();
        drTotAmt += m.value("amount").toDouble();
        drTotInt += m.value("interestAmt").toDouble();
    }

    double netInt = drTotInt - crTotInt;
    QString netIntType = netInt >= 0 ? "Dr (Receivable)" : "Cr (Payable)";

    double ledgerBal = drTotAmt - crTotAmt;
    QString ledgerBalType = ledgerBal >= 0 ? "Dr" : "Cr";

    double netBal = ledgerBal + netInt;
    QString netBalType = netBal >= 0 ? "Dr" : "Cr";

    res["party_name"] = cleanName;
    res["from_date"] = fFmt;
    res["to_date"] = tFmt;
    res["from_iso"] = fIso;
    res["to_iso"] = tIso;
    res["cr_items"] = crItems;
    res["dr_items"] = drItems;
    res["cr_total_amount"] = crTotAmt;
    res["cr_total_interest"] = crTotInt;
    res["dr_total_amount"] = drTotAmt;
    res["dr_total_interest"] = drTotInt;
    res["net_interest"] = std::abs(netInt);
    res["net_interest_raw"] = netInt;
    res["net_interest_type"] = netIntType;
    res["ledger_balance"] = std::abs(ledgerBal);
    res["ledger_balance_raw"] = ledgerBal;
    res["ledger_balance_type"] = ledgerBalType;
    res["final_net_balance"] = std::abs(netBal);
    res["final_net_balance_raw"] = netBal;
    res["final_net_balance_type"] = netBalType;

    return res;
}

bool InterestModel::post_interest_voucher(
    const QString& partyName,
    const QString& vchDate,
    double interestAmount,
    bool isReceivable,
    const QString& narration
) {
    if (partyName.trimmed().isEmpty() || interestAmount <= 0.001) return false;
    QString dt = vchDate.trimmed().isEmpty() ? QDate::currentDate().toString("yyyy-MM-dd") : vchDate.trimmed();
    auto [isoD, fmtD] = parseDateFormatted(dt);

    auto& db = DatabaseManager::instance();

    // Resolve FY
    QVariantList fyRows = db.executeQuery(
        "SELECT id, year_name FROM financial_years WHERE start_date <= ? AND end_date >= ? LIMIT 1;",
        {isoD, isoD}
    );
    int fyId = 28;
    QString fyLabel = "FY 2025-26";
    if (!fyRows.isEmpty()) {
        fyId = fyRows.first().toMap().value("id").toInt();
        fyLabel = fyRows.first().toMap().value("year_name").toString();
    }

    // Next voucher number for Jrnl
    QVariant maxVch = db.executeScalar(
        "SELECT MAX(CAST(voucher_no AS INTEGER)) FROM vouchers WHERE legacy_type = 'Jrnl' OR voucher_type = 'Journal';"
    );
    int nextVchNo = (maxVch.isValid() && !maxVch.isNull()) ? maxVch.toInt() + 1 : 1;
    QString vchNoStr = QString::number(nextVchNo);

    // Resolve Party ID
    QVariant pIdVal = db.executeScalar("SELECT id FROM parties WHERE name = ? COLLATE NOCASE LIMIT 1;", {partyName.trimmed()});
    int partyId = pIdVal.isValid() ? pIdVal.toInt() : 0;

    QString defaultNarr = QString("Interest for the period up to %1").arg(fmtD);
    QString fullNarr = !narration.trimmed().isEmpty() ? narration.trimmed() : defaultNarr;

    QString opposingAcc = isReceivable ? "Interest Received A/c" : "Interest Paid A/c";

    db.beginTransaction();

    // Insert Header Voucher
    bool ok1 = db.executeNonQuery(
        "INSERT INTO vouchers (fy_id, financial_year, voucher_no, voucher_date, voucher_type, legacy_type, party_id, ledger_id, party_name, account_type, amount, narration) "
        "VALUES (?, ?, ?, ?, 'Journal', 'Jrnl', ?, ?, ?, ?, ?, ?);",
        {fyId, fyLabel, vchNoStr, isoD, partyId, partyId, partyName.trimmed(), opposingAcc, interestAmount, fullNarr}
    );

    // Insert Multi-Leg transactions
    if (isReceivable) {
        // Debit Party, Credit Interest Received
        db.executeNonQuery(
            "INSERT INTO transactions (fy_id, financial_year, voucher_no, voucher_date, voucher_type, trans_type, party_id, party_name, opposing_account, dr_cr, amount, narration) "
            "VALUES (?, ?, ?, ?, 'Journal', 'Jrnl', ?, ?, ?, 'Dr', ?, ?);",
            {fyId, fyLabel, vchNoStr, isoD, partyId, partyName.trimmed(), opposingAcc, interestAmount, fullNarr}
        );
        db.executeNonQuery(
            "INSERT INTO transactions (fy_id, financial_year, voucher_no, voucher_date, voucher_type, trans_type, party_id, party_name, opposing_account, dr_cr, amount, narration) "
            "VALUES (?, ?, ?, ?, 'Journal', 'Jrnl', 0, ?, ?, 'Cr', ?, ?);",
            {fyId, fyLabel, vchNoStr, isoD, opposingAcc, partyName.trimmed(), interestAmount, fullNarr}
        );
    } else {
        // Debit Interest Paid, Credit Party
        db.executeNonQuery(
            "INSERT INTO transactions (fy_id, financial_year, voucher_no, voucher_date, voucher_type, trans_type, party_id, party_name, opposing_account, dr_cr, amount, narration) "
            "VALUES (?, ?, ?, ?, 'Journal', 'Jrnl', 0, ?, ?, 'Dr', ?, ?);",
            {fyId, fyLabel, vchNoStr, isoD, opposingAcc, partyName.trimmed(), interestAmount, fullNarr}
        );
        db.executeNonQuery(
            "INSERT INTO transactions (fy_id, financial_year, voucher_no, voucher_date, voucher_type, trans_type, party_id, party_name, opposing_account, dr_cr, amount, narration) "
            "VALUES (?, ?, ?, ?, 'Journal', 'Jrnl', ?, ?, ?, 'Cr', ?, ?);",
            {fyId, fyLabel, vchNoStr, isoD, partyId, partyName.trimmed(), opposingAcc, interestAmount, fullNarr}
        );
    }

    if (ok1) {
        db.commit();
        emit interestVoucherPosted();
        std::cout << "[SUCCESS] Posted Interest Journal Voucher #" << vchNoStr.toStdString() << " for ₹" << interestAmount << std::endl;
        return true;
    } else {
        db.rollback();
        return false;
    }
}
