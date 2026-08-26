class AccountingEngine:
    @staticmethod
    def calculate_moisture_deduction(gross_weight_qtl: float, moisture_pct: float, base_moisture_limit: float = 14.0) -> float:
        """
        Calculates moisture weight deduction in Quintals.
        Standard Rice Mill rule: 1% extra moisture over 14% causes ~1% weight deduction.
        """
        if moisture_pct <= base_moisture_limit:
            return 0.0
        excess_moisture = moisture_pct - base_moisture_limit
        deduction_qtl = round(gross_weight_qtl * (excess_moisture / 100.0), 2)
        return deduction_qtl

    @staticmethod
    def calculate_paddy_net_amount(net_weight_qtl: float, rate_per_qtl: float, hamali_charges: float = 0.0) -> float:
        """
        Paddy net payable = (Net Weight Qtl * Rate per Qtl) + Hamali/Handling Charges
        """
        base_amt = net_weight_qtl * rate_per_qtl
        return round(base_amt + hamali_charges, 2)

    @staticmethod
    def calculate_milling_yield(paddy_input_qtl: float, head_rice_qtl: float, broken_rice_qtl: float, bran_qtl: float, husk_qtl: float):
        """
        Calculates milling yield percentage and wastage.
        Standard recovery: ~67% Head Rice, 7% Broken Rice, 8% Bran, 17% Husk, 1% Wastage.
        """
        total_output = head_rice_qtl + broken_rice_qtl + bran_qtl + husk_qtl
        wastage_qtl = max(0.0, round(paddy_input_qtl - total_output, 2))
        yield_pct = round((head_rice_qtl / paddy_input_qtl) * 100.0, 2) if paddy_input_qtl > 0 else 0.0
        return {
            "total_output_qtl": total_output,
            "wastage_qtl": wastage_qtl,
            "yield_pct": yield_pct
        }

    @staticmethod
    def calculate_sales_tax(taxable_amount: float, gst_pct: float = 5.0):
        """
        Calculates GST amount and total sales invoice amount.
        """
        gst_amount = round(taxable_amount * (gst_pct / 100.0), 2)
        total_amount = round(taxable_amount + gst_amount, 2)
        return {
            "gst_amount": gst_amount,
            "total_amount": total_amount
        }
