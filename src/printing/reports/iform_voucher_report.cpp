#include "iform_voucher_report.h"
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

QString renderIFormVoucherHtml(const QString& voucherNo) {
    auto firm = getFirmProfile();

    QVariantList iRows = DatabaseManager::instance().executeQuery(
        "SELECT * FROM iform_vouchers WHERE voucher_no = ? OR id = ? LIMIT 1;",
        {voucherNo.trimmed(), voucherNo.trimmed()}
    );
    if (iRows.isEmpty()) {
        return "<h2>I-Form voucher not found: " + voucherNo + "</h2>";
    }

    QVariantMap i = iRows.first().toMap();
    QString vDate = FiscalYearHelper::formatDisplayDate(i.value("voucher_date").toString());
    QString buyer = i.value("buyer_name").toString();
    QString variety = i.value("item_name", i.value("commodity")).toString();
    long long bags = i.value("bag_count").toLongLong();
    double wt = i.value("weight_qtl").toDouble();
    double rate = i.value("rate_per_qtl").toDouble();
    double totalAmt = i.value("total_amount", wt * rate).toDouble();

    QString body = ErpPrint::letterheadHtml(firm)
        + ErpPrint::titleBlockHtml("FORM 'I' - SALE VOUCHER / MEMO (MANDI)",
            QString("I-Form No: %1  |  Date: %2").arg(voucherNo.trimmed(), vDate))
        + QString(
        "<table width='100%' border='0' cellspacing='1' cellpadding='6' bgcolor='#94A3B8'>"
        "  <tr>"
        "    <td colspan='2' bgcolor='#FFFFFF'><b>Buyer / Trader:</b><br><font size='3'><b>%1</b></font></td>"
        "  </tr>"
        "</table>"
        "<table width='100%' border='0' cellspacing='1' cellpadding='5' bgcolor='#94A3B8'>"
        "  <tr>"
        "    <th bgcolor='#0F172A'><font color='#FFFFFF'><b>Commodity / Variety</b></font></th>"
        "    <th bgcolor='#0F172A'><font color='#FFFFFF'><b>Bags</b></font></th>"
        "    <th bgcolor='#0F172A'><font color='#FFFFFF'><b>Weight (Qtl)</b></font></th>"
        "    <th bgcolor='#0F172A'><font color='#FFFFFF'><b>Rate (Rs./Qtl)</b></font></th>"
        "    <th bgcolor='#0F172A'><font color='#FFFFFF'><b>Total Amount (Rs.)</b></font></th>"
        "  </tr>"
        "  <tr>"
        "    <td bgcolor='#FFFFFF'><b>%2</b></td>"
        "    <td align='center' bgcolor='#FFFFFF'>%3</td>"
        "    <td align='right' bgcolor='#FFFFFF'>%4</td>"
        "    <td align='right' bgcolor='#FFFFFF'>%5</td>"
        "    <td align='right' bgcolor='#FFFFFF'><b>%6</b></td>"
        "  </tr>"
        "</table>"
        "<table width='100%' border='0' cellspacing='1' cellpadding='6' bgcolor='#94A3B8'>"
        "  <tr>"
        "    <td width='65%' bgcolor='#FFFFFF'><b>Amount in Words:</b><br><span class='words'>%7</span></td>"
        "    <td width='35%' align='right' bgcolor='#FFFFFF'><b>TOTAL AMOUNT:<br><font size='3'>%6</font></b></td>"
        "  </tr>"
        "</table>")
        .arg(buyer.toHtmlEscaped())
        .arg(variety.toHtmlEscaped())
        .arg(bags)
        .arg(QString::number(wt, 'f', 2))
        .arg(QString::number(rate, 'f', 2))
        .arg(formatINR(totalAmt))
        .arg(amountInWords(totalAmt).toHtmlEscaped())
        + ErpPrint::voucherSignHtml(QStringList() << "Buyer's Signature" << ErpPrint::forFirmLabel(firm));

    return ErpPrint::wrapPrintDocument(body);
}


} // namespace PrintReports
