#include "profit_loss_printer.h"
#include "../engine/accounting_engine.h"
#include <QPrinter>
#include <QPrintDialog>
#include <QTextDocument>
#include <QPageLayout>
#include <QPageSize>
#include <QMarginsF>
#include <QDir>
#include <QFile>
#include <QTextStream>
#include <QDateTime>
#include <QDesktopServices>
#include <QUrl>
#include <algorithm>

QString ProfitLossPrinter::generateHtml(const ProfitLossData& data) {
    auto formatGroupList = [](const QVector<ProfitLossItem>& groups) -> QString {
        QString outHtml = "";
        bool first = true;
        for (const auto& g : groups) {
            if (!first) {
                outHtml += "<tr height='8'><td colspan='2' style='height: 8px; font-size: 1px;'>&nbsp;</td></tr>";
            }
            first = false;

            if (g.children.isEmpty()) {
                outHtml += QString(
                    "<tr>"
                    "<td align='left' class='%1'>%2</td>"
                    "<td align='right' class='num-amt %1'>%3</td>"
                    "</tr>"
                ).arg(g.isCalculated || g.isGroup ? "group-title" : "child-item", g.name, g.amountFmt);
            } else {
                outHtml += QString(
                    "<tr>"
                    "<td align='left' class='group-title'><b>%1</b></td>"
                    "<td align='right' class='num-amt group-title'><b>%2</b></td>"
                    "</tr>"
                ).arg(g.name.toUpper(), g.amountFmt);

                for (const auto& c : g.children) {
                    outHtml += QString(
                        "<tr>"
                        "<td align='left' class='child-item'>- %1</td>"
                        "<td align='right' class='num-amt'>%2</td>"
                        "</tr>"
                    ).arg(c.name, c.amountFmt);
                }
            }
        }
        return outHtml;
    };

    QString trDrHtml = formatGroupList(data.tradingDrGroups);
    QString trCrHtml = formatGroupList(data.tradingCrGroups);
    QString plDrHtml = formatGroupList(data.plDrGroups);
    QString plCrHtml = formatGroupList(data.plCrGroups);

    QString html = QString(
        "<!DOCTYPE html><html><head><meta charset='utf-8'><style>"
        "  body { font-family: 'Segoe UI', -apple-system, BlinkMacSystemFont, Roboto, 'Helvetica Neue', Arial, 'Liberation Sans', 'DejaVu Sans', sans-serif; font-size: 8.5pt; color: #000000; margin: 0; padding: 0; background-color: #ffffff; }"
        "  table { border-collapse: collapse; }"
        "  .main-table { width: 100%; border-collapse: collapse; margin-top: 4pt; }"
        "  .col-hdr-left { border-top: 1.5pt solid #000000; border-bottom: 1.5pt solid #000000; border-right: 1pt solid #000000; font-weight: bold; font-size: 8.5pt; padding: 4pt 8pt 4pt 4pt; background-color: #f8f8f8; }"
        "  .col-hdr-right { border-top: 1.5pt solid #000000; border-bottom: 1.5pt solid #000000; font-weight: bold; font-size: 8.5pt; padding: 4pt 4pt 4pt 8pt; background-color: #f8f8f8; }"
        "  .side-cell-left { width: 50%; vertical-align: top; border-right: 1pt solid #000000; padding-right: 8pt; padding-top: 4pt; }"
        "  .side-cell-right { width: 50%; vertical-align: top; padding-left: 8pt; padding-top: 4pt; }"
        "  .items-table { width: 100%; border-collapse: collapse; border: none; }"
        "  .items-table td { border: none; padding: 1.5pt 2pt; font-size: 8pt; }"
        "  .section-hdr { background-color: #eaeaea; font-weight: bold; text-align: center; font-size: 8pt; padding: 2.5pt 0; margin: 4pt 0 3pt 0; }"
        "  .group-title { font-weight: bold; padding-top: 2pt !important; text-decoration: underline; }"
        "  .child-item { padding-left: 10pt !important; }"
        "  .num-amt { font-family: 'Consolas', 'Menlo', 'Monaco', 'DejaVu Sans Mono', 'Liberation Mono', 'Courier New', monospace; font-weight: bold; text-align: right; white-space: nowrap; }"
        "  .subtotal-table { width: 100%; border-collapse: collapse; margin: 4pt 0; border-top: 1pt solid #000000; border-bottom: 2pt double #000000; background-color: #f4f4f4; }"
        "  .subtotal-table td { padding: 3pt 4pt; font-weight: bold; font-size: 8pt; }"
        "  .summary-box { width: 100%; margin-top: 8pt; border: 1.5pt solid #000000; background-color: #f8f8f8; border-collapse: collapse; }"
        "  .summary-box td { padding: 4pt 8pt; font-weight: bold; font-size: 8.5pt; }"
        "  .sign-table { width: 100%; margin-top: 20pt; border-collapse: collapse; border: none; }"
        "  .sign-box { border-top: 1pt solid #000000; padding-top: 3pt; min-width: 170px; text-align: center; font-size: 8.5pt; font-weight: bold; }"
        "</style></head><body>"
        ""
        "<div align='center' style='margin-bottom: 4pt;'>"
        "  <font size='4'><b>%1</b></font><br>"
        "  <font size='1'>%2 &nbsp;|&nbsp; <b>GSTIN:</b> %3</font><br>"
        "  <table border='1' cellspacing='0' cellpadding='2' style='margin-top: 3pt; display: inline-table;'>"
        "    <tr><td align='center'><b>TRADING AND PROFIT & LOSS ACCOUNT</b></td></tr>"
        "  </table><br>"
        "  <font size='1'><b>Period: %4 To %5 (Financial Year: %6)</b></font>"
        "</div>"
        ""
        "<table class='main-table' border='0' cellspacing='0' cellpadding='0'>"
        "  <thead>"
        "    <tr>"
        "      <th class='col-hdr-left' width='50%'>"
        "        <table width='100%' border='0' cellspacing='0' cellpadding='0'>"
        "          <tr>"
        "            <td align='left' style='font-weight: bold;'>EXPENSES / Dr PARTICULARS</td>"
        "            <td align='right' style='font-weight: bold;'>AMOUNT (₹)</td>"
        "          </tr>"
        "        </table>"
        "      </th>"
        "      <th class='col-hdr-right' width='50%'>"
        "        <table width='100%' border='0' cellspacing='0' cellpadding='0'>"
        "          <tr>"
        "            <td align='left' style='font-weight: bold;'>INCOMES / Cr PARTICULARS</td>"
        "            <td align='right' style='font-weight: bold;'>AMOUNT (₹)</td>"
        "          </tr>"
        "        </table>"
        "      </th>"
        "    </tr>"
        "  </thead>"
        "  <tbody>"
        "    <tr>"
        "      <!-- EXPENSES SIDE (Left) -->"
        "      <td class='side-cell-left'>"
        "        <div class='section-hdr'>TRADING ACCOUNT (EXPENSES)</div>"
        "        <table class='items-table' border='0' cellspacing='0' cellpadding='0'>"
        "          %7"
        "        </table>"
        "        <table class='subtotal-table' border='0' cellspacing='0' cellpadding='0'>"
        "          <tr>"
        "            <td align='left'>TRADING A/C TOTAL:</td>"
        "            <td align='right' class='num-amt'>₹ %8</td>"
        "          </tr>"
        "        </table>"
        "        <div class='section-hdr' style='margin-top: 8pt;'>PROFIT & LOSS ACCOUNT (EXPENSES)</div>"
        "        <table class='items-table' border='0' cellspacing='0' cellpadding='0'>"
        "          %9"
        "        </table>"
        "        <table class='subtotal-table' border='0' cellspacing='0' cellpadding='0'>"
        "          <tr>"
        "            <td align='left'>PROFIT & LOSS A/C TOTAL:</td>"
        "            <td align='right' class='num-amt'>₹ %10</td>"
        "          </tr>"
        "        </table>"
        "      </td>"
        ""
        "      <!-- INCOMES SIDE (Right) -->"
        "      <td class='side-cell-right'>"
        "        <div class='section-hdr'>TRADING ACCOUNT (INCOMES)</div>"
        "        <table class='items-table' border='0' cellspacing='0' cellpadding='0'>"
        "          %11"
        "        </table>"
        "        <table class='subtotal-table' border='0' cellspacing='0' cellpadding='0'>"
        "          <tr>"
        "            <td align='left'>TRADING A/C TOTAL:</td>"
        "            <td align='right' class='num-amt'>₹ %12</td>"
        "          </tr>"
        "        </table>"
        "        <div class='section-hdr' style='margin-top: 8pt;'>PROFIT & LOSS ACCOUNT (INCOMES)</div>"
        "        <table class='items-table' border='0' cellspacing='0' cellpadding='0'>"
        "          %13"
        "        </table>"
        "        <table class='subtotal-table' border='0' cellspacing='0' cellpadding='0'>"
        "          <tr>"
        "            <td align='left'>PROFIT & LOSS A/C TOTAL:</td>"
        "            <td align='right' class='num-amt'>₹ %14</td>"
        "          </tr>"
        "        </table>"
        "      </td>"
        "    </tr>"
        "  </tbody>"
        "</table>"
        ""
        "<table class='summary-box' border='0' cellspacing='0' cellpadding='0'>"
        "  <tr>"
        "    <td width='50%' align='left' style='border-right: 1.5pt solid #000000;'>"
        "      <table width='100%' border='0' cellspacing='0' cellpadding='0'>"
        "        <tr>"
        "          <td align='left' style='font-weight: bold;'>GROSS PROFIT:</td>"
        "          <td align='right' class='num-amt'>₹ %15</td>"
        "        </tr>"
        "      </table>"
        "    </td>"
        "    <td width='50%' align='right'>"
        "      <table width='100%' border='0' cellspacing='0' cellpadding='0'>"
        "        <tr>"
        "          <td align='left' style='font-weight: bold; padding-left: 8pt;'>NET PROFIT:</td>"
        "          <td align='right' class='num-amt'>₹ %16</td>"
        "        </tr>"
        "      </table>"
        "    </td>"
        "  </tr>"
        "</table>"
        ""
        "<table class='sign-table' border='0' cellspacing='0' cellpadding='0'>"
        "  <tr>"
        "    <td width='50%' valign='bottom' align='left'>"
        "      <font size='1'>Note: Prepared from books of account produced &amp; maintained.<br>Generated on: %17 &nbsp;|&nbsp; E. &amp; O.E.</font>"
        "    </td>"
        "    <td width='50%' align='right' valign='bottom'>"
        "      <table border='0' cellspacing='0' cellpadding='0' align='right'>"
        "        <tr><td align='center' style='font-size: 8.5pt; font-weight: bold;'>For %1</td></tr>"
        "        <tr><td height='60' style='height: 60px; font-size: 1px;'>&nbsp;</td></tr>"
        "        <tr>"
        "          <td align='center' class='sign-box'>"
        "            Authorised Signatory"
        "          </td>"
        "        </tr>"
        "      </table>"
        "    </td>"
        "  </tr>"
        "</table>"
        "</body></html>"
    )
    .arg(data.firmName)
    .arg(data.firmAddress)
    .arg(data.firmGstin)
    .arg(data.displayFromDate)
    .arg(data.displayToDate)
    .arg(data.financialYear)
    .arg(trDrHtml)
    .arg(data.totalTradingDrFmt)
    .arg(plDrHtml)
    .arg(data.totalPlDrFmt)
    .arg(trCrHtml)
    .arg(data.totalTradingCrFmt)
    .arg(plCrHtml)
    .arg(data.totalPlCrFmt)
    .arg(AccountingEngine::formatIndianCurrency(data.grossProfit, true))
    .arg(AccountingEngine::formatIndianCurrency(data.netProfit, true))
    .arg(QDateTime::currentDateTime().toString("dd-MM-yyyy hh:mm AP"));

    return html;
}

