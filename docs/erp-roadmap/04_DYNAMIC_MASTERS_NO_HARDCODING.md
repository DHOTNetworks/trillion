# 04 — Dynamic Masters: Kill Hardcoding (v1.0)

## What it should look like

Zero firm-specific literals in code. Every dropdown, placeholder, rate, state,
unit comes from DB (`company_info`, `financial_years`, `stock_units`,
`MasterDataProvider::getActiveTaxRates/getUnitNames/getStockItemNames/
getTcsRate/getTds194QRate/getActiveCompanyInfo`). New firm onboards with blank
masters; GSTIN checksum + state-code derivation validate input instead of
suggesting `06AAAAA0000A1Z5`.

## What we currently have

`src/services/master_data_provider.cpp:217-386` EXISTS and is correct — but
**zero hits in `src/widgets/`**. All widgets use static lists/placeholders.

## What we lack (grep-verified sample)

- GSTIN placeholder `06AAAAA0000A1Z5`: `new_firm_dialog.cpp:173`,
  `new_ledger_widget.cpp:568`, `modify_ledger_widget.cpp:603`.
- FSSAI `10822019000152`: `new_firm_dialog.cpp:185` default,
  `bahi_khata_migrator.cpp:382` fallback, `bahi_khata_exporter.cpp:1920`.
- Firm/address: exporter `1921 BABA SAWAN SINGH THERI ROAD...SIRSA`,
  `bahi_khata_export_dialog.cpp:229 Mahadev Rice Mill`,
  `main.cpp:34`, `dashboard_widget.cpp:131`.
- Ledger defaults: `KRISHNA FOODS`, `125055`, `Phone / Mobile No.` ×2 widgets.
- State derivation `06→Haryana/03→Punjab...` in `new_firm_dialog.cpp:319-323`
  (should be 36-state table + GSTIN checksum already in engines).
- Tax lists: `debit_credit_note_widget.cpp:354 CGST 2.5%`, `:720 default 5.0`,
  `new_stock_item_widget.cpp:306 {5%,0%,12%,18%,28%}`, `sales/purchase:617`.
- Units `QTL` default in 20+ sites; challan `0021`; TDS lists, Q1-Q4, Dr/Cr, Market/Mandi.
- Static `setPlaceholderText` in ~30 sites (never DB-fetched).

## How to do it (ordered)

1. **P0 — provider wiring (1 PR):** every widget combo/placeholder calls
   `MasterDataProvider::instance()`; cache per-FY invalidation. Start with
   GST rates, units, states, company info (kills 80% of literals).
2. **P0 — firm literals purge:** move `BABA SAWAN SINGH...`, `Mahadev Rice Mill`,
   `KRISHNA FOODS`, `125055`, FSSAI default into `company_info` seed migration;
   exporter reads active firm row. Add CI grep failing on `06AAAAA|10822019|SAWAN SINGH|KRISHNA FOODS|125055` in `src/`.
3. **State table:** 36 states/UTs with codes in `app_settings` seed; GSTIN
   state-code auto-derivation + checksum (reuse `gst_tax_engine` validator).
4. **Verify:** new-firm test — create blank firm, assert no Maharashtra/Haryana
   leakage, no default GSTIN/FSSAI/phone; tax dropdowns == provider output.
