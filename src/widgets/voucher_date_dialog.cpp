#include "voucher_date_dialog.h"
#include <QGraphicsDropShadowEffect>
#include <QDate>

VoucherDateDialog::VoucherDateDialog(const QString& currentDate, QWidget* parent)
    : QDialog(parent)
    , m_initialDate(currentDate.trimmed())
{
    setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
    setAttribute(Qt::WA_TranslucentBackground, false);
    setAttribute(Qt::WA_StyledBackground, true);

    QString rawDate = !m_initialDate.isEmpty() ? m_initialDate : m_fyModel.get_working_date();
    QDate parsedDate;
    if (!rawDate.isEmpty()) {
        parsedDate = QDate::fromString(rawDate, "dd-MM-yyyy");
        if (!parsedDate.isValid()) {
            parsedDate = QDate::fromString(rawDate, "yyyy-MM-dd");
        }
        if (!parsedDate.isValid()) {
            parsedDate = QDate::fromString(rawDate, "dd/MM/yyyy");
        }
        if (!parsedDate.isValid()) {
            parsedDate = QDate::fromString(rawDate, "yyyy/MM/dd");
        }
    }
    if (!parsedDate.isValid()) {
        parsedDate = QDate::currentDate();
    }
    m_workingDate = parsedDate.toString("dd-MM-yyyy");

    setupUi();
}

void VoucherDateDialog::setupUi() {
    setFixedWidth(360);
    setFixedHeight(210);

    setStyleSheet(
        "QDialog {"
        "  background-color: #E0F7FA;"
        "  border: 2px solid #0284C7;"
        "  border-radius: 0px;"
        "}"
        "QLabel {"
        "  color: #0369A1;"
        "  background: transparent;"
        "  font-family: 'Segoe UI', -apple-system, sans-serif;"
        "}"
        "QLineEdit {"
        "  background-color: #FFFFFF;"
        "  color: #0F172A;"
        "  border: 2px solid #0284C7;"
        "  border-radius: 0px;"
        "  font-size: 16px;"
        "  font-weight: 800;"
        "  padding: 4px 8px;"
        "}"
        "QLineEdit:focus {"
        "  border: 2px solid #EAB308;"
        "  background-color: #FEFCE8;"
        "}"
        "QPushButton {"
        "  font-size: 11px;"
        "  font-weight: 800;"
        "  padding: 5px 12px;"
        "  border-radius: 0px;"
        "}"
    );

    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(18, 16, 18, 14);
    mainLayout->setSpacing(8);

    // Title label: (Current Voucher Date) or (Current Working Date)
    QString titleText = !m_initialDate.isEmpty() ? "(Current Voucher Date)" : "(Current Working Date)";
    QLabel* topHint = new QLabel(titleText, this);
    topHint->setAlignment(Qt::AlignCenter);
    topHint->setStyleSheet("font-size: 12px; font-weight: 700; color: #0369A1;");
    mainLayout->addWidget(topHint);

    // Working Date display
    m_currentDateLabel = new QLabel(m_workingDate, this);
    m_currentDateLabel->setAlignment(Qt::AlignCenter);
    m_currentDateLabel->setStyleSheet("font-size: 16px; font-weight: 900; color: #082F49; letter-spacing: 0.5px;");
    mainLayout->addWidget(m_currentDateLabel);

    // New Voucher Date label
    QLabel* newDateLbl = new QLabel("New Voucher Date", this);
    newDateLbl->setAlignment(Qt::AlignCenter);
    newDateLbl->setStyleSheet("font-size: 13px; font-weight: 800; color: #0369A1; text-decoration: underline; margin-top: 2px;");
    mainLayout->addWidget(newDateLbl);

    // Date Input
    m_dateInput = new QLineEdit(this);
    m_dateInput->setAlignment(Qt::AlignCenter);
    m_dateInput->setText(m_workingDate);
    m_dateInput->setFixedHeight(36);
    mainLayout->addWidget(m_dateInput);

    // Error message label
    m_errorLabel = new QLabel("", this);
    m_errorLabel->setAlignment(Qt::AlignCenter);
    m_errorLabel->setStyleSheet("font-size: 11px; font-weight: 700; color: #DC2626;");
    m_errorLabel->setVisible(false);
    mainLayout->addWidget(m_errorLabel);

    // Bottom action / hint
    QHBoxLayout* btnLayout = new QHBoxLayout();
    btnLayout->setSpacing(8);

    m_applyBtn = new QPushButton("Apply (Enter)", this);
    m_applyBtn->setStyleSheet("QPushButton { background-color: #0284C7; color: #FFFFFF; border: 1px solid #0369A1; } QPushButton:hover { background-color: #0369A1; }");
    connect(m_applyBtn, &QPushButton::clicked, this, &VoucherDateDialog::onAccept);
    btnLayout->addWidget(m_applyBtn);

    m_cancelBtn = new QPushButton("Cancel (Esc)", this);
    m_cancelBtn->setStyleSheet("QPushButton { background-color: #FFFFFF; color: #475569; border: 1px solid #CBD5E1; } QPushButton:hover { background-color: #F1F5F9; }");
    connect(m_cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
    btnLayout->addWidget(m_cancelBtn);

    mainLayout->addLayout(btnLayout);

    connect(m_dateInput, &QLineEdit::returnPressed, this, &VoucherDateDialog::onAccept);
}

void VoucherDateDialog::showEvent(QShowEvent* event) {
    QDialog::showEvent(event);
    if (m_dateInput) {
        m_dateInput->setFocus();
        m_dateInput->selectAll();
    }
}

void VoucherDateDialog::keyPressEvent(QKeyEvent* event) {
    if (event->key() == Qt::Key_Escape) {
        reject();
        event->accept();
        return;
    }
    QDialog::keyPressEvent(event);
}

void VoucherDateDialog::onAccept() {
    QString val = m_dateInput ? m_dateInput->text().trimmed() : "";
    if (val.isEmpty()) {
        m_errorLabel->setText("Please enter a valid date.");
        m_errorLabel->setVisible(true);
        if (m_dateInput) m_dateInput->setFocus();
        return;
    }

    QVariantMap res = m_fyModel.validate_voucher_date(val, m_workingDate);
    bool valid = res.value("valid").toBool();
    if (!valid) {
        QString err = res.value("error").toString();
        if (err.isEmpty()) err = "Date is outside active Financial Year.";
        m_errorLabel->setText(err);
        m_errorLabel->setVisible(true);
        if (m_dateInput) {
            m_dateInput->setFocus();
            m_dateInput->selectAll();
        }
        return;
    }

    m_formattedDate = res.value("formattedDate").toString();
    m_isoDate = res.value("isoDate").toString();

    m_fyModel.set_working_date(m_formattedDate);
    accept();
}

bool VoucherDateDialog::getVoucherDate(QWidget* parent,
                                      const QString& currentDate,
                                      QString* outDisplayDate,
                                      QString* outIsoDate)
{
    VoucherDateDialog dlg(currentDate, parent);
    if (dlg.exec() == QDialog::Accepted) {
        if (outDisplayDate) {
            *outDisplayDate = dlg.formattedDate();
        }
        if (outIsoDate) {
            *outIsoDate = dlg.isoDate();
        }
        return true;
    }
    return false;
}
