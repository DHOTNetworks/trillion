#include "fiscal_year_helper.h"
#include "../database_manager.h"
#include "accounting_engine.h"
#include "../services/accounting_date_service.h"
#include <QDate>
#include <QRegularExpression>
#include <QSet>
#include <cmath>
#include <algorithm>

QString FiscalYearHelper::normalizeToIso(const QString& dateStr) {
    QString s = dateStr.trimmed();
    if (s.isEmpty()) return "";
    return AccountingDateService::instance().toIso(s);
}

QString FiscalYearHelper::formatDisplayDate(const QString& dateStr) {
    QString iso = normalizeToIso(dateStr);
    if (iso.length() == 10 && iso.at(4) == '-' && iso.at(7) == '-') {
        QStringList p = iso.split('-');
        return QString("%1-%2-%3").arg(p[2], p[1], p[0]);
    }
    return dateStr;
}

void FiscalYearHelper::ensureFiscalYearsDiscovered() {
    // 1. Check if financial_years table exists
    QVariant tableExists = DatabaseManager::instance().executeScalar(
        "SELECT COUNT(*) FROM sqlite_master WHERE type='table' AND name='financial_years';"
    );
    if (tableExists.toInt() == 0) return;

    // 2. Query existing financial years in database
    QVariantList existingRows = DatabaseManager::instance().executeQuery(
        "SELECT id, year_name, start_date, end_date, is_active, is_locked FROM financial_years ORDER BY start_date ASC;"
    );

    // If financial years already populated in database:
    if (!existingRows.isEmpty()) {
        bool hasActiveCheck = false;
        for (const auto& rVar : existingRows) {
            if (rVar.toMap().value("is_active").toBool()) {
                hasActiveCheck = true;
                break;
            }
        }
        // If active year already exists, we are done
        if (hasActiveCheck) {
            return;
        }

        // If financial_years has rows but none is marked active, activate the latest existing one
        QVariantMap latestRow = existingRows.last().toMap();
        QString latestName = latestRow.value("year_name").toString();
        if (!latestName.isEmpty()) {
            DatabaseManager::instance().executeNonQuery("UPDATE financial_years SET is_active = 0;");
            DatabaseManager::instance().executeNonQuery(
                "UPDATE financial_years SET is_active = 1 WHERE year_name = ?;",
                {latestName}
            );
        }
        return;
    }

    QSet<QString> seenFys;
    int earliestYear = 9999;
    int latestYear = 0;

    // 3. Scan company_info for fiscal year and books_from dates
    QVariantList compRows = DatabaseManager::instance().executeQuery(
        "SELECT acc_year_from, acc_year_to, books_from FROM company_info LIMIT 5;"
    );
    for (const auto& cVar : compRows) {
        QVariantMap c = cVar.toMap();
        QStringList dateCandidates = {
            c.value("acc_year_from").toString(),
            c.value("acc_year_to").toString(),
            c.value("books_from").toString()
        };
        for (const auto& dStr : dateCandidates) {
            QString iso = normalizeToIso(dStr);
            if (iso.length() >= 7) {
                int y = iso.left(4).toInt();
                int m = iso.mid(5, 2).toInt();
                if (y > 1990 && y < 2100) {
                    int sYear = (m >= 4) ? y : y - 1;
                    if (sYear < earliestYear) earliestYear = sYear;
                    if (sYear > latestYear) latestYear = sYear;
                }
            }
        }
    }

    // 4. Scan transaction tables for min/max dates
    QStringList dateQueries = {
        "SELECT MIN(voucher_date), MAX(voucher_date) FROM vouchers WHERE voucher_date IS NOT NULL AND voucher_date != '';",
        "SELECT MIN(voucher_date), MAX(voucher_date) FROM transactions WHERE voucher_date IS NOT NULL AND voucher_date != '';",
        "SELECT MIN(invoice_date), MAX(invoice_date) FROM sales_invoices WHERE invoice_date IS NOT NULL AND invoice_date != '';",
        "SELECT MIN(invoice_date), MAX(invoice_date) FROM purchase_invoices WHERE invoice_date IS NOT NULL AND invoice_date != '';",
        "SELECT MIN(arrival_date), MAX(arrival_date) FROM paddy_procurement WHERE arrival_date IS NOT NULL AND arrival_date != '';",
        "SELECT MIN(batch_date), MAX(batch_date) FROM milling_batches WHERE batch_date IS NOT NULL AND batch_date != '';",
        "SELECT MIN(voucher_date), MAX(voucher_date) FROM jform_vouchers WHERE voucher_date IS NOT NULL AND voucher_date != '';",
        "SELECT MIN(voucher_date), MAX(voucher_date) FROM tds_vouchers WHERE voucher_date IS NOT NULL AND voucher_date != '';",
        "SELECT MIN(dispatch_date), MAX(dispatch_date) FROM transport_dispatches WHERE dispatch_date IS NOT NULL AND dispatch_date != '';",
        "SELECT MIN(note_date), MAX(note_date) FROM debit_credit_notes WHERE note_date IS NOT NULL AND note_date != '';"
    };

    for (const auto& sql : dateQueries) {
        QVariantList res = DatabaseManager::instance().executeQuery(sql);
        if (!res.isEmpty()) {
            QVariantMap row = res.first().toMap();
            for (auto it = row.begin(); it != row.end(); ++it) {
                QString dStr = it.value().toString();
                QString iso = normalizeToIso(dStr);
                if (iso.length() >= 7) {
                    int y = iso.left(4).toInt();
                    int m = iso.mid(5, 2).toInt();
                    if (y > 1990 && y < 2100) {
                        int sYear = (m >= 4) ? y : y - 1;
                        if (sYear < earliestYear) earliestYear = sYear;
                        if (sYear > latestYear) latestYear = sYear;
                    }
                }
            }
        }
    }

    // 5. If no dates were found in database at all (brand new empty database), default to current date's FY
    if (earliestYear == 9999 && latestYear == 0) {
        QDate today = QDate::currentDate();
        int currentStartYear = (today.month() >= 4) ? today.year() : today.year() - 1;
        earliestYear = currentStartYear;
        latestYear = currentStartYear;
    } else if (earliestYear == 9999) {
        earliestYear = latestYear;
    } else if (latestYear == 0) {
        latestYear = earliestYear;
    }

    // 6. Insert only the actual discovered financial years (never invent future years)
    for (int y = earliestYear; y <= latestYear; ++y) {
        QString fyName = QString("FY %1-%2").arg(y).arg(QString::number((y + 1) % 100).rightJustified(2, '0'));
        if (!seenFys.contains(fyName)) {
            QString fromD = QString("%1-04-01").arg(y);
            QString toD = QString("%1-03-31").arg(y + 1);
            DatabaseManager::instance().executeNonQuery(
                "INSERT INTO financial_years (year_name, start_date, end_date, is_active, is_locked) "
                "VALUES (?, ?, ?, 0, 0);",
                {fyName, fromD, toD}
            );
            seenFys.insert(fyName);
        }
    }

    // 7. Ensure at least one active financial year exists (the latest actual year)
    QString defaultActive = QString("FY %1-%2").arg(latestYear).arg(QString::number((latestYear + 1) % 100).rightJustified(2, '0'));
    DatabaseManager::instance().executeNonQuery("UPDATE financial_years SET is_active = 0;");
    DatabaseManager::instance().executeNonQuery(
        "UPDATE financial_years SET is_active = 1 WHERE year_name = ?;",
        {defaultActive}
    );
}

