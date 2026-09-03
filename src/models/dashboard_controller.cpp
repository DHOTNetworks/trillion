#include "dashboard_controller.h"
#include "../database_manager.h"
#include "../engine/accounting_engine.h"

DashboardController::DashboardController(QObject* parent) : QObject(parent) {
    refresh_stats();
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

    // 1. Point-in-time Stock as of tDate (Paddy Basmati - Item 43)
    double paddyVal = 0.0;
    QString targetDate = !tDate.isEmpty() ? tDate : "9999-12-31";
    QVariant pAudited = DatabaseManager::instance().executeScalar(
        "SELECT weight_qtl FROM custom_closing_stocks "
        "WHERE item_code = '43' AND closing_date = ? LIMIT 1;",
        {targetDate}
    );
    if (pAudited.isValid() && pAudited.toDouble() > 0.0) {
        paddyVal = pAudited.toDouble();
    } else {
        // Query latest audited closing stock on or before targetDate
        QVariantList pPriorList = DatabaseManager::instance().executeQuery(
            "SELECT closing_date, weight_qtl FROM custom_closing_stocks "
            "WHERE item_code = '43' AND closing_date <= ? ORDER BY closing_date DESC LIMIT 1;",
            {targetDate}
        );
        QString cDate = "1900-01-01";
        double opPaddy = 0.0;
        if (!pPriorList.isEmpty()) {
            cDate = pPriorList.first().toMap().value("closing_date").toString();
            opPaddy = pPriorList.first().toMap().value("weight_qtl").toDouble();
        }

        // Live transactions between cDate and targetDate
        QVariant pIn = DatabaseManager::instance().executeScalar(
            "SELECT SUM(weight_qtl) FROM stock_transactions "
            "WHERE item_code = '43' AND trans_type IN ('Purc', 'Inward', 'P') "
            "AND voucher_date > ? AND voucher_date <= ?;",
            {cDate, targetDate}
        );
        double inPaddy = pIn.isValid() ? pIn.toDouble() : 0.0;

        QVariant pOut = DatabaseManager::instance().executeScalar(
            "SELECT SUM(weight_qtl) FROM stock_transactions "
            "WHERE item_code = '43' AND trans_type IN ('Sale', 'Outward', 'S') "
            "AND voucher_date > ? AND voucher_date <= ?;",
            {cDate, targetDate}
        );
        double outPaddy = pOut.isValid() ? pOut.toDouble() : 0.0;

        paddyVal = opPaddy + inPaddy - outPaddy;
    }
    m_paddyStock = AccountingEngine::formatIndianNumber(paddyVal, 1, "Qtl");

    // 2. Point-in-time Stock as of tDate (Rice Basmati Non Branded - Item 30)
    double riceVal = 0.0;
    QVariant rAudited = DatabaseManager::instance().executeScalar(
        "SELECT weight_qtl FROM custom_closing_stocks "
        "WHERE item_code = '30' AND closing_date = ? LIMIT 1;",
        {targetDate}
    );
    if (rAudited.isValid() && rAudited.toDouble() > 0.0) {
        riceVal = rAudited.toDouble();
    } else {
        // Query latest audited closing stock on or before targetDate
        QVariantList rPriorList = DatabaseManager::instance().executeQuery(
            "SELECT closing_date, weight_qtl FROM custom_closing_stocks "
            "WHERE item_code = '30' AND closing_date <= ? ORDER BY closing_date DESC LIMIT 1;",
            {targetDate}
        );
        QString cDate = "1900-01-01";
        double opRice = 0.0;
        if (!rPriorList.isEmpty()) {
            cDate = rPriorList.first().toMap().value("closing_date").toString();
            opRice = rPriorList.first().toMap().value("weight_qtl").toDouble();
        }

        // Live purchases between cDate and targetDate
        QVariant rIn = DatabaseManager::instance().executeScalar(
            "SELECT SUM(weight_qtl) FROM stock_transactions "
            "WHERE item_code = '30' AND trans_type IN ('Purc', 'Inward', 'P') "
            "AND voucher_date > ? AND voucher_date <= ?;",
            {cDate, targetDate}
        );
        double inRice = rIn.isValid() ? rIn.toDouble() : 0.0;

        // Live milling production between cDate and targetDate
        QVariant rMill = DatabaseManager::instance().executeScalar(
            "SELECT SUM(weight_qtl) FROM milling_voucher_items "
            "WHERE item_code = '30' AND drcr = 'Dr' "
            "AND batch_date > ? AND batch_date <= ?;",
            {cDate, targetDate}
        );
        double inMilling = rMill.isValid() ? rMill.toDouble() : 0.0;

        // Live sales between cDate and targetDate
        QVariant rOut = DatabaseManager::instance().executeScalar(
            "SELECT SUM(weight_qtl) FROM stock_transactions "
            "WHERE item_code = '30' AND trans_type IN ('Sale', 'Outward', 'S') "
            "AND voucher_date > ? AND voucher_date <= ?;",
            {cDate, targetDate}
        );
        double outRice = rOut.isValid() ? rOut.toDouble() : 0.0;

        riceVal = opRice + inRice + inMilling - outRice;
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
