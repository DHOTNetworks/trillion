#pragma once

#include <QLineEdit>
#include <QDate>

class AccountingDateEdit : public QLineEdit {
    Q_OBJECT

public:
    explicit AccountingDateEdit(QWidget* parent = nullptr);
    explicit AccountingDateEdit(const QDate& initialDate, QWidget* parent = nullptr);

    QDate date() const;
    void setDate(const QDate& date);
    QString isoDate() const;
    void setIsoDate(const QString& isoDate);
    QString formattedDate() const;

    // Drop-in compatibility with QDateEdit
    void setDisplayFormat(const QString& format) { Q_UNUSED(format); }
    void setCalendarPopup(bool enable) { Q_UNUSED(enable); }

public slots:
    void openDateDialog();

signals:
    void dateChanged(const QDate& date);

protected:
    void focusInEvent(QFocusEvent* event) override;
    void focusOutEvent(QFocusEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    void mouseDoubleClickEvent(QMouseEvent* event) override;

private:
    void setupCalendarAction();
    void formatInput();
    QAction* m_calendarAction = nullptr;
    QDate m_lastDate;
};

#include <QLabel>

class AccountingDateDisplay : public QLabel {
    Q_OBJECT

public:
    explicit AccountingDateDisplay(QWidget* parent = nullptr);
    explicit AccountingDateDisplay(const QDate& initialDate, QWidget* parent = nullptr);

    QDate date() const;
    void setDate(const QDate& date);
    QString isoDate() const;
    void setIsoDate(const QString& isoDate);
    QString formattedDate() const;

    // Drop-in compatibility with QDateEdit / AccountingDateEdit
    void setDisplayFormat(const QString& format) { Q_UNUSED(format); }
    void setCalendarPopup(bool enable) { Q_UNUSED(enable); }

signals:
    void dateChanged(const QDate& date);
    void clicked();

protected:
    void mousePressEvent(QMouseEvent* event) override;

private:
    void updateDisplayText();
    QDate m_date;
};

