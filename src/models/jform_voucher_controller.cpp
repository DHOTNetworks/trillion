#include "jform_voucher_controller.h"
#include "jform_model.h"
#include "parties_model.h"
#include "financial_years_model.h"
#include "../services/accounting_date_service.h"
#include "../services/financial_math_service.h"
#include <cmath>

JFormLineItemsModel::JFormLineItemsModel(QObject *parent)
    : QAbstractListModel(parent) {
}

int JFormLineItemsModel::rowCount(const QModelIndex &parent) const {
    if (parent.isValid()) return 0;
    return m_items.size();
}

QVariant JFormLineItemsModel::data(const QModelIndex &index, int role) const {
    if (!index.isValid() || index.row() < 0 || index.row() >= m_items.size()) {
        return QVariant();
    }

    const auto &item = m_items.at(index.row());
    switch (role) {
    case ItemNameRole: return item.itemName;
    case BagsRole: return item.bags;
    case LooseRole: return item.loose > 0.0001 ? item.loose : QVariant(0.0);
    case PackingRole: return item.packing;
    case WeightRole: return item.weight;
    case RateRole: return item.rate;
    case AmountRole: return item.amount;
    default: return QVariant();
    }
}

bool JFormLineItemsModel::setData(const QModelIndex &index, const QVariant &value, int role) {
    if (!index.isValid() || index.row() < 0 || index.row() >= m_items.size()) {
        return false;
    }

    auto &item = m_items[index.row()];
    switch (role) {
    case ItemNameRole: item.itemName = value.toString(); break;
    case BagsRole: item.bags = value.toInt(); break;
    case LooseRole: item.loose = value.toDouble(); break;
    case PackingRole: item.packing = value.toString(); break;
    case WeightRole: item.weight = value.toDouble(); break;
    case RateRole: item.rate = value.toDouble(); break;
    case AmountRole: item.amount = value.toDouble(); break;
    default: return false;
    }

    emit dataChanged(index, index, {role});
    emit itemsChanged();
    return true;
}

QHash<int, QByteArray> JFormLineItemsModel::roleNames() const {
    QHash<int, QByteArray> roles;
    roles[ItemNameRole] = "itemName";
    roles[BagsRole] = "bags";
    roles[LooseRole] = "loose";
    roles[PackingRole] = "packing";
    roles[WeightRole] = "weight";
    roles[RateRole] = "rate";
    roles[AmountRole] = "amount";
    return roles;
}

void JFormLineItemsModel::appendRow(const QString &itemName, int bags, double loose,
                                    const QString &packing, double weight, double rate, double amount) {
    beginInsertRows(QModelIndex(), m_items.size(), m_items.size());
    m_items.append({itemName, bags, loose, packing, weight, rate, amount});
    endInsertRows();
    emit itemsChanged();
}

void JFormLineItemsModel::removeRowAt(int index) {
    if (index < 0 || index >= m_items.size()) return;
    beginRemoveRows(QModelIndex(), index, index);
    m_items.removeAt(index);
    endRemoveRows();
    emit itemsChanged();
}

void JFormLineItemsModel::clear() {
    beginResetModel();
    m_items.clear();
    endResetModel();
    emit itemsChanged();
}

QVariantMap JFormLineItemsModel::getRow(int index) const {
    QVariantMap map;
    if (index < 0 || index >= m_items.size()) return map;
    const auto &item = m_items.at(index);
    map["itemName"] = item.itemName;
    map["bags"] = item.bags;
    map["loose"] = item.loose;
    map["packing"] = item.packing;
    map["weight"] = item.weight;
    map["rate"] = item.rate;
    map["amount"] = item.amount;
    return map;
}

QVariantList JFormLineItemsModel::toVariantList() const {
    QVariantList list;
    for (const auto &item : m_items) {
        QVariantMap map;
        map["item_id"] = 0;
        map["item_name"] = item.itemName;
        map["bags"] = item.bags;
        map["loose_weight"] = item.loose;
        map["packing"] = item.packing;
        map["weight"] = item.weight;
        map["rate"] = item.rate;
        map["amount"] = item.amount;
        list.append(map);
    }
    return list;
}

// -------------------------------------------------------------
// JFormVoucherController
// -------------------------------------------------------------
JFormVoucherController::JFormVoucherController(QObject *parent)
    : QObject(parent) {
    connect(&m_lineItemsModel, &JFormLineItemsModel::itemsChanged, this, &JFormVoucherController::recalculateTotals);
}

void JFormVoucherController::setVoucherDate(const QString &d) {
    QString resolved = AccountingDateService::instance().resolveDate(d);
    if (m_voucherDate != resolved) {
        m_voucherDate = resolved;
        emit voucherDateChanged();
        emit dayOfWeekChanged();
    }
}

QString JFormVoucherController::dayOfWeek() const {
    return AccountingDateService::instance().getDayOfWeek(m_voucherDate);
}

void JFormVoucherController::setZimidarName(const QString &name) {
    if (m_zimidarName != name) {
        m_zimidarName = name;
        emit zimidarNameChanged();
        updateZimidarBalance(name);
    }
}

