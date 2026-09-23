#include "transport_dispatch_widget.h"
#include "weighbridge_kanda_dialog.h"
#include "kbd_badge_button.h"
#include "custom_dialogs.h"
#include "../engine/fiscal_year_helper.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QHeaderView>
#include <QShortcut>
#include <QFileDialog>
#include <QFile>
#include <QTextStream>

namespace MahadevERP {

TransportDispatchWidget::TransportDispatchWidget(TransportDispatchController* controller,
                                                 PrintExportController* printExportCtrl,
                                                 QWidget* parent)
    : QWidget(parent)
    , m_controller(controller)
    , m_printExportCtrl(printExportCtrl)
{
    setupUi();
    FiscalYearInfo activeFy = FiscalYearHelper::getActiveFiscalYear();
    QDate sDate = QDate::fromString(activeFy.startDate, "yyyy-MM-dd");
    QDate eDate = QDate::fromString(activeFy.endDate, "yyyy-MM-dd");
    if (!sDate.isValid()) sDate = QDate(QDate::currentDate().month() < 4 ? QDate::currentDate().year() - 1 : QDate::currentDate().year(), 4, 1);
    if (!eDate.isValid()) eDate = QDate::currentDate();
    m_fromDateEdit->setDate(sDate);
    m_toDateEdit->setDate(eDate);
    loadData(sDate, eDate);
}

void TransportDispatchWidget::setupUi() {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(16, 16, 16, 16);
    mainLayout->setSpacing(12);

    // 1. Header Bar
    auto* headerLayout = new QHBoxLayout();
    auto* titleCol = new QVBoxLayout();
    auto* titleLabel = new QLabel("Transport Dispatch & Gate Pass Register", this);
    titleLabel->setStyleSheet("font-size: 18px; font-weight: bold; color: #0F172A;");
    auto* subLabel = new QLabel("Audit lorry dispatches, weighbridge out-turn records, vehicle tracking, and transporter freight settlements.", this);
    subLabel->setStyleSheet("font-size: 11px; color: #64748B;");
    titleCol->addWidget(titleLabel);
    titleCol->addWidget(subLabel);
    headerLayout->addLayout(titleCol);
    headerLayout->addStretch();

    auto* newSlipBtn = new KbdBadgeButton("+ New Dispatch Slip", "F2", this);
    newSlipBtn->setPrimaryColor("#16A34A", "#15803D");
    newSlipBtn->setTextColor("#FFFFFF");
    connect(newSlipBtn, &QPushButton::clicked, this, &TransportDispatchWidget::openNewDispatchDialog);
    headerLayout->addWidget(newSlipBtn);

    auto* csvBtn = new QPushButton("Export CSV", this);
    csvBtn->setStyleSheet("background-color: #FFFFFF; border: 1px solid #CBD5E1; border-radius: 6px; padding: 6px 12px; font-weight: bold; color: #334155;");
    connect(csvBtn, &QPushButton::clicked, this, &TransportDispatchWidget::onExportCsv);
    headerLayout->addWidget(csvBtn);

    auto* backBtn = new KbdBadgeButton("← Back to Dashboard", "Esc", this);
    backBtn->setPrimaryColor("#F1F5F9", "#E2E8F0");
    backBtn->setTextColor("#475569");
    connect(backBtn, &QPushButton::clicked, this, &TransportDispatchWidget::backRequested);
    headerLayout->addWidget(backBtn);
    mainLayout->addLayout(headerLayout);

    // ========================================================================
    // TIER 2: FILTER & DATE RANGE BAR
    // ========================================================================
    auto* filterCard = new QFrame(this);
    filterCard->setStyleSheet(
        "QFrame { background-color: #FFFFFF; border: 1px solid #CBD5E1; border-radius: 8px; }"
        "QLabel { color: #475569; font-weight: 700; font-size: 11.5px; border: none; background: transparent; }"
        "QDateEdit, QLineEdit { background-color: #FFFFFF; border: 1.5px solid #CBD5E1; border-radius: 6px; padding: 4px 8px; font-size: 12px; color: #0F172A; font-weight: 600; }"
        "QDateEdit:focus, QLineEdit:focus { border-color: #2563EB; background-color: #F8FAFC; }"
    );
    auto* filterLayout = new QHBoxLayout(filterCard);
    filterLayout->setContentsMargins(12, 8, 12, 8);
    filterLayout->setSpacing(12);

    filterLayout->addWidget(new QLabel("From Date:", filterCard));
    FiscalYearInfo initFy = FiscalYearHelper::getActiveFiscalYear();
    QDate initSDate = QDate::fromString(initFy.startDate, "yyyy-MM-dd");
    if (!initSDate.isValid()) initSDate = QDate(QDate::currentDate().month() < 4 ? QDate::currentDate().year() - 1 : QDate::currentDate().year(), 4, 1);
    m_fromDateEdit = new QDateEdit(initSDate, filterCard);
    m_fromDateEdit->setDisplayFormat("dd-MM-yyyy");
    m_fromDateEdit->setCalendarPopup(true);
    connect(m_fromDateEdit, &QDateEdit::dateChanged, this, &TransportDispatchWidget::onDateFilterChanged);
    filterLayout->addWidget(m_fromDateEdit);

    filterLayout->addWidget(new QLabel("To Date:", filterCard));
    m_toDateEdit = new QDateEdit(QDate::currentDate(), filterCard);
    m_toDateEdit->setDisplayFormat("dd-MM-yyyy");
    m_toDateEdit->setCalendarPopup(true);
    connect(m_toDateEdit, &QDateEdit::dateChanged, this, &TransportDispatchWidget::onDateFilterChanged);
    filterLayout->addWidget(m_toDateEdit);

    filterLayout->addSpacing(16);

    filterLayout->addWidget(new QLabel("Search Query:", filterCard));
    m_searchEdit = new QLineEdit(filterCard);
    m_searchEdit->setPlaceholderText("Search slip no, vehicle, party, transporter, destination...");
    m_searchEdit->setClearButtonEnabled(true);
    connect(m_searchEdit, &QLineEdit::textChanged, this, &TransportDispatchWidget::onSearchChanged);
    filterLayout->addWidget(m_searchEdit, 1);

    mainLayout->addWidget(filterCard);

    // ========================================================================
    // TIER 3: HIGH-PERFORMANCE DATA SURFACE (TABLE)
    // ========================================================================
    m_table = new QTableWidget(this);
    m_table->setColumnCount(12);
    m_table->setHorizontalHeaderLabels({
        "Slip No", "Date", "Vehicle No", "Party / Consignee", "Commodity",
        "Bags", "Net Wt (Qtl)", "Transporter", "Destination", "Total Freight (₹)", "Balance (₹)", "Status"
    });
    m_table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Stretch);
    for (int c = 4; c < 12; ++c) {
        m_table->horizontalHeader()->setSectionResizeMode(c, QHeaderView::ResizeToContents);
    }
    m_table->verticalHeader()->setVisible(false);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setAlternatingRowColors(true);
    m_table->setStyleSheet(
        "QTableWidget { background-color: #FFFFFF; border: 1px solid #CBD5E1; border-radius: 8px; gridline-color: #F1F5F9; font-size: 12px; color: #0F172A; }"
        "QTableWidget::item { padding: 6px 10px; }"
        "QTableWidget::item:selected { background-color: #EFF6FF; color: #1E3A8A; font-weight: bold; }"
        "QHeaderView::section { background-color: #0F172A; color: #FFFFFF; font-weight: 800; font-size: 11px; padding: 6px 10px; border: none; }"
    );
    connect(m_table, &QTableWidget::cellDoubleClicked, this, &TransportDispatchWidget::onTableDoubleClicked);
    mainLayout->addWidget(m_table, 1);

