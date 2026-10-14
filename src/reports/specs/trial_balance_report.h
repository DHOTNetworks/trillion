#pragma once

// Trial Balance — canonical spec. The widget (trial_balance_widget.*),
// controller (trial_balance_controller.*) and printer
// (PrintExportController::renderTrialBalanceHtml) must all resolve columns
// from here. View-mode variants (turnover / opening / without-op-bal) only
// ADD columns; base 4 columns are stable.
#include "../report_spec.h"

namespace ErpReports {

inline const ReportSpec* trialBalanceSpec() {
    static const ReportSpec* s = registerOnce(ReportSpec{
        ReportId::TrialBalance,
        "trial-balance",
        "Trial Balance Sheet",
        "General ledger mathematical balancing verification \u2022 Press F4 for View Options",
        {"Esc", "F2", "F4", "Ctrl+F", "Alt+P", "Alt+E"},
        {ExportKind::Pdf, ExportKind::Odf, ExportKind::Excel, ExportKind::Csv},
        {
            {"account",  "Particulars / Account Name", ColumnAlign::Left,   true,  false},
            {"group",    "Group",                      ColumnAlign::Left,   false, false},
            {"debit",    "Debit Balance (Dr)",         ColumnAlign::Right,  false, true},
            {"credit",   "Credit Balance (Cr)",        ColumnAlign::Right,  false, true},
            // mode extensions (ShowTurnover / ShowOpeningBal / WithoutOpBal)
            {"opening",  "Opening Balance",            ColumnAlign::Right,  false, true},
            {"turn_dr",  "Debit Turnover",             ColumnAlign::Right,  false, true},
            {"turn_cr",  "Credit Turnover",            ColumnAlign::Right,  false, true},
            {"closing",  "Closing Balance",            ColumnAlign::Right,  false, true},
        },
        4,
    });
    return s;
}

} // namespace ErpReports
