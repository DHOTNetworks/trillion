# MahadevERP Native QtWidgets UI & Design System Rules

This document establishes the mandatory design, layout, styling, custom helper usage, and keyboard navigation rules for all native C++20 Qt 6 QtWidgets across the Mahadev Rice Mill ERP codebase.

---

## 1. Zero Unstyled / Palette Invariant (Mandatory Contrast Rule)
* **Never rely on OS default palettes**: On macOS and Windows, system palettes in dark mode or dynamic appearance will cause light-on-light (invisible text) or unstyled gray blocks if background colors are set without explicit foreground colors.
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

## 2. Standard 4-Tier Screen Hierarchy

Every full-screen native widget in MahadevERP must strictly follow the standard 4-Tier layout:

```
+-----------------------------------------------------------------------------------------+
| Tier 1: Header Bar Card (Title, Subtitle, KbdBadgeButtons: [Back], [Save], [PDF], etc.)|
+-----------------------------------------------------------------------------------------+
| Tier 2: Filter & Control Bar Card (As on Date, Search [Ctrl+F], Status Badge, FY Alt+F2)|
+-----------------------------------------------------------------------------------------+
| Tier 3: High-Performance Data Surface (QTableWidget / QTreeView with Dark Navy Headers)  |
|                                                                                         |
|                                                                                         |
+-----------------------------------------------------------------------------------------+
| Tier 4: Summary Metrics Footer Cards (Total Items, Bags, Weight, Grand Valuation ₹)    |
+-----------------------------------------------------------------------------------------+
```

### Tier 1: Header Bar Card
* `QFrame` container: `background-color: #FFFFFF; border: 1px solid #E2E8F0; border-radius: 8px;`
* Title: `font-size: 16px; font-weight: 800; color: #0F172A; font-family: 'Segoe UI', -apple-system, BlinkMacSystemFont, Roboto, sans-serif;`
* Subtitle: `font-size: 11px; color: #64748B;`
* Action buttons: Use `KbdBadgeButton` with standard color accents.

### Tier 2: Filter & Control Bar Card
* Height: `52px` to `58px`.
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
* Row height: `30px` (or `28px` for compact ledgers).
* Selection mode: `QAbstractItemView::SelectRows` + `QAbstractItemView::SingleSelection`.

### Tier 4: Summary Metrics Footer Cards
* 4 distinct cards in horizontal layout:
  * Left border accent: `4px solid <AccentColor>`
  * Upper label: `font-size: 10px; font-weight: 800; color: #64748B; letter-spacing: 0.5px; text-transform: uppercase;`
  * Value label: `font-size: 16px; font-weight: 800; color: <AccentColor>;`
  * Grand Total card: Given stretch factor 2 and Emerald `#16A34A` accent.

---

## 3. Keyboard-First Architecture
* Every primary action must have a designated keyboard shortcut:
  * `Esc` or `Alt+Left`: Return to Dashboard.
  * `F5`: Refresh data / Recalculate live stock.
  * `Ctrl+S` or `Alt+S`: Save / Lock snapshot.
  * `Alt+Del` or `Alt+D`: Delete / Reset.
  * `Alt+P`: Export PDF.
  * `Alt+E`: Export Excel / CSV.
  * `Ctrl+F`: Focus search filter box.
  * `Alt+F2`: Open Accounting Period dialog.
* Implement `keyPressEvent(QKeyEvent* event)` on every custom widget class.
* Action buttons must visually display their shortcut badge via `KbdBadgeButton`.

---

## 4. Financial Formatting & Alignment Rules
* **Text / Descriptions / Party Names**: Left-aligned (`Qt::AlignLeft | Qt::AlignVCenter`), bold Segoe UI.
* **Codes / Dates / Units / Voucher Numbers**: Center-aligned (`Qt::AlignCenter`).
* **Quantities / Bags / Weights**: Right-aligned (`Qt::AlignRight | Qt::AlignVCenter`), 2 decimal places for weights (`'f', 2`).
* **Currency & Valuation Amounts**: Right-aligned, formatted with `AccountingEngine::formatIndianCurrency(val, true)` (e.g. `₹42,79,96,193.95`).

---

## 5. Standard Custom Helpers & Components Guide

All new and refactored UI components MUST use the project's native custom helpers instead of raw Qt primitives:

