#include "balance_sheet_printer.h"
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

QString BalanceSheetPrinter::generateHtml(const BalanceSheetData& data) {
    QString liabRowsHtml = "";
    bool firstLiab = true;
    for (const auto& g : data.liabilitiesGroups) {
        if (!firstLiab) {
            liabRowsHtml += "<tr height='8'><td colspan='2' style='height: 8px; font-size: 1px;'>&nbsp;</td></tr>";
        }
        firstLiab = false;

        liabRowsHtml += QString(
            "<tr>"
            "<td align='left' class='group-title'><b>%1</b></td>"
            "<td align='right' class='num-amt group-title'><b>%2</b></td>"
            "</tr>"
        ).arg(g.name.toUpper(), g.amountFmt);

        for (const auto& c : g.children) {
            liabRowsHtml += QString(
                "<tr>"
                "<td align='left' class='child-item'>- %1</td>"
                "<td align='right' class='num-amt'>%2</td>"
                "</tr>"
            ).arg(c.name, c.amountFmt);
        }
    }

    QString assetRowsHtml = "";
    bool firstAsset = true;
    for (const auto& g : data.assetsGroups) {
        if (!firstAsset) {
            assetRowsHtml += "<tr height='8'><td colspan='2' style='height: 8px; font-size: 1px;'>&nbsp;</td></tr>";
        }
        firstAsset = false;

        assetRowsHtml += QString(
            "<tr>"
            "<td align='left' class='group-title'><b>%1</b></td>"
            "<td align='right' class='num-amt group-title'><b>%2</b></td>"
            "</tr>"
        ).arg(g.name.toUpper(), g.amountFmt);

        for (const auto& c : g.children) {
            assetRowsHtml += QString(
                "<tr>"
                "<td align='left' class='child-item'>- %1</td>"
                "<td align='right' class='num-amt'>%2</td>"
                "</tr>"
            ).arg(c.name, c.amountFmt);
        }
    }

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
        "  .group-title { font-weight: bold; padding-top: 2pt !important; text-decoration: underline; }"
        "  .child-item { padding-left: 10pt !important; }"
        "  .num-amt { font-family: 'Consolas', 'Menlo', 'Monaco', 'DejaVu Sans Mono', 'Liberation Mono', 'Courier New', monospace; font-weight: bold; text-align: right; white-space: nowrap; }"
        "  .tot-table { width: 100%; border-collapse: collapse; margin-top: 6pt; border-top: 1.5pt solid #000000; border-bottom: 3.5pt double #000000; background-color: #f8f8f8; }"
        "  .sign-table { width: 100%; margin-top: 20pt; border-collapse: collapse; border: none; }"
        "  .sign-box { border-top: 1pt solid #000000; padding-top: 3pt; min-width: 170px; text-align: center; font-size: 8.5pt; font-weight: bold; }"
        "</style></head><body>"
        ""
        "<div align='center' style='margin-bottom: 4pt;'>"
        "  <font size='4'><b>%1</b></font><br>"
        "  <font size='1'>%2 &nbsp;|&nbsp; <b>GSTIN:</b> %3</font><br>"
        "  <table border='1' cellspacing='0' cellpadding='2' style='margin-top: 3pt; display: inline-table;'>"
        "    <tr><td align='center'><b>BALANCE SHEET AS ON %4</b></td></tr>"
        "  </table><br>"
        "  <font size='1'><b>(Financial Year: %5)</b></font>"
        "</div>"
        ""
        "<table class='main-table' border='0' cellspacing='0' cellpadding='0'>"
        "  <thead>"
        "    <tr>"
        "      <th class='col-hdr-left' width='50%'>"
        "        <table width='100%' border='0' cellspacing='0' cellpadding='0'>"
        "          <tr>"
        "            <td align='left' style='font-weight: bold;'>LIABILITIES</td>"
        "            <td align='right' style='font-weight: bold;'>AMOUNT (₹)</td>"
        "          </tr>"
        "        </table>"
        "      </th>"
        "      <th class='col-hdr-right' width='50%'>"
        "        <table width='100%' border='0' cellspacing='0' cellpadding='0'>"
        "          <tr>"
        "            <td align='left' style='font-weight: bold;'>ASSETS</td>"
        "            <td align='right' style='font-weight: bold;'>AMOUNT (₹)</td>"
        "          </tr>"
        "        </table>"
        "      </th>"
        "    </tr>"
        "  </thead>"
        "  <tbody>"
        "    <tr>"
        "      <td class='side-cell-left'>"
        "        <table class='items-table' border='0' cellspacing='0' cellpadding='0'>"
        "          %6"
        "        </table>"
        "      </td>"
        "      <td class='side-cell-right'>"
        "        <table class='items-table' border='0' cellspacing='0' cellpadding='0'>"
        "          %7"
        "        </table>"
        "      </td>"
        "    </tr>"
        "  </tbody>"
        "</table>"
        ""
        "<table class='tot-table' border='0' cellspacing='0' cellpadding='0'>"
        "  <tr>"
        "    <td width='50%' valign='middle' style='border-right: 1pt solid #000000; padding: 4pt 8pt 4pt 4pt;'>"
        "      <table width='100%' border='0' cellspacing='0' cellpadding='0'>"
        "        <tr>"
        "          <td align='left' style='font-weight: bold; font-size: 8.5pt;'>GRAND TOTAL:</td>"
        "          <td align='right' class='num-amt' style='font-size: 8.5pt;'>₹ %8</td>"
        "        </tr>"
        "      </table>"
        "    </td>"
        "    <td width='50%' valign='middle' style='padding: 4pt 4pt 4pt 8pt;'>"
        "      <table width='100%' border='0' cellspacing='0' cellpadding='0'>"
        "        <tr>"
        "          <td align='left' style='font-weight: bold; font-size: 8.5pt;'>GRAND TOTAL:</td>"
        "          <td align='right' class='num-amt' style='font-size: 8.5pt;'>₹ %9</td>"
        "        </tr>"
        "      </table>"
        "    </td>"
        "  </tr>"
        "</table>"
        ""
        "<table class='sign-table' border='0' cellspacing='0' cellpadding='0'>"
        "  <tr>"
        "    <td width='50%' valign='bottom' align='left'>"
        "      <font size='1'>Difference: ₹ %10<br>Generated on: %11 &nbsp;|&nbsp; E. &amp; O.E.</font>"
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
    .arg(data.displayAsOnDate)
    .arg(data.financialYear)
    .arg(liabRowsHtml)
    .arg(assetRowsHtml)
    .arg(data.totalLiabilitiesFmt)
    .arg(data.totalAssetsFmt)
    .arg(AccountingEngine::formatIndianCurrency(data.difference, true))
    .arg(QDateTime::currentDateTime().toString("dd-MM-yyyy hh:mm AP"));

    return html;
}

