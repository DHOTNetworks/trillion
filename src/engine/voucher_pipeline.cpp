#include "voucher_pipeline.h"
#include "../database_manager.h"
#include "fiscal_year_helper.h"
#include <QMutexLocker>
#include <cmath>
#include <QDebug>

VoucherPipeline::VoucherPipeline(QObject* parent)
    : QObject(parent)
{
}

VoucherPipeline& VoucherPipeline::instance() {
    static VoucherPipeline s_instance;
    return s_instance;
}

QVector<UnifiedVoucherData> VoucherPipeline::queryVouchers(
    const QString& fromDateIso,
    const QString& toDateIso,
    const QStringList& voucherTypes,
    int partyId,
    int itemId,
    int limit
) {
    Q_UNUSED(itemId);
    QMutexLocker locker(&m_mutex);

    QString sql = "SELECT voucher_no, voucher_type, voucher_date, party_id, party_name, "
                  "       invoice_no, amount, dr_cr, narration "
                  "FROM transactions WHERE 1=1 ";
    QVariantList params;

    if (!fromDateIso.isEmpty()) {
        sql += " AND voucher_date >= ?";
        params << fromDateIso;
    }
    if (!toDateIso.isEmpty()) {
        sql += " AND voucher_date <= ?";
        params << toDateIso;
    }
    if (!voucherTypes.isEmpty()) {
        QStringList placeholders;
        for (const QString& t : voucherTypes) {
            placeholders << "?";
            params << t;
        }
        sql += " AND voucher_type IN (" + placeholders.join(", ") + ")";
    }
    if (partyId > 0) {
        sql += " AND party_id = ?";
        params << partyId;
    }

    sql += " ORDER BY voucher_date DESC, voucher_no DESC";
    if (limit > 0) {
        sql += QString(" LIMIT %1;").arg(limit);
    } else {
        sql += ";";
    }

    QVariantList rows = DatabaseManager::instance().executeQuery(sql, params);
    QMap<QString, UnifiedVoucherData> voucherMap;

    for (const auto& rVar : rows) {
        QVariantMap r = rVar.toMap();
        QString vNo = r.value("voucher_no").toString();
        QString vType = r.value("voucher_type").toString();
        QString vDate = r.value("voucher_date").toString();
        QString key = vType + "_" + vNo + "_" + vDate;

        if (!voucherMap.contains(key)) {
            UnifiedVoucherData v;
            v.voucherNumber = r.value("voucher_no").toInt();
            v.voucherType = vType;
            v.voucherDate = vDate;
            v.partyId = r.value("party_id").toInt();
            v.partyName = r.value("party_name").toString();
            v.invoiceNo = r.value("invoice_no").toString();
            v.narration = r.value("narration").toString();
            voucherMap[key] = v;
        }

        VoucherEntryLine entry;
        entry.partyId = r.value("party_id").toInt();
        entry.partyName = r.value("party_name").toString();
        entry.drCr = r.value("dr_cr").toString();
        entry.amount = r.value("amount").toDouble();
        entry.narration = r.value("narration").toString();
        entry.lineNumber = voucherMap[key].entries.size() + 1;

        voucherMap[key].entries.append(entry);
        if (entry.drCr.compare("Dr", Qt::CaseInsensitive) == 0) {
            voucherMap[key].totalAmount += entry.amount;
        }
    }

    QVector<UnifiedVoucherData> result;
    for (const auto& v : voucherMap) {
        result.append(v);
    }
    return result;
}

UnifiedVoucherData VoucherPipeline::getVoucherById(int voucherId) {
    Q_UNUSED(voucherId);
    return UnifiedVoucherData();
}

UnifiedVoucherData VoucherPipeline::getVoucherByNumberAndType(int voucherNumber, const QString& voucherType, int fyId) {
    Q_UNUSED(fyId);
    QVector<UnifiedVoucherData> list = queryVouchers("", "", { voucherType }, 0, 0, 500);
    for (const auto& v : list) {
        if (v.voucherNumber == voucherNumber) {
            return v;
        }
    }
    return UnifiedVoucherData();
}

int VoucherPipeline::getNextVoucherNumber(const QString& voucherType, int fyId) {
    Q_UNUSED(fyId);
    QVariant maxVal = DatabaseManager::instance().executeScalar(
        "SELECT MAX(CAST(voucher_no AS INTEGER)) FROM transactions WHERE voucher_type = ?;",
        { voucherType }
    );
    int next = maxVal.toInt() + 1;
    return (next > 0) ? next : 1;
}

VoucherPipeline::SaveResult VoucherPipeline::saveVoucher(const UnifiedVoucherData& data) {
    SaveResult res;
    if (!data.isBalanced()) {
        res.success = false;
        res.errorMessage = "Voucher is not balanced: Total Debits must strictly equal Total Credits.";
        return res;
    }

    if (data.entries.isEmpty()) {
        res.success = false;
        res.errorMessage = "Voucher contains no entry lines.";
        return res;
    }

    DatabaseManager::instance().beginTransaction();

    int vNo = data.voucherNumber;
    if (vNo <= 0) {
        vNo = getNextVoucherNumber(data.voucherType, data.financialYearId);
    }

    bool allOk = true;
    for (const auto& e : data.entries) {
        bool ok = DatabaseManager::instance().executeNonQuery(
            "INSERT INTO transactions (voucher_no, voucher_type, voucher_date, party_id, party_name, "
            "invoice_no, amount, dr_cr, narration) "
            "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?);",
            { vNo, data.voucherType, data.voucherDate, e.partyId, e.partyName,
              data.invoiceNo, e.amount, e.drCr, e.narration.isEmpty() ? data.narration : e.narration }
        );
        if (!ok) {
            allOk = false;
            break;
        }
    }

    if (allOk) {
        DatabaseManager::instance().commit();
        res.success = true;
        res.voucherNumber = vNo;
        emit vouchersChanged();
    } else {
        DatabaseManager::instance().rollback();
        res.success = false;
        res.errorMessage = "Database failure while saving voucher lines.";
    }

    return res;
}

bool VoucherPipeline::deleteVoucher(int voucherId, QString* outError) {
    Q_UNUSED(voucherId);
    Q_UNUSED(outError);
    return false;
}

QStringList VoucherPipeline::getAvailableVoucherTypes() {
    QVariantList rows = DatabaseManager::instance().executeQuery(
        "SELECT DISTINCT voucher_type FROM transactions ORDER BY voucher_type ASC;"
    );
    QStringList types;
    for (const auto& r : rows) {
        types << r.toMap().value("voucher_type").toString();
    }
    if (types.isEmpty()) {
        types << "Sales" << "Purchase" << "Payment" << "Receipt" << "Journal" << "Contra" << "JFrm" << "IFrm";
    }
    return types;
}

QVariantList VoucherPipeline::getVoucherSummaries(const QString& fromDateIso, const QString& toDateIso, const QString& typeFilter) {
    QStringList types;
    if (!typeFilter.isEmpty()) types << typeFilter;
    QVector<UnifiedVoucherData> vouchers = queryVouchers(fromDateIso, toDateIso, types);

    QVariantList list;
    for (const auto& v : vouchers) {
        QVariantMap m;
        m["voucher_no"] = v.voucherNumber;
        m["voucher_type"] = v.voucherType;
        m["voucher_date"] = v.voucherDate;
        m["party_name"] = v.partyName;
        m["invoice_no"] = v.invoiceNo;
        m["total_amount"] = v.totalAmount;
        m["narration"] = v.narration;
        m["is_balanced"] = v.isBalanced();
        list.append(m);
    }
    return list;
}
