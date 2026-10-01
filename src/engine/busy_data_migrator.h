#pragma once

#include <QObject>
#include <QString>
#include <QVariantList>
#include <QVariantMap>
#include <QDate>

namespace MahadevERP {

struct BusyMigrationStats {
    int totalAccounts = 0;
    int totalGroups = 0;
    int totalStockItems = 0;
    int totalStockUnits = 0;
    int totalSalesInvoices = 0;
    int totalPurchaseInvoices = 0;
    int totalGlVouchers = 0;
    int totalGlTransactions = 0;
    int totalMandiRecords = 0;
    int totalTdsVouchers = 0;
    double totalDebitSum = 0.0;
    double totalCreditSum = 0.0;
    double glDiscrepancy = 0.0;
    bool isBalanced = true;
    QString companyName;
    QString gstin;
    QString financialYear;
};

class BusyDataMigrator : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool isMigrating READ isMigrating NOTIFY migratingChanged)
    Q_PROPERTY(int progressPercent READ progressPercent NOTIFY progressChanged)
    Q_PROPERTY(QString statusText READ statusText NOTIFY statusChanged)

public:
    explicit BusyDataMigrator(QObject* parent = nullptr);
    ~BusyDataMigrator() override;

    bool isMigrating() const { return m_isMigrating; }
    int progressPercent() const { return m_progressPercent; }
    QString statusText() const { return m_statusText; }

    // Inspect Busy directory or database file without modifying SQLite
    Q_INVOKABLE QVariantMap inspect_busy_data(const QString& busyPath);

    // Full in-process migration of Busy database into target SQLite database
    Q_INVOKABLE bool migrate_busy_data(const QString& busyPath);

    // Native folder/file picker for Busy directory (DATA folder or db.bds)
    Q_INVOKABLE QString choose_busy_directory(const QString& startDir = "");

signals:
    void migratingChanged();
    void progressChanged(int percent);
    void statusChanged(const QString& text);
    void migrationProgress(int percent, const QString& currentStep);
    void migrationFinished(bool success, const QString& summaryMessage);

private:
    void updateProgress(int percent, const QString& status);
    static QString parseMdbDate(const QString& rawDate);
    static QString determineFinancialYear(const QString& isoDate);

    bool m_isMigrating = false;
    int m_progressPercent = 0;
    QString m_statusText = "Idle";
};

} // namespace MahadevERP
