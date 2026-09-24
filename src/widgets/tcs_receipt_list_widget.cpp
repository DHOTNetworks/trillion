#include "tcs_receipt_list_widget.h"
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
    reloadData();
}

void TcsReceiptListWidget::setupUi()
{
    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(14, 12, 14, 12);
    mainLayout->setSpacing(10);

    // ========================================================================
    // TIER 1: STICKY HEADER CARD
    // ========================================================================
    auto *headerCard = new QFrame(this);
    headerCard->setStyleSheet("QFrame { background-color: #FFFFFF; border: 1px solid #E2E8F0; border-radius: 8px; }");
    auto *headerLayout = new QHBoxLayout(headerCard);
    headerLayout->setContentsMargins(14, 10, 14, 10);

    auto *titleCol = new QVBoxLayout();
    titleCol->setSpacing(2);
    auto *title = new QLabel("TCS Receipt Vouchers Register", headerCard);
    title->setStyleSheet("font-size: 16px; font-weight: 800; color: #0F172A; border: none; background: transparent;");
    auto *subLabel = new QLabel("Customer payment receipts subject to Section 206C(1H) collection and tax ledger credit.", headerCard);
    subLabel->setStyleSheet("font-size: 11px; color: #64748B; border: none; background: transparent;");
    titleCol->addWidget(title);
    titleCol->addWidget(subLabel);
    headerLayout->addLayout(titleCol);
    headerLayout->addStretch(1);

    auto *btnNew = new QPushButton("[Ins] + New Receipt", headerCard);
    btnNew->setCursor(Qt::PointingHandCursor);
    btnNew->setStyleSheet(
        "QPushButton { background-color: #16A34A; color: #FFFFFF; border: none; border-radius: 6px; padding: 6px 16px; font-weight: 800; font-size: 12px; }"
        "QPushButton:hover { background-color: #15803D; }"
    );
    connect(btnNew, &QPushButton::clicked, this, &TcsReceiptListWidget::onNewClicked);
    headerLayout->addWidget(btnNew);

    auto *btnDelete = new QPushButton("[Del] Delete", headerCard);
    btnDelete->setCursor(Qt::PointingHandCursor);
    btnDelete->setStyleSheet(
        "QPushButton { background-color: #DC2626; color: #FFFFFF; border: none; border-radius: 6px; padding: 6px 14px; font-weight: 700; font-size: 12px; }"
        "QPushButton:hover { background-color: #B91C1C; }"
    );
    connect(btnDelete, &QPushButton::clicked, this, &TcsReceiptListWidget::onDeleteClicked);
    headerLayout->addWidget(btnDelete);

    auto *btnRefresh = new QPushButton("[F5] Refresh", headerCard);
    btnRefresh->setCursor(Qt::PointingHandCursor);
    btnRefresh->setStyleSheet(
        "QPushButton { background-color: #FFFFFF; border: 1.5px solid #CBD5E1; padding: 6px 12px; border-radius: 6px; font-weight: 700; font-size: 11.5px; color: #334155; }"
        "QPushButton:hover { background-color: #F8FAFC; }"
    );
    connect(btnRefresh, &QPushButton::clicked, this, &TcsReceiptListWidget::onRefreshClicked);
    headerLayout->addWidget(btnRefresh);

    auto *btnBack = new QPushButton("← [Esc] Back", headerCard);
    btnBack->setCursor(Qt::PointingHandCursor);
    btnBack->setStyleSheet(
        "QPushButton { background-color: #F1F5F9; border: 1px solid #CBD5E1; border-radius: 6px; padding: 6px 12px; font-weight: 700; color: #475569; font-size: 11.5px; }"
        "QPushButton:hover { background-color: #E2E8F0; }"
    );
    connect(btnBack, &QPushButton::clicked, this, &TcsReceiptListWidget::backRequested);
    headerLayout->addWidget(btnBack);

    mainLayout->addWidget(headerCard);

    // ========================================================================
    // TIER 2: SEARCH & FILTER TOOLBAR
    // ========================================================================
    auto *filterCard = new QFrame(this);
    filterCard->setStyleSheet(
        "QFrame { background-color: #FFFFFF; border: 1px solid #CBD5E1; border-radius: 8px; }"
        "QLabel { color: #475569; font-weight: 700; font-size: 11.5px; border: none; background: transparent; }"
        "QLineEdit { background-color: #FFFFFF; border: 1.5px solid #CBD5E1; border-radius: 6px; padding: 4px 8px; font-size: 12px; color: #0F172A; font-weight: 600; }"
        "QLineEdit:focus { border-color: #2563EB; background-color: #F8FAFC; }"
    );
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
    m_table->verticalHeader()->setVisible(false);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setAlternatingRowColors(true);
    m_table->setStyleSheet(
        "QTableWidget { background-color: #FFFFFF; border: 1px solid #CBD5E1; border-radius: 8px; gridline-color: #F1F5F9; font-size: 12px; color: #0F172A; }"
        "QTableWidget::item { padding: 6px 10px; }"
        "QTableWidget::item:selected { background-color: #EFF6FF; color: #1E3A8A; font-weight: bold; }"
        "QHeaderView::section { background-color: #0F172A; color: #FFFFFF; font-weight: 800; font-size: 11px; padding: 6px 10px; border: none; }"
    );
    mainLayout->addWidget(m_table, 1);

    // ========================================================================
    // TIER 4: SUMMARY METRICS FOOTER CARDS
    // ========================================================================
    auto *metricsLayout = new QHBoxLayout();
    metricsLayout->setSpacing(10);

    auto createMetricCard = [this, metricsLayout](const QString &title, const QString &initVal, const QString &color) {
        auto *card = new QFrame(this);
        card->setStyleSheet(
            "QFrame { background-color: #FFFFFF; border: 1px solid #E2E8F0; border-radius: 8px; border-left: 4px solid " + color + "; }"
            "QLabel { border: none; background: transparent; }"
        );
        auto *cLayout = new QVBoxLayout(card);
        cLayout->setContentsMargins(12, 6, 12, 6);
        cLayout->setSpacing(2);

        auto *tLabel = new QLabel(title, card);
        tLabel->setStyleSheet("font-size: 9.5px; font-weight: 800; color: #64748B; letter-spacing: 0.5px; border: none; background: transparent;");
        auto *vLabel = new QLabel(initVal, card);
        vLabel->setStyleSheet("font-size: 14px; font-weight: 800; color: #0F172A; border: none; background: transparent;");
        cLayout->addWidget(tLabel);
        cLayout->addWidget(vLabel);
        metricsLayout->addWidget(card);
        return vLabel;
    };

    m_totalVouchersVal = createMetricCard("TOTAL VOUCHERS", "0", "#2563EB");
    m_totalBankReceiptVal = createMetricCard("TOTAL BANK RECEIPT", "₹ 0.00", "#0284C7");
    m_totalTcsVal = createMetricCard("TOTAL TCS COLLECTED", "₹ 0.00", "#D97706");
    m_totalCreditPartyVal = createMetricCard("TOTAL PARTY CREDIT", "₹ 0.00", "#16A34A");

    mainLayout->addLayout(metricsLayout);

    // Shortcuts
    new QShortcut(QKeySequence(Qt::Key_Insert), this, SLOT(onNewClicked()));
    new QShortcut(QKeySequence("F5"), this, SLOT(onRefreshClicked()));
    new QShortcut(QKeySequence(Qt::Key_Delete), this, SLOT(onDeleteClicked()));
    new QShortcut(QKeySequence("Escape"), this, SIGNAL(backRequested()));
}

void TcsReceiptListWidget::reloadData()
{
    m_currentReceipts = m_controller->getReceiptVouchersList();
    populateTable();
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
        m_table->setItem(i, 2, new QTableWidgetItem(dt.isValid() ? dt.toString("dd-MM-yyyy") : m.value("receipt_date").toString()));

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

void TcsReceiptListWidget::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Escape) {
        emit backRequested();
        return;
    }
    QWidget::keyPressEvent(event);
}

} // namespace MahadevERP
