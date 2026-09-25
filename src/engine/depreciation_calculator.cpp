#include "depreciation_calculator.h"
#include "../database_manager.h"
#include "fiscal_year_helper.h"
#include <cmath>
#include <algorithm>
#include <QDebug>

namespace MahadevERP {

static double getDefaultDepRate(const QString& name, const QString& group) {
    QString n = name.toLower() + " " + group.toLower();
    if (n.contains("computer") || n.contains("software") || n.contains("server")) return 40.0;
    if (n.contains("building") || n.contains("shed") || n.contains("godown")) return 10.0;
    if (n.contains("furniture") || n.contains("fixture") || n.contains("fitting")) return 10.0;
    if (n.contains("vehicle") || n.contains("car") || n.contains("truck") || n.contains("tractor") || n.contains("trolley")) return 15.0;
    if (n.contains("pollution") || n.contains("water treatment") || n.contains("solar")) return 40.0;
    if (n.contains("machinery") || n.contains("plant") || n.contains("generator") || n.contains("dryer") || n.contains("mill")) return 15.0;
    return 15.0; // Standard plant & machinery rate under Income Tax Act Sec 32
}

QVector<DepreciationAssetItem> DepreciationCalculator::calculateSchedule(const QDate& fromDate, const QDate& toDate, bool isDetailed) {
    QVector<DepreciationAssetItem> results;
    DatabaseManager& db = DatabaseManager::instance();

    QString fromIso = fromDate.isValid() ? fromDate.toString("yyyy-MM-dd") : "2025-04-01";
    QString toIso = toDate.isValid() ? toDate.toString("yyyy-MM-dd") : "2026-03-31";

    // Income Tax Act Sec 32 cutoff date for 180 days: Oct 3 (or 180 days before March 31)
    QDate sDate = QDate::fromString(fromIso, "yyyy-MM-dd");
    QDate octCutoff(sDate.year(), 10, 3);
    QString cutoffIso = octCutoff.toString("yyyy-MM-dd");

    // 1. Fetch Fixed Assets ledgers
    QVariantList assetRows = db.executeQuery(
        "SELECT id, name, group_name, opening_balance, COALESCE(balance_type, 'Dr') AS dr_cr FROM parties "
        "WHERE group_name LIKE '%Fixed Asset%' OR group_name LIKE '%Machinery%' "
        "   OR group_name LIKE '%Plant%' OR group_name LIKE '%Building%' OR group_name LIKE '%Furniture%' "
        "   OR name LIKE '%Machinery%' OR name LIKE '%Building%' OR name LIKE '%Vehicle%' "
        "   OR name LIKE '%Furniture%' OR name LIKE '%Computer%' OR name LIKE '%Plant & Machinery%' "
        "ORDER BY name ASC;"
    );

    bool hasPostings = hasExistingDepreciationPostings(fromDate, toDate);

    for (const QVariant& v : assetRows) {
        QVariantMap row = v.toMap();
        DepreciationAssetItem item;
        item.ledgerId = row.value("id").toInt();
        item.itemName = row.value("name").toString().trimmed();
        item.groupName = row.value("group_name").toString().trimmed();
        if (item.groupName.isEmpty()) item.groupName = "Fixed Assets";
        item.depRate = getDefaultDepRate(item.itemName, item.groupName);
        item.isPosted = hasPostings;

        // Opening Balance (as on fromDate)
        double opBase = 0.0;
        if (row.value("dr_cr").toString().compare("Dr", Qt::CaseInsensitive) == 0) {
            opBase = row.value("opening_balance").toDouble();
        } else {
            opBase = -row.value("opening_balance").toDouble();
        }

        // Transactions prior to fromDate
        QVariant priorDr = db.executeScalar(
            "SELECT SUM(amount) FROM transactions "
            "WHERE (party_id = ? OR party_name = ?) AND voucher_date < ? AND dr_cr = 'Dr' AND voucher_no NOT LIKE 'DEP-%';",
            {item.ledgerId, item.itemName, fromIso}
        );
        QVariant priorCr = db.executeScalar(
            "SELECT SUM(amount) FROM transactions "
            "WHERE (party_id = ? OR party_name = ?) AND voucher_date < ? AND dr_cr = 'Cr' AND voucher_no NOT LIKE 'DEP-%';",
            {item.ledgerId, item.itemName, fromIso}
        );

        double priorDrVal = priorDr.isValid() ? priorDr.toDouble() : 0.0;
        double priorCrVal = priorCr.isValid() ? priorCr.toDouble() : 0.0;
        item.opBalance = std::max(0.0, opBase + priorDrVal - priorCrVal);

        // Additions 1st Half (From fromDate to cutoffDate, excluding DEP entries)
        QVariant add1Dr = db.executeScalar(
            "SELECT SUM(amount) FROM transactions "
            "WHERE (party_id = ? OR party_name = ?) AND voucher_date >= ? AND voucher_date < ? AND dr_cr = 'Dr' AND voucher_no NOT LIKE 'DEP-%';",
            {item.ledgerId, item.itemName, fromIso, cutoffIso}
        );
        item.add1stHalf = add1Dr.isValid() ? add1Dr.toDouble() : 0.0;

        // Additions 2nd Half (From cutoffDate to toDate, excluding DEP entries)
        QVariant add2Dr = db.executeScalar(
            "SELECT SUM(amount) FROM transactions "
            "WHERE (party_id = ? OR party_name = ?) AND voucher_date >= ? AND voucher_date <= ? AND dr_cr = 'Dr' AND voucher_no NOT LIKE 'DEP-%';",
            {item.ledgerId, item.itemName, cutoffIso, toIso}
        );
        item.add2ndHalf = add2Dr.isValid() ? add2Dr.toDouble() : 0.0;

        // Sales / Deductions during the period
        QVariant salesCr = db.executeScalar(
            "SELECT SUM(amount) FROM transactions "
            "WHERE (party_id = ? OR party_name = ?) AND voucher_date >= ? AND voucher_date <= ? AND dr_cr = 'Cr' AND voucher_no NOT LIKE 'DEP-%';",
            {item.ledgerId, item.itemName, fromIso, toIso}
        );
        item.sales = salesCr.isValid() ? salesCr.toDouble() : 0.0;

        // Total Depreciable Base
        item.totalBase = std::max(0.0, item.opBalance + item.add1stHalf + item.add2ndHalf - item.sales);

        // Income Tax Act Sec 32 Calculation:
        // 100% rate on (Op + 1st Half - Sales), 50% rate on 2nd Half Additions
        double baseForFullRate = std::max(0.0, item.opBalance + item.add1stHalf - item.sales);
        double depFull = baseForFullRate * (item.depRate / 100.0);
        double depHalf = item.add2ndHalf * ((item.depRate * 0.5) / 100.0);

        item.depreciation = std::min(item.totalBase, depFull + depHalf);
        item.closingBalance = std::max(0.0, item.totalBase - item.depreciation);

        results.append(item);
    }

    if (!isDetailed) {
        // Group by Group Name / Block
        QMap<QString, DepreciationAssetItem> grouped;
        for (const auto& itm : results) {
            QString gKey = QString("%1 (%2%)").arg(itm.groupName).arg(QString::number(itm.depRate, 'f', 0));
            if (!grouped.contains(gKey)) {
                DepreciationAssetItem g;
                g.itemName = gKey;
                g.groupName = itm.groupName;
                g.depRate = itm.depRate;
                grouped[gKey] = g;
            }
            auto& g = grouped[gKey];
            g.opBalance += itm.opBalance;
            g.add1stHalf += itm.add1stHalf;
            g.add2ndHalf += itm.add2ndHalf;
            g.sales += itm.sales;
            g.totalBase += itm.totalBase;
            g.depreciation += itm.depreciation;
            g.closingBalance += itm.closingBalance;
            g.isPosted = itm.isPosted;
        }
        return grouped.values().toVector();
    }

    return results;
}

DepreciationSummaryTotals DepreciationCalculator::calculateTotals(const QVector<DepreciationAssetItem>& items) {
    DepreciationSummaryTotals totals;
    totals.totalItems = items.size();
    for (const auto& itm : items) {
        totals.totalOpBalance += itm.opBalance;
        totals.totalAdd1stHalf += itm.add1stHalf;
        totals.totalAdd2ndHalf += itm.add2ndHalf;
        totals.totalSales += itm.sales;
        totals.totalBase += itm.totalBase;
        totals.totalDepreciation += itm.depreciation;
        totals.totalClosingBalance += itm.closingBalance;
    }
    return totals;
}

bool DepreciationCalculator::hasExistingDepreciationPostings(const QDate& fromDate, const QDate& toDate) {
    QString fromIso = fromDate.isValid() ? fromDate.toString("yyyy-MM-dd") : "2025-04-01";
    QString toIso = toDate.isValid() ? toDate.toString("yyyy-MM-dd") : "2026-03-31";

    QVariant cnt = DatabaseManager::instance().executeScalar(
        "SELECT COUNT(*) FROM transactions WHERE voucher_no LIKE 'DEP-%' AND voucher_date >= ? AND voucher_date <= ?;",
        {fromIso, toIso}
    );
    return cnt.isValid() && cnt.toInt() > 0;
}

bool DepreciationCalculator::postDepreciationToBooks(const QVector<DepreciationAssetItem>& items,
                                                    const QString& depreciationLedgerName,
                                                    const QDate& voucherDate,
                                                    QString& outError) {
    DatabaseManager& db = DatabaseManager::instance();
    QString vDateIso = voucherDate.isValid() ? voucherDate.toString("yyyy-MM-dd") : QDate::currentDate().toString("yyyy-MM-dd");

    QString depLedger = depreciationLedgerName.trimmed();
    if (depLedger.isEmpty()) depLedger = "Depreciation A/c";

    // 1. Ensure Depreciation A/c exists in parties
    QVariant exists = db.executeScalar("SELECT id FROM parties WHERE name = ?;", {depLedger});
    int depLedgerId = exists.isValid() ? exists.toInt() : 0;
    if (depLedgerId == 0) {
        bool ok = db.executeNonQuery(
            "INSERT INTO parties (name, group_name, opening_balance, dr_cr, address, city, state, gstin, pan) "
            "VALUES (?, 'Indirect Expenses', 0.0, 'Dr', 'Auto Created', 'HQ', 'Haryana', '', '');",
            {depLedger}
        );
        if (!ok) {
            outError = "Failed to create Depreciation ledger.";
            return false;
        }
        depLedgerId = db.executeScalar("SELECT last_insert_rowid();").toInt();
    }

    // 2. Generate unique voucher number
    int vYear = voucherDate.isValid() ? voucherDate.year() : 2026;
    QString vchNo = QString("DEP-%1-%2").arg(vYear).arg(vYear + 1);

    if (!db.beginTransaction()) {
        outError = "Failed to start database transaction.";
        return false;
    }

    // 3. Post double-entry rows for each depreciable asset
    for (const auto& itm : items) {
        if (itm.depreciation <= 0.0) continue;

        QString narration = QString("Depreciation on %1 @ %2% (Sec 32)").arg(itm.itemName).arg(QString::number(itm.depRate, 'f', 0));

        // Debit Depreciation A/c
        bool drOk = db.executeNonQuery(
            "INSERT INTO transactions (voucher_date, voucher_no, voucher_type, party_id, party_name, amount, dr_cr, narration) "
            "VALUES (?, ?, 'Journal', ?, ?, ?, 'Dr', ?);",
            {vDateIso, vchNo, depLedgerId, depLedger, itm.depreciation, narration}
        );

        // Credit Asset Ledger
        bool crOk = db.executeNonQuery(
            "INSERT INTO transactions (voucher_date, voucher_no, voucher_type, party_id, party_name, amount, dr_cr, narration) "
            "VALUES (?, ?, 'Journal', ?, ?, ?, 'Cr', ?);",
            {vDateIso, vchNo, itm.ledgerId, itm.itemName, itm.depreciation, narration}
        );

        if (!drOk || !crOk) {
            db.rollback();
            outError = "Failed to insert depreciation journal voucher entries.";
            return false;
        }
    }

    db.commit();
    return true;
}

bool DepreciationCalculator::deleteDepreciationFromBooks(const QDate& fromDate, const QDate& toDate, QString& outError) {
    DatabaseManager& db = DatabaseManager::instance();
    QString fromIso = fromDate.isValid() ? fromDate.toString("yyyy-MM-dd") : "2025-04-01";
    QString toIso = toDate.isValid() ? toDate.toString("yyyy-MM-dd") : "2026-03-31";

    bool ok = db.executeNonQuery(
        "DELETE FROM transactions WHERE (voucher_no LIKE 'DEP-%' OR narration LIKE '%Depreciation%') "
        "AND voucher_date >= ? AND voucher_date <= ?;",
        {fromIso, toIso}
    );

    if (!ok) {
        outError = "Failed to remove depreciation journal vouchers.";
        return false;
    }
    return true;
}

} // namespace MahadevERP
