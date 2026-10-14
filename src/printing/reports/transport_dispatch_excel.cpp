#include "transport_dispatch_report.h"

namespace PrintReports {

QString renderTransportDispatchExcelHtml(int dispatchId) {
    // Excel-dedicated variant: same figures as print/PDF today.
    // Customize here per target (raw numbers + num/date classes)
    // without touching the print layout in transport_dispatch_report.cpp.
    return renderTransportDispatchHtml(dispatchId);
}

} // namespace PrintReports
