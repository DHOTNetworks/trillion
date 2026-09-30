#include "ledger_pipeline.h"
#include "../database_manager.h"
#include "../models/account_classifier.h"
#include "data_preprocessor.h"
#include <QMutexLocker>
#include <cmath>
#include <QDebug>

LedgerPipeline::LedgerPipeline(QObject* parent)
    : QObject(parent)
{
}

LedgerPipeline& LedgerPipeline::instance() {
    static LedgerPipeline s_instance;
    return s_instance;
}

void LedgerPipeline::invalidateCache() {
    QMutexLocker locker(&m_mutex);
    m_byId.clear();
    m_byName.clear();
    m_byLegacyId.clear();
    m_valid = false;
    emit ledgersChanged();
}

void LedgerPipeline::ensureLoaded() {
    if (m_valid) return;

    m_byId.clear();
    m_byName.clear();
    m_byLegacyId.clear();

    QVariantList rows = DatabaseManager::instance().executeQuery(
        "SELECT p.id, p.legacy_id, p.name, COALESCE(p.mailing_name, p.name) AS print_name, p.alias, p.group_id, p.group_name, "
        "       p.opening_balance, p.balance_type, p.gstin, p.pan, p.mobile, "
        "       p.party_station, p.city, p.state, p.state_code, p.bank_name, p.bank_account, p.ifsc_code, "
        "       p.credit_limit, p.credit_days, p.interest_rate, p.apply_tcs, "
        "       g.nature AS grp_nature, g.extract_in_balance_sheet AS grp_extract_bs "
        "FROM parties p "
        "LEFT JOIN account_groups g ON p.group_id = g.id OR LOWER(p.group_name) = LOWER(g.name);"
    );

    for (const auto& rVar : rows) {
        QVariantMap r = rVar.toMap();
        LedgerNode node;
        node.id = r.value("id").toInt();
        node.legacyId = r.value("legacy_id").toInt();
        node.name = r.value("name").toString().trimmed();
        node.printName = r.value("print_name").toString().trimmed();
        node.alias = r.value("alias").toString().trimmed();
        node.groupId = r.value("group_id").toInt();
        node.groupName = r.value("group_name").toString().trimmed();
        node.nature = r.value("grp_nature").toString().trimmed();
        if (node.nature.isEmpty()) {
            node.nature = GroupHierarchyPipeline::instance().getEffectiveNatureByName(node.groupName);
        }
        node.extractInBalanceSheet = r.value("grp_extract_bs").toInt();
        node.openingBalance = r.value("opening_balance").toDouble();
        node.openingBalanceType = r.value("balance_type").toString().trimmed();
        node.gstin = r.value("gstin").toString().trimmed();
        node.pan = r.value("pan").toString().trimmed();
        node.mobile = r.value("mobile").toString().trimmed();
        node.station = r.value("party_station").toString().trimmed();
        node.city = r.value("city").toString().trimmed();
        node.state = r.value("state").toString().trimmed();
        node.stateCode = r.value("state_code").toString().trimmed();
        node.bankName = r.value("bank_name").toString().trimmed();
        node.bankAccount = r.value("bank_account").toString().trimmed();
        node.ifscCode = r.value("ifsc_code").toString().trimmed();
        node.creditLimit = r.value("credit_limit").toDouble();
        node.creditDays = r.value("credit_days").toInt();
        node.interestRate = r.value("interest_rate").toDouble();
        node.isTcsApplicable = (r.value("apply_tcs").toInt() == 1);

        m_byId.insert(node.id, node);
        m_byName.insert(node.name.toLower(), node);
        if (node.legacyId > 0) {
            m_byLegacyId.insert(node.legacyId, node);
        }
    }

    m_valid = true;
}

QVector<LedgerNode> LedgerPipeline::getAllLedgers() {
    QMutexLocker locker(&m_mutex);
    ensureLoaded();
    QVector<LedgerNode> res;
    res.reserve(m_byId.size());
    for (const auto& n : m_byId) {
        res.append(n);
    }
    return res;
}

LedgerNode LedgerPipeline::getLedgerById(int ledgerId) {
    QMutexLocker locker(&m_mutex);
    ensureLoaded();
    return m_byId.value(ledgerId);
}

LedgerNode LedgerPipeline::getLedgerByName(const QString& name) {
    QMutexLocker locker(&m_mutex);
    ensureLoaded();
    return m_byName.value(name.trimmed().toLower());
}

LedgerNode LedgerPipeline::getLedgerByLegacyId(int legacyId) {
    QMutexLocker locker(&m_mutex);
    ensureLoaded();
    return m_byLegacyId.value(legacyId);
}

