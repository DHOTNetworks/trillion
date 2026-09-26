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
#include <QRegularExpression>
#include <QDebug>
#include <cmath>
#include <algorithm>

static QString formatINR(double val) {
    if (std::abs(val) < 0.001) return "₹0.00";
    bool neg = val < 0;
    double absVal = std::abs(val);
    long long integerPart = static_cast<long long>(absVal);
    int decimalPart = static_cast<int>(std::round((absVal - integerPart) * 100.0));
    if (decimalPart >= 100) {
        integerPart += 1;
        decimalPart = 0;
    }

    QString s = QString::number(integerPart);
    QString res = "";
    int n = s.length();
    if (n <= 3) {
        res = s;
    } else {
        res = s.right(3);
        int rem = n - 3;
        while (rem > 0) {
            int take = std::min(2, rem);
            res = s.mid(rem - take, take) + "," + res;
            rem -= take;
        }
    }
    QString decStr = QString::number(decimalPart);
    if (decStr.length() == 1) decStr = "0" + decStr;
    return (neg ? "-₹" : "₹") + res + "." + decStr;
}

static QString formatPlainINR(double val) {
    if (std::abs(val) < 0.001) return "0.00";
    bool neg = val < 0;
    double absVal = std::abs(val);
    long long integerPart = static_cast<long long>(absVal);
    int decimalPart = static_cast<int>(std::round((absVal - integerPart) * 100.0));
    if (decimalPart >= 100) {
        integerPart += 1;
        decimalPart = 0;
    }

    QString s = QString::number(integerPart);
    QString res = "";
    int n = s.length();
    if (n <= 3) {
        res = s;
    } else {
        res = s.right(3);
        int rem = n - 3;
        while (rem > 0) {
            int take = std::min(2, rem);
            res = s.mid(rem - take, take) + "," + res;
            rem -= take;
        }
    }
    QString decStr = QString::number(decimalPart);
    if (decStr.length() == 1) decStr = "0" + decStr;
    return (neg ? "-" : "") + res + "." + decStr;
}

static QString formatDisplayDate(const QString& iso) {
    if (iso.isEmpty()) return "";
    QStringList p = iso.split("-");
    if (p.size() == 3 && p[0].length() == 4) {
        return p[2] + "-" + p[1] + "-" + p[0];
    }
    return iso;
}

PrintExportController::PrintExportController(QObject* parent)
    : QObject(parent)
{
}

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

QVariantMap PrintExportController::getFirmProfile() {
    QVariantMap firm;
    QVariantList rows = DatabaseManager::instance().executeQuery("SELECT * FROM company_info LIMIT 1;");
    if (!rows.isEmpty()) {
        QVariantMap r = rows.first().toMap();
        firm["firm_name"] = r.value("company_name").toString().trimmed();
        firm["address"] = r.value("address").toString().trimmed();
        firm["city"] = r.value("city").toString().trimmed();
        firm["phone"] = r.value("phone").toString().trimmed();
        if (firm.value("phone").toString().isEmpty()) {
            firm["phone"] = r.value("mobile").toString().trimmed();
        }
        firm["email"] = r.value("email").toString().trimmed();
        firm["gstin"] = r.value("gstin").toString().trimmed();
        QString stateStr = r.value("state").toString().trimmed();
        QString stateCode = r.value("state_code").toString().trimmed();
        if (!stateCode.isEmpty() && !stateStr.contains(stateCode)) {
            stateStr += " (" + stateCode + ")";
        }
        firm["state"] = stateStr;
        firm["pan"] = r.value("pan_no").toString().trimmed();
        firm["bank_name"] = r.value("bank_name").toString().trimmed();
        QString bAc = r.value("bank_account").toString().trimmed();
        bAc.remove(QRegularExpression("^(A/c\\s*No[:\\.-]*|Account\\s*No[:\\.-]*)\\s*", QRegularExpression::CaseInsensitiveOption));
        firm["bank_ac"] = bAc.trimmed();

        QString bIfsc = r.value("ifsc_code").toString().trimmed();
        bIfsc.remove(QRegularExpression("^(IFSC[:\\.-]*|RTGS[:\\.-]*)\\s*", QRegularExpression::CaseInsensitiveOption));
        firm["bank_ifsc"] = bIfsc.trimmed();

        firm["fssai"] = r.value("fssai_no").toString().trimmed();
        firm["bank_branch"] = r.value("bank_branch", "Main Branch").toString().trimmed();
    }
    
    // Ensure standard keys exist even if company_info record is empty
    if (!firm.contains("firm_name")) firm["firm_name"] = "";
    if (!firm.contains("address")) firm["address"] = "";
    if (!firm.contains("city")) firm["city"] = "";
    if (!firm.contains("phone")) firm["phone"] = "";
    if (!firm.contains("email")) firm["email"] = "";
    if (!firm.contains("gstin")) firm["gstin"] = "";
    if (!firm.contains("state")) firm["state"] = "";
    if (!firm.contains("pan")) firm["pan"] = "";
    if (!firm.contains("bank_name")) firm["bank_name"] = "";
    if (!firm.contains("bank_ac")) firm["bank_ac"] = "";
    if (!firm.contains("bank_ifsc")) firm["bank_ifsc"] = "";
    if (!firm.contains("bank_branch")) firm["bank_branch"] = "";
    if (!firm.contains("fssai")) firm["fssai"] = "";

    return firm;
}

QVariantMap PrintExportController::getPartyProfile(int partyId, const QString& partyName) {
    QVariantMap profile;
    QVariantList rows;
    if (partyId > 0) {
        rows = DatabaseManager::instance().executeQuery("SELECT * FROM parties WHERE id = ? LIMIT 1;", {partyId});
    } else if (!partyName.isEmpty()) {
        rows = DatabaseManager::instance().executeQuery("SELECT * FROM parties WHERE name = ? COLLATE NOCASE LIMIT 1;", {partyName});
    }
    if (!rows.isEmpty()) {
        QVariantMap r = rows.first().toMap();
        profile["id"] = r.value("id");
        profile["name"] = r.value("name").toString().trimmed();
        profile["city"] = r.value("city").toString().trimmed();
        profile["address"] = r.value("address").toString().trimmed();
        profile["gstin"] = r.value("gstin").toString().trimmed();
        profile["pan"] = r.value("pan").toString().trimmed();
        profile["phone"] = r.value("phone").toString().trimmed();
        if (profile.value("phone").toString().isEmpty()) {
            profile["phone"] = r.value("mobile").toString().trimmed();
        }
        profile["state"] = r.value("state").toString().trimmed();
        profile["group_name"] = r.value("group_name").toString().trimmed();
    }
    return profile;
}

QString PrintExportController::numberToWords(long long n) {
    if (n == 0) return "";
    static const QString ones[] = {
        "", "One", "Two", "Three", "Four", "Five", "Six", "Seven", "Eight", "Nine",
        "Ten", "Eleven", "Twelve", "Thirteen", "Fourteen", "Fifteen", "Sixteen",
        "Seventeen", "Eighteen", "Nineteen"
    };
    static const QString tens[] = {
        "", "", "Twenty", "Thirty", "Forty", "Fifty", "Sixty", "Seventy", "Eighty", "Ninety"
    };

    if (n < 20) return ones[n];
    if (n < 100) return tens[n / 10] + (n % 10 != 0 ? " " + ones[n % 10] : "");
    if (n < 100) return ones[n / 100] + " Hundred" + (n % 100 != 0 ? " " + numberToWords(n % 100) : "");
    if (n < 1000) return ones[n / 100] + " Hundred" + (n % 100 != 0 ? " " + numberToWords(n % 100) : "");
    if (n < 100000) return numberToWords(n / 1000) + " Thousand" + (n % 1000 != 0 ? " " + numberToWords(n % 1000) : "");
    if (n < 10000000) return numberToWords(n / 100000) + " Lakh" + (n % 100000 != 0 ? " " + numberToWords(n % 100000) : "");
    return numberToWords(n / 10000000) + " Crore" + (n % 10000000 != 0 ? " " + numberToWords(n % 10000000) : "");
}

QString PrintExportController::amountInWords(double amount) {
    long long rupees = static_cast<long long>(std::floor(amount));
    int paise = static_cast<int>(std::round((amount - rupees) * 100.0));
    if (paise >= 100) {
        rupees += 1;
        paise = 0;
    }

    QString res = "INR ";
    if (rupees == 0) {
        res += "Zero";
    } else {
        res += numberToWords(rupees);
    }
    res += " Rupees";

    if (paise > 0) {
        res += " and " + numberToWords(paise) + " Paise";
    }
    res += " Only";
    return res;
}

// ==========================================
// 1. GENERIC EXPORT ENGINES
// ==========================================
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
        "th { background-color: #f1f5f9; font-weight: bold; border: 0.5pt solid #000000; padding: 5px; text-align: center; }\n"
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
    auto firm = getFirmProfile();

    QVariantList invRows = DatabaseManager::instance().executeQuery(
        "SELECT * FROM sales_invoices WHERE invoice_no = ? OR id = ? LIMIT 1;",
        {invoiceNo.trimmed(), invoiceNo.trimmed()}
    );
    if (invRows.isEmpty()) {
        return "<h2>Invoice not found: " + invoiceNo + "</h2>";
    }

    QVariantMap inv = invRows.first().toMap();
    int invId = inv.value("id").toInt();
    QString invNum = inv.value("invoice_no").toString();
    QString invDate = formatDisplayDate(inv.value("invoice_date").toString());
    QString custName = inv.value("customer_name").toString();
    int custId = inv.value("customer_id").toInt();
    QString vehNo = inv.value("vehicle_no").toString();

    QVariantList items = DatabaseManager::instance().executeQuery(
        "SELECT * FROM sales_invoice_items WHERE invoice_id = ? ORDER BY id ASC;",
        {invId}
    );

    QString itemRowsHtml = "";
    int rowIdx = 1;
    long long totalBags = 0;
    double totalWeight = 0.0;
    double totalTaxable = 0.0;
    double totalGst = 0.0;

    for (const auto& var : items) {
        QVariantMap it = var.toMap();
        long long bags = it.value("bag_count").toLongLong();
        double wt = it.value("weight_qtl").toDouble();
        double rate = it.value("rate_per_qtl").toDouble();
        double taxAmt = it.value("taxable_amount").toDouble();
        double gstVal = it.value("gst_amount").toDouble();

        totalBags += bags;
        totalWeight += wt;
        totalTaxable += taxAmt;
        totalGst += gstVal;

        itemRowsHtml += QString(
            "<tr>"
            "<td align='center'>%1</td>"
            "<td><b>%2</b></td>"
            "<td align='center'>%3</td>"
            "<td align='center'>%4</td>"
            "<td align='right'>%5</td>"
            "<td align='right'>%6</td>"
            "<td align='right'>%7</td>"
            "<td align='right'><b>%8</b></td>"
            "</tr>"
        )
        .arg(rowIdx++)
        .arg(it.value("item_name").toString())
        .arg(it.value("hsn_code", "1006").toString())
        .arg(bags)
        .arg(QString::number(wt, 'f', 2))
        .arg(QString::number(rate, 'f', 2))
        .arg(formatINR(taxAmt))
        .arg(formatINR(taxAmt + gstVal));
    }

    double grandTotal = inv.value("total_amount").toDouble();

    QString html = QString(
        "<!DOCTYPE html><html><head><style>"
        "body { font-family: 'Helvetica Neue', Arial, sans-serif; font-size: 8pt; color: #000; margin: 0; padding: 10px; }"
        "table { width: 100%; border-collapse: collapse; margin-bottom: 6px; }"
        "th, td { border: 1px solid #000; padding: 4px; font-size: 8pt; }"
        "th { background-color: #f1f5f9; text-align: center; font-weight: bold; }"
        ".header-box { border: 1.5px solid #000; padding: 8px; margin-bottom: 6px; text-align: center; }"
        "</style></head><body>"
        "<div class='header-box'>"
        "  <div style='font-size: 14pt; font-weight: bold;'>%1</div>"
        "  <div>%2 &nbsp;|&nbsp; Phone: %3</div>"
        "  <div><b>GSTIN:</b> %4 &nbsp;|&nbsp; <b>State:</b> %5</div>"
        "  <div style='font-size: 10pt; font-weight: bold; margin-top: 4px; text-decoration: underline;'>TAX INVOICE (%6)</div>"
        "</div>"
        "<table>"
        "  <tr>"
        "    <td width='50%' valign='top'><b>Billed To:</b><br><font size='3'><b>%7</b></font><br>GSTIN: %8</td>"
        "    <td width='50%' valign='top'><b>Invoice No:</b> %9<br><b>Date:</b> %10<br><b>Vehicle No:</b> %11</td>"
        "  </tr>"
        "</table>"
        "<table>"
        "  <tr>"
        "    <th width='5%'>S.N.</th>"
        "    <th width='35%'>Commodity / Description</th>"
        "    <th width='10%'>HSN</th>"
        "    <th width='10%'>Bags</th>"
        "    <th width='10%'>Weight (Qtl)</th>"
        "    <th width='10%'>Rate ₹</th>"
        "    <th width='10%'>Taxable ₹</th>"
        "    <th width='10%'>Total ₹</th>"
        "  </tr>"
        "  %12"
        "  <tr style='font-weight: bold; background-color: #f8fafc;'>"
        "    <td colspan='3' align='right'>TOTAL:</td>"
        "    <td align='center'>%13</td>"
        "    <td align='right'>%14</td>"
        "    <td></td>"
        "    <td align='right'>%15</td>"
        "    <td align='right'>%16</td>"
        "  </tr>"
        "</table>"
        "<table>"
        "  <tr>"
        "    <td width='65%'><b>Amount in Words:</b><br>%17</td>"
        "    <td width='35%' align='right'><b>GRAND TOTAL: <font size='3'>%16</font></b></td>"
        "  </tr>"
        "</table>"
        "<table style='margin-top: 15px; border: none;'>"
        "  <tr>"
        "    <td style='border: none;' width='50%'>E. & O.E.</td>"
        "    <td style='border: none;' width='50%' align='right'><b>For %1</b><br><br><br>Authorized Signatory</td>"
        "  </tr>"
        "</table>"
        "</body></html>"
    )
    .arg(firm.value("firm_name").toString())
    .arg(firm.value("address").toString())
    .arg(firm.value("phone").toString())
    .arg(firm.value("gstin").toString())
    .arg(firm.value("state").toString())
    .arg(copySubtitle)
    .arg(custName)
    .arg(inv.value("customer_gstin").toString())
    .arg(invNum)
    .arg(invDate)
    .arg(vehNo.isEmpty() ? "N/A" : vehNo)
    .arg(itemRowsHtml)
    .arg(totalBags)
    .arg(QString::number(totalWeight, 'f', 2))
    .arg(formatINR(totalTaxable))
    .arg(formatINR(grandTotal))
    .arg(amountInWords(grandTotal));

    return html;
}

QString PrintExportController::renderSalesInvoiceHtml(const QString& invoiceNo, const QString& copyType) {
    QString norm = copyType.trimmed().toUpper();
    if (norm == "ALL" || norm == "BOTH" || norm == "ORIGINAL + DUPLICATE" || norm == "ALL COPIES") {
        return renderSalesInvoiceSingleHtml(invoiceNo, "ORIGINAL FOR RECIPIENT") +
               "<div style='page-break-before: always;'></div>" +
               renderSalesInvoiceSingleHtml(invoiceNo, "DUPLICATE FOR TRANSPORTER");
    }
    return renderSalesInvoiceSingleHtml(invoiceNo, copyType);
}

QString PrintExportController::export_sales_invoice_pdf(const QString& invoiceNo, const QString& customPath, const QString& copyType) {
    QString html = renderSalesInvoiceHtml(invoiceNo, copyType);
    QString cleanInv = invoiceNo.trimmed().replace("/", "_").replace("\\", "_");
    QString fileName = "Invoice_" + cleanInv + ".pdf";
    return exportHtmlToPdf(html, fileName, customPath, false);
}

QString PrintExportController::export_sales_invoice_duplicate_pdf(const QString& invoiceNo, const QString& customPath) {
    return export_sales_invoice_pdf(invoiceNo, customPath, "DUPLICATE FOR TRANSPORTER");
}

QString PrintExportController::export_sales_invoice_all_copies_pdf(const QString& invoiceNo, const QString& customPath) {
    return export_sales_invoice_pdf(invoiceNo, customPath, "ALL");
}

QString PrintExportController::export_sales_invoice_odf(const QString& invoiceNo, const QString& customPath) {
    QString html = renderSalesInvoiceHtml(invoiceNo, "ORIGINAL FOR RECIPIENT");
    QString cleanInv = invoiceNo.trimmed().replace("/", "_").replace("\\", "_");
    QString fileName = "Invoice_" + cleanInv + ".odt";
    return exportHtmlToOdf(html, fileName, customPath, false);
}

QString PrintExportController::export_sales_invoice_excel(const QString& invoiceNo, const QString& customPath) {
    QString html = renderSalesInvoiceHtml(invoiceNo, "ORIGINAL FOR RECIPIENT");
    QString cleanInv = invoiceNo.trimmed().replace("/", "_").replace("\\", "_");
    QString fileName = "Invoice_" + cleanInv + ".xls";
    return exportHtmlToExcel(html, fileName, customPath);
}

bool PrintExportController::print_sales_invoice(const QString& invoiceNo, const QString& copyType) {
    QString html = renderSalesInvoiceHtml(invoiceNo, copyType);
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
    auto firm = getFirmProfile();

    QVariantList purcRows = DatabaseManager::instance().executeQuery(
        "SELECT * FROM purchase_invoices WHERE invoice_no = ? OR voucher_no = ? OR id = ? LIMIT 1;",
        {invoiceNo.trimmed(), invoiceNo.trimmed(), invoiceNo.trimmed()}
    );
    if (purcRows.isEmpty()) {
        return "<h2>Purchase voucher not found: " + invoiceNo + "</h2>";
    }

    QVariantMap p = purcRows.first().toMap();
    int pId = p.value("id").toInt();
    QString vNum = p.value("voucher_no").toString();
    QString invNum = p.value("invoice_no").toString();
    QString invDate = formatDisplayDate(p.value("invoice_date").toString());
    QString suppName = p.value("supplier_name").toString();
    double totalBill = p.value("total_amount").toDouble();

    QVariantList items = DatabaseManager::instance().executeQuery(
        "SELECT * FROM purchase_invoice_items WHERE invoice_id = ? ORDER BY id ASC;",
        {pId}
    );

    QString itemRowsHtml = "";
    int rowIdx = 1;
    long long totalBags = 0;
    double totalWeight = 0.0;
    double totalTaxable = 0.0;

    for (const auto& var : items) {
        QVariantMap it = var.toMap();
        long long bags = it.value("bag_count").toLongLong();
        double wt = it.value("weight_qtl").toDouble();
        double rate = it.value("rate_per_qtl").toDouble();
        double taxAmt = it.value("taxable_amount").toDouble();
        double gstVal = it.value("gst_amount").toDouble();

        totalBags += bags;
        totalWeight += wt;
        totalTaxable += taxAmt;

        itemRowsHtml += QString(
            "<tr>"
            "<td align='center'>%1</td>"
            "<td><b>%2</b></td>"
            "<td align='center'>%3</td>"
            "<td align='center'>%4</td>"
            "<td align='right'>%5</td>"
            "<td align='right'>%6</td>"
            "<td align='right'>%7</td>"
            "<td align='right'><b>%8</b></td>"
            "</tr>"
        )
        .arg(rowIdx++)
        .arg(it.value("item_name").toString())
        .arg(it.value("hsn_code", "1006").toString())
        .arg(bags)
        .arg(QString::number(wt, 'f', 2))
        .arg(QString::number(rate, 'f', 2))
        .arg(formatINR(taxAmt))
        .arg(formatINR(taxAmt + gstVal));
    }

    QString html = QString(
        "<!DOCTYPE html><html><head><style>"
        "body { font-family: 'Helvetica Neue', Arial, sans-serif; font-size: 8pt; color: #000; margin: 0; padding: 10px; }"
        "table { width: 100%; border-collapse: collapse; margin-bottom: 6px; }"
        "th, td { border: 1px solid #000; padding: 4px; font-size: 8pt; }"
        "th { background-color: #f1f5f9; text-align: center; font-weight: bold; }"
        ".header-box { border: 1.5px solid #000; padding: 8px; margin-bottom: 6px; text-align: center; }"
        "</style></head><body>"
        "<div class='header-box'>"
        "  <div style='font-size: 14pt; font-weight: bold;'>%1</div>"
        "  <div>%2 &nbsp;|&nbsp; Phone: %3</div>"
        "  <div style='font-size: 10pt; font-weight: bold; margin-top: 4px;'>GOODS INWARD / PURCHASE VOUCHER</div>"
        "</div>"
        "<table>"
        "  <tr>"
        "    <td width='50%' valign='top'><b>Supplier:</b><br><font size='3'><b>%4</b></font><br>GSTIN: %5</td>"
        "    <td width='50%' valign='top'><b>Voucher No:</b> %6<br><b>Bill No:</b> %7<br><b>Date:</b> %8</td>"
        "  </tr>"
        "</table>"
        "<table>"
        "  <tr>"
        "    <th width='5%'>S.N.</th>"
        "    <th width='35%'>Commodity / Item</th>"
        "    <th width='10%'>HSN</th>"
        "    <th width='10%'>Bags</th>"
        "    <th width='10%'>Weight (Qtl)</th>"
        "    <th width='10%'>Rate ₹</th>"
        "    <th width='10%'>Taxable ₹</th>"
        "    <th width='10%'>Total ₹</th>"
        "  </tr>"
        "  %9"
        "  <tr style='font-weight: bold; background-color: #f8fafc;'>"
        "    <td colspan='3' align='right'>TOTAL:</td>"
        "    <td align='center'>%10</td>"
        "    <td align='right'>%11</td>"
        "    <td></td>"
        "    <td align='right'>%12</td>"
        "    <td align='right'>%13</td>"
        "  </tr>"
        "</table>"
        "<table>"
        "  <tr>"
        "    <td width='65%'><b>Amount in Words:</b><br>%14</td>"
        "    <td width='35%' align='right'><b>TOTAL BILL: <font size='3'>%13</font></b></td>"
        "  </tr>"
        "</table>"
        "</body></html>"
    )
    .arg(firm.value("firm_name").toString())
    .arg(firm.value("address").toString())
    .arg(firm.value("phone").toString())
    .arg(suppName)
    .arg(p.value("supplier_gstin").toString())
    .arg(vNum)
    .arg(invNum)
    .arg(invDate)
    .arg(itemRowsHtml)
    .arg(totalBags)
    .arg(QString::number(totalWeight, 'f', 2))
    .arg(formatINR(totalTaxable))
    .arg(formatINR(totalBill))
    .arg(amountInWords(totalBill));

    return html;
}

