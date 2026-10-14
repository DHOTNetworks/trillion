#include "print_export_controller.h"
#include "../database_manager.h"
#include "../engine/accounting_engine.h"
#include "../engine/fiscal_year_helper.h"
#include "../engine/profit_loss_calculator.h"
#include "../engine/balance_sheet_calculator.h"
#include "../models/account_classifier.h"
#include <QPrinter>
#include <QPrintDialog>
#include <QTextDocument>
#include <QTextDocumentWriter>
#include <QPageLayout>
#include <QPageSize>
#include <QMarginsF>
#include <QDesktopServices>
#include <QUrl>
#include <QStandardPaths>
#include <QDir>
#include <QFile>
#include <QTextStream>
#include <QDateTime>
#include <QHash>
#include <QRegularExpression>
#include <QDebug>
#include <cmath>
#include <algorithm>

PrintExportController::PrintExportController(QObject* parent)
    : QObject(parent)
{
}


// Per-report renderers (one module per report — see src/printing/reports/).
#include "../printing/reports/sales_invoice_report.h"
#include "../printing/reports/purchase_invoice_report.h"
#include "../printing/reports/cash_voucher_report.h"
#include "../printing/reports/cheque_voucher_report.h"
#include "../printing/reports/journal_voucher_report.h"
#include "../printing/reports/jform_voucher_report.h"
#include "../printing/reports/iform_voucher_report.h"
#include "../printing/reports/tds_voucher_report.h"
#include "../printing/reports/debit_credit_note_report.h"
#include "../printing/reports/transport_dispatch_report.h"
#include "../printing/reports/ledger_statement_report.h"
#include "../printing/reports/day_book_report.h"
#include "../printing/reports/trial_balance_report.h"
#include "../printing/reports/profit_loss_report.h"
#include "../printing/reports/balance_sheet_report.h"
#include "../printing/reports/capital_accounts_report.h"
#include "../printing/reports/cash_bank_flow_report.h"
#include "../printing/reports/interest_statement_report.h"
#include "../printing/reports/depreciation_chart_report.h"
#include "../printing/reports/sales_register_report.h"
#include "../printing/reports/purchase_register_report.h"
#include "../printing/reports/stock_register_report.h"
#include "../printing/reports/item_movement_report.h"
#include "../printing/reports/closing_stock_report.h"
#include "../printing/reports/milling_statement_report.h"
#include "../printing/reports/mandi_report_report.h"
#include "../printing/reports/gstr_report_report.h"
QString PrintExportController::get_default_reports_dir() const {
    QString docPath = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    if (docPath.isEmpty()) docPath = QDir::currentPath();
    QDir dir(docPath + "/MahadevERP_Reports");
    if (!dir.exists()) {
        dir.mkpath(".");
    }
    return dir.absolutePath();
}

void PrintExportController::open_file_in_os(const QString& filePath) {
    if (filePath.isEmpty() || !QFile::exists(filePath)) return;
    QDesktopServices::openUrl(QUrl::fromLocalFile(filePath));
}

bool PrintExportController::printHtml(const QString& htmlContent, const QString& docTitle, bool landscape) {
    QPrinter printer(QPrinter::ScreenResolution);
    printer.setDocName(docTitle);
    printer.setPageSize(QPageSize(QPageSize::A4));
    printer.setPageOrientation(landscape ? QPageLayout::Landscape : QPageLayout::Portrait);
    printer.setPageMargins(QMarginsF(8, 8, 8, 8), QPageLayout::Millimeter);

    QPrintDialog dialog(&printer, nullptr);
    dialog.setWindowTitle("Print " + docTitle);
    if (dialog.exec() == QDialog::Accepted) {
        QTextDocument doc;
        doc.setDocumentMargin(0);
        doc.setPageSize(printer.pageLayout().paintRectPoints().size());
        doc.setHtml(htmlContent);
        doc.print(&printer);
        m_isSuccess = true;
        m_statusMessage = "Printing sent successfully for: " + docTitle;
        emit statusMessageChanged();
        emit printCompleted(true, m_statusMessage);
        return true;
    }
    m_isSuccess = false;
    m_statusMessage = "Print cancelled by user";
    emit statusMessageChanged();
    return false;
}

QString PrintExportController::exportHtmlToPdf(const QString& htmlContent, const QString& defaultFileName, const QString& customPath, bool landscape) {
    QString outPath = customPath.trimmed();
    if (outPath.isEmpty()) {
        QString dir = get_default_reports_dir();
        outPath = dir + "/" + defaultFileName;
        if (!outPath.endsWith(".pdf", Qt::CaseInsensitive)) {
            outPath += ".pdf";
        }
    }

    QPrinter printer(QPrinter::ScreenResolution);
    printer.setOutputFormat(QPrinter::PdfFormat);
    printer.setOutputFileName(outPath);
    printer.setPageSize(QPageSize(QPageSize::A4));
    printer.setPageOrientation(landscape ? QPageLayout::Landscape : QPageLayout::Portrait);
    printer.setPageMargins(QMarginsF(8, 8, 8, 8), QPageLayout::Millimeter);

    QTextDocument doc;
    doc.setDocumentMargin(0);
    doc.setPageSize(printer.pageLayout().paintRectPoints().size());
    doc.setHtml(htmlContent);
    doc.print(&printer);

    if (QFile::exists(outPath)) {
        m_lastExportedFile = outPath;
        m_isSuccess = true;
        m_statusMessage = "PDF generated: " + outPath;
        emit lastExportedFileChanged();
        emit statusMessageChanged();
        emit printCompleted(true, m_statusMessage);
        open_file_in_os(outPath);
        return outPath;
    } else {
        m_isSuccess = false;
        m_statusMessage = "Failed to create PDF at: " + outPath;
        emit statusMessageChanged();
        emit printCompleted(false, m_statusMessage);
        return "";
    }
}

QString PrintExportController::exportHtmlToOdf(const QString& htmlContent, const QString& defaultFileName, const QString& customPath, bool landscape) {
    Q_UNUSED(landscape);
    QString outPath = customPath.trimmed();
    if (outPath.isEmpty()) {
        QString dir = get_default_reports_dir();
        outPath = dir + "/" + defaultFileName;
        if (!outPath.endsWith(".odt", Qt::CaseInsensitive) && !outPath.endsWith(".odf", Qt::CaseInsensitive)) {
            outPath += ".odt";
        }
    }

    QTextDocument doc;
    doc.setHtml(htmlContent);
    QTextDocumentWriter writer(outPath);
    writer.setFormat("ODF");
    bool ok = writer.write(&doc);

    if (ok && QFile::exists(outPath)) {
        m_lastExportedFile = outPath;
        m_isSuccess = true;
        m_statusMessage = "ODF document generated: " + outPath;
        emit lastExportedFileChanged();
        emit statusMessageChanged();
        emit printCompleted(true, m_statusMessage);
        open_file_in_os(outPath);
        return outPath;
    } else {
        m_isSuccess = false;
        m_statusMessage = "Failed to export ODF to: " + outPath;
        emit statusMessageChanged();
        emit printCompleted(false, m_statusMessage);
        return "";
    }
}

