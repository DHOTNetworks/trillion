#include "tds_fvu_exporter.h"
#include <QStringList>
#include <QRegularExpression>
#include <cmath>

namespace MahadevERP {

QString TdsFvuExporter::sanitizeField(const QString& input, int maxLen) {
    QString res = input;
    res.remove('^');
    res.remove('|');
    res.remove('\r');
    res.remove('\n');
    res = res.trimmed();
    if (maxLen > 0 && res.length() > maxLen) {
        res = res.left(maxLen);
    }
    return res;
}

QString TdsFvuExporter::formatDateForFvu(const QString& rawDate) {
    if (rawDate.isEmpty()) return "";
    QString cleaned = rawDate.trimmed();
    QStringList parts = cleaned.split('-');
    if (parts.size() == 1) parts = cleaned.split('/');

    if (parts.size() == 3) {
        if (parts[0].length() == 4) {
            // YYYY-MM-DD -> DDMMYYYY
            return QString("%1%2%3").arg(parts[2].rightJustified(2, '0'), parts[1].rightJustified(2, '0'), parts[0]);
        } else {
            // DD-MM-YYYY -> DDMMYYYY
            return QString("%1%2%3").arg(parts[0].rightJustified(2, '0'), parts[1].rightJustified(2, '0'), parts[2]);
        }
    }
    return cleaned;
}

QString TdsFvuExporter::generateForm26Q(
    const QString& financialYear,
    const QString& quarter,
    const TdsDeductorInfo& deductor,
    const QList<TdsChallanEntry>& challans,
    const QList<TdsDeducteeEntry>& deductees
) {
    QStringList lines;

    double totalChallanAmt = 0.0;
    for (const auto& ch : challans) totalChallanAmt += ch.totalChallanAmt;

    double totalDeducteeTax = 0.0;
    for (const auto& dd : deductees) totalDeducteeTax += dd.totalTaxDeposited;

    // 1. File Header (FH) Record
    // Format: 1^FH^SL1^Quarter^TAN^TotalBatchCount^FileHash^...
    QStringList fh;
    fh << "1"                                    // Line Number
       << "FH"                                   // Record Type
       << "NSDL-TDS-FVU-26Q"                     // File Type
       << "R"                                    // Upload Type: Regular
       << QDate::currentDate().toString("ddMMyyyy") // File Creation Date
       << "1"                                    // File Sequence No
       << "O"                                    // Deductor Category
       << sanitizeField(deductor.tan, 10)        // TAN of Deductor
       << "1"                                    // Total Batch Records
       << ""                                     // Hash/Filler
       << "";
    lines.append(fh.join('^'));

    // 2. Batch Header (BH) Record
    // Format: 2^BH^1^ChallanCount^FormNo^Quarter^FY^DeductorInfo...
    QString fyClean = financialYear;
    fyClean = fyClean.remove("FY ").remove("-").trimmed();

    QStringList bh;
    bh << "2"                                    // Line Number
       << "BH"                                   // Record Type
       << "1"                                    // Batch Number
       << QString::number(challans.size())       // Total Challans
       << "26Q"                                  // Form Number
       << ""                                     // Transaction Type: Nil / Regular
       << sanitizeField(deductor.tan, 10)        // TAN
       << sanitizeField(deductor.pan, 10)        // PAN
       << fyClean                                // Assessment / Financial Year
       << quarter.toUpper()                      // Quarter (Q1/Q2/Q3/Q4)
       << sanitizeField(deductor.deductorName, 75) // Deductor Name
       << sanitizeField(deductor.deductorType, 1)  // Deductor Type
       << sanitizeField(deductor.address1, 25)   // Address 1
       << sanitizeField(deductor.address2, 25)   // Address 2
       << sanitizeField(deductor.city, 25)       // City
       << sanitizeField(deductor.stateCode, 2)   // State Code
       << sanitizeField(deductor.pinCode, 6)     // Pin Code
       << sanitizeField(deductor.email, 50)      // Email
       << sanitizeField(deductor.phone, 15)      // Phone
       << sanitizeField(deductor.respPersonName, 75) // Responsible Person
       << sanitizeField(deductor.respPersonDesig, 20)
       << sanitizeField(deductor.respPersonPan, 10)
       << QString::number(totalChallanAmt, 'f', 2) // Total Challan Amount
       << QString::number(deductees.size())      // Total Deductee Records
       << QString::number(totalDeducteeTax, 'f', 2);
    lines.append(bh.join('^'));

    int lineNo = 3;

    // 3. Challan Details (CD) & Deductee Details (DD)
    int chIndex = 1;
    for (const auto& ch : challans) {
        QStringList cd;
        cd << QString::number(lineNo++)
           << "CD"
           << "1"                                // Batch No
           << QString::number(chIndex)           // Challan Record No
           << QString::number(deductees.size())  // Count of deductees under this challan
           << "N"                                // Nil Challan Indicator
           << sanitizeField(ch.bsrCode, 7)       // BSR Code
           << formatDateForFvu(ch.challanDate)   // Challan Date (DDMMYYYY)
           << sanitizeField(ch.challanNo, 5)     // Challan Serial No
           << (ch.minorHead.isEmpty() ? "200" : ch.minorHead) // Minor Head
           << QString::number(ch.basicTax, 'f', 2)
           << QString::number(ch.surcharge, 'f', 2)
           << QString::number(ch.cess, 'f', 2)
           << QString::number(ch.interest, 'f', 2)
           << QString::number(ch.penalty, 'f', 2)
           << QString::number(ch.fee, 'f', 2)
           << QString::number(ch.totalChallanAmt, 'f', 2)
           << sanitizeField(ch.chequeNo, 15)
           << "Y";                               // Book Entry / Paid
        lines.append(cd.join('^'));

        // Output Deductees associated with this Challan
        int ddIndex = 1;
        for (const auto& dd : deductees) {
            if (dd.linkedChallanRecordNo == ch.challanRecordNo || challans.size() == 1) {
                QStringList ddr;
                ddr << QString::number(lineNo++)
                    << "DD"
                    << "1"                       // Batch No
                    << QString::number(chIndex)  // Challan No
                    << QString::number(ddIndex++) // Deductee Serial No
                    << sanitizeField(dd.deducteeCode, 2) // "01" Company / "02" Non-Company
                    << sanitizeField(dd.pan, 10)
                    << sanitizeField(dd.deducteeName, 75)
                    << QString::number(dd.amountPaid, 'f', 2)
                    << QString::number(dd.totalTaxDeposited, 'f', 2)
                    << formatDateForFvu(dd.paymentDate)
                    << formatDateForFvu(dd.dateOfDeduction)
                    << QString::number(dd.tdsRate, 'f', 2)
                    << sanitizeField(dd.sectionCode, 5)
                    << sanitizeField(dd.reasonForNonDeduction, 1)
                    << sanitizeField(dd.certificateNo, 10);
                lines.append(ddr.join('^'));
            }
        }
        chIndex++;
    }

    return lines.join("\r\n") + "\r\n";
}

QString TdsFvuExporter::generateForm27Q(
    const QString& financialYear,
    const QString& quarter,
    const TdsDeductorInfo& deductor,
    const QList<TdsChallanEntry>& challans,
    const QList<TdsDeducteeEntry>& deductees
) {
    // Form 27Q format mirrors 26Q with 27Q form code and remittance country fields
    QString fvu26 = generateForm26Q(financialYear, quarter, deductor, challans, deductees);
    fvu26.replace("^26Q^", "^27Q^");
    return fvu26;
}

} // namespace MahadevERP
