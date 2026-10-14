#include "item_movement_report.h"

namespace PrintReports {

QString renderItemMovementExcelHtml(int itemId, const QString& fromDate, const QString& toDate) {
    // Excel-dedicated variant: same figures as print/PDF today.
    // Customize here per target (raw numbers + num/date classes)
    // without touching the print layout in item_movement_report.cpp.
    return renderItemMovementHtml(itemId, fromDate, toDate);
}

} // namespace PrintReports
