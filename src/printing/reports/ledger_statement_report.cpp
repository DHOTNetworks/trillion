#include "ledger_statement_report.h"
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

QString renderLedgerStatementHtml(const QString& partyName, const QString& fromDate, const QString& toDate) {
    auto firm = getFirmProfile();

    QString fDate = fromDate.trimmed();
    QString tDate = toDate.trimmed();
    if (fDate.isEmpty() && tDate.isEmpty()) {
        fDate = AccountingEngine::getActiveFromDate();
        tDate = AccountingEngine::getActiveToDate();
    }

    QString pName = partyName.trimmed();
    PartitionedLedgerData partData = FiscalYearHelper::partitionPartyTransactions(pName, fDate, tDate);

    auto pProfile = getPartyProfile(0, pName);
    QString pPlace = pProfile.value("city").toString();
    QString pGstin = pProfile.value("gstin").toString();
    QString pPhone = pProfile.value("phone").toString();

    QString drRowsHtml = "";
    double drTotal = 0.0;
    int maxRows = std::max(partData.drEntries.size(), partData.crEntries.size());

    // Memory guard (same class as the day-book render cap): a multi-year
    // khata prints at most MaxPrintLegs legs per side; totals below always
    // cover the FULL statement so money never lies.
    static const int MaxPrintLegs = 5000;
    const int drCount = partData.drEntries.size();
    const int crCount = partData.crEntries.size();

    for (const auto& e : partData.drEntries) drTotal += e.amount;
    for (int ei = 0; ei < partData.drEntries.size() && ei < MaxPrintLegs; ++ei) {
        const auto& e = partData.drEntries.at(ei);
        QString dt = FiscalYearHelper::formatDisplayDate(e.vDate);
        QString vNo = (e.id == -1) ? "" : (!e.refNo.isEmpty() ? e.refNo : e.voucherNo);
        QString desc = e.particulars;
        if (e.id == -1) desc = "<b>OPENING BALANCE</b>";

        drRowsHtml += QString(
            "<tr>"
            "<td align='center' bgcolor='#FFFFFF'>%1</td>"
            "<td align='center' bgcolor='#FFFFFF'>%2</td>"
            "<td bgcolor='#FFFFFF'>%3</td>"
            "<td align='right' bgcolor='#FFFFFF'>%4</td>"
            "</tr>"
        )
        .arg(dt.toHtmlEscaped())
        .arg(vNo.toHtmlEscaped())
        .arg(desc)
        .arg(formatINR(e.amount));
    }

    QString crRowsHtml = "";
    double crTotal = 0.0;
    for (const auto& e : partData.crEntries) crTotal += e.amount;
    for (int ei = 0; ei < partData.crEntries.size() && ei < MaxPrintLegs; ++ei) {
        const auto& e = partData.crEntries.at(ei);
        QString dt = FiscalYearHelper::formatDisplayDate(e.vDate);
        QString vNo = (e.id == -1) ? "" : (!e.refNo.isEmpty() ? e.refNo : e.voucherNo);
        QString desc = e.particulars;
        if (e.id == -1) desc = "<b>OPENING BALANCE</b>";

        crRowsHtml += QString(
            "<tr>"
            "<td align='center' bgcolor='#FFFFFF'>%1</td>"
            "<td align='center' bgcolor='#FFFFFF'>%2</td>"
            "<td bgcolor='#FFFFFF'>%3</td>"
            "<td align='right' bgcolor='#FFFFFF'>%4</td>"
            "</tr>"
        )
        .arg(dt.toHtmlEscaped())
        .arg(vNo.toHtmlEscaped())
        .arg(desc)
        .arg(formatINR(e.amount));
    }

    double netBal = std::abs(drTotal - crTotal);
    QString netBalType = (drTotal >= crTotal) ? "Dr (Receivable / Debit)" : "Cr (Payable / Credit)";

    const QString acctMeta = QString("<table width='100%' border='0' cellspacing='1' cellpadding='6' bgcolor='#94A3B8'><tr>"
                  "<td width='60%' bgcolor='#FFFFFF'><b>Account:</b> %1<br>City/Station: %2 &nbsp;|&nbsp; Phone: %3</td>"
                  "<td width='40%' bgcolor='#FFFFFF'><b>GSTIN:</b> %4</td>"
                  "</tr></table>")
        .arg(pName.toHtmlEscaped(),
             (pPlace.isEmpty() ? "Local" : pPlace).toHtmlEscaped(),
             pPhone.toHtmlEscaped(),
             (pGstin.isEmpty() ? "URP / Unregistered" : pGstin).toHtmlEscaped());
    const QString drNote = drCount > MaxPrintLegs
        ? QString("<tr><td colspan='4' align='center' bgcolor='#FFFFFF'>First %1 of %2 debit legs shown — narrow the period for the rest.</td></tr>")
              .arg(MaxPrintLegs).arg(drCount)
        : QString();
    const QString crNote = crCount > MaxPrintLegs
        ? QString("<tr><td colspan='4' align='center' bgcolor='#FFFFFF'>First %1 of %2 credit legs shown — narrow the period for the rest.</td></tr>")
              .arg(MaxPrintLegs).arg(crCount)
        : QString();
    const QString body = ErpPrint::letterheadHtml(firm)
        + ErpPrint::titleBlockHtml("STATEMENT OF ACCOUNT (KHATA)",
            QString("%1  |  Period: %2 to %3").arg(
                pName, FiscalYearHelper::formatDisplayDate(fDate),
                FiscalYearHelper::formatDisplayDate(tDate)))
        + acctMeta
        + QString("<table width='100%' border='0' cellspacing='1' cellpadding='4' bgcolor='#94A3B8' style='font-size:9pt'>"
                  "<tr><th colspan='4' bgcolor='#0F172A'><font color='#FFFFFF'><b>DEBIT (JAMA)</b></font></th></tr>"
                  "<tr><th width='15%' bgcolor='#0F172A'><font color='#FFFFFF'><b>Date</b></font></th><th width='15%' bgcolor='#0F172A'><font color='#FFFFFF'><b>Vch No</b></font></th>"
                  "<th width='46%' bgcolor='#0F172A'><font color='#FFFFFF'><b>Particulars</b></font></th><th width='24%' bgcolor='#0F172A'><font color='#FFFFFF'><b>Debit (Rs.)</b></font></th></tr>"
                  "%1"
                  "<tr bgcolor='#F1F5F9'><td colspan='3' align='right' bgcolor='#F1F5F9'>Total Debit (full period)</td>"
                  "<td align='right' bgcolor='#F1F5F9'>%2</td></tr>%3</table>"
                  "<table width='100%' border='0' cellspacing='1' cellpadding='4' bgcolor='#94A3B8' style='font-size:9pt'>"
                  "<tr><th colspan='4' bgcolor='#0F172A'><font color='#FFFFFF'><b>CREDIT (NAMA)</b></font></th></tr>"
                  "<tr><th width='15%' bgcolor='#0F172A'><font color='#FFFFFF'><b>Date</b></font></th><th width='15%' bgcolor='#0F172A'><font color='#FFFFFF'><b>Vch No</b></font></th>"
                  "<th width='46%' bgcolor='#0F172A'><font color='#FFFFFF'><b>Particulars</b></font></th><th width='24%' bgcolor='#0F172A'><font color='#FFFFFF'><b>Credit (Rs.)</b></font></th></tr>"
                  "%4"
                  "<tr bgcolor='#F1F5F9'><td colspan='3' align='right' bgcolor='#F1F5F9'>Total Credit (full period)</td>"
                  "<td align='right' bgcolor='#F1F5F9'>%5</td></tr>%6</table>"
                  "<table width='100%' border='0' cellspacing='1' cellpadding='4' bgcolor='#94A3B8' style='font-size:9pt'>"
                  "<tr bgcolor='#F1F5F9'><td width='60%' bgcolor='#F1F5F9'>NET CLOSING BALANCE: %7</td>"
                  "<td width='40%' align='right' bgcolor='#F1F5F9'>%8</td></tr></table>")
            .arg(drRowsHtml, formatINR(drTotal), drNote,
                 crRowsHtml, formatINR(crTotal), crNote,
                 formatINR(netBal), netBalType.toHtmlEscaped())
        + ErpPrint::signatoryHtml(firm.value("firm_name").toString())
        + ErpPrint::generatedFooterHtml();

    return ErpPrint::wrapPrintDocument(body);
}


