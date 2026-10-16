#pragma once

#include <QString>

// FyCloseEngine (F3): guided year-end close as pure engine calls. The close
// dialog (Agent 2) renders state and calls these; no UI here.
// Order per close: trial check -> P&L net -> closing journal (skipped when
// the net is zero) -> next-FY ensure -> openings snapshot -> lock.
// Reopen reverses ONLY the closing voucher, then unlocks. Every failure is
// loud with row/voucher context; nothing half-writes (single transaction).
struct FyCloseResult {
    bool ok = false;
    QString error;
    QString closingVoucherNo; // empty when P&L net was zero (no journal)
    int snapshotRows = 0;
    double plNet = 0.0;
};

class FyCloseEngine {
public:
    // |Dr - Cr| over the FY's voucher dates. < 0 on error (see errorOut).
    static double trialDiff(const QString& fyName, QString* errorOut = nullptr);
    // actor = audit identity (calculator falls back to "local").
    static FyCloseResult closeYear(const QString& fyName,
                                   const QString& capitalLedgerName,
                                   const QString& actor);
    static FyCloseResult reopenYear(const QString& fyName, const QString& actor);
};
