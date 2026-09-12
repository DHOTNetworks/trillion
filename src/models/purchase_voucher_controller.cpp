#include "purchase_voucher_controller.h"
#include "services/accounting_date_service.h"
#include "services/financial_math_service.h"
#include "purchase_model.h"
#include "parties_model.h"
#include "financial_years_model.h"
#include <QDebug>

// ============================================================================
// PurchaseLineItemsModel Implementation
// ============================================================================

PurchaseLineItemsModel::PurchaseLineItemsModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

int PurchaseLineItemsModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid()) return 0;
    return m_items.size();
}

QVariant PurchaseLineItemsModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_items.size()) {
        return QVariant();
    }

    const auto &item = m_items.at(index.row());
    switch (role) {
    case ItemNameRole: return item.itemName;
    case HsnCodeRole: return item.hsnCode;
    case UnitRole: return item.unit;
    case BagsRole: return item.bags;
    case PackingKgRole: return item.packingKg;
    case WeightQtlRole: return item.weightQtl;
    case RateRole: return item.rate;
    case AmountRole: return item.amount;
    case IsStockRole: return item.isStock;
    default: return QVariant();
    }
}

bool PurchaseLineItemsModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_items.size()) {
        return false;
    }

    auto &item = m_items[index.row()];
    bool changed = false;

    switch (role) {
    case ItemNameRole:
        if (item.itemName != value.toString()) { item.itemName = value.toString(); changed = true; }
        break;
    case HsnCodeRole:
        if (item.hsnCode != value.toString()) { item.hsnCode = value.toString(); changed = true; }
        break;
    case UnitRole:
        if (item.unit != value.toString()) { item.unit = value.toString(); changed = true; }
        break;
    case BagsRole:
        if (item.bags != value.toInt()) { item.bags = value.toInt(); changed = true; }
        break;
    case PackingKgRole:
        if (std::abs(item.packingKg - value.toDouble()) > 0.001) { item.packingKg = value.toDouble(); changed = true; }
        break;
    case WeightQtlRole:
        if (std::abs(item.weightQtl - value.toDouble()) > 0.001) { item.weightQtl = value.toDouble(); changed = true; }
        break;
    case RateRole:
        if (std::abs(item.rate - value.toDouble()) > 0.001) { item.rate = value.toDouble(); changed = true; }
        break;
    case AmountRole:
        if (std::abs(item.amount - value.toDouble()) > 0.001) { item.amount = value.toDouble(); changed = true; }
        break;
    case IsStockRole:
        if (item.isStock != value.toBool()) { item.isStock = value.toBool(); changed = true; }
        break;
    }

    if (changed) {
        emit dataChanged(index, index, {role});
        emit itemsChanged();
        return true;
    }
    return false;
}

QHash<int, QByteArray> PurchaseLineItemsModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[ItemNameRole] = "itemName";
    roles[HsnCodeRole] = "hsnCode";
    roles[UnitRole] = "unit";
    roles[BagsRole] = "bags";
    roles[PackingKgRole] = "packingKg";
    roles[WeightQtlRole] = "weightQtl";
    roles[RateRole] = "rate";
    roles[AmountRole] = "amount";
    roles[IsStockRole] = "isStock";
    return roles;
}

void PurchaseLineItemsModel::appendRow(const QString &itemName, const QString &hsn, const QString &unit,
                                      int bags, double packing, double weight, double rate, double amount)
{
    beginInsertRows(QModelIndex(), m_items.size(), m_items.size());
    PurchaseLineItem item;
    item.itemName = itemName;
    item.hsnCode = hsn;
    item.unit = unit.isEmpty() ? "QTL" : unit;
    item.bags = bags;
    item.packingKg = packing;
    item.weightQtl = weight;
    item.rate = rate;
    item.amount = (amount > 0.001) ? amount : FinancialMathService::round2(weight * rate);
    item.isStock = true;
    m_items.append(item);
    endInsertRows();
    emit itemsChanged();
}

void PurchaseLineItemsModel::removeRowAt(int index)
{
    if (index < 0 || index >= m_items.size()) return;
    beginRemoveRows(QModelIndex(), index, index);
    m_items.removeAt(index);
    endRemoveRows();
    emit itemsChanged();
}

void PurchaseLineItemsModel::clear()
{
    beginResetModel();
    m_items.clear();
    endResetModel();
    emit itemsChanged();
}

QVariantMap PurchaseLineItemsModel::getRow(int index) const
{
    QVariantMap map;
    if (index < 0 || index >= m_items.size()) return map;
    const auto &item = m_items.at(index);
    map["itemName"] = item.itemName;
    map["hsnCode"] = item.hsnCode;
    map["unit"] = item.unit;
    map["bags"] = item.bags;
    map["packingKg"] = item.packingKg;
    map["weightQtl"] = item.weightQtl;
    map["rate"] = item.rate;
    map["amount"] = item.amount;
    map["isStock"] = item.isStock;
    return map;
}

