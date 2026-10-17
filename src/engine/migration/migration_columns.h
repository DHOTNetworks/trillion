#pragma once

// MigrationColumns — JSON-driven source-field lookup for C3 strangler
// slices (Doc 21). Wired tables: Bahi Groups (Groups -> account_groups,
// slice G), Bahi Ledgers (Ledgers -> parties, slice L), Bahi Stock
// (StockGroups/StockUnits/StockItems, slice S), Bahi Vouchers
// (Transactions -> vouchers+transactions dual-write, StockTransactions
// -> stock_transactions, SaleTransportationDetail -> transport_dispatches,
// slice V), Bahi Mandi (MillingVouchers -> manufacturing_batches+items,
// Transactions -> jform/iform vouchers+items, slice M1); later slices add
// their tables following the same pattern (columns() per entry +
// fallback mirror + verify function).
//
// Precedence: plan/schema_mapping.json wins when loadable; otherwise the
// compiled-in fallback mirror applies. The fallback is byte-identical by
// review, and verify*() fails LOUD on any drift (the sync tests run them),
// so the two can never diverge silently. Shipped apps without a plan/
// directory behave exactly as before.

#include "config/schema_mapping.h"
#include <QMap>
#include <QPair>
#include <QStringList>
#include <QVariant>

namespace MahadevERP {

class MigrationColumns {
public:
    // Source field for a Bahi-Groups target key ("name", "code1st", ...).
    // Empty when the target key is unknown (caller skips loud).
    static inline QString groupField(const QString& target) {
        return columnsFor("Groups", "account_groups", "bahikhata", groupFallback()).value(target.trimmed());
    }

    // Source field for a Bahi-Ledgers target key ("name", "pan", ...).
    // Empty when the target key is unknown (caller skips loud).
    static inline QString ledgerField(const QString& target) {
        return columnsFor("Ledgers", "parties", "bahikhata", ledgerFallback()).value(target.trimmed());
    }

    // "" when the JSON entry and the fallback agree (or JSON unloadable —
    // absence is not drift); otherwise a description of the mismatch.
    static inline QString verifyGroupColumns() {
        return verifyEntry("Groups", "account_groups", "bahikhata", groupFallback());
    }

    // "" when the JSON entry and the fallback agree (or JSON unloadable —
    // absence is not drift); otherwise a description of the mismatch.
    static inline QString verifyLedgerColumns() {
        return verifyEntry("Ledgers", "parties", "bahikhata", ledgerFallback());
    }

    // Source field for a Bahi-StockGroups target key ("group_name", ...).
    // Empty when the target key is unknown (caller skips loud).
    static inline QString stockGroupField(const QString& target) {
        return columnsFor("StockGroups", "stock_groups", "bahikhata", stockGroupFallback()).value(target.trimmed());
    }

    // Source field for a Bahi-StockUnits target key ("unit_name", ...).
    // Empty when the target key is unknown (caller skips loud).
    static inline QString stockUnitField(const QString& target) {
        return columnsFor("StockUnits", "stock_units", "bahikhata", stockUnitFallback()).value(target.trimmed());
    }

    // Source field for a Bahi-StockItems target key ("name", "gst_rate", ...).
    // Empty when the target key is unknown (caller skips loud).
    static inline QString stockField(const QString& target) {
        return columnsFor("StockItems", "stock_items", "bahikhata", stockFallback()).value(target.trimmed());
    }

    // "" when the JSON entry and the fallback agree (or JSON unloadable —
    // absence is not drift); otherwise a description of the mismatch.
    static inline QString verifyStockGroupColumns() {
        return verifyEntry("StockGroups", "stock_groups", "bahikhata", stockGroupFallback());
    }

    // "" when the JSON entry and the fallback agree (or JSON unloadable —
    // absence is not drift); otherwise a description of the mismatch.
    static inline QString verifyStockUnitColumns() {
        return verifyEntry("StockUnits", "stock_units", "bahikhata", stockUnitFallback());
    }

    // "" when the JSON entry and the fallback agree (or JSON unloadable —
    // absence is not drift); otherwise a description of the mismatch.
    static inline QString verifyStockColumns() {
        return verifyEntry("StockItems", "stock_items", "bahikhata", stockFallback());
    }

    // Source field for a Bahi-Transactions target key ("voucher_no",
    // "dr_cr", ...). C3 slice V. The JSON entry is a DUAL-WRITE map:
    // the legs block targets transactions/* and the header block targets
    // vouchers/*; duplicate targets collapse last-wins (header block is
    // listed second, so header sources win — e.g. party_name is
    // derived:party-resolution, amount is derived:leg-sum). derived:*
    // values are LOGIC markers resolved in VoucherMigrator, never source
    // field reads — mirror them for parity, never pass them to getField.
    static inline QString voucherField(const QString& target) {
        return columnsFor("Transactions", "vouchers+transactions", "bahikhata", voucherFallback()).value(target.trimmed());
    }

    // Source field for a Bahi-StockTransactions target key ("bags",
    // "weight_qtl", ...). C3 slice V (PASS 5.3 inventory ledger).
    static inline QString stockTransField(const QString& target) {
        return columnsFor("StockTransactions", "stock_transactions", "bahikhata", stockTransFallback()).value(target.trimmed());
    }

    // Source field for a Bahi-SaleTransportationDetail target key. C3
    // slice V: mirror + verify ONLY — the entry targets the
    // transport_dispatches table, which is populated at RUNTIME by
    // TransportDispatchController::syncFromVouchers (INSERT..SELECT from
    // migrated vouchers); the Bahi migration path does NOT write it. The
    // PASS 4 logistics cache keeps its literals.
    static inline QString transportField(const QString& target) {
        return columnsFor("SaleTransportationDetail", "transport_dispatches", "bahikhata", transportFallback()).value(target.trimmed());
    }

    // "" when the JSON entry and the fallback agree (or JSON unloadable —
    // absence is not drift); otherwise a description of the mismatch.
    static inline QString verifyVoucherColumns() {
        return verifyEntry("Transactions", "vouchers+transactions", "bahikhata", voucherFallback());
    }

    static inline QString verifyStockTransColumns() {
        return verifyEntry("StockTransactions", "stock_transactions", "bahikhata", stockTransFallback());
    }

    static inline QString verifyTransportColumns() {
        return verifyEntry("SaleTransportationDetail", "transport_dispatches", "bahikhata", transportFallback());
    }

    // Source field for a Bahi-MillingVouchers target key ("batch_no",
    // "weight_qtl", ...). C3 slice M1. Pipe-target keys
    // ("output_qtl|byproduct_qtl") and derived:* values are logic markers —
    // mirrored for parity, never field reads.
    static inline QString millingField(const QString& target) {
        return columnsFor("MillingVouchers", "manufacturing_batches+manufacturing_voucher_items", "bahikhata", millingFallback()).value(target.trimmed());
    }

    // Source field for a Bahi J-Form target key ("jform_no",
    // "zimidar_name", ...). C3 slice M1 (PASS 3.5). Form-specific keys
    // only — generic Transactions keys (DrCr/AccountCode/TransType) come
    // from voucherField() (same rows, same strings).
    static inline QString jformField(const QString& target) {
        return columnsFor("Transactions", "jform_vouchers+jform_voucher_items", "bahikhata", jformFallback()).value(target.trimmed());
    }

    // Source field for a Bahi I-Form target key ("iform_no",
    // "buyer_name", ...). C3 slice M1 (PASS 3.6). Same split as J-Form.
    static inline QString iformField(const QString& target) {
        return columnsFor("Transactions", "iform_vouchers+iform_voucher_items", "bahikhata", iformFallback()).value(target.trimmed());
    }

    static inline QString verifyMillingColumns() {
        return verifyEntry("MillingVouchers", "manufacturing_batches+manufacturing_voucher_items", "bahikhata", millingFallback());
    }

