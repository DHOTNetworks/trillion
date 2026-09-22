#pragma once

#include <QString>
#include <QDate>
#include <QList>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <QVariantMap>

namespace MahadevERP {

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
    double invoiceValue = 0.0;
    QString pos;
    QString reverseCharge = "N";
    QString invoiceType = "R"; // Regular
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
    QString supplyType = "OE"; // Other than E-commerce
    QString pos;
    double rate = 0.0;
    double taxableValue = 0.0;
    double igst = 0.0;
    double cgst = 0.0;
    double sgst = 0.0;
    double cess = 0.0;
};

struct Gstr1HsnItem {
    int serialNo = 1;
    QString hsnCode;
    QString description;
    QString uqc = "QTL";
    double totalQty = 0.0;
    double totalValue = 0.0;
    double taxableValue = 0.0;
    double igst = 0.0;
    double cgst = 0.0;
    double sgst = 0.0;
    double cess = 0.0;
};

struct Gstr1DocSummary {
    int docType = 1; // Invoices for outward supply
    QString docName = "Invoices for outward supply";
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
    QList<Gstr1HsnItem> hsn;
    QList<Gstr1DocSummary> docs;
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
};

} // namespace MahadevERP
