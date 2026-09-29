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

class DateRangeDialog : public QDialog {
    Q_OBJECT

public:
    explicit DateRangeDialog(const QString& fromDate = "",
                             const QString& toDate = "",
                             const FiscalYearInfo& fyContext = FiscalYearInfo(),
                             QWidget* parent = nullptr);
    ~DateRangeDialog() override = default;

    QString fromDisplayDate() const { return m_fromDisplayDate; }
    QString toDisplayDate() const { return m_toDisplayDate; }
    QString fromIsoDate() const { return m_fromIsoDate; }
    QString toIsoDate() const { return m_toIsoDate; }

    QDate fromDate() const { return QDate::fromString(m_fromIsoDate, "yyyy-MM-dd"); }
    QDate toDate() const { return QDate::fromString(m_toIsoDate, "yyyy-MM-dd"); }

protected:
    void keyPressEvent(QKeyEvent* event) override;
    void showEvent(QShowEvent* event) override;
    bool eventFilter(QObject* watched, QEvent* event) override;

private slots:
    void onAccept();
    void setPresetFullFy();
    void setPresetCurrentMonth();
    void setPresetToday();
    void setPresetQuarter(int q);
    void updateDurationLabel();

private:
    void setupUi();
    QDate parseDateFlexible(const QString& input, const QDate& fallback);

    FiscalYearInfo m_contextFy;
    QString m_initialFrom;
    QString m_initialTo;

    QString m_fromDisplayDate;
    QString m_toDisplayDate;
    QString m_fromIsoDate;
    QString m_toIsoDate;

    QLabel* m_fyInfoLabel = nullptr;
    QLineEdit* m_fromInput = nullptr;
    QLineEdit* m_toInput = nullptr;
    QLabel* m_durationLabel = nullptr;
    QLabel* m_errorLabel = nullptr;
    QPushButton* m_applyBtn = nullptr;
    QPushButton* m_cancelBtn = nullptr;

    FinancialYearsModel m_fyModel;
};

class VoucherDateDialog : public QDialog {
    Q_OBJECT

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

    static bool getDateRange(QWidget* parent,
                             QString* outFromDisplayDate,
                             QString* outToDisplayDate,
                             QString* outFromIso = nullptr,
                             QString* outToIso = nullptr,
                             const QString& initialFromDate = "",
                             const QString& initialToDate = "",
                             const FiscalYearInfo& fyContext = FiscalYearInfo());

    static bool selectDateRange(QWidget* parent,
                                QDate* outFromDate,
                                QDate* outToDate,
                                const QDate& initialFrom = QDate(),
                                const QDate& initialTo = QDate(),
                                const FiscalYearInfo& fyContext = FiscalYearInfo());

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
