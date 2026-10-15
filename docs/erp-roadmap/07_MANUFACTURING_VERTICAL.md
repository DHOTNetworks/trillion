# 07 — Generic Manufacturing Vertical: Multi-Industry Production ERP (v2.0)

## Executive Summary
This document defines the architectural transition from single-purpose rice milling vouchers (`milling_batches`, `milling_voucher_items`, `MillingYieldEngine`) to a **fully generic, multi-industry Manufacturing & Production ERP system**. 

The target system natively supports continuous process manufacturing, batch assembly, and extraction industries (Rice Mills, Cotton Ginning & Pressing, Oil Expelling & Solvent Extraction, Flour Mills, Feed Plants, and Discrete Assembly) while maintaining bit-level backward compatibility with legacy Bahi-Khata / Mahadev Rice Industry milling data.

---

## 1. Architectural Problems with Current State

1. **Hardcoded Domain Tables**: `milling_batches` hardcodes agricultural columns: `paddy_variety`, `paddy_input_qtl`, `head_rice_qtl`, `broken_rice_qtl`, `bran_qtl`, `husk_qtl`. Cotton, mustard oil, or generic manufacturing cannot record transactions without polluting irrelevant columns.
2. **Hardcoded Engine Math**: `MillingYieldEngine` and `MillingBatchController::autoPickStandardItems()` hardcode rice yields (`65% Rice`, `15% Bran`, `10% Broken`, `5% Nakku`, `5% Husk`).
3. **Absence of Joint-Cost Apportionment**: Costing for co-products (e.g., Raw Oil vs Oil Cake, or Cotton Lint vs Cotton Seed) currently uses arbitrary manual rates instead of standard Net Realizable Value (NRV) or physical weight proportions (CAS-3 / IND AS 2 compliance).
4. **No Multi-Stage / Multi-Operation Work Orders**: Process industries requiring staged production (e.g., Seed Cleaning $\to$ Expeller $\to$ Solvent Extraction $\to$ Refinery) cannot chain batch runs.
5. **Single Godown & No Lot/Batch Traceability**: All consumed and produced stock flows into a single implicit godown without lot tracking, expiry, moisture tracking, or batch inspection.
6. **Disconnected Landed Cost**: Raw material landed cost (procurement price + mandi tax + freight + labour) calculated in procurement is not automatically passed into production consumption costing.

---

## 2. Target Generic Architecture

```
┌─────────────────────────────────────────────────────────────────────────────────┐
│                           MANUFACTURING MASTER & TEMPLATES                       │
│  - production_templates (BOM / Yield Formula: Input ratios, Outputs, Wastage)   │
│  - production_stages (Multi-step routing: Pre-clean -> Process -> Finish)        │
└────────────────────────────────────────┬────────────────────────────────────────┘
                                         │
                                         ▼
┌─────────────────────────────────────────────────────────────────────────────────┐
│                         GENERIC PRODUCTION VOUCHER ENGINE                       │
│  - production_vouchers (Header: Order/Batch No, Stage, Date, Godown, BOM Ref)    │
│  - production_voucher_items (Type: CONSUMED [Cr] | PRODUCED [Dr] | BYPRODUCT [Dr]│
│                              | EXPENSE_ADDITION)                                │
│  - Cost Apportionment Engine (NRV / Sales Value Basis / Weight Proportion)       │
└────────────────────────────────────────┬────────────────────────────────────────┘
                                         │
                    ┌────────────────────┴────────────────────┐
                    ▼                                         ▼
┌───────────────────────────────────────┐ ┌───────────────────────────────────────┐
│      STOCK PIPELINE & LOT TRACKING     │ │           FINANCIAL LEDGERS           │
│ - Multi-Godown (Raw Material Store ──>│ │ - Direct Manufacturing Expenses      │
│   Shop Floor ──> Finished Goods)      │ │   apportioned to output costs         │
│ - Lot / Batch Nos, Expiry, Moisture % │ │ - Stock in Hand (RM Cr, FG Dr)        │
│ - WAC / FIFO layers                   │ │ - Scrap / Wastage Write-off Ledger    │
└───────────────────────────────────────┘ └───────────────────────────────────────┘
```

---

## 3. Core Database Schema

