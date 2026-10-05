#pragma once

#include <QWidget>
#include <QTableWidget>
#include <QLineEdit>
#include <QComboBox>
#include <QPushButton>
#include <QLabel>
#include <QTabWidget>
#include <QFrame>
#include "accounting_date_edit.h"
#include "account_search_box.h"
#include "item_search_delegate.h"
#include "../models/gate_register_controller.h"
#include "../services/print_export_controller.h"

class KbdBadgeButton;

class GateRegisterWidget : public QWidget {
    Q_OBJECT
public:
    explicit GateRegisterWidget(GateRegisterController* controller, PrintExportController* printCtrl, QWidget* parent = nullptr);

    void refreshData();
    void markDirty() { m_isDirty = true; }
    bool isDirty() const { return m_isDirty; }

signals:
    void backRequested();
    void openWeighbridgeRequested(const QString& gatePassNo, const QString& vehicleNo, const QString& partyName);

protected:
    void keyPressEvent(QKeyEvent* event) override;

private slots:
    void onSaveGateEntry();
    void onFilterChanged();
    void onStatusUpdate(const QString& newStatus);
    void onSendToWeighbridge();
    void onExportPdf();
    void onExportCsv();
    void toggleEntryForm();

private:
    void setupUi();
    void applyCustomStyles();
    void populateLiveTable();
    void populateHistoryTable();
    void updateSummaryKpis();

    GateRegisterController* m_controller = nullptr;
    PrintExportController* m_printCtrl = nullptr;

    // Header & KPI Metrics
    QLabel* m_lblActiveAtGate = nullptr;
    QLabel* m_lblInwardToday = nullptr;
    QLabel* m_lblOutwardToday = nullptr;
    QLabel* m_lblTotalWeight = nullptr;

    QTabWidget* m_tabWidget = nullptr;
    QFrame* m_entryCard = nullptr;

    // Entry Form Controls
    QLineEdit* m_gatePassEdit = nullptr;
    AccountingDateEdit* m_dateEdit = nullptr;
    QLineEdit* m_timeEdit = nullptr;
    QComboBox* m_directionCombo = nullptr;
    QComboBox* m_purposeCombo = nullptr;
    QLineEdit* m_vehicleEdit = nullptr;
    QLineEdit* m_driverNameEdit = nullptr;
    QLineEdit* m_driverPhoneEdit = nullptr;
    AccountSearchBox* m_transporterSearchBox = nullptr;
    AccountSearchBox* m_partySearchBox = nullptr;
    ItemSearchEditor* m_commodityEdit = nullptr;
    QLineEdit* m_bagCountEdit = nullptr;
    QLineEdit* m_grossWtEdit = nullptr;
    QLineEdit* m_remarksEdit = nullptr;
    QPushButton* m_btnSave = nullptr;

    // Tables
    QTableWidget* m_liveTable = nullptr;
    QTableWidget* m_historyTable = nullptr;

    // Filter Controls
    AccountingDateEdit* m_histFromDate = nullptr;
    AccountingDateEdit* m_histToDate = nullptr;
    QComboBox* m_histDirectionCombo = nullptr;
    QLineEdit* m_searchLiveEdit = nullptr;
    QLineEdit* m_searchHistEdit = nullptr;
    bool m_isDirty = true;
};
