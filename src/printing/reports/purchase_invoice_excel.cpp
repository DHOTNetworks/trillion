#include "purchase_invoice_report.h"

namespace PrintReports {

QString renderPurchaseInvoiceExcelHtml(const QString& invoiceNo) {
    // Excel-dedicated variant: same figures as print/PDF today.
    // Customize here per target (raw numbers + num/date classes)
    // without touching the print layout in purchase_invoice_report.cpp.
    return renderPurchaseInvoiceHtml(invoiceNo);
}

} // namespace PrintReports