    static inline QString verifyJFormColumns() {
        return verifyEntry("Transactions", "jform_vouchers+jform_voucher_items", "bahikhata", jformFallback());
    }

    static inline QString verifyIFormColumns() {
        return verifyEntry("Transactions", "iform_vouchers+iform_voucher_items", "bahikhata", iformFallback());
    }

    // Source field for a Bahi-TDSDeductions target key ("cert_no"...).
    // C3 slice M2 (monolith PASS 5.4). Pipe compounds
    // (RateTDS|TDSRate) are ordered fallbacks via getFieldFirstClean.
    static inline QString tdsField(const QString& target) {
        return columnsFor("TDSDeductions", "tds_vouchers", "bahikhata", tdsFallback()).value(target.trimmed());
    }

    // Source field for a Bahi-CustomClosingStocks target key. C3 slice M2
    // (PASS 5.5). Fully covered — this pass has zero JSON-gap literals.
    static inline QString closingField(const QString& target) {
        return columnsFor("CustomClosingStocks", "custom_closing_stocks", "bahikhata", closingFallback()).value(target.trimmed());
    }

    // Source field for a Bahi-BardanaTransactions target key. C3 slice M2
    // (PASS 5.6). Fully covered.
    static inline QString bardanaField(const QString& target) {
        return columnsFor("BardanaTransactions", "bardana_transactions", "bahikhata", bardanaFallback()).value(target.trimmed());
    }

    // Source field for Bahi gate entries (PASS 5.7). Two JSON entries share
    // one table: GatePassVouchers (live rows) + GateRegister (mirror-only —
    // the monolith never reads gateRegRows, documented here so the entry
    // cannot drift silently).
    static inline QString gatepassField(const QString& target) {
        return columnsFor("GatePassVouchers", "gate_register", "bahikhata", gatepassFallback()).value(target.trimmed());
    }

    static inline QString gateregField(const QString& target) {
        return columnsFor("GateRegister", "gate_register", "bahikhata", gateregFallback()).value(target.trimmed());
    }

    // Source field for Bahi sauda entries (PASS 5.8). Same split:
    // SaudaVouchers is live; SaudaTransactions is mirror-only (the pass
    // reads voucher rows only).
    static inline QString saudaField(const QString& target) {
        return columnsFor("SaudaVouchers", "sauda_contracts", "bahikhata", saudaFallback()).value(target.trimmed());
    }

    static inline QString saudatxField(const QString& target) {
        return columnsFor("SaudaTransactions", "sauda_contracts", "bahikhata", saudatxFallback()).value(target.trimmed());
    }

    // Source field for a Bahi-BrokerageVouchers target key. C3 slice M2
    // (PASS 5.9). CrLedger stays a documented gap literal: the JSON maps
    // broker_id <- PartyCode but the code resolves the CrLedger column.
    static inline QString dalaliField(const QString& target) {
        return columnsFor("BrokerageVouchers", "dalali_settlements", "bahikhata", dalaliFallback()).value(target.trimmed());
    }

    static inline QString verifyTdsColumns() {
        return verifyEntry("TDSDeductions", "tds_vouchers", "bahikhata", tdsFallback());
    }

    static inline QString verifyClosingColumns() {
        return verifyEntry("CustomClosingStocks", "custom_closing_stocks", "bahikhata", closingFallback());
    }

    static inline QString verifyBardanaColumns() {
        return verifyEntry("BardanaTransactions", "bardana_transactions", "bahikhata", bardanaFallback());
    }

    static inline QString verifyGatepassColumns() {
        return verifyEntry("GatePassVouchers", "gate_register", "bahikhata", gatepassFallback());
    }

    static inline QString verifyGateregColumns() {
        return verifyEntry("GateRegister", "gate_register", "bahikhata", gateregFallback());
    }

    static inline QString verifySaudaColumns() {
        return verifyEntry("SaudaVouchers", "sauda_contracts", "bahikhata", saudaFallback());
    }

    static inline QString verifySaudatxColumns() {
        return verifyEntry("SaudaTransactions", "sauda_contracts", "bahikhata", saudatxFallback());
    }

    static inline QString verifyDalaliColumns() {
        return verifyEntry("BrokerageVouchers", "dalali_settlements", "bahikhata", dalaliFallback());
    }

    // ---- C3 Tally slice: XML element-name lookup ----
    // The Tally source is a directory of XML files, so the "source field"
    // is one or more element names (pipe-joined ordered fallback) that the
    // parser matches inside the corresponding record tag. See the parser
    // helper TallyDataMigrator::accepts(sub, key).
    static inline QString tallyAccountField(const QString& target) {
        return columnsFor("TallyTag:GROUP/LEDGER", "account_groups+parties", "tally", tallyAccountFallback()).value(target.trimmed());
    }

    static inline QString tallyStockField(const QString& target) {
        return columnsFor("TallyTag:STOCKGROUP/STOCKITEM", "stock_groups+stock_items", "tally", tallyStockFallback()).value(target.trimmed());
    }

    static inline QString tallyUnitField(const QString& target) {
        return columnsFor("TallyTag:UNIT", "stock_units", "tally", tallyUnitFallback()).value(target.trimmed());
    }

    static inline QString tallyVoucherField(const QString& target) {
        return columnsFor("TallyTag:VOUCHER+ALLLEDGERENTRIES.LIST", "vouchers+transactions", "tally", tallyVoucherFallback()).value(target.trimmed());
    }

    static inline QString verifyTallyAccountColumns() {
        return verifyEntry("TallyTag:GROUP/LEDGER", "account_groups+parties", "tally", tallyAccountFallback());
    }

    static inline QString verifyTallyStockColumns() {
        return verifyEntry("TallyTag:STOCKGROUP/STOCKITEM", "stock_groups+stock_items", "tally", tallyStockFallback());
    }

    static inline QString verifyTallyUnitColumns() {
        return verifyEntry("TallyTag:UNIT", "stock_units", "tally", tallyUnitFallback());
    }

    static inline QString verifyTallyVoucherColumns() {
        return verifyEntry("TallyTag:VOUCHER+ALLLEDGERENTRIES.LIST", "vouchers+transactions", "tally", tallyVoucherFallback());
    }

    // ---- C3 Busy slice: MDB-table column roster ----
    // One entry per Busy source table ("Busy:<Table>"). Keys are target
    // fields; values are the source column spec (pipe = ordered fallback
    // chain). This is the SSOT for which Busy columns feed which target.
    // The inspect and migrate paths legitimately use DIFFERENT chains for
    // the same column (e.g. Company.gstin is "GSTNo|GSTIN" in inspect but
    // "GSTNo" alone in migrate), so the roster DECLARES the union and
    // scripts/busy_strangler_sim.py gates every read literal in
    // busy_data_migrator.cpp against it: a read of an undeclared column
    // fails the sim, never silently at runtime.
    //
    // Source spec for one target field of a Busy table ("" when unknown).
    static inline QString busyField(const QString& entry, const QString& target) {
        return busyFallback(entry).value(target.trimmed());
    }

    // "" when the JSON entry and the fallback agree (or JSON unloadable —
    // absence is not drift); otherwise a description of the mismatch.
    static inline QString verifyBusyColumns(const QString& entry) {
        const QString tgt = busyTarget(entry);
        if (tgt.isEmpty()) return entry + " is not a known Busy roster entry";
        return verifyEntry(entry, tgt, "busy", busyFallback(entry));
    }

    // First non-empty verify across the whole roster ("" = all agree).
    // (busyEntries()/busyTarget() are defined just below.)

    static inline QStringList busyEntries() {
        static const QStringList e = {
            "Busy:Company",
            "Busy:Master1",
            "Busy:MasterAddressInfo",
            "Busy:Folio1",
            "Busy:Tran1",
            "Busy:Tran2",
            "Busy:Tran3",
            "Busy:MandiVchItemDet",
            "Busy:MandiItemWiseExpense",
            "Busy:VchGSTSumItemWise",
            "Busy:Config",
            "Busy:MandiVchConfig",
            "Busy:DailySum",
            "Busy:VchOtherInfo"
        };
        return e;
    }