FiscalYearInfo FiscalYearHelper::getActiveFiscalYear() {
    // 1. Check AccountingEngine in-memory cache if active
    QString cFrom = AccountingEngine::getActiveFromDate();
    QString cTo = AccountingEngine::getActiveToDate();
    QString cLabel = AccountingEngine::getActiveFyLabel();

    if (!cFrom.isEmpty() && !cTo.isEmpty()) {
        FiscalYearInfo info;
        info.name = !cLabel.isEmpty() ? cLabel : QString("%1 To %2").arg(formatDisplayDate(cFrom), formatDisplayDate(cTo));
        info.startDate = normalizeToIso(cFrom);
        info.endDate = normalizeToIso(cTo);
        info.isActive = true;
        return info;
    }

    ensureFiscalYearsDiscovered();
    FiscalYearInfo info;

    // 2. Check Database saved settings for last selected FY/period across app restarts
    QString lastFy = DatabaseManager::instance().getSetting("last_selected_fy");
    QString lastFrom = DatabaseManager::instance().getSetting("last_from_date");
    QString lastTo = DatabaseManager::instance().getSetting("last_to_date");
    if (!lastFrom.isEmpty() && !lastTo.isEmpty()) {
        info.name = !lastFy.isEmpty() ? lastFy : QString("%1 To %2").arg(formatDisplayDate(lastFrom), formatDisplayDate(lastTo));
        info.startDate = normalizeToIso(lastFrom);
        info.endDate = normalizeToIso(lastTo);
        info.isActive = true;
        AccountingEngine::setActivePeriod(info.startDate, info.endDate, info.name);
        return info;
    }

    // 3. Query database for active year first (guaranteed single source of truth for the active database)
    QVariantList activeRows = DatabaseManager::instance().executeQuery(
        "SELECT year_name, start_date, end_date, is_active, is_locked FROM financial_years WHERE is_active = 1 LIMIT 1;"
    );
    if (!activeRows.isEmpty()) {
        QVariantMap r = activeRows.first().toMap();
        info.name = r.value("year_name").toString().trimmed();
        info.startDate = normalizeToIso(r.value("start_date").toString());
        info.endDate = normalizeToIso(r.value("end_date").toString());
        info.isActive = true;
        info.isLocked = r.value("is_locked").toBool();

        AccountingEngine::setActivePeriod(info.startDate, info.endDate, info.name);
        return info;
    }

    // 4. Fallback to the latest year in database
    QVariantList latestRows = DatabaseManager::instance().executeQuery(
        "SELECT year_name, start_date, end_date, is_active, is_locked FROM financial_years ORDER BY start_date DESC LIMIT 1;"
    );
    if (!latestRows.isEmpty()) {
        QVariantMap r = latestRows.first().toMap();
        info.name = r.value("year_name").toString().trimmed();
        info.startDate = normalizeToIso(r.value("start_date").toString());
        info.endDate = normalizeToIso(r.value("end_date").toString());
        info.isActive = r.value("is_active").toBool();
        info.isLocked = r.value("is_locked").toBool();

        AccountingEngine::setActivePeriod(info.startDate, info.endDate, info.name);
        return info;
    }

    // 5. Default fallback based on current date
    QDate today = QDate::currentDate();
    int startY = (today.month() >= 4) ? today.year() : today.year() - 1;
    int endY = startY + 1;
    info.name = QString("FY %1-%2").arg(startY).arg(QString::number(endY).right(2));
    info.startDate = QString("%1-04-01").arg(startY);
    info.endDate = QString("%1-03-31").arg(endY);
    info.isActive = true;

    AccountingEngine::setActivePeriod(info.startDate, info.endDate, info.name);
    return info;
}

