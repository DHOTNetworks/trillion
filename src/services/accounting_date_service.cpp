#include "accounting_date_service.h"
#include <QStringList>

AccountingDateService::AccountingDateService(QObject *parent)
    : QObject(parent)
{
}

AccountingDateService& AccountingDateService::instance()
{
    static AccountingDateService s_instance;
    return s_instance;
}

QString AccountingDateService::resolveDate(const QString &rawInput, int baseYear) const
{
    QString trimmed = rawInput.trimmed();
    if (trimmed.isEmpty()) return QString();

    // Standardize separators to '/'
    trimmed.replace('.', '/').replace('-', '/');
    QStringList parts = trimmed.split('/', Qt::SkipEmptyParts);

    if (parts.size() == 3) {
        int day = parts[0].toInt();
        int month = parts[1].toInt();
        int year = parts[2].toInt();

        // 2-digit year conversion
        if (parts[2].length() <= 2) {
            year += 2000;
        }

        QDate d(year, month, day);
        if (d.isValid()) {
            return d.toString("dd/MM/yyyy");
        }
    } else if (parts.size() == 2) {
        // e.g. "15/04" -> assume current working year
        int day = parts[0].toInt();
        int month = parts[1].toInt();
        int year = (baseYear > 0) ? baseYear : QDate::currentDate().year();
        QDate d(year, month, day);
        if (d.isValid()) {
            return d.toString("dd/MM/yyyy");
        }
    }

    return rawInput.trimmed();
}

QString AccountingDateService::toIso(const QString &dmyStr) const
{
    QString s = dmyStr.trimmed();
    if (s.isEmpty()) return QString();

    s.replace('.', '/').replace('-', '/');
    QStringList parts = s.split('/', Qt::SkipEmptyParts);
    if (parts.size() == 3) {
        int day = parts[0].toInt();
        int month = parts[1].toInt();
        int year = parts[2].toInt();
        if (parts[2].length() <= 2) year += 2000;
        QDate d(year, month, day);
        if (d.isValid()) {
            return d.toString("yyyy-MM-dd");
        }
    }
    // If already in ISO format
    QDate d = QDate::fromString(dmyStr.trimmed(), "yyyy-MM-dd");
    if (d.isValid()) {
        return d.toString("yyyy-MM-dd");
    }
    return dmyStr.trimmed();
}

QString AccountingDateService::fromIso(const QString &isoStr) const
{
    QString s = isoStr.trimmed();
    if (s.isEmpty()) return QString();

    QDate d = QDate::fromString(s, "yyyy-MM-dd");
    if (d.isValid()) {
        return d.toString("dd/MM/yyyy");
    }
    return s;
}

QString AccountingDateService::getDayOfWeek(const QString &dmyStr) const
{
    QString iso = toIso(dmyStr);
    QDate d = QDate::fromString(iso, "yyyy-MM-dd");
    if (d.isValid()) {
        static const QStringList days = {
            "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday", "Sunday"
        };
        int dayNum = d.dayOfWeek(); // 1 = Monday, 7 = Sunday
        if (dayNum >= 1 && dayNum <= 7) {
            return days[dayNum - 1];
        }
    }
    return QString();
}

bool AccountingDateService::validateDateInFy(const QString &dmyStr, int fyYearStart) const
{
    if (fyYearStart <= 0) return true; // No restriction if FY not set

    QString iso = toIso(dmyStr);
    QDate d = QDate::fromString(iso, "yyyy-MM-dd");
    if (!d.isValid()) return false;

    QDate fyStart(fyYearStart, 4, 1);
    QDate fyEnd(fyYearStart + 1, 3, 31);

    return (d >= fyStart && d <= fyEnd);
}

QString AccountingDateService::getValidationErrorMessage(const QString &dmyStr, int fyYearStart, const QString &fyLabel) const
{
    if (dmyStr.trimmed().isEmpty()) {
        return "Date cannot be empty.";
    }

    QString iso = toIso(dmyStr);
    QDate d = QDate::fromString(iso, "yyyy-MM-dd");
    if (!d.isValid()) {
        return "Invalid date format. Use dd/MM/yyyy.";
    }

    if (fyYearStart > 0) {
        QDate fyStart(fyYearStart, 4, 1);
        QDate fyEnd(fyYearStart + 1, 3, 31);
        if (d < fyStart) {
            QString label = fyLabel.isEmpty() ? QString("%1-%2").arg(fyYearStart).arg((fyYearStart + 1) % 100, 2, 10, QChar('0')) : fyLabel;
            return QString("Voucher date (%1) is BEFORE active Financial Year %2 (Starts 01/04/%3).")
                .arg(d.toString("dd/MM/yyyy")).arg(label).arg(fyYearStart);
        }
        if (d > fyEnd) {
            QString label = fyLabel.isEmpty() ? QString("%1-%2").arg(fyYearStart).arg((fyYearStart + 1) % 100, 2, 10, QChar('0')) : fyLabel;
            return QString("Voucher date (%1) is BEYOND active Financial Year %2 (Ends 31/03/%3). Please select next Financial Year first.")
                .arg(d.toString("dd/MM/yyyy")).arg(label).arg(fyYearStart + 1);
        }
    }

    return QString();
}

QString AccountingDateService::currentWorkingDate() const
{
    return QDate::currentDate().toString("dd/MM/yyyy");
}
