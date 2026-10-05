#include "ledger_directory_widget.h"
#include "../database_manager.h"
#include "../engine/fiscal_year_helper.h"
#include "../engine/ledger_pipeline.h"
#include "../engine/accounting_engine.h"
#include "../services/financial_math_service.h"
#include "kbd_badge_button.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QPainter>
#include <QKeyEvent>
#include <QMessageBox>
#include <QSqlQuery>
#include <QSqlError>
#include <cmath>
#include <algorithm>

namespace MahadevERP {

static QString formatINR(double val, bool showNil = false) {
    if (std::abs(val) < 0.001) return showNil ? "₹0.00" : "-";
    return FinancialMathService::instance().formatInr(val, false);
}

LedgerDirectoryWidget::LedgerDirectoryWidget(PrintExportController* printExportCtrl, QWidget* parent)
    : QWidget(parent)
    , m_printExportCtrl(printExportCtrl)
{
    setupUi();
}

void LedgerDirectoryWidget::setupUi() {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(12, 10, 12, 10);
    mainLayout->setSpacing(8);

    // 1. Header Card
    auto* headerCard = new QFrame(this);
    headerCard->setObjectName("headerCard");
    headerCard->setFixedHeight(50);
    headerCard->setStyleSheet(
        "QFrame#headerCard {"
        "  background-color: #FFFFFF;"
        "  border: 1px solid #E2E8F0;"
        "  border-radius: 8px;"
        "}"
        "QFrame#headerCard QLabel {"
        "  border: none;"
        "  background: transparent;"
        "}"
    );
    auto* headerLayout = new QHBoxLayout(headerCard);
    headerLayout->setContentsMargins(14, 0, 14, 0);
    headerLayout->setSpacing(8);

    auto* titleLabel = new QLabel("ALL LEDGERS DIRECTORY / MASTER LIST", headerCard);
    titleLabel->setStyleSheet("font-size: 13px; font-weight: 800; color: #0F172A; letter-spacing: 0.5px; border: none; background: transparent;");
    headerLayout->addWidget(titleLabel);

    m_fyBadge = new QLabel(headerCard);
    m_fyBadge->setStyleSheet("background-color: #EFF6FF; color: #1D4ED8; font-size: 11px; font-weight: 700; border: 1px solid #BFDBFE; border-radius: 4px; padding: 2px 8px;");
    headerLayout->addWidget(m_fyBadge);

    headerLayout->addStretch(1);

    auto* newLedgerBtn = new KbdBadgeButton("New Ledger", "F3", headerCard);
    connect(newLedgerBtn, &KbdBadgeButton::clicked, this, &LedgerDirectoryWidget::newLedgerRequested);
    headerLayout->addWidget(newLedgerBtn);

    auto* modLedgerBtn = new KbdBadgeButton("Modify", "Alt+M", headerCard);
    connect(modLedgerBtn, &KbdBadgeButton::clicked, this, [this]() {
        int r = m_table->currentRow();
        if (r >= 0 && r < m_filteredItems.size()) {
            emit modifyLedgerRequested(m_filteredItems[r].id, m_filteredItems[r].name);
        }
    });
    headerLayout->addWidget(modLedgerBtn);

    auto* printBtn = new KbdBadgeButton("Print", "Ctrl+P", headerCard);
    connect(printBtn, &KbdBadgeButton::clicked, this, &LedgerDirectoryWidget::onPrintClicked);
    headerLayout->addWidget(printBtn);

    auto* pdfBtn = new KbdBadgeButton("PDF", "Ctrl+E", headerCard);
    connect(pdfBtn, &KbdBadgeButton::clicked, this, &LedgerDirectoryWidget::onExportPdfClicked);
    headerLayout->addWidget(pdfBtn);

    auto* csvBtn = new KbdBadgeButton("CSV", "Alt+C", headerCard);
    connect(csvBtn, &KbdBadgeButton::clicked, this, &LedgerDirectoryWidget::onExportCsvClicked);
    headerLayout->addWidget(csvBtn);

    auto* backBtn = new KbdBadgeButton("Back", "Esc", headerCard);
    connect(backBtn, &KbdBadgeButton::clicked, this, &LedgerDirectoryWidget::backRequested);
    headerLayout->addWidget(backBtn);

    mainLayout->addWidget(headerCard);

    // 2. Filter & Controls Bar
    auto* filterCard = new QFrame(this);
    filterCard->setObjectName("filterCard");
    filterCard->setFixedHeight(46);
    filterCard->setStyleSheet(
        "QFrame#filterCard {"
        "  background-color: #FFFFFF;"
        "  border: 1px solid #E2E8F0;"
        "  border-radius: 8px;"
        "}"
        "QFrame#filterCard QLabel {"
        "  border: none;"
        "  background: transparent;"
        "}"
    );
    auto* filterLayout = new QHBoxLayout(filterCard);
    filterLayout->setContentsMargins(14, 0, 14, 0);
    filterLayout->setSpacing(10);

    auto* searchTag = new QLabel("SEARCH", filterCard);
    searchTag->setStyleSheet("font-size: 10px; font-weight: 800; color: #64748B; letter-spacing: 0.5px; border: none;");
    filterLayout->addWidget(searchTag);

    m_searchEdit = new QLineEdit(filterCard);
    m_searchEdit->setPlaceholderText("Filter by Name, Group, City, Mobile, GSTIN, PAN, Balance... (Ctrl+F)");
    m_searchEdit->setClearButtonEnabled(true);
    m_searchEdit->setStyleSheet(
        "QLineEdit {"
        "  background-color: #F8FAFC;"
        "  border: 1px solid #CBD5E1;"
        "  border-radius: 6px;"
        "  padding: 5px 10px;"
        "  font-size: 11px;"
        "  color: #0F172A;"
        "  min-width: 280px;"
        "}"
        "QLineEdit:focus {"
        "  background-color: #FFFFFF;"
        "  border: 1.5px solid #2563EB;"
        "}"
    );
    connect(m_searchEdit, &QLineEdit::textChanged, this, &LedgerDirectoryWidget::onSearchChanged);
    filterLayout->addWidget(m_searchEdit, 2);

    auto* grpLabel = new QLabel("Group:", filterCard);
    grpLabel->setStyleSheet("font-size: 11px; font-weight: 700; color: #475569; border: none;");
    filterLayout->addWidget(grpLabel);

    m_groupCombo = new QComboBox(filterCard);
    m_groupCombo->addItem("All Groups");
    m_groupCombo->setStyleSheet(
        "QComboBox {"
        "  background-color: #F8FAFC;"
        "  border: 1px solid #CBD5E1;"
        "  border-radius: 6px;"
        "  padding: 4px 8px;"
        "  font-size: 11px;"
        "  color: #0F172A;"
        "  min-width: 150px;"
        "}"
        "QComboBox:focus {"
        "  border: 1.5px solid #2563EB;"
        "}"
    );
    connect(m_groupCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &LedgerDirectoryWidget::onGroupFilterChanged);
    filterLayout->addWidget(m_groupCombo, 1);

    auto* balLabel = new QLabel("Filter Balances:", filterCard);
    balLabel->setStyleSheet("font-size: 11px; font-weight: 700; color: #475569; border: none;");
    filterLayout->addWidget(balLabel);

    m_balanceFilterCombo = new QComboBox(filterCard);
    m_balanceFilterCombo->addItem("All Accounts");
    m_balanceFilterCombo->addItem("Only Non-Zero Balances");
    m_balanceFilterCombo->addItem("Debit Balances Only (Dr)");
    m_balanceFilterCombo->addItem("Credit Balances Only (Cr)");
    m_balanceFilterCombo->addItem("Zero Balance Only");
    m_balanceFilterCombo->setStyleSheet(
        "QComboBox {"
        "  background-color: #F8FAFC;"
        "  border: 1px solid #CBD5E1;"
        "  border-radius: 6px;"
        "  padding: 4px 8px;"
        "  font-size: 11px;"
        "  color: #0F172A;"
        "  min-width: 150px;"
        "}"
        "QComboBox:focus {"
        "  border: 1.5px solid #2563EB;"
        "}"
    );
    connect(m_balanceFilterCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &LedgerDirectoryWidget::onBalanceFilterChanged);
    filterLayout->addWidget(m_balanceFilterCombo, 1);

    mainLayout->addWidget(filterCard);

    // 3. Summary Stat Bar (Evenly distributed, clean typography, no child borders)
    auto* statsFrame = new QFrame(this);
    statsFrame->setObjectName("statsBarFrame");
    statsFrame->setFixedHeight(54);
    statsFrame->setStyleSheet(
        "QFrame#statsBarFrame {"
        "  background-color: #FFFFFF;"
        "  border: 1px solid #CBD5E1;"
        "  border-radius: 8px;"
        "}"
        "QFrame#statsBarFrame QWidget {"
        "  background: transparent;"
        "  border: none;"
        "}"
        "QFrame#statsBarFrame QLabel {"
        "  background: transparent;"
        "  border: none;"
        "}"
    );
    auto* statsLayout = new QHBoxLayout(statsFrame);
    statsLayout->setContentsMargins(16, 6, 16, 6);
    statsLayout->setSpacing(0);

    auto createStat = [statsFrame, statsLayout](const QString& title, QLabel*& valLabel, const QString& color, bool isLast = false) {
        auto* w = new QWidget(statsFrame);
        auto* l = new QVBoxLayout(w);
        l->setContentsMargins(8, 0, 8, 0);
        l->setSpacing(2);
        l->setAlignment(Qt::AlignVCenter | Qt::AlignLeft);

        auto* t = new QLabel(title, w);
        t->setStyleSheet("font-size: 10px; font-weight: 800; color: #64748B; letter-spacing: 0.5px; border: none; background: transparent;");
        valLabel = new QLabel("-", w);
        valLabel->setStyleSheet(QString("font-size: 13px; font-weight: 800; color: %1; border: none; background: transparent;").arg(color));
        l->addWidget(t);
        l->addWidget(valLabel);

        statsLayout->addWidget(w, 1);

        if (!isLast) {
            auto* sep = new QFrame(statsFrame);
            sep->setFrameShape(QFrame::VLine);
            sep->setFixedWidth(1);
            sep->setFixedHeight(32);
            sep->setStyleSheet("background-color: #E2E8F0; border: none;");
            statsLayout->addWidget(sep);
        }
    };

    createStat("TOTAL ACCOUNTS", m_totalAccountsLabel, "#0F172A");
    createStat("TOTAL OP. DEBIT", m_totalOpDrLabel, "#1D4ED8");
    createStat("TOTAL OP. CREDIT", m_totalOpCrLabel, "#047857");
    createStat("TOTAL CLOSING DEBIT", m_totalCloseDrLabel, "#1D4ED8");
    createStat("TOTAL CLOSING CREDIT", m_totalCloseCrLabel, "#047857");
    createStat("NET CLOSING BALANCE", m_netDiffLabel, "#2563EB", true);

    mainLayout->addWidget(statsFrame);

    // 4. Robust White Grid Data Table
    m_table = new QTableWidget(this);
    m_table->setColumnCount(8);
    m_table->setHorizontalHeaderLabels({
        "#", "Account / Ledger Name", "Group", "Station / City", "Mobile / Phone", "GSTIN", "Opening Balance", "Closing Balance"
    });

    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setShowGrid(true);
    m_table->setGridStyle(Qt::SolidLine);
    m_table->verticalHeader()->setVisible(false);
    m_table->verticalHeader()->setDefaultSectionSize(36);
    m_table->setAlternatingRowColors(true);
    m_table->setFocusPolicy(Qt::StrongFocus);

    m_table->setStyleSheet(
        "QTableWidget {"
        "  background-color: #FFFFFF;"
        "  alternate-background-color: #F8FAFC;"
        "  border: 1px solid #CBD5E1;"
        "  gridline-color: #E2E8F0;"
        "  selection-background-color: #DBEAFE;"
        "  selection-color: #0F172A;"
        "  font-family: 'Segoe UI', -apple-system, BlinkMacSystemFont, Roboto, sans-serif;"
        "  font-size: 11.5px;"
        "}"
        "QHeaderView::section {"
        "  background-color: #F1F5F9;"
        "  color: #0F172A;"
        "  font-weight: 800;"
        "  font-size: 11.5px;"
        "  padding: 8px 8px;"
        "  border: none;"
        "  border-bottom: 2px solid #CBD5E1;"
        "  border-right: 1px solid #E2E8F0;"
        "}"
        "QTableWidget::item:selected {"
        "  background-color: #DBEAFE;"
        "  color: #1E3A8A;"
        "  font-weight: 700;"
        "}"
        "QScrollBar:vertical {"
        "  border: none;"
        "  background: #F1F5F9;"
        "  width: 8px;"
        "  border-radius: 4px;"
        "}"
        "QScrollBar::handle:vertical {"
        "  background: #CBD5E1;"
        "  min-height: 24px;"
        "  border-radius: 4px;"
        "}"
        "QScrollBar::handle:vertical:hover {"
        "  background: #94A3B8;"
        "}"
    );

    m_table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Fixed);
    m_table->setColumnWidth(0, 48); // #
    m_table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch); // Name
    m_table->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents); // Group
    m_table->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents); // City
    m_table->horizontalHeader()->setSectionResizeMode(4, QHeaderView::ResizeToContents); // Phone
    m_table->horizontalHeader()->setSectionResizeMode(5, QHeaderView::ResizeToContents); // GSTIN
    m_table->horizontalHeader()->setSectionResizeMode(6, QHeaderView::ResizeToContents); // Op Bal
    m_table->horizontalHeader()->setSectionResizeMode(7, QHeaderView::ResizeToContents); // Cl Bal

    connect(m_table, &QTableWidget::cellDoubleClicked, this, &LedgerDirectoryWidget::onTableDoubleClicked);

    mainLayout->addWidget(m_table, 1);
}

