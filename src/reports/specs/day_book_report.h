#pragma once

// Day Book — canonical spec. Grid is capped newest-first (perf guard, see
// Doc 05) while SQL totals always cover the FULL filter range; the footer
// must state the window explicitly ("Showing latest N of M").
#include "../report_spec.h"

namespace ErpReports {

inline const ReportSpec* dayBookSpec() {
    static const ReportSpec* s = registerOnce(ReportSpec{
        ReportId::DayBook,
        "day-book",
        "Day Book",
        "All vouchers in date order \u2022 totals cover full range even when grid is windowed",
        {"Esc", "F2", "F5", "Ctrl+F", "Alt+P", "Alt+E"},
        {ExportKind::Pdf, ExportKind::Odf, ExportKind::Excel, ExportKind::Csv},
        {
            {"date",      "Date",           ColumnAlign::Center, false, false},
            {"vch_no",    "Voucher No",     ColumnAlign::Left,   false, false},
            {"type",      "Type",           ColumnAlign::Left,   false, false},
            {"party",     "Party / Narration", ColumnAlign::Left, true, false},
            {"debit",     "Debit (Dr)",     ColumnAlign::Right,  false, true},
            {"credit",    "Credit (Cr)",    ColumnAlign::Right,  false, true},
        },
        4,
    });
    return s;
}

} // namespace ErpReports
