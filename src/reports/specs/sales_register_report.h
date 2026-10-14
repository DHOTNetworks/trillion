#pragma once

// Sales Register — canonical spec. Shared by sales_register_widget.* and
// PrintExportController::renderSalesRegisterHtml. Six T4 cards (spec allows
// >4 for registers with qty + money totals); every other report uses 4.
#include "../report_spec.h"

namespace ErpReports {

inline const ReportSpec* salesRegisterSpec() {
    static const ReportSpec* s = registerOnce(ReportSpec{
        ReportId::SalesRegister,
        "sales-register",
        "Sales Invoices Register",
        "Customer invoices, dispatch billing and output GST \u2022 Enter alters invoice",
        {"Esc", "F2", "F8", "Ctrl+F", "Ctrl+P", "Alt+P"},
        {ExportKind::Pdf, ExportKind::Odf, ExportKind::Excel, ExportKind::Csv},
        {
            {"invoice_no","Invoice No",    ColumnAlign::Center, false, false},
            {"date",      "Date",          ColumnAlign::Center, false, false},
            {"customer",  "Customer / Buyer", ColumnAlign::Left, true, false},
            {"commodity", "Commodity / Item", ColumnAlign::Left, false, false},
            {"bags",      "Bags",          ColumnAlign::Right,  false, false},
            {"weight",    "Weight (Qtl)",  ColumnAlign::Right,  false, false},
            {"rate",      "Rate (\u20B9)", ColumnAlign::Right,  false, true},
            {"taxable",   "Taxable (\u20B9)", ColumnAlign::Right, false, true},
            {"gst",       "GST (\u20B9)",  ColumnAlign::Right,  false, true},
            {"total",     "Total (\u20B9)", ColumnAlign::Right, false, true},
            {"vehicle",   "Vehicle No",    ColumnAlign::Center, false, false},
        },
        6,
    });
    return s;
}

} // namespace ErpReports
