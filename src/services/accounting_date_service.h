#pragma once

#include <QObject>
#include <QString>
#include <QDate>
#include <QRegularExpression>

class AccountingDateService : public QObject {
    Q_OBJECT

public:
    explicit AccountingDateService(QObject *parent = nullptr);
    static AccountingDateService& instance();

    // Resolves inputs like "1.3.26", "01.03.26", "1/3/26", "1-3-2026" -> "01/03/2026"
    Q_INVOKABLE QString resolveDate(const QString &rawInput, int baseYear = 0) const;

    // Converts dd/MM/yyyy or dd-MM-yyyy to yyyy-MM-dd ISO format
    Q_INVOKABLE QString toIso(const QString &dmyStr) const;

    // Converts yyyy-MM-dd ISO to dd/MM/yyyy
    Q_INVOKABLE QString fromIso(const QString &isoStr) const;

    // Returns day of week e.g. "Monday", "Tuesday"
    Q_INVOKABLE QString getDayOfWeek(const QString &dmyStr) const;

    // Validates whether the given date string falls strictly within the Financial Year
    // e.g. fyYearStart 2025 means FY 2025-2026 (01/04/2025 to 31/03/2026)
    Q_INVOKABLE bool validateDateInFy(const QString &dmyStr, int fyYearStart) const;

    // Returns error message if date is invalid or outside FY boundaries; empty string if valid
    Q_INVOKABLE QString getValidationErrorMessage(const QString &dmyStr, int fyYearStart, const QString &fyLabel = QString()) const;

    // Formats today's working date
    Q_INVOKABLE QString currentWorkingDate() const;
};
