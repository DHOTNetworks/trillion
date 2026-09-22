#include "gstr2_reconciler.h"
#include <QSet>
#include <cmath>

namespace MahadevERP {

static double round2(double val) {
    return std::round(val * 100.0) / 100.0;
}

QString Gstr2Reconciler::normalizeInvoiceNumber(const QString& rawInvNo) {
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

    return result;
}

Gstr2ReconciliationSummary Gstr2Reconciler::reconcile(
    const QList<Gstr2BookRecord>& bookRecords,
    const QList<Gstr2PortalRecord>& portalRecords,
    double tolerance
) {
    Gstr2ReconciliationSummary summary;
    summary.totalBookRecords = bookRecords.size();
    summary.totalPortalRecords = portalRecords.size();

    QSet<int> matchedPortalIndices;

    for (const auto& book : bookRecords) {
        QString cleanBookInv = normalizeInvoiceNumber(book.invoiceNo);
        QString cleanBookGstin = book.supplierGstin.trimmed().toUpper();

        int bestPortalIdx = -1;
        bool exactMatch = false;

        for (int pIdx = 0; pIdx < portalRecords.size(); ++pIdx) {
            if (matchedPortalIndices.contains(pIdx)) continue;

            const auto& portal = portalRecords[pIdx];
            QString cleanPortalGstin = portal.supplierGstin.trimmed().toUpper();
            QString cleanPortalInv = normalizeInvoiceNumber(portal.invoiceNo);

            if (cleanBookGstin == cleanPortalGstin && cleanBookInv == cleanPortalInv) {
                bestPortalIdx = pIdx;
                double taxDiff = std::abs(book.taxAmount - portal.taxAmount);
                double taxableDiff = std::abs(book.taxableValue - portal.taxableValue);
                if (taxDiff <= tolerance && taxableDiff <= tolerance) {
                    exactMatch = true;
                    break;
                }
            }
        }

        Gstr2ReconciledItem item;
        item.bookInvNo = book.invoiceNo;
        item.bookInvDate = book.invoiceDate;
        item.bookTaxable = book.taxableValue;
        item.bookTax = book.taxAmount;

        if (bestPortalIdx >= 0) {
            matchedPortalIndices.insert(bestPortalIdx);
            const auto& portal = portalRecords[bestPortalIdx];
            item.portalGstin = portal.supplierGstin;
            item.portalSupplierName = portal.supplierName;
            item.portalInvNo = portal.invoiceNo;
            item.portalInvDate = portal.invoiceDate;
            item.portalTaxable = portal.taxableValue;
            item.portalTax = portal.taxAmount;

            item.taxableDiff = round2(book.taxableValue - portal.taxableValue);
            item.taxDiff = round2(book.taxAmount - portal.taxAmount);

            if (exactMatch) {
                item.status = MatchStatus::Matched;
                item.statusText = "MATCHED (ITC Verified)";
                summary.matchedCount++;
                summary.matchedItcAmount += book.taxAmount;
            } else {
                item.status = MatchStatus::ValueMismatch;
                item.statusText = "VALUE MISMATCH";
                summary.valueMismatchCount++;
                summary.mismatchItcAmount += book.taxAmount;
            }
        } else {
            item.status = MatchStatus::NotInPortal;
            item.statusText = "NOT IN PORTAL (Supplier Default)";
            item.taxDiff = book.taxAmount;
            item.taxableDiff = book.taxableValue;
            summary.notInPortalCount++;
            summary.missingPortalItcAmount += book.taxAmount;
        }

        summary.items.append(item);
    }

    // Process portal records not present in books
    for (int pIdx = 0; pIdx < portalRecords.size(); ++pIdx) {
        if (!matchedPortalIndices.contains(pIdx)) {
            const auto& portal = portalRecords[pIdx];
            Gstr2ReconciledItem item;
            item.status = MatchStatus::NotInBooks;
            item.statusText = "NOT IN BOOKS (Unclaimed ITC)";
            item.portalGstin = portal.supplierGstin;
            item.portalSupplierName = portal.supplierName;
            item.portalInvNo = portal.invoiceNo;
            item.portalInvDate = portal.invoiceDate;
            item.portalTaxable = portal.taxableValue;
            item.portalTax = portal.taxAmount;
            item.taxDiff = -portal.taxAmount;
            item.taxableDiff = -portal.taxableValue;

            summary.notInBooksCount++;
            summary.unclaimedPortalItcAmount += portal.taxAmount;
            summary.items.append(item);
        }
    }

    return summary;
}

} // namespace MahadevERP
