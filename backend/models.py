from PySide6.QtCore import QObject, QAbstractTableModel, Qt, QModelIndex, Property, Signal, Slot
from backend.database import get_connection
from backend.accounting_engine import AccountingEngine
from datetime import datetime, date
import re

def get_fy_and_id_for_date(v_date, cursor):
    if v_date:
        cursor.execute("SELECT id, year_name FROM financial_years WHERE start_date <= ? AND end_date >= ? LIMIT 1", (v_date, v_date))
        row = cursor.fetchone()
        if row:
            return row[0], row[1]
    cursor.execute("SELECT id, year_name FROM financial_years WHERE is_active = 1 LIMIT 1")
    row = cursor.fetchone()
    if row:
        return row[0], row[1]
    return 28, "FY 2026-27"


def increment_invoice_str(inv_str, default_prefix="MRI/2627-"):
    if not inv_str or not inv_str.strip():
        return f"{default_prefix}1"
    inv_clean = inv_str.strip()
    m = re.match(r'^(.*?)(\d+)$', inv_clean)
    if m:
        prefix = m.group(1)
        num_str = m.group(2)
        num_val = int(num_str) + 1
        if num_str.startswith('0') and len(num_str) > 1:
            next_num_str = str(num_val).zfill(len(num_str))
        else:
            next_num_str = str(num_val)
        return f"{prefix}{next_num_str}"
    return f"{inv_clean}-1"

class BaseTableModel(QAbstractTableModel):
    def __init__(self, headers, role_keys, parent=None):
        super().__init__(parent)
        self._headers = headers
        self._role_keys = role_keys
        self._data = []

    def rowCount(self, parent=QModelIndex()):
        return len(self._data)

    def columnCount(self, parent=QModelIndex()):
        return len(self._headers)

    def data(self, index, role=Qt.DisplayRole):
        if not index.isValid() or not (0 <= index.row() < len(self._data)):
            return None
        
        row_dict = self._data[index.row()]
        
        if role == Qt.DisplayRole:
            key = self._role_keys[index.column()]
            return str(row_dict.get(key, ""))
        elif role >= Qt.UserRole:
            role_index = role - Qt.UserRole
            if 0 <= role_index < len(self._role_keys):
                key = self._role_keys[role_index]
                return row_dict.get(key, "")
        return None

    def headerData(self, section, orientation, role=Qt.DisplayRole):
        if orientation == Qt.Horizontal and role == Qt.DisplayRole:
            if 0 <= section < len(self._headers):
                return self._headers[section]
        return None

    def roleNames(self):
        roles = {}
        for idx, key in enumerate(self._role_keys):
            roles[Qt.UserRole + idx] = key.encode('utf-8')
        return roles


class PaddyArrivalsModel(BaseTableModel):
    dataChangedSignal = Signal()

    def __init__(self, parent=None):
        headers = ["Slip No", "Date", "Farmer", "Variety", "Bags", "Gross (Qtl)", "Moist %", "Deduct (Qtl)", "Net (Qtl)", "Rate (₹)", "Net Amt (₹)", "Status"]
        role_keys = ["slip_no", "arrival_date", "farmer_name", "paddy_variety", "bag_count", "gross_weight_qtl", "moisture_pct", "moisture_deduction_qtl", "net_weight_qtl", "rate_per_qtl", "net_amount", "payment_status"]
        super().__init__(headers, role_keys, parent)
        self.reload_data()

    @Slot()
    def reload_data(self):
        self.beginResetModel()
        conn = get_connection()
        cursor = conn.cursor()
        cursor.execute("SELECT * FROM paddy_arrivals ORDER BY id DESC")
        rows = cursor.fetchall()
        self._data = [dict(r) for r in rows]
        conn.close()
        self.endResetModel()
        self.dataChangedSignal.emit()

    @Slot(str, str, str, int, float, float, float, float, str, result=bool)
    def add_arrival(self, farmer_name, paddy_variety, arrival_date, bags, gross_qtl, moisture_pct, rate_qtl, hamali, status):
        try:
            conn = get_connection()
            cursor = conn.cursor()
            
            # Generate slip no
            cursor.execute("SELECT MAX(id) FROM paddy_arrivals")
            max_id = cursor.fetchone()[0] or 1000
            slip_no = f"SLIP-{max_id + 1}"
            
            # Calculate moisture deduction
            moist_ded = AccountingEngine.calculate_moisture_deduction(gross_qtl, moisture_pct)
            net_qtl = max(0.1, round(gross_qtl - moist_ded, 2))
            net_amt = AccountingEngine.calculate_paddy_net_amount(net_qtl, rate_qtl, hamali)
            
            if not arrival_date:
                arrival_date = date.today().strftime("%Y-%m-%d")

            cursor.execute("""
            INSERT INTO paddy_arrivals 
            (slip_no, arrival_date, farmer_id, farmer_name, paddy_variety, bag_count, gross_weight_qtl, moisture_pct, moisture_deduction_qtl, net_weight_qtl, rate_per_qtl, hamali_charges, net_amount, payment_status)
            VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
            """, (slip_no, arrival_date, 1, farmer_name, paddy_variety, bags, gross_qtl, moisture_pct, moist_ded, net_qtl, rate_qtl, hamali, net_amt, status))
            
            # Update stock in inventory dynamically
            cursor.execute("UPDATE inventory SET current_stock_qtl = current_stock_qtl + ? WHERE item_name LIKE ?", (net_qtl, f"%{paddy_variety}%"))
            
            conn.commit()
            conn.close()
            self.reload_data()
            return True
        except Exception as e:
            print(f"Error adding paddy arrival: {e}")
            return False


class MillingModel(BaseTableModel):
    dataChangedSignal = Signal()

    def __init__(self, parent=None):
        headers = ["Batch No", "Date", "Variety", "Paddy Input (Qtl)", "Head Rice (Qtl)", "Broken (Qtl)", "Bran (Qtl)", "Husk (Qtl)", "Wastage (Qtl)", "Yield %"]
        role_keys = ["batch_no", "batch_date", "paddy_variety", "paddy_input_qtl", "head_rice_qtl", "broken_rice_qtl", "bran_qtl", "husk_qtl", "wastage_qtl", "yield_pct"]
        super().__init__(headers, role_keys, parent)
        self.reload_data()

    @Slot()
    def reload_data(self):
        self.beginResetModel()
        conn = get_connection()
        cursor = conn.cursor()
        cursor.execute("SELECT * FROM milling_batches ORDER BY id DESC")
        rows = cursor.fetchall()
        self._data = [dict(r) for r in rows]
        conn.close()
        self.endResetModel()
        self.dataChangedSignal.emit()

    @Slot(result=str)
    @Slot(str, result=str)
    def get_next_batch_no(self, fy=""):
        try:
            conn = get_connection()
            cursor = conn.cursor()
            if not fy:
                cursor.execute("SELECT year_name FROM financial_years WHERE is_active = 1 LIMIT 1")
                row = cursor.fetchone()
                fy = row[0] if row else "FY 2025-26"
            cursor.execute("SELECT batch_no FROM milling_batches WHERE batch_no LIKE 'Mill-%' AND financial_year = ?", (fy,))
            nums = []
            for r in cursor.fetchall():
                parts = (r[0] or "").split("-")
                if len(parts) >= 2 and parts[-1].isdigit():
                    nums.append(int(parts[-1]))
            max_id = max(nums) if nums else 0
            conn.close()
            return f"Mill-{max_id + 1}"
        except Exception as e:
            return "Mill-1"

    @Slot(str, str, str, "QVariantList", "QVariantList", result=bool)
    def add_milling_voucher_full(self, batch_no, batch_date, particulars, consumed_items, produced_items):
        try:
            conn = get_connection()
            cursor = conn.cursor()

            if not batch_date:
                batch_date = date.today().strftime("%Y-%m-%d")

            fy_id, fy_label = get_fy_and_id_for_date(batch_date, cursor)

            if not batch_no:
                batch_no = self.get_next_batch_no(fy_label)

            total_consumed_wt = sum(float(c.get("weight", 0.0) or 0.0) for c in consumed_items)
            total_produced_wt = sum(float(p.get("weight", 0.0) or 0.0) for p in produced_items)

            head_rice = 0.0
            broken_rice = 0.0
            bran = 0.0
            husk = 0.0
            paddy_variety = consumed_items[0].get("itemName", "Paddy Basmati") if consumed_items else "Paddy Basmati"

            for p in produced_items:
                name = str(p.get("itemName", "")).lower()
                wt = float(p.get("weight", 0.0) or 0.0)
                if "broken" in name or "nakku" in name:
                    broken_rice += wt
                elif "bran" in name:
                    bran += wt
                elif "husk" in name:
                    husk += wt
                elif "rice" in name:
                    head_rice += wt
                else:
                    head_rice += wt

            wastage = max(0.0, round(total_consumed_wt - total_produced_wt, 3))
            yield_pct = round((head_rice / total_consumed_wt * 100.0), 2) if total_consumed_wt > 0 else 0.0

            cursor.execute("""
            INSERT INTO milling_batches 
            (fy_id, financial_year, batch_no, batch_date, paddy_variety, paddy_input_qtl, head_rice_qtl, broken_rice_qtl, bran_qtl, husk_qtl, wastage_qtl, yield_pct, narration)
            VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
            """, (fy_id, fy_label, batch_no, batch_date, paddy_variety, total_consumed_wt, head_rice, broken_rice, bran, husk, wastage, yield_pct, particulars))
            batch_db_id = cursor.lastrowid

            # Insert Consumed Items (Cr)
            r_idx = 1
            for c in consumed_items:
                c_name = c.get("itemName", "")
                if c_name and str(c_name).strip():
                    c_wt = float(c.get("weight", 0.0) or 0.0)
                    c_bags = int(c.get("bags", 0) or 0)
                    c_amt = float(c.get("amount", 0.0) or 0.0)
                    c_rate = round(c_amt / c_wt, 2) if c_wt > 0 else 0.0
                    cursor.execute("""
                    INSERT INTO milling_voucher_items (
                        batch_id, batch_no, batch_date, row_no, drcr, item_name, weight_qtl, bags, rate, amount, narration
                    ) VALUES (?, ?, ?, ?, 'Cr', ?, ?, ?, ?, ?, ?)
                    """, (batch_db_id, batch_no, batch_date, r_idx, c_name, c_wt, c_bags, c_rate, c_amt, particulars))
                    r_idx += 1

            # Insert Produced Items (Dr)
            for p in produced_items:
                p_name = p.get("itemName", "")
                if p_name and str(p_name).strip():
                    p_pct = float(p.get("yieldPct", 0.0) or 0.0)
                    p_wt = float(p.get("weight", 0.0) or 0.0)
                    p_bags = int(p.get("bags", 0) or 0)
                    p_amt = float(p.get("amount", 0.0) or 0.0)
                    p_rate = round(p_amt / p_wt, 2) if p_wt > 0 else 0.0
                    cursor.execute("""
                    INSERT INTO milling_voucher_items (
                        batch_id, batch_no, batch_date, row_no, drcr, item_name, percentage, weight_qtl, bags, rate, amount, narration
                    ) VALUES (?, ?, ?, ?, 'Dr', ?, ?, ?, ?, ?, ?, ?)
                    """, (batch_db_id, batch_no, batch_date, r_idx, p_name, p_pct, p_wt, p_bags, p_rate, p_amt, particulars))
                    r_idx += 1

            vch_narration = f"Milling Batch {batch_no}: Input {total_consumed_wt:.3f} Qtl {paddy_variety} -> Produced {total_produced_wt:.3f} Qtl ({yield_pct:.2f}% Yield)"
            if particulars:
                vch_narration += " | " + particulars

            cursor.execute("""
            INSERT INTO vouchers (fy_id, financial_year, voucher_no, voucher_date, voucher_type, legacy_type, party_name, account_type, amount, narration)
            VALUES (?, ?, ?, ?, 'Milling', 'Mill', 'Milling / Production Account', 'Production Account', 0.0, ?)
            """, (fy_id, fy_label, batch_no, batch_date, vch_narration))

            conn.commit()
            conn.close()
            self.reload_data()
            return True
            conn.close()
            self.reload_data()
            return True
        except Exception as e:
            print(f"Error adding milling voucher: {e}")
            return False

    @Slot(str, result=list)
    def get_batch_items(self, batch_no):
        try:
            conn = get_connection()
            cursor = conn.cursor()
            cursor.execute("SELECT * FROM milling_voucher_items WHERE batch_no = ? ORDER BY row_no ASC", (str(batch_no),))
            raw_rows = [dict(r) for r in cursor.fetchall()]
            conn.close()
            for row in raw_rows:
                wt = float(row.get("weight_qtl") or 0.0)
                amt = float(row.get("amount") or 0.0)
                rt = float(row.get("rate") or 0.0)
                row["weight_fmt"] = format_indian_number(wt, decimals=3)
                row["amount_fmt"] = format_indian_currency(amt)
                row["rate_fmt"] = format_indian_currency(rt)
            return raw_rows
        except Exception as e:
            print(f"Error fetching batch items for {batch_no}: {e}")
            return []

    @Slot(result=list)
    @Slot(str, result=list)
    @Slot(str, str, result=list)
    @Slot(str, str, str, result=list)
    def get_milling_statement(self, from_date="", to_date="", variety=""):
        try:
            conn = get_connection()
            cursor = conn.cursor()
            query = "SELECT * FROM milling_batches WHERE 1=1"
            params = []
            if from_date:
                query += " AND batch_date >= ?"
                params.append(from_date)
            if to_date:
                query += " AND batch_date <= ?"
                params.append(to_date)
            if variety and variety != "All Varieties" and variety.strip() != "":
                query += " AND LOWER(paddy_variety) LIKE LOWER(?)"
                params.append(f"%{variety}%")
            query += " ORDER BY batch_date DESC, id DESC"
            cursor.execute(query, params)
            raw_rows = [dict(r) for r in cursor.fetchall()]
            conn.close()
            for row in raw_rows:
                p_in = float(row.get("paddy_input_qtl") or 0.0)
                h_rice = float(row.get("head_rice_qtl") or 0.0)
                b_rice = float(row.get("broken_rice_qtl") or 0.0)
                bran = float(row.get("bran_qtl") or 0.0)
                husk = float(row.get("husk_qtl") or 0.0)
                wast = float(row.get("wastage_qtl") or 0.0)
                y_pct = float(row.get("yield_pct") or 0.0)

                row["paddy_input_fmt"] = format_indian_number(p_in, decimals=3)
                row["head_rice_fmt"] = format_indian_number(h_rice, decimals=3)
                row["broken_rice_fmt"] = format_indian_number(b_rice, decimals=3)
                row["bran_fmt"] = format_indian_number(bran, decimals=3)
                row["husk_fmt"] = format_indian_number(husk, decimals=3)
                row["wastage_fmt"] = format_indian_number(wast, decimals=3)
                row["yield_pct_fmt"] = f"{y_pct:.2f}%"
            return raw_rows
        except Exception as e:
            print(f"Error fetching milling statement: {e}")
            return []

    @Slot(result=dict)
    @Slot(str, str, result=dict)
    def get_milling_totals(self, from_date="", to_date=""):
        try:
            conn = get_connection()
            cursor = conn.cursor()
            query = """
            SELECT 
                COUNT(*) as total_batches,
                SUM(paddy_input_qtl) as total_paddy,
                SUM(head_rice_qtl) as total_head_rice,
                SUM(broken_rice_qtl) as total_broken,
                SUM(bran_qtl) as total_bran,
                SUM(husk_qtl) as total_husk,
                SUM(wastage_qtl) as total_wastage,
                AVG(yield_pct) as avg_yield
            FROM milling_batches WHERE 1=1
            """
            params = []
            if from_date:
                query += " AND batch_date >= ?"
                params.append(from_date)
            if to_date:
                query += " AND batch_date <= ?"
                params.append(to_date)
            cursor.execute(query, params)
            r = cursor.fetchone()
            conn.close()
            if r:
                t_batches = int(r["total_batches"] or 0)
                t_paddy = float(r["total_paddy"] or 0.0)
                t_head = float(r["total_head_rice"] or 0.0)
                t_broken = float(r["total_broken"] or 0.0)
                t_bran = float(r["total_bran"] or 0.0)
                t_husk = float(r["total_husk"] or 0.0)
                t_wastage = float(r["total_wastage"] or 0.0)
                avg_y = float(r["avg_yield"] or 0.0)

                return {
                    "total_batches": t_batches,
                    "total_paddy": t_paddy,
                    "total_head_rice": t_head,
                    "total_broken": t_broken,
                    "total_bran": t_bran,
                    "total_husk": t_husk,
                    "total_wastage": t_wastage,
                    "avg_yield": avg_y,
                    "total_paddy_fmt": format_indian_number(t_paddy, decimals=2),
                    "total_head_rice_fmt": format_indian_number(t_head, decimals=2),
                    "total_broken_fmt": format_indian_number(t_broken, decimals=2),
                    "total_bran_fmt": format_indian_number(t_bran, decimals=2),
                    "total_husk_fmt": format_indian_number(t_husk, decimals=2),
                    "total_wastage_fmt": format_indian_number(t_wastage, decimals=2),
                    "avg_yield_fmt": f"{avg_y:.2f}%"
                }
        except Exception as e:
            print(f"Error fetching milling totals: {e}")
        return {
            "total_batches": 0, "total_paddy": 0.0, "total_head_rice": 0.0,
            "total_broken": 0.0, "total_bran": 0.0, "total_husk": 0.0,
            "total_wastage": 0.0, "avg_yield": 0.0,
            "total_paddy_fmt": "0.00", "total_head_rice_fmt": "0.00",
            "total_broken_fmt": "0.00", "total_bran_fmt": "0.00",
            "total_husk_fmt": "0.00", "total_wastage_fmt": "0.00", "avg_yield_fmt": "0.00%"
        }

    @Slot(str, str, float, float, float, float, float, result=bool)
    def add_batch(self, paddy_variety, batch_date, paddy_input, head_rice, broken_rice, bran, husk):
        try:
            conn = get_connection()
            cursor = conn.cursor()
            
            batch_no = self.get_next_batch_no()
            
            yield_res = AccountingEngine.calculate_milling_yield(paddy_input, head_rice, broken_rice, bran, husk)
            
            if not batch_date:
                batch_date = date.today().strftime("%Y-%m-%d")

            cursor.execute("""
            INSERT INTO milling_batches 
            (batch_no, batch_date, paddy_variety, paddy_input_qtl, head_rice_qtl, broken_rice_qtl, bran_qtl, husk_qtl, wastage_qtl, yield_pct)
            VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
            """, (batch_no, batch_date, paddy_variety, paddy_input, head_rice, broken_rice, bran, husk, yield_res["wastage_qtl"], yield_res["yield_pct"]))
            
            conn.commit()
            conn.close()
            self.reload_data()
            return True
        except Exception as e:
            print(f"Error adding milling batch: {e}")
            return False


