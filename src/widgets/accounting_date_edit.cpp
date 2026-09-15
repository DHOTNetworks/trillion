#include "accounting_date_edit.h"
#include <QKeyEvent>
#include <QFocusEvent>

AccountingDateEdit::AccountingDateEdit(QWidget* parent)
    : QLineEdit(parent)
{
    setInputMask("00-00-0000;_");
    setText(QDate::currentDate().toString("dd-MM-yyyy"));
    setFixedHeight(34);
    setAlignment(Qt::AlignCenter);
    setStyleSheet(
        "QLineEdit {"
        "  background-color: #FFFFFF;"
        "  color: #0F172A;"
        "  border: 1px solid #CBD5E1;"
        "  border-radius: 6px;"
        "  padding: 4px 8px;"
        "  font-size: 13px;"
        "  font-weight: bold;"
        "  font-family: 'Segoe UI', sans-serif;"
        "}"
        "QLineEdit:focus {"
        "  border: 2px solid #2563EB;"
        "  background-color: #F8FAFC;"
        "}"
    );

    connect(this, &QLineEdit::editingFinished, this, &AccountingDateEdit::formatInput);
}

QDate AccountingDateEdit::date() const {
    QString t = text().trimmed();
    QDate d = QDate::fromString(t, "dd-MM-yyyy");
    if (d.isValid()) return d;
    return QDate();
}

void AccountingDateEdit::setDate(const QDate& date) {
    if (date.isValid()) {
        setText(date.toString("dd-MM-yyyy"));
        emit dateChanged(date);
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

void AccountingDateEdit::keyPressEvent(QKeyEvent* event) {
    if (event->key() == Qt::Key_Up) {
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
    if (d.isValid()) {
        emit dateChanged(d);
    }
}
