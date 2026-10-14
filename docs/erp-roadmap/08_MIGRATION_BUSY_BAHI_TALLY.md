# 08 — Migration Forensics: Busy db.bds + Bahi-Khata Data.002 + Tally (v1.0)

## Verified on this VM (2026-10-12, headless Python sqlite3, no Qt)

- 44 tables (`sqlite_master`), `parties` has `name`, NO `party_name` → confirms
  Doc 01 P0 bug (11 queries hit zero rows).
- Migrated Data.002 contents: 2090 parties, 47,401 transactions (Bahi source
  47,263; +138 derived), 18,656 vouchers, 3159 sales + 3278 purchase invoices,
  37 stock items, 48 account groups.
- Trial balance: Dr = Cr = Rs 23,173,587,848.53, diff 0.00. Migration is
  paisa-exact. Script: `C:\Temp\erp-tools\db_check.py` ( rerun any time with
  `Python312-arm64\python.exe`).

## 8.1 Busy `db.bds` (this VM, verified bytes)

- Files: `busy-data/mandi/db.bds` (9,150,464 B), `db12026.bds` (12,578,816 B).
- Magic: `00 01 00 00 "Standard Jet DB"` → **Jet 4.x (Access 2000-2003)**,
  readable by in-tree `third_party/libmdb` + `src/engine/jet4_reader.cpp` +
  `busy_data_migrator.cpp`. No Windows DAO DLL needed on any platform.
- Reference installs on this VM: `C:\BusyWin` (Busy21) + `C:\BusyMagic` for
  behavior comparison (open same company, screenshot voucher/ledger/GSTR flows).
- What Busy schema gives us (via migrator + decompiled specs): voucher-type
  enumeration, multi-rate invoice grouping, GSTR 5-pass reconcile statues
  (Matched / Probable / Tax-mismatch / Date-mismatch / Missing-in-books-portal).

## 8.2 Bahi-Khata `Data.002` (Mac + forensics in repo)

- Lives on Mac (`Bahi-Khata-Data/Data.002`, ~80 MB; `Data.018` ~28 MB).
  VM has: `decompiled-data/Data_002_Schema.sql` (**148 tables**),
  `Extracted_SQL_Queries.sql`, full audit report + GSTR spec, and the migrated
  SQLite `data/mahadev_rice_industry_data_002.db` (31 MB, verified `SQLite format 3`).
- Composition (audited row counts): `SaleTransportationDetail` 71,341 •
  `Transactions` 47,263 • `StockTransactions` 8,957 • `MillingVouchers` 1,656 •
  `VerifiedVouchers` 1,343 • `TDSDeductions` 503 • `TempAankTypeInterest` 481 •
  `ChallanVouchers` 96 • `TDSDeposits` 57 • `CustomClosingStocks` 55 •
  `DebitCreditNotes` 7 • `BankAccounts` 3 (cheque-leaf coords) •
  `InterestSettings` 3 (grace/drop/360-vs-365).
- Core trilogy mapped: `Ledgers→parties`, `Groups→account_groups`,
  `Transactions(RowNo,VoucherNumber,VoucherDate,TransType,AccountCode,DrCr,Amount)→
  transactions+vouchers`. Per-commodity voucher explosion (`Cotton/Powder/Timber/
  RiceBran/...`) and ~60 `Temp*` tables are **deliberately NOT copied** (see Doc 01).
- Bridge tooling (Mac): `bahi_khata_engine/` (Jackcess/UCanAccess JDBC, snapshot
  + rollback + Dr=Cr guard) + `TOOL-USAGE.md` CLI (`audit`, `reconcile closing-stock`,
  `snapshot create/list/rollback`, `query`). Use on Mac for forensics; VM uses the
  C++ `jet4_reader` path.

## 8.3 Tally samples

- `tally-data/`: 11 sample dirs + 12 zips (ledger rows-vs-columns, GST columns,
  stock formats, e-way/e-invoice) consumed by `tally_data_migrator.cpp`.
  P&L/BS already tested on `self-data` (`225000` fixture).

## What migrators currently do (and must stop doing)

- Today: mass `DELETE FROM transactions/vouchers/.../parties/account_groups/...`
  then re-INSERT in one transaction (destructive, idempotent re-run, no audit).
  Preserves `legacy_id` + canonical `voucher_no` + byte-identical invoice strings
  (exporter round-trip asserts Jet4 `0x14==1`, `tableCount>10`). No source hash,
  no `migration_log`, TDS recomputed not carried.

## How to do it (one-click importer spec)

1. **Dry-run first:** `inspect_*` (already non-mutating) → report
   (tables, rows, `isBalanced`, `glDiscrepancy<0.01`) → user confirms → migrate.
2. **Upsert, never wipe:** match by `(legacy_id)` for masters, `(voucher_no, fy_id)`
   for vouchers; write `migration_log(source, hash, rows, user, ts)`.
3. **Post-migration proof screen:** trial-balance zero, GSTR-1↔3B tie-out,
   stock closing vs `CustomClosingStocks`, checksum vs source counts.
4. **Compat export:** Bahi Jet4 write-back stays byte-identical (keep `jet4_writer`
   + `TDSDeductions` compat view); Busy/Tally need no write-back (import-only).
5. **VM role:** run `EnginesTestSuite` migrator tests headless against the two
   `.bds` files + zipped Tally samples; Mac runs the Java bridge audit on `Data.002`.
