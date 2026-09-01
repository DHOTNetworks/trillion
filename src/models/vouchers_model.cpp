#include "vouchers_model.h"
#include "../database_manager.h"
#include <QDate>
#include <QRegularExpression>

VouchersModel::VouchersModel(QObject* parent)
    : BaseTableModel(
        {"Voucher No", "Date", "Type", "Party", "Account", "Amount (₹)", "Narration"},
        {"voucher_no", "voucher_date", "voucher_type", "party_name", "account_type", "amount", "narration"},
        parent
    )
{
    reload_data();
}

void VouchersModel::reload_data() {
    beginResetModel();
    m_data = DatabaseManager::instance().executeQuery("SELECT * FROM vouchers ORDER BY id DESC;");
    endResetModel();
    emit dataChangedSignal();
    emit countChanged();
}

QString VouchersModel::get_next_voucher_no(const QString& v_type, const QString& fy) {
    QString prefix = v_type.isEmpty() ? "ChPt" : v_type;
    QString targetFy = fy;
    if (targetFy.isEmpty()) {
        QVariant fyVal = DatabaseManager::instance().executeScalar("SELECT year_name FROM financial_years WHERE is_active = 1 LIMIT 1;");
        targetFy = fyVal.isValid() ? fyVal.toString() : "FY 2026-27";
    }

    QVariantList rows = DatabaseManager::instance().executeQuery(
        "SELECT voucher_no FROM vouchers WHERE voucher_no LIKE ? AND financial_year = ?;",
        {prefix + "-%", targetFy}
    );

    long long maxId = 0;
    QRegularExpression re("-(\\d+)$");
    for (const QVariant& r : rows) {
        QString v = r.toMap().value("voucher_no").toString();
        QRegularExpressionMatch m = re.match(v);
        if (m.hasMatch()) {
            long long num = m.captured(1).toLongLong();
            if (num > maxId) maxId = num;
        }
    }
    return QString("%1-%2").arg(prefix, QString::number(maxId + 1));
}

bool VouchersModel::add_voucher(const QString& vch_type, const QString& party_name, const QString& vch_date, const QString& account_type, double amount, const QString& narration) {
    QString dt = vch_date.isEmpty() ? QDate::currentDate().toString("yyyy-MM-dd") : vch_date;
    QVariantList fyRows = DatabaseManager::instance().executeQuery(
        "SELECT id, year_name FROM financial_years WHERE start_date <= ? AND end_date >= ? LIMIT 1;",
        {dt, dt}
    );
    int fyId = 28;
    QString fyLabel = "FY 2026-27";
    if (!fyRows.isEmpty()) {
        fyId = fyRows.first().toMap().value("id").toInt();
        fyLabel = fyRows.first().toMap().value("year_name").toString();
    }

    QString vchNo = get_next_voucher_no(vch_type, fyLabel);

    bool ok = DatabaseManager::instance().executeNonQuery(
        "INSERT INTO vouchers (fy_id, financial_year, voucher_no, voucher_date, voucher_type, legacy_type, party_id, party_name, account_type, amount, narration) "
        "VALUES (?, ?, ?, ?, ?, ?, 1, ?, ?, ?, ?);",
        {fyId, fyLabel, vchNo, dt, vch_type, vch_type, party_name, account_type, amount, narration}
    );
    if (ok) reload_data();
    return ok;
}

bool VouchersModel::add_cheque_voucher(const QString& vch_type, const QString& dr_party, const QString& cr_party, double amount, const QString& chq_no, const QString& narration, const QString& vch_date) {
    QString dt = vch_date.isEmpty() ? QDate::currentDate().toString("yyyy-MM-dd") : vch_date;
    QVariantList fyRows = DatabaseManager::instance().executeQuery(
        "SELECT id, year_name FROM financial_years WHERE start_date <= ? AND end_date >= ? LIMIT 1;",
        {dt, dt}
    );
    int fyId = 28;
    QString fyLabel = "FY 2026-27";
    if (!fyRows.isEmpty()) {
        fyId = fyRows.first().toMap().value("id").toInt();
        fyLabel = fyRows.first().toMap().value("year_name").toString();
    }

    QString vchNo = get_next_voucher_no(vch_type, fyLabel);
    QString fullNarr = chq_no.isEmpty() ? "" : ("Ch. No. " + chq_no);
    if (!narration.isEmpty()) {
        fullNarr += fullNarr.isEmpty() ? narration : (" | " + narration);
    }

    bool ok = DatabaseManager::instance().executeNonQuery(
        "INSERT INTO vouchers (fy_id, financial_year, voucher_no, instrument_no, voucher_date, voucher_type, legacy_type, party_id, party_name, account_type, amount, narration) "
        "VALUES (?, ?, ?, ?, ?, ?, ?, 1, ?, ?, ?, ?);",
        {fyId, fyLabel, vchNo, chq_no, dt, vch_type, vch_type, dr_party, cr_party, amount, fullNarr}
    );
    if (ok) reload_data();
    return ok;
}

bool VouchersModel::add_journal_voucher(const QString& dr_party, const QString& cr_party, double amount, const QString& ref_no, const QString& narration, const QString& vch_date, const QString& vch_type) {
    QString dt = vch_date.isEmpty() ? QDate::currentDate().toString("yyyy-MM-dd") : vch_date;
    QVariantList fyRows = DatabaseManager::instance().executeQuery(
        "SELECT id, year_name FROM financial_years WHERE start_date <= ? AND end_date >= ? LIMIT 1;",
        {dt, dt}
    );
    int fyId = 28;
    QString fyLabel = "FY 2026-27";
    if (!fyRows.isEmpty()) {
        fyId = fyRows.first().toMap().value("id").toInt();
        fyLabel = fyRows.first().toMap().value("year_name").toString();
    }

    QString vchNo = get_next_voucher_no("Jrnl", fyLabel);
    QString fullNarr = ref_no.isEmpty() ? "" : ("Ref: " + ref_no);
    if (!narration.isEmpty()) {
        fullNarr += fullNarr.isEmpty() ? narration : (" | " + narration);
    }

    bool ok = DatabaseManager::instance().executeNonQuery(
        "INSERT INTO vouchers (fy_id, financial_year, voucher_no, instrument_no, voucher_date, voucher_type, legacy_type, party_id, party_name, account_type, amount, narration) "
        "VALUES (?, ?, ?, ?, ?, ?, 'Jrnl', 1, ?, ?, ?, ?);",
        {fyId, fyLabel, vchNo, ref_no, dt, vch_type, dr_party, cr_party, amount, fullNarr}
    );
    if (ok) reload_data();
    return ok;
}
