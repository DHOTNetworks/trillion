#include "transport_dispatch_controller.h"
#include "../database_manager.h"
#include "../services/financial_math_service.h"
#include "../services/accounting_date_service.h"
#include <QDebug>
#include <QDate>
#include <QTime>
#include <cmath>

// ---------------------------------------------------------------------------
// TransportDispatchModel Implementation
// ---------------------------------------------------------------------------

TransportDispatchModel::TransportDispatchModel(QObject *parent)
    : QAbstractListModel(parent) {
}

int TransportDispatchModel::rowCount(const QModelIndex &parent) const {
    if (parent.isValid()) return 0;
    return m_visibleIndices.size();
}

QVariant TransportDispatchModel::data(const QModelIndex &index, int role) const {
    if (!index.isValid() || index.row() < 0 || index.row() >= m_visibleIndices.size()) {
        return QVariant();
    }

    int rawIdx = m_visibleIndices.at(index.row());
    const auto &r = m_allRecords.at(rawIdx);
    auto &math = FinancialMathService::instance();

    switch (role) {
    case IdRole: return r.id;
    case SlipNoRole: return r.slipNo;
    case DispatchDateRole: return r.dispatchDate;
    case DispatchTimeRole: return r.dispatchTime;
    case InvoiceNoRole: return r.invoiceNo;
    case VoucherTypeRole: return r.voucherType;
    case PartyNameRole: return r.partyName;
    case ItemNameRole: return r.itemName;
    case GradeRole: return r.grade;
    case VehicleNoRole: return r.vehicleNo;
    case DriverNameRole: return r.driverName;
    case DriverPhoneRole: return r.driverPhone;
    case TransporterNameRole: return r.transporterName;
    case TransporterGstinRole: return r.transporterGstin;
    case GrNoRole: return r.grNo;
    case GrDateRole: return r.grDate;
    case DestinationRole: return r.destination;
    case DistanceKmRole: return r.distanceKm;
    case BagCountRole: return r.bagCount;
    case PackingKgRole: return r.packingKg;
    case GrossWeightRole: return r.grossWeightQtl;
    case TareWeightRole: return r.tareWeightQtl;
    case BagTareKgRole: return r.bagTareKg;
    case NetWeightRole: return r.netWeightQtl;
    case FreightCalcTypeRole: return r.freightCalcType;
    case FreightRateRole: return r.freightRate;
    case TotalFreightRole: return r.totalFreight;
    case AdvanceFreightRole: return r.advanceFreight;
    case BalanceFreightRole: return r.balanceFreight;
    case FreightPaymentStatusRole: return r.freightPaymentStatus;
    case EwayBillNoRole: return r.ewayBillNo;
    case IrnNoRole: return r.irnNo;
    case NotesRole: return r.notes;
    case TotalFreightFmtRole: return math.formatInr(r.totalFreight);
    case AdvanceFreightFmtRole: return math.formatInr(r.advanceFreight);
    case BalanceFreightFmtRole: return math.formatInr(r.balanceFreight);
    default: return QVariant();
    }
}

