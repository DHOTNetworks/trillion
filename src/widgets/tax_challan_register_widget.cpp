#include "tax_challan_register_widget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QDate>
#include <QShortcut>
#include <QMessageBox>
#include <QKeyEvent>

namespace MahadevERP {

TaxChallanRegisterWidget::TaxChallanRegisterWidget(TaxChallanController *controller, const QString &initialType, QWidget *parent)
    : QWidget(parent)
    , m_controller(controller)
    , m_taxType(initialType)
{
    setupUi();
    reloadData();
}

void TaxChallanRegisterWidget::setTaxType(const QString &type)
{
    m_taxType = type;
    if (m_titleLabel) {
        m_titleLabel->setText(QString("%1 Deposit Challans Register").arg(m_taxType));
    }
    if (m_typeFilterCombo) {
        int idx = m_typeFilterCombo->findText(m_taxType);
        if (idx >= 0) m_typeFilterCombo->setCurrentIndex(idx);
    }
    reloadData();
}

void TaxChallanRegisterWidget::setupUi()
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
    m_titleLabel = new QLabel(QString("%1 Deposit Challans Register").arg(m_taxType), headerCard);
    m_titleLabel->setStyleSheet("font-size: 16px; font-weight: 800; color: #0F172A; border: none; background: transparent;");
    auto *subLabel = new QLabel("Statutory ITNS 281 tax bank deposit entries, BSR codes, and constituent voucher audit.", headerCard);
    subLabel->setStyleSheet("font-size: 11px; color: #64748B; border: none; background: transparent;");
    titleCol->addWidget(m_titleLabel);
    titleCol->addWidget(subLabel);
    headerLayout->addLayout(titleCol);
    headerLayout->addStretch(1);

    auto *btnNew = new QPushButton("[Ins] + New Challan", headerCard);
    btnNew->setCursor(Qt::PointingHandCursor);
    btnNew->setStyleSheet(
        "QPushButton { background-color: #16A34A; color: #FFFFFF; border: none; border-radius: 6px; padding: 6px 16px; font-weight: 800; font-size: 12px; }"
        "QPushButton:hover { background-color: #15803D; }"
    );
    connect(btnNew, &QPushButton::clicked, this, &TaxChallanRegisterWidget::onNewClicked);
    headerLayout->addWidget(btnNew);

    auto *btnDelete = new QPushButton("[Del] Delete", headerCard);
    btnDelete->setCursor(Qt::PointingHandCursor);
    btnDelete->setStyleSheet(
        "QPushButton { background-color: #DC2626; color: #FFFFFF; border: none; border-radius: 6px; padding: 6px 14px; font-weight: 700; font-size: 12px; }"
        "QPushButton:hover { background-color: #B91C1C; }"
    );
    connect(btnDelete, &QPushButton::clicked, this, &TaxChallanRegisterWidget::onDeleteClicked);
    headerLayout->addWidget(btnDelete);

    auto *btnRefresh = new QPushButton("[F5] Refresh", headerCard);
    btnRefresh->setCursor(Qt::PointingHandCursor);
    btnRefresh->setStyleSheet(
        "QPushButton { background-color: #FFFFFF; border: 1.5px solid #CBD5E1; border-radius: 6px; padding: 6px 12px; font-weight: 700; font-size: 11.5px; color: #334155; }"
        "QPushButton:hover { background-color: #F8FAFC; }"
    );
    connect(btnRefresh, &QPushButton::clicked, this, &TaxChallanRegisterWidget::onRefreshClicked);
    headerLayout->addWidget(btnRefresh);

    auto *btnBack = new QPushButton("← [Esc] Back", headerCard);
    btnBack->setCursor(Qt::PointingHandCursor);
    btnBack->setStyleSheet(
        "QPushButton { background-color: #F1F5F9; border: 1px solid #CBD5E1; border-radius: 6px; padding: 6px 12px; font-weight: 700; color: #475569; font-size: 11.5px; }"
        "QPushButton:hover { background-color: #E2E8F0; }"
    );
    connect(btnBack, &QPushButton::clicked, this, &TaxChallanRegisterWidget::backRequested);
    headerLayout->addWidget(btnBack);

    mainLayout->addWidget(headerCard);

