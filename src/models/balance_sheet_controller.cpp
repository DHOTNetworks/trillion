#include "balance_sheet_controller.h"
#include "../engine/accounting_engine.h"
#include <QFile>
#include <QTextStream>
#include <QDir>
#include <QDateTime>
#include <QDesktopServices>
#include <QUrl>
#include <QPrinter>
#include <QPrintDialog>
#include <QTextDocument>

BalanceSheetController::BalanceSheetController(PrintExportController* printExportCtrl, QObject* parent)
    : QObject(parent)
    , m_printExportCtrl(printExportCtrl)
{
    reload();
}

void BalanceSheetController::reload(const QString& requestedAsOnDate) {
    m_data = BalanceSheetCalculator::calculate(requestedAsOnDate);
    emit dataChanged();
    emit totalsChanged();
}

QString BalanceSheetController::differenceFmt() const {
    return AccountingEngine::formatIndianCurrency(m_data.difference, true);
}

QString BalanceSheetController::netProfitFmt() const {
    return AccountingEngine::formatIndianCurrency(std::abs(m_data.netProfit), true);
}

QString BalanceSheetController::closingStockValueFmt() const {
    return AccountingEngine::formatIndianCurrency(m_data.closingStockValue, true);
}

#include "../printing/balance_sheet_printer.h"

QString BalanceSheetController::renderHtml() const {
    return BalanceSheetPrinter::generateHtml(m_data);
}

QString BalanceSheetController::exportPdf(const QString& customPath) {
    QString outPath = BalanceSheetPrinter::exportPdf(m_data, customPath);
    emit exportCompleted(!outPath.isEmpty(), outPath);
    return outPath;
}

QString BalanceSheetController::exportCsv(const QString& customPath) {
    QString outPath = BalanceSheetPrinter::exportCsv(m_data, customPath);
    emit exportCompleted(!outPath.isEmpty(), outPath);
    return outPath;
}

bool BalanceSheetController::print() {
    return BalanceSheetPrinter::print(m_data, nullptr);
}
