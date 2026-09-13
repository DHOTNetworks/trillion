#include "debit_credit_note_controller.h"
#include "../database_manager.h"
#include "../services/financial_math_service.h"
#include "../services/accounting_date_service.h"
#include <QDebug>
#include <QDate>
#include <QTime>
#include <cmath>

// ---------------------------------------------------------------------------
// DebitCreditNoteItemsModel Implementation
// ---------------------------------------------------------------------------

DebitCreditNoteItemsModel::DebitCreditNoteItemsModel(QObject *parent)
    : QAbstractListModel(parent) {
}

int DebitCreditNoteItemsModel::rowCount(const QModelIndex &parent) const {
    if (parent.isValid()) return 0;
    return m_items.size();
}

QVariant DebitCreditNoteItemsModel::data(const QModelIndex &index, int role) const {
    if (!index.isValid() || index.row() < 0 || index.row() >= m_items.size()) {
        return QVariant();
    }

    const auto &it = m_items.at(index.row());
    switch (role) {
    case ItemNameRole: return it.itemName;
    case HsnCodeRole: return it.hsnCode;
    case UnitRole: return it.unit;
    case BagsRole: return it.bags;
    case WeightQtlRole: return it.weightQtl;
    case RateRole: return it.rate;
    case TaxableAmountRole: return it.taxableAmount;
    case GstPctRole: return it.gstPct;
    default: return QVariant();
    }
}

bool DebitCreditNoteItemsModel::setData(const QModelIndex &index, const QVariant &value, int role) {
    if (!index.isValid() || index.row() < 0 || index.row() >= m_items.size()) {
        return false;
    }

    auto &it = m_items[index.row()];
    switch (role) {
    case ItemNameRole: it.itemName = value.toString(); break;
    case HsnCodeRole: it.hsnCode = value.toString(); break;
    case UnitRole: it.unit = value.toString(); break;
    case BagsRole: it.bags = value.toInt(); break;
    case WeightQtlRole: it.weightQtl = value.toDouble(); break;
    case RateRole: it.rate = value.toDouble(); break;
    case TaxableAmountRole: it.taxableAmount = value.toDouble(); break;
    case GstPctRole: it.gstPct = value.toDouble(); break;
    default: return false;
    }

    emit dataChanged(index, index, {role});
    emit itemDataChanged();
    return true;
}

QHash<int, QByteArray> DebitCreditNoteItemsModel::roleNames() const {
    QHash<int, QByteArray> roles;
    roles[ItemNameRole] = "itemName";
    roles[HsnCodeRole] = "hsnCode";
    roles[UnitRole] = "unit";
    roles[BagsRole] = "bags";
    roles[WeightQtlRole] = "weightQtl";
    roles[RateRole] = "rate";
    roles[TaxableAmountRole] = "taxableAmount";
    roles[GstPctRole] = "gstPct";
    return roles;
}

void DebitCreditNoteItemsModel::setItems(const QVector<DebitCreditNoteItem> &items) {
    beginResetModel();
    m_items = items;
    endResetModel();
    emit countChanged();
    emit itemDataChanged();
}

void DebitCreditNoteItemsModel::addItem(const QString &itemName, const QString &hsn,
                                        int bags, double weight, double rate,
                                        double amount, double gst) {
    beginInsertRows(QModelIndex(), m_items.size(), m_items.size());
    DebitCreditNoteItem it;
    it.itemName = itemName.isEmpty() ? "Rice Byproduct" : itemName;
    it.hsnCode = hsn.isEmpty() ? "1006" : hsn;
    it.unit = "QTL";
    it.bags = bags;
    it.weightQtl = weight;
    it.rate = rate;
    it.taxableAmount = (amount > 0.0) ? amount : ((weight > 0.0) ? (weight * rate) : (bags * rate));
    it.gstPct = gst;
    m_items.append(it);
    endInsertRows();
    emit countChanged();
    emit itemDataChanged();
}

void DebitCreditNoteItemsModel::removeItem(int index) {
    if (index < 0 || index >= m_items.size()) return;
    beginRemoveRows(QModelIndex(), index, index);
    m_items.removeAt(index);
    endRemoveRows();
    if (m_items.isEmpty()) {
        addItem();
    }
    emit countChanged();
    emit itemDataChanged();
}

void DebitCreditNoteItemsModel::updateItem(int index, const QString &field, const QVariant &value) {
    if (index < 0 || index >= m_items.size()) return;
    auto &it = m_items[index];
    if (field == "itemName") it.itemName = value.toString();
    else if (field == "hsnCode") it.hsnCode = value.toString();
    else if (field == "unit") it.unit = value.toString();
    else if (field == "bags") {
        it.bags = value.toInt();
        if (it.weightQtl <= 0.0 && it.rate > 0.0) it.taxableAmount = it.bags * it.rate;
    } else if (field == "weightQtl") {
        it.weightQtl = value.toDouble();
        if (it.weightQtl > 0.0 && it.rate > 0.0) it.taxableAmount = it.weightQtl * it.rate;
    } else if (field == "rate") {
        it.rate = value.toDouble();
        if (it.weightQtl > 0.0) it.taxableAmount = it.weightQtl * it.rate;
        else if (it.bags > 0) it.taxableAmount = it.bags * it.rate;
    } else if (field == "taxableAmount") {
        it.taxableAmount = value.toDouble();
    } else if (field == "gstPct") {
        it.gstPct = value.toDouble();
    }

    QModelIndex idx = createIndex(index, 0);
    emit dataChanged(idx, idx);
    emit itemDataChanged();
}

void DebitCreditNoteItemsModel::clear() {
    beginResetModel();
    m_items.clear();
    endResetModel();
    emit countChanged();
    emit itemDataChanged();
}

QVariantMap DebitCreditNoteItemsModel::get(int index) const {
    if (index < 0 || index >= m_items.size()) return QVariantMap();
    const auto &it = m_items.at(index);
    QVariantMap map;
    map["itemName"] = it.itemName;
    map["hsnCode"] = it.hsnCode;
    map["unit"] = it.unit;
    map["bags"] = it.bags;
    map["weightQtl"] = it.weightQtl;
    map["rate"] = it.rate;
    map["taxableAmount"] = it.taxableAmount;
    map["gstPct"] = it.gstPct;
    return map;
}

QVariantList DebitCreditNoteItemsModel::toVariantList() const {
    QVariantList list;
    for (const auto &it : m_items) {
        QVariantMap map;
        map["itemName"] = it.itemName;
        map["hsnCode"] = it.hsnCode;
        map["unit"] = it.unit;
        map["bags"] = it.bags;
        map["weightQtl"] = it.weightQtl;
        map["rate"] = it.rate;
        map["taxableAmount"] = it.taxableAmount;
        map["gstPct"] = it.gstPct;
        list.append(map);
    }
    return list;
}

// ---------------------------------------------------------------------------
// DebitCreditNoteModel Implementation
// ---------------------------------------------------------------------------

DebitCreditNoteModel::DebitCreditNoteModel(QObject *parent)
    : QAbstractListModel(parent) {
}

int DebitCreditNoteModel::rowCount(const QModelIndex &parent) const {
    if (parent.isValid()) return 0;
    return m_visibleIndices.size();
}

