#include "closing_stock_report.h"

namespace PrintReports {

QString renderClosingStockExcelHtml(const QString& asOnDate) {
    // Excel-dedicated variant: same figures as print/PDF today.
    // Customize here per target (raw numbers + num/date classes)
    // without touching the print layout in closing_stock_report.cpp.
    return renderClosingStockHtml(asOnDate);
}

} // namespace PrintReports
