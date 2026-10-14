#include "tax_challan_register_widget.h"
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

TaxChallanRegisterWidget::TaxChallanRegisterWidget(TaxChallanController *controller, const QString &initialType, QWidget *parent)
    : QWidget(parent)
    , m_controller(controller)
    , m_taxType(initialType)
{
    setupUi();
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
    m_titleLabel = new QLabel(QString("%1 Deposit Challans Register").arg(m_taxType), headerCard);
    m_titleLabel->setStyleSheet(ErpTheme::headerTitleStyle(16));
    auto *subLabel = new QLabel("Statutory ITNS 281 tax bank deposit entries, BSR codes, and constituent voucher audit.", headerCard);
    subLabel->setStyleSheet(ErpTheme::mutedLabelStyle(11));
    titleCol->addWidget(m_titleLabel);
    titleCol->addWidget(subLabel);
    headerLayout->addLayout(titleCol);
    headerLayout->addStretch(1);

    auto *btnNew = new KbdBadgeButton("+ New Challan", "Ins", headerCard);
    btnNew->setPrimaryColor(ErpTheme::kGreen, ErpTheme::kGreenDark);
    btnNew->setTextColor(ErpTheme::kPaper);
    btnNew->bindShortcut(QKeySequence(Qt::Key_Insert));
    connect(btnNew, &QPushButton::clicked, this, &TaxChallanRegisterWidget::onNewClicked);
    headerLayout->addWidget(btnNew);

    auto *btnDelete = new KbdBadgeButton("Delete", "Del", headerCard);
    btnDelete->setPrimaryColor(ErpTheme::kRed, QColor("#B91C1C"));
    btnDelete->setTextColor(ErpTheme::kPaper);
    btnDelete->bindShortcut(QKeySequence(Qt::Key_Delete));
    connect(btnDelete, &QPushButton::clicked, this, &TaxChallanRegisterWidget::onDeleteClicked);
    headerLayout->addWidget(btnDelete);

    auto *btnRefresh = new KbdBadgeButton("Refresh", "F5", headerCard);
    btnRefresh->setButtonColors(QColor("#FFFFFF"), QColor("#F8FAFC"), QColor("#334155"), QColor("#CBD5E1"));
    btnRefresh->bindShortcut(QKeySequence(Qt::Key_F5));
    connect(btnRefresh, &QPushButton::clicked, this, &TaxChallanRegisterWidget::onRefreshClicked);
    headerLayout->addWidget(btnRefresh);

    auto *btnBack = new KbdBadgeButton("Back", "Esc", headerCard);
    btnBack->setButtonColors(QColor("#F1F5F9"), QColor("#E2E8F0"), QColor("#475569"), QColor("#CBD5E1"));
    connect(btnBack, &QPushButton::clicked, this, &TaxChallanRegisterWidget::backRequested);
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
    ErpTheme::styleTable(m_table);
    mainLayout->addWidget(m_table, 1);

    // ========================================================================
    // TIER 4: SUMMARY METRICS FOOTER CARDS
    // ========================================================================
    auto *metricsLayout = new QHBoxLayout();
    metricsLayout->setSpacing(10);

        m_totalChallansVal = ErpTheme::makeMetricCard(this, metricsLayout, "TOTAL CHALLANS", "0", ErpTheme::kBlue);
    m_basicTaxVal = ErpTheme::makeMetricCard(this, metricsLayout, "BASIC TAX", "₹ 0.00", ErpTheme::kSky);
    m_interestPenaltyVal = ErpTheme::makeMetricCard(this, metricsLayout, "INTEREST & PENALTY", "₹ 0.00", ErpTheme::kAmber);
    m_totalDepositedVal = ErpTheme::makeMetricCard(this, metricsLayout, "TOTAL DEPOSITED", "₹ 0.00", ErpTheme::kGreen);

    mainLayout->addLayout(metricsLayout);

    // Shortcuts
    new QShortcut(QKeySequence(Qt::ALT | Qt::Key_N), this, SLOT(onNewClicked()));
    new QShortcut(QKeySequence(Qt::ALT | Qt::Key_F5), this, SLOT(onRefreshClicked()));
    new QShortcut(QKeySequence(Qt::ALT | Qt::Key_D), this, SLOT(onDeleteClicked()));
    new QShortcut(QKeySequence(Qt::ALT | Qt::Key_Escape), this, SIGNAL(backRequested()));
    new QShortcut(QKeySequence(Qt::Key_Escape), this, SIGNAL(backRequested()));
}

void TaxChallanRegisterWidget::reloadData()
{
    QString filterType = m_typeFilterCombo ? m_typeFilterCombo->currentText() : m_taxType;
    if (filterType == "ALL") filterType = "";
    m_currentChallans = m_controller->getChallansList(filterType);
    populateTable();
    m_isDirty = false;
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
        m_table->setItem(i, 3, new QTableWidgetItem(dt.isValid() ? dt.toString("dd-MM-yyyy") : FiscalYearHelper::formatDisplayDate(m.value("challan_date").toString())));

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

void TaxChallanRegisterWidget::focusTable()
{
    if (!m_table) return;
    if (m_table->rowCount() > 0 && m_table->currentRow() < 0) m_table->selectRow(0);
    m_table->setFocus();
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