QVector<LedgerPeriodBalance> LedgerPipeline::calculateBalancesForPeriod(
    const QString& fromDateIso,
    const QString& toDateIso,
    const QString& filterNature,
    int filterGroupId
) {
    QMutexLocker locker(&m_mutex);
    ensureLoaded();

    QVector<LedgerPeriodBalance> results;

    // 1. Fetch prior period transaction sums (before fromDateIso)
    struct AggBalance {
        double priorDr = 0.0;
        double priorCr = 0.0;
        double periodDr = 0.0;
        double periodCr = 0.0;
    };
    QHash<int, AggBalance> aggMap;

    if (!fromDateIso.isEmpty()) {
        QVariantList priorRows = DatabaseManager::instance().executeQuery(
            "SELECT party_id, dr_cr, SUM(amount) AS total_amt FROM transactions "
            "WHERE voucher_date < ? AND party_id IS NOT NULL AND party_id > 0 "
            "GROUP BY party_id, dr_cr;",
            { fromDateIso }
        );
        for (const auto& rVar : priorRows) {
            QVariantMap r = rVar.toMap();
            int pId = r.value("party_id").toInt();
            QString drCr = r.value("dr_cr").toString().trimmed();
            double amt = r.value("total_amt").toDouble();
            if (drCr.compare("Dr", Qt::CaseInsensitive) == 0) {
                aggMap[pId].priorDr += amt;
            } else {
                aggMap[pId].priorCr += amt;
            }
        }
    }

    // 2. Fetch current period transactions (fromDateIso <= voucher_date <= toDateIso)
    QString currentSql = "SELECT party_id, dr_cr, SUM(amount) AS total_amt FROM transactions WHERE 1=1 ";
    QVariantList currentParams;
    if (!fromDateIso.isEmpty()) {
        currentSql += " AND voucher_date >= ?";
        currentParams << fromDateIso;
    }
    if (!toDateIso.isEmpty()) {
        currentSql += " AND voucher_date <= ?";
        currentParams << toDateIso;
    }
    currentSql += " AND party_id IS NOT NULL AND party_id > 0 GROUP BY party_id, dr_cr;";

    QVariantList curRows = DatabaseManager::instance().executeQuery(currentSql, currentParams);
    for (const auto& rVar : curRows) {
        QVariantMap r = rVar.toMap();
        int pId = r.value("party_id").toInt();
        QString drCr = r.value("dr_cr").toString().trimmed();
        double amt = r.value("total_amt").toDouble();
        if (drCr.compare("Dr", Qt::CaseInsensitive) == 0) {
            aggMap[pId].periodDr += amt;
        } else {
            aggMap[pId].periodCr += amt;
        }
    }

    // 3. Resolve for each ledger in chart of accounts
    for (const auto& node : m_byId) {
        if (!filterNature.isEmpty() && node.nature.compare(filterNature, Qt::CaseInsensitive) != 0) {
            continue;
        }
        if (filterGroupId > 0 && node.groupId != filterGroupId) {
            continue;
        }

        LedgerPeriodBalance bal;
        bal.accountId = node.id;
        bal.legacyId = node.legacyId;
        bal.accountName = node.name;
        bal.groupId = node.groupId;
        bal.groupName = node.groupName;
        bal.nature = node.nature;
        bal.extractInBalanceSheet = node.extractInBalanceSheet;

        double initOpDr = (node.openingBalanceType.compare("Dr", Qt::CaseInsensitive) == 0) ? node.openingBalance : 0.0;
        double initOpCr = (node.openingBalanceType.compare("Cr", Qt::CaseInsensitive) == 0) ? node.openingBalance : 0.0;

        AggBalance ab = aggMap.value(node.id);

        bal.openingDr = initOpDr + ab.priorDr;
        bal.openingCr = initOpCr + ab.priorCr;
        bal.periodDr = ab.periodDr;
        bal.periodCr = ab.periodCr;
        bal.closingDr = bal.openingDr + bal.periodDr;
        bal.closingCr = bal.openingCr + bal.periodCr;

        bal.netBalance = bal.closingDr - bal.closingCr;
        bal.netDrCr = (bal.netBalance >= 0) ? "Dr" : "Cr";

        results.append(bal);
    }

    return results;
}

LedgerPeriodBalance LedgerPipeline::calculateSingleLedgerBalance(int ledgerId, const QString& asOnDateIso) {
    QVector<LedgerPeriodBalance> all = calculateBalancesForPeriod("", asOnDateIso);
    for (const auto& b : all) {
        if (b.accountId == ledgerId) return b;
    }
    return LedgerPeriodBalance();
}

