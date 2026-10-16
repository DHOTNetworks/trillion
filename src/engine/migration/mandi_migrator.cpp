#include "mandi_migrator.h"
#include "../database_manager.h"
#include "../data_preprocessor.h"
#include "migration_columns.h"
#include "migration_utils.h"
#include <QDate>
#include <QDebug>
#include <cmath>
#include <algorithm>
#include <iostream>
#include <set>

namespace MahadevERP {

// Verbatim monolith helpers (same rationale as voucher_migrator.cpp:
// DataPreprocessor's date parser lacks the 1900-1970 century window).
static QString parseDateFormatted(const QString& raw) {
    QString s = raw.trimmed();
    if (s.isEmpty()) return "";
    QString firstPart = s.split(' ').first();
    QStringList parts = firstPart.split('/');
    if (parts.size() == 3) {
        int m = parts[0].toInt();
        int d = parts[1].toInt();
        int y = parts[2].toInt();
        if (y < 100) y += 2000;
        else if (y >= 1900 && y <= 1970) y += 100;

        // In Jet DB libmdb date strings, format is MM/DD/YYYY
        // If m > 12 and d <= 12, then it was DD/MM/YYYY
        if (m > 12 && d <= 12) {
            std::swap(m, d);
        }

        return QString("%1-%2-%3")
            .arg(y, 4, 10, QChar('0'))
            .arg(m, 2, 10, QChar('0'))
            .arg(d, 2, 10, QChar('0'));
    }
    parts = firstPart.split('-');
    if (parts.size() == 3) {
        if (parts[0].length() == 4) {
            int y = parts[0].toInt();
            if (y >= 1900 && y <= 1970) y += 100;
            return QString("%1-%2-%3").arg(y, 4, 10, QChar('0')).arg(parts[1]).arg(parts[2]);
        }
        int d = parts[0].toInt();
        int m = parts[1].toInt();
        int y = parts[2].toInt();
        if (y < 100) y += 2000;
        else if (y >= 1900 && y <= 1970) y += 100;
        return QString("%1-%2-%3")
            .arg(y, 4, 10, QChar('0'))
            .arg(m, 2, 10, QChar('0'))
            .arg(d, 2, 10, QChar('0'));
    }
    return firstPart;
}

static QString computeFinancialYear(const QString& isoDate) {
    if (isoDate.length() < 7) {
        QDate cur = QDate::currentDate();
        int curYr = (cur.month() >= 4) ? cur.year() : (cur.year() - 1);
        return QString("FY %1-%2").arg(curYr).arg(QString::number((curYr + 1) % 100).rightJustified(2, '0'));
    }
    int year = isoDate.left(4).toInt();
    int month = isoDate.mid(5, 2).toInt();
    if (month >= 4) {
        return QString("FY %1-%2").arg(year).arg(QString::number((year + 1) % 100).rightJustified(2, '0'));
    } else {
        return QString("FY %1-%2").arg(year - 1).arg(QString::number(year % 100).rightJustified(2, '0'));
    }
}

// Forward declaration: defined with the other file-local helpers below
// (kept in place for the pre-existing busy/dead-Bahi bodies).
static std::string getFieldStr(const std::map<std::string, std::string>& m, const std::string& key, const std::string& def = "");

// Ordered-fallback read for pipe-compound JSON sources
// ("RateTDS|TDSRate"): first non-empty (post-clean) wins — mirrors the
// monolith's explicit if-empty chains exactly.
static std::string getFieldFirstClean(
    const std::map<std::string, std::string>& m, const std::string& pipeKeys, const std::string& def = "") {
    size_t start = 0;
    while (start <= pipeKeys.size()) {
        const size_t bar = pipeKeys.find('|', start);
        std::string key = (bar == std::string::npos) ? pipeKeys.substr(start) : pipeKeys.substr(start, bar - start);
        const size_t f = key.find_first_not_of(" \t");
        const size_t l = key.find_last_not_of(" \t");
        if (f != std::string::npos) {
            std::string v = DataPreprocessor::cleanText(getFieldStr(m, key.substr(f, l - f + 1)));
            if (!v.empty()) return v;
        }
        if (bar == std::string::npos) break;
        start = bar + 1;
    }
    return def;
}

// Verbatim monolith helper (ledger display-name fallback chain). One copy
// lives per strangler TU (file-local, no back-edge); the monolith keeps
// its own until the mandi slice retires it.
static std::string lookupLedgerName(
    int acCode,
    const std::map<int, PartyDetail>& ledgerDetailMap,
    const std::map<int, std::string>& ledgerCodeMap,
    const std::map<int, std::string>& ledgerDeletedNameMap
) {
    auto it = ledgerDetailMap.find(acCode);
    if (it != ledgerDetailMap.end() && !it->second.name.empty()) return it->second.name;
    auto jt = ledgerCodeMap.find(acCode);
    if (jt != ledgerCodeMap.end() && !jt->second.empty()) return jt->second;
    auto kt = ledgerDeletedNameMap.find(acCode);
    if (kt != ledgerDeletedNameMap.end() && !kt->second.empty()) return kt->second;
    return "";
}

bool MandiMigrator::migrate_manufacturing_bahikhata(
    MigrationContext& ctx,
    const std::vector<std::map<std::string, std::string>>& manufacturingRows,
    const std::map<int, ItemDetail>& itemCodeMap,
    std::map<QString, int>& fyNameToId,
    size_t& outGroupCount
) {
    // C3 slice M1 (monolith PASS 5.2): transplanted verbatim. Milling
    // keys via millingField() — the JSON entry covers every read, so
    // this pass has zero JSON-gap literals. Batch numbers stay raw
    // ("Manu-<vNo>": no canonicalVoucherNo minting). Silent fy skips
    // gained the F5 warn+count (row output unchanged).
    auto MF = [](const char* t) { return MigrationColumns::millingField(QString::fromLatin1(t)).toStdString(); };
    auto& db = DatabaseManager::instance();
    ctx.updateProgress(75, QString("Migrating Manufacturing Production Batches & Line Items..."));
    std::map<std::pair<std::string, std::string>, std::vector<std::map<std::string, std::string>>> manufacturingGroups;
    for (const auto& mr : manufacturingRows) {
        std::string mvNo = DataPreprocessor::cleanText(getFieldStr(mr, MF("batch_no")));
        std::string mvDate = parseDateFormatted(QString::fromStdString(getFieldStr(mr, MF("batch_date")))).toStdString();
        manufacturingGroups[{mvNo, mvDate}].push_back(mr);
    }
    outGroupCount = manufacturingGroups.size();

    db.executeNonQuery("DELETE FROM manufacturing_batches;");
    db.executeNonQuery("DELETE FROM manufacturing_voucher_items;");

    int batchCount = 0;
    for (const auto& pair : manufacturingGroups) {
        std::string mvNo = pair.first.first;
        std::string mvDate = pair.first.second;
        const auto& mRows = pair.second;

        QString batchNoStr = QString("Manu-%1").arg(QString::fromStdString(mvNo));
        QString fyVal = computeFinancialYear(QString::fromStdString(mvDate));
        int fyId = MigrationUtils::fyIdForStd(fyNameToId, fyVal);
        // F5: fail loud with row context (was a silent skip).
        if (fyId <= 0) { ++ctx.stats.unmappedFyRows; qWarning() << "[Migration] manufacturing batch skipped (unresolvable FY):" << QString::fromStdString(mvNo) << fyVal; continue; }

        double paddyIn = 0.0;
        std::string paddyVariety = "Paddy Basmati";
        double headRice = 0.0;
        double brokenRice = 0.0;
        double bran = 0.0;
        double husk = 0.0;
        std::string batchNarration;

        for (const auto& r : mRows) {
            int iCode = DataPreprocessor::parseInt(getFieldStr(r, MF("item_id")));
            auto itItem = itemCodeMap.find(iCode);
            std::string iName = (itItem != itemCodeMap.end()) ? itItem->second.name : DataPreprocessor::cleanText(getFieldStr(r, MF("item_name")));
            if (iName.empty()) iName = "Item #" + std::to_string(iCode);

            std::string iLower = DataPreprocessor::toLowerStr(iName);
            double wt = DataPreprocessor::parseDouble(getFieldStr(r, MF("weight_qtl")));
            std::string drcr = DataPreprocessor::cleanText(getFieldStr(r, MF("drcr")));
            std::string narr = DataPreprocessor::cleanText(getFieldStr(r, MF("narration")));
            if (!narr.empty() && batchNarration.empty()) batchNarration = narr;

            if (drcr == "Cr") {
                paddyIn += wt;
                paddyVariety = iName;
            } else {
                if (iLower.find("bran") != std::string::npos && iLower.find("brand") == std::string::npos) {
                    bran += wt;
                } else if (iLower.find("broken") != std::string::npos || iLower.find("nakku") != std::string::npos || iLower.find("tibar") != std::string::npos || iLower.find("dubar") != std::string::npos || iLower.find("mogra") != std::string::npos || iLower.find("kinki") != std::string::npos) {
                    brokenRice += wt;
                } else if (iLower.find("husk") != std::string::npos || iLower.find("phak") != std::string::npos || iLower.find("bhusa") != std::string::npos) {
                    husk += wt;
                } else {
                    headRice += wt;
                }
            }
        }

        double totalOut = headRice + brokenRice + bran + husk;
        double wastage = std::max(0.0, std::round((paddyIn - totalOut) * 1000.0) / 1000.0);
        double yieldPct = (paddyIn > 0.0) ? std::round((headRice / paddyIn * 100.0) * 100.0) / 100.0 : 0.0;

        db.executeNonQuery(
            "INSERT INTO manufacturing_batches ("
            "fy_id, financial_year, batch_no, batch_date, input_variety, input_qtl, "
            "output_qtl, byproduct_qtl, "
            "wastage_qtl, yield_pct, narration) "
            "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);",
            {
                (fyId > 0 ? QVariant(fyId) : QVariant()),
                fyVal,
                batchNoStr,
                QString::fromStdString(mvDate),
                QString::fromStdString(paddyVariety),
                paddyIn,
                headRice,
                brokenRice + bran + husk,
                wastage,
                yieldPct,
                QString::fromStdString(batchNarration)
            }
        );
        int batchDbId = static_cast<int>(db.lastInsertedId());

        for (const auto& r : mRows) {
            int rowNo = DataPreprocessor::parseInt(getFieldStr(r, MF("row_no")), 1);
            std::string drcr = DataPreprocessor::cleanText(getFieldStr(r, MF("drcr")));
            int iCode = DataPreprocessor::parseInt(getFieldStr(r, MF("item_id")));
            auto itItem = itemCodeMap.find(iCode);
            int iId = (itItem != itemCodeMap.end()) ? itItem->second.id : 0;
            std::string iName = (itItem != itemCodeMap.end()) ? itItem->second.name : DataPreprocessor::cleanText(getFieldStr(r, MF("item_name")));
            if (iName.empty()) iName = "Item #" + std::to_string(iCode);

            double pct = DataPreprocessor::parseDouble(getFieldStr(r, MF("percentage")));
            double wt = DataPreprocessor::parseDouble(getFieldStr(r, MF("weight_qtl")));
            int bags = DataPreprocessor::parseInt(getFieldStr(r, MF("bags")));
            double amt = DataPreprocessor::parseDouble(getFieldStr(r, MF("amount")));
            double rate = (wt > 0.0) ? std::round((amt / wt) * 100.0) / 100.0 : 0.0;
            std::string narr = DataPreprocessor::cleanText(getFieldStr(r, MF("narration")));

            db.executeNonQuery(
                "INSERT INTO manufacturing_voucher_items ("
                "batch_id, batch_no, batch_date, row_no, drcr, "
                "item_id, item_code, item_name, percentage, "
                "weight_qtl, bags, rate, amount, narration) "
                "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);",
                {
                    batchDbId,
                    batchNoStr,
                    QString::fromStdString(mvDate),
                    rowNo,
                    QString::fromStdString(drcr),
                    (iId > 0 ? QVariant(iId) : QVariant()),
                    QString::number(iCode),
                    QString::fromStdString(iName),
                    pct,
                    wt,
                    bags,
                    rate,
                    amt,
                    QString::fromStdString(narr)
                }
            );
        }

        QString vchNarr = QString("Manufacturing Batch %1: Input %2 Qtl %3 -> Produced %4 Qtl (%5% Yield)")
            .arg(batchNoStr)
            .arg(paddyIn, 0, 'f', 2)
            .arg(QString::fromStdString(paddyVariety))
            .arg(totalOut, 0, 'f', 2)
            .arg(yieldPct, 0, 'f', 1);
        if (!batchNarration.empty()) {
            vchNarr += " | " + QString::fromStdString(batchNarration);
        }

        db.executeNonQuery(
            "INSERT INTO vouchers ("
            "fy_id, financial_year, voucher_no, voucher_date, voucher_type, legacy_type, "
            "party_name, account_type, amount, narration) "
            "VALUES (?, ?, ?, ?, 'Manufacturing', 'Manu', 'Manufacturing / Production Account', 'Production Account', 0.0, ?);",
            {
                (fyId > 0 ? QVariant(fyId) : QVariant()),
                fyVal,
                batchNoStr,
                QString::fromStdString(mvDate),
                vchNarr
            }
        );
        batchCount++;
    }
    std::cout << "[INFO] Migrated " << batchCount << " Manufacturing production batches!" << std::endl;
    return true;
}

bool MandiMigrator::migrate_jform_bahikhata(
    MigrationContext& ctx,
    const std::vector<std::map<std::string, std::string>>& transRows,
    const std::vector<std::map<std::string, std::string>>& stockTransRows,
    const std::map<int, PartyDetail>& ledgerDetailMap,
    const std::map<int, ItemDetail>& itemCodeMap,
    std::map<QString, int>& fyNameToId
) {
    // C3 slice M1 (monolith PASS 3.5): transplanted verbatim. Form keys
    // via jformField(), generic Transactions keys (DrCr/AccountCode) via
    // voucherField(). JSON-gap literals: TransType filter, EntryType,
    // real-field Amount reads stay literal-free via JF("amount")=="Amount"
    // (no dual-write clash in this entry), DheriPurchaseFrom /
    // CommissionPartyCode fallbacks. Raw voucher numbers preserved.
    auto JF = [](const char* t) { return MigrationColumns::jformField(QString::fromLatin1(t)).toStdString(); };
    auto VF = [](const char* t) { return MigrationColumns::voucherField(QString::fromLatin1(t)).toStdString(); };
    auto& db = DatabaseManager::instance();
    ctx.updateProgress(88, "Migrating J-Form Mandi Procurement Vouchers...");

    // Group StockTransactions where TransType == 'JFrm' by (VoucherNumber, VoucherDate)
    std::map<std::pair<std::string, std::string>, std::vector<std::map<std::string, std::string>>> jformStockGroups;
    for (const auto& st : stockTransRows) {
        std::string tt = DataPreprocessor::cleanText(getFieldStr(st, "TransType")); // JSON-gap(JF): filter VALUE, not a column read
        if (tt != "JFrm") continue;
        std::string vNo = DataPreprocessor::cleanText(getFieldStr(st, JF("voucher_no")));
        QString vDate = parseDateFormatted(QString::fromStdString(getFieldStr(st, JF("voucher_date"))));
        jformStockGroups[{vNo, vDate.toStdString()}].push_back(st);
    }

    // Group Transactions where TransType == 'JFrm' by (VoucherNumber, VoucherDate)
    std::map<std::pair<std::string, std::string>, std::vector<std::map<std::string, std::string>>> jformTxGroups;
    for (const auto& tr : transRows) {
        std::string tt = DataPreprocessor::cleanText(getFieldStr(tr, "TransType")); // JSON-gap(JF)
        if (tt != "JFrm") continue;
        std::string vNo = DataPreprocessor::cleanText(getFieldStr(tr, JF("voucher_no")));
        QString vDate = parseDateFormatted(QString::fromStdString(getFieldStr(tr, JF("voucher_date"))));
        jformTxGroups[{vNo, vDate.toStdString()}].push_back(tr);
    }

    int jformCount = 0;
    std::set<std::pair<std::string, std::string>> allJFormKeys;
    for (const auto& p : jformStockGroups) allJFormKeys.insert(p.first);
    for (const auto& p : jformTxGroups) allJFormKeys.insert(p.first);

    for (const auto& k : allJFormKeys) {
        std::string vNo = k.first;
        QString vDate = QString::fromStdString(k.second);
        int vchNum = DataPreprocessor::parseInt(vNo, 1);
        QString fyVal = computeFinancialYear(vDate);
        int fyId = MigrationUtils::fyIdForStd(fyNameToId, fyVal);
        // F5: fail loud with row context (was a silent skip).
        if (fyId <= 0) { ++ctx.stats.unmappedFyRows; qWarning() << "[Migration] J-Form skipped (unresolvable FY):" << QString::fromStdString(vNo) << fyVal; continue; }

        QString jformNo = QString::fromStdString(vNo);
        int zimidarId = 0;
        QString zimidarName = "";
        int partyId = 0;
        QString partyName = "Self Purchase";
        QString auctionStatus = "Zimidara Self Purchase";
        int dueDays = 0;
        double goodsAmount = 0.0;
        double labourAmount = 0.0;
        double roundOff = 0.0;
        double grandTotal = 0.0;
        QString narration = "";

        // Check Transactions for header details
        auto itTx = jformTxGroups.find(k);
        if (itTx != jformTxGroups.end()) {
            for (const auto& tr : itTx->second) {
                std::string invNo = DataPreprocessor::cleanText(getFieldStr(tr, JF("jform_no")));
                if (!invNo.empty()) jformNo = QString::fromStdString(invNo);

                std::string stNarr = DataPreprocessor::cleanText(getFieldStr(tr, JF("narration")));
                if (!stNarr.empty() && narration.isEmpty()) narration = QString::fromStdString(stNarr);

                std::string zName = DataPreprocessor::cleanText(getFieldStr(tr, JF("zimidar_name")));
                if (!zName.empty() && zimidarName.isEmpty()) zimidarName = QString::fromStdString(zName);

                std::string jStatus = DataPreprocessor::cleanText(getFieldStr(tr, JF("auction_sale_status")));
                if (!jStatus.empty()) auctionStatus = QString::fromStdString(jStatus);

                int dDays = DataPreprocessor::parseInt(getFieldStr(tr, JF("due_days")));
                if (dDays > 0) dueDays = dDays;

                std::string entryType = DataPreprocessor::cleanText(getFieldStr(tr, "EntryType")); // JSON-gap(JF)
                std::string drCr = DataPreprocessor::cleanText(getFieldStr(tr, VF("dr_cr")));
                double amt = DataPreprocessor::parseDouble(getFieldStr(tr, JF("amount")));
                int acCode = DataPreprocessor::parseInt(getFieldStr(tr, VF("account_code")));

                if (entryType == "Ledger" && drCr == "Cr") {
                    grandTotal = amt;
                    auto itP = ledgerDetailMap.find(acCode);
                    if (itP != ledgerDetailMap.end()) {
                        zimidarId = itP->second.id;
                        if (zimidarName.isEmpty()) zimidarName = QString::fromStdString(itP->second.name);
                    }
                } else if (entryType == "JFormIFormLedger" || drCr == "Dr") {
                    goodsAmount = amt;
                    auto itP = ledgerDetailMap.find(acCode);
                    if (itP != ledgerDetailMap.end()) {
                        partyId = itP->second.id;
                        partyName = QString::fromStdString(itP->second.name);
                    }
                } else if (entryType == "Labour") {
                    labourAmount = amt;
                } else if (entryType == "Round Off") {
                    roundOff = (drCr == "Dr" ? amt : -amt);
                }
            }
        }

        // Sum items from stock transactions
        int totalBags = 0;
        double totalWeight = 0.0;
        double stockGoodsAmount = 0.0;

        auto itStock = jformStockGroups.find(k);
        if (itStock != jformStockGroups.end()) {
            for (const auto& st : itStock->second) {
                totalBags += DataPreprocessor::parseInt(getFieldStr(st, JF("bags")));
                totalWeight += DataPreprocessor::parseDouble(getFieldStr(st, JF("weight")));
                stockGoodsAmount += DataPreprocessor::parseDouble(getFieldStr(st, JF("amount")));
                if (zimidarId == 0) {
                    int dpCode = DataPreprocessor::parseInt(getFieldStr(st, "DheriPurchaseFrom")); // JSON-gap(JF)
                    if (dpCode == 0) dpCode = DataPreprocessor::parseInt(getFieldStr(st, "CommissionPartyCode")); // JSON-gap(JF)
                    auto itP = ledgerDetailMap.find(dpCode);
                    if (itP != ledgerDetailMap.end()) {
                        zimidarId = itP->second.id;
                        if (zimidarName.isEmpty()) zimidarName = QString::fromStdString(itP->second.name);
                    }
                }
            }
        }

        if (goodsAmount <= 0.001) goodsAmount = stockGoodsAmount;
        if (grandTotal <= 0.001) grandTotal = goodsAmount - labourAmount + roundOff;
        if (zimidarName.isEmpty()) zimidarName = "Farmer / Zimidar";

        db.executeNonQuery(
            "INSERT INTO jform_vouchers ("
            "fy_id, financial_year, voucher_no, voucher_date, jform_no, zimidar_id, zimidar_name, "
            "party_id, party_name, auction_sale_status, due_days, total_bags, total_weight, "
            "goods_amount, bonus_amount, relief_amount, subtotal_amount, labour_amount, "
            "round_off, grand_total, narration) "
            "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, 0.0, 0.0, ?, ?, ?, ?, ?);",
            {
                (fyId > 0 ? QVariant(fyId) : QVariant()), fyVal, vchNum, vDate, jformNo,
                (zimidarId > 0 ? QVariant(zimidarId) : QVariant()), zimidarName,
                (partyId > 0 ? QVariant(partyId) : QVariant()), partyName,
                auctionStatus, dueDays, totalBags, totalWeight,
                goodsAmount, goodsAmount, labourAmount, roundOff, grandTotal, narration
            }
        );

        QVariant newJFormId = db.executeScalar("SELECT last_insert_rowid();");
        int jfVoucherId = newJFormId.toInt();

        // Insert line items
        if (itStock != jformStockGroups.end()) {
            for (const auto& st : itStock->second) {
                int iCode = DataPreprocessor::parseInt(getFieldStr(st, JF("item_id")));
                int itemId = 0;
                std::string itemName = "Paddy";
                auto itItem = itemCodeMap.find(iCode);
                if (itItem != itemCodeMap.end()) {
                    itemId = itItem->second.id;
                    itemName = itItem->second.name;
                }

                int bags = DataPreprocessor::parseInt(getFieldStr(st, JF("bags")));
                double loose = DataPreprocessor::parseDouble(getFieldStr(st, JF("loose_weight")));
                double packing = DataPreprocessor::parseDouble(getFieldStr(st, JF("packing")), 0.500);
                double wt = DataPreprocessor::parseDouble(getFieldStr(st, JF("weight")));
                double rate = DataPreprocessor::parseDouble(getFieldStr(st, JF("rate")));
                double amt = DataPreprocessor::parseDouble(getFieldStr(st, JF("amount")));

                db.executeNonQuery(
                    "INSERT INTO jform_voucher_items ("
                    "voucher_id, voucher_no, item_id, item_name, bags, loose_weight, packing, weight, rate, amount) "
                    "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?);",
                    {
                        jfVoucherId, vchNum, (itemId > 0 ? QVariant(itemId) : QVariant()), QString::fromStdString(itemName),
                        bags, loose, packing, wt, rate, amt
                    }
                );
            }
        }
        jformCount++;
    }
    std::cout << "[INFO] Migrated " << jformCount << " J-Form vouchers!" << std::endl;
    return true;
}

bool MandiMigrator::migrate_iform_bahikhata(
    MigrationContext& ctx,
    const std::vector<std::map<std::string, std::string>>& transRows,
    const std::vector<std::map<std::string, std::string>>& stockTransRows,
    const std::map<int, PartyDetail>& ledgerDetailMap,
    const std::map<int, ItemDetail>& itemCodeMap,
    std::map<QString, int>& fyNameToId
) {
    // C3 slice M1 (monolith PASS 3.6): transplanted verbatim. Same key
    // split as J-Form (IF for form keys, VF for generic leg keys).
    // Writes iform_vouchers + items — never transport_dispatches (the
    // prototype mismatch the recon flagged). F5 warn+count on fy skips.
    auto IF = [](const char* t) { return MigrationColumns::iformField(QString::fromLatin1(t)).toStdString(); };
    auto VF = [](const char* t) { return MigrationColumns::voucherField(QString::fromLatin1(t)).toStdString(); };
    auto& db = DatabaseManager::instance();
    ctx.updateProgress(90, "Migrating I-Form Mandi Buyer Issue Vouchers...");

    // Group StockTransactions where TransType == 'IFrm' by (VoucherNumber, VoucherDate)
    std::map<std::pair<std::string, std::string>, std::vector<std::map<std::string, std::string>>> iformStockGroups;
    for (const auto& st : stockTransRows) {
        std::string tt = DataPreprocessor::cleanText(getFieldStr(st, "TransType")); // JSON-gap(IF)
        if (tt != "IFrm") continue;
        std::string vNo = DataPreprocessor::cleanText(getFieldStr(st, IF("voucher_no")));
        QString vDate = parseDateFormatted(QString::fromStdString(getFieldStr(st, IF("voucher_date"))));
        iformStockGroups[{vNo, vDate.toStdString()}].push_back(st);
    }

    // Group Transactions where TransType == 'IFrm' by (VoucherNumber, VoucherDate)
    std::map<std::pair<std::string, std::string>, std::vector<std::map<std::string, std::string>>> iformTxGroups;
    for (const auto& tr : transRows) {
        std::string tt = DataPreprocessor::cleanText(getFieldStr(tr, "TransType")); // JSON-gap(IF)
        if (tt != "IFrm") continue;
        std::string vNo = DataPreprocessor::cleanText(getFieldStr(tr, IF("voucher_no")));
        QString vDate = parseDateFormatted(QString::fromStdString(getFieldStr(tr, IF("voucher_date"))));
        iformTxGroups[{vNo, vDate.toStdString()}].push_back(tr);
    }

    int iformCount = 0;
    std::set<std::pair<std::string, std::string>> allIFormKeys;
    for (const auto& p : iformStockGroups) allIFormKeys.insert(p.first);
    for (const auto& p : iformTxGroups) allIFormKeys.insert(p.first);

    for (const auto& k : allIFormKeys) {
        std::string vNo = k.first;
        QString vDate = QString::fromStdString(k.second);
        int vchNum = DataPreprocessor::parseInt(vNo, 1);
        QString fyVal = computeFinancialYear(vDate);
        int fyId = MigrationUtils::fyIdForStd(fyNameToId, fyVal);
        // F5: fail loud with row context (was a silent skip).
        if (fyId <= 0) { ++ctx.stats.unmappedFyRows; qWarning() << "[Migration] I-Form skipped (unresolvable FY):" << QString::fromStdString(vNo) << fyVal; continue; }

        QString iformNo = QString::fromStdString(vNo);
        int buyerId = 0;
        QString buyerName = "";
        QString brokerName = "";
        QString vehicleNo = "";
        int dueDays = 0;
        double goodsAmount = 0.0;
        double damiAmount = 0.0;
        double damiRate = 2.5;
        double mandiFeeAmount = 0.0;
        double mandiFeeRate = 2.0;
        double hrdfAmount = 0.0;
        double hrdfRate = 0.5;
        double labourAmount = 0.0;
        double taxAmount = 0.0;
        double roundOff = 0.0;
        double grandTotal = 0.0;
        QString narration = "";

        // Check Transactions for header and breakdown details
        auto itTx = iformTxGroups.find(k);
        if (itTx != iformTxGroups.end()) {
            for (const auto& tr : itTx->second) {
                std::string invNo = DataPreprocessor::cleanText(getFieldStr(tr, IF("iform_no")));
                if (!invNo.empty()) iformNo = QString::fromStdString(invNo);

                std::string stNarr = DataPreprocessor::cleanText(getFieldStr(tr, IF("narration")));
                if (!stNarr.empty() && narration.isEmpty()) narration = QString::fromStdString(stNarr);

                std::string bName = DataPreprocessor::cleanText(getFieldStr(tr, IF("broker_name")));
                if (!bName.empty() && brokerName.isEmpty()) brokerName = QString::fromStdString(bName);

                int dDays = DataPreprocessor::parseInt(getFieldStr(tr, IF("due_days")));
                if (dDays > 0) dueDays = dDays;

                std::string entryType = DataPreprocessor::cleanText(getFieldStr(tr, "EntryType")); // JSON-gap(IF)
                std::string drCr = DataPreprocessor::cleanText(getFieldStr(tr, VF("dr_cr")));
                double amt = DataPreprocessor::parseDouble(getFieldStr(tr, IF("amount")));
                int acCode = DataPreprocessor::parseInt(getFieldStr(tr, VF("account_code")));
                std::string acName = "";
                auto itP = ledgerDetailMap.find(acCode);
                if (itP != ledgerDetailMap.end()) {
                    acName = itP->second.name;
                }
                std::string acNameLower = DataPreprocessor::toLowerStr(acName);

                if (drCr == "Dr") {
                    grandTotal = amt;
                    if (buyerId == 0 && itP != ledgerDetailMap.end()) {
                        buyerId = itP->second.id;
                        buyerName = QString::fromStdString(itP->second.name);
                    }
                } else if (drCr == "Cr") {
                    if (acNameLower.find("dami") != std::string::npos || acNameLower.find("commission") != std::string::npos) {
                        damiAmount += amt;
                    } else if (acNameLower.find("market fee") != std::string::npos || acNameLower.find("mkt fee") != std::string::npos) {
                        mandiFeeAmount += amt;
                    } else if (acNameLower.find("h.r.d.f") != std::string::npos || acNameLower.find("hrdf") != std::string::npos) {
                        hrdfAmount += amt;
                    } else if (acNameLower.find("labour") != std::string::npos || acNameLower.find("hamali") != std::string::npos) {
                        labourAmount += amt;
                    } else if (acNameLower.find("vat") != std::string::npos || acNameLower.find("gst") != std::string::npos || acNameLower.find("tax") != std::string::npos) {
                        taxAmount += amt;
                    } else if (acNameLower.find("maal") != std::string::npos || acNameLower.find("sale") != std::string::npos || acNameLower.find("purchase") != std::string::npos) {
                        goodsAmount += amt;
                    } else if (entryType == "Round Off") {
                        roundOff -= amt;
                    }
                }
            }
        }

        // Sum items from stock transactions
        int totalBags = 0;
        double totalWeight = 0.0;
        double stockGoodsAmount = 0.0;

        auto itStock = iformStockGroups.find(k);
        if (itStock != iformStockGroups.end()) {
            for (const auto& st : itStock->second) {
                totalBags += DataPreprocessor::parseInt(getFieldStr(st, IF("bags")));
                totalWeight += DataPreprocessor::parseDouble(getFieldStr(st, IF("weight")));
                stockGoodsAmount += DataPreprocessor::parseDouble(getFieldStr(st, IF("amount")));
                double mfRate = DataPreprocessor::parseDouble(getFieldStr(st, IF("mandi_fee_rate")));
                if (mfRate > 0) mandiFeeRate = mfRate;
                double hrRate = DataPreprocessor::parseDouble(getFieldStr(st, IF("hrdf_rate")));
                if (hrRate > 0) hrdfRate = hrRate;
            }
        }

        if (goodsAmount <= 0.001) goodsAmount = stockGoodsAmount;
        if (grandTotal <= 0.001) grandTotal = goodsAmount + damiAmount + mandiFeeAmount + hrdfAmount + labourAmount + taxAmount + roundOff;
        if (buyerName.isEmpty()) buyerName = "Mandi Buyer";

        db.executeNonQuery(
            "INSERT INTO iform_vouchers ("
            "fy_id, financial_year, voucher_no, voucher_date, iform_no, buyer_id, buyer_name, "
            "broker_name, due_days, vehicle_no, total_bags, total_weight, "
            "goods_amount, dami_rate, dami_amount, mandi_fee_rate, mandi_fee_amount, "
            "hrdf_rate, hrdf_amount, labour_amount, taxable_amount, tax_amount, "
            "round_off, grand_total, narration) "
            "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);",
            {
                (fyId > 0 ? QVariant(fyId) : QVariant()), fyVal, vchNum, vDate, iformNo,
                (buyerId > 0 ? QVariant(buyerId) : QVariant()), buyerName,
                brokerName, dueDays, vehicleNo, totalBags, totalWeight,
                goodsAmount, damiRate, damiAmount, mandiFeeRate, mandiFeeAmount,
                hrdfRate, hrdfAmount, labourAmount, goodsAmount, taxAmount,
                roundOff, grandTotal, narration
            }
        );

        QVariant newIFormId = db.executeScalar("SELECT last_insert_rowid();");
        int ifVoucherId = newIFormId.toInt();

        // Insert line items
        if (itStock != iformStockGroups.end()) {
            for (const auto& st : itStock->second) {
                int iCode = DataPreprocessor::parseInt(getFieldStr(st, IF("item_id")));
                int itemId = 0;
                std::string itemName = "Paddy";
                auto itItem = itemCodeMap.find(iCode);
                if (itItem != itemCodeMap.end()) {
                    itemId = itItem->second.id;
                    itemName = itItem->second.name;
                }

                int bags = DataPreprocessor::parseInt(getFieldStr(st, IF("bags")));
                double loose = DataPreprocessor::parseDouble(getFieldStr(st, IF("loose_weight")));
                double packing = DataPreprocessor::parseDouble(getFieldStr(st, IF("packing")), 0.500);
                double wt = DataPreprocessor::parseDouble(getFieldStr(st, IF("weight")));
                double rate = DataPreprocessor::parseDouble(getFieldStr(st, IF("rate")));
                double amt = DataPreprocessor::parseDouble(getFieldStr(st, IF("amount")));
                double lineDami = (amt * damiRate) / 100.0;
                double lineMFee = (amt * mandiFeeRate) / 100.0;
                double lineHRDF = (amt * hrdfRate) / 100.0;

                db.executeNonQuery(
                    "INSERT INTO iform_voucher_items ("
                    "voucher_id, voucher_no, item_id, item_name, bags, loose_weight, packing, weight, rate, amount, "
                    "dami_rate, dami_amount, mandi_fee_rate, mandi_fee_amount, hrdf_rate, hrdf_amount, labour_amount, tax_rate, tax_amount) "
                    "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, 0.0, 0.0, 0.0);",
                    {
                        ifVoucherId, vchNum, (itemId > 0 ? QVariant(itemId) : QVariant()), QString::fromStdString(itemName),
                        bags, loose, packing, wt, rate, amt,
                        damiRate, lineDami, mandiFeeRate, lineMFee, hrdfRate, lineHRDF
                    }
                );
            }
        }
        iformCount++;
    }
    std::cout << "[INFO] Migrated " << iformCount << " I-Form vouchers!" << std::endl;
    return true;
}

// Default arg lives on the forward declaration above (line ~77); C++ forbids
// repeating it here. M1 build-unblock (Agent 2, announced).
bool MandiMigrator::migrate_tds_bahikhata(
    MigrationContext& ctx,
    const std::vector<std::map<std::string, std::string>>& transRows,
    const std::vector<std::map<std::string, std::string>>& tdsRows,
    const std::map<int, PartyDetail>& ledgerDetailMap,
    std::map<QString, int>& fyNameToId
) {
    // C3 slice M2 (monolith PASS 5.4): transplanted verbatim. TD keys via
    // tdsField() (compound RateTDS|TDSRate via ordered fallback);
    // Transactions-source reads via voucherField(). Silent fy skips gained
    // the F5 warn+count (row output unchanged).
    auto TD = [](const char* t) { return MigrationColumns::tdsField(QString::fromLatin1(t)).toStdString(); };
    auto VF = [](const char* t) { return MigrationColumns::voucherField(QString::fromLatin1(t)).toStdString(); };
    auto& db = DatabaseManager::instance();
    ctx.updateProgress(93, QString("Migrating %1 Historical TDS Vouchers...").arg(tdsRows.size()));

    // Group Transactions where TransType == 'Jrnl' by (VoucherNumber, VoucherDate)
    std::map<std::pair<int, std::string>, std::vector<std::map<std::string, std::string>>> jrnlTxGroups;
    for (const auto& tr : transRows) {
        std::string tt = DataPreprocessor::cleanText(getFieldStr(tr, "TransType")); // JSON-gap(TD): filter VALUE, not a column read
        if (tt != "Jrnl") continue;
        int vNo = DataPreprocessor::parseInt(getFieldStr(tr, VF("voucher_no")));
        QString vDate = parseDateFormatted(QString::fromStdString(getFieldStr(tr, VF("voucher_date"))));
        jrnlTxGroups[{vNo, vDate.toStdString()}].push_back(tr);
    }

    int tdsCount = 0;
    for (const auto& tr : tdsRows) {
        QString vDate = parseDateFormatted(QString::fromStdString(getFieldStr(tr, TD("voucher_date"))));
        if (vDate.isEmpty()) continue;

        int vchNum = DataPreprocessor::parseInt(getFieldStr(tr, TD("voucher_no")));
        int partyCode = DataPreprocessor::parseInt(getFieldStr(tr, TD("ledger_id")));
        int partyId = 0;
        QString partyName = "";
        auto itP = ledgerDetailMap.find(partyCode);
        if (itP != ledgerDetailMap.end()) {
            partyId = itP->second.id;
            partyName = QString::fromStdString(itP->second.name);
        }
        if (partyName.isEmpty()) partyName = "TDS Deductee";

        double incAmt = DataPreprocessor::parseDouble(getFieldStr(tr, TD("income_amount")));
        double prevAmt = DataPreprocessor::parseDouble(getFieldStr(tr, TD("previous_amount")));
        double totTds = incAmt + prevAmt;
        double rateTds = DataPreprocessor::parseDouble(getFieldFirstClean(tr, TD("rate_tds")));
        double amtTds = DataPreprocessor::parseDouble(getFieldStr(tr, TD("tax_amount_tds")));
        double rateSur = DataPreprocessor::parseDouble(getFieldStr(tr, TD("rate_surcharge")));
        double amtSur = DataPreprocessor::parseDouble(getFieldStr(tr, TD("tax_amount_surcharge")));
        double rateCess = DataPreprocessor::parseDouble(getFieldStr(tr, TD("rate_cess")));
        double amtCess = DataPreprocessor::parseDouble(getFieldStr(tr, TD("tax_amount_cess")));
        double totTaxRate = rateTds + rateSur + rateCess;
        double totTaxAmt = amtTds + amtSur + amtCess;
        double netAmt = incAmt - totTaxAmt;
        if (netAmt < 0.0) netAmt = 0.0;

        QString incType = QString::fromStdString(DataPreprocessor::cleanText(getFieldStr(tr, TD("tds_type")))).trimmed().toUpper();
        if (incType.isEmpty()) incType = "RENT";

        QString nonDeductReason = QString::fromStdString(DataPreprocessor::cleanText(getFieldStr(tr, TD("non_deduction_reason"))));
        int postInBooks = DataPreprocessor::parseInt(getFieldStr(tr, TD("post_in_books")), 1);
        QString narration = QString::fromStdString(DataPreprocessor::cleanText(getFieldStr(tr, TD("narration"))));

        QDate dt = QDate::fromString(vDate, "yyyy-MM-dd");
        QString dayOfWeek = dt.toString("dddd");

        QString fyVal = computeFinancialYear(vDate);
        int fyId = MigrationUtils::fyIdForStd(fyNameToId, fyVal);
        // F5: fail loud with row context (was a silent skip).
        if (fyId <= 0) { ++ctx.stats.unmappedFyRows; qWarning() << "[Migration] TDS row skipped (unresolvable FY):" << vchNum << fyVal; continue; }

        // Find Expense Dr Ledger and TDS Cr Ledger from Transactions if present
        int expLedgerId = 0;
        QString expLedgerName = incType + " Expense";
        int tdsLedgerId = 0;
        QString tdsLedgerName = "T D S PEYABAL";

        auto itTrGroup = jrnlTxGroups.find({vchNum, vDate.toStdString()});
        if (itTrGroup != jrnlTxGroups.end()) {
            for (const auto& tx : itTrGroup->second) {
                std::string entryType = DataPreprocessor::cleanText(getFieldStr(tx, "EntryType")); // JSON-gap(TD)
                int acCode = DataPreprocessor::parseInt(getFieldStr(tx, VF("account_code")));
                auto itLedger = ledgerDetailMap.find(acCode);
                if (itLedger != ledgerDetailMap.end()) {
                    if (entryType == "Exp.") {
                        expLedgerId = itLedger->second.id;
                        expLedgerName = QString::fromStdString(itLedger->second.name);
                    } else if (entryType == "TDS") {
                        tdsLedgerId = itLedger->second.id;
                        tdsLedgerName = QString::fromStdString(itLedger->second.name);
                    }
                }
            }
        }

        db.executeNonQuery(
            "INSERT INTO tds_vouchers ("
            "fy_id, financial_year, voucher_no, voucher_date, day_of_week, post_in_books, tds_type, "
            "ledger_id, ledger_name, income_amount, previous_amount, total_for_tds, narration, "
            "rate_tds, tax_amount_tds, rate_surcharge, tax_amount_surcharge, rate_cess, tax_amount_cess, "
            "use_rounded_total, total_tax_rate, total_tax_amount, net_amount, non_deduction_reason, "
            "exp_ledger_id, exp_ledger_name, tds_ledger_id, tds_ledger_name) "
            "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, 1, ?, ?, ?, ?, ?, ?, ?, ?);",
            {
                (fyId > 0 ? QVariant(fyId) : QVariant()), fyVal, vchNum, vDate, dayOfWeek, postInBooks, incType,
                (partyId > 0 ? QVariant(partyId) : QVariant()), partyName, incAmt, prevAmt, totTds, narration,
                rateTds, amtTds, rateSur, amtSur, rateCess, amtCess,
                totTaxRate, totTaxAmt, netAmt, nonDeductReason,
                (expLedgerId > 0 ? QVariant(expLedgerId) : QVariant()), expLedgerName, (tdsLedgerId > 0 ? QVariant(tdsLedgerId) : QVariant()), tdsLedgerName
            }
        );
        tdsCount++;
    }
    std::cout << "[INFO] Migrated " << tdsCount << " TDS vouchers!" << std::endl;
    return true;
}

bool MandiMigrator::migrate_closing_bahikhata(
    MigrationContext& ctx,
    const std::vector<std::map<std::string, std::string>>& customRows,
    const std::map<int, ItemDetail>& itemCodeMap,
    std::map<QString, int>& fyNameToId
) {
    // C3 slice M2 (monolith PASS 5.5): transplanted verbatim. Fully
    // JSON-covered — zero gap literals. Silent fy skips gained the F5
    // warn+count (row output unchanged).
    auto CL = [](const char* t) { return MigrationColumns::closingField(QString::fromLatin1(t)).toStdString(); };
    auto& db = DatabaseManager::instance();
    ctx.updateProgress(95, QString("Migrating %1 Audited Custom Closing Stock Records...").arg(customRows.size()));
    db.executeNonQuery("DELETE FROM custom_closing_stocks;");

    for (const auto& cr : customRows) {
        QString cDate = parseDateFormatted(QString::fromStdString(getFieldStr(cr, CL("closing_date"))));
        if (cDate.isEmpty()) continue;

        QString cFy = computeFinancialYear(cDate);
        int cFyId = MigrationUtils::fyIdForStd(fyNameToId, cFy);
        // F5: fail loud with row context (was a silent skip).
        if (cFyId <= 0) { ++ctx.stats.unmappedFyRows; qWarning() << "[Migration] closing-stock row skipped (unresolvable FY):" << cDate << cFy; continue; }

        std::string cItemCode = DataPreprocessor::cleanText(getFieldStr(cr, CL("item_id")));
        int iCode = DataPreprocessor::parseInt(cItemCode);
        auto itItem = itemCodeMap.find(iCode);
        int cItemId = (itItem != itemCodeMap.end()) ? itItem->second.id : 0;
        std::string cItemName = (itItem != itemCodeMap.end()) ? itItem->second.name : ("Item #" + cItemCode);

        int cBags = DataPreprocessor::parseInt(getFieldStr(cr, CL("bags")));
        double cWt = DataPreprocessor::parseDouble(getFieldStr(cr, CL("weight_qtl")));
        double cAmt = DataPreprocessor::parseDouble(getFieldStr(cr, CL("amount")));
        double cRate = (cWt > 0.0) ? std::round((cAmt / cWt) * 100.0) / 100.0 : 0.0;

        db.executeNonQuery(
            "INSERT INTO custom_closing_stocks ("
            "fy_id, financial_year, closing_date, item_id, item_code, item_name, bags, weight_qtl, rate, amount) "
            "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?);",
            {
                (cFyId > 0 ? QVariant(cFyId) : QVariant()),
                cFy,
                cDate,
                (cItemId > 0 ? QVariant(cItemId) : QVariant()),
                QString::fromStdString(cItemCode),
                QString::fromStdString(cItemName),
                cBags,
                cWt,
                cRate,
                cAmt
            }
        );
    }
    return true;
}

bool MandiMigrator::migrate_bardana_bahikhata(
    MigrationContext& ctx,
    const std::vector<std::map<std::string, std::string>>& bardanaRows,
    const std::map<int, std::string>& ledgerCodeMap,
    const std::map<int, PartyDetail>& ledgerDetailMap,
    const std::map<int, std::string>& ledgerDeletedNameMap,
    std::map<QString, int>& fyNameToId
) {
    // C3 slice M2 (monolith PASS 5.6): transplanted verbatim. Fully
    // JSON-covered (fy is a computed label, not a lookup). No fyId gates
    // in the live pass — kept verbatim.
    auto BD = [](const char* t) { return MigrationColumns::bardanaField(QString::fromLatin1(t)).toStdString(); };
    auto& db = DatabaseManager::instance();
    Q_UNUSED(fyNameToId);
    int bardanaCount = 0;
    if (!bardanaRows.empty()) {
        ctx.updateProgress(96, QString("Migrating %1 Historical Bardana (Gunny Bag) Transactions...").arg(bardanaRows.size()));
        for (const auto& br : bardanaRows) {
            QString vDate = parseDateFormatted(QString::fromStdString(getFieldStr(br, BD("voucher_date"))));
            if (vDate.isEmpty()) continue;

            QString fyVal = computeFinancialYear(vDate);
            int vchNum = DataPreprocessor::parseInt(getFieldStr(br, BD("voucher_no")));
            QString vchNoStr = vchNum > 0 ? QString("BD-%1").arg(vchNum) : QString::fromStdString(DataPreprocessor::cleanText(getFieldStr(br, BD("bill_no"))));
            if (vchNoStr.isEmpty()) vchNoStr = QString("BD-%1").arg(bardanaCount + 1);

            QString vchType = QString::fromStdString(DataPreprocessor::cleanText(getFieldStr(br, BD("vch_type"))));
            if (vchType.isEmpty()) vchType = "ISSUE";

            int acCode = DataPreprocessor::parseInt(getFieldStr(br, BD("party_id")));
            int partyId = 0;
            QString partyName = "";
            auto itP = ledgerDetailMap.find(acCode);
            if (itP != ledgerDetailMap.end()) {
                partyId = itP->second.id;
                partyName = QString::fromStdString(itP->second.name);
            }
            if (partyName.isEmpty()) {
                std::string delName = lookupLedgerName(acCode, ledgerDetailMap, ledgerCodeMap, ledgerDeletedNameMap);
                partyName = delName.empty() ? QString("Party #%1").arg(acCode)
                                            : QString::fromStdString(delName);
            }

            QString bardanaType = QString::fromStdString(DataPreprocessor::cleanText(getFieldStr(br, BD("bardana_type"), "Jute 50kg (Pukka)")));
            if (bardanaType.isEmpty()) bardanaType = "Jute 50kg (Pukka)";

            QString godownName = QString::fromStdString(DataPreprocessor::cleanText(getFieldStr(br, BD("godown_name"), "Main Godown")));
            if (godownName.isEmpty()) godownName = "Main Godown";

            QString drCr = (vchType.toUpper().contains("ISSUE") || vchType.toUpper().contains("SALE")) ? "Dr" : "Cr";
            int qty = DataPreprocessor::parseInt(getFieldStr(br, BD("qty")));
            double rate = DataPreprocessor::parseDouble(getFieldStr(br, BD("rate")), 0.0);
            double amount = std::round((qty * rate) * 100.0) / 100.0;
            QString vehNo = QString::fromStdString(DataPreprocessor::cleanText(getFieldStr(br, BD("vehicle_no"))));
            QString billNo = QString::fromStdString(DataPreprocessor::cleanText(getFieldStr(br, BD("bill_no"))));
            QString narration = QString::fromStdString(DataPreprocessor::cleanText(getFieldStr(br, BD("narration"))));

            db.executeNonQuery(
                "INSERT INTO bardana_transactions ("
                "  financial_year, voucher_no, voucher_date, vch_type, party_id, party_name, "
                "  bardana_type, godown_name, dr_cr, qty, rate, amount, vehicle_no, bill_no, narration"
                ") VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);",
                {
                    fyVal, vchNoStr, vDate, vchType,
                    (partyId > 0 ? QVariant(partyId) : QVariant()), partyName,
                    bardanaType, godownName, drCr, qty, rate, amount, vehNo, billNo, narration
                }
            );
            bardanaCount++;
        }
        std::cout << "[INFO] Migrated " << bardanaCount << " Bardana transactions!" << std::endl;
    }
    return true;
}

bool MandiMigrator::migrate_gate_bahikhata(
    MigrationContext& ctx,
    const std::vector<std::map<std::string, std::string>>& gatePassRows,
    const std::map<int, PartyDetail>& ledgerDetailMap,
    const std::map<int, ItemDetail>& itemCodeMap
) {
    // C3 slice M2 (monolith PASS 5.7): transplanted verbatim. GateRegister
    // source rows are intentionally unread (mirror-only JSON coverage).
    auto GP = [](const char* t) { return MigrationColumns::gatepassField(QString::fromLatin1(t)).toStdString(); };
    auto& db = DatabaseManager::instance();
    int gatePassCount = 0;
    if (!gatePassRows.empty()) {
        ctx.updateProgress(97, QString("Migrating %1 Gate Pass Records & Gate Log...").arg(gatePassRows.size()));
        for (const auto& gr : gatePassRows) {
            QString gDate = parseDateFormatted(QString::fromStdString(getFieldFirstClean(gr, GP("entry_date"))));
            if (gDate.isEmpty()) continue;

            QString fyVal = computeFinancialYear(gDate);
            int gpNo = DataPreprocessor::parseInt(getFieldStr(gr, GP("gate_pass_no")));
            QString passNoStr = gpNo > 0 ? QString("GP-%1").arg(gpNo, 5, 10, QChar('0')) : QString("GP-%1").arg(gatePassCount + 1, 5, 10, QChar('0'));

            int pCode = DataPreprocessor::parseInt(getFieldStr(gr, GP("party_id")));
            int partyId = 0;
            QString partyName = "";
            auto itP = ledgerDetailMap.find(pCode);
            if (itP != ledgerDetailMap.end()) {
                partyId = itP->second.id;
                partyName = QString::fromStdString(itP->second.name);
            }
            if (partyName.isEmpty()) partyName = "Consignee Party";

            int iCode = DataPreprocessor::parseInt(getFieldStr(gr, GP("commodity")));
            QString commodity = "Paddy / Rice";
            auto itI = itemCodeMap.find(iCode);
            if (itI != itemCodeMap.end()) {
                commodity = QString::fromStdString(itI->second.name);
            }

            int bags = DataPreprocessor::parseInt(getFieldStr(gr, GP("bag_count")));
            double grossWt = DataPreprocessor::parseDouble(getFieldFirstClean(gr, GP("gross_weight_qtl")));
            double netWt = grossWt;
            QString vehNo = QString::fromStdString(DataPreprocessor::cleanText(getFieldStr(gr, GP("vehicle_no"))));
            QString driver = QString::fromStdString(DataPreprocessor::cleanText(getFieldStr(gr, GP("driver_name"))));
            QString narr = QString::fromStdString(DataPreprocessor::cleanText(getFieldStr(gr, GP("remarks"))));

            db.executeNonQuery(
                "INSERT INTO gate_register ("
                "  financial_year, gate_pass_no, entry_date, entry_time, direction, purpose, "
                "  vehicle_no, driver_name, driver_phone, transport, party_id, party_name, "
                "  commodity, bag_count, gross_weight_qtl, tare_weight_qtl, net_weight_qtl, status, remarks"
                ") VALUES (?, ?, ?, '10:00', 'OUTWARD', 'Dispatch Delivery', ?, ?, '', 'Self', ?, ?, ?, ?, ?, 0.0, ?, 'COMPLETED', ?);",
                {
                    fyVal, passNoStr, gDate, vehNo, driver,
                    (partyId > 0 ? QVariant(partyId) : QVariant()), partyName,
                    commodity, bags, grossWt, netWt, narr
                }
            );
            gatePassCount++;
        }
        std::cout << "[INFO] Migrated " << gatePassCount << " Gate passes!" << std::endl;
    }
    return true;
}

bool MandiMigrator::migrate_sauda_bahikhata(
    MigrationContext& ctx,
    const std::vector<std::map<std::string, std::string>>& saudaVchRows,
    const std::map<int, PartyDetail>& ledgerDetailMap,
    const std::map<int, ItemDetail>& itemCodeMap,
    std::map<QString, int>& fyNameToId
) {
    // C3 slice M2 (monolith PASS 5.8): transplanted verbatim. SaudaTransactions
    // source rows are intentionally unread (mirror-only JSON coverage) — the
    // live pass consumes voucher rows only. VoucherNumber stays a gap
    // literal (numbering fallback, not a column read).
    auto SD = [](const char* t) { return MigrationColumns::saudaField(QString::fromLatin1(t)).toStdString(); };
    auto& db = DatabaseManager::instance();
    Q_UNUSED(fyNameToId);
    int saudaCount = 0;
    if (!saudaVchRows.empty()) {
        ctx.updateProgress(98, QString("Migrating %1 Sauda Forward Contracts...").arg(saudaVchRows.size()));
        for (const auto& sv : saudaVchRows) {
            QString sDate = parseDateFormatted(QString::fromStdString(getFieldStr(sv, SD("sauda_date"))));
            if (sDate.isEmpty()) continue;

            QString fyVal = computeFinancialYear(sDate);
            QString sNo = QString::fromStdString(DataPreprocessor::cleanText(getFieldStr(sv, SD("sauda_no"))));
            if (sNo.isEmpty()) {
                int vNum = DataPreprocessor::parseInt(getFieldStr(sv, "VoucherNumber")); // JSON-gap(SD): numbering fallback, not a column read
                sNo = vNum > 0 ? QString("SD-%1").arg(vNum, 5, 10, QChar('0')) : QString("SD-%1").arg(saudaCount + 1, 5, 10, QChar('0'));
            }

            QString sType = QString::fromStdString(DataPreprocessor::cleanText(getFieldStr(sv, SD("sauda_type")))).toUpper();
            if (!sType.contains("BUY") && !sType.contains("PURCHASE")) sType = "SALE";
            else sType = "BUY";

            int pCode = DataPreprocessor::parseInt(getFieldStr(sv, SD("party_id")));
            int partyId = 0;
            QString partyName = "";
            auto itP = ledgerDetailMap.find(pCode);
            if (itP != ledgerDetailMap.end()) {
                partyId = itP->second.id;
                partyName = QString::fromStdString(itP->second.name);
            }
            if (partyName.isEmpty()) partyName = "Party Trader";

            QString broker = QString::fromStdString(DataPreprocessor::cleanText(getFieldStr(sv, SD("broker_name"))));
            QString itemName = QString::fromStdString(DataPreprocessor::cleanText(getFieldStr(sv, SD("item_name"))));
            if (itemName.isEmpty()) {
                int iCode = DataPreprocessor::parseInt(getFieldStr(sv, SD("item_id")));
                auto itI = itemCodeMap.find(iCode);
                if (itI != itemCodeMap.end()) itemName = QString::fromStdString(itI->second.name);
                else itemName = "Basmati Rice 1121";
            }

            double weight = DataPreprocessor::parseDouble(getFieldStr(sv, SD("contracted_weight_qtl")));
            double rate = DataPreprocessor::parseDouble(getFieldStr(sv, SD("rate_per_qtl")));
            int bags = static_cast<int>(weight * 2.0);
            std::string condition = DataPreprocessor::cleanText(getFieldStr(sv, SD("condition_notes")));
            QString dueDt = parseDateFormatted(QString::fromStdString(getFieldStr(sv, SD("delivery_to"))));
            if (dueDt.isEmpty()) dueDt = sDate;

            db.executeNonQuery(
                "INSERT INTO sauda_contracts ("
                "  financial_year, sauda_no, sauda_date, sauda_type, party_id, party_name, "
                "  broker_id, broker_name, item_id, item_name, grade, contracted_bags, "
                "  contracted_weight_qtl, rate_per_qtl, dalali_rate_per_qtl, dalali_pct, "
                "  delivery_from, delivery_to, payment_terms, condition_notes, "
                "  fulfilled_weight_qtl, fulfilled_bags, status"
                ") VALUES (?, ?, ?, ?, ?, ?, NULL, ?, NULL, ?, 'Grade-A', ?, ?, ?, 10.0, 0.0, ?, ?, 'Standard', ?, ?, ?, 'FULFILLED');",
                {
                    fyVal, sNo, sDate, sType,
                    (partyId > 0 ? QVariant(partyId) : QVariant()), partyName,
                    broker, itemName, bags, weight, rate,
                    sDate, dueDt, QString::fromStdString(condition), weight, bags
                }
            );
            saudaCount++;
        }
        std::cout << "[INFO] Migrated " << saudaCount << " Sauda contracts!" << std::endl;
    }
    return true;
}

bool MandiMigrator::migrate_dalali_bahikhata(
    MigrationContext& ctx,
    const std::vector<std::map<std::string, std::string>>& brokerageRows,
    const std::map<int, PartyDetail>& ledgerDetailMap,
    std::map<QString, int>& fyNameToId
) {
    // C3 slice M2 (monolith PASS 5.9): transplanted verbatim. CrLedger
    // stays a gap literal: the JSON maps broker_id <- PartyCode but the
    // code resolves the CrLedger column (different source).
    auto DL = [](const char* t) { return MigrationColumns::dalaliField(QString::fromLatin1(t)).toStdString(); };
    auto& db = DatabaseManager::instance();
    Q_UNUSED(fyNameToId);
    int dalaliCount = 0;
    if (!brokerageRows.empty()) {
        ctx.updateProgress(99, QString("Migrating %1 Brokerage (Dalali) Settlements...").arg(brokerageRows.size()));
        for (const auto& bv : brokerageRows) {
            QString vDate = parseDateFormatted(QString::fromStdString(getFieldStr(bv, DL("settlement_date"))));
            if (vDate.isEmpty()) continue;

            QString fyVal = computeFinancialYear(vDate);
            int vNum = DataPreprocessor::parseInt(getFieldStr(bv, DL("settlement_no")));
            QString settNo = QString("DAL-%1").arg(vNum > 0 ? vNum : (dalaliCount + 1), 5, 10, QChar('0'));

            int crLedger = DataPreprocessor::parseInt(getFieldStr(bv, "CrLedger")); // JSON-gap(DL): code reads CrLedger, JSON maps PartyCode
            int brokerId = 0;
            QString brokerName = "";
            auto itBr = ledgerDetailMap.find(crLedger);
            if (itBr != ledgerDetailMap.end()) {
                brokerId = itBr->second.id;
                brokerName = QString::fromStdString(itBr->second.name);
            }
            if (brokerName.isEmpty()) brokerName = "Dalal / Broker";

            int pCode = DataPreprocessor::parseInt(getFieldStr(bv, DL("broker_id")));
            QString partyName = "Party";
            auto itP = ledgerDetailMap.find(pCode);
            if (itP != ledgerDetailMap.end()) {
                partyName = QString::fromStdString(itP->second.name);
            }

            QString invNo = QString::fromStdString(DataPreprocessor::cleanText(getFieldStr(bv, DL("invoice_no"))));
            double itemWt = DataPreprocessor::parseDouble(getFieldStr(bv, DL("weight_qtl")));
            double itemRate = DataPreprocessor::parseDouble(getFieldStr(bv, DL("rate_per_qtl")));
            double dRate = DataPreprocessor::parseDouble(getFieldStr(bv, DL("dalali_rate")));
            double dAmt = DataPreprocessor::parseDouble(getFieldStr(bv, DL("dalali_amount")));
            if (dAmt <= 0.0 && itemWt > 0.0 && dRate > 0.0) {
                dAmt = std::round((itemWt * dRate) * 100.0) / 100.0;
            }
            double tdsAmt = std::round((dAmt * 0.05) * 100.0) / 100.0;
            double netPayable = dAmt - tdsAmt;
            QString narr = QString::fromStdString(DataPreprocessor::cleanText(getFieldStr(bv, DL("narration"))));

            db.executeNonQuery(
                "INSERT INTO dalali_settlements ("
                "  financial_year, settlement_no, settlement_date, broker_id, broker_name, "
                "  sauda_id, sauda_no, voucher_type, voucher_no, invoice_no, party_name, "
                "  item_name, weight_qtl, rate_per_qtl, dalali_rate, dalali_amount, "
                "  tds_pct, tds_amount, net_dalali_payable, is_posted_to_jv, journal_voucher_no, narration"
                ") VALUES (?, ?, ?, ?, ?, NULL, 'SD-MIGRATED', 'SALE', ?, ?, ?, 'Rice Basmati', ?, ?, ?, ?, 5.0, ?, ?, 1, ?, ?);",
                {
                    fyVal, settNo, vDate,
                    (brokerId > 0 ? QVariant(brokerId) : QVariant()), brokerName,
                    invNo, invNo, partyName,
                    itemWt, itemRate, dRate, dAmt,
                    tdsAmt, netPayable, QString("JV-DAL-%1").arg(vNum > 0 ? vNum : (dalaliCount + 1)), narr
                }
            );
            dalaliCount++;
        }
        std::cout << "[INFO] Migrated " << dalaliCount << " Brokerage settlements!" << std::endl;
    }
    return true;
}

static std::string getFieldStr(const std::map<std::string, std::string>& m, const std::string& key, const std::string& def) {
    auto it = m.find(key);
    if (it != m.end()) return it->second;
    for (const auto& kv : m) {
        if (QString::compare(QString::fromStdString(kv.first), QString::fromStdString(key), Qt::CaseInsensitive) == 0) {
            return kv.second;
        }
    }
    return def;
}

static double parseDoubleVal(const std::string& s, double def = 0.0) {
    std::string clean;
    for (char c : s) {
        if (c != ',' && c != '%' && c != ' ') clean += c;
    }
    if (clean.empty()) return def;
    try {
        return std::stod(clean);
    } catch (...) {
        return def;
    }
}

static int parseInteger(const std::string& s, int def = 0) {
    std::string clean;
    for (char c : s) {
        if (c != ',' && c != ' ') clean += c;
    }
    if (clean.empty()) return def;
    try {
        return std::stoi(clean);
    } catch (...) {
        return def;
    }
}

static std::string toLowerStr(const std::string& s) {
    std::string out = s;
    std::transform(out.begin(), out.end(), out.begin(), [](unsigned char c) { return std::tolower(c); });
    return out;
}

bool MandiMigrator::migrate_mandi_bahikhata(
    MigrationContext& ctx,
    const std::vector<std::map<std::string, std::string>>& transRows,
    const std::vector<std::map<std::string, std::string>>& manufacturingRows,
    const std::vector<std::map<std::string, std::string>>& customClosingRows,
    const std::vector<std::map<std::string, std::string>>& tdsRows,
    const std::vector<std::map<std::string, std::string>>& bardanaRows,
    const std::vector<std::map<std::string, std::string>>& gatePassRows,
    const std::vector<std::map<std::string, std::string>>& gateRegRows,
    const std::vector<std::map<std::string, std::string>>& saudaRows,
    const std::vector<std::map<std::string, std::string>>& saudaTxRows,
    const std::vector<std::map<std::string, std::string>>& brokerageRows,
    const std::map<int, std::string>& ledgerCodeToName,
    const std::map<int, qint64>& ledgerCodeToId,
    const std::map<int, std::string>& itemCodeToName,
    const std::map<int, qint64>& itemCodeToId,
    const std::map<int, std::string>& stockUnitMap,
    std::map<QString, int>& fyNameToId
) {
    Q_UNUSED(gatePassRows);
    Q_UNUSED(saudaTxRows);
    Q_UNUSED(itemCodeToId);
    Q_UNUSED(stockUnitMap);

    auto& db = DatabaseManager::instance();
    ctx.updateProgress(90, QString("Migrating Manufacturing Batches, Mandi Forms & Agri Operations..."));

    // 1. Manufacturing Production Batches & Line Items
    std::map<std::pair<std::string, std::string>, std::vector<std::map<std::string, std::string>>> manufacturingGroups;
    for (const auto& mr : manufacturingRows) {
        std::string mvNo = MigrationUtils::cleanText(QString::fromStdString(getFieldStr(mr, "VoucherNumber"))).toStdString();
        std::string mvDate = MigrationUtils::parseNormalizedDate(QString::fromStdString(getFieldStr(mr, "VoucherDate"))).toStdString();
        manufacturingGroups[{mvNo, mvDate}].push_back(mr);
    }

    int batchCount = 0;
    for (const auto& pair : manufacturingGroups) {
        std::string mvNo = pair.first.first;
        std::string mvDate = pair.first.second;
        const auto& mRows = pair.second;

        QString batchNoStr = QString("Manu-%1").arg(QString::fromStdString(mvNo));
        QString fyVal = MigrationUtils::resolveFinancialYear(QString::fromStdString(mvDate));
        int fyId = MigrationUtils::fyIdForStd(fyNameToId, fyVal);
        if (fyId <= 0) { ++ctx.stats.unmappedFyRows; continue; }

        double paddyIn = 0.0;
        std::string paddyVariety = "Paddy Basmati";
        double headRice = 0.0;
        double brokenRice = 0.0;
        double bran = 0.0;
        double husk = 0.0;
        std::string batchNarration;

        for (const auto& r : mRows) {
            int iCode = parseInteger(getFieldStr(r, "ItemCode"));
            std::string iName = itemCodeToName.count(iCode) ? itemCodeToName.at(iCode) : MigrationUtils::cleanText(QString::fromStdString(getFieldStr(r, "ItemName"))).toStdString();
            if (iName.empty()) iName = "Item #" + std::to_string(iCode);

            std::string iLower = toLowerStr(iName);
            double wt = parseDoubleVal(getFieldStr(r, "Weight"));
            std::string drcr = MigrationUtils::cleanText(QString::fromStdString(getFieldStr(r, "DrCr"))).toStdString();
            std::string narr = MigrationUtils::cleanText(QString::fromStdString(getFieldStr(r, "Narrtn"))).toStdString();
            if (!narr.empty() && batchNarration.empty()) batchNarration = narr;

            if (drcr == "Cr") {
                paddyIn += wt;
                paddyVariety = iName;
            } else {
                if (iLower.find("bran") != std::string::npos && iLower.find("brand") == std::string::npos) {
                    bran += wt;
                } else if (iLower.find("broken") != std::string::npos || iLower.find("nakku") != std::string::npos || iLower.find("tibar") != std::string::npos || iLower.find("dubar") != std::string::npos || iLower.find("mogra") != std::string::npos || iLower.find("kinki") != std::string::npos) {
                    brokenRice += wt;
                } else if (iLower.find("husk") != std::string::npos || iLower.find("phak") != std::string::npos || iLower.find("bhusa") != std::string::npos) {
                    husk += wt;
                } else {
                    headRice += wt;
                }
            }
        }

        double totalOut = headRice + brokenRice + bran + husk;
        double wastage = std::max(0.0, std::round((paddyIn - totalOut) * 1000.0) / 1000.0);
        double yieldPct = (paddyIn > 0.0) ? std::round((headRice / paddyIn * 100.0) * 100.0) / 100.0 : 0.0;

        db.executeNonQuery(
            "INSERT INTO manufacturing_batches ("
            "batch_number, start_date, end_date, paddy_input_qty, output_head_rice_qty, "
            "output_broken_rice_qty, output_bran_qty, output_husk_qty, wastage_qty, "
            "outturn_ratio_percent, status, notes, financial_year_id"
            ") VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, 'COMPLETED', ?, ?);",
            {
                batchNoStr,
                QString::fromStdString(mvDate),
                QString::fromStdString(mvDate),
                paddyIn,
                headRice,
                brokenRice,
                bran,
                husk,
                wastage,
                yieldPct,
                QString::fromStdString(batchNarration),
                fyId
            }
        );
        qint64 batchId = db.lastInsertedId();
        batchCount++;

        for (const auto& r : mRows) {
            int iCode = parseInteger(getFieldStr(r, "ItemCode"));
            std::string iName = itemCodeToName.count(iCode) ? itemCodeToName.at(iCode) : MigrationUtils::cleanText(QString::fromStdString(getFieldStr(r, "ItemName"))).toStdString();
            double bags = parseDoubleVal(getFieldStr(r, "Bags"));
            double wt = parseDoubleVal(getFieldStr(r, "Weight"));
            double rate = parseDoubleVal(getFieldStr(r, "Rate"));
            double amt = parseDoubleVal(getFieldStr(r, "Amount"));
            std::string drcr = MigrationUtils::cleanText(QString::fromStdString(getFieldStr(r, "DrCr"))).toStdString();
            std::string itemType = (drcr == "Cr") ? "CONSUMED" : "PRODUCED";

            db.executeNonQuery(
                "INSERT INTO manufacturing_voucher_items (batch_id, item_name, item_type, bags, weight_qtl, rate, amount) "
                "VALUES (?, ?, ?, ?, ?, ?, ?);",
                {batchId, QString::fromStdString(iName), QString::fromStdString(itemType), bags, wt, rate, amt}
            );
        }
    }
    ctx.stats.totalManufacturingBatches = batchCount;

    // 2. J-Form & I-Form Procurement
    int jCount = 0;
    int iCount = 0;
    for (const auto& r : transRows) {
        std::string rawType = MigrationUtils::cleanText(QString::fromStdString(getFieldStr(r, "TransType"))).toStdString();
        if (rawType == "JFrm") {
            std::string vNo = MigrationUtils::cleanText(QString::fromStdString(getFieldStr(r, "VoucherNumber"))).toStdString();
            std::string vDate = MigrationUtils::parseNormalizedDate(QString::fromStdString(getFieldStr(r, "VoucherDate"))).toStdString();
            QString fyVal = MigrationUtils::resolveFinancialYear(QString::fromStdString(vDate));
            int fyId = MigrationUtils::fyIdForStd(fyNameToId, fyVal);
        if (fyId <= 0) { ++ctx.stats.unmappedFyRows; continue; }

            int acCode = parseInteger(getFieldStr(r, "AccountCode"));
            std::string partyName = ledgerCodeToName.count(acCode) ? ledgerCodeToName.at(acCode) : ("Farmer #" + std::to_string(acCode));
            qint64 partyId = ledgerCodeToId.count(acCode) ? ledgerCodeToId.at(acCode) : 0;
            double amt = parseDoubleVal(getFieldStr(r, "Amount"));

            db.executeNonQuery(
                "INSERT INTO jform_vouchers ("
                "jform_no, jform_date, farmer_id, farmer_name, total_amount, financial_year_id"
                ") VALUES (?, ?, ?, ?, ?, ?);",
                {
                    QString::fromStdString(vNo),
                    QString::fromStdString(vDate),
                    partyId,
                    QString::fromStdString(partyName),
                    amt,
                    fyId
                }
            );
            jCount++;
        } else if (rawType == "IFrm") {
            std::string vNo = MigrationUtils::cleanText(QString::fromStdString(getFieldStr(r, "VoucherNumber"))).toStdString();
            std::string vDate = MigrationUtils::parseNormalizedDate(QString::fromStdString(getFieldStr(r, "VoucherDate"))).toStdString();
            int acCode = parseInteger(getFieldStr(r, "AccountCode"));
            std::string partyName = ledgerCodeToName.count(acCode) ? ledgerCodeToName.at(acCode) : "";
            double amt = parseDoubleVal(getFieldStr(r, "Amount"));

            db.executeNonQuery(
                "INSERT INTO transport_dispatches ("
                "dispatch_no, dispatch_date, consignee_name, total_freight"
                ") VALUES (?, ?, ?, ?);",
                {
                    QString::fromStdString(vNo),
                    QString::fromStdString(vDate),
                    QString::fromStdString(partyName),
                    amt
                }
            );
            iCount++;
        }
    }
    ctx.stats.totalJForms = jCount;
    ctx.stats.totalIForms = iCount;

    // 3. TDS Vouchers
    int tdsCount = 0;
    for (const auto& t : tdsRows) {
        std::string certNo = MigrationUtils::cleanText(QString::fromStdString(getFieldStr(t, "CertNo", getFieldStr(t, "TDSNo")))).toStdString();
        std::string tDate = MigrationUtils::parseNormalizedDate(QString::fromStdString(getFieldStr(t, "Date", getFieldStr(t, "VoucherDate")))).toStdString();
        std::string section = MigrationUtils::cleanText(QString::fromStdString(getFieldStr(t, "Section", "194Q"))).toStdString();
        double gross = parseDoubleVal(getFieldStr(t, "GrossAmount", getFieldStr(t, "Amount")));
        double rate = parseDoubleVal(getFieldStr(t, "TDSRate", getFieldStr(t, "Rate")));
        double tdsAmt = parseDoubleVal(getFieldStr(t, "TDSAmount", getFieldStr(t, "TDS")));
        int acCode = parseInteger(getFieldStr(t, "AccountCode"));
        std::string partyName = ledgerCodeToName.count(acCode) ? ledgerCodeToName.at(acCode) : "";
        qint64 partyId = ledgerCodeToId.count(acCode) ? ledgerCodeToId.at(acCode) : 0;

        db.executeNonQuery(
            "INSERT INTO tds_vouchers ("
            "certificate_no, voucher_date, section_code, party_id, party_name, "
            "gross_amount, tds_rate, tds_amount"
            ") VALUES (?, ?, ?, ?, ?, ?, ?, ?);",
            {
                QString::fromStdString(certNo),
                QString::fromStdString(tDate),
                QString::fromStdString(section),
                partyId,
                QString::fromStdString(partyName),
                gross,
                rate,
                tdsAmt
            }
        );
        tdsCount++;
    }
    ctx.stats.totalTdsVouchers = tdsCount;

    // 4. Custom Closing Stocks
    for (const auto& c : customClosingRows) {
        std::string iName = MigrationUtils::cleanText(QString::fromStdString(getFieldStr(c, "ItemName"))).toStdString();
        double qty = parseDoubleVal(getFieldStr(c, "ClosingQty", getFieldStr(c, "Qty")));
        double rate = parseDoubleVal(getFieldStr(c, "Rate", getFieldStr(c, "ClosingRate")));
        double val = parseDoubleVal(getFieldStr(c, "Value", getFieldStr(c, "ClosingValue")));
        std::string cDate = MigrationUtils::parseNormalizedDate(QString::fromStdString(getFieldStr(c, "Date", getFieldStr(c, "ClosingDate")))).toStdString();

        db.executeNonQuery(
            "INSERT INTO custom_closing_stocks (item_name, closing_date, quantity, rate, valuation) "
            "VALUES (?, ?, ?, ?, ?);",
            {QString::fromStdString(iName), QString::fromStdString(cDate), qty, rate, val}
        );
    }

    // 5. Bardana Transactions
    for (const auto& b : bardanaRows) {
        std::string bDate = MigrationUtils::parseNormalizedDate(QString::fromStdString(getFieldStr(b, "Date", getFieldStr(b, "VoucherDate")))).toStdString();
        std::string bType = MigrationUtils::cleanText(QString::fromStdString(getFieldStr(b, "BagType", getFieldStr(b, "BardanaType")))).toStdString();
        double qtyIn = parseDoubleVal(getFieldStr(b, "QtyIn", getFieldStr(b, "InBags")));
        double qtyOut = parseDoubleVal(getFieldStr(b, "QtyOut", getFieldStr(b, "OutBags")));
        double rate = parseDoubleVal(getFieldStr(b, "Rate"));
        int acCode = parseInteger(getFieldStr(b, "AccountCode"));
        std::string partyName = ledgerCodeToName.count(acCode) ? ledgerCodeToName.at(acCode) : "";

        db.executeNonQuery(
            "INSERT INTO bardana_transactions (transaction_date, bag_type, party_name, bags_in, bags_out, rate) "
            "VALUES (?, ?, ?, ?, ?, ?);",
            {QString::fromStdString(bDate), QString::fromStdString(bType), QString::fromStdString(partyName), qtyIn, qtyOut, rate}
        );
    }

    // 6. Gate Register
    for (const auto& gr : gateRegRows) {
        std::string gDate = MigrationUtils::parseNormalizedDate(QString::fromStdString(getFieldStr(gr, "Date", getFieldStr(gr, "GateDate")))).toStdString();
        std::string vehicleNo = MigrationUtils::cleanText(QString::fromStdString(getFieldStr(gr, "VehicleNo"))).toStdString();
        std::string party = MigrationUtils::cleanText(QString::fromStdString(getFieldStr(gr, "PartyName"))).toStdString();
        std::string item = MigrationUtils::cleanText(QString::fromStdString(getFieldStr(gr, "ItemName"))).toStdString();
        double gross = parseDoubleVal(getFieldStr(gr, "GrossWeight", getFieldStr(gr, "Gross")));
        double tare = parseDoubleVal(getFieldStr(gr, "TareWeight", getFieldStr(gr, "Tare")));
        double net = parseDoubleVal(getFieldStr(gr, "NetWeight", getFieldStr(gr, "Net")));

        db.executeNonQuery(
            "INSERT INTO gate_register (gate_date, vehicle_no, party_name, item_name, gross_weight, tare_weight, net_weight) "
            "VALUES (?, ?, ?, ?, ?, ?, ?);",
            {QString::fromStdString(gDate), QString::fromStdString(vehicleNo), QString::fromStdString(party), QString::fromStdString(item), gross, tare, net}
        );
    }

    // 7. Sauda Forward Contracts
    for (const auto& s : saudaRows) {
        std::string sNo = MigrationUtils::cleanText(QString::fromStdString(getFieldStr(s, "SaudaNo", getFieldStr(s, "ContractNo")))).toStdString();
        std::string sDate = MigrationUtils::parseNormalizedDate(QString::fromStdString(getFieldStr(s, "Date", getFieldStr(s, "SaudaDate")))).toStdString();
        std::string party = MigrationUtils::cleanText(QString::fromStdString(getFieldStr(s, "PartyName"))).toStdString();
        std::string item = MigrationUtils::cleanText(QString::fromStdString(getFieldStr(s, "ItemName"))).toStdString();
        double qty = parseDoubleVal(getFieldStr(s, "Quantity", getFieldStr(s, "Weight")));
        double rate = parseDoubleVal(getFieldStr(s, "Rate", getFieldStr(s, "SaudaRate")));
        std::string broker = MigrationUtils::cleanText(QString::fromStdString(getFieldStr(s, "BrokerName"))).toStdString();

        db.executeNonQuery(
            "INSERT INTO sauda_contracts (contract_no, contract_date, party_name, commodity, quantity_qtl, contract_rate, broker_name) "
            "VALUES (?, ?, ?, ?, ?, ?, ?);",
            {QString::fromStdString(sNo), QString::fromStdString(sDate), QString::fromStdString(party), QString::fromStdString(item), qty, rate, QString::fromStdString(broker)}
        );
    }

    // 8. Brokerage / Dalali Settlements
    for (const auto& br : brokerageRows) {
        std::string bDate = MigrationUtils::parseNormalizedDate(QString::fromStdString(getFieldStr(br, "Date", getFieldStr(br, "VoucherDate")))).toStdString();
        std::string broker = MigrationUtils::cleanText(QString::fromStdString(getFieldStr(br, "BrokerName"))).toStdString();
        double amt = parseDoubleVal(getFieldStr(br, "Amount", getFieldStr(br, "BrokerageAmount")));
        std::string narr = MigrationUtils::cleanText(QString::fromStdString(getFieldStr(br, "Narration"))).toStdString();

        db.executeNonQuery(
            "INSERT INTO dalali_settlements (settlement_date, broker_name, amount, narration) "
            "VALUES (?, ?, ?, ?);",
            {QString::fromStdString(bDate), QString::fromStdString(broker), amt, QString::fromStdString(narr)}
        );
    }

    ctx.stats.totalMandiRecords = jCount + iCount + batchCount + tdsCount;
    return true;
}

bool MandiMigrator::migrate_mandi_busy(
    MigrationContext& ctx,
    const std::vector<std::map<std::string, std::string>>& mandiRows,
    const std::map<int, std::string>& accountCodeToName,
    const std::map<int, qint64>& accountCodeToId,
    std::map<QString, int>& fyNameToId
) {
    auto& db = DatabaseManager::instance();
    ctx.updateProgress(95, QString("Migrating Busy Mandi & Procurement Records..."));

    int mCount = 0;
    for (const auto& r : mandiRows) {
        std::string formNo = MigrationUtils::cleanText(QString::fromStdString(getFieldStr(r, "FormNo", getFieldStr(r, "VchNo")))).toStdString();
        std::string fDate = MigrationUtils::parseNormalizedDate(QString::fromStdString(getFieldStr(r, "Date", getFieldStr(r, "VchDate")))).toStdString();
        QString fyVal = MigrationUtils::resolveFinancialYear(QString::fromStdString(fDate));
        int fyId = MigrationUtils::fyIdForStd(fyNameToId, fyVal);
        if (fyId <= 0) { ++ctx.stats.unmappedFyRows; continue; }

        int acCode = parseInteger(getFieldStr(r, "AccountCode", getFieldStr(r, "FarmerCode")));
        std::string partyName = accountCodeToName.count(acCode) ? accountCodeToName.at(acCode) : ("Farmer #" + std::to_string(acCode));
        qint64 partyId = accountCodeToId.count(acCode) ? accountCodeToId.at(acCode) : 0;
        double amt = parseDoubleVal(getFieldStr(r, "Amount", getFieldStr(r, "TotalAmt")));

        db.executeNonQuery(
            "INSERT INTO jform_vouchers (jform_no, jform_date, farmer_id, farmer_name, total_amount, financial_year_id) "
            "VALUES (?, ?, ?, ?, ?, ?);",
            {
                QString::fromStdString(formNo),
                QString::fromStdString(fDate),
                partyId,
                QString::fromStdString(partyName),
                amt,
                fyId
            }
        );
        mCount++;
    }

    ctx.stats.totalMandiRecords = mCount;
    return true;
}

} // namespace MahadevERP
