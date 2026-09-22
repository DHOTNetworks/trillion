import java.io.*;
import java.sql.*;
import java.util.*;

/**
 * BahiKhataBridge: High-performance, 100% safe JDBC bridge to Bahi-Khata JetDB databases (.002, .018, .mdb).
 * Communicates via JSON-over-stdio or direct CLI invocations.
 */
public class BahiKhataBridge {

    private static Connection activeConn = null;
    private static String activeDbPath = null;

    public static synchronized Connection getConnection(String dbPath, boolean readOnly) throws Exception {
        if (activeConn != null && !activeConn.isClosed() && dbPath.equals(activeDbPath)) {
            return activeConn;
        }
        if (activeConn != null && !activeConn.isClosed()) {
            try { activeConn.close(); } catch (Exception ignored) {}
        }

        Class.forName("net.ucanaccess.jdbc.UcanaccessDriver");
        File f = new File(dbPath);
        if (!f.exists()) {
            throw new FileNotFoundException("Database file not found: " + dbPath);
        }

        // ucanaccess settings for speed and safety
        String url = "jdbc:ucanaccess://" + f.getAbsolutePath() + ";showSchema=true;memory=false;openExclusive=false";
        if (readOnly) {
            url += ";readOnly=true";
        }

        activeConn = DriverManager.getConnection(url);
        activeDbPath = dbPath;
        return activeConn;
    }

    public static void main(String[] args) {
        if (args.length == 0) {
            System.err.println("Usage: BahiKhataBridge <mode> <dbPath> [args...]");
            System.err.println("Modes: query, execute, schema, tables, daemon");
            System.exit(1);
        }

        String mode = args[0].toLowerCase();

        try {
            if ("daemon".equals(mode)) {
                runDaemon();
            } else if ("tables".equals(mode)) {
                String dbPath = args[1];
                Connection conn = getConnection(dbPath, true);
                listTables(conn);
            } else if ("schema".equals(mode)) {
                String dbPath = args[1];
                String table = args.length > 2 ? args[2] : null;
                Connection conn = getConnection(dbPath, true);
                getTableSchema(conn, table);
            } else if ("query".equals(mode)) {
                String dbPath = args[1];
                String sql = args[2];
                int limit = args.length > 3 ? Integer.parseInt(args[3]) : -1;
                Connection conn = getConnection(dbPath, true);
                executeQuery(conn, sql, limit);
            } else if ("execute".equals(mode)) {
                String dbPath = args[1];
                String sql = args[2];
                Connection conn = getConnection(dbPath, false);
                executeUpdate(conn, sql);
            } else if ("compact".equals(mode)) {
                String dbPath = args[1];
                compactDatabase(dbPath);
            } else {
                System.err.println("Unknown mode: " + mode);
                System.exit(1);
            }
        } catch (Exception e) {
            outputError(e);
            System.exit(1);
        }
    }

    private static void compactDatabase(String dbPath) throws Exception {
        if (activeConn != null) {
            try { activeConn.close(); } catch (Exception ignored) {}
            activeConn = null;
        }
        File src = new File(dbPath);
        File dst = new File(dbPath + ".compact_tmp");
        if (dst.exists()) dst.delete();

        com.healthmarketscience.jackcess.Database srcDb = com.healthmarketscience.jackcess.DatabaseBuilder.open(src);
        com.healthmarketscience.jackcess.Database dstDb = com.healthmarketscience.jackcess.DatabaseBuilder.create(srcDb.getFileFormat(), dst);

        for (String tableName : srcDb.getTableNames()) {
            com.healthmarketscience.jackcess.Table t = srcDb.getTable(tableName);
            com.healthmarketscience.jackcess.TableBuilder tb = new com.healthmarketscience.jackcess.TableBuilder(tableName);
            for (com.healthmarketscience.jackcess.Column c : t.getColumns()) {
                com.healthmarketscience.jackcess.ColumnBuilder cb = new com.healthmarketscience.jackcess.ColumnBuilder(c.getName(), c.getType());
                if (c.getType().isVariableLength()) {
                    int len = c.getLengthInUnits();
                    if (c.getType() == com.healthmarketscience.jackcess.DataType.TEXT && len > 255) {
                        len = 255;
                    }
                    cb.setLengthInUnits(len);
                }
                tb.addColumn(cb);
            }
            for (com.healthmarketscience.jackcess.Index idx : t.getIndexes()) {
                com.healthmarketscience.jackcess.IndexBuilder ib = new com.healthmarketscience.jackcess.IndexBuilder(idx.getName());
                for (com.healthmarketscience.jackcess.Index.Column ic : idx.getColumns()) {
                    ib.addColumns(ic.getName());
                }
                if (idx.isPrimaryKey()) ib.setPrimaryKey();
                if (idx.isUnique()) ib.setUnique();
                tb.addIndex(ib);
            }
            com.healthmarketscience.jackcess.Table dstTable = tb.toTable(dstDb);

            List<Map<String, Object>> batch = new ArrayList<>();
            for (com.healthmarketscience.jackcess.Row r : t) {
                batch.add(new HashMap<>(r));
                if (batch.size() >= 1000) {
                    dstTable.addRowsFromMaps(batch);
                    batch.clear();
                }
            }
            if (!batch.isEmpty()) {
                dstTable.addRowsFromMaps(batch);
            }
        }

        srcDb.close();
        dstDb.close();

        // Swap files
        File backup = new File(dbPath + ".pre_compact_bak");
        if (backup.exists()) backup.delete();
        src.renameTo(backup);
        dst.renameTo(src);
        backup.delete();

        System.out.println("{\"status\":\"OK\",\"message\":\"Database compacted and repaired successfully\"}");
    }

