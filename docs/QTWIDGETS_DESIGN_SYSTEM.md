# MahadevERP Native QtWidgets UI & Design System Rules

This document establishes the mandatory design, layout, styling, custom helper usage, single-slate architecture, and keyboard navigation rules for all native C++20 Qt 6 QtWidgets across the Mahadev Rice Mill ERP codebase.

---

## 1. Strict Single-Slate Architecture (Zero-Scroll Rule)

* **Zero Unnecessary Scrolling**: Primary master creation/alteration views (Items, Groups, Ledgers, Firms) and voucher entry forms (Sales, Purchase, Mandi, Milling, Payment, Journal, TDS, Notes) **must be strictly Single-Slate**. Users must be able to view, fill, and submit all fields on a standard desktop resolution (`1024x768` to `1920x1080`) without vertical scrolling.
* **Layout Organization**:
  * Organize input forms into 2-column or 3-column side-by-side grouped cards (`QHBoxLayout` containing structured `QFrame` cards) instead of stacked vertical sections.
  * Use compact form grids (`QGridLayout` with `8px` vertical spacing, `10px` horizontal spacing).
  * Group logically into:
    1. **Identity & Classification** (Names, Codes, Categories, Units).
    2. **Pricing, Tax & Statutory Matrix** (Rates, HSN/SAC, GST %, Cess, Discounts).
    3. **Opening Balances, Valuation & Ledger Mapping** (Quantities, Rates, Computed Values, Default Accounts).
* **Sticky Header & Action Footer**:
  * Header and Action Footer are fixed at top and bottom.
  * Content area takes remaining space with stretch factor 1.

---

## 2. Zero Unstyled / Palette Invariant (Mandatory Contrast Rule)

* **Never rely on OS default palettes**: On macOS and Windows, system palettes in dark mode or dynamic appearance cause light-on-light (invisible text) or unstyled gray blocks if background colors are set without explicit foreground colors.
* **Explicit Item Foregrounds**:
  * Every `QTableWidgetItem` must explicitly have its foreground set:
    ```cpp
    item->setForeground(QBrush(QColor("#0F172A"))); // Dark slate text
    ```
  * Editable cells should have a distinct subtle background:
    ```cpp
    item->setBackground(QBrush(QColor("#F8FAFC")));
    item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable);
    ```
  * Currency & positive amounts: `#16A34A` (Emerald green).
  * Debit / negative / danger amounts: `#DC2626` (Crimson red).

---

## 3. Standard 4-Tier Screen Hierarchy (Registers & Reports)

Every full-screen register and reporting view in MahadevERP must strictly follow the standard 4-Tier layout:

```
+-----------------------------------------------------------------------------------------+
| Tier 1: Header Bar Card (Title, Subtitle, KbdBadgeButtons: [Back], [Save], [PDF], etc.)|
+-----------------------------------------------------------------------------------------+
| Tier 2: Filter & Control Bar Card (From/To Date, Search [Ctrl+F], FY Badge, Period F2) |
+-----------------------------------------------------------------------------------------+
| Tier 3: High-Performance Data Surface (QTableWidget with Dark Navy #0F172A Headers)    |
|                                                                                         |
+-----------------------------------------------------------------------------------------+
| Tier 4: Summary Metrics Footer Cards (4 distinct cards with left colored stripe borders)|
+-----------------------------------------------------------------------------------------+
```

### Tier 1: Header Bar Card
* `QFrame` container: `background-color: #FFFFFF; border: 1px solid #E2E8F0; border-radius: 8px;`
* Title: `font-size: 16px; font-weight: 800; color: #0F172A;`
* Subtitle: `font-size: 11px; color: #64748B;`
* Action buttons: Use `KbdBadgeButton` with standard color accents.

### Tier 2: Filter & Control Bar Card
* Height: `48px` to `54px`.
* Controls (`QDateEdit`, `QComboBox`, `QLineEdit`, `AccountingDateEdit`):
  * `background-color: #FFFFFF; color: #0F172A; border: 1.5px solid #CBD5E1; border-radius: 6px; padding: 4px 10px; font-weight: 700; font-size: 12px;`
  * Focus state: `border: 2px solid #2563EB; background-color: #FFFFF0;`