    // ========================================================================
    // TIER 4: SUMMARY METRICS FOOTER CARDS
    // ========================================================================
    auto* metricsLayout = new QHBoxLayout();
    metricsLayout->setSpacing(10);

    auto createMetric = [this, metricsLayout](const QString& title, const QString& initVal, const QString& color) {
        auto* card = new QFrame(this);
        card->setStyleSheet(
            "QFrame { background-color: #FFFFFF; border: 1px solid #E2E8F0; border-radius: 8px; border-left: 4px solid " + color + "; }"
            "QLabel { border: none; background: transparent; }"
        );
        auto* cLayout = new QVBoxLayout(card);
        cLayout->setContentsMargins(10, 6, 10, 6);
        cLayout->setSpacing(2);

        auto* tLabel = new QLabel(title, card);
        tLabel->setStyleSheet("font-size: 9.5px; font-weight: 800; color: #64748B; letter-spacing: 0.5px; border: none; background: transparent;");
        auto* vLabel = new QLabel(initVal, card);
        vLabel->setStyleSheet("font-size: 14px; font-weight: 800; color: #0F172A; border: none; background: transparent;");
        cLayout->addWidget(tLabel);
        cLayout->addWidget(vLabel);
        metricsLayout->addWidget(card);
        return vLabel;
    };

