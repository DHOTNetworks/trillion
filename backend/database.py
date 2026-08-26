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
        pincode TEXT,
        
        -- Contact Info
        phone TEXT,
        mobile TEXT,
        whatsapp TEXT,
        email TEXT,
        contact_person TEXT,
        
        -- Statutory & Tax Compliance
        gstin TEXT,
        pan TEXT,
        aadhaar TEXT,
        gst_party_type TEXT DEFAULT 'Unregistered', -- 'Registered', 'Unregistered', 'Composition'
        
        -- Banking Details
        bank_name TEXT,
        bank_account TEXT,
        ifsc_code TEXT,
        
        -- Trading Terms
        credit_limit REAL DEFAULT 0.0,
        credit_days INTEGER DEFAULT 30
    );
    """)

    # 2. Paddy Procurement / Arrival Slips
    cursor.execute("""
    CREATE TABLE IF NOT EXISTS paddy_arrivals (
        id INTEGER PRIMARY KEY AUTOINCREMENT,
        slip_no TEXT UNIQUE NOT NULL,
        arrival_date TEXT NOT NULL,
        farmer_id INTEGER,
        farmer_name TEXT NOT NULL,
        paddy_variety TEXT NOT NULL, -- e.g. Sona Masoori, IR64, Wada Kolam
        bag_count INTEGER NOT NULL,
        gross_weight_qtl REAL NOT NULL,
        moisture_pct REAL DEFAULT 14.0,
        moisture_deduction_qtl REAL DEFAULT 0.0,
        net_weight_qtl REAL NOT NULL,
        rate_per_qtl REAL NOT NULL,
        hamali_charges REAL DEFAULT 0.0,
        net_amount REAL NOT NULL,
        payment_status TEXT DEFAULT 'Unpaid', -- 'Paid', 'Unpaid', 'Partial'
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

    # 5. Financial Vouchers (Cash, Bank, Journal, Debit Note, Credit Note)
    cursor.execute("""
    CREATE TABLE IF NOT EXISTS vouchers (
        id INTEGER PRIMARY KEY AUTOINCREMENT,
        voucher_no TEXT UNIQUE NOT NULL,
        voucher_date TEXT NOT NULL,
        voucher_type TEXT NOT NULL, -- 'Payment', 'Receipt', 'Contra', 'Journal'
        party_name TEXT NOT NULL,
        account_type TEXT NOT NULL, -- 'Cash', 'HDFC Bank', 'SBI Raichur'
        amount REAL NOT NULL,
        narration TEXT
    );
    """)

    # 6. Account Groups Table (TrillionApp AccountGroup Model)
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

    # 7. Inventory Levels
    cursor.execute("""
    CREATE TABLE IF NOT EXISTS inventory (
        id INTEGER PRIMARY KEY AUTOINCREMENT,
        item_code TEXT UNIQUE NOT NULL,
        item_name TEXT NOT NULL,
        category TEXT NOT NULL, -- 'Raw Paddy', 'Finished Rice', 'By-Product'
        unit TEXT DEFAULT 'Qtl',
        current_stock_qtl REAL DEFAULT 0.0,
        reorder_level_qtl REAL DEFAULT 50.0
    );
    """)

    # 8. TrillionApp Comprehensive Stock Items Master Table
    cursor.execute("""
    CREATE TABLE IF NOT EXISTS stock_items (
        id INTEGER PRIMARY KEY AUTOINCREMENT,
        name TEXT UNIQUE NOT NULL,
        code TEXT,
        item_type TEXT DEFAULT 'Finished Rice', -- 'Raw Paddy', 'Finished Rice', 'By-Product', 'Packing Material'
        goods_type TEXT DEFAULT 'Goods',
        company_name TEXT,
        category_name TEXT DEFAULT 'Finished Rice',
        unit TEXT DEFAULT 'Qtl',
        purchase_rate REAL DEFAULT 0.0,
        sale_rate REAL DEFAULT 0.0,
        mrp REAL DEFAULT 0.0,
        discount REAL DEFAULT 0.0,
        hsn_code TEXT,
        gst_rate REAL DEFAULT 0.0,
        cess_rate REAL DEFAULT 0.0,
        packing_kg REAL DEFAULT 26.0,
        market_fee_rate REAL DEFAULT 0.0,
        dami_rate REAL DEFAULT 0.0,
        opening_bags INTEGER DEFAULT 0,
        opening_qty REAL DEFAULT 0.0,
        opening_rate REAL DEFAULT 0.0,
        opening_value REAL DEFAULT 0.0,
        purchase_ledger TEXT DEFAULT 'Paddy Procurement Purchases',
        sale_ledger TEXT DEFAULT 'Rice Milling Sales Revenue',
        stock_ledger TEXT DEFAULT 'Stock-in-Hand (Paddy & Rice)',
        is_milling_item INTEGER DEFAULT 1,
        include_in_trading INTEGER DEFAULT 1,
        calculate_stock INTEGER DEFAULT 1
    );
    """)

    cursor.execute("SELECT COUNT(*) FROM stock_items")
    if cursor.fetchone()[0] == 0:
        default_stock_items = [
            ("Sona Masoori Steam Rice 26kg", "RICE-SONA-26", "Finished Rice", "Goods", "Mahadev Brand", "Finished Rice", "Bags", 0.0, 3850.0, 4200.0, 0.0, "100630", 5.0, 0.0, 26.0, 0.0, 0.0, 0, 0.0, 0.0, 0.0, "Paddy Procurement Purchases", "Rice Milling Sales Revenue", "Stock-in-Hand (Paddy & Rice)", 1, 1, 1),
            ("Paddy Sona Masoori Raw", "PAD-SONA-01", "Raw Paddy", "Goods", "Raw Grain", "Raw Paddy", "Qtl", 2450.0, 0.0, 0.0, 0.0, "100610", 0.0, 0.0, 100.0, 1.5, 2.0, 0, 0.0, 0.0, 0.0, "Paddy Procurement Purchases", "Rice Milling Sales Revenue", "Stock-in-Hand (Paddy & Rice)", 1, 1, 1),
            ("Rice Bran (16% Oil)", "BY-BRAN-01", "By-Product", "Goods", "Mill Output", "By-Product", "Qtl", 0.0, 2400.0, 2600.0, 0.0, "230240", 5.0, 0.0, 50.0, 0.0, 0.0, 0, 0.0, 0.0, 0.0, "Paddy Procurement Purchases", "Rice Milling Sales Revenue", "Stock-in-Hand (Paddy & Rice)", 1, 1, 1),
            ("Paddy Husk Loose", "BY-HUSK-01", "By-Product", "Goods", "Mill Output", "By-Product", "Qtl", 0.0, 650.0, 750.0, 0.0, "230240", 0.0, 0.0, 100.0, 0.0, 0.0, 0, 0.0, 0.0, 0.0, "Paddy Procurement Purchases", "Rice Milling Sales Revenue", "Stock-in-Hand (Paddy & Rice)", 1, 1, 1),
            ("Broken Rice Nakku", "BY-NAKKU-01", "By-Product", "Goods", "Mill Output", "By-Product", "Qtl", 0.0, 2800.0, 3000.0, 0.0, "100640", 5.0, 0.0, 50.0, 0.0, 0.0, 0, 0.0, 0.0, 0.0, "Paddy Procurement Purchases", "Rice Milling Sales Revenue", "Stock-in-Hand (Paddy & Rice)", 1, 1, 1)
        ]
        cursor.executemany("""
        INSERT INTO stock_items (name, code, item_type, goods_type, company_name, category_name, unit, purchase_rate, sale_rate, mrp, discount, hsn_code, gst_rate, cess_rate, packing_kg, market_fee_rate, dami_rate, opening_bags, opening_qty, opening_rate, opening_value, purchase_ledger, sale_ledger, stock_ledger, is_milling_item, include_in_trading, calculate_stock)
        VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
        """, default_stock_items)
        conn.commit()

    conn.commit()

    # Seed initial demo data if empty
    cursor.execute("SELECT COUNT(*) FROM parties")
    if cursor.fetchone()[0] == 0:
        seed_demo_data(cursor)
        conn.commit()

    conn.close()

def seed_demo_data(cursor):
    today = date.today().strftime("%Y-%m-%d")
    yesterday = (date.today() - timedelta(days=1)).strftime("%Y-%m-%d")
    prev_week = (date.today() - timedelta(days=5)).strftime("%Y-%m-%d")

    # Seed Parties with TrillionApp fields
    parties = [
        ("Ramesh Kumar (Farmer)", "RK-FARM", "Shri", "Sundry Creditors", "Farmer", "Paddy Seller", 0.0, "Cr", "Ramesh Kumar", "Village Raichur", "Raichur", "Raichur", "Karnataka", "584101", "9876543210", "9876543210", "9876543210", "ramesh@farmer.com", "Ramesh Kumar", "", "AAAPR1234F", "998877665544", "Unregistered", "SBI Raichur", "30998877665", "SBIN0001234", 100000.0, 30),
        ("Suresh Patil (Farmer)", "SP-FARM", "Shri", "Sundry Creditors", "Farmer", "Paddy Seller", 0.0, "Cr", "Suresh Patil", "Koppal Farm Road", "Koppal", "Koppal", "Karnataka", "583231", "9876543211", "9876543211", "9876543211", "suresh@farmer.com", "Suresh Patil", "", "AAAPS5678G", "887766554433", "Unregistered", "Canara Bank", "40998877661", "CNRB0004321", 150000.0, 30),
        ("Sri Lakshmi Traders", "SLT-BLR", "M/s", "Sundry Debtors", "Buyer", "Rice Buyer", 45000.0, "Dr", "Sri Lakshmi Traders Pvt Ltd", "APMC Yard Market", "Bengaluru", "Bengaluru Urban", "Karnataka", "560002", "9811122233", "9811122233", "9811122233", "sales@srilakshmi.com", "Venkatesh Rao", "29ABCDE1234F1Z5", "ABCDE1234F", "112233445566", "Registered", "HDFC Bank", "50100998877", "HDFC0000123", 500000.0, 45),
        ("Balaji Grain Suppliers", "BGS-HYD", "M/s", "Sundry Debtors", "Buyer", "Rice Buyer", 82000.0, "Dr", "Balaji Grain Suppliers", "Ganj Market", "Hyderabad", "Hyderabad", "Telangana", "500001", "9822233344", "9822233344", "9822233344", "info@balajigrains.in", "Balaji Reddy", "36ABCDE5678F1Z2", "ABCDE5678F", "223344556677", "Registered", "ICICI Bank", "60998877112", "ICIC0000456", 750000.0, 45),
        ("Golden Poultry Feeds", "GPF-HPT", "M/s", "Sundry Debtors", "Buyer", "Rice Buyer", 12500.0, "Dr", "Golden Poultry Feeds", "Industrial Area", "Hospet", "Vijayanagara", "Karnataka", "583201", "9833344455", "9833344455", "9833344455", "purchases@goldenfeeds.com", "Mahesh Kumar", "29XYZPS9988H1Z1", "XYZPS9988H", "334455667788", "Registered", "Axis Bank", "91802001122", "UTIB0000789", 300000.0, 30),
    ]
    cursor.executemany("""
    INSERT INTO parties (name, alias, prefix, group_name, party_type, special_type, opening_balance, balance_type, mailing_name, address, city, district, state, pincode, phone, mobile, whatsapp, email, contact_person, gstin, pan, aadhaar, gst_party_type, bank_name, bank_account, ifsc_code, credit_limit, credit_days)
    VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
    """, parties)

    # Seed Inventory Items
    inventory = [
        ("PAD-SONA", "Paddy (Sona Masoori)", "Raw Paddy", "Qtl", 1450.0, 200.0),
        ("PAD-IR64", "Paddy (IR64)", "Raw Paddy", "Qtl", 820.0, 150.0),
        ("RICE-SONA-1", "Sona Masoori Steam Rice Grade-A", "Finished Rice", "Qtl", 680.0, 100.0),
        ("RICE-RAW-IR64", "IR64 Raw Rice", "Finished Rice", "Qtl", 410.0, 80.0),
        ("BY-BROKEN", "Broken Rice (Nakku)", "By-Product", "Qtl", 95.0, 20.0),
        ("BY-BRAN", "Rice Bran (16% Oil)", "By-Product", "Qtl", 140.0, 30.0),
        ("BY-HUSK", "Paddy Husk", "By-Product", "Qtl", 220.0, 50.0),
    ]
    cursor.executemany("""
    INSERT INTO inventory (item_code, item_name, category, unit, current_stock_qtl, reorder_level_qtl)
    VALUES (?, ?, ?, ?, ?, ?)
    """, inventory)

    # Seed Paddy Arrivals
    arrivals = [
        ("SLIP-1001", prev_week, 1, "Ramesh Kumar (Farmer)", "Sona Masoori", 150, 112.5, 14.5, 0.5, 112.0, 2450.0, 750.0, 275150.0, "Paid"),
        ("SLIP-1002", yesterday, 2, "Suresh Patil (Farmer)", "IR64", 200, 150.0, 16.0, 3.0, 147.0, 2150.0, 1000.0, 317050.0, "Unpaid"),
        ("SLIP-1003", today, 3, "Venkatesh Farmers Co", "Sona Masoori", 280, 210.0, 15.0, 2.1, 207.9, 2480.0, 1400.0, 516992.0, "Unpaid"),
    ]
    cursor.executemany("""
    INSERT INTO paddy_arrivals (slip_no, arrival_date, farmer_id, farmer_name, paddy_variety, bag_count, gross_weight_qtl, moisture_pct, moisture_deduction_qtl, net_weight_qtl, rate_per_qtl, hamali_charges, net_amount, payment_status)
    VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
    """, arrivals)

    # Seed Milling Batches
    milling = [
        ("MB-2026-01", prev_week, "Sona Masoori", 300.0, 201.0, 21.0, 24.0, 51.0, 3.0, 67.0),
        ("MB-2026-02", yesterday, "IR64", 250.0, 167.5, 17.5, 20.0, 42.5, 2.5, 67.0),
    ]
    cursor.executemany("""
    INSERT INTO milling_batches (batch_no, batch_date, paddy_variety, paddy_input_qtl, head_rice_qtl, broken_rice_qtl, bran_qtl, husk_qtl, wastage_qtl, yield_pct)
    VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
    """, milling)

    # Seed Sales Invoices
    sales = [
        ("INV-5001", prev_week, 4, "Sri Lakshmi Traders", "Sona Masoori Steam Rice Grade-A", 200, 100.0, 3850.0, 385000.0, 5.0, 19250.0, 404250.0, "Credit"),
        ("INV-5002", yesterday, 6, "Golden Poultry Feeds", "Rice Bran (16% Oil)", 120, 60.0, 2400.0, 144000.0, 5.0, 7200.0, 151200.0, "Cash"),
    ]
    cursor.executemany("""
    INSERT INTO sales_invoices (invoice_no, invoice_date, customer_id, customer_name, item_name, bag_count, weight_qtl, rate_per_qtl, taxable_amount, gst_pct, gst_amount, total_amount, payment_mode)
    VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
    """, sales)

    # Seed Vouchers
    vouchers = [
        ("VCH-9001", prev_week, "Payment", 1, "Ramesh Kumar (Farmer)", "HDFC Bank", 275150.0, "Payment for Paddy Slip-1001"),
        ("VCH-9002", yesterday, "Receipt", 6, "Golden Poultry Feeds", "Cash", 151200.0, "Cash receipt against INV-5002"),
    ]
    cursor.executemany("""
    INSERT INTO vouchers (voucher_no, voucher_date, voucher_type, party_id, party_name, account_type, amount, narration)
    VALUES (?, ?, ?, ?, ?, ?, ?, ?)
    """, vouchers)