QVariant DebitCreditNoteModel::data(const QModelIndex &index, int role) const {
    if (!index.isValid() || index.row() < 0 || index.row() >= m_visibleIndices.size()) {
        return QVariant();
    }

    int rawIdx = m_visibleIndices.at(index.row());
    const auto &r = m_allRecords.at(rawIdx);
    auto &math = FinancialMathService::instance();

    switch (role) {
    case IdRole: return r.id;
    case NoteTypeRole: return r.noteType;
    case NoteNoRole: return r.noteNo;
    case NoteDateRole: return r.noteDate;
    case NoteTimeRole: return r.noteTime;
    case OriginalInvoiceNoRole: return r.originalInvoiceNo;
    case OriginalInvoiceDateRole: return r.originalInvoiceDate;
    case OriginalInvoiceTypeRole: return r.originalInvoiceType;
    case PartyNameRole: return r.partyName;
    case PartyGstinRole: return r.partyGstin;
    case IsInterstateRole: return r.isInterstate;
    case ReasonCodeRole: return r.reasonCode;
    case AdjustmentTypeRole: return r.adjustmentType;
    case ItemNameRole: return r.itemName;
    case HsnCodeRole: return r.hsnCode;
    case TotalBagsRole: return r.totalBags;
    case TotalWeightRole: return r.totalWeightQtl;
    case TaxableAmountRole: return r.taxableAmount;
    case GstPctRole: return r.gstPct;
    case CgstAmountRole: return r.cgstAmount;
    case SgstAmountRole: return r.sgstAmount;
    case IgstAmountRole: return r.igstAmount;
    case TotalTaxAmountRole: return r.totalTaxAmount;
    case RoundOffRole: return r.roundOff;
    case GrandTotalRole: return r.grandTotal;
    case NarrationRole: return r.narration;
    case TaxableAmountFmtRole: return math.formatInr(r.taxableAmount);
    case TotalTaxAmountFmtRole: return math.formatInr(r.totalTaxAmount);
    case GrandTotalFmtRole: return math.formatInr(r.grandTotal);
    default: return QVariant();
    }
}

QHash<int, QByteArray> DebitCreditNoteModel::roleNames() const {
    QHash<int, QByteArray> roles;
    roles[IdRole] = "noteId";
    roles[NoteTypeRole] = "noteType";
    roles[NoteNoRole] = "noteNo";
    roles[NoteDateRole] = "noteDate";
    roles[NoteTimeRole] = "noteTime";
    roles[OriginalInvoiceNoRole] = "originalInvoiceNo";
    roles[OriginalInvoiceDateRole] = "originalInvoiceDate";
    roles[OriginalInvoiceTypeRole] = "originalInvoiceType";
    roles[PartyNameRole] = "partyName";
    roles[PartyGstinRole] = "partyGstin";
    roles[IsInterstateRole] = "isInterstate";
    roles[ReasonCodeRole] = "reasonCode";
    roles[AdjustmentTypeRole] = "adjustmentType";
    roles[ItemNameRole] = "itemName";
    roles[HsnCodeRole] = "hsnCode";
    roles[TotalBagsRole] = "totalBags";
    roles[TotalWeightRole] = "totalWeightQtl";
    roles[TaxableAmountRole] = "taxableAmount";
    roles[GstPctRole] = "gstPct";
    roles[CgstAmountRole] = "cgstAmount";
    roles[SgstAmountRole] = "sgstAmount";
    roles[IgstAmountRole] = "igstAmount";
    roles[TotalTaxAmountRole] = "totalTaxAmount";
    roles[RoundOffRole] = "roundOff";
    roles[GrandTotalRole] = "grandTotal";
    roles[NarrationRole] = "narration";
    roles[TaxableAmountFmtRole] = "taxableAmountFmt";
    roles[TotalTaxAmountFmtRole] = "totalTaxAmountFmt";
    roles[GrandTotalFmtRole] = "grandTotalFmt";
    return roles;
}

void DebitCreditNoteModel::setAllRecords(const QVector<DebitCreditNoteRecord> &records) {
    beginResetModel();
    m_allRecords = records;
    applyFilter();
    endResetModel();
    emit countChanged();
}

void DebitCreditNoteModel::setFilter(const QString &filterType, const QString &searchQuery) {
    beginResetModel();
    m_filterType = filterType.trimmed().toUpper();
    m_searchQuery = searchQuery.trimmed().toLower();
    applyFilter();
    endResetModel();
    emit countChanged();
}

void DebitCreditNoteModel::applyFilter() {
    m_visibleIndices.clear();

    for (int i = 0; i < m_allRecords.size(); ++i) {
        const auto &r = m_allRecords.at(i);

        // Filter Category check
        if (m_filterType == "CREDIT_NOTES" && r.noteType != "Credit Note") continue;
        if (m_filterType == "DEBIT_NOTES" && r.noteType != "Debit Note") continue;
        if (m_filterType == "SALES_RETURNS" && r.adjustmentType != "Sales Return") continue;
        if (m_filterType == "RATE_CUTS" && !r.adjustmentType.contains("Rate Cut", Qt::CaseInsensitive)) continue;

        // Search Query check
        if (!m_searchQuery.isEmpty()) {
            bool matches = r.noteNo.toLower().contains(m_searchQuery) ||
                           r.originalInvoiceNo.toLower().contains(m_searchQuery) ||
                           r.partyName.toLower().contains(m_searchQuery) ||
                           r.partyGstin.toLower().contains(m_searchQuery) ||
                           r.itemName.toLower().contains(m_searchQuery) ||
                           r.adjustmentType.toLower().contains(m_searchQuery);
            if (!matches) continue;
        }

        m_visibleIndices.append(i);
    }
}

QVariantMap DebitCreditNoteModel::get(int visibleIndex) const {
    if (visibleIndex < 0 || visibleIndex >= m_visibleIndices.size()) return QVariantMap();
    int rawIdx = m_visibleIndices.at(visibleIndex);
    const auto &r = m_allRecords.at(rawIdx);
    auto &math = FinancialMathService::instance();

    QVariantMap map;
    map["id"] = r.id;
    map["noteId"] = r.id;
    map["noteType"] = r.noteType;
    map["noteNo"] = r.noteNo;
    map["noteDate"] = r.noteDate;
    map["noteTime"] = r.noteTime;
    map["originalInvoiceNo"] = r.originalInvoiceNo;
    map["originalInvoiceDate"] = r.originalInvoiceDate;
    map["originalInvoiceType"] = r.originalInvoiceType;
    map["partyName"] = r.partyName;
    map["partyGstin"] = r.partyGstin;
    map["isInterstate"] = r.isInterstate;
    map["reasonCode"] = r.reasonCode;
    map["adjustmentType"] = r.adjustmentType;
    map["itemName"] = r.itemName;
    map["hsnCode"] = r.hsnCode;
    map["totalBags"] = r.totalBags;
    map["totalWeightQtl"] = r.totalWeightQtl;
    map["taxableAmount"] = r.taxableAmount;
    map["gstPct"] = r.gstPct;
    map["cgstAmount"] = r.cgstAmount;
    map["sgstAmount"] = r.sgstAmount;
    map["igstAmount"] = r.igstAmount;
    map["totalTaxAmount"] = r.totalTaxAmount;
    map["roundOff"] = r.roundOff;
    map["grandTotal"] = r.grandTotal;
    map["narration"] = r.narration;
    map["taxableAmountFmt"] = math.formatInr(r.taxableAmount);
    map["totalTaxAmountFmt"] = math.formatInr(r.totalTaxAmount);
    map["grandTotalFmt"] = math.formatInr(r.grandTotal);
    return map;
}

// ---------------------------------------------------------------------------
// DebitCreditNoteController Implementation
// ---------------------------------------------------------------------------

