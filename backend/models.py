from PySide6.QtCore import QObject, QAbstractTableModel, Qt, QModelIndex, Property, Signal, Slot
from backend.database import get_connection
from backend.accounting_engine import AccountingEngine
from datetime import datetime, date

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
            
            # Update stock in inventory
            item_code = "PAD-SONA" if "Sona" in paddy_variety else "PAD-IR64"
            cursor.execute("UPDATE inventory SET current_stock_qtl = current_stock_qtl + ? WHERE item_code = ?", (net_qtl, item_code))
            
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

    @Slot(str, str, float, float, float, float, float, result=bool)
    def add_batch(self, paddy_variety, batch_date, paddy_input, head_rice, broken_rice, bran, husk):
        try:
            conn = get_connection()
            cursor = conn.cursor()
            
            cursor.execute("SELECT MAX(id) FROM milling_batches")
            max_id = cursor.fetchone()[0] or 0
            batch_no = f"MB-2026-{max_id + 1:02d}"
            
            yield_res = AccountingEngine.calculate_milling_yield(paddy_input, head_rice, broken_rice, bran, husk)
            
            if not batch_date:
                batch_date = date.today().strftime("%Y-%m-%d")

            cursor.execute("""
            INSERT INTO milling_batches 
            (batch_no, batch_date, paddy_variety, paddy_input_qtl, head_rice_qtl, broken_rice_qtl, bran_qtl, husk_qtl, wastage_qtl, yield_pct)
            VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
            """, (batch_no, batch_date, paddy_variety, paddy_input, head_rice, broken_rice, bran, husk, yield_res["wastage_qtl"], yield_res["yield_pct"]))
            
            # Deduct Paddy Stock & Increase Finished Goods Stock
            paddy_code = "PAD-SONA" if "Sona" in paddy_variety else "PAD-IR64"
            cursor.execute("UPDATE inventory SET current_stock_qtl = max(0, current_stock_qtl - ?) WHERE item_code = ?", (paddy_input, paddy_code))
            cursor.execute("UPDATE inventory SET current_stock_qtl = current_stock_qtl + ? WHERE item_code = 'RICE-SONA-1'", (head_rice,))
            cursor.execute("UPDATE inventory SET current_stock_qtl = current_stock_qtl + ? WHERE item_code = 'BY-BROKEN'", (broken_rice,))
            cursor.execute("UPDATE inventory SET current_stock_qtl = current_stock_qtl + ? WHERE item_code = 'BY-BRAN'", (bran,))
            cursor.execute("UPDATE inventory SET current_stock_qtl = current_stock_qtl + ? WHERE item_code = 'BY-HUSK'", (husk,))

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
    def get_next_voucher_no(self):
        try:
            conn = get_connection()
            cursor = conn.cursor()
            cursor.execute("SELECT MAX(id) FROM vouchers")
            max_id = cursor.fetchone()[0] or 9000
            conn.close()
            return f"VCH-{max_id + 1}"
        except Exception as e:
            return "VCH-9001"

    @Slot(str, str, str, str, str, str, int, float, float, float, float, float, float, float, float, float, str, str, str, str, result=bool)
    def add_sales_invoice_full(self, invoice_no, invoice_date, party_ledger, gstin, item_name, hsn_code, bag_count, weight_qtl, rate_per_qtl, taxable_amount, gst_pct, cgst_amount, sgst_amount, igst_amount, round_off, total_amount, payment_mode, vehicle_no, eway_bill_no, narration, sale_status="Self Sale", market_fee_status="Paid", dami=0.0, labour=0.0, auction=0.0, m_fee=0.0, hrdf=0.0, other_exp=0.0, welfare=0.0, dhrmd=0.0, sutli=0.0, less_amount=0.0, gr_no="", driver="", bill_time="", sauda_date="", shipping_address="", po_no="", grade="", kanda_weight="", transport="", broker_name=""):
        try:
            conn = get_connection()
            cursor = conn.cursor()
            
            if not invoice_no:
                cursor.execute("SELECT MAX(id) FROM sales_invoices")
                max_id = cursor.fetchone()[0] or 5000
                invoice_no = f"INV-{max_id + 1}"
            
            if not invoice_date:
                invoice_date = date.today().strftime("%Y-%m-%d")

            gst_amount = cgst_amount + sgst_amount + igst_amount

            cursor.execute("""
            INSERT INTO sales_invoices 
            (invoice_no, invoice_date, customer_id, customer_name, gstin, item_name, hsn_code, bag_count, weight_qtl, rate_per_qtl, taxable_amount, gst_pct, cgst_amount, sgst_amount, igst_amount, round_off, gst_amount, total_amount, payment_mode, vehicle_no, eway_bill_no, narration, sale_status, market_fee_status, dami, labour, auction, m_fee, hrdf, other_exp, welfare, dhrmd, sutli, less_amount, gr_no, driver, bill_time, sauda_date, shipping_address, po_no, grade, kanda_weight, transport, broker_name)
            VALUES (?, ?, 1, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
            """, (invoice_no, invoice_date, party_ledger, gstin, item_name, hsn_code, bag_count, weight_qtl, rate_per_qtl, taxable_amount, gst_pct, cgst_amount, sgst_amount, igst_amount, round_off, gst_amount, total_amount, payment_mode, vehicle_no, eway_bill_no, narration, sale_status, market_fee_status, dami, labour, auction, m_fee, hrdf, other_exp, welfare, dhrmd, sutli, less_amount, gr_no, driver, bill_time, sauda_date, shipping_address, po_no, grade, kanda_weight, transport, broker_name))
            
            # Post Double-Entry Ledger Voucher into vouchers table
            cursor.execute("SELECT MAX(id) FROM vouchers")
            v_max = cursor.fetchone()[0] or 9000
            vch_no = f"VCH-{v_max + 1}"
            vch_narration = f"Sales Invoice {invoice_no} - {item_name} ({weight_qtl} Qtl @ ₹{rate_per_qtl})"
            if narration: vch_narration += " | " + narration

            cursor.execute("""
            INSERT INTO vouchers (voucher_no, voucher_date, voucher_type, party_name, account_type, amount, narration)
            VALUES (?, ?, 'Sales', ?, 'Sales Account', ?, ?)
            """, (vch_no, invoice_date, party_ledger, total_amount, vch_narration))

            conn.commit()
            conn.close()
            self.reload_data()
            return True
        except Exception as e:
            print(f"Error adding sales invoice full: {e}")
            return False