    // ========================================================================
    // TIER 2: SEARCH & FILTER TOOLBAR
    // ========================================================================
    auto *filterCard = new QFrame(this);
    filterCard->setStyleSheet(
        "QFrame { background-color: #FFFFFF; border: 1px solid #CBD5E1; border-radius: 8px; }"
        "QLabel { color: #475569; font-weight: 700; font-size: 11.5px; border: none; background: transparent; }"
        "QLineEdit, QComboBox { background-color: #FFFFFF; border: 1.5px solid #CBD5E1; border-radius: 6px; padding: 4px 8px; font-size: 12px; color: #0F172A; font-weight: 600; }"
        "QLineEdit:focus, QComboBox:focus { border-color: #2563EB; background-color: #F8FAFC; }"
    );
    auto *filterLayout = new QHBoxLayout(filterCard);
    filterLayout->setContentsMargins(12, 8, 12, 8);
    filterLayout->setSpacing(12);

    filterLayout->addWidget(new QLabel("Search:", filterCard));
    m_searchEdit = new QLineEdit(filterCard);
    m_searchEdit->setPlaceholderText("Filter by challan no, bank, BSR code...");
    m_searchEdit->setClearButtonEnabled(true);
    connect(m_searchEdit, &QLineEdit::textChanged, this, &TaxChallanRegisterWidget::onFilterChanged);
    filterLayout->addWidget(m_searchEdit, 1);

    filterLayout->addWidget(new QLabel("Tax Type:", filterCard));
    m_typeFilterCombo = new QComboBox(filterCard);
    m_typeFilterCombo->addItems({"ALL", "TDS", "TCS"});
    int initialIdx = m_typeFilterCombo->findText(m_taxType);
    if (initialIdx >= 0) m_typeFilterCombo->setCurrentIndex(initialIdx);
    connect(m_typeFilterCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &TaxChallanRegisterWidget::onFilterChanged);
    filterLayout->addWidget(m_typeFilterCombo);

    mainLayout->addWidget(filterCard);

    // ========================================================================
    // TIER 3: REGISTER TABLE
    // ========================================================================
    m_table = new QTableWidget(this);
    m_table->setColumnCount(11);
    m_table->setHorizontalHeaderLabels({
        "ID", "Type", "Challan No", "Date", "Period",
        "Basic Tax (₹)", "Interest (₹)", "Penalty (₹)", "Total Amount (₹)", "Bank A/c", "BSR Code"
    });
    m_table->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(9, QHeaderView::Stretch);
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

    m_totalChallansVal = createMetricCard("TOTAL CHALLANS", "0", "#2563EB");
    m_basicTaxVal = createMetricCard("BASIC TAX", "₹ 0.00", "#0284C7");
    m_interestPenaltyVal = createMetricCard("INTEREST & PENALTY", "₹ 0.00", "#D97706");
    m_totalDepositedVal = createMetricCard("TOTAL DEPOSITED", "₹ 0.00", "#16A34A");

    mainLayout->addLayout(metricsLayout);

    // Shortcuts
    new QShortcut(QKeySequence(Qt::Key_Insert), this, SLOT(onNewClicked()));
    new QShortcut(QKeySequence("F5"), this, SLOT(onRefreshClicked()));
    new QShortcut(QKeySequence(Qt::Key_Delete), this, SLOT(onDeleteClicked()));
    new QShortcut(QKeySequence("Escape"), this, SIGNAL(backRequested()));
}

void TaxChallanRegisterWidget::reloadData()
{
    QString filterType = m_typeFilterCombo ? m_typeFilterCombo->currentText() : m_taxType;
    if (filterType == "ALL") filterType = "";
    m_currentChallans = m_controller->getChallansList(filterType);
    populateTable();
}

