#pragma once

// Standardized reporting contract — one spec per report, one file per report.
// See docs/erp-roadmap/10_UI_REPORTING_OVERHAUL.md.
//
// Each report in the app (Tally/Busy-competitive surface) gets:
//   1. a spec header in src/reports/specs/<report>_report.h  (columns, title,
//      shortcuts, export set — pure data, no SQL, no HTML),
//   2. a renderer in src/printing/reports/<report>_report.{h,cpp} (HTML,
//      shares chrome from report_html_chrome.h),
//   3. the existing widget keeps owning T1-T4 layout via ErpTheme builders
//      and reads its column contract from the spec.
//
// This header is QtCore-only so specs stay headless-testable.

#include <QMap>
#include <QString>
#include <QStringList>
#include <QVector>

namespace ErpReports {

enum class ReportId {
    TrialBalance,
    LedgerStatement,
    DayBook,
    SalesRegister,
    PurchaseRegister,
    StockRegister,
    ItemMovement,
    ClosingStock,
    ProfitLoss,
    BalanceSheet,
    CapitalAccounts,
    CashBankFlow,
    InterestStatement,   // Aank / interest calculator
    DepreciationChart,
    MillingStatement,
    MandiReport,         // Form M / Form 9 / RDF / Market fee (parameterized by reportType)
    GstrReport,          // GSTR-1 / 2 / 3B (parameterized by gstrType)
    AuditRegister,
    SalaryRegister,
    GateRegister,
    CashBook,
    VoucherRegister,     // numbering / gap audit surface
};

enum class ExportKind { Pdf, Odf, Excel, Csv };

inline QString exportKindName(ExportKind k) {
    switch (k) {
    case ExportKind::Pdf:   return "PDF";
    case ExportKind::Odf:   return "ODF";
    case ExportKind::Excel: return "Excel";
    case ExportKind::Csv:   return "CSV";
    }
    return "PDF";
}

enum class ColumnAlign { Left, Center, Right };

struct ReportColumn {
    QString key;        // stable data key, e.g. "close_debit"
    QString header;     // visible header, e.g. "Debit Balance (Dr)"
    ColumnAlign align = ColumnAlign::Left;
    bool stretch = false; // exactly one stretch column per spec (col 0)
    bool isMoney = false; // right-aligned + tabular figures in print
};

struct ReportSpec {
    ReportId id;
    QString code;        // e.g. "trial-balance" — file names, menu ids
    QString title;       // T1 title
    QString subtitle;    // T1 subtitle (includes shortcut hint)
    QStringList shortcuts; // e.g. {"Esc", "F2", "F4", "Ctrl+F", "Alt+P", "Alt+E"}
    QVector<ExportKind> exports; // supported export targets
    QVector<ReportColumn> columns;
    int summaryCards = 4; // T4 card count (spec default)
};

/// Central registry: widgets, printers and tests resolve the contract from
/// here instead of duplicating header strings in 3 places.
class ReportRegistry {
public:
    static ReportRegistry& instance() {
        static ReportRegistry r;
        return r;
    }

    void registerSpec(const ReportSpec& spec) { m_specs[static_cast<int>(spec.id)] = spec; }

    const ReportSpec* spec(ReportId id) const {
        auto it = m_specs.constFind(static_cast<int>(id));
        return it == m_specs.constEnd() ? nullptr : &(*it);
    }

    QVector<ReportSpec> allSpecs() const { return m_specs.values().toVector(); }
    int count() const { return m_specs.size(); }

private:
    ReportRegistry() = default;
    QMap<int, ReportSpec> m_specs;
};

/// Helper for spec headers: registers on first call, returns the spec.
inline const ReportSpec* registerOnce(const ReportSpec& s) {
    ReportRegistry::instance().registerSpec(s);
    return ReportRegistry::instance().spec(s.id);
}

} // namespace ErpReports
