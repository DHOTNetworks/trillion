#include "account_classifier.h"
#include "../database_manager.h"
#include <QStringList>
#include <QHash>
#include <QMutex>
#include <QMutexLocker>

static QMutex s_groupCacheMutex;
static bool s_groupCacheValid = false;
static QHash<QString, GroupHierarchyInfo> s_groupCacheByName;
static QHash<int, GroupHierarchyInfo> s_groupCacheByCode;
static QHash<qint64, GroupHierarchyInfo> s_groupCacheById;

static void ensureGroupCacheLoaded() {
    QMutexLocker locker(&s_groupCacheMutex);
    if (s_groupCacheValid) return;

    s_groupCacheByName.clear();
    s_groupCacheByCode.clear();
    s_groupCacheById.clear();

    QVariantList rows = DatabaseManager::instance().executeQuery(
        "SELECT id, name, parent_group_name, nature, extract_in_balance_sheet, is_system, code1st, code2nd, code3rd, code4th "
        "FROM account_groups;"
    );

    for (const auto& rowVar : rows) {
        QVariantMap r = rowVar.toMap();
        GroupHierarchyInfo info;
        info.id = r.value("id").toLongLong();
        info.name = r.value("name").toString().trimmed();
        info.parentName = r.value("parent_group_name").toString().trimmed();
        info.nature = r.value("nature").toString().trimmed();
        info.extractInBs = r.value("extract_in_balance_sheet").toInt();
        info.isSystem = (r.value("is_system").toInt() == 1);
        info.code1 = r.value("code1st").toInt();
        info.code2 = r.value("code2nd").toInt();
        info.code3 = r.value("code3rd").toInt();
        info.code4 = r.value("code4th").toInt();

        s_groupCacheByName.insert(info.name.toLower(), info);
        s_groupCacheByCode.insert(info.code1, info);
        s_groupCacheById.insert(info.id, info);
    }

    s_groupCacheValid = true;
}

void AccountClassifier::invalidateCache() {
    QMutexLocker locker(&s_groupCacheMutex);
    s_groupCacheByName.clear();
    s_groupCacheByCode.clear();
    s_groupCacheById.clear();
    s_groupCacheValid = false;
}

GroupHierarchyInfo AccountClassifier::getGroupInfo(const QString& groupName) {
    ensureGroupCacheLoaded();
    QMutexLocker locker(&s_groupCacheMutex);
    return s_groupCacheByName.value(groupName.trimmed().toLower());
}

GroupHierarchyInfo AccountClassifier::getGroupInfoByCode(int code1st) {
    ensureGroupCacheLoaded();
    QMutexLocker locker(&s_groupCacheMutex);
    return s_groupCacheByCode.value(code1st);
}

GroupHierarchyInfo AccountClassifier::getGroupInfoById(qint64 id) {
    ensureGroupCacheLoaded();
    QMutexLocker locker(&s_groupCacheMutex);
    return s_groupCacheById.value(id);
}

QString AccountClassifier::generateHierarchySqlClause(const QVector<int>& rootCodes, const QString& groupTableAlias) {
    if (rootCodes.isEmpty()) return "1=0";
    
    QStringList codeStrs;
    for (int c : rootCodes) {
        codeStrs << QString::number(c);
    }
    QString joined = codeStrs.join(", ");
    
    QString alias = groupTableAlias.trimmed();
    if (!alias.isEmpty() && !alias.endsWith('.')) {
        alias += ".";
    }

    return QString("((%1code1st IN (%2) OR %1code2nd IN (%2) OR %1code3rd IN (%2) OR %1code4th IN (%2)))")
        .arg(alias, joined);
}

QString AccountClassifier::generateHierarchySqlClause(const QVector<StandardGroupCode>& roots, const QString& groupTableAlias) {
    QVector<int> codes;
    codes.reserve(roots.size());
    for (auto r : roots) {
        codes.append(static_cast<int>(r));
    }
    return generateHierarchySqlClause(codes, groupTableAlias);
}

bool AccountClassifier::isDescendantOf(int c1, int c2, int c3, int c4, int targetRoot) {
    if (targetRoot <= 0) return false;
    return (c1 == targetRoot || c2 == targetRoot || c3 == targetRoot || c4 == targetRoot);
}

bool AccountClassifier::isDescendantOf(int c1, int c2, int c3, int c4, StandardGroupCode targetRoot) {
    return isDescendantOf(c1, c2, c3, c4, static_cast<int>(targetRoot));
}

QString AccountClassifier::getStandardGroupName(int code) {
    switch (code) {
        case 1:  return "Capital A/c";
        case 2:  return "Current Assets";
        case 3:  return "Bank(s) A/c";
        case 4:  return "Cash-In-Hand";
        case 6:  return "Loan & Advances (Assets)";
        case 7:  return "Stock-In-Hand";
        case 8:  return "Sundry Debtors";
        case 9:  return "Current Liabilities";
        case 10: return "Duties & Taxes";
        case 11: return "Sundry Creditors";
        case 12: return "Provisions";
        case 13: return "Loans (Liability)";
        case 14: return "Fixed Assets";
        case 15: return "Profit & Loss";
        case 16: return "Income A/c";
        case 17: return "Expenditure A/c";
        case 18: return "Trading Items Stock A/c";
        case 19: return "Purchase A/c";
        case 20: return "Sale A/c";
        case 21: return "Commission Basis Parties A/c";
        case 22: return "Manufacturing Group";
        case 23: return "Manufacturing Exp.";
        case 24: return "Trading Exp.";
        case 25: return "Suspense A/c";
        case 26: return "Deposit (Assets)";
        case 27: return "Branches / Divisions";
        case 28: return "Secured Loans";
        case 29: return "Unsecured Loans";
        case 30: return "Security A/c";
        case 31: return "Local Mandi Creditors";
        case 32: return "Zimidara Debtors";
        case 33: return "Zimidara Creditors";
        case 34: return "Mandi Debtors";
        case 35: return "Employees";
        case -1: return "Isht Dev";
        default: return "";
    }
}