void TaxChallanRegisterWidget::populateTable()
{
    m_table->setRowCount(0);
    QString query = m_searchEdit ? m_searchEdit->text().trimmed().toLower() : "";

    QVariantList filtered;
    for (const auto &c : m_currentChallans) {
        QVariantMap m = c.toMap();
        if (!query.isEmpty()) {
            QString haystack = QString("%1 %2 %3 %4")
                .arg(m.value("challan_no").toString())
                .arg(m.value("bank_name").toString())
                .arg(m.value("bsr_code").toString())
                .arg(m.value("tax_type").toString()).toLower();
            if (!haystack.contains(query)) continue;
        }
        filtered.append(m);
    }

    m_table->setRowCount(filtered.size());
    double totalBasic = 0.0;
    double totalIntPen = 0.0;
    double totalDeposited = 0.0;

    for (int i = 0; i < filtered.size(); ++i) {
        QVariantMap m = filtered[i].toMap();
        double basic = m.value("basic_tax").toDouble();
        double interest = m.value("interest_amount").toDouble();
        double penalty = m.value("penalty_amount").toDouble();
        double tot = m.value("total_challan_amount").toDouble();

        totalBasic += basic;
        totalIntPen += (interest + penalty);
        totalDeposited += tot;

        m_table->setItem(i, 0, new QTableWidgetItem(m.value("id").toString()));
        m_table->setItem(i, 1, new QTableWidgetItem(m.value("tax_type").toString()));
        m_table->setItem(i, 2, new QTableWidgetItem(m.value("challan_no").toString()));

        QDate dt = QDate::fromString(m.value("challan_date").toString(), "yyyy-MM-dd");
        m_table->setItem(i, 3, new QTableWidgetItem(dt.isValid() ? dt.toString("dd-MM-yyyy") : m.value("challan_date").toString()));

        QString period = QString("%1 to %2").arg(m.value("period_from").toString(), m.value("period_to").toString());
        m_table->setItem(i, 4, new QTableWidgetItem(period));

        auto *bt = new QTableWidgetItem(QString::number(basic, 'f', 2));
        bt->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        m_table->setItem(i, 5, bt);

        auto *it = new QTableWidgetItem(QString::number(interest, 'f', 2));
        it->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        m_table->setItem(i, 6, it);

        auto *pt = new QTableWidgetItem(QString::number(penalty, 'f', 2));
        pt->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        m_table->setItem(i, 7, pt);

        auto *totItem = new QTableWidgetItem(QString::number(tot, 'f', 2));
        totItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        totItem->setForeground(QBrush(QColor("#16A34A")));
        QFont f = totItem->font();
        f.setBold(true);
        totItem->setFont(f);
        m_table->setItem(i, 8, totItem);

        m_table->setItem(i, 9, new QTableWidgetItem(m.value("bank_name").toString()));
        m_table->setItem(i, 10, new QTableWidgetItem(m.value("bsr_code").toString()));
    }

    if (m_totalChallansVal) m_totalChallansVal->setText(QString::number(filtered.size()));
    if (m_basicTaxVal) m_basicTaxVal->setText(QString("₹ %1").arg(QString::number(totalBasic, 'f', 2)));
    if (m_interestPenaltyVal) m_interestPenaltyVal->setText(QString("₹ %1").arg(QString::number(totalIntPen, 'f', 2)));
    if (m_totalDepositedVal) m_totalDepositedVal->setText(QString("₹ %1").arg(QString::number(totalDeposited, 'f', 2)));
}

void TaxChallanRegisterWidget::onNewClicked()
{
    QString type = m_typeFilterCombo ? m_typeFilterCombo->currentText() : m_taxType;
    if (type == "ALL") type = m_taxType;
    emit newChallanRequested(type);
}

void TaxChallanRegisterWidget::onDeleteClicked()
{
    int row = m_table->currentRow();
    if (row < 0) return;

    int id = m_table->item(row, 0)->text().toInt();
    QString no = m_table->item(row, 2)->text();

    int res = QMessageBox::question(
        this, "Delete Challan",
        QString("Are you sure you want to delete Challan %1?\nLinked vouchers will be restored to un-deposited state.").arg(no),
        QMessageBox::Yes | QMessageBox::No
    );

    if (res == QMessageBox::Yes) {
        m_controller->deleteChallan(id);
        reloadData();
    }
}

void TaxChallanRegisterWidget::onRefreshClicked()
{
    reloadData();
}

void TaxChallanRegisterWidget::onFilterChanged()
{
    reloadData();
}

void TaxChallanRegisterWidget::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Escape) {
        emit backRequested();
        return;
    }
    QWidget::keyPressEvent(event);
}

} // namespace MahadevERP
