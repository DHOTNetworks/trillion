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
    // Bahi-Khata Stock Groups, Units, Items & Inventory
    static bool migrate_stocks_bahikhata(
        MigrationContext& ctx,
        const std::vector<std::map<std::string, std::string>>& stockGroupRows,
        const std::vector<std::map<std::string, std::string>>& stockUnitRows,
        const std::vector<std::map<std::string, std::string>>& itemRows,
        std::map<int, std::string>& outStockGroupMap,
        std::map<int, std::string>& outStockUnitMap,
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
