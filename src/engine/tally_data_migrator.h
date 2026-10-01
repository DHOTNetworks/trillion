#pragma once

#include <QObject>
#include <QString>
#include <QVariantMap>
#include <QList>
#include <QDate>

namespace MahadevERP {

struct TallyMigrationStats {
    int totalUnits = 0;
    int totalGroups = 0;
    int totalAccounts = 0;
    int totalStockItems = 0;
    int totalVouchers = 0;
    int totalTransactions = 0;
    int totalInventoryEntries = 0;
    double totalDr = 0.0;
    double totalCr = 0.0;
    double glDifference = 0.0;
    bool isBalanced = true;
};

class TallyDataMigrator : public QObject {
    Q_OBJECT

public:
    explicit TallyDataMigrator(QObject* parent = nullptr);
    ~TallyDataMigrator() override = default;

    // Inspection: Non-destructive dry-run analysis of Tally XML or directory
    QVariantMap inspect_tally_data(const QString& tallyPath);

    // Full Migration: Imports Units, Groups, Ledgers, Items, Vouchers & Inventory
    bool migrate_tally_data(const QString& tallyPath, const QString& targetDbPath = QString());

    // File Dialog Helper
    static QString choose_tally_path(const QString& startDir = QString());

signals:
    void progressChanged(int percent);
    void statusChanged(const QString& statusText);
    void migrationProgress(int percent, const QString& status);
    void migrationFinished(bool success, const QString& message);

private:
    void updateProgress(int percent, const QString& status);
    static QString normalizePath(const QString& p);
    static QString parseDateStr(const QString& raw);
    static QString computeFy(const QString& isoDate);
    static QString cleanStr(const QString& s);
    static QString resolveState(const QString& rawState);
    static QString getStateCode(const QString& stateName);

    int m_progressPercent = 0;
    QString m_statusText;
};

} // namespace MahadevERP
