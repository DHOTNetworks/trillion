#include "day_book_report.h"

namespace PrintReports {

QString renderDayBookExcelHtml(const QString& fromDate, const QString& toDate) {
    // Excel-dedicated variant: same figures as print/PDF today.
    // Customize here per target (raw numbers + num/date classes)
    // without touching the print layout in day_book_report.cpp.
    return renderDayBookHtml(fromDate, toDate);
}

} // namespace PrintReports
