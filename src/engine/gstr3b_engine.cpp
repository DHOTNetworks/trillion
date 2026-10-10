#include "gstr3b_engine.h"
#include "gst_tax_engine.h"
#include "../database_manager.h"
#include <QRegularExpression>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QCoreApplication>
#include <miniz.h>
#include <cmath>

namespace MahadevERP {

static double round2(double val) {
    return std::round(val * 100.0) / 100.0;
}

static QString getMonthName(int month) {
    switch (month) {
        case 1: return "January";
        case 2: return "February";
        case 3: return "March";
        case 4: return "April";
        case 5: return "May";
        case 6: return "June";
        case 7: return "July";
        case 8: return "August";
        case 9: return "September";
        case 10: return "October";
        case 11: return "November";
        case 12: return "December";
        default: return "";
    }
}

Gstr3BReturnSummary Gstr3BEngine::generateFromDatabase(
    const QString& gstin,
    const QString& legalName,
    const QString& stateCode,
    const QDate& fromDate,
    const QDate& toDate,
    const Gstr1ReturnPayload* gstr1
) {
    Gstr3BReturnSummary summary;
    summary.gstin = gstin.trimmed().toUpper();
    summary.legalName = legalName.trimmed();
    summary.tradeName = legalName.trimmed();
    summary.fromDate = fromDate;
    summary.toDate = toDate;
    summary.returnPeriod = QString("%1%2").arg(fromDate.month(), 2, 10, QChar('0')).arg(fromDate.year());
    summary.monthName = getMonthName(fromDate.month());
    QString supplierState = stateCode.trimmed().rightJustified(2, '0');

    int startYear = fromDate.month() >= 4 ? fromDate.year() : fromDate.year() - 1;
    summary.fiscalYear = QString("%1-%2").arg(startYear).arg((startYear + 1) % 100, 2, 10, QChar('0'));

    QString fromStr = fromDate.toString("yyyy-MM-dd");
    QString toStr = toDate.toString("yyyy-MM-dd");

    // 1. Outward supplies, LINE by LINE (same partition as GSTR-1, so 3.1(a)
    // ties to GSTR-1 Tables 4/5/7 and 3.1(c) ties to GSTR-1 Table 8).
    // 0-rated lines (exempt rice) -> 3.1(c); rated lines -> 3.1(a) with tax
    // recomputed from rate x taxable by POS (never stored totals).
    QString salesSql = QString(
        "SELECT "
        "  COALESCE(si.taxable_amount, s.taxable_amount, s.total_amount) AS amount, "
        "  COALESCE(si.gst_pct, stk.gst_rate, s.gst_pct, 0.0) AS tax_rate, "
        "  COALESCE(s.gstin, '') AS party_gstin, "
        "  COALESCE(s.place_of_supply, '') AS place_of_supply, "
        "  COALESCE(s.tax_status, '') AS tax_status "
        "FROM sales_invoices s "
        "LEFT JOIN sales_invoice_items si ON s.id = si.invoice_id "
        "LEFT JOIN stock_items stk ON (si.item_id = stk.id OR (si.item_id IS NULL AND s.item_id = stk.id) OR stk.name = s.item_name OR stk.name = si.item_name) "
        "WHERE s.invoice_date >= '%1' AND s.invoice_date <= '%2' "
        "ORDER BY s.invoice_date ASC, s.id ASC;"
    ).arg(fromStr, toStr);

    QVariantList salesRows = DatabaseManager::instance().executeQuery(salesSql);
    for (const auto& var : salesRows) {
        QVariantMap r = var.toMap();
        double amt = r.value("amount").toDouble();
        double rate = r.value("tax_rate").toDouble();
        QString taxStatus = r.value("tax_status").toString().trimmed();
        QString gstinPos = GstTaxEngine::extractStateCodeFromGstin(r.value("party_gstin").toString());
        if (gstinPos.isEmpty()) gstinPos = r.value("place_of_supply").toString().trimmed();
        QString pos = gstinPos.isEmpty() ? supplierState : gstinPos.rightJustified(2, '0');
        bool isIntra = (pos == supplierState);

        if (taxStatus.contains("Non-GST", Qt::CaseInsensitive)) {
            summary.table31.txValE += amt;
            continue;
        }
        if (taxStatus.contains("Zero", Qt::CaseInsensitive) || taxStatus.contains("Export", Qt::CaseInsensitive)) {
            summary.table31.txValB += amt; // zero-rated (exports/SEZ): no tax
            continue;
        }
        if (std::fabs(rate) < 1e-9) {
            summary.table31.txValC += amt; // nil/exempt -> ties to GSTR-1 Table 8
            continue;
        }
        double igst = isIntra ? 0.0 : round2(amt * (rate / 100.0));
        double cgst = isIntra ? round2(amt * (rate / 200.0)) : 0.0;
        double sgst = isIntra ? round2(amt * (rate / 200.0)) : 0.0;
        summary.table31.txValA += amt;
        summary.table31.cAmtA += cgst;
        summary.table31.sAmtA += sgst;
        summary.table31.iAmtA += igst;
    }

    // 1b. Table 3.2 from the SAME GSTR-1 payload (portal keeps 3.2
    // non-editable since July 2025 — it must equal GSTR-1 Tables 5/7B).
    if (gstr1) {
        QMap<QString, QPair<double, double>> inter; // pos -> (txVal, iAmt)
        for (const auto& c : gstr1->b2cl)
            inter[c.pos] = qMakePair(inter.value(c.pos).first + c.taxableValue,
                                     inter.value(c.pos).second + c.igst);
        for (const auto& cs : gstr1->b2cs) {
            if (cs.splyTy != "INTER") continue;
            inter[cs.pos] = qMakePair(inter.value(cs.pos).first + cs.taxableValue,
                                      inter.value(cs.pos).second + cs.igst);
        }
        for (auto it = inter.constBegin(); it != inter.constEnd(); ++it) {
            Gstr3BTable32Row row;
            row.desc = "Supplies made to Unregistered Persons";
            row.pos = Gstr1Engine::posDisplayName(it.key());
            row.txVal = round2(it.value().first);
            row.iAmt = round2(it.value().second);
            summary.table32.rows.append(row);
        }
    }

    // 2. Process Inward Supplies (Purchases) for Table 4 ITC & Table 5 Exempt Inward
    QString purcSql = QString(
        "SELECT "
        "  p.id, p.supplier_id, p.total_amount, "
        "  COALESCE(p.taxable_amount, p.total_amount) AS amount, "
        "  COALESCE(p.gst_amount, p.cgst_amount + p.sgst_amount + p.igst_amount, 0.0) AS tax_amount, "
        "  COALESCE(p.cgst_amount, 0.0) AS cgst_amount, "
        "  COALESCE(p.sgst_amount, 0.0) AS sgst_amount, "
        "  COALESCE(p.igst_amount, 0.0) AS igst_amount, "
        "  COALESCE(p.gstin, '') AS party_gstin, "
        "  COALESCE(p.place_of_supply, '') AS place_of_supply, "
        "  COALESCE(p.tax_status, '') AS tax_status, "
        "  COALESCE(p.gst_pct, stk.gst_rate, 0.0) AS tax_rate, "
        "  COALESCE(pt.state_code, '') AS party_state_code "
        "FROM purchase_invoices p "
        "LEFT JOIN parties pt ON p.supplier_id = pt.id "
        "LEFT JOIN stock_items stk ON (p.item_id = stk.id OR stk.name = p.item_name) "
        "WHERE p.invoice_date >= '%1' AND p.invoice_date <= '%2';"
    ).arg(fromStr, toStr);

    QVariantList purcRows = DatabaseManager::instance().executeQuery(purcSql);
    for (const auto& var : purcRows) {
        QVariantMap r = var.toMap();
        double amt = r.value("amount").toDouble();
        double cgst = r.value("cgst_amount").toDouble();
        double sgst = r.value("sgst_amount").toDouble();
        double igst = r.value("igst_amount").toDouble();
        double taxAmt = r.value("tax_amount").toDouble();
        QString taxStatus = r.value("tax_status").toString().trimmed();

        if (taxAmt > 0.001 || (cgst + sgst + igst > 0.001)) {
            if (taxStatus.contains("RCM", Qt::CaseInsensitive)) {
                summary.table31.txValD += amt;
                summary.table31.cAmtD += cgst;
                summary.table31.sAmtD += sgst;
                summary.table31.iAmtD += igst;

                summary.table4.cAmtA3 += cgst;
                summary.table4.sAmtA3 += sgst;
                summary.table4.iAmtA3 += igst;
            } else {
                summary.table4.cAmtA5 += cgst;
                summary.table4.sAmtA5 += sgst;
                summary.table4.iAmtA5 += igst;
            }
        } else {
            // Exempt / Nil rated / Composition Inward Supplies -> Table 5
            QString sState = GstTaxEngine::extractStateCodeFromGstin(r.value("party_gstin").toString());
            if (sState.isEmpty()) sState = r.value("party_state_code").toString().trimmed();
            if (sState.isEmpty()) sState = r.value("place_of_supply").toString().trimmed();
            if (sState.isEmpty() || sState == "0") sState = stateCode;
            sState = sState.trimmed().rightJustified(2, '0');

            bool isIntra = (sState == supplierState);

            if (taxStatus.contains("Non-GST", Qt::CaseInsensitive)) {
                if (isIntra) summary.table5.intraNonGst += amt;
                else summary.table5.interNonGst += amt;
            } else {
                if (isIntra) summary.table5.intraExempt += amt;
                else summary.table5.interExempt += amt;
            }
        }
    }

    // 3. Round Table 3.1 values
    summary.table31.txValA = round2(summary.table31.txValA);
    summary.table31.iAmtA = round2(summary.table31.iAmtA);
    summary.table31.cAmtA = round2(summary.table31.cAmtA);
    summary.table31.sAmtA = round2(summary.table31.sAmtA);
    summary.table31.csAmtA = round2(summary.table31.csAmtA);

    summary.table31.txValB = round2(summary.table31.txValB);
    summary.table31.iAmtB = round2(summary.table31.iAmtB);
    summary.table31.csAmtB = round2(summary.table31.csAmtB);

    summary.table31.txValC = round2(summary.table31.txValC);

    summary.table31.txValD = round2(summary.table31.txValD);
    summary.table31.iAmtD = round2(summary.table31.iAmtD);
    summary.table31.cAmtD = round2(summary.table31.cAmtD);
    summary.table31.sAmtD = round2(summary.table31.sAmtD);
    summary.table31.csAmtD = round2(summary.table31.csAmtD);

    summary.table31.txValE = round2(summary.table31.txValE);

    // 4. Round Table 4 ITC values (every leg, not just A5).
    summary.table4.iAmtA1 = round2(summary.table4.iAmtA1);
    summary.table4.cAmtA1 = round2(summary.table4.cAmtA1);
    summary.table4.sAmtA1 = round2(summary.table4.sAmtA1);
    summary.table4.csAmtA1 = round2(summary.table4.csAmtA1);
    summary.table4.iAmtA2 = round2(summary.table4.iAmtA2);
    summary.table4.cAmtA2 = round2(summary.table4.cAmtA2);
    summary.table4.sAmtA2 = round2(summary.table4.sAmtA2);
    summary.table4.csAmtA2 = round2(summary.table4.csAmtA2);
    summary.table4.iAmtA3 = round2(summary.table4.iAmtA3);
    summary.table4.cAmtA3 = round2(summary.table4.cAmtA3);
    summary.table4.sAmtA3 = round2(summary.table4.sAmtA3);
    summary.table4.csAmtA3 = round2(summary.table4.csAmtA3);
    summary.table4.iAmtA4 = round2(summary.table4.iAmtA4);
    summary.table4.cAmtA4 = round2(summary.table4.cAmtA4);
    summary.table4.sAmtA4 = round2(summary.table4.sAmtA4);
    summary.table4.csAmtA4 = round2(summary.table4.csAmtA4);
    summary.table4.iAmtA5 = round2(summary.table4.iAmtA5);
    summary.table4.cAmtA5 = round2(summary.table4.cAmtA5);
    summary.table4.sAmtA5 = round2(summary.table4.sAmtA5);
    summary.table4.csAmtA5 = round2(summary.table4.csAmtA5);
    summary.table4.iAmtB1 = round2(summary.table4.iAmtB1);
    summary.table4.cAmtB1 = round2(summary.table4.cAmtB1);
    summary.table4.sAmtB1 = round2(summary.table4.sAmtB1);
    summary.table4.csAmtB1 = round2(summary.table4.csAmtB1);
    summary.table4.iAmtB2 = round2(summary.table4.iAmtB2);
    summary.table4.cAmtB2 = round2(summary.table4.cAmtB2);
    summary.table4.sAmtB2 = round2(summary.table4.sAmtB2);
    summary.table4.csAmtB2 = round2(summary.table4.csAmtB2);
    summary.table4.iAmtD1 = round2(summary.table4.iAmtD1);
    summary.table4.cAmtD1 = round2(summary.table4.cAmtD1);
    summary.table4.sAmtD1 = round2(summary.table4.sAmtD1);
    summary.table4.csAmtD1 = round2(summary.table4.csAmtD1);
    summary.table4.iAmtD2 = round2(summary.table4.iAmtD2);
    summary.table4.cAmtD2 = round2(summary.table4.cAmtD2);
    summary.table4.sAmtD2 = round2(summary.table4.sAmtD2);
    summary.table4.csAmtD2 = round2(summary.table4.csAmtD2);

    summary.table4.netIgst = round2(summary.table4.iAmtA1 + summary.table4.iAmtA2 + summary.table4.iAmtA3 + summary.table4.iAmtA4 + summary.table4.iAmtA5 - summary.table4.iAmtB1 - summary.table4.iAmtB2);
    summary.table4.netCgst = round2(summary.table4.cAmtA1 + summary.table4.cAmtA2 + summary.table4.cAmtA3 + summary.table4.cAmtA4 + summary.table4.cAmtA5 - summary.table4.cAmtB1 - summary.table4.cAmtB2);
    summary.table4.netSgst = round2(summary.table4.sAmtA1 + summary.table4.sAmtA2 + summary.table4.sAmtA3 + summary.table4.sAmtA4 + summary.table4.sAmtA5 - summary.table4.sAmtB1 - summary.table4.sAmtB2);
    summary.table4.netCess = round2(summary.table4.csAmtA1 + summary.table4.csAmtA2 + summary.table4.csAmtA3 + summary.table4.csAmtA4 + summary.table4.csAmtA5 - summary.table4.csAmtB1 - summary.table4.csAmtB2);

    // 5. Round Table 5 values
    summary.table5.interExempt = round2(summary.table5.interExempt);
    summary.table5.intraExempt = round2(summary.table5.intraExempt);
    summary.table5.interNonGst = round2(summary.table5.interNonGst);
    summary.table5.intraNonGst = round2(summary.table5.intraNonGst);

    // 6. Calculate Table 6.1 Tax Payment & Net Cash Payable
    // Each liability is offset only by same-type credit (always legal;
    // cross-utilization is left for the portal filing, never assumed here).
    summary.table61.taxPayableIgst = round2(summary.table31.iAmtA + summary.table31.iAmtB + summary.table31.iAmtD);
    summary.table61.taxPayableCgst = round2(summary.table31.cAmtA + summary.table31.cAmtD);
    summary.table61.taxPayableSgst = round2(summary.table31.sAmtA + summary.table31.sAmtD);
    summary.table61.taxPayableCess = round2(summary.table31.csAmtA + summary.table31.csAmtB + summary.table31.csAmtD);

    summary.table61.itcPaidIgst = std::min(summary.table61.taxPayableIgst, summary.table4.netIgst);
    summary.table61.itcPaidCgst = std::min(summary.table61.taxPayableCgst, summary.table4.netCgst);
    summary.table61.itcPaidSgst = std::min(summary.table61.taxPayableSgst, summary.table4.netSgst);
    summary.table61.itcPaidCess = std::min(summary.table61.taxPayableCess, summary.table4.netCess);

    summary.table61.cashPaidIgst = std::max(0.0, round2(summary.table61.taxPayableIgst - summary.table61.itcPaidIgst));
    summary.table61.cashPaidCgst = std::max(0.0, round2(summary.table61.taxPayableCgst - summary.table61.itcPaidCgst));
    summary.table61.cashPaidSgst = std::max(0.0, round2(summary.table61.taxPayableSgst - summary.table61.itcPaidSgst));
    summary.table61.cashPaidCess = std::max(0.0, round2(summary.table61.taxPayableCess - summary.table61.itcPaidCess));

    summary.netPayableIgst = summary.table61.cashPaidIgst;
    summary.netPayableCgst = summary.table61.cashPaidCgst;
    summary.netPayableSgst = summary.table61.cashPaidSgst;
    summary.netPayableTotal = round2(summary.netPayableIgst + summary.netPayableCgst + summary.netPayableSgst);

    return summary;
}

QString Gstr3BEngine::findTemplatePath(const QString& preferredPath) {
    if (!preferredPath.isEmpty() && QFile::exists(preferredPath)) {
        return preferredPath;
    }

    QStringList candidates = {
        QDir::currentPath() + "/gst-data/GSTR-3B.xls",
        QDir::currentPath() + "/data/GSTR-3B.xls",
        QDir::currentPath() + "/GSTR-3B.xls",
        QCoreApplication::applicationDirPath() + "/gst-data/GSTR-3B.xls",
        QCoreApplication::applicationDirPath() + "/../Resources/templates/GSTR-3B.xls",
        QDir::homePath() + "/MahadevAc/gst-data/GSTR-3B.xls"
    };

    for (const auto& path : candidates) {
        if (QFile::exists(path)) {
            return path;
        }
    }
    return "";
}

namespace {

// ---------- from-scratch v1.2 workbook (no template file needed) ----------

QString esc3b(const QString& s) {
    QString o = s;
    o.replace('&', "&amp;").replace('<', "&lt;").replace('>', "&gt;");
    return o;
}

QString colName3b(int idx0) {
    QString c;
    int n = idx0;
    do {
        c.prepend(QChar('A' + (n % 26)));
        n = n / 26 - 1;
    } while (n >= 0);
    return c;
}

QString cellT3b(const QString& ref, const QString& text, bool bold = false) {
    return QString("<c r=\"%1\" t=\"inlineStr\"%2><is><t xml:space=\"preserve\">%3</t></is></c>")
        .arg(ref, bold ? " s=\"1\"" : "", esc3b(text));
}

QString cellN3b(const QString& ref, double v) {
    return QString("<c r=\"%1\"><v>%2</v></c>").arg(ref, QString::number(v, 'f', 2));
}

QString sheetOpen3b(const QString& dimRef) {
    return "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>\n"
        "<worksheet xmlns=\"http://schemas.openxmlformats.org/spreadsheetml/2006/main\">"
        + QString("<dimension ref=\"%1\"/>").arg(dimRef) +
        "<sheetViews><sheetView workbookViewId=\"0\"/></sheetViews>"
        "<sheetFormat defaultRowHeight=\"15\"/><sheetData>";
}

QString dvDecimal3b(const QString& sqref) {
    return QString("<dataValidation type=\"decimal\" operator=\"greaterThanOrEqual\" allowBlank=\"1\" "
                   "showDropDown=\"1\" showErrorMessage=\"1\" sqref=\"%1\">"
                   "<formula1>0</formula1></dataValidation>").arg(sqref);
}

QString dvList3b(const QString& sqref, const QString& name) {
    return QString("<dataValidation type=\"list\" allowBlank=\"1\" showDropDown=\"1\" sqref=\"%1\">"
                   "<formula1>%2</formula1></dataValidation>").arg(sqref, name);
}

// Master POS column, verbatim from the v1.2 template (old spellings kept).
QStringList masterPos3b() {
    return {"01-Jammu & Kashmir","02-Himachal Pradesh","03-Punjab","04-Chandigarh",
        "05-Uttarakhand","06-Haryana","07-Delhi","08-Rajasthan","09-Uttar Pradesh",
        "10-Bihar","11-Sikkim","12-Arunachal Pradesh","13-Nagaland","14-Manipur",
        "15-Mizoram","16-Tripura","17-Meghalaya","18-Assam","19-West Bengal",
        "20-Jharkhand","21-Odisha","22-Chhattisgarh","23-Madhya Pradesh","24-Gujarat",
        "25-Daman & Diu","26-Dadra & Nagar Haveli","27-Maharashtra","29-Karnataka",
        "30-Goa","31-Lakshdweep","32-Kerala","33-Tamil Nadu","34-Pondicherry",
        "35-Andaman & Nicobar Islands","36-Telengana","37-Andhra Pradesh"};
}

} // namespace

bool Gstr3BEngine::exportToExcelTemplate(
    const Gstr3BReturnSummary& summary,
    const QString& outputPath,
    const QString& templatePath
) {
    Q_UNUSED(templatePath);
    // From-scratch GSTR-3B v1.2 workbook: same 9 sheets in order, same labels,
    // same validations + defined names. No template file dependency.
    // NOTE: 6.1/4(C) carry computed VALUES, not the template's live formulas:
    // the template's 6.1 formula adds the 3.2 column on top of 3.1 (3.2 is a
    // subset, not additive), so keeping it would double-count once 3.2 rows
    // are filled. The offline tool recomputes on import regardless.

    QStringList sheetNames = {"Master","Index","3.1","3.1.1","3.2","4","5","6.1","6.2"};
    QMap<QString, QString> xmlByName;

    // Master
    {
        QString x = sheetOpen3b("A1:B36") + "<row r=\"1\">"
            + cellT3b("A1", "Supplies made to Unregistered Persons", true)
            + cellT3b("B1", masterPos3b().at(0)) + "</row>"
            + "<row r=\"2\">"
            + cellT3b("A2", "Supplies Made to Composition Taxable Persons", true)
            + cellT3b("B2", masterPos3b().at(1)) + "</row>"
            + "<row r=\"3\">"
            + cellT3b("A3", "Supplies made to UIN Holders", true)
            + cellT3b("B3", masterPos3b().at(2)) + "</row>";
        QStringList pos = masterPos3b();
        for (int r = 4; r <= 36; r++)
            x += QString("<row r=\"%1\">%2</row>").arg(r).arg(cellT3b("B" + QString::number(r), pos.at(r - 1)));
        x += "</sheetData></worksheet>";
        xmlByName["Master"] = x;
    }

    // Index
    {
        QString fp = summary.returnPeriod; // MMYYYY
        QString yy = fp.length() == 6 ? fp.mid(2, 4) : "";
        QString mm = fp.length() == 6 ? QString::number(fp.left(2).toInt()) : "";
        QString x = sheetOpen3b("A1:C15")
            + "<row r=\"1\">" + cellT3b("A1", "Form GSTR-3B", true) + "</row>"
            + "<row r=\"3\">" + cellT3b("A3", "1. GSTIN", true) + cellT3b("B3", summary.gstin) + "</row>"
            + "<row r=\"4\">" + cellT3b("A4", "2. Legal Name of the registered person", true) + cellT3b("B4", summary.legalName) + "</row>"
            + "<row r=\"5\">" + cellT3b("A5", "Year", true) + cellT3b("B5", yy) + "</row>"
            + "<row r=\"6\">" + cellT3b("A6", "Month", true) + cellT3b("B6", mm) + "</row>"
            + "<row r=\"8\">" + cellT3b("A8", "Forms:", true) + "</row>";
        QStringList forms = {
            "3.1 Details of outward Supplies and inward supplies liable to reverse charge",
            "3.1.1 Details of supplies notified under section 9(5) of the CGST Act, 2017 and corresponding provision in IGST/UTGST/SGST Acts",
            "3.2 Of the supplies shown in 3.1 (a) above, details of inter-State supplies made to unregistered persons,composition taxable persons and UIN holders",
            "4. Eligible ITC", "5. Value of exempt,nal-rated and non-GST inward supplies",
            "6.1 Payment of tax", "6.2 TDS/TCS Credit"};
        for (int i = 0; i < forms.size(); i++)
            x += QString("<row r=\"%1\">%2%3</row>").arg(9 + i)
                .arg(cellT3b("A" + QString::number(9 + i), forms[i], true))
                .arg(cellT3b("C" + QString::number(9 + i), "View"));
        x += "</sheetData></worksheet>";
        xmlByName["Index"] = x;
    }

    auto headers6 = [&](const QStringList& hs) {
        QString r;
        for (int c = 0; c < hs.size(); c++)
            r += cellT3b(colName3b(c) + "2", hs[c], true);
        return r;
    };

    // 3.1
    {
        QStringList descs = {
            "(a) Outward taxable supplies (Other than zero rated, nil rated and exempted)",
            "(b) Outward taxable supplies (zero rated)",
            "(c) Other outward supplies (nil rated, exempted)",
            "(d) Inward supplies (liable to reverse charge)",
            "(e) Non-GST outward supplies"};
        QVector<QVector<double>> vals = {
            {summary.table31.txValA, summary.table31.iAmtA, summary.table31.cAmtA, summary.table31.sAmtA, summary.table31.csAmtA},
            {summary.table31.txValB, summary.table31.iAmtB, 0.0, 0.0, summary.table31.csAmtB},
            {summary.table31.txValC, 0.0, 0.0, 0.0, 0.0},
            {summary.table31.txValD, summary.table31.iAmtD, summary.table31.cAmtD, summary.table31.sAmtD, summary.table31.csAmtD},
            {summary.table31.txValE, 0.0, 0.0, 0.0, 0.0}};
        QString x = sheetOpen3b("A1:F7")
            + "<row r=\"1\">" + cellT3b("A1", "3.1 Details of outward Supplies and inward supplies liable to reverse charge", true) + cellT3b("F1", "Back to Index") + "</row>"
            + "<row r=\"2\">" + headers6({"Nature of Supply","Total Taxable Value","Integrated Tax","Central Tax","State/UT Tax","Cess"}) + "</row>";
        for (int i = 0; i < 5; i++) {
            x += QString("<row r=\"%1\">%2").arg(3 + i).arg(cellT3b("A" + QString::number(3 + i), descs[i]));
            for (int c = 0; c < 5; c++)
                x += cellN3b(colName3b(c + 1) + QString::number(3 + i), vals[i][c]);
            x += "</row>";
        }
        x += "</sheetData><dataValidations count=\"1\">" + dvDecimal3b("B3:F7") + "</dataValidations></worksheet>";
        xmlByName["3.1"] = x;
    }

    // 3.1.1 (no 9(5) supplies in this trade: zeros, structure preserved)
    {
        QStringList descs = {
            "(i) Taxable supplies on which electronic commerce operator pays tax u/s 9(5)\n[to be furnished by electronic commerce operator]",
            "(ii) Taxable supplies made by registered person through electronic commerce operator, on which electronic commerce operator is required to pay tax u/s 9(5)\n[to be furnished by registered person making supplies through electronic commerce operator]"};
        QString x = sheetOpen3b("A1:F4")
            + "<row r=\"1\">" + cellT3b("A1", "3.1.1 Details of supplies notified under section 9(5) of the CGST Act, 2017 and corresponding provision in IGST/UTGST/SGST Acts", true) + cellT3b("F1", "Back to Index") + "</row>"
            + "<row r=\"2\">" + headers6({"Description","Total Taxable Value","Integrated Tax","Central Tax","State/UT Tax","Cess"}) + "</row>";
        for (int i = 0; i < 2; i++) {
            x += QString("<row r=\"%1\">%2").arg(3 + i).arg(cellT3b("A" + QString::number(3 + i), descs[i]));
            for (int c = 1; c <= 5; c++) x += cellN3b(colName3b(c) + QString::number(3 + i), 0.0);
            x += "</row>";
        }
        x += "</sheetData><dataValidations count=\"1\">" + dvDecimal3b("B3:F4") + "</dataValidations></worksheet>";
        xmlByName["3.1.1"] = x;
    }

    // 3.2 POS-wise rows
    {
        int n = summary.table32.rows.size();
        int lastRow = qMax(2, 2 + n);
        QString x = sheetOpen3b(QString("A1:D%1").arg(lastRow))
            + "<row r=\"1\">" + cellT3b("A1", "3.2 Of the supplies shown in 3.1 (a) above, details of inter-State supplies made to unregistered persons,composition taxable persons and UIN holders", true) + cellT3b("D1", "Back to Index") + "</row>"
            + "<row r=\"2\">" + headers6({"Description","Place of supply (State/UT)","Total Taxable Value","Amount of Integrated Tax"}) + "</row>";
        for (int i = 0; i < n; i++) {
            const auto& r = summary.table32.rows[i];
            x += QString("<row r=\"%1\">%2%3%4%5</row>").arg(3 + i)
                .arg(cellT3b("A" + QString::number(3 + i), r.desc))
                .arg(cellT3b("B" + QString::number(3 + i), r.pos))
                .arg(cellN3b("C" + QString::number(3 + i), r.txVal))
                .arg(cellN3b("D" + QString::number(3 + i), r.iAmt));
        }
        x += "</sheetData><dataValidations count=\"3\">"
            + dvList3b("A3:A1048576", "SuppliesDesc") + dvList3b("B3:B1048576", "state")
            + dvDecimal3b("C3:D1048576") + "</dataValidations></worksheet>";
        xmlByName["3.2"] = x;
    }

    // 4. Eligible ITC
    {
        QStringList descs = {"(A) ITC Available (Whether in full or part)","(1) Import of goods",
            "(2) Import of services","(3) Inward supplies liable to reverse charge (other than 1 and 2 above)",
            "(4) Inward supplies from ISD","(5) All other ITC","(B) ITC Reversed",
            "(1) As per rule 42 & 43 of CGST Rules","(2) Others","(C) Net ITC Available (A) - (B)",
            "(D) Ineligible ITC","(1) As per section 17(5)","(2) Others"};
        const auto& t = summary.table4;
        QVector<QVector<double>> vals = {
            {0,0,0,0},{t.iAmtA1,t.cAmtA1,t.sAmtA1,t.csAmtA1},{t.iAmtA2,t.cAmtA2,t.sAmtA2,t.csAmtA2},
            {t.iAmtA3,t.cAmtA3,t.sAmtA3,t.csAmtA3},{t.iAmtA4,t.cAmtA4,t.sAmtA4,t.csAmtA4},
            {t.iAmtA5,t.cAmtA5,t.sAmtA5,t.csAmtA5},{0,0,0,0},
            {t.iAmtB1,t.cAmtB1,t.sAmtB1,t.csAmtB1},{t.iAmtB2,t.cAmtB2,t.sAmtB2,t.csAmtB2},
            {t.netIgst,t.netCgst,t.netSgst,t.netCess},{0,0,0,0},
            {t.iAmtD1,t.cAmtD1,t.sAmtD1,t.csAmtD1},{t.iAmtD2,t.cAmtD2,t.sAmtD2,t.csAmtD2}};
        QString x = sheetOpen3b("A1:E15")
            + "<row r=\"1\">" + cellT3b("A1", "4. Eligible ITC", true) + cellT3b("E1", "Back to Index") + "</row>"
            + "<row r=\"2\">" + headers6({"Details","Integrated Tax","Central Tax","State/UT Tax","Cess"}) + "</row>";
        for (int i = 0; i < descs.size(); i++) {
            x += QString("<row r=\"%1\">%2").arg(3 + i).arg(cellT3b("A" + QString::number(3 + i), descs[i], i == 0 || i == 6 || i == 9 || i == 10));
            for (int c = 0; c < 4; c++)
                x += cellN3b(colName3b(c + 1) + QString::number(3 + i), vals[i][c]);
            x += "</row>";
        }
        x += "</sheetData><dataValidations count=\"1\">" + dvDecimal3b("B4:E8 B10:E11 B14:E15") + "</dataValidations></worksheet>";
        xmlByName["4"] = x;
    }

    // 5. Exempt / nil / non-GST inward
    {
        QString x = sheetOpen3b("A1:C5")
            + "<row r=\"1\">" + cellT3b("A1", "5. Value of exempt,nil-rated and non-GST inward supplies", true) + cellT3b("C1", "Back to Index") + "</row>"
            + "<row r=\"2\">" + headers6({"Nature of supplies","Inter-State supplies","Intra-state supplies"}) + "</row>"
            + "<row r=\"3\">" + cellT3b("A3", "From a supplier under composition scheme, Exempted and Nil rated")
            + cellN3b("B3", summary.table5.interExempt) + cellN3b("C3", summary.table5.intraExempt) + "</row>"
            + "<row r=\"4\">" + cellT3b("A4", "Supply") + "</row>"
            + "<row r=\"5\">" + cellT3b("A5", "Non GST supply")
            + cellN3b("B5", summary.table5.interNonGst) + cellN3b("C5", summary.table5.intraNonGst) + "</row>"
            + "</sheetData><dataValidations count=\"1\">" + dvDecimal3b("B3:C5") + "</dataValidations></worksheet>";
        xmlByName["5"] = x;
    }

    // 6.1 Payment of tax (computed values; same-type credit only, portal-legal)
    {
        const auto& p = summary.table61;
        QString x = sheetOpen3b("A1:J7")
            + "<row r=\"1\">" + cellT3b("A1", "6.1 Payment of tax", true) + cellT3b("J1", "Back to Index") + "</row>"
            + "<row r=\"2\">" + cellT3b("A2", "Description", true) + cellT3b("B2", "Tax Payable", true)
            + cellT3b("C2", "Paid through ITC", true) + cellT3b("G2", "Tax paid TDS/TCS", true)
            + cellT3b("H2", "Tax/Cess paid in cash", true) + cellT3b("I2", "Interest", true) + cellT3b("J2", "Late Fee", true) + "</row>"
            + "<row r=\"3\">" + cellT3b("C3", "Integrated Tax", true) + cellT3b("D3", "Central Tax", true)
            + cellT3b("E3", "State/UT Tax", true) + cellT3b("F3", "Cess", true) + "</row>";
        struct R { QString n; double pay, ci, cc, cs, ce, cash, intr, late; };
        QVector<R> rows = {
            {"Integrated Tax", p.taxPayableIgst, p.itcPaidIgst, 0, 0, 0, p.cashPaidIgst, p.interestPaidIgst, 0},
            {"Central Tax", p.taxPayableCgst, 0, p.itcPaidCgst, 0, 0, p.cashPaidCgst, p.interestPaidCgst, summary.table51.lateFeeCgst},
            {"State/UT Tax", p.taxPayableSgst, 0, 0, p.itcPaidSgst, 0, p.cashPaidSgst, p.interestPaidSgst, summary.table51.lateFeeSgst},
            {"Cess", p.taxPayableCess, 0, 0, 0, p.itcPaidCess, p.cashPaidCess, p.interestPaidCess, 0}};
        for (int i = 0; i < 4; i++) {
            const R& r = rows[i];
            x += QString("<row r=\"%1\">").arg(4 + i) + cellT3b("A" + QString::number(4 + i), r.n)
                + cellN3b("B" + QString::number(4 + i), r.pay) + cellN3b("C" + QString::number(4 + i), r.ci)
                + cellN3b("D" + QString::number(4 + i), r.cc) + cellN3b("E" + QString::number(4 + i), r.cs)
                + cellN3b("F" + QString::number(4 + i), r.ce) + cellN3b("G" + QString::number(4 + i), 0.0)
                + cellN3b("H" + QString::number(4 + i), r.cash) + cellN3b("I" + QString::number(4 + i), r.intr)
                + cellN3b("J" + QString::number(4 + i), r.late) + "</row>";
        }
        x += "</sheetData><dataValidations count=\"1\">" + dvDecimal3b("B4:F7 H4:J7") + "</dataValidations></worksheet>";
        xmlByName["6.1"] = x;
    }

    // 6.2 TDS/TCS credit (not tracked in this app: zeros, structure preserved)
    {
        QString x = sheetOpen3b("A1:D4")
            + "<row r=\"1\">" + cellT3b("A1", "6.2 TDS/TCS Credit", true) + cellT3b("D1", "Back to Index") + "</row>"
            + "<row r=\"2\">" + headers6({"Details","Integrated Tax","Central Tax","State/UT Tax"}) + "</row>"
            + "<row r=\"3\">" + cellT3b("A3", "TDS") + cellN3b("B3", 0.0) + cellN3b("C3", 0.0) + cellN3b("D3", 0.0) + "</row>"
            + "<row r=\"4\">" + cellT3b("A4", "TCS") + cellN3b("B4", 0.0) + cellN3b("C4", 0.0) + cellN3b("D4", 0.0) + "</row>"
            + "</sheetData><dataValidations count=\"1\">" + dvDecimal3b("B3:D4") + "</dataValidations></worksheet>";
        xmlByName["6.2"] = x;
    }

    // ---------- package ----------
    auto sheetFile = [&](int i) { return QString("xl/worksheets/sheet%1.xml").arg(i + 1); };
    QString contentTypes = "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>\n"
        "<Types xmlns=\"http://schemas.openxmlformats.org/package/2006/content-types\">"
        "<Default Extension=\"rels\" ContentType=\"application/vnd.openxml-package.relationships+xml\"/>"
        "<Default Extension=\"xml\" ContentType=\"application/xml\"/>"
        "<Override PartName=\"/xl/workbook.xml\" ContentType=\"application/vnd.openxmlformats-officedocument.spreadsheetml.sheet.main+xml\"/>"
        "<Override PartName=\"/xl/styles.xml\" ContentType=\"application/vnd.openxmlformats-officedocument.spreadsheetml.styles+xml\"/>"
        "<Override PartName=\"/docProps/core.xml\" ContentType=\"application/vnd.openxml-package.core-properties+xml\"/>"
        "<Override PartName=\"/docProps/app.xml\" ContentType=\"application/vnd.openxmlformats-officedocument.extended-properties+xml\"/>";
    for (int i = 0; i < sheetNames.size(); i++)
        contentTypes += QString("<Override PartName=\"/%1\" ContentType=\"application/vnd.openxmlformats-officedocument.spreadsheetml.worksheet+xml\"/>").arg(sheetFile(i));
    contentTypes += "</Types>";

    QString pkgRels = "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>\n"
        "<Relationships xmlns=\"http://schemas.openxmlformats.org/package/2006/relationships\">"
        "<Relationship Id=\"rId1\" Type=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships/officeDocument\" Target=\"xl/workbook.xml\"/>"
        "<Relationship Id=\"rId2\" Type=\"http://schemas.openxmlformats.org/package/2006/relationships/metadata/core-properties\" Target=\"docProps/core.xml\"/>"
        "<Relationship Id=\"rId3\" Type=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships/extended-properties\" Target=\"docProps/app.xml\"/>"
        "</Relationships>";

    QString wbRels = "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>\n"
        "<Relationships xmlns=\"http://schemas.openxmlformats.org/package/2006/relationships\">";
    for (int i = 0; i < sheetNames.size(); i++)
        wbRels += QString("<Relationship Id=\"rId%1\" Type=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships/worksheet\" Target=\"worksheets/sheet%1.xml\"/>").arg(i + 1);
    wbRels += QString("<Relationship Id=\"rId%1\" Type=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships/styles\" Target=\"styles.xml\"/>").arg(sheetNames.size() + 1);
    wbRels += "</Relationships>";

    QString workbook = "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>\n"
        "<workbook xmlns=\"http://schemas.openxmlformats.org/spreadsheetml/2006/main\" "
        "xmlns:r=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships\">"
        "<sheets>";
    for (int i = 0; i < sheetNames.size(); i++)
        workbook += QString("<sheet name=\"%1\" sheetId=\"%2\" r:id=\"rId%2\"/>").arg(esc3b(sheetNames[i])).arg(i + 1);
    workbook += "</sheets><definedNames>"
        "<definedName name=\"state\">Master!$B$1:$B$36</definedName>"
        "<definedName name=\"SuppliesDesc\">Master!$A$1:$A$3</definedName>"
        "</definedNames></workbook>";

    QString styles = "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>\n"
        "<styleSheet xmlns=\"http://schemas.openxmlformats.org/spreadsheetml/2006/main\">"
        "<fonts count=\"2\"><font><sz val=\"11\"/><name val=\"Calibri\"/></font>"
        "<font><b/><sz val=\"11\"/><name val=\"Calibri\"/></font></fonts>"
        "<fills count=\"2\"><fill><patternFill patternType=\"none\"/></fill>"
        "<fill><patternFill patternType=\"gray125\"/></fill></fills>"
        "<borders count=\"1\"><border/></borders>"
        "<cellStyleXfs count=\"1\"><xf numFmtId=\"0\" fontId=\"0\" fillId=\"0\" borderId=\"0\"/></cellStyleXfs>"
        "<cellXfs count=\"2\"><xf numFmtId=\"0\" fontId=\"0\" fillId=\"0\" borderId=\"0\" xfId=\"0\"/>"
        "<xf numFmtId=\"0\" fontId=\"1\" fillId=\"0\" borderId=\"0\" xfId=\"0\" applyFont=\"1\"/></cellXfs>"
        "</styleSheet>";

    QString core = "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>\n"
        "<cp:coreProperties xmlns:cp=\"http://schemas.openxmlformats.org/package/2006/metadata/core-properties\" "
        "xmlns:dc=\"http://purl.org/dc/elements/1.1/\" xmlns:dcterms=\"http://purl.org/dc/terms/\" "
        "xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\">"
        "<dc:creator>MahadevRiceMillERP</dc:creator><cp:lastModifiedBy>MahadevRiceMillERP</cp:lastModifiedBy>"
        "<dcterms:created xsi:type=\"dcterms:W3CDTF\">2025-01-01T00:00:00Z</dcterms:created>"
        "<dcterms:modified xsi:type=\"dcterms:W3CDTF\">2025-01-01T00:00:00Z</dcterms:modified>"
        "</cp:coreProperties>";
    QString app = "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>\n"
        "<Properties xmlns=\"http://schemas.openxmlformats.org/officeDocument/2006/extended-properties\">"
        "<Application>MahadevRiceMillERP GSTR-3B</Application>"
        "</Properties>";

    mz_zip_archive outZip;
    memset(&outZip, 0, sizeof(outZip));
    if (!mz_zip_writer_init_heap(&outZip, 0, 0)) return false;
    auto addPart = [&](const QString& name, const QString& xmlText) -> bool {
        QByteArray bytes = xmlText.toUtf8();
        return mz_zip_writer_add_mem(&outZip, name.toUtf8().constData(),
                                     bytes.constData(), bytes.size(), MZ_DEFAULT_COMPRESSION) != 0;
    };
    bool addOk = true;
    addOk = addOk && addPart("[Content_Types].xml", contentTypes);
    addOk = addOk && addPart("_rels/.rels", pkgRels);
    addOk = addOk && addPart("xl/workbook.xml", workbook);
    addOk = addOk && addPart("xl/_rels/workbook.xml.rels", wbRels);
    addOk = addOk && addPart("xl/styles.xml", styles);
    addOk = addOk && addPart("docProps/core.xml", core);
    addOk = addOk && addPart("docProps/app.xml", app);
    for (int i = 0; i < sheetNames.size() && addOk; i++)
        addOk = addOk && addPart(sheetFile(i), xmlByName[sheetNames[i]]);
    if (!addOk) {
        mz_zip_writer_end(&outZip);
        return false;
    }
    void* pOutBuf = nullptr;
    size_t outSize = 0;
    if (!mz_zip_writer_finalize_heap_archive(&outZip, &pOutBuf, &outSize)) {
        mz_zip_writer_end(&outZip);
        return false;
    }
    QFileInfo outInfo(outputPath);
    QDir().mkpath(outInfo.absolutePath());
    QFile outFile(outputPath);
    if (!outFile.open(QIODevice::WriteOnly)) {
        mz_free(pOutBuf);
        mz_zip_writer_end(&outZip);
        return false;
    }
    outFile.write((const char*)pOutBuf, (qint64)outSize);
    outFile.close();
    mz_free(pOutBuf);
    mz_zip_writer_end(&outZip);
    return true;
}

} // namespace MahadevERP