QString PrintExportController::export_purchase_invoice_pdf(const QString& invoiceNo, const QString& customPath) {
    QString html = renderPurchaseInvoiceHtml(invoiceNo);
    QString cleanInv = invoiceNo.trimmed().replace("/", "_").replace("\\", "_");
    QString fileName = "Purchase_" + cleanInv + ".pdf";
    return exportHtmlToPdf(html, fileName, customPath, false);
}

QString PrintExportController::export_purchase_invoice_odf(const QString& invoiceNo, const QString& customPath) {
    QString html = renderPurchaseInvoiceHtml(invoiceNo);
    QString cleanInv = invoiceNo.trimmed().replace("/", "_").replace("\\", "_");
    QString fileName = "Purchase_" + cleanInv + ".odt";
    return exportHtmlToOdf(html, fileName, customPath, false);
}

QString PrintExportController::export_purchase_invoice_excel(const QString& invoiceNo, const QString& customPath) {
    QString html = renderPurchaseInvoiceHtml(invoiceNo);
    QString cleanInv = invoiceNo.trimmed().replace("/", "_").replace("\\", "_");
    QString fileName = "Purchase_" + cleanInv + ".xls";
    return exportHtmlToExcel(html, fileName, customPath);
}

bool PrintExportController::print_purchase_invoice(const QString& invoiceNo) {
    QString html = renderPurchaseInvoiceHtml(invoiceNo);
    return printHtml(html, "Purchase " + invoiceNo, false);
}

// 2.3 Cash Voucher (Payment / Receipt)
QString PrintExportController::renderCashVoucherHtml(const QString& voucherNo, const QString& voucherType) {
    auto firm = getFirmProfile();

    QVariantList txRows = DatabaseManager::instance().executeQuery(
        "SELECT * FROM transactions WHERE voucher_no = ? ORDER BY id ASC;",
        {voucherNo.trimmed()}
    );
    if (txRows.isEmpty()) {
        return "<h2>Cash voucher not found: " + voucherNo + "</h2>";
    }

    QVariantMap first = txRows.first().toMap();
    QString vDate = formatDisplayDate(first.value("voucher_date").toString());
    QString pName = first.value("party_name").toString();
    QString oppAcc = first.value("opposing_account").toString();
    double amt = first.value("amount").toDouble();
    QString narr = first.value("narration").toString();
    QString vType = first.value("voucher_type", voucherType).toString();

    QString html = QString(
        "<!DOCTYPE html><html><head><style>"
        "body { font-family: 'Helvetica Neue', Arial, sans-serif; font-size: 8.5pt; color: #000; margin: 0; padding: 10px; }"
        "table { width: 100%; border-collapse: collapse; margin-bottom: 6px; }"
        "th, td { border: 1px solid #000; padding: 5px; font-size: 8.5pt; }"
        ".header-box { border: 1.5px solid #000; padding: 6px; margin-bottom: 6px; text-align: center; }"
        "</style></head><body>"
        "<div class='header-box'>"
        "  <div style='font-size: 13pt; font-weight: bold;'>%1</div>"
        "  <div>%2 &nbsp;|&nbsp; Phone: %3</div>"
        "  <div style='font-size: 10pt; font-weight: bold; margin-top: 4px;'>%4</div>"
        "</div>"
        "<table>"
        "  <tr>"
        "    <td width='50%'><b>Voucher No:</b> %5</td>"
        "    <td width='50%'><b>Date:</b> %6</td>"
        "  </tr>"
        "  <tr>"
        "    <td><b>Account / Party:</b><br><font size='3'><b>%7</b></font></td>"
        "    <td><b>Cash Account:</b><br><b>%8</b></td>"
        "  </tr>"
        "  <tr>"
        "    <td colspan='2'><b>Amount:</b> <font size='3'><b>%9</b></font><br><i>(%10)</i></td>"
        "  </tr>"
        "  <tr>"
        "    <td colspan='2'><b>Narration / Remarks:</b><br>%11</td>"
        "  </tr>"
        "</table>"
        "<table style='margin-top: 25px; border: none;'>"
        "  <tr>"
        "    <td style='border: none;' width='33%' align='center'>Prepared By</td>"
        "    <td style='border: none;' width='33%' align='center'>Passed By</td>"
        "    <td style='border: none;' width='34%' align='center'>Receiver's Signature</td>"
        "  </tr>"
        "</table>"
        "</body></html>"
    )
    .arg(firm.value("firm_name").toString())
    .arg(firm.value("address").toString())
    .arg(firm.value("phone").toString())
    .arg(vType.toUpper())
    .arg(voucherNo)
    .arg(vDate)
    .arg(pName)
    .arg(oppAcc.isEmpty() ? "Cash-In-Hand" : oppAcc)
    .arg(formatINR(amt))
    .arg(amountInWords(amt))
    .arg(narr.isEmpty() ? "Cash transaction" : narr);

    return html;
}

QString PrintExportController::export_cash_voucher_pdf(const QString& voucherNo, const QString& customPath) {
    QString html = renderCashVoucherHtml(voucherNo);
    QString clean = voucherNo.trimmed().replace("/", "_").replace("\\", "_");
    return exportHtmlToPdf(html, "Cash_" + clean + ".pdf", customPath, false);
}

QString PrintExportController::export_cash_voucher_odf(const QString& voucherNo, const QString& customPath) {
    QString html = renderCashVoucherHtml(voucherNo);
    QString clean = voucherNo.trimmed().replace("/", "_").replace("\\", "_");
    return exportHtmlToOdf(html, "Cash_" + clean + ".odt", customPath, false);
}

QString PrintExportController::export_cash_voucher_excel(const QString& voucherNo, const QString& customPath) {
    QString html = renderCashVoucherHtml(voucherNo);
    QString clean = voucherNo.trimmed().replace("/", "_").replace("\\", "_");
    return exportHtmlToExcel(html, "Cash_" + clean + ".xls", customPath);
}

bool PrintExportController::print_cash_voucher(const QString& voucherNo) {
    QString html = renderCashVoucherHtml(voucherNo);
    return printHtml(html, "Cash Voucher " + voucherNo, false);
}

// 2.4 Cheque / Bank Voucher
QString PrintExportController::renderChequeVoucherHtml(const QString& voucherNo, const QString& voucherType) {
    auto firm = getFirmProfile();

    QVariantList txRows = DatabaseManager::instance().executeQuery(
        "SELECT * FROM transactions WHERE voucher_no = ? ORDER BY id ASC;",
        {voucherNo.trimmed()}
    );
    if (txRows.isEmpty()) {
        return "<h2>Bank voucher not found: " + voucherNo + "</h2>";
    }

    QVariantMap first = txRows.first().toMap();
    QString vDate = formatDisplayDate(first.value("voucher_date").toString());
    QString pName = first.value("party_name").toString();
    QString oppAcc = first.value("opposing_account").toString();
    double amt = first.value("amount").toDouble();
    QString narr = first.value("narration").toString();
    QString bDate = formatDisplayDate(first.value("bank_date").toString());
    QString chqNo = first.value("invoice_no").toString();

    QString html = QString(
        "<!DOCTYPE html><html><head><style>"
        "body { font-family: 'Helvetica Neue', Arial, sans-serif; font-size: 8.5pt; color: #000; margin: 0; padding: 10px; }"
        "table { width: 100%; border-collapse: collapse; margin-bottom: 6px; }"
        "th, td { border: 1px solid #000; padding: 5px; font-size: 8.5pt; }"
        ".header-box { border: 1.5px solid #000; padding: 6px; margin-bottom: 6px; text-align: center; }"
        "</style></head><body>"
        "<div class='header-box'>"
        "  <div style='font-size: 13pt; font-weight: bold;'>%1</div>"
        "  <div>%2 &nbsp;|&nbsp; Phone: %3</div>"
        "  <div style='font-size: 10pt; font-weight: bold; margin-top: 4px;'>%4</div>"
        "</div>"
        "<table>"
        "  <tr>"
        "    <td width='50%'><b>Voucher No:</b> %5</td>"
        "    <td width='50%'><b>Voucher Date:</b> %6</td>"
        "  </tr>"
        "  <tr>"
        "    <td><b>Party / Beneficiary:</b><br><font size='3'><b>%7</b></font></td>"
        "    <td><b>Bank Account:</b><br><b>%8</b></td>"
        "  </tr>"
        "  <tr>"
        "    <td><b>Cheque / Ref No:</b> %9</td>"
        "    <td><b>Cheque / Value Date:</b> %10</td>"
        "  </tr>"
        "  <tr>"
        "    <td colspan='2'><b>Amount:</b> <font size='3'><b>%11</b></font><br><i>(%12)</i></td>"
        "  </tr>"
        "  <tr>"
        "    <td colspan='2'><b>Narration / Particulars:</b><br>%13</td>"
        "  </tr>"
        "</table>"
        "<table style='margin-top: 25px; border: none;'>"
        "  <tr>"
        "    <td style='border: none;' width='50%'>Prepared By: Accounts</td>"
        "    <td style='border: none;' width='50%' align='right'><b>For %1</b><br><br><br>Authorized Signatory</td>"
        "  </tr>"
        "</table>"
        "</body></html>"
    )
    .arg(firm.value("firm_name").toString())
    .arg(firm.value("address").toString())
    .arg(firm.value("phone").toString())
    .arg(voucherType.toUpper())
    .arg(voucherNo)
    .arg(vDate)
    .arg(pName)
    .arg(oppAcc)
    .arg(chqNo.isEmpty() ? "N/A" : chqNo)
    .arg(bDate.isEmpty() ? vDate : bDate)
    .arg(formatINR(amt))
    .arg(amountInWords(amt))
    .arg(narr.isEmpty() ? "Bank transaction" : narr);

    return html;
}

QString PrintExportController::export_cheque_voucher_pdf(const QString& voucherNo, const QString& customPath) {
    QString html = renderChequeVoucherHtml(voucherNo);
    QString clean = voucherNo.trimmed().replace("/", "_").replace("\\", "_");
    return exportHtmlToPdf(html, "Cheque_" + clean + ".pdf", customPath, false);
}

QString PrintExportController::export_cheque_voucher_odf(const QString& voucherNo, const QString& customPath) {
    QString html = renderChequeVoucherHtml(voucherNo);
    QString clean = voucherNo.trimmed().replace("/", "_").replace("\\", "_");
    return exportHtmlToOdf(html, "Cheque_" + clean + ".odt", customPath, false);
}

QString PrintExportController::export_cheque_voucher_excel(const QString& voucherNo, const QString& customPath) {
    QString html = renderChequeVoucherHtml(voucherNo);
    QString clean = voucherNo.trimmed().replace("/", "_").replace("\\", "_");
    return exportHtmlToExcel(html, "Cheque_" + clean + ".xls", customPath);
}

bool PrintExportController::print_cheque_voucher(const QString& voucherNo) {
    QString html = renderChequeVoucherHtml(voucherNo);
    return printHtml(html, "Cheque Voucher " + voucherNo, false);
}

// 2.5 Journal Voucher
QString PrintExportController::renderJournalVoucherHtml(const QString& voucherNo) {
    auto firm = getFirmProfile();

    QVariantList txRows = DatabaseManager::instance().executeQuery(
        "SELECT * FROM transactions WHERE voucher_no = ? ORDER BY id ASC;",
        {voucherNo.trimmed()}
    );
    if (txRows.isEmpty()) {
        return "<h2>Journal voucher not found: " + voucherNo + "</h2>";
    }

    QString vDate = formatDisplayDate(txRows.first().toMap().value("voucher_date").toString());
    QString rowsHtml = "";
    int rowIdx = 1;
    double totalDr = 0.0, totalCr = 0.0;
    QString narr = "";

    for (const auto& var : txRows) {
        QVariantMap t = var.toMap();
        QString side = t.value("dr_cr").toString();
        double amt = t.value("amount").toDouble();
        if (narr.isEmpty()) narr = t.value("narration").toString();

        double drAmt = (side == "Dr") ? amt : 0.0;
        double crAmt = (side == "Cr") ? amt : 0.0;
        totalDr += drAmt;
        totalCr += crAmt;

        rowsHtml += QString(
            "<tr>"
            "<td align='center'>%1</td>"
            "<td><b>%2</b></td>"
            "<td>%3</td>"
            "<td align='right'>%4</td>"
            "<td align='right'>%5</td>"
            "</tr>"
        )
        .arg(rowIdx++)
        .arg(t.value("party_name").toString())
        .arg(t.value("opposing_account").toString())
        .arg(drAmt > 0 ? formatINR(drAmt) : "-")
        .arg(crAmt > 0 ? formatINR(crAmt) : "-");
    }

    QString html = QString(
        "<!DOCTYPE html><html><head><style>"
        "body { font-family: 'Helvetica Neue', Arial, sans-serif; font-size: 8.5pt; color: #000; margin: 0; padding: 10px; }"
        "table { width: 100%; border-collapse: collapse; margin-bottom: 6px; }"
        "th, td { border: 1px solid #000; padding: 5px; font-size: 8.5pt; }"
        "th { background-color: #f1f5f9; text-align: center; font-weight: bold; }"
        ".header-box { border: 1.5px solid #000; padding: 6px; margin-bottom: 6px; text-align: center; }"
        "</style></head><body>"
        "<div class='header-box'>"
        "  <div style='font-size: 13pt; font-weight: bold;'>%1</div>"
        "  <div>%2 &nbsp;|&nbsp; Phone: %3</div>"
        "  <div style='font-size: 10pt; font-weight: bold; margin-top: 4px;'>JOURNAL ADJUSTMENT VOUCHER</div>"
        "</div>"
        "<table>"
        "  <tr>"
        "    <td width='50%'><b>Voucher No:</b> %4</td>"
        "    <td width='50%'><b>Date:</b> %5</td>"
        "  </tr>"
        "</table>"
        "<table>"
        "  <tr>"
        "    <th width='6%'>S.N.</th>"
        "    <th width='44%'>Particulars / Account</th>"
        "    <th width='26%'>Opposing Account</th>"
        "    <th width='12%'>Debit ₹</th>"
        "    <th width='12%'>Credit ₹</th>"
        "  </tr>"
        "  %6"
        "  <tr style='font-weight: bold; background-color: #f8fafc;'>"
        "    <td colspan='3' align='right'>TOTAL:</td>"
        "    <td align='right'>%7</td>"
        "    <td align='right'>%8</td>"
        "  </tr>"
        "</table>"
        "<table>"
        "  <tr>"
        "    <td><b>Narration:</b><br>%9</td>"
        "  </tr>"
        "</table>"
        "<table style='margin-top: 25px; border: none;'>"
        "  <tr>"
        "    <td style='border: none;' width='50%'>Prepared By: Accounts</td>"
        "    <td style='border: none;' width='50%' align='right'><b>For %1</b><br><br><br>Authorized Signatory</td>"
        "  </tr>"
        "</table>"
        "</body></html>"
    )
    .arg(firm.value("firm_name").toString())
    .arg(firm.value("address").toString())
    .arg(firm.value("phone").toString())
    .arg(voucherNo)
    .arg(vDate)
    .arg(rowsHtml)
    .arg(formatINR(totalDr))
    .arg(formatINR(totalCr))
    .arg(narr.isEmpty() ? "Adjustment entry" : narr);

    return html;
}

QString PrintExportController::export_journal_voucher_pdf(const QString& voucherNo, const QString& customPath) {
    QString html = renderJournalVoucherHtml(voucherNo);
    QString clean = voucherNo.trimmed().replace("/", "_").replace("\\", "_");
    return exportHtmlToPdf(html, "Journal_" + clean + ".pdf", customPath, false);
}

QString PrintExportController::export_journal_voucher_odf(const QString& voucherNo, const QString& customPath) {
    QString html = renderJournalVoucherHtml(voucherNo);
    QString clean = voucherNo.trimmed().replace("/", "_").replace("\\", "_");
    return exportHtmlToOdf(html, "Journal_" + clean + ".odt", customPath, false);
}

QString PrintExportController::export_journal_voucher_excel(const QString& voucherNo, const QString& customPath) {
    QString html = renderJournalVoucherHtml(voucherNo);
    QString clean = voucherNo.trimmed().replace("/", "_").replace("\\", "_");
    return exportHtmlToExcel(html, "Journal_" + clean + ".xls", customPath);
}

bool PrintExportController::print_journal_voucher(const QString& voucherNo) {
    QString html = renderJournalVoucherHtml(voucherNo);
    return printHtml(html, "Journal Voucher " + voucherNo, false);
}

