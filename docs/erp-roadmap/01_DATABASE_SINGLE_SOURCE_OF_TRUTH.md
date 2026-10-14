# 01 — Database: Single Source of Truth (v1.0)

## What it should look like (industry standard)

One DDL owner. One canonical table per business concept. One column dictionary.
Tally/Busy parity rule: the *posting* table is truth; documents (invoices) are
projections; masters (ledgers, stock) are referenced by id, never duplicated by name.
Every write goes through a single pipeline function per voucher kind, inside one
SQLite transaction, with FY id + voucher_no assigned atomically.

```
DatabaseManager::ensureTablesExist()   ← ONLY place with CREATE/ALTER/INDEX
        │
        ├─ masters:  parties (=Ledgers), account_groups (=Groups),
        │            stock_items, stock_units, stock_groups
        ├─ postings: vouchers (header) + transactions (lines)  ← TRUTH
        ├─ documents: sales_invoices, purchase_invoices (+items) ← projections
        ├─ movements: stock_transactions (+ inventory = materialised view)
        └─ domain:    milling_batches, gate_register, sauda_contracts, ...
```

Column dictionary (canonical, enforced): `name` on parties (never `party_name`
on that table); `voucher_no TEXT` (never `voucher_number`); `fy_id`
(never `financial_year_id`); `invoice_no` (never `invoice_number`);
`driver_name/transport/broker_name/vehicle_no/gr_no` (never `driver/transport_name`).

## What we currently have (verified)

- All 43 app tables created ONLY in `src/database_manager.cpp:198-2075`.
  No controller/widget/model issues DDL. Indexes + `addColumnIfNotExists`
  migrations also confined there. This is good — keep it.
- Correct collapse of Bahi-Khata trilogy: `Ledgers→parties`,
  `Groups→account_groups`, `Transactions→transactions+vouchers`. Bahi's ~60
  `Temp*` scratch tables correctly NOT replicated.
- `resolveDocumentMetadata` (`database_manager.cpp:2099-2156`) papers over
  header/line ambiguity with COALESCE. Works, but hides the truth question.

## What we lack / bugs (with file:line)

1. **Wrong-column bug (P0, breaks lookups silently):** `parties` has `name`, NOT
   `party_name`, but 7 sites query `WHERE party_name` on it → 0 rows:
   `models/jform_model.cpp:99,109,257,273`, `models/iform_model.cpp:100,228,239,251,263,275`,
   `models/debit_credit_note_controller.cpp:758`.
2. **Exact-duplicate tables:** `paddy_procurement` (:439) vs `paddy_arrivals` (:460)
   — same grain-inward concept, different column names (`receipt_no` vs `slip_no`).
3. **4× stock truth:** `stock_items` (master) vs `inventory` (snapshot) vs
   `stock_transactions` (ledger) vs `custom_closing_stocks`. Migrators write all;
   readers pick interchangeably (`stock_valuation_engine.cpp:114,162,217`).
4. **TDS dual truth:** `tds_vouchers` (:930) vs legacy CamelCase `TDSDeductions`
   (:1589) with one-way backfill only (:1616-1632) → drift.
5. **5× sale/purchase write:** invoice header + items + `vouchers` + `transactions`
   + `stock_transactions`, each hand-crafted per migrator
   (`busy_data_migrator.cpp:1889-1968`, `bahi_khata_migrator.cpp:2078+`). No single
   posting function; `ledger_statement_widget.cpp:624-654` hand-INSERTs with
   divergent columns.
6. **Dead DDL in `src/engine/migration/*`:** `voucher_migrator.cpp:190,224,266,290`,
   `stock_migrator.cpp:116,139`, `ledger_migrator.cpp:210,287,370` reference columns
   that do not exist (`voucher_number`, `is_balanced`, `invoice_number`, `name` on
   stock_items...). Runtime `SQL Prepare Error` if that path ever runs.
7. **Test stubs mask drift:** `tests/test_firm_enter.cpp:19,200-373` creates 2-3-col
   stubs of 30-90-col production tables → tests pass, prod breaks.
8. **No audit log, no views/triggers.** Deletes are hard; no history.

## How to do it (ordered, each verifiable)

1. **P0 — fix wrong column (30 min):** replace all `FROM parties WHERE party_name`
   with `WHERE name` in the 7 sites above. Add test:
   `PRAGMA table_info(parties)` must not contain `party_name`; seed zimidar/labour/
   round-off parties and assert lookup returns id. (Mac compiles, VM `ctest`.)
2. **Freeze DDL ownership (1 h):** add CI step failing on
   `CREATE TABLE|ALTER TABLE` outside `src/database_manager.cpp` and `tests/`.
   Delete `src/engine/migration/*` OR port to canonical columns — no middle state.
3. **Declare posting truth (2 h design + enforce):** `transactions` = truth,
   `vouchers` = header view. Forbid hand-rolled `INSERT INTO vouchers/transactions`
   in widgets (move `ledger_statement_widget.cpp:624-654` into `VoucherPipeline`/
   `VouchersModel`). Grep gate: `INSERT INTO (vouchers|transactions)` allowed only
   in `models/vouchers_model.cpp`, `engine/voucher_pipeline.cpp`, migrators.
4. **Merge duplicates (one PR each):**
   - Drop `paddy_arrivals`, keep `paddy_procurement` (or make one a VIEW).
   - `TDSDeductions` → compat VIEW over `tds_vouchers` (keep CamelCase name for
     Bahi export only).
   - `inventory` → rebuild-from-`stock_transactions` snapshot, never a write target.
   - Unify line-items column contract across `sales/purchase_invoice_items`,
     `jform/iform_voucher_items`, `debit_credit_note_items`, `milling_voucher_items`
     (long-term: single `voucher_line_items`).
5. **Column dictionary doc + heal-block removal:** publish canonical names (§top),
   then delete the `UPPER(TRIM())` + legacy-alias heal at `database_manager.cpp:1807-1893`
   once migrators emit canonical names.
6. **Add `audit_log` table** (see Doc 05) and `migration_log` (source-row hash,
   see Doc 08). Never `DELETE FROM parties/account_groups` in migrators again —
   upsert by `(legacy_id)` instead.
