#pragma once

#include "migration_types.h"
#include <QMap>
#include <QString>
#include <map>
#include <string>
#include <vector>

namespace MahadevERP {

class LedgerMigrator {
public:
    // Bahi-Khata Ledger & Party Migration
    static bool migrate_ledgers_bahikhata(
        MigrationContext& ctx,
        const std::vector<std::map<std::string, std::string>>& ledgerRows,
        const std::map<int, std::string>& groupCodeMap,
        const std::map<int, qint64>& groupCodeToIdMap,
        std::map<int, std::string>& outLedgerCodeToName,
        std::map<int, qint64>& outLedgerCodeToId
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
