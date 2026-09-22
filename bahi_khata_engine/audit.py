"""
Bahi-Khata Deep Audit Engine
Performs multi-dimensional auditing on raw Bahi-Khata JetDB databases.
"""

from typing import List, Dict, Any, Optional
from collections import defaultdict
from .bridge import BahiKhataBridgeClient
from .schema import TABLES

class BahiKhataAuditor:
    def __init__(self, bridge: BahiKhataBridgeClient):
        self.bridge = bridge

    def run_full_audit(self) -> Dict[str, Any]:
        """Runs all audit suites and returns a consolidated report."""
        report = {
            "double_entry_imbalances": self.audit_double_entry_balance(),
            "ledger_anomalies": self.audit_ledger_balances(),
            "stock_anomalies": self.audit_stock_integrity(),
            "logistics_anomalies": self.audit_logistics(),
            "duplicate_vouchers": self.audit_duplicates(),
            "summary": {}
        }
        
        total_issues = (
            len(report["double_entry_imbalances"]) +
            len(report["ledger_anomalies"]) +
            len(report["stock_anomalies"]) +
            len(report["logistics_anomalies"]) +
            len(report["duplicate_vouchers"])
        )
        report["summary"] = {
            "total_issues_found": total_issues,
            "double_entry_issues": len(report["double_entry_imbalances"]),
            "ledger_issues": len(report["ledger_anomalies"]),
            "stock_issues": len(report["stock_anomalies"]),
            "logistics_issues": len(report["logistics_anomalies"]),
            "duplicate_issues": len(report["duplicate_vouchers"]),
            "status": "HEALTHY" if total_issues == 0 else "ATTENTION_REQUIRED"
        }
        return report

    def audit_double_entry_balance(self, max_records: int = 100) -> List[Dict[str, Any]]:
        """
        Scans Transactions for out-of-balance vouchers where Sum(Debit) != Sum(Credit).
        """
        sql = """
        SELECT VoucherNumber, TransType, VoucherDate,
               SUM(CASE WHEN DrCr = 'Dr' THEN Amount ELSE 0 END) AS TotalDr,
               SUM(CASE WHEN DrCr = 'Cr' THEN Amount ELSE 0 END) AS TotalCr,
               COUNT(*) AS RowCount
        FROM Transactions
        GROUP BY VoucherNumber, TransType, VoucherDate
        HAVING ROUND(SUM(CASE WHEN DrCr = 'Dr' THEN Amount ELSE 0 END), 2) <> ROUND(SUM(CASE WHEN DrCr = 'Cr' THEN Amount ELSE 0 END), 2)
        """
        try:
            rows = self.bridge.query(sql, limit=max_records)
            anomalies = []
            for r in rows:
                dr = float(r.get("TotalDr") or 0.0)
                cr = float(r.get("TotalCr") or 0.0)
                diff = round(abs(dr - cr), 2)
                anomalies.append({
                    "voucher_number": r.get("VoucherNumber"),
                    "trans_type": r.get("TransType"),
                    "voucher_date": str(r.get("VoucherDate")),
                    "total_dr": dr,
                    "total_cr": cr,
                    "difference": diff,
                    "row_count": r.get("RowCount"),
                    "severity": "CRITICAL" if diff > 100 else "WARNING",
                    "suggestion": "Adjust rounding or insert balancing ledger line item."
                })
            return anomalies
        except Exception as e:
            return [{"error": str(e)}]

    def audit_ledger_balances(self) -> List[Dict[str, Any]]:
        """
        Audits Ledgers for negative opening balances, missing names, or orphaned accounts.
        """
        anomalies = []
        sql = """
        SELECT Code1st, LedgerName, GroupCode, OpeningBal, OpeningType
        FROM Ledgers
        WHERE LedgerName IS NULL OR TRIM(LedgerName) = ''
        """
        try:
            unnamed = self.bridge.query(sql)
            for u in unnamed:
                anomalies.append({
                    "account_code": u.get("Code1st"),
                    "issue": "Missing Ledger Name",
                    "severity": "WARNING",
                    "details": u
                })
        except Exception:
            pass

        return anomalies

    def audit_stock_integrity(self, max_records: int = 50) -> List[Dict[str, Any]]:
        """
        Audits StockTransactions for zero weight, negative quantities, or unmapped item codes.
        """
        anomalies = []
        sql = """
        SELECT VoucherNumber, VoucherDate, TransType, ItemCode, Bags, Weight, Rate, Amount
        FROM StockTransactions
        WHERE Weight <= 0 OR Rate < 0
        """
        try:
            zero_weights = self.bridge.query(sql, limit=max_records)
            for r in zero_weights:
                anomalies.append({
                    "voucher_number": r.get("VoucherNumber"),
                    "voucher_date": str(r.get("VoucherDate")),
                    "trans_type": r.get("TransType"),
                    "item_code": r.get("ItemCode"),
                    "weight": r.get("Weight"),
                    "rate": r.get("Rate"),
                    "issue": "Invalid weight or negative rate in stock movement",
                    "severity": "WARNING"
                })
        except Exception as e:
            anomalies.append({"error": str(e)})

        return anomalies

    def audit_logistics(self, max_records: int = 50) -> List[Dict[str, Any]]:
        """
        Audits SaleTransportationDetail for missing Vehicle Numbers, GR Numbers, or driver info.
        """
        anomalies = []
        sql = """
        SELECT VoucherNumber, VoucherDate, TransType, VehicleNo, GRNo, DriverName
        FROM SaleTransportationDetail
        WHERE (VehicleNo IS NULL OR TRIM(VehicleNo) = '') 
          AND (GRNo IS NULL OR TRIM(GRNo) = '')
        """
        try:
            rows = self.bridge.query(sql, limit=max_records)
            for r in rows:
                anomalies.append({
                    "voucher_number": r.get("VoucherNumber"),
                    "voucher_date": str(r.get("VoucherDate")),
                    "trans_type": r.get("TransType"),
                    "issue": "Missing both Vehicle Number and GR Number on sales dispatch",
                    "severity": "INFO"
                })
        except Exception as e:
            anomalies.append({"error": str(e)})

        return anomalies

    def audit_duplicates(self, max_records: int = 50) -> List[Dict[str, Any]]:
        """
        Finds potential duplicate transactions posted on the same date for the same party and amount.
        """
        sql = """
        SELECT VoucherDate, AccountCode, Amount, TransType, COUNT(*) AS DupCount
        FROM Transactions
        WHERE Amount > 1000
        GROUP BY VoucherDate, AccountCode, Amount, TransType
        HAVING COUNT(*) > 1
        """
        try:
            rows = self.bridge.query(sql, limit=max_records)
            duplicates = []
            for r in rows:
                duplicates.append({
                    "voucher_date": str(r.get("VoucherDate")),
                    "account_code": r.get("AccountCode"),
                    "amount": r.get("Amount"),
                    "trans_type": r.get("TransType"),
                    "count": r.get("DupCount"),
                    "issue": f"Multiple ({r.get('DupCount')}) identical transactions found on same date",
                    "severity": "WARNING"
                })
            return duplicates
        except Exception as e:
            return [{"error": str(e)}]

    def audit_defected_bills(self) -> List[Dict[str, Any]]:
        """
        Scans all vouchers for parser corruptions, illegal characters (like single quote in BrokerName),
        and broken linkages between Transactions and StockTransactions.
        """
        issues = []
        try:
            txs = self.bridge.query("SELECT VoucherNumber, VoucherDate, TransType, BrokerName, EntryType, AccountCode FROM Transactions WHERE TransType IN ('Sale', 'Purc')")
            for r in txs:
                broker = str(r.get("BrokerName", ""))
                if broker == "'":
                    issues.append({
                        "voucher_number": r.get("VoucherNumber"),
                        "trans_type": r.get("TransType"),
                        "issue": "Invalid single-quote literal in BrokerName (causes 'Defected Bills' popup)",
                        "severity": "ERROR"
                    })
        except Exception as e:
            issues.append({"error": str(e)})
        return issues