    static inline QString busyTarget(const QString& entry) {
        static const QMap<QString, QString> t = {
            {"Busy:Company", "company_info"},
            {"Busy:Master1", "parties+account_groups+stock_items"},
            {"Busy:MasterAddressInfo", "parties"},
            {"Busy:Folio1", "parties"},
            {"Busy:Tran1", "vouchers+transactions"},
            {"Busy:Tran2", "vouchers+transactions"},
            {"Busy:Tran3", "vouchers+transactions"},
            {"Busy:MandiVchItemDet", "paddy_slips+jform_voucher_items+iform_voucher_items"},
            {"Busy:MandiItemWiseExpense", "jform_voucher_items+iform_voucher_items"},
            {"Busy:VchGSTSumItemWise", "transactions"},
            {"Busy:Config", "series-config"},
            {"Busy:MandiVchConfig", "series-config"},
            {"Busy:DailySum", "day-book-summary"},
            {"Busy:VchOtherInfo", "vouchers"},
        };
        return t.value(entry);
    }

    static inline QString verifyAllBusyColumns() {
        for (const QString& e : busyEntries()) {
            const QString r = verifyBusyColumns(e);
            if (!r.isEmpty()) return r;
        }
        return QString();
    }

    // ---- C3 exporter slice: SQLite -> Jet positional roster ----
    // The exporter writes Jet tables POSITIONALLY (col 0..N), so the TDS
    // write-back mapping is an ORDERED pair of lists, not a name map. The
    // JSON "export" entry tds_vouchers -> TDSDeductions (columns[] carry
    // position / source / target / jet_type) is the SSOT; it is mirrored in
    // exporterTdsFallbackSides(). scripts/exporter_strangler_sim.py gates
    // every occurrence of that 19-column roster (SQLite DDL, backfill
    // INSERT, tds_model INSERT, exporter SELECT, exporter positional
    // write + dedup read) against it, and the exporter itself refuses to
    // write TDS rows when the target template's column order does not match
    // (fail loud, skip the table instead of writing mis-ordered values).
    static inline QPair<QStringList, QStringList> exporterTdsSides() {
        static QPair<QStringList, QStringList> cached;
        static bool loaded = false;
        if (!loaded) {
            loaded = true;
            const QPair<QStringList, QStringList> j = exporterTdsSidesFromJson();
            cached = j.first.isEmpty() ? exporterTdsFallbackSides() : j;
        }
        return cached;
    }

    // Jet column names in positional order ("" entries never occur; a
    // short list means the roster is empty and the caller must skip loud).
    static inline QStringList exporterTdsJetColumns() {
        return exporterTdsSides().second;
    }

    // SQLite column names in the same positional order.
    static inline QStringList exporterTdsSqliteColumns() {
        return exporterTdsSides().first;
    }

    static inline int exporterTdsCount() { return exporterTdsJetColumns().size(); }

    static inline QString exporterTdsJet(int pos) {
        const QStringList l = exporterTdsJetColumns();
        return (pos >= 0 && pos < l.size()) ? l.at(pos) : QString();
    }

    static inline QString exporterTdsSqlite(int pos) {
        const QStringList l = exporterTdsSqliteColumns();
        return (pos >= 0 && pos < l.size()) ? l.at(pos) : QString();
    }

    // "" when the JSON roster and the fallback agree (both sides, in order)
    // or the entry is absent/unloadable (absence is not drift); otherwise a
    // description of the mismatch.
    static inline QString verifyExporterTdsColumns() {
        const QPair<QStringList, QStringList> j = exporterTdsSidesFromJson();
        if (j.first.isEmpty()) return QString();
        const QPair<QStringList, QStringList> f = exporterTdsFallbackSides();
        if (j == f) return QString();
        return QString("exporter TDS roster drift: json=%1|%2 fallback=%3|%4")
            .arg(j.first.join(","), j.second.join(","), f.first.join(","), f.second.join(","));
    }

private:
    // JSON-ordered TDS roster sides; empty pair when the entry is
    // absent/unloadable or still uses the retired placeholder shape.
    static inline QPair<QStringList, QStringList> exporterTdsSidesFromJson() {
        QPair<QStringList, QStringList> out;
        SchemaMapping sm;
        if (!sm.loadDefault()) return out;
        for (const QVariant& v : sm.tables()) {
            QVariantMap m = v.toMap();
            if (m.value("source").toString() != "tds_vouchers") continue;
            if (m.value("target").toString() != "TDSDeductions") continue;
            if (m.value("provider").toString() != "bahikhata") continue;
            QMap<int, QPair<QString, QString>> byPos;
            for (const QVariant& c : m.value("columns").toList()) {
                QVariantMap cm = c.toMap();
                if (!cm.contains("position")) return out; // legacy placeholder
                byPos.insert(cm.value("position").toInt(),
                             qMakePair(cm.value("source").toString(),
                                       cm.value("target").toString()));
            }
            for (auto it = byPos.constBegin(); it != byPos.constEnd(); ++it) {
                out.first.append(it.value().first);
                out.second.append(it.value().second);
            }
            return out;
        }
        return out;
    }

    // Mirror of the JSON export entry (positional order 0..18). Kept
    // byte-identical to the file by verifyExporterTdsColumns().
    static inline QPair<QStringList, QStringList> exporterTdsFallbackSides() {
        static const QStringList sqlite = {
            "voucher_date", "voucher_number", "trans_type", "account_code",
            "income_amount", "tds", "surcharge", "education", "tds_rate",
            "income_type", "reason_for_non_deduction", "rate_tds",
            "rate_surcharge", "rate_education", "post_in_books",
            "challan_voucher_date", "challan_voucher_number",
            "income_narration", "prev_income_amount"
        };
        static const QStringList jet = {
            "VoucherDate", "VoucherNumber", "TransType", "AccountCode",
            "IncomeAmount", "TDS", "Surcharge", "Education", "TDSRate",
            "IncomeType", "ReasonForNonDeduction", "RateTDS", "RateSurcharge",
            "RateEducation", "PostInBooks", "ChallanVoucherDate",
            "ChallanVoucherNumber", "IncomeNarration", "PrevIncomeAmount"
        };
        return qMakePair(sqlite, jet);
    }

    // ---- Busy roster mirrors (JSON-first; verify enforces) ----
    static inline QMap<QString, QString> busyFallback_Busy_Company() {
        // Busy:Company -> company_info
        QMap<QString, QString> f;
        f["name"] = "Name";
        f["gstin"] = "GSTNo|GSTIN";
        f["pan"] = "ITPAN|PAN";
        f["print_name"] = "PrintName";
        f["phone"] = "TelNo";
        f["email"] = "Email";
        f["address1"] = "Address1";
        f["address2"] = "Address2";
        f["address3"] = "Address3";
        f["fy_beg"] = "BegFY";
        return f;
    }

    static inline QMap<QString, QString> busyFallback_Busy_Master1() {
        // Busy:Master1 -> parties+account_groups+stock_items
        QMap<QString, QString> f;
        f["legacy_code"] = "Code";
        f["name"] = "Name";
        f["alias"] = "Alias";
        f["print_name"] = "PrintName";
        f["master_type"] = "MasterType";
        f["parent_group"] = "ParentGrp";
        f["address1"] = "Address1";
        f["address2"] = "Address2";
        f["address3"] = "Address3";
        f["address4"] = "Address4";
        f["city"] = "City";
        f["pincode"] = "PinCode";
        f["gstin"] = "GSTNo";
        f["pan"] = "ITPAN";
        f["mobile"] = "MobileNo";
        f["email"] = "Email";
        f["bank_name"] = "BankName";
        f["bank_account"] = "BankAcNo";
        f["ifsc_code"] = "IFSCCode";
        f["state_code"] = "StateCodeLong";
        f["opening_balance"] = "D1";
        f["balance_sign"] = "I2";
        f["credit_days"] = "D6";
        f["credit_limit"] = "D7";
        f["opening_qty"] = "D1";
        f["opening_rate"] = "D2";
        f["opening_value"] = "D3";
        f["gst_rate"] = "D4";
        f["hsn_code"] = "HSNCode";
        f["unit_code"] = "CM2|MasterSupport";
        f["sale_ledger"] = "CM3";
        f["purc_ledger"] = "CM4";
        f["sundry_flag"] = "I1";
        f["sundry_post_1"] = "CM1";
        f["sundry_post_2"] = "CM2";
        return f;
    }