int AccountClassifier::getStandardGroupParent(int code) {
    switch (code) {
        case 3:  return 2;  // Bank -> Current Assets
        case 4:  return 2;  // Cash -> Current Assets
        case 6:  return 2;  // Loan & Advances -> Current Assets
        case 7:  return 2;  // Stock-In-Hand -> Current Assets
        case 8:  return 2;  // Sundry Debtors -> Current Assets
        case 10: return 9;  // Duties & Taxes -> Current Liabilities
        case 11: return 9;  // Sundry Creditors -> Current Liabilities
        case 12: return 9;  // Provisions -> Current Liabilities
        case 13: return 9;  // Loans -> Current Liabilities
        case 16: return 15; // Income -> Profit & Loss
        case 17: return 15; // Expenditure -> Profit & Loss
        case 19: return 18; // Purchase -> Trading Root
        case 20: return 18; // Sale -> Trading Root
        case 23: return 22; // Manufacturing Exp -> Manufacturing Root
        case 24: return 18; // Trading Exp -> Trading Root
        case 26: return 2;  // Deposit -> Current Assets
        case 28: return 13; // Secured Loans -> Loans (Liability)
        case 29: return 13; // Unsecured Loans -> Loans (Liability)
        case 30: return 2;  // Security -> Current Assets
        case 31: return 11; // Local Mandi Creditors -> Sundry Creditors
        case 32: return 8;  // Zimidara Debtors -> Sundry Debtors
        case 33: return 11; // Zimidara Creditors -> Sundry Creditors
        case 34: return 8;  // Mandi Debtors -> Sundry Debtors
        case 35: return 8;  // Employees -> Sundry Debtors (Data.001)
        case -1: return 0;  // Isht Dev -> Primary
        default: return 0;
    }
}

QString AccountClassifier::getNatureForGroup(int code1, int code2, int code3, int code4) {
    if (isTrading(code1, code2, code3, code4)) {
        if (isDescendantOf(code1, code2, code3, code4, 20)) return "Income";
        return "Expense";
    }

    if (isProfitAndLoss(code1, code2, code3, code4)) {
        if (isDescendantOf(code1, code2, code3, code4, 16)) return "Income";
        return "Expense";
    }

    if (code1 == -1 ||
        isDescendantOf(code1, code2, code3, code4, 1) ||
        isDescendantOf(code1, code2, code3, code4, 9) ||
        isDescendantOf(code1, code2, code3, code4, 13) ||
        isDescendantOf(code1, code2, code3, code4, 28) ||
        isDescendantOf(code1, code2, code3, code4, 29)) {
        return "Liabilities";
    }

    return "Assets";
}

bool AccountClassifier::isNominal(int c1, int c2, int c3, int c4) {
    return isTrading(c1, c2, c3, c4) || isProfitAndLoss(c1, c2, c3, c4);
}

bool AccountClassifier::isTrading(int c1, int c2, int c3, int c4) {
    return isDescendantOf(c1, c2, c3, c4, 18) || // TradingStockRoot
           isDescendantOf(c1, c2, c3, c4, 19) || // Purchase
           isDescendantOf(c1, c2, c3, c4, 20) || // Sale
           isDescendantOf(c1, c2, c3, c4, 22) || // ManufacturingRoot
           isDescendantOf(c1, c2, c3, c4, 23) || // ManufacturingExp
           isDescendantOf(c1, c2, c3, c4, 24);   // TradingExp
}

bool AccountClassifier::isProfitAndLoss(int c1, int c2, int c3, int c4) {
    return isDescendantOf(c1, c2, c3, c4, 15) || // ProfitAndLoss
           isDescendantOf(c1, c2, c3, c4, 16) || // Income
           isDescendantOf(c1, c2, c3, c4, 17);   // Expenditure
}

bool AccountClassifier::isDirectExpense(int c1, int c2, int c3, int c4, int calcDirectExpense) {
    if (calcDirectExpense == 1) return true;
    return isDescendantOf(c1, c2, c3, c4, 22) ||
           isDescendantOf(c1, c2, c3, c4, 23) ||
           isDescendantOf(c1, c2, c3, c4, 24);
}

bool AccountClassifier::isBalanceSheetRealPersonal(int c1, int c2, int c3, int c4, int extractInBs) {
    if (extractInBs == 0) return false;
    return !isNominal(c1, c2, c3, c4);
}

StandardGroupCode AccountClassifier::getRootGroup(int c1, int c2, int c3, int c4) {
    int codes[4] = {c1, c2, c3, c4};
    for (int i = 3; i >= 0; --i) {
        int c = codes[i];
        if (c > 0 && c <= 35) {
            return static_cast<StandardGroupCode>(c);
        }
    }
    return StandardGroupCode::Primary;
}

QVector<StandardGroupCode> AccountClassifier::getBankGroupCodes() {
    return {StandardGroupCode::BankAccounts, StandardGroupCode::SecuredLoansCC};
}

QVector<StandardGroupCode> AccountClassifier::getBankAndCashGroupCodes() {
    return {StandardGroupCode::BankAccounts, StandardGroupCode::SecuredLoansCC, StandardGroupCode::CashInHand};
}

QVector<StandardGroupCode> AccountClassifier::getDebtorsGroupCodes() {
    return {StandardGroupCode::SundryDebtors, StandardGroupCode::MandiDebtors, StandardGroupCode::ZimidaraDebtors};
}

QVector<StandardGroupCode> AccountClassifier::getCreditorsGroupCodes() {
    return {StandardGroupCode::SundryCreditors, StandardGroupCode::LocalMandiCreditors, StandardGroupCode::ZimidaraCreditors};
}

QVector<StandardGroupCode> AccountClassifier::getFixedAssetsGroupCodes() {
    return {StandardGroupCode::FixedAssets};
}

QVector<StandardGroupCode> AccountClassifier::getCapitalGroupCodes() {
    return {StandardGroupCode::Capital};
}

