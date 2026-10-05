#pragma once

#include "migration_types.h"
#include <QMap>
#include <QString>
#include <map>
#include <string>
#include <vector>

namespace MahadevERP {

class VoucherMigrator {
public:
    // Bahi-Khata Double-Entry Vouchers & Invoices
    static bool migrate_vouchers_bahikhata(
        MigrationContext& ctx,
        const std::vector<std::map<std::string, std::string>>& transRows,
        const std::vector<std::map<std::string, std::string>>& stockTransRows,
        const std::vector<std::map<std::string, std::string>>& transportRows,
        const std::map<int, std::string>& ledgerCodeToName,
        const std::map<int, qint64>& ledgerCodeToId,
        const std::map<int, std::string>& itemCodeToName,
        const std::map<int, qint64>& itemCodeToId,
        const std::map<int, std::string>& stockUnitMap,
        const QMap<QString, int>& fyNameToId
    );

    // Busy Double-Entry Vouchers & Invoices
    static bool migrate_vouchers_busy(
        MigrationContext& ctx,
        const std::vector<std::map<std::string, std::string>>& tran1Rows,
        const std::vector<std::map<std::string, std::string>>& tran2Rows,
        const std::vector<std::map<std::string, std::string>>& tran3Rows,
        const std::map<int, std::string>& accountCodeToName,
        const std::map<int, qint64>& accountCodeToId,
        const std::map<int, std::string>& itemCodeToName,
        const std::map<int, qint64>& itemCodeToId,
        const QMap<QString, int>& fyNameToId
    );

    // Tally Double-Entry Vouchers & Invoices
    static bool migrate_vouchers_tally(
        MigrationContext& ctx,
        const QList<QVariantMap>& voucherRecords,
        const std::map<std::string, qint64>& ledgerNameToId,
        const std::map<std::string, qint64>& itemNameToId,
        const QMap<QString, int>& fyNameToId
    );

    static QString mapVoucherTypeStr(const QString& rawType);
};

} // namespace MahadevERP
