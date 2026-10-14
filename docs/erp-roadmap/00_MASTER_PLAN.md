# MahadevAC → Industry-Grade ERP: Master Plan (v1.0, 2026-10-12)

Target: beat Tally Prime + Busy 21 for commission agents, mandi arhtiyas,
rice / cotton / oil mills. Generic for anyone on Tally/Busy today.
Inspiration (not clone): Bahi-Khata `Data.002` (Jet DB, 148 tables, ~80 MB),
Busy `db.bds` (Standard Jet DB, verified 9.1 MB + 12.5 MB in `busy-data/mandi/`).

## Verified facts (Windows VM, this session)

- `db.bds` magic = `Standard Jet DB` (Jet 4.x). Readable by in-tree
  `third_party/libmdb` + `src/engine/jet4_reader.cpp`. No Windows DLL needed.
- `data/mahadev_rice_industry_data_002.db` = SQLite 31 MB (migrated Data.002).
  `Data.002` itself lives on the Mac; VM has `decompiled-data/Data_002_Schema.sql`.
- App DDL owner is single: `DatabaseManager::ensureTablesExist()`
  (`src/database_manager.cpp:198-2075`, 43 tables). No widget/controller issues DDL.
- UI system exists: `docs/QTWIDGETS_DESIGN_SYSTEM.md` (4-tier register, navy
  `#0F172A`, keyboard-first). Compliance ≈ 5.5/10, worst = sales/purchase vouchers.
- Engines: GSTR-1/3B + mandi calculator + bill series are production-grade;
  money = `double` everywhere (no paise int64), FY lock unenforced, no audit log.
- Toolchain on this VM: bare (no Qt/CMake/MSVC). Download stage started in
  background (`C:\Temp\erp-tools`). Mac remains the compile machine until
  MSVC ARM64 + Qt `win64_msvc2022_arm64` land here.

## Document map (`docs/erp-roadmap/`)

| # | File | Question answered |
|---|------|-------------------|
| 0 | `00_MASTER_PLAN` (this) | Where are we, what order do we fix |
| 1 | `01_DATABASE_SINGLE_SOURCE_OF_TRUTH` | Kill duplicate tables, one DDL owner |
| 2 | `02_UI_ALIGNMENT_DESIGN_SYSTEM` | Fix verbose/tacky misalignment |
| 3 | `03_KEYBOARD_FIRST_WORKFLOW` | Tally-grade keyboard determinism |
| 4 | `04_DYNAMIC_MASTERS_NO_HARDCODING` | DB-driven, zero hardcoded firm/tax |
| 5 | `05_DETERMINISTIC_ENGINES` | Paise math, FY locks, audit trail |
| 6 | `06_MANDI_COMMISSION_VERTICAL` | Arhtiya/dalali/J-Form/I-Form moat |
| 7 | `07_MANUFACTURING_VERTICAL` | Rice/cotton/oil yield engines |
| 8 | `08_MIGRATION_BUSY_BAHI_TALLY` | db.bds + Data.002 analysis & importers |
| 9 | `09_WINDOWS_VM_TOOLCHAIN` | Headless Qt test setup for this VM |

## Execution order (4 phases, dual-machine)

**Phase A — Determinism foundation (week 1-2, both machines)**
1. Doc 01: fix `WHERE party_name` bug (7 files), delete/port `src/engine/migration/*`
   dead DDL, add CI grep gate (DDL only in `database_manager.cpp`).
2. Doc 05: centralize `round2` → `FinancialMathService`, fix CGST/SGST 1p split bug.
3. Doc 04: route GSTIN placeholder + tax lists through `MasterDataProvider`.
4. VM: finish toolchain (Doc 09), run `LogicBoardTestSuite` + `EnginesTestSuite`
   headless (`QT_QPA_PLATFORM=offscreen`).

**Phase B — Input trust (week 3-4, Mac compiles, VM tests)**
5. Doc 03: reconcile F5/F6/F11/Ctrl+E/Del bindings, kill `GlobalKeyFilter` dupe,
   bind `KbdBadgeButton`→`QShortcut`.
6. Doc 02: centralize QSS constants, remove fixed-px in vouchers, add
   `setForeground` to voucher grids (dark-mode bug).
7. Doc 05: enforce FY lock on write paths, add `audit_log` table.

**Phase C — Market moat (week 5-8)**
8. Doc 06: mandi vertical (Aank engine posting, 194Q>206C(1H), BRS recon).
9. Doc 07: cotton/oil yield engines, FIFO/WAC valuation, multi-godown.
10. Doc 08: one-click Busy/Tally/Bahi importers with dry-run + checksums.

**Phase D — Launch hardening**
11. Voucher gap detection, edit-log UI, role permissions, e-invoice/e-way,
    FY rollover wizard, performance pass (virtual scroll, indexes).

## Non-goals (explicit)

- Not a Bahi-Khata clone: no per-commodity voucher explosion, no `Temp*` tables,
  no CamelCase legacy names in new code (`TDSDeductions` stays as compat view only).
- No QML: stack is QtWidgets C++ (per SCRATCH.md decision). No new QML files.
- No second DDL owner, ever. CI fails the build otherwise.

## Status audit (2026-10-13, grep-verified — see each doc for detail)

| Doc | State | Evidence |
|---|---|---|
| 01 DB truth | P0 DONE, rest OPEN | `WHERE party_name`→0 hits; OPEN: 9 hand-`INSERT` sites in widgets, `TDSDeductions` 21 refs, `paddy_arrivals` 9 refs |
| 02 UI system | registers + sale/purchase vouchers DONE | T1-T4 builders, 0 `setFixed`/legacy tokens in trial/sales/purchase; OPEN: gray headers ~40 files, ledger single-table |
| 03 keyboard | core DONE | `GlobalKeyFilter` 0 refs, `keymap.h` truth, F11/Alt+F1/F1 wired; OPEN: `test_keyboard_flow.cpp` |
| 04 no-hardcode | OPEN | provider in 3/40+ widgets; 7 firm-literal hits remain |
| 05 engines | core DONE | one `round2`, 1p split, FY `writeBlockReason` ×6, `audit_log` wired; OPEN: void-delete, paise dual-write, tie-out gates |
| 06 mandi | OPEN | interest-post loop, 194Q/206C view, BRS, credit block |
| 07 mfg | OPEN | templates, apportionment, FIFO/WAC, godowns, landed cost |
| 08 migration | OPEN | upsert-not-wipe, `migration_log`, proof screen |
| 09 toolchain | DONE (Mac-only decision) | VM headless-lite, no Qt/MSVC |
| 10 query/index | batch DONE | 14 indexes, N+1s fixed; 8 ranked items open |
| 10 UI overhaul | statements + contract DONE; print split DONE | 28/28 focus/Enter/Esc/dirty; controller 3922→~1000 lines, 27 reports × (report+excel files), trial CA stationery |
| 11 dedupe | keepers DONE | formatINR/party-lookup/type-token/round2 migrations queued |

Recommended next: Doc 01 correctness leftovers (widget INSERTs → `VouchersModel`)
→ Doc 06 interest-post + 194Q/206C view (foundation ready) → Doc 04 provider
wiring → Doc 02 leftovers → Doc 05 P2 → Doc 07/08.

## Dual-machine protocol

- Mac (M1): full app compile + visual UI verification.
- Windows VM (this): headless `ctest` (`offscreen`), Python analyzers,
  Jet DB inspection, Busy21/TallyPrime behavior reference (both installed at
  `C:\BusyWin`, `C:\BusyMagic`, `C:\Program Files\TallyPrime`).
