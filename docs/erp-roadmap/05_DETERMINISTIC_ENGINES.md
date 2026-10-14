# 05 — Deterministic Accounting Engines (v1.0)

## What it should look like (Tally guarantee)

Money = int64 paise end-to-end. `round2` defined ONCE. Every posting asserts
`ΣDr == ΣCr` per voucher AND trial-balance `difference < 1paise` globally.
FY lock blocks writes. Voucher numbers gapless-per-series with UNIQUE guard.
Every mutation leaves an audit row (who/when/before/after). GSTR-1↔3B tie-out
is a release gate, not a test nicety.

## What we currently have (maturity)

| Engine | State | Note |
|---|---|---|
| GSTR-1 + Excel writer | Production | B2B/B2CL/B2CS/CDNR/HSN, byte-identical rewrite test |
| GSTR-3B | Production | 3.1/3.2/4/5/6.1, 3.2 derived from GSTR-1 payload |
| GST tax engine | Prod-ish | Split + TCS-206C(1H) + RCM; only 13-state map |
| Mandi calculator | Prod (domain) | Gross/tare/moisture/net + dami/mkt/HRDF/RDF/labour, per-charge round-off switches; exact fixture `461330` |
| Bill series | Production | JSON config, adopt-from-history, `nextNumber/validate` |
| FY helper | Prod partition / POC lock | Zero-leakage partition proven; `isLocked` read, never enforced |
| Balance-sheet / P&L | Prod-ish | Hooks for opening/closing stock; tested on Tally self-data `225000` |
| GSTR-2B reconciler | Prod-ish | 5-pass, tol ₹1, 30d window, 6m lookback |
| Stock valuation | Prod-ish | BUT latest-rate fallback only — no FIFO/WAC |
| Aank interest | Mid | `/36500`, leap opt, tested `3287.67` |
| Depreciation | Mid | IT-WDV + Sec-32 half-year; no Co. SLM |
| TDS FVU exporter | Format-prod | `^FH^/^BH^/^CD^/^DD^`; no challan↔deductee cross-total assert |
| GSTR-9 | POC | Structs only, no numeric tie-out |
| Milling yield | Thin POC | Pure % math, no cost apportionment |
| Voucher pipeline | Legacy, avoid | Global MAX numbering, collision-prone key |
| Tests | 2478L logicboard + 643L engines | Exact QCOMPAREs, byte-determinism, FY zero-leakage — but never assert `isBalanced==true`, never test lock/concurrency/audit |

## What we lack (determinism holes)

1. **All-double money:** every engine `double amount`; only "paise" hit is
   amount-in-words. Three duplicate `round2`s (service + 2 file-static);
   engines bypass `FinancialMathService`.
2. **1p split bug:** `GstTaxEngine::computeTax` rounds halves independently
   (`100.01×5%` drifts); `FinancialMathService::calculateGst` is sum-preserving.
   Pick ONE (the latter).
3. **Numbering not atomic:** `BillSeriesManager::nextForConfig`
   (`bill_series.cpp:228`) = SELECT-then-render, no transaction/UNIQUE;
   `VoucherPipeline::getNextVoucherNumber` = global MAX ignoring fyId.
4. **FY lock = label:** `financial_years.is_locked` displayed, zero write-guards.
5. **No audit:** grep `audit_log|voucher_log|edit_log` = 0. Hard DELETEs, no void.
6. **Migrators destructive:** mass DELETE+re-INSERT, no source hash, no log.

## Status (implemented 2026-10-12, Mac must rebuild + run both suites)

- One rounding: `FinancialMathService::round2/round3/calculateGrandTotal/
  calculateRoundOff` is the single implementation. Removed file-static dupes in
  `gst_tax_engine.cpp`, `milling_yield_engine.cpp`, `bahi_khata_exporter.cpp`
  (`round2dbl`); `MandiCalculator::round2/roundToRupee` and
  `BahiKhataExporter::round2` kept as 1-line delegating APIs (no caller churn);
  `AccountingEngine`, `AankInterestEngine`, `StockValuationEngine` routed to it.
  Left as-is deliberately: migrator one-shot import math (Bahi byte-fidelity),
  integer cents/bags math, generic `data_preprocessor` precision helper.