// 2.6 J-Form Voucher (Paddy Procurement)
QString PrintExportController::renderJFormVoucherHtml(const QString& voucherNo) {
    auto firm = getFirmProfile();

    QVariantList jRows = DatabaseManager::instance().executeQuery(
        "SELECT * FROM jform_vouchers WHERE voucher_no = ? OR id = ? LIMIT 1;",
        {voucherNo.trimmed(), voucherNo.trimmed()}
    );
    if (jRows.isEmpty()) {
        return "<h2>J-Form voucher not found: " + voucherNo + "</h2>";
    }

    QVariantMap j = jRows.first().toMap();
    QString vDate = formatDisplayDate(j.value("voucher_date").toString());
    QString farmer = j.value("farmer_name").toString();
    QString commAgent = j.value("broker_name", j.value("commission_agent")).toString();
    QString variety = j.value("item_name", j.value("commodity")).toString();
    long long bags = j.value("bag_count").toLongLong();
    double wt = j.value("weight_qtl").toDouble();
    double rate = j.value("rate_per_qtl").toDouble();
    double grossAmt = j.value("gross_amount", wt * rate).toDouble();
    double netAmt = j.value("total_amount", grossAmt).toDouble();

    QString html = QString(
        "<!DOCTYPE html><html><head><style>"
        "body { font-family: 'Helvetica Neue', Arial, sans-serif; font-size: 8.5pt; color: #000; margin: 0; padding: 10px; }"
        "table { width: 100%; border-collapse: collapse; margin-bottom: 6px; }"
        "th, td { border: 1px solid #000; padding: 5px; font-size: 8.5pt; }"
        "th { background-color: #f1f5f9; text-align: center; font-weight: bold; }"
        ".header-box { border: 1.5px solid #000; padding: 6px; margin-bottom: 6px; text-align: center; }"
        "</style></head><body>"
        "<div class='header-box'>"
        "  <div style='font-size: 13pt; font-weight: bold;'>%1</div>"
        "  <div>%2 &nbsp;|&nbsp; Mandi: %3</div>"
        "  <div style='font-size: 10pt; font-weight: bold; margin-top: 4px;'>FORM 'J' - PADDY PURCHASE VOUCHER (MANDI)</div>"
        "</div>"
        "<table>"
        "  <tr>"
        "    <td width='50%'><b>J-Form No:</b> %4</td>"
        "    <td width='50%'><b>Date:</b> %5</td>"
        "  </tr>"
        "  <tr>"
        "    <td><b>Farmer / Seller:</b><br><font size='3'><b>%6</b></font></td>"
        "    <td><b>Commission Agent / Kaccha Arhtia:</b><br><b>%7</b></td>"
        "  </tr>"
        "</table>"
        "<table>"
        "  <tr>"
        "    <th>Commodity / Variety</th>"
        "    <th>Bags</th>"
        "    <th>Weight (Qtl)</th>"
        "    <th>Rate ₹ / Qtl</th>"
        "    <th>Gross Amount ₹</th>"
        "    <th>Net Payable ₹</th>"
        "  </tr>"
        "  <tr>"
        "    <td><b>%8</b></td>"
        "    <td align='center'>%9</td>"
        "    <td align='right'>%10</td>"
        "    <td align='right'>%11</td>"
        "    <td align='right'>%12</td>"
        "    <td align='right'><b>%13</b></td>"
        "  </tr>"
        "</table>"
        "<table>"
        "  <tr>"
        "    <td width='65%'><b>Amount in Words:</b><br>%14</td>"
        "    <td width='35%' align='right'><b>NET PAYABLE: <font size='3'>%13</font></b></td>"
        "  </tr>"
        "</table>"
        "<table style='margin-top: 25px; border: none;'>"
        "  <tr>"
        "    <td style='border: none;' width='50%'>Farmer's Signature</td>"
        "    <td style='border: none;' width='50%' align='right'><b>For %1</b><br><br><br>Authorized Signatory</td>"
        "  </tr>"
        "</table>"
        "</body></html>"
    )
    .arg(firm.value("firm_name").toString())
    .arg(firm.value("address").toString())
    .arg(firm.value("city", "Mandi Area").toString())
    .arg(voucherNo)
    .arg(vDate)
    .arg(farmer)
    .arg(commAgent.isEmpty() ? "Direct Mandi" : commAgent)
    .arg(variety)
    .arg(bags)
    .arg(QString::number(wt, 'f', 2))
    .arg(QString::number(rate, 'f', 2))
    .arg(formatINR(grossAmt))
    .arg(formatINR(netAmt))
    .arg(amountInWords(netAmt));

    return html;
}

QString PrintExportController::export_jform_voucher_pdf(const QString& voucherNo, const QString& customPath) {
    QString html = renderJFormVoucherHtml(voucherNo);
    QString clean = voucherNo.trimmed().replace("/", "_").replace("\\", "_");
    return exportHtmlToPdf(html, "JForm_" + clean + ".pdf", customPath, false);
}

QString PrintExportController::export_jform_voucher_odf(const QString& voucherNo, const QString& customPath) {
    QString html = renderJFormVoucherHtml(voucherNo);
    QString clean = voucherNo.trimmed().replace("/", "_").replace("\\", "_");
    return exportHtmlToOdf(html, "JForm_" + clean + ".odt", customPath, false);
}

QString PrintExportController::export_jform_voucher_excel(const QString& voucherNo, const QString& customPath) {
    QString html = renderJFormVoucherHtml(voucherNo);
    QString clean = voucherNo.trimmed().replace("/", "_").replace("\\", "_");
    return exportHtmlToExcel(html, "JForm_" + clean + ".xls", customPath);
}

bool PrintExportController::print_jform_voucher(const QString& voucherNo) {
    QString html = renderJFormVoucherHtml(voucherNo);
    return printHtml(html, "J-Form " + voucherNo, false);
}

// 2.7 I-Form Voucher (Commission Sale)
QString PrintExportController::renderIFormVoucherHtml(const QString& voucherNo) {
    auto firm = getFirmProfile();

    QVariantList iRows = DatabaseManager::instance().executeQuery(
        "SELECT * FROM iform_vouchers WHERE voucher_no = ? OR id = ? LIMIT 1;",
        {voucherNo.trimmed(), voucherNo.trimmed()}
    );
    if (iRows.isEmpty()) {
        return "<h2>I-Form voucher not found: " + voucherNo + "</h2>";
    }

    QVariantMap i = iRows.first().toMap();
    QString vDate = formatDisplayDate(i.value("voucher_date").toString());
    QString buyer = i.value("buyer_name").toString();
    QString variety = i.value("item_name", i.value("commodity")).toString();
    long long bags = i.value("bag_count").toLongLong();
    double wt = i.value("weight_qtl").toDouble();
    double rate = i.value("rate_per_qtl").toDouble();
    double totalAmt = i.value("total_amount", wt * rate).toDouble();

    QString html = QString(
        "<!DOCTYPE html><html><head><style>"
        "body { font-family: 'Helvetica Neue', Arial, sans-serif; font-size: 8.5pt; color: #000; margin: 0; padding: 10px; }"
        "table { width: 100%; border-collapse: collapse; margin-bottom: 6px; }"
        "th, td { border: 1px solid #000; padding: 5px; font-size: 8.5pt; }"
        "th { background-color: #f1f5f9; text-align: center; font-weight: bold; }"
        ".header-box { border: 1.5px solid #000; padding: 6px; margin-bottom: 6px; text-align: center; }"
        "</style></head><body>"
        "<div class='header-box'>"
        "  <div style='font-size: 13pt; font-weight: bold;'>%1</div>"
        "  <div>%2 &nbsp;|&nbsp; Mandi: %3</div>"
        "  <div style='font-size: 10pt; font-weight: bold; margin-top: 4px;'>FORM 'I' - SALE VOUCHER / MEMO (MANDI)</div>"
        "</div>"
        "<table>"
        "  <tr>"
        "    <td width='50%'><b>I-Form No:</b> %4</td>"
        "    <td width='50%'><b>Date:</b> %5</td>"
        "  </tr>"
        "  <tr>"
        "    <td colspan='2'><b>Buyer / Trader:</b><br><font size='3'><b>%6</b></font></td>"
        "  </tr>"
        "</table>"
        "<table>"
        "  <tr>"
        "    <th>Commodity / Variety</th>"
        "    <th>Bags</th>"
        "    <th>Weight (Qtl)</th>"
        "    <th>Rate ₹ / Qtl</th>"
        "    <th>Total Amount ₹</th>"
        "  </tr>"
        "  <tr>"
        "    <td><b>%7</b></td>"
        "    <td align='center'>%8</td>"
        "    <td align='right'>%9</td>"
        "    <td align='right'>%10</td>"
        "    <td align='right'><b>%11</b></td>"
        "  </tr>"
        "</table>"
        "<table>"
        "  <tr>"
        "    <td width='65%'><b>Amount in Words:</b><br>%12</td>"
        "    <td width='35%' align='right'><b>TOTAL AMOUNT: <font size='3'>%11</font></b></td>"
        "  </tr>"
        "</table>"
        "<table style='margin-top: 25px; border: none;'>"
        "  <tr>"
        "    <td style='border: none;' width='50%'>Buyer's Signature</td>"
        "    <td style='border: none;' width='50%' align='right'><b>For %1</b><br><br><br>Authorized Signatory</td>"
        "  </tr>"
        "</table>"
        "</body></html>"
    )
    .arg(firm.value("firm_name").toString())
    .arg(firm.value("address").toString())
    .arg(firm.value("city", "Mandi Area").toString())
    .arg(voucherNo)
    .arg(vDate)
    .arg(buyer)
    .arg(variety)
    .arg(bags)
    .arg(QString::number(wt, 'f', 2))
    .arg(QString::number(rate, 'f', 2))
    .arg(formatINR(totalAmt))
    .arg(amountInWords(totalAmt));

    return html;
}

QString PrintExportController::export_iform_voucher_pdf(const QString& voucherNo, const QString& customPath) {
    QString html = renderIFormVoucherHtml(voucherNo);
    QString clean = voucherNo.trimmed().replace("/", "_").replace("\\", "_");
    return exportHtmlToPdf(html, "IForm_" + clean + ".pdf", customPath, false);
}

QString PrintExportController::export_iform_voucher_odf(const QString& voucherNo, const QString& customPath) {
    QString html = renderIFormVoucherHtml(voucherNo);
    QString clean = voucherNo.trimmed().replace("/", "_").replace("\\", "_");
    return exportHtmlToOdf(html, "IForm_" + clean + ".odt", customPath, false);
}

QString PrintExportController::export_iform_voucher_excel(const QString& voucherNo, const QString& customPath) {
    QString html = renderIFormVoucherHtml(voucherNo);
    QString clean = voucherNo.trimmed().replace("/", "_").replace("\\", "_");
    return exportHtmlToExcel(html, "IForm_" + clean + ".xls", customPath);
}

bool PrintExportController::print_iform_voucher(const QString& voucherNo) {
    QString html = renderIFormVoucherHtml(voucherNo);
    return printHtml(html, "I-Form " + voucherNo, false);
}

// 2.8 TDS / TCS / Tax Challan
QString PrintExportController::renderTdsVoucherHtml(const QString& voucherNo) {
    auto firm = getFirmProfile();

    QVariantList rows = DatabaseManager::instance().executeQuery(
        "SELECT * FROM tds_vouchers WHERE voucher_no = ? OR id = ? LIMIT 1;",
        {voucherNo.trimmed(), voucherNo.trimmed()}
    );
    if (rows.isEmpty()) {
        return "<h2>TDS voucher not found: " + voucherNo + "</h2>";
    }

    QVariantMap r = rows.first().toMap();
    QString vDate = formatDisplayDate(r.value("voucher_date").toString());
    QString pName = r.value("party_name").toString();
    QString pan = r.value("pan").toString();
    QString sec = r.value("section").toString();
    double gross = r.value("gross_amount").toDouble();
    double rate = r.value("tds_rate").toDouble();
    double tdsAmt = r.value("tds_amount").toDouble();

    QString html = QString(
        "<!DOCTYPE html><html><head><style>"
        "body { font-family: 'Helvetica Neue', Arial, sans-serif; font-size: 8.5pt; color: #000; margin: 0; padding: 10px; }"
        "table { width: 100%; border-collapse: collapse; margin-bottom: 6px; }"
        "th, td { border: 1px solid #000; padding: 5px; font-size: 8.5pt; }"
        ".header-box { border: 1.5px solid #000; padding: 6px; margin-bottom: 6px; text-align: center; }"
        "</style></head><body>"
        "<div class='header-box'>"
        "  <div style='font-size: 13pt; font-weight: bold;'>%1</div>"
        "  <div>%2 &nbsp;|&nbsp; TAN: %3</div>"
        "  <div style='font-size: 10pt; font-weight: bold; margin-top: 4px;'>TDS DEDUCTION CERTIFICATE / SLIP</div>"
        "</div>"
        "<table>"
        "  <tr>"
        "    <td width='50%'><b>Voucher No:</b> %4</td>"
        "    <td width='50%'><b>Date:</b> %5</td>"
        "  </tr>"
        "  <tr>"
        "    <td><b>Deductee / Party:</b><br><font size='3'><b>%6</b></font></td>"
        "    <td><b>PAN:</b> %7<br><b>Section:</b> %8</td>"
        "  </tr>"
        "  <tr>"
        "    <td><b>Gross Payment Amount:</b><br>%9</td>"
        "    <td><b>TDS Rate:</b> %10% &nbsp;|&nbsp; <b>TDS Deducted:</b><br><font size='3'><b>%11</b></font></td>"
        "  </tr>"
        "</table>"
        "</body></html>"
    )
    .arg(firm.value("firm_name").toString())
    .arg(firm.value("address").toString())
    .arg(firm.value("tan", "N/A").toString())
    .arg(voucherNo)
    .arg(vDate)
    .arg(pName)
    .arg(pan)
    .arg(sec)
    .arg(formatINR(gross))
    .arg(QString::number(rate, 'f', 2))
    .arg(formatINR(tdsAmt));

    return html;
}

QString PrintExportController::export_tds_voucher_pdf(const QString& voucherNo, const QString& customPath) {
    QString html = renderTdsVoucherHtml(voucherNo);
    QString clean = voucherNo.trimmed().replace("/", "_").replace("\\", "_");
    return exportHtmlToPdf(html, "TDS_" + clean + ".pdf", customPath, false);
}

QString PrintExportController::export_tds_voucher_odf(const QString& voucherNo, const QString& customPath) {
    QString html = renderTdsVoucherHtml(voucherNo);
    QString clean = voucherNo.trimmed().replace("/", "_").replace("\\", "_");
    return exportHtmlToOdf(html, "TDS_" + clean + ".odt", customPath, false);
}

QString PrintExportController::export_tds_voucher_excel(const QString& voucherNo, const QString& customPath) {
    QString html = renderTdsVoucherHtml(voucherNo);
    QString clean = voucherNo.trimmed().replace("/", "_").replace("\\", "_");
    return exportHtmlToExcel(html, "TDS_" + clean + ".xls", customPath);
}

bool PrintExportController::print_tds_voucher(const QString& voucherNo) {
    QString html = renderTdsVoucherHtml(voucherNo);
    return printHtml(html, "TDS " + voucherNo, false);
}

// 2.9 Debit / Credit Note
QString PrintExportController::renderDebitCreditNoteHtml(const QString& noteNo) {
    auto firm = getFirmProfile();

    QVariantList rows = DatabaseManager::instance().executeQuery(
        "SELECT * FROM debit_credit_notes WHERE note_no = ? OR id = ? LIMIT 1;",
        {noteNo.trimmed(), noteNo.trimmed()}
    );
    if (rows.isEmpty()) {
        return "<h2>Note not found: " + noteNo + "</h2>";
    }

    QVariantMap r = rows.first().toMap();
    QString nDate = formatDisplayDate(r.value("note_date").toString());
    QString nType = r.value("note_type", "Debit Note").toString();
    QString pName = r.value("party_name").toString();
    QString origInv = r.value("original_invoice_no").toString();
    double taxable = r.value("taxable_amount").toDouble();
    double gst = r.value("gst_amount").toDouble();
    double total = r.value("total_amount").toDouble();
    QString reason = r.value("reason").toString();

    QString html = QString(
        "<!DOCTYPE html><html><head><style>"
        "body { font-family: 'Helvetica Neue', Arial, sans-serif; font-size: 8.5pt; color: #000; margin: 0; padding: 10px; }"
        "table { width: 100%; border-collapse: collapse; margin-bottom: 6px; }"
        "th, td { border: 1px solid #000; padding: 5px; font-size: 8.5pt; }"
        ".header-box { border: 1.5px solid #000; padding: 6px; margin-bottom: 6px; text-align: center; }"
        "</style></head><body>"
        "<div class='header-box'>"
        "  <div style='font-size: 13pt; font-weight: bold;'>%1</div>"
        "  <div>%2 &nbsp;|&nbsp; GSTIN: %3</div>"
        "  <div style='font-size: 11pt; font-weight: bold; margin-top: 4px;'>%4</div>"
        "</div>"
        "<table>"
        "  <tr>"
        "    <td width='50%'><b>Note No:</b> %5</td>"
        "    <td width='50%'><b>Date:</b> %6</td>"
        "  </tr>"
        "  <tr>"
        "    <td><b>Party:</b><br><font size='3'><b>%7</b></font></td>"
        "    <td><b>Original Invoice Ref:</b> %8</td>"
        "  </tr>"
        "  <tr>"
        "    <td><b>Taxable Amount:</b> %9<br><b>GST Adjustment:</b> %10</td>"
        "    <td><b>TOTAL ADJUSTMENT:</b><br><font size='3'><b>%11</b></font></td>"
        "  </tr>"
        "  <tr>"
        "    <td colspan='2'><b>Reason for Note:</b> %12</td>"
        "  </tr>"
        "</table>"
        "</body></html>"
    )
    .arg(firm.value("firm_name").toString())
    .arg(firm.value("address").toString())
    .arg(firm.value("gstin").toString())
    .arg(nType.toUpper())
    .arg(noteNo)
    .arg(nDate)
    .arg(pName)
    .arg(origInv.isEmpty() ? "N/A" : origInv)
    .arg(formatINR(taxable))
    .arg(formatINR(gst))
    .arg(formatINR(total))
    .arg(reason.isEmpty() ? "Price/Quality adjustment" : reason);

    return html;
}

QString PrintExportController::export_debit_credit_note_pdf(const QString& noteNo, const QString& customPath) {
    QString html = renderDebitCreditNoteHtml(noteNo);
    QString clean = noteNo.trimmed().replace("/", "_").replace("\\", "_");
    return exportHtmlToPdf(html, "Note_" + clean + ".pdf", customPath, false);
}

QString PrintExportController::export_debit_credit_note_odf(const QString& noteNo, const QString& customPath) {
    QString html = renderDebitCreditNoteHtml(noteNo);
    QString clean = noteNo.trimmed().replace("/", "_").replace("\\", "_");
    return exportHtmlToOdf(html, "Note_" + clean + ".odt", customPath, false);
}

QString PrintExportController::export_debit_credit_note_excel(const QString& noteNo, const QString& customPath) {
    QString html = renderDebitCreditNoteHtml(noteNo);
    QString clean = noteNo.trimmed().replace("/", "_").replace("\\", "_");
    return exportHtmlToExcel(html, "Note_" + clean + ".xls", customPath);
}

bool PrintExportController::print_debit_credit_note(const QString& noteNo) {
    QString html = renderDebitCreditNoteHtml(noteNo);
    return printHtml(html, "Note " + noteNo, false);
}

// 2.10 Transport Dispatch / Gate Pass
QString PrintExportController::renderTransportDispatchHtml(int dispatchId) {
    auto firm = getFirmProfile();

    QVariantList rows = DatabaseManager::instance().executeQuery(
        "SELECT * FROM transport_dispatches WHERE id = ? LIMIT 1;", {dispatchId}
    );
    if (rows.isEmpty()) {
        return "<h2>Dispatch record not found</h2>";
    }

    QVariantMap r = rows.first().toMap();
    QString dDate = formatDisplayDate(r.value("dispatch_date").toString());
    QString vehNo = r.value("vehicle_no").toString();
    QString driver = r.value("driver_name").toString();
    QString party = r.value("party_name").toString();
    QString item = r.value("item_name").toString();
    long long bags = r.value("bag_count").toLongLong();
    double netWt = r.value("net_weight_qtl").toDouble();
    double grossWt = r.value("gross_weight_qtl").toDouble();
    double tareWt = r.value("tare_weight_qtl").toDouble();

    QString html = QString(
        "<!DOCTYPE html><html><head><style>"
        "body { font-family: 'Helvetica Neue', Arial, sans-serif; font-size: 8.5pt; color: #000; margin: 0; padding: 10px; }"
        "table { width: 100%; border-collapse: collapse; margin-bottom: 6px; }"
        "th, td { border: 1px solid #000; padding: 5px; font-size: 8.5pt; }"
        ".header-box { border: 1.5px solid #000; padding: 6px; margin-bottom: 6px; text-align: center; }"
        "</style></head><body>"
        "<div class='header-box'>"
        "  <div style='font-size: 13pt; font-weight: bold;'>%1</div>"
        "  <div>%2 &nbsp;|&nbsp; Phone: %3</div>"
        "  <div style='font-size: 11pt; font-weight: bold; margin-top: 4px;'>GATE PASS / TRANSPORT DISPATCH MEMO</div>"
        "</div>"
        "<table>"
        "  <tr>"
        "    <td width='50%'><b>Pass No:</b> GP-%4</td>"
        "    <td width='50%'><b>Date:</b> %5</td>"
        "  </tr>"
        "  <tr>"
        "    <td><b>Vehicle No:</b> <font size='3'><b>%6</b></font><br><b>Driver:</b> %7</td>"
        "    <td><b>Consignee / Destination:</b><br><font size='3'><b>%8</b></font></td>"
        "  </tr>"
        "  <tr>"
        "    <td><b>Commodity:</b> <b>%9</b><br><b>Bags Count:</b> %10 Bags</td>"
        "    <td><b>Gross Weight:</b> %11 Qtl<br><b>Tare Weight:</b> %12 Qtl<br><b>NET WEIGHT:</b> <b>%13 Qtl</b></td>"
        "  </tr>"
        "</table>"
        "<table style='margin-top: 25px; border: none;'>"
        "  <tr>"
        "    <td style='border: none;' width='33%'>Driver Signature</td>"
        "    <td style='border: none;' width='33%' align='center'>Weighbridge Incharge</td>"
        "    <td style='border: none;' width='34%' align='right'>Security Gate Pass Out</td>"
        "  </tr>"
        "</table>"
        "</body></html>"
    )
    .arg(firm.value("firm_name").toString())
    .arg(firm.value("address").toString())
    .arg(firm.value("phone").toString())
    .arg(dispatchId)
    .arg(dDate)
    .arg(vehNo)
    .arg(driver.isEmpty() ? "N/A" : driver)
    .arg(party)
    .arg(item)
    .arg(bags)
    .arg(QString::number(grossWt, 'f', 2))
    .arg(QString::number(tareWt, 'f', 2))
    .arg(QString::number(netWt, 'f', 2));

    return html;
}

