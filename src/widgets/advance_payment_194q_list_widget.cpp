#include "advance_payment_194q_list_widget.h"
#include "../database_manager.h"
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
    reloadData();
}

void AdvancePayment194QListWidget::setupUi()
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
    auto *title = new QLabel("Advance Payments U/S 194-Q Register", headerCard);
    title->setStyleSheet("font-size: 16px; font-weight: 800; color: #0F172A; border: none; background: transparent;");
    auto *subLabel = new QLabel("Purchase advance payments recorded with 0.1% tax deduction at source.", headerCard);
    subLabel->setStyleSheet("font-size: 11px; color: #64748B; border: none; background: transparent;");
    titleCol->addWidget(title);
    titleCol->addWidget(subLabel);
    headerLayout->addLayout(titleCol);
    headerLayout->addStretch(1);

    auto *btnNew = new QPushButton("[Ins] + New Advance", headerCard);
    btnNew->setCursor(Qt::PointingHandCursor);
    btnNew->setStyleSheet(
        "QPushButton { background-color: #16A34A; color: #FFFFFF; border: none; border-radius: 6px; padding: 6px 16px; font-weight: 800; font-size: 12px; }"
        "QPushButton:hover { background-color: #15803D; }"
    );
    connect(btnNew, &QPushButton::clicked, this, &AdvancePayment194QListWidget::onNewClicked);
    headerLayout->addWidget(btnNew);

    auto *btnDelete = new QPushButton("[Del] Delete", headerCard);
    btnDelete->setCursor(Qt::PointingHandCursor);
    btnDelete->setStyleSheet(
        "QPushButton { background-color: #DC2626; color: #FFFFFF; border: none; border-radius: 6px; padding: 6px 14px; font-weight: 700; font-size: 12px; }"
        "QPushButton:hover { background-color: #B91C1C; }"
    );
    connect(btnDelete, &QPushButton::clicked, this, &AdvancePayment194QListWidget::onDeleteClicked);
    headerLayout->addWidget(btnDelete);

    auto *btnRefresh = new QPushButton("[F5] Refresh", headerCard);
    btnRefresh->setCursor(Qt::PointingHandCursor);
    btnRefresh->setStyleSheet(
        "QPushButton { background-color: #FFFFFF; border: 1.5px solid #CBD5E1; padding: 6px 12px; border-radius: 6px; font-weight: 700; font-size: 11.5px; color: #334155; }"
        "QPushButton:hover { background-color: #F8FAFC; }"
    );
    connect(btnRefresh, &QPushButton::clicked, this, &AdvancePayment194QListWidget::onRefreshClicked);
    headerLayout->addWidget(btnRefresh);

    auto *btnBack = new QPushButton("← [Esc] Back", headerCard);
    btnBack->setCursor(Qt::PointingHandCursor);
    btnBack->setStyleSheet(
        "QPushButton { background-color: #F1F5F9; border: 1px solid #CBD5E1; border-radius: 6px; padding: 6px 12px; font-weight: 700; color: #475569; font-size: 11.5px; }"
        "QPushButton:hover { background-color: #E2E8F0; }"
    );
    connect(btnBack, &QPushButton::clicked, this, &AdvancePayment194QListWidget::backRequested);
    headerLayout->addWidget(btnBack);

    mainLayout->addWidget(headerCard);

    // ========================================================================
    // TIER 2: SEARCH TOOLBAR
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
    m_totalGrossVal = createMetricCard("TOTAL GROSS ADVANCE", "₹ 0.00", "#0284C7");
    m_totalTaxVal = createMetricCard("TOTAL TDS U/S 194-Q", "₹ 0.00", "#D97706");
    m_totalNetVal = createMetricCard("TOTAL NET PAID", "₹ 0.00", "#16A34A");

    mainLayout->addLayout(metricsLayout);

    // Shortcuts
    new QShortcut(QKeySequence(Qt::Key_Insert), this, SLOT(onNewClicked()));
    new QShortcut(QKeySequence("F5"), this, SLOT(onRefreshClicked()));
    new QShortcut(QKeySequence(Qt::Key_Delete), this, SLOT(onDeleteClicked()));
    new QShortcut(QKeySequence("Escape"), this, SIGNAL(backRequested()));
}

void AdvancePayment194QListWidget::reloadData()
{
    DatabaseManager &db = DatabaseManager::instance();
    m_currentVouchers = db.executeQuery(
        "SELECT a.id, a.voucher_no, a.voucher_date, p.name as supplier_name, b.name as bank_name, "
        "a.gross_advance, a.tds_rate, a.tds_amount, a.net_payment "
        "FROM advance_payments_194q a "
        "LEFT JOIN parties p ON a.supplier_id = p.id "
        "LEFT JOIN parties b ON a.bank_id = b.id "
        "ORDER BY a.voucher_no DESC;"
    );
    populateTable();
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
        double gross = m.value("gross_advance").toDouble();
        double tax = m.value("tds_amount").toDouble();
        double net = m.value("net_payment").toDouble();
        totalGross += gross;
        totalTax += tax;
        totalNet += net;

        m_table->setItem(i, 0, new QTableWidgetItem(m.value("id").toString()));
        m_table->setItem(i, 1, new QTableWidgetItem(QString("#%1").arg(m.value("voucher_no").toInt())));

        QDate dt = QDate::fromString(m.value("voucher_date").toString(), "yyyy-MM-dd");
        m_table->setItem(i, 2, new QTableWidgetItem(dt.isValid() ? dt.toString("dd-MM-yyyy") : m.value("voucher_date").toString()));

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

    if (res == QMessageBox::Yes) {
        DatabaseManager &db = DatabaseManager::instance();
        db.beginTransaction();
        QVariant vchId = db.executeScalar("SELECT voucher_id FROM advance_payments_194q WHERE id = ?;", {id});
        if (vchId.isValid() && vchId.toInt() > 0) {
            db.executeNonQuery("DELETE FROM voucher_items WHERE voucher_id = ?;", {vchId.toInt()});
            db.executeNonQuery("DELETE FROM vouchers WHERE id = ?;", {vchId.toInt()});
        }
        db.executeNonQuery("DELETE FROM advance_payments_194q WHERE id = ?;", {id});
        db.commit();
        reloadData();
    }
}

void AdvancePayment194QListWidget::onRefreshClicked()
{
    reloadData();
}

void AdvancePayment194QListWidget::onFilterChanged()
{
    populateTable();
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
