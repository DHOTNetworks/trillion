#include "stock_register_report.h"
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

QString renderStockRegisterHtml(const QString& fromDate, const QString& toDate) {
    auto firm = getFirmProfile();

    QString fDate = fromDate.trimmed().isEmpty() ? AccountingEngine::getActiveFromDate() : fromDate.trimmed();
    QString tDate = toDate.trimmed().isEmpty() ? AccountingEngine::getActiveToDate() : toDate.trimmed();

    QVariantList items = DatabaseManager::instance().executeQuery("SELECT * FROM stock_items ORDER BY name COLLATE NOCASE ASC;");
    QString rowsHtml = "";
    int rowIdx = 1;
    double totalCloseQty = 0.0, totalCloseVal = 0.0;

    for (const auto& itmVar : items) {
        QVariantMap item = itmVar.toMap();
        QString name = item.value("name").toString();
        int itemId = item.value("id").toInt();
        QString code = item.value("code").toString();
        QString unit = item.value("unit", "Qtl").toString();
        double opQty = item.value("opening_qty").toDouble();

        QVariant inVal = DatabaseManager::instance().executeScalar(
            "SELECT SUM(weight_qtl) FROM stock_transactions WHERE (item_id = ? OR item_name = ?) AND trans_type IN ('Purc', 'SlRn', 'Inward', 'P', 'M');",
            {itemId, name}
        );
        QVariant outVal = DatabaseManager::instance().executeScalar(
            "SELECT SUM(weight_qtl) FROM stock_transactions WHERE (item_id = ? OR item_name = ?) AND trans_type IN ('Sale', 'PrRn', 'Outward', 'S');",
            {itemId, name}
        );

        double inQty = inVal.isValid() ? inVal.toDouble() : 0.0;
        double outQty = outVal.isValid() ? outVal.toDouble() : 0.0;
        double closeQty = opQty + inQty - outQty;
        double rate = item.value("rate", item.value("standard_cost", 0.0)).toDouble();
        double closeVal = closeQty * rate;

        totalCloseQty += closeQty;
        totalCloseVal += closeVal;

        rowsHtml += QString(
            "<tr>"
            "<td align='center' bgcolor='#FFFFFF'>%1</td>"
            "<td bgcolor='#FFFFFF'><b>%2</b></td>"
            "<td align='center' bgcolor='#FFFFFF'>%3</td>"
            "<td align='center' bgcolor='#FFFFFF'>%4</td>"
            "<td align='right' bgcolor='#FFFFFF'>%5</td>"
            "<td align='right' bgcolor='#FFFFFF'>%6</td>"
            "<td align='right' bgcolor='#FFFFFF'>%7</td>"
            "<td align='right' bgcolor='#FFFFFF'><b>%8</b></td>"
            "<td align='right' bgcolor='#FFFFFF'>%9</td>"
            "<td align='right' bgcolor='#FFFFFF'><b>%10</b></td>"
            "</tr>"
        )
        .arg(rowIdx++)
        .arg(name.toHtmlEscaped())
        .arg(code.toHtmlEscaped())
        .arg(unit.toHtmlEscaped())
        .arg(QString::number(opQty, 'f', 2))
        .arg(QString::number(inQty, 'f', 2))
        .arg(QString::number(outQty, 'f', 2))
        .arg(QString::number(closeQty, 'f', 2))
        .arg(QString::number(rate, 'f', 2))
        .arg(formatINR(closeVal));
    }

    const QString body = ErpPrint::letterheadHtml(firm)
        + ErpPrint::titleBlockHtml("STOCK REGISTER & INVENTORY VALUATION",
            QString("Period: %1 to %2  |  %3 item(s)").arg(
                FiscalYearHelper::formatDisplayDate(fDate),
                FiscalYearHelper::formatDisplayDate(tDate),
                QString::number(rowIdx - 1)))
        + QString("<table width='100%' border='0' cellspacing='1' cellpadding='4' bgcolor='#94A3B8' style='font-size:9pt'>"
        "  <tr>"
        "    <th width='4%' bgcolor='#0F172A'><font color='#FFFFFF'><b>S.N.</b></font></th>"
        "    <th width='26%' bgcolor='#0F172A'><font color='#FFFFFF'><b>Item Description</b></font></th>"
        "    <th width='8%' bgcolor='#0F172A'><font color='#FFFFFF'><b>Code</b></font></th>"
        "    <th width='6%' bgcolor='#0F172A'><font color='#FFFFFF'><b>Unit</b></font></th>"
        "    <th width='9%' bgcolor='#0F172A'><font color='#FFFFFF'><b>Opening Qtl</b></font></th>"
        "    <th width='9%' bgcolor='#0F172A'><font color='#FFFFFF'><b>Inward Qtl</b></font></th>"
        "    <th width='9%' bgcolor='#0F172A'><font color='#FFFFFF'><b>Outward Qtl</b></font></th>"
        "    <th width='10%' bgcolor='#0F172A'><font color='#FFFFFF'><b>Closing Qtl</b></font></th>"
        "    <th width='9%' bgcolor='#0F172A'><font color='#FFFFFF'><b>Rate ₹</b></font></th>"
        "    <th width='10%' bgcolor='#0F172A'><font color='#FFFFFF'><b>Closing Value ₹</b></font></th>"
        "  </tr>"
        "  %1"
        "  <tr bgcolor='#F1F5F9'>"
        "    <td colspan='7' align='right' bgcolor='#F1F5F9'>TOTAL STOCK VALUATION</td>"
        "    <td align='right' bgcolor='#F1F5F9'>%2</td>"
        "    <td bgcolor='#F1F5F9'></td>"
        "    <td align='right' bgcolor='#F1F5F9'><b>%3</b></td>"
        "  </tr>"
        "</table>")
            .arg(rowsHtml, QString::number(totalCloseQty, 'f', 2),
                 formatINR(totalCloseVal))
        + ErpPrint::signatoryHtml(firm.value("firm_name").toString())
        + ErpPrint::generatedFooterHtml();

    return ErpPrint::wrapPrintDocument(body);
}


