#include "debit_credit_note_report.h"
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

QString renderDebitCreditNoteHtml(const QString& noteNo) {
    auto firm = getFirmProfile();

    QVariantList rows = DatabaseManager::instance().executeQuery(
        "SELECT * FROM debit_credit_notes WHERE note_no = ? OR id = ? LIMIT 1;",
        {noteNo.trimmed(), noteNo.trimmed()}
    );
    if (rows.isEmpty()) {
        return "<h2>Note not found: " + noteNo + "</h2>";
    }

    QVariantMap r = rows.first().toMap();
    QString nDate = FiscalYearHelper::formatDisplayDate(r.value("note_date").toString());
    QString nType = r.value("note_type", "Debit Note").toString();
    QString pName = r.value("party_name").toString();
    QString origInv = r.value("original_invoice_no").toString();
    double taxable = r.value("taxable_amount").toDouble();
    double gst = r.value("gst_amount").toDouble();
    double total = r.value("total_amount").toDouble();
    QString reason = r.value("reason").toString();

    QString body = ErpPrint::letterheadHtml(firm)
        + ErpPrint::titleBlockHtml(nType.toUpper(),
            QString("Note No: %1  |  Date: %2").arg(noteNo.trimmed(), nDate))
        + QString(
        "<table width='100%' border='0' cellspacing='1' cellpadding='6' bgcolor='#94A3B8'>"
        "  <tr>"
        "    <td width='50%' bgcolor='#FFFFFF'><b>Party:</b><br><font size='3'><b>%1</b></font></td>"
        "    <td width='50%' bgcolor='#FFFFFF'><b>Original Invoice Ref:</b> %2</td>"
        "  </tr>"
        "  <tr>"
        "    <td bgcolor='#FFFFFF'><b>Taxable Amount:</b> %3<br><b>GST Adjustment:</b> %4</td>"
        "    <td bgcolor='#FFFFFF'><b>TOTAL ADJUSTMENT:</b><br><font size='3'><b>%5</b></font></td>"
        "  </tr>"
        "  <tr>"
        "    <td colspan='2' bgcolor='#FFFFFF'><b>Reason for Note:</b> %6</td>"
        "  </tr>"
        "</table>")
        .arg(pName.toHtmlEscaped())
        .arg((origInv.isEmpty() ? "N/A" : origInv).toHtmlEscaped())
        .arg(formatINR(taxable))
        .arg(formatINR(gst))
        .arg(formatINR(total))
        .arg((reason.isEmpty() ? "Price/Quality adjustment" : reason).toHtmlEscaped())
        + ErpPrint::voucherSignHtml(QStringList() << "Prepared By" << ErpPrint::forFirmLabel(firm));

    return ErpPrint::wrapPrintDocument(body);
}


} // namespace PrintReports
