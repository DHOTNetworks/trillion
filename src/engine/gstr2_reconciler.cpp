#include "gstr2_reconciler.h"
#include "../database_manager.h"
#include <QRegularExpression>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QSet>
#include <cmath>

namespace MahadevERP {

static inline double round2(double val) {
    return std::round(val * 100.0) / 100.0;
}

QString Gstr2Reconciler::normalizeGstin(const QString& rawGstin) {
    return rawGstin.trimmed().toUpper();
}

QString Gstr2Reconciler::normalizeInvoiceNumber(const QString& rawInvNo) {
    if (rawInvNo.isEmpty()) return "";

    QString result;
    QString currentDigitChunk;

    for (const QChar& c : rawInvNo) {
        if (c.isDigit()) {
            currentDigitChunk.append(c);
        } else {
            if (!currentDigitChunk.isEmpty()) {
                // Strip leading zeros from current digit chunk
                int start = 0;
                while (start < currentDigitChunk.length() - 1 && currentDigitChunk[start] == '0') {
                    start++;
                }
                result.append(currentDigitChunk.mid(start));
                currentDigitChunk.clear();
            }
            if (c.isLetter()) {
                result.append(c.toUpper());
            }
        }
    }

    if (!currentDigitChunk.isEmpty()) {
        int start = 0;
        while (start < currentDigitChunk.length() - 1 && currentDigitChunk[start] == '0') {
            start++;
        }
        result.append(currentDigitChunk.mid(start));
    }

    return result.isEmpty() ? rawInvNo.trimmed().toUpper() : result;
}

QList<Gstr2PortalRecord> Gstr2Reconciler::parseGstr2BJson(const QByteArray& jsonData, const QString& returnPeriod) {
    QList<Gstr2PortalRecord> records;
    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(jsonData, &err);
    if (doc.isNull() || !doc.isObject()) return records;

    QJsonObject root = doc.object();
    QJsonObject dataObj = root.contains("data") ? root["data"].toObject() : root;
    QJsonObject docData = dataObj.contains("docdata") ? dataObj["docdata"].toObject() : dataObj;

    auto parseTaxItems = [](const QJsonArray& items, Gstr2PortalRecord& rec) {
        for (const auto& itemVal : items) {
            QJsonObject itm = itemVal.toObject();
            rec.taxableValue += itm["txval"].toDouble();
            rec.igst += itm["iamt"].toDouble();
            rec.cgst += itm["camt"].toDouble();
            rec.sgst += itm["samt"].toDouble();
            rec.cess += itm["csamt"].toDouble();
        }
        rec.taxAmount = round2(rec.igst + rec.cgst + rec.sgst + rec.cess);
        rec.taxableValue = round2(rec.taxableValue);
        rec.totalValue = round2(rec.taxableValue + rec.taxAmount);
    };

    // 1. Ingest B2B Invoices
    if (docData.contains("b2b") && docData["b2b"].isArray()) {
        for (const auto& suppVal : docData["b2b"].toArray()) {
            QJsonObject supp = suppVal.toObject();
            QString gstin = normalizeGstin(supp["ctin"].toString());
            QString tradeName = supp["trdnm"].toString();

            for (const auto& invVal : supp["inv"].toArray()) {
                QJsonObject inv = invVal.toObject();
                Gstr2PortalRecord rec;
                rec.section = "B2B";
                rec.supplierGstin = gstin;
                rec.supplierName = tradeName;
                rec.invoiceNo = inv["inum"].toString();
                rec.normalizedInvNo = normalizeInvoiceNumber(rec.invoiceNo);
                rec.invoiceDate = QDate::fromString(inv["dt"].toString(), "dd-MM-yyyy");
                if (!rec.invoiceDate.isValid()) {
                    rec.invoiceDate = QDate::fromString(inv["dt"].toString(), "yyyy-MM-dd");
                }
                rec.invoiceType = inv["typ"].toString("R");
                rec.returnPeriod = returnPeriod.isEmpty() ? root["fp"].toString() : returnPeriod;
                rec.itcEligibility = inv["itcavl"].toString("Y");
                
                parseTaxItems(inv["items"].toArray(), rec);
                records.append(rec);
            }
        }
    }

    // 2. Ingest CDNR (Credit / Debit Notes)
    if (docData.contains("cdnr") && docData["cdnr"].isArray()) {
        for (const auto& suppVal : docData["cdnr"].toArray()) {
            QJsonObject supp = suppVal.toObject();
            QString gstin = normalizeGstin(supp["ctin"].toString());
            QString tradeName = supp["trdnm"].toString();

            for (const auto& ntVal : supp["nt"].toArray()) {
                QJsonObject nt = ntVal.toObject();
                Gstr2PortalRecord rec;
                rec.section = "CDNR";
                rec.supplierGstin = gstin;
                rec.supplierName = tradeName;
                rec.invoiceNo = nt["nt_num"].toString();
                rec.normalizedInvNo = normalizeInvoiceNumber(rec.invoiceNo);
                rec.invoiceDate = QDate::fromString(nt["nt_dt"].toString(), "dd-MM-yyyy");
                rec.invoiceType = nt["ntty"].toString();
                rec.returnPeriod = returnPeriod.isEmpty() ? root["fp"].toString() : returnPeriod;
                rec.itcEligibility = nt["itcavl"].toString("Y");

                parseTaxItems(nt["items"].toArray(), rec);
                records.append(rec);
            }
        }
    }

    return records;
}

Gstr2ReconciliationSummary Gstr2Reconciler::reconcile(
    const QList<Gstr2BookRecord>& bookRecords,
    const QList<Gstr2PortalRecord>& portalRecords,
    double tolerance,
    int dateToleranceDays
) {
    Gstr2ReconciliationSummary summary;
    summary.totalBookRecords = bookRecords.size();
    summary.totalPortalRecords = portalRecords.size();

    QSet<int> matchedPortalIndices;
    QSet<int> matchedBookIndices;

    auto isClose = [](double a, double b, double tol) {
        return std::abs(a - b) <= tol;
    };

    // -------------------------------------------------------------
    // PASS 1: Exact Match (GSTIN + Inv# + Date + Tax)
    // -------------------------------------------------------------
    for (int bIdx = 0; bIdx < bookRecords.size(); ++bIdx) {
        if (matchedBookIndices.contains(bIdx)) continue;
        const auto& book = bookRecords[bIdx];
        QString cleanBookInv = normalizeInvoiceNumber(book.invoiceNo);
        QString cleanBookGstin = normalizeGstin(book.supplierGstin);

        for (int pIdx = 0; pIdx < portalRecords.size(); ++pIdx) {
            if (matchedPortalIndices.contains(pIdx)) continue;
            const auto& portal = portalRecords[pIdx];
            QString cleanPortalGstin = normalizeGstin(portal.supplierGstin);
            QString cleanPortalInv = normalizeInvoiceNumber(portal.invoiceNo);

            if (cleanBookGstin == cleanPortalGstin && cleanBookInv == cleanPortalInv) {
                double taxDiff = std::abs(book.taxAmount - portal.taxAmount);
                double taxableDiff = std::abs(book.taxableValue - portal.taxableValue);

                if (taxDiff == 0.0 && taxableDiff == 0.0 && book.invoiceDate == portal.invoiceDate) {
                    Gstr2ReconciledItem item;
                    item.status = MatchStatus::Matched;
                    item.statusText = "MATCHED (ITC Verified)";
                    item.discrepancyRemarks = "100% Exact Match";
                    
                    item.bookVoucherId = book.voucherId;
                    item.bookGstin = book.supplierGstin;
                    item.bookSupplierName = book.supplierName;
                    item.bookInvNo = book.invoiceNo;
                    item.bookInvDate = book.invoiceDate;
                    item.bookTaxable = book.taxableValue;
                    item.bookTax = book.taxAmount;
                    item.bookTotal = book.totalValue;

                    item.portalGstin = portal.supplierGstin;
                    item.portalSupplierName = portal.supplierName;
                    item.portalInvNo = portal.invoiceNo;
                    item.portalInvDate = portal.invoiceDate;
                    item.portalTaxable = portal.taxableValue;
                    item.portalTax = portal.taxAmount;
                    item.portalTotal = portal.totalValue;
                    item.portalSection = portal.section;
                    item.itcEligibility = portal.itcEligibility;

                    matchedPortalIndices.insert(pIdx);
                    matchedBookIndices.insert(bIdx);
                    summary.items.append(item);
                    summary.matchedCount++;
                    summary.exactMatchCount++;
                    summary.matchedItcAmount += book.taxAmount;
                    break;
                }
            }
        }
    }

    // -------------------------------------------------------------
    // PASS 2: Rounding / Tolerance Match (|Δ| <= tolerance)
    // -------------------------------------------------------------
    for (int bIdx = 0; bIdx < bookRecords.size(); ++bIdx) {
        if (matchedBookIndices.contains(bIdx)) continue;
        const auto& book = bookRecords[bIdx];
        QString cleanBookInv = normalizeInvoiceNumber(book.invoiceNo);
        QString cleanBookGstin = normalizeGstin(book.supplierGstin);

        for (int pIdx = 0; pIdx < portalRecords.size(); ++pIdx) {
            if (matchedPortalIndices.contains(pIdx)) continue;
            const auto& portal = portalRecords[pIdx];
            QString cleanPortalGstin = normalizeGstin(portal.supplierGstin);
            QString cleanPortalInv = normalizeInvoiceNumber(portal.invoiceNo);

            if (cleanBookGstin == cleanPortalGstin && cleanBookInv == cleanPortalInv) {
                double taxDiff = std::abs(book.taxAmount - portal.taxAmount);
                double taxableDiff = std::abs(book.taxableValue - portal.taxableValue);

                if (taxDiff <= tolerance && taxableDiff <= tolerance) {
                    Gstr2ReconciledItem item;
                    item.status = MatchStatus::Matched;
                    item.statusText = "MATCHED (Rounding Tolerance)";
                    item.discrepancyRemarks = QString("Rounding difference: Tax Δ=₹%1").arg(round2(book.taxAmount - portal.taxAmount));

                    item.bookVoucherId = book.voucherId;
                    item.bookGstin = book.supplierGstin;
                    item.bookSupplierName = book.supplierName;
                    item.bookInvNo = book.invoiceNo;
                    item.bookInvDate = book.invoiceDate;
                    item.bookTaxable = book.taxableValue;
                    item.bookTax = book.taxAmount;
                    item.bookTotal = book.totalValue;

                    item.portalGstin = portal.supplierGstin;
                    item.portalSupplierName = portal.supplierName;
                    item.portalInvNo = portal.invoiceNo;
                    item.portalInvDate = portal.invoiceDate;
                    item.portalTaxable = portal.taxableValue;
                    item.portalTax = portal.taxAmount;
                    item.portalTotal = portal.totalValue;
                    item.portalSection = portal.section;
                    item.itcEligibility = portal.itcEligibility;
                    item.taxableDiff = round2(book.taxableValue - portal.taxableValue);
                    item.taxDiff = round2(book.taxAmount - portal.taxAmount);

                    matchedPortalIndices.insert(pIdx);
                    matchedBookIndices.insert(bIdx);
                    summary.items.append(item);
                    summary.matchedCount++;
                    summary.roundingMatchCount++;
                    summary.matchedItcAmount += book.taxAmount;
                    break;
                }
            }
        }
    }

    // -------------------------------------------------------------
    // PASS 3: Tax / Value Mismatch (GSTIN + Inv# Match, Tax differs)
    // -------------------------------------------------------------
    for (int bIdx = 0; bIdx < bookRecords.size(); ++bIdx) {
        if (matchedBookIndices.contains(bIdx)) continue;
        const auto& book = bookRecords[bIdx];
        QString cleanBookInv = normalizeInvoiceNumber(book.invoiceNo);
        QString cleanBookGstin = normalizeGstin(book.supplierGstin);

        for (int pIdx = 0; pIdx < portalRecords.size(); ++pIdx) {
            if (matchedPortalIndices.contains(pIdx)) continue;
            const auto& portal = portalRecords[pIdx];
            QString cleanPortalGstin = normalizeGstin(portal.supplierGstin);
            QString cleanPortalInv = normalizeInvoiceNumber(portal.invoiceNo);

            if (cleanBookGstin == cleanPortalGstin && cleanBookInv == cleanPortalInv) {
                Gstr2ReconciledItem item;
                item.status = MatchStatus::ValueMismatch;
                item.statusText = "VALUE MISMATCH";
                item.taxableDiff = round2(book.taxableValue - portal.taxableValue);
                item.taxDiff = round2(book.taxAmount - portal.taxAmount);
                item.discrepancyRemarks = QString("Value Mismatch: Taxable Δ=₹%1, Tax Δ=₹%2").arg(item.taxableDiff).arg(item.taxDiff);

                item.bookVoucherId = book.voucherId;
                item.bookGstin = book.supplierGstin;
                item.bookSupplierName = book.supplierName;
                item.bookInvNo = book.invoiceNo;
                item.bookInvDate = book.invoiceDate;
                item.bookTaxable = book.taxableValue;
                item.bookTax = book.taxAmount;
                item.bookTotal = book.totalValue;

                item.portalGstin = portal.supplierGstin;
                item.portalSupplierName = portal.supplierName;
                item.portalInvNo = portal.invoiceNo;
                item.portalInvDate = portal.invoiceDate;
                item.portalTaxable = portal.taxableValue;
                item.portalTax = portal.taxAmount;
                item.portalTotal = portal.totalValue;
                item.portalSection = portal.section;
                item.itcEligibility = portal.itcEligibility;

                matchedPortalIndices.insert(pIdx);
                matchedBookIndices.insert(bIdx);
                summary.items.append(item);
                summary.valueMismatchCount++;
                summary.mismatchItcAmount += book.taxAmount;
                break;
            }
        }
    }

    // -------------------------------------------------------------
    // PASS 4: Probable Fuzzy Match (GSTIN + Amount match, typo in Inv#)
    // -------------------------------------------------------------
    for (int bIdx = 0; bIdx < bookRecords.size(); ++bIdx) {
        if (matchedBookIndices.contains(bIdx)) continue;
        const auto& book = bookRecords[bIdx];
        QString cleanBookGstin = normalizeGstin(book.supplierGstin);

        for (int pIdx = 0; pIdx < portalRecords.size(); ++pIdx) {
            if (matchedPortalIndices.contains(pIdx)) continue;
            const auto& portal = portalRecords[pIdx];
            QString cleanPortalGstin = normalizeGstin(portal.supplierGstin);

            if (cleanBookGstin == cleanPortalGstin && isClose(book.taxAmount, portal.taxAmount, 0.01)) {
                if (book.invoiceDate.isValid() && portal.invoiceDate.isValid() &&
                    std::abs(book.invoiceDate.daysTo(portal.invoiceDate)) <= dateToleranceDays) 
                {
                    Gstr2ReconciledItem item;
                    item.status = MatchStatus::ProbableFuzzyMatch;
                    item.statusText = "PROBABLE MATCH (Typo in Inv#)";
                    item.discrepancyRemarks = QString("Books Inv# '%1' vs Portal Inv# '%2'").arg(book.invoiceNo).arg(portal.invoiceNo);

                    item.bookVoucherId = book.voucherId;
                    item.bookGstin = book.supplierGstin;
                    item.bookSupplierName = book.supplierName;
                    item.bookInvNo = book.invoiceNo;
                    item.bookInvDate = book.invoiceDate;
                    item.bookTaxable = book.taxableValue;
                    item.bookTax = book.taxAmount;
                    item.bookTotal = book.totalValue;

                    item.portalGstin = portal.supplierGstin;
                    item.portalSupplierName = portal.supplierName;
                    item.portalInvNo = portal.invoiceNo;
                    item.portalInvDate = portal.invoiceDate;
                    item.portalTaxable = portal.taxableValue;
                    item.portalTax = portal.taxAmount;
                    item.portalTotal = portal.totalValue;
                    item.portalSection = portal.section;
                    item.itcEligibility = portal.itcEligibility;

                    matchedPortalIndices.insert(pIdx);
                    matchedBookIndices.insert(bIdx);
                    summary.items.append(item);
                    summary.probableMatchCount++;
                    break;
                }
            }
        }
    }

    // -------------------------------------------------------------
    // Unmatched: Missing in Portal (Supplier Default)
    // -------------------------------------------------------------
    for (int bIdx = 0; bIdx < bookRecords.size(); ++bIdx) {
        if (!matchedBookIndices.contains(bIdx)) {
            const auto& book = bookRecords[bIdx];
            Gstr2ReconciledItem item;
            item.status = MatchStatus::NotInPortal;
            item.statusText = "NOT IN PORTAL (Supplier Default)";
            item.discrepancyRemarks = "Supplier has not uploaded invoice in GSTR-1";

            item.bookVoucherId = book.voucherId;
            item.bookGstin = book.supplierGstin;
            item.bookSupplierName = book.supplierName;
            item.bookInvNo = book.invoiceNo;
            item.bookInvDate = book.invoiceDate;
            item.bookTaxable = book.taxableValue;
            item.bookTax = book.taxAmount;
            item.bookTotal = book.totalValue;
            item.taxDiff = book.taxAmount;
            item.taxableDiff = book.taxableValue;

            summary.items.append(item);
            summary.notInPortalCount++;
            summary.missingPortalItcAmount += book.taxAmount;
        }
    }

    // -------------------------------------------------------------
    // Unmatched: Missing in Books (Unclaimed ITC)
    // -------------------------------------------------------------
    for (int pIdx = 0; pIdx < portalRecords.size(); ++pIdx) {
        if (!matchedPortalIndices.contains(pIdx)) {
            const auto& portal = portalRecords[pIdx];
            Gstr2ReconciledItem item;
            item.status = MatchStatus::NotInBooks;
            item.statusText = "NOT IN BOOKS (Unclaimed ITC)";
            item.discrepancyRemarks = "Invoice present in 2B, but missing in purchase register";

            item.portalGstin = portal.supplierGstin;
            item.portalSupplierName = portal.supplierName;
            item.portalInvNo = portal.invoiceNo;
            item.portalInvDate = portal.invoiceDate;
            item.portalTaxable = portal.taxableValue;
            item.portalTax = portal.taxAmount;
            item.portalTotal = portal.totalValue;
            item.portalSection = portal.section;
            item.itcEligibility = portal.itcEligibility;
            item.taxDiff = -portal.taxAmount;
            item.taxableDiff = -portal.taxableValue;

            summary.items.append(item);
            summary.notInBooksCount++;
            summary.unclaimedPortalItcAmount += portal.taxAmount;
        }
    }

    return summary;
}

QList<Gstr2BookRecord> Gstr2Reconciler::loadBookPurchasesFromDb(const QDate& fromDate, const QDate& toDate) {
    QList<Gstr2BookRecord> records;
    DatabaseManager& db = DatabaseManager::instance();

    QString sql = R"(
        SELECT v.id AS voucher_id,
               v.voucher_no,
               v.voucher_date,
               v.instrument_no AS bill_no,
               v.instrument_date AS bill_date,
               l.name AS party_name,
               l.gstin AS party_gstin,
               COALESCE(v.taxable_amount, 0.0) AS taxable_val,
               COALESCE(v.cgst_amount, 0.0) AS cgst,
               COALESCE(v.sgst_amount, 0.0) AS sgst,
               COALESCE(v.igst_amount, 0.0) AS igst,
               COALESCE(v.cess_amount, 0.0) AS cess,
               COALESCE(v.amount, 0.0) AS total_val
        FROM vouchers v
        JOIN ledgers l ON (v.party_id = l.id OR v.ledger_id = l.id)
        WHERE v.voucher_type IN ('PURCHASE', 'PURCHASE_VOUCHER', 'DEBIT_NOTE')
          AND v.voucher_date >= ? AND v.voucher_date <= ?
          AND l.gstin IS NOT NULL AND length(trim(l.gstin)) = 15
    )";

