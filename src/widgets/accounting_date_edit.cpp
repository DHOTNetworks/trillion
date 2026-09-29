#include "accounting_date_edit.h"
#include "voucher_date_dialog.h"
#include <QKeyEvent>
#include <QFocusEvent>
#include <QMouseEvent>
#include <QAction>
#include <QPainter>
#include <QPixmap>
#include <QIcon>

static QIcon createCalendarIcon() {
    QPixmap pixmap(20, 20);
    pixmap.fill(Qt::transparent);
    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);

    // Outer card
    painter.setPen(QPen(QColor("#2563EB"), 1.5));
    painter.setBrush(QColor("#FFFFFF"));
    painter.drawRoundedRect(2, 3, 16, 14, 3, 3);

    // Top banner
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor("#2563EB"));
    painter.drawRoundedRect(2, 3, 16, 4, 2, 2);

    // Top binder clips
    painter.setBrush(QColor("#1E40AF"));
    painter.drawRect(5, 1, 2, 3);
    painter.drawRect(13, 1, 2, 3);

    // Calendar grid marks
    painter.setBrush(QColor("#64748B"));
    painter.drawRect(5, 9, 2, 2);
    painter.drawRect(9, 9, 2, 2);
    painter.drawRect(13, 9, 2, 2);
    painter.drawRect(5, 13, 2, 2);
    painter.drawRect(9, 13, 2, 2);
    painter.drawRect(13, 13, 2, 2);

    return QIcon(pixmap);
}

AccountingDateEdit::AccountingDateEdit(QWidget* parent)
    : QLineEdit(parent)
{
    setInputMask("00-00-0000;_");
    m_lastDate = QDate::currentDate();
    setText(m_lastDate.toString("dd-MM-yyyy"));
    setFixedHeight(34);
    setAlignment(Qt::AlignCenter);
    setStyleSheet(
        "QLineEdit {"
        "  background-color: #FFFFFF;"
        "  color: #0F172A;"
        "  border: 1px solid #CBD5E1;"
        "  border-radius: 6px;"
        "  padding: 4px 6px;"
        "  font-size: 13px;"
        "  font-weight: bold;"
        "  font-family: 'Segoe UI', -apple-system, BlinkMacSystemFont, Roboto, sans-serif;"
        "}"
        "QLineEdit:focus {"
        "  border: 2px solid #2563EB;"
        "  background-color: #F8FAFC;"
        "}"
    );

    setupCalendarAction();
    connect(this, &QLineEdit::editingFinished, this, &AccountingDateEdit::formatInput);
}

AccountingDateEdit::AccountingDateEdit(const QDate& initialDate, QWidget* parent)
    : AccountingDateEdit(parent)
{
    if (initialDate.isValid()) {
        setDate(initialDate);
    }
}

void AccountingDateEdit::setupCalendarAction() {
    m_calendarAction = addAction(createCalendarIcon(), QLineEdit::TrailingPosition);
    m_calendarAction->setToolTip("Open Date Selector (F2 / Double Click)");
    connect(m_calendarAction, &QAction::triggered, this, &AccountingDateEdit::openDateDialog);
}

void AccountingDateEdit::openDateDialog() {
    QDate cur = date();
    if (!cur.isValid()) cur = QDate::currentDate();

    FiscalYearInfo activeFy = FiscalYearHelper::getActiveFiscalYear();
    QDate chosen = VoucherDateDialog::selectDate(this, cur, activeFy);
    if (chosen.isValid() && chosen != date()) {
        setDate(chosen);
    }
}

QDate AccountingDateEdit::date() const {
    QString t = text().trimmed();
    QDate d = QDate::fromString(t, "dd-MM-yyyy");
    if (d.isValid()) return d;
    return QDate();
}

void AccountingDateEdit::setDate(const QDate& date) {
    if (!date.isValid()) return;
    QDate target = date;
    FiscalYearInfo activeFy = FiscalYearHelper::getActiveFiscalYear();
    if (activeFy.isValid()) {
        QDate fyStart = QDate::fromString(activeFy.startDate, "yyyy-MM-dd");
        QDate fyEnd = QDate::fromString(activeFy.endDate, "yyyy-MM-dd");
        if (fyStart.isValid() && fyEnd.isValid()) {
            if (target < fyStart) target = fyStart;
            if (target > fyEnd) target = fyEnd;
        }
    }
    QString formatted = target.toString("dd-MM-yyyy");
    if (text() != formatted) {
        setText(formatted);
    }
    if (target != m_lastDate) {
        m_lastDate = target;
        emit dateChanged(target);
    }
}

