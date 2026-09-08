#include "purchase_register_model.h"
#include "../database_manager.h"
#include "../engine/accounting_engine.h"
#include <QDate>
#include <cmath>

static QString formatINR(double val) {
    if (std::abs(val) < 0.001) return "₹0.00";
    bool neg = val < 0;
    double absVal = std::abs(val);
    long long integerPart = static_cast<long long>(absVal);
    int decimalPart = static_cast<int>(std::round((absVal - integerPart) * 100.0));
    if (decimalPart >= 100) {
        integerPart += 1;
        decimalPart = 0;
    }

    QString s = QString::number(integerPart);
    QString res = "";
    int n = s.length();
    if (n <= 3) {
        res = s;
    } else {
        res = s.right(3);
        int rem = n - 3;
        while (rem > 0) {
            int take = std::min(2, rem);
            res = s.mid(rem - take, take) + "," + res;
            rem -= take;
        }
    }
    QString decStr = QString::number(decimalPart);
    if (decStr.length() == 1) decStr = "0" + decStr;
    return (neg ? "-₹" : "₹") + res + "." + decStr;
}

static QString formatDisplayDate(const QString& iso) {
    if (iso.isEmpty()) return "";
    QStringList p = iso.split("-");
    if (p.size() == 3 && p[0].length() == 4) {
        return p[2] + "-" + p[1] + "-" + p[0];
    }
    return iso;
}

PurchaseRegisterModel::PurchaseRegisterModel(QObject* parent)
    : QAbstractListModel(parent)
{
}

int PurchaseRegisterModel::rowCount(const QModelIndex& parent) const {
    if (parent.isValid()) return 0;
    return m_entries.size();
}

QVariant PurchaseRegisterModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || index.row() < 0 || index.row() >= m_entries.size())
        return QVariant();

    const auto& item = m_entries.at(index.row());
    switch (role) {
    case IdRole: return item.id;
    case VchNoValRole: return item.vchNoVal;
    case InvNoValRole: return item.invNoVal;
    case DateValRole: return item.dateVal;
    case SuppValRole: return item.suppVal;
    case ItemValRole: return item.itemVal;
    case BagsValRole: return item.bagsVal > 0 ? QVariant(item.bagsVal) : QVariant("-");
    case WeightValRole: return item.weightVal > 0 ? QVariant(QString::number(item.weightVal, 'f', 2)) : QVariant("-");
    case RateValRole: return item.rateVal > 0 ? QVariant(QString::number(item.rateVal, 'f', 2)) : QVariant("-");
    case RateFmtRole: return item.rateFmt;
    case TaxableValRole: return item.taxableVal > 0 ? QVariant(QString::number(item.taxableVal, 'f', 2)) : QVariant("-");
    case TaxableFmtRole: return item.taxableFmt;
    case GstValRole: return item.gstVal > 0 ? QVariant(QString::number(item.gstVal, 'f', 2)) : QVariant("-");
    case GstFmtRole: return item.gstFmt;
    case TotalValRole: return QString::number(item.totalVal, 'f', 2);
    case TotalFmtRole: return item.totalFmt;
    case VehValRole: return item.vehVal.isEmpty() ? "-" : item.vehVal;
    default: return QVariant();
    }
}

QHash<int, QByteArray> PurchaseRegisterModel::roleNames() const {
    QHash<int, QByteArray> roles;
    roles[IdRole] = "id";
    roles[VchNoValRole] = "vchNoVal";
    roles[InvNoValRole] = "invNoVal";
    roles[DateValRole] = "dateVal";
    roles[SuppValRole] = "suppVal";
    roles[ItemValRole] = "itemVal";
    roles[BagsValRole] = "bagsVal";
    roles[WeightValRole] = "weightVal";
    roles[RateValRole] = "rateVal";
    roles[RateFmtRole] = "rateFmt";
    roles[TaxableValRole] = "taxableVal";
    roles[TaxableFmtRole] = "taxableFmt";
    roles[GstValRole] = "gstVal";
    roles[GstFmtRole] = "gstFmt";
    roles[TotalValRole] = "totalVal";
    roles[TotalFmtRole] = "totalFmt";
    roles[VehValRole] = "vehVal";
    return roles;
}

void PurchaseRegisterModel::setEntries(const QVector<PurchaseRegisterEntry>& entries) {
    beginResetModel();
    m_entries = entries;
    endResetModel();
    emit countChanged();
}

void PurchaseRegisterModel::clear() {
    beginResetModel();
    m_entries.clear();
    endResetModel();
    emit countChanged();
}

QVariantMap PurchaseRegisterModel::get(int row) const {
    if (row < 0 || row >= m_entries.size()) return QVariantMap();
    const auto& item = m_entries.at(row);
    QVariantMap m;
    m["id"] = item.id;
    m["vchNoVal"] = item.vchNoVal;
    m["invNoVal"] = item.invNoVal;
    m["dateVal"] = item.dateVal;
    m["suppVal"] = item.suppVal;
    m["itemVal"] = item.itemVal;
    m["bagsVal"] = item.bagsVal;
    m["weightVal"] = item.weightVal;
    m["rateVal"] = item.rateVal;
    m["rateFmt"] = item.rateFmt;
    m["taxableVal"] = item.taxableVal;
    m["taxableFmt"] = item.taxableFmt;
    m["gstVal"] = item.gstVal;
    m["gstFmt"] = item.gstFmt;
    m["totalVal"] = item.totalVal;
    m["totalFmt"] = item.totalFmt;
    m["vehVal"] = item.vehVal;
    return m;
}

PurchaseRegisterController::PurchaseRegisterController(QObject* parent)
    : QObject(parent)
{
}

