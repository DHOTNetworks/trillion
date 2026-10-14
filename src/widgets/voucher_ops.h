#pragma once

// Canonical voucher-type router + deleter — the ONLY place that maps a
// statement row to its voucher kind, target view, and owning-model delete.
//
// Replaces the hand-rolled type switches in ledger_statement_widget
// (openVoucherForEntry) and day_book_widget (onRowDoubleClicked) for DELETE
// flows; the alter paths delegate their kind/view resolution here too, so
// Enter-to-alter and Alt+D-to-delete can never disagree (Doc 12 §5).
//
// Identity: sales/purchase rows carry their invoice-table id; daybook/ledger
// rows carry voucher_no (+date); lookups reuse the same model getters the
// alter path uses. Money deletes enforce FY-lock + audit inside the models
// (Doc 05) — identical guarantees to deleting from the voucher screen.

#include <QString>
#include <QVariantMap>
#include <QWidget>

#include "custom_dialogs.h"
#include "../database_manager.h"
#include "../models/sales_model.h"
#include "../models/purchase_model.h"
#include "../models/tds_model.h"
#include "../models/tds_voucher_controller.h"
#include "../models/jform_model.h"
#include "../models/iform_model.h"
#include "../models/vouchers_model.h"
#include "../models/debit_credit_note_controller.h"

