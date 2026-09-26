#include "voucher_date_dialog.h"
#include <QGraphicsDropShadowEffect>
#include <QDate>
#include <QApplication>

VoucherDateDialog::VoucherDateDialog(const QString& currentDate, const FiscalYearInfo& fyContext, QWidget* parent)
    : QDialog(parent)
    , m_contextFy(fyContext)
    , m_initialDate(currentDate.trimmed())
{
    setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
    setWindowModality(Qt::ApplicationModal);
    setModal(true);
    setAttribute(Qt::WA_TranslucentBackground, false);
    setAttribute(Qt::WA_StyledBackground, true);

    if (!m_contextFy.isValid()) {
        if (!m_initialDate.isEmpty()) {
            m_contextFy = FiscalYearHelper::getFiscalYearForDate(m_initialDate);
        }
        if (!m_contextFy.isValid()) {
            m_contextFy = FiscalYearHelper::getActiveFiscalYear();
        }
    }

    QDate parsedDate;
    if (!m_initialDate.isEmpty()) {
        parsedDate = QDate::fromString(m_initialDate, "dd-MM-yyyy");
        if (!parsedDate.isValid()) parsedDate = QDate::fromString(m_initialDate, "yyyy-MM-dd");
        if (!parsedDate.isValid()) parsedDate = QDate::fromString(m_initialDate, "dd/MM/yyyy");
        if (!parsedDate.isValid()) parsedDate = QDate::fromString(m_initialDate, "yyyy/MM/dd");
    }
    if (!parsedDate.isValid()) {
        QString wDate = m_fyModel.get_working_date();
        parsedDate = QDate::fromString(wDate, "dd-MM-yyyy");
        if (!parsedDate.isValid()) parsedDate = QDate::fromString(wDate, "yyyy-MM-dd");
    }
    if (!parsedDate.isValid()) {
        parsedDate = QDate::fromString(m_contextFy.startDate, "yyyy-MM-dd");
    }
    if (!parsedDate.isValid()) {
        parsedDate = QDate::currentDate();
    }
    m_workingDate = parsedDate.toString("dd-MM-yyyy");

    setupUi();
}

