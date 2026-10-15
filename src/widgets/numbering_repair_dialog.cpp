#include "numbering_repair_dialog.h"
#include "erp_theme.h"
#include "../engine/voucher_numbering.h"
#include "../models/vouchers_model.h"
#include "../services/audit_helper.h"
#include "../database_manager.h"
#include "../config/settings_registry.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QMessageBox>
#include <QBrush>
#include <QDebug>

namespace MahadevERP {

NumberingRepairDialog::NumberingRepairDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle("Voucher Numbering Repair");
    setModal(true);
    resize(680, 480);
    setupUi();
    reloadGroups();
}

void NumberingRepairDialog::showRepair(QWidget* parent) {
    NumberingRepairDialog dlg(parent);
    dlg.exec();
}

void NumberingRepairDialog::setupUi() {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(16, 14, 16, 14);
    mainLayout->setSpacing(10);

    auto* title = new QLabel("Duplicate voucher numbers (posted more than once)", this);
    title->setStyleSheet(ErpTheme::headerTitleStyle(15));
    mainLayout->addWidget(title);
    auto* sub = new QLabel(
        "Each group below shares one number. Renumbering keeps the OLDEST posting "
        "and assigns fresh claimed numbers to the extras — amounts untouched, "
        "every renumber written to the audit trail.", this);
    sub->setWordWrap(true);
    sub->setStyleSheet(ErpTheme::mutedLabelStyle(11));
    mainLayout->addWidget(sub);

    m_table = new QTableWidget(this);
    m_table->setColumnCount(4);
    m_table->setHorizontalHeaderLabels({"Type", "Financial Year", "Voucher No", "Postings"});
    m_table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_table->verticalHeader()->setVisible(false);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setAlternatingRowColors(true);
    m_table->setStyleSheet(
        "QTableWidget { background-color: #FFFFFF; alternate-background-color: #F8FAFC; border: 1px solid #CBD5E1; border-radius: 8px; gridline-color: #F1F5F9; font-size: 12.5px; color: #0F172A; }"
        "QTableWidget::item { padding: 6px 12px; }"
        "QHeaderView::section { background-color: #0F172A; color: #FFFFFF; font-weight: 800; font-size: 12px; padding: 8px 12px; border: none;  }");
    mainLayout->addWidget(m_table, 1);

    m_statusLabel = new QLabel(this);
    m_statusLabel->setStyleSheet(ErpTheme::mutedLabelStyle(12));
    mainLayout->addWidget(m_statusLabel);

    auto* bottomLayout = new QHBoxLayout();
    bottomLayout->addStretch();
    m_repairBtn = new KbdBadgeButton("Renumber duplicates", "", this);
    connect(m_repairBtn, &QPushButton::clicked, this, &NumberingRepairDialog::onRenumberClicked);
    bottomLayout->addWidget(m_repairBtn);
    auto* refreshBtn = new KbdBadgeButton("Refresh", "F5", this);
    refreshBtn->bindShortcut(QKeySequence(Qt::Key_F5));
    connect(refreshBtn, &QPushButton::clicked, this, &NumberingRepairDialog::onRefreshClicked);
    bottomLayout->addWidget(refreshBtn);
    auto* closeBtn = new KbdBadgeButton("Close", "Esc", this);
    closeBtn->bindShortcut(QKeySequence(Qt::Key_Escape));
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::accept);
    bottomLayout->addWidget(closeBtn);
    mainLayout->addLayout(bottomLayout);
}

void NumberingRepairDialog::reloadGroups() {
    m_groups = VoucherNumbering::findDuplicateVoucherNos();
    m_table->setRowCount(0);
    m_table->setRowCount(m_groups.size());
    const QColor fg("#0F172A");
    for (int i = 0; i < m_groups.size(); ++i) {
        QVariantMap g = m_groups.at(i).toMap();
        auto cell = [&](const QString& text) {
            auto* it = new QTableWidgetItem(text);
            it->setForeground(QBrush(fg));
            return it;
        };
        m_table->setItem(i, 0, cell(g.value("voucher_type").toString()));
        m_table->setItem(i, 1, cell(g.value("financial_year").toString()));
        m_table->setItem(i, 2, cell(g.value("voucher_no").toString()));
        m_table->setItem(i, 3, cell(g.value("postings").toString()));
    }
    if (m_groups.isEmpty()) {
        m_statusLabel->setText("Numbering is clean — no duplicate voucher numbers.");
        DatabaseManager::instance().setSetting(SettingsKeys::numbering_repair_needed(), "0");
    } else {
        m_statusLabel->setText(QString("%1 duplicate group(s). Oldest posting in each group is kept.").arg(m_groups.size()));
    }
    m_repairBtn->setEnabled(!m_groups.isEmpty());
}

