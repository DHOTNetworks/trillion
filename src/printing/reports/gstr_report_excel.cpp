#include "gstr_report_report.h"

namespace PrintReports {

QString renderGstrReportExcelHtml(const QString& gstrType, const QString& fromDate, const QString& toDate) {
    // Excel-dedicated variant: same figures as print/PDF today.
    // Customize here per target (raw numbers + num/date classes)
    // without touching the print layout in gstr_report_report.cpp.
    return renderGstrReportHtml(gstrType, fromDate, toDate);
}

} // namespace PrintReports
