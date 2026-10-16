#pragma once

#include "base_table_model.h"

// VoucherTypeModel: CRUD + cache for the voucher_types master table.
// Single source of truth for every voucher kind (generic-ERP pivot).
// System core types (is_system=1) can be renamed but never deleted.
class VoucherTypeModel : public BaseTableModel {
    Q_OBJECT

public:
    explicit VoucherTypeModel(QObject* parent = nullptr);
    Q_INVOKABLE void reload_data() override;

    Q_INVOKABLE QVariantList activeTypes() const;
    Q_INVOKABLE QVariantList typesByCategory(const QString& category) const;
    Q_INVOKABLE QVariantMap byCode(const QString& code) const;
    Q_INVOKABLE bool isDeletable(const QString& code) const;
    Q_INVOKABLE bool isSystem(const QString& code) const;

    // Insert or update a (non-system or new) type. Returns false when the
    // code belongs to a system type being deleted/renamed-to-duplicate.
    Q_INVOKABLE bool saveType(const QVariantMap& type);
    Q_INVOKABLE bool deleteType(const QString& code);
    Q_INVOKABLE bool setActive(const QString& code, bool active);

    // Domain top-up (the flags engine calls this; idempotent by code).
    static void ensureDomainTypes(const QString& domain);
};