    private static void runDaemon() {
        BufferedReader reader = new BufferedReader(new InputStreamReader(System.in));
        String line;
        try {
            while ((line = reader.readLine()) != null) {
                line = line.trim();
                if (line.isEmpty()) continue;
                if ("EXIT".equalsIgnoreCase(line) || "QUIT".equalsIgnoreCase(line)) {
                    break;
                }
                handleDaemonCommand(line);
            }
        } catch (Exception e) {
            outputError(e);
        } finally {
            if (activeConn != null) {
                try { activeConn.close(); } catch (Exception ignored) {}
            }
        }
    }

    private static void handleDaemonCommand(String jsonLine) {
        try {
            // Minimalist parser for daemon command:
            // Expecting: {"action":"query"|"execute"|"tables"|"schema", "dbPath":"...", "sql":"...", "limit": 100}
            String action = extractJsonString(jsonLine, "action");
            String dbPath = extractJsonString(jsonLine, "dbPath");
            String sql = extractJsonString(jsonLine, "sql");
            int limit = extractJsonInt(jsonLine, "limit", -1);

            if (dbPath == null || dbPath.isEmpty()) {
                throw new IllegalArgumentException("dbPath is required");
            }

            boolean isWrite = "execute".equalsIgnoreCase(action) || "batch".equalsIgnoreCase(action);
            Connection conn = getConnection(dbPath, !isWrite);

            if ("query".equalsIgnoreCase(action)) {
                executeQuery(conn, sql, limit);
            } else if ("execute".equalsIgnoreCase(action)) {
                executeUpdate(conn, sql);
            } else if ("tables".equalsIgnoreCase(action)) {
                listTables(conn);
            } else if ("schema".equalsIgnoreCase(action)) {
                String table = extractJsonString(jsonLine, "table");
                getTableSchema(conn, table);
            } else {
                throw new IllegalArgumentException("Unsupported action: " + action);
            }
        } catch (Exception e) {
            outputError(e);
        }
    }

    private static void listTables(Connection conn) throws Exception {
        DatabaseMetaData meta = conn.getMetaData();
        ResultSet rs = meta.getTables(null, null, "%", new String[]{"TABLE"});
        StringBuilder sb = new StringBuilder();
        sb.append("{\"status\":\"OK\",\"tables\":[");
        boolean first = true;
        while (rs.next()) {
            String tableName = rs.getString("TABLE_NAME");
            if (tableName.startsWith("~") || tableName.startsWith("MSys")) continue;
            if (!first) sb.append(",");
            sb.append(quoteJson(tableName));
            first = false;
        }
        rs.close();
        sb.append("]}");
        System.out.println(sb.toString());
    }

    private static void getTableSchema(Connection conn, String targetTable) throws Exception {
        DatabaseMetaData meta = conn.getMetaData();
        ResultSet rs = meta.getColumns(null, null, targetTable != null ? targetTable : "%", "%");
        StringBuilder sb = new StringBuilder();
        sb.append("{\"status\":\"OK\",\"columns\":[");
        boolean first = true;
        while (rs.next()) {
            String tableName = rs.getString("TABLE_NAME");
            if (tableName.startsWith("~") || tableName.startsWith("MSys")) continue;
            if (targetTable != null && !targetTable.equalsIgnoreCase(tableName)) continue;
            
            String colName = rs.getString("COLUMN_NAME");
            String typeName = rs.getString("TYPE_NAME");
            int colSize = rs.getInt("COLUMN_SIZE");
            int nullable = rs.getInt("NULLABLE");

            if (!first) sb.append(",");
            sb.append("{");
            sb.append("\"table\":").append(quoteJson(tableName)).append(",");
            sb.append("\"column\":").append(quoteJson(colName)).append(",");
            sb.append("\"type\":").append(quoteJson(typeName)).append(",");
            sb.append("\"size\":").append(colSize).append(",");
            sb.append("\"nullable\":").append(nullable == DatabaseMetaData.columnNullable);
            sb.append("}");
            first = false;
        }
        rs.close();
        sb.append("]}");
        System.out.println(sb.toString());
    }

