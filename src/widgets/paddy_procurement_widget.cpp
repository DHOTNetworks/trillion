#include "paddy_procurement_widget.h"
#include "weighbridge_kanda_dialog.h"
#include "kbd_badge_button.h"
#include "custom_dialogs.h"
#include "account_search_box.h"
#include "../engine/fiscal_year_helper.h"
#include "../database_manager.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QHeaderView>
#include <QShortcut>
#include <QFileDialog>
#include <QFile>
#include <QTextStream>
#include <QDialog>
#include <QComboBox>

namespace MahadevERP {

PaddyProcurementWidget::PaddyProcurementWidget(PaddyArrivalsModel* arrivalsModel,
                                               PaddyProcurementController* procurementCtrl,
                                               PrintExportController* printExportCtrl,
                                               QWidget* parent)
    : QWidget(parent)
    , m_arrivalsModel(arrivalsModel)
    , m_procurementCtrl(procurementCtrl)
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
    loadArrivals(sDate, eDate);
}

void PaddyProcurementWidget::setupUi() {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(16, 16, 16, 16);
    mainLayout->setSpacing(12);

    // 1. Header Bar
    auto* headerLayout = new QHBoxLayout();
    auto* titleCol = new QVBoxLayout();
    auto* titleLabel = new QLabel("Paddy Procurement & Farmer Purchase Register", this);
    titleLabel->setStyleSheet("font-size: 18px; font-weight: bold; color: #0F172A;");
    auto* subLabel = new QLabel("Record raw paddy arrivals, calculate moisture & weight deductions, and manage farmer settlements.", this);
    subLabel->setStyleSheet("font-size: 11px; color: #64748B;");
    titleCol->addWidget(titleLabel);
    titleCol->addWidget(subLabel);
    headerLayout->addLayout(titleCol);
    headerLayout->addStretch();

    auto* newArrivalBtn = new KbdBadgeButton("+ New Paddy Arrival Slip", "F2", this);
    newArrivalBtn->setPrimaryColor("#16A34A", "#15803D");
    newArrivalBtn->setTextColor("#FFFFFF");
    connect(newArrivalBtn, &QPushButton::clicked, this, &PaddyProcurementWidget::openNewArrivalDialog);
    headerLayout->addWidget(newArrivalBtn);

    auto* kandaBtn = new QPushButton("Weighbridge (Kanda) ⚖️", this);
    kandaBtn->setStyleSheet("background-color: #EFF6FF; border: 1.5px solid #93C5FD; border-radius: 6px; padding: 6px 12px; font-weight: bold; color: #1D4ED8;");
    connect(kandaBtn, &QPushButton::clicked, this, [this]() {
        WeighbridgeKandaDialog::openKandaSlip(nullptr, 0, this);
    });
    headerLayout->addWidget(kandaBtn);

    auto* csvBtn = new QPushButton("Export CSV", this);
    csvBtn->setStyleSheet("background-color: #FFFFFF; border: 1px solid #CBD5E1; border-radius: 6px; padding: 6px 12px; font-weight: bold; color: #334155;");
    connect(csvBtn, &QPushButton::clicked, this, &PaddyProcurementWidget::onExportCsv);
    headerLayout->addWidget(csvBtn);

    auto* backBtn = new KbdBadgeButton("← Back", "Esc", this);
    backBtn->setPrimaryColor("#F1F5F9", "#E2E8F0");
    backBtn->setTextColor("#475569");
    connect(backBtn, &QPushButton::clicked, this, &PaddyProcurementWidget::backRequested);
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
    connect(m_fromDateEdit, &QDateEdit::dateChanged, this, &PaddyProcurementWidget::onDateFilterChanged);
    filterLayout->addWidget(m_fromDateEdit);

    filterLayout->addWidget(new QLabel("To Date:", filterCard));
    m_toDateEdit = new QDateEdit(QDate::currentDate(), filterCard);
    m_toDateEdit->setDisplayFormat("dd-MM-yyyy");
    m_toDateEdit->setCalendarPopup(true);
    connect(m_toDateEdit, &QDateEdit::dateChanged, this, &PaddyProcurementWidget::onDateFilterChanged);
    filterLayout->addWidget(m_toDateEdit);

    filterLayout->addSpacing(16);

    filterLayout->addWidget(new QLabel("Search Farmer / Variety:", filterCard));
    m_searchEdit = new QLineEdit(filterCard);
    m_searchEdit->setPlaceholderText("Search slip no, farmer, variety, status...");
    m_searchEdit->setClearButtonEnabled(true);
    connect(m_searchEdit, &QLineEdit::textChanged, this, &PaddyProcurementWidget::onSearchChanged);
    filterLayout->addWidget(m_searchEdit, 1);

    mainLayout->addWidget(filterCard);

    // ========================================================================
    // TIER 3: HIGH-PERFORMANCE DATA SURFACE (TABLE)
    // ========================================================================
    m_table = new QTableWidget(this);
    m_table->setColumnCount(12);
    m_table->setHorizontalHeaderLabels({
        "Slip No", "Date", "Farmer / Supplier", "Variety",
        "Bags", "Gross Qtl", "Moist %", "Deduct Qtl", "Net Qtl", "Rate (₹)", "Net Amt (₹)", "Status"
    });
    m_table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);
    m_table->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
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
    connect(m_table, &QTableWidget::cellDoubleClicked, this, &PaddyProcurementWidget::onTableDoubleClicked);
    mainLayout->addWidget(m_table, 1);

    // ========================================================================
    // TIER 4: SUMMARY METRICS FOOTER CARDS
    // ========================================================================
    auto* metricsLayout = new QHBoxLayout();
    metricsLayout->setSpacing(10);

    auto createMetricCard = [this, metricsLayout](const QString& title, const QString& initVal, const QString& color) {
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

    m_totalSlipsLabel = createMetricCard("TOTAL SLIPS", "0", "#2563EB");
    m_totalBagsLabel = createMetricCard("TOTAL BAGS", "0", "#0284C7");
    m_totalGrossLabel = createMetricCard("GROSS WEIGHT (QTL)", "0.00", "#7C3AED");
    m_totalDeductLabel = createMetricCard("DEDUCTION (QTL)", "0.00", "#DC2626");
    m_totalNetLabel = createMetricCard("NET WEIGHT (QTL)", "0.00", "#16A34A");
    m_totalAmountLabel = createMetricCard("TOTAL PURCHASE (₹)", "₹ 0.00", "#059669");

    mainLayout->addLayout(metricsLayout);

    // Keyboard Shortcuts
    connect(new QShortcut(QKeySequence(Qt::Key_Escape), this), &QShortcut::activated, this, &PaddyProcurementWidget::backRequested);
    connect(new QShortcut(QKeySequence(Qt::Key_F2), this), &QShortcut::activated, this, &PaddyProcurementWidget::openNewArrivalDialog);
}

