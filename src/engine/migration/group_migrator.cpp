#include "group_migrator.h"
#include "../database_manager.h"
#include "../models/account_classifier.h"
#include "migration_utils.h"
#include <QDebug>
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

QString GroupMigrator::resolveStandardGroup(const QString& rawGroupName, QString& outPrimaryGroup, QString& outNature) {
    QString qName = rawGroupName.trimmed().toLower();
    outPrimaryGroup = "Primary";
    outNature = "Assets";

    if (qName == "capital account" || qName == "capital accounts" || qName == "reserves & surplus") {
        outPrimaryGroup = "Capital Account";
        outNature = "Liabilities";
        return "Capital Account";
    }
    if (qName == "current assets") {
        outPrimaryGroup = "Current Assets";
        outNature = "Assets";
        return "Current Assets";
    }
    if (qName.contains("bank")) {
        outPrimaryGroup = "Current Assets";
        outNature = "Assets";
        return "Bank Accounts";
    }
    if (qName.contains("cash")) {
        outPrimaryGroup = "Current Assets";
        outNature = "Assets";
        return "Cash-in-hand";
    }
    if (qName.contains("debtor") || qName.contains("customer")) {
        outPrimaryGroup = "Current Assets";
        outNature = "Assets";
        return "Sundry Debtors";
    }
    if (qName.contains("creditor") || qName.contains("supplier") || qName.contains("vendor")) {
        outPrimaryGroup = "Current Liabilities";
        outNature = "Liabilities";
        return "Sundry Creditors";
    }
    if (qName.contains("duty") || qName.contains("tax") || qName.contains("gst")) {
        outPrimaryGroup = "Current Liabilities";
        outNature = "Liabilities";
        return "Duties & Taxes";
    }
    if (qName.contains("sale")) {
        outPrimaryGroup = "Sales Accounts";
        outNature = "Income";
        return "Sales Accounts";
    }
    if (qName.contains("purchase")) {
        outPrimaryGroup = "Purchase Accounts";
        outNature = "Expense";
        return "Purchase Accounts";
    }
    if (qName.contains("direct exp") || qName.contains("mandi exp") || qName.contains("labor") || qName.contains("labour") || qName.contains("freight")) {
        outPrimaryGroup = "Direct Expenses";
        outNature = "Expense";
        return "Direct Expenses";
    }
    if (qName.contains("expense")) {
        outPrimaryGroup = "Indirect Expenses";
        outNature = "Expense";
        return "Indirect Expenses";
    }
    if (qName.contains("income")) {
        outPrimaryGroup = "Indirect Incomes";
        outNature = "Income";
        return "Indirect Incomes";
    }
    if (qName.contains("fixed asset") || qName.contains("machinery") || qName.contains("building") || qName.contains("land")) {
        outPrimaryGroup = "Fixed Assets";
        outNature = "Assets";
        return "Fixed Assets";
    }
    if (qName.contains("loan") || qName.contains("borrowing")) {
        outPrimaryGroup = "Loans (Liability)";
        outNature = "Liabilities";
        return "Loans (Liability)";
    }

    return rawGroupName.trimmed();
}

bool GroupMigrator::migrate_groups_bahikhata(
    MigrationContext& ctx,
    const std::vector<std::map<std::string, std::string>>& groupRows,
    std::map<int, std::string>& outGroupCodeMap,
    std::map<int, qint64>& outGroupCodeToIdMap
) {
    auto& db = DatabaseManager::instance();
    ctx.updateProgress(22, QString("Migrating %1 Account Groups from Bahi-Khata...").arg(groupRows.size()));

    outGroupCodeMap.clear();
    outGroupCodeToIdMap.clear();

    // First pass: collect all group codes and names
    for (const auto& g : groupRows) {
        std::string gName = MigrationUtils::cleanText(QString::fromStdString(getFieldStr(g, "GroupName"))).toStdString();
        if (gName.empty()) continue;
        int code1 = parseInteger(getFieldStr(g, "Code1st"));
        int code2 = parseInteger(getFieldStr(g, "Code2nd"));
        if (code2 == 0 && code1 > 35) {
            StandardGroupCode root = AccountClassifier::inferRootCodeFromName(QString::fromStdString(gName));
            int rootCode = static_cast<int>(root);
            if (rootCode > 0) {
                code2 = rootCode;
            }
        }
        outGroupCodeMap[code1] = gName;
    }

    // Second pass: insert with 4-level canonical hierarchy
    for (const auto& g : groupRows) {
        std::string gName = MigrationUtils::cleanText(QString::fromStdString(getFieldStr(g, "GroupName"))).toStdString();
        if (gName.empty()) continue;
        int code1 = parseInteger(getFieldStr(g, "Code1st"));
        int code2 = parseInteger(getFieldStr(g, "Code2nd"));
        int code3 = parseInteger(getFieldStr(g, "Code3rd"));
        int code4 = parseInteger(getFieldStr(g, "Code4th"));
        int extractBs = parseInteger(getFieldStr(g, "ExtractInBalanceSheet", "1"), 1);

        if (code2 == 0 && code1 > 35) {
            StandardGroupCode root = AccountClassifier::inferRootCodeFromName(QString::fromStdString(gName));
            int rootCode = static_cast<int>(root);
            if (rootCode > 0) {
                code2 = rootCode;
                code3 = AccountClassifier::getStandardGroupParent(rootCode);
            }
        }

        QString nature = AccountClassifier::getNatureForGroup(code1, code2, code3, code4);
        if (nature.isEmpty()) {
            nature = "Assets";
        }

        std::string parentName = "Primary";
        if (code2 > 0 && outGroupCodeMap.count(code2)) {
            parentName = outGroupCodeMap[code2];
        } else if (code2 > 0) {
            parentName = AccountClassifier::getStandardGroupName(code2).toStdString();
            if (parentName.empty()) parentName = "Primary";
        }

        int isSys = (code1 >= -1 && code1 <= 35) ? 1 : 0;

        db.executeNonQuery(
            "INSERT INTO account_groups (name, parent_group_name, nature, description, extract_in_balance_sheet, is_system, code1st, code2nd, code3rd, code4th) "
            "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?);",
            {
                QString::fromStdString(gName),
                QString::fromStdString(parentName),
                nature,
                QString("Legacy Group Code #%1").arg(code1),
                extractBs,
                isSys,
                code1,
                code2,
                code3,
                code4
            }
        );
        qint64 gid = db.lastInsertedId();
        outGroupCodeToIdMap[code1] = gid;
    }

    ctx.stats.totalGroups = static_cast<int>(outGroupCodeToIdMap.size());
    return true;
}

