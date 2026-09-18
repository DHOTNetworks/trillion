#pragma once

#include <QString>
#include <QVector>
#include <QVariantMap>
#include <QVariantList>

struct BalanceSheetItem {
    QString name;
    QString groupName;
    double amount = 0.0;
    QString amountFmt;
    QString balanceType; // "Dr" or "Cr"
    bool isGroup = false;
    bool isTotal = false;
    bool isCalculated = false; // Net Profit, Closing Stock, etc.
    int partyId = 0;
    int level = 0; // 0 = Major Group, 1 = Subgroup, 2 = Ledger/Party
    QVector<BalanceSheetItem> children;
};

struct BalanceSheetData {
    QString asOnDate;           // ISO format "YYYY-MM-DD"
    QString displayAsOnDate;    // "DD-MM-YYYY"
    QString financialYear;      // "FY 2026-27"
    QString firmName;
    QString firmAddress;
    QString firmGstin;

    // Left Side: Liabilities & Capital
    QVector<BalanceSheetItem> liabilitiesGroups;
    double totalLiabilities = 0.0;
    QString totalLiabilitiesFmt;

    // Right Side: Assets
    QVector<BalanceSheetItem> assetsGroups;
    double totalAssets = 0.0;
    QString totalAssetsFmt;

    // Mathematical reconciliation
    double difference = 0.0;
    bool isBalanced = true;

    // Financial Metrics
    double openingStockValue = 0.0;
    double closingStockValue = 0.0;
    double totalProcurement = 0.0;
    double totalDirectExpenses = 0.0;
    double totalSalesRevenue = 0.0;
    double grossProfit = 0.0;
    double indirectIncomes = 0.0;
    double indirectExpenses = 0.0;
    double netProfit = 0.0;
};

class BalanceSheetCalculator {
public:
    static BalanceSheetData calculate(const QString& requestedAsOnDate = "");
    static double calculateClosingStockValuation(const QString& asOnDateIso);
    static double calculateOpeningStockValuation(const QString& fyStartDateIso);
};