void FiscalYearHelper::setActiveFiscalYear(const QString& fyNameOrLabel) {
    if (fyNameOrLabel.trimmed().isEmpty()) return;
    FiscalYearInfo fy = getFiscalYearByName(fyNameOrLabel.trimmed());
    if (fy.isValid()) {
        DatabaseManager::instance().executeNonQuery("UPDATE financial_years SET is_active = 0;");
        DatabaseManager::instance().executeNonQuery("UPDATE financial_years SET is_active = 1 WHERE year_name = ?;", {fy.name});
        DatabaseManager::instance().setSetting("last_selected_fy", fy.name);
        DatabaseManager::instance().setSetting("last_from_date", fy.startDate);
        DatabaseManager::instance().setSetting("last_to_date", fy.endDate);
        AccountingEngine::setActivePeriod(fy.startDate, fy.endDate, fy.name);
    }
}

void FiscalYearHelper::setActiveCustomPeriod(const QString& fromIso, const QString& toIso, const QString& label) {
    QString f = normalizeToIso(fromIso);
    QString t = normalizeToIso(toIso);
    if (f.isEmpty() || t.isEmpty()) return;
    QString lbl = !label.isEmpty() ? label : QString("%1 To %2").arg(formatDisplayDate(f), formatDisplayDate(t));
    DatabaseManager::instance().setSetting("last_selected_fy", lbl);
    DatabaseManager::instance().setSetting("last_from_date", f);
    DatabaseManager::instance().setSetting("last_to_date", t);
    AccountingEngine::setActivePeriod(f, t, lbl);
}

FiscalYearInfo FiscalYearHelper::getFiscalYearForDate(const QString& dateStr) {
    QString iso = normalizeToIso(dateStr);
    if (iso.isEmpty()) return getActiveFiscalYear();

    QVariantList rows = DatabaseManager::instance().executeQuery(
        "SELECT year_name, start_date, end_date, is_active, is_locked FROM financial_years WHERE start_date <= ? AND end_date >= ? LIMIT 1;",
        {iso, iso}
    );
    if (!rows.isEmpty()) {
        QVariantMap r = rows.first().toMap();
        FiscalYearInfo info;
        info.name = r.value("year_name").toString().trimmed();
        info.startDate = normalizeToIso(r.value("start_date").toString());
        info.endDate = normalizeToIso(r.value("end_date").toString());
        info.isActive = r.value("is_active").toBool();
        info.isLocked = r.value("is_locked").toBool();
        return info;
    }

    // Calculate Indian FY from date components
    QStringList p = iso.split('-');
    if (p.size() == 3) {
        int y = p[0].toInt();
        int m = p[1].toInt();
        int startY = (m >= 4) ? y : y - 1;
        int endY = startY + 1;
        FiscalYearInfo info;
        info.name = QString("FY %1-%2").arg(startY).arg(QString::number(endY).right(2));
        info.startDate = QString("%1-04-01").arg(startY);
        info.endDate = QString("%1-03-31").arg(endY);
        return info;
    }

    return getActiveFiscalYear();
}

FiscalYearInfo FiscalYearHelper::getFiscalYearByName(const QString& fyName) {
    QString cleanName = fyName.trimmed();
    if (cleanName.isEmpty()) return getActiveFiscalYear();

    QVariantList rows = DatabaseManager::instance().executeQuery(
        "SELECT year_name, start_date, end_date, is_active, is_locked FROM financial_years WHERE year_name = ? OR year_name LIKE ? LIMIT 1;",
        {cleanName, "%" + cleanName + "%"}
    );
    if (!rows.isEmpty()) {
        QVariantMap r = rows.first().toMap();
        FiscalYearInfo info;
        info.name = r.value("year_name").toString().trimmed();
        info.startDate = normalizeToIso(r.value("start_date").toString());
        info.endDate = normalizeToIso(r.value("end_date").toString());
        info.isActive = r.value("is_active").toBool();
        info.isLocked = r.value("is_locked").toBool();
        return info;
    }

    return getActiveFiscalYear();
}

