#include "voucher_type_model.h"
#include "../database_manager.h"

VoucherTypeModel::VoucherTypeModel(QObject* parent)
    : BaseTableModel(
        {"Code", "Name", "Category", "Prefix", "Numbering", "Active"},
        {"code", "name", "category", "prefix", "numbering_method", "is_active"},
        parent)
{
}

void VoucherTypeModel::reload_data() {
    beginResetModel();
    m_data = DatabaseManager::instance().executeQuery(
        "SELECT id, code, name, category, is_system, prefix, numbering_method, "
        "separator, number_width, reset_each_fy, affects_stock, requires_party, "
        "requires_stock, requires_gst, sort_order, is_active "
        "FROM voucher_types ORDER BY sort_order ASC, name ASC;");
    endResetModel();
    emit dataChangedSignal();
    emit countChanged();
}

QVariantList VoucherTypeModel::activeTypes() const {
    QVariantList out;
    for (const QVariant& v : m_data) {
        if (v.toMap().value("is_active").toInt() != 0) out.append(v);
    }
    return out;
}

QVariantList VoucherTypeModel::typesByCategory(const QString& category) const {
    QVariantList out;
    for (const QVariant& v : m_data) {
        QVariantMap m = v.toMap();
        if (m.value("is_active").toInt() != 0 &&
            m.value("category").toString().compare(category.trimmed(), Qt::CaseInsensitive) == 0) {
            out.append(v);
        }
    }
    return out;
}

QVariantMap VoucherTypeModel::byCode(const QString& code) const {
    const QString want = code.trimmed().toUpper();
    for (const QVariant& v : m_data) {
        QVariantMap m = v.toMap();
        if (m.value("code").toString().toUpper() == want) return m;
    }
    // Fall back to a live read so callers see rows saved outside the model.
    QVariantList rows = DatabaseManager::instance().executeQuery(
        "SELECT id, code, name, category, is_system, prefix, numbering_method, "
        "separator, number_width, reset_each_fy, affects_stock, requires_party, "
        "requires_stock, requires_gst, sort_order, is_active "
        "FROM voucher_types WHERE UPPER(code) = ? LIMIT 1;", {want});
    if (!rows.isEmpty()) return rows.first().toMap();
    return QVariantMap();
}

bool VoucherTypeModel::isSystem(const QString& code) const {
    return byCode(code).value("is_system").toInt() != 0;
}

bool VoucherTypeModel::isDeletable(const QString& code) const {
    QVariantMap m = byCode(code);
    if (m.isEmpty()) return false;
    return m.value("is_system").toInt() == 0;
}