QString PrintExportController::exportHtmlToExcel(const QString& htmlContent, const QString& defaultFileName, const QString& customPath) {
    QString outPath = customPath.trimmed();
    if (outPath.isEmpty()) {
        QString dir = get_default_reports_dir();
        outPath = dir + "/" + defaultFileName;
        if (!outPath.endsWith(".xls", Qt::CaseInsensitive)) {
            outPath += ".xls";
        }
    }

    QFile file(outPath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        m_isSuccess = false;
        m_statusMessage = "Failed to create Excel file at: " + outPath;
        emit statusMessageChanged();
        return "";
    }

    QString excelHtml = 
        "<html xmlns:o=\"urn:schemas-microsoft-com:office:office\" "
        "xmlns:x=\"urn:schemas-microsoft-com:office:excel\" "
        "xmlns=\"http://www.w3.org/TR/REC-html40\">\n"
        "<head>\n"
        "<meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\">\n"
        "<!--[if gte mso 9]><xml>\n"
        "<x:ExcelWorkbook>\n"
        "  <x:ExcelWorksheets>\n"
        "    <x:ExcelWorksheet>\n"
        "      <x:Name>Sheet1</x:Name>\n"
        "      <x:WorksheetOptions>\n"
        "        <x:DisplayGridlines/>\n"
        "      </x:WorksheetOptions>\n"
        "    </x:ExcelWorksheet>\n"
        "  </x:ExcelWorksheets>\n"
        "</x:ExcelWorkbook>\n"
        "</xml><![endif]-->\n"
        "<style>\n"
        "body { font-family: Calibri, Arial, sans-serif; font-size: 10pt; }\n"
        "table { border-collapse: collapse; width: 100%; }\n"
        "th { background-color: #0f172A; color: #ffffff; font-weight: bold; border: 0.5pt solid #000000; padding: 5px; text-align: center; }\n"
        "td { border: 0.5pt solid #000000; padding: 4px; }\n"
        ".num { mso-number-format: '\\#\\,\\#\\#0\\.00'; text-align: right; }\n"
        ".date { mso-number-format: 'dd\\-mm\\-yyyy'; text-align: center; }\n"
        ".bold { font-weight: bold; }\n"
        "</style>\n"
        "</head>\n"
        "<body>\n" + htmlContent + "\n</body>\n</html>";

    QTextStream out(&file);
    out << excelHtml;
    file.close();

    m_lastExportedFile = outPath;
    m_isSuccess = true;
    m_statusMessage = "Excel workbook generated: " + outPath;
    emit lastExportedFileChanged();
    emit statusMessageChanged();
    emit printCompleted(true, m_statusMessage);
    open_file_in_os(outPath);
    return outPath;
}

QString PrintExportController::exportTableToCsv(const QStringList& headers, const QVector<QStringList>& rows, const QString& defaultFileName, const QString& customPath) {
    QString outPath = customPath.trimmed();
    if (outPath.isEmpty()) {
        QString dir = get_default_reports_dir();
        outPath = dir + "/" + defaultFileName;
        if (!outPath.endsWith(".csv", Qt::CaseInsensitive)) {
            outPath += ".csv";
        }
    }

    QFile file(outPath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        m_isSuccess = false;
        m_statusMessage = "Cannot open CSV file for writing: " + outPath;
        emit statusMessageChanged();
        return "";
    }

    QTextStream out(&file);
    auto quoteCsv = [](const QString& s) -> QString {
        QString clean = s;
        clean.replace("\"", "\"\"");
        return "\"" + clean + "\"";
    };

    QStringList quotedHeaders;
    for (const auto& h : headers) quotedHeaders << quoteCsv(h);
    out << quotedHeaders.join(",") << "\n";

    for (const auto& row : rows) {
        QStringList quotedRow;
        for (const auto& col : row) quotedRow << quoteCsv(col);
        out << quotedRow.join(",") << "\n";
    }
    file.close();

    m_lastExportedFile = outPath;
    m_isSuccess = true;
    m_statusMessage = "CSV exported: " + outPath;
    emit lastExportedFileChanged();
    emit statusMessageChanged();
    emit printCompleted(true, m_statusMessage);
    open_file_in_os(outPath);
    return outPath;
}

// ==========================================
// 2. VOUCHER RENDERERS & EXPORTERS
// ==========================================

// 2.1 Sales Invoice
QString PrintExportController::renderSalesInvoiceSingleHtml(const QString& invoiceNo, const QString& copySubtitle) {
    return PrintReports::renderSalesInvoiceSingleHtml(invoiceNo, copySubtitle);
}
QString PrintExportController::renderSalesInvoiceHtml(const QString& invoiceNo, const QString& copyType) {
    return PrintReports::renderSalesInvoiceHtml(invoiceNo, copyType);
}
QString PrintExportController::export_sales_invoice_pdf(const QString& invoiceNo, const QString& customPath, const QString& copyType) {
    QString html = PrintReports::renderSalesInvoiceHtml(invoiceNo, copyType);
    QString cleanInv = invoiceNo.trimmed().replace("/", "_").replace("\\", "_");
    QString fileName = cleanInv + ".pdf";
    return exportHtmlToPdf(html, fileName, customPath, false);
}

QString PrintExportController::export_sales_invoice_duplicate_pdf(const QString& invoiceNo, const QString& customPath) {
    return export_sales_invoice_pdf(invoiceNo, customPath, "DUPLICATE FOR TRANSPORTER");
}

QString PrintExportController::export_sales_invoice_all_copies_pdf(const QString& invoiceNo, const QString& customPath) {
    return export_sales_invoice_pdf(invoiceNo, customPath, "ALL");
}

QString PrintExportController::export_sales_invoice_odf(const QString& invoiceNo, const QString& customPath) {
    QString html = PrintReports::renderSalesInvoiceHtml(invoiceNo, "ORIGINAL FOR RECIPIENT");
    QString cleanInv = invoiceNo.trimmed().replace("/", "_").replace("\\", "_");
    QString fileName = "Invoice_" + cleanInv + ".odt";
    return exportHtmlToOdf(html, fileName, customPath, false);
}

QString PrintExportController::export_sales_invoice_excel(const QString& invoiceNo, const QString& customPath) {
    QString html = PrintReports::renderSalesInvoiceExcelHtml(invoiceNo, "ORIGINAL FOR RECIPIENT");
    QString cleanInv = invoiceNo.trimmed().replace("/", "_").replace("\\", "_");
    QString fileName = "Invoice_" + cleanInv + ".xls";
    return exportHtmlToExcel(html, fileName, customPath);
}

