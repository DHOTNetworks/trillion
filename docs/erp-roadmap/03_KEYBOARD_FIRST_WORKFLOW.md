# 03 — Keyboard-First Workflow (v1.0)

## What it should look like (Tally/Busy contract)

Full voucher entry without mouse. Global, discoverable, non-colliding:
`F2` date • `F4/F7` contra/journal • `F5/F6` payment/receipt (Tally muscle) •
`F8/F9` sales/purchase • `F10` J-Form • `F11` I-Form • `F12` config • `F1/?` help •
`Ctrl+S` save everywhere • `Del` row-delete / `Alt+Del` voucher-delete •
`Insert` new row • `Ctrl+F` global search • `Ctrl+P` print • `Ctrl+E` export •
`Alt+F1` company • `Esc` back/close. Every badge shows its key; every key works.

## What we currently have

Architecture: `AppKeyboardController` (qApp eventFilter, 512 lines) +
`GlobalKeyFilter` (header-only, 45 lines) + `KbdBadgeButton` (paint-only, no
shortcut linkage) + per-widget `keyPressEvent`/`QShortcut` +
`keyboard_shortcuts_dialog.cpp:96-133` truth-table. Ledger widget is best-wired
(12 shortcuts, tab-order); vouchers rely on `ItemSearchDelegate` grid nav only.

## What we lack (gaps vs spec + Tally)

- `Ctrl+S` covers only 6 views (14,15,16,17,60,24) — J-Form(18)/I-Form(19)/
  Milling(31)/DebitNote(28)/journals silently don't save.
- Bare `Del` unhandled globally (dialog claims row-delete); `Ctrl+D` only on same 6.
- `F5=expandAll/F6=collapseAll` COLLIDES with spec `F5=Refresh` + Tally
  payment/receipt; dialog says `F5=Journal`. Three meanings, all wrong somewhere.
- `F11` (I-Form) has NO controller binding though dialog documents it.
- `triggerCsvExport()` has NO key; dialog says `Ctrl+E`=Excel but code binds
  `Ctrl+E→triggerPdfExport`; `Alt+P`=PDF vs `Ctrl+P`=print confusion.
- `Ctrl+F` only on ledger; spec = everywhere. `Alt+F` on ledger = period dialog
  (Tally = voucher-type/filter). `Alt+F1` unbound (only `Alt+F2` in main_window).
- `GlobalKeyFilter` is DEAD code: only Esc branch works, f-signals never emitted;
  double-filters with `AppKeyboardController` (both bail on modal/popup).
- `KbdBadgeButton` can LIE: badge text ≠ bound shortcut (ledger CSV button badge ""
  but `Alt+E` bound separately; dashboard `1-6` only when card focused).
- `isVoucherActiveView()` helper defined, never used. No global `F1`, `Space`, `Insert`.

## Status (implemented 2026-10-12, Mac must rebuild)

- P0 wrong-column fixed: `jform_model.cpp`, `iform_model.cpp`,
  `debit_credit_note_controller.cpp` now query `WHERE name` (+ `COLLATE NOCASE`
  on equality lookups). Gate: 0 hits for `FROM parties WHERE party_name`.
- Single keymap: `src/services/keymap.h` (ErpKeymap::bindings) is truth;
  `AppKeyboardController::handleKeyPress` dispatches from it via
  `openDateForView/saveActiveVoucher/deleteActiveVoucher` (one switch each).
- Added: F11→I-Form(19), Alt+F1→firm selector(22), F1→shortcuts dialog,
  Alt+E→CSV export, Ctrl+F→ledger search. Removed: Ctrl+Alt+P/B dupe nav.
- Scoped: F5/F6 to reports 29/30 only; print/PDF/CSV/search swallowed only
  where the view supports them (bool triggers).
- Deleted `GlobalKeyFilter` (file + main.cpp include + CMakeLists entry);
  0 references remain.
- `KbdBadgeButton::bindShortcut(seq)` creates a real QShortcut — badge can't lie.
  Shortcuts dialog renders from the keymap (old hardcoded list with false
  Alt+1..7 / F3-F5 claims deleted) and uses ErpTheme + badge button.
- Canonical GST states: `src/services/gst_state_codes.h` (37 codes); rewired
  PartiesModel (both maps + standard list), GstTaxEngine (explicit "Code XX"
  fallback), NewFirmDialog (all-states, not 5).
- Dynamic GSTIN hint: `MasterDataProvider::gstinPlaceholder()` from active
  `company_info.state_code`; fake `06AAAAA0000A1Z5` purged (0 hits in src).
- Provider fallbacks now `qWarning` (units, voucher_settings, tax slabs).
- Shared theme: `src/widgets/erp_theme.h` palette/geometry/helpers; applied to
  shortcuts dialog as first consumer.

Still open (next batches): voucher fixed-px purge, foreground sweep, tab-order
chains, VoucherCommon grid-nav rollout, audit_log + FY-lock enforcement,
paise dual-write, one-rounding (Doc 05 P0).

## How to do it — remaining
1. **Keyboard-only flow test:** post one voucher per kind via `QTest::keyClick`
   as `tests/test_keyboard_flow.cpp` (Mac; VM runs it headless once Qt lands).
