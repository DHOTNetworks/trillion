#pragma once

#include "base_table_model.h"

class VouchersModel : public BaseTableModel {
    Q_OBJECT

public:
    explicit VouchersModel(QObject* parent = nullptr);
    Q_INVOKABLE void reload_data() override;

    Q_INVOKABLE QString get_next_voucher_no(const QString& v_type = "ChPt", const QString& fy = "");
    Q_INVOKABLE bool add_voucher(const QString& vch_type, const QString& party_name, const QString& vch_date, const QString& account_type, double amount, const QString& narration);
    Q_INVOKABLE bool add_cheque_voucher(const QString& vch_type, const QString& dr_party, const QString& cr_party, double amount, const QString& chq_no, const QString& narration, const QString& vch_date = "");
    Q_INVOKABLE bool add_journal_voucher(const QString& dr_party, const QString& cr_party, double amount, const QString& ref_no = "", const QString& narration = "", const QString& vch_date = "", const QString& vch_type = "Journal");
};
