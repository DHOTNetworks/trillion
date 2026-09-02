#include "dashboard_controller.h"
#include "../database_manager.h"
#include "../engine/accounting_engine.h"

DashboardController::DashboardController(QObject* parent) : QObject(parent) {
    refresh_stats();
}

void DashboardController::refresh_stats(const QString& fromDate, const QString& toDate, const QString& fyLabel) {
    QString fDate = fromDate;
    QString tDate = toDate;
    QString fy = fyLabel;

    if (fy.isEmpty() && fDate.isEmpty()) {
        fDate = AccountingEngine::getActiveFromDate();
        tDate = AccountingEngine::getActiveToDate();
        fy = AccountingEngine::getActiveFyLabel();
    }

    if (fy.isEmpty() && fDate.isEmpty()) {
        QVariantList rows = DatabaseManager::instance().executeQuery("SELECT year_name, start_date, end_date FROM financial_years WHERE is_active = 1 LIMIT 1;");
        if (!rows.isEmpty()) {
            QVariantMap r = rows.first().toMap();
            fy = r.value("year_name").toString();
            fDate = r.value("start_date").toString();
            tDate = r.value("end_date").toString();
        } else {
            fy = "FY 2026-27";
            fDate = "2026-04-01";
            tDate = "2027-03-31";
        }
    }

    // 1. Paddy Stock (Filtered by active FY Closing Stock)
    double paddyVal = 0.0;
    if (!tDate.isEmpty()) {
        QVariant pRow = DatabaseManager::instance().executeScalar(
            "SELECT SUM(weight_qtl) FROM custom_closing_stocks WHERE (item_name LIKE '%Paddy%' OR item_code = '43') AND closing_date = ?;",
            {tDate}
        );
        paddyVal = pRow.isValid() ? pRow.toDouble() : 0.0;
    } else {
        QVariant pRow = DatabaseManager::instance().executeScalar("SELECT SUM(current_stock_qtl) FROM inventory WHERE category = 'Raw Paddy';");
        paddyVal = pRow.isValid() ? pRow.toDouble() : 0.0;
    }
    m_paddyStock = AccountingEngine::formatIndianNumber(paddyVal, 1, "Qtl");

    // 2. Rice Stock (Filtered by active FY Closing Stock)
    double riceVal = 0.0;
    if (!tDate.isEmpty()) {
        QVariant rRow = DatabaseManager::instance().executeScalar(
            "SELECT SUM(weight_qtl) FROM custom_closing_stocks WHERE item_code = '30' AND closing_date = ?;",
            {tDate}
        );
        riceVal = rRow.isValid() ? rRow.toDouble() : 0.0;
    } else {
        QVariant rRow = DatabaseManager::instance().executeScalar("SELECT SUM(current_stock_qtl) FROM inventory WHERE category = 'Finished Rice';");
        riceVal = rRow.isValid() ? rRow.toDouble() : 0.0;
    }
    m_riceStock = AccountingEngine::formatIndianNumber(riceVal, 1, "Qtl");

    // 3. Sales Turnover (Taxable Turnover matching Bahi-Khata for active period)
    double salesVal = 0.0;
    if (!fDate.isEmpty() && !tDate.isEmpty()) {
        QVariant sRow = DatabaseManager::instance().executeScalar(
            "SELECT SUM(COALESCE(taxable_amount, total_amount)) FROM sales_invoices WHERE invoice_date >= ? AND invoice_date <= ?;",
            {fDate, tDate}
        );
        salesVal = sRow.isValid() ? sRow.toDouble() : 0.0;
    } else if (!fy.isEmpty() && fy != "All") {
        QVariant sRow = DatabaseManager::instance().executeScalar(
            "SELECT SUM(COALESCE(taxable_amount, total_amount)) FROM sales_invoices WHERE financial_year = ?;",
            {fy}
        );
        salesVal = sRow.isValid() ? sRow.toDouble() : 0.0;
    } else {
        QVariant sRow = DatabaseManager::instance().executeScalar("SELECT SUM(COALESCE(taxable_amount, total_amount)) FROM sales_invoices;");
        salesVal = sRow.isValid() ? sRow.toDouble() : 0.0;
    }
    m_totalSales = AccountingEngine::formatIndianCurrency(salesVal);

    // 4. Procurement
    double procVal = 0.0;
    if (!fDate.isEmpty() && !tDate.isEmpty()) {
        QVariant pRow = DatabaseManager::instance().executeScalar(
            "SELECT SUM(COALESCE(total_amount, taxable_amount)) FROM purchase_invoices WHERE invoice_date >= ? AND invoice_date <= ?;",
            {fDate, tDate}
        );
        procVal = pRow.isValid() ? pRow.toDouble() : 0.0;
    } else if (!fy.isEmpty() && fy != "All") {
        QVariant pRow = DatabaseManager::instance().executeScalar(
            "SELECT SUM(COALESCE(total_amount, taxable_amount)) FROM purchase_invoices WHERE financial_year = ?;",
            {fy}
        );
        procVal = pRow.isValid() ? pRow.toDouble() : 0.0;
    } else {
        QVariant pRow = DatabaseManager::instance().executeScalar("SELECT SUM(COALESCE(total_amount, taxable_amount)) FROM purchase_invoices;");
        procVal = pRow.isValid() ? pRow.toDouble() : 0.0;
    }
    m_totalProcurement = AccountingEngine::formatIndianCurrency(procVal);

    // 5. Avg Milling Efficiency
    double effVal = 65.3;
    if (!fDate.isEmpty() && !tDate.isEmpty()) {
        QVariant effRow = DatabaseManager::instance().executeScalar(
            "SELECT AVG(yield_pct) FROM milling_batches WHERE batch_date >= ? AND batch_date <= ?;",
            {fDate, tDate}
        );
        if (effRow.isValid()) effVal = effRow.toDouble();
    } else {
        QVariant effRow = DatabaseManager::instance().executeScalar("SELECT AVG(yield_pct) FROM milling_batches;");
        if (effRow.isValid()) effVal = effRow.toDouble();
    }
    m_millingEfficiency = QString::number(effVal, 'f', 1) + "%";

    emit statsChanged();
}

QString DashboardController::format_inr(double amount) {
    return AccountingEngine::formatIndianCurrency(amount);
}

QString DashboardController::format_inr(const QString& amount) {
    return AccountingEngine::formatIndianCurrency(amount.toDouble());
}

QString DashboardController::format_qty(double qty) {
    return AccountingEngine::formatIndianNumber(qty, 2);
}

QString DashboardController::format_qty(const QString& qty) {
    return AccountingEngine::formatIndianNumber(qty.toDouble(), 2);
}
