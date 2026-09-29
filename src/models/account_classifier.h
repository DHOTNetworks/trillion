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

enum class StandardLedgerCode : int {
    Cash = 1,
    ProfitAndLoss = 2,
    RoundOff = 44,
    PaddyParmal = 45,
    VAT = 54,
    CST = 55,
    Dami = 56,
    MarketFee = 57,
    HRDF = 58,
    TDSCommission = 61,
    Bonus = 63,
    MaalKhata = 65,
    AuctionCharges = 66,
    AssociationCharges = 67,
    GaushalaCharges = 68,
    Commission = 69,
    Discount = 70,
    InsuranceCharges = 71,
    Jaffery = 72,
    Brokerage = 73,
    MiscExp = 74,
    Salary = 75,
    Depreciation = 76,
    FreightInward = 77,
    Labour = 78,
    Interest = 79,
    OtherExp = 80,
    Bardana = 209,
    TDSPayableInterest = 498,
    RiceBran = 500,
    PaddyHusk = 501,
    RiceBroken = 502,
    Machinery = 503,
    MachineryRepair = 504,
    BardanaExport = 505,
    Thread = 522,
    QualityClaim = 533,
    FreightExportRice = 542,
    FreightOutward = 543,
    ExportExpenses = 544,
    GeneralExpenses = 545,
    ElectricityCharges = 546,
    ProvidentFund = 547,
    Imprest = 548,
    ChequeIssuedNotCleared = 571,
    InterestPayable = 582,
    FreightPayable = 675,
    Rice = 680,
    GaneshJiMaharaj = 686,
    ProvidentFundPayable = 695,
    DutiesTaxesPayable = 697,
    PaddyBasmati = 882,
    TDSPayableLabour = 910,
    RiceBasmati = 942,
    SGST = 943,
    CGST = 944,
    IGST = 945,
    CESS = 946,
    ReverseChargePayable = 947,
    TCSPayable = 948,
    TCSReceivable = 949,
    TDS194Q = 950,
    TDSPayable = 951,
    InterestOnTax = 952,
    PenaltyOnTax = 953,
    DiscountAllowed = 954,
    InterestReceived = 955
};

struct GroupHierarchyInfo {
    qint64 id = 0;
    int code1 = 0;
    int code2 = 0;
    int code3 = 0;
    int code4 = 0;
    QString name;
    QString parentName;
    QString nature;
    int extractInBs = 1;
    bool isSystem = false;
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

    // Multi-tier Lineage & Financial Classification
    static bool isNominal(int c1, int c2, int c3, int c4);
    static bool isTrading(int c1, int c2, int c3, int c4);
    static bool isProfitAndLoss(int c1, int c2, int c3, int c4);
    static bool isDirectExpense(int c1, int c2, int c3, int c4, int calcDirectExpense = 0);
    static bool isBalanceSheetRealPersonal(int c1, int c2, int c3, int c4, int extractInBs = 1);
    static StandardGroupCode getRootGroup(int c1, int c2, int c3, int c4);

    // Predefined Standard Group Code Sets
    static QVector<StandardGroupCode> getBankGroupCodes();
    static QVector<StandardGroupCode> getBankAndCashGroupCodes();
    static QVector<StandardGroupCode> getDebtorsGroupCodes();
    static QVector<StandardGroupCode> getCreditorsGroupCodes();
    static QVector<StandardGroupCode> getFixedAssetsGroupCodes();
    static QVector<StandardGroupCode> getCapitalGroupCodes();
    static QVector<StandardGroupCode> getTradingStockGroupCodes();
    static QVector<StandardGroupCode> getTaxGroupCodes();

    // SQL Helper to join parties and account_groups
    static QString partiesJoinClause(const QString& partyAlias = "p", const QString& groupAlias = "g");

    // Dynamic Party / Group Classifier (replaces all string matching)
    static QString classifyPartyType(int c1, int c2, int c3, int c4);
    static QString classifyPartyTypeForGroup(const QString& groupName);
    static QString getParentGroupName(const QString& groupName);

    // Deterministic System Ledger Lookups (Exact matching by legacy code)
    static int getStandardLedgerId(StandardLedgerCode code);
    static int getStandardLedgerId(int legacyCode);
    static QString getStandardLedgerName(StandardLedgerCode code);
    static int getStandardLedgerGroupCode(StandardLedgerCode code);

    // In-memory Group Cache & Lookups ($O(1)$ fast lookups)
    static void invalidateCache();
    static GroupHierarchyInfo getGroupInfo(const QString& groupName);
    static GroupHierarchyInfo getGroupInfoByCode(int code1st);
    static GroupHierarchyInfo getGroupInfoById(qint64 id);

    // Semantic Inference & Self-Healing for Imported / Custom / Foreign Groups
    static StandardGroupCode inferRootCodeFromName(const QString& groupName);
    static qint64 ensureGroupExists(const QString& groupName, const QString& parentHint = "");
    static void healAllGroups();

    // Group Searching & Management
    static QVariantList searchGroups(const QString& query, int limit = 50);
    static QStringList getAllGroupNames();
    static QStringList getParentGroupNames();
    static QVariantMap getGroupDetails(const QString& groupName);
    static QVariantMap getGroupDetailsById(qint64 groupId);
    static bool createOrUpdateGroup(int groupId, const QString& name, const QString& parentGroupName, const QString& nature, const QString& desc, bool extractInBs, QString* outError = nullptr);
    static bool deleteGroup(int groupId, QString* outError = nullptr);
};
