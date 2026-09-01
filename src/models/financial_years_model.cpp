#include "financial_years_model.h"
#include "../database_manager.h"

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
    if (ok) reload_data();
    return ok;
}

QVariantList FinancialYearsModel::get_all_years() const {
    return m_data;
}
