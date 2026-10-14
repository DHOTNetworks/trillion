#pragma once

#include <QObject>
#include <QVariantList>
#include <QVariantMap>
#include <QString>
#include <QFuture>

namespace MahadevERP { class DbTx; }

class TdsModel : public QObject {
    Q_OBJECT

public:
    explicit TdsModel(QObject* parent = nullptr);
    ~TdsModel() override = default;

    Q_INVOKABLE QVariantMap get_next_voucher_info(const QString& tdsType = "RENT", const QString& fy = "");
    Q_INVOKABLE QVariantMap get_party_info(int partyId, const QString& tdsType = "RENT");
    Q_INVOKABLE QString get_last_narration(int partyId, const QString& tdsType = "");
    Q_INVOKABLE bool save_tds_voucher(const QVariantMap& data);
    // Connection-agnostic cores: identical SQL on the UI thread (sync
    // wrapper, legacy callers/tests) and on the DB worker (async). No
    // DatabaseManager, no hidden-DB helpers, no signals, no BEGIN/COMMIT —
    // the caller owns the transaction.
    static bool save_tds_voucher_in(MahadevERP::DbTx& tx, const QVariantMap& data, int& outVoucherNo);
    static bool delete_tds_voucher_in(MahadevERP::DbTx& tx, int voucherId);
    // Worker-thread entry points for the UI. Never wait on them from the GUI;
    // use a QFutureWatcher. Empty resolution == refused/failed, keep old data.
    static QFuture<bool> save_tds_voucher_async(const QVariantMap& data);
    static QFuture<bool> delete_tds_voucher_async(int voucherId);
    Q_INVOKABLE QVariantMap get_tds_voucher(int voucherId);
    Q_INVOKABLE QVariantMap get_tds_voucher_by_id(int voucherId);
    Q_INVOKABLE QVariantMap get_tds_voucher_by_ref(int voucherNo, const QString& voucherDate = "", const QString& financialYear = "");
    Q_INVOKABLE QVariantMap get_tds_voucher_for_party(int partyId, int voucherNo = 0, const QString& voucherDate = "");
    Q_INVOKABLE bool is_tds_voucher(int voucherNo, const QString& voucherDate = "", const QString& partyName = "");
    Q_INVOKABLE QVariantList get_tds_register(const QString& fromDate = "", const QString& toDate = "");
    // Shared register SQL (single owner) + worker-thread reader for lists.
    static void registerSql(const QString& fromDate, const QString& toDate,
                            QString& sql, QVariantList& args);
    static QFuture<QVariantList> get_tds_register_async(const QString& fromDate = "",
                                                        const QString& toDate = "");
    Q_INVOKABLE bool delete_tds_voucher(int voucherId);

signals:
    void voucherSaved();
    void voucherDeleted();
};