class SalesModel(BaseTableModel):
    dataChangedSignal = Signal()

    def __init__(self, parent=None):
        headers = ["Invoice No", "Date", "Customer", "Item", "Bags", "Weight (Qtl)", "Rate (₹)", "Taxable (₹)", "GST %", "Total (₹)", "Mode"]
        role_keys = ["invoice_no", "invoice_date", "customer_name", "item_name", "bag_count", "weight_qtl", "rate_per_qtl", "taxable_amount", "gst_pct", "total_amount", "payment_mode"]
        super().__init__(headers, role_keys, parent)
        self.reload_data()

    @Slot()
    def reload_data(self):
        self.beginResetModel()
        conn = get_connection()
        cursor = conn.cursor()
        cursor.execute("SELECT * FROM sales_invoices ORDER BY id DESC")
        rows = cursor.fetchall()
        self._data = [dict(r) for r in rows]
        conn.close()
        self.endResetModel()
        self.dataChangedSignal.emit()

    @Slot(str, str, str, int, float, float, float, str, result=bool)
    def add_invoice(self, customer_name, item_name, invoice_date, bags, weight_qtl, rate_qtl, gst_pct, pay_mode):
        try:
            conn = get_connection()
            cursor = conn.cursor()
            
            cursor.execute("SELECT MAX(id) FROM sales_invoices")
            max_id = cursor.fetchone()[0] or 5000
            invoice_no = f"INV-{max_id + 1}"
            
            taxable = round(weight_qtl * rate_qtl, 2)
            tax_res = AccountingEngine.calculate_sales_tax(taxable, gst_pct)
            
            if not invoice_date:
                invoice_date = date.today().strftime("%Y-%m-%d")

            cursor.execute("""
            INSERT INTO sales_invoices 
            (invoice_no, invoice_date, customer_id, customer_name, item_name, bag_count, weight_qtl, rate_per_qtl, taxable_amount, gst_pct, gst_amount, total_amount, payment_mode)
            VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
            """, (invoice_no, invoice_date, 1, customer_name, item_name, bags, weight_qtl, rate_qtl, taxable, gst_pct, tax_res["gst_amount"], tax_res["total_amount"], pay_mode))
            
            conn.commit()
            conn.close()
            self.reload_data()
            return True
        except Exception as e:
            print(f"Error adding invoice: {e}")
            return False

    @Slot(result=str)
    @Slot(str, result=str)
    def get_next_voucher_no(self, fy=""):
        try:
            conn = get_connection()
            cursor = conn.cursor()
            if not fy:
                cursor.execute("SELECT year_name FROM financial_years WHERE is_active = 1 LIMIT 1")
                row = cursor.fetchone()
                fy = row[0] if row else "FY 2025-26"
            cursor.execute("SELECT voucher_no FROM vouchers WHERE voucher_no LIKE 'Sale-%' AND financial_year = ?", (fy,))
            nums = []
            for r in cursor.fetchall():
                parts = (r[0] or "").split("-")
                if len(parts) >= 2 and parts[-1].isdigit():
                    nums.append(int(parts[-1]))
            max_id = max(nums) if nums else 0
            conn.close()
            return f"Sale-{max_id + 1}"
        except Exception as e:
            return "Sale-1"

    @Slot(result=str)
    @Slot(str, result=str)
    def get_next_invoice_no(self, fy=""):
        try:
            conn = get_connection()
            cursor = conn.cursor()
            if not fy:
                cursor.execute("SELECT year_name FROM financial_years WHERE is_active = 1 LIMIT 1")
                row = cursor.fetchone()
                fy = row[0] if row else "FY 2026-27"
            
            cursor.execute("SELECT invoice_no FROM sales_invoices WHERE financial_year = ? AND invoice_no IS NOT NULL AND invoice_no != '' ORDER BY id DESC LIMIT 1", (fy,))
            row = cursor.fetchone()
            conn.close()
            
            m_fy = re.search(r'(\d{2})(\d{2})-(\d{2})', fy)
            default_prefix = f"MRI/{m_fy.group(2)}{m_fy.group(3)}-" if m_fy else "MRI/"
            
            if row and row[0]:
                return increment_invoice_str(row[0], default_prefix)
            
            return f"{default_prefix}1"
        except Exception as e:
            return "MRI/2627-1"

    @Slot(str, result=str)
    def increment_invoice(self, inv_str):
        return increment_invoice_str(inv_str)

    @Slot(str, str, str, str, str, str, int, float, float, float, float, float, float, float, float, float, str, str, str, str, result=bool)
    def add_sales_invoice_full(self, invoice_no, invoice_date, party_ledger, gstin, item_name, hsn_code, bag_count, weight_qtl, rate_per_qtl, taxable_amount, gst_pct, cgst_amount, sgst_amount, igst_amount, round_off, total_amount, payment_mode, vehicle_no, eway_bill_no, narration, sale_status="Self Sale", market_fee_status="Paid", dami=0.0, labour=0.0, auction=0.0, m_fee=0.0, hrdf=0.0, other_exp=0.0, welfare=0.0, dhrmd=0.0, sutli=0.0, less_amount=0.0, gr_no="", driver="", bill_time="", sauda_date="", shipping_address="", po_no="", grade="", kanda_weight="", transport="", broker_name="", voucher_no=""):
        try:
            conn = get_connection()
            cursor = conn.cursor()
            
            if not invoice_date:
                invoice_date = date.today().strftime("%Y-%m-%d")

            fy_id, fy_label = get_fy_and_id_for_date(invoice_date, cursor)

            if not voucher_no:
                voucher_no = self.get_next_voucher_no(fy_label)

            if not invoice_no:
                invoice_no = self.get_next_invoice_no(fy_label)

            gst_amount = cgst_amount + sgst_amount + igst_amount

            cursor.execute("""
            INSERT INTO sales_invoices 
            (fy_id, financial_year, voucher_no, invoice_no, invoice_date, customer_id, customer_name, gstin, item_name, hsn_code, bag_count, weight_qtl, rate_per_qtl, taxable_amount, gst_pct, cgst_amount, sgst_amount, igst_amount, round_off, gst_amount, total_amount, payment_mode, vehicle_no, eway_bill_no, narration, sale_status, market_fee_status, dami, labour, auction, m_fee, hrdf, other_exp, welfare, dhrmd, sutli, less_amount, gr_no, driver, bill_time, sauda_date, shipping_address, po_no, grade, kanda_weight, transport, broker_name)
            VALUES (?, ?, ?, ?, ?, 1, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
            """, (fy_id, fy_label, voucher_no, invoice_no, invoice_date, party_ledger, gstin, item_name, hsn_code, bag_count, weight_qtl, rate_per_qtl, taxable_amount, gst_pct, cgst_amount, sgst_amount, igst_amount, round_off, gst_amount, total_amount, payment_mode, vehicle_no, eway_bill_no, narration, sale_status, market_fee_status, dami, labour, auction, m_fee, hrdf, other_exp, welfare, dhrmd, sutli, less_amount, gr_no, driver, bill_time, sauda_date, shipping_address, po_no, grade, kanda_weight, transport, broker_name))
            
            # Post Double-Entry Ledger Voucher into vouchers table
            vch_narration = f"Sales Invoice {invoice_no} - {item_name} ({weight_qtl} Qtl @ ₹{rate_per_qtl})"
            if narration: vch_narration += " | " + narration

            cursor.execute("""
            INSERT INTO vouchers (fy_id, financial_year, voucher_no, instrument_no, voucher_date, voucher_type, legacy_type, party_name, account_type, amount, narration)
            VALUES (?, ?, ?, ?, ?, 'Sales', 'Sale', ?, 'Sales Account', ?, ?)
            """, (fy_id, fy_label, voucher_no, invoice_no, invoice_date, party_ledger, total_amount, vch_narration))

            conn.commit()
            conn.close()
            self.reload_data()
            return True
        except Exception as e:
            print(f"Error adding sales invoice full: {e}")
            return False

    @Slot(result=list)
    @Slot(str, result=list)
    @Slot(str, str, result=list)
    def get_sales_register(self, param1="", param2=""):
        try:
            conn = get_connection()
            cursor = conn.cursor()
            from_date = ""
            to_date = ""
            fy_label = ""
            if param1 and param2:
                from_date = param1
                to_date = param2
            elif param1 and param1 != "All":
                fy_label = param1
            else:
                cursor.execute("SELECT year_name, start_date, end_date FROM financial_years WHERE is_active = 1 LIMIT 1")
                row_fy = cursor.fetchone()
                if row_fy:
                    fy_label = row_fy[0]
                    from_date = row_fy[1]
                    to_date = row_fy[2]

            if from_date and to_date:
                cursor.execute("""
                SELECT 
                    id, voucher_no, invoice_no, invoice_date, customer_name, item_name,
                    bag_count, weight_qtl, rate_per_qtl, taxable_amount, gst_pct,
                    cgst_amount, sgst_amount, igst_amount, gst_amount, round_off,
                    total_amount, payment_mode, vehicle_no, eway_bill_no,
                    financial_year, narration
                FROM sales_invoices
                WHERE invoice_date >= ? AND invoice_date <= ?
                ORDER BY invoice_date DESC, id DESC
                """, (from_date, to_date))
            elif fy_label:
                cursor.execute("""
                SELECT 
                    id, voucher_no, invoice_no, invoice_date, customer_name, item_name,
                    bag_count, weight_qtl, rate_per_qtl, taxable_amount, gst_pct,
                    cgst_amount, sgst_amount, igst_amount, gst_amount, round_off,
                    total_amount, payment_mode, vehicle_no, eway_bill_no,
                    financial_year, narration
                FROM sales_invoices
                WHERE financial_year = ?
                ORDER BY invoice_date DESC, id DESC
                """, (fy_label,))
            else:
                cursor.execute("""
                SELECT 
                    id, voucher_no, invoice_no, invoice_date, customer_name, item_name,
                    bag_count, weight_qtl, rate_per_qtl, taxable_amount, gst_pct,
                    cgst_amount, sgst_amount, igst_amount, gst_amount, round_off,
                    total_amount, payment_mode, vehicle_no, eway_bill_no,
                    financial_year, narration
                FROM sales_invoices
                ORDER BY invoice_date DESC, id DESC
                """)
            
            rows = cursor.fetchall()
            conn.close()
            result = []
            for r in rows:
                vch_no = r[1] or ""
                inv_no = r[2] or ""
                bags = r[6] or 0
                wt = r[7] or 0.0
                rate = r[8] or 0.0
                taxable = r[9] or 0.0
                gst_pct = r[10] or 0.0
                cgst = r[11] or 0.0
                sgst = r[12] or 0.0
                igst = r[13] or 0.0
                gst_tot = r[14] or (cgst + sgst + igst)
                round_off = r[15] or 0.0
                total = r[16] or 0.0
                p_name = str(r[4] or "").replace("\u00a0", " ") if r[4] else ""

                result.append({
                    "id": r[0],
                    "voucher_no": vch_no,
                    "invoice_no": inv_no,
                    "invoice_date": r[3] or "",
                    "customer_name": p_name,
                    "item_name": r[5] or "",
                    "bag_count": bags,
                    "weight_qtl": wt,
                    "rate_per_qtl": rate,
                    "taxable_amount": taxable,
                    "gst_pct": gst_pct,
                    "cgst_amount": cgst,
                    "sgst_amount": sgst,
                    "igst_amount": igst,
                    "gst_amount": gst_tot,
                    "round_off": round_off,
                    "total_amount": total,
                    "payment_mode": r[17] or "Credit",
                    "vehicle_no": r[18] or "",
                    "eway_bill_no": r[19] or "",
                    "financial_year": r[20] or "",
                    "narration": r[21] or "",
                    "bag_count_fmt": format_indian_number(bags, decimals=0),
                    "weight_qtl_fmt": format_indian_number(wt, decimals=2, unit="Qtl"),
                    "rate_fmt": format_indian_currency(rate),
                    "taxable_amount_fmt": format_indian_currency(taxable),
                    "gst_amount_fmt": format_indian_currency(gst_tot),
                    "total_amount_fmt": format_indian_currency(total)
                })
            return result
        except Exception as e:
            print(f"Error fetching sales register: {e}")
            return []


