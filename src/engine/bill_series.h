#pragma once

#include <QString>

// BillSeriesConfig: the user-customizable sale-bill numbering series.
//
// Stored as ONE JSON row in app_settings ("bill_series/sale") inside the
// firm's own .db file, so the numbering system travels with the file and no
// firm-specific literal (MRI/..., STC/...) is ever hardcoded in source.
// Format examples this config can express:
//   prefix=MRI  sepA=/  fy=yyYY  sepB=-  width=1  -> "MRI/2627-11"   (plain)
//   prefix=STC  sepA=/  fy=yyYY  sepB=-  width=4  -> "STC/2627-0001" (padded)
//   prefix=INV  sepA=/  fy=none  sepB=-  width=1  -> "INV-11"
//   prefix=""   sepA="" fy=none  sepB="" width=4  -> "0001"
struct BillSeriesConfig {
    QString prefix = "INV";       // Neutral factory default; never a firm name.
    QString sepAfterPrefix = "/";
    QString fyStyle = "yyYY";     // "yyYY" (2627) | "yy-yy" (26-27) |
                                  // "YYYY-YY" (2026-27) | "none"
    QString sepBeforeSeq = "-";
    int seqWidth = 1;             // 1 = plain (-11); 4 = zero-padded (-0001)
    long long startNumber = 1;
    bool resetEachFy = true;      // GST-correct: restart at startNumber each FY
    QString suffix;               // Optional tail, e.g. "/A" (must not start with a digit)

    // FY long name ("FY 2026-27") -> token per fyStyle ("" when "none").
    QString fyToken(const QString& fyLongName) const;
    // Matchable head for the series in one FY, e.g. "MRI/2627-".
    QString staticHead(const QString& fyLongName) const;
    // Full bill number for one sequence value.
    QString render(long long seq, const QString& fyLongName) const;

    QString toJson() const;
    static BillSeriesConfig fromJson(const QString& json, bool* ok = nullptr);
    // "" when valid, else a human-readable reason.
    QString validate() const;
};

class BillSeriesManager {
public:
    static QString settingsKey(const QString& docType = "sale"); // "bill_series/sale"

    // Effective config: saved row if present and valid; otherwise ADOPTED from
    // the dominant legacy prefix already in the table (so existing firms keep
    // their exact series with zero hardcoding); otherwise the neutral default.
    // Read-through on every call — no cache, so a saved dialog change and a
    // file copied to another machine are both picked up immediately.
    static BillSeriesConfig load(const QString& docType, const QString& table);
    static bool save(const QString& docType, const BillSeriesConfig& cfg);

    // Next bill number for docType/table in the FY resolved from fyOrDate
    // (FY label or bill date; active FY when empty). Counts the configured
    // series within scope (this FY when resetEachFy, else all years); when the
    // series has no rows yet in scope, continues past the highest legacy
    // number so sequences never restart at 1 mid-history by accident.
    static QString nextNumber(const QString& docType, const QString& table,
                              const QString& fyOrDate = "");
    // Same computation for an explicit (e.g. dialog-draft) config.
    static QString nextForConfig(const BillSeriesConfig& cfg, const QString& table,
                                 const QString& fyOrDate = "");
};
