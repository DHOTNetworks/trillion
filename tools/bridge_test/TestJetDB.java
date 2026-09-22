
import java.sql.*;
import java.io.File;

public class TestJetDB {
    public static void main(String[] args) {
        try {
            Class.forName("net.ucanaccess.jdbc.UcanaccessDriver");
            String dbPath = new File("Bahi-Khata-Data/Data.002").getAbsolutePath();
            String url = "jdbc:ucanaccess://" + dbPath + ";showSchema=true;memory=false;openExclusive=false";
            System.out.println("Connecting to JetDB: " + url);
            Connection conn = DriverManager.getConnection(url);
            System.out.println("Connected successfully!");
            
            Statement stmt = conn.createStatement();
            ResultSet rs = stmt.executeQuery("SELECT COUNT(*) FROM Transactions");
            if (rs.next()) {
                System.out.println("Total Transactions row count: " + rs.getInt(1));
            }
            rs.close();
            stmt.close();
            conn.close();
            System.out.println("Closed cleanly!");
        } catch (Exception e) {
            e.printStackTrace();
        }
    }
}
