#include "db_worker.h"

#include <QCoreApplication>
#include <QDebug>
#include <QElapsedTimer>
#include <QMetaObject>
#include <QMutexLocker>
#include <QPromise>
#include <QVariant>
#include <QtGlobal>
#include <future>
#include <memory>

namespace MahadevERP {

namespace {

// SQLite type -> QVariant (single canonical mapping, shared by query/scalar).
QVariant columnToVariant(sqlite3_stmt* stmt, int col) {
    switch (sqlite3_column_type(stmt, col)) {
    case SQLITE_INTEGER:
        return QVariant::fromValue(sqlite3_column_int64(stmt, col));
    case SQLITE_FLOAT:
        return QVariant::fromValue(sqlite3_column_double(stmt, col));
    case SQLITE_TEXT:
        return QString::fromUtf8(
            reinterpret_cast<const char*>(sqlite3_column_text(stmt, col)),
            sqlite3_column_bytes(stmt, col));
    case SQLITE_BLOB: {
        const void* blob = sqlite3_column_blob(stmt, col);
        const int bytes = sqlite3_column_bytes(stmt, col);
        return QByteArray(static_cast<const char*>(blob), bytes);
    }
    case SQLITE_NULL:
    default:
        return QVariant();
    }
}

// QVariant -> SQLite bind (mirrors DatabaseManager's binding contract:
// int/longlong -> INT64, double -> DOUBLE, QString/byte array -> TEXT,
// null/invalid -> NULL, everything else -> TEXT).
bool bindParams(sqlite3_stmt* stmt, const QVariantList& params) {
    for (int i = 0; i < params.size(); ++i) {
        const QVariant& p = params.at(i);
        const int idx = i + 1;
        int rc = SQLITE_ERROR;
        switch (p.typeId()) {
        case QMetaType::Int:
        case QMetaType::LongLong:
        case QMetaType::UInt:
        case QMetaType::ULongLong:
        case QMetaType::Bool:
            rc = sqlite3_bind_int64(stmt, idx, p.toLongLong());
            break;
        case QMetaType::Double:
        case QMetaType::Float:
            rc = sqlite3_bind_double(stmt, idx, p.toDouble());
            break;
        default:
            if (p.isNull() || !p.isValid()) {
                rc = sqlite3_bind_null(stmt, idx);
            } else {
                const QByteArray bytes = p.toString().toUtf8();
                rc = sqlite3_bind_text(stmt, idx, bytes.constData(), bytes.size(), SQLITE_TRANSIENT);
            }
            break;
        }
        if (rc != SQLITE_OK) return false;
    }
    return true;
}

void applyPragmas(sqlite3* db) {
    static const char* kPragmas[] = {
        "PRAGMA journal_mode=WAL;",
        "PRAGMA synchronous=NORMAL;",
        "PRAGMA foreign_keys=ON;",
        "PRAGMA busy_timeout=5000;",
        "PRAGMA cache_size=-64000;",
        "PRAGMA temp_store=MEMORY;",
    };
    for (const char* pragma : kPragmas) {
        sqlite3_stmt* stmt = nullptr;
        if (sqlite3_prepare_v2(db, pragma, -1, &stmt, nullptr) == SQLITE_OK) {
            sqlite3_step(stmt);
            sqlite3_finalize(stmt);
        }
    }
}

} // namespace

QVariantList DbTx::query(const QString& sql, const QVariantList& params) {
    QVariantList rows;
    sqlite3_stmt* stmt = nullptr;
    const QByteArray utf8 = sql.toUtf8();
    if (sqlite3_prepare_v2(m_db, utf8.constData(), utf8.size(), &stmt, nullptr) != SQLITE_OK) {
        m_lastError = QString::fromUtf8(sqlite3_errmsg(m_db));
        return rows;
    }
    if (!bindParams(stmt, params)) {
        m_lastError = QString::fromUtf8(sqlite3_errmsg(m_db));
        sqlite3_finalize(stmt);
        return rows;
    }
    const int cols = sqlite3_column_count(stmt);
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        QVariantMap row;
        for (int c = 0; c < cols; ++c) {
            row.insert(QString::fromUtf8(sqlite3_column_name(stmt, c)),
                       columnToVariant(stmt, c));
        }
        rows.append(row);
    }
    sqlite3_finalize(stmt);
    return rows;
}

bool DbTx::exec(const QString& sql, const QVariantList& params) {
    sqlite3_stmt* stmt = nullptr;
    const QByteArray utf8 = sql.toUtf8();
    if (sqlite3_prepare_v2(m_db, utf8.constData(), utf8.size(), &stmt, nullptr) != SQLITE_OK) {
        m_lastError = QString::fromUtf8(sqlite3_errmsg(m_db));
        return false;
    }
    if (!bindParams(stmt, params)) {
        m_lastError = QString::fromUtf8(sqlite3_errmsg(m_db));
        sqlite3_finalize(stmt);
        return false;
    }
    const int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    if (rc != SQLITE_DONE && rc != SQLITE_ROW) {
        m_lastError = QString::fromUtf8(sqlite3_errmsg(m_db));
        return false;
    }
    return true;
}