void PurchaseLineItemsModel::setRowProperty(int index, const QString &property, const QVariant &value)
{
    if (index < 0 || index >= m_items.size()) return;
    QModelIndex idx = this->index(index, 0);
    if (property == "itemName") setData(idx, value, ItemNameRole);
    else if (property == "hsnCode") setData(idx, value, HsnCodeRole);
    else if (property == "unit") setData(idx, value, UnitRole);
    else if (property == "bags") setData(idx, value, BagsRole);
    else if (property == "packingKg") setData(idx, value, PackingKgRole);
    else if (property == "weightQtl") setData(idx, value, WeightQtlRole);
    else if (property == "rate") setData(idx, value, RateRole);
    else if (property == "amount") setData(idx, value, AmountRole);
    else if (property == "isStock") setData(idx, value, IsStockRole);
}

QVariantList PurchaseLineItemsModel::toVariantList() const
{
    QVariantList list;
    for (const auto &item : m_items) {
        QVariantMap map;
        map["item_name"] = item.itemName;
        map["hsn_code"] = item.hsnCode;
        map["unit"] = item.unit;
        map["bags"] = item.bags;
        map["packing_kg"] = item.packingKg;
        map["weight_qtl"] = item.weightQtl;
        map["rate"] = item.rate;
        map["amount"] = item.amount;
        list.append(map);
    }
    return list;
}

void PurchaseLineItemsModel::loadFromVariantList(const QVariantList &list)
{
    beginResetModel();
    m_items.clear();
    for (const auto &v : list) {
        QVariantMap map = v.toMap();
        PurchaseLineItem item;
        item.itemName = map.value("item_name").toString();
        item.hsnCode = map.value("hsn_code").toString();
        item.unit = map.value("unit", "QTL").toString();
        item.bags = map.value("bags").toInt();
        item.packingKg = map.value("packing_kg").toDouble();
        item.weightQtl = map.value("weight_qtl").toDouble();
        item.rate = map.value("rate").toDouble();
        item.amount = map.value("amount").toDouble();
        item.isStock = true;
        m_items.append(item);
    }
    endResetModel();
    emit itemsChanged();
}

// ============================================================================
// PurchaseVoucherController Implementation
// ============================================================================

PurchaseVoucherController::PurchaseVoucherController(QObject *parent)
    : QObject(parent)
{
    connect(&m_lineItemsModel, &PurchaseLineItemsModel::itemsChanged, this, &PurchaseVoucherController::recalculateTotals);
}

void PurchaseVoucherController::setEditingInvoiceId(int id)
{
    if (m_editingInvoiceId != id) {
        m_editingInvoiceId = id;
        emit editingInvoiceIdChanged();
        emit isEditModeChanged();
    }
}

void PurchaseVoucherController::setInvoiceDate(const QString &v)
{
    QString resolved = AccountingDateService::instance().resolveDate(v);
    if (m_invoiceDate != resolved) {
        m_invoiceDate = resolved;
        emit invoiceDateChanged();
        emit dayOfWeekChanged();
    }
}

QString PurchaseVoucherController::dayOfWeek() const
{
    return AccountingDateService::instance().getDayOfWeek(m_invoiceDate);
}

void PurchaseVoucherController::setGstin(const QString &v)
{
    QString upper = v.trimmed().toUpper();
    if (m_gstin != upper) {
        m_gstin = upper;
        emit gstinChanged();

        if (m_gstin.length() >= 2) {
            QString stateCode = m_gstin.left(2);
            bool isOtherState = (stateCode != "03"); // 03 is Punjab
            setIsInterstate(isOtherState);
        }
    }
}

QString PurchaseVoucherController::taxableAmountFmt() const
{
    return FinancialMathService::instance().formatInr(m_taxableAmount);
}

QString PurchaseVoucherController::cgstAmountFmt() const
{
    return FinancialMathService::instance().formatInr(m_cgstAmount);
}

QString PurchaseVoucherController::sgstAmountFmt() const
{
    return FinancialMathService::instance().formatInr(m_sgstAmount);
}

QString PurchaseVoucherController::igstAmountFmt() const
{
    return FinancialMathService::instance().formatInr(m_igstAmount);
}

QString PurchaseVoucherController::roundOffFmt() const
{
    return FinancialMathService::instance().formatInr(m_roundOff);
}

QString PurchaseVoucherController::grandTotalFmt() const
{
    return FinancialMathService::instance().formatInr(m_grandTotal);
}