void buildStockCsvDocument(const QString& fromDate, const QString& toDate, QStringList* headersOut, QVector<QStringList>* rowsOut, QString* fileNameOut) {

    Q_UNUSED(fromDate);
    Q_UNUSED(toDate);
    QVariantList items = DatabaseManager::instance().executeQuery("SELECT * FROM stock_items ORDER BY name COLLATE NOCASE ASC;");

    QStringList headers = {"Item Name", "Code", "Type", "Unit", "Opening Qtl", "Inward Qtl", "Outward Qtl", "Closing Qtl", "Rate", "Closing Value"};
    QVector<QStringList> csvRows;

    for (const auto& var : items) {
        QVariantMap it = var.toMap();
        QString name = it.value("name").toString();
        int itemId = it.value("id").toInt();
        QString code = it.value("code").toString();
        QString type = it.value("item_type").toString();
        QString unit = it.value("unit", "Qtl").toString();
        double op = it.value("opening_qty").toDouble();

        QVariant inVal = DatabaseManager::instance().executeScalar(
            "SELECT SUM(weight_qtl) FROM stock_transactions WHERE (item_id = ? OR item_name = ?) AND trans_type IN ('Purc', 'SlRn', 'Inward', 'P', 'M');",
            {itemId, name}
        );
        QVariant outVal = DatabaseManager::instance().executeScalar(
            "SELECT SUM(weight_qtl) FROM stock_transactions WHERE (item_id = ? OR item_name = ?) AND trans_type IN ('Sale', 'PrRn', 'Outward', 'S');",
            {itemId, name}
        );

        double inQty = inVal.isValid() ? inVal.toDouble() : 0.0;
        double outQty = outVal.isValid() ? outVal.toDouble() : 0.0;
        double close = op + inQty - outQty;
        double rate = it.value("rate", it.value("standard_cost", 0.0)).toDouble();
        double val = close * rate;

        csvRows.append({
            name, code, type, unit,
            QString::number(op, 'f', 2),
            QString::number(inQty, 'f', 2),
            QString::number(outQty, 'f', 2),
            QString::number(close, 'f', 2),
            QString::number(rate, 'f', 2),
            QString::number(val, 'f', 2)
        });
    }

    
    *headersOut = headers;
    *rowsOut = csvRows;
    *fileNameOut = "StockRegister.csv";
    return;
}

} // namespace PrintReports
