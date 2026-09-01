#include "database_manager.h"
#include <QDir>
#include <QFileInfo>
#include <QDebug>

DatabaseManager::DatabaseManager() {}

DatabaseManager::~DatabaseManager() {
    closeDatabase();
}

DatabaseManager& DatabaseManager::instance() {
    static DatabaseManager s_instance;
    return s_instance;
}

bool DatabaseManager::initDatabase(const QString& dbPath) {
    QMutexLocker locker(&m_mutex);
    if (m_db) return true;

    m_dbPath = dbPath;
    QFileInfo fi(dbPath);
    QDir dir = fi.dir();
    if (!dir.exists()) {
        dir.mkpath(".");
    }

    int rc = sqlite3_open(m_dbPath.toUtf8().constData(), &m_db);
    if (rc != SQLITE_OK) {
        qCritical() << "Failed to open SQLite database:" << sqlite3_errmsg(m_db);
        if (m_db) {
            sqlite3_close(m_db);
            m_db = nullptr;
        }
        return false;
    }

    // Enable WAL mode for high concurrency and performance
    executeNonQuery("PRAGMA journal_mode=WAL;");
    executeNonQuery("PRAGMA synchronous=NORMAL;");

    runMigrations();
    return true;
}

sqlite3* DatabaseManager::getConnection() {
    return m_db;
}

void DatabaseManager::closeDatabase() {
    QMutexLocker locker(&m_mutex);
    if (m_db) {
        sqlite3_close(m_db);
        m_db = nullptr;
    }
}

static void bindParams(sqlite3_stmt* stmt, const QVariantList& params) {
    for (int i = 0; i < params.size(); ++i) {
        int idx = i + 1;
        const QVariant& val = params.at(i);
        if (val.isNull()) {
            sqlite3_bind_null(stmt, idx);
        } else if (val.userType() == QMetaType::Int || val.userType() == QMetaType::LongLong) {
            sqlite3_bind_int64(stmt, idx, val.toLongLong());
        } else if (val.userType() == QMetaType::Double || val.userType() == QMetaType::Float) {
            sqlite3_bind_double(stmt, idx, val.toDouble());
        } else {
            QByteArray utf8 = val.toString().toUtf8();
            sqlite3_bind_text(stmt, idx, utf8.constData(), utf8.length(), SQLITE_TRANSIENT);
        }
    }
}

QVariantList DatabaseManager::executeQuery(const QString& sql, const QVariantList& params) {
    QMutexLocker locker(&m_mutex);
    QVariantList results;
    if (!m_db) return results;

    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(m_db, sql.toUtf8().constData(), -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        qWarning() << "SQL Prepare Error:" << sqlite3_errmsg(m_db) << "in SQL:" << sql;
        return results;
    }

    bindParams(stmt, params);

    int colCount = sqlite3_column_count(stmt);
    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        QVariantMap row;
        for (int i = 0; i < colCount; ++i) {
            QString colName = QString::fromUtf8(sqlite3_column_name(stmt, i));
            int colType = sqlite3_column_type(stmt, i);
            QVariant colVal;
            if (colType == SQLITE_INTEGER) {
                colVal = static_cast<qint64>(sqlite3_column_int64(stmt, i));
            } else if (colType == SQLITE_FLOAT) {
                colVal = sqlite3_column_double(stmt, i);
            } else if (colType == SQLITE_TEXT) {
                colVal = QString::fromUtf8(reinterpret_cast<const char*>(sqlite3_column_text(stmt, i)));
            } else if (colType == SQLITE_NULL) {
                colVal = QVariant();
            }
            row[colName] = colVal;
        }
        results.append(row);
    }

    sqlite3_finalize(stmt);
    return results;
}

bool DatabaseManager::executeNonQuery(const QString& sql, const QVariantList& params) {
    QMutexLocker locker(&m_mutex);
    if (!m_db) return false;

    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(m_db, sql.toUtf8().constData(), -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        qWarning() << "SQL Prepare Error:" << sqlite3_errmsg(m_db) << "in SQL:" << sql;
        return false;
    }

    bindParams(stmt, params);
    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return (rc == SQLITE_DONE || rc == SQLITE_ROW);
}

QVariant DatabaseManager::executeScalar(const QString& sql, const QVariantList& params) {
    QMutexLocker locker(&m_mutex);
    if (!m_db) return QVariant();

    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(m_db, sql.toUtf8().constData(), -1, &stmt, nullptr);
    if (rc != SQLITE_OK) return QVariant();

    bindParams(stmt, params);
    QVariant result;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        int colType = sqlite3_column_type(stmt, 0);
        if (colType == SQLITE_INTEGER) {
            result = static_cast<qint64>(sqlite3_column_int64(stmt, 0));
        } else if (colType == SQLITE_FLOAT) {
            result = sqlite3_column_double(stmt, 0);
        } else if (colType == SQLITE_TEXT) {
            result = QString::fromUtf8(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0)));
        }
    }
    sqlite3_finalize(stmt);
    return result;
}