class PurchaseModel(BaseTableModel):
    dataChangedSignal = Signal()

    def __init__(self, parent=None):
        headers = ["Voucher No", "Bill No", "Date", "Supplier", "Item", "Bags", "Weight (Qtl)", "Rate (₹)", "Taxable (₹)", "GST %", "Total (₹)", "Mode"]
        role_keys = ["voucher_no", "invoice_no", "invoice_date", "supplier_name", "item_name", "bag_count", "weight_qtl", "rate_per_qtl", "taxable_amount", "gst_pct", "total_amount", "payment_mode"]
        super().__init__(headers, role_keys, parent)
        self.reload_data()

    @Slot()
    def reload_data(self):
        self.beginResetModel()
        conn = get_connection()
        cursor = conn.cursor()
        cursor.execute("SELECT * FROM purchase_invoices ORDER BY id DESC")
        rows = cursor.fetchall()
        self._data = [dict(r) for r in rows]
        conn.close()
        self.endResetModel()
        self.dataChangedSignal.emit()

    @Slot(result=str)
    @Slot(str, result=str)
    def get_next_voucher_no(self, fy=""):
        try:
            conn = get_connection()
            cursor = conn.cursor()
            if not fy:
                cursor.execute("SELECT year_name FROM financial_years WHERE is_active = 1 LIMIT 1")
                row = cursor.fetchone()
                fy = row[0] if row else "FY 2025-26"
            cursor.execute("SELECT voucher_no FROM vouchers WHERE voucher_no LIKE 'Purc-%' AND financial_year = ?", (fy,))
            nums = []
            for r in cursor.fetchall():
                parts = (r[0] or "").split("-")
                if len(parts) >= 2 and parts[-1].isdigit():
                    nums.append(int(parts[-1]))
            max_id = max(nums) if nums else 0
            conn.close()
            return f"Purc-{max_id + 1}"
        except Exception as e:
            return "Purc-1"

    @Slot(result=str)
    @Slot(str, result=str)
    def get_next_invoice_no(self, fy=""):
        try:
            conn = get_connection()
            cursor = conn.cursor()
            if not fy:
                cursor.execute("SELECT year_name FROM financial_years WHERE is_active = 1 LIMIT 1")
                row = cursor.fetchone()
                fy = row[0] if row else "FY 2026-27"
            
            cursor.execute("SELECT invoice_no FROM purchase_invoices WHERE financial_year = ? AND invoice_no IS NOT NULL AND invoice_no != '' ORDER BY id DESC LIMIT 1", (fy,))
            row = cursor.fetchone()
            conn.close()
            
            if row and row[0]:
                return increment_invoice_str(row[0], "PUR/")
            
            return "PUR/1"
        except Exception as e:
            return "PUR/1"

    @Slot(str, result=str)
    def increment_invoice(self, inv_str):
        return increment_invoice_str(inv_str, "PUR/")

    @Slot(str, str, str, str, str, str, int, float, float, float, float, float, float, float, float, float, str, str, str, str, result=bool)
    def add_purchase_invoice_full(self, invoice_no, invoice_date, party_ledger, gstin, item_name, hsn_code, bag_count, weight_qtl, rate_per_qtl, taxable_amount, gst_pct, cgst_amount, sgst_amount, igst_amount, round_off, total_amount, payment_mode, vehicle_no, eway_bill_no, narration, sale_status="Self Sale", market_fee_status="Paid", dami=0.0, labour=0.0, auction=0.0, m_fee=0.0, hrdf=0.0, other_exp=0.0, welfare=0.0, dhrmd=0.0, sutli=0.0, less_amount=0.0, gr_no="", driver="", bill_time="", sauda_date="", shipping_address="", po_no="", grade="", kanda_weight="", transport="", broker_name="", voucher_no=""):
        try:
            conn = get_connection()
            cursor = conn.cursor()
            
            if not invoice_date:
                invoice_date = date.today().strftime("%Y-%m-%d")

            fy_id, fy_label = get_fy_and_id_for_date(invoice_date, cursor)

            if not voucher_no:
                voucher_no = self.get_next_voucher_no(fy_label)

            if not invoice_no:
                invoice_no = self.get_next_invoice_no(fy_label)

            gst_amount = cgst_amount + sgst_amount + igst_amount

            cursor.execute("""
            INSERT INTO purchase_invoices 
            (fy_id, financial_year, voucher_no, invoice_no, invoice_date, supplier_id, supplier_name, gstin, item_name, hsn_code, bag_count, weight_qtl, rate_per_qtl, taxable_amount, gst_pct, cgst_amount, sgst_amount, igst_amount, round_off, gst_amount, total_amount, payment_mode, vehicle_no, eway_bill_no, narration, sale_status, market_fee_status, dami, labour, auction, m_fee, hrdf, other_exp, welfare, dhrmd, sutli, less_amount, gr_no, driver, bill_time, sauda_date, shipping_address, po_no, grade, kanda_weight, transport, broker_name)
            VALUES (?, ?, ?, ?, ?, 1, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
            """, (fy_id, fy_label, voucher_no, invoice_no, invoice_date, party_ledger, gstin, item_name, hsn_code, bag_count, weight_qtl, rate_per_qtl, taxable_amount, gst_pct, cgst_amount, sgst_amount, igst_amount, round_off, gst_amount, total_amount, payment_mode, vehicle_no, eway_bill_no, narration, sale_status, market_fee_status, dami, labour, auction, m_fee, hrdf, other_exp, welfare, dhrmd, sutli, less_amount, gr_no, driver, bill_time, sauda_date, shipping_address, po_no, grade, kanda_weight, transport, broker_name))
            
            # Post Double-Entry Ledger Voucher into vouchers table
            vch_narration = f"Purchase Bill {invoice_no} - {item_name} ({weight_qtl} Qtl @ ₹{rate_per_qtl})"
            if narration: vch_narration += " | " + narration

            cursor.execute("""
            INSERT INTO vouchers (fy_id, financial_year, voucher_no, instrument_no, voucher_date, voucher_type, legacy_type, party_name, account_type, amount, narration)
            VALUES (?, ?, ?, ?, ?, 'Purchase', 'Purc', ?, 'Purchase Account', ?, ?)
            """, (fy_id, fy_label, voucher_no, invoice_no, invoice_date, party_ledger, total_amount, vch_narration))

            conn.commit()
            conn.close()
            self.reload_data()
            return True
        except Exception as e:
            print(f"Error adding purchase invoice full: {e}")
            return False

    @Slot(result=list)
    @Slot(str, result=list)
    @Slot(str, str, result=list)
    def get_purchase_register(self, param1="", param2=""):
        try:
            conn = get_connection()
            cursor = conn.cursor()
            from_date = ""
            to_date = ""
            fy_label = ""
            if param1 and param2:
                from_date = param1
                to_date = param2
            elif param1 and param1 != "All":
                fy_label = param1
            else:
                cursor.execute("SELECT year_name, start_date, end_date FROM financial_years WHERE is_active = 1 LIMIT 1")
                row_fy = cursor.fetchone()
                if row_fy:
                    fy_label = row_fy[0]
                    from_date = row_fy[1]
                    to_date = row_fy[2]

            if from_date and to_date:
                cursor.execute("""
                SELECT 
                    id, voucher_no, invoice_no, invoice_date, supplier_name, item_name,
                    bag_count, weight_qtl, rate_per_qtl, taxable_amount, gst_pct,
                    cgst_amount, sgst_amount, igst_amount, gst_amount, round_off,
                    total_amount, payment_mode, vehicle_no, eway_bill_no,
                    financial_year, narration
                FROM purchase_invoices
                WHERE invoice_date >= ? AND invoice_date <= ?
                ORDER BY invoice_date DESC, id DESC
                """, (from_date, to_date))
            elif fy_label:
                cursor.execute("""
                SELECT 
                    id, voucher_no, invoice_no, invoice_date, supplier_name, item_name,
                    bag_count, weight_qtl, rate_per_qtl, taxable_amount, gst_pct,
                    cgst_amount, sgst_amount, igst_amount, gst_amount, round_off,
                    total_amount, payment_mode, vehicle_no, eway_bill_no,
                    financial_year, narration
                FROM purchase_invoices
                WHERE financial_year = ?
                ORDER BY invoice_date DESC, id DESC
                """, (fy_label,))
            else:
                cursor.execute("""
                SELECT 
                    id, voucher_no, invoice_no, invoice_date, supplier_name, item_name,
                    bag_count, weight_qtl, rate_per_qtl, taxable_amount, gst_pct,
                    cgst_amount, sgst_amount, igst_amount, gst_amount, round_off,
                    total_amount, payment_mode, vehicle_no, eway_bill_no,
                    financial_year, narration
                FROM purchase_invoices
                ORDER BY invoice_date DESC, id DESC
                """)
            
            rows = cursor.fetchall()
            conn.close()
            result = []
            for r in rows:
                vch_no = r[1] or ""
                inv_no = r[2] or ""
                bags = r[6] or 0
                wt = r[7] or 0.0
                rate = r[8] or 0.0
                taxable = r[9] or 0.0
                gst_pct = r[10] or 0.0
                cgst = r[11] or 0.0
                sgst = r[12] or 0.0
                igst = r[13] or 0.0
                gst_tot = r[14] or (cgst + sgst + igst)
                round_off = r[15] or 0.0
                total = r[16] or 0.0
                p_name = str(r[4] or "").replace("\u00a0", " ") if r[4] else ""

                result.append({
                    "id": r[0],
                    "voucher_no": vch_no,
                    "invoice_no": inv_no,
                    "invoice_date": r[3] or "",
                    "supplier_name": p_name,
                    "item_name": r[5] or "",
                    "bag_count": bags,
                    "weight_qtl": wt,
                    "rate_per_qtl": rate,
                    "taxable_amount": taxable,
                    "gst_pct": gst_pct,
                    "cgst_amount": cgst,
                    "sgst_amount": sgst,
                    "igst_amount": igst,
                    "gst_amount": gst_tot,
                    "round_off": round_off,
                    "total_amount": total,
                    "payment_mode": r[17] or "Credit",
                    "vehicle_no": r[18] or "",
                    "eway_bill_no": r[19] or "",
                    "financial_year": r[20] or "",
                    "narration": r[21] or "",
                    "bag_count_fmt": format_indian_number(bags, decimals=0),
                    "weight_qtl_fmt": format_indian_number(wt, decimals=2, unit="Qtl"),
                    "rate_fmt": format_indian_currency(rate),
                    "taxable_amount_fmt": format_indian_currency(taxable),
                    "gst_amount_fmt": format_indian_currency(gst_tot),
                    "total_amount_fmt": format_indian_currency(total)
                })
            return result
        except Exception as e:
            print(f"Error fetching purchase register: {e}")
            return []