void JFormVoucherController::updateZimidarBalance(const QString &partyName) {
    m_zimidarName = partyName.trimmed();
    emit zimidarNameChanged();

    PartiesModel pModel;
    auto party = pModel.get_party_by_name(m_zimidarName);
    if (!party.isEmpty() && party.contains("id")) {
        m_zimidarId = party.value("id").toInt();
        emit zimidarIdChanged();

        JFormModel jModel;
        auto res = jModel.get_zimidar_balance(m_zimidarId);
        m_zimidarBalanceText = res.value("formatted_balance", "Date Bal. 0.00 Dr").toString();
        emit zimidarBalanceTextChanged();
    } else {
        m_zimidarId = 0;
        emit zimidarIdChanged();
        m_zimidarBalanceText = "Date Bal. 0.00 Dr";
        emit zimidarBalanceTextChanged();
    }
}

void JFormVoucherController::setPartyName(const QString &name) {
    if (m_partyName != name) {
        m_partyName = name;
        emit partyNameChanged();

        PartiesModel pModel;
        auto party = pModel.get_party_by_name(m_partyName);
        m_partyId = party.value("id", 0).toInt();
        emit partyIdChanged();
    }
}

void JFormVoucherController::setBonusAmount(double v) {
    if (std::abs(m_bonusAmount - v) > 0.001) {
        m_bonusAmount = v;
        recalculateTotals();
    }
}

void JFormVoucherController::setReliefAmount(double v) {
    if (std::abs(m_reliefAmount - v) > 0.001) {
        m_reliefAmount = v;
        recalculateTotals();
    }
}

void JFormVoucherController::setLabourAmount(double v) {
    if (std::abs(m_labourAmount - v) > 0.001) {
        m_labourAmount = v;
        recalculateTotals();
    }
}

void JFormVoucherController::setRoundOffAmount(double v) {
    if (std::abs(m_roundOffAmount - v) > 0.001) {
        m_roundOffAmount = v;
        m_grandTotal = FinancialMathService::instance().round2(m_subtotalAmount - m_labourAmount + m_roundOffAmount);
        emit totalsChanged();
    }
}

QString JFormVoucherController::goodsAmountFmt() const {
    return FinancialMathService::instance().formatInr(m_goodsAmount);
}

QString JFormVoucherController::subtotalAmountFmt() const {
    return FinancialMathService::instance().formatInr(m_subtotalAmount);
}

QString JFormVoucherController::grandTotalFmt() const {
    return FinancialMathService::instance().formatInr(m_grandTotal);
}

void JFormVoucherController::resetForm(const QString &workingDate) {
    JFormModel jModel;
    auto info = jModel.get_next_voucher_info();
    m_voucherNo = info.value("next_voucher_no", 1).toInt();
    emit voucherNoChanged();
    m_jformNo = info.value("next_jform_no", "1").toString();
    emit jformNoChanged();

    FinancialYearsModel fyModel;
    QString wDate = !workingDate.isEmpty() ? workingDate : fyModel.get_working_date();
    if (wDate.isEmpty()) {
        wDate = QDate::currentDate().toString("dd/MM/yyyy");
    }
    setVoucherDate(wDate);

    m_dueDays = 0;
    emit dueDaysChanged();

    m_zimidarId = 0;
    emit zimidarIdChanged();
    m_zimidarName.clear();
    emit zimidarNameChanged();
    m_zimidarBalanceText = "Date Bal. 0.00 Dr";
    emit zimidarBalanceTextChanged();

    m_partyId = 0;
    emit partyIdChanged();
    m_partyName = "Self Purchase";
    emit partyNameChanged();

    m_saleStatus = "Zimidara Self Purchase";
    emit saleStatusChanged();
    m_procurementMode = "Direct Farmer";
    emit procurementModeChanged();

    m_vehicleNo.clear(); emit vehicleNoChanged();
    m_driverName.clear(); emit driverNameChanged();
    m_gatePassNo.clear(); emit gatePassNoChanged();
    m_ewayBillNo.clear(); emit ewayBillNoChanged();
    m_billTime.clear(); emit billTimeChanged();
    m_saudaDate.clear(); emit saudaDateChanged();
    m_mandiPlace.clear(); emit mandiPlaceChanged();
    m_lotNo.clear(); emit lotNoChanged();
    m_grade.clear(); emit gradeChanged();
    m_transportName.clear(); emit transportNameChanged();
    m_brokerName.clear(); emit brokerNameChanged();
    m_challanNo.clear(); emit challanNoChanged();
    m_kandaWeight.clear(); emit kandaWeightChanged();
    m_narration.clear(); emit narrationChanged();

    m_bonusAmount = 0.0;
    m_reliefAmount = 0.0;
    m_labourAmount = 0.0;
    m_roundOffAmount = 0.0;

    m_lineItemsModel.clear();
    m_statusMessage.clear();
    m_isError = false;
    emit statusChanged();

    recalculateTotals();
}

