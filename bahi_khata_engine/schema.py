"""
Bahi-Khata JetDB Schema Definitions & Field Mappings
"""

# Voucher Type Standard Codes in Bahi-Khata
VOUCHER_TYPES = {
    "SL": "Sales Invoice",
    "PU": "Purchase Invoice",
    "ChPt": "Cheque Payment",
    "ChRt": "Cheque Receipt",
    "Pymt": "Cash Payment",
    "Rcpt": "Cash Receipt",
    "Jrnl": "Journal Voucher",
    "DrCr": "Debit/Credit Note",
    "Mill": "Milling Production Voucher",
    "JFrm": "J-Form Mandi Purchase",
    "IFrm": "I-Form Mandi Sale",
    "Chln": "Delivery Challan (Bardana)"
}

# Core Tables in Bahi Khata
TABLES = {
    "TRANSACTIONS": "Transactions",
    "LEDGERS": "Ledgers",
    "GROUPS": "Groups",
    "STOCK_ITEMS": "StockItems",
    "STOCK_TRANSACTIONS": "StockTransactions",
    "SALE_TRANSPORTATION": "SaleTransportationDetail",
    "MILLING_VOUCHERS": "MillingVouchers",
    "TDS_DEDUCTIONS": "TDSDeductions",
    "TDS_DEPOSITS": "TDSDeposits",
    "DEBIT_CREDIT_NOTES": "DebitCreditNotes",
    "CUSTOM_CLOSING_STOCKS": "CustomClosingStocks",
    "BANK_ACCOUNTS": "BankAccounts",
    "INTEREST_SETTINGS": "InterestSettings",
    "CHALLAN_VOUCHERS": "ChallanVouchers",
    "VERIFIED_VOUCHERS": "VerifiedVouchers"
}

# GST Tax Slabs
GST_SLABS = [0.0, 5.0, 12.0, 18.0, 28.0]
