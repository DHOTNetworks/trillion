#include "milling_statement_report.h"

namespace PrintReports {

QString renderMillingStatementExcelHtml(const QString& fromDate, const QString& toDate) {
    // Excel-dedicated variant: same figures as print/PDF today.
    // Customize here per target (raw numbers + num/date classes)
    // without touching the print layout in milling_statement_report.cpp.
    return renderMillingStatementHtml(fromDate, toDate);
}

} // namespace PrintReports
