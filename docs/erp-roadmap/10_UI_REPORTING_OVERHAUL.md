# 10 — UI Standardization & Reporting Excision (v1.0, 2026-10-12)

Two workstreams, one goal: every screen looks like one product and every
report is maintainable in isolation — Tally Prime / Busy 21 competitive.

Foundation landed (additive, no behavior change except Trial Balance's
spec-locked header/rows):
- `src/widgets/erp_theme.h` — full builder API (was 3 helpers, 54 lines).
- `src/reports/report_spec.h` — per-report contract + registry (QtCore-only).
- `src/reports/specs/` — 4 canonical specs (trial-balance, ledger, day-book,
  sales-register). Remaining 18 follow the same 20-line shape.
- `src/printing/reports/report_html_chrome.h` — single print CSS + firm
  header + footer for all 22 renderers.
- `src/printing/reports/trial_balance_report.h` — first excised renderer
  (pure data→HTML, no DB/QPrinter). Old controller path untouched until the
  Mac build verifies byte-identical output.
- `trial_balance_widget.cpp` — exemplar T1-T4 retrofit (all inline QSS gone,
  `setFixedWidth` → `setMinimumWidth`, navy spec header, 29px rows).

## A. UI retrofit queue (1 PR per row, screenshot 1280×800 + 1024×680)

