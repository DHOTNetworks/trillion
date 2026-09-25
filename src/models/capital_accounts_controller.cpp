#include "capital_accounts_controller.h"
#include "../database_manager.h"
#include "../engine/fiscal_year_helper.h"
#include "../engine/profit_loss_calculator.h"
#include "../services/financial_math_service.h"
#include <cmath>
#include <algorithm>

namespace MahadevERP {

static QString formatINR(double val) {
    return FinancialMathService::instance().formatInr(val, false);
}

CapitalAccountsController::CapitalAccountsController(QObject* parent)
    : QObject(parent)
{
    FiscalYearInfo activeFy = FiscalYearHelper::getActiveFiscalYear();
    m_fromDate = QDate::fromString(activeFy.startDate, "yyyy-MM-dd");
    m_toDate = QDate::fromString(activeFy.endDate, "yyyy-MM-dd");
}

void CapitalAccountsController::setDateRange(const QDate& fromDate, const QDate& toDate) {
    m_fromDate = fromDate;
    m_toDate = toDate;
    calculate();
}

void CapitalAccountsController::reload() {
    calculate();
}

void CapitalAccountsController::calculate() {
    m_items.clear();
    m_totals = CapitalAccountTotals();

    DatabaseManager& db = DatabaseManager::instance();
    QString fromIso = m_fromDate.isValid() ? m_fromDate.toString("yyyy-MM-dd") : "2025-04-01";
    QString toIso = m_toDate.isValid() ? m_toDate.toString("yyyy-MM-dd") : "2026-03-31";

    // 1. Fetch all ledgers in Capital A/c group
    QVariantList capRows = db.executeQuery(
        "SELECT id, name, group_name, opening_balance, COALESCE(balance_type, 'Dr') AS dr_cr FROM parties "
        "WHERE group_name LIKE '%Capital%' OR name LIKE '%Capital%' ORDER BY name ASC;"
    );

    // 2. Compute Net Profit from P&L Engine
    ProfitLossData pnl = ProfitLossCalculator::calculate(fromIso, toIso);
    double totalNetProfit = pnl.netProfit;
    int partnerCount = std::max(1, (int)capRows.size());
    double profitSharePerPartner = totalNetProfit / partnerCount;

    for (const auto& v : capRows) {
        QVariantMap m = v.toMap();
        CapitalAccountItem item;
        item.ledgerId = m.value("id").toInt();
        item.partnerName = m.value("name").toString().trimmed();

        // Opening Balance
        double initialOp = m.value("opening_balance").toDouble();
        QString initialDrCr = m.value("dr_cr").toString().trimmed();
        double netOp = (initialDrCr.compare("Cr", Qt::CaseInsensitive) == 0 ? initialOp : -initialOp);

        QVariant priorDr = db.executeScalar(
            "SELECT SUM(amount) FROM transactions WHERE (party_id = ? OR party_name = ?) AND voucher_date < ? AND dr_cr = 'Dr';",
            {item.ledgerId, item.partnerName, fromIso}
        );
        QVariant priorCr = db.executeScalar(
            "SELECT SUM(amount) FROM transactions WHERE (party_id = ? OR party_name = ?) AND voucher_date < ? AND dr_cr = 'Cr';",
            {item.ledgerId, item.partnerName, fromIso}
        );
        double pDr = priorDr.isValid() ? priorDr.toDouble() : 0.0;
        double pCr = priorCr.isValid() ? priorCr.toDouble() : 0.0;
        item.opCapital = netOp + pCr - pDr;

        // Current Period Additions (Cr) and Drawings (Dr)
        QVariant perDr = db.executeScalar(
            "SELECT SUM(amount) FROM transactions WHERE (party_id = ? OR party_name = ?) AND voucher_date >= ? AND voucher_date <= ? AND dr_cr = 'Dr';",
            {item.ledgerId, item.partnerName, fromIso, toIso}
        );
        QVariant perCr = db.executeScalar(
            "SELECT SUM(amount) FROM transactions WHERE (party_id = ? OR party_name = ?) AND voucher_date >= ? AND voucher_date <= ? AND dr_cr = 'Cr';",
            {item.ledgerId, item.partnerName, fromIso, toIso}
        );

        item.drawings = perDr.isValid() ? perDr.toDouble() : 0.0;
        item.additions = perCr.isValid() ? perCr.toDouble() : 0.0;
        item.profitShare = profitSharePerPartner;
        item.interest = 0.0; // Standard 0 unless specified

        item.closingCapital = item.opCapital + item.additions - item.drawings + item.profitShare + item.interest;

        item.opCapitalFmt = formatINR(item.opCapital);
        item.additionsFmt = formatINR(item.additions);
        item.drawingsFmt = formatINR(item.drawings);
        item.profitShareFmt = formatINR(item.profitShare);
        item.interestFmt = formatINR(item.interest);
        item.closingCapitalFmt = formatINR(item.closingCapital);

        m_items.append(item);

        m_totals.totalOpCapital += item.opCapital;
        m_totals.totalAdditions += item.additions;
        m_totals.totalDrawings += item.drawings;
        m_totals.totalProfitShare += item.profitShare;
        m_totals.totalInterest += item.interest;
        m_totals.totalClosingCapital += item.closingCapital;
    }

    m_totals.totalPartners = m_items.size();
    m_totals.totalOpCapitalFmt = formatINR(m_totals.totalOpCapital);
    m_totals.totalAdditionsFmt = formatINR(m_totals.totalAdditions);
    m_totals.totalDrawingsFmt = formatINR(m_totals.totalDrawings);
    m_totals.totalProfitShareFmt = formatINR(m_totals.totalProfitShare);
    m_totals.totalInterestFmt = formatINR(m_totals.totalInterest);
    m_totals.totalClosingCapitalFmt = formatINR(m_totals.totalClosingCapital);

    emit dataChanged();
}

} // namespace MahadevERP
