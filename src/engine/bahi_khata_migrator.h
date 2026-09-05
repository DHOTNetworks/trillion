#pragma once

#include <QObject>
#include <QString>
#include <QVariantList>
#include <QVariantMap>

class BahiKhataMigrator : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool isMigrating READ isMigrating NOTIFY migratingChanged)
    Q_PROPERTY(int progressPercent READ progressPercent NOTIFY progressChanged)
    Q_PROPERTY(QString statusText READ statusText NOTIFY statusChanged)

public:
    explicit BahiKhataMigrator(QObject* parent = nullptr);
    ~BahiKhataMigrator() override;

    bool isMigrating() const { return m_isMigrating; }
    int progressPercent() const { return m_progressPercent; }
    QString statusText() const { return m_statusText; }

    // Inspect tables and record counts in MDB without modifying SQLite
    Q_INVOKABLE QVariantMap inspect_mdb_file(const QString& mdbFilePath);

    // Full in-process migration of MDB into mahadev_accounting.db
    Q_INVOKABLE bool migrate_mdb_file(const QString& mdbFilePath);

    // Native file picker for MDB/Data.* database files
    Q_INVOKABLE QString choose_mdb_file(const QString& startDir = "");

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
