#include "gstr9_engine.h"
#include "../database_manager.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QStringList>
#include <cmath>

namespace MahadevERP {

static double safeDouble(const QVariant& val) {
    return val.isValid() ? val.toDouble() : 0.0;
}

Gstr9AnnualSummary Gstr9Engine::computeAnnualReturn(const QString& financialYear) {
    Gstr9AnnualSummary summary;
    summary.financialYear = financialYear;

    // 1. Fetch Company Info
    QVariantList compRows = DatabaseManager::instance().executeQuery("SELECT * FROM company_info LIMIT 1;");
    if (!compRows.isEmpty()) {
        QVariantMap comp = compRows.first().toMap();
        summary.gstin = comp.value("gstin").toString();
        summary.legalName = comp.value("company_name").toString();
        summary.tradeName = comp.value("company_name").toString();
    }

    // 2. Compute Table 4: Outward Taxable Supplies
    // B2B Sales
    QVariantList b2bSales = DatabaseManager::instance().executeQuery(
        "SELECT "
        "  COALESCE(SUM(taxable_amount), 0.0) as tax_val, "
        "  COALESCE(SUM(igst_amount), 0.0) as igst_val, "
        "  COALESCE(SUM(cgst_amount), 0.0) as cgst_val, "
        "  COALESCE(SUM(sgst_amount), 0.0) as sgst_val "
        "FROM sales_invoices "
        "WHERE (financial_year = ? OR fy_id IN (SELECT id FROM financial_years WHERE year_name = ?)) "
        "  AND LENGTH(TRIM(COALESCE(gstin, ''))) = 15;",
        {financialYear, financialYear}
    );
    if (!b2bSales.isEmpty()) {
        QVariantMap r = b2bSales.first().toMap();
        summary.table4.b2bTaxable = safeDouble(r["tax_val"]);
        summary.table4.b2bIgst = safeDouble(r["igst_val"]);
        summary.table4.b2bCgst = safeDouble(r["cgst_val"]);
        summary.table4.b2bSgst = safeDouble(r["sgst_val"]);
    }

    // B2C Sales (Unregistered)
    QVariantList b2cSales = DatabaseManager::instance().executeQuery(
        "SELECT "
        "  COALESCE(SUM(taxable_amount), 0.0) as tax_val, "
        "  COALESCE(SUM(igst_amount), 0.0) as igst_val, "
        "  COALESCE(SUM(cgst_amount), 0.0) as cgst_val, "
        "  COALESCE(SUM(sgst_amount), 0.0) as sgst_val "
        "FROM sales_invoices "
        "WHERE (financial_year = ? OR fy_id IN (SELECT id FROM financial_years WHERE year_name = ?)) "
        "  AND (gstin IS NULL OR LENGTH(TRIM(gstin)) != 15);",
        {financialYear, financialYear}
    );
    if (!b2cSales.isEmpty()) {
        QVariantMap r = b2cSales.first().toMap();
        summary.table4.b2cTaxable = safeDouble(r["tax_val"]);
        summary.table4.b2cIgst = safeDouble(r["igst_val"]);
        summary.table4.b2cCgst = safeDouble(r["cgst_val"]);
        summary.table4.b2cSgst = safeDouble(r["sgst_val"]);
    }

    // Credit & Debit Notes
    QVariantList dcnList = DatabaseManager::instance().executeQuery(
        "SELECT "
        "  note_type, "
        "  COALESCE(SUM(taxable_amount), 0.0) as tax_val, "
        "  COALESCE(SUM(igst_amount), 0.0) as igst_val, "
        "  COALESCE(SUM(cgst_amount), 0.0) as cgst_val, "
        "  COALESCE(SUM(sgst_amount), 0.0) as sgst_val "
        "FROM debit_credit_notes "
        "WHERE (financial_year = ? OR fy_id IN (SELECT id FROM financial_years WHERE year_name = ?)) "
        "GROUP BY note_type;",
        {financialYear, financialYear}
    );
    for (const auto& row : dcnList) {
        QVariantMap r = row.toMap();
        QString type = r.value("note_type").toString().toUpper();
        if (type.contains("CREDIT")) {
            summary.table4.cdnrTaxable += safeDouble(r["tax_val"]);
            summary.table4.cdnrIgst += safeDouble(r["igst_val"]);
            summary.table4.cdnrCgst += safeDouble(r["cgst_val"]);
            summary.table4.cdnrSgst += safeDouble(r["sgst_val"]);
        } else {
            summary.table4.cdnurTaxable += safeDouble(r["tax_val"]);
            summary.table4.cdnurIgst += safeDouble(r["igst_val"]);
            summary.table4.cdnurCgst += safeDouble(r["cgst_val"]);
            summary.table4.cdnurSgst += safeDouble(r["sgst_val"]);
        }
    }

    summary.table4.totalTaxable = summary.table4.b2bTaxable + summary.table4.b2cTaxable - summary.table4.cdnrTaxable + summary.table4.cdnurTaxable;
    summary.table4.totalIgst = summary.table4.b2bIgst + summary.table4.b2cIgst - summary.table4.cdnrIgst + summary.table4.cdnurIgst;
    summary.table4.totalCgst = summary.table4.b2bCgst + summary.table4.b2cCgst - summary.table4.cdnrCgst + summary.table4.cdnurCgst;
    summary.table4.totalSgst = summary.table4.b2bSgst + summary.table4.b2cSgst - summary.table4.cdnrSgst + summary.table4.cdnurSgst;

    // 3. Compute Table 6: Inward ITC Availed
    QVariantList itcRows = DatabaseManager::instance().executeQuery(
        "SELECT "
        "  COALESCE(SUM(igst_amount), 0.0) as igst_val, "
        "  COALESCE(SUM(cgst_amount), 0.0) as cgst_val, "
        "  COALESCE(SUM(sgst_amount), 0.0) as sgst_val "
        "FROM purchase_invoices "
        "WHERE (financial_year = ? OR fy_id IN (SELECT id FROM financial_years WHERE year_name = ?));",
        {financialYear, financialYear}
    );
    if (!itcRows.isEmpty()) {
        QVariantMap r = itcRows.first().toMap();
        summary.table6.b2bInputsIgst = safeDouble(r["igst_val"]);
        summary.table6.b2bInputsCgst = safeDouble(r["cgst_val"]);
        summary.table6.b2bInputsSgst = safeDouble(r["sgst_val"]);
        summary.table6.totalItcAvailed = summary.table6.b2bInputsIgst + summary.table6.b2bInputsCgst + summary.table6.b2bInputsSgst;
        summary.table6.gstr3bTotalItc = summary.table6.totalItcAvailed;
    }

    // 4. Compute Table 7: Net ITC Available
    summary.table7.totalItcReversed = 0.0;
    summary.table7.netItcAvailable = summary.table6.totalItcAvailed - summary.table7.totalItcReversed;

    // 5. Compute Table 8: 2A Reconciliation
    summary.table8.table6bItc = summary.table6.totalItcAvailed;
    summary.table8.gstr2aItc = summary.table6.totalItcAvailed; // Defaults to matched
    summary.table8.difference = summary.table8.gstr2aItc - summary.table8.table6bItc;

    // 6. Compute Table 9: Tax Paid
    summary.table9.integratedTaxPayable = summary.table4.totalIgst;
    summary.table9.integratedTaxPaidItc = std::min(summary.table9.integratedTaxPayable, summary.table6.b2bInputsIgst);
    summary.table9.integratedTaxPaidCash = std::max(0.0, summary.table9.integratedTaxPayable - summary.table9.integratedTaxPaidItc);

    summary.table9.centralTaxPayable = summary.table4.totalCgst;
    summary.table9.centralTaxPaidItc = std::min(summary.table9.centralTaxPayable, summary.table6.b2bInputsCgst);
    summary.table9.centralTaxPaidCash = std::max(0.0, summary.table9.centralTaxPayable - summary.table9.centralTaxPaidItc);

    summary.table9.stateTaxPayable = summary.table4.totalSgst;
    summary.table9.stateTaxPaidItc = std::min(summary.table9.stateTaxPayable, summary.table6.b2bInputsSgst);
    summary.table9.stateTaxPaidCash = std::max(0.0, summary.table9.stateTaxPayable - summary.table9.stateTaxPaidItc);

    return summary;
}

QString Gstr9Engine::exportGstr9Json(const Gstr9AnnualSummary& s) {
    QJsonObject root;
    root["gstin"] = s.gstin;
    root["fp"] = s.financialYear;
    root["legal_name"] = s.legalName;

    // Table 4
    QJsonObject t4;
    t4["b2b_taxable"] = s.table4.b2bTaxable;
    t4["b2b_igst"] = s.table4.b2bIgst;
    t4["b2b_cgst"] = s.table4.b2bCgst;
    t4["b2b_sgst"] = s.table4.b2bSgst;
    t4["b2c_taxable"] = s.table4.b2cTaxable;
    t4["total_taxable"] = s.table4.totalTaxable;
    t4["total_igst"] = s.table4.totalIgst;
    t4["total_cgst"] = s.table4.totalCgst;
    t4["total_sgst"] = s.table4.totalSgst;
    root["table4_outward_taxable"] = t4;

    // Table 6
    QJsonObject t6;
    t6["total_itc_availed"] = s.table6.totalItcAvailed;
    t6["inputs_igst"] = s.table6.b2bInputsIgst;
    t6["inputs_cgst"] = s.table6.b2bInputsCgst;
    t6["inputs_sgst"] = s.table6.b2bInputsSgst;
    root["table6_itc_availed"] = t6;

    // Table 9
    QJsonObject t9;
    t9["igst_paid_cash"] = s.table9.integratedTaxPaidCash;
    t9["cgst_paid_cash"] = s.table9.centralTaxPaidCash;
    t9["sgst_paid_cash"] = s.table9.stateTaxPaidCash;
    root["table9_tax_paid"] = t9;

    QJsonDocument doc(root);
    return QString::fromUtf8(doc.toJson(QJsonDocument::Indented));
}

QString Gstr9Engine::exportGstr9Csv(const Gstr9AnnualSummary& s) {
    QStringList csv;
    csv << "Table,Field Description,Taxable Value,IGST Amount,CGST Amount,SGST Amount,Cess Amount";
    csv << QString("4A,Supplies to Registered Persons (B2B),%1,%2,%3,%4,0.00")
           .arg(s.table4.b2bTaxable, 0, 'f', 2).arg(s.table4.b2bIgst, 0, 'f', 2)
           .arg(s.table4.b2bCgst, 0, 'f', 2).arg(s.table4.b2bSgst, 0, 'f', 2);
    csv << QString("4B,Supplies to Unregistered Persons (B2C),%1,%2,%3,%4,0.00")
           .arg(s.table4.b2cTaxable, 0, 'f', 2).arg(s.table4.b2cIgst, 0, 'f', 2)
           .arg(s.table4.b2cCgst, 0, 'f', 2).arg(s.table4.b2cSgst, 0, 'f', 2);
    csv << QString("4N,Total Outward Taxable Supplies (4A to 4M),%1,%2,%3,%4,0.00")
           .arg(s.table4.totalTaxable, 0, 'f', 2).arg(s.table4.totalIgst, 0, 'f', 2)
           .arg(s.table4.totalCgst, 0, 'f', 2).arg(s.table4.totalSgst, 0, 'f', 2);
    csv << QString("6A,Total ITC availed through FORM GSTR-3B,0.00,%1,%2,%3,0.00")
           .arg(s.table6.b2bInputsIgst, 0, 'f', 2).arg(s.table6.b2bInputsCgst, 0, 'f', 2)
           .arg(s.table6.b2bInputsSgst, 0, 'f', 2);
    csv << QString("9A,Integrated Tax Paid in Cash,0.00,%1,0.00,0.00,0.00").arg(s.table9.integratedTaxPaidCash, 0, 'f', 2);
    csv << QString("9B,Central Tax Paid in Cash,0.00,0.00,%1,0.00,0.00").arg(s.table9.centralTaxPaidCash, 0, 'f', 2);
    csv << QString("9C,State Tax Paid in Cash,0.00,0.00,0.00,%1,0.00").arg(s.table9.stateTaxPaidCash, 0, 'f', 2);
    return csv.join("\r\n") + "\r\n";
}

} // namespace MahadevERP
