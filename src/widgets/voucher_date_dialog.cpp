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

    QDate parsedDate;
    if (!m_initialDate.isEmpty()) {
        QString iso = FiscalYearHelper::normalizeToIso(m_initialDate);
        if (!iso.isEmpty()) {
            parsedDate = QDate::fromString(iso, "yyyy-MM-dd");
        }
        if (!parsedDate.isValid()) {
            QString parsed = m_fyModel.parse_date_pattern(m_initialDate);
            if (!parsed.isEmpty()) {
                parsedDate = QDate::fromString(parsed, "dd-MM-yyyy");
            }
        }
        if (!parsedDate.isValid()) parsedDate = QDate::fromString(m_initialDate, "dd-MM-yyyy");
        if (!parsedDate.isValid()) parsedDate = QDate::fromString(m_initialDate, "yyyy-MM-dd");
        if (!parsedDate.isValid()) parsedDate = QDate::fromString(m_initialDate, "dd/MM/yyyy");
        if (!parsedDate.isValid()) parsedDate = QDate::fromString(m_initialDate, "yyyy/MM/dd");
    }

    if (!m_contextFy.isValid()) {
        if (parsedDate.isValid()) {
            m_contextFy = FiscalYearHelper::getFiscalYearForDate(parsedDate.toString("yyyy-MM-dd"));
        }
        if (!m_contextFy.isValid() && !m_initialDate.isEmpty()) {
            QString iso = FiscalYearHelper::normalizeToIso(m_initialDate);
            if (!iso.isEmpty()) {
                m_contextFy = FiscalYearHelper::getFiscalYearForDate(iso);
            }
        }
        if (!m_contextFy.isValid()) {
            m_contextFy = FiscalYearHelper::getActiveFiscalYear();
        }
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

    if (valid) {
        FiscalYearInfo targetFy = m_contextFy.isValid() ? m_contextFy : FiscalYearHelper::getActiveFiscalYear();
        if (targetFy.isValid()) {
            QDate d = QDate::fromString(res.value("formattedDate").toString(), "dd-MM-yyyy");
            QDate s = QDate::fromString(targetFy.startDate, "yyyy-MM-dd");
            QDate e = QDate::fromString(targetFy.endDate, "yyyy-MM-dd");
            if (s.isValid() && e.isValid() && (d < s || d > e)) {
                FiscalYearInfo altFy = FiscalYearHelper::getFiscalYearForDate(res.value("isoDate").toString());
                if (altFy.isValid()) {
                    m_contextFy = altFy;
                } else {
                    valid = false;
                    res["error"] = QString("Date %1 is outside the Financial Year Period (%2: %3 to %4).\nPlease enter a date within this period or switch the Financial Year (Alt+F).")
                                        .arg(d.toString("dd-MM-yyyy"), targetFy.name, s.toString("dd-MM-yyyy"), e.toString("dd-MM-yyyy"));
                }
            }
        }
    }

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

bool VoucherDateDialog::getDateRange(QWidget* parent,
                                     QString* outFromDisplayDate,
                                     QString* outToDisplayDate,
                                     QString* outFromIso,
                                     QString* outToIso,
                                     const QString& initialFromDate,
                                     const QString& initialToDate,
                                     const FiscalYearInfo& fyContext)
{
    if (QApplication::focusWidget()) {
        QApplication::focusWidget()->clearFocus();
    }
    if (parent) {
        parent->clearFocus();
    }

    QWidget* topParent = parent ? parent->window() : nullptr;
    DateRangeDialog dlg(initialFromDate, initialToDate, fyContext, topParent);
    if (topParent) {
        dlg.move(topParent->geometry().center() - dlg.rect().center());
    }
    int code = dlg.exec();
    if (topParent) {
        topParent->activateWindow();
    }
    if (code == QDialog::Accepted) {
        if (outFromDisplayDate) *outFromDisplayDate = dlg.fromDisplayDate();
        if (outToDisplayDate) *outToDisplayDate = dlg.toDisplayDate();
        if (outFromIso) *outFromIso = dlg.fromIsoDate();
        if (outToIso) *outToIso = dlg.toIsoDate();
        return true;
    }
    return false;
}

bool VoucherDateDialog::selectDateRange(QWidget* parent,
                                        QDate* outFromDate,
                                        QDate* outToDate,
                                        const QDate& initialFrom,
                                        const QDate& initialTo,
                                        const FiscalYearInfo& fyContext)
{
    QString fromStr = initialFrom.isValid() ? initialFrom.toString("dd-MM-yyyy") : "";
    QString toStr = initialTo.isValid() ? initialTo.toString("dd-MM-yyyy") : "";
    QString fDisp, tDisp, fIso, tIso;
    if (getDateRange(parent, &fDisp, &tDisp, &fIso, &tIso, fromStr, toStr, fyContext)) {
        if (outFromDate) *outFromDate = QDate::fromString(fIso, "yyyy-MM-dd");
        if (outToDate) *outToDate = QDate::fromString(tIso, "yyyy-MM-dd");
        return true;
    }
    return false;
}

// ============================================================================
// DateRangeDialog Implementation
// ============================================================================

DateRangeDialog::DateRangeDialog(const QString& fromDate, const QString& toDate, const FiscalYearInfo& fyContext, QWidget* parent)
    : QDialog(parent)
    , m_contextFy(fyContext)
    , m_initialFrom(fromDate.trimmed())
    , m_initialTo(toDate.trimmed())
{
    setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
    setWindowModality(Qt::ApplicationModal);
    setModal(true);
    setAttribute(Qt::WA_TranslucentBackground, false);
    setAttribute(Qt::WA_StyledBackground, true);

    if (!m_contextFy.isValid()) {
        if (!m_initialFrom.isEmpty()) {
            m_contextFy = FiscalYearHelper::getFiscalYearForDate(m_initialFrom);
        }
        if (!m_contextFy.isValid()) {
            m_contextFy = FiscalYearHelper::getActiveFiscalYear();
        }
    }

    QDate defFrom = QDate::fromString(m_contextFy.startDate, "yyyy-MM-dd");
    if (!defFrom.isValid()) defFrom = QDate(QDate::currentDate().month() < 4 ? QDate::currentDate().year() - 1 : QDate::currentDate().year(), 4, 1);
    QDate defTo = QDate::currentDate();

    QDate pFrom = parseDateFlexible(m_initialFrom, defFrom);
    QDate pTo = parseDateFlexible(m_initialTo, defTo);

    m_fromDisplayDate = pFrom.toString("dd-MM-yyyy");
    m_toDisplayDate = pTo.toString("dd-MM-yyyy");
    m_fromIsoDate = pFrom.toString("yyyy-MM-dd");
    m_toIsoDate = pTo.toString("yyyy-MM-dd");

    setupUi();
}

QDate DateRangeDialog::parseDateFlexible(const QString& input, const QDate& fallback) {
    QString val = input.trimmed();
    if (val.isEmpty()) return fallback;

    val.replace('/', '-').replace('.', '-');

    // If day-month without year (e.g. "1-4" or "15-8")
    QStringList parts = val.split('-', Qt::SkipEmptyParts);
    if (parts.size() == 2) {
        int d = parts[0].toInt();
        int m = parts[1].toInt();
        if (d >= 1 && d <= 31 && m >= 1 && m <= 12) {
            int y = QDate::currentDate().year();
            if (m_contextFy.isValid()) {
                QDate fyStart = QDate::fromString(m_contextFy.startDate, "yyyy-MM-dd");
                QDate fyEnd = QDate::fromString(m_contextFy.endDate, "yyyy-MM-dd");
                if (m >= 4 && fyStart.isValid()) y = fyStart.year();
                else if (m <= 3 && fyEnd.isValid()) y = fyEnd.year();
            }
            QDate res(y, m, d);
            if (res.isValid()) return res;
        }
    }

    if (parts.size() == 3) {
        int d = 0, m = 0, y = 0;
        if (parts[0].length() == 4 && parts[0].toInt() >= 1900) {
            // ISO yyyy-MM-dd
            y = parts[0].toInt();
            m = parts[1].toInt();
            d = parts[2].toInt();
        } else {
            d = parts[0].toInt();
            m = parts[1].toInt();
            y = parts[2].toInt();
            if (parts[2].length() <= 2 || y < 100) {
                y += 2000;
            }
        }
        if (y >= 1900 && y <= 1970) {
            y += 100;
        }
        QDate res(y, m, d);
        if (res.isValid()) return res;
    }

    // All digits: e.g. 010426 (6) or 01042026 (8)
    if (val.length() == 6 && val.toInt() > 0) {
        int d = val.left(2).toInt();
        int m = val.mid(2, 2).toInt();
        int y = val.mid(4, 2).toInt() + 2000;
        QDate res(y, m, d);
        if (res.isValid()) return res;
    }
    if (val.length() == 8 && val.toInt() > 0) {
        int d = val.left(2).toInt();
        int m = val.mid(2, 2).toInt();
        int y = val.mid(4, 4).toInt();
        if (y >= 1900 && y <= 1970) y += 100;
        QDate res(y, m, d);
        if (res.isValid()) return res;
    }

    FinancialYearsModel fyModel;
    QString parsed = fyModel.parse_date_pattern(val);
    if (!parsed.isEmpty()) {
        QDate res = QDate::fromString(parsed, "dd-MM-yyyy");
        if (res.isValid()) return res;
    }

    return fallback;
}

void DateRangeDialog::setupUi() {
    setFixedWidth(440);

    setStyleSheet(
        "QDialog {"
        "  background-color: #F8FAFC;"
        "  border: 2px solid #2563EB;"
        "  border-radius: 10px;"
        "}"
        "QLabel {"
        "  color: #0F172A;"
        "  background: transparent;"
        "  font-family: 'Segoe UI', -apple-system, BlinkMacSystemFont, Roboto, sans-serif;"
        "}"
        "QLineEdit {"
        "  background-color: #FFFFFF;"
        "  color: #0F172A;"
        "  border: 2px solid #CBD5E1;"
        "  border-radius: 6px;"
        "  font-size: 15px;"
        "  font-weight: 800;"
        "  padding: 6px 10px;"
        "}"
        "QLineEdit:focus {"
        "  border: 2px solid #2563EB;"
        "  background-color: #EFF6FF;"
        "}"
        "QPushButton {"
        "  font-size: 11.5px;"
        "  font-weight: 700;"
        "  padding: 5px 10px;"
        "  border-radius: 6px;"
        "}"
    );

    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(20, 18, 20, 18);
    mainLayout->setSpacing(10);

    // Title / FY context
    QString fyTitle = m_contextFy.isValid()
        ? QString("Select Date Range — %1 (%2 to %3)").arg(m_contextFy.name, FiscalYearHelper::formatDisplayDate(m_contextFy.startDate), FiscalYearHelper::formatDisplayDate(m_contextFy.endDate))
        : "Select Period / Date Range";
    m_fyInfoLabel = new QLabel(fyTitle, this);
    m_fyInfoLabel->setAlignment(Qt::AlignCenter);
    m_fyInfoLabel->setStyleSheet("font-size: 12px; font-weight: 800; color: #1E40AF; padding-bottom: 2px;");
    mainLayout->addWidget(m_fyInfoLabel);

    // Preset Chips Bar
    QHBoxLayout* presetsLayout = new QHBoxLayout();
    presetsLayout->setSpacing(5);

    auto addPresetBtn = [this, presetsLayout](const QString& label, auto slotFunc) {
        QPushButton* btn = new QPushButton(label, this);
        btn->setStyleSheet(
            "QPushButton { background-color: #E2E8F0; color: #334155; border: 1px solid #CBD5E1; font-weight: 700; font-size: 11px; padding: 3px 8px; }"
            "QPushButton:hover { background-color: #DBEAFE; color: #1D4ED8; border-color: #93C5FD; }"
            "QPushButton:pressed { background-color: #BFDBFE; }"
        );
        connect(btn, &QPushButton::clicked, this, slotFunc);
        presetsLayout->addWidget(btn);
    };

    addPresetBtn("Full FY", &DateRangeDialog::setPresetFullFy);
    addPresetBtn("This Month", &DateRangeDialog::setPresetCurrentMonth);
    addPresetBtn("Today", &DateRangeDialog::setPresetToday);
    addPresetBtn("Q1", [this]() { setPresetQuarter(1); });
    addPresetBtn("Q2", [this]() { setPresetQuarter(2); });
    addPresetBtn("Q3", [this]() { setPresetQuarter(3); });
    addPresetBtn("Q4", [this]() { setPresetQuarter(4); });

    mainLayout->addLayout(presetsLayout);

    // Inputs Grid (From Date & To Date)
    QFrame* inputCard = new QFrame(this);
    inputCard->setStyleSheet("background-color: #FFFFFF; border: 1px solid #E2E8F0; border-radius: 8px;");
    QHBoxLayout* inputLayout = new QHBoxLayout(inputCard);
    inputLayout->setContentsMargins(12, 10, 12, 10);
    inputLayout->setSpacing(12);

    QVBoxLayout* fromCol = new QVBoxLayout();
    fromCol->setSpacing(4);
    QLabel* fromLbl = new QLabel("FROM DATE", inputCard);
    fromLbl->setStyleSheet("font-size: 10.5px; font-weight: 800; color: #475569; letter-spacing: 0.5px;");
    m_fromInput = new QLineEdit(inputCard);
    m_fromInput->setAlignment(Qt::AlignCenter);
    m_fromInput->setText(m_fromDisplayDate);
    m_fromInput->installEventFilter(this);
    connect(m_fromInput, &QLineEdit::textChanged, this, &DateRangeDialog::updateDurationLabel);
    fromCol->addWidget(fromLbl);
    fromCol->addWidget(m_fromInput);
    inputLayout->addLayout(fromCol, 1);

    QLabel* arrowLbl = new QLabel("→", inputCard);
    arrowLbl->setStyleSheet("font-size: 18px; font-weight: 900; color: #94A3B8; margin-top: 14px;");
    arrowLbl->setAlignment(Qt::AlignCenter);
    inputLayout->addWidget(arrowLbl);

    QVBoxLayout* toCol = new QVBoxLayout();
    toCol->setSpacing(4);
    QLabel* toLbl = new QLabel("TO DATE", inputCard);
    toLbl->setStyleSheet("font-size: 10.5px; font-weight: 800; color: #475569; letter-spacing: 0.5px;");
    m_toInput = new QLineEdit(inputCard);
    m_toInput->setAlignment(Qt::AlignCenter);
    m_toInput->setText(m_toDisplayDate);
    m_toInput->installEventFilter(this);
    connect(m_toInput, &QLineEdit::textChanged, this, &DateRangeDialog::updateDurationLabel);
    toCol->addWidget(toLbl);
    toCol->addWidget(m_toInput);
    inputLayout->addLayout(toCol, 1);

    mainLayout->addWidget(inputCard);

    // Duration & Summary label
    m_durationLabel = new QLabel("", this);
    m_durationLabel->setAlignment(Qt::AlignCenter);
    m_durationLabel->setStyleSheet("font-size: 12px; font-weight: 700; color: #059669; padding: 2px;");
    mainLayout->addWidget(m_durationLabel);

    // Error label
    m_errorLabel = new QLabel("", this);
    m_errorLabel->setAlignment(Qt::AlignCenter);
    m_errorLabel->setStyleSheet("font-size: 11px; font-weight: 700; color: #DC2626; padding: 2px;");
    m_errorLabel->setWordWrap(true);
    m_errorLabel->setVisible(false);
    mainLayout->addWidget(m_errorLabel);

    // Action buttons
    QHBoxLayout* btnLayout = new QHBoxLayout();
    btnLayout->setSpacing(8);

    m_applyBtn = new QPushButton("Apply Period (Enter)", this);
    m_applyBtn->setStyleSheet(
        "QPushButton { background-color: #2563EB; color: #FFFFFF; border: 1px solid #1D4ED8; font-weight: 800; padding: 8px 16px; font-size: 12px; } "
        "QPushButton:hover { background-color: #1D4ED8; } "
        "QPushButton:pressed { background-color: #1E40AF; }"
    );
    connect(m_applyBtn, &QPushButton::clicked, this, &DateRangeDialog::onAccept);
    btnLayout->addWidget(m_applyBtn);

    m_cancelBtn = new QPushButton("Cancel (Esc)", this);
    m_cancelBtn->setStyleSheet("QPushButton { background-color: #FFFFFF; color: #475569; border: 1px solid #CBD5E1; padding: 8px 16px; font-size: 12px; } QPushButton:hover { background-color: #F1F5F9; }");
    connect(m_cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
    btnLayout->addWidget(m_cancelBtn);

    mainLayout->addLayout(btnLayout);

    updateDurationLabel();
    adjustSize();
}

void DateRangeDialog::showEvent(QShowEvent* event) {
    QDialog::showEvent(event);
    activateWindow();
    raise();
    if (m_fromInput) {
        m_fromInput->setFocus(Qt::OtherFocusReason);
        m_fromInput->selectAll();
    }
}

bool DateRangeDialog::eventFilter(QObject* watched, QEvent* event) {
    if (event->type() == QEvent::KeyPress) {
        auto* kEvent = static_cast<QKeyEvent*>(event);
        if (kEvent->key() == Qt::Key_Return || kEvent->key() == Qt::Key_Enter) {
            if (watched == m_fromInput) {
                // Shift to To input
                QDate f = parseDateFlexible(m_fromInput->text(), QDate::currentDate());
                m_fromInput->setText(f.toString("dd-MM-yyyy"));
                if (m_toInput) {
                    m_toInput->setFocus();
                    m_toInput->selectAll();
                }
                return true;
            } else if (watched == m_toInput) {
                onAccept();
                return true;
            }
        } else if (kEvent->key() == Qt::Key_Escape) {
            reject();
            return true;
        }
    }
    return QDialog::eventFilter(watched, event);
}

void DateRangeDialog::keyPressEvent(QKeyEvent* event) {
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

void DateRangeDialog::updateDurationLabel() {
    QDate f = parseDateFlexible(m_fromInput ? m_fromInput->text() : "", QDate());
    QDate t = parseDateFlexible(m_toInput ? m_toInput->text() : "", QDate());

    if (f.isValid() && t.isValid()) {
        if (f <= t) {
            int days = f.daysTo(t) + 1;
            m_durationLabel->setText(QString(" %1 to %2 (%3 days)").arg(f.toString("dd-MM-yyyy"), t.toString("dd-MM-yyyy"), QString::number(days)));
            m_durationLabel->setStyleSheet("font-size: 12px; font-weight: 700; color: #059669;");
            if (m_errorLabel) m_errorLabel->setVisible(false);
        } else {
            m_durationLabel->setText("From Date cannot be after To Date");
            m_durationLabel->setStyleSheet("font-size: 12px; font-weight: 700; color: #DC2626;");
        }
    }
}

void DateRangeDialog::setPresetFullFy() {
    FiscalYearInfo fy = m_contextFy.isValid() ? m_contextFy : FiscalYearHelper::getActiveFiscalYear();
    QDate f = QDate::fromString(fy.startDate, "yyyy-MM-dd");
    QDate t = QDate::fromString(fy.endDate, "yyyy-MM-dd");
    if (!f.isValid()) f = QDate(QDate::currentDate().month() < 4 ? QDate::currentDate().year() - 1 : QDate::currentDate().year(), 4, 1);
    if (!t.isValid()) t = QDate(f.year() + 1, 3, 31);

    m_fromInput->setText(f.toString("dd-MM-yyyy"));
    m_toInput->setText(t.toString("dd-MM-yyyy"));
    updateDurationLabel();
}

void DateRangeDialog::setPresetCurrentMonth() {
    QDate now = QDate::currentDate();
    QDate f(now.year(), now.month(), 1);
    QDate t(now.year(), now.month(), now.daysInMonth());

    m_fromInput->setText(f.toString("dd-MM-yyyy"));
    m_toInput->setText(t.toString("dd-MM-yyyy"));
    updateDurationLabel();
}

void DateRangeDialog::setPresetToday() {
    QDate now = QDate::currentDate();
    m_fromInput->setText(now.toString("dd-MM-yyyy"));
    m_toInput->setText(now.toString("dd-MM-yyyy"));
    updateDurationLabel();
}

void DateRangeDialog::setPresetQuarter(int q) {
    FiscalYearInfo fy = m_contextFy.isValid() ? m_contextFy : FiscalYearHelper::getActiveFiscalYear();
    QDate fyStart = QDate::fromString(fy.startDate, "yyyy-MM-dd");
    int startYear = fyStart.isValid() ? fyStart.year() : (QDate::currentDate().month() < 4 ? QDate::currentDate().year() - 1 : QDate::currentDate().year());

    QDate f, t;
    if (q == 1) { // Apr - Jun
        f = QDate(startYear, 4, 1);
        t = QDate(startYear, 6, 30);
    } else if (q == 2) { // Jul - Sep
        f = QDate(startYear, 7, 1);
        t = QDate(startYear, 9, 30);
    } else if (q == 3) { // Oct - Dec
        f = QDate(startYear, 10, 1);
        t = QDate(startYear, 12, 31);
    } else if (q == 4) { // Jan - Mar
        f = QDate(startYear + 1, 1, 1);
        t = QDate(startYear + 1, 3, 31);
    }

    m_fromInput->setText(f.toString("dd-MM-yyyy"));
    m_toInput->setText(t.toString("dd-MM-yyyy"));
    updateDurationLabel();
}

void DateRangeDialog::onAccept() {
    QDate f = parseDateFlexible(m_fromInput ? m_fromInput->text() : "", QDate());
    QDate t = parseDateFlexible(m_toInput ? m_toInput->text() : "", QDate());

    if (!f.isValid()) {
        m_errorLabel->setText("Please enter a valid From Date (dd-MM-yyyy).");
        m_errorLabel->setVisible(true);
        if (m_fromInput) { m_fromInput->setFocus(); m_fromInput->selectAll(); }
        return;
    }
    if (!t.isValid()) {
        m_errorLabel->setText("Please enter a valid To Date (dd-MM-yyyy).");
        m_errorLabel->setVisible(true);
        if (m_toInput) { m_toInput->setFocus(); m_toInput->selectAll(); }
        return;
    }
    if (f > t) {
        m_errorLabel->setText("From Date cannot be later than To Date.");
        m_errorLabel->setVisible(true);
        return;
    }

    m_fromDisplayDate = f.toString("dd-MM-yyyy");
    m_toDisplayDate = t.toString("dd-MM-yyyy");
    m_fromIsoDate = f.toString("yyyy-MM-dd");
    m_toIsoDate = t.toString("yyyy-MM-dd");

    accept();
}
