#include "gst_portal_sync_dialog.h"
#include "../database_manager.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QFileDialog>
#include <QMessageBox>
#include <QDir>
#include <QFile>

namespace MahadevERP {

GstPortalSyncDialog::GstPortalSyncDialog(const QDate& defaultDate, QWidget* parent)
    : QDialog(parent),
      m_currentDate(defaultDate.isValid() ? defaultDate : QDate::currentDate()),
      m_portalService(new GstPortalService(this))
{
    setWindowTitle("GST Portal GSTR-2B Auto-Download & Reconciler");
    setFixedWidth(560);
    setModal(true);

    setupUi();
}

void GstPortalSyncDialog::setupUi() {
    setStyleSheet(
        "QDialog { background-color: #F8FAFC; }"
        "QGroupBox { font-weight: 700; font-size: 13px; color: #1E293B; border: 1px solid #CBD5E1; border-radius: 8px; margin-top: 10px; padding-top: 14px; background-color: #FFFFFF; }"
        "QGroupBox::title { subcontrol-origin: margin; left: 12px; padding: 0 4px; }"
        "QLineEdit, QComboBox { padding: 8px 10px; border: 1px solid #CBD5E1; border-radius: 6px; font-size: 13px; background-color: #FFFFFF; color: #0F172A; }"
        "QLineEdit:focus, QComboBox:focus { border: 1px solid #2563EB; background-color: #EFF6FF; }"
        "QPushButton { padding: 8px 16px; font-size: 13px; font-weight: 700; border-radius: 6px; }"
    );

    auto* rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(20, 20, 20, 20);
    rootLayout->setSpacing(16);

    // 1. Header description
    auto* descLabel = new QLabel("Automatically connect to the GSTN Portal API, download GSTR-2B inward supplies, and run instant 5-way matching against your purchase ledger.", this);
    descLabel->setWordWrap(true);
    descLabel->setStyleSheet("color: #475569; font-size: 13px; font-weight: 500;");
    rootLayout->addWidget(descLabel);

    // 2. Credentials & Tax Period Group
    auto* credGroup = new QGroupBox("GST Portal API Credentials & Tax Period", this);
    auto* credForm = new QFormLayout(credGroup);
    credForm->setContentsMargins(16, 16, 16, 16);
    credForm->setSpacing(12);

    // GSTIN (Pre-fill dynamically from company_info or app_settings)
    QString defaultGstin = DatabaseManager::instance().getSetting("company_gstin", "");
    if (defaultGstin.isEmpty()) {
        QVariant gVal = DatabaseManager::instance().executeScalar("SELECT gstin FROM company_info LIMIT 1;");
        if (gVal.isValid() && !gVal.isNull()) defaultGstin = gVal.toString().trimmed();
    }
    m_gstinEdit = new QLineEdit(defaultGstin, credGroup);
    m_gstinEdit->setMaxLength(15);
    credForm->addRow("<b>Company GSTIN:</b>", m_gstinEdit);

    m_usernameEdit = new QLineEdit(defaultGstin, credGroup);
    credForm->addRow("<b>Portal Username:</b>", m_usernameEdit);

    // Tax Period (Month + Year)
    auto* periodLayout = new QHBoxLayout();
    m_monthCombo = new QComboBox(credGroup);
    QStringList months = { "01 - January", "02 - February", "03 - March", "04 - April", "05 - May", "06 - June",
                           "07 - July", "08 - August", "09 - September", "10 - October", "11 - November", "12 - December" };
    m_monthCombo->addItems(months);
    m_monthCombo->setCurrentIndex(m_currentDate.month() - 1);

    m_yearCombo = new QComboBox(credGroup);
    for (int y = 2024; y <= 2030; ++y) {
        m_yearCombo->addItem(QString::number(y));
    }
    m_yearCombo->setCurrentText(QString::number(m_currentDate.year()));

    periodLayout->addWidget(m_monthCombo, 2);
    periodLayout->addWidget(m_yearCombo, 1);
    credForm->addRow("<b>Return Period:</b>", periodLayout);

    rootLayout->addWidget(credGroup);

    // 3. OTP Authentication Group
    auto* otpGroup = new QGroupBox("2-Factor GSTN Authentication (OTP)", this);
    auto* otpLayout = new QVBoxLayout(otpGroup);
    otpLayout->setContentsMargins(16, 16, 16, 16);
    otpLayout->setSpacing(12);

    auto* otpRow = new QHBoxLayout();
    m_requestOtpBtn = new QPushButton("Request OTP (Alt+O)", otpGroup);
    m_requestOtpBtn->setStyleSheet("background-color: #2563EB; color: #FFFFFF; border: none;");
    m_requestOtpBtn->setShortcut(QKeySequence("Alt+O"));
    connect(m_requestOtpBtn, &QPushButton::clicked, this, &GstPortalSyncDialog::onRequestOtpClicked);

    m_otpEdit = new QLineEdit(otpGroup);
    m_otpEdit->setPlaceholderText("Enter 6-digit OTP");
    m_otpEdit->setMaxLength(6);

    otpRow->addWidget(m_requestOtpBtn, 1);
    otpRow->addWidget(m_otpEdit, 1);
    otpLayout->addLayout(otpRow);

    rootLayout->addWidget(otpGroup);

    // 4. Progress & Status Display
    m_progressBar = new QProgressBar(this);
    m_progressBar->setFixedHeight(8);
    m_progressBar->setTextVisible(false);
    m_progressBar->setRange(0, 0); // Indeterminate
    m_progressBar->setVisible(false);
    rootLayout->addWidget(m_progressBar);

    m_statusLabel = new QLabel("Ready to connect to GST Portal.", this);
    m_statusLabel->setStyleSheet("color: #64748B; font-size: 12px; font-weight: 600;");
    rootLayout->addWidget(m_statusLabel);

    // 5. Action Buttons
    auto* btnLayout = new QHBoxLayout();
    m_browseJsonBtn = new QPushButton("Import Local 2B JSON", this);
    m_browseJsonBtn->setStyleSheet("background-color: #F1F5F9; color: #334155; border: 1px solid #CBD5E1;");
    connect(m_browseJsonBtn, &QPushButton::clicked, this, &GstPortalSyncDialog::onBrowseLocalJsonClicked);

    btnLayout->addWidget(m_browseJsonBtn);
    btnLayout->addStretch(1);

    m_cancelBtn = new QPushButton("Cancel (Esc)", this);
    m_cancelBtn->setStyleSheet("background-color: #FFFFFF; color: #475569; border: 1px solid #CBD5E1;");
    connect(m_cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
    btnLayout->addWidget(m_cancelBtn);

    m_downloadMatchBtn = new QPushButton("Download & Auto-Match", this);
    m_downloadMatchBtn->setStyleSheet("background-color: #16A34A; color: #FFFFFF; font-size: 13px; font-weight: 800; border: none; padding: 10px 20px;");
    m_downloadMatchBtn->setDefault(true);
    connect(m_downloadMatchBtn, &QPushButton::clicked, this, &GstPortalSyncDialog::onDownloadAndMatchClicked);
    btnLayout->addWidget(m_downloadMatchBtn);

    rootLayout->addLayout(btnLayout);
}

QString GstPortalSyncDialog::getSelectedReturnPeriod() const {
    int month = m_monthCombo->currentIndex() + 1;
    QString monthStr = QString("%1").arg(month, 2, 10, QChar('0'));
    QString yearStr = m_yearCombo->currentText();
    return monthStr + yearStr; // e.g. "042026"
}

void GstPortalSyncDialog::setStatus(const QString& msg, bool isError) {
    m_statusLabel->setText(msg);
    m_statusLabel->setStyleSheet(isError ? "color: #DC2626; font-size: 12px; font-weight: 700;"
                                         : "color: #16A34A; font-size: 12px; font-weight: 600;");
}

void GstPortalSyncDialog::onRequestOtpClicked() {
    QString gstin = m_gstinEdit->text().trimmed().toUpper();
    QString username = m_usernameEdit->text().trimmed();
    if (gstin.length() != 15) {
        QMessageBox::warning(this, "Invalid GSTIN", "Please enter a valid 15-character GSTIN.");
        return;
    }
    if (username.isEmpty()) {
        username = gstin;
    }

    m_progressBar->setVisible(true);
    setStatus("Connecting to GST Portal & requesting OTP via GSP Gateway...");
    m_requestOtpBtn->setEnabled(false);

    m_portalService->requestOtp(gstin, username, [this](bool success, const QString& msg) {
        m_progressBar->setVisible(false);
        m_requestOtpBtn->setEnabled(true);
        if (success) {
            setStatus(msg, false);
            m_otpEdit->setFocus();
            QMessageBox::information(this, "OTP Sent", msg + "\nPlease enter the OTP to proceed.");
        } else {
            setStatus(msg, true);
            QMessageBox msgBox(this);
            msgBox.setIcon(QMessageBox::Information);
            msgBox.setWindowTitle("GST Portal Authentication");
            msgBox.setText(QString("<b>GST API Response:</b> %1<br><br>"
                                   "If your GST Portal session or API access is not active, you can also import your downloaded GSTR-2B JSON file directly.").arg(msg));
            msgBox.setInformativeText("Would you like to select your downloaded GSTR-2B JSON file now?");
            msgBox.setStandardButtons(QMessageBox::Open | QMessageBox::Cancel);
            msgBox.setDefaultButton(QMessageBox::Open);
            msgBox.button(QMessageBox::Open)->setText("Select 2B JSON File");
            
            if (msgBox.exec() == QMessageBox::Open) {
                onBrowseLocalJsonClicked();
            }
        }
    });
}

void GstPortalSyncDialog::onDownloadAndMatchClicked() {
    QString gstin = m_gstinEdit->text().trimmed().toUpper();
    QString username = m_usernameEdit->text().trimmed();
    QString otp = m_otpEdit->text().trimmed();
    QString retPeriod = getSelectedReturnPeriod();

    if (gstin.length() != 15) {
        QMessageBox::warning(this, "Invalid GSTIN", "Please enter a valid 15-character GSTIN.");
        return;
    }
    if (username.isEmpty()) {
        username = gstin;
    }

    m_progressBar->setVisible(true);
    m_downloadMatchBtn->setEnabled(false);
    setStatus("Connecting to GSTN and downloading GSTR-2B...");

    auto runDownload = [this, gstin, retPeriod]() {
        m_portalService->downloadGstr2B(gstin, retPeriod, [this, retPeriod](bool success, const QByteArray& payload, const QString& error) {
            m_progressBar->setVisible(false);
            m_downloadMatchBtn->setEnabled(true);

            QByteArray finalPayload = payload;
            if (!success || finalPayload.isEmpty()) {
                // If API endpoint returns error or is offline, check local cache
                QString cachedFile = QString("data/gstr2b/GSTR2B_%1.json").arg(retPeriod);
                QFile f(cachedFile);
                if (f.open(QIODevice::ReadOnly)) {
                    finalPayload = f.readAll();
                    f.close();
                } else {
                    setStatus(error, true);
                    QMessageBox::warning(this, "Download Failed", error.isEmpty() ? "No data returned from GST Portal." : error);
                    return;
                }
            } else {
                // Save locally to data/gstr2b/
                QDir().mkpath("data/gstr2b");
                QFile f(QString("data/gstr2b/GSTR2B_%1.json").arg(retPeriod));
                if (f.open(QIODevice::WriteOnly)) {
                    f.write(finalPayload);
                    f.close();
                }
            }

            // Parse Invoices & Reconcile
            QList<Gstr2PortalRecord> portal = Gstr2Reconciler::parseGstr2BJson(finalPayload, retPeriod);
            QList<Gstr2BookRecord> books = Gstr2Reconciler::loadBookPurchasesForPeriod(retPeriod);
            Gstr2ReconciliationSummary summary = Gstr2Reconciler::reconcile(books, portal, 1.0, 30);

            emit reconciliationCompleted(summary);
            accept();
        });
    };

    if (!otp.isEmpty() && m_authToken.isEmpty()) {
        m_portalService->authenticateWithOtp(gstin, username, otp, [this, runDownload](bool success, const QString& token, const QString& msg) {
            if (success) {
                m_authToken = token;
                runDownload();
            } else {
                m_progressBar->setVisible(false);
                m_downloadMatchBtn->setEnabled(true);
                setStatus(msg, true);
                QMessageBox::warning(this, "Authentication Failed", msg);
            }
        });
    } else {
        runDownload();
    }
}

void GstPortalSyncDialog::onBrowseLocalJsonClicked() {
    QString jsonPath = QFileDialog::getOpenFileName(this, "Select GSTR-2B JSON or Excel File", "gst-data", "GST Returns (*.json *.xlsx *.xls *.zip);;JSON Files (*.json);;Excel Files (*.xlsx *.xls);;All Files (*.*)");
    if (jsonPath.isEmpty()) return;

    QString retPeriod = getSelectedReturnPeriod();
    QList<Gstr2PortalRecord> portal = Gstr2Reconciler::loadPortalRecordsFromFile(jsonPath, retPeriod);
    if (portal.isEmpty()) {
        QMessageBox::warning(this, "File Error", "Could not parse valid inward supply records from selected file.");
        return;
    }

    QList<Gstr2BookRecord> books = Gstr2Reconciler::loadBookPurchasesForPeriod(retPeriod);
    Gstr2ReconciliationSummary summary = Gstr2Reconciler::reconcile(books, portal, 1.0, 30);

    emit reconciliationCompleted(summary);
    accept();
}

} // namespace MahadevERP
