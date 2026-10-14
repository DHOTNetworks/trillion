#include "tds_vouchers_list_widget.h"
#include <QFutureWatcher>
#include "kbd_badge_button.h"
#include "erp_theme.h"
#include "../engine/fiscal_year_helper.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QDate>
#include <QShortcut>
#include <QMessageBox>
#include <QKeyEvent>

namespace MahadevERP {

TdsVouchersListWidget::TdsVouchersListWidget(TdsVoucherController *controller, QWidget *parent)
    : QWidget(parent)
    , m_controller(controller)
{
    setupUi();
}

void TdsVouchersListWidget::setupUi()
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
    auto *title = new QLabel("TDS Deduction Vouchers Register", headerCard);
    title->setStyleSheet(ErpTheme::headerTitleStyle(16));
    auto *subLabel = new QLabel("Tax withheld at source on rent, contractors, commission, professional fees, and salary.", headerCard);
    subLabel->setStyleSheet(ErpTheme::mutedLabelStyle(11));
    titleCol->addWidget(title);
    titleCol->addWidget(subLabel);
    headerLayout->addLayout(titleCol);
    headerLayout->addStretch(1);

    auto *btnNew = new KbdBadgeButton("+ New Voucher", "Ins", headerCard);
    btnNew->setPrimaryColor(ErpTheme::kGreen, ErpTheme::kGreenDark);
    btnNew->setTextColor(ErpTheme::kPaper);
    btnNew->bindShortcut(QKeySequence(Qt::Key_Insert));
    connect(btnNew, &QPushButton::clicked, this, &TdsVouchersListWidget::onNewClicked);
    headerLayout->addWidget(btnNew);

    auto *btnEdit = new KbdBadgeButton("Edit", "Enter", headerCard);
    btnEdit->setButtonColors(QColor("#FFFFFF"), QColor("#F8FAFC"), QColor("#334155"), QColor("#CBD5E1"));
    connect(btnEdit, &QPushButton::clicked, this, &TdsVouchersListWidget::onEditClicked);
    headerLayout->addWidget(btnEdit);

    auto *btnDelete = new KbdBadgeButton("Delete", "Del", headerCard);
    btnDelete->setPrimaryColor(ErpTheme::kRed, QColor("#B91C1C"));
    btnDelete->setTextColor(ErpTheme::kPaper);
    btnDelete->bindShortcut(QKeySequence(Qt::Key_Delete));
    connect(btnDelete, &QPushButton::clicked, this, &TdsVouchersListWidget::onDeleteClicked);
    headerLayout->addWidget(btnDelete);

    auto *btnRefresh = new KbdBadgeButton("Refresh", "F5", headerCard);
    btnRefresh->setButtonColors(QColor("#FFFFFF"), QColor("#F8FAFC"), QColor("#334155"), QColor("#CBD5E1"));
    btnRefresh->bindShortcut(QKeySequence(Qt::Key_F5));
    connect(btnRefresh, &QPushButton::clicked, this, &TdsVouchersListWidget::onRefreshClicked);
    headerLayout->addWidget(btnRefresh);

    auto *btnBack = new KbdBadgeButton("Back", "Esc", headerCard);
    btnBack->setButtonColors(QColor("#F1F5F9"), QColor("#E2E8F0"), QColor("#475569"), QColor("#CBD5E1"));
    connect(btnBack, &QPushButton::clicked, this, &TdsVouchersListWidget::backRequested);
    headerLayout->addWidget(btnBack);

    mainLayout->addWidget(headerCard);

    // ========================================================================
    // TIER 2: SEARCH & FILTER TOOLBAR
    // ========================================================================
    auto *filterCard = new QFrame(this);
    filterCard->setStyleSheet(ErpTheme::filterBarStyle());
    auto *filterLayout = new QHBoxLayout(filterCard);
    filterLayout->setContentsMargins(12, 8, 12, 8);
    filterLayout->setSpacing(12);

    filterLayout->addWidget(new QLabel("Search:", filterCard));
    m_searchEdit = new QLineEdit(filterCard);
    m_searchEdit->setPlaceholderText("Filter by voucher no, party name, nature...");
    m_searchEdit->setClearButtonEnabled(true);
    connect(m_searchEdit, &QLineEdit::textChanged, this, &TdsVouchersListWidget::onFilterChanged);
    filterLayout->addWidget(m_searchEdit, 1);