    static inline QMap<QString, QString> busyFallback_Busy_MasterAddressInfo() {
        // Busy:MasterAddressInfo -> parties
        QMap<QString, QString> f;
        f["master_code"] = "MasterCode";
        f["address1"] = "Address1";
        f["address2"] = "Address2";
        f["address3"] = "Address3";
        f["address4"] = "Address4";
        f["city"] = "City";
        f["pincode"] = "PinCode";
        f["gstin"] = "GSTNo";
        f["pan"] = "ITPAN";
        f["mobile"] = "MobileNo";
        f["email"] = "Email";
        f["bank_name"] = "BankName";
        f["bank_account"] = "BankAcNo";
        f["ifsc_code"] = "IFSCCode";
        f["state_code"] = "StateCodeLong";
        return f;
    }

    static inline QMap<QString, QString> busyFallback_Busy_Folio1() {
        // Busy:Folio1 -> parties
        QMap<QString, QString> f;
        f["master_code"] = "MasterCode";
        f["flag"] = "B1";
        f["value"] = "D1";
        return f;
    }

    static inline QMap<QString, QString> busyFallback_Busy_Tran1() {
        // Busy:Tran1 -> vouchers+transactions
        QMap<QString, QString> f;
        f["vch_code"] = "VchCode";
        f["vch_type"] = "VchType";
        f["vch_series"] = "VchSeriesCode";
        f["voucher_no"] = "VchNo";
        f["voucher_date"] = "Date";
        f["total_amount"] = "VchAmtBaseCur|VchSalePurcAmt|OrgVchAmtBaseCur|TotalAmount";
        f["narration"] = "Narration";
        f["party_code"] = "MasterCode1";
        f["auto_no"] = "AutoVchNo";
        f["cancelled"] = "Cancelled";
        f["vch_cancelled"] = "VchCancelled";
        return f;
    }

    static inline QMap<QString, QString> busyFallback_Busy_Tran2() {
        // Busy:Tran2 -> vouchers+transactions
        QMap<QString, QString> f;
        f["vch_code"] = "VchCode";
        f["record_kind"] = "RecType";
        f["type"] = "Type";
        f["amount"] = "Amount";
        f["value1"] = "Value1";
        f["value3"] = "Value3";
        f["d1"] = "D1";
        f["d2"] = "D2";
        f["d5"] = "D5";
        f["account_code"] = "MasterCode1";
        f["second_code"] = "MasterCode2";
        f["row_no"] = "SrNo";
        f["short_narration"] = "ShortNar";
        return f;
    }

    static inline QMap<QString, QString> busyFallback_Busy_Tran3() {
        // Busy:Tran3 -> vouchers+transactions
        QMap<QString, QString> f;
        f["vch_code"] = "VchCode";
        f["ref_no"] = "No";
        f["ref_status"] = "Status";
        f["ref_type"] = "Type";
        return f;
    }

    static inline QMap<QString, QString> busyFallback_Busy_MandiVchItemDet() {
        // Busy:MandiVchItemDet -> paddy_slips+jform_voucher_items+iform_voucher_items
        QMap<QString, QString> f;
        f["family"] = "derived:series-config";
        f["vch_code"] = "VchCode";
        f["voucher_no"] = "VchNo";
        f["voucher_date"] = "Date";
        f["row_no"] = "SrNo";
        f["item_code"] = "ItemCode";
        f["bs_code"] = "BSCode";
        f["bags"] = "BagQty";
        f["weight_per_bag"] = "WeightPerBag";
        f["loose_weight"] = "LooseWeight";
        f["buyer_weight"] = "BuyerWeight";
        f["party_code"] = "PartyCode";
        f["purchaser_code"] = "PurchaserCode";
        f["purc_default"] = "PurcDefVal";
        f["sale_default"] = "SaleDefVal";
        f["d1"] = "D1";
        f["d3"] = "D3";
        f["d5"] = "D5";
        f["value1"] = "Value1";
        f["narration1"] = "Narration1";
        f["narration2"] = "Narration2";
        f["bill_no"] = "PurchaseBillNo";
        f["bill_date"] = "PurchaseBillDate";
        f["vehicle_no"] = "VehicleNo";
        return f;
    }

    static inline QMap<QString, QString> busyFallback_Busy_MandiItemWiseExpense() {
        // Busy:MandiItemWiseExpense -> jform_voucher_items+iform_voucher_items
        QMap<QString, QString> f;
        f["item_code"] = "ItemCode";
        f["bs_code"] = "BSCode";
        f["purc_default"] = "PurcDefVal";
        f["sale_default"] = "SaleDefVal";
        return f;
    }

    static inline QMap<QString, QString> busyFallback_Busy_VchGSTSumItemWise() {
        // Busy:VchGSTSumItemWise -> transactions
        QMap<QString, QString> f;
        f["vch_code"] = "VchCode";
        f["party_code"] = "PartyCode";
        f["hsn_code"] = "HSNCode";
        f["tax_cat"] = "TaxCatCode";
        f["tax_rate"] = "TaxRate";
        f["party_state_code"] = "PartyStateCode";
        f["party_state_tin_digit"] = "PartyStateTinDigit";
        f["pos_state_tin_digit"] = "POSStateTinDigit";
        return f;
    }

    static inline QMap<QString, QString> busyFallback_Busy_Config() {
        // Busy:Config -> series-config
        QMap<QString, QString> f;
        f["record_kind"] = "RecType";
        f["series_code"] = "L1";
        f["series_prefix"] = "C2";
        return f;
    }

    static inline QMap<QString, QString> busyFallback_Busy_MandiVchConfig() {
        // Busy:MandiVchConfig -> series-config
        QMap<QString, QString> f;
        f["vs_code"] = "VSCode";
        f["def_mat_centre"] = "DefMatCentreCode";
        f["def_sale_purc_acc"] = "DefSalePurcAccCode";
        f["def_sale_purc_type"] = "DefSalePurcType";
        f["def_trading_account"] = "DefTradingAccount";
        f["jrnl_vch_series"] = "JrnlVchSeries";
        f["stk_tfr_vch_series"] = "StkTfrVchSeries";
        return f;
    }

    static inline QMap<QString, QString> busyFallback_Busy_DailySum() {
        // Busy:DailySum -> day-book-summary
        QMap<QString, QString> f;
        f["master_type"] = "MasterType";
        f["master_code1"] = "MasterCode1";
        f["master_code2"] = "MasterCode2";
        f["dr1"] = "Dr1";
        f["cr1"] = "Cr1";
        f["dr3"] = "Dr3";
        f["cr3"] = "Cr3";
        return f;
    }

    static inline QMap<QString, QString> busyFallback_Busy_VchOtherInfo() {
        // Busy:VchOtherInfo -> vouchers
        QMap<QString, QString> f;
        f["vch_code"] = "VchCode";
        f["gr_no"] = "GrNo";
        return f;
    }

