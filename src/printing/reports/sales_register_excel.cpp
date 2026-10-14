#include "sales_register_report.h"

namespace PrintReports {

QString renderSalesRegisterExcelHtml(const QString& fromDate, const QString& toDate) {
    // Excel-dedicated variant: same figures as print/PDF today.
    // Customize here per target (raw numbers + num/date classes)
    // without touching the print layout in sales_register_report.cpp.
    return renderSalesRegisterHtml(fromDate, toDate);
}

} // namespace PrintReports