DebitCreditNoteController::DebitCreditNoteController(QObject *parent)
    : QObject(parent), m_itemsModel(this) {
    connect(&m_itemsModel, &DebitCreditNoteItemsModel::itemDataChanged,
            this, &DebitCreditNoteController::recalculateFormTotals);
    connect(&m_itemsModel, &DebitCreditNoteItemsModel::countChanged,
            this, &DebitCreditNoteController::recalculateFormTotals);
    resetDraft();
    reload();
}

QString DebitCreditNoteController::totalCreditAmountFmt() const {
    return FinancialMathService::instance().formatInr(m_totalCreditAmount);
}

QString DebitCreditNoteController::totalDebitAmountFmt() const {
    return FinancialMathService::instance().formatInr(m_totalDebitAmount);
}

QString DebitCreditNoteController::totalGstAdjustedFmt() const {
    return FinancialMathService::instance().formatInr(m_totalGstAdjusted);
}

QString DebitCreditNoteController::netAdjustmentValueFmt() const {
    return FinancialMathService::instance().formatInr(m_netAdjustmentValue);
}

void DebitCreditNoteController::reload() {
    QVector<DebitCreditNoteRecord> records;
    QVariantList rows = DatabaseManager::instance().executeQuery(
        "SELECT * FROM debit_credit_notes ORDER BY id DESC;"
    );

    for (const auto &item : rows) {
        auto map = item.toMap();
        DebitCreditNoteRecord r;
        r.id = map.value("id").toInt();
        r.fyId = map.value("fy_id").toInt();
        r.financialYear = map.value("financial_year").toString();
        r.noteType = map.value("note_type").toString();
        r.noteNo = map.value("note_no").toString();
        r.noteDate = map.value("note_date").toString();
        r.noteTime = map.value("note_time").toString();
        r.originalInvoiceId = map.value("original_invoice_id").toInt();
        r.originalInvoiceNo = map.value("original_invoice_no").toString();
        r.originalInvoiceDate = map.value("original_invoice_date").toString();
        r.originalInvoiceType = map.value("original_invoice_type").toString();
        r.partyId = map.value("party_id").toInt();
        r.partyName = map.value("party_name").toString();
        r.partyGstin = map.value("party_gstin").toString();
        r.stateCode = map.value("state_code").toString();
        r.isInterstate = map.value("is_interstate").toInt() == 1;
        r.reasonCode = map.value("reason_code").toString();
        r.adjustmentType = map.value("adjustment_type").toString();
        r.itemName = map.value("item_name").toString();
        r.hsnCode = map.value("hsn_code").toString();
        r.totalBags = map.value("total_bags").toInt();
        r.totalWeightQtl = map.value("total_weight_qtl").toDouble();
        r.taxableAmount = map.value("taxable_amount").toDouble();
        r.gstPct = map.value("gst_pct").toDouble();
        r.cgstAmount = map.value("cgst_amount").toDouble();
        r.sgstAmount = map.value("sgst_amount").toDouble();
        r.igstAmount = map.value("igst_amount").toDouble();
        r.totalTaxAmount = map.value("total_tax_amount").toDouble();
        r.roundOff = map.value("round_off").toDouble();
        r.grandTotal = map.value("grand_total").toDouble();
        r.narration = map.value("narration").toString();
        r.voucherId = map.value("voucher_id").toInt();
        records.append(r);
    }

    m_model.setAllRecords(records);
    recalculateSummary();
}

void DebitCreditNoteController::recalculateSummary() {
    m_totalCount = m_model.allRecords().size();
    m_totalCreditAmount = 0.0;
    m_totalDebitAmount = 0.0;
    m_totalGstAdjusted = 0.0;

    for (const auto &r : m_model.allRecords()) {
        if (r.noteType == "Credit Note") {
            m_totalCreditAmount += r.grandTotal;
        } else {
            m_totalDebitAmount += r.grandTotal;
        }
        m_totalGstAdjusted += r.totalTaxAmount;
    }

    m_netAdjustmentValue = m_totalCreditAmount - m_totalDebitAmount;
    emit summaryChanged();
}

QVariantMap DebitCreditNoteController::fetchOriginalInvoice(const QString &invType, const QString &invNo) {
    QVariantMap res;
    QString qInvNo = invNo.trimmed();
    if (qInvNo.isEmpty()) return res;

    if (invType == "Purchase") {
        QVariantList rows = DatabaseManager::instance().executeQuery(
            "SELECT id, invoice_no, invoice_date, supplier_name, gstin, state_code, is_interstate, "
            "       gst_pct, item_name, hsn_code, bag_count, weight_qtl, rate_per_qtl, taxable_amount, total_amount "
            "FROM purchase_invoices WHERE invoice_no = ? LIMIT 1;",
            {qInvNo}
        );

        if (!rows.isEmpty()) {
            auto map = rows.first().toMap();
            res["found"] = true;
            res["invoiceId"] = map.value("id").toInt();
            res["invoiceNo"] = map.value("invoice_no").toString();
            res["invoiceDate"] = map.value("invoice_date").toString();
            res["partyName"] = map.value("supplier_name").toString();
            res["partyGstin"] = map.value("gstin").toString();
            res["stateCode"] = map.value("state_code").toString();
            res["isInterstate"] = map.value("is_interstate").toInt() == 1;
            res["gstPct"] = map.value("gst_pct", 5.0).toDouble();
            res["itemName"] = map.value("item_name").toString();
            res["hsnCode"] = map.value("hsn_code").toString();
            res["bagCount"] = map.value("bag_count").toInt();
            res["weightQtl"] = map.value("weight_qtl").toDouble();
            res["rate"] = map.value("rate_per_qtl").toDouble();
            res["taxableAmount"] = map.value("taxable_amount").toDouble();
            res["totalAmount"] = map.value("total_amount").toDouble();

            // Check if line items exist in purchase_invoice_items
            QVariantList itemRows = DatabaseManager::instance().executeQuery(
                "SELECT item_name, hsn_code, unit, bag_count, weight_qtl, rate_per_qtl, taxable_amount, gst_pct, total_amount "
                "FROM purchase_invoice_items WHERE invoice_id = ?;",
                {map.value("id").toInt()}
            );

            QVariantList itemsList;
            if (!itemRows.isEmpty()) {
                for (const auto &it : itemRows) {
                    auto itMap = it.toMap();
                    QVariantMap line;
                    line["itemName"] = itMap.value("item_name").toString();
                    line["hsnCode"] = itMap.value("hsn_code").toString();
                    line["unit"] = itMap.value("unit", "QTL").toString();
                    line["bags"] = itMap.value("bag_count").toInt();
                    line["weightQtl"] = itMap.value("weight_qtl").toDouble();
                    line["rate"] = itMap.value("rate_per_qtl").toDouble();
                    line["taxableAmount"] = itMap.value("taxable_amount").toDouble();
                    line["gstPct"] = itMap.value("gst_pct", 5.0).toDouble();
                    line["totalAmount"] = itMap.value("total_amount").toDouble();
                    itemsList.append(line);
                }
            } else if (!res["itemName"].toString().isEmpty()) {
                QVariantMap single;
                single["itemName"] = res["itemName"];
                single["hsnCode"] = res["hsnCode"];
                single["unit"] = "QTL";
                single["bags"] = res["bagCount"];
                single["weightQtl"] = res["weightQtl"];
                single["rate"] = res["rate"];
                single["taxableAmount"] = res["taxableAmount"];
                single["gstPct"] = res["gstPct"];
                single["totalAmount"] = res["totalAmount"];
                itemsList.append(single);
            }
            res["items"] = itemsList;
            return res;
        }
    } else { // "Sale"
        QVariantList rows = DatabaseManager::instance().executeQuery(
            "SELECT id, invoice_no, invoice_date, customer_name, gstin, state_code, is_interstate, "
            "       gst_pct, item_name, hsn_code, bag_count, weight_qtl, rate_per_qtl, taxable_amount, total_amount "
            "FROM sales_invoices WHERE invoice_no = ? LIMIT 1;",
            {qInvNo}
        );

        if (!rows.isEmpty()) {
            auto map = rows.first().toMap();
            res["found"] = true;
            res["invoiceId"] = map.value("id").toInt();
            res["invoiceNo"] = map.value("invoice_no").toString();
            res["invoiceDate"] = map.value("invoice_date").toString();
            res["partyName"] = map.value("customer_name").toString();
            res["partyGstin"] = map.value("gstin").toString();
            res["stateCode"] = map.value("state_code").toString();
            res["isInterstate"] = map.value("is_interstate").toInt() == 1;
            res["gstPct"] = map.value("gst_pct", 5.0).toDouble();
            res["itemName"] = map.value("item_name").toString();
            res["hsnCode"] = map.value("hsn_code").toString();
            res["bagCount"] = map.value("bag_count").toInt();
            res["weightQtl"] = map.value("weight_qtl").toDouble();
            res["rate"] = map.value("rate_per_qtl").toDouble();
            res["taxableAmount"] = map.value("taxable_amount").toDouble();
            res["totalAmount"] = map.value("total_amount").toDouble();

            // Check if line items exist in sales_invoice_items
            QVariantList itemRows = DatabaseManager::instance().executeQuery(
                "SELECT item_name, hsn_code, unit, bag_count, weight_qtl, rate_per_qtl, taxable_amount, gst_pct, total_amount "
                "FROM sales_invoice_items WHERE invoice_id = ?;",
                {map.value("id").toInt()}
            );

            QVariantList itemsList;
            if (!itemRows.isEmpty()) {
                for (const auto &it : itemRows) {
                    auto itMap = it.toMap();
                    QVariantMap line;
                    line["itemName"] = itMap.value("item_name").toString();
                    line["hsnCode"] = itMap.value("hsn_code").toString();
                    line["unit"] = itMap.value("unit", "QTL").toString();
                    line["bags"] = itMap.value("bag_count").toInt();
                    line["weightQtl"] = itMap.value("weight_qtl").toDouble();
                    line["rate"] = itMap.value("rate_per_qtl").toDouble();
                    line["taxableAmount"] = itMap.value("taxable_amount").toDouble();
                    line["gstPct"] = itMap.value("gst_pct", 5.0).toDouble();
                    line["totalAmount"] = itMap.value("total_amount").toDouble();
                    itemsList.append(line);
                }
            } else if (!res["itemName"].toString().isEmpty()) {
                QVariantMap single;
                single["itemName"] = res["itemName"];
                single["hsnCode"] = res["hsnCode"];
                single["unit"] = "QTL";
                single["bags"] = res["bagCount"];
                single["weightQtl"] = res["weightQtl"];
                single["rate"] = res["rate"];
                single["taxableAmount"] = res["taxableAmount"];
                single["gstPct"] = res["gstPct"];
                single["totalAmount"] = res["totalAmount"];
                itemsList.append(single);
            }
            res["items"] = itemsList;
            return res;
        }
    }

    res["found"] = false;
    return res;
}

