#pragma once

#include <QWidget>
#include <QTableWidget>
#include <QLineEdit>
#include <QDateEdit>
#include <QComboBox>
#include <QCheckBox>
#include <QPushButton>
#include <QLabel>
#include "../models/interest_model.h"
#include "../services/print_export_controller.h"
#include "account_search_box.h"

class KbdBadgeButton;

namespace MahadevERP {

class InterestCalculatorWidget : public QWidget {
    Q_OBJECT

public:
    explicit InterestCalculatorWidget(InterestModel* model = nullptr,
                                      PrintExportController* printExportCtrl = nullptr,
                                      QWidget* parent = nullptr);

    void calculateInterest();
    void setParty(const QString& partyName);

signals:
    void backRequested();
    void voucherPosted();

private slots:
    void onPartySelected(const QString& partyName);
    void onCalculateClicked();
    void onPostVoucherClicked();
    void onExportCsv();

private:
    void setupUi();
    void populateTable(const QVariantMap& data);

    InterestModel* m_model = nullptr;
    PrintExportController* m_printExportCtrl = nullptr;

    AccountSearchBox* m_partySearch = nullptr;
    QDateEdit* m_fromDateEdit = nullptr;
    QDateEdit* m_toDateEdit = nullptr;

    QLineEdit* m_crRateEdit = nullptr;
    QLineEdit* m_drRateEdit = nullptr;
    QComboBox* m_divisorCombo = nullptr;
    QCheckBox* m_includeOpBalCheck = nullptr;

    // Metric Cards
    QLabel* m_totalDrProductLabel = nullptr;
    QLabel* m_totalCrProductLabel = nullptr;
    QLabel* m_netInterestLabel = nullptr;

    QTableWidget* m_table = nullptr;
    KbdBadgeButton* m_postBtn = nullptr;

    double m_lastCalculatedInterest = 0.0;
    bool m_isReceivable = true;
};

} // namespace MahadevERP
