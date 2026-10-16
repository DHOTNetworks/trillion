#include "manufacturing_statement_report.h"

namespace PrintReports {

QString renderManufacturingStatementExcelHtml(const QString& fromDate, const QString& toDate) {
    // Excel-dedicated variant: same figures as print/PDF today.
    // Customize here per target (raw numbers + num/date classes)
    // without touching the print layout in manufacturing_statement_report.cpp.
    return renderManufacturingStatementHtml(fromDate, toDate);
}

} // namespace PrintReports
