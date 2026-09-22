"""
Bahi-Khata Reconciliation Engine
Reconciles Bank Statements, Year-End Closing Stocks, and Inter-Firm Cross-Databases.
"""

from typing import List, Dict, Any, Optional, Tuple
from datetime import datetime
from collections import defaultdict
from .bridge import BahiKhataBridgeClient

def parse_date(d_str: Any) -> Optional[datetime]:
    if not d_str:
        return None
    s = str(d_str).strip()
    for fmt in [
        "%Y-%m-%d %H:%M:%S.%f",
        "%Y-%m-%d %H:%M:%S",
        "%Y-%m-%d",
        "%m/%d/%y %H:%M:%S",
        "%m/%d/%Y %H:%M:%S",
        "%m/%d/%y",
        "%m/%d/%Y",
        "%d/%m/%y",
        "%d/%m/%Y"
    ]:
        try:
            return datetime.strptime(s, fmt)
        except Exception:
            pass
    return None

def norm_str(s: Any) -> str:
    if not s:
        return ""
    return str(s).strip().replace(" ", "").replace("/", "").replace("-", "").upper()

class BahiKhataReconciler:
    def __init__(self, bridge: BahiKhataBridgeClient):
        self.bridge = bridge

    def reconcile_closing_stock(self) -> List[Dict[str, Any]]:
        """Reconciles CustomClosingStocks records with StockItems masters."""
        sql = """
        SELECT c.ClosingStockDate, c.ItemCode, s.ItemName, c.Bags, c.Weight, c.Amount
        FROM CustomClosingStocks c
        LEFT JOIN StockItems s ON c.ItemCode = s.Code1st
        ORDER BY c.ClosingStockDate DESC
        """
        try:
            return self.bridge.query(sql)
        except Exception as e:
            return [{"error": str(e)}]

    def reconcile_bank_account(self, bank_account_code: int, statement_records: List[Dict[str, Any]]) -> Dict[str, Any]:
        """Reconciles a Bahi Khata bank ledger against an external statement list."""
        sql = f"""
        SELECT VoucherNumber, VoucherDate, TransType, DrCr, Amount, Narration, BankDate, VoucherReconcile
        FROM Transactions
        WHERE AccountCode = {bank_account_code}
        ORDER BY VoucherDate
        """
        db_rows = self.bridge.query(sql)

        matched = []
        unmatched_db = []
        unmatched_statement = list(statement_records)

        for d in db_rows:
            d_amt = float(d.get("Amount") or 0.0)
            d_drcr = str(d.get("DrCr")).upper()
            found_match = False

            for s in unmatched_statement:
                s_amt = float(s.get("amount") or 0.0)
                s_expected_drcr = "CR" if d_drcr == "DR" else "DR"
                s_drcr = str(s.get("type", "")).upper()

                if abs(d_amt - s_amt) < 0.01 and s_drcr == s_expected_drcr:
                    matched.append({"voucher": d, "statement": s})
                    unmatched_statement.remove(s)
                    found_match = True
                    break

            if not found_match:
                unmatched_db.append(d)

        return {
            "bank_account_code": bank_account_code,
            "total_db_entries": len(db_rows),
            "matched_count": len(matched),
            "unmatched_db_count": len(unmatched_db),
            "unmatched_statement_count": len(unmatched_statement),
            "matched": matched,
            "unmatched_db": unmatched_db,
            "unmatched_statement": unmatched_statement
        }


