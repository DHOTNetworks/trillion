#include "db_service.h"
#include "db_worker.h"
#include "../database_manager.h"

#include <QCoreApplication>
#include <QDebug>
#include <QPromise>
#include <memory>

namespace MahadevERP {

DbService& DbService::instance() {
    static DbService service;
    return service;
}

DbService::DbService(QObject* parent)
    : QObject(parent)
    // Parent-less on purpose: DbWorker must moveToThread(), which Qt refuses
    // for objects with a parent (affinity silently stays on the UI thread and
    // every later BlockingQueuedConnection into stop() deadlocks the app).
    , m_worker(new DbWorker())
{
}

DbService::~DbService() {
    m_worker->stop();
    delete m_worker;
    m_worker = nullptr;
}

bool DbService::start(const QString& dbPath) {
    if (dbPath.trimmed().isEmpty()) return false;
    // Escape hatch: MAHADEV_SYNC_DB=1 disables the worker (fully legacy
    // sync reads). If a hang disappears with it set, the worker is at fault.
    if (qEnvironmentVariableIntValue("MAHADEV_SYNC_DB") != 0) {
        qInfo() << "DbService disabled via MAHADEV_SYNC_DB; using sync reads.";
        return false;
    }
    return m_worker->start(dbPath);
}

bool DbService::restart(const QString& dbPath) {
    if (dbPath.trimmed().isEmpty()) return false;
    return m_worker->restart(dbPath);
}

void DbService::stop() {
    m_worker->stop();
}

bool DbService::isAsync() const {
    return m_worker->isRunning();
}

QString DbService::dbPath() const {
    return m_worker->dbPath();
}

QFuture<QVariantList> DbService::query(const QString& sql, const QVariantList& params) {
    if (m_worker->isRunning()) return m_worker->asyncQuery(sql, params);
    // Fallback: synchronous legacy path (worker down). Never hangs callers:
    // resolves immediately with real data or empty on SQL error.
    auto promise = std::make_shared<QPromise<QVariantList>>();
    promise->start();
    QFuture<QVariantList> future = promise->future();
    QVariantList rows = DatabaseManager::instance().executeQuery(sql, params);
    promise->addResult(std::move(rows));
    promise->finish();
    return future;
}

QFuture<QVariant> DbService::scalar(const QString& sql, const QVariantList& params) {
    if (m_worker->isRunning()) return m_worker->asyncScalar(sql, params);
    auto promise = std::make_shared<QPromise<QVariant>>();
    promise->start();
    QFuture<QVariant> future = promise->future();
    promise->addResult(DatabaseManager::instance().executeScalar(sql, params));
    promise->finish();
    return future;
}

QFuture<bool> DbService::transact(std::function<bool(DbTx&)> fn) {
    if (m_worker->isRunning()) return m_worker->asyncTransact(std::move(fn));
    auto promise = std::make_shared<QPromise<bool>>();
    promise->start();
    QFuture<bool> future = promise->future();
    promise->addResult(false); // no worker, no atomicity: refuse, don't fake it
    promise->finish();
    return future;
}

qint64 DbService::totalTasks() const {
    return m_worker->totalTasks();
}

int DbService::pendingTasks() const {
    return m_worker->pendingTasks();
}

qint64 DbService::lastElapsedMs() const {
    return m_worker->lastElapsedMs();
}

} // namespace MahadevERP