void PurchaseVoucherController::resetForm(const QString &workingDate)
{
    m_editingInvoiceId = 0;
    emit editingInvoiceIdChanged();
    emit isEditModeChanged();

    m_invoiceDate = workingDate.isEmpty() ? AccountingDateService::instance().currentWorkingDate() : workingDate;
    emit invoiceDateChanged();
    emit dayOfWeekChanged();

    m_partyLedger.clear();
    emit partyLedgerChanged();

    m_gstin.clear();
    emit gstinChanged();

    m_dueDays = 30;
    emit dueDaysChanged();

    m_isInterstate = false;
    emit isInterstateChanged();

    m_freightCharges = 0.0;
    m_tcsRate = 0.0;
    m_dami = 0.0;
    m_labour = 0.0;
    m_auction = 0.0;
    m_marketFee = 0.0;
    m_hrdf = 0.0;
    m_otherExp = 0.0;

    m_vehicleNo.clear();
    emit vehicleNoChanged();

    m_ewayBillNo.clear();
    emit ewayBillNoChanged();

    m_grNo.clear();
    emit grNoChanged();

    m_transportName.clear();
    emit transportNameChanged();

    m_shippingAddress.clear();
    emit shippingAddressChanged();

    m_poNo.clear();
    emit poNoChanged();

    m_narration.clear();
    emit narrationChanged();

    m_lineItemsModel.clear();
    recalculateTotals();

    m_statusMessage.clear();
    m_isError = false;
    emit statusChanged();
}

bool PurchaseVoucherController::loadInvoice(int invoiceId)
{
    PurchaseModel purchaseModel;
    QVariantMap inv = purchaseModel.get_purchase_invoice(QString::number(invoiceId));
    if (inv.isEmpty() || !inv.contains("id")) {
        m_statusMessage = "Invoice not found";
        m_isError = true;
        emit statusChanged();
        return false;
    }

    setEditingInvoiceId(inv.value("id").toInt());
    setInvoiceNo(inv.value("invoice_no").toString());
    setVoucherNo(inv.value("voucher_no", inv.value("invoice_no").toString()).toString());
    setInvoiceDate(inv.value("invoice_date").toString());
    setPartyLedger(inv.value("supplier_name").toString());
    setGstin(inv.value("gstin").toString());
    setPaymentMode(inv.value("payment_mode", "Credit").toString());
    setMarketType(inv.value("market_type", "Market Type (With Stock)").toString());
    setSaleStatus(inv.value("sale_status", "Self Purchase").toString());
    setVehicleNo(inv.value("vehicle_no").toString());
    setEwayBillNo(inv.value("eway_bill_no").toString());
    setNarration(inv.value("narration").toString());
    setDueDays(inv.value("due_days", 30).toInt());
    setFreightCharges(inv.value("freight_charges", 0.0).toDouble());
    setTcsRate(inv.value("tcs_rate", 0.0).toDouble());
    setGstRate(inv.value("gst_pct", 5.0).toDouble());

    setDami(inv.value("dami", 0.0).toDouble());
    setLabour(inv.value("labour", 0.0).toDouble());
    setAuction(inv.value("auction", 0.0).toDouble());
    setMarketFee(inv.value("m_fee", 0.0).toDouble());
    setHrdf(inv.value("hrdf", 0.0).toDouble());
    setOtherExp(inv.value("other_exp", 0.0).toDouble());

    QVariantList items = inv.value("items").toList();
    if (items.isEmpty() && !inv.value("item_name").toString().isEmpty()) {
        QVariantMap single;
        single["item_name"] = inv.value("item_name");
        single["hsn_code"] = inv.value("hsn_code");
        single["unit"] = inv.value("unit", "QTL");
        single["bags"] = inv.value("bag_count");
        single["weight_qtl"] = inv.value("weight_qtl");
        single["rate"] = inv.value("rate_per_qtl");
        single["amount"] = inv.value("taxable_amount");
        items.append(single);
    }
    m_lineItemsModel.loadFromVariantList(items);

    recalculateTotals();
    m_statusMessage = "Loaded purchase invoice " + m_invoiceNo;
    m_isError = false;
    emit statusChanged();
    return true;
}

bool PurchaseVoucherController::loadInvoiceByNo(const QString &invNo)
{
    PurchaseModel purchaseModel;
    QVariantMap inv = purchaseModel.get_purchase_invoice(invNo.trimmed());
    if (inv.contains("id")) {
        return loadInvoice(inv.value("id").toInt());
    }
    return false;
}