    QVariantList params = { fromDate.toString("yyyy-MM-dd"), toDate.toString("yyyy-MM-dd") };
    QVariantList rows = db.executeQuery(sql, params);

    for (const auto& rowVal : rows) {
        QVariantMap row = rowVal.toMap();
        Gstr2BookRecord rec;
        rec.voucherId = row["voucher_id"].toInt();
        rec.supplierGstin = normalizeGstin(row["party_gstin"].toString());
        rec.supplierName = row["party_name"].toString();
        
        QString billNo = row["bill_no"].toString().trimmed();
        rec.invoiceNo = billNo.isEmpty() ? row["voucher_no"].toString() : billNo;
        rec.normalizedInvNo = normalizeInvoiceNumber(rec.invoiceNo);

        QString bDate = row["bill_date"].toString();
        rec.invoiceDate = bDate.isEmpty() ? QDate::fromString(row["voucher_date"].toString(), "yyyy-MM-dd")
                                          : QDate::fromString(bDate, "yyyy-MM-dd");

        rec.taxableValue = row["taxable_val"].toDouble();
        rec.cgst = row["cgst"].toDouble();
        rec.sgst = row["sgst"].toDouble();
        rec.igst = row["igst"].toDouble();
        rec.cess = row["cess"].toDouble();
        rec.taxAmount = round2(rec.cgst + rec.sgst + rec.igst + rec.cess);
        rec.totalValue = row["total_val"].toDouble();

        records.append(rec);
    }

    return records;
}

bool Gstr2Reconciler::saveReconciliationStatus(int voucherId, MatchStatus status, const QString& remarks) {
    DatabaseManager& db = DatabaseManager::instance();
    QString statusStr = "UNMATCHED";
    if (status == MatchStatus::Matched || status == MatchStatus::ExactMatch || status == MatchStatus::RoundingMatch) {
        statusStr = "MATCHED";
    } else if (status == MatchStatus::ValueMismatch || status == MatchStatus::TaxValueMismatch) {
        statusStr = "MISMATCH";
    }

    QString sql = "UPDATE vouchers SET narration = COALESCE(narration, '') || ' [GST: ' || ? || ']' WHERE id = ?";
    return db.executeNonQuery(sql, { statusStr + " " + remarks, voucherId });
}

} // namespace MahadevERP
