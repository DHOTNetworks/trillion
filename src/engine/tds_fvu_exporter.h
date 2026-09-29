#pragma once

#include <QString>
#include <QList>
#include <QDate>
#include <QVariantMap>

namespace MahadevERP {

struct TdsDeductorInfo {
    QString tan;
    QString pan;
    QString deductorName;
    QString deductorType; // 'C' for Company, 'O' for Other/Firm/Proprietorship
    QString address1;
    QString address2;
    QString city;
    QString stateCode; // 2-digit e.g. "06"
    QString pinCode;
    QString email;
    QString phone;
    
    // Responsible Person
    QString respPersonName;
    QString respPersonDesig;
    QString respPersonPan;
    QString respPersonPhone;
    QString respPersonEmail;
};

struct TdsChallanEntry {
    int challanRecordNo = 1;
    QString bsrCode;
    QString challanDate; // YYYY-MM-DD or DD-MM-YYYY
    QString challanNo;
    QString minorHead;   // "200" (TDS Payable by Taxpayer) or "400" (Regular Assessment)
    double basicTax = 0.0;
    double surcharge = 0.0;
    double cess = 0.0;
    double interest = 0.0;
    double penalty = 0.0;
    double fee = 0.0;
    double totalChallanAmt = 0.0;
    QString chequeNo;
};

struct TdsDeducteeEntry {
    int deducteeRecordNo = 1;
    int linkedChallanRecordNo = 1;
    QString deducteeCode; // "01" for Company, "02" for Non-Company
    QString pan;
    QString deducteeName;
    QString sectionCode; // "194C", "194J", "194I", "194Q", "194H", "206C"
    QString paymentDate;
    double amountPaid = 0.0;
    double tdsRate = 0.0;
    double tdsAmount = 0.0;
    double surcharge = 0.0;
    double cess = 0.0;
    double totalTaxDeducted = 0.0;
    double totalTaxDeposited = 0.0;
    QString dateOfDeduction;
    QString reasonForNonDeduction; // "A", "B", "C", "Y" or empty
    QString certificateNo;
};

class TdsFvuExporter {
public:
    // Generate official pipe-delimited NSDL e-TDS FVU text format for Form 26Q
    static QString generateForm26Q(
        const QString& financialYear,
        const QString& quarter, // "Q1", "Q2", "Q3", "Q4"
        const TdsDeductorInfo& deductor,
        const QList<TdsChallanEntry>& challans,
        const QList<TdsDeducteeEntry>& deductees
    );

    // Generate Form 27Q (Non-resident deductions)
    static QString generateForm27Q(
        const QString& financialYear,
        const QString& quarter,
        const TdsDeductorInfo& deductor,
        const QList<TdsChallanEntry>& challans,
        const QList<TdsDeducteeEntry>& deductees
    );

    static QString sanitizeField(const QString& input, int maxLen = 0);
    static QString formatDateForFvu(const QString& rawDate);
};

} // namespace MahadevERP
