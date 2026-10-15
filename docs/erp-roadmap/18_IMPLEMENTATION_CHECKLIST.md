# Implementation Checklist (Generic-ERP Pivot)

Order matters: each phase is independently shippable and verified before next.

## Phase 1 — Voucher Type Master (Doc 13)
- [ ] `voucher_types` table + core seed in `ensureTablesExist`
- [ ] `VoucherTypeModel` (CRUD, cache, delete guard)
- [ ] Route: exporter `normalizeTxType`, migrator `mapVoucherTypeStr`,
      `vouchers_model` alias chains, iform/jform `prefixRe`, pipeline list
- [ ] `master_data_provider` fallbacks read the table (no static lists)
- [ ] Tests: seed count, custom-type round-trip, CI grep gate on literals
- [ ] Suites green on Mac (`LogicBoardTestSuite`, `EnginesTestSuite`)

## Phase 2 — Generic Groups (Doc 14)
- [ ] Replace group seed block (generic 13-primary chart, no mandi/mill names)
- [ ] `ensureDomainGroups(domain)` idempotent helper
- [ ] Trim default parties to system six; repoint classifier/pipeline old names
- [ ] EnginesTestSuite totals unchanged (no rupee moves groups)

## Phase 3 — Flags + Engine (Doc 15)
- [ ] `feature_flags.h` + `app_settings feature.*` + setup-wizard wiring
- [ ] Gate domain DDL with migrated-data auto-detection (never strand data)
- [ ] Gate domain widgets/menus; reserve index bands; F-key fall-through check
- [ ] Category-based dispatch replaces raw-string branches
- [ ] Fresh-DB table inventory has zero domain tables; migrated DB lossless

## Migration & Data (Doc 16)
- [ ] Copy-on-read discipline documented + enforced (bridge opens copies only)
- [ ] `migration_log` per run with proofs (trial 0.00, GSTR tie-out)
- [ ] Name-match-then-create rule for canonical ledgers (no code-guessing)

## Schema (Doc 17)
- [ ] Merge `paddy_arrivals` → `paddy_procurement`; `inventory` snapshot-only
- [ ] `TDSDeductions` as compat view; unify line-item contract
- [ ] CI gates: DDL owner, column dictionary, display format, no fake seeds

## Per-Phase Verification (VM headless, no Qt needed)
- [ ] `db_check.py` (schema/table inventory), `dupe_check.py` (numbering),
      `trial_check.py` (trial equivalence), `tcs_verify.py` (posting math),
      `ddl_time.py` (migration cost), `dedupe_*.py` (repair safety)
- [ ] Brace-balance on every edited file
- [ ] Mac: full rebuild + both suites + visual pass per phase
