#include "tax_challan_controller.h"
#include "../database_manager.h"
#include <QDate>
#include <QRegularExpression>
#include <cmath>

TaxChallanController::TaxChallanController(QObject *parent)
    : QObject(parent)
{
    resetForm();
}

void TaxChallanController::setTaxType(const QString &type)
{
    QString upper = type.trimmed().toUpper();
    if (upper != "TDS" && upper != "TCS") upper = "TDS";
    if (m_taxType != upper) {
        m_taxType = upper;
        emit taxTypeChanged();
        resetForm();
    }
}

void TaxChallanController::setChallanNo(const QString &no)
{
    if (m_challanNo != no) {
        m_challanNo = no;
        emit challanNoChanged();
    }
}

void TaxChallanController::setChallanDate(const QString &d)
{
    if (m_challanDate != d) {
        m_challanDate = d;
        emit challanDateChanged();
    }
}

void TaxChallanController::setPostInBooks(bool b)
{
    if (m_postInBooks != b) {
        m_postInBooks = b;
        emit postInBooksChanged();
    }
}

void TaxChallanController::setPeriodFrom(const QString &d)
{
    if (m_periodFrom != d) {
        m_periodFrom = d;
        emit periodChanged();
    }
}

void TaxChallanController::setPeriodTo(const QString &d)
{
    if (m_periodTo != d) {
        m_periodTo = d;
        emit periodChanged();
    }
}

void TaxChallanController::setSurcharge(double amt)
{
    m_surcharge = std::max(0.0, amt);
    recalculate();
}

void TaxChallanController::setCess(double amt)
{
    m_cess = std::max(0.0, amt);
    recalculate();
}

void TaxChallanController::setInterestAmount(double amt)
{
    m_interestAmount = std::max(0.0, amt);
    recalculate();
}

void TaxChallanController::setInterestLedgerId(int id)
{
    if (m_interestLedgerId != id) {
        m_interestLedgerId = id;
        emit interestLedgerIdChanged();
    }
}

void TaxChallanController::setPenaltyAmount(double amt)
{
    m_penaltyAmount = std::max(0.0, amt);
    recalculate();
}

void TaxChallanController::setPenaltyLedgerId(int id)
{
    if (m_penaltyLedgerId != id) {
        m_penaltyLedgerId = id;
        emit penaltyLedgerIdChanged();
    }
}

void TaxChallanController::setOtherAmount(double amt)
{
    m_otherAmount = amt;
    recalculate();
}

void TaxChallanController::setOtherLedgerId(int id)
{
    if (m_otherLedgerId != id) {
        m_otherLedgerId = id;
        emit otherLedgerIdChanged();
    }
}

void TaxChallanController::setBankLedgerId(int id)
{
    if (m_bankLedgerId != id) {
        m_bankLedgerId = id;
        emit bankLedgerIdChanged();
    }
}

void TaxChallanController::setChequeNo(const QString &no)
{
    if (m_chequeNo != no) {
        m_chequeNo = no;
        emit chequeNoChanged();
    }
}

void TaxChallanController::setChequeDate(const QString &d)
{
    if (m_chequeDate != d) {
        m_chequeDate = d;
        emit chequeDateChanged();
    }
}

void TaxChallanController::setBsrCode(const QString &code)
{
    if (m_bsrCode != code) {
        m_bsrCode = code;
        emit bsrCodeChanged();
    }
}

void TaxChallanController::setMinorHead(const QString &h)
{
    if (m_minorHead != h) {
        m_minorHead = h;
        emit minorHeadChanged();
    }
}

void TaxChallanController::setMajorHead(const QString &h)
{
    if (m_majorHead != h) {
        m_majorHead = h;
        emit majorHeadChanged();
    }
}

void TaxChallanController::setNarration(const QString &n)
{
    if (m_narration != n) {
        m_narration = n;
        emit narrationChanged();
    }
}

