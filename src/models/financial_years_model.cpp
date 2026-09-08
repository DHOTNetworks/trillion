#include "financial_years_model.h"
#include "../database_manager.h"
#include <QDate>
#include <QRegularExpression>

FinancialYearsModel::FinancialYearsModel(QObject* parent)
    : BaseTableModel(
        {"Year Name", "Start Date", "End Date", "Active", "Locked"},
        {"year_name", "start_date", "end_date", "is_active", "is_locked"},
        parent
    )
{
    reload_data();
}

void FinancialYearsModel::reload_data() {
    beginResetModel();
    m_data = DatabaseManager::instance().executeQuery("SELECT * FROM financial_years ORDER BY start_date ASC;");
    endResetModel();
    emit dataChangedSignal();
    emit countChanged();
}

QVariantMap FinancialYearsModel::get_active_year() const {
    QVariantList rows = DatabaseManager::instance().executeQuery("SELECT * FROM financial_years WHERE is_active = 1 LIMIT 1;");
    if (!rows.isEmpty()) {
        return rows.first().toMap();
    }
    return QVariantMap();
}

bool FinancialYearsModel::set_active_year(const QString& yearName) {
    DatabaseManager::instance().executeNonQuery("UPDATE financial_years SET is_active = 0;");
    bool ok = DatabaseManager::instance().executeNonQuery(
        "UPDATE financial_years SET is_active = 1 WHERE year_name = ?;",
        {yearName}
    );
    if (ok) {
        m_workingDate = "";
        reload_data();
    }
    return ok;
}

QVariantList FinancialYearsModel::get_all_years() const {
    return m_data;
}

QString FinancialYearsModel::get_working_date() const {
    QVariantMap activeFy = get_active_year();
    QDate startD = QDate::fromString(activeFy.value("start_date").toString(), "yyyy-MM-dd");
    QDate endD = QDate::fromString(activeFy.value("end_date").toString(), "yyyy-MM-dd");

    if (!m_workingDate.isEmpty()) {
        QDate wD = QDate::fromString(m_workingDate, "dd-MM-yyyy");
        if (!wD.isValid()) wD = QDate::fromString(m_workingDate, "yyyy-MM-dd");
        if (wD.isValid() && startD.isValid() && endD.isValid()) {
            if (wD >= startD && wD <= endD) {
                return wD.toString("dd-MM-yyyy");
            }
        }
    }

    QDate today = QDate::currentDate();
    if (startD.isValid() && endD.isValid()) {
        if (today >= startD && today <= endD) {
            m_workingDate = today.toString("dd-MM-yyyy");
            return m_workingDate;
        }
        if (today > endD) {
            m_workingDate = endD.toString("dd-MM-yyyy");
            return m_workingDate;
        }
        if (today < startD) {
            m_workingDate = startD.toString("dd-MM-yyyy");
            return m_workingDate;
        }
    }

    m_workingDate = today.toString("dd-MM-yyyy");
    return m_workingDate;
}

void FinancialYearsModel::set_working_date(const QString& dateStr) {
    QString parsed = parse_date_pattern(dateStr);
    if (!parsed.isEmpty()) {
        m_workingDate = parsed;
    } else {
        m_workingDate = dateStr;
    }
}

