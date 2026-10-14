#include "tcs_receipt_voucher_controller.h"
#include "../services/db_worker.h"
#include "../services/db_service.h"
#include "../services/financial_math_service.h"
#include "account_classifier.h"
#include "../database_manager.h"
#include <QDate>
#include <cmath>

TcsReceiptVoucherController::TcsReceiptVoucherController(QObject *parent)
    : QObject(parent)
{
    resetForm();
}

void TcsReceiptVoucherController::setReceiptNo(int no)
{
    if (m_receiptNo != no) {
        m_receiptNo = no;
        emit receiptNoChanged();
    }
}

void TcsReceiptVoucherController::setReceiptDate(const QString &d)
{
    if (m_receiptDate != d) {
        m_receiptDate = d;
        emit receiptDateChanged();
    }
}

void TcsReceiptVoucherController::setReceiptType(const QString &t)
{
    QString upper = t.trimmed().toUpper();
    if (upper != "BANK" && upper != "CASH") upper = "BANK";
    if (m_receiptType != upper) {
        m_receiptType = upper;
        emit receiptTypeChanged();
    }
}

void TcsReceiptVoucherController::setPostInBooks(bool b)
{
    if (m_postInBooks != b) {
        m_postInBooks = b;
        emit postInBooksChanged();
    }
}

void TcsReceiptVoucherController::setPartyId(int id)
{
    if (m_partyId != id) {
        m_partyId = id;
        DatabaseManager &db = DatabaseManager::instance();
        QVariantList p = db.executeQuery("SELECT name, pan FROM parties WHERE id = ? LIMIT 1;", {m_partyId});
        if (!p.isEmpty()) {
            QVariantMap m = p.first().toMap();
            m_partyName = m.value("name").toString();
            m_partyPan = m.value("pan").toString().trimmed().toUpper();
        } else {
            m_partyName.clear();
            m_partyPan.clear();
        }

        // PAN alert
        if (m_partyPan.isEmpty()) {
            m_statusMessage = "Note: Party does not have a PAN registered. Higher TCS rate may apply.";
            m_isError = false;
        } else {
            m_statusMessage.clear();
            m_isError = false;
        }

        emit partyIdChanged();
        emit partyNameChanged();
        emit partyPanChanged();
        emit statusChanged();
    }
}

void TcsReceiptVoucherController::setBankLedgerId(int id)
{
    if (m_bankLedgerId != id) {
        m_bankLedgerId = id;
        emit bankLedgerIdChanged();
    }
}

void TcsReceiptVoucherController::setBankAmount(double amt)
{
    m_bankAmount = std::max(0.0, amt);
    if (m_withoutTcsAmount == 0.0 && m_bankAmount > 0.0) {
        // Estimate without TCS
        m_withoutTcsAmount = FinancialMathService::round2((m_bankAmount / (1.0 + (m_tcsRate / 100.0))));
    }
    recalculate();
}

void TcsReceiptVoucherController::setWithoutTcsAmount(double amt)
{
    m_withoutTcsAmount = std::max(0.0, amt);
    recalculate();
}

void TcsReceiptVoucherController::setTcsRate(double r)
{
    m_tcsRate = std::max(0.0, r);
    recalculate();
}

void TcsReceiptVoucherController::setTcsReceived(bool b)
{
    if (m_tcsReceived != b) {
        m_tcsReceived = b;
        recalculate();
    }
}

void TcsReceiptVoucherController::setInterestReceived(double amt)
{
    m_interestReceived = std::max(0.0, amt);
    recalculate();
}

void TcsReceiptVoucherController::setInterestLedgerId(int id)
{
    if (m_interestLedgerId != id) {
        m_interestLedgerId = id;
        emit interestLedgerIdChanged();
    }
}

void TcsReceiptVoucherController::setDiscountAllowed(double amt)
{
    m_discountAllowed = std::max(0.0, amt);
    recalculate();
}

void TcsReceiptVoucherController::setDiscountLedgerId(int id)
{
    if (m_discountLedgerId != id) {
        m_discountLedgerId = id;
        emit discountLedgerIdChanged();
    }
}