bool PrintExportController::print_sales_invoice(const QString& invoiceNo, const QString& copyType) {
    QString html = PrintReports::renderSalesInvoiceHtml(invoiceNo, copyType);
    return printHtml(html, "Invoice " + invoiceNo, false);
}

bool PrintExportController::print_sales_invoice_duplicate(const QString& invoiceNo) {
    return print_sales_invoice(invoiceNo, "DUPLICATE FOR TRANSPORTER");
}

bool PrintExportController::print_sales_invoice_all_copies(const QString& invoiceNo) {
    return print_sales_invoice(invoiceNo, "ALL");
}

// 2.2 Purchase Invoice
QString PrintExportController::renderPurchaseInvoiceHtml(const QString& invoiceNo) {
    return PrintReports::renderPurchaseInvoiceHtml(invoiceNo);
}
QString PrintExportController::export_purchase_invoice_pdf(const QString& invoiceNo, const QString& customPath) {
    QString html = PrintReports::renderPurchaseInvoiceHtml(invoiceNo);
    QString cleanInv = invoiceNo.trimmed().replace("/", "_").replace("\\", "_");
    QString fileName = cleanInv + ".pdf";
    return exportHtmlToPdf(html, fileName, customPath, false);
}

QString PrintExportController::export_purchase_invoice_odf(const QString& invoiceNo, const QString& customPath) {
    QString html = PrintReports::renderPurchaseInvoiceHtml(invoiceNo);
    QString cleanInv = invoiceNo.trimmed().replace("/", "_").replace("\\", "_");
    QString fileName = "Purchase_" + cleanInv + ".odt";
    return exportHtmlToOdf(html, fileName, customPath, false);
}

QString PrintExportController::export_purchase_invoice_excel(const QString& invoiceNo, const QString& customPath) {
    QString html = PrintReports::renderPurchaseInvoiceExcelHtml(invoiceNo);
    QString cleanInv = invoiceNo.trimmed().replace("/", "_").replace("\\", "_");
    QString fileName = "Purchase_" + cleanInv + ".xls";
    return exportHtmlToExcel(html, fileName, customPath);
}

bool PrintExportController::print_purchase_invoice(const QString& invoiceNo) {
    QString html = PrintReports::renderPurchaseInvoiceHtml(invoiceNo);
    return printHtml(html, "Purchase " + invoiceNo, false);
}

// 2.3 Cash Voucher (Payment / Receipt)
QString PrintExportController::renderCashVoucherHtml(const QString& voucherNo, const QString& voucherType) {
    return PrintReports::renderCashVoucherHtml(voucherNo, voucherType);
}
QString PrintExportController::export_cash_voucher_pdf(const QString& voucherNo, const QString& customPath) {
    QString html = PrintReports::renderCashVoucherHtml(voucherNo);
    QString clean = voucherNo.trimmed().replace("/", "_").replace("\\", "_");
    return exportHtmlToPdf(html, "Cash_" + clean + ".pdf", customPath, false);
}

QString PrintExportController::export_cash_voucher_odf(const QString& voucherNo, const QString& customPath) {
    QString html = PrintReports::renderCashVoucherHtml(voucherNo);
    QString clean = voucherNo.trimmed().replace("/", "_").replace("\\", "_");
    return exportHtmlToOdf(html, "Cash_" + clean + ".odt", customPath, false);
}

QString PrintExportController::export_cash_voucher_excel(const QString& voucherNo, const QString& customPath) {
    QString html = PrintReports::renderCashVoucherExcelHtml(voucherNo);
    QString clean = voucherNo.trimmed().replace("/", "_").replace("\\", "_");
    return exportHtmlToExcel(html, "Cash_" + clean + ".xls", customPath);
}

bool PrintExportController::print_cash_voucher(const QString& voucherNo) {
    QString html = PrintReports::renderCashVoucherHtml(voucherNo);
    return printHtml(html, "Cash Voucher " + voucherNo, false);
}

// 2.4 Cheque / Bank Voucher
QString PrintExportController::renderChequeVoucherHtml(const QString& voucherNo, const QString& voucherType) {
    return PrintReports::renderChequeVoucherHtml(voucherNo, voucherType);
}
QString PrintExportController::export_cheque_voucher_pdf(const QString& voucherNo, const QString& customPath) {
    QString html = PrintReports::renderChequeVoucherHtml(voucherNo);
    QString clean = voucherNo.trimmed().replace("/", "_").replace("\\", "_");
    return exportHtmlToPdf(html, "Cheque_" + clean + ".pdf", customPath, false);
}

QString PrintExportController::export_cheque_voucher_odf(const QString& voucherNo, const QString& customPath) {
    QString html = PrintReports::renderChequeVoucherHtml(voucherNo);
    QString clean = voucherNo.trimmed().replace("/", "_").replace("\\", "_");
    return exportHtmlToOdf(html, "Cheque_" + clean + ".odt", customPath, false);
}

QString PrintExportController::export_cheque_voucher_excel(const QString& voucherNo, const QString& customPath) {
    QString html = PrintReports::renderChequeVoucherExcelHtml(voucherNo);
    QString clean = voucherNo.trimmed().replace("/", "_").replace("\\", "_");
    return exportHtmlToExcel(html, "Cheque_" + clean + ".xls", customPath);
}

bool PrintExportController::print_cheque_voucher(const QString& voucherNo) {
    QString html = PrintReports::renderChequeVoucherHtml(voucherNo);
    return printHtml(html, "Cheque Voucher " + voucherNo, false);
}

// 2.5 Journal Voucher
QString PrintExportController::renderJournalVoucherHtml(const QString& voucherNo) {
    return PrintReports::renderJournalVoucherHtml(voucherNo);
}
QString PrintExportController::export_journal_voucher_pdf(const QString& voucherNo, const QString& customPath) {
    QString html = PrintReports::renderJournalVoucherHtml(voucherNo);
    QString clean = voucherNo.trimmed().replace("/", "_").replace("\\", "_");
    return exportHtmlToPdf(html, "Journal_" + clean + ".pdf", customPath, false);
}

QString PrintExportController::export_journal_voucher_odf(const QString& voucherNo, const QString& customPath) {
    QString html = PrintReports::renderJournalVoucherHtml(voucherNo);
    QString clean = voucherNo.trimmed().replace("/", "_").replace("\\", "_");
    return exportHtmlToOdf(html, "Journal_" + clean + ".odt", customPath, false);
}

