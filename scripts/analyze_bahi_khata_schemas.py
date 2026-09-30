import os
import subprocess
import csv
import io
import re
import json

def get_schema(db):
    res = subprocess.run(['mdb-schema', db, 'sqlite'], capture_output=True)
    return res.stdout.decode('latin1')

def get_table_list(db):
    res = subprocess.run(['mdb-tables', '-1', db], capture_output=True)
    lines = res.stdout.decode('latin1').strip().split('\n')
    return [l.strip() for l in lines if l.strip()]

def get_table_data(db, table, limit=10):
    res = subprocess.run(['mdb-export', db, table], capture_output=True)
    text = res.stdout.decode('latin1')
    reader = csv.reader(io.StringIO(text))
    rows = list(reader)
    if not rows:
        return [], []
    headers = rows[0]
    data = rows[1:limit+1]
    return headers, data

def analyze():
    print("=== Analyzing Bahi-Khata Schemas & Pre-filled Data ===")
    tables_001 = get_table_list('Bahi-Khata-Data/Data.001')
    tables_002 = get_table_list('Bahi-Khata-Data/Data.002')
    
    print(f"Data.001 tables: {len(tables_001)}")
    print(f"Data.002 tables: {len(tables_002)}")
    
    # Categorization of tables
    categories = {
        "Accounting & Financial Ledger": [
            "Transactions", "Ledgers", "Groups", "BankAccounts", "BillWiseReceiptVouchers",
            "DebitCreditNotes", "VerifiedVouchers", "ReconcileVouchers", "ChequesList",
            "PostDatedCheques", "EmptyTransactions", "ManualVouchers"
        ],
        "Mandi, Commission & Arhat": [
            "BikriIssueVouchers", "SaudaVouchers", "SaudaTransactions", "ChallanVouchers",
            "DeliveryChallans", "BrokerageVouchers", "LagatBikriVouchers", "BardanaTransactions",
            "FormsVouchers", "FormsVouchersDetails", "FormsNames", "CottonVouchers",
            "CottonBalesDetail", "TimberVouchers", "PowderVouchers", "RiceBranVouchers",
            "RiceHuskVouchers", "MilkRateList", "MilkRateListNew"
        ],
        "Inventory, Stock & Milling": [
            "StockItems", "StockGroups", "StockUnits", "StockTypes", "StockGodowns",
            "StockTransactions", "StockGodownTransactions", "JournalStockTransactions",
            "MillingVouchers", "ItemStockDetails", "ItemOpeningStockDetails",
            "CustomClosingStocks", "CommissionBasisItemStock", "CommissionBasisItemOpeningStock",
            "ItemRateSlabs"
        ],
        "Logistics, Gate & Invoicing": [
            "SaleTransportationDetail", "SaleExpHeading", "BillSeries", "BillImageMargins",
            "GatePassVouchers", "GateRegister", "MemoRegister", "LLFormReceiptNo"
        ],
        "Taxation (GST, TDS, TCS, VAT)": [
            "GSTRates", "TDSRates", "TDSSettings", "TDSDeductions", "TDSDeposits",
            "TDSAcknowLedgementNos", "TaxDeposits"
        ],
        "Configuration, Settings & Masters": [
            "CompanyInfo", "Settings", "BooksSettings", "VoucherSettings", "OtherSettings",
            "LedgerCreationSettings", "JFrmSettings", "InterestSettings", "InterestCDSettings",
            "CustomNarrationForSale", "CustomNarrationForPurc", "CustomNarrationForPymt",
            "CustomNarrationForRcpt", "CustomNarrationForJrnl", "CustomNarrationForChPt",
            "CustomNarrationForChRt", "CustomNarrationForDrCr", "CustomNarrationForIFrm",
            "CustomNarrationForJFrm", "CustomNarrationForMemo", "Notes", "OtherLedgers",
            "ListForSMS", "tblWhatsapp", "LedgerVerifications", "AuditTrail"
        ],
        "Temporary / Report Scratch Tables (JetDB engine)": [
            t for t in tables_002 if t.startswith("Temp") or t.startswith("temp") or t in ["NewTable", "MSysCompactError"]
        ]
    }
    
    print("\n=== Table Breakdown by Subsystem ===")
    for cat, tbls in categories.items():
        print(f"\n{cat} ({len(tbls)} tables):")
        for t in tbls:
            if t in tables_002:
                # get row count
                res = subprocess.run(['mdb-export', '-H', 'Bahi-Khata-Data/Data.002', t], capture_output=True)
                cnt2 = len([l for l in res.stdout.split(b'\n') if l.strip()])
                res1 = subprocess.run(['mdb-export', '-H', 'Bahi-Khata-Data/Data.001', t], capture_output=True) if t in tables_001 else None
                cnt1 = len([l for l in res1.stdout.split(b'\n') if l.strip()]) if res1 else 0
                print(f"  - {t:32} | Data.001 (Template): {cnt1:5} | Data.002 (Active): {cnt2:6}")

if __name__ == '__main__':
    analyze()
