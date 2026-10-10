#include "bill_series.h"
#include "../database_manager.h"
#include "accounting_engine.h"
#include "fiscal_year_helper.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QMap>
#include <QRegularExpression>

namespace {

// "FY 2026-27" -> (26, 27). False for non-FY strings.
bool splitFyLong(const QString& fyLongName, int& y1, int& y2full) {
    static const QRegularExpression re(R"(20(\d{2})\D*(\d{2})\s*$)");
    auto m = re.match(fyLongName.trimmed());
    if (!m.hasMatch()) return false;
    y1 = m.captured(1).toInt();
    int y2 = m.captured(2).toInt();
    if ((y1 + 1) % 100 != y2) return false;
    y2full = 2000 + y2;
    return true;
}

// Dominant trailing-digit prefix across the whole table, e.g. "MRI/2627-"
// with the width of its highest sequence. Used ONLY when no config row was
// ever saved: the firm's own history becomes the config (adopt, don't hardcode).
bool adoptFromHistory(const QString& table, BillSeriesConfig& out) {
    if (table != "sales_invoices" && table != "purchase_invoices") return false;
    QVariantList rows = DatabaseManager::instance().executeQuery(
        "SELECT invoice_no FROM " + table + " WHERE invoice_no IS NOT NULL AND invoice_no != '';");
    if (rows.isEmpty()) return false;

    static const QRegularExpression tailRe(R"(^(.*?)(\d+)$)");
    QMap<QString, int> prefixCount;
    QMap<QString, long long> prefixMax;
    QMap<QString, int> prefixWidth;
    for (const QVariant& r : rows) {
        QString inv = r.toMap().value("invoice_no").toString().trimmed();
        if (inv.isEmpty()) continue;
        auto m = tailRe.match(inv);
        if (!m.hasMatch()) continue; // rows without a trailing sequence don't vote
        QString pfx = m.captured(1);
        long long num = m.captured(2).toLongLong();
        prefixCount[pfx]++;
        if (num > prefixMax.value(pfx, 0)) {
            prefixMax[pfx] = num;
            prefixWidth[pfx] = m.captured(2).length();
        }
    }
    if (prefixCount.isEmpty()) return false;

    QString best;
    int bestCount = -1;
    long long bestNum = -1;
    for (auto it = prefixCount.constBegin(); it != prefixCount.constEnd(); ++it) {
        long long n = prefixMax.value(it.key(), 0);
        if (it.value() > bestCount || (it.value() == bestCount && n > bestNum)) {
            bestCount = it.value();
            bestNum = n;
            best = it.key();
        }
    }

    BillSeriesConfig cfg;
    cfg.seqWidth = qBound(1, prefixWidth.value(best, 1), 10);
    cfg.startNumber = 1;
    cfg.resetEachFy = true;

    // Try structured split: <prefix><sep><FY token><sep>, e.g. "MRI/2627-".
    static const QRegularExpression structRe(
        R"(^(.*?)([\/\-\s]+)?(\d{4}|\d{2}[\-\/]\d{2}|20\d{2}[\-\/]\d{2})([\/\-\s]+)?$)");
    auto sm = structRe.match(best);
    bool structured = false;
    if (sm.hasMatch()) {
        QString mid = sm.captured(3);
        QString digits = mid;
        digits.remove('-').remove('/');
        int y1 = -1, y2 = -1;
        if (digits.length() == 8) { // YYYY-YY
            int f1 = digits.left(4).toInt(), f2 = digits.mid(4, 2).toInt();
            if (f2 == (f1 + 1) % 100) { y1 = f1 % 100; y2 = f2; cfg.fyStyle = "YYYY-YY"; structured = true; }
        } else if (digits.length() == 4) { // yyYY or yy-yy
            y1 = digits.left(2).toInt(); y2 = digits.mid(2, 2).toInt();
            if (y2 == (y1 + 1) % 100) {
                cfg.fyStyle = mid.contains('-') || mid.contains('/') ? "yy-yy" : "yyYY";
                structured = true;
            }
        }
        if (structured) {
            cfg.prefix = sm.captured(1);
            cfg.sepAfterPrefix = sm.captured(2);
            cfg.sepBeforeSeq = sm.captured(4);
        }
    }
    if (!structured) {
        // Opaque legacy head (e.g. "PUR-"): match it verbatim, no FY token.
        cfg.prefix = best;
        cfg.sepAfterPrefix = "";
        cfg.fyStyle = "none";
        cfg.sepBeforeSeq = "";
    }
    out = cfg;
    return true;
}

} // namespace

