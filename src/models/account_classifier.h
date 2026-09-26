#pragma once

#include <QString>
#include <QVector>
#include <QMap>

enum class StandardGroupCode : int {
    Primary = 0,
    Capital = 1,
    CurrentAssets = 2,
    BankAccounts = 3,
    CashInHand = 4,
    LoanAdvancesAssets = 6,
    StockInHand = 7,
    SundryDebtors = 8,
    CurrentLiabilities = 9,
    DutiesTaxes = 10,
    SundryCreditors = 11,
    Provisions = 12,
    LoansLiability = 13,
    FixedAssets = 14,
    ProfitAndLoss = 15,
    Income = 16,
    Expenditure = 17,
    TradingStockRoot = 18,
    Purchase = 19,
    Sale = 20,
    CommissionParties = 21,
    ManufacturingRoot = 22,
    ManufacturingExp = 23,
    TradingExp = 24,
    Suspense = 25,
    DepositAssets = 26,
    BranchesDivisions = 27,
    SecuredLoansCC = 28,
    UnsecuredLoans = 29,
    SecurityAssets = 30,
    LocalMandiCreditors = 31,
    ZimidaraDebtors = 32,
    ZimidaraCreditors = 33,
    MandiDebtors = 34,
    Employees = 35
};

class AccountClassifier {
public:
    // Generate SQL snippet to filter groups belonging to any of the given root integer codes
    // e.g. "((g.code1st IN (3, 28) OR g.code2nd IN (3, 28) OR g.code3rd IN (3, 28) OR g.code4th IN (3, 28)))"
    static QString generateHierarchySqlClause(const QVector<int>& rootCodes, const QString& groupTableAlias = "g");
    static QString generateHierarchySqlClause(const QVector<StandardGroupCode>& roots, const QString& groupTableAlias = "g");

    // Pure mathematical ancestor check: returns true if any tier matches targetRoot
    static bool isDescendantOf(int c1, int c2, int c3, int c4, int targetRoot);
    static bool isDescendantOf(int c1, int c2, int c3, int c4, StandardGroupCode targetRoot);

    // Standard Bahi-Khata Group Name for a master code
    static QString getStandardGroupName(int code);

    // Standard Parent Group Code for a master code
    static int getStandardGroupParent(int code);

    // Determine balance sheet nature ("Assets", "Liabilities", "Income", "Expense") deterministically
    static QString getNatureForGroup(int code1, int code2, int code3, int code4);
};
