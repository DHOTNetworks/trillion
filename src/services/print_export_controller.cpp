#include "print_export_controller.h"
#include "../database_manager.h"
#include "../engine/accounting_engine.h"
#include <QPrinter>
#include <QPrintDialog>
#include <QTextDocument>
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
    if (!filePath.isEmpty() && QFile::exists(filePath)) {
        QDesktopServices::openUrl(QUrl::fromLocalFile(filePath));
    }
}

QVariantMap PrintExportController::getFirmProfile() {
    QVariantMap firm;
    QVariantList rows = DatabaseManager::instance().executeQuery("SELECT * FROM company_info LIMIT 1;");
    if (!rows.isEmpty()) {
        QVariantMap r = rows.first().toMap();
        firm["firm_name"] = r.value("company_name").toString().trimmed();
        
        QString addr = r.value("address").toString().trimmed();
        QString city = r.value("city").toString().trimmed();
        QString st = r.value("state").toString().trimmed();
        QString pin = r.value("pincode").toString().trimmed();
        QString stCode = r.value("state_code").toString().trimmed();
        
        QString fullAddr = addr;
        if (!city.isEmpty() && !fullAddr.contains(city, Qt::CaseInsensitive)) {
            if (!fullAddr.isEmpty()) fullAddr += ", ";
            fullAddr += city;
        }
        if (!st.isEmpty() && !fullAddr.contains(st, Qt::CaseInsensitive)) {
            if (!fullAddr.isEmpty()) fullAddr += " (" + st + ")";
            else fullAddr += st;
        }
        if (!pin.isEmpty() && !fullAddr.contains(pin)) {
            fullAddr += " - " + pin;
        }
        firm["address"] = fullAddr;

        QString ph = r.value("phone").toString().trimmed();
        QString mob = r.value("mobile").toString().trimmed();
        if (!ph.isEmpty() && !mob.isEmpty()) firm["phone"] = ph + ", " + mob;
        else if (!mob.isEmpty()) firm["phone"] = mob;
        else firm["phone"] = ph;

        firm["gstin"] = r.value("gstin").toString().trimmed();
        
        QString stateStr = st;
        if (!stCode.isEmpty()) {
            stateStr += " (" + stCode + ")";
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
    
    if (firm.value("firm_name").toString().isEmpty()) {
        firm["firm_name"] = "COMPANY NAME";
        firm["address"] = "";
        firm["phone"] = "";
        firm["gstin"] = "";
        firm["state"] = "";
        firm["pan"] = "";
        firm["bank_name"] = "";
        firm["bank_ac"] = "";
        firm["bank_ifsc"] = "";
        firm["bank_branch"] = "";
    }
    return firm;
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

static QString getStateNameFromGst(const QString& gstOrCode, const QString& fallbackState = "") {
    QString codeStr;
    QString cleanGst = gstOrCode.trimmed();
    if (cleanGst.length() >= 2 && cleanGst.left(2).toInt() > 0) {
        codeStr = cleanGst.left(2);
    } else if (!cleanGst.isEmpty() && cleanGst.toInt() > 0) {
        codeStr = QString("%1").arg(cleanGst.toInt(), 2, 10, QChar('0'));
    }

    static const QMap<QString, QString> stateMap = {
        {"01", "Jammu & Kashmir"},
        {"02", "Himachal Pradesh"},
        {"03", "Punjab"},
        {"04", "Chandigarh"},
        {"05", "Uttarakhand"},
        {"06", "Haryana"},
        {"07", "Delhi"},
        {"08", "Rajasthan"},
        {"09", "Uttar Pradesh"},
        {"10", "Bihar"},
        {"11", "Sikkim"},
        {"12", "Arunachal Pradesh"},
        {"13", "Nagaland"},
        {"14", "Manipur"},
        {"15", "Mizoram"},
        {"16", "Tripura"},
        {"17", "Meghalaya"},
        {"18", "Assam"},
        {"19", "West Bengal"},
        {"20", "Jharkhand"},
        {"21", "Odisha"},
        {"22", "Chhattisgarh"},
        {"23", "Madhya Pradesh"},
        {"24", "Gujarat"},
        {"27", "Maharashtra"},
        {"29", "Karnataka"},
        {"30", "Goa"},
        {"32", "Kerala"},
        {"33", "Tamil Nadu"},
        {"36", "Telangana"},
        {"37", "Andhra Pradesh"}
    };

    if (!codeStr.isEmpty() && stateMap.contains(codeStr)) {
        return QString("%1 (%2)").arg(stateMap.value(codeStr), codeStr);
    }

    QString cleanFallback = fallbackState.trimmed();
    if (!cleanFallback.isEmpty() && cleanFallback != "INTER-STATE" && cleanFallback != "WITHIN STATE") {
        for (auto it = stateMap.begin(); it != stateMap.end(); ++it) {
            if (cleanFallback.contains(it.value(), Qt::CaseInsensitive) || it.value().contains(cleanFallback, Qt::CaseInsensitive)) {
                return QString("%1 (%2)").arg(it.value(), it.key());
            }
        }
        return cleanFallback;
    }

    return "Haryana (06)";
}

static QVariantMap getPartyProfile(int partyId, const QString& partyName) {
    QVariantMap party;
    if (partyId > 0) {
        QVariantList pRows = DatabaseManager::instance().executeQuery(
            "SELECT * FROM parties WHERE id = ? LIMIT 1;", {partyId}
        );
        if (!pRows.isEmpty()) party = pRows.first().toMap();
    }
    if (party.isEmpty() && !partyName.trimmed().isEmpty()) {
        QVariantList pRows = DatabaseManager::instance().executeQuery(
            "SELECT * FROM parties WHERE name = ? OR mailing_name = ? OR alias = ? LIMIT 1;",
            {partyName.trimmed(), partyName.trimmed(), partyName.trimmed()}
        );
        if (!pRows.isEmpty()) party = pRows.first().toMap();
    }
    if (party.isEmpty() && !partyName.trimmed().isEmpty()) {
        QString cleanName = partyName;
        cleanName = cleanName.remove(QRegularExpression("\\[.*?\\]")).trimmed();
        if (!cleanName.isEmpty()) {
            QVariantList pRows = DatabaseManager::instance().executeQuery(
                "SELECT * FROM parties WHERE name LIKE ? LIMIT 1;",
                {"%" + cleanName + "%"}
            );
            if (!pRows.isEmpty()) party = pRows.first().toMap();
        }
    }
    return party;
}

// ------------------------------------------------------------------------------------------------
// 1. GST TAX INVOICE (SALES BILL) HTML TEMPLATE - MODERN BOXED MONOCHROME B&W
// ------------------------------------------------------------------------------------------------
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
    QString ewayNo = inv.value("eway_bill_no").toString();
    QString transport = inv.value("transport").toString();
    QString grNo = inv.value("gr_no").toString();

    // Query party profile
    QVariantMap party = getPartyProfile(custId, custName);

    QString buyerDisplayName = custName.trimmed();
    if (buyerDisplayName.isEmpty()) buyerDisplayName = party.value("name").toString().trimmed();
    if (buyerDisplayName.isEmpty()) buyerDisplayName = party.value("mailing_name").toString().trimmed();

    QString buyerAddr = party.value("address").toString().trimmed();
    QString buyerCity = party.value("city").toString().trimmed();
    QString buyerPin = party.value("pincode").toString().trimmed();
    QString fullBuyerAddr = buyerAddr;
    if (!buyerCity.isEmpty() && !fullBuyerAddr.contains(buyerCity, Qt::CaseInsensitive)) {
        if (!fullBuyerAddr.isEmpty()) fullBuyerAddr += ", ";
        fullBuyerAddr += buyerCity;
    }
    if (!buyerPin.isEmpty() && !fullBuyerAddr.contains(buyerPin)) {
        if (!fullBuyerAddr.isEmpty()) fullBuyerAddr += " - ";
        fullBuyerAddr += "PIN: " + buyerPin;
    }
    if (fullBuyerAddr.isEmpty()) fullBuyerAddr = "-";

    QString buyerGst = party.value("gstin").toString().trimmed();
    if (buyerGst.isEmpty()) buyerGst = inv.value("gstin").toString().trimmed();
    QString buyerGstDisplay = buyerGst.isEmpty() ? "URP (Unregistered)" : buyerGst;

    QString buyerPan = party.value("pan").toString().trimmed();
    if (buyerPan.isEmpty() && buyerGst.length() >= 12) {
        buyerPan = buyerGst.mid(2, 10);
    }
    if (buyerPan.isEmpty()) buyerPan = "-";

    QString rawPartyState = party.value("state_code").toString().trimmed();
    if (rawPartyState.isEmpty() || rawPartyState == "INTER-STATE" || rawPartyState == "WITHIN STATE") {
        rawPartyState = party.value("state").toString().trimmed();
    }
    QString buyerState = getStateNameFromGst(buyerGst, rawPartyState);

    QString buyerPhone = party.value("mobile").toString().trimmed();
    if (buyerPhone.isEmpty()) buyerPhone = party.value("phone").toString().trimmed();
    if (buyerPhone.isEmpty()) buyerPhone = "-";

    QVariantList items = DatabaseManager::instance().executeQuery(
        "SELECT * FROM sales_invoice_items WHERE invoice_id = ? ORDER BY id ASC;",
        {invId}
    );
    if (items.isEmpty()) {
        QVariantMap itm;
        itm["item_name"] = inv.value("item_name");
        itm["hsn_code"] = inv.value("hsn_code", "100630");
        itm["bag_count"] = inv.value("bag_count");
        itm["weight_qtl"] = inv.value("weight_qtl");
        itm["rate_per_qtl"] = inv.value("rate_per_qtl");
        itm["taxable_amount"] = inv.value("taxable_amount");
        itm["gst_pct"] = inv.value("gst_pct");
        itm["total_amount"] = inv.value("total_amount");
        items.append(itm);
    }

    double taxableTot = inv.value("taxable_amount").toDouble();
    double cgstTot = inv.value("cgst_amount").toDouble();
    double sgstTot = inv.value("sgst_amount").toDouble();
    double igstTot = inv.value("igst_amount").toDouble();
    double gstTot = inv.value("gst_amount").toDouble();
    if (gstTot <= 0.001) gstTot = cgstTot + sgstTot + igstTot;
    double roundOff = inv.value("round_off").toDouble();
    double grandTot = inv.value("total_amount").toDouble();

    QString itemsRowsHtml = "";
    int rowIdx = 1;
    long long totalBags = 0;
    double totalWeight = 0.0;

    for (const auto& var : items) {
        QVariantMap it = var.toMap();
        QString itmName = it.value("item_name").toString();
        QString hsn = it.value("hsn_code", "100630").toString();
        long long bg = it.value("bag_count").toLongLong();
        double wt = it.value("weight_qtl").toDouble();
        double r = it.value("rate_per_qtl").toDouble();
        double tax = it.value("taxable_amount").toDouble();
        double gPct = it.value("gst_pct").toDouble();

        totalBags += bg;
        totalWeight += wt;

        itemsRowsHtml += QString(
            "<tr>"
            "<td align='center' valign='top'>%1</td>"
            "<td valign='top'><b>%2</b></td>"
            "<td align='center' valign='top'>%3</td>"
            "<td align='right' valign='top'>%4</td>"
            "<td align='right' valign='top'>%5</td>"
            "<td align='right' valign='top'>%6</td>"
            "<td align='center' valign='top'>%7%</td>"
            "<td align='right' valign='top'><b>%8</b></td>"
            "</tr>"
        ).arg(QString::number(rowIdx++), itmName, hsn,
             bg > 0 ? QString::number(bg) : "-",
             wt > 0 ? QString::number(wt, 'f', 3) : "-",
             r > 0 ? formatPlainINR(r) : "-",
             QString::number(gPct, 'f', 0),
             formatPlainINR(tax));
    }

    QString html = QString(
        "<table border='1' cellspacing='0' cellpadding='0' width='100%'>"
        "  <tr>"
        "    <td colspan='2' align='center' style='padding: 4pt 0;'>"
        "      <font size='4'><b>TAX INVOICE</b></font><br>"
        "      <font size='1'><b>(%33)</b></font>"
        "    </td>"
        "  </tr>"
        "  <tr>"
        "    <td width='58%' valign='top' style='padding: 5pt;'>"
        "      <font size='3'><b>%1</b></font><br>"
        "      <font size='1'>"
        "      %2<br>"
        "      <b>GSTIN:</b> %3 &nbsp;|&nbsp; <b>PAN:</b> %5<br>"
        "      <b>State:</b> %4 &nbsp;|&nbsp; <b>Phone:</b> %6"
        "      </font>"
        "    </td>"
        "    <td width='42%' valign='top' style='padding: 0;'>"
        "      <table border='1' cellspacing='0' cellpadding='3' width='100%'>"
        "        <tr>"
        "          <td width='50%'>Invoice No:<br><font size='2'><b>%7</b></font></td>"
        "          <td width='50%'>Dated:<br><b>%8</b></td>"
        "        </tr>"
        "        <tr>"
        "          <td>Place of Supply:<br><b>%13</b></td>"
        "          <td>Reverse Charge:<br><b>No</b></td>"
        "        </tr>"
        "        <tr>"
        "          <td>GR / RR No:<br><b>%17</b></td>"
        "          <td>E-Way Bill No:<br><b>%18</b></td>"
        "        </tr>"
        "      </table>"
        "    </td>"
        "  </tr>"
        "  <tr>"
        "    <td width='58%' valign='top' style='padding: 5pt;'>"
        "      <font size='1'><b>BILLED TO / CONSIGNEE (BUYER):</b></font><br>"
        "      <font size='2'><b>%9</b></font><br>"
        "      <font size='1'>"
        "      <b>Address:</b> %10<br>"
        "      <b>GSTIN / UIN:</b> %11 &nbsp;|&nbsp; <b>PAN:</b> %12<br>"
        "      <b>State:</b> %13 &nbsp;|&nbsp; <b>Phone:</b> %14"
        "      </font>"
        "    </td>"
        "    <td width='42%' valign='top' style='padding: 0;'>"
        "      <table border='1' cellspacing='0' cellpadding='4' width='100%'>"
        "        <tr>"
        "          <td width='50%'>Vehicle No:<br><b>%15</b></td>"
        "          <td width='50%'>Transport:<br><b>%16</b></td>"
        "        </tr>"
        "      </table>"
        "    </td>"
        "  </tr>"
        "  <tr>"
        "    <td colspan='2' style='padding: 0;'>"
        "      <table border='1' cellspacing='0' cellpadding='3' width='100%'>"
        "        <tr>"
        "          <th width='4%' align='center'>#</th>"
        "          <th width='37%' align='left'>Description of Goods</th>"
        "          <th width='11%' align='center'>HSN/SAC</th>"
        "          <th width='8%' align='right'>Bags</th>"
        "          <th width='12%' align='right'>Weight (Qtl)</th>"
        "          <th width='11%' align='right'>Rate (₹/Qtl)</th>"
        "          <th width='6%' align='center'>GST%</th>"
        "          <th width='11%' align='right'>Taxable (₹)</th>"
        "        </tr>"
        "        %19"
        "        <tr style='font-weight: bold;'>"
        "          <td colspan='3' align='right'><b>TOTAL:</b></td>"
        "          <td align='right'><b>%20</b></td>"
        "          <td align='right'><b>%21</b></td>"
        "          <td></td>"
        "          <td></td>"
        "          <td align='right'><b>%22</b></td>"
        "        </tr>"
        "      </table>"
        "    </td>"
        "  </tr>"
        "  <tr>"
        "    <td width='58%' valign='top' style='padding: 5pt;'>"
        "      <b>Bank Details:</b><br>"
        "      <font size='1'>"
        "      <b>Bank:</b> %23 &nbsp;|&nbsp; <b>A/C No:</b> %24<br>"
        "      <b>IFSC:</b> %25 &nbsp;|&nbsp; <b>Branch:</b> %26"
        "      </font>"
        "      <br><br>"
        "      <b>Amount in Words:</b><br>"
        "      <font size='1'><b>%27</b></font>"
        "    </td>"
        "    <td width='42%' valign='top' style='padding: 0;'>"
        "      <table border='1' cellspacing='0' cellpadding='3' width='100%'>"
        "        <tr>"
        "          <td width='55%'>Taxable Amount:</td>"
        "          <td width='45%' align='right'>%22</td>"
        "        </tr>"
        "        <tr>"
        "          <td>CGST:</td>"
        "          <td align='right'>%28</td>"
        "        </tr>"
        "        <tr>"
        "          <td>SGST:</td>"
        "          <td align='right'>%29</td>"
        "        </tr>"
        "        <tr>"
        "          <td>IGST:</td>"
        "          <td align='right'>%30</td>"
        "        </tr>"
        "        <tr>"
        "          <td>Round Off:</td>"
        "          <td align='right'>%31</td>"
        "        </tr>"
        "        <tr style='font-weight: bold;'>"
        "          <td><font size='2'><b>GRAND TOTAL:</b></font></td>"
        "          <td align='right'><font size='2'><b>%32</b></font></td>"
        "        </tr>"
        "      </table>"
        "    </td>"
        "  </tr>"
        "  <tr>"
        "    <td width='58%' valign='top' style='padding: 5pt;'>"
        "      <b>Terms & Conditions:</b><br>"
        "      <font size='1'>"
        "      1. Goods once sold will not be taken back.<br>"
        "      2. Interest @ 18% p.a. will be charged if payment is not made within the due date.<br>"
        "      3. Subject to local jurisdiction only."
        "      </font>"
        "    </td>"
        "    <td width='42%' align='right' valign='top' style='padding: 5pt;'>"
        "      <font size='1'><b>For %1</b></font>"
        "      <br><br><br><br>"
        "      <font size='1'>Authorized Signatory</font>"
        "    </td>"
        "  </tr>"
        "</table>"
    )
    .arg(firm.value("firm_name").toString())
    .arg(firm.value("address").toString())
    .arg(firm.value("gstin").toString())
    .arg(firm.value("state").toString())
    .arg(firm.value("pan").toString())
    .arg(firm.value("phone").toString())
    .arg(invNum)
    .arg(invDate)
    .arg(buyerDisplayName)
    .arg(fullBuyerAddr)
    .arg(buyerGstDisplay)
    .arg(buyerPan)
    .arg(buyerState)
    .arg(buyerPhone)
    .arg(vehNo.isEmpty() ? "-" : vehNo)
    .arg(transport.isEmpty() ? "Direct / Self" : transport)
    .arg(grNo.isEmpty() ? "-" : grNo)
    .arg(ewayNo.isEmpty() ? "-" : ewayNo)
    .arg(itemsRowsHtml)
    .arg(totalBags > 0 ? QString::number(totalBags) : "-")
    .arg(totalWeight > 0 ? QString::number(totalWeight, 'f', 3) : "-")
    .arg(formatPlainINR(taxableTot))
    .arg(firm.value("bank_name").toString().isEmpty() ? "-" : firm.value("bank_name").toString())
    .arg(firm.value("bank_ac").toString().isEmpty() ? "-" : firm.value("bank_ac").toString())
    .arg(firm.value("bank_ifsc").toString().isEmpty() ? "-" : firm.value("bank_ifsc").toString())
    .arg(firm.value("bank_branch", "Main Branch").toString())
    .arg(amountInWords(grandTot))
    .arg(formatPlainINR(cgstTot))
    .arg(formatPlainINR(sgstTot))
    .arg(formatPlainINR(igstTot))
    .arg(QString::number(roundOff, 'f', 2))
    .arg(formatINR(grandTot))
    .arg(copySubtitle);

    return html;
}

QString PrintExportController::renderSalesInvoiceHtml(const QString& invoiceNo, const QString& copyType) {
    QString normCopy = copyType.trimmed().toUpper();
    if (normCopy.isEmpty()) normCopy = "ORIGINAL FOR RECIPIENT";

    if (normCopy == "ALL" || normCopy == "BOTH" || normCopy == "ORIGINAL + DUPLICATE" || normCopy == "ALL COPIES") {
        QString c1 = renderSalesInvoiceSingleHtml(invoiceNo, "ORIGINAL FOR RECIPIENT");
        QString c2 = renderSalesInvoiceSingleHtml(invoiceNo, "DUPLICATE FOR TRANSPORTER");
        return QString(
            "<!DOCTYPE html><html><head><meta charset='utf-8'><style>"
            "  body { font-family: Arial, Helvetica, sans-serif; font-size: 8.5pt; color: #000000; margin: 0; padding: 0; background: #ffffff; }"
            "  table { border-collapse: collapse; }"
            "  th { font-weight: bold; font-size: 8pt; }"
            "  td { font-size: 8pt; }"
            "</style></head><body>"
            "%1"
            "<div style='page-break-before: always;'></div><br>"
            "%2"
            "</body></html>"
        ).arg(c1, c2);
    }

    QString subtitle = "ORIGINAL FOR RECIPIENT";
    if (normCopy.contains("DUPLICATE") || normCopy.contains("TRANSPORTER")) {
        subtitle = "DUPLICATE FOR TRANSPORTER";
    } else if (normCopy.contains("TRIPLICATE") || normCopy.contains("SUPPLIER")) {
        subtitle = "TRIPLICATE FOR SUPPLIER";
    } else if (normCopy.contains("EXTRA")) {
        subtitle = "EXTRA COPY";
    } else if (!normCopy.contains("ORIGINAL")) {
        subtitle = normCopy;
    }

    return QString(
        "<!DOCTYPE html><html><head><meta charset='utf-8'><style>"
        "  body { font-family: Arial, Helvetica, sans-serif; font-size: 8.5pt; color: #000000; margin: 0; padding: 0; background: #ffffff; }"
        "  table { border-collapse: collapse; }"
        "  th { font-weight: bold; font-size: 8pt; }"
        "  td { font-size: 8pt; }"
        "</style></head><body>"
        "%1"
        "</body></html>"
    ).arg(renderSalesInvoiceSingleHtml(invoiceNo, subtitle));
}

// ------------------------------------------------------------------------------------------------
// 2. PURCHASE BILL HTML TEMPLATE - MODERN BOXED MONOCHROME B&W
// ------------------------------------------------------------------------------------------------
QString PrintExportController::renderPurchaseInvoiceHtml(const QString& invoiceNo) {
    auto firm = getFirmProfile();

    QVariantList invRows = DatabaseManager::instance().executeQuery(
        "SELECT * FROM purchase_invoices WHERE invoice_no = ? OR id = ? LIMIT 1;",
        {invoiceNo.trimmed(), invoiceNo.trimmed()}
    );
    if (invRows.isEmpty()) {
        return "<html><body><h2>Purchase invoice not found: " + invoiceNo + "</h2></body></html>";
    }

    QVariantMap inv = invRows.first().toMap();
    QString invNum = inv.value("invoice_no").toString();
    QString invDate = formatDisplayDate(inv.value("invoice_date").toString());
    QString suppName = inv.value("supplier_name").toString();
    int suppId = inv.value("supplier_id").toInt();
    QString item = inv.value("item_name").toString();
    long long bags = inv.value("bag_count").toLongLong();
    double wt = inv.value("weight_qtl").toDouble();
    double r = inv.value("rate_per_qtl").toDouble();
    double taxable = inv.value("taxable_amount").toDouble();
    double gstAmt = inv.value("gst_amount").toDouble();
    double total = inv.value("total_amount").toDouble();
    QString veh = inv.value("vehicle_no").toString();

    // Query supplier party profile
    QVariantMap party = getPartyProfile(suppId, suppName);

    QString suppDisplayName = suppName.trimmed();
    if (suppDisplayName.isEmpty()) suppDisplayName = party.value("name").toString().trimmed();
    if (suppDisplayName.isEmpty()) suppDisplayName = party.value("mailing_name").toString().trimmed();

    QString suppAddr = party.value("address").toString().trimmed();
    QString suppCity = party.value("city").toString().trimmed();
    QString suppPin = party.value("pincode").toString().trimmed();
    QString fullSuppAddr = suppAddr;
    if (!suppCity.isEmpty() && !fullSuppAddr.contains(suppCity, Qt::CaseInsensitive)) {
        if (!fullSuppAddr.isEmpty()) fullSuppAddr += ", ";
        fullSuppAddr += suppCity;
    }
    if (!suppPin.isEmpty() && !fullSuppAddr.contains(suppPin)) {
        if (!fullSuppAddr.isEmpty()) fullSuppAddr += " - ";
        fullSuppAddr += "PIN: " + suppPin;
    }
    if (fullSuppAddr.isEmpty()) fullSuppAddr = "-";

    QString suppGst = party.value("gstin").toString().trimmed();
    if (suppGst.isEmpty()) suppGst = inv.value("gstin").toString().trimmed();
    QString suppGstDisplay = suppGst.isEmpty() ? "URP (Unregistered)" : suppGst;

    QString suppPan = party.value("pan").toString().trimmed();
    if (suppPan.isEmpty() && suppGst.length() >= 12) {
        suppPan = suppGst.mid(2, 10);
    }
    if (suppPan.isEmpty()) suppPan = "-";

    QString rawPartyState = party.value("state_code").toString().trimmed();
    if (rawPartyState.isEmpty() || rawPartyState == "INTER-STATE" || rawPartyState == "WITHIN STATE") {
        rawPartyState = party.value("state").toString().trimmed();
    }
    QString suppState = getStateNameFromGst(suppGst, rawPartyState);

    QString suppPhone = party.value("mobile").toString().trimmed();
    if (suppPhone.isEmpty()) suppPhone = party.value("phone").toString().trimmed();
    if (suppPhone.isEmpty()) suppPhone = "-";

    QString html = QString(
        "<!DOCTYPE html><html><head><meta charset='utf-8'><style>"
        "  body { font-family: Arial, Helvetica, sans-serif; font-size: 8.5pt; color: #000000; margin: 0; padding: 0; background: #ffffff; }"
        "  table { border-collapse: collapse; }"
        "  th { font-weight: bold; font-size: 8pt; }"
        "  td { font-size: 8pt; }"
        "</style></head><body>"
        "<table border='1' cellspacing='0' cellpadding='0' width='100%'>"
        "  <tr>"
        "    <td colspan='2' align='center' style='padding: 4pt 0;'>"
        "      <font size='4'><b>PURCHASE VOUCHER</b></font><br>"
        "      <font size='1'><b>(INWARD COMMODITY PURCHASE BILL)</b></font>"
        "    </td>"
        "  </tr>"
        "  <tr>"
        "    <td width='58%' valign='top' style='padding: 5pt;'>"
        "      <font size='3'><b>%1</b></font><br>"
        "      <font size='1'>"
        "      %2<br>"
        "      <b>GSTIN:</b> %3 &nbsp;|&nbsp; <b>PAN:</b> %5<br>"
        "      <b>State:</b> %4 &nbsp;|&nbsp; <b>Phone:</b> %6"
        "      </font>"
        "    </td>"
        "    <td width='42%' valign='top' style='padding: 0;'>"
        "      <table border='1' cellspacing='0' cellpadding='3' width='100%'>"
        "        <tr>"
        "          <td width='50%'>Bill / Voucher No:<br><font size='2'><b>%7</b></font></td>"
        "          <td width='50%'>Dated:<br><b>%8</b></td>"
        "        </tr>"
        "        <tr>"
        "          <td>Place of Supply:<br><b>%4</b></td>"
        "          <td>Type:<br><b>Paddy / Inward</b></td>"
        "        </tr>"
        "        <tr>"
        "          <td>Vehicle No:<br><b>%15</b></td>"
        "          <td>E-Way Bill:<br><b>-</b></td>"
        "        </tr>"
        "      </table>"
        "    </td>"
        "  </tr>"
        "  <tr>"
        "    <td width='58%' valign='top' style='padding: 5pt;'>"
        "      <font size='1'><b>SUPPLIER / FARMER ACCOUNT:</b></font><br>"
        "      <font size='2'><b>%9</b></font><br>"
        "      <font size='1'>"
        "      <b>Address:</b> %10<br>"
        "      <b>GSTIN / UIN:</b> %11 &nbsp;|&nbsp; <b>PAN:</b> %12<br>"
        "      <b>State:</b> %13 &nbsp;|&nbsp; <b>Phone:</b> %14"
        "      </font>"
        "    </td>"
        "    <td width='42%' valign='top' style='padding: 0;'>"
        "      <table border='1' cellspacing='0' cellpadding='4' width='100%'>"
        "        <tr>"
        "          <td width='50%'>Weight (Qtl):<br><b>%18</b></td>"
        "          <td width='50%'>Bags:<br><b>%17</b></td>"
        "        </tr>"
        "      </table>"
        "    </td>"
        "  </tr>"
        "  <tr>"
        "    <td colspan='2' style='padding: 0;'>"
        "      <table border='1' cellspacing='0' cellpadding='3' width='100%'>"
        "        <tr>"
        "          <th width='4%' align='center'>#</th>"
        "          <th width='40%' align='left'>Commodity Item Description</th>"
        "          <th width='10%' align='center'>HSN/SAC</th>"
        "          <th width='10%' align='right'>Bags</th>"
        "          <th width='12%' align='right'>Weight (Qtl)</th>"
        "          <th width='12%' align='right'>Rate (₹/Qtl)</th>"
        "          <th width='12%' align='right'>Total Amount (₹)</th>"
        "        </tr>"
        "        <tr>"
        "          <td align='center' valign='top'>1</td>"
        "          <td valign='top'><b>%16</b></td>"
        "          <td align='center' valign='top'>100610</td>"
        "          <td align='right' valign='top'>%17</td>"
        "          <td align='right' valign='top'>%18</td>"
        "          <td align='right' valign='top'>%19</td>"
        "          <td align='right' valign='top'><b>%20</b></td>"
        "        </tr>"
        "        <tr style='font-weight: bold;'>"
        "          <td colspan='3' align='right'><b>TOTAL:</b></td>"
        "          <td align='right'><b>%17</b></td>"
        "          <td align='right'><b>%18</b></td>"
        "          <td></td>"
        "          <td align='right'><b>%20</b></td>"
        "        </tr>"
        "      </table>"
        "    </td>"
        "  </tr>"
        "  <tr>"
        "    <td width='58%' valign='top' style='padding: 5pt;'>"
        "      <b>Amount in Words:</b><br>"
        "      <font size='1'><b>%22</b></font>"
        "    </td>"
        "    <td width='42%' valign='top' style='padding: 0;'>"
        "      <table border='1' cellspacing='0' cellpadding='3' width='100%'>"
        "        <tr>"
        "          <td width='55%'>Taxable Value:</td>"
        "          <td width='45%' align='right'>%20</td>"
        "        </tr>"
        "        <tr>"
        "          <td>GST Amount:</td>"
        "          <td align='right'>%21</td>"
        "        </tr>"
        "        <tr style='font-weight: bold;'>"
        "          <td><font size='2'><b>NET PAYABLE:</b></font></td>"
        "          <td align='right'><font size='2'><b>%23</b></font></td>"
        "        </tr>"
        "      </table>"
        "    </td>"
        "  </tr>"
        "  <tr>"
        "    <td width='58%' valign='top' style='padding: 5pt;'>"
        "      <b>Notes:</b><br>"
        "      <font size='1'>"
        "      Goods received in good condition. Verified & Passed for ledger credit."
        "      </font>"
        "    </td>"
        "    <td width='42%' align='right' valign='top' style='padding: 5pt;'>"
        "      <font size='1'><b>For %1</b></font>"
        "      <br><br><br><br>"
        "      <font size='1'>Authorized Signatory</font>"
        "    </td>"
        "  </tr>"
        "</table>"
        "</body></html>"
    )
    .arg(firm.value("firm_name").toString())
    .arg(firm.value("address").toString())
    .arg(firm.value("gstin").toString())
    .arg(firm.value("state").toString())
    .arg(firm.value("pan").toString())
    .arg(firm.value("phone").toString())
    .arg(invNum)
    .arg(invDate)
    .arg(suppDisplayName)
    .arg(fullSuppAddr)
    .arg(suppGstDisplay)
    .arg(suppPan)
    .arg(suppState)
    .arg(suppPhone)
    .arg(veh.isEmpty() ? "-" : veh)
    .arg(item)
    .arg(bags > 0 ? QString::number(bags) : "-")
    .arg(wt > 0 ? QString::number(wt, 'f', 3) : "-")
    .arg(r > 0 ? formatPlainINR(r) : "-")
    .arg(formatPlainINR(taxable))
    .arg(formatPlainINR(gstAmt))
    .arg(amountInWords(total))
    .arg(formatINR(total));

    return html;
}

// ------------------------------------------------------------------------------------------------
// 3. 2-COLUMN DR/CR LEDGER STATEMENT HTML TEMPLATE - MODERN BOXED MONOCHROME B&W
// ------------------------------------------------------------------------------------------------
QString PrintExportController::renderLedgerStatementHtml(const QString& partyName, const QString& fromDate, const QString& toDate) {
    auto firm = getFirmProfile();
    QString pName = partyName.trimmed();

    QString fDate = fromDate.trimmed();
    QString tDate = toDate.trimmed();
    if (fDate.isEmpty() && tDate.isEmpty()) {
        fDate = AccountingEngine::getActiveFromDate();
        tDate = AccountingEngine::getActiveToDate();
    }

    QVariantList pRows = DatabaseManager::instance().executeQuery(
        "SELECT id, legacy_id, name, group_name, party_type, phone, city, gstin, opening_balance, balance_type "
        "FROM parties WHERE name = ? OR alias = ? OR name LIKE ? LIMIT 1;",
        {pName, pName, "%" + pName + "%"}
    );
    QString pPhone = "-", pPlace = "-", pGstin = "-";
    int partyId = 0;
    int legacyCode = 0;
    double initialOp = 0.0;
    QString initialOpType = "Cr";

    if (!pRows.isEmpty()) {
        QVariantMap pm = pRows.first().toMap();
        partyId = pm.value("id").toInt();
        legacyCode = pm.value("legacy_id").toInt();
        pPhone = pm.value("phone", "-").toString();
        pPlace = pm.value("city", pm.value("place", "-")).toString();
        pGstin = pm.value("gstin", "-").toString();
        initialOp = pm.value("opening_balance").toDouble();
        initialOpType = pm.value("balance_type").toString();
    }

    double priorDr = 0.0;
    double priorCr = 0.0;
    if (initialOpType.compare("Dr", Qt::CaseInsensitive) == 0) priorDr += initialOp;
    else priorCr += initialOp;

    if (!fDate.isEmpty() && fDate != "ALL" && fDate != "All") {
        QVariantList priorRows = DatabaseManager::instance().executeQuery(
            "SELECT dr_cr, SUM(amount) as total_amt FROM transactions "
            "WHERE (party_name = ? OR party_name LIKE ? OR party_id = ? OR (account_code > 0 AND account_code = ?)) "
            "AND voucher_date < ? GROUP BY dr_cr;",
            {pName, "%" + pName + "%", partyId, legacyCode, fDate}
        );
        for (const auto& pr : priorRows) {
            QVariantMap m = pr.toMap();
            if (m.value("dr_cr").toString().compare("Dr", Qt::CaseInsensitive) == 0) {
                priorDr += m.value("total_amt").toDouble();
            } else {
                priorCr += m.value("total_amt").toDouble();
            }
        }
    }

    double netOp = priorDr - priorCr;

    QString drRowsHtml = "";
    QString crRowsHtml = "";
    double drTotal = 0.0;
    double crTotal = 0.0;

    if (std::abs(netOp) > 0.001) {
        QString opDate = formatDisplayDate(!fDate.isEmpty() && fDate != "ALL" ? fDate : "2025-04-01");
        if (netOp >= 0) {
            drTotal += netOp;
            drRowsHtml += QString(
                "<tr style='font-weight:bold;'>"
                "<td align='center'>%1</td>"
                "<td align='center'>OP-BAL</td>"
                "<td align='left'>Opening Balance (Dr)</td>"
                "<td align='right'>%2</td>"
                "</tr>"
            ).arg(opDate, formatPlainINR(netOp));
        } else {
            double crAmt = std::abs(netOp);
            crTotal += crAmt;
            crRowsHtml += QString(
                "<tr style='font-weight:bold;'>"
                "<td align='center'>%1</td>"
                "<td align='center'>OP-BAL</td>"
                "<td align='left'>Opening Balance (Cr)</td>"
                "<td align='right'>%2</td>"
                "</tr>"
            ).arg(opDate, formatPlainINR(crAmt));
        }
    }

    QString sql = "SELECT id, voucher_no, voucher_date, voucher_type, trans_type, opposing_account, dr_cr, amount, invoice_no, narration, broker_name, vehicle_no, taxable_amount, tds_amount FROM transactions WHERE (party_name = ? OR party_name LIKE ? OR party_id = ? OR (account_code > 0 AND account_code = ?))";
    QVariantList params = {pName, "%" + pName + "%", partyId, legacyCode};
    if (!fDate.isEmpty() && !tDate.isEmpty() && fDate != "ALL" && fDate != "All") {
        sql += " AND voucher_date >= ? AND voucher_date <= ?";
        params << fDate << tDate;
    }
    sql += " ORDER BY voucher_date ASC, CAST(voucher_no AS INTEGER) ASC, id ASC;";

    QVariantList rows = DatabaseManager::instance().executeQuery(sql, params);

    for (const auto& rVar : rows) {
        QVariantMap t = rVar.toMap();
        QString vNo = t.value("voucher_no").toString();
        QString rawType = t.value("trans_type").toString();
        QString vType = t.value("voucher_type").toString();
        QString opposing = t.value("opposing_account").toString();
        QString drCr = t.value("dr_cr").toString();
        double amt = t.value("amount").toDouble();
        QString invNo = t.value("invoice_no").toString();
        QString narr = t.value("narration").toString().trimmed();
        QString veh = t.value("vehicle_no").toString().trimmed();
        QString broker = t.value("broker_name").toString().trimmed();
        QString dt = formatDisplayDate(t.value("voucher_date").toString());

        if (rawType == "TDS" || vType == "TDS") {
            rawType = "TDS";
            vType = "TDS";
        }
        QString displayRef = !rawType.isEmpty() ? QString("%1 %2").arg(rawType, vNo).trimmed() : QString("%1 %2").arg(vType, vNo).trimmed();
        if (displayRef.isEmpty()) displayRef = vNo;

        QString desc;
        if (rawType == "Sale" || vType == "Sales") {
            desc = QString("Sales Invoice: %1").arg(!invNo.isEmpty() ? invNo : vNo);
        } else if (rawType == "Purc" || vType == "Purchase") {
            desc = QString("Purchase Bill: %1").arg(!invNo.isEmpty() ? invNo : vNo);
        } else if (rawType == "ChRt" || rawType == "Rcpt" || vType == "Receipt") {
            desc = QString("Receipt via %1").arg(!opposing.isEmpty() ? opposing : "Bank/Cash");
        } else if (rawType == "ChPt" || rawType == "Pymt" || vType == "Payment") {
            desc = QString("Payment to %1").arg(!opposing.isEmpty() ? opposing : "Bank/Cash");
        } else if (rawType == "TDS" || vType == "TDS") {
            desc = !narr.isEmpty() ? narr : QString("TDS: %1").arg(!opposing.isEmpty() ? opposing : "TDS");
        } else if (rawType == "Jrnl" || vType == "Journal") {
            desc = QString("Journal: %1").arg(!opposing.isEmpty() ? opposing : "A/c");
        } else if (rawType == "JFrm" || vType == "J-Form") {
            desc = QString("J-Form: %1").arg(!opposing.isEmpty() ? opposing : "Paddy Purchase");
        } else {
            desc = QString("%1: %2").arg(vType, opposing);
        }

        if (!veh.isEmpty()) desc += " | Veh: " + veh;
        if (!broker.isEmpty()) desc += " | Broker: " + broker;
        if (!narr.isEmpty() && rawType != "TDS" && vType != "TDS" && !desc.contains(narr)) desc += " | " + narr;

        double tdsAmt = t.value("tds_amount").toDouble();
        if ((rawType == "Purc" || vType == "Purchase") && tdsAmt > 0.0) {
            double grossAmt = amt + tdsAmt;
            crTotal += grossAmt;
            QString purcDesc = QString("B.No. %1").arg(!invNo.isEmpty() ? invNo : vNo);
            if (!veh.isEmpty()) purcDesc += " | Veh: " + veh;
            if (!broker.isEmpty()) purcDesc += " | Broker: " + broker;
            if (!narr.isEmpty()) purcDesc += " | " + narr;

            crRowsHtml += QString(
                "<tr>"
                "<td align='center'>%1</td>"
                "<td align='center'>%2</td>"
                "<td align='left'>%3</td>"
                "<td align='right'><b>%4</b></td>"
                "</tr>"
            ).arg(dt, displayRef, purcDesc, formatPlainINR(grossAmt));

            drTotal += tdsAmt;
            drRowsHtml += QString(
                "<tr>"
                "<td align='center'>%1</td>"
                "<td align='center'>%2</td>"
                "<td align='left'>%3</td>"
                "<td align='right'><b>%4</b></td>"
                "</tr>"
            ).arg(dt, displayRef, QString("T.D.S. U/S 194Q (B.No. %1)").arg(!invNo.isEmpty() ? invNo : vNo), formatPlainINR(tdsAmt));
        } else {
            if (drCr.compare("Dr", Qt::CaseInsensitive) == 0) {
                drTotal += amt;
                drRowsHtml += QString(
                    "<tr>"
                    "<td align='center'>%1</td>"
                    "<td align='center'>%2</td>"
                    "<td align='left'>%3</td>"
                    "<td align='right'><b>%4</b></td>"
                    "</tr>"
                ).arg(dt, displayRef, desc, formatPlainINR(amt));
            } else {
                crTotal += amt;
                crRowsHtml += QString(
                    "<tr>"
                    "<td align='center'>%1</td>"
                    "<td align='center'>%2</td>"
                    "<td align='left'>%3</td>"
                    "<td align='right'><b>%4</b></td>"
                    "</tr>"
                ).arg(dt, displayRef, desc, formatPlainINR(amt));
            }
        }
    }

    double netBal = std::abs(drTotal - crTotal);
    QString netBalType = (drTotal >= crTotal) ? "Dr (Receivable)" : "Cr (Payable)";

    QString html = QString(
        "<!DOCTYPE html><html><head><meta charset='utf-8'><style>"
        "  body { font-family: 'Helvetica Neue', Helvetica, Arial, sans-serif; font-size: 8.5pt; color: #000000; margin: 0; padding: 0; background-color: #ffffff; }"
        "  table { border-collapse: collapse; }"
        "  th { font-weight: bold; font-size: 8pt; }"
        "  td { font-size: 8pt; }"
        "</style></head><body>"
        ""
        "<div align='center' style='margin-bottom: 4pt;'>"
        "  <font size='4'><b>%1</b></font><br>"
        "  <font size='1'>%2 &nbsp;|&nbsp; <b>GSTIN:</b> %3 &nbsp;|&nbsp; <b>Phone:</b> %4</font><br>"
        "  <font size='2'><b>ACCOUNT LEDGER STATEMENT</b></font>"
        "</div>"
        ""
        "<table width='100%' style='margin-bottom: 5pt;'>"
        "  <tr>"
        "    <td width='60%'>"
        "      <b>Account:</b> <font size='2'><b>%5</b></font><br>"
        "      <b>Station / Place:</b> %6 &nbsp;|&nbsp; <b>Phone:</b> %7"
        "    </td>"
        "    <td width='40%' align='right' valign='top'>"
        "      <b>Period:</b> %8 To %9<br>"
        "      <b>GSTIN:</b> %10"
        "    </td>"
        "  </tr>"
        "</table>"
        ""
        "<table width='100%' border='0' cellspacing='0' cellpadding='0'>"
        "  <tr>"
        "    <!-- DEBIT SIDE (Left) -->"
        "    <td width='50%' valign='top' style='padding-right: 4pt;'>"
        "      <table border='1' cellspacing='0' cellpadding='3' width='100%'>"
        "        <tr>"
        "          <td colspan='4' align='center'><b>DEBIT / JAMA (RECEIVABLE / GOODS DISPATCHED)</b></td>"
        "        </tr>"
        "        <tr>"
        "          <th width='15%' align='center'>Date</th>"
        "          <th width='15%' align='center'>Vch No</th>"
        "          <th width='46%' align='left'>Particulars</th>"
        "          <th width='24%' align='right'>Amount ₹</th>"
        "        </tr>"
        "        %11"
        "        <tr style='font-weight: bold;'>"
        "          <td colspan='3' align='right'>Total Debit:</td>"
        "          <td align='right'>%13</td>"
        "        </tr>"
        "      </table>"
        "    </td>"
        ""
        "    <!-- CREDIT SIDE (Right) -->"
        "    <td width='50%' valign='top' style='padding-left: 4pt;'>"
        "      <table border='1' cellspacing='0' cellpadding='3' width='100%'>"
        "        <tr>"
        "          <td colspan='4' align='center'><b>CREDIT / NAMA (PAYABLE / PAYMENTS RECEIVED)</b></td>"
        "        </tr>"
        "        <tr>"
        "          <th width='15%' align='center'>Date</th>"
        "          <th width='15%' align='center'>Vch No</th>"
        "          <th width='46%' align='left'>Particulars</th>"
        "          <th width='24%' align='right'>Amount ₹</th>"
        "        </tr>"
        "        %12"
        "        <tr style='font-weight: bold;'>"
        "          <td colspan='3' align='right'>Total Credit:</td>"
        "          <td align='right'>%14</td>"
        "        </tr>"
        "      </table>"
        "    </td>"
        "  </tr>"
        "</table>"
        ""
        "<table width='100%' style='margin-top: 6pt; border-top: 1.5pt solid #000000; border-bottom: 1.5pt solid #000000; padding: 4pt 0;'>"
        "  <tr>"
        "    <td width='60%'><font size='2'><b>NET CLOSING BALANCE: %15</b></font></td>"
        "    <td width='40%' align='right'><font size='2'><b>STATUS: %16</b></font></td>"
        "  </tr>"
        "</table>"
        ""
        "<table width='100%' style='margin-top: 8pt;'>"
        "  <tr>"
        "    <td width='50%' valign='bottom'>"
        "      <font size='1'>Generated on: %17 &nbsp;|&nbsp; E. & O.E.</font>"
        "    </td>"
        "    <td width='50%' align='right' valign='bottom'>"
        "      <font size='1'><b>For %1</b></font><br><br><br>"
        "      <font size='1'>Authorized Signatory</font>"
        "    </td>"
        "  </tr>"
        "</table>"
        "</body></html>"
    )
    .arg(firm.value("firm_name").toString())
    .arg(firm.value("address").toString())
    .arg(firm.value("gstin").toString())
    .arg(firm.value("phone").toString())
    .arg(pName)
    .arg(pPlace)
    .arg(pPhone)
    .arg(formatDisplayDate(fDate))
    .arg(formatDisplayDate(tDate))
    .arg(pGstin)
    .arg(drRowsHtml)
    .arg(crRowsHtml)
    .arg(formatINR(drTotal))
    .arg(formatINR(crTotal))
    .arg(formatINR(netBal))
    .arg(netBalType)
    .arg(QDateTime::currentDateTime().toString("dd-MM-yyyy hh:mm AP"));

    return html;
}

// ------------------------------------------------------------------------------------------------
// 4. STOCK REGISTER REPORT HTML TEMPLATE - MODERN BOXED MONOCHROME B&W
// ------------------------------------------------------------------------------------------------
QString PrintExportController::renderStockRegisterHtml(const QString& fromDate, const QString& toDate) {
    auto firm = getFirmProfile();

    QString fDate = fromDate.trimmed();
    QString tDate = toDate.trimmed();
    if (fDate.isEmpty() && tDate.isEmpty()) {
        fDate = AccountingEngine::getActiveFromDate();
        tDate = AccountingEngine::getActiveToDate();
    }

    QVariantList items = DatabaseManager::instance().executeQuery("SELECT * FROM stock_items ORDER BY name COLLATE NOCASE ASC;");
    QString rowsHtml = "";
    int rowIdx = 1;
    double totalCloseQty = 0.0;
    double totalCloseVal = 0.0;

    for (const auto& itmVar : items) {
        QVariantMap item = itmVar.toMap();
        QString name = item.value("name").toString();
        int itemId = item.value("id").toInt();
        QString code = item.value("code").toString();
        QString itemType = item.value("item_type", "Goods").toString();
        QString unit = item.value("unit", "Qtl").toString();

        double opQty = item.value("opening_qty").toDouble();
        double inQty = 0.0;
        double outQty = 0.0;
        double closeQty = opQty;
        double rate = item.value("rate", item.value("standard_cost", 0.0)).toDouble();

        QVariant inVal = DatabaseManager::instance().executeScalar(
            "SELECT SUM(weight_qtl) FROM stock_transactions WHERE (item_id = ? OR item_name = ?) AND trans_type IN ('Purc', 'SlRn', 'Inward', 'P', 'M');",
            {itemId, name}
        );
        if (inVal.isValid()) inQty = inVal.toDouble();

        QVariant outVal = DatabaseManager::instance().executeScalar(
            "SELECT SUM(weight_qtl) FROM stock_transactions WHERE (item_id = ? OR item_name = ?) AND trans_type IN ('Sale', 'PrRn', 'Outward', 'S');",
            {itemId, name}
        );
        if (outVal.isValid()) outQty = outVal.toDouble();

        closeQty = (opQty + inQty - outQty);
        double closeVal = closeQty * rate;

        totalCloseQty += closeQty;
        totalCloseVal += closeVal;

        rowsHtml += QString(
            "<tr>"
            "<td align='center'>%1</td>"
            "<td><b>%2</b></td>"
            "<td align='center'>%3</td>"
            "<td align='center'>%4</td>"
            "<td align='center'>%5</td>"
            "<td align='right'>%6</td>"
            "<td align='right'>%7</td>"
            "<td align='right'>%8</td>"
            "<td align='right'><b>%9</b></td>"
            "<td align='right'>%10</td>"
            "<td align='right'><b>%11</b></td>"
            "</tr>"
        ).arg(QString::number(rowIdx++), name, code.isEmpty() ? "-" : code, itemType, unit,
             QString::number(opQty, 'f', 2), QString::number(inQty, 'f', 2), QString::number(outQty, 'f', 2),
             QString::number(closeQty, 'f', 2), formatPlainINR(rate), formatPlainINR(closeVal));
    }

    QString html = QString(
        "<!DOCTYPE html><html><head><meta charset='utf-8'><style>"
        "  body { font-family: 'Helvetica Neue', Helvetica, Arial, sans-serif; font-size: 8.5pt; color: #000000; margin: 0; padding: 0; background-color: #ffffff; }"
        "  table { width: 100%; border-collapse: collapse; }"
        "  th { border: 1pt solid #000000; padding: 3.5pt 3pt; font-size: 8pt; font-weight: bold; color: #000000; background-color: #ffffff; }"
        "  td { border: 0.5pt solid #000000; padding: 3pt 3pt; font-size: 8pt; }"
        "</style></head><body>"
        "<div align='center' style='margin-bottom: 4pt;'>"
        "  <font size='4'><b>%1</b></font><br>"
        "  <font size='1'>%2 &nbsp;|&nbsp; <b>GSTIN:</b> %3</font><br>"
        "  <font size='2'><b>ITEMIZED PHYSICAL STOCK REGISTER & VALUATION REPORT</b></font><br>"
        "  <font size='1'><b>Period:</b> %4 To %5</font>"
        "</div>"
        ""
        "<table width='100%'>"
        "  <thead>"
        "    <tr>"
        "      <th width='4%'>#</th>"
        "      <th width='28%' align='left'>Item Commodity Name</th>"
        "      <th width='8%' align='center'>Code</th>"
        "      <th width='8%' align='center'>Type</th>"
        "      <th width='5%' align='center'>Unit</th>"
        "      <th width='8%' align='right'>Opening</th>"
        "      <th width='8%' align='right'>Inward</th>"
        "      <th width='8%' align='right'>Outward</th>"
        "      <th width='9%' align='right'>Closing</th>"
        "      <th width='8%' align='right'>Rate (₹)</th>"
        "      <th width='14%' align='right'>Closing Value (₹)</th>"
        "    </tr>"
        "  </thead>"
        "  <tbody>"
        "    %6"
        "    <tr style='font-weight: bold; border-top: 1.5pt solid #000000;'>"
        "      <td colspan='8' align='right' style='padding: 4pt;'>GRAND TOTAL CLOSING STOCK & VALUATION:</td>"
        "      <td align='right' style='padding: 4pt;'>%7 Qtl</td>"
        "      <td></td>"
        "      <td align='right' style='padding: 4pt;'>%8</td>"
        "    </tr>"
        "  </tbody>"
        "</table>"
        ""
        "<table width='100%' style='margin-top: 10pt; border: none;'>"
        "  <tr>"
        "    <td width='50%' style='border:none; font-size:7.5pt;' valign='bottom'>"
        "      Report Generated on: %9 &nbsp;|&nbsp; E. & O.E."
        "    </td>"
        "    <td width='50%' align='right' style='border:none;' valign='bottom'>"
        "      <div style='font-size: 8.5pt; font-weight: bold;'>For %1</div>"
        "      <div style='height: 25pt;'></div>"
        "      <div style='font-size: 7.5pt; border-top: 1pt solid #000000; display: inline-block; padding-top: 2pt;'>Authorized Signatory</div>"
        "    </td>"
        "  </tr>"
        "</table>"
        "</body></html>"
    )
    .arg(firm.value("firm_name").toString()) // %1
    .arg(firm.value("address").toString()) // %2
    .arg(firm.value("gstin").toString()) // %3
    .arg(formatDisplayDate(fDate)) // %4
    .arg(formatDisplayDate(tDate)) // %5
    .arg(rowsHtml) // %6
    .arg(QString::number(totalCloseQty, 'f', 2)) // %7
    .arg(formatINR(totalCloseVal)) // %8
    .arg(QDateTime::currentDateTime().toString("dd-MM-yyyy hh:mm AP")); // %9

    return html;
}

// ------------------------------------------------------------------------------------------------
// PUBLIC INVOCABLE CONTROLLER METHODS
// ------------------------------------------------------------------------------------------------
QString PrintExportController::export_sales_invoice_pdf(const QString& invoiceNo, const QString& customPath, const QString& copyType) {
    QString html = renderSalesInvoiceHtml(invoiceNo, copyType);
    QString cleanInv = invoiceNo.trimmed().replace("/", "_").replace("\\", "_");
    QString suffix = "Original";
    QString norm = copyType.trimmed().toUpper();
    if (norm.contains("DUPLICATE") || norm.contains("TRANSPORTER")) suffix = "Duplicate";
    else if (norm.contains("TRIPLICATE") || norm.contains("SUPPLIER")) suffix = "Triplicate";
    else if (norm == "ALL" || norm == "BOTH" || norm == "ORIGINAL + DUPLICATE" || norm == "ALL COPIES") suffix = "AllCopies";

    QString fileName = "Invoice_" + cleanInv + "_" + suffix + ".pdf";
    return exportHtmlToPdf(html, fileName, customPath, false);
}

QString PrintExportController::export_sales_invoice_duplicate_pdf(const QString& invoiceNo, const QString& customPath) {
    return export_sales_invoice_pdf(invoiceNo, customPath, "DUPLICATE FOR TRANSPORTER");
}

QString PrintExportController::export_sales_invoice_all_copies_pdf(const QString& invoiceNo, const QString& customPath) {
    return export_sales_invoice_pdf(invoiceNo, customPath, "ALL");
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

QString PrintExportController::export_purchase_invoice_pdf(const QString& invoiceNo, const QString& customPath) {
    QString html = renderPurchaseInvoiceHtml(invoiceNo);
    QString fileName = "Purchase_" + invoiceNo.trimmed().replace("/", "_").replace("\\", "_") + ".pdf";
    return exportHtmlToPdf(html, fileName, customPath, false);
}

bool PrintExportController::print_purchase_invoice(const QString& invoiceNo) {
    QString html = renderPurchaseInvoiceHtml(invoiceNo);
    return printHtml(html, "Purchase " + invoiceNo, false);
}

QString PrintExportController::export_ledger_statement_pdf(const QString& partyName, const QString& fromDate, const QString& toDate, const QString& customPath) {
    QString html = renderLedgerStatementHtml(partyName, fromDate, toDate);
    QString pClean = partyName.trimmed().replace(" ", "_").replace("/", "_").replace("\\", "_");
    QString fileName = "Ledger_" + pClean + ".pdf";
    return exportHtmlToPdf(html, fileName, customPath, true); // Landscape for 2-column Dr/Cr ledger
}

bool PrintExportController::print_ledger_statement(const QString& partyName, const QString& fromDate, const QString& toDate) {
    QString html = renderLedgerStatementHtml(partyName, fromDate, toDate);
    return printHtml(html, "Ledger " + partyName, true); // Landscape for 2-column Dr/Cr ledger
}

QString PrintExportController::export_stock_register_pdf(const QString& fromDate, const QString& toDate, const QString& customPath) {
    QString html = renderStockRegisterHtml(fromDate, toDate);
    QString fileName = "Stock_Register_" + QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss") + ".pdf";
    return exportHtmlToPdf(html, fileName, customPath, false);
}

bool PrintExportController::print_stock_register(const QString& fromDate, const QString& toDate) {
    QString html = renderStockRegisterHtml(fromDate, toDate);
    return printHtml(html, "Stock Register", false);
}

// ------------------------------------------------------------------------------------------------
// CSV / EXCEL EXPORTS
// ------------------------------------------------------------------------------------------------
QString PrintExportController::export_ledger_csv(const QString& partyName, const QString& fromDate, const QString& toDate, const QString& customPath) {
    QString pName = partyName.trimmed();
    QString fDate = fromDate.trimmed();
    QString tDate = toDate.trimmed();
    if (fDate.isEmpty() && tDate.isEmpty()) {
        fDate = AccountingEngine::getActiveFromDate();
        tDate = AccountingEngine::getActiveToDate();
    }

    // Query party profile
    QVariantList pRows = DatabaseManager::instance().executeQuery(
        "SELECT id, legacy_id, opening_balance, balance_type FROM parties WHERE name = ? OR alias = ? OR name LIKE ? LIMIT 1;",
        {pName, pName, "%" + pName + "%"}
    );
    int partyId = 0;
    int legacyCode = 0;
    double initialOp = 0.0;
    QString initialOpType = "Cr";
    if (!pRows.isEmpty()) {
        partyId = pRows.first().toMap().value("id").toInt();
        legacyCode = pRows.first().toMap().value("legacy_id").toInt();
        initialOp = pRows.first().toMap().value("opening_balance").toDouble();
        initialOpType = pRows.first().toMap().value("balance_type").toString();
    }

    double priorDr = 0.0;
    double priorCr = 0.0;
    if (initialOpType.compare("Dr", Qt::CaseInsensitive) == 0) priorDr += initialOp;
    else priorCr += initialOp;

    if (!fDate.isEmpty() && fDate != "ALL" && fDate != "All") {
        QVariantList priorRows = DatabaseManager::instance().executeQuery(
            "SELECT dr_cr, SUM(amount) as total_amt FROM transactions "
            "WHERE (party_name = ? OR party_name LIKE ? OR party_id = ? OR (account_code > 0 AND account_code = ?)) "
            "AND voucher_date < ? GROUP BY dr_cr;",
            {pName, "%" + pName + "%", partyId, legacyCode, fDate}
        );
        for (const auto& pr : priorRows) {
            QVariantMap m = pr.toMap();
            if (m.value("dr_cr").toString().compare("Dr", Qt::CaseInsensitive) == 0) {
                priorDr += m.value("total_amt").toDouble();
            } else {
                priorCr += m.value("total_amt").toDouble();
            }
        }
    }

    double netOp = priorDr - priorCr;

    QString outPath = customPath.trimmed();
    if (outPath.isEmpty()) {
        outPath = get_default_reports_dir() + "/Ledger_" + pName.replace(" ", "_").replace("/", "_") + ".csv";
    }

    QFile file(outPath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        m_isSuccess = false;
        m_statusMessage = "Cannot open CSV file for writing: " + outPath;
        emit statusMessageChanged();
        return "";
    }

    QTextStream out(&file);
    out << "Date,Voucher No,Voucher Type,Particulars,Debit (₹),Credit (₹)\n";

    double drTot = 0.0, crTot = 0.0;
    if (std::abs(netOp) > 0.001) {
        QString opD = formatDisplayDate(!fDate.isEmpty() && fDate != "ALL" ? fDate : "2025-04-01");
        if (netOp >= 0) {
            drTot += netOp;
            out << opD << ",OP,OBal,\"Opening Balance (Dr)\"," << QString::number(netOp, 'f', 2) << ",0.00\n";
        } else {
            double cAmt = std::abs(netOp);
            crTot += cAmt;
            out << opD << ",OP,OBal,\"Opening Balance (Cr)\",0.00," << QString::number(cAmt, 'f', 2) << "\n";
        }
    }

    QString sql = "SELECT voucher_no, voucher_date, voucher_type, trans_type, opposing_account, dr_cr, amount, invoice_no, narration, broker_name, vehicle_no, tds_amount FROM transactions WHERE (party_name = ? OR party_name LIKE ? OR party_id = ? OR (account_code > 0 AND account_code = ?))";
    QVariantList params = {pName, "%" + pName + "%", partyId, legacyCode};
    if (!fDate.isEmpty() && !tDate.isEmpty() && fDate != "ALL" && fDate != "All") {
        sql += " AND voucher_date >= ? AND voucher_date <= ?";
        params << fDate << tDate;
    }
    sql += " ORDER BY voucher_date ASC, CAST(voucher_no AS INTEGER) ASC, id ASC;";

    QVariantList rows = DatabaseManager::instance().executeQuery(sql, params);

    for (const auto& rVar : rows) {
        QVariantMap t = rVar.toMap();
        QString dt = formatDisplayDate(t.value("voucher_date").toString());
        QString vNo = t.value("voucher_no").toString();
        QString rawType = t.value("trans_type").toString();
        QString vType = t.value("voucher_type").toString();
        QString opposing = t.value("opposing_account").toString();
        QString drCr = t.value("dr_cr").toString();
        double amt = t.value("amount").toDouble();
        QString invNo = t.value("invoice_no").toString();
        QString narr = t.value("narration").toString().trimmed().replace(",", ";").replace("\"", "'");
        QString veh = t.value("vehicle_no").toString().trimmed();
        QString broker = t.value("broker_name").toString().trimmed();

        QString displayRef = !rawType.isEmpty() ? QString("%1 %2").arg(rawType, vNo).trimmed() : QString("%1 %2").arg(vType, vNo).trimmed();
        if (displayRef.isEmpty()) displayRef = vNo;

        QString desc;
        if (rawType == "Sale" || vType == "Sales") desc = QString("Sales Invoice: %1").arg(!invNo.isEmpty() ? invNo : vNo);
        else if (rawType == "Purc" || vType == "Purchase") desc = QString("Purchase Bill: %1").arg(!invNo.isEmpty() ? invNo : vNo);
        else if (rawType == "ChRt" || rawType == "Rcpt" || vType == "Receipt") desc = QString("Receipt via %1").arg(!opposing.isEmpty() ? opposing : "Bank/Cash");
        else if (rawType == "ChPt" || rawType == "Pymt" || vType == "Payment") desc = QString("Payment to %1").arg(!opposing.isEmpty() ? opposing : "Bank/Cash");
        else if (rawType == "TDS" || vType == "TDS") desc = !narr.isEmpty() ? narr : QString("TDS: %1").arg(!opposing.isEmpty() ? opposing : "TDS");
        else desc = QString("%1: %2").arg(vType, opposing);

        if (!veh.isEmpty()) desc += " | Veh: " + veh;
        if (!broker.isEmpty()) desc += " | Broker: " + broker;
        if (!narr.isEmpty() && !desc.contains(narr)) desc += " | " + narr;
        desc.replace(",", ";").replace("\"", "'");

        double tdsAmt = t.value("tds_amount").toDouble();
        if ((rawType == "Purc" || vType == "Purchase") && tdsAmt > 0.0) {
            double grossAmt = amt + tdsAmt;
            crTot += grossAmt;
            out << dt << "," << displayRef << "," << vType << ",\"" << desc << "\",0.00," << QString::number(grossAmt, 'f', 2) << "\n";

            drTot += tdsAmt;
            out << dt << "," << displayRef << ",TDS,\"T.D.S. U/S 194Q (B.No. " << (!invNo.isEmpty() ? invNo : vNo) << ")\"," << QString::number(tdsAmt, 'f', 2) << ",0.00\n";
        } else {
            if (drCr.compare("Dr", Qt::CaseInsensitive) == 0) {
                drTot += amt;
                out << dt << "," << displayRef << "," << vType << ",\"" << desc << "\"," << QString::number(amt, 'f', 2) << ",0.00\n";
            } else {
                crTot += amt;
                out << dt << "," << displayRef << "," << vType << ",\"" << desc << "\",0.00," << QString::number(amt, 'f', 2) << "\n";
            }
        }
    }

    out << ",,,TOTAL," << QString::number(drTot, 'f', 2) << "," << QString::number(crTot, 'f', 2) << "\n";
    out << ",,,NET BALANCE," << QString::number(std::abs(drTot - crTot), 'f', 2) << "," << (drTot >= crTot ? "Dr (Receivable)" : "Cr (Payable)") << "\n";
    file.close();

    m_lastExportedFile = outPath;
    m_isSuccess = true;
    m_statusMessage = "CSV exported: " + outPath;
    emit lastExportedFileChanged();
    emit statusMessageChanged();
    open_file_in_os(outPath);
    return outPath;
}


QString PrintExportController::export_stock_csv(const QString& fromDate, const QString& toDate, const QString& customPath) {
    QVariantList items = DatabaseManager::instance().executeQuery("SELECT * FROM stock_items ORDER BY name COLLATE NOCASE ASC;");

    QString outPath = customPath.trimmed();
    if (outPath.isEmpty()) {
        outPath = get_default_reports_dir() + "/Stock_Register_" + QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss") + ".csv";
    }

    QFile file(outPath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) return "";

    QTextStream out(&file);
    out << "Item Name,Code,Type,Unit,Opening Qtl,Inward Qtl,Outward Qtl,Closing Qtl,Rate (₹),Closing Value (₹)\n";

    for (const auto& var : items) {
        QVariantMap it = var.toMap();
        QString name = it.value("name").toString().replace(",", ";");
        int itemId = it.value("id").toInt();
        QString code = it.value("code").toString();
        QString type = it.value("item_type").toString();
        QString unit = it.value("unit", "Qtl").toString();
        double op = it.value("opening_qty").toDouble();
        double inQty = 0.0, outQty = 0.0;

        QVariant inVal = DatabaseManager::instance().executeScalar(
            "SELECT SUM(weight_qtl) FROM stock_transactions WHERE (item_id = ? OR item_name = ?) AND trans_type IN ('Purc', 'SlRn', 'Inward', 'P', 'M');",
            {itemId, name}
        );
        if (inVal.isValid()) inQty = inVal.toDouble();

        QVariant outVal = DatabaseManager::instance().executeScalar(
            "SELECT SUM(weight_qtl) FROM stock_transactions WHERE (item_id = ? OR item_name = ?) AND trans_type IN ('Sale', 'PrRn', 'Outward', 'S');",
            {itemId, name}
        );
        if (outVal.isValid()) outQty = outVal.toDouble();

        double close = op + inQty - outQty;
        double rate = it.value("rate", it.value("standard_cost", 0.0)).toDouble();
        double val = close * rate;

        out << "\"" << name << "\"," << code << "," << type << "," << unit << ","
            << op << "," << inQty << "," << outQty << "," << close << "," << rate << "," << val << "\n";
    }
    file.close();

    m_lastExportedFile = outPath;
    m_isSuccess = true;
    m_statusMessage = "CSV exported: " + outPath;
    emit lastExportedFileChanged();
    emit statusMessageChanged();
    open_file_in_os(outPath);
    return outPath;
}

QString PrintExportController::export_sales_register_csv(const QString& fromDate, const QString& toDate, const QString& customPath) {
    QString outPath = customPath.trimmed();
    if (outPath.isEmpty()) {
        outPath = get_default_reports_dir() + "/Sales_Register_" + QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss") + ".csv";
    }

    QFile file(outPath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) return "";

    QTextStream out(&file);
    out << "Invoice No,Date,Customer Name,Commodity,Bags,Weight (Qtl),Rate (₹),Taxable (₹),GST (₹),Total Amount (₹),Vehicle No\n";

    QVariantList rows = DatabaseManager::instance().executeQuery("SELECT * FROM sales_invoices ORDER BY invoice_date DESC, id DESC;");
    for (const auto& var : rows) {
        QVariantMap r = var.toMap();
        out << r.value("invoice_no").toString() << ","
            << r.value("invoice_date").toString() << ",\""
            << r.value("customer_name").toString().replace(",", ";") << "\",\""
            << r.value("item_name").toString().replace(",", ";") << "\","
            << r.value("bag_count").toLongLong() << ","
            << r.value("weight_qtl").toDouble() << ","
            << r.value("rate_per_qtl").toDouble() << ","
            << r.value("taxable_amount").toDouble() << ","
            << r.value("gst_amount").toDouble() << ","
            << r.value("total_amount").toDouble() << ","
            << r.value("vehicle_no").toString() << "\n";
    }
    file.close();

    m_lastExportedFile = outPath;
    m_isSuccess = true;
    m_statusMessage = "Sales CSV exported: " + outPath;
    emit lastExportedFileChanged();
    emit statusMessageChanged();
    open_file_in_os(outPath);
    return outPath;
}

QString PrintExportController::export_purchase_register_csv(const QString& fromDate, const QString& toDate, const QString& customPath) {
    QString outPath = customPath.trimmed();
    if (outPath.isEmpty()) {
        outPath = get_default_reports_dir() + "/Purchase_Register_" + QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss") + ".csv";
    }

    QFile file(outPath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) return "";

    QTextStream out(&file);
    out << "Voucher No,Bill No,Date,Supplier Name,Commodity,Bags,Weight (Qtl),Rate (₹),Taxable (₹),GST (₹),Total Bill (₹),Vehicle No\n";

    QVariantList rows = DatabaseManager::instance().executeQuery("SELECT * FROM purchase_invoices ORDER BY invoice_date DESC, id DESC;");
    for (const auto& var : rows) {
        QVariantMap r = var.toMap();
        out << r.value("voucher_no").toString() << ","
            << r.value("invoice_no").toString() << ","
            << r.value("invoice_date").toString() << ",\""
            << r.value("supplier_name").toString().replace(",", ";") << "\",\""
            << r.value("item_name").toString().replace(",", ";") << "\","
            << r.value("bag_count").toLongLong() << ","
            << r.value("weight_qtl").toDouble() << ","
            << r.value("rate_per_qtl").toDouble() << ","
            << r.value("taxable_amount").toDouble() << ","
            << r.value("gst_amount").toDouble() << ","
            << r.value("total_amount").toDouble() << ","
            << r.value("vehicle_no").toString() << "\n";
    }
    file.close();

    m_lastExportedFile = outPath;
    m_isSuccess = true;
    m_statusMessage = "Purchase CSV exported: " + outPath;
    emit lastExportedFileChanged();
    emit statusMessageChanged();
    open_file_in_os(outPath);
    return outPath;
}