void buildLedgerCsvDocument(const QString& partyName, const QString& fromDate, const QString& toDate, QStringList* headersOut, QVector<QStringList>* rowsOut, QString* fileNameOut) {

    QString pName = partyName.trimmed();
    PartitionedLedgerData partData = FiscalYearHelper::partitionPartyTransactions(pName, fromDate, toDate);

    QStringList headers = {"Side", "Date", "Voucher No", "Voucher Type", "Particulars", "Debit", "Credit"};
    QVector<QStringList> rows;

    for (const auto& e : partData.drEntries) {
        rows.append({
            "Dr",
            FiscalYearHelper::formatDisplayDate(e.vDate),
            (e.id == -1) ? "OP-BAL" : (!e.refNo.isEmpty() ? e.refNo : e.voucherNo),
            (e.id == -1) ? "Opening Balance" : e.transType,
            e.particulars,
            QString::number(e.amount, 'f', 2),
            "0.00"
        });
    }

    for (const auto& e : partData.crEntries) {
        rows.append({
            "Cr",
            FiscalYearHelper::formatDisplayDate(e.vDate),
            (e.id == -1) ? "OP-BAL" : (!e.refNo.isEmpty() ? e.refNo : e.voucherNo),
            (e.id == -1) ? "Opening Balance" : e.transType,
            e.particulars,
            "0.00",
            QString::number(e.amount, 'f', 2)
        });
    }

    QString safeName = pName;
    safeName.replace(" ", "_").replace("/", "_");
    
    *headersOut = headers;
    *rowsOut = rows;
    *fileNameOut = "Ledger_" + safeName + ".csv";
    return;
}

} // namespace PrintReports