QVector<StandardGroupCode> AccountClassifier::getTradingStockGroupCodes() {
    return {StandardGroupCode::TradingStockRoot, StandardGroupCode::StockInHand, StandardGroupCode::Purchase, StandardGroupCode::Sale};
}

QVector<StandardGroupCode> AccountClassifier::getTaxGroupCodes() {
    return {StandardGroupCode::DutiesTaxes};
}

QString AccountClassifier::partiesJoinClause(const QString& partyAlias, const QString& groupAlias) {
    QString p = partyAlias.trimmed();
    QString g = groupAlias.trimmed();
    return QString("LEFT JOIN account_groups %1 ON (%2.group_id = %1.id OR (%2.group_code > 0 AND %2.group_code = %1.code1st) OR %2.group_name = %1.name)")
        .arg(g, p);
}

QString AccountClassifier::classifyPartyType(int c1, int c2, int c3, int c4) {
    if (isDescendantOf(c1, c2, c3, c4, StandardGroupCode::SundryDebtors) ||
        isDescendantOf(c1, c2, c3, c4, StandardGroupCode::MandiDebtors) ||
        isDescendantOf(c1, c2, c3, c4, StandardGroupCode::ZimidaraDebtors)) {
        return "Buyer";
    }
    if (isDescendantOf(c1, c2, c3, c4, StandardGroupCode::SundryCreditors) ||
        isDescendantOf(c1, c2, c3, c4, StandardGroupCode::LocalMandiCreditors) ||
        isDescendantOf(c1, c2, c3, c4, StandardGroupCode::ZimidaraCreditors)) {
        return "Vendor";
    }
    if (isDescendantOf(c1, c2, c3, c4, StandardGroupCode::BankAccounts) ||
        isDescendantOf(c1, c2, c3, c4, StandardGroupCode::SecuredLoansCC)) {
        return "Bank";
    }
    if (isDescendantOf(c1, c2, c3, c4, StandardGroupCode::CashInHand)) {
        return "Cash";
    }
    if (isDescendantOf(c1, c2, c3, c4, StandardGroupCode::DutiesTaxes)) {
        return "Tax";
    }
    if (isDescendantOf(c1, c2, c3, c4, StandardGroupCode::FixedAssets)) {
        return "Asset";
    }
    if (isDescendantOf(c1, c2, c3, c4, StandardGroupCode::Sale) ||
        isDescendantOf(c1, c2, c3, c4, StandardGroupCode::Income)) {
        return "Income";
    }
    if (isDescendantOf(c1, c2, c3, c4, StandardGroupCode::Purchase) ||
        isDescendantOf(c1, c2, c3, c4, StandardGroupCode::Expenditure) ||
        isDescendantOf(c1, c2, c3, c4, StandardGroupCode::ManufacturingExp) ||
        isDescendantOf(c1, c2, c3, c4, StandardGroupCode::TradingExp)) {
        return "Expense";
    }
    return "General";
}

QString AccountClassifier::classifyPartyTypeForGroup(const QString& groupName) {
    QString gName = groupName.trimmed();
    if (gName.isEmpty()) return "General";

    GroupHierarchyInfo info = getGroupInfo(gName);
    if (info.id > 0 || info.code1 != 0 || info.code2 != 0 || info.code3 != 0 || info.code4 != 0) {
        return classifyPartyType(info.code1, info.code2, info.code3, info.code4);
    }

    // Fallback: Infer root code from name, auto-heal in account_groups, and classify
    StandardGroupCode inferredRoot = inferRootCodeFromName(gName);
    if (inferredRoot != StandardGroupCode::Primary) {
        int rootCode = static_cast<int>(inferredRoot);
        ensureGroupExists(gName);
        return classifyPartyType(0, rootCode, getStandardGroupParent(rootCode), 0);
    }

    return "General";
}

QString AccountClassifier::getParentGroupName(const QString& groupName) {
    QString gName = groupName.trimmed();
    if (gName.isEmpty()) return "";

    GroupHierarchyInfo info = getGroupInfo(gName);
    if (info.id > 0) {
        if (!info.parentName.isEmpty() && info.parentName != "Primary / Root Group" && info.parentName != "Primary") {
            return info.parentName;
        }
        if (info.code2 > 0) {
            QString stdParent = getStandardGroupName(info.code2);
            if (!stdParent.isEmpty()) return stdParent;
        }
        if (info.code1 > 0) {
            QString stdName = getStandardGroupName(info.code1);
            if (!stdName.isEmpty()) return stdName;
        }
    }

    // Fallback: Infer standard parent name
    StandardGroupCode inferredRoot = inferRootCodeFromName(gName);
    if (inferredRoot != StandardGroupCode::Primary) {
        QString stdName = getStandardGroupName(static_cast<int>(inferredRoot));
        if (!stdName.isEmpty()) return stdName;
    }

    return gName;
}

