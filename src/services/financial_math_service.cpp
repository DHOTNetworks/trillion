#include "financial_math_service.h"
#include <QStringList>

FinancialMathService::FinancialMathService(QObject *parent)
    : QObject(parent)
{
}

FinancialMathService& FinancialMathService::instance()
{
    static FinancialMathService s_instance;
    return s_instance;
}

double FinancialMathService::round2(double val)
{
    return std::round(val * 100.0) / 100.0;
}

QString FinancialMathService::formatInr(double amount, bool includeSymbol) const
{
    bool isNegative = amount < -0.0001;
    double absAmount = std::abs(amount);
    double rounded = round2(absAmount);

    qint64 integerPart = static_cast<qint64>(rounded);
    int decimalPart = static_cast<int>(std::round((rounded - integerPart) * 100.0));
    if (decimalPart >= 100) {
        integerPart += 1;
        decimalPart = 0;
    }

    QString intStr = QString::number(integerPart);
    QString formattedInt;

    if (intStr.length() <= 3) {
        formattedInt = intStr;
    } else {
        QString lastThree = intStr.right(3);
        QString remaining = intStr.left(intStr.length() - 3);

        QStringList chunks;
        while (remaining.length() > 2) {
            chunks.prepend(remaining.right(2));
            remaining.chop(2);
        }
        if (!remaining.isEmpty()) {
            chunks.prepend(remaining);
        }
        formattedInt = chunks.join(",") + "," + lastThree;
    }

    QString decStr = QString("%1").arg(decimalPart, 2, 10, QChar('0'));
    QString result = formattedInt + "." + decStr;

    if (isNegative) {
        result = "-" + result;
    }
    if (includeSymbol) {
        result = "₹ " + result;
    }
    return result;
}

double FinancialMathService::calculateLineAmount(double bags, double weightQtl, double rate, const QString &rateType) const
{
    QString rType = rateType.toLower().trimmed();
    double amt = 0.0;
    if (rType == "per_bag" || rType == "bag" || rType == "per bag") {
        amt = bags * rate;
    } else if (rType == "per_kg" || rType == "kg") {
        amt = (weightQtl * 100.0) * rate;
    } else {
        // Default: per quintal (Qtl)
        amt = weightQtl * rate;
    }
    return round2(amt);
}

QVariantMap FinancialMathService::calculateGst(double taxableAmount, double gstRatePercent, bool isInterstate) const
{
    QVariantMap res;
    if (gstRatePercent <= 0.0001 || taxableAmount <= 0.0001) {
        res["cgst"] = 0.0;
        res["sgst"] = 0.0;
        res["igst"] = 0.0;
        res["totalTax"] = 0.0;
        return res;
    }

    double totalTax = round2(taxableAmount * (gstRatePercent / 100.0));
    if (isInterstate) {
        res["cgst"] = 0.0;
        res["sgst"] = 0.0;
        res["igst"] = totalTax;
        res["totalTax"] = totalTax;
    } else {
        double halfTax = round2(totalTax / 2.0);
        // Ensure halfTax + halfTax equals totalTax
        double otherHalf = round2(totalTax - halfTax);
        res["cgst"] = halfTax;
        res["sgst"] = otherHalf;
        res["igst"] = 0.0;
        res["totalTax"] = totalTax;
    }
    return res;
}

double FinancialMathService::calculateRoundOff(double grossTotal) const
{
    double rounded = std::round(grossTotal);
    return round2(rounded - grossTotal);
}

double FinancialMathService::calculateGrandTotal(double grossTotal) const
{
    return round2(std::round(grossTotal));
}

double FinancialMathService::calculateBrokerage(double taxableAmount, double bags, double rate, const QString &brokerageType) const
{
    if (rate <= 0.0001) return 0.0;
    QString bType = brokerageType.toLower().trimmed();
    if (bType == "per_bag" || bType == "bag") {
        return round2(bags * rate);
    }
    // Percent
    return round2(taxableAmount * (rate / 100.0));
}
