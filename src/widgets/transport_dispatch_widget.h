#pragma once

#include <QWidget>
#include <QTableWidget>
#include <QLineEdit>
#include <QDateEdit>
#include <QPushButton>
#include <QLabel>
#include "../models/transport_dispatch_controller.h"
#include "../services/print_export_controller.h"

namespace MahadevERP {

class TransportDispatchWidget : public QWidget {
    Q_OBJECT

public:
    explicit TransportDispatchWidget(TransportDispatchController* controller = nullptr,
                                    PrintExportController* printExportCtrl = nullptr,
                                    QWidget* parent = nullptr);

    void loadData(const QDate& fromDate = QDate(), const QDate& toDate = QDate());
    void focusTable();

signals:
    void backRequested();
    void newDispatchRequested();

public slots:
    void openNewDispatchDialog();

private slots:
    void onSearchChanged(const QString& query);
    void onDateFilterChanged();
    void onTableDoubleClicked(int row, int col);
    void onExportCsv();

private:
    void setupUi();
    void populateTable();

    TransportDispatchController* m_controller = nullptr;
    PrintExportController* m_printExportCtrl = nullptr;

    QDateEdit* m_fromDateEdit = nullptr;
    QDateEdit* m_toDateEdit = nullptr;
    QLineEdit* m_searchEdit = nullptr;

    // Stat Cards
    QLabel* m_totalDispatchesLabel = nullptr;
    QLabel* m_totalNetWeightLabel = nullptr;
    QLabel* m_totalFreightLabel = nullptr;
    QLabel* m_totalBalancePayableLabel = nullptr;

    QTableWidget* m_table = nullptr;
};

} // namespace MahadevERP
