#include "advance_payment_194q_list_widget.h"
#include "kbd_badge_button.h"
#include "erp_theme.h"
#include "../engine/fiscal_year_helper.h"
#include "../database_manager.h"
#include "../services/db_service.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QDate>
#include <QShortcut>
#include <QMessageBox>
#include <QKeyEvent>

namespace MahadevERP {

AdvancePayment194QListWidget::AdvancePayment194QListWidget(QWidget *parent)
    : QWidget(parent)
{
    setupUi();
}

void AdvancePayment194QListWidget::setupUi()
{
    auto *mainLayout = ErpTheme::makePageLayout(this);

    // ========================================================================
    // TIER 1: STICKY HEADER CARD
    // ========================================================================
    auto *headerCard = new QFrame(this);
    headerCard->setStyleSheet(ErpTheme::headerCardStyle());
    auto *headerLayout = new QHBoxLayout(headerCard);
    headerLayout->setContentsMargins(14, 10, 14, 10);

    auto *titleCol = new QVBoxLayout();
    titleCol->setSpacing(2);
    auto *title = new QLabel("Advance Payments U/S 194-Q Register", headerCard);
    title->setStyleSheet(ErpTheme::headerTitleStyle(16));
    auto *subLabel = new QLabel("Purchase advance payments recorded with 0.1% tax deduction at source.", headerCard);
    subLabel->setStyleSheet(ErpTheme::mutedLabelStyle(11));
    titleCol->addWidget(title);
    titleCol->addWidget(subLabel);
    headerLayout->addLayout(titleCol);
    headerLayout->addStretch(1);

    auto *btnNew = new KbdBadgeButton("+ New Advance", "Ins", headerCard);
    btnNew->setPrimaryColor(ErpTheme::kGreen, ErpTheme::kGreenDark);
    btnNew->setTextColor(ErpTheme::kPaper);
    btnNew->bindShortcut(QKeySequence(Qt::Key_Insert));
    connect(btnNew, &QPushButton::clicked, this, &AdvancePayment194QListWidget::onNewClicked);
    headerLayout->addWidget(btnNew);

    auto *btnDelete = new KbdBadgeButton("Delete", "Del", headerCard);
    btnDelete->setPrimaryColor(ErpTheme::kRed, QColor("#B91C1C"));
    btnDelete->setTextColor(ErpTheme::kPaper);
    btnDelete->bindShortcut(QKeySequence(Qt::Key_Delete));
    connect(btnDelete, &QPushButton::clicked, this, &AdvancePayment194QListWidget::onDeleteClicked);
    headerLayout->addWidget(btnDelete);

    auto *btnRefresh = new KbdBadgeButton("Refresh", "F5", headerCard);
    btnRefresh->setButtonColors(QColor("#FFFFFF"), QColor("#F8FAFC"), QColor("#334155"), QColor("#CBD5E1"));
    btnRefresh->bindShortcut(QKeySequence(Qt::Key_F5));
    connect(btnRefresh, &QPushButton::clicked, this, &AdvancePayment194QListWidget::onRefreshClicked);
    headerLayout->addWidget(btnRefresh);

    auto *btnBack = new KbdBadgeButton("Back", "Esc", headerCard);
    btnBack->setButtonColors(QColor("#F1F5F9"), QColor("#E2E8F0"), QColor("#475569"), QColor("#CBD5E1"));
    connect(btnBack, &QPushButton::clicked, this, &AdvancePayment194QListWidget::backRequested);
    headerLayout->addWidget(btnBack);

    mainLayout->addWidget(headerCard);

    // ========================================================================
    // TIER 2: SEARCH TOOLBAR
    // ========================================================================
    auto *filterCard = new QFrame(this);
    filterCard->setStyleSheet(ErpTheme::filterBarStyle());
    auto *filterLayout = new QHBoxLayout(filterCard);
    filterLayout->setContentsMargins(12, 8, 12, 8);
    filterLayout->setSpacing(12);

    filterLayout->addWidget(new QLabel("Search:", filterCard));
    m_searchEdit = new QLineEdit(filterCard);
    m_searchEdit->setPlaceholderText("Filter by voucher no, supplier name, bank...");
    m_searchEdit->setClearButtonEnabled(true);
    connect(m_searchEdit, &QLineEdit::textChanged, this, &AdvancePayment194QListWidget::onFilterChanged);
    filterLayout->addWidget(m_searchEdit, 1);

    mainLayout->addWidget(filterCard);

    // ========================================================================
    // TIER 3: REGISTER TABLE
    // ========================================================================
    m_table = new QTableWidget(this);
    m_table->setColumnCount(9);
    m_table->setHorizontalHeaderLabels({
        "ID", "Voucher No", "Date", "Supplier Name", "Bank A/c",
        "Gross Advance (₹)", "TDS Rate (%)", "TDS Amount (₹)", "Net Paid (₹)"
    });
    m_table->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Stretch);
    ErpTheme::styleTable(m_table);
    mainLayout->addWidget(m_table, 1);

