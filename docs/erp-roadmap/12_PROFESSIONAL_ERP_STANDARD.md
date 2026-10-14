# 12 — Professional ERP Standard: Vouchers & UI (v1.0, 2026-10-13)

Reference: Tally Prime voucher anatomy + Busy 21 density, adapted to our
slate/navy design system (`QTWIDGETS_DESIGN_SYSTEM.md`, `ErpTheme`).

## 1. The standard (every voucher, no exceptions)

```
T1  Voucher header card (white): navy 15px title left ("F8 : Sale Voucher"),
    type badge (Sale=info-blue, Purchase=danger-red, Payment/Receipt mode),
    red ALTERATION badge only in edit mode, Back(Esc) badge button right.
T2  Meta strip (white card): voucher-no (read-only, monospace, blue — system
    value, never user-typed), date + weekday, FY badge, party ledger search
    with live balance, GSTIN text, tax-status radios. ONE strip, not three.
T3  Entry grid: navy header + white text, 29px rows, alternating wash,
    explicit foreground on every cell (editable tint, read-only white,
    amounts green-dark). Cell widgets (combos/searches) inherit global input
    style — never per-cell QSS.
T4  Totals card (white): taxable / tax / freight / round-off / TCS|TDS-194Q /
    G.Total (16px navy, 2px blue ring). One card, right-aligned.
T5  Action footer: KbdBadgeButtons — Date(F2) · Add Row(Insert) · Delete(Ctrl+D,
    red, hidden unless editing) · Back(Esc) · Save(Ctrl+S, blue, StrongFocus).
```

Rules: slate `#F8FAFC` canvas, zero cream/peach/tan (`FFF3EA/FDEEE4/FEECE0/
E2D5C8/F1E5DB/7F9DB9/4A1500/0000CC/D5F5D5/FF0000` all banned — CI-gated);
`setMinimum*` never `setFixed*` (exempt: system number fields, mode
indicators, 40px row-action cells, icon boxes); badges never lie (every
printed shortcut works — Ctrl+P/Ctrl+E wired where promised); Esc preserves
state, dashboard refreshes (Doc 10 §E contract).

## 2. Applied

| Batch | Treatment |
|---|---|
| Sale (F8), Purchase (F9) | full overhaul (see Doc 10) |
| Journal, Cash, Cheque | slate canvas, navy titles/grids, badge footers, `ErpTheme` builders; mode buttons stay native (active-state recolor) |
| All widgets (programmatic) | 208 `setFixed`→`setMinimum`, 16 cream roots→slate, 33 `:focus` washes→ivory |
| **Statements phase (2026-10-13)** | **44/44 grid headers navy (`#0F172A` + white, right-dividers only), all `::item:selected` + `selection-*` → blue/white, across 37 statement/register/voucher files; light-bottom header rules dropped; `keyboard_shortcuts_dialog` already themed** |

## 3. Queued (same recipe per file)

1. ~~Gray `QHeaderView::section` blocks → navy~~ DONE (§2 statements phase).
2. **Secondary-voucher footers + truthful badges (2026-10-13 batch):**
   J-Form + I-Form (badge footers, dead `#primaryButton/#dangerButton` QSS
   deleted) • TCS receipt (F3/Ctrl+S wired — were dead) • 194Q advance
   (F3/F2 wired — were dead) • Tax challan (badge Save/Back, Ctrl+S already
   wired) • Debit/Credit note (badge add/remove, Del wired) • Milling (navy
   title; back/save already badges).
3. ~~Masters~~ DONE (ledgers badge footers + `kEmerald`; stock/group were
   already badge-native; masters keep their `m_navOrder` traversal + focus
   first-field — no tab chains needed).
   ~~Dashboard/shell~~ PARTIAL (Open Firm badge; structural T2/T3/T4 retrofit
   still open — dashboard cards are intentional design, not debt).
   ~~Dialogs~~ DONE (bill-series, print-copy, period, voucher-date ×2,
   cashbook-options verified keyboard-native).
   ~~Print fidelity~~ DONE (all 22 print `th` rules navy, matching
   `report_html_chrome.h`; B/W-safe near-black).
