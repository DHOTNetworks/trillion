#pragma once

// Settings registry: the single catalog of every app_settings key.
// Rules (plan/23 §2, decision D2):
// - No raw literals in getSetting()/setSetting() calls; new keys are
//   registered here first.
// - Firm data lives in app_settings (DatabaseManager); machine-local UI
//   state lives in QSettings. Never mix the two.
// - gsp_password and api keys are flagged ENCRYPT-LATER and must never
//   be written to logs.
// New literals are snake_case (legacy CamelCase survives only inside the
// mapping JSON and at the MDB boundary).

#include <QString>

struct SettingsKeys {
    // Package flags (absent key = OFF; the flags engine owns semantics).
    static QString feature_mandi()     { return QStringLiteral("feature.mandi"); }
    static QString feature_manufacturer()      { return QStringLiteral("feature.manufacturer"); }
    static QString feature_bardana()   { return QStringLiteral("feature.bardana"); }
    static QString feature_transport() { return QStringLiteral("feature.transport"); }

    // Firm identity / provenance.
    static QString firm_business_type()   { return QStringLiteral("firm.business_type"); }
    static QString firm_source_type()     { return QStringLiteral("firm_source_type"); }
    static QString is_migrated()          { return QStringLiteral("is_migrated"); }
    static QString migration_source_mdb() { return QStringLiteral("migration_source_mdb"); }
    static QString audit_actor()          { return QStringLiteral("audit_actor"); }

    // Fiscal-year context.
    static QString last_selected_fy() { return QStringLiteral("last_selected_fy"); }
    static QString last_from_date()   { return QStringLiteral("last_from_date"); }
    static QString last_to_date()     { return QStringLiteral("last_to_date"); }

    // One-shot repair markers.
    static QString transport_deduped_v1()   { return QStringLiteral("transport_deduped_v1"); }
    static QString numbering_repair_needed() { return QStringLiteral("numbering_repair_needed"); }

    // Per-document bill series (JSON blobs under one prefix).
    // Existing keys are lowercase: the helper lowercases (callers must not
    // pre-lowercase; the key is byte-identical either way).
    static QString bill_series_prefix() { return QStringLiteral("bill_series/"); }
    static QString bill_series(const QString& docType) {
        return bill_series_prefix() + docType.trimmed().toLower();
    }

    // TCS configuration.
    static QString tcs_threshold_limit()   { return QStringLiteral("tcs_threshold_limit"); }
    static QString tcs_rate_with_pan()     { return QStringLiteral("tcs_rate_with_pan"); }
    static QString tcs_rate_without_pan()  { return QStringLiteral("tcs_rate_without_pan"); }
    static QString tcs_payable_ledger_id() { return QStringLiteral("tcs_payable_ledger_id"); }

    // GST identity + GSP credentials (ENCRYPT-LATER — never log values).
    // The lookup key is hierarchical: "gstin_lookup/api_key" (that is the
    // actual stored key — never a flat "gstin_api_key").
    static QString company_gstin()  { return QStringLiteral("company_gstin"); }
    static QString gstin_lookup()   { return QStringLiteral("gstin_lookup"); }
    static QString gstin_api_key()  { return gstin_lookup() + QStringLiteral("/api_key"); }
    static QString gsp_username()   { return QStringLiteral("gsp_username"); }
    static QString gsp_password()   { return QStringLiteral("gsp_password"); }   // ENCRYPT-LATER
    static QString gsp_client_id()  { return QStringLiteral("gsp_client_id"); }  // ENCRYPT-LATER
    static QString gsp_client_secret() { return QStringLiteral("gsp_client_secret"); }  // ENCRYPT-LATER
    static QString gsp_access_token()  { return QStringLiteral("gsp_access_token"); }   // ENCRYPT-LATER

    // Machine-local UI state (QSettings side, not app_settings).
    static QString ui_scale_percent() { return QStringLiteral("ui_scale_percent"); }

    // Year-end close (F3 engine): closing voucher ref per FY. Added here per
    // D2 (new keys registered in the registry); announced to Agent 2.
    static QString fy_close_voucher(const QString& fyName) {
        return QStringLiteral("fy_close_voucher/") + fyName.trimmed();
    }

    // Year-end close custom steps (F3 skeleton; JSON arrays of
    // {id,title,note} under close_steps/*). Config, not a table:
    // definitions versioned by key, completions audited as events.
    // Lookup order: close_steps/custom (firm-edited) else
    // close_steps/<firm.business_type>; absent both = no extra steps.
    static QString close_steps_prefix() { return QStringLiteral("close_steps/"); }
    static QString close_steps(const QString& scope) {
        return close_steps_prefix() + scope.trimmed().toLower();
    }
    // Built-in S4 defaults per package (W9-A, one place — no DB seeds).
    // Lookup order in the dialog: close_steps/custom, then
    // close_steps/<business_type>, then this fallback. trader /
    // transporter / general / unknown ship no extra steps.
    static QString close_steps_default(const QString& businessType) {
        const QString pkg = businessType.trimmed().toLower();
        if (pkg == "commission_agent") {
            return "[{\"id\":\"sauda_settlement\",\"title\":\"Sauda settlement sign-off\","
                   "\"note\":\"All forward contracts settled or carried forward\"},"
                   "{\"id\":\"dalali_settlement\",\"title\":\"Dalali settlement sign-off\","
                   "\"note\":\"Brokerage settled for the year\"}]";
        }
        if (pkg == "manufacturing") {
            return "[{\"id\":\"stock_verification\",\"title\":\"Stock verification\","
                   "\"note\":\"Physical stock tallied with book stock\"},"
                   "{\"id\":\"manufacturing_yield_review\",\"title\":\"Manufacturing Production review\","
                   "\"note\":\"Out-turn percentages reviewed for the year\"}]";
        }
        return "";
    }
};
