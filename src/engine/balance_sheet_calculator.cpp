#include "balance_sheet_calculator.h"
#include "../database_manager.h"
#include "accounting_engine.h"
#include "fiscal_year_helper.h"
#include <cmath>
#include <algorithm>
#include <QDate>
#include <QSet>
#include <QMap>
#include <QHash>
#include <QDebug>

double BalanceSheetCalculator::calculateClosingStockValuation(const QString& asOnDateIso) {
    QString targetDate = asOnDateIso.trimmed();
    if (targetDate.isEmpty()) targetDate = "9999-12-31";

    double totalValuation = 0.0;

    // 1. Fetch all stock items in one query
    QVariantList items = DatabaseManager::instance().executeQuery(
        "SELECT id, code, name, unit, purchase_rate, sale_rate FROM stock_items ORDER BY name COLLATE NOCASE ASC;"
    );
    if (items.isEmpty()) return 0.0;

    // 2. Fetch all audited custom closing stocks on or before targetDate in one batch
    QVariantList customList = DatabaseManager::instance().executeQuery(
        "SELECT item_id, item_name, item_code, closing_date, weight_qtl, rate FROM custom_closing_stocks "
        "WHERE closing_date <= ? ORDER BY closing_date ASC;",
        {targetDate}
    );

    struct CustomClosingInfo {
        QString closingDate;
        double weight = 0.0;
        double rate = 0.0;
    };
    QHash<int, CustomClosingInfo> customByItemId;
    QHash<QString, CustomClosingInfo> customByCode;
    QHash<QString, CustomClosingInfo> customByName;

    for (const auto& cVar : customList) {
        QVariantMap c = cVar.toMap();
        int itemId = c.value("item_id").toInt();
        QString iCode = c.value("item_code").toString().trimmed().toLower();
        QString iName = c.value("item_name").toString().trimmed().toLower();
        CustomClosingInfo info;
        info.closingDate = c.value("closing_date").toString().trimmed();
        info.weight = c.value("weight_qtl").toDouble();
        info.rate = c.value("rate").toDouble();

        if (itemId > 0) customByItemId[itemId] = info;
        if (!iCode.isEmpty()) customByCode[iCode] = info;
        if (!iName.isEmpty()) customByName[iName] = info;
    }

    // 3. Batch query stock transactions up to targetDate
    QVariantList transList = DatabaseManager::instance().executeQuery(
        "SELECT item_id, item_code, item_name, trans_type, voucher_date, SUM(weight_qtl) as tot_qty "
        "FROM stock_transactions WHERE voucher_date <= ? "
        "GROUP BY item_id, item_code, item_name, trans_type, voucher_date;",
        {targetDate}
    );

    struct StockTx {
        int itemId = 0;
        QString code;
        QString name;
        QString transType;
        QString date;
        double qty = 0.0;
    };
    QVector<StockTx> allTx;
    allTx.reserve(transList.size());
    for (const auto& tVar : transList) {
        QVariantMap t = tVar.toMap();
        StockTx tx;
        tx.itemId = t.value("item_id").toInt();
        tx.code = t.value("item_code").toString().trimmed().toLower();
        tx.name = t.value("item_name").toString().trimmed().toLower();
        tx.transType = t.value("trans_type").toString().trimmed();
        tx.date = t.value("voucher_date").toString().trimmed();
        tx.qty = t.value("tot_qty").toDouble();
        allTx.append(tx);
    }

    // 4. Batch query milling voucher items up to targetDate
    QVariantList millList = DatabaseManager::instance().executeQuery(
        "SELECT item_code, drcr, batch_date, SUM(weight_qtl) as tot_qty "
        "FROM milling_voucher_items WHERE batch_date <= ? "
        "GROUP BY item_code, drcr, batch_date;",
        {targetDate}
    );

    struct MillTx {
        QString code;
        QString drcr;
        QString date;
        double qty = 0.0;
    };
    QVector<MillTx> allMill;
    allMill.reserve(millList.size());
    for (const auto& mVar : millList) {
        QVariantMap m = mVar.toMap();
        MillTx mx;
        mx.code = m.value("item_code").toString().trimmed().toLower();
        mx.drcr = m.value("drcr").toString().trimmed();
        mx.date = m.value("batch_date").toString().trimmed();
        mx.qty = m.value("tot_qty").toDouble();
        allMill.append(mx);
    }

    // 5. Compute closing stock per item in memory (O(1) lookups)
    for (const auto& itemVar : items) {
        QVariantMap item = itemVar.toMap();
        int itemId = item.value("id").toInt();
        QString itemCode = item.value("code").toString().trimmed().toLower();
        QString itemName = item.value("name").toString().trimmed().toLower();
        double pRate = item.value("purchase_rate").toDouble();
        double sRate = item.value("sale_rate").toDouble();
        double rate = (pRate > 0.0) ? pRate : ((sRate > 0.0) ? sRate : 0.0);

        bool hasCustom = false;
        CustomClosingInfo cInfo;
        if (itemId > 0 && customByItemId.contains(itemId)) {
            hasCustom = true;
            cInfo = customByItemId[itemId];
        } else if (!itemCode.isEmpty() && customByCode.contains(itemCode)) {
            hasCustom = true;
            cInfo = customByCode[itemCode];
        } else if (!itemName.isEmpty() && customByName.contains(itemName)) {
            hasCustom = true;
            cInfo = customByName[itemName];
        }

        double closingQty = 0.0;
        if (hasCustom) {
            double opQty = cInfo.weight;
            if (cInfo.rate > 0.0) rate = cInfo.rate;
            QString cDate = cInfo.closingDate;

            double inQty = 0.0;
            double outQty = 0.0;
            for (const auto& tx : allTx) {
                if ((tx.itemId == itemId || tx.code == itemCode || tx.name == itemName) && tx.date > cDate && tx.date <= targetDate) {
                    if (tx.transType == "Purc" || tx.transType == "Inward" || tx.transType == "P") inQty += tx.qty;
                    else if (tx.transType == "Sale" || tx.transType == "Outward" || tx.transType == "S") outQty += tx.qty;
                }
            }

            double millIn = 0.0;
            double millOut = 0.0;
            for (const auto& mx : allMill) {
                if ((mx.code == itemCode || mx.code == QString::number(itemId)) && mx.date > cDate && mx.date <= targetDate) {
                    if (mx.drcr == "Dr") millIn += mx.qty;
                    else if (mx.drcr == "Cr") millOut += mx.qty;
                }
            }

            closingQty = opQty + inQty + millIn - outQty - millOut;
        } else {
            double inQty = 0.0;
            double outQty = 0.0;
            for (const auto& tx : allTx) {
                if (tx.itemId == itemId || tx.code == itemCode || tx.name == itemName) {
                    if (tx.transType == "Purc" || tx.transType == "Inward" || tx.transType == "P") inQty += tx.qty;
                    else if (tx.transType == "Sale" || tx.transType == "Outward" || tx.transType == "S") outQty += tx.qty;
                }
            }

            double millIn = 0.0;
            double millOut = 0.0;
            for (const auto& mx : allMill) {
                if (mx.code == itemCode || mx.code == QString::number(itemId)) {
                    if (mx.drcr == "Dr") millIn += mx.qty;
                    else if (mx.drcr == "Cr") millOut += mx.qty;
                }
            }

            closingQty = inQty + millIn - outQty - millOut;
        }

        if (closingQty > 0.001) {
            totalValuation += (closingQty * rate);
        }
    }

    // Fallback to inventory table if valuation is 0
    if (totalValuation < 0.01) {
        QVariant invVal = DatabaseManager::instance().executeScalar(
            "SELECT SUM(current_stock_qtl * sale_rate) FROM inventory WHERE current_stock_qtl > 0;"
        );
        if (invVal.isValid() && invVal.toDouble() > 0.0) {
            totalValuation = invVal.toDouble();
        }
    }

    return totalValuation;
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
        data.firmName = "MAHADEV RICE INDUSTRY";
        data.firmAddress = "Mandi Dabwali Road, Sirsa (Haryana)";
        data.firmGstin = "06AAAAA0000A1Z5";
    }

    // 3. Stock Valuations (Optimized in-memory calculation)
    data.closingStockValue = calculateClosingStockValuation(data.asOnDate);
    data.openingStockValue = calculateOpeningStockValuation(fy.startDate);

    // 4. Sales Revenue up to asOnDate
    QVariant salesRow = DatabaseManager::instance().executeScalar(
        "SELECT SUM(COALESCE(taxable_amount, total_amount)) FROM sales_invoices "
        "WHERE invoice_date >= ? AND invoice_date <= ?;",
        {fy.startDate, data.asOnDate}
    );
    data.totalSalesRevenue = salesRow.isValid() ? salesRow.toDouble() : 0.0;

    // 5. Procurement Cost up to asOnDate
    QVariant purcRow = DatabaseManager::instance().executeScalar(
        "SELECT SUM(COALESCE(total_amount, taxable_amount)) FROM purchase_invoices "
        "WHERE invoice_date >= ? AND invoice_date <= ?;",
        {fy.startDate, data.asOnDate}
    );
    data.totalProcurement = purcRow.isValid() ? purcRow.toDouble() : 0.0;

    // 6. Direct Expenses up to asOnDate
    QVariant dirExpRow = DatabaseManager::instance().executeScalar(
        "SELECT SUM(t.amount) FROM transactions t "
        "JOIN parties p ON t.party_id = p.id OR t.party_name = p.name "
        "WHERE p.group_name LIKE '%Direct Expense%' "
        "AND t.dr_cr = 'Dr' AND t.voucher_date >= ? AND t.voucher_date <= ?;",
        {fy.startDate, data.asOnDate}
    );
    data.totalDirectExpenses = dirExpRow.isValid() ? dirExpRow.toDouble() : 0.0;

    // Gross Profit = (Sales + Closing Stock) - (Opening Stock + Procurement + Direct Expenses)
    double totalTradingCr = data.totalSalesRevenue + data.closingStockValue;
    double totalTradingDr = data.openingStockValue + data.totalProcurement + data.totalDirectExpenses;
    data.grossProfit = totalTradingCr - totalTradingDr;

    // 7. Indirect Incomes & Indirect Expenses
    QVariant indIncRow = DatabaseManager::instance().executeScalar(
        "SELECT SUM(t.amount) FROM transactions t "
        "JOIN parties p ON t.party_id = p.id OR t.party_name = p.name "
        "WHERE p.group_name LIKE '%Indirect Income%' "
        "AND t.dr_cr = 'Cr' AND t.voucher_date >= ? AND t.voucher_date <= ?;",
        {fy.startDate, data.asOnDate}
    );
    data.indirectIncomes = indIncRow.isValid() ? indIncRow.toDouble() : 0.0;

    QVariant indExpRow = DatabaseManager::instance().executeScalar(
        "SELECT SUM(t.amount) FROM transactions t "
        "JOIN parties p ON t.party_id = p.id OR t.party_name = p.name "
        "WHERE p.group_name LIKE '%Indirect Expense%' "
        "AND t.dr_cr = 'Dr' AND t.voucher_date >= ? AND t.voucher_date <= ?;",
        {fy.startDate, data.asOnDate}
    );
    data.indirectExpenses = indExpRow.isValid() ? indExpRow.toDouble() : 0.0;

    data.netProfit = data.grossProfit + data.indirectIncomes - data.indirectExpenses;

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
        "SELECT name, nature, extract_in_balance_sheet FROM account_groups;"
    );
    QHash<QString, QString> groupNature;
    QHash<QString, int> groupExtractBs;
    for (const auto& gVar : grpList) {
        QVariantMap g = gVar.toMap();
        QString gName = g.value("name").toString().trimmed();
        groupNature[gName] = g.value("nature").toString().trimmed();
        groupExtractBs[gName] = g.value("extract_in_balance_sheet").toInt();
    }

    // Fetch all parties once
    QVariantList partyList = DatabaseManager::instance().executeQuery(
        "SELECT id, legacy_id, name, group_name, party_type, opening_balance, balance_type FROM parties ORDER BY group_name, name COLLATE NOCASE ASC;"
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

        if (extractBs == 0 || nature.compare("Income", Qt::CaseInsensitive) == 0 ||
            nature.compare("Expense", Qt::CaseInsensitive) == 0 ||
            gLower.contains("expenditure") || gLower.contains("expense") ||
            gLower.contains("income") || gLower.contains("profit & loss") ||
            gLower.contains("sale") || gLower.contains("purchase") ||
            gLower.contains("trading") || gLower.contains("manufacturing")) {
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
        if (gName == "Sundry Debtors" || gName == "Sundry Creditors" ||
            gName == "Local Mandi Creditors" || gName == "Mandi Debtors") {
            if (netBal < 0) {
                targetGrp = "Sundry Creditors";
            } else {
                targetGrp = "Sundry Debtors";
            }
        }

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

    // 9. Add Trading Items Stock A/c (Stock-in-Hand) to Assets
    BalanceSheetItem stockGroup;
    stockGroup.name = "Trading Items Stock A/c";
    stockGroup.groupName = "Stock-in-Hand";
    stockGroup.isGroup = true;
    stockGroup.level = 0;
    stockGroup.amount = data.closingStockValue;

    // Fetch breakdown of closing stock from custom_closing_stocks or stock_items
    QVariantList customStocks = DatabaseManager::instance().executeQuery(
        "SELECT item_name, weight_qtl, rate, amount FROM custom_closing_stocks "
        "WHERE (closing_date = ? OR financial_year = ? OR financial_year = ?) AND amount > 0 ORDER BY item_name COLLATE NOCASE ASC;",
        {data.asOnDate, data.financialYear, "FY " + data.financialYear}
    );

    double sumCustomStock = 0.0;
    for (const auto& csVar : customStocks) {
        QVariantMap cs = csVar.toMap();
        BalanceSheetItem sItem;
        sItem.name = cs.value("item_name").toString().trimmed();
        sItem.amount = cs.value("amount").toDouble();
        sItem.amountFmt = AccountingEngine::formatIndianCurrency(sItem.amount, true);
        sItem.balanceType = "Dr";
        sItem.level = 1;
        sItem.isCalculated = true;
        stockGroup.children.append(sItem);
        sumCustomStock += sItem.amount;
    }

    if (sumCustomStock > 0.01) {
        stockGroup.amount = sumCustomStock;
        data.closingStockValue = sumCustomStock;
    }
    stockGroup.amountFmt = AccountingEngine::formatIndianCurrency(stockGroup.amount, true);
    assetGroupMap["Trading Items Stock A/c"] = stockGroup;

    // 10. Reconcile Net Profit & Balance Sheet Totals
    double sumLiabBeforeProfit = 0.0;
    for (const auto& grp : liabGroupMap) sumLiabBeforeProfit += grp.amount;

    double sumAssets = 0.0;
    for (const auto& grp : assetGroupMap) sumAssets += grp.amount;

    double netBalancingProfit = sumAssets - sumLiabBeforeProfit;
    data.netProfit = netBalancingProfit;

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

    // 14. Reconcile Totals
    double totLiab = 0.0;
    for (const auto& g : data.liabilitiesGroups) totLiab += g.amount;
    data.totalLiabilities = totLiab;
    data.totalLiabilitiesFmt = AccountingEngine::formatIndianCurrency(data.totalLiabilities, true);

    double totAssets = 0.0;
    for (const auto& g : data.assetsGroups) totAssets += g.amount;
    data.totalAssets = totAssets;
    data.totalAssetsFmt = AccountingEngine::formatIndianCurrency(data.totalAssets, true);

    data.difference = std::abs(data.totalLiabilities - data.totalAssets);
    data.isBalanced = (data.difference < 0.01);

    return data;
}
