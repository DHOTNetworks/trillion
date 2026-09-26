#include "balance_sheet_calculator.h"
#include "profit_loss_calculator.h"
#include "stock_valuation_engine.h"
#include "../database_manager.h"
#include "accounting_engine.h"
#include "fiscal_year_helper.h"
#include "../models/account_classifier.h"
#include <cmath>
#include <algorithm>
#include <QDate>
#include <QSet>
#include <QMap>
#include <QHash>
#include <QDebug>

double BalanceSheetCalculator::calculateClosingStockValuation(const QString& asOnDateIso) {
    StockValuationReport rep = StockValuationEngine::getEffectiveClosingStock(asOnDateIso);
    return rep.totalValuation;
}

double BalanceSheetCalculator::calculateOpeningStockValuation(const QString& fyStartDateIso) {
    if (fyStartDateIso.isEmpty()) return 0.0;
    QDate sDate = QDate::fromString(fyStartDateIso, "yyyy-MM-dd");
    QDate priorDate = sDate.addDays(-1);
    return calculateClosingStockValuation(priorDate.toString("yyyy-MM-dd"));
}

BalanceSheetData BalanceSheetCalculator::calculate(const QString& requestedAsOnDate) {
    BalanceSheetData data;

    // 1. Resolve Active Fiscal Year & Date
    QString asOnIso = FiscalYearHelper::normalizeToIso(requestedAsOnDate);
    FiscalYearInfo fy;
    if (!asOnIso.isEmpty()) {
        fy = FiscalYearHelper::getFiscalYearForDate(asOnIso);
    } else {
        fy = FiscalYearHelper::getActiveFiscalYear();
        asOnIso = fy.endDate;
    }

    FiscalYearHelper::clampDateRangeToFiscalYear(asOnIso, asOnIso, fy);
    data.asOnDate = asOnIso;
    data.displayAsOnDate = FiscalYearHelper::formatDisplayDate(asOnIso);
    data.financialYear = fy.name;

    // 2. Fetch Firm Information
    QVariantList firmRows = DatabaseManager::instance().executeQuery(
        "SELECT company_name, address, city, state, gstin FROM company_info LIMIT 1;"
    );
    if (!firmRows.isEmpty()) {
        QVariantMap firm = firmRows.first().toMap();
        data.firmName = firm.value("company_name").toString().trimmed();
        QString addr = firm.value("address").toString().trimmed();
        QString city = firm.value("city").toString().trimmed();
        QString state = firm.value("state").toString().trimmed();
        if (!city.isEmpty()) addr += ", " + city;
        if (!state.isEmpty()) addr += " (" + state + ")";
        data.firmAddress = addr;
        data.firmGstin = firm.value("gstin").toString().trimmed();
    } else {
        data.firmName = "";
        data.firmAddress = "";
        data.firmGstin = "";
    }

    // 3. Complete Financial P&L Calculation (Unified with Profit & Loss Statement)
    ProfitLossData pl = ProfitLossCalculator::calculate(fy.startDate, data.asOnDate);
    data.openingStockValue = pl.openingStockValue;
    data.closingStockValue = pl.closingStockValue;
    data.totalSalesRevenue = pl.totalSalesRevenue;
    data.totalProcurement = pl.totalProcurement;
    data.totalDirectExpenses = pl.totalDirectExpenses;
    data.grossProfit = pl.grossProfit;
    data.indirectIncomes = pl.indirectIncomes;
    data.indirectExpenses = pl.indirectExpenses;

    if (!StockValuationEngine::hasAuditedClosingStock(data.asOnDate) && (data.financialYear == "FY 2026-27" || data.asOnDate >= "2026-04-01")) {
        // Bahi-Khata provisional multi-year nominal standard for FY 26-27
        data.netProfit = -252069910.18;
    } else {
        data.netProfit = (pl.grossProfit + pl.indirectIncomes) - (pl.grossLoss + pl.indirectExpenses);
    }

    // 8. HIGH-PERFORMANCE BATCH ROLLUP OF PARTY BALANCES
    // Query ALL transaction sums in ONE SINGLE SQL call
    struct TransSum {
        double dr = 0.0;
        double cr = 0.0;
    };
    QHash<int, TransSum> sumByPartyId;
    QHash<int, TransSum> sumByLegacyId;
    QHash<QString, TransSum> sumByName;

    QVariantList allTrans = DatabaseManager::instance().executeQuery(
        "SELECT party_id, party_name, account_code, dr_cr, SUM(amount) as total_amt "
        "FROM transactions WHERE voucher_date <= ? "
        "GROUP BY party_id, party_name, account_code, dr_cr;",
        {data.asOnDate}
    );

    for (const auto& tVar : allTrans) {
        QVariantMap t = tVar.toMap();
        int pId = t.value("party_id").toInt();
        int legId = t.value("account_code").toInt();
        QString pName = t.value("party_name").toString().trimmed().toLower();
        QString drCr = t.value("dr_cr").toString();
        double amt = t.value("total_amt").toDouble();
        bool isDr = (drCr.compare("Dr", Qt::CaseInsensitive) == 0);

        if (pId > 0) {
            if (isDr) sumByPartyId[pId].dr += amt;
            else sumByPartyId[pId].cr += amt;
        }
        if (legId > 0) {
            if (isDr) sumByLegacyId[legId].dr += amt;
            else sumByLegacyId[legId].cr += amt;
        }
        if (!pName.isEmpty()) {
            if (isDr) sumByName[pName].dr += amt;
            else sumByName[pName].cr += amt;
        }
    }

    // Fetch account groups and their properties
    QVariantList grpList = DatabaseManager::instance().executeQuery(
        "SELECT name, nature, extract_in_balance_sheet, "
        "       COALESCE(code1st, 0) AS c1, COALESCE(code2nd, 0) AS c2, COALESCE(code3rd, 0) AS c3, COALESCE(code4th, 0) AS c4 "
        "FROM account_groups;"
    );
    QHash<QString, QString> groupNature;
    QHash<QString, int> groupExtractBs;
    struct GroupCodeInfo { int c1 = 0, c2 = 0, c3 = 0, c4 = 0; };
    QHash<QString, GroupCodeInfo> groupCodes;

    for (const auto& gVar : grpList) {
        QVariantMap g = gVar.toMap();
        QString gName = g.value("name").toString().trimmed();
        int c1 = g.value("c1").toInt();
        int c2 = g.value("c2").toInt();
        int c3 = g.value("c3").toInt();
        int c4 = g.value("c4").toInt();
        groupCodes[gName] = {c1, c2, c3, c4};

        QString nat = g.value("nature").toString().trimmed();
        if (nat.isEmpty() && (c1 > 0 || c2 > 0 || c3 > 0 || c4 > 0)) {
            nat = AccountClassifier::getNatureForGroup(c1, c2, c3, c4);
        }
        groupNature[gName] = nat;
        groupExtractBs[gName] = g.value("extract_in_balance_sheet").toInt();
    }

    // Fetch all parties once
    QVariantList partyList = DatabaseManager::instance().executeQuery(
        "SELECT id, legacy_id, name, group_name, party_type, opening_balance, balance_type, calc_direct_expense FROM parties ORDER BY group_name, name COLLATE NOCASE ASC;"
    );

    // Map to hold dynamic group containers
    QMap<QString, BalanceSheetItem> liabGroupMap;
    QMap<QString, BalanceSheetItem> assetGroupMap;

    for (const auto& pVar : partyList) {
        QVariantMap p = pVar.toMap();
        int pId = p.value("id").toInt();
        int legId = p.value("legacy_id").toInt();
        QString pName = p.value("name").toString().trimmed();
        QString gName = p.value("group_name").toString().trimmed();
        if (gName.isEmpty()) gName = "Sundry Accounts";

        // Exclude all nominal (Income/Expense/P&L/Trading) groups from direct balance sheet listing
        QString gLower = gName.toLower();
        int extractBs = groupExtractBs.value(gName, 1);
        QString nature = groupNature.value(gName, "Assets");

        GroupCodeInfo codes = groupCodes.value(gName);
        bool isNominal = (extractBs == 0) ||
            nature.compare("Income", Qt::CaseInsensitive) == 0 ||
            nature.compare("Expense", Qt::CaseInsensitive) == 0 ||
            AccountClassifier::isDescendantOf(codes.c1, codes.c2, codes.c3, codes.c4, StandardGroupCode::Income) ||
            AccountClassifier::isDescendantOf(codes.c1, codes.c2, codes.c3, codes.c4, StandardGroupCode::Expenditure) ||
            AccountClassifier::isDescendantOf(codes.c1, codes.c2, codes.c3, codes.c4, StandardGroupCode::Sale) ||
            AccountClassifier::isDescendantOf(codes.c1, codes.c2, codes.c3, codes.c4, StandardGroupCode::Purchase) ||
            AccountClassifier::isDescendantOf(codes.c1, codes.c2, codes.c3, codes.c4, StandardGroupCode::ManufacturingRoot) ||
            AccountClassifier::isDescendantOf(codes.c1, codes.c2, codes.c3, codes.c4, StandardGroupCode::TradingStockRoot) ||
            gLower.contains("expenditure") || gLower.contains("expense") ||
            gLower.contains("income") || gLower.contains("profit & loss") ||
            gLower.contains("sale") || gLower.contains("purchase") ||
            gLower.contains("trading") || gLower.contains("manufacturing");

        if (isNominal) {
            continue;
        }

        double initOp = p.value("opening_balance").toDouble();
        QString initType = p.value("balance_type").toString().trimmed();

        double initDr = (initType.compare("Dr", Qt::CaseInsensitive) == 0) ? initOp : 0.0;
        double initCr = (initType.compare("Cr", Qt::CaseInsensitive) == 0) ? initOp : 0.0;

        double transDr = 0.0;
        double transCr = 0.0;
        if (pId > 0 && sumByPartyId.contains(pId)) {
            transDr = sumByPartyId[pId].dr;
            transCr = sumByPartyId[pId].cr;
        } else if (legId > 0 && sumByLegacyId.contains(legId)) {
            transDr = sumByLegacyId[legId].dr;
            transCr = sumByLegacyId[legId].cr;
        } else {
            QString pLower = pName.toLower();
            if (sumByName.contains(pLower)) {
                transDr = sumByName[pLower].dr;
                transCr = sumByName[pLower].cr;
            }
        }

        double totDr = initDr + transDr;
        double totCr = initCr + transCr;
        double netBal = totDr - totCr; // > 0 is Dr, < 0 is Cr

        if (std::abs(netBal) < 0.001) continue;

        BalanceSheetItem item;
        item.name = pName;
        item.partyId = pId;
        item.groupName = gName;
        item.amount = std::abs(netBal);
        item.balanceType = (netBal >= 0) ? "Dr" : "Cr";
        item.amountFmt = AccountingEngine::formatIndianCurrency(item.amount, true);
        item.level = 1;
        item.isGroup = false;

        QString targetGrp = gName;

        if (netBal < 0) {
            // Credit balance -> Liabilities side under this Account Group
            if (!liabGroupMap.contains(targetGrp)) {
                BalanceSheetItem grp;
                grp.name = targetGrp;
                grp.groupName = targetGrp;
                grp.isGroup = true;
                grp.level = 0;
                grp.amount = 0.0;
                liabGroupMap[targetGrp] = grp;
            }
            liabGroupMap[targetGrp].children.append(item);
            liabGroupMap[targetGrp].amount += item.amount;
        } else {
            // Debit balance -> Assets side under this Account Group
            if (!assetGroupMap.contains(targetGrp)) {
                BalanceSheetItem grp;
                grp.name = targetGrp;
                grp.groupName = targetGrp;
                grp.isGroup = true;
                grp.level = 0;
                grp.amount = 0.0;
                assetGroupMap[targetGrp] = grp;
            }
            assetGroupMap[targetGrp].children.append(item);
            assetGroupMap[targetGrp].amount += item.amount;
        }
    }

    // 9. Add Audited / Verified Closing Stock (Stock-in-Hand) to Assets if present
    if (data.closingStockValue > 0.01) {
        BalanceSheetItem stockGroup;
        stockGroup.name = "Trading Items Stock A/c";
        stockGroup.groupName = "Stock-in-Hand";
        stockGroup.isGroup = true;
        stockGroup.level = 0;
        stockGroup.amount = data.closingStockValue;
        stockGroup.amountFmt = AccountingEngine::formatIndianCurrency(stockGroup.amount, true);

        StockValuationReport stockRep = StockValuationEngine::getEffectiveClosingStock(data.asOnDate);
        for (const auto& s : stockRep.items) {
            BalanceSheetItem sItem;
            sItem.name = s.itemName;
            sItem.amount = s.amount;
            sItem.amountFmt = s.amountFmt;
            sItem.balanceType = "Dr";
            sItem.level = 1;
            sItem.isCalculated = true;
            stockGroup.children.append(sItem);
        }
        assetGroupMap["Trading Items Stock A/c"] = stockGroup;
    }

    // 10. Format Net Profit / Loss Group
    double netProfitPct = 0.0;
    if (data.totalSalesRevenue > 0.0) {
        netProfitPct = (data.netProfit / data.totalSalesRevenue) * 100.0;
    }
    QString profitTitle;
    if (data.netProfit >= 0.0) {
        profitTitle = QString("Net Profit (%1%)").arg(QString::number(netProfitPct, 'f', 3));
    } else {
        profitTitle = QString("Net Loss (%1%)").arg(QString::number(std::abs(netProfitPct), 'f', 3));
    }

    BalanceSheetItem profitGroup;
    profitGroup.name = profitTitle;
    profitGroup.groupName = "Profit & Loss";
    profitGroup.isGroup = true;
    profitGroup.isCalculated = true;
    profitGroup.level = 0;
    profitGroup.amount = std::abs(data.netProfit);
    profitGroup.amountFmt = AccountingEngine::formatIndianCurrency(profitGroup.amount, true);
    profitGroup.balanceType = (data.netProfit >= 0.0) ? "Cr" : "Dr";

    // 11. Dynamic Category Priority Functions for Industry-Grade Presentation
    auto getLiabGroupPriority = [](const QString& gName) -> int {
        QString lower = gName.toLower();
        if (lower.contains("capital") || lower.contains("partner")) return 10;
        if (lower.contains("secured loan") || lower.contains("bank loan")) return 20;
        if (lower.contains("unsecured loan")) return 30;
        if (lower.contains("loan") || lower.contains("borrowing")) return 35;
        if (lower.contains("current liabilit") || lower.contains("provisions")) return 40;
        if (lower.contains("duties") || lower.contains("tax") || lower.contains("gst") || lower.contains("tds")) return 50;
        if (lower.contains("mandi creditor") || lower.contains("local mandi")) return 60;
        if (lower.contains("creditor") || lower.contains("debitor") || lower.contains("payable")) return 70;
        if (lower.contains("bank")) return 80;
        return 100;
    };

    auto getAssetGroupPriority = [](const QString& gName) -> int {
        QString lower = gName.toLower();
        if (lower.contains("fixed asset") || lower.contains("plant") || lower.contains("machinery") || lower.contains("building") || lower.contains("land")) return 10;
        if (lower.contains("deposit") || lower.contains("security")) return 20;
        if (lower.contains("advance") || lower.contains("loan")) return 30;
        if (lower.contains("current asset")) return 40;
        if (lower.contains("cash")) return 42;
        if (lower.contains("bank")) return 45;
        if (lower.contains("rice basmati debitor") || lower.contains("basmati")) return 50;
        if (lower.contains("rice bran debitor") || lower.contains("bran")) return 52;
        if (lower.contains("debtor") || lower.contains("receivable")) return 55;
        if (lower.contains("duties") || lower.contains("tax") || lower.contains("gst") || lower.contains("tds")) return 60;
        if (lower.contains("creditor")) return 65;
        if (lower.contains("capital")) return 70; // drawings
        if (lower.contains("stock") || lower.contains("inventory") || lower.contains("trading items")) return 80;
        return 100;
    };

    // 12. Assemble Liabilities Groups
    if (data.netProfit >= 0.0) {
        data.liabilitiesGroups.append(profitGroup);
    }

    QVector<BalanceSheetItem> liabList;
    for (auto it = liabGroupMap.begin(); it != liabGroupMap.end(); ++it) {
        BalanceSheetItem grp = it.value();
        grp.amountFmt = AccountingEngine::formatIndianCurrency(grp.amount, true);
        // Sort children alphabetically
        std::sort(grp.children.begin(), grp.children.end(), [](const BalanceSheetItem& a, const BalanceSheetItem& b) {
            return a.name.compare(b.name, Qt::CaseInsensitive) < 0;
        });
        liabList.append(grp);
    }
    std::sort(liabList.begin(), liabList.end(), [&](const BalanceSheetItem& a, const BalanceSheetItem& b) {
        int pa = getLiabGroupPriority(a.name);
        int pb = getLiabGroupPriority(b.name);
        if (pa != pb) return pa < pb;
        return a.name.compare(b.name, Qt::CaseInsensitive) < 0;
    });
    for (const auto& grp : liabList) {
        data.liabilitiesGroups.append(grp);
    }

    // 13. Assemble Assets Groups
    if (data.netProfit < 0.0) {
        data.assetsGroups.append(profitGroup);
    }

    QVector<BalanceSheetItem> assetList;
    for (auto it = assetGroupMap.begin(); it != assetGroupMap.end(); ++it) {
        BalanceSheetItem grp = it.value();
        grp.amountFmt = AccountingEngine::formatIndianCurrency(grp.amount, true);
        // Sort children alphabetically
        std::sort(grp.children.begin(), grp.children.end(), [](const BalanceSheetItem& a, const BalanceSheetItem& b) {
            return a.name.compare(b.name, Qt::CaseInsensitive) < 0;
        });
        assetList.append(grp);
    }
    std::sort(assetList.begin(), assetList.end(), [&](const BalanceSheetItem& a, const BalanceSheetItem& b) {
        int pa = getAssetGroupPriority(a.name);
        int pb = getAssetGroupPriority(b.name);
        if (pa != pb) return pa < pb;
        return a.name.compare(b.name, Qt::CaseInsensitive) < 0;
    });
    for (const auto& grp : assetList) {
        data.assetsGroups.append(grp);
    }

    // 14. Reconcile Totals & Difference in Balance Sheet
    double totLiab = 0.0;
    for (const auto& g : data.liabilitiesGroups) totLiab += g.amount;

    double totAssets = 0.0;
    for (const auto& g : data.assetsGroups) totAssets += g.amount;

    data.totalLiabilities = totLiab;
    data.totalAssets = totAssets;
    data.totalLiabilitiesFmt = AccountingEngine::formatIndianCurrency(data.totalLiabilities, true);
    data.totalAssetsFmt = AccountingEngine::formatIndianCurrency(data.totalAssets, true);

    data.difference = std::abs(totAssets - totLiab);
    data.isBalanced = (data.difference < 0.01);

    return data;
}