QString AccountClassifier::getStandardLedgerName(StandardLedgerCode code) {
    switch (code) {
        case StandardLedgerCode::Cash: return "Cash";
        case StandardLedgerCode::ProfitAndLoss: return "Profit & Loss";
        case StandardLedgerCode::RoundOff: return "Round Off A/c";
        case StandardLedgerCode::PaddyParmal: return "Paddy (Parmal) A/c";
        case StandardLedgerCode::VAT: return "VAT A/c";
        case StandardLedgerCode::CST: return "CST A/c";
        case StandardLedgerCode::Dami: return "Dami A/c";
        case StandardLedgerCode::MarketFee: return "Market Fee A/c";
        case StandardLedgerCode::HRDF: return "H.R.D.F. A/c";
        case StandardLedgerCode::TDSCommission: return "T.D.S. On Comm. A/c";
        case StandardLedgerCode::Bonus: return "Bonus A/c";
        case StandardLedgerCode::MaalKhata: return "Maal Khata";
        case StandardLedgerCode::AuctionCharges: return "Auction Charges";
        case StandardLedgerCode::AssociationCharges: return "Association Charges";
        case StandardLedgerCode::GaushalaCharges: return "Gaushala A/c";
        case StandardLedgerCode::Commission: return "Commission A/c";
        case StandardLedgerCode::Discount: return "Discount A/c";
        case StandardLedgerCode::InsuranceCharges: return "Insurance Charges";
        case StandardLedgerCode::Jaffery: return "Jaffery A/c";
        case StandardLedgerCode::Brokerage: return "Brokerage A/c";
        case StandardLedgerCode::MiscExp: return "Misc. Exp. A/c";
        case StandardLedgerCode::Salary: return "Salary A/c";
        case StandardLedgerCode::Depreciation: return "Depreciation A/c";
        case StandardLedgerCode::FreightInward: return "Freight Inward (Purc.)";
        case StandardLedgerCode::Labour: return "Labour A/c";
        case StandardLedgerCode::Interest: return "Interest A/c";
        case StandardLedgerCode::OtherExp: return "Other Expenses A/c";
        case StandardLedgerCode::Bardana: return "Bardana A/c";
        case StandardLedgerCode::TDSPayableInterest: return "T.D.S. (Payable) A/c [Interest]";
        case StandardLedgerCode::RiceBran: return "Rice Bran A/c";
        case StandardLedgerCode::PaddyHusk: return "Paddy Husk A/c";
        case StandardLedgerCode::RiceBroken: return "Rice Broken A/c";
        case StandardLedgerCode::Machinery: return "Machinery A/c";
        case StandardLedgerCode::MachineryRepair: return "Machinery Repair A/c";
        case StandardLedgerCode::BardanaExport: return "Bardana Export A/c";
        case StandardLedgerCode::Thread: return "Thread A/c";
        case StandardLedgerCode::QualityClaim: return "Quality Claim / Cut A/c";
        case StandardLedgerCode::FreightExportRice: return "Freight (Export Rice)";
        case StandardLedgerCode::FreightOutward: return "Freight Outward (Sale)";
        case StandardLedgerCode::ExportExpenses: return "Export Expenses";
        case StandardLedgerCode::GeneralExpenses: return "General Expenses";
        case StandardLedgerCode::ElectricityCharges: return "Electricity Charges";
        case StandardLedgerCode::ProvidentFund: return "Provident Fund";
        case StandardLedgerCode::Imprest: return "Imprest A/c";
        case StandardLedgerCode::ChequeIssuedNotCleared: return "Cheque Issued But Not Cleared";
        case StandardLedgerCode::InterestPayable: return "Interest Payable";
        case StandardLedgerCode::FreightPayable: return "Freight Payable A/c";
        case StandardLedgerCode::Rice: return "Rice A/c";
        case StandardLedgerCode::GaneshJiMaharaj: return "Ganesh Ji Maharaj";
        case StandardLedgerCode::ProvidentFundPayable: return "Provident Fund Payable";
        case StandardLedgerCode::DutiesTaxesPayable: return "Duties & Taxes Payable";
        case StandardLedgerCode::PaddyBasmati: return "Paddy (Basmati) A/c";
        case StandardLedgerCode::TDSPayableLabour: return "T.D.S. (Payable) A/c [Labour]";
        case StandardLedgerCode::RiceBasmati: return "Rice (Basmati) A/c";
        case StandardLedgerCode::SGST: return "SGST A/c";
        case StandardLedgerCode::CGST: return "CGST A/c";
        case StandardLedgerCode::IGST: return "IGST A/c";
        case StandardLedgerCode::CESS: return "CESS A/c";
        case StandardLedgerCode::ReverseChargePayable: return "Reverse Charge Payable";
        case StandardLedgerCode::TCSPayable: return "TCS Payable A/c";
        case StandardLedgerCode::TCSReceivable: return "TCS Receivable A/c";
        case StandardLedgerCode::TDS194Q: return "TDS 194Q A/c";
        case StandardLedgerCode::TDSPayable: return "TDS Payable A/c";
        case StandardLedgerCode::InterestOnTax: return "Interest On Tax";
        case StandardLedgerCode::PenaltyOnTax: return "Penalty On Tax";
        case StandardLedgerCode::DiscountAllowed: return "Discount Allowed A/c";
        case StandardLedgerCode::InterestReceived: return "Interest Received A/c";
        default: return "";
    }
}