void TcsReceiptVoucherController::setOtherAmount(double amt)
{
    m_otherAmount = amt;
    recalculate();
}

void TcsReceiptVoucherController::setOtherLedgerId(int id)
{
    if (m_otherLedgerId != id) {
        m_otherLedgerId = id;
        emit otherLedgerIdChanged();
    }
}

void TcsReceiptVoucherController::setNarration(const QString &n)
{
    if (m_narration != n) {
        m_narration = n;
        emit narrationChanged();
    }
}

void TcsReceiptVoucherController::resetForm()
{
    m_editVoucherId = 0;
    m_receiptNo = nextReceiptNumber();
    m_receiptDate = QDate::currentDate().toString("yyyy-MM-dd");
    m_receiptType = "BANK";
    m_postInBooks = true;

    m_partyId = 0;
    m_partyName.clear();
    m_partyPan.clear();

    m_bankAmount = 0.0;
    m_withoutTcsAmount = 0.0;
    m_tcsRate = 0.10;
    m_tcsAmount = 0.0;
    m_tcsReceived = true;
    m_netBankReceipt = 0.0;

    m_interestReceived = 0.0;
    m_discountAllowed = 0.0;
    m_otherAmount = 0.0;
    m_netCreditToParty = 0.0;

    m_narration = "Receipt with TCS u/s 206C(1H)";
    m_statusMessage.clear();
    m_isError = false;

    ensureDefaultLedgers();

    emit receiptNoChanged();
    emit receiptDateChanged();
    emit receiptTypeChanged();
    emit postInBooksChanged();
    emit partyIdChanged();
    emit partyNameChanged();
    emit partyPanChanged();
    emit totalsChanged();
    emit narrationChanged();
    emit statusChanged();
}

int TcsReceiptVoucherController::nextReceiptNumber() const
{
    DatabaseManager &db = DatabaseManager::instance();
    QVariant v = db.executeScalar("SELECT MAX(receipt_no) FROM tcs_receipt_vouchers;");
    return (v.isValid() && !v.isNull()) ? v.toInt() + 1 : 1;
}

void TcsReceiptVoucherController::ensureDefaultLedgers()
{
    DatabaseManager &db = DatabaseManager::instance();

    if (m_bankLedgerId <= 0) {
        QString sql = QString(
            "SELECT p.id FROM parties p %1 WHERE %2 OR p.party_type = 'Bank' ORDER BY p.id ASC LIMIT 1;"
        ).arg(AccountClassifier::partiesJoinClause("p", "g"),
             AccountClassifier::generateHierarchySqlClause(AccountClassifier::getBankGroupCodes(), "g"));
        QVariant bId = db.executeScalar(sql);
        if (bId.isValid() && !bId.isNull()) {
            m_bankLedgerId = bId.toInt();
            emit bankLedgerIdChanged();
        }
    }

    if (m_discountLedgerId <= 0) {
        m_discountLedgerId = AccountClassifier::getStandardLedgerId(StandardLedgerCode::DiscountAllowed);
        if (m_discountLedgerId <= 0) {
            m_discountLedgerId = AccountClassifier::getStandardLedgerId(StandardLedgerCode::Discount);
        }
        if (m_discountLedgerId <= 0) {
            QVariant dId = db.executeScalar("SELECT id FROM parties WHERE name LIKE '%Discount%' LIMIT 1;");
            if (dId.isValid() && !dId.isNull()) {
                m_discountLedgerId = dId.toInt();
            }
        }
        if (m_discountLedgerId <= 0) {
            db.executeNonQuery("INSERT INTO parties (name, group_name, group_code, party_type, legacy_id) VALUES ('Discount Allowed A/c', 'Expenditure A/c', 17, 'Expense', 954);");
            m_discountLedgerId = db.lastInsertedId();
        }
        emit discountLedgerIdChanged();
    }

    if (m_interestLedgerId <= 0) {
        m_interestLedgerId = AccountClassifier::getStandardLedgerId(StandardLedgerCode::InterestReceived);
        if (m_interestLedgerId <= 0) {
            m_interestLedgerId = AccountClassifier::getStandardLedgerId(StandardLedgerCode::Interest);
        }
        if (m_interestLedgerId <= 0) {
            QVariant iId = db.executeScalar("SELECT id FROM parties WHERE name LIKE '%Interest%' LIMIT 1;");
            if (iId.isValid() && !iId.isNull()) {
                m_interestLedgerId = iId.toInt();
            }
        }
        if (m_interestLedgerId <= 0) {
            db.executeNonQuery("INSERT INTO parties (name, group_name, group_code, party_type, legacy_id) VALUES ('Interest Received A/c', 'Income A/c', 16, 'Income', 955);");
            m_interestLedgerId = db.lastInsertedId();
        }
        emit interestLedgerIdChanged();
    }
}