QString BillSeriesConfig::fyToken(const QString& fyLongName) const {
    if (fyStyle == "none") return QString();
    int y1 = 0, y2full = 0;
    if (!splitFyLong(fyLongName, y1, y2full)) {
        // Fall back to the generic helper (accepts "2627", "26-27", ...).
        QString shortTok = FiscalYearHelper::fyShortToken(fyLongName);
        if (shortTok.length() != 4) return QString();
        y1 = shortTok.left(2).toInt();
        y2full = 2000 + shortTok.mid(2, 2).toInt();
    }
    int y1full = 2000 + y1;
    int y2 = y2full % 100;
    if (fyStyle == "yy-yy") {
        return QString("%1-%2").arg(y1, 2, 10, QChar('0')).arg(y2, 2, 10, QChar('0'));
    }
    if (fyStyle == "YYYY-YY") {
        return QString("%1-%2").arg(y1full).arg(y2, 2, 10, QChar('0'));
    }
    return QString("%1%2").arg(y1, 2, 10, QChar('0')).arg(y2, 2, 10, QChar('0')); // yyYY
}

QString BillSeriesConfig::staticHead(const QString& fyLongName) const {
    QString head = prefix;
    QString fy = fyToken(fyLongName);
    if (!head.isEmpty() && !fy.isEmpty()) head += sepAfterPrefix;
    head += fy;
    if (!head.isEmpty()) head += sepBeforeSeq;
    return head;
}

QString BillSeriesConfig::render(long long seq, const QString& fyLongName) const {
    QString num = QString::number(seq);
    int w = qBound(1, seqWidth, 10);
    if (w > 1 && num.length() < w) num = num.rightJustified(w, '0');
    return staticHead(fyLongName) + num + suffix;
}

QString BillSeriesConfig::toJson() const {
    QJsonObject o;
    o["prefix"] = prefix;
    o["sepAfterPrefix"] = sepAfterPrefix;
    o["fyStyle"] = fyStyle;
    o["sepBeforeSeq"] = sepBeforeSeq;
    o["seqWidth"] = seqWidth;
    o["startNumber"] = QString::number(startNumber);
    o["resetEachFy"] = resetEachFy;
    o["suffix"] = suffix;
    return QString::fromUtf8(QJsonDocument(o).toJson(QJsonDocument::Compact));
}

BillSeriesConfig BillSeriesConfig::fromJson(const QString& json, bool* ok) {
    BillSeriesConfig cfg;
    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(json.toUtf8(), &err);
    if (err.error != QJsonParseError::NoError || !doc.isObject()) {
        if (ok) *ok = false;
        return cfg;
    }
    QJsonObject o = doc.object();
    cfg.prefix = o.value("prefix").toString(cfg.prefix);
    cfg.sepAfterPrefix = o.value("sepAfterPrefix").toString(cfg.sepAfterPrefix);
    cfg.fyStyle = o.value("fyStyle").toString(cfg.fyStyle);
    cfg.sepBeforeSeq = o.value("sepBeforeSeq").toString(cfg.sepBeforeSeq);
    cfg.seqWidth = o.value("seqWidth").toInt(cfg.seqWidth);
    cfg.startNumber = o.value("startNumber").toString(QString::number(cfg.startNumber)).toLongLong();
    cfg.resetEachFy = o.value("resetEachFy").toBool(cfg.resetEachFy);
    cfg.suffix = o.value("suffix").toString(cfg.suffix);
    if (!cfg.validate().isEmpty()) {
        if (ok) *ok = false;
        return BillSeriesConfig();
    }
    if (ok) *ok = true;
    return cfg;
}

