#pragma once

// Shared print data lookups for ALL report renderers (moved verbatim from
// PrintExportController's private helpers — same queries, same keys).
#include <QString>
#include <QVariantMap>
#include <QVariantList>
#include <QRegularExpression>
#include <cmath>

#include "../../database_manager.h"

namespace PrintReports {

inline QVariantMap getFirmProfile() {
    QVariantMap firm;
    QVariantList rows = DatabaseManager::instance().executeQuery("SELECT * FROM company_info LIMIT 1;");
    if (!rows.isEmpty()) {
        QVariantMap r = rows.first().toMap();
        firm["firm_name"] = r.value("company_name").toString().trimmed();
        firm["address"] = r.value("address").toString().trimmed();
        firm["city"] = r.value("city").toString().trimmed();
        firm["phone"] = r.value("phone").toString().trimmed();
        if (firm.value("phone").toString().isEmpty()) {
            firm["phone"] = r.value("mobile").toString().trimmed();
        }
        firm["email"] = r.value("email").toString().trimmed();
        firm["gstin"] = r.value("gstin").toString().trimmed();
        QString stateStr = r.value("state").toString().trimmed();
        QString stateCode = r.value("state_code").toString().trimmed();
        if (!stateCode.isEmpty() && !stateStr.contains(stateCode)) {
            stateStr += " (" + stateCode + ")";
        }
        firm["state"] = stateStr;
        firm["pan"] = r.value("pan_no").toString().trimmed();
        firm["bank_name"] = r.value("bank_name").toString().trimmed();
        QString bAc = r.value("bank_account").toString().trimmed();
        bAc.remove(QRegularExpression("^(A/c\\s*No[:\\.-]*|Account\\s*No[:\\.-]*)\\s*", QRegularExpression::CaseInsensitiveOption));
        firm["bank_ac"] = bAc.trimmed();

        QString bIfsc = r.value("ifsc_code").toString().trimmed();
        bIfsc.remove(QRegularExpression("^(IFSC[:\\.-]*|RTGS[:\\.-]*)\\s*", QRegularExpression::CaseInsensitiveOption));
        firm["bank_ifsc"] = bIfsc.trimmed();

        firm["fssai"] = r.value("fssai_no").toString().trimmed();
        firm["bank_branch"] = r.value("bank_branch", "Main Branch").toString().trimmed();
    }

    // Ensure standard keys exist even if company_info record is empty
    if (!firm.contains("firm_name")) firm["firm_name"] = "";
    if (!firm.contains("address")) firm["address"] = "";
    if (!firm.contains("city")) firm["city"] = "";
    if (!firm.contains("phone")) firm["phone"] = "";
    if (!firm.contains("email")) firm["email"] = "";
    if (!firm.contains("gstin")) firm["gstin"] = "";
    if (!firm.contains("state")) firm["state"] = "";
    if (!firm.contains("pan")) firm["pan"] = "";
    if (!firm.contains("bank_name")) firm["bank_name"] = "";
    if (!firm.contains("bank_ac")) firm["bank_ac"] = "";
    if (!firm.contains("bank_ifsc")) firm["bank_ifsc"] = "";
    if (!firm.contains("bank_branch")) firm["bank_branch"] = "";
    if (!firm.contains("fssai")) firm["fssai"] = "";

    return firm;
}

inline QVariantMap getPartyProfile(int partyId = 0, const QString& partyName = "") {
    QVariantMap profile;
    QVariantList rows;
    if (partyId > 0) {
        rows = DatabaseManager::instance().executeQuery("SELECT * FROM parties WHERE id = ? LIMIT 1;", {partyId});
    } else if (!partyName.isEmpty()) {
        rows = DatabaseManager::instance().executeQuery("SELECT * FROM parties WHERE name = ? COLLATE NOCASE LIMIT 1;", {partyName});
    }
    if (!rows.isEmpty()) {
        QVariantMap r = rows.first().toMap();
        profile["id"] = r.value("id");
        profile["name"] = r.value("name").toString().trimmed();
        profile["city"] = r.value("city").toString().trimmed();
        profile["address"] = r.value("address").toString().trimmed();
        profile["gstin"] = r.value("gstin").toString().trimmed();
        profile["pan"] = r.value("pan").toString().trimmed();
        profile["phone"] = r.value("phone").toString().trimmed();
        if (profile.value("phone").toString().isEmpty()) {
            profile["phone"] = r.value("mobile").toString().trimmed();
        }
        profile["state"] = r.value("state").toString().trimmed();
        profile["group_name"] = r.value("group_name").toString().trimmed();
    }
    return profile;
}

inline QString numberToWords(long long n) {
    if (n == 0) return "";
    static const QString ones[] = {
        "", "One", "Two", "Three", "Four", "Five", "Six", "Seven", "Eight", "Nine",
        "Ten", "Eleven", "Twelve", "Thirteen", "Fourteen", "Fifteen", "Sixteen",
        "Seventeen", "Eighteen", "Nineteen"
    };
    static const QString tens[] = {
        "", "", "Twenty", "Thirty", "Forty", "Fifty", "Sixty", "Seventy", "Eighty", "Ninety"
    };

    if (n < 20) return ones[n];
    if (n < 100) return tens[n / 10] + (n % 10 != 0 ? " " + ones[n % 10] : "");
    if (n < 100) return ones[n / 100] + " Hundred" + (n % 100 != 0 ? " " + numberToWords(n % 100) : "");
    if (n < 1000) return ones[n / 100] + " Hundred" + (n % 100 != 0 ? " " + numberToWords(n % 100) : "");
    if (n < 100000) return numberToWords(n / 1000) + " Thousand" + (n % 1000 != 0 ? " " + numberToWords(n % 1000) : "");
    if (n < 10000000) return numberToWords(n / 100000) + " Lakh" + (n % 100000 != 0 ? " " + numberToWords(n % 100000) : "");
    return numberToWords(n / 10000000) + " Crore" + (n % 10000000 != 0 ? " " + numberToWords(n % 10000000) : "");
}

inline QString amountInWords(double amount) {
    long long rupees = static_cast<long long>(std::floor(amount));
    int paise = static_cast<int>(std::round((amount - rupees) * 100.0));
    if (paise >= 100) {
        rupees += 1;
        paise = 0;
    }

    QString res = "INR ";
    if (rupees == 0) {
        res += "Zero";
    } else {
        res += numberToWords(rupees);
    }
    res += " Rupees";

    if (paise > 0) {
        res += " and " + numberToWords(paise) + " Paise";
    }
    res += " Only";
    return res;
}

} // namespace PrintReports