void PaddyProcurementWidget::loadArrivals(const QDate& fromDate, const QDate& toDate) {
    if (fromDate.isValid()) m_fromDateEdit->setDate(fromDate);
    if (toDate.isValid()) m_toDateEdit->setDate(toDate);

    if (m_arrivalsModel) {
        m_arrivalsModel->reload_data();
    }

    populateTable();
}

void PaddyProcurementWidget::populateTable() {
    m_table->setRowCount(0);

    QString sDate = m_fromDateEdit->date().toString("yyyy-MM-dd");
    QString eDate = m_toDateEdit->date().toString("yyyy-MM-dd");
    QString q = m_searchEdit->text();

    QString sql = "SELECT id, slip_no, arrival_date, farmer_name, paddy_variety, bag_count, gross_weight_qtl, moisture_pct, moisture_deduction_qtl, net_weight_qtl, rate_per_qtl, net_amount, payment_status "
                  "FROM paddy_arrivals WHERE arrival_date >= '" + sDate + "' AND arrival_date <= '" + eDate + "' "
                  "ORDER BY arrival_date DESC, slip_no DESC;";
    QVariantList rows = DatabaseManager::instance().executeQuery(sql);

    int row = 0;
    int totSlips = 0;
    long long totBags = 0;
    double totGross = 0.0;
    double totDeduct = 0.0;
    double totNet = 0.0;
    double totAmt = 0.0;

    for (const QVariant& itemValRow : rows) {
        QVariantMap rowMap = itemValRow.toMap();
        QString slipNo = rowMap.value("slip_no").toString();
        QString dateVal = rowMap.value("arrival_date").toString();
        QString farmer = rowMap.value("farmer_name").toString();
        QString variety = rowMap.value("paddy_variety").toString();
        long long bags = rowMap.value("bag_count").toLongLong();
        double gross = rowMap.value("gross_weight_qtl").toDouble();
        double moist = rowMap.value("moisture_pct").toDouble();
        double deduct = rowMap.value("moisture_deduction_qtl").toDouble();
        double net = rowMap.value("net_weight_qtl").toDouble();
        double rate = rowMap.value("rate_per_qtl").toDouble();
        double netAmt = rowMap.value("net_amount").toDouble();
        QString status = rowMap.value("payment_status").toString();
        int id = rowMap.value("id").toInt();

        if (!q.isEmpty()) {
            QString searchTarget = (slipNo + " " + farmer + " " + variety + " " + status).toLower();
            if (!searchTarget.contains(q.toLower())) continue;
        }

        totSlips++;
        totBags += bags;
        totGross += gross;
        totDeduct += deduct;
        totNet += net;
        totAmt += netAmt;

        m_table->insertRow(row);
        auto* itemSlip = new QTableWidgetItem(slipNo);
        itemSlip->setData(Qt::UserRole, id);
        itemSlip->setFont(QFont("Segoe UI", 9, QFont::Bold));
        itemSlip->setForeground(QBrush(QColor("#1D4ED8")));
        m_table->setItem(row, 0, itemSlip);

        m_table->setItem(row, 1, new QTableWidgetItem(FiscalYearHelper::formatDisplayDate(dateVal)));
        m_table->setItem(row, 2, new QTableWidgetItem(farmer));
        m_table->setItem(row, 3, new QTableWidgetItem(variety));

        auto* itemBags = new QTableWidgetItem(QString::number(bags));
        itemBags->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        m_table->setItem(row, 4, itemBags);

        auto* itemGross = new QTableWidgetItem(QString::number(gross, 'f', 2));
        itemGross->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        m_table->setItem(row, 5, itemGross);

        auto* itemMoist = new QTableWidgetItem(QString::number(moist, 'f', 1) + "%");
        itemMoist->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        m_table->setItem(row, 6, itemMoist);

        auto* itemDeduct = new QTableWidgetItem(QString::number(deduct, 'f', 2));
        itemDeduct->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        itemDeduct->setForeground(QBrush(QColor("#DC2626")));
        m_table->setItem(row, 7, itemDeduct);

        auto* itemNet = new QTableWidgetItem(QString::number(net, 'f', 2));
        itemNet->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        itemNet->setFont(QFont("Segoe UI", 9, QFont::Bold));
        itemNet->setForeground(QBrush(QColor("#16A34A")));
        m_table->setItem(row, 8, itemNet);

        auto* itemRate = new QTableWidgetItem(QString::number(rate, 'f', 2));
        itemRate->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        m_table->setItem(row, 9, itemRate);

        auto* itemTot = new QTableWidgetItem(QString::number(netAmt, 'f', 2));
        itemTot->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        itemTot->setFont(QFont("Segoe UI", 9, QFont::Bold));
        itemTot->setForeground(QBrush(QColor("#15803D")));
        m_table->setItem(row, 10, itemTot);

        auto* itemStatus = new QTableWidgetItem(status);
        itemStatus->setTextAlignment(Qt::AlignCenter);
        m_table->setItem(row, 11, itemStatus);

        row++;
    }

    m_totalSlipsLabel->setText(QString::number(totSlips));
    m_totalBagsLabel->setText(QString::number(totBags));
    m_totalGrossLabel->setText(QString::number(totGross, 'f', 2));
    m_totalDeductLabel->setText(QString::number(totDeduct, 'f', 2));
    m_totalNetLabel->setText(QString::number(totNet, 'f', 2));
    m_totalAmountLabel->setText("₹ " + QString::number(totAmt, 'f', 2));
}

