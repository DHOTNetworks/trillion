#include "item_movement_report.h"
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

QString renderItemMovementHtml(int itemId, const QString& fromDate, const QString& toDate) {
    auto firm = getFirmProfile();

    QVariantList itmRows = DatabaseManager::instance().executeQuery("SELECT * FROM stock_items WHERE id = ? LIMIT 1;", {itemId});
    QString itmName = itmRows.isEmpty() ? "Commodity Item" : itmRows.first().toMap().value("name").toString();

    QString fDate = fromDate.trimmed().isEmpty() ? AccountingEngine::getActiveFromDate() : fromDate.trimmed();
    QString tDate = toDate.trimmed().isEmpty() ? AccountingEngine::getActiveToDate() : toDate.trimmed();

    QVariantList txRows = DatabaseManager::instance().executeQuery(
        "SELECT * FROM stock_transactions WHERE (item_id = ? OR item_name = ?) AND trans_date >= ? AND trans_date <= ? ORDER BY trans_date ASC, id ASC;",
        {itemId, itmName, fDate, tDate}
    );

    QString rowsHtml = "";
    int rowIdx = 1;
    double runningQty = 0.0;

    for (const auto& var : txRows) {
        QVariantMap t = var.toMap();
        QString type = t.value("trans_type").toString();
        double wt = t.value("weight_qtl").toDouble();
        bool isInward = (type == "Purc" || type == "Inward" || type == "P" || type == "M");
        if (isInward) runningQty += wt;
        else runningQty -= wt;

        rowsHtml += QString(
            "<tr>"
            "<td align='center' bgcolor='#FFFFFF'>%1</td>"
            "<td align='center' bgcolor='#FFFFFF'>%2</td>"
            "<td align='center' bgcolor='#FFFFFF'>%3</td>"
            "<td bgcolor='#FFFFFF'>%4</td>"
            "<td align='right' bgcolor='#FFFFFF'>%5</td>"
            "<td align='right' bgcolor='#FFFFFF'>%6</td>"
            "<td align='right' bgcolor='#FFFFFF'><b>%7</b></td>"
            "</tr>"
        )
        .arg(rowIdx++)
        .arg(FiscalYearHelper::formatDisplayDate(t.value("trans_date").toString()))
        .arg(t.value("voucher_no").toString().toHtmlEscaped())
        .arg(t.value("party_name").toString().toHtmlEscaped())
        .arg(isInward ? QString::number(wt, 'f', 2) : "-")
        .arg(!isInward ? QString::number(wt, 'f', 2) : "-")
        .arg(QString::number(runningQty, 'f', 2));
    }

    QString body = ErpPrint::letterheadHtml(firm)
        + ErpPrint::titleBlockHtml("ITEM MOVEMENT TIMELINE & AUDIT",
            QString("Item: %1  |  Period: %2 to %3")
                .arg(itmName, FiscalYearHelper::formatDisplayDate(fDate),
                     FiscalYearHelper::formatDisplayDate(tDate)))
        + QString(
        "<table width='100%' border='0' cellspacing='1' cellpadding='4' bgcolor='#94A3B8' style='font-size:9pt'>"
        "  <tr>"
        "    <th width='5%' bgcolor='#0F172A'><font color='#FFFFFF'><b>S.N.</b></font></th>"
        "    <th width='12%' bgcolor='#0F172A'><font color='#FFFFFF'><b>Date</b></font></th>"
        "    <th width='15%' bgcolor='#0F172A'><font color='#FFFFFF'><b>Voucher No</b></font></th>"
        "    <th width='38%' bgcolor='#0F172A'><font color='#FFFFFF'><b>Party / Particulars</b></font></th>"
        "    <th width='10%' bgcolor='#0F172A'><font color='#FFFFFF'><b>Inward Qtl</b></font></th>"
        "    <th width='10%' bgcolor='#0F172A'><font color='#FFFFFF'><b>Outward Qtl</b></font></th>"
        "    <th width='10%' bgcolor='#0F172A'><font color='#FFFFFF'><b>Balance Qtl</b></font></th>"
        "  </tr>"
        "  %1"
        "</table>")
        .arg(rowsHtml)
        + ErpPrint::signatoryHtml(firm.value("firm_name").toString())
        + ErpPrint::generatedFooterHtml();

    return ErpPrint::wrapPrintDocument(body);
}


} // namespace PrintReports
