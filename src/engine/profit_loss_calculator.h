#pragma once

#include <QString>
#include <QVector>
#include <QVariantMap>
#include <QVariantList>

struct ProfitLossItem {
    QString name;
    QString groupName;
    double amount = 0.0;
    QString amountFmt;
    QString side; // "Dr" (Expense) or "Cr" (Income)
    bool isGroup = false;
    bool isSectionHeader = false;
    bool isCalculated = false; // Gross Profit, Net Profit, Closing Stock
    int partyId = 0;
    int level = 0; // 0 = Section Header, 1 = Major Group, 2 = Ledger/Party
    QVector<ProfitLossItem> children;
};

struct ProfitLossData {
    QString fromDate;           // ISO "YYYY-MM-DD"
    QString toDate;             // ISO "YYYY-MM-DD"
    QString displayFromDate;    // "DD-MM-YYYY"
    QString displayToDate;      // "DD-MM-YYYY"
    QString financialYear;      // "FY 2026-27"
    QString firmName;
    QString firmAddress;
    QString firmGstin;

    // Trading Account
    double openingStockValue = 0.0;
    double totalProcurement = 0.0;
    double totalDirectExpenses = 0.0;
    double totalSalesRevenue = 0.0;
    double closingStockValue = 0.0;

    double grossProfit = 0.0;   // If > 0, Gross Profit; if < 0, Gross Loss
    double grossLoss = 0.0;

    QVector<ProfitLossItem> tradingDrGroups; // Opening Stock, Purchases, Direct Expenses, Gross Profit c/d
    QVector<ProfitLossItem> tradingCrGroups; // Sales, Closing Stock, Gross Loss c/d
    double totalTradingDr = 0.0;
    double totalTradingCr = 0.0;
    QString totalTradingDrFmt;
    QString totalTradingCrFmt;

    // Profit & Loss Account
    double indirectIncomes = 0.0;
    double indirectExpenses = 0.0;
    double netProfit = 0.0;      // If > 0, Net Profit; if < 0, Net Loss
    double netLoss = 0.0;

    QVector<ProfitLossItem> plDrGroups; // Gross Loss b/d, Indirect Expenses, Net Profit
    QVector<ProfitLossItem> plCrGroups; // Gross Profit b/d, Indirect Incomes, Net Loss
    double totalPlDr = 0.0;
    double totalPlCr = 0.0;
    QString totalPlDrFmt;
    QString totalPlCrFmt;

    // Combined Full View Lists (for side-by-side display)
    QVector<ProfitLossItem> expensesSide; // Trading Dr + P&L Dr
    QVector<ProfitLossItem> incomesSide;  // Trading Cr + P&L Cr
    double grandTotalExpenses = 0.0;
    double grandTotalIncomes = 0.0;
    QString grandTotalExpensesFmt;
    QString grandTotalIncomesFmt;
};

class ProfitLossCalculator {
public:
    static ProfitLossData calculate(const QString& requestedFromDate = "", const QString& requestedToDate = "");
    static double calculateClosingStockValuation(const QString& asOnDateIso);
    static double calculateOpeningStockValuation(const QString& fyStartDateIso);
};
