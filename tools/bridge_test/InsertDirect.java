
import java.io.File;
import java.sql.Timestamp;
import java.text.SimpleDateFormat;
import com.healthmarketscience.jackcess.*;

public class InsertDirect {
    public static void main(String[] args) {
        try {
            File dbFile = new File("Bahi-Khata-Data/Data.018");
            Database db = new DatabaseBuilder(dbFile).setReadOnly(false).open();
            Table table = db.getTable("Transactions");
            
            SimpleDateFormat sdf = new SimpleDateFormat("yyyy-MM-dd HH:mm:ss");
            Timestamp ts = new Timestamp(sdf.parse("2026-01-25 00:00:00").getTime());
            
            // Row 3 for Voucher 525: TDS Cr 2688.0
            Object[] row = new Object[table.getColumnCount()];
            table.addRow(
                (short)3,               // RowNo
                525,                    // VoucherNumber
                ts,                     // VoucherDate
                "Purc",                 // TransType
                (short)1442,            // AccountCode
                "Cr",                   // DrCr
                2688.0,                 // Amount
                "MRI/2526-669",         // InvoiceNo
                null,                   // PartyCode
                null,                   // JFormSaleStatus
                null,                   // IFormTaxStatus
                null,                   // IFormMarketFeeStatus
                null,                   // PurchaseType
                null,                   // CashCreditStatus
                "",                     // Narration
                null,                   // EntryType
                null, null, null, null, null, null, null, null, null, null, null, null, null, null,
                null, null, null, null, null, null, null, null, null, null, null, null, null, null,
                null, null, null, null, null, null, null, null, null, null, null, null, null, null,
                null, null, null, null, null, null, null, null, null, null, null, null, null, null,
                null, null, null, null, null, null, null, null, null, null, null, null, null, null,
                null, null, null, null, null, null, null, null, null, null, null, null, null, null,
                null, null, null, null, null, null, null, null, null, null, null, null, null, null,
                null, null, null, null, null, null, null, null, null, null, null, null, null, null
            );
            
            db.close();
            System.out.println("SUCCESSFULLY INSERTED ROW 3 VIA JACKCESS NATIVE!");
        } catch (Exception e) {
            e.printStackTrace();
        }
    }
}
