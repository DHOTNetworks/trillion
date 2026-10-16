#pragma once

// SchemaMapping — C++ loader for plan/schema_mapping.json (Doc 22, stage C2).
// Parses + self-tests ONLY; nothing consumes it yet (promotion is C3/C4).
// QObject-free and DatabaseManager-free on purpose: this TU links into EVERY
// target (app, both suites, CLI), like migration_utils. Qt JSON only.
//
// Fail-closed contract (§2): a version mismatch, missing file, or malformed
// document yields isValid() == false with lastError() set. Callers must
// check validity — never fall back to hardcoded scatter.

#include <QString>
#include <QStringList>
#include <QVariantList>
#include <QVariantMap>

namespace MahadevERP {

struct SchemaTable {
    QString source;       // e.g. "Ledgers" (Bahi-Khata) / Tally/Busy source
    QString target;       // e.g. "parties" (our table)
    QString direction;    // "both" (Bahi-Khata) or "import" (Tally/Busy stubs)
    QString provider;     // "bahikhata" | "tally" | "busy"
    bool fyScoped = false;
    QStringList domainFlags; // e.g. {"feature.mandi"} or {"feature.mandi","feature.manufacturer"}
    bool hasReadFilter = false; // derived source (jform/iform/paddy TransType filter)
};

class SchemaMapping {
public:
    // The single version this code understands. Bump together with
    // meta.version in plan/schema_mapping.json — never accept unknown.
    static QString supportedVersion();

    // Locate plan/schema_mapping.json from common CWDs (repo root, build/).
    // Empty when not found (caller fails loud; no fake data).
    static QString findMappingFile();

    // Parse + validate. False on missing file, bad JSON, missing meta, or
    // version mismatch — isValid() stays false and lastError() explains.
    bool load(const QString& path);
    bool loadDefault(); // findMappingFile() + load(); false when unlocatable

    bool isValid() const { return m_valid; }
    QString lastError() const { return m_error; }
    QString version() const { return m_version; }
    QVariantList tables() const;
    QStringList tableTargets() const;
    QStringList domainTableTargets() const;
    QVariantMap codeMap(const QString& name) const;
    QStringList codeMapNames() const;

private:
    bool m_valid = false;
    QString m_error;
    QString m_version;
    QVariantList m_tables;
    QVariantMap m_codeMaps;
};

} // namespace MahadevERP