void TaxChallanController::resetForm()
{
    m_editChallanId = 0;
    QDate today = QDate::currentDate();
    m_challanDate = today.toString("yyyy-MM-dd");

    // Default period: Current month from 1st to today
    QDate firstOfMonth(today.year(), today.month(), 1);
    m_periodFrom = firstOfMonth.toString("yyyy-MM-dd");
    m_periodTo = today.toString("yyyy-MM-dd");

    m_postInBooks = true;
    m_challanNo = nextChallanNumber();

    m_basicTax = 0.0;
    m_surcharge = 0.0;
    m_cess = 0.0;
    m_totalTax = 0.0;

    m_interestAmount = 0.0;
    m_penaltyAmount = 0.0;
    m_otherAmount = 0.0;
    m_totalChallanAmount = 0.0;

    m_chequeNo.clear();
    m_chequeDate = m_challanDate;
    m_bsrCode.clear();
    m_minorHead = "200";
    m_majorHead = "0021";
    m_narration = QString("%1 Deposit Challan (ITNS 281)").arg(m_taxType);

    m_statusMessage.clear();
    m_isError = false;

    ensureDefaultLedgers();
    fetchUndepositedVouchers();

    emit challanNoChanged();
    emit challanDateChanged();
    emit postInBooksChanged();
    emit periodChanged();
    emit totalsChanged();
    emit bsrCodeChanged();
    emit narrationChanged();
    emit statusChanged();
}

QString TaxChallanController::nextChallanNumber() const
{
    DatabaseManager &db = DatabaseManager::instance();
    QVariant val = db.executeScalar(
        "SELECT MAX(id) FROM tax_deposit_challans WHERE tax_type = ?;",
        {m_taxType}
    );
    int nextId = (val.isValid() && !val.isNull()) ? val.toInt() + 1 : 1;
    return QString("%1-%2").arg(m_taxType).arg(nextId, 4, 10, QChar('0'));
}

void TaxChallanController::ensureDefaultLedgers()
{
    DatabaseManager &db = DatabaseManager::instance();

    // Default Bank Account: first Bank account found
    if (m_bankLedgerId <= 0) {
        QVariant bId = db.executeScalar(
            "SELECT id FROM parties WHERE group_name LIKE '%Bank%' OR party_type = 'Bank' LIMIT 1;"
        );
        if (bId.isValid() && !bId.isNull()) {
            m_bankLedgerId = bId.toInt();
            emit bankLedgerIdChanged();
        }
    }

    // Default Interest Account
    if (m_interestLedgerId <= 0) {
        QVariant intId = db.executeScalar("SELECT id FROM parties WHERE name = 'Interest on Tax A/c' LIMIT 1;");
        if (intId.isValid() && !intId.isNull()) {
            m_interestLedgerId = intId.toInt();
            emit interestLedgerIdChanged();
        }
    }

    // Default Penalty Account
    if (m_penaltyLedgerId <= 0) {
        QVariant penId = db.executeScalar("SELECT id FROM parties WHERE name = 'Penalty on Tax A/c' LIMIT 1;");
        if (penId.isValid() && !penId.isNull()) {
            m_penaltyLedgerId = penId.toInt();
            emit penaltyLedgerIdChanged();
        }
    }
}

