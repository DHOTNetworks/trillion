#pragma once

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QString>
#include <QByteArray>
#include <functional>

namespace MahadevERP {

class GstPortalService : public QObject {
    Q_OBJECT

public:
    explicit GstPortalService(QObject* parent = nullptr);
    ~GstPortalService() override = default;

    // 1. Request OTP for Direct GSTN Auth
    void requestOtp(const QString& gstin, 
                    std::function<void(bool success, const QString& msg)> callback);

    // 2. Submit OTP and obtain Session Token
    void authenticateWithOtp(const QString& gstin, 
                             const QString& otp, 
                             std::function<void(bool success, const QString& token, const QString& msg)> callback);

    // 3. Download GSTR-2B JSON for Return Period (e.g., '042026')
    void downloadGstr2B(const QString& gstin, 
                        const QString& returnPeriod, 
                        const QString& authToken,
                        std::function<void(bool success, const QByteArray& jsonPayload, const QString& error)> callback);

private:
    QNetworkAccessManager* m_netManager = nullptr;
    QString m_baseUrl = "https://api.gst.gov.in/taxpayerapi/v1.0";
};

} // namespace MahadevERP
