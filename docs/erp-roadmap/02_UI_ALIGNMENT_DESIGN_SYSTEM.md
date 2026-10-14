# 02 — UI Alignment & Design-System Compliance (v1.0)

## What it should look like

`docs/QTWIDGETS_DESIGN_SYSTEM.md` is the law: single-slate zero-scroll, 2-3 col
cards, 8/10px grid, sticky header/footer. 4-tier register:
T1 header card (16px/800 `#0F172A` + `KbdBadgeButton`), T2 filter bar (48-54px,
inputs 1.5px `#CBD5E1` 6px radius, focus 2px `#2563EB` on `#FFFFF0`),
T3 single `QTableWidget` (navy header, 28-30px rows, SelectRows/Single),
T4 four summary cards (4px accent stripe). Positive `#16A34A`, negative `#DC2626`,
muted `#64748B`. Every table cell sets foreground (`#0F172A`, editable `#F8FAFC`).

## What we currently have (scored)

| Surface | Score | Note |
|---|---|---|
| LedgerStatement | 7.0/10 | T1/T2 compliant, 12 shortcuts; T3 is split Dr+Cr (spec: single table), T4 single bar |
| MainWindow shell | 7.0/10 | Clean, min 1024×680; no badges/tab-order |
| Dashboard | 6.0/10 | Palette close; custom cards, no T2/T3/T4, verbose firm/FY stack |
| Debit/Journal/Transport | 5.5/10 | Shape right; fixed-px + style duplication |
| Sales/Purchase vouchers | 4.0/10 | Worst: ~50 `setStyleSheet`+`setFixed` in 500 lines, legacy browns (`#4A1500` hint, `#FDEEE4` bar, `#D5F5D5` edit bg, `#0000CC` underline), 4-row logistics matrix + mandi expenses + summary = cramped 9px hints, 22px inputs |
| **Overall** | **≈5.5/10** | Not shippable as "system-compliant" |

## What we lack (anti-patterns, all grep-verified)

- **A. Fixed-pixel proliferation:** widths `42/65/85/105/110/160/190`, heights
  `22/24/26/32/44/46/50/54`, dialogs `540/560/720×560`. Breaks DPI + single-slate.
  Files: `sales_voucher_widget.cpp:80-508`, `ledger_statement_widget.cpp:45-331`,
  `dashboard_widget.cpp:145-718`, `journal_voucher_widget.cpp:46-254`.
- **B. Margin drift:** `16,16,16,16/12` vs `10,6/6` vs `12,10/8` vs `20,20/14`.
- **C. Style duplication:** 80+ inline QSS hits; every QLabel repeats
  `border:none;background:transparent;font-size...`; header gray `#F1F5F9` vs spec navy.
- **D. Zero-unstyled violation (dark-mode invisible text):** sales/purchase/journal/
  cash/cheque voucher tables set ZERO `setForeground`. Only ledger/transport/debit/
  closing-stock/capital/milling do.
- **E. 4-tier non-compliance:** vouchers have no T1-T4 (custom badge/matrix/summary);
  dashboard has no T2/T3/T4; `keyboard_shortcuts_dialog.cpp:22` uses raw QPushButton.
- **F. No buddies/tab-order:** zero `setBuddy` in `src/widgets/`; only
  `ledger_statement_widget.cpp:424-426` sets tab-order (3 links).

## How to do it (ordered)

1. **Centralize theme (P0, 1 PR):** new `src/widgets/erp_theme.h` — constants
   `kNavy/kMuted/kBlue/kGreen/kRed/kFilterH/kRowH/kRadius`, functions
   `styleFilterInput(), styleHeaderLabel(), styleSummaryCard(accent)`.
   Replace inline QSS in vouchers first (biggest win). CI: warn on new `setStyleSheet`
   with literal `#` outside `erp_theme.*`.
2. **Kill fixed-px in vouchers (P0):** size policies + stretches replace
   `setFixedWidth/Height` on header fields; logistics matrix → collapsible
   `QToolBox` section (Veh/GR/Driver/E-Way/BillTime/Sauda/POS/PO/Grade/Transport/
   Challan/Kanda/Broker hidden by default, Tally-style `Alt+F2` expands).
3. **Foreground sweep (P0, 1 h):** add `setForeground` to every voucher grid cell
   (`sales/purchase/journal/cash/cheque`); unify Dr/Cr colors to spec
   (`#16A34A`/`#DC2626`, today ledger uses `#1D4ED8`/`#15803D`).
4. **4-tier retrofit per register (1 PR per register):** single T3 table, T4 four
   stripe cards, T1 `KbdBadgeButton`s. Start with sales register (most-seen screen).
5. **Tab-order + buddies (1 PR):** full `setTabOrder` chains on vouchers
   (Date→VoucherNo→Party→Grid→Save), `setBuddy` on all form labels. Verify by
   keyboard-only voucher entry test (see Doc 03).
6. **Mac visual pass:** screenshot each fixed screen at 1280×800 + 1024×680,
   attach before/after to PR. VM cannot do visual pass (offscreen only).

## Daybook pass (2026-10-13, day_book_widget only)
- Black BG: pinned QTableCornerButton::section to navy + light-slate
  QScrollBar rules in the table sheet (both fell back to OS dark theme);
  removed dead duplicate QTableWidget/QHeaderView rules from
  pplyCustomStyles() (m_table's own sheet always won � single source now).
- Sizing: row height 36 -> ErpTheme::kTableRowH (29, spec 28-30); filter
  bar/table/footer already minimums + stretch (only fixed item left is the
  36x36 DB badge, intentional).
- State-safe reload: snapshot selected voucher_no + scrollbar value before
  setRowCount wipe, restore same-voucher selection (fallback: clamped
  scroll) after populate; search box debounced 250ms (one reload per pause,
  not per keystroke). Dirty-gate on showEvent + markAllStatementsDirty on
  every save path already correct � untouched.

## App-wide dd-MM-yyyy display (2026-10-13)
- Keeper: FiscalYearHelper::formatDisplayDate (normalizes then formats).
- Routed 15 display sites to it: daybook date col, mandi 5 tables, milling
  batch col (+detail label reads back formatted), stock MonthlyDaily col,
  bank-import date col, gate live+history, bardana register, sauda date,
  tcs/tds/challan/form16a/advance fallbacks, audit ts (custom ISO-T parser).
- Already compliant, untouched: transport, paddy, sales/purchase registers,
  tcs/tds primaries, aank, interest model, GSTR, salary month combo.
- Storage/query paths stay ISO (verified: all date tables in the sample DB
  are ISO; only display changed).
- Keeper hardened for bare 8-digit stamps (YYYYMMDD/DDMMYYYY from Busy or
  migrated firms) which the ISO parser echoed raw � the one shape that could
  still render as yyyymmdd.
