#pragma once

#include <QWidget>
#include <QTableWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QFrame>
#include "item_search_delegate.h"
#include "../models/manufacturing_batch_controller.h"
#include "../models/manufacturing_model.h"
#include "../services/print_export_controller.h"

namespace MahadevERP {

class ManufacturingVoucherWidget : public QWidget {
    Q_OBJECT

public:
    explicit ManufacturingVoucherWidget(ManufacturingBatchController* batchCtrl = nullptr,
                                  ManufacturingModel* manufacturingModel = nullptr,
                                  PrintExportController* printExportCtrl = nullptr,
                                  QWidget* parent = nullptr);

    void resetForm();
    bool loadBatchForEditing(const QVariant& batchIdOrNo);
    void openDateDialog(bool isInitial = false);
    void saveVoucher() { onSaveClicked(); }

signals:
    void backRequested();
    void batchSaved(const QString& batchNo);

private slots:
    void onRecalculate();
    void onSaveClicked();
    void onAddConsumedRow();
    void onRemoveConsumedRow();
    void onAddProducedRow();
    void onRemoveProducedRow();
    void onConsumedItemConfigured(int row, const QVariantMap& itemData);
    void onProducedItemConfigured(int row, const QVariantMap& itemData);
    void advanceConsumedCell();
    void retreatConsumedCell();
    void advanceProducedCell();
    void retreatProducedCell();

private:
    void setupUi();

    ManufacturingBatchController* m_batchCtrl = nullptr;
    ManufacturingModel* m_manufacturingModel = nullptr;
    PrintExportController* m_printExportCtrl = nullptr;

    int m_editingBatchId = 0;

    // Delegates
    ItemSearchDelegate* m_consumedItemDelegate = nullptr;
    ItemSearchDelegate* m_producedItemDelegate = nullptr;

    // Header Meta
    QLineEdit* m_batchNoEdit = nullptr;
    QLineEdit* m_batchDateEdit = nullptr;
    QLabel* m_dayLabel = nullptr;
    QLineEdit* m_narrationEdit = nullptr;

    // Tables
    QTableWidget* m_consumedTable = nullptr;
    QTableWidget* m_producedTable = nullptr;

    // Summary Metric Cards
    QLabel* m_totalInputWeightLabel = nullptr;
    QLabel* m_totalOutputWeightLabel = nullptr;
    QLabel* m_yieldPctLabel = nullptr;
    QLabel* m_shortageLabel = nullptr;
};

} // namespace MahadevERP
