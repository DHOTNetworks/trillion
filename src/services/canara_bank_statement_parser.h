#pragma once

#include <QString>
#include <QVector>
#include <QDate>
#include <QByteArray>
#include <QVariantMap>

struct CanaraBankTransaction {
    int index = 0;
    QString dateStr;           // e.g. "02-04-2026" or "02/04/2026"
    QString isoDate;           // e.g. "2026-04-02"
    QString rawNarration;      // Full original narration
    QString cleanNarration;    // Cleaned narration
    QString utrRef;            // Extracted UTR / Instrument reference
    double withdrawal = 0.0;   // Debit from bank (Payment)
    double deposit = 0.0;      // Credit to bank (Receipt)
    double balance = 0.0;      // Running balance
    QString category;          // "CUSTOMER_RECEIPT", "SUPPLIER_PAYMENT", "BANK_CHARGES", "INTEREST_DEBIT", "CHEQUE_RETURN_CHARGES", "INTERNAL_TRANSFER", "CHEQUE_CLEARING", "OTHER"
    QString extractedParty;    // Party name parsed from narration
    QString suggestedDrAccount;// Suggested Debit ledger
    QString suggestedCrAccount;// Suggested Credit ledger
    QString voucherType;       // "Cheque Receipt" or "Cheque Payment"
    QString confidence;        // "HIGH", "MEDIUM", "ALIAS_MATCH", "UNMATCHED"
    bool isDuplicate = false;  // Whether matching entry exists in DB
    bool isSelected = true;    // Selected for posting
};

struct CanaraBankStatementHeader {
    QString accountNo;
    QString firmName;
    QString ifscCode;
    QString branchName;
    QString fromDate;
    QString toDate;
    double openingBalance = 0.0;
};

class CanaraBankStatementParser {
public:
    CanaraBankStatementParser();

    // Parse PDF directly from file path
    bool parsePdf(const QString &pdfPath, 
                  CanaraBankStatementHeader &header, 
                  QVector<CanaraBankTransaction> &transactions, 
                  QString &errorMessage);

    // Parse from raw PDF byte array
    bool parsePdfData(const QByteArray &pdfData, 
                      CanaraBankStatementHeader &header, 
                      QVector<CanaraBankTransaction> &transactions, 
                      QString &errorMessage);

    // Extract text lines from PDF raw data
    static QVector<QString> extractPdfTextLines(const QByteArray &pdfData);

private:
    static QString decompressStream(const QByteArray &compressedData);
    static QVector<QString> parseTextFromContentStream(const QString &streamContent);
    static void classifyAndExtractParty(CanaraBankTransaction &txn);
};
