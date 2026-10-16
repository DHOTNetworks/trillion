#pragma once

#include <QString>
#include <QStringList>
#include <QVariantList>
#include "../database_manager.h"
#include "settings_registry.h"

// FeatureFlags: opt-in domains for mandi/manufacturer/bardana/
// transport. Core (masters, sale/purchase/payment/receipt/journal,
// registers, GST, TDS/TCS, reports) always runs; domain DDL/widgets
// (gated below) consult these flags. Keys live in SettingsKeys (D2).
//
// Semantics:
// - Absent key == OFF (generic fresh firms write nothing).
// - Detection only ever turns flags ON, never OFF: existing data is never
//   stranded behind an off flag, and explicit user keys are never clobbered
//   with "0" (a user OFF with live rows is upgraded back to ON — data wins).
struct FeatureFlags {
    bool mandi = false;      // paddy, J-Form, I-Form, sauda, dalali
    bool manufacturer = false;       // manufacturering batches, yield
    bool bardana = false;    // gunny-bag ledger
    bool transport = false;  // dispatch/gate register

    static bool isOn(const QString& key) {
        return DatabaseManager::instance().getSetting(key, "0").trimmed() == "1";
    }

    static FeatureFlags load() {
        FeatureFlags f;
        f.mandi = isOn(SettingsKeys::feature_mandi());
        f.manufacturer = isOn(SettingsKeys::feature_manufacturer());
        f.bardana = isOn(SettingsKeys::feature_bardana());
        f.transport = isOn(SettingsKeys::feature_transport());
        return f;
    }

    static void save(const FeatureFlags& f) {
        // Explicit write path for the setup wizard / Settings dialog.
        auto& db = DatabaseManager::instance();
        db.setSetting(SettingsKeys::feature_mandi(), f.mandi ? "1" : "0");
        db.setSetting(SettingsKeys::feature_manufacturer(), f.manufacturer ? "1" : "0");
        db.setSetting(SettingsKeys::feature_bardana(), f.bardana ? "1" : "0");
        db.setSetting(SettingsKeys::feature_transport(), f.transport ? "1" : "0");
    }

    // True when the table exists (any row count). Used to guard DDL that
    // references domain tables on generic DBs where they were never created.
    static bool tableExists(const QString& table) {
        QVariant present = DatabaseManager::instance().executeScalar(
            "SELECT 1 FROM sqlite_master WHERE type='table' AND name=? LIMIT 1;", {table});
        return present.isValid() && !present.isNull();
    }

    // Doc 20 C2: table-name allowlist at the loader boundary. The COUNT(*)
    // below interpolates a table name, so only names from this fixed set
    // (the S7 12-table roster — exactly what detectFromData passes) are
    // accepted; anything else is refused instead of reaching SQL.
    static bool isKnownDomainTable(const QString& table) {
        static const QStringList kAllowed = {
            "jform_vouchers", "jform_voucher_items",
            "iform_vouchers", "iform_voucher_items",
            "paddy_slips",
            "sauda_contracts", "dalali_settlements",
            "manufacturing_batches", "manufacturing_voucher_items",
            "bardana_transactions",
            "gate_register", "transport_dispatches"
        };
        return kAllowed.contains(table);
    }

    // True when the table exists and holds at least one row. sqlite_master
    // is consulted first so detection never fails on generic DBs whose
    // domain tables were never created.
    static bool tableHasRows(const QString& table) {
        if (!isKnownDomainTable(table)) return false;
        if (!tableExists(table)) return false;
        auto& db = DatabaseManager::instance();
        QVariant n = db.executeScalar(QString("SELECT COUNT(*) FROM \"%1\";").arg(table));
        return n.isValid() && !n.isNull() && n.toLongLong() > 0;
    }

    static bool anyHasRows(const QStringList& tables) {
        for (const QString& t : tables) {
            if (tableHasRows(t)) return true;
        }
        return false;
    }