void DatabaseManager::runMigrations() {
    // Financial Years
    executeNonQuery(
        "CREATE TABLE IF NOT EXISTS financial_years ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "year_name TEXT NOT NULL UNIQUE,"
        "start_date TEXT NOT NULL,"
        "end_date TEXT NOT NULL,"
        "is_active INTEGER DEFAULT 0,"
        "is_locked INTEGER DEFAULT 0"
        ");"
    );

    // Seed default financial years if empty
    QVariant fyCount = executeScalar("SELECT COUNT(*) FROM financial_years;");
    if (!fyCount.isValid() || fyCount.toLongLong() == 0) {
        executeNonQuery("INSERT INTO financial_years (year_name, start_date, end_date, is_active) VALUES ('FY 2026-27', '2026-04-01', '2027-03-31', 1);");
        executeNonQuery("INSERT INTO financial_years (year_name, start_date, end_date, is_active) VALUES ('FY 2025-26', '2025-04-01', '2026-03-31', 0);");
    }

    // Account Groups
    executeNonQuery(
        "CREATE TABLE IF NOT EXISTS account_groups ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "name TEXT NOT NULL UNIQUE,"
        "parent_group_name TEXT,"
        "nature TEXT NOT NULL,"
        "description TEXT,"
        "extract_in_balance_sheet INTEGER DEFAULT 0,"
        "is_system INTEGER DEFAULT 1"
        ");"
    );

    // Paddy Arrivals
    executeNonQuery(
        "CREATE TABLE IF NOT EXISTS paddy_arrivals ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "slip_no TEXT NOT NULL,"
        "arrival_date TEXT NOT NULL,"
        "farmer_id INTEGER,"
        "farmer_name TEXT,"
        "paddy_variety TEXT,"
        "bag_count INTEGER,"
        "gross_weight_qtl REAL,"
        "moisture_pct REAL,"
        "moisture_deduction_qtl REAL,"
        "net_weight_qtl REAL,"
        "rate_per_qtl REAL,"
        "hamali_charges REAL,"
        "net_amount REAL,"
        "payment_status TEXT"
        ");"
    );

    // Vouchers Table
    executeNonQuery(
        "CREATE TABLE IF NOT EXISTS vouchers ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "fy_id INTEGER,"
        "financial_year TEXT,"
        "voucher_no TEXT NOT NULL,"
        "instrument_no TEXT,"
        "voucher_date TEXT NOT NULL,"
        "voucher_type TEXT NOT NULL,"
        "legacy_type TEXT,"
        "party_id INTEGER,"
        "party_name TEXT,"
        "account_type TEXT,"
        "amount REAL NOT NULL,"
        "narration TEXT"
        ");"
    );

    // Sales Invoices
    executeNonQuery(
        "CREATE TABLE IF NOT EXISTS sales_invoices ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "fy_id INTEGER,"
        "financial_year TEXT,"
        "voucher_no TEXT,"
        "invoice_no TEXT NOT NULL,"
        "invoice_date TEXT NOT NULL,"
        "customer_id INTEGER,"
        "customer_name TEXT NOT NULL,"
        "gstin TEXT,"
        "item_name TEXT NOT NULL,"
        "hsn_code TEXT,"
        "bag_count INTEGER,"
        "weight_qtl REAL,"
        "rate_per_qtl REAL,"
        "taxable_amount REAL,"
        "gst_pct REAL,"
        "cgst_amount REAL DEFAULT 0.0,"
        "sgst_amount REAL DEFAULT 0.0,"
        "igst_amount REAL DEFAULT 0.0,"
        "round_off REAL DEFAULT 0.0,"
        "gst_amount REAL,"
        "total_amount REAL,"
        "payment_mode TEXT,"
        "vehicle_no TEXT,"
        "eway_bill_no TEXT,"
        "narration TEXT,"
        "sale_status TEXT DEFAULT 'Self Sale',"
        "market_fee_status TEXT DEFAULT 'Paid',"
        "dami REAL DEFAULT 0.0,"
        "labour REAL DEFAULT 0.0,"
        "auction REAL DEFAULT 0.0,"
        "m_fee REAL DEFAULT 0.0,"
        "hrdf REAL DEFAULT 0.0,"
        "other_exp REAL DEFAULT 0.0,"
        "welfare REAL DEFAULT 0.0,"
        "dhrmd REAL DEFAULT 0.0,"
        "sutli REAL DEFAULT 0.0,"
        "less_amount REAL DEFAULT 0.0,"
        "gr_no TEXT,"
        "driver TEXT,"
        "bill_time TEXT,"
        "sauda_date TEXT,"
        "shipping_address TEXT,"
        "po_no TEXT,"
        "grade TEXT,"
        "kanda_weight TEXT,"
        "transport TEXT,"
        "broker_name TEXT"
        ");"
    );

    // Purchase Invoices
    executeNonQuery(
        "CREATE TABLE IF NOT EXISTS purchase_invoices ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "fy_id INTEGER,"
        "financial_year TEXT,"
        "voucher_no TEXT,"
        "invoice_no TEXT NOT NULL,"
        "invoice_date TEXT NOT NULL,"
        "supplier_id INTEGER,"
        "supplier_name TEXT NOT NULL,"
        "gstin TEXT,"
        "item_name TEXT NOT NULL,"
        "hsn_code TEXT,"
        "bag_count INTEGER,"
        "weight_qtl REAL,"
        "rate_per_qtl REAL,"
        "taxable_amount REAL,"
        "gst_pct REAL,"
        "cgst_amount REAL DEFAULT 0.0,"
        "sgst_amount REAL DEFAULT 0.0,"
        "igst_amount REAL DEFAULT 0.0,"
        "round_off REAL DEFAULT 0.0,"
        "gst_amount REAL,"
        "total_amount REAL,"
        "payment_mode TEXT,"
        "vehicle_no TEXT,"
        "eway_bill_no TEXT,"
        "narration TEXT,"
        "sale_status TEXT DEFAULT 'Self Sale',"
        "market_fee_status TEXT DEFAULT 'Paid',"
        "dami REAL DEFAULT 0.0,"
        "labour REAL DEFAULT 0.0,"
        "auction REAL DEFAULT 0.0,"
        "m_fee REAL DEFAULT 0.0,"
        "hrdf REAL DEFAULT 0.0,"
        "other_exp REAL DEFAULT 0.0,"
        "welfare REAL DEFAULT 0.0,"
        "dhrmd REAL DEFAULT 0.0,"
        "sutli REAL DEFAULT 0.0,"
        "less_amount REAL DEFAULT 0.0,"
        "gr_no TEXT,"
        "driver TEXT,"
        "bill_time TEXT,"
        "sauda_date TEXT,"
        "shipping_address TEXT,"
        "po_no TEXT,"
        "grade TEXT,"
        "kanda_weight TEXT,"
        "transport TEXT,"
        "broker_name TEXT"
        ");"
    );

    // Stock Items
    executeNonQuery(
        "CREATE TABLE IF NOT EXISTS stock_items ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "item_name TEXT NOT NULL UNIQUE,"
        "category TEXT NOT NULL,"
        "item_type TEXT,"
        "packing_kg REAL DEFAULT 50.0,"
        "opening_bags INTEGER DEFAULT 0,"
        "opening_weight_qtl REAL DEFAULT 0.0,"
        "opening_rate REAL DEFAULT 0.0,"
        "opening_value REAL DEFAULT 0.0,"
        "sale_rate REAL DEFAULT 0.0,"
        "purchase_rate REAL DEFAULT 0.0,"
        "gst_rate TEXT DEFAULT '5%',"
        "hsn_code TEXT,"
        "current_bags INTEGER DEFAULT 0,"
        "current_weight_qtl REAL DEFAULT 0.0"
        ");"
    );

    // Parties Table
    executeNonQuery(
        "CREATE TABLE IF NOT EXISTS parties ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "party_name TEXT NOT NULL UNIQUE,"
        "group_name TEXT NOT NULL,"
        "station TEXT,"
        "mobile TEXT,"
        "address TEXT,"
        "gstin TEXT,"
        "opening_balance REAL DEFAULT 0.0,"
        "balance_type TEXT DEFAULT 'Dr',"
        "credit_days INTEGER DEFAULT 0,"
        "credit_limit REAL DEFAULT 0.0"
        ");"
    );

    // Milling Batches Table
    executeNonQuery(
        "CREATE TABLE IF NOT EXISTS milling_batches ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "fy_id INTEGER,"
        "financial_year TEXT,"
        "batch_no TEXT NOT NULL UNIQUE,"
        "batch_date TEXT NOT NULL,"
        "paddy_item TEXT NOT NULL,"
        "paddy_bags INTEGER,"
        "paddy_weight_qtl REAL,"
        "rice_item TEXT NOT NULL,"
        "rice_bags INTEGER,"
        "rice_weight_qtl REAL,"
        "outturn_pct REAL,"
        "rice_cost REAL DEFAULT 0.0,"
        "bran_item TEXT,"
        "bran_bags INTEGER,"
        "bran_weight_qtl REAL,"
        "bran_cost REAL DEFAULT 0.0,"
        "husk_item TEXT,"
        "husk_bags INTEGER,"
        "husk_weight_qtl REAL,"
        "husk_cost REAL DEFAULT 0.0,"
        "nakku_item TEXT,"
        "nakku_bags INTEGER,"
        "nakku_weight_qtl REAL,"
        "nakku_cost REAL DEFAULT 0.0,"
        "other_byproduct_item TEXT,"
        "other_byproduct_bags INTEGER,"
        "other_byproduct_weight_qtl REAL,"
        "other_byproduct_cost REAL DEFAULT 0.0,"
        "total_byproduct_bags INTEGER DEFAULT 0,"
        "total_byproduct_weight_qtl REAL DEFAULT 0.0,"
        "total_byproduct_cost REAL DEFAULT 0.0,"
        "loss_weight_qtl REAL,"
        "loss_pct REAL,"
        "milling_cost_per_qtl REAL,"
        "total_milling_cost REAL,"
        "operator_name TEXT,"
        "notes TEXT"
        ");"
    );
}
