#include "gstr_report_report.h"
#include "report_format.h"
#include "report_data.h"
#include "report_html_chrome.h"
#include "../../database_manager.h"
#include "../../engine/accounting_engine.h"
#include "../../engine/fiscal_year_helper.h"
#include "../../engine/profit_loss_calculator.h"
#include "../../engine/balance_sheet_calculator.h"
#include "../../engine/stock_valuation_engine.h"
#include "../../models/account_classifier.h"
#include <QString>
#include <QVariant>
#include <QVariantMap>
#include <QVariantList>
#include <QDate>
#include <QDateTime>
#include <QHash>
#include <QRegularExpression>
#include <QDebug>
#include <cmath>
#include <algorithm>

namespace PrintReports {

QString renderGstrReportHtml(const QString& gstrType, const QString& fromDate, const QString& toDate) {
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

    QString body = ErpPrint::letterheadHtml(firm)
        + ErpPrint::titleBlockHtml(QString("STATUTORY GST REPORT - %1").arg(gstrType.toUpper()),
            QString("Period: %1 to %2")
                .arg(FiscalYearHelper::formatDisplayDate(fDate),
                     FiscalYearHelper::formatDisplayDate(tDate)))
        + QString(
        "<table width='100%' border='0' cellspacing='1' cellpadding='4' bgcolor='#94A3B8' style='font-size:9pt'>"
        "  <tr>"
        "    <th bgcolor='#0F172A'><font color='#FFFFFF'><b>Section / Table</b></font></th>"
        "    <th bgcolor='#0F172A'><font color='#FFFFFF'><b>Invoice Count</b></font></th>"
        "    <th bgcolor='#0F172A'><font color='#FFFFFF'><b>Taxable Value (Rs.)</b></font></th>"
        "    <th bgcolor='#0F172A'><font color='#FFFFFF'><b>CGST (Rs.)</b></font></th>"
        "    <th bgcolor='#0F172A'><font color='#FFFFFF'><b>SGST (Rs.)</b></font></th>"
        "    <th bgcolor='#0F172A'><font color='#FFFFFF'><b>IGST (Rs.)</b></font></th>"
        "    <th bgcolor='#0F172A'><font color='#FFFFFF'><b>Total Value (Rs.)</b></font></th>"
        "  </tr>"
        "  <tr>"
        "    <td bgcolor='#FFFFFF'><b>4A - B2B Regular Supplies</b></td>"
        "    <td align='center' bgcolor='#FFFFFF'>%1</td>"
        "    <td align='right' bgcolor='#FFFFFF'>%2</td>"
        "    <td align='right' bgcolor='#FFFFFF'>%3</td>"
        "    <td align='right' bgcolor='#FFFFFF'>%4</td>"
        "    <td align='right' bgcolor='#FFFFFF'>%5</td>"
        "    <td align='right' bgcolor='#FFFFFF'><b>%6</b></td>"
        "  </tr>"
        "  <tr bgcolor='#F1F5F9'>"
        "    <td bgcolor='#F1F5F9'>TOTAL OUTWARD SUPPLIES</td>"
        "    <td align='center' bgcolor='#F1F5F9'>%1</td>"
        "    <td align='right' bgcolor='#F1F5F9'>%2</td>"
        "    <td align='right' bgcolor='#F1F5F9'>%3</td>"
        "    <td align='right' bgcolor='#F1F5F9'>%4</td>"
        "    <td align='right' bgcolor='#F1F5F9'>%5</td>"
        "    <td align='right' bgcolor='#F1F5F9'>%6</td>"
        "  </tr>"
        "</table>")
        .arg(sales.size())
        .arg(formatINR(b2bTaxable))
        .arg(formatINR(b2bCgst))
        .arg(formatINR(b2bSgst))
        .arg(formatINR(b2bIgst))
        .arg(formatINR(b2bTotal))
        + ErpPrint::signatoryHtml(firm.value("firm_name").toString())
        + ErpPrint::generatedFooterHtml();

    return ErpPrint::wrapPrintDocument(body);
}


} // namespace PrintReports
