#include "journal_voucher_report.h"
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

QString renderJournalVoucherHtml(const QString& voucherNo) {
    auto firm = getFirmProfile();

    QVariantList txRows = DatabaseManager::instance().executeQuery(
        "SELECT * FROM transactions WHERE voucher_no = ? ORDER BY id ASC;",
        {voucherNo.trimmed()}
    );
    if (txRows.isEmpty()) {
        return "<h2>Journal voucher not found: " + voucherNo + "</h2>";
    }

    QString vDate = FiscalYearHelper::formatDisplayDate(txRows.first().toMap().value("voucher_date").toString());
    QString rowsHtml = "";
    int rowIdx = 1;
    double totalDr = 0.0, totalCr = 0.0;
    QString narr = "";

    for (const auto& var : txRows) {
        QVariantMap t = var.toMap();
        QString side = t.value("dr_cr").toString();
        double amt = t.value("amount").toDouble();
        if (narr.isEmpty()) narr = t.value("narration").toString();

        double drAmt = (side == "Dr") ? amt : 0.0;
        double crAmt = (side == "Cr") ? amt : 0.0;
        totalDr += drAmt;
        totalCr += crAmt;

        rowsHtml += QString(
            "<tr>"
            "<td align='center' bgcolor='#FFFFFF'>%1</td>"
            "<td bgcolor='#FFFFFF'><b>%2</b></td>"
            "<td bgcolor='#FFFFFF'>%3</td>"
            "<td align='right' bgcolor='#FFFFFF'>%4</td>"
            "<td align='right' bgcolor='#FFFFFF'>%5</td>"
            "</tr>"
        )
        .arg(rowIdx++)
        .arg(t.value("party_name").toString().toHtmlEscaped())
        .arg(t.value("opposing_account").toString().toHtmlEscaped())
        .arg(drAmt > 0 ? formatINR(drAmt) : "-")
        .arg(crAmt > 0 ? formatINR(crAmt) : "-");
    }

    QString body = ErpPrint::letterheadHtml(firm)
        + ErpPrint::titleBlockHtml("JOURNAL ADJUSTMENT VOUCHER",
            QString("Voucher No: %1  |  Date: %2").arg(voucherNo.trimmed(), vDate))
        + QString(
        "<table width='100%' border='0' cellspacing='1' cellpadding='5' bgcolor='#94A3B8'>"
        "  <tr>"
        "    <th width='6%' bgcolor='#0F172A'><font color='#FFFFFF'><b>S.N.</b></font></th>"
        "    <th width='44%' bgcolor='#0F172A'><font color='#FFFFFF'><b>Particulars / Account</b></font></th>"
        "    <th width='26%' bgcolor='#0F172A'><font color='#FFFFFF'><b>Opposing Account</b></font></th>"
        "    <th width='12%' bgcolor='#0F172A'><font color='#FFFFFF'><b>Debit (Rs.)</b></font></th>"
        "    <th width='12%' bgcolor='#0F172A'><font color='#FFFFFF'><b>Credit (Rs.)</b></font></th>"
        "  </tr>"
        "  %1"
        "  <tr bgcolor='#F1F5F9'>"
        "    <td colspan='3' align='right' bgcolor='#F1F5F9'>TOTAL</td>"
        "    <td align='right' bgcolor='#F1F5F9'>%2</td>"
        "    <td align='right' bgcolor='#F1F5F9'>%3</td>"
        "  </tr>"
        "</table>"
        "<table width='100%' border='0' cellspacing='1' cellpadding='6' bgcolor='#94A3B8'>"
        "  <tr>"
        "    <td bgcolor='#FFFFFF'><b>Narration:</b><br>%4</td>"
        "  </tr>"
        "</table>")
        .arg(rowsHtml)
        .arg(formatINR(totalDr))
        .arg(formatINR(totalCr))
        .arg((narr.isEmpty() ? "Adjustment entry" : narr).toHtmlEscaped())
        + ErpPrint::voucherSignHtml(QStringList() << "Prepared By" << ErpPrint::forFirmLabel(firm));

    return ErpPrint::wrapPrintDocument(body);
}


} // namespace PrintReports