void PaddyProcurementWidget::onSearchChanged(const QString& query) {
    Q_UNUSED(query);
    populateTable();
}

void PaddyProcurementWidget::onDateFilterChanged() {
    populateTable();
}

void PaddyProcurementWidget::focusTable() {
    if (m_table->rowCount() > 0) {
        m_table->setFocus();
        m_table->selectRow(0);
    } else {
        m_searchEdit->setFocus();
    }
}

void PaddyProcurementWidget::onTableDoubleClicked(int row, int col) {
    Q_UNUSED(col);
    if (row < 0 || row >= m_table->rowCount()) return;
    // Drilldown or view info
}

void PaddyProcurementWidget::openNewArrivalDialog() {
    QDialog dlg(this);
    dlg.setWindowTitle("Record New Paddy Arrival Slip • Mahadev ERP");
    dlg.setModal(true);
    dlg.resize(560, 420);
    dlg.setStyleSheet(
        "QDialog { background-color: #F8FAFC; }"
        "QLabel { color: #475569; font-size: 12px; font-weight: 500; }"
        "QLineEdit, QComboBox, QDateEdit { background-color: #FFFFFF; border: 1.5px solid #CBD5E1; border-radius: 6px; padding: 6px 10px; font-size: 13px; color: #0F172A; }"
        "QLineEdit:focus, QComboBox:focus, QDateEdit:focus { border-color: #2563EB; background-color: #F0FDF4; }"
    );

    auto* layout = new QVBoxLayout(&dlg);
    layout->setContentsMargins(20, 20, 20, 20);
    layout->setSpacing(12);

    auto* titleLabel = new QLabel("New Paddy Arrival Entry", &dlg);
    titleLabel->setStyleSheet("font-size: 16px; font-weight: bold; color: #0F172A;");
    layout->addWidget(titleLabel);

    auto* grid = new QGridLayout();
    grid->setSpacing(10);

    auto* farmerEdit = new AccountSearchBox(&dlg);
    farmerEdit->setPlaceholderText("Farmer / Supplier Name");
    grid->addWidget(new QLabel("Farmer / Supplier *:"), 0, 0);
    grid->addWidget(farmerEdit, 0, 1, 1, 3);

    auto* varietyCombo = new QComboBox(&dlg);
    varietyCombo->setEditable(true);
    QVariantList stockItems = DatabaseManager::instance().executeQuery(
        "SELECT DISTINCT name FROM stock_items WHERE name != '' ORDER BY name ASC;"
    );
    for (const QVariant& row : stockItems) {
        QString vName = row.toMap().value("name").toString();
        if (!vName.isEmpty()) {
            varietyCombo->addItem(vName);
        }
    }
    grid->addWidget(new QLabel("Paddy Variety *:"), 1, 0);
    grid->addWidget(varietyCombo, 1, 1);

    auto* bagsEdit = new QLineEdit(&dlg);
    bagsEdit->setPlaceholderText("e.g. 200");
    grid->addWidget(new QLabel("Total Bags *:"), 1, 2);
    grid->addWidget(bagsEdit, 1, 3);

    auto* grossEdit = new QLineEdit(&dlg);
    grossEdit->setPlaceholderText("Gross Qtl e.g. 150.0");
    grid->addWidget(new QLabel("Gross Weight (Qtl) *:"), 2, 0);
    grid->addWidget(grossEdit, 2, 1);

    auto* moistEdit = new QLineEdit("14.0", &dlg);
    grid->addWidget(new QLabel("Moisture %:"), 2, 2);
    grid->addWidget(moistEdit, 2, 3);

    auto* rateEdit = new QLineEdit(&dlg);
    rateEdit->setPlaceholderText("e.g. 2450.00");
    grid->addWidget(new QLabel("Rate (₹/Qtl) *:"), 3, 0);
    grid->addWidget(rateEdit, 3, 1);

    auto* netCalcLabel = new QLabel("Net: 0.00 Qtl | Total: ₹ 0.00", &dlg);
    netCalcLabel->setStyleSheet("font-size: 13px; font-weight: bold; color: #16A34A; background-color: #DCFCE7; padding: 6px 10px; border-radius: 4px;");
    grid->addWidget(netCalcLabel, 4, 0, 1, 4);

    layout->addLayout(grid);

    auto updateCalc = [grossEdit, moistEdit, rateEdit, netCalcLabel]() {
        double gross = grossEdit->text().toDouble();
        double moist = moistEdit->text().toDouble();
        double rate = rateEdit->text().toDouble();

        double deduct = 0.0;
        if (moist > 14.0) {
            deduct = gross * ((moist - 14.0) / 100.0);
        }
        double net = std::max(0.0, gross - deduct);
        double tot = net * rate;
        netCalcLabel->setText(QString("Net Wt: %1 Qtl (Deduct: %2 Qtl) | Total: ₹ %3")
            .arg(QString::number(net, 'f', 2), QString::number(deduct, 'f', 2), QString::number(tot, 'f', 2)));
    };

    connect(grossEdit, &QLineEdit::textChanged, &dlg, updateCalc);
    connect(moistEdit, &QLineEdit::textChanged, &dlg, updateCalc);
    connect(rateEdit, &QLineEdit::textChanged, &dlg, updateCalc);

    layout->addStretch();

    auto* btmLayout = new QHBoxLayout();
    auto* cancelBtn = new QPushButton("Cancel", &dlg);
    connect(cancelBtn, &QPushButton::clicked, &dlg, &QDialog::reject);
    btmLayout->addWidget(cancelBtn);
    btmLayout->addStretch();

    auto* saveBtn = new QPushButton("Record Arrival Slip", &dlg);
    saveBtn->setStyleSheet("background-color: #16A34A; color: #FFF; font-weight: bold; padding: 8px 18px; border-radius: 6px;");
    connect(saveBtn, &QPushButton::clicked, [&]() {
        QString fName = farmerEdit->text().trimmed();
        if (fName.isEmpty()) {
            CustomMessageBox::showWarning(&dlg, "Validation Error", "Please enter Farmer Name.");
            farmerEdit->setFocus();
            return;
        }
        double gross = grossEdit->text().toDouble();
        if (gross <= 0.0) {
            CustomMessageBox::showWarning(&dlg, "Validation Error", "Please enter Gross Weight.");
            grossEdit->setFocus();
            return;
        }

        bool ok = false;
        if (m_procurementCtrl) {
            m_procurementCtrl->setFarmerName(fName);
            m_procurementCtrl->setPaddyVariety(varietyCombo->currentText());
            m_procurementCtrl->setBagCount(bagsEdit->text().toInt());
            m_procurementCtrl->setGrossWeightQtl(gross);
            m_procurementCtrl->setMoisturePct(moistEdit->text().toDouble());
            m_procurementCtrl->setRatePerQtl(rateEdit->text().toDouble());
            ok = m_procurementCtrl->saveArrivalSlip();
        } else if (m_arrivalsModel) {
            ok = m_arrivalsModel->add_arrival(
                fName,
                varietyCombo->currentText(),
                QDate::currentDate().toString("yyyy-MM-dd"),
                bagsEdit->text().toInt(),
                gross,
                moistEdit->text().toDouble(),
                rateEdit->text().toDouble(),
                0.0,
                "Unpaid"
            );
        }

        if (ok) {
            CustomMessageBox::showInformation(this, "Success", "Paddy Arrival recorded successfully.");
            dlg.accept();
            loadArrivals(m_fromDateEdit->date(), m_toDateEdit->date());
        } else {
            CustomMessageBox::showCritical(&dlg, "Save Failed", "Failed to save Paddy Arrival slip.");
        }
    });
    btmLayout->addWidget(saveBtn);
    layout->addLayout(btmLayout);

    dlg.exec();
}

void PaddyProcurementWidget::onPrintRegister() {
}

void PaddyProcurementWidget::onExportCsv() {
    QString fileName = QFileDialog::getSaveFileName(this, "Export Paddy Procurement CSV", "PaddyProcurement.csv", "CSV Files (*.csv)");
    if (fileName.isEmpty()) return;

    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        CustomMessageBox::showCritical(this, "File Error", "Could not open file for writing.");
        return;
    }

    QTextStream out(&file);
    out << "Slip No,Date,Farmer,Variety,Bags,Gross Qtl,Moisture %,Deduct Qtl,Net Qtl,Rate,Net Amount,Status\n";

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
    CustomMessageBox::showInformation(this, "Export Complete", "Paddy Procurement register exported to " + fileName);
}

} // namespace MahadevERP