### 3.1 Bill of Materials (BOM) & Yield Templates
```sql
CREATE TABLE IF NOT EXISTS production_templates (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    name TEXT UNIQUE NOT NULL,             -- e.g. "Rice Milling 67% Basmati", "Cotton Ginning 34% Lint"
    industry_type TEXT NOT NULL,           -- RICE | COTTON | OIL | FLOUR | GENERAL
    standard_input_item_id INTEGER,
    standard_batch_qty REAL DEFAULT 100.0, -- Standard batch scale (e.g. 100 Qtl)
    expected_wastage_pct REAL DEFAULT 1.0,
    cost_allocation_method TEXT DEFAULT 'NRV', -- NRV | WEIGHT_RATIO | BYPRODUCT_SUBTRACTION | FIXED_RATE
    is_active INTEGER DEFAULT 1,
    FOREIGN KEY (standard_input_item_id) REFERENCES stock_items(id)
);

CREATE TABLE IF NOT EXISTS production_template_items (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    template_id INTEGER NOT NULL,
    item_id INTEGER NOT NULL,
    item_role TEXT NOT NULL,               -- INPUT | OUTPUT | BYPRODUCT | OVERHEAD
    ratio_pct REAL NOT NULL DEFAULT 0.0,   -- Expected recovery % or consumption ratio
    cost_share_pct REAL DEFAULT 0.0,       -- Optional fixed cost apportionment %
    valuation_basis TEXT DEFAULT 'AUTO',   -- AUTO | ESTIMATED_RATE | MARKET_RATE
    FOREIGN KEY (template_id) REFERENCES production_templates(id),
    FOREIGN KEY (item_id) REFERENCES stock_items(id)
);
```

### 3.2 Production Vouchers & Items (The Execution Engine)
```sql
CREATE TABLE IF NOT EXISTS production_vouchers (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    fy_id INTEGER,
    financial_year TEXT NOT NULL,
    voucher_no TEXT UNIQUE NOT NULL,
    voucher_date TEXT NOT NULL,
    template_id INTEGER,
    batch_no TEXT NOT NULL,
    source_godown_id INTEGER,
    dest_godown_id INTEGER,
    stage_name TEXT DEFAULT 'Production',
    total_input_weight REAL DEFAULT 0.0,
    total_output_weight REAL DEFAULT 0.0,
    wastage_weight REAL DEFAULT 0.0,
    wastage_pct REAL DEFAULT 0.0,
    total_input_cost REAL DEFAULT 0.0,
    total_overhead_cost REAL DEFAULT 0.0,
    total_output_cost REAL DEFAULT 0.0,
    narration TEXT,
    created_by TEXT DEFAULT 'System Admin',
    created_at TEXT DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (fy_id) REFERENCES financial_years(id),
    FOREIGN KEY (template_id) REFERENCES production_templates(id),
    FOREIGN KEY (source_godown_id) REFERENCES stock_godowns(id),
    FOREIGN KEY (dest_godown_id) REFERENCES stock_godowns(id)
);

CREATE TABLE IF NOT EXISTS production_voucher_items (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    voucher_id INTEGER NOT NULL,
    batch_no TEXT NOT NULL,
    batch_date TEXT NOT NULL,
    row_no INTEGER DEFAULT 1,
    row_type TEXT NOT NULL,                -- CONSUMED (Cr) | PRODUCED (Dr) | BYPRODUCT (Dr) | WASTAGE
    item_id INTEGER,
    item_code TEXT,
    item_name TEXT NOT NULL,
    godown_id INTEGER,
    lot_no TEXT,
    bags INTEGER DEFAULT 0,
    weight_qtl REAL NOT NULL DEFAULT 0.0,
    yield_pct REAL DEFAULT 0.0,
    rate REAL DEFAULT 0.0,
    amount REAL DEFAULT 0.0,
    cost_apportionment_pct REAL DEFAULT 0.0,
    narration TEXT,
    FOREIGN KEY (voucher_id) REFERENCES production_vouchers(id) ON DELETE CASCADE,
    FOREIGN KEY (item_id) REFERENCES stock_items(id),
    FOREIGN KEY (godown_id) REFERENCES stock_godowns(id)
);
```

### 3.3 Multi-Godown Master
```sql
CREATE TABLE IF NOT EXISTS stock_godowns (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    code TEXT UNIQUE NOT NULL,             -- e.g. "MAIN", "RM-01", "FG-01"
    name TEXT NOT NULL,                    -- "Main Godown", "Raw Material Silo 1", "Paddy Yard"
    is_system INTEGER DEFAULT 0,
    address TEXT,
    is_active INTEGER DEFAULT 1
);
```

---

## 4. Cost Accounting & Joint-Cost Apportionment Algorithms

In manufacturing, raw material costs and direct expenses (electricity, steam, direct labour) must be mathematically apportioned across multiple joint products and by-products.