void LedgerDirectoryWidget::focusTable() {
    if (m_table) m_table->setFocus();
}

void LedgerDirectoryWidget::focusSearch() {
    if (m_searchEdit) {
        m_searchEdit->setFocus();
        m_searchEdit->selectAll();
    }
}

void LedgerDirectoryWidget::showEvent(QShowEvent* event) {
    QWidget::showEvent(event);
    FiscalYearInfo activeFy = FiscalYearHelper::getActiveFiscalYear();
    m_fyBadge->setText("FY: " + activeFy.name);

    if (m_isDirty || m_allItems.isEmpty()) {
        loadData();
    }
}

void LedgerDirectoryWidget::paintEvent(QPaintEvent* event) {
    QPainter painter(this);
    painter.fillRect(rect(), QColor("#F8FAFC"));
    QWidget::paintEvent(event);
}

void LedgerDirectoryWidget::keyPressEvent(QKeyEvent* event) {
    if (event->key() == Qt::Key_Escape) {
        event->accept();
        emit backRequested();
        return;
    }

    if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter || event->key() == Qt::Key_F4) {
        if (m_table->hasFocus() || m_searchEdit->hasFocus()) {
            int r = m_table->currentRow();
            if (r < 0 && !m_filteredItems.isEmpty()) r = 0;
            if (r >= 0 && r < m_filteredItems.size()) {
                event->accept();
                emit openStatementRequested(m_filteredItems[r].name);
                return;
            }
        }
    }

    if (event->key() == Qt::Key_Down && m_searchEdit->hasFocus()) {
        if (!m_filteredItems.isEmpty()) {
            m_table->setFocus();
            if (m_table->currentRow() < 0) m_table->selectRow(0);
            event->accept();
            return;
        }
    }

    if (event->key() == Qt::Key_F3 || (event->key() == Qt::Key_N && (event->modifiers() & Qt::AltModifier))) {
        event->accept();
        emit newLedgerRequested();
        return;
    }

    if (event->key() == Qt::Key_F2 || (event->key() == Qt::Key_M && (event->modifiers() & Qt::AltModifier))) {
        int r = m_table->currentRow();
        if (r >= 0 && r < m_filteredItems.size()) {
            event->accept();
            emit modifyLedgerRequested(m_filteredItems[r].id, m_filteredItems[r].name);
            return;
        }
    }

    if (event->key() == Qt::Key_Slash || (event->key() == Qt::Key_F && (event->modifiers() & Qt::ControlModifier))) {
        focusSearch();
        event->accept();
        return;
    }

    if (event->matches(QKeySequence::Print) || (event->key() == Qt::Key_P && (event->modifiers() & Qt::ControlModifier))) {
        onPrintClicked();
        event->accept();
        return;
    }

    if (event->key() == Qt::Key_E && (event->modifiers() & Qt::ControlModifier)) {
        onExportPdfClicked();
        event->accept();
        return;
    }

    QWidget::keyPressEvent(event);
}

