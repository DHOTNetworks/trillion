#include "form16a_list_widget.h"
#include "kbd_badge_button.h"
#include "erp_theme.h"
#include "form16a_entry_dialog.h"
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

Form16AListWidget::Form16AListWidget(QWidget *parent)
    : QWidget(parent)
{
    setupUi();
}

void Form16AListWidget::setupUi()
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
    auto *title = new QLabel("Received Forms-16A / 27D Register", headerCard);
    title->setStyleSheet(ErpTheme::headerTitleStyle(16));
    auto *subLabel = new QLabel("Withholding tax certificates received from buyers and clients with TRACES token tracking.", headerCard);
    subLabel->setStyleSheet(ErpTheme::mutedLabelStyle(11));
    titleCol->addWidget(title);
    titleCol->addWidget(subLabel);
    headerLayout->addLayout(titleCol);
    headerLayout->addStretch(1);

    auto *btnNew = new KbdBadgeButton("+ Receive Form-16A", "Ins", headerCard);
    btnNew->setPrimaryColor(ErpTheme::kGreen, ErpTheme::kGreenDark);
    btnNew->setTextColor(ErpTheme::kPaper);
    btnNew->bindShortcut(QKeySequence(Qt::Key_Insert));
    connect(btnNew, &QPushButton::clicked, this, &Form16AListWidget::onNewClicked);
    headerLayout->addWidget(btnNew);

    auto *btnDelete = new KbdBadgeButton("Delete", "Del", headerCard);
    btnDelete->setPrimaryColor(ErpTheme::kRed, QColor("#B91C1C"));
    btnDelete->setTextColor(ErpTheme::kPaper);
    btnDelete->bindShortcut(QKeySequence(Qt::Key_Delete));
    connect(btnDelete, &QPushButton::clicked, this, &Form16AListWidget::onDeleteClicked);
    headerLayout->addWidget(btnDelete);

    auto *btnRefresh = new KbdBadgeButton("Refresh", "F5", headerCard);
    btnRefresh->setButtonColors(QColor("#FFFFFF"), QColor("#F8FAFC"), QColor("#334155"), QColor("#CBD5E1"));
    btnRefresh->bindShortcut(QKeySequence(Qt::Key_F5));
    connect(btnRefresh, &QPushButton::clicked, this, &Form16AListWidget::onRefreshClicked);
    headerLayout->addWidget(btnRefresh);

    auto *btnBack = new KbdBadgeButton("Back", "Esc", headerCard);
    btnBack->setButtonColors(QColor("#F1F5F9"), QColor("#E2E8F0"), QColor("#475569"), QColor("#CBD5E1"));
    connect(btnBack, &QPushButton::clicked, this, &Form16AListWidget::backRequested);
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
    m_searchEdit->setPlaceholderText("Filter by certificate no, customer, quarter...");
    m_searchEdit->setClearButtonEnabled(true);
    connect(m_searchEdit, &QLineEdit::textChanged, this, &Form16AListWidget::onFilterChanged);
    filterLayout->addWidget(m_searchEdit, 1);

    mainLayout->addWidget(filterCard);

    // ========================================================================
    // TIER 3: REGISTER TABLE
    // ========================================================================
    m_table = new QTableWidget(this);
    m_table->setColumnCount(8);
    m_table->setHorizontalHeaderLabels({
        "ID", "Certificate No", "Customer / Deductor", "Quarter", "Receipt Date",
        "Total Paid (₹)", "TDS Deducted (₹)", "Status"
    });
    m_table->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);
    ErpTheme::styleTable(m_table);
    mainLayout->addWidget(m_table, 1);

    // ========================================================================
    // TIER 4: SUMMARY METRICS FOOTER CARDS
    // ========================================================================
    auto *metricsLayout = new QHBoxLayout();
    metricsLayout->setSpacing(10);

        m_totalCertsVal = ErpTheme::makeMetricCard(this, metricsLayout, "TOTAL CERTIFICATES", "0", ErpTheme::kBlue);
    m_totalPaidVal = ErpTheme::makeMetricCard(this, metricsLayout, "TOTAL PAID AMOUNT", "₹ 0.00", ErpTheme::kSky);
    m_totalTdsVal = ErpTheme::makeMetricCard(this, metricsLayout, "TOTAL TDS DEDUCTED", "₹ 0.00", ErpTheme::kGreen);

    mainLayout->addLayout(metricsLayout);

    // Shortcuts
    new QShortcut(QKeySequence(Qt::ALT | Qt::Key_N), this, SLOT(onNewClicked()));
    new QShortcut(QKeySequence(Qt::ALT | Qt::Key_D), this, SLOT(onDeleteClicked()));
    new QShortcut(QKeySequence("Escape"), this, SIGNAL(backRequested()));
    connect(&m_deleteWatcher, &QFutureWatcher<bool>::finished,
            this, &Form16AListWidget::onDeleteFinished);
    connect(&m_queryWatcher, &QFutureWatcher<QVariantList>::finished,
            this, &Form16AListWidget::onQueryReady);
}

void Form16AListWidget::reloadData()
{
    // Isolated read: scan runs on the worker, render lands in onQueryReady.
    m_queryWatcher.setFuture(MahadevERP::DbService::instance().query(
        "SELECT f.id, f.certificate_no, p.name as customer_name, f.quarter, f.receipt_date, "
        "f.gross_amount as total_amount_credited, f.tds_amount as total_tds_deducted, f.matched_with_26as as is_verified "
        "FROM received_forms_16a f "
        "LEFT JOIN parties p ON f.party_id = p.id "
        "ORDER BY f.id DESC;",
        {}));
}