QString PrintExportController::export_journal_voucher_excel(const QString& voucherNo, const QString& customPath) {
    QString html = PrintReports::renderJournalVoucherExcelHtml(voucherNo);
    QString clean = voucherNo.trimmed().replace("/", "_").replace("\\", "_");
    return exportHtmlToExcel(html, "Journal_" + clean + ".xls", customPath);
}

bool PrintExportController::print_journal_voucher(const QString& voucherNo) {
    QString html = PrintReports::renderJournalVoucherHtml(voucherNo);
    return printHtml(html, "Journal Voucher " + voucherNo, false);
}

// 2.6 J-Form Voucher (Paddy Procurement)
QString PrintExportController::renderJFormVoucherHtml(const QString& voucherNo) {
    return PrintReports::renderJFormVoucherHtml(voucherNo);
}
QString PrintExportController::export_jform_voucher_pdf(const QString& voucherNo, const QString& customPath) {
    QString html = PrintReports::renderJFormVoucherHtml(voucherNo);
    QString clean = voucherNo.trimmed().replace("/", "_").replace("\\", "_");
    return exportHtmlToPdf(html, "JForm_" + clean + ".pdf", customPath, false);
}

QString PrintExportController::export_jform_voucher_odf(const QString& voucherNo, const QString& customPath) {
    QString html = PrintReports::renderJFormVoucherHtml(voucherNo);
    QString clean = voucherNo.trimmed().replace("/", "_").replace("\\", "_");
    return exportHtmlToOdf(html, "JForm_" + clean + ".odt", customPath, false);
}

QString PrintExportController::export_jform_voucher_excel(const QString& voucherNo, const QString& customPath) {
    QString html = PrintReports::renderJFormVoucherExcelHtml(voucherNo);
    QString clean = voucherNo.trimmed().replace("/", "_").replace("\\", "_");
    return exportHtmlToExcel(html, "JForm_" + clean + ".xls", customPath);
}

bool PrintExportController::print_jform_voucher(const QString& voucherNo) {
    QString html = PrintReports::renderJFormVoucherHtml(voucherNo);
    return printHtml(html, "J-Form " + voucherNo, false);
}

// 2.7 I-Form Voucher (Commission Sale)
QString PrintExportController::renderIFormVoucherHtml(const QString& voucherNo) {
    return PrintReports::renderIFormVoucherHtml(voucherNo);
}
QString PrintExportController::export_iform_voucher_pdf(const QString& voucherNo, const QString& customPath) {
    QString html = PrintReports::renderIFormVoucherHtml(voucherNo);
    QString clean = voucherNo.trimmed().replace("/", "_").replace("\\", "_");
    return exportHtmlToPdf(html, "IForm_" + clean + ".pdf", customPath, false);
}

QString PrintExportController::export_iform_voucher_odf(const QString& voucherNo, const QString& customPath) {
    QString html = PrintReports::renderIFormVoucherHtml(voucherNo);
    QString clean = voucherNo.trimmed().replace("/", "_").replace("\\", "_");
    return exportHtmlToOdf(html, "IForm_" + clean + ".odt", customPath, false);
}

QString PrintExportController::export_iform_voucher_excel(const QString& voucherNo, const QString& customPath) {
    QString html = PrintReports::renderIFormVoucherExcelHtml(voucherNo);
    QString clean = voucherNo.trimmed().replace("/", "_").replace("\\", "_");
    return exportHtmlToExcel(html, "IForm_" + clean + ".xls", customPath);
}

bool PrintExportController::print_iform_voucher(const QString& voucherNo) {
    QString html = PrintReports::renderIFormVoucherHtml(voucherNo);
    return printHtml(html, "I-Form " + voucherNo, false);
}

// 2.8 TDS / TCS / Tax Challan
QString PrintExportController::renderTdsVoucherHtml(const QString& voucherNo) {
    return PrintReports::renderTdsVoucherHtml(voucherNo);
}
QString PrintExportController::export_tds_voucher_pdf(const QString& voucherNo, const QString& customPath) {
    QString html = PrintReports::renderTdsVoucherHtml(voucherNo);
    QString clean = voucherNo.trimmed().replace("/", "_").replace("\\", "_");
    return exportHtmlToPdf(html, "TDS_" + clean + ".pdf", customPath, false);
}

QString PrintExportController::export_tds_voucher_odf(const QString& voucherNo, const QString& customPath) {
    QString html = PrintReports::renderTdsVoucherHtml(voucherNo);
    QString clean = voucherNo.trimmed().replace("/", "_").replace("\\", "_");
    return exportHtmlToOdf(html, "TDS_" + clean + ".odt", customPath, false);
}

QString PrintExportController::export_tds_voucher_excel(const QString& voucherNo, const QString& customPath) {
    QString html = PrintReports::renderTdsVoucherExcelHtml(voucherNo);
    QString clean = voucherNo.trimmed().replace("/", "_").replace("\\", "_");
    return exportHtmlToExcel(html, "TDS_" + clean + ".xls", customPath);
}

bool PrintExportController::print_tds_voucher(const QString& voucherNo) {
    QString html = PrintReports::renderTdsVoucherHtml(voucherNo);
    return printHtml(html, "TDS " + voucherNo, false);
}

// 2.9 Debit / Credit Note
QString PrintExportController::renderDebitCreditNoteHtml(const QString& noteNo) {
    return PrintReports::renderDebitCreditNoteHtml(noteNo);
}
QString PrintExportController::export_debit_credit_note_pdf(const QString& noteNo, const QString& customPath) {
    QString html = PrintReports::renderDebitCreditNoteHtml(noteNo);
    QString clean = noteNo.trimmed().replace("/", "_").replace("\\", "_");
    return exportHtmlToPdf(html, "Note_" + clean + ".pdf", customPath, false);
}

QString PrintExportController::export_debit_credit_note_odf(const QString& noteNo, const QString& customPath) {
    QString html = PrintReports::renderDebitCreditNoteHtml(noteNo);
    QString clean = noteNo.trimmed().replace("/", "_").replace("\\", "_");
    return exportHtmlToOdf(html, "Note_" + clean + ".odt", customPath, false);
}

QString PrintExportController::export_debit_credit_note_excel(const QString& noteNo, const QString& customPath) {
    QString html = PrintReports::renderDebitCreditNoteExcelHtml(noteNo);
    QString clean = noteNo.trimmed().replace("/", "_").replace("\\", "_");
    return exportHtmlToExcel(html, "Note_" + clean + ".xls", customPath);
}

bool PrintExportController::print_debit_credit_note(const QString& noteNo) {
    QString html = PrintReports::renderDebitCreditNoteHtml(noteNo);
    return printHtml(html, "Note " + noteNo, false);
}

