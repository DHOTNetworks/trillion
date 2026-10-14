#include "purchase_register_report.h"

namespace PrintReports {

QString renderPurchaseRegisterExcelHtml(const QString& fromDate, const QString& toDate) {
    // Excel-dedicated variant: same figures as print/PDF today.
    // Customize here per target (raw numbers + num/date classes)
    // without touching the print layout in purchase_register_report.cpp.
    return renderPurchaseRegisterHtml(fromDate, toDate);
}

} // namespace PrintReports