void LedgerDirectoryWidget::loadData() {
    m_allItems.clear();

    FiscalYearInfo activeFy = FiscalYearHelper::getActiveFiscalYear();
    QString fromIso = activeFy.startDate;
    QString toIso = activeFy.endDate;

    // Calculate real dynamic balances from LedgerPipeline
    QVector<::LedgerPeriodBalance> balances = ::LedgerPipeline::instance().calculateBalancesForPeriod(fromIso, toIso);
    QHash<int, ::LedgerPeriodBalance> balById;
    QHash<QString, ::LedgerPeriodBalance> balByName;
    for (const auto& b : balances) {
        balById.insert(b.accountId, b);
        balByName.insert(b.accountName.trimmed().toLower(), b);
    }

    // Query all parties from master table via DatabaseManager
    QVariantList parties = DatabaseManager::instance().executeQuery(
        "SELECT id, legacy_id, name, alias, group_name, city, party_station, phone, mobile, gstin, pan, address, opening_balance, balance_type "
        "FROM parties ORDER BY name COLLATE NOCASE ASC;"
    );

    QSet<QString> groupSet;
    for (const auto& var : parties) {
        QVariantMap row = var.toMap();
        LedgerDirectoryItem item;
        item.id = row.value("id").toInt();
        item.name = row.value("name").toString().trimmed();
        item.alias = row.value("alias").toString().trimmed();
        item.groupName = row.value("group_name").toString().trimmed();
        if (item.groupName.isEmpty()) item.groupName = "General";
        item.city = row.value("city").toString().trimmed();
        if (item.city.isEmpty()) item.city = row.value("party_station").toString().trimmed();
        item.phone = row.value("phone").toString().trimmed();
        item.mobile = row.value("mobile").toString().trimmed();
        item.gstin = row.value("gstin").toString().trimmed();
        item.pan = row.value("pan").toString().trimmed();
        item.address = row.value("address").toString().trimmed();

        // Match with pipeline period balance
        ::LedgerPeriodBalance pb;
        if (balById.contains(item.id)) {
            pb = balById.value(item.id);
        } else {
            pb = balByName.value(item.name.toLower());
        }

        // Net Opening Balance calculation
        double netOp = pb.openingDr - pb.openingCr;
        if (std::abs(netOp) < 0.001 && pb.accountId == 0) {
            // Fallback if not in pipeline cache
            double masterOp = row.value("opening_balance").toDouble();
            QString bType = row.value("balance_type").toString().trimmed();
            netOp = (bType.compare("Dr", Qt::CaseInsensitive) == 0 || bType.compare("D", Qt::CaseInsensitive) == 0) ? masterOp : -masterOp;
        }

        if (netOp > 0.001) {
            item.opDebit = netOp;
            item.opCredit = 0.0;
            item.opBalFmt = formatINR(netOp) + " Dr";
        } else if (netOp < -0.001) {
            item.opDebit = 0.0;
            item.opCredit = std::abs(netOp);
            item.opBalFmt = formatINR(std::abs(netOp)) + " Cr";
        } else {
            item.opDebit = 0.0;
            item.opCredit = 0.0;
            item.opBalFmt = "-";
        }

        // Net Closing Balance calculation
        double netClose = pb.closingDr - pb.closingCr;
        if (std::abs(netClose) < 0.001 && pb.accountId == 0) {
            netClose = netOp;
        }

        item.netClosing = netClose;
        if (netClose > 0.001) {
            item.closeDebit = netClose;
            item.closeCredit = 0.0;
            item.closeBalFmt = formatINR(netClose) + " Dr";
        } else if (netClose < -0.001) {
            item.closeDebit = 0.0;
            item.closeCredit = std::abs(netClose);
            item.closeBalFmt = formatINR(std::abs(netClose)) + " Cr";
        } else {
            item.closeDebit = 0.0;
            item.closeCredit = 0.0;
            item.closeBalFmt = "-";
        }

        groupSet.insert(item.groupName);
        m_allItems.append(item);
    }

    // Update group dropdown if count changed
    QString curGrp = m_groupCombo->currentText();
    m_groupCombo->blockSignals(true);
    m_groupCombo->clear();
    m_groupCombo->addItem("All Groups");
    QStringList sortedGroups = groupSet.values();
    sortedGroups.sort(Qt::CaseInsensitive);
    for (const QString& g : sortedGroups) {
        m_groupCombo->addItem(g);
    }
    int idx = m_groupCombo->findText(curGrp);
    if (idx >= 0) m_groupCombo->setCurrentIndex(idx);
    else m_groupCombo->setCurrentIndex(0);
    m_groupCombo->blockSignals(false);

    applyFilter();
    m_isDirty = false;
}

