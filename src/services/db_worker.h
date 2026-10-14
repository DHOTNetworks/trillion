#pragma once

// DbWorker: the ONLY object allowed to touch its own SQLite connection.
// It lives on a dedicated QThread; the GUI thread never executes SQL here —
// it only posts functors via Qt::QueuedConnection and receives QFutures.
//
// Isolation contract (see docs/erp-roadmap/01_DATABASE_SINGLE_SOURCE_OF_TRUTH.md):
//  - One connection per thread, exactly one worker thread. SQLite's locking
//    plus WAL lets worker reads proceed while the legacy UI-thread
//    connection writes, and vice versa (5 s busy_timeout both sides).
//  - Transactions are single tasks: DbTx only exists inside asyncTransact.
//    Never split begin/commit across thread hops.
//  - No DDL here, ever. DatabaseManager::ensureTablesExist stays the single
//    DDL owner; the worker only runs DML/SELECT against the open file.
//  - Every task is timed; failures resolve the future with an error string
//    instead of hanging the caller. Callers keep last-good data on error.

#include <QFuture>
#include <QObject>
#include <QString>
#include <QThread>
#include <QVariantList>
#include <QVariantMap>
#include <atomic>
#include <QMutex>
#include <functional>

#include "sqlite3.h"

namespace MahadevERP {

/// Transaction handle. Valid ONLY inside the asyncTransact functor, on the
/// worker thread. RAII at the worker level: false return or any exception
/// rolls everything back; only a clean true commits.
class DbTx {
public:
    // Wraps a live connection handle. Public so legacy sync wrappers can run
    // the same cores on the UI thread; the "only inside transact or a sync
    // wrapper transaction" lifetime rule is a documented contract, not a type
    // gate (same discipline as the old begin/commit pairing).
    explicit DbTx(sqlite3* db) : m_db(db) {}
    [[nodiscard]] QVariantList query(const QString& sql, const QVariantList& params = {});
    [[nodiscard]] bool exec(const QString& sql, const QVariantList& params = {});
    // Fire-and-forget for idempotent cleanup / best-effort legs where legacy
    // behavior ignores failure and keeps the transaction alive. Prefer exec().
    bool execUnchecked(const QString& sql, const QVariantList& params = {});
    [[nodiscard]] QVariant scalar(const QString& sql, const QVariantList& params = {});
    [[nodiscard]] QString lastError() const { return m_lastError; }
    /// Connection-scoped last row id (correct on whichever thread owns tx).
    [[nodiscard]] qint64 lastInsertId();
    /// Ledger resolve-or-create with the canonical match order (exact NOCASE
    /// → spaceless → prefix both ways → create). Shared by every voucher
    /// port so masters never diverge by writer.
    [[nodiscard]] int resolvePartyId(int existingId, const QString& name,
                                     const QString& defaultGroup,
                                     const QString& partyType);

private:
    sqlite3* m_db = nullptr;
    QString m_lastError;
};

class DbWorker : public QObject {
    Q_OBJECT
public:
    explicit DbWorker(QObject* parent = nullptr);
    ~DbWorker() override;

    DbWorker(const DbWorker&) = delete;
    DbWorker& operator=(const DbWorker&) = delete;

    /// Opens the worker connection on the worker thread and starts it.
    /// Idempotent: second call with the same path is a no-op returning true.
    [[nodiscard]] bool start(const QString& dbPath);
    /// Reopens a different file (firm switch). Drains, closes, reopens.
    [[nodiscard]] bool restart(const QString& dbPath);
    /// Blocks until queued tasks finish, joins the thread, closes the handle.
    void stop();

    [[nodiscard]] bool isRunning() const;
    [[nodiscard]] QString dbPath() const;
    [[nodiscard]] qint64 totalTasks() const { return m_totalTasks.load(); }
    [[nodiscard]] int pendingTasks() const { return m_pending.load(); }
    [[nodiscard]] qint64 lastElapsedMs() const { return m_lastElapsedMs.load(); }

    // All async: safe to call from any thread, resolve on completion.
    // If the worker is down, futures resolve immediately with an error.
    [[nodiscard]] QFuture<QVariantList> asyncQuery(const QString& sql,
                                                   const QVariantList& params = {});
    [[nodiscard]] QFuture<QVariant> asyncScalar(const QString& sql,
                                                const QVariantList& params = {});
    [[nodiscard]] QFuture<bool> asyncExec(const QString& sql,
                                          const QVariantList& params = {});
    /// fn runs inside BEGIN IMMEDIATE..COMMIT on the worker thread.
    /// false return or exception => ROLLBACK, future resolves false.
    [[nodiscard]] QFuture<bool> asyncTransact(std::function<bool(DbTx&)> fn);

private:
    bool openOnThread(const QString& dbPath);
    void closeOnThread();
    QVariantList runQuery(const QString& sql, const QVariantList& params, QString* error);
    QVariant runScalar(const QString& sql, const QVariantList& params, QString* error);
    bool runExec(const QString& sql, const QVariantList& params, QString* error);

    // Packages work into a promise + queued functor. Down worker resolves
    // empty immediately (never hangs the caller). Defined in db_worker.cpp;
    // instantiated only there, so no header definition needed.
    template <typename T>
    QFuture<T> postTask(std::function<T(QString*)> work);

    QThread* m_thread = nullptr;
    sqlite3* m_db = nullptr; // touched ONLY on m_thread
    QString m_dbPath;
    mutable QMutex m_stateMutex; // guards start/restart/stop + m_dbPath only
    std::atomic<qint64> m_totalTasks = 0;
    std::atomic<int> m_pending = 0;
    std::atomic<qint64> m_lastElapsedMs = 0;
};

} // namespace MahadevERP