void VoucherDateDialog::setupUi() {
    setFixedWidth(380);

    setStyleSheet(
        "QDialog {"
        "  background-color: #F0FDF4;"
        "  border: 2px solid #059669;"
        "  border-radius: 8px;"
        "}"
        "QLabel {"
        "  color: #065F46;"
        "  background: transparent;"
        "  font-family: 'Segoe UI', -apple-system, BlinkMacSystemFont, Roboto, 'Helvetica Neue', 'Noto Sans', 'Liberation Sans', Arial, sans-serif;"
        "}"
        "QLineEdit {"
        "  background-color: #FFFFFF;"
        "  color: #0F172A;"
        "  border: 2px solid #059669;"
        "  border-radius: 4px;"
        "  font-size: 16px;"
        "  font-weight: 800;"
        "  padding: 5px 8px;"
        "}"
        "QLineEdit:focus {"
        "  border: 2px solid #EAB308;"
        "  background-color: #FEFCE8;"
        "}"
        "QPushButton {"
        "  font-size: 11px;"
        "  font-weight: 800;"
        "  padding: 6px 14px;"
        "  border-radius: 4px;"
        "}"
    );

    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(20, 18, 20, 18);
    mainLayout->setSpacing(8);

    // FY Context Header
    QString fyTitle = m_contextFy.isValid() ? QString("%1 (%2 to %3)").arg(m_contextFy.name, FiscalYearHelper::formatDisplayDate(m_contextFy.startDate), FiscalYearHelper::formatDisplayDate(m_contextFy.endDate)) : "Active Fiscal Year";
    m_fyInfoLabel = new QLabel(fyTitle, this);
    m_fyInfoLabel->setAlignment(Qt::AlignCenter);
    m_fyInfoLabel->setStyleSheet("font-size: 12px; font-weight: 800; color: #047857; padding-bottom: 2px;");
    mainLayout->addWidget(m_fyInfoLabel);

    // Working Date label
    m_currentDateLabel = new QLabel(QString("Current Date: %1").arg(m_workingDate), this);
    m_currentDateLabel->setAlignment(Qt::AlignCenter);
    m_currentDateLabel->setStyleSheet("font-size: 14px; font-weight: 900; color: #064E3B;");
    mainLayout->addWidget(m_currentDateLabel);

    // Prompt label
    QLabel* newDateLbl = new QLabel("Enter Voucher Date", this);
    newDateLbl->setAlignment(Qt::AlignCenter);
    newDateLbl->setStyleSheet("font-size: 13px; font-weight: 800; color: #059669; text-decoration: underline; margin-top: 4px;");
    mainLayout->addWidget(newDateLbl);

    // Date Input
    m_dateInput = new QLineEdit(this);
    m_dateInput->setAlignment(Qt::AlignCenter);
    m_dateInput->setText(m_workingDate);
    m_dateInput->setFixedHeight(36);
    m_dateInput->installEventFilter(this);
    mainLayout->addWidget(m_dateInput);

    // Error message label
    m_errorLabel = new QLabel("", this);
    m_errorLabel->setAlignment(Qt::AlignCenter);
    m_errorLabel->setStyleSheet("font-size: 11px; font-weight: 700; color: #DC2626; padding: 2px;");
    m_errorLabel->setWordWrap(true);
    m_errorLabel->setVisible(false);
    mainLayout->addWidget(m_errorLabel);

    // Bottom action
    QHBoxLayout* btnLayout = new QHBoxLayout();
    btnLayout->setSpacing(8);

    m_applyBtn = new QPushButton("Apply (Enter)", this);
    m_applyBtn->setStyleSheet(
        "QPushButton { background-color: #059669; color: #FFFFFF; border: 1px solid #047857; font-weight: 800; } "
        "QPushButton:hover { background-color: #047857; } "
        "QPushButton:pressed { background-color: #064E3B; }"
    );
    connect(m_applyBtn, &QPushButton::clicked, this, &VoucherDateDialog::onAccept);
    btnLayout->addWidget(m_applyBtn);

    m_cancelBtn = new QPushButton("Cancel (Esc)", this);
    m_cancelBtn->setStyleSheet("QPushButton { background-color: #FFFFFF; color: #475569; border: 1px solid #CBD5E1; } QPushButton:hover { background-color: #F1F5F9; }");
    connect(m_cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
    btnLayout->addWidget(m_cancelBtn);

    mainLayout->addLayout(btnLayout);

    connect(m_dateInput, &QLineEdit::returnPressed, this, &VoucherDateDialog::onAccept);
    adjustSize();
}

void VoucherDateDialog::showEvent(QShowEvent* event) {
    QDialog::showEvent(event);
    activateWindow();
    raise();
    if (m_dateInput) {
        m_dateInput->setFocus(Qt::OtherFocusReason);
        m_dateInput->selectAll();
    }
}

bool VoucherDateDialog::eventFilter(QObject* watched, QEvent* event) {
    if (watched == m_dateInput && event->type() == QEvent::KeyPress) {
        auto* kEvent = static_cast<QKeyEvent*>(event);
        if (kEvent->key() == Qt::Key_Return || kEvent->key() == Qt::Key_Enter) {
            onAccept();
            return true;
        } else if (kEvent->key() == Qt::Key_Escape) {
            reject();
            return true;
        }
    }
    return QDialog::eventFilter(watched, event);
}

void VoucherDateDialog::keyPressEvent(QKeyEvent* event) {
    if (event->key() == Qt::Key_Escape) {
        reject();
        event->accept();
        return;
    }
    if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
        onAccept();
        event->accept();
        return;
    }
    QDialog::keyPressEvent(event);
}

void VoucherDateDialog::onAccept() {
    QString val = m_dateInput ? m_dateInput->text().trimmed() : "";
    if (val.isEmpty()) {
        val = m_workingDate;
    }

    QVariantMap res = m_fyModel.validate_voucher_date(val, m_workingDate);
    bool valid = res.value("valid").toBool();
    if (!valid) {
        QString err = res.value("error").toString();
        if (err.isEmpty()) err = "Date is outside active Financial Year.";
        m_errorLabel->setText(err);
        m_errorLabel->setVisible(true);
        adjustSize();
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
                                      QString* outIsoDate,
                                      const FiscalYearInfo& fyContext)
{
    // Complete dissociation: ensure no background widget holds focus
    if (QApplication::focusWidget()) {
        QApplication::focusWidget()->clearFocus();
    }
    if (parent) {
        parent->clearFocus();
    }

    QWidget* topParent = parent ? parent->window() : nullptr;
    VoucherDateDialog dlg(currentDate, fyContext, topParent);
    if (topParent) {
        dlg.move(topParent->geometry().center() - dlg.rect().center());
    }
    int code = dlg.exec();
    if (topParent) {
        topParent->activateWindow();
    }
    if (code == QDialog::Accepted) {
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

QDate VoucherDateDialog::selectDate(QWidget* parent, const QDate& currentDate, const FiscalYearInfo& fyContext) {
    QString curStr = currentDate.isValid() ? currentDate.toString("dd-MM-yyyy") : "";
    QString disp, iso;
    if (getVoucherDate(parent, curStr, &disp, &iso, fyContext)) {
        return QDate::fromString(iso, "yyyy-MM-dd");
    }
    return QDate();
}
