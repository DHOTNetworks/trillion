#pragma once

// DbService: the UI thread's ONLY gateway to worker-thread SQL.
//
// Migration rule (enforced by CI grep): widgets/models may call
// DatabaseManager directly today, but every NEW read path goes through here,
// and migrated paths delete their UI-thread executeQuery. Terminal state:
// src/widgets/* includes DbService, never DatabaseManager.
//
// Behavior contract:
//  - query/scalar/transact return QFutures; attach a QFutureWatcher, never
//    .waitForFinished() on the GUI thread.
//  - A future with resultCount() == 0 means "worker down or SQL error" —
//    keep last-good data, never blank the screen.
//  - Writes still go through DatabaseManager on the UI thread until their
//    voucher is migrated (migration-strangler: one migrator per slice).
//    Reads via the worker already proceed concurrently thanks to WAL +
//    busy_timeout.
//  - nextGeneration() hands out stale-result discard tokens: capture one per
//    request, ignore watcher results from older generations.

#include <QFuture>
#include <QObject>
#include <QPromise>
#include <QString>
#include <QVariantList>
#include <QVariantMap>
#include <atomic>
#include <functional>

#include "db_worker.h"

namespace MahadevERP {

class DbTx;
class DbWorker;

class DbService : public QObject {
    Q_OBJECT
public:
    static DbService& instance();

    DbService(const DbService&) = delete;
    DbService& operator=(const DbService&) = delete;
    ~DbService();

    /// Starts the worker on dbPath. Idempotent for the same path.
    [[nodiscard]] bool start(const QString& dbPath);
    /// Firm switch: drains, reopens the new file, resumes.
    [[nodiscard]] bool restart(const QString& dbPath);
    void stop();

    [[nodiscard]] bool isAsync() const;
    [[nodiscard]] QString dbPath() const;

    // Async reads. Fall back to sync DatabaseManager when the worker is
    // down (headless tests, early startup) so callers always resolve.
    [[nodiscard]] QFuture<QVariantList> query(const QString& sql,
                                              const QVariantList& params = {});
    [[nodiscard]] QFuture<QVariant> scalar(const QString& sql,
                                           const QVariantList& params = {});
    // Atomic unit of work on the worker connection (generic-ERP: migrated writes).
    [[nodiscard]] QFuture<bool> transact(std::function<bool(DbTx&)> fn);

    // Stale-result discard tokens for debounced search/showEvent reloads.
    [[nodiscard]] quint64 nextGeneration() { return m_generation.fetch_add(1) + 1; }

    // Pre-refused future for validation rejections (no thread hop, never hangs).
    template <typename T>
    [[nodiscard]] static QFuture<T> refused(const T& value = T()) {
        QPromise<T> promise;
        promise.start();
        QFuture<T> future = promise.future();
        promise.addResult(value);
        promise.finish();
        return future;
    }

    // Observability (debug builds / diagnostics dialog).
    [[nodiscard]] qint64 totalTasks() const;
    [[nodiscard]] int pendingTasks() const;
    [[nodiscard]] qint64 lastElapsedMs() const;

private:
    explicit DbService(QObject* parent = nullptr);
    DbWorker* m_worker = nullptr;
    std::atomic<quint64> m_generation = 0;
};

} // namespace MahadevERP