void TcsReceiptVoucherController::recalculate()
{
    // Calculate TCS Amount
    m_tcsAmount = FinancialMathService::round2((m_withoutTcsAmount * (m_tcsRate / 100.0)));

    // Net Bank Receipt
    if (m_tcsReceived) {
        m_netBankReceipt = m_withoutTcsAmount + m_tcsAmount;
    } else {
        m_netBankReceipt = m_withoutTcsAmount;
    }
    m_bankAmount = m_netBankReceipt;

    // Net Amount Credit to Party = Without TCS + Discount Allowed - Interest Received + Other
    m_netCreditToParty = m_withoutTcsAmount + m_discountAllowed - m_interestReceived + m_otherAmount;
    m_netCreditToParty = FinancialMathService::round2(m_netCreditToParty);

    emit totalsChanged();
}

namespace {

// Snapshot keys (see buildPayload): receipt_no, receipt_date, receipt_type,
// post_in_books, party_id, party_name, bank_ledger_id, bank_amount,
// without_tcs, tcs_rate, tcs_amount, tds_received->tcs_received,
// net_bank_receipt, interest_received, interest_ledger_id, discount_allowed,
// discount_ledger_id, other_amount, other_ledger_id, net_credit, narration.
bool saveTcsIn(MahadevERP::DbTx& tx, const QVariantMap& p, int& outReceiptId) {
    outReceiptId = 0;
    const int receiptNo = p.value("receipt_no").toInt();
    const QString receiptDate = p.value("receipt_date").toString();
    const QString receiptType = p.value("receipt_type").toString();
    const bool postInBooks = p.value("post_in_books").toBool();
    const int partyId = p.value("party_id").toInt();
    const QString partyName = p.value("party_name").toString();
    const int bankLedgerId = p.value("bank_ledger_id").toInt();
    const double bankAmount = p.value("bank_amount").toDouble();
    const double withoutTcs = p.value("without_tcs").toDouble();
    const double tcsRate = p.value("tcs_rate").toDouble();
    const double tcsAmount = p.value("tcs_amount").toDouble();
    const bool tcsReceived = p.value("tcs_received").toBool();
    const double netBankReceipt = p.value("net_bank_receipt").toDouble();
    const double interestReceived = p.value("interest_received").toDouble();
    const int interestLedgerId = p.value("interest_ledger_id").toInt();
    const double discountAllowed = p.value("discount_allowed").toDouble();
    const int discountLedgerId = p.value("discount_ledger_id").toInt();
    const double otherAmount = p.value("other_amount").toDouble();
    const int otherLedgerId = p.value("other_ledger_id").toInt();
    const double netCredit = p.value("net_credit").toDouble();
    const QString narration = p.value("narration").toString();

    // FY in-transaction (legacy hardcoded a "FY 2025-26" label fallback).
    int fyId = 1;
    QString fyLabel;
    QVariantList fyRows = tx.query(
        "SELECT id, year_name FROM financial_years WHERE is_active = 1 LIMIT 1;", {});
    if (fyRows.isEmpty()) {
        fyRows = tx.query(
            "SELECT id, year_name FROM financial_years ORDER BY id DESC LIMIT 1;", {});
    }
    if (!fyRows.isEmpty()) {
        fyId = fyRows.first().toMap().value("id", 1).toInt();
        fyLabel = fyRows.first().toMap().value("year_name").toString();
    }

    int glVoucherId = 0;

    // 1. Post to General Ledger (Double-Entry)
    if (postInBooks) {
        const int tcsPayableId = tx.resolvePartyId(
            0, "TCS Payable A/c", "Duties & Taxes (GST)", "Tax");
        if (tcsPayableId <= 0) return false;

        const QString bankName = tx.scalar(
            "SELECT name FROM parties WHERE id = ?;", {bankLedgerId}).toString();
        const QString vchNo = QString("TCS-%1").arg(receiptNo, 4, 10, QChar('0'));
        const QString vchNarration = narration.isEmpty()
            ? QString("Receipt with TCS from %1").arg(partyName)
            : narration;

        // Vouchers Header
        if (!tx.exec(
                "INSERT INTO vouchers ("
                "fy_id, financial_year, voucher_no, voucher_date, voucher_type, legacy_type, "
                "party_id, ledger_id, party_name, account_type, amount, narration"
                ") VALUES (?, ?, ?, ?, 'RECEIPT', ?, ?, ?, ?, 'Dr', ?, ?);",
                {fyId, fyLabel, vchNo, receiptDate, receiptType,
                 partyId, partyId, partyName, netBankReceipt, vchNarration})) {
            return false;
        }
        glVoucherId = static_cast<int>(tx.lastInsertId());

        int rowNo = 1;

        // Leg 1: Debit Bank / Cash (Net Receipt)
        tx.execUnchecked(
            "INSERT INTO transactions ("
            "fy_id, financial_year, voucher_no, voucher_date, voucher_type, trans_type, "
            "account_code, party_id, party_name, opposing_account, dr_cr, amount, "
            "narration, taxable_amount, tds_amount, tds_rate, row_no"
            ") VALUES (?, ?, ?, ?, 'RECEIPT', ?, ?, ?, ?, ?, 'Dr', ?, ?, ?, ?, ?, ?);",
            {fyId, fyLabel, vchNo, receiptDate, receiptType,
             bankLedgerId, bankLedgerId, bankName, partyName, netBankReceipt,
             vchNarration, withoutTcs, tcsAmount, tcsRate, rowNo++});

        // Leg 2: Debit Discount Allowed (if any)
        if (discountAllowed > 0.0 && discountLedgerId > 0) {
            const QString discName = tx.scalar(
                "SELECT name FROM parties WHERE id = ?;", {discountLedgerId}).toString();
            tx.execUnchecked(
                "INSERT INTO transactions ("
                "fy_id, financial_year, voucher_no, voucher_date, voucher_type, trans_type, "
                "account_code, party_id, party_name, opposing_account, dr_cr, amount, "
                "narration, taxable_amount, tds_amount, tds_rate, row_no"
                ") VALUES (?, ?, ?, ?, 'RECEIPT', ?, ?, ?, ?, ?, 'Dr', ?, ?, 0, 0, 0, ?);",
                {fyId, fyLabel, vchNo, receiptDate, receiptType,
                 discountLedgerId, discountLedgerId, discName, partyName, discountAllowed,
                 "Cash Discount Allowed", rowNo++});
        }

        // Leg 3: Credit Party Account (Net Credit)
        tx.execUnchecked(
            "INSERT INTO transactions ("
            "fy_id, financial_year, voucher_no, voucher_date, voucher_type, trans_type, "
            "account_code, party_id, party_name, opposing_account, dr_cr, amount, "
            "narration, taxable_amount, tds_amount, tds_rate, row_no"
            ") VALUES (?, ?, ?, ?, 'RECEIPT', ?, ?, ?, ?, ?, 'Cr', ?, ?, ?, 0, 0, ?);",
            {fyId, fyLabel, vchNo, receiptDate, receiptType,
             partyId, partyId, partyName, bankName, netCredit,
             vchNarration, withoutTcs, rowNo++});

        // Leg 4: Credit TCS Payable A/c
        if (tcsAmount > 0.0) {
            tx.execUnchecked(
                "INSERT INTO transactions ("
                "fy_id, financial_year, voucher_no, voucher_date, voucher_type, trans_type, "
                "account_code, party_id, party_name, opposing_account, dr_cr, amount, "
                "narration, taxable_amount, tds_amount, tds_rate, row_no"
                ") VALUES (?, ?, ?, ?, 'RECEIPT', ?, ?, ?, ?, ?, 'Cr', ?, ?, ?, ?, ?, ?);",
                {fyId, fyLabel, vchNo, receiptDate, receiptType,
                 tcsPayableId, tcsPayableId, "TCS Payable A/c", partyName, tcsAmount,
                 "TCS u/s 206C(1H) Payable", withoutTcs, tcsAmount, tcsRate, rowNo++});
        }

        // Leg 5: Credit Interest Received (if any)
        if (interestReceived > 0.0 && interestLedgerId > 0) {
            const QString intName = tx.scalar(
                "SELECT name FROM parties WHERE id = ?;", {interestLedgerId}).toString();
            tx.execUnchecked(
                "INSERT INTO transactions ("
                "fy_id, financial_year, voucher_no, voucher_date, voucher_type, trans_type, "
                "account_code, party_id, party_name, opposing_account, dr_cr, amount, "
                "narration, taxable_amount, tds_amount, tds_rate, row_no"
                ") VALUES (?, ?, ?, ?, 'RECEIPT', ?, ?, ?, ?, ?, 'Cr', ?, ?, 0, 0, 0, ?);",
                {fyId, fyLabel, vchNo, receiptDate, receiptType,
                 interestLedgerId, interestLedgerId, intName, partyName, interestReceived,
                 "Interest on Overdue Payment", rowNo++});
        }

        // Leg 6: Other Adjustments (if any)
        if (otherAmount != 0.0 && otherLedgerId > 0) {
            const QString othName = tx.scalar(
                "SELECT name FROM parties WHERE id = ?;", {otherLedgerId}).toString();
            const QString drCr = otherAmount > 0.0 ? "Cr" : "Dr";
            const double absAmt = std::abs(otherAmount);
            tx.execUnchecked(
                "INSERT INTO transactions ("
                "fy_id, financial_year, voucher_no, voucher_date, voucher_type, trans_type, "
                "account_code, party_id, party_name, opposing_account, dr_cr, amount, "
                "narration, taxable_amount, tds_amount, tds_rate, row_no"
                ") VALUES (?, ?, ?, ?, 'RECEIPT', ?, ?, ?, ?, ?, ?, ?, ?, 0, 0, 0, ?);",
                {fyId, fyLabel, vchNo, receiptDate, receiptType,
                 otherLedgerId, otherLedgerId, othName, partyName, drCr, absAmt,
                 "Other Adjustments", rowNo++});
        }
    }

    // 2. Insert into tcs_receipt_vouchers
    const int tcsPayableId = tx.scalar(
        "SELECT id FROM parties WHERE name = 'TCS Payable A/c' LIMIT 1;").toInt();

    if (!tx.exec(
            "INSERT INTO tcs_receipt_vouchers ("
            "fy_id, receipt_no, receipt_date, receipt_type, post_in_books, party_id, bank_ledger_id, "
            "bank_amount, without_tcs_amount, tcs_rate, tcs_amount, tcs_received, net_bank_receipt, "
            "interest_received, interest_ledger_id, discount_allowed, discount_ledger_id, "
            "other_amount, other_ledger_id, net_credit_to_party, tcs_payable_ledger_id, "
            "is_deposited, voucher_id, narration"
            ") VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, 0, ?, ?);",
            {fyId, receiptNo, receiptDate, receiptType, postInBooks ? 1 : 0,
             partyId, bankLedgerId, bankAmount, withoutTcs, tcsRate, tcsAmount,
             tcsReceived ? 1 : 0, netBankReceipt, interestReceived,
             interestLedgerId > 0 ? interestLedgerId : QVariant(),
             discountAllowed,
             discountLedgerId > 0 ? discountLedgerId : QVariant(),
             otherAmount,
             otherLedgerId > 0 ? otherLedgerId : QVariant(),
             netCredit,
             tcsPayableId > 0 ? tcsPayableId : QVariant(),
             glVoucherId > 0 ? glVoucherId : QVariant(),
             narration})) {
        return false;
    }

    outReceiptId = static_cast<int>(tx.lastInsertId());
    return true;
}

bool deleteTcsIn(MahadevERP::DbTx& tx, int voucherId) {
    if (voucherId <= 0) return false;
    const QVariant vId = tx.scalar(
        "SELECT voucher_id FROM tcs_receipt_vouchers WHERE id = ?;", {voucherId});
    if (vId.isValid() && !vId.isNull() && vId.toInt() > 0) {
        const int glId = vId.toInt();
        const QString vNo = tx.scalar(
            "SELECT voucher_no FROM vouchers WHERE id = ?;", {glId}).toString();
        tx.execUnchecked("DELETE FROM transactions WHERE voucher_no = ?;", {vNo});
        tx.execUnchecked("DELETE FROM vouchers WHERE id = ?;", {glId});
    }
    return tx.exec("DELETE FROM tcs_receipt_vouchers WHERE id = ?;", {voucherId});
}

} // namespace

