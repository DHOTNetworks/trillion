# Phase 1: Voucher Type Master System

## Goal
Replace all hardcoded voucher-type strings (`Sale/Purc/Pymt/Rcpt/Jrnl/ChPt/ChRt/JFrm/IFrm/TDS/TCS/Mill`)
with a data-driven `voucher_types` master table, exactly like Tally Prime's Voucher Type masters.

## Current State (Verified)
- ~100 hardcoded voucher-type string literals across `src/models`, `src/widgets`,
  `src/engine` (incl. `bahi_khata_exporter.cpp:208-221` normalizer,
  `bahi_khata_migrator.cpp:162,224`, `master_data_provider.cpp:284-312` fallbacks).
- `FiscalYearHelper::canonicalTypeToken` exists but is only used by migrators.
- `voucher_pipeline.cpp:196` lists types inline.
- No user-defined voucher types possible; adding one means editing 50+ places.

## Target State
- `voucher_types` table is the single source of truth for every voucher kind.
- System core types seeded once (undeletable); mandi/manufacturing types seeded
  only when their feature flag is on; users can add custom types
  (Delivery Note, Stock Transfer, Job Work In/Out, …).
- All code looks types up through `VoucherTypeModel` / `FiscalYearHelper::canonicalTypeToken`.

## Exact Edits

### 1. NEW `src/models/voucher_type_model.h` + `.cpp`
```cpp
// VoucherTypeModel: CRUD + cache for voucher_types. Singleton access via
// VoucherTypeModel::instance(). Seed-on-first-use for core types.
struct VoucherTypeDef {
    QString code;            // SALE, PURC, PYMT, RCPT, JRNL, CONTRA, ...
    QString name;            // Sales Invoice, Purchase Bill, ...
    QString category;        // SALES | PURCHASE | PAYMENT | RECEIPT | JOURNAL | CONTRA | OTHER
    bool isSystem = false;   // core types: rename ok, delete blocked
    QString prefix;          // SAL-, PUR-, PAY-, REC-, JNL-
    QString numberingMethod; // AUTO | MANUAL
    QString separator;       // '-' or '/'
    int numberWidth = 4;
    bool resetEachFy = true;
    bool affectsStock = false;
    bool requiresParty = true;
    bool requiresStock = false;
    bool requiresGst = false;
    int sortOrder = 0;
    bool isActive = true;
};
class VoucherTypeModel : public BaseTableModel {
    Q_OBJECT
public:
    static VoucherTypeModel& instance();
    void ensureSeeded();                       // core types if table empty
    void ensureDomainTypes(const QString& domain); // JFrm/IFrm/Mill when flag on
    QVariantList activeTypes() const;
    QVariantMap byCode(const QString& code) const;
    bool isDeletable(const QString& code) const;   // false for is_system
};
```

### 2. `src/database_manager.cpp` — new table + core seed (runs always)
```sql
CREATE TABLE IF NOT EXISTS voucher_types (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    code TEXT UNIQUE NOT NULL,
    name TEXT NOT NULL,
    category TEXT NOT NULL DEFAULT 'OTHER',
    is_system INTEGER DEFAULT 0,
    prefix TEXT DEFAULT '',
    numbering_method TEXT DEFAULT 'AUTO',
    separator TEXT DEFAULT '-',
    number_width INTEGER DEFAULT 4,
    reset_each_fy INTEGER DEFAULT 1,
    affects_stock INTEGER DEFAULT 0,
    requires_party INTEGER DEFAULT 1,
    requires_stock INTEGER DEFAULT 0,
    requires_gst INTEGER DEFAULT 0,
    sort_order INTEGER DEFAULT 0,
    is_active INTEGER DEFAULT 1
);
-- Core seed (Tally-compatible codes; legacy short tokens kept as aliases
-- in canonicalTypeToken, never as separate types):
-- SALE/Sales Invoice/SALES, PURC/Purchase Bill/PURCHASE,
-- PYMT/Payment/PAYMENT, RCPT/Receipt/RECEIPT, JRNL/Journal/JOURNAL,
-- CONTRA/Contra/CONTRA, CHPT/Cheque Payment/PAYMENT, CHRT/Cheque Receipt/RECEIPT
```

### 3. `src/services/master_data_provider.cpp:284-312` — delete hardcoded fallbacks
- `getAvailableVoucherTypes()` / `getVoucherTypeCodes()` read `voucher_types`
  (no static Sale/Purc/… lists); empty table = empty result, never fake data.

### 4. `src/engine/bahi_khata_exporter.cpp:208-221` — route `normalizeTxType`
- Through `FiscalYearHelper::canonicalTypeToken`; keep ONLY the export-specific
  quirk (Contra/TDS→Jrnl) as a documented post-step, not a parallel map.

### 5. `src/engine/migration/voucher_migrator.cpp:46-62` — route `mapVoucherTypeStr`
- Through `canonicalTypeToken`; delete the display-name parallel map
  (display names come from `voucher_types.name`).

### 6. `src/models/vouchers_model.cpp:34-48,903-918` — alias groups
- Replace alias if-chains with `canonicalTypeToken` + `VoucherTypeModel::byCode`.

### 7. `src/models/iform_model.cpp:318`, `jform_model.cpp:319` — delete local prefixRe
- `canonicalTypeToken` already covers JFrm/IFrm.

### 8. `src/engine/voucher_pipeline.cpp:196` — types from model, not literal list.

### 9. `src/widgets/*voucher_widget.cpp` mode strings
- `"Cash Payment"/"Cheque Payment"/...` literals stay as UI labels, but every
  comparison/storage path converts once via `canonicalTypeToken` at the
  widget→controller boundary.

## Verification
- New DB boots with exactly the 8 core types, all `is_system=1`.
- `SELECT COUNT(*) FROM voucher_types` in LogicBoard suite + round-trip:
  custom type insert → visible in type pickers → deletable (non-system) works.
- Grep gate: fail CI on new `"Sale"|"Purc"|"Jrnl"` literals outside
  `canonicalTypeToken` + seed blocks.