QString AccountingDateEdit::isoDate() const {
    QDate d = date();
    if (d.isValid()) {
        return d.toString("yyyy-MM-dd");
    }
    return "";
}

void AccountingDateEdit::setIsoDate(const QString& isoDate) {
    if (isoDate.trimmed().isEmpty()) return;
    QDate d = QDate::fromString(isoDate.trimmed(), "yyyy-MM-dd");
    if (d.isValid()) {
        setDate(d);
    }
}

QString AccountingDateEdit::formattedDate() const {
    return text();
}

void AccountingDateEdit::focusInEvent(QFocusEvent* event) {
    QLineEdit::focusInEvent(event);
    selectAll();
}

void AccountingDateEdit::focusOutEvent(QFocusEvent* event) {
    formatInput();
    QLineEdit::focusOutEvent(event);
}

void AccountingDateEdit::mouseDoubleClickEvent(QMouseEvent* event) {
    QLineEdit::mouseDoubleClickEvent(event);
    openDateDialog();
}

void AccountingDateEdit::keyPressEvent(QKeyEvent* event) {
    if (event->key() == Qt::Key_F2) {
        openDateDialog();
        event->accept();
        return;
    } else if (event->key() == Qt::Key_Up) {
        QDate d = date();
        if (d.isValid()) {
            setDate(d.addDays(1));
            selectAll();
            event->accept();
            return;
        }
    } else if (event->key() == Qt::Key_Down) {
        QDate d = date();
        if (d.isValid()) {
            setDate(d.addDays(-1));
            selectAll();
            event->accept();
            return;
        }
    }
    QLineEdit::keyPressEvent(event);
}

void AccountingDateEdit::formatInput() {
    QDate d = date();
    if (!d.isValid()) {
        QString t = text().trimmed();
        FinancialYearsModel fyModel;
        QString parsed = fyModel.parse_date_pattern(t);
        if (!parsed.isEmpty()) {
            d = QDate::fromString(parsed, "dd-MM-yyyy");
        }
    }
    if (d.isValid()) {
        setDate(d);
    } else if (m_lastDate.isValid()) {
        setText(m_lastDate.toString("dd-MM-yyyy"));
    }
}

// ==========================================
// AccountingDateDisplay Implementation
// ==========================================

AccountingDateDisplay::AccountingDateDisplay(QWidget* parent)
    : QLabel(parent)
{
    m_date = QDate::currentDate();
    setFixedHeight(34);
    setMinimumWidth(110);
    setAlignment(Qt::AlignCenter);
    setCursor(Qt::PointingHandCursor);
    setStyleSheet(
        "QLabel {"
        "  background-color: #F8FAFC;"
        "  color: #0F172A;"
        "  border: 1px solid #CBD5E1;"
        "  border-radius: 6px;"
        "  padding: 4px 10px;"
        "  font-size: 13px;"
        "  font-weight: bold;"
        "  font-family: 'Segoe UI', -apple-system, BlinkMacSystemFont, Roboto, sans-serif;"
        "}"
        "QLabel:hover {"
        "  background-color: #F1F5F9;"
        "  border-color: #94A3B8;"
        "}"
    );
    updateDisplayText();
}

AccountingDateDisplay::AccountingDateDisplay(const QDate& initialDate, QWidget* parent)
    : AccountingDateDisplay(parent)
{
    if (initialDate.isValid()) {
        m_date = initialDate;
        updateDisplayText();
    }
}

QDate AccountingDateDisplay::date() const {
    return m_date;
}

void AccountingDateDisplay::setDate(const QDate& date) {
    if (!date.isValid() || date == m_date) return;
    m_date = date;
    updateDisplayText();
    emit dateChanged(date);
}

QString AccountingDateDisplay::isoDate() const {
    return m_date.isValid() ? m_date.toString("yyyy-MM-dd") : "";
}

void AccountingDateDisplay::setIsoDate(const QString& isoDate) {
    if (isoDate.trimmed().isEmpty()) return;
    QDate d = QDate::fromString(isoDate.trimmed(), "yyyy-MM-dd");
    if (d.isValid()) {
        setDate(d);
    }
}

QString AccountingDateDisplay::formattedDate() const {
    return m_date.isValid() ? m_date.toString("dd-MM-yyyy") : "";
}

void AccountingDateDisplay::updateDisplayText() {
    setText(formattedDate());
}

void AccountingDateDisplay::mousePressEvent(QMouseEvent* event) {
    QLabel::mousePressEvent(event);
    emit clicked();
}