    static inline QMap<QString, QString> busyFallback(const QString& entry) {
        if (entry == "Busy:Company") return busyFallback_Busy_Company();
        if (entry == "Busy:Master1") return busyFallback_Busy_Master1();
        if (entry == "Busy:MasterAddressInfo") return busyFallback_Busy_MasterAddressInfo();
        if (entry == "Busy:Folio1") return busyFallback_Busy_Folio1();
        if (entry == "Busy:Tran1") return busyFallback_Busy_Tran1();
        if (entry == "Busy:Tran2") return busyFallback_Busy_Tran2();
        if (entry == "Busy:Tran3") return busyFallback_Busy_Tran3();
        if (entry == "Busy:MandiVchItemDet") return busyFallback_Busy_MandiVchItemDet();
        if (entry == "Busy:MandiItemWiseExpense") return busyFallback_Busy_MandiItemWiseExpense();
        if (entry == "Busy:VchGSTSumItemWise") return busyFallback_Busy_VchGSTSumItemWise();
        if (entry == "Busy:Config") return busyFallback_Busy_Config();
        if (entry == "Busy:MandiVchConfig") return busyFallback_Busy_MandiVchConfig();
        if (entry == "Busy:DailySum") return busyFallback_Busy_DailySum();
        if (entry == "Busy:VchOtherInfo") return busyFallback_Busy_VchOtherInfo();
        return {};
    }

    static inline QMap<QString, QString> groupFallback() {
        // Mirror of the JSON entry — change together (verify enforces).
        QMap<QString, QString> f;
        f["name"] = "GroupName";
        f["code1st"] = "Code1st";
        f["code2nd"] = "Code2nd";
        f["code3rd"] = "Code3rd";
        f["code4th"] = "Code4th";
        f["extract_in_balance_sheet"] = "ExtractInBalanceSheet";
        return f;
    }

    static inline QMap<QString, QString> ledgerFallback() {
        // Mirror of the JSON entry — change together (verify enforces).
        // Targets are parties columns except documented logic-only keys:
        // group_code (group lookup), legacy_id (Code1st identity),
        // tin_alt/VAT+urn_alt/BRN (ordered fallbacks), state_raw (STATE).
        QMap<QString, QString> f;
        f["name"] = "LedgerName";
        f["alias"] = "LedgerAlais";
        f["prefix"] = "Prefix";
        f["group_code"] = "GroupCode";
        f["opening_balance"] = "OpeningBal";
        f["balance_type"] = "OpeningType";
        f["mailing_name"] = "MailingName";
        f["address"] = "MailingAdd";
        f["party_station"] = "PartyStation";
        f["station"] = "Station";
        f["party_state"] = "PartyState";
        f["gstin"] = "GSTIN";
        f["pan"] = "IncomeTaxNo";
        f["tan"] = "PartyTAN";
        f["aadhaar"] = "AadharNo";
        f["pincode"] = "PartyPINcode";
        f["route"] = "Route";
        f["phone"] = "Phone_O";
        f["mobile"] = "MobNoForSMS";
        f["tin"] = "SaleTaxNo";
        f["whatsapp"] = "WhatsappNo";
        f["email"] = "Email";
        f["contact_person"] = "ConcernedPerson";
        f["gst_party_type"] = "GSTPartyType";
        f["bank_name"] = "BankName";
        f["bank_account"] = "BankAccount";
        f["ifsc_code"] = "IFSCCode";
        f["credit_limit"] = "CreditLimit";
        f["interest_rate"] = "InterestRate";
        f["commission_rate"] = "Percentage";
        f["salary_per_month"] = "SalaryPerMonth";
        f["commission_on"] = "CommnCalcOn";
        f["apply_tcs"] = "ApplyTCSForParty";
        f["tcs_exempt"] = "TCSNotApplyInSale";
        f["party_type"] = "PartyType";
        f["special_type"] = "SpecialPartyType";
        f["shop_no"] = "ShopNo";
        f["tin_alt"] = "VAT";
        f["urn"] = "URN";
        f["urn_alt"] = "BRN";
        f["stock_not_calc"] = "StockNotCalculateInLedger";
        f["show_date_totals"] = "ShowDateTotals";
        f["calc_direct_expense"] = "CalculateInJointTrading";
        f["legacy_id"] = "Code1st";
        f["state_raw"] = "STATE";
        return f;
    }

    static inline QMap<QString, QString> stockGroupFallback() {
        // Mirror of the JSON entry — change together (verify enforces).
        QMap<QString, QString> f;
        f["group_name"] = "GroupName";
        f["legacy_code"] = "Code1st";
        f["qty_not_show_in_trading"] = "QtyNotShowInTrading";
        f["cl_stock_rate"] = "ClStockRate";
        return f;
    }

    static inline QMap<QString, QString> stockUnitFallback() {
        // Mirror of the JSON entry — change together (verify enforces).
        QMap<QString, QString> f;
        f["unit_name"] = "UnitName";
        f["legacy_code"] = "Code1st";
        f["decimal_places"] = "DecimalPlaces";
        return f;
    }

    static inline QMap<QString, QString> stockFallback() {
        // Mirror of the JSON entry — change together (verify enforces).
        // Targets are stock_items columns except logic-only keys: group_code/unit (map lookups),
        QMap<QString, QString> f;
        f["name"] = "ItemName";
        f["code"] = "Code1st";
        f["legacy_code"] = "Code1st";
        f["item_type"] = "ItemType";
        f["goods_type"] = "GoodsType";
        f["group_code"] = "GroupCode";
        f["unit"] = "UnitCode";
        f["unit_code"] = "UnitCode";
        f["rate_calc_on"] = "RateCalcOn";
        f["auto_adjust_name"] = "AutoSplitUpdate";
        f["item_narration"] = "ItemNarration";
        f["capital_goods"] = "CapitalGoods";
        f["hsn_code"] = "HSNCode";
        f["gst_rate"] = "GSTRateSlab";
        f["cess_rate"] = "CessRate";
        f["vat_rate"] = "VAT";
        f["vat_ledger"] = "VATLedger";
        f["vat_ledger_id"] = "VATLedger";
        f["surcharge_on_vat"] = "Surcharge";
        f["vat_against_d1"] = "VATLowerRate";
        f["cst_rate"] = "CST";
        f["cst_ledger"] = "CSTLedger";
        f["cst_ledger_id"] = "CSTLedger";
        f["cst_without_cform"] = "CSTWithoutCForm";
        f["dami_rate"] = "Dami";
        f["dami_ledger"] = "DamiLedger";
        f["dami_ledger_id"] = "DamiLedger";
        f["market_fee_rate"] = "MarketFee";
        f["market_fee_ledger"] = "MarketFeeLedger";
        f["market_fee_ledger_id"] = "MarketFeeLedger";
        f["hrdf_rate"] = "HRDF";
        f["hrdf_ledger"] = "HRDFLedger";
        f["hrdf_ledger_id"] = "HRDFLedger";
        f["market_commtt_form_apply"] = "MarketCommttFormApply";
        f["market_commtt_coupon_apply"] = "MarketCommttCouponApply";
        f["dami_calc_on_weight"] = "DamiCalcOnWeight";
        f["tax_on_qty"] = "TaxPayable";
        f["purchase_ledger"] = "PurchaseLedger";
        f["purchase_ledger_id"] = "PurchaseLedger";
        f["purchase_return_ledger"] = "PurchaseReturnLedger";
        f["sale_ledger"] = "SaleLedger";
        f["sale_ledger_id"] = "SaleLedger";
        f["sale_return_ledger"] = "SaleReturnLedger";
        f["stock_ledger"] = "StockLedger";
        f["stock_ledger_id"] = "StockLedger";
        f["is_manufacturing_item"] = "MillingItem";
        f["include_in_trading"] = "CalculateInTrading";
        f["calculate_stock"] = "StockCalculate";
        f["labour_rate_unit"] = "LabourRateUnit";
        f["purchase_rate"] = "PurcRate";
        f["sale_rate"] = "SaleRate";
        f["bonus_approved"] = "DDInsentive";
        f["mrp"] = "MRP";
        f["discount"] = "Discount";
        f["packing_kg"] = "Packing";
        f["opening_bags"] = "OpeningBags";
        f["opening_qty"] = "OpeningQty";
        f["opening_rate"] = "OpeningRate";
        f["opening_value"] = "OpeningValue";
        f["company_name"] = "CompanyName";
        f["utrai_rate_1"] = "UtraiRate1";
        f["jharai_rate_1"] = "JharaiRate1";
        f["bharai_rate_1"] = "BharaiRate1";
        f["tulai_rate_1"] = "TulaiRate1";
        f["khichai_rate_1"] = "KhichaiRate1";
        f["silai_rate_1"] = "SilaiRate1";
        f["loading_rate_1"] = "LoadingRate1";
        f["utrai_rate_2"] = "UtraiRate2";
        f["jharai_rate_2"] = "JharaiRate2";
        f["bharai_rate_2"] = "BharaiRate2";
        f["tulai_rate_2"] = "TulaiRate2";
        f["khichai_rate_2"] = "KhichaiRate2";
        f["silai_rate_2"] = "SilaiRate2";
        f["loading_rate_2"] = "LoadingRate2";
        f["utrai_rate_3"] = "UtraiRate3";
        f["jharai_rate_3"] = "JharaiRate3";
        f["bharai_rate_3"] = "BharaiRate3";
        f["tulai_rate_3"] = "TulaiRate3";
        f["khichai_rate_3"] = "KhichaiRate3";
        f["silai_rate_3"] = "SilaiRate3";
        f["loading_rate_3"] = "LoadingRate3";
        f["purchase_return_ledger_id"] = "PurchaseReturnLedger";
        f["sale_return_ledger_id"] = "SaleReturnLedger";
        return f;
    }

