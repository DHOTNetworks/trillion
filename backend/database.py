import sqlite3
import os
import sys
from datetime import datetime, date, timedelta

def get_db_path():
    base_dir = getattr(sys, '_MEIPASS', os.path.dirname(os.path.abspath(__file__)))
    if os.path.basename(base_dir) == 'backend':
        return os.path.abspath(os.path.join(base_dir, "..", "mahadev_accounting.db"))
    cand = os.path.join(base_dir, "mahadev_accounting.db")
    if os.path.exists(cand):
        return cand
    return os.path.abspath(os.path.join(base_dir, "..", "mahadev_accounting.db"))

DB_FILE = get_db_path()

def get_connection():
    conn = sqlite3.connect(get_db_path())
    conn.row_factory = sqlite3.Row
    return conn

def migrate_db():
    try:
        conn = get_connection()
        cursor = conn.cursor()
        
        # 1. Migrate parties table
        existing_cols = [r[1] for r in cursor.execute("PRAGMA table_info(parties)").fetchall()]
        cols_to_add = {
            "alias": "TEXT",
            "prefix": "TEXT DEFAULT 'M/s'",
            "group_name": "TEXT DEFAULT 'Sundry Debtors'",
            "special_type": "TEXT DEFAULT 'Rice Buyer'",
            "mailing_name": "TEXT",
            "address": "TEXT",
            "city": "TEXT",
            "district": "TEXT",
            "state": "TEXT DEFAULT 'Karnataka'",
            "state_code": "TEXT",
            "pincode": "TEXT",
            "route": "TEXT",
            "mobile": "TEXT",
            "whatsapp": "TEXT",
            "phone": "TEXT",
            "email": "TEXT",
            "contact_person": "TEXT",
            "pan": "TEXT",
            "aadhaar": "TEXT",
            "tan": "TEXT",
            "gstin": "TEXT",
            "gst_party_type": "TEXT DEFAULT 'Unregistered'",
            "bank_name": "TEXT",
            "bank_account": "TEXT",
            "ifsc_code": "TEXT",
            "credit_limit": "REAL DEFAULT 0.0",
            "credit_days": "INTEGER DEFAULT 30",
            "interest_rate": "REAL DEFAULT 0.0",
            "commission_rate": "REAL DEFAULT 0.0",
            "commission_on": "TEXT",
            "apply_tcs": "INTEGER DEFAULT 0",
            "tcs_exempt": "INTEGER DEFAULT 0",
            "legacy_id": "INTEGER"
        }
        for col, col_type in cols_to_add.items():
            if col not in existing_cols:
                cursor.execute(f"ALTER TABLE parties ADD COLUMN {col} {col_type}")
        conn.commit()

        # 1.5 Migrate stock_items table
        item_tables = [r[0] for r in cursor.execute("SELECT name FROM sqlite_master WHERE type='table' AND name='stock_items'").fetchall()]
        if item_tables:
            item_cols = [r[1] for r in cursor.execute("PRAGMA table_info(stock_items)").fetchall()]
            item_cols_to_add = {
                "legacy_code": "INTEGER",
                "sap_code": "TEXT",
                "dami_rate": "REAL DEFAULT 0.0",
                "market_fee_rate": "REAL DEFAULT 0.0",
                "hrdf_rate": "REAL DEFAULT 0.0",
                "utrai_rate": "REAL DEFAULT 0.0",
                "jharai_rate": "REAL DEFAULT 0.0",
                "bharai_rate": "REAL DEFAULT 0.0",
                "tulai_rate": "REAL DEFAULT 0.0",
                "khichai_rate": "REAL DEFAULT 0.0",
                "silai_rate": "REAL DEFAULT 0.0",
                "loading_rate": "REAL DEFAULT 0.0",
                "reverse_charge": "INTEGER DEFAULT 0",
                "item_form_type": "TEXT DEFAULT 'Both'",
                "trading_ledger": "TEXT",
                "gst_ledger": "TEXT",
                "dami_ledger": "TEXT",
                "market_fee_ledger": "TEXT",
                "hrdf_ledger": "TEXT"
            }
            for col, col_type in item_cols_to_add.items():
                if col not in item_cols:
                    cursor.execute(f"ALTER TABLE stock_items ADD COLUMN {col} {col_type}")
            conn.commit()

        # 1.8 Migrate vouchers table
        vch_tables = [r[0] for r in cursor.execute("SELECT name FROM sqlite_master WHERE type='table' AND name='vouchers'").fetchall()]
        if vch_tables:
            vch_cols = [r[1] for r in cursor.execute("PRAGMA table_info(vouchers)").fetchall()]
            vch_cols_to_add = {
                "legacy_type": "TEXT",
                "instrument_no": "TEXT",
                "instrument_date": "TEXT",
                "bank_date": "TEXT",
                "taxable_amount": "REAL DEFAULT 0.0",
                "gst_pct": "REAL DEFAULT 0.0",
                "cgst_amount": "REAL DEFAULT 0.0",
                "sgst_amount": "REAL DEFAULT 0.0",
                "igst_amount": "REAL DEFAULT 0.0",
                "cess_amount": "REAL DEFAULT 0.0",
                "round_off": "REAL DEFAULT 0.0",
                "vehicle_no": "TEXT",
                "gr_no": "TEXT",
                "driver_name": "TEXT",
                "eway_bill_no": "TEXT",
                "broker_name": "TEXT",
                "farmer_name": "TEXT",
                "sauda_date": "TEXT",
                "dami": "REAL DEFAULT 0.0",
                "labour": "REAL DEFAULT 0.0",
                "auction": "REAL DEFAULT 0.0",
                "m_fee": "REAL DEFAULT 0.0",
                "hrdf": "REAL DEFAULT 0.0",
                "other_exp": "REAL DEFAULT 0.0",
                "welfare": "REAL DEFAULT 0.0",
                "dhrmd": "REAL DEFAULT 0.0",
                "sutli": "REAL DEFAULT 0.0",
                "less_amount": "REAL DEFAULT 0.0"
            }
            for col, col_type in vch_cols_to_add.items():
                if col not in vch_cols:
                    cursor.execute(f"ALTER TABLE vouchers ADD COLUMN {col} {col_type}")
            conn.commit()

        # 2. Migrate sales_invoices table
        sales_tables = [r[0] for r in cursor.execute("SELECT name FROM sqlite_master WHERE type='table' AND name='sales_invoices'").fetchall()]
        if sales_tables:
            sales_cols = [r[1] for r in cursor.execute("PRAGMA table_info(sales_invoices)").fetchall()]
            sales_cols_to_add = {
                "voucher_no": "TEXT",
                "gstin": "TEXT",
                "hsn_code": "TEXT",
                "cgst_amount": "REAL DEFAULT 0.0",
                "sgst_amount": "REAL DEFAULT 0.0",
                "igst_amount": "REAL DEFAULT 0.0",
                "round_off": "REAL DEFAULT 0.0",
                "vehicle_no": "TEXT",
                "eway_bill_no": "TEXT",
                "narration": "TEXT",
                "sale_status": "TEXT DEFAULT 'Self Sale'",
                "market_fee_status": "TEXT DEFAULT 'Paid'",
                "dami": "REAL DEFAULT 0.0",
                "labour": "REAL DEFAULT 0.0",
                "auction": "REAL DEFAULT 0.0",
                "m_fee": "REAL DEFAULT 0.0",
                "hrdf": "REAL DEFAULT 0.0",
                "other_exp": "REAL DEFAULT 0.0",
                "welfare": "REAL DEFAULT 0.0",
                "dhrmd": "REAL DEFAULT 0.0",
                "sutli": "REAL DEFAULT 0.0",
                "less_amount": "REAL DEFAULT 0.0",
                "gr_no": "TEXT",
                "driver": "TEXT",
                "bill_time": "TEXT",
                "sauda_date": "TEXT",
                "shipping_address": "TEXT",
                "po_no": "TEXT",
                "grade": "TEXT",
                "kanda_weight": "TEXT",
                "transport": "TEXT",
                "broker_name": "TEXT"
            }
            for col, col_type in sales_cols_to_add.items():
                if col not in sales_cols:
                    cursor.execute(f"ALTER TABLE sales_invoices ADD COLUMN {col} {col_type}")
            conn.commit()

        # 3. Migrate purchase_invoices table
        pur_tables = [r[0] for r in cursor.execute("SELECT name FROM sqlite_master WHERE type='table' AND name='purchase_invoices'").fetchall()]
        if pur_tables:
            pur_cols = [r[1] for r in cursor.execute("PRAGMA table_info(purchase_invoices)").fetchall()]
            pur_cols_to_add = {
                "voucher_no": "TEXT",
                "gstin": "TEXT",
                "hsn_code": "TEXT",
                "cgst_amount": "REAL DEFAULT 0.0",
                "sgst_amount": "REAL DEFAULT 0.0",
                "igst_amount": "REAL DEFAULT 0.0",
                "round_off": "REAL DEFAULT 0.0",
                "vehicle_no": "TEXT",
                "eway_bill_no": "TEXT",
                "narration": "TEXT",
                "sale_status": "TEXT DEFAULT 'Self Sale'",
                "market_fee_status": "TEXT DEFAULT 'Paid'",
                "dami": "REAL DEFAULT 0.0",
                "labour": "REAL DEFAULT 0.0",
                "auction": "REAL DEFAULT 0.0",
                "m_fee": "REAL DEFAULT 0.0",
                "hrdf": "REAL DEFAULT 0.0",
                "other_exp": "REAL DEFAULT 0.0",
                "welfare": "REAL DEFAULT 0.0",
                "dhrmd": "REAL DEFAULT 0.0",
                "sutli": "REAL DEFAULT 0.0",
                "less_amount": "REAL DEFAULT 0.0",
                "gr_no": "TEXT",
                "driver": "TEXT",
                "bill_time": "TEXT",
                "sauda_date": "TEXT",
                "shipping_address": "TEXT",
                "po_no": "TEXT",
                "grade": "TEXT",
                "kanda_weight": "TEXT",
                "transport": "TEXT",
                "broker_name": "TEXT"
            }
            for col, col_type in pur_cols_to_add.items():
                if col not in pur_cols:
                    cursor.execute(f"ALTER TABLE purchase_invoices ADD COLUMN {col} {col_type}")
            conn.commit()
        conn.close()
    except Exception as e:
        print(f"Migration notice: {e}")