QVariantMap TcsReceiptVoucherController::buildPayload() const {
    QVariantMap p;
    p["receipt_no"] = m_receiptNo;
    p["receipt_date"] = m_receiptDate;
    p["receipt_type"] = m_receiptType;
    p["post_in_books"] = m_postInBooks;
    p["party_id"] = m_partyId;
    p["party_name"] = m_partyName;
    p["bank_ledger_id"] = m_bankLedgerId;
    p["bank_amount"] = m_bankAmount;
    p["without_tcs"] = m_withoutTcsAmount;
    p["tcs_rate"] = m_tcsRate;
    p["tcs_amount"] = m_tcsAmount;
    p["tcs_received"] = m_tcsReceived;
    p["net_bank_receipt"] = m_netBankReceipt;
    p["interest_received"] = m_interestReceived;
    p["interest_ledger_id"] = m_interestLedgerId;
    p["discount_allowed"] = m_discountAllowed;
    p["discount_ledger_id"] = m_discountLedgerId;
    p["other_amount"] = m_otherAmount;
    p["other_ledger_id"] = m_otherLedgerId;
    p["net_credit"] = m_netCreditToParty;
    p["narration"] = m_narration;
    return p;
}

QString TcsReceiptVoucherController::validateForSave() const {
    if (m_partyId <= 0) return "Please select a Party / Customer.";
    if (m_bankLedgerId <= 0) return "Please select a Bank / Cash Account.";
    if (m_withoutTcsAmount <= 0.0 && m_bankAmount <= 0.0) {
        return "Receipt Amount must be greater than zero.";
    }
    return "";
}