    static inline QMap<QString, QString> voucherFallback() {
        // Mirror of the JSON Transactions->vouchers+transactions entry —
        // change together (verify enforces). DUAL-WRITE: legs block +
        // header block collapsed last-wins (header listed second). Keys
        // with derived:* values are logic markers (fy dates, party
        // resolution, double-entry opposing, leg-sum amount, stock
        // presence, TDS math) — mirrored for parity, never field reads.
        // Pipe compounds (invoice_no, narration) are ordered fallbacks:
        // code splits on '|' and tries each in order.
        QMap<QString, QString> f;
        f["fy_id"] = "derived:VoucherDate";
        f["financial_year"] = "derived:VoucherDate";
        f["voucher_no"] = "VoucherNumber";
        f["voucher_date"] = "VoucherDate";
        f["voucher_type"] = "TransType";
        f["trans_type"] = "TransType";
        f["account_code"] = "AccountCode";
        f["party_id"] = "AccountCode";
        f["party_name"] = "derived:party-resolution";
        f["opposing_account"] = "derived:double-entry";
        f["dr_cr"] = "DrCr";
        f["amount"] = "derived:leg-sum";
        f["invoice_no"] = "InvoiceNo|TaxInvoiceNo";
        f["narration"] = "Narration|Narrtn";
        f["sauda_date"] = "SaudaDate";
        f["bank_date"] = "BankDate";
        f["broker_name"] = "BrokerName";
        f["vehicle_no"] = "VehicleNo";
        f["gr_no"] = "GRNo";
        f["taxable_amount"] = "Taxable194Q";
        f["tds_amount"] = "derived:TDS194Q";
        f["tds_rate"] = "TDSRate194Q";
        f["row_no"] = "RowNo";
        f["legacy_type"] = "TransType";
        f["ledger_id"] = "derived:party-resolution";
        f["account_type"] = "derived:double-entry";
        f["driver_name"] = "DriverName";
        f["eway_bill_no"] = "EWayBillNo";
        f["farmer_name"] = "ZimidarName";
        f["due_days"] = "DueDays";
        f["market_type"] = "derived:stock-presence";
        f["tax_status"] = "IFormTaxStatus";
        f["place_of_supply"] = "PlaceOfSupply";
        f["challan_no"] = "ChallanVchNo";
        return f;
    }

    static inline QMap<QString, QString> stockTransFallback() {
        // Mirror of the JSON StockTransactions->stock_transactions entry
        // (PASS 5.3 inventory ledger) — change together (verify enforces).
        // party_name/item_name are logic markers (ledger/item maps).
        QMap<QString, QString> f;
        f["voucher_no"] = "VoucherNumber";
        f["voucher_date"] = "VoucherDate";
        f["trans_type"] = "TransType";
        f["voucher_type"] = "VoucherType";
        f["party_id"] = "PartyCode";
        f["party_name"] = "derived:ledger-map";
        f["bill_no"] = "InvoiceNo|TaxInvoiceNo";
        f["item_id"] = "ItemCode";
        f["item_code"] = "ItemCode";
        f["item_name"] = "derived:item-name";
        f["bags"] = "Bags";
        f["packing"] = "Packing";
        f["weight_qtl"] = "Weight";
        f["rate"] = "Rate";
        f["amount"] = "Amount";
        f["taxable_amount"] = "TaxableAmount";
        f["tax"] = "Tax";
        f["tax_type"] = "TaxType";
        f["narration"] = "Narration|Narrtn";
        f["row_no"] = "RowNo";
        return f;
    }

    static inline QMap<QString, QString> transportFallback() {
        // Mirror of the JSON SaleTransportationDetail->transport_dispatches
        // entry — change together (verify enforces). The table is populated
        // at runtime by TransportDispatchController::syncFromVouchers; the
        // Bahi migration path does not write it.
        QMap<QString, QString> f;
        f["voucher_no"] = "VoucherNumber";
        f["dispatch_date"] = "VoucherDate";
        f["party_name"] = "AccountCode";
        f["total_freight"] = "Amount";
        return f;
    }

    static inline QMap<QString, QString> millingFallback() {
        // Mirror of the JSON MillingVouchers->manufacturing_batches+
        // manufacturing_voucher_items entry — change together (verify
        // enforces). Pipe-target keys and derived:* values are logic
        // markers (output split, sums, yield, rate) — mirrored for
        // parity, never field reads.
        QMap<QString, QString> f;
        f["batch_no"] = "VoucherNumber";
        f["batch_date"] = "VoucherDate";
        f["input_variety"] = "derived:paddy-sum";
        f["input_qtl"] = "derived:input-sum";
        f["output_qtl|byproduct_qtl"] = "derived:output-split";
        f["wastage_qtl"] = "derived:input-output";
        f["yield_pct"] = "derived:yield";
        f["narration"] = "Narrtn";
        f["row_no"] = "RowNo";
        f["drcr"] = "DrCr";
        f["item_id"] = "ItemCode";
        f["item_code"] = "ItemCode";
        f["item_name"] = "ItemName";
        f["percentage"] = "Percentage";
        f["weight_qtl"] = "Weight";
        f["bags"] = "Bags";
        f["rate"] = "derived:rate";
        f["amount"] = "Amount";
        return f;
    }

    static inline QMap<QString, QString> jformFallback() {
        // Mirror of the JSON Transactions->jform_vouchers+jform_voucher_
        // items entry — change together (verify enforces). Form-specific
        // keys only; generic Transactions keys (DrCr/AccountCode/Amount)
        // resolve through voucherField(), zimidar/party resolution and
        // item sums are logic (derived: markers, never field reads).
        QMap<QString, QString> f;
        f["voucher_no"] = "VoucherNumber";
        f["voucher_date"] = "VoucherDate";
        f["jform_no"] = "InvoiceNo";
        f["zimidar_id"] = "derived:zimidar-resolution";
        f["zimidar_name"] = "ZimidarName";
        f["auction_sale_status"] = "JFormSaleStatus";
        f["due_days"] = "DueDays";
        f["total_bags|total_weight|goods_amount"] = "derived:item-sums";
        f["narration"] = "Narration";
        f["item_id"] = "ItemCode";
        f["item_name"] = "derived:item-name";
        f["bags"] = "Bags";
        f["loose_weight"] = "LooseWeight";
        f["packing"] = "Packing";
        f["weight"] = "Weight";
        f["rate"] = "Rate";
        f["amount"] = "Amount";
        return f;
    }

