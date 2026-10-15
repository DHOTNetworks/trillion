# Database Targeting & Migration Mapping

## Goal
One document that says, for each source database: where it lives, how we read
it, what maps to what, and what we deliberately do NOT copy.

## Sources & Access

| Source | Location | Format | Reader | Safety rule |
|---|---|---|---|---|
| Bahi-Khata live | `Z:\MahadevAc\Bahi-Khata-Data\Data.002` (~80 MB) + `.018/.004/.007` | Jet 4.x MDB | `bahi_khata_engine/bridge.py` → UCanAccess (`tools/ucanaccess/*`, JDK 21 at `C:\Tools\jdk-21.0.12.1+1`) via `C:\Temp\erp-tools\jetq.py` | COPY-ON-READ: always copy to scratch first (`Data.002.copy` pattern); live file never opened for write |
| Busy | `busy-data/mandi/db.bds`, `db12026.bds` | Jet (Busy proprietary layout) | `src/engine/busy_data_migrator.cpp` + in-tree libmdb | Same copy-on-read discipline |
| Tally samples | `tally-data/*.zip` + sample dirs | XML/Excel/Default-template exports | `src/engine/tally_data_migrator.cpp` | Read-only by construction |
| Migrated SQLite | `data/mahadev_rice_industry_data_002.db` (~31 MB) | SQLite | stdlib `sqlite3` / app `DatabaseManager` | This is the dev/test target, freely writable |

## Canonical Concept Map (source → our schema)

| Concept | Bahi-Khata (Data.002) | Busy (.bds) | Tally | Ours (SQLite) |
|---|---|---|---|---|
| Ledger master | `Ledgers` (`LedgerName/Code1st/GroupCode`, per-firm names) | Ledger masters | Ledgers | `parties` (`name`, canonical groups) |
| Groups | `Groups` | Groups | Groups (13 primary) | `account_groups` (Phase 2 generic chart) |
| Postings | `Transactions` (`VoucherNumber/Date/TransType/AccountCode/DrCr/Amount`, `TCSTaxable/TCSRate`, `EntryType`) | Vouchers | Vouchers | `vouchers` (header) + `transactions` (legs) |
| Sale bill | Sale `Transactions` + stock rows | Sale vouchers | Sales vouchers | `sales_invoices` + `sales_invoice_items` + legs |
| TCS config | `VoucherSettings` (`ApplyTCS/InSale/InPurc`, rates, `SaleTCSLedger=1729`) | — | TCS masters | `app_settings` (`tcs_*`) + `TCS Payable A/c` (948) |
| TCS posting | `EntryType='TCS'` Cr leg, same voucher no; Dr incl TCS | — | TCS additions | Same shape (`entry_type='TCS206C'`) |
| TDS 194Q | `EntryType='TDS194Q'`, `TDSDeductions`, `TDSDeposits` | TDS | TDS | `tcs_amount/tds_*` cols + `tds_vouchers` + `TDSDeductions` compat |
| Stock master | `StockItems` | Stock items | Stock items | `stock_items` + `stock_groups` + `stock_units` |
| Stock movement | `StockTransactions`, `StockGodowns` | Stock journals | Stock vouchers | `stock_transactions` |
| Mandi forms | `JFrm/IFrm` settings + Sauda | (Busy mandi DLLs) | — | `jform/iform/sauda/dalali` (flag-gated, Phase 3) |
| Transport | `SaleTransportationDetail` (71k rows) | Dispatch | — | `transport_dispatches` + `gate_register` (flag-gated) |
| Interest | `InterestSettings`, `TempAankTypeInterest` | Interest | Interest | `interest_model` + Aank engine |

## Field-Level Rules (verified examples)
- **Identity**: our `voucher_no` canonical `2627/Sale-1092` ⟷ Bahi-Khata bare `1092`
  + `TransType` + date (MRI/2526-1089 case: invoice_no embeds FY-series).
- **Money**: Bahi-Khata stores `TCSTaxable=1628875/TCSRate=0.1`, posts 1628.88
  (= round-half-away). Our engine must reproduce bit-exact (verified).
- **Names**: Bahi-Khata renames firm TCS ledgers to canonical
  (`'TCS (Tax Collected at Source)'` → `'TCS Payable A/c'`); we match by
  canonical name, then create, never by legacy code alone (948 collides —
  verified against live data).
- **Dates**: Bahi-Khata `VoucherDate` carries `.0` millis + mixed formats;
  normalize on READ via `AccountingDateService::toIso`, store ISO, display
  `dd-MM-yyyy` via `FiscalYearHelper::formatDisplayDate`.

## Deliberately NOT Migrated
- ~60 `Temp*` scratch tables (report temp state, never truth).
- Per-commodity voucher explosion (`Cotton/Powder/Timber/RiceBran/...`
  tables) — unified into generic voucher + item rows.
- `VerifiedVouchers` locks → replaced by `audit_log` + FY `is_locked`.
- Firm-specific ledger names as canonical (e.g. never seed firm names).

## Workflow (per migration run)
1. Copy source to scratch. 2. Run `inspect_*` dry-run (counts, trial-balance
   zero, discrepancy < 0.01). 3. Migrate into clean app DB. 4. Post proofs:
   trial diff 0.00, GSTR-1↔3B tie-out, stock closing vs source. 5. Record
   `migration_log` (source hash, rows, user, ts).