QString FiscalYearHelper::extractFiscalYearToken(const QString& str) {
    if (str.isEmpty()) return QString();

    // 1. Matches formats like "2627", "2526", "2425", "2324" (YY(YY+1))
    // e.g. "MRI/2627-247", "2627-247", "STC2627-01", "2627/Sale-247"
    static const QRegularExpression yyYyRe(R"((?:^|[^0-9])([2-9][0-9])([0-9]{2})(?:[^0-9]|$))");
    auto match = yyYyRe.match(str);
    if (match.hasMatch()) {
        int y1 = match.captured(1).toInt();
        int y2 = match.captured(2).toInt();
        if (y2 == y1 + 1 || (y1 == 99 && y2 == 0)) {
            int fullY1 = 2000 + y1;
            int fullY2 = 2000 + y2;
            return QString("FY %1-%2").arg(fullY1).arg(QString::number(fullY2).right(2));
        }
    }

    // 2. Matches formats like "23-24", "24-25", "25-26", "26-27" (YY-YY or YY/YY)
    // e.g. "MRI/23-24/247"
    static const QRegularExpression yyDashYyRe(R"((?:^|[^0-9])([2-9][0-9])[-/]([0-9]{2})(?:[^0-9]|$))");
    match = yyDashYyRe.match(str);
    if (match.hasMatch()) {
        int y1 = match.captured(1).toInt();
        int y2 = match.captured(2).toInt();
        if (y2 == y1 + 1 || (y1 == 99 && y2 == 0)) {
            int fullY1 = 2000 + y1;
            int fullY2 = 2000 + y2;
            return QString("FY %1-%2").arg(fullY1).arg(QString::number(fullY2).right(2));
        }
    }

    // 3. Matches full formats like "2024-25", "2025-26", "2026-27"
    static const QRegularExpression fullFyRe(R"((?:FY\s*)?(20[2-9][0-9])[-/](20[2-9][0-9]|[0-9]{2}))", QRegularExpression::CaseInsensitiveOption);
    match = fullFyRe.match(str);
    if (match.hasMatch()) {
        int y1 = match.captured(1).toInt();
        QString y2Str = match.captured(2);
        int y2 = y2Str.length() == 4 ? y2Str.toInt() : (2000 + y2Str.toInt());
        if (y2 == y1 + 1) {
            return QString("FY %1-%2").arg(y1).arg(QString::number(y2).right(2));
        }
    }

    return QString();
}

FiscalYearInfo FiscalYearHelper::resolveFiscalYear(const QString& invOrVchStr, const QString& dateHint, const QString& explicitFy) {
    // 1. Try explicit FY first if given and valid
    if (!explicitFy.trimmed().isEmpty()) {
        FiscalYearInfo fy = getFiscalYearByName(explicitFy.trimmed());
        if (fy.isValid()) return fy;
    }

    // 2. Try extracting FY token from the invoice/voucher text (e.g. "MRI/2627-247" -> "FY 2026-27")
    QString token = extractFiscalYearToken(invOrVchStr);
    if (!token.isEmpty()) {
        FiscalYearInfo fy = getFiscalYearByName(token);
        if (fy.isValid()) return fy;
    }

    // 3. Try dateHint if valid
    QString iso = normalizeToIso(dateHint);
    if (!iso.isEmpty()) {
        FiscalYearInfo fy = getFiscalYearForDate(iso);
        if (fy.isValid()) return fy;
    }

    // 4. Default to currently active fiscal year
    return getActiveFiscalYear();
}

QString FiscalYearHelper::fyShortToken(const QString& fyName) {
    QString s = fyName.trimmed();
    // Accept "FY 2026-27", "2026-27", "2026/27", "2627", "FY 26-27".
    static const QRegularExpression fullRe(R"(20(\d{2})\D*(\d{2}))");
    auto m = fullRe.match(s);
    if (m.hasMatch()) {
        int y1 = m.captured(1).toInt();
        int y2 = m.captured(2).toInt();
        if ((y1 + 1) % 100 == y2) {
            return QString("%1%2").arg(y1, 2, 10, QChar('0')).arg(y2, 2, 10, QChar('0'));
        }
    }
    static const QRegularExpression shortRe(R"(^(\d{2})(\d{2})$)");
    auto m2 = shortRe.match(s.remove(' ').remove('-').remove('/'));
    if (m2.hasMatch()) {
        int y1 = m2.captured(1).toInt();
        int y2 = m2.captured(2).toInt();
        if ((y1 + 1) % 100 == y2) return m2.captured(1) + m2.captured(2);
    }
    return QString();
}