    static inline QMap<QString, QString> iformFallback() {
        // Mirror of the JSON Transactions->iform_vouchers+iform_voucher_
        // items entry — change together (verify enforces). Same split as
        // J-Form (form keys here, generic keys via voucherField()).
        QMap<QString, QString> f;
        f["voucher_no"] = "VoucherNumber";
        f["voucher_date"] = "VoucherDate";
        f["iform_no"] = "InvoiceNo";
        f["buyer_id"] = "derived:buyer-resolution";
        f["buyer_name"] = "derived:buyer-name";
        f["broker_name"] = "BrokerName";
        f["due_days"] = "DueDays";
        f["total_bags|total_weight|goods_amount"] = "derived:item-sums";
        f["mandi_fee_rate"] = "MarketFeeRate";
        f["hrdf_rate"] = "HRDFRate";
        f["narration"] = "Narration";
        f["item_id"] = "ItemCode";
        f["item_name"] = "derived:item-name";
        f["bags"] = "Bags";
        f["loose_weight"] = "LooseWeight";
        f["packing"] = "Packing";
        f["weight"] = "Weight";
        f["rate"] = "Rate";
        f["amount"] = "Amount";
        return f;
    }

    static inline QMap<QString, QString> tdsFallback() {
        // Mirror of the JSON TDSDeductions->tds_vouchers entry — change
        // together (verify enforces). Pipe compound (RateTDS|TDSRate) is
        // an ordered fallback; derived:* are logic markers.
        QMap<QString, QString> f;
        f["fy_id"] = "derived:VoucherDate";
        f["financial_year"] = "derived:VoucherDate";
        f["voucher_no"] = "VoucherNumber";
        f["voucher_date"] = "VoucherDate";
        f["day_of_week"] = "derived:weekday";
        f["post_in_books"] = "PostInBooks";
        f["tds_type"] = "IncomeType";
        f["ledger_id"] = "AccountCode";
        f["ledger_name"] = "derived:ledger-map|const:'TDS Deductee'";
        f["income_amount"] = "IncomeAmount";
        f["previous_amount"] = "PrevIncomeAmount";
        f["total_for_tds"] = "derived:sum";
        f["narration"] = "IncomeNarration";
        f["rate_tds"] = "RateTDS|TDSRate";
        f["tax_amount_tds"] = "TDS";
        f["rate_surcharge"] = "RateSurcharge";
        f["tax_amount_surcharge"] = "Surcharge";
        f["rate_cess"] = "RateEducation";
        f["tax_amount_cess"] = "Education";
        f["use_rounded_total"] = "const:1";
        f["total_tax_rate"] = "derived:sum";
        f["total_tax_amount"] = "derived:sum";
        f["net_amount"] = "derived:net";
        f["non_deduction_reason"] = "ReasonForNonDeduction";
        // Legacy TDS register fields (C3 TDS register-fidelity slice):
        // carried verbatim so the exporter round-trips the firm's
        // TDSDeductions rows instead of fabricating 'Jrnl' / '' / 0.
        f["trans_type"] = "TransType";
        f["challan_voucher_date"] = "ChallanVoucherDate";
        f["challan_voucher_number"] = "ChallanVoucherNumber";
        f["exp_ledger_id|tds_ledger_id"] = "derived:jrnl-EntryType";
        f["exp_ledger_name|tds_ledger_name"] = "derived:incType-expense|const:'T D S PEYABAL'";
        return f;
    }

    static inline QMap<QString, QString> closingFallback() {
        // Mirror of the JSON CustomClosingStocks->custom_closing_stocks entry.
        QMap<QString, QString> f;
        f["closing_date"] = "ClosingStockDate";
        f["item_id"] = "ItemCode";
        f["item_code"] = "ItemCode";
        f["item_name"] = "derived:item-name";
        f["bags"] = "Bags";
        f["weight_qtl"] = "Weight";
        f["amount"] = "Amount";
        return f;
    }

    static inline QMap<QString, QString> bardanaFallback() {
        // Mirror of the JSON BardanaTransactions->bardana_transactions entry.
        QMap<QString, QString> f;
        f["voucher_no"] = "VoucherNumber";
        f["voucher_date"] = "VoucherDate";
        f["vch_type"] = "VchType";
        f["party_id"] = "AccountCode";
        f["party_name"] = "derived:ledger-map";
        f["bardana_type"] = "BardanaType";
        f["godown_name"] = "GodownName";
        f["dr_cr"] = "derived:DrCr";
        f["qty"] = "Qty";
        f["rate"] = "Rate";
        f["amount"] = "derived:qty-rate";
        f["vehicle_no"] = "VehNo";
        f["bill_no"] = "InvoiceNo";
        f["narration"] = "Narration";
        return f;
    }

    static inline QMap<QString, QString> gatepassFallback() {
        // Mirror of the JSON GatePassVouchers->gate_register entry. Pipe
        // compounds are ordered fallbacks; const:* are fixed cell values.
        QMap<QString, QString> f;
        f["gate_pass_no"] = "GatePassNo";
        f["entry_date"] = "GatePassDate|VoucherDate";
        f["entry_time|direction"] = "const:'10:00'|const:'OUTWARD'";
        f["vehicle_no"] = "VehNo";
        f["driver_name"] = "DriverName";
        f["party_id"] = "AccountCode";
        f["party_name"] = "derived:ledger-map";
        f["commodity"] = "ItemCode";
        f["bag_count"] = "Bags";
        f["gross_weight_qtl"] = "TotalWeight|Weight";
        f["net_weight_qtl"] = "Weight";
        f["remarks"] = "Narration";
        return f;
    }

    static inline QMap<QString, QString> gateregFallback() {
        // Mirror of the JSON GateRegister->gate_register entry (verify-only:
        // the monolith never reads gateRegRows).
        QMap<QString, QString> f;
        f["gate_pass_no"] = "GatePassNo";
        f["entry_date"] = "GatePassDate|VoucherDate";
        f["entry_time|direction"] = "const:'10:00'|const:'OUTWARD'";
        f["vehicle_no"] = "VehNo";
        f["driver_name"] = "DriverName";
        f["party_id"] = "AccountCode";
        f["party_name"] = "derived:ledger-map";
        f["commodity"] = "ItemCode";
        f["bag_count"] = "Bags";
        f["gross_weight_qtl"] = "TotalWeight|Weight";
        f["net_weight_qtl"] = "Weight";
        f["remarks"] = "Narration";
        return f;
    }

    static inline QMap<QString, QString> saudaFallback() {
        // Mirror of the JSON SaudaVouchers->sauda_contracts entry.
        QMap<QString, QString> f;
        f["sauda_no"] = "SaudaNo";
        f["sauda_date"] = "VoucherDate";
        f["sauda_type"] = "SaudaType";
        f["party_id"] = "AccountCode";
        f["party_name"] = "derived:ledger-map";
        f["broker_name"] = "Broker";
        f["item_id"] = "ItemCode";
        f["item_name"] = "ItemName";
        f["contracted_weight_qtl"] = "Weight";
        f["rate_per_qtl"] = "Rate";
        f["condition_notes"] = "Condition";
        f["delivery_to"] = "DueDt";
        return f;
    }

    static inline QMap<QString, QString> saudatxFallback() {
        // Mirror of the JSON SaudaTransactions->sauda_contracts entry
        // (verify-only: the pass reads voucher rows only).
        QMap<QString, QString> f;
        f["sauda_no"] = "SaudaNo";
        f["sauda_date"] = "VoucherDate";
        f["sauda_type"] = "SaudaType";
        f["party_id"] = "AccountCode";
        f["party_name"] = "derived:ledger-map";
        f["broker_name"] = "Broker";
        f["item_id"] = "ItemCode";
        f["item_name"] = "ItemName";
        f["contracted_weight_qtl"] = "Weight";
        f["rate_per_qtl"] = "Rate";
        f["condition_notes"] = "Condition";
        f["delivery_to"] = "DueDt";
        return f;
    }