QVariantMap DebitCreditNoteController::calculateTotals(const QVariantList &items, double gstPct, bool isInterstate) {
    double taxable = 0.0;
    int totalBags = 0;
    double totalWeight = 0.0;

    for (const auto &it : items) {
        auto map = it.toMap();
        int bags = map.value("bags", 0).toInt();
        double wt = map.value("weightQtl", 0.0).toDouble();
        double amt = map.value("taxableAmount", 0.0).toDouble();

        if (amt <= 0.0) {
            double rate = map.value("rate", 0.0).toDouble();
            if (wt > 0.0) amt = wt * rate;
            else if (bags > 0) amt = bags * rate;
        }

        taxable += amt;
        totalBags += bags;
        totalWeight += wt;
    }

    taxable = FinancialMathService::round2(taxable);
    totalWeight = FinancialMathService::round2(totalWeight);

    auto &math = FinancialMathService::instance();
    QVariantMap gst = math.calculateGst(taxable, gstPct, isInterstate);
    double cgst = gst.value("cgst").toDouble();
    double sgst = gst.value("sgst").toDouble();
    double igst = gst.value("igst").toDouble();
    double totalTax = gst.value("totalTax").toDouble();

    double rawTotal = taxable + totalTax;
    double roundOff = math.calculateRoundOff(rawTotal);
    double grandTotal = FinancialMathService::round2(rawTotal + roundOff);

    QVariantMap res;
    res["totalBags"] = totalBags;
    res["totalWeightQtl"] = totalWeight;
    res["taxableAmount"] = taxable;
    res["gstPct"] = gstPct;
    res["cgstAmount"] = cgst;
    res["sgstAmount"] = sgst;
    res["igstAmount"] = igst;
    res["totalTaxAmount"] = totalTax;
    res["roundOff"] = roundOff;
    res["grandTotal"] = grandTotal;
    res["taxableAmountFmt"] = math.formatInr(taxable);
    res["cgstAmountFmt"] = math.formatInr(cgst);
    res["sgstAmountFmt"] = math.formatInr(sgst);
    res["igstAmountFmt"] = math.formatInr(igst);
    res["totalTaxAmountFmt"] = math.formatInr(totalTax);
    res["grandTotalFmt"] = math.formatInr(grandTotal);
    return res;
}

QString DebitCreditNoteController::getNextNoteNo(const QString &noteType) {
    QString prefix = (noteType == "Debit Note") ? "DN" : "CN";
    QVariant maxVal = DatabaseManager::instance().executeScalar(
        "SELECT MAX(id) FROM debit_credit_notes WHERE note_type = ?;",
        {noteType}
    );
    int nextId = maxVal.isValid() ? (maxVal.toInt() + 1) : 1;
    return QString("%1-%2").arg(prefix).arg(nextId, 4, 10, QChar('0'));
}

QVariantList DebitCreditNoteController::searchInvoices(const QString &invType, const QString &query) {
    QString q = "%" + query.trimmed() + "%";
    if (invType == "Purchase") {
        return DatabaseManager::instance().executeQuery(
            "SELECT id, invoice_no, invoice_date, supplier_name AS party_name, total_amount "
            "FROM purchase_invoices "
            "WHERE invoice_no LIKE ? OR supplier_name LIKE ? "
            "ORDER BY id DESC LIMIT 15;",
            {q, q}
        );
    } else {
        return DatabaseManager::instance().executeQuery(
            "SELECT id, invoice_no, invoice_date, customer_name AS party_name, total_amount "
            "FROM sales_invoices "
            "WHERE invoice_no LIKE ? OR customer_name LIKE ? "
            "ORDER BY id DESC LIMIT 15;",
            {q, q}
        );
    }
}

