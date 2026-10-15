# Phase 3: Feature Flags + Generic Voucher Engine

## Goal
Mandi/mill tables and flows become opt-in domains behind flags; voucher logic
is driven by `voucher_types` rows (Phase 1), not by `if (type == "JFrm")` chains.

## Current State (Verified)
- Domain tables always created in `ensureTablesExist`: `paddy_procurement`,
  `paddy_arrivals`, `jform_vouchers(+items)`, `iform_vouchers(+items)`,
  `milling_batches`, `milling_voucher_items`, `bardana_transactions`,
  `gate_register`, `sauda_contracts`, `dalali_settlements`, `TDSDeductions`.
- Every widget/model/controller for these loads unconditionally in
  `MainWindow` ctor + stacked widget (~50 widgets, all constructed at startup).
- Voucher-type if-chains duplicated per site: `vouchers_model.cpp:34-48,903-918`,
  exporter `normalizeTxType`, migrator `mapVoucherTypeStr`, jform/iform `prefixRe`.

## Target State
- `app_settings` keys `feature.mandi`, `feature.mill`, `feature.bardana`,
  `feature.transport` (default OFF for new firms; migrated firms get flags from
  detected data).
- Domain tables created only when their flag is on; domain widgets/menus only
  registered then; core app (masters, sale/purchase/payment/receipt/journal,
  registers, GST, TDS/TCS, reports) works with zero domain code paths.
- One dispatch: `canonicalTypeToken` + `VoucherTypeModel::byCode` everywhere.

## Exact Edits

### 1. NEW `src/config/feature_flags.h` (header-only)
```cpp
#pragma once
#include <QString>
struct FeatureFlags {
    bool mandi = false;      // paddy, J-Form, I-Form, dalali
    bool mill = false;       // milling batches, yield
    bool bardana = false;    // gunny-bag ledger
    bool transport = false;  // dispatch/gate register
    static FeatureFlags load();   // from app_settings feature.* (default off)
    static void save(const FeatureFlags&);
};
// Firm-setup wizard writes these; Settings dialog toggles them later.
```

### 2. `src/database_manager.cpp` — gate domain DDL
- Wrap each domain `CREATE TABLE` block:
  `if (features.mandi) { /* paddy, jform, iform, sauda, dalali */ }`
  `if (features.mill) { /* milling_* */ }`
  `if (features.bardana) { ... }` `if (features.transport) { gate_register ... }`
- Migrated firms: set flags by detection (`SELECT COUNT(*) FROM <t> > 0`
  before gating — never strand existing data behind an off flag).
- Core tables (parties, groups, stock_*, vouchers, transactions, invoices,
  tds/tcs, audit_log, financial_years, voucher_types) ALWAYS created.

### 3. `src/widgets/main_window.cpp` — gate domain widgets/menus
- Construct + `addWidget` + menu entries for JForm/IForm/milling/bardana/
  gate/sauda/paddy only when the flag is on; indices for core views NEVER shift
  (reserve index bands: core 0-49, mandi 50-69, mill 70-79 — document the band
  map in `menu_tree_manager`).
- `AppKeyboardController` F-key map skips unregistered views gracefully
  (already falls through — verify F10/F11 with flags off).

### 4. Voucher dispatch consolidation (the engine half)
- `src/models/vouchers_model.cpp`: alias groups + save-time chains →
  `canonicalTypeToken` + `VoucherTypeModel::byCode` (Phase-1 API).
- `src/engine/bahi_khata_exporter.cpp`: `normalizeTxType` → keeper + keep only
  the documented Contra/TDS→Jrnl export quirk as post-step.
- `src/engine/migration/voucher_migrator.cpp`: `mapVoucherTypeStr` → keeper;
  display names from `voucher_types.name`.
- `src/models/iform_model.cpp:318`, `jform_model.cpp:319`: delete local
  `prefixRe`, use keeper.
- Rule going forward: voucher behavior branches on
  `VoucherTypeModel::byCode(code)["category"]` (SALES/PURCHASE/PAYMENT/…),
  never on raw type strings.

### 5. Tests
- Flag-off fresh DB: domain tables absent, core suites green, F-keys fall through.
- Flag-on: domain tables + widgets present, mandi tests green.
- Migrated DB: flags auto-detected ON, zero data loss (row counts before/after).

## Verification
- `SELECT name FROM sqlite_master WHERE type='table'` on generic fresh DB
  contains NO paddy/jform/iform/milling/bardana/sauda/dalali tables.
- Grep gate: no `== "JFrm"` / `== "Sale"` style branches outside keeper +
  category dispatch.
