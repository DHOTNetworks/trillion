#include "sales_invoice_report.h"
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

QString renderSalesInvoiceSingleHtml(const QString& invoiceNo, const QString& copySubtitle) {
    auto firm = getFirmProfile();
    QString q = invoiceNo.trimmed();

    QVariantList invRows = DatabaseManager::instance().executeQuery(
        "SELECT * FROM sales_invoices "
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
    if (invRows.isEmpty()) {
        return "<h2>Invoice not found: " + invoiceNo + "</h2>";
    }

    QVariantMap inv = invRows.first().toMap();
    int invId = inv.value("id").toInt();
    QString invNum = inv.value("invoice_no").toString();
    if (invNum.isEmpty()) invNum = inv.value("voucher_no").toString();
    QString invDate = FiscalYearHelper::formatDisplayDate(inv.value("invoice_date").toString());
    QString custName = inv.value("customer_name").toString();
    int custId = inv.value("customer_id").toInt();
    QString vehNo = inv.value("vehicle_no").toString();
    QString ewayNo = inv.value("eway_bill_no").toString();
    QString grNo = inv.value("gr_no").toString();

    QVariantMap party = getPartyProfile(custId, custName);
    QString custGstin = inv.value("gstin").toString().trimmed();
    if (custGstin.isEmpty()) custGstin = inv.value("customer_gstin").toString().trimmed();
    if (custGstin.isEmpty()) custGstin = party.value("gstin").toString().trimmed();

    QString custAddress = party.value("address").toString().trimmed();
    QString custCity = party.value("city").toString().trimmed();
    QString custState = party.value("state").toString().trimmed();

    // Billing block (left): name, GST, address, then shipping address
    // (falls back to the billing address when the invoice carries none).
    QString billLoc;
    if (!custCity.isEmpty() || !custState.isEmpty()) {
        QStringList loc;
        if (!custCity.isEmpty()) loc << custCity.toHtmlEscaped();
        if (!custState.isEmpty()) loc << custState.toHtmlEscaped();
        billLoc = loc.join(", ");
    }
    QString billAddrHtml;
    if (!custAddress.isEmpty()) billAddrHtml = custAddress.toHtmlEscaped();
    if (!billLoc.isEmpty()) billAddrHtml += (billAddrHtml.isEmpty() ? "" : "<br>") + billLoc;
    QString billedToHtml = "<b>Billed To:</b><br><b>"
        + (custName.isEmpty() ? "Cash Sale" : custName.toHtmlEscaped()) + "</b>";
    if (!billAddrHtml.isEmpty()) billedToHtml += "<br>" + billAddrHtml;
    billedToHtml += "<br><b>GSTIN:</b> "
        + (custGstin.isEmpty() ? "Unregistered / Consumer" : custGstin.toHtmlEscaped());
    QString shipAddr = inv.value("shipping_address").toString().trimmed();
    QString shipHtml = shipAddr.isEmpty()
        ? (billAddrHtml.isEmpty() ? "-" : billAddrHtml)
        : shipAddr.toHtmlEscaped().replace("\n", "<br>");
    billedToHtml += "<br><b>Shipping Address:</b><br>" + shipHtml;

    // Invoice meta block (right, fixed above the items): date, number,
    // transport details from the invoice's own fields.
    const QString transporter = inv.value("transport").toString().trimmed();
    QString driverName = inv.value("driver_name").toString().trimmed();
    QString driverNo = inv.value("driver").toString().trimmed();
    if (driverNo.isEmpty()) {
        // Entry sometimes packs both into one field ("70820-29613(Sonu)");
        // split it back into number + name for a clean "Name (Number)".
        QRegularExpression bothRe("^\\s*([\\d\\s\\-+]+?)\\s*\\(\\s*([^)]+?)\\s*\\)\\s*$");
        QRegularExpressionMatch bothMt = bothRe.match(driverName);
        if (bothMt.hasMatch() && bothMt.captured(1).trimmed().length() >= 7) {
            driverNo = bothMt.captured(1).trimmed();
            driverName = bothMt.captured(2).trimmed();
        }
    }
    QString driverLine;
    {
        // Data entry sometimes swaps the two fields: the part that looks
        // like a phone number (more digits) prints second, in brackets.
        auto digitCount = [](const QString& s) {
            int n = 0;
            for (const QChar ch : s) if (ch.isDigit()) ++n;
            return n;
        };
        QString dName = driverName, dNo = driverNo;
        if (digitCount(dNo) < digitCount(dName)) qSwap(dName, dNo);
        driverLine = dName;
        if (!dNo.isEmpty())
            driverLine += (driverLine.isEmpty() ? "" : " (") + dNo + (dName.isEmpty() ? "" : ")");
    }
    QString invoiceMetaHtml = "<b>Date:</b> " + invDate.toHtmlEscaped()
        + "<br><b>Invoice No:</b> " + invNum.toHtmlEscaped();
    if (!ewayNo.isEmpty()) {
        invoiceMetaHtml += "<br><b>E-way No:</b> " + ewayNo.toHtmlEscaped();
    }
    if (!transporter.isEmpty()) {
        invoiceMetaHtml += "<br><b>Transporter:</b> " + transporter.toHtmlEscaped();
    }
    if (!vehNo.isEmpty()) {
        invoiceMetaHtml += "<br><b>Vehicle No:</b> " + vehNo.toHtmlEscaped();
    }
    if (!driverLine.isEmpty()) {
        invoiceMetaHtml += "<br><b>Driver:</b> " + driverLine.toHtmlEscaped();
    }
    if (!grNo.isEmpty()) {
        invoiceMetaHtml += "<br><b>GR No:</b> " + grNo.toHtmlEscaped();
    }

    QVariantList items = DatabaseManager::instance().executeQuery(
        "SELECT * FROM sales_invoice_items WHERE invoice_id = ? ORDER BY id ASC;",
        {invId}
    );

    if (items.isEmpty()) {
        QString itemName = inv.value("item_name").toString().trimmed();
        if (itemName.isEmpty()) itemName = inv.value("commodity").toString().trimmed();
        if (itemName.isEmpty()) itemName = "Rice / Paddy";

        QVariantMap fallbackItem;
        fallbackItem["item_name"] = itemName;
        fallbackItem["hsn_code"] = inv.value("hsn_code", "").toString();
        fallbackItem["bag_count"] = inv.value("bag_count", 0);
        fallbackItem["weight_qtl"] = inv.value("weight_qtl", 0.0);
        fallbackItem["rate_per_qtl"] = inv.value("rate_per_qtl", 0.0);
        fallbackItem["taxable_amount"] = inv.value("taxable_amount", 0.0);
        fallbackItem["gst_amount"] = inv.value("gst_amount", 0.0);
        items.append(fallbackItem);
    }

    // HSN the proper way: line value, else invoice header, else stock master
    // (one prefetched map — never per-row queries).
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
    const QString headerHsn = inv.value("hsn_code", "").toString().trimmed();

    QString itemRowsHtml = "";
    itemRowsHtml.reserve(items.size() * 220 + 1024);
    int rowIdx = 1;
    long long totalBags = 0;
    double totalWeight = 0.0;
    double totalTaxable = 0.0;
    double totalGst = 0.0;
    double totalLines = 0.0;
    double totalGstLn = 0.0;
    auto trimPct = [](double p) {
        QString s = QString::number(p, 'f', 2);
        while (s.endsWith('0')) s.chop(1);
        if (s.endsWith('.')) s.chop(1);
        return s.isEmpty() ? QString("0") : s;
    };

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
            itemCell += QString("<br><font size='2' color='#475569'>%1</font>").arg(grade.toHtmlEscaped());

        const QString packing = it.value("packing", "").toString().trimmed();
        double lineAmt = it.value("total_amount").toDouble();
        if (lineAmt <= 0.0) lineAmt = taxAmt + gstVal;
        double lineGstPct = it.value("gst_pct", 0.0).toDouble();
        if (lineGstPct <= 0.0) lineGstPct = inv.value("gst_pct", 0.0).toDouble();
        double lineGst = lineAmt - taxAmt;
        if (lineGst < 0.0) lineGst = 0.0;
        if (lineGst <= 0.0 && taxAmt > 0.0 && lineGstPct > 0.0)
            lineGst = taxAmt * lineGstPct / 100.0;

        totalBags += bags;
        totalWeight += wt;
        totalTaxable += taxAmt;
        totalGst += gstVal;
        totalLines += lineAmt;
        totalGstLn += lineGst;

        // Excel-style hairlines: the 1px cellspacing gap over the table's
        // slate background draws every rule. Attribute-only (bgcolor/align)
        // so shared print CSS can never double the lines. Item names may
        // wrap; every other cell is locked to one line.
        itemRowsHtml += QString(
            "<tr>"
            "<td align='center' nowrap style='white-space:nowrap;' bgcolor='#FFFFFF'>%1</td>"
            "<td valign='top' bgcolor='#FFFFFF'>%2</td>"
            "<td align='center' nowrap style='white-space:nowrap;' bgcolor='#FFFFFF'>%3</td>"
            "<td align='right' nowrap style='white-space:nowrap;' bgcolor='#FFFFFF'>%4</td>"
            "<td align='center' nowrap style='white-space:nowrap;' bgcolor='#FFFFFF'>%5</td>"
            "<td align='center' nowrap style='white-space:nowrap;' bgcolor='#FFFFFF'>%6</td>"
            "<td align='right' nowrap style='white-space:nowrap;' bgcolor='#FFFFFF'>%7</td>"
            "<td align='center' nowrap style='white-space:nowrap;' bgcolor='#FFFFFF'>%8</td>"
            "<td align='right' nowrap style='white-space:nowrap;' bgcolor='#FFFFFF'>%9</td>"
            "<td align='right' nowrap style='white-space:nowrap;' bgcolor='#FFFFFF'><b>%10</b></td>"
            "</tr>"
        )
        .arg(rowIdx++)
        .arg(itemCell)
        .arg(hsn.isEmpty() ? "-" : hsn.toHtmlEscaped())
        .arg(QString::number(wt, 'f', 2))
        .arg(bags)
        .arg(packing.isEmpty() ? "-" : packing.toHtmlEscaped())
        .arg(QString::number(rate, 'f', 2))
        .arg(trimPct(lineGstPct) + "%")
        .arg(formatINR(lineGst))
        .arg(formatINR(lineAmt));
    }

    // Clean white sheet: item rows only for entered items — no filler
    // grids on non-existent items. Free space stays blank white.
    double grandTotal = inv.value("total_amount").toDouble();
    if (grandTotal <= 0.0 && totalTaxable > 0.0) {
        grandTotal = totalTaxable + totalGst;
    }

    // Firm header block: small copy tag on top, bold firm name, address,
    // then GST+PAN on the left and Phone on the right.
    const QString firmName = firm.value("firm_name").toString().trimmed();
    QString fAddr = firm.value("address").toString().trimmed();
    const QString fCity = firm.value("city").toString().trimmed();
    const QString fStateCore = firm.value("state").toString().split('(').first().trimmed();
    if (!fCity.isEmpty() && !fAddr.contains(fCity, Qt::CaseInsensitive))
        fAddr += (fAddr.isEmpty() ? "" : ", ") + fCity;
    if (!fStateCore.isEmpty() && !fAddr.contains(fStateCore, Qt::CaseInsensitive))
        fAddr += (fAddr.isEmpty() ? "" : ", ") + fStateCore;
    QString topHtml;
    if (!copySubtitle.trimmed().isEmpty())
        topHtml += QString("<div align='center' style='font-size:8pt;color:#475569;'>%1</div>")
                       .arg(copySubtitle.trimmed().toHtmlEscaped());
    // 7.5mm cap height needs ~30pt+ type; CSS size with a legacy <font>
    // fallback so the heading stays big even where QTextDocument drops
    // the stylesheet rule (CSS wins when honored, font tag otherwise).
    topHtml += QString("<div align='center' style='font-size:13pt;'><b>TAX INVOICE</b></div>");
    topHtml += QString("<div align='center' style='font-size:29pt;'><font size='7'><b>%1</b></font></div>")
                   .arg(firmName.isEmpty() ? "TAX INVOICE" : firmName.toHtmlEscaped());
    if (!fAddr.isEmpty())
        topHtml += QString("<div align='center'>%1</div>").arg(fAddr.toHtmlEscaped());
    // Firm GST/PAN/Phone sit as small plain side lines below the firm
    // details — not inside a bordered box.
    topHtml += QString("<table width='100%' border='0' cellspacing='0' cellpadding='2'><tr>"
                       "<td width='50%' valign='top' style='font-size:8.5pt;color:#334155;' bgcolor='#FFFFFF'>"
                       "<b>GSTIN:</b> %1 &nbsp; <b>PAN:</b> %2</td>"
                       "<td width='50%' valign='top' align='right' style='font-size:8.5pt;color:#334155;' bgcolor='#FFFFFF'>"
                       "<b>Phone:</b> %3</td>"
                       "</tr></table>")
                   .arg(firm.value("gstin").toString().trimmed().toHtmlEscaped())
                   .arg(firm.value("pan").toString().trimmed().toHtmlEscaped())
                   .arg(firm.value("phone").toString().trimmed().toHtmlEscaped());

    // Tax summary box (right-sided, just above the fixed bottom): GST split
    // (IGST for inter-state, else CGST+SGST), then round off, discount,
    // grand total. IGST presence decides the split — no state parsing.
    const double cgstAmt = inv.value("cgst_amount").toDouble();
    const double sgstAmt = inv.value("sgst_amount").toDouble();
    const double igstAmt = inv.value("igst_amount").toDouble();
    const double roundOff = inv.value("round_off").toDouble();
    const double discount = inv.value("less_amount").toDouble();
    const double gstPct = inv.value("gst_pct").toDouble();
    QString taxRows;
    const double headGst = inv.value("gst_amount").toDouble();
    if (igstAmt > 0.005) {
        taxRows += QString("<tr><td bgcolor='#FFFFFF'>IGST (%1%)</td><td align='right' nowrap style='white-space:nowrap;' bgcolor='#FFFFFF'>%2</td></tr>")
                       .arg(trimPct(gstPct), formatINR(igstAmt));
    } else if (cgstAmt > 0.005 || sgstAmt > 0.005 || gstPct > 0.0 || headGst > 0.005) {
        const double half = gstPct > 0.0 ? gstPct / 2.0 : 0.0;
        taxRows += QString("<tr><td bgcolor='#FFFFFF'>CGST (%1%)</td><td align='right' nowrap style='white-space:nowrap;' bgcolor='#FFFFFF'>%2</td></tr>")
                       .arg(trimPct(half), formatINR(cgstAmt > 0.005 ? cgstAmt : headGst / 2.0));
        taxRows += QString("<tr><td bgcolor='#FFFFFF'>SGST (%1%)</td><td align='right' nowrap style='white-space:nowrap;' bgcolor='#FFFFFF'>%2</td></tr>")
                       .arg(trimPct(half), formatINR(sgstAmt > 0.005 ? sgstAmt : headGst / 2.0));
    } else {
        taxRows += QString("<tr><td bgcolor='#FFFFFF'>CGST (%1%)</td><td align='right' nowrap style='white-space:nowrap;' bgcolor='#FFFFFF'>%2</td></tr>")
                       .arg(trimPct(gstPct > 0.0 ? gstPct / 2.0 : 0.0), formatINR(cgstAmt));
        taxRows += QString("<tr><td bgcolor='#FFFFFF'>SGST (%1%)</td><td align='right' nowrap style='white-space:nowrap;' bgcolor='#FFFFFF'>%2</td></tr>")
                       .arg(trimPct(gstPct > 0.0 ? gstPct / 2.0 : 0.0), formatINR(sgstAmt));
    }
    taxRows += QString("<tr><td bgcolor='#FFFFFF'>Round off</td><td align='right' nowrap style='white-space:nowrap;' bgcolor='#FFFFFF'>%1</td></tr>")
                   .arg(formatINR(roundOff));
    taxRows += QString("<tr><td bgcolor='#FFFFFF'>Discount</td><td align='right' nowrap style='white-space:nowrap;' bgcolor='#FFFFFF'>%1</td></tr>")
                   .arg(formatINR(discount));
    taxRows += QString("<tr><td bgcolor='#F1F5F9'><b>GRAND TOTAL</b></td><td align='right' nowrap style='white-space:nowrap;' bgcolor='#F1F5F9'><b>%1</b></td></tr>")
                   .arg(formatINR(grandTotal));

    // Bottom pin: blank gridless whitespace after the item grid so the
    // words band + tax box + fixed footer always start at the page bottom
    // no matter how few items the invoice has. Non-empty <div> lines (never
    // bare <br>, which QTextDocument collapses away). Count scales with the
    // item shortfall of a 12-row page and is capped so the footer itself
    // can never spill onto the next page; long invoices flow untouched.
    QString spacerHtml;
    const int missingRows = 12 - items.size();
    if (missingRows > 0) {
        int divs = missingRows * 2;
        if (divs > 18) divs = 18;
        for (int k = 0; k < divs; ++k) spacerHtml += "<div>&nbsp;</div>";
    }

    const QString body = topHtml
        + QString("<table width='100%' border='0' cellspacing='1' cellpadding='6' bgcolor='#94A3B8'><tr>"
                  "<td width='50%' valign='top' bgcolor='#FFFFFF'>%1</td>"
                  "<td width='50%' valign='top' bgcolor='#FFFFFF'>%2</td>"
                  "</tr></table>"
                  "<table width='100%' border='0' cellspacing='1' cellpadding='5' bgcolor='#94A3B8'>"
                  "<tr>"
                  "<th width='4%' bgcolor='#0F172A' style='white-space:nowrap'><font color='#FFFFFF'><b>S.N.</b></font></th>"
                  "<th width='26%' bgcolor='#0F172A'><font color='#FFFFFF'><b>Item Description</b></font></th>"
                  "<th width='9%' bgcolor='#0F172A' style='white-space:nowrap'><font color='#FFFFFF'><b>HSN</b></font></th>"
                  "<th width='8%' bgcolor='#0F172A' style='white-space:nowrap'><font color='#FFFFFF'><b>Qty (Qtl)</b></font></th>"
                  "<th width='6%' bgcolor='#0F172A' style='white-space:nowrap'><font color='#FFFFFF'><b>Bags</b></font></th>"
                  "<th width='8%' bgcolor='#0F172A' style='white-space:nowrap'><font color='#FFFFFF'><b>Packing</b></font></th>"
                  "<th width='10%' bgcolor='#0F172A' style='white-space:nowrap'><font color='#FFFFFF'><b>Rate (Rs.)</b></font></th>"
                  "<th width='6%' bgcolor='#0F172A' style='white-space:nowrap'><font color='#FFFFFF'><b>GST %</b></font></th>"
                  "<th width='9%' bgcolor='#0F172A' style='white-space:nowrap'><font color='#FFFFFF'><b>GST Amt (Rs.)</b></font></th>"
                  "<th width='14%' bgcolor='#0F172A' style='white-space:nowrap'><font color='#FFFFFF'><b>Amount (Rs.)</b></font></th></tr>"
                  "%3"
                  "<tr><td colspan='3' align='right' bgcolor='#F1F5F9'><b>TOTAL</b></td>"
                  "<td align='right' nowrap style='white-space:nowrap;' bgcolor='#F1F5F9'>%4</td>"
                  "<td align='center' nowrap style='white-space:nowrap;' bgcolor='#F1F5F9'>%5</td>"
                  "<td bgcolor='#F1F5F9'></td><td bgcolor='#F1F5F9'></td><td bgcolor='#F1F5F9'></td>"
                  "<td align='right' nowrap style='white-space:nowrap;' bgcolor='#F1F5F9'>%6</td>"
                  "<td align='right' nowrap style='white-space:nowrap;' bgcolor='#F1F5F9'><b>%7</b></td></tr></table>")
            .arg(billedToHtml, invoiceMetaHtml, itemRowsHtml,
                 QString::number(totalWeight, 'f', 2), QString::number(totalBags),
                 formatINR(totalGstLn), formatINR(totalLines))
        + spacerHtml
        + QString("<table width='100%' border='0' cellspacing='0' cellpadding='0'><tr>"
                  "<td width='55%' bgcolor='#FFFFFF'></td>"
                  "<td width='45%' valign='top' bgcolor='#FFFFFF'><table width='100%' border='0' cellspacing='1' cellpadding='5' bgcolor='#94A3B8'>%1</table></td>"
                  "</tr></table>"
                  "<table width='100%' border='0' cellspacing='1' cellpadding='6' bgcolor='#94A3B8'><tr>"
                  "<td bgcolor='#FFFFFF'><b>Amount in Words:</b> %2</td>"
                  "</tr></table>")
            .arg(taxRows, amountInWords(grandTotal).toHtmlEscaped())
        + ErpPrint::salesInvoiceFooterHtml(firm);

    return ErpPrint::wrapPrintDocument(body);
}


QString renderSalesInvoiceHtml(const QString& invoiceNo, const QString& copyType) {
    QString norm = copyType.trimmed().toUpper();
    if (norm == "ALL" || norm == "BOTH" || norm == "ORIGINAL + DUPLICATE" || norm == "ALL COPIES") {
        return renderSalesInvoiceSingleHtml(invoiceNo, "ORIGINAL FOR RECIPIENT") +
               "<div style='page-break-before: always;'></div>" +
               renderSalesInvoiceSingleHtml(invoiceNo, "DUPLICATE FOR TRANSPORTER");
    }
    return renderSalesInvoiceSingleHtml(invoiceNo, copyType);
}


} // namespace PrintReports
