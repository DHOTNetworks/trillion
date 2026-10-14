#include "balance_sheet_report.h"

namespace PrintReports {

QString renderBalanceSheetExcelHtml(const QString& asOnDate) {
    // Excel-dedicated variant: same figures as print/PDF today.
    // Customize here per target (raw numbers + num/date classes)
    // without touching the print layout in balance_sheet_report.cpp.
    return renderBalanceSheetHtml(asOnDate);
}

} // namespace PrintReports
