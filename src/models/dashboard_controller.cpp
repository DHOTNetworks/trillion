#include "dashboard_controller.h"
#include "../database_manager.h"
#include "../engine/accounting_engine.h"
#include "../engine/fiscal_year_helper.h"

DashboardController::DashboardController(QObject* parent) : QObject(parent) {
}

QString DashboardController::dbPath() const {
    return DatabaseManager::instance().dbPath();
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
        FiscalYearInfo activeFy = FiscalYearHelper::getActiveFiscalYear();
        if (activeFy.isValid()) {
            fy = activeFy.name;
            fDate = activeFy.startDate;
            tDate = activeFy.endDate;
        }
    }

    QString targetDate = !tDate.isEmpty() ? tDate : "9999-12-31";

    // 1. Single-pass stock aggregation from closing stocks + live stock transactions
    QVariant maxCloseDate = DatabaseManager::instance().executeScalar(
        "SELECT MAX(closing_date) FROM custom_closing_stocks WHERE closing_date <= ?;",
        {targetDate}
    );
    QString cDate = maxCloseDate.isValid() && !maxCloseDate.isNull() ? maxCloseDate.toString() : "1900-01-01";

    // Closing stocks on cDate
    double opPaddy = 0.0;
    double opRice = 0.0;
    QVariantList closeRows = DatabaseManager::instance().executeQuery(
        "SELECT item_code, item_name, weight_qtl FROM custom_closing_stocks WHERE closing_date = ?;",
        {cDate}
    );
    for (const auto& r : closeRows) {
        QVariantMap m = r.toMap();
        QString name = m.value("item_name").toString().toLower();
        QString code = m.value("item_code").toString();
        double w = m.value("weight_qtl").toDouble();
        if (code == "43" || name.contains("paddy")) opPaddy += w;
        if (code == "30" || name.contains("rice")) opRice += w;
    }

    double inPaddy = 0.0, outPaddy = 0.0;
    double inRice = 0.0, outRice = 0.0;

    // Single-pass transaction aggregation using index idx_stock_trans_item_code / idx_stock_trans_date_item
    QVariantList transRows = DatabaseManager::instance().executeQuery(
        "SELECT item_code, item_name, trans_type, SUM(weight_qtl) as tot_weight "
        "FROM stock_transactions "
        "WHERE voucher_date > ? AND voucher_date <= ? "
        "GROUP BY item_code, item_name, trans_type;",
        {cDate, targetDate}
    );
    for (const auto& tr : transRows) {
        QVariantMap m = tr.toMap();
        QString name = m.value("item_name").toString().toLower();
        QString code = m.value("item_code").toString();
        QString type = m.value("trans_type").toString().toUpper();
        double w = m.value("tot_weight").toDouble();

        bool isPaddy = (code == "43" || name.contains("paddy"));
        bool isRice = (code == "30" || name.contains("rice"));

        if (type.startsWith("P") || type == "INWARD") {
            if (isPaddy) inPaddy += w;
            if (isRice) inRice += w;
        } else if (type.startsWith("S") || type == "OUTWARD") {
            if (isPaddy) outPaddy += w;
            if (isRice) outRice += w;
        }
    }

    // Single-pass milling aggregation
    QVariantList millRows = DatabaseManager::instance().executeQuery(
        "SELECT item_code, item_name, drcr, SUM(weight_qtl) as tot_weight "
        "FROM milling_voucher_items "
        "WHERE batch_date > ? AND batch_date <= ? "
        "GROUP BY item_code, item_name, drcr;",
        {cDate, targetDate}
    );
    double inMilling = 0.0;
    for (const auto& mr : millRows) {
        QVariantMap m = mr.toMap();
        QString name = m.value("item_name").toString().toLower();
        QString code = m.value("item_code").toString();
        QString drcr = m.value("drcr").toString();
        double w = m.value("tot_weight").toDouble();
        if (drcr == "Dr" && (code == "30" || name.contains("rice"))) {
            inMilling += w;
        }
    }

    double paddyVal = opPaddy + inPaddy - outPaddy;
    double riceVal = opRice + inRice + inMilling - outRice;

    m_paddyStock = AccountingEngine::formatIndianNumber(paddyVal, 1, "Qtl");
    m_riceStock = AccountingEngine::formatIndianNumber(riceVal, 1, "Qtl");

    // 3. Sales Turnover (Taxable Turnover matching Bahi-Khata for active period)
    double salesVal = 0.0;
    if (!fDate.isEmpty() && !tDate.isEmpty()) {
        QVariant sRow = DatabaseManager::instance().executeScalar(
            "SELECT SUM(COALESCE(taxable_amount, total_amount)) FROM sales_invoices WHERE invoice_date >= ? AND invoice_date <= ?;",
            {fDate, tDate}
        );
        salesVal = sRow.isValid() ? sRow.toDouble() : 0.0;
    } else if (!fy.isEmpty() && fy != "All" && fy != "Custom Period") {
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
    } else if (!fy.isEmpty() && fy != "All" && fy != "Custom Period") {
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