### 4.1 Method 1: Net Realizable Value (NRV) / Sales Value Basis
Recommended for co-products of high relative value (e.g. Edible Oil & Oil Cake, or Rice & Bran):
$$\text{Total Joint Cost} = \sum \text{Input Costs} + \sum \text{Direct Expenses}$$
$$\text{Sales Value}_i = \text{Output Weight}_i \times \text{Estimated Market Rate}_i$$
$$\text{Allocated Cost}_i = \text{Total Joint Cost} \times \left( \frac{\text{Sales Value}_i}{\sum_{k} \text{Sales Value}_k} \right)$$
$$\text{Unit Cost per Qtl}_i = \frac{\text{Allocated Cost}_i}{\text{Output Weight}_i}$$

### 4.2 Method 2: By-Product Cost Deduction
Standard for processes yielding minor by-products with definite scrap value (e.g., Rice Husk or Ginning Trash):
$$\text{Net Cost to Main Products} = \text{Total Joint Cost} - \sum (\text{Byproduct Qty} \times \text{Net Realizable Byproduct Rate})$$
The remaining cost is then apportioned solely among the primary finished items.

### 4.3 Method 3: Physical Weight Proportion
Used when items share equivalent intrinsic value per unit weight (e.g. graded outputs or chemical fractions):
$$\text{Allocated Cost}_i = \text{Total Joint Cost} \times \left( \frac{\text{Output Weight}_i}{\sum_{k} \text{Output Weight}_k} \right)$$

---

## 5. Seeded Industry Profiles

| Industry Profile | Input Materials | Finished Co-Products | By-Products & Scrap | Process Stages |
|---|---|---|---|---|
| **Rice Milling** | Paddy (Basmati / Non-Basmati) | Head Rice, Broken Rice (Tibbar/Dubar) | Rice Bran, Paddy Husk, Nakku | Cleaning $\to$ Parboiling/Drying $\to$ Dehusking $\to$ Polishing $\to$ Grading |
| **Cotton Ginning & Pressing** | Raw Seed Cotton (Kapas) | Cotton Lint (Bales) | Cotton Seed (Binola), Cotton Trash | Ginning $\to$ Lint Cleaning $\to$ Baling Press |
| **Edible Oil Extraction** | Mustard / Soy / Sunflower Seed | Raw Expeller Oil, Filtered Oil | Oil Cake (Khal / Meal) | Seed Pre-clean $\to$ Expeller Press $\to$ Filter Press |
| **Flour Milling (Atta Chakkis)** | Whole Wheat | Atta, Maida, Sooji | Choker (Wheat Bran) | Pre-cleaning $\to$ Conditioning $\to$ Roller Milling $\to$ Sifting |
| **Feed / Blending** | Grains, Cake, Minerals, Molasses | Cattle Feed / Poultry Pellets | Dust / Sweepings | Batch Weighing $\to$ Grinding $\to$ Mixing $\to$ Pelletizing |

---

## 6. Compatibility & Migration Guarantees

1. **Zero-Breakage Backward Compatibility**:
   - `milling_batches` and `milling_voucher_items` are preserved.
   - For firms with `feature.mill = true`, the system creates compatibility views or transparently dual-writes to `production_vouchers` where `template_id` corresponds to the default Rice Milling template.
2. **Deterministic Inventory Postings**:
   - Every `CONSUMED` line generates a `stock_transactions` entry with `trans_type = 'Outward'` (`Cr`).
   - Every `PRODUCED` or `BYPRODUCT` line generates a `stock_transactions` entry with `trans_type = 'Inward'` (`Dr`).
   - Stamped with `godown_id`, `lot_no`, `batch_no`, and the exact calculated unit rate.
3. **Audit & FY Locks**:
   - Production vouchers strictly obey FY `is_locked` rules and emit detailed JSON entries into `audit_log`.

---

## 7. Delivery Checklist

- [ ] **DB DDL**: Create `stock_godowns`, `production_templates`, `production_template_items`, `production_vouchers`, `production_voucher_items`.
- [ ] **Engine**: Implement `ProductionCostEngine` with NRV, By-product subtraction, and Weight methods.
- [ ] **Template Model**: Implement `ProductionTemplateModel` with default seeds for Rice, Cotton, and Oil.
- [ ] **Controller**: Implement `ProductionVoucherController` with live outturn validation, auto-yield computation, and multi-godown stock postings.
- [ ] **UI View**: Implement generic `ProductionVoucherWidget` supporting recipe selection, keyboard-first navigation, and real-time yield metric cards.
