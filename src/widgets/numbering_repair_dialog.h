#pragma once

#include <QDialog>
#include <QTableWidget>
#include <QLabel>
#include <QVariantList>
#include "kbd_badge_button.h"

namespace MahadevERP {

// Numbering repair: lists every (voucher_type, financial_year, voucher_no)
// posted more than once (legacy double-posts that predate the UNIQUE guard)
// and renumbers the extras to fresh claimed numbers — money preserved,
// identity fixed, every renumber audited. Clears the
// numbering_repair_needed flag when clean.
class NumberingRepairDialog : public QDialog {
    Q_OBJECT

public:
    explicit NumberingRepairDialog(QWidget* parent = nullptr);

    static void showRepair(QWidget* parent = nullptr);

private slots:
    void onRenumberClicked();
    void onRefreshClicked();

private:
    void setupUi();
    void reloadGroups();

    QTableWidget* m_table = nullptr;
    QLabel* m_statusLabel = nullptr;
    KbdBadgeButton* m_repairBtn = nullptr;
    QVariantList m_groups;
};

} // namespace MahadevERP
