#include "jform_voucher_report.h"

namespace PrintReports {

QString renderJFormVoucherExcelHtml(const QString& voucherNo) {
    // Excel-dedicated variant: same figures as print/PDF today.
    // Customize here per target (raw numbers + num/date classes)
    // without touching the print layout in jform_voucher_report.cpp.
    return renderJFormVoucherHtml(voucherNo);
}

} // namespace PrintReports