QVariant DbTx::scalar(const QString& sql, const QVariantList& params) {
    const QVariantList rows = query(sql, params);
    if (rows.isEmpty()) return QVariant();
    const QVariantMap row = rows.first().toMap();
    if (row.isEmpty()) return QVariant();
    return row.constBegin().value();
}

bool DbTx::execUnchecked(const QString& sql, const QVariantList& params) {
    return exec(sql, params);
}

qint64 DbTx::lastInsertId() {
    return scalar("SELECT last_insert_rowid();").toLongLong();
}

int DbTx::resolvePartyId(int existingId, const QString& name,
                         const QString& defaultGroup, const QString& partyType) {
    if (existingId > 0) return existingId;
    const QString clean = name.trimmed();
    if (clean.isEmpty()) return 0;
    const QVariant lid = scalar(
        "SELECT id FROM parties WHERE name = ? COLLATE NOCASE "
        "OR replace(name, ' ', '') = replace(?, ' ', '') "
        "OR ? LIKE (name || '%') "
        "OR name LIKE (? || '%') "
        "LIMIT 1;",
        {clean, clean, clean, clean.left(40)});
    if (lid.isValid() && lid.toInt() > 0) return lid.toInt();
    if (!exec("INSERT INTO parties (name, group_name, party_type) VALUES (?, ?, ?);",
              {clean, defaultGroup, partyType})) {
        return 0;
    }
    return static_cast<int>(lastInsertId());
}

DbWorker::DbWorker(QObject* parent)
    : QObject(parent)
{
}

DbWorker::~DbWorker() {
    stop();
}

bool DbWorker::start(const QString& dbPath) {
    const QString clean = dbPath.trimmed();
    if (clean.isEmpty()) return false;
    {
        QMutexLocker locker(&m_stateMutex);
        if (m_thread && m_thread->isRunning() && m_dbPath == clean) return true;
    }
    stop(); // clean slate (no-op when never started)

    QThread* thread = new QThread();
    // The worker must be parent-less: Qt refuses moveToThread() on objects
    // with a parent (affinity silently stays put, and every later
    // BlockingQueuedConnection back to "itself" deadlocks the caller).
    if (parent() != nullptr) {
        qWarning() << "DbWorker had a parent; dropping it so the thread move works.";
        setParent(nullptr);
    }
    moveToThread(thread);

    // Blocking handshake: open runs ON the worker thread; start() returns
    // only after the connection is up (or failed). No racy spin-wait.
    auto opened = std::make_shared<std::promise<bool>>();
    std::future<bool> openedFuture = opened->get_future();
    connect(thread, &QThread::started, this, [this, clean, opened]() {
        opened->set_value(openOnThread(clean));
    }, Qt::DirectConnection);
    thread->start();
    // Bounded handshake: 10 s, then give up instead of hanging boot forever.
    // (Thread-start failure is near-impossible; this is best-effort cleanup.
    // Affinity may already sit on the dead thread, in which case the move
    // warns and later starts fall back to sync — never a hang.)
    if (openedFuture.wait_for(std::chrono::seconds(10)) != std::future_status::ready) {
        qWarning() << "DbWorker thread failed to come up; giving up.";
        thread->quit();
        thread->wait();
        moveToThread(QThread::currentThread());
        delete thread;
        return false;
    }
    const bool ok = openedFuture.get(); // runs before the event loop spins
    if (!ok) {
        // Move home from inside the object's own thread (see stop()).
        QThread* home = QThread::currentThread();
        QMetaObject::invokeMethod(this, [this, home]() { this->moveToThread(home); },
                                  Qt::BlockingQueuedConnection);
        thread->quit();
        thread->wait();
        delete thread;
        return false;
    }
    QMutexLocker locker(&m_stateMutex);
    m_thread = thread;
    m_dbPath = clean;
    return true;
}

bool DbWorker::restart(const QString& dbPath) {
    stop();
    return start(dbPath);
}

void DbWorker::stop() {
    QThread* thread = nullptr;
    {
        QMutexLocker locker(&m_stateMutex);
        thread = m_thread;
        m_thread = nullptr;
    }
    if (!thread) return;
    if (thread == QThread::currentThread()) {
        // Must never block-wait on ourselves; close and bail (misuse path).
        qWarning() << "DbWorker::stop() called from the worker thread; skipping join.";
        closeOnThread();
        thread->quit();
        return;
    }
    // Affinity home must change from inside the object's own thread:
    // moveToThread() from any other thread warns and silently no-ops,
    // leaving later queued calls stranded on a dead thread (hard hang).
    QThread* home = QThread::currentThread();
    QMetaObject::invokeMethod(this, [this, home]() {
        this->moveToThread(home);
        closeOnThread();
    }, Qt::BlockingQueuedConnection);
    thread->quit();
    thread->wait();
    delete thread;
    QMutexLocker locker(&m_stateMutex);
    m_dbPath.clear();
}

bool DbWorker::isRunning() const {
    QMutexLocker locker(&m_stateMutex);
    return m_thread && m_thread->isRunning() && m_db != nullptr;
}

QString DbWorker::dbPath() const {
    QMutexLocker locker(&m_stateMutex);
    return m_dbPath;
}