void JFormVoucherController::addItemRow(const QString &itemName, int bags, double loose,
                                       const QString &packing, double weight, double rate, double amount) {
    m_lineItemsModel.appendRow(itemName, bags, loose, packing, weight, rate, amount);
    recalculateTotals();
}

void JFormVoucherController::removeLineItem(int index) {
    m_lineItemsModel.removeRowAt(index);
    recalculateTotals();
}

void JFormVoucherController::recalculateTotals() {
    int sumBags = 0;
    double sumWeight = 0.0;
    double sumAmount = 0.0;

    for (int i = 0; i < m_lineItemsModel.count(); ++i) {
        auto r = m_lineItemsModel.getRow(i);
        sumBags += r.value("bags").toInt();
        sumWeight += r.value("weight").toDouble();
        sumAmount += r.value("amount").toDouble();
    }

    m_totalBags = sumBags;
    m_totalWeight = std::round(sumWeight * 1000.0) / 1000.0;
    m_goodsAmount = FinancialMathService::instance().round2(sumAmount);

    m_subtotalAmount = FinancialMathService::instance().round2(m_goodsAmount + m_bonusAmount + m_reliefAmount);
    double netBeforeRound = m_subtotalAmount - m_labourAmount;

    double rounded = std::round(netBeforeRound);
    m_roundOffAmount = FinancialMathService::instance().round2(rounded - netBeforeRound);
    m_grandTotal = rounded;

    emit totalsChanged();
}

bool JFormVoucherController::saveVoucher() {
    m_statusMessage.clear();
    m_isError = false;

    if (m_zimidarName.trimmed().isEmpty()) {
        m_statusMessage = "Please select a Zimidar (Farmer) Ledger Account.";
        m_isError = true;
        emit statusChanged();
        return false;
    }

    if (m_lineItemsModel.count() == 0) {
        m_statusMessage = "Please enter at least one Item in the grid.";
        m_isError = true;
        emit statusChanged();
        return false;
    }

    // FY date check
    FinancialYearsModel fyModel;
    auto activeFy = fyModel.get_active_year();
    int fyStartYear = QDate::fromString(activeFy.value("start_date").toString(), "yyyy-MM-dd").year();
    if (fyStartYear <= 0) fyStartYear = 2026;

    if (!AccountingDateService::instance().validateDateInFy(m_voucherDate, fyStartYear)) {
        m_statusMessage = "Voucher Date is outside the active Financial Year.";
        m_isError = true;
        emit statusChanged();
        return false;
    }

    QString isoDate = AccountingDateService::instance().toIso(m_voucherDate);

    QVariantMap headerData;
    headerData["voucher_no"] = m_voucherNo;
    headerData["voucher_date"] = isoDate;
    headerData["jform_no"] = m_jformNo;
    headerData["zimidar_id"] = m_zimidarId;
    headerData["zimidar_name"] = m_zimidarName;
    headerData["party_id"] = m_partyId;
    headerData["party_name"] = m_partyName;
    headerData["auction_sale_status"] = m_saleStatus;
    headerData["due_days"] = m_dueDays;
    headerData["vehicle_no"] = m_vehicleNo;
    headerData["driver_name"] = m_driverName;
    headerData["gatePassNo"] = m_gatePassNo;
    headerData["gate_pass_no"] = m_gatePassNo;
    headerData["eway_bill_no"] = m_ewayBillNo;
    headerData["bill_time"] = m_billTime;
    headerData["sauda_date"] = m_saudaDate;
    headerData["mandi_place"] = m_mandiPlace;
    headerData["procurement_mode"] = m_procurementMode;
    headerData["lot_no"] = m_lotNo;
    headerData["grade"] = m_grade;
    headerData["transport_name"] = m_transportName;
    headerData["broker_name"] = m_brokerName;
    headerData["challan_no"] = m_challanNo;
    headerData["kanda_weight"] = m_kandaWeight;
    headerData["total_bags"] = m_totalBags;
    headerData["total_weight"] = m_totalWeight;
    headerData["goods_amount"] = m_goodsAmount;
    headerData["bonus_amount"] = m_bonusAmount;
    headerData["relief_amount"] = m_reliefAmount;
    headerData["subtotal_amount"] = m_subtotalAmount;
    headerData["labour_amount"] = m_labourAmount;
    headerData["round_off"] = m_roundOffAmount;
    headerData["grand_total"] = m_grandTotal;
    headerData["narration"] = m_narration.trimmed();

    QVariantList itemsList = m_lineItemsModel.toVariantList();

    JFormModel jModel;
    bool ok = jModel.save_jform_voucher(headerData, itemsList);

    if (ok) {
        m_statusMessage = QString("J-Form Voucher #%1 (Form J: %2) posted successfully!").arg(QString::number(m_voucherNo), m_jformNo);
        m_isError = false;
        emit statusChanged();
        emit voucherSaved();
        resetForm();
        return true;
    } else {
        m_statusMessage = "Failed to save J-Form Voucher in database.";
        m_isError = true;
        emit statusChanged();
        return false;
    }
}