bool VoucherTypeModel::saveType(const QVariantMap& type) {
    const QString code = type.value("code").toString().trimmed().toUpper();
    if (code.isEmpty() || type.value("name").toString().trimmed().isEmpty()) return false;

    QVariantMap existing = byCode(code);
    if (!existing.isEmpty() && existing.value("is_system").toInt() != 0) {
        // System types: rename-only; code/category/flags stay canonical.
        // (A same-name save is a harmless no-op, never a column rewrite.)
        const QString nm = type.value("name").toString().trimmed();
        if (nm.isEmpty() || nm == existing.value("name").toString()) return true;
        bool ok = DatabaseManager::instance().executeNonQuery(
            "UPDATE voucher_types SET name = ? WHERE code = ?;", {nm, code});
        if (ok) reload_data();
        return ok;
    }

    const QString category = type.value("category", "OTHER").toString().trimmed().toUpper();
    const QString prefix = type.value("prefix", "").toString().trimmed();
    const QString numbering = type.value("numbering_method", "AUTO").toString().trimmed().toUpper();
    const QString sep = type.value("separator", "-").toString();
    const int width = qBound(1, type.value("number_width", 4).toInt(), 8);
    const int resetFy = type.value("reset_each_fy", 1).toInt() != 0 ? 1 : 0;
    const int affectsStock = type.value("affects_stock", 0).toInt() != 0 ? 1 : 0;
    const int reqParty = type.value("requires_party", 1).toInt() != 0 ? 1 : 0;
    const int reqStock = type.value("requires_stock", 0).toInt() != 0 ? 1 : 0;
    const int reqGst = type.value("requires_gst", 0).toInt() != 0 ? 1 : 0;

    bool ok;
    if (existing.isEmpty()) {
        ok = DatabaseManager::instance().executeNonQuery(
            "INSERT INTO voucher_types (code, name, category, is_system, prefix, "
            "numbering_method, separator, number_width, reset_each_fy, "
            "affects_stock, requires_party, requires_stock, requires_gst, sort_order, is_active) "
            "VALUES (?, ?, ?, 0, ?, ?, ?, ?, ?, ?, ?, ?, ?, 100, 1);",
            {code, type.value("name").toString().trimmed(), category, prefix,
             (numbering == "MANUAL" ? "MANUAL" : "AUTO"), sep, width, resetFy,
             affectsStock, reqParty, reqStock, reqGst});
    } else {
        ok = DatabaseManager::instance().executeNonQuery(
            "UPDATE voucher_types SET name = ?, category = ?, prefix = ?, "
            "numbering_method = ?, separator = ?, number_width = ?, reset_each_fy = ?, "
            "affects_stock = ?, requires_party = ?, requires_stock = ?, requires_gst = ? "
            "WHERE code = ?;",
            {type.value("name").toString().trimmed(), category, prefix,
             (numbering == "MANUAL" ? "MANUAL" : "AUTO"), sep, width, resetFy,
             affectsStock, reqParty, reqStock, reqGst, code});
    }
    if (ok) reload_data();
    return ok;
}

bool VoucherTypeModel::deleteType(const QString& code) {
    if (!isDeletable(code)) return false;
    bool ok = DatabaseManager::instance().executeNonQuery(
        "DELETE FROM voucher_types WHERE code = ?;", {code.trimmed().toUpper()});
    if (ok) reload_data();
    return ok;
}

bool VoucherTypeModel::setActive(const QString& code, bool active) {
    QVariantMap m = byCode(code);
    if (m.isEmpty()) return false;
    bool ok = DatabaseManager::instance().executeNonQuery(
        "UPDATE voucher_types SET is_active = ? WHERE code = ?;",
        {active ? 1 : 0, code.trimmed().toUpper()});
    if (ok) reload_data();
    return ok;
}

void VoucherTypeModel::ensureDomainTypes(const QString& domain) {
    const QString d = domain.trimmed().toLower();
    struct DomainType { const char* code; const char* name; const char* category; const char* prefix; int order; };
    static const DomainType mandi[] = {
        {"JFRM", "J-Form (Mandi Purchase)", "PURCHASE", "JF-", 110},
        {"IFRM", "I-Form (Mandi Sale)", "SALES", "IF-", 111},
    };
    static const DomainType manufacturer[] = {
        {"MANU", "Manufacturing Production", "OTHER", "MF-", 120},
    };
    const DomainType* list = nullptr;
    int count = 0;
    if (d == "mandi") { list = mandi; count = 2; }
    else if (d == "manufacturer") { list = manufacturer; count = 1; }
    else return;
    for (int i = 0; i < count; ++i) {
        QVariantList rows = DatabaseManager::instance().executeQuery(
            "SELECT id FROM voucher_types WHERE code = ? LIMIT 1;", {list[i].code});
        if (!rows.isEmpty()) continue;
        DatabaseManager::instance().executeNonQuery(
            "INSERT INTO voucher_types (code, name, category, is_system, prefix, "
            "numbering_method, separator, number_width, reset_each_fy, "
            "affects_stock, requires_party, requires_stock, requires_gst, sort_order, is_active) "
            "VALUES (?, ?, ?, 0, ?, 'AUTO', '-', 4, 1, 0, 1, 0, 0, ?, 1);",
            {list[i].code, list[i].name, list[i].category, list[i].prefix, list[i].order});
    }
}
