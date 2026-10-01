#pragma once

#include <QDialog>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QComboBox>
#include <QProgressBar>
#include "../engine/gstr2_reconciler.h"
#include "../services/gst_portal_service.h"

namespace MahadevERP {

class GstPortalSyncDialog : public QDialog {
    Q_OBJECT

public:
    explicit GstPortalSyncDialog(const QDate& defaultDate, QWidget* parent = nullptr);
    ~GstPortalSyncDialog() override = default;

signals:
    void reconciliationCompleted(const Gstr2ReconciliationSummary& summary);

private slots:
    void onRequestOtpClicked();
    void onDownloadAndMatchClicked();
    void onBrowseLocalJsonClicked();

private:
    void setupUi();
    void setStatus(const QString& msg, bool isError = false);
    QString getSelectedReturnPeriod() const;

    QDate m_currentDate;
    GstPortalService* m_portalService = nullptr;

    QLineEdit* m_gstinEdit = nullptr;
    QLineEdit* m_usernameEdit = nullptr;
    QComboBox* m_monthCombo = nullptr;
    QComboBox* m_yearCombo = nullptr;
    QPushButton* m_requestOtpBtn = nullptr;
    QLineEdit* m_otpEdit = nullptr;
    QPushButton* m_downloadMatchBtn = nullptr;
    QPushButton* m_browseJsonBtn = nullptr;
    QPushButton* m_cancelBtn = nullptr;

    QLabel* m_statusLabel = nullptr;
    QProgressBar* m_progressBar = nullptr;
    QString m_authToken;
};

} // namespace MahadevERP