QString PrintExportController::export_transport_dispatch_pdf(int dispatchId, const QString& customPath) {
    QString html = renderTransportDispatchHtml(dispatchId);
    return exportHtmlToPdf(html, QString("GatePass_%1.pdf").arg(dispatchId), customPath, false);
}

QString PrintExportController::export_transport_dispatch_odf(int dispatchId, const QString& customPath) {
    QString html = renderTransportDispatchHtml(dispatchId);
    return exportHtmlToOdf(html, QString("GatePass_%1.odt").arg(dispatchId), customPath, false);
}

QString PrintExportController::export_transport_dispatch_excel(int dispatchId, const QString& customPath) {
    QString html = renderTransportDispatchHtml(dispatchId);
    return exportHtmlToExcel(html, QString("GatePass_%1.xls").arg(dispatchId), customPath);
}

bool PrintExportController::print_transport_dispatch(int dispatchId) {
    QString html = renderTransportDispatchHtml(dispatchId);
    return printHtml(html, QString("Gate Pass %1").arg(dispatchId), false);
}

// ==========================================
// 3. STATEMENTS & FINANCIAL REPORTS RENDERERS
// ==========================================

// 3.1 Ledger Statement (T-Account)
QString PrintExportController::renderLedgerStatementHtml(const QString& partyName, const QString& fromDate, const QString& toDate) {
    auto firm = getFirmProfile();

    QString fDate = fromDate.trimmed();
    QString tDate = toDate.trimmed();
    if (fDate.isEmpty() && tDate.isEmpty()) {
        fDate = AccountingEngine::getActiveFromDate();
        tDate = AccountingEngine::getActiveToDate();
    }

    QString pName = partyName.trimmed();
    PartitionedLedgerData partData = FiscalYearHelper::partitionPartyTransactions(pName, fDate, tDate);

    auto pProfile = getPartyProfile(0, pName);
    QString pPlace = pProfile.value("city").toString();
    QString pGstin = pProfile.value("gstin").toString();
    QString pPhone = pProfile.value("phone").toString();

    QString drRowsHtml = "";
    double drTotal = 0.0;
    int maxRows = std::max(partData.drEntries.size(), partData.crEntries.size());

    for (const auto& e : partData.drEntries) {
        drTotal += e.amount;
        QString dt = formatDisplayDate(e.vDate);
        QString vNo = (e.id == -1) ? "" : (!e.refNo.isEmpty() ? e.refNo : e.voucherNo);
        QString desc = e.particulars;
        if (e.id == -1) desc = "<b>OPENING BALANCE</b>";

        drRowsHtml += QString(
            "<tr>"
            "<td align='center'>%1</td>"
            "<td align='center'>%2</td>"
            "<td>%3</td>"
            "<td align='right'>%4</td>"
            "</tr>"
        )
        .arg(dt)
        .arg(vNo)
        .arg(desc)
        .arg(formatINR(e.amount));
    }

    QString crRowsHtml = "";
    double crTotal = 0.0;
    for (const auto& e : partData.crEntries) {
        crTotal += e.amount;
        QString dt = formatDisplayDate(e.vDate);
        QString vNo = (e.id == -1) ? "" : (!e.refNo.isEmpty() ? e.refNo : e.voucherNo);
        QString desc = e.particulars;
        if (e.id == -1) desc = "<b>OPENING BALANCE</b>";

        crRowsHtml += QString(
            "<tr>"
            "<td align='center'>%1</td>"
            "<td align='center'>%2</td>"
            "<td>%3</td>"
            "<td align='right'>%4</td>"
            "</tr>"
        )
        .arg(dt)
        .arg(vNo)
        .arg(desc)
        .arg(formatINR(e.amount));
    }

    double netBal = std::abs(drTotal - crTotal);
    QString netBalType = (drTotal >= crTotal) ? "Dr (Receivable / Debit)" : "Cr (Payable / Credit)";

    QString html = QString(
        "<!DOCTYPE html><html><head><style>"
        "body { font-family: 'Helvetica Neue', Arial, sans-serif; font-size: 8pt; color: #000; margin: 0; padding: 8px; }"
        "table { width: 100%; border-collapse: collapse; }"
        "th, td { border: 1px solid #000; padding: 3px; font-size: 7.5pt; }"
        "th { background-color: #f1f5f9; text-align: center; font-weight: bold; }"
        ".header-box { border: 1.5px solid #000; padding: 6px; margin-bottom: 5px; text-align: center; }"
        "</style></head><body>"
        "<div class='header-box'>"
        "  <div style='font-size: 13pt; font-weight: bold;'>%1</div>"
        "  <div>%2 &nbsp;|&nbsp; Phone: %3</div>"
        "  <div><b>GSTIN:</b> %4</div>"
        "  <div style='font-size: 10pt; font-weight: bold; margin-top: 3px;'>STATEMENT OF ACCOUNT (KHATA VAHI)</div>"
        "  <div>Period: <b>%5</b> to <b>%6</b></div>"
        "</div>"
        "<table style='margin-bottom: 5px;'>"
        "  <tr>"
        "    <td width='60%'><b>Account Name:</b> <font size='3'><b>%7</b></font><br>City/Station: %8 &nbsp;|&nbsp; Phone: %9</td>"
        "    <td width='40%'><b>GSTIN:</b> %10</td>"
        "  </tr>"
        "</table>"
        "<table width='100%' style='border: none; margin-bottom: 5px;'>"
        "  <tr>"
        "    <td width='50%' valign='top' style='padding-right: 3px; border: none;'>"
        "      <table>"
        "        <tr><th colspan='4' style='background-color:#e2e8f0;'>DEBIT (JAMA / GOODS & PAYMENTS TO PARTY)</th></tr>"
        "        <tr><th width='15%'>Date</th><th width='15%'>Vch No</th><th width='46%'>Particulars</th><th width='24%'>Debit ₹</th></tr>"
        "        %11"
        "        <tr style='font-weight: bold; background-color:#f8fafc;'><td colspan='3' align='right'>Total Debit:</td><td align='right'>%13</td></tr>"
        "      </table>"
        "    </td>"
        "    <td width='50%' valign='top' style='padding-left: 3px; border: none;'>"
        "      <table>"
        "        <tr><th colspan='4' style='background-color:#e2e8f0;'>CREDIT (NAMA / RECEIPTS & PURCHASES FROM PARTY)</th></tr>"
        "        <tr><th width='15%'>Date</th><th width='15%'>Vch No</th><th width='46%'>Particulars</th><th width='24%'>Credit ₹</th></tr>"
        "        %12"
        "        <tr style='font-weight: bold; background-color:#f8fafc;'><td colspan='3' align='right'>Total Credit:</td><td align='right'>%14</td></tr>"
        "      </table>"
        "    </td>"
        "  </tr>"
        "</table>"
        "<table>"
        "  <tr>"
        "    <td width='60%'><b>NET CLOSING BALANCE: <font size='3'>%15</font></b></td>"
        "    <td width='40%' align='right'><b>BALANCE STATUS: %16</b></td>"
        "  </tr>"
        "</table>"
        "<table style='margin-top: 15px; border: none;'>"
        "  <tr>"
        "    <td style='border: none;' width='50%'>Generated on: %17 &nbsp;|&nbsp; E. & O.E.</td>"
        "    <td style='border: none;' width='50%' align='right'><b>For %1</b><br><br><br>Authorized Signatory</td>"
        "  </tr>"
        "</table>"
        "</body></html>"
    )
    .arg(firm.value("firm_name").toString())
    .arg(firm.value("address").toString())
    .arg(firm.value("phone").toString())
    .arg(firm.value("gstin").toString())
    .arg(formatDisplayDate(fDate))
    .arg(formatDisplayDate(tDate))
    .arg(pName)
    .arg(pPlace.isEmpty() ? "Local" : pPlace)
    .arg(pPhone)
    .arg(pGstin.isEmpty() ? "URP / Unregistered" : pGstin)
    .arg(drRowsHtml)
    .arg(crRowsHtml)
    .arg(formatINR(drTotal))
    .arg(formatINR(crTotal))
    .arg(formatINR(netBal))
    .arg(netBalType)
    .arg(QDateTime::currentDateTime().toString("dd-MM-yyyy hh:mm AP"));

    return html;
}

QString PrintExportController::export_ledger_statement_pdf(const QString& partyName, const QString& fromDate, const QString& toDate, const QString& customPath) {
    QString html = renderLedgerStatementHtml(partyName, fromDate, toDate);
    QString pClean = partyName.trimmed().replace(" ", "_").replace("/", "_").replace("\\", "_");
    QString fileName = "Ledger_" + pClean + ".pdf";
    return exportHtmlToPdf(html, fileName, customPath, true);
}

QString PrintExportController::export_ledger_statement_odf(const QString& partyName, const QString& fromDate, const QString& toDate, const QString& customPath) {
    QString html = renderLedgerStatementHtml(partyName, fromDate, toDate);
    QString pClean = partyName.trimmed().replace(" ", "_").replace("/", "_").replace("\\", "_");
    QString fileName = "Ledger_" + pClean + ".odt";
    return exportHtmlToOdf(html, fileName, customPath, true);
}

QString PrintExportController::export_ledger_statement_excel(const QString& partyName, const QString& fromDate, const QString& toDate, const QString& customPath) {
    QString html = renderLedgerStatementHtml(partyName, fromDate, toDate);
    QString pClean = partyName.trimmed().replace(" ", "_").replace("/", "_").replace("\\", "_");
    QString fileName = "Ledger_" + pClean + ".xls";
    return exportHtmlToExcel(html, fileName, customPath);
}

QString PrintExportController::export_ledger_csv(const QString& partyName, const QString& fromDate, const QString& toDate, const QString& customPath) {
    QString pName = partyName.trimmed();
    PartitionedLedgerData partData = FiscalYearHelper::partitionPartyTransactions(pName, fromDate, toDate);

    QStringList headers = {"Side", "Date", "Voucher No", "Voucher Type", "Particulars", "Debit", "Credit"};
    QVector<QStringList> rows;

    for (const auto& e : partData.drEntries) {
        rows.append({
            "Dr",
            formatDisplayDate(e.vDate),
            (e.id == -1) ? "OP-BAL" : (!e.refNo.isEmpty() ? e.refNo : e.voucherNo),
            (e.id == -1) ? "Opening Balance" : e.transType,
            e.particulars,
            QString::number(e.amount, 'f', 2),
            "0.00"
        });
    }

    for (const auto& e : partData.crEntries) {
        rows.append({
            "Cr",
            formatDisplayDate(e.vDate),
            (e.id == -1) ? "OP-BAL" : (!e.refNo.isEmpty() ? e.refNo : e.voucherNo),
            (e.id == -1) ? "Opening Balance" : e.transType,
            e.particulars,
            "0.00",
            QString::number(e.amount, 'f', 2)
        });
    }

    QString safeName = pName;
    safeName.replace(" ", "_").replace("/", "_");
    return exportTableToCsv(headers, rows, "Ledger_" + safeName + ".csv", customPath);
}

bool PrintExportController::print_ledger_statement(const QString& partyName, const QString& fromDate, const QString& toDate) {
    QString html = renderLedgerStatementHtml(partyName, fromDate, toDate);
    return printHtml(html, "Ledger " + partyName, true);
}

// 3.2 Day Book
QString PrintExportController::renderDayBookHtml(const QString& fromDate, const QString& toDate) {
    auto firm = getFirmProfile();

    QString fDate = fromDate.trimmed();
    QString tDate = toDate.trimmed();
    if (fDate.isEmpty()) fDate = QDate::currentDate().toString("yyyy-MM-dd");
    if (tDate.isEmpty()) tDate = fDate;

    QVariantList txRows = DatabaseManager::instance().executeQuery(
        "SELECT * FROM transactions WHERE voucher_date >= ? AND voucher_date <= ? ORDER BY voucher_date ASC, id ASC;",
        {fDate, tDate}
    );

    QString rowsHtml = "";
    int rowIdx = 1;
    double totalDr = 0.0, totalCr = 0.0;

    for (const auto& var : txRows) {
        QVariantMap t = var.toMap();
        QString vNum = t.value("voucher_no").toString();
        QString vType = t.value("voucher_type").toString();
        QString pName = t.value("party_name").toString();
        QString opp = t.value("opposing_account").toString();
        QString side = t.value("dr_cr").toString();
        double amt = t.value("amount").toDouble();

        double dr = (side == "Dr") ? amt : 0.0;
        double cr = (side == "Cr") ? amt : 0.0;
        totalDr += dr;
        totalCr += cr;

        rowsHtml += QString(
            "<tr>"
            "<td align='center'>%1</td>"
            "<td align='center'>%2</td>"
            "<td align='center'>%3</td>"
            "<td align='center'>%4</td>"
            "<td><b>%5</b></td>"
            "<td>%6</td>"
            "<td align='right'>%7</td>"
            "<td align='right'>%8</td>"
            "</tr>"
        )
        .arg(rowIdx++)
        .arg(formatDisplayDate(t.value("voucher_date").toString()))
        .arg(vNum)
        .arg(vType)
        .arg(pName)
        .arg(opp)
        .arg(dr > 0 ? formatINR(dr) : "-")
        .arg(cr > 0 ? formatINR(cr) : "-");
    }

    QString html = QString(
        "<!DOCTYPE html><html><head><style>"
        "body { font-family: 'Helvetica Neue', Arial, sans-serif; font-size: 8pt; color: #000; margin: 0; padding: 8px; }"
        "table { width: 100%; border-collapse: collapse; margin-bottom: 6px; }"
        "th, td { border: 1px solid #000; padding: 4px; font-size: 8pt; }"
        "th { background-color: #f1f5f9; text-align: center; font-weight: bold; }"
        ".header-box { border: 1.5px solid #000; padding: 6px; margin-bottom: 6px; text-align: center; }"
        "</style></head><body>"
        "<div class='header-box'>"
        "  <div style='font-size: 13pt; font-weight: bold;'>%1</div>"
        "  <div>%2 &nbsp;|&nbsp; Phone: %3</div>"
        "  <div style='font-size: 10pt; font-weight: bold; margin-top: 3px;'>DAY BOOK REPORT</div>"
        "  <div>Period: <b>%4</b> to <b>%5</b></div>"
        "</div>"
        "<table>"
        "  <tr>"
        "    <th width='5%'>S.N.</th>"
        "    <th width='10%'>Date</th>"
        "    <th width='12%'>Voucher No</th>"
        "    <th width='12%'>Type</th>"
        "    <th width='25%'>Account Name</th>"
        "    <th width='18%'>Opposing Account</th>"
        "    <th width='9%'>Debit ₹</th>"
        "    <th width='9%'>Credit ₹</th>"
        "  </tr>"
        "  %6"
        "  <tr style='font-weight: bold; background-color: #f8fafc;'>"
        "    <td colspan='6' align='right'>TOTAL:</td>"
        "    <td align='right'>%7</td>"
        "    <td align='right'>%8</td>"
        "  </tr>"
        "</table>"
        "</body></html>"
    )
    .arg(firm.value("firm_name").toString())
    .arg(firm.value("address").toString())
    .arg(firm.value("phone").toString())
    .arg(formatDisplayDate(fDate))
    .arg(formatDisplayDate(tDate))
    .arg(rowsHtml)
    .arg(formatINR(totalDr))
    .arg(formatINR(totalCr));

    return html;
}

QString PrintExportController::export_day_book_pdf(const QString& fromDate, const QString& toDate, const QString& customPath) {
    QString html = renderDayBookHtml(fromDate, toDate);
    return exportHtmlToPdf(html, "DayBook.pdf", customPath, true);
}

QString PrintExportController::export_day_book_odf(const QString& fromDate, const QString& toDate, const QString& customPath) {
    QString html = renderDayBookHtml(fromDate, toDate);
    return exportHtmlToOdf(html, "DayBook.odt", customPath, true);
}

QString PrintExportController::export_day_book_excel(const QString& fromDate, const QString& toDate, const QString& customPath) {
    QString html = renderDayBookHtml(fromDate, toDate);
    return exportHtmlToExcel(html, "DayBook.xls", customPath);
}

bool PrintExportController::print_day_book(const QString& fromDate, const QString& toDate) {
    QString html = renderDayBookHtml(fromDate, toDate);
    return printHtml(html, "Day Book", true);
}

// 3.3 Trial Balance
QString PrintExportController::renderTrialBalanceHtml(const QString& asOnDate, int mode) {
    Q_UNUSED(mode);
    auto firm = getFirmProfile();

    QString dateIso = asOnDate.trimmed().isEmpty() ? QDate::currentDate().toString("yyyy-MM-dd") : asOnDate.trimmed();

    QVariantList partyList = DatabaseManager::instance().executeQuery(
        "SELECT p.id, p.name, p.group_name, p.opening_balance, p.balance_type FROM parties p ORDER BY p.group_name, p.name ASC;"
    );

    QString rowsHtml = "";
    int rowIdx = 1;
    double totalDr = 0.0, totalCr = 0.0;

    for (const auto& var : partyList) {
        QVariantMap p = var.toMap();
        int pId = p.value("id").toInt();
        QString name = p.value("name").toString();
        QString grp = p.value("group_name").toString();
        double op = p.value("opening_balance").toDouble();
        QString opType = p.value("balance_type").toString();

        double opDr = (opType.compare("Dr", Qt::CaseInsensitive) == 0) ? op : 0.0;
        double opCr = (opType.compare("Cr", Qt::CaseInsensitive) == 0) ? op : 0.0;

        QVariant drVal = DatabaseManager::instance().executeScalar(
            "SELECT SUM(amount) FROM transactions WHERE party_id = ? AND voucher_date <= ? AND dr_cr = 'Dr';",
            {pId, dateIso}
        );
        QVariant crVal = DatabaseManager::instance().executeScalar(
            "SELECT SUM(amount) FROM transactions WHERE party_id = ? AND voucher_date <= ? AND dr_cr = 'Cr';",
            {pId, dateIso}
        );

        double curDr = drVal.isValid() ? drVal.toDouble() : 0.0;
        double curCr = crVal.isValid() ? crVal.toDouble() : 0.0;

        double netBal = (opDr + curDr) - (opCr + curCr);
        if (std::abs(netBal) < 0.01) continue;

        double closeDr = (netBal > 0) ? netBal : 0.0;
        double closeCr = (netBal < 0) ? -netBal : 0.0;

        totalDr += closeDr;
        totalCr += closeCr;

        rowsHtml += QString(
            "<tr>"
            "<td align='center'>%1</td>"
            "<td><b>%2</b></td>"
            "<td>%3</td>"
            "<td align='right'>%4</td>"
            "<td align='right'>%5</td>"
            "</tr>"
        )
        .arg(rowIdx++)
        .arg(name)
        .arg(grp)
        .arg(closeDr > 0 ? formatINR(closeDr) : "-")
        .arg(closeCr > 0 ? formatINR(closeCr) : "-");
    }

    QString html = QString(
        "<!DOCTYPE html><html><head><style>"
        "body { font-family: 'Helvetica Neue', Arial, sans-serif; font-size: 8pt; color: #000; margin: 0; padding: 8px; }"
        "table { width: 100%; border-collapse: collapse; margin-bottom: 6px; }"
        "th, td { border: 1px solid #000; padding: 4px; font-size: 8pt; }"
        "th { background-color: #f1f5f9; text-align: center; font-weight: bold; }"
        ".header-box { border: 1.5px solid #000; padding: 6px; margin-bottom: 6px; text-align: center; }"
        "</style></head><body>"
        "<div class='header-box'>"
        "  <div style='font-size: 13pt; font-weight: bold;'>%1</div>"
        "  <div>%2 &nbsp;|&nbsp; Phone: %3</div>"
        "  <div style='font-size: 10pt; font-weight: bold; margin-top: 3px;'>TRIAL BALANCE (KACHA CHITHA)</div>"
        "  <div>As on Date: <b>%4</b></div>"
        "</div>"
        "<table>"
        "  <tr>"
        "    <th width='6%'>S.N.</th>"
        "    <th width='44%'>Account Name</th>"
        "    <th width='26%'>Account Group</th>"
        "    <th width='12%'>Debit Closing ₹</th>"
        "    <th width='12%'>Credit Closing ₹</th>"
        "  </tr>"
        "  %5"
        "  <tr style='font-weight: bold; background-color: #f8fafc;'>"
        "    <td colspan='3' align='right'>TOTAL TRIAL BALANCE:</td>"
        "    <td align='right'>%6</td>"
        "    <td align='right'>%7</td>"
        "  </tr>"
        "</table>"
        "</body></html>"
    )
    .arg(firm.value("firm_name").toString())
    .arg(firm.value("address").toString())
    .arg(firm.value("phone").toString())
    .arg(formatDisplayDate(dateIso))
    .arg(rowsHtml)
    .arg(formatINR(totalDr))
    .arg(formatINR(totalCr));

    return html;
}