QString FiscalYearHelper::canonicalTypeToken(const QString& type) {
    // Must mirror VouchersModel::get_next_voucher_no alias groups exactly:
    // same family in, same token out, whether the caller is migration or
    // live voucher generation. Cash/bank families stay distinct
    // (Pymt vs ChPt, Rcpt vs ChRt) — merging them would collide sequences.
    QString t = type.trimmed().toLower();
    t.remove(' ');
    t.remove('-');
    if (t.isEmpty() || t == "jv" || t == "j") return "Jrnl";
    if (t.startsWith("sale") || t.startsWith("slrn") || t == "sl") return "Sale";
    if (t.startsWith("purc") || t.startsWith("prrn") || t == "pur" || t == "pr" || t == "pu") return "Purc";
    if (t == "cn" || t == "cntr" || t == "c") return "Contra";
    if (t == "rc" || t == "r") return "Rcpt";
    if (t == "p") return "Pymt";
    if (t.startsWith("jrnl") || t.startsWith("journ")) return "Jrnl";
    if (t == "cashpayment" || t == "pymt" || t == "pym" || t == "py") return "Pymt";
    if (t.startsWith("chpt") || t == "chequepayment" || t == "payment" || t.startsWith("paym") || t.startsWith("chqpy")) return "ChPt";
    if (t == "cashreceipt" || t == "rcpt" || t == "rcp" || t == "rct") return "Rcpt";
    if (t.startsWith("chrt") || t == "chequereceipt" || t == "receipt" || t.startsWith("rece") || t.startsWith("chqrc")) return "ChRt";
    if (t.startsWith("jfrm") || t.startsWith("jform")) return "JFrm";
    if (t.startsWith("ifrm") || t.startsWith("iform")) return "IFrm";
    if (t.startsWith("contra")) return "Contra";
    if (t.startsWith("tds")) return "TDS";
    if (t.startsWith("tcs")) return "TCS";
    if (t.startsWith("crnt") || t.startsWith("creditnote") || t == "cr") return "CrNt";
    if (t.startsWith("dbnt") || t.startsWith("debitnote") || t == "db") return "DbNt";
    if (t.startsWith("mill")) return "Mill";
    // Unknown family: reuse verbatim (capitalized) so the transform is lossless.
    QString out = type.trimmed();
    if (!out.isEmpty()) out[0] = out[0].toUpper();
    return out;
}

bool FiscalYearHelper::parseCanonicalVoucherNo(const QString& vchNo, QString& fyTokenOut, QString& typeOut, QString& rawOut) {
    fyTokenOut.clear();
    typeOut.clear();
    rawOut.clear();
    static const QRegularExpression canonRe(R"(^(\d{4})/([A-Za-z]+)-(.+)$)");
    auto m = canonRe.match(vchNo.trimmed());
    if (!m.hasMatch()) return false;
    QString tok = m.captured(1);
    int y1 = tok.left(2).toInt();
    int y2 = tok.right(2).toInt();
    if ((y1 + 1) % 100 != y2) return false;
    fyTokenOut = tok;
    typeOut = m.captured(2);
    rawOut = m.captured(3).trimmed();
    if (rawOut.isEmpty()) return false;
    return true;
}

QString FiscalYearHelper::canonicalVoucherNo(const QString& fyNameOrToken, const QString& type, const QString& rawNo) {
    QString raw = rawNo.trimmed();
    if (raw.isEmpty()) return raw;
    // Idempotent: canonical input passes through untouched.
    QString pFy, pType, pRaw;
    if (parseCanonicalVoucherNo(raw, pFy, pType, pRaw)) return raw;
    QString typeTok = canonicalTypeToken(type);
    // Already FY-qualified source numbering (Busy "1/2026-27", firm series
    // like "MRI/2526-247"): keep byte-identical, never double-prefix.
    if (raw.contains('/') && !extractFiscalYearToken(raw).isEmpty()) return raw;
    // Legacy app shape ("Sale-247") with the same family: drop the duplicated
    // type affix so the result is "2627/Sale-247", not "2627/Sale-Sale-247".
    if (raw.startsWith(typeTok + "-", Qt::CaseInsensitive)) raw = raw.mid(typeTok.length() + 1).trimmed();
    if (raw.isEmpty()) return rawNo.trimmed();
    QString tok = fyShortToken(fyNameOrToken);
    if (tok.isEmpty()) {
        // Last resort: derive from embedded token, else active FY.
        tok = extractFiscalYearToken(raw);
        if (!tok.isEmpty()) tok = fyShortToken(tok);
        if (tok.isEmpty()) {
            FiscalYearInfo afy = getActiveFiscalYear();
            tok = fyShortToken(afy.name);
        }
    }
    if (tok.isEmpty()) return raw;
    return QString("%1/%2-%3").arg(tok, typeTok, raw);
}