void TaxChallanController::fetchUndepositedVouchers()
{
    m_undepositedVouchers.clear();
    DatabaseManager &db = DatabaseManager::instance();

    if (m_taxType == "TDS") {
        // Query regular TDS vouchers
        QVariantList rows = db.executeQuery(
            "SELECT id, 'TDS_VOUCHER' as vch_type, voucher_no, voucher_date, ledger_name as party_name, "
            "total_for_tds as gross_amount, tax_amount_tds as basic_tax, "
            "tax_amount_surcharge as surcharge, tax_amount_cess as cess, total_tax_amount "
            "FROM tds_vouchers "
            "WHERE (is_deposited = 0 OR is_deposited IS NULL) "
            "  AND (challan_id IS NULL OR challan_id = 0) "
            "  AND (voucher_date >= ? AND voucher_date <= ?) "
            "ORDER BY voucher_date ASC, voucher_no ASC;",
            {m_periodFrom, m_periodTo}
        );

        for (const auto &r : rows) {
            QVariantMap m = r.toMap();
            m["selected"] = true;
            m_undepositedVouchers.append(m);
        }

        // Also query 194-Q advance payments
        QVariantList advRows = db.executeQuery(
            "SELECT a.id, 'ADVANCE_194Q' as vch_type, a.voucher_no, a.voucher_date, p.name as party_name, "
            "a.gross_advance_amount as gross_amount, a.tds_amount as basic_tax, "
            "0.0 as surcharge, 0.0 as cess, a.tds_amount as total_tax_amount "
            "FROM advance_payments_194q a "
            "LEFT JOIN parties p ON a.supplier_id = p.id "
            "WHERE (a.is_deposited = 0 OR a.is_deposited IS NULL) "
            "  AND (a.challan_id IS NULL OR a.challan_id = 0) "
            "  AND (a.voucher_date >= ? AND a.voucher_date <= ?) "
            "ORDER BY a.voucher_date ASC, a.voucher_no ASC;",
            {m_periodFrom, m_periodTo}
        );

        for (const auto &r : advRows) {
            QVariantMap m = r.toMap();
            m["selected"] = true;
            m_undepositedVouchers.append(m);
        }

    } else { // TCS
        QVariantList tcsRows = db.executeQuery(
            "SELECT t.id, 'TCS_RECEIPT' as vch_type, t.receipt_no as voucher_no, t.receipt_date as voucher_date, "
            "p.name as party_name, t.without_tcs_amount as gross_amount, t.tcs_amount as basic_tax, "
            "0.0 as surcharge, 0.0 as cess, t.tcs_amount as total_tax_amount "
            "FROM tcs_receipt_vouchers t "
            "LEFT JOIN parties p ON t.party_id = p.id "
            "WHERE (t.is_deposited = 0 OR t.is_deposited IS NULL) "
            "  AND (t.challan_id IS NULL OR t.challan_id = 0) "
            "  AND (t.receipt_date >= ? AND t.receipt_date <= ?) "
            "ORDER BY t.receipt_date ASC, t.receipt_no ASC;",
            {m_periodFrom, m_periodTo}
        );

        for (const auto &r : tcsRows) {
            QVariantMap m = r.toMap();
            m["selected"] = true;
            m_undepositedVouchers.append(m);
        }
    }

    emit undepositedVouchersChanged();
    recalculate();
}

void TaxChallanController::toggleVoucherSelection(int index, bool selected)
{
    if (index >= 0 && index < m_undepositedVouchers.size()) {
        QVariantMap m = m_undepositedVouchers[index].toMap();
        m["selected"] = selected;
        m_undepositedVouchers[index] = m;
        recalculate();
        emit undepositedVouchersChanged();
    }
}

void TaxChallanController::selectAllVouchers(bool selected)
{
    for (int i = 0; i < m_undepositedVouchers.size(); ++i) {
        QVariantMap m = m_undepositedVouchers[i].toMap();
        m["selected"] = selected;
        m_undepositedVouchers[i] = m;
    }
    recalculate();
    emit undepositedVouchersChanged();
}

void TaxChallanController::recalculate()
{
    double sumBasic = 0.0;
    double sumSurcharge = 0.0;
    double sumCess = 0.0;

    for (const auto &v : m_undepositedVouchers) {
        QVariantMap m = v.toMap();
        if (m.value("selected").toBool()) {
            sumBasic += m.value("basic_tax").toDouble();
            sumSurcharge += m.value("surcharge").toDouble();
            sumCess += m.value("cess").toDouble();
        }
    }

    m_basicTax = sumBasic;
    m_surcharge = sumSurcharge;
    m_cess = sumCess;
    m_totalTax = m_basicTax + m_surcharge + m_cess;

    m_totalChallanAmount = m_totalTax + m_interestAmount + m_penaltyAmount + m_otherAmount;
    m_totalChallanAmount = std::round(m_totalChallanAmount * 100.0) / 100.0;

    emit totalsChanged();
}