    filterLayout->addWidget(new QLabel("Nature / Section:", filterCard));
    m_typeFilterCombo = new QComboBox(filterCard);
    m_typeFilterCombo->addItems({"ALL", "RENT", "CONTRACTOR", "COMMISSION", "PROFESSIONAL", "SALARY"});
    connect(m_typeFilterCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &TdsVouchersListWidget::onFilterChanged);
    filterLayout->addWidget(m_typeFilterCombo);

    mainLayout->addWidget(filterCard);

    // ========================================================================
    // TIER 3: REGISTER TABLE
    // ========================================================================
    m_table = new QTableWidget(this);
    m_table->setColumnCount(10);
    m_table->setHorizontalHeaderLabels({
        "ID", "Voucher No", "Date", "TDS Section", "Party / Deductee",
        "Gross Value (₹)", "TDS Rate (%)", "Tax Deducted (₹)", "Net Paid (₹)", "Status"
    });
    m_table->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(4, QHeaderView::Stretch);
    ErpTheme::styleTable(m_table);
    connect(m_table, &QTableWidget::cellDoubleClicked, this, [this](int row, int) {
        if (row >= 0 && row < m_table->rowCount()) {
            int id = m_table->item(row, 0)->text().toInt();
            emit editVoucherRequested(id);
        }
    });
    // Enter alters the focused voucher (same as double-click), scoped to grid.
    auto openFocusedRow = [this]() { onEditClicked(); };
    auto* enterShortcut = new QShortcut(QKeySequence(Qt::Key_Return), m_table);
    enterShortcut->setContext(Qt::WidgetWithChildrenShortcut);
    connect(enterShortcut, &QShortcut::activated, this, openFocusedRow);
    auto* numEnterShortcut = new QShortcut(QKeySequence(Qt::Key_Enter), m_table);
    numEnterShortcut->setContext(Qt::WidgetWithChildrenShortcut);
    connect(numEnterShortcut, &QShortcut::activated, this, openFocusedRow);
    mainLayout->addWidget(m_table, 1);

    // ========================================================================
    // TIER 4: SUMMARY METRICS FOOTER CARDS
    // ========================================================================
    auto *metricsLayout = new QHBoxLayout();
    metricsLayout->setSpacing(10);

        m_totalVouchersVal = ErpTheme::makeMetricCard(this, metricsLayout, "TOTAL VOUCHERS", "0", ErpTheme::kBlue);
    m_totalGrossVal = ErpTheme::makeMetricCard(this, metricsLayout, "TOTAL GROSS", "₹ 0.00", ErpTheme::kSky);
    m_totalTaxVal = ErpTheme::makeMetricCard(this, metricsLayout, "TOTAL TDS DEDUCTED", "₹ 0.00", ErpTheme::kAmber);
    m_totalNetVal = ErpTheme::makeMetricCard(this, metricsLayout, "NET PAYABLE / PAID", "₹ 0.00", ErpTheme::kGreen);

    mainLayout->addLayout(metricsLayout);

    // Shortcuts
    new QShortcut(QKeySequence(Qt::ALT | Qt::Key_S), this, SLOT(onNewClicked()));
    new QShortcut(QKeySequence(Qt::ALT | Qt::Key_F5), this, SLOT(onRefreshClicked()));
    new QShortcut(QKeySequence(Qt::ALT | Qt::Key_D), this, SLOT(onDeleteClicked()));
    new QShortcut(QKeySequence(Qt::Key_Escape), this, SIGNAL(backRequested()));
    connect(&m_deleteWatcher, &QFutureWatcher<bool>::finished,
            this, &TdsVouchersListWidget::onDeleteFinished);
    connect(&m_queryWatcher, &QFutureWatcher<QVariantList>::finished,
            this, &TdsVouchersListWidget::onQueryReady);
}

void TdsVouchersListWidget::reloadData()
{
    // Isolated read: scan runs on the worker, render lands in onQueryReady.
    m_queryWatcher.setFuture(m_controller->getTdsVouchersListAsync());
}