bool DbWorker::openOnThread(const QString& dbPath) {
    Q_ASSERT_X(QThread::currentThread() != QCoreApplication::instance()->thread(),
               "DbWorker::openOnThread", "must run on the worker thread");
    sqlite3* db = nullptr;
    if (sqlite3_open(dbPath.toUtf8().constData(), &db) != SQLITE_OK) {
        if (db) sqlite3_close(db);
        return false;
    }
    applyPragmas(db);
    m_db = db; // written before start() returns; read only on this thread after
    return true;
}

void DbWorker::closeOnThread() {
    if (m_db) {
        sqlite3_close(m_db);
        m_db = nullptr;
    }
}

QVariantList DbWorker::runQuery(const QString& sql, const QVariantList& params, QString* error) {
    if (!m_db) {
        // Racy post-stop task: refuse instead of touching a dead handle.
        if (error) *error = QString("database worker is stopped");
        return QVariantList();
    }
    DbTx tx(m_db);
    QVariantList rows = tx.query(sql, params);
    // Distinguish "zero rows" from failure: prepare/bind errors set lastError.
    if (rows.isEmpty() && !tx.lastError().isEmpty() && error) *error = tx.lastError();
    return rows;
}

QVariant DbWorker::runScalar(const QString& sql, const QVariantList& params, QString* error) {
    if (!m_db) {
        if (error) *error = QString("database worker is stopped");
        return QVariant();
    }
    DbTx tx(m_db);
    QVariantList rows = tx.query(sql, params);
    if (!tx.lastError().isEmpty() && error) *error = tx.lastError();
    if (rows.isEmpty()) return QVariant();
    const QVariantMap row = rows.first().toMap();
    return row.isEmpty() ? QVariant() : row.constBegin().value();
}

bool DbWorker::runExec(const QString& sql, const QVariantList& params, QString* error) {
    if (!m_db) {
        if (error) *error = QString("database worker is stopped");
        return false;
    }
    DbTx tx(m_db);
    if (!tx.exec(sql, params)) {
        if (error) *error = tx.lastError();
        return false;
    }
    return true;
}

template <typename T>
QFuture<T> DbWorker::postTask(std::function<T(QString*)> work) {
    auto promise = std::make_shared<QPromise<T>>();
    promise->start();
    QFuture<T> future = promise->future();
    if (!isRunning()) {
        promise->finish(); // down worker: resolve empty, never hang
        return future;
    }
    m_pending.fetch_add(1);
    QMetaObject::invokeMethod(this,
        [this, work = std::move(work), promise]() {
            QElapsedTimer timer;
            timer.start();
            QString error;
            T value = work(&error);
            m_lastElapsedMs.store(timer.elapsed());
            m_totalTasks.fetch_add(1);
            m_pending.fetch_sub(1);
            if (!error.isEmpty()) {
                // Error travels out-of-band: resolve empty so callers that
                // ignore errors keep old data instead of crashing.
                promise->finish();
            } else {
                promise->addResult(std::move(value));
                promise->finish();
            }
        },
        Qt::QueuedConnection);
    return future;
}

QFuture<QVariantList> DbWorker::asyncQuery(const QString& sql, const QVariantList& params) {
    return postTask<QVariantList>(
        [this, sql, params](QString* error) { return runQuery(sql, params, error); });
}
QFuture<QVariant> DbWorker::asyncScalar(const QString& sql, const QVariantList& params) {
    return postTask<QVariant>(
        [this, sql, params](QString* error) { return runScalar(sql, params, error); });
}

QFuture<bool> DbWorker::asyncExec(const QString& sql, const QVariantList& params) {
    return postTask<bool>(
        [this, sql, params](QString* error) { return runExec(sql, params, error); });
}

QFuture<bool> DbWorker::asyncTransact(std::function<bool(DbTx&)> fn) {
    auto promise = std::make_shared<QPromise<bool>>();
    promise->start();
    QFuture<bool> future = promise->future();
    if (!isRunning()) {
        promise->addResult(false);
        promise->finish();
        return future;
    }
    m_pending.fetch_add(1);
    QMetaObject::invokeMethod(this, [this, fn = std::move(fn), promise]() {
        QElapsedTimer timer;
        timer.start();
        bool committed = false;
        if (m_db) {
            char* errMsg = nullptr;
            if (sqlite3_exec(m_db, "BEGIN IMMEDIATE;", nullptr, nullptr, &errMsg) == SQLITE_OK) {
                try {
                    DbTx tx(m_db);
                    committed = fn(tx) && tx.lastError().isEmpty();
                } catch (...) {
                    committed = false;
                }
                sqlite3_exec(m_db, committed ? "COMMIT;" : "ROLLBACK;", nullptr, nullptr, nullptr);
            } else {
                sqlite3_free(errMsg);
            }
        }
        m_lastElapsedMs.store(timer.elapsed());
        m_totalTasks.fetch_add(1);
        m_pending.fetch_sub(1);
        promise->addResult(committed);
        promise->finish();
    }, Qt::QueuedConnection);
    return future;
}

} // namespace MahadevERP