LedgerPipeline::BalanceSheetPartition LedgerPipeline::partitionForBalanceSheet(const QString& asOnDateIso) {
    QVector<LedgerPeriodBalance> balances = calculateBalancesForPeriod("", asOnDateIso);
    BalanceSheetPartition partition;

    for (const auto& b : balances) {
        if (std::abs(b.netBalance) < 0.001) continue;

        if (b.extractInBalanceSheet == 0) {
            // Rollup under group name
            partition.groupRollups[b.groupName].append(b);
        } else if (b.extractInBalanceSheet == 2) {
            // Attached schedule
            partition.schedules[b.groupName].append(b);
        } else {
            // Itemized on face of Balance Sheet
            partition.itemizedLedgers[b.groupName].append(b);
        }
    }
    return partition;
}

QVariantList LedgerPipeline::searchLedgers(const QString& query, const QString& filterGroup, int limit) {
    QMutexLocker locker(&m_mutex);
    ensureLoaded();

    QString q = query.trimmed().toLower();
    QString fg = filterGroup.trimmed().toLower();
    QVariantList results;

    for (const auto& node : m_byId) {
        if (!fg.isEmpty() && node.groupName.toLower() != fg) {
            continue;
        }
        if (!q.isEmpty()) {
            bool matches = node.name.toLower().contains(q) ||
                            node.city.toLower().contains(q) ||
                            node.station.toLower().contains(q) ||
                            node.mobile.contains(q) ||
                            node.gstin.toLower().contains(q);
            if (!matches) continue;
        }

        QVariantMap m;
        m["id"] = node.id;
        m["legacy_id"] = node.legacyId;
        m["name"] = node.name;
        m["group_name"] = node.groupName;
        m["city"] = node.city;
        m["station"] = node.station;
        m["mobile"] = node.mobile;
        m["gstin"] = node.gstin;
        m["pan"] = node.pan;
        m["opening_balance"] = node.openingBalance;
        m["balance_type"] = node.openingBalanceType;
        m["credit_limit"] = node.creditLimit;
        m["credit_days"] = node.creditDays;
        results.append(m);

        if (limit > 0 && results.size() >= limit) break;
    }
    return results;
}

QStringList LedgerPipeline::getLedgerNames(const QString& filterGroup) {
    QMutexLocker locker(&m_mutex);
    ensureLoaded();

    QStringList names;
    QString fg = filterGroup.trimmed().toLower();
    for (const auto& node : m_byId) {
        if (fg.isEmpty() || node.groupName.toLower() == fg) {
            names << node.name;
        }
    }
    names.sort(Qt::CaseInsensitive);
    return names;
}

QVariantList LedgerPipeline::searchLedgers(const QString& query, int filterGroupId, int limit) {
    QMutexLocker locker(&m_mutex);
    ensureLoaded();

    QString q = query.trimmed().toLower();
    QVariantList results;
    QVector<int> validGroupIds;
    if (filterGroupId > 0) {
        validGroupIds = GroupHierarchyPipeline::instance().getSubtreeGroupIds(filterGroupId);
        validGroupIds.append(filterGroupId);
    }

    for (const auto& node : m_byId) {
        if (filterGroupId > 0 && !validGroupIds.contains(node.groupId)) {
            continue;
        }
        if (!q.isEmpty()) {
            bool matches = node.name.toLower().contains(q) ||
                            node.city.toLower().contains(q) ||
                            node.station.toLower().contains(q) ||
                            node.mobile.contains(q) ||
                            node.gstin.toLower().contains(q);
            if (!matches) continue;
        }

        QVariantMap m;
        m["id"] = node.id;
        m["legacy_id"] = node.legacyId;
        m["name"] = node.name;
        m["group_id"] = node.groupId;
        m["group_name"] = node.groupName;
        m["city"] = node.city;
        m["station"] = node.station;
        m["mobile"] = node.mobile;
        m["gstin"] = node.gstin;
        m["pan"] = node.pan;
        m["opening_balance"] = node.openingBalance;
        m["balance_type"] = node.openingBalanceType;
        m["credit_limit"] = node.creditLimit;
        m["credit_days"] = node.creditDays;
        results.append(m);

        if (limit > 0 && results.size() >= limit) break;
    }
    return results;
}

QVariantList LedgerPipeline::searchLedgersByRootCode(const QString& query, int rootCode, int limit) {
    AccountGroupNode grp = GroupHierarchyPipeline::instance().getGroupByCode(rootCode);
    if (grp.id > 0) {
        return searchLedgers(query, grp.id, limit);
    }
    return searchLedgers(query, 0, limit);
}