// 2.10 Transport Dispatch / Gate Pass
QString PrintExportController::renderTransportDispatchHtml(int dispatchId) {
    return PrintReports::renderTransportDispatchHtml(dispatchId);
}
QString PrintExportController::export_transport_dispatch_pdf(int dispatchId, const QString& customPath) {
    QString html = PrintReports::renderTransportDispatchHtml(dispatchId);
    return exportHtmlToPdf(html, QString("GatePass_%1.pdf").arg(dispatchId), customPath, false);
}

QString PrintExportController::export_transport_dispatch_odf(int dispatchId, const QString& customPath) {
    QString html = PrintReports::renderTransportDispatchHtml(dispatchId);
    return exportHtmlToOdf(html, QString("GatePass_%1.odt").arg(dispatchId), customPath, false);
}

QString PrintExportController::export_transport_dispatch_excel(int dispatchId, const QString& customPath) {
    QString html = PrintReports::renderTransportDispatchExcelHtml(dispatchId);
    return exportHtmlToExcel(html, QString("GatePass_%1.xls").arg(dispatchId), customPath);
}

bool PrintExportController::print_transport_dispatch(int dispatchId) {
    QString html = PrintReports::renderTransportDispatchHtml(dispatchId);
    return printHtml(html, QString("Gate Pass %1").arg(dispatchId), false);
}

// ==========================================
// 3. STATEMENTS & FINANCIAL REPORTS RENDERERS
// ==========================================

// 3.1 Ledger Statement (T-Account)
QString PrintExportController::renderLedgerStatementHtml(const QString& partyName, const QString& fromDate, const QString& toDate) {
    return PrintReports::renderLedgerStatementHtml(partyName, fromDate, toDate);
}
QString PrintExportController::export_ledger_statement_pdf(const QString& partyName, const QString& fromDate, const QString& toDate, const QString& customPath) {
    QString html = PrintReports::renderLedgerStatementHtml(partyName, fromDate, toDate);
    QString pClean = partyName.trimmed().replace(" ", "_").replace("/", "_").replace("\\", "_");
    QString fileName = "Ledger_" + pClean + ".pdf";
    return exportHtmlToPdf(html, fileName, customPath, false);
}

QString PrintExportController::export_ledger_statement_odf(const QString& partyName, const QString& fromDate, const QString& toDate, const QString& customPath) {
    QString html = PrintReports::renderLedgerStatementHtml(partyName, fromDate, toDate);
    QString pClean = partyName.trimmed().replace(" ", "_").replace("/", "_").replace("\\", "_");
    QString fileName = "Ledger_" + pClean + ".odt";
    return exportHtmlToOdf(html, fileName, customPath, true);
}

QString PrintExportController::export_ledger_statement_excel(const QString& partyName, const QString& fromDate, const QString& toDate, const QString& customPath) {
    QString html = PrintReports::renderLedgerStatementExcelHtml(partyName, fromDate, toDate);
    QString pClean = partyName.trimmed().replace(" ", "_").replace("/", "_").replace("\\", "_");
    QString fileName = "Ledger_" + pClean + ".xls";
    return exportHtmlToExcel(html, fileName, customPath);
}

QString PrintExportController::export_ledger_csv(const QString& partyName, const QString& fromDate, const QString& toDate, const QString& customPath) {
    QStringList headers;
    QVector<QStringList> rows;
    QString defName;
    PrintReports::buildLedgerCsvDocument(partyName, fromDate, toDate, &headers, &rows, &defName);
    return exportTableToCsv(headers, rows, defName, customPath);
}
bool PrintExportController::print_ledger_statement(const QString& partyName, const QString& fromDate, const QString& toDate) {
    QString html = PrintReports::renderLedgerStatementHtml(partyName, fromDate, toDate);
    return printHtml(html, "Ledger " + partyName, false);
}

// 3.2 Day Book
QString PrintExportController::renderDayBookHtml(const QString& fromDate, const QString& toDate) {
    return PrintReports::renderDayBookHtml(fromDate, toDate);
}
QString PrintExportController::export_day_book_pdf(const QString& fromDate, const QString& toDate, const QString& customPath) {
    QString html = PrintReports::renderDayBookHtml(fromDate, toDate);
    return exportHtmlToPdf(html, "DayBook.pdf", customPath, false);
}

QString PrintExportController::export_day_book_odf(const QString& fromDate, const QString& toDate, const QString& customPath) {
    QString html = PrintReports::renderDayBookHtml(fromDate, toDate);
    return exportHtmlToOdf(html, "DayBook.odt", customPath, true);
}

QString PrintExportController::export_day_book_excel(const QString& fromDate, const QString& toDate, const QString& customPath) {
    QString html = PrintReports::renderDayBookExcelHtml(fromDate, toDate);
    return exportHtmlToExcel(html, "DayBook.xls", customPath);
}

bool PrintExportController::print_day_book(const QString& fromDate, const QString& toDate) {
    QString html = PrintReports::renderDayBookHtml(fromDate, toDate);
    return printHtml(html, "Day Book", false);
}

// 3.3 Trial Balance
QString PrintExportController::renderTrialBalanceHtml(const QString& asOnDate, int mode) {
    return PrintReports::renderTrialBalanceHtml(asOnDate, mode);
}
QString PrintExportController::export_trial_balance_pdf(const QString& asOnDate, int mode, const QString& customPath) {
    QString html = PrintReports::renderTrialBalanceHtml(asOnDate, mode);
    return exportHtmlToPdf(html, "TrialBalance.pdf", customPath, false);
}

QString PrintExportController::export_trial_balance_odf(const QString& asOnDate, int mode, const QString& customPath) {
    QString html = PrintReports::renderTrialBalanceHtml(asOnDate, mode);
    return exportHtmlToOdf(html, "TrialBalance.odt", customPath, false);
}

QString PrintExportController::export_trial_balance_excel(const QString& asOnDate, int mode, const QString& customPath) {
    QString html = PrintReports::renderTrialBalanceExcelHtml(asOnDate, mode);
    return exportHtmlToExcel(html, "TrialBalance.xls", customPath);
}

bool PrintExportController::print_trial_balance(const QString& asOnDate, int mode) {
    QString html = PrintReports::renderTrialBalanceHtml(asOnDate, mode);
    return printHtml(html, "Trial Balance", false);
}

// 3.4 Profit & Loss Statement
QString PrintExportController::renderProfitLossHtml(const QString& fromDate, const QString& toDate) {
    return PrintReports::renderProfitLossHtml(fromDate, toDate);
}
QString PrintExportController::export_profit_loss_pdf(const QString& fromDate, const QString& toDate, const QString& customPath) {
    QString html = PrintReports::renderProfitLossHtml(fromDate, toDate);
    return exportHtmlToPdf(html, "ProfitLoss.pdf", customPath, false);
}

