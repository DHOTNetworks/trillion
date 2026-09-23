#pragma once

#include <QWidget>
#include <QTableWidget>
#include <QLineEdit>
#include <QDateEdit>
#include <QPushButton>
#include <QLabel>
#include "../models/paddy_arrivals_model.h"
#include "../models/paddy_procurement_controller.h"
#include "../services/print_export_controller.h"

namespace MahadevERP {

class PaddyProcurementWidget : public QWidget {
    Q_OBJECT

public:
    explicit PaddyProcurementWidget(PaddyArrivalsModel* arrivalsModel = nullptr,
                                    PaddyProcurementController* procurementCtrl = nullptr,
                                    PrintExportController* printExportCtrl = nullptr,
                                    QWidget* parent = nullptr);

    void loadArrivals(const QDate& fromDate = QDate(), const QDate& toDate = QDate());
    void focusTable();

signals:
    void backRequested();
    void newArrivalRequested();

public slots:
    void openNewArrivalDialog();

private slots:
    void onSearchChanged(const QString& query);
    void onDateFilterChanged();
    void onTableDoubleClicked(int row, int col);
    void onPrintRegister();
    void onExportCsv();

private:
    void setupUi();
    void populateTable();

    PaddyArrivalsModel* m_arrivalsModel = nullptr;
    PaddyProcurementController* m_procurementCtrl = nullptr;
    PrintExportController* m_printExportCtrl = nullptr;

    QDateEdit* m_fromDateEdit = nullptr;
    QDateEdit* m_toDateEdit = nullptr;
    QLineEdit* m_searchEdit = nullptr;

    // Stat Cards
    QLabel* m_totalSlipsLabel = nullptr;
    QLabel* m_totalBagsLabel = nullptr;
    QLabel* m_totalGrossLabel = nullptr;
    QLabel* m_totalDeductLabel = nullptr;
    QLabel* m_totalNetLabel = nullptr;
    QLabel* m_totalAmountLabel = nullptr;

    QTableWidget* m_table = nullptr;
};

} // namespace MahadevERP
