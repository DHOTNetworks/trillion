#include "interest_statement_report.h"
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

QString renderInterestCalculationHtml(const QString& partyName, const QString& fromDate, const QString& toDate, double annualRate) {
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
            "<td align='center' bgcolor='#FFFFFF'>%1</td>"
            "<td align='center' bgcolor='#FFFFFF'>%2</td>"
            "<td align='center' bgcolor='#FFFFFF'>%3</td>"
            "<td align='right' bgcolor='#FFFFFF'>%4</td>"
            "<td align='center' bgcolor='#FFFFFF'>%5</td>"
            "<td align='right' bgcolor='#FFFFFF'>%6</td>"
            "<td align='right' bgcolor='#FFFFFF'>%7</td>"
            "</tr>"
        )
        .arg(rowIdx++)
        .arg(FiscalYearHelper::formatDisplayDate(t.value("voucher_date").toString()))
        .arg(t.value("voucher_no").toString().toHtmlEscaped())
        .arg(formatINR(amt))
        .arg(days)
        .arg(formatINR(intAmt))
        .arg(formatINR(runningBal));

        prevDate = curDate;
    }

    QString body = ErpPrint::letterheadHtml(firm)
        + ErpPrint::titleBlockHtml("INTEREST CALCULATION STATEMENT (AANK)",
            QString("Party: %1  |  Rate: %2% p.a.  |  Period: %3 to %4")
                .arg(partyName.trimmed(),
                     QString::number(annualRate, 'f', 2),
                     FiscalYearHelper::formatDisplayDate(fDate),
                     FiscalYearHelper::formatDisplayDate(tDate)))
        + QString(
        "<table width='100%' border='0' cellspacing='1' cellpadding='4' bgcolor='#94A3B8' style='font-size:9pt'>"
        "  <tr>"
        "    <th width='5%' bgcolor='#0F172A'><font color='#FFFFFF'><b>S.N.</b></font></th>"
        "    <th width='12%' bgcolor='#0F172A'><font color='#FFFFFF'><b>Date</b></font></th>"
        "    <th width='15%' bgcolor='#0F172A'><font color='#FFFFFF'><b>Voucher No</b></font></th>"
        "    <th width='18%' bgcolor='#0F172A'><font color='#FFFFFF'><b>Transaction (Rs.)</b></font></th>"
        "    <th width='10%' bgcolor='#0F172A'><font color='#FFFFFF'><b>Days</b></font></th>"
        "    <th width='20%' bgcolor='#0F172A'><font color='#FFFFFF'><b>Interest (Rs.)</b></font></th>"
        "    <th width='20%' bgcolor='#0F172A'><font color='#FFFFFF'><b>Balance (Rs.)</b></font></th>"
        "  </tr>"
        "  %1"
        "  <tr bgcolor='#F1F5F9'>"
        "    <td colspan='5' align='right' bgcolor='#F1F5F9'>TOTAL ACCRUED INTEREST</td>"
        "    <td align='right' bgcolor='#F1F5F9'>%2</td>"
        "    <td align='right' bgcolor='#F1F5F9'>%3</td>"
        "  </tr>"
        "</table>")
        .arg(rowsHtml)
        .arg(formatINR(totalInterest))
        .arg(formatINR(runningBal))
        + ErpPrint::signatoryHtml(firm.value("firm_name").toString())
        + ErpPrint::generatedFooterHtml();

    return ErpPrint::wrapPrintDocument(body);
}


} // namespace PrintReports