    static inline QMap<QString, QString> dalaliFallback() {
        // Mirror of the JSON BrokerageVouchers->dalali_settlements entry.
        // broker_id maps PartyCode in JSON but the code resolves the
        // CrLedger column — documented gap literal, never "fixed" by
        // swapping the lookup (different source column).
        QMap<QString, QString> f;
        f["settlement_no"] = "VoucherNumber";
        f["settlement_date"] = "VoucherDate";
        f["broker_id"] = "PartyCode";
        f["broker_name"] = "derived:ledger-map";
        f["sauda_no"] = "const:'SD-MIGRATED'";
        f["voucher_type"] = "const:'SALE'";
        f["voucher_no"] = "VoucherNumber";
        f["invoice_no"] = "InvoiceNo";
        f["party_name"] = "derived:party-name";
        f["item_name"] = "const:'Rice Basmati'";
        f["weight_qtl"] = "ItemWeight";
        f["rate_per_qtl"] = "ItemRate";
        f["dalali_rate"] = "Rate";
        f["dalali_amount"] = "Amount";
        f["tds_pct"] = "const:5.0";
        f["tds_amount"] = "derived:5pct";
        f["net_dalali_payable"] = "derived:net";
        f["journal_voucher_no"] = "derived:JV-DAL-series";
        f["narration"] = "Narration";
        return f;
    }

    // ---- C3 Tally slice fallbacks (mirror the JSON; verify enforces) ----
    static inline QMap<QString, QString> tallyAccountFallback() {
        // Mirror of TallyTag:GROUP/LEDGER -> account_groups+parties.
        // Values are XML element names (pipe = ordered fallback); the
        // parser matches them inside GROUP/LEDGER via accepts().
        QMap<QString, QString> f;
        f["name"] = "NAME";
        f["group_name"] = "PARENT";
        f["opening_balance"] = "OPENINGBALANCE";
        f["balance_type"] = "ISDEEMEDPOSITIVE";
        f["address"] = "ADDRESS";
        f["pincode"] = "PINCODE";
        f["state"] = "PARTYSTATE|STATENAME|LEDSTATENAME";
        f["pan"] = "INCOMETAXNUMBER|PAN";
        f["gstin"] = "PARTYGSTIN|GSTIN|GSTREGISTRATIONNUMBER";
        f["bank_name"] = "BANKNAME";
        f["bank_account"] = "ACCOUNTNUMBER|BANKDETAILS";
        f["bank_ifsc"] = "IFSCODE";
        f["credit_limit"] = "CREDITLIMIT";
        f["credit_days"] = "BILLCREDITPERIOD";
        f["nature"] = "ISREVENUE|AFFECTSGROSSPROFIT";
        return f;
    }

    static inline QMap<QString, QString> tallyStockFallback() {
        // Mirror of TallyTag:STOCKGROUP/STOCKITEM -> stock_groups+stock_items.
        QMap<QString, QString> f;
        f["name"] = "NAME";
        f["group_name"] = "PARENT";
        f["unit"] = "BASEUNITS|UNIT";
        f["opening_qty"] = "OPENINGBALANCE";
        f["opening_rate"] = "OPENINGRATE";
        f["opening_value"] = "OPENINGVALUE";
        f["gst_rate"] = "GSTRATE";
        f["hsn_code"] = "HSNCODE";
        return f;
    }

    static inline QMap<QString, QString> tallyUnitFallback() {
        // Mirror of TallyTag:UNIT -> stock_units.
        QMap<QString, QString> f;
        f["unit_name"] = "NAME";
        f["decimal_places"] = "DECIMALPLACES";
        return f;
    }

    static inline QMap<QString, QString> tallyVoucherFallback() {
        // Mirror of TallyTag:VOUCHER+ALLLEDGERENTRIES.LIST ->
        // vouchers+transactions. item_* keys drive the nested
        // ALLINVENTORYENTRIES.LIST block; ledger_* keys the split blocks.
        QMap<QString, QString> f;
        f["voucher_no"] = "VOUCHERNUMBER";
        f["voucher_date"] = "DATE";
        f["voucher_type"] = "VOUCHERTYPENAME|VCHTYPE";
        f["narration"] = "NARRATION";
        f["item_name"] = "STOCKITEMNAME";
        f["item_rate"] = "RATE";
        f["item_amount"] = "AMOUNT";
        f["item_qty"] = "ACTUALQTY|BILLEDQTY";
        f["item_batch"] = "BATCHNAME";
        f["item_godown"] = "GODOWNNAME";
        f["party_name"] = "PARTYLEDGERNAME|PARTYNAME";
        f["party_gstin"] = "PARTYGSTIN";
        f["party_state"] = "STATENAME|PARTYSTATE";
        f["party_pincode"] = "PARTYPINCODE";
        f["ledger_name"] = "LEDGERNAME";
        f["ledger_amount"] = "AMOUNT";
        f["ledger_positive"] = "ISDEEMEDPOSITIVE";
        return f;
    }
        // legacy_code (Code1st identity), *_ledger* (ledger maps), tax_on_qty (TaxPayable primary,
        // CessCalcOnQty fallback in code). Inventory INSERT derives from in-memory values.

    static inline QString verifyEntry(const QString& source, const QString& target,
                                      const QString& provider,
                                      const QMap<QString, QString>& fallback) {
        SchemaMapping sm;
        if (!sm.loadDefault()) return QString();
        for (const QVariant& v : sm.tables()) {
            QVariantMap m = v.toMap();
            if (m.value("source").toString() != source) continue;
            if (m.value("target").toString() != target) continue;
            if (m.value("provider").toString() != provider) continue;
            QMap<QString, QString> live;
            for (const QVariant& c : m.value("columns").toList()) {
                QVariantMap cm = c.toMap();
                live[cm.value("target").toString()] = cm.value("source").toString();
            }
            if (live == fallback) return QString();
            return QString("%1 columns drift: json=%2 fallback=%3")
                .arg(source, mapStr(live), mapStr(fallback));
        }
        return QString("%1 entry missing from schema map").arg(source);
    }

    static inline QMap<QString, QString> columnsFor(const QString& source,
                                                    const QString& target,
                                                    const QString& provider,
                                                    const QMap<QString, QString>& fallback) {
        // Process-level cache: migration rows hit this per field (~50× per
        // row); the JSON is read-only input within a process, so no
        // invalidation is needed. Without this, every lookup re-reads +
        // re-parses the file (measured 5× migration slowdown).
        static QMap<QString, QMap<QString, QString>> cache;
        const QString key = source + "|" + target + "|" + provider;
        auto hit = cache.constFind(key);
        if (hit != cache.constEnd()) {
            return hit.value().isEmpty() ? fallback : hit.value();
        }
        SchemaMapping sm;
        if (sm.loadDefault()) {
            for (const QVariant& v : sm.tables()) {
                QVariantMap m = v.toMap();
                if (m.value("source").toString() != source) continue;
                if (m.value("target").toString() != target) continue;
                if (m.value("provider").toString() != provider) continue;
                QMap<QString, QString> live;
                for (const QVariant& c : m.value("columns").toList()) {
                    QVariantMap cm = c.toMap();
                    live[cm.value("target").toString()] = cm.value("source").toString();
                }
                if (!live.isEmpty()) {
                    cache[key] = live;
                    return live;
                }
            }
        }
        cache[key] = QMap<QString, QString>(); // negative cache: use fallback
        return fallback;
    }

    static inline QString mapStr(const QMap<QString, QString>& m) {
        QStringList parts;
        for (auto it = m.constBegin(); it != m.constEnd(); ++it) {
            parts.append(it.key() + "<-" + it.value());
        }
        return parts.join(",");
    }
};

} // namespace MahadevERP