* Status badges:
  * Active / Calculated: `background-color: #F0F9FF; color: #0284C7; border: 1.5px solid #BAE6FD;`
  * Locked / Audited: `background-color: #F0FDF4; color: #16A34A; border: 1.5px solid #86EFAC;`

### Tier 3: Data Surface (Table / Tree)
* Table style:
  ```css
  QTableWidget {
    background-color: #FFFFFF;
    alternate-background-color: #F8FAFC;
    border: 1px solid #CBD5E1;
    border-radius: 6px;
    gridline-color: #E2E8F0;
    font-size: 12px;
    font-family: 'Segoe UI', -apple-system, BlinkMacSystemFont, Roboto, sans-serif;
    color: #0F172A;
  }
  QTableWidget::item:selected {
    background-color: #2563EB;
    color: #FFFFFF;
    font-weight: 700;
  }
  QHeaderView::section {
    background-color: #0F172A;
    color: #FFFFFF;
    font-weight: 700;
    font-size: 12px;
    padding: 8px 8px;
    border: none;
    border-right: 1px solid #334155;
  }
  ```
* Row height: `28px` to `30px`.
* Selection mode: `QAbstractItemView::SelectRows` + `QAbstractItemView::SingleSelection`.

### Tier 4: Summary Metrics Footer Cards
* 4 distinct cards in horizontal layout:
  * Left border accent: `4px solid <AccentColor>`
  * Upper label: `font-size: 10px; font-weight: 800; color: #64748B; letter-spacing: 0.5px; text-transform: uppercase;`
  * Value label: `font-size: 16px; font-weight: 800; color: <AccentColor>;`
  * Grand Total card: Given stretch factor 2 and Emerald `#16A34A` accent.

---

## 4. Keyboard-First Architecture
* Every primary action must have a designated keyboard shortcut:
  * `Esc`: Return to Dashboard.
  * `F2`: Contextual Date / Accounting Period dialog.
  * `F5`: Refresh data / Clear form.
  * `Ctrl+S` or `F2`: Save voucher / Master.
  * `Alt+Del` or `Del`: Delete entry.
  * `Alt+P` or `Ctrl+P`: Export / Print PDF.
  * `Alt+E`: Export CSV.
  * `Ctrl+F`: Focus search filter box.
  * `Alt+F2`: Open Accounting Period dialog.
* Implement `keyPressEvent(QKeyEvent* event)` on every custom widget class.
* Action buttons must visually display their shortcut badge via `KbdBadgeButton`.

---

## 5. Standard Custom Helpers & Global Fiscal Year Synchronization

All native QtWidgets MUST use the project's native custom helpers:

### A. Global Fiscal Year Synchronization (`FiscalYearHelper`)
* Every view MUST initialize its date range from `FiscalYearHelper::getActiveFiscalYear()`.
* Every view must handle period changes when `AccountingPeriodDialog::selectAndApplyGlobalPeriod()` is invoked.
  ```cpp
  FiscalYearInfo fy = FiscalYearHelper::getActiveFiscalYear();
  m_fromDateEdit->setDate(QDate::fromString(fy.startDate, "yyyy-MM-dd"));
  m_toDateEdit->setDate(QDate::fromString(fy.endDate, "yyyy-MM-dd"));
  ```

### B. Dialogs & Messages (`CustomMessageBox` & `CustomInputDialog`)
* Never use raw unstyled `QMessageBox`. Use `CustomMessageBox::information`, `CustomMessageBox::critical`, `CustomMessageBox::question`.

### C. Party & Account Auto-Complete (`AccountSearchBox`)
* Use `AccountSearchBox` for searching and picking ledger accounts with live balance badge.

### D. Grid Item Master Delegates (`ItemSearchDelegate`)
* Use `ItemSearchDelegate` for fast inline item/commodity search in table rows.

### E. Action Buttons with Badges (`KbdBadgeButton`)
* For toolbar action buttons displaying their keyboard shortcuts.

### F. Print & Exports (`PrintExportController`)
* For PDF report generation, CSV table exports, and opening files in OS viewer.
