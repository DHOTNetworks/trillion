import sqlite3
import os
from datetime import datetime, date, timedelta

DB_FILE = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "mahadev_accounting.db")

def get_connection():
    conn = sqlite3.connect(DB_FILE)
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
            "pincode": "TEXT",
            "mobile": "TEXT",
            "whatsapp": "TEXT",
            "email": "TEXT",
            "contact_person": "TEXT",
            "pan": "TEXT",
            "aadhaar": "TEXT",
            "gst_party_type": "TEXT DEFAULT 'Unregistered'",
            "bank_name": "TEXT",
            "bank_account": "TEXT",
            "ifsc_code": "TEXT",
            "credit_limit": "REAL DEFAULT 0.0",
            "credit_days": "INTEGER DEFAULT 30"
        }
        for col, col_type in cols_to_add.items():
            if col not in existing_cols:
                cursor.execute(f"ALTER TABLE parties ADD COLUMN {col} {col_type}")
        conn.commit()

        # Seed default Bank Accounts and Sample Parties if not present
        default_banks = [
            ("IndusInd Bank(200999406993)", "INDUS-BANK", "Bank Accounts", "Vendor", "IndusInd Bank", "200999406993", "INDB0000123", 17535.24, "Dr"),
            ("SBI Raichur Main Branch", "SBI-RCH", "Bank Accounts", "Vendor", "State Bank of India", "30998877665", "SBIN0001234", 150000.00, "Dr"),
            ("HDFC Bank MG Road", "HDFC-BLR", "Bank Accounts", "Vendor", "HDFC Bank", "501002003004", "HDFC0000123", 450000.00, "Dr"),
            ("Baksish Kumar Pro. [Sirsa]", "BKP-SIRSA", "Sundry Debtors (Buyers)", "Buyer", "IndusInd Bank", "200999406993", "INDB0000123", 25709746.75, "Dr")
        ]
        for b_name, b_alias, b_group, b_ptype, b_bname, b_bacc, b_ifsc, b_op, b_type in default_banks:
            cursor.execute("SELECT id FROM parties WHERE name = ?", (b_name,))
            if not cursor.fetchone():
                cursor.execute("""
                INSERT INTO parties (name, alias, group_name, party_type, bank_name, bank_account, ifsc_code, opening_balance, balance_type)
                VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?)
                """, (b_name, b_alias, b_group, b_ptype, b_bname, b_bacc, b_ifsc, b_op, b_type))
        conn.commit()

        # 2. Migrate sales_invoices table
        sales_tables = [r[0] for r in cursor.execute("SELECT name FROM sqlite_master WHERE type='table' AND name='sales_invoices'").fetchall()]
        if sales_tables:
            sales_cols = [r[1] for r in cursor.execute("PRAGMA table_info(sales_invoices)").fetchall()]
            sales_cols_to_add = {
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

        # Backfill rich party details for pre-existing records with empty fields
        cursor.execute("""
        UPDATE parties SET 
            alias = COALESCE(NULLIF(alias, ''), 'RK-FARM'),
            group_name = COALESCE(NULLIF(group_name, ''), 'Sundry Creditors (Farmers/Vendors)'),
            party_type = COALESCE(NULLIF(party_type, ''), 'Farmer'),
            special_type = COALESCE(NULLIF(special_type, ''), 'Paddy Seller'),
            mailing_name = COALESCE(NULLIF(mailing_name, ''), name),
            address = COALESCE(NULLIF(address, ''), 'Village Farm Road, APMC Market'),
            city = COALESCE(NULLIF(city, ''), 'Raichur'),
            district = COALESCE(NULLIF(district, ''), 'Raichur'),
            state = COALESCE(NULLIF(state, ''), 'Karnataka'),
            pincode = COALESCE(NULLIF(pincode, ''), '584101'),
            mobile = COALESCE(NULLIF(mobile, ''), COALESCE(phone, '9876543210')),
            whatsapp = COALESCE(NULLIF(whatsapp, ''), COALESCE(phone, '9876543210')),
            email = COALESCE(NULLIF(email, ''), 'ramesh@farmer.com'),
            contact_person = COALESCE(NULLIF(contact_person, ''), name),
            gstin = COALESCE(NULLIF(gstin, ''), '29ABCDE1234F1Z5'),
            pan = COALESCE(NULLIF(pan, ''), 'AAAPR1234F'),
            aadhaar = COALESCE(NULLIF(aadhaar, ''), '998877665544'),
            bank_name = COALESCE(NULLIF(bank_name, ''), 'SBI Raichur Main Branch'),
            bank_account = COALESCE(NULLIF(bank_account, ''), '30998877665'),
            ifsc_code = COALESCE(NULLIF(ifsc_code, ''), 'SBIN0001234')
        WHERE group_name IS NULL OR group_name = '' OR city IS NULL OR city = '' OR district IS NULL OR district = '';
        """)
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
        party_type TEXT NOT NULL DEFAULT 'Farmer', -- 'Farmer', 'Buyer', 'Merchant', 'Vendor'
        special_type TEXT DEFAULT 'Paddy Seller', -- 'Paddy Seller', 'Rice Buyer', 'Mandi Agent', 'Transporter', 'Hamali Contractor'
        
        -- Opening Balance
        opening_balance REAL DEFAULT 0.0,
        balance_type TEXT DEFAULT 'Cr', -- 'Dr' or 'Cr'
        
        -- Address & Location
        mailing_name TEXT,
        address TEXT,
        city TEXT,
        district TEXT,
        state TEXT DEFAULT 'Karnataka',
        pincode TEXT DEFAULT '584101',
        country TEXT DEFAULT 'India',
        
        -- Contact Info
        mobile TEXT,
        whatsapp TEXT,
        phone TEXT,
        email TEXT,
        contact_person TEXT,
        
        -- GST & Statutory
        pan TEXT,
        aadhaar TEXT,
        gstin TEXT,
        gst_party_type TEXT DEFAULT 'Unregistered',
        
        -- Banking Details
        bank_name TEXT,
        bank_account TEXT,
        ifsc_code TEXT,
        
        -- Credit Policy
        credit_limit REAL DEFAULT 0.0,
        credit_days INTEGER DEFAULT 30
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

    # 3. Milling Production Batches
    cursor.execute("""
    CREATE TABLE IF NOT EXISTS milling_batches (
        id INTEGER PRIMARY KEY AUTOINCREMENT,
        batch_no TEXT UNIQUE NOT NULL,
        batch_date TEXT NOT NULL,
        paddy_variety TEXT NOT NULL,
        paddy_input_qtl REAL NOT NULL,
        head_rice_qtl REAL NOT NULL,
        broken_rice_qtl REAL NOT NULL,
        bran_qtl REAL NOT NULL,
        husk_qtl REAL NOT NULL,
        wastage_qtl REAL NOT NULL,
        yield_pct REAL NOT NULL
    );
    """)

    # 4. Sales Invoices (Finished Rice & By-Products)
    cursor.execute("""
    CREATE TABLE IF NOT EXISTS sales_invoices (
        id INTEGER PRIMARY KEY AUTOINCREMENT,
        invoice_no TEXT UNIQUE NOT NULL,
        invoice_date TEXT NOT NULL,
        customer_id INTEGER,
        customer_name TEXT NOT NULL,
        gstin TEXT,
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
        FOREIGN KEY (customer_id) REFERENCES parties(id)
    );
    """)

    # 5. Purchase Invoices (Raw Paddy & Material Purchases)
    cursor.execute("""
    CREATE TABLE IF NOT EXISTS purchase_invoices (
        id INTEGER PRIMARY KEY AUTOINCREMENT,
        invoice_no TEXT UNIQUE NOT NULL,
        invoice_date TEXT NOT NULL,
        supplier_id INTEGER,
        supplier_name TEXT NOT NULL,
        gstin TEXT,
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
        FOREIGN KEY (supplier_id) REFERENCES parties(id)
    );
    """)

    # 6. Financial Vouchers (Cash, Bank, Journal, Debit Note, Credit Note)
    cursor.execute("""
    CREATE TABLE IF NOT EXISTS vouchers (
        id INTEGER PRIMARY KEY AUTOINCREMENT,
        voucher_no TEXT UNIQUE NOT NULL,
        voucher_date TEXT NOT NULL,
        voucher_type TEXT NOT NULL, -- 'Payment', 'Receipt', 'Contra', 'Journal', 'Purchase', 'Sales'
        party_name TEXT NOT NULL,
        account_type TEXT NOT NULL, -- 'Cash', 'HDFC Bank', 'SBI Raichur'
        amount REAL NOT NULL,
        narration TEXT
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

    # Seed initial data if tables are empty
    cursor.execute("SELECT COUNT(*) FROM parties")
    if cursor.fetchone()[0] == 0:
        cursor.executemany("""
        INSERT INTO parties (name, alias, prefix, group_name, party_type, special_type, mailing_name, address, city, district, state, pincode, mobile, whatsapp, email, contact_person, pan, aadhaar, gstin, bank_name, bank_account, ifsc_code)
        VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
        """, [
            ("Ramesh Kumar (Farmer)", "RK-FARM", "M/s", "Sundry Creditors (Farmers/Vendors)", "Farmer", "Paddy Seller", "Ramesh Kumar", "Village Farm Road", "Raichur", "Raichur", "Karnataka", "584101", "9876543210", "9876543210", "ramesh@farmer.com", "Ramesh Kumar", "AAAPR1234F", "998877665544", "29ABCDE1234F1Z5", "SBI Raichur Main Branch", "30998877665", "SBIN0001234"),
            ("Karnataka Rice Traders", "KRT-BLR", "M/s", "Sundry Debtors (Buyers)", "Buyer", "Rice Buyer", "Karnataka Rice Traders", "APMC Yard Market", "Bengaluru", "Bengaluru", "Karnataka", "560001", "9123456789", "9123456789", "orders@krt.com", "Suresh Patel", "BBBPK5678G", "887766554433", "29XYZAB5678C1Z2", "HDFC Bank MG Road", "501002003004", "HDFC0000123"),
            ("Venkateswara Food Supplies", "VFS-HYD", "M/s", "Sundry Debtors (Buyers)", "Buyer", "Rice Buyer", "Venkateswara Food Supplies", "Koti Grain Market", "Hyderabad", "Hyderabad", "Telangana", "500001", "9988112233", "9988112233", "info@vfs.com", "Venkatesh Rao", "CCCPV9012H", "776655443322", "36AAAAA9999A1Z5", "ICICI Bank Abids", "000405006007", "ICIC0000004"),
            ("Sri Rama Traders (Mandi)", "SRT-RCH", "M/s", "Sundry Creditors (Farmers/Vendors)", "Vendor", "Mandi Agent", "Sri Rama Traders", "Cotton Market Road", "Raichur", "Raichur", "Karnataka", "584101", "9448012345", "9448012345", "srirama@mandi.com", "Ramachandra", "DDDPR3456I", "665544332211", "29BBBBB8888B1Z6", "Canara Bank APMC", "110022334455", "CNRB0001100")
        ])

    cursor.execute("SELECT COUNT(*) FROM inventory")
    if cursor.fetchone()[0] == 0:
        cursor.executemany("""
        INSERT INTO inventory (item_code, item_name, category, current_stock_qtl, sale_rate, gst_rate, packing_kg)
        VALUES (?, ?, ?, ?, ?, ?, ?)
        """, [
            ("RICE-SONA-1", "Sona Masoori Raw Rice (50kg)", "Finished Rice", 450.0, 4800.0, "5%", 50),
            ("RICE-STEAM-1", "Sona Masoori Steam Rice (25kg)", "Finished Rice", 320.0, 5200.0, "5%", 25),
            ("RICE-IR64", "IR-64 Raw Rice", "Finished Rice", 600.0, 3600.0, "5%", 50),
            ("BY-BROKEN", "Broken Rice (100%)", "By-Product", 180.0, 2200.0, "5%", 50),
            ("BY-BRAN", "Rice Bran (16% Oil)", "By-Product", 210.0, 2400.0, "5%", 50),
            ("BY-HUSK", "Paddy Husk Loose", "By-Product", 150.0, 650.0, "5%", 50)
        ])

    cursor.execute("SELECT COUNT(*) FROM paddy_procurement")
    if cursor.fetchone()[0] == 0:
        cursor.executemany("""
        INSERT INTO paddy_procurement (receipt_no, arrival_date, farmer_id, farmer_name, variety, bag_count, gross_weight_qtl, moisture_pct, deduction_qtl, net_weight_qtl, rate_per_qtl, total_amount, status, payment_mode)
        VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
        """, [
            ("ARR-1001", "2026-08-20", 1, "Ramesh Kumar (Farmer)", "Sona Masoori Paddy", 120, 61.20, 14.5, 0.60, 60.60, 2350.0, 142410.0, "Paid", "Bank Transfer"),
            ("ARR-1002", "2026-08-22", 4, "Sri Rama Traders (Mandi)", "IR-64 Paddy", 200, 102.50, 15.0, 1.50, 101.00, 2100.0, 212100.0, "Unpaid", "Pending")
        ])

    cursor.execute("SELECT COUNT(*) FROM sales_invoices")
    if cursor.fetchone()[0] == 0:
        cursor.executemany("""
        INSERT INTO sales_invoices (invoice_no, invoice_date, customer_id, customer_name, gstin, item_name, hsn_code, bag_count, weight_qtl, rate_per_qtl, taxable_amount, gst_pct, cgst_amount, sgst_amount, igst_amount, round_off, gst_amount, total_amount, payment_mode, vehicle_no, eway_bill_no, narration)
        VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
        """, [
            ("INV-5001", "2026-08-21", 2, "Karnataka Rice Traders", "29XYZAB5678C1Z2", "Sona Masoori Raw Rice (50kg)", "10063010", 200, 100.0, 4800.0, 480000.0, 5.0, 12000.0, 12000.0, 0.0, 0.0, 24000.0, 504000.0, "Credit", "KA-36-EA-4589", "181002938475", "Sales invoice entry against Order #4029"),
            ("INV-5002", "2026-08-23", 3, "Venkateswara Food Supplies", "36AAAAA9999A1Z5", "Rice Bran (16% Oil)", "23069090", 100, 50.0, 2400.0, 120000.0, 5.0, 0.0, 0.0, 6000.0, 0.0, 6000.0, 126000.0, "Credit", "AP-21-TX-9012", "181002938476", "Sales of rice bran by-product")
        ])

    cursor.execute("SELECT COUNT(*) FROM purchase_invoices")
    if cursor.fetchone()[0] == 0:
        cursor.executemany("""
        INSERT INTO purchase_invoices (invoice_no, invoice_date, supplier_id, supplier_name, gstin, item_name, hsn_code, bag_count, weight_qtl, rate_per_qtl, taxable_amount, gst_pct, cgst_amount, sgst_amount, igst_amount, round_off, gst_amount, total_amount, payment_mode, vehicle_no, eway_bill_no, narration)
        VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
        """, [
            ("PUR-7001", "2026-08-20", 1, "Ramesh Kumar (Farmer)", "29ABCDE1234F1Z5", "Sona Masoori Paddy", "10061010", 120, 60.6, 2350.0, 142410.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 142410.0, "Credit", "KA-36-EA-4589", "181002938475", "Raw paddy purchase arrival")
        ])

    cursor.execute("SELECT COUNT(*) FROM vouchers")
    if cursor.fetchone()[0] == 0:
        cursor.executemany("""
        INSERT INTO vouchers (voucher_no, voucher_date, voucher_type, party_name, account_type, amount, narration)
        VALUES (?, ?, ?, ?, ?, ?, ?)
        """, [
            ("VCH-9001", "2026-08-20", "Payment", "Ramesh Kumar (Farmer)", "SBI Raichur", 142410.0, "Paddy procurement payment for receipt ARR-1001"),
            ("VCH-9002", "2026-08-21", "Receipt", "Karnataka Rice Traders", "HDFC Bank", 250000.0, "Advance receipt against Sales Invoice INV-5001"),
            ("VCH-9003", "2026-08-22", "Payment", "Direct Expenses (Hamali/Freight)", "Cash", 4500.0, "Hamali unloading charges for arrival ARR-1002")
        ])

    conn.commit()
    conn.close()

if __name__ == "__main__":
    init_db()
    print("Mahadev Accounting Database Initialized & Migrated Successfully!")