class VouchersModel(BaseTableModel):
    dataChangedSignal = Signal()

    def __init__(self, parent=None):
        headers = ["Voucher No", "Date", "Type", "Party", "Account", "Amount (₹)", "Narration"]
        role_keys = ["voucher_no", "voucher_date", "voucher_type", "party_name", "account_type", "amount", "narration"]
        super().__init__(headers, role_keys, parent)
        self.reload_data()

    @Slot()
    def reload_data(self):
        self.beginResetModel()
        conn = get_connection()
        cursor = conn.cursor()
        cursor.execute("SELECT * FROM vouchers ORDER BY id DESC")
        rows = cursor.fetchall()
        self._data = [dict(r) for r in rows]
        conn.close()
        self.endResetModel()
        self.dataChangedSignal.emit()

    @Slot(result=str)
    @Slot(str, result=str)
    @Slot(str, str, result=str)
    def get_next_voucher_no(self, v_type="ChPt", fy=""):
        try:
            conn = get_connection()
            cursor = conn.cursor()
            prefix = v_type or "ChPt"
            if not fy:
                cursor.execute("SELECT year_name FROM financial_years WHERE is_active = 1 LIMIT 1")
                row = cursor.fetchone()
                fy = row[0] if row else "FY 2025-26"
            cursor.execute("SELECT voucher_no FROM vouchers WHERE voucher_no LIKE ? AND financial_year = ?", (f"{prefix}-%", fy))
            nums = []
            for r in cursor.fetchall():
                parts = (r[0] or "").split("-")
                if len(parts) >= 2 and parts[-1].isdigit():
                    nums.append(int(parts[-1]))
            max_id = max(nums) if nums else 0
            conn.close()
            return f"{prefix}-{max_id + 1}"
        except Exception as e:
            return f"{v_type}-1"

    @Slot(str, str, str, str, float, str, result=bool)
    def add_voucher(self, vch_type, party_name, vch_date, account_type, amount, narration):
        try:
            conn = get_connection()
            cursor = conn.cursor()
            
            if not vch_date:
                vch_date = date.today().strftime("%Y-%m-%d")

            fy_id, fy_label = get_fy_and_id_for_date(vch_date, cursor)
            voucher_no = self.get_next_voucher_no(vch_type, fy_label)

            cursor.execute("""
            INSERT INTO vouchers 
            (fy_id, financial_year, voucher_no, voucher_date, voucher_type, legacy_type, party_id, party_name, account_type, amount, narration)
            VALUES (?, ?, ?, ?, ?, ?, 1, ?, ?, ?, ?)
            """, (fy_id, fy_label, voucher_no, vch_date, vch_type, vch_type, party_name, account_type, amount, narration))
            
            conn.commit()
            conn.close()
            self.reload_data()
            return True
        except Exception as e:
            print(f"Error adding voucher: {e}")
            return False

    @Slot(str, str, str, float, str, str, str, result=bool)
    def add_cheque_voucher(self, vch_type, dr_party, cr_party, amount, chq_no, narration, vch_date=""):
        try:
            conn = get_connection()
            cursor = conn.cursor()
            
            if not vch_date:
                vch_date = date.today().strftime("%Y-%m-%d")

            fy_id, fy_label = get_fy_and_id_for_date(vch_date, cursor)
            voucher_no = self.get_next_voucher_no(vch_type, fy_label)

            full_narration = f"Ch. No. {chq_no}" if chq_no else ""
            if narration:
                full_narration += " | " + narration if full_narration else narration

            cursor.execute("""
            INSERT INTO vouchers 
            (fy_id, financial_year, voucher_no, instrument_no, voucher_date, voucher_type, legacy_type, party_id, party_name, account_type, amount, narration)
            VALUES (?, ?, ?, ?, ?, ?, ?, 1, ?, ?, ?, ?)
            """, (fy_id, fy_label, voucher_no, chq_no, vch_date, vch_type, vch_type, dr_party, cr_party, amount, full_narration))
            
            conn.commit()
            conn.close()
            self.reload_data()
            return True
        except Exception as e:
            print(f"Error adding cheque voucher: {e}")
            return False

    @Slot(str, str, float, str, str, str, str, result=bool)
    def add_journal_voucher(self, dr_party, cr_party, amount, ref_no="", narration="", vch_date="", vch_type="Journal"):
        try:
            conn = get_connection()
            cursor = conn.cursor()
            
            if not vch_date:
                vch_date = date.today().strftime("%Y-%m-%d")

            fy_id, fy_label = get_fy_and_id_for_date(vch_date, cursor)
            voucher_no = self.get_next_voucher_no("Jrnl", fy_label)

            full_narration = f"Ref: {ref_no}" if ref_no else ""
            if narration:
                full_narration += " | " + narration if full_narration else narration

            cursor.execute("""
            INSERT INTO vouchers 
            (fy_id, financial_year, voucher_no, instrument_no, voucher_date, voucher_type, legacy_type, party_id, party_name, account_type, amount, narration)
            VALUES (?, ?, ?, ?, ?, ?, 'Jrnl', 1, ?, ?, ?, ?)
            """, (fy_id, fy_label, voucher_no, ref_no, vch_date, vch_type, dr_party, cr_party, amount, full_narration))
            
            conn.commit()
            conn.close()
            self.reload_data()
            return True
        except Exception as e:
            print(f"Error adding journal voucher: {e}")
            return False


