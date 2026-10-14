#include "tcs_receipt_list_widget.h"
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

TcsReceiptListWidget::TcsReceiptListWidget(TcsReceiptVoucherController *controller, QWidget *parent)
    : QWidget(parent)
    , m_controller(controller)
{
    setupUi();
}

void TcsReceiptListWidget::setupUi()
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
    auto *title = new QLabel("TCS Receipt Vouchers Register", headerCard);
    title->setStyleSheet(ErpTheme::headerTitleStyle(16));
    auto *subLabel = new QLabel("Customer payment receipts subject to Section 206C(1H) collection and tax ledger credit.", headerCard);
    subLabel->setStyleSheet(ErpTheme::mutedLabelStyle(11));
    titleCol->addWidget(title);
    titleCol->addWidget(subLabel);
    headerLayout->addLayout(titleCol);
    headerLayout->addStretch(1);

    auto *btnNew = new KbdBadgeButton("+ New Receipt", "Ins", headerCard);
    btnNew->setPrimaryColor(ErpTheme::kGreen, ErpTheme::kGreenDark);
    btnNew->setTextColor(ErpTheme::kPaper);
    btnNew->bindShortcut(QKeySequence(Qt::Key_Insert));
    connect(btnNew, &QPushButton::clicked, this, &TcsReceiptListWidget::onNewClicked);
    headerLayout->addWidget(btnNew);

    auto *btnDelete = new KbdBadgeButton("Delete", "Del", headerCard);
    btnDelete->setPrimaryColor(ErpTheme::kRed, QColor("#B91C1C"));
    btnDelete->setTextColor(ErpTheme::kPaper);
    btnDelete->bindShortcut(QKeySequence(Qt::Key_Delete));
    connect(btnDelete, &QPushButton::clicked, this, &TcsReceiptListWidget::onDeleteClicked);
    headerLayout->addWidget(btnDelete);

    auto *btnRefresh = new KbdBadgeButton("Refresh", "F5", headerCard);
    btnRefresh->setButtonColors(QColor("#FFFFFF"), QColor("#F8FAFC"), QColor("#334155"), QColor("#CBD5E1"));
    btnRefresh->bindShortcut(QKeySequence(Qt::Key_F5));
    connect(btnRefresh, &QPushButton::clicked, this, &TcsReceiptListWidget::onRefreshClicked);
    headerLayout->addWidget(btnRefresh);

    auto *btnBack = new KbdBadgeButton("Back", "Esc", headerCard);
    btnBack->setButtonColors(QColor("#F1F5F9"), QColor("#E2E8F0"), QColor("#475569"), QColor("#CBD5E1"));
    connect(btnBack, &QPushButton::clicked, this, &TcsReceiptListWidget::backRequested);
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
    m_searchEdit->setPlaceholderText("Filter by receipt no, customer name, bank...");
    m_searchEdit->setClearButtonEnabled(true);
    connect(m_searchEdit, &QLineEdit::textChanged, this, &TcsReceiptListWidget::onFilterChanged);
    filterLayout->addWidget(m_searchEdit, 1);

    mainLayout->addWidget(filterCard);

    // ========================================================================
    // TIER 3: REGISTER TABLE
    // ========================================================================
    m_table = new QTableWidget(this);
    m_table->setColumnCount(9);
    m_table->setHorizontalHeaderLabels({
        "ID", "Receipt No", "Date", "Customer Name", "Bank A/c",
        "Bank Receipt (₹)", "TCS Rate (%)", "TCS Amount (₹)", "Net Credit Party (₹)"
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
    m_totalBankReceiptVal = ErpTheme::makeMetricCard(this, metricsLayout, "TOTAL BANK RECEIPT", "₹ 0.00", ErpTheme::kSky);
    m_totalTcsVal = ErpTheme::makeMetricCard(this, metricsLayout, "TOTAL TCS COLLECTED", "₹ 0.00", ErpTheme::kAmber);
    m_totalCreditPartyVal = ErpTheme::makeMetricCard(this, metricsLayout, "TOTAL PARTY CREDIT", "₹ 0.00", ErpTheme::kGreen);

    mainLayout->addLayout(metricsLayout);

    // Shortcuts
    new QShortcut(QKeySequence(Qt::ALT | Qt::Key_F5), this, SLOT(onRefreshClicked()));
    new QShortcut(QKeySequence(Qt::ALT | Qt::Key_D), this, SLOT(onDeleteClicked()));
    new QShortcut(QKeySequence(Qt::Key_Escape), this, SIGNAL(backRequested()));
}

void TcsReceiptListWidget::reloadData()
{
    m_currentReceipts = m_controller->getReceiptVouchersList();
    populateTable();
    m_isDirty = false;
}

void TcsReceiptListWidget::populateTable()
{
    m_table->setRowCount(0);
    QString query = m_searchEdit ? m_searchEdit->text().trimmed().toLower() : "";

    QVariantList filtered;
    for (const auto &r : m_currentReceipts) {
        QVariantMap m = r.toMap();
        if (!query.isEmpty()) {
            QString haystack = QString("%1 %2 %3")
                .arg(m.value("receipt_no").toString())
                .arg(m.value("party_name").toString())
                .arg(m.value("bank_name").toString()).toLower();
            if (!haystack.contains(query)) continue;
        }
        filtered.append(m);
    }

    m_table->setRowCount(filtered.size());
    double totalBank = 0.0;
    double totalTcs = 0.0;
    double totalParty = 0.0;

    for (int i = 0; i < filtered.size(); ++i) {
        QVariantMap m = filtered[i].toMap();
        double bankAmt = m.value("bank_amount").toDouble();
        double tcsAmt = m.value("tcs_amount").toDouble();
        double partyAmt = m.value("net_credit_party").toDouble();
        totalBank += bankAmt;
        totalTcs += tcsAmt;
        totalParty += partyAmt;

        m_table->setItem(i, 0, new QTableWidgetItem(m.value("id").toString()));
        m_table->setItem(i, 1, new QTableWidgetItem(QString("#%1").arg(m.value("receipt_no").toInt())));

        QDate dt = QDate::fromString(m.value("receipt_date").toString(), "yyyy-MM-dd");
        m_table->setItem(i, 2, new QTableWidgetItem(dt.isValid() ? dt.toString("dd-MM-yyyy") : FiscalYearHelper::formatDisplayDate(m.value("receipt_date").toString())));

        m_table->setItem(i, 3, new QTableWidgetItem(m.value("party_name").toString()));
        m_table->setItem(i, 4, new QTableWidgetItem(m.value("bank_name").toString()));

        auto *bk = new QTableWidgetItem(QString::number(bankAmt, 'f', 2));
        bk->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        m_table->setItem(i, 5, bk);

        auto *rt = new QTableWidgetItem(QString::number(m.value("tcs_rate").toDouble(), 'f', 3));
        rt->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        m_table->setItem(i, 6, rt);

        auto *tx = new QTableWidgetItem(QString::number(tcsAmt, 'f', 2));
        tx->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        tx->setForeground(QBrush(QColor("#D97706")));
        QFont fTx = tx->font();
        fTx.setBold(true);
        tx->setFont(fTx);
        m_table->setItem(i, 7, tx);

        auto *cr = new QTableWidgetItem(QString::number(partyAmt, 'f', 2));
        cr->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        cr->setForeground(QBrush(QColor("#16A34A")));
        QFont fCr = cr->font();
        fCr.setBold(true);
        cr->setFont(fCr);
        m_table->setItem(i, 8, cr);
    }

    if (m_totalVouchersVal) m_totalVouchersVal->setText(QString::number(filtered.size()));
    if (m_totalBankReceiptVal) m_totalBankReceiptVal->setText(QString("₹ %1").arg(QString::number(totalBank, 'f', 2)));
    if (m_totalTcsVal) m_totalTcsVal->setText(QString("₹ %1").arg(QString::number(totalTcs, 'f', 2)));
    if (m_totalCreditPartyVal) m_totalCreditPartyVal->setText(QString("₹ %1").arg(QString::number(totalParty, 'f', 2)));
}

void TcsReceiptListWidget::onNewClicked()
{
    emit newReceiptRequested();
}

void TcsReceiptListWidget::onDeleteClicked()
{
    int row = m_table->currentRow();
    if (row < 0) return;

    int id = m_table->item(row, 0)->text().toInt();
    QString no = m_table->item(row, 1)->text();

    int res = QMessageBox::question(
        this, "Delete Voucher",
        QString("Are you sure you want to delete TCS Receipt Voucher %1?").arg(no),
        QMessageBox::Yes | QMessageBox::No
    );

    if (res == QMessageBox::Yes) {
        m_controller->deleteVoucher(id);
        reloadData();
    }
}

void TcsReceiptListWidget::onRefreshClicked()
{
    reloadData();
}

void TcsReceiptListWidget::onFilterChanged()
{
    populateTable();
}

void TcsReceiptListWidget::focusTable()
{
    if (!m_table) return;
    if (m_table->rowCount() > 0 && m_table->currentRow() < 0) m_table->selectRow(0);
    m_table->setFocus();
}

void TcsReceiptListWidget::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Escape) {
        emit backRequested();
        return;
    }
    QWidget::keyPressEvent(event);
}

} // namespace MahadevERP
