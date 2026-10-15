# Phase 2: Generic Account Groups & Seed Data

## Goal
Replace the Bahi-Khata-clone group/ledger seeds with a generic Tally-compatible
chart of accounts. Mandi-specific groups become opt-in domain seeds, never defaults.

## Current State (Verified)
- `src/database_manager.cpp:1262-1311` seeds ~37 groups on empty DB, including
  mandi-specific: `Zimidara Debtors (32)`, `Zimidara Creditors (33)`,
  `Mandi Debtors (34)`, `Local Mandi Creditors (31)`, `Commission Basis Parties A/c (21)`,
  `Manufacturing Group (22)`, `Manufacturing Exp. (23)`, `Trading Exp. (24)`,
  plus joke/legacy row `Isht Dev (-1)`.
- Seed is all-or-nothing: every new firm — kirana store, clinic, trader — gets
  paddy/mandi groups it can never use.
- `parties` table itself is generic already (name/alias/group/gst/bank/credit
  fields) — keep the table, change only the seed rows + lookup defaults.

## Target State
- Fresh DB seeds the 13 Tally primary groups + standard sub-groups only.
- Mandi/manufacturing groups seed ONLY via `ensureDomainGroups("mandi"|"mill")`
  when the feature flag is on (Phase 3).
- `parties` seed: Cash, Bank, P&L, TDS/TCS/GST payables only (no mandi parties).

## Exact Edits

### 1. `src/database_manager.cpp:1262-1311` — replace seed block
```cpp
// Generic Tally-compatible chart. code1st values are OUR canonical ids
// (do NOT reuse Bahi-Khata code1st numbers for new meanings — legacy_*
// columns carry migration identity instead).
static const BahiGroupSeed defaults[] = {
    // primary (parent = self-ish "Primary" root row first)
    {0, 0, 0, 0, "Primary", "Primary", "Assets", 1},
    {1, 0, 0, 0, "Capital Account", "Primary", "Liabilities", 1},
    {2, 0, 0, 0, "Loans (Liability)", "Primary", "Liabilities", 1},
    {3, 0, 0, 0, "Current Liabilities", "Primary", "Liabilities", 1},
    {4, 0, 0, 0, "Fixed Assets", "Primary", "Assets", 1},
    {5, 0, 0, 0, "Investments", "Primary", "Assets", 1},
    {6, 0, 0, 0, "Current Assets", "Primary", "Assets", 1},
    {7, 0, 0, 0, "Branch / Divisions", "Primary", "Liabilities", 1},
    {8, 0, 0, 0, "Suspense A/c", "Primary", "Liabilities", 0},
    {9, 0, 0, 0, "Misc. Expenses (Asset)", "Primary", "Assets", 0},
    // current-liability children
    {10, 3, 0, 0, "Sundry Creditors", "Current Liabilities", "Liabilities", 1},
    {11, 3, 0, 0, "Duties & Taxes", "Current Liabilities", "Liabilities", 1},
    {12, 3, 0, 0, "Provisions", "Current Liabilities", "Liabilities", 1},
    // current-asset children
    {20, 6, 0, 0, "Sundry Debtors", "Current Assets", "Assets", 1},
    {21, 6, 0, 0, "Cash-in-Hand", "Current Assets", "Assets", 1},
    {22, 6, 0, 0, "Bank Accounts", "Current Assets", "Assets", 1},
    {23, 6, 0, 0, "Deposits (Asset)", "Current Assets", "Assets", 1},
    {24, 6, 0, 0, "Loans & Advances (Asset)", "Current Assets", "Assets", 1},
    {25, 6, 0, 0, "Stock-in-Hand", "Current Assets", "Assets", 0},
    // income / expense roots + standard children
    {30, 0, 0, 0, "Income A/c", "Primary", "Income", 1},
    {31, 0, 0, 0, "Expenditure A/c", "Primary", "Expense", 1},
    {100, 30, 0, 0, "Direct Income", "Income A/c", "Income", 1},
    {101, 30, 0, 0, "Indirect Income", "Income A/c", "Income", 1},
    {200, 31, 0, 0, "Direct Expenses", "Expenditure A/c", "Expense", 1},
    {201, 31, 0, 0, "Indirect Expenses", "Expenditure A/c", "Expense", 1},
};
// DELETE from the old list: Zimidara Debtors/Creditors, Mandi Debtors,
// Local Mandi Creditors, Commission Basis Parties, Manufacturing Group/Exp,
// Trading Exp., Trading Items Stock A/c subtree, Isht Dev.
```

### 2. NEW `src/models/account_groups_model.*` (or extend existing) — `ensureDomainGroups(domain)`
```cpp
// Seeded idempotently (INSERT OR IGNORE by name) when Phase-3 flags turn on:
// "mandi" -> Zimidara Debtors/Creditors, Mandi Debtors, Local Mandi Creditors;
// "mill"  -> Manufacturing Group, Manufacturing Exp.
// Never runs for generic firms.
void ensureDomainGroups(const QString& domain);
```

### 3. Default parties audit
- Grep `INSERT INTO parties` seed blocks in `database_manager.cpp` + migrators;
  keep ONLY: Cash, Bank, Profit & Loss, TDS Payable, TCS Payable, GST Payable
  (create via the same `resolveTcs206CLedger`-style ensure-or-create used for TCS).
- Delete any mandi-party seeds (zimidar/labour/round-off auto-creates stay as
  *runtime* fallbacks in jform/iform models — out of scope here).

### 4. `src/models/account_classifier.*` + `group_hierarchy_pipeline.*`
- Nature mapping (`Assets/Liabilities/Income/Expense`) must resolve against the
  NEW group names; grep for old names (`"Mandi Debtors"`, `"Trading Exp."`, …)
  and repoint to generic equivalents or delete the branch.
- Trial/P&L/BS calculators read `account_groups` dynamically — verify no
  hardcoded group-name lists remain (grep `"Sundry Debtors"` etc. in
  `src/engine/*calculator*`, `trial_balance_controller.*`).

### 5. Migration safety
- Existing DBs: seed block only runs when `account_groups` is EMPTY, so live
  firms keep their groups untouched. Add a one-time `UPDATE` mapping ONLY for
  exact Bahi-Khata-clone rows on never-used firms? NO — do not rewrite user
  data. Document instead.

## Verification
- Fresh DB: `SELECT name FROM account_groups` == generic list only, zero
  mandi/mill names.
- Trial Balance / P&L / Balance Sheet on empty + sample firm: identical totals
  before/after (group renames must not move a rupee — run EnginesTestSuite).
- `ensureDomainGroups("mandi")` twice → no duplicates (UNIQUE name).