QVariantMap DebitCreditNoteController::saveNote(const QVariantMap &data) {
    QVariantMap response;
    int id = data.value("id", 0).toInt();

    QString noteType = data.value("noteType", "Credit Note").toString().trimmed();
    QString noteNo = data.value("noteNo").toString().trimmed();
    if (noteNo.isEmpty()) {
        noteNo = getNextNoteNo(noteType);
    }

    QString noteDate = data.value("noteDate").toString().trimmed();
    if (noteDate.isEmpty()) {
        noteDate = QDate::currentDate().toString("yyyy-MM-dd");
    }

    QString noteTime = data.value("noteTime").toString().trimmed();
    if (noteTime.isEmpty()) {
        noteTime = QTime::currentTime().toString("hh:mm AP");
    }

    QString origInvNo = data.value("originalInvoiceNo").toString().trimmed();
    QString origInvDate = data.value("originalInvoiceDate").toString().trimmed();
    QString origInvType = data.value("originalInvoiceType", "Sale").toString().trimmed();
    int origInvId = data.value("originalInvoiceId", 0).toInt();

    QString partyName = data.value("partyName").toString().trimmed();
    QString partyGstin = data.value("partyGstin").toString().trimmed().toUpper();
    QString stateCode = data.value("stateCode").toString().trimmed();
    bool isInterstate = data.value("isInterstate", false).toBool();
    QString reasonCode = data.value("reasonCode", "01-Sales Return").toString().trimmed();
    QString adjType = data.value("adjustmentType", "Sales Return").toString().trimmed();
    QString narration = data.value("narration").toString().trimmed();

    if (partyName.isEmpty()) {
        response["success"] = false;
        response["message"] = "Party / Customer / Supplier name is required.";
        return response;
    }

    QVariantList itemsList = data.value("items").toList();
    double gstPct = data.value("gstPct", 5.0).toDouble();

    QVariantMap totals = calculateTotals(itemsList, gstPct, isInterstate);
    int totalBags = totals.value("totalBags").toInt();
    double totalWeight = totals.value("totalWeightQtl").toDouble();
    double taxableAmount = totals.value("taxableAmount").toDouble();
    double cgst = totals.value("cgstAmount").toDouble();
    double sgst = totals.value("sgstAmount").toDouble();
    double igst = totals.value("igstAmount").toDouble();
    double totalTax = totals.value("totalTaxAmount").toDouble();
    double roundOff = totals.value("roundOff").toDouble();
    double grandTotal = totals.value("grandTotal").toDouble();

    QString primaryItem;
    QString primaryHsn;
    if (!itemsList.isEmpty()) {
        auto firstIt = itemsList.first().toMap();
        primaryItem = firstIt.value("itemName").toString();
        primaryHsn = firstIt.value("hsnCode").toString();
    }

    // Resolve Party ID
    QVariant partyIdVal = QVariant();
    QVariant pid = DatabaseManager::instance().executeScalar(
        "SELECT id FROM parties WHERE party_name = ? COLLATE NOCASE;", {partyName}
    );
    if (pid.isValid() && pid.toInt() > 0) {
        partyIdVal = pid.toInt();
    }

    if (id <= 0) {
        // Insert Note
        bool ok = DatabaseManager::instance().executeNonQuery(
            "INSERT INTO debit_credit_notes ("
            "  note_type, note_no, note_date, note_time, original_invoice_id, "
            "  original_invoice_no, original_invoice_date, original_invoice_type, "
            "  party_id, party_name, party_gstin, state_code, is_interstate, "
            "  reason_code, adjustment_type, item_name, hsn_code, total_bags, "
            "  total_weight_qtl, taxable_amount, gst_pct, cgst_amount, sgst_amount, "
            "  igst_amount, total_tax_amount, round_off, grand_total, narration"
            ") VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);",
            {
                noteType, noteNo, noteDate, noteTime, origInvId,
                origInvNo, origInvDate, origInvType,
                partyIdVal, partyName, partyGstin, stateCode, isInterstate ? 1 : 0,
                reasonCode, adjType, primaryItem, primaryHsn, totalBags,
                totalWeight, taxableAmount, gstPct, cgst, sgst,
                igst, totalTax, roundOff, grandTotal, narration
            }
        );

        if (!ok) {
            response["success"] = false;
            response["message"] = "Failed to insert Debit/Credit Note into database.";
            return response;
        }

        QVariant lastId = DatabaseManager::instance().executeScalar("SELECT last_insert_rowid();");
        id = lastId.toInt();
    } else {
        // Update Note
        bool ok = DatabaseManager::instance().executeNonQuery(
            "UPDATE debit_credit_notes SET "
            "  note_type = ?, note_no = ?, note_date = ?, note_time = ?, "
            "  original_invoice_id = ?, original_invoice_no = ?, original_invoice_date = ?, original_invoice_type = ?, "
            "  party_id = ?, party_name = ?, party_gstin = ?, state_code = ?, is_interstate = ?, "
            "  reason_code = ?, adjustment_type = ?, item_name = ?, hsn_code = ?, total_bags = ?, "
            "  total_weight_qtl = ?, taxable_amount = ?, gst_pct = ?, cgst_amount = ?, sgst_amount = ?, "
            "  igst_amount = ?, total_tax_amount = ?, round_off = ?, grand_total = ?, narration = ? "
            "WHERE id = ?;",
            {
                noteType, noteNo, noteDate, noteTime,
                origInvId, origInvNo, origInvDate, origInvType,
                partyIdVal, partyName, partyGstin, stateCode, isInterstate ? 1 : 0,
                reasonCode, adjType, primaryItem, primaryHsn, totalBags,
                totalWeight, taxableAmount, gstPct, cgst, sgst,
                igst, totalTax, roundOff, grandTotal, narration,
                id
            }
        );

        if (!ok) {
            response["success"] = false;
            response["message"] = "Failed to update Debit/Credit Note in database.";
            return response;
        }

        // Delete previous items
        DatabaseManager::instance().executeNonQuery("DELETE FROM debit_credit_note_items WHERE note_id = ?;", {id});
    }

    // Insert Line Items
    for (const auto &it : itemsList) {
        auto itMap = it.toMap();
        QString itName = itMap.value("itemName").toString();
        QString hsn = itMap.value("hsnCode").toString();
        QString unit = itMap.value("unit", "QTL").toString();
        int bags = itMap.value("bags", 0).toInt();
        double wt = itMap.value("weightQtl", 0.0).toDouble();
        double rate = itMap.value("rate", 0.0).toDouble();
        double amt = itMap.value("taxableAmount", 0.0).toDouble();
        if (amt <= 0.0) {
            if (wt > 0.0) amt = wt * rate;
            else if (bags > 0) amt = bags * rate;
        }
        double itemGst = itMap.value("gstPct", gstPct).toDouble();
        double tot = itMap.value("totalAmount", amt * (1.0 + itemGst / 100.0)).toDouble();

        QVariant itemIdVal = QVariant();
        if (!itName.isEmpty()) {
            QVariant iid = DatabaseManager::instance().executeScalar(
                "SELECT id FROM stock_items WHERE item_name = ? COLLATE NOCASE;", {itName}
            );
            if (iid.isValid() && iid.toInt() > 0) {
                itemIdVal = iid.toInt();
            }
        }

        DatabaseManager::instance().executeNonQuery(
            "INSERT INTO debit_credit_note_items ("
            "  note_id, item_id, item_name, hsn_code, unit, bags, weight_qtl, rate, taxable_amount, gst_pct, total_amount"
            ") VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);",
            {
                id, itemIdVal, itName, hsn, unit, bags, wt, rate, amt, itemGst, tot
            }
        );
    }

    // Post Double-Entry Vouchers
    DebitCreditNoteRecord noteRec;
    noteRec.id = id;
    noteRec.noteType = noteType;
    noteRec.noteNo = noteNo;
    noteRec.noteDate = noteDate;
    noteRec.originalInvoiceNo = origInvNo;
    noteRec.partyName = partyName;
    noteRec.taxableAmount = taxableAmount;
    noteRec.cgstAmount = cgst;
    noteRec.sgstAmount = sgst;
    noteRec.igstAmount = igst;
    noteRec.grandTotal = grandTotal;
    noteRec.narration = narration;
    noteRec.isInterstate = isInterstate;
    noteRec.adjustmentType = adjType;
    postDoubleEntryVoucher(noteRec);

    reload();
    emit noteSaved(id, noteNo);

    response["success"] = true;
    response["message"] = QString("%1 %2 saved successfully.").arg(noteType, noteNo);
    response["id"] = id;
    response["noteNo"] = noteNo;
    return response;
}

