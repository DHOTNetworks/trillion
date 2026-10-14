#include "purchase_register_report.h"
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

QString renderPurchaseRegisterHtml(const QString& fromDate, const QString& toDate) {
    auto firm = getFirmProfile();

    QString fDate = fromDate.trimmed().isEmpty() ? AccountingEngine::getActiveFromDate() : fromDate.trimmed();
    QString tDate = toDate.trimmed().isEmpty() ? AccountingEngine::getActiveToDate() : toDate.trimmed();

    QVariantList rows = DatabaseManager::instance().executeQuery(
        "SELECT * FROM purchase_invoices WHERE invoice_date >= ? AND invoice_date <= ? ORDER BY invoice_date ASC, id ASC;",
        {fDate, tDate}
    );

    QString rowsHtml = "";
    int rowIdx = 1;
    long long totalBags = 0;
    double totalWeight = 0.0, totalAmt = 0.0;

    for (const auto& var : rows) {
        QVariantMap r = var.toMap();
        long long bags = r.value("bag_count").toLongLong();
        double wt = r.value("weight_qtl").toDouble();
        double tot = r.value("total_amount").toDouble();

        totalBags += bags;
        totalWeight += wt;
        totalAmt += tot;

        rowsHtml += QString(
            "<tr>"
            "<td align='center' bgcolor='#FFFFFF'>%1</td>"
            "<td align='center' bgcolor='#FFFFFF'>%2</td>"
            "<td align='center' bgcolor='#FFFFFF'>%3</td>"
            "<td align='center' bgcolor='#FFFFFF'>%4</td>"
            "<td bgcolor='#FFFFFF'><b>%5</b></td>"
            "<td bgcolor='#FFFFFF'>%6</td>"
            "<td align='center' bgcolor='#FFFFFF'>%7</td>"
            "<td align='right' bgcolor='#FFFFFF'>%8</td>"
            "<td align='right' bgcolor='#FFFFFF'>%9</td>"
            "<td align='right' bgcolor='#FFFFFF'><b>%10</b></td>"
            "</tr>"
        )
        .arg(rowIdx++)
        .arg(FiscalYearHelper::formatDisplayDate(r.value("invoice_date").toString()))
        .arg(r.value("voucher_no").toString().toHtmlEscaped())
        .arg(r.value("invoice_no").toString().toHtmlEscaped())
        .arg(r.value("supplier_name").toString().toHtmlEscaped())
        .arg(r.value("item_name").toString().toHtmlEscaped())
        .arg(bags)
        .arg(QString::number(wt, 'f', 2))
        .arg(QString::number(r.value("rate_per_qtl").toDouble(), 'f', 2))
        .arg(formatINR(tot));
    }

    const QString body = ErpPrint::letterheadHtml(firm)
        + ErpPrint::titleBlockHtml("PURCHASE REGISTER",
            QString("Period: %1 to %2  |  %3 bill(s)").arg(
                FiscalYearHelper::formatDisplayDate(fDate),
                FiscalYearHelper::formatDisplayDate(tDate),
                QString::number(rowIdx - 1)))
        + QString("<table width='100%' border='0' cellspacing='1' cellpadding='4' bgcolor='#94A3B8' style='font-size:9pt'>"
        "  <tr>"
        "    <th width='4%' bgcolor='#0F172A'><font color='#FFFFFF'><b>S.N.</b></font></th>"
        "    <th width='8%' bgcolor='#0F172A'><font color='#FFFFFF'><b>Date</b></font></th>"
        "    <th width='9%' bgcolor='#0F172A'><font color='#FFFFFF'><b>Vch No</b></font></th>"
        "    <th width='9%' bgcolor='#0F172A'><font color='#FFFFFF'><b>Bill No</b></font></th>"
        "    <th width='22%' bgcolor='#0F172A'><font color='#FFFFFF'><b>Supplier Name</b></font></th>"
        "    <th width='14%' bgcolor='#0F172A'><font color='#FFFFFF'><b>Commodity</b></font></th>"
        "    <th width='6%' bgcolor='#0F172A'><font color='#FFFFFF'><b>Bags</b></font></th>"
        "    <th width='7%' bgcolor='#0F172A'><font color='#FFFFFF'><b>Weight</b></font></th>"
        "    <th width='7%' bgcolor='#0F172A'><font color='#FFFFFF'><b>Rate</b></font></th>"
        "    <th width='14%' bgcolor='#0F172A'><font color='#FFFFFF'><b>Total Bill (Rs.)</b></font></th>"
        "  </tr>"
        "  %1"
        "  <tr bgcolor='#F1F5F9'>"
        "    <td colspan='6' align='right' bgcolor='#F1F5F9'>TOTAL</td>"
        "    <td align='center' bgcolor='#F1F5F9'>%2</td>"
        "    <td align='right' bgcolor='#F1F5F9'>%3</td>"
        "    <td bgcolor='#F1F5F9'></td>"
        "    <td align='right' bgcolor='#F1F5F9'><b>%4</b></td>"
        "  </tr>"
        "</table>")
            .arg(rowsHtml, QString::number(totalBags),
                 QString::number(totalWeight, 'f', 2),
                 formatINR(totalAmt))
        + ErpPrint::signatoryHtml(firm.value("firm_name").toString())
        + ErpPrint::generatedFooterHtml();

    return ErpPrint::wrapPrintDocument(body);
}


void buildPurchaseRegisterCsvDocument(const QString& fromDate, const QString& toDate, QStringList* headersOut, QVector<QStringList>* rowsOut, QString* fileNameOut) {

    QString fDate = fromDate.trimmed().isEmpty() ? AccountingEngine::getActiveFromDate() : fromDate.trimmed();
    QString tDate = toDate.trimmed().isEmpty() ? AccountingEngine::getActiveToDate() : toDate.trimmed();

    QVariantList rows = DatabaseManager::instance().executeQuery(
        "SELECT * FROM purchase_invoices WHERE invoice_date >= ? AND invoice_date <= ? ORDER BY invoice_date ASC, id ASC;",
        {fDate, tDate}
    );

    QStringList headers = {"Voucher No", "Bill No", "Date", "Supplier Name", "Commodity", "Bags", "Weight Qtl", "Rate", "Taxable", "Total Bill", "Vehicle"};
    QVector<QStringList> csvRows;

    for (const auto& var : rows) {
        QVariantMap r = var.toMap();
        csvRows.append({
            r.value("voucher_no").toString(),
            r.value("invoice_no").toString(),
            FiscalYearHelper::formatDisplayDate(r.value("invoice_date").toString()),
            r.value("supplier_name").toString(),
            r.value("item_name").toString(),
            QString::number(r.value("bag_count").toLongLong()),
            QString::number(r.value("weight_qtl").toDouble(), 'f', 2),
            QString::number(r.value("rate_per_qtl").toDouble(), 'f', 2),
            QString::number(r.value("taxable_amount").toDouble(), 'f', 2),
            QString::number(r.value("total_amount").toDouble(), 'f', 2),
            r.value("vehicle_no").toString()
        });
    }

    
    *headersOut = headers;
    *rowsOut = csvRows;
    *fileNameOut = "PurchaseRegister.csv";
    return;
}

} // namespace PrintReports
