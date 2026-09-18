#pragma once

#include <QWidget>
#include <QTableWidget>
#include <QLineEdit>
#include <QComboBox>
#include <QLabel>
#include <QPushButton>
#include <QRadioButton>
#include <QButtonGroup>
#include <QCheckBox>
#include <QGroupBox>
#include <QFrame>
#include "accounting_date_edit.h"
#include "party_search_widget.h"
#include "item_search_delegate.h"
#include "../models/sales_voucher_controller.h"
#include "../models/sales_model.h"
#include "../models/parties_model.h"
#include "../models/stock_items_model.h"
#include "../services/print_export_controller.h"

class SalesVoucherWidget : public QWidget {
    Q_OBJECT

public:
    explicit SalesVoucherWidget(PrintExportController* printExportCtrl = nullptr,
                                QWidget* parent = nullptr);
    ~SalesVoucherWidget() override;

    bool isEditMode() const { return m_editingInvoiceId > 0; }
    int editingInvoiceId() const { return m_editingInvoiceId; }

    void resetForm();
    bool loadInvoiceForEditing(const QVariant& invNoOrId, const QString& dateHint = "");
    void setWorkingDate(const QString& dateStr);

public slots:
    void openDateDialog();
    void openAlterVoucherDialog();
    void loadPreviousVoucher();
    void loadNextVoucher();
    void saveVoucher();
    void deleteVoucher();
    void printInvoice();
    void exportPdf();
    void addNewLineRow();
    void removeLineRow(int row);
    void recalculateTotals();
    void advanceCell();
    void retreatCell();
    void moveCell(int row, int col);
    void focusTableAt(int row, int col);

signals:
    void backRequested();
    void invoiceSaved(const QString& invoiceNo);
    void invoiceDeleted(const QString& invoiceNo);

protected:
    void showEvent(QShowEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    bool eventFilter(QObject* watched, QEvent* event) override;

private slots:
    void onDateChanged(const QDate& date);
    void onPartySelected(const QVariantMap& partyData);
    void onMarketTypeChanged(int index);
    void onTaxStatusChanged();
    void onSaleStatusChanged();
    void onMarketFeeStatusChanged();
    void onTableCellChanged(int row, int column);
    void onStockItemConfigured(int row, const QVariantMap& itemData);

private:
    void setupUi();
    void setupNavigationChains();
    void applyCustomStyles();
    void updateNextNumbers();
    void updateFiscalYearBadge();
    void updateDayOfWeek(const QDate& date);
    void populateRow(int row, const QString& itemName, const QString& grade, int bags, double packing, double weight, double rate, double amount, double gstPct);
    void syncModelFromTable();
    void syncTableFromModel();
    void setStatusMessage(const QString& message, bool isError);

    SalesVoucherController m_controller;
    SalesModel m_salesModel;
    PartiesModel m_partiesModel;
    StockItemsModel m_stockModel;
    PrintExportController* m_printExportCtrl = nullptr;

    int m_editingInvoiceId = 0;
    QString m_editingInvoiceNo;
    QString m_editingVoucherNo;
    bool m_hasInitialDateOpened = false;

    // Top Bar Controls
    QLabel* m_saleBadge = nullptr;
    QLabel* m_voucherNoDisplay = nullptr;
    QLabel* m_titleHeaderLabel = nullptr;
    AccountingDateEdit* m_invoiceDateEdit = nullptr;
    QLabel* m_dayOfWeekLabel = nullptr;
    QCheckBox* m_revChargeCheck = nullptr;
    QLabel* m_helpHintLabel = nullptr;
    QLabel* m_statusBanner = nullptr;

    // Row 1: Meta Controls
    QComboBox* m_marketTypeCombo = nullptr;
    QLineEdit* m_invoiceNoEdit = nullptr;
    QLineEdit* m_dueDaysEdit = nullptr;

    // Tax Status Radio Group
    QGroupBox* m_taxStatusGroupBox = nullptr;
    QRadioButton* m_taxGstRadio = nullptr;
    QRadioButton* m_taxIgstRadio = nullptr;
    QRadioButton* m_taxExportRadio = nullptr;
    QButtonGroup* m_taxButtonGroup = nullptr;
    QString m_selectedTaxStatus = "GST / Exempt";

    // Mandi Sale Status Radio Group (Mandi Type)
    QGroupBox* m_saleStatusGroupBox = nullptr;
    QRadioButton* m_saleSelfRadio = nullptr;
    QRadioButton* m_saleTransferRadio = nullptr;
    QRadioButton* m_saleLagatRadio = nullptr;
    QRadioButton* m_saleThirdPartyRadio = nullptr;
    QButtonGroup* m_saleButtonGroup = nullptr;
    QString m_selectedSaleStatus = "Self Sale";

    // Mandi Market Fee Status Radio Group (Mandi Type)
    QGroupBox* m_marketFeeStatusGroupBox = nullptr;
    QRadioButton* m_feePayableRadio = nullptr;
    QRadioButton* m_feePaidRadio = nullptr;
    QButtonGroup* m_feeButtonGroup = nullptr;
    QString m_selectedMarketFeeStatus = "Paid";

    // Row 2: Party Controls
    QLabel* m_partyTagLabel = nullptr;
    PartySearchWidget* m_partySearchWidget = nullptr;
    QLabel* m_partyBalLabel = nullptr;
    QLineEdit* m_gstinDisplay = nullptr;
    QComboBox* m_posCombo = nullptr;

    // Line Items Table
    QCheckBox* m_inclTaxCheck = nullptr;
    QTableWidget* m_tableWidget = nullptr;
    ItemSearchDelegate* m_itemDelegate = nullptr;
    bool m_isUpdatingTable = false;

    // Table Summary Bar
    QLabel* m_totalBagsLabel = nullptr;
    QLabel* m_totalWeightLabel = nullptr;
    QLabel* m_subtotalTaxableLabel = nullptr;

    // Mandi Expenses Grid Frame
    QFrame* m_mandiExpensesFrame = nullptr;
    QLineEdit* m_damiEdit = nullptr;
    QLineEdit* m_labourEdit = nullptr;
    QLineEdit* m_auctionEdit = nullptr;
    QLineEdit* m_mFeeEdit = nullptr;
    QLineEdit* m_hrdfEdit = nullptr;
    QLineEdit* m_mandiOtherExpEdit = nullptr;
    QLineEdit* m_welfareEdit = nullptr;
    QLineEdit* m_dhrmdEdit = nullptr;
    QLineEdit* m_sutliEdit = nullptr;
    QLineEdit* m_mandiLessEdit = nullptr;

    // Logistics 4x4 Grid
    QLineEdit* m_vehicleNoEdit = nullptr;
    QLineEdit* m_grNoEdit = nullptr;
    QLineEdit* m_driverNameEdit = nullptr;
    QLineEdit* m_ewayBillNoEdit = nullptr;
    QLineEdit* m_billTimeEdit = nullptr;
    QLineEdit* m_saudaDateEdit = nullptr;
    QPushButton* m_shippingAddressBtn = nullptr;
    QLineEdit* m_shippingAddressEdit = nullptr;
    QLineEdit* m_poNoEdit = nullptr;
    QLineEdit* m_gradeEdit = nullptr;
    QLineEdit* m_transportEdit = nullptr;
    QLineEdit* m_challanNoEdit = nullptr;
    QLineEdit* m_kandaWeightEdit = nullptr;
    QLineEdit* m_brokerEdit = nullptr;

    // Financial Summary Panel (Right)
    QFrame* m_summaryFrame = nullptr;
    QLabel* m_mandiTotalTitleLabel = nullptr;
    QLabel* m_mandiTotalValLabel = nullptr;
    QLabel* m_commissionTitleLabel = nullptr;
    QLineEdit* m_commissionEdit = nullptr;
    QLabel* m_taxTitleLabel = nullptr;
    QLineEdit* m_taxAmountEdit = nullptr;
    QLabel* m_otherExpTitleLabel = nullptr;
    QLineEdit* m_otherExpEdit = nullptr;
    QLabel* m_lessTitleLabel = nullptr;
    QLineEdit* m_lessAmountEdit = nullptr;
    QLabel* m_freightTitleLabel = nullptr;
    QLineEdit* m_freightChargesEdit = nullptr;
    QLabel* m_roundOffTitleLabel = nullptr;
    QLabel* m_roundOffLabel = nullptr;
    QLabel* m_tcsTitleLabel = nullptr;
    QLabel* m_tcsAmountLabel = nullptr;
    QLineEdit* m_tcsRateEdit = nullptr;

    // Footer Bar
    QLineEdit* m_narrationEdit = nullptr;
    QLabel* m_grandTotalLabel = nullptr;

    // Actions
    QPushButton* m_saveBtn = nullptr;
    QPushButton* m_deleteBtn = nullptr;
    QPushButton* m_printBtn = nullptr;
    QPushButton* m_pdfBtn = nullptr;
    QPushButton* m_newBtn = nullptr;
    QPushButton* m_backBtn = nullptr;
};
