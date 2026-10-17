#include "database_manager.h"
#include "config/feature_flags.h"
#include "models/account_classifier.h"
#include <initializer_list>
#include "engine/accounting_engine.h"
#include <QDir>
#include <QFileInfo>
#include <QDebug>
#include <QElapsedTimer>

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
    if (dbPath.trimmed().isEmpty()) return false;
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

    // High performance WAL mode with memory-mapped I/O and large memory cache
    executeNonQuery("PRAGMA journal_mode=WAL;");
    executeNonQuery("PRAGMA synchronous=NORMAL;");
    executeNonQuery("PRAGMA foreign_keys=ON;");
    executeNonQuery("PRAGMA busy_timeout=5000;"); // 5-second automatic lock wait for multi-instance concurrency
    executeNonQuery("PRAGMA cache_size=-64000;"); // 64 MB page cache
    executeNonQuery("PRAGMA temp_store=MEMORY;");
    executeNonQuery("PRAGMA mmap_size=268435456;"); // 256 MB memory-mapped I/O for instant reads

    ensureTablesExist();
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
    AccountClassifier::invalidateCache();
    AccountingEngine::clearActivePeriod();
}

bool DatabaseManager::switchDatabase(const QString& newDbPath) {
    QMutexLocker locker(&m_mutex);
    closeDatabase();
    return initDatabase(newDbPath);
}

bool DatabaseManager::beginTransaction() {
    return executeNonQuery("BEGIN TRANSACTION;");
}

bool DatabaseManager::commit() {
    return executeNonQuery("COMMIT;");
}

bool DatabaseManager::rollback() {
    return executeNonQuery("ROLLBACK;");
}

qint64 DatabaseManager::lastInsertedId() {
    QMutexLocker locker(&m_mutex);
    if (!m_db) return 0;
    return static_cast<qint64>(sqlite3_last_insert_rowid(m_db));
}

QString DatabaseManager::lastErrorText() {
    QMutexLocker locker(&m_mutex);
    return m_lastError;
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
        } else if (val.userType() == QMetaType::Bool) {
            sqlite3_bind_int(stmt, idx, val.toBool() ? 1 : 0);
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
            } else if (colType == SQLITE_BLOB) {
                const char* bData = reinterpret_cast<const char*>(sqlite3_column_blob(stmt, i));
                int bBytes = sqlite3_column_bytes(stmt, i);
                colVal = QByteArray(bData, bBytes);
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
    m_lastError.clear();
    if (!m_db) return false;

    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(m_db, sql.toUtf8().constData(), -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        m_lastError = QString::fromUtf8(sqlite3_errmsg(m_db));
        qWarning() << "SQL Prepare Error:" << m_lastError << "in SQL:" << sql;
        return false;
    }

    bindParams(stmt, params);
    rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE && rc != SQLITE_ROW) {
        m_lastError = QString::fromUtf8(sqlite3_errmsg(m_db));
        qWarning() << "SQL Step Error:" << m_lastError << "in SQL:" << sql;
    }
    sqlite3_finalize(stmt);
    return (rc == SQLITE_DONE || rc == SQLITE_ROW);
}

