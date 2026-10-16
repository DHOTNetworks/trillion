#pragma once

#include <QString>
#include <QVariantList>
#include <QVariantMap>
#include <QRecursiveMutex>
#include "sqlite3.h"

class DatabaseManager {
public:
    static DatabaseManager& instance();

    bool initDatabase(const QString& dbPath);
    bool switchDatabase(const QString& newDbPath);
    sqlite3* getConnection();
    QString dbPath() const { return m_dbPath; }
    void closeDatabase();

    // Transactions
    bool beginTransaction();
    bool commit();
    bool rollback();

    // Query Execution Helpers
    QVariantList executeQuery(const QString& sql, const QVariantList& params = {});
    bool executeNonQuery(const QString& sql, const QVariantList& params = {});
    QVariant executeScalar(const QString& sql, const QVariantList& params = {});
    qint64 lastInsertedId();
    void ensureTablesExist();

    // Package master top-ups: idempotent domain groups + voucher
    // types for every currently enabled package. Never deletes. Call after
    // wizard saves, successful migrations, or re-opens with new data.
    void ensureDomainMasters();

    // Last SQLite error text (e.g. "UNIQUE constraint failed …"). Set on
    // every failed executeNonQuery/executeScalar, cleared on entry. Read it
    // immediately after a false return — any later call may overwrite it.
    QString lastErrorText();

    // Deterministic Document Metadata Resolver (Unifies transport, broker, vehicle, driver)
    struct DocumentMetadata {
        QString transport;
        QString brokerName;
        QString vehicleNo;
        QString driverName;
    };
    DocumentMetadata resolveDocumentMetadata(const QString& voucherNo, const QString& invoiceNo,
                                            const QString& existingTransport = QString(),
                                            const QString& existingBroker = QString(),
                                            const QString& existingVehicle = QString(),
                                            const QString& existingDriver = QString());

    // App Settings Key-Value Store
    QString getSetting(const QString& key, const QString& defaultVal = "");
    bool setSetting(const QString& key, const QString& val);

    // Audit trail (Tally Edit-Log parity foundation). Best-effort: returns
    // false on DB error but never throws — callers must not fail a posting
    // because the audit row failed. Rows written inside the caller's
    // transaction roll back with it; BLOCKED rows are logged outside any txn.
    bool logAudit(const QString& action, const QString& tableName,
                  const QString& rowId, const QString& beforeJson,
                  const QString& afterJson, const QString& voucherNo = "",
                  const QString& fyName = "");

private:
    DatabaseManager();
    ~DatabaseManager();
    DatabaseManager(const DatabaseManager&) = delete;
    DatabaseManager& operator=(const DatabaseManager&) = delete;

    sqlite3* m_db = nullptr;
    QRecursiveMutex m_mutex;
    QString m_dbPath;
    QString m_lastError;
};