void DebitCreditNoteController::deleteDoubleEntryVouchers(int noteId) {
    Q_UNUSED(noteId);
    // Vouchers associated with note are cleaned up in postDoubleEntryVoucher by reference no
}

void DebitCreditNoteController::postDoubleEntryVoucher(const DebitCreditNoteRecord &note) {
    // 1. Delete prior double entry postings for this note
    DatabaseManager::instance().executeNonQuery(
        "DELETE FROM vouchers WHERE voucher_type IN ('Credit Note', 'Debit Note') AND (voucher_no = ? OR narration LIKE ?);",
        {note.noteNo, QString("%%1%").arg(note.noteNo)}
    );

    QString vchDate = note.noteDate;
    QString refNo = note.noteNo;
    QString baseNarr = QString("%1 %2 Ref Inv: %3 (%4)").arg(note.noteType, note.noteNo, note.originalInvoiceNo, note.adjustmentType);
    if (!note.narration.isEmpty()) {
        baseNarr += " - " + note.narration;
    }

    if (note.noteType == "Credit Note") {
        // CREDIT NOTE (Sales Return / Quality Deduction / Rebate)
        // 1. Credit Party (Customer) with Grand Total (reduces receivable)
        DatabaseManager::instance().executeNonQuery(
            "INSERT INTO vouchers (voucher_type, voucher_no, voucher_date, party_name, account_type, amount, narration) "
            "VALUES (?, ?, ?, ?, 'Cr', ?, ?);",
            {"Credit Note", refNo, vchDate, note.partyName, note.grandTotal, baseNarr}
        );

        // 2. Debit Sales Return / Deduction Account with Taxable Amount
        QString salesReturnAcct = (note.adjustmentType.contains("Rate Cut", Qt::CaseInsensitive))
                                      ? "Quality Deduction / Rate Cut on Sales"
                                      : "Sales Return Account";
        DatabaseManager::instance().executeNonQuery(
            "INSERT INTO vouchers (voucher_type, voucher_no, voucher_date, party_name, account_type, amount, narration) "
            "VALUES (?, ?, ?, ?, 'Dr', ?, ?);",
            {"Credit Note", refNo, vchDate, salesReturnAcct, note.taxableAmount, baseNarr}
        );

        // 3. Debit Output GST (reversing output liability)
        if (note.isInterstate && note.igstAmount > 0.0) {
            DatabaseManager::instance().executeNonQuery(
                "INSERT INTO vouchers (voucher_type, voucher_no, voucher_date, party_name, account_type, amount, narration) "
                "VALUES (?, ?, ?, 'Output IGST Account', 'Dr', ?, ?);",
                {"Credit Note", refNo, vchDate, note.igstAmount, baseNarr}
            );
        } else {
            if (note.cgstAmount > 0.0) {
                DatabaseManager::instance().executeNonQuery(
                    "INSERT INTO vouchers (voucher_type, voucher_no, voucher_date, party_name, account_type, amount, narration) "
                    "VALUES (?, ?, ?, 'Output CGST Account', 'Dr', ?, ?);",
                    {"Credit Note", refNo, vchDate, note.cgstAmount, baseNarr}
                );
            }
            if (note.sgstAmount > 0.0) {
                DatabaseManager::instance().executeNonQuery(
                    "INSERT INTO vouchers (voucher_type, voucher_no, voucher_date, party_name, account_type, amount, narration) "
                    "VALUES (?, ?, ?, 'Output SGST Account', 'Dr', ?, ?);",
                    {"Credit Note", refNo, vchDate, note.sgstAmount, baseNarr}
                );
            }
        }
    } else {
        // DEBIT NOTE (Purchase Return / Purchase Rate Cut)
        // 1. Debit Party (Supplier) with Grand Total (reduces payable)
        DatabaseManager::instance().executeNonQuery(
            "INSERT INTO vouchers (voucher_type, voucher_no, voucher_date, party_name, account_type, amount, narration) "
            "VALUES (?, ?, ?, ?, 'Dr', ?, ?);",
            {"Debit Note", refNo, vchDate, note.partyName, note.grandTotal, baseNarr}
        );

        // 2. Credit Purchase Return / Rate Cut Account with Taxable Amount
        QString purReturnAcct = (note.adjustmentType.contains("Rate Cut", Qt::CaseInsensitive))
                                    ? "Quality Deduction / Purchase Rate Cut"
                                    : "Purchase Return Account";
        DatabaseManager::instance().executeNonQuery(
            "INSERT INTO vouchers (voucher_type, voucher_no, voucher_date, party_name, account_type, amount, narration) "
            "VALUES (?, ?, ?, ?, 'Cr', ?, ?);",
            {"Debit Note", refNo, vchDate, purReturnAcct, note.taxableAmount, baseNarr}
        );

        // 3. Credit Input GST ITC (reversing ITC)
        if (note.isInterstate && note.igstAmount > 0.0) {
            DatabaseManager::instance().executeNonQuery(
                "INSERT INTO vouchers (voucher_type, voucher_no, voucher_date, party_name, account_type, amount, narration) "
                "VALUES (?, ?, ?, 'Input IGST ITC Account', 'Cr', ?, ?);",
                {"Debit Note", refNo, vchDate, note.igstAmount, baseNarr}
            );
        } else {
            if (note.cgstAmount > 0.0) {
                DatabaseManager::instance().executeNonQuery(
                    "INSERT INTO vouchers (voucher_type, voucher_no, voucher_date, party_name, account_type, amount, narration) "
                    "VALUES (?, ?, ?, 'Input CGST ITC Account', 'Cr', ?, ?);",
                    {"Debit Note", refNo, vchDate, note.cgstAmount, baseNarr}
                );
            }
            if (note.sgstAmount > 0.0) {
                DatabaseManager::instance().executeNonQuery(
                    "INSERT INTO vouchers (voucher_type, voucher_no, voucher_date, party_name, account_type, amount, narration) "
                    "VALUES (?, ?, ?, 'Input SGST ITC Account', 'Cr', ?, ?);",
                    {"Debit Note", refNo, vchDate, note.sgstAmount, baseNarr}
                );
            }
        }
    }
}

bool DebitCreditNoteController::deleteNote(int id) {
    QVariant noteNoVal = DatabaseManager::instance().executeScalar(
        "SELECT note_no FROM debit_credit_notes WHERE id = ?;", {id}
    );

    if (noteNoVal.isValid()) {
        QString nNo = noteNoVal.toString();
        DatabaseManager::instance().executeNonQuery(
            "DELETE FROM vouchers WHERE voucher_type IN ('Credit Note', 'Debit Note') AND voucher_no = ?;",
            {nNo}
        );
    }

    bool ok = DatabaseManager::instance().executeNonQuery(
        "DELETE FROM debit_credit_notes WHERE id = ?;", {id}
    );

    if (ok) {
        reload();
        emit noteDeleted(id);
    }
    return ok;
}

