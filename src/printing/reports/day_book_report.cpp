#include "day_book_report.h"
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

QString renderDayBookHtml(const QString& fromDate, const QString& toDate) {
    auto firm = getFirmProfile();

    QString fDate = fromDate.trimmed();
    QString tDate = toDate.trimmed();
    if (fDate.isEmpty()) fDate = QDate::currentDate().toString("yyyy-MM-dd");
    if (tDate.isEmpty()) tDate = fDate;

    // One row per voucher (statement_data.h): the original entry up front,
    // counter legs folded into Opposing Account. Same indexed scan as legs;
    // debit/credit are voucher-side sums so day/grand totals are unchanged.
    // A4-bound: render at most the latest MaxVouchers (full FY = 11k+
    // vouchers / 539 pages otherwise); grand totals always cover the full
    // period via dayBookTotals so money never lies.
    const QVector<StatementData::Voucher> allVouchers = StatementData::dayBookVouchers(fDate, tDate);
    double totalDr = 0.0, totalCr = 0.0;
    StatementData::dayBookTotals(fDate, tDate, &totalDr, &totalCr);

    static const int MaxVouchers = 2000;
    const int totalCount = allVouchers.size();
    const int skipCount = qMax(0, totalCount - MaxVouchers);
    QVector<StatementData::Voucher> vouchers;
    vouchers.reserve(qMin(totalCount, MaxVouchers));
    for (int i = skipCount; i < totalCount; ++i) vouchers.append(allVouchers.at(i));

    QString scopeNote = (skipCount > 0)
        ? QString("  |  Showing latest %1 of %2 vouchers").arg(vouchers.size()).arg(totalCount)
        : QString();
    QString body = ErpPrint::letterheadHtml(firm)
        + ErpPrint::titleBlockHtml("DAY BOOK",
            QString("Period: %1 to %2  |  %3 vouchers%4")
                .arg(FiscalYearHelper::formatDisplayDate(fDate),
                     FiscalYearHelper::formatDisplayDate(tDate),
                     QString::number(totalCount), scopeNote));
    body.reserve(body.size() + vouchers.size() * 220 + 4096);

    QString curDay;
    QString dayRows;
    double dayDr = 0.0, dayCr = 0.0;
    int dayCount = 0, rowIdx = 1;
    auto flushDay = [&]() {
        if (curDay.isEmpty()) return;
        const QDate d = QDate::fromString(curDay, "yyyy-MM-dd");
        body += QString("<div class='dayband'>%1 &nbsp;|&nbsp; %2 vouchers</div>"
                        "<table width='100%' border='0' cellspacing='1' cellpadding='4' bgcolor='#94A3B8' style='font-size:9pt'>"
                        "<tr><th width='5%' style='white-space:nowrap' bgcolor='#0F172A'><font color='#FFFFFF'><b>S.N.</b></font></th>"
                        "<th width='14%' style='white-space:nowrap' bgcolor='#0F172A'><font color='#FFFFFF'><b>Voucher No</b></font></th>"
                        "<th width='9%' style='white-space:nowrap' bgcolor='#0F172A'><font color='#FFFFFF'><b>Type</b></font></th>"
                        "<th width='28%' bgcolor='#0F172A'><font color='#FFFFFF'><b>Particulars</b></font></th>"
                        "<th width='24%' bgcolor='#0F172A'><font color='#FFFFFF'><b>Opposing Account</b></font></th>"
                        "<th width='10%' style='white-space:nowrap' bgcolor='#0F172A'><font color='#FFFFFF'><b>Debit (Rs.)</b></font></th>"
                        "<th width='10%' style='white-space:nowrap' bgcolor='#0F172A'><font color='#FFFFFF'><b>Credit (Rs.)</b></font></th></tr>")
                    .arg((d.isValid() ? d.toString("dddd, dd-MM-yyyy") : curDay).toHtmlEscaped())
                    .arg(dayCount);
        body += dayRows;
        body += QString("<tr bgcolor='#F1F5F9'><td colspan='5' align='right' bgcolor='#F1F5F9'>Day Total</td>"
                        "<td align='right' bgcolor='#F1F5F9'>%1</td><td align='right' bgcolor='#F1F5F9'>%2</td></tr></table>")
                    .arg(formatINR(dayDr)).arg(formatINR(dayCr));
        dayRows.clear();
        dayDr = 0.0; dayCr = 0.0; dayCount = 0;
    };

    for (const StatementData::Voucher& v : vouchers) {
        if (v.date != curDay) {
            flushDay();
            curDay = v.date;
        }
        dayRows += QString("<tr><td align='center' bgcolor='#FFFFFF'>%1</td><td align='center' bgcolor='#FFFFFF'>%2</td><td bgcolor='#FFFFFF'>%3</td>"
                           "<td bgcolor='#FFFFFF'><b>%4</b></td><td bgcolor='#FFFFFF'>%5</td>"
                           "<td align='right' bgcolor='#FFFFFF'>%6</td><td align='right' bgcolor='#FFFFFF'>%7</td></tr>")
                       .arg(rowIdx++)
                       .arg(v.voucherNo.toHtmlEscaped())
                       .arg(v.voucherType.toHtmlEscaped())
                       .arg(v.primary.toHtmlEscaped())
                       .arg(v.opposing.isEmpty() ? "-" : v.opposing.toHtmlEscaped())
                       .arg(v.debit > 0 ? formatINR(v.debit) : "-")
                       .arg(v.credit > 0 ? formatINR(v.credit) : "-");
        dayDr += v.debit; dayCr += v.credit; ++dayCount;
    }
    flushDay();

    body += QString("<table width='100%' border='0' cellspacing='1' cellpadding='4' bgcolor='#94A3B8' style='font-size:9pt'>"
                    "<tr bgcolor='#F1F5F9'><td colspan='5' align='right' bgcolor='#F1F5F9'>GRAND TOTAL (%1 to %2)</td>"
                    "<td align='right' bgcolor='#F1F5F9'>%3</td><td align='right' bgcolor='#F1F5F9'>%4</td></tr></table>")
                .arg(FiscalYearHelper::formatDisplayDate(fDate),
                     FiscalYearHelper::formatDisplayDate(tDate),
                     formatINR(totalDr), formatINR(totalCr));
    body += ErpPrint::signatoryHtml(firm.value("firm_name").toString()) + ErpPrint::generatedFooterHtml();
    return ErpPrint::wrapPrintDocument(body);
}

} // namespace PrintReports