QString ProfitLossPrinter::exportPdf(const ProfitLossData& data, const QString& customPath) {
    QString outPath = customPath.trimmed();
    if (outPath.isEmpty()) {
        QString baseDir = QDir::homePath() + "/Documents/MahadevReports";
        QDir().mkpath(baseDir);
        QString dateTag = data.toDate;
        dateTag.remove('-');
        outPath = QString("%1/Trading_ProfitLoss_%2_%3.pdf").arg(baseDir, dateTag, QString::number(QDateTime::currentSecsSinceEpoch()));
    }

    QPrinter printer(QPrinter::ScreenResolution);
    printer.setOutputFormat(QPrinter::PdfFormat);
    printer.setOutputFileName(outPath);
    printer.setPageSize(QPageSize(QPageSize::A4));
    printer.setPageOrientation(QPageLayout::Portrait);
    printer.setPageMargins(QMarginsF(8, 8, 8, 8), QPageLayout::Millimeter);

    QTextDocument doc;
    doc.setDocumentMargin(0);
    doc.setPageSize(printer.pageLayout().paintRectPoints().size());
    doc.setHtml(generateHtml(data));
    doc.print(&printer);

    if (QFile::exists(outPath)) {
        QDesktopServices::openUrl(QUrl::fromLocalFile(outPath));
        return outPath;
    }
    return "";
}