QStringList LedgerPipeline::getLedgerNames(int filterGroupId) {
    QMutexLocker locker(&m_mutex);
    ensureLoaded();

    QStringList names;
    QVector<int> validGroupIds;
    if (filterGroupId > 0) {
        validGroupIds = GroupHierarchyPipeline::instance().getSubtreeGroupIds(filterGroupId);
        validGroupIds.append(filterGroupId);
    }

    for (const auto& node : m_byId) {
        if (filterGroupId <= 0 || validGroupIds.contains(node.groupId)) {
            names << node.name;
        }
    }
    names.sort(Qt::CaseInsensitive);
    return names;
}

QStringList LedgerPipeline::getLedgerNamesByRootCode(int rootCode) {
    AccountGroupNode grp = GroupHierarchyPipeline::instance().getGroupByCode(rootCode);
    if (grp.id > 0) {
        return getLedgerNames(grp.id);
    }
    return getLedgerNames(0);
}

bool LedgerPipeline::saveLedger(const LedgerNode& node, QString* outError) {
    QString trimmedName = node.name.trimmed();
    if (trimmedName.isEmpty()) {
        if (outError) *outError = "Ledger name cannot be empty.";
        return false;
    }

    QString gName = node.groupName.trimmed();
    qint64 gid = node.groupId;
    int gCode = 0;

    if (gid > 0) {
        GroupHierarchyInfo gInfo = AccountClassifier::getGroupInfoById(gid);
        if (gInfo.id > 0) {
            if (gName.isEmpty()) gName = gInfo.name;
            gCode = gInfo.code1;
        }
    } else if (!gName.isEmpty()) {
        GroupHierarchyInfo gInfo = AccountClassifier::getGroupInfo(gName);
        if (gInfo.id > 0) {
            gid = gInfo.id;
            gCode = gInfo.code1;
        } else {
            gid = AccountClassifier::ensureGroupExists(gName);
            GroupHierarchyInfo gInfo2 = AccountClassifier::getGroupInfoById(gid);
            gCode = gInfo2.code1;
        }
    }

    if (gCode == 0) {
        gCode = 8; // Sundry Debtors default
    }

    QString partyType = AccountClassifier::classifyPartyTypeForGroup(gName);

    bool success = false;
    if (node.id > 0) {
        success = DatabaseManager::instance().executeNonQuery(
            "UPDATE parties SET name = ?, mailing_name = ?, alias = ?, group_id = ?, group_name = ?, group_code = ?, party_type = ?, "
            "opening_balance = ?, balance_type = ?, gstin = ?, pan = ?, mobile = ?, "
            "party_station = ?, city = ?, state = ?, state_code = ?, bank_name = ?, bank_account = ?, ifsc_code = ?, "
            "credit_limit = ?, credit_days = ?, interest_rate = ?, apply_tcs = ? WHERE id = ?;",
            { trimmedName, node.printName, node.alias, gid, gName, gCode, partyType,
              node.openingBalance, node.openingBalanceType, node.gstin, node.pan, node.mobile,
              node.station, node.city, node.state, node.stateCode, node.bankName, node.bankAccount, node.ifscCode,
              node.creditLimit, node.creditDays, node.interestRate, node.isTcsApplicable ? 1 : 0, node.id }
        );
    } else {
        success = DatabaseManager::instance().executeNonQuery(
            "INSERT INTO parties (name, mailing_name, alias, group_id, group_name, group_code, party_type, opening_balance, balance_type, "
            "gstin, pan, mobile, party_station, city, state, state_code, bank_name, bank_account, ifsc_code, "
            "credit_limit, credit_days, interest_rate, apply_tcs) "
            "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);",
            { trimmedName, node.printName, node.alias, gid, gName, gCode, partyType,
              node.openingBalance, node.openingBalanceType, node.gstin, node.pan, node.mobile,
              node.station, node.city, node.state, node.stateCode, node.bankName, node.bankAccount, node.ifscCode,
              node.creditLimit, node.creditDays, node.interestRate, node.isTcsApplicable ? 1 : 0 }
        );
    }

    if (success) {
        invalidateCache();
    } else if (outError) {
        *outError = "Database error while saving ledger.";
    }
    return success;
}

bool LedgerPipeline::deleteLedger(int ledgerId, QString* outError) {
    if (ledgerId <= 0) return false;

    // Check if transactions exist
    QVariant txCount = DatabaseManager::instance().executeScalar(
        "SELECT COUNT(*) FROM transactions WHERE party_id = ?;", { ledgerId }
    );
    if (txCount.toInt() > 0) {
        if (outError) *outError = "Cannot delete ledger with existing transactions.";
        return false;
    }

    bool success = DatabaseManager::instance().executeNonQuery(
        "DELETE FROM parties WHERE id = ?;", { ledgerId }
    );
    if (success) {
        invalidateCache();
    } else if (outError) {
        *outError = "Database error while deleting ledger.";
    }
    return success;
}
