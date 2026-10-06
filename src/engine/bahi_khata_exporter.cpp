#include "bahi_khata_exporter.h"
#include "../database_manager.h"
#include "fiscal_year_helper.h"
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QFileDialog>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QDateTime>
#include <QRegularExpression>
#include <QDebug>
#include <cmath>
#include <set>
#include <tuple>
#include <iostream>

#if defined(HAS_LIBMDB) || __has_include("mdbtools.h")
#include "mdbtools.h"
#define USE_LIBMDB 1

extern "C" ssize_t mdb_write_pg(MdbHandle *mdb, unsigned long pg);
#endif

namespace MahadevERP {

BahiKhataExporter::BahiKhataExporter(QObject* parent)
    : QObject(parent)
{
}

BahiKhataExporter::~BahiKhataExporter()
{
}

void BahiKhataExporter::updateProgress(int percent, const QString& status) {
    m_progressPercent = percent;
    m_statusText = status;
    emit progressChanged(percent);
    emit statusChanged(status);
    emit exportProgress(percent, status);
}

double BahiKhataExporter::round2(double val) {
    return std::round(val * 100.0) / 100.0;
}

QString BahiKhataExporter::formatMdbDate(const QString& isoDate) {
    if (isoDate.isEmpty()) return "";
    QDate d = QDate::fromString(isoDate.left(10), "yyyy-MM-dd");
    if (!d.isValid()) d = QDate::fromString(isoDate.left(10), Qt::ISODate);
    if (!d.isValid()) return isoDate;
    return d.toString("MM/dd/yyyy");
}

#if USE_LIBMDB
static double toOleDate(const QString& dateStr) {
    if (dateStr.isEmpty()) return 0.0;
    QDate d = QDate::fromString(dateStr.left(10), "yyyy-MM-dd");
    if (!d.isValid()) d = QDate::fromString(dateStr.left(10), Qt::ISODate);
    if (!d.isValid()) return 0.0;
    QDate base(1899, 12, 30);
    return static_cast<double>(base.daysTo(d));
}

/* Native Jet honors the per-column UnicodeCompression property
 * (Ledgers.CurrentBalance and Transactions.DrCr are "no" -> always UCS-2LE).
 * mdb_ascii2unicode ignores it; we must not, for byte-exact rows. */
static bool jetColCompress(MdbColumn* col) {
    if (!col || !col->props || !col->props->hash) return true; // legacy default
    char* v = static_cast<char*>(g_hash_table_lookup(col->props->hash, (gpointer)"UnicodeCompression"));
    if (!v) return true;
    QString s = QString::fromUtf8(v).trimmed();
    return s.compare("yes", Qt::CaseInsensitive) == 0 || s == "1" || s.compare("true", Qt::CaseInsensitive) == 0;
}

static QByteArray toJet4TextRaw(const QString& str, bool allowCompress) {
    if (str.isEmpty()) return QByteArray();
    // Replicate mdb_ascii2unicode compression logic (iconv.c:209):
    // - Convert to UCS-2LE (2 bytes per char)
    // - Only apply Unicode Compression (FF FE + toggles) if UCS-2 length > 4
    //   This matches native Bahi-Khata Data.002: "Jrnl"/"ChPt" (6B compressed) vs "Dr"/"0" (4B/2B uncompressed)
    bool isAscii = true;
    for (int i = 0; i < str.length(); ++i) {
        if (str.at(i).unicode() > 0xFF) { isAscii = false; break; }
    }
    QByteArray ucs2;
    ucs2.reserve(str.length() * 2);
    for (int i = 0; i < str.length(); ++i) {
        ushort u = str.at(i).unicode();
        ucs2.append(static_cast<char>(u & 0xFF));
        ucs2.append(static_cast<char>((u >> 8) & 0xFF));
    }
    // No compression for Jet3, for Jet4 only if >4 bytes and pure ASCII
    if (!allowCompress || !isAscii || ucs2.size() <= 4) {
        return ucs2;
    }
    // Compressed path: FF FE + single bytes (all ASCII, so no mode toggles needed)
    QByteArray res;
    res.reserve(2 + str.length());
    res.append(static_cast<char>(0xFF));
    res.append(static_cast<char>(0xFE));
    for (int i = 0; i < str.length(); ++i) {
        res.append(static_cast<char>(str.at(i).toLatin1()));
    }
    return res;
}

static QByteArray toJet4Text(const QString& str) {
    return toJet4TextRaw(str, true);
}

static QByteArray toJet4TextForCol(const QString& str, MdbColumn* col) {
    return toJet4TextRaw(str, jetColCompress(col));
}

static double round2dbl(double v) { return std::round(v * 100.0) / 100.0; }

/* Shared voucher-type normalization (Jet short codes). Used by both engines. */
static QString normalizeTxType(const QString& raw) {
    QString t = raw.trimmed();
    if (t.compare("Journal", Qt::CaseInsensitive) == 0) return "Jrnl";
    if (t.compare("Payment", Qt::CaseInsensitive) == 0) return "ChPt";
    if (t.compare("Receipt", Qt::CaseInsensitive) == 0) return "ChRt";
    if (t.compare("Sales", Qt::CaseInsensitive) == 0) return "Sale";
    if (t.compare("Purchase", Qt::CaseInsensitive) == 0) return "Purc";
    if (t.compare("Contra", Qt::CaseInsensitive) == 0) return "Jrnl";
    if (t.length() > 5) return t.left(5);
    return t;
}

/* Shared FY filter builder. Empty/"ALL" -> no filter (export all years).
 * Matches Data.002's "FY 2023-24" storage against "2026-2027"/"26-27"/etc. */
static QString buildFyWhere(const QString& fyParam, const QString& col = "t.financial_year") {
    QString p = fyParam.trimmed();
    if (p.isEmpty() || p.compare("ALL", Qt::CaseInsensitive) == 0) return QString();
    QString norm = p;
    norm.remove("FY", Qt::CaseInsensitive);
    norm.remove(' ');
    QRegularExpression reFy("(\\d{2})\\D*(\\d{2,4})");
    QString likePat;
    auto m = reFy.globalMatch(norm);
    if (m.hasNext()) {
        QRegularExpressionMatch lm;
        while (m.hasNext()) lm = m.next();
        QString a = lm.captured(1), b = lm.captured(2);
        if (b.length() == 4) b = b.right(2);
        likePat = a + "-" + b;
    }
    if (likePat.isEmpty()) likePat = norm.left(5);
    QString fyWithPrefix = p.startsWith("FY", Qt::CaseInsensitive) ? p : "FY " + p;
    return QString("WHERE (%1 = '%2' OR %1 = '%3' OR %1 LIKE '%%4%' OR %1 LIKE '%%5%') ")
        .arg(col, p, fyWithPrefix, likePat, norm);
}

/* ---- Cached ledger balance (Ledgers.CurrentBalance) maintenance ----
 * Bahi-Khata reads ledger balances from the stored CurrentBalance TEXT
 * ("0.00" | "N,NNN.NN Dr|Cr", US grouping), NOT computed live. Its voucher
 * save applies each leg incrementally (Dr-positive signed arithmetic).
 * The exporter must do the same or balances/reports stay stale. */
static double parseJetBalance(const QString& txt, QString& sideOut) {
    sideOut.clear();
    QString s = txt.trimmed();
    if (s.isEmpty() || s.compare("None", Qt::CaseInsensitive) == 0) return 0.0;
    // Optional trailing Dr/Cr (native uses " Dr"/" Cr"; tolerate missing/attached)
    QString up = s.toUpper();
    bool isCr = up.endsWith("CR");
    bool isDr = !isCr && up.endsWith("DR");
    if (isCr || isDr) {
        sideOut = isCr ? "Cr" : "Dr";
        s = s.left(s.length() - 2).trimmed();
    }
    s.remove(','); // US grouping
    s.remove(' ');
    bool ok = false;
    double v = s.toDouble(&ok);
    if (!ok) return 0.0;
    if (isCr) v = -std::abs(v);
    else if (isDr) v = std::abs(v);
    return v;
}

static QString formatJetBalance(double signedVal, const QString& zeroSideFallback) {
    double v = round2dbl(signedVal);
    if (std::abs(v) < 0.005) {
        // Preserve a zero-with-side residue ("0.00 Dr") when present natively.
        return zeroSideFallback.isEmpty() ? QString("0.00")
                                          : QString("0.00 %1").arg(zeroSideFallback);
    }
    QString side = (v > 0) ? "Dr" : "Cr";
    // US grouping with 2 decimals, exactly as the app stores ("61,218,935.35 Cr").
    static const QLocale usLoc(QLocale::English, QLocale::UnitedStates);
    QString num = usLoc.toString(std::abs(v), 'f', 2);
    return QString("%1 %2").arg(num, side);
}

/* Surgically replaces one data row: same-size edits overwrite in place;
 * growing/shrinking rows shift only the contiguous block at/below them,
 * preserving offset order, flag bits, gaps and every other byte.
 * (mdb_replace_row rebuilds whole pages; this keeps the structural delta
 * minimal for Jet compatibility.) Returns false on any validation failure. */
static bool surgicalReplaceRow(MdbHandle* mdb, MdbTableDef* tbl, int rowIdx0,
                               const unsigned char* newRow, int newSize) {
    if (!mdb || !tbl || !tbl->entry || !newRow || newSize <= 0 || newSize > 4000)
        return false;
    const int pgSize = mdb->fmt->pg_size;
    const int rco = mdb->fmt->row_count_offset;
    unsigned char* buf = (unsigned char*)mdb->pg_buf;
    int nrows = mdb_get_int16(buf, rco);
    if (rowIdx0 < 0 || rowIdx0 >= nrows) return false;
    std::vector<int> offs(nrows), flags(nrows);
    for (int i = 0; i < nrows; i++) {
        int raw = mdb_get_int16(buf, rco + 2 + i * 2);
        offs[i] = raw & 0x0FFF;
        flags[i] = raw & ~0x0FFF;
        if (offs[i] < rco + 2 + nrows * 2 || offs[i] > pgSize) return false;
    }
    int oldStart = offs[rowIdx0];
    // Old size = next higher offset - own (page end if highest).
    int oldEnd = pgSize;
    for (int o : offs) if (o > oldStart && o < oldEnd) oldEnd = o;
    int oldSize = oldEnd - oldStart;
    // Shared storage (duplicate offsets, e.g. native triplet slots) cannot be
    // resized per-slot: only allow in-place (delta==0) updates there.
    int sharers = 0;
    for (int o : offs) if (o == oldStart) sharers++;
    if (oldSize <= 0 || oldSize > 4000) return false;
    int delta = newSize - oldSize;
    if (sharers > 1 && delta != 0) return false; // shared storage: in-place only
    int freeNow = mdb_get_int16(buf, 2);
    if (delta > freeNow) return false; // not enough room

    if (delta == 0) {
        memcpy(buf + oldStart, newRow, newSize);
    } else if (delta > 0) {
        // Growth: move span [spanLo, oldEnd) DOWN by delta into free space.
        // Target lands at oldStart - delta and ends exactly at oldEnd.
        int spanLo = oldStart;
        for (int o : offs) if (o < spanLo) spanLo = o;
        if (spanLo - delta < rco + 2 + nrows * 2) return false;
        memmove(buf + spanLo - delta, buf + spanLo, oldEnd - spanLo);
        int newStart = oldStart - delta;
        memcpy(buf + newStart, newRow, newSize);
        for (int i = 0; i < nrows; i++) {
            if (offs[i] >= spanLo && offs[i] < oldEnd) {
                int noff = offs[i] - delta;
                if (noff < rco + 2 + nrows * 2 || noff > pgSize) return false;
                mdb_put_int16(buf, rco + 2 + i * 2, (guint32)(noff | flags[i]));
            }
        }
    } else {
        // Shrink: overwrite in place; the freed tail becomes a zeroed interior
        // gap (exactly like post-delete state: offsets and free count stay).
        int k = -delta;
        memcpy(buf + oldStart, newRow, newSize);
        memset(buf + oldStart + newSize, 0, (size_t)k);
    }
    if (delta != 0) {
        // Exact free-space recompute (native semantics).
        int minStart = pgSize;
        for (int i = 0; i < nrows; i++) {
            int o = mdb_get_int16(buf, rco + 2 + i * 2) & 0x0FFF;
            if (o < minStart) minStart = o;
        }
        mdb_put_int16(buf, 2, (guint32)(minStart - (rco + 2 + nrows * 2)));
    }
    return mdb_write_pg(mdb, tbl->cur_phys_pg) != 0;
}

/* Applies one signed delta (Dr-positive) to a ledger's cached CurrentBalance
 * by repacking that ledger's row in place. Mirrors the app's voucher-save. */
static bool applyLedgerBalanceDelta(MdbHandle* mdb, int ledgerCode, double delta) {
    if (!mdb || ledgerCode <= 0 || std::abs(delta) < 0.0005) return true;
    MdbTableDef* tbl = mdb_read_table_by_name(mdb, (char*)"Ledgers", MDB_TABLE);
    if (!tbl) return false;
    mdb_read_columns(tbl);
    // Ledgers layout (Data.002): col 3 = Code1st (INT), col 16 = CurrentBalance (TEXT)
    bool done = false;
    mdb_rewind_table(tbl);
    while (!done && mdb_fetch_row(tbl)) {
        MdbColumn* colCode = (MdbColumn*)g_ptr_array_index(tbl->columns, 3);
        int code = 0;
        if (colCode->cur_value_len == 2)
            code = mdb_get_int16(mdb->pg_buf, colCode->cur_value_start);
        else {
            char* s = mdb_col_to_string(mdb, mdb->pg_buf, colCode->cur_value_start,
                                        colCode->col_type, colCode->cur_value_len);
            if (s) { code = atoi(s); g_free(s); }
        }
        if (code != ledgerCode) continue;

        int rowStart = 0;
        size_t rowSize = 0;
        mdb_find_row(mdb, tbl->cur_row - 1, &rowStart, &rowSize);
        rowStart &= 0x0FFF;
        MdbField fields[128];
        memset(fields, 0, sizeof(fields));
        if (mdb_crack_row(tbl, rowStart, rowSize, fields) < 0) break;

        MdbColumn* colBal = (MdbColumn*)g_ptr_array_index(tbl->columns, 16);
        char* curStr = mdb_col_to_string(mdb, mdb->pg_buf, colBal->cur_value_start,
                                         colBal->col_type, colBal->cur_value_len);
        QString oldText = curStr ? QString::fromUtf8(curStr) : QString();
        if (curStr) g_free(curStr);
        QString side;
        double cur = parseJetBalance(oldText, side);
        double updated = round2dbl(cur + delta);
        QString newText = formatJetBalance(updated, side);
        // CurrentBalance is UnicodeCompression=no -> always UCS-2LE.
        QByteArray newBytes = toJet4TextForCol(newText, colBal);
        // Empty must stay a valid pointer (Jet4Writer null-vs-empty rule).
        static const char kEmpty = '\0';
        fields[16].is_null = 0;
        fields[16].value = newBytes.isEmpty() ? (void*)&kEmpty : (void*)newBytes.data();
        fields[16].siz = newBytes.size();

        unsigned char rowBuf[4096];
        int newSize = mdb_pack_row(tbl, rowBuf, tbl->num_cols, fields);
        if (newSize <= 0 || newSize > 4000) break;
        // Surgical in-place/sliding update (NOT mdb_replace_row: minimal delta).
        if (!surgicalReplaceRow(mdb, tbl, tbl->cur_row - 1, rowBuf, newSize)) {
            std::cout << "[BAL] skip ledger " << ledgerCode << " (no room)" << std::endl;
            break;
        }
        {
            std::cout << "[BAL] ledger " << ledgerCode << " "
                      << oldText.toStdString() << " -> " << newText.toStdString() << std::endl;
            done = true;
        }
        break;
    }
    mdb_free_tabledef(tbl);
    return done;
}

static bool insertJet4Row(MdbHandle* mdb, MdbTableDef* table, MdbField* fields, int num_fields) {
    Q_UNUSED(mdb);
    return mdb_insert_row(table, num_fields, fields) != 0;
}

static void finalizeJet4Table(MdbHandle* mdb, MdbTableDef* table) {
    if (!mdb || !table || !table->entry) return;
    mdb_read_pg(mdb, table->entry->table_pg);
    mdb_put_int32(mdb->pg_buf, mdb->fmt->tab_num_rows_offset, table->num_rows);
    if (table->indices) {
        for (guint i = 0; i < table->indices->len; i++) {
            MdbIndex *pidx = (MdbIndex *)g_ptr_array_index(table->indices, i);
            if (pidx->index_type != 2) {
                int off = mdb->fmt->tab_cols_start_offset + (pidx->index_num * mdb->fmt->tab_ridx_entry_size);
                mdb_put_int32(mdb->pg_buf, off, pidx->num_rows);
            }
        }
    }
    mdb_write_pg(mdb, table->entry->table_pg);
}

struct BahiKhataJournalTx {
    guint16 rowNo = 1;
    guint32 vchNo = 0;
    QString dateStr;
    QString transType = "Jrnl";
    guint16 accountCode = 0;
    QString drCr = "Dr";
    double amount = 0.0;
    QString narration;
    guint16 partyCode = 0;
};

static bool insertBahiKhataJournalTx(MdbHandle* mdb, MdbTableDef* txTbl, const BahiKhataJournalTx& tx) {
    if (!mdb || !txTbl) return false;
    std::vector<MdbField> fields(txTbl->num_cols);
    for (int i = 0; i < txTbl->num_cols; i++) {
        MdbColumn* col = (MdbColumn*)g_ptr_array_index(txTbl->columns, i);
        fields[i].colnum = i;
        fields[i].is_fixed = col->is_fixed;
        fields[i].is_null = 1;
        fields[i].value = nullptr;
        fields[i].siz = 0;
    }

    guint16 rowNo = tx.rowNo;
    fields[0].is_null = 0; fields[0].value = &rowNo; fields[0].siz = 2;

    guint32 vchNo = tx.vchNo;
    fields[1].is_null = 0; fields[1].value = &vchNo; fields[1].siz = 4;

    double oleD = toOleDate(tx.dateStr);
    fields[2].is_null = 0; fields[2].value = &oleD; fields[2].siz = 8;

    MdbColumn* colTT = (MdbColumn*)g_ptr_array_index(txTbl->columns, 3);
    MdbColumn* colDC = (MdbColumn*)g_ptr_array_index(txTbl->columns, 5);
    MdbColumn* colNarr = (MdbColumn*)g_ptr_array_index(txTbl->columns, 14);
    QByteArray ttBytes = toJet4TextForCol(tx.transType.left(5), colTT);
    fields[3].is_null = 0; fields[3].value = ttBytes.data(); fields[3].siz = ttBytes.size();

    guint16 acVal = tx.accountCode;
    fields[4].is_null = 0; fields[4].value = &acVal; fields[4].siz = 2;

    // DrCr is UnicodeCompression=no -> always UCS-2LE ("Dr" = 44 00 72 00).
    QByteArray dcBytes = toJet4TextForCol(tx.drCr, colDC);
    fields[5].is_null = 0; fields[5].value = dcBytes.data(); fields[5].siz = dcBytes.size();

    double amtVal = tx.amount;
    fields[6].is_null = 0; fields[6].value = &amtVal; fields[6].siz = 8;

    // Col 7: InvoiceNo is null for Journal
    fields[7].is_null = 1;

    guint16 ptVal = tx.partyCode;
    fields[8].is_null = 0; fields[8].value = &ptVal; fields[8].siz = 2;

    QByteArray narrBytes = toJet4TextForCol(tx.narration.left(200), colNarr);
    fields[14].is_null = 0; fields[14].value = narrBytes.data(); fields[14].siz = narrBytes.size();

    // Canonical Bahi-Khata default values matching authentic Data.002 Journal (1883) schema
    // Data.002 Journal not-null set: 0,1,2,3,4,5,6,8,14,15,16,17,35,38,39,42,45,46,50,52,53,54,55,57-60,62,63,65,68-70,73-76,78
    guint16 zero16 = 0;
    guint32 zero32 = 0;
    double zeroDbl = 0.0;
    float zeroFlt = 0.0f;
    QByteArray zeroStr = toJet4Text("0");
    static const char kEmpty = '\0'; // 0-length but NOT NULL (pristine mask 7f c1... bit 15 set)

    // Col 15: EntryType = empty (not null, 0-length) for Journal — pristine Jrnl 472 mask
    // has bit 15 SET. NULL would emit single 0x00 in Idx_DateTransVchWise (5th key EntryType)
    // instead of 7F 01 00, breaking Bahi-Khata date-wise seeks and max-date lookup.
    fields[15].is_null = 0; fields[15].value = (void*)&kEmpty; fields[15].siz = 0;

    fields[16].is_null = 0; fields[16].value = &zero16; fields[16].siz = 2; // DueDays
    fields[17].is_null = 0; fields[17].value = &zero16; fields[17].siz = 2; // ItemCode

    // Col 29: BankDate = same as VoucherDate for Journal (matches authentic Data.002)
    fields[29].is_null = 0; fields[29].value = &oleD; fields[29].siz = 8;

    // Col 30: VoucherReconcile = 0 for Journal (matches authentic Data.002)
    fields[30].is_null = 0; fields[30].value = &zero32; fields[30].siz = 4;

    fields[35].is_null = 0; fields[35].value = &zeroDbl; fields[35].siz = 8;// ExpRate
    fields[37].is_null = 0; fields[37].value = (void*)&kEmpty; fields[37].siz = 0; // ImagePath = "" (not null empty, pristine Jrnl 472)
    fields[38].is_null = 0; fields[38].value = (void*)&kEmpty; fields[38].siz = 0; // BrokerName = "" (not null empty)
    fields[39].is_null = 0; fields[39].value = &zeroDbl; fields[39].siz = 8;// EstimatedAmount
    fields[42].is_null = 0; fields[42].value = &zero32; fields[42].siz = 4; // MktCommttSrNo
    fields[45].is_null = 0; fields[45].value = &zero32; fields[45].siz = 4; // URDPurc
    fields[46].is_null = 0; fields[46].value = &zero32; fields[46].siz = 4; // E1PartyCode
    fields[50].is_null = 0; fields[50].value = &zero32; fields[50].siz = 4; // CompositionVch
    fields[52].is_null = 0; fields[52].value = zeroStr.data(); fields[52].siz = zeroStr.size(); // PlaceOfSupply
    fields[53].is_null = 0; fields[53].value = zeroStr.data(); fields[53].siz = zeroStr.size(); // ECommGSTIN
    fields[54].is_null = 0; fields[54].value = &zero32; fields[54].siz = 4; // TransReturn
    fields[55].is_null = 0; fields[55].value = &zero32; fields[55].siz = 4; // GroupTick
    fields[57].is_null = 0; fields[57].value = &zero32; fields[57].siz = 4; // ActualInv
    fields[58].is_null = 0; fields[58].value = &zero32; fields[58].siz = 4; // ITCNotClaim
    fields[59].is_null = 0; fields[59].value = &zero32; fields[59].siz = 4; // ReverseChargePayable
    fields[60].is_null = 0; fields[60].value = &zero32; fields[60].siz = 4; // GSTOnGoodsAmount
    fields[62].is_null = 0; fields[62].value = &zeroFlt; fields[62].siz = 4; // TCSRate
    fields[63].is_null = 0; fields[63].value = &zeroDbl; fields[63].siz = 8; // TCSTaxable
    fields[65].is_null = 0; fields[65].value = &zero32; fields[65].siz = 4; // ChallanVchNo
    // 67 TempInv remains NULL for Journal (as Data.002)
    fields[68].is_null = 0; fields[68].value = &zeroFlt; fields[68].siz = 4; // TDSRate194Q
    fields[69].is_null = 0; fields[69].value = &zeroDbl; fields[69].siz = 8; // Taxable194Q
    fields[70].is_null = 0; fields[70].value = &zero32; fields[70].siz = 4; // TDS194QChallanVchNo
    fields[73].is_null = 0; fields[73].value = &oleD; fields[73].siz = 8; // TaxInputDate = voucher date (pristine Jrnl 472)
    fields[74].is_null = 0; fields[74].value = &zero32; fields[74].siz = 4; // PymtDone
    fields[75].is_null = 0; fields[75].value = &zeroDbl; fields[75].siz = 8;// tmpBookNo
    fields[76].is_null = 0; fields[76].value = &zeroDbl; fields[76].siz = 8;// tmpSlipNo
    fields[78].is_null = 0; fields[78].value = &zeroDbl; fields[78].siz = 8;// RunningBNo

    return mdb_insert_row(txTbl, txTbl->num_cols, fields.data()) != 0;
}
#endif

QString BahiKhataExporter::resolveSeedTemplate(const QString& explicitPath) {
    /* Test hook: JET4_SEED_PATH forces an explicit seed file. */
    if (const char* envSeed = getenv("JET4_SEED_PATH")) {
        QString s = QString::fromLocal8Bit(envSeed);
        if (QFile::exists(s)) return s;
    }
    if (!explicitPath.isEmpty()) {
        if (QFile::exists(explicitPath)) return explicitPath;
        // Try resolve relative explicitPath from build/app dirs (for tests)
        QString appDir = QCoreApplication::applicationDirPath();
        QStringList roots = { QDir::currentPath(), appDir };
        for (auto root : roots) {
            QDir d(root);
            for (int up=0; up<4; ++up) {
                QString p = d.filePath(explicitPath);
                if (QFile::exists(p)) return p;
                if (!d.cdUp()) break;
            }
        }
        QString abs = "/Users/karan/MahadevAc/" + explicitPath;
        if (QFile::exists(abs)) return abs;
    }

    QStringList candidates = {
        "Bahi-Khata-Data/Data.002",
        "Bahi-Khata-Data/Data.001",
        "Bahi-Khata-Data/Data.004",
        "Bahi-Khata-Data/Data.005",
        "Bahi-Khata-Data/Data.018"
    };

    for (const auto& c : candidates) {
        if (QFile::exists(c)) {
            return c;
        }
    }

    QString appDir = QCoreApplication::applicationDirPath();
    for (const auto& c : candidates) {
        QString p = QDir(appDir).filePath(c);
        if (QFile::exists(p)) return p;
    }
    // Search upward from appDir and current dir (for tests running in build/)
    QStringList searchRoots = { QDir::currentPath(), appDir };
    for (auto root : searchRoots) {
        QDir d(root);
        for (int up=0; up<4; ++up) {
            for (const auto& c : candidates) {
                QString p = d.filePath(c);
                if (QFile::exists(p)) return p;
            }
            if (!d.cdUp()) break;
        }
    }
    // Last resort: absolute source location (matches Data.002)
    for (const auto& c : candidates) {
        QString abs = "/Users/karan/MahadevAc/" + c;
        if (QFile::exists(abs)) return abs;
    }

    return "";
}

QString BahiKhataExporter::chooseTargetMdbFile(const QString& startDir) {
    QString dir = startDir.isEmpty() ? QDir::homePath() : startDir;
    return QFileDialog::getSaveFileName(
        nullptr,
        "Select Target Bahi-Khata File (Data.00x)",
        dir,
        "Bahi-Khata JetDB (*.001 *.002 *.004 *.005 *.018 *.mdb);;All Files (*.*)"
    );
}

bool BahiKhataExporter::exportDatabase(const QString& targetMdbPath, const QString& financialYear) {
    ExportOptions opts;
    opts.targetMdbPath = targetMdbPath;
    opts.financialYear = financialYear;
    opts.createBackup = true;
    opts.exportTransactions = true;

    ExportSummary res = exportToBahiKhata(opts);
    return res.success;
}

BahiKhataExporter::ExportSummary BahiKhataExporter::exportToBahiKhata(const ExportOptions& options) {
    m_isExporting = true;
    emit exportingChanged();
    updateProgress(5, "Initializing Bahi-Khata JetDB export pipeline...");

    ExportSummary summary;
    summary.targetFilePath = options.targetMdbPath;

    if (options.targetMdbPath.isEmpty()) {
        summary.success = false;
        summary.errorMessage = "Target file path is empty.";
        updateProgress(100, summary.errorMessage);
        emit exportFinished(false, summary.errorMessage);
        m_isExporting = false;
        emit exportingChanged();
        return summary;
    }

    // 1. Prepare target file (Clone seed template if target does not exist, or backup existing)
    QString targetPath = QDir::toNativeSeparators(options.targetMdbPath);
    {
        QFileInfo tInfo(targetPath);
        QDir parent = tInfo.dir();
        if (!parent.exists()) parent.mkpath(".");
    }

    if (!QFile::exists(targetPath)) {
        QString seedPath = resolveSeedTemplate(options.templateMdbPath);
        if (seedPath.isEmpty()) {
            summary.success = false;
            summary.errorMessage = "No seed Bahi-Khata template found to construct JetDB schema.";
            updateProgress(100, summary.errorMessage);
            emit exportFinished(false, summary.errorMessage);
            m_isExporting = false;
            emit exportingChanged();
            return summary;
        }

        updateProgress(10, QString("Creating target database from seed template: %1").arg(seedPath));
        if (!QFile::copy(seedPath, targetPath)) {
            summary.success = false;
            summary.errorMessage = QString("Failed to create target file at %1").arg(targetPath);
            updateProgress(100, summary.errorMessage);
            emit exportFinished(false, summary.errorMessage);
            m_isExporting = false;
            emit exportingChanged();
            return summary;
        }
    } else {
        if (options.createBackup) {
            updateProgress(10, "Creating safety backup of target file...");
            QString backupPath = targetPath + QString(".bak_%1").arg(QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss"));
            QFile::copy(targetPath, backupPath);
        }
    }

    // 2. Select execution engine based on platform and driver availability
    bool hasOdbc = QSqlDatabase::isDriverAvailable("QODBC");
#ifdef Q_OS_WIN
    if (hasOdbc) {
        summary = exportViaOdbc(options, targetPath);
    } else {
        summary = exportViaLibMdb(options, targetPath);
    }
#else
    summary = exportViaLibMdb(options, targetPath);
#endif

    m_isExporting = false;
    emit exportingChanged();
    return summary;
}

BahiKhataExporter::ExportSummary BahiKhataExporter::exportViaOdbc(const ExportOptions& options, const QString& targetPath) {
    ExportSummary summary;
    summary.targetFilePath = targetPath;
    updateProgress(15, "Connecting to MS Access Jet 4.0 engine via ODBC...");

    QString connName = QString("BahiKhataExport_%1").arg(QDateTime::currentMSecsSinceEpoch());
    {
        QSqlDatabase mdb = QSqlDatabase::addDatabase("QODBC", connName);
        QString connStr = QString("DRIVER={Microsoft Access Driver (*.mdb, *.accdb)};DBQ=%1;").arg(targetPath);
        mdb.setDatabaseName(connStr);

        if (!mdb.open()) {
            summary.success = false;
            summary.errorMessage = QString("ODBC Connection Error: %1").arg(mdb.lastError().text());
            updateProgress(100, summary.errorMessage);
            emit exportFinished(false, summary.errorMessage);
            return summary;
        }

        auto& appDb = DatabaseManager::instance();

        // 1. Export CompanyInfo
        updateProgress(20, "Exporting Company Master...");
        QVariantList compRows = appDb.executeQuery("SELECT company_name, business_type, address, phone, mobile, pan_no, gstin FROM company_info LIMIT 1;");
        if (!compRows.isEmpty()) {
            QVariantMap c = compRows.first().toMap();
            QSqlQuery q(mdb);
            q.prepare("UPDATE CompanyInfo SET CompanyName = ?, Business = ?, Address = ?, Phone_O = ?, Mobile1 = ?, PAN_No = ?, GSTIN = ?;");
            q.addBindValue(c.value("company_name").toString());
            q.addBindValue(c.value("business_type", "Rice Milling & Trading").toString());
            q.addBindValue(c.value("address").toString());
            q.addBindValue(c.value("phone").toString());
            q.addBindValue(c.value("mobile").toString());
            q.addBindValue(c.value("pan_no").toString());
            q.addBindValue(c.value("gstin").toString());
            q.exec();
        }

        // 2. Export Groups
        updateProgress(30, "Exporting 4-code Account Groups...");
        QVariantList grpRows = appDb.executeQuery("SELECT name, code1st, code2nd, code3rd, code4th, extract_in_balance_sheet FROM account_groups ORDER BY code1st ASC;");
        {
            QSqlQuery qDel(mdb);
            qDel.exec("DELETE FROM Groups;");
            
            QSqlQuery q(mdb);
            q.prepare("INSERT INTO Groups (GroupName, Code1st, Code2nd, Code3rd, Code4th, ExtractInBalanceSheet) VALUES (?, ?, ?, ?, ?, ?);");
            for (const auto& grpVar : grpRows) {
                QVariantMap g = grpVar.toMap();
                int c1 = g.value("code1st").toInt();
                int c2 = g.value("code2nd", c1).toInt();
                int c3 = g.value("code3rd", c1).toInt();
                int c4 = g.value("code4th", c1).toInt();
                double extBs = g.value("extract_in_balance_sheet", 0.0).toDouble();

                q.addBindValue(g.value("name").toString());
                q.addBindValue(c1);
                q.addBindValue(c2);
                q.addBindValue(c3);
                q.addBindValue(c4);
                q.addBindValue(extBs);
                if (q.exec()) summary.groupsExported++;
            }
        }

        // 3. Export Ledgers
        updateProgress(45, "Exporting Ledgers and Account Masters...");
        QVariantList partyRows = appDb.executeQuery(
            "SELECT id, legacy_id, name, group_code, opening_balance, balance_type, "
            "address, city, party_station, phone, mobile, contact_person, pan, gstin, "
            "salary_per_month, bank_account, bank_name, ifsc_code "
            "FROM parties ORDER BY COALESCE(legacy_id, id) ASC;"
        );
        {
            QSqlQuery qDel(mdb);
            qDel.exec("DELETE FROM Ledgers;");

            QSqlQuery q(mdb);
            q.prepare(
                "INSERT INTO Ledgers (Code1st, LedgerName, GroupCode, OpeningBalance, DrCr, Address, Station, "
                "Phone_O, Mobile1, ConcernedPerson, IncomeTaxNo, GSTIN, SalaryPerMonth, BankAccount, BankName, IFSCCode) "
                "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);"
            );

            for (const auto& pVar : partyRows) {
                QVariantMap p = pVar.toMap();
                int legId = p.value("legacy_id").toInt();
                if (legId <= 0) legId = p.value("id").toInt();

                q.addBindValue(legId);
                q.addBindValue(p.value("name").toString().left(50));
                q.addBindValue(p.value("group_code").toInt());
                q.addBindValue(round2(p.value("opening_balance").toDouble()));
                q.addBindValue(p.value("balance_type", "Dr").toString().left(2));
                q.addBindValue(p.value("address").toString().left(250));
                q.addBindValue(p.value("party_station", p.value("city")).toString().left(50));
                q.addBindValue(p.value("phone").toString().left(50));
                q.addBindValue(p.value("mobile").toString().left(50));
                q.addBindValue(p.value("contact_person").toString().left(50));
                q.addBindValue(p.value("pan").toString().left(50));
                q.addBindValue(p.value("gstin").toString().left(50));
                q.addBindValue(round2(p.value("salary_per_month").toDouble()));
                q.addBindValue(p.value("bank_account").toString().left(50));
                q.addBindValue(p.value("bank_name").toString().left(50));
                q.addBindValue(p.value("ifsc_code").toString().left(50));

                if (q.exec()) summary.ledgersExported++;
            }
        }

        // 4. Export Transactions
        if (options.exportTransactions && !options.exportMastersOnly) {
            updateProgress(65, "Exporting Double-Entry Vouchers & Transactions...");
            
            QString txSql =
                "SELECT t.voucher_no, t.voucher_date, t.trans_type, t.dr_cr, t.amount, t.narration, "
                "COALESCE(p.legacy_id, p.id) as ac_code, "
                "COALESCE(op.legacy_id, op.id, 0) as party_code, "
                "t.financial_year "
                "FROM transactions t "
                "LEFT JOIN parties p ON t.party_id = p.id OR t.party_name = p.name "
                "LEFT JOIN parties op ON t.opposing_account = op.name ";

            txSql += buildFyWhere(options.financialYear);
            txSql += "ORDER BY t.voucher_date ASC, t.id ASC;";

            QVariantList txRows = appDb.executeQuery(txSql);

            // Snapshot native cached balances BEFORE wiping, so per-leg deltas
            // below use identical incremental semantics as the libmdb engine.
            QMap<int, QString> nativeBalances;
            {
                QSqlQuery qBal(mdb);
                if (qBal.exec("SELECT Code1st, CurrentBalance FROM Ledgers;")) {
                    while (qBal.next())
                        nativeBalances[qBal.value(0).toInt()] = qBal.value(1).toString();
                }
            }

            QSqlQuery qDel(mdb);
            qDel.exec("DELETE FROM Transactions;");

            QSqlQuery q(mdb);
            q.prepare(
                "INSERT INTO Transactions (RowNo, VoucherNumber, VoucherDate, TransType, AccountCode, DrCr, Amount, InvoiceNo, PartyCode, Narration) "
                "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?);"
            );

            // RowNo is sequential per voucher (1..n), matching native Jet rows.
            QMap<QString, int> voucherRowCounter;
            QMap<int, double> balDeltas;
            int rowIdx = 1;
            for (const auto& tVar : txRows) {
                QVariantMap t = tVar.toMap();
                int vchNum = t.value("voucher_no").toInt();
                if (vchNum <= 0) {
                    static QRegularExpression reDigits(R"(\d+)");
                    QRegularExpressionMatch mm = reDigits.match(t.value("voucher_no").toString());
                    vchNum = mm.hasMatch() ? mm.captured(0).toInt() : rowIdx;
                }
                QString isoDate = t.value("voucher_date").toString().left(10);
                QString vKey = QString("%1|%2|%3").arg(vchNum).arg(isoDate).arg(t.value("financial_year").toString());
                int nextRow = voucherRowCounter.value(vKey, 0) + 1;
                voucherRowCounter[vKey] = nextRow;

                double amt = round2(t.value("amount").toDouble());
                QString drCr = t.value("dr_cr", "Dr").toString().left(2);
                QString transType = normalizeTxType(t.value("trans_type", "Jrnl").toString());
                int acCode = t.value("ac_code").toInt();
                if (drCr == "Dr") summary.totalDebitAmount += amt;
                else summary.totalCreditAmount += amt;

                q.addBindValue(nextRow);
                q.addBindValue(vchNum);
                q.addBindValue(formatMdbDate(t.value("voucher_date").toString()));
                q.addBindValue(transType.left(5));
                q.addBindValue(acCode);
                q.addBindValue(drCr);
                q.addBindValue(amt);
                q.addBindValue(t.value("voucher_no").toString().left(50));
                q.addBindValue(transType.compare("Jrnl", Qt::CaseInsensitive) == 0
                               ? 0 : t.value("party_code").toInt());
                q.addBindValue(t.value("narration").toString().left(200));

                if (q.exec()) {
                    summary.transactionsExported++;
                    rowIdx++;
                    if (acCode > 0 && std::abs(amt) >= 0.0005) {
                        double signedAmt = (drCr == "Dr") ? amt : -amt;
                        balDeltas[acCode] = round2dbl(balDeltas.value(acCode, 0.0) + signedAmt);
                    }
                }
            }

            // Mirror the app's voucher-save: post each leg to the ledger's
            // cached CurrentBalance, starting from the snapshotted native value.
            updateProgress(80, "Updating cached ledger balances...");
            QSqlQuery qBalUpd(mdb);
            qBalUpd.prepare("UPDATE Ledgers SET CurrentBalance = ? WHERE Code1st = ?;");
            for (auto it = balDeltas.constBegin(); it != balDeltas.constEnd(); ++it) {
                if (std::abs(it.value()) < 0.0005) continue;
                QString side;
                double cur = parseJetBalance(nativeBalances.value(it.key(), "0.00"), side);
                QString newText = formatJetBalance(round2dbl(cur + it.value()), side);
                qBalUpd.addBindValue(newText);
                qBalUpd.addBindValue(it.key());
                qBalUpd.exec();
            }
        }

        // 5. Export Stock Items
        if (options.exportMillingAndStock) {
            updateProgress(85, "Exporting Stock Items & Commodity Masters...");
            QVariantList stockRows = appDb.executeQuery("SELECT code, name, category_name, opening_bags, opening_qty, opening_value, hsn_code FROM stock_items;");
            QSqlQuery qDel(mdb);
            qDel.exec("DELETE FROM StockItems;");

            QSqlQuery q(mdb);
            q.prepare("INSERT INTO StockItems (Code1st, GroupCode, ItemName, OpeningBags, OpeningQty, OpeningValue, HSNCode) VALUES (?, ?, ?, ?, ?, ?, ?);");
            for (const auto& sVar : stockRows) {
                QVariantMap s = sVar.toMap();
                q.addBindValue(s.value("code").toInt());
                q.addBindValue(1); // Default group
                q.addBindValue(s.value("name").toString().left(50));
                q.addBindValue(s.value("opening_bags").toDouble());
                q.addBindValue(s.value("opening_qty").toDouble());
                q.addBindValue(s.value("opening_value").toDouble());
                q.addBindValue(s.value("hsn_code").toString().left(255));
                if (q.exec()) summary.stockItemsExported++;
            }
        }

        mdb.close();
    }
    QSqlDatabase::removeDatabase(connName);

    summary.success = true;
    updateProgress(100, QString("Export completed successfully! Ledgers: %1, Vouchers: %2")
                            .arg(summary.ledgersExported)
                            .arg(summary.transactionsExported));
    emit exportFinished(true, QString("Exported %1 ledgers and %2 transactions to Bahi-Khata JetDB at %3")
                                  .arg(summary.ledgersExported)
                                  .arg(summary.transactionsExported)
                                  .arg(targetPath));
    return summary;
}

BahiKhataExporter::ExportSummary BahiKhataExporter::exportViaLibMdb(const ExportOptions& options, const QString& targetPath) {
    ExportSummary summary;
    summary.targetFilePath = targetPath;
    updateProgress(15, "Opening Jet 4.0 database via embedded binary engine...");

#ifdef USE_LIBMDB
    QByteArray pathBytes = QFile::encodeName(targetPath);
    MdbHandle* mdb = mdb_open(pathBytes.constData(), MDB_WRITABLE);
    if (!mdb) mdb = mdb_open(targetPath.toUtf8().constData(), MDB_WRITABLE);

    if (!mdb) {
        summary.success = false;
        summary.errorMessage = QString("Failed to open %1 in writable mode via libmdb.").arg(targetPath);
        updateProgress(100, summary.errorMessage);
        emit exportFinished(false, summary.errorMessage);
        return summary;
    }

    auto& appDb = DatabaseManager::instance();

    // 0. Sync CompanyInfo
    updateProgress(20, "Updating CompanyInfo in Jet 4 database...");
    MdbTableDef* compTbl = mdb_read_table_by_name(mdb, (char*)"CompanyInfo", MDB_TABLE);
    if (compTbl) {
        mdb_read_columns(compTbl);
        QVariantList compList = appDb.executeQuery("SELECT * FROM company_info LIMIT 1;");
        if (!compList.isEmpty()) {
            QVariantMap c = compList.first().toMap();
            std::vector<MdbField> fieldsVec(compTbl->num_cols);
            MdbField* fields = fieldsVec.data();
            for (int i = 0; i < compTbl->num_cols; i++) {
                MdbColumn* col = (MdbColumn*)g_ptr_array_index(compTbl->columns, i);
                fields[i].colnum = i;
                fields[i].is_fixed = col->is_fixed;
                fields[i].is_null = 1;
            }

            // --- Complete Format as Data.002: 4 FY rows, all share same Company metadata ---
            QVariantList fyRows = appDb.executeQuery("SELECT year_name, start_date, end_date FROM financial_years ORDER BY start_date ASC;");
            if (fyRows.isEmpty()) {
                // Fallback to single FY from company_info (legacy)
                QVariantMap single;
                single["year_name"] = "FY 2026-27";
                single["start_date"] = c.value("acc_year_from", "2026-04-01");
                single["end_date"] = c.value("acc_year_to", "2027-03-31");
                fyRows.append(single);
            }
            // BooksBeginingFrom is earliest FY start (Data.002: 04/01/23 for all rows)
            QString booksIso = c.value("books_from", "").toString();
            if (booksIso.isEmpty()) booksIso = fyRows.first().toMap().value("start_date").toString();
            // Ensure earliest FY determines BooksBeginingFrom if not set
            {
                QString earliest = fyRows.first().toMap().value("start_date").toString();
                for (auto &v : fyRows) {
                    QString s = v.toMap().value("start_date").toString();
                    if (s < earliest) earliest = s;
                }
                if (c.value("books_from").toString().isEmpty()) booksIso = earliest;
                if (booksIso.isEmpty()) booksIso = "2023-04-01";
            }
            double dBooks = toOleDate(booksIso);

            // Cache existing AccYearFrom values to avoid duplicates
            std::set<long long> existingFromDays;
            {
                MdbColumn* colFrom = nullptr;
                for (int i = 0; i < compTbl->num_cols; i++) {
                    MdbColumn* col = (MdbColumn*)g_ptr_array_index(compTbl->columns, i);
                    if (QString::fromLatin1(col->name) == "AccYearFrom") { colFrom = col; break; }
                }
                if (colFrom && colFrom->col_type == MDB_DATETIME) {
                    mdb_rewind_table(compTbl);
                    while (mdb_fetch_row(compTbl)) {
                        if (colFrom->cur_value_len == sizeof(double)) {
                            double rowFrom = mdb_get_double(mdb->pg_buf, colFrom->cur_value_start);
                            existingFromDays.insert((long long)llround(rowFrom));
                        }
                    }
                }
            }

            for (auto &fyVar : fyRows) {
                QVariantMap fy = fyVar.toMap();
                QString fyStart = fy.value("start_date").toString();
                QString fyEnd = fy.value("end_date").toString();
                if (fyStart.isEmpty() || fyEnd.isEmpty()) continue;
                double dFrom = toOleDate(fyStart);
                double dTo = toOleDate(fyEnd);
                long long key = llround(dFrom);
                if (existingFromDays.find(key) != existingFromDays.end()) continue;

                // Build fresh field set per FY (QByteArray must stay alive until insert)
                auto compCol = [&](const char* n) -> MdbColumn* {
                    for (int i = 0; i < compTbl->num_cols; i++) {
                        MdbColumn* cc = (MdbColumn*)g_ptr_array_index(compTbl->columns, i);
                        if (strcmp(cc->name, n) == 0) return cc;
                    }
                    return nullptr;
                };
                QByteArray nameBytes = toJet4Text(c.value("company_name", "M/S MAHADEV RICE INDUSTRY").toString().left(50));
                QByteArray busBytes = toJet4Text(c.value("business_type", "RICE MANUFACTURER (FSSAI LIC NO. : 10822019000152)").toString().left(100));
                QByteArray addBytes = toJet4Text(c.value("address", "BABA SAWAN SINGH THERI ROAD,VPO SIKANDERPUR, SIRSA, HARYANA -125055").toString().left(250));
                QByteArray phoneBytes = toJet4Text(c.value("phone", "01666-297533").toString().left(50));
                QByteArray mobBytes = toJet4Text(c.value("mobile", "9416595091").toString().left(50));
                QByteArray panBytes = toJet4Text(c.value("pan_no", "ABKFM5928Q").toString().left(50));
                QByteArray gstinBytes = toJet4Text(c.value("gstin", "06ABKFM5928Q1ZG").toString().left(50));
                // MyStation/MySTATE/Bank2 are UnicodeCompression=no -> UCS-2LE.
                QByteArray stnBytes = toJet4TextForCol(c.value("city", "SIRSA").toString().left(50), compCol("MyStation"));
                QByteArray stateBytes = toJet4TextForCol(c.value("state", "HARYANA").toString().left(50), compCol("MySTATE"));
                QByteArray b2Bytes = toJet4TextForCol(QString("A/c No:- %1").arg(c.value("bank_account", "128001400717").toString()).left(50), compCol("Bank2"));
                QByteArray b3Bytes = toJet4Text(QString("IFSC:- %1").arg(c.value("ifsc_code", "CNRB0002058").toString()).left(50));
                QByteArray firmTypeBytes = toJet4Text("Partnership Firm");
                guint32 syncVal = 1;
                guint16 zero16 = 0;

                // Reset fields
                for (int i = 0; i < compTbl->num_cols; i++) {
                    MdbColumn* col = (MdbColumn*)g_ptr_array_index(compTbl->columns, i);
                    fields[i].colnum = i; fields[i].is_fixed = col->is_fixed; fields[i].is_null = 1; fields[i].value = nullptr; fields[i].siz = 0;
                }
                for (int i = 0; i < compTbl->num_cols; i++) {
                    MdbColumn* col = (MdbColumn*)g_ptr_array_index(compTbl->columns, i);
                    QString cname = QString::fromLatin1(col->name);
                    if (cname == "CompanyName") { fields[i].is_null = 0; fields[i].value = nameBytes.data(); fields[i].siz = nameBytes.size(); }
                    else if (cname == "Business") { fields[i].is_null = 0; fields[i].value = busBytes.data(); fields[i].siz = busBytes.size(); }
                    else if (cname == "Address") { fields[i].is_null = 0; fields[i].value = addBytes.data(); fields[i].siz = addBytes.size(); }
                    else if (cname == "Phone_O") { fields[i].is_null = 0; fields[i].value = phoneBytes.data(); fields[i].siz = phoneBytes.size(); }
                    else if (cname == "Mobile1") { fields[i].is_null = 0; fields[i].value = mobBytes.data(); fields[i].siz = mobBytes.size(); }
                    else if (cname == "PAN_No") { fields[i].is_null = 0; fields[i].value = panBytes.data(); fields[i].siz = panBytes.size(); }
                    else if (cname == "GSTIN") { fields[i].is_null = 0; fields[i].value = gstinBytes.data(); fields[i].siz = gstinBytes.size(); }
                    else if (cname == "MyStation") { fields[i].is_null = 0; fields[i].value = stnBytes.data(); fields[i].siz = stnBytes.size(); }
                    else if (cname == "MySTATE") { fields[i].is_null = 0; fields[i].value = stateBytes.data(); fields[i].siz = stateBytes.size(); }
                    else if (cname == "Bank2") { fields[i].is_null = 0; fields[i].value = b2Bytes.data(); fields[i].siz = b2Bytes.size(); }
                    else if (cname == "Bank3") { fields[i].is_null = 0; fields[i].value = b3Bytes.data(); fields[i].siz = b3Bytes.size(); }
                    else if (cname == "FirmType") { fields[i].is_null = 0; fields[i].value = firmTypeBytes.data(); fields[i].siz = firmTypeBytes.size(); }
                    else if (cname == "AccYearFrom") { fields[i].is_null = 0; fields[i].value = &dFrom; fields[i].siz = 8; }
                    else if (cname == "AccYearTo") { fields[i].is_null = 0; fields[i].value = &dTo; fields[i].siz = 8; }
                    else if (cname == "BooksBeginingFrom") { fields[i].is_null = 0; fields[i].value = &dBooks; fields[i].siz = 8; }
                    else if (cname == "CompositionScheme") { fields[i].is_null = 0; fields[i].value = &zero16; fields[i].siz = 2; }
                    else if (cname == "SyncEnabled") { fields[i].is_null = 0; fields[i].value = &syncVal; fields[i].siz = 4; }
                }
                insertJet4Row(mdb, compTbl, fields, compTbl->num_cols);
                existingFromDays.insert(key);
            }
        }
        finalizeJet4Table(mdb, compTbl);
        mdb_free_tabledef(compTbl);
    }

    // 1. Sync Groups
    updateProgress(25, "Syncing Account Groups into Jet 4 Groups table...");
    MdbTableDef* grpTbl = mdb_read_table_by_name(mdb, (char*)"Groups", MDB_TABLE);
    if (grpTbl) {
        mdb_read_columns(grpTbl);
        mdb_read_indices(grpTbl);

        std::set<int> existingGroupCodes;
        mdb_rewind_table(grpTbl);
        MdbColumn* colC1 = (MdbColumn*)g_ptr_array_index(grpTbl->columns, 1);
        while (mdb_fetch_row(grpTbl)) {
            char* cStr = mdb_col_to_string(mdb, mdb->pg_buf, colC1->cur_value_start, colC1->col_type, colC1->cur_value_len);
            if (cStr) {
                existingGroupCodes.insert(atoi(cStr));
                g_free(cStr);
            }
        }

        QVariantList grpRows = appDb.executeQuery("SELECT name, code1st, code2nd, code3rd, code4th, extract_in_balance_sheet FROM account_groups ORDER BY code1st ASC;");
        std::cout << "[EXPORT] Account Groups in SQLite: " << grpRows.size() << ", existing in JetDB: " << existingGroupCodes.size() << std::endl;
        for (const auto& gVar : grpRows) {
            QVariantMap g = gVar.toMap();
            int c1 = g.value("code1st").toInt();
            if (existingGroupCodes.find(c1) == existingGroupCodes.end()) {
                // Insert new group
                std::vector<MdbField> fieldsVec(grpTbl->num_cols);
                MdbField* fields = fieldsVec.data();
                for (int i = 0; i < grpTbl->num_cols; i++) {
                    MdbColumn* col = (MdbColumn*)g_ptr_array_index(grpTbl->columns, i);
                    fields[i].colnum = i;
                    fields[i].is_fixed = col->is_fixed;
                    fields[i].is_null = 1;
                }

                QByteArray gNameBytes = toJet4Text(g.value("name").toString().left(50));
                fields[0].is_null = 0; fields[0].value = gNameBytes.data(); fields[0].siz = gNameBytes.size();

                guint16 c1Val = c1;
                fields[1].is_null = 0; fields[1].value = &c1Val; fields[1].siz = 2;

                guint16 c2Val = g.value("code2nd", c1).toInt();
                fields[2].is_null = 0; fields[2].value = &c2Val; fields[2].siz = 2;

                guint16 c3Val = g.value("code3rd", c1).toInt();
                fields[3].is_null = 0; fields[3].value = &c3Val; fields[3].siz = 2;

                guint16 c4Val = g.value("code4th", c1).toInt();
                fields[4].is_null = 0; fields[4].value = &c4Val; fields[4].siz = 2;

                double extBs = g.value("extract_in_balance_sheet", 0.0).toDouble();
                fields[5].is_null = 0; fields[5].value = &extBs; fields[5].siz = 8;

                if (insertJet4Row(mdb, grpTbl, fields, grpTbl->num_cols)) {
                    summary.groupsExported++;
                }
            } else {
                summary.groupsExported++;
            }
        }
        finalizeJet4Table(mdb, grpTbl);
        mdb_free_tabledef(grpTbl);
    }

    // 2. Sync Ledgers
    updateProgress(45, "Syncing Ledgers and Account Masters into Jet 4 Ledgers table...");
    MdbTableDef* ledgersTbl = mdb_read_table_by_name(mdb, (char*)"Ledgers", MDB_TABLE);
    if (ledgersTbl) {
        mdb_read_columns(ledgersTbl);
        mdb_read_indices(ledgersTbl);

        std::map<int, int> ledgerCodeToPhysPg;
        mdb_rewind_table(ledgersTbl);
        MdbColumn* colCode = (MdbColumn*)g_ptr_array_index(ledgersTbl->columns, 3);
        MdbColumn* colOpBal = (MdbColumn*)g_ptr_array_index(ledgersTbl->columns, 6);
        MdbColumn* colSalary = (MdbColumn*)g_ptr_array_index(ledgersTbl->columns, 19);

        // In-place updates for existing ledgers
        QVariantList partyRows = appDb.executeQuery(
            "SELECT id, legacy_id, name, group_code, opening_balance, balance_type, "
            "address, city, party_station, phone, mobile, contact_person, pan, gstin, "
            "salary_per_month, bank_account, bank_name, ifsc_code "
            "FROM parties ORDER BY COALESCE(legacy_id, id) ASC;"
        );

        std::map<int, QVariantMap> partyMap;
        for (const auto& pVar : partyRows) {
            QVariantMap p = pVar.toMap();
            int legId = p.value("legacy_id").toInt();
            if (legId <= 0) legId = p.value("id").toInt();
            partyMap[legId] = p;
        }
        std::cout << "[EXPORT] Parties fetched from SQLite: " << partyMap.size() << std::endl;

        while (mdb_fetch_row(ledgersTbl)) {
            char* cStr = mdb_col_to_string(mdb, mdb->pg_buf, colCode->cur_value_start, colCode->col_type, colCode->cur_value_len);
            if (cStr) {
                int c = atoi(cStr);
                g_free(cStr);
                ledgerCodeToPhysPg[c] = ledgersTbl->cur_phys_pg;

                auto it = partyMap.find(c);
                if (it != partyMap.end()) {
                    bool modified = false;
                    double opBal = round2(it->second.value("opening_balance").toDouble());
                    if (colOpBal->cur_value_start > 0 &&
                        colOpBal->cur_value_len == (int)sizeof(double)) {
                        memcpy(mdb->pg_buf + colOpBal->cur_value_start, &opBal, sizeof(double));
                        modified = true;
                    }
                    double sal = round2(it->second.value("salary_per_month").toDouble());
                    if (colSalary->cur_value_start > 0 &&
                        colSalary->cur_value_len == (int)sizeof(double)) {
                        memcpy(mdb->pg_buf + colSalary->cur_value_start, &sal, sizeof(double));
                        modified = true;
                    }
                    if (modified) {
                        mdb_write_pg(mdb, ledgersTbl->cur_phys_pg);
                    }
                    summary.ledgersExported++;
                }
            }
        }

        // Insert new ledgers not existing in Jet 4
        for (const auto& pair : partyMap) {
            int code = pair.first;
            if (ledgerCodeToPhysPg.find(code) == ledgerCodeToPhysPg.end()) {
                const QVariantMap& p = pair.second;
                std::vector<MdbField> fieldsVec(ledgersTbl->num_cols);
                MdbField* fields = fieldsVec.data();
                for (int i = 0; i < ledgersTbl->num_cols; i++) {
                    MdbColumn* col = (MdbColumn*)g_ptr_array_index(ledgersTbl->columns, i);
                    fields[i].colnum = i;
                    fields[i].is_fixed = col->is_fixed;
                    fields[i].is_null = 1;
                }
                // Column-aware text encoding (respects UnicodeCompression=no).
                auto T4C = [&](const QString& s, int colIdx) {
                    MdbColumn* c = (MdbColumn*)g_ptr_array_index(ledgersTbl->columns, colIdx);
                    return toJet4TextForCol(s, c);
                };

                QByteArray prefixBytes = toJet4Text("None");
                fields[0].is_null = 0; fields[0].value = prefixBytes.data(); fields[0].siz = prefixBytes.size();

                QByteArray nameBytes = toJet4Text(p.value("name").toString().left(50));
                fields[1].is_null = 0; fields[1].value = nameBytes.data(); fields[1].siz = nameBytes.size();

                guint16 codeVal = code;
                fields[3].is_null = 0; fields[3].value = &codeVal; fields[3].siz = 2;

                guint16 grpVal = p.value("group_code", 1).toInt();
                fields[4].is_null = 0; fields[4].value = &grpVal; fields[4].siz = 2;

                QByteArray invChoice = toJet4Text("No");
                fields[5].is_null = 0; fields[5].value = invChoice.data(); fields[5].siz = invChoice.size();

                double opBal = round2(p.value("opening_balance").toDouble());
                fields[6].is_null = 0; fields[6].value = &opBal; fields[6].siz = 8;

                QByteArray drCrBytes = toJet4Text(p.value("balance_type", "Dr").toString().left(2));
                fields[7].is_null = 0; fields[7].value = drCrBytes.data(); fields[7].siz = drCrBytes.size();

                QByteArray mailName = toJet4Text("01/04/2023");
                fields[8].is_null = 0; fields[8].value = mailName.data(); fields[8].siz = mailName.size();

                QByteArray addBytes = toJet4Text(p.value("address").toString().left(250));
                fields[9].is_null = 0; fields[9].value = addBytes.data(); fields[9].siz = addBytes.size();

                QByteArray panBytes = toJet4Text(p.value("pan").toString().left(50));
                fields[10].is_null = 0; fields[10].value = panBytes.data(); fields[10].siz = panBytes.size();

                QByteArray stnBytes = toJet4Text(p.value("party_station", p.value("city")).toString().left(50));
                fields[13].is_null = 0; fields[13].value = stnBytes.data(); fields[13].siz = stnBytes.size();

                QByteArray noneBytes = toJet4Text("None");
                fields[14].is_null = 0; fields[14].value = noneBytes.data(); fields[14].siz = noneBytes.size(); // Route
                fields[15].is_null = 0; fields[15].value = noneBytes.data(); fields[15].siz = noneBytes.size(); // Distt

                QByteArray curBalBytes = T4C("0.00", 16);
                fields[16].is_null = 0; fields[16].value = curBalBytes.data(); fields[16].siz = curBalBytes.size(); // CurrentBalance

                double zeroDbl = 0.0;
                guint32 zero32 = 0;
                fields[17].is_null = 0; fields[17].value = &zeroDbl; fields[17].siz = 8; // Percentage
                fields[18].is_null = 0; fields[18].value = &zeroDbl; fields[18].siz = 8; // InterestRate

                double sal = round2(p.value("salary_per_month").toDouble());
                fields[19].is_null = 0; fields[19].value = &sal; fields[19].siz = 8;

                QByteArray acctBytes = T4C(p.value("bank_account").toString().left(50), 20);
                fields[20].is_null = 0; fields[20].value = acctBytes.data(); fields[20].siz = acctBytes.size();

                QByteArray stateBytes = T4C("STATE", 21);
                fields[21].is_null = 0; fields[21].value = stateBytes.data(); fields[21].siz = stateBytes.size();

                fields[22].is_null = 0; fields[22].value = &zero32; fields[22].siz = 4; // ShowDateTotals
                fields[23].is_null = 0; fields[23].value = &zero32; fields[23].siz = 4; // AutoSplitUpdate

                QByteArray pTypeBytes = T4C("Proprietorship Firm", 25);
                fields[25].is_null = 0; fields[25].value = pTypeBytes.data(); fields[25].siz = pTypeBytes.size();

                QByteArray cpBytes = T4C(p.value("contact_person").toString().left(50), 26);
                fields[26].is_null = 0; fields[26].value = cpBytes.data(); fields[26].siz = cpBytes.size();

                QByteArray pStateBytes = T4C("Haryana", 27);
                fields[27].is_null = 0; fields[27].value = pStateBytes.data(); fields[27].siz = pStateBytes.size();

                QByteArray vatDlrBytes = T4C("NON-VAT DEALER", 29);
                fields[29].is_null = 0; fields[29].value = vatDlrBytes.data(); fields[29].siz = vatDlrBytes.size();

                fields[30].is_null = 0; fields[30].value = stnBytes.data(); fields[30].siz = stnBytes.size(); // PartyStation
                fields[31].is_null = 0; fields[31].value = &zeroDbl; fields[31].siz = 8; // CreditLimit
                fields[33].is_null = 0; fields[33].value = &zero32; fields[33].siz = 4; // CalculateInJointTrading

                QByteArray naBytes = toJet4Text("N/A");
                fields[34].is_null = 0; fields[34].value = naBytes.data(); fields[34].siz = naBytes.size(); // SpecialPartyType
                fields[36].is_null = 0; fields[36].value = naBytes.data(); fields[36].siz = naBytes.size(); // CommnCalcOn

                fields[38].is_null = 0; fields[38].value = &zero32; fields[38].siz = 4; // StockNotCalculateInLedger

                QByteArray gstinBytes = toJet4Text(p.value("gstin").toString().left(50));
                fields[39].is_null = 0; fields[39].value = gstinBytes.data(); fields[39].siz = gstinBytes.size();

                QByteArray gstTypeBytes = toJet4Text("Normal Dealer");
                fields[41].is_null = 0; fields[41].value = gstTypeBytes.data(); fields[41].siz = gstTypeBytes.size();

                fields[43].is_null = 0; fields[43].value = &zero32; fields[43].siz = 4; // SyncEnabled

                QByteArray ifscBytes = toJet4Text(p.value("ifsc_code").toString().left(50));
                fields[44].is_null = 0; fields[44].value = ifscBytes.data(); fields[44].siz = ifscBytes.size();

                QByteArray bankBytes = toJet4Text(p.value("bank_name").toString().left(50));
                fields[45].is_null = 0; fields[45].value = bankBytes.data(); fields[45].siz = bankBytes.size();

                fields[46].is_null = 0; fields[46].value = &zero32; fields[46].siz = 4; // ApplyTCSForParty
                fields[49].is_null = 0; fields[49].value = &zero32; fields[49].siz = 4; // TCSNotApplyInSale

                bool insOk = insertJet4Row(mdb, ledgersTbl, fields, ledgersTbl->num_cols);
                static int ledgDbg = 0;
                if (++ledgDbg <= 5 || !insOk) {
                    std::cout << "[LEDGER INSERT] code=" << code << " name=" << p.value("name").toString().toStdString() << " ok=" << insOk << std::endl;
                }
                if (insOk) {
                    summary.ledgersExported++;
                }
            }
        }
        std::cout << "[EXPORT] Total Ledgers Exported: " << summary.ledgersExported << std::endl;

        finalizeJet4Table(mdb, ledgersTbl);
        mdb_free_tabledef(ledgersTbl);
    }

    // 3. Sync Transactions
    if (options.exportTransactions && !options.exportMastersOnly) {
        updateProgress(65, "Syncing Double-Entry Vouchers into Jet 4 Transactions table...");
        MdbTableDef* txTbl = mdb_read_table_by_name(mdb, (char*)"Transactions", MDB_TABLE);
        if (txTbl) {
            mdb_read_columns(txTbl);
            mdb_read_indices(txTbl);

            std::set<std::tuple<int, QString, int, int, QString>> existingTxSignatures;
            mdb_rewind_table(txTbl);

            MdbColumn* colVchNo = (MdbColumn*)g_ptr_array_index(txTbl->columns, 1);
            MdbColumn* colVchDate = (MdbColumn*)g_ptr_array_index(txTbl->columns, 2);
            MdbColumn* colAcCode = (MdbColumn*)g_ptr_array_index(txTbl->columns, 4);
            MdbColumn* colDrCr = (MdbColumn*)g_ptr_array_index(txTbl->columns, 5);
            MdbColumn* colAmt = (MdbColumn*)g_ptr_array_index(txTbl->columns, 6);

            while (mdb_fetch_row(txTbl)) {
                int vNo = (colVchNo && colVchNo->cur_value_len > 0) ? mdb_get_int32(mdb->pg_buf, colVchNo->cur_value_start) : 0;
                int ac = (colAcCode && colAcCode->cur_value_len > 0) ? mdb_get_int16(mdb->pg_buf, colAcCode->cur_value_start) : 0;
                double amt = (colAmt && colAmt->cur_value_len > 0) ? mdb_get_double(mdb->pg_buf, colAmt->cur_value_start) : 0.0;
                int amtCents = static_cast<int>(std::round(amt * 100.0));

                QString dVal;
                if (colVchDate && colVchDate->cur_value_len > 0) {
                    double oleDays = mdb_get_double(mdb->pg_buf, colVchDate->cur_value_start);
                    QDate qd = QDate(1899, 12, 30).addDays(static_cast<qint64>(oleDays));
                    dVal = qd.toString("yyyy-MM-dd");
                }

                QString dc = "Dr";
                if (colDrCr && colDrCr->cur_value_len > 0) {
                    const unsigned char* p = (const unsigned char*)mdb->pg_buf + colDrCr->cur_value_start;
                    if (colDrCr->cur_value_len >= 3 && p[0] == 0xFF && p[1] == 0xFE) {
                        if (p[2] == 'C' || p[2] == 'c') dc = "Cr";
                    } else if (p[0] == 'C' || p[0] == 'c' || (colDrCr->cur_value_len >= 2 && p[1] == 'C')) {
                        dc = "Cr";
                    }
                }

                existingTxSignatures.insert(std::make_tuple(vNo, dVal, ac, amtCents, dc));
            }

            // --- Match Data.002: export ALL FYs when param empty, else robust FY match ---
            QString txSql = 
                "SELECT t.id, t.voucher_no, t.voucher_date, t.trans_type, t.dr_cr, t.amount, t.narration, "
                "COALESCE(t.account_code, p.legacy_id, p.id) as ac_code, "
                "COALESCE(op.legacy_id, op.id, 0) as party_code, "
                "t.financial_year "
                "FROM transactions t "
                "LEFT JOIN parties p ON t.party_id = p.id OR t.party_name = p.name "
                "LEFT JOIN parties op ON t.opposing_account = op.name ";

            // Shared robust FY filter (same normalization as the ODBC engine).
            txSql += buildFyWhere(options.financialYear);
            txSql += "ORDER BY t.voucher_date ASC, t.id ASC;";
            QVariantList txRows = appDb.executeQuery(txSql);
            std::cout << "[EXPORT] Transactions fetched from SQLite: " << txRows.size() << ", existing in JetDB: " << existingTxSignatures.size() << std::endl;
            // RowNo must be sequential per voucher (Bahi-Khata expects 1..n per VoucherNumber+Date)
            QMap<QString, int> voucherRowCounter;
            int rowIdx = 1;
            // Signed per-ledger deltas (Dr-positive) for Ledgers.CurrentBalance.
            QMap<int, double> balDeltas;

            for (const auto& tVar : txRows) {
                QVariantMap t = tVar.toMap();
                int rawVchNo = t.value("voucher_no").toInt();
                if (rawVchNo <= 0) {
                    QString vStr = t.value("voucher_no").toString();
                    static QRegularExpression reDigits(R"(\d+)");
                    QRegularExpressionMatch m = reDigits.match(vStr);
                    if (m.hasMatch()) rawVchNo = m.captured(0).toInt();
                    else rawVchNo = rowIdx;
                }

                QString isoDate = t.value("voucher_date").toString().left(10);
                int acCode = t.value("ac_code").toInt();
                double amt = round2(t.value("amount").toDouble());
                int amtCents = static_cast<int>(std::round(amt * 100.0));
                QString drCr = t.value("dr_cr", "Dr").toString().left(2);

                auto sig = std::make_tuple(rawVchNo, isoDate, acCode, amtCents, drCr);
                if (existingTxSignatures.find(sig) == existingTxSignatures.end()) {
                    BahiKhataJournalTx tx;
                    QString vKey = QString("%1|%2|%3").arg(rawVchNo).arg(isoDate).arg(t.value("financial_year").toString());
                    int nextRow = voucherRowCounter.value(vKey, 0) + 1;
                    voucherRowCounter[vKey] = nextRow;
                    tx.rowNo = nextRow;
                    tx.vchNo = rawVchNo;
                    tx.dateStr = isoDate;
                    tx.transType = normalizeTxType(t.value("trans_type", "Jrnl").toString());
                    tx.accountCode = acCode;
                    tx.drCr = drCr;
                    tx.amount = amt;
                    tx.narration = t.value("narration").toString();
                    // PartyCode: 0 for Journal, else party_code (as Data.002)
                    tx.partyCode = (tx.transType.compare("Jrnl", Qt::CaseInsensitive)==0) ? 0 : t.value("party_code").toInt();

                    bool txInsOk = insertBahiKhataJournalTx(mdb, txTbl, tx);
                    static int txDbg = 0;
                    if (++txDbg <= 5 || !txInsOk) {
                        std::cout << "[TX INSERT] vch=" << rawVchNo << " drCr=" << drCr.toStdString() << " amt=" << amt << " ok=" << txInsOk << std::endl;
                    }
                    if (txInsOk) {
                        summary.transactionsExported++;
                        if (drCr == "Dr") summary.totalDebitAmount += amt;
                        else summary.totalCreditAmount += amt;
                        existingTxSignatures.insert(sig);
                        // Mirror the app's voucher-save: post each leg to its
                        // ledger's cached CurrentBalance (Dr-positive signed).
                        if (acCode > 0 && std::abs(amt) >= 0.0005) {
                            double signedAmt = (drCr == "Dr") ? amt : -amt;
                            balDeltas[acCode] = round2dbl(
                                balDeltas.value(acCode, 0.0) + signedAmt);
                        }
                    }
                } else {
                    summary.transactionsExported++;
                    if (drCr == "Dr") summary.totalDebitAmount += amt;
                    else summary.totalCreditAmount += amt;
                }
                rowIdx++;
            }
            std::cout << "[EXPORT] Total Transactions Exported: " << summary.transactionsExported << std::endl;

            // Apply cached ledger balances for newly inserted vouchers so the
            // app reflects them without a manual open+save per voucher.
            updateProgress(80, "Updating cached ledger balances...");
            for (auto it = balDeltas.constBegin(); it != balDeltas.constEnd(); ++it) {
                if (std::abs(it.value()) >= 0.0005)
                    applyLedgerBalanceDelta(mdb, it.key(), it.value());
            }

            finalizeJet4Table(mdb, txTbl);
            mdb_free_tabledef(txTbl);

            // Sync TempLastEnteredVoucher with latest transaction date and FY range
            MdbTableDef* lastVchTbl = mdb_read_table_by_name(mdb, (char*)"TempLastEnteredVoucher", MDB_TABLE);
            if (lastVchTbl) {
                mdb_read_columns(lastVchTbl);
                mdb_rewind_table(lastVchTbl);
                if (mdb_fetch_row(lastVchTbl)) {
                    guint32 data_pg = lastVchTbl->cur_phys_pg;
                    MdbColumn* colVT = (MdbColumn*)g_ptr_array_index(lastVchTbl->columns, 0);
                    MdbColumn* colVD = (MdbColumn*)g_ptr_array_index(lastVchTbl->columns, 1);
                    MdbColumn* colFrom = (MdbColumn*)g_ptr_array_index(lastVchTbl->columns, 2);
                    MdbColumn* colTo = (MdbColumn*)g_ptr_array_index(lastVchTbl->columns, 3);

                    int v_start = colVD->cur_value_start;
                    int v_len = colVD->cur_value_len;
                    int from_start = colFrom->cur_value_start;
                    int from_len = colFrom->cur_value_len;
                    int to_start = colTo->cur_value_start;
                    int to_len = colTo->cur_value_len;
                    int vt_start = colVT->cur_value_start;
                    int vt_len = colVT->cur_value_len;

                    // Query latest transaction date & FY from SQLite
                    QVariantList maxTx = appDb.executeQuery(
                        "SELECT t.voucher_date, t.trans_type, f.start_date, f.end_date "
                        "FROM transactions t "
                        "LEFT JOIN financial_years f ON t.financial_year = f.year_name "
                        "ORDER BY t.voucher_date DESC, t.id DESC LIMIT 1;"
                    );

                    if (!maxTx.isEmpty()) {
                        QVariantMap m = maxTx.first().toMap();
                        QString maxDate = m.value("voucher_date").toString();
                        QString tt = normalizeTxType(m.value("trans_type", "Jrnl").toString());
                        QString fStart = m.value("start_date", "2026-04-01").toString();
                        QString fEnd = m.value("end_date", "2027-03-31").toString();

                        double oleMaxDate = toOleDate(maxDate);
                        double oleFrom = toOleDate(fStart);
                        double oleTo = toOleDate(fEnd);

                        mdb_read_pg(mdb, data_pg);
                        if (v_start > 0 && v_len == (int)sizeof(double) && oleMaxDate > 0.0) {
                            memcpy(mdb->pg_buf + v_start, &oleMaxDate, sizeof(double));
                        }
                        if (from_start > 0 && from_len == (int)sizeof(double) && oleFrom > 0.0) {
                            memcpy(mdb->pg_buf + from_start, &oleFrom, sizeof(double));
                        }
                        if (to_start > 0 && to_len == (int)sizeof(double) && oleTo > 0.0) {
                            memcpy(mdb->pg_buf + to_start, &oleTo, sizeof(double));
                        }
                        if (vt_start > 0 && vt_len == 6) {
                            QByteArray ttBytes = toJet4Text(tt.left(4));
                            if (ttBytes.size() == 6) {
                                memcpy(mdb->pg_buf + vt_start, ttBytes.constData(), 6);
                            }
                        }
                        mdb_write_pg(mdb, data_pg);
                        std::cout << "[EXPORT] Updated TempLastEnteredVoucher: date=" << maxDate.toStdString() << " type=" << tt.toStdString() << std::endl;
                    }
                }
                mdb_free_tabledef(lastVchTbl);
            }
        }
    }

    // 4. Sync Stock Items
    if (options.exportMillingAndStock) {
        updateProgress(90, "Syncing Commodity & Stock Items...");
        MdbTableDef* stkTbl = mdb_read_table_by_name(mdb, (char*)"StockItems", MDB_TABLE);
        if (stkTbl) {
            mdb_read_columns(stkTbl);
            mdb_read_indices(stkTbl);

            std::set<int> existingStockCodes;
            mdb_rewind_table(stkTbl);
            MdbColumn* colCode = (MdbColumn*)g_ptr_array_index(stkTbl->columns, 1);
            while (mdb_fetch_row(stkTbl)) {
                char* cStr = mdb_col_to_string(mdb, mdb->pg_buf, colCode->cur_value_start, colCode->col_type, colCode->cur_value_len);
                if (cStr) {
                    existingStockCodes.insert(atoi(cStr));
                    g_free(cStr);
                }
            }

            QVariantList stockRows = appDb.executeQuery("SELECT code, name, category_name, opening_bags, opening_qty, opening_value, hsn_code FROM stock_items;");
            for (const auto& sVar : stockRows) {
                QVariantMap s = sVar.toMap();
                int c = s.value("code").toInt();
                if (existingStockCodes.find(c) == existingStockCodes.end()) {
                    std::vector<MdbField> fieldsVec(stkTbl->num_cols);
                    MdbField* fields = fieldsVec.data();
                    for (int i = 0; i < stkTbl->num_cols; i++) {
                        MdbColumn* col = (MdbColumn*)g_ptr_array_index(stkTbl->columns, i);
                        fields[i].colnum = i;
                        fields[i].is_fixed = col->is_fixed;
                        fields[i].is_null = 1;
                    }

                    QByteArray nameBytes = toJet4Text(s.value("name").toString().left(50));
                    fields[0].is_null = 0; fields[0].value = nameBytes.data(); fields[0].siz = nameBytes.size();

                    guint16 cVal = c;
                    fields[1].is_null = 0; fields[1].value = &cVal; fields[1].siz = 2;

                    guint16 grpVal = 1;
                    fields[2].is_null = 0; fields[2].value = &grpVal; fields[2].siz = 2;

                    double opBags = s.value("opening_bags").toDouble();
                    fields[18].is_null = 0; fields[18].value = &opBags; fields[18].siz = 8;

                    double opQty = s.value("opening_qty").toDouble();
                    fields[19].is_null = 0; fields[19].value = &opQty; fields[19].siz = 8;

                    double opVal = s.value("opening_value").toDouble();
                    fields[20].is_null = 0; fields[20].value = &opVal; fields[20].siz = 8;

                    if (insertJet4Row(mdb, stkTbl, fields, stkTbl->num_cols)) {
                        summary.stockItemsExported++;
                    }
                } else {
                    summary.stockItemsExported++;
                }
            }

            finalizeJet4Table(mdb, stkTbl);
            mdb_free_tabledef(stkTbl);
        }
    }

    mdb_close(mdb);

    summary.success = true;
    updateProgress(100, QString("Export completed! Ledgers: %1, Vouchers: %2")
                            .arg(summary.ledgersExported)
                            .arg(summary.transactionsExported));
    emit exportFinished(true, QString("Successfully exported %1 ledgers and %2 transactions to Bahi-Khata JetDB at %3")
                                  .arg(summary.ledgersExported)
                                  .arg(summary.transactionsExported)
                                  .arg(targetPath));
    return summary;
#else
    summary.success = false;
    summary.errorMessage = "libmdb is not enabled in this build.";
    updateProgress(100, summary.errorMessage);
    emit exportFinished(false, summary.errorMessage);
    return summary;
#endif
}

} // namespace MahadevERP