    m_totalDispatchesLabel = createMetric("TOTAL DISPATCHES", "0", "#2563EB");
    m_totalNetWeightLabel = createMetric("TOTAL NET WEIGHT (QTL)", "0.00", "#16A34A");
    m_totalFreightLabel = createMetric("TOTAL FREIGHT CHARGES", "₹ 0.00", "#7C3AED");
    m_totalBalancePayableLabel = createMetric("BALANCE FREIGHT PAYABLE", "₹ 0.00", "#DC2626");

    mainLayout->addLayout(metricsLayout);

    // Keyboard Shortcuts
    connect(new QShortcut(QKeySequence(Qt::Key_Escape), this), &QShortcut::activated, this, &TransportDispatchWidget::backRequested);
    connect(new QShortcut(QKeySequence(Qt::Key_F2), this), &QShortcut::activated, this, &TransportDispatchWidget::openNewDispatchDialog);
}

void TransportDispatchWidget::loadData(const QDate& fromDate, const QDate& toDate) {
    if (fromDate.isValid()) m_fromDateEdit->setDate(fromDate);
    if (toDate.isValid()) m_toDateEdit->setDate(toDate);

    if (m_controller) {
        m_controller->reload();
    }

    populateTable();
}

void TransportDispatchWidget::populateTable() {
    m_table->setRowCount(0);

    if (!m_controller) return;

    auto* mdl = m_controller->model();
    int count = mdl ? mdl->rowCount() : 0;
    QString q = m_searchEdit->text();

    int row = 0;
    double totWeight = 0.0;
    double totFreight = 0.0;
    double totBal = 0.0;

    for (int i = 0; i < count; ++i) {
        auto map = mdl->get(i);
        QString slipNo = map.value("slipNo").toString();
        QString dateVal = map.value("dispatchDate").toString();
        QString veh = map.value("vehicleNo").toString();
        QString party = map.value("partyName").toString();
        QString item = map.value("itemName").toString();
        QString dest = map.value("destination").toString();
        QString trans = map.value("transporterName").toString();
        QString status = map.value("freightPaymentStatus").toString();
        int id = map.value("id").toInt();

        if (!q.isEmpty()) {
            QString searchTarget = (slipNo + " " + veh + " " + party + " " + item + " " + dest + " " + trans + " " + status).toLower();
            if (!searchTarget.contains(q.toLower())) continue;
        }

        int bags = map.value("bagCount").toInt();
        double weight = map.value("netWeightQtl").toDouble();
        double freight = map.value("totalFreight").toDouble();
        double bal = map.value("balanceFreight").toDouble();

        totWeight += weight;
        totFreight += freight;
        totBal += bal;

        m_table->insertRow(row);

        auto* itemSlip = new QTableWidgetItem(slipNo);
        itemSlip->setData(Qt::UserRole, id);
        itemSlip->setFont(QFont("Segoe UI", 9, QFont::Bold));
        itemSlip->setForeground(QBrush(QColor("#1D4ED8")));
        m_table->setItem(row, 0, itemSlip);

        m_table->setItem(row, 1, new QTableWidgetItem(FiscalYearHelper::formatDisplayDate(dateVal)));

        auto* itemVeh = new QTableWidgetItem(veh);
        itemVeh->setFont(QFont("Segoe UI", 9, QFont::Bold));
        m_table->setItem(row, 2, itemVeh);

        m_table->setItem(row, 3, new QTableWidgetItem(party));
        m_table->setItem(row, 4, new QTableWidgetItem(item));

        auto* itemBags = new QTableWidgetItem(QString::number(bags));
        itemBags->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        m_table->setItem(row, 5, itemBags);

        auto* itemWt = new QTableWidgetItem(QString::number(weight, 'f', 2));
        itemWt->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        itemWt->setFont(QFont("Segoe UI", 9, QFont::Bold));
        itemWt->setForeground(QBrush(QColor("#16A34A")));
        m_table->setItem(row, 6, itemWt);

        m_table->setItem(row, 7, new QTableWidgetItem(trans));
        m_table->setItem(row, 8, new QTableWidgetItem(dest));

        auto* itemFr = new QTableWidgetItem(QString::number(freight, 'f', 2));
        itemFr->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        m_table->setItem(row, 9, itemFr);

        auto* itemBal = new QTableWidgetItem(QString::number(bal, 'f', 2));
        itemBal->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        itemBal->setFont(QFont("Segoe UI", 9, QFont::Bold));
        itemBal->setForeground(QBrush(QColor("#DC2626")));
        m_table->setItem(row, 10, itemBal);

        auto* itemSt = new QTableWidgetItem(status);
        itemSt->setTextAlignment(Qt::AlignCenter);
        m_table->setItem(row, 11, itemSt);

        row++;
    }

    m_totalDispatchesLabel->setText(QString::number(row));
    m_totalNetWeightLabel->setText(QString::number(totWeight, 'f', 2));
    m_totalFreightLabel->setText("₹ " + QString::number(totFreight, 'f', 2));
    m_totalBalancePayableLabel->setText("₹ " + QString::number(totBal, 'f', 2));
}