void LedgerDirectoryWidget::onSearchChanged(const QString& /*query*/) {
    applyFilter();
}

void LedgerDirectoryWidget::onGroupFilterChanged(int /*index*/) {
    applyFilter();
}

void LedgerDirectoryWidget::onBalanceFilterChanged(int /*index*/) {
    applyFilter();
}

void LedgerDirectoryWidget::applyFilter() {
    m_filteredItems.clear();

    QString query = m_searchEdit ? m_searchEdit->text().trimmed().toLower() : "";
    QString selectedGroup = m_groupCombo ? m_groupCombo->currentText() : "All Groups";
    int balMode = m_balanceFilterCombo ? m_balanceFilterCombo->currentIndex() : 0;

    for (const auto& item : m_allItems) {
        // Group filter
        if (selectedGroup != "All Groups" && item.groupName.compare(selectedGroup, Qt::CaseInsensitive) != 0) {
            continue;
        }

        // Balance mode filter
        // 0: All Accounts, 1: Non-Zero, 2: Debit Only, 3: Credit Only, 4: Zero Only
        if (balMode == 1 && (item.closeDebit < 0.001 && item.closeCredit < 0.001)) {
            continue;
        } else if (balMode == 2 && item.closeDebit < 0.001) {
            continue;
        } else if (balMode == 3 && item.closeCredit < 0.001) {
            continue;
        } else if (balMode == 4 && (item.closeDebit > 0.001 || item.closeCredit > 0.001)) {
            continue;
        }

        // Search text filter
        if (!query.isEmpty()) {
            bool matches = item.name.toLower().contains(query) ||
                            item.alias.toLower().contains(query) ||
                            item.groupName.toLower().contains(query) ||
                            item.city.toLower().contains(query) ||
                            item.phone.contains(query) ||
                            item.mobile.contains(query) ||
                            item.gstin.toLower().contains(query) ||
                            item.pan.toLower().contains(query) ||
                            item.opBalFmt.toLower().contains(query) ||
                            item.closeBalFmt.toLower().contains(query);
            if (!matches) continue;
        }

        m_filteredItems.append(item);
    }

    populateTable();
    updateSummaryCards();
}