bool TaxChallanController::saveChallan()
{
    m_isError = false;
    m_statusMessage.clear();

    if (m_challanNo.trimmed().isEmpty()) {
        m_isError = true;
        m_statusMessage = "Challan Number cannot be empty.";
        emit statusChanged();
        return false;
    }

    if (m_bankLedgerId <= 0) {
        m_isError = true;
        m_statusMessage = "Please select a Bank / Cash account for deposit payment.";
        emit statusChanged();
        return false;
    }

    if (!m_bsrCode.trimmed().isEmpty()) {
        static const QRegularExpression bsrRegex("^[0-9]{7}$");
        if (!bsrRegex.match(m_bsrCode.trimmed()).hasMatch()) {
            m_isError = true;
            m_statusMessage = "BSR Code must be exactly 7 numeric digits.";
            emit statusChanged();
            return false;
        }
    }

    // Ensure at least some amount is being deposited
    if (m_totalChallanAmount <= 0.0) {
        m_isError = true;
        m_statusMessage = "Total Challan Amount must be greater than zero.";
        emit statusChanged();
        return false;
    }

    DatabaseManager &db = DatabaseManager::instance();
    db.beginTransaction();

    int fyId = 1;
    QString fyLabel = "FY 2025-26";
    QVariant fyRow = db.executeScalar("SELECT id FROM financial_years WHERE is_active = 1 LIMIT 1;");
    if (!fyRow.isValid() || fyRow.isNull()) {
        fyRow = db.executeScalar("SELECT id FROM financial_years ORDER BY id DESC LIMIT 1;");
    }
    if (fyRow.isValid() && !fyRow.isNull()) {
        fyId = fyRow.toInt();
        QVariant nameVal = db.executeScalar("SELECT year_name FROM financial_years WHERE id = ?;", {fyId});
        if (nameVal.isValid()) fyLabel = nameVal.toString();
    }

    int glVoucherId = 0;

    // 1. Post in Books (Double-Entry General Ledger) if enabled
    if (m_postInBooks) {
        // Resolve Tax Payable Account
        QString taxLedgerName = (m_taxType == "TDS") ? "TDS Payable A/c" : "TCS Payable A/c";
        int taxLedgerId = 0;
        QVariant tRow = db.executeScalar("SELECT id FROM parties WHERE name = ? LIMIT 1;", {taxLedgerName});
        if (tRow.isValid() && !tRow.isNull()) {
            taxLedgerId = tRow.toInt();
        } else {
            db.executeNonQuery(
                "INSERT INTO parties (name, group_name, party_type) VALUES (?, 'Duties & Taxes (GST)', 'Tax');",
                {taxLedgerName}
            );
            taxLedgerId = db.executeScalar("SELECT last_insert_rowid();").toInt();
        }

        // Bank Account Name
        QString bankName = db.executeScalar("SELECT name FROM parties WHERE id = ?;", {m_bankLedgerId}).toString();

        // Insert Voucher Master Header
        QString vchNo = QString("CH-%1").arg(m_challanNo.trimmed());
        QString vchNarration = m_narration.isEmpty()
            ? QString("%1 Tax Deposit Challan ITNS-281 BSR:%2").arg(m_taxType, m_bsrCode)
            : m_narration;

        db.executeNonQuery(
            "INSERT INTO vouchers ("
            "fy_id, financial_year, voucher_no, voucher_date, voucher_type, legacy_type, "
            "party_id, ledger_id, party_name, account_type, amount, narration"
            ") VALUES (?, ?, ?, ?, 'PAYMENT', 'BANK', ?, ?, ?, 'Cr', ?, ?);",
            {
                fyId, fyLabel, vchNo, m_challanDate,
                m_bankLedgerId, m_bankLedgerId, bankName, m_totalChallanAmount,
                vchNarration
            }
        );
        glVoucherId = db.executeScalar("SELECT last_insert_rowid();").toInt();

        int rowNo = 1;

        // Leg 1: Debit Tax Payable
        if (m_totalTax > 0.0) {
            db.executeNonQuery(
                "INSERT INTO transactions ("
                "fy_id, financial_year, voucher_no, voucher_date, voucher_type, trans_type, "
                "account_code, party_id, party_name, opposing_account, dr_cr, amount, "
                "narration, taxable_amount, tds_amount, tds_rate, row_no"
                ") VALUES (?, ?, ?, ?, 'PAYMENT', 'BANK', ?, ?, ?, ?, 'Dr', ?, ?, 0, ?, 0, ?);",
                {
                    fyId, fyLabel, vchNo, m_challanDate,
                    taxLedgerId, taxLedgerId, taxLedgerName, bankName, m_totalTax,
                    vchNarration, m_totalTax, rowNo++
                }
            );
        }

        // Leg 2: Debit Interest (if any)
        if (m_interestAmount > 0.0 && m_interestLedgerId > 0) {
            QString intName = db.executeScalar("SELECT name FROM parties WHERE id = ?;", {m_interestLedgerId}).toString();
            db.executeNonQuery(
                "INSERT INTO transactions ("
                "fy_id, financial_year, voucher_no, voucher_date, voucher_type, trans_type, "
                "account_code, party_id, party_name, opposing_account, dr_cr, amount, "
                "narration, taxable_amount, tds_amount, tds_rate, row_no"
                ") VALUES (?, ?, ?, ?, 'PAYMENT', 'BANK', ?, ?, ?, ?, 'Dr', ?, ?, 0, 0, 0, ?);",
                {
                    fyId, fyLabel, vchNo, m_challanDate,
                    m_interestLedgerId, m_interestLedgerId, intName, bankName, m_interestAmount,
                    "Interest on Tax Deposit", rowNo++
                }
            );
        }

        // Leg 3: Debit Penalty (if any)
        if (m_penaltyAmount > 0.0 && m_penaltyLedgerId > 0) {
            QString penName = db.executeScalar("SELECT name FROM parties WHERE id = ?;", {m_penaltyLedgerId}).toString();
            db.executeNonQuery(
                "INSERT INTO transactions ("
                "fy_id, financial_year, voucher_no, voucher_date, voucher_type, trans_type, "
                "account_code, party_id, party_name, opposing_account, dr_cr, amount, "
                "narration, taxable_amount, tds_amount, tds_rate, row_no"
                ") VALUES (?, ?, ?, ?, 'PAYMENT', 'BANK', ?, ?, ?, ?, 'Dr', ?, ?, 0, 0, 0, ?);",
                {
                    fyId, fyLabel, vchNo, m_challanDate,
                    m_penaltyLedgerId, m_penaltyLedgerId, penName, bankName, m_penaltyAmount,
                    "Penalty on Tax Deposit", rowNo++
                }
            );
        }

        // Leg 4: Debit Other Charges (if any)
        if (m_otherAmount > 0.0 && m_otherLedgerId > 0) {
            QString othName = db.executeScalar("SELECT name FROM parties WHERE id = ?;", {m_otherLedgerId}).toString();
            db.executeNonQuery(
                "INSERT INTO transactions ("
                "fy_id, financial_year, voucher_no, voucher_date, voucher_type, trans_type, "
                "account_code, party_id, party_name, opposing_account, dr_cr, amount, "
                "narration, taxable_amount, tds_amount, tds_rate, row_no"
                ") VALUES (?, ?, ?, ?, 'PAYMENT', 'BANK', ?, ?, ?, ?, 'Dr', ?, ?, 0, 0, 0, ?);",
                {
                    fyId, fyLabel, vchNo, m_challanDate,
                    m_otherLedgerId, m_otherLedgerId, othName, bankName, m_otherAmount,
                    "Other Charges on Tax Deposit", rowNo++
                }
            );
        }

        // Balancing Leg: Credit Bank
        db.executeNonQuery(
            "INSERT INTO transactions ("
            "fy_id, financial_year, voucher_no, voucher_date, voucher_type, trans_type, "
            "account_code, party_id, party_name, opposing_account, dr_cr, amount, "
            "narration, taxable_amount, tds_amount, tds_rate, row_no"
            ") VALUES (?, ?, ?, ?, 'PAYMENT', 'BANK', ?, ?, ?, ?, 'Cr', ?, ?, 0, 0, 0, ?);",
            {
                fyId, fyLabel, vchNo, m_challanDate,
                m_bankLedgerId, m_bankLedgerId, bankName, taxLedgerName, m_totalChallanAmount,
                vchNarration, rowNo++
            }
        );
    }

    // 2. Insert into tax_deposit_challans
    bool ok = db.executeNonQuery(
        "INSERT INTO tax_deposit_challans ("
        "fy_id, tax_type, challan_no, challan_date, post_in_books, period_from, period_to, "
        "basic_tax, surcharge, cess, total_tax, interest_amount, interest_ledger_id, "
        "penalty_amount, penalty_ledger_id, other_amount, other_ledger_id, total_challan_amount, "
        "bank_ledger_id, cheque_no, cheque_date, bsr_code, minor_head, major_head, voucher_id, narration"
        ") VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);",
        {
            fyId, m_taxType, m_challanNo.trimmed(), m_challanDate, m_postInBooks ? 1 : 0,
            m_periodFrom, m_periodTo, m_basicTax, m_surcharge, m_cess, m_totalTax,
            m_interestAmount,
            m_interestLedgerId > 0 ? m_interestLedgerId : QVariant(),
            m_penaltyAmount,
            m_penaltyLedgerId > 0 ? m_penaltyLedgerId : QVariant(),
            m_otherAmount,
            m_otherLedgerId > 0 ? m_otherLedgerId : QVariant(),
            m_totalChallanAmount, m_bankLedgerId,
            m_chequeNo.trimmed(), m_chequeDate, m_bsrCode.trimmed(), m_minorHead, m_majorHead,
            glVoucherId > 0 ? glVoucherId : QVariant(), m_narration.trimmed()
        }
    );

    if (!ok) {
        db.rollback();
        m_isError = true;
        m_statusMessage = "Failed to save Tax Deposit Challan to database.";
        emit statusChanged();
        return false;
    }

    int challanId = db.executeScalar("SELECT last_insert_rowid();").toInt();

    // 3. Insert linked vouchers into tax_deposit_challan_items & mark as deposited
    for (const auto &v : m_undepositedVouchers) {
        QVariantMap m = v.toMap();
        if (m.value("selected").toBool()) {
            int srcId = m.value("id").toInt();
            QString vchType = m.value("vch_type").toString();
            double taxAmt = m.value("basic_tax").toDouble();
            double surAmt = m.value("surcharge").toDouble();
            double cessAmt = m.value("cess").toDouble();
            double totTax = m.value("total_tax_amount").toDouble();

            db.executeNonQuery(
                "INSERT INTO tax_deposit_challan_items ("
                "challan_id, voucher_type, source_voucher_id, tax_amount, surcharge_amount, cess_amount, total_tax_amount"
                ") VALUES (?, ?, ?, ?, ?, ?, ?);",
                {challanId, vchType, srcId, taxAmt, surAmt, cessAmt, totTax}
            );

            // Mark source voucher as deposited
            if (vchType == "TDS_VOUCHER") {
                db.executeNonQuery(
                    "UPDATE tds_vouchers SET is_deposited = 1, challan_id = ? WHERE id = ?;",
                    {challanId, srcId}
                );
            } else if (vchType == "ADVANCE_194Q") {
                db.executeNonQuery(
                    "UPDATE advance_payments_194q SET is_deposited = 1, challan_id = ? WHERE id = ?;",
                    {challanId, srcId}
                );
            } else if (vchType == "TCS_RECEIPT") {
                db.executeNonQuery(
                    "UPDATE tcs_receipt_vouchers SET is_deposited = 1, challan_id = ? WHERE id = ?;",
                    {challanId, srcId}
                );
            }
        }
    }

    db.commit();

    m_statusMessage = QString("%1 Deposit Challan %2 saved successfully!").arg(m_taxType, m_challanNo);
    m_isError = false;
    emit statusChanged();
    emit challanSaved(challanId);

    resetForm();
    return true;
}