QString PrintExportController::export_trial_balance_pdf(const QString& asOnDate, int mode, const QString& customPath) {
    QString html = renderTrialBalanceHtml(asOnDate, mode);
    return exportHtmlToPdf(html, "TrialBalance.pdf", customPath, false);
}

QString PrintExportController::export_trial_balance_odf(const QString& asOnDate, int mode, const QString& customPath) {
    QString html = renderTrialBalanceHtml(asOnDate, mode);
    return exportHtmlToOdf(html, "TrialBalance.odt", customPath, false);
}

QString PrintExportController::export_trial_balance_excel(const QString& asOnDate, int mode, const QString& customPath) {
    QString html = renderTrialBalanceHtml(asOnDate, mode);
    return exportHtmlToExcel(html, "TrialBalance.xls", customPath);
}

bool PrintExportController::print_trial_balance(const QString& asOnDate, int mode) {
    QString html = renderTrialBalanceHtml(asOnDate, mode);
    return printHtml(html, "Trial Balance", false);
}

// 3.4 Profit & Loss Statement
QString PrintExportController::renderProfitLossHtml(const QString& fromDate, const QString& toDate) {
    auto firm = getFirmProfile();
    ProfitLossData pnl = ProfitLossCalculator::calculate(fromDate, toDate);

    QString html = QString(
        "<!DOCTYPE html><html><head><style>"
        "body { font-family: 'Helvetica Neue', Arial, sans-serif; font-size: 8pt; color: #000; margin: 0; padding: 8px; }"
        "table { width: 100%; border-collapse: collapse; margin-bottom: 6px; }"
        "th, td { border: 1px solid #000; padding: 4px; font-size: 8pt; }"
        "th { background-color: #f1f5f9; text-align: center; font-weight: bold; }"
        ".header-box { border: 1.5px solid #000; padding: 6px; margin-bottom: 6px; text-align: center; }"
        "</style></head><body>"
        "<div class='header-box'>"
        "  <div style='font-size: 13pt; font-weight: bold;'>%1</div>"
        "  <div>%2 &nbsp;|&nbsp; Phone: %3</div>"
        "  <div style='font-size: 10pt; font-weight: bold; margin-top: 3px;'>TRADING & PROFIT & LOSS ACCOUNT</div>"
        "  <div>Period: <b>%4</b> to <b>%5</b></div>"
        "</div>"
        "<table>"
        "  <tr>"
        "    <th width='50%' style='background-color:#e2e8f0;'>EXPENDITURE / DEBIT (TRADING & P&L)</th>"
        "    <th width='50%' style='background-color:#e2e8f0;'>INCOME / CREDIT (TRADING & P&L)</th>"
        "  </tr>"
        "  <tr>"
        "    <td valign='top'>"
        "      <table>"
        "        <tr><td>To Opening Stock</td><td align='right'>%6</td></tr>"
        "        <tr><td>To Paddy Procurement / Purchases</td><td align='right'>%7</td></tr>"
        "        <tr><td>To Direct & Milling Expenses</td><td align='right'>%8</td></tr>"
        "        <tr style='font-weight: bold; background-color:#f8fafc;'><td>To Gross Profit c/d</td><td align='right'>%9</td></tr>"
        "        <tr><td colspan='2' style='border-top:1.5px solid #000;'><b>Indirect Expenses:</b></td></tr>"
        "        <tr><td>To Administrative & Office Expenses</td><td align='right'>%10</td></tr>"
        "        <tr style='font-weight: bold; background-color:#f8fafc;'><td>To Net Profit (Transferred to Capital)</td><td align='right'><b>%11</b></td></tr>"
        "      </table>"
        "    </td>"
        "    <td valign='top'>"
        "      <table>"
        "        <tr><td>By Sales Revenue (Rice/Broken/Bran/Husk)</td><td align='right'>%12</td></tr>"
        "        <tr><td>By Closing Stock Valuation</td><td align='right'>%13</td></tr>"
        "        <tr><td>&nbsp;</td><td></td></tr>"
        "        <tr style='font-weight: bold; background-color:#f8fafc;'><td>By Gross Profit b/d</td><td align='right'>%9</td></tr>"
        "        <tr><td colspan='2' style='border-top:1.5px solid #000;'><b>Indirect Incomes:</b></td></tr>"
        "        <tr><td>By Interest & Other Receipts</td><td align='right'>%14</td></tr>"
        "      </table>"
        "    </td>"
        "  </tr>"
        "</table>"
        "</body></html>"
    )
    .arg(firm.value("firm_name").toString())
    .arg(firm.value("address").toString())
    .arg(firm.value("phone").toString())
    .arg(formatDisplayDate(pnl.fromDate))
    .arg(formatDisplayDate(pnl.toDate))
    .arg(formatINR(pnl.openingStockValue))
    .arg(formatINR(pnl.totalProcurement))
    .arg(formatINR(pnl.totalDirectExpenses))
    .arg(formatINR(pnl.grossProfit))
    .arg(formatINR(pnl.indirectExpenses))
    .arg(formatINR(pnl.netProfit))
    .arg(formatINR(pnl.totalSalesRevenue))
    .arg(formatINR(pnl.closingStockValue))
    .arg(formatINR(pnl.indirectIncomes));

    return html;
}

QString PrintExportController::export_profit_loss_pdf(const QString& fromDate, const QString& toDate, const QString& customPath) {
    QString html = renderProfitLossHtml(fromDate, toDate);
    return exportHtmlToPdf(html, "ProfitLoss.pdf", customPath, true);
}

QString PrintExportController::export_profit_loss_odf(const QString& fromDate, const QString& toDate, const QString& customPath) {
    QString html = renderProfitLossHtml(fromDate, toDate);
    return exportHtmlToOdf(html, "ProfitLoss.odt", customPath, true);
}

QString PrintExportController::export_profit_loss_excel(const QString& fromDate, const QString& toDate, const QString& customPath) {
    QString html = renderProfitLossHtml(fromDate, toDate);
    return exportHtmlToExcel(html, "ProfitLoss.xls", customPath);
}

bool PrintExportController::print_profit_loss(const QString& fromDate, const QString& toDate) {
    QString html = renderProfitLossHtml(fromDate, toDate);
    return printHtml(html, "Profit & Loss", true);
}

// 3.5 Balance Sheet
QString PrintExportController::renderBalanceSheetHtml(const QString& asOnDate) {
    auto firm = getFirmProfile();
    BalanceSheetData bs = BalanceSheetCalculator::calculate(asOnDate);

    QString html = QString(
        "<!DOCTYPE html><html><head><style>"
        "body { font-family: 'Helvetica Neue', Arial, sans-serif; font-size: 8pt; color: #000; margin: 0; padding: 8px; }"
        "table { width: 100%; border-collapse: collapse; margin-bottom: 6px; }"
        "th, td { border: 1px solid #000; padding: 4px; font-size: 8pt; }"
        "th { background-color: #f1f5f9; text-align: center; font-weight: bold; }"
        ".header-box { border: 1.5px solid #000; padding: 6px; margin-bottom: 6px; text-align: center; }"
        "</style></head><body>"
        "<div class='header-box'>"
        "  <div style='font-size: 13pt; font-weight: bold;'>%1</div>"
        "  <div>%2 &nbsp;|&nbsp; Phone: %3</div>"
        "  <div style='font-size: 10pt; font-weight: bold; margin-top: 3px;'>BALANCE SHEET (CHITHA)</div>"
        "  <div>As on Date: <b>%4</b></div>"
        "</div>"
        "<table>"
        "  <tr>"
        "    <th width='50%' style='background-color:#e2e8f0;'>CAPITAL & LIABILITIES (JAMA)</th>"
        "    <th width='50%' style='background-color:#e2e8f0;'>PROPERTY & ASSETS (NAVE)</th>"
        "  </tr>"
        "  <tr>"
        "    <td valign='top'>"
        "      <table>"
        "        <tr><td><b>Capital Account:</b></td><td align='right'><b>%5</b></td></tr>"
        "        <tr><td>&nbsp;&nbsp;Add: Current Net Profit</td><td align='right'>%6</td></tr>"
        "        <tr><td><b>Secured & Term Loans:</b></td><td align='right'>%7</td></tr>"
        "        <tr><td><b>Sundry Creditors & Payables:</b></td><td align='right'>%8</td></tr>"
        "        <tr><td><b>Duties & Taxes / Provisions:</b></td><td align='right'>%9</td></tr>"
        "        <tr style='font-weight: bold; background-color:#f8fafc;'><td style='border-top:1.5px solid #000;'>TOTAL LIABILITIES:</td><td align='right' style='border-top:1.5px solid #000;'><b>%10</b></td></tr>"
        "      </table>"
        "    </td>"
        "    <td valign='top'>"
        "      <table>"
        "        <tr><td><b>Fixed Assets (Plant & Machinery):</b></td><td align='right'><b>%11</b></td></tr>"
        "        <tr><td><b>Current Assets:</b></td><td></td></tr>"
        "        <tr><td>&nbsp;&nbsp;Stock-in-Hand (Closing Valuation)</td><td align='right'>%12</td></tr>"
        "        <tr><td>&nbsp;&nbsp;Sundry Debtors / Rice Buyers</td><td align='right'>%13</td></tr>"
        "        <tr><td>&nbsp;&nbsp;Bank Balances</td><td align='right'>%14</td></tr>"
        "        <tr><td>&nbsp;&nbsp;Cash-in-Hand</td><td align='right'>%15</td></tr>"
        "        <tr style='font-weight: bold; background-color:#f8fafc;'><td style='border-top:1.5px solid #000;'>TOTAL ASSETS:</td><td align='right' style='border-top:1.5px solid #000;'><b>%10</b></td></tr>"
        "      </table>"
        "    </td>"
        "  </tr>"
        "</table>"
        "</body></html>"
    )
    .arg(firm.value("firm_name").toString())
    .arg(firm.value("address").toString())
    .arg(firm.value("phone").toString())
    .arg(formatDisplayDate(bs.asOnDate))
    .arg(formatINR(bs.netProfit > 0 ? (bs.totalLiabilities - bs.netProfit) : bs.totalLiabilities))
    .arg(formatINR(bs.netProfit))
    .arg(formatINR(0.0))
    .arg(formatINR(0.0))
    .arg(formatINR(0.0))
    .arg(formatINR(bs.totalLiabilities))
    .arg(formatINR(0.0))
    .arg(formatINR(bs.closingStockValue))
    .arg(formatINR(0.0))
    .arg(formatINR(0.0))
    .arg(formatINR(0.0));

    return html;
}

QString PrintExportController::export_balance_sheet_pdf(const QString& asOnDate, const QString& customPath) {
    QString html = renderBalanceSheetHtml(asOnDate);
    return exportHtmlToPdf(html, "BalanceSheet.pdf", customPath, true);
}

QString PrintExportController::export_balance_sheet_odf(const QString& asOnDate, const QString& customPath) {
    QString html = renderBalanceSheetHtml(asOnDate);
    return exportHtmlToOdf(html, "BalanceSheet.odt", customPath, true);
}

QString PrintExportController::export_balance_sheet_excel(const QString& asOnDate, const QString& customPath) {
    QString html = renderBalanceSheetHtml(asOnDate);
    return exportHtmlToExcel(html, "BalanceSheet.xls", customPath);
}

bool PrintExportController::print_balance_sheet(const QString& asOnDate) {
    QString html = renderBalanceSheetHtml(asOnDate);
    return printHtml(html, "Balance Sheet", true);
}

// 3.6 Capital Accounts Schedule
QString PrintExportController::renderCapitalAccountsHtml(const QString& fromDate, const QString& toDate) {
    auto firm = getFirmProfile();

    QString fDate = fromDate.trimmed();
    QString tDate = toDate.trimmed();
    if (fDate.isEmpty()) fDate = AccountingEngine::getActiveFromDate();
    if (tDate.isEmpty()) tDate = AccountingEngine::getActiveToDate();

    // Fetch Capital Accounts strictly via AccountClassifier (code 1)
    QString capClause = AccountClassifier::generateHierarchySqlClause({StandardGroupCode::Capital}, "g");
    QVariantList capRows = DatabaseManager::instance().executeQuery(
        "SELECT p.id, p.name, p.opening_balance, p.balance_type FROM parties p "
        "LEFT JOIN account_groups g ON (p.group_id = g.id OR (p.group_code > 0 AND p.group_code = g.code1st) OR p.group_name = g.name) "
        "WHERE " + capClause + " ORDER BY p.name ASC;"
    );

    ProfitLossData pnl = ProfitLossCalculator::calculate(fDate, tDate);
    int partnerCount = std::max(1, (int)capRows.size());
    double profitPerPartner = pnl.netProfit / partnerCount;

    QString rowsHtml = "";
    int rowIdx = 1;
    double totOp = 0.0, totAdd = 0.0, totDraw = 0.0, totProf = 0.0, totClose = 0.0;

    for (const auto& var : capRows) {
        QVariantMap p = var.toMap();
        int pId = p.value("id").toInt();
        QString name = p.value("name").toString();
        double op = p.value("opening_balance").toDouble();

        QVariant drVal = DatabaseManager::instance().executeScalar(
            "SELECT SUM(amount) FROM transactions WHERE party_id = ? AND voucher_date >= ? AND voucher_date <= ? AND dr_cr = 'Dr';",
            {pId, fDate, tDate}
        );
        QVariant crVal = DatabaseManager::instance().executeScalar(
            "SELECT SUM(amount) FROM transactions WHERE party_id = ? AND voucher_date >= ? AND voucher_date <= ? AND dr_cr = 'Cr';",
            {pId, fDate, tDate}
        );

        double drawings = drVal.isValid() ? drVal.toDouble() : 0.0;
        double additions = crVal.isValid() ? crVal.toDouble() : 0.0;
        double close = op + additions - drawings + profitPerPartner;

        totOp += op;
        totAdd += additions;
        totDraw += drawings;
        totProf += profitPerPartner;
        totClose += close;

        rowsHtml += QString(
            "<tr>"
            "<td align='center'>%1</td>"
            "<td><b>%2</b></td>"
            "<td align='right'>%3</td>"
            "<td align='right'>%4</td>"
            "<td align='right'>%5</td>"
            "<td align='right'>%6</td>"
            "<td align='right'><b>%7</b></td>"
            "</tr>"
        )
        .arg(rowIdx++)
        .arg(name)
        .arg(formatINR(op))
        .arg(formatINR(additions))
        .arg(formatINR(drawings))
        .arg(formatINR(profitPerPartner))
        .arg(formatINR(close));
    }

    QString html = QString(
        "<!DOCTYPE html><html><head><style>"
        "body { font-family: 'Helvetica Neue', Arial, sans-serif; font-size: 8pt; color: #000; margin: 0; padding: 8px; }"
        "table { width: 100%; border-collapse: collapse; margin-bottom: 6px; }"
        "th, td { border: 1px solid #000; padding: 4px; font-size: 8pt; }"
        "th { background-color: #f1f5f9; text-align: center; font-weight: bold; }"
        ".header-box { border: 1.5px solid #000; padding: 6px; margin-bottom: 6px; text-align: center; }"
        "</style></head><body>"
        "<div class='header-box'>"
        "  <div style='font-size: 13pt; font-weight: bold;'>%1</div>"
        "  <div>%2 &nbsp;|&nbsp; Phone: %3</div>"
        "  <div style='font-size: 10pt; font-weight: bold; margin-top: 3px;'>PARTNERS CAPITAL ACCOUNTS SCHEDULE</div>"
        "  <div>Period: <b>%4</b> to <b>%5</b></div>"
        "</div>"
        "<table>"
        "  <tr>"
        "    <th width='5%'>S.N.</th>"
        "    <th width='35%'>Partner Name</th>"
        "    <th width='12%'>Opening Capital ₹</th>"
        "    <th width='12%'>Additions / Intro ₹</th>"
        "    <th width='12%'>Drawings ₹</th>"
        "    <th width='12%'>Profit Share ₹</th>"
        "    <th width='12%'>Closing Capital ₹</th>"
        "  </tr>"
        "  %6"
        "  <tr style='font-weight: bold; background-color: #f8fafc;'>"
        "    <td colspan='2' align='right'>TOTAL CAPITAL:</td>"
        "    <td align='right'>%7</td>"
        "    <td align='right'>%8</td>"
        "    <td align='right'>%9</td>"
        "    <td align='right'>%10</td>"
        "    <td align='right'>%11</td>"
        "  </tr>"
        "</table>"
        "</body></html>"
    )
    .arg(firm.value("firm_name").toString())
    .arg(firm.value("address").toString())
    .arg(firm.value("phone").toString())
    .arg(formatDisplayDate(fDate))
    .arg(formatDisplayDate(tDate))
    .arg(rowsHtml)
    .arg(formatINR(totOp))
    .arg(formatINR(totAdd))
    .arg(formatINR(totDraw))
    .arg(formatINR(totProf))
    .arg(formatINR(totClose));

    return html;
}

QString PrintExportController::export_capital_accounts_pdf(const QString& fromDate, const QString& toDate, const QString& customPath) {
    QString html = renderCapitalAccountsHtml(fromDate, toDate);
    return exportHtmlToPdf(html, "CapitalAccounts.pdf", customPath, false);
}

QString PrintExportController::export_capital_accounts_odf(const QString& fromDate, const QString& toDate, const QString& customPath) {
    QString html = renderCapitalAccountsHtml(fromDate, toDate);
    return exportHtmlToOdf(html, "CapitalAccounts.odt", customPath, false);
}

QString PrintExportController::export_capital_accounts_excel(const QString& fromDate, const QString& toDate, const QString& customPath) {
    QString html = renderCapitalAccountsHtml(fromDate, toDate);
    return exportHtmlToExcel(html, "CapitalAccounts.xls", customPath);
}

bool PrintExportController::print_capital_accounts(const QString& fromDate, const QString& toDate) {
    QString html = renderCapitalAccountsHtml(fromDate, toDate);
    return printHtml(html, "Capital Accounts", false);
}

