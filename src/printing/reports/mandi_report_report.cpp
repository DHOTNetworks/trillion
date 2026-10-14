#include "mandi_report_report.h"
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

QString renderMandiReportHtml(const QString& reportType, const QString& fromDate, const QString& toDate) {
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
            "<td align='center' bgcolor='#FFFFFF'>%1</td>"
            "<td align='center' bgcolor='#FFFFFF'>%2</td>"
            "<td align='center' bgcolor='#FFFFFF'>%3</td>"
            "<td bgcolor='#FFFFFF'>%4</td>"
            "<td bgcolor='#FFFFFF'>%5</td>"
            "<td align='center' bgcolor='#FFFFFF'>%6</td>"
            "<td align='right' bgcolor='#FFFFFF'>%7</td>"
            "<td align='right' bgcolor='#FFFFFF'>%8</td>"
            "<td align='right' bgcolor='#FFFFFF'>%9</td>"
            "<td align='right' bgcolor='#FFFFFF'>%10</td>"
            "</tr>"
        )
        .arg(rowIdx++)
        .arg(FiscalYearHelper::formatDisplayDate(j.value("voucher_date").toString()))
        .arg(j.value("voucher_no").toString().toHtmlEscaped())
        .arg(j.value("farmer_name").toString().toHtmlEscaped())
        .arg(j.value("item_name", "Paddy PR106").toString().toHtmlEscaped())
        .arg(bags)
        .arg(QString::number(wt, 'f', 2))
        .arg(formatINR(amt))
        .arg(formatINR(mfee))
        .arg(formatINR(rdf));
    }

    QString body = ErpPrint::letterheadHtml(firm)
        + ErpPrint::titleBlockHtml(QString("%1 - STATUTORY MANDI REPORT").arg(reportType.toUpper()),
            QString("Period: %1 to %2")
                .arg(FiscalYearHelper::formatDisplayDate(fDate),
                     FiscalYearHelper::formatDisplayDate(tDate)))
        + QString(
        "<table width='100%' border='0' cellspacing='1' cellpadding='4' bgcolor='#94A3B8' style='font-size:9pt'>"
        "  <tr>"
        "    <th width='4%' bgcolor='#0F172A'><font color='#FFFFFF'><b>S.N.</b></font></th>"
        "    <th width='9%' bgcolor='#0F172A'><font color='#FFFFFF'><b>Date</b></font></th>"
        "    <th width='10%' bgcolor='#0F172A'><font color='#FFFFFF'><b>Form No</b></font></th>"
        "    <th width='24%' bgcolor='#0F172A'><font color='#FFFFFF'><b>Farmer / Seller</b></font></th>"
        "    <th width='15%' bgcolor='#0F172A'><font color='#FFFFFF'><b>Variety</b></font></th>"
        "    <th width='6%' bgcolor='#0F172A'><font color='#FFFFFF'><b>Bags</b></font></th>"
        "    <th width='8%' bgcolor='#0F172A'><font color='#FFFFFF'><b>Weight Qtl</b></font></th>"
        "    <th width='10%' bgcolor='#0F172A'><font color='#FFFFFF'><b>Value (Rs.)</b></font></th>"
        "    <th width='7%' bgcolor='#0F172A'><font color='#FFFFFF'><b>M.Fee (2%)</b></font></th>"
        "    <th width='7%' bgcolor='#0F172A'><font color='#FFFFFF'><b>RDF (2%)</b></font></th>"
        "  </tr>"
        "  %1"
        "  <tr bgcolor='#F1F5F9'>"
        "    <td colspan='5' align='right' bgcolor='#F1F5F9'>TOTAL</td>"
        "    <td align='center' bgcolor='#F1F5F9'>%2</td>"
        "    <td align='right' bgcolor='#F1F5F9'>%3</td>"
        "    <td align='right' bgcolor='#F1F5F9'>%4</td>"
        "    <td align='right' bgcolor='#F1F5F9'>%5</td>"
        "    <td align='right' bgcolor='#F1F5F9'>%6</td>"
        "  </tr>"
        "</table>")
        .arg(rowsHtml)
        .arg(totalBags)
        .arg(QString::number(totalWeight, 'f', 2))
        .arg(formatINR(totalAmt))
        .arg(formatINR(totalMfee))
        .arg(formatINR(totalRdf))
        + ErpPrint::signatoryHtml(firm.value("firm_name").toString())
        + ErpPrint::generatedFooterHtml();

    return ErpPrint::wrapPrintDocument(body);
}


} // namespace PrintReports
