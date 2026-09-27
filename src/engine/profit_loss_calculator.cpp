#include "profit_loss_calculator.h"
#include "balance_sheet_calculator.h"
#include "stock_valuation_engine.h"
#include "fiscal_year_helper.h"
#include "accounting_engine.h"
#include "../database_manager.h"
#include "../models/account_classifier.h"
#include <QDebug>
#include <cmath>
#include <algorithm>

double ProfitLossCalculator::calculateClosingStockValuation(const QString& asOnDateIso) {
    StockValuationReport rep = StockValuationEngine::getEffectiveClosingStock(asOnDateIso);
    return rep.totalValuation;
}

double ProfitLossCalculator::calculateOpeningStockValuation(const QString& fyStartDateIso) {
    if (fyStartDateIso.isEmpty()) return 0.0;
    QDate sDate = QDate::fromString(fyStartDateIso, "yyyy-MM-dd");
    QDate priorDate = sDate.addDays(-1);
    return calculateClosingStockValuation(priorDate.toString("yyyy-MM-dd"));
}

ProfitLossData ProfitLossCalculator::calculate(const QString& requestedFromDate, const QString& requestedToDate) {
    ProfitLossData data;

    // 1. Resolve Fiscal Year & Dates
    QString fIso = FiscalYearHelper::normalizeToIso(requestedFromDate);
    QString tIso = FiscalYearHelper::normalizeToIso(requestedToDate);

    FiscalYearInfo fy;
    if (!fIso.isEmpty()) {
        fy = FiscalYearHelper::getFiscalYearForDate(fIso);
    } else {
        fy = FiscalYearHelper::getActiveFiscalYear();
        fIso = fy.startDate;
    }

    if (tIso.isEmpty()) {
        tIso = fy.endDate;
    }

    FiscalYearHelper::clampDateRangeToFiscalYear(fIso, tIso, fy);
    data.fromDate = fIso;
    data.toDate = tIso;
    data.financialYear = fy.name;

    data.displayFromDate = FiscalYearHelper::formatDisplayDate(data.fromDate);
    data.displayToDate = FiscalYearHelper::formatDisplayDate(data.toDate);

    // 2. Firm Profile Info
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

    // 3. Stock Valuations
    data.openingStockValue = calculateOpeningStockValuation(data.fromDate);
    data.closingStockValue = calculateClosingStockValuation(data.toDate);

    // 4. HIGH-PERFORMANCE BATCH ROLLUP OF NOMINAL TRANSACTIONS
    struct TransSum {
        double dr = 0.0;
        double cr = 0.0;
    };
    QHash<int, TransSum> sumByPartyId;
    QHash<int, TransSum> sumByLegacyId;
    QHash<QString, TransSum> sumByName;

    QVariantList allTrans = DatabaseManager::instance().executeQuery(
        "SELECT party_id, party_name, account_code, dr_cr, SUM(amount) as total_amt "
        "FROM transactions WHERE voucher_date >= ? AND voucher_date <= ? "
        "GROUP BY party_id, party_name, account_code, dr_cr;",
        {data.fromDate, data.toDate}
    );

    for (const auto& tVar : allTrans) {
        QVariantMap t = tVar.toMap();
        int pId = t.value("party_id").toInt();
        int legId = t.value("account_code").toInt();
        QString pName = t.value("party_name").toString().trimmed().toLower();
        QString side = t.value("dr_cr").toString();
        double amt = t.value("total_amt").toDouble();
        bool isDr = (side.compare("Dr", Qt::CaseInsensitive) == 0);

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
    struct GroupCodeInfo { int c1 = 0, c2 = 0, c3 = 0, c4 = 0; QString nature; int extractBs = 1; };
    QHash<QString, GroupCodeInfo> groupMeta;

    for (const auto& gVar : grpList) {
        QVariantMap g = gVar.toMap();
        QString gName = g.value("name").toString().trimmed();
        int c1 = g.value("c1").toInt();
        int c2 = g.value("c2").toInt();
        int c3 = g.value("c3").toInt();
        int c4 = g.value("c4").toInt();
        QString nat = g.value("nature").toString().trimmed();
        if (nat.isEmpty() && (c1 > 0 || c2 > 0 || c3 > 0 || c4 > 0)) {
            nat = AccountClassifier::getNatureForGroup(c1, c2, c3, c4);
        }
        int ext = g.value("extract_in_balance_sheet").toInt();
        groupMeta[gName] = {c1, c2, c3, c4, nat, ext};
    }

    // Query nominal ledger totals and build group items in one single pass
    double calcSales = 0.0;
    double calcPurc = 0.0;
    double calcDirExp = 0.0;
    double calcIndInc = 0.0;
    double calcIndExp = 0.0;

    QMap<QString, QVector<ProfitLossItem>> directExpByGroup;
    QMap<QString, QVector<ProfitLossItem>> indirectExpByGroup;
    QMap<QString, QVector<ProfitLossItem>> indirectIncByGroup;
    QMap<QString, QVector<ProfitLossItem>> salesByGroup;
    QMap<QString, QVector<ProfitLossItem>> purcByGroup;

    QVariantList parties = DatabaseManager::instance().executeQuery(
        "SELECT id, legacy_id, name, group_name, calc_direct_expense FROM parties ORDER BY group_name, name;"
    );

    for (const auto& pVar : parties) {
        QVariantMap p = pVar.toMap();
        int pId = p.value("id").toInt();
        int legId = p.value("legacy_id").toInt();
        QString name = p.value("name").toString().trimmed();
        QString grp = p.value("group_name").toString().trimmed();
        int calcDirect = p.value("calc_direct_expense").toInt();

        double dr = 0.0, cr = 0.0;
        if (pId > 0 && sumByPartyId.contains(pId)) {
            dr = sumByPartyId[pId].dr;
            cr = sumByPartyId[pId].cr;
        } else if (legId > 0 && sumByLegacyId.contains(legId)) {
            dr = sumByLegacyId[legId].dr;
            cr = sumByLegacyId[legId].cr;
        } else if (sumByName.contains(name.toLower())) {
            dr = sumByName[name.toLower()].dr;
            cr = sumByName[name.toLower()].cr;
        }

        if (dr <= 0.001 && cr <= 0.001) continue;

        GroupCodeInfo info = groupMeta.value(grp);
        bool isTrading = AccountClassifier::isTrading(info.c1, info.c2, info.c3, info.c4);
        bool isPl = AccountClassifier::isProfitAndLoss(info.c1, info.c2, info.c3, info.c4);

        if (!isTrading && !isPl) continue;

        ProfitLossItem itm;
        itm.partyId = pId;
        itm.name = name;
        itm.groupName = grp;
        itm.level = 2;

        if (isTrading) {
            if (cr > 0.0) {
                calcSales += cr;
                itm.amount = cr;
                itm.amountFmt = AccountingEngine::formatIndianCurrency(cr, true);
                itm.side = "Cr";
                salesByGroup[grp].append(itm);
            }
            if (dr > 0.0) {
                if (AccountClassifier::isDirectExpense(info.c1, info.c2, info.c3, info.c4, calcDirect)) {
                    calcDirExp += dr;
                    itm.amount = dr;
                    itm.amountFmt = AccountingEngine::formatIndianCurrency(dr, true);
                    itm.side = "Dr";
                    directExpByGroup[grp].append(itm);
                } else {
                    calcPurc += dr;
                    itm.amount = dr;
                    itm.amountFmt = AccountingEngine::formatIndianCurrency(dr, true);
                    itm.side = "Dr";
                    purcByGroup[grp].append(itm);
                }
            }
        } else if (isPl) {
            if (cr > 0.0) {
                calcIndInc += cr;
                itm.amount = cr;
                itm.amountFmt = AccountingEngine::formatIndianCurrency(cr, true);
                itm.side = "Cr";
                indirectIncByGroup[grp].append(itm);
            }
            if (dr > 0.0) {
                calcIndExp += dr;
                itm.amount = dr;
                itm.amountFmt = AccountingEngine::formatIndianCurrency(dr, true);
                itm.side = "Dr";
                indirectExpByGroup[grp].append(itm);
            }
        }
    }

    data.totalSalesRevenue = calcSales;
    data.totalProcurement = calcPurc;
    data.totalDirectExpenses = calcDirExp;
    data.indirectIncomes = calcIndInc;
    data.indirectExpenses = calcIndExp;

    // 7. Trading Account Gross Profit / Loss
    double totalTradingCrRaw = data.totalSalesRevenue + data.closingStockValue;
    double totalTradingDrRaw = data.openingStockValue + data.totalProcurement + data.totalDirectExpenses;
    double grossDiff = totalTradingCrRaw - totalTradingDrRaw;

    if (grossDiff >= 0.0) {
        data.grossProfit = grossDiff;
        data.grossLoss = 0.0;
    } else {
        data.grossProfit = 0.0;
        data.grossLoss = std::abs(grossDiff);
    }

    // 9. Profit & Loss Account Net Profit / Loss
    double totalPlCrRaw = data.grossProfit + data.indirectIncomes;
    double totalPlDrRaw = data.grossLoss + data.indirectExpenses;
    double netDiff = totalPlCrRaw - totalPlDrRaw;

    if (netDiff >= 0.0) {
        data.netProfit = netDiff;
        data.netLoss = 0.0;
    } else {
        data.netProfit = 0.0;
        data.netLoss = std::abs(netDiff);
    }

    // -------------------------------------------------------------
    // BUILD TRADING DR GROUPS (Left Side, Top)
    // -------------------------------------------------------------
    // 1. Opening Stock
    ProfitLossItem openingStockGrp;
    openingStockGrp.name = "Opening Stock";
    openingStockGrp.groupName = "Trading Dr";

    QDate sDate = QDate::fromString(data.fromDate, "yyyy-MM-dd");
    QString priorDateIso = sDate.addDays(-1).toString("yyyy-MM-dd");
    StockValuationReport opRep = StockValuationEngine::getEffectiveClosingStock(priorDateIso);

    data.openingStockValue = opRep.totalValuation;
    openingStockGrp.amount = opRep.totalValuation;
    openingStockGrp.amountFmt = opRep.totalValuationFmt;
    openingStockGrp.side = "Dr";
    openingStockGrp.isGroup = true;
    openingStockGrp.level = 1;

    for (const auto& s : opRep.items) {
        ProfitLossItem sItm;
        sItm.name = s.itemName + QString(" (%1 Qtl)").arg(s.weightQtl, 0, 'f', 1);
        sItm.groupName = "Opening Stock";
        sItm.amount = s.amount;
        sItm.amountFmt = s.amountFmt;
        sItm.side = "Dr";
        sItm.level = 2;
        openingStockGrp.children.append(sItm);
    }
    data.tradingDrGroups.append(openingStockGrp);

    // 2. Purchase Accounts / Paddy Procurement
    ProfitLossItem purcGrp;
    purcGrp.name = "Purchases / Procurement Accounts";
    purcGrp.groupName = "Trading Dr";
    double purcTot = 0.0;
    for (auto it = purcByGroup.begin(); it != purcByGroup.end(); ++it) {
        for (const auto& item : it.value()) {
            purcTot += item.amount;
            purcGrp.children.append(item);
        }
    }
    if (purcTot <= 0.0 && data.totalProcurement > 0.0) purcTot = data.totalProcurement;
    purcGrp.amount = purcTot;
    purcGrp.amountFmt = AccountingEngine::formatIndianCurrency(purcTot, true);
    purcGrp.side = "Dr";
    purcGrp.isGroup = true;
    purcGrp.level = 1;
    data.tradingDrGroups.append(purcGrp);

    // 3. Direct Expenses
    for (auto it = directExpByGroup.begin(); it != directExpByGroup.end(); ++it) {
        ProfitLossItem dGrp;
        dGrp.name = it.key();
        dGrp.groupName = "Direct Expenses";
        double gTot = 0.0;
        for (const auto& item : it.value()) {
            gTot += item.amount;
            dGrp.children.append(item);
        }
        dGrp.amount = gTot;
        dGrp.amountFmt = AccountingEngine::formatIndianCurrency(gTot, true);
        dGrp.side = "Dr";
        dGrp.isGroup = true;
        dGrp.level = 1;
        data.tradingDrGroups.append(dGrp);
    }
    if (directExpByGroup.isEmpty() && data.totalDirectExpenses > 0.0) {
        ProfitLossItem dGrp;
        dGrp.name = "Direct Manufacturing & Milling Expenses";
        dGrp.groupName = "Direct Expenses";
        dGrp.amount = data.totalDirectExpenses;
        dGrp.amountFmt = AccountingEngine::formatIndianCurrency(data.totalDirectExpenses, true);
        dGrp.side = "Dr";
        dGrp.isGroup = true;
        dGrp.level = 1;
        data.tradingDrGroups.append(dGrp);
    }

    // 4. Gross Profit c/d (if Gross Profit > 0)
    if (data.grossProfit > 0.0) {
        ProfitLossItem gpItem;
        gpItem.name = "Gross Profit c/d (Transferred to P&L A/c)";
        gpItem.groupName = "Trading Dr";
        gpItem.amount = data.grossProfit;
        gpItem.amountFmt = AccountingEngine::formatIndianCurrency(data.grossProfit, true);
        gpItem.side = "Dr";
        gpItem.isCalculated = true;
        gpItem.isGroup = false;
        gpItem.level = 1;
        data.tradingDrGroups.append(gpItem);
    }

    // -------------------------------------------------------------
    // BUILD TRADING CR GROUPS (Right Side, Top)
    // -------------------------------------------------------------
    // 1. Sales Revenue
    ProfitLossItem salesGrp;
    salesGrp.name = "Sales Revenue Accounts";
    salesGrp.groupName = "Trading Cr";
    double salesTot = 0.0;
    for (auto it = salesByGroup.begin(); it != salesByGroup.end(); ++it) {
        for (const auto& item : it.value()) {
            salesTot += item.amount;
            salesGrp.children.append(item);
        }
    }
    if (salesTot <= 0.0 && data.totalSalesRevenue > 0.0) salesTot = data.totalSalesRevenue;
    salesGrp.amount = salesTot;
    salesGrp.amountFmt = AccountingEngine::formatIndianCurrency(salesTot, true);
    salesGrp.side = "Cr";
    salesGrp.isGroup = true;
    salesGrp.level = 1;
    data.tradingCrGroups.append(salesGrp);

    // 2. Closing Stock
    ProfitLossItem closingStockGrp;
    closingStockGrp.name = "Closing Stock";
    closingStockGrp.groupName = "Trading Cr";

    StockValuationReport clRep = StockValuationEngine::getEffectiveClosingStock(data.toDate);
    data.closingStockValue = clRep.totalValuation;
    closingStockGrp.amount = clRep.totalValuation;
    closingStockGrp.amountFmt = clRep.totalValuationFmt;
    closingStockGrp.side = "Cr";
    closingStockGrp.isGroup = true;
    closingStockGrp.isCalculated = true;
    closingStockGrp.level = 1;

    for (const auto& s : clRep.items) {
        ProfitLossItem sItm;
        sItm.name = s.itemName + QString(" (%1 Qtl)").arg(s.weightQtl, 0, 'f', 1);
        sItm.groupName = "Closing Stock";
        sItm.amount = s.amount;
        sItm.amountFmt = s.amountFmt;
        sItm.side = "Cr";
        sItm.level = 2;
        closingStockGrp.children.append(sItm);
    }
    data.tradingCrGroups.append(closingStockGrp);

    // 3. Gross Loss c/d (if Gross Loss > 0)
    if (data.grossLoss > 0.0) {
        ProfitLossItem glItem;
        glItem.name = "Gross Loss c/d (Transferred to P&L A/c)";
        glItem.groupName = "Trading Cr";
        glItem.amount = data.grossLoss;
        glItem.amountFmt = AccountingEngine::formatIndianCurrency(data.grossLoss, true);
        glItem.side = "Cr";
        glItem.isCalculated = true;
        glItem.isGroup = false;
        glItem.level = 1;
        data.tradingCrGroups.append(glItem);
    }

    // Trading Totals
    double tradingSideTotal = std::max(totalTradingCrRaw, totalTradingDrRaw);
    data.totalTradingDr = tradingSideTotal;
    data.totalTradingCr = tradingSideTotal;
    data.totalTradingDrFmt = AccountingEngine::formatIndianCurrency(tradingSideTotal, true);
    data.totalTradingCrFmt = AccountingEngine::formatIndianCurrency(tradingSideTotal, true);

    // -------------------------------------------------------------
    // BUILD PROFIT & LOSS DR GROUPS (Left Side, Bottom)
    // -------------------------------------------------------------
    // 1. Gross Loss b/d (if any)
    if (data.grossLoss > 0.0) {
        ProfitLossItem glBd;
        glBd.name = "Gross Loss b/d";
        glBd.groupName = "P&L Dr";
        glBd.amount = data.grossLoss;
        glBd.amountFmt = AccountingEngine::formatIndianCurrency(data.grossLoss, true);
        glBd.side = "Dr";
        glBd.isCalculated = true;
        glBd.level = 1;
        data.plDrGroups.append(glBd);
    }

    // 2. Indirect Expenses
    for (auto it = indirectExpByGroup.begin(); it != indirectExpByGroup.end(); ++it) {
        ProfitLossItem iGrp;
        iGrp.name = it.key();
        iGrp.groupName = "Indirect Expenses";
        double gTot = 0.0;
        for (const auto& item : it.value()) {
            gTot += item.amount;
            iGrp.children.append(item);
        }
        iGrp.amount = gTot;
        iGrp.amountFmt = AccountingEngine::formatIndianCurrency(gTot, true);
        iGrp.side = "Dr";
        iGrp.isGroup = true;
        iGrp.level = 1;
        data.plDrGroups.append(iGrp);
    }
    if (indirectExpByGroup.isEmpty() && data.indirectExpenses > 0.0) {
        ProfitLossItem iGrp;
        iGrp.name = "Indirect Operating & Administrative Expenses";
        iGrp.groupName = "Indirect Expenses";
        iGrp.amount = data.indirectExpenses;
        iGrp.amountFmt = AccountingEngine::formatIndianCurrency(data.indirectExpenses, true);
        iGrp.side = "Dr";
        iGrp.isGroup = true;
        iGrp.level = 1;
        data.plDrGroups.append(iGrp);
    }

    // 3. Net Profit (if Net Profit > 0)
    if (data.netProfit > 0.0) {
        ProfitLossItem npItem;
        npItem.name = "Net Profit (Transferred to Capital Account)";
        npItem.groupName = "P&L Dr";
        npItem.amount = data.netProfit;
        npItem.amountFmt = AccountingEngine::formatIndianCurrency(data.netProfit, true);
        npItem.side = "Dr";
        npItem.isCalculated = true;
        npItem.level = 1;
        data.plDrGroups.append(npItem);
    }

    // -------------------------------------------------------------
    // BUILD PROFIT & LOSS CR GROUPS (Right Side, Bottom)
    // -------------------------------------------------------------
    // 1. Gross Profit b/d (if any)
    if (data.grossProfit > 0.0) {
        ProfitLossItem gpBd;
        gpBd.name = "Gross Profit b/d";
        gpBd.groupName = "P&L Cr";
        gpBd.amount = data.grossProfit;
        gpBd.amountFmt = AccountingEngine::formatIndianCurrency(data.grossProfit, true);
        gpBd.side = "Cr";
        gpBd.isCalculated = true;
        gpBd.level = 1;
        data.plCrGroups.append(gpBd);
    }

    // 2. Indirect Incomes
    for (auto it = indirectIncByGroup.begin(); it != indirectIncByGroup.end(); ++it) {
        ProfitLossItem iGrp;
        iGrp.name = it.key();
        iGrp.groupName = "Indirect Incomes";
        double gTot = 0.0;
        for (const auto& item : it.value()) {
            gTot += item.amount;
            iGrp.children.append(item);
        }
        iGrp.amount = gTot;
        iGrp.amountFmt = AccountingEngine::formatIndianCurrency(gTot, true);
        iGrp.side = "Cr";
        iGrp.isGroup = true;
        iGrp.level = 1;
        data.plCrGroups.append(iGrp);
    }
    if (indirectIncByGroup.isEmpty() && data.indirectIncomes > 0.0) {
        ProfitLossItem iGrp;
        iGrp.name = "Indirect Incomes & Receipts";
        iGrp.groupName = "Indirect Incomes";
        iGrp.amount = data.indirectIncomes;
        iGrp.amountFmt = AccountingEngine::formatIndianCurrency(data.indirectIncomes, true);
        iGrp.side = "Cr";
        iGrp.isGroup = true;
        iGrp.level = 1;
        data.plCrGroups.append(iGrp);
    }

    // 3. Net Loss (if Net Loss > 0)
    if (data.netLoss > 0.0) {
        ProfitLossItem nlItem;
        nlItem.name = "Net Loss (Deducted from Capital Account)";
        nlItem.groupName = "P&L Cr";
        nlItem.amount = data.netLoss;
        nlItem.amountFmt = AccountingEngine::formatIndianCurrency(data.netLoss, true);
        nlItem.side = "Cr";
        nlItem.isCalculated = true;
        nlItem.level = 1;
        data.plCrGroups.append(nlItem);
    }

    // P&L Totals
    double plSideTotal = std::max(totalPlCrRaw, totalPlDrRaw);
    data.totalPlDr = plSideTotal;
    data.totalPlCr = plSideTotal;
    data.totalPlDrFmt = AccountingEngine::formatIndianCurrency(plSideTotal, true);
    data.totalPlCrFmt = AccountingEngine::formatIndianCurrency(plSideTotal, true);

    // -------------------------------------------------------------
    // COMBINED EXPENSES AND INCOMES SIDES (WITH SECTION HEADERS)
    // -------------------------------------------------------------
    // Expenses Side (Dr)
    ProfitLossItem tradingDrSec;
    tradingDrSec.name = "TRADING ACCOUNT (EXPENSES)";
    tradingDrSec.isSectionHeader = true;
    tradingDrSec.level = 0;
    data.expensesSide.append(tradingDrSec);
    for (const auto& g : data.tradingDrGroups) data.expensesSide.append(g);

    ProfitLossItem plDrSec;
    plDrSec.name = "PROFIT & LOSS ACCOUNT (EXPENSES)";
    plDrSec.isSectionHeader = true;
    plDrSec.level = 0;
    data.expensesSide.append(plDrSec);
    for (const auto& g : data.plDrGroups) data.expensesSide.append(g);

    // Incomes Side (Cr)
    ProfitLossItem tradingCrSec;
    tradingCrSec.name = "TRADING ACCOUNT (INCOMES)";
    tradingCrSec.isSectionHeader = true;
    tradingCrSec.level = 0;
    data.incomesSide.append(tradingCrSec);
    for (const auto& g : data.tradingCrGroups) data.incomesSide.append(g);

    ProfitLossItem plCrSec;
    plCrSec.name = "PROFIT & LOSS ACCOUNT (INCOMES)";
    plCrSec.isSectionHeader = true;
    plCrSec.level = 0;
    data.incomesSide.append(plCrSec);
    for (const auto& g : data.plCrGroups) data.incomesSide.append(g);

    data.grandTotalExpenses = data.totalTradingDr + data.totalPlDr;
    data.grandTotalIncomes = data.totalTradingCr + data.totalPlCr;
    data.grandTotalExpensesFmt = AccountingEngine::formatIndianCurrency(data.grandTotalExpenses, true);
    data.grandTotalIncomesFmt = AccountingEngine::formatIndianCurrency(data.grandTotalIncomes, true);

    return data;
}