QString ProfitLossPrinter::exportCsv(const ProfitLossData& data, const QString& customPath) {
    QString outPath = customPath.trimmed();
    if (outPath.isEmpty()) {
        QString baseDir = QDir::homePath() + "/Documents/MahadevReports";
        QDir().mkpath(baseDir);
        QString dateTag = data.toDate;
        dateTag.remove('-');
        outPath = QString("%1/Trading_ProfitLoss_%2_%3.csv").arg(baseDir, dateTag, QString::number(QDateTime::currentSecsSinceEpoch()));
    }

    QFile file(outPath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return "";
    }

    QTextStream ts(&file);
    ts << "\"TRADING AND PROFIT & LOSS STATEMENT - " << data.firmName << "\"\n";
    ts << "\"Period: " << data.displayFromDate << " to " << data.displayToDate << " (" << data.financialYear << ")\"\n\n";
    ts << "\"EXPENSES / DR PARTICULARS\",\"AMOUNT (₹)\",\"\",\"INCOMES / CR PARTICULARS\",\"AMOUNT (₹)\"\n";

    int maxRows = std::max(data.expensesSide.size(), data.incomesSide.size());
    for (int i = 0; i < maxRows; ++i) {
        QString expName = (i < data.expensesSide.size()) ? ("\"" + data.expensesSide[i].name + "\"") : "\"\"";
        QString expAmt = (i < data.expensesSide.size()) ? ("\"" + data.expensesSide[i].amountFmt + "\"") : "\"\"";
        QString incName = (i < data.incomesSide.size()) ? ("\"" + data.incomesSide[i].name + "\"") : "\"\"";
        QString incAmt = (i < data.incomesSide.size()) ? ("\"" + data.incomesSide[i].amountFmt + "\"") : "\"\"";
        ts << expName << "," << expAmt << ",\"\"," << incName << "," << incAmt << "\n";
    }

    ts << "\"GROSS PROFIT\",\"" << AccountingEngine::formatIndianCurrency(data.grossProfit, true) << "\",\"\","
       << "\"NET PROFIT\",\"" << AccountingEngine::formatIndianCurrency(data.netProfit, true) << "\"\n";

    file.close();
    return outPath;
}

bool ProfitLossPrinter::print(const ProfitLossData& data, QWidget* parent) {
    QPrinter printer(QPrinter::ScreenResolution);
    printer.setDocName(QString("Trading and Profit Loss - %1 to %2").arg(data.displayFromDate, data.displayToDate));
    printer.setPageSize(QPageSize(QPageSize::A4));
    printer.setPageOrientation(QPageLayout::Portrait);
    printer.setPageMargins(QMarginsF(8, 8, 8, 8), QPageLayout::Millimeter);

    QPrintDialog dialog(&printer, parent);
    dialog.setWindowTitle("Print Trading & Profit Loss Statement");
    if (dialog.exec() == QDialog::Accepted) {
        QTextDocument doc;
        doc.setDocumentMargin(0);
        doc.setPageSize(printer.pageLayout().paintRectPoints().size());
        doc.setHtml(generateHtml(data));
        doc.print(&printer);
        return true;
    }
    return false;
}