QHash<int, QByteArray> TransportDispatchModel::roleNames() const {
    QHash<int, QByteArray> roles;
    roles[IdRole] = "dispatchId";
    roles[SlipNoRole] = "slipNo";
    roles[DispatchDateRole] = "dispatchDate";
    roles[DispatchTimeRole] = "dispatchTime";
    roles[InvoiceNoRole] = "invoiceNo";
    roles[VoucherTypeRole] = "voucherType";
    roles[PartyNameRole] = "partyName";
    roles[ItemNameRole] = "itemName";
    roles[GradeRole] = "grade";
    roles[VehicleNoRole] = "vehicleNo";
    roles[DriverNameRole] = "driverName";
    roles[DriverPhoneRole] = "driverPhone";
    roles[TransporterNameRole] = "transporterName";
    roles[TransporterGstinRole] = "transporterGstin";
    roles[GrNoRole] = "grNo";
    roles[GrDateRole] = "grDate";
    roles[DestinationRole] = "destination";
    roles[DistanceKmRole] = "distanceKm";
    roles[BagCountRole] = "bagCount";
    roles[PackingKgRole] = "packingKg";
    roles[GrossWeightRole] = "grossWeightQtl";
    roles[TareWeightRole] = "tareWeightQtl";
    roles[BagTareKgRole] = "bagTareKg";
    roles[NetWeightRole] = "netWeightQtl";
    roles[FreightCalcTypeRole] = "freightCalcType";
    roles[FreightRateRole] = "freightRate";
    roles[TotalFreightRole] = "totalFreight";
    roles[AdvanceFreightRole] = "advanceFreight";
    roles[BalanceFreightRole] = "balanceFreight";
    roles[FreightPaymentStatusRole] = "freightPaymentStatus";
    roles[EwayBillNoRole] = "ewayBillNo";
    roles[IrnNoRole] = "irnNo";
    roles[NotesRole] = "notes";
    roles[TotalFreightFmtRole] = "totalFreightFmt";
    roles[AdvanceFreightFmtRole] = "advanceFreightFmt";
    roles[BalanceFreightFmtRole] = "balanceFreightFmt";
    return roles;
}

void TransportDispatchModel::setAllRecords(const QVector<TransportDispatchRecord> &records) {
    m_allRecords = records;
    applyFilter();
}

void TransportDispatchModel::applyFilter() {
    beginResetModel();
    m_visibleIndices.clear();
    m_filteredNetWeightQtl = 0.0;
    m_filteredFreightAmount = 0.0;
    m_filteredBalancePayable = 0.0;

    QString q = m_searchQuery.trimmed().toLower();
    QString ft = m_filterType.toUpper();
    QString today = QDate::currentDate().toString("yyyy-MM-dd");

    for (int i = 0; i < m_allRecords.size(); ++i) {
        const auto &r = m_allRecords[i];

        if (ft == "PENDING" && r.balanceFreight <= 0.01) continue;
        if (ft == "SETTLED" && r.balanceFreight > 0.01) continue;
        if (ft == "TODAY" && r.dispatchDate != today) continue;

        if (!q.isEmpty()) {
            bool matches = r.slipNo.toLower().contains(q) ||
                           r.vehicleNo.toLower().contains(q) ||
                           r.driverName.toLower().contains(q) ||
                           r.transporterName.toLower().contains(q) ||
                           r.partyName.toLower().contains(q) ||
                           r.invoiceNo.toLower().contains(q) ||
                           r.grNo.toLower().contains(q) ||
                           r.destination.toLower().contains(q) ||
                           r.itemName.toLower().contains(q);
            if (!matches) continue;
        }

        m_visibleIndices.append(i);
        m_filteredNetWeightQtl += r.netWeightQtl;
        m_filteredFreightAmount += r.totalFreight;
        m_filteredBalancePayable += r.balanceFreight;
    }
    endResetModel();
    emit countChanged();
    emit filterSummaryChanged();
}

QString TransportDispatchModel::filteredNetWeightFmt() const {
    return QString("%1 Qtl").arg(QString::number(m_filteredNetWeightQtl, 'f', 2));
}

QString TransportDispatchModel::filteredFreightAmountFmt() const {
    return FinancialMathService::instance().formatInr(m_filteredFreightAmount);
}

QString TransportDispatchModel::filteredBalancePayableFmt() const {
    return FinancialMathService::instance().formatInr(m_filteredBalancePayable);
}

void TransportDispatchModel::setFilter(const QString &filterType, const QString &searchQuery) {
    if (m_filterType != filterType || m_searchQuery != searchQuery) {
        m_filterType = filterType;
        m_searchQuery = searchQuery;
        applyFilter();
    }
}

