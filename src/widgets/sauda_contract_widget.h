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
#include "../models/sauda_controller.h"
#include "../services/print_export_controller.h"

class KbdBadgeButton;

class SaudaContractWidget : public QWidget {
    Q_OBJECT
public:
    explicit SaudaContractWidget(SaudaController* controller, PrintExportController* printCtrl, QWidget* parent = nullptr);

    void refreshData();

signals:
    void backRequested();

protected:
    void keyPressEvent(QKeyEvent* event) override;

private slots:
    void onSaveSauda();
    void onFilterChanged();
    void onFulfillSaudaSelected();
    void onSettleDalali();
    void onPostJournalVoucher();
    void onExportPdf();
    void onExportCsv();
    void toggleEntryForm();

private:
    void setupUi();
    void applyCustomStyles();
    void populateSaudaTable();
    void populateDalaliTable();
    void updateSummaryKpis();

    SaudaController* m_controller = nullptr;
    PrintExportController* m_printCtrl = nullptr;

    // Header & KPI Metrics
    QLabel* m_lblPendingVolume = nullptr;
    QLabel* m_lblFulfilledVolume = nullptr;
    QLabel* m_lblTotalValue = nullptr;
    QLabel* m_lblUnpostedDalali = nullptr;

    QTabWidget* m_tabWidget = nullptr;
    QFrame* m_entryCard = nullptr;

    // Sauda Entry Controls
    QLineEdit* m_saudaNoEdit = nullptr;
    AccountingDateEdit* m_dateEdit = nullptr;
    QComboBox* m_typeCombo = nullptr;
    AccountSearchBox* m_partySearchBox = nullptr;
    AccountSearchBox* m_brokerSearchBox = nullptr;
    ItemSearchEditor* m_itemEdit = nullptr;
    QLineEdit* m_gradeEdit = nullptr;
    QLineEdit* m_bagsEdit = nullptr;
    QLineEdit* m_weightEdit = nullptr;
    QLineEdit* m_rateEdit = nullptr;
    QLineEdit* m_dalaliRateEdit = nullptr;
    QLineEdit* m_dalaliPctEdit = nullptr;
    QLineEdit* m_deliveryFromEdit = nullptr;
    QLineEdit* m_deliveryToEdit = nullptr;
    QLineEdit* m_notesEdit = nullptr;
    QPushButton* m_btnSaveSauda = nullptr;

    // Tables
    QTableWidget* m_saudaTable = nullptr;
    QTableWidget* m_dalaliTable = nullptr;

    // Filter Controls
    QComboBox* m_filterTypeCombo = nullptr;
    QComboBox* m_filterStatusCombo = nullptr;
    QLineEdit* m_searchSaudaEdit = nullptr;
    AccountSearchBox* m_searchDalaliBox = nullptr;
};