QString BalanceSheetPrinter::exportPdf(const BalanceSheetData& data, const QString& customPath) {
    QString outPath = customPath.trimmed();
    if (outPath.isEmpty()) {
        QString baseDir = QDir::homePath() + "/Documents/MahadevReports";
        QDir().mkpath(baseDir);
        QString dateTag = data.asOnDate;
        dateTag.remove('-');
        outPath = QString("%1/Balance_Sheet_%2_%3.pdf").arg(baseDir, dateTag, QString::number(QDateTime::currentSecsSinceEpoch()));
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

QString BalanceSheetPrinter::exportCsv(const BalanceSheetData& data, const QString& customPath) {
    QString outPath = customPath.trimmed();
    if (outPath.isEmpty()) {
        QString baseDir = QDir::homePath() + "/Documents/MahadevReports";
        QDir().mkpath(baseDir);
        QString dateTag = data.asOnDate;
        dateTag.remove('-');
        outPath = QString("%1/Balance_Sheet_%2_%3.csv").arg(baseDir, dateTag, QString::number(QDateTime::currentSecsSinceEpoch()));
    }

    QFile file(outPath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return "";
    }

    QTextStream ts(&file);
    ts << "\"BALANCE SHEET - " << data.firmName << "\"\n";
    ts << "\"As on: " << data.displayAsOnDate << " (" << data.financialYear << ")\"\n\n";
    ts << "\"LIABILITIES\",\"AMOUNT (₹)\",\"\",\"ASSETS\",\"AMOUNT (₹)\"\n";

    int maxGroups = std::max(data.liabilitiesGroups.size(), data.assetsGroups.size());
    for (int i = 0; i < maxGroups; ++i) {
        QString lName = (i < data.liabilitiesGroups.size()) ? ("\"" + data.liabilitiesGroups[i].name + "\"") : "\"\"";
        QString lAmt = (i < data.liabilitiesGroups.size()) ? ("\"" + data.liabilitiesGroups[i].amountFmt + "\"") : "\"\"";
        QString aName = (i < data.assetsGroups.size()) ? ("\"" + data.assetsGroups[i].name + "\"") : "\"\"";
        QString aAmt = (i < data.assetsGroups.size()) ? ("\"" + data.assetsGroups[i].amountFmt + "\"") : "\"\"";
        ts << lName << "," << lAmt << ",\"\"," << aName << "," << aAmt << "\n";
    }

    ts << "\"GRAND TOTAL\",\"" << data.totalLiabilitiesFmt << "\",\"\","
       << "\"GRAND TOTAL\",\"" << data.totalAssetsFmt << "\"\n";

    file.close();
    return outPath;
}

bool BalanceSheetPrinter::print(const BalanceSheetData& data, QWidget* parent) {
    QPrinter printer(QPrinter::ScreenResolution);
    printer.setDocName("Balance Sheet - " + data.displayAsOnDate);
    printer.setPageSize(QPageSize(QPageSize::A4));
    printer.setPageOrientation(QPageLayout::Portrait);
    printer.setPageMargins(QMarginsF(8, 8, 8, 8), QPageLayout::Millimeter);

    QPrintDialog dialog(&printer, parent);
    dialog.setWindowTitle("Print Balance Sheet");
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
