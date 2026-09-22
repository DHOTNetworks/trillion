#pragma once

#include <QWidget>
#include <QTabWidget>
#include <QTableWidget>
#include <QLineEdit>
#include <QComboBox>
#include <QLabel>
#include <QPushButton>
#include <QDateEdit>
#include "../models/mandi_reports_controller.h"
#include "../services/print_export_controller.h"

class MandiReportsWidget : public QWidget {
    Q_OBJECT

public:
    explicit MandiReportsWidget(PrintExportController* printExportCtrl = nullptr,
                                QWidget* parent = nullptr);
    ~MandiReportsWidget() override;

    void refreshAllTabs();
    void setWorkingPeriod(const QString& fromDate, const QString& toDate);

public slots:
    void generateFormM();
    void generateJFormRegister();
    void generateIFormRegister();
    void generateFarmerStatement();
    void generateDamiRegister();

signals:
    void backRequested();
    void openJFormRequested(int voucherId);
    void openIFormRequested(int voucherId);

protected:
    void keyPressEvent(QKeyEvent* event) override;
    bool eventFilter(QObject* watched, QEvent* event) override;

private:
    void setupUi();
    void setupFormMTab();
    void setupJFormRegisterTab();
    void setupIFormRegisterTab();
    void setupFarmerStatementTab();
    void setupDamiRegisterTab();
    void applyCustomStyles();

    MandiReportsController m_mandiCtrl;
    PrintExportController* m_printExportCtrl = nullptr;

    QTabWidget* m_tabWidget = nullptr;

    // Tab 1: Form M
    QDateEdit* m_formMFromDate = nullptr;
    QDateEdit* m_formMToDate = nullptr;
    QTableWidget* m_formMTable = nullptr;
    QLabel* m_formMBagsSummary = nullptr;
    QLabel* m_formMWeightSummary = nullptr;
    QLabel* m_formMGoodsSummary = nullptr;
    QLabel* m_formMMFeeSummary = nullptr;
    QLabel* m_formMHRDFSummary = nullptr;
    QLabel* m_formMTotLevySummary = nullptr;

    // Tab 2: J-Form Register
    QDateEdit* m_jfFromDate = nullptr;
    QDateEdit* m_jfToDate = nullptr;
    QComboBox* m_jfFarmerCombo = nullptr;
    QTableWidget* m_jfTable = nullptr;
    QLabel* m_jfSummaryLabel = nullptr;

    // Tab 3: I-Form Register
    QDateEdit* m_ifFromDate = nullptr;
    QDateEdit* m_ifToDate = nullptr;
    QComboBox* m_ifBuyerCombo = nullptr;
    QTableWidget* m_ifTable = nullptr;
    QLabel* m_ifSummaryLabel = nullptr;

    // Tab 4: Farmer Statement
    QComboBox* m_stmtFarmerCombo = nullptr;
    QDateEdit* m_stmtFromDate = nullptr;
    QDateEdit* m_stmtToDate = nullptr;
    QTableWidget* m_stmtDheriesTable = nullptr;
    QTableWidget* m_stmtPaymentsTable = nullptr;
    QLabel* m_stmtCreditsLabel = nullptr;
    QLabel* m_stmtDebitsLabel = nullptr;
    QLabel* m_stmtNetBalLabel = nullptr;

    // Tab 5: Dami Register
    QDateEdit* m_damiFromDate = nullptr;
    QDateEdit* m_damiToDate = nullptr;
    QTableWidget* m_damiTable = nullptr;
    QLabel* m_damiSummaryLabel = nullptr;
};