QVariantMap DebitCreditNoteController::getNote(int id) {
    QVariantList rows = DatabaseManager::instance().executeQuery(
        "SELECT * FROM debit_credit_notes WHERE id = ? LIMIT 1;", {id}
    );
    if (rows.isEmpty()) return QVariantMap();

    auto raw = rows.first().toMap();
    auto &math = FinancialMathService::instance();
    QVariantMap map = raw;
    map["id"] = raw.value("id").toInt();
    map["noteId"] = raw.value("id").toInt();
    map["noteType"] = raw.value("note_type").toString();
    map["noteNo"] = raw.value("note_no").toString();
    map["noteDate"] = raw.value("note_date").toString();
    map["noteTime"] = raw.value("note_time").toString();
    map["originalInvoiceId"] = raw.value("original_invoice_id").toInt();
    map["originalInvoiceNo"] = raw.value("original_invoice_no").toString();
    map["originalInvoiceDate"] = raw.value("original_invoice_date").toString();
    map["originalInvoiceType"] = raw.value("original_invoice_type").toString();
    map["partyName"] = raw.value("party_name").toString();
    map["partyGstin"] = raw.value("party_gstin").toString();
    map["stateCode"] = raw.value("state_code").toString();
    map["isInterstate"] = raw.value("is_interstate").toInt() == 1;
    map["reasonCode"] = raw.value("reason_code").toString();
    map["adjustmentType"] = raw.value("adjustment_type").toString();
    map["itemName"] = raw.value("item_name").toString();
    map["hsnCode"] = raw.value("hsn_code").toString();
    map["totalBags"] = raw.value("total_bags").toInt();
    map["totalWeightQtl"] = raw.value("total_weight_qtl").toDouble();
    map["taxableAmount"] = raw.value("taxable_amount").toDouble();
    map["gstPct"] = raw.value("gst_pct").toDouble();
    map["cgstAmount"] = raw.value("cgst_amount").toDouble();
    map["sgstAmount"] = raw.value("sgst_amount").toDouble();
    map["igstAmount"] = raw.value("igst_amount").toDouble();
    map["totalTaxAmount"] = raw.value("total_tax_amount").toDouble();
    map["roundOff"] = raw.value("round_off").toDouble();
    map["grandTotal"] = raw.value("grand_total").toDouble();
    map["narration"] = raw.value("narration").toString();
    map["taxableAmountFmt"] = math.formatInr(raw.value("taxable_amount").toDouble());
    map["totalTaxAmountFmt"] = math.formatInr(raw.value("total_tax_amount").toDouble());
    map["grandTotalFmt"] = math.formatInr(raw.value("grand_total").toDouble());

    // Fetch Line Items
    QVariantList itemRows = DatabaseManager::instance().executeQuery(
        "SELECT item_name, hsn_code, unit, bags, weight_qtl, rate, taxable_amount, gst_pct, total_amount "
        "FROM debit_credit_note_items WHERE note_id = ? ORDER BY id ASC;",
        {id}
    );

    QVariantList itemsList;
    for (const auto &it : itemRows) {
        auto itMap = it.toMap();
        QVariantMap line;
        line["itemName"] = itMap.value("item_name").toString();
        line["hsnCode"] = itMap.value("hsn_code").toString();
        line["unit"] = itMap.value("unit", "QTL").toString();
        line["bags"] = itMap.value("bags").toInt();
        line["weightQtl"] = itMap.value("weight_qtl").toDouble();
        line["rate"] = itMap.value("rate").toDouble();
        line["taxableAmount"] = itMap.value("taxable_amount").toDouble();
        line["gstPct"] = itMap.value("gst_pct").toDouble();
        line["totalAmount"] = itMap.value("total_amount").toDouble();
        itemsList.append(line);
    }
    map["items"] = itemsList;

    return map;
}

void DebitCreditNoteController::setDraftId(int val) {
    if (m_draftId != val) {
        m_draftId = val;
        emit draftChanged();
    }
}

void DebitCreditNoteController::setDraftNoteType(const QString &val) {
    if (m_draftNoteType != val) {
        m_draftNoteType = val;
        if (m_draftId <= 0) {
            m_draftNoteNo = getNextNoteNo(m_draftNoteType);
        }
        emit draftChanged();
    }
}

void DebitCreditNoteController::setDraftNoteNo(const QString &val) {
    if (m_draftNoteNo != val) {
        m_draftNoteNo = val;
        emit draftChanged();
    }
}

void DebitCreditNoteController::setDraftNoteDate(const QString &val) {
    if (m_draftNoteDate != val) {
        m_draftNoteDate = val;
        emit draftChanged();
    }
}

void DebitCreditNoteController::setDraftNoteTime(const QString &val) {
    if (m_draftNoteTime != val) {
        m_draftNoteTime = val;
        emit draftChanged();
    }
}

void DebitCreditNoteController::setDraftOrigInvNo(const QString &val) {
    if (m_draftOrigInvNo != val) {
        m_draftOrigInvNo = val;
        emit draftChanged();
    }
}

void DebitCreditNoteController::setDraftOrigInvDate(const QString &val) {
    if (m_draftOrigInvDate != val) {
        m_draftOrigInvDate = val;
        emit draftChanged();
    }
}

void DebitCreditNoteController::setDraftOrigInvType(const QString &val) {
    if (m_draftOrigInvType != val) {
        m_draftOrigInvType = val;
        emit draftChanged();
    }
}

void DebitCreditNoteController::setDraftPartyName(const QString &val) {
    if (m_draftPartyName != val) {
        m_draftPartyName = val;
        emit draftChanged();
    }
}

void DebitCreditNoteController::setDraftPartyGstin(const QString &val) {
    if (m_draftPartyGstin != val) {
        m_draftPartyGstin = val;
        emit draftChanged();
    }
}

void DebitCreditNoteController::setDraftIsInterstate(bool val) {
    if (m_draftIsInterstate != val) {
        m_draftIsInterstate = val;
        emit draftChanged();
        recalculateFormTotals();
    }
}

void DebitCreditNoteController::setDraftGstPct(double val) {
    if (std::abs(m_draftGstPct - val) > 0.001) {
        m_draftGstPct = val;
        emit draftChanged();
        recalculateFormTotals();
    }
}

void DebitCreditNoteController::setDraftReasonCode(const QString &val) {
    if (m_draftReasonCode != val) {
        m_draftReasonCode = val;
        emit draftChanged();
    }
}

void DebitCreditNoteController::setDraftAdjType(const QString &val) {
    if (m_draftAdjType != val) {
        m_draftAdjType = val;
        emit draftChanged();
    }
}

void DebitCreditNoteController::setDraftNarration(const QString &val) {
    if (m_draftNarration != val) {
        m_draftNarration = val;
        emit draftChanged();
    }
}

QString DebitCreditNoteController::currentDateIso() const {
    return QDate::currentDate().toString("yyyy-MM-dd");
}

QString DebitCreditNoteController::currentTimeIso() const {
    return QTime::currentTime().toString("hh:mm");
}

void DebitCreditNoteController::resetDraft() {
    m_draftId = 0;
    m_draftNoteType = "Credit Note";
    m_draftNoteNo = getNextNoteNo(m_draftNoteType);
    m_draftNoteDate = currentDateIso();
    m_draftNoteTime = currentTimeIso();
    m_draftOrigInvNo.clear();
    m_draftOrigInvDate = m_draftNoteDate;
    m_draftOrigInvType = "Sale";
    m_draftOrigInvId = 0;
    m_draftPartyName.clear();
    m_draftPartyGstin.clear();
    m_draftStateCode.clear();
    m_draftIsInterstate = false;
    m_draftGstPct = 5.0;
    m_draftReasonCode = "01-Sales Return";
    m_draftAdjType = "Sales Return";
    m_draftNarration.clear();

    m_itemsModel.clear();
    m_itemsModel.addItem("Basmati Rice", "10063020", 0, 0.0, 0.0, 0.0, 5.0);
    emit draftChanged();
    recalculateFormTotals();
}

