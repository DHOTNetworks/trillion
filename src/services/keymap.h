#pragma once

#include <QKeySequence>
#include <QVector>
#include <QString>

/// @brief Single source of truth for every global keyboard shortcut.
///
/// Tally/Busy semantics: F-keys navigate to voucher kinds from anywhere,
/// Ctrl+S saves on every voucher view, Esc always backs out, badges and the
/// shortcuts dialog both render FROM this table so neither can lie.
/// Views are MainWindow stacked indices (see MainWindow::currentViewIndex).
namespace ErpKeymap {

struct Binding {
    QKeySequence key;   // exact key chord
    const char* action; // stable id: "nav.sales", "voucher.save", ...
    const char* label;  // human label for dialog + badge
    const char* views;  // "" = global, else comma list of view indices
};

inline QVector<Binding> bindings() {
    return {
        // --- Voucher navigation (Tally F-key muscle, global) ---
        { QKeySequence(Qt::Key_F4),  "nav.contra",   "Contra / Bank Voucher",  "" },
        { QKeySequence(Qt::Key_F7),  "nav.journal",  "Journal Voucher",        "" },
        { QKeySequence(Qt::Key_F8),  "nav.sales",    "Sales Invoice",          "" },
        { QKeySequence(Qt::Key_F9),  "nav.purchase", "Purchase Bill",          "" },
        { QKeySequence(Qt::Key_F10), "nav.jform",    "J-Form Procurement",     "" },
        { QKeySequence(Qt::Key_F11), "nav.iform",    "I-Form Buyer Issue",     "" },
        { QKeySequence(Qt::Key_F12), "nav.stock",    "Stock Register",         "" },
        // --- Date / period ---
        { QKeySequence(Qt::Key_F2),              "voucher.date",  "Voucher / Statement Date", "" },
        { QKeySequence(Qt::ALT | Qt::Key_F2),    "app.period",    "Accounting Period / FY",   "" },
        { QKeySequence(Qt::ALT | Qt::Key_F1),    "app.firm",      "Firm / Company Selector",  "" },
        // --- Voucher actions (every voucher view) ---
        { QKeySequence(Qt::CTRL | Qt::Key_S), "voucher.save",   "Save Voucher",   "" },
        { QKeySequence(Qt::CTRL | Qt::Key_D), "voucher.delete", "Delete Voucher", "" },
        { QKeySequence(Qt::Key_Delete),       "row.delete",     "Delete Row",     "" },
        { QKeySequence(Qt::Key_Insert),       "row.insert",     "Insert Row",     "" },
        // --- Reports (Balance Sheet=29, P&L=30 only) ---
        { QKeySequence(Qt::Key_F5), "report.expand",   "Expand All Groups",   "29,30" },
        { QKeySequence(Qt::Key_F6), "report.collapse", "Collapse All Groups", "29,30" },
        // --- Print / export ---
        { QKeySequence(Qt::CTRL | Qt::Key_P), "app.print", "Print Active View",      "" },
        { QKeySequence(Qt::ALT  | Qt::Key_P), "app.print", "Print Active View",      "" },
        { QKeySequence(Qt::CTRL | Qt::Key_E), "app.pdf",   "Export PDF",             "" },
        { QKeySequence(Qt::ALT  | Qt::Key_E), "app.csv",   "Export Excel / CSV",     "" },
        // --- Search / help / back ---
        { QKeySequence(Qt::CTRL | Qt::Key_F), "app.search", "Focus Search",          "8" },
        { QKeySequence(Qt::Key_F1),           "app.help",   "Shortcuts Help",        "" },
        { QKeySequence(Qt::Key_Escape),       "app.back",   "Back / Close",          "" },
    };
}

/// View indices that behave as voucher entry forms (save/delete/date apply).
inline bool isVoucherView(int vIdx) {
    switch (vIdx) {
    case 14: case 15: case 16: case 17: case 18: case 19:
    case 24: case 28: case 31: case 60:
        return true;
    default:
        return false;
    }
}

/// Navigation target per F-key action id. Returns -1 for non-nav actions.
inline int navTargetFor(const char* actionId) {
    QString a = QString::fromLatin1(actionId);
    if (a == "nav.contra")   return 16;
    if (a == "nav.journal")  return 17;
    if (a == "nav.sales")    return 14;
    if (a == "nav.purchase") return 15;
    if (a == "nav.jform")    return 18;
    if (a == "nav.iform")    return 19;
    if (a == "nav.stock")    return 13;
    if (a == "app.firm")     return 22;
    return -1;
}

} // namespace ErpKeymap
