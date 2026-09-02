#pragma once

#include <QString>
#include <QStringList>
#include <QVariantMap>
#include <cmath>

class AccountingEngine {
public:
    static inline QString s_activeFromDate = "";
    static inline QString s_activeToDate = "";
    static inline QString s_activeFyLabel = "";

    static void setActivePeriod(const QString& fromDate, const QString& toDate, const QString& fyLabel) {
        s_activeFromDate = fromDate;
        s_activeToDate = toDate;
        s_activeFyLabel = fyLabel;
    }

    static QString getActiveFromDate() { return s_activeFromDate; }
    static QString getActiveToDate() { return s_activeToDate; }
    static QString getActiveFyLabel() { return s_activeFyLabel; }

    static double calculateMoistureDeduction(double grossWeightQtl, double moisturePct, double baseMoistureLimit = 14.0) {
        if (moisturePct <= baseMoistureLimit) return 0.0;
        double excess = moisturePct - baseMoistureLimit;
        return std::round((grossWeightQtl * (excess / 100.0)) * 100.0) / 100.0;
    }

    static double calculatePaddyNetAmount(double netWeightQtl, double ratePerQtl, double hamaliCharges = 0.0) {
        double baseAmt = netWeightQtl * ratePerQtl;
        return std::round((baseAmt + hamaliCharges) * 100.0) / 100.0;
    }

    static QVariantMap calculateMillingYield(double paddyInputQtl, double headRiceQtl, double brokenRiceQtl, double branQtl, double huskQtl) {
        double totalOutput = headRiceQtl + brokenRiceQtl + branQtl + huskQtl;
        double wastageQtl = std::max(0.0, std::round((paddyInputQtl - totalOutput) * 100.0) / 100.0);
        double yieldPct = paddyInputQtl > 0 ? (std::round(((headRiceQtl / paddyInputQtl) * 100.0) * 100.0) / 100.0) : 0.0;
        
        QVariantMap res;
        res["total_output_qtl"] = totalOutput;
        res["wastage_qtl"] = wastageQtl;
        res["yield_pct"] = yieldPct;
        return res;
    }

    static QVariantMap calculateSalesTax(double taxableAmount, double gstPct = 5.0) {
        double gstAmount = std::round((taxableAmount * (gstPct / 100.0)) * 100.0) / 100.0;
        double totalAmount = std::round((taxableAmount + gstAmount) * 100.0) / 100.0;
        
        QVariantMap res;
        res["gst_amount"] = gstAmount;
        res["total_amount"] = totalAmount;
        return res;
    }

    static QString formatIndianNumber(double amount, int decimals = 2, const QString& unit = "", bool includeSymbol = false) {
        bool isNegative = amount < 0;
        double val = std::abs(amount);

        QString s = QString::number(val, 'f', decimals);
        QStringList parts = s.split('.');
        QString intPart = parts[0];
        QString decPart = parts.size() > 1 ? parts[1] : "";

        QString formattedInt;
        if (intPart.length() <= 3) {
            formattedInt = intPart;
        } else {
            QString last3 = intPart.right(3);
            QString rest = intPart.left(intPart.length() - 3);
            QStringList groups;
            while (rest.length() > 2) {
                groups.prepend(rest.right(2));
                rest.chop(2);
            }
            if (!rest.isEmpty()) {
                groups.prepend(rest);
            }
            formattedInt = groups.join(',') + ',' + last3;
        }

        QString result = (decimals > 0) ? (formattedInt + "." + decPart) : formattedInt;
        if (isNegative) result = "-" + result;
        if (includeSymbol) result = "₹" + result;
        if (!unit.isEmpty()) result = result + " " + unit;
        return result;
    }

    static QString formatIndianCurrency(double amount, bool includeSymbol = true, int decimals = 2) {
        return formatIndianNumber(amount, decimals, "", includeSymbol);
    }
};
