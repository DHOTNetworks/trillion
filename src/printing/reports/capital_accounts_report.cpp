#include "capital_accounts_report.h"
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

QString renderCapitalAccountsHtml(const QString& fromDate, const QString& toDate) {
    auto firm = getFirmProfile();

    QString fDate = fromDate.trimmed();
    QString tDate = toDate.trimmed();
    if (fDate.isEmpty()) fDate = AccountingEngine::getActiveFromDate();
    if (tDate.isEmpty()) tDate = AccountingEngine::getActiveToDate();

    // Fetch Capital Accounts strictly via AccountClassifier (code 1)
    QString capClause = AccountClassifier::generateHierarchySqlClause({StandardGroupCode::Capital}, "g");
    QVariantList capRows = DatabaseManager::instance().executeQuery(
        "SELECT p.id, p.name, p.opening_balance, p.balance_type FROM parties p "
        "LEFT JOIN account_groups g ON (p.group_id = g.id OR (p.group_code > 0 AND p.group_code = g.code1st) OR p.group_name = g.name) "
        "WHERE " + capClause + " ORDER BY p.name ASC;"
    );

    ProfitLossData pnl = ProfitLossCalculator::calculate(fDate, tDate);
    int partnerCount = std::max(1, (int)capRows.size());
    double profitPerPartner = pnl.netProfit / partnerCount;

    QString rowsHtml = "";
    int rowIdx = 1;
    double totOp = 0.0, totAdd = 0.0, totDraw = 0.0, totProf = 0.0, totClose = 0.0;

    for (const auto& var : capRows) {
        QVariantMap p = var.toMap();
        int pId = p.value("id").toInt();
        QString name = p.value("name").toString();
        double op = p.value("opening_balance").toDouble();

        QVariant drVal = DatabaseManager::instance().executeScalar(
            "SELECT SUM(amount) FROM transactions WHERE party_id = ? AND voucher_date >= ? AND voucher_date <= ? AND dr_cr = 'Dr';",
            {pId, fDate, tDate}
        );
        QVariant crVal = DatabaseManager::instance().executeScalar(
            "SELECT SUM(amount) FROM transactions WHERE party_id = ? AND voucher_date >= ? AND voucher_date <= ? AND dr_cr = 'Cr';",
            {pId, fDate, tDate}
        );

        double drawings = drVal.isValid() ? drVal.toDouble() : 0.0;
        double additions = crVal.isValid() ? crVal.toDouble() : 0.0;
        double close = op + additions - drawings + profitPerPartner;

        totOp += op;
        totAdd += additions;
        totDraw += drawings;
        totProf += profitPerPartner;
        totClose += close;

        rowsHtml += QString(
            "<tr>"
            "<td align='center' bgcolor='#FFFFFF'>%1</td>"
            "<td bgcolor='#FFFFFF'><b>%2</b></td>"
            "<td align='right' bgcolor='#FFFFFF'>%3</td>"
            "<td align='right' bgcolor='#FFFFFF'>%4</td>"
            "<td align='right' bgcolor='#FFFFFF'>%5</td>"
            "<td align='right' bgcolor='#FFFFFF'>%6</td>"
            "<td align='right' bgcolor='#FFFFFF'><b>%7</b></td>"
            "</tr>"
        )
        .arg(rowIdx++)
        .arg(name.toHtmlEscaped())
        .arg(formatINR(op))
        .arg(formatINR(additions))
        .arg(formatINR(drawings))
        .arg(formatINR(profitPerPartner))
        .arg(formatINR(close));
    }

    QString body = ErpPrint::letterheadHtml(firm)
        + ErpPrint::titleBlockHtml("PARTNERS CAPITAL ACCOUNTS SCHEDULE",
            QString("Period: %1 to %2")
                .arg(FiscalYearHelper::formatDisplayDate(fDate),
                     FiscalYearHelper::formatDisplayDate(tDate)))
        + QString(
        "<table width='100%' border='0' cellspacing='1' cellpadding='4' bgcolor='#94A3B8' style='font-size:9pt'>"
        "  <tr>"
        "    <th width='5%' bgcolor='#0F172A'><font color='#FFFFFF'><b>S.N.</b></font></th>"
        "    <th width='35%' bgcolor='#0F172A'><font color='#FFFFFF'><b>Partner Name</b></font></th>"
        "    <th width='12%' bgcolor='#0F172A'><font color='#FFFFFF'><b>Opening Capital (Rs.)</b></font></th>"
        "    <th width='12%' bgcolor='#0F172A'><font color='#FFFFFF'><b>Additions / Intro (Rs.)</b></font></th>"
        "    <th width='12%' bgcolor='#0F172A'><font color='#FFFFFF'><b>Drawings (Rs.)</b></font></th>"
        "    <th width='12%' bgcolor='#0F172A'><font color='#FFFFFF'><b>Profit Share (Rs.)</b></font></th>"
        "    <th width='12%' bgcolor='#0F172A'><font color='#FFFFFF'><b>Closing Capital (Rs.)</b></font></th>"
        "  </tr>"
        "  %1"
        "  <tr bgcolor='#F1F5F9'>"
        "    <td colspan='2' align='right' bgcolor='#F1F5F9'>TOTAL CAPITAL</td>"
        "    <td align='right' bgcolor='#F1F5F9'>%2</td>"
        "    <td align='right' bgcolor='#F1F5F9'>%3</td>"
        "    <td align='right' bgcolor='#F1F5F9'>%4</td>"
        "    <td align='right' bgcolor='#F1F5F9'>%5</td>"
        "    <td align='right' bgcolor='#F1F5F9'>%6</td>"
        "  </tr>"
        "</table>")
        .arg(rowsHtml)
        .arg(formatINR(totOp))
        .arg(formatINR(totAdd))
        .arg(formatINR(totDraw))
        .arg(formatINR(totProf))
        .arg(formatINR(totClose))
        + ErpPrint::signatoryHtml(firm.value("firm_name").toString())
        + ErpPrint::generatedFooterHtml();

    return ErpPrint::wrapPrintDocument(body);
}


} // namespace PrintReports
