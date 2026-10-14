#include "iform_voucher_report.h"

namespace PrintReports {

QString renderIFormVoucherExcelHtml(const QString& voucherNo) {
    // Excel-dedicated variant: same figures as print/PDF today.
    // Customize here per target (raw numbers + num/date classes)
    // without touching the print layout in iform_voucher_report.cpp.
    return renderIFormVoucherHtml(voucherNo);
}

} // namespace PrintReports
