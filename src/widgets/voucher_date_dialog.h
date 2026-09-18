#pragma once

#include <QDialog>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QKeyEvent>
#include "../engine/fiscal_year_helper.h"
#include "../models/financial_years_model.h"

class VoucherDateDialog : public QDialog {
    Q_OBJECT

public:
    explicit VoucherDateDialog(const QString& currentDate = "", QWidget* parent = nullptr);
    ~VoucherDateDialog() override = default;

    QString formattedDate() const { return m_formattedDate; }
    QString isoDate() const { return m_isoDate; }

    static bool getVoucherDate(QWidget* parent,
                              const QString& currentDate,
                              QString* outDisplayDate,
                              QString* outIsoDate = nullptr);

protected:
    void keyPressEvent(QKeyEvent* event) override;
    void showEvent(QShowEvent* event) override;

private slots:
    void onAccept();

private:
    void setupUi();

    QString m_initialDate;
    QString m_workingDate;
    QString m_formattedDate;
    QString m_isoDate;

    QLabel* m_currentDateLabel = nullptr;
    QLineEdit* m_dateInput = nullptr;
    QLabel* m_errorLabel = nullptr;
    QPushButton* m_applyBtn = nullptr;
    QPushButton* m_cancelBtn = nullptr;

    FinancialYearsModel m_fyModel;
};