void PurchaseRegisterController::setSearchQuery(const QString& query) {
    if (m_searchQuery != query) {
        m_searchQuery = query;
        emit searchQueryChanged();
        applyFilter();
    }
}

void PurchaseRegisterController::reload(const QString& fromDate, const QString& toDate) {
    QString fDate = fromDate.trimmed();
    QString tDate = toDate.trimmed();

    if (fDate.isEmpty() && tDate.isEmpty()) {
        fDate = AccountingEngine::getActiveFromDate();
        tDate = AccountingEngine::getActiveToDate();
    }

    if (fDate.isEmpty() || tDate.isEmpty()) {
        QVariantList fyRows = DatabaseManager::instance().executeQuery(
            "SELECT start_date, end_date FROM financial_years WHERE is_active = 1 LIMIT 1;"
        );
        if (!fyRows.isEmpty()) {
            QVariantMap r = fyRows.first().toMap();
            fDate = r.value("start_date").toString();
            tDate = r.value("end_date").toString();
        }
    }

    QString sql;
    QVariantList params;
    if (!fDate.isEmpty() && !tDate.isEmpty() && fDate != "ALL" && fDate != "All") {
        sql = "SELECT id, voucher_no, invoice_no, invoice_date, supplier_name, item_name, bag_count, weight_qtl, "
              "rate_per_qtl, taxable_amount, gst_pct, cgst_amount, sgst_amount, igst_amount, gst_amount, round_off, "
              "total_amount, payment_mode, vehicle_no, eway_bill_no, financial_year, narration "
              "FROM purchase_invoices "
              "WHERE invoice_date >= ? AND invoice_date <= ? "
              "ORDER BY invoice_date DESC, id DESC;";
        params << fDate << tDate;
    } else {
        sql = "SELECT id, voucher_no, invoice_no, invoice_date, supplier_name, item_name, bag_count, weight_qtl, "
              "rate_per_qtl, taxable_amount, gst_pct, cgst_amount, sgst_amount, igst_amount, gst_amount, round_off, "
              "total_amount, payment_mode, vehicle_no, eway_bill_no, financial_year, narration "
              "FROM purchase_invoices "
              "ORDER BY invoice_date DESC, id DESC;";
    }

    QVariantList rows = DatabaseManager::instance().executeQuery(sql, params);
    m_allEntries.clear();
    m_allEntries.reserve(rows.size());

    for (int i = 0; i < rows.size(); ++i) {
        QVariantMap r = rows[i].toMap();
        PurchaseRegisterEntry e;
        e.id = r.value("id").toInt();
        e.vchNoVal = r.value("voucher_no").toString();
        if (e.vchNoVal.isEmpty()) e.vchNoVal = QString("Purc-%1").arg(i + 1);
        e.invNoVal = r.value("invoice_no").toString();
        e.dateVal = formatDisplayDate(r.value("invoice_date").toString());
        e.suppVal = r.value("supplier_name").toString();
        e.itemVal = r.value("item_name").toString();
        e.bagsVal = r.value("bag_count").toLongLong();
        e.weightVal = r.value("weight_qtl").toDouble();
        e.rateVal = r.value("rate_per_qtl").toDouble();
        e.rateFmt = e.rateVal > 0 ? formatINR(e.rateVal) : "-";
        e.taxableVal = r.value("taxable_amount").toDouble();
        e.taxableFmt = e.taxableVal > 0 ? formatINR(e.taxableVal) : "-";
        e.gstVal = r.value("gst_amount").toDouble();
        if (e.gstVal <= 0.001) {
            e.gstVal = r.value("cgst_amount").toDouble() + r.value("sgst_amount").toDouble() + r.value("igst_amount").toDouble();
        }
        e.gstFmt = e.gstVal > 0 ? formatINR(e.gstVal) : "-";
        e.totalVal = r.value("total_amount").toDouble();
        e.totalFmt = formatINR(e.totalVal);
        e.vehVal = r.value("vehicle_no").toString();

        m_allEntries.append(e);
    }

    applyFilter();
}

void PurchaseRegisterController::applyFilter() {
    QString q = m_searchQuery.trimmed().toLower();
    QVector<PurchaseRegisterEntry> filtered;
    filtered.reserve(m_allEntries.size());

    m_totalInvoicesCount = 0;
    m_totalBagsCount = 0;
    m_totalWeightQtl = 0.0;
    m_totalTaxableAmt = 0.0;
    m_totalGstAmt = 0.0;
    m_totalGrossAmt = 0.0;

    for (const auto& e : m_allEntries) {
        if (!q.isEmpty()) {
            if (!e.invNoVal.toLower().contains(q) &&
                !e.suppVal.toLower().contains(q) &&
                !e.itemVal.toLower().contains(q) &&
                !e.vehVal.toLower().contains(q) &&
                !e.dateVal.toLower().contains(q) &&
                !e.vchNoVal.toLower().contains(q)) {
                continue;
            }
        }

        filtered.append(e);

        m_totalInvoicesCount++;
        m_totalBagsCount += e.bagsVal;
        m_totalWeightQtl += e.weightVal;
        m_totalTaxableAmt += e.taxableVal;
        m_totalGstAmt += e.gstVal;
        m_totalGrossAmt += e.totalVal;
    }

    m_totalWeightFmt = QString::number(m_totalWeightQtl, 'f', 2) + " Qtl";
    m_totalTaxableFmt = formatINR(m_totalTaxableAmt);
    m_totalGrossFmt = formatINR(m_totalGrossAmt);

    m_model.setEntries(filtered);
    emit totalsChanged();
}
