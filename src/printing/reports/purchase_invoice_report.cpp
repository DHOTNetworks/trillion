#include "purchase_invoice_report.h"
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

QString renderPurchaseInvoiceHtml(const QString& invoiceNo) {
    auto firm = getFirmProfile();
    QString q = invoiceNo.trimmed();

    QVariantList purcRows = DatabaseManager::instance().executeQuery(
        "SELECT * FROM purchase_invoices "
        "WHERE invoice_no = ? "
        "   OR voucher_no = ? "
        "   OR id = ? "
        "   OR invoice_no LIKE '%/' || ? "
        "   OR invoice_no LIKE '%-' || ? "
        "ORDER BY "
        "   CASE WHEN invoice_no = ? THEN 1 "
        "        WHEN voucher_no = ? THEN 2 "
        "        WHEN id = ? THEN 3 "
        "        ELSE 4 END, "
        "   invoice_date DESC, id DESC "
        "LIMIT 1;",
        {q, q, q, q, q, q, q, q}
    );
    if (purcRows.isEmpty()) {
        return "<h2>Purchase voucher not found: " + invoiceNo + "</h2>";
    }

    QVariantMap p = purcRows.first().toMap();
    int pId = p.value("id").toInt();
    QString vNum = p.value("voucher_no").toString();
    QString invNum = p.value("invoice_no").toString();
    if (invNum.isEmpty()) invNum = vNum;
    QString invDate = FiscalYearHelper::formatDisplayDate(p.value("invoice_date").toString());
    QString suppName = p.value("supplier_name").toString();
    int suppId = p.value("supplier_id").toInt();
    QString vehNo = p.value("vehicle_no").toString();
    QString ewayNo = p.value("eway_bill_no").toString();
    QString grNo = p.value("gr_no").toString();

    QVariantMap party = getPartyProfile(suppId, suppName);
    QString suppGstin = p.value("gstin").toString().trimmed();
    if (suppGstin.isEmpty()) suppGstin = p.value("supplier_gstin").toString().trimmed();
    if (suppGstin.isEmpty()) suppGstin = party.value("gstin").toString().trimmed();

    QString suppAddress = party.value("address").toString().trimmed();
    QString suppCity = party.value("city").toString().trimmed();
    QString suppState = party.value("state").toString().trimmed();
    QString suppPhone = party.value("phone").toString().trimmed();

    QString suppDetailsHtml = "<b>" + (suppName.isEmpty() ? "Supplier" : suppName.toHtmlEscaped()) + "</b><br>";
    if (!suppAddress.isEmpty()) suppDetailsHtml += suppAddress.toHtmlEscaped() + "<br>";
    if (!suppCity.isEmpty() || !suppState.isEmpty()) {
        QStringList loc;
        if (!suppCity.isEmpty()) loc << suppCity.toHtmlEscaped();
        if (!suppState.isEmpty()) loc << suppState.toHtmlEscaped();
        suppDetailsHtml += loc.join(", ") + "<br>";
    }
    if (!suppGstin.isEmpty()) {
        suppDetailsHtml += "<b>GSTIN:</b> " + suppGstin.toHtmlEscaped();
    } else {
        suppDetailsHtml += "<b>GSTIN:</b> Unregistered";
    }
    if (!party.value("pan").toString().isEmpty()) {
        suppDetailsHtml += " &nbsp;|&nbsp; <b>PAN:</b> " + party.value("pan").toString().toHtmlEscaped();
    }
    if (!suppPhone.isEmpty()) {
        suppDetailsHtml += " &nbsp;|&nbsp; <b>Ph:</b> " + suppPhone.toHtmlEscaped();
    }

    QString voucherDetailsHtml = "";
    if (!invNum.isEmpty()) voucherDetailsHtml += "<b>Bill / Inv No:</b> " + invNum.toHtmlEscaped() + "<br>";
    voucherDetailsHtml += "<b>Date:</b> " + invDate.toHtmlEscaped() + "<br>";
    if (!vehNo.isEmpty()) voucherDetailsHtml += "<b>Vehicle No:</b> " + vehNo.toHtmlEscaped() + "<br>";
    if (!ewayNo.isEmpty()) voucherDetailsHtml += "<b>E-Way Bill:</b> " + ewayNo.toHtmlEscaped() + "<br>";
    if (!grNo.isEmpty()) voucherDetailsHtml += "<b>GR No:</b> " + grNo.toHtmlEscaped() + "<br>";

    QVariantList items = DatabaseManager::instance().executeQuery(
        "SELECT * FROM purchase_invoice_items WHERE invoice_id = ? ORDER BY id ASC;",
        {pId}
    );

    if (items.isEmpty()) {
        QString itemName = p.value("item_name").toString().trimmed();
        if (itemName.isEmpty()) itemName = p.value("commodity").toString().trimmed();
        if (itemName.isEmpty()) itemName = "Paddy / Rice";

        QVariantMap fallbackItem;
        fallbackItem["item_name"] = itemName;
        fallbackItem["hsn_code"] = p.value("hsn_code", "").toString();
        fallbackItem["bag_count"] = p.value("bag_count", 0);
        fallbackItem["weight_qtl"] = p.value("weight_qtl", 0.0);
        fallbackItem["rate_per_qtl"] = p.value("rate_per_qtl", 0.0);
        fallbackItem["taxable_amount"] = p.value("taxable_amount", 0.0);
        fallbackItem["gst_amount"] = p.value("gst_amount", 0.0);
        items.append(fallbackItem);
    }

    QHash<int, QString> hsnById;
    QHash<QString, QString> hsnByName;
    {
        QVariantList stockRows = DatabaseManager::instance().executeQuery(
            "SELECT id, name, hsn_code FROM stock_items;");
        for (const QVariant& sv : stockRows) {
            QVariantMap sm = sv.toMap();
            const QString h = sm.value("hsn_code").toString().trimmed();
            if (h.isEmpty()) continue;
            hsnById[sm.value("id").toInt()] = h;
            hsnByName[sm.value("name").toString().trimmed().toLower()] = h;
        }
    }
    const QString headerHsn = p.value("hsn_code", "").toString().trimmed();

    QString itemRowsHtml = "";
    int rowIdx = 1;
    long long totalBags = 0;
    double totalWeight = 0.0;
    double totalTaxable = 0.0;
    double totalGst = 0.0;

    for (const auto& var : items) {
        QVariantMap it = var.toMap();
        long long bags = it.value("bag_count").toLongLong();
        double wt = it.value("weight_qtl").toDouble();
        double rate = it.value("rate_per_qtl").toDouble();
        double taxAmt = it.value("taxable_amount").toDouble();
        double gstVal = it.value("gst_amount").toDouble();
        const QString itemName = it.value("item_name").toString().trimmed();
        const QString grade = it.value("grade", "").toString().trimmed();
        QString hsn = it.value("hsn_code", "").toString().trimmed();
        if (hsn.isEmpty()) {
            const int itemId = it.value("item_id").toInt();
            if (itemId > 0) hsn = hsnById.value(itemId);
        }
        if (hsn.isEmpty()) hsn = hsnByName.value(itemName.toLower());
        if (hsn.isEmpty()) hsn = headerHsn;
        QString itemCell = QString("<b>%1</b>").arg(itemName.toHtmlEscaped());
        if (!grade.isEmpty())
            itemCell += QString("<br><span class='grade'>Grade: %1</span>").arg(grade.toHtmlEscaped());

        totalBags += bags;
        totalWeight += wt;
        totalTaxable += taxAmt;
        totalGst += gstVal;

        itemRowsHtml += QString(
            "<tr>"
            "<td align='center' bgcolor='#FFFFFF'>%1</td>"
            "<td bgcolor='#FFFFFF'>%2</td>"
            "<td align='center' bgcolor='#FFFFFF'>%3</td>"
            "<td align='center' bgcolor='#FFFFFF'>%4</td>"
            "<td align='right' bgcolor='#FFFFFF'>%5</td>"
            "<td align='right' bgcolor='#FFFFFF'>%6</td>"
            "<td align='right' bgcolor='#FFFFFF'>%7</td>"
            "<td align='right' bgcolor='#FFFFFF'><b>%8</b></td>"
            "</tr>"
        )
        .arg(rowIdx++)
        .arg(itemCell)
        .arg(hsn.isEmpty() ? "-" : hsn.toHtmlEscaped())
        .arg(bags)
        .arg(QString::number(wt, 'f', 2))
        .arg(QString::number(rate, 'f', 2))
        .arg(formatINR(taxAmt))
        .arg(formatINR(taxAmt + gstVal));
    }

    double totalBill = p.value("total_amount").toDouble();
    if (totalBill <= 0.0 && totalTaxable > 0.0) {
        totalBill = totalTaxable + totalGst;
    }

    static const int MinGridRows = 12;
    while (rowIdx <= MinGridRows) {
        itemRowsHtml += "<tr><td align='center' bgcolor='#FFFFFF'>&nbsp;</td><td bgcolor='#FFFFFF'>&nbsp;</td>"
                        "<td align='center' bgcolor='#FFFFFF'>&nbsp;</td><td align='center' bgcolor='#FFFFFF'>&nbsp;</td>"
                        "<td align='right' bgcolor='#FFFFFF'>&nbsp;</td><td align='right' bgcolor='#FFFFFF'>&nbsp;</td>"
                        "<td align='right' bgcolor='#FFFFFF'>&nbsp;</td><td align='right' bgcolor='#FFFFFF'>&nbsp;</td></tr>";
        ++rowIdx;
    }

    const QString body = ErpPrint::docBarHtml("GOODS INWARD / PURCHASE VOUCHER",
            QString("Bill No: %1  |  Date: %2").arg(invNum, invDate))
        + ErpPrint::letterheadHtml(firm)
        + QString("<table width='100%' border='0' cellspacing='1' cellpadding='6' bgcolor='#94A3B8'><tr>"
                  "<td width='50%' bgcolor='#FFFFFF'><b>Supplier:</b><br>%1</td>"
                  "<td width='50%' bgcolor='#FFFFFF'>%2</td>"
                  "</tr></table>"
                  "<table width='100%' border='0' cellspacing='1' cellpadding='5' bgcolor='#94A3B8'>"
                  "<tr><th width='7%' style='white-space:nowrap' bgcolor='#0F172A'><font color='#FFFFFF'><b>S.N.</b></font></th>"
                  "<th width='28%' bgcolor='#0F172A'><font color='#FFFFFF'><b>Commodity / Item</b></font></th>"
                  "<th width='12%' style='white-space:nowrap' bgcolor='#0F172A'><font color='#FFFFFF'><b>HSN</b></font></th>"
                  "<th width='7%' style='white-space:nowrap' bgcolor='#0F172A'><font color='#FFFFFF'><b>Bags</b></font></th>"
                  "<th width='10%' style='white-space:nowrap' bgcolor='#0F172A'><font color='#FFFFFF'><b>Weight (Qtl)</b></font></th>"
                  "<th width='11%' style='white-space:nowrap' bgcolor='#0F172A'><font color='#FFFFFF'><b>Rate (Rs.)</b></font></th>"
                  "<th width='12%' style='white-space:nowrap' bgcolor='#0F172A'><font color='#FFFFFF'><b>Taxable (Rs.)</b></font></th>"
                  "<th width='13%' style='white-space:nowrap' bgcolor='#0F172A'><font color='#FFFFFF'><b>Total (Rs.)</b></font></th></tr>"
                  "%3"
                  "<tr bgcolor='#F1F5F9'><td colspan='3' align='right' bgcolor='#F1F5F9'>TOTAL</td>"
                  "<td align='center' bgcolor='#F1F5F9'>%4</td><td align='right' bgcolor='#F1F5F9'>%5</td><td bgcolor='#F1F5F9'></td>"
                  "<td align='right' bgcolor='#F1F5F9'>%6</td><td align='right' bgcolor='#F1F5F9'>%7</td></tr></table>"
                  "<table width='100%' border='0' cellspacing='1' cellpadding='6' bgcolor='#94A3B8'><tr>"
                  "<td width='65%' bgcolor='#FFFFFF'><b>Amount in Words:</b><br><span class='words'>%8</span></td>"
                  "<td width='35%' align='right' bgcolor='#FFFFFF'><b>TOTAL BILL: %7</b></td>"
                  "</tr></table>")
            .arg(suppDetailsHtml, voucherDetailsHtml, itemRowsHtml,
                 QString::number(totalBags), QString::number(totalWeight, 'f', 2),
                 formatINR(totalTaxable), formatINR(totalBill),
                 amountInWords(totalBill).toHtmlEscaped())
        + ErpPrint::invoiceFooterHtml(firm);

    return ErpPrint::wrapPrintDocument(body);
}


} // namespace PrintReports
