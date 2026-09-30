#include "trial_balance_controller.h"
#include "../database_manager.h"
#include "../engine/fiscal_year_helper.h"
#include "../engine/ledger_pipeline.h"
#include "../services/financial_math_service.h"
#include <cmath>
#include <algorithm>
#include <QMap>
#include <QSet>
#include <QDebug>

namespace MahadevERP {

static QString formatINR(double val) {
    if (std::abs(val) < 0.001) return "-";
    return FinancialMathService::instance().formatInr(val, false);
}

TrialBalanceController::TrialBalanceController(QObject* parent)
    : QObject(parent)
{
    FiscalYearInfo activeFy = FiscalYearHelper::getActiveFiscalYear();
    m_fromDate = QDate::fromString(activeFy.startDate, "yyyy-MM-dd");
    m_toDate = QDate::fromString(activeFy.endDate, "yyyy-MM-dd");
}

void TrialBalanceController::setMode(TrialBalanceMode mode) {
    if (m_mode != mode) {
        m_mode = mode;
        calculate();
    }
}

void TrialBalanceController::setSearchQuery(const QString& query) {
    m_searchQuery = query;
    applyFilter();
}

void TrialBalanceController::setDateRange(const QDate& fromDate, const QDate& toDate) {
    m_fromDate = fromDate;
    m_toDate = toDate;
    calculate();
}

void TrialBalanceController::reload() {
    calculate();
}

void TrialBalanceController::calculate() {
    m_allRows.clear();
    m_totals = TrialBalanceTotals();

    FiscalYearInfo activeFy = FiscalYearHelper::getActiveFiscalYear();
    QString fromIso = m_fromDate.isValid() ? m_fromDate.toString("yyyy-MM-dd") : activeFy.startDate;
    QString toIso = m_toDate.isValid() ? m_toDate.toString("yyyy-MM-dd") : activeFy.endDate;

    QVector<::LedgerPeriodBalance> balances = ::LedgerPipeline::instance().calculateBalancesForPeriod(fromIso, toIso);

    struct RawLedgerInfo {
        int id;
        QString name;
        QString group;
        double opDr = 0.0;
        double opCr = 0.0;
        double periodDr = 0.0;
        double periodCr = 0.0;
        double closeDr = 0.0;
        double closeCr = 0.0;
    };

    QVector<RawLedgerInfo> rawLedgers;
    QMap<QString, QVector<RawLedgerInfo>> groupMap;

    for (const auto& b : balances) {
        RawLedgerInfo l;
        l.id = b.accountId;
        l.name = b.accountName;
        l.group = b.groupName.isEmpty() ? "General" : b.groupName;
        l.opDr = b.openingDr;
        l.opCr = b.openingCr;
        l.periodDr = b.periodDr;
        l.periodCr = b.periodCr;

        if (m_mode == TrialBalanceMode::WithoutOpBal) {
            double netPeriod = l.periodDr - l.periodCr;
            if (netPeriod >= 0.0) {
                l.closeDr = netPeriod;
                l.closeCr = 0.0;
            } else {
                l.closeDr = 0.0;
                l.closeCr = -netPeriod;
            }
        } else {
            l.closeDr = b.closingDr;
            l.closeCr = b.closingCr;
        }

        // Only include if has activity or opening balance
        if (l.opDr > 0.001 || l.opCr > 0.001 || l.periodDr > 0.001 || l.periodCr > 0.001 || l.closeDr > 0.001 || l.closeCr > 0.001) {
            rawLedgers.append(l);
            groupMap[l.group].append(l);
        }
    }

    // Process rows based on mode:
    if (m_mode == TrialBalanceMode::NormalView) {
        // Group totals only
        for (auto it = groupMap.begin(); it != groupMap.end(); ++it) {
            QString gName = it.key();
            TrialBalanceRow r;
            r.accountName = gName;
            r.groupName = gName;
            r.isGroupHeader = true;
            r.depth = 0;

            for (const auto& l : it.value()) {
                r.opDebit += l.opDr;
                r.opCredit += l.opCr;
                r.periodDebit += l.periodDr;
                r.periodCredit += l.periodCr;
                r.closeDebit += l.closeDr;
                r.closeCredit += l.closeCr;
            }
            r.opBalanceFmt = formatINR(r.opDebit > 0 ? r.opDebit : r.opCredit) + (r.opDebit > 0 ? " Dr" : (r.opCredit > 0 ? " Cr" : ""));
            r.periodDebitFmt = formatINR(r.periodDebit);
            r.periodCreditFmt = formatINR(r.periodCredit);
            r.closeDebitFmt = formatINR(r.closeDebit);
            r.closeCreditFmt = formatINR(r.closeCredit);

            m_allRows.append(r);
            m_totals.totalOpDebit += r.opDebit;
            m_totals.totalOpCredit += r.opCredit;
            m_totals.totalPeriodDebit += r.periodDebit;
            m_totals.totalPeriodCredit += r.periodCredit;
            m_totals.totalCloseDebit += r.closeDebit;
            m_totals.totalCloseCredit += r.closeCredit;
        }
    } else if (m_mode == TrialBalanceMode::FlatView) {
        // Alphabetical flat list of ledgers
        std::sort(rawLedgers.begin(), rawLedgers.end(), [](const RawLedgerInfo& a, const RawLedgerInfo& b) {
            return a.name.compare(b.name, Qt::CaseInsensitive) < 0;
        });
        for (const auto& l : rawLedgers) {
            TrialBalanceRow r;
            r.ledgerId = l.id;
            r.accountName = l.name;
            r.groupName = l.group;
            r.isGroupHeader = false;
            r.depth = 0;
            r.opDebit = l.opDr;
            r.opCredit = l.opCr;
            r.periodDebit = l.periodDr;
            r.periodCredit = l.periodCr;
            r.closeDebit = l.closeDr;
            r.closeCredit = l.closeCr;

            r.opBalanceFmt = formatINR(r.opDebit > 0 ? r.opDebit : r.opCredit) + (r.opDebit > 0 ? " Dr" : (r.opCredit > 0 ? " Cr" : ""));
            r.periodDebitFmt = formatINR(r.periodDebit);
            r.periodCreditFmt = formatINR(r.periodCredit);
            r.closeDebitFmt = formatINR(r.closeDebit);
            r.closeCreditFmt = formatINR(r.closeCredit);

            m_allRows.append(r);
            m_totals.totalOpDebit += r.opDebit;
            m_totals.totalOpCredit += r.opCredit;
            m_totals.totalPeriodDebit += r.periodDebit;
            m_totals.totalPeriodCredit += r.periodCredit;
            m_totals.totalCloseDebit += r.closeDebit;
            m_totals.totalCloseCredit += r.closeCredit;
        }
    } else if (m_mode == TrialBalanceMode::FlatGrouped || m_mode == TrialBalanceMode::NormalDetailed) {
        // Group Header followed by accounts
        for (auto it = groupMap.begin(); it != groupMap.end(); ++it) {
            QString gName = it.key();
            TrialBalanceRow gRow;
            gRow.accountName = gName;
            gRow.groupName = gName;
            gRow.isGroupHeader = true;
            gRow.depth = 0;

            for (const auto& l : it.value()) {
                gRow.opDebit += l.opDr;
                gRow.opCredit += l.opCr;
                gRow.periodDebit += l.periodDr;
                gRow.periodCredit += l.periodCr;
                gRow.closeDebit += l.closeDr;
                gRow.closeCredit += l.closeCr;
            }
            gRow.opBalanceFmt = formatINR(gRow.opDebit > 0 ? gRow.opDebit : gRow.opCredit) + (gRow.opDebit > 0 ? " Dr" : (gRow.opCredit > 0 ? " Cr" : ""));
            gRow.periodDebitFmt = formatINR(gRow.periodDebit);
            gRow.periodCreditFmt = formatINR(gRow.periodCredit);
            gRow.closeDebitFmt = formatINR(gRow.closeDebit);
            gRow.closeCreditFmt = formatINR(gRow.closeCredit);

            m_allRows.append(gRow);
            m_totals.totalOpDebit += gRow.opDebit;
            m_totals.totalOpCredit += gRow.opCredit;
            m_totals.totalPeriodDebit += gRow.periodDebit;
            m_totals.totalPeriodCredit += gRow.periodCredit;
            m_totals.totalCloseDebit += gRow.closeDebit;
            m_totals.totalCloseCredit += gRow.closeCredit;

            for (const auto& l : it.value()) {
                TrialBalanceRow r;
                r.ledgerId = l.id;
                r.accountName = l.name;
                r.groupName = l.group;
                r.isGroupHeader = false;
                r.depth = 1;
                r.opDebit = l.opDr;
                r.opCredit = l.opCr;
                r.periodDebit = l.periodDr;
                r.periodCredit = l.periodCr;
                r.closeDebit = l.closeDr;
                r.closeCredit = l.closeCr;

                r.opBalanceFmt = formatINR(r.opDebit > 0 ? r.opDebit : r.opCredit) + (r.opDebit > 0 ? " Dr" : (r.opCredit > 0 ? " Cr" : ""));
                r.periodDebitFmt = formatINR(r.periodDebit);
                r.periodCreditFmt = formatINR(r.periodCredit);
                r.closeDebitFmt = formatINR(r.closeDebit);
                r.closeCreditFmt = formatINR(r.closeCredit);

                m_allRows.append(r);
            }
        }
    } else {
        // WithoutOpBal, ShowTurnover, ShowOpeningBal -> List of accounts with custom columns
        for (const auto& l : rawLedgers) {
            TrialBalanceRow r;
            r.ledgerId = l.id;
            r.accountName = l.name;
            r.groupName = l.group;
            r.isGroupHeader = false;
            r.depth = 0;
            r.opDebit = l.opDr;
            r.opCredit = l.opCr;
            r.periodDebit = l.periodDr;
            r.periodCredit = l.periodCr;
            r.closeDebit = l.closeDr;
            r.closeCredit = l.closeCr;

            r.opBalanceFmt = formatINR(r.opDebit > 0 ? r.opDebit : r.opCredit) + (r.opDebit > 0 ? " Dr" : (r.opCredit > 0 ? " Cr" : ""));
            r.periodDebitFmt = formatINR(r.periodDebit);
            r.periodCreditFmt = formatINR(r.periodCredit);
            r.closeDebitFmt = formatINR(r.closeDebit);
            r.closeCreditFmt = formatINR(r.closeCredit);

            m_allRows.append(r);
            m_totals.totalOpDebit += r.opDebit;
            m_totals.totalOpCredit += r.opCredit;
            m_totals.totalPeriodDebit += r.periodDebit;
            m_totals.totalPeriodCredit += r.periodCredit;
            m_totals.totalCloseDebit += r.closeDebit;
            m_totals.totalCloseCredit += r.closeCredit;
        }
    }

    m_totals.difference = std::abs(m_totals.totalCloseDebit - m_totals.totalCloseCredit);
    m_totals.isBalanced = (m_totals.difference < 0.01);
    m_totals.totalCloseDebitFmt = formatINR(m_totals.totalCloseDebit);
    m_totals.totalCloseCreditFmt = formatINR(m_totals.totalCloseCredit);
    m_totals.differenceFmt = formatINR(m_totals.difference);

    applyFilter();
}

void TrialBalanceController::applyFilter() {
    m_filteredRows.clear();
    QString query = m_searchQuery.trimmed().toLower();

    for (const auto& r : m_allRows) {
        if (query.isEmpty() || r.accountName.toLower().contains(query) || r.groupName.toLower().contains(query)) {
            m_filteredRows.append(r);
        }
    }
    emit dataChanged();
}

} // namespace MahadevERP
