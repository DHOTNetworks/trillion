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

    // Ensure active Bearer Access Token (using OAuth or cached fallback)
    void ensureAccessToken(std::function<void(bool success, const QString& token, const QString& error)> callback);

    // Check / Register GSTIN with GSP Gateway
    void checkGstinRegistration(const QString& gstin,
                                const QString& username,
                                std::function<void(bool success, const QString& msg)> callback);

    // 1. Request OTP for Direct GSTN Auth
    void requestOtp(const QString& gstin, 
                    const QString& username,
                    std::function<void(bool success, const QString& msg)> callback);

    // 2. Submit OTP and obtain/verify Session Token
    void authenticateWithOtp(const QString& gstin, 
                             const QString& username,
                             const QString& otp, 
                             std::function<void(bool success, const QString& token, const QString& msg)> callback);

    // 3. Download GSTR-2B JSON for Return Period (e.g., '042026')
    void downloadGstr2B(const QString& gstin, 
                        const QString& returnPeriod, 
                        std::function<void(bool success, const QByteArray& jsonPayload, const QString& error)> callback);

    QString currentAccessToken() const { return m_accessToken; }
    void setAccessToken(const QString& token) { m_accessToken = token; }

private:
    QNetworkAccessManager* m_netManager = nullptr;
    QString m_accessToken = "ef6599bdb1b1d091c56a5fbef6fab0af4b38f692";
    QString m_gspAuthUrl = "https://pro.mastersindia.co/oauth/access_token";
    QString m_gspCheckGstinUrl = "https://pro.mastersindia.co/bussiness/checkGstin";
    QString m_gstApiAuthUrl = "https://gstapi.in/taxpayerapis/authenticate";
    QString m_proAuthUrl = "https://pro.mastersindia.co/taxpayerapis/authenticate";
    QString m_gstr2bUrl = "https://gstapi.in/taxpayerapis/returns/gstr2b?action=GET2B";
};

} // namespace MahadevERP

