#pragma once

// Ledger Statement — canonical spec. Note the current widget renders a split
// Dr+Cr T-account; the spec locks the SINGLE-table contract (spec §3 T3) as
// the migration target: Date | Particulars | Vch Type/No | Debit | Credit |
// Balance. The split view stays available only as a print variant.
#include "../report_spec.h"

namespace ErpReports {

inline const ReportSpec* ledgerStatementSpec() {
    static const ReportSpec* s = registerOnce(ReportSpec{
        ReportId::LedgerStatement,
        "ledger-statement",
        "Ledger Statement",
        "Party / account running balance with opening, period and closing \u2022 Enter drills to voucher",
        {"Esc", "F2", "F5", "Ctrl+F", "Alt+P", "Alt+E"},
        {ExportKind::Pdf, ExportKind::Odf, ExportKind::Excel, ExportKind::Csv},
        {
            {"date",      "Date",               ColumnAlign::Center, false, false},
            {"particulars","Particulars",       ColumnAlign::Left,   true,  false},
            {"vch",       "Vch Type / No",      ColumnAlign::Left,   false, false},
            {"debit",     "Debit (Dr)",         ColumnAlign::Right,  false, true},
            {"credit",    "Credit (Cr)",        ColumnAlign::Right,  false, true},
            {"balance",   "Balance",            ColumnAlign::Right,  false, true},
        },
        4,
    });
    return s;
}

} // namespace ErpReports