void DebitCreditNoteController::recalculateFormTotals() {
    QVariantMap totals = calculateTotals(m_itemsModel.toVariantList(), m_draftGstPct, m_draftIsInterstate);
    m_formTotalBags = totals.value("totalBags").toInt();
    m_formTotalWeightQtl = totals.value("totalWeightQtl").toDouble();
    m_formTaxableAmount = totals.value("taxableAmount").toDouble();
    m_formCgstAmount = totals.value("cgstAmount").toDouble();
    m_formSgstAmount = totals.value("sgstAmount").toDouble();
    m_formIgstAmount = totals.value("igstAmount").toDouble();
    m_formTotalTaxAmount = totals.value("totalTaxAmount").toDouble();
    m_formRoundOff = totals.value("roundOff").toDouble();
    m_formGrandTotal = totals.value("grandTotal").toDouble();
    emit formTotalsChanged();
}

QString DebitCreditNoteController::formTaxableAmountFmt() const { return FinancialMathService::instance().formatInr(m_formTaxableAmount); }
QString DebitCreditNoteController::formCgstAmountFmt() const { return FinancialMathService::instance().formatInr(m_formCgstAmount); }
QString DebitCreditNoteController::formSgstAmountFmt() const { return FinancialMathService::instance().formatInr(m_formSgstAmount); }
QString DebitCreditNoteController::formIgstAmountFmt() const { return FinancialMathService::instance().formatInr(m_formIgstAmount); }
QString DebitCreditNoteController::formTotalTaxAmountFmt() const { return FinancialMathService::instance().formatInr(m_formTotalTaxAmount); }
QString DebitCreditNoteController::formRoundOffFmt() const { return FinancialMathService::instance().formatInr(m_formRoundOff); }
QString DebitCreditNoteController::formGrandTotalFmt() const { return FinancialMathService::instance().formatInr(m_formGrandTotal); }

bool DebitCreditNoteController::loadNoteIntoDraft(int id) {
    if (id <= 0) return false;
    QVariantMap data = getNote(id);
    if (!data.contains("noteNo") || data.value("noteNo").toString().isEmpty()) return false;

    m_draftId = id;
    m_draftNoteType = data.value("noteType", "Credit Note").toString();
    m_draftNoteNo = data.value("noteNo").toString();
    m_draftNoteDate = data.value("noteDate").toString();
    m_draftNoteTime = data.value("noteTime").toString();
    m_draftOrigInvNo = data.value("originalInvoiceNo").toString();
    m_draftOrigInvDate = data.value("originalInvoiceDate").toString();
    m_draftOrigInvType = data.value("originalInvoiceType", "Sale").toString();
    m_draftPartyName = data.value("partyName").toString();
    m_draftPartyGstin = data.value("partyGstin").toString();
    m_draftIsInterstate = data.value("isInterstate", false).toBool();
    m_draftGstPct = data.value("gstPct", 5.0).toDouble();
    m_draftReasonCode = data.value("reasonCode", "01-Sales Return").toString();
    m_draftAdjType = data.value("adjustmentType", "Sales Return").toString();
    m_draftNarration = data.value("narration").toString();

    QVector<DebitCreditNoteItem> items;
    QVariantList itList = data.value("items").toList();
    if (!itList.isEmpty()) {
        for (const auto &v : itList) {
            auto m = v.toMap();
            DebitCreditNoteItem it;
            it.itemName = m.value("itemName").toString();
            it.hsnCode = m.value("hsnCode").toString();
            it.unit = m.value("unit", "QTL").toString();
            it.bags = m.value("bags", 0).toInt();
            it.weightQtl = m.value("weightQtl", 0.0).toDouble();
            it.rate = m.value("rate", 0.0).toDouble();
            it.taxableAmount = m.value("taxableAmount", 0.0).toDouble();
            it.gstPct = m.value("gstPct", 5.0).toDouble();
            items.append(it);
        }
    } else {
        DebitCreditNoteItem it;
        it.itemName = data.value("itemName").toString();
        it.hsnCode = data.value("hsnCode").toString();
        it.unit = "QTL";
        it.bags = data.value("totalBags", 0).toInt();
        it.weightQtl = data.value("totalWeightQtl", 0.0).toDouble();
        it.taxableAmount = data.value("taxableAmount", 0.0).toDouble();
        it.gstPct = data.value("gstPct", 5.0).toDouble();
        items.append(it);
    }
    m_itemsModel.setItems(items);
    emit draftChanged();
    recalculateFormTotals();
    return true;
}

bool DebitCreditNoteController::fetchAndLoadOriginalInvoice(const QString &invType, const QString &invNo) {
    QVariantMap res = fetchOriginalInvoice(invType, invNo);
    if (!res.value("found", false).toBool()) return false;

    m_draftOrigInvNo = res.value("invoiceNo").toString();
    m_draftOrigInvDate = res.value("invoiceDate").toString();
    m_draftPartyName = res.value("partyName").toString();
    m_draftPartyGstin = res.value("partyGstin").toString();
    m_draftStateCode = res.value("stateCode").toString();
    m_draftIsInterstate = res.value("isInterstate", false).toBool();
    m_draftGstPct = res.value("gstPct", 5.0).toDouble();

    QVariantList itemsList = res.value("items").toList();
    if (!itemsList.isEmpty()) {
        QVector<DebitCreditNoteItem> items;
        for (const auto &v : itemsList) {
            auto m = v.toMap();
            DebitCreditNoteItem it;
            it.itemName = m.value("itemName").toString();
            it.hsnCode = m.value("hsnCode").toString();
            it.unit = m.value("unit", "QTL").toString();
            it.bags = m.value("bags", 0).toInt();
            it.weightQtl = m.value("weightQtl", 0.0).toDouble();
            it.rate = m.value("rate", 0.0).toDouble();
            it.taxableAmount = m.value("taxableAmount", 0.0).toDouble();
            it.gstPct = m.value("gstPct", 5.0).toDouble();
            items.append(it);
        }
        m_itemsModel.setItems(items);
    }
    emit draftChanged();
    recalculateFormTotals();
    return true;
}

bool DebitCreditNoteController::saveCurrentDraft() {
    QVariantMap payload;
    payload["id"] = m_draftId;
    payload["noteType"] = m_draftNoteType;
    payload["noteNo"] = m_draftNoteNo;
    payload["noteDate"] = m_draftNoteDate;
    payload["noteTime"] = m_draftNoteTime;
    payload["originalInvoiceNo"] = m_draftOrigInvNo;
    payload["originalInvoiceDate"] = m_draftOrigInvDate;
    payload["originalInvoiceType"] = m_draftOrigInvType;
    payload["partyName"] = m_draftPartyName;
    payload["partyGstin"] = m_draftPartyGstin;
    payload["isInterstate"] = m_draftIsInterstate;
    payload["reasonCode"] = m_draftReasonCode;
    payload["adjustmentType"] = m_draftAdjType;
    payload["gstPct"] = m_draftGstPct;
    payload["narration"] = m_draftNarration;
    payload["items"] = m_itemsModel.toVariantList();

    QVariantMap res = saveNote(payload);
    return res.value("success", false).toBool();
}

