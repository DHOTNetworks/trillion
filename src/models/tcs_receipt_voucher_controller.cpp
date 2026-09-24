#include "tcs_receipt_voucher_controller.h"
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
        m_withoutTcsAmount = std::round((m_bankAmount / (1.0 + (m_tcsRate / 100.0))) * 100.0) / 100.0;
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
        QVariant bId = db.executeScalar("SELECT id FROM parties WHERE group_name LIKE '%Bank%' OR party_type = 'Bank' LIMIT 1;");
        if (bId.isValid() && !bId.isNull()) {
            m_bankLedgerId = bId.toInt();
            emit bankLedgerIdChanged();
        }
    }

    if (m_discountLedgerId <= 0) {
        QVariant dId = db.executeScalar("SELECT id FROM parties WHERE name = 'Discount Allowed A/c' LIMIT 1;");
        if (dId.isValid() && !dId.isNull()) {
            m_discountLedgerId = dId.toInt();
            emit discountLedgerIdChanged();
        }
    }

    if (m_interestLedgerId <= 0) {
        QVariant iId = db.executeScalar("SELECT id FROM parties WHERE name = 'Interest Received A/c' LIMIT 1;");
        if (iId.isValid() && !iId.isNull()) {
            m_interestLedgerId = iId.toInt();
            emit interestLedgerIdChanged();
        }
    }
}

void TcsReceiptVoucherController::recalculate()
{
    // Calculate TCS Amount
    m_tcsAmount = std::round((m_withoutTcsAmount * (m_tcsRate / 100.0)) * 100.0) / 100.0;

    // Net Bank Receipt
    if (m_tcsReceived) {
        m_netBankReceipt = m_withoutTcsAmount + m_tcsAmount;
    } else {
        m_netBankReceipt = m_withoutTcsAmount;
    }
    m_bankAmount = m_netBankReceipt;

    // Net Amount Credit to Party = Without TCS + Discount Allowed - Interest Received + Other
    m_netCreditToParty = m_withoutTcsAmount + m_discountAllowed - m_interestReceived + m_otherAmount;
    m_netCreditToParty = std::round(m_netCreditToParty * 100.0) / 100.0;

    emit totalsChanged();
}