| Order | Widget | Issue | Fix (mechanical) |
|---|---|---|---|
| DONE | trial_balance | gray header, 36px rows, fixed widths | exemplar — copy this diff |
| DONE | sales voucher | cream `#FFF3EA`, red title, brown hints, peach grid, 47× setFixed, zero foregrounds, lying Ctrl+P/E badges | slate canvas, navy title/grid, badge footer, Ctrl+P/E wired, per-cell foregrounds |
| DONE | balance_sheet + profit_loss trees | gray `#F1F5F9` headers, 25× setFixed | `ErpTheme::treeStyle()` (new builder), fixed→minimum sweep |
| DONE | fixed-geometry sweep (27 statement files, 138 sites) | `setFixedHeight/Width` breaks DPI | `setMinimum*` (identical at 100%); only icon boxes (`iconBox->setFixedSize`) exempt |
| DONE | purchase voucher | same disease as sales + TDS-194Q row | replayed 1:1 (green radio accent kept, red Purchase badge, F9) |
| 2 | gray `QHeaderView::section` blocks → `styleTable()` | ~40 files still carry `#F1F5F9` headers | 1-block swap per read-only grid (editable grids keep custom QSS + foregrounds) |
| 3 | ledger_statement | split Dr+Cr tables (spec: single), single T4 bar | single-table migration per `ledger_statement_report.h` |
| 2 | sales/purchase registers | shape right, gray header, CSV hand-rolled per file | `styleTable` + spec columns + shared CSV writer |
| 3 | ledger_statement | split Dr+Cr tables (spec: single), single T4 bar | single-table migration per `ledger_statement_report.h` |
| 4 | day_book | no window notice, unbounded render | cap + "Showing latest N of M" footer (spec'd) |
| 5 | dashboard | no T2/T3/T4, verbose firm/FY stack | FY badge + 4 metric cards, drop custom cards |
| 6 | balance_sheet / profit_loss | own printers in `src/printing/` (only 2 excised) | move renderers to `src/printing/reports/` |
| 7 | gstr / mandi / milling / stock / registers (rest) | per-widget QSS copies | builder sweep, no logic change |
| 8 | dialogs (`keyboard_shortcuts`, period, copy-options) | raw QPushButton, fixed 540/560/720px | `KbdBadgeButton` + `bindShortcut`, size policies |

Rules per PR: no `setStyleSheet` with a `#` literal outside `erp_theme.*`;
no `setFixedWidth/Height` on fields (`setMinimumWidth` allowed); every
`QTableWidgetItem` via `ErpTheme::textItem/moneyItem/debitItem/creditItem`;
full `setTabOrder` chain + `setBuddy` on labels; `bindShortcut` so badges
can't lie (kills `GlobalKeyFilter` dupe per Doc 03).

## B. Report excision catalog (22 reports → 1 spec + 1 renderer each)

Spec = `src/reports/specs/<code>_report.h` (columns/title/shortcuts/exports).
Renderer = `src/printing/reports/<code>_report.h` (+`.cpp` only if it needs
DB; prefer pure data→HTML with formatting in `FinancialMathService`).

| # | Report | Spec | Widget today | Printer methods today | State |
|---|---|---|---|---|---|
| 1 | Trial Balance | ✅ `trial_balance_report.h` | `trial_balance_widget` ✅ retrofitted | `render/export/print_trial_balance` | EXEMPLAR |
| 2 | Ledger Statement | ✅ `ledger_statement_report.h` | `ledger_statement_widget` (split-table) | `render/export/print_ledger_statement` + `export_ledger_csv` | spec'd |
| 3 | Day Book | ✅ `day_book_report.h` | `day_book_widget` | `render/export/print_day_book` | spec'd |
| 4 | Sales Register | ✅ `sales_register_report.h` | `sales_register_widget` | `render/export/print_sales_register` + csv | spec'd |
| 5 | Purchase Register | TODO | `purchase_register_widget` | `render/export/print_purchase_register` + csv | — |
| 6 | Stock Register | TODO | `stock_detail_widget` | `render/export/print_stock_register` + csv | — |
| 7 | Item Movement | TODO | `item_movement_dialog` | `render/export/print_item_movement` | — |
| 8 | Closing Stock | TODO | `custom_closing_stock_widget` | `render/export/print_closing_stock` | — |
| 9 | Profit & Loss | TODO | `profit_loss_widget` | `render/export/print_profit_loss` + `printing/profit_loss_printer` | — |
| 10 | Balance Sheet | TODO | `balance_sheet_widget` | `render/export/print_balance_sheet` + `printing/balance_sheet_printer` | — |
| 11 | Capital Accounts | TODO | `capital_accounts_widget` | `render/export/print_capital_accounts` | — |
| 12 | Cash & Bank Flow | TODO | `cash_bank_flow_widget` | `render/export/print_cash_bank_flow` | — |
| 13 | Interest / Aank | TODO | `interest_calculator_widget` | `render/export/print_interest_statement` | — |
| 14 | Depreciation | TODO | `depreciation_chart_widget` | `render/export/print_depreciation_chart` | — |
| 15 | Milling Statement | TODO | `milling_statement_widget` | `render/export/print_milling_statement` | — |
| 16 | Mandi (M/9/RDF/fee) | TODO (parameterized) | `mandi_reports_widget` | `render/export/print_mandi_report` | — |
| 17 | GSTR (1/2/3B) | TODO (parameterized) | `gstr_reports_widget` | `render/export/print_gstr_report` | — |
| 18 | Audit Trail | TODO | `audit_register_widget` (newest, closest) | (viewer only, no print yet) | — |
| 19 | Salary Register | TODO | `salary_register_widget` | — (register has no printer path; add) | GAP |
| 20 | Gate Register | TODO | `gate_register_widget` | — (same gap) | GAP |
| 21 | Cash Book | TODO | `cashbook_options_dialog` + cash widget | `render/export/print_cash_voucher` | — |
| 22 | Voucher Numbering | TODO | (Doc 05 report fns, no widget) | `findDuplicateVoucherNos` / `findInvoiceGaps` | GAP — needs repair view |

## C. Per-report migration recipe (DONE 2026-10-13 via `tools/carve_print_reports.py`)

`print_export_controller.cpp` 3922 → ~1000 lines (plumbing + thin delegates
only). All 27 reports carved verbatim into `src/printing/reports/`:

- `<code>_report.h` — the report's contract (render decls).
- `<code>_report.cpp` — print/PDF/ODF renderer (moved verbatim, scope
  `PrintExportController::` → `PrintReports::`).
- `<code>_excel.cpp` — per-target Excel variant (same figures today, raw
  numbers + `num`/`date` classes customizable per report without touching
  print layout).
- Shared: `report_format.h` (`formatINR`), `report_data.h` (firm/party
  profiles, amount-in-words), `report_html_chrome.h` (CA-firm stationery).
- Controller keeps its full public API (delegates) — `LogicBoardTestSuite`
  print tests stay green untouched. New reports register via CMake GLOB
  (`PRINTING_REPORT_SOURCES`) — no per-file build edits.
- Re-run rule: carving is one-way. New work goes directly into the module
  files; never re-add renderer bodies to the controller (CI gate §E).

## D. CA-firm stationery (27/27 full-chrome DONE 2026-10-14)

`report_html_chrome.h` print builders: `printCss()`, `letterheadHtml(firm)`
(name/address/GSTIN/PAN + double rule), `titleBlockHtml()`,
`signatoryHtml(firmName)` (Prepared/Checked/For-<firm>-Authorised),
`generatedFooterHtml()`, `wrapPrintDocument()`, `.dayband` + `.grade` +
`table.meta` + `.words` classes.
`statement_data.h` central resolvers: `dayBookLegs` (original legs only),
`dayBookTotals`, `trialRows` — single indexed scans, no N+1, no shared state
(mutex-safe via DatabaseManager; re-entrant; worker-thread ready).
Full chrome (builders + grid + signatory + footer): trial, daybook (one row
per voucher, opposing-account column, 2000-voucher A4 cap), sale invoice
(Busy layout: copy tag + centered TAX INVOICE title + 29pt bold firm + address + GST/PAN|Phone header;
Billed-To with GST + shipping fallback | Date/No/E-way(below No)/Transporter/Vehicle/
Driver/GR meta; dynamic item grid Item+grade-direct/HSN/Qty/Bags/
Packing/Rate/GST%/GST-Amt/Amount (30pt firm heading for 7.5mm caps, unboxed small GST-PAN/Phone side
lines, excel hairlines via cellspacing-gap tables with zero CSS classes,
nowrap single-line cells, items-only rows on clean white, amount-in-words
band, gridless blank spacer scaled to item count pinning the bottom block
to the page bottom, driver name/number auto-ordered (digits decide);
right-sided CGST/SGST-or-IGST + round-off + discount
+ grand-total box; bank+T&C | For-firm footer), purchase invoice (same
chrome, supplier/bill meta), ledger statement (khata
Dr/Cr grids + net-balance band), P&L + balance-sheet modules (BS module now
renders real calculator groups, not placeholder zeros), all 3 registers,
all 7 vouchers (cash/cheque/journal/iform/jform/tds/debit-credit-note),
capital/cash-flow/closing-stock (via stock register)/depreciation/gstr/
interest/item-movement/mandi/milling/transport. Zero `header-box` shells
remain anywhere under `src/printing/`; every grid carries `border='1'`
(QTextDocument-safe alongside CSS); footers are brand-free
(`Generated <dt> | E. & O.E.`).
Compact portrait grids (2026-10-14): Taxable column dropped from sale +
purchase register PDFs (CSV keeps full data); all statement/register
grids set to tighter padding + 9pt type so every figure fits one line on
portrait A4; daybook amount columns widened to 10% (widths sum to 100
everywhere — strictly checked).
Portrait + hairlines (2026-10-14): EVERY report is strictly portrait A4
(all landscape PDF/print flags flipped, incl. daybook/registers/ledger/
P&L/BS/mandi/milling/gstr/stock/closing-stock + ledger-directory export).
All 25 grid reports converted from `class='grid/meta' border='1'` (+
shared-CSS borders that stacked into thick dividers) to excel hairlines:
borderless tables with 1px cellspacing gaps over slate, white/wash cells
and navy headers via attributes only — zero CSS classes left in any
report table, so nothing can double up. Ledgers right/center-aligned,
totals washed, imbalance rows kept red via font tags. Live P&L printer
rebuilt the same way (navy bands/totals/summary via attributes). Figures
untouched everywhere (per-file %N audit identical before/after).
Old live printers fixed: ProfitLossPrinter converted to
letterhead/title/signatory builders, navy headers, tabular amounts,
escaped names. Balance sheet follows the audited-report reference
(`balance_sheet_ca.h`, shared by live `BalanceSheetPrinter` and the
`PrintReports` module): serif Times, borderless Liabilities/Amount sides,
underlined+bold group headers with the group total on the same row,
plain ledger lines below each group, bold net profit/loss,
double-ruled grand totals,
plain 2-decimal amounts, full ledger lists (never "As Per List Attached"),
firm-only footer (`For M/s. X` + Authorised Signatory — no CA block, UDIN,
Generated stamp or triple). Module + widget figures identical (same
calculator). BS export/print is portrait A4.
Footer policy (2026-10-14, per reference `Printing_Invoice_Simple.pdf` Bill
of Supply): triple Prepared/Checked/Authorised + `Generated | E.&O.E.`
on STATEMENTS/registers only. Vouchers carry contextual underscore
signatures with NO Generated stamp (cash: Prepared/Passed/Receiver;
cheque/journal/tds/note: Prepared + For-firm; iform: Buyer + For-firm;
jform: Farmer + For-firm; transport: Driver/Weighbridge/Security).
Invoices carry a Busy-style bar (Page No. 1 of 1 | title | copy tag) +
Terms & bank & For-firm footer (`docBarHtml`/`invoiceFooterHtml`), never
the triple. Daybook is A4-bound (latest 2000 vouchers + "Showing latest N
of M" note; grand totals still full-period). `cash_bank_flow_widget`
bypass export now sets A4 + 8mm margins + doc page size like the central
controller. NOTE: root `DayBook.pdf`/`Invoice_*.pdf` artifacts are stale
(pre-rewrite Mac binaries) — rebuild on Mac after sync to see the new
one-row-per-voucher daybook, nowrap headers, and brand-free footers.

## E. CI gates (add when queue starts)

- Fail on legacy palette tokens anywhere in `src/widgets/`: `FFF3EA FDEEE4
  FEECE0 4A1500 D5F5D5 0000CC E2D5C8 F1E5DB 7F9DB9 FF0000` (all routed through
  `ErpTheme` now; global/table QSS blocks are the only `#` literals allowed,
  and they must use spec values).
- Fail on new `setFixedWidth|setFixedHeight|setFixedSize` in `src/widgets/`
  (voucher grid `setColumnWidth` exempt — functional column sizing).
- Fail on new `new QTableWidgetItem(` in `src/widgets/` without a
  `setForeground` within 5 lines (use `ErpTheme::textItem/moneyItem/...`).
- Fail on new `render*Html` bodies inside `print_export_controller.cpp`
  (must live in `src/printing/reports/`).
- Headless `ctest` stays release-blocking (Doc 05 tie-out gates).

## F. Statement interaction contract (landed 2026-10-13 — all 28 registers)

Every statement/register behaves identically (balance-sheet pattern):

1. **Grid focus on open.** `navigateToView` calls `setFocus()` + `focusTable()`
   (focuses grid, selects row 0 when nothing is selected). Tabbed reports
   (mandi/gstr) focus the active tab's grid; split views focus the active
   side (ledger Dr/Cr, balance Liabilities/Assets); gate/bardana/sauda focus
   their primary register table.
2. **Enter drills.** Table-scoped `Return`/`Enter` shortcuts (`WidgetWithChildren
   Shortcut`, so search boxes keep their keys) invoke the same slot as
   double-click: trial→ledger, ledger→voucher alteration, registers→alter,
   salary→party statement, depreciation→ledger, TDS list→alter, paddy→row,
   audit→before/after sidecar. Lists with no drill target (challans, TCS,
   194Q, 16A) and tabbed compliance reports (GSTR, Form M) get focus only.
3. **Esc preserves state.** Drill-down never marks dirty, and every branch
   reloads only on `isDirty() || fromDashboard` — returning via history keeps
   scroll, selection, filters and search text. `showEvent` self-refreshes are
   dirty-guarded (cash-bank-flow's unconditional `showEvent` refresh fixed).
4. **Dashboard always refreshes.** `fromDashboard` forces reload; unconditional
   reloads removed (depreciation, TCS/194Q/16A lists, cash/bank flows).
   `markAllStatementsDirty()` extended to depreciation, cash-flow, TCS, 194Q,
   16A — vouchers saved anywhere invalidate every statement exactly once.
5. **Keyboard truth.** Ctrl+P/Ctrl+E on the sale voucher now work (badges no
   longer lie); KbdBadgeButtons everywhere on voucher footers.
6. **Print routing (2026-10-14).** Registers print/export their REGISTER
   reports for the filtered period (`print/export_*_register_*`); single
   -invoice printing with the Original/Duplicate/Triplicate/ALL copy dialog
   lives solely on the sales voucher page (Print Ctrl+P, PDF Ctrl+E,
   empty-draft guarded). Invoice PDFs are named `<invoice-no>.pdf`.
   Copy dialog (`print_copy_options_dialog`, 1/2/3/4 + O/D/T/A keys, Esc
   cancels) verified — Original/Duplicate/Triplicate/ALL all render; the
   earlier "reports not saving" was registers never calling report
   renderers plus purchase callers discarding the dialog result, both
   removed by this rewiring. Firm gate: Esc/back on a firmless firm
   selector stays put; only opening a firm reaches the dashboard.

## G. Explicit non-goals

- No QML, no second DDL owner, no per-commodity voucher explosion (per
  `00_MASTER_PLAN.md`). Voucher *prints* (10 types) stay in the controller
  until all 22 *reports* are excised — vouchers are phase 2.
- Visual pass is Mac-only (VM is offscreen). This container has no Qt
  toolchain, so nothing here is compile-verified — Mac rebuild + screenshots
  required before merge.
