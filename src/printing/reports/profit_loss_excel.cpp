#include "profit_loss_report.h"

namespace PrintReports {

QString renderProfitLossExcelHtml(const QString& fromDate, const QString& toDate) {
    // Excel-dedicated variant: same figures as print/PDF today.
    // Customize here per target (raw numbers + num/date classes)
    // without touching the print layout in profit_loss_report.cpp.
    return renderProfitLossHtml(fromDate, toDate);
}

} // namespace PrintReports
