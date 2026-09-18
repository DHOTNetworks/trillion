#include "profit_loss_controller.h"
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

ProfitLossController::ProfitLossController(PrintExportController* printExportCtrl, QObject* parent)
    : QObject(parent)
    , m_printExportCtrl(printExportCtrl)
{
    reload();
}

void ProfitLossController::reload(const QString& requestedFromDate, const QString& requestedToDate) {
    m_data = ProfitLossCalculator::calculate(requestedFromDate, requestedToDate);
    emit dataChanged();
    emit totalsChanged();
}

QString ProfitLossController::grossProfitFmt() const {
    double amt = (m_data.grossProfit > 0.0) ? m_data.grossProfit : m_data.grossLoss;
    return AccountingEngine::formatIndianCurrency(amt, true);
}

QString ProfitLossController::netProfitFmt() const {
    double amt = (m_data.netProfit > 0.0) ? m_data.netProfit : m_data.netLoss;
    return AccountingEngine::formatIndianCurrency(amt, true);
}

#include "../printing/profit_loss_printer.h"

QString ProfitLossController::renderHtml() const {
    return ProfitLossPrinter::generateHtml(m_data);
}

QString ProfitLossController::exportPdf(const QString& customPath) {
    QString outPath = ProfitLossPrinter::exportPdf(m_data, customPath);
    emit exportCompleted(!outPath.isEmpty(), outPath);
    return outPath;
}

QString ProfitLossController::exportCsv(const QString& customPath) {
    QString outPath = ProfitLossPrinter::exportCsv(m_data, customPath);
    emit exportCompleted(!outPath.isEmpty(), outPath);
    return outPath;
}

bool ProfitLossController::print() {
    return ProfitLossPrinter::print(m_data, nullptr);
}
