#pragma once

#include <QObject>
#include <QString>
#include <QVector>
#include <QDate>
#include <QVariantMap>

namespace MahadevERP {

enum class TrialBalanceMode {
    NormalView,       // Group Hierarchy Tree
    FlatView,         // Alphabetical flat list of all ledgers
    FlatGrouped,      // Flat list categorized under groups
    NormalDetailed,   // Expanded recursive hierarchy down to leaf ledgers
    WithoutOpBal,     // Period-only net transaction movements
    ShowTurnover,     // 4-Column: Op Bal + Dr Turnover + Cr Turnover + Cl Bal
    ShowOpeningBal    // Dual Balance: Opening Bal + Closing Bal
};

struct TrialBalanceRow {
    int ledgerId = 0;
    QString accountName;      // Ledger or Group Name
    QString groupName;        // Parent Group Name
    bool isGroupHeader = false;
    int depth = 0;            // Indentation depth for tree views

    double opDebit = 0.0;
    double opCredit = 0.0;
    double periodDebit = 0.0;
    double periodCredit = 0.0;
    double closeDebit = 0.0;
    double closeCredit = 0.0;

    // Formatted strings
    QString opBalanceFmt;
    QString periodDebitFmt;
    QString periodCreditFmt;
    QString closeDebitFmt;
    QString closeCreditFmt;
};

struct TrialBalanceTotals {
    double totalOpDebit = 0.0;
    double totalOpCredit = 0.0;
    double totalPeriodDebit = 0.0;
    double totalPeriodCredit = 0.0;
    double totalCloseDebit = 0.0;
    double totalCloseCredit = 0.0;
    double difference = 0.0;
    bool isBalanced = true;

    QString totalCloseDebitFmt;
    QString totalCloseCreditFmt;
    QString differenceFmt;
};

class TrialBalanceController : public QObject {
    Q_OBJECT

public:
    explicit TrialBalanceController(QObject* parent = nullptr);

    TrialBalanceMode mode() const { return m_mode; }
    void setMode(TrialBalanceMode mode);

    QString searchQuery() const { return m_searchQuery; }
    void setSearchQuery(const QString& query);

    QDate fromDate() const { return m_fromDate; }
    QDate toDate() const { return m_toDate; }
    void setDateRange(const QDate& fromDate, const QDate& toDate);

    void reload();

    const QVector<TrialBalanceRow>& rows() const { return m_filteredRows; }
    const TrialBalanceTotals& totals() const { return m_totals; }

signals:
    void dataChanged();

private:
    void calculate();
    void applyFilter();

    TrialBalanceMode m_mode = TrialBalanceMode::NormalView;
    QString m_searchQuery;
    QDate m_fromDate;
    QDate m_toDate;

    QVector<TrialBalanceRow> m_allRows;
    QVector<TrialBalanceRow> m_filteredRows;
    TrialBalanceTotals m_totals;
};

} // namespace MahadevERP
