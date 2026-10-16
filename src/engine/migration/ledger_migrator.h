#pragma once

#include "migration_types.h"
#include "../jet4_reader.h"
#include <QMap>
#include <QString>
#include <map>
#include <string>
#include <vector>

namespace MahadevERP {

class LedgerMigrator {
public:
    // Bahi-Khata Ledger & Party Migration (C3 strangler slice L: field keys
    // come from the schema map via MigrationColumns, not literals — the JSON
    // entry Ledgers->parties is the reviewed source; the compiled fallback
    // is byte-identical and drift-checked by testLedgerStranglerColumns).
    static bool migrate_ledgers_bahikhata(
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
    );

    // Busy Ledger & Party Migration
    static bool migrate_ledgers_busy(
        MigrationContext& ctx,
        const std::vector<std::map<std::string, std::string>>& accountRows,
        const std::map<int, std::string>& groupCodeMap,
        const std::map<int, qint64>& groupCodeToIdMap,
        std::map<int, std::string>& outAccountCodeToName,
        std::map<int, qint64>& outAccountCodeToId
    );

    // Tally Master Ledger Migration
    static bool migrate_ledgers_tally(
        MigrationContext& ctx,
        const QList<QVariantMap>& ledgerRecords,
        const std::map<std::string, qint64>& groupNameToIdMap,
        std::map<std::string, qint64>& outLedgerNameToId
    );
};

} // namespace MahadevERP