bool TaxChallanController::loadChallan(int challanId)
{
    if (challanId <= 0) return false;
    DatabaseManager &db = DatabaseManager::instance();

    QVariantList rows = db.executeQuery(
        "SELECT * FROM tax_deposit_challans WHERE id = ? LIMIT 1;",
        {challanId}
    );
    if (rows.isEmpty()) return false;

    QVariantMap c = rows.first().toMap();
    m_editChallanId = challanId;
    m_taxType = c.value("tax_type").toString();
    m_challanNo = c.value("challan_no").toString();
    m_challanDate = c.value("challan_date").toString();
    m_postInBooks = c.value("post_in_books").toInt() == 1;
    m_periodFrom = c.value("period_from").toString();
    m_periodTo = c.value("period_to").toString();
    m_basicTax = c.value("basic_tax").toDouble();
    m_surcharge = c.value("surcharge").toDouble();
    m_cess = c.value("cess").toDouble();
    m_totalTax = c.value("total_tax").toDouble();
    m_interestAmount = c.value("interest_amount").toDouble();
    m_interestLedgerId = c.value("interest_ledger_id").toInt();
    m_penaltyAmount = c.value("penalty_amount").toDouble();
    m_penaltyLedgerId = c.value("penalty_ledger_id").toInt();
    m_otherAmount = c.value("other_amount").toDouble();
    m_otherLedgerId = c.value("other_ledger_id").toInt();
    m_totalChallanAmount = c.value("total_challan_amount").toDouble();
    m_bankLedgerId = c.value("bank_ledger_id").toInt();
    m_chequeNo = c.value("cheque_no").toString();
    m_chequeDate = c.value("cheque_date").toString();
    m_bsrCode = c.value("bsr_code").toString();
    m_minorHead = c.value("minor_head").toString();
    m_majorHead = c.value("major_head").toString();
    m_narration = c.value("narration").toString();

    emit taxTypeChanged();
    emit challanNoChanged();
    emit challanDateChanged();
    emit postInBooksChanged();
    emit periodChanged();
    emit totalsChanged();
    emit interestLedgerIdChanged();
    emit penaltyLedgerIdChanged();
    emit otherLedgerIdChanged();
    emit bankLedgerIdChanged();
    emit chequeNoChanged();
    emit chequeDateChanged();
    emit bsrCodeChanged();
    emit minorHeadChanged();
    emit majorHeadChanged();
    emit narrationChanged();

    return true;
}

