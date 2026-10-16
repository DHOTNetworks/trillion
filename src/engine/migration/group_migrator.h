#pragma once

#include "migration_types.h"
#include <QMap>
#include <QString>
#include <map>
#include <string>
#include <vector>

namespace MahadevERP {

class GroupMigrator {
public:
    // Bahi-Khata Group Migration (C3 strangler slice G: field keys come from
    // the schema map via MigrationColumns, not literals — the JSON entry
    // Groups->account_groups is the reviewed source; the compiled fallback
    // is byte-identical and drift-checked by testGroupStranglerColumns).
    static bool migrate_groups_bahikhata(
        MigrationContext& ctx,
        const std::vector<std::map<std::string, std::string>>& groupRows,
        std::map<int, std::string>& outGroupCodeMap,
        std::map<int, qint64>& outGroupCodeToIdMap,
        std::map<int, std::vector<int>>* outLineage = nullptr
    );

    // Busy Group Migration
    static bool migrate_groups_busy(
        MigrationContext& ctx,
        const std::vector<std::map<std::string, std::string>>& groupRows,
        std::map<int, std::string>& outGroupCodeMap,
        std::map<int, qint64>& outGroupCodeToIdMap
    );

    // Tally Group Migration
    static bool migrate_groups_tally(
        MigrationContext& ctx,
        const QMap<QString, QString>& parentMap,
        const QList<QString>& groupNames,
        std::map<std::string, qint64>& outGroupNameToIdMap
    );

    // Canonical standard group resolver
    static QString resolveStandardGroup(const QString& rawGroupName, QString& outPrimaryGroup, QString& outNature);
};

} // namespace MahadevERP