QVariant DatabaseManager::executeScalar(const QString& sql, const QVariantList& params) {
    QMutexLocker locker(&m_mutex);
    m_lastError.clear();
    if (!m_db) return QVariant();

    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(m_db, sql.toUtf8().constData(), -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        m_lastError = QString::fromUtf8(sqlite3_errmsg(m_db));
        return QVariant();
    }

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

void DatabaseManager::ensureTablesExist() {
    QElapsedTimer ddlTimer;
    ddlTimer.start();
    executeNonQuery("BEGIN TRANSACTION;");

    // -1. Application Key-Value Settings
    executeNonQuery(
        "CREATE TABLE IF NOT EXISTS app_settings ("
        "key TEXT PRIMARY KEY,"
        "value TEXT"
        ");"
    );

    // Flag-resolved: effective domain set for the gated DDL below (stored
    // keys upgraded by data-detected ON bits, so migrated firms always keep
    // their domain tables; fresh generic firms get core tables only).
    // Computed after app_settings exists; detection is existence-guarded,
    // so position within this function is safe.
    const FeatureFlags domainFlags = FeatureFlags::resolveEffective();

    // -0. GSTIN lookup response cache (one fetch per GSTIN per database, ever).
    executeNonQuery(
        "CREATE TABLE IF NOT EXISTS gstin_lookup_cache ("
        "gstin TEXT PRIMARY KEY,"
        "payload TEXT NOT NULL,"
        "fetched_at TEXT DEFAULT CURRENT_TIMESTAMP"
        ");"
    );

    // 0. Company Info
    executeNonQuery(
        "CREATE TABLE IF NOT EXISTS company_info ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "company_name TEXT NOT NULL,"
        "firm_type TEXT,"
        "business_type TEXT,"
        "address TEXT,"
        "city TEXT,"
        "state TEXT,"
        "state_code TEXT,"
        "pincode TEXT,"
        "phone TEXT,"
        "mobile TEXT,"
        "email TEXT,"
        "gstin TEXT,"
        "pan_no TEXT,"
        "fssai_no TEXT,"
        "ml_no TEXT,"
        "bank_name TEXT,"
        "bank_account TEXT,"
        "ifsc_code TEXT,"
        "books_from TEXT,"
        "acc_year_from TEXT,"
        "acc_year_to TEXT,"
        "data_file_source TEXT"
        ");"
    );

    // 1. Financial Years
    executeNonQuery(
        "CREATE TABLE IF NOT EXISTS financial_years ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "year_name TEXT UNIQUE NOT NULL,"
        "start_date TEXT NOT NULL,"
        "end_date TEXT NOT NULL,"
        "is_active INTEGER DEFAULT 0,"
        "is_locked INTEGER DEFAULT 0"
        ");"
    );

    // 1b. Year-End Opening Balances (F3 close engine writes these; F4 reports
    // consume them). Core table: one row per balance-sheet ledger per opening
    // year. Written only by the close procedure / reversal cleanup.
    executeNonQuery(
        "CREATE TABLE IF NOT EXISTS fy_opening_balances ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "fy_id INTEGER NOT NULL,"
        "party_id INTEGER,"
        "party_name TEXT NOT NULL,"
        "amount REAL NOT NULL DEFAULT 0.0,"
        "balance_type TEXT NOT NULL DEFAULT 'Dr',"
        "created_at TEXT DEFAULT CURRENT_TIMESTAMP,"
        "FOREIGN KEY (fy_id) REFERENCES financial_years(id),"
        "FOREIGN KEY (party_id) REFERENCES parties(id)"
        ");"
    );

    // 1b. Voucher Types Master (generic-ERP pivot: single source of
    // truth for every voucher kind; replaces hardcoded Sale/Purc/… literals
    // and the never-created voucher_settings table the provider used to read).
    executeNonQuery(
        "CREATE TABLE IF NOT EXISTS voucher_types ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "code TEXT UNIQUE NOT NULL,"
        "name TEXT NOT NULL,"
        "category TEXT NOT NULL DEFAULT 'OTHER',"
        "is_system INTEGER DEFAULT 0,"
        "prefix TEXT DEFAULT '',"
        "numbering_method TEXT DEFAULT 'AUTO',"
        "separator TEXT DEFAULT '-',"
        "number_width INTEGER DEFAULT 4,"
        "reset_each_fy INTEGER DEFAULT 1,"
        "affects_stock INTEGER DEFAULT 0,"
        "requires_party INTEGER DEFAULT 1,"
        "requires_stock INTEGER DEFAULT 0,"
        "requires_gst INTEGER DEFAULT 0,"
        "sort_order INTEGER DEFAULT 0,"
        "is_active INTEGER DEFAULT 1"
        ");"
    );

    // Core seed: Tally-compatible codes, undeletable. Legacy short tokens
    // (Sale/Purc/Jrnl/…) live on as aliases in canonicalTypeToken only.
    {
        struct VoucherTypeSeed {
            const char* code; const char* name; const char* category;
            const char* prefix; int width; int affectsStock;
            int reqParty; int reqStock; int reqGst; int order;
        };
        static const VoucherTypeSeed coreTypes[] = {
            {"SALE",   "Sales Invoice",  "SALES",    "SAL-", 4, 1, 1, 1, 1, 10},
            {"PURC",   "Purchase Bill",  "PURCHASE", "PUR-", 4, 1, 1, 1, 1, 20},
            {"PYMT",   "Payment",        "PAYMENT",  "PAY-", 4, 0, 1, 0, 0, 30},
            {"RCPT",   "Receipt",        "RECEIPT",  "REC-", 4, 0, 1, 0, 0, 40},
            {"JRNL",   "Journal",        "JOURNAL",  "JNL-", 4, 0, 1, 0, 0, 50},
            {"CONTRA", "Contra",         "CONTRA",   "CNR-", 4, 0, 1, 0, 0, 60},
            {"CHPT",   "Cheque Payment", "PAYMENT",  "CHP-", 4, 0, 1, 0, 0, 31},
            {"CHRT",   "Cheque Receipt", "RECEIPT",  "CHR-", 4, 0, 1, 0, 0, 41},
        };
        QVariant vtCount = executeScalar("SELECT COUNT(*) FROM voucher_types;");
        if (!vtCount.isValid() || vtCount.toLongLong() == 0) {
            for (const auto& t : coreTypes) {
                executeNonQuery(
                    "INSERT INTO voucher_types (code, name, category, is_system, prefix, "
                    "numbering_method, separator, number_width, reset_each_fy, "
                    "affects_stock, requires_party, requires_stock, requires_gst, "
                    "sort_order, is_active) "
                    "VALUES (?, ?, ?, 1, ?, 'AUTO', '-', ?, 1, ?, ?, ?, ?, ?, 1);",
                    {t.code, t.name, t.category, t.prefix, t.width,
                     t.affectsStock, t.reqParty, t.reqStock, t.reqGst, t.order}
                );
            }
        }
    }

    // 2. Parties Master
    executeNonQuery(
        "CREATE TABLE IF NOT EXISTS parties ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "name TEXT NOT NULL,"
        "alias TEXT,"
        "prefix TEXT,"
        "group_name TEXT,"
        "group_code INTEGER DEFAULT 8,"
        "group_id INTEGER,"
        "party_type TEXT,"
        "special_type TEXT,"
        "opening_balance REAL DEFAULT 0.0,"
        "balance_type TEXT DEFAULT 'Cr',"
        "mailing_name TEXT,"
        "address TEXT,"
        "city TEXT,"
        "district TEXT,"
        "state TEXT,"
        "state_code TEXT,"
        "pincode TEXT,"
        "country TEXT DEFAULT 'India',"
        "route TEXT,"
        "mobile TEXT,"
        "whatsapp TEXT,"
        "phone TEXT,"
        "email TEXT,"
        "contact_person TEXT,"
        "pan TEXT,"
        "aadhaar TEXT,"
        "tan TEXT,"
        "gstin TEXT,"
        "gst_party_type TEXT DEFAULT 'Unregistered',"
        "bank_name TEXT,"
        "bank_account TEXT,"
        "ifsc_code TEXT,"
        "credit_limit REAL DEFAULT 0.0,"
        "credit_days INTEGER DEFAULT 30,"
        "interest_rate REAL DEFAULT 0.0,"
        "commission_rate REAL DEFAULT 0.0,"
        "commission_on TEXT,"
        "apply_tcs INTEGER DEFAULT 0,"
        "tcs_exempt INTEGER DEFAULT 0,"
        "party_station TEXT,"
        "use_routes INTEGER DEFAULT 0,"
        "shop_no TEXT,"
        "tin TEXT,"
        "urn TEXT,"
        "stock_not_calc INTEGER DEFAULT 0,"
        "use_credit_limit INTEGER DEFAULT 1,"
        "show_date_totals INTEGER DEFAULT 0,"
        "calc_direct_expense INTEGER DEFAULT 0,"
        "set_title_case INTEGER DEFAULT 1,"
        "ledger_open_from TEXT,"
        "books_start_from TEXT,"
        "salary_per_month REAL DEFAULT 0.0,"
        "legacy_id INTEGER"
        ");"
    );

    // 3. Stock Items Master & Groups / Units
    executeNonQuery(
        "CREATE TABLE IF NOT EXISTS stock_groups ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "group_name TEXT UNIQUE NOT NULL,"
        "legacy_code INTEGER,"
        "qty_not_show_in_trading INTEGER DEFAULT 0,"
        "cl_stock_rate REAL DEFAULT 0.0"
        ");"
    );

    executeNonQuery(
        "CREATE TABLE IF NOT EXISTS stock_units ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "unit_name TEXT UNIQUE NOT NULL,"
        "legacy_code INTEGER,"
        "decimal_places INTEGER DEFAULT 2"
        ");"
    );

    executeNonQuery(
        "CREATE TABLE IF NOT EXISTS stock_items ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "name TEXT NOT NULL,"
        "code TEXT UNIQUE NOT NULL,"
        "alias TEXT,"
        "print_name TEXT,"
        "item_type TEXT NOT NULL DEFAULT 'Both',"
        "goods_type TEXT DEFAULT 'Goods',"
        "trading_group TEXT,"
        "group_code INTEGER,"
        "company_name TEXT,"
        "category_name TEXT,"
        "unit TEXT DEFAULT 'Qtl.',"
        "unit_code INTEGER,"
        "alt_unit TEXT DEFAULT 'Bags',"
        "conversion_factor REAL DEFAULT 1.0,"
        "rate_calc_on TEXT DEFAULT 'N/A',"
        "auto_adjust_name INTEGER DEFAULT 1,"
        "item_narration TEXT,"
        "capital_goods INTEGER DEFAULT 0,"
        "hsn_code TEXT DEFAULT '',"
        "gst_rate REAL DEFAULT 5.0,"
        "cess_rate REAL DEFAULT 0.0,"
        "vat_rate REAL DEFAULT 0.0,"
        "vat_ledger TEXT DEFAULT 'VAT A/c',"
        "surcharge_on_vat REAL DEFAULT 0.0,"
        "vat_against_d1 REAL DEFAULT 0.0,"
        "cst_rate REAL DEFAULT 0.0,"
        "cst_ledger TEXT DEFAULT 'CST A/c',"
        "cst_without_cform REAL DEFAULT 0.0,"
        "dami_rate REAL DEFAULT 0.0,"
        "dami_ledger TEXT DEFAULT 'Dami A/c',"
        "market_fee_rate REAL DEFAULT 0.0,"
        "market_fee_ledger TEXT DEFAULT 'Market Fee A/c',"
        "hrdf_rate REAL DEFAULT 0.0,"
        "hrdf_ledger TEXT DEFAULT 'H.R.D.F. A/c',"
        "market_commtt_form_apply INTEGER DEFAULT 0,"
        "market_commtt_coupon_apply INTEGER DEFAULT 0,"
        "dami_calc_on_weight INTEGER DEFAULT 0,"
        "tax_on_qty INTEGER DEFAULT 0,"
        "purchase_rate REAL DEFAULT 0.0,"
        "sale_rate REAL DEFAULT 0.0,"
        "bonus_approved REAL DEFAULT 0.0,"
        "mrp REAL DEFAULT 0.0,"
        "min_rate REAL DEFAULT 0.0,"
        "discount REAL DEFAULT 0.0,"
        "packing_kg REAL DEFAULT 50.0,"
        "opening_bags INTEGER DEFAULT 0,"
        "opening_qty REAL DEFAULT 0.0,"
        "opening_rate REAL DEFAULT 0.0,"
        "opening_value REAL DEFAULT 0.0,"
        "purchase_ledger TEXT DEFAULT 'Purchase Accounts',"
        "purchase_ledger_id INTEGER,"
        "purchase_return_ledger TEXT DEFAULT 'Purchase Accounts',"
        "purchase_return_ledger_id INTEGER,"
        "sale_ledger TEXT DEFAULT 'Sales Accounts',"
        "sale_ledger_id INTEGER,"
        "sale_return_ledger TEXT DEFAULT 'Sales Accounts',"
        "sale_return_ledger_id INTEGER,"
        "stock_ledger TEXT DEFAULT 'Stock-in-Hand',"
        "stock_ledger_id INTEGER,"
        "gst_ledger TEXT DEFAULT 'Duties & Taxes',"
        "gst_ledger_id INTEGER,"
        "vat_ledger_id INTEGER,"
        "cst_ledger_id INTEGER,"
        "dami_ledger_id INTEGER,"
        "market_fee_ledger_id INTEGER,"
        "hrdf_ledger_id INTEGER,"
        "is_manufacturing_item INTEGER DEFAULT 0,"
        "manufacturing_role TEXT NOT NULL DEFAULT '',"
        "include_in_trading INTEGER DEFAULT 1,"
        "calculate_stock INTEGER DEFAULT 1,"
        "labour_rate_unit TEXT DEFAULT 'Packing',"
        "utrai_rate_1 REAL DEFAULT 0.0,"
        "jharai_rate_1 REAL DEFAULT 0.0,"
        "bharai_rate_1 REAL DEFAULT 0.0,"
        "tulai_rate_1 REAL DEFAULT 0.0,"
        "khichai_rate_1 REAL DEFAULT 0.0,"
        "silai_rate_1 REAL DEFAULT 0.0,"
        "loading_rate_1 REAL DEFAULT 0.0,"
        "utrai_rate_2 REAL DEFAULT 0.0,"
        "jharai_rate_2 REAL DEFAULT 0.0,"
        "bharai_rate_2 REAL DEFAULT 0.0,"
        "tulai_rate_2 REAL DEFAULT 0.0,"
        "khichai_rate_2 REAL DEFAULT 0.0,"
        "silai_rate_2 REAL DEFAULT 0.0,"
        "loading_rate_2 REAL DEFAULT 0.0,"
        "utrai_rate_3 REAL DEFAULT 0.0,"
        "jharai_rate_3 REAL DEFAULT 0.0,"
        "bharai_rate_3 REAL DEFAULT 0.0,"
        "tulai_rate_3 REAL DEFAULT 0.0,"
        "khichai_rate_3 REAL DEFAULT 0.0,"
        "silai_rate_3 REAL DEFAULT 0.0,"
        "loading_rate_3 REAL DEFAULT 0.0,"
        "legacy_code INTEGER"
        ");"
    );

    // 4. Paddy slips, unified (mandi OR manufacturer domain — paddy serves both
    // packages per spec §23.3; core firms skip them). S7 SSOT:
    // arrivals + procurement live in ONE table with slip_type; the two
    // legacy tables are copied over once (setting-guarded) and dropped.
    if (domainFlags.mandi || domainFlags.manufacturer) {
    executeNonQuery(
        "CREATE TABLE IF NOT EXISTS paddy_slips ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "slip_type TEXT NOT NULL DEFAULT 'arrival',"
        "slip_no TEXT,"
        "arrival_date TEXT NOT NULL,"
        "farmer_id INTEGER,"
        "farmer_name TEXT,"
        "variety TEXT NOT NULL,"
        "bag_count INTEGER,"
        "gross_weight_qtl REAL,"
        "moisture_pct REAL DEFAULT 14.0,"
        "moisture_deduction_qtl REAL DEFAULT 0.0,"
        "net_weight_qtl REAL,"
        "rate_per_qtl REAL,"
        "hamali_charges REAL DEFAULT 0.0,"
        "net_amount REAL,"
        "payment_status TEXT DEFAULT 'Unpaid',"
        "payment_mode TEXT,"
        "FOREIGN KEY (farmer_id) REFERENCES parties(id)"
        ");"
    );
    executeNonQuery(
        "CREATE UNIQUE INDEX IF NOT EXISTS uq_paddy_purchase_slip "
        "ON paddy_slips(slip_no) WHERE slip_type = 'purchase';"
    );
    } // domainFlags.mandi || domainFlags.manufacturer (paddy_slips)

    // S7 one-time unification: copy legacy tables into paddy_slips with
    // verified counts, then drop the olds. Idempotent via the marker;
    // runs inside ensureTablesExist's transaction (atomic).
    if (getSetting("schema_paddy_unified_v1", "") != "1"
        && FeatureFlags::tableExists("paddy_slips")) {
        bool unifiedOk = true;
        QVariant hasArrivals = executeScalar(
            "SELECT 1 FROM sqlite_master WHERE type='table' AND name='paddy_arrivals' LIMIT 1;");
        if (hasArrivals.isValid() && !hasArrivals.isNull()) {
            QVariant nArr = executeScalar("SELECT COUNT(*) FROM paddy_arrivals;");
            unifiedOk = executeNonQuery(
                "INSERT INTO paddy_slips (slip_type, slip_no, arrival_date, farmer_id, "
                "farmer_name, variety, bag_count, gross_weight_qtl, moisture_pct, "
                "moisture_deduction_qtl, net_weight_qtl, rate_per_qtl, hamali_charges, "
                "net_amount, payment_status) "
                "SELECT 'arrival', slip_no, arrival_date, farmer_id, farmer_name, paddy_variety, "
                "bag_count, gross_weight_qtl, moisture_pct, moisture_deduction_qtl, net_weight_qtl, "
                "rate_per_qtl, hamali_charges, net_amount, payment_status FROM paddy_arrivals;");
            if (unifiedOk) {
                QVariant nNew = executeScalar(
                    "SELECT COUNT(*) FROM paddy_slips WHERE slip_type = 'arrival';");
                unifiedOk = nNew.isValid() && !nNew.isNull()
                    && nNew.toLongLong() >= (nArr.isValid() && !nArr.isNull() ? nArr.toLongLong() : 0);
            }
            if (unifiedOk) unifiedOk = executeNonQuery("DROP TABLE paddy_arrivals;");
        }
        QVariant hasProc = executeScalar(
            "SELECT 1 FROM sqlite_master WHERE type='table' AND name='paddy_procurement' LIMIT 1;");
        if (unifiedOk && hasProc.isValid() && !hasProc.isNull()) {
            QVariant nProc = executeScalar("SELECT COUNT(*) FROM paddy_procurement;");
            unifiedOk = executeNonQuery(
                "INSERT INTO paddy_slips (slip_type, slip_no, arrival_date, farmer_id, "
                "farmer_name, variety, bag_count, gross_weight_qtl, moisture_pct, "
                "moisture_deduction_qtl, net_weight_qtl, rate_per_qtl, net_amount, "
                "payment_status, payment_mode) "
                "SELECT 'purchase', receipt_no, arrival_date, farmer_id, farmer_name, variety, "
                "bag_count, gross_weight_qtl, moisture_pct, deduction_qtl, net_weight_qtl, "
                "rate_per_qtl, total_amount, status, payment_mode FROM paddy_procurement;");
            if (unifiedOk) {
                QVariant nNew = executeScalar(
                    "SELECT COUNT(*) FROM paddy_slips WHERE slip_type = 'purchase';");
                unifiedOk = nNew.isValid() && !nNew.isNull()
                    && nNew.toLongLong() >= (nProc.isValid() && !nProc.isNull() ? nProc.toLongLong() : 0);
            }
            if (unifiedOk) unifiedOk = executeNonQuery("DROP TABLE paddy_procurement;");
        }
        if (unifiedOk) setSetting("schema_paddy_unified_v1", "1");
    }

    // 5. manufacturering Batches & Line Items (manufacturer domain; core firms skip them)
    if (domainFlags.manufacturer) {
    executeNonQuery(
        "CREATE TABLE IF NOT EXISTS manufacturing_batches ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "fy_id INTEGER,"
        "financial_year TEXT DEFAULT 'FY 2025-26',"
        "batch_no TEXT NOT NULL,"
        "batch_date TEXT NOT NULL,"
        "input_variety TEXT NOT NULL DEFAULT '',"
        "input_qtl REAL NOT NULL DEFAULT 0.0,"
        "output_qtl REAL NOT NULL DEFAULT 0.0,"
        "byproduct_qtl REAL NOT NULL DEFAULT 0.0,"
        "wastage_qtl REAL NOT NULL DEFAULT 0.0,"
        "yield_pct REAL NOT NULL DEFAULT 0.0,"
        "narration TEXT,"
        "FOREIGN KEY (fy_id) REFERENCES financial_years(id)"
        ");"
    );
    } // domainFlags.manufacturer (manufacturing_batches)

    if (domainFlags.manufacturer) {
    executeNonQuery(
        "CREATE TABLE IF NOT EXISTS manufacturing_voucher_items ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "batch_id INTEGER,"
        "batch_no TEXT NOT NULL,"
        "batch_date TEXT NOT NULL,"
        "row_no INTEGER DEFAULT 1,"
        "drcr TEXT NOT NULL,"
        "item_id INTEGER,"
        "item_code TEXT,"
        "item_name TEXT NOT NULL,"
        "percentage REAL DEFAULT 0.0,"
        "weight_qtl REAL NOT NULL DEFAULT 0.0,"
        "bags INTEGER DEFAULT 0,"
        "rate REAL DEFAULT 0.0,"
        "amount REAL DEFAULT 0.0,"
        "narration TEXT,"
        "FOREIGN KEY (batch_id) REFERENCES manufacturing_batches(id),"
        "FOREIGN KEY (item_id) REFERENCES stock_items(id)"
        ");"
    );
    } // domainFlags.manufacturer (manufacturering_voucher_items)

    // 6. Sales Invoices
    executeNonQuery(
        "CREATE TABLE IF NOT EXISTS sales_invoices ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "fy_id INTEGER,"
        "financial_year TEXT DEFAULT 'FY 2025-26',"
        "voucher_no TEXT,"
        "invoice_no TEXT NOT NULL,"
        "invoice_date TEXT NOT NULL,"
        "customer_id INTEGER,"
        "customer_name TEXT NOT NULL,"
        "gstin TEXT,"
        "item_id INTEGER,"
        "item_name TEXT NOT NULL,"
        "hsn_code TEXT,"
        "bag_count INTEGER DEFAULT 0,"
        "weight_qtl REAL DEFAULT 0.0,"
        "rate_per_qtl REAL DEFAULT 0.0,"
        "taxable_amount REAL DEFAULT 0.0,"
        "gst_pct REAL DEFAULT 5.0,"
        "cgst_amount REAL DEFAULT 0.0,"
        "sgst_amount REAL DEFAULT 0.0,"
        "igst_amount REAL DEFAULT 0.0,"
        "round_off REAL DEFAULT 0.0,"
        "gst_amount REAL DEFAULT 0.0,"
        "total_amount REAL DEFAULT 0.0,"
        "payment_mode TEXT DEFAULT 'Credit',"
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
        "driver_name TEXT,"
        "broker_name TEXT,"
        "shipping_address TEXT,"
        "po_no TEXT,"
        "distance INTEGER DEFAULT 0,"
        "irn_no TEXT,"
        "bill_time TEXT,"
        "sauda_date TEXT,"
        "grade TEXT,"
        "kanda_weight TEXT,"
        "transport TEXT,"
        "FOREIGN KEY (customer_id) REFERENCES parties(id),"
        "FOREIGN KEY (item_id) REFERENCES stock_items(id),"
        "FOREIGN KEY (fy_id) REFERENCES financial_years(id)"
        ");"
    );

    // 6b. Sales Invoice Items (Line items for multi-item invoices)
    executeNonQuery(
        "CREATE TABLE IF NOT EXISTS sales_invoice_items ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "invoice_id INTEGER NOT NULL,"
        "invoice_no TEXT,"
        "item_id INTEGER,"
        "item_name TEXT,"
        "grade TEXT DEFAULT '',"
        "bag_count INTEGER DEFAULT 0,"
        "packing TEXT,"
        "weight_qtl REAL DEFAULT 0.0,"
        "rate_per_qtl REAL DEFAULT 0.0,"
        "taxable_amount REAL DEFAULT 0.0,"
        "gst_pct REAL DEFAULT 5.0,"
        "total_amount REAL DEFAULT 0.0,"
        "FOREIGN KEY (invoice_id) REFERENCES sales_invoices(id) ON DELETE CASCADE"
        ");"
    );

    // 7. Purchase Invoices
    executeNonQuery(
        "CREATE TABLE IF NOT EXISTS purchase_invoices ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "fy_id INTEGER,"
        "financial_year TEXT DEFAULT 'FY 2025-26',"
        "voucher_no TEXT,"
        "invoice_no TEXT NOT NULL,"
        "invoice_date TEXT NOT NULL,"
        "supplier_id INTEGER,"
        "supplier_name TEXT NOT NULL,"
        "gstin TEXT,"
        "item_id INTEGER,"
        "item_name TEXT NOT NULL,"
        "hsn_code TEXT,"
        "bag_count INTEGER DEFAULT 0,"
        "weight_qtl REAL DEFAULT 0.0,"
        "rate_per_qtl REAL DEFAULT 0.0,"
        "taxable_amount REAL DEFAULT 0.0,"
        "gst_pct REAL DEFAULT 5.0,"
        "cgst_amount REAL DEFAULT 0.0,"
        "sgst_amount REAL DEFAULT 0.0,"
        "igst_amount REAL DEFAULT 0.0,"
        "round_off REAL DEFAULT 0.0,"
        "gst_amount REAL DEFAULT 0.0,"
        "total_amount REAL DEFAULT 0.0,"
        "payment_mode TEXT DEFAULT 'Credit',"
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
        "driver_name TEXT,"
        "broker_name TEXT,"
        "shipping_address TEXT,"
        "po_no TEXT,"
        "distance INTEGER DEFAULT 0,"
        "irn_no TEXT,"
        "bill_time TEXT,"
        "sauda_date TEXT,"
        "grade TEXT,"
        "kanda_weight TEXT,"
        "transport TEXT,"
        "FOREIGN KEY (supplier_id) REFERENCES parties(id),"
        "FOREIGN KEY (item_id) REFERENCES stock_items(id),"
        "FOREIGN KEY (fy_id) REFERENCES financial_years(id)"
        ");"
    );

    // 7b. Purchase Invoice Items (Line items for multi-item invoices)
    executeNonQuery(
        "CREATE TABLE IF NOT EXISTS purchase_invoice_items ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "invoice_id INTEGER NOT NULL,"
        "invoice_no TEXT,"
        "item_id INTEGER,"
        "item_name TEXT,"
        "grade TEXT DEFAULT '',"
        "bag_count INTEGER DEFAULT 0,"
        "packing TEXT,"
        "weight_qtl REAL DEFAULT 0.0,"
        "rate_per_qtl REAL DEFAULT 0.0,"
        "taxable_amount REAL DEFAULT 0.0,"
        "gst_pct REAL DEFAULT 5.0,"
        "total_amount REAL DEFAULT 0.0,"
        "FOREIGN KEY (invoice_id) REFERENCES purchase_invoices(id) ON DELETE CASCADE"
        ");"
    );

    // 7c. J-Form Mandi Procurement Vouchers (mandi domain; core firms skip them)
    if (domainFlags.mandi) {
    executeNonQuery(
        "CREATE TABLE IF NOT EXISTS jform_vouchers ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "fy_id INTEGER,"
        "financial_year TEXT DEFAULT 'FY 2025-26',"
        "voucher_no INTEGER NOT NULL,"
        "voucher_date TEXT NOT NULL,"
        "jform_no TEXT NOT NULL,"
        "zimidar_id INTEGER,"
        "zimidar_name TEXT NOT NULL,"
        "party_id INTEGER,"
        "party_name TEXT NOT NULL DEFAULT 'Self Purchase',"
        "auction_sale_status TEXT DEFAULT 'Zimidara Self Purchase',"
        "due_days INTEGER DEFAULT 0,"
        "vehicle_no TEXT,"
        "driver_name TEXT,"
        "gate_pass_no TEXT,"
        "eway_bill_no TEXT,"
        "bill_time TEXT,"
        "sauda_date TEXT,"
        "mandi_place TEXT,"
        "procurement_mode TEXT,"
        "lot_no TEXT,"
        "grade TEXT,"
        "transport TEXT,"
        "broker_name TEXT,"
        "challan_no TEXT,"
        "kanda_weight TEXT,"
        "total_bags INTEGER DEFAULT 0,"
        "total_weight REAL DEFAULT 0.0,"
        "goods_amount REAL DEFAULT 0.0,"
        "bonus_amount REAL DEFAULT 0.0,"
        "relief_amount REAL DEFAULT 0.0,"
        "subtotal_amount REAL DEFAULT 0.0,"
        "labour_amount REAL DEFAULT 0.0,"
        "round_off REAL DEFAULT 0.0,"
        "grand_total REAL DEFAULT 0.0,"
        "narration TEXT,"
        "created_at DATETIME DEFAULT CURRENT_TIMESTAMP,"
        "FOREIGN KEY (zimidar_id) REFERENCES parties(id),"
        "FOREIGN KEY (party_id) REFERENCES parties(id),"
        "FOREIGN KEY (fy_id) REFERENCES financial_years(id)"
        ");"
    );
    } // domainFlags.mandi (jform_vouchers)

    // 8. Transport, Weighbridge (Kanda) & e-Way Dispatches (transport domain)
    if (domainFlags.transport) {
    executeNonQuery(
        "CREATE TABLE IF NOT EXISTS transport_dispatches ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "fy_id INTEGER,"
        "financial_year TEXT DEFAULT 'FY 2025-26',"
        "dispatch_date TEXT NOT NULL,"
        "dispatch_time TEXT,"
        "slip_no TEXT,"
        "voucher_no TEXT,"
        "invoice_no TEXT,"
        "voucher_type TEXT DEFAULT 'Sale',"
        "party_id INTEGER,"
        "party_name TEXT NOT NULL,"
        "item_id INTEGER,"
        "item_name TEXT,"
        "grade TEXT,"
        "vehicle_no TEXT NOT NULL,"
        "driver_name TEXT,"
        "driver_phone TEXT,"
        "transport TEXT,"
        "transporter_gstin TEXT,"
        "gr_no TEXT,"
        "gr_date TEXT,"
        "destination TEXT,"
        "distance_km INTEGER DEFAULT 0,"
        "bag_count INTEGER DEFAULT 0,"
        "packing_kg REAL DEFAULT 50.0,"
        "gross_weight_qtl REAL DEFAULT 0.0,"
        "tare_weight_qtl REAL DEFAULT 0.0,"
        "bag_tare_kg REAL DEFAULT 0.0,"
        "net_weight_qtl REAL DEFAULT 0.0,"
        "freight_calc_type TEXT DEFAULT 'Per Qtl',"
        "freight_rate REAL DEFAULT 0.0,"
        "total_freight REAL DEFAULT 0.0,"
        "advance_freight REAL DEFAULT 0.0,"
        "balance_freight REAL DEFAULT 0.0,"
        "freight_payment_status TEXT DEFAULT 'Unpaid',"
        "eway_bill_no TEXT,"
        "irn_no TEXT,"
        "notes TEXT,"
        "created_at DATETIME DEFAULT CURRENT_TIMESTAMP,"
        "FOREIGN KEY (party_id) REFERENCES parties(id),"
        "FOREIGN KEY (fy_id) REFERENCES financial_years(id)"
        ");"
    );

    executeNonQuery("CREATE INDEX IF NOT EXISTS idx_transport_date ON transport_dispatches(dispatch_date);");
    executeNonQuery("CREATE INDEX IF NOT EXISTS idx_transport_vehicle ON transport_dispatches(vehicle_no);");
    executeNonQuery("CREATE INDEX IF NOT EXISTS idx_transport_invoice ON transport_dispatches(invoice_no);");
    } // domainFlags.transport (transport_dispatches)

    // 9. GST Debit Notes & Credit Notes (DebitCreditNotes)
    executeNonQuery(
        "CREATE TABLE IF NOT EXISTS debit_credit_notes ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "fy_id INTEGER,"
        "financial_year TEXT DEFAULT 'FY 2025-26',"
        "note_type TEXT NOT NULL,"
        "note_no TEXT NOT NULL,"
        "note_date TEXT NOT NULL,"
        "note_time TEXT,"
        "original_invoice_id INTEGER,"
        "original_invoice_no TEXT,"
        "original_invoice_date TEXT,"
        "original_invoice_type TEXT DEFAULT 'Sale',"
        "party_id INTEGER,"
        "party_name TEXT NOT NULL,"
        "party_gstin TEXT,"
        "state_code TEXT,"
        "is_interstate INTEGER DEFAULT 0,"
        "reason_code TEXT DEFAULT '01-Sales Return',"
        "adjustment_type TEXT DEFAULT 'Sales Return',"
        "item_name TEXT,"
        "hsn_code TEXT,"
        "total_bags INTEGER DEFAULT 0,"
        "total_weight_qtl REAL DEFAULT 0.0,"
        "taxable_amount REAL DEFAULT 0.0,"
        "gst_pct REAL DEFAULT 5.0,"
        "cgst_amount REAL DEFAULT 0.0,"
        "sgst_amount REAL DEFAULT 0.0,"
        "igst_amount REAL DEFAULT 0.0,"
        "total_tax_amount REAL DEFAULT 0.0,"
        "round_off REAL DEFAULT 0.0,"
        "grand_total REAL DEFAULT 0.0,"
        "narration TEXT,"
        "voucher_id INTEGER,"
        "created_at DATETIME DEFAULT CURRENT_TIMESTAMP,"
        "FOREIGN KEY (party_id) REFERENCES parties(id),"
        "FOREIGN KEY (fy_id) REFERENCES financial_years(id)"
        ");"
    );

    executeNonQuery(
        "CREATE TABLE IF NOT EXISTS debit_credit_note_items ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "note_id INTEGER NOT NULL,"
        "item_id INTEGER,"
        "item_name TEXT,"
        "hsn_code TEXT,"
        "unit TEXT DEFAULT 'QTL',"
        "bags INTEGER DEFAULT 0,"
        "weight_qtl REAL DEFAULT 0.0,"
        "rate REAL DEFAULT 0.0,"
        "taxable_amount REAL DEFAULT 0.0,"
        "gst_pct REAL DEFAULT 5.0,"
        "total_amount REAL DEFAULT 0.0,"
        "FOREIGN KEY (note_id) REFERENCES debit_credit_notes(id) ON DELETE CASCADE"
        ");"
    );

    executeNonQuery("CREATE INDEX IF NOT EXISTS idx_dcn_note_no ON debit_credit_notes(note_no);");
    executeNonQuery("CREATE INDEX IF NOT EXISTS idx_dcn_date ON debit_credit_notes(note_date);");
    executeNonQuery("CREATE INDEX IF NOT EXISTS idx_dcn_invoice ON debit_credit_notes(original_invoice_no);");
    executeNonQuery("CREATE INDEX IF NOT EXISTS idx_dcn_party ON debit_credit_notes(party_name);");

    // 7d. J-Form Voucher Line Items (mandi domain; core firms skip them)
    if (domainFlags.mandi) {
    executeNonQuery(
        "CREATE TABLE IF NOT EXISTS jform_voucher_items ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "voucher_id INTEGER NOT NULL,"
        "voucher_no INTEGER,"
        "item_id INTEGER,"
        "item_name TEXT NOT NULL,"
        "bags INTEGER DEFAULT 0,"
        "loose_weight REAL DEFAULT 0.0,"
        "packing REAL DEFAULT 0.500,"
        "weight REAL DEFAULT 0.0,"
        "rate REAL DEFAULT 0.0,"
        "amount REAL DEFAULT 0.0,"
        "FOREIGN KEY (voucher_id) REFERENCES jform_vouchers(id) ON DELETE CASCADE"
        ");"
    );
    } // domainFlags.mandi (jform_voucher_items)

    // 7d-2. I-Form Mandi Buyer Issue Vouchers (mandi domain; core firms skip them)
    if (domainFlags.mandi) {
    executeNonQuery(
        "CREATE TABLE IF NOT EXISTS iform_vouchers ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "fy_id INTEGER,"
        "financial_year TEXT DEFAULT 'FY 2025-26',"
        "voucher_no INTEGER NOT NULL,"
        "voucher_date TEXT NOT NULL,"
        "iform_no TEXT NOT NULL,"
        "buyer_id INTEGER,"
        "buyer_name TEXT NOT NULL,"
        "broker_name TEXT,"
        "transport TEXT,"
        "due_days INTEGER DEFAULT 0,"
        "vehicle_no TEXT,"
        "driver_name TEXT,"
        "gr_no TEXT,"
        "gate_pass_no TEXT,"
        "total_bags INTEGER DEFAULT 0,"
        "total_weight REAL DEFAULT 0.0,"
        "goods_amount REAL DEFAULT 0.0,"
        "dami_rate REAL DEFAULT 2.5,"
        "dami_amount REAL DEFAULT 0.0,"
        "mandi_fee_rate REAL DEFAULT 2.0,"
        "mandi_fee_amount REAL DEFAULT 0.0,"
        "hrdf_rate REAL DEFAULT 0.5,"
        "hrdf_amount REAL DEFAULT 0.0,"
        "labour_amount REAL DEFAULT 0.0,"
        "taxable_amount REAL DEFAULT 0.0,"
        "tax_amount REAL DEFAULT 0.0,"
        "round_off REAL DEFAULT 0.0,"
        "grand_total REAL DEFAULT 0.0,"
        "narration TEXT,"
        "lot_no TEXT,"
        "created_at DATETIME DEFAULT CURRENT_TIMESTAMP,"
        "FOREIGN KEY (buyer_id) REFERENCES parties(id),"
        "FOREIGN KEY (fy_id) REFERENCES financial_years(id)"
        ");"
    );
    } // domainFlags.mandi (iform_vouchers)

    // 7d-3. I-Form Voucher Line Items (mandi domain; core firms skip them)
    if (domainFlags.mandi) {
    executeNonQuery(
        "CREATE TABLE IF NOT EXISTS iform_voucher_items ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "voucher_id INTEGER NOT NULL,"
        "voucher_no INTEGER,"
        "item_id INTEGER,"
        "item_name TEXT NOT NULL,"
        "bags INTEGER DEFAULT 0,"
        "loose_weight REAL DEFAULT 0.0,"
        "packing REAL DEFAULT 0.500,"
        "weight REAL DEFAULT 0.0,"
        "rate REAL DEFAULT 0.0,"
        "amount REAL DEFAULT 0.0,"
        "dami_rate REAL DEFAULT 2.5,"
        "dami_amount REAL DEFAULT 0.0,"
        "mandi_fee_rate REAL DEFAULT 2.0,"
        "mandi_fee_amount REAL DEFAULT 0.0,"
        "hrdf_rate REAL DEFAULT 0.5,"
        "hrdf_amount REAL DEFAULT 0.0,"
        "labour_amount REAL DEFAULT 0.0,"
        "tax_rate REAL DEFAULT 0.0,"
        "tax_amount REAL DEFAULT 0.0,"
        "FOREIGN KEY (voucher_id) REFERENCES iform_vouchers(id) ON DELETE CASCADE"
        ");"
    );
    } // domainFlags.mandi (iform_voucher_items)

    // 7e. TDS Vouchers. trans_type / challan_voucher_date /
    // challan_voucher_number are the firm's LEGACY Jet TDSDeductions
    // register fields, carried verbatim by migration so the exporter can
    // round-trip them (C3 TDS register-fidelity slice). They are distinct
    // from challan_id, which links the app's own ITNS-281 deposit record in
    // tax_deposit_challans.
    executeNonQuery(
        "CREATE TABLE IF NOT EXISTS tds_vouchers ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "fy_id INTEGER,"
        "financial_year TEXT DEFAULT 'FY 2025-26',"
        "voucher_no INTEGER NOT NULL,"
        "voucher_date TEXT NOT NULL,"
        "day_of_week TEXT,"
        "post_in_books INTEGER DEFAULT 1,"
        "tds_type TEXT NOT NULL DEFAULT 'RENT',"
        "ledger_id INTEGER,"
        "ledger_name TEXT NOT NULL,"
        "income_amount REAL DEFAULT 0.0,"
        "previous_amount REAL DEFAULT 0.0,"
        "total_for_tds REAL DEFAULT 0.0,"
        "narration TEXT,"
        "trans_type TEXT DEFAULT 'Jrnl',"
        "challan_voucher_date TEXT,"
        "challan_voucher_number INTEGER DEFAULT 0,"
        "rate_tds REAL DEFAULT 0.0,"
        "tax_amount_tds REAL DEFAULT 0.0,"
        "rate_surcharge REAL DEFAULT 0.0,"
        "tax_amount_surcharge REAL DEFAULT 0.0,"
        "rate_cess REAL DEFAULT 0.0,"
        "tax_amount_cess REAL DEFAULT 0.0,"
        "use_rounded_total INTEGER DEFAULT 1,"
        "total_tax_rate REAL DEFAULT 0.0,"
        "total_tax_amount REAL DEFAULT 0.0,"
        "net_amount REAL DEFAULT 0.0,"
        "non_deduction_reason TEXT,"
        "exp_ledger_id INTEGER,"
        "exp_ledger_name TEXT,"
        "tds_ledger_id INTEGER,"
        "tds_ledger_name TEXT,"
        "created_at DATETIME DEFAULT CURRENT_TIMESTAMP,"
        "FOREIGN KEY (ledger_id) REFERENCES parties(id),"
        "FOREIGN KEY (exp_ledger_id) REFERENCES parties(id),"
        "FOREIGN KEY (tds_ledger_id) REFERENCES parties(id),"
        "FOREIGN KEY (fy_id) REFERENCES financial_years(id)"
        ");"
    );

    // 7f. Tax Deposit Challans (ITNS 281 for TDS and TCS)
    executeNonQuery(
        "CREATE TABLE IF NOT EXISTS tax_deposit_challans ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "fy_id INTEGER NOT NULL,"
        "tax_type TEXT NOT NULL CHECK(tax_type IN ('TDS', 'TCS')),"
        "challan_no TEXT NOT NULL,"
        "challan_date TEXT NOT NULL,"
        "post_in_books INTEGER DEFAULT 1,"
        "period_from TEXT NOT NULL,"
        "period_to TEXT NOT NULL,"
        "basic_tax REAL DEFAULT 0.0,"
        "surcharge REAL DEFAULT 0.0,"
        "cess REAL DEFAULT 0.0,"
        "total_tax REAL DEFAULT 0.0,"
        "interest_amount REAL DEFAULT 0.0,"
        "interest_ledger_id INTEGER,"
        "penalty_amount REAL DEFAULT 0.0,"
        "penalty_ledger_id INTEGER,"
        "other_amount REAL DEFAULT 0.0,"
        "other_ledger_id INTEGER,"
        "total_challan_amount REAL DEFAULT 0.0,"
        "bank_ledger_id INTEGER NOT NULL,"
        "cheque_no TEXT,"
        "cheque_date TEXT,"
        "bsr_code TEXT,"
        "minor_head TEXT DEFAULT '200',"
        "major_head TEXT DEFAULT '0021',"
        "voucher_id INTEGER,"
        "narration TEXT,"
        "created_at DATETIME DEFAULT CURRENT_TIMESTAMP,"
        "FOREIGN KEY (fy_id) REFERENCES financial_years(id),"
        "FOREIGN KEY (bank_ledger_id) REFERENCES parties(id),"
        "FOREIGN KEY (interest_ledger_id) REFERENCES parties(id),"
        "FOREIGN KEY (penalty_ledger_id) REFERENCES parties(id),"
        "FOREIGN KEY (other_ledger_id) REFERENCES parties(id)"
        ");"
    );

    // 7g. Tax Deposit Challan Items (vouchers linked to challan)
    executeNonQuery(
        "CREATE TABLE IF NOT EXISTS tax_deposit_challan_items ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "challan_id INTEGER NOT NULL,"
        "voucher_type TEXT NOT NULL,"
        "source_voucher_id INTEGER NOT NULL,"
        "tax_amount REAL NOT NULL,"
        "surcharge_amount REAL DEFAULT 0.0,"
        "cess_amount REAL DEFAULT 0.0,"
        "total_tax_amount REAL NOT NULL,"
        "FOREIGN KEY (challan_id) REFERENCES tax_deposit_challans(id) ON DELETE CASCADE"
        ");"
    );

    // 7h. TCS Receipt Vouchers
    executeNonQuery(
        "CREATE TABLE IF NOT EXISTS tcs_receipt_vouchers ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "fy_id INTEGER NOT NULL,"
        "receipt_no INTEGER NOT NULL,"
        "receipt_date TEXT NOT NULL,"
        "receipt_type TEXT NOT NULL DEFAULT 'BANK',"
        "post_in_books INTEGER DEFAULT 1,"
        "party_id INTEGER NOT NULL,"
        "bank_ledger_id INTEGER NOT NULL,"
        "bank_amount REAL DEFAULT 0.0,"
        "without_tcs_amount REAL DEFAULT 0.0,"
        "tcs_rate REAL DEFAULT 0.10,"
        "tcs_amount REAL DEFAULT 0.0,"
        "tcs_received INTEGER DEFAULT 1,"
        "net_bank_receipt REAL DEFAULT 0.0,"
        "interest_received REAL DEFAULT 0.0,"
        "interest_ledger_id INTEGER,"
        "discount_allowed REAL DEFAULT 0.0,"
        "discount_ledger_id INTEGER,"
        "other_amount REAL DEFAULT 0.0,"
        "other_ledger_id INTEGER,"
        "net_credit_to_party REAL DEFAULT 0.0,"
        "tcs_payable_ledger_id INTEGER,"
        "challan_id INTEGER,"
        "is_deposited INTEGER DEFAULT 0,"
        "voucher_id INTEGER,"
        "narration TEXT,"
        "created_at DATETIME DEFAULT CURRENT_TIMESTAMP,"
        "FOREIGN KEY (fy_id) REFERENCES financial_years(id),"
        "FOREIGN KEY (party_id) REFERENCES parties(id),"
        "FOREIGN KEY (bank_ledger_id) REFERENCES parties(id),"
        "FOREIGN KEY (interest_ledger_id) REFERENCES parties(id),"
        "FOREIGN KEY (discount_ledger_id) REFERENCES parties(id),"
        "FOREIGN KEY (other_ledger_id) REFERENCES parties(id),"
        "FOREIGN KEY (tcs_payable_ledger_id) REFERENCES parties(id),"
        "FOREIGN KEY (challan_id) REFERENCES tax_deposit_challans(id)"
        ");"
    );

    // 7i. Advance Payment Vouchers U/S 194-Q
    executeNonQuery(
        "CREATE TABLE IF NOT EXISTS advance_payments_194q ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "fy_id INTEGER NOT NULL,"
        "voucher_no INTEGER NOT NULL,"
        "voucher_date TEXT NOT NULL,"
        "post_in_books INTEGER DEFAULT 1,"
        "supplier_id INTEGER NOT NULL,"
        "bank_ledger_id INTEGER NOT NULL,"
        "gross_advance_amount REAL DEFAULT 0.0,"
        "tds_rate REAL DEFAULT 0.10,"
        "tds_amount REAL DEFAULT 0.0,"
        "net_payment_amount REAL DEFAULT 0.0,"
        "tds_payable_ledger_id INTEGER NOT NULL,"
        "cheque_no TEXT,"
        "cheque_date TEXT,"
        "challan_id INTEGER,"
        "is_deposited INTEGER DEFAULT 0,"
        "voucher_id INTEGER,"
        "narration TEXT,"
        "created_at DATETIME DEFAULT CURRENT_TIMESTAMP,"
        "FOREIGN KEY (fy_id) REFERENCES financial_years(id),"
        "FOREIGN KEY (supplier_id) REFERENCES parties(id),"
        "FOREIGN KEY (bank_ledger_id) REFERENCES parties(id),"
        "FOREIGN KEY (tds_payable_ledger_id) REFERENCES parties(id),"
        "FOREIGN KEY (challan_id) REFERENCES tax_deposit_challans(id)"
        ");"
    );

    // 7j. Received Form-16A
    executeNonQuery(
        "CREATE TABLE IF NOT EXISTS received_forms_16a ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "fy_id INTEGER NOT NULL,"
        "certificate_no TEXT NOT NULL,"
        "receipt_date TEXT NOT NULL,"
        "party_id INTEGER NOT NULL,"
        "pan_of_deductor TEXT,"
        "quarter TEXT NOT NULL,"
        "gross_amount REAL DEFAULT 0.0,"
        "tds_amount REAL DEFAULT 0.0,"
        "tds_receivable_ledger_id INTEGER,"
        "matched_with_26as INTEGER DEFAULT 0,"
        "remarks TEXT,"
        "created_at DATETIME DEFAULT CURRENT_TIMESTAMP,"
        "FOREIGN KEY (fy_id) REFERENCES financial_years(id),"
        "FOREIGN KEY (party_id) REFERENCES parties(id),"
        "FOREIGN KEY (tds_receivable_ledger_id) REFERENCES parties(id)"
        ");"
    );

    // 7k. TDS Return Acknowledgements
    executeNonQuery(
        "CREATE TABLE IF NOT EXISTS tds_acknowledgements ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "fy_id INTEGER NOT NULL,"
        "form_type TEXT NOT NULL,"
        "quarter TEXT NOT NULL,"
        "ack_number TEXT NOT NULL,"
        "filing_date TEXT NOT NULL,"
        "token_number TEXT,"
        "total_deductees INTEGER DEFAULT 0,"
        "total_tax_deposited REAL DEFAULT 0.0,"
        "remarks TEXT,"
        "created_at DATETIME DEFAULT CURRENT_TIMESTAMP,"
        "FOREIGN KEY (fy_id) REFERENCES financial_years(id)"
        ");"
    );

    // 8. Vouchers Table
    executeNonQuery(
        "CREATE TABLE IF NOT EXISTS vouchers ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "fy_id INTEGER,"
        "financial_year TEXT DEFAULT 'FY 2025-26',"
        "voucher_no TEXT NOT NULL,"
        "voucher_date TEXT NOT NULL,"
        "voucher_type TEXT NOT NULL,"
        "legacy_type TEXT,"
        "ledger_id INTEGER,"
        "party_id INTEGER,"
        "party_name TEXT NOT NULL,"
        "account_type TEXT NOT NULL,"
        "amount REAL NOT NULL,"
        "narration TEXT,"
        "instrument_no TEXT,"
        "instrument_date TEXT,"
        "bank_date TEXT,"
        "taxable_amount REAL DEFAULT 0.0,"
        "gst_pct REAL DEFAULT 0.0,"
        "cgst_amount REAL DEFAULT 0.0,"
        "sgst_amount REAL DEFAULT 0.0,"
        "igst_amount REAL DEFAULT 0.0,"
        "cess_amount REAL DEFAULT 0.0,"
        "round_off REAL DEFAULT 0.0,"
        "vehicle_no TEXT,"
        "gr_no TEXT,"
        "driver_name TEXT,"
        "eway_bill_no TEXT,"
        "broker_name TEXT,"
        "farmer_name TEXT,"
        "sauda_date TEXT,"
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
        "due_days INTEGER DEFAULT 0,"
        "market_type TEXT,"
        "tax_status TEXT,"
        "place_of_supply TEXT,"
        "challan_no TEXT,"
        "FOREIGN KEY (ledger_id) REFERENCES parties(id),"
        "FOREIGN KEY (fy_id) REFERENCES financial_years(id)"
        ");"
    );

    // 8b. Unified Double-Entry Ledger Transactions Table
    executeNonQuery(
        "CREATE TABLE IF NOT EXISTS transactions ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "fy_id INTEGER,"
        "financial_year TEXT,"
        "voucher_no TEXT NOT NULL,"
        "voucher_date TEXT NOT NULL,"
        "voucher_type TEXT NOT NULL,"
        "trans_type TEXT,"
        "account_code INTEGER,"
        "party_id INTEGER,"
        "party_name TEXT NOT NULL,"
        "opposing_account TEXT,"
        "dr_cr TEXT NOT NULL,"
        "amount REAL NOT NULL,"
        "invoice_no TEXT,"
        "narration TEXT,"
        "sauda_date TEXT,"
        "bank_date TEXT,"
        "broker_name TEXT,"
        "vehicle_no TEXT,"
        "gr_no TEXT,"
        "taxable_amount REAL DEFAULT 0.0,"
        "tds_amount REAL DEFAULT 0.0,"
        "tds_rate REAL DEFAULT 0.0,"
        "gst_pct REAL DEFAULT 0.0,"
        "row_no INTEGER DEFAULT 1,"
        "due_days INTEGER DEFAULT 0,"
        "place_of_supply TEXT,"
        "market_type TEXT,"
        "FOREIGN KEY (party_id) REFERENCES parties(id),"
        "FOREIGN KEY (fy_id) REFERENCES financial_years(id)"
        ");"
    );
    executeNonQuery("CREATE INDEX IF NOT EXISTS idx_transactions_party_name ON transactions(party_name, voucher_date);");
    executeNonQuery("CREATE INDEX IF NOT EXISTS idx_transactions_date ON transactions(voucher_date, id);");
    executeNonQuery("CREATE INDEX IF NOT EXISTS idx_transactions_party_id_date ON transactions(party_id, voucher_date);");
    executeNonQuery("CREATE INDEX IF NOT EXISTS idx_transactions_account_code ON transactions(account_code, voucher_date);");
    executeNonQuery("CREATE INDEX IF NOT EXISTS idx_transactions_voucher ON transactions(voucher_no, voucher_type, voucher_date);");
    executeNonQuery("CREATE INDEX IF NOT EXISTS idx_transactions_voucher_key ON transactions(trans_type, voucher_no, voucher_date);");
    executeNonQuery("CREATE INDEX IF NOT EXISTS idx_transactions_fy ON transactions(financial_year);");

    // 9. Account Groups Table
    executeNonQuery(
        "CREATE TABLE IF NOT EXISTS account_groups ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "name TEXT UNIQUE NOT NULL,"
        "parent_group_name TEXT DEFAULT 'Primary',"
        "nature TEXT NOT NULL DEFAULT 'Assets',"
        "description TEXT,"
        "extract_in_balance_sheet INTEGER DEFAULT 1,"
        "is_system INTEGER DEFAULT 0,"
        "code1st INTEGER DEFAULT 0,"
        "code2nd INTEGER DEFAULT 0,"
        "code3rd INTEGER DEFAULT 0,"
        "code4th INTEGER DEFAULT 0,"
        "standard_root INTEGER DEFAULT 0"
        ");"
    );

    // Seed the generic Tally-compatible chart (fresh firms only: seed-if-empty).
    // Same-meaning groups keep their Bahi-Khata codes AND names so every
    // code-driven consumer (classifier, P&L/BS engines, migrators, salary
    // discovery) works unchanged; domain rows (Trading 18-20/24, Commission
    // 21, Manufacturing 22-23, Mandi 31-34, Employees 35, Isht Dev -1) are
    // dropped here and restored per-package by ensureDomainGroups.
    // New-meaning groups take codes >= 100 (never collide with legacy sync).
    // NOTE: "Cash-In-Hand" keeps its Bahi spelling on purpose — the cash
    // report, classifier and migrators all match it; renaming would churn
    // three subsystems for zero behavior gain.
    QVariant groupCount = executeScalar("SELECT COUNT(*) FROM account_groups;");
    if (!groupCount.isValid() || groupCount.toLongLong() == 0) {
        struct GenericGroupSeed { int c1, c2, c3, c4; const char* name; const char* parent; const char* nature; int extractBs; };
        static const GenericGroupSeed defaults[] = {
            {0, 0, 0, 0, "Primary", "Primary", "Assets", 1},
            {1, 0, 0, 0, "Capital A/c", "Primary", "Liabilities", 1},
            {2, 0, 0, 0, "Current Assets", "Primary", "Assets", 1},
            {3, 2, 0, 0, "Bank(s) A/c", "Current Assets", "Assets", 1},
            {4, 2, 0, 0, "Cash-In-Hand", "Current Assets", "Assets", 1},
            {6, 2, 0, 0, "Loan & Advances (Assets)", "Current Assets", "Assets", 1},
            {7, 2, 0, 0, "Stock-In-Hand", "Current Assets", "Assets", 0},
            {8, 2, 0, 0, "Sundry Debtors", "Current Assets", "Assets", 1},
            {26, 2, 0, 0, "Deposit (Assets)", "Current Assets", "Assets", 1},
            {30, 2, 0, 0, "Security A/c", "Current Assets", "Assets", 1},
            {9, 0, 0, 0, "Current Liabilities", "Primary", "Liabilities", 1},
            {10, 9, 0, 0, "Duties & Taxes", "Current Liabilities", "Liabilities", 1},
            {11, 9, 0, 0, "Sundry Creditors", "Current Liabilities", "Liabilities", 1},
            {12, 9, 0, 0, "Provisions", "Current Liabilities", "Liabilities", 1},
            {13, 9, 0, 0, "Loans (Liability)", "Current Liabilities", "Liabilities", 1},
            {28, 13, 9, 0, "Secured Loans", "Loans (Liability)", "Liabilities", 1},
            {29, 13, 9, 0, "Unsecured Loans", "Loans (Liability)", "Liabilities", 1},
            {14, 0, 0, 0, "Fixed Assets", "Primary", "Assets", 1},
            {15, 0, 0, 0, "Profit & Loss", "Primary", "Liabilities", 1},
            {16, 15, 0, 0, "Income A/c", "Profit & Loss", "Income", 1},
            {100, 16, 15, 0, "Direct Income", "Income A/c", "Income", 1},
            {101, 16, 15, 0, "Indirect Income", "Income A/c", "Income", 1},
            {17, 15, 0, 0, "Expenditure A/c", "Profit & Loss", "Expense", 1},
            {200, 17, 15, 0, "Direct Expenses", "Expenditure A/c", "Expense", 1},
            {201, 17, 15, 0, "Indirect Expenses", "Expenditure A/c", "Expense", 1},
            {25, 0, 0, 0, "Suspense A/c", "Primary", "Liabilities", 1},
            {27, 0, 0, 0, "Branches / Divisions", "Primary", "Liabilities", 1},
            {102, 0, 0, 0, "Misc. Expenses (Asset)", "Primary", "Assets", 1},
        };
        for (const auto& g : defaults) {
            executeNonQuery(
                "INSERT INTO account_groups (name, parent_group_name, nature, description, extract_in_balance_sheet, is_system, code1st, code2nd, code3rd, code4th) "
                "VALUES (?, ?, ?, 'Generic Chart v1', ?, 1, ?, ?, ?, ?);",
                {g.name, g.parent, g.nature, g.extractBs, g.c1, g.c2, g.c3, g.c4}
            );
        }
    }

    // 10. Inventory Levels
    executeNonQuery(
        "CREATE TABLE IF NOT EXISTS inventory ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "item_code TEXT UNIQUE NOT NULL,"
        "item_name TEXT NOT NULL,"
        "category TEXT NOT NULL,"
        "current_stock_qtl REAL NOT NULL DEFAULT 0.0,"
        "reorder_level_qtl REAL DEFAULT 50.0,"
        "unit TEXT DEFAULT 'Qtl',"
        "sale_rate REAL DEFAULT 0.0,"
        "gst_rate TEXT DEFAULT '5%',"
        "packing_kg INTEGER DEFAULT 50"
        ");"
    );

    // 11. Custom Closing Stocks
    executeNonQuery(
        "CREATE TABLE IF NOT EXISTS custom_closing_stocks ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "fy_id INTEGER,"
        "financial_year TEXT NOT NULL,"
        "closing_date TEXT NOT NULL,"
        "item_id INTEGER,"
        "item_code TEXT NOT NULL,"
        "item_name TEXT NOT NULL,"
        "bags INTEGER DEFAULT 0,"
        "weight_qtl REAL DEFAULT 0.0,"
        "rate REAL DEFAULT 0.0,"
        "amount REAL DEFAULT 0.0,"
        "FOREIGN KEY (item_id) REFERENCES stock_items(id),"
        "FOREIGN KEY (fy_id) REFERENCES financial_years(id)"
        ");"
    );

    // 12. Stock Transactions
    executeNonQuery(
        "CREATE TABLE IF NOT EXISTS stock_transactions ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "fy_id INTEGER,"
        "financial_year TEXT,"
        "voucher_no TEXT,"
        "voucher_date TEXT NOT NULL,"
        "trans_type TEXT NOT NULL,"
        "voucher_type TEXT,"
        "party_id INTEGER,"
        "party_name TEXT,"
        "bill_no TEXT,"
        "item_id INTEGER,"
        "item_code TEXT NOT NULL,"
        "item_name TEXT NOT NULL,"
        "bags INTEGER DEFAULT 0,"
        "packing REAL DEFAULT 0.0,"
        "weight_qtl REAL NOT NULL DEFAULT 0.0,"
        "rate REAL DEFAULT 0.0,"
        "amount REAL DEFAULT 0.0,"
        "taxable_amount REAL DEFAULT 0.0,"
        "tax REAL DEFAULT 0.0,"
        "tax_type TEXT,"
        "narration TEXT,"
        "row_no INTEGER DEFAULT 1"
        ");"
    );

    // 13. Bank Narration Aliases & Mapping Memory
    executeNonQuery(
        "CREATE TABLE IF NOT EXISTS bank_narration_aliases ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "bank_code TEXT DEFAULT 'CNRB',"
        "narration_pattern TEXT UNIQUE COLLATE NOCASE,"
        "mapped_party_name TEXT NOT NULL,"
        "voucher_type TEXT,"
        "created_at TEXT"
        ");"
    );

    // 14. Bardana (Gunny Bags & Packaging) Transactions (bardana domain)
    if (domainFlags.bardana) {
    executeNonQuery(
        "CREATE TABLE IF NOT EXISTS bardana_transactions ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "fy_id INTEGER,"
        "financial_year TEXT DEFAULT 'FY 2025-26',"
        "voucher_no TEXT NOT NULL,"
        "voucher_date TEXT NOT NULL,"
        "vch_type TEXT NOT NULL,"
        "party_id INTEGER,"
        "party_name TEXT NOT NULL,"
        "bardana_type TEXT NOT NULL,"
        "godown_name TEXT DEFAULT 'Main Godown',"
        "dr_cr TEXT NOT NULL DEFAULT 'Dr',"
        "qty INTEGER NOT NULL DEFAULT 0,"
        "rate REAL DEFAULT 0.0,"
        "amount REAL DEFAULT 0.0,"
        "vehicle_no TEXT,"
        "bill_no TEXT,"
        "narration TEXT,"
        "created_at DATETIME DEFAULT CURRENT_TIMESTAMP,"
        "FOREIGN KEY (party_id) REFERENCES parties(id),"
        "FOREIGN KEY (fy_id) REFERENCES financial_years(id)"
        ");"
    );
    executeNonQuery("CREATE INDEX IF NOT EXISTS idx_bardana_party ON bardana_transactions(party_name, voucher_date);");
    executeNonQuery("CREATE INDEX IF NOT EXISTS idx_bardana_type ON bardana_transactions(bardana_type, godown_name);");
    } // domainFlags.bardana (bardana_transactions)

    // 15. Gate Inward / Outward Register (transport domain; core firms skip it)
    if (domainFlags.transport) {
    executeNonQuery(
        "CREATE TABLE IF NOT EXISTS gate_register ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "fy_id INTEGER,"
        "financial_year TEXT DEFAULT 'FY 2025-26',"
        "gate_pass_no TEXT UNIQUE NOT NULL,"
        "entry_date TEXT NOT NULL,"
        "entry_time TEXT,"
        "exit_time TEXT,"
        "direction TEXT NOT NULL DEFAULT 'INWARD',"
        "purpose TEXT DEFAULT 'Paddy Purchase',"
        "vehicle_no TEXT NOT NULL,"
        "driver_name TEXT,"
        "driver_phone TEXT,"
        "transport TEXT,"
        "party_id INTEGER,"
        "party_name TEXT NOT NULL,"
        "commodity TEXT DEFAULT 'Paddy',"
        "bag_count INTEGER DEFAULT 0,"
        "gross_weight_qtl REAL DEFAULT 0.0,"
        "tare_weight_qtl REAL DEFAULT 0.0,"
        "net_weight_qtl REAL DEFAULT 0.0,"
        "status TEXT DEFAULT 'AT_GATE',"
        "weighbridge_slip_no TEXT,"
        "linked_voucher_no TEXT,"
        "remarks TEXT,"
        "created_at DATETIME DEFAULT CURRENT_TIMESTAMP,"
        "FOREIGN KEY (party_id) REFERENCES parties(id),"
        "FOREIGN KEY (fy_id) REFERENCES financial_years(id)"
        ");"
    );
    executeNonQuery("CREATE INDEX IF NOT EXISTS idx_gate_reg_date ON gate_register(entry_date, direction);");
    executeNonQuery("CREATE INDEX IF NOT EXISTS idx_gate_reg_vehicle ON gate_register(vehicle_no);");
    } // domainFlags.transport (gate_register)

    // 16. Sauda (Forward Broker Contracts) (mandi domain; core firms skip them)
    if (domainFlags.mandi) {
    executeNonQuery(
        "CREATE TABLE IF NOT EXISTS sauda_contracts ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "fy_id INTEGER,"
        "financial_year TEXT DEFAULT 'FY 2025-26',"
        "sauda_no TEXT UNIQUE NOT NULL,"
        "sauda_date TEXT NOT NULL,"
        "sauda_type TEXT NOT NULL DEFAULT 'SALE',"
        "party_id INTEGER,"
        "party_name TEXT NOT NULL,"
        "broker_id INTEGER,"
        "broker_name TEXT,"
        "item_id INTEGER,"
        "item_name TEXT NOT NULL,"
        "grade TEXT,"
        "contracted_bags INTEGER DEFAULT 0,"
        "contracted_weight_qtl REAL NOT NULL DEFAULT 0.0,"
        "rate_per_qtl REAL NOT NULL DEFAULT 0.0,"
        "dalali_rate_per_qtl REAL DEFAULT 0.0,"
        "dalali_pct REAL DEFAULT 0.0,"
        "delivery_from TEXT,"
        "delivery_to TEXT,"
        "payment_terms TEXT,"
        "condition_notes TEXT,"
        "fulfilled_weight_qtl REAL DEFAULT 0.0,"
        "fulfilled_bags INTEGER DEFAULT 0,"
        "status TEXT DEFAULT 'PENDING',"
        "created_at DATETIME DEFAULT CURRENT_TIMESTAMP,"
        "FOREIGN KEY (party_id) REFERENCES parties(id),"
        "FOREIGN KEY (broker_id) REFERENCES parties(id),"
        "FOREIGN KEY (item_id) REFERENCES stock_items(id),"
        "FOREIGN KEY (fy_id) REFERENCES financial_years(id)"
        ");"
    );
    executeNonQuery("CREATE INDEX IF NOT EXISTS idx_sauda_party ON sauda_contracts(party_name, status);");
    executeNonQuery("CREATE INDEX IF NOT EXISTS idx_sauda_broker ON sauda_contracts(broker_name, sauda_date);");
    } // domainFlags.mandi (sauda_contracts)

    // 17. Dalali / Brokerage Settlements (mandi domain; core firms skip them)
    if (domainFlags.mandi) {
    executeNonQuery(
        "CREATE TABLE IF NOT EXISTS dalali_settlements ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "fy_id INTEGER,"
        "financial_year TEXT DEFAULT 'FY 2025-26',"
        "settlement_no TEXT UNIQUE NOT NULL,"
        "settlement_date TEXT NOT NULL,"
        "broker_id INTEGER NOT NULL,"
        "broker_name TEXT NOT NULL,"
        "sauda_id INTEGER,"
        "sauda_no TEXT,"
        "voucher_type TEXT DEFAULT 'SALE',"
        "voucher_no TEXT,"
        "invoice_no TEXT,"
        "party_name TEXT,"
        "item_name TEXT,"
        "weight_qtl REAL DEFAULT 0.0,"
        "rate_per_qtl REAL DEFAULT 0.0,"
        "dalali_rate REAL DEFAULT 0.0,"
        "dalali_amount REAL NOT NULL DEFAULT 0.0,"
        "tds_pct REAL DEFAULT 5.0,"
        "tds_amount REAL DEFAULT 0.0,"
        "net_dalali_payable REAL NOT NULL DEFAULT 0.0,"
        "is_posted_to_jv INTEGER DEFAULT 0,"
        "journal_voucher_no TEXT,"
        "narration TEXT,"
        "created_at DATETIME DEFAULT CURRENT_TIMESTAMP,"
        "FOREIGN KEY (broker_id) REFERENCES parties(id),"
        "FOREIGN KEY (sauda_id) REFERENCES sauda_contracts(id),"
        "FOREIGN KEY (fy_id) REFERENCES financial_years(id)"
        ");"
    );
    executeNonQuery("CREATE INDEX IF NOT EXISTS idx_dalali_broker ON dalali_settlements(broker_name, settlement_date);");
    } // domainFlags.mandi (dalali_settlements)

    QMap<QString, QSet<QString>> tableColsCache;
    auto getCols = [this, &tableColsCache](const QString& table) -> const QSet<QString>& {
        if (!tableColsCache.contains(table)) {
            QSet<QString> cols;
            QVariantList list = executeQuery(QString("PRAGMA table_info(%1);").arg(table));
            for (const auto& c : list) {
                cols.insert(c.toMap().value("name").toString().toLower());
            }
            tableColsCache[table] = cols;
        }
        return tableColsCache[table];
    };

    auto addColumnIfNotExists = [this, &getCols, &tableColsCache](const QString& table, const QString& column, const QString& type) {
        // Existence guard: on generic DBs the domain table may not
        // exist at all — skip instead of erroring. Flag checks stay out of
        // heal/alter paths by design; presence is the only question here.
        QVariant texists = executeScalar("SELECT 1 FROM sqlite_master WHERE type='table' AND name=? LIMIT 1;", {table});
        if (!texists.isValid() || texists.isNull()) return;
        if (!getCols(table).contains(column.toLower())) {
            executeNonQuery(QString("ALTER TABLE %1 ADD COLUMN %2 %3;").arg(table, column, type));
            tableColsCache[table].insert(column.toLower());
        }
    };

    // F2 status lifecycle (open/soft_closed/locked). Pure addition: existing
    // rows default 'open'; legacy locked rows backfill below. is_locked stays
    // as the mirror (migrators/tests/reports read it; transitions write both).
    addColumnIfNotExists("financial_years", "status", "TEXT DEFAULT 'open'");
    executeNonQuery("UPDATE financial_years SET status = 'locked' "
                    "WHERE is_locked != 0 AND (status IS NULL OR status = '' OR status = 'open');");

    // Ensure columns exist on existing databases
    addColumnIfNotExists("sales_invoices", "market_type", "TEXT DEFAULT 'Market Type (With Stock)'");
    addColumnIfNotExists("sales_invoices", "due_days", "INTEGER DEFAULT 0");
    addColumnIfNotExists("sales_invoices", "tax_status", "TEXT DEFAULT 'GST / Exempt'");
    addColumnIfNotExists("sales_invoices", "challan_no", "TEXT");
    addColumnIfNotExists("sales_invoices", "freight_charges", "REAL DEFAULT 0.0");
    addColumnIfNotExists("sales_invoices", "tcs_amount", "REAL DEFAULT 0.0");
    addColumnIfNotExists("sales_invoices", "tcs_rate", "REAL DEFAULT 0.0");
    addColumnIfNotExists("sales_invoices", "place_of_supply", "TEXT");
    addColumnIfNotExists("sales_invoice_items", "grade", "TEXT DEFAULT ''");
    
    QVariant unlinkedSalesGrades = executeScalar("SELECT 1 FROM sales_invoice_items WHERE (grade IS NULL OR grade = '') AND invoice_id IN (SELECT id FROM sales_invoices WHERE grade != '') LIMIT 1;");
    if (unlinkedSalesGrades.isValid() && !unlinkedSalesGrades.isNull()) {
        executeNonQuery(
            "UPDATE sales_invoice_items "
            "SET grade = (SELECT grade FROM sales_invoices WHERE sales_invoices.id = sales_invoice_items.invoice_id) "
            "WHERE (grade IS NULL OR grade = '') "
            "  AND EXISTS (SELECT 1 FROM sales_invoices WHERE sales_invoices.id = sales_invoice_items.invoice_id AND sales_invoices.grade != '');"
        );
    }

    addColumnIfNotExists("purchase_invoices", "market_type", "TEXT DEFAULT 'Market Type (With Stock)'");
    addColumnIfNotExists("purchase_invoices", "due_days", "INTEGER DEFAULT 0");
    addColumnIfNotExists("purchase_invoices", "tax_status", "TEXT DEFAULT 'GST / Exempt'");
    addColumnIfNotExists("purchase_invoices", "challan_no", "TEXT");
    addColumnIfNotExists("purchase_invoices", "freight_charges", "REAL DEFAULT 0.0");
    addColumnIfNotExists("purchase_invoices", "tcs_amount", "REAL DEFAULT 0.0");
    addColumnIfNotExists("purchase_invoices", "tcs_rate", "REAL DEFAULT 0.0");
    addColumnIfNotExists("purchase_invoices", "tds_amount", "REAL DEFAULT 0.0");
    addColumnIfNotExists("purchase_invoices", "tds_rate", "REAL DEFAULT 0.0");
    addColumnIfNotExists("purchase_invoices", "place_of_supply", "TEXT");
    addColumnIfNotExists("purchase_invoice_items", "grade", "TEXT DEFAULT ''");

    QVariant unlinkedPurchGrades = executeScalar("SELECT 1 FROM purchase_invoice_items WHERE (grade IS NULL OR grade = '') AND invoice_id IN (SELECT id FROM purchase_invoices WHERE grade != '') LIMIT 1;");
    if (unlinkedPurchGrades.isValid() && !unlinkedPurchGrades.isNull()) {
        executeNonQuery(
            "UPDATE purchase_invoice_items "
            "SET grade = (SELECT grade FROM purchase_invoices WHERE purchase_invoices.id = purchase_invoice_items.invoice_id) "
            "WHERE (grade IS NULL OR grade = '') "
            "  AND EXISTS (SELECT 1 FROM purchase_invoices WHERE purchase_invoices.id = purchase_invoice_items.invoice_id AND purchase_invoices.grade != '');"
        );
    }

    addColumnIfNotExists("vouchers", "due_days", "INTEGER DEFAULT 0");
    addColumnIfNotExists("vouchers", "market_type", "TEXT");
    addColumnIfNotExists("vouchers", "tax_status", "TEXT");
    addColumnIfNotExists("vouchers", "place_of_supply", "TEXT");
    addColumnIfNotExists("vouchers", "challan_no", "TEXT");
    addColumnIfNotExists("iform_vouchers", "lot_no", "TEXT");
    addColumnIfNotExists("jform_vouchers", "lot_no", "TEXT");

    addColumnIfNotExists("transactions", "due_days", "INTEGER DEFAULT 0");
    addColumnIfNotExists("transactions", "place_of_supply", "TEXT");
    addColumnIfNotExists("transactions", "market_type", "TEXT");
    addColumnIfNotExists("transactions", "purchase_type", "TEXT");
    addColumnIfNotExists("transactions", "entry_type", "TEXT");
    addColumnIfNotExists("transactions", "tcs_amount", "REAL DEFAULT 0.0");
    addColumnIfNotExists("transactions", "tcs_rate", "REAL DEFAULT 0.0");
    addColumnIfNotExists("transactions", "voucher_reconcile", "INTEGER DEFAULT 0");

    // TDSDeductions table matching canonical Data.002 schema
    executeNonQuery(
        "CREATE TABLE IF NOT EXISTS TDSDeductions ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "voucher_date TEXT,"
        "voucher_number INTEGER,"
        "trans_type TEXT DEFAULT 'Jrnl',"
        "account_code INTEGER,"
        "income_amount REAL,"
        "tds REAL,"
        "surcharge REAL DEFAULT 0.0,"
        "education REAL DEFAULT 0.0,"
        "tds_rate REAL,"
        "income_type TEXT,"
        "reason_for_non_deduction TEXT,"
        "rate_tds REAL,"
        "rate_surcharge REAL DEFAULT 0.0,"
        "rate_education REAL DEFAULT 0.0,"
        "post_in_books INTEGER DEFAULT 1,"
        "challan_voucher_date TEXT,"
        "challan_voucher_number INTEGER DEFAULT 0,"
        "income_narration TEXT,"
        "prev_income_amount REAL DEFAULT 0.0"
        ");"
    );
    executeNonQuery("CREATE INDEX IF NOT EXISTS idx_tds_deductions_vch ON TDSDeductions(trans_type, voucher_number, voucher_date);");
    executeNonQuery("CREATE INDEX IF NOT EXISTS idx_tds_deductions_party ON TDSDeductions(account_code, voucher_date);");

    // Backfill TDSDeductions from existing tds_vouchers if table is empty.
    // trans_type / challan_voucher_* are carried from tds_vouchers instead
    // of being fabricated ('Jrnl' / '' / 0) so a migrated firm's legacy TDS
    // register round-trips through the exporter (C3 TDS register-fidelity
    // slice). post_in_books = 1 stays the row filter (app semantics: only
    // posted TDS vouchers appear in the register).
    QVariant tdsDedCount = executeScalar("SELECT COUNT(*) FROM TDSDeductions;");
    if (!tdsDedCount.isValid() || tdsDedCount.toLongLong() == 0) {
        executeNonQuery(
            "INSERT INTO TDSDeductions ("
            "voucher_date, voucher_number, trans_type, account_code, "
            "income_amount, tds, surcharge, education, tds_rate, "
            "income_type, reason_for_non_deduction, rate_tds, rate_surcharge, rate_education, "
            "post_in_books, challan_voucher_date, challan_voucher_number, income_narration, prev_income_amount"
            ") SELECT v.voucher_date, v.voucher_no, COALESCE(NULLIF(TRIM(v.trans_type), ''), 'Jrnl'), COALESCE(p.legacy_id, v.ledger_id), "
            "v.income_amount, v.tax_amount_tds, v.tax_amount_surcharge, v.tax_amount_cess, v.total_tax_rate, "
            "v.tds_type, v.non_deduction_reason, v.rate_tds, v.rate_surcharge, v.rate_cess, "
            "v.post_in_books, v.challan_voucher_date, COALESCE(v.challan_voucher_number, 0), v.narration, v.previous_amount "
            "FROM tds_vouchers v "
            "LEFT JOIN parties p ON v.ledger_id = p.id "
            "WHERE v.post_in_books = 1;"
        );
    }

    // Parties Master Bahi-Khata attributes
    addColumnIfNotExists("parties", "party_station", "TEXT");
    addColumnIfNotExists("parties", "use_routes", "INTEGER DEFAULT 0");
    addColumnIfNotExists("parties", "shop_no", "TEXT");
    addColumnIfNotExists("parties", "tin", "TEXT");
    addColumnIfNotExists("parties", "urn", "TEXT");
    addColumnIfNotExists("parties", "stock_not_calc", "INTEGER DEFAULT 0");
    addColumnIfNotExists("parties", "use_credit_limit", "INTEGER DEFAULT 1");
    addColumnIfNotExists("parties", "show_date_totals", "INTEGER DEFAULT 0");
    addColumnIfNotExists("parties", "calc_direct_expense", "INTEGER DEFAULT 0");
    addColumnIfNotExists("parties", "set_title_case", "INTEGER DEFAULT 1");
    addColumnIfNotExists("parties", "ledger_open_from", "TEXT");
    addColumnIfNotExists("parties", "books_start_from", "TEXT");
    addColumnIfNotExists("parties", "salary_per_month", "REAL DEFAULT 0.0");

    // Salary Payments & Payroll Log Table
    executeNonQuery(
        "CREATE TABLE IF NOT EXISTS salary_payments ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "fy_id INTEGER,"
        "financial_year TEXT,"
        "month_year TEXT NOT NULL,"
        "voucher_date TEXT NOT NULL,"
        "voucher_no TEXT NOT NULL,"
        "party_id INTEGER NOT NULL,"
        "employee_name TEXT NOT NULL,"
        "base_salary REAL DEFAULT 0.0,"
        "days_in_month INTEGER DEFAULT 30,"
        "present_days REAL DEFAULT 30.0,"
        "earned_basic REAL DEFAULT 0.0,"
        "allowances REAL DEFAULT 0.0,"
        "pf_deduction REAL DEFAULT 0.0,"
        "tds_deduction REAL DEFAULT 0.0,"
        "advance_deduction REAL DEFAULT 0.0,"
        "other_deduction REAL DEFAULT 0.0,"
        "net_salary REAL NOT NULL,"
        "narration TEXT,"
        "created_at TEXT DEFAULT CURRENT_TIMESTAMP,"
        "FOREIGN KEY (party_id) REFERENCES parties(id)"
        ");"
    );
    executeNonQuery("CREATE INDEX IF NOT EXISTS idx_salary_payments_month ON salary_payments(month_year);");
    executeNonQuery("CREATE INDEX IF NOT EXISTS idx_salary_payments_party ON salary_payments(party_id, month_year);");

    // Ensure deterministic hierarchy columns exist for account_groups and parties
    addColumnIfNotExists("account_groups", "code1st", "INTEGER DEFAULT 0");
    addColumnIfNotExists("account_groups", "code2nd", "INTEGER DEFAULT 0");
    addColumnIfNotExists("account_groups", "code3rd", "INTEGER DEFAULT 0");
    addColumnIfNotExists("account_groups", "code4th", "INTEGER DEFAULT 0");
    addColumnIfNotExists("account_groups", "standard_root", "INTEGER DEFAULT 0");

    addColumnIfNotExists("parties", "group_code", "INTEGER DEFAULT 8");
    addColumnIfNotExists("parties", "group_id", "INTEGER");

    executeNonQuery("CREATE INDEX IF NOT EXISTS idx_account_groups_code1st ON account_groups(code1st);");
    executeNonQuery("CREATE INDEX IF NOT EXISTS idx_account_groups_hierarchy ON account_groups(code1st, code2nd, code3rd, code4th);");
    executeNonQuery("CREATE INDEX IF NOT EXISTS idx_parties_group_code ON parties(group_code);");
    executeNonQuery("CREATE INDEX IF NOT EXISTS idx_parties_group_id ON parties(group_id);");
    executeNonQuery("CREATE INDEX IF NOT EXISTS idx_stock_trans_date_item ON stock_transactions(voucher_date, item_name, trans_type);");
    executeNonQuery("CREATE INDEX IF NOT EXISTS idx_stock_trans_item_code ON stock_transactions(item_code, voucher_date);");
    executeNonQuery("CREATE INDEX IF NOT EXISTS idx_sales_invoices_date ON sales_invoices(invoice_date);");
    executeNonQuery("CREATE INDEX IF NOT EXISTS idx_purchase_invoices_date ON purchase_invoices(invoice_date);");
    executeNonQuery("CREATE INDEX IF NOT EXISTS idx_custom_closing_stocks_date ON custom_closing_stocks(closing_date, item_code);");

    // 1. Backfill legacy group codes from description if code1st = 0
    QVariant needGroupBackfill = executeScalar("SELECT 1 FROM account_groups WHERE (code1st IS NULL OR code1st = 0) AND description LIKE 'Legacy Group Code #%' LIMIT 1;");
    if (needGroupBackfill.isValid() && !needGroupBackfill.isNull()) {
        executeNonQuery(
            "UPDATE account_groups "
            "SET code1st = CAST(SUBSTR(description, 20) AS INTEGER) "
            "WHERE (code1st IS NULL OR code1st = 0) AND description LIKE 'Legacy Group Code #%';"
        );
    }

    // 2. Synchronize standard group codes and parent hierarchy
    struct MasterGroupInfo {
        int code1;
        int code2;
        int code3;
        int code4;
        const char* name;
        const char* nature;
    };
    static const MasterGroupInfo stdGroups[] = {
        {1, 0, 0, 0, "Capital A/c", "Liabilities"},
        {2, 0, 0, 0, "Current Assets", "Assets"},
        {3, 2, 0, 0, "Bank(s) A/c", "Assets"},
        {4, 2, 0, 0, "Cash-In-Hand", "Assets"},
        {6, 2, 0, 0, "Loan & Advances (Assets)", "Assets"},
        {7, 2, 0, 0, "Stock-In-Hand", "Assets"},
        {8, 2, 0, 0, "Sundry Debtors", "Assets"},
        {9, 0, 0, 0, "Current Liabilities", "Liabilities"},
        {10, 9, 0, 0, "Duties & Taxes", "Liabilities"},
        {11, 9, 0, 0, "Sundry Creditors", "Liabilities"},
        {12, 9, 0, 0, "Provisions", "Liabilities"},
        {13, 9, 0, 0, "Loans (Liability)", "Liabilities"},
        {14, 0, 0, 0, "Fixed Assets", "Assets"},
        {15, 0, 0, 0, "Profit & Loss", "Liabilities"},
        {16, 15, 0, 0, "Income A/c", "Income"},
        {17, 15, 0, 0, "Expenditure A/c", "Expense"},
        {18, 0, 0, 0, "Trading Items Stock A/c", "Assets"},
        {19, 18, 0, 0, "Purchase A/c", "Expense"},
        {20, 18, 0, 0, "Sale A/c", "Income"},
        {21, 0, 0, 0, "Commission Basis Parties A/c", "Liabilities"},
        {22, 0, 0, 0, "Manufacturing Group", "Expense"},
        {23, 22, 0, 0, "Manufacturing Exp.", "Expense"},
        {24, 18, 0, 0, "Trading Exp.", "Expense"},
        {25, 0, 0, 0, "Suspense A/c", "Liabilities"},
        {26, 2, 0, 0, "Deposit (Assets)", "Assets"},
        {27, 0, 0, 0, "Branches / Divisions", "Liabilities"},
        {28, 13, 9, 0, "Secured Loans", "Liabilities"},
        {29, 13, 9, 0, "Unsecured Loans", "Liabilities"},
        {30, 2, 0, 0, "Security A/c", "Assets"},
        {31, 11, 9, 0, "Local Mandi Creditors", "Liabilities"},
        {32, 8, 2, 0, "Zimidara Debtors", "Assets"},
        {33, 11, 9, 0, "Zimidara Creditors", "Liabilities"},
        {34, 8, 2, 0, "Mandi Debtors", "Assets"},
        {35, 9, 0, 0, "Employees", "Liabilities"},
        {36, 11, 9, 0, "Mandi Creditors", "Liabilities"},
        {37, 8, 2, 0, "Mandi Trader/s", "Assets"},
        {38, 8, 2, 0, "Rice Bran Debitors", "Assets"},
        {39, 8, 2, 0, "Rice Basmati Debitors", "Assets"},
        {40, 9, 0, 0, "Payable", "Liabilities"},
        {41, 11, 9, 0, "Machinery Parts Creditors", "Liabilities"},
        {42, 20, 18, 0, "Sale Return", "Income"},
        {43, 11, 9, 0, "Rice Basmati Creditors", "Liabilities"},
        {44, 17, 15, 0, "Thekedar A/c", "Expense"},
        {45, 9, 0, 0, "Salary PF", "Liabilities"},
        {46, 8, 2, 0, "Broker", "Assets"},
        {47, 11, 9, 0, "Rice Bran Creditors", "Liabilities"}
    };

    // Legacy sync runs ONLY on legacy charts (Standard/Legacy markers). Fresh
    // generic charts carry 'Generic Chart v1' rows and are never bulk-filled.
    // Domain codes (mandi 31-34, manufacturer 22-23) are UPDATE-only here: the domain
    // top-up (ensureDomainGroups) owns their creation, so this sync
    // can neither re-insert them on generic firms nor strand migrated rows.
    QVariant legacyChart = executeScalar(
        "SELECT 1 FROM account_groups WHERE description LIKE 'Standard Group #%' "
        "OR description LIKE 'Legacy Group Code #%' LIMIT 1;");
    if (legacyChart.isValid() && !legacyChart.isNull()) {
        for (const auto& sg : stdGroups) {
            const bool domainOwned = (sg.code1 == 22 || sg.code1 == 23 ||
                                      sg.code1 == 31 || sg.code1 == 32 ||
                                      sg.code1 == 33 || sg.code1 == 34);
            QVariant v = executeScalar("SELECT id FROM account_groups WHERE name = ? OR code1st = ? LIMIT 1;", {sg.name, sg.code1});
            if (v.isValid() && !v.isNull()) {
                executeNonQuery(
                    "UPDATE account_groups SET code1st = ?, code2nd = ?, code3rd = ?, code4th = ?, nature = ? WHERE id = ?;",
                    {sg.code1, sg.code2, sg.code3, sg.code4, sg.nature, v}
                );
            } else if (!domainOwned) {
                executeNonQuery(
                    "INSERT INTO account_groups (name, parent_group_name, nature, code1st, code2nd, code3rd, code4th, is_system, extract_in_balance_sheet) "
                    "VALUES (?, 'Primary', ?, ?, ?, ?, ?, 1, 1);",
                    {sg.name, sg.nature, sg.code1, sg.code2, sg.code3, sg.code4}
                );
            }
        }
    }

    // 3. Backfill parties foreign keys group_id and group_code (only if unlinked parties exist)
    QVariant unlinkedParties = executeScalar("SELECT 1 FROM parties WHERE group_id IS NULL OR group_code IS NULL OR group_code = 0 LIMIT 1;");
    if (unlinkedParties.isValid() && !unlinkedParties.isNull()) {
        executeNonQuery(
            "UPDATE parties "
            "SET group_id = (SELECT id FROM account_groups WHERE account_groups.name = parties.group_name LIMIT 1), "
            "    group_code = (SELECT code1st FROM account_groups WHERE account_groups.name = parties.group_name LIMIT 1) "
            "WHERE (group_id IS NULL OR group_code IS NULL OR group_code = 0) "
            "  AND EXISTS (SELECT 1 FROM account_groups WHERE account_groups.name = parties.group_name);"
        );
    }

    addColumnIfNotExists("jform_vouchers", "vehicle_no", "TEXT");
    addColumnIfNotExists("iform_vouchers", "vehicle_no", "TEXT");
    addColumnIfNotExists("purchase_invoices", "vehicle_no", "TEXT");
    addColumnIfNotExists("sales_invoices", "vehicle_no", "TEXT");
    addColumnIfNotExists("vouchers", "vehicle_no", "TEXT");
    addColumnIfNotExists("gate_register", "vehicle_no", "TEXT");
    addColumnIfNotExists("bardana_transactions", "vehicle_no", "TEXT");
    addColumnIfNotExists("transport_dispatches", "vehicle_no", "TEXT");

    // Deterministic uppercase normalization for all vehicle_no entries.
    // Each UPDATE is existence-guarded — generic DBs may not have
    // the domain tables at all (fail-loud stays for real errors elsewhere).
    executeNonQuery("UPDATE purchase_invoices SET vehicle_no = UPPER(TRIM(vehicle_no)) WHERE vehicle_no IS NOT NULL AND vehicle_no != UPPER(TRIM(vehicle_no));");
    executeNonQuery("UPDATE sales_invoices SET vehicle_no = UPPER(TRIM(vehicle_no)) WHERE vehicle_no IS NOT NULL AND vehicle_no != UPPER(TRIM(vehicle_no));");
    executeNonQuery("UPDATE vouchers SET vehicle_no = UPPER(TRIM(vehicle_no)) WHERE vehicle_no IS NOT NULL AND vehicle_no != UPPER(TRIM(vehicle_no));");
    if (FeatureFlags::tableExists("gate_register")) executeNonQuery("UPDATE gate_register SET vehicle_no = UPPER(TRIM(vehicle_no)) WHERE vehicle_no IS NOT NULL AND vehicle_no != UPPER(TRIM(vehicle_no));");
    if (FeatureFlags::tableExists("bardana_transactions")) executeNonQuery("UPDATE bardana_transactions SET vehicle_no = UPPER(TRIM(vehicle_no)) WHERE vehicle_no IS NOT NULL AND vehicle_no != UPPER(TRIM(vehicle_no));");
    if (FeatureFlags::tableExists("jform_vouchers")) executeNonQuery("UPDATE jform_vouchers SET vehicle_no = UPPER(TRIM(vehicle_no)) WHERE vehicle_no IS NOT NULL AND vehicle_no != UPPER(TRIM(vehicle_no));");
    if (FeatureFlags::tableExists("iform_vouchers")) executeNonQuery("UPDATE iform_vouchers SET vehicle_no = UPPER(TRIM(vehicle_no)) WHERE vehicle_no IS NOT NULL AND vehicle_no != UPPER(TRIM(vehicle_no));");
    if (FeatureFlags::tableExists("transport_dispatches")) executeNonQuery("UPDATE transport_dispatches SET vehicle_no = UPPER(TRIM(vehicle_no)) WHERE vehicle_no IS NOT NULL AND vehicle_no != UPPER(TRIM(vehicle_no));");

    // Deterministic indexes for vehicle_no lookups
    executeNonQuery("CREATE INDEX IF NOT EXISTS idx_purchase_vehicle ON purchase_invoices(vehicle_no);");
    executeNonQuery("CREATE INDEX IF NOT EXISTS idx_sales_vehicle ON sales_invoices(vehicle_no);");
    executeNonQuery("CREATE INDEX IF NOT EXISTS idx_vouchers_vehicle ON vouchers(vehicle_no);");
    if (FeatureFlags::tableExists("bardana_transactions")) executeNonQuery("CREATE INDEX IF NOT EXISTS idx_bardana_vehicle ON bardana_transactions(vehicle_no);");
    if (FeatureFlags::tableExists("jform_vouchers")) executeNonQuery("CREATE INDEX IF NOT EXISTS idx_jform_vehicle ON jform_vouchers(vehicle_no);");
    if (FeatureFlags::tableExists("iform_vouchers")) executeNonQuery("CREATE INDEX IF NOT EXISTS idx_iform_vehicle ON iform_vouchers(vehicle_no);");
    addColumnIfNotExists("jform_vouchers", "driver_name", "TEXT");
    addColumnIfNotExists("jform_vouchers", "gate_pass_no", "TEXT");
    addColumnIfNotExists("jform_vouchers", "eway_bill_no", "TEXT");
    addColumnIfNotExists("jform_vouchers", "bill_time", "TEXT");
    addColumnIfNotExists("jform_vouchers", "sauda_date", "TEXT");
    addColumnIfNotExists("jform_vouchers", "mandi_place", "TEXT");
    addColumnIfNotExists("jform_vouchers", "procurement_mode", "TEXT");
    addColumnIfNotExists("jform_vouchers", "lot_no", "TEXT");
    addColumnIfNotExists("jform_vouchers", "grade", "TEXT");
    addColumnIfNotExists("jform_vouchers", "transport", "TEXT");
    addColumnIfNotExists("jform_vouchers", "broker_name", "TEXT");
    addColumnIfNotExists("jform_vouchers", "challan_no", "TEXT");
    addColumnIfNotExists("jform_vouchers", "kanda_weight", "TEXT");
    addColumnIfNotExists("jform_vouchers", "gr_no", "TEXT");

    addColumnIfNotExists("iform_vouchers", "transport", "TEXT");
    addColumnIfNotExists("iform_vouchers", "broker_name", "TEXT");
    addColumnIfNotExists("iform_vouchers", "driver_name", "TEXT");
    addColumnIfNotExists("iform_vouchers", "challan_no", "TEXT");
    addColumnIfNotExists("iform_vouchers", "gr_no", "TEXT");

    addColumnIfNotExists("vouchers", "transport", "TEXT");
    addColumnIfNotExists("vouchers", "broker_name", "TEXT");
    addColumnIfNotExists("vouchers", "driver_name", "TEXT");
    addColumnIfNotExists("vouchers", "gate_pass_no", "TEXT");

    addColumnIfNotExists("transactions", "transport", "TEXT");
    addColumnIfNotExists("transactions", "broker_name", "TEXT");
    addColumnIfNotExists("transactions", "driver_name", "TEXT");
    addColumnIfNotExists("transactions", "gate_pass_no", "TEXT");

    addColumnIfNotExists("purchase_invoices", "transport", "TEXT");
    addColumnIfNotExists("purchase_invoices", "broker_name", "TEXT");
    addColumnIfNotExists("purchase_invoices", "driver_name", "TEXT");
    addColumnIfNotExists("purchase_invoices", "gate_pass_no", "TEXT");

    addColumnIfNotExists("sales_invoices", "transport", "TEXT");
    addColumnIfNotExists("sales_invoices", "broker_name", "TEXT");
    addColumnIfNotExists("sales_invoices", "driver_name", "TEXT");
    addColumnIfNotExists("sales_invoices", "gate_pass_no", "TEXT");

    addColumnIfNotExists("transport_dispatches", "transport", "TEXT");
    addColumnIfNotExists("transport_dispatches", "broker_name", "TEXT");
    addColumnIfNotExists("transport_dispatches", "gate_pass_no", "TEXT");

    addColumnIfNotExists("gate_register", "transport", "TEXT");
    addColumnIfNotExists("gate_register", "broker_name", "TEXT");
    addColumnIfNotExists("gate_register", "gr_no", "TEXT");

    // Migrate any legacy non-standard column data into standard single definitions
    if (getCols("sales_invoices").contains("driver")) {
        executeNonQuery("UPDATE sales_invoices SET driver_name = driver WHERE (driver_name IS NULL OR driver_name = '') AND driver IS NOT NULL AND driver != '';");
    }
    if (getCols("purchase_invoices").contains("driver")) {
        executeNonQuery("UPDATE purchase_invoices SET driver_name = driver WHERE (driver_name IS NULL OR driver_name = '') AND driver IS NOT NULL AND driver != '';");
    }
    if (getCols("vouchers").contains("driver")) {
        executeNonQuery("UPDATE vouchers SET driver_name = driver WHERE (driver_name IS NULL OR driver_name = '') AND driver IS NOT NULL AND driver != '';");
    }
    if (getCols("transport_dispatches").contains("transporter_name")) {
        executeNonQuery("UPDATE transport_dispatches SET transport = transporter_name WHERE (transport IS NULL OR transport = '') AND transporter_name IS NOT NULL AND transporter_name != '';");
    }
    if (getCols("gate_register").contains("transporter_name")) {
        executeNonQuery("UPDATE gate_register SET transport = transporter_name WHERE (transport IS NULL OR transport = '') AND transporter_name IS NOT NULL AND transporter_name != '';");
    }
    if (getCols("jform_vouchers").contains("transport_name")) {
        executeNonQuery("UPDATE jform_vouchers SET transport = transport_name WHERE (transport IS NULL OR transport = '') AND transport_name IS NOT NULL AND transport_name != '';");
    }
    if (getCols("iform_vouchers").contains("transport_name")) {
        executeNonQuery("UPDATE iform_vouchers SET transport = transport_name WHERE (transport IS NULL OR transport = '') AND transport_name IS NOT NULL AND transport_name != '';");
    }

    // Deterministic indexes for broker and transport lookups
    executeNonQuery("CREATE INDEX IF NOT EXISTS idx_purchase_transport ON purchase_invoices(transport);");
    executeNonQuery("CREATE INDEX IF NOT EXISTS idx_sales_transport ON sales_invoices(transport);");
    executeNonQuery("CREATE INDEX IF NOT EXISTS idx_vouchers_transport ON vouchers(transport);");
    executeNonQuery("CREATE INDEX IF NOT EXISTS idx_purchase_broker ON purchase_invoices(broker_name);");
    executeNonQuery("CREATE INDEX IF NOT EXISTS idx_sales_broker ON sales_invoices(broker_name);");
    executeNonQuery("CREATE INDEX IF NOT EXISTS idx_vouchers_broker ON vouchers(broker_name);");
    if (FeatureFlags::tableExists("transport_dispatches")) executeNonQuery("CREATE INDEX IF NOT EXISTS idx_transport_dispatches_transport ON transport_dispatches(transport);");
    if (FeatureFlags::tableExists("gate_register")) executeNonQuery("CREATE INDEX IF NOT EXISTS idx_gate_register_transport ON gate_register(transport);");

    // Ensure all Stock Item columns exist for backward compatibility
    addColumnIfNotExists("stock_items", "trading_group", "TEXT");
    addColumnIfNotExists("stock_items", "group_code", "INTEGER");
    addColumnIfNotExists("stock_items", "unit_code", "INTEGER");
    addColumnIfNotExists("stock_items", "rate_calc_on", "TEXT DEFAULT 'N/A'");
    addColumnIfNotExists("stock_items", "auto_adjust_name", "INTEGER DEFAULT 1");
    addColumnIfNotExists("stock_items", "item_narration", "TEXT");
    addColumnIfNotExists("stock_items", "capital_goods", "INTEGER DEFAULT 0");
    addColumnIfNotExists("stock_items", "vat_rate", "REAL DEFAULT 0.0");
    addColumnIfNotExists("stock_items", "vat_ledger", "TEXT DEFAULT 'VAT A/c'");
    addColumnIfNotExists("stock_items", "surcharge_on_vat", "REAL DEFAULT 0.0");
    addColumnIfNotExists("stock_items", "vat_against_d1", "REAL DEFAULT 0.0");
    addColumnIfNotExists("stock_items", "cst_rate", "REAL DEFAULT 0.0");
    addColumnIfNotExists("stock_items", "cst_ledger", "TEXT DEFAULT 'CST A/c'");
    addColumnIfNotExists("stock_items", "cst_without_cform", "REAL DEFAULT 0.0");
    addColumnIfNotExists("stock_items", "dami_ledger", "TEXT DEFAULT 'Dami A/c'");
    addColumnIfNotExists("stock_items", "market_fee_ledger", "TEXT DEFAULT 'Market Fee A/c'");
    addColumnIfNotExists("stock_items", "hrdf_ledger", "TEXT DEFAULT 'H.R.D.F. A/c'");
    addColumnIfNotExists("stock_items", "market_commtt_form_apply", "INTEGER DEFAULT 0");
    addColumnIfNotExists("stock_items", "market_commtt_coupon_apply", "INTEGER DEFAULT 0");
    addColumnIfNotExists("stock_items", "dami_calc_on_weight", "INTEGER DEFAULT 0");
    addColumnIfNotExists("stock_items", "tax_on_qty", "INTEGER DEFAULT 0");
    addColumnIfNotExists("stock_items", "purchase_return_ledger", "TEXT DEFAULT 'Purchase Accounts'");
    addColumnIfNotExists("stock_items", "sale_return_ledger", "TEXT DEFAULT 'Sales Accounts'");
    addColumnIfNotExists("stock_items", "gst_ledger", "TEXT DEFAULT 'Duties & Taxes'");
    addColumnIfNotExists("stock_items", "bonus_approved", "REAL DEFAULT 0.0");
    addColumnIfNotExists("stock_items", "labour_rate_unit", "TEXT DEFAULT 'Packing'");
    addColumnIfNotExists("stock_items", "utrai_rate_1", "REAL DEFAULT 0.0");
    addColumnIfNotExists("stock_items", "jharai_rate_1", "REAL DEFAULT 0.0");
    addColumnIfNotExists("stock_items", "bharai_rate_1", "REAL DEFAULT 0.0");
    addColumnIfNotExists("stock_items", "tulai_rate_1", "REAL DEFAULT 0.0");
    addColumnIfNotExists("stock_items", "khichai_rate_1", "REAL DEFAULT 0.0");
    addColumnIfNotExists("stock_items", "silai_rate_1", "REAL DEFAULT 0.0");
    addColumnIfNotExists("stock_items", "loading_rate_1", "REAL DEFAULT 0.0");
    addColumnIfNotExists("stock_items", "utrai_rate_2", "REAL DEFAULT 0.0");
    addColumnIfNotExists("stock_items", "jharai_rate_2", "REAL DEFAULT 0.0");
    addColumnIfNotExists("stock_items", "bharai_rate_2", "REAL DEFAULT 0.0");
    addColumnIfNotExists("stock_items", "tulai_rate_2", "REAL DEFAULT 0.0");
    addColumnIfNotExists("stock_items", "khichai_rate_2", "REAL DEFAULT 0.0");
    addColumnIfNotExists("stock_items", "silai_rate_2", "REAL DEFAULT 0.0");
    addColumnIfNotExists("stock_items", "loading_rate_2", "REAL DEFAULT 0.0");
    addColumnIfNotExists("stock_items", "utrai_rate_3", "REAL DEFAULT 0.0");
    addColumnIfNotExists("stock_items", "jharai_rate_3", "REAL DEFAULT 0.0");
    addColumnIfNotExists("stock_items", "bharai_rate_3", "REAL DEFAULT 0.0");
    addColumnIfNotExists("stock_items", "tulai_rate_3", "REAL DEFAULT 0.0");
    addColumnIfNotExists("stock_items", "khichai_rate_3", "REAL DEFAULT 0.0");
    addColumnIfNotExists("stock_items", "loading_rate_3", "REAL DEFAULT 0.0");
    addColumnIfNotExists("stock_items", "purchase_ledger_id", "INTEGER");
    addColumnIfNotExists("stock_items", "purchase_return_ledger_id", "INTEGER");
    addColumnIfNotExists("stock_items", "sale_ledger_id", "INTEGER");
    addColumnIfNotExists("stock_items", "sale_return_ledger_id", "INTEGER");
    addColumnIfNotExists("stock_items", "stock_ledger_id", "INTEGER");
    addColumnIfNotExists("stock_items", "gst_ledger_id", "INTEGER");
    addColumnIfNotExists("stock_items", "vat_ledger_id", "INTEGER");
    addColumnIfNotExists("stock_items", "cst_ledger_id", "INTEGER");
    addColumnIfNotExists("stock_items", "dami_ledger_id", "INTEGER");
    addColumnIfNotExists("stock_items", "market_fee_ledger_id", "INTEGER");
    addColumnIfNotExists("stock_items", "hrdf_ledger_id", "INTEGER");
    // Generic manufacturing: item role drives production classification
    // (input/output/byproduct) — data, never item-name heuristics.
    addColumnIfNotExists("stock_items", "manufacturing_role", "TEXT DEFAULT ''");

    // Generic manufacturing batch schema (manufacturer domain). The legacy
    // rice-mill column names (paddy_variety / paddy_input_qtl / head_rice_qtl
    // / broken_rice_qtl / bran_qtl / husk_qtl) are carried once into the
    // generic input_variety / input_qtl / output_qtl / byproduct_qtl. The old
    // columns are left in place (SQLite cannot drop them portably) but nothing
    // reads them after this point; fresh DBs are created generic.
    addColumnIfNotExists("manufacturing_batches", "input_variety", "TEXT DEFAULT ''");
    addColumnIfNotExists("manufacturing_batches", "input_qtl", "REAL DEFAULT 0.0");
    addColumnIfNotExists("manufacturing_batches", "output_qtl", "REAL DEFAULT 0.0");
    addColumnIfNotExists("manufacturing_batches", "byproduct_qtl", "REAL DEFAULT 0.0");
    if (getCols("manufacturing_batches").contains("paddy_variety")
        && getSetting("schema_manufacturing_generic_v1", "0") != "1") {
        executeNonQuery(
            "UPDATE manufacturing_batches SET "
            " input_variety = paddy_variety, "
            " input_qtl = paddy_input_qtl, "
            " output_qtl = head_rice_qtl, "
            " byproduct_qtl = COALESCE(broken_rice_qtl,0) + COALESCE(bran_qtl,0) + COALESCE(husk_qtl,0);");
        setSetting("schema_manufacturing_generic_v1", "1");
    }

    // TDS Vouchers deposit tracking
    addColumnIfNotExists("tds_vouchers", "challan_id", "INTEGER");
    addColumnIfNotExists("tds_vouchers", "is_deposited", "INTEGER DEFAULT 0");
    // Legacy Jet TDSDeductions register fields (C3 TDS register-fidelity
    // slice): additive on existing DBs, carried verbatim for export.
    addColumnIfNotExists("tds_vouchers", "trans_type", "TEXT DEFAULT 'Jrnl'");
    addColumnIfNotExists("tds_vouchers", "challan_voucher_date", "TEXT");
    addColumnIfNotExists("tds_vouchers", "challan_voucher_number", "INTEGER DEFAULT 0");

    // Ensure the six generic system ledgers (fresh, non-migrated firms only).
    // Groups resolve BY NAME from the live chart (robust to renames); the
    // legacy Bahi-Khata template list retires with the Bahi chart seed.
    // (TCS Payable posts Cr like TDS Payable — the template's Dr was a bug.)
    struct GenDefLedger { int code; const char* groupName; const char* name; const char* balType; };
    static const GenDefLedger defaultLedgers[] = {
        {1, "Cash-In-Hand", "Cash", "Dr"},
        {0, "Bank(s) A/c", "Bank", "Dr"},
        {2, "Profit & Loss", "Profit & Loss", "Dr"},
        {951, "Duties & Taxes", "TDS Payable A/c", "Cr"},
        {948, "Duties & Taxes", "TCS Payable A/c", "Cr"},
        {0, "Duties & Taxes", "GST Payable A/c", "Cr"},
    };

    // Only insert default ledgers for brand-new, empty, non-migrated databases
    int partyCount = executeScalar("SELECT COUNT(*) FROM parties;").toInt();
    QString isMigrated = getSetting("is_migrated", "0");
    QString srcType = getSetting("firm_source_type", "");
    if (partyCount == 0 && isMigrated != "1" && srcType.isEmpty()) {
        for (const auto& bl : defaultLedgers) {
            // Name-only existence check: legacy_id 0 is shared by Bank and GST
            // Payable, so an OR-legacy check would match the earlier row and
            // silently skip the later one (fresh DBs seeded 5 ledgers, not 6).
            QVariant v = executeScalar("SELECT id FROM parties WHERE name = ? LIMIT 1;", {bl.name});
            if (!v.isValid() || v.isNull()) {
                qint64 gId = 0;
                int gCode = 0;
                QVariantList grow = executeQuery("SELECT id, code1st FROM account_groups WHERE name = ? LIMIT 1;", {QString::fromUtf8(bl.groupName)});
                if (!grow.isEmpty()) {
                    QVariantMap gm = grow.first().toMap();
                    gId = gm.value("id").toLongLong();
                    gCode = gm.value("code1st").toInt();
                }
                executeNonQuery(
                    "INSERT INTO parties (name, group_name, group_code, group_id, balance_type, legacy_id) "
                    "VALUES (?, ?, ?, ?, ?, ?);",
                    {bl.name, QString::fromUtf8(bl.groupName), gCode, gId, bl.balType, bl.code}
                );
            }
        }
    } else if (partyCount > 0) {
        // Automatically purge unreferenced duplicate parties
        executeNonQuery(
            "DELETE FROM parties WHERE id NOT IN ("
            "  SELECT MIN(id) FROM parties GROUP BY LOWER(TRIM(name)), group_code"
            ") AND id NOT IN ("
            "  SELECT DISTINCT party_id FROM transactions WHERE party_id IS NOT NULL AND party_id > 0"
            ") AND id NOT IN ("
            "  SELECT DISTINCT party_id FROM vouchers WHERE party_id IS NOT NULL AND party_id > 0"
            ");"
        );
    }

    // Audit trail: every mutation leaves a row (Tally Edit-Log parity).
    // Written by DatabaseManager::logAudit from inside the caller's
    // transaction, so a rolled-back posting leaves no audit row either.
    executeNonQuery(
        "CREATE TABLE IF NOT EXISTS audit_log ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "ts TEXT NOT NULL DEFAULT (strftime('%Y-%m-%dT%H:%M:%fZ','now')),"
        "actor TEXT NOT NULL DEFAULT 'local',"
        "action TEXT NOT NULL,"
        "table_name TEXT NOT NULL,"
        "row_id TEXT NOT NULL DEFAULT '',"
        "voucher_no TEXT NOT NULL DEFAULT '',"
        "fy_name TEXT NOT NULL DEFAULT '',"
        "before_json TEXT NOT NULL DEFAULT '',"
        "after_json TEXT NOT NULL DEFAULT ''"
        ");"
    );
    executeNonQuery("CREATE INDEX IF NOT EXISTS idx_audit_table_row ON audit_log(table_name, row_id);");
    executeNonQuery("CREATE INDEX IF NOT EXISTS idx_audit_ts ON audit_log(ts);");

    // Query-audit index coverage (2026-10-12): every predicate observed in
    // hot paths gets a leftmost-column index. OR-conditions across columns
    // still scan — those call sites are being rewritten to UNION/single-key
    // lookups separately; these indexes cover the single-key halves.
    executeNonQuery("CREATE INDEX IF NOT EXISTS idx_vouchers_no ON vouchers(voucher_no);");
    executeNonQuery("CREATE INDEX IF NOT EXISTS idx_vouchers_date ON vouchers(voucher_date);");
    executeNonQuery("CREATE INDEX IF NOT EXISTS idx_vouchers_party_date ON vouchers(party_name, voucher_date);");
    executeNonQuery("CREATE INDEX IF NOT EXISTS idx_transactions_invoice ON transactions(invoice_no);");
    executeNonQuery("CREATE INDEX IF NOT EXISTS idx_transactions_date ON transactions(voucher_date, id);");
    executeNonQuery("CREATE INDEX IF NOT EXISTS idx_transactions_opposing ON transactions(opposing_account, voucher_date);");
    executeNonQuery("CREATE INDEX IF NOT EXISTS idx_sales_inv_nos ON sales_invoices(invoice_no, voucher_no);");
    executeNonQuery("CREATE INDEX IF NOT EXISTS idx_sales_customer ON sales_invoices(customer_name);");
    executeNonQuery("CREATE INDEX IF NOT EXISTS idx_purchase_inv_nos ON purchase_invoices(invoice_no, voucher_no);");
    executeNonQuery("CREATE INDEX IF NOT EXISTS idx_purchase_supplier ON purchase_invoices(supplier_name);");
    if (FeatureFlags::tableExists("transport_dispatches")) executeNonQuery("CREATE INDEX IF NOT EXISTS idx_transport_voucher ON transport_dispatches(voucher_no);");
    executeNonQuery("CREATE INDEX IF NOT EXISTS idx_parties_name ON parties(name COLLATE NOCASE);");
    if (FeatureFlags::tableExists("jform_vouchers")) executeNonQuery("CREATE INDEX IF NOT EXISTS idx_jform_nos ON jform_vouchers(voucher_no, jform_no);");
    if (FeatureFlags::tableExists("iform_vouchers")) executeNonQuery("CREATE INDEX IF NOT EXISTS idx_iform_nos ON iform_vouchers(voucher_no, iform_no);");
    if (FeatureFlags::tableExists("manufacturing_batches")) executeNonQuery("CREATE INDEX IF NOT EXISTS idx_manufacturing_batch_no ON manufacturing_batches(batch_no);");

    // One-time transport dedupe (v1): the pre-fix sync re-inserted rows on
    // every view visit, so legacy files can hold thousands of byte-identical
    // dispatches (same slip/voucher/invoice/party/date/vehicle/freight).
    // Keep MIN(id) per identical group; runs once, gated by setting.
    // Skipped entirely when the table was never created.
    if (getSetting("transport_deduped_v1", "") != "1" && FeatureFlags::tableExists("transport_dispatches")) {
        QVariant dupeGroups = executeScalar(
            "SELECT COUNT(*) FROM (SELECT voucher_type, financial_year, voucher_no, invoice_no, slip_no, "
            "party_name, dispatch_date, vehicle_no, total_freight FROM transport_dispatches "
            "GROUP BY voucher_type, financial_year, voucher_no, invoice_no, slip_no, "
            "party_name, dispatch_date, vehicle_no, total_freight HAVING COUNT(*) > 1);");
        const qint64 ng = dupeGroups.isValid() ? dupeGroups.toLongLong() : 0;
        if (ng > 0) {
            QVariant dupeRows = executeScalar(
                "SELECT COUNT(*) - COUNT(DISTINCT voucher_type || '|' || financial_year || '|' || voucher_no || '|' || "
                "invoice_no || '|' || slip_no || '|' || party_name || '|' || dispatch_date || '|' || "
                "vehicle_no || '|' || total_freight) FROM transport_dispatches;");
            executeNonQuery(
                "DELETE FROM transport_dispatches WHERE id NOT IN ("
                "  SELECT MIN(id) FROM transport_dispatches GROUP BY voucher_type, financial_year, voucher_no, "
                "  invoice_no, slip_no, party_name, dispatch_date, vehicle_no, total_freight"
                ");");
            qWarning() << "[DB] transport dedupe v1: removed duplicates across" << ng
                       << "groups (" << (dupeRows.isValid() ? dupeRows.toLongLong() : -1) << "rows).";
            logAudit("REPAIR", "transport_dispatches", "",
                     QString("{\"duplicate_groups\":%1}").arg(ng), "", "", "");
        }
        setSetting("transport_deduped_v1", "1");
    }

    // Numbering guard: own-numbered series get a UNIQUE backstop so a
    // select-then-insert race can never double-issue a number.
    // sales_invoices carry OUR bills (unique per FY by construction).
    executeNonQuery(
        "CREATE UNIQUE INDEX IF NOT EXISTS uq_sales_fy_invoice "
        "ON sales_invoices(financial_year, invoice_no);");
    // purchase_invoices carry the SUPPLIER's bill verbatim — the same number
    // legitimately repeats across suppliers/FYs, so it must NEVER be unique.
    if (getSetting("numbering_repair_needed", "") == "1") {
        // Previously degraded: re-check — self-heals to UNIQUE once repaired.
        QVariant stillDup = executeScalar(
            "SELECT COUNT(*) FROM (SELECT voucher_type, financial_year, voucher_no "
            "FROM vouchers GROUP BY voucher_type, financial_year, voucher_no HAVING COUNT(*) > 1);");
        if (stillDup.isValid() && stillDup.toLongLong() == 0) {
            if (executeNonQuery(
                    "CREATE UNIQUE INDEX IF NOT EXISTS uq_vouchers_type_fy_no "
                    "ON vouchers(voucher_type, financial_year, voucher_no);")) {
                setSetting("numbering_repair_needed", "0");
            }
        } else {
            executeNonQuery(
                "CREATE INDEX IF NOT EXISTS idx_vouchers_type_fy_no "
                "ON vouchers(voucher_type, financial_year, voucher_no);");
        }
    } else if (!executeNonQuery(
                   "CREATE UNIQUE INDEX IF NOT EXISTS uq_vouchers_type_fy_no "
                   "ON vouchers(voucher_type, financial_year, voucher_no);")) {
        // Legacy migrated data can hold real double-posts (see
        // VoucherNumbering::findDuplicateVoucherNos). Never delete money:
        // degrade to a plain index, flag repair, keep the app-level claim
        // loop as the live guard.
        QVariant dupes = executeScalar(
            "SELECT COUNT(*) FROM (SELECT voucher_type, financial_year, voucher_no "
            "FROM vouchers GROUP BY voucher_type, financial_year, voucher_no HAVING COUNT(*) > 1);");
        qWarning() << "[DB] UNIQUE vouchers guard skipped:"
                   << (dupes.isValid() ? dupes.toLongLong() : -1)
                   << "legacy duplicate groups; repair report available, app-level claim loop active.";
        setSetting("numbering_repair_needed", "1");
        executeNonQuery(
            "CREATE INDEX IF NOT EXISTS idx_vouchers_type_fy_no "
            "ON vouchers(voucher_type, financial_year, voucher_no);");
    } else {
        setSetting("numbering_repair_needed", "0");
    }

    executeNonQuery("COMMIT;");
    qDebug() << "[DB] ensureTablesExist finished in" << ddlTimer.elapsed()
             << "ms (db:" << m_dbPath << ")";

    // Flag-resolved: migrated firms pick up domain flags from detected data
    // (ON-only, never strands rows, never writes for fresh generic DBs).
    FeatureFlags::persistDetected();

    // Automatically heal and catalog any custom or foreign groups
    AccountClassifier::healAllGroups();
}

void DatabaseManager::ensureDomainMasters() {
    // Package master top-ups for migration/engine paths (wizard uses the
    // model methods instead — same rows, caches handled there). Raw SQL on
    // purpose: this file links into EVERY target (app, both suites, CLI),
    // while the Q_OBJECT models do not. Row lists mirror
    // AccountGroupsModel::ensureDomainGroups and
    // VoucherTypeModel::ensureDomainTypes 1:1 — change them together.
    // Idempotent (INSERT-if-absent); never deletes. Cache note: pipeline/
    // classifier caches refresh on next load or firm switch; migration and
    // wizard flows both end with the user landing on fresh views.
    const FeatureFlags eff = FeatureFlags::resolveEffective();
    struct DomainGroup {
        const char* name; const char* parent; const char* nature; const char* tag;
        int c1, c2, c3, c4, extractBs;
    };
    auto topUpGroups = [&](std::initializer_list<DomainGroup> list) {
        for (const auto& g : list) {
            QVariantList rows = executeQuery(
                "SELECT id FROM account_groups WHERE name = ? LIMIT 1;", {g.name});
            if (!rows.isEmpty()) continue;
            executeNonQuery(
                "INSERT INTO account_groups (name, parent_group_name, nature, description, "
                "extract_in_balance_sheet, is_system, code1st, code2nd, code3rd, code4th) "
                "VALUES (?, ?, ?, ?, ?, 0, ?, ?, ?, ?);",
                {g.name, g.parent, g.nature, g.tag, g.extractBs, g.c1, g.c2, g.c3, g.c4});
        }
    };
    struct DomainType {
        const char* code; const char* name; const char* category; const char* prefix; int order;
    };
    auto topUpTypes = [&](std::initializer_list<DomainType> list) {
        for (const auto& t : list) {
            QVariantList rows = executeQuery(
                "SELECT id FROM voucher_types WHERE code = ? LIMIT 1;", {t.code});
            if (!rows.isEmpty()) continue;
            executeNonQuery(
                "INSERT INTO voucher_types (code, name, category, is_system, prefix, "
                "numbering_method, separator, number_width, reset_each_fy, "
                "affects_stock, requires_party, requires_stock, requires_gst, sort_order, is_active) "
                "VALUES (?, ?, ?, 0, ?, 'AUTO', '-', 4, 1, 0, 1, 0, 0, ?, 1);",
                {t.code, t.name, t.category, t.prefix, t.order});
        }
    };
    if (eff.mandi) {
        topUpGroups({
            {"Zimidara Debtors", "Sundry Debtors", "Assets", "Domain: mandi", 32, 20, 6, 0, 1},
            {"Zimidara Creditors", "Sundry Creditors", "Liabilities", "Domain: mandi", 33, 11, 9, 0, 1},
            {"Mandi Debtors", "Sundry Debtors", "Assets", "Domain: mandi", 34, 20, 6, 0, 1},
            {"Local Mandi Creditors", "Sundry Creditors", "Liabilities", "Domain: mandi", 31, 11, 9, 0, 1},
        });
        topUpTypes({
            {"JFRM", "J-Form (Mandi Purchase)", "PURCHASE", "JF-", 110},
            {"IFRM", "I-Form (Mandi Sale)", "SALES", "IF-", 111},
        });
    }
    if (eff.manufacturer) {
        topUpGroups({
            {"Manufacturing Group", "Primary", "Expense", "Domain: manufacturer", 22, 0, 0, 0, 0},
            {"Manufacturing Exp.", "Manufacturing Group", "Expense", "Domain: manufacturer", 23, 22, 0, 0, 0},
        });
        topUpTypes({
            {"MANU", "manufacturing Production", "OTHER", "MF-", 120},
        });
    }
}

QString DatabaseManager::getSetting(const QString& key, const QString& defaultVal) {
    if (key.trimmed().isEmpty()) return defaultVal;
    QVariant v = executeScalar("SELECT value FROM app_settings WHERE key = ? LIMIT 1;", {key.trimmed()});
    if (v.isValid() && !v.isNull()) {
        return v.toString();
    }
    return defaultVal;
}

bool DatabaseManager::setSetting(const QString& key, const QString& val) {
    if (key.trimmed().isEmpty()) return false;
    return executeNonQuery(
        "INSERT INTO app_settings (key, value) VALUES (?, ?) "
        "ON CONFLICT(key) DO UPDATE SET value = excluded.value;",
        {key.trimmed(), val}
    );
}

bool DatabaseManager::logAudit(const QString& action, const QString& tableName,
                               const QString& rowId, const QString& beforeJson,
                               const QString& afterJson, const QString& voucherNo,
                               const QString& fyName) {
    if (action.trimmed().isEmpty() || tableName.trimmed().isEmpty()) return false;
    const QString actor = getSetting("audit_actor", "local");
    return executeNonQuery(
        "INSERT INTO audit_log (actor, action, table_name, row_id, voucher_no, fy_name, before_json, after_json) "
        "VALUES (?, ?, ?, ?, ?, ?, ?, ?);",
        {actor, action.trimmed().toUpper(), tableName.trimmed(), rowId, voucherNo, fyName, beforeJson, afterJson}
    );
}

DatabaseManager::DocumentMetadata DatabaseManager::resolveDocumentMetadata(
    const QString& voucherNo, const QString& invoiceNo,
    const QString& existingTransport, const QString& existingBroker,
    const QString& existingVehicle, const QString& existingDriver)
{
    DocumentMetadata meta;
    meta.transport = existingTransport.trimmed();
    meta.brokerName = existingBroker.trimmed();
    meta.vehicleNo = existingVehicle.trimmed().toUpper();
    meta.driverName = existingDriver.trimmed();

    // If transport, broker, vehicle, and driver are already non-empty, return immediately (0 queries!)
    if (!meta.transport.isEmpty() && !meta.brokerName.isEmpty() && !meta.vehicleNo.isEmpty() && !meta.driverName.isEmpty()) {
        return meta;
    }

    const QString vNo = voucherNo.trimmed();
    const QString invNo = invoiceNo.trimmed();
    if (vNo.isEmpty() && invNo.isEmpty()) {
        return meta;
    }

    // 1 single consolidated query with indexed subqueries
    static const QString sql = QStringLiteral(
        "SELECT "
        "  COALESCE("
        "    NULLIF(TRIM(?1), ''),"
        "    (SELECT transport FROM transport_dispatches WHERE (voucher_no = ?3 OR invoice_no = ?4) AND transport IS NOT NULL AND TRIM(transport) != '' LIMIT 1),"
        "    (SELECT transport FROM gate_register WHERE (linked_voucher_no = ?3 OR weighbridge_slip_no = ?4) AND transport IS NOT NULL AND TRIM(transport) != '' LIMIT 1),"
        "    (SELECT transport FROM vouchers WHERE (voucher_no = ?3 OR instrument_no = ?4) AND transport IS NOT NULL AND TRIM(transport) != '' LIMIT 1)"
        "  ) AS transport,"
        "  COALESCE("
        "    NULLIF(TRIM(?2), ''),"
        "    (SELECT broker_name FROM vouchers WHERE (voucher_no = ?3 OR instrument_no = ?4) AND broker_name IS NOT NULL AND TRIM(broker_name) != '' LIMIT 1),"
        "    (SELECT broker_name FROM transactions WHERE (voucher_no = ?3 OR invoice_no = ?4) AND broker_name IS NOT NULL AND TRIM(broker_name) != '' LIMIT 1)"
        "  ) AS broker_name,"
        "  COALESCE("
        "    NULLIF(TRIM(?5), ''),"
        "    (SELECT vehicle_no FROM vouchers WHERE (voucher_no = ?3 OR instrument_no = ?4) AND vehicle_no IS NOT NULL AND TRIM(vehicle_no) != '' LIMIT 1),"
        "    (SELECT vehicle_no FROM transport_dispatches WHERE (voucher_no = ?3 OR invoice_no = ?4) AND vehicle_no IS NOT NULL AND TRIM(vehicle_no) != '' LIMIT 1)"
        "  ) AS vehicle_no,"
        "  COALESCE("
        "    NULLIF(TRIM(?6), ''),"
        "    (SELECT driver_name FROM vouchers WHERE (voucher_no = ?3 OR instrument_no = ?4) AND driver_name IS NOT NULL AND TRIM(driver_name) != '' LIMIT 1),"
        "    (SELECT driver_name FROM transport_dispatches WHERE (voucher_no = ?3 OR invoice_no = ?4) AND driver_name IS NOT NULL AND TRIM(driver_name) != '' LIMIT 1)"
        "  ) AS driver_name;"
    );

    QVariantList rows = executeQuery(sql, {meta.transport, meta.brokerName, vNo, invNo, meta.vehicleNo, meta.driverName});
    if (!rows.isEmpty()) {
        QVariantMap m = rows.first().toMap();
        if (meta.transport.isEmpty()) meta.transport = m.value("transport").toString().trimmed();
        if (meta.brokerName.isEmpty()) meta.brokerName = m.value("broker_name").toString().trimmed();
        if (meta.vehicleNo.isEmpty()) meta.vehicleNo = m.value("vehicle_no").toString().trimmed().toUpper();
        if (meta.driverName.isEmpty()) meta.driverName = m.value("driver_name").toString().trimmed();
    }
    return meta;
}

