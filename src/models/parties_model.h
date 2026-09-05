#pragma once

#include "base_table_model.h"

class PartiesModel : public BaseTableModel {
    Q_OBJECT

public:
    explicit PartiesModel(QObject* parent = nullptr);
    Q_INVOKABLE void reload_data() override;

    Q_INVOKABLE QStringList get_parties_list() const;
    Q_INVOKABLE QStringList get_party_list() const;
    Q_INVOKABLE QStringList get_bank_accounts_list() const;
    Q_INVOKABLE QString get_ledger_live_balance(const QString& ledgerName);
    Q_INVOKABLE QString get_party_live_balance_by_id(int partyId) const;
    Q_INVOKABLE QVariantMap get_party_statement(const QString& partyName);
    Q_INVOKABLE QVariantMap get_party_by_name(const QString& name) const;
    Q_INVOKABLE QVariantMap get_party_by_id(int partyId) const;
    Q_INVOKABLE QVariantList search_parties(const QString& query) const;
    Q_INVOKABLE QStringList get_account_groups() const;
    Q_INVOKABLE QStringList get_cities() const;
    Q_INVOKABLE QStringList get_districts() const;
    Q_INVOKABLE QStringList get_stations() const;

    Q_INVOKABLE bool add_party(
        const QString& name, const QString& ptype, const QString& phone,
        const QString& place, const QString& gstin, double op_bal, const QString& bal_type
    );

    Q_INVOKABLE bool update_ledger_full(
        int party_id, const QString& name, const QString& alias, const QString& prefix,
        const QString& group_name, const QString& party_type, const QString& special_type,
        double opening_balance, const QString& balance_type, const QString& mailing_name,
        const QString& address, const QString& city, const QString& district,
        const QString& state, const QString& pincode, const QString& phone,
        const QString& mobile, const QString& whatsapp, const QString& email,
        const QString& contact_person, const QString& gstin, const QString& pan,
        const QString& aadhaar, double credit_limit, int credit_days,
        const QString& bank_name = "", const QString& bank_account = "", const QString& ifsc_code = ""
    );
};