void PurchaseVoucherController::recalculateTotals()
{
    int bags = 0;
    double weight = 0.0;
    double taxable = 0.0;

    for (const auto &item : m_lineItemsModel.items()) {
        bags += item.bags;
        weight += item.weightQtl;
        taxable += item.amount;
    }

    m_totalBags = bags;
    m_totalWeightQtl = FinancialMathService::round2(weight);
    m_taxableAmount = FinancialMathService::round2(taxable);

    // GST Calculation
    QVariantMap gst = FinancialMathService::instance().calculateGst(m_taxableAmount, m_gstRate, m_isInterstate);
    m_cgstAmount = gst.value("cgst").toDouble();
    m_sgstAmount = gst.value("sgst").toDouble();
    m_igstAmount = gst.value("igst").toDouble();
    m_totalTaxAmount = gst.value("totalTax").toDouble();

    // Additional Expenses
    double expenses = m_dami + m_labour + m_auction + m_marketFee + m_hrdf + m_otherExp + m_freightCharges;

    // TCS Calculation
    m_tcsAmount = (m_tcsRate > 0.001) ? FinancialMathService::round2((m_taxableAmount + m_totalTaxAmount) * (m_tcsRate / 100.0)) : 0.0;

    // Gross Total before roundoff
    double gross = m_taxableAmount + m_totalTaxAmount + expenses + m_tcsAmount;
    m_roundOff = FinancialMathService::instance().calculateRoundOff(gross);
    m_grandTotal = FinancialMathService::instance().calculateGrandTotal(gross);

    emit totalsChanged();
}

bool PurchaseVoucherController::saveVoucher()
{
    if (m_partyLedger.trimmed().isEmpty()) {
        m_statusMessage = "Please select a Supplier / Party Account.";
        m_isError = true;
        emit statusChanged();
        return false;
    }

    if (m_lineItemsModel.count() == 0) {
        m_statusMessage = "Please add at least one line item.";
        m_isError = true;
        emit statusChanged();
        return false;
    }

    // Validate FY boundary
    FinancialYearsModel fyModel;
    QVariantMap activeFy = fyModel.get_active_year();
    int currentFy = 0;
    if (activeFy.contains("start_date")) {
        currentFy = QDate::fromString(activeFy.value("start_date").toString(), "yyyy-MM-dd").year();
    }
    QString errorMsg = AccountingDateService::instance().getValidationErrorMessage(m_invoiceDate, currentFy, activeFy.value("year_name").toString());
    if (!errorMsg.isEmpty()) {
        m_statusMessage = errorMsg;
        m_isError = true;
        emit statusChanged();
        return false;
    }

    recalculateTotals();

    PurchaseModel purchaseModel;
    QString firstItem = m_lineItemsModel.items().first().itemName;
    QString firstHsn = m_lineItemsModel.items().first().hsnCode;
    double firstRate = m_lineItemsModel.items().first().rate;

    bool ok = false;
    if (m_editingInvoiceId > 0) {
        ok = purchaseModel.update_purchase_invoice_full(
            m_editingInvoiceId, m_invoiceNo, m_invoiceDate, m_partyLedger,
            m_gstin, firstItem, firstHsn, m_totalBags, m_totalWeightQtl, firstRate,
            m_taxableAmount, m_gstRate, m_cgstAmount, m_sgstAmount, m_igstAmount,
            m_roundOff, m_grandTotal, m_paymentMode, m_vehicleNo, m_ewayBillNo,
            m_narration, m_saleStatus, "Paid", m_dami, m_labour, m_auction,
            m_marketFee, m_hrdf, m_otherExp, 0.0, 0.0, 0.0, 0.0, m_grNo, "", "", "",
            m_shippingAddress, m_poNo, "", "", m_transportName, "", m_voucherNo,
            m_lineItemsModel.toVariantList(), m_marketType, m_dueDays, "",
            m_freightCharges, m_tcsAmount, m_tcsRate, m_taxStatus, ""
        );
    } else {
        ok = purchaseModel.add_purchase_invoice_full(
            m_invoiceNo, m_invoiceDate, m_partyLedger,
            m_gstin, firstItem, firstHsn, m_totalBags, m_totalWeightQtl, firstRate,
            m_taxableAmount, m_gstRate, m_cgstAmount, m_sgstAmount, m_igstAmount,
            m_roundOff, m_grandTotal, m_paymentMode, m_vehicleNo, m_ewayBillNo,
            m_narration, m_saleStatus, "Paid", m_dami, m_labour, m_auction,
            m_marketFee, m_hrdf, m_otherExp, 0.0, 0.0, 0.0, 0.0, m_grNo, "", "", "",
            m_shippingAddress, m_poNo, "", "", m_transportName, "", m_voucherNo,
            m_lineItemsModel.toVariantList(), m_marketType, m_dueDays, "",
            m_freightCharges, m_tcsAmount, m_tcsRate, m_taxStatus, ""
        );
    }

    if (ok) {
        m_statusMessage = "Purchase Invoice " + m_invoiceNo + " saved successfully!";
        m_isError = false;
        emit statusChanged();
        emit savedSuccess(m_invoiceNo);
        return true;
    } else {
        m_statusMessage = "Failed to save Purchase Invoice. Check database logs.";
        m_isError = true;
        emit statusChanged();
        return false;
    }
}