bool TaxChallanController::deleteChallan(int challanId)
{
    if (challanId <= 0) return false;
    DatabaseManager &db = DatabaseManager::instance();
    db.beginTransaction();

    // 1. Release deposited vouchers
    QVariantList items = db.executeQuery(
        "SELECT voucher_type, source_voucher_id FROM tax_deposit_challan_items WHERE challan_id = ?;",
        {challanId}
    );
    for (const auto &it : items) {
        QVariantMap m = it.toMap();
        QString vType = m.value("voucher_type").toString();
        int srcId = m.value("source_voucher_id").toInt();
        if (vType == "TDS_VOUCHER") {
            db.executeNonQuery("UPDATE tds_vouchers SET is_deposited = 0, challan_id = NULL WHERE id = ?;", {srcId});
        } else if (vType == "ADVANCE_194Q") {
            db.executeNonQuery("UPDATE advance_payments_194q SET is_deposited = 0, challan_id = NULL WHERE id = ?;", {srcId});
        } else if (vType == "TCS_RECEIPT") {
            db.executeNonQuery("UPDATE tcs_receipt_vouchers SET is_deposited = 0, challan_id = NULL WHERE id = ?;", {srcId});
        }
    }

    // 2. Remove GL voucher if one was created
    QVariant vId = db.executeScalar("SELECT voucher_id FROM tax_deposit_challans WHERE id = ?;", {challanId});
    if (vId.isValid() && !vId.isNull() && vId.toInt() > 0) {
        int glId = vId.toInt();
        QString vNo = db.executeScalar("SELECT voucher_no FROM vouchers WHERE id = ?;", {glId}).toString();
        db.executeNonQuery("DELETE FROM transactions WHERE voucher_no = ?;", {vNo});
        db.executeNonQuery("DELETE FROM vouchers WHERE id = ?;", {glId});
    }

    // 3. Delete challan & items
    db.executeNonQuery("DELETE FROM tax_deposit_challan_items WHERE challan_id = ?;", {challanId});
    db.executeNonQuery("DELETE FROM tax_deposit_challans WHERE id = ?;", {challanId});

    db.commit();
    return true;
}

QVariantList TaxChallanController::getChallansList(const QString &typeFilter) const
{
    DatabaseManager &db = DatabaseManager::instance();
    QString sql =
        "SELECT c.id, c.tax_type, c.challan_no, c.challan_date, c.period_from, c.period_to, "
        "c.basic_tax, c.interest_amount, c.penalty_amount, c.other_amount, c.total_challan_amount, "
        "c.bsr_code, c.cheque_no, p.name as bank_name "
        "FROM tax_deposit_challans c "
        "LEFT JOIN parties p ON c.bank_ledger_id = p.id ";

    if (!typeFilter.isEmpty()) {
        sql += QString("WHERE c.tax_type = '%1' ").arg(typeFilter.toUpper());
    }
    sql += "ORDER BY c.challan_date DESC, c.id DESC;";

    return db.executeQuery(sql);
}
