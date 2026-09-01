#pragma once

#include <QString>
#include <QStringList>
#include <QVariantMap>
#include <cmath>

class AccountingEngine {
public:
    static QVariantMap calculateSalesTax(double taxableAmount, double gstPct, bool isIgst = false) {
        QVariantMap result;
        double gstAmount = std::round(taxableAmount * (gstPct / 100.0) * 100.0) / 100.0;
        double cgst = 0.0;
        double sgst = 0.0;
        double igst = 0.0;

        if (isIgst) {
            igst = gstAmount;
        } else {
            cgst = std::round((gstAmount / 2.0) * 100.0) / 100.0;
            sgst = std::round((gstAmount / 2.0) * 100.0) / 100.0;
        }

        double totalAmount = std::round((taxableAmount + gstAmount) * 100.0) / 100.0;

        result["taxable_amount"] = taxableAmount;
        result["gst_pct"] = gstPct;
        result["cgst_amount"] = cgst;
        result["sgst_amount"] = sgst;
        result["igst_amount"] = igst;
        result["gst_amount"] = gstAmount;
        result["total_amount"] = totalAmount;
        return result;
    }

    static QString formatIndianNumber(double val, int decimals = 2, const QString& unit = "") {
        if (std::isnan(val)) return "0.00";
        bool isNegative = val < 0;
        double absVal = std::abs(val);
        
        long long intPart = static_cast<long long>(absVal);
        double fracPart = absVal - intPart;
        
        QString intStr = QString::number(intPart);
        QString res = "";
        
        if (intStr.length() <= 3) {
            res = intStr;
        } else {
            QString last3 = intStr.right(3);
            QString remaining = intStr.left(intStr.length() - 3);
            QStringList groups;
            while (remaining.length() > 2) {
                groups.prepend(remaining.right(2));
                remaining = remaining.left(remaining.length() - 2);
            }
            if (!remaining.isEmpty()) {
                groups.prepend(remaining);
            }
            res = groups.join(",") + "," + last3;
        }

        if (isNegative) res = "-" + res;

        if (decimals > 0) {
            long long multiplier = 1;
            for (int i = 0; i < decimals; ++i) multiplier *= 10;
            long long fracInt = static_cast<long long>(std::round(fracPart * multiplier));
            QString fracStr = QString::number(fracInt).rightJustified(decimals, '0');
            res += "." + fracStr;
        }

        if (!unit.isEmpty()) res += " " + unit;
        return res;
    }

    static QString formatIndianCurrency(double val) {
        return "₹" + formatIndianNumber(val, 2);
    }
};
