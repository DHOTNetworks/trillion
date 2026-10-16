#include "manufacturing_statement_report.h"
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

QString renderManufacturingStatementHtml(const QString& fromDate, const QString& toDate) {
    auto firm = getFirmProfile();

    QString fDate = fromDate.trimmed().isEmpty() ? AccountingEngine::getActiveFromDate() : fromDate.trimmed();
    QString tDate = toDate.trimmed().isEmpty() ? AccountingEngine::getActiveToDate() : toDate.trimmed();

    QVariantList rows = DatabaseManager::instance().executeQuery(
        "SELECT * FROM manufacturing_batches WHERE batch_date >= ? AND batch_date <= ? ORDER BY batch_date ASC, id ASC;",
        {fDate, tDate}
    );

    QString rowsHtml = "";
    int rowIdx = 1;
    double totInput = 0.0, totOutput = 0.0, totByproduct = 0.0;

    for (const auto& var : rows) {
        QVariantMap r = var.toMap();
        double pIn = r.value("input_qtl").toDouble();
        double oOut = r.value("output_qtl").toDouble();
        double byOut = r.value("byproduct_qtl").toDouble();
        double wOut = r.value("wastage_qtl").toDouble();
        double yPct = r.value("yield_pct").toDouble();

        totInput += pIn;
        totOutput += oOut;
        totByproduct += byOut;

        double recovery = (pIn > 0) ? ((oOut + byOut) / pIn) * 100.0 : 0.0;

        rowsHtml += QString(
            "<tr>"
            "<td align='center' bgcolor='#FFFFFF'>%1</td>"
            "<td align='center' bgcolor='#FFFFFF'>%2</td>"
            "<td align='center' bgcolor='#FFFFFF'>%3</td>"
            "<td bgcolor='#FFFFFF'>%4</td>"
            "<td align='right' bgcolor='#FFFFFF'>%5</td>"
            "<td align='right' bgcolor='#FFFFFF'>%6</td>"
            "<td align='right' bgcolor='#FFFFFF'>%7</td>"
            "<td align='right' bgcolor='#FFFFFF'>%8</td>"
            "<td align='center' bgcolor='#FFFFFF'><b>%9%</b></td>"
            "</tr>"
        )
        .arg(rowIdx++)
        .arg(FiscalYearHelper::formatDisplayDate(r.value("batch_date").toString()))
        .arg(r.value("batch_no").toString().toHtmlEscaped())
        .arg(r.value("input_variety").toString().toHtmlEscaped())
        .arg(QString::number(pIn, 'f', 2))
        .arg(QString::number(oOut, 'f', 2))
        .arg(QString::number(byOut, 'f', 2))
        .arg(QString::number(wOut, 'f', 2))
        .arg(QString::number(yPct > 0.0 ? yPct : recovery, 'f', 2));
    }

    QString body = ErpPrint::letterheadHtml(firm)
        + ErpPrint::titleBlockHtml("MANUFACTURING PRODUCTION & YIELD STATEMENT",
            QString("Period: %1 to %2")
                .arg(FiscalYearHelper::formatDisplayDate(fDate),
                     FiscalYearHelper::formatDisplayDate(tDate)))
        + QString(
        "<table width='100%' border='0' cellspacing='1' cellpadding='4' bgcolor='#94A3B8' style='font-size:9pt'>"
        "  <tr>"
        "    <th width='4%' bgcolor='#0F172A'><font color='#FFFFFF'><b>S.N.</b></font></th>"
        "    <th width='9%' bgcolor='#0F172A'><font color='#FFFFFF'><b>Date</b></font></th>"
        "    <th width='10%' bgcolor='#0F172A'><font color='#FFFFFF'><b>Batch No</b></font></th>"
        "    <th width='15%' bgcolor='#0F172A'><font color='#FFFFFF'><b>Raw Material</b></font></th>"
        "    <th width='11%' bgcolor='#0F172A'><font color='#FFFFFF'><b>Input Qtl</b></font></th>"
        "    <th width='11%' bgcolor='#0F172A'><font color='#FFFFFF'><b>Output Qtl</b></font></th>"
        "    <th width='11%' bgcolor='#0F172A'><font color='#FFFFFF'><b>By-products Qtl</b></font></th>"
        "    <th width='11%' bgcolor='#0F172A'><font color='#FFFFFF'><b>Wastage Qtl</b></font></th>"
        "    <th width='10%' bgcolor='#0F172A'><font color='#FFFFFF'><b>Yield %</b></font></th>"
        "  </tr>"
        "  %1"
        "  <tr bgcolor='#F1F5F9'>"
        "    <td colspan='4' align='right' bgcolor='#F1F5F9'>TOTAL PRODUCTION</td>"
        "    <td align='right' bgcolor='#F1F5F9'>%2</td>"
        "    <td align='right' bgcolor='#F1F5F9'>%3</td>"
        "    <td align='right' bgcolor='#F1F5F9'>%4</td>"
        "    <td bgcolor='#F1F5F9'></td>"
        "  </tr>"
        "</table>")
        .arg(rowsHtml)
        .arg(QString::number(totInput, 'f', 2))
        .arg(QString::number(totOutput, 'f', 2))
        .arg(QString::number(totByproduct, 'f', 2))
        + ErpPrint::signatoryHtml(firm.value("firm_name").toString())
        + ErpPrint::generatedFooterHtml();

    return ErpPrint::wrapPrintDocument(body);
}


} // namespace PrintReports