// 3.7 Cash & Bank Flow Statement
QString PrintExportController::renderCashBankFlowHtml(int flowType, const QString& fromDate, const QString& toDate) {
    auto firm = getFirmProfile();

    QString fDate = fromDate.trimmed().isEmpty() ? AccountingEngine::getActiveFromDate() : fromDate.trimmed();
    QString tDate = toDate.trimmed().isEmpty() ? AccountingEngine::getActiveToDate() : toDate.trimmed();

    QString title = (flowType == 0) ? "CASH FLOW STATEMENT" : ((flowType == 1) ? "BANK FLOW STATEMENT" : "CASH & BANK FLOW STATEMENT");

    QVariantList txRows = DatabaseManager::instance().executeQuery(
        "SELECT party_name, opposing_account, trans_type, dr_cr, SUM(amount) as total_amt FROM transactions "
        "WHERE voucher_date >= ? AND voucher_date <= ? GROUP BY party_name, opposing_account, trans_type, dr_cr ORDER BY party_name ASC;",
        {fDate, tDate}
    );

    QString inRowsHtml = "";
    QString outRowsHtml = "";
    double totIn = 0.0, totOut = 0.0;

    for (const auto& var : txRows) {
        QVariantMap t = var.toMap();
        QString pName = t.value("party_name").toString();
        QString side = t.value("dr_cr").toString();
        double amt = t.value("total_amt").toDouble();

        if (side == "Dr") {
            totOut += amt;
            outRowsHtml += QString("<tr><td>%1</td><td align='right'>%2</td></tr>").arg(pName, formatINR(amt));
        } else {
            totIn += amt;
            inRowsHtml += QString("<tr><td>%1</td><td align='right'>%2</td></tr>").arg(pName, formatINR(amt));
        }
    }

    QString html = QString(
        "<!DOCTYPE html><html><head><style>"
        "body { font-family: 'Helvetica Neue', Arial, sans-serif; font-size: 8pt; color: #000; margin: 0; padding: 8px; }"
        "table { width: 100%; border-collapse: collapse; margin-bottom: 6px; }"
        "th, td { border: 1px solid #000; padding: 4px; font-size: 8pt; }"
        "th { background-color: #f1f5f9; text-align: center; font-weight: bold; }"
        ".header-box { border: 1.5px solid #000; padding: 6px; margin-bottom: 6px; text-align: center; }"
        "</style></head><body>"
        "<div class='header-box'>"
        "  <div style='font-size: 13pt; font-weight: bold;'>%1</div>"
        "  <div>%2 &nbsp;|&nbsp; Phone: %3</div>"
        "  <div style='font-size: 10pt; font-weight: bold; margin-top: 3px;'>%4</div>"
        "  <div>Period: <b>%5</b> to <b>%6</b></div>"
        "</div>"
        "<table>"
        "  <tr>"
        "    <th width='50%' style='background-color:#e2e8f0;'>RECEIPTS / INFLOW (CR)</th>"
        "    <th width='50%' style='background-color:#e2e8f0;'>PAYMENTS / OUTFLOW (DR)</th>"
        "  </tr>"
        "  <tr>"
        "    <td valign='top'><table>%7<tr style='font-weight:bold;'><td>Total Inflow:</td><td align='right'>%8</td></tr></table></td>"
        "    <td valign='top'><table>%9<tr style='font-weight:bold;'><td>Total Outflow:</td><td align='right'>%10</td></tr></table></td>"
        "  </tr>"
        "</table>"
        "</body></html>"
    )
    .arg(firm.value("firm_name").toString())
    .arg(firm.value("address").toString())
    .arg(firm.value("phone").toString())
    .arg(title)
    .arg(formatDisplayDate(fDate))
    .arg(formatDisplayDate(tDate))
    .arg(inRowsHtml)
    .arg(formatINR(totIn))
    .arg(outRowsHtml)
    .arg(formatINR(totOut));

    return html;
}

QString PrintExportController::export_cash_bank_flow_pdf(int flowType, const QString& fromDate, const QString& toDate, const QString& customPath) {
    QString html = renderCashBankFlowHtml(flowType, fromDate, toDate);
    return exportHtmlToPdf(html, "CashBankFlow.pdf", customPath, false);
}

QString PrintExportController::export_cash_bank_flow_odf(int flowType, const QString& fromDate, const QString& toDate, const QString& customPath) {
    QString html = renderCashBankFlowHtml(flowType, fromDate, toDate);
    return exportHtmlToOdf(html, "CashBankFlow.odt", customPath, false);
}

QString PrintExportController::export_cash_bank_flow_excel(int flowType, const QString& fromDate, const QString& toDate, const QString& customPath) {
    QString html = renderCashBankFlowHtml(flowType, fromDate, toDate);
    return exportHtmlToExcel(html, "CashBankFlow.xls", customPath);
}

bool PrintExportController::print_cash_bank_flow(int flowType, const QString& fromDate, const QString& toDate) {
    QString html = renderCashBankFlowHtml(flowType, fromDate, toDate);
    return printHtml(html, "Cash Bank Flow", false);
}

// 3.8 Interest Calculator / Aank Statement
QString PrintExportController::renderInterestCalculationHtml(const QString& partyName, const QString& fromDate, const QString& toDate, double annualRate) {
    auto firm = getFirmProfile();

    QString fDate = fromDate.trimmed().isEmpty() ? AccountingEngine::getActiveFromDate() : fromDate.trimmed();
    QString tDate = toDate.trimmed().isEmpty() ? AccountingEngine::getActiveToDate() : toDate.trimmed();

    QVariantList txRows = DatabaseManager::instance().executeQuery(
        "SELECT voucher_date, voucher_no, trans_type, dr_cr, amount FROM transactions "
        "WHERE (party_name = ? OR opposing_account = ?) AND voucher_date >= ? AND voucher_date <= ? ORDER BY voucher_date ASC;",
        {partyName.trimmed(), partyName.trimmed(), fDate, tDate}
    );

    QString rowsHtml = "";
    int rowIdx = 1;
    double runningBal = 0.0;
    double totalInterest = 0.0;
    QDate prevDate = QDate::fromString(fDate, "yyyy-MM-dd");

    for (const auto& var : txRows) {
        QVariantMap t = var.toMap();
        QDate curDate = QDate::fromString(t.value("voucher_date").toString(), "yyyy-MM-dd");
        int days = prevDate.isValid() ? prevDate.daysTo(curDate) : 0;
        double intAmt = (runningBal * (annualRate / 100.0) * days) / 365.0;
        totalInterest += intAmt;

        QString side = t.value("dr_cr").toString();
        double amt = t.value("amount").toDouble();
        if (side == "Dr") runningBal += amt;
        else runningBal -= amt;

        rowsHtml += QString(
            "<tr>"
            "<td align='center'>%1</td>"
            "<td align='center'>%2</td>"
            "<td align='center'>%3</td>"
            "<td align='right'>%4</td>"
            "<td align='center'>%5</td>"
            "<td align='right'>%6</td>"
            "<td align='right'>%7</td>"
            "</tr>"
        )
        .arg(rowIdx++)
        .arg(formatDisplayDate(t.value("voucher_date").toString()))
        .arg(t.value("voucher_no").toString())
        .arg(formatINR(amt))
        .arg(days)
        .arg(formatINR(intAmt))
        .arg(formatINR(runningBal));

        prevDate = curDate;
    }

    QString html = QString(
        "<!DOCTYPE html><html><head><style>"
        "body { font-family: 'Helvetica Neue', Arial, sans-serif; font-size: 8pt; color: #000; margin: 0; padding: 8px; }"
        "table { width: 100%; border-collapse: collapse; margin-bottom: 6px; }"
        "th, td { border: 1px solid #000; padding: 4px; font-size: 8pt; }"
        "th { background-color: #f1f5f9; text-align: center; font-weight: bold; }"
        ".header-box { border: 1.5px solid #000; padding: 6px; margin-bottom: 6px; text-align: center; }"
        "</style></head><body>"
        "<div class='header-box'>"
        "  <div style='font-size: 13pt; font-weight: bold;'>%1</div>"
        "  <div>%2 &nbsp;|&nbsp; Phone: %3</div>"
        "  <div style='font-size: 10pt; font-weight: bold; margin-top: 3px;'>INTEREST CALCULATION STATEMENT (AANK)</div>"
        "  <div>Party: <b>%4</b> &nbsp;|&nbsp; Rate: <b>%5% p.a.</b> &nbsp;|&nbsp; Period: <b>%6</b> to <b>%7</b></div>"
        "</div>"
        "<table>"
        "  <tr>"
        "    <th width='5%'>S.N.</th>"
        "    <th width='12%'>Date</th>"
        "    <th width='15%'>Voucher No</th>"
        "    <th width='18%'>Transaction ₹</th>"
        "    <th width='10%'>Days</th>"
        "    <th width='20%'>Interest ₹</th>"
        "    <th width='20%'>Balance ₹</th>"
        "  </tr>"
        "  %8"
        "  <tr style='font-weight: bold; background-color: #f8fafc;'>"
        "    <td colspan='5' align='right'>TOTAL ACCRUED INTEREST:</td>"
        "    <td align='right'><b>%9</b></td>"
        "    <td align='right'><b>%10</b></td>"
        "  </tr>"
        "</table>"
        "</body></html>"
    )
    .arg(firm.value("firm_name").toString())
    .arg(firm.value("address").toString())
    .arg(firm.value("phone").toString())
    .arg(partyName)
    .arg(QString::number(annualRate, 'f', 2))
    .arg(formatDisplayDate(fDate))
    .arg(formatDisplayDate(tDate))
    .arg(rowsHtml)
    .arg(formatINR(totalInterest))
    .arg(formatINR(runningBal));

    return html;
}

QString PrintExportController::export_interest_statement_pdf(const QString& partyName, const QString& fromDate, const QString& toDate, double annualRate, const QString& customPath) {
    QString html = renderInterestCalculationHtml(partyName, fromDate, toDate, annualRate);
    QString pClean = partyName.trimmed().replace(" ", "_").replace("/", "_");
    return exportHtmlToPdf(html, "Interest_" + pClean + ".pdf", customPath, false);
}

QString PrintExportController::export_interest_statement_odf(const QString& partyName, const QString& fromDate, const QString& toDate, double annualRate, const QString& customPath) {
    QString html = renderInterestCalculationHtml(partyName, fromDate, toDate, annualRate);
    QString pClean = partyName.trimmed().replace(" ", "_").replace("/", "_");
    return exportHtmlToOdf(html, "Interest_" + pClean + ".odt", customPath, false);
}

QString PrintExportController::export_interest_statement_excel(const QString& partyName, const QString& fromDate, const QString& toDate, double annualRate, const QString& customPath) {
    QString html = renderInterestCalculationHtml(partyName, fromDate, toDate, annualRate);
    QString pClean = partyName.trimmed().replace(" ", "_").replace("/", "_");
    return exportHtmlToExcel(html, "Interest_" + pClean + ".xls", customPath);
}

bool PrintExportController::print_interest_statement(const QString& partyName, const QString& fromDate, const QString& toDate, double annualRate) {
    QString html = renderInterestCalculationHtml(partyName, fromDate, toDate, annualRate);
    return printHtml(html, "Interest Statement " + partyName, false);
}

// 3.9 Depreciation Chart
QString PrintExportController::renderDepreciationChartHtml(const QString& asOnDate) {
    auto firm = getFirmProfile();

    QString dIso = asOnDate.trimmed().isEmpty() ? QDate::currentDate().toString("yyyy-MM-dd") : asOnDate.trimmed();

    QVariantList assets = DatabaseManager::instance().executeQuery("SELECT * FROM fixed_assets ORDER BY name ASC;");
    QString rowsHtml = "";
    int rowIdx = 1;
    double totOp = 0.0, totAdd = 0.0, totDep = 0.0, totClose = 0.0;

    for (const auto& var : assets) {
        QVariantMap a = var.toMap();
        QString name = a.value("name").toString();
        double op = a.value("opening_wdv").toDouble();
        double add = a.value("additions_cost").toDouble();
        double rate = a.value("depreciation_rate").toDouble();
        double dep = ((op + add) * rate) / 100.0;
        double close = (op + add) - dep;

        totOp += op;
        totAdd += add;
        totDep += dep;
        totClose += close;

        rowsHtml += QString(
            "<tr>"
            "<td align='center'>%1</td>"
            "<td><b>%2</b></td>"
            "<td align='right'>%3</td>"
            "<td align='right'>%4</td>"
            "<td align='center'>%5%</td>"
            "<td align='right'>%6</td>"
            "<td align='right'><b>%7</b></td>"
            "</tr>"
        )
        .arg(rowIdx++)
        .arg(name)
        .arg(formatINR(op))
        .arg(formatINR(add))
        .arg(QString::number(rate, 'f', 1))
        .arg(formatINR(dep))
        .arg(formatINR(close));
    }

    QString html = QString(
        "<!DOCTYPE html><html><head><style>"
        "body { font-family: 'Helvetica Neue', Arial, sans-serif; font-size: 8pt; color: #000; margin: 0; padding: 8px; }"
        "table { width: 100%; border-collapse: collapse; margin-bottom: 6px; }"
        "th, td { border: 1px solid #000; padding: 4px; font-size: 8pt; }"
        "th { background-color: #f1f5f9; text-align: center; font-weight: bold; }"
        ".header-box { border: 1.5px solid #000; padding: 6px; margin-bottom: 6px; text-align: center; }"
        "</style></head><body>"
        "<div class='header-box'>"
        "  <div style='font-size: 13pt; font-weight: bold;'>%1</div>"
        "  <div>%2 &nbsp;|&nbsp; Phone: %3</div>"
        "  <div style='font-size: 10pt; font-weight: bold; margin-top: 3px;'>FIXED ASSETS DEPRECIATION SCHEDULE (COMPANIES ACT / IT ACT)</div>"
        "  <div>As on Date: <b>%4</b></div>"
        "</div>"
        "<table>"
        "  <tr>"
        "    <th width='5%'>S.N.</th>"
        "    <th width='35%'>Asset Description / Block</th>"
        "    <th width='13%'>Opening WDV ₹</th>"
        "    <th width='13%'>Additions ₹</th>"
        "    <th width='8%'>Rate</th>"
        "    <th width='13%'>Depreciation ₹</th>"
        "    <th width='13%'>Closing WDV ₹</th>"
        "  </tr>"
        "  %5"
        "  <tr style='font-weight: bold; background-color: #f8fafc;'>"
        "    <td colspan='2' align='right'>TOTAL:</td>"
        "    <td align='right'>%6</td>"
        "    <td align='right'>%7</td>"
        "    <td></td>"
        "    <td align='right'>%8</td>"
        "    <td align='right'>%9</td>"
        "  </tr>"
        "</table>"
        "</body></html>"
    )
    .arg(firm.value("firm_name").toString())
    .arg(firm.value("address").toString())
    .arg(firm.value("phone").toString())
    .arg(formatDisplayDate(dIso))
    .arg(rowsHtml)
    .arg(formatINR(totOp))
    .arg(formatINR(totAdd))
    .arg(formatINR(totDep))
    .arg(formatINR(totClose));

    return html;
}

QString PrintExportController::export_depreciation_chart_pdf(const QString& asOnDate, const QString& customPath) {
    QString html = renderDepreciationChartHtml(asOnDate);
    return exportHtmlToPdf(html, "DepreciationChart.pdf", customPath, false);
}

QString PrintExportController::export_depreciation_chart_odf(const QString& asOnDate, const QString& customPath) {
    QString html = renderDepreciationChartHtml(asOnDate);
    return exportHtmlToOdf(html, "DepreciationChart.odt", customPath, false);
}

QString PrintExportController::export_depreciation_chart_excel(const QString& asOnDate, const QString& customPath) {
    QString html = renderDepreciationChartHtml(asOnDate);
    return exportHtmlToExcel(html, "DepreciationChart.xls", customPath);
}

bool PrintExportController::print_depreciation_chart(const QString& asOnDate) {
    QString html = renderDepreciationChartHtml(asOnDate);
    return printHtml(html, "Depreciation Chart", false);
}

// ==========================================
// 4. REGISTERS & INVENTORY / COMPLIANCE
// ==========================================

// 4.1 Sales Register
QString PrintExportController::renderSalesRegisterHtml(const QString& fromDate, const QString& toDate) {
    auto firm = getFirmProfile();

    QString fDate = fromDate.trimmed().isEmpty() ? AccountingEngine::getActiveFromDate() : fromDate.trimmed();
    QString tDate = toDate.trimmed().isEmpty() ? AccountingEngine::getActiveToDate() : toDate.trimmed();

    QVariantList rows = DatabaseManager::instance().executeQuery(
        "SELECT * FROM sales_invoices WHERE invoice_date >= ? AND invoice_date <= ? ORDER BY invoice_date ASC, id ASC;",
        {fDate, tDate}
    );

    QString rowsHtml = "";
    int rowIdx = 1;
    long long totalBags = 0;
    double totalWeight = 0.0, totalTaxable = 0.0, totalGst = 0.0, totalAmt = 0.0;

    for (const auto& var : rows) {
        QVariantMap r = var.toMap();
        long long bags = r.value("bag_count").toLongLong();
        double wt = r.value("weight_qtl").toDouble();
        double tax = r.value("taxable_amount").toDouble();
        double gst = r.value("gst_amount").toDouble();
        double tot = r.value("total_amount").toDouble();

        totalBags += bags;
        totalWeight += wt;
        totalTaxable += tax;
        totalGst += gst;
        totalAmt += tot;

        rowsHtml += QString(
            "<tr>"
            "<td align='center'>%1</td>"
            "<td align='center'>%2</td>"
            "<td align='center'>%3</td>"
            "<td><b>%4</b></td>"
            "<td>%5</td>"
            "<td align='center'>%6</td>"
            "<td align='right'>%7</td>"
            "<td align='right'>%8</td>"
            "<td align='right'>%9</td>"
            "<td align='right'>%10</td>"
            "<td align='right'><b>%11</b></td>"
            "</tr>"
        )
        .arg(rowIdx++)
        .arg(formatDisplayDate(r.value("invoice_date").toString()))
        .arg(r.value("invoice_no").toString())
        .arg(r.value("customer_name").toString())
        .arg(r.value("item_name").toString())
        .arg(bags)
        .arg(QString::number(wt, 'f', 2))
        .arg(QString::number(r.value("rate_per_qtl").toDouble(), 'f', 2))
        .arg(formatINR(tax))
        .arg(formatINR(gst))
        .arg(formatINR(tot));
    }

    QString html = QString(
        "<!DOCTYPE html><html><head><style>"
        "body { font-family: 'Helvetica Neue', Arial, sans-serif; font-size: 7.5pt; color: #000; margin: 0; padding: 6px; }"
        "table { width: 100%; border-collapse: collapse; margin-bottom: 5px; }"
        "th, td { border: 1px solid #000; padding: 3px; font-size: 7.5pt; }"
        "th { background-color: #f1f5f9; text-align: center; font-weight: bold; }"
        ".header-box { border: 1.5px solid #000; padding: 5px; margin-bottom: 5px; text-align: center; }"
        "</style></head><body>"
        "<div class='header-box'>"
        "  <div style='font-size: 13pt; font-weight: bold;'>%1</div>"
        "  <div>%2 &nbsp;|&nbsp; GSTIN: %3</div>"
        "  <div style='font-size: 10pt; font-weight: bold; margin-top: 3px;'>SALES REGISTER (BILL OF SUPPLY / TAX INVOICES)</div>"
        "  <div>Period: <b>%4</b> to <b>%5</b></div>"
        "</div>"
        "<table>"
        "  <tr>"
        "    <th width='4%'>S.N.</th>"
        "    <th width='8%'>Date</th>"
        "    <th width='10%'>Invoice No</th>"
        "    <th width='24%'>Customer Name</th>"
        "    <th width='16%'>Commodity</th>"
        "    <th width='6%'>Bags</th>"
        "    <th width='7%'>Weight</th>"
        "    <th width='7%'>Rate</th>"
        "    <th width='9%'>Taxable ₹</th>"
        "    <th width='9%'>GST ₹</th>"
        "    <th width='10%'>Total ₹</th>"
        "  </tr>"
        "  %6"
        "  <tr style='font-weight: bold; background-color: #f8fafc;'>"
        "    <td colspan='5' align='right'>TOTAL:</td>"
        "    <td align='center'>%7</td>"
        "    <td align='right'>%8</td>"
        "    <td></td>"
        "    <td align='right'>%9</td>"
        "    <td align='right'>%10</td>"
        "    <td align='right'>%11</td>"
        "  </tr>"
        "</table>"
        "</body></html>"
    )
    .arg(firm.value("firm_name").toString())
    .arg(firm.value("address").toString())
    .arg(firm.value("gstin").toString())
    .arg(formatDisplayDate(fDate))
    .arg(formatDisplayDate(tDate))
    .arg(rowsHtml)
    .arg(totalBags)
    .arg(QString::number(totalWeight, 'f', 2))
    .arg(formatINR(totalTaxable))
    .arg(formatINR(totalGst))
    .arg(formatINR(totalAmt));

    return html;
}

QString PrintExportController::export_sales_register_pdf(const QString& fromDate, const QString& toDate, const QString& customPath) {
    QString html = renderSalesRegisterHtml(fromDate, toDate);
    return exportHtmlToPdf(html, "SalesRegister.pdf", customPath, true);
}

QString PrintExportController::export_sales_register_odf(const QString& fromDate, const QString& toDate, const QString& customPath) {
    QString html = renderSalesRegisterHtml(fromDate, toDate);
    return exportHtmlToOdf(html, "SalesRegister.odt", customPath, true);
}

QString PrintExportController::export_sales_register_excel(const QString& fromDate, const QString& toDate, const QString& customPath) {
    QString html = renderSalesRegisterHtml(fromDate, toDate);
    return exportHtmlToExcel(html, "SalesRegister.xls", customPath);
}

