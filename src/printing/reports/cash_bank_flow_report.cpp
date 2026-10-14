#include "cash_bank_flow_report.h"
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

QString renderCashBankFlowHtml(int flowType, const QString& fromDate, const QString& toDate) {
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
            outRowsHtml += QString("<tr><td bgcolor='#FFFFFF'>%1</td><td align='right' bgcolor='#FFFFFF'>%2</td></tr>")
                .arg(pName.toHtmlEscaped(), formatINR(amt));
        } else {
            totIn += amt;
            inRowsHtml += QString("<tr><td bgcolor='#FFFFFF'>%1</td><td align='right' bgcolor='#FFFFFF'>%2</td></tr>")
                .arg(pName.toHtmlEscaped(), formatINR(amt));
        }
    }

    QString body = ErpPrint::letterheadHtml(firm)
        + ErpPrint::titleBlockHtml(title,
            QString("Period: %1 to %2")
                .arg(FiscalYearHelper::formatDisplayDate(fDate),
                     FiscalYearHelper::formatDisplayDate(tDate)))
        + QString(
        "<table width='100%' border='0' cellspacing='1' cellpadding='4' bgcolor='#94A3B8' style='font-size:9pt'>"
        "  <tr>"
        "    <th width='50%' bgcolor='#0F172A'><font color='#FFFFFF'><b>RECEIPTS / INFLOW (CR)</b></font></th>"
        "    <th width='50%' bgcolor='#0F172A'><font color='#FFFFFF'><b>PAYMENTS / OUTFLOW (DR)</b></font></th>"
        "  </tr>"
        "  <tr>"
        "    <td valign='top' bgcolor='#FFFFFF'><table width='100%' border='0' cellspacing='1' cellpadding='4' bgcolor='#94A3B8' style='font-size:9pt'>%1"
        "<tr bgcolor='#F1F5F9'><td bgcolor='#F1F5F9'>Total Inflow</td><td align='right' bgcolor='#F1F5F9'>%2</td></tr></table></td>"
        "    <td valign='top' bgcolor='#FFFFFF'><table width='100%' border='0' cellspacing='1' cellpadding='4' bgcolor='#94A3B8' style='font-size:9pt'>%3"
        "<tr bgcolor='#F1F5F9'><td bgcolor='#F1F5F9'>Total Outflow</td><td align='right' bgcolor='#F1F5F9'>%4</td></tr></table></td>"
        "  </tr>"
        "</table>")
        .arg(inRowsHtml)
        .arg(formatINR(totIn))
        .arg(outRowsHtml)
        .arg(formatINR(totOut))
        + ErpPrint::signatoryHtml(firm.value("firm_name").toString())
        + ErpPrint::generatedFooterHtml();

    return ErpPrint::wrapPrintDocument(body);
}


} // namespace PrintReports
