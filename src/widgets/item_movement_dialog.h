#pragma once

#include <QDialog>
#include <QTableWidget>
#include <QLabel>
#include <QPushButton>
#include <QDateEdit>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSplitter>
#include <QHeaderView>
#include "kbd_badge_button.h"
#include "custom_dialogs.h"
#include "../services/print_export_controller.h"

namespace MahadevERP {

class ItemMovementDialog : public QDialog {
    Q_OBJECT

public:
    explicit ItemMovementDialog(const QString& itemName, const QDate& fromDate, const QDate& toDate,
                                PrintExportController* printCtrl = nullptr, QWidget* parent = nullptr);

    void loadMovements(const QString& itemName, const QDate& fromDate, const QDate& toDate);

signals:
    void openInvoiceRequested(const QString& refNo, const QString& invType);

protected:
    void keyPressEvent(QKeyEvent* event) override;

private slots:
    void onRefreshClicked();
    void onExportPdfClicked();
    void onExportCsvClicked();
    void onInwardDoubleClicked(int row, int col);
    void onOutwardDoubleClicked(int row, int col);

private:
    void setupUi();
    void applyCustomStyles();
    void updateSummaryTotals();
    QWidget* createMetricCard(const QString& title, QLabel*& valueLabel, const QString& accentColor);

    QString m_itemName;
    PrintExportController* m_printCtrl = nullptr;

    QDateEdit* m_fromDateEdit = nullptr;
    QDateEdit* m_toDateEdit = nullptr;
    QLabel* m_itemTitleLabel = nullptr;

    QTableWidget* m_inwardTable = nullptr;
    QTableWidget* m_outwardTable = nullptr;

    QLabel* m_lblInwardBags = nullptr;
    QLabel* m_lblInwardWeight = nullptr;
    QLabel* m_lblInwardVal = nullptr;

    QLabel* m_lblOutwardBags = nullptr;
    QLabel* m_lblOutwardWeight = nullptr;
    QLabel* m_lblOutwardVal = nullptr;

    QLabel* m_lblClosingBags = nullptr;
    QLabel* m_lblClosingWeight = nullptr;
    QLabel* m_lblClosingVal = nullptr;

    KbdBadgeButton* m_btnRefresh = nullptr;
    KbdBadgeButton* m_btnExportPdf = nullptr;
    KbdBadgeButton* m_btnExportCsv = nullptr;
    KbdBadgeButton* m_btnClose = nullptr;
};

} // namespace MahadevERP
