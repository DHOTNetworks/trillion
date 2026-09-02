#pragma once

#include <QString>
#include <QVariantList>
#include <QVariantMap>
#include <QMutex>
#include "sqlite3.h"

class DatabaseManager {
public:
    static DatabaseManager& instance();

    bool initDatabase(const QString& dbPath = "mahadev_accounting.db");
    sqlite3* getConnection();
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

private:
    DatabaseManager();
    ~DatabaseManager();
    DatabaseManager(const DatabaseManager&) = delete;
    DatabaseManager& operator=(const DatabaseManager&) = delete;

    sqlite3* m_db = nullptr;
    QMutex m_mutex;
    QString m_dbPath;

    void ensureTablesExist();
};
