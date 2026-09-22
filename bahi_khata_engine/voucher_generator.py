"""
Bahi-Khata Voucher Generator Engine
Safely constructs and posts double-entry vouchers directly into Bahi-Khata JetDB.
"""

from typing import List, Dict, Any, Optional
from datetime import datetime
from .bridge import BahiKhataBridgeClient
from .safety import SnapshotManager, AuditTrailLogger, AccountingGuard

class BahiKhataVoucherGenerator:
    def __init__(self, bridge: BahiKhataBridgeClient):
        self.bridge = bridge
        self.snapshot_mgr = SnapshotManager(bridge.db_path)
        self.audit_logger = AuditTrailLogger(bridge.db_path)

    def get_next_voucher_number(self, trans_type: str) -> int:
        """Finds the maximum VoucherNumber for the given TransType and returns max + 1."""
        sql = f"SELECT MAX(VoucherNumber) AS MaxVch FROM Transactions WHERE TransType = '{trans_type}'"
        rows = self.bridge.query(sql)
        if rows and rows[0].get("MaxVch") is not None:
            return int(rows[0]["MaxVch"]) + 1
        return 1

    def post_journal_voucher(self, voucher_date: str, rows: List[Dict[str, Any]], narration: str = "", agent: str = "AIAgent") -> Dict[str, Any]:
        """
        Posts a multi-line Journal Voucher (TransType='Jrnl').
        rows: [{"account_code": 101, "drcr": "Dr", "amount": 5000.0, "narration": "..."}, ...]
        """
        # 1. Validate Accounting Invariants
        trans_rows = [{"DrCr": r["drcr"], "Amount": r["amount"]} for r in rows]
        is_balanced, msg, total_dr, total_cr = AccountingGuard.validate_double_entry(trans_rows)
        if not is_balanced:
            raise ValueError(f"Cannot post unbalanced Journal Voucher: {msg}")

        # 2. Get Next Voucher Number
        vch_no = self.get_next_voucher_number("Jrnl")
        
        # 3. Prepare SQL Statements
        sql_statements = []
        for idx, r in enumerate(rows, start=1):
            acc_code = int(r["account_code"])
            drcr = "Dr" if str(r["drcr"]).upper() in ["DR", "D"] else "Cr"
            amt = float(r["amount"])
            row_narration = (r.get("narration") or narration).replace("'", "''")
            
            sql = f"""
            INSERT INTO Transactions (
                VoucherNumber, VoucherDate, TransType, RowNo, AccountCode, DrCr, Amount, Narration
            ) VALUES (
                {vch_no}, '{voucher_date}', 'Jrnl', {idx}, {acc_code}, '{drcr}', {amt}, '{row_narration}'
            )
            """
            sql_statements.append(sql)

        # 4. Execute with Atomic Snapshot Protection
        snapshot_path = self.snapshot_mgr.create_snapshot(tag="post_voucher")
        try:
            for sql in sql_statements:
                self.bridge.execute(sql)

            self.audit_logger.log(
                action="POST_VOUCHER_JRNL",
                agent=agent,
                reason=f"Post Journal Voucher #{vch_no} amount {total_dr:,.2f}",
                details={"voucher_no": vch_no, "date": voucher_date, "total": total_dr, "rows": len(rows)},
                snapshot_path=snapshot_path
            )

            return {
                "status": "SUCCESS",
                "voucher_number": vch_no,
                "trans_type": "Jrnl",
                "date": voucher_date,
                "amount": total_dr,
                "snapshot": snapshot_path
            }
        except Exception as e:
            self.snapshot_mgr.rollback(snapshot_path)
            raise RuntimeError(f"Failed to post Journal Voucher. Rolled back cleanly. Error: {e}")