QVariantMap TransportDispatchModel::get(int visibleIndex) const {
    if (visibleIndex < 0 || visibleIndex >= m_visibleIndices.size()) return QVariantMap();
    int rawIdx = m_visibleIndices.at(visibleIndex);
    const auto &r = m_allRecords.at(rawIdx);
    auto &math = FinancialMathService::instance();

    QVariantMap map;
    map["id"] = r.id;
    map["slipNo"] = r.slipNo;
    map["dispatchDate"] = r.dispatchDate;
    map["dispatchTime"] = r.dispatchTime;
    map["invoiceNo"] = r.invoiceNo;
    map["voucherType"] = r.voucherType;
    map["partyId"] = r.partyId;
    map["partyName"] = r.partyName;
    map["itemId"] = r.itemId;
    map["itemName"] = r.itemName;
    map["grade"] = r.grade;
    map["vehicleNo"] = r.vehicleNo;
    map["driverName"] = r.driverName;
    map["driverPhone"] = r.driverPhone;
    map["transporterName"] = r.transporterName;
    map["transporterGstin"] = r.transporterGstin;
    map["grNo"] = r.grNo;
    map["grDate"] = r.grDate;
    map["destination"] = r.destination;
    map["distanceKm"] = r.distanceKm;
    map["bagCount"] = r.bagCount;
    map["packingKg"] = r.packingKg;
    map["grossWeightQtl"] = r.grossWeightQtl;
    map["tareWeightQtl"] = r.tareWeightQtl;
    map["bagTareKg"] = r.bagTareKg;
    map["netWeightQtl"] = r.netWeightQtl;
    map["freightCalcType"] = r.freightCalcType;
    map["freightRate"] = r.freightRate;
    map["totalFreight"] = r.totalFreight;
    map["advanceFreight"] = r.advanceFreight;
    map["balanceFreight"] = r.balanceFreight;
    map["freightPaymentStatus"] = r.freightPaymentStatus;
    map["ewayBillNo"] = r.ewayBillNo;
    map["irnNo"] = r.irnNo;
    map["notes"] = r.notes;
    map["totalFreightFmt"] = math.formatInr(r.totalFreight);
    map["advanceFreightFmt"] = math.formatInr(r.advanceFreight);
    map["balanceFreightFmt"] = math.formatInr(r.balanceFreight);
    return map;
}

// ---------------------------------------------------------------------------
// TransportDispatchController Implementation
// ---------------------------------------------------------------------------

TransportDispatchController::TransportDispatchController(QObject *parent)
    : QObject(parent) {
    reload();
}

QString TransportDispatchController::totalNetWeightFmt() const {
    return QString("%1 Qtl").arg(QString::number(m_totalNetWeightQtl, 'f', 2));
}

QString TransportDispatchController::totalFreightAmountFmt() const {
    return FinancialMathService::instance().formatInr(m_totalFreightAmount);
}

QString TransportDispatchController::totalAdvancePaidFmt() const {
    return FinancialMathService::instance().formatInr(m_totalAdvancePaid);
}

QString TransportDispatchController::totalBalancePayableFmt() const {
    return FinancialMathService::instance().formatInr(m_totalBalancePayable);
}

