#include "balance_sheet_printer.h"
#include "reports/report_html_chrome.h"
#include "reports/balance_sheet_ca.h"
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
    // CA-audited stationery (shared builder): serif, underline-ruled,
    // borderless sides, full ledger lists, firm-only footer. Figures come
    // straight from BalanceSheetData, so totals match the widget exactly.
    QString dots = data.displayAsOnDate.trimmed();
    if (dots.isEmpty()) dots = data.asOnDate;
    dots.replace("-", ".");
    return ErpPrint::balanceSheetCaHtml(data.firmName, dots,
                                        data.liabilitiesGroups,
                                        data.assetsGroups,
                                        data.totalLiabilities,
                                        data.totalAssets);
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
