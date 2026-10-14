# 07 — Manufacturing Vertical: Rice / Cotton / Oil (v1.0)

## What it should look like (generic for all three)

One milling/production core with per-industry yield templates:
rice (paddy→rice+bran+nakku+husk), cotton ginning (kapas→lint+seed),
oil (seed→oil+cake, expeller/solvent/refinery loss stages). Joint-cost
apportionment by saleable value, by-product valuation, batch/lot traceability,
multi-godown transfers, FIFO/WAC selectable like Tally's Stock Valuation Method.

## What we currently have

`milling_batches` + `milling_voucher_items`, `milling_yield_engine` (thin % math),
`milling_statement` model+widget, paddy procurement with moisture math,
`custom_closing_stocks` + `stock_valuation_engine` (latest-rate fallback),
single `Main Godown`, cost build-up analyzer (`bahi_khata_analyzer.py`: dheri
auction avg + mandi extra + freight = landed cost/Qtl; recovery matrix 65-70%).

## What we lack

1. **Cotton/oil engines: zero code.** No ginning outturn, no expeller/solvent/
   refinery stage yields, no joint-cost apportionment anywhere (rice included —
   `milling_yield_engine` is pure percentages with tolerance flag).
2. **Inventory costing:** latest-rate fallback only; no FIFO/WAC; no method selector.
3. **Godown/batch:** single godown, no transfer costing, no batch/expiry/lot
   (critical for oil/cotton-seed).
4. **Landed-cost ↔ milling link:** analyzer computes `final_landed_cost_per_qtl`
   in Python, but nothing feeds it into milling input valuation in C++.
5. Landed freight allocation per-batch; bardana consumption per-batch.

## How to do it (ordered)

1. **Yield template table (1 PR):** `production_templates(kind, input_item,
   outputs[{item, pct, rate_basis}], loss_pct, cost_method)` seeded with
   rice-67%, cotton-ginning, mustard-expeller profiles. `milling_batch_controller`
   validates `Σoutputs + loss == input` within tolerance (test: 67/7/8/17/99).
2. **Joint-cost apportionment (1 PR):** extend `milling_yield_engine` with
   value-basis + weight-basis methods; by-product stock receipts auto-valued;
   tie to `stock_transactions` (one transaction per output, same batch id).
3. **FIFO/WAC (1 PR):** `stock_valuation_engine` gains method column per item;
   WAC running average + FIFO layers in `stock_transactions`; Tally-parity
   selector in `new_stock_item_widget` (replacing hardcoded 5/0/12/18/28 list —
   rates from provider, method from engine).
4. **Multi-godown + lots (1 PR):** `stock_godowns` + `transfer_vouchers`;
   batch/lot columns on `stock_transactions`; godown-wise stock register.
5. **Feed landed cost in (1 PR):** port `bahi_khata_analyzer.py` formula into
   `FinancialMathService::landedCostPerQtl()`; procurement save stores it;
   milling input defaults to it. Verifies `qty × rate == total paid` invariant.
