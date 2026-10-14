#include "tds_voucher_report.h"

namespace PrintReports {

QString renderTdsVoucherExcelHtml(const QString& voucherNo) {
    // Excel-dedicated variant: same figures as print/PDF today.
    // Customize here per target (raw numbers + num/date classes)
    // without touching the print layout in tds_voucher_report.cpp.
    return renderTdsVoucherHtml(voucherNo);
}

} // namespace PrintReports
