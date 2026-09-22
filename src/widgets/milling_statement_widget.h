#pragma once

#include <QWidget>
#include <QTableWidget>
#include <QLabel>
#include <QPushButton>
#include <QLineEdit>
#include <QComboBox>
#include <QDateEdit>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSplitter>
#include "../engine/milling_yield_engine.h"
#include "../models/milling_statement_model.h"
#include "../services/print_export_controller.h"

namespace MahadevERP {

class MillingStatementWidget : public QWidget {
    Q_OBJECT

public:
    explicit MillingStatementWidget(PrintExportController* printExportCtrl = nullptr, QWidget* parent = nullptr);

    void loadMillingData(const QDate& fromDate, const QDate& toDate, const QString& varietyFilter = "All");

signals:
    void backRequested();
    void newBatchRequested();

protected:
    void keyPressEvent(QKeyEvent* event) override;

private slots:
    void onRefreshClicked();
    void onBatchSelectionChanged();
    void onNewBatchClicked();

private:
    void setupUi();
    void applyCustomStyles();
    void populateBatchTable();
    void populateDetailCard(int batchId);

    PrintExportController* m_printExportCtrl = nullptr;

    // Header & Filter
    QDateEdit* m_fromDateEdit = nullptr;
    QDateEdit* m_toDateEdit = nullptr;
    QComboBox* m_varietyCombo = nullptr;
    QLineEdit* m_searchBox = nullptr;
    QPushButton* m_refreshBtn = nullptr;
    QPushButton* m_newBatchBtn = nullptr;
    QPushButton* m_backBtn = nullptr;

    // Summary Metrics Cards
    QLabel* m_totalBatchesLabel = nullptr;
    QLabel* m_totalPaddyInputLabel = nullptr;
    QLabel* m_totalHeadRiceLabel = nullptr;
    QLabel* m_avgYieldLabel = nullptr;
    QLabel* m_totalByProductsLabel = nullptr;

    // Tables
    QTableWidget* m_batchTable = nullptr;
    QTableWidget* m_detailTable = nullptr;

    // Detail Summary Info
    QLabel* m_detailBatchNoLabel = nullptr;
    QLabel* m_detailDateLabel = nullptr;
    QLabel* m_detailVarietyLabel = nullptr;
    QLabel* m_detailYieldLabel = nullptr;
    QLabel* m_detailVarianceLabel = nullptr;

    QVariantList m_rawBatches;
};

} // namespace MahadevERP