QString PrintExportController::export_profit_loss_odf(const QString& fromDate, const QString& toDate, const QString& customPath) {
    QString html = PrintReports::renderProfitLossHtml(fromDate, toDate);
    return exportHtmlToOdf(html, "ProfitLoss.odt", customPath, true);
}

QString PrintExportController::export_profit_loss_excel(const QString& fromDate, const QString& toDate, const QString& customPath) {
    QString html = PrintReports::renderProfitLossExcelHtml(fromDate, toDate);
    return exportHtmlToExcel(html, "ProfitLoss.xls", customPath);
}

bool PrintExportController::print_profit_loss(const QString& fromDate, const QString& toDate) {
    QString html = PrintReports::renderProfitLossHtml(fromDate, toDate);
    return printHtml(html, "Profit & Loss", false);
}

// 3.5 Balance Sheet
QString PrintExportController::renderBalanceSheetHtml(const QString& asOnDate) {
    return PrintReports::renderBalanceSheetHtml(asOnDate);
}
QString PrintExportController::export_balance_sheet_pdf(const QString& asOnDate, const QString& customPath) {
    QString html = PrintReports::renderBalanceSheetHtml(asOnDate);
    return exportHtmlToPdf(html, "BalanceSheet.pdf", customPath, false);
}

QString PrintExportController::export_balance_sheet_odf(const QString& asOnDate, const QString& customPath) {
    QString html = PrintReports::renderBalanceSheetHtml(asOnDate);
    return exportHtmlToOdf(html, "BalanceSheet.odt", customPath, true);
}

QString PrintExportController::export_balance_sheet_excel(const QString& asOnDate, const QString& customPath) {
    QString html = PrintReports::renderBalanceSheetExcelHtml(asOnDate);
    return exportHtmlToExcel(html, "BalanceSheet.xls", customPath);
}

bool PrintExportController::print_balance_sheet(const QString& asOnDate) {
    QString html = PrintReports::renderBalanceSheetHtml(asOnDate);
    return printHtml(html, "Balance Sheet", false);
}

// 3.6 Capital Accounts Schedule
QString PrintExportController::renderCapitalAccountsHtml(const QString& fromDate, const QString& toDate) {
    return PrintReports::renderCapitalAccountsHtml(fromDate, toDate);
}
QString PrintExportController::export_capital_accounts_pdf(const QString& fromDate, const QString& toDate, const QString& customPath) {
    QString html = PrintReports::renderCapitalAccountsHtml(fromDate, toDate);
    return exportHtmlToPdf(html, "CapitalAccounts.pdf", customPath, false);
}

QString PrintExportController::export_capital_accounts_odf(const QString& fromDate, const QString& toDate, const QString& customPath) {
    QString html = PrintReports::renderCapitalAccountsHtml(fromDate, toDate);
    return exportHtmlToOdf(html, "CapitalAccounts.odt", customPath, false);
}

QString PrintExportController::export_capital_accounts_excel(const QString& fromDate, const QString& toDate, const QString& customPath) {
    QString html = PrintReports::renderCapitalAccountsExcelHtml(fromDate, toDate);
    return exportHtmlToExcel(html, "CapitalAccounts.xls", customPath);
}

bool PrintExportController::print_capital_accounts(const QString& fromDate, const QString& toDate) {
    QString html = PrintReports::renderCapitalAccountsHtml(fromDate, toDate);
    return printHtml(html, "Capital Accounts", false);
}

// 3.7 Cash & Bank Flow Statement
QString PrintExportController::renderCashBankFlowHtml(int flowType, const QString& fromDate, const QString& toDate) {
    return PrintReports::renderCashBankFlowHtml(flowType, fromDate, toDate);
}
QString PrintExportController::export_cash_bank_flow_pdf(int flowType, const QString& fromDate, const QString& toDate, const QString& customPath) {
    QString html = PrintReports::renderCashBankFlowHtml(flowType, fromDate, toDate);
    return exportHtmlToPdf(html, "CashBankFlow.pdf", customPath, false);
}

QString PrintExportController::export_cash_bank_flow_odf(int flowType, const QString& fromDate, const QString& toDate, const QString& customPath) {
    QString html = PrintReports::renderCashBankFlowHtml(flowType, fromDate, toDate);
    return exportHtmlToOdf(html, "CashBankFlow.odt", customPath, false);
}

QString PrintExportController::export_cash_bank_flow_excel(int flowType, const QString& fromDate, const QString& toDate, const QString& customPath) {
    QString html = PrintReports::renderCashBankFlowExcelHtml(flowType, fromDate, toDate);
    return exportHtmlToExcel(html, "CashBankFlow.xls", customPath);
}

bool PrintExportController::print_cash_bank_flow(int flowType, const QString& fromDate, const QString& toDate) {
    QString html = PrintReports::renderCashBankFlowHtml(flowType, fromDate, toDate);
    return printHtml(html, "Cash Bank Flow", false);
}

// 3.8 Interest Calculator / Aank Statement
QString PrintExportController::renderInterestCalculationHtml(const QString& partyName, const QString& fromDate, const QString& toDate, double annualRate) {
    return PrintReports::renderInterestCalculationHtml(partyName, fromDate, toDate, annualRate);
}
QString PrintExportController::export_interest_statement_pdf(const QString& partyName, const QString& fromDate, const QString& toDate, double annualRate, const QString& customPath) {
    QString html = PrintReports::renderInterestCalculationHtml(partyName, fromDate, toDate, annualRate);
    QString pClean = partyName.trimmed().replace(" ", "_").replace("/", "_");
    return exportHtmlToPdf(html, "Interest_" + pClean + ".pdf", customPath, false);
}

QString PrintExportController::export_interest_statement_odf(const QString& partyName, const QString& fromDate, const QString& toDate, double annualRate, const QString& customPath) {
    QString html = PrintReports::renderInterestCalculationHtml(partyName, fromDate, toDate, annualRate);
    QString pClean = partyName.trimmed().replace(" ", "_").replace("/", "_");
    return exportHtmlToOdf(html, "Interest_" + pClean + ".odt", customPath, false);
}

QString PrintExportController::export_interest_statement_excel(const QString& partyName, const QString& fromDate, const QString& toDate, double annualRate, const QString& customPath) {
    QString html = PrintReports::renderInterestCalculationExcelHtml(partyName, fromDate, toDate, annualRate);
    QString pClean = partyName.trimmed().replace(" ", "_").replace("/", "_");
    return exportHtmlToExcel(html, "Interest_" + pClean + ".xls", customPath);
}

bool PrintExportController::print_interest_statement(const QString& partyName, const QString& fromDate, const QString& toDate, double annualRate) {
    QString html = PrintReports::renderInterestCalculationHtml(partyName, fromDate, toDate, annualRate);
    return printHtml(html, "Interest Statement " + partyName, false);
}

