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
    // Bahi-Khata manufacturing batches + line items (C3 slice M1:
    // monolith PASS 5.2, verbatim). MillingVouchers-source keys via
    // millingField(); outGroupCount echoes the grouped-key count for the
    // monolith summary (value-identical: skips never increment either).
    static bool migrate_manufacturing_bahikhata(
        MigrationContext& ctx,
        const std::vector<std::map<std::string, std::string>>& manufacturingRows,
        const std::map<int, ItemDetail>& itemCodeMap,
        std::map<QString, int>& fyNameToId,
        size_t& outGroupCount
    );

    // Bahi-Khata J-Form procurement vouchers + items (C3 slice M1:
    // monolith PASS 3.5, verbatim). Form keys via jformField(), generic
    // Transactions keys via voucherField(); raw voucher numbers preserved
    // (no canonicalVoucherNo minting — JFrm keeps Bahi numbers).
    static bool migrate_jform_bahikhata(
        MigrationContext& ctx,
        const std::vector<std::map<std::string, std::string>>& transRows,
        const std::vector<std::map<std::string, std::string>>& stockTransRows,
        const std::map<int, PartyDetail>& ledgerDetailMap,
        const std::map<int, ItemDetail>& itemCodeMap,
        std::map<QString, int>& fyNameToId
    );

    // Bahi-Khata I-Form mandi issue vouchers + items (C3 slice M1:
    // monolith PASS 3.6, verbatim). Same key split as J-Form. Writes
    // iform_vouchers (NOT transport_dispatches — the prototype mismatch
    // the recon flagged).
    static bool migrate_iform_bahikhata(
        MigrationContext& ctx,
        const std::vector<std::map<std::string, std::string>>& transRows,
        const std::vector<std::map<std::string, std::string>>& stockTransRows,
        const std::map<int, PartyDetail>& ledgerDetailMap,
        const std::map<int, ItemDetail>& itemCodeMap,
        std::map<QString, int>& fyNameToId
    );

    // Bahi-Khata TDS vouchers (C3 slice M2: monolith PASS 5.4, verbatim).
    // TD keys via tdsField(); Jrnl grouping + EntryType leg split stay
    // literal (Transactions-source reads use voucherField()).
    static bool migrate_tds_bahikhata(
        MigrationContext& ctx,
        const std::vector<std::map<std::string, std::string>>& transRows,
        const std::vector<std::map<std::string, std::string>>& tdsRows,
        const std::map<int, PartyDetail>& ledgerDetailMap,
        std::map<QString, int>& fyNameToId
    );

    // Bahi-Khata audited custom closing stocks (C3 slice M2: PASS 5.5).
    static bool migrate_closing_bahikhata(
        MigrationContext& ctx,
        const std::vector<std::map<std::string, std::string>>& customRows,
        const std::map<int, ItemDetail>& itemCodeMap,
        std::map<QString, int>& fyNameToId
    );

    // Bahi-Khata bardana (gunny-bag) transactions (C3 slice M2: PASS 5.6).
    static bool migrate_bardana_bahikhata(
        MigrationContext& ctx,
        const std::vector<std::map<std::string, std::string>>& bardanaRows,
        const std::map<int, std::string>& ledgerCodeMap,
        const std::map<int, PartyDetail>& ledgerDetailMap,
        const std::map<int, std::string>& ledgerDeletedNameMap,
        std::map<QString, int>& fyNameToId
    );

    // Bahi-Khata gate pass + register (C3 slice M2: PASS 5.7). GateRegister
    // source rows are intentionally unread (mirror-only JSON coverage) —
    // the live pass consumes GatePassVouchers only.
    static bool migrate_gate_bahikhata(
        MigrationContext& ctx,
        const std::vector<std::map<std::string, std::string>>& gatePassRows,
        const std::map<int, PartyDetail>& ledgerDetailMap,
        const std::map<int, ItemDetail>& itemCodeMap
    );

    // Bahi-Khata sauda forward contracts (C3 slice M2: PASS 5.8).
    static bool migrate_sauda_bahikhata(
        MigrationContext& ctx,
        const std::vector<std::map<std::string, std::string>>& saudaVchRows,
        const std::map<int, PartyDetail>& ledgerDetailMap,
        const std::map<int, ItemDetail>& itemCodeMap,
        std::map<QString, int>& fyNameToId
    );

    // Bahi-Khata brokerage/dalali settlements (C3 slice M2: PASS 5.9).
    static bool migrate_dalali_bahikhata(
        MigrationContext& ctx,
        const std::vector<std::map<std::string, std::string>>& brokerageRows,
        const std::map<int, PartyDetail>& ledgerDetailMap,
        std::map<QString, int>& fyNameToId
    );

    // Busy Mandi & Procurement Migration
    static bool migrate_mandi_busy(
        MigrationContext& ctx,
        const std::vector<std::map<std::string, std::string>>& mandiRows,
        const std::map<int, std::string>& accountCodeToName,
        const std::map<int, qint64>& accountCodeToId,
        std::map<QString, int>& fyNameToId
    );
};

} // namespace MahadevERP
