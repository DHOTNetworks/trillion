#include "cheque_voucher_report.h"
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

QString renderChequeVoucherHtml(const QString& voucherNo, const QString& voucherType) {
    auto firm = getFirmProfile();

    QVariantList txRows = DatabaseManager::instance().executeQuery(
        "SELECT * FROM transactions WHERE voucher_no = ? ORDER BY id ASC;",
        {voucherNo.trimmed()}
    );
    if (txRows.isEmpty()) {
        return "<h2>Bank voucher not found: " + voucherNo + "</h2>";
    }

    QVariantMap first = txRows.first().toMap();
    QString vDate = FiscalYearHelper::formatDisplayDate(first.value("voucher_date").toString());
    QString pName = first.value("party_name").toString();
    QString oppAcc = first.value("opposing_account").toString();
    double amt = first.value("amount").toDouble();
    QString narr = first.value("narration").toString();
    QString bDate = FiscalYearHelper::formatDisplayDate(first.value("bank_date").toString());
    QString chqNo = first.value("invoice_no").toString();

    QString vType = voucherType.trimmed();
    if (vType.isEmpty()) vType = "Bank Voucher";

    QString body = ErpPrint::letterheadHtml(firm)
        + ErpPrint::titleBlockHtml(vType.toUpper(),
            QString("Voucher No: %1  |  Voucher Date: %2").arg(voucherNo.trimmed(), vDate))
        + QString(
        "<table width='100%' border='0' cellspacing='1' cellpadding='6' bgcolor='#94A3B8'>"
        "  <tr>"
        "    <td width='50%' bgcolor='#FFFFFF'><b>Party / Beneficiary:</b><br><font size='3'><b>%1</b></font></td>"
        "    <td width='50%' bgcolor='#FFFFFF'><b>Bank Account:</b><br><b>%2</b></td>"
        "  </tr>"
        "  <tr>"
        "    <td bgcolor='#FFFFFF'><b>Cheque / Ref No:</b> %3</td>"
        "    <td bgcolor='#FFFFFF'><b>Cheque / Value Date:</b> %4</td>"
        "  </tr>"
        "  <tr>"
        "    <td colspan='2' bgcolor='#FFFFFF'><b>Amount:</b> <font size='3'><b>%5</b></font><br><span class='words'>%6</span></td>"
        "  </tr>"
        "  <tr>"
        "    <td colspan='2' bgcolor='#FFFFFF'><b>Narration / Particulars:</b><br>%7</td>"
        "  </tr>"
        "</table>")
        .arg(pName.toHtmlEscaped())
        .arg(oppAcc.toHtmlEscaped())
        .arg((chqNo.isEmpty() ? "N/A" : chqNo).toHtmlEscaped())
        .arg(bDate.isEmpty() ? vDate : bDate)
        .arg(formatINR(amt))
        .arg(amountInWords(amt).toHtmlEscaped())
        .arg((narr.isEmpty() ? "Bank transaction" : narr).toHtmlEscaped())
        + ErpPrint::voucherSignHtml(QStringList() << "Prepared By" << ErpPrint::forFirmLabel(firm));

    return ErpPrint::wrapPrintDocument(body);
}


} // namespace PrintReports
