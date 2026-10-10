#include "gstr1_engine.h"
#include "gst_tax_engine.h"
#include "accounting_engine.h"
#include "../database_manager.h"
#include <QSqlQuery>
#include <QSqlRecord>
#include <QMap>
#include <QSet>
#include <QRegularExpression>
#include <cmath>
#include <cstring>

namespace MahadevERP {

static double round2(double val) {
    return std::round(val * 100.0) / 100.0;
}

bool Gstr1Engine::isValidGstRate(double rate) {
    static const QList<double> kRates = {0.0, 0.1, 0.25, 1.0, 1.5, 3.0, 5.0,
                                         6.0, 7.5, 12.0, 18.0, 28.0, 40.0};
    for (double r : kRates) {
        if (std::fabs(r - rate) < 1e-9) return true;
    }
    return false;
}

bool Gstr1Engine::isValidInvoiceNo(const QString& invNo) {
    // Portal rule: max 16 characters, alphanumeric plus '-' and '/' only.
    if (invNo.isEmpty() || invNo.length() > 16) return false;
    static const QRegularExpression re("^[A-Za-z0-9/\\-]+$");
    return re.match(invNo).hasMatch();
}

bool Gstr1Engine::isValidGstin(const QString& gstin) {
    // Structure: SS(01-37 except 28, 38, 97) + PAN [A-Z]{5}[0-9]{4}[A-Z] +
    // entity code [A-Z0-9] + 'Z' + checksum [A-Z0-9]; then mod-36 checksum.
    static const QRegularExpression re(
        "^(\\d{2})([A-Z]{5}[0-9]{4}[A-Z])([A-Z0-9])(Z)([A-Z0-9])$");
    QString g = gstin.trimmed().toUpper();
    auto m = re.match(g);
    if (!m.hasMatch()) return false;
    int state = m.captured(1).toInt();
    bool stateOk = (state >= 1 && state <= 37 && state != 28) || state == 38 || state == 97;
    if (!stateOk) return false;

    static const char* kCharset = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    auto codeOf = [&](QChar c) -> int {
        const char* p = strchr(kCharset, c.toLatin1());
        return p ? int(p - kCharset) : -1;
    };
    // Proven against 1243/1246 real dealer GSTINs in firm data: factor
    // starts at 1 and alternates 1,2,1,2... (3 misses are source typos).
    int sum = 0, factor = 1;
    for (int i = 0; i < 14; i++) {
        int code = codeOf(g.at(i));
        if (code < 0) return false;
        int digit = factor * code;
        factor = (factor == 2) ? 1 : 2;
        digit = (digit / 36) + (digit % 36);
        sum += digit;
    }
    int checkPoint = (36 - (sum % 36)) % 36;
    return codeOf(g.at(14)) == checkPoint;
}

QString Gstr1Engine::stateNameForCode(const QString& code) {
    static const QMap<QString, QString> kStates = {
        {"01", "Jammu & Kashmir"}, {"02", "Himachal Pradesh"}, {"03", "Punjab"},
        {"04", "Chandigarh"}, {"05", "Uttarakhand"}, {"06", "Haryana"},
        {"07", "Delhi"}, {"08", "Rajasthan"}, {"09", "Uttar Pradesh"},
        {"10", "Bihar"}, {"11", "Sikkim"}, {"12", "Arunachal Pradesh"},
        {"13", "Nagaland"}, {"14", "Manipur"}, {"15", "Mizoram"},
        {"16", "Tripura"}, {"17", "Meghalaya"}, {"18", "Assam"},
        {"19", "West Bengal"}, {"20", "Jharkhand"}, {"21", "Odisha"},
        {"22", "Chhattisgarh"}, {"23", "Madhya Pradesh"}, {"24", "Gujarat"},
        {"25", "Daman & Diu"}, {"26", "Dadra & Nagar Haveli"},
        {"27", "Maharashtra"}, {"29", "Karnataka"}, {"30", "Goa"},
        {"31", "Lakshdweep"}, {"32", "Kerala"}, {"33", "Tamil Nadu"},
        {"34", "Puducherry"}, {"35", "Andaman & Nicobar Islands"},
        {"36", "Telangana"}, {"37", "Andhra Pradesh"}, {"38", "Ladakh"},
        {"97", "Other Territory"},
    };
    return kStates.value(code.trimmed().rightJustified(2, '0'), QString());
}

QString Gstr1Engine::posDisplayName(const QString& code) {
    QString c = code.trimmed().rightJustified(2, '0');
    QString name = stateNameForCode(c);
    if (name.isEmpty()) return c;
    return c + "-" + name; // exactly the offline-tool dropdown text
}

namespace {

// Numeric-aware series key: trailing sequence as number, whole string as tiebreak.
QPair<long long, QString> seriesKey(const QString& s) {
    static const QRegularExpression tailRe("(\\d+)$");
    auto m = tailRe.match(s.trimmed());
    long long n = m.hasMatch() ? m.captured(1).toLongLong() : 0;
    return qMakePair(n, s);
}

} // namespace

Gstr1ReturnPayload Gstr1Engine::generateFromDatabase(
    const QString& gstin,
    const QString& legalName,
    const QString& stateCode,
    const QDate& fromDate,
    const QDate& toDate
) {
    Gstr1ReturnPayload payload;
    payload.gstin = gstin.trimmed().toUpper();
    payload.legalName = legalName;
    payload.fp = QString("%1%2").arg(fromDate.month(), 2, 10, QChar('0')).arg(fromDate.year());
    QString supplierState = stateCode.trimmed().rightJustified(2, '0');

    QString fromStr = fromDate.toString("yyyy-MM-dd");
    QString toStr = toDate.toString("yyyy-MM-dd");

    // 1. Sales invoices in range (deterministic order).
    QString sql = QString(
        "SELECT "
        "  s.id, s.invoice_no, s.invoice_date, "
        "  COALESCE(s.customer_name, 'Cash Customer') AS party_name, "
        "  COALESCE(s.gstin, '') AS party_gstin, "
        "  COALESCE(s.place_of_supply, '') AS place_of_supply, "
        "  COALESCE(si.item_name, s.item_name, stk.name, 'Commodity') AS item_name, "
        "  COALESCE(stk.hsn_code, s.hsn_code, '') AS hsn_code, "
        "  COALESCE(si.weight_qtl, s.weight_qtl, 0.0) AS quantity, "
        "  COALESCE(si.gst_pct, stk.gst_rate, s.gst_pct, 0.0) AS tax_rate, "
        "  COALESCE(si.taxable_amount, s.taxable_amount, s.total_amount) AS amount "
        "FROM sales_invoices s "
        "LEFT JOIN sales_invoice_items si ON s.id = si.invoice_id "
        "LEFT JOIN stock_items stk ON (si.item_id = stk.id OR (si.item_id IS NULL AND s.item_id = stk.id) OR stk.name = s.item_name OR stk.name = si.item_name) "
        "WHERE s.invoice_date >= '%1' AND s.invoice_date <= '%2' "
        "ORDER BY s.invoice_date ASC, s.id ASC;"
    ).arg(fromStr, toStr);

    QVariantList rawRows = DatabaseManager::instance().executeQuery(sql);

    struct TempInv {
        QString invNo;
        QDate invDate;
        QString partyName;
        QString partyGstin;
        QString pos;
        QList<QVariantMap> items;
    };
    QMap<QString, TempInv> invMap; // keyed by invoice_no; ORDER BY above keeps it deterministic
    QList<QString> invOrder;

    for (const auto& var : rawRows) {
        QVariantMap r = var.toMap();
        QString invNo = r.value("invoice_no").toString().trimmed();
        if (invNo.isEmpty()) continue;
        if (!invMap.contains(invNo)) {
            TempInv ti;
            ti.invNo = invNo;
            QString dStr = r.value("invoice_date").toString();
            ti.invDate = QDate::fromString(dStr, "yyyy-MM-dd");
            if (!ti.invDate.isValid()) ti.invDate = QDate::fromString(dStr, Qt::ISODate);
            ti.partyName = r.value("party_name").toString();
            ti.partyGstin = r.value("party_gstin").toString().trimmed().toUpper();
            QString gstinPos = GstTaxEngine::extractStateCodeFromGstin(ti.partyGstin);
            if (gstinPos.isEmpty()) gstinPos = r.value("place_of_supply").toString().trimmed();
            ti.pos = gstinPos.isEmpty() ? supplierState : gstinPos.rightJustified(2, '0');
            invMap.insert(invNo, ti);
            invOrder.append(invNo);
        }
        invMap[invNo].items.append(r);
    }

    QMap<QString, Gstr1HsnItem> hsnB2BMap, hsnB2CMap;
    QMap<QString, Gstr1B2CSSummary> b2csAgg;
    QStringList invSeries, creditSeries, debitSeries;

    // Table-8 exempt buckets: 0=reg+inter, 1=reg+intra, 2=unreg+inter, 3=unreg+intra.
    // Rice (unbranded/unpacked) is EXEMPT by notification: 0-rated lines never
    // enter B2B/B2CL/B2CS — they file as Exempted here. Nil/non-GST stay 0.
    double exempBuckets[4] = {0.0, 0.0, 0.0, 0.0};
    double exemptTotal = 0.0;
    int exemptLines = 0;
    auto addExempt = [&](bool registered, bool intra, double amt) {
        int idx = registered ? (intra ? 1 : 0) : (intra ? 3 : 2);
        exempBuckets[idx] += amt;
        exemptTotal += amt;
        exemptLines++;
    };

    auto hsnAccumulate = [&](QMap<QString, Gstr1HsnItem>& map, const QVariantMap& r,
                             const QString& pos) {
        QString hsn = r.value("hsn_code").toString().trimmed().toUpper();
        if (hsn.isEmpty()) return false;
        double qty = r.value("quantity").toDouble();
        double itemAmt = r.value("amount").toDouble();
        double taxRate = r.value("tax_rate").toDouble();
        bool isIntra = (pos == supplierState);
        double igst = isIntra ? 0.0 : round2(itemAmt * (taxRate / 100.0));
        double cgst = isIntra ? round2(itemAmt * (taxRate / 200.0)) : 0.0;
        double sgst = isIntra ? round2(itemAmt * (taxRate / 200.0)) : 0.0;
        QString key = hsn + "|" + QString::number(taxRate, 'g', 12);
        if (!map.contains(key)) {
            Gstr1HsnItem hi;
            hi.serialNo = map.size() + 1;
            hi.hsnCode = hsn;
            hi.rate = taxRate;
            hi.description = r.value("item_name").toString().trimmed();
            hi.uqc = "QTL";
            map.insert(key, hi);
        }
        map[key].totalQty += qty;
        map[key].totalValue += (itemAmt + igst + cgst + sgst);
        map[key].taxableValue += itemAmt;
        map[key].igst += igst;
        map[key].cgst += cgst;
        map[key].sgst += sgst;
        return true;
    };

    for (const QString& invNo : invOrder) {
        const TempInv& ti = invMap[invNo];
        bool looksRegistered = !ti.partyGstin.isEmpty();
        bool validGstin = looksRegistered && isValidGstin(ti.partyGstin);
        if (looksRegistered && !validGstin) {
            payload.quarantine.append(
                QString("B2B %1: receiver GSTIN '%2' is invalid — fix the ledger GSTIN.").arg(invNo, ti.partyGstin));
            continue;
        }
        if (!isValidInvoiceNo(invNo)) {
            payload.quarantine.append(
                QString("%1: bill number must be 1-16 chars of A-Z 0-9 / -.").arg(invNo));
            continue;
        }
        bool badRate = false, badHsn = false;
        for (const auto& it : ti.items) {
            if (!isValidGstRate(it.value("tax_rate").toDouble())) badRate = true;
            if (it.value("hsn_code").toString().trimmed().isEmpty()) badHsn = true;
        }
        if (badRate) {
            payload.quarantine.append(QString("%1: GST rate is not a notified rate (0/0.1/0.25/1/1.5/3/5/6/7.5/12/18/28/40).").arg(invNo));
            continue;
        }
        if (badHsn) {
            payload.quarantine.append(QString("%1: HSN code is missing — HSN is mandatory on the return.").arg(invNo));
            continue;
        }

        bool isRegistered = validGstin;
        bool isIntra = (ti.pos == supplierState);

        // Partition lines: 0-rated (exempt rice) -> Table 8; rated lines file
        // normally. Per-item tax is the single source of truth.
        struct T { double rate, amt, igst, cgst, sgst; };
        QList<T> taxed;       // rated lines only
        QList<QVariantMap> taxedItems;
        double exemptAmt = 0.0;
        double taxableSum = 0.0, igstSum = 0.0, cgstSum = 0.0, sgstSum = 0.0;
        for (const auto& it : ti.items) {
            double rate = it.value("tax_rate").toDouble();
            double amt = it.value("amount").toDouble();
            if (std::fabs(rate) < 1e-9) {
                addExempt(isRegistered, isIntra, amt);
                exemptAmt += amt;
                continue;
            }
            T t;
            t.rate = rate;
            t.amt = amt;
            if (isIntra) {
                t.cgst = round2(t.amt * (t.rate / 200.0));
                t.sgst = round2(t.amt * (t.rate / 200.0));
                t.igst = 0.0;
            } else {
                t.igst = round2(t.amt * (t.rate / 100.0));
                t.cgst = 0.0;
                t.sgst = 0.0;
            }
            taxed.append(t);
            taxedItems.append(it);
            taxableSum += t.amt;
            igstSum += t.igst;
            cgstSum += t.cgst;
            sgstSum += t.sgst;
        }
        double ratedVal = round2(taxableSum + igstSum + cgstSum + sgstSum);
        double fullVal = round2(ratedVal + exemptAmt);
        payload.grossTurnover += fullVal;
        invSeries.append(invNo); // document issued, whatever its tax mix
        // Exempt lines still enter Table 12 (HSN covers all outward supply).
        for (const auto& it : ti.items) {
            if (std::fabs(it.value("tax_rate").toDouble()) < 1e-9)
                hsnAccumulate(isRegistered ? hsnB2BMap : hsnB2CMap, it, ti.pos);
        }
        if (taxed.isEmpty()) continue; // fully exempt: Table 8 only (+HSN/docs above)

        if (isRegistered) {
            Gstr1B2BInvoice b;
            b.ctin = ti.partyGstin;
            b.receiverName = ti.partyName;
            b.invoiceNo = invNo;
            b.invoiceDate = ti.invDate;
            b.invoiceValue = ratedVal;
            b.pos = ti.pos;
            for (const T& t : taxed) {
                Gstr1B2BItem bi;
                bi.rate = t.rate;
                bi.taxableValue = t.amt;
                bi.igst = t.igst;
                bi.cgst = t.cgst;
                bi.sgst = t.sgst;
                b.items.append(bi);
            }
            payload.b2b.append(b);
            for (const auto& it : taxedItems) hsnAccumulate(hsnB2BMap, it, ti.pos);
        } else if (!isIntra && fullVal > 250000.0) {
            // Table 5: B2CL — inter-state B2C above Rs 2.5 lakh, IGST only.
            for (int i = 0; i < taxed.size(); i++) {
                Gstr1B2CLInvoice c;
                c.invoiceNo = invNo;
                c.invoiceDate = ti.invDate;
                // Full invoice value on every line (the tool dedupes by
                // invoice number for its summary, same as Tally's export).
                c.invoiceValue = ratedVal;
                c.pos = ti.pos;
                c.rate = taxed[i].rate;
                c.taxableValue = taxed[i].amt;
                c.igst = taxed[i].igst;
                payload.b2cl.append(c);
                hsnAccumulate(hsnB2CMap, taxedItems[i], ti.pos);
            }
        } else {
            // Table 7: B2CS — aggregated by POS + rate + supply type.
            QString splyTy = isIntra ? "INTRA" : "INTER";
            for (int i = 0; i < taxed.size(); i++) {
                QString key = QString("%1_%2_%3").arg(ti.pos).arg(taxed[i].rate).arg(splyTy);
                if (!b2csAgg.contains(key)) {
                    Gstr1B2CSSummary cs;
                    cs.splyTy = splyTy;
                    cs.typ = "OE";
                    cs.pos = ti.pos;
                    cs.rate = taxed[i].rate;
                    b2csAgg.insert(key, cs);
                }
                b2csAgg[key].taxableValue += taxed[i].amt;
                b2csAgg[key].igst += taxed[i].igst;
                b2csAgg[key].cgst += taxed[i].cgst;
                b2csAgg[key].sgst += taxed[i].sgst;
                hsnAccumulate(hsnB2CMap, taxedItems[i], ti.pos);
            }
        }
    }

    // 2. Debit / credit notes in range (Table 9B).
    QVariantList noteRows = DatabaseManager::instance().executeQuery(
        "SELECT n.note_type, n.note_no, n.note_date, n.party_name, n.party_gstin, "
        "  n.state_code, n.is_interstate, n.reason_code, n.original_invoice_no, n.original_invoice_date, "
        "  COALESCE(i.item_name, n.item_name, 'Commodity') AS item_name, "
        "  COALESCE(i.hsn_code, n.hsn_code, '') AS hsn_code, "
        "  COALESCE(i.weight_qtl, n.total_weight_qtl, 0.0) AS quantity, "
        "  COALESCE(i.gst_pct, n.gst_pct, 0.0) AS tax_rate, "
        "  COALESCE(i.taxable_amount, n.taxable_amount, 0.0) AS amount "
        "FROM debit_credit_notes n "
        "LEFT JOIN debit_credit_note_items i ON i.note_id = n.id "
        "WHERE n.note_date >= '" + fromStr + "' AND n.note_date <= '" + toStr + "' "
        "ORDER BY n.note_date ASC, n.id ASC;");

    struct TempNote {
        QString kind; // "C" or "D"
        QString noteNo;
        QDate noteDate;
        QString partyName, partyGstin, pos;
        bool interstate = false;
        QString reason, origNo;
        QDate origDate;
        QList<QVariantMap> items;
    };
    QMap<QString, TempNote> noteMap;
    QList<QString> noteOrder;
    for (const auto& var : noteRows) {
        QVariantMap r = var.toMap();
        QString nNo = r.value("note_no").toString().trimmed();
        if (nNo.isEmpty()) continue;
        if (!noteMap.contains(nNo)) {
            TempNote tn;
            QString t = r.value("note_type").toString().trimmed().toLower();
            tn.kind = (t.startsWith("debit") || t == "d") ? "D" : "C";
            tn.noteNo = nNo;
            tn.noteDate = QDate::fromString(r.value("note_date").toString(), "yyyy-MM-dd");
            if (!tn.noteDate.isValid()) tn.noteDate = QDate::fromString(r.value("note_date").toString(), Qt::ISODate);
            tn.partyName = r.value("party_name").toString();
            tn.partyGstin = r.value("party_gstin").toString().trimmed().toUpper();
            QString p = GstTaxEngine::extractStateCodeFromGstin(tn.partyGstin);
            if (p.isEmpty()) p = r.value("state_code").toString().trimmed();
            if (p.isEmpty() && r.value("is_interstate").toInt() == 0) p = supplierState;
            tn.pos = p.rightJustified(2, '0');
            tn.interstate = r.value("is_interstate").toInt() != 0 || (!tn.pos.isEmpty() && tn.pos != supplierState);
            QString rsn = r.value("reason_code").toString().trimmed();
            int dash = rsn.indexOf('-');
            tn.reason = (dash >= 0) ? rsn.mid(dash + 1).trimmed() : rsn;
            if (tn.reason.isEmpty()) tn.reason = "Others";
            tn.origNo = r.value("original_invoice_no").toString().trimmed();
            tn.origDate = QDate::fromString(r.value("original_invoice_date").toString(), "yyyy-MM-dd");
            noteMap.insert(nNo, tn);
            noteOrder.append(nNo);
        }
        noteMap[nNo].items.append(r);
    }

    for (const QString& nNo : noteOrder) {
        const TempNote& tn = noteMap[nNo];
        bool looksRegistered = !tn.partyGstin.isEmpty();
        if (looksRegistered && !isValidGstin(tn.partyGstin)) {
            payload.quarantine.append(
                QString("Note %1: receiver GSTIN '%2' is invalid.").arg(nNo, tn.partyGstin));
            continue;
        }
        if (!isValidInvoiceNo(nNo)) {
            payload.quarantine.append(QString("Note %1: number must be 1-16 chars of A-Z 0-9 / -.").arg(nNo));
            continue;
        }
        bool badRate = false, badHsn = false;
        for (const auto& it : tn.items) {
            if (!isValidGstRate(it.value("tax_rate").toDouble())) badRate = true;
            if (it.value("hsn_code").toString().trimmed().isEmpty()) badHsn = true;
        }
        if (badRate) {
            payload.quarantine.append(QString("Note %1: GST rate is not a notified rate.").arg(nNo));
            continue;
        }
        if (badHsn) {
            payload.quarantine.append(QString("Note %1: HSN code is missing.").arg(nNo));
            continue;
        }

        bool isIntra = !tn.interstate && tn.pos == supplierState;
        bool isRegistered = looksRegistered && isValidGstin(tn.partyGstin);
        double taxableSum = 0.0, igstSum = 0.0, cgstSum = 0.0, sgstSum = 0.0;
        struct NT { double rate, amt, igst, cgst, sgst; };
        QList<NT> taxed;
        QList<QVariantMap> taxedItemMaps;
        for (const auto& it : tn.items) {
            double rate = it.value("tax_rate").toDouble();
            double amt = it.value("amount").toDouble();
            if (std::fabs(rate) < 1e-9) {
                addExempt(isRegistered, isIntra, amt); // exempt note lines -> Table 8
                continue;
            }
            taxedItemMaps.append(it);
            NT t;
            t.rate = rate;
            t.amt = amt;
            if (isIntra) {
                t.cgst = round2(t.amt * (t.rate / 200.0));
                t.sgst = round2(t.amt * (t.rate / 200.0));
                t.igst = 0.0;
            } else {
                t.igst = round2(t.amt * (t.rate / 100.0));
                t.cgst = 0.0;
                t.sgst = 0.0;
            }
            taxed.append(t);
            taxableSum += t.amt;
            igstSum += t.igst;
            cgstSum += t.cgst;
            sgstSum += t.sgst;
        }
        double noteVal = round2(taxableSum + igstSum + cgstSum + sgstSum);
        if (tn.kind == "C") creditSeries.append(nNo); else debitSeries.append(nNo);
        if (taxed.isEmpty()) continue; // fully exempt note: Table 8 only

        if (isRegistered) {
            Gstr1CdnrNote n;
            n.ctin = tn.partyGstin;
            n.receiverName = tn.partyName;
            n.noteType = tn.kind;
            n.noteNo = nNo;
            n.noteDate = tn.noteDate;
            n.reason = tn.reason;
            n.pos = tn.pos.isEmpty() ? supplierState : tn.pos;
            n.origInvNo = tn.origNo;
            n.origInvDate = tn.origDate;
            n.noteValue = noteVal;
            for (const NT& t : taxed) {
                Gstr1CdnItem ci;
                ci.rate = t.rate;
                ci.taxableValue = t.amt;
                ci.igst = t.igst;
                ci.cgst = t.cgst;
                ci.sgst = t.sgst;
                n.items.append(ci);
            }
            payload.cdnr.append(n);
            for (const auto& it : taxedItemMaps) hsnAccumulate(hsnB2BMap, it, n.pos);
            for (const auto& it : tn.items) {
                if (std::fabs(it.value("tax_rate").toDouble()) < 1e-9)
                    hsnAccumulate(hsnB2BMap, it, n.pos);
            }
        } else if (tn.interstate) {
            // CDNUR only covers B2CL-kind originals (inter-state unregistered).
            Gstr1CdnurNote n;
            n.urType = "B2CL";
            n.noteType = tn.kind;
            n.noteNo = nNo;
            n.noteDate = tn.noteDate;
            n.pos = tn.pos.isEmpty() ? supplierState : tn.pos;
            n.noteValue = noteVal;
            for (const NT& t : taxed) {
                Gstr1CdnItem ci;
                ci.rate = t.rate;
                ci.taxableValue = t.amt;
                ci.igst = t.igst;
                ci.cgst = t.cgst;
                ci.sgst = t.sgst;
                n.items.append(ci);
            }
            payload.cdnur.append(n);
            for (const auto& it : taxedItemMaps) hsnAccumulate(hsnB2CMap, it, n.pos);
            for (const auto& it : tn.items) {
                if (std::fabs(it.value("tax_rate").toDouble()) < 1e-9)
                    hsnAccumulate(hsnB2CMap, it, n.pos);
            }
        } else {
            // Intra-state unregistered note: no CDNUR bucket exists for it —
            // adjust the B2CS aggregate by hand, do not misfile it.
            payload.quarantine.append(
                QString("Note %1: intra-state unregistered note has no CDNUR bucket — adjust B2CS manually.").arg(nNo));
            continue;
        }
    }

    for (const auto& cs : b2csAgg) payload.b2cs.append(cs);
    for (const auto& hi : hsnB2BMap) payload.hsnB2B.append(hi);
    for (const auto& hi : hsnB2CMap) payload.hsnB2C.append(hi);

    // 3. Table 8 (nil / exempt / non-GST): 0-rated lines file as Exempted
    // (unbranded rice etc., exempt by notification). Nil / non-GST stay 0.
    static const QStringList kExempTtyps = {
        "Inter-State supplies to registered persons",
        "Intra-State supplies to registered persons",
        "Inter-State supplies to unregistered persons",
        "Intra-State supplies to unregistered persons",
    };
    for (int i = 0; i < 4; i++) {
        Gstr1ExempRow er;
        er.ttyp = kExempTtyps[i];
        er.exmpAmt = round2(exempBuckets[i]);
        payload.exemp.append(er);
    }
    if (exemptLines > 0) {
        payload.infoNotes.append(
            QString("%1 exempt (0-rated) line(s) totaling %2 filed as Exempted in Table 8, not in B2B/B2CS. "
                    "Confirm no taxable item is billed at 0%.")
                .arg(exemptLines)
                .arg(AccountingEngine::formatIndianCurrency(round2(exemptTotal))));
    }

    // 4. Table 13 document register (numeric-aware series ranges).
    auto addDocRow = [&](int docNum, const QString& name, const QStringList& series) {
        if (series.isEmpty()) return;
        QString from = series.first(), to = series.first();
        QPair<long long, QString> lo = seriesKey(from), hi = lo;
        for (const QString& s : series) {
            QPair<long long, QString> k = seriesKey(s);
            if (k < lo) { lo = k; from = s; }
            if (hi < k) { hi = k; to = s; }
        }
        Gstr1DocSummary d;
        d.docType = docNum;
        d.docName = name;
        d.fromSerial = from;
        d.toSerial = to;
        d.totalCount = series.size();
        d.cancelledCount = 0; // no void/cancel concept in this app
        d.netIssuedCount = series.size();
        payload.docs.append(d);
    };
    addDocRow(1, "Invoices for outward supply", invSeries);
    addDocRow(4, "Debit Note", debitSeries);
    addDocRow(5, "Credit Note", creditSeries);

    return payload;
}

QJsonDocument Gstr1Engine::exportToGovtOfflineJson(const Gstr1ReturnPayload& payload) {
    QJsonObject root;
    root["gstin"] = payload.gstin;
    root["fp"] = payload.fp;
    root["gt"] = round2(payload.grossTurnover);
    root["cur_gt"] = round2(payload.grossTurnover);
    root["version"] = "GST3.0.4";

    auto itmsJson = [](const QList<Gstr1B2BItem>& items) {
        QJsonArray arr;
        int num = 1;
        for (const auto& itm : items) {
            QJsonObject d;
            d["rt"] = itm.rate;
            d["txval"] = round2(itm.taxableValue);
            d["iamt"] = round2(itm.igst);
            d["camt"] = round2(itm.cgst);
            d["samt"] = round2(itm.sgst);
            d["csamt"] = round2(itm.cess);
            QJsonObject o;
            o["num"] = num++;
            o["itm_det"] = d;
            arr.append(o);
        }
        return arr;
    };
    auto cdnItmsJson = [](const QList<Gstr1CdnItem>& items) {
        QJsonArray arr;
        int num = 1;
        for (const auto& itm : items) {
            QJsonObject d;
            d["rt"] = itm.rate;
            d["txval"] = round2(itm.taxableValue);
            d["iamt"] = round2(itm.igst);
            d["camt"] = round2(itm.cgst);
            d["samt"] = round2(itm.sgst);
            d["csamt"] = round2(itm.cess);
            QJsonObject o;
            o["num"] = num++;
            o["itm_det"] = d;
            arr.append(o);
        }
        return arr;
    };

    // 1. B2B grouped by receiver GSTIN.
    QMap<QString, QJsonArray> b2bByGstin;
    for (const auto& inv : payload.b2b) {
        QJsonObject o;
        o["inum"] = inv.invoiceNo;
        o["idt"] = inv.invoiceDate.toString("dd-MM-yyyy");
        o["val"] = round2(inv.invoiceValue);
        o["pos"] = inv.pos;
        o["rchrg"] = inv.reverseCharge;
        o["inv_typ"] = inv.invoiceType;
        o["itms"] = itmsJson(inv.items);
        b2bByGstin[inv.ctin].append(o);
    }
    QJsonArray b2bArray;
    for (auto it = b2bByGstin.begin(); it != b2bByGstin.end(); ++it) {
        QJsonObject c;
        c["ctin"] = it.key();
        c["inv"] = it.value();
        b2bArray.append(c);
    }
    root["b2b"] = b2bArray;

    // 2. B2CL grouped by POS (no ctin; IGST only). Lines regrouped per
    // invoice: one inv object with one itm per line.
    struct B2clInv {
        QDate dt;
        double val = 0.0;
        QList<Gstr1B2CLInvoice> lines;
    };
    QMap<QString, QMap<QString, B2clInv>> b2clPosInv; // pos -> inum -> lines
    for (const auto& c : payload.b2cl) {
        B2clInv& b = b2clPosInv[c.pos][c.invoiceNo];
        b.dt = c.invoiceDate;
        b.val = qMax(b.val, round2(c.invoiceValue));
        b.lines.append(c);
    }
    QMap<QString, QJsonArray> b2clByPos;
    for (auto pit = b2clPosInv.begin(); pit != b2clPosInv.end(); ++pit) {
        for (auto iit = pit.value().begin(); iit != pit.value().end(); ++iit) {
            QJsonArray itms;
            int num = 1;
            for (const auto& ln : iit.value().lines) {
                QJsonObject itmDet;
                itmDet["rt"] = ln.rate;
                itmDet["txval"] = round2(ln.taxableValue);
                itmDet["iamt"] = round2(ln.igst);
                itmDet["csamt"] = round2(ln.cess);
                QJsonObject itm;
                itm["num"] = num++;
                itm["itm_det"] = itmDet;
                itms.append(itm);
            }
            QJsonObject o;
            o["inum"] = iit.key();
            o["idt"] = iit.value().dt.toString("dd-MM-yyyy");
            o["val"] = iit.value().val;
            o["itms"] = itms;
            b2clByPos[pit.key()].append(o);
        }
    }
    QJsonArray b2clArray;
    for (auto it = b2clByPos.begin(); it != b2clByPos.end(); ++it) {
        QJsonObject g;
        g["pos"] = it.key();
        g["inv"] = it.value();
        b2clArray.append(g);
    }
    root["b2cl"] = b2clArray;

    // 3. B2CS aggregated rows.
    QJsonArray b2csArray;
    for (const auto& cs : payload.b2cs) {
        QJsonObject o;
        o["sply_ty"] = cs.splyTy;
        o["typ"] = cs.typ;
        o["pos"] = cs.pos;
        o["rt"] = cs.rate;
        o["txval"] = round2(cs.taxableValue);
        o["iamt"] = round2(cs.igst);
        o["camt"] = round2(cs.cgst);
        o["samt"] = round2(cs.sgst);
        o["csamt"] = round2(cs.cess);
        b2csArray.append(o);
    }
    root["b2cs"] = b2csArray;

    // 4. CDNR grouped by receiver GSTIN.
    QMap<QString, QJsonArray> cdnrByGstin;
    for (const auto& n : payload.cdnr) {
        QJsonObject o;
        o["ntty"] = n.noteType;
        o["nt_num"] = n.noteNo;
        o["nt_dt"] = n.noteDate.toString("dd-MM-yyyy");
        o["rsn"] = n.reason;
        o["p_gst"] = "N";
        o["inum"] = n.origInvNo;
        o["idt"] = n.origInvDate.isValid() ? n.origInvDate.toString("dd-MM-yyyy") : "";
        o["val"] = round2(n.noteValue);
        o["itms"] = cdnItmsJson(n.items);
        cdnrByGstin[n.ctin].append(o);
    }
    QJsonArray cdnrArray;
    for (auto it = cdnrByGstin.begin(); it != cdnrByGstin.end(); ++it) {
        QJsonObject c;
        c["ctin"] = it.key();
        c["nt"] = it.value();
        cdnrArray.append(c);
    }
    root["cdnr"] = cdnrArray;

    // 5. CDNUR (unregistered notes).
    QJsonArray cdnurArray;
    for (const auto& n : payload.cdnur) {
        QJsonObject o;
        o["typ"] = n.urType;
        o["ntty"] = n.noteType;
        o["nt_num"] = n.noteNo;
        o["nt_dt"] = n.noteDate.toString("dd-MM-yyyy");
        o["pos"] = n.pos;
        o["val"] = round2(n.noteValue);
        o["itms"] = cdnItmsJson(n.items);
        cdnurArray.append(o);
    }
    root["cdnur"] = cdnurArray;

    // 6. HSN Table 12, bifurcated B2B / B2C.
    auto hsnJson = [](const QList<Gstr1HsnItem>& list) {
        QJsonArray arr;
        int num = 1;
        for (const auto& h : list) {
            QJsonObject o;
            o["num"] = num++;
            o["hsn_sc"] = h.hsnCode;
            o["desc"] = h.description;
            o["uqc"] = h.uqc;
            o["qty"] = round2(h.totalQty);
            o["val"] = round2(h.totalValue);
            o["txval"] = round2(h.taxableValue);
            o["iamt"] = round2(h.igst);
            o["camt"] = round2(h.cgst);
            o["samt"] = round2(h.sgst);
            o["csamt"] = round2(h.cess);
            arr.append(o);
        }
        return arr;
    };
    QJsonObject hsnSection;
    hsnSection["hsn_b2b"] = hsnJson(payload.hsnB2B);
    hsnSection["hsn_b2c"] = hsnJson(payload.hsnB2C);
    root["hsn"] = hsnSection;

    // 7. Table 8 nil / exempt / non-GST.
    QJsonArray exempArray;
    for (const auto& e : payload.exemp) {
        QJsonObject o;
        o["ttyp"] = e.ttyp;
        o["nil_amt"] = round2(e.nilAmt);
        o["exmp_amt"] = round2(e.exmpAmt);
        o["ngsup_amt"] = round2(e.ngsupAmt);
        exempArray.append(o);
    }
    root["exemp"] = exempArray;

    // 8. Table 13 documents issued.
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
