#pragma once

#include <QString>
#include <QStringList>
#include <QDate>
#include <QList>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <QVariantMap>

namespace MahadevERP {

// GSTR-1 payload, shaped exactly to the GSTN return tables and the official
// offline-tool workbook (GSTR1GovtTemplate_v2.1):
//   b2b/b2cl/b2cs  Tables 4-5,7   cdnr/cdnur Table 9B
//   hsnB2B/hsnB2C   Table 12 (bifurcated B2B/B2C since May-2025)
//   exemp           Table 8        docs Table 13
// Rows that cannot legally go on the return (bad GSTIN, bad bill number,
// unknown rate/UQC/HSN, unclassifiable notes) are EXCLUDED and listed in
// `quarantine` with reasons, so the export is deterministic and the user
// knows exactly what to fix. Same input -> same payload, same file.

struct Gstr1B2BItem {
    double rate = 0.0;
    double taxableValue = 0.0;
    double igst = 0.0;
    double cgst = 0.0;
    double sgst = 0.0;
    double cess = 0.0;
};

struct Gstr1B2BInvoice {
    QString ctin; // Receiver GSTIN
    QString receiverName;
    QString invoiceNo;
    QDate invoiceDate;
    double invoiceValue = 0.0; // computed = taxable + taxes (rounded), never stored total
    QString pos;               // 2-digit state code ("06"); writer maps to "06-Haryana"
    QString reverseCharge = "N";
    QString invoiceType = "R"; // Regular (R/SEWP/SEWOP/DE in JSON, display name in Excel)
    QList<Gstr1B2BItem> items;
};

struct Gstr1B2CLInvoice {
    QString invoiceNo;
    QDate invoiceDate;
    double invoiceValue = 0.0;
    QString pos;
    double rate = 0.0;
    double taxableValue = 0.0;
    double igst = 0.0;
    double cess = 0.0;
};

struct Gstr1B2CSSummary {
    QString splyTy = "INTRA"; // INTER/INTRA from POS vs supplier state (JSON sply_ty)
    QString typ = "OE";       // OE = other than e-commerce (JSON typ / Excel Type)
    QString pos;
    double rate = 0.0;
    double taxableValue = 0.0;
    double igst = 0.0;
    double cgst = 0.0;
    double sgst = 0.0;
    double cess = 0.0;
};

struct Gstr1CdnItem {
    double rate = 0.0;
    double taxableValue = 0.0;
    double igst = 0.0;
    double cgst = 0.0;
    double sgst = 0.0;
    double cess = 0.0;
};

struct Gstr1CdnrNote {
    QString ctin;
    QString receiverName;
    QString noteType = "C"; // C = credit, D = debit
    QString noteNo;
    QDate noteDate;
    QString reason;         // e.g. "Sales Return" (reason_code minus "01-")
    QString pos;
    QString reverseCharge = "N";
    QString invType = "R";
    QString origInvNo;
    QDate origInvDate;
    double noteValue = 0.0;
    QList<Gstr1CdnItem> items;
};

struct Gstr1CdnurNote {
    QString urType = "B2CL"; // B2CL / EXPWP / EXPWOP (original supply kind)
    QString noteType = "C";
    QString noteNo;
    QDate noteDate;
    QString pos;
    double noteValue = 0.0;
    QList<Gstr1CdnItem> items;
};

struct Gstr1HsnItem {
    int serialNo = 1;
    QString hsnCode;
    double rate = 0.0; // Table 12 is HSN- *and rate*-wise; map key is hsn|rate
    QString description;
    QString uqc = "QTL"; // code only; writer maps to "QTL-QUINTAL" for Excel
    double totalQty = 0.0;
    double totalValue = 0.0;
    double taxableValue = 0.0;
    double igst = 0.0;
    double cgst = 0.0;
    double sgst = 0.0;
    double cess = 0.0;
};

struct Gstr1ExempRow {
    QString ttyp; // fixed Table-8 description
    double nilAmt = 0.0;
    double exmpAmt = 0.0;
    double ngsupAmt = 0.0;
};

struct Gstr1DocSummary {
    int docType = 1; // 1 = outward invoices, 4 = debit notes, 5 = credit notes
    QString docName;
    QString fromSerial;
    QString toSerial;
    int totalCount = 0;
    int cancelledCount = 0;
    int netIssuedCount = 0;
};

struct Gstr1ReturnPayload {
    QString gstin;
    QString legalName;
    QString fp; // Return Period e.g. "092026"
    double grossTurnover = 0.0;

    QList<Gstr1B2BInvoice> b2b;
    QList<Gstr1B2CLInvoice> b2cl;
    QList<Gstr1B2CSSummary> b2cs;
    QList<Gstr1CdnrNote> cdnr;
    QList<Gstr1CdnurNote> cdnur;
    QList<Gstr1HsnItem> hsnB2B; // Table 12, B2B tab (mandatory)
    QList<Gstr1HsnItem> hsnB2C; // Table 12, B2C tab
    QList<Gstr1ExempRow> exemp; // Table 8 (zeros when no exempt trade)
    QList<Gstr1DocSummary> docs;
    QStringList quarantine;     // excluded rows + reasons
    QStringList infoNotes;      // filed-but-confirm items (e.g. exempt total)
};

class Gstr1Engine {
public:
    static Gstr1ReturnPayload generateFromDatabase(
        const QString& gstin,
        const QString& legalName,
        const QString& stateCode,
        const QDate& fromDate,
        const QDate& toDate
    );

    static QJsonDocument exportToGovtOfflineJson(const Gstr1ReturnPayload& payload);

    // Portal rules (also used by the Excel writer + tests).
    static bool isValidGstin(const QString& gstin);       // structure + checksum
    static bool isValidInvoiceNo(const QString& invNo);   // <=16 chars, [A-Za-z0-9/-]
    static bool isValidGstRate(double rate);              // one of the 13 notified rates
    static QString posDisplayName(const QString& code);   // "06" -> "06-Haryana"
    static QString stateNameForCode(const QString& code);
};

} // namespace MahadevERP
