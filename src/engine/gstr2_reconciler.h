#pragma once

#include <QString>
#include <QDate>
#include <QList>
#include <QVariantMap>

namespace MahadevERP {

enum class MatchStatus {
    Matched,        // Fully matched on GSTIN, Invoice No, and values within +/- 1.00
    ValueMismatch,  // GSTIN & InvNo match, but amounts differ
    NotInBooks,     // Invoice in Portal GSTR-2A, but missing in local purchase register
    NotInPortal     // In local purchase register, but supplier has not uploaded to GSTR-2A
};

struct Gstr2BookRecord {
    int voucherId = 0;
    QString supplierGstin;
    QString supplierName;
    QString invoiceNo;
    QString normalizedInvNo;
    QDate invoiceDate;
    double taxableValue = 0.0;
    double taxAmount = 0.0;
    double totalValue = 0.0;
};

struct Gstr2PortalRecord {
    QString supplierGstin;
    QString supplierName;
    QString invoiceNo;
    QString normalizedInvNo;
    QDate invoiceDate;
    double taxableValue = 0.0;
    double taxAmount = 0.0;
    double totalValue = 0.0;
    QString filingStatus = "Y"; // Filed by supplier
};

struct Gstr2ReconciledItem {
    MatchStatus status = MatchStatus::NotInPortal;
    QString statusText;
    
    // Books Data
    QString bookInvNo;
    QDate bookInvDate;
    double bookTaxable = 0.0;
    double bookTax = 0.0;
    
    // Portal Data
    QString portalGstin;
    QString portalSupplierName;
    QString portalInvNo;
    QDate portalInvDate;
    double portalTaxable = 0.0;
    double portalTax = 0.0;
    
    // Discrepancy
    double taxableDiff = 0.0;
    double taxDiff = 0.0;
};

struct Gstr2ReconciliationSummary {
    int totalBookRecords = 0;
    int totalPortalRecords = 0;
    int matchedCount = 0;
    int valueMismatchCount = 0;
    int notInBooksCount = 0;
    int notInPortalCount = 0;
    
    double matchedItcAmount = 0.0;
    double mismatchItcAmount = 0.0;
    double missingPortalItcAmount = 0.0;
    double unclaimedPortalItcAmount = 0.0;
    
    QList<Gstr2ReconciledItem> items;
};

class Gstr2Reconciler {
public:
    static QString normalizeInvoiceNumber(const QString& rawInvNo);
    
    static Gstr2ReconciliationSummary reconcile(
        const QList<Gstr2BookRecord>& bookRecords,
        const QList<Gstr2PortalRecord>& portalRecords,
        double tolerance = 1.0
    );
};

} // namespace MahadevERP