void TransportDispatchWidget::onSearchChanged(const QString& query) {
    Q_UNUSED(query);
    populateTable();
}

void TransportDispatchWidget::onDateFilterChanged() {
    loadData(m_fromDateEdit->date(), m_toDateEdit->date());
}

void TransportDispatchWidget::focusTable() {
    if (m_table->rowCount() > 0) {
        m_table->setFocus();
        m_table->selectRow(0);
    } else {
        m_searchEdit->setFocus();
    }
}

void TransportDispatchWidget::onTableDoubleClicked(int row, int col) {
    Q_UNUSED(col);
    if (row < 0 || row >= m_table->rowCount()) return;

    auto* item = m_table->item(row, 0);
    if (!item) return;

    int id = item->data(Qt::UserRole).toInt();
    if (id > 0) {
        bool ok = WeighbridgeKandaDialog::openKandaSlip(m_controller, id, this);
        if (ok) {
            loadData(m_fromDateEdit->date(), m_toDateEdit->date());
        }
    }
}

void TransportDispatchWidget::openNewDispatchDialog() {
    bool ok = WeighbridgeKandaDialog::openKandaSlip(m_controller, 0, this);
    if (ok) {
        loadData(m_fromDateEdit->date(), m_toDateEdit->date());
    }
}

void TransportDispatchWidget::onExportCsv() {
    QString fileName = QFileDialog::getSaveFileName(this, "Export Transport Dispatch CSV", "TransportDispatch.csv", "CSV Files (*.csv)");
    if (fileName.isEmpty()) return;

    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        CustomMessageBox::showCritical(this, "File Error", "Could not open file for writing.");
        return;
    }

    QTextStream out(&file);
    out << "Slip No,Date,Vehicle No,Party,Commodity,Bags,Net Weight Qtl,Transporter,Destination,Total Freight,Balance,Status\n";

    for (int r = 0; r < m_table->rowCount(); ++r) {
        QStringList rowVals;
        for (int c = 0; c < m_table->columnCount(); ++c) {
            auto* it = m_table->item(r, c);
            QString txt = it ? it->text() : "";
            txt.replace("\"", "\"\"");
            rowVals.append("\"" + txt + "\"");
        }
        out << rowVals.join(",") << "\n";
    }

    file.close();
    CustomMessageBox::showInformation(this, "Export Complete", "Transport Dispatch register exported to " + fileName);
}

} // namespace MahadevERP