    // Data-driven detection for migrated firms. Any row in any domain
    // table forces its flag on.
    static FeatureFlags detectFromData() {
        FeatureFlags f;
        f.mandi = anyHasRows({"jform_vouchers", "jform_voucher_items",
                              "iform_vouchers", "iform_voucher_items",
                              "paddy_slips",
                              "sauda_contracts", "dalali_settlements"});
        f.manufacturer = anyHasRows({"manufacturing_batches", "manufacturing_voucher_items"});
        f.bardana = anyHasRows({"bardana_transactions"});
        f.transport = anyHasRows({"gate_register", "transport_dispatches"});
        return f;
    }

    // Called at the end of ensureTablesExist: persists detected ON flags
    // (upsert "1"); never writes "0", never touches unrelated keys.
    // Fresh generic DBs detect nothing and write nothing.
    static void persistDetected() {
        FeatureFlags det = detectFromData();
        auto& db = DatabaseManager::instance();
        if (det.mandi) db.setSetting(SettingsKeys::feature_mandi(), "1");
        if (det.manufacturer) db.setSetting(SettingsKeys::feature_manufacturer(), "1");
        if (det.bardana) db.setSetting(SettingsKeys::feature_bardana(), "1");
        if (det.transport) db.setSetting(SettingsKeys::feature_transport(), "1");
    }

    // ON-only priming for migrators: turns ON exactly the passed domains
    // (writes "1"); never writes "0", never clears user keys. Call BEFORE
    // ensureTablesExist so the gated domain tables exist for the rows about
    // to be migrated (migrator derives the bits from SOURCE data).
    static void primeFromSource(bool mandi, bool manufacturer, bool bardana, bool transport) {
        auto& db = DatabaseManager::instance();
        if (mandi) db.setSetting(SettingsKeys::feature_mandi(), "1");
        if (manufacturer) db.setSetting(SettingsKeys::feature_manufacturer(), "1");
        if (bardana) db.setSetting(SettingsKeys::feature_bardana(), "1");
        if (transport) db.setSetting(SettingsKeys::feature_transport(), "1");
    }

    // In-memory effective flags for DDL gating: stored keys upgraded by
    // data-detected ON bits. No writes here (persistDetected records them
    // at the end of ensureTablesExist). Existing rows always force their
    // domain on, so gating can never strand data.
    static FeatureFlags resolveEffective() {
        FeatureFlags eff = load();
        FeatureFlags det = detectFromData();
        if (det.mandi) eff.mandi = true;
        if (det.manufacturer) eff.manufacturer = true;
        if (det.bardana) eff.bardana = true;
        if (det.transport) eff.transport = true;
        return eff;
    }

    // Stacked-view index -> required feature keys (empty = core, always on).
    // Single map for menu filtering + the navigation guard; indices never
    // shift (disabled views stay in place, unreachable). A view is enabled
    // when ANY of its keys is on (S3: paddy serves mandi AND manufacturer firms).
    static QStringList requiredFlagsForView(int viewIndex) {
        switch (viewIndex) {
        case 18: case 19: case 20: case 72: return {SettingsKeys::feature_mandi()};
        case 1: return {SettingsKeys::feature_mandi(), SettingsKeys::feature_manufacturer()};
        case 31: case 35: return {SettingsKeys::feature_manufacturer()};
        case 70: return {SettingsKeys::feature_bardana()};
        case 27: case 71: return {SettingsKeys::feature_transport()};
        default: return {};
        }
    }
    // Compat: first key (the primary domain) for single-label call sites.
    static QString requiredFlagForView(int viewIndex) {
        const QStringList keys = requiredFlagsForView(viewIndex);
        return keys.isEmpty() ? QString() : keys.first();
    }
    static bool isViewEnabled(int viewIndex) {
        const QStringList keys = requiredFlagsForView(viewIndex);
        if (keys.isEmpty()) return true;
        for (const QString& k : keys) {
            if (isOn(k)) return true;
        }
        return false;
    }
    static QString domainLabelForFlag(const QString& key) {
        if (key == SettingsKeys::feature_mandi()) return "Mandi";
        if (key == SettingsKeys::feature_manufacturer()) return "Manufacturer";
        if (key == SettingsKeys::feature_bardana()) return "Bardana";
        if (key == SettingsKeys::feature_transport()) return "Transport";
        return "";
    }
};
