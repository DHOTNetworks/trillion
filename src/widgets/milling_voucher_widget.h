#pragma once

#include <QWidget>
#include <QTableWidget>
#include <QLineEdit>
#include <QDateEdit>
#include <QPushButton>
#include <QLabel>
#include "../models/milling_batch_controller.h"
#include "../models/milling_model.h"
#include "../services/print_export_controller.h"

namespace MahadevERP {

class MillingVoucherWidget : public QWidget {
    Q_OBJECT

public:
    explicit MillingVoucherWidget(MillingBatchController* batchCtrl = nullptr,
                                  MillingModel* millingModel = nullptr,
                                  PrintExportController* printExportCtrl = nullptr,
                                  QWidget* parent = nullptr);

    void resetForm();
    bool loadBatchForEditing(const QVariant& batchIdOrNo);
    void openDateDialog(bool isInitial = false);

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

private:
    void setupUi();

    MillingBatchController* m_batchCtrl = nullptr;
    MillingModel* m_millingModel = nullptr;
    PrintExportController* m_printExportCtrl = nullptr;

    int m_editingBatchId = 0;

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
