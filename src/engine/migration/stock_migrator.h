#pragma once

#include "migration_types.h"
#include <QMap>
#include <QString>
#include <map>
#include <string>
#include <vector>

namespace MahadevERP {

class StockMigrator {
public:
    // Bahi-Khata Stock Groups & Units (C3 strangler slice S: field keys come
    // from the schema map via MigrationColumns — the JSON StockGroups/
    // StockUnits entries are the reviewed source; the compiled fallback is
    // byte-identical and drift-checked by testStockStranglerColumns).
    static bool migrate_stock_masters_bahikhata(
        MigrationContext& ctx,
        const std::vector<std::map<std::string, std::string>>& stockGroupRows,
        const std::vector<std::map<std::string, std::string>>& stockUnitRows,
        std::map<int, std::string>& outStockGroupMap,
        std::map<int, std::string>& outStockUnitMap
    );

    // Bahi-Khata Stock Items & Inventory (C3 strangler slice S: JSON
    // StockItems entry via MigrationColumns; fully data-driven — no
    // hardcoded gst/packing fallbacks).
    static bool migrate_stock_items_bahikhata(
        MigrationContext& ctx,
        const std::vector<std::map<std::string, std::string>>& itemRows,
        const std::map<int, std::string>& ledgerCodeMap,
        const std::map<int, int>& legacyIdToPartyId,
        const std::map<int, std::string>& inStockGroupMap,
        const std::map<int, std::string>& inStockUnitMap,
        std::map<int, ItemDetail>& outItemCodeMap,
        std::map<int, std::string>& outItemCodeToName,
        std::map<int, qint64>& outItemCodeToId
    );

    // Busy Stock Groups, Units, Items & Inventory
    static bool migrate_stocks_busy(
        MigrationContext& ctx,
        const std::vector<std::map<std::string, std::string>>& itemRows,
        const std::vector<std::map<std::string, std::string>>& unitRows,
        const std::vector<std::map<std::string, std::string>>& groupRows,
        std::map<int, std::string>& outItemCodeToName,
        std::map<int, qint64>& outItemCodeToId
    );

    // Tally Stock Groups, Units, Items & Inventory
    static bool migrate_stocks_tally(
        MigrationContext& ctx,
        const QList<QVariantMap>& unitRecords,
        const QList<QVariantMap>& itemRecords,
        std::map<std::string, qint64>& outItemNameToId
    );
};

} // namespace MahadevERP
