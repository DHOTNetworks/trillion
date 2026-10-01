#include "gst_portal_service.h"
#include "../database_manager.h"
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
    m_accessToken = DatabaseManager::instance().getSetting("gsp_access_token", "ef6599bdb1b1d091c56a5fbef6fab0af4b38f692");
}

void GstPortalService::ensureAccessToken(std::function<void(bool success, const QString& token, const QString& error)> callback)
{
    QUrl url(m_gspAuthUrl);
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QJsonObject body;
    body["username"] = "anilkamra2007@gmail.com";
    body["password"] = "Anil@123";
    body["client_id"] = "GHfUTjxGRuofVyqxNa";
    body["client_secret"] = "yA24uD9KtiHnvLm0BtxBAXFT";
    body["grant_type"] = "password";

    QNetworkReply* reply = m_netManager->post(request, QJsonDocument(body).toJson(QJsonDocument::Compact));
    connect(reply, &QNetworkReply::finished, [this, reply, callback]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            // Fallback to cached default token
            callback(true, m_accessToken, "");
            return;
        }

        QByteArray data = reply->readAll();
        QJsonDocument doc = QJsonDocument::fromJson(data);
        if (doc.isObject() && doc.object().contains("access_token")) {
            m_accessToken = doc.object().value("access_token").toString().trimmed();
            DatabaseManager::instance().setSetting("gsp_access_token", m_accessToken);
            callback(true, m_accessToken, "");
        } else {
            callback(true, m_accessToken, "");
        }
    });
}

void GstPortalService::checkGstinRegistration(const QString& gstin,
                                              const QString& username,
                                              std::function<void(bool success, const QString& msg)> callback)
{
    ensureAccessToken([this, gstin, username, callback](bool, const QString& token, const QString&) {
        QUrl url(m_gspCheckGstinUrl);
        QNetworkRequest request(url);
        request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

        QJsonObject body;
        body["access_token"] = token;
        body["gstin_number"] = gstin;
        body["gst_userName"] = username;

        QNetworkReply* reply = m_netManager->post(request, QJsonDocument(body).toJson(QJsonDocument::Compact));
        connect(reply, &QNetworkReply::finished, [reply, callback]() {
            reply->deleteLater();
            if (reply->error() != QNetworkReply::NoError) {
                callback(false, reply->errorString());
                return;
            }

            QByteArray data = reply->readAll();
            QJsonDocument doc = QJsonDocument::fromJson(data);
            if (doc.isObject()) {
                QString status = doc.object().value("status").toString();
                if (status.compare("Success", Qt::CaseInsensitive) == 0 || status == "1") {
                    callback(true, "GSTIN verified successfully.");
                    return;
                }
            }
            callback(false, "GSTIN registration check failed.");
        });
    });
}

void GstPortalService::requestOtp(const QString& gstin, 
                                  const QString& username,
                                  std::function<void(bool success, const QString& msg)> callback) 
{
    ensureAccessToken([this, gstin, username, callback](bool, const QString& token, const QString&) {
        QUrl url(m_gstApiAuthUrl);
        QNetworkRequest request(url);
        request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
        request.setRawHeader("Authorization", QString("Bearer %1").arg(token).toUtf8());

        QJsonObject body;
        body["action"] = "OTPREQUEST";
        body["gstin"] = gstin;
        body["gst_username"] = username;

        QNetworkReply* reply = m_netManager->post(request, QJsonDocument(body).toJson(QJsonDocument::Compact));
        connect(reply, &QNetworkReply::finished, [reply, callback]() {
            reply->deleteLater();
            if (reply->error() != QNetworkReply::NoError) {
                callback(false, reply->errorString());
                return;
            }

            QByteArray data = reply->readAll();
            QJsonDocument doc = QJsonDocument::fromJson(data);
            if (doc.isObject()) {
                QJsonObject obj = doc.object();
                QString status = obj.value("status").toVariant().toString();
                QString msg = obj.value("message").toString("OTP requested.");
                if (status == "1" || status.compare("Success", Qt::CaseInsensitive) == 0) {
                    callback(true, msg);
                    return;
                }
                callback(false, msg);
            } else {
                callback(false, "Unexpected response from GST API Gateway.");
            }
        });
    });
}

