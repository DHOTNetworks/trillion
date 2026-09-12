#pragma once

#include <QObject>
#include <QString>
#include <QVariantMap>
#include <cmath>

class FinancialMathService : public QObject {
    Q_OBJECT

public:
    explicit FinancialMathService(QObject *parent = nullptr);
    static FinancialMathService& instance();

    // Deterministic 2-decimal rounding with double precision
    Q_INVOKABLE static double round2(double val);

    // Formats double into Indian Rupee style e.g. "1,25,000.50" or "₹ 1,25,000.50"
    Q_INVOKABLE QString formatInr(double amount, bool includeSymbol = true) const;

    // Calculates line item amount based on rate type ("per_qtl" / "per_bag" / "per_kg" / "per_unit")
    Q_INVOKABLE double calculateLineAmount(double bags, double weightQtl, double rate, const QString &rateType = "per_qtl") const;

    // GST Breakdown Calculator
    // Returns QVariantMap with "cgst", "sgst", "igst", "totalTax"
    Q_INVOKABLE QVariantMap calculateGst(double taxableAmount, double gstRatePercent, bool isInterstate) const;

    // Calculates round-off: roundOff = std::round(grossTotal) - grossTotal
    Q_INVOKABLE double calculateRoundOff(double grossTotal) const;

    // Calculates final payable total after rounding
    Q_INVOKABLE double calculateGrandTotal(double grossTotal) const;

    // Calculates Brokerage/Dami on taxable amount or bags
    Q_INVOKABLE double calculateBrokerage(double taxableAmount, double bags, double rate, const QString &brokerageType = "percent") const;
};