void TransportDispatchController::reload() {
    // Sync any sales/purchase dispatches into transport_dispatches
    DatabaseManager::instance().executeNonQuery(
        "INSERT INTO transport_dispatches ("
        "  fy_id, financial_year, dispatch_date, dispatch_time, slip_no, voucher_no, invoice_no, "
        "  voucher_type, party_name, item_name, grade, vehicle_no, driver_name, transporter_name, "
        "  gr_no, destination, distance_km, bag_count, net_weight_qtl, total_freight, "
        "  advance_freight, balance_freight, freight_payment_status, eway_bill_no, irn_no, notes"
        ") "
        "SELECT "
        "  si.fy_id, si.financial_year, si.invoice_date, si.bill_time, 'DISP-' || si.invoice_no, si.voucher_no, si.invoice_no, "
        "  'Sale', si.customer_name, si.item_name, si.grade, si.vehicle_no, si.driver_name, si.transport, "
        "  si.gr_no, si.shipping_address, si.distance, si.bag_count, "
        "  CASE WHEN CAST(si.kanda_weight AS REAL) > 0 THEN CAST(si.kanda_weight AS REAL) ELSE si.weight_qtl END, "
        "  si.freight_charges, 0.0, si.freight_charges, "
        "  CASE WHEN si.freight_charges > 0 THEN 'Unpaid' ELSE 'Settled' END, "
        "  si.eway_bill_no, si.irn_no, si.narration "
        "FROM sales_invoices si "
        "WHERE (si.vehicle_no IS NOT NULL AND si.vehicle_no != '') "
        "  AND NOT EXISTS ("
        "    SELECT 1 FROM transport_dispatches td WHERE td.invoice_no = si.invoice_no AND td.voucher_type = 'Sale'"
        "  );"
    );

    DatabaseManager::instance().executeNonQuery(
        "INSERT INTO transport_dispatches ("
        "  fy_id, financial_year, dispatch_date, slip_no, voucher_no, invoice_no, "
        "  voucher_type, party_name, item_name, vehicle_no, driver_name, transporter_name, "
        "  gr_no, bag_count, net_weight_qtl, total_freight, "
        "  advance_freight, balance_freight, freight_payment_status, eway_bill_no, notes"
        ") "
        "SELECT "
        "  pi.fy_id, pi.financial_year, pi.invoice_date, 'INW-' || pi.invoice_no, pi.voucher_no, pi.invoice_no, "
        "  'Purchase', pi.supplier_name, pi.item_name, pi.vehicle_no, pi.driver_name, pi.transport, "
        "  pi.gr_no, pi.bag_count, "
        "  CASE WHEN CAST(pi.kanda_weight AS REAL) > 0 THEN CAST(pi.kanda_weight AS REAL) ELSE pi.weight_qtl END, "
        "  pi.freight_charges, 0.0, pi.freight_charges, "
        "  CASE WHEN pi.freight_charges > 0 THEN 'Unpaid' ELSE 'Settled' END, "
        "  pi.eway_bill_no, pi.narration "
        "FROM purchase_invoices pi "
        "WHERE (pi.vehicle_no IS NOT NULL AND pi.vehicle_no != '') "
        "  AND NOT EXISTS ("
        "    SELECT 1 FROM transport_dispatches td WHERE td.invoice_no = pi.invoice_no AND td.voucher_type = 'Purchase'"
        "  );"
    );

    QVector<TransportDispatchRecord> records;
    QVariantList rows = DatabaseManager::instance().executeQuery(
        "SELECT * FROM transport_dispatches ORDER BY id DESC;"
    );

    for (const auto &item : rows) {
        auto map = item.toMap();
        TransportDispatchRecord r;
        r.id = map.value("id").toInt();
        r.fyId = map.value("fy_id").toInt();
        r.financialYear = map.value("financial_year").toString();
        r.dispatchDate = map.value("dispatch_date").toString();
        r.dispatchTime = map.value("dispatch_time").toString();
        r.slipNo = map.value("slip_no").toString();
        r.voucherNo = map.value("voucher_no").toString();
        r.invoiceNo = map.value("invoice_no").toString();
        r.voucherType = map.value("voucher_type").toString();
        r.partyId = map.value("party_id").toInt();
        r.partyName = map.value("party_name").toString();
        r.itemId = map.value("item_id").toInt();
        r.itemName = map.value("item_name").toString();
        r.grade = map.value("grade").toString();
        r.vehicleNo = map.value("vehicle_no").toString();
        r.driverName = map.value("driver_name").toString();
        r.driverPhone = map.value("driver_phone").toString();
        r.transporterName = map.value("transporter_name").toString();
        r.transporterGstin = map.value("transporter_gstin").toString();
        r.grNo = map.value("gr_no").toString();
        r.grDate = map.value("gr_date").toString();
        r.destination = map.value("destination").toString();
        r.distanceKm = map.value("distance_km").toInt();
        r.bagCount = map.value("bag_count").toInt();
        r.packingKg = map.value("packing_kg").toDouble();
        r.grossWeightQtl = map.value("gross_weight_qtl").toDouble();
        r.tareWeightQtl = map.value("tare_weight_qtl").toDouble();
        r.bagTareKg = map.value("bag_tare_kg").toDouble();
        r.netWeightQtl = map.value("net_weight_qtl").toDouble();
        r.freightCalcType = map.value("freight_calc_type").toString();
        r.freightRate = map.value("freight_rate").toDouble();
        r.totalFreight = map.value("total_freight").toDouble();
        r.advanceFreight = map.value("advance_freight").toDouble();
        r.balanceFreight = map.value("balance_freight").toDouble();
        r.freightPaymentStatus = map.value("freight_payment_status").toString();
        r.ewayBillNo = map.value("eway_bill_no").toString();
        r.irnNo = map.value("irn_no").toString();
        r.notes = map.value("notes").toString();
        records.append(r);
    }

    m_model.setAllRecords(records);
    recalculateSummary();
}

