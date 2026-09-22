#include "gstr1_engine.h"
#include "gst_tax_engine.h"
#include "../database_manager.h"
#include <QSqlQuery>
#include <QSqlRecord>
#include <QMap>
#include <cmath>

namespace MahadevERP {

static double round2(double val) {
    return std::round(val * 100.0) / 100.0;
}

Gstr1ReturnPayload Gstr1Engine::generateFromDatabase(
    const QString& gstin,
    const QString& legalName,
    const QString& stateCode,
    const QDate& fromDate,
    const QDate& toDate
) {
    Gstr1ReturnPayload payload;
    payload.gstin = gstin;
    payload.legalName = legalName;
    payload.fp = QString("%1%2").arg(fromDate.month(), 2, 10, QChar('0')).arg(fromDate.year());

    QString fromStr = fromDate.toString("yyyy-MM-dd");
    QString toStr = toDate.toString("yyyy-MM-dd");

    // 1. Query all Sales invoices from database within date range
    QString sql = QString(
        "SELECT "
        "  s.id, s.invoice_no, s.invoice_date, "
        "  COALESCE(s.customer_name, 'Cash Customer') AS party_name, "
        "  COALESCE(s.gstin, '') AS party_gstin, "
        "  COALESCE(s.place_of_supply, '') AS place_of_supply, "
        "  s.total_amount, "
        "  COALESCE(s.gst_amount, s.cgst_amount + s.sgst_amount + s.igst_amount, 0.0) AS tax_amount, "
        "  COALESCE(si.item_name, s.item_name, 'Rice Commodity') AS item_name, "
        "  COALESCE(s.hsn_code, '1006') AS hsn_code, "
        "  COALESCE(si.weight_qtl, s.weight_qtl, 0.0) AS quantity, "
        "  COALESCE(si.rate_per_qtl, s.rate_per_qtl, 0.0) AS rate, "
        "  COALESCE(si.gst_pct, s.gst_pct, 5.0) AS tax_rate, "
        "  COALESCE(si.taxable_amount, s.taxable_amount, s.total_amount) AS amount "
        "FROM sales_invoices s "
        "LEFT JOIN sales_invoice_items si ON s.id = si.invoice_id "
        "WHERE s.invoice_date >= '%1' AND s.invoice_date <= '%2' "
        "ORDER BY s.invoice_date ASC, s.id ASC;"
    ).arg(fromStr, toStr);

    QVariantList rawRows = DatabaseManager::instance().executeQuery(sql);

    // Group rows by invoice_no
    struct TempInv {
        QString invNo;
        QDate invDate;
        QString partyName;
        QString partyGstin;
        QString pos;
        double totalVal = 0.0;
        QList<QVariantMap> items;
    };
    QMap<QString, TempInv> invMap;
    QMap<QString, Gstr1HsnItem> hsnMap;

    for (const auto& var : rawRows) {
        QVariantMap r = var.toMap();
        QString invNo = r.value("invoice_no").toString();
        if (invNo.isEmpty()) continue;

        if (!invMap.contains(invNo)) {
            TempInv ti;
            ti.invNo = invNo;
            QString dStr = r.value("invoice_date").toString();
            ti.invDate = QDate::fromString(dStr, "yyyy-MM-dd");
            if (!ti.invDate.isValid()) ti.invDate = QDate::fromString(dStr, Qt::ISODate);
            ti.partyName = r.value("party_name").toString();
            ti.partyGstin = r.value("party_gstin").toString().trimmed();
            
            // Derive POS from party GSTIN or fallback to stateCode
            QString gstinPos = GstTaxEngine::extractStateCodeFromGstin(ti.partyGstin);
            ti.pos = gstinPos.isEmpty() ? stateCode : gstinPos;
            ti.totalVal = r.value("total_amount").toDouble();
            invMap.insert(invNo, ti);
        }

        invMap[invNo].items.append(r);

        // Aggregate HSN
        QString hsn = r.value("hsn_code").toString().trimmed();
        if (hsn.isEmpty()) hsn = "1006"; // Default Rice / Agricultural HSN
        
        double qty = r.value("quantity").toDouble();
        double itemAmt = r.value("amount").toDouble();
        double taxRate = r.value("tax_rate").toDouble();
        if (taxRate <= 0.0) taxRate = 5.0;

        bool isIntra = (invMap[invNo].pos == stateCode);
        double igst = isIntra ? 0.0 : round2(itemAmt * (taxRate / 100.0));
        double cgst = isIntra ? round2(itemAmt * (taxRate / 200.0)) : 0.0;
        double sgst = isIntra ? round2(itemAmt * (taxRate / 200.0)) : 0.0;

        if (!hsnMap.contains(hsn)) {
            Gstr1HsnItem hi;
            hi.serialNo = hsnMap.size() + 1;
            hi.hsnCode = hsn;
            hi.description = r.value("item_name").toString();
            hi.uqc = "QTL";
            hsnMap.insert(hsn, hi);
        }
        hsnMap[hsn].totalQty += qty;
        hsnMap[hsn].totalValue += (itemAmt + igst + cgst + sgst);
        hsnMap[hsn].taxableValue += itemAmt;
        hsnMap[hsn].igst += igst;
        hsnMap[hsn].cgst += cgst;
        hsnMap[hsn].sgst += sgst;
    }

    // Classify into B2B, B2CL, B2CS
    QMap<QString, Gstr1B2CSSummary> b2csAgg;

    for (const auto& ti : invMap) {
        payload.grossTurnover += ti.totalVal;
        bool isRegistered = (ti.partyGstin.length() == 15);
        bool isIntra = (ti.pos == stateCode);

        if (isRegistered) {
            // Table 4: B2B
            Gstr1B2BInvoice b2bInv;
            b2bInv.ctin = ti.partyGstin;
            b2bInv.receiverName = ti.partyName;
            b2bInv.invoiceNo = ti.invNo;
            b2bInv.invoiceDate = ti.invDate;
            b2bInv.invoiceValue = ti.totalVal;
            b2bInv.pos = ti.pos;

            for (const auto& item : ti.items) {
                Gstr1B2BItem bi;
                bi.rate = item.value("tax_rate").toDouble();
                if (bi.rate <= 0.0) bi.rate = 5.0;
                bi.taxableValue = item.value("amount").toDouble();
                if (isIntra) {
                    bi.cgst = round2(bi.taxableValue * (bi.rate / 200.0));
                    bi.sgst = round2(bi.taxableValue * (bi.rate / 200.0));
                    bi.igst = 0.0;
                } else {
                    bi.igst = round2(bi.taxableValue * (bi.rate / 100.0));
                    bi.cgst = 0.0;
                    bi.sgst = 0.0;
                }
                b2bInv.items.append(bi);
            }
            payload.b2b.append(b2bInv);
        } else {
            // Unregistered Customer
            if (!isIntra && ti.totalVal > 250000.0) {
                // Table 5: B2CL (Inter-state Large > 2.5 Lakhs)
                for (const auto& item : ti.items) {
                    Gstr1B2CLInvoice b2cl;
                    b2cl.invoiceNo = ti.invNo;
                    b2cl.invoiceDate = ti.invDate;
                    b2cl.invoiceValue = ti.totalVal;
                    b2cl.pos = ti.pos;
                    b2cl.rate = item.value("tax_rate").toDouble();
                    if (b2cl.rate <= 0.0) b2cl.rate = 5.0;
                    b2cl.taxableValue = item.value("amount").toDouble();
                    b2cl.igst = round2(b2cl.taxableValue * (b2cl.rate / 100.0));
                    payload.b2cl.append(b2cl);
                }
            } else {
                // Table 7: B2CS (Small Invoices aggregated by POS + Rate)
                for (const auto& item : ti.items) {
                    double r = item.value("tax_rate").toDouble();
                    if (r <= 0.0) r = 5.0;
                    QString key = QString("%1_%2").arg(ti.pos).arg(r);
                    if (!b2csAgg.contains(key)) {
                        Gstr1B2CSSummary cs;
                        cs.pos = ti.pos;
                        cs.rate = r;
                        b2csAgg.insert(key, cs);
                    }
                    double amt = item.value("amount").toDouble();
                    b2csAgg[key].taxableValue += amt;
                    if (isIntra) {
                        b2csAgg[key].cgst += round2(amt * (r / 200.0));
                        b2csAgg[key].sgst += round2(amt * (r / 200.0));
                    } else {
                        b2csAgg[key].igst += round2(amt * (r / 100.0));
                    }
                }
            }
        }
    }

    for (const auto& cs : b2csAgg) {
        payload.b2cs.append(cs);
    }
    for (const auto& hi : hsnMap) {
        payload.hsn.append(hi);
    }

    // Document Register Table 13
    if (!invMap.isEmpty()) {
        Gstr1DocSummary doc;
        doc.docType = 1;
        doc.docName = "Invoices for outward supply";
        doc.fromSerial = invMap.firstKey();
        doc.toSerial = invMap.lastKey();
        doc.totalCount = invMap.size();
        doc.cancelledCount = 0;
        doc.netIssuedCount = doc.totalCount;
        payload.docs.append(doc);
    }

    return payload;
}

QJsonDocument Gstr1Engine::exportToGovtOfflineJson(const Gstr1ReturnPayload& payload) {
    QJsonObject root;
    root["gstin"] = payload.gstin;
    root["fp"] = payload.fp;
    root["gt"] = payload.grossTurnover;
    root["cur_gt"] = payload.grossTurnover;
    root["version"] = "GST3.0.4";

    // 1. B2B Array
    QJsonArray b2bArray;
    QMap<QString, QJsonArray> b2bByGstin;
    for (const auto& inv : payload.b2b) {
        QJsonObject invObj;
        invObj["inum"] = inv.invoiceNo;
        invObj["idt"] = inv.invoiceDate.toString("dd-MM-yyyy");
        invObj["val"] = inv.invoiceValue;
        invObj["pos"] = inv.pos;
        invObj["rchrg"] = inv.reverseCharge;
        invObj["inv_typ"] = inv.invoiceType;

        QJsonArray itmsArray;
        int num = 1;
        for (const auto& itm : inv.items) {
            QJsonObject itemDetail;
            itemDetail["rt"] = itm.rate;
            itemDetail["txval"] = itm.taxableValue;
            itemDetail["iamt"] = itm.igst;
            itemDetail["camt"] = itm.cgst;
            itemDetail["samt"] = itm.sgst;
            itemDetail["csamt"] = itm.cess;

            QJsonObject itmObj;
            itmObj["num"] = num++;
            itmObj["itm_det"] = itemDetail;
            itmsArray.append(itmObj);
        }
        invObj["itms"] = itmsArray;
        b2bByGstin[inv.ctin].append(invObj);
    }

    for (auto it = b2bByGstin.begin(); it != b2bByGstin.end(); ++it) {
        QJsonObject ctinObj;
        ctinObj["ctin"] = it.key();
        ctinObj["inv"] = it.value();
        b2bArray.append(ctinObj);
    }
    root["b2b"] = b2bArray;

    // 2. B2CS Array
    QJsonArray b2csArray;
    for (const auto& cs : payload.b2cs) {
        QJsonObject csObj;
        csObj["sply_ty"] = cs.supplyType;
        csObj["pos"] = cs.pos;
        csObj["rt"] = cs.rate;
        csObj["txval"] = cs.taxableValue;
        csObj["iamt"] = cs.igst;
        csObj["camt"] = cs.cgst;
        csObj["samt"] = cs.sgst;
        csObj["csamt"] = cs.cess;
        b2csArray.append(csObj);
    }
    root["b2cs"] = b2csArray;

    // 3. HSN Array
    QJsonObject hsnSection;
    QJsonArray hsnData;
    for (const auto& h : payload.hsn) {
        QJsonObject hObj;
        hObj["num"] = h.serialNo;
        hObj["hsn_sc"] = h.hsnCode;
        hObj["desc"] = h.description;
        hObj["uqc"] = h.uqc;
        hObj["qty"] = h.totalQty;
        hObj["val"] = h.totalValue;
        hObj["txval"] = h.taxableValue;
        hObj["iamt"] = h.igst;
        hObj["camt"] = h.cgst;
        hObj["samt"] = h.sgst;
        hObj["csamt"] = h.cess;
        hsnData.append(hObj);
    }
    hsnSection["data"] = hsnData;
    root["hsn"] = hsnSection;

    // 4. Document Issue Section
    QJsonObject docSection;
    QJsonArray docDetArray;
    for (const auto& d : payload.docs) {
        QJsonObject dObj;
        dObj["doc_num"] = d.docType;
        QJsonArray docsArray;
        QJsonObject item;
        item["num"] = 1;
        item["from"] = d.fromSerial;
        item["to"] = d.toSerial;
        item["totnum"] = d.totalCount;
        item["canc"] = d.cancelledCount;
        item["net_issue"] = d.netIssuedCount;
        docsArray.append(item);
        dObj["docs"] = docsArray;
        docDetArray.append(dObj);
    }
    docSection["doc_det"] = docDetArray;
    root["doc_issue"] = docSection;

    return QJsonDocument(root);
}

} // namespace MahadevERP