void LedgerDirectoryWidget::populateTable() {
    int curRow = m_table->currentRow();
    m_table->setRowCount(0);
    m_table->setRowCount(m_filteredItems.size());

    for (int r = 0; r < m_filteredItems.size(); ++r) {
        const auto& item = m_filteredItems[r];

        // 0: #
        auto* numItem = new QTableWidgetItem(QString::number(r + 1));
        numItem->setTextAlignment(Qt::AlignCenter);
        numItem->setForeground(QBrush(QColor("#64748B")));
        numItem->setFont(QFont("Segoe UI", 10, QFont::DemiBold));
        m_table->setItem(r, 0, numItem);

        // 1: Name
        auto* nameItem = new QTableWidgetItem(item.name);
        nameItem->setFont(QFont("Segoe UI", 11, QFont::Bold));
        nameItem->setForeground(QBrush(QColor("#0F172A")));
        m_table->setItem(r, 1, nameItem);

        // 2: Group
        auto* grpItem = new QTableWidgetItem(item.groupName);
        grpItem->setForeground(QBrush(QColor("#1D4ED8")));
        grpItem->setFont(QFont("Segoe UI", 10, QFont::DemiBold));
        m_table->setItem(r, 2, grpItem);

        // 3: City
        auto* cityItem = new QTableWidgetItem(item.city.isEmpty() ? "-" : item.city);
        cityItem->setForeground(QBrush(QColor("#1E293B")));
        cityItem->setFont(QFont("Segoe UI", 10));
        m_table->setItem(r, 3, cityItem);

        // 4: Phone / Mobile
        QString phoneDisp = item.mobile;
        if (phoneDisp.isEmpty()) phoneDisp = item.phone;
        if (phoneDisp.isEmpty()) phoneDisp = "-";
        auto* phoneItem = new QTableWidgetItem(phoneDisp);
        phoneItem->setForeground(QBrush(QColor("#1E293B")));
        phoneItem->setFont(QFont("Segoe UI", 10));
        m_table->setItem(r, 4, phoneItem);

        // 5: GSTIN
        QString gstinDisp = item.gstin;
        if (gstinDisp.isEmpty() && !item.pan.isEmpty()) gstinDisp = "PAN: " + item.pan;
        if (gstinDisp.isEmpty()) gstinDisp = "-";
        auto* gstinItem = new QTableWidgetItem(gstinDisp);
        gstinItem->setForeground(QBrush(QColor("#334155")));
        gstinItem->setFont(QFont("Segoe UI", 10));
        m_table->setItem(r, 5, gstinItem);

        // 6: Opening Balance
        auto* opItem = new QTableWidgetItem(item.opBalFmt);
        opItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        opItem->setFont(QFont("Segoe UI", 11, QFont::Bold));
        if (item.opDebit > 0.001) opItem->setForeground(QBrush(QColor("#1D4ED8")));
        else if (item.opCredit > 0.001) opItem->setForeground(QBrush(QColor("#047857")));
        else opItem->setForeground(QBrush(QColor("#64748B")));
        m_table->setItem(r, 6, opItem);

        // 7: Closing Balance
        auto* clItem = new QTableWidgetItem(item.closeBalFmt);
        clItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        clItem->setFont(QFont("Segoe UI", 11, QFont::Bold));
        if (item.closeDebit > 0.001) {
            clItem->setForeground(QBrush(QColor("#1D4ED8"))); // Royal Blue for Dr
        } else if (item.closeCredit > 0.001) {
            clItem->setForeground(QBrush(QColor("#047857"))); // Emerald Green for Cr
        } else {
            clItem->setForeground(QBrush(QColor("#64748B")));
        }
        m_table->setItem(r, 7, clItem);
    }

    if (!m_filteredItems.isEmpty()) {
        int targetRow = (curRow >= 0) ? qBound(0, curRow, m_filteredItems.size() - 1) : 0;
        m_table->selectRow(targetRow);
    }
}