namespace VoucherOps {

enum class Kind {
    Unknown,
    Sale,       // view 14
    Purchase,   // view 15
    Tds,        // view 24
    JForm,      // view 18
    IForm,      // view 19
    Cash,       // view 60 (PYMT/RCPT)
    Cheque,     // view 16 (CHPT/CHRT/BANK)
    Journal,    // view 17
    Milling,    // view 31 (no delete backend — see deleteEntry)
    DebitNote,  // view 28
};

/// Union of the ledger + daybook classification rules. Order matters:
/// cash (exact "CASH PAYMENT") is tested before cheque (contains "PAYMENT").
inline Kind kindForEntry(const QVariantMap& e) {
    const QString vType = e.value("voucher_type", e.value("voucherType")).toString().trimmed().toUpper();
    QString raw = e.value("legacy_type", e.value("legacyType")).toString().trimmed().toUpper();
    if (raw.isEmpty()) raw = e.value("trans_type", e.value("transType")).toString().trimmed().toUpper();
    const QString ref = e.value("voucher_no", e.value("voucherNo", e.value("refNo"))).toString().trimmed().toUpper();
    const QString part = e.value("particulars").toString().toLower();

    const auto starts = [&](const QString& p) { return ref.startsWith(p); };
    if (vType == "TDS" || raw == "TDS" || starts("TDS") || part.contains("t.d.s.")) return Kind::Tds;
    if (vType.contains("J-FORM") || vType.contains("JFORM") || vType == "JFRM" || vType == "JF"
        || raw.contains("J-FORM") || raw.contains("JFORM") || raw == "JFRM" || raw == "JF"
        || starts("JFRM") || starts("J-FORM") || starts("JFORM") || starts("JF-")) return Kind::JForm;
    if (vType.contains("I-FORM") || vType.contains("IFORM") || vType == "IFRM" || vType == "IF"
        || raw.contains("I-FORM") || raw.contains("IFORM") || raw == "IFRM" || raw == "IF"
        || starts("IFRM") || starts("I-FORM") || starts("IFORM") || starts("IF-")) return Kind::IForm;
    if (vType.contains("SALE") || raw.contains("SALE") || raw == "SL" || starts("SALE") || starts("SL-")) return Kind::Sale;
    if (vType.contains("PURCHASE") || vType.contains("PURC") || raw.contains("PURCHASE") || raw.contains("PURC")
        || raw == "PU" || starts("PURC") || starts("PUR") || starts("PU-")) return Kind::Purchase;
    if (raw == "PYMT" || raw == "RCPT" || starts("PYMT") || starts("RCPT")
        || vType == "CASH PAYMENT" || vType == "CASH RECEIPT") return Kind::Cash;
    if (vType == "PAYMENT" || vType == "RECEIPT" || raw == "CHPT" || raw == "CHRT" || raw == "BANK"
        || starts("CHPT") || starts("CHRT") || starts("CHQ") || starts("BANK")) return Kind::Cheque;
    if (vType.contains("JOURNAL") || vType == "JRNL" || vType == "JV" || vType == "JL" || vType == "JRN"
        || raw.contains("JOURNAL") || raw == "JRNL" || raw == "JV" || raw == "JL" || raw == "JRN"
        || starts("JRNL") || starts("JV-") || starts("JL-") || starts("JOURNAL")) return Kind::Journal;
    if (vType.contains("MILLING") || raw.contains("MILL") || raw == "PROD" || raw == "ML"
        || starts("MILL") || starts("ML-") || starts("PROD")) return Kind::Milling;
    if (vType.contains("DEBIT NOTE") || vType.contains("CREDIT NOTE") || raw == "DBNT" || raw == "CRNT"
        || raw == "DN" || raw == "CN" || starts("DBNT") || starts("CRNT") || starts("DN-") || starts("CN-")) return Kind::DebitNote;
    return Kind::Unknown;
}

inline int viewIndexForKind(Kind k) {
    switch (k) {
    case Kind::Sale:      return 14;
    case Kind::Purchase:  return 15;
    case Kind::Tds:       return 24;
    case Kind::JForm:     return 18;
    case Kind::IForm:     return 19;
    case Kind::Cash:      return 60;
    case Kind::Cheque:    return 16;
    case Kind::Journal:   return 17;
    case Kind::Milling:   return 31;
    case Kind::DebitNote: return 28;
    case Kind::Unknown:   return -1;
    }
    return -1;
}

inline QString refNoOf(const QVariantMap& e) {
    QString r = e.value("voucher_no").toString().trimmed();
    if (r.isEmpty()) r = e.value("voucherNo").toString().trimmed();
    if (r.isEmpty()) r = e.value("invoiceNo", e.value("invoice_no")).toString().trimmed();
    if (r.isEmpty()) r = e.value("refNo").toString().trimmed();
    return r;
}

inline QString describeEntry(const QVariantMap& e) {
    const QString no = refNoOf(e);
    switch (kindForEntry(e)) {
    case Kind::Sale:      return QString("Sale Invoice %1").arg(no);
    case Kind::Purchase:  return QString("Purchase Bill %1").arg(no);
    case Kind::Tds:       return QString("TDS Voucher %1").arg(no);
    case Kind::JForm:     return QString("J-Form %1").arg(no);
    case Kind::IForm:     return QString("I-Form %1").arg(no);
    case Kind::Cash:      return QString("Cash Voucher %1").arg(no);
    case Kind::Cheque:    return QString("Bank Voucher %1").arg(no);
    case Kind::Journal:   return QString("Journal Voucher %1").arg(no);
    case Kind::Milling:   return QString("Milling Batch %1").arg(no);
    case Kind::DebitNote: return QString("Note %1").arg(no);
    case Kind::Unknown:   return no.isEmpty() ? QString("this entry") : no;
    }
    return no;
}

/// Confirm + delete through the owning model. Returns true on success.
/// Returns false with an EMPTY error when the user cancels; non-empty error
/// means unsupported type or failed lookup/delete (caller shows it).
inline bool deleteEntry(QWidget* parent, const QVariantMap& e, QString* error) {
    const Kind k = kindForEntry(e);
    if (k == Kind::Unknown) {
        if (error) *error = QString("Cannot identify the voucher type for this row.");
        return false;
    }
    if (k == Kind::Milling) {
        if (error) *error = QString("Milling batches cannot be deleted yet — no delete backend (see Doc 01).");
        return false;
    }
    if (!CustomMessageBox::question(parent, QString("Confirm Delete"),
            QString("Permanently delete %1?\nThis cannot be undone.").arg(describeEntry(e)))) {
        return false; // cancelled — error stays empty
    }

    const QString no = refNoOf(e);
    const QString invNo = e.value("invoiceNo", e.value("invoice_no")).toString().trimmed();
    bool ok = false;
    if (k == Kind::Sale) {
        int id = e.value("id").toInt();
        // Invoice-no first: supplier/customer bill numbers resolve even when
        // the row text shows the system voucher_no instead.
        if (id <= 0 && !invNo.isEmpty()) id = SalesModel().get_sales_invoice(invNo, QString()).value("id").toInt();
        if (id <= 0) id = SalesModel().get_sales_invoice(no, QString()).value("id").toInt();
        ok = (id > 0) && SalesModel().delete_sales_invoice(id, !invNo.isEmpty() ? invNo : no);
    } else if (k == Kind::Purchase) {
        int id = e.value("id").toInt();
        if (id <= 0 && !invNo.isEmpty()) id = PurchaseModel().get_purchase_invoice(invNo, QString()).value("id").toInt();
        if (id <= 0) id = PurchaseModel().get_purchase_invoice(no, QString()).value("id").toInt();
        ok = (id > 0) && PurchaseModel().delete_purchase_invoice(id, !invNo.isEmpty() ? invNo : no);
    } else if (k == Kind::Tds) {
        const QString date = e.value("date", e.value("vIso", e.value("invoice_date"))).toString();
        const QString fy = e.value("financial_year", e.value("financialYear")).toString();
        const int vNo = no.split(' ').last().split('-').last().split('/').last().toInt();
        int id = (vNo > 0) ? TdsModel().get_tds_voucher_by_ref(vNo, date, fy).value("id").toInt() : 0;
        if (id <= 0 && !no.isEmpty()) {
            // TEXT voucher_no fallback (by_ref binds an INT).
            QString safeNo = no;
            safeNo.replace("'", "''");
            const QVariantList rows = DatabaseManager::instance().executeQuery(
                QString("SELECT id FROM tds_vouchers WHERE voucher_no = '%1' ORDER BY id DESC LIMIT 1;").arg(safeNo));
            if (!rows.isEmpty()) id = rows.first().toMap().value("id").toInt();
        }
        ok = (id > 0) && TdsVoucherController().deleteVoucher(id);
    } else if (k == Kind::JForm) {
        const int id = JFormModel().get_jform_voucher(no).value("id").toInt();
        ok = (id > 0) && JFormModel().delete_jform_voucher(id);
    } else if (k == Kind::IForm) {
        const int id = IFormModel().get_iform_voucher(no).value("id").toInt();
        ok = (id > 0) && IFormModel().delete_iform_voucher(id);
    } else if (k == Kind::Cash || k == Kind::Cheque || k == Kind::Journal) {
        const int id = VouchersModel().get_voucher(no, QString(), QString()).value("id").toInt();
        ok = (id > 0) && VouchersModel().delete_voucher(id);
    } else if (k == Kind::DebitNote) {
        QString safeNo = no;
        safeNo.replace("'", "''");
        const QVariantList rows = DatabaseManager::instance().executeQuery(
            QString("SELECT id FROM debit_credit_notes WHERE note_no = '%1' LIMIT 1;").arg(safeNo));
        const int id = rows.isEmpty() ? 0 : rows.first().toMap().value("id").toInt();
        ok = (id > 0) && DebitCreditNoteController().deleteNote(id);
    }
    if (!ok && error && error->isEmpty())
        *error = QString("Delete failed for %1 (record not found or locked).").arg(describeEntry(e));
    return ok;
}

} // namespace VoucherOps
