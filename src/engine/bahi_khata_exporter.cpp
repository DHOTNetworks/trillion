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
 * We determine this safely by column name to avoid uninitialized props pointers. */
static bool jetColCompress(MdbColumn* col) {
    if (!col) return true;
    const char* n = col->name;
    if (strcmp(n, "CurrentBalance") == 0 || strcmp(n, "DrCr") == 0 ||
        strcmp(n, "MyStation") == 0 || strcmp(n, "MySTATE") == 0 || strcmp(n, "Bank2") == 0) {
        return false;
    }
    return true;
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

static void finalizeJet4Table(MdbHandle* mdb, MdbTableDef* table);

/* Applies signed deltas (Dr-positive) to cached CurrentBalance of all
 * modified ledgers in a single pass over the Ledgers table. Mirrors the app's voucher-save. */
static bool applyLedgerBalanceDeltas(MdbHandle* mdb, const QMap<int, double>& balDeltas) {
    if (!mdb || balDeltas.isEmpty()) return true;
    MdbTableDef* tbl = mdb_read_table_by_name(mdb, (char*)"Ledgers", MDB_TABLE);
    if (!tbl) return false;
    mdb_read_columns(tbl);
    mdb_rewind_table(tbl);
    int updatedCount = 0;
    while (mdb_fetch_row(tbl)) {
        MdbColumn* colCode = (MdbColumn*)g_ptr_array_index(tbl->columns, 3);
        int code = 0;
        if (colCode->cur_value_len == 2)
            code = mdb_get_int16(mdb->pg_buf, colCode->cur_value_start);
        else {
            char* s = mdb_col_to_string(mdb, mdb->pg_buf, colCode->cur_value_start,
                                        colCode->col_type, colCode->cur_value_len);
            if (s) { code = atoi(s); g_free(s); }
        }
        auto it = balDeltas.constFind(code);
        if (it == balDeltas.constEnd() || std::abs(it.value()) < 0.0005) continue;
        double delta = it.value();

        int rowStart = 0;
        size_t rowSize = 0;
        mdb_find_row(mdb, tbl->cur_row - 1, &rowStart, &rowSize);
        rowStart &= 0x0FFF;
        MdbField fields[128];
        memset(fields, 0, sizeof(fields));
        if (mdb_crack_row(tbl, rowStart, rowSize, fields) < 0) continue;

        MdbColumn* colBal = (MdbColumn*)g_ptr_array_index(tbl->columns, 16);
        char* curStr = mdb_col_to_string(mdb, mdb->pg_buf, colBal->cur_value_start,
                                         colBal->col_type, colBal->cur_value_len);
        QString oldText = curStr ? QString::fromUtf8(curStr) : QString();
        if (curStr) g_free(curStr);
        QString side;
        double cur = parseJetBalance(oldText, side);
        double updated = round2dbl(cur + delta);
        QString newText = formatJetBalance(updated, side);
        QByteArray newBytes = toJet4TextForCol(newText, colBal);
        static const char kEmpty = '\0';
        fields[16].is_null = 0;
        fields[16].value = newBytes.isEmpty() ? (void*)&kEmpty : (void*)newBytes.data();
        fields[16].siz = newBytes.size();

        unsigned char rowBuf[4096];
        int newSize = mdb_pack_row(tbl, rowBuf, tbl->num_cols, fields);
        if (newSize <= 0 || newSize > 4000) continue;
        if (surgicalReplaceRow(mdb, tbl, tbl->cur_row - 1, rowBuf, newSize)) {
            updatedCount++;
        }
    }
    finalizeJet4Table(mdb, tbl);
    mdb_free_tabledef(tbl);
    std::cout << "[BAL] Applied balance deltas to " << updatedCount << " ledgers." << std::endl;
    return true;
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

/* ================= Sale / Purchase voucher support =================
 * Native shapes (Data.002 exemplars: Sale-394, Purc-449/Nutech):
 * - Sale: party-Dr "Ledger" leg + detail-Cr legs ("Goods Amount", taxes...).
 * - Purc: party-Cr "Ledger" leg + detail-Dr legs (charges matched to invoice).
 * Two row shapes per type (party leg vs detail leg) differ in ~15 fields.
 * StockTransactions item lines mirror sales/purchase_invoice_items.
 * Fallback label pool + loud logging cover unmapped charge legs. */

struct JetInvoiceItem {
    int jetCode = 0;          // Jet StockItems code (from stock_items.code)
    QString name;
    QString grade;
    int bags = 0;
    double packing = 0.0;
    double weight = 0.0;
    double rate = 0.0;
    double amount = 0.0;      // taxable/total line amount
    double taxable = 0.0;
    double gstPct = 0.0;
};

struct JetInvoiceCtx {
    bool valid = false;
    bool isSale = false;
    QString invoiceNo;
    QString dateStr;          // yyyy-MM-dd
    QString saleStatus;       // Self Sale / Self Purchase
    QString taxStatus;        // GST / IGST (mapped)
    QString marketFeeStatus;  // Paid
    QString marketType;       // Market Type (With Stock)
    QString payMode;          // Credit
    QString broker;           // broker/transport text or ""
    int invSeq = 0;           // trailing integer of invoice_no
    double tcsRate = 0.0;
    int dueDays = 0;
    int headerItemCode = 0;
    std::vector<JetInvoiceItem> items;
    // charge multiset: cents -> label (consumed once each, exact match)
    std::multimap<long long, QString> charges;
};

static long long toCents(double v) { return (long long)std::llround(v * 100.0); }

struct JetInvoiceCache {
    std::unordered_map<int, int> stockItemCodes; // sqlite item id -> code
    std::unordered_map<std::string, QVariantMap> salesInvoices;
    std::unordered_map<std::string, QVariantMap> purcInvoices;
    std::unordered_map<int, std::vector<QVariantMap>> salesItems;
    std::unordered_map<int, std::vector<QVariantMap>> purcItems;
};

static JetInvoiceCache buildInvoiceCache(DatabaseManager& appDb) {
    JetInvoiceCache cache;
    QVariantList stkV = appDb.executeQuery("SELECT id, code FROM stock_items;");
    for (const auto& v : stkV) {
        QVariantMap m = v.toMap();
        cache.stockItemCodes[m.value("id").toInt()] = m.value("code").toInt();
    }
    QVariantList sInvV = appDb.executeQuery("SELECT * FROM sales_invoices;");
    for (const auto& v : sInvV) {
        QVariantMap m = v.toMap();
        cache.salesInvoices[m.value("invoice_no").toString().trimmed().toStdString()] = m;
    }
    QVariantList pInvV = appDb.executeQuery("SELECT * FROM purchase_invoices;");
    for (const auto& v : pInvV) {
        QVariantMap m = v.toMap();
        cache.purcInvoices[m.value("invoice_no").toString().trimmed().toStdString()] = m;
    }
    QVariantList sItmV = appDb.executeQuery(
        "SELECT item_id, item_name, grade, bag_count, packing, weight_qtl,"
        " rate_per_qtl, taxable_amount, total_amount, gst_pct, invoice_id FROM sales_invoice_items ORDER BY id ASC;");
    for (const auto& v : sItmV) {
        QVariantMap m = v.toMap();
        cache.salesItems[m.value("invoice_id").toInt()].push_back(m);
    }
    QVariantList pItmV = appDb.executeQuery(
        "SELECT item_id, item_name, grade, bag_count, packing, weight_qtl,"
        " rate_per_qtl, taxable_amount, total_amount, gst_pct, invoice_id FROM purchase_invoice_items ORDER BY id ASC;");
    for (const auto& v : pItmV) {
        QVariantMap m = v.toMap();
        cache.purcItems[m.value("invoice_id").toInt()].push_back(m);
    }
    return cache;
}

static int jetItemCodeCached(const JetInvoiceCache& cache, int sqliteItemId) {
    auto it = cache.stockItemCodes.find(sqliteItemId);
    return (it != cache.stockItemCodes.end()) ? it->second : 0;
}

static int parseInvSeq(const QString& invNo) {
    static QRegularExpression re(R"((\d+)(?!.*\d))");
    QRegularExpressionMatch m = re.match(invNo);
    return m.hasMatch() ? m.captured(1).toInt() : 0;
}

static QString mapTaxStatus(const QString& raw) {
    QString t = raw.trimmed();
    if (t.contains("IGST", Qt::CaseInsensitive)) return "IGST";
    return "GST"; // native default (covers "GST", "GST / Exempt", empty)
}

static JetInvoiceCtx fetchInvoiceCtxCached(const JetInvoiceCache& cache, const QString& transType,
                                           const QString& invoiceNo) {
    JetInvoiceCtx ctx;
    ctx.isSale = (transType.compare("Sale", Qt::CaseInsensitive) == 0);
    std::string invKey = invoiceNo.trimmed().toStdString();
    if (invKey.empty()) return ctx;

    const auto& invMap = ctx.isSale ? cache.salesInvoices : cache.purcInvoices;
    auto it = invMap.find(invKey);
    if (it == invMap.end()) return ctx;
    const QVariantMap& h = it->second;

    ctx.valid = true;
    ctx.invoiceNo = invoiceNo;
    QString dcol = "invoice_date";
    ctx.dateStr = h.value(dcol).toString().left(10);
    ctx.saleStatus = h.value("sale_status", ctx.isSale ? "Self Sale" : "Self Purchase").toString();
    if (ctx.saleStatus.isEmpty()) ctx.saleStatus = ctx.isSale ? "Self Sale" : "Self Purchase";
    ctx.taxStatus = mapTaxStatus(h.value("tax_status", "GST").toString());
    ctx.marketFeeStatus = h.value("market_fee_status", "Paid").toString();
    if (ctx.marketFeeStatus.isEmpty()) ctx.marketFeeStatus = "Paid";
    ctx.marketType = h.value("market_type", "Market Type (With Stock)").toString();
    if (ctx.marketType.isEmpty()) ctx.marketType = "Market Type (With Stock)";
    ctx.payMode = h.value("payment_mode", "Credit").toString();
    if (ctx.payMode.isEmpty()) ctx.payMode = "Credit";
    ctx.broker = h.value("broker_name").toString().trimmed();
    if (ctx.broker.isEmpty()) ctx.broker = h.value("transport", h.value("transport_name")).toString().trimmed();
    ctx.invSeq = parseInvSeq(invoiceNo);
    ctx.tcsRate = h.value("tcs_rate", 0.0).toDouble();
    ctx.dueDays = h.value("due_days", 0).toInt();
    ctx.headerItemCode = jetItemCodeCached(cache, h.value("item_id").toInt());

    auto addCharge = [&](const QString& label, double amt) {
        if (std::abs(amt) >= 0.005) ctx.charges.insert({toCents(amt), label});
    };
    addCharge("SGST", h.value("sgst_amount").toDouble());
    addCharge("CGST", h.value("cgst_amount").toDouble());
    addCharge("IGST", h.value("igst_amount").toDouble());
    addCharge("Dami", h.value("dami").toDouble());
    addCharge("MFees", h.value("m_fee").toDouble());
    addCharge("CessHRDF", h.value("hrdf").toDouble());
    addCharge("Loading", h.value("labour").toDouble());
    addCharge("Sutli", h.value("sutli").toDouble());
    addCharge("Welfare", h.value("welfare").toDouble());
    addCharge("Dharmada", h.value("dhrmd").toDouble());
    addCharge("Auction", h.value("auction").toDouble());
    addCharge("OtherExp", h.value("other_exp").toDouble());
    addCharge("Freight", h.value("freight_charges").toDouble());

    int invId = h.value("id").toInt();
    const auto& itemsMap = ctx.isSale ? cache.salesItems : cache.purcItems;
    auto itemIt = itemsMap.find(invId);
    if (itemIt != itemsMap.end()) {
        for (const auto& m : itemIt->second) {
            JetInvoiceItem item;
            item.jetCode = jetItemCodeCached(cache, m.value("item_id").toInt());
            item.name = m.value("item_name").toString();
            item.grade = m.value("grade").toString();
            item.bags = m.value("bag_count").toInt();
            item.packing = m.value("packing").toString().toFloat();
            if (item.packing == 0.0f) item.packing = (float)m.value("packing").toDouble();
            item.weight = m.value("weight_qtl").toDouble();
            item.rate = m.value("rate_per_qtl").toDouble();
            item.amount = m.value("total_amount").toDouble();
            if (std::abs(item.amount) < 0.005) item.amount = m.value("taxable_amount").toDouble();
            item.taxable = m.value("taxable_amount").toDouble();
            item.gstPct = m.value("gst_pct").toDouble();
            ctx.items.push_back(item);
        }
    }
    if (ctx.headerItemCode == 0 && !ctx.items.empty())
        ctx.headerItemCode = ctx.items.front().jetCode;
    return ctx;
}

/* System ledger accounts with fixed EntryType labels (stable across firms). */
static QString systemEntryLabel(int accountCode, const QString& partyName) {
    switch (accountCode) {
        case 943: return "SGST";
        case 944: return "CGST";
        case 945: return "IGST";
        case 44: return "Round Off";
        case 2176: return "TDS194Q";
        case 1729: return "TCS";
        case 61: return "DamiTDS";
        case 57: return "MFee";
        case 80: return "OtherExp";
        default: break;
    }
    QString pl = partyName.trimmed().toUpper();
    if (pl.contains("T.D.S.") || pl.contains("TDS")) return "TDS194Q";
    if (pl.contains("T C S") || pl == "TCS" || pl.contains("TCS ")) return "TCS";
    if (pl.contains("SGST")) return "SGST";
    if (pl.contains("CGST")) return "CGST";
    if (pl.contains("IGST")) return "IGST";
    if (pl.contains("ROUND")) return "Round Off";
    return QString();
}

/* Ordered EntryType resolution for a non-Ledger detail leg.
 * fallbackIdx is per-voucher state (caller-owned) for deterministic output. */
static QString resolveDetailEntryType(JetInvoiceCtx& ctx, double amount,
                                      int accountCode, const QString& partyName,
                                      bool goodsAssigned, int& fallbackIdx) {
    QString sys = systemEntryLabel(accountCode, partyName);
    if (!sys.isEmpty()) return sys;
    long long cents = toCents(amount);
    // Exact charge match (consumed once).
    auto range = ctx.charges.equal_range(cents);
    for (auto it = range.first; it != range.second; ++it) {
        QString label = it->second;
        ctx.charges.erase(it);
        return label;
    }
    if (!goodsAssigned) return "Goods Amount";
    // Canonical-order fallback for unmapped charge legs (logged by caller).
    static const char* kFallback[] = {"Tulai", "Commission", "Freight", "Labour",
        "MktFee", "Cess", "CessTax", "Bardana", "Sutli", "Welfare", "Dharmada",
        "Gaushala", "OtherExp", "Auction"};
    QString pick(kFallback[fallbackIdx % 14]);
    fallbackIdx++;
    return pick;
}

/* Insert one Sale/Purchase/Cash voucher leg with an explicit per-type field
 * profile (native nullmask sets from Data.002 exemplars). isFirstLeg marks
 * the party ("Ledger") leg; detail legs carry EntryType/ItemCode shapes. */
struct SalePurcLeg {
    guint16 rowNo = 1;
    guint32 vchNo = 0;
    QString dateStr;
    QString transType;      // Sale / Purc (canonical short)
    guint16 accountCode = 0;
    QString drCr = "Dr";
    double amount = 0.0;
    QString narration;
    QString invoiceNo;
    QString entryType;      // resolved label ("" + emptyNotNull for "")
    bool entryEmpty = false;
    guint16 itemCode = 0;
    bool isFirstLeg = false;
    // Voucher-level (same for all legs of the voucher):
    QString jform;          // Self Sale / Self Purchase
    QString iformTax;       // GST / IGST
    QString mfeeStatus;     // Paid (Purc) / "" (Sale -> NULL)
    QString marketType;     // Market Type (...)
    QString cashCredit;     // Credit
    QString broker;         // Sale broker text (Purc: "")
    double balAmount = 0.0; // party-leg voucher total (detail legs: ignored)
    bool hasBalAmount = false;
    int invSeq = 0;         // Sale invoice sequence (Purc: 0)
    double tcsRate = 0.0;
    int dueDays = 0;
};

static bool insertSalePurcLeg(MdbHandle* mdb, MdbTableDef* txTbl, const SalePurcLeg& L) {
    if (!mdb || !txTbl) return false;
    const bool isSale = (L.transType.compare("Sale", Qt::CaseInsensitive) == 0);
    std::vector<MdbField> fields(txTbl->num_cols);
    std::vector<QByteArray> textBuffers(txTbl->num_cols);
    for (int i = 0; i < txTbl->num_cols; i++) {
        MdbColumn* col = (MdbColumn*)g_ptr_array_index(txTbl->columns, i);
        fields[i].colnum = i;
        fields[i].is_fixed = col->is_fixed;
        fields[i].is_null = 1;
        fields[i].value = nullptr;
        fields[i].siz = 0;
    }
    auto setText = [&](int idx, const QByteArray& b) {
        textBuffers[idx] = b;
        fields[idx].is_null = 0;
        fields[idx].value = (void*)textBuffers[idx].data();
        fields[idx].siz = textBuffers[idx].size();
    };
    auto setTextOrNull = [&](int idx, const QByteArray& b, bool isNull) {
        if (isNull) { fields[idx].is_null = 1; return; }
        setText(idx, b);
    };
    auto colOf = [&](int idx) {
        return (MdbColumn*)g_ptr_array_index(txTbl->columns, idx);
    };

    guint16 rowNo = L.rowNo;
    fields[0].is_null = 0; fields[0].value = &rowNo; fields[0].siz = 2;
    guint32 vchNo = L.vchNo;
    fields[1].is_null = 0; fields[1].value = &vchNo; fields[1].siz = 4;
    double oleD = toOleDate(L.dateStr);
    fields[2].is_null = 0; fields[2].value = &oleD; fields[2].siz = 8;

    setText(3, toJet4TextForCol(L.transType.left(5), colOf(3)));

    guint16 acVal = L.accountCode;
    fields[4].is_null = 0; fields[4].value = &acVal; fields[4].siz = 2;

    setText(5, toJet4TextForCol(L.drCr, colOf(5)));

    double amtVal = L.amount;
    fields[6].is_null = 0; fields[6].value = &amtVal; fields[6].siz = 8;

    QByteArray invBytes = toJet4TextForCol(L.invoiceNo.left(50), colOf(7));
    if (invBytes.isEmpty()) { fields[7].is_null = 1; }
    else setText(7, invBytes);

    guint16 zero16 = 0;
    fields[8].is_null = 0; fields[8].value = &zero16; fields[8].siz = 2; // PartyCode 0

    QByteArray jfBytes = toJet4TextForCol(L.jform, colOf(9));
    setTextOrNull(9, jfBytes, jfBytes.isEmpty());
    QByteArray itBytes = toJet4TextForCol(L.iformTax, colOf(10));
    setTextOrNull(10, itBytes, itBytes.isEmpty());
    if (!isSale) {
        QByteArray mfBytes = toJet4TextForCol(L.mfeeStatus, colOf(11));
        setTextOrNull(11, mfBytes, mfBytes.isEmpty());
    }
    QByteArray ptBytes = toJet4TextForCol(L.marketType, colOf(12));
    setTextOrNull(12, ptBytes, ptBytes.isEmpty());
    QByteArray ccBytes = toJet4TextForCol(L.cashCredit, colOf(13));
    setTextOrNull(13, ccBytes, ccBytes.isEmpty());

    QByteArray narrBytes = toJet4TextForCol(L.narration.left(200), colOf(14));
    fields[14].is_null = 0;
    fields[14].value = narrBytes.isEmpty() ? (void*)&zero16 : (void*)narrBytes.data();
    fields[14].siz = narrBytes.size();

    if (L.entryEmpty) {
        static const char kEmpty = '\0';
        fields[15].is_null = 0; fields[15].value = (void*)&kEmpty; fields[15].siz = 0;
    } else if (L.entryType.isEmpty()) {
        fields[15].is_null = 1;
    } else {
        setText(15, toJet4TextForCol(L.entryType.left(25), colOf(15)));
    }

    guint16 dueVal = (guint16)qMax(0, L.dueDays);
    fields[16].is_null = 0; fields[16].value = &dueVal; fields[16].siz = 2;
    guint16 itemVal = L.itemCode;
    fields[17].is_null = 0; fields[17].value = &itemVal; fields[17].siz = 2;

    guint16 zero16b = 0;
    guint32 zero32 = 0;
    double zeroDbl = 0.0;
    float zeroFlt = 0.0f;
    static const char kEmpty2 = '\0';
    auto setEmpty = [&](int idx) {
        fields[idx].is_null = 0; fields[idx].value = (void*)&kEmpty2; fields[idx].siz = 0;
    };
    QByteArray zeroStr = toJet4Text("0");
    const double oleZeroDate = 2.0; // 01/01/1900 sentinel ("01/01/00")

    if (isSale) {
        setText(18, toJet4TextForCol("None", colOf(18)));
        if (L.isFirstLeg) {
            setEmpty(19); setEmpty(20);
            fields[21].is_null = 0; fields[21].value = &zero32; fields[21].siz = 4;
            fields[23].is_null = 0; fields[23].value = &zero32; fields[23].siz = 4;
            setEmpty(27);
            double bal = L.hasBalAmount ? L.balAmount : L.amount;
            fields[41].is_null = 0; fields[41].value = &bal; fields[41].siz = 8;
        }
        // 19/20/21/23/27/41 stay NULL on detail legs
    } else {
        if (L.isFirstLeg) {
            setEmpty(19); setEmpty(20);
            fields[21].is_null = 0; fields[21].value = &zero32; fields[21].siz = 4;
            fields[23].is_null = 0; fields[23].value = &zero32; fields[23].siz = 4;
            setEmpty(27);
            double bal = L.hasBalAmount ? L.balAmount : L.amount;
            fields[41].is_null = 0; fields[41].value = &bal; fields[41].siz = 8;
        } else {
            fields[23].is_null = 0; fields[23].value = &zero32; fields[23].siz = 4;
            setEmpty(28);
        }
        setEmpty(28);
        // 19/20/21/27/41 stay NULL on detail legs (23 is 0, 28 is "")
    }

    // Shared Sale/Purc defaults (native shapes)
    fields[25].is_null = isSale ? 0 : 1;
    if (isSale) { fields[25].value = &zeroDbl; fields[25].siz = 8; } // HideAmount
    fields[26].is_null = 0; fields[26].value = &zero32; fields[26].siz = 4; // FormQuery
    fields[31].is_null = 0; fields[31].value = &zero32; fields[31].siz = 4; // DebitCreditNote
    fields[35].is_null = 0; fields[35].value = &zeroDbl; fields[35].siz = 8; // ExpRate
    setEmpty(37); // ImagePath ""
    if (isSale) {
        QByteArray brBytes = toJet4TextForCol(L.broker.left(255), colOf(38));
        if (brBytes.isEmpty()) setEmpty(38); else setText(38, brBytes);
    } else {
        setEmpty(38);
    }
    fields[39].is_null = 0; fields[39].value = &zeroDbl; fields[39].siz = 8; // EstimatedAmount
    fields[42].is_null = 0; fields[42].value = &zero32; fields[42].siz = 4; // MktCommttSrNo
    double retD = oleD;
    fields[43].is_null = 0; fields[43].value = &retD; fields[43].siz = 8; // ReturnDate
    fields[45].is_null = 0; fields[45].value = &zero32; fields[45].siz = 4; // URDPurc
    fields[46].is_null = 0; fields[46].value = &zero32; fields[46].siz = 4; // E1PartyCode
    if (isSale) { setEmpty(47); setEmpty(48); setEmpty(49); }
    fields[50].is_null = 0; fields[50].value = &zero32; fields[50].siz = 4; // CompositionVch
    setText(52, toJet4TextForCol("0", colOf(52))); // PlaceOfSupply "0"
    if (isSale) {
        setEmpty(53); // ECommGSTIN "" (Purc: "0")
    } else {
        setText(53, toJet4TextForCol("0", colOf(53)));
    }
    fields[54].is_null = 0; fields[54].value = &zero32; fields[54].siz = 4; // TransReturn
    fields[55].is_null = 0; fields[55].value = &zero32; fields[55].siz = 4; // GroupTick
    guint32 invSeqVal = (guint32)(isSale ? L.invSeq : 0);
    fields[57].is_null = 0; fields[57].value = &invSeqVal; fields[57].siz = 4; // ActualInv
    fields[58].is_null = 0; fields[58].value = &zero32; fields[58].siz = 4; // ITCNotClaim
    fields[59].is_null = 0; fields[59].value = &zero32; fields[59].siz = 4; // ReverseChargePayable
    fields[60].is_null = 0; fields[60].value = &zero32; fields[60].siz = 4; // GSTOnGoodsAmount
    float tcsRate = (float)L.tcsRate;
    fields[62].is_null = 0; fields[62].value = &tcsRate; fields[62].siz = 4; // TCSRate
    double tcsTax = isSale ? L.amount : 0.0;
    fields[63].is_null = 0; fields[63].value = &tcsTax; fields[63].siz = 8; // TCSTaxable
    fields[65].is_null = 0; fields[65].value = &zero32; fields[65].siz = 4; // ChallanVchNo
    if (isSale) {
        fields[66].is_null = 0; fields[66].value = (void*)&oleZeroDate; fields[66].siz = 8;
    }
    setText(67, toJet4TextForCol("0", colOf(67))); // TempInv "0"
    float tdsRate = 0.0f;
    fields[68].is_null = 0; fields[68].value = &tdsRate; fields[68].siz = 4; // TDSRate194Q
    fields[69].is_null = 0; fields[69].value = &zeroDbl; fields[69].siz = 8; // Taxable194Q
    fields[70].is_null = 0; fields[70].value = &zero32; fields[70].siz = 4; // TDS194QChallanVchNo
    if (!isSale) {
        fields[71].is_null = 0; fields[71].value = (void*)&oleZeroDate; fields[71].siz = 8;
        setEmpty(72);
    }
    fields[73].is_null = 0; fields[73].value = &oleD; fields[73].siz = 8; // TaxInputDate
    fields[74].is_null = 0; fields[74].value = &zero32; fields[74].siz = 4; // PymtDone
    fields[75].is_null = 0; fields[75].value = &zeroDbl; fields[75].siz = 8; // tmpBookNo
    fields[76].is_null = 0; fields[76].value = &zeroDbl; fields[76].siz = 8; // tmpSlipNo
    double runB = isSale ? (double)L.invSeq : 0.0;
    fields[78].is_null = 0; fields[78].value = &runB; fields[78].siz = 8; // RunningBNo

    return mdb_insert_row(txTbl, txTbl->num_cols, fields.data()) != 0;
}

/* Cash-bank family leg (ChPt/ChRt/Pymt/Rcpt): Journal base minus per-type
 * null sets (native shapes; fixes the EntryType Seek class for ~20k rows).
 * Rcpt additionally carries LtNo "" (col 77). */
static bool insertCashLeg(MdbHandle* mdb, MdbTableDef* txTbl, const BahiKhataJournalTx& tx,
                          const QString& kind) {
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
    QByteArray dcBytes = toJet4TextForCol(tx.drCr, colDC);
    fields[5].is_null = 0; fields[5].value = dcBytes.data(); fields[5].siz = dcBytes.size();
    double amtVal = tx.amount;
    fields[6].is_null = 0; fields[6].value = &amtVal; fields[6].siz = 8;
    fields[7].is_null = 1; // InvoiceNo null
    guint16 ptVal = tx.partyCode;
    fields[8].is_null = 0; fields[8].value = &ptVal; fields[8].siz = 2;
    QByteArray narrBytes = toJet4TextForCol(tx.narration.left(200), colNarr);
    static const char kEmptyNarr = '\0';
    fields[14].is_null = 0;
    fields[14].value = narrBytes.isEmpty() ? (void*)&kEmptyNarr : (void*)narrBytes.data();
    fields[14].siz = narrBytes.size();
    // NOTE: col 15 EntryType stays NULL for the cash family (native shape).
    guint16 zero16 = 0;
    guint32 zero32 = 0;
    double zeroDbl = 0.0;
    float zeroFlt = 0.0f;
    QByteArray zeroStr = toJet4Text("0");
    static const char kEmpty = '\0';
    auto setEmpty = [&](int idx) {
        fields[idx].is_null = 0; fields[idx].value = (void*)&kEmpty; fields[idx].siz = 0;
    };
    fields[16].is_null = 0; fields[16].value = &zero16; fields[16].siz = 2;
    fields[17].is_null = 0; fields[17].value = &zero16; fields[17].siz = 2;
    if (kind == "Pymt")
        { fields[23].is_null = 0; fields[23].value = &zero32; fields[23].siz = 4; }
    fields[29].is_null = 0; fields[29].value = &oleD; fields[29].siz = 8;
    fields[30].is_null = 0; fields[30].value = &zero32; fields[30].siz = 4;
    fields[35].is_null = 0; fields[35].value = &zeroDbl; fields[35].siz = 8;
    setEmpty(37); setEmpty(38);
    fields[39].is_null = 0; fields[39].value = &zeroDbl; fields[39].siz = 8;
    fields[42].is_null = 0; fields[42].value = &zero32; fields[42].siz = 4;
    fields[45].is_null = 0; fields[45].value = &zero32; fields[45].siz = 4;
    fields[46].is_null = 0; fields[46].value = &zero32; fields[46].siz = 4;
    fields[50].is_null = 0; fields[50].value = &zero32; fields[50].siz = 4;
    fields[52].is_null = 0; fields[52].value = zeroStr.data(); fields[52].siz = zeroStr.size();
    fields[53].is_null = 0; fields[53].value = zeroStr.data(); fields[53].siz = zeroStr.size();
    fields[54].is_null = 0; fields[54].value = &zero32; fields[54].siz = 4;
    fields[55].is_null = 0; fields[55].value = &zero32; fields[55].siz = 4;
    fields[57].is_null = 0; fields[57].value = &zero32; fields[57].siz = 4;
    fields[58].is_null = 0; fields[58].value = &zero32; fields[58].siz = 4;
    fields[59].is_null = 0; fields[59].value = &zero32; fields[59].siz = 4;
    fields[60].is_null = 0; fields[60].value = &zero32; fields[60].siz = 4;
    fields[62].is_null = 0; fields[62].value = &zeroFlt; fields[62].siz = 4;
    fields[63].is_null = 0; fields[63].value = &zeroDbl; fields[63].siz = 8;
    fields[65].is_null = 0; fields[65].value = &zero32; fields[65].siz = 4;
    fields[67].is_null = 0; fields[67].value = zeroStr.data(); fields[67].siz = zeroStr.size();
    fields[68].is_null = 0; fields[68].value = &zeroFlt; fields[68].siz = 4;
    fields[69].is_null = 0; fields[69].value = &zeroDbl; fields[69].siz = 8;
    fields[70].is_null = 0; fields[70].value = &zero32; fields[70].siz = 4;
    fields[73].is_null = 0; fields[73].value = &oleD; fields[73].siz = 8;
    fields[74].is_null = 0; fields[74].value = &zero32; fields[74].siz = 4;
    fields[75].is_null = 0; fields[75].value = &zeroDbl; fields[75].siz = 8;
    fields[76].is_null = 0; fields[76].value = &zeroDbl; fields[76].siz = 8;
    if (kind == "Rcpt") setEmpty(77); // LtNo ""
    fields[78].is_null = 0; fields[78].value = &zeroDbl; fields[78].siz = 8;
    return mdb_insert_row(txTbl, txTbl->num_cols, fields.data()) != 0;
}

/* StockTransactions item lines for one Sale/Purc voucher. */
static int insertStockLines(MdbHandle* mdb, MdbTableDef* stkTxTbl, const JetInvoiceCtx& ctx,
                            guint32 vchNo, const QString& isoDate, const QString& transType) {
    if (!mdb || !stkTxTbl || ctx.items.empty()) return 0;
    int done = 0;
    double oleD = toOleDate(isoDate);
    const double oleZero = 2.0; // 01/01/1900 sentinel
    const bool isSale = ctx.isSale;
    int rowNo = 0;
    for (const auto& it : ctx.items) {
        rowNo++;
        std::vector<MdbField> fields(stkTxTbl->num_cols);
        for (int i = 0; i < stkTxTbl->num_cols; i++) {
            MdbColumn* col = (MdbColumn*)g_ptr_array_index(stkTxTbl->columns, i);
            fields[i].colnum = i;
            fields[i].is_fixed = col->is_fixed;
            fields[i].is_null = 1;
        }
        auto colOf = [&](int idx) {
            return (MdbColumn*)g_ptr_array_index(stkTxTbl->columns, idx);
        };
        auto setT = [&](int idx, const QByteArray& b) {
            fields[idx].is_null = 0; fields[idx].value = (void*)b.data(); fields[idx].siz = b.size();
        };
        auto setTNull = [&](int idx, const QByteArray& b, bool isNull) {
            if (isNull) { fields[idx].is_null = 1; return; }
            setT(idx, b);
        };
        static const char kEmpty = '\0';
        auto setE = [&](int idx) {
            fields[idx].is_null = 0; fields[idx].value = (void*)&kEmpty; fields[idx].siz = 0;
        };
        guint16 rn = (guint16)rowNo;
        fields[0].is_null = 0; fields[0].value = &rn; fields[0].siz = 2;
        fields[1].is_null = 0; fields[1].value = &oleD; fields[1].siz = 8;
        guint32 vn = vchNo;
        fields[2].is_null = 0; fields[2].value = &vn; fields[2].siz = 4;
        QByteArray ttB = toJet4TextForCol(transType.left(5), colOf(3));
        setT(3, ttB);
        guint16 ic = (guint16)it.jetCode;
        fields[4].is_null = 0; fields[4].value = &ic; fields[4].siz = 2;
        double bags = it.bags, pack = it.packing, wt = it.weight;
        double rate = it.rate, amt = it.amount;
        fields[5].is_null = 0; fields[5].value = &bags; fields[5].siz = 8;
        float packF = (float)pack;
        fields[6].is_null = 0; fields[6].value = &packF; fields[6].siz = 4;
        fields[7].is_null = 0; fields[7].value = &wt; fields[7].siz = 8;
        float vatcst = 0.0f;
        fields[8].is_null = 0; fields[8].value = &vatcst; fields[8].siz = 4;
        fields[9].is_null = 0; fields[9].value = &rate; fields[9].siz = 8;
        fields[10].is_null = 0; fields[10].value = &amt; fields[10].siz = 8;
        fields[11].is_null = 1; // DheriPurchaseDate NULL
        guint16 zero16 = 0;
        fields[12].is_null = 0; fields[12].value = &zero16; fields[12].siz = 2;
        if (isSale) {
            QByteArray gB = toJet4TextForCol(it.grade.left(50), colOf(13));
            setTNull(13, gB, gB.isEmpty());
        } else {
            fields[13].is_null = 1;
        }
        fields[14].is_null = 0; fields[14].value = &zero16; fields[14].siz = 2;
        if (isSale) {
            fields[15].is_null = 1; fields[16].is_null = 1; // Taxable/Tax NULL
        } else {
            double tax = it.taxable;
            double taxAmt = (it.gstPct > 0 && tax > 0) ? round2dbl(tax * it.gstPct / 100.0) : 0.0;
            fields[15].is_null = 0; fields[15].value = &tax; fields[15].siz = 8;
            fields[16].is_null = 0; fields[16].value = &taxAmt; fields[16].siz = 8;
        }
        QByteArray txB = toJet4TextForCol(ctx.taxStatus, colOf(17));
        setTNull(17, txB, txB.isEmpty());
        if (isSale) {
            fields[18].is_null = 1; // Narration NULL
        } else {
            setE(18);
        }
        QByteArray vtB = toJet4TextForCol(ctx.saleStatus, colOf(19));
        setTNull(19, vtB, vtB.isEmpty());
        fields[20].is_null = 1; // CommissionPartyCode NULL
        if (isSale) {
            guint32 zero32 = 0;
            fields[21].is_null = 0; fields[21].value = &zero32; fields[21].siz = 4; // GodownCode 0
            // 22..28 are NULL on Sale rows (0.0 not-null on Purc rows below).
            fields[22].is_null = 1; fields[23].is_null = 1; fields[24].is_null = 1;
            fields[25].is_null = 1; fields[26].is_null = 1; fields[27].is_null = 1;
            fields[28].is_null = 1;
        } else {
            double z = 0.0;
            fields[22].is_null = 0; fields[22].value = &z; fields[22].siz = 8;
            fields[23].is_null = 0; fields[23].value = &z; fields[23].siz = 8;
            fields[24].is_null = 0; fields[24].value = &z; fields[24].siz = 8;
            fields[25].is_null = 0; fields[25].value = &z; fields[25].siz = 8;
            fields[26].is_null = 0; fields[26].value = &z; fields[26].siz = 8;
            fields[27].is_null = 0; fields[27].value = &z; fields[27].siz = 8;
            fields[28].is_null = 0; fields[28].value = &z; fields[28].siz = 8;
        }
        fields[29].is_null = 1; // TimberItemRowNo NULL
        double zl = 0.0;
        fields[30].is_null = 0; fields[30].value = &zl; fields[30].siz = 8; // LooseWeight
        if (isSale) {
            fields[31].is_null = 1; // TaxIncluding NULL
        } else {
            // Native Purc rows carry TaxIncluding text ("Excluding" observed).
            QByteArray tiB = toJet4TextForCol("Excluding", colOf(31));
            setTNull(31, tiB, tiB.isEmpty());
        }
        float zf = 0.0f;
        fields[32].is_null = 0; fields[32].value = &zf; fields[32].siz = 4;
        fields[33].is_null = 0; fields[33].value = &zf; fields[33].siz = 4;
        fields[34].is_null = 0; fields[34].value = &zl; fields[34].siz = 8;
        // ActivationDate: native stores 2.0 (01/01/00), NOT the voucher date.
        fields[35].is_null = 0; fields[35].value = (void*)&oleZero; fields[35].siz = 8;
        fields[36].is_null = 0; fields[36].value = &zl; fields[36].siz = 8;
        setE(37);
        double z2 = 0.0;
        fields[38].is_null = 0; fields[38].value = &z2; fields[38].siz = 8;
        fields[39].is_null = 0; fields[39].value = &z2; fields[39].siz = 8;
        fields[40].is_null = 0; fields[40].value = &z2; fields[40].siz = 8;
        fields[41].is_null = 0; fields[41].value = &z2; fields[41].siz = 8;
        guint32 z32 = 0;
        fields[42].is_null = 0; fields[42].value = &z32; fields[42].siz = 4;
        float zf2 = 0.0f;
        fields[43].is_null = 0; fields[43].value = &zf2; fields[43].siz = 4;
        fields[44].is_null = 0; fields[44].value = &z2; fields[44].siz = 8;
        fields[45].is_null = 0; fields[45].value = &z2; fields[45].siz = 8;
        fields[46].is_null = 0; fields[46].value = &z2; fields[46].siz = 8;
        fields[47].is_null = 0; fields[47].value = &z32; fields[47].siz = 4;
        fields[48].is_null = 0; fields[48].value = &z32; fields[48].siz = 4;
        fields[49].is_null = 0; fields[49].value = &z32; fields[49].siz = 4;
        double ibags = it.bags;
        fields[50].is_null = 0; fields[50].value = &ibags; fields[50].siz = 8;
        if (insertJet4Row(mdb, stkTxTbl, fields.data(), stkTxTbl->num_cols)) done++;
    }
    return done;
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
                "SELECT t.id, t.voucher_no, t.voucher_date, t.trans_type, t.dr_cr, t.amount, t.narration, "
                "COALESCE(p.legacy_id, (SELECT legacy_id FROM parties WHERE name = t.party_name), (SELECT legacy_id FROM parties WHERE id = t.account_code), p.id, t.account_code) as ac_code, "
                "COALESCE(op.legacy_id, op.id, (SELECT legacy_id FROM parties WHERE name = t.opposing_account), 0) as party_code, "
                "t.party_name, t.opposing_account, "
                "t.financial_year "
                "FROM transactions t "
                "LEFT JOIN parties p ON (t.party_id = p.id OR (t.party_id IS NULL AND t.party_name = p.name)) "
                "LEFT JOIN parties op ON (t.opposing_account = op.name) ";

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
                "COALESCE(p.legacy_id, (SELECT legacy_id FROM parties WHERE name = t.party_name), (SELECT legacy_id FROM parties WHERE id = t.account_code), p.id, t.account_code) as ac_code, "
                "COALESCE(op.legacy_id, op.id, (SELECT legacy_id FROM parties WHERE name = t.opposing_account), 0) as party_code, "
                "t.party_name, t.opposing_account, "
                "t.financial_year, t.row_no, t.invoice_no, t.due_days, "
                "t.tds_amount, t.taxable_amount "
                "FROM transactions t "
                "LEFT JOIN parties p ON (t.party_id = p.id OR (t.party_id IS NULL AND t.party_name = p.name)) "
                "LEFT JOIN parties op ON (t.opposing_account = op.name) ";

            // Shared robust FY filter (same normalization as the ODBC engine).
            txSql += buildFyWhere(options.financialYear);
            txSql += "ORDER BY t.voucher_date ASC, t.id ASC;";
            QVariantList txRows = appDb.executeQuery(txSql);
            std::cout << "[EXPORT] Transactions fetched from SQLite: " << txRows.size() << ", existing in JetDB: " << existingTxSignatures.size() << std::endl;

            // StockTransactions table for Sale/Purc item lines (with indexes
            // so Jet4Writer maintains them like Transactions).
            MdbTableDef* stkTxTbl = nullptr;
            std::set<std::tuple<int, QString, QString>> existingStockSig;
            if (options.exportMillingAndStock) {
                stkTxTbl = mdb_read_table_by_name(mdb, (char*)"StockTransactions", MDB_TABLE);
                if (stkTxTbl) {
                    mdb_read_columns(stkTxTbl);
                    mdb_read_indices(stkTxTbl);
                    MdbColumn* scDate = (MdbColumn*)g_ptr_array_index(stkTxTbl->columns, 1);
                    MdbColumn* scVch = (MdbColumn*)g_ptr_array_index(stkTxTbl->columns, 2);
                    MdbColumn* scType = (MdbColumn*)g_ptr_array_index(stkTxTbl->columns, 3);
                    mdb_rewind_table(stkTxTbl);
                    while (mdb_fetch_row(stkTxTbl)) {
                        int vn = (scVch && scVch->cur_value_len == 4)
                            ? mdb_get_int32(mdb->pg_buf, scVch->cur_value_start) : 0;
                        QString dv;
                        if (scDate && scDate->cur_value_len == 8) {
                            double od = mdb_get_double(mdb->pg_buf, scDate->cur_value_start);
                            dv = QDate(1899, 12, 30).addDays((qint64)od).toString("yyyy-MM-dd");
                        }
                        QString tt;
                        if (scType && scType->cur_value_len > 0) {
                            const unsigned char* p = (const unsigned char*)mdb->pg_buf + scType->cur_value_start;
                            if (scType->cur_value_len >= 3 && p[0] == 0xFF && p[1] == 0xFE)
                                tt = QString::fromLatin1((const char*)p + 2, scType->cur_value_len - 2);
                            else {
                                QString u;
                                for (int bi = 0; bi + 1 < scType->cur_value_len; bi += 2) u += QChar(p[bi]);
                                tt = u;
                            }
                        }
                        existingStockSig.insert(std::make_tuple(vn, dv, tt.trimmed()));
                    }
                }
            }

            // Group legs by voucher (ordered), sort each by (row_no, id).
            // RowNo comes from SQLite (native-preserved); fallback counter fills gaps.
            struct LegRow { QVariantMap m; int id = 0; int rowNo = 0; };
            QList<QString> groupOrder;
            QMap<QString, QList<LegRow>> groups;
            {
                int rowIdx = 1;
                for (const auto& tVar : txRows) {
                    QVariantMap t = tVar.toMap();
                    int rawVchNo = t.value("voucher_no").toInt();
                    if (rawVchNo <= 0) {
                        QString vStr = t.value("voucher_no").toString();
                        static QRegularExpression reDigits(R"(\d+)");
                        QRegularExpressionMatch mm = reDigits.match(vStr);
                        if (mm.hasMatch()) rawVchNo = mm.captured(0).toInt();
                        else rawVchNo = rowIdx;
                    }
                    QString isoDate = t.value("voucher_date").toString().left(10);
                    QString vKey = QString("%1|%2|%3").arg(rawVchNo).arg(isoDate).arg(t.value("financial_year").toString());
                    if (!groups.contains(vKey)) { groups[vKey] = QList<LegRow>(); groupOrder.append(vKey); }
                    LegRow lr;
                    lr.m = t; lr.m["__vch"] = rawVchNo; lr.m["__date"] = isoDate;
                    lr.id = t.value("id").toInt();
                    lr.rowNo = t.value("row_no").toInt();
                    groups[vKey].append(lr);
                    rowIdx++;
                }
            }
            // Signed per-ledger deltas (Dr-positive) for Ledgers.CurrentBalance.
            QMap<int, double> balDeltas;
            static int txDbg = 0;
            long stockLines = 0;

            auto noteInserted = [&](int rawVchNo, const QString& isoDate, int acCode,
                                    double amt, const QString& drCr,
                                    const std::tuple<int, QString, int, int, QString>& sig) {
                summary.transactionsExported++;
                if (drCr == "Dr") summary.totalDebitAmount += amt;
                else summary.totalCreditAmount += amt;
                existingTxSignatures.insert(sig);
                if (acCode > 0 && std::abs(amt) >= 0.0005) {
                    double signedAmt = (drCr == "Dr") ? amt : -amt;
                    balDeltas[acCode] = round2dbl(balDeltas.value(acCode, 0.0) + signedAmt);
                }
                if (++txDbg <= 5) {
                    std::cout << "[TX INSERT] vch=" << rawVchNo << " drCr=" << drCr.toStdString()
                              << " amt=" << amt << " ok=1" << std::endl;
                }
            };

            JetInvoiceCache invCache = buildInvoiceCache(appDb);

            for (const QString& vKey : groupOrder) {
                QList<LegRow> legs = groups[vKey];
                std::sort(legs.begin(), legs.end(), [](const LegRow& a, const LegRow& b) {
                    int ra = a.rowNo > 0 ? a.rowNo : 1000000 + a.id;
                    int rb = b.rowNo > 0 ? b.rowNo : 1000000 + b.id;
                    if (ra != rb) return ra < rb;
                    return a.id < b.id;
                });
                // Assign RowNo: keep sqlite row_no on first sight, else smallest
                // free positive int (native rows are dense 1..n per voucher).
                {
                    QSet<int> seen;
                    int nextFree = 1;
                    for (auto& lr : legs) {
                        int orig = lr.m.value("row_no").toInt();
                        if (orig > 0 && !seen.contains(orig)) {
                            lr.rowNo = orig;
                            seen.insert(orig);
                        } else {
                            while (seen.contains(nextFree)) nextFree++;
                            lr.rowNo = nextFree;
                            seen.insert(nextFree);
                        }
                        while (seen.contains(nextFree)) nextFree++;
                    }
                }
                QString vType = normalizeTxType(legs.first().m.value("trans_type", "Jrnl").toString());
                const bool isSalePurc = (vType.compare("Sale", Qt::CaseInsensitive) == 0 ||
                                         vType.compare("Purc", Qt::CaseInsensitive) == 0);

                // Invoice context for Sale/Purc (entry labels, items, stock).
                JetInvoiceCtx ictx;
                QString groupInvoice;
                if (isSalePurc) {
                    for (auto& lr : legs) {
                        QString ino = lr.m.value("invoice_no").toString().trimmed();
                        if (!ino.isEmpty()) { groupInvoice = ino; break; }
                    }
                    ictx = fetchInvoiceCtxCached(invCache, vType, groupInvoice);
                }

                bool goodsAssigned = false;
                int fallbackIdx = 0;
                bool stockDoneForGroup = false;
                for (int li = 0; li < legs.size(); li++) {
                    QVariantMap t = legs[li].m;
                    int rawVchNo = t.value("__vch").toInt();
                    QString isoDate = t.value("__date").toString();
                    int acCode = t.value("ac_code").toInt();
                    double amt = round2(t.value("amount").toDouble());
                    int amtCents = static_cast<int>(std::round(amt * 100.0));
                    QString drCr = t.value("dr_cr", "Dr").toString().left(2);
                    QString narration = t.value("narration").toString();
                    QString invoiceNo = t.value("invoice_no").toString().left(50);
                    int dueDays = t.value("due_days", 0).toInt();

                    auto sig = std::make_tuple(rawVchNo, isoDate, acCode, amtCents, drCr);
                    if (existingTxSignatures.find(sig) != existingTxSignatures.end()) {
                        summary.transactionsExported++;
                        if (drCr == "Dr") summary.totalDebitAmount += amt;
                        else summary.totalCreditAmount += amt;
                        continue;
                    }

                    bool txInsOk = false;
                    const bool isFirst = (li == 0);
                    if (vType.compare("Jrnl", Qt::CaseInsensitive) == 0 ||
                        (vType != "Sale" && vType != "Purc" && vType != "ChPt" &&
                         vType != "ChRt" && vType != "Pymt" && vType != "Rcpt")) {
                        // Journal + all other rare types: frozen canonical shape.
                        BahiKhataJournalTx tx;
                        tx.rowNo = (guint16)legs[li].rowNo;
                        tx.vchNo = (guint32)rawVchNo;
                        tx.dateStr = isoDate;
                        tx.transType = vType;
                        tx.accountCode = (guint16)acCode;
                        tx.drCr = drCr;
                        tx.amount = amt;
                        tx.narration = narration;
                        tx.partyCode = 0;
                        txInsOk = insertBahiKhataJournalTx(mdb, txTbl, tx);
                    } else if (vType == "ChPt" || vType == "ChRt" ||
                               vType == "Pymt" || vType == "Rcpt") {
                        BahiKhataJournalTx tx;
                        tx.rowNo = (guint16)legs[li].rowNo;
                        tx.vchNo = (guint32)rawVchNo;
                        tx.dateStr = isoDate;
                        tx.transType = vType;
                        tx.accountCode = (guint16)acCode;
                        tx.drCr = drCr;
                        tx.amount = amt;
                        tx.narration = narration;
                        tx.partyCode = 0;
                        txInsOk = insertCashLeg(mdb, txTbl, tx, vType);
                    } else {
                        // Sale / Purc leg with resolved shape.
                        SalePurcLeg L;
                        L.rowNo = (guint16)legs[li].rowNo;
                        L.vchNo = (guint32)rawVchNo;
                        L.dateStr = isoDate;
                        L.transType = vType;
                        L.accountCode = (guint16)acCode;
                        L.drCr = drCr;
                        L.amount = amt;
                        if (vType == "Sale" && narration.startsWith("Sales Invoice", Qt::CaseInsensitive)) {
                            L.narration = "";
                        } else {
                            L.narration = narration;
                        }
                        L.invoiceNo = invoiceNo;
                        L.isFirstLeg = isFirst;
                        L.dueDays = dueDays;
                        if (isFirst) {
                            L.entryType = "Ledger";
                            L.itemCode = 0;
                            L.hasBalAmount = true;
                            L.balAmount = amt;
                        } else {
                            QString party = t.value("party_name").toString();
                            L.entryType = resolveDetailEntryType(
                                ictx, amt, acCode, party, goodsAssigned, fallbackIdx);
                            if (L.entryType == "Goods Amount") goodsAssigned = true;
                            if (L.entryType != "Round Off" && L.entryType != "TDS194Q" &&
                                L.entryType != "TCS" && L.entryType != "DamiTDS" &&
                                L.entryType != "Freight") {
                                L.itemCode = (guint16)ictx.headerItemCode;
                            } else {
                                L.itemCode = 0;
                            }
                            if (L.entryType != "Ledger" && L.entryType != "Goods Amount" &&
                                L.entryType != "SGST" && L.entryType != "CGST" &&
                                L.entryType != "IGST" && L.entryType != "Round Off" &&
                                L.entryType != "TDS194Q" && L.entryType != "TCS" &&
                                L.entryType != "DamiTDS") {
                                std::cout << "[TX LABEL] vch=" << rawVchNo << " ac=" << acCode
                                          << " amt=" << amt << " -> " << L.entryType.toStdString()
                                          << " (fallback)" << std::endl;
                            }
                        }
                        L.jform = ictx.valid ? ictx.saleStatus
                                             : (vType == "Sale" ? "Self Sale" : "Self Purchase");
                        L.iformTax = ictx.valid ? ictx.taxStatus : "GST";
                        L.mfeeStatus = ictx.valid ? ictx.marketFeeStatus
                                                  : (vType == "Purc" ? "Paid" : "");
                        L.marketType = ictx.valid ? ictx.marketType : "Market Type (With Stock)";
                        L.cashCredit = "Credit";
                        L.broker = ictx.valid ? ictx.broker : "";
                        L.invSeq = ictx.valid ? ictx.invSeq : parseInvSeq(invoiceNo);
                        L.tcsRate = ictx.valid ? ictx.tcsRate : 0.0;
                        txInsOk = insertSalePurcLeg(mdb, txTbl, L);
                    }
                    if (!txInsOk) {
                        std::cout << "[TX INSERT] vch=" << rawVchNo << " FAILED drCr="
                                  << drCr.toStdString() << " amt=" << amt << std::endl;
                        continue;
                    }
                    noteInserted(rawVchNo, isoDate, acCode, amt, drCr, sig);
                }

                // Stock item lines once per Sale/Purc voucher with items.
                if (isSalePurc && stkTxTbl && ictx.valid && !ictx.items.empty()) {
                    auto skey = std::make_tuple(
                        legs.first().m.value("__vch").toInt(),
                        legs.first().m.value("__date").toString(), vType);
                    if (existingStockSig.find(skey) == existingStockSig.end()) {
                        int n = insertStockLines(mdb, stkTxTbl, ictx,
                                                 (guint32)legs.first().m.value("__vch").toInt(),
                                                 legs.first().m.value("__date").toString(), vType);
                        if (n > 0) {
                            stockLines += n;
                            existingStockSig.insert(skey);
                        }
                    }
                }
            }
            std::cout << "[EXPORT] Total Transactions Exported: " << summary.transactionsExported
                      << ", StockTransactions lines: " << stockLines << std::endl;

            // Apply cached ledger balances for newly inserted vouchers so the
            // app reflects them without a manual open+save per voucher.
            updateProgress(80, "Updating cached ledger balances...");
            applyLedgerBalanceDeltas(mdb, balDeltas);

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
