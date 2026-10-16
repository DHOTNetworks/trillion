#include "ledger_migrator.h"
#include "../database_manager.h"
#include "../models/account_classifier.h"
#include "../data_preprocessor.h"
#include "../jet4_reader.h"
#include "migration_utils.h"
#include "migration_columns.h"
#include <QDebug>
#include <QRegularExpression>
#include <regex>
#include <algorithm>

namespace MahadevERP {

static std::string getFieldStr(const std::map<std::string, std::string>& m, const std::string& key, const std::string& def = "") {
    auto it = m.find(key);
    if (it != m.end()) return it->second;
    for (const auto& kv : m) {
        if (QString::compare(QString::fromStdString(kv.first), QString::fromStdString(key), Qt::CaseInsensitive) == 0) {
            return kv.second;
        }
    }
    return def;
}

static double parseDoubleVal(const std::string& s, double def = 0.0) {
    std::string clean;
    for (char c : s) {
        if (c != ',' && c != '%' && c != ' ') clean += c;
    }
    if (clean.empty()) return def;
    try {
        return std::stod(clean);
    } catch (...) {
        return def;
    }
}

static int parseInteger(const std::string& s, int def = 0) {
    std::string clean;
    for (char c : s) {
        if (c != ',' && c != ' ') clean += c;
    }
    if (clean.empty()) return def;
    try {
        return std::stoi(clean);
    } catch (...) {
        return def;
    }
}

static QString toTitleCase(const QString& str) {
    if (str.trimmed().isEmpty()) return "";
    QStringList parts = str.split(' ', Qt::SkipEmptyParts);
    for (QString& p : parts) {
        if (!p.isEmpty()) {
            p = p.left(1).toUpper() + p.mid(1).toLower();
        }
    }
    return parts.join(" ");
}

bool LedgerMigrator::migrate_ledgers_bahikhata(
    MigrationContext& ctx,
    const std::vector<std::map<std::string, std::string>>& ledgerRows,
    const QString& globalBooksFrom,
    const std::map<int, std::string>& groupCodeMap,
    const std::map<int, std::vector<int>>& groupLineageMap,
    const std::map<int, qint64>& groupCodeToIdMap,
    MdbHandle* mdb,
    std::map<int, std::string>& outLedgerCodeMap,
    std::map<int, PartyDetail>& outLedgerDetailMap,
    std::map<int, int>& outLegacyIdToPartyId,
    std::map<int, std::string>& outDeletedNameMap
) {
    // C3 strangler slice L: transplanted verbatim from the retired monolith
    // PASS 2 (field keys via MigrationColumns = JSON-driven). Behavior
    // identical by construction + md5 parity; the monolith keeps a thin
    // wrapper. Helpers: DataPreprocessor mirrors (verbatim-identical bodies).
    auto& db = DatabaseManager::instance();
    auto LF = [](const char* t) { return MigrationColumns::ledgerField(QString::fromLatin1(t)).toStdString(); };
    ctx.updateProgress(30, QString("Migrating %1 Ledgers & Parties...").arg(ledgerRows.size()));
    db.executeNonQuery("DELETE FROM parties;");
    outLedgerCodeMap.clear();
    outLedgerDetailMap.clear();
    outLegacyIdToPartyId.clear();

    for (const auto& l : ledgerRows) {
        std::string lName = DataPreprocessor::cleanText(getFieldStr(l, LF("name")));
        if (lName.empty()) continue;

        int legacyId = DataPreprocessor::parseInt(getFieldStr(l, LF("legacy_id")));
        std::string alias = DataPreprocessor::cleanText(getFieldStr(l, LF("alias")));
        std::string prefix = DataPreprocessor::cleanText(getFieldStr(l, LF("prefix"), "M/s"));
        if (prefix.empty()) prefix = "M/s";

        int gCode = DataPreprocessor::parseInt(getFieldStr(l, LF("group_code")));
        std::string groupName = "Sundry Debtors";
        { auto git = groupCodeMap.find(gCode); if (git != groupCodeMap.end()) groupName = git->second; }

        double opBal = DataPreprocessor::parseDouble(getFieldStr(l, LF("opening_balance")));
        std::string balType = DataPreprocessor::cleanText(getFieldStr(l, LF("balance_type"), "Dr"));
        if (balType.empty()) balType = "Dr";

        std::string rawMailing = DataPreprocessor::cleanText(getFieldStr(l, LF("mailing_name")));
        std::string booksStartFrom = "";
        static const std::regex datePattern(R"(^\d{1,2}[/-]\d{1,2}[/-]\d{2,4}$)");
        if (std::regex_match(rawMailing, datePattern)) {
            booksStartFrom = rawMailing;
            std::replace(booksStartFrom.begin(), booksStartFrom.end(), '/', '-');
        }
        if (booksStartFrom.empty()) {
            booksStartFrom = globalBooksFrom.toStdString();
        }

        std::string mailingName = rawMailing;
        if (mailingName.empty() || mailingName == "None" || mailingName == "none" || std::regex_match(mailingName, datePattern)) {
            // Strip [Station] from lName for clean business mailing name
            size_t bStart = lName.rfind('[');
            if (bStart != std::string::npos && bStart > 0) {
                mailingName = DataPreprocessor::cleanText(lName.substr(0, bStart));
            } else {
                mailingName = lName;
            }
        }
        std::string address = DataPreprocessor::cleanText(getFieldStr(l, LF("address")));

        // 1. Station / City resolution
        std::string partyStation = DataPreprocessor::cleanText(getFieldStr(l, LF("party_station")));
        std::string rawStation = DataPreprocessor::cleanText(getFieldStr(l, LF("station")));
        std::string city = "";
        if (!partyStation.empty() && partyStation != "None" && partyStation != "none") {
            city = partyStation;
        } else if (!rawStation.empty() && rawStation != "None" && rawStation != "none") {
            city = rawStation;
        } else {
            // Check if lName ends with or contains [StationName]
            size_t bStart = lName.rfind('[');
            size_t bEnd = lName.rfind(']');
            if (bStart != std::string::npos && bEnd != std::string::npos && bEnd > bStart + 1) {
                city = DataPreprocessor::cleanText(lName.substr(bStart + 1, bEnd - bStart - 1));
            }
        }
        city = DataPreprocessor::toTitleCase(QString::fromStdString(city)).toStdString();

        // 2. District (deprecated/omitted)
        std::string district = "";

        // 3. GSTIN, PAN, Aadhaar, TAN
        std::string gstin = DataPreprocessor::cleanText(getFieldStr(l, LF("gstin")));
        std::string pan = DataPreprocessor::cleanText(getFieldStr(l, LF("pan")));
        std::string tan = DataPreprocessor::cleanText(getFieldStr(l, LF("tan")));
        std::string aadhaar = DataPreprocessor::cleanText(getFieldStr(l, LF("aadhaar")));

        // 4. State & State Code
        std::string partyState = DataPreprocessor::cleanText(getFieldStr(l, LF("party_state")));
        std::string state = "";
        if (!partyState.empty() && partyState != "None" && partyState != "none" && 
            partyState != "STATE" && partyState != "INTER-STATE") {
            state = DataPreprocessor::toTitleCase(QString::fromStdString(partyState)).toStdString();
        } else if (!gstin.empty() && gstin.length() >= 2 && isdigit(gstin[0]) && isdigit(gstin[1])) {
            QString sc = QString::fromStdString(gstin.substr(0, 2));
            state = DataPreprocessor::getStateNameFromCode(sc).toStdString();
        }
        if (state.empty()) {
            std::string rawState = DataPreprocessor::cleanText(getFieldStr(l, LF("state_raw")));
            if (rawState == "INTER-STATE") {
                if (city == "Delhi") state = "Delhi";
                else if (city == "Ludhiana" || city == "Ludhiyana" || city == "Moga" || city == "Jalandhar") state = "Punjab";
                else if (city == "Jaipur" || city == "Alwar" || city == "Kota") state = "Rajasthan";
                else state = "Haryana";
            } else {
                state = "Haryana";
            }
        }
        std::string stateCode = DataPreprocessor::extractStateCode(QString::fromStdString(gstin), QString::fromStdString(state)).toStdString();

        // 5. PIN Code & Route
        std::string pincode = DataPreprocessor::cleanText(getFieldStr(l, LF("pincode")));
        if (pincode == "None" || pincode == "none") pincode = "";
        if (pincode.empty() && !address.empty()) {
            pincode = DataPreprocessor::extractPincode(QString::fromStdString(address)).toStdString();
        }
        std::string route = DataPreprocessor::cleanText(getFieldStr(l, LF("route")));
        if (route == "None" || route == "none") route = "";
        else route = DataPreprocessor::toTitleCase(QString::fromStdString(route)).toStdString();

        std::string phone = DataPreprocessor::cleanText(getFieldStr(l, LF("phone")));
        std::string mobile = DataPreprocessor::cleanText(getFieldStr(l, LF("mobile")));
        std::string saleTaxNo = DataPreprocessor::cleanText(getFieldStr(l, LF("tin")));
        if (mobile.empty() && !saleTaxNo.empty() && saleTaxNo.find_first_of("0123456789") != std::string::npos && saleTaxNo.length() >= 8) {
            mobile = saleTaxNo;
        }
        if (mobile.empty()) mobile = phone;
        std::string whatsapp = DataPreprocessor::cleanText(getFieldStr(l, LF("whatsapp")));
        if (whatsapp.empty()) whatsapp = mobile;
        std::string email = DataPreprocessor::cleanText(getFieldStr(l, LF("email")));
        std::string contactPerson = DataPreprocessor::cleanText(getFieldStr(l, LF("contact_person")));
        if (contactPerson.empty() || contactPerson == "None") {
            // Strip [Station] from lName for clean contact person
            size_t bStart = lName.rfind('[');
            if (bStart != std::string::npos && bStart > 0) {
                contactPerson = DataPreprocessor::cleanText(lName.substr(0, bStart));
            } else {
                contactPerson = lName;
            }
        }

        std::string gstPartyType = DataPreprocessor::cleanText(getFieldStr(l, LF("gst_party_type")));
        if (gstPartyType.empty()) gstPartyType = gstin.empty() ? "Unregistered" : "Registered";

        std::string bankName = DataPreprocessor::cleanText(getFieldStr(l, LF("bank_name")));
        std::string bankAccount = DataPreprocessor::cleanText(getFieldStr(l, LF("bank_account")));
        std::string ifscCode = DataPreprocessor::cleanText(getFieldStr(l, LF("ifsc_code")));

        double creditLimit = DataPreprocessor::parseDouble(getFieldStr(l, LF("credit_limit")));
        double interestRate = DataPreprocessor::parseDouble(getFieldStr(l, LF("interest_rate")));
        double commissionRate = DataPreprocessor::parseDouble(getFieldStr(l, LF("commission_rate")));
        double salaryPerMonth = DataPreprocessor::parseDouble(getFieldStr(l, LF("salary_per_month")));
        std::string commissionOn = DataPreprocessor::cleanText(getFieldStr(l, LF("commission_on")));

        std::string applyTcsStr = DataPreprocessor::toLowerStr(getFieldStr(l, LF("apply_tcs")));
        int applyTcs = (applyTcsStr == "true" || applyTcsStr == "1" || applyTcsStr == "yes") ? 1 : 0;
        std::string tcsExemptStr = DataPreprocessor::toLowerStr(getFieldStr(l, LF("tcs_exempt")));
        int tcsExempt = (tcsExemptStr == "true" || tcsExemptStr == "1" || tcsExemptStr == "yes") ? 1 : 0;

        std::string partyType = DataPreprocessor::cleanText(getFieldStr(l, LF("party_type")));
        if (partyType.empty()) {
            if (groupLineageMap.count(gCode)) {
                const auto& lin = groupLineageMap.find(gCode)->second;
                partyType = AccountClassifier::classifyPartyType(lin[0], lin[1], lin[2], lin[3]).toStdString();
            } else {
                partyType = "General";
            }
        }
        std::string specialType = DataPreprocessor::cleanText(getFieldStr(l, LF("special_type")));
        if (specialType.empty()) {
            specialType = (partyType == "Buyer") ? "Rice Buyer" : ((partyType == "Vendor") ? "Paddy Seller" : partyType);
        }

        std::string shopNo = DataPreprocessor::cleanText(getFieldStr(l, LF("shop_no")));
        std::string tin = DataPreprocessor::cleanText(getFieldStr(l, LF("tin")));
        if (tin.empty()) tin = DataPreprocessor::cleanText(getFieldStr(l, LF("tin_alt")));
        std::string urn = DataPreprocessor::cleanText(getFieldStr(l, LF("urn")));
        if (urn.empty()) urn = DataPreprocessor::cleanText(getFieldStr(l, LF("urn_alt")));

        std::string sncStr = DataPreprocessor::toLowerStr(getFieldStr(l, LF("stock_not_calc")));
        int stockNotCalc = (sncStr == "1" || sncStr == "true" || sncStr == "yes") ? 1 : 0;
        std::string sdtStr = DataPreprocessor::toLowerStr(getFieldStr(l, LF("show_date_totals")));
        int showDateTotals = (sdtStr == "1" || sdtStr == "true" || sdtStr == "yes") ? 1 : 0;
        std::string cjtStr = DataPreprocessor::toLowerStr(getFieldStr(l, LF("calc_direct_expense")));
        int calcDirectExpense = (cjtStr == "1" || cjtStr == "true" || cjtStr == "yes") ? 1 : 0;
        int useCreditLimit = (creditLimit > 0.0) ? 1 : 0;
        qint64 targetGroupId = 0;
        { auto tit = groupCodeToIdMap.find(gCode); if (tit != groupCodeToIdMap.end()) targetGroupId = tit->second; }

        db.executeNonQuery(
            "INSERT INTO parties ("
            "name, alias, prefix, group_name, group_code, group_id, party_type, special_type, "
            "opening_balance, balance_type, mailing_name, address, city, district, state, state_code, pincode, route, "
            "mobile, whatsapp, phone, email, contact_person, pan, aadhaar, tan, gstin, gst_party_type, "
            "bank_name, bank_account, ifsc_code, credit_limit, credit_days, interest_rate, commission_rate, commission_on, "
            "apply_tcs, tcs_exempt, party_station, use_routes, shop_no, tin, urn, stock_not_calc, use_credit_limit, "
            "show_date_totals, calc_direct_expense, set_title_case, books_start_from, salary_per_month, legacy_id) "
            "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, 30, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, 1, ?, ?, ?);",
            {
                QString::fromStdString(lName),
                QString::fromStdString(alias),
                QString::fromStdString(prefix),
                QString::fromStdString(groupName),
                gCode,
                targetGroupId,
                QString::fromStdString(partyType),
                QString::fromStdString(specialType),
                opBal,
                QString::fromStdString(balType),
                QString::fromStdString(mailingName),
                QString::fromStdString(address),
                QString::fromStdString(city),
                QString::fromStdString(district),
                QString::fromStdString(state),
                QString::fromStdString(stateCode),
                QString::fromStdString(pincode),
                QString::fromStdString(route),
                QString::fromStdString(mobile),
                QString::fromStdString(whatsapp),
                QString::fromStdString(phone),
                QString::fromStdString(email),
                QString::fromStdString(contactPerson),
                QString::fromStdString(pan),
                QString::fromStdString(aadhaar),
                QString::fromStdString(tan),
                QString::fromStdString(gstin),
                QString::fromStdString(gstPartyType),
                QString::fromStdString(bankName),
                QString::fromStdString(bankAccount),
                QString::fromStdString(ifscCode),
                creditLimit,
                interestRate,
                commissionRate,
                QString::fromStdString(commissionOn),
                applyTcs,
                tcsExempt,
                QString::fromStdString(city),
                (route.empty() || route == "None") ? 0 : 1,
                QString::fromStdString(shopNo),
                QString::fromStdString(tin),
                QString::fromStdString(urn),
                stockNotCalc,
                useCreditLimit,
                showDateTotals,
                calcDirectExpense,
                QString::fromStdString(booksStartFrom),
                salaryPerMonth,
                legacyId
            }
        );

        int partyDbId = static_cast<int>(db.lastInsertedId());
        outLegacyIdToPartyId[legacyId] = partyDbId;
        outLedgerCodeMap[legacyId] = lName;
        outLedgerDetailMap[legacyId] = {partyDbId, lName, groupName, partyType, legacyId, gstin, address, city, state, phone};
    }

    // Deleted-ledger display fallback: vouchers may reference ledgers that
    // were later deleted in Bahi-Khata (their slots stay on disk until the
    // file is compacted). Collect their names for display only — never for
    // party creation, balances, or counts. Codes present in the live map win.
    outDeletedNameMap.clear();
    {
        auto delRows = Jet4Reader::readDeletedRows(mdb, "Ledgers");
        for (const auto& d : delRows) {
            int dCode = DataPreprocessor::parseInt(getFieldStr(d, LF("legacy_id")));
            if (dCode <= 0 || outLedgerDetailMap.count(dCode) || outDeletedNameMap.count(dCode)) continue;
            std::string dName = DataPreprocessor::cleanText(getFieldStr(d, LF("name")));
            if (!dName.empty()) outDeletedNameMap[dCode] = dName;
        }
    }

    // Auto-heal any uncoded or missing groups from imported data
    AccountClassifier::healAllGroups();
    return true;
}

bool LedgerMigrator::migrate_ledgers_busy(
    MigrationContext& ctx,
    const std::vector<std::map<std::string, std::string>>& accountRows,
    const std::map<int, std::string>& groupCodeMap,
    const std::map<int, qint64>& groupCodeToIdMap,
    std::map<int, std::string>& outAccountCodeToName,
    std::map<int, qint64>& outAccountCodeToId
) {
    auto& db = DatabaseManager::instance();
    ctx.updateProgress(50, QString("Migrating %1 Parties & Accounts from Busy...").arg(accountRows.size()));

    outAccountCodeToName.clear();
    outAccountCodeToId.clear();

    for (const auto& a : accountRows) {
        std::string name = MigrationUtils::cleanText(QString::fromStdString(getFieldStr(a, "Name"))).toStdString();
        if (name.empty()) continue;

        int code = parseInteger(getFieldStr(a, "Code"));
        std::string printName = MigrationUtils::cleanText(QString::fromStdString(getFieldStr(a, "PrintName", name))).toStdString();
        int parentCode = parseInteger(getFieldStr(a, "ParentGroupCode", getFieldStr(a, "ParentGroup", "0")));
        std::string groupName = groupCodeMap.count(parentCode) ? groupCodeMap.at(parentCode) : "Sundry Debtors";

        double opBal = parseDoubleVal(getFieldStr(a, "OpBal", getFieldStr(a, "OpeningBal", "0")));
        std::string balType = MigrationUtils::cleanText(QString::fromStdString(getFieldStr(a, "OpBalType", getFieldStr(a, "OpeningType", "Dr")))).toStdString();
        if (balType.empty()) balType = (opBal >= 0 ? "Dr" : "Cr");
        opBal = std::abs(opBal);

        std::string address1 = MigrationUtils::cleanText(QString::fromStdString(getFieldStr(a, "Address1"))).toStdString();
        std::string address2 = MigrationUtils::cleanText(QString::fromStdString(getFieldStr(a, "Address2"))).toStdString();
        std::string fullAddr = address1;
        if (!address2.empty()) fullAddr += ", " + address2;

        std::string city = MigrationUtils::cleanText(QString::fromStdString(getFieldStr(a, "City"))).toStdString();
        std::string state = MigrationUtils::cleanText(QString::fromStdString(getFieldStr(a, "State", "Haryana"))).toStdString();
        std::string stateCode = MigrationUtils::cleanText(QString::fromStdString(getFieldStr(a, "StateCode"))).toStdString();
        std::string pincode = MigrationUtils::cleanText(QString::fromStdString(getFieldStr(a, "PinCode", getFieldStr(a, "Pincode")))).toStdString();
        std::string gstin = MigrationUtils::cleanText(QString::fromStdString(getFieldStr(a, "GSTIN", getFieldStr(a, "GSTNo")))).toStdString();
        std::string pan = MigrationUtils::cleanText(QString::fromStdString(getFieldStr(a, "PAN", getFieldStr(a, "ITPan")))).toStdString();
        if (pan.empty() && gstin.length() == 15) {
            pan = gstin.substr(2, 10);
        }
        if (stateCode.empty()) {
            stateCode = MigrationUtils::extractStateCode(QString::fromStdString(gstin), QString::fromStdString(state)).toStdString();
        }

        std::string mobile = MigrationUtils::cleanText(QString::fromStdString(getFieldStr(a, "Mobile", getFieldStr(a, "MobileNo")))).toStdString();
        std::string phone = MigrationUtils::cleanText(QString::fromStdString(getFieldStr(a, "Phone", getFieldStr(a, "PhoneNo")))).toStdString();
        std::string email = MigrationUtils::cleanText(QString::fromStdString(getFieldStr(a, "Email"))).toStdString();

        std::string bankName = MigrationUtils::cleanText(QString::fromStdString(getFieldStr(a, "BankName"))).toStdString();
        std::string bankAcc = MigrationUtils::cleanText(QString::fromStdString(getFieldStr(a, "BankAcc", getFieldStr(a, "BankAccNo")))).toStdString();
        std::string ifsc = MigrationUtils::cleanText(QString::fromStdString(getFieldStr(a, "IFSC", getFieldStr(a, "IFSCCode")))).toStdString();

        QString firmType = MigrationUtils::resolveFirmTypeFromPan("", QString::fromStdString(pan), QString::fromStdString(name));
        qint64 groupId = groupCodeToIdMap.count(parentCode) ? groupCodeToIdMap.at(parentCode) : 0;

        db.executeNonQuery(
            "INSERT INTO parties ("
            "name, legal_name, alias, party_type, group_name, group_id, legacy_code, "
            "opening_balance, opening_balance_type, gstin, pan_no, "
            "address_line1, city, state, state_code, pincode, "
            "mobile, phone, email, bank_name, bank_account_no, bank_ifsc, "
            "firm_type, is_active"
            ") VALUES (?, ?, ?, 'M/s', ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, 1);",
            {
                QString::fromStdString(name),
                QString::fromStdString(printName),
                QString::fromStdString(printName),
                QString::fromStdString(groupName),
                groupId,
                code,
                opBal,
                QString::fromStdString(balType),
                QString::fromStdString(gstin),
                QString::fromStdString(pan),
                QString::fromStdString(fullAddr),
                QString::fromStdString(city),
                QString::fromStdString(state),
                QString::fromStdString(stateCode),
                QString::fromStdString(pincode),
                QString::fromStdString(mobile),
                QString::fromStdString(phone),
                QString::fromStdString(email),
                QString::fromStdString(bankName),
                QString::fromStdString(bankAcc),
                QString::fromStdString(ifsc),
                firmType
            }
        );
        qint64 pid = db.lastInsertedId();
        outAccountCodeToName[code] = name;
        outAccountCodeToId[code] = pid;
    }

    ctx.stats.totalAccounts = static_cast<int>(outAccountCodeToId.size());
    return true;
}

bool LedgerMigrator::migrate_ledgers_tally(
    MigrationContext& ctx,
    const QList<QVariantMap>& ledgerRecords,
    const std::map<std::string, qint64>& groupNameToIdMap,
    std::map<std::string, qint64>& outLedgerNameToId
) {
    auto& db = DatabaseManager::instance();
    ctx.updateProgress(70, QString("Writing %1 Master Ledgers from Tally...").arg(ledgerRecords.size()));

    outLedgerNameToId.clear();

    for (const auto& l : ledgerRecords) {
        QString name = MigrationUtils::cleanText(l.value("name").toString());
        if (name.isEmpty()) continue;

        QString parent = MigrationUtils::cleanText(l.value("parent").toString());
        if (parent.isEmpty()) parent = "Sundry Debtors";

        double opBal = l.value("opening_balance", 0.0).toDouble();
        QString balType = "Dr";
        if (opBal < 0) {
            balType = "Cr";
            opBal = -opBal;
        }

        QString address = l.value("address").toString();
        QString state = l.value("state").toString();
        QString pincode = l.value("pincode").toString();
        QString gstin = l.value("gstin").toString();
        QString pan = l.value("pan").toString();
        if (pan.isEmpty() && gstin.length() == 15) {
            pan = gstin.mid(2, 10);
        }
        QString stateCode = MigrationUtils::extractStateCode(gstin, state);
        QString firmType = MigrationUtils::resolveFirmTypeFromPan("", pan, name);

        qint64 groupId = 0;
        if (groupNameToIdMap.count(parent.toStdString())) {
            groupId = groupNameToIdMap.at(parent.toStdString());
        }

        db.executeNonQuery(
            "INSERT INTO parties ("
            "name, legal_name, alias, party_type, group_name, group_id, legacy_code, "
            "opening_balance, opening_balance_type, gstin, pan_no, "
            "address_line1, state, state_code, pincode, "
            "firm_type, is_active"
            ") VALUES (?, ?, ?, 'M/s', ?, ?, 0, ?, ?, ?, ?, ?, ?, ?, ?, ?, 1);",
            {
                name,
                name,
                name,
                parent,
                groupId,
                opBal,
                balType,
                gstin,
                pan,
                address,
                state,
                stateCode,
                pincode,
                firmType
            }
        );
        qint64 pid = db.lastInsertedId();
        outLedgerNameToId[name.toStdString()] = pid;
    }

    ctx.stats.totalAccounts = static_cast<int>(outLedgerNameToId.size());
    return true;
}

} // namespace MahadevERP
