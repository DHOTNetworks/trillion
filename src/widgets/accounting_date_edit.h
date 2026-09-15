#pragma once

#include <QLineEdit>
#include <QDate>

class AccountingDateEdit : public QLineEdit {
    Q_OBJECT

public:
    explicit AccountingDateEdit(QWidget* parent = nullptr);

    QDate date() const;
    void setDate(const QDate& date);
    QString isoDate() const;
    void setIsoDate(const QString& isoDate);
    QString formattedDate() const;

signals:
    void dateChanged(const QDate& date);

protected:
    void focusInEvent(QFocusEvent* event) override;
    void focusOutEvent(QFocusEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;

private:
    void formatInput();
};