void TransportDispatchController::recalculateSummary() {
    m_totalCount = m_model.allRecords().size();
    m_totalNetWeightQtl = 0.0;
    m_totalFreightAmount = 0.0;
    m_totalAdvancePaid = 0.0;
    m_totalBalancePayable = 0.0;

    for (const auto &r : m_model.allRecords()) {
        m_totalNetWeightQtl += r.netWeightQtl;
        m_totalFreightAmount += r.totalFreight;
        m_totalAdvancePaid += r.advanceFreight;
        m_totalBalancePayable += r.balanceFreight;
    }

    emit summaryChanged();
}

double TransportDispatchController::calculateNetWeight(double grossQtl, double tareQtl, int bagCount, double bagTareKg) {
    // Net Weight (Qtl) = Gross (Qtl) - Tare (Qtl) - (Bags * BagTareKg / 100.0)
    double bagDeductionQtl = (bagCount * bagTareKg) / 100.0;
    double net = grossQtl - tareQtl - bagDeductionQtl;
    if (net < 0.0) net = 0.0;
    return std::round(net * 100.0) / 100.0;
}

QVariantMap TransportDispatchController::calculateFreight(const QString &calcType, double rate, double netWeightQtl, int bagCount, double advancePaid) {
    double total = 0.0;
    if (calcType == "Per Qtl") {
        total = rate * netWeightQtl;
    } else if (calcType == "Per Bag") {
        total = rate * bagCount;
    } else { // "Fixed" / Lumpsum
        total = rate;
    }

    total = std::round(total * 100.0) / 100.0;
    double balance = total - advancePaid;
    if (balance < 0.0) balance = 0.0;
    balance = std::round(balance * 100.0) / 100.0;

    QString status = "Unpaid";
    if (advancePaid >= total && total > 0.0) {
        status = "Settled";
    } else if (advancePaid > 0.0) {
        status = "Partially Paid";
    }

    auto &math = FinancialMathService::instance();
    QVariantMap res;
    res["totalFreight"] = total;
    res["advanceFreight"] = advancePaid;
    res["balanceFreight"] = balance;
    res["status"] = status;
    res["paymentStatus"] = status;
    res["totalFreightFmt"] = math.formatInr(total);
    res["advanceFreightFmt"] = math.formatInr(advancePaid);
    res["balanceFreightFmt"] = math.formatInr(balance);
    return res;
}

QString TransportDispatchController::getNextSlipNo() {
    QVariant maxVal = DatabaseManager::instance().executeScalar(
        "SELECT MAX(id) FROM transport_dispatches;"
    );
    int nextId = maxVal.isValid() ? (maxVal.toInt() + 1) : 1;
    return QString("KP-%1").arg(nextId, 4, 10, QChar('0'));
}

