#pragma once

#include <QWidget>
#include <QTableWidget>
#include <QLineEdit>
#include <QComboBox>
#include <QLabel>
#include <QTextEdit>
#include <QVariantList>
#include "kbd_badge_button.h"

namespace MahadevERP {

// Read-only audit trail register (Tally Edit-Log parity): every voucher
// INSERT/UPDATE/DELETE and every refused (BLOCKED) write, newest first.
// Fully keyboard-driven: Esc back, F5 refresh, Ctrl+F search, arrows browse.
class AuditRegisterWidget : public QWidget {
    Q_OBJECT

public:
    explicit AuditRegisterWidget(QWidget* parent = nullptr);

    void reloadData();
    void markDirty() { m_isDirty = true; }
    bool isDirty() const { return m_isDirty; }
    void focusTable();

signals:
    void backRequested();

private slots:
    void onRefreshClicked();
    void onFilterChanged();
    void onCurrentRowChanged(int currentRow);

private:
    void setupUi();
    void populateTable();
    static QString summarize(const QVariantMap& row);

    QLineEdit* m_searchEdit = nullptr;
    QComboBox* m_actionCombo = nullptr;
    QTableWidget* m_table = nullptr;
    QTextEdit* m_detailView = nullptr;
    QLabel* m_totalVal = nullptr;
    QLabel* m_blockedVal = nullptr;
    QLabel* m_tablesVal = nullptr;

    QVariantList m_rows;
    bool m_isDirty = true;
};

} // namespace MahadevERP