class PurchaseModel(BaseTableModel):
    dataChangedSignal = Signal()

    def __init__(self, parent=None):
        headers = ["Invoice No", "Date", "Supplier", "Item", "Bags", "Weight (Qtl)", "Rate (₹)", "Taxable (₹)", "GST %", "Total (₹)", "Mode"]
        role_keys = ["invoice_no", "invoice_date", "supplier_name", "item_name", "bag_count", "weight_qtl", "rate_per_qtl", "taxable_amount", "gst_pct", "total_amount", "payment_mode"]
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
    def get_next_voucher_no(self):
        try:
            conn = get_connection()
            cursor = conn.cursor()
            cursor.execute("SELECT MAX(id) FROM vouchers")
            max_id = cursor.fetchone()[0] or 9000
            conn.close()
            return f"VCH-{max_id + 1}"
        except Exception as e:
            return "VCH-9001"

    @Slot(str, str, str, str, str, str, int, float, float, float, float, float, float, float, float, float, str, str, str, str, result=bool)
    def add_purchase_invoice_full(self, invoice_no, invoice_date, party_ledger, gstin, item_name, hsn_code, bag_count, weight_qtl, rate_per_qtl, taxable_amount, gst_pct, cgst_amount, sgst_amount, igst_amount, round_off, total_amount, payment_mode, vehicle_no, eway_bill_no, narration, sale_status="Self Sale", market_fee_status="Paid", dami=0.0, labour=0.0, auction=0.0, m_fee=0.0, hrdf=0.0, other_exp=0.0, welfare=0.0, dhrmd=0.0, sutli=0.0, less_amount=0.0, gr_no="", driver="", bill_time="", sauda_date="", shipping_address="", po_no="", grade="", kanda_weight="", transport="", broker_name=""):
        try:
            conn = get_connection()
            cursor = conn.cursor()
            
            if not invoice_no:
                cursor.execute("SELECT MAX(id) FROM purchase_invoices")
                max_id = cursor.fetchone()[0] or 7000
                invoice_no = f"PUR-{max_id + 1}"
            
            if not invoice_date:
                invoice_date = date.today().strftime("%Y-%m-%d")

            gst_amount = cgst_amount + sgst_amount + igst_amount

            cursor.execute("""
            INSERT INTO purchase_invoices 
            (invoice_no, invoice_date, supplier_id, supplier_name, gstin, item_name, hsn_code, bag_count, weight_qtl, rate_per_qtl, taxable_amount, gst_pct, cgst_amount, sgst_amount, igst_amount, round_off, gst_amount, total_amount, payment_mode, vehicle_no, eway_bill_no, narration, sale_status, market_fee_status, dami, labour, auction, m_fee, hrdf, other_exp, welfare, dhrmd, sutli, less_amount, gr_no, driver, bill_time, sauda_date, shipping_address, po_no, grade, kanda_weight, transport, broker_name)
            VALUES (?, ?, 1, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
            """, (invoice_no, invoice_date, party_ledger, gstin, item_name, hsn_code, bag_count, weight_qtl, rate_per_qtl, taxable_amount, gst_pct, cgst_amount, sgst_amount, igst_amount, round_off, gst_amount, total_amount, payment_mode, vehicle_no, eway_bill_no, narration, sale_status, market_fee_status, dami, labour, auction, m_fee, hrdf, other_exp, welfare, dhrmd, sutli, less_amount, gr_no, driver, bill_time, sauda_date, shipping_address, po_no, grade, kanda_weight, transport, broker_name))
            
            # Post Double-Entry Ledger Voucher into vouchers table
            cursor.execute("SELECT MAX(id) FROM vouchers")
            v_max = cursor.fetchone()[0] or 9000
            vch_no = f"VCH-{v_max + 1}"
            vch_narration = f"Purchase Bill {invoice_no} - {item_name} ({weight_qtl} Qtl @ ₹{rate_per_qtl})"
            if narration: vch_narration += " | " + narration

            cursor.execute("""
            INSERT INTO vouchers (voucher_no, voucher_date, voucher_type, party_name, account_type, amount, narration)
            VALUES (?, ?, 'Purchase', ?, 'Purchase Account', ?, ?)
            """, (vch_no, invoice_date, party_ledger, total_amount, vch_narration))

            conn.commit()
            conn.close()
            self.reload_data()
            return True
        except Exception as e:
            print(f"Error adding purchase invoice full: {e}")
            return False


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
    def get_next_voucher_no(self):
        try:
            conn = get_connection()
            cursor = conn.cursor()
            cursor.execute("SELECT MAX(id) FROM vouchers")
            max_id = cursor.fetchone()[0] or 9000
            conn.close()
            return f"VCH-{max_id + 1}"
        except Exception as e:
            return "VCH-9001"

    @Slot(str, str, str, str, float, str, result=bool)
    def add_voucher(self, vch_type, party_name, vch_date, account_type, amount, narration):
        try:
            conn = get_connection()
            cursor = conn.cursor()
            
            cursor.execute("SELECT MAX(id) FROM vouchers")
            max_id = cursor.fetchone()[0] or 9000
            voucher_no = f"VCH-{max_id + 1}"
            
            if not vch_date:
                vch_date = date.today().strftime("%Y-%m-%d")

            cursor.execute("""
            INSERT INTO vouchers 
            (voucher_no, voucher_date, voucher_type, party_id, party_name, account_type, amount, narration)
            VALUES (?, ?, ?, 1, ?, ?, ?, ?)
            """, (voucher_no, vch_date, vch_type, 1, party_name, account_type, amount, narration))
            
            conn.commit()
            conn.close()
            self.reload_data()
            return True
        except Exception as e:
            print(f"Error adding voucher: {e}")
            return False

    @Slot(str, str, str, float, str, str, result=bool)
    def add_cheque_voucher(self, vch_type, dr_party, cr_party, amount, chq_no, narration, vch_date=""):
        try:
            conn = get_connection()
            cursor = conn.cursor()
            
            cursor.execute("SELECT MAX(id) FROM vouchers")
            max_id = cursor.fetchone()[0] or 9000
            voucher_no = f"VCH-{max_id + 1}"
            
            if not vch_date:
                vch_date = date.today().strftime("%Y-%m-%d")

            full_narration = f"Ch. No. {chq_no}" if chq_no else ""
            if narration:
                full_narration += " | " + narration if full_narration else narration

            cursor.execute("""
            INSERT INTO vouchers 
            (voucher_no, voucher_date, voucher_type, party_id, party_name, account_type, amount, narration)
            VALUES (?, ?, ?, 1, ?, ?, ?, ?)
            """, (voucher_no, vch_date, vch_type, dr_party, cr_party, amount, full_narration))
            
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
            
            cursor.execute("SELECT MAX(id) FROM vouchers")
            max_id = cursor.fetchone()[0] or 9000
            voucher_no = f"VCH-{max_id + 1}"
            
            if not vch_date:
                vch_date = date.today().strftime("%Y-%m-%d")

            full_narration = f"Ref: {ref_no}" if ref_no else ""
            if narration:
                full_narration += " | " + narration if full_narration else narration

            cursor.execute("""
            INSERT INTO vouchers 
            (voucher_no, voucher_date, voucher_type, party_id, party_name, account_type, amount, narration)
            VALUES (?, ?, ?, 1, ?, ?, ?, ?)
            """, (voucher_no, vch_date, vch_type, dr_party, cr_party, amount, full_narration))
            
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
        return [r["name"] for r in self._data if "name" in r and r.get("name")]

    @Slot(result=list)
    def get_bank_accounts_list(self):
        try:
            conn = get_connection()
            cursor = conn.cursor()
            cursor.execute("""
            SELECT name FROM parties 
            WHERE group_name LIKE '%Bank%' OR party_type = 'Bank' OR name LIKE '%Bank%'
            ORDER BY name ASC
            """)
            rows = [r[0] for r in cursor.fetchall()]
            conn.close()
            if not rows:
                rows = ["IndusInd Bank(200999406993)", "SBI Raichur Main Branch", "HDFC Bank MG Road"]
            return rows
        except Exception as e:
            print(f"Error fetching bank accounts list: {e}")
            return ["IndusInd Bank(200999406993)", "SBI Raichur Main Branch", "HDFC Bank MG Road"]

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
            return False
    def get_party_list(self):
        return self._data

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
                # Opening balance
                cursor.execute("SELECT opening_balance, balance_type FROM parties WHERE name LIKE ? OR name = ?", (f"%{clean_name}%", clean_name))
                p_row = cursor.fetchone()
                if p_row and p_row["opening_balance"] and float(p_row["opening_balance"]) > 0:
                    op_val = float(p_row["opening_balance"])
                    b_type = p_row["balance_type"] or "Dr"
                    op_item = {
                        "isSelected": False,
                        "vIso": "2026-04-01",
                        "vDate": "01-04-2026",
                        "refNo": "OP-BAL",
                        "particulars": f"Opening Balance ({b_type})",
                        "amount": op_val
                    }
                    if b_type == "Dr":
                        dr_items.append(op_item)
                    else:
                        cr_items.append(op_item)

                # Sales Invoices (Dr)
                cursor.execute("""
                SELECT invoice_no, invoice_date, item_name, weight_qtl, total_amount, narration 
                FROM sales_invoices 
                WHERE customer_name LIKE ? OR customer_name = ?
                ORDER BY id ASC
                """, (f"%{clean_name}%", clean_name))
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
                        "amount": float(r["total_amount"])
                    })

                # Purchase Invoices (Cr)
                cursor.execute("""
                SELECT invoice_no, invoice_date, item_name, weight_qtl, total_amount, narration 
                FROM purchase_invoices 
                WHERE supplier_name LIKE ? OR supplier_name = ?
                ORDER BY id ASC
                """, (f"%{clean_name}%", clean_name))
                for r in cursor.fetchall():
                    iso_d, fmt_d = parse_dates(r["invoice_date"])
                    parts_str = f"Purchase Bill: {r['item_name']} ({r['weight_qtl']} Qtl)"
                    if r["narration"]:
                        parts_str += f" | {r['narration']}"

                    cr_items.append({
                        "isSelected": False,
                        "vIso": iso_d,
                        "vDate": fmt_d,
                        "refNo": r["invoice_no"],
                        "particulars": parts_str,
                        "amount": float(r["total_amount"])
                    })

                # Paddy Arrivals (Dr)
                cursor.execute("""
                SELECT receipt_no, arrival_date, variety, net_weight_qtl, total_amount 
                FROM paddy_procurement 
                WHERE farmer_name LIKE ? OR farmer_name = ?
                ORDER BY id ASC
                """, (f"%{clean_name}%", clean_name))
                for pa in cursor.fetchall():
                    iso_d, fmt_d = parse_dates(pa["arrival_date"])
                    dr_items.append({
                        "isSelected": False,
                        "vIso": iso_d,
                        "vDate": fmt_d,
                        "refNo": pa["receipt_no"],
                        "particulars": f"Paddy Arrival: {pa['variety']} ({pa['net_weight_qtl']} Qtl)",
                        "amount": float(pa["total_amount"])
                    })

                # Vouchers (Double Entry Ledger Statement matching)
                cursor.execute("""
                SELECT voucher_no, voucher_date, voucher_type, party_name, account_type, amount, narration 
                FROM vouchers 
                ORDER BY id ASC
                """)
                clean_name_lower = clean_name.lower()
                for vr in cursor.fetchall():
                    v_type = vr["voucher_type"]
                    if v_type in ["Sales", "Purchase"]:
                        continue

                    dr_p = (vr["party_name"] or "").strip()
                    cr_p = (vr["account_type"] or "").strip()
                    v_amt = float(vr["amount"] or 0.0)
                    iso_d, fmt_d = parse_dates(vr["voucher_date"])
                    narration_note = vr["narration"] or ""

                    is_dr_match = dr_p and (dr_p.lower() == clean_name_lower or clean_name_lower in dr_p.lower())
                    is_cr_match = cr_p and (cr_p.lower() == clean_name_lower or clean_name_lower in cr_p.lower())

                    if is_dr_match:
                        part_desc = f"{v_type} (Cr: {cr_p})"
                        if narration_note:
                            part_desc += f" | {narration_note}"
                        dr_items.append({
                            "isSelected": False,
                            "vIso": iso_d,
                            "vDate": fmt_d,
                            "refNo": vr["voucher_no"],
                            "particulars": part_desc,
                            "amount": v_amt
                        })

                    if is_cr_match:
                        part_desc = f"{v_type} (Dr: {dr_p})"
                        if narration_note:
                            part_desc += f" | {narration_note}"
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
                cursor.execute("SELECT invoice_no, invoice_date, customer_name, item_name, weight_qtl, total_amount, narration FROM sales_invoices ORDER BY id ASC")
                for r in cursor.fetchall():
                    iso_d, fmt_d = parse_dates(r["invoice_date"])
                    parts_str = f"[{r['customer_name']}] Sales Invoice: {r['item_name']} ({r['weight_qtl']} Qtl)"
                    if r["narration"]: parts_str += f" | {r['narration']}"
                    dr_items.append({ "isSelected": False, "vIso": iso_d, "vDate": fmt_d, "refNo": r["invoice_no"], "particulars": parts_str, "amount": float(r["total_amount"]) })

                cursor.execute("SELECT invoice_no, invoice_date, supplier_name, item_name, weight_qtl, total_amount, narration FROM purchase_invoices ORDER BY id ASC")
                for r in cursor.fetchall():
                    iso_d, fmt_d = parse_dates(r["invoice_date"])
                    parts_str = f"[{r['supplier_name']}] Purchase Bill: {r['item_name']} ({r['weight_qtl']} Qtl)"
                    if r["narration"]: parts_str += f" | {r['narration']}"
                    cr_items.append({ "isSelected": False, "vIso": iso_d, "vDate": fmt_d, "refNo": r["invoice_no"], "particulars": parts_str, "amount": float(r["total_amount"]) })

                cursor.execute("SELECT receipt_no, arrival_date, farmer_name, variety, net_weight_qtl, total_amount FROM paddy_procurement ORDER BY id ASC")
                for pa in cursor.fetchall():
                    iso_d, fmt_d = parse_dates(pa["arrival_date"])
                    dr_items.append({ "isSelected": False, "vIso": iso_d, "vDate": fmt_d, "refNo": pa["receipt_no"], "particulars": f"[{pa['farmer_name']}] Paddy Arrival: {pa['variety']} ({pa['net_weight_qtl']} Qtl)", "amount": float(pa["total_amount"]) })

                cursor.execute("SELECT voucher_no, voucher_date, voucher_type, party_name, account_type, amount, narration FROM vouchers ORDER BY id ASC")
                for vr in cursor.fetchall():
                    if vr["voucher_type"] in ["Sales", "Purchase"]: continue
                    iso_d, fmt_d = parse_dates(vr["voucher_date"])
                    dr_p = vr["party_name"] or ""
                    cr_p = vr["account_type"] or ""
                    narration_note = vr["narration"] or ""
                    
                    dr_items.append({ "isSelected": False, "vIso": iso_d, "vDate": fmt_d, "refNo": vr["voucher_no"], "particulars": f"[{dr_p}] {vr['voucher_type']} (Cr: {cr_p}) - {narration_note}", "amount": float(vr["amount"]) })
                    cr_items.append({ "isSelected": False, "vIso": iso_d, "vDate": fmt_d, "refNo": vr["voucher_no"], "particulars": f"[{cr_p}] {vr['voucher_type']} (Dr: {dr_p}) - {narration_note}", "amount": float(vr["amount"]) })

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
            "Sundry Debtors (Buyers)",
            "Sundry Creditors (Farmers/Vendors)",
            "Bank Accounts",
            "Cash-in-hand",
            "Direct Expenses (Hamali/Freight)",
            "Rice Milling Sales Revenue",
            "Paddy Procurement Purchases",
            "Duties & Taxes (GST)",
            "Loans & Liabilities"
        ]
        try:
            conn = get_connection()
            cursor = conn.cursor()
            cursor.execute("SELECT name FROM account_groups ORDER BY name ASC")
            rows = [r[0] for r in cursor.fetchall()]
            conn.close()
            return rows
        except Exception as e:
            return defaults

    @Slot(result=list)
    def get_cities(self):
        defaults = ["Raichur", "Koppal", "Bengaluru", "Hyderabad", "Hospet", "Vijayanagara", "Ballari", "Kalaburagi"]
        try:
            conn = get_connection()
            cursor = conn.cursor()
            cursor.execute("SELECT DISTINCT city FROM parties WHERE city IS NOT NULL AND city != ''")
            rows = [r[0] for r in cursor.fetchall()]
            conn.close()
            for d in defaults:
                if d not in rows:
                    rows.append(d)
            return rows
        except Exception as e:
            return defaults

    @Slot(result=list)
    def get_districts(self):
        defaults = ["Raichur", "Koppal", "Bengaluru Urban", "Hyderabad", "Vijayanagara", "Ballari"]
        try:
            conn = get_connection()
            cursor = conn.cursor()
            cursor.execute("SELECT DISTINCT district FROM parties WHERE district IS NOT NULL AND district != ''")
            rows = [r[0] for r in cursor.fetchall()]
            conn.close()
            for d in defaults:
                if d not in rows:
                    rows.append(d)
            return rows
        except Exception as e:
            return defaults

    @Slot(result=list)
    def get_stations(self):
        defaults = ["Raichur APMC Yard", "Koppal Mandi", "Bengaluru Ganj", "Hyderabad Market", "Karnataka", "Telangana", "Andhra Pradesh"]
        try:
            conn = get_connection()
            cursor = conn.cursor()
            cursor.execute("SELECT DISTINCT state FROM parties WHERE state IS NOT NULL AND state != ''")
            rows = [r[0] for r in cursor.fetchall()]
            conn.close()
            for d in defaults:
                if d not in rows:
                    rows.append(d)
            return rows
        except Exception as e:
            return defaults

    @Slot(str, result=dict)
    def get_party_by_name(self, party_name):
        try:
            conn = get_connection()
            cursor = conn.cursor()
            cursor.execute("SELECT * FROM parties WHERE name = ?", (party_name,))
            row = cursor.fetchone()
            conn.close()
            if row:
                return dict(row)
        except Exception as e:
            print(f"Error fetching party by name: {e}")
        return {}

    @Slot(str, str, str, str, str, str, float, str, str, str, str, str, str, str, str, str, str, str, str, str, str, str, float, int, result=bool)
    def add_ledger_full(self, name, alias, prefix, group_name, party_type, special_type, opening_balance, balance_type, mailing_name, address, city, district, state, pincode, phone, mobile, whatsapp, email, contact_person, gstin, pan, aadhaar, credit_limit, credit_days):
        try:
            conn = get_connection()
            cursor = conn.cursor()
            cursor.execute("""
            INSERT INTO parties 
            (name, alias, prefix, group_name, party_type, special_type, opening_balance, balance_type, mailing_name, address, city, district, state, pincode, phone, mobile, whatsapp, email, contact_person, gstin, pan, aadhaar, credit_limit, credit_days)
            VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
            """, (name, alias, prefix, group_name, party_type, special_type, opening_balance, balance_type, mailing_name, address, city, district, state, pincode, phone, mobile, whatsapp, email, contact_person, gstin, pan, aadhaar, credit_limit, credit_days))
            conn.commit()
            conn.close()
            self.reload_data()
            return True
        except Exception as e:
            print(f"Error adding full ledger: {e}")
            return False

    @Slot(int, str, str, str, str, str, str, float, str, str, str, str, str, str, str, str, str, str, str, str, str, str, str, float, int, result=bool)
    def update_ledger_full(self, party_id, name, alias, prefix, group_name, party_type, special_type, opening_balance, balance_type, mailing_name, address, city, district, state, pincode, phone, mobile, whatsapp, email, contact_person, gstin, pan, aadhaar, credit_limit, credit_days):
        try:
            conn = get_connection()
            cursor = conn.cursor()
            cursor.execute("""
            UPDATE parties 
            SET name = ?, alias = ?, prefix = ?, group_name = ?, party_type = ?, special_type = ?, opening_balance = ?, balance_type = ?, mailing_name = ?, address = ?, city = ?, district = ?, state = ?, pincode = ?, phone = ?, mobile = ?, whatsapp = ?, email = ?, contact_person = ?, gstin = ?, pan = ?, aadhaar = ?, credit_limit = ?, credit_days = ?
            WHERE id = ?
            """, (name, alias, prefix, group_name, party_type, special_type, opening_balance, balance_type, mailing_name, address, city, district, state, pincode, phone, mobile, whatsapp, email, contact_person, gstin, pan, aadhaar, credit_limit, credit_days, party_id))
            conn.commit()
            conn.close()
            self.reload_data()
            return True
        except Exception as e:
            print(f"Error updating full ledger: {e}")
            return False


