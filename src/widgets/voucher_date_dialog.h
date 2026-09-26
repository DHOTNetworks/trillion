#pragma once

#include <QDialog>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QDate>
#include "../engine/fiscal_year_helper.h"
#include "../models/financial_years_model.h"

class VoucherDateDialog : public QDialog {
    Q_OBJECT

public:
public:
    explicit VoucherDateDialog(const QString& currentDate = "", const FiscalYearInfo& fyContext = FiscalYearInfo(), QWidget* parent = nullptr);
    ~VoucherDateDialog() override = default;

    QString formattedDate() const { return m_formattedDate; }
    QString isoDate() const { return m_isoDate; }

    static bool getVoucherDate(QWidget* parent,
                              const QString& currentDate,
                              QString* outDisplayDate,
                              QString* outIsoDate = nullptr,
                              const FiscalYearInfo& fyContext = FiscalYearInfo());

    static QDate selectDate(QWidget* parent, const QDate& currentDate = QDate(), const FiscalYearInfo& fyContext = FiscalYearInfo());

protected:
    void keyPressEvent(QKeyEvent* event) override;
    void showEvent(QShowEvent* event) override;
    bool eventFilter(QObject* watched, QEvent* event) override;

private slots:
    void onAccept();

private:
    void setupUi();

    FiscalYearInfo m_contextFy;
    QString m_initialDate;
    QString m_workingDate;
    QString m_formattedDate;
    QString m_isoDate;

    QLabel* m_fyInfoLabel = nullptr;
    QLabel* m_currentDateLabel = nullptr;
    QLineEdit* m_dateInput = nullptr;
    QLabel* m_errorLabel = nullptr;
    QPushButton* m_applyBtn = nullptr;
    QPushButton* m_cancelBtn = nullptr;

    FinancialYearsModel m_fyModel;
};
