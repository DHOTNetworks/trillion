#include "account_classifier.h"
#include <QStringList>

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
        case 35: return 9;  // Employees -> Current Liabilities
        default: return 0;
    }
}

QString AccountClassifier::getNatureForGroup(int code1, int code2, int code3, int code4) {
    // Check Assets: 2 (Current Assets), 14 (Fixed Assets), 26 (Deposit Assets), 30 (Security Assets), 3, 4, 6, 7, 8
    if (isDescendantOf(code1, code2, code3, code4, 2) ||
        isDescendantOf(code1, code2, code3, code4, 14) ||
        isDescendantOf(code1, code2, code3, code4, 26) ||
        isDescendantOf(code1, code2, code3, code4, 30)) {
        return "Assets";
    }

    // Check Liabilities: 1 (Capital), 9 (Current Liabilities), 10, 11, 12, 13, 28, 29, 31, 33, 35
    if (isDescendantOf(code1, code2, code3, code4, 1) ||
        isDescendantOf(code1, code2, code3, code4, 9) ||
        isDescendantOf(code1, code2, code3, code4, 13) ||
        isDescendantOf(code1, code2, code3, code4, 28) ||
        isDescendantOf(code1, code2, code3, code4, 29)) {
        return "Liabilities";
    }

    // Check Income: 16 (Income A/c), 20 (Sale A/c)
    if (isDescendantOf(code1, code2, code3, code4, 16) ||
        isDescendantOf(code1, code2, code3, code4, 20)) {
        return "Income";
    }

    // Check Expense: 17 (Expenditure), 19 (Purchase), 22, 23 (Mfg Exp), 24 (Trading Exp)
    if (isDescendantOf(code1, code2, code3, code4, 17) ||
        isDescendantOf(code1, code2, code3, code4, 18) ||
        isDescendantOf(code1, code2, code3, code4, 19) ||
        isDescendantOf(code1, code2, code3, code4, 22) ||
        isDescendantOf(code1, code2, code3, code4, 23) ||
        isDescendantOf(code1, code2, code3, code4, 24)) {
        return "Expense";
    }

    return "Assets";
}
