#pragma once

#include <QWidget>
#include <QTableWidget>
#include <QLineEdit>
#include <QDateEdit>
#include <QPushButton>
#include <QLabel>
#include "../models/sales_register_model.h"
#include "../services/print_export_controller.h"

namespace MahadevERP {

class SalesRegisterWidget : public QWidget {
    Q_OBJECT

public:
    explicit SalesRegisterWidget(SalesRegisterController* controller = nullptr, PrintExportController* printExportCtrl = nullptr, QWidget* parent = nullptr);

    void loadData(const QDate& fromDate = QDate(), const QDate& toDate = QDate());
    void focusTable();

signals:
    void backRequested();
    void alterInvoiceRequested(int targetViewIndex, const QVariantMap& entry);
    void newInvoiceRequested();

private slots:
    void onSearchChanged(const QString& query);
    void onDateFilterChanged();
    void onTableDoubleClicked(int row, int col);
    void onPrintInvoice();
    void onExportPdf();
    void onExportCsv();

private:
    void setupUi();
    void updateSummaryCards();
    void populateTable();

    SalesRegisterController* m_controller = nullptr;
    PrintExportController* m_printExportCtrl = nullptr;

    QDateEdit* m_fromDateEdit = nullptr;
    QDateEdit* m_toDateEdit = nullptr;
    QLineEdit* m_searchEdit = nullptr;

    // Stat Cards
    QLabel* m_totalInvoicesLabel = nullptr;
    QLabel* m_totalBagsLabel = nullptr;
    QLabel* m_totalWeightLabel = nullptr;
    QLabel* m_totalTaxableLabel = nullptr;
    QLabel* m_totalGstLabel = nullptr;
    QLabel* m_totalGrossLabel = nullptr;

    QTableWidget* m_table = nullptr;
};

} // namespace MahadevERP