QFuture<bool> TcsReceiptVoucherController::saveVoucherAsync() {
    if (!validateForSave().isEmpty()) return MahadevERP::DbService::refused<bool>();
    const QVariantMap payload = buildPayload();
    return MahadevERP::DbService::instance().transact(
        [payload](MahadevERP::DbTx& tx) {
            int outId = 0;
            return saveTcsIn(tx, payload, outId);
        });
}

QFuture<bool> TcsReceiptVoucherController::deleteVoucherAsync(int voucherId) {
    if (voucherId <= 0) return MahadevERP::DbService::refused<bool>();
    return MahadevERP::DbService::instance().transact(
        [voucherId](MahadevERP::DbTx& tx) {
            return deleteTcsIn(tx, voucherId);
        });
}

bool TcsReceiptVoucherController::saveVoucher()
{
    m_isError = false;
    m_statusMessage.clear();

    const QString err = validateForSave();
    if (!err.isEmpty()) {
        m_isError = true;
        m_statusMessage = err;
        emit statusChanged();
        return false;
    }

    DatabaseManager &db = DatabaseManager::instance();
    db.beginTransaction();
    int receiptId = 0;
    MahadevERP::DbTx tx(db.getConnection());
    const bool ok = saveTcsIn(tx, buildPayload(), receiptId);
    if (!ok) {
        db.rollback();
        m_isError = true;
        m_statusMessage = "Failed to save TCS Receipt Voucher.";
        emit statusChanged();
        return false;
    }
    db.commit();

    m_statusMessage = QString("TCS Receipt Voucher #%1 saved successfully!").arg(m_receiptNo);
    m_isError = false;
    emit statusChanged();
    emit voucherSaved(receiptId);

    resetForm();
    return true;
}

