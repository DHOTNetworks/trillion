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
    // Bahi-Khata stock-movement + logistics caching (C3 strangler slice V:
    // transplanted verbatim from the monolith PASS 4; field keys come from
    // the schema map via MigrationColumns where the JSON entries cover
    // them, documented JSON-gap literals elsewhere). Builds the
    // voucher-keyed caches the transaction pass consumes.
    static bool migrate_movements_bahikhata(
        MigrationContext& ctx,
        const std::vector<std::map<std::string, std::string>>& stockTransRows,
        const std::vector<std::map<std::string, std::string>>& transportRows,
        const std::map<int, ItemDetail>& itemCodeMap,
        std::map<std::string, std::vector<StockMovementEntry>>& outStockTransMap,
        std::map<std::string, LogisticsEntry>& outTransportMap
    );

    // Bahi-Khata double-entry transactions + invoice/voucher reconstruction
    // (C3 slice V: monolith PASS 5 sections 1-2, verbatim). Writes
    // transactions, vouchers, sales/purchase invoices + items, and the
    // paddy arrival derived from purchase items. Outs feed the monolith
    // summary + later mandi passes.
    static bool migrate_transactions_bahikhata(
        MigrationContext& ctx,
        const std::vector<std::map<std::string, std::string>>& transRows,
        const std::map<std::string, std::vector<StockMovementEntry>>& stockTransMap,
        const std::map<std::string, LogisticsEntry>& transportMap,
        const std::map<int, std::string>& ledgerCodeMap,
        const std::map<int, PartyDetail>& ledgerDetailMap,
        const std::map<int, std::string>& ledgerDeletedNameMap,
        std::map<QString, int>& fyNameToId,
        std::map<std::string, std::vector<std::map<std::string, std::string>>>& outVoucherGroups,
        int& outSalesInvoiceCount,
        int& outPurchaseInvoiceCount
    );

    // Bahi-Khata inventory ledger (C3 slice V: monolith PASS 5.3,
    // verbatim). Builds its own txLookup from EntryType=="Ledger" rows.
    static bool migrate_stock_ledger_bahikhata(
        MigrationContext& ctx,
        const std::vector<std::map<std::string, std::string>>& transRows,
        const std::vector<std::map<std::string, std::string>>& stockTransRows,
        const std::map<int, PartyDetail>& ledgerDetailMap,
        const std::map<int, ItemDetail>& itemCodeMap,
        std::map<QString, int>& fyNameToId
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
        std::map<QString, int>& fyNameToId
    );

    // Tally Double-Entry Vouchers & Invoices
    static bool migrate_vouchers_tally(
        MigrationContext& ctx,
        const QList<QVariantMap>& voucherRecords,
        const std::map<std::string, qint64>& ledgerNameToId,
        const std::map<std::string, qint64>& itemNameToId,
        std::map<QString, int>& fyNameToId
    );

    static QString mapVoucherTypeStr(const QString& rawType);
};

} // namespace MahadevERP