### A. Dialogs & Messages: Use `CustomMessageBox` and `CustomInputDialog`
* **Never use raw `QMessageBox`** (which renders unstyled standard OS dialogs).
* **Information dialog**:
  ```cpp
  CustomMessageBox::information(this, "Title", "Formatted informational message.");
  ```
* **Critical / Error dialog**:
  ```cpp
  CustomMessageBox::critical(this, "Save Failed", QString("Error: %1").arg(errorMsg));
  ```
* **Confirmation prompt**:
  ```cpp
  bool confirmed = CustomMessageBox::question(this, "Confirm Action", "Are you sure?", "Yes, Proceed", "Cancel");
  ```
* **Themed input prompt**:
  ```cpp
  bool ok = false;
  QString text = CustomInputDialog::getText(this, "New Group", "Enter Group Name:", "", &ok);
  ```

### B. Fiscal Year & Accounting Period: Use `FiscalYearHelper` & `AccountingPeriodDialog`
* **Get Active Financial Year**:
  ```cpp
  FiscalYearInfo fy = FiscalYearHelper::getActiveFiscalYear();
  // fy.name ("2025-2026"), fy.startDate ("2025-04-01"), fy.endDate ("2026-03-31")
  ```
* **Global Period Selector Trigger (`Alt+F2`)**:
  ```cpp
  QString fIso, tIso, fyLabel;
  bool applied = AccountingPeriodDialog::selectAndApplyGlobalPeriod(this, &fIso, &tIso, &fyLabel);
  if (applied) {
      reloadData();
  }
  ```
* **Date Boundary Clamping**:
  ```cpp
  bool inYear = FiscalYearHelper::isDateInActiveYear(targetDate);
  ```

### C. Fast Date Inputs: Use `AccountingDateEdit` & `VoucherDateDialog`
* **`AccountingDateEdit`**: High-speed numeric typing with automatic slash/dash auto-formatting, date clamping, and calendar popup.
* **`VoucherDateDialog`**: Compact calendar modal for vouchers (`F2` shortcut).

### D. Party & Account Auto-Complete: Use `AccountSearchBox` & `PartySearchWidget`
* For selecting accounts/ledgers with real-time fuzzy search, keyboard navigation (`Up`/`Down`/`Enter`), and current balance badge preview:
  ```cpp
  AccountSearchBox* search = new AccountSearchBox(parent);
  connect(search, &AccountSearchBox::accountSelected, this, &MyWidget::onPartySelected);
  ```

### E. Grid Item Master Delegates: Use `ItemSearchDelegate`
* For embedded item selection in `QTableWidget` / `QTableView` with live stock balance, unit, and standard rates.

### F. Action Buttons with Badges: Use `KbdBadgeButton`
* For all toolbar and header action buttons:
  ```cpp
  KbdBadgeButton* btn = new KbdBadgeButton("Save", "Ctrl+S", QColor("#16A34A"), QColor("#15803D"), QColor("#FFFFFF"), QColor("#16A34A"), parent);
  ```

### G. Printing & File Exports: Use `PrintExportController`
* For PDF report rendering, CSV table export, and opening files in OS:
  ```cpp
  if (m_printCtrl) {
      QString outPath = m_printCtrl->export_stock_register_pdf(fromDate, toDate, targetPath);
      m_printCtrl->open_file_in_os(outPath);
  }
  ```

---

## 6. Main Window Stack & Navigation Integration Checklist
When introducing a new native QtWidgets view (e.g., `View N`):
1. **Instantiation in `MainWindow::MainWindow`**: Add instance to `m_stackedWidget` and connect `backRequested` signal to `navigateToView(0)`.
2. **Handle in `MainWindow::checkQmlView()`**: Add `else if (vIdx == N && m_stackedWidget->currentWidget() != m_targetWidget) { navigateToView(N); }`.
3. **Exclude from QML Fallback in `checkQmlView()`**: Add `vIdx != N` to the `else if` condition before `m_qmlContainer`.
4. **Register in `MainWindow::currentViewIndex()`**: Add `if (cur == m_targetWidget) return N;`.
5. **Register in `MainWindow::restoreActiveViewFocus()`**: Add `else if (vIdx == N && m_targetWidget) { m_targetWidget->setFocus(Qt::OtherFocusReason); }`.
6. **Register in `DashboardWidget` Menus**: Add menu items in `dashboard_widget.cpp` (`openStockMenu`, `openReportsMenu`, etc.).