class PartiesModel(BaseTableModel):
    def __init__(self, parent=None):
        headers = ["Party Name", "Group", "Type", "Specialty", "Phone", "City", "GSTIN", "Balance (₹)", "Dr/Cr"]
        role_keys = ["name", "group_name", "party_type", "special_type", "phone", "city", "gstin", "opening_balance", "balance_type"]
        super().__init__(headers, role_keys, parent)
        self.reload_data()

    @Slot(result=list)
    def get_parties_list(self):
        return sorted([r["name"] for r in self._data if "name" in r and r.get("name")], key=lambda x: str(x).lower())

    @Slot(result=list)
    def get_bank_accounts_list(self):
        try:
            conn = get_connection()
            cursor = conn.cursor()
            cursor.execute("""
            SELECT name FROM parties 
            WHERE group_name LIKE '%Bank%' OR party_type = 'Bank' OR name LIKE '%Bank%'
            ORDER BY name COLLATE NOCASE ASC
            """)
            rows = [r[0] for r in cursor.fetchall()]
            conn.close()
            if not rows:
                rows = ["HDFC Bank MG Road", "IndusInd Bank(200999406993)", "SBI Raichur Main Branch"]
            return sorted(rows, key=lambda x: str(x).lower())
        except Exception as e:
            print(f"Error fetching bank accounts list: {e}")
            return ["HDFC Bank MG Road", "IndusInd Bank(200999406993)", "SBI Raichur Main Branch"]

    @Slot(str, result=str)
    def get_ledger_live_balance(self, ledger_name):
        if not ledger_name or not ledger_name.strip():
            return "0.00 Dr"
        try:
            clean_name = ledger_name.strip()
            clean_name_lower = clean_name.lower()
            conn = get_connection()
            cursor = conn.cursor()

            # Get party opening balance
            cursor.execute("SELECT opening_balance, balance_type FROM parties WHERE LOWER(name) = ? OR LOWER(name) LIKE ?", (clean_name_lower, f"%{clean_name_lower}%"))
            p_row = cursor.fetchone()
            net_dr = 0.0
            net_cr = 0.0
            if p_row:
                op = float(p_row["opening_balance"] or 0.0)
                btype = p_row["balance_type"] or "Dr"
                if btype == "Dr":
                    net_dr += op
                else:
                    net_cr += op

            # Sum Sales Invoices (Dr)
            cursor.execute("SELECT SUM(total_amount) FROM sales_invoices WHERE LOWER(customer_name) = ? OR LOWER(customer_name) LIKE ?", (clean_name_lower, f"%{clean_name_lower}%"))
            s_sum = cursor.fetchone()[0] or 0.0
            net_dr += float(s_sum)

            # Sum Paddy Procurement Arrivals (Dr)
            cursor.execute("SELECT SUM(total_amount) FROM paddy_procurement WHERE LOWER(farmer_name) = ? OR LOWER(farmer_name) LIKE ?", (clean_name_lower, f"%{clean_name_lower}%"))
            pa_sum = cursor.fetchone()[0] or 0.0
            net_dr += float(pa_sum)

            # Sum Purchase Invoices (Cr)
            cursor.execute("SELECT SUM(total_amount) FROM purchase_invoices WHERE LOWER(supplier_name) = ? OR LOWER(supplier_name) LIKE ?", (clean_name_lower, f"%{clean_name_lower}%"))
            pur_sum = cursor.fetchone()[0] or 0.0
            net_cr += float(pur_sum)

            # Sum Vouchers (Double Entry Balance Evaluation)
            cursor.execute("SELECT voucher_type, party_name, account_type, amount FROM vouchers")
            for v_row in cursor.fetchall():
                v_type = v_row["voucher_type"]
                if v_type in ["Sales", "Purchase"]:
                    continue

                dr_p = (v_row["party_name"] or "").strip().lower()
                cr_p = (v_row["account_type"] or "").strip().lower()
                v_val = float(v_row["amount"] or 0.0)

                if dr_p and (dr_p == clean_name_lower or clean_name_lower in dr_p):
                    net_dr += v_val
                if cr_p and (cr_p == clean_name_lower or clean_name_lower in cr_p):
                    net_cr += v_val

            conn.close()

            diff = net_dr - net_cr
            if diff >= 0:
                return f"{diff:,.2f} Dr"
            else:
                return f"{abs(diff):,.2f} Cr"
        except Exception as e:
            print(f"Error fetching live balance for {ledger_name}: {e}")
            return "0.00 Dr"

    @Slot()
    def reload_data(self):
        self.beginResetModel()
        conn = get_connection()
        cursor = conn.cursor()
        cursor.execute("SELECT * FROM parties ORDER BY name ASC")
        rows = cursor.fetchall()
        self._data = [dict(r) for r in rows]
        conn.close()
        self.endResetModel()

    @Slot(str, str, str, str, str, float, str, result=bool)
    def add_party(self, name, ptype, phone, place, gstin, op_bal, bal_type):
        try:
            conn = get_connection()
            cursor = conn.cursor()
            cursor.execute("""
            INSERT INTO parties (name, party_type, phone, city, gstin, opening_balance, balance_type)
            VALUES (?, ?, ?, ?, ?, ?, ?)
            """, (name, ptype, phone, place, gstin, op_bal, bal_type))
            conn.commit()
            conn.close()
            self.reload_data()
            return True
        except Exception as e:
            print(f"Error adding party: {e}")
    @Slot(result=list)
    def get_party_list(self):
        return sorted([r["name"] for r in self._data if "name" in r and r.get("name")], key=lambda x: str(x).lower())

    @Slot(str, result=dict)
    def get_party_statement(self, party_name):
        dr_items = []
        cr_items = []

        def parse_dates(d_str):
            if not d_str:
                return "9999-12-31", ""
            s = str(d_str).strip()
            if "-" in s:
                p = s.split("-")
                if len(p) == 3:
                    if len(p[0]) == 4:
                        return f"{p[0]}-{int(p[1]):02d}-{int(p[2]):02d}", f"{int(p[2]):02d}-{int(p[1]):02d}-{p[0]}"
                    elif len(p[2]) == 4:
                        return f"{p[2]}-{int(p[1]):02d}-{int(p[0]):02d}", f"{int(p[0]):02d}-{int(p[1]):02d}-{p[2]}"
            elif "/" in s:
                p = s.split("/")
                if len(p) == 3 and len(p[2]) == 4:
                    return f"{p[2]}-{int(p[1]):02d}-{int(p[0]):02d}", f"{int(p[0]):02d}-{int(p[1]):02d}-{p[2]}"
            return s, s

        try:
            conn = get_connection()
            cursor = conn.cursor()

            clean_name = party_name.strip() if party_name else ""

            if clean_name:
                clean_name_lower = clean_name.lower()

                # 1. Opening balance
                cursor.execute("SELECT opening_balance, balance_type FROM parties WHERE name = ? OR name LIKE ?", (clean_name, f"%{clean_name}%"))
                p_row = cursor.fetchone()
                if p_row and p_row["opening_balance"] and float(p_row["opening_balance"]) > 0:
                    op_val = float(p_row["opening_balance"])
                    b_type = p_row["balance_type"] or "Dr"
                    op_item = {
                        "isSelected": False,
                        "vIso": "2024-04-01",
                        "vDate": "01-04-2024",
                        "refNo": "OP-BAL",
                        "particulars": f"Opening Balance ({b_type})",
                        "amount": op_val
                    }
                    if b_type == "Dr":
                        dr_items.append(op_item)
                    else:
                        cr_items.append(op_item)

                # 2. Sales Invoices (Dr: Customer Account Debited)
                cursor.execute("""
                SELECT invoice_no, invoice_date, item_name, weight_qtl, total_amount, narration 
                FROM sales_invoices 
                WHERE customer_name = ? OR customer_name LIKE ?
                ORDER BY invoice_date ASC, id ASC
                """, (clean_name, f"%{clean_name}%"))
                for r in cursor.fetchall():
                    iso_d, fmt_d = parse_dates(r["invoice_date"])
                    parts_str = f"Sales Invoice: {r['item_name']} ({r['weight_qtl']} Qtl)"
                    if r["narration"]:
                        parts_str += f" | {r['narration']}"

                    dr_items.append({
                        "isSelected": False,
                        "vIso": iso_d,
                        "vDate": fmt_d,
                        "refNo": r["invoice_no"],
                        "particulars": parts_str,
                        "amount": float(r["total_amount"] or 0.0)
                    })

                # 3. Purchase Invoices (Cr: Supplier Account Credited at Gross, with TDS on Dr)
                cursor.execute("""
                SELECT invoice_no, invoice_date, item_name, weight_qtl, taxable_amount, total_amount, narration 
                FROM purchase_invoices 
                WHERE supplier_name = ? OR supplier_name LIKE ?
                ORDER BY invoice_date ASC, id ASC
                """, (clean_name, f"%{clean_name}%"))
                for r in cursor.fetchall():
                    iso_d, fmt_d = parse_dates(r["invoice_date"])
                    taxable_val = float(r["taxable_amount"] or 0.0)
                    total_val = float(r["total_amount"] or 0.0)
                    gross_amt = taxable_val if taxable_val > 0 else total_val
                    tds_amt = round(gross_amt - total_val, 2) if gross_amt > total_val else 0.0

                    parts_str = f"B.No. {r['invoice_no']} | {r['item_name']} ({r['weight_qtl']} Qtl)"
                    if r["narration"]:
                        parts_str += f" | {r['narration']}"

                    cr_items.append({
                        "isSelected": False,
                        "vIso": iso_d,
                        "vDate": fmt_d,
                        "refNo": r["invoice_no"],
                        "particulars": parts_str,
                        "amount": gross_amt
                    })

                    if tds_amt > 0:
                        dr_items.append({
                            "isSelected": False,
                            "vIso": iso_d,
                            "vDate": fmt_d,
                            "refNo": r["invoice_no"],
                            "particulars": f"T.D.S. U/S 194Q (B.No. {r['invoice_no']})",
                            "amount": tds_amt
                        })

                # 4. Paddy Procurement / Arrivals (Cr: Farmer Account Credited)
                cursor.execute("""
                SELECT receipt_no, arrival_date, variety, net_weight_qtl, total_amount 
                FROM paddy_procurement 
                WHERE farmer_name = ? OR farmer_name LIKE ?
                ORDER BY arrival_date ASC, id ASC
                """, (clean_name, f"%{clean_name}%"))
                for pa in cursor.fetchall():
                    iso_d, fmt_d = parse_dates(pa["arrival_date"])
                    cr_items.append({
                        "isSelected": False,
                        "vIso": iso_d,
                        "vDate": fmt_d,
                        "refNo": pa["receipt_no"],
                        "particulars": f"Paddy Arrival: {pa['variety']} ({pa['net_weight_qtl']} Qtl)",
                        "amount": float(pa["total_amount"] or 0.0)
                    })

                # 5. Vouchers (Double Entry Ledger: Payments -> Dr, Receipts -> Cr)
                cursor.execute("""
                SELECT voucher_no, voucher_date, voucher_type, legacy_type, party_name, account_type, amount, narration 
                FROM vouchers 
                WHERE (party_name = ? OR party_name LIKE ? OR account_type = ? OR account_type LIKE ?)
                  AND voucher_type NOT IN ('Sales', 'Purchase')
                ORDER BY voucher_date ASC, id ASC
                """, (clean_name, f"%{clean_name}%", clean_name, f"%{clean_name}%"))
                
                for vr in cursor.fetchall():
                    v_type = vr["voucher_type"]
                    leg_type = vr["legacy_type"] or ""
                    dr_p = (vr["party_name"] or "").strip()
                    cr_p = (vr["account_type"] or "").strip()
                    v_amt = float(vr["amount"] or 0.0)
                    iso_d, fmt_d = parse_dates(vr["voucher_date"])
                    narration_note = vr["narration"] or ""

                    is_party_match = dr_p and (dr_p.lower() == clean_name_lower or clean_name_lower in dr_p.lower())
                    is_acc_match = cr_p and (cr_p.lower() == clean_name_lower or clean_name_lower in cr_p.lower())

                    if is_party_match:
                        if v_type in ["Payment", "Debit Note"] or leg_type in ["ChPt", "CashPt", "CP", "BP"]:
                            # Payment to party -> Party is DEBITED (Dr)
                            part_desc = f"{v_type} (Paid via {cr_p})"
                            if narration_note: part_desc += f" | {narration_note}"
                            dr_items.append({
                                "isSelected": False,
                                "vIso": iso_d,
                                "vDate": fmt_d,
                                "refNo": vr["voucher_no"],
                                "particulars": part_desc,
                                "amount": v_amt
                            })
                        elif v_type in ["Receipt", "Credit Note"] or leg_type in ["ChRt", "CashRt", "CR", "BR"]:
                            # Receipt from party -> Party is CREDITED (Cr)
                            part_desc = f"{v_type} (Received in {cr_p})"
                            if narration_note: part_desc += f" | {narration_note}"
                            cr_items.append({
                                "isSelected": False,
                                "vIso": iso_d,
                                "vDate": fmt_d,
                                "refNo": vr["voucher_no"],
                                "particulars": part_desc,
                                "amount": v_amt
                            })
                        else:
                            # Journal / Other vouchers where party is party_name
                            part_desc = f"{v_type} ({cr_p})"
                            if narration_note: part_desc += f" | {narration_note}"
                            dr_items.append({
                                "isSelected": False,
                                "vIso": iso_d,
                                "vDate": fmt_d,
                                "refNo": vr["voucher_no"],
                                "particulars": part_desc,
                                "amount": v_amt
                            })

                    if is_acc_match and not is_party_match:
                        # Opposing account match (e.g. Bank or Cash account)
                        if v_type in ["Receipt"] or leg_type in ["ChRt", "CashRt", "CR", "BR"]:
                            # Receipt into this Bank/Cash -> Bank/Cash is DEBITED (Dr)
                            part_desc = f"Receipt from {dr_p}"
                            if narration_note: part_desc += f" | {narration_note}"
                            dr_items.append({
                                "isSelected": False,
                                "vIso": iso_d,
                                "vDate": fmt_d,
                                "refNo": vr["voucher_no"],
                                "particulars": part_desc,
                                "amount": v_amt
                            })
                        elif v_type in ["Payment"] or leg_type in ["ChPt", "CashPt", "CP", "BP"]:
                            # Payment out of this Bank/Cash -> Bank/Cash is CREDITED (Cr)
                            part_desc = f"Payment to {dr_p}"
                            if narration_note: part_desc += f" | {narration_note}"
                            cr_items.append({
                                "isSelected": False,
                                "vIso": iso_d,
                                "vDate": fmt_d,
                                "refNo": vr["voucher_no"],
                                "particulars": part_desc,
                                "amount": v_amt
                            })
            else:
                # Load all transactions across all parties
                cursor.execute("SELECT invoice_no, invoice_date, customer_name, item_name, weight_qtl, total_amount, narration FROM sales_invoices ORDER BY invoice_date ASC, id ASC")
                for r in cursor.fetchall():
                    iso_d, fmt_d = parse_dates(r["invoice_date"])
                    parts_str = f"[{r['customer_name']}] Sales Invoice: {r['item_name']} ({r['weight_qtl']} Qtl)"
                    if r["narration"]: parts_str += f" | {r['narration']}"
                    dr_items.append({ "isSelected": False, "vIso": iso_d, "vDate": fmt_d, "refNo": r["invoice_no"], "particulars": parts_str, "amount": float(r["total_amount"] or 0.0) })

                cursor.execute("SELECT invoice_no, invoice_date, supplier_name, item_name, weight_qtl, taxable_amount, total_amount, narration FROM purchase_invoices ORDER BY invoice_date ASC, id ASC")
                for r in cursor.fetchall():
                    iso_d, fmt_d = parse_dates(r["invoice_date"])
                    taxable_val = float(r["taxable_amount"] or 0.0)
                    total_val = float(r["total_amount"] or 0.0)
                    gross_amt = taxable_val if taxable_val > 0 else total_val
                    tds_amt = round(gross_amt - total_val, 2) if gross_amt > total_val else 0.0

                    parts_str = f"[{r['supplier_name']}] Purchase Bill: {r['item_name']} ({r['weight_qtl']} Qtl)"
                    if r["narration"]: parts_str += f" | {r['narration']}"
                    cr_items.append({ "isSelected": False, "vIso": iso_d, "vDate": fmt_d, "refNo": r["invoice_no"], "particulars": parts_str, "amount": gross_amt })
                    if tds_amt > 0:
                        dr_items.append({ "isSelected": False, "vIso": iso_d, "vDate": fmt_d, "refNo": r["invoice_no"], "particulars": f"[{r['supplier_name']}] T.D.S. U/S 194Q ({r['invoice_no']})", "amount": tds_amt })

                cursor.execute("SELECT voucher_no, voucher_date, voucher_type, legacy_type, party_name, account_type, amount, narration FROM vouchers WHERE voucher_type NOT IN ('Sales', 'Purchase') ORDER BY voucher_date ASC, id ASC")
                for vr in cursor.fetchall():
                    v_type = vr["voucher_type"]
                    leg_type = vr["legacy_type"] or ""
                    iso_d, fmt_d = parse_dates(vr["voucher_date"])
                    dr_p = vr["party_name"] or ""
                    cr_p = vr["account_type"] or ""
                    narration_note = vr["narration"] or ""
                    v_amt = float(vr["amount"] or 0.0)
                    
                    if v_type in ["Payment", "Debit Note"] or leg_type in ["ChPt", "CashPt", "CP", "BP"]:
                        dr_items.append({ "isSelected": False, "vIso": iso_d, "vDate": fmt_d, "refNo": vr["voucher_no"], "particulars": f"[{dr_p}] {v_type} (Paid via {cr_p}) - {narration_note}", "amount": v_amt })
                        cr_items.append({ "isSelected": False, "vIso": iso_d, "vDate": fmt_d, "refNo": vr["voucher_no"], "particulars": f"[{cr_p}] {v_type} (Dr: {dr_p}) - {narration_note}", "amount": v_amt })
                    elif v_type in ["Receipt", "Credit Note"] or leg_type in ["ChRt", "CashRt", "CR", "BR"]:
                        dr_items.append({ "isSelected": False, "vIso": iso_d, "vDate": fmt_d, "refNo": vr["voucher_no"], "particulars": f"[{cr_p}] Receipt from {dr_p} - {narration_note}", "amount": v_amt })
                        cr_items.append({ "isSelected": False, "vIso": iso_d, "vDate": fmt_d, "refNo": vr["voucher_no"], "particulars": f"[{dr_p}] {v_type} (Received in {cr_p}) - {narration_note}", "amount": v_amt })
                    else:
                        dr_items.append({ "isSelected": False, "vIso": iso_d, "vDate": fmt_d, "refNo": vr["voucher_no"], "particulars": f"[{dr_p}] {v_type} (Cr: {cr_p}) - {narration_note}", "amount": v_amt })
                        cr_items.append({ "isSelected": False, "vIso": iso_d, "vDate": fmt_d, "refNo": vr["voucher_no"], "particulars": f"[{cr_p}] {v_type} (Dr: {dr_p}) - {narration_note}", "amount": v_amt })

            conn.close()

            dr_items.sort(key=lambda x: x.get("vIso", ""))
            cr_items.sort(key=lambda x: x.get("vIso", ""))

            return {"dr_items": dr_items, "cr_items": cr_items}
        except Exception as e:
            print(f"Error fetching party statement for {party_name}: {e}")
            return {"dr_items": [], "cr_items": []}

    @Slot(result=list)
    def get_account_groups(self):
        defaults = [
            "Bank Accounts",
            "Cash-in-hand",
            "Direct Expenses (Hamali/Freight)",
            "Duties & Taxes (GST)",
            "Loans & Liabilities",
            "Paddy Procurement Purchases",
            "Rice Milling Sales Revenue",
            "Sundry Creditors (Farmers/Vendors)",
            "Sundry Debtors (Buyers)"
        ]
        try:
            conn = get_connection()
            cursor = conn.cursor()
            cursor.execute("SELECT name FROM account_groups ORDER BY name COLLATE NOCASE ASC")
            rows = [r[0] for r in cursor.fetchall()]
            conn.close()
            for d in defaults:
                if d not in rows:
                    rows.append(d)
            return sorted(rows, key=lambda x: str(x).lower())
        except Exception as e:
            return sorted(defaults, key=lambda x: str(x).lower())

    @Slot(result=list)
    def get_cities(self):
        defaults = ["Ballari", "Bengaluru", "Hospet", "Hyderabad", "Kalaburagi", "Koppal", "Raichur", "Vijayanagara"]
        try:
            conn = get_connection()
            cursor = conn.cursor()
            cursor.execute("SELECT DISTINCT city FROM parties WHERE city IS NOT NULL AND city != ''")
            rows = [r[0] for r in cursor.fetchall()]
            conn.close()
            for d in defaults:
                if d not in rows:
                    rows.append(d)
            return sorted(rows, key=lambda x: str(x).lower())
        except Exception as e:
            return sorted(defaults, key=lambda x: str(x).lower())

    @Slot(result=list)
    def get_districts(self):
        defaults = ["Ballari", "Bengaluru Urban", "Hyderabad", "Koppal", "Raichur", "Vijayanagara"]
        try:
            conn = get_connection()
            cursor = conn.cursor()
            cursor.execute("SELECT DISTINCT district FROM parties WHERE district IS NOT NULL AND district != ''")
            rows = [r[0] for r in cursor.fetchall()]
            conn.close()
            for d in defaults:
                if d not in rows:
                    rows.append(d)
            return sorted(rows, key=lambda x: str(x).lower())
        except Exception as e:
            return sorted(defaults, key=lambda x: str(x).lower())

    @Slot(result=list)
    def get_stations(self):
        defaults = ["Andhra Pradesh", "Bengaluru Ganj", "Hyderabad Market", "Karnataka", "Koppal Mandi", "Raichur APMC Yard", "Telangana"]
        try:
            conn = get_connection()
            cursor = conn.cursor()
            cursor.execute("SELECT DISTINCT state FROM parties WHERE state IS NOT NULL AND state != ''")
            rows = [r[0] for r in cursor.fetchall()]
            conn.close()
            for d in defaults:
                if d not in rows:
                    rows.append(d)
            return sorted(rows, key=lambda x: str(x).lower())
        except Exception as e:
            return sorted(defaults, key=lambda x: str(x).lower())

    @Slot(str, result=dict)
    def get_party_by_name(self, party_name):
        if not party_name or not party_name.strip():
            return {}
        try:
            clean = party_name.strip()
            conn = get_connection()
            cursor = conn.cursor()
            cursor.execute("SELECT * FROM parties WHERE LOWER(name) = LOWER(?) OR name LIKE ?", (clean, f"%{clean}%"))
            row = cursor.fetchone()
            conn.close()
            if row:
                return dict(row)
        except Exception as e:
            print(f"Error fetching party by name: {e}")
        return {}

    @Slot(str, str, str, str, str, str, float, str, str, str, str, str, str, str, str, str, str, str, str, str, str, str, float, int, str, str, str, result=bool)
    def add_ledger_full(self, name, alias, prefix, group_name, party_type, special_type, opening_balance, balance_type, mailing_name, address, city, district, state, pincode, phone, mobile, whatsapp, email, contact_person, gstin, pan, aadhaar, credit_limit, credit_days, bank_name="", bank_account="", ifsc_code=""):
        try:
            conn = get_connection()
            cursor = conn.cursor()
            cursor.execute("""
            INSERT INTO parties 
            (name, alias, prefix, group_name, party_type, special_type, opening_balance, balance_type, mailing_name, address, city, district, state, pincode, phone, mobile, whatsapp, email, contact_person, gstin, pan, aadhaar, credit_limit, credit_days, bank_name, bank_account, ifsc_code)
            VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
            """, (name, alias, prefix, group_name, party_type, special_type, opening_balance, balance_type, mailing_name, address, city, district, state, pincode, phone, mobile, whatsapp, email, contact_person, gstin, pan, aadhaar, credit_limit, credit_days, bank_name, bank_account, ifsc_code))
            conn.commit()
            conn.close()
            self.reload_data()
            return True
        except Exception as e:
            print(f"Error adding full ledger: {e}")
            return False

    @Slot(int, str, str, str, str, str, str, float, str, str, str, str, str, str, str, str, str, str, str, str, str, str, str, float, int, str, str, str, result=bool)
    def update_ledger_full(self, party_id, name, alias, prefix, group_name, party_type, special_type, opening_balance, balance_type, mailing_name, address, city, district, state, pincode, phone, mobile, whatsapp, email, contact_person, gstin, pan, aadhaar, credit_limit, credit_days, bank_name="", bank_account="", ifsc_code=""):
        try:
            conn = get_connection()
            cursor = conn.cursor()
            cursor.execute("""
            UPDATE parties 
            SET name = ?, alias = ?, prefix = ?, group_name = ?, party_type = ?, special_type = ?, opening_balance = ?, balance_type = ?, mailing_name = ?, address = ?, city = ?, district = ?, state = ?, pincode = ?, phone = ?, mobile = ?, whatsapp = ?, email = ?, contact_person = ?, gstin = ?, pan = ?, aadhaar = ?, credit_limit = ?, credit_days = ?, bank_name = ?, bank_account = ?, ifsc_code = ?
            WHERE id = ?
            """, (name, alias, prefix, group_name, party_type, special_type, opening_balance, balance_type, mailing_name, address, city, district, state, pincode, phone, mobile, whatsapp, email, contact_person, gstin, pan, aadhaar, credit_limit, credit_days, bank_name, bank_account, ifsc_code, party_id))
            conn.commit()
            conn.close()
            self.reload_data()
            return True
        except Exception as e:
            print(f"Error updating full ledger: {e}")
            return False


