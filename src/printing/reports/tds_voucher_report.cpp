#include "tds_voucher_report.h"
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

QString renderTdsVoucherHtml(const QString& voucherNo) {
    auto firm = getFirmProfile();

    QVariantList rows = DatabaseManager::instance().executeQuery(
        "SELECT * FROM tds_vouchers WHERE voucher_no = ? OR id = ? LIMIT 1;",
        {voucherNo.trimmed(), voucherNo.trimmed()}
    );
    if (rows.isEmpty()) {
        return "<h2>TDS voucher not found: " + voucherNo + "</h2>";
    }

    QVariantMap r = rows.first().toMap();
    QString vDate = FiscalYearHelper::formatDisplayDate(r.value("voucher_date").toString());
    QString pName = r.value("party_name").toString();
    QString pan = r.value("pan").toString();
    QString sec = r.value("section").toString();
    double gross = r.value("gross_amount").toDouble();
    double rate = r.value("tds_rate").toDouble();
    double tdsAmt = r.value("tds_amount").toDouble();

    QString body = ErpPrint::letterheadHtml(firm)
        + ErpPrint::titleBlockHtml("TDS DEDUCTION CERTIFICATE / SLIP",
            QString("Voucher No: %1  |  Date: %2").arg(voucherNo.trimmed(), vDate))
        + QString(
        "<table width='100%' border='0' cellspacing='1' cellpadding='6' bgcolor='#94A3B8'>"
        "  <tr>"
        "    <td width='50%' bgcolor='#FFFFFF'><b>Deductee / Party:</b><br><font size='3'><b>%1</b></font></td>"
        "    <td width='50%' bgcolor='#FFFFFF'><b>PAN:</b> %2<br><b>Section:</b> %3</td>"
        "  </tr>"
        "  <tr>"
        "    <td bgcolor='#FFFFFF'><b>Gross Payment Amount:</b><br>%4</td>"
        "    <td bgcolor='#FFFFFF'><b>TDS Rate:</b> %5% &nbsp;|&nbsp; <b>TDS Deducted:</b><br><font size='3'><b>%6</b></font></td>"
        "  </tr>"
        "</table>")
        .arg(pName.toHtmlEscaped())
        .arg(pan.toHtmlEscaped())
        .arg(sec.toHtmlEscaped())
        .arg(formatINR(gross))
        .arg(QString::number(rate, 'f', 2))
        .arg(formatINR(tdsAmt))
        + ErpPrint::voucherSignHtml(QStringList() << "Prepared By" << ErpPrint::forFirmLabel(firm));

    return ErpPrint::wrapPrintDocument(body);
}


} // namespace PrintReports