QString FiscalYearHelper::rawVoucherNo(const QString& vchNo) {
    QString s = vchNo.trimmed();
    if (s.isEmpty()) return s;
    // Strip canonical "yyYY/" prefix.
    static const QRegularExpression canonPreRe(R"(^\d{4}/)");
    s.remove(canonPreRe);
    // Strip one leading family token ("Sale-"/"Purc-"/...). Longest-first so
    // "Purchase-" beats "Pur"-style prefixes; canonical tokens are exact.
    static const QStringList tokens = {
        "Purchase", "Contra", "Journal", "Payment", "Receipt",
        "Sale", "Purc", "Jrnl", "ChPt", "ChRt", "Pymt", "Rcpt",
        "JFrm", "IFrm", "CrNt", "DbNt", "Mill", "TDS", "TCS",
        "Pur", "Jr", "J"
    };
    for (const QString& tk : tokens) {
        if (s.startsWith(tk + "-", Qt::CaseInsensitive)) {
            s = s.mid(tk.length() + 1).trimmed();
            break;
        }
    }
    return s;
}

QList<FiscalYearInfo> FiscalYearHelper::getAllFiscalYears() {
    ensureFiscalYearsDiscovered();
    QList<FiscalYearInfo> list;
    QVariantList rows = DatabaseManager::instance().executeQuery(
        "SELECT year_name, start_date, end_date, is_active, is_locked FROM financial_years ORDER BY start_date ASC;"
    );
    for (const auto& rowVar : rows) {
        QVariantMap r = rowVar.toMap();
        FiscalYearInfo info;
        info.name = r.value("year_name").toString().trimmed();
        info.startDate = normalizeToIso(r.value("start_date").toString());
        info.endDate = normalizeToIso(r.value("end_date").toString());
        info.isActive = r.value("is_active").toBool();
        info.isLocked = r.value("is_locked").toBool();
        list.append(info);
    }
    return list;
}

void FiscalYearHelper::clampDateRangeToFiscalYear(QString& fromIso, QString& toIso, const FiscalYearInfo& fy) {
    fromIso = normalizeToIso(fromIso);
    toIso = normalizeToIso(toIso);

    if (fromIso.isEmpty() || fromIso < fy.startDate || fromIso > fy.endDate) {
        fromIso = fy.startDate;
    }
    if (toIso.isEmpty() || toIso > fy.endDate || toIso < fy.startDate) {
        toIso = fy.endDate;
    }
    if (fromIso > toIso) {
        fromIso = fy.startDate;
        toIso = fy.endDate;
    }
}

bool FiscalYearHelper::isDateInFiscalYear(const QString& dateStr, const FiscalYearInfo& fy) {
    QString iso = normalizeToIso(dateStr);
    if (iso.isEmpty()) return false;
    return (iso >= fy.startDate && iso <= fy.endDate);
}

bool FiscalYearHelper::isDatePriorTo(const QString& dateStr, const QString& boundaryIso) {
    QString iso = normalizeToIso(dateStr);
    if (iso.isEmpty()) return false;
    return (iso < boundaryIso);
}