def format_indian_number(amount, include_symbol=False, decimals=2, unit=""):
    try:
        val = float(amount or 0.0)
    except Exception:
        val = 0.0
    is_negative = val < 0
    val = abs(val)
    
    s = f"{val:.{decimals}f}" if decimals >= 0 else f"{val:.2f}"
    parts = s.split(".")
    int_part = parts[0]
    dec_part = parts[1] if len(parts) > 1 else ""
    
    if len(int_part) <= 3:
        formatted_int = int_part
    else:
        last3 = int_part[-3:]
        rest = int_part[:-3]
        groups = []
        while len(rest) > 2:
            groups.insert(0, rest[-2:])
            rest = rest[:-2]
        if rest:
            groups.insert(0, rest)
        formatted_int = ",".join(groups) + "," + last3
        
    result = f"{formatted_int}.{dec_part}" if decimals > 0 else formatted_int
    if is_negative:
        result = f"-{result}"
    if include_symbol:
        result = f"₹{result}"
    if unit:
        result = f"{result} {unit}"
    return result


def format_indian_currency(amount, include_symbol=True, decimals=2):
    return format_indian_number(amount, include_symbol=include_symbol, decimals=decimals)


class DashboardController(QObject):
    statsChanged = Signal()

    def __init__(self, parent=None):
        super().__init__(parent)
        self._paddy_stock = "0.0 Qtl"
        self._rice_stock = "0.0 Qtl"
        self._total_sales = "₹0.00"
        self._total_procurement = "₹0.00"
        self._milling_efficiency = "0.0%"
        self.refresh_stats()

    @Slot()
    @Slot(str)
    @Slot(str, str)
    @Slot(str, str, str)
    def refresh_stats(self, from_date="", to_date="", fy_label=""):
        try:
            conn = get_connection()
            cursor = conn.cursor()
            
            # If no explicit FY given, fetch active FY
            if not fy_label and not from_date:
                cursor.execute("SELECT year_name, start_date, end_date FROM financial_years WHERE is_active = 1 LIMIT 1")
                row = cursor.fetchone()
                if row:
                    fy_label = row[0]
                    from_date = row[1]
                    to_date = row[2]

            # 1. Paddy Stock (Filtered by active FY Closing Stock)
            if to_date:
                cursor.execute("SELECT SUM(weight_qtl) FROM custom_closing_stocks WHERE (item_name LIKE '%Paddy%' OR item_code = '43') AND closing_date = ?", (to_date,))
                row_p = cursor.fetchone()
                paddy_val = (row_p[0] if row_p and row_p[0] is not None else 0.0)
            else:
                cursor.execute("SELECT SUM(current_stock_qtl) FROM inventory WHERE category = 'Raw Paddy'")
                paddy_val = cursor.fetchone()[0] or 0.0
            self._paddy_stock = format_indian_number(paddy_val, decimals=1, unit="Qtl")

            # 2. Rice Stock (Filtered by active FY Closing Stock)
            if to_date:
                cursor.execute("SELECT SUM(weight_qtl) FROM custom_closing_stocks WHERE item_code = '30' AND closing_date = ?", (to_date,))
                row_r = cursor.fetchone()
                rice_val = (row_r[0] if row_r and row_r[0] is not None else 0.0)
            else:
                cursor.execute("SELECT SUM(current_stock_qtl) FROM inventory WHERE category = 'Finished Rice'")
                rice_val = cursor.fetchone()[0] or 0.0
            self._rice_stock = format_indian_number(rice_val, decimals=1, unit="Qtl")

            # 3. Total Sales Turnover (Taxable Turnover matching Bahi-Khata)
            if from_date and to_date:
                cursor.execute("SELECT SUM(COALESCE(taxable_amount, total_amount)) FROM sales_invoices WHERE invoice_date >= ? AND invoice_date <= ?", (from_date, to_date))
            elif fy_label:
                cursor.execute("SELECT SUM(COALESCE(taxable_amount, total_amount)) FROM sales_invoices WHERE financial_year = ?", (fy_label,))
            else:
                cursor.execute("SELECT SUM(COALESCE(taxable_amount, total_amount)) FROM sales_invoices")
            sales_val = cursor.fetchone()[0] or 0.0
            self._total_sales = format_indian_currency(sales_val)

            # 4. Total Procurement (Filtered by FY / Date Range)
            if from_date and to_date:
                cursor.execute("SELECT SUM(COALESCE(total_amount, taxable_amount)) FROM purchase_invoices WHERE invoice_date >= ? AND invoice_date <= ?", (from_date, to_date))
            elif fy_label:
                cursor.execute("SELECT SUM(COALESCE(total_amount, taxable_amount)) FROM purchase_invoices WHERE financial_year = ?", (fy_label,))
            else:
                cursor.execute("SELECT SUM(COALESCE(total_amount, taxable_amount)) FROM purchase_invoices")
            proc_val = cursor.fetchone()[0] or 0.0
            self._total_procurement = format_indian_currency(proc_val)

            # 5. Avg Milling Efficiency
            if from_date and to_date:
                cursor.execute("SELECT AVG(yield_pct) FROM milling_batches WHERE batch_date >= ? AND batch_date <= ?", (from_date, to_date))
            else:
                cursor.execute("SELECT AVG(yield_pct) FROM milling_batches")
            eff_val = cursor.fetchone()[0] or 65.3
            self._milling_efficiency = f"{eff_val:.1f}%"

            conn.close()
            self.statsChanged.emit()
        except Exception as e:
            print(f"Error refreshing dashboard stats: {e}")

    @Slot(float, result=str)
    @Slot(str, result=str)
    def format_inr(self, amount):
        return format_indian_currency(amount)

    @Slot(float, result=str)
    @Slot(str, result=str)
    def format_qty(self, qty):
        return format_indian_number(qty, decimals=2)

    @Property(str, notify=statsChanged)
    def paddyStock(self):
        return self._paddy_stock

    @Property(str, notify=statsChanged)
    def riceStock(self):
        return self._rice_stock

    @Property(str, notify=statsChanged)
    def totalSales(self):
        return self._total_sales

    @Property(str, notify=statsChanged)
    def totalProcurement(self):
        return self._total_procurement

    @Property(str, notify=statsChanged)
    def millingEfficiency(self):
        return self._milling_efficiency


class AccountGroupsModel(BaseTableModel):
    def __init__(self, parent=None):
        headers = ["Group Name", "Parent Group", "Nature", "Description", "Balance Sheet", "Type"]
        role_keys = ["name", "parent_group_name", "nature", "description", "extract_in_balance_sheet", "is_system"]
        super().__init__(headers, role_keys, parent)
        self.reload_data()

    @Slot()
    def reload_data(self):
        self.beginResetModel()
        try:
            conn = get_connection()
            cursor = conn.cursor()
            cursor.execute("SELECT * FROM account_groups ORDER BY name ASC")
            rows = cursor.fetchall()
            self._data = [dict(r) for r in rows]
            conn.close()
        except Exception as e:
            print(f"Error reloading account groups: {e}")
            self._data = []
        self.endResetModel()

    @Slot(result=list)
    def get_groups_list(self):
        return self._data

    @Slot(result=list)
    def get_parent_groups(self):
        defaults = [
            "Branch / Divisions",
            "Capital Account",
            "Current Assets",
            "Current Liabilities",
            "Direct Expenses",
            "Duties & Taxes",
            "Loans (Liability)",
            "Primary / Root Group",
            "Purchase Accounts",
            "Sales Accounts"
        ]
        try:
            conn = get_connection()
            cursor = conn.cursor()
            cursor.execute("SELECT name FROM account_groups ORDER BY name COLLATE NOCASE ASC")
            rows = [r[0] for r in cursor.fetchall()]
            conn.close()
            for d in defaults:
                if d not in rows:
                    rows.append(d)
            return sorted(rows, key=lambda x: str(x).lower())
        except Exception as e:
            return sorted(defaults, key=lambda x: str(x).lower())

    @Slot(str, result=dict)
    def get_group_by_name(self, group_name):
        try:
            conn = get_connection()
            cursor = conn.cursor()
            cursor.execute("SELECT * FROM account_groups WHERE name = ?", (group_name,))
            row = cursor.fetchone()
            conn.close()
            if row:
                return dict(row)
        except Exception as e:
            print(f"Error fetching group by name: {e}")
        return {}

    @Slot(str, str, str, str, bool, result=bool)
    def add_group(self, name, parent_group_name, nature, description, extract_in_balance_sheet):
        try:
            conn = get_connection()
            cursor = conn.cursor()
            cursor.execute("""
            INSERT INTO account_groups (name, parent_group_name, nature, description, extract_in_balance_sheet, is_system)
            VALUES (?, ?, ?, ?, ?, 0)
            """, (name, parent_group_name, nature, description, 1 if extract_in_balance_sheet else 0))
            conn.commit()
            conn.close()
            self.reload_data()
            return True
        except Exception as e:
            print(f"Error adding group: {e}")
            return False

    @Slot(int, str, str, str, str, bool, result=bool)
    def update_group(self, group_id, name, parent_group_name, nature, description, extract_in_balance_sheet):
        try:
            conn = get_connection()
            cursor = conn.cursor()
            cursor.execute("""
            UPDATE account_groups 
            SET name = ?, parent_group_name = ?, nature = ?, description = ?, extract_in_balance_sheet = ?
            WHERE id = ?
            """, (name, parent_group_name, nature, description, 1 if extract_in_balance_sheet else 0, group_id))
            conn.commit()
            conn.close()
            self.reload_data()
            return True
        except Exception as e:
            print(f"Error updating group: {e}")
            return False

    @Slot(int, result=bool)
    def delete_group(self, group_id):
        try:
            conn = get_connection()
            cursor = conn.cursor()
            cursor.execute("DELETE FROM account_groups WHERE id = ? AND is_system = 0", (group_id,))
            conn.commit()
            conn.close()
            self.reload_data()
            return True
        except Exception as e:
            print(f"Error deleting group: {e}")
            return False