    // ========================================================================
    // TIER 4: SUMMARY METRICS FOOTER CARDS
    // ========================================================================
    auto *metricsLayout = new QHBoxLayout();
    metricsLayout->setSpacing(10);

        m_totalVouchersVal = ErpTheme::makeMetricCard(this, metricsLayout, "TOTAL VOUCHERS", "0", ErpTheme::kBlue);
    m_totalGrossVal = ErpTheme::makeMetricCard(this, metricsLayout, "TOTAL GROSS ADVANCE", "₹ 0.00", ErpTheme::kSky);
    m_totalTaxVal = ErpTheme::makeMetricCard(this, metricsLayout, "TOTAL TDS U/S 194-Q", "₹ 0.00", ErpTheme::kAmber);
    m_totalNetVal = ErpTheme::makeMetricCard(this, metricsLayout, "TOTAL NET PAID", "₹ 0.00", ErpTheme::kGreen);

    mainLayout->addLayout(metricsLayout);

    // Shortcuts
    new QShortcut(QKeySequence(Qt::ALT | Qt::Key_N), this, SLOT(onNewClicked()));
    new QShortcut(QKeySequence(Qt::ALT | Qt::Key_D), this, SLOT(onDeleteClicked()));
    new QShortcut(QKeySequence("Escape"), this, SIGNAL(backRequested()));
    connect(&m_deleteWatcher, &QFutureWatcher<bool>::finished,
            this, &AdvancePayment194QListWidget::onDeleteFinished);
    connect(&m_queryWatcher, &QFutureWatcher<QVariantList>::finished,
            this, &AdvancePayment194QListWidget::onQueryReady);
}

void AdvancePayment194QListWidget::reloadData()
{
    // Isolated read: scan runs on the worker, render lands in onQueryReady.
    m_queryWatcher.setFuture(MahadevERP::DbService::instance().query(
        "SELECT a.id, a.voucher_no, a.voucher_date, p.name as supplier_name, b.name as bank_name, "
        "a.gross_advance_amount, a.tds_rate, a.tds_amount, a.net_payment_amount "
        "FROM advance_payments_194q a "
        "LEFT JOIN parties p ON a.supplier_id = p.id "
        "LEFT JOIN parties b ON a.bank_ledger_id = b.id "
        "ORDER BY a.voucher_no DESC;",
        {}));
}

