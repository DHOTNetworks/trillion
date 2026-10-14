#include "sales_invoice_report.h"

namespace PrintReports {

QString renderSalesInvoiceSingleExcelHtml(const QString& invoiceNo, const QString& copySubtitle) {
    // Excel-dedicated variant: same figures as print/PDF today.
    // Customize here per target (raw numbers + num/date classes)
    // without touching the print layout in sales_invoice_report.cpp.
    return renderSalesInvoiceSingleHtml(invoiceNo, copySubtitle);
}

QString renderSalesInvoiceExcelHtml(const QString& invoiceNo, const QString& copyType) {
    // Excel-dedicated variant: same figures as print/PDF today.
    // Customize here per target (raw numbers + num/date classes)
    // without touching the print layout in sales_invoice_report.cpp.
    return renderSalesInvoiceHtml(invoiceNo, copyType);
}

} // namespace PrintReports
