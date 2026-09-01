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
        QVariantList rows = DatabaseManager::instance().executeQuery("SELECT year_name, start_date, end_date FROM financial_years WHERE is_active = 1 LIMIT 1;");
        if (!rows.isEmpty()) {
            QVariantMap r = rows.first().toMap();
            fy = r.value("year_name").toString();
            fDate = r.value("start_date").toString();
            tDate = r.value("end_date").toString();
        }
    }

    // 1. Paddy Stock
    double paddyVal = 0.0;
    if (!tDate.isEmpty()) {
        QVariant pRow = DatabaseManager::instance().executeScalar(
            "SELECT SUM(weight_qtl) FROM custom_closing_stocks WHERE (item_name LIKE '%Paddy%' OR item_code = '43') AND closing_date = ?;",
            {tDate}
        );
        paddyVal = pRow.isValid() ? pRow.toDouble() : 0.0;
    } else {
        QVariant pRow = DatabaseManager::instance().executeScalar("SELECT SUM(current_weight_qtl) FROM stock_items WHERE category = 'Raw Paddy';");
        paddyVal = pRow.isValid() ? pRow.toDouble() : 0.0;
    }
    m_paddyStock = AccountingEngine::formatIndianNumber(paddyVal, 1, "Qtl");

    // 2. Rice Stock
    double riceVal = 0.0;
    if (!tDate.isEmpty()) {
        QVariant rRow = DatabaseManager::instance().executeScalar(
            "SELECT SUM(weight_qtl) FROM custom_closing_stocks WHERE item_code = '30' AND closing_date = ?;",
            {tDate}
        );
        riceVal = rRow.isValid() ? rRow.toDouble() : 0.0;
    } else {
        QVariant rRow = DatabaseManager::instance().executeScalar("SELECT SUM(current_weight_qtl) FROM stock_items WHERE category = 'Finished Rice';");
        riceVal = rRow.isValid() ? rRow.toDouble() : 0.0;
    }
    m_riceStock = AccountingEngine::formatIndianNumber(riceVal, 1, "Qtl");

    // 3. Sales Turnover
    double salesVal = 0.0;
    if (!fDate.isEmpty() && !tDate.isEmpty()) {
        QVariant sRow = DatabaseManager::instance().executeScalar(
            "SELECT SUM(COALESCE(taxable_amount, total_amount)) FROM sales_invoices WHERE invoice_date >= ? AND invoice_date <= ?;",
            {fDate, tDate}
        );
        salesVal = sRow.isValid() ? sRow.toDouble() : 0.0;
    } else if (!fy.isEmpty()) {
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
    } else if (!fy.isEmpty()) {
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

    // 5. Milling Efficiency
    QVariant effRow = DatabaseManager::instance().executeScalar("SELECT AVG(outturn_pct) FROM milling_batches;");
    double effVal = effRow.isValid() ? effRow.toDouble() : 65.3;
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