QString FinancialYearsModel::parse_date_pattern(const QString& input, const QString& referenceDate) const {
    QString raw = input.trimmed();
    if (raw.isEmpty()) return "";

    QDate refDate;
    if (!referenceDate.trimmed().isEmpty()) {
        refDate = QDate::fromString(referenceDate.trimmed(), "dd-MM-yyyy");
        if (!refDate.isValid()) refDate = QDate::fromString(referenceDate.trimmed(), "yyyy-MM-dd");
    }
    if (!refDate.isValid()) {
        QString wDate = get_working_date();
        refDate = QDate::fromString(wDate, "dd-MM-yyyy");
        if (!refDate.isValid()) refDate = QDate::currentDate();
    }

    QVariantMap activeFy = get_active_year();
    QDate fyStart = QDate::fromString(activeFy.value("start_date").toString(), "yyyy-MM-dd");
    QDate fyEnd = QDate::fromString(activeFy.value("end_date").toString(), "yyyy-MM-dd");

    // Standardize separators: convert /, -, \, ;, spaces to .
    QString s = raw;
    s.replace(QRegularExpression("[/\\-_;\\s]+"), ".");

    int day = 0, month = 0, year = 0;

    bool isAllDigits = true;
    for (QChar c : s) {
        if (!c.isDigit()) { isAllDigits = false; break; }
    }

    if (isAllDigits) {
        if (s.length() == 1 || s.length() == 2) {
            day = s.toInt();
            month = refDate.month();
            year = refDate.year();
        } else if (s.length() == 4) {
            day = s.left(2).toInt();
            month = s.mid(2, 2).toInt();
            year = refDate.year();
        } else if (s.length() == 6) {
            day = s.left(2).toInt();
            month = s.mid(2, 2).toInt();
            year = s.mid(4, 2).toInt();
            year += (year <= 50 ? 2000 : 1900);
        } else if (s.length() == 8) {
            if (s.startsWith("20") || s.startsWith("19")) {
                year = s.left(4).toInt();
                month = s.mid(4, 2).toInt();
                day = s.mid(6, 2).toInt();
            } else {
                day = s.left(2).toInt();
                month = s.mid(2, 2).toInt();
                year = s.mid(4, 4).toInt();
            }
        } else {
            return "";
        }
    } else {
        QStringList parts = s.split('.', Qt::SkipEmptyParts);
        if (parts.size() == 3) {
            if (parts[0].length() == 4 && parts[0].toInt() > 1900) {
                year = parts[0].toInt();
                month = parts[1].toInt();
                day = parts[2].toInt();
            } else {
                day = parts[0].toInt();
                month = parts[1].toInt();
                year = parts[2].toInt();
                if (year < 100) {
                    year += (year <= 50 ? 2000 : 1900);
                }
            }
        } else if (parts.size() == 2) {
            day = parts[0].toInt();
            month = parts[1].toInt();
            if (fyStart.isValid() && fyEnd.isValid()) {
                if (month >= fyStart.month() && month <= 12) {
                    year = fyStart.year();
                } else {
                    year = fyEnd.year();
                }
            } else {
                year = refDate.year();
            }
        } else if (parts.size() == 1) {
            day = parts[0].toInt();
            month = refDate.month();
            year = refDate.year();
        } else {
            return "";
        }
    }

    QDate result(year, month, day);
    if (!result.isValid()) return "";
    return result.toString("dd-MM-yyyy");
}

QVariantMap FinancialYearsModel::validate_voucher_date(const QString& input, const QString& referenceDate) const {
    QVariantMap res;
    res["valid"] = false;
    res["formattedDate"] = "";
    res["isoDate"] = "";
    res["error"] = "";

    QVariantMap activeFy = get_active_year();
    QString fyName = activeFy.value("year_name").toString();
    QDate fyStart = QDate::fromString(activeFy.value("start_date").toString(), "yyyy-MM-dd");
    QDate fyEnd = QDate::fromString(activeFy.value("end_date").toString(), "yyyy-MM-dd");
    res["fyName"] = fyName;
    res["startDate"] = activeFy.value("start_date").toString();
    res["endDate"] = activeFy.value("end_date").toString();
    res["startFormatted"] = fyStart.isValid() ? fyStart.toString("dd-MM-yyyy") : "";
    res["endFormatted"] = fyEnd.isValid() ? fyEnd.toString("dd-MM-yyyy") : "";

    QString parsed = parse_date_pattern(input, referenceDate);
    if (parsed.isEmpty()) {
        res["error"] = "Invalid date format. Please enter e.g. 01.03.26 or 01-03-2026.";
        return res;
    }

    QDate d = QDate::fromString(parsed, "dd-MM-yyyy");
    if (!d.isValid()) {
        res["error"] = "Invalid calendar date.";
        return res;
    }

    res["formattedDate"] = d.toString("dd-MM-yyyy");
    res["isoDate"] = d.toString("yyyy-MM-dd");

    if (fyStart.isValid() && fyEnd.isValid()) {
        if (d < fyStart || d > fyEnd) {
            res["valid"] = false;
            res["error"] = QString("Date %1 is outside the active Financial Year (%2: %3 to %4).\nPlease enter a date within this period or switch the Financial Year in FY Selector (Alt+F).")
                                .arg(d.toString("dd-MM-yyyy"), fyName, fyStart.toString("dd-MM-yyyy"), fyEnd.toString("dd-MM-yyyy"));
            return res;
        }
    }

    res["valid"] = true;
    return res;
}
