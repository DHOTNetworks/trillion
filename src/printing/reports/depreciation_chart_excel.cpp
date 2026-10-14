#include "depreciation_chart_report.h"

namespace PrintReports {

QString renderDepreciationChartExcelHtml(const QString& asOnDate) {
    // Excel-dedicated variant: same figures as print/PDF today.
    // Customize here per target (raw numbers + num/date classes)
    // without touching the print layout in depreciation_chart_report.cpp.
    return renderDepreciationChartHtml(asOnDate);
}

} // namespace PrintReports
