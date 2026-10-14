#include "depreciation_chart_report.h"
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

QString renderDepreciationChartHtml(const QString& asOnDate) {
    auto firm = getFirmProfile();

    QString dIso = asOnDate.trimmed().isEmpty() ? QDate::currentDate().toString("yyyy-MM-dd") : asOnDate.trimmed();

    QVariantList assets = DatabaseManager::instance().executeQuery("SELECT * FROM fixed_assets ORDER BY name ASC;");
    QString rowsHtml = "";
    int rowIdx = 1;
    double totOp = 0.0, totAdd = 0.0, totDep = 0.0, totClose = 0.0;

    for (const auto& var : assets) {
        QVariantMap a = var.toMap();
        QString name = a.value("name").toString();
        double op = a.value("opening_wdv").toDouble();
        double add = a.value("additions_cost").toDouble();
        double rate = a.value("depreciation_rate").toDouble();
        double dep = ((op + add) * rate) / 100.0;
        double close = (op + add) - dep;

        totOp += op;
        totAdd += add;
        totDep += dep;
        totClose += close;

        rowsHtml += QString(
            "<tr>"
            "<td align='center' bgcolor='#FFFFFF'>%1</td>"
            "<td bgcolor='#FFFFFF'><b>%2</b></td>"
            "<td align='right' bgcolor='#FFFFFF'>%3</td>"
            "<td align='right' bgcolor='#FFFFFF'>%4</td>"
            "<td align='center' bgcolor='#FFFFFF'>%5%</td>"
            "<td align='right' bgcolor='#FFFFFF'>%6</td>"
            "<td align='right' bgcolor='#FFFFFF'><b>%7</b></td>"
            "</tr>"
        )
        .arg(rowIdx++)
        .arg(name.toHtmlEscaped())
        .arg(formatINR(op))
        .arg(formatINR(add))
        .arg(QString::number(rate, 'f', 1))
        .arg(formatINR(dep))
        .arg(formatINR(close));
    }

    QString body = ErpPrint::letterheadHtml(firm)
        + ErpPrint::titleBlockHtml("FIXED ASSETS DEPRECIATION SCHEDULE (COMPANIES ACT / IT ACT)",
            QString("As on %1").arg(FiscalYearHelper::formatDisplayDate(dIso)))
        + QString(
        "<table width='100%' border='0' cellspacing='1' cellpadding='4' bgcolor='#94A3B8' style='font-size:9pt'>"
        "  <tr>"
        "    <th width='5%' bgcolor='#0F172A'><font color='#FFFFFF'><b>S.N.</b></font></th>"
        "    <th width='35%' bgcolor='#0F172A'><font color='#FFFFFF'><b>Asset Description / Block</b></font></th>"
        "    <th width='13%' bgcolor='#0F172A'><font color='#FFFFFF'><b>Opening WDV (Rs.)</b></font></th>"
        "    <th width='13%' bgcolor='#0F172A'><font color='#FFFFFF'><b>Additions (Rs.)</b></font></th>"
        "    <th width='8%' bgcolor='#0F172A'><font color='#FFFFFF'><b>Rate</b></font></th>"
        "    <th width='13%' bgcolor='#0F172A'><font color='#FFFFFF'><b>Depreciation (Rs.)</b></font></th>"
        "    <th width='13%' bgcolor='#0F172A'><font color='#FFFFFF'><b>Closing WDV (Rs.)</b></font></th>"
        "  </tr>"
        "  %1"
        "  <tr bgcolor='#F1F5F9'>"
        "    <td colspan='2' align='right' bgcolor='#F1F5F9'>TOTAL</td>"
        "    <td align='right' bgcolor='#F1F5F9'>%2</td>"
        "    <td align='right' bgcolor='#F1F5F9'>%3</td>"
        "    <td bgcolor='#F1F5F9'></td>"
        "    <td align='right' bgcolor='#F1F5F9'>%4</td>"
        "    <td align='right' bgcolor='#F1F5F9'>%5</td>"
        "  </tr>"
        "</table>")
        .arg(rowsHtml)
        .arg(formatINR(totOp))
        .arg(formatINR(totAdd))
        .arg(formatINR(totDep))
        .arg(formatINR(totClose))
        + ErpPrint::signatoryHtml(firm.value("firm_name").toString())
        + ErpPrint::generatedFooterHtml();

    return ErpPrint::wrapPrintDocument(body);
}


} // namespace PrintReports