4. **Firm gate** — firm selector + new-firm dialog full badge sets (F3/F4/F5/
   Ctrl+N/Enter/Esc all verified wired).

## 4. Universal delete contract (2026-10-13)

- `src/widgets/voucher_ops.h` — ONE canonical classifier (`kindForEntry`),
  view router (`viewIndexForKind`: 14/15/24/18/19/60/16/17/31/28), confirmer
  and owning-model deleter (`deleteEntry`). Replaces the ledger + daybook
  hand-rolled type switches (~100 lines deleted); alter and delete agree.
- **Alt+D** deletes the focused voucher in ledger statement, day book, sales
  register, purchase register, transport register (dispatch delete), plus
  Alt+D aliases on the Del-lists (TDS/challan/TCS/194Q/16A). Every delete
  emits `entriesChanged()` → `markAllStatementsDirty()` and reloads in place
  with filters preserved. Model deletes keep FY-lock + audit (Doc 05).
- Voucher screens missing delete: milling batches have NO backend (Doc 01/07
  work); TCS/challan/194Q/form16a entry screens are always-new (their
  registers delete). 194Q widget edit-mode delete left to its list on purpose
  (list owns the multi-table SQL — no second copy).

## 5. Keyboard-truth batch (2026-10-14, Mac rebuild required)
- 194Q advance save rewritten to the canonical schema (`fy_id`,
  `bank_ledger_id`, `gross_advance_amount`, `net_payment_amount`,
  `tds_payable_ledger_id`) — voucher creation previously failed on
  `no such column`; list delete now clears `transactions` (was
  `voucher_items`). F2 no longer saves (focuses date); Ctrl+S added.
- Milling + challan Save badges now true (Ctrl+S bound alongside Alt+S);
  TCS voucher gains real F3/Ctrl+S shortcuts; TDS voucher gains real
  Ctrl+S/Ctrl+D/F2 bindings behind its badges.
- All five TDS-domain lists honor their printed keys (Insert/Delete/F5
  added next to the Alt+* aliases; challan Esc fixed — was Alt+Esc only).
- TDS ack dialog: `setFixedSize` → minimum + resize; Del/Esc now work.
- Sauda/gate/bardana registers: Alt+D/Del delete; focusTable already
  existed on all three. Full ErpTheme restyle of bardana/sauda/gate
  (15–16 inline sheets each, zero tab-order) deliberately deferred —
  same recipe as Doc 10 §A, needs Mac visual pass.

## 6. TDS/TCS standard-format pass (2026-10-14, Mac rebuild required)
- All 11 TDS/TCS surfaces on `ErpTheme` builders, zero `#` literals left
  in any `setStyleSheet` line: TDS voucher, TCS receipt, 194Q advance,
  challan creation, Form-16A dialog, ack dialog, all 5 registers.
- Vouchers: T1 builder header + red ALTERATION badge in edit mode
  (TDS/TCS/194Q titles flip Creation↔Alteration), T4 metric cards via
  `makeMetricCard`, tables via `styleTable` (challan/ack), full Tab-order
  chains mirroring new complete Enter chains, initial focus on
  new/load, date-picker buttons show "…" + tooltip (were blank).
- Registers: raw QPushButtons → `KbdBadgeButton` + `bindShortcut`
  (Insert/Delete/F5 now button-owned, loose dupe shortcuts removed so
  nothing double-fires; Back/Esc and TDS-Edit/Enter stay on their
  existing shortcuts by design). Metric lambdas → `makeMetricCard`.
- Real bug fixed: TCS Row-2 T.C.S. Amount label+field both at grid
  (2,4) — amount field now at (2,5), label visible again.

## 5. Non-goals

No behavior/logic changes in this pass (save paths, calculations, delegates
untouched). No `styleTable()` on editable grids (kills editing). No second
theme source — `ErpTheme` only.