int AccountClassifier::getStandardLedgerGroupCode(StandardLedgerCode code) {
    switch (code) {
        case StandardLedgerCode::Cash: return 4;
        case StandardLedgerCode::ProfitAndLoss: return 15;
        case StandardLedgerCode::RoundOff: return 17;
        case StandardLedgerCode::PaddyParmal: return 18;
        case StandardLedgerCode::VAT: return 10;
        case StandardLedgerCode::CST: return 10;
        case StandardLedgerCode::Dami: return 17;
        case StandardLedgerCode::MarketFee: return 17;
        case StandardLedgerCode::HRDF: return 17;
        case StandardLedgerCode::TDSCommission: return 10;
        case StandardLedgerCode::Bonus: return 17;
        case StandardLedgerCode::MaalKhata: return 18;
        case StandardLedgerCode::AuctionCharges: return 17;
        case StandardLedgerCode::AssociationCharges: return 17;
        case StandardLedgerCode::GaushalaCharges: return 17;
        case StandardLedgerCode::Commission: return 17;
        case StandardLedgerCode::Discount: return 15;
        case StandardLedgerCode::InsuranceCharges: return 17;
        case StandardLedgerCode::Jaffery: return 17;
        case StandardLedgerCode::Brokerage: return 17;
        case StandardLedgerCode::MiscExp: return 17;
        case StandardLedgerCode::Salary: return 17;
        case StandardLedgerCode::Depreciation: return 17;
        case StandardLedgerCode::FreightInward: return 17;
        case StandardLedgerCode::Labour: return 17;
        case StandardLedgerCode::Interest: return 17;
        case StandardLedgerCode::OtherExp: return 17;
        case StandardLedgerCode::Bardana: return 18;
        case StandardLedgerCode::TDSPayableInterest: return 10;
        case StandardLedgerCode::RiceBran: return 18;
        case StandardLedgerCode::PaddyHusk: return 18;
        case StandardLedgerCode::RiceBroken: return 18;
        case StandardLedgerCode::Machinery: return 14;
        case StandardLedgerCode::MachineryRepair: return 17;
        case StandardLedgerCode::BardanaExport: return 18;
        case StandardLedgerCode::Thread: return 18;
        case StandardLedgerCode::QualityClaim: return 17;
        case StandardLedgerCode::FreightExportRice: return 17;
        case StandardLedgerCode::FreightOutward: return 17;
        case StandardLedgerCode::ExportExpenses: return 17;
        case StandardLedgerCode::GeneralExpenses: return 17;
        case StandardLedgerCode::ElectricityCharges: return 17;
        case StandardLedgerCode::ProvidentFund: return 17;
        case StandardLedgerCode::Imprest: return 2;
        case StandardLedgerCode::ChequeIssuedNotCleared: return 9;
        case StandardLedgerCode::InterestPayable: return 12;
        case StandardLedgerCode::FreightPayable: return 12;
        case StandardLedgerCode::Rice: return 18;
        case StandardLedgerCode::GaneshJiMaharaj: return -1;
        case StandardLedgerCode::ProvidentFundPayable: return 12;
        case StandardLedgerCode::DutiesTaxesPayable: return 12;
        case StandardLedgerCode::PaddyBasmati: return 18;
        case StandardLedgerCode::TDSPayableLabour: return 10;
        case StandardLedgerCode::RiceBasmati: return 18;
        case StandardLedgerCode::SGST: return 10;
        case StandardLedgerCode::CGST: return 10;
        case StandardLedgerCode::IGST: return 10;
        case StandardLedgerCode::CESS: return 10;
        case StandardLedgerCode::ReverseChargePayable: return 10;
        case StandardLedgerCode::TCSPayable: return 10;
        case StandardLedgerCode::TCSReceivable: return 10;
        case StandardLedgerCode::TDS194Q: return 10;
        case StandardLedgerCode::TDSPayable: return 10;
        case StandardLedgerCode::InterestOnTax: return 10;
        case StandardLedgerCode::PenaltyOnTax: return 10;
        case StandardLedgerCode::DiscountAllowed: return 17;
        case StandardLedgerCode::InterestReceived: return 16;
        default: return 0;
    }
}

int AccountClassifier::getStandardLedgerId(StandardLedgerCode code) {
    int legCode = static_cast<int>(code);
    return getStandardLedgerId(legCode);
}

int AccountClassifier::getStandardLedgerId(int legacyCode) {
    if (legacyCode == 0) return 0;

    auto& db = DatabaseManager::instance();
    QVariant val = db.executeScalar(
        "SELECT id FROM parties WHERE legacy_id = ? LIMIT 1;",
        {legacyCode}
    );
    if (val.isValid() && !val.isNull() && val.toInt() > 0) {
        return val.toInt();
    }

    // Fallback: lookup by standard ledger name
    QString stdName = getStandardLedgerName(static_cast<StandardLedgerCode>(legacyCode));
    if (!stdName.isEmpty()) {
        QVariant valByName = db.executeScalar(
            "SELECT id FROM parties WHERE name = ? LIMIT 1;",
            {stdName}
        );
        if (valByName.isValid() && !valByName.isNull() && valByName.toInt() > 0) {
            return valByName.toInt();
        }
    }

    return 0;
}

StandardGroupCode AccountClassifier::inferRootCodeFromName(const QString& groupName) {
    QString n = groupName.trimmed().toLower();
    if (n.isEmpty()) return StandardGroupCode::Primary;

    // 1. Bank & Cash
    if (n.contains("bank") || n.contains("c/c") || n.contains("od account") || n.contains("overdraft")) return StandardGroupCode::BankAccounts;
    if (n.contains("cash")) return StandardGroupCode::CashInHand;

    // 2. Debtors / Buyers / Farmers
    if (n.contains("zimidar") || n.contains("kisan") || n.contains("farmer") || n.contains("grower")) return StandardGroupCode::ZimidaraDebtors;
    if (n.contains("mandi debtor")) return StandardGroupCode::MandiDebtors;
    if (n.contains("debtor") || n.contains("debitor") || n.contains("buyer") || n.contains("customer") || n.contains("receivable")) return StandardGroupCode::SundryDebtors;

    // 3. Creditors / Suppliers / Mandi Creditors
    if (n.contains("local mandi") || n.contains("kacha arhtia") || n.contains("pucca arhtia") || n.contains("mandi creditor") || n.contains("mandi trader")) return StandardGroupCode::LocalMandiCreditors;
    if (n.contains("creditor") || n.contains("supplier") || n.contains("vendor") || n.contains("payable") || n.contains("seller")) return StandardGroupCode::SundryCreditors;

    // 4. Duties & Taxes
    if (n.contains("tax") || n.contains("gst") || n.contains("duty") || n.contains("duties") || 
        n.contains("tds") || n.contains("tcs") || n.contains("vat") || n.contains("cess") ||
        n.contains("market fee") || n.contains("hrdf") || n.contains("rdfs") || n.contains("kalyan")) {
        return StandardGroupCode::DutiesTaxes;
    }

    // 5. Fixed Assets
    if (n.contains("fixed asset") || n.contains("machinery") || n.contains("building") || 
        n.contains("furniture") || n.contains("vehicle") || n.contains("computer") || 
        n.contains("plant") || n.contains("equipment") || n.contains("land") || n.contains("godown")) {
        return StandardGroupCode::FixedAssets;
    }

    // 6. Capital & Loans
    if (n.contains("capital") || n.contains("drawing") || n.contains("partner") || n.contains("shareholder") || n.contains("proprietor")) {
        return StandardGroupCode::Capital;
    }
    if (n.contains("secured loan")) return StandardGroupCode::SecuredLoansCC;
    if (n.contains("unsecured loan")) return StandardGroupCode::UnsecuredLoans;
    if (n.contains("loan") || n.contains("borrowing")) return StandardGroupCode::LoansLiability;

    // 7. Trading & Stock
    if (n.contains("stock") || n.contains("inventory") || n.contains("bardana") || n.contains("packaging")) return StandardGroupCode::StockInHand;
    if (n.contains("purchase") || n.contains("procurement")) return StandardGroupCode::Purchase;
    if (n.contains("sale") || n.contains("revenue") || n.contains("billing")) return StandardGroupCode::Sale;
    if (n.contains("thekedar") || n.contains("manufacturing")) return StandardGroupCode::ManufacturingExp;
    if (n.contains("trading exp")) return StandardGroupCode::TradingExp;

    // 8. Nominal (Income / Expense)
    if (n.contains("expense") || n.contains("exp.") || n.contains("salary") || n.contains("wage") || 
        n.contains("rent") || n.contains("depreciation") || n.contains("depriciation") || 
        n.contains("discount allowed") || n.contains("interest paid") || n.contains("freight") || 
        n.contains("labour") || n.contains("electricity") || n.contains("repair") || n.contains("audit") || n.contains("broker")) {
        return StandardGroupCode::Expenditure;
    }
    if (n.contains("income") || n.contains("interest received") || n.contains("commission received") || 
        n.contains("discount received") || n.contains("brokerage")) {
        return StandardGroupCode::Income;
    }
    if (n.contains("employee") || n.contains("staff")) return StandardGroupCode::Employees;
    if (n.contains("family")) return StandardGroupCode::LoanAdvancesAssets;
    if (n.contains("suspense")) return StandardGroupCode::Suspense;
    if (n.contains("branch") || n.contains("division")) return StandardGroupCode::BranchesDivisions;
    if (n.contains("deposit")) return StandardGroupCode::DepositAssets;
    if (n.contains("provision")) return StandardGroupCode::Provisions;

    return StandardGroupCode::Primary;
}

