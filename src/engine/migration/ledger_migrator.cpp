#include "ledger_migrator.h"
#include "../database_manager.h"
#include "../models/account_classifier.h"
#include "migration_utils.h"
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
    const std::map<int, std::string>& groupCodeMap,
    const std::map<int, qint64>& groupCodeToIdMap,
    std::map<int, std::string>& outLedgerCodeToName,
    std::map<int, qint64>& outLedgerCodeToId
) {
    auto& db = DatabaseManager::instance();
    ctx.updateProgress(30, QString("Migrating %1 Ledgers & Parties from Bahi-Khata...").arg(ledgerRows.size()));

    outLedgerCodeToName.clear();
    outLedgerCodeToId.clear();

    for (const auto& l : ledgerRows) {
        std::string lName = MigrationUtils::cleanText(QString::fromStdString(getFieldStr(l, "LedgerName"))).toStdString();
        if (lName.empty()) continue;

        int legacyId = parseInteger(getFieldStr(l, "Code1st"));
        std::string alias = MigrationUtils::cleanText(QString::fromStdString(getFieldStr(l, "LedgerAlais"))).toStdString();
        std::string prefix = MigrationUtils::cleanText(QString::fromStdString(getFieldStr(l, "Prefix", "M/s"))).toStdString();
        if (prefix.empty()) prefix = "M/s";

        int gCode = parseInteger(getFieldStr(l, "GroupCode"));
        std::string groupName = groupCodeMap.count(gCode) ? groupCodeMap.at(gCode) : "Sundry Debtors";

        double opBal = parseDoubleVal(getFieldStr(l, "OpeningBal"));
        std::string balType = MigrationUtils::cleanText(QString::fromStdString(getFieldStr(l, "OpeningType", "Dr"))).toStdString();
        if (balType.empty()) balType = "Dr";

        std::string rawMailing = MigrationUtils::cleanText(QString::fromStdString(getFieldStr(l, "MailingName"))).toStdString();
        std::string mailingName = rawMailing;
        static const std::regex datePattern(R"(^\d{1,2}[/-]\d{1,2}[/-]\d{2,4}$)");
        if (mailingName.empty() || mailingName == "None" || mailingName == "none" || std::regex_match(mailingName, datePattern)) {
            size_t bStart = lName.rfind('[');
            if (bStart != std::string::npos && bStart > 0) {
                mailingName = MigrationUtils::cleanText(QString::fromStdString(lName.substr(0, bStart))).toStdString();
            } else {
                mailingName = lName;
            }
        }
        std::string address = MigrationUtils::cleanText(QString::fromStdString(getFieldStr(l, "MailingAdd"))).toStdString();

        std::string partyStation = MigrationUtils::cleanText(QString::fromStdString(getFieldStr(l, "PartyStation"))).toStdString();
        std::string rawStation = MigrationUtils::cleanText(QString::fromStdString(getFieldStr(l, "Station"))).toStdString();
        std::string city = "";
        if (!partyStation.empty() && partyStation != "None" && partyStation != "none") {
            city = partyStation;
        } else if (!rawStation.empty() && rawStation != "None" && rawStation != "none") {
            city = rawStation;
        } else {
            size_t bStart = lName.rfind('[');
            size_t bEnd = lName.rfind(']');
            if (bStart != std::string::npos && bEnd != std::string::npos && bEnd > bStart + 1) {
                city = MigrationUtils::cleanText(QString::fromStdString(lName.substr(bStart + 1, bEnd - bStart - 1))).toStdString();
            }
        }
        city = toTitleCase(QString::fromStdString(city)).toStdString();

        std::string gstin = MigrationUtils::cleanText(QString::fromStdString(getFieldStr(l, "GSTIN"))).toStdString();
        std::string pan = MigrationUtils::cleanText(QString::fromStdString(getFieldStr(l, "IncomeTaxNo"))).toStdString();
        std::string tan = MigrationUtils::cleanText(QString::fromStdString(getFieldStr(l, "PartyTAN"))).toStdString();
        std::string aadhaar = MigrationUtils::cleanText(QString::fromStdString(getFieldStr(l, "AadharNo"))).toStdString();

        std::string partyState = MigrationUtils::cleanText(QString::fromStdString(getFieldStr(l, "PartyState"))).toStdString();
        std::string state = "";
        if (!partyState.empty() && partyState != "None" && partyState != "none" && 
            partyState != "STATE" && partyState != "INTER-STATE") {
            state = toTitleCase(QString::fromStdString(partyState)).toStdString();
        } else if (!gstin.empty() && gstin.length() >= 2 && isdigit(gstin[0]) && isdigit(gstin[1])) {
            QString sc = QString::fromStdString(gstin.substr(0, 2));
            state = MigrationUtils::extractStateCode(QString::fromStdString(gstin), "").toStdString();
        }
        if (state.empty()) {
            state = "Haryana";
        }
        std::string stateCode = MigrationUtils::extractStateCode(QString::fromStdString(gstin), QString::fromStdString(state)).toStdString();

        std::string pincode = MigrationUtils::cleanText(QString::fromStdString(getFieldStr(l, "PartyPINcode"))).toStdString();
        if (pincode == "None" || pincode == "none") pincode = "";
        if (pincode.empty() && !address.empty()) {
            pincode = MigrationUtils::extractPincode(QString::fromStdString(address)).toStdString();
        }

        std::string phone = MigrationUtils::cleanText(QString::fromStdString(getFieldStr(l, "Phone_O"))).toStdString();
        std::string mobile = MigrationUtils::cleanText(QString::fromStdString(getFieldStr(l, "Mobile1"))).toStdString();
        if (mobile.empty()) mobile = MigrationUtils::cleanText(QString::fromStdString(getFieldStr(l, "Mobile2"))).toStdString();
        std::string email = MigrationUtils::cleanText(QString::fromStdString(getFieldStr(l, "EMail"))).toStdString();
        std::string fssai = MigrationUtils::cleanText(QString::fromStdString(getFieldStr(l, "FSSAINo"))).toStdString();
        if (fssai == "None" || fssai == "none") fssai = "";

        QString bankName = QString::fromStdString(getFieldStr(l, "BankName"));
        QString bankAcc = QString::fromStdString(getFieldStr(l, "BankAcNo"));
        QString ifsc = QString::fromStdString(getFieldStr(l, "IFSCCode"));
        if (bankName.isEmpty() && bankAcc.isEmpty()) {
            MigrationUtils::cleanBankingDetails(
                QString::fromStdString(getFieldStr(l, "Mobile2")),
                QString::fromStdString(getFieldStr(l, "Bank2")),
                QString::fromStdString(getFieldStr(l, "Bank3")),
                bankName, bankAcc, ifsc
            );
        }

        double crLimit = parseDoubleVal(getFieldStr(l, "CreditLimit"));
        int crDays = parseInteger(getFieldStr(l, "CreditDays"));
        double interestRate = parseDoubleVal(getFieldStr(l, "InterestRate"));
        int tcsApp = parseInteger(getFieldStr(l, "TCSApplicable", "0"));
        std::string partyType = MigrationUtils::cleanText(QString::fromStdString(getFieldStr(l, "GSTPartyType"))).toStdString();
        std::string rawFirmType = MigrationUtils::cleanText(QString::fromStdString(getFieldStr(l, "FirmType"))).toStdString();
        QString firmType = MigrationUtils::resolveFirmTypeFromPan(QString::fromStdString(rawFirmType), QString::fromStdString(pan), QString::fromStdString(lName));

        qint64 groupId = 0;
        if (groupCodeToIdMap.count(gCode)) {
            groupId = groupCodeToIdMap.at(gCode);
        }

        QVariantList params;
        params << QString::fromStdString(lName)
               << QString::fromStdString(mailingName)
               << QString::fromStdString(alias)
               << QString::fromStdString(prefix)
               << QString::fromStdString(groupName)
               << static_cast<qlonglong>(groupId)
               << legacyId
               << opBal
               << QString::fromStdString(balType)
               << QString::fromStdString(gstin)
               << QString::fromStdString(pan)
               << QString::fromStdString(tan)
               << QString::fromStdString(aadhaar)
               << QString::fromStdString(fssai)
               << QString::fromStdString(address)
               << QString::fromStdString(city)
               << QString::fromStdString(state)
               << QString::fromStdString(stateCode)
               << QString::fromStdString(pincode)
               << QString::fromStdString(mobile)
               << QString::fromStdString(phone)
               << QString::fromStdString(email)
               << bankName
               << bankAcc
               << ifsc
               << crLimit
               << crDays
               << interestRate
               << tcsApp
               << QString::fromStdString(partyType)
               << firmType;

        db.executeNonQuery(
            "INSERT INTO parties ("
            "name, legal_name, alias, party_type, group_name, group_id, legacy_code, "
            "opening_balance, opening_balance_type, gstin, pan_no, tan_no, aadhar_no, "
            "fssai_no, address_line1, city, state, state_code, pincode, "
            "mobile, phone, email, bank_name, bank_account_no, bank_ifsc, "
            "credit_limit, credit_period_days, interest_rate_percent, tcs_applicable, "
            "gst_party_type, firm_type, is_active"
            ") VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, 1);",
            params
        );
        qint64 pid = db.lastInsertedId();
        outLedgerCodeToName[legacyId] = lName;
        outLedgerCodeToId[legacyId] = pid;
    }

    ctx.stats.totalAccounts = static_cast<int>(outLedgerCodeToId.size());
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
