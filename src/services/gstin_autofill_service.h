#pragma once

// GstinAutofillService — silent background GSTIN lookup.
//
// Attach to any GSTIN QLineEdit (New Company, New Ledger). When the user
// finishes a checksum-valid 15-char GSTIN, the service fetches the firm
// profile (cache-first, async, never blocks UI) and hands it to the caller,
// which fills its own empty fields. No dialogs, no buttons, no popups.
//
// Cost control: checksum-gated (typos never hit network), SQLite-backed
// response cache (a GSTIN is fetched once per database, ever).

#include <QObject>
#include <QLineEdit>
#include <QMap>
#include <QList>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QString>
#include <QVariantMap>
#include <functional>

namespace MahadevERP {

struct GstinProfile {
    bool valid = false;
    QString gstin;
    QString legalName;
    QString tradeName;
    QString status;          // Active / Cancelled / ...
    QString taxpayerType;    // Regular / Composition / ...
    QString stateCode;       // "06"
    QString address;
    QString city;
    QString pincode;
    QString registrationDate;
    QString blockStatus;
    QString error;           // set when valid == false
};

class GstinAutofillService : public QObject {
    Q_OBJECT

public:
    static GstinAutofillService& instance();

    using FillCallback = std::function<void(const GstinProfile&)>;

    // Watch a GSTIN field: on editingFinished with a valid GSTIN, fetch and
    // invoke onFilled (possibly immediately from cache).
    void watch(QLineEdit* gstinEdit, FillCallback onFilled);

    // One-shot fetch (cache-first, async). Callback always invoked exactly once.
    void fetch(const QString& gstin, FillCallback cb);

    // Offline checksum gate (no network on typos).
    static bool isValidGstin(const QString& gstin);

    // Key resolution: app_settings "gstin_lookup/api_key", else data/.gstin_api_key.
    static QString apiKey();

    // Parse a gstinapi.in response body into a profile (pure, testable).
    static GstinProfile parseResponse(const QByteArray& body, const QString& askedGstin);

private:
    explicit GstinAutofillService(QObject* parent = nullptr);

    void fetchFromNetwork(const QString& gstin);
    void finishPending(const QString& gstin, const GstinProfile& profile);
    static GstinProfile profileFromCache(const QString& gstin, bool& found);

    QNetworkAccessManager* m_net = nullptr;
    QMap<QString, QList<FillCallback>> m_pending;
};

} // namespace MahadevERP