PartitionedLedgerData FiscalYearHelper::partitionPartyTransactions(
    const QString& partyName,
    const QString& requestedFromDate,
    const QString& requestedToDate
) {
    PartitionedLedgerData result;
    QString fIso = normalizeToIso(requestedFromDate);
    QString tIso = normalizeToIso(requestedToDate);
    if (!fIso.isEmpty()) {
        result.activeFy = getFiscalYearForDate(fIso);
    } else {
        result.activeFy = getActiveFiscalYear();
    }
    clampDateRangeToFiscalYear(fIso, tIso, result.activeFy);

    result.effectiveFromDate = fIso;
    result.effectiveToDate = tIso;

    QString cleanName = partyName.trimmed();
    if (cleanName.isEmpty()) {
        return result;
    }

    QString namePattern = cleanName;
    namePattern.replace(QChar(0x00A0), '%');
    namePattern.replace(' ', '%');
    QString wildcard = "%" + namePattern + "%";

    // 1. Fetch party info from master with exact match
    QVariantList pRows = DatabaseManager::instance().executeQuery(
        "SELECT id, legacy_id, name, opening_balance, balance_type FROM parties WHERE name = ? COLLATE NOCASE OR alias = ? COLLATE NOCASE LIMIT 1;",
        {cleanName, cleanName}
    );
    int partyId = 0;
    int legacyCode = 0;
    double initialOp = 0.0;
    QString initialOpType = "Cr";
    if (!pRows.isEmpty()) {
        partyId = pRows.first().toMap().value("id").toInt();
        legacyCode = pRows.first().toMap().value("legacy_id").toInt();
        initialOp = pRows.first().toMap().value("opening_balance").toDouble();
        initialOpType = pRows.first().toMap().value("balance_type").toString();
    }

    // 2. Compute Prior Transactions (All vouchers strictly before effectiveFromDate)
    double priorDr = 0.0;
    double priorCr = 0.0;
    if (initialOpType.compare("Dr", Qt::CaseInsensitive) == 0) {
        priorDr += initialOp;
    } else {
        priorCr += initialOp;
    }

    if (!result.effectiveFromDate.isEmpty()) {
        QVariantList priorRows;
        if (partyId > 0 || legacyCode > 0) {
            priorRows = DatabaseManager::instance().executeQuery(
                "SELECT dr_cr, SUM(amount) as total_amt FROM transactions "
                "WHERE (party_id = ? OR (account_code > 0 AND account_code = ?)) "
                "AND voucher_date < ? GROUP BY dr_cr;",
                {partyId, legacyCode, result.effectiveFromDate}
            );
        } else {
            priorRows = DatabaseManager::instance().executeQuery(
                "SELECT dr_cr, SUM(amount) as total_amt FROM transactions "
                "WHERE party_name = ? COLLATE NOCASE "
                "AND voucher_date < ? GROUP BY dr_cr;",
                {cleanName, result.effectiveFromDate}
            );
        }
        for (const auto& pr : priorRows) {
            QVariantMap m = pr.toMap();
            if (m.value("dr_cr").toString().compare("Dr", Qt::CaseInsensitive) == 0) {
                priorDr += m.value("total_amt").toDouble();
            } else {
                priorCr += m.value("total_amt").toDouble();
            }
        }
    }

    result.priorDebit = priorDr;
    result.priorCredit = priorCr;
    double netOp = priorDr - priorCr;
    result.netOpeningBalance = std::abs(netOp);
    result.openingBalanceType = (netOp >= 0) ? "Dr" : "Cr";

    // 3. Generate Opening Balance entry if non-zero
    if (std::abs(netOp) > 0.001) {
        LedgerStatementEntry opEntry;
        opEntry.id = -1;
        opEntry.isSelected = false;
        opEntry.vIso = result.effectiveFromDate;
        opEntry.vDate = formatDisplayDate(result.effectiveFromDate);
        opEntry.refNo = "OP-BAL";
        opEntry.voucherNo = "OP";
        opEntry.invoiceNo = "";
        opEntry.voucherType = "OBal";
        opEntry.legacyType = "OBal";
        opEntry.transType = "OBal";
        opEntry.particulars = QString("Opening Balance (%1)").arg(result.openingBalanceType);
        opEntry.amount = std::abs(netOp);
        opEntry.amountFmt = AccountingEngine::formatIndianCurrency(opEntry.amount, true);
        opEntry.financialYear = result.activeFy.name;
        opEntry.side = result.openingBalanceType;

        if (result.openingBalanceType == "Dr") {
            result.drEntries.append(opEntry);
        } else {
            result.crEntries.append(opEntry);
        }
    }

    // 4. Fetch Fiscal Transactions (Strictly between effectiveFromDate and effectiveToDate)
    QString sql;
    QVariantList params;
    if (partyId > 0 || legacyCode > 0) {
        sql = "SELECT id, voucher_no, voucher_date, voucher_type, trans_type, opposing_account, dr_cr, amount, "
              "invoice_no, narration, financial_year, broker_name, vehicle_no, gr_no, taxable_amount, tds_amount, party_id, party_name "
              "FROM transactions "
              "WHERE (party_id = ? OR (account_code > 0 AND account_code = ?)) "
              "AND voucher_date >= ? AND voucher_date <= ? "
              "ORDER BY voucher_date ASC, CAST(voucher_no AS INTEGER) ASC, id ASC;";
        params = {partyId, legacyCode, result.effectiveFromDate, result.effectiveToDate};
    } else {
        sql = "SELECT id, voucher_no, voucher_date, voucher_type, trans_type, opposing_account, dr_cr, amount, "
              "invoice_no, narration, financial_year, broker_name, vehicle_no, gr_no, taxable_amount, tds_amount, party_id, party_name "
              "FROM transactions "
              "WHERE party_name = ? COLLATE NOCASE "
              "AND voucher_date >= ? AND voucher_date <= ? "
              "ORDER BY voucher_date ASC, CAST(voucher_no AS INTEGER) ASC, id ASC;";
        params = {cleanName, result.effectiveFromDate, result.effectiveToDate};
    }
    QVariantList rows = DatabaseManager::instance().executeQuery(sql, params);

    for (const auto& r : rows) {
        QVariantMap t = r.toMap();
        int vId = t.value("id").toInt();
        int rPartyId = t.value("party_id").toInt();
        QString rPartyName = t.value("party_name").toString().trimmed();
        QString vNo = t.value("voucher_no").toString();
        QString rawType = t.value("trans_type").toString();
        QString vType = t.value("voucher_type").toString();
        QString opposing = t.value("opposing_account").toString();
        QString drCr = t.value("dr_cr").toString();
        double amt = t.value("amount").toDouble();
        QString invNo = t.value("invoice_no").toString();
        QString narr = t.value("narration").toString().trimmed();
        QString veh = t.value("vehicle_no").toString().trimmed();
        QString broker = t.value("broker_name").toString().trimmed();
        QString isoD = normalizeToIso(t.value("voucher_date").toString());
        QString fmtD = formatDisplayDate(isoD);

        // Zero-leakage sanity check
        if (isoD < result.effectiveFromDate || isoD > result.effectiveToDate) {
            continue;
        }

        if (rawType == "TDS" || vType == "TDS") {
            rawType = "TDS";
            vType = "TDS";
        }
        QString displayRef = !rawType.isEmpty() ? QString("%1 %2").arg(rawType, vNo).trimmed() : QString("%1 %2").arg(vType, vNo).trimmed();
        if (displayRef.isEmpty()) displayRef = vNo;

        QString desc;
        if (rawType == "Sale" || vType == "Sales") {
            desc = QString("Sales Invoice: %1").arg(!invNo.isEmpty() ? invNo : vNo);
        } else if (rawType == "Purc" || vType == "Purchase") {
            desc = QString("Purchase Bill: %1").arg(!invNo.isEmpty() ? invNo : vNo);
        } else if (rawType == "ChRt" || rawType == "Rcpt" || vType == "Receipt") {
            desc = QString("Receipt via %1").arg(!opposing.isEmpty() ? opposing : "Bank/Cash");
        } else if (rawType == "ChPt" || rawType == "Pymt" || vType == "Payment") {
            desc = QString("Payment to %1").arg(!opposing.isEmpty() ? opposing : "Bank/Cash");
        } else if (rawType == "TDS" || vType == "TDS") {
            desc = !narr.isEmpty() ? narr : QString("TDS: %1").arg(!opposing.isEmpty() ? opposing : "TDS");
        } else if (rawType == "Jrnl" || vType == "Journal") {
            desc = QString("Journal: %1").arg(!opposing.isEmpty() ? opposing : "A/c");
        } else if (rawType == "JFrm" || vType == "J-Form") {
            desc = QString("J-Form: %1").arg(!opposing.isEmpty() ? opposing : "Paddy Purchase");
        } else {
            desc = QString("%1: %2").arg(vType, opposing);
        }

        double tdsAmt = t.value("tds_amount").toDouble();
        if ((rawType == "Purc" || vType == "Purchase") && tdsAmt > 0.0) {
            LedgerStatementEntry item;
            item.id = vId;
            item.partyId = rPartyId;
            item.partyName = rPartyName;
            item.isSelected = false;
            item.vIso = isoD;
            item.vDate = fmtD;
            item.refNo = displayRef;
            item.voucherNo = vNo;
            item.invoiceNo = invNo;
            item.voucherType = vType;
            item.legacyType = rawType;
            item.transType = rawType;
            item.particulars = QString("B.No. %1").arg(!invNo.isEmpty() ? invNo : vNo);
            if (!veh.isEmpty()) item.particulars += " | Veh: " + veh;
            if (!broker.isEmpty()) item.particulars += " | Broker: " + broker;
            if (!narr.isEmpty()) item.particulars += " | " + narr;
            item.amount = amt + tdsAmt;
            item.amountFmt = AccountingEngine::formatIndianCurrency(item.amount, true);
            item.financialYear = result.activeFy.name;
            item.side = "Cr";
            result.crEntries.append(item);

            LedgerStatementEntry tdsItem;
            tdsItem.id = vId;
            tdsItem.partyId = rPartyId;
            tdsItem.partyName = rPartyName;
            tdsItem.isSelected = false;
            tdsItem.vIso = isoD;
            tdsItem.vDate = fmtD;
            tdsItem.refNo = displayRef;
            tdsItem.voucherNo = vNo;
            tdsItem.invoiceNo = invNo;
            tdsItem.voucherType = "TDS";
            tdsItem.legacyType = "TDS";
            tdsItem.transType = "TDS";
            tdsItem.particulars = QString("T.D.S. U/S 194Q (B.No. %1)").arg(!invNo.isEmpty() ? invNo : vNo);
            tdsItem.amount = tdsAmt;
            tdsItem.amountFmt = AccountingEngine::formatIndianCurrency(tdsItem.amount, true);
            tdsItem.financialYear = result.activeFy.name;
            tdsItem.side = "Dr";
            result.drEntries.append(tdsItem);
        } else {
            LedgerStatementEntry entry;
            entry.id = vId;
            entry.partyId = rPartyId;
            entry.partyName = rPartyName;
            entry.isSelected = false;
            entry.vIso = isoD;
            entry.vDate = fmtD;
            entry.refNo = displayRef;
            entry.voucherNo = vNo;
            entry.invoiceNo = invNo;
            entry.voucherType = vType;
            entry.legacyType = rawType;
            entry.transType = !rawType.isEmpty() ? rawType : vType;
            entry.particulars = desc;
            entry.amount = amt;
            entry.amountFmt = AccountingEngine::formatIndianCurrency(amt, true);
            entry.financialYear = result.activeFy.name;
            entry.side = drCr;

            if (drCr.compare("Dr", Qt::CaseInsensitive) == 0) {
                result.drEntries.append(entry);
            } else {
                result.crEntries.append(entry);
            }
        }
    }

    return result;
}