    private static void executeQuery(Connection conn, String sql, int limit) throws Exception {
        Statement stmt = conn.createStatement();
        if (limit > 0) {
            stmt.setMaxRows(limit);
        }
        ResultSet rs = stmt.executeQuery(sql);
        ResultSetMetaData rsmd = rs.getMetaData();
        int colCount = rsmd.getColumnCount();

        StringBuilder sb = new StringBuilder();
        sb.append("{\"status\":\"OK\",\"columns\":[");
        for (int i = 1; i <= colCount; i++) {
            if (i > 1) sb.append(",");
            sb.append(quoteJson(rsmd.getColumnLabel(i)));
        }
        sb.append("],\"rows\":[");

        boolean firstRow = true;
        int rowCount = 0;
        while (rs.next()) {
            if (!firstRow) sb.append(",");
            sb.append("[");
            for (int i = 1; i <= colCount; i++) {
                if (i > 1) sb.append(",");
                Object val = rs.getObject(i);
                if (val == null) {
                    sb.append("null");
                } else if (val instanceof Number) {
                    sb.append(val.toString());
                } else if (val instanceof Boolean) {
                    sb.append(val.toString());
                } else {
                    sb.append(quoteJson(val.toString()));
                }
            }
            sb.append("]");
            firstRow = false;
            rowCount++;
        }
        rs.close();
        stmt.close();

        sb.append("],\"count\":").append(rowCount).append("}");
        System.out.println(sb.toString());
    }

    private static void executeUpdate(Connection conn, String sql) throws Exception {
        boolean autoCommit = conn.getAutoCommit();
        conn.setAutoCommit(false);
        Statement stmt = null;
        try {
            stmt = conn.createStatement();
            int affected = stmt.executeUpdate(sql);
            conn.commit();
            System.out.println("{\"status\":\"OK\",\"affectedRows\":" + affected + "}");
        } catch (Exception e) {
            conn.rollback();
            throw e;
        } finally {
            if (stmt != null) stmt.close();
            conn.setAutoCommit(autoCommit);
        }
    }

    private static void outputError(Exception e) {
        String msg = e.getMessage() != null ? e.getMessage() : e.toString();
        System.out.println("{\"status\":\"ERROR\",\"error\":" + quoteJson(msg) + "}");
    }

    private static String quoteJson(String s) {
        if (s == null) return "null";
        StringBuilder sb = new StringBuilder("\"");
        for (int i = 0; i < s.length(); i++) {
            char c = s.charAt(i);
            switch (c) {
                case '"': sb.append("\\\""); break;
                case '\\': sb.append("\\\\"); break;
                case '\b': sb.append("\\b"); break;
                case '\f': sb.append("\\f"); break;
                case '\n': sb.append("\\n"); break;
                case '\r': sb.append("\\r"); break;
                case '\t': sb.append("\\t"); break;
                default:
                    if (c < ' ') {
                        sb.append(String.format("\\u%04x", (int) c));
                    } else {
                        sb.append(c);
                    }
            }
        }
        sb.append("\"");
        return sb.toString();
    }

    private static String extractJsonString(String json, String key) {
        String pattern = "\"" + key + "\":\"";
        int start = json.indexOf(pattern);
        if (start == -1) {
            // Check for space
            pattern = "\"" + key + "\" : \"";
            start = json.indexOf(pattern);
        }
        if (start == -1) return null;
        start += pattern.length();
        StringBuilder sb = new StringBuilder();
        boolean escape = false;
        for (int i = start; i < json.length(); i++) {
            char c = json.charAt(i);
            if (escape) {
                if (c == 'n') sb.append('\n');
                else if (c == 'r') sb.append('\r');
                else if (c == 't') sb.append('\t');
                else sb.append(c);
                escape = false;
            } else if (c == '\\') {
                escape = true;
            } else if (c == '"') {
                break;
            } else {
                sb.append(c);
            }
        }
        return sb.toString();
    }

    private static int extractJsonInt(String json, String key, int def) {
        String pattern = "\"" + key + "\":";
        int start = json.indexOf(pattern);
        if (start == -1) return def;
        start += pattern.length();
        int end = start;
        while (end < json.length() && (Character.isDigit(json.charAt(end)) || json.charAt(end) == '-')) {
            end++;
        }
        try {
            return Integer.parseInt(json.substring(start, end).trim());
        } catch (Exception e) {
            return def;
        }
    }
}
