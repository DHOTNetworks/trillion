#include "database_manager.h"
#include "models/account_classifier.h"
#include "engine/accounting_engine.h"
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
    if (!m_db) return false;

    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(m_db, sql.toUtf8().constData(), -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        qWarning() << "SQL Prepare Error:" << sqlite3_errmsg(m_db) << "in SQL:" << sql;
        return false;
    }

    bindParams(stmt, params);
    rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE && rc != SQLITE_ROW) {
        qWarning() << "SQL Step Error:" << sqlite3_errmsg(m_db) << "in SQL:" << sql;
    }
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

void DatabaseManager::ensureTablesExist() {
    executeNonQuery("BEGIN TRANSACTION;");

    // -1. Application Key-Value Settings
    executeNonQuery(
        "CREATE TABLE IF NOT EXISTS app_settings ("
        "key TEXT PRIMARY KEY,"
        "value TEXT"
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
        "hsn_code TEXT DEFAULT '1006',"
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
        "is_milling_item INTEGER DEFAULT 0,"
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

    // 4. Paddy Procurement Arrivals
    executeNonQuery(
        "CREATE TABLE IF NOT EXISTS paddy_procurement ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "receipt_no TEXT UNIQUE NOT NULL,"
        "arrival_date TEXT NOT NULL,"
        "farmer_id INTEGER,"
        "farmer_name TEXT NOT NULL,"
        "variety TEXT NOT NULL,"
        "bag_count INTEGER NOT NULL,"
        "gross_weight_qtl REAL NOT NULL,"
        "moisture_pct REAL DEFAULT 14.0,"
        "deduction_qtl REAL DEFAULT 0.0,"
        "net_weight_qtl REAL NOT NULL,"
        "rate_per_qtl REAL NOT NULL,"
        "total_amount REAL NOT NULL,"
        "status TEXT DEFAULT 'Unpaid',"
        "payment_mode TEXT DEFAULT 'Pending',"
        "FOREIGN KEY (farmer_id) REFERENCES parties(id)"
        ");"
    );

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

    // 5. Milling Batches & Line Items
    executeNonQuery(
        "CREATE TABLE IF NOT EXISTS milling_batches ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "fy_id INTEGER,"
        "financial_year TEXT DEFAULT 'FY 2025-26',"
        "batch_no TEXT NOT NULL,"
        "batch_date TEXT NOT NULL,"
        "paddy_variety TEXT NOT NULL DEFAULT 'Paddy Basmati',"
        "paddy_input_qtl REAL NOT NULL DEFAULT 0.0,"
        "head_rice_qtl REAL NOT NULL DEFAULT 0.0,"
        "broken_rice_qtl REAL NOT NULL DEFAULT 0.0,"
        "bran_qtl REAL NOT NULL DEFAULT 0.0,"
        "husk_qtl REAL NOT NULL DEFAULT 0.0,"
        "wastage_qtl REAL NOT NULL DEFAULT 0.0,"
        "yield_pct REAL NOT NULL DEFAULT 0.0,"
        "narration TEXT,"
        "FOREIGN KEY (fy_id) REFERENCES financial_years(id)"
        ");"
    );

    executeNonQuery(
        "CREATE TABLE IF NOT EXISTS milling_voucher_items ("
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
        "FOREIGN KEY (batch_id) REFERENCES milling_batches(id),"
        "FOREIGN KEY (item_id) REFERENCES stock_items(id)"
        ");"
    );

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
        "driver TEXT,"
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
        "driver TEXT,"
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

    // 7c. J-Form Mandi Procurement Vouchers
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
        "transport_name TEXT,"
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

    // 8. Transport, Weighbridge (Kanda) & e-Way Dispatches
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
        "transporter_name TEXT,"
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

    // 7d. J-Form Voucher Line Items
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

    // 7d-2. I-Form Mandi Buyer Issue Vouchers
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
        "created_at DATETIME DEFAULT CURRENT_TIMESTAMP,"
        "FOREIGN KEY (buyer_id) REFERENCES parties(id),"
        "FOREIGN KEY (fy_id) REFERENCES financial_years(id)"
        ");"
    );

    // 7d-3. I-Form Voucher Line Items
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

    // 7e. TDS Vouchers
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

    // Seed standard Bahi-Khata default account groups matching Data.001 exactly
    QVariant groupCount = executeScalar("SELECT COUNT(*) FROM account_groups;");
    if (!groupCount.isValid() || groupCount.toLongLong() == 0) {
        struct BahiGroupSeed { int c1, c2, c3, c4; const char* name; const char* parent; const char* nature; int extractBs; };
        static const BahiGroupSeed defaults[] = {
            {0, 0, 0, 0, "Primary", "Primary", "Assets", 1},
            {1, 0, 0, 0, "Capital A/c", "Primary", "Liabilities", 1},
            {2, 0, 0, 0, "Current Assets", "Primary", "Assets", 1},
            {3, 2, 0, 0, "Bank(s) A/c", "Current Assets", "Assets", 1},
            {4, 2, 0, 0, "Cash-In-Hand", "Current Assets", "Assets", 1},
            {6, 2, 0, 0, "Loan & Advances (Assets)", "Current Assets", "Assets", 1},
            {7, 2, 0, 0, "Stock-In-Hand", "Current Assets", "Assets", 0},
            {8, 2, 0, 0, "Sundry Debtors", "Current Assets", "Assets", 1},
            {9, 0, 0, 0, "Current Liabilities", "Primary", "Liabilities", 1},
            {10, 9, 0, 0, "Duties & Taxes", "Current Liabilities", "Liabilities", 1},
            {11, 9, 0, 0, "Sundry Creditors", "Current Liabilities", "Liabilities", 1},
            {12, 9, 0, 0, "Provisions", "Current Liabilities", "Liabilities", 1},
            {13, 9, 0, 0, "Loans (Liability)", "Current Liabilities", "Liabilities", 1},
            {14, 0, 0, 0, "Fixed Assets", "Primary", "Assets", 1},
            {15, 0, 0, 0, "Profit & Loss", "Primary", "Liabilities", 1},
            {16, 15, 0, 0, "Income A/c", "Profit & Loss", "Income", 1},
            {17, 15, 0, 0, "Expenditure A/c", "Profit & Loss", "Expense", 1},
            {18, 0, 0, 0, "Trading Items Stock A/c", "Primary", "Assets", 0},
            {19, 18, 0, 0, "Purchase A/c", "Trading Items Stock A/c", "Expense", 0},
            {20, 18, 0, 0, "Sale A/c", "Trading Items Stock A/c", "Income", 0},
            {21, 0, 0, 0, "Commission Basis Parties A/c", "Primary", "Liabilities", 1},
            {22, 0, 0, 0, "Manufacturing Group", "Primary", "Expense", 0},
            {23, 22, 0, 0, "Manufacturing Exp.", "Manufacturing Group", "Expense", 0},
            {24, 18, 0, 0, "Trading Exp.", "Trading Items Stock A/c", "Expense", 0},
            {25, 0, 0, 0, "Suspense A/c", "Primary", "Liabilities", 1},
            {26, 2, 0, 0, "Deposit (Assets)", "Current Assets", "Assets", 1},
            {27, 0, 0, 0, "Branches / Divisions", "Primary", "Liabilities", 1},
            {28, 13, 9, 0, "Secured Loans", "Loans (Liability)", "Liabilities", 1},
            {29, 13, 9, 0, "Unsecured Loans", "Loans (Liability)", "Liabilities", 1},
            {30, 2, 0, 0, "Security A/c", "Current Assets", "Assets", 1},
            {31, 11, 9, 0, "Local Mandi Creditors", "Sundry Creditors", "Liabilities", 1},
            {32, 8, 2, 0, "Zimidara Debtors", "Sundry Debtors", "Assets", 1},
            {33, 11, 9, 0, "Zimidara Creditors", "Sundry Creditors", "Liabilities", 1},
            {34, 8, 2, 0, "Mandi Debtors", "Sundry Debtors", "Assets", 1},
            {35, 8, 2, 0, "Employees", "Sundry Debtors", "Assets", 1},
            {-1, 0, 0, 0, "Isht Dev", "Primary", "Liabilities", 1}
        };
        for (const auto& g : defaults) {
            executeNonQuery(
                "INSERT INTO account_groups (name, parent_group_name, nature, description, extract_in_balance_sheet, is_system, code1st, code2nd, code3rd, code4th) "
                "VALUES (?, ?, ?, ?, ?, 1, ?, ?, ?, ?);",
                {g.name, g.parent, g.nature, QString("Standard Group #%1").arg(g.c1), g.extractBs, g.c1, g.c2, g.c3, g.c4}
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

    // 14. Bardana (Gunny Bags & Packaging) Transactions
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

    // 15. Gate Inward / Outward Register
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
        "transporter_name TEXT,"
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

    // 16. Sauda (Forward Broker Contracts)
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

    // 17. Dalali / Brokerage Settlements
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
        if (!getCols(table).contains(column.toLower())) {
            executeNonQuery(QString("ALTER TABLE %1 ADD COLUMN %2 %3;").arg(table, column, type));
            tableColsCache[table].insert(column.toLower());
        }
    };

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

    addColumnIfNotExists("transactions", "due_days", "INTEGER DEFAULT 0");
    addColumnIfNotExists("transactions", "place_of_supply", "TEXT");
    addColumnIfNotExists("transactions", "market_type", "TEXT");

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

    QVariant stdGroupCount = executeScalar("SELECT COUNT(*) FROM account_groups WHERE code1st > 0;");
    if (!stdGroupCount.isValid() || stdGroupCount.toInt() < 40) {
        for (const auto& sg : stdGroups) {
            QVariant v = executeScalar("SELECT id FROM account_groups WHERE name = ? OR code1st = ? LIMIT 1;", {sg.name, sg.code1});
            if (v.isValid() && !v.isNull()) {
                executeNonQuery(
                    "UPDATE account_groups SET code1st = ?, code2nd = ?, code3rd = ?, code4th = ?, nature = ? WHERE id = ?;",
                    {sg.code1, sg.code2, sg.code3, sg.code4, sg.nature, v}
                );
            } else {
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
    addColumnIfNotExists("jform_vouchers", "driver_name", "TEXT");
    addColumnIfNotExists("jform_vouchers", "gate_pass_no", "TEXT");
    addColumnIfNotExists("jform_vouchers", "eway_bill_no", "TEXT");
    addColumnIfNotExists("jform_vouchers", "bill_time", "TEXT");
    addColumnIfNotExists("jform_vouchers", "sauda_date", "TEXT");
    addColumnIfNotExists("jform_vouchers", "mandi_place", "TEXT");
    addColumnIfNotExists("jform_vouchers", "procurement_mode", "TEXT");
    addColumnIfNotExists("jform_vouchers", "lot_no", "TEXT");
    addColumnIfNotExists("jform_vouchers", "grade", "TEXT");
    addColumnIfNotExists("jform_vouchers", "transport_name", "TEXT");
    addColumnIfNotExists("jform_vouchers", "broker_name", "TEXT");
    addColumnIfNotExists("jform_vouchers", "challan_no", "TEXT");
    addColumnIfNotExists("jform_vouchers", "kanda_weight", "TEXT");

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

    // TDS Vouchers deposit tracking
    addColumnIfNotExists("tds_vouchers", "challan_id", "INTEGER");
    addColumnIfNotExists("tds_vouchers", "is_deposited", "INTEGER DEFAULT 0");

    // Ensure Standard Bahi-Khata Default System Ledgers Matching Data.001
    struct BahiDefLedger { int code, groupCode; const char* name; const char* balType; };
    static const BahiDefLedger defaultBahiLedgers[] = {
        {2, 15, "Profit & Loss", "Dr"},
        {55, 10, "CST A/c", "Dr"},
        {58, 10, "H.R.D.F. A/c", "Dr"},
        {57, 10, "Market Fee A/c", "Dr"},
        {63, 9, "Bonus A/c", "Dr"},
        {67, 17, "Association Charges A/c", "Dr"},
        {68, 17, "Gaushala Charges A/c", "Dr"},
        {69, 16, "Commission A/c", "Dr"},
        {71, 17, "Insurance Charges A/c", "Dr"},
        {72, 17, "Jaffery A/c", "Dr"},
        {74, 17, "Misc. Exp. a/c", "Dr"},
        {75, 17, "Salary A/c", "Dr"},
        {78, 17, "Labour A/c", "Dr"},
        {79, 17, "Interest A/c", "Dr"},
        {80, 17, "Other Exp. A/c", "Dr"},
        {61, 10, "T.D.S. Payable (Dami/Commission)", "Cr"},
        {54, 10, "VAT A/c", "Dr"},
        {70, 15, "Discount A/c", "Dr"},
        {44, 17, "Round +/- A/c", "Dr"},
        {1, 4, "Cash", "Dr"},
        {377, 25, "Suspense A/c", "Dr"},
        {77, 17, "Freight Inward A/c", "Dr"},
        {505, 18, "Bardan Export", "Dr"},
        {209, 18, "Bardana A/c", "Dr"},
        {882, 18, "Paddy Basmati A/c", "Dr"},
        {501, 18, "Paddy Husk A/c", "Dr"},
        {502, 18, "Rice Broken", "Dr"},
        {503, 14, "Machinery A/c", "Dr"},
        {533, 16, "Quality Claim A/c", "Dr"},
        {542, 17, "Freight Export Rice A/c", "Dr"},
        {544, 17, "Export Expenses", "Dr"},
        {545, 17, "General Expenses", "Dr"},
        {547, 17, "Provident Fund", "Dr"},
        {548, 12, "Imprest A/c", "Dr"},
        {571, 12, "Cheque Issued But Not Cleared", "Cr"},
        {686, -1, "Sh. Ganesh Ji Maharaj", "Cr"},
        {45, 18, "Paddy Parmal A/c", "Dr"},
        {680, 18, "Rice A/c", "Dr"},
        {500, 18, "Rice Bran A/c", "Dr"},
        {522, 18, "Thread A/C", "Dr"},
        {675, 9, "Freight Payable A/c", "Cr"},
        {695, 12, "Provident Fund Payable A/c", "Cr"},
        {697, 12, "Duties & Taxes Payable", "Cr"},
        {582, 12, "Interest Payable A/c", "Cr"},
        {910, 10, "T.D.S. Payable (Labour)", "Cr"},
        {942, 18, "Rice Basmati", "Dr"},
        {943, 10, "SGST A/c", "Dr"},
        {945, 10, "IGST A/c", "Dr"},
        {946, 10, "CESS A/c", "Dr"},
        {947, 10, "Reverse Charge Payable A/c", "Dr"},
        {56, 16, "Dami A/c", "Dr"},
        {65, 7, "Maal Khata A/c", "Dr"},
        {73, 17, "Brokerage A/c", "Dr"},
        {504, 18, "Machinery Repair", "Dr"},
        {543, 17, "Freight Outward A/C", "Dr"},
        {546, 17, "Electricity Charges", "Dr"},
        {76, 17, "Depriciation A/c", "Dr"},
        {498, 10, "T.D.S. Payable (Interest)", "Cr"},
        {944, 10, "CGST A/c", "Dr"},
        {948, 10, "TCS Payable A/c", "Dr"},
        {949, 2, "TCS Receivable A/c", "Dr"},
        {66, 9, "Auction Charges A/c", "Dr"},
        {950, 10, "TDS U/S 194-Q", "Dr"},
        {951, 10, "TDS Payable A/c", "Cr"},
        {952, 17, "Interest on Tax A/c", "Dr"},
        {953, 17, "Penalty on Tax A/c", "Dr"},
        {954, 17, "Discount Allowed A/c", "Dr"},
        {955, 16, "Interest Received A/c", "Cr"}
    };

    // Only insert default Bahi-Khata template ledgers for brand-new, empty, non-migrated databases
    int partyCount = executeScalar("SELECT COUNT(*) FROM parties;").toInt();
    QString isMigrated = getSetting("is_migrated", "0");
    QString srcType = getSetting("firm_source_type", "");
    if (partyCount == 0 && isMigrated != "1" && srcType.isEmpty()) {
        for (const auto& bl : defaultBahiLedgers) {
            QVariant v = executeScalar("SELECT id FROM parties WHERE name = ? OR legacy_id = ? LIMIT 1;", {bl.name, bl.code});
            if (!v.isValid() || v.isNull()) {
                QVariant grp = executeScalar("SELECT id, name FROM account_groups WHERE code1st = ? LIMIT 1;", {bl.groupCode});
                QString gName = AccountClassifier::getStandardGroupName(bl.groupCode);
                qint64 gId = 0;
                if (grp.isValid() && !grp.isNull()) {
                    gId = grp.toLongLong();
                }
                executeNonQuery(
                    "INSERT INTO parties (name, group_name, group_code, group_id, balance_type, legacy_id) "
                    "VALUES (?, ?, ?, ?, ?, ?);",
                    {bl.name, gName, bl.groupCode, gId, bl.balType, bl.code}
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

    executeNonQuery("COMMIT;");

    // Automatically heal and catalog any custom or foreign groups
    AccountClassifier::healAllGroups();
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