- 1p split fixed in `GstTaxEngine::computeTax`: total rounded once, second half
  absorbs odd paise (matches `calculateGst`).
- FY lock enforced: `FiscalYearHelper::writeBlockReason(date)` (" " = writable);
  guards in `VouchersModel::save_multi_row_voucher` + `delete_voucher`,
  `DepreciationCalculator::postDepreciationToBooks` (via outError),
  sales + purchase `saveVoucher` (via status message). Locked/invalid dates
  return before `beginTransaction`; DB untouched.
- Tests: `testGstOddPaiseSplitPreservesTotal` (engines; invariant-based, float-
  simulated bit-exact incl. proof old code drifts 16.66 vs 16.67),
  `testFyWriteLockGuard` (logicboard; seeded locked/open FYs + writer refusal,
  BLOCKED + INSERT audit rows, hermetic cleanup).
- Audit trail: `audit_log` table (DDL in `ensureTablesExist`, migrates via
  IF NOT EXISTS) + `DatabaseManager::logAudit` (best-effort, in-caller-txn);
  wired to voucher INSERT/UPDATE/DELETE/BLOCKED in `VouchersModel`.
  Actor from `audit_actor` setting (default `local`).
- Atomic numbering: `DatabaseManager::lastErrorText()` (capture-before-rollback
  discipline); UNIQUE `uq_sales_fy_invoice` (own bills) + `uq_vouchers_type_fy_no`
  with legacy-dupe fallback (plain index + `numbering_repair_needed=1` + repair
  report; self-heals to UNIQUE once clean). Purchase `invoice_no` deliberately
  NOT unique (supplier bills repeat legitimately — verified 200 groups).
- `VouchersModel::claimVoucherNo` (fresh MAX + existence check, 8 tries) used by
  all 4 voucher writers + sales reclaim loops; header/legs failures now roll
  back with explicit `lastErrorText` instead of committing partial postings.
- `VoucherNumbering` report header: `findDuplicateVoucherNos` (live data: 19
  legacy groups incl. `2425/Pymt-31..35` double-posts) + `findInvoiceGaps[ForConfig]`.
- Tests: `testVoucherNumberingAtomicityAndReports` (claim distinctness, dupe
  report, TST/1,2,4→TST/3 gap, UNIQUE refusal + error text, hermetic cleanup
  incl. guard restore).
- Trial print N+1 fixed (`renderTrialBalanceHtml`: 4000+ scalar SUMs → one
  GROUP BY; verified bit-exact on 2090 parties via headless reimplementation).
- Audit wiring extended to invoice tables: sales/purchase add/update/delete all
  write INSERT/UPDATE/DELETE rows with identical `AuditJson::invoice` payloads
  (identity column = bill number, searchable); shared `audit_helper.h` replaces
  the voucher model's local builder. Repair dialog (`NumberingRepairDialog`,
  from audit register header): lists dupe groups, renumbers extras to claimed
  numbers across vouchers+transactions+invoice/stock projections in one txn per
  row, audits every renumber, clears `numbering_repair_needed` when clean.
- Tests: `testInvoiceAuditWiring` (end-to-end sale+purchase save→audit→delete→
  audit, hermetic).
- Memory guards (45GB render blowup): `renderDayBookHtml` capped at 5000 detail
  legs with full-range SQL totals + truncation notice; daybook grid capped at
  10000 newest-first rows with slim projection + footer window notice (totals
  follow the filter state explicitly). Transport sync made idempotent
  (voucher-identity guard; NULL-invoice dupe growth stopped).
