#pragma once

#include "migration_types.h"
#include <QMap>
#include <QString>
#include <map>
#include <string>
#include <vector>

namespace MahadevERP {

class MandiMigrator {
public:
    // Bahi-Khata Mandi, Milling, Bardana, Gate, Sauda & Brokerage Migration
    static bool migrate_mandi_bahikhata(
        MigrationContext& ctx,
        const std::vector<std::map<std::string, std::string>>& transRows,
        const std::vector<std::map<std::string, std::string>>& millingRows,
        const std::vector<std::map<std::string, std::string>>& customClosingRows,
        const std::vector<std::map<std::string, std::string>>& tdsRows,
        const std::vector<std::map<std::string, std::string>>& bardanaRows,
        const std::vector<std::map<std::string, std::string>>& gatePassRows,
        const std::vector<std::map<std::string, std::string>>& gateRegRows,
        const std::vector<std::map<std::string, std::string>>& saudaRows,
        const std::vector<std::map<std::string, std::string>>& saudaTxRows,
        const std::vector<std::map<std::string, std::string>>& brokerageRows,
        const std::map<int, std::string>& ledgerCodeToName,
        const std::map<int, qint64>& ledgerCodeToId,
        const std::map<int, std::string>& itemCodeToName,
        const std::map<int, qint64>& itemCodeToId,
        const std::map<int, std::string>& stockUnitMap,
        const QMap<QString, int>& fyNameToId
    );

    // Busy Mandi & Procurement Migration
    static bool migrate_mandi_busy(
        MigrationContext& ctx,
        const std::vector<std::map<std::string, std::string>>& mandiRows,
        const std::map<int, std::string>& accountCodeToName,
        const std::map<int, qint64>& accountCodeToId,
        const QMap<QString, int>& fyNameToId
    );
};

} // namespace MahadevERP