class StockItemsModel(BaseTableModel):
    def __init__(self, parent=None):
        headers = ["Item Name", "Code", "Type", "Unit", "Packing (kg)", "Purchase Rate", "Sale Rate", "GST Rate %", "HSN"]
        role_keys = ["name", "code", "item_type", "unit", "packing_kg", "purchase_rate", "sale_rate", "gst_rate", "hsn_code"]
        super().__init__(headers, role_keys, parent)
        self._current_financial_year = ""
        self._current_from_date = ""
        self._current_to_date = ""
        self._init_active_period()
        self.reload_data()

    def _init_active_period(self):
        try:
            conn = get_connection()
            cursor = conn.cursor()
            cursor.execute("SELECT year_name, start_date, end_date FROM financial_years WHERE is_active = 1 LIMIT 1")
            row = cursor.fetchone()
            if not row:
                cursor.execute("SELECT year_name, start_date, end_date FROM financial_years ORDER BY start_date DESC LIMIT 1")
                row = cursor.fetchone()
            if row:
                self._current_financial_year = row[0]
                self._current_from_date = row[1]
                self._current_to_date = row[2]
            conn.close()
        except Exception as e:
            print(f"Notice initializing active period: {e}")

    @Slot()
    def reload_data(self):
        self.beginResetModel()
        try:
            conn = get_connection()
            cursor = conn.cursor()
            cursor.execute("SELECT * FROM stock_items ORDER BY name COLLATE NOCASE ASC")
            rows = cursor.fetchall()
            self._data = [dict(r) for r in rows]
            conn.close()
        except Exception as e:
            print(f"Error reloading stock items: {e}")
            self._data = []
        self.endResetModel()

    @Slot(result=list)
    def get_items_list(self):
        return sorted([r["name"] for r in self._data if "name" in r and r.get("name")], key=lambda x: str(x).lower())

    @Slot(str, result=dict)
    def get_item_by_name(self, item_name):
        try:
            conn = get_connection()
            cursor = conn.cursor()
            cursor.execute("SELECT * FROM stock_items WHERE name = ?", (item_name,))
            row = cursor.fetchone()
            conn.close()
            if row:
                return dict(row)
        except Exception as e:
            print(f"Error fetching stock item by name: {e}")
        return {}

    @Slot(result=list)
    def get_item_types(self):
        return sorted(["Finished Rice", "Raw Paddy", "By-Product", "Packing Material", "General Goods"], key=lambda x: str(x).lower())

    @Slot(result=list)
    def get_units(self):
        return sorted(["Bags", "Kg", "Ltr", "MT", "Nos", "Pcs", "Qtl"], key=lambda x: str(x).lower())

    @Slot(result=list)
    def get_gst_rates(self):
        return ["0%", "5%", "12%", "18%", "28%"]

    @Slot(str, str, str, str, str, float, float, float, float, str, float, float, float, int, float, float, float, str, str, str, result=bool)
    def add_stock_item(self, name, code, item_type, company_name, unit, purchase_rate, sale_rate, mrp, discount, hsn_code, gst_rate, cess_rate, packing_kg, opening_bags, opening_qty, opening_rate, opening_value, purchase_ledger, sale_ledger, stock_ledger):
        try:
            conn = get_connection()
            cursor = conn.cursor()
            cursor.execute("""
            INSERT INTO stock_items 
            (name, code, item_type, goods_type, company_name, category_name, unit, purchase_rate, sale_rate, mrp, discount, hsn_code, gst_rate, cess_rate, packing_kg, opening_bags, opening_qty, opening_rate, opening_value, purchase_ledger, sale_ledger, stock_ledger, is_milling_item, include_in_trading, calculate_stock)
            VALUES (?, ?, ?, 'Goods', ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, 1, 1, 1)
            """, (name, code, item_type, company_name, item_type, unit, purchase_rate, sale_rate, mrp, discount, hsn_code, gst_rate, cess_rate, packing_kg, opening_bags, opening_qty, opening_rate, opening_value, purchase_ledger, sale_ledger, stock_ledger))
            conn.commit()
            conn.close()
            self.reload_data()
            return True
        except Exception as e:
            print(f"Error adding stock item: {e}")
            return False

    @Slot(int, str, str, str, str, str, float, float, float, float, str, float, float, float, int, float, float, float, str, str, str, result=bool)
    def update_stock_item(self, item_id, name, code, item_type, company_name, unit, purchase_rate, sale_rate, mrp, discount, hsn_code, gst_rate, cess_rate, packing_kg, opening_bags, opening_qty, opening_rate, opening_value, purchase_ledger, sale_ledger, stock_ledger):
        try:
            conn = get_connection()
            cursor = conn.cursor()
            cursor.execute("""
            UPDATE stock_items 
            SET name = ?, code = ?, item_type = ?, company_name = ?, category_name = ?, unit = ?, purchase_rate = ?, sale_rate = ?, mrp = ?, discount = ?, hsn_code = ?, gst_rate = ?, cess_rate = ?, packing_kg = ?, opening_bags = ?, opening_qty = ?, opening_rate = ?, opening_value = ?, purchase_ledger = ?, sale_ledger = ?, stock_ledger = ?
            WHERE id = ?
            """, (name, code, item_type, company_name, item_type, unit, purchase_rate, sale_rate, mrp, discount, hsn_code, gst_rate, cess_rate, packing_kg, opening_bags, opening_qty, opening_rate, opening_value, purchase_ledger, sale_ledger, stock_ledger, item_id))
            conn.commit()
            conn.close()
            self.reload_data()
            return True
        except Exception as e:
            print(f"Error updating stock item: {e}")
            return False

    @Slot(int, result=bool)
    def delete_stock_item(self, item_id):
        try:
            conn = get_connection()
            cursor = conn.cursor()
            cursor.execute("DELETE FROM stock_items WHERE id = ?", (item_id,))
            conn.commit()
            conn.close()
            self.reload_data()
            return True
        except Exception as e:
            print(f"Error deleting stock item: {e}")
            return False

    @Slot(result=str)
    def get_financial_year(self):
        return self._current_financial_year

    @Slot(result=str)
    def get_from_date(self):
        return self._current_from_date

    @Slot(result=str)
    def get_to_date(self):
        return self._current_to_date

    @Slot(str)
    def set_financial_year(self, fy):
        self._current_financial_year = fy
        try:
            conn = get_connection()
            cursor = conn.cursor()
            cursor.execute("SELECT start_date, end_date FROM financial_years WHERE year_name = ?", (fy,))
            row = cursor.fetchone()
            if row:
                self._current_from_date = row[0]
                self._current_to_date = row[1]
            conn.close()
        except Exception:
            pass
        self.reload_data()

    @Slot(str, str)
    @Slot(str, str, str)
    def set_accounting_period(self, from_date, to_date, fy_label=""):
        self._current_from_date = from_date
        self._current_to_date = to_date
        self._current_financial_year = fy_label or f"{from_date} To {to_date}"
        self.reload_data()

    @Slot(result=list)
    def get_available_financial_years(self):
        try:
            conn = get_connection()
            cursor = conn.cursor()
            cursor.execute("SELECT year_name, start_date, end_date, is_active FROM financial_years ORDER BY year_name ASC")
            fys = [dict(r) for r in cursor.fetchall()]
            conn.close()
            result = []
            for f in fys:
                y_name = f["year_name"]
                sd = f.get("start_date", "") or ""
                ed = f.get("end_date", "") or ""
                s_fmt = "-".join(reversed(sd.split("-"))) if "-" in sd else sd
                e_fmt = "-".join(reversed(ed.split("-"))) if "-" in ed else ed
                result.append({
                    "name": y_name,
                    "label": f"{y_name} (Active)" if f.get("is_active") else y_name,
                    "startDate": sd,
                    "endDate": ed,
                    "startFormatted": s_fmt,
                    "endFormatted": e_fmt,
                    "isActive": bool(f.get("is_active"))
                })
            result.append({
                "name": "All",
                "label": "All Financial Years",
                "startDate": "",
                "endDate": "",
                "startFormatted": "Beginning",
                "endFormatted": "Latest",
                "isActive": False
            })
            return result
        except Exception as e:
            print(f"Error fetching available financial years: {e}")
            return []

    @Slot(result=list)
    @Slot(str, result=list)
    @Slot(str, str, result=list)
    def get_stock_register(self, param1="", param2=""):
        try:
            conn = get_connection()
            cursor = conn.cursor()
            cursor.execute("SELECT * FROM stock_items ORDER BY name COLLATE NOCASE ASC")
            items = [dict(r) for r in cursor.fetchall()]
            
            from_date = ""
            to_date = ""

            if param1 and param2:
                from_date = param1
                to_date = param2
            elif param1 and param1 != "All":
                cursor.execute("SELECT start_date, end_date FROM financial_years WHERE year_name = ?", (param1,))
                row_fy = cursor.fetchone()
                if row_fy:
                    from_date, to_date = row_fy[0], row_fy[1]
                else:
                    from_date = self._current_from_date
                    to_date = self._current_to_date
            else:
                from_date = self._current_from_date
                to_date = self._current_to_date

            result = []
            for item in items:
                name = item.get("name", "")
                item_id = item.get("id")
                op_qty = item.get("opening_qty", 0.0) or 0.0
                op_bags = item.get("opening_bags", 0) or 0

                # 1. Opening Stock from custom_closing_stocks (or stock_items master)
                if from_date:
                    cursor.execute("""
                    SELECT closing_date, bags, weight_qtl 
                    FROM custom_closing_stocks 
                    WHERE (item_id = ? OR item_name = ? OR item_code = ?) AND closing_date < ?
                    ORDER BY closing_date DESC LIMIT 1
                    """, (item_id, name, item.get("code", ""), from_date))
                    custom_row = cursor.fetchone()
                    
                    if custom_row:
                        c_date = custom_row[0]
                        op_bags = (custom_row[1] or 0)
                        op_qty = (custom_row[2] or 0.0)
                        
                        # Add intermediate stock transactions between audited closing date and from_date
                        cursor.execute("""
                        SELECT 
                            SUM(CASE WHEN trans_type IN ('Purc', 'Inward', 'P', 'M') THEN weight_qtl ELSE 0 END),
                            SUM(CASE WHEN trans_type IN ('Purc', 'Inward', 'P', 'M') THEN bags ELSE 0 END),
                            SUM(CASE WHEN trans_type IN ('Sale', 'Outward', 'S') THEN weight_qtl ELSE 0 END),
                            SUM(CASE WHEN trans_type IN ('Sale', 'Outward', 'S') THEN bags ELSE 0 END)
                        FROM stock_transactions
                        WHERE (item_id = ? OR item_name = ? OR item_code = ?) AND voucher_date > ? AND voucher_date < ?
                        """, (item_id, name, item.get("code", ""), c_date, from_date))
                        r_int = cursor.fetchone()
                        if r_int:
                            op_qty += ((r_int[0] or 0.0) - (r_int[2] or 0.0))
                            op_bags += ((r_int[1] or 0) - (r_int[3] or 0))

                # 2. Inwards from Stock Transactions (Purchases & Inward Movements)
                if from_date and to_date:
                    cursor.execute("""
                    SELECT SUM(weight_qtl), SUM(bags) 
                    FROM stock_transactions 
                    WHERE (item_id = ? OR item_name = ? OR item_code = ?) 
                      AND trans_type IN ('Purc', 'Inward', 'P', 'M') 
                      AND voucher_date >= ? AND voucher_date <= ?
                    """, (item_id, name, item.get("code", ""), from_date, to_date))
                else:
                    cursor.execute("""
                    SELECT SUM(weight_qtl), SUM(bags) 
                    FROM stock_transactions 
                    WHERE (item_id = ? OR item_name = ? OR item_code = ?) 
                      AND trans_type IN ('Purc', 'Inward', 'P', 'M')
                    """, (item_id, name, item.get("code", "")))
                row_st_in = cursor.fetchone()
                inward_pur = (row_st_in[0] if row_st_in else 0.0) or 0.0
                inward_pur_bags = (row_st_in[1] if row_st_in else 0) or 0

                # Inward from Milling Production
                if from_date and to_date:
                    cursor.execute("""
                    SELECT SUM(mvi.weight_qtl), SUM(mvi.bags)
                    FROM milling_voucher_items mvi
                    JOIN milling_batches mb ON mb.batch_no = mvi.batch_no
                    WHERE (mvi.item_id = ? OR mvi.item_name = ? OR mvi.item_code = ?) 
                      AND mvi.drcr = 'Dr' AND mb.batch_date >= ? AND mb.batch_date <= ?
                    """, (item_id, name, item.get("code", ""), from_date, to_date))
                else:
                    cursor.execute("""
                    SELECT SUM(mvi.weight_qtl), SUM(mvi.bags)
                    FROM milling_voucher_items mvi
                    WHERE (mvi.item_id = ? OR mvi.item_name = ? OR mvi.item_code = ?) 
                      AND mvi.drcr = 'Dr'
                    """, (item_id, name, item.get("code", "")))
                row_mill = cursor.fetchone()
                inward_milling = (row_mill[0] if row_mill else 0.0) or 0.0
                inward_milling_bags = (row_mill[1] if row_mill else 0) or 0

                # Inward from Paddy Arrivals (for Raw Paddy items)
                inward_paddy = 0.0
                inward_paddy_bags = 0
                if "Paddy" in name:
                    if from_date and to_date:
                        cursor.execute("SELECT SUM(net_weight_qtl), SUM(bag_count) FROM paddy_arrivals WHERE paddy_variety = ? AND arrival_date >= ? AND arrival_date <= ?", (name, from_date, to_date))
                    else:
                        cursor.execute("SELECT SUM(net_weight_qtl), SUM(bag_count) FROM paddy_arrivals WHERE paddy_variety = ?", (name,))
                    row_pad = cursor.fetchone()
                    inward_paddy = (row_pad[0] if row_pad else 0.0) or 0.0
                    inward_paddy_bags = (row_pad[1] if row_pad else 0) or 0

                # 3. Outwards from Stock Transactions (Sales)
                if from_date and to_date:
                    cursor.execute("""
                    SELECT SUM(weight_qtl), SUM(bags) 
                    FROM stock_transactions 
                    WHERE (item_id = ? OR item_name = ? OR item_code = ?) 
                      AND trans_type IN ('Sale', 'Outward', 'S') 
                      AND voucher_date >= ? AND voucher_date <= ?
                    """, (item_id, name, item.get("code", ""), from_date, to_date))
                else:
                    cursor.execute("""
                    SELECT SUM(weight_qtl), SUM(bags) 
                    FROM stock_transactions 
                    WHERE (item_id = ? OR item_name = ? OR item_code = ?) 
                      AND trans_type IN ('Sale', 'Outward', 'S')
                    """, (item_id, name, item.get("code", "")))
                row_st_out = cursor.fetchone()
                outward_sales = (row_st_out[0] if row_st_out else 0.0) or 0.0
                outward_sales_bags = (row_st_out[1] if row_st_out else 0) or 0

                inward_total = inward_pur + inward_paddy + inward_milling
                inward_bags_total = inward_pur_bags + inward_paddy_bags + inward_milling_bags
                outward_total = outward_sales
                outward_bags_total = outward_sales_bags

                # If audited closing stock exists on to_date, use exact audited closing
                closing_qty = op_qty + inward_total - outward_total
                closing_bags = op_bags + inward_bags_total - outward_bags_total

                rate = item.get("sale_rate", 0.0) or item.get("purchase_rate", 0.0)
                if to_date:
                    cursor.execute("SELECT rate, amount, weight_qtl FROM custom_closing_stocks WHERE (item_id = ? OR item_name = ? OR item_code = ?) AND closing_date = ?", (item_id, name, item.get("code", ""), to_date))
                    audited_cl = cursor.fetchone()
                    if audited_cl:
                        if audited_cl[0] and audited_cl[0] > 0:
                            rate = audited_cl[0]
                        closing_qty = audited_cl[2]
                
                if not rate or rate == 0.0:
                    cursor.execute("SELECT AVG(rate) FROM stock_transactions WHERE (item_id = ? OR item_name = ? OR item_code = ?) AND rate > 0", (item_id, name, item.get("code", "")))
                    avg_r = cursor.fetchone()[0]
                    rate = round(avg_r, 2) if avg_r else 2500.0
                closing_val = closing_qty * rate
                
                result.append({
                    "id": item.get("id"),
                    "name": name,
                    "code": item.get("code", ""),
                    "item_type": item.get("item_type", "General Goods"),
                    "unit": item.get("unit", "Qtl"),
                    "opening_bags": op_bags,
                    "opening_qty": op_qty,
                    "inward_bags": inward_bags_total,
                    "inward_qty": inward_total,
                    "outward_bags": outward_bags_total,
                    "outward_qty": outward_total,
                    "closing_bags": closing_bags,
                    "closing_qty": closing_qty,
                    "rate": rate,
                    "closing_value": closing_val,
                    "opening_qty_fmt": format_indian_number(op_qty, decimals=3),
                    "inward_qty_fmt": format_indian_number(inward_total, decimals=3),
                    "outward_qty_fmt": format_indian_number(outward_total, decimals=3),
                    "closing_qty_fmt": format_indian_number(closing_qty, decimals=3),
                    "opening_bags_fmt": format_indian_number(op_bags, decimals=0),
                    "inward_bags_fmt": format_indian_number(inward_bags_total, decimals=0),
                    "outward_bags_fmt": format_indian_number(outward_bags_total, decimals=0),
                    "closing_bags_fmt": format_indian_number(closing_bags, decimals=0),
                    "rate_fmt": format_indian_currency(rate),
                    "closing_value_fmt": format_indian_currency(closing_val)
                })
            conn.close()
            return result
        except Exception as e:
            print(f"Error computing stock register: {e}")
            return []

    @Slot(str, result=list)
    @Slot(str, str, result=list)
    @Slot(str, str, str, result=list)
    def get_item_movements(self, item_name, param1="", param2=""):
        try:
            conn = get_connection()
            cursor = conn.cursor()
            movements = []
            
            from_date = ""
            to_date = ""

            if param1 and param2:
                from_date = param1
                to_date = param2
            elif param1 and param1 != "All":
                cursor.execute("SELECT start_date, end_date FROM financial_years WHERE year_name = ?", (param1,))
                row_fy = cursor.fetchone()
                if row_fy:
                    from_date, to_date = row_fy[0], row_fy[1]
                else:
                    from_date = self._current_from_date
                    to_date = self._current_to_date
            else:
                from_date = self._current_from_date
                to_date = self._current_to_date

            # Calculate opening balance prior to from_date
            if from_date:
                cursor.execute("""
                SELECT closing_date, bags, weight_qtl 
                FROM custom_closing_stocks 
                WHERE item_name = ? AND closing_date < ?
                ORDER BY closing_date DESC LIMIT 1
                """, (item_name, from_date))
                custom_row = cursor.fetchone()
                
                op_wt = 0.0
                op_bags = 0
                if custom_row:
                    c_date = custom_row[0]
                    op_bags = custom_row[1] or 0
                    op_wt = custom_row[2] or 0.0
                    cursor.execute("SELECT SUM(weight_qtl), SUM(bag_count) FROM purchase_invoices WHERE item_name = ? AND invoice_date > ? AND invoice_date < ?", (item_name, c_date, from_date))
                    r_pur_int = cursor.fetchone()
                    cursor.execute("SELECT SUM(net_weight_qtl), SUM(bag_count) FROM paddy_arrivals WHERE paddy_variety = ? AND arrival_date > ? AND arrival_date < ?", (item_name, c_date, from_date))
                    r_pad_int = cursor.fetchone()
                    cursor.execute("SELECT SUM(weight_qtl), SUM(bag_count) FROM sales_invoices WHERE item_name = ? AND invoice_date > ? AND invoice_date < ?", (item_name, c_date, from_date))
                    r_sal_int = cursor.fetchone()

                    op_wt += ((r_pur_int[0] or 0.0) + (r_pad_int[0] or 0.0) - (r_sal_int[0] or 0.0))
                    op_bags += ((r_pur_int[1] or 0) + (r_pad_int[1] or 0) - (r_sal_int[1] or 0))
                else:
                    cursor.execute("SELECT SUM(weight_qtl), SUM(bag_count) FROM purchase_invoices WHERE item_name = ? AND invoice_date < ?", (item_name, from_date))
                    r_pur_prior = cursor.fetchone()
                    pur_prior_wt = (r_pur_prior[0] if r_pur_prior else 0.0) or 0.0
                    pur_prior_bags = (r_pur_prior[1] if r_pur_prior else 0) or 0

                    cursor.execute("SELECT SUM(net_weight_qtl), SUM(bag_count) FROM paddy_arrivals WHERE paddy_variety = ? AND arrival_date < ?", (item_name, from_date))
                    r_pad_prior = cursor.fetchone()
                    pad_prior_wt = (r_pad_prior[0] if r_pad_prior else 0.0) or 0.0
                    pad_prior_bags = (r_pad_prior[1] if r_pad_prior else 0) or 0

                    cursor.execute("SELECT SUM(weight_qtl), SUM(bag_count) FROM sales_invoices WHERE item_name = ? AND invoice_date < ?", (item_name, from_date))
                    r_sal_prior = cursor.fetchone()
                    sal_prior_wt = (r_sal_prior[0] if r_sal_prior else 0.0) or 0.0
                    sal_prior_bags = (r_sal_prior[1] if r_sal_prior else 0) or 0

                    op_wt = pur_prior_wt + pad_prior_wt - sal_prior_wt
                    op_bags = pur_prior_bags + pad_prior_bags - sal_prior_bags

                if op_wt != 0.0 or op_bags != 0:
                    movements.append({
                        "vDate": from_date,
                        "refNo": "OP-BAL",
                        "type": "Opening Balance (B/F)",
                        "party": "Opening Stock (B/F)",
                        "bags": op_bags,
                        "qty": op_wt,
                        "rate": 0.0,
                        "amount": 0.0,
                        "financial_year": "Opening",
                        "isInward": True if op_wt >= 0 else False
                    })
            
            # 1. Purchase Invoices (Inwards)
            if from_date and to_date:
                cursor.execute("""
                SELECT invoice_no, invoice_date, supplier_name, bag_count, weight_qtl, rate_per_qtl, total_amount, financial_year 
                FROM purchase_invoices 
                WHERE item_name = ? AND invoice_date >= ? AND invoice_date <= ?
                ORDER BY invoice_date DESC
                """, (item_name, from_date, to_date))
            else:
                cursor.execute("""
                SELECT invoice_no, invoice_date, supplier_name, bag_count, weight_qtl, rate_per_qtl, total_amount, financial_year 
                FROM purchase_invoices 
                WHERE item_name = ? 
                ORDER BY invoice_date DESC
                """, (item_name,))
            for r in cursor.fetchall():
                movements.append({
                    "vDate": r[1],
                    "refNo": r[0],
                    "type": "Purchase Invoice (Inward)",
                    "party": r[2],
                    "bags": r[3] or 0,
                    "qty": r[4] or 0.0,
                    "rate": r[5] or 0.0,
                    "amount": r[6] or 0.0,
                    "financial_year": r[7],
                    "isInward": True
                })

            # 2. Paddy Procurement Arrivals (Inwards)
            if from_date and to_date:
                cursor.execute("""
                SELECT slip_no, arrival_date, farmer_name, bag_count, net_weight_qtl, rate_per_qtl, net_amount 
                FROM paddy_arrivals 
                WHERE paddy_variety = ? AND arrival_date >= ? AND arrival_date <= ?
                ORDER BY arrival_date DESC
                """, (item_name, from_date, to_date))
            else:
                cursor.execute("""
                SELECT slip_no, arrival_date, farmer_name, bag_count, net_weight_qtl, rate_per_qtl, net_amount 
                FROM paddy_arrivals 
                WHERE paddy_variety = ? 
                ORDER BY arrival_date DESC
                """, (item_name,))
            for r in cursor.fetchall():
                movements.append({
                    "vDate": r[1],
                    "refNo": r[0],
                    "type": "Paddy Arrival (Inward)",
                    "party": r[2],
                    "bags": r[3] or 0,
                    "qty": r[4] or 0.0,
                    "rate": r[5] or 0.0,
                    "amount": r[6] or 0.0,
                    "financial_year": r[1][:4],
                    "isInward": True
                })

            # 3. Sales Invoices (Outwards)
            if from_date and to_date:
                cursor.execute("""
                SELECT invoice_no, invoice_date, customer_name, bag_count, weight_qtl, rate_per_qtl, total_amount, financial_year 
                FROM sales_invoices 
                WHERE item_name = ? AND invoice_date >= ? AND invoice_date <= ?
                ORDER BY invoice_date DESC
                """, (item_name, from_date, to_date))
            else:
                cursor.execute("""
                SELECT invoice_no, invoice_date, customer_name, bag_count, weight_qtl, rate_per_qtl, total_amount, financial_year 
                FROM sales_invoices 
                WHERE item_name = ? 
                ORDER BY invoice_date DESC
                """, (item_name,))
            for r in cursor.fetchall():
                movements.append({
                    "vDate": r[1],
                    "refNo": r[0],
                    "type": "Sales Invoice (Outward)",
                    "party": r[2],
                    "bags": r[3] or 0,
                    "qty": r[4] or 0.0,
                    "rate": r[5] or 0.0,
                    "amount": r[6] or 0.0,
                    "financial_year": r[7],
                    "isInward": False
                })
                
            conn.close()
            movements.sort(key=lambda x: x["vDate"], reverse=True)
            return movements
        except Exception as e:
            print(f"Error fetching item movements: {e}")
            return []
