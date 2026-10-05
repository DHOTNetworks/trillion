#include "salary_register_controller.h"
#include "../database_manager.h"
#include "../engine/fiscal_year_helper.h"
#include "../models/vouchers_model.h"
#include "../models/account_classifier.h"
#include <QDate>
#include <QDir>
#include <QFileInfo>
#include <QProcess>
#include <QRegularExpression>
#include <QDebug>
#include <cmath>

#if defined(HAS_LIBMDB) || __has_include("mdbtools.h")
#include "mdbtools.h"
#define USE_LIBMDB 1
#endif

namespace MahadevERP {

SalaryRegisterController::SalaryRegisterController(QObject* parent)
    : QObject(parent)
{
    FiscalYearInfo activeFy = FiscalYearHelper::getActiveFiscalYear();
    QDate cur = QDate::currentDate();
    m_currentYear = cur.year();
    m_currentMonth = cur.month();
    if (m_currentMonth == 0) m_currentMonth = 4;
}

QString SalaryRegisterController::getMonthYearKey(int year, int month) const {
    return QString("%1-%2").arg(year).arg(month, 2, 10, QChar('0'));
}

QString SalaryRegisterController::currentMonthLabel() const {
    QDate d(m_currentYear, m_currentMonth, 1);
    return d.toString("MMMM yyyy");
}

double SalaryRegisterController::calculateEarnedBasic(double baseSalary, int daysInMonth, double presentDays) {
    if (daysInMonth <= 0 || baseSalary <= 0.0) return 0.0;
    if (presentDays <= 0.0) return 0.0;
    if (presentDays >= daysInMonth) return std::round(baseSalary * 100.0) / 100.0;
    double earned = (baseSalary / static_cast<double>(daysInMonth)) * presentDays;
    return std::round(earned * 100.0) / 100.0;
}

double SalaryRegisterController::calculateNet(double earnedBasic, double allowances, double pf, double tds, double advance, double other) {
    double net = earnedBasic + allowances - pf - tds - advance - other;
    return std::max(0.0, std::round(net * 100.0) / 100.0);
}

void SalaryRegisterController::resolveDesignationAndStation(const QString& rawName, const QString& rawContact, const QString& rawStation, const QString& rawCity, QString& outDesignation, QString& outStation) {
    QString st = rawStation.trimmed();
    if (st.isEmpty()) st = rawCity.trimmed();

    static const QRegularExpression stRegex(R"(\[(.*?)\])");
    auto stMatch = stRegex.match(rawName);
    if (stMatch.hasMatch()) {
        st = stMatch.captured(1).trimmed();
    }

    QString desig;
    QString lower = rawName.toLower();

    static const QRegularExpression accRegex(R"(\b(accountant|acc)\b)");
    static const QRegularExpression labRegex(R"(\b(lab|laboratory)\b)");
    static const QRegularExpression sellaRegex(R"(\b(sella|shellar|sheller)\b)");

    if (accRegex.match(lower).hasMatch()) desig = "Accountant";
    else if (lower.contains("shellar foreman") || lower.contains("sheller foreman")) desig = "Sheller Foreman";
    else if (lower.contains("boiler") && (lower.contains("foreman") || lower.contains("forman"))) desig = "Boiler Foreman";
    else if (lower.contains("dryer") && (lower.contains("foreman") || lower.contains("forman"))) desig = "Dryer Foreman";
    else if (lower.contains("sella") && (lower.contains("foreman") || lower.contains("forman"))) desig = "Sella Foreman";
    else if (lower.contains("foreman") || lower.contains("forman")) desig = "Foreman";
    else if (lower.contains("munshi")) desig = "Munshi";
    else if (lower.contains("clerk")) desig = "Clerk";
    else if (lower.contains("driver")) desig = "Driver";
    else if (lower.contains("unloader") || lower.contains("unloading")) desig = "Paddy Unloader";
    else if (lower.contains("silai")) desig = "Silai / Stitching";
    else if (lower.contains("paking") || lower.contains("packing")) desig = "Packing Staff";
    else if (lower.contains("bharai")) desig = "Bharai / Loading";
    else if (labRegex.match(lower).hasMatch()) desig = "Lab Assistant";
    else if (lower.contains("mistri") || lower.contains("mistry")) desig = "Mistri / Mechanic";
    else if (lower.contains("helper")) desig = "Helper";
    else if (lower.contains("bijli") || lower.contains("electrician")) desig = "Electrician";
    else if (lower.contains("kitchen") || lower.contains("rasoiya") || lower.contains("cook")) desig = "Kitchen / Cook";
    else if (lower.contains("thekedar") || lower.contains("thakedar") || lower.contains("thekdar")) desig = "Thekedar / Contractor";
    else if (lower.contains("plant operetor") || lower.contains("plant operator")) desig = "Plant Operator";
    else if (lower.contains("operetor") || lower.contains("operator")) desig = "Operator";
    else if (lower.contains("boiler")) desig = "Boiler Operator";
    else if (sellaRegex.match(lower).hasMatch()) desig = "Sella Operator";
    else if (lower.contains("bardane") || lower.contains("bardana")) desig = "Bardana Staff";
    else if (lower.contains("guard") || lower.contains("chowkidar") || lower.contains("security")) desig = "Security Guard";
    else if (lower.contains("bhatti")) desig = "Bhatti Operator";

    static const QRegularExpression coRegex(R"((c/o\s+[^\[]+))", QRegularExpression::CaseInsensitiveOption);
    auto coMatch = coRegex.match(rawName);
    if (coMatch.hasMatch()) {
        QString coTxt = coMatch.captured(1).trimmed();
        static const QRegularExpression monthSuffix(R"(\b(jan|feb|mar|apr|may|jun|jul|aug|sep|oct|nov|dec|april|jully|july|august|march)\b.*)", QRegularExpression::CaseInsensitiveOption);
        coTxt = coTxt.replace(monthSuffix, "").trimmed();
        if (!desig.isEmpty()) {
            desig = QString("%1 (%2)").arg(desig, coTxt);
        } else {
            desig = coTxt;
        }
    }

    if (desig.isEmpty() && !rawContact.isEmpty()) {
        QString cleanCp = rawContact;
        cleanCp = cleanCp.replace(stRegex, "").trimmed();
        QString cleanNm = rawName;
        cleanNm = cleanNm.replace(stRegex, "").trimmed();
        if (cleanCp.compare(cleanNm, Qt::CaseInsensitive) != 0 && cleanCp.compare(rawName, Qt::CaseInsensitive) != 0) {
            desig = cleanCp;
        }
    }

    outDesignation = desig;
    outStation = st;
}

void SalaryRegisterController::loadPayroll(int year, int month) {
    m_currentYear = year;
    m_currentMonth = month;
    m_items.clear();

    QDate monthStart(year, month, 1);
    int daysInMonth = monthStart.daysInMonth();
    QString myKey = getMonthYearKey(year, month);
    QString monthName = monthStart.toString("MMMM");
    QString shortMonth = monthStart.toString("MMM");
    QString shortYear = monthStart.toString("yy");

    auto& db = DatabaseManager::instance();

    // 1. Fetch all employees from parties
    // Group 35 is Employees, also check group_name and party_type
    QVariantList empRows = db.executeQuery(
        "SELECT id, name, contact_person, phone, mobile, party_station, city, salary_per_month, legacy_id "
        "FROM parties "
        "WHERE group_code = 35 OR group_name LIKE '%Employee%' OR party_type = 'Employee' "
        "ORDER BY name COLLATE NOCASE ASC;"
    );

    // 2. Fetch any already recorded salary_payments for this month
    QVariantList paidRows = db.executeQuery(
        "SELECT id, party_id, voucher_no, voucher_date, base_salary, days_in_month, present_days, "
        "earned_basic, allowances, pf_deduction, tds_deduction, advance_deduction, other_deduction, net_salary "
        "FROM salary_payments "
        "WHERE month_year = ?;",
        {myKey}
    );

    QHash<int, QVariantMap> paidMap;
    for (const auto& r : paidRows) {
        QVariantMap m = r.toMap();
        paidMap.insert(m.value("party_id").toInt(), m);
    }

    // 3. Also check transactions/vouchers for posted salary journal entries for this month
    QVariantList vchRows = db.executeQuery(
        "SELECT party_id, party_name, voucher_no, voucher_date, amount, narration "
        "FROM transactions "
        "WHERE trans_type = 'Jrnl' AND dr_cr = 'Cr' "
        "AND (narration LIKE ? OR narration LIKE ? OR narration LIKE ?);",
        {
            QString("%%1%%2%").arg(monthName).arg(year),
            QString("%%1-%2%").arg(shortMonth).arg(year),
            QString("%%1-%2%").arg(shortMonth).arg(shortYear)
        }
    );

    QHash<QString, QVariantMap> postedTxMap;
    for (const auto& vr : vchRows) {
        QVariantMap tm = vr.toMap();
        QString pName = tm.value("party_name").toString().trimmed().toLower();
        postedTxMap.insert(pName, tm);
        int pId = tm.value("party_id").toInt();
        if (pId > 0) {
            postedTxMap.insert(QString::number(pId), tm);
        }
    }

    // 4. Assemble employee salary items
    for (const auto& rowVar : empRows) {
        QVariantMap r = rowVar.toMap();
        EmployeeSalaryItem item;
        item.partyId = r.value("id").toInt();
        item.legacyCode = r.value("legacy_id").toInt();
        item.employeeName = r.value("name").toString().trimmed();
        
        QString rawContact = r.value("contact_person").toString().trimmed();
        QString rawStation = r.value("party_station").toString().trimmed();
        QString rawCity = r.value("city").toString().trimmed();
        
        resolveDesignationAndStation(item.employeeName, rawContact, rawStation, rawCity, item.designation, item.station);
        
        item.phone = r.value("mobile", r.value("phone")).toString().trimmed();
        item.masterSalary = r.value("salary_per_month").toDouble();
        item.daysInMonth = daysInMonth;

        if (paidMap.contains(item.partyId)) {
            const QVariantMap& p = paidMap.value(item.partyId);
            item.salaryPaymentId = p.value("id").toInt();
            item.masterSalary = p.value("base_salary").toDouble();
            item.daysInMonth = p.value("days_in_month").toInt();
            item.presentDays = p.value("present_days").toDouble();
            item.earnedBasic = p.value("earned_basic").toDouble();
            item.allowances = p.value("allowances").toDouble();
            item.pfDeduction = p.value("pf_deduction").toDouble();
            item.tdsDeduction = p.value("tds_deduction").toDouble();
            item.advanceDeduction = p.value("advance_deduction").toDouble();
            item.otherDeduction = p.value("other_deduction").toDouble();
            item.netPayable = p.value("net_salary").toDouble();
            item.isPosted = true;
            item.postedVoucherNo = p.value("voucher_no").toString();
            item.postedVoucherDate = p.value("voucher_date").toString();
            item.isSelected = false; // already posted, default unselected
        } else {
            // Check if posted in transactions
            QString pNameKey = item.employeeName.toLower();
            QString pIdKey = QString::number(item.partyId);
            if (postedTxMap.contains(pNameKey) || postedTxMap.contains(pIdKey)) {
                QVariantMap tm = postedTxMap.contains(pNameKey) ? postedTxMap.value(pNameKey) : postedTxMap.value(pIdKey);
                item.isPosted = true;
                item.postedVoucherNo = tm.value("voucher_no").toString();
                item.postedVoucherDate = tm.value("voucher_date").toString();
                item.presentDays = static_cast<double>(daysInMonth);
                item.earnedBasic = item.masterSalary;
                item.netPayable = tm.value("amount").toDouble();
                if (item.netPayable <= 0.0) item.netPayable = item.masterSalary;
                item.isSelected = false;
            } else {
                item.isPosted = false;
                item.presentDays = static_cast<double>(daysInMonth);
                item.earnedBasic = calculateEarnedBasic(item.masterSalary, item.daysInMonth, item.presentDays);
                item.allowances = 0.0;
                item.pfDeduction = 0.0;
                item.tdsDeduction = 0.0;
                item.advanceDeduction = 0.0;
                item.otherDeduction = 0.0;
                item.netPayable = calculateNet(item.earnedBasic, item.allowances, item.pfDeduction, item.tdsDeduction, item.advanceDeduction, item.otherDeduction);
                item.isSelected = (item.masterSalary > 0.0);
            }
        }

        m_items.append(item);
    }

    recalculateSummary();
    emit dataLoaded();
}

void SalaryRegisterController::refresh() {
    loadPayroll(m_currentYear, m_currentMonth);
}

void SalaryRegisterController::updateItemCalculations(int index) {
    if (index < 0 || index >= m_items.size()) return;
    auto& item = m_items[index];
    item.earnedBasic = calculateEarnedBasic(item.masterSalary, item.daysInMonth, item.presentDays);
    item.netPayable = calculateNet(item.earnedBasic, item.allowances, item.pfDeduction, item.tdsDeduction, item.advanceDeduction, item.otherDeduction);
    recalculateSummary();
}

void SalaryRegisterController::setItemPresentDays(int index, double days) {
    if (index < 0 || index >= m_items.size()) return;
    m_items[index].presentDays = std::max(0.0, std::min(static_cast<double>(m_items[index].daysInMonth), days));
    updateItemCalculations(index);
}

void SalaryRegisterController::setItemAllowances(int index, double allowances) {
    if (index < 0 || index >= m_items.size()) return;
    m_items[index].allowances = std::max(0.0, allowances);
    updateItemCalculations(index);
}

void SalaryRegisterController::setItemPf(int index, double pf) {
    if (index < 0 || index >= m_items.size()) return;
    m_items[index].pfDeduction = std::max(0.0, pf);
    updateItemCalculations(index);
}

void SalaryRegisterController::setItemTds(int index, double tds) {
    if (index < 0 || index >= m_items.size()) return;
    m_items[index].tdsDeduction = std::max(0.0, tds);
    updateItemCalculations(index);
}

void SalaryRegisterController::setItemAdvance(int index, double advance) {
    if (index < 0 || index >= m_items.size()) return;
    m_items[index].advanceDeduction = std::max(0.0, advance);
    updateItemCalculations(index);
}

void SalaryRegisterController::setItemOther(int index, double other) {
    if (index < 0 || index >= m_items.size()) return;
    m_items[index].otherDeduction = std::max(0.0, other);
    updateItemCalculations(index);
}

void SalaryRegisterController::setItemSelected(int index, bool selected) {
    if (index < 0 || index >= m_items.size()) return;
    m_items[index].isSelected = selected;
    recalculateSummary();
}

void SalaryRegisterController::selectAll(bool select) {
    for (auto& item : m_items) {
        if (!item.isPosted) {
            item.isSelected = select;
        }
    }
    recalculateSummary();
}

void SalaryRegisterController::selectPendingOnly() {
    for (auto& item : m_items) {
        item.isSelected = (!item.isPosted && item.netPayable > 0.0);
    }
    recalculateSummary();
}

void SalaryRegisterController::recalculateSummary() {
    m_summary = SalaryRegisterSummary();
    m_summary.totalEmployees = m_items.size();

    for (const auto& item : m_items) {
        m_summary.totalMasterSalary += item.masterSalary;
        m_summary.totalEarnedBasic += item.earnedBasic;
        m_summary.totalAllowances += item.allowances;
        m_summary.totalPf += item.pfDeduction;
        m_summary.totalTds += item.tdsDeduction;
        m_summary.totalAdvances += item.advanceDeduction;
        m_summary.totalOther += item.otherDeduction;

        if (item.isPosted) {
            m_summary.postedEmployees++;
            m_summary.totalPostedAmount += item.netPayable;
        } else {
            m_summary.pendingEmployees++;
        }

        if (item.isSelected) {
            m_summary.selectedEmployees++;
            m_summary.totalNetPayable += item.netPayable;
        }
    }

    emit summaryChanged();
}

bool SalaryRegisterController::updateMasterSalary(int partyId, double newSalary) {
    if (partyId <= 0 || newSalary < 0.0) return false;
    auto& db = DatabaseManager::instance();
    bool ok = db.executeNonQuery(
        "UPDATE parties SET salary_per_month = ? WHERE id = ?;",
        {newSalary, partyId}
    );
    if (ok) {
        for (int i = 0; i < m_items.size(); ++i) {
            if (m_items[i].partyId == partyId) {
                m_items[i].masterSalary = newSalary;
                updateItemCalculations(i);
                break;
            }
        }
    }
    return ok;
}

SalaryRegisterController::PostResult SalaryRegisterController::postSelectedSalaries(const QDate& voucherDate, const QString& customNarration) {
    PostResult res;

    // Filter selected items with positive net payable
    QVector<EmployeeSalaryItem> selectedItems;
    for (const auto& item : m_items) {
        if (item.isSelected && item.netPayable > 0.0) {
            selectedItems.append(item);
        }
    }

    if (selectedItems.isEmpty()) {
        res.errorMessage = "No unposted employees selected or net payable is 0.";
        emit postingFinished(false, res.errorMessage);
        return res;
    }

    auto& db = DatabaseManager::instance();

    // Ensure Salary A/c ledger exists
    QVariant salLedgerVar = db.executeScalar("SELECT id, name FROM parties WHERE name = 'Salary A/c' OR legacy_id = 75 LIMIT 1;");
    int salLedgerId = 0;
    QString salLedgerName = "Salary A/c";
    if (salLedgerVar.isValid() && !salLedgerVar.isNull()) {
        salLedgerId = salLedgerVar.toInt();
    } else {
        db.executeNonQuery(
            "INSERT INTO parties (name, group_name, group_code, party_type, opening_balance, balance_type, legacy_id) "
            "VALUES ('Salary A/c', 'Expenditure A/c', 17, 'Expense', 0.0, 'Dr', 75);"
        );
        salLedgerId = static_cast<int>(db.lastInsertedId());
    }

    // Resolve date and Fiscal Year
    QString dtStr = voucherDate.isValid() ? voucherDate.toString("yyyy-MM-dd") : QDate(m_currentYear, m_currentMonth, 1).toString("yyyy-MM-dd");
    FiscalYearInfo fy = FiscalYearHelper::getFiscalYearForDate(dtStr);
    int fyId = 1;
    QString fyLabel = fy.name;
    QVariant fyIdVar = db.executeScalar("SELECT id FROM financial_years WHERE year_name = ? LIMIT 1;", {fy.name});
    if (fyIdVar.isValid() && !fyIdVar.isNull()) {
        fyId = fyIdVar.toInt();
    } else {
        QVariant anyFy = db.executeScalar("SELECT id FROM financial_years WHERE is_active = 1 LIMIT 1;");
        if (anyFy.isValid() && !anyFy.isNull()) fyId = anyFy.toInt();
    }

    QString monthYearKey = getMonthYearKey(m_currentYear, m_currentMonth);
    QString monthName = QDate(m_currentYear, m_currentMonth, 1).toString("MMMM yyyy");
    QString shortMonthYear = QDate(m_currentYear, m_currentMonth, 1).toString("MMM-yyyy");

    VouchersModel vModel;

    db.beginTransaction();

    for (const auto& item : selectedItems) {
        QString vchNo = vModel.get_next_voucher_no("Jrnl", fyLabel);

        QString narrDr = customNarration.isEmpty() ? QString("Salary %1 (%2)").arg(shortMonthYear, item.employeeName) : customNarration;
        QString narrCr = customNarration.isEmpty() ? QString("Salary For %1").arg(shortMonthYear) : customNarration;

        // Insert Voucher Header
        bool okVch = db.executeNonQuery(
            "INSERT INTO vouchers (fy_id, financial_year, voucher_no, voucher_date, voucher_type, legacy_type, party_id, ledger_id, party_name, account_type, amount, narration) "
            "VALUES (?, ?, ?, ?, 'Journal', 'Jrnl', ?, ?, ?, ?, ?, ?);",
            {fyId, fyLabel, vchNo, dtStr, item.partyId, salLedgerId, salLedgerName, item.employeeName, item.netPayable, narrDr}
        );

        if (!okVch) {
            db.rollback();
            res.errorMessage = QString("Failed to create voucher for %1").arg(item.employeeName);
            emit postingFinished(false, res.errorMessage);
            return res;
        }

        // 1. Debit leg: Salary A/c Dr
        db.executeNonQuery(
            "INSERT INTO transactions (fy_id, financial_year, voucher_no, voucher_date, voucher_type, trans_type, party_id, party_name, opposing_account, dr_cr, amount, narration) "
            "VALUES (?, ?, ?, ?, 'Journal', 'Jrnl', ?, ?, ?, 'Dr', ?, ?);",
            {fyId, fyLabel, vchNo, dtStr, salLedgerId, salLedgerName, item.employeeName, item.netPayable, narrDr}
        );

        // 2. Credit leg: Employee Party A/c Cr
        db.executeNonQuery(
            "INSERT INTO transactions (fy_id, financial_year, voucher_no, voucher_date, voucher_type, trans_type, party_id, party_name, opposing_account, dr_cr, amount, narration) "
            "VALUES (?, ?, ?, ?, 'Journal', 'Jrnl', ?, ?, ?, 'Cr', ?, ?);",
            {fyId, fyLabel, vchNo, dtStr, item.partyId, item.employeeName, salLedgerName, item.netPayable, narrCr}
        );

        // 3. Log in salary_payments
        db.executeNonQuery(
            "INSERT INTO salary_payments (fy_id, financial_year, month_year, voucher_date, voucher_no, party_id, employee_name, "
            "base_salary, days_in_month, present_days, earned_basic, allowances, pf_deduction, tds_deduction, advance_deduction, other_deduction, net_salary, narration) "
            "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);",
            {
                fyId, fyLabel, monthYearKey, dtStr, vchNo, item.partyId, item.employeeName,
                item.masterSalary, item.daysInMonth, item.presentDays, item.earnedBasic,
                item.allowances, item.pfDeduction, item.tdsDeduction, item.advanceDeduction,
                item.otherDeduction, item.netPayable, narrDr
            }
        );

        res.vouchersCreated++;
        res.totalAmountPosted += item.netPayable;
        res.createdVoucherNumbers.append(vchNo);
    }

    db.commit();
    res.success = true;

    // Reload state
    loadPayroll(m_currentYear, m_currentMonth);

    emit postingFinished(true, QString("Successfully posted %1 salary journal vouchers totaling ₹%2.")
                                  .arg(res.vouchersCreated)
                                  .arg(res.totalAmountPosted, 0, 'f', 2));
    return res;
}

int SalaryRegisterController::syncMasterSalariesFromBahiKhata(const QString& explicitMdbPath) {
    QString mdbPath = explicitMdbPath;
    if (mdbPath.isEmpty()) {
        QStringList candidates = {
            "Bahi-Khata-Data/Data.002",
            "Bahi-Khata-Data/Data.001",
            "Bahi-Khata-Data/Data.004",
            "Bahi-Khata-Data/Data.005",
            "Bahi-Khata-Data/Data.018"
        };
        for (const auto& c : candidates) {
            if (QFile::exists(c)) {
                mdbPath = c;
                break;
            }
        }
    }

    if (mdbPath.isEmpty() || !QFile::exists(mdbPath)) {
        return 0;
    }

    int updatedCount = 0;
    auto& db = DatabaseManager::instance();

#ifdef USE_LIBMDB
    MdbHandle* mdb = mdb_open(QFile::encodeName(mdbPath).constData(), MDB_NOFLAGS);
    if (!mdb) mdb = mdb_open(mdbPath.toUtf8().constData(), MDB_NOFLAGS);

    if (mdb) {
        char ledgersTblName[] = "Ledgers";
        MdbTableDef* table = mdb_read_table_by_name(mdb, ledgersTblName, MDB_TABLE);
        if (table) {
            mdb_read_columns(table);
            mdb_rewind_table(table);

            char boundValues[256][4096];
            for (int i = 0; i < table->num_cols && i < 256; ++i) {
                mdb_bind_column(table, i + 1, boundValues[i], nullptr);
            }

            int codeCol = -1;
            int nameCol = -1;
            int salCol = -1;

            for (int i = 0; i < table->num_cols; ++i) {
                MdbColumn* col = static_cast<MdbColumn*>(g_ptr_array_index(table->columns, i));
                if (!col) continue;
                QString colName = QString::fromUtf8(col->name).trimmed();
                if (colName.compare("Code1st", Qt::CaseInsensitive) == 0) codeCol = i;
                else if (colName.compare("LedgerName", Qt::CaseInsensitive) == 0) nameCol = i;
                else if (colName.compare("SalaryPerMonth", Qt::CaseInsensitive) == 0) salCol = i;
            }

            if (salCol >= 0 && (nameCol >= 0 || codeCol >= 0)) {
                db.beginTransaction();
                while (mdb_fetch_row(table)) {
                    double sal = 0.0;
                    if (salCol >= 0) sal = QString::fromUtf8(boundValues[salCol]).trimmed().toDouble();
                    if (sal > 0.0) {
                        QString name = nameCol >= 0 ? QString::fromUtf8(boundValues[nameCol]).trimmed() : "";
                        int code = codeCol >= 0 ? QString::fromUtf8(boundValues[codeCol]).trimmed().toInt() : 0;

                        bool ok = false;
                        if (code > 0) {
                            ok = db.executeNonQuery("UPDATE parties SET salary_per_month = ? WHERE legacy_id = ? OR name = ?;", {sal, code, name});
                        } else if (!name.isEmpty()) {
                            ok = db.executeNonQuery("UPDATE parties SET salary_per_month = ? WHERE name = ?;", {sal, name});
                        }
                        if (ok) updatedCount++;
                    }
                }
                db.commit();
            }
            mdb_free_tabledef(table);
        }
        mdb_close(mdb);
    }
#else
    // Process fallback using mdb-export CLI if available
    QProcess proc;
    proc.start("mdb-export", {"-H", mdbPath, "Ledgers"});
    if (proc.waitForFinished(10000)) {
        QByteArray out = proc.readAllStandardOutput();
        // Parse CSV rows
    }
#endif

    if (updatedCount > 0) {
        refresh();
    }

    return updatedCount;
}

} // namespace MahadevERP
