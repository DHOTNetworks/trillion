#include "weighbridge_kanda_dialog.h"
#include "custom_dialogs.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QFrame>
#include <QShortcut>

namespace MahadevERP {

WeighbridgeKandaDialog::WeighbridgeKandaDialog(TransportDispatchController* controller, int editDispatchId, QWidget* parent)
    : QDialog(parent)
    , m_controller(controller)
    , m_editDispatchId(editDispatchId)
{
    setWindowTitle("Weighbridge (Kanda) Slip & Transport Dispatch • Mahadev ERP");
    setModal(true);
    resize(960, 560);
    setStyleSheet(
        "QDialog { background-color: #F8FAFC; font-family: 'Segoe UI', -apple-system, BlinkMacSystemFont, Roboto, sans-serif; }"
        "QLabel { color: #475569; font-size: 11.5px; font-weight: 700; border: none; background: transparent; }"
        "QLineEdit, QDateEdit, QTimeEdit, QComboBox { background-color: #FFFFFF; border: 1.5px solid #CBD5E1; border-radius: 6px; padding: 4px 8px; font-size: 12px; color: #0F172A; font-weight: 600; }"
        "QLineEdit:focus, QDateEdit:focus, QTimeEdit:focus, QComboBox:focus { border-color: #2563EB; background-color: #F8FAFC; }"
    );

    setupUi();
    if (m_editDispatchId > 0) {
        loadDispatch(m_editDispatchId);
    } else {
        onResetForm();
    }
}

bool WeighbridgeKandaDialog::openKandaSlip(TransportDispatchController* controller, int editDispatchId, QWidget* parent) {
    WeighbridgeKandaDialog dlg(controller, editDispatchId, parent);
    return (dlg.exec() == QDialog::Accepted);
}

void WeighbridgeKandaDialog::setupUi() {
    auto* rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(16, 14, 16, 14);
    rootLayout->setSpacing(10);

    const QString cardStyle = "QFrame { background-color: #FFFFFF; border: 1px solid #E2E8F0; border-radius: 8px; }";
    const QString sectionHeaderStyle = "color: #2563EB; font-size: 10.5px; font-weight: 800; letter-spacing: 0.5px; border: none; background: transparent;";

    // ========================================================================
    // TIER 1: HEADER BAR CARD
    // ========================================================================
    auto* headerCard = new QFrame(this);
    headerCard->setFixedHeight(54);
    headerCard->setStyleSheet(cardStyle);
    auto* headerLayout = new QHBoxLayout(headerCard);
    headerLayout->setContentsMargins(14, 6, 14, 6);
    headerLayout->setSpacing(10);

    auto* titleCol = new QVBoxLayout();
    titleCol->setSpacing(1);
    auto* titleLabel = new QLabel("Weighbridge (Kanda) Slip Entry & Out-turn Calculation", headerCard);
    titleLabel->setStyleSheet("font-size: 15px; font-weight: 800; color: #0F172A; border: none; background: transparent;");
    auto* subLabel = new QLabel("Calculate Gross/Tare/Net weight with bag deduction and automated freight matrix", headerCard);
    subLabel->setStyleSheet("font-size: 11px; color: #64748B; border: none; background: transparent;");
    titleCol->addWidget(titleLabel);
    titleCol->addWidget(subLabel);
    headerLayout->addLayout(titleCol);
    headerLayout->addStretch(1);

    rootLayout->addWidget(headerCard);

    // ========================================================================
    // TIER 2 & 3: 2-COLUMN SINGLE-SLATE CARDS (NO VERTICAL SCROLLING)
    // ========================================================================
    auto* mainGrid = new QGridLayout();
    mainGrid->setSpacing(10);

    // ------------------------------------------------------------------------
    // CARD 1: DISPATCH & PARTY IDENTIFICATION (Top Left)
    // ------------------------------------------------------------------------
    auto* card1 = new QFrame(this);
    card1->setStyleSheet(cardStyle);
    auto* layout1 = new QVBoxLayout(card1);
    layout1->setContentsMargins(12, 10, 12, 10);
    layout1->setSpacing(8);

    auto* header1 = new QLabel("1. DISPATCH & PARTY IDENTIFICATION", card1);
    header1->setStyleSheet(sectionHeaderStyle);
    layout1->addWidget(header1);

    auto* grid1 = new QGridLayout();
    grid1->setHorizontalSpacing(10);
    grid1->setVerticalSpacing(6);
    grid1->setColumnStretch(1, 1);
    grid1->setColumnStretch(3, 1);

    // Row 0: Slip No, Date, Time
    grid1->addWidget(new QLabel("Slip No:", card1), 0, 0);
    m_slipNoEdit = new QLineEdit(card1);
    m_slipNoEdit->setReadOnly(true);
    m_slipNoEdit->setStyleSheet("background-color: #F8FAFC; border: 1.5px solid #CBD5E1; border-radius: 6px; padding: 4px 8px; font-weight: 800; color: #1D4ED8; font-family: 'Consolas', monospace;");
    grid1->addWidget(m_slipNoEdit, 0, 1);

    grid1->addWidget(new QLabel("Date / Time:", card1), 0, 2);
    auto* dateTimeBox = new QHBoxLayout();
    dateTimeBox->setSpacing(4);
    dateTimeBox->setContentsMargins(0, 0, 0, 0);

    m_dateEdit = new QDateEdit(QDate::currentDate(), card1);
    m_dateEdit->setDisplayFormat("dd-MM-yyyy");
    m_dateEdit->setCalendarPopup(true);
    dateTimeBox->addWidget(m_dateEdit, 3);

    m_timeEdit = new QTimeEdit(QTime::currentTime(), card1);
    m_timeEdit->setDisplayFormat("HH:mm");
    dateTimeBox->addWidget(m_timeEdit, 2);
    grid1->addLayout(dateTimeBox, 0, 3);

    // Row 1: Invoice Ref, Party / Buyer
    grid1->addWidget(new QLabel("Invoice Ref:", card1), 1, 0);
    m_invoiceNoEdit = new QLineEdit(card1);
    m_invoiceNoEdit->setPlaceholderText("Optional Invoice Ref");
    grid1->addWidget(m_invoiceNoEdit, 1, 1);

    grid1->addWidget(new QLabel("Party / Buyer *:", card1), 1, 2);
    m_partyNameEdit = new AccountSearchBox(card1);
    m_partyNameEdit->setPlaceholderText("Search Buyer / Consignee...");
    grid1->addWidget(m_partyNameEdit, 1, 3);

    // Row 2: Item / Commodity
    grid1->addWidget(new QLabel("Item / Commodity *:", card1), 2, 0);
    m_itemNameEdit = new QLineEdit(card1);
    m_itemNameEdit->setPlaceholderText("Commodity / Rice Variety (e.g. Basmati)");
    grid1->addWidget(m_itemNameEdit, 2, 1, 1, 3);

    layout1->addLayout(grid1);
    mainGrid->addWidget(card1, 0, 0);

    // ------------------------------------------------------------------------
    // CARD 2: TRANSPORT & VEHICLE PARTICULARS (Top Right)
    // ------------------------------------------------------------------------
    auto* card2 = new QFrame(this);
    card2->setStyleSheet(cardStyle);
    auto* layout2 = new QVBoxLayout(card2);
    layout2->setContentsMargins(12, 10, 12, 10);
    layout2->setSpacing(8);

    auto* header2 = new QLabel("2. TRANSPORT & VEHICLE PARTICULARS", card2);
    header2->setStyleSheet(sectionHeaderStyle);
    layout2->addWidget(header2);

    auto* grid2 = new QGridLayout();
    grid2->setHorizontalSpacing(10);
    grid2->setVerticalSpacing(6);
    grid2->setColumnStretch(1, 1);
    grid2->setColumnStretch(3, 1);

    // Row 0: Vehicle No *, Driver Name
    grid2->addWidget(new QLabel("Vehicle No *:", card2), 0, 0);
    m_vehicleNoEdit = new QLineEdit(card2);
    m_vehicleNoEdit->setPlaceholderText("e.g. PB10-AB-1234");
    m_vehicleNoEdit->setStyleSheet("font-weight: 800; color: #0F172A; text-transform: uppercase; font-family: 'Consolas', monospace;");
    grid2->addWidget(m_vehicleNoEdit, 0, 1);

    grid2->addWidget(new QLabel("Driver Name:", card2), 0, 2);
    m_driverNameEdit = new QLineEdit(card2);
    m_driverNameEdit->setPlaceholderText("Driver Name");
    grid2->addWidget(m_driverNameEdit, 0, 3);

    // Row 1: Driver Phone, Destination
    grid2->addWidget(new QLabel("Driver Phone:", card2), 1, 0);
    m_driverPhoneEdit = new QLineEdit(card2);
    m_driverPhoneEdit->setPlaceholderText("10-digit Mobile");
    grid2->addWidget(m_driverPhoneEdit, 1, 1);

    grid2->addWidget(new QLabel("Destination:", card2), 1, 2);
    m_destinationEdit = new QLineEdit(card2);
    m_destinationEdit->setPlaceholderText("City / Delivery Location");
    grid2->addWidget(m_destinationEdit, 1, 3);

    // Row 2: Transporter, Transporter GST
    grid2->addWidget(new QLabel("Transporter:", card2), 2, 0);
    m_transporterNameEdit = new AccountSearchBox(card2);
    m_transporterNameEdit->setPlaceholderText("Search Transport Company...");
    grid2->addWidget(m_transporterNameEdit, 2, 1);

    grid2->addWidget(new QLabel("Transporter GST:", card2), 2, 2);
    m_transporterGstinEdit = new QLineEdit(card2);
    m_transporterGstinEdit->setPlaceholderText("15-digit GSTIN");
    grid2->addWidget(m_transporterGstinEdit, 2, 3);

    layout2->addLayout(grid2);
    mainGrid->addWidget(card2, 0, 1);

    // ------------------------------------------------------------------------
    // CARD 3: WEIGHBRIDGE GROSS / TARE / NET CALCULATION (Bottom Left)
    // ------------------------------------------------------------------------
    auto* card3 = new QFrame(this);
    card3->setStyleSheet(cardStyle);
    auto* layout3 = new QVBoxLayout(card3);
    layout3->setContentsMargins(12, 10, 12, 10);
    layout3->setSpacing(8);

    auto* header3 = new QLabel("3. WEIGHBRIDGE GROSS / TARE / NET CALCULATION", card3);
    header3->setStyleSheet(sectionHeaderStyle);
    layout3->addWidget(header3);

    auto* grid3 = new QGridLayout();
    grid3->setHorizontalSpacing(10);
    grid3->setVerticalSpacing(6);
    grid3->setColumnStretch(1, 1);
    grid3->setColumnStretch(3, 1);
    grid3->setColumnStretch(5, 1);

    // Row 0: Total Bags, Packing (Kg), Bag Tare (Kg)
    grid3->addWidget(new QLabel("Total Bags:", card3), 0, 0);
    m_bagCountEdit = new QLineEdit(card3);
    m_bagCountEdit->setPlaceholderText("0");
    m_bagCountEdit->setAlignment(Qt::AlignRight);
    connect(m_bagCountEdit, &QLineEdit::textChanged, this, &WeighbridgeKandaDialog::onRecalculate);
    grid3->addWidget(m_bagCountEdit, 0, 1);

    grid3->addWidget(new QLabel("Packing (Kg):", card3), 0, 2);
    m_packingKgEdit = new QLineEdit("50.0", card3);
    m_packingKgEdit->setAlignment(Qt::AlignRight);
    connect(m_packingKgEdit, &QLineEdit::textChanged, this, &WeighbridgeKandaDialog::onRecalculate);
    grid3->addWidget(m_packingKgEdit, 0, 3);

    grid3->addWidget(new QLabel("Bag Tare (Kg):", card3), 0, 4);
    m_bagTareEdit = new QLineEdit("0.00", card3);
    m_bagTareEdit->setAlignment(Qt::AlignRight);
    connect(m_bagTareEdit, &QLineEdit::textChanged, this, &WeighbridgeKandaDialog::onRecalculate);
    grid3->addWidget(m_bagTareEdit, 0, 5);

    // Row 1: Gross Wt, Tare Wt, Net Weight
    grid3->addWidget(new QLabel("Gross Wt (Qtl):", card3), 1, 0);
    m_grossWeightEdit = new QLineEdit(card3);
    m_grossWeightEdit->setPlaceholderText("0.00");
    m_grossWeightEdit->setAlignment(Qt::AlignRight);
    connect(m_grossWeightEdit, &QLineEdit::textChanged, this, &WeighbridgeKandaDialog::onRecalculate);
    grid3->addWidget(m_grossWeightEdit, 1, 1);

    grid3->addWidget(new QLabel("Tare Wt (Qtl):", card3), 1, 2);
    m_tareWeightEdit = new QLineEdit(card3);
    m_tareWeightEdit->setPlaceholderText("0.00");
    m_tareWeightEdit->setAlignment(Qt::AlignRight);
    connect(m_tareWeightEdit, &QLineEdit::textChanged, this, &WeighbridgeKandaDialog::onRecalculate);
    grid3->addWidget(m_tareWeightEdit, 1, 3);

    grid3->addWidget(new QLabel("Net Weight:", card3), 1, 4);
    m_netWeightLabel = new QLabel("0.00 Qtl", card3);
    m_netWeightLabel->setAlignment(Qt::AlignCenter);
    m_netWeightLabel->setStyleSheet("font-size: 13px; font-weight: 800; color: #15803D; background-color: #DCFCE7; border: 1px solid #86EFAC; border-radius: 6px; padding: 4px 6px;");
    grid3->addWidget(m_netWeightLabel, 1, 5);

    layout3->addLayout(grid3);
    mainGrid->addWidget(card3, 1, 0);

    // ------------------------------------------------------------------------
    // CARD 4: FREIGHT CHARGES & STATUTORY DETAILS (Bottom Right)
    // ------------------------------------------------------------------------
    auto* card4 = new QFrame(this);
    card4->setStyleSheet(cardStyle);
    auto* layout4 = new QVBoxLayout(card4);
    layout4->setContentsMargins(12, 10, 12, 10);
    layout4->setSpacing(8);

    auto* header4 = new QLabel("4. FREIGHT CHARGES & STATUTORY DETAILS", card4);
    header4->setStyleSheet(sectionHeaderStyle);
    layout4->addWidget(header4);

    auto* grid4 = new QGridLayout();
    grid4->setHorizontalSpacing(10);
    grid4->setVerticalSpacing(6);
    grid4->setColumnStretch(1, 1);
    grid4->setColumnStretch(3, 1);
    grid4->setColumnStretch(5, 1);

    // Row 0: Freight Type, Freight Rate, Total Freight
    grid4->addWidget(new QLabel("Freight Type:", card4), 0, 0);
    m_freightTypeCombo = new QComboBox(card4);
    m_freightTypeCombo->addItems({"Per Qtl", "Per Bag", "Fixed"});
    connect(m_freightTypeCombo, &QComboBox::currentIndexChanged, this, &WeighbridgeKandaDialog::onRecalculate);
    grid4->addWidget(m_freightTypeCombo, 0, 1);

    grid4->addWidget(new QLabel("Rate (₹):", card4), 0, 2);
    m_freightRateEdit = new QLineEdit("0.00", card4);
    m_freightRateEdit->setAlignment(Qt::AlignRight);
    connect(m_freightRateEdit, &QLineEdit::textChanged, this, &WeighbridgeKandaDialog::onRecalculate);
    grid4->addWidget(m_freightRateEdit, 0, 3);

    grid4->addWidget(new QLabel("Total Freight:", card4), 0, 4);
    m_totalFreightLabel = new QLabel("₹ 0.00", card4);
    m_totalFreightLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    m_totalFreightLabel->setStyleSheet("font-size: 13px; font-weight: 800; color: #0F172A;");
    grid4->addWidget(m_totalFreightLabel, 0, 5);

    // Row 1: Advance Paid, Balance Payable, E-Way Bill No
    grid4->addWidget(new QLabel("Advance Paid:", card4), 1, 0);
    m_advanceFreightEdit = new QLineEdit("0.00", card4);
    m_advanceFreightEdit->setAlignment(Qt::AlignRight);
    connect(m_advanceFreightEdit, &QLineEdit::textChanged, this, &WeighbridgeKandaDialog::onRecalculate);
    grid4->addWidget(m_advanceFreightEdit, 1, 1);

    grid4->addWidget(new QLabel("Balance (₹):", card4), 1, 2);
    m_balanceFreightLabel = new QLabel("₹ 0.00", card4);
    m_balanceFreightLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    m_balanceFreightLabel->setStyleSheet("font-size: 13px; font-weight: 800; color: #DC2626;");
    grid4->addWidget(m_balanceFreightLabel, 1, 3);

    grid4->addWidget(new QLabel("E-Way Bill:", card4), 1, 4);
    m_ewayBillEdit = new QLineEdit(card4);
    m_ewayBillEdit->setPlaceholderText("12-digit E-Way");
    grid4->addWidget(m_ewayBillEdit, 1, 5);

    // Row 2: Remarks
    grid4->addWidget(new QLabel("Remarks:", card4), 2, 0);
    m_notesEdit = new QLineEdit(card4);
    m_notesEdit->setPlaceholderText("Remarks / Quality / Out-turn notes");
    grid4->addWidget(m_notesEdit, 2, 1, 1, 5);

    layout4->addLayout(grid4);
    mainGrid->addWidget(card4, 1, 1);

    rootLayout->addLayout(mainGrid, 1);

    // ========================================================================
    // TIER 4: ACTION FOOTER BUTTONS
    // ========================================================================
    auto* bottomLayout = new QHBoxLayout();
    bottomLayout->setSpacing(10);

    auto* cancelBtn = new QPushButton("Cancel (Esc)", this);
    cancelBtn->setCursor(Qt::PointingHandCursor);
    cancelBtn->setStyleSheet(
        "QPushButton { background-color: #F1F5F9; color: #475569; border: 1.5px solid #CBD5E1; border-radius: 6px; padding: 7px 18px; font-weight: 700; font-size: 12px; }"
        "QPushButton:hover { background-color: #E2E8F0; }"
    );
    connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
    bottomLayout->addWidget(cancelBtn);

    bottomLayout->addStretch(1);

    auto* saveBtn = new QPushButton("Save Kanda Slip (Enter)", this);
    saveBtn->setCursor(Qt::PointingHandCursor);
    saveBtn->setStyleSheet(
        "QPushButton { background-color: #16A34A; color: #FFFFFF; border: 1px solid #15803D; border-radius: 6px; padding: 7px 22px; font-weight: 800; font-size: 13px; }"
        "QPushButton:hover { background-color: #15803D; }"
    );
    connect(saveBtn, &QPushButton::clicked, this, &WeighbridgeKandaDialog::onSaveClicked);
    bottomLayout->addWidget(saveBtn);

    rootLayout->addLayout(bottomLayout);

    new QShortcut(QKeySequence(Qt::Key_Escape), this, SLOT(reject()));
}

void WeighbridgeKandaDialog::onResetForm() {
    if (m_controller) {
        m_slipNoEdit->setText(m_controller->getNextSlipNo());
    } else {
        m_slipNoEdit->setText("KND-" + QString::number(QDateTime::currentMSecsSinceEpoch() % 100000));
    }
    m_dateEdit->setDate(QDate::currentDate());
    m_timeEdit->setTime(QTime::currentTime());
    m_invoiceNoEdit->clear();
    m_partyNameEdit->clear();
    m_itemNameEdit->setText("Rice Basmati");
    m_vehicleNoEdit->clear();
    m_driverNameEdit->clear();
    m_driverPhoneEdit->clear();
    m_transporterNameEdit->clear();
    m_transporterGstinEdit->clear();
    m_destinationEdit->clear();
    m_bagCountEdit->setText("0");
    m_packingKgEdit->setText("50.0");
    m_grossWeightEdit->clear();
    m_tareWeightEdit->clear();
    m_bagTareEdit->setText("0.00");
    m_freightTypeCombo->setCurrentIndex(0);
    m_freightRateEdit->setText("0.00");
    m_advanceFreightEdit->setText("0.00");
    m_ewayBillEdit->clear();
    m_notesEdit->clear();
    onRecalculate();
}

void WeighbridgeKandaDialog::loadDispatch(int id) {
    if (!m_controller) return;
    auto map = m_controller->getDispatch(id);
    if (map.isEmpty()) return;

    m_editDispatchId = id;
    m_slipNoEdit->setText(map.value("slipNo").toString());
    m_dateEdit->setDate(QDate::fromString(map.value("dispatchDate").toString(), "yyyy-MM-dd"));
    m_timeEdit->setTime(QTime::fromString(map.value("dispatchTime").toString(), "HH:mm"));
    m_invoiceNoEdit->setText(map.value("invoiceNo").toString());
    m_partyNameEdit->setText(map.value("partyName").toString());
    m_itemNameEdit->setText(map.value("itemName").toString());
    m_vehicleNoEdit->setText(map.value("vehicleNo").toString());
    m_driverNameEdit->setText(map.value("driverName").toString());
    m_driverPhoneEdit->setText(map.value("driverPhone").toString());
    m_transporterNameEdit->setText(map.value("transporterName").toString());
    m_transporterGstinEdit->setText(map.value("transporterGstin").toString());
    m_destinationEdit->setText(map.value("destination").toString());
    m_bagCountEdit->setText(QString::number(map.value("bagCount").toInt()));
    m_packingKgEdit->setText(QString::number(map.value("packingKg").toDouble(), 'f', 1));
    m_grossWeightEdit->setText(QString::number(map.value("grossWeightQtl").toDouble(), 'f', 2));
    m_tareWeightEdit->setText(QString::number(map.value("tareWeightQtl").toDouble(), 'f', 2));
    m_bagTareEdit->setText(QString::number(map.value("bagTareKg").toDouble(), 'f', 2));

    QString fType = map.value("freightCalcType").toString();
    if (fType == "Per Bag") m_freightTypeCombo->setCurrentIndex(1);
    else if (fType == "Fixed") m_freightTypeCombo->setCurrentIndex(2);
    else m_freightTypeCombo->setCurrentIndex(0);

    m_freightRateEdit->setText(QString::number(map.value("freightRate").toDouble(), 'f', 2));
    m_advanceFreightEdit->setText(QString::number(map.value("advanceFreight").toDouble(), 'f', 2));
    m_ewayBillEdit->setText(map.value("ewayBillNo").toString());
    m_notesEdit->setText(map.value("notes").toString());

    onRecalculate();
}

void WeighbridgeKandaDialog::onRecalculate() {
    double gross = m_grossWeightEdit->text().toDouble();
    double tare = m_tareWeightEdit->text().toDouble();
    int bags = m_bagCountEdit->text().toInt();
    double packKg = m_packingKgEdit->text().toDouble();
    double bagTare = m_bagTareEdit->text().toDouble();

    double netQtl = 0.0;
    if (gross > 0 || tare > 0) {
        double rawNet = gross - tare;
        double totalBagTareQtl = (bags * bagTare) / 100.0;
        netQtl = std::max(0.0, rawNet - totalBagTareQtl);
    } else if (bags > 0 && packKg > 0) {
        netQtl = (bags * packKg) / 100.0;
    }
    m_netWeightLabel->setText(QString::number(netQtl, 'f', 2) + " Qtl");

    QString fType = m_freightTypeCombo->currentText();
    double rate = m_freightRateEdit->text().toDouble();
    double totalFreight = 0.0;
    if (fType == "Per Qtl") totalFreight = netQtl * rate;
    else if (fType == "Per Bag") totalFreight = bags * rate;
    else totalFreight = rate;

    double advance = m_advanceFreightEdit->text().toDouble();
    double balance = std::max(0.0, totalFreight - advance);

    m_totalFreightLabel->setText("₹ " + QString::number(totalFreight, 'f', 2));
    m_balanceFreightLabel->setText("₹ " + QString::number(balance, 'f', 2));
}

void WeighbridgeKandaDialog::onSaveClicked() {
    if (m_vehicleNoEdit->text().trimmed().isEmpty()) {
        CustomMessageBox::showWarning(this, "Validation Error", "Please specify Vehicle No.");
        m_vehicleNoEdit->setFocus();
        return;
    }

    if (!m_controller) {
        accept();
        return;
    }

    QVariantMap data;
    data["id"] = m_editDispatchId;
    data["slipNo"] = m_slipNoEdit->text().trimmed();
    data["dispatchDate"] = m_dateEdit->date().toString("yyyy-MM-dd");
    data["dispatchTime"] = m_timeEdit->time().toString("HH:mm");
    data["invoiceNo"] = m_invoiceNoEdit->text().trimmed();
    data["partyName"] = m_partyNameEdit->text().trimmed();
    data["itemName"] = m_itemNameEdit->text().trimmed();
    data["vehicleNo"] = m_vehicleNoEdit->text().trimmed();
    data["driverName"] = m_driverNameEdit->text().trimmed();
    data["driverPhone"] = m_driverPhoneEdit->text().trimmed();
    data["transporterName"] = m_transporterNameEdit->text().trimmed();
    data["transporterGstin"] = m_transporterGstinEdit->text().trimmed();
    data["destination"] = m_destinationEdit->text().trimmed();
    data["bagCount"] = m_bagCountEdit->text().toInt();
    data["packingKg"] = m_packingKgEdit->text().toDouble();
    data["grossWeightQtl"] = m_grossWeightEdit->text().toDouble();
    data["tareWeightQtl"] = m_tareWeightEdit->text().toDouble();
    data["bagTareKg"] = m_bagTareEdit->text().toDouble();
    data["freightCalcType"] = m_freightTypeCombo->currentText();
    data["freightRate"] = m_freightRateEdit->text().toDouble();
    data["advanceFreight"] = m_advanceFreightEdit->text().toDouble();
    data["ewayBillNo"] = m_ewayBillEdit->text().trimmed();
    data["notes"] = m_notesEdit->text().trimmed();

    QVariantMap res = m_controller->saveDispatch(data);
    bool ok = res.value("success", false).toBool();
    if (ok) {
        emit slipSaved();
        accept();
    } else {
        QString errMsg = res.value("error", "Failed to save Weighbridge Slip. Check database connection.").toString();
        CustomMessageBox::showCritical(this, "Save Error", errMsg);
    }
}

} // namespace MahadevERP
