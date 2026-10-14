#include "mandi_report_report.h"

namespace PrintReports {

QString renderMandiReportExcelHtml(const QString& reportType, const QString& fromDate, const QString& toDate) {
    // Excel-dedicated variant: same figures as print/PDF today.
    // Customize here per target (raw numbers + num/date classes)
    // without touching the print layout in mandi_report_report.cpp.
    return renderMandiReportHtml(reportType, fromDate, toDate);
}

} // namespace PrintReports
