#include "profit_loss_report.h"
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

QString renderProfitLossHtml(const QString& fromDate, const QString& toDate) {
    auto firm = getFirmProfile();
    ProfitLossData pnl = ProfitLossCalculator::calculate(fromDate, toDate);

    const QString body = ErpPrint::letterheadHtml(firm)
        + ErpPrint::titleBlockHtml("TRADING AND PROFIT & LOSS ACCOUNT",
            QString("Period: %1 to %2").arg(
                FiscalYearHelper::formatDisplayDate(pnl.fromDate),
                FiscalYearHelper::formatDisplayDate(pnl.toDate)))
        + QString(
            "<table width='100%' border='0' cellspacing='1' cellpadding='4' bgcolor='#94A3B8' style='font-size:9pt'>"
            "<tr><th width='50%' bgcolor='#0F172A'><font color='#FFFFFF'><b>EXPENDITURE / DEBIT</b></font></th>"
            "<th width='50%' bgcolor='#0F172A'><font color='#FFFFFF'><b>INCOME / CREDIT</b></font></th></tr>"
            "<tr>"
            "<td valign='top' bgcolor='#FFFFFF'>"
            "<table width='100%' border='0' cellspacing='1' cellpadding='4' bgcolor='#94A3B8' style='font-size:9pt'>"
            "<tr><td bgcolor='#FFFFFF'>To Opening Stock</td><td align='right' bgcolor='#FFFFFF'>%1</td></tr>"
            "<tr><td bgcolor='#FFFFFF'>To Paddy Procurement / Purchases</td><td align='right' bgcolor='#FFFFFF'>%2</td></tr>"
            "<tr><td bgcolor='#FFFFFF'>To Direct &amp; Milling Expenses</td><td align='right' bgcolor='#FFFFFF'>%3</td></tr>"
            "<tr bgcolor='#F1F5F9'><td bgcolor='#F1F5F9'>To Gross Profit c/d</td><td align='right' bgcolor='#F1F5F9'>%4</td></tr>"
            "<tr><td colspan='2' bgcolor='#FFFFFF'><b>Indirect Expenses</b></td></tr>"
            "<tr><td bgcolor='#FFFFFF'>To Administrative &amp; Office Expenses</td><td align='right' bgcolor='#FFFFFF'>%5</td></tr>"
            "<tr bgcolor='#F1F5F9'><td bgcolor='#F1F5F9'>To Net Profit (to Capital)</td><td align='right' bgcolor='#F1F5F9'><b>%6</b></td></tr>"
            "</table>"
            "</td>"
            "<td valign='top' bgcolor='#FFFFFF'>"
            "<table width='100%' border='0' cellspacing='1' cellpadding='4' bgcolor='#94A3B8' style='font-size:9pt'>"
            "<tr><td bgcolor='#FFFFFF'>By Sales Revenue</td><td align='right' bgcolor='#FFFFFF'>%7</td></tr>"
            "<tr><td bgcolor='#FFFFFF'>By Closing Stock Valuation</td><td align='right' bgcolor='#FFFFFF'>%8</td></tr>"
            "<tr bgcolor='#F1F5F9'><td bgcolor='#F1F5F9'>By Gross Profit b/d</td><td align='right' bgcolor='#F1F5F9'>%4</td></tr>"
            "<tr><td colspan='2' bgcolor='#FFFFFF'><b>Indirect Incomes</b></td></tr>"
            "<tr><td bgcolor='#FFFFFF'>By Interest &amp; Other Receipts</td><td align='right' bgcolor='#FFFFFF'>%9</td></tr>"
            "</table>"
            "</td>"
            "</tr>"
            "</table>")
            .arg(formatINR(pnl.openingStockValue),
                 formatINR(pnl.totalProcurement),
                 formatINR(pnl.totalDirectExpenses),
                 formatINR(pnl.grossProfit),
                 formatINR(pnl.indirectExpenses),
                 formatINR(pnl.netProfit),
                 formatINR(pnl.totalSalesRevenue),
                 formatINR(pnl.closingStockValue),
                 formatINR(pnl.indirectIncomes))
        + ErpPrint::signatoryHtml(firm.value("firm_name").toString())
        + ErpPrint::generatedFooterHtml();

    return ErpPrint::wrapPrintDocument(body);
}


} // namespace PrintReports
