#include "jform_voucher_report.h"
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

QString renderJFormVoucherHtml(const QString& voucherNo) {
    auto firm = getFirmProfile();

    QVariantList jRows = DatabaseManager::instance().executeQuery(
        "SELECT * FROM jform_vouchers WHERE voucher_no = ? OR id = ? LIMIT 1;",
        {voucherNo.trimmed(), voucherNo.trimmed()}
    );
    if (jRows.isEmpty()) {
        return "<h2>J-Form voucher not found: " + voucherNo + "</h2>";
    }

    QVariantMap j = jRows.first().toMap();
    QString vDate = FiscalYearHelper::formatDisplayDate(j.value("voucher_date").toString());
    QString farmer = j.value("farmer_name").toString();
    QString commAgent = j.value("broker_name", j.value("commission_agent")).toString();
    QString variety = j.value("item_name", j.value("commodity")).toString();
    long long bags = j.value("bag_count").toLongLong();
    double wt = j.value("weight_qtl").toDouble();
    double rate = j.value("rate_per_qtl").toDouble();
    double grossAmt = j.value("gross_amount", wt * rate).toDouble();
    double netAmt = j.value("total_amount", grossAmt).toDouble();

    QString body = ErpPrint::letterheadHtml(firm)
        + ErpPrint::titleBlockHtml("FORM 'J' - PADDY PURCHASE VOUCHER (MANDI)",
            QString("J-Form No: %1  |  Date: %2").arg(voucherNo.trimmed(), vDate))
        + QString(
        "<table width='100%' border='0' cellspacing='1' cellpadding='6' bgcolor='#94A3B8'>"
        "  <tr>"
        "    <td width='50%' bgcolor='#FFFFFF'><b>Farmer / Seller:</b><br><font size='3'><b>%1</b></font></td>"
        "    <td width='50%' bgcolor='#FFFFFF'><b>Commission Agent / Kaccha Arhtia:</b><br><b>%2</b></td>"
        "  </tr>"
        "</table>"
        "<table width='100%' border='0' cellspacing='1' cellpadding='5' bgcolor='#94A3B8'>"
        "  <tr>"
        "    <th bgcolor='#0F172A'><font color='#FFFFFF'><b>Commodity / Variety</b></font></th>"
        "    <th bgcolor='#0F172A'><font color='#FFFFFF'><b>Bags</b></font></th>"
        "    <th bgcolor='#0F172A'><font color='#FFFFFF'><b>Weight (Qtl)</b></font></th>"
        "    <th bgcolor='#0F172A'><font color='#FFFFFF'><b>Rate (Rs./Qtl)</b></font></th>"
        "    <th bgcolor='#0F172A'><font color='#FFFFFF'><b>Gross Amount (Rs.)</b></font></th>"
        "    <th bgcolor='#0F172A'><font color='#FFFFFF'><b>Net Payable (Rs.)</b></font></th>"
        "  </tr>"
        "  <tr>"
        "    <td bgcolor='#FFFFFF'><b>%3</b></td>"
        "    <td align='center' bgcolor='#FFFFFF'>%4</td>"
        "    <td align='right' bgcolor='#FFFFFF'>%5</td>"
        "    <td align='right' bgcolor='#FFFFFF'>%6</td>"
        "    <td align='right' bgcolor='#FFFFFF'>%7</td>"
        "    <td align='right' bgcolor='#FFFFFF'><b>%8</b></td>"
        "  </tr>"
        "</table>"
        "<table width='100%' border='0' cellspacing='1' cellpadding='6' bgcolor='#94A3B8'>"
        "  <tr>"
        "    <td width='65%' bgcolor='#FFFFFF'><b>Amount in Words:</b><br><span class='words'>%9</span></td>"
        "    <td width='35%' align='right' bgcolor='#FFFFFF'><b>NET PAYABLE:<br><font size='3'>%8</font></b></td>"
        "  </tr>"
        "</table>")
        .arg(farmer.toHtmlEscaped())
        .arg((commAgent.isEmpty() ? "Direct Mandi" : commAgent).toHtmlEscaped())
        .arg(variety.toHtmlEscaped())
        .arg(bags)
        .arg(QString::number(wt, 'f', 2))
        .arg(QString::number(rate, 'f', 2))
        .arg(formatINR(grossAmt))
        .arg(formatINR(netAmt))
        .arg(amountInWords(netAmt).toHtmlEscaped())
        + ErpPrint::voucherSignHtml(QStringList() << "Farmer's Signature" << ErpPrint::forFirmLabel(firm));

    return ErpPrint::wrapPrintDocument(body);
}


} // namespace PrintReports