bool TcsReceiptVoucherController::deleteVoucher(int voucherId)
{
    if (voucherId <= 0) return false;
    DatabaseManager &db = DatabaseManager::instance();
    db.beginTransaction();
    MahadevERP::DbTx tx(db.getConnection());
    const bool ok = deleteTcsIn(tx, voucherId);
    if (ok) db.commit();
    else db.rollback();
    return ok;
}

bool TcsReceiptVoucherController::loadVoucher(int voucherId)
{
    if (voucherId <= 0) return false;
    DatabaseManager &db = DatabaseManager::instance();

    QVariantList rows = db.executeQuery("SELECT * FROM tcs_receipt_vouchers WHERE id = ? LIMIT 1;", {voucherId});
    if (rows.isEmpty()) return false;

    QVariantMap r = rows.first().toMap();
    m_editVoucherId = voucherId;
    m_receiptNo = r.value("receipt_no").toInt();
    m_receiptDate = r.value("receipt_date").toString();
    m_receiptType = r.value("receipt_type").toString();
    m_postInBooks = r.value("post_in_books").toInt() == 1;
    m_bankLedgerId = r.value("bank_ledger_id").toInt();
    m_bankAmount = r.value("bank_amount").toDouble();
    m_withoutTcsAmount = r.value("without_tcs_amount").toDouble();
    m_tcsRate = r.value("tcs_rate").toDouble();
    m_tcsAmount = r.value("tcs_amount").toDouble();
    m_tcsReceived = r.value("tcs_received").toInt() == 1;
    m_netBankReceipt = r.value("net_bank_receipt").toDouble();
    m_interestReceived = r.value("interest_received").toDouble();
    m_interestLedgerId = r.value("interest_ledger_id").toInt();
    m_discountAllowed = r.value("discount_allowed").toDouble();
    m_discountLedgerId = r.value("discount_ledger_id").toInt();
    m_otherAmount = r.value("other_amount").toDouble();
    m_otherLedgerId = r.value("other_ledger_id").toInt();
    m_netCreditToParty = r.value("net_credit_to_party").toDouble();
    m_narration = r.value("narration").toString();

    setPartyId(r.value("party_id").toInt());

    emit receiptNoChanged();
    emit receiptDateChanged();
    emit receiptTypeChanged();
    emit postInBooksChanged();
    emit bankLedgerIdChanged();
    emit totalsChanged();
    emit narrationChanged();

    return true;
}

QString TcsReceiptVoucherController::receiptListSql() {
    return "SELECT t.id, t.receipt_no, t.receipt_date, t.receipt_type, p.name as party_name, "
        "b.name as bank_name, t.without_tcs_amount, t.tcs_amount, t.net_bank_receipt, "
        "t.net_credit_to_party, t.is_deposited "
        "FROM tcs_receipt_vouchers t "
        "LEFT JOIN parties p ON t.party_id = p.id "
        "LEFT JOIN parties b ON t.bank_ledger_id = b.id "
        "ORDER BY t.receipt_date DESC, t.receipt_no DESC;";
}

QVariantList TcsReceiptVoucherController::getReceiptVouchersList() const
{
    DatabaseManager &db = DatabaseManager::instance();
    return db.executeQuery(receiptListSql());
}

QFuture<QVariantList> TcsReceiptVoucherController::getReceiptVouchersListAsync() const {
    return MahadevERP::DbService::instance().query(receiptListSql(), {});
}
