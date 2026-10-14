#include "trial_balance_report.h"
#include "report_format.h"
#include "report_data.h"
#include "report_html_chrome.h"
#include "statement_data.h"
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

QString renderTrialBalanceHtml(const QString& asOnDate, int mode) {
    Q_UNUSED(mode);
    auto firm = getFirmProfile();

    QString dateIso = asOnDate.trimmed().isEmpty() ? QDate::currentDate().toString("yyyy-MM-dd") : asOnDate.trimmed();

    // Centralized resolver (statement_data.h): single GROUP BY, never N+1.
    double totalDr = 0.0, totalCr = 0.0;
    const QVector<StatementData::TrialRow> trows = StatementData::trialRows(dateIso, &totalDr, &totalCr);

    QString rowsHtml = "";
    rowsHtml.reserve(trows.size() * 150 + 1024);
    int rowIdx = 1;

    for (const StatementData::TrialRow& r : trows) {
        rowsHtml += QString(
            "<tr>"
            "<td align='center' bgcolor='#FFFFFF'>%1</td>"
            "<td bgcolor='#FFFFFF'><b>%2</b></td>"
            "<td bgcolor='#FFFFFF'>%3</td>"
            "<td align='right' bgcolor='#FFFFFF'>%4</td>"
            "<td align='right' bgcolor='#FFFFFF'>%5</td>"
            "</tr>"
        )
        .arg(rowIdx++)
        .arg(r.account.toHtmlEscaped())
        .arg(r.group.toHtmlEscaped())
        .arg(r.closeDebit > 0 ? formatINR(r.closeDebit) : "-")
        .arg(r.closeCredit > 0 ? formatINR(r.closeCredit) : "-");
    }

    const double diff = totalDr - totalCr;
    const bool balanced = std::abs(diff) < 0.01;

    // CA-firm stationery: letterhead + title + bordered grid + ruled totals
    // + difference row + signatures. Query/figures logic above is untouched.
    QString body = ErpPrint::letterheadHtml(firm)
        + ErpPrint::titleBlockHtml("TRIAL BALANCE",
            QString("As on %1").arg(FiscalYearHelper::formatDisplayDate(dateIso)))
        + "<table width='100%' border='0' cellspacing='1' cellpadding='4' bgcolor='#94A3B8' style='font-size:9pt'>"
          "<tr>"
          "<th width='6%' bgcolor='#0F172A'><font color='#FFFFFF'><b>S.N.</b></font></th>"
          "<th width='44%' bgcolor='#0F172A'><font color='#FFFFFF'><b>Account Name</b></font></th>"
          "<th width='26%' bgcolor='#0F172A'><font color='#FFFFFF'><b>Account Group</b></font></th>"
          "<th width='12%' bgcolor='#0F172A'><font color='#FFFFFF'><b>Debit Closing (Rs.)</b></font></th>"
          "<th width='12%' bgcolor='#0F172A'><font color='#FFFFFF'><b>Credit Closing (Rs.)</b></font></th>"
          "</tr>"
        + rowsHtml
        + QString(
            "<tr bgcolor='#F1F5F9'>"
            "<td colspan='3' align='right' bgcolor='#F1F5F9'>TOTAL</td>"
            "<td align='right' bgcolor='#F1F5F9'>%1</td>"
            "<td align='right' bgcolor='#F1F5F9'>%2</td>"
            "</tr>")
            .arg(formatINR(totalDr))
            .arg(formatINR(totalCr));
    if (!balanced) {
        body += QString(
            "<tr bgcolor='#F1F5F9'>"
            "<td colspan='3' align='right' bgcolor='#F1F5F9'><font color='#DC2626'>DIFFERENCE (Dr − Cr)</font></td>"
            "<td align='right' colspan='2' bgcolor='#F1F5F9'><font color='#DC2626'>%1</font></td>"
            "</tr>").arg(formatINR(diff));
    }
    body += QString("</table>"
        "<div class='notes'>Status: %1 &nbsp;|&nbsp; %2 ledger account(s) with non-zero closing balance.</div>")
        .arg(balanced ? "Balanced (difference Rs. 0.00)" : ("Out of balance by " + formatINR(diff)),
             QString::number(rowIdx - 1))
        + ErpPrint::signatoryHtml(firm.value("firm_name").toString())
        + ErpPrint::generatedFooterHtml();

    return ErpPrint::wrapPrintDocument(body);
}


} // namespace PrintReports
