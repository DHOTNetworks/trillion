#pragma once

#include <QWidget>
#include <QTableWidget>
#include <QLineEdit>
#include <QLabel>
#include <QPushButton>
#include <QFrame>
#include "../models/cash_bank_flow_controller.h"
#include "../widgets/accounting_date_edit.h"
#include "../services/print_export_controller.h"

namespace MahadevERP {

class CashBankFlowWidget : public QWidget {
    Q_OBJECT

public:
    explicit CashBankFlowWidget(PrintExportController* printExportCtrl = nullptr,
                                QWidget* parent = nullptr);
    ~CashBankFlowWidget() override = default;

    void setStatementMode(FlowStatementType mode);
    FlowStatementType statementMode() const { return m_controller.statementType(); }

    void refreshData();
    void setDateRange(const QString& fromDate, const QString& toDate);

signals:
    void backRequested();
    void partyStatementRequested(const QString& partyName, const QString& fromDate, const QString& toDate);

protected:
    void showEvent(QShowEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;

private slots:
    void onDateChanged();
    void onSearchTextChanged(const QString& text);
    void onRowDoubleClicked(int row, int col);
    void onExportPdf();
    void onExportCsv();
    void onAccountingPeriod();

private:
    void setupUi();
    void updateTableData();
    void updateHeaderLabels();
    void updateSummaryMetrics();

    CashBankFlowController m_controller;
    PrintExportController* m_printExportCtrl = nullptr;

    // Header Tier 1
    QLabel* m_titleLabel = nullptr;
    QLabel* m_subTitleLabel = nullptr;
    QPushButton* m_cashModeBtn = nullptr;
    QPushButton* m_bankModeBtn = nullptr;
    QPushButton* m_jointModeBtn = nullptr;
    QPushButton* m_pdfBtn = nullptr;
    QPushButton* m_csvBtn = nullptr;
    QPushButton* m_backBtn = nullptr;

    // Filter Tier 2
    AccountingDateEdit* m_fromDateEdit = nullptr;
    AccountingDateEdit* m_toDateEdit = nullptr;
    QLineEdit* m_searchBox = nullptr;
    QLabel* m_fyBadge = nullptr;
    QPushButton* m_periodBtn = nullptr;

    // Data Table Tier 3
    QTableWidget* m_table = nullptr;

    // Footer Tier 4 Cards
    QLabel* m_opBalValLabel = nullptr;
    QLabel* m_receiptsValLabel = nullptr;
    QLabel* m_paymentsValLabel = nullptr;
    QLabel* m_clBalValLabel = nullptr;
    QLabel* m_clBalTitleLabel = nullptr;
};

} // namespace MahadevERP
