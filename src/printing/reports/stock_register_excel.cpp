#include "stock_register_report.h"

namespace PrintReports {

QString renderStockRegisterExcelHtml(const QString& fromDate, const QString& toDate) {
    // Excel-dedicated variant: same figures as print/PDF today.
    // Customize here per target (raw numbers + num/date classes)
    // without touching the print layout in stock_register_report.cpp.
    return renderStockRegisterHtml(fromDate, toDate);
}

} // namespace PrintReports