void AdvancePayment194QListWidget::populateTable()
{
    m_table->setRowCount(0);
    QString query = m_searchEdit ? m_searchEdit->text().trimmed().toLower() : "";

    QVariantList filtered;
    for (const auto &v : m_currentVouchers) {
        QVariantMap m = v.toMap();
        if (!query.isEmpty()) {
            QString haystack = QString("%1 %2 %3")
                .arg(m.value("voucher_no").toString())
                .arg(m.value("supplier_name").toString())
                .arg(m.value("bank_name").toString()).toLower();
            if (!haystack.contains(query)) continue;
        }
        filtered.append(m);
    }

    m_table->setRowCount(filtered.size());
    double totalGross = 0.0;
    double totalTax = 0.0;
    double totalNet = 0.0;

    for (int i = 0; i < filtered.size(); ++i) {
        QVariantMap m = filtered[i].toMap();
        double gross = m.value("gross_advance_amount").toDouble();
        double tax = m.value("tds_amount").toDouble();
        double net = m.value("net_payment_amount").toDouble();
        totalGross += gross;
        totalTax += tax;
        totalNet += net;

        m_table->setItem(i, 0, new QTableWidgetItem(m.value("id").toString()));
        m_table->setItem(i, 1, new QTableWidgetItem(QString("#%1").arg(m.value("voucher_no").toInt())));

        QDate dt = QDate::fromString(m.value("voucher_date").toString(), "yyyy-MM-dd");
        m_table->setItem(i, 2, new QTableWidgetItem(dt.isValid() ? dt.toString("dd-MM-yyyy") : FiscalYearHelper::formatDisplayDate(m.value("voucher_date").toString())));

        m_table->setItem(i, 3, new QTableWidgetItem(m.value("supplier_name").toString()));
        m_table->setItem(i, 4, new QTableWidgetItem(m.value("bank_name").toString()));

        auto *gr = new QTableWidgetItem(QString::number(gross, 'f', 2));
        gr->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        m_table->setItem(i, 5, gr);

        auto *rt = new QTableWidgetItem(QString::number(m.value("tds_rate").toDouble(), 'f', 3));
        rt->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        m_table->setItem(i, 6, rt);

        auto *tx = new QTableWidgetItem(QString::number(tax, 'f', 2));
        tx->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        tx->setForeground(QBrush(QColor("#D97706")));
        QFont fTx = tx->font();
        fTx.setBold(true);
        tx->setFont(fTx);
        m_table->setItem(i, 7, tx);

        auto *nt = new QTableWidgetItem(QString::number(net, 'f', 2));
        nt->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        nt->setForeground(QBrush(QColor("#16A34A")));
        QFont fNt = nt->font();
        fNt.setBold(true);
        nt->setFont(fNt);
        m_table->setItem(i, 8, nt);
    }

    if (m_totalVouchersVal) m_totalVouchersVal->setText(QString::number(filtered.size()));
    if (m_totalGrossVal) m_totalGrossVal->setText(QString("₹ %1").arg(QString::number(totalGross, 'f', 2)));
    if (m_totalTaxVal) m_totalTaxVal->setText(QString("₹ %1").arg(QString::number(totalTax, 'f', 2)));
    if (m_totalNetVal) m_totalNetVal->setText(QString("₹ %1").arg(QString::number(totalNet, 'f', 2)));
}

void AdvancePayment194QListWidget::onNewClicked()
{
    emit newVoucherRequested();
}

void AdvancePayment194QListWidget::onDeleteClicked()
{
    int row = m_table->currentRow();
    if (row < 0) return;

    int id = m_table->item(row, 0)->text().toInt();
    QString no = m_table->item(row, 1)->text();

    int res = QMessageBox::question(
        this, "Delete Voucher",
        QString("Are you sure you want to delete Advance Payment Voucher %1?").arg(no),
        QMessageBox::Yes | QMessageBox::No
    );

    if (res == QMessageBox::Yes && !m_deleting) {
        // Isolated delete: voucher legs + advance row clear atomically
        // on the worker (was a UI-thread transaction).
        m_deleting = true;
        m_deleteWatcher.setFuture(MahadevERP::DbService::instance().transact(
            [id](MahadevERP::DbTx& tx) {
                const QVariant vchId = tx.scalar(
                    "SELECT voucher_id FROM advance_payments_194q WHERE id = ?;", {id});
                if (vchId.isValid() && !vchId.isNull() && vchId.toInt() > 0) {
                    const QString vchNo = tx.scalar(
                        "SELECT voucher_no FROM vouchers WHERE id = ?;", {vchId.toInt()}).toString();
                    tx.execUnchecked("DELETE FROM transactions WHERE voucher_no = ?;", {vchNo});
                    tx.execUnchecked("DELETE FROM vouchers WHERE id = ?;", {vchId.toInt()});
                }
                return tx.exec("DELETE FROM advance_payments_194q WHERE id = ?;", {id});
            }));
    }
}

void AdvancePayment194QListWidget::onDeleteFinished() {
    m_deleting = false;
    const bool ok = m_deleteWatcher.future().resultCount() > 0 && m_deleteWatcher.result();
    if (ok) {
        reloadData();
    } else {
        QMessageBox::critical(this, "Delete Failed",
            "Failed to delete advance voucher (record not found or locked).");
    }
}

void AdvancePayment194QListWidget::onQueryReady() {
    if (m_queryWatcher.future().resultCount() == 0) return; // keep last-good rows
    m_currentVouchers = m_queryWatcher.result();
    populateTable();
    m_isDirty = false;
}

void AdvancePayment194QListWidget::onRefreshClicked()
{
    reloadData();
}

void AdvancePayment194QListWidget::onFilterChanged()
{
    populateTable();
}

void AdvancePayment194QListWidget::focusTable()
{
    if (!m_table) return;
    if (m_table->rowCount() > 0 && m_table->currentRow() < 0) m_table->selectRow(0);
    m_table->setFocus();
}

void AdvancePayment194QListWidget::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Escape) {
        emit backRequested();
        return;
    }
    QWidget::keyPressEvent(event);
}

} // namespace MahadevERP