QVariantMap TransportDispatchController::saveDispatch(const QVariantMap &data) {
    QVariantMap response;
    int id = data.value("id", 0).toInt();

    QString slipNo = data.value("slipNo").toString().trimmed();
    if (slipNo.isEmpty()) {
        slipNo = getNextSlipNo();
    }

    QString dispatchDate = data.value("dispatchDate").toString().trimmed();
    if (dispatchDate.isEmpty()) {
        dispatchDate = QDate::currentDate().toString("yyyy-MM-dd");
    }

    QString dispatchTime = data.value("dispatchTime").toString().trimmed();
    if (dispatchTime.isEmpty()) {
        dispatchTime = QTime::currentTime().toString("hh:mm AP");
    }

    QString vehicleNo = data.value("vehicleNo").toString().trimmed().toUpper();
    QString partyName = data.value("partyName").toString().trimmed();
    QString invoiceNo = data.value("invoiceNo").toString().trimmed();
    QString itemName = data.value("itemName").toString().trimmed();
    QString grade = data.value("grade").toString().trimmed();
    QString driverName = data.value("driverName").toString().trimmed();
    QString driverPhone = data.value("driverPhone").toString().trimmed();
    QString transporterName = data.value("transporterName").toString().trimmed();
    QString transporterGstin = data.value("transporterGstin").toString().trimmed();
    QString grNo = data.value("grNo").toString().trimmed();
    QString grDate = data.value("grDate").toString().trimmed();
    QString destination = data.value("destination").toString().trimmed();
    int distanceKm = data.value("distanceKm", 0).toInt();

    int bagCount = data.value("bagCount", 0).toInt();
    double packingKg = data.value("packingKg", 50.0).toDouble();
    double grossWeightQtl = data.value("grossWeightQtl", 0.0).toDouble();
    double tareWeightQtl = data.value("tareWeightQtl", 0.0).toDouble();
    double bagTareKg = data.value("bagTareKg", 0.0).toDouble();
    double netWeightQtl = data.value("netWeightQtl", 0.0).toDouble();

    if (netWeightQtl <= 0.0) {
        netWeightQtl = calculateNetWeight(grossWeightQtl, tareWeightQtl, bagCount, bagTareKg);
        if (netWeightQtl <= 0.0 && bagCount > 0 && packingKg > 0.0) {
            netWeightQtl = std::round((bagCount * packingKg / 100.0) * 100.0) / 100.0;
        }
    }

    QString freightCalcType = data.value("freightCalcType", "Per Qtl").toString();
    double freightRate = data.value("freightRate", 0.0).toDouble();
    double advanceFreight = data.value("advanceFreight", 0.0).toDouble();

    double totalFreight = data.value("totalFreight", 0.0).toDouble();
    if (freightRate > 0.0) {
        QVariantMap fCalc = calculateFreight(freightCalcType, freightRate, netWeightQtl, bagCount, advanceFreight);
        totalFreight = fCalc.value("totalFreight").toDouble();
    }
    double balanceFreight = std::max(0.0, totalFreight - advanceFreight);
    QString freightPaymentStatus = data.value("freightPaymentStatus").toString();
    if (freightPaymentStatus.isEmpty()) {
        freightPaymentStatus = (totalFreight > 0.0 && balanceFreight <= 0.0) ? "Settled" : (advanceFreight > 0.0 ? "Partially Paid" : (totalFreight > 0.0 ? "Unpaid" : "Settled"));
    }

    QString ewayBillNo = data.value("ewayBillNo").toString().trimmed();
    QString irnNo = data.value("irnNo").toString().trimmed();
    QString notes = data.value("notes").toString().trimmed();

    if (vehicleNo.isEmpty()) {
        response["success"] = false;
        response["message"] = "Vehicle Registration Number is required.";
        return response;
    }

    QVariant partyIdVal = QVariant();
    if (!partyName.isEmpty()) {
        QVariant pid = DatabaseManager::instance().executeScalar(
            "SELECT id FROM parties WHERE party_name = ? COLLATE NOCASE;", {partyName}
        );
        if (pid.isValid() && pid.toInt() > 0) {
            partyIdVal = pid.toInt();
        }
    }

    QVariant itemIdVal = QVariant();
    if (!itemName.isEmpty()) {
        QVariant iid = DatabaseManager::instance().executeScalar(
            "SELECT id FROM stock_items WHERE item_name = ? COLLATE NOCASE;", {itemName}
        );
        if (iid.isValid() && iid.toInt() > 0) {
            itemIdVal = iid.toInt();
        }
    }

    if (id <= 0) {
        // Insert new dispatch
        bool ok = DatabaseManager::instance().executeNonQuery(
            "INSERT INTO transport_dispatches ("
            "  dispatch_date, dispatch_time, slip_no, voucher_no, invoice_no, voucher_type, "
            "  party_id, party_name, item_id, item_name, grade, vehicle_no, driver_name, "
            "  driver_phone, transporter_name, transporter_gstin, gr_no, gr_date, destination, "
            "  distance_km, bag_count, packing_kg, gross_weight_qtl, tare_weight_qtl, bag_tare_kg, "
            "  net_weight_qtl, freight_calc_type, freight_rate, total_freight, advance_freight, "
            "  balance_freight, freight_payment_status, eway_bill_no, irn_no, notes"
            ") VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);",
            {
                dispatchDate, dispatchTime, slipNo, "", invoiceNo, "Sale",
                partyIdVal, partyName, itemIdVal, itemName, grade, vehicleNo, driverName,
                driverPhone, transporterName, transporterGstin, grNo, grDate, destination,
                distanceKm, bagCount, packingKg, grossWeightQtl, tareWeightQtl, bagTareKg,
                netWeightQtl, freightCalcType, freightRate, totalFreight, advanceFreight,
                balanceFreight, freightPaymentStatus, ewayBillNo, irnNo, notes
            }
        );

        if (!ok) {
            response["success"] = false;
            response["message"] = "Failed to insert dispatch record into database.";
            return response;
        }

        QVariant lastId = DatabaseManager::instance().executeScalar("SELECT last_insert_rowid();");
        id = lastId.toInt();
    } else {
        // Update existing dispatch
        bool ok = DatabaseManager::instance().executeNonQuery(
            "UPDATE transport_dispatches SET "
            "  dispatch_date = ?, dispatch_time = ?, slip_no = ?, invoice_no = ?, "
            "  party_name = ?, item_name = ?, grade = ?, vehicle_no = ?, driver_name = ?, "
            "  driver_phone = ?, transporter_name = ?, transporter_gstin = ?, gr_no = ?, "
            "  gr_date = ?, destination = ?, distance_km = ?, bag_count = ?, packing_kg = ?, "
            "  gross_weight_qtl = ?, tare_weight_qtl = ?, bag_tare_kg = ?, net_weight_qtl = ?, "
            "  freight_calc_type = ?, freight_rate = ?, total_freight = ?, advance_freight = ?, "
            "  balance_freight = ?, freight_payment_status = ?, eway_bill_no = ?, irn_no = ?, notes = ? "
            "WHERE id = ?;",
            {
                dispatchDate, dispatchTime, slipNo, invoiceNo,
                partyName, itemName, grade, vehicleNo, driverName,
                driverPhone, transporterName, transporterGstin, grNo,
                grDate, destination, distanceKm, bagCount, packingKg,
                grossWeightQtl, tareWeightQtl, bagTareKg, netWeightQtl,
                freightCalcType, freightRate, totalFreight, advanceFreight,
                balanceFreight, freightPaymentStatus, ewayBillNo, irnNo, notes,
                id
            }
        );

        if (!ok) {
            response["success"] = false;
            response["message"] = "Failed to update dispatch record in database.";
            return response;
        }
    }

    // Sync logistics details to linked sales invoice if present
    if (!invoiceNo.isEmpty()) {
        DatabaseManager::instance().executeNonQuery(
            "UPDATE sales_invoices SET "
            "  vehicle_no = ?, gr_no = ?, driver_name = ?, driver = ?, "
            "  kanda_weight = ?, transport = ?, eway_bill_no = ?, irn_no = ? "
            "WHERE invoice_no = ?;",
            {
                vehicleNo, grNo, driverName, driverPhone,
                QString("%1 Qtl").arg(QString::number(netWeightQtl, 'f', 2)),
                transporterName, ewayBillNo, irnNo,
                invoiceNo
            }
        );
    }

    reload();
    emit dispatchSaved(id, slipNo);

    response["success"] = true;
    response["message"] = QString("Weighbridge Dispatch Slip %1 saved successfully.").arg(slipNo);
    response["id"] = id;
    response["slipNo"] = slipNo;
    return response;
}

