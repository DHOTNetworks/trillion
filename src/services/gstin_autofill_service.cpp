#include "gstin_autofill_service.h"
#include "../database_manager.h"
#include "../config/settings_registry.h"
#include "../engine/gstr1_engine.h"
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkRequest>
#include <QTimer>
#include <QUrl>

namespace MahadevERP {

GstinAutofillService& GstinAutofillService::instance() {
    static GstinAutofillService svc;
    return svc;
}

GstinAutofillService::GstinAutofillService(QObject* parent)
    : QObject(parent), m_net(new QNetworkAccessManager(this)) {}

bool GstinAutofillService::isValidGstin(const QString& gstin) {
    return Gstr1Engine::isValidGstin(gstin);
}

QString GstinAutofillService::apiKey() {
    QString k = DatabaseManager::instance().getSetting(SettingsKeys::gstin_api_key()).trimmed();
    if (!k.isEmpty()) return k;
    QFile f(QDir::current().filePath("data/.gstin_api_key"));
    if (f.open(QIODevice::ReadOnly)) {
        k = QString::fromUtf8(f.readAll()).trimmed();
        f.close();
    }
    return k;
}

void GstinAutofillService::watch(QLineEdit* gstinEdit, FillCallback onFilled) {
    if (!gstinEdit) return;
    connect(gstinEdit, &QLineEdit::editingFinished, this, [this, gstinEdit, onFilled]() {
        const QString g = gstinEdit->text().trimmed().toUpper();
        if (g.length() != 15) return; // incomplete; ignore silently
        fetch(g, onFilled);
    });
}

void GstinAutofillService::fetch(const QString& gstin, FillCallback cb) {
    const QString g = gstin.trimmed().toUpper();
    if (!isValidGstin(g)) {
        GstinProfile p;
        p.error = "Invalid GSTIN checksum.";
        // Stay async so callers never branch on timing.
        QTimer::singleShot(0, this, [cb, p]() { cb(p); });
        return;
    }
    bool found = false;
    GstinProfile cached = profileFromCache(g, found);
    if (found) {
        QTimer::singleShot(0, this, [cb, cached]() { cb(cached); });
        return;
    }
    if (apiKey().isEmpty()) {
        GstinProfile p;
        p.error = "GSTIN lookup key not configured.";
        QTimer::singleShot(0, this, [cb, p]() { cb(p); });
        return;
    }
    if (m_pending.contains(g)) {
        m_pending[g].append(cb);
        return;
    }
    m_pending[g].append(cb);
    fetchFromNetwork(g);
}

GstinProfile GstinAutofillService::profileFromCache(const QString& gstin, bool& found) {
    found = false;
    GstinProfile p;
    QVariant v = DatabaseManager::instance().executeScalar(
        "SELECT payload FROM gstin_lookup_cache WHERE gstin = ? LIMIT 1;", {gstin});
    if (!v.isValid() || v.isNull()) return p;
    p = parseResponse(v.toString().toUtf8(), gstin);
    found = p.valid;
    return p;
}

GstinProfile GstinAutofillService::parseResponse(const QByteArray& body, const QString& askedGstin) {
    GstinProfile p;
    p.gstin = askedGstin.trimmed().toUpper();
    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(body, &err);
    if (err.error != QJsonParseError::NoError || !doc.isObject()) {
        p.error = "Unparsable lookup response.";
        return p;
    }
    QJsonObject root = doc.object();
    if (!root.value("success").toBool(false)) {
        p.error = root.value("message").toString("GSTIN not found.");
        if (p.error.trimmed().isEmpty()) p.error = "GSTIN not found.";
        return p;
    }
    QJsonObject d = root.value("data").toObject();
    if (d.isEmpty()) {
        p.error = "Empty lookup result.";
        return p;
    }
    p.legalName = d.value("legal_name").toString().trimmed();
    p.tradeName = d.value("trade_name").toString().trimmed();
    p.status = d.value("status").toString().trimmed();
    p.taxpayerType = d.value("taxpayer_type").toString().trimmed();
    p.stateCode = d.value("state_code").toString().trimmed();
    p.address = d.value("address").toString().trimmed();
    p.city = d.value("city").toString().trimmed();
    p.pincode = d.value("pincode").toString().trimmed();
    p.registrationDate = d.value("registration_date").toString().trimmed();
    p.blockStatus = d.value("block_status").toString().trimmed();
    if (p.legalName.isEmpty() && p.tradeName.isEmpty()) {
        p.error = "Empty lookup result.";
        return p;
    }
    p.valid = true;
    return p;
}

void GstinAutofillService::fetchFromNetwork(const QString& gstin) {
    QUrl url("https://www.gstinapi.in/v1/gstin/" + gstin);
    QNetworkRequest req(url);
    req.setRawHeader("x-api-key", apiKey().toUtf8());
    QNetworkReply* reply = m_net->get(req);
    connect(reply, &QNetworkReply::finished, this, [this, reply, gstin]() {
        reply->deleteLater();
        GstinProfile p;
        if (reply->error() != QNetworkReply::NoError) {
            p.error = "Lookup failed: " + reply->errorString();
        } else {
            const QByteArray body = reply->readAll();
            p = parseResponse(body, gstin);
            if (p.valid) {
                DatabaseManager::instance().executeNonQuery(
                    "INSERT OR REPLACE INTO gstin_lookup_cache (gstin, payload, fetched_at) "
                    "VALUES (?, ?, datetime('now'));",
                    {gstin, QString::fromUtf8(body)});
            }
        }
        finishPending(gstin, p);
    });
}

void GstinAutofillService::finishPending(const QString& gstin, const GstinProfile& profile) {
    const QList<FillCallback> cbs = m_pending.take(gstin);
    for (const auto& cb : cbs) cb(profile);
}

} // namespace MahadevERP