def init_db():
    migrate_db()
    conn = get_connection()
    cursor = conn.cursor()
    
    # 1. Parties (Farmers, Buyers, Grain Merchants, Vendors) - TrillionApp Comprehensive Schema
    cursor.execute("""
    CREATE TABLE IF NOT EXISTS parties (
        id INTEGER PRIMARY KEY AUTOINCREMENT,
        name TEXT NOT NULL,
        alias TEXT,
        prefix TEXT DEFAULT 'M/s',
        group_name TEXT DEFAULT 'Sundry Debtors',
        party_type TEXT NOT NULL DEFAULT 'Buyer',
        special_type TEXT DEFAULT 'Rice Buyer',
        
        -- Opening Balance
        opening_balance REAL DEFAULT 0.0,
        balance_type TEXT DEFAULT 'Cr',
        
        -- Address & Location
        mailing_name TEXT,
        address TEXT,
        city TEXT,
        district TEXT,
        state TEXT DEFAULT 'Haryana',
        state_code TEXT,
        pincode TEXT DEFAULT '125055',
        country TEXT DEFAULT 'India',
        route TEXT,
        
        -- Contact Info
        mobile TEXT,
        whatsapp TEXT,
        phone TEXT,
        email TEXT,
        contact_person TEXT,
        
        -- GST & Statutory
        pan TEXT,
        aadhaar TEXT,
        tan TEXT,
        gstin TEXT,
        gst_party_type TEXT DEFAULT 'Unregistered',
        
        -- Banking Details
        bank_name TEXT,
        bank_account TEXT,
        ifsc_code TEXT,
        
        -- Credit Policy
        credit_limit REAL DEFAULT 0.0,
        credit_days INTEGER DEFAULT 30,
        interest_rate REAL DEFAULT 0.0,
        commission_rate REAL DEFAULT 0.0,
        commission_on TEXT,
        apply_tcs INTEGER DEFAULT 0,
        tcs_exempt INTEGER DEFAULT 0,
        legacy_id INTEGER
    );
    """)

    # 1.5 Stock Items Master
    cursor.execute("""
    CREATE TABLE IF NOT EXISTS stock_items (
        id INTEGER PRIMARY KEY AUTOINCREMENT,
        name TEXT NOT NULL,
        code TEXT UNIQUE NOT NULL,
        alias TEXT,
        print_name TEXT,
        item_type TEXT NOT NULL DEFAULT 'Finished Rice',
        goods_type TEXT DEFAULT 'Goods',
        company_name TEXT DEFAULT 'Mill Master',
        category_name TEXT,
        unit TEXT DEFAULT 'Qtl',
        alt_unit TEXT DEFAULT 'Bags',
        conversion_factor REAL DEFAULT 1.0,
        hsn_code TEXT DEFAULT '1006',
        gst_rate REAL DEFAULT 5.0,
        cess_rate REAL DEFAULT 0.0,
        purchase_rate REAL DEFAULT 0.0,
        sale_rate REAL DEFAULT 0.0,
        mrp REAL DEFAULT 0.0,
        min_rate REAL DEFAULT 0.0,
        discount REAL DEFAULT 0.0,
        packing_kg REAL DEFAULT 50.0,
        opening_bags INTEGER DEFAULT 0,
        opening_qty REAL DEFAULT 0.0,
        opening_rate REAL DEFAULT 0.0,
        opening_value REAL DEFAULT 0.0,
        purchase_ledger TEXT DEFAULT 'Purchase Accounts',
        sale_ledger TEXT DEFAULT 'Sales Accounts',
        stock_ledger TEXT DEFAULT 'Stock-in-Hand',
        is_milling_item INTEGER DEFAULT 0,
        include_in_trading INTEGER DEFAULT 1,
        calculate_stock INTEGER DEFAULT 1,
        dami_rate REAL DEFAULT 0.0,
        market_fee_rate REAL DEFAULT 0.0,
        hrdf_rate REAL DEFAULT 0.0,
        legacy_code INTEGER
    );
    """)

    # 2. Paddy Procurement Arrivals
    cursor.execute("""
    CREATE TABLE IF NOT EXISTS paddy_procurement (
        id INTEGER PRIMARY KEY AUTOINCREMENT,
        receipt_no TEXT UNIQUE NOT NULL,
        arrival_date TEXT NOT NULL,
        farmer_id INTEGER,
        farmer_name TEXT NOT NULL,
        variety TEXT NOT NULL,
        bag_count INTEGER NOT NULL,
        gross_weight_qtl REAL NOT NULL,
        moisture_pct REAL DEFAULT 14.0,
        deduction_qtl REAL DEFAULT 0.0,
        net_weight_qtl REAL NOT NULL,
        rate_per_qtl REAL NOT NULL,
        total_amount REAL NOT NULL,
        status TEXT DEFAULT 'Unpaid',
        payment_mode TEXT DEFAULT 'Pending',
        FOREIGN KEY (farmer_id) REFERENCES parties(id)
    );
    """)

    # 3. Milling Production Batches & Line Items
    cursor.execute("""
    CREATE TABLE IF NOT EXISTS milling_batches (
        id INTEGER PRIMARY KEY AUTOINCREMENT,
        fy_id INTEGER,
        financial_year TEXT DEFAULT 'FY 2025-26',
        batch_no TEXT NOT NULL,
        batch_date TEXT NOT NULL,
        paddy_variety TEXT NOT NULL DEFAULT 'Paddy Basmati',
        paddy_input_qtl REAL NOT NULL DEFAULT 0.0,
        head_rice_qtl REAL NOT NULL DEFAULT 0.0,
        broken_rice_qtl REAL NOT NULL DEFAULT 0.0,
        bran_qtl REAL NOT NULL DEFAULT 0.0,
        husk_qtl REAL NOT NULL DEFAULT 0.0,
        wastage_qtl REAL NOT NULL DEFAULT 0.0,
        yield_pct REAL NOT NULL DEFAULT 0.0,
        narration TEXT,
        FOREIGN KEY (fy_id) REFERENCES financial_years(id)
    );
    """)

    cursor.execute("""
    CREATE TABLE IF NOT EXISTS milling_voucher_items (
        id INTEGER PRIMARY KEY AUTOINCREMENT,
        batch_id INTEGER,
        batch_no TEXT NOT NULL,
        batch_date TEXT NOT NULL,
        row_no INTEGER DEFAULT 1,
        drcr TEXT NOT NULL, -- 'Cr' (Consumed) or 'Dr' (Produced)
        item_id INTEGER,
        item_code TEXT,
        item_name TEXT NOT NULL,
        percentage REAL DEFAULT 0.0,
        weight_qtl REAL NOT NULL DEFAULT 0.0,
        bags INTEGER DEFAULT 0,
        rate REAL DEFAULT 0.0,
        amount REAL DEFAULT 0.0,
        narration TEXT,
        FOREIGN KEY (batch_id) REFERENCES milling_batches(id),
        FOREIGN KEY (item_id) REFERENCES stock_items(id)
    );
    """)

    # 4. Sales Invoices (Finished Rice & By-Products)
    cursor.execute("""
    CREATE TABLE IF NOT EXISTS sales_invoices (
        id INTEGER PRIMARY KEY AUTOINCREMENT,
        fy_id INTEGER,
        financial_year TEXT DEFAULT 'FY 2025-26',
        invoice_no TEXT NOT NULL,
        invoice_date TEXT NOT NULL,
        customer_id INTEGER,
        customer_name TEXT NOT NULL,
        gstin TEXT,
        item_id INTEGER,
        item_name TEXT NOT NULL,
        hsn_code TEXT,
        bag_count INTEGER DEFAULT 0,
        weight_qtl REAL NOT NULL,
        rate_per_qtl REAL NOT NULL,
        taxable_amount REAL NOT NULL,
        gst_pct REAL DEFAULT 5.0,
        cgst_amount REAL DEFAULT 0.0,
        sgst_amount REAL DEFAULT 0.0,
        igst_amount REAL DEFAULT 0.0,
        round_off REAL DEFAULT 0.0,
        gst_amount REAL DEFAULT 0.0,
        total_amount REAL NOT NULL,
        payment_mode TEXT DEFAULT 'Credit',
        vehicle_no TEXT,
        eway_bill_no TEXT,
        narration TEXT,
        gr_no TEXT,
        driver_name TEXT,
        driver TEXT,
        broker_name TEXT,
        shipping_address TEXT,
        po_no TEXT,
        distance INTEGER DEFAULT 0,
        irn_no TEXT,
        bill_time TEXT,
        sauda_date TEXT,
        dami REAL DEFAULT 0.0,
        labour REAL DEFAULT 0.0,
        auction REAL DEFAULT 0.0,
        m_fee REAL DEFAULT 0.0,
        hrdf REAL DEFAULT 0.0,
        other_exp REAL DEFAULT 0.0,
        welfare REAL DEFAULT 0.0,
        dhrmd REAL DEFAULT 0.0,
        sutli REAL DEFAULT 0.0,
        less_amount REAL DEFAULT 0.0,
        FOREIGN KEY (customer_id) REFERENCES parties(id),
        FOREIGN KEY (item_id) REFERENCES stock_items(id),
        FOREIGN KEY (fy_id) REFERENCES financial_years(id)
    );
    """)

    # 5. Purchase Invoices (Raw Paddy & Material Purchases)
    cursor.execute("""
    CREATE TABLE IF NOT EXISTS purchase_invoices (
        id INTEGER PRIMARY KEY AUTOINCREMENT,
        fy_id INTEGER,
        financial_year TEXT DEFAULT 'FY 2025-26',
        invoice_no TEXT NOT NULL,
        invoice_date TEXT NOT NULL,
        supplier_id INTEGER,
        supplier_name TEXT NOT NULL,
        gstin TEXT,
        item_id INTEGER,
        item_name TEXT NOT NULL,
        hsn_code TEXT,
        bag_count INTEGER DEFAULT 0,
        weight_qtl REAL NOT NULL,
        rate_per_qtl REAL NOT NULL,
        taxable_amount REAL NOT NULL,
        gst_pct REAL DEFAULT 5.0,
        cgst_amount REAL DEFAULT 0.0,
        sgst_amount REAL DEFAULT 0.0,
        igst_amount REAL DEFAULT 0.0,
        round_off REAL DEFAULT 0.0,
        gst_amount REAL DEFAULT 0.0,
        total_amount REAL NOT NULL,
        payment_mode TEXT DEFAULT 'Credit',
        vehicle_no TEXT,
        eway_bill_no TEXT,
        narration TEXT,
        gr_no TEXT,
        driver_name TEXT,
        driver TEXT,
        broker_name TEXT,
        shipping_address TEXT,
        po_no TEXT,
        distance INTEGER DEFAULT 0,
        irn_no TEXT,
        bill_time TEXT,
        sauda_date TEXT,
        dami REAL DEFAULT 0.0,
        labour REAL DEFAULT 0.0,
        auction REAL DEFAULT 0.0,
        m_fee REAL DEFAULT 0.0,
        hrdf REAL DEFAULT 0.0,
        other_exp REAL DEFAULT 0.0,
        welfare REAL DEFAULT 0.0,
        dhrmd REAL DEFAULT 0.0,
        sutli REAL DEFAULT 0.0,
        less_amount REAL DEFAULT 0.0,
        FOREIGN KEY (supplier_id) REFERENCES parties(id),
        FOREIGN KEY (item_id) REFERENCES stock_items(id),
        FOREIGN KEY (fy_id) REFERENCES financial_years(id)
    );
    """)

    # 6. Financial Vouchers (Cash, Bank, Journal, Debit Note, Credit Note)
    cursor.execute("""
    CREATE TABLE IF NOT EXISTS vouchers (
        id INTEGER PRIMARY KEY AUTOINCREMENT,
        fy_id INTEGER,
        financial_year TEXT DEFAULT 'FY 2025-26',
        voucher_no TEXT NOT NULL,
        voucher_date TEXT NOT NULL,
        voucher_type TEXT NOT NULL,
        legacy_type TEXT,
        ledger_id INTEGER,
        party_name TEXT NOT NULL,
        account_type TEXT NOT NULL,
        amount REAL NOT NULL,
        narration TEXT,
        instrument_no TEXT,
        instrument_date TEXT,
        bank_date TEXT,
        taxable_amount REAL DEFAULT 0.0,
        gst_pct REAL DEFAULT 0.0,
        cgst_amount REAL DEFAULT 0.0,
        sgst_amount REAL DEFAULT 0.0,
        igst_amount REAL DEFAULT 0.0,
        cess_amount REAL DEFAULT 0.0,
        round_off REAL DEFAULT 0.0,
        vehicle_no TEXT,
        gr_no TEXT,
        driver_name TEXT,
        eway_bill_no TEXT,
        broker_name TEXT,
        farmer_name TEXT,
        sauda_date TEXT,
        dami REAL DEFAULT 0.0,
        labour REAL DEFAULT 0.0,
        auction REAL DEFAULT 0.0,
        m_fee REAL DEFAULT 0.0,
        hrdf REAL DEFAULT 0.0,
        other_exp REAL DEFAULT 0.0,
        welfare REAL DEFAULT 0.0,
        dhrmd REAL DEFAULT 0.0,
        sutli REAL DEFAULT 0.0,
        less_amount REAL DEFAULT 0.0,
        FOREIGN KEY (ledger_id) REFERENCES parties(id),
        FOREIGN KEY (fy_id) REFERENCES financial_years(id)
    );
    """)

    # 7. Account Groups Table (TrillionApp AccountGroup Model)
    cursor.execute("""
    CREATE TABLE IF NOT EXISTS account_groups (
        id INTEGER PRIMARY KEY AUTOINCREMENT,
        name TEXT UNIQUE NOT NULL,
        parent_group_name TEXT DEFAULT 'Primary',
        nature TEXT NOT NULL DEFAULT 'Assets', -- 'Assets', 'Liabilities', 'Income', 'Expense'
        description TEXT,
        extract_in_balance_sheet INTEGER DEFAULT 1,
        is_system INTEGER DEFAULT 0
    );
    """)

    cursor.execute("SELECT COUNT(*) FROM account_groups")
    if cursor.fetchone()[0] == 0:
        default_groups = [
            ("Primary / Root Group", "Primary", "Assets", "Top-level primary root group", 1, 1),
            ("Sundry Debtors (Buyers)", "Current Assets", "Assets", "Trade debtors and rice buyers", 1, 1),
            ("Sundry Creditors (Farmers/Vendors)", "Current Liabilities", "Liabilities", "Farmer suppliers and mandi vendors", 1, 1),
            ("Bank Accounts", "Current Assets", "Assets", "Current and savings bank accounts", 1, 1),
            ("Cash-in-hand", "Current Assets", "Assets", "Physical cash balance and petty cash", 1, 1),
            ("Direct Expenses (Hamali/Freight)", "Direct Expenses", "Expense", "Mill labor, hamali, unloading, and transport charges", 1, 1),
            ("Rice Milling Sales Revenue", "Sales Accounts", "Income", "Revenue from head rice, broken rice, bran & husk sales", 1, 1),
            ("Paddy Procurement Purchases", "Purchase Accounts", "Expense", "Raw paddy arrivals purchase cost", 1, 1),
            ("Duties & Taxes (GST)", "Current Liabilities", "Liabilities", "CGST, SGST, IGST, and Mandi Tax liabilities", 1, 1),
            ("Loans & Liabilities", "Loans (Liability)", "Liabilities", "Bank term loans and working capital credit", 1, 1),
            ("Stock-in-Hand (Paddy & Rice)", "Current Assets", "Assets", "Raw paddy and finished rice inventory evaluation", 1, 1)
        ]
        cursor.executemany("""
        INSERT INTO account_groups (name, parent_group_name, nature, description, extract_in_balance_sheet, is_system)
        VALUES (?, ?, ?, ?, ?, ?)
        """, default_groups)
        conn.commit()

    # 8. Inventory Levels
    cursor.execute("""
    CREATE TABLE IF NOT EXISTS inventory (
        id INTEGER PRIMARY KEY AUTOINCREMENT,
        item_code TEXT UNIQUE NOT NULL,
        item_name TEXT NOT NULL,
        category TEXT NOT NULL, -- 'Raw Paddy', 'Finished Rice', 'By-Product'
        current_stock_qtl REAL NOT NULL DEFAULT 0.0,
        reorder_level_qtl REAL DEFAULT 50.0,
        unit TEXT DEFAULT 'Qtl',
        sale_rate REAL DEFAULT 0.0,
        gst_rate TEXT DEFAULT '5%',
        packing_kg INTEGER DEFAULT 50
    );
    """)

    # 9. Financial Years Master
    cursor.execute("""
    CREATE TABLE IF NOT EXISTS financial_years (
        id INTEGER PRIMARY KEY AUTOINCREMENT,
        year_name TEXT UNIQUE NOT NULL,
        start_date TEXT NOT NULL,
        end_date TEXT NOT NULL,
        is_active INTEGER DEFAULT 0
    );
    """)

    # 10. Custom Closing Stocks (Bahi-Khata Audited Year-End Inventories)
    cursor.execute("""
    CREATE TABLE IF NOT EXISTS custom_closing_stocks (
        id INTEGER PRIMARY KEY AUTOINCREMENT,
        fy_id INTEGER,
        financial_year TEXT NOT NULL,
        closing_date TEXT NOT NULL,
        item_id INTEGER,
        item_code TEXT NOT NULL,
        item_name TEXT NOT NULL,
        bags INTEGER DEFAULT 0,
        weight_qtl REAL DEFAULT 0.0,
        rate REAL DEFAULT 0.0,
        amount REAL DEFAULT 0.0,
        FOREIGN KEY (item_id) REFERENCES stock_items(id),
        FOREIGN KEY (fy_id) REFERENCES financial_years(id)
    );
    """)

    # 11. Stock Transactions (Lossless Bahi-Khata Inventory Ledger)
    cursor.execute("""
    CREATE TABLE IF NOT EXISTS stock_transactions (
        id INTEGER PRIMARY KEY AUTOINCREMENT,
        fy_id INTEGER,
        financial_year TEXT,
        voucher_no TEXT,
        voucher_date TEXT NOT NULL,
        trans_type TEXT NOT NULL,
        voucher_type TEXT,
        party_id INTEGER,
        party_name TEXT,
        bill_no TEXT,
        item_id INTEGER,
        item_code TEXT NOT NULL,
        item_name TEXT NOT NULL,
        bags INTEGER DEFAULT 0,
        packing REAL DEFAULT 0.0,
        weight_qtl REAL NOT NULL DEFAULT 0.0,
        rate REAL DEFAULT 0.0,
        amount REAL DEFAULT 0.0,
        taxable_amount REAL DEFAULT 0.0,
        tax REAL DEFAULT 0.0,
        tax_type TEXT,
        narration TEXT,
        row_no INTEGER DEFAULT 1
    );
    """)

    conn.commit()
    conn.close()

if __name__ == "__main__":
    init_db()
    print("Mahadev Accounting Database Initialized Successfully!")