void LedgerDirectoryWidget::updateSummaryCards() {
    double totalOpDr = 0.0;
    double totalOpCr = 0.0;
    double totalCloseDr = 0.0;
    double totalCloseCr = 0.0;

    for (const auto& item : m_filteredItems) {
        totalOpDr += item.opDebit;
        totalOpCr += item.opCredit;
        totalCloseDr += item.closeDebit;
        totalCloseCr += item.closeCredit;
    }

    double netClose = totalCloseDr - totalCloseCr;
    QString netType = (netClose >= 0.0) ? "Dr" : "Cr";
    double absNet = std::abs(netClose);

    m_totalAccountsLabel->setText(QString::number(m_filteredItems.size()) + " Accounts");
    m_totalOpDrLabel->setText(formatINR(totalOpDr, true) + " Dr");
    m_totalOpCrLabel->setText(formatINR(totalOpCr, true) + " Cr");
    m_totalCloseDrLabel->setText(formatINR(totalCloseDr, true) + " Dr");
    m_totalCloseCrLabel->setText(formatINR(totalCloseCr, true) + " Cr");
    m_netDiffLabel->setText(formatINR(absNet, true) + " " + netType);
}

void LedgerDirectoryWidget::onTableDoubleClicked(int row, int /*col*/) {
    if (row >= 0 && row < m_filteredItems.size()) {
        emit openStatementRequested(m_filteredItems[row].name);
    }
}