class DashboardController(QObject):
    statsChanged = Signal()

    def __init__(self, parent=None):
        super().__init__(parent)
        self._paddy_stock = "0.0 Qtl"
        self._rice_stock = "0.0 Qtl"
        self._total_sales = "₹0.0"
        self._total_procurement = "₹0.0"
        self._milling_efficiency = "0.0%"
        self.refresh_stats()

    @Slot()
    def refresh_stats(self):
        try:
            conn = get_connection()
            cursor = conn.cursor()
            
            # Paddy Stock
            cursor.execute("SELECT SUM(current_stock_qtl) FROM inventory WHERE category = 'Raw Paddy'")
            paddy_val = cursor.fetchone()[0] or 0.0
            self._paddy_stock = f"{paddy_val:,.1f} Qtl"

            # Rice Stock
            cursor.execute("SELECT SUM(current_stock_qtl) FROM inventory WHERE category = 'Finished Rice'")
            rice_val = cursor.fetchone()[0] or 0.0
            self._rice_stock = f"{rice_val:,.1f} Qtl"

            # Total Sales
            cursor.execute("SELECT SUM(total_amount) FROM sales_invoices")
            sales_val = cursor.fetchone()[0] or 0.0
            self._total_sales = f"₹{sales_val:,.2f}"

            # Total Procurement
            cursor.execute("SELECT SUM(net_amount) FROM paddy_arrivals")
            proc_val = cursor.fetchone()[0] or 0.0
            self._total_procurement = f"₹{proc_val:,.2f}"

            # Avg Milling Efficiency
            cursor.execute("SELECT AVG(yield_pct) FROM milling_batches")
            eff_val = cursor.fetchone()[0] or 67.0
            self._milling_efficiency = f"{eff_val:.1f}%"

            conn.close()
            self.statsChanged.emit()
        except Exception as e:
            print(f"Error refreshing dashboard stats: {e}")

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
            "Primary / Root Group",
            "Current Assets",
            "Current Liabilities",
            "Direct Expenses",
            "Sales Accounts",
            "Purchase Accounts",
            "Duties & Taxes",
            "Loans (Liability)",
            "Capital Account",
            "Branch / Divisions"
        ]
        try:
            conn = get_connection()
            cursor = conn.cursor()
            cursor.execute("SELECT name FROM account_groups ORDER BY name ASC")
            rows = [r[0] for r in cursor.fetchall()]
            conn.close()
            return rows
        except Exception as e:
            return defaults

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
        self.reload_data()

    @Slot()
    def reload_data(self):
        self.beginResetModel()
        try:
            conn = get_connection()
            cursor = conn.cursor()
            cursor.execute("SELECT * FROM stock_items ORDER BY name ASC")
            rows = cursor.fetchall()
            self._data = [dict(r) for r in rows]
            conn.close()
        except Exception as e:
            print(f"Error reloading stock items: {e}")
            self._data = []
        self.endResetModel()

    @Slot(result=list)
    def get_items_list(self):
        return [r["name"] for r in self._data if "name" in r]

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
        return ["Finished Rice", "Raw Paddy", "By-Product", "Packing Material", "General Goods"]

    @Slot(result=list)
    def get_units(self):
        return ["Qtl", "Bags", "Kg", "Ltr", "Nos", "Pcs", "MT"]

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

    @Slot(result=list)
    def get_stock_register(self):
        try:
            conn = get_connection()
            cursor = conn.cursor()
            cursor.execute("SELECT * FROM stock_items ORDER BY name ASC")
            items = [dict(r) for r in cursor.fetchall()]
            
            result = []
            for item in items:
                name = item.get("name", "")
                op_qty = item.get("opening_qty", 0.0) or 0.0
                
                n_lower = name.lower()
                if "sona" in n_lower: kw = "%Sona%"
                elif "ir64" in n_lower: kw = "%IR64%"
                elif "bran" in n_lower: kw = "%Bran%"
                elif "husk" in n_lower: kw = "%Husk%"
                elif "nakku" in n_lower or "broken" in n_lower: kw = "%Broken%"
                else: kw = f"%{name}%"

                inward_paddy = 0.0
                inward_milling = 0.0
                outward_sales = 0.0

                if "paddy" in n_lower:
                    cursor.execute("SELECT SUM(net_weight_qtl) FROM paddy_arrivals WHERE paddy_variety LIKE ?", (kw,))
                    inward_paddy = cursor.fetchone()[0] or 0.0
                elif "rice" in n_lower and "steam" in n_lower:
                    cursor.execute("SELECT SUM(head_rice_qtl) FROM milling_batches WHERE paddy_variety LIKE ?", (kw,))
                    inward_milling = cursor.fetchone()[0] or 0.0
                    cursor.execute("SELECT SUM(weight_qtl) FROM sales_invoices WHERE item_name LIKE ?", (kw,))
                    outward_sales = cursor.fetchone()[0] or 0.0
                elif "bran" in n_lower:
                    cursor.execute("SELECT SUM(bran_qtl) FROM milling_batches")
                    inward_milling = cursor.fetchone()[0] or 0.0
                    cursor.execute("SELECT SUM(weight_qtl) FROM sales_invoices WHERE item_name LIKE ?", (kw,))
                    outward_sales = cursor.fetchone()[0] or 0.0
                elif "broken" in n_lower or "nakku" in n_lower:
                    cursor.execute("SELECT SUM(broken_rice_qtl) FROM milling_batches")
                    inward_milling = cursor.fetchone()[0] or 0.0
                elif "husk" in n_lower:
                    cursor.execute("SELECT SUM(husk_qtl) FROM milling_batches")
                    inward_milling = cursor.fetchone()[0] or 0.0

                inward_total = inward_paddy + inward_milling
                outward_total = outward_sales
                closing_qty = op_qty + inward_total - outward_total
                
                rate = item.get("sale_rate", 0.0) or item.get("purchase_rate", 0.0) or 2500.0
                closing_val = closing_qty * rate
                
                result.append({
                    "id": item.get("id"),
                    "name": name,
                    "code": item.get("code", ""),
                    "item_type": item.get("item_type", "General Goods"),
                    "unit": item.get("unit", "Qtl"),
                    "opening_qty": op_qty,
                    "inward_qty": inward_total,
                    "outward_qty": outward_total,
                    "closing_qty": closing_qty,
                    "rate": rate,
                    "closing_value": closing_val
                })
            conn.close()
            return result
        except Exception as e:
            print(f"Error computing stock register: {e}")
            return []

    @Slot(str, result=list)
    def get_item_movements(self, item_name):
        try:
            conn = get_connection()
            cursor = conn.cursor()
            movements = []
            
            n_lower = item_name.lower()
            if "sona" in n_lower: kw = "%Sona%"
            elif "ir64" in n_lower: kw = "%IR64%"
            elif "bran" in n_lower: kw = "%Bran%"
            elif "husk" in n_lower: kw = "%Husk%"
            elif "nakku" in n_lower or "broken" in n_lower: kw = "%Broken%"
            else: kw = f"%{item_name}%"

            # 1. Paddy Arrivals (Inward for Paddy)
            if "paddy" in n_lower:
                cursor.execute("SELECT slip_no, arrival_date, farmer_name, bag_count, net_weight_qtl, rate_per_qtl, net_amount FROM paddy_arrivals WHERE paddy_variety LIKE ? ORDER BY arrival_date DESC", (kw,))
                for r in cursor.fetchall():
                    movements.append({
                        "vDate": r[1],
                        "refNo": r[0],
                        "type": "Paddy Arrival (Inward)",
                        "party": r[2],
                        "bags": r[3],
                        "qty": r[4],
                        "rate": r[5],
                        "amount": r[6],
                        "isInward": True
                    })

            # 2. Milling Output Production (Inward for Rice, Bran, Broken, Husk)
            if "rice" in n_lower or "bran" in n_lower or "broken" in n_lower or "husk" in n_lower:
                cursor.execute("SELECT batch_no, batch_date, paddy_variety, head_rice_qtl, broken_rice_qtl, bran_qtl, husk_qtl FROM milling_batches ORDER BY batch_date DESC")
                for r in cursor.fetchall():
                    batch_no, bdate, variety, head_qtl, broken_qtl, bran_qtl, husk_qtl = r
                    out_qty = 0.0
                    if "rice" in n_lower and "steam" in n_lower: out_qty = head_qtl
                    elif "bran" in n_lower: out_qty = bran_qtl
                    elif "broken" in n_lower or "nakku" in n_lower: out_qty = broken_qtl
                    elif "husk" in n_lower: out_qty = husk_qtl

                    if out_qty > 0:
                        movements.append({
                            "vDate": bdate,
                            "refNo": batch_no,
                            "type": "Milling Yield (Inward Production)",
                            "party": "Internal Milling Process (" + variety + ")",
                            "bags": int(out_qty * 2),
                            "qty": out_qty,
                            "rate": 2500.0,
                            "amount": out_qty * 2500.0,
                            "isInward": True
                        })

            # 3. Sales Invoices (Outward)
            cursor.execute("SELECT invoice_no, invoice_date, customer_name, bag_count, weight_qtl, rate_per_qtl, total_amount FROM sales_invoices WHERE item_name LIKE ? ORDER BY invoice_date DESC", (kw,))
            for r in cursor.fetchall():
                movements.append({
                    "vDate": r[1],
                    "refNo": r[0],
                    "type": "Sales Invoice (Outward)",
                    "party": r[2],
                    "bags": r[3],
                    "qty": r[4],
                    "rate": r[5],
                    "amount": r[6],
                    "isInward": False
                })
                
            conn.close()
            movements.sort(key=lambda x: x["vDate"], reverse=True)
            return movements
        except Exception as e:
            print(f"Error fetching item movements: {e}")
            return []
