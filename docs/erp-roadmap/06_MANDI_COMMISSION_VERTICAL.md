# 06 — Mandi / Commission-Agent Vertical (v1.0)

## What it should look like (the moat vs Tally/Busy)

An arhtiya opens the app and sees THEIR world: J-Form procurement with
dami/market-fee/HRDF/RDF/labour auto-split, I-Form sales, Form-M, sauda/dalali
with 194H, 194Q deduction > 206C(1H) collection precedence, Interest
postable in one click, bardana issued/returned per agent, gate-register with
weighbridge net-weight math, bank-reconciliation (BRS) closing the loop.
Every number ties: J-Form ↔ purchase register ↔ GSTR-1 ↔ 3B ↔ TDS-26Q.

## What we currently have

J-Form/I-Form vouchers + items, mandi reports controller, `mandi_calculator`
(production, exact fixture), bardana/gate/sauda/dalali controllers + widgets,
TDS vouchers + 194Q advance + Form-16A + challan widgets, paddy procurement +
weighbridge dialog, bank XLS/PDF import with narration aliases.

## What we lack (vs Busy Mandi + market practice)

1. Aank interest calculates but the **one-click post-to-journal** loop is weak;
   three methods (Aank/product ÷3000/3600, Mandi-rule grace/drop/360-vs-365,
   flat) not all exposed in one `InterestCalculator` view.
2. **194Q vs 206C(1H) precedence** (`>50L` purchase-TDS beats sale-TCS) — engine
   has both rates, no combined settlement view proving which fired per party.
3. **Bank reconciliation (BRS):** import exists, Tally-style unreconciled
   statement-vs-book matching does not.
4. **Credit-limit block:** `credit_limit/use_credit_limit` stored, never blocks save.
5. **Verified/locked vouchers:** Bahi `VerifiedVouchers` (1.3k rows) has no
   counterpart — audited quarters can still be edited (see Doc 05 audit log).
6. Interest-on-overdue auto-post, multi-commission slabs per agent.

## How to do it (ordered)

1. **Interest posting loop (1 PR):** `TempAankTypeInterest`-parity sheet in
   `InterestCalculatorWidget` (product, divisor 3000/3600/36500, grace/drop/due
   from `InterestSettings`-equivalent seed) → `Post as Journal` button creating
   balanced voucher → appears in ledger + trial balance same FY. Test with `3287.67`
   fixture end-to-end.
2. **194Q/206C(1H) settlement view (1 PR):** per-party FY turnover strip showing
   threshold crossing, which section fired, challan linkage, Form-16A status.
   Reuse `tcs_config_dialog` + `advance_payment_194q` widgets as base.
3. **BRS (1 PR):** `bank_statement_controller` gains `reconcile(book_txn, stmt_txn)`;
   unreconciled aging report; `bank_narration_aliases` learning from matches.
4. **Credit block + sauda slabs (1 PR each):** enforce `use_credit_limit` on save
   with override reason logged to audit; per-agent dalali slabs in `sauda_contracts`.
5. **Verify on market data:** run full mandi fixture (J-Form 122000/120500,
   dalali 4000/200/3800) through GSTR-1 + 26Q on VM headless; Mac screenshots
   the arhtiya dashboard.

## Status (2026-10-14 batch, Mac rebuild required — not compile-verified here)
- Gatepass ↔ purchase/sale link live: J-Form/I-Form saves call
  `GateRegisterController::linkVoucher(gate_pass_no, voucher_no)`; transport
  meta resolver now hits via `linked_voucher_no`.
- J-Form delete also clears `transactions` (was orphaning ledger legs).
- Paddy inward reads `paddy_arrivals(paddy_variety/arrival_date)` — Raw-Paddy
  stock no longer stuck at 0 (Doc 01 `paddy_procurement` starvation noted).
- Sauda + gate registers have real deletes (Alt+D/Del): sauda refuses when a
  settlement is posted to JV; gate deletes by pass no from either tab.
- Bardana Alt+D/Del wired to its existing delete.
