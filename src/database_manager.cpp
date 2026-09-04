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

    // High performance WAL mode
    executeNonQuery("PRAGMA journal_mode=WAL;");
    executeNonQuery("PRAGMA synchronous=NORMAL;");
    executeNonQuery("PRAGMA foreign_keys=ON;");

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
    // 0. Company Info
    executeNonQuery(
        "CREATE TABLE IF NOT EXISTS company_info ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "company_name TEXT NOT NULL,"
        "firm_type TEXT DEFAULT 'Partnership Firm',"
        "business_type TEXT,"
        "address TEXT,"
        "city TEXT,"
        "state TEXT DEFAULT 'Haryana',"
        "state_code TEXT DEFAULT '06',"
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
        "prefix TEXT DEFAULT 'M/s',"
        "group_name TEXT DEFAULT 'Sundry Debtors',"
        "party_type TEXT NOT NULL DEFAULT 'Buyer',"
        "special_type TEXT DEFAULT 'Rice Buyer',"
        "opening_balance REAL DEFAULT 0.0,"
        "balance_type TEXT DEFAULT 'Cr',"
        "mailing_name TEXT,"
        "address TEXT,"
        "city TEXT,"
        "district TEXT,"
        "state TEXT DEFAULT 'Haryana',"
        "state_code TEXT,"
        "pincode TEXT DEFAULT '125055',"
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
        "company_name TEXT DEFAULT 'Mill Master',"
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
        "purchase_return_ledger TEXT DEFAULT 'Purchase Accounts',"
        "sale_ledger TEXT DEFAULT 'Sales Accounts',"
        "sale_return_ledger TEXT DEFAULT 'Sales Accounts',"
        "stock_ledger TEXT DEFAULT 'Stock-in-Hand',"
        "gst_ledger TEXT DEFAULT 'Duties & Taxes',"
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

    // Ensure columns exist on existing databases
    executeNonQuery("ALTER TABLE jform_vouchers ADD COLUMN vehicle_no TEXT;");
    executeNonQuery("ALTER TABLE jform_vouchers ADD COLUMN driver_name TEXT;");
    executeNonQuery("ALTER TABLE jform_vouchers ADD COLUMN gate_pass_no TEXT;");
    executeNonQuery("ALTER TABLE jform_vouchers ADD COLUMN eway_bill_no TEXT;");
    executeNonQuery("ALTER TABLE jform_vouchers ADD COLUMN bill_time TEXT;");
    executeNonQuery("ALTER TABLE jform_vouchers ADD COLUMN sauda_date TEXT;");
    executeNonQuery("ALTER TABLE jform_vouchers ADD COLUMN mandi_place TEXT;");
    executeNonQuery("ALTER TABLE jform_vouchers ADD COLUMN procurement_mode TEXT;");
    executeNonQuery("ALTER TABLE jform_vouchers ADD COLUMN lot_no TEXT;");
    executeNonQuery("ALTER TABLE jform_vouchers ADD COLUMN grade TEXT;");
    executeNonQuery("ALTER TABLE jform_vouchers ADD COLUMN transport_name TEXT;");
    executeNonQuery("ALTER TABLE jform_vouchers ADD COLUMN broker_name TEXT;");
    executeNonQuery("ALTER TABLE jform_vouchers ADD COLUMN challan_no TEXT;");
    executeNonQuery("ALTER TABLE jform_vouchers ADD COLUMN kanda_weight TEXT;");

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
        "FOREIGN KEY (ledger_id) REFERENCES parties(id),"
        "FOREIGN KEY (fy_id) REFERENCES financial_years(id)"
        ");"
    );

    // 9. Account Groups Table
    executeNonQuery(
        "CREATE TABLE IF NOT EXISTS account_groups ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "name TEXT UNIQUE NOT NULL,"
        "parent_group_name TEXT DEFAULT 'Primary',"
        "nature TEXT NOT NULL DEFAULT 'Assets',"
        "description TEXT,"
        "extract_in_balance_sheet INTEGER DEFAULT 1,"
        "is_system INTEGER DEFAULT 0"
        ");"
    );

    // Seed default account groups if table is empty
    QVariant groupCount = executeScalar("SELECT COUNT(*) FROM account_groups;");
    if (!groupCount.isValid() || groupCount.toLongLong() == 0) {
        struct DefGroup { const char* name; const char* parent; const char* nature; const char* desc; };
        DefGroup defaults[] = {
            {"Primary / Root Group", "Primary", "Assets", "Top-level primary root group"},
            {"Sundry Debtors (Buyers)", "Current Assets", "Assets", "Trade debtors and rice buyers"},
            {"Sundry Creditors (Farmers/Vendors)", "Current Liabilities", "Liabilities", "Farmer suppliers and mandi vendors"},
            {"Bank Accounts", "Current Assets", "Assets", "Current and savings bank accounts"},
            {"Cash-in-hand", "Current Assets", "Assets", "Physical cash balance and petty cash"},
            {"Direct Expenses (Hamali/Freight)", "Direct Expenses", "Expense", "Mill labor, hamali, unloading, and transport charges"},
            {"Rice Milling Sales Revenue", "Sales Accounts", "Income", "Revenue from head rice, broken rice, bran & husk sales"},
            {"Paddy Procurement Purchases", "Purchase Accounts", "Expense", "Raw paddy arrivals purchase cost"},
            {"Duties & Taxes (GST)", "Current Liabilities", "Liabilities", "CGST, SGST, IGST, and Mandi Tax liabilities"},
            {"Loans & Liabilities", "Loans (Liability)", "Liabilities", "Bank term loans and working capital credit"},
            {"Stock-in-Hand (Paddy & Rice)", "Current Assets", "Assets", "Raw paddy and finished rice inventory evaluation"}
        };
        for (const auto& g : defaults) {
            executeNonQuery(
                "INSERT INTO account_groups (name, parent_group_name, nature, description, extract_in_balance_sheet, is_system) "
                "VALUES (?, ?, ?, ?, 1, 1);",
                {g.name, g.parent, g.nature, g.desc}
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

    // Ensure all Stock Item columns exist for backward compatibility
    executeNonQuery("ALTER TABLE stock_items ADD COLUMN trading_group TEXT;");
    executeNonQuery("ALTER TABLE stock_items ADD COLUMN group_code INTEGER;");
    executeNonQuery("ALTER TABLE stock_items ADD COLUMN unit_code INTEGER;");
    executeNonQuery("ALTER TABLE stock_items ADD COLUMN rate_calc_on TEXT DEFAULT 'N/A';");
    executeNonQuery("ALTER TABLE stock_items ADD COLUMN auto_adjust_name INTEGER DEFAULT 1;");
    executeNonQuery("ALTER TABLE stock_items ADD COLUMN item_narration TEXT;");
    executeNonQuery("ALTER TABLE stock_items ADD COLUMN capital_goods INTEGER DEFAULT 0;");
    executeNonQuery("ALTER TABLE stock_items ADD COLUMN vat_rate REAL DEFAULT 0.0;");
    executeNonQuery("ALTER TABLE stock_items ADD COLUMN vat_ledger TEXT DEFAULT 'VAT A/c';");
    executeNonQuery("ALTER TABLE stock_items ADD COLUMN surcharge_on_vat REAL DEFAULT 0.0;");
    executeNonQuery("ALTER TABLE stock_items ADD COLUMN vat_against_d1 REAL DEFAULT 0.0;");
    executeNonQuery("ALTER TABLE stock_items ADD COLUMN cst_rate REAL DEFAULT 0.0;");
    executeNonQuery("ALTER TABLE stock_items ADD COLUMN cst_ledger TEXT DEFAULT 'CST A/c';");
    executeNonQuery("ALTER TABLE stock_items ADD COLUMN cst_without_cform REAL DEFAULT 0.0;");
    executeNonQuery("ALTER TABLE stock_items ADD COLUMN dami_ledger TEXT DEFAULT 'Dami A/c';");
    executeNonQuery("ALTER TABLE stock_items ADD COLUMN market_fee_ledger TEXT DEFAULT 'Market Fee A/c';");
    executeNonQuery("ALTER TABLE stock_items ADD COLUMN hrdf_ledger TEXT DEFAULT 'H.R.D.F. A/c';");
    executeNonQuery("ALTER TABLE stock_items ADD COLUMN market_commtt_form_apply INTEGER DEFAULT 0;");
    executeNonQuery("ALTER TABLE stock_items ADD COLUMN market_commtt_coupon_apply INTEGER DEFAULT 0;");
    executeNonQuery("ALTER TABLE stock_items ADD COLUMN dami_calc_on_weight INTEGER DEFAULT 0;");
    executeNonQuery("ALTER TABLE stock_items ADD COLUMN tax_on_qty INTEGER DEFAULT 0;");
    executeNonQuery("ALTER TABLE stock_items ADD COLUMN purchase_return_ledger TEXT DEFAULT 'Purchase Accounts';");
    executeNonQuery("ALTER TABLE stock_items ADD COLUMN sale_return_ledger TEXT DEFAULT 'Sales Accounts';");
    executeNonQuery("ALTER TABLE stock_items ADD COLUMN gst_ledger TEXT DEFAULT 'Duties & Taxes';");
    executeNonQuery("ALTER TABLE stock_items ADD COLUMN bonus_approved REAL DEFAULT 0.0;");
    executeNonQuery("ALTER TABLE stock_items ADD COLUMN labour_rate_unit TEXT DEFAULT 'Packing';");
    executeNonQuery("ALTER TABLE stock_items ADD COLUMN utrai_rate_1 REAL DEFAULT 0.0;");
    executeNonQuery("ALTER TABLE stock_items ADD COLUMN jharai_rate_1 REAL DEFAULT 0.0;");
    executeNonQuery("ALTER TABLE stock_items ADD COLUMN bharai_rate_1 REAL DEFAULT 0.0;");
    executeNonQuery("ALTER TABLE stock_items ADD COLUMN tulai_rate_1 REAL DEFAULT 0.0;");
    executeNonQuery("ALTER TABLE stock_items ADD COLUMN khichai_rate_1 REAL DEFAULT 0.0;");
    executeNonQuery("ALTER TABLE stock_items ADD COLUMN silai_rate_1 REAL DEFAULT 0.0;");
    executeNonQuery("ALTER TABLE stock_items ADD COLUMN loading_rate_1 REAL DEFAULT 0.0;");
    executeNonQuery("ALTER TABLE stock_items ADD COLUMN utrai_rate_2 REAL DEFAULT 0.0;");
    executeNonQuery("ALTER TABLE stock_items ADD COLUMN jharai_rate_2 REAL DEFAULT 0.0;");
    executeNonQuery("ALTER TABLE stock_items ADD COLUMN bharai_rate_2 REAL DEFAULT 0.0;");
    executeNonQuery("ALTER TABLE stock_items ADD COLUMN tulai_rate_2 REAL DEFAULT 0.0;");
    executeNonQuery("ALTER TABLE stock_items ADD COLUMN khichai_rate_2 REAL DEFAULT 0.0;");
    executeNonQuery("ALTER TABLE stock_items ADD COLUMN silai_rate_2 REAL DEFAULT 0.0;");
    executeNonQuery("ALTER TABLE stock_items ADD COLUMN loading_rate_2 REAL DEFAULT 0.0;");
    executeNonQuery("ALTER TABLE stock_items ADD COLUMN utrai_rate_3 REAL DEFAULT 0.0;");
    executeNonQuery("ALTER TABLE stock_items ADD COLUMN jharai_rate_3 REAL DEFAULT 0.0;");
    executeNonQuery("ALTER TABLE stock_items ADD COLUMN bharai_rate_3 REAL DEFAULT 0.0;");
    executeNonQuery("ALTER TABLE stock_items ADD COLUMN tulai_rate_3 REAL DEFAULT 0.0;");
    executeNonQuery("ALTER TABLE stock_items ADD COLUMN khichai_rate_3 REAL DEFAULT 0.0;");
    executeNonQuery("ALTER TABLE stock_items ADD COLUMN silai_rate_3 REAL DEFAULT 0.0;");
    executeNonQuery("ALTER TABLE stock_items ADD COLUMN loading_rate_3 REAL DEFAULT 0.0;");
}