// 3.9 Depreciation Chart
QString PrintExportController::renderDepreciationChartHtml(const QString& asOnDate) {
    return PrintReports::renderDepreciationChartHtml(asOnDate);
}
QString PrintExportController::export_depreciation_chart_pdf(const QString& asOnDate, const QString& customPath) {
    QString html = PrintReports::renderDepreciationChartHtml(asOnDate);
    return exportHtmlToPdf(html, "DepreciationChart.pdf", customPath, false);
}

QString PrintExportController::export_depreciation_chart_odf(const QString& asOnDate, const QString& customPath) {
    QString html = PrintReports::renderDepreciationChartHtml(asOnDate);
    return exportHtmlToOdf(html, "DepreciationChart.odt", customPath, false);
}

QString PrintExportController::export_depreciation_chart_excel(const QString& asOnDate, const QString& customPath) {
    QString html = PrintReports::renderDepreciationChartExcelHtml(asOnDate);
    return exportHtmlToExcel(html, "DepreciationChart.xls", customPath);
}

bool PrintExportController::print_depreciation_chart(const QString& asOnDate) {
    QString html = PrintReports::renderDepreciationChartHtml(asOnDate);
    return printHtml(html, "Depreciation Chart", false);
}

// ==========================================
// 4. REGISTERS & INVENTORY / COMPLIANCE
// ==========================================

// 4.1 Sales Register
QString PrintExportController::renderSalesRegisterHtml(const QString& fromDate, const QString& toDate) {
    return PrintReports::renderSalesRegisterHtml(fromDate, toDate);
}
QString PrintExportController::export_sales_register_pdf(const QString& fromDate, const QString& toDate, const QString& customPath) {
    QString html = PrintReports::renderSalesRegisterHtml(fromDate, toDate);
    return exportHtmlToPdf(html, "SalesRegister.pdf", customPath, false);
}

QString PrintExportController::export_sales_register_odf(const QString& fromDate, const QString& toDate, const QString& customPath) {
    QString html = PrintReports::renderSalesRegisterHtml(fromDate, toDate);
    return exportHtmlToOdf(html, "SalesRegister.odt", customPath, true);
}

QString PrintExportController::export_sales_register_excel(const QString& fromDate, const QString& toDate, const QString& customPath) {
    QString html = PrintReports::renderSalesRegisterExcelHtml(fromDate, toDate);
    return exportHtmlToExcel(html, "SalesRegister.xls", customPath);
}

QString PrintExportController::export_sales_register_csv(const QString& fromDate, const QString& toDate, const QString& customPath) {
    QStringList headers;
    QVector<QStringList> rows;
    QString defName;
    PrintReports::buildSalesRegisterCsvDocument(fromDate, toDate, &headers, &rows, &defName);
    return exportTableToCsv(headers, rows, defName, customPath);
}
bool PrintExportController::print_sales_register(const QString& fromDate, const QString& toDate) {
    QString html = PrintReports::renderSalesRegisterHtml(fromDate, toDate);
    return printHtml(html, "Sales Register", false);
}

// 4.2 Purchase Register
QString PrintExportController::renderPurchaseRegisterHtml(const QString& fromDate, const QString& toDate) {
    return PrintReports::renderPurchaseRegisterHtml(fromDate, toDate);
}
QString PrintExportController::export_purchase_register_pdf(const QString& fromDate, const QString& toDate, const QString& customPath) {
    QString html = PrintReports::renderPurchaseRegisterHtml(fromDate, toDate);
    return exportHtmlToPdf(html, "PurchaseRegister.pdf", customPath, false);
}

QString PrintExportController::export_purchase_register_odf(const QString& fromDate, const QString& toDate, const QString& customPath) {
    QString html = PrintReports::renderPurchaseRegisterHtml(fromDate, toDate);
    return exportHtmlToOdf(html, "PurchaseRegister.odt", customPath, true);
}

QString PrintExportController::export_purchase_register_excel(const QString& fromDate, const QString& toDate, const QString& customPath) {
    QString html = PrintReports::renderPurchaseRegisterExcelHtml(fromDate, toDate);
    return exportHtmlToExcel(html, "PurchaseRegister.xls", customPath);
}

QString PrintExportController::export_purchase_register_csv(const QString& fromDate, const QString& toDate, const QString& customPath) {
    QStringList headers;
    QVector<QStringList> rows;
    QString defName;
    PrintReports::buildPurchaseRegisterCsvDocument(fromDate, toDate, &headers, &rows, &defName);
    return exportTableToCsv(headers, rows, defName, customPath);
}
bool PrintExportController::print_purchase_register(const QString& fromDate, const QString& toDate) {
    QString html = PrintReports::renderPurchaseRegisterHtml(fromDate, toDate);
    return printHtml(html, "Purchase Register", false);
}

// 4.3 Stock Register
QString PrintExportController::renderStockRegisterHtml(const QString& fromDate, const QString& toDate) {
    return PrintReports::renderStockRegisterHtml(fromDate, toDate);
}
QString PrintExportController::export_stock_register_pdf(const QString& fromDate, const QString& toDate, const QString& customPath) {
    QString html = PrintReports::renderStockRegisterHtml(fromDate, toDate);
    return exportHtmlToPdf(html, "StockRegister.pdf", customPath, false);
}

QString PrintExportController::export_stock_register_odf(const QString& fromDate, const QString& toDate, const QString& customPath) {
    QString html = PrintReports::renderStockRegisterHtml(fromDate, toDate);
    return exportHtmlToOdf(html, "StockRegister.odt", customPath, true);
}

QString PrintExportController::export_stock_register_excel(const QString& fromDate, const QString& toDate, const QString& customPath) {
    QString html = PrintReports::renderStockRegisterExcelHtml(fromDate, toDate);
    return exportHtmlToExcel(html, "StockRegister.xls", customPath);
}

QString PrintExportController::export_stock_csv(const QString& fromDate, const QString& toDate, const QString& customPath) {
    QStringList headers;
    QVector<QStringList> rows;
    QString defName;
    PrintReports::buildStockCsvDocument(fromDate, toDate, &headers, &rows, &defName);
    return exportTableToCsv(headers, rows, defName, customPath);
}
bool PrintExportController::print_stock_register(const QString& fromDate, const QString& toDate) {
    QString html = PrintReports::renderStockRegisterHtml(fromDate, toDate);
    return printHtml(html, "Stock Register", false);
}

// 4.4 Item Movement Analysis
QString PrintExportController::renderItemMovementHtml(int itemId, const QString& fromDate, const QString& toDate) {
    return PrintReports::renderItemMovementHtml(itemId, fromDate, toDate);
}
QString PrintExportController::export_item_movement_pdf(int itemId, const QString& fromDate, const QString& toDate, const QString& customPath) {
    QString html = PrintReports::renderItemMovementHtml(itemId, fromDate, toDate);
    return exportHtmlToPdf(html, QString("ItemMovement_%1.pdf").arg(itemId), customPath, false);
}

