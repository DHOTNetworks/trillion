#pragma once

#include <QString>
#include <QDate>
#include <QList>
#include <QVariantMap>
#include <cmath>

namespace MahadevERP {

struct AankTransactionEntry {
    QDate date;
    QString voucherNo;
    QString voucherType;
    QString narration;
    double debitAmount = 0.0;
    double creditAmount = 0.0;
    double runningBalance = 0.0; // Positive = Dr, Negative = Cr
    int days = 0;
    double productAank = 0.0;    // RunningBalance * Days
    double interestAmount = 0.0;
};

struct AankInterestStatement {
    int accountCode = 0;
    QString accountName;
    QDate fromDate;
    QDate toDate;
    double drInterestRate = 12.0; // Annual % for Debit balance (Advances given)
    double crInterestRate = 12.0; // Annual % for Credit balance (Deposits taken)
    double totalDrAank = 0.0;
    double totalCrAank = 0.0;
    double totalDrInterest = 0.0;
    double totalCrInterest = 0.0;
    double netInterest = 0.0;     // Positive = Dr Interest (receivable), Negative = Cr Interest (payable)
    QList<AankTransactionEntry> entries;
};

class AankInterestEngine {
public:
    static AankInterestStatement calculateStatement(
        int accountCode,
        const QString& accountName,
        double opBal,
        const QString& opDrCr,
        const QDate& fromDate,
        const QDate& toDate,
        double drInterestRate,
        double crInterestRate,
        const QList<QVariantMap>& rawVouchers,
        bool isLeapYearDivisor = false
    );

    static double computeInterest(double productSum, double annualRate, bool isLeapYear = false);
};

} // namespace MahadevERP