void GstPortalService::authenticateWithOtp(const QString& gstin, 
                                           const QString& username,
                                           const QString& otp, 
                                           std::function<void(bool success, const QString& token, const QString& msg)> callback) 
{
    ensureAccessToken([this, gstin, username, otp, callback](bool, const QString& token, const QString&) {
        QUrl url(m_proAuthUrl);
        QNetworkRequest request(url);
        request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
        request.setRawHeader("Authorization", QString("Bearer %1").arg(token).toUtf8());

        QJsonObject body;
        body["action"] = "OTPVERIFY";
        body["gstin"] = gstin;
        body["gst_username"] = username;
        body["otp"] = otp;
        body["auth_extension"] = "1";

        QNetworkReply* reply = m_netManager->post(request, QJsonDocument(body).toJson(QJsonDocument::Compact));
        connect(reply, &QNetworkReply::finished, [reply, callback]() {
            reply->deleteLater();
            if (reply->error() != QNetworkReply::NoError) {
                callback(false, "", reply->errorString());
                return;
            }

            QByteArray data = reply->readAll();
            QJsonDocument doc = QJsonDocument::fromJson(data);
            if (doc.isObject()) {
                QJsonObject obj = doc.object();
                QString status = obj.value("status").toVariant().toString();
                QString msg = obj.value("message").toString("OTP verified.");
                if (status == "1" || status.compare("Success", Qt::CaseInsensitive) == 0) {
                    callback(true, "SESSION_ACTIVE", msg);
                    return;
                }
                callback(false, "", msg);
            } else {
                callback(false, "", "Unexpected response from GST Gateway.");
            }
        });
    });
}

void GstPortalService::downloadGstr2B(const QString& gstin, 
                                      const QString& returnPeriod, 
                                      std::function<void(bool success, const QByteArray& jsonPayload, const QString& error)> callback) 
{
    ensureAccessToken([this, gstin, returnPeriod, callback](bool, const QString& token, const QString&) {
        QString urlStr = QString("%1&gstin=%2&ret_period=%3")
                             .arg(m_gstr2bUrl, gstin, returnPeriod);
        QUrl url(urlStr);
        QNetworkRequest request(url);
        request.setRawHeader("Authorization", QString("Bearer %1").arg(token).toUtf8());

        QNetworkReply* reply = m_netManager->get(request);
        connect(reply, &QNetworkReply::finished, [reply, callback]() {
            reply->deleteLater();
            if (reply->error() != QNetworkReply::NoError) {
                callback(false, QByteArray(), reply->errorString());
                return;
            }

            QByteArray rawResponse = reply->readAll();
            
            // Response format: { "status": "1", "data": "<base64_encoded_json>" }
            QJsonDocument doc = QJsonDocument::fromJson(rawResponse);
            if (doc.isObject()) {
                QJsonObject rootObj = doc.object();
                QString status = rootObj.value("status").toVariant().toString();
                if (status == "1") {
                    QString base64Data = rootObj.value("data").toString().trimmed();
                    if (!base64Data.isEmpty()) {
                        QByteArray decodedJson = QByteArray::fromBase64(base64Data.toUtf8());
                        callback(true, decodedJson, "");
                        return;
                    }
                }
                QString msg = rootObj.value("message").toString("Failed to retrieve return data.");
                callback(false, QByteArray(), msg);
            } else {
                // If the response is directly the raw GSTR-2B JSON (e.g. from local proxy)
                if (rawResponse.contains("b2b") || rawResponse.contains("data")) {
                    callback(true, rawResponse, "");
                } else {
                    callback(false, QByteArray(), "Invalid JSON format received from server.");
                }
            }
        });
    });
}

} // namespace MahadevERP