bool TransportDispatchController::deleteDispatch(int id) {
    bool ok = DatabaseManager::instance().executeNonQuery(
        "DELETE FROM transport_dispatches WHERE id = ?;",
        {id}
    );
    if (ok) {
        reload();
        emit dispatchDeleted(id);
    }
    return ok;
}

QVariantMap TransportDispatchController::getDispatch(int id) {
    QVariantList rows = DatabaseManager::instance().executeQuery(
        "SELECT * FROM transport_dispatches WHERE id = ? LIMIT 1;",
        {id}
    );
    if (rows.isEmpty()) return QVariantMap();
    auto raw = rows.first().toMap();
    auto &math = FinancialMathService::instance();
    QVariantMap map = raw;
    map["id"] = raw.value("id").toInt();
    map["slipNo"] = raw.value("slip_no").toString();
    map["dispatchDate"] = raw.value("dispatch_date").toString();
    map["dispatchTime"] = raw.value("dispatch_time").toString();
    map["invoiceNo"] = raw.value("invoice_no").toString();
    map["partyName"] = raw.value("party_name").toString();
    map["itemName"] = raw.value("item_name").toString();
    map["grade"] = raw.value("grade").toString();
    map["vehicleNo"] = raw.value("vehicle_no").toString();
    map["driverName"] = raw.value("driver_name").toString();
    map["driverPhone"] = raw.value("driver_phone").toString();
    map["transporterName"] = raw.value("transporter_name").toString();
    map["transporterGstin"] = raw.value("transporter_gstin").toString();
    map["grNo"] = raw.value("gr_no").toString();
    map["grDate"] = raw.value("gr_date").toString();
    map["destination"] = raw.value("destination").toString();
    map["distanceKm"] = raw.value("distance_km").toInt();
    map["bagCount"] = raw.value("bag_count").toInt();
    map["packingKg"] = raw.value("packing_kg").toDouble();
    map["grossWeightQtl"] = raw.value("gross_weight_qtl").toDouble();
    map["tareWeightQtl"] = raw.value("tare_weight_qtl").toDouble();
    map["bagTareKg"] = raw.value("bag_tare_kg").toDouble();
    map["netWeightQtl"] = raw.value("net_weight_qtl").toDouble();
    map["freightCalcType"] = raw.value("freight_calc_type").toString();
    map["freightRate"] = raw.value("freight_rate").toDouble();
    map["totalFreight"] = raw.value("total_freight").toDouble();
    map["advanceFreight"] = raw.value("advance_freight").toDouble();
    map["balanceFreight"] = raw.value("balance_freight").toDouble();
    map["freightPaymentStatus"] = raw.value("freight_payment_status").toString();
    map["ewayBillNo"] = raw.value("eway_bill_no").toString();
    map["irnNo"] = raw.value("irn_no").toString();
    map["notes"] = raw.value("notes").toString();
    map["totalFreightFmt"] = math.formatInr(raw.value("total_freight").toDouble());
    map["advanceFreightFmt"] = math.formatInr(raw.value("advance_freight").toDouble());
    map["balanceFreightFmt"] = math.formatInr(raw.value("balance_freight").toDouble());
    return map;
}

QVariantList TransportDispatchController::searchInvoices(const QString &query) {
    QVariantList results;
    QString q = "%" + query.trimmed() + "%";
    QVariantList rows = DatabaseManager::instance().executeQuery(
        "SELECT id, invoice_no, invoice_date, customer_name, item_name, bag_count, weight_qtl, vehicle_no, gr_no, eway_bill_no "
        "FROM sales_invoices "
        "WHERE invoice_no LIKE ? OR customer_name LIKE ? OR vehicle_no LIKE ? "
        "ORDER BY id DESC LIMIT 15;",
        {q, q, q}
    );
    return rows;
}