QString BillSeriesConfig::validate() const {
    static const QStringList styles = {"yyYY", "yy-yy", "YYYY-YY", "none"};
    if (!styles.contains(fyStyle)) return "Unknown FY style.";
    if (prefix.length() > 12) return "Prefix is too long (max 12).";
    if (!prefix.isEmpty() && prefix.right(1).at(0).isDigit())
        return "Prefix must not end with a digit (it would merge into the sequence).";
    if (sepAfterPrefix.length() > 3 || sepBeforeSeq.length() > 3)
        return "Separators are too long (max 3 characters).";
    if (seqWidth < 1 || seqWidth > 8) return "Sequence width must be 1-8 (1 = plain, e.g. -11).";
    if (startNumber < 0 || startNumber > 999999999) return "Start number is out of range.";
    if (!suffix.isEmpty() && suffix.at(0).isDigit())
        return "Suffix must not start with a digit (it would merge into the sequence).";
    if (suffix.length() > 8) return "Suffix is too long (max 8).";
    return QString();
}

QString BillSeriesManager::settingsKey(const QString& docType) {
    return "bill_series/" + docType.trimmed().toLower();
}

BillSeriesConfig BillSeriesManager::load(const QString& docType, const QString& table) {
    QString stored = DatabaseManager::instance().getSetting(settingsKey(docType));
    if (!stored.trimmed().isEmpty()) {
        bool ok = false;
        BillSeriesConfig cfg = BillSeriesConfig::fromJson(stored, &ok);
        if (ok) return cfg;
        // Corrupt row: fall through to adoption (never crash on bad JSON).
    }
    BillSeriesConfig adopted;
    if (adoptFromHistory(table, adopted)) return adopted;
    return BillSeriesConfig(); // neutral factory default ("INV/yyYY-…")
}

bool BillSeriesManager::save(const QString& docType, const BillSeriesConfig& cfg) {
    if (!cfg.validate().isEmpty()) return false;
    return DatabaseManager::instance().setSetting(settingsKey(docType), cfg.toJson());
}

QString BillSeriesManager::nextNumber(const QString& docType, const QString& table,
                                      const QString& fyOrDate) {
    // Table names are code constants; refuse anything else (no SQL interpolation).
    if (table != "sales_invoices" && table != "purchase_invoices") return QString();
    return nextForConfig(load(docType, table), table, fyOrDate);
}

QString BillSeriesManager::nextForConfig(const BillSeriesConfig& cfg, const QString& table,
                                         const QString& fyOrDate) {
    if (table != "sales_invoices" && table != "purchase_invoices") return QString();
    QString targetFy = AccountingEngine::resolveFinancialYear(fyOrDate);

    QVariantList rows;
    if (cfg.resetEachFy) {
        QString fyPattern = "%" + targetFy.mid(3).trimmed() + "%";
        rows = DatabaseManager::instance().executeQuery(
            "SELECT invoice_no FROM " + table + " WHERE (financial_year = ? OR financial_year LIKE ?) "
            "AND invoice_no IS NOT NULL AND invoice_no != '';",
            {targetFy, fyPattern});
    } else {
        rows = DatabaseManager::instance().executeQuery(
            "SELECT invoice_no FROM " + table + " WHERE invoice_no IS NOT NULL AND invoice_no != '';");
    }

    static const QRegularExpression tailRe(R"(^(.*?)(\d+)$)");
    static const QRegularExpression leadRe(R"(^(\d+))");
    const QString head = cfg.staticHead(targetFy);
    long long seriesMax = 0, guardMax = 0;
    for (const QVariant& r : rows) {
        QString inv = r.toMap().value("invoice_no").toString().trimmed();
        if (inv.isEmpty()) continue;
        auto tm = tailRe.match(inv);
        if (tm.hasMatch()) guardMax = qMax(guardMax, tm.captured(2).toLongLong());
        if (!head.isEmpty() && !inv.startsWith(head, Qt::CaseInsensitive)) continue;
        QString rest = head.isEmpty() ? inv : inv.mid(head.length());
        auto lm = leadRe.match(rest);
        if (lm.hasMatch()) seriesMax = qMax(seriesMax, lm.captured(1).toLongLong());
    }

    long long base = seriesMax > 0 ? seriesMax : guardMax;
    long long seq = qMax(base + 1, cfg.startNumber);
    if (seq < 1) seq = 1;
    return cfg.render(seq, targetFy);
}
