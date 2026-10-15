# Complete Schema Architecture (Target)

## Layering
```
firm DB (one SQLite file per firm)
├── A. CORE — always present, generic (Tally-parity)
├── B. COMPLIANCE — always present (GST/TDS/TCS/audit)
└── C. DOMAIN — created only when its feature flag is on (mandi/mill/…)
```

## A. Core tables
- `financial_years (id, year_name UNIQUE, start_date, end_date, is_active, is_locked)`
- `account_groups (id, name UNIQUE, parent_group_name, nature, extract_in_balance_sheet, is_system, code1st..4th, standard_root)` — Phase-2 generic chart
- `parties` — ledger master as today (name/alias/group/gst/bank/credit/pan/…,
  `apply_tcs/tcs_exempt`, `legacy_id` bridge). No mandi-party seeds.
- `voucher_types` — Phase-1 master (code UNIQUE, name, category, prefix,
  numbering_*, flags, affects_stock, requires_*).
- `stock_groups (group_name UNIQUE, ...)`, `stock_units (unit_name UNIQUE, ...)`,
  `stock_items (name, code UNIQUE, group/unit/rate/gst, ...)` — generic, no
  commodity-specific columns.
- `vouchers` — header: `(voucher_type, financial_year, voucher_no)` UNIQUE
  (`uq_vouchers_type_fy_no`), plus parties/dates/amounts/narration.
- `transactions` — legs: `voucher_no/type/trans_type/account_code/party_id/
  party_name/opposing/dr_cr/amount`, tax cols (`taxable/tcs/tds_amount+rate`),
  `entry_type` (`Ledger/Goods Amount/TCS206C/TDS194Q/…`), `invoice_no`, `row_no`.
- `sales_invoices + sales_invoice_items` — own bills, UNIQUE `(financial_year,
  invoice_no)`; `purchase_invoices + purchase_invoice_items` — supplier bills
  verbatim (NOT unique: repeats across suppliers are legitimate).
- `company_info`, `app_settings` (incl. `feature.*`, `bill_series/*`,
  `tcs_*`, `audit_actor`, `numbering_repair_needed`, `transport_deduped_v1`),
  `audit_log`, `migration_log`.

## B. Compliance tables
- `tds_vouchers`, `tax_deposit_challans(+items)`, `tcs_receipt_vouchers`,
  `advance_payments_194q`, `received_forms_16a`, `tds_acknowledgements`,
  `TDSDeductions` (compat view of Bahi-Khata shape), `debit_credit_notes(+items)`,
  `custom_closing_stocks`, `stock_transactions` (movement ledger),
  `inventory` (snapshot rebuilt from movements, never a write target).

## C. Domain tables (flag-gated)
- mandi: `paddy_procurement` (merge `paddy_arrivals` into it — exact dupes today),
  `jform_vouchers(+items)`, `iform_vouchers(+items)`, `sauda_contracts`,
  `dalali_settlements`.
- mill: `milling_batches`, `milling_voucher_items`.
- bardana: `bardana_transactions`. transport: `transport_dispatches`, `gate_register`.
- Shared line-item contract across all `*_items` tables (or single
  `voucher_line_items` long-term — Phase-1 follow-up).

## Invariants (release-blocking)
1. One DDL owner: `DatabaseManager::ensureTablesExist` (+ gated domain blocks).
2. `ΣDr == ΣCr` per voucher AND trial `difference < 0.01` globally.
3. Money = `FinancialMathService` rounding only; int64-paise migration per Doc 05.
4. FY `is_locked` enforced on all write paths; every mutation → `audit_log`.
5. Column dictionary: `name` on parties; `voucher_no TEXT`; `fy_id`;
   `invoice_no`; `driver_name/transport/broker_name/vehicle_no/gr_no` — CI-grepped.
6. Display `dd-MM-yyyy` everywhere; storage ISO; no fake identities in seeds.