bool TcsReceiptVoucherController::saveVoucher()
{
    m_isError = false;
    m_statusMessage.clear();

    if (m_partyId <= 0) {
        m_isError = true;
        m_statusMessage = "Please select a Party / Customer.";
        emit statusChanged();
        return false;
    }

    if (m_bankLedgerId <= 0) {
        m_isError = true;
        m_statusMessage = "Please select a Bank / Cash Account.";
        emit statusChanged();
        return false;
    }

    if (m_withoutTcsAmount <= 0.0 && m_bankAmount <= 0.0) {
        m_isError = true;
        m_statusMessage = "Receipt Amount must be greater than zero.";
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

    // 1. Post to General Ledger (Double-Entry)
    if (m_postInBooks) {
        // Resolve TCS Payable A/c
        int tcsPayableId = 0;
        QVariant tcsRow = db.executeScalar("SELECT id FROM parties WHERE name = 'TCS Payable A/c' LIMIT 1;");
        if (tcsRow.isValid() && !tcsRow.isNull()) {
            tcsPayableId = tcsRow.toInt();
        } else {
            db.executeNonQuery(
                "INSERT INTO parties (name, group_name, party_type) VALUES ('TCS Payable A/c', 'Duties & Taxes (GST)', 'Tax');"
            );
            tcsPayableId = db.executeScalar("SELECT last_insert_rowid();").toInt();
        }

        QString bankName = db.executeScalar("SELECT name FROM parties WHERE id = ?;", {m_bankLedgerId}).toString();
        QString vchNo = QString("TCS-%1").arg(m_receiptNo, 4, 10, QChar('0'));
        QString vchNarration = m_narration.isEmpty()
            ? QString("Receipt with TCS from %1").arg(m_partyName)
            : m_narration;

        // Vouchers Header
        db.executeNonQuery(
            "INSERT INTO vouchers ("
            "fy_id, financial_year, voucher_no, voucher_date, voucher_type, legacy_type, "
            "party_id, ledger_id, party_name, account_type, amount, narration"
            ") VALUES (?, ?, ?, ?, 'RECEIPT', ?, ?, ?, ?, 'Dr', ?, ?);",
            {
                fyId, fyLabel, vchNo, m_receiptDate, m_receiptType,
                m_partyId, m_partyId, m_partyName, m_netBankReceipt,
                vchNarration
            }
        );
        glVoucherId = db.executeScalar("SELECT last_insert_rowid();").toInt();

        int rowNo = 1;

        // Leg 1: Debit Bank / Cash (Net Receipt)
        db.executeNonQuery(
            "INSERT INTO transactions ("
            "fy_id, financial_year, voucher_no, voucher_date, voucher_type, trans_type, "
            "account_code, party_id, party_name, opposing_account, dr_cr, amount, "
            "narration, taxable_amount, tds_amount, tds_rate, row_no"
            ") VALUES (?, ?, ?, ?, 'RECEIPT', ?, ?, ?, ?, ?, 'Dr', ?, ?, ?, ?, ?, ?);",
            {
                fyId, fyLabel, vchNo, m_receiptDate, m_receiptType,
                m_bankLedgerId, m_bankLedgerId, bankName, m_partyName, m_netBankReceipt,
                vchNarration, m_withoutTcsAmount, m_tcsAmount, m_tcsRate, rowNo++
            }
        );

        // Leg 2: Debit Discount Allowed (if any)
        if (m_discountAllowed > 0.0 && m_discountLedgerId > 0) {
            QString discName = db.executeScalar("SELECT name FROM parties WHERE id = ?;", {m_discountLedgerId}).toString();
            db.executeNonQuery(
                "INSERT INTO transactions ("
                "fy_id, financial_year, voucher_no, voucher_date, voucher_type, trans_type, "
                "account_code, party_id, party_name, opposing_account, dr_cr, amount, "
                "narration, taxable_amount, tds_amount, tds_rate, row_no"
                ") VALUES (?, ?, ?, ?, 'RECEIPT', ?, ?, ?, ?, ?, 'Dr', ?, ?, 0, 0, 0, ?);",
                {
                    fyId, fyLabel, vchNo, m_receiptDate, m_receiptType,
                    m_discountLedgerId, m_discountLedgerId, discName, m_partyName, m_discountAllowed,
                    "Cash Discount Allowed", rowNo++
                }
            );
        }

        // Leg 3: Credit Party Account (Net Credit)
        db.executeNonQuery(
            "INSERT INTO transactions ("
            "fy_id, financial_year, voucher_no, voucher_date, voucher_type, trans_type, "
            "account_code, party_id, party_name, opposing_account, dr_cr, amount, "
            "narration, taxable_amount, tds_amount, tds_rate, row_no"
            ") VALUES (?, ?, ?, ?, 'RECEIPT', ?, ?, ?, ?, ?, 'Cr', ?, ?, ?, 0, 0, ?);",
            {
                fyId, fyLabel, vchNo, m_receiptDate, m_receiptType,
                m_partyId, m_partyId, m_partyName, bankName, m_netCreditToParty,
                vchNarration, m_withoutTcsAmount, rowNo++
            }
        );

        // Leg 4: Credit TCS Payable A/c
        if (m_tcsAmount > 0.0) {
            db.executeNonQuery(
                "INSERT INTO transactions ("
                "fy_id, financial_year, voucher_no, voucher_date, voucher_type, trans_type, "
                "account_code, party_id, party_name, opposing_account, dr_cr, amount, "
                "narration, taxable_amount, tds_amount, tds_rate, row_no"
                ") VALUES (?, ?, ?, ?, 'RECEIPT', ?, ?, ?, ?, ?, 'Cr', ?, ?, ?, ?, ?, ?);",
                {
                    fyId, fyLabel, vchNo, m_receiptDate, m_receiptType,
                    tcsPayableId, tcsPayableId, "TCS Payable A/c", m_partyName, m_tcsAmount,
                    "TCS u/s 206C(1H) Payable", m_withoutTcsAmount, m_tcsAmount, m_tcsRate, rowNo++
                }
            );
        }

        // Leg 5: Credit Interest Received (if any)
        if (m_interestReceived > 0.0 && m_interestLedgerId > 0) {
            QString intName = db.executeScalar("SELECT name FROM parties WHERE id = ?;", {m_interestLedgerId}).toString();
            db.executeNonQuery(
                "INSERT INTO transactions ("
                "fy_id, financial_year, voucher_no, voucher_date, voucher_type, trans_type, "
                "account_code, party_id, party_name, opposing_account, dr_cr, amount, "
                "narration, taxable_amount, tds_amount, tds_rate, row_no"
                ") VALUES (?, ?, ?, ?, 'RECEIPT', ?, ?, ?, ?, ?, 'Cr', ?, ?, 0, 0, 0, ?);",
                {
                    fyId, fyLabel, vchNo, m_receiptDate, m_receiptType,
                    m_interestLedgerId, m_interestLedgerId, intName, m_partyName, m_interestReceived,
                    "Interest on Overdue Payment", rowNo++
                }
            );
        }

        // Leg 6: Other Adjustments (if any)
        if (m_otherAmount != 0.0 && m_otherLedgerId > 0) {
            QString othName = db.executeScalar("SELECT name FROM parties WHERE id = ?;", {m_otherLedgerId}).toString();
            QString drCr = m_otherAmount > 0.0 ? "Cr" : "Dr";
            double absAmt = std::abs(m_otherAmount);
            db.executeNonQuery(
                "INSERT INTO transactions ("
                "fy_id, financial_year, voucher_no, voucher_date, voucher_type, trans_type, "
                "account_code, party_id, party_name, opposing_account, dr_cr, amount, "
                "narration, taxable_amount, tds_amount, tds_rate, row_no"
                ") VALUES (?, ?, ?, ?, 'RECEIPT', ?, ?, ?, ?, ?, ?, ?, ?, 0, 0, 0, ?);",
                {
                    fyId, fyLabel, vchNo, m_receiptDate, m_receiptType,
                    m_otherLedgerId, m_otherLedgerId, othName, m_partyName, drCr, absAmt,
                    "Other Adjustments", rowNo++
                }
            );
        }
    }

    // 2. Insert into tcs_receipt_vouchers
    int tcsPayableId = db.executeScalar("SELECT id FROM parties WHERE name = 'TCS Payable A/c' LIMIT 1;").toInt();

    bool ok = db.executeNonQuery(
        "INSERT INTO tcs_receipt_vouchers ("
        "fy_id, receipt_no, receipt_date, receipt_type, post_in_books, party_id, bank_ledger_id, "
        "bank_amount, without_tcs_amount, tcs_rate, tcs_amount, tcs_received, net_bank_receipt, "
        "interest_received, interest_ledger_id, discount_allowed, discount_ledger_id, "
        "other_amount, other_ledger_id, net_credit_to_party, tcs_payable_ledger_id, "
        "is_deposited, voucher_id, narration"
        ") VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, 0, ?, ?);",
        {
            fyId, m_receiptNo, m_receiptDate, m_receiptType, m_postInBooks ? 1 : 0,
            m_partyId, m_bankLedgerId, m_bankAmount, m_withoutTcsAmount, m_tcsRate, m_tcsAmount,
            m_tcsReceived ? 1 : 0, m_netBankReceipt, m_interestReceived,
            m_interestLedgerId > 0 ? m_interestLedgerId : QVariant(),
            m_discountAllowed,
            m_discountLedgerId > 0 ? m_discountLedgerId : QVariant(),
            m_otherAmount,
            m_otherLedgerId > 0 ? m_otherLedgerId : QVariant(),
            m_netCreditToParty,
            tcsPayableId > 0 ? tcsPayableId : QVariant(),
            glVoucherId > 0 ? glVoucherId : QVariant(),
            m_narration
        }
    );

    if (!ok) {
        db.rollback();
        m_isError = true;
        m_statusMessage = "Failed to save TCS Receipt Voucher.";
        emit statusChanged();
        return false;
    }

    int receiptId = db.executeScalar("SELECT last_insert_rowid();").toInt();
    db.commit();

    m_statusMessage = QString("TCS Receipt Voucher #%1 saved successfully!").arg(m_receiptNo);
    m_isError = false;
    emit statusChanged();
    emit voucherSaved(receiptId);

    resetForm();
    return true;
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

bool TcsReceiptVoucherController::deleteVoucher(int voucherId)
{
    if (voucherId <= 0) return false;
    DatabaseManager &db = DatabaseManager::instance();
    db.beginTransaction();

    QVariant vId = db.executeScalar("SELECT voucher_id FROM tcs_receipt_vouchers WHERE id = ?;", {voucherId});
    if (vId.isValid() && !vId.isNull() && vId.toInt() > 0) {
        int glId = vId.toInt();
        QString vNo = db.executeScalar("SELECT voucher_no FROM vouchers WHERE id = ?;", {glId}).toString();
        db.executeNonQuery("DELETE FROM transactions WHERE voucher_no = ?;", {vNo});
        db.executeNonQuery("DELETE FROM vouchers WHERE id = ?;", {glId});
    }

    db.executeNonQuery("DELETE FROM tcs_receipt_vouchers WHERE id = ?;", {voucherId});
    db.commit();
    return true;
}

QVariantList TcsReceiptVoucherController::getReceiptVouchersList() const
{
    DatabaseManager &db = DatabaseManager::instance();
    return db.executeQuery(
        "SELECT t.id, t.receipt_no, t.receipt_date, t.receipt_type, p.name as party_name, "
        "b.name as bank_name, t.without_tcs_amount, t.tcs_amount, t.net_bank_receipt, "
        "t.net_credit_to_party, t.is_deposited "
        "FROM tcs_receipt_vouchers t "
        "LEFT JOIN parties p ON t.party_id = p.id "
        "LEFT JOIN parties b ON t.bank_ledger_id = b.id "
        "ORDER BY t.receipt_date DESC, t.receipt_no DESC;"
    );
}
