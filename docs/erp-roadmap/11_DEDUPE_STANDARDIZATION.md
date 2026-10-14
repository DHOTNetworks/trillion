# 11 — Dedupe & Standardization (2026-10-12, full-app)

## Keepers (single implementations — use these, nothing else)

| Concept | Keeper |
|---|---|
| Money rounding | `FinancialMathService::round2/round3/calculateRoundOff/calculateGrandTotal` |
| GST state codes | `GstStateCodes::{stateForCode,codeForState,stateForGstin}` |
| Display dates | `FiscalYearHelper::formatDisplayDate` (→ `AccountingDateService`) |
| Parse → ISO | `AccountingDateService::toIso` / `FiscalYearHelper::normalizeToIso` |
| FY resolve | `FiscalYearHelper::{resolveFiscalYear,getFiscalYearForDate,getActiveFiscalYear}` |
| Voucher-no canonical | `FiscalYearHelper::{canonicalVoucherNo,parseCanonicalVoucherNo,fyShortToken}` |
| Voucher-type token | `FiscalYearHelper::canonicalTypeToken` |
| Theme | `ErpTheme::*` |
| Keymap | `ErpKeymap::bindings()` |
| Numbering reports | `VoucherNumbering::{findDuplicateVoucherNos,findInvoiceGapsForConfig}` |

## Done this batch

- `formatDisplayDate` ×3 deleted (sales/purchase register models, print controller
  53 calls) → keeper. `formatPlainINR` deleted (0 callers).
- Widget feedback audit: all loops provably terminate (guards/`textEdited`/
  blockers); added missing blockers (aank spin init); #5 tax-challan ping-pong
  noted for guard.

## Queued (effort)

- **M — `formatINR` ×4 → `formatInr` (~150 calls):** behavioural (truncate vs
  round, `₹` vs `₹ `). Needs Mac snapshot verification per surface. Do NOT
  batch-blindly; migrate per-widget with screenshots.
- **M — party lookup ×7 shapes → `PartiesModel::resolvePartyId`** (exact NOCASE
  → alias → LIKE + spaceless/legacy fallback) + `parties(name)` index (added).
- **M — voucher-type if-chains → `canonicalTypeToken`** (save-time chains,
  get_next aliases, exporter/migrator maps; watch Payment vs Pymt display/token).
- **M — migrator rounding sites (~12) → `round2`** (incl. `:1874` `/100` bug).
- **M — metric-card/filter-bar/table-style/badge-header/footer-export builders
  → `ErpTheme` builders** (~18+14+20+25+10 copies across widgets).
- **S — remaining inline currency `std::round` in controllers** (tcs/sauda/salary/
  interest/transport/paddy/tds/jform/stock) → `round2`/`calculateGrandTotal`.
- **S — sales/purchase `OR voucher_no` + `deleteVoucherLegs` + `findInvoiceId`
  shared helpers** in `DatabaseManager`.