void TdsVouchersListWidget::populateTable()
{
    m_table->setRowCount(0);
    QString typeFilter = m_typeFilterCombo ? m_typeFilterCombo->currentText() : "ALL";
    QString query = m_searchEdit ? m_searchEdit->text().trimmed().toLower() : "";

    QVariantList filtered;
    for (const auto &v : m_currentVouchers) {
        QVariantMap m = v.toMap();
        if (typeFilter != "ALL" && m.value("tds_type").toString().compare(typeFilter, Qt::CaseInsensitive) != 0) {
            continue;
        }
        if (!query.isEmpty()) {
            QString haystack = QString("%1 %2 %3")
                .arg(m.value("voucher_no").toString())
                .arg(m.value("ledger_name").toString())
                .arg(m.value("tds_type").toString()).toLower();
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
        double gross = m.value("total_for_tds").toDouble();
        double tax = m.value("total_tax_amount").toDouble();
        double net = m.value("net_amount").toDouble();
        totalGross += gross;
        totalTax += tax;
        totalNet += net;

        m_table->setItem(i, 0, new QTableWidgetItem(m.value("id").toString()));
        m_table->setItem(i, 1, new QTableWidgetItem(QString("#%1").arg(m.value("voucher_no").toInt())));

        QDate dt = QDate::fromString(m.value("voucher_date").toString(), "yyyy-MM-dd");
        m_table->setItem(i, 2, new QTableWidgetItem(dt.isValid() ? dt.toString("dd-MM-yyyy") : FiscalYearHelper::formatDisplayDate(m.value("voucher_date").toString())));

        m_table->setItem(i, 3, new QTableWidgetItem(m.value("tds_type").toString()));
        m_table->setItem(i, 4, new QTableWidgetItem(m.value("ledger_name").toString()));

        auto *gr = new QTableWidgetItem(QString::number(gross, 'f', 2));
        gr->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        m_table->setItem(i, 5, gr);

        auto *rt = new QTableWidgetItem(QString::number(m.value("rate_tds").toDouble(), 'f', 2));
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

        bool deposited = m.value("is_deposited").toInt() == 1;
        auto *st = new QTableWidgetItem(deposited ? "Deposited" : "Pending");
        st->setForeground(QBrush(QColor(deposited ? "#16A34A" : "#D97706")));
        QFont fSt = st->font();
        fSt.setBold(true);
        st->setFont(fSt);
        st->setTextAlignment(Qt::AlignCenter);
        m_table->setItem(i, 9, st);
    }

    if (m_totalVouchersVal) m_totalVouchersVal->setText(QString::number(filtered.size()));
    if (m_totalGrossVal) m_totalGrossVal->setText(QString("₹ %1").arg(QString::number(totalGross, 'f', 2)));
    if (m_totalTaxVal) m_totalTaxVal->setText(QString("₹ %1").arg(QString::number(totalTax, 'f', 2)));
    if (m_totalNetVal) m_totalNetVal->setText(QString("₹ %1").arg(QString::number(totalNet, 'f', 2)));
}

void TdsVouchersListWidget::onNewClicked()
{
    emit newVoucherRequested();
}

void TdsVouchersListWidget::focusTable()
{
    if (!m_table) return;
    if (m_table->rowCount() > 0 && m_table->currentRow() < 0) m_table->selectRow(0);
    m_table->setFocus();
}

void TdsVouchersListWidget::onEditClicked()
{
    int row = m_table->currentRow();
    if (row >= 0 && row < m_table->rowCount()) {
        int id = m_table->item(row, 0)->text().toInt();
        emit editVoucherRequested(id);
    }
}

void TdsVouchersListWidget::onDeleteClicked()
{
    int row = m_table->currentRow();
    if (row < 0) return;

    int id = m_table->item(row, 0)->text().toInt();
    QString no = m_table->item(row, 1)->text();

    int res = QMessageBox::question(
        this, "Delete Voucher",
        QString("Are you sure you want to delete TDS Voucher %1?").arg(no),
        QMessageBox::Yes | QMessageBox::No
    );

    if (res == QMessageBox::Yes && !m_deleting) {
        // Isolated delete: postings clear on the worker, list reloads after.
        m_deleting = true;
        m_deleteWatcher.setFuture(m_controller->deleteVoucherAsync(id));
    }
}

void TdsVouchersListWidget::onDeleteFinished() {
    m_deleting = false;
    const bool ok = m_deleteWatcher.future().resultCount() > 0 && m_deleteWatcher.result();
    if (ok) {
        reloadData();
    } else {
        QMessageBox::critical(this, "Delete Failed",
            "Failed to delete TDS Voucher (record not found or locked).");
    }
}

void TdsVouchersListWidget::onQueryReady() {
    if (m_queryWatcher.future().resultCount() == 0) return; // keep last-good rows
    m_currentVouchers = m_queryWatcher.result();
    populateTable();
    m_isDirty = false;
}

void TdsVouchersListWidget::onRefreshClicked()
{
    reloadData();
}

void TdsVouchersListWidget::onFilterChanged()
{
    populateTable();
}

void TdsVouchersListWidget::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Escape) {
        emit backRequested();
        return;
    }
    QWidget::keyPressEvent(event);
}

} // namespace MahadevERP
