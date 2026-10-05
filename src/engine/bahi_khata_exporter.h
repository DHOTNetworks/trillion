#pragma once

#include <QObject>
#include <QString>
#include <QDate>
#include <QVariantMap>
#include <QVariantList>

namespace MahadevERP {

/**
 * @brief High-precision, bidirectional Bahi-Khata JetDB (MDB) Exporter.
 * 
 * Exports the complete SQLite database back into Bahi-Khata's native Jet 4 (Data.001, Data.002, etc.)
 * database format with 100% schema alignment and mathematical precision.
 * 
 * Supports:
 *  - Native Windows ODBC (Microsoft Access Driver / Jet 4.0 engine) for transactional SQL fidelity.
 *  - Embedded cross-platform libmdb engine for macOS / Linux direct page & record insertion.
 *  - Comprehensive table coverage: CompanyInfo, Groups, Ledgers, Transactions, StockItems,
 *    MillingVouchers, GateRegister, BardanaTransactions, SaudaVouchers, TDSDeductions.
 */
class BahiKhataExporter : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool isExporting READ isExporting NOTIFY exportingChanged)
    Q_PROPERTY(int progressPercent READ progressPercent NOTIFY progressChanged)
    Q_PROPERTY(QString statusText READ statusText NOTIFY statusChanged)

public:
    struct ExportOptions {
        QString targetMdbPath;        // Path to target Data.00x file
        QString templateMdbPath;      // Path to blank/seed Data.00x template
        QString financialYear;        // Optional FY filter (e.g. "FY 2025-26" or empty for all)
        bool createBackup = true;     // Create .bak before overwriting target
        bool exportTransactions = true;
        bool exportMastersOnly = false;
        bool exportMillingAndStock = true;
    };

    struct ExportSummary {
        bool success = false;
        QString errorMessage;
        int groupsExported = 0;
        int ledgersExported = 0;
        int transactionsExported = 0;
        int stockItemsExported = 0;
        int millingExported = 0;
        int bardanaExported = 0;
        int gateRegisterExported = 0;
        double totalDebitAmount = 0.0;
        double totalCreditAmount = 0.0;
        QString targetFilePath;
    };

    explicit BahiKhataExporter(QObject* parent = nullptr);
    ~BahiKhataExporter() override;

    bool isExporting() const { return m_isExporting; }
    int progressPercent() const { return m_progressPercent; }
    QString statusText() const { return m_statusText; }

    // Primary export entry point
    ExportSummary exportToBahiKhata(const ExportOptions& options);

    // Q_INVOKABLE methods for UI integration
    Q_INVOKABLE bool exportDatabase(const QString& targetMdbPath, const QString& financialYear = "");
    Q_INVOKABLE QString chooseTargetMdbFile(const QString& startDir = "");

signals:
    void exportingChanged();
    void progressChanged(int percent);
    void statusChanged(const QString& text);
    void exportProgress(int percent, const QString& stepDescription);
    void exportFinished(bool success, const QString& summaryMessage);

private:
    void updateProgress(int percent, const QString& status);
    
    // ODBC Windows Engine
    ExportSummary exportViaOdbc(const ExportOptions& options, const QString& targetPath);

    // Embedded LibMdb Engine (macOS / Linux / Windows Fallback)
    ExportSummary exportViaLibMdb(const ExportOptions& options, const QString& targetPath);

    // Helper functions
    static QString resolveSeedTemplate(const QString& explicitPath);
    static double round2(double val);
    static QString formatMdbDate(const QString& isoDate);

    bool m_isExporting = false;
    int m_progressPercent = 0;
    QString m_statusText = "Idle";
};

} // namespace MahadevERP