class InterFirmReconciler:
    """
    Automated Forensic Cross-Database Reconciler between two Bahi-Khata firms.
    Reconciles Trade Invoices, TDS 194Q, Payments, Receipts, and Closing Ledger Balances.
    """
    def __init__(self, bridge1: BahiKhataBridgeClient, ledger_code1: int,
                 bridge2: BahiKhataBridgeClient, ledger_code2: int,
                 fy_start: str = "2025-04-01", fy_end: str = "2026-03-31"):
        self.b1 = bridge1
        self.code1 = ledger_code1
        self.b2 = bridge2
        self.code2 = ledger_code2
        self.fy_start = datetime.strptime(fy_start, "%Y-%m-%d")
        self.fy_end = datetime.strptime(fy_end + " 23:59:59", "%Y-%m-%d %H:%M:%S")

    def run_reconciliation(self) -> Dict[str, Any]:
        """Runs the complete end-to-end inter-firm forensic audit."""
        # 1. Fetch Company Info
        c1 = self.b1.query("SELECT * FROM CompanyInfo")[0]
        c2 = self.b2.query("SELECT * FROM CompanyInfo")[0]

        # 2. Fetch Ledger Masters
        l1 = self.b1.query(f"SELECT * FROM Ledgers WHERE Code1st = {self.code1}")[0]
        l2 = self.b2.query(f"SELECT * FROM Ledgers WHERE Code1st = {self.code2}")[0]

        # 3. Fetch Transactions
        tx1 = self.b1.query(f"SELECT * FROM Transactions WHERE AccountCode = {self.code1} OR PartyCode = {self.code1}")
        tx2 = self.b2.query(f"SELECT * FROM Transactions WHERE AccountCode = {self.code2} OR PartyCode = {self.code2}")

        # Filter by FY
        tx1_fy = [r for r in tx1 if self._in_fy(r.get("VoucherDate"))]
        tx2_fy = [r for r in tx2 if self._in_fy(r.get("VoucherDate"))]

        # 4. Reconcile Trade: Firm1 Sales vs Firm2 Purchases
        sales1_purc2 = self._reconcile_sales_purchases(
            sales_tx=[r for r in tx1_fy if r.get("TransType") == "Sale"],
            purc_tx=[r for r in tx2_fy if r.get("TransType") == "Purc"],
            seller_name=c1.get("CompanyName"),
            buyer_name=c2.get("CompanyName")
        )

        # 5. Reconcile Trade: Firm2 Sales vs Firm1 Purchases
        sales2_purc1 = self._reconcile_sales_purchases(
            sales_tx=[r for r in tx2_fy if r.get("TransType") == "Sale"],
            purc_tx=[r for r in tx1_fy if r.get("TransType") == "Purc"],
            seller_name=c2.get("CompanyName"),
            buyer_name=c1.get("CompanyName")
        )

        # 6. Reconcile Payments: Firm1 Payments -> Firm2 Receipts
        pay1_rcpt2 = self._reconcile_payments(
            pay_tx=[r for r in tx1_fy if r.get("TransType") == "ChPt"],
            rcpt_tx=[r for r in tx2_fy if r.get("TransType") in ["ChRt", "Jrnl", "Rcpt"]],
            payer_name=c1.get("CompanyName"),
            payee_name=c2.get("CompanyName")
        )

        # 7. Reconcile Payments: Firm2 Payments -> Firm1 Receipts
        pay2_rcpt1 = self._reconcile_payments(
            pay_tx=[r for r in tx2_fy if r.get("TransType") == "ChPt"],
            rcpt_tx=[r for r in tx1_fy if r.get("TransType") in ["ChRt", "Jrnl", "Rcpt"]],
            payer_name=c2.get("CompanyName"),
            payee_name=c1.get("CompanyName")
        )

        # 8. Compute Running Balances & Opening Discrepancy
        balances = self._compute_ledger_balances(l1, tx1, l2, tx2)

        return {
            "firm1": {"name": c1.get("CompanyName"), "gstin": c1.get("GSTIN"), "db": self.b1.db_path, "ledger": l1.get("LedgerName")},
            "firm2": {"name": c2.get("CompanyName"), "gstin": c2.get("GSTIN"), "db": self.b2.db_path, "ledger": l2.get("LedgerName")},
            "sales1_to_purc2": sales1_purc2,
            "sales2_to_purc1": sales2_purc1,
            "payments1_to_rcpt2": pay1_rcpt2,
            "payments2_to_rcpt1": pay2_rcpt1,
            "ledger_balances": balances
        }

    def _in_fy(self, date_val: Any) -> bool:
        dt = parse_date(date_val)
        return bool(dt and self.fy_start <= dt <= self.fy_end)

    def _reconcile_sales_purchases(self, sales_tx: List[Dict[str, Any]], purc_tx: List[Dict[str, Any]], seller_name: str, buyer_name: str) -> Dict[str, Any]:
        sales_by_inv = {}
        for r in sales_tx:
            inv = norm_str(r.get("InvoiceNo") or r.get("Narration") or f"VCH{r.get('VoucherNumber')}")
            sales_by_inv[inv] = r

        purc_by_inv = {}
        for r in purc_tx:
            inv = norm_str(r.get("InvoiceNo") or r.get("Narration") or f"VCH{r.get('VoucherNumber')}")
            purc_by_inv[inv] = r

        matched = []
        omitted_in_buyer = []
        only_in_buyer = []
        diff_amounts = []

        all_invs = sorted(set(list(sales_by_inv.keys()) + list(purc_by_inv.keys())))
        total_tds_calculated = 0.0

        for inv in all_invs:
            s_row = sales_by_inv.get(inv)
            p_row = purc_by_inv.get(inv)

            if s_row and not p_row:
                # Check for possible typo or duplicate
                omitted_in_buyer.append(s_row)
            elif p_row and not s_row:
                only_in_buyer.append(p_row)
            elif s_row and p_row:
                s_amt = float(s_row.get("Amount") or 0.0)
                p_amt = float(p_row.get("Amount") or 0.0)
                # Check TDS 194Q (0.1%)
                expected_tds = round(s_amt * 0.001)
                net_expected = s_amt - expected_tds
                if abs(s_amt - p_amt) <= 2.0 or abs(net_expected - p_amt) <= 2.0:
                    matched.append({"invoice": inv, "sale": s_row, "purc": p_row, "tds_applied": s_amt > p_amt})
                    if s_amt > p_amt:
                        total_tds_calculated += (s_amt - p_amt)
                else:
                    diff_amounts.append({"invoice": inv, "sale_amount": s_amt, "purc_amount": p_amt, "diff": s_amt - p_amt})

        return {
            "seller": seller_name,
            "buyer": buyer_name,
            "sales_count": len(sales_tx),
            "sales_total": sum(float(r.get("Amount") or 0.0) for r in sales_tx),
            "purc_count": len(purc_tx),
            "purc_total": sum(float(r.get("Amount") or 0.0) for r in purc_tx),
            "matched_count": len(matched),
            "omitted_in_buyer": omitted_in_buyer,
            "only_in_buyer": only_in_buyer,
            "amount_mismatches": diff_amounts,
            "total_tds_deducted": round(total_tds_calculated, 2)
        }

    def _reconcile_payments(self, pay_tx: List[Dict[str, Any]], rcpt_tx: List[Dict[str, Any]], payer_name: str, payee_name: str) -> Dict[str, Any]:
        pay_tot = sum(float(r.get("Amount") or 0.0) for r in pay_tx)
        rcpt_tot = sum(float(r.get("Amount") or 0.0) for r in rcpt_tx)
        
        # Match by amounts
        pay_by_amt = defaultdict(list)
        for r in pay_tx:
            pay_by_amt[round(float(r.get("Amount") or 0.0), 2)].append(r)

        rcpt_by_amt = defaultdict(list)
        for r in rcpt_tx:
            rcpt_by_amt[round(float(r.get("Amount") or 0.0), 2)].append(r)

        amt_discrepancies = []
        for amt in sorted(set(list(pay_by_amt.keys()) + list(rcpt_by_amt.keys()))):
            p_list = pay_by_amt.get(amt, [])
            r_list = rcpt_by_amt.get(amt, [])
            if len(p_list) != len(r_list):
                amt_discrepancies.append({
                    "amount": amt,
                    "payer_count": len(p_list),
                    "payee_count": len(r_list)
                })

        return {
            "payer": payer_name,
            "payee": payee_name,
            "payment_count": len(pay_tx),
            "payment_total": pay_tot,
            "receipt_count": len(rcpt_tx),
            "receipt_total": rcpt_tot,
            "variance": round(pay_tot - rcpt_tot, 2),
            "amount_discrepancies": amt_discrepancies
        }

    def _compute_ledger_balances(self, l1: Dict[str, Any], tx1: List[Dict[str, Any]], l2: Dict[str, Any], tx2: List[Dict[str, Any]]) -> Dict[str, Any]:
        def get_balance(l_master, all_tx):
            op_bal = float(l_master.get("OpeningBal") or 0.0)
            op_type = l_master.get("OpeningType")
            running = op_bal if op_type == "Dr" else -op_bal

            # Sum all pre-FY transactions before fy_start
            pre_dr = sum(float(r.get("Amount") or 0.0) for r in all_tx if (parse_date(r.get("VoucherDate")) or datetime(1900,1,1)) < self.fy_start and r.get("DrCr") == "Dr")
            pre_cr = sum(float(r.get("Amount") or 0.0) for r in all_tx if (parse_date(r.get("VoucherDate")) or datetime(1900,1,1)) < self.fy_start and r.get("DrCr") == "Cr")
            
            fy_dr_tx = [r for r in all_tx if self._in_fy(r.get("VoucherDate")) and r.get("DrCr") == "Dr"]
            fy_cr_tx = [r for r in all_tx if self._in_fy(r.get("VoucherDate")) and r.get("DrCr") == "Cr"]
            fy_dr = sum(float(r.get("Amount") or 0.0) for r in fy_dr_tx)
            fy_cr = sum(float(r.get("Amount") or 0.0) for r in fy_cr_tx)

            op_fy = running + pre_dr - pre_cr
            cl_fy = op_fy + fy_dr - fy_cr
            
            gui_dr_total = fy_dr + (op_fy if op_fy > 0 else 0)
            gui_cr_total = fy_cr + (-op_fy if op_fy < 0 else 0)

            return {
                "opening_bal": abs(round(op_fy, 2)),
                "opening_type": "Dr" if op_fy > 0 else "Cr",
                "vch_count_dr": len(fy_dr_tx),
                "vch_count_cr": len(fy_cr_tx),
                "gui_dr_total": round(gui_dr_total, 2),
                "gui_cr_total": round(gui_cr_total, 2),
                "closing_bal": abs(round(cl_fy, 2)),
                "closing_type": "Dr (नामे)" if cl_fy > 0 else "Cr (जमा)",
                "net_closing_signed": cl_fy
            }

        bal1 = get_balance(l1, tx1)
        bal2 = get_balance(l2, tx2)

        # Mirror balance check: Firm 1 Dr must equal Firm 2 Cr
        mirror_closing_diff = round(abs(bal1["net_closing_signed"] + bal2["net_closing_signed"]), 2)
        mirror_opening_diff = round(abs(bal1["opening_bal"] - bal2["opening_bal"]), 2)

        return {
            "firm1_ledger": bal1,
            "firm2_ledger": bal2,
            "opening_match": mirror_opening_diff < 0.01,
            "closing_match": mirror_closing_diff < 0.01,
            "opening_variance": mirror_opening_diff,
            "closing_variance": mirror_closing_diff
        }