bool GroupMigrator::migrate_groups_busy(
    MigrationContext& ctx,
    const std::vector<std::map<std::string, std::string>>& groupRows,
    std::map<int, std::string>& outGroupCodeMap,
    std::map<int, qint64>& outGroupCodeToIdMap
) {
    auto& db = DatabaseManager::instance();
    ctx.updateProgress(30, QString("Migrating %1 Account Groups from Busy...").arg(groupRows.size()));

    outGroupCodeMap.clear();
    outGroupCodeToIdMap.clear();

    for (const auto& g : groupRows) {
        std::string name = MigrationUtils::cleanText(QString::fromStdString(getFieldStr(g, "Name"))).toStdString();
        if (name.empty()) continue;
        int code = parseInteger(getFieldStr(g, "Code"));
        outGroupCodeMap[code] = name;
    }

    for (const auto& g : groupRows) {
        std::string name = MigrationUtils::cleanText(QString::fromStdString(getFieldStr(g, "Name"))).toStdString();
        if (name.empty()) continue;
        int code = parseInteger(getFieldStr(g, "Code"));
        int parentCode = parseInteger(getFieldStr(g, "ParentGroupCode", getFieldStr(g, "ParentGroup", "0")));
        int primaryCode = parseInteger(getFieldStr(g, "PrimaryGroupCode", "0"));

        QString parentName = "Primary";
        if (parentCode > 0 && outGroupCodeMap.count(parentCode)) {
            parentName = QString::fromStdString(outGroupCodeMap[parentCode]);
        }

        QString qPrimary, qNature;
        resolveStandardGroup(QString::fromStdString(name), qPrimary, qNature);

        db.executeNonQuery(
            "INSERT INTO account_groups (name, parent_group_name, nature, description, extract_in_balance_sheet, is_system, code1st, code2nd, code3rd, code4th) "
            "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?);",
            {
                QString::fromStdString(name),
                parentName,
                qNature,
                QString("Busy Master2 Code #%1").arg(code),
                1,
                (code <= 35 ? 1 : 0),
                code,
                parentCode,
                primaryCode,
                0
            }
        );
        qint64 gid = db.lastInsertedId();
        outGroupCodeToIdMap[code] = gid;
    }

    ctx.stats.totalGroups = static_cast<int>(outGroupCodeToIdMap.size());
    return true;
}

bool GroupMigrator::migrate_groups_tally(
    MigrationContext& ctx,
    const QMap<QString, QString>& parentMap,
    const QList<QString>& groupNames,
    std::map<std::string, qint64>& outGroupNameToIdMap
) {
    auto& db = DatabaseManager::instance();
    ctx.updateProgress(55, QString("Writing %1 Account Groups from Tally...").arg(groupNames.size()));

    outGroupNameToIdMap.clear();

    for (const QString& gName : groupNames) {
        QString cleanGName = MigrationUtils::cleanText(gName);
        if (cleanGName.isEmpty()) continue;

        QString parent = parentMap.value(cleanGName, "Primary");
        QString qPrimary, qNature;
        resolveStandardGroup(cleanGName, qPrimary, qNature);

        db.executeNonQuery(
            "INSERT INTO account_groups (name, parent_group_name, nature, description, extract_in_balance_sheet, is_system, code1st, code2nd, code3rd, code4th) "
            "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?);",
            {
                cleanGName,
                parent,
                qNature,
                QString("Imported from Tally XML"),
                1,
                0,
                0,
                0,
                0,
                0
            }
        );
        qint64 gid = db.lastInsertedId();
        outGroupNameToIdMap[cleanGName.toStdString()] = gid;
    }

    ctx.stats.totalGroups = static_cast<int>(outGroupNameToIdMap.size());
    return true;
}

} // namespace MahadevERP