qint64 AccountClassifier::ensureGroupExists(const QString& groupName, const QString& parentHint) {
    QString gName = groupName.trimmed();
    if (gName.isEmpty()) return 0;

    auto& db = DatabaseManager::instance();
    QVariantList rows = db.executeQuery(
        "SELECT id, code1st, code2nd, code3rd, code4th FROM account_groups WHERE name = ? LIMIT 1;",
        {gName}
    );

    if (!rows.isEmpty()) {
        QVariantMap r = rows.first().toMap();
        qint64 gid = r.value("id").toLongLong();
        int c1 = r.value("code1st").toInt();
        int c2 = r.value("code2nd").toInt();
        int c3 = r.value("code3rd").toInt();
        int c4 = r.value("code4th").toInt();

        // If it exists but has all 0 codes, heal it!
        if (c1 == 0 && c2 == 0 && c3 == 0 && c4 == 0) {
            StandardGroupCode root = inferRootCodeFromName(!parentHint.isEmpty() ? parentHint : gName);
            int rootCode = static_cast<int>(root);
            if (rootCode > 0) {
                int pCode = getStandardGroupParent(rootCode);
                QString nature = getNatureForGroup(0, rootCode, pCode, 0);
                QString pName = getStandardGroupName(rootCode);
                int extractBs = isNominal(0, rootCode, pCode, 0) ? 0 : 1;
                db.executeNonQuery(
                    "UPDATE account_groups SET parent_group_name = ?, nature = ?, extract_in_balance_sheet = ?, code2nd = ?, code3rd = ? WHERE id = ?;",
                    {pName, nature, extractBs, rootCode, pCode, gid}
                );
                invalidateCache();
            }
        }
        return gid;
    }

    // New unknown group -> auto-create with inferred 4-tier lineage
    StandardGroupCode root = inferRootCodeFromName(!parentHint.isEmpty() ? parentHint : gName);
    int rootCode = static_cast<int>(root);
    int parentCode = (rootCode > 0) ? getStandardGroupParent(rootCode) : 0;
    QString nature = getNatureForGroup(0, rootCode, parentCode, 0);
    QString parentName = (rootCode > 0) ? getStandardGroupName(rootCode) : "Primary / Root Group";
    int extractBs = isNominal(0, rootCode, parentCode, 0) ? 0 : 1;

    // Assign next code1st
    QVariant maxCode = db.executeScalar("SELECT MAX(code1st) FROM account_groups;");
    int nextCode = (maxCode.isValid() && !maxCode.isNull() && maxCode.toInt() >= 100) ? maxCode.toInt() + 1 : 101;

    bool ok = db.executeNonQuery(
        "INSERT INTO account_groups (name, parent_group_name, nature, description, extract_in_balance_sheet, is_system, code1st, code2nd, code3rd, code4th) "
        "VALUES (?, ?, ?, ?, ?, 0, ?, ?, ?, 0);",
        {gName, parentName, nature, QString("Auto-Inferred Group #%1").arg(nextCode), extractBs, nextCode, rootCode, parentCode}
    );

    invalidateCache();
    return ok ? db.lastInsertedId() : 0;
}