void Form16AListWidget::populateTable()
{
    m_table->setRowCount(0);
    QString query = m_searchEdit ? m_searchEdit->text().trimmed().toLower() : "";

    QVariantList filtered;
    for (const auto &c : m_currentCerts) {
        QVariantMap m = c.toMap();
        if (!query.isEmpty()) {
            QString haystack = QString("%1 %2 %3")
                .arg(m.value("certificate_no").toString())
                .arg(m.value("customer_name").toString())
                .arg(m.value("quarter").toString()).toLower();
            if (!haystack.contains(query)) continue;
        }
        filtered.append(m);
    }

    m_table->setRowCount(filtered.size());
    double totalPaid = 0.0;
    double totalTds = 0.0;

    for (int i = 0; i < filtered.size(); ++i) {
        QVariantMap m = filtered[i].toMap();
        double paid = m.value("total_amount_credited").toDouble();
        double tds = m.value("total_tds_deducted").toDouble();
        totalPaid += paid;
        totalTds += tds;

        m_table->setItem(i, 0, new QTableWidgetItem(m.value("id").toString()));
        m_table->setItem(i, 1, new QTableWidgetItem(m.value("certificate_no").toString()));
        m_table->setItem(i, 2, new QTableWidgetItem(m.value("customer_name").toString()));
        m_table->setItem(i, 3, new QTableWidgetItem(m.value("quarter").toString()));

        QDate dt = QDate::fromString(m.value("receipt_date").toString(), "yyyy-MM-dd");
        m_table->setItem(i, 4, new QTableWidgetItem(dt.isValid() ? dt.toString("dd-MM-yyyy") : FiscalYearHelper::formatDisplayDate(m.value("receipt_date").toString())));

        auto *pd = new QTableWidgetItem(QString::number(paid, 'f', 2));
        pd->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        m_table->setItem(i, 5, pd);

        auto *td = new QTableWidgetItem(QString::number(tds, 'f', 2));
        td->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        td->setForeground(QBrush(QColor("#16A34A")));
        QFont fTd = td->font();
        fTd.setBold(true);
        td->setFont(fTd);
        m_table->setItem(i, 6, td);

        bool verified = m.value("is_verified").toInt() == 1;
        auto *st = new QTableWidgetItem(verified ? "Verified" : "Unverified");
        st->setForeground(QBrush(QColor(verified ? "#16A34A" : "#D97706")));
        QFont fSt = st->font();
        fSt.setBold(true);
        st->setFont(fSt);
        st->setTextAlignment(Qt::AlignCenter);
        m_table->setItem(i, 7, st);
    }

    if (m_totalCertsVal) m_totalCertsVal->setText(QString::number(filtered.size()));
    if (m_totalPaidVal) m_totalPaidVal->setText(QString("₹ %1").arg(QString::number(totalPaid, 'f', 2)));
    if (m_totalTdsVal) m_totalTdsVal->setText(QString("₹ %1").arg(QString::number(totalTds, 'f', 2)));
}

void Form16AListWidget::onNewClicked()
{
    Form16AEntryDialog dlg(this);
    if (dlg.exec() == QDialog::Accepted) {
        reloadData();
    }
}

void Form16AListWidget::onDeleteClicked()
{
    int row = m_table->currentRow();
    if (row < 0) return;

    int id = m_table->item(row, 0)->text().toInt();
    QString no = m_table->item(row, 1)->text();

    int res = QMessageBox::question(
        this, "Delete Certificate",
        QString("Are you sure you want to delete Form-16A / 27D certificate %1?").arg(no),
        QMessageBox::Yes | QMessageBox::No
    );

    if (res == QMessageBox::Yes && !m_deleting) {
        // Isolated delete (was a UI-thread write).
        m_deleting = true;
        m_deleteWatcher.setFuture(MahadevERP::DbService::instance().transact(
            [id](MahadevERP::DbTx& tx) {
                return tx.exec("DELETE FROM received_forms_16a WHERE id = ?;", {id});
            }));
    }
}

void Form16AListWidget::onDeleteFinished() {
    m_deleting = false;
    const bool ok = m_deleteWatcher.future().resultCount() > 0 && m_deleteWatcher.result();
    if (ok) {
        reloadData();
    } else {
        QMessageBox::critical(this, "Delete Failed",
            "Failed to delete certificate (record not found or locked).");
    }
}

void Form16AListWidget::onQueryReady() {
    if (m_queryWatcher.future().resultCount() == 0) return; // keep last-good rows
    m_currentCerts = m_queryWatcher.result();
    populateTable();
    m_isDirty = false;
}

void Form16AListWidget::onRefreshClicked()
{
    reloadData();
}

void Form16AListWidget::onFilterChanged()
{
    populateTable();
}

void Form16AListWidget::focusTable()
{
    if (!m_table) return;
    if (m_table->rowCount() > 0 && m_table->currentRow() < 0) m_table->selectRow(0);
    m_table->setFocus();
}

void Form16AListWidget::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Escape) {
        emit backRequested();
        return;
    }
    QWidget::keyPressEvent(event);
}

} // namespace MahadevERP
