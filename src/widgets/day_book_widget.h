#pragma once

#include <QWidget>
#include <QTableWidget>
#include <QTimer>
#include <QFutureWatcher>
#include <QScrollBar>
#include "accounting_date_edit.h"
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include "../services/print_export_controller.h"

namespace MahadevERP {

class DayBookWidget : public QWidget {
    Q_OBJECT

public:
    explicit DayBookWidget(PrintExportController* printExportCtrl = nullptr, QWidget* parent = nullptr);

    void loadDayBookData(const QDate& fromDate, const QDate& toDate, const QString& voucherTypeFilter = "ALL");
    void markDirty() { m_isDirty = true; }
    bool isDirty() const { return m_isDirty; }
    void focusTable();

signals:
    void backRequested();
    void alterVoucherRequested(int targetViewIndex, const QVariantMap& entry);
    void entriesChanged();

protected:
    void keyPressEvent(QKeyEvent* event) override;
    void showEvent(QShowEvent* event) override;

private slots:
    void onRefreshClicked();
    void onDayBookQueryReady();
    void onRowDoubleClicked(int row, int column);
    void deleteFocusedRow();
    void onExportPdfClicked();
    void onExportOdfClicked();
    void onExportExcelClicked();
    void onPrintClicked();
    void onDateChanged();

private:
    void setupUi();
    void applyCustomStyles();

    AccountingDateDisplay* m_fromDateEdit = nullptr;
    AccountingDateDisplay* m_toDateEdit = nullptr;
    QComboBox* m_typeFilterCombo = nullptr;
    QLineEdit* m_searchBox = nullptr;
    QPushButton* m_refreshBtn = nullptr;
    QPushButton* m_pdfBtn = nullptr;
    QPushButton* m_backBtn = nullptr;

    QTableWidget* m_table = nullptr;
    QLabel* m_totalDrLabel = nullptr;
    QLabel* m_totalCrLabel = nullptr;
    QLabel* m_rowCountLabel = nullptr;

    PrintExportController* m_printExportCtrl = nullptr;
    QVariantList m_records;
    bool m_isDirty = true;
    QTimer* m_searchDebounce = nullptr;
    // Isolated read flight: the worker thread owns the SQL, the UI thread
    // only renders watcher results. QFutureWatcher auto-discards superseded
    // flights (setFuture unwatches the old future), so debounced keystrokes
    // can never render out of order.
    QFutureWatcher<QVariantList> m_queryWatcher;
};

} // namespace MahadevERP
