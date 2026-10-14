# 10 — Query & Index Audit (2026-10-12, full-app)

Volumes: transactions 47k, vouchers 18k, parties 2k, sales 3k, purchase 3k,
stock-tx 9k, dispatch 5k (VM copy; Mac live DB is larger — same shapes).

## Fixed this batch

- N+1 cash-flow priors (2 queries × parties per refresh+keystroke) → single
  `GROUP BY party_name, dr_cr` (`cash_bank_flow_controller.cpp`).
- Transport sync re-insert loop → identity guard (`transport_dispatch_controller.cpp`).
- Uncapped daybook/ledger prints → 5000-leg cap + full-range totals.
- Uncapped daybook/transport grids → 10k/5k newest-first + notices.
- Aank grid → 10k cap + full-total labels; spin feedback blocked.
- 14 covering indexes added in `ensureTablesExist` (vouchers no/date/party,
  transactions invoice/opposing, sales+purchase inv_nos + names,
  transport voucher_no, parties name NOCASE, jform/iform nos, milling batch_no).

## Still open (ranked)

1. **Trial-balance print N+1** (`print_export_controller.cpp:2153`): 2 scalars ×
   2k parties over 47k tx. Same GROUP BY fix as cash-flow. (S)
2. **Stock register print N+1 + ignored date params** (`:3266`, `:3382`):
   per-item double-SUM + `trans_date` vs indexed `voucher_date` mismatch.
   Single GROUP BY + cap. (M)
3. **Ledger open OR-query** (`parties_model.cpp:867`, `interest_model.cpp:96`):
   3-way `LIKE '%x%'` over 47k. Resolve id once → `party_id=?` + BETWEEN. (M)
4. **OR-index lookups** (`invoice_no=? OR voucher_no=?` in sales/purchase/vouchers
   models + `resolveDocumentMetadata`): rewrite as UNION ALL / single-key +
   `findInvoiceId` shared helper. (M)
5. **Interpolated dates** (sales/purchase/paddy register widgets,
   daybook LIMIT): bind `?` params; push search into SQL + LIMIT. (S)
6. **Full-model loads** (`SELECT * ORDER BY id DESC` ×6 models + parties/stock
   masters per dropdown): FY-bounded + LIMIT 500 paged helper; masters via
   `MasterDataProvider` lite projections. (M)
7. **jform/iform mirror duplication**: merge lookups into one mandi helper. (M)
8. **Autocomplete scans** (`name LIKE '%x%'` per keystroke): prefix LIKE + LIMIT 50. (S)

## Memory verdict (CLOSED 2026-10-13 via macOS spindump)

- **Footprint was 227 MB, not 45 GB** — virtual-memory misread; real memory fine.
- **It was a hang, not a leak**: 1.07s main-thread hang, all 11 samples in
  `loadDayBookData → setItem → dataChanged → visualRect → resizeSections →
  sizeHintForColumn → HarfBuzz shaping` (`day_book_widget.cpp:430-456`).
- Root cause: `ResizeToContents` on 5 columns. `setUpdatesEnabled(false)`
  gates only PAINT, not layout — every one of ~330k setItems re-measured the
  column with QSS-delegate text shaping. No query bug, no loop bug, no dupe
  growth involved at all.
- Entry doubled it: `navigateToView` loads directly AND `setCurrentWidget →
  showEvent → onRefreshClicked` loads again.
- Fix (daybook): Fixed column widths (Stretch kept on 3/4) + dirty-gated
  `showEvent` (mutations already mark dirty in ~20 places). Same data, same
  rows, no measuring ever.
- Same `ResizeToContents` pattern exists in ~20 other tables (widget audit
  list) — apply the same Fixed-width treatment per view as follow-ups.
