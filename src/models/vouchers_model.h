#pragma once

#include "base_table_model.h"

class VouchersModel : public BaseTableModel {
    Q_OBJECT

public:
    explicit VouchersModel(QObject* parent = nullptr);
    Q_INVOKABLE void reload_data() override;

    Q_INVOKABLE QString get_next_voucher_no(const QString& v_type = "ChPt", const QString& fy = "");

    // Claims a free voucher number: fresh MAX read + existence check per
    // attempt (mutex-serialized, so thread-race-free), UNIQUE index as the
    // cross-process backstop. insertType = value written to voucher_type,
    // genType = family passed to get_next_voucher_no (kept separate so legacy
    // numbering families never shift). "" when exhausted (8 tries).
    Q_INVOKABLE QString claimVoucherNo(const QString& insertType, const QString& genType, const QString& fyLabel);
    Q_INVOKABLE bool add_voucher(const QString& vch_type, const QString& party_name, const QString& vch_date, const QString& account_type, double amount, const QString& narration);
    Q_INVOKABLE bool add_cheque_voucher(const QString& vch_type, const QString& dr_party, const QString& cr_party, double amount, const QString& chq_no, const QString& narration, const QString& vch_date = "");
    Q_INVOKABLE bool add_journal_voucher(const QString& dr_party, const QString& cr_party, double amount, const QString& ref_no = "", const QString& narration = "", const QString& vch_date = "", const QString& vch_type = "Journal");
    Q_INVOKABLE QVariantMap get_voucher(const QVariant& vchNoOrId, const QString& dateHint = "", const QString& partyHint = "");
    Q_INVOKABLE QVariantMap get_cheque_voucher(const QVariant& vchNoOrId, const QString& dateHint = "", const QString& partyHint = "");
    Q_INVOKABLE QVariantMap get_journal_voucher(const QVariant& vchNoOrId, const QString& dateHint = "", const QString& partyHint = "");
    Q_INVOKABLE bool save_multi_row_voucher(int editId, const QString& vch_type, const QString& vch_no, const QString& vch_date, const QString& narration, const QVariantList& rows);
    Q_INVOKABLE bool delete_voucher(int voucherId);
};