void LedgerDirectoryWidget::onPrintClicked() {
    if (!m_printExportCtrl) return;

    QString html = "<h2>ALL LEDGERS DIRECTORY / MASTER LIST</h2>";
    html += "<p><b>Total Ledgers:</b> " + QString::number(m_filteredItems.size()) + "</p>";
    html += "<table border='1' cellspacing='0' cellpadding='5' width='100%'>";
    html += "<tr style='background-color:#F1F5F9; font-weight:bold;'>";
    html += "<th>#</th><th>Account / Ledger Name</th><th>Group</th><th>City</th><th>Phone</th><th>GSTIN</th><th>Opening Bal</th><th>Closing Bal</th>";
    html += "</tr>";

    for (int i = 0; i < m_filteredItems.size(); ++i) {
        const auto& it = m_filteredItems[i];
        html += QString("<tr><td>%1</td><td><b>%2</b></td><td>%3</td><td>%4</td><td>%5</td><td>%6</td><td align='right'>%7</td><td align='right'><b>%8</b></td></tr>")
                    .arg(i + 1)
                    .arg(it.name.toHtmlEscaped())
                    .arg(it.groupName.toHtmlEscaped())
                    .arg(it.city.toHtmlEscaped())
                    .arg(it.mobile.toHtmlEscaped())
                    .arg(it.gstin.toHtmlEscaped())
                    .arg(it.opBalFmt)
                    .arg(it.closeBalFmt);
    }
    html += "</table>";

    m_printExportCtrl->printHtml(html, "Ledger Directory");
}

