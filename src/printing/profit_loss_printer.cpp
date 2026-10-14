#include "profit_loss_printer.h"
#include "reports/report_html_chrome.h"
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
    QVariantMap firmMap;
    firmMap["firm_name"] = data.firmName;
    firmMap["address"] = data.firmAddress;
    firmMap["gstin"] = data.firmGstin;
    const QString headerHtml = ErpPrint::letterheadHtml(firmMap)
        + ErpPrint::titleBlockHtml("TRADING AND PROFIT & LOSS ACCOUNT",
            QString("Period: %1 to %2 (%3)").arg(data.displayFromDate, data.displayToDate, data.financialYear));
    const QString signHtml = ErpPrint::signatoryHtml(data.firmName)
        + ErpPrint::generatedFooterHtml();
    auto formatGroupList = [](const QVector<ProfitLossItem>& groups) -> QString {
        QString outHtml = "";
        bool first = true;
        for (const auto& g : groups) {
            if (!first) {
                outHtml += "<tr><td colspan='2' bgcolor='#FFFFFF'>&nbsp;</td></tr>";
            }
            first = false;

            if (g.children.isEmpty()) {
                outHtml += QString(
                    "<tr>"
                    "<td align='left' valign='top' bgcolor='#FFFFFF'>%1</td>"
                    "<td align='right' nowrap style='white-space:nowrap;' bgcolor='#FFFFFF'>%2</td>"
                    "</tr>"
                ).arg((g.isCalculated || g.isGroup ? "<b>" + g.name.toHtmlEscaped() + "</b>" : g.name.toHtmlEscaped()), g.amountFmt);
            } else {
                outHtml += QString(
                    "<tr>"
                    "<td align='left' valign='top' bgcolor='#FFFFFF'><b>%1</b></td>"
                    "<td align='right' nowrap style='white-space:nowrap;' bgcolor='#FFFFFF'><b>%2</b></td>"
                    "</tr>"
                ).arg(g.name.toUpper().toHtmlEscaped(), g.amountFmt);

                for (const auto& c : g.children) {
                    outHtml += QString(
                        "<tr>"
                        "<td align='left' valign='top' bgcolor='#FFFFFF'>&nbsp;&nbsp;- %1</td>"
                        "<td align='right' nowrap style='white-space:nowrap;' bgcolor='#FFFFFF'>%2</td>"
                        "</tr>"
                    ).arg(c.name.toHtmlEscaped(), c.amountFmt);
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
        "  .lh{text-align:center;margin:0 0 2px 0;}"
        "  .lh-name{font-size:17pt;font-weight:800;}"
        "  .lh-line{font-size:9pt;color:#334155;}"
        "  .rule{border:none;border-top:2px solid #0F172A;margin:4px 0 1px 0;}"
        "  .rule2{border:none;border-top:1px solid #0F172A;margin:0 0 8px 0;}"
        "  .title{text-align:center;font-size:13pt;font-weight:800;margin:6px 0 1px 0;}"
        "  .period{text-align:center;font-size:9.5pt;color:#334155;margin:0 0 8px 0;}"
        "  .notes{font-size:8.5pt;color:#334155;margin:6px 0;}"
        "  .sign{width:100%;margin-top:18px;border-collapse:collapse;}"
        "  .sign td{width:33%;text-align:center;font-size:9pt;vertical-align:bottom;"
        "padding-top:28px;border:none;border-top:0.5pt solid #0F172A;}"
        "  .genfoot{margin-top:10px;font-size:7.5pt;color:#94A3B8;text-align:right;}"
        "</style></head><body>"
        ""
        "%1"
        ""
        "<table width='100%' border='0' cellspacing='0' cellpadding='0'>"
        "  <thead>"
        "    <tr>"
        "      <th width='50%' bgcolor='#0F172A' align='left'>"
        "        <table width='100%' border='0' cellspacing='0' cellpadding='0'>"
        "          <tr>"
        "            <td align='left'><font color='#FFFFFF'><b>EXPENSES / Dr PARTICULARS</b></font></td>"
        "            <td align='right'><font color='#FFFFFF'><b>AMOUNT (Rs.)</b></font></td>"
        "          </tr>"
        "        </table>"
        "      </th>"
        "      <th width='50%' bgcolor='#0F172A' align='left'>"
        "        <table width='100%' border='0' cellspacing='0' cellpadding='0'>"
        "          <tr>"
        "            <td align='left'><font color='#FFFFFF'><b>INCOMES / Cr PARTICULARS</b></font></td>"
        "            <td align='right'><font color='#FFFFFF'><b>AMOUNT (Rs.)</b></font></td>"
        "          </tr>"
        "        </table>"
        "      </th>"
        "    </tr>"
        "  </thead>"
        "  <tbody>"
        "    <tr>"
        "      <!-- EXPENSES SIDE (Left) -->"
        "      <td width='50%' valign='top'>"
        "        <table width='100%' border='0' cellspacing='1' cellpadding='4' bgcolor='#94A3B8'><tr><td align='center' bgcolor='#0F172A'><font color='#FFFFFF'><b>TRADING ACCOUNT (EXPENSES)</b></font></td></tr></table>"
        "        <table width='100%' border='0' cellspacing='1' cellpadding='4' bgcolor='#94A3B8'>"
        "          %2"
        "        </table>"
        "        <table width='100%' border='0' cellspacing='1' cellpadding='4' bgcolor='#94A3B8'>"
        "          <tr>"
        "            <td align='left' bgcolor='#F1F5F9'><b>TRADING A/C TOTAL:</b></td>"
        "            <td align='right' nowrap style='white-space:nowrap;' bgcolor='#F1F5F9'><b>%3</b></td>"
        "          </tr>"
        "        </table>"
        "        <table width='100%' border='0' cellspacing='1' cellpadding='4' bgcolor='#94A3B8'><tr><td align='center' bgcolor='#0F172A'><font color='#FFFFFF'><b>PROFIT &amp; LOSS ACCOUNT (EXPENSES)</b></font></td></tr></table>"
        "        <table width='100%' border='0' cellspacing='1' cellpadding='4' bgcolor='#94A3B8'>"
        "          %4"
        "        </table>"
        "        <table width='100%' border='0' cellspacing='1' cellpadding='4' bgcolor='#94A3B8'>"
        "          <tr>"
        "            <td align='left' bgcolor='#F1F5F9'><b>PROFIT &amp; LOSS A/C TOTAL:</b></td>"
        "            <td align='right' nowrap style='white-space:nowrap;' bgcolor='#F1F5F9'><b>%5</b></td>"
        "          </tr>"
        "        </table>"
        "      </td>"
        ""
        "      <!-- INCOMES SIDE (Right) -->"
        "      <td width='50%' valign='top'>"
        "        <table width='100%' border='0' cellspacing='1' cellpadding='4' bgcolor='#94A3B8'><tr><td align='center' bgcolor='#0F172A'><font color='#FFFFFF'><b>TRADING ACCOUNT (INCOMES)</b></font></td></tr></table>"
        "        <table width='100%' border='0' cellspacing='1' cellpadding='4' bgcolor='#94A3B8'>"
        "          %6"
        "        </table>"
        "        <table width='100%' border='0' cellspacing='1' cellpadding='4' bgcolor='#94A3B8'>"
        "          <tr>"
        "            <td align='left' bgcolor='#F1F5F9'><b>TRADING A/C TOTAL:</b></td>"
        "            <td align='right' nowrap style='white-space:nowrap;' bgcolor='#F1F5F9'><b>%7</b></td>"
        "          </tr>"
        "        </table>"
        "        <table width='100%' border='0' cellspacing='1' cellpadding='4' bgcolor='#94A3B8'><tr><td align='center' bgcolor='#0F172A'><font color='#FFFFFF'><b>PROFIT &amp; LOSS ACCOUNT (INCOMES)</b></font></td></tr></table>"
        "        <table width='100%' border='0' cellspacing='1' cellpadding='4' bgcolor='#94A3B8'>"
        "          %8"
        "        </table>"
        "        <table width='100%' border='0' cellspacing='1' cellpadding='4' bgcolor='#94A3B8'>"
        "          <tr>"
        "            <td align='left' bgcolor='#F1F5F9'><b>PROFIT &amp; LOSS A/C TOTAL:</b></td>"
        "            <td align='right' nowrap style='white-space:nowrap;' bgcolor='#F1F5F9'><b>%9</b></td>"
        "          </tr>"
        "        </table>"
        "      </td>"
        "    </tr>"
        "  </tbody>"
        "</table>"
        ""
        "<table width='100%' border='0' cellspacing='1' cellpadding='6' bgcolor='#94A3B8'>"
        "  <tr>"
        "    <td width='50%' align='left' bgcolor='#F1F5F9'>"
        "      <table width='100%' border='0' cellspacing='0' cellpadding='0'>"
        "        <tr>"
        "          <td align='left'><b>GROSS PROFIT:</b></td>"
        "          <td align='right' nowrap style='white-space:nowrap;' bgcolor='#F1F5F9'><b>%10</b></td>"
        "        </tr>"
        "      </table>"
        "    </td>"
        "    <td width='50%' align='right' bgcolor='#F1F5F9'>"
        "      <table width='100%' border='0' cellspacing='0' cellpadding='0'>"
        "        <tr>"
        "          <td align='left'><b>NET PROFIT:</b></td>"
        "          <td align='right' nowrap style='white-space:nowrap;' bgcolor='#F1F5F9'><b>%11</b></td>"
        "        </tr>"
        "      </table>"
        "    </td>"
        "  </tr>"
        "</table>"
        ""
        "<div class='notes'>Note: Prepared from books of account produced &amp; maintained. %12</div>"
        "%13"
        "</body></html>"
    )
    .arg(headerHtml)
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
    .arg(QString())
    .arg(signHtml);

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
