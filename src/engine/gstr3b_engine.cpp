#include "gstr3b_engine.h"
#include "gst_tax_engine.h"
#include "../database_manager.h"
#include <cmath>

namespace MahadevERP {

static double round2(double val) {
    return std::round(val * 100.0) / 100.0;
}

Gstr3BReturnSummary Gstr3BEngine::generateFromDatabase(
    const QString& gstin,
    const QString& legalName,
    const QString& stateCode,
    const QDate& fromDate,
    const QDate& toDate
) {
    Gstr3BReturnSummary summary;
    summary.gstin = gstin;
    summary.legalName = legalName;
    summary.fromDate = fromDate;
    summary.toDate = toDate;
    summary.returnPeriod = QString("%1%2").arg(fromDate.month(), 2, 10, QChar('0')).arg(fromDate.year());

    QString fromStr = fromDate.toString("yyyy-MM-dd");
    QString toStr = toDate.toString("yyyy-MM-dd");

    // 1. Process Outward Supplies (Sales) for Table 3.1
    QString salesSql = QString(
        "SELECT "
        "  s.id, s.total_amount, "
        "  COALESCE(s.gst_amount, s.cgst_amount + s.sgst_amount + s.igst_amount, 0.0) AS tax_amount, "
        "  COALESCE(s.gstin, '') AS party_gstin, "
        "  COALESCE(s.place_of_supply, '') AS place_of_supply, "
        "  COALESCE(s.taxable_amount, s.total_amount) AS amount, "
        "  COALESCE(s.gst_pct, 5.0) AS tax_rate "
        "FROM sales_invoices s "
        "WHERE s.invoice_date >= '%1' AND s.invoice_date <= '%2';"
    ).arg(fromStr, toStr);

    QVariantList salesRows = DatabaseManager::instance().executeQuery(salesSql);
    for (const auto& var : salesRows) {
        QVariantMap r = var.toMap();
        double amt = r.value("amount").toDouble();
        double rate = r.value("tax_rate").toDouble();
        if (rate <= 0.0) rate = 5.0;
        
        QString pGstin = r.value("party_gstin").toString().trimmed();
        QString pos = GstTaxEngine::extractStateCodeFromGstin(pGstin);
        if (pos.isEmpty()) pos = r.value("place_of_supply").toString().trimmed();
        if (pos.isEmpty()) pos = stateCode;

        bool isIntra = (pos == stateCode);

        summary.table31.txValA += amt;
        if (isIntra) {
            summary.table31.cAmtA += round2(amt * (rate / 200.0));
            summary.table31.sAmtA += round2(amt * (rate / 200.0));
        } else {
            summary.table31.iAmtA += round2(amt * (rate / 100.0));
        }
    }

    // 2. Process Inward Supplies (Purchases) for Table 4 ITC
    QString purcSql = QString(
        "SELECT "
        "  p.id, p.total_amount, "
        "  COALESCE(p.gst_amount, p.cgst_amount + p.sgst_amount + p.igst_amount, 0.0) AS tax_amount, "
        "  COALESCE(p.gstin, '') AS party_gstin, "
        "  COALESCE(p.place_of_supply, '') AS place_of_supply, "
        "  COALESCE(p.taxable_amount, p.total_amount) AS amount, "
        "  COALESCE(p.gst_pct, 5.0) AS tax_rate "
        "FROM purchase_invoices p "
        "WHERE p.invoice_date >= '%1' AND p.invoice_date <= '%2';"
    ).arg(fromStr, toStr);

    QVariantList purcRows = DatabaseManager::instance().executeQuery(purcSql);
    for (const auto& var : purcRows) {
        QVariantMap r = var.toMap();
        double amt = r.value("amount").toDouble();
        double rate = r.value("tax_rate").toDouble();
        if (rate <= 0.0) rate = 5.0;

        QString pGstin = r.value("party_gstin").toString().trimmed();
        QString pos = GstTaxEngine::extractStateCodeFromGstin(pGstin);
        if (pos.isEmpty()) pos = stateCode;

        bool isIntra = (pos == stateCode);

        if (isIntra) {
            summary.table4.cAmtA5 += round2(amt * (rate / 200.0));
            summary.table4.sAmtA5 += round2(amt * (rate / 200.0));
        } else {
            summary.table4.iAmtA5 += round2(amt * (rate / 100.0));
        }
    }

    // Rounding & Totals
    summary.table31.txValA = round2(summary.table31.txValA);
    summary.table31.iAmtA = round2(summary.table31.iAmtA);
    summary.table31.cAmtA = round2(summary.table31.cAmtA);
    summary.table31.sAmtA = round2(summary.table31.sAmtA);

    summary.table4.iAmtA5 = round2(summary.table4.iAmtA5);
    summary.table4.cAmtA5 = round2(summary.table4.cAmtA5);
    summary.table4.sAmtA5 = round2(summary.table4.sAmtA5);

    summary.table4.netIgst = summary.table4.iAmtA5;
    summary.table4.netCgst = summary.table4.cAmtA5;
    summary.table4.netSgst = summary.table4.sAmtA5;

    summary.netPayableIgst = std::max(0.0, round2(summary.table31.iAmtA - summary.table4.netIgst));
    summary.netPayableCgst = std::max(0.0, round2(summary.table31.cAmtA - summary.table4.netCgst));
    summary.netPayableSgst = std::max(0.0, round2(summary.table31.sAmtA - summary.table4.netSgst));
    summary.netPayableTotal = round2(summary.netPayableIgst + summary.netPayableCgst + summary.netPayableSgst);

    return summary;
}

} // namespace MahadevERP