void NumberingRepairDialog::onRefreshClicked() {
    reloadGroups();
}

void NumberingRepairDialog::onRenumberClicked() {
    if (m_groups.isEmpty()) return;
    auto reply = QMessageBox::question(
        this, "Renumber duplicates",
        QString("Renumber the extras in %1 group(s)? Amounts are untouched; "
                "every renumber is audited. This cannot be undone.").arg(m_groups.size()));
    if (reply != QMessageBox::Yes) return;

    auto& db = DatabaseManager::instance();
    VouchersModel claimer;
    int fixed = 0, failed = 0;
    for (const QVariant& gv : m_groups) {
        QVariantMap g = gv.toMap();
        const QString type = g.value("voucher_type").toString();
        const QString fy = g.value("financial_year").toString();
        const QString oldNo = g.value("voucher_no").toString();

        QVariantList rows = db.executeQuery(
            "SELECT id, voucher_date, party_name, amount FROM vouchers "
            "WHERE voucher_type = ? AND financial_year = ? AND voucher_no = ? ORDER BY id ASC;",
            {type, fy, oldNo});
        if (rows.size() < 2) continue; // already resolved by an earlier group pass
        for (int k = 1; k < rows.size(); ++k) {
            QVariantMap r = rows.at(k).toMap();
            // Preserve the number family: cash stays Pymt, bank stays ChPt,
            // cash receipts Rcpt, bank receipts ChRt — never cross-renumber.
            const QString fresh = claimer.claimVoucherNo(
                type, VoucherNumbering::familyTokenFor(type, oldNo), fy);
            if (fresh.isEmpty()) {
                ++failed;
                continue;
            }
            db.beginTransaction();
            bool ok = db.executeNonQuery("UPDATE vouchers SET voucher_no = ? WHERE id = ?;",
                                         {fresh, r.value("id")});
            if (ok) {
                ok = db.executeNonQuery(
                    "UPDATE transactions SET voucher_no = ? WHERE voucher_no = ? AND financial_year = ? "
                    "AND (voucher_type = ? OR trans_type = ?);",
                    {fresh, oldNo, fy, type, type});
            }
            // Keep document projections pointing at the same posting.
            if (ok) {
                db.executeNonQuery("UPDATE sales_invoices SET voucher_no = ? WHERE voucher_no = ? AND financial_year = ?;",
                                   {fresh, oldNo, fy});
                db.executeNonQuery("UPDATE purchase_invoices SET voucher_no = ? WHERE voucher_no = ? AND financial_year = ?;",
                                   {fresh, oldNo, fy});
                db.executeNonQuery("UPDATE stock_transactions SET voucher_no = ? WHERE voucher_no = ? AND financial_year = ?;",
                                   {fresh, oldNo, fy});
            }
            if (ok) {
                db.logAudit("UPDATE", "vouchers", r.value("id").toString(),
                            AuditJson::voucher(oldNo, type, r.value("voucher_date").toString(),
                                               r.value("amount").toDouble(),
                                               r.value("party_name").toString(), fy),
                            AuditJson::voucher(fresh, type, r.value("voucher_date").toString(),
                                               r.value("amount").toDouble(),
                                               r.value("party_name").toString(), fy),
                            fresh, fy);
                db.commit();
                ++fixed;
            } else {
                const QString err = db.lastErrorText();
                db.rollback();
                qWarning() << "[Repair] renumber failed for" << oldNo << ":" << err;
                ++failed;
            }
        }
    }

    reloadGroups();
    QMessageBox::information(this, "Renumber complete",
                             QString("Renumbered %1 posting(s)%2.")
                                 .arg(fixed)
                                 .arg(failed > 0 ? QString(", %1 failed (see logs)").arg(failed) : QString()));
}

} // namespace MahadevERP