void LedgerDirectoryWidget::onExportPdfClicked() {
    if (!m_printExportCtrl) return;

    QString html = "<h2>ALL LEDGERS DIRECTORY / MASTER LIST</h2>";
    html += "<p><b>Total Ledgers:</b> " + QString::number(m_filteredItems.size()) + "</p>";
    html += "<table border='1' cellspacing='0' cellpadding='5' width='100%'>";
    html += "<tr style='background-color:#F1F5F9; font-weight:bold;'>";
    html += "<th>#</th><th>Account / Ledger Name</th><th>Group</th><th>City</th><th>Phone</th><th>GSTIN</th><th>Opening Bal</th><th>Closing Bal</th>";
    html += "</tr>";

    for (int i = 0; i < m_filteredItems.size(); ++i) {
        const auto& it = m_filteredItems[i];
        html += QString("<tr><td>%1</td><td><b>%2</b></td><td>%3</td><td>%4</td><td>%5</td><td>%6</td><td align='right'>%7</td><td align='right'><b>%8</b></td></tr>")
                    .arg(i + 1)
                    .arg(it.name.toHtmlEscaped())
                    .arg(it.groupName.toHtmlEscaped())
                    .arg(it.city.toHtmlEscaped())
                    .arg(it.mobile.toHtmlEscaped())
                    .arg(it.gstin.toHtmlEscaped())
                    .arg(it.opBalFmt)
                    .arg(it.closeBalFmt);
    }
    html += "</table>";

    m_printExportCtrl->exportHtmlToPdf(html, "Ledger_Directory.pdf", "", true);
}

void LedgerDirectoryWidget::onExportCsvClicked() {
    if (!m_printExportCtrl) return;

    QStringList headers = {"#", "Account Name", "Group", "City", "Phone", "GSTIN", "Opening Balance", "Closing Balance"};
    QVector<QStringList> rows;
    for (int i = 0; i < m_filteredItems.size(); ++i) {
        const auto& it = m_filteredItems[i];
        rows.append({
            QString::number(i + 1),
            it.name,
            it.groupName,
            it.city,
            it.mobile,
            it.gstin,
            it.opBalFmt,
            it.closeBalFmt
        });
    }

    m_printExportCtrl->exportTableToCsv(headers, rows, "Ledger_Directory.csv");
}

} // namespace MahadevERP
