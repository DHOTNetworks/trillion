#pragma once
#include <QString>
#include <QStringList>
#include <QVariantMap>
#include <cmath>
#include "../database_manager.h"
#include <QRegularExpression>
#include <QDate>

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

    static QString resolveFinancialYear(const QString& input = "") {
        QString raw = input.trimmed();
        if (raw.isEmpty()) {
            QVariant fyVal = DatabaseManager::instance().executeScalar("SELECT year_name FROM financial_years WHERE is_active = 1 LIMIT 1;");
            if (fyVal.isValid() && !fyVal.toString().trimmed().isEmpty()) {
                s_activeFyLabel = fyVal.toString().trimmed();
                return s_activeFyLabel;
            }
            if (!s_activeFyLabel.isEmpty()) return s_activeFyLabel;
            QVariant lastFy = DatabaseManager::instance().executeScalar("SELECT year_name FROM financial_years ORDER BY start_date DESC LIMIT 1;");
            return lastFy.isValid() ? lastFy.toString().trimmed() : "FY 2026-27";
        }

        if (raw.startsWith("FY ")) return raw;
        if (QRegularExpression("^\\d{4}-\\d{2,4}$").match(raw).hasMatch()) {
            return "FY " + raw;
        }

        QString isoDate = raw;
        QStringList parts = raw.split('-');
        if (parts.size() == 1) parts = raw.split('/');
        if (parts.size() == 3) {
            if (parts[0].length() == 4) {
                isoDate = QString("%1-%2-%3").arg(parts[0], parts[1].rightJustified(2, '0'), parts[2].rightJustified(2, '0'));
            } else {
                isoDate = QString("%1-%2-%3").arg(parts[2], parts[1].rightJustified(2, '0'), parts[0].rightJustified(2, '0'));
            }
            QVariant fyVal = DatabaseManager::instance().executeScalar(
                "SELECT year_name FROM financial_years WHERE start_date <= ? AND end_date >= ? LIMIT 1;",
                {isoDate, isoDate}
            );
            if (fyVal.isValid() && !fyVal.toString().trimmed().isEmpty()) {
                return fyVal.toString().trimmed();
            }
        }

        QVariant directMatch = DatabaseManager::instance().executeScalar(
            "SELECT year_name FROM financial_years WHERE year_name = ? OR year_name LIKE ? LIMIT 1;",
            {raw, "%" + raw + "%"}
        );
        if (directMatch.isValid() && !directMatch.toString().trimmed().isEmpty()) {
            return directMatch.toString().trimmed();
        }

        return raw;
    }

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
