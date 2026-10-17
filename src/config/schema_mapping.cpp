#include "schema_mapping.h"

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QVariant>

namespace MahadevERP {

QString SchemaMapping::supportedVersion() {
    // Single place. plan/schema_mapping.json meta.version must equal this
    // exactly — minor or draft suffix differences fail closed.
    return QStringLiteral("0.3-draft");
}

QString SchemaMapping::findMappingFile() {
    // CWD-dependent lookup only (repo root when tools run there, build/
    // when suites run there). No registry, no network, no fallback
    // document — absent file is a loud failure, not a default.
    static const char* candidates[] = {
        "plan/schema_mapping.json",
        "../plan/schema_mapping.json",
    };
    for (const char* c : candidates) {
        if (QFile::exists(QString::fromLatin1(c))) return QString::fromLatin1(c);
    }
    return QString();
}

bool SchemaMapping::load(const QString& path) {
    m_valid = false;
    m_error.clear();
    m_version.clear();
    m_tables.clear();
    m_codeMaps.clear();

    QFile f(path);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) {
        m_error = QString("schema map unreadable: %1").arg(path);
        return false;
    }
    QJsonParseError perr;
    QJsonDocument doc = QJsonDocument::fromJson(f.readAll(), &perr);
    if (perr.error != QJsonParseError::NoError || !doc.isObject()) {
        m_error = QString("schema map malformed JSON (%1): %2").arg(path, perr.errorString());
        return false;
    }
    QJsonObject root = doc.object();
    QJsonObject meta = root.value("meta").toObject();
    m_version = meta.value("version").toString();
    if (m_version.isEmpty()) {
        m_error = QString("schema map has no meta.version: %1").arg(path);
        return false;
    }
    if (m_version != supportedVersion()) {
        // Fail closed (Doc 22 §7): code MUST NOT interpret a mapping whose
        // shape it was not reviewed against. Bump supportedVersion()
        // together with the JSON after reviewing the diff.
        m_error = QString("schema map version %1 != supported %2 (refusing to interpret)")
                      .arg(m_version, supportedVersion());
        return false;
    }
    QJsonArray tables = root.value("tables").toArray();
    if (tables.isEmpty()) {
        m_error = QString("schema map has no tables[]: %1").arg(path);
        return false;
    }
    for (const QJsonValue& tv : tables) {
        QJsonObject t = tv.toObject();
        QVariantMap m;
        m["source"] = t.value("source").toString();
        m["target"] = t.value("target").toString();
        m["direction"] = t.value("direction").toString();
        m["provider"] = t.value("provider").toString();
        m["fy_scoped"] = t.value("fy_scoped").toBool();
        QStringList df;
        for (const QJsonValue& d : t.value("domain_flags").toArray()) df.append(d.toString());
        m["domain_flags"] = df;
        m["has_read_filter"] = t.contains("read_filter");
        QVariantList cols;
        for (const QJsonValue& c : t.value("columns").toArray()) {
            // Pass every declared key through, not just source/target: the
            // exporter roster (tds_vouchers -> TDSDeductions) is ORDERED and
            // carries position/jet_type. Consumers key off source/target and
            // ignore the rest, so extra metadata is additive.
            QVariantMap cm = c.toObject().toVariantMap();
            if (!cm["source"].toString().isEmpty()) cols.append(cm);
        }
        m["columns"] = cols;
        if (m["source"].toString().isEmpty() || m["target"].toString().isEmpty()
            || m["direction"].toString().isEmpty()) {
            m_error = QString("schema map table entry missing source/target/direction: %1").arg(path);
            m_tables.clear();
            return false;
        }
        m_tables.append(m);
    }
    QJsonObject cm = root.value("code_maps").toObject();
    for (auto it = cm.begin(); it != cm.end(); ++it) m_codeMaps[it.key()] = it.value().toVariant();
    m_valid = true;
    return true;
}

bool SchemaMapping::loadDefault() {
    const QString p = findMappingFile();
    if (p.isEmpty()) {
        m_valid = false;
        m_error = QString("schema map not found (tried plan/ and ../plan/)");
        return false;
    }
    return load(p);
}

QVariantList SchemaMapping::tables() const {
    return m_tables;
}

QStringList SchemaMapping::tableTargets() const {
    QStringList out;
    for (const QVariant& v : m_tables) out.append(v.toMap().value("target").toString());
    return out;
}

QStringList SchemaMapping::domainTableTargets() const {
    QStringList out;
    for (const QVariant& v : m_tables) {
        QVariantMap m = v.toMap();
        if (!m.value("domain_flags").toStringList().isEmpty()) out.append(m.value("target").toString());
    }
    return out;
}

QVariantMap SchemaMapping::codeMap(const QString& name) const {
    return m_codeMaps.value(name).toMap();
}

QStringList SchemaMapping::codeMapNames() const {
    return m_codeMaps.keys();
}

} // namespace MahadevERP
