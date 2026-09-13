#pragma once

#include <QString>
#include <QVector>
#include <QVariantMap>
#include <QVariantList>

struct BankStatementTransaction {
    QString date;             // ISO: YYYY-MM-DD
    QString rawDate;          // Original date string
    QString txnId;            // Cheque No / UTR / Reference
    double withdrawal = 0.0;  // Debit
    double deposit = 0.0;     // Credit
    double balance = 0.0;     // Running balance
    QString remarks;          // Full narration
    QString matchedParty;     // Auto-matched party name
    int matchedPartyId = 0;   // Party ID in SQLite
    QString voucherType;      // Payment / Receipt / Contra / Journal
};

struct BankStatementMetadata {
    QString bankName;
    QString accountNumber;
    QString customerId;
    QString ifscCode;
    QString branchName;
    QString accountName;
    QString address;
    QString periodFrom;
    QString periodTo;
    double openingBalance = 0.0;
    double closingBalance = 0.0;
    double totalWithdrawals = 0.0;
    double totalDeposits = 0.0;
    int totalTransactions = 0;
};

class BankStatementExcelParser {
public:
    static bool parseFile(const QString& filePath,
                          BankStatementMetadata& outMeta,
                          QVector<BankStatementTransaction>& outTxns,
                          QString& outError);

    static bool parseXls(const QString& filePath,
                         BankStatementMetadata& outMeta,
                         QVector<BankStatementTransaction>& outTxns,
                         QString& outError);

    static bool parseXlsx(const QString& filePath,
                          BankStatementMetadata& outMeta,
                          QVector<BankStatementTransaction>& outTxns,
                          QString& outError);

    static bool parseCsv(const QString& filePath,
                         BankStatementMetadata& outMeta,
                         QVector<BankStatementTransaction>& outTxns,
                         QString& outError);

    static QString parseDateToIso(const QString& rawDate);
    static double parseAmount(const QString& rawAmount);
};