void AccountClassifier::healAllGroups() {
    auto& db = DatabaseManager::instance();

    // 1. Ensure all distinct group_name values in parties exist in account_groups
    QVariantList partyGroups = db.executeQuery(
        "SELECT DISTINCT TRIM(group_name) AS gname FROM parties WHERE group_name IS NOT NULL AND TRIM(group_name) != '';"
    );
    for (const auto& r : partyGroups) {
        QString gn = r.toMap().value("gname").toString().trimmed();
        if (!gn.isEmpty()) {
            ensureGroupExists(gn);
        }
    }

    // 2. Backfill group_id and group_code on parties where they are missing (0 or NULL)
    db.executeNonQuery(
        "UPDATE parties "
        "SET group_id = (SELECT g.id FROM account_groups g WHERE g.name = parties.group_name LIMIT 1), "
        "    group_code = (SELECT g.code1st FROM account_groups g WHERE g.name = parties.group_name LIMIT 1) "
        "WHERE (group_id IS NULL OR group_id = 0 OR group_code IS NULL OR group_code = 0) "
        "  AND group_name IS NOT NULL AND group_name != '';"
    );

    // 3. Heal any account_groups with all 0 codes
    QVariantList zeroGroups = db.executeQuery(
        "SELECT id, name FROM account_groups WHERE code1st = 0 AND code2nd = 0 AND code3rd = 0 AND code4th = 0;"
    );
    for (const auto& zg : zeroGroups) {
        qint64 gid = zg.toMap().value("id").toLongLong();
        QString gn = zg.toMap().value("name").toString();
        StandardGroupCode root = inferRootCodeFromName(gn);
        int rootCode = static_cast<int>(root);
        if (rootCode > 0) {
            int pCode = getStandardGroupParent(rootCode);
            QString nature = getNatureForGroup(0, rootCode, pCode, 0);
            QString pName = getStandardGroupName(rootCode);
            int extractBs = isNominal(0, rootCode, pCode, 0) ? 0 : 1;
            db.executeNonQuery(
                "UPDATE account_groups SET parent_group_name = ?, nature = ?, extract_in_balance_sheet = ?, code2nd = ?, code3rd = ? WHERE id = ?;",
                {pName, nature, extractBs, rootCode, pCode, gid}
            );
        }
    }

    invalidateCache();
}

QVariantList AccountClassifier::searchGroups(const QString& query, int limit) {
    auto& db = DatabaseManager::instance();
    QString q = query.trimmed();
    QVariantList rows;

    if (q.isEmpty()) {
        rows = db.executeQuery(
            QString("SELECT id, name, parent_group_name, nature, description, extract_in_balance_sheet, is_system, code1st, code2nd, code3rd, code4th "
                    "FROM account_groups ORDER BY name COLLATE NOCASE ASC LIMIT %1;").arg(limit)
        );
    } else {
        QString pattern = "%" + q + "%";
        QString prefixPattern = q + "%";
        rows = db.executeQuery(
            QString("SELECT id, name, parent_group_name, nature, description, extract_in_balance_sheet, is_system, code1st, code2nd, code3rd, code4th "
                    "FROM account_groups "
                    "WHERE name LIKE ? OR parent_group_name LIKE ? OR nature LIKE ? OR CAST(code1st AS TEXT) LIKE ? "
                    "ORDER BY CASE "
                    "  WHEN name LIKE ? THEN 0 "
                    "  WHEN name LIKE ? THEN 1 "
                    "  ELSE 2 "
                    "END, name COLLATE NOCASE ASC LIMIT %1;").arg(limit),
            {pattern, pattern, pattern, pattern, prefixPattern, pattern}
        );
    }

    QVariantList results;
    results.reserve(rows.size());
    for (const auto& rVar : rows) {
        QVariantMap r = rVar.toMap();
        QVariantMap item;
        item["id"] = r.value("id");
        item["name"] = r.value("name");
        item["party_name"] = r.value("name");
        item["title"] = r.value("name");
        item["parent_group_name"] = r.value("parent_group_name");
        item["parentGroup"] = r.value("parent_group_name");
        item["nature"] = r.value("nature");
        item["description"] = r.value("description");
        item["code1st"] = r.value("code1st");
        item["code2nd"] = r.value("code2nd");
        item["code3rd"] = r.value("code3rd");
        item["code4th"] = r.value("code4th");
        item["extract_in_balance_sheet"] = r.value("extract_in_balance_sheet");
        item["affectsBalanceSheet"] = (r.value("extract_in_balance_sheet").toInt() == 1);
        item["is_system"] = (r.value("is_system").toInt() == 1);

        // Friendly subtitle info for search popup
        QString parent = r.value("parent_group_name").toString().trimmed();
        QString nature = r.value("nature").toString().trimmed();
        int code1 = r.value("code1st").toInt();
        item["city"] = !parent.isEmpty() ? QString("Under: %1 | %2").arg(parent, nature) : nature;
        item["gstin"] = code1 > 0 ? QString("Code #%1").arg(code1) : "";

        results.append(item);
    }
    return results;
}

QStringList AccountClassifier::getAllGroupNames() {
    auto& db = DatabaseManager::instance();
    QVariantList rows = db.executeQuery(
        "SELECT DISTINCT name FROM account_groups WHERE name IS NOT NULL AND TRIM(name) != '' "
        "ORDER BY name COLLATE NOCASE ASC;"
    );
    QStringList result;
    result.reserve(rows.size());
    for (const auto& r : rows) {
        QString n = r.toMap().value("name").toString().trimmed();
        if (!n.isEmpty() && !result.contains(n, Qt::CaseInsensitive)) {
            result.append(n);
        }
    }
    return result;
}

QStringList AccountClassifier::getParentGroupNames() {
    QStringList result;
    result << "Primary";
    auto& db = DatabaseManager::instance();
    QVariantList rows = db.executeQuery(
        "SELECT DISTINCT parent_group_name AS name FROM account_groups WHERE parent_group_name IS NOT NULL AND TRIM(parent_group_name) != '' "
        "UNION "
        "SELECT DISTINCT name FROM account_groups WHERE name IS NOT NULL AND TRIM(name) != '' "
        "ORDER BY 1 COLLATE NOCASE ASC;"
    );
    for (const auto& r : rows) {
        QString n = r.toMap().value("name").toString().trimmed();
        if (!n.isEmpty() && n != "Primary / Root Group" && !result.contains(n, Qt::CaseInsensitive)) {
            result.append(n);
        }
    }
    return result;
}

QVariantMap AccountClassifier::getGroupDetails(const QString& groupName) {
    QString cleanName = groupName.trimmed();
    if (cleanName.isEmpty()) return {};

    auto& db = DatabaseManager::instance();
    QVariantList rows = db.executeQuery(
        "SELECT * FROM account_groups WHERE name = ? COLLATE NOCASE LIMIT 1;",
        {cleanName}
    );
    if (!rows.isEmpty()) {
        QVariantMap r = rows.first().toMap();
        r["parentGroup"] = r.value("parent_group_name");
        r["affectsBalanceSheet"] = (r.value("extract_in_balance_sheet").toInt() == 1);
        return r;
    }
    return {};
}