QString PrintExportController::export_sales_register_csv(const QString& fromDate, const QString& toDate, const QString& customPath) {
    QString fDate = fromDate.trimmed().isEmpty() ? AccountingEngine::getActiveFromDate() : fromDate.trimmed();
    QString tDate = toDate.trimmed().isEmpty() ? AccountingEngine::getActiveToDate() : toDate.trimmed();

    QVariantList rows = DatabaseManager::instance().executeQuery(
        "SELECT * FROM sales_invoices WHERE invoice_date >= ? AND invoice_date <= ? ORDER BY invoice_date ASC, id ASC;",
        {fDate, tDate}
    );

    QStringList headers = {"Invoice No", "Date", "Customer Name", "Commodity", "Bags", "Weight Qtl", "Rate", "Taxable", "GST", "Total Amount", "Vehicle"};
    QVector<QStringList> csvRows;

    for (const auto& var : rows) {
        QVariantMap r = var.toMap();
        csvRows.append({
            r.value("invoice_no").toString(),
            formatDisplayDate(r.value("invoice_date").toString()),
            r.value("customer_name").toString(),
            r.value("item_name").toString(),
            QString::number(r.value("bag_count").toLongLong()),
            QString::number(r.value("weight_qtl").toDouble(), 'f', 2),
            QString::number(r.value("rate_per_qtl").toDouble(), 'f', 2),
            QString::number(r.value("taxable_amount").toDouble(), 'f', 2),
            QString::number(r.value("gst_amount").toDouble(), 'f', 2),
            QString::number(r.value("total_amount").toDouble(), 'f', 2),
            r.value("vehicle_no").toString()
        });
    }

    return exportTableToCsv(headers, csvRows, "SalesRegister.csv", customPath);
}

bool PrintExportController::print_sales_register(const QString& fromDate, const QString& toDate) {
    QString html = renderSalesRegisterHtml(fromDate, toDate);
    return printHtml(html, "Sales Register", true);
}

// 4.2 Purchase Register
QString PrintExportController::renderPurchaseRegisterHtml(const QString& fromDate, const QString& toDate) {
    auto firm = getFirmProfile();

    QString fDate = fromDate.trimmed().isEmpty() ? AccountingEngine::getActiveFromDate() : fromDate.trimmed();
    QString tDate = toDate.trimmed().isEmpty() ? AccountingEngine::getActiveToDate() : toDate.trimmed();

    QVariantList rows = DatabaseManager::instance().executeQuery(
        "SELECT * FROM purchase_invoices WHERE invoice_date >= ? AND invoice_date <= ? ORDER BY invoice_date ASC, id ASC;",
        {fDate, tDate}
    );

    QString rowsHtml = "";
    int rowIdx = 1;
    long long totalBags = 0;
    double totalWeight = 0.0, totalTaxable = 0.0, totalAmt = 0.0;

    for (const auto& var : rows) {
        QVariantMap r = var.toMap();
        long long bags = r.value("bag_count").toLongLong();
        double wt = r.value("weight_qtl").toDouble();
        double tax = r.value("taxable_amount").toDouble();
        double tot = r.value("total_amount").toDouble();

        totalBags += bags;
        totalWeight += wt;
        totalTaxable += tax;
        totalAmt += tot;

        rowsHtml += QString(
            "<tr>"
            "<td align='center'>%1</td>"
            "<td align='center'>%2</td>"
            "<td align='center'>%3</td>"
            "<td align='center'>%4</td>"
            "<td><b>%5</b></td>"
            "<td>%6</td>"
            "<td align='center'>%7</td>"
            "<td align='right'>%8</td>"
            "<td align='right'>%9</td>"
            "<td align='right'>%10</td>"
            "<td align='right'><b>%11</b></td>"
            "</tr>"
        )
        .arg(rowIdx++)
        .arg(formatDisplayDate(r.value("invoice_date").toString()))
        .arg(r.value("voucher_no").toString())
        .arg(r.value("invoice_no").toString())
        .arg(r.value("supplier_name").toString())
        .arg(r.value("item_name").toString())
        .arg(bags)
        .arg(QString::number(wt, 'f', 2))
        .arg(QString::number(r.value("rate_per_qtl").toDouble(), 'f', 2))
        .arg(formatINR(tax))
        .arg(formatINR(tot));
    }

    QString html = QString(
        "<!DOCTYPE html><html><head><style>"
        "body { font-family: 'Helvetica Neue', Arial, sans-serif; font-size: 7.5pt; color: #000; margin: 0; padding: 6px; }"
        "table { width: 100%; border-collapse: collapse; margin-bottom: 5px; }"
        "th, td { border: 1px solid #000; padding: 3px; font-size: 7.5pt; }"
        "th { background-color: #f1f5f9; text-align: center; font-weight: bold; }"
        ".header-box { border: 1.5px solid #000; padding: 5px; margin-bottom: 5px; text-align: center; }"
        "</style></head><body>"
        "<div class='header-box'>"
        "  <div style='font-size: 13pt; font-weight: bold;'>%1</div>"
        "  <div>%2 &nbsp;|&nbsp; GSTIN: %3</div>"
        "  <div style='font-size: 10pt; font-weight: bold; margin-top: 3px;'>PURCHASE REGISTER (INWARD SUPPLIES)</div>"
        "  <div>Period: <b>%4</b> to <b>%5</b></div>"
        "</div>"
        "<table>"
        "  <tr>"
        "    <th width='4%'>S.N.</th>"
        "    <th width='8%'>Date</th>"
        "    <th width='9%'>Vch No</th>"
        "    <th width='9%'>Bill No</th>"
        "    <th width='24%'>Supplier Name</th>"
        "    <th width='16%'>Commodity</th>"
        "    <th width='6%'>Bags</th>"
        "    <th width='7%'>Weight</th>"
        "    <th width='7%'>Rate</th>"
        "    <th width='10%'>Taxable ₹</th>"
        "    <th width='10%'>Total Bill ₹</th>"
        "  </tr>"
        "  %6"
        "  <tr style='font-weight: bold; background-color: #f8fafc;'>"
        "    <td colspan='6' align='right'>TOTAL:</td>"
        "    <td align='center'>%7</td>"
        "    <td align='right'>%8</td>"
        "    <td></td>"
        "    <td align='right'>%9</td>"
        "    <td align='right'>%10</td>"
        "  </tr>"
        "</table>"
        "</body></html>"
    )
    .arg(firm.value("firm_name").toString())
    .arg(firm.value("address").toString())
    .arg(firm.value("gstin").toString())
    .arg(formatDisplayDate(fDate))
    .arg(formatDisplayDate(tDate))
    .arg(rowsHtml)
    .arg(totalBags)
    .arg(QString::number(totalWeight, 'f', 2))
    .arg(formatINR(totalTaxable))
    .arg(formatINR(totalAmt));

    return html;
}

QString PrintExportController::export_purchase_register_pdf(const QString& fromDate, const QString& toDate, const QString& customPath) {
    QString html = renderPurchaseRegisterHtml(fromDate, toDate);
    return exportHtmlToPdf(html, "PurchaseRegister.pdf", customPath, true);
}

QString PrintExportController::export_purchase_register_odf(const QString& fromDate, const QString& toDate, const QString& customPath) {
    QString html = renderPurchaseRegisterHtml(fromDate, toDate);
    return exportHtmlToOdf(html, "PurchaseRegister.odt", customPath, true);
}

QString PrintExportController::export_purchase_register_excel(const QString& fromDate, const QString& toDate, const QString& customPath) {
    QString html = renderPurchaseRegisterHtml(fromDate, toDate);
    return exportHtmlToExcel(html, "PurchaseRegister.xls", customPath);
}

QString PrintExportController::export_purchase_register_csv(const QString& fromDate, const QString& toDate, const QString& customPath) {
    QString fDate = fromDate.trimmed().isEmpty() ? AccountingEngine::getActiveFromDate() : fromDate.trimmed();
    QString tDate = toDate.trimmed().isEmpty() ? AccountingEngine::getActiveToDate() : toDate.trimmed();

    QVariantList rows = DatabaseManager::instance().executeQuery(
        "SELECT * FROM purchase_invoices WHERE invoice_date >= ? AND invoice_date <= ? ORDER BY invoice_date ASC, id ASC;",
        {fDate, tDate}
    );

    QStringList headers = {"Voucher No", "Bill No", "Date", "Supplier Name", "Commodity", "Bags", "Weight Qtl", "Rate", "Taxable", "Total Bill", "Vehicle"};
    QVector<QStringList> csvRows;

    for (const auto& var : rows) {
        QVariantMap r = var.toMap();
        csvRows.append({
            r.value("voucher_no").toString(),
            r.value("invoice_no").toString(),
            formatDisplayDate(r.value("invoice_date").toString()),
            r.value("supplier_name").toString(),
            r.value("item_name").toString(),
            QString::number(r.value("bag_count").toLongLong()),
            QString::number(r.value("weight_qtl").toDouble(), 'f', 2),
            QString::number(r.value("rate_per_qtl").toDouble(), 'f', 2),
            QString::number(r.value("taxable_amount").toDouble(), 'f', 2),
            QString::number(r.value("total_amount").toDouble(), 'f', 2),
            r.value("vehicle_no").toString()
        });
    }

    return exportTableToCsv(headers, csvRows, "PurchaseRegister.csv", customPath);
}

bool PrintExportController::print_purchase_register(const QString& fromDate, const QString& toDate) {
    QString html = renderPurchaseRegisterHtml(fromDate, toDate);
    return printHtml(html, "Purchase Register", true);
}

// 4.3 Stock Register
QString PrintExportController::renderStockRegisterHtml(const QString& fromDate, const QString& toDate) {
    auto firm = getFirmProfile();

    QString fDate = fromDate.trimmed().isEmpty() ? AccountingEngine::getActiveFromDate() : fromDate.trimmed();
    QString tDate = toDate.trimmed().isEmpty() ? AccountingEngine::getActiveToDate() : toDate.trimmed();

    QVariantList items = DatabaseManager::instance().executeQuery("SELECT * FROM stock_items ORDER BY name COLLATE NOCASE ASC;");
    QString rowsHtml = "";
    int rowIdx = 1;
    double totalCloseQty = 0.0, totalCloseVal = 0.0;

    for (const auto& itmVar : items) {
        QVariantMap item = itmVar.toMap();
        QString name = item.value("name").toString();
        int itemId = item.value("id").toInt();
        QString code = item.value("code").toString();
        QString unit = item.value("unit", "Qtl").toString();
        double opQty = item.value("opening_qty").toDouble();

        QVariant inVal = DatabaseManager::instance().executeScalar(
            "SELECT SUM(weight_qtl) FROM stock_transactions WHERE (item_id = ? OR item_name = ?) AND trans_type IN ('Purc', 'SlRn', 'Inward', 'P', 'M');",
            {itemId, name}
        );
        QVariant outVal = DatabaseManager::instance().executeScalar(
            "SELECT SUM(weight_qtl) FROM stock_transactions WHERE (item_id = ? OR item_name = ?) AND trans_type IN ('Sale', 'PrRn', 'Outward', 'S');",
            {itemId, name}
        );

        double inQty = inVal.isValid() ? inVal.toDouble() : 0.0;
        double outQty = outVal.isValid() ? outVal.toDouble() : 0.0;
        double closeQty = opQty + inQty - outQty;
        double rate = item.value("rate", item.value("standard_cost", 0.0)).toDouble();
        double closeVal = closeQty * rate;

        totalCloseQty += closeQty;
        totalCloseVal += closeVal;

        rowsHtml += QString(
            "<tr>"
            "<td align='center'>%1</td>"
            "<td><b>%2</b></td>"
            "<td align='center'>%3</td>"
            "<td align='center'>%4</td>"
            "<td align='right'>%5</td>"
            "<td align='right'>%6</td>"
            "<td align='right'>%7</td>"
            "<td align='right'><b>%8</b></td>"
            "<td align='right'>%9</td>"
            "<td align='right'><b>%10</b></td>"
            "</tr>"
        )
        .arg(rowIdx++)
        .arg(name)
        .arg(code)
        .arg(unit)
        .arg(QString::number(opQty, 'f', 2))
        .arg(QString::number(inQty, 'f', 2))
        .arg(QString::number(outQty, 'f', 2))
        .arg(QString::number(closeQty, 'f', 2))
        .arg(QString::number(rate, 'f', 2))
        .arg(formatINR(closeVal));
    }

    QString html = QString(
        "<!DOCTYPE html><html><head><style>"
        "body { font-family: 'Helvetica Neue', Arial, sans-serif; font-size: 7.5pt; color: #000; margin: 0; padding: 6px; }"
        "table { width: 100%; border-collapse: collapse; margin-bottom: 5px; }"
        "th, td { border: 1px solid #000; padding: 3px; font-size: 7.5pt; }"
        "th { background-color: #f1f5f9; text-align: center; font-weight: bold; }"
        ".header-box { border: 1.5px solid #000; padding: 5px; margin-bottom: 5px; text-align: center; }"
        "</style></head><body>"
        "<div class='header-box'>"
        "  <div style='font-size: 13pt; font-weight: bold;'>%1</div>"
        "  <div>%2 &nbsp;|&nbsp; GSTIN: %3</div>"
        "  <div style='font-size: 10pt; font-weight: bold; margin-top: 3px;'>STOCK REGISTER & INVENTORY VALUATION</div>"
        "  <div>Period: <b>%4</b> to <b>%5</b></div>"
        "</div>"
        "<table>"
        "  <tr>"
        "    <th width='4%'>S.N.</th>"
        "    <th width='26%'>Item Description</th>"
        "    <th width='8%'>Code</th>"
        "    <th width='6%'>Unit</th>"
        "    <th width='9%'>Opening Qtl</th>"
        "    <th width='9%'>Inward Qtl</th>"
        "    <th width='9%'>Outward Qtl</th>"
        "    <th width='10%'>Closing Qtl</th>"
        "    <th width='9%'>Rate ₹</th>"
        "    <th width='10%'>Closing Value ₹</th>"
        "  </tr>"
        "  %6"
        "  <tr style='font-weight: bold; background-color: #f8fafc;'>"
        "    <td colspan='7' align='right'>TOTAL STOCK VALUATION:</td>"
        "    <td align='right'>%7</td>"
        "    <td></td>"
        "    <td align='right'><b>%8</b></td>"
        "  </tr>"
        "</table>"
        "</body></html>"
    )
    .arg(firm.value("firm_name").toString())
    .arg(firm.value("address").toString())
    .arg(firm.value("gstin").toString())
    .arg(formatDisplayDate(fDate))
    .arg(formatDisplayDate(tDate))
    .arg(rowsHtml)
    .arg(QString::number(totalCloseQty, 'f', 2))
    .arg(formatINR(totalCloseVal));

    return html;
}

QString PrintExportController::export_stock_register_pdf(const QString& fromDate, const QString& toDate, const QString& customPath) {
    QString html = renderStockRegisterHtml(fromDate, toDate);
    return exportHtmlToPdf(html, "StockRegister.pdf", customPath, true);
}

QString PrintExportController::export_stock_register_odf(const QString& fromDate, const QString& toDate, const QString& customPath) {
    QString html = renderStockRegisterHtml(fromDate, toDate);
    return exportHtmlToOdf(html, "StockRegister.odt", customPath, true);
}

QString PrintExportController::export_stock_register_excel(const QString& fromDate, const QString& toDate, const QString& customPath) {
    QString html = renderStockRegisterHtml(fromDate, toDate);
    return exportHtmlToExcel(html, "StockRegister.xls", customPath);
}

QString PrintExportController::export_stock_csv(const QString& fromDate, const QString& toDate, const QString& customPath) {
    Q_UNUSED(fromDate);
    Q_UNUSED(toDate);
    QVariantList items = DatabaseManager::instance().executeQuery("SELECT * FROM stock_items ORDER BY name COLLATE NOCASE ASC;");

    QStringList headers = {"Item Name", "Code", "Type", "Unit", "Opening Qtl", "Inward Qtl", "Outward Qtl", "Closing Qtl", "Rate", "Closing Value"};
    QVector<QStringList> csvRows;

    for (const auto& var : items) {
        QVariantMap it = var.toMap();
        QString name = it.value("name").toString();
        int itemId = it.value("id").toInt();
        QString code = it.value("code").toString();
        QString type = it.value("item_type").toString();
        QString unit = it.value("unit", "Qtl").toString();
        double op = it.value("opening_qty").toDouble();

        QVariant inVal = DatabaseManager::instance().executeScalar(
            "SELECT SUM(weight_qtl) FROM stock_transactions WHERE (item_id = ? OR item_name = ?) AND trans_type IN ('Purc', 'SlRn', 'Inward', 'P', 'M');",
            {itemId, name}
        );
        QVariant outVal = DatabaseManager::instance().executeScalar(
            "SELECT SUM(weight_qtl) FROM stock_transactions WHERE (item_id = ? OR item_name = ?) AND trans_type IN ('Sale', 'PrRn', 'Outward', 'S');",
            {itemId, name}
        );

        double inQty = inVal.isValid() ? inVal.toDouble() : 0.0;
        double outQty = outVal.isValid() ? outVal.toDouble() : 0.0;
        double close = op + inQty - outQty;
        double rate = it.value("rate", it.value("standard_cost", 0.0)).toDouble();
        double val = close * rate;

        csvRows.append({
            name, code, type, unit,
            QString::number(op, 'f', 2),
            QString::number(inQty, 'f', 2),
            QString::number(outQty, 'f', 2),
            QString::number(close, 'f', 2),
            QString::number(rate, 'f', 2),
            QString::number(val, 'f', 2)
        });
    }

    return exportTableToCsv(headers, csvRows, "StockRegister.csv", customPath);
}

bool PrintExportController::print_stock_register(const QString& fromDate, const QString& toDate) {
    QString html = renderStockRegisterHtml(fromDate, toDate);
    return printHtml(html, "Stock Register", true);
}

// 4.4 Item Movement Analysis
QString PrintExportController::renderItemMovementHtml(int itemId, const QString& fromDate, const QString& toDate) {
    auto firm = getFirmProfile();

    QVariantList itmRows = DatabaseManager::instance().executeQuery("SELECT * FROM stock_items WHERE id = ? LIMIT 1;", {itemId});
    QString itmName = itmRows.isEmpty() ? "Commodity Item" : itmRows.first().toMap().value("name").toString();

    QString fDate = fromDate.trimmed().isEmpty() ? AccountingEngine::getActiveFromDate() : fromDate.trimmed();
    QString tDate = toDate.trimmed().isEmpty() ? AccountingEngine::getActiveToDate() : toDate.trimmed();

    QVariantList txRows = DatabaseManager::instance().executeQuery(
        "SELECT * FROM stock_transactions WHERE (item_id = ? OR item_name = ?) AND trans_date >= ? AND trans_date <= ? ORDER BY trans_date ASC, id ASC;",
        {itemId, itmName, fDate, tDate}
    );

    QString rowsHtml = "";
    int rowIdx = 1;
    double runningQty = 0.0;

    for (const auto& var : txRows) {
        QVariantMap t = var.toMap();
        QString type = t.value("trans_type").toString();
        double wt = t.value("weight_qtl").toDouble();
        bool isInward = (type == "Purc" || type == "Inward" || type == "P" || type == "M");
        if (isInward) runningQty += wt;
        else runningQty -= wt;

        rowsHtml += QString(
            "<tr>"
            "<td align='center'>%1</td>"
            "<td align='center'>%2</td>"
            "<td align='center'>%3</td>"
            "<td>%4</td>"
            "<td align='right'>%5</td>"
            "<td align='right'>%6</td>"
            "<td align='right'><b>%7</b></td>"
            "</tr>"
        )
        .arg(rowIdx++)
        .arg(formatDisplayDate(t.value("trans_date").toString()))
        .arg(t.value("voucher_no").toString())
        .arg(t.value("party_name").toString())
        .arg(isInward ? QString::number(wt, 'f', 2) : "-")
        .arg(!isInward ? QString::number(wt, 'f', 2) : "-")
        .arg(QString::number(runningQty, 'f', 2));
    }

    QString html = QString(
        "<!DOCTYPE html><html><head><style>"
        "body { font-family: 'Helvetica Neue', Arial, sans-serif; font-size: 8pt; color: #000; margin: 0; padding: 8px; }"
        "table { width: 100%; border-collapse: collapse; margin-bottom: 6px; }"
        "th, td { border: 1px solid #000; padding: 4px; font-size: 8pt; }"
        "th { background-color: #f1f5f9; text-align: center; font-weight: bold; }"
        ".header-box { border: 1.5px solid #000; padding: 6px; margin-bottom: 6px; text-align: center; }"
        "</style></head><body>"
        "<div class='header-box'>"
        "  <div style='font-size: 13pt; font-weight: bold;'>%1</div>"
        "  <div>%2 &nbsp;|&nbsp; Phone: %3</div>"
        "  <div style='font-size: 10pt; font-weight: bold; margin-top: 3px;'>ITEM MOVEMENT TIMELINE & AUDIT</div>"
        "  <div>Item: <b>%4</b> &nbsp;|&nbsp; Period: <b>%5</b> to <b>%6</b></div>"
        "</div>"
        "<table>"
        "  <tr>"
        "    <th width='5%'>S.N.</th>"
        "    <th width='12%'>Date</th>"
        "    <th width='15%'>Voucher No</th>"
        "    <th width='38%'>Party / Particulars</th>"
        "    <th width='10%'>Inward Qtl</th>"
        "    <th width='10%'>Outward Qtl</th>"
        "    <th width='10%'>Balance Qtl</th>"
        "  </tr>"
        "  %7"
        "</table>"
        "</body></html>"
    )
    .arg(firm.value("firm_name").toString())
    .arg(firm.value("address").toString())
    .arg(firm.value("phone").toString())
    .arg(itmName)
    .arg(formatDisplayDate(fDate))
    .arg(formatDisplayDate(tDate))
    .arg(rowsHtml);

    return html;
}