QString PrintExportController::export_item_movement_odf(int itemId, const QString& fromDate, const QString& toDate, const QString& customPath) {
    QString html = PrintReports::renderItemMovementHtml(itemId, fromDate, toDate);
    return exportHtmlToOdf(html, QString("ItemMovement_%1.odt").arg(itemId), customPath, false);
}

QString PrintExportController::export_item_movement_excel(int itemId, const QString& fromDate, const QString& toDate, const QString& customPath) {
    QString html = PrintReports::renderItemMovementExcelHtml(itemId, fromDate, toDate);
    return exportHtmlToExcel(html, QString("ItemMovement_%1.xls").arg(itemId), customPath);
}

bool PrintExportController::print_item_movement(int itemId, const QString& fromDate, const QString& toDate) {
    QString html = PrintReports::renderItemMovementHtml(itemId, fromDate, toDate);
    return printHtml(html, QString("Item Movement %1").arg(itemId), false);
}

// 4.5 Physical / Closing Stock Valuation
QString PrintExportController::renderClosingStockHtml(const QString& asOnDate) {
    return PrintReports::renderClosingStockHtml(asOnDate);
}
QString PrintExportController::export_closing_stock_pdf(const QString& asOnDate, const QString& customPath) {
    QString html = PrintReports::renderClosingStockHtml(asOnDate);
    return exportHtmlToPdf(html, "ClosingStockValuation.pdf", customPath, false);
}

QString PrintExportController::export_closing_stock_odf(const QString& asOnDate, const QString& customPath) {
    QString html = PrintReports::renderClosingStockHtml(asOnDate);
    return exportHtmlToOdf(html, "ClosingStockValuation.odt", customPath, true);
}

QString PrintExportController::export_closing_stock_excel(const QString& asOnDate, const QString& customPath) {
    QString html = PrintReports::renderClosingStockExcelHtml(asOnDate);
    return exportHtmlToExcel(html, "ClosingStockValuation.xls", customPath);
}

bool PrintExportController::print_closing_stock(const QString& asOnDate) {
    QString html = PrintReports::renderClosingStockHtml(asOnDate);
    return printHtml(html, "Closing Stock Valuation", false);
}

// 4.6 Milling Production Statement
QString PrintExportController::renderMillingStatementHtml(const QString& fromDate, const QString& toDate) {
    return PrintReports::renderMillingStatementHtml(fromDate, toDate);
}
QString PrintExportController::export_milling_statement_pdf(const QString& fromDate, const QString& toDate, const QString& customPath) {
    QString html = PrintReports::renderMillingStatementHtml(fromDate, toDate);
    return exportHtmlToPdf(html, "MillingStatement.pdf", customPath, false);
}

QString PrintExportController::export_milling_statement_odf(const QString& fromDate, const QString& toDate, const QString& customPath) {
    QString html = PrintReports::renderMillingStatementHtml(fromDate, toDate);
    return exportHtmlToOdf(html, "MillingStatement.odt", customPath, true);
}

QString PrintExportController::export_milling_statement_excel(const QString& fromDate, const QString& toDate, const QString& customPath) {
    QString html = PrintReports::renderMillingStatementExcelHtml(fromDate, toDate);
    return exportHtmlToExcel(html, "MillingStatement.xls", customPath);
}

bool PrintExportController::print_milling_statement(const QString& fromDate, const QString& toDate) {
    QString html = PrintReports::renderMillingStatementHtml(fromDate, toDate);
    return printHtml(html, "Milling Statement", false);
}

// 4.7 Mandi Reports (Form M, Form 9, RDF, Market Fee)
QString PrintExportController::renderMandiReportHtml(const QString& reportType, const QString& fromDate, const QString& toDate) {
    return PrintReports::renderMandiReportHtml(reportType, fromDate, toDate);
}
QString PrintExportController::export_mandi_report_pdf(const QString& reportType, const QString& fromDate, const QString& toDate, const QString& customPath) {
    QString html = PrintReports::renderMandiReportHtml(reportType, fromDate, toDate);
    return exportHtmlToPdf(html, "MandiReport_" + reportType + ".pdf", customPath, false);
}

QString PrintExportController::export_mandi_report_odf(const QString& reportType, const QString& fromDate, const QString& toDate, const QString& customPath) {
    QString html = PrintReports::renderMandiReportHtml(reportType, fromDate, toDate);
    return exportHtmlToOdf(html, "MandiReport_" + reportType + ".odt", customPath, true);
}

QString PrintExportController::export_mandi_report_excel(const QString& reportType, const QString& fromDate, const QString& toDate, const QString& customPath) {
    QString html = PrintReports::renderMandiReportExcelHtml(reportType, fromDate, toDate);
    return exportHtmlToExcel(html, "MandiReport_" + reportType + ".xls", customPath);
}

bool PrintExportController::print_mandi_report(const QString& reportType, const QString& fromDate, const QString& toDate) {
    QString html = PrintReports::renderMandiReportHtml(reportType, fromDate, toDate);
    return printHtml(html, "Mandi Report " + reportType, false);
}

// 4.8 GST Compliance (GSTR-1, GSTR-2, GSTR-3B)
QString PrintExportController::renderGstrReportHtml(const QString& gstrType, const QString& fromDate, const QString& toDate) {
    return PrintReports::renderGstrReportHtml(gstrType, fromDate, toDate);
}
QString PrintExportController::export_gstr_report_pdf(const QString& gstrType, const QString& fromDate, const QString& toDate, const QString& customPath) {
    QString html = PrintReports::renderGstrReportHtml(gstrType, fromDate, toDate);
    return exportHtmlToPdf(html, "GSTR_" + gstrType + ".pdf", customPath, false);
}

QString PrintExportController::export_gstr_report_odf(const QString& gstrType, const QString& fromDate, const QString& toDate, const QString& customPath) {
    QString html = PrintReports::renderGstrReportHtml(gstrType, fromDate, toDate);
    return exportHtmlToOdf(html, "GSTR_" + gstrType + ".odt", customPath, true);
}

QString PrintExportController::export_gstr_report_excel(const QString& gstrType, const QString& fromDate, const QString& toDate, const QString& customPath) {
    QString html = PrintReports::renderGstrReportExcelHtml(gstrType, fromDate, toDate);
    return exportHtmlToExcel(html, "GSTR_" + gstrType + ".xls", customPath);
}

bool PrintExportController::print_gstr_report(const QString& gstrType, const QString& fromDate, const QString& toDate) {
    QString html = PrintReports::renderGstrReportHtml(gstrType, fromDate, toDate);
    return printHtml(html, "GSTR " + gstrType, false);
}
