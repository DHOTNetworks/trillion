#pragma once

#include <QWidget>
#include <QLineEdit>
#include <QComboBox>
#include <QCheckBox>
#include <QPushButton>
#include <QLabel>
#include "account_search_box.h"
#include "../models/stock_master_controller.h"
#include "../models/stock_items_model.h"

class KbdBadgeButton;

namespace MahadevERP {

class ModifyStockItemWidget : public QWidget {
    Q_OBJECT

public:
    explicit ModifyStockItemWidget(StockMasterController* controller = nullptr, StockItemsModel* stockModel = nullptr, QWidget* parent = nullptr);

    void resetForm();
    void focusSearch();
    void refreshStockGroups();
    void refreshUnits();
    void refreshLedgers();

signals:
    void backRequested();
    void savedSuccess();

private slots:
    void onItemSelected(const QString& itemName);
    void onRecalculateOpening();
    void onUpdateClicked();
    void onDeleteClicked();
    void onCopyAllLedgersFromPurchase();
    void onCreateStockGroup();
    void refreshItemsList();

private:
    void setupUi();
    void setupSearchableCombo(QComboBox* combo, const QStringList& items);
    void setComboText(QComboBox* combo, const QString& text);

    StockMasterController* m_controller = nullptr;
    StockItemsModel* m_stockModel = nullptr;
    int m_currentItemId = -1;

    // Selector
    QComboBox* m_selectItemCombo = nullptr;

    // Controls
    QComboBox* m_goodsTypeCombo = nullptr;
    QComboBox* m_itemTypeCombo = nullptr;

    QLineEdit* m_nameEdit = nullptr;
    QLineEdit* m_codeEdit = nullptr;
    QComboBox* m_stockGroupCombo = nullptr;
    QPushButton* m_createGroupBtn = nullptr;
    QComboBox* m_unitCombo = nullptr;
    QLineEdit* m_companyEdit = nullptr;

    QCheckBox* m_autoAdjustNameCheck = nullptr;
    QCheckBox* m_stockCalculateCheck = nullptr;
    QCheckBox* m_calcInTradingCheck = nullptr;
    QCheckBox* m_capitalGoodsCheck = nullptr;
    QCheckBox* m_taxOnQtyCheck = nullptr;

    QLineEdit* m_purchaseRateEdit = nullptr;
    QLineEdit* m_saleRateEdit = nullptr;
    QLineEdit* m_mrpEdit = nullptr;
    QLineEdit* m_discountEdit = nullptr;
    QLineEdit* m_hsnEdit = nullptr;
    QComboBox* m_gstRateCombo = nullptr;
    QLineEdit* m_cessRateEdit = nullptr;

    QLineEdit* m_packingKgEdit = nullptr;
    QLineEdit* m_openingBagsEdit = nullptr;
    QLineEdit* m_openingQtyEdit = nullptr;
    QLineEdit* m_openingRateEdit = nullptr;
    QLabel* m_openingValueLabel = nullptr;

    AccountSearchBox* m_purchaseLedgerBox = nullptr;
    AccountSearchBox* m_purcReturnLedgerBox = nullptr;
    AccountSearchBox* m_saleLedgerBox = nullptr;
    AccountSearchBox* m_saleReturnLedgerBox = nullptr;
    AccountSearchBox* m_stockLedgerBox = nullptr;

    KbdBadgeButton* m_updateBtn = nullptr;
    KbdBadgeButton* m_deleteBtn = nullptr;
    KbdBadgeButton* m_cancelBtn = nullptr;
};

} // namespace MahadevERP
