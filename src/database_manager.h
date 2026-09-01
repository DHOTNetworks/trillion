#pragma once

#include <QString>
#include <QVariantList>
#include <QVariantMap>
#include <QMutex>
#include "sqlite3.h"

class DatabaseManager {
public:
    static DatabaseManager& instance();

    bool initDatabase(const QString& dbPath = "data/mahadev_erp.db");
    sqlite3* getConnection();
    void closeDatabase();

    // Query helper methods
    QVariantList executeQuery(const QString& sql, const QVariantList& params = {});
    bool executeNonQuery(const QString& sql, const QVariantList& params = {});
    QVariant executeScalar(const QString& sql, const QVariantList& params = {});

private:
    DatabaseManager();
    ~DatabaseManager();
    DatabaseManager(const DatabaseManager&) = delete;
    DatabaseManager& operator=(const DatabaseManager&) = delete;

    sqlite3* m_db = nullptr;
    QMutex m_mutex;
    QString m_dbPath;

    void runMigrations();
};