- Dashboard: `sales_vouchers` ghost-table fallback fixed (→ `sales_invoices`);
  Audit Trail quick widget (live counts + jump to view 64) replaces the
  reserved placeholder.
- Tests: `testTransportSyncIdempotent` (double + triple reload, zero growth).
- Still open: same cap pattern for ledger/registers/stock prints (next).

Still open: void-instead-of-delete, atomic numbering UNIQUE
guard, paise dual-write, bank-posting guard.

## How to do it — remaining
1. **Wider audit wiring (viewer DONE):** `AuditRegisterWidget` (view 64, Books &
   Registers item 10, ErpTheme 4-tier, Esc/F5/Ctrl+F, before/after JSON sidecar).
   Next: route sales/purchase invoice writes + master (party/stock) mutations
   through `logAudit`; set `audit_actor` per login when user roles land.
2. **Numbering repair UI:** surface `numbering_repair_needed=1` + the 19 legacy
   dupe groups in a repair view (merge/renumber with audit rows, never delete
   money); clears the flag when `findDuplicateVoucherNos` is empty.
3. **P1 — void-instead-of-delete:** reversal entry for voucher deletes, original
   preserved; surfaced in the audit register.
4. **P2 — paise migration:** new `amount_paise INTEGER` columns alongside `REAL`,
   dual-write one release, assert `abs(real*100-paise)<1` in tests, then cut over.
   (Full int64 rewrite is Phase D; dual-write de-risks.)
5. **P2 — tie-out gates:** GSTR-1↔3B, trial-balance `isBalanced==true`, GL
   `ΣDr==ΣCr` exact — all as `QCOMPARE`, all release-blocking in `ctest`.

## TCS 206C(1H) sale posting (2026-10-13, verified vs live Data.002)
- Bahi-Khata model (live reads): VoucherSettings ApplyTCS=1/ApplyTCSInSale=1,
  rates 0.1/1.0, SaleTCSLedger=1729; base legs carry TCSTaxable+TCSRate;
  TCS posts as separate EntryType='TCS' Cr leg to the TCS ledger on the SAME
  voucher number; customer Dr INCLUDES TCS (voucher 144: Dr 2629627 =
  goods 2627000 + TCS 2627; Cr goods + Cr TCS).
- Bill MRI/2526-1089 (= Sale voucher 1092, 2026-03-28, Krish Cereals):
  taxable 1628875 x 0.1pct = 1628.88 posted amount; migrated books hold the
  header field only (no TCS legs) - new postings now write the full entry.
- Our shape mirrors it: Leg 1 Dr Customer = total incl TCS; Leg 7 Cr
  'TCS Payable A/c' = tcs_amount (entry_type TCS206C, taxable/tcs columns);
  ledger resolved via tcs_payable_ledger_id setting, else 'TCS Payable A/c'
  name, else legacy 948.
- Sale voucher TCS dialog mirrors Purchase TDS dialog (Calc button, F7/Alt+D,
  Enter-chain); turnover rows hidden by design, members retained.

## TCS resolver hardening (same day)
- Live-data find: legacy_id 948 in the migrated DB belongs to 'Sushil Trading
  Company [Sirsa]' (Sundry Debtors), NOT a TCS ledger. A code-only fallback
  would post TCS against a random customer, so resolveTcs206CLedger matches
  by canonical name only (setting id -> 'TCS Payable A/c' -> auto-create
  with legacy 948); never by code alone. (Purchase 2176 verified safe here -
  genuinely 'T.D.S. U/S 194-Q' - but same fragility class, flagged.)
- Canonical name confirmed 'TCS Payable A/c' (decompiled rename:
  Update Ledgers Set LedgerName='TCS Payable A/c' Where LedgerName=
  'TCS (Tax Collected at Source)'). Firm variants seen live: 'Tcs received
  2020-21', 'Tds & Tcs 2020-21' (Data.018) - hence name-match-then-create.