QString PrintExportController::export_item_movement_pdf(int itemId, const QString& fromDate, const QString& toDate, const QString& customPath) {
    QString html = renderItemMovementHtml(itemId, fromDate, toDate);
    return exportHtmlToPdf(html, QString("ItemMovement_%1.pdf").arg(itemId), customPath, false);
}

QString PrintExportController::export_item_movement_odf(int itemId, const QString& fromDate, const QString& toDate, const QString& customPath) {
    QString html = renderItemMovementHtml(itemId, fromDate, toDate);
    return exportHtmlToOdf(html, QString("ItemMovement_%1.odt").arg(itemId), customPath, false);
}

QString PrintExportController::export_item_movement_excel(int itemId, const QString& fromDate, const QString& toDate, const QString& customPath) {
    QString html = renderItemMovementHtml(itemId, fromDate, toDate);
    return exportHtmlToExcel(html, QString("ItemMovement_%1.xls").arg(itemId), customPath);
}

bool PrintExportController::print_item_movement(int itemId, const QString& fromDate, const QString& toDate) {
    QString html = renderItemMovementHtml(itemId, fromDate, toDate);
    return printHtml(html, QString("Item Movement %1").arg(itemId), false);
}

// 4.5 Physical / Closing Stock Valuation
QString PrintExportController::renderClosingStockHtml(const QString& asOnDate) {
    return renderStockRegisterHtml("", asOnDate);
}

QString PrintExportController::export_closing_stock_pdf(const QString& asOnDate, const QString& customPath) {
    QString html = renderClosingStockHtml(asOnDate);
    return exportHtmlToPdf(html, "ClosingStockValuation.pdf", customPath, true);
}

QString PrintExportController::export_closing_stock_odf(const QString& asOnDate, const QString& customPath) {
    QString html = renderClosingStockHtml(asOnDate);
    return exportHtmlToOdf(html, "ClosingStockValuation.odt", customPath, true);
}

QString PrintExportController::export_closing_stock_excel(const QString& asOnDate, const QString& customPath) {
    QString html = renderClosingStockHtml(asOnDate);
    return exportHtmlToExcel(html, "ClosingStockValuation.xls", customPath);
}

bool PrintExportController::print_closing_stock(const QString& asOnDate) {
    QString html = renderClosingStockHtml(asOnDate);
    return printHtml(html, "Closing Stock Valuation", true);
}

// 4.6 Milling Production Statement
QString PrintExportController::renderMillingStatementHtml(const QString& fromDate, const QString& toDate) {
    auto firm = getFirmProfile();

    QString fDate = fromDate.trimmed().isEmpty() ? AccountingEngine::getActiveFromDate() : fromDate.trimmed();
    QString tDate = toDate.trimmed().isEmpty() ? AccountingEngine::getActiveToDate() : toDate.trimmed();

    QVariantList rows = DatabaseManager::instance().executeQuery(
        "SELECT * FROM milling_batches WHERE batch_date >= ? AND batch_date <= ? ORDER BY batch_date ASC, id ASC;",
        {fDate, tDate}
    );

    QString rowsHtml = "";
    int rowIdx = 1;
    double totPaddy = 0.0, totRice = 0.0, totBroken = 0.0, totBran = 0.0, totHusk = 0.0;

    for (const auto& var : rows) {
        QVariantMap r = var.toMap();
        double pIn = r.value("paddy_input_qtl").toDouble();
        double rOut = r.value("head_rice_qtl").toDouble();
        double bOut = r.value("broken_rice_qtl").toDouble();
        double brOut = r.value("rice_bran_qtl").toDouble();
        double hOut = r.value("husk_qtl").toDouble();

        totPaddy += pIn;
        totRice += rOut;
        totBroken += bOut;
        totBran += brOut;
        totHusk += hOut;

        double recovery = (pIn > 0) ? ((rOut + bOut) / pIn) * 100.0 : 0.0;

        rowsHtml += QString(
            "<tr>"
            "<td align='center'>%1</td>"
            "<td align='center'>%2</td>"
            "<td align='center'>%3</td>"
            "<td>%4</td>"
            "<td align='right'>%5</td>"
            "<td align='right'>%6</td>"
            "<td align='right'>%7</td>"
            "<td align='right'>%8</td>"
            "<td align='right'>%9</td>"
            "<td align='center'><b>%10%</b></td>"
            "</tr>"
        )
        .arg(rowIdx++)
        .arg(formatDisplayDate(r.value("batch_date").toString()))
        .arg(r.value("batch_no").toString())
        .arg(r.value("paddy_variety").toString())
        .arg(QString::number(pIn, 'f', 2))
        .arg(QString::number(rOut, 'f', 2))
        .arg(QString::number(bOut, 'f', 2))
        .arg(QString::number(brOut, 'f', 2))
        .arg(QString::number(hOut, 'f', 2))
        .arg(QString::number(recovery, 'f', 2));
    }

    QString html = QString(
        "<!DOCTYPE html><html><head><style>"
        "body { font-family: 'Helvetica Neue', Arial, sans-serif; font-size: 7.5pt; color: #000; margin: 0; padding: 6px; }"
        "table { width: 100%; border-collapse: collapse; margin-bottom: 5px; }"
        "th, td { border: 1px solid #000; padding: 3px; font-size: 7.5pt; }"
        "th { background-color: #f1f5f9; text-align: center; font-weight: bold; }"
        ".header-box { border: 1.5px solid #000; padding: 5px; margin-bottom: 5px; text-align: center; }"
        "</style></head><body>"
        "<div class='header-box'>"
        "  <div style='font-size: 13pt; font-weight: bold;'>%1</div>"
        "  <div>%2 &nbsp;|&nbsp; Phone: %3</div>"
        "  <div style='font-size: 10pt; font-weight: bold; margin-top: 3px;'>RICE MILLING PRODUCTION & YIELD RECOVERY STATEMENT</div>"
        "  <div>Period: <b>%4</b> to <b>%5</b></div>"
        "</div>"
        "<table>"
        "  <tr>"
        "    <th width='4%'>S.N.</th>"
        "    <th width='9%'>Date</th>"
        "    <th width='10%'>Batch No</th>"
        "    <th width='15%'>Paddy Variety</th>"
        "    <th width='11%'>Paddy Input Qtl</th>"
        "    <th width='11%'>Head Rice Qtl</th>"
        "    <th width='10%'>Broken Qtl</th>"
        "    <th width='10%'>Bran Qtl</th>"
        "    <th width='10%'>Husk Qtl</th>"
        "    <th width='10%'>Recovery %</th>"
        "  </tr>"
        "  %6"
        "  <tr style='font-weight: bold; background-color: #f8fafc;'>"
        "    <td colspan='4' align='right'>TOTAL PRODUCTION:</td>"
        "    <td align='right'>%7</td>"
        "    <td align='right'>%8</td>"
        "    <td align='right'>%9</td>"
        "    <td align='right'>%10</td>"
        "    <td align='right'>%11</td>"
        "    <td></td>"
        "  </tr>"
        "</table>"
        "</body></html>"
    )
    .arg(firm.value("firm_name").toString())
    .arg(firm.value("address").toString())
    .arg(firm.value("phone").toString())
    .arg(formatDisplayDate(fDate))
    .arg(formatDisplayDate(tDate))
    .arg(rowsHtml)
    .arg(QString::number(totPaddy, 'f', 2))
    .arg(QString::number(totRice, 'f', 2))
    .arg(QString::number(totBroken, 'f', 2))
    .arg(QString::number(totBran, 'f', 2))
    .arg(QString::number(totHusk, 'f', 2));

    return html;
}

QString PrintExportController::export_milling_statement_pdf(const QString& fromDate, const QString& toDate, const QString& customPath) {
    QString html = renderMillingStatementHtml(fromDate, toDate);
    return exportHtmlToPdf(html, "MillingStatement.pdf", customPath, true);
}

QString PrintExportController::export_milling_statement_odf(const QString& fromDate, const QString& toDate, const QString& customPath) {
    QString html = renderMillingStatementHtml(fromDate, toDate);
    return exportHtmlToOdf(html, "MillingStatement.odt", customPath, true);
}

QString PrintExportController::export_milling_statement_excel(const QString& fromDate, const QString& toDate, const QString& customPath) {
    QString html = renderMillingStatementHtml(fromDate, toDate);
    return exportHtmlToExcel(html, "MillingStatement.xls", customPath);
}

bool PrintExportController::print_milling_statement(const QString& fromDate, const QString& toDate) {
    QString html = renderMillingStatementHtml(fromDate, toDate);
    return printHtml(html, "Milling Statement", true);
}

// 4.7 Mandi Reports (Form M, Form 9, RDF, Market Fee)
QString PrintExportController::renderMandiReportHtml(const QString& reportType, const QString& fromDate, const QString& toDate) {
    auto firm = getFirmProfile();

    QString fDate = fromDate.trimmed().isEmpty() ? AccountingEngine::getActiveFromDate() : fromDate.trimmed();
    QString tDate = toDate.trimmed().isEmpty() ? AccountingEngine::getActiveToDate() : toDate.trimmed();

    QVariantList jRows = DatabaseManager::instance().executeQuery(
        "SELECT * FROM jform_vouchers WHERE voucher_date >= ? AND voucher_date <= ? ORDER BY voucher_date ASC;",
        {fDate, tDate}
    );

    QString rowsHtml = "";
    int rowIdx = 1;
    long long totalBags = 0;
    double totalWeight = 0.0, totalAmt = 0.0, totalMfee = 0.0, totalRdf = 0.0;

    for (const auto& var : jRows) {
        QVariantMap j = var.toMap();
        long long bags = j.value("bag_count").toLongLong();
        double wt = j.value("weight_qtl").toDouble();
        double amt = j.value("total_amount").toDouble();
        double mfee = amt * 0.02; // 2% Market Fee
        double rdf = amt * 0.02;  // 2% RDF

        totalBags += bags;
        totalWeight += wt;
        totalAmt += amt;
        totalMfee += mfee;
        totalRdf += rdf;

        rowsHtml += QString(
            "<tr>"
            "<td align='center'>%1</td>"
            "<td align='center'>%2</td>"
            "<td align='center'>%3</td>"
            "<td>%4</td>"
            "<td>%5</td>"
            "<td align='center'>%6</td>"
            "<td align='right'>%7</td>"
            "<td align='right'>%8</td>"
            "<td align='right'>%9</td>"
            "<td align='right'>%10</td>"
            "</tr>"
        )
        .arg(rowIdx++)
        .arg(formatDisplayDate(j.value("voucher_date").toString()))
        .arg(j.value("voucher_no").toString())
        .arg(j.value("farmer_name").toString())
        .arg(j.value("item_name", "Paddy PR106").toString())
        .arg(bags)
        .arg(QString::number(wt, 'f', 2))
        .arg(formatINR(amt))
        .arg(formatINR(mfee))
        .arg(formatINR(rdf));
    }

    QString html = QString(
        "<!DOCTYPE html><html><head><style>"
        "body { font-family: 'Helvetica Neue', Arial, sans-serif; font-size: 7.5pt; color: #000; margin: 0; padding: 6px; }"
        "table { width: 100%; border-collapse: collapse; margin-bottom: 5px; }"
        "th, td { border: 1px solid #000; padding: 3px; font-size: 7.5pt; }"
        "th { background-color: #f1f5f9; text-align: center; font-weight: bold; }"
        ".header-box { border: 1.5px solid #000; padding: 5px; margin-bottom: 5px; text-align: center; }"
        "</style></head><body>"
        "<div class='header-box'>"
        "  <div style='font-size: 13pt; font-weight: bold;'>%1</div>"
        "  <div>%2 &nbsp;|&nbsp; Mandi Market Committee: %3</div>"
        "  <div style='font-size: 10pt; font-weight: bold; margin-top: 3px;'>%4 - STATUTORY MANDI REPORT</div>"
        "  <div>Period: <b>%5</b> to <b>%6</b></div>"
        "</div>"
        "<table>"
        "  <tr>"
        "    <th width='4%'>S.N.</th>"
        "    <th width='9%'>Date</th>"
        "    <th width='10%'>Form No</th>"
        "    <th width='24%'>Farmer / Seller</th>"
        "    <th width='15%'>Variety</th>"
        "    <th width='6%'>Bags</th>"
        "    <th width='8%'>Weight Qtl</th>"
        "    <th width='10%'>Value ₹</th>"
        "    <th width='7%'>M.Fee (2%)</th>"
        "    <th width='7%'>RDF (2%)</th>"
        "  </tr>"
        "  %7"
        "  <tr style='font-weight: bold; background-color: #f8fafc;'>"
        "    <td colspan='5' align='right'>TOTAL:</td>"
        "    <td align='center'>%8</td>"
        "    <td align='right'>%9</td>"
        "    <td align='right'>%10</td>"
        "    <td align='right'>%11</td>"
        "    <td align='right'>%12</td>"
        "  </tr>"
        "</table>"
        "</body></html>"
    )
    .arg(firm.value("firm_name").toString())
    .arg(firm.value("address").toString())
    .arg(firm.value("city", "Sirsa").toString())
    .arg(reportType.toUpper())
    .arg(formatDisplayDate(fDate))
    .arg(formatDisplayDate(tDate))
    .arg(rowsHtml)
    .arg(totalBags)
    .arg(QString::number(totalWeight, 'f', 2))
    .arg(formatINR(totalAmt))
    .arg(formatINR(totalMfee))
    .arg(formatINR(totalRdf));

    return html;
}

QString PrintExportController::export_mandi_report_pdf(const QString& reportType, const QString& fromDate, const QString& toDate, const QString& customPath) {
    QString html = renderMandiReportHtml(reportType, fromDate, toDate);
    return exportHtmlToPdf(html, "MandiReport_" + reportType + ".pdf", customPath, true);
}

QString PrintExportController::export_mandi_report_odf(const QString& reportType, const QString& fromDate, const QString& toDate, const QString& customPath) {
    QString html = renderMandiReportHtml(reportType, fromDate, toDate);
    return exportHtmlToOdf(html, "MandiReport_" + reportType + ".odt", customPath, true);
}

QString PrintExportController::export_mandi_report_excel(const QString& reportType, const QString& fromDate, const QString& toDate, const QString& customPath) {
    QString html = renderMandiReportHtml(reportType, fromDate, toDate);
    return exportHtmlToExcel(html, "MandiReport_" + reportType + ".xls", customPath);
}

bool PrintExportController::print_mandi_report(const QString& reportType, const QString& fromDate, const QString& toDate) {
    QString html = renderMandiReportHtml(reportType, fromDate, toDate);
    return printHtml(html, "Mandi Report " + reportType, true);
}

// 4.8 GST Compliance (GSTR-1, GSTR-2, GSTR-3B)
QString PrintExportController::renderGstrReportHtml(const QString& gstrType, const QString& fromDate, const QString& toDate) {
    auto firm = getFirmProfile();

    QString fDate = fromDate.trimmed().isEmpty() ? AccountingEngine::getActiveFromDate() : fromDate.trimmed();
    QString tDate = toDate.trimmed().isEmpty() ? AccountingEngine::getActiveToDate() : toDate.trimmed();

    QVariantList sales = DatabaseManager::instance().executeQuery(
        "SELECT * FROM sales_invoices WHERE invoice_date >= ? AND invoice_date <= ? ORDER BY invoice_date ASC;",
        {fDate, tDate}
    );

    double b2bTaxable = 0.0, b2bCgst = 0.0, b2bSgst = 0.0, b2bIgst = 0.0, b2bTotal = 0.0;
    for (const auto& var : sales) {
        QVariantMap s = var.toMap();
        double tax = s.value("taxable_amount").toDouble();
        double gst = s.value("gst_amount").toDouble();
        double tot = s.value("total_amount").toDouble();
        b2bTaxable += tax;
        b2bCgst += gst / 2.0;
        b2bSgst += gst / 2.0;
        b2bTotal += tot;
    }

    QString html = QString(
        "<!DOCTYPE html><html><head><style>"
        "body { font-family: 'Helvetica Neue', Arial, sans-serif; font-size: 8pt; color: #000; margin: 0; padding: 8px; }"
        "table { width: 100%; border-collapse: collapse; margin-bottom: 6px; }"
        "th, td { border: 1px solid #000; padding: 5px; font-size: 8pt; }"
        "th { background-color: #f1f5f9; text-align: center; font-weight: bold; }"
        ".header-box { border: 1.5px solid #000; padding: 6px; margin-bottom: 6px; text-align: center; }"
        "</style></head><body>"
        "<div class='header-box'>"
        "  <div style='font-size: 13pt; font-weight: bold;'>%1</div>"
        "  <div>%2 &nbsp;|&nbsp; GSTIN: %3</div>"
        "  <div style='font-size: 10pt; font-weight: bold; margin-top: 3px;'>STATUTORY GST REPORT - %4</div>"
        "  <div>Period: <b>%5</b> to <b>%6</b></div>"
        "</div>"
        "<table>"
        "  <tr>"
        "    <th>Section / Table</th>"
        "    <th>Invoice Count</th>"
        "    <th>Taxable Value ₹</th>"
        "    <th>CGST ₹</th>"
        "    <th>SGST ₹</th>"
        "    <th>IGST ₹</th>"
        "    <th>Total Value ₹</th>"
        "  </tr>"
        "  <tr>"
        "    <td><b>4A - B2B Regular Supplies</b></td>"
        "    <td align='center'>%7</td>"
        "    <td align='right'>%8</td>"
        "    <td align='right'>%9</td>"
        "    <td align='right'>%10</td>"
        "    <td align='right'>%11</td>"
        "    <td align='right'><b>%12</b></td>"
        "  </tr>"
        "  <tr style='font-weight: bold; background-color: #f8fafc;'>"
        "    <td>TOTAL OUTWARD SUPPLIES:</td>"
        "    <td align='center'>%7</td>"
        "    <td align='right'>%8</td>"
        "    <td align='right'>%9</td>"
        "    <td align='right'>%10</td>"
        "    <td align='right'>%11</td>"
        "    <td align='right'>%12</td>"
        "  </tr>"
        "</table>"
        "</body></html>"
    )
    .arg(firm.value("firm_name").toString())
    .arg(firm.value("address").toString())
    .arg(firm.value("gstin").toString())
    .arg(gstrType.toUpper())
    .arg(formatDisplayDate(fDate))
    .arg(formatDisplayDate(tDate))
    .arg(sales.size())
    .arg(formatINR(b2bTaxable))
    .arg(formatINR(b2bCgst))
    .arg(formatINR(b2bSgst))
    .arg(formatINR(b2bIgst))
    .arg(formatINR(b2bTotal));

    return html;
}

QString PrintExportController::export_gstr_report_pdf(const QString& gstrType, const QString& fromDate, const QString& toDate, const QString& customPath) {
    QString html = renderGstrReportHtml(gstrType, fromDate, toDate);
    return exportHtmlToPdf(html, "GSTR_" + gstrType + ".pdf", customPath, true);
}

QString PrintExportController::export_gstr_report_odf(const QString& gstrType, const QString& fromDate, const QString& toDate, const QString& customPath) {
    QString html = renderGstrReportHtml(gstrType, fromDate, toDate);
    return exportHtmlToOdf(html, "GSTR_" + gstrType + ".odt", customPath, true);
}

QString PrintExportController::export_gstr_report_excel(const QString& gstrType, const QString& fromDate, const QString& toDate, const QString& customPath) {
    QString html = renderGstrReportHtml(gstrType, fromDate, toDate);
    return exportHtmlToExcel(html, "GSTR_" + gstrType + ".xls", customPath);
}

bool PrintExportController::print_gstr_report(const QString& gstrType, const QString& fromDate, const QString& toDate) {
    QString html = renderGstrReportHtml(gstrType, fromDate, toDate);
    return printHtml(html, "GSTR " + gstrType, true);
}
