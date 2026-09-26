#pragma once

#include <QWidget>
#include <QTableWidget>
#include <QLineEdit>
#include <QComboBox>
#include <QLabel>
#include <QPushButton>
#include <QFrame>
#include <QVariantMap>
#include <QVariantList>
#include "account_search_box.h"
#include "accounting_date_edit.h"
#include "../models/vouchers_model.h"
#include "../services/print_export_controller.h"

namespace MahadevERP {

class CashVoucherWidget : public QWidget {
    Q_OBJECT

public:
    explicit CashVoucherWidget(PrintExportController* printExportCtrl = nullptr,
                               QWidget* parent = nullptr);
    ~CashVoucherWidget() override = default;

    QString voucherMode() const { return m_voucherMode; }
    void setVoucherMode(const QString& mode);
    void setVoucherType(const QString& type) { setVoucherMode(type); }

    bool isEditMode() const { return m_editingVoucherId > 0; }
    int editingVoucherId() const { return m_editingVoucherId; }

    void resetForm();
    bool loadVoucherForEditing(const QVariant& vchNoOrId, const QString& dateHint = "");
    void setWorkingDate(const QString& dateStr);

public slots:
    void openDateDialog(bool isInitial = false);
    void saveVoucher();
    void deleteVoucher();
    void addNewRow(const QString& drcr = "", const QString& ledger = "", double debit = 0.0, double credit = 0.0, const QString& ref = "");
    void removeRow(int row);
    void recalculateTotals();
    void focusFirstRow();

signals:
    void backRequested();
    void voucherSaved(const QString& voucherNo);
    void voucherDeleted(const QString& voucherNo);

protected:
    void showEvent(QShowEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    bool eventFilter(QObject* watched, QEvent* event) override;

private slots:
    void onDateChanged(const QDate& date);
    void onTableRowChanged();

private:
    void setupUi();
    void applyCustomStyles();
    void updateFiscalYearAndVoucherNo();
    void updateDayOfWeek(const QDate& date);
    void setStatusMessage(const QString& message, bool isError);
    void setupRowWidgets(int row, const QString& drcr, const QString& ledger, double debit, double credit, const QString& ref);
    void focusCell(int row, int col);

    VouchersModel m_vouchersModel;
    PrintExportController* m_printExportCtrl = nullptr;

    QString m_voucherMode = "Payment"; // "Payment" (Pymt) or "Receipt" (Rcpt)
    int m_editingVoucherId = 0;
    QString m_editingVoucherNo;

    // Header Controls
    QLabel* m_modeBadge = nullptr;
    QPushButton* m_paymentModeBtn = nullptr;
    QPushButton* m_receiptModeBtn = nullptr;
    QLabel* m_voucherNoLabel = nullptr;
    QLineEdit* m_voucherNoInput = nullptr;
    AccountingDateEdit* m_dateEdit = nullptr;
    QLabel* m_dayOfWeekLabel = nullptr;
    QLabel* m_fyBadge = nullptr;
    QLabel* m_alterBadge = nullptr;

    // Grid Table
    QTableWidget* m_table = nullptr;

    // Bottom Section
    QLineEdit* m_narrationInput = nullptr;
    QLabel* m_totalDebitLabel = nullptr;
    QLabel* m_totalCreditLabel = nullptr;
    QLabel* m_balanceBadge = nullptr;
    QLabel* m_statusLabel = nullptr;

    // Action Buttons
    QPushButton* m_dateBtn = nullptr;
    QPushButton* m_addRowBtn = nullptr;
    QPushButton* m_saveBtn = nullptr;
    QPushButton* m_deleteBtn = nullptr;
    QPushButton* m_backBtn = nullptr;
};

} // namespace MahadevERP
