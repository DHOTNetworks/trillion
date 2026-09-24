#include "form16a_list_widget.h"
#include "form16a_entry_dialog.h"
#include "../database_manager.h"
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
    reloadData();
}

void Form16AListWidget::setupUi()
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
    auto *title = new QLabel("Received Forms-16A / 27D Register", headerCard);
    title->setStyleSheet("font-size: 16px; font-weight: 800; color: #0F172A; border: none; background: transparent;");
    auto *subLabel = new QLabel("Withholding tax certificates received from buyers and clients with TRACES token tracking.", headerCard);
    subLabel->setStyleSheet("font-size: 11px; color: #64748B; border: none; background: transparent;");
    titleCol->addWidget(title);
    titleCol->addWidget(subLabel);
    headerLayout->addLayout(titleCol);
    headerLayout->addStretch(1);

    auto *btnNew = new QPushButton("[Ins] + Receive Form-16A", headerCard);
    btnNew->setCursor(Qt::PointingHandCursor);
    btnNew->setStyleSheet(
        "QPushButton { background-color: #16A34A; color: #FFFFFF; border: none; border-radius: 6px; padding: 6px 16px; font-weight: 800; font-size: 12px; }"
        "QPushButton:hover { background-color: #15803D; }"
    );
    connect(btnNew, &QPushButton::clicked, this, &Form16AListWidget::onNewClicked);
    headerLayout->addWidget(btnNew);

    auto *btnDelete = new QPushButton("[Del] Delete", headerCard);
    btnDelete->setCursor(Qt::PointingHandCursor);
    btnDelete->setStyleSheet(
        "QPushButton { background-color: #DC2626; color: #FFFFFF; border: none; border-radius: 6px; padding: 6px 14px; font-weight: 700; font-size: 12px; }"
        "QPushButton:hover { background-color: #B91C1C; }"
    );
    connect(btnDelete, &QPushButton::clicked, this, &Form16AListWidget::onDeleteClicked);
    headerLayout->addWidget(btnDelete);

    auto *btnRefresh = new QPushButton("[F5] Refresh", headerCard);
    btnRefresh->setCursor(Qt::PointingHandCursor);
    btnRefresh->setStyleSheet(
        "QPushButton { background-color: #FFFFFF; border: 1.5px solid #CBD5E1; padding: 6px 12px; border-radius: 6px; font-weight: 700; font-size: 11.5px; color: #334155; }"
        "QPushButton:hover { background-color: #F8FAFC; }"
    );
    connect(btnRefresh, &QPushButton::clicked, this, &Form16AListWidget::onRefreshClicked);
    headerLayout->addWidget(btnRefresh);

    auto *btnBack = new QPushButton("← [Esc] Back", headerCard);
    btnBack->setCursor(Qt::PointingHandCursor);
    btnBack->setStyleSheet(
        "QPushButton { background-color: #F1F5F9; border: 1px solid #CBD5E1; border-radius: 6px; padding: 6px 12px; font-weight: 700; color: #475569; font-size: 11.5px; }"
        "QPushButton:hover { background-color: #E2E8F0; }"
    );
    connect(btnBack, &QPushButton::clicked, this, &Form16AListWidget::backRequested);
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

    m_totalCertsVal = createMetricCard("TOTAL CERTIFICATES", "0", "#2563EB");
    m_totalPaidVal = createMetricCard("TOTAL PAID AMOUNT", "₹ 0.00", "#0284C7");
    m_totalTdsVal = createMetricCard("TOTAL TDS DEDUCTED", "₹ 0.00", "#16A34A");

    mainLayout->addLayout(metricsLayout);

    // Shortcuts
    new QShortcut(QKeySequence(Qt::Key_Insert), this, SLOT(onNewClicked()));
    new QShortcut(QKeySequence("F5"), this, SLOT(onRefreshClicked()));
    new QShortcut(QKeySequence(Qt::Key_Delete), this, SLOT(onDeleteClicked()));
    new QShortcut(QKeySequence("Escape"), this, SIGNAL(backRequested()));
}

void Form16AListWidget::reloadData()
{
    DatabaseManager &db = DatabaseManager::instance();
    m_currentCerts = db.executeQuery(
        "SELECT f.id, f.certificate_no, p.name as customer_name, f.quarter, f.receipt_date, "
        "f.total_amount_credited, f.total_tds_deducted, f.is_verified "
        "FROM received_forms_16a f "
        "LEFT JOIN parties p ON f.customer_id = p.id "
        "ORDER BY f.id DESC;"
    );
    populateTable();
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
        m_table->setItem(i, 4, new QTableWidgetItem(dt.isValid() ? dt.toString("dd-MM-yyyy") : m.value("receipt_date").toString()));

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

    if (res == QMessageBox::Yes) {
        DatabaseManager &db = DatabaseManager::instance();
        db.executeNonQuery("DELETE FROM received_forms_16a WHERE id = ?;", {id});
        reloadData();
    }
}

void Form16AListWidget::onRefreshClicked()
{
    reloadData();
}

void Form16AListWidget::onFilterChanged()
{
    populateTable();
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
