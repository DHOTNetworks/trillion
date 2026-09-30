#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QVariantList>
#include <QVariantMap>
#include <QVector>
#include <QHash>
#include "group_hierarchy_pipeline.h"

/**
 * @brief Dynamic Ledger & Balances Pipeline
 * 
 * Manages Chart of Accounts Ledgers / Parties, connects each ledger to its Group Node,
 * evaluates 'extract_in_balance_sheet' rule, and computes mathematically unique Dr/Cr balances.
 */
struct LedgerNode {
    int id = 0;
    int legacyId = 0;
    QString name;
    QString printName;
    QString alias;
    int groupId = 0;
    QString groupName;
    QString nature; // Inherited from group
    int extractInBalanceSheet = 1; // 0=Rollup at group level, 1=Individual Ledger line, 2=Schedule
    double openingBalance = 0.0;
    QString openingBalanceType; // "Dr" or "Cr"
    QString gstin;
    QString pan;
    QString mobile;
    QString station;
    QString city;
    QString state;
    QString stateCode;
    QString bankName;
    QString bankAccount;
    QString ifscCode;
    double creditLimit = 0.0;
    int creditDays = 30;
    double interestRate = 0.0;
    bool isTcsApplicable = false;
    bool isTdsApplicable = false;
};

struct LedgerPeriodBalance {
    int accountId = 0;
    int legacyId = 0;
    QString accountName;
    int groupId = 0;
    QString groupName;
    QString nature;
    int extractInBalanceSheet = 1;

    double openingDr = 0.0;
    double openingCr = 0.0;
    double periodDr = 0.0;
    double periodCr = 0.0;
    double closingDr = 0.0;
    double closingCr = 0.0;
    double netBalance = 0.0; // > 0 is Dr, < 0 is Cr
    QString netDrCr;        // "Dr" or "Cr"
};

class LedgerPipeline : public QObject {
    Q_OBJECT

public:
    static LedgerPipeline& instance();

    void invalidateCache();
    QVector<LedgerNode> getAllLedgers();
    LedgerNode getLedgerById(int ledgerId);
    LedgerNode getLedgerByName(const QString& name);
    LedgerNode getLedgerByLegacyId(int legacyId);

    // Dynamic Balance Engine
    QVector<LedgerPeriodBalance> calculateBalancesForPeriod(
        const QString& fromDateIso,
        const QString& toDateIso,
        const QString& filterNature = "",
        int filterGroupId = 0
    );

    LedgerPeriodBalance calculateSingleLedgerBalance(
        int ledgerId,
        const QString& asOnDateIso
    );

    // Balance Sheet Extract Helper: Partitions ledgers according to extract_in_balance_sheet
    struct BalanceSheetPartition {
        QMap<QString, QVector<LedgerPeriodBalance>> groupRollups;    // extract_in_bs == 0 (Rolled up into group)
        QMap<QString, QVector<LedgerPeriodBalance>> itemizedLedgers; // extract_in_bs == 1 (Directly itemized on face)
        QMap<QString, QVector<LedgerPeriodBalance>> schedules;       // extract_in_bs == 2 (Separate schedule)
    };

    BalanceSheetPartition partitionForBalanceSheet(const QString& asOnDateIso);

    // Search & Feeding API for UI components (AccountSearchBox, etc.)
    QVariantList searchLedgers(const QString& query, const QString& filterGroup = "", int limit = 50);
    QVariantList searchLedgers(const QString& query, int filterGroupId, int limit = 50);
    QVariantList searchLedgersByRootCode(const QString& query, int rootCode, int limit = 50);
    QStringList getLedgerNames(const QString& filterGroup = "");
    QStringList getLedgerNames(int filterGroupId);
    QStringList getLedgerNamesByRootCode(int rootCode);

    bool saveLedger(const LedgerNode& node, QString* outError = nullptr);
    bool deleteLedger(int ledgerId, QString* outError = nullptr);

signals:
    void ledgersChanged();

private:
    LedgerPipeline(QObject* parent = nullptr);
    ~LedgerPipeline() override = default;
    LedgerPipeline(const LedgerPipeline&) = delete;
    LedgerPipeline& operator=(const LedgerPipeline&) = delete;

    void ensureLoaded();

    QMutex m_mutex;
    bool m_valid = false;
    QHash<int, LedgerNode> m_byId;
    QHash<QString, LedgerNode> m_byName;
    QHash<int, LedgerNode> m_byLegacyId;
};
