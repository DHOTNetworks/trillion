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
#include "../models/bardana_controller.h"
#include "../services/print_export_controller.h"

class KbdBadgeButton;

class BardanaWidget : public QWidget {
    Q_OBJECT
public:
    explicit BardanaWidget(BardanaController* controller, PrintExportController* printCtrl, QWidget* parent = nullptr);

    void refreshData();
    void markDirty() { m_isDirty = true; }
    bool isDirty() const { return m_isDirty; }

signals:
    void backRequested();

protected:
    void keyPressEvent(QKeyEvent* event) override;

private slots:
    void onSaveTransaction();
    void onFilterChanged();
    void onTabChanged(int index);
    void onDeleteSelected();
    void onExportPdf();
    void onExportCsv();
    void toggleEntryForm();

private:
    void setupUi();
    void applyCustomStyles();
    void populateRegisterTable();
    void populateGodownSummary();
    void populatePartySummary();
    void updateSummaryKpis();

    BardanaController* m_controller = nullptr;
    PrintExportController* m_printCtrl = nullptr;

    // Header & KPIs
    QLabel* m_lblTotalIssued = nullptr;
    QLabel* m_lblTotalReceived = nullptr;
    QLabel* m_lblNetPartyBalance = nullptr;
    QLabel* m_lblGodownStock = nullptr;

    QTabWidget* m_tabWidget = nullptr;
    QFrame* m_entryCard = nullptr;

    // Entry Form Controls
    QComboBox* m_vchTypeCombo = nullptr;
    AccountingDateEdit* m_dateEdit = nullptr;
    QLineEdit* m_vchNoEdit = nullptr;
    AccountSearchBox* m_partySearchBox = nullptr;
    ItemSearchEditor* m_bardanaItemEdit = nullptr;
    QComboBox* m_godownCombo = nullptr;
    QComboBox* m_drCrCombo = nullptr;
    QLineEdit* m_qtyEdit = nullptr;
    QLineEdit* m_rateEdit = nullptr;
    QLineEdit* m_amountEdit = nullptr;
    QLineEdit* m_vehicleEdit = nullptr;
    QLineEdit* m_billNoEdit = nullptr;
    QLineEdit* m_narrationEdit = nullptr;
    QPushButton* m_btnSave = nullptr;

    // Tables
    QTableWidget* m_registerTable = nullptr;
    QTableWidget* m_godownTable = nullptr;
    QTableWidget* m_partyTable = nullptr;

    // Filter Controls
    AccountingDateEdit* m_filterFromDate = nullptr;
    AccountingDateEdit* m_filterToDate = nullptr;
    QComboBox* m_filterTypeCombo = nullptr;
    AccountSearchBox* m_searchPartyBox = nullptr;
    QLineEdit* m_searchGeneralEdit = nullptr;
    bool m_isDirty = true;
};