QVariantMap AccountClassifier::getGroupDetailsById(qint64 groupId) {
    if (groupId <= 0) return {};
    auto& db = DatabaseManager::instance();
    QVariantList rows = db.executeQuery(
        "SELECT * FROM account_groups WHERE id = ? LIMIT 1;",
        {groupId}
    );
    if (!rows.isEmpty()) {
        QVariantMap r = rows.first().toMap();
        r["parentGroup"] = r.value("parent_group_name");
        r["affectsBalanceSheet"] = (r.value("extract_in_balance_sheet").toInt() == 1);
        return r;
    }
    return {};
}

bool AccountClassifier::createOrUpdateGroup(int groupId, const QString& name, const QString& parentGroupName, const QString& nature, const QString& desc, bool extractInBs, QString* outError) {
    QString gName = name.trimmed();
    if (gName.isEmpty()) {
        if (outError) *outError = "Group Name cannot be empty.";
        return false;
    }

    auto& db = DatabaseManager::instance();
    QString pName = parentGroupName.trimmed();
    if (pName.isEmpty() || pName.compare("Primary", Qt::CaseInsensitive) == 0 || pName.compare("Primary / Root Group", Qt::CaseInsensitive) == 0) {
        pName = "Primary";
    }

    int code1 = 0;
    int code2 = 0;
    int code3 = 0;
    int code4 = 0;

    if (pName != "Primary") {
        GroupHierarchyInfo pInfo = getGroupInfo(pName);
        if (pInfo.id > 0 || pInfo.code1 > 0) {
            code2 = pInfo.code1;
            code3 = pInfo.code2;
            code4 = pInfo.code3;
        } else {
            StandardGroupCode root = inferRootCodeFromName(pName);
            int rootCode = static_cast<int>(root);
            if (rootCode > 0) {
                code2 = rootCode;
                code3 = getStandardGroupParent(rootCode);
                code4 = 0;
            }
        }
    }

    QString nat = nature.trimmed();
    if (nat.isEmpty()) {
        nat = getNatureForGroup(0, code2, code3, code4);
        if (nat.isEmpty()) nat = "Assets";
    }

    int extractBs = extractInBs ? 1 : 0;

    if (groupId <= 0) {
        // Check for duplicate name
        QVariant existing = db.executeScalar("SELECT id FROM account_groups WHERE name = ? COLLATE NOCASE LIMIT 1;", {gName});
        if (existing.isValid() && !existing.isNull()) {
            if (outError) *outError = QString("An account group with name '%1' already exists.").arg(gName);
            return false;
        }

        // Assign next code1st
        QVariant maxCode = db.executeScalar("SELECT MAX(code1st) FROM account_groups;");
        int nextCode = (maxCode.isValid() && !maxCode.isNull() && maxCode.toInt() >= 100) ? maxCode.toInt() + 1 : 101;
        code1 = nextCode;

        bool ok = db.executeNonQuery(
            "INSERT INTO account_groups (name, parent_group_name, nature, description, extract_in_balance_sheet, is_system, code1st, code2nd, code3rd, code4th) "
            "VALUES (?, ?, ?, ?, ?, 0, ?, ?, ?, ?);",
            {gName, pName, nat, desc.trimmed(), extractBs, code1, code2, code3, code4}
        );
        if (!ok) {
            if (outError) *outError = "Failed to insert account group into database.";
            return false;
        }
    } else {
        // Updating existing
        QVariant oldRow = db.executeScalar("SELECT code1st FROM account_groups WHERE id = ? LIMIT 1;", {groupId});
        if (!oldRow.isValid() || oldRow.isNull()) {
            if (outError) *outError = "Account group not found.";
            return false;
        }
        code1 = oldRow.toInt();

        // Check for duplicate name under different id
        QVariant dupId = db.executeScalar("SELECT id FROM account_groups WHERE name = ? COLLATE NOCASE AND id != ? LIMIT 1;", {gName, groupId});
        if (dupId.isValid() && !dupId.isNull()) {
            if (outError) *outError = QString("Another account group with name '%1' already exists.").arg(gName);
            return false;
        }

        bool ok = db.executeNonQuery(
            "UPDATE account_groups SET name = ?, parent_group_name = ?, nature = ?, description = ?, extract_in_balance_sheet = ?, code2nd = ?, code3rd = ?, code4th = ? "
            "WHERE id = ?;",
            {gName, pName, nat, desc.trimmed(), extractBs, code2, code3, code4, groupId}
        );
        if (!ok) {
            if (outError) *outError = "Failed to update account group in database.";
            return false;
        }

        // Propagate group name and updated party_type to linked parties
        QString newPartyType = classifyPartyType(code1, code2, code3, code4);
        db.executeNonQuery(
            "UPDATE parties SET group_name = ?, group_code = ?, party_type = ? WHERE group_id = ? OR (group_code > 0 AND group_code = ?);",
            {gName, code1, newPartyType, groupId, code1}
        );
    }

    invalidateCache();
    return true;
}

bool AccountClassifier::deleteGroup(int groupId, QString* outError) {
    if (groupId <= 0) return false;
    auto& db = DatabaseManager::instance();

    // Check if system group
    QVariant isSys = db.executeScalar("SELECT is_system FROM account_groups WHERE id = ? LIMIT 1;", {groupId});
    if (isSys.isValid() && isSys.toInt() == 1) {
        if (outError) *outError = "System default account groups cannot be deleted.";
        return false;
    }

    // Check if any parties are using this group
    QVariant pCount = db.executeScalar("SELECT COUNT(*) FROM parties WHERE group_id = ?;", {groupId});
    if (pCount.isValid() && pCount.toInt() > 0) {
        if (outError) *outError = QString("Cannot delete this group: %1 ledger accounts are currently linked to it.").arg(pCount.toInt());
        return false;
    }

    bool ok = db.executeNonQuery("DELETE FROM account_groups WHERE id = ?;", {groupId});
    if (ok) {
        invalidateCache();
    }
    return ok;
}
