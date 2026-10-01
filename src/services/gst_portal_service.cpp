#include "gst_portal_service.h"
#include <QNetworkRequest>
#include <QJsonDocument>
#include <QJsonObject>
#include <QUrl>
#include <QUrlQuery>

namespace MahadevERP {

GstPortalService::GstPortalService(QObject* parent)
    : QObject(parent),
      m_netManager(new QNetworkAccessManager(this))
{
}

void GstPortalService::requestOtp(const QString& gstin, 
                                  std::function<void(bool success, const QString& msg)> callback) 
{
    QUrl url(m_baseUrl + "/authenticate");
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("clientid", "MAHADEVAC_CLIENT");
    request.setRawHeader("state-cd", gstin.left(2).toUtf8());

    QJsonObject body;
    body["action"] = "OTPREQUEST";
    body["username"] = gstin;

    QNetworkReply* reply = m_netManager->post(request, QJsonDocument(body).toJson(QJsonDocument::Compact));
    connect(reply, &QNetworkReply::finished, [reply, callback]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            callback(false, reply->errorString());
            return;
        }

        QByteArray data = reply->readAll();
        QJsonDocument doc = QJsonDocument::fromJson(data);
        if (doc.isObject() && doc.object().value("status_cd").toString() == "1") {
            callback(true, "OTP sent successfully to registered mobile/email.");
        } else {
            QString errMsg = doc.object().value("message").toString("Failed to request OTP from GST Portal.");
            callback(false, errMsg);
        }
    });
}

void GstPortalService::authenticateWithOtp(const QString& gstin, 
                                           const QString& otp, 
                                           std::function<void(bool success, const QString& token, const QString& msg)> callback) 
{
    QUrl url(m_baseUrl + "/authenticate");
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("clientid", "MAHADEVAC_CLIENT");
    request.setRawHeader("state-cd", gstin.left(2).toUtf8());

    QJsonObject body;
    body["action"] = "AUTHTOKEN";
    body["username"] = gstin;
    body["otp"] = otp;

    QNetworkReply* reply = m_netManager->post(request, QJsonDocument(body).toJson(QJsonDocument::Compact));
    connect(reply, &QNetworkReply::finished, [reply, callback]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            callback(false, "", reply->errorString());
            return;
        }

        QByteArray data = reply->readAll();
        QJsonDocument doc = QJsonDocument::fromJson(data);
        if (doc.isObject() && doc.object().value("status_cd").toString() == "1") {
            QString token = doc.object().value("auth_token").toString();
            callback(true, token, "Authentication successful.");
        } else {
            QString errMsg = doc.object().value("message").toString("Invalid OTP or expired session.");
            callback(false, "", errMsg);
        }
    });
}

void GstPortalService::downloadGstr2B(const QString& gstin, 
                                      const QString& returnPeriod, 
                                      const QString& authToken,
                                      std::function<void(bool success, const QByteArray& jsonPayload, const QString& error)> callback) 
{
    QUrl url(m_baseUrl + "/returns/gstr2b");
    QUrlQuery query;
    query.addQueryItem("gstin", gstin);
    query.addQueryItem("ret_period", returnPeriod);
    url.setQuery(query);

    QNetworkRequest request(url);
    request.setRawHeader("auth-token", authToken.toUtf8());
    request.setRawHeader("clientid", "MAHADEVAC_CLIENT");
    request.setRawHeader("state-cd", gstin.left(2).toUtf8());

    QNetworkReply* reply = m_netManager->get(request);
    connect(reply, &QNetworkReply::finished, [reply, callback]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            callback(false, QByteArray(), reply->errorString());
            return;
        }

        QByteArray payload = reply->readAll();
        callback(true, payload, "");
    });
}

} // namespace MahadevERP
