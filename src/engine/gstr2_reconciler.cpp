#include "gstr2_reconciler.h"
#include "../database_manager.h"
#include <QRegularExpression>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QSet>
#include <QFile>
#include <QFileInfo>
#include <QXmlStreamReader>
#include <miniz.h>
#include <cmath>

namespace MahadevERP {

static inline double round2(double val) {
    return std::round(val * 100.0) / 100.0;
}

QString Gstr2Reconciler::normalizeGstin(const QString& rawGstin) {
    return rawGstin.trimmed().toUpper();
}

QString Gstr2Reconciler::normalizeInvoiceNumber(const QString& rawInvNo) {
    if (rawInvNo.isEmpty()) return "";

    QString result;
    QString currentDigitChunk;

    for (const QChar& c : rawInvNo) {
        if (c.isDigit()) {
            currentDigitChunk.append(c);
        } else {
            if (!currentDigitChunk.isEmpty()) {
                // Strip leading zeros from current digit chunk
                int start = 0;
                while (start < currentDigitChunk.length() - 1 && currentDigitChunk[start] == '0') {
                    start++;
                }
                result.append(currentDigitChunk.mid(start));
                currentDigitChunk.clear();
            }
            if (c.isLetter()) {
                result.append(c.toUpper());
            }
        }
    }

    if (!currentDigitChunk.isEmpty()) {
        int start = 0;
        while (start < currentDigitChunk.length() - 1 && currentDigitChunk[start] == '0') {
            start++;
        }
        result.append(currentDigitChunk.mid(start));
    }

    return result.isEmpty() ? rawInvNo.trimmed().toUpper() : result;
}

QList<Gstr2PortalRecord> Gstr2Reconciler::parseGstr2BJson(const QByteArray& jsonData, const QString& returnPeriod) {
    QList<Gstr2PortalRecord> records;
    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(jsonData, &err);
    if (doc.isNull() || !doc.isObject()) return records;

    QJsonObject root = doc.object();
    QJsonObject dataObj = root.contains("data") ? root["data"].toObject() : root;
    QJsonObject docData = dataObj.contains("docdata") ? dataObj["docdata"].toObject() : dataObj;

    QString detectedPeriod = returnPeriod;
    if (detectedPeriod.isEmpty()) {
        if (dataObj.contains("rtnprd")) detectedPeriod = dataObj["rtnprd"].toString();
        else if (root.contains("rtnprd")) detectedPeriod = root["rtnprd"].toString();
        else if (root.contains("fp")) detectedPeriod = root["fp"].toString();
    }

    auto parseInvObject = [&](const QJsonObject& inv, const QString& gstin, const QString& tradeName, const QString& section, const QDate& filingDate) -> Gstr2PortalRecord {
        Gstr2PortalRecord rec;
        rec.section = section;
        rec.supplierGstin = gstin;
        rec.supplierName = tradeName;
        rec.invoiceNo = inv.contains("inum") ? inv["inum"].toString() : inv["nt_num"].toString();
        rec.normalizedInvNo = normalizeInvoiceNumber(rec.invoiceNo);
        
        QString dtStr = inv.contains("dt") ? inv["dt"].toString() : inv["nt_dt"].toString();
        rec.invoiceDate = QDate::fromString(dtStr, "dd-MM-yyyy");
        if (!rec.invoiceDate.isValid()) rec.invoiceDate = QDate::fromString(dtStr, "yyyy-MM-dd");
        if (!rec.invoiceDate.isValid()) rec.invoiceDate = QDate::fromString(dtStr, "dd/MM/yyyy");

        rec.invoiceType = inv.contains("typ") ? inv["typ"].toString("R") : inv["ntty"].toString("R");
        rec.returnPeriod = detectedPeriod;
        rec.itcEligibility = inv.value("itcavl").toString("Y");
        rec.supplierFilingDate = filingDate;

        // Check direct summary fields on inv (Official GSTR-2B JSON format)
        if (inv.contains("txval") || inv.contains("cgst") || inv.contains("sgst") || inv.contains("igst")) {
            rec.taxableValue = inv["txval"].toDouble();
            rec.cgst = inv["cgst"].toDouble();
            rec.sgst = inv["sgst"].toDouble();
            rec.igst = inv["igst"].toDouble();
            rec.cess = inv["cess"].toDouble();
            rec.taxAmount = round2(rec.cgst + rec.sgst + rec.igst + rec.cess);
            rec.totalValue = inv.contains("val") ? inv["val"].toDouble() : round2(rec.taxableValue + rec.taxAmount);
        } else if (inv.contains("items") && inv["items"].isArray()) {
            // Nested items array fallback (GSTR-2A / GSTR-1 format)
            for (const auto& itemVal : inv["items"].toArray()) {
                QJsonObject itm = itemVal.toObject();
                rec.taxableValue += itm["txval"].toDouble();
                rec.igst += itm.contains("iamt") ? itm["iamt"].toDouble() : itm["igst"].toDouble();
                rec.cgst += itm.contains("camt") ? itm["camt"].toDouble() : itm["cgst"].toDouble();
                rec.sgst += itm.contains("samt") ? itm["samt"].toDouble() : itm["sgst"].toDouble();
                rec.cess += itm.contains("csamt") ? itm["csamt"].toDouble() : itm["cess"].toDouble();
            }
            rec.taxAmount = round2(rec.igst + rec.cgst + rec.sgst + rec.cess);
            rec.taxableValue = round2(rec.taxableValue);
            rec.totalValue = inv.contains("val") ? inv["val"].toDouble() : round2(rec.taxableValue + rec.taxAmount);
        }
        return rec;
    };

    // 1. Ingest B2B Invoices
    if (docData.contains("b2b") && docData["b2b"].isArray()) {
        for (const auto& suppVal : docData["b2b"].toArray()) {
            QJsonObject supp = suppVal.toObject();
            QString gstin = normalizeGstin(supp["ctin"].toString());
            QString tradeName = supp["trdnm"].toString();
            QDate filingDate = QDate::fromString(supp["supfildt"].toString(), "dd-MM-yyyy");

            for (const auto& invVal : supp["inv"].toArray()) {
                records.append(parseInvObject(invVal.toObject(), gstin, tradeName, "B2B", filingDate));
            }
        }
    }

    // 2. Ingest B2BA (Amendments)
    if (docData.contains("b2ba") && docData["b2ba"].isArray()) {
        for (const auto& suppVal : docData["b2ba"].toArray()) {
            QJsonObject supp = suppVal.toObject();
            QString gstin = normalizeGstin(supp["ctin"].toString());
            QString tradeName = supp["trdnm"].toString();
            QDate filingDate = QDate::fromString(supp["supfildt"].toString(), "dd-MM-yyyy");

            for (const auto& invVal : supp["inv"].toArray()) {
                records.append(parseInvObject(invVal.toObject(), gstin, tradeName, "B2BA", filingDate));
            }
        }
    }

    // 3. Ingest CDNR (Credit / Debit Notes)
    if (docData.contains("cdnr") && docData["cdnr"].isArray()) {
        for (const auto& suppVal : docData["cdnr"].toArray()) {
            QJsonObject supp = suppVal.toObject();
            QString gstin = normalizeGstin(supp["ctin"].toString());
            QString tradeName = supp["trdnm"].toString();
            QDate filingDate = QDate::fromString(supp["supfildt"].toString(), "dd-MM-yyyy");

            for (const auto& ntVal : supp["nt"].toArray()) {
                records.append(parseInvObject(ntVal.toObject(), gstin, tradeName, "CDNR", filingDate));
            }
        }
    }

    // 4. Ingest CDNRA (Amendments to Credit / Debit Notes)
    if (docData.contains("cdnra") && docData["cdnra"].isArray()) {
        for (const auto& suppVal : docData["cdnra"].toArray()) {
            QJsonObject supp = suppVal.toObject();
            QString gstin = normalizeGstin(supp["ctin"].toString());
            QString tradeName = supp["trdnm"].toString();
            QDate filingDate = QDate::fromString(supp["supfildt"].toString(), "dd-MM-yyyy");

            for (const auto& ntVal : supp["nt"].toArray()) {
                records.append(parseInvObject(ntVal.toObject(), gstin, tradeName, "CDNRA", filingDate));
            }
        }
    }

    return records;
}

QList<Gstr2PortalRecord> Gstr2Reconciler::parseGstr2BExcel(const QString& filePath, const QString& returnPeriod) {
    QList<Gstr2PortalRecord> records;
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) return records;

    QByteArray fileBytes = file.readAll();
    file.close();
    if (fileBytes.isEmpty()) return records;

    mz_zip_archive zipArchive;
    memset(&zipArchive, 0, sizeof(zipArchive));
    if (!mz_zip_reader_init_mem(&zipArchive, fileBytes.constData(), static_cast<size_t>(fileBytes.size()), 0)) {
        return records;
    }

    // 1. Read sharedStrings.xml
    QStringList sharedStrings;
    int sstIdx = mz_zip_reader_locate_file(&zipArchive, "xl/sharedStrings.xml", nullptr, 0);
    if (sstIdx >= 0) {
        size_t uncompSize = 0;
        void* sstData = mz_zip_reader_extract_to_heap(&zipArchive, sstIdx, &uncompSize, 0);
        if (sstData) {
            QByteArray xmlBytes(static_cast<const char*>(sstData), static_cast<int>(uncompSize));
            mz_free(sstData);

            QXmlStreamReader xml(xmlBytes);
            while (!xml.atEnd()) {
                xml.readNext();
                if (xml.isStartElement() && xml.name() == QLatin1String("t")) {
                    sharedStrings.append(xml.readElementText());
                }
            }
        }
    }

    // 2. Read workbook.xml to map sheet names to sheet IDs
    QMap<QString, QString> sheetTargets;
    int wbIdx = mz_zip_reader_locate_file(&zipArchive, "xl/workbook.xml", nullptr, 0);
    if (wbIdx >= 0) {
        size_t wbSize = 0;
        void* wbData = mz_zip_reader_extract_to_heap(&zipArchive, wbIdx, &wbSize, 0);
        if (wbData) {
            QByteArray wbXml(static_cast<const char*>(wbData), static_cast<int>(wbSize));
            mz_free(wbData);

            QXmlStreamReader xml(wbXml);
            while (!xml.atEnd()) {
                xml.readNext();
                if (xml.isStartElement() && xml.name() == QLatin1String("sheet")) {
                    QString name = xml.attributes().value("name").toString().trimmed();
                    QString rId = xml.attributes().value("r:id").toString();
                    if (rId.isEmpty()) rId = xml.attributes().value("id").toString();
                    sheetTargets[name] = rId;
                }
            }
        }
    }

    // Process all sheet files inside the zip
    mz_uint numFiles = mz_zip_reader_get_num_files(&zipArchive);
    for (mz_uint i = 0; i < numFiles; ++i) {
        mz_zip_archive_file_stat fileStat;
        if (!mz_zip_reader_file_stat(&zipArchive, i, &fileStat)) continue;

        QString fn = QString::fromUtf8(fileStat.m_filename);
        if (!fn.startsWith("xl/worksheets/sheet", Qt::CaseInsensitive) || !fn.endsWith(".xml", Qt::CaseInsensitive)) {
            continue;
        }

        size_t sheetSize = 0;
        void* sheetData = mz_zip_reader_extract_to_heap(&zipArchive, i, &sheetSize, 0);
        if (!sheetData) continue;

        QByteArray sheetXmlBytes(static_cast<const char*>(sheetData), static_cast<int>(sheetSize));
        mz_free(sheetData);

        QXmlStreamReader xml(sheetXmlBytes);
        QVector<QVector<QString>> matrix;
        QVector<QString> currentRow;
        QString currentCellType;
        QString currentCellValue;
        int currentColIdx = 0;

        while (!xml.atEnd()) {
            xml.readNext();
            if (xml.isStartElement()) {
                if (xml.name() == QLatin1String("row")) {
                    currentRow.clear();
                    currentColIdx = 0;
                } else if (xml.name() == QLatin1String("c")) {
                    currentCellType = xml.attributes().value("t").toString();
                    QString rAttr = xml.attributes().value("r").toString();
                    // Parse column letters to 0-indexed column
                    int targetCol = 0;
                    for (const QChar& ch : rAttr) {
                        if (ch.isLetter()) {
                            targetCol = targetCol * 26 + (ch.toUpper().unicode() - 'A' + 1);
                        } else break;
                    }
                    targetCol = qMax(0, targetCol - 1);
                    while (currentRow.size() < targetCol) {
                        currentRow.append("");
                    }
                } else if (xml.name() == QLatin1String("v") || xml.name() == QLatin1String("t")) {
                    currentCellValue = xml.readElementText().trimmed();
                    if (currentCellType == "s") {
                        int sstId = currentCellValue.toInt();
                        if (sstId >= 0 && sstId < sharedStrings.size()) {
                            currentCellValue = sharedStrings[sstId];
                        }
                    }
                    currentRow.append(currentCellValue);
                }
            } else if (xml.isEndElement() && xml.name() == QLatin1String("row")) {
                if (!currentRow.isEmpty()) {
                    matrix.append(currentRow);
                }
            }
        }

        // Search for B2B invoice rows in this sheet matrix
        // Typical GSTR-2B B2B layout:
        // Col 0: GSTIN, Col 1: Trade Name, Col 2: Inv#, Col 3: Type, Col 4: Date, Col 5: Val, Col 8: Taxable, Col 9: IGST, Col 10: CGST, Col 11: SGST, Col 12: Cess
        for (const auto& row : matrix) {
            if (row.size() < 6) continue;
            QString gstin = row[0].trimmed().toUpper();
            if (gstin.length() == 15 && (gstin[0].isDigit() || gstin.startsWith("06") || gstin.startsWith("03") || gstin.startsWith("07"))) {
                Gstr2PortalRecord rec;
                rec.supplierGstin = gstin;
                rec.supplierName = row.size() > 1 ? row[1].trimmed() : "";
                rec.invoiceNo = row.size() > 2 ? row[2].trimmed() : "";
                rec.normalizedInvNo = normalizeInvoiceNumber(rec.invoiceNo);

                QString dtStr = row.size() > 4 ? row[4].trimmed() : "";
                rec.invoiceDate = QDate::fromString(dtStr, "dd/MM/yyyy");
                if (!rec.invoiceDate.isValid()) rec.invoiceDate = QDate::fromString(dtStr, "dd-MM-yyyy");
                if (!rec.invoiceDate.isValid()) rec.invoiceDate = QDate::fromString(dtStr, "yyyy-MM-dd");

                rec.totalValue = row.size() > 5 ? row[5].toDouble() : 0.0;
                rec.taxableValue = row.size() > 8 ? row[8].toDouble() : 0.0;
                rec.igst = row.size() > 9 ? row[9].toDouble() : 0.0;
                rec.cgst = row.size() > 10 ? row[10].toDouble() : 0.0;
                rec.sgst = row.size() > 11 ? row[11].toDouble() : 0.0;
                rec.cess = row.size() > 12 ? row[12].toDouble() : 0.0;
                rec.taxAmount = round2(rec.igst + rec.cgst + rec.sgst + rec.cess);
                rec.section = "B2B";
                rec.returnPeriod = returnPeriod;
                rec.itcEligibility = "Y";

                if (!rec.invoiceNo.isEmpty()) {
                    records.append(rec);
                }
            }
        }
    }

    mz_zip_reader_end(&zipArchive);
    return records;
}

QList<Gstr2PortalRecord> Gstr2Reconciler::loadPortalRecordsFromFile(const QString& filePath, const QString& returnPeriod) {
    QFileInfo fi(filePath);
    QString ext = fi.suffix().toLower();
    if (ext == "json") {
        QFile f(filePath);
        if (f.open(QIODevice::ReadOnly)) {
            QByteArray bytes = f.readAll();
            f.close();
            return parseGstr2BJson(bytes, returnPeriod);
        }
    } else if (ext == "xlsx" || ext == "xls") {
        return parseGstr2BExcel(filePath, returnPeriod);
    }
    return {};
}

QList<Gstr2BookRecord> Gstr2Reconciler::loadBookPurchasesFromDb(const QDate& fromDate, const QDate& toDate) {
    QList<Gstr2BookRecord> records;
    DatabaseManager& db = DatabaseManager::instance();
    QSet<QString> seenKeys;

    QString fromStr = fromDate.toString("yyyy-MM-dd");
    QString toStr = toDate.toString("yyyy-MM-dd");

    // 1. Query purchase_invoices table (Primary Mahadev ERP Purchase Register)
    // Filter to registered B2B purchases with valid 15-char GSTIN and tax / taxable amount
    QString piSql = R"(
        SELECT p.id AS voucher_id,
               p.voucher_no,
               COALESCE(p.invoice_no, p.voucher_no) AS invoice_no,
               p.invoice_date,
               COALESCE(p.supplier_name, 'Supplier') AS supplier_name,
               COALESCE(p.gstin, '') AS supplier_gstin,
               COALESCE(p.taxable_amount, p.total_amount) AS taxable_val,
               COALESCE(p.cgst_amount, 0.0) AS cgst,
               COALESCE(p.sgst_amount, 0.0) AS sgst,
               COALESCE(p.igst_amount, 0.0) AS igst,
               COALESCE(p.total_amount, 0.0) AS total_val
        FROM purchase_invoices p
        WHERE p.invoice_date >= ? AND p.invoice_date <= ?
          AND p.gstin IS NOT NULL AND length(trim(p.gstin)) = 15
          AND (COALESCE(p.cgst_amount, 0) > 0 OR COALESCE(p.sgst_amount, 0) > 0 OR COALESCE(p.igst_amount, 0) > 0 OR COALESCE(p.taxable_amount, 0) > 0)
        ORDER BY p.invoice_date ASC, p.id ASC
    )";

    QVariantList piRows = db.executeQuery(piSql, { fromStr, toStr });
    for (const auto& rowVal : piRows) {
        QVariantMap row = rowVal.toMap();
        Gstr2BookRecord rec;
        rec.voucherId = row["voucher_id"].toInt();
        rec.supplierGstin = normalizeGstin(row["supplier_gstin"].toString());
        rec.supplierName = row["supplier_name"].toString();
        rec.invoiceNo = row["invoice_no"].toString().trimmed();
        rec.normalizedInvNo = normalizeInvoiceNumber(rec.invoiceNo);
        rec.invoiceDate = QDate::fromString(row["invoice_date"].toString(), "yyyy-MM-dd");
        rec.taxableValue = row["taxable_val"].toDouble();
        rec.cgst = row["cgst"].toDouble();
        rec.sgst = row["sgst"].toDouble();
        rec.igst = row["igst"].toDouble();
        rec.cess = 0.0;
        rec.taxAmount = round2(rec.cgst + rec.sgst + rec.igst + rec.cess);
        rec.totalValue = row["total_val"].toDouble();

        QString key = QString("%1|%2|%3").arg(rec.supplierGstin, rec.normalizedInvNo, rec.invoiceDate.toString("yyyyMMdd"));
        seenKeys.insert(key);
        records.append(rec);
    }

    // 2. Also query vouchers table for any taxable expense/journal vouchers with registered GSTIN
    QString vSql = R"(
        SELECT v.id AS voucher_id,
               v.voucher_no,
               COALESCE(v.instrument_no, v.voucher_no) AS invoice_no,
               COALESCE(v.instrument_date, v.voucher_date) AS invoice_date,
               pt.name AS supplier_name,
               COALESCE(pt.gstin, '') AS supplier_gstin,
               COALESCE(v.taxable_amount, 0.0) AS taxable_val,
               COALESCE(v.cgst_amount, 0.0) AS cgst,
               COALESCE(v.sgst_amount, 0.0) AS sgst,
               COALESCE(v.igst_amount, 0.0) AS igst,
               COALESCE(v.cess_amount, 0.0) AS cess,
               COALESCE(v.amount, 0.0) AS total_val
        FROM vouchers v
        JOIN parties pt ON (v.party_id = pt.id OR v.ledger_id = pt.id)
        WHERE v.voucher_type IN ('PURCHASE', 'Purchase', 'PURCHASE_VOUCHER', 'DEBIT_NOTE', 'Debit Note', 'Journal', 'Payment')
          AND v.voucher_date >= ? AND v.voucher_date <= ?
          AND pt.gstin IS NOT NULL AND length(trim(pt.gstin)) = 15
          AND (COALESCE(v.taxable_amount, 0) > 0 OR COALESCE(v.cgst_amount, 0) > 0 OR COALESCE(v.igst_amount, 0) > 0)
    )";

    QVariantList vRows = db.executeQuery(vSql, { fromStr, toStr });
    for (const auto& rowVal : vRows) {
        QVariantMap row = rowVal.toMap();
        Gstr2BookRecord rec;
        rec.voucherId = row["voucher_id"].toInt();
        rec.supplierGstin = normalizeGstin(row["supplier_gstin"].toString());
        rec.supplierName = row["supplier_name"].toString();
        rec.invoiceNo = row["invoice_no"].toString().trimmed();
        rec.normalizedInvNo = normalizeInvoiceNumber(rec.invoiceNo);
        rec.invoiceDate = QDate::fromString(row["invoice_date"].toString(), "yyyy-MM-dd");
        rec.taxableValue = row["taxable_val"].toDouble();
        rec.cgst = row["cgst"].toDouble();
        rec.sgst = row["sgst"].toDouble();
        rec.igst = row["igst"].toDouble();
        rec.cess = row["cess"].toDouble();
        rec.taxAmount = round2(rec.cgst + rec.sgst + rec.igst + rec.cess);
        rec.totalValue = row["total_val"].toDouble();

        QString key = QString("%1|%2|%3").arg(rec.supplierGstin, rec.normalizedInvNo, rec.invoiceDate.toString("yyyyMMdd"));
        if (!seenKeys.contains(key)) {
            seenKeys.insert(key);
            records.append(rec);
        }
    }

    return records;
}

QList<Gstr2BookRecord> Gstr2Reconciler::loadBookPurchasesForReconciliation(
    const QDate& fromDate,
    const QDate& toDate,
    const QList<Gstr2PortalRecord>& portalRecords
) {
    QList<Gstr2BookRecord> records = loadBookPurchasesFromDb(fromDate, toDate);
    QSet<QString> seenKeys;
    for (const auto& r : records) {
        seenKeys.insert(QString("%1|%2").arg(r.supplierGstin, r.normalizedInvNo));
    }

    // Target specific suppliers from portal whose invoices are dated prior to fromDate (late filed)
    QSet<QString> priorSupplierGstins;
    for (const auto& p : portalRecords) {
        if (p.invoiceDate.isValid() && p.invoiceDate < fromDate && !p.supplierGstin.isEmpty()) {
            priorSupplierGstins.insert(p.supplierGstin);
        }
    }

    if (!priorSupplierGstins.isEmpty()) {
        DatabaseManager& db = DatabaseManager::instance();
        for (const QString& gstin : priorSupplierGstins) {
            QString sql = R"(
                SELECT p.id AS voucher_id,
                       p.voucher_no,
                       COALESCE(p.invoice_no, p.voucher_no) AS invoice_no,
                       p.invoice_date,
                       COALESCE(p.supplier_name, 'Supplier') AS supplier_name,
                       COALESCE(p.gstin, '') AS supplier_gstin,
                       COALESCE(p.taxable_amount, p.total_amount) AS taxable_val,
                       COALESCE(p.cgst_amount, 0.0) AS cgst,
                       COALESCE(p.sgst_amount, 0.0) AS sgst,
                       COALESCE(p.igst_amount, 0.0) AS igst,
                       COALESCE(p.total_amount, 0.0) AS total_val
                FROM purchase_invoices p
                WHERE p.gstin = ? AND p.invoice_date < ? AND p.invoice_date >= ?
                  AND (COALESCE(p.cgst_amount, 0) > 0 OR COALESCE(p.sgst_amount, 0) > 0 OR COALESCE(p.igst_amount, 0) > 0)
            )";
            QDate lookbackDate = fromDate.addMonths(-12);
            QVariantList rows = db.executeQuery(sql, { gstin, fromDate.toString("yyyy-MM-dd"), lookbackDate.toString("yyyy-MM-dd") });
            for (const auto& rowVal : rows) {
                QVariantMap row = rowVal.toMap();
                Gstr2BookRecord rec;
                rec.voucherId = row["voucher_id"].toInt();
                rec.supplierGstin = normalizeGstin(row["supplier_gstin"].toString());
                rec.supplierName = row["supplier_name"].toString();
                rec.invoiceNo = row["invoice_no"].toString().trimmed();
                rec.normalizedInvNo = normalizeInvoiceNumber(rec.invoiceNo);
                rec.invoiceDate = QDate::fromString(row["invoice_date"].toString(), "yyyy-MM-dd");
                rec.taxableValue = row["taxable_val"].toDouble();
                rec.cgst = row["cgst"].toDouble();
                rec.sgst = row["sgst"].toDouble();
                rec.igst = row["igst"].toDouble();
                rec.cess = 0.0;
                rec.taxAmount = round2(rec.cgst + rec.sgst + rec.igst + rec.cess);
                rec.totalValue = row["total_val"].toDouble();

                QString key = QString("%1|%2").arg(rec.supplierGstin, rec.normalizedInvNo);
                if (!seenKeys.contains(key)) {
                    seenKeys.insert(key);
                    records.append(rec);
                }
            }
        }
    }

    return records;
}

QList<Gstr2BookRecord> Gstr2Reconciler::loadBookPurchasesForPeriod(const QString& returnPeriod, int lookbackMonths) {
    int month = 4;
    int year = 2026;
    if (returnPeriod.length() == 6) {
        month = returnPeriod.left(2).toInt();
        year = returnPeriod.mid(2).toInt();
    }

    QDate fromDate(year, month, 1);
    QDate toDate(year, month, fromDate.daysInMonth());
    return loadBookPurchasesFromDb(fromDate, toDate);
}

Gstr2ReconciliationSummary Gstr2Reconciler::reconcile(
    const QList<Gstr2BookRecord>& bookRecords,
    const QList<Gstr2PortalRecord>& portalRecords,
    double tolerance,
    int dateToleranceDays
) {
    Gstr2ReconciliationSummary summary;
    summary.totalBookRecords = bookRecords.size();
    summary.totalPortalRecords = portalRecords.size();

    QSet<int> matchedPortalIndices;
    QSet<int> matchedBookIndices;

    auto isClose = [](double a, double b, double tol) {
        return std::abs(a - b) <= tol;
    };

    // -------------------------------------------------------------
    // PASS 1: Exact Match (GSTIN + Inv# + Date + Tax)
    // -------------------------------------------------------------
    for (int bIdx = 0; bIdx < bookRecords.size(); ++bIdx) {
        if (matchedBookIndices.contains(bIdx)) continue;
        const auto& book = bookRecords[bIdx];
        if (book.supplierGstin.isEmpty()) continue;
        QString cleanBookInv = normalizeInvoiceNumber(book.invoiceNo);
        QString cleanBookGstin = normalizeGstin(book.supplierGstin);

        for (int pIdx = 0; pIdx < portalRecords.size(); ++pIdx) {
            if (matchedPortalIndices.contains(pIdx)) continue;
            const auto& portal = portalRecords[pIdx];
            QString cleanPortalGstin = normalizeGstin(portal.supplierGstin);
            QString cleanPortalInv = normalizeInvoiceNumber(portal.invoiceNo);

            if (cleanBookGstin == cleanPortalGstin && cleanBookInv == cleanPortalInv) {
                double taxDiff = std::abs(book.taxAmount - portal.taxAmount);
                double taxableDiff = std::abs(book.taxableValue - portal.taxableValue);

                if (taxDiff <= 0.05 && taxableDiff <= 0.05 && book.invoiceDate == portal.invoiceDate) {
                    Gstr2ReconciledItem item;
                    item.status = MatchStatus::ExactMatch;
                    item.statusText = "EXACT MATCH (100%)";
                    item.discrepancyRemarks = "Exact match on GSTIN, Invoice #, Date & Tax values";
                    
                    item.bookVoucherId = book.voucherId;
                    item.bookGstin = book.supplierGstin;
                    item.bookSupplierName = book.supplierName;
                    item.bookInvNo = book.invoiceNo;
                    item.bookInvDate = book.invoiceDate;
                    item.bookTaxable = book.taxableValue;
                    item.bookTax = book.taxAmount;
                    item.bookTotal = book.totalValue;

                    item.portalGstin = portal.supplierGstin;
                    item.portalSupplierName = portal.supplierName;
                    item.portalInvNo = portal.invoiceNo;
                    item.portalInvDate = portal.invoiceDate;
                    item.portalTaxable = portal.taxableValue;
                    item.portalTax = portal.taxAmount;
                    item.portalTotal = portal.totalValue;
                    item.portalSection = portal.section;
                    item.itcEligibility = portal.itcEligibility;

                    matchedPortalIndices.insert(pIdx);
                    matchedBookIndices.insert(bIdx);
                    summary.items.append(item);
                    summary.matchedCount++;
                    summary.exactMatchCount++;
                    summary.matchedItcAmount += book.taxAmount;
                    break;
                }
            }
        }
    }

    // -------------------------------------------------------------
    // PASS 2: Rounding / Tolerance Match (|Δ| <= tolerance)
    // -------------------------------------------------------------
    for (int bIdx = 0; bIdx < bookRecords.size(); ++bIdx) {
        if (matchedBookIndices.contains(bIdx)) continue;
        const auto& book = bookRecords[bIdx];
        if (book.supplierGstin.isEmpty()) continue;
        QString cleanBookInv = normalizeInvoiceNumber(book.invoiceNo);
        QString cleanBookGstin = normalizeGstin(book.supplierGstin);

        for (int pIdx = 0; pIdx < portalRecords.size(); ++pIdx) {
            if (matchedPortalIndices.contains(pIdx)) continue;
            const auto& portal = portalRecords[pIdx];
            QString cleanPortalGstin = normalizeGstin(portal.supplierGstin);
            QString cleanPortalInv = normalizeInvoiceNumber(portal.invoiceNo);

            if (cleanBookGstin == cleanPortalGstin && cleanBookInv == cleanPortalInv) {
                double taxDiff = std::abs(book.taxAmount - portal.taxAmount);
                double taxableDiff = std::abs(book.taxableValue - portal.taxableValue);

                if (taxDiff <= tolerance && taxableDiff <= tolerance) {
                    Gstr2ReconciledItem item;
                    item.status = MatchStatus::RoundingMatch;
                    item.statusText = "MATCHED (Rounding Tolerance)";
                    item.discrepancyRemarks = QString("Rounding difference: Tax Δ=₹%1, Taxable Δ=₹%2")
                        .arg(round2(book.taxAmount - portal.taxAmount))
                        .arg(round2(book.taxableValue - portal.taxableValue));

                    item.bookVoucherId = book.voucherId;
                    item.bookGstin = book.supplierGstin;
                    item.bookSupplierName = book.supplierName;
                    item.bookInvNo = book.invoiceNo;
                    item.bookInvDate = book.invoiceDate;
                    item.bookTaxable = book.taxableValue;
                    item.bookTax = book.taxAmount;
                    item.bookTotal = book.totalValue;

                    item.portalGstin = portal.supplierGstin;
                    item.portalSupplierName = portal.supplierName;
                    item.portalInvNo = portal.invoiceNo;
                    item.portalInvDate = portal.invoiceDate;
                    item.portalTaxable = portal.taxableValue;
                    item.portalTax = portal.taxAmount;
                    item.portalTotal = portal.totalValue;
                    item.portalSection = portal.section;
                    item.itcEligibility = portal.itcEligibility;
                    item.taxableDiff = round2(book.taxableValue - portal.taxableValue);
                    item.taxDiff = round2(book.taxAmount - portal.taxAmount);

                    matchedPortalIndices.insert(pIdx);
                    matchedBookIndices.insert(bIdx);
                    summary.items.append(item);
                    summary.matchedCount++;
                    summary.roundingMatchCount++;
                    summary.matchedItcAmount += book.taxAmount;
                    break;
                }
            }
        }
    }

    // -------------------------------------------------------------
    // PASS 3: Date Window Mismatch (GSTIN + Inv# + Tax match, Dates differ)
    // -------------------------------------------------------------
    for (int bIdx = 0; bIdx < bookRecords.size(); ++bIdx) {
        if (matchedBookIndices.contains(bIdx)) continue;
        const auto& book = bookRecords[bIdx];
        if (book.supplierGstin.isEmpty()) continue;
        QString cleanBookInv = normalizeInvoiceNumber(book.invoiceNo);
        QString cleanBookGstin = normalizeGstin(book.supplierGstin);

        for (int pIdx = 0; pIdx < portalRecords.size(); ++pIdx) {
            if (matchedPortalIndices.contains(pIdx)) continue;
            const auto& portal = portalRecords[pIdx];
            QString cleanPortalGstin = normalizeGstin(portal.supplierGstin);
            QString cleanPortalInv = normalizeInvoiceNumber(portal.invoiceNo);

            if (cleanBookGstin == cleanPortalGstin && cleanBookInv == cleanPortalInv) {
                double taxDiff = std::abs(book.taxAmount - portal.taxAmount);
                if (taxDiff <= tolerance) {
                    Gstr2ReconciledItem item;
                    item.status = MatchStatus::DateWindowMismatch;
                    item.statusText = "DATE MISMATCH";
                    item.discrepancyRemarks = QString("Books Date (%1) vs Portal Date (%2)")
                        .arg(book.invoiceDate.toString("dd-MM-yyyy"))
                        .arg(portal.invoiceDate.toString("dd-MM-yyyy"));

                    item.bookVoucherId = book.voucherId;
                    item.bookGstin = book.supplierGstin;
                    item.bookSupplierName = book.supplierName;
                    item.bookInvNo = book.invoiceNo;
                    item.bookInvDate = book.invoiceDate;
                    item.bookTaxable = book.taxableValue;
                    item.bookTax = book.taxAmount;
                    item.bookTotal = book.totalValue;

                    item.portalGstin = portal.supplierGstin;
                    item.portalSupplierName = portal.supplierName;
                    item.portalInvNo = portal.invoiceNo;
                    item.portalInvDate = portal.invoiceDate;
                    item.portalTaxable = portal.taxableValue;
                    item.portalTax = portal.taxAmount;
                    item.portalTotal = portal.totalValue;
                    item.portalSection = portal.section;
                    item.itcEligibility = portal.itcEligibility;
                    item.taxableDiff = round2(book.taxableValue - portal.taxableValue);
                    item.taxDiff = round2(book.taxAmount - portal.taxAmount);

                    matchedPortalIndices.insert(pIdx);
                    matchedBookIndices.insert(bIdx);
                    summary.items.append(item);
                    summary.dateMismatchCount++;
                    summary.matchedCount++;
                    summary.matchedItcAmount += book.taxAmount;
                    break;
                }
            }
        }
    }

    // -------------------------------------------------------------
    // PASS 4: Tax / Value Mismatch (GSTIN + Inv# Match, Tax differs)
    // -------------------------------------------------------------
    for (int bIdx = 0; bIdx < bookRecords.size(); ++bIdx) {
        if (matchedBookIndices.contains(bIdx)) continue;
        const auto& book = bookRecords[bIdx];
        if (book.supplierGstin.isEmpty()) continue;
        QString cleanBookInv = normalizeInvoiceNumber(book.invoiceNo);
        QString cleanBookGstin = normalizeGstin(book.supplierGstin);

        for (int pIdx = 0; pIdx < portalRecords.size(); ++pIdx) {
            if (matchedPortalIndices.contains(pIdx)) continue;
            const auto& portal = portalRecords[pIdx];
            QString cleanPortalGstin = normalizeGstin(portal.supplierGstin);
            QString cleanPortalInv = normalizeInvoiceNumber(portal.invoiceNo);

            if (cleanBookGstin == cleanPortalGstin && cleanBookInv == cleanPortalInv) {
                Gstr2ReconciledItem item;
                item.status = MatchStatus::ValueMismatch;
                item.statusText = "VALUE MISMATCH";
                item.taxableDiff = round2(book.taxableValue - portal.taxableValue);
                item.taxDiff = round2(book.taxAmount - portal.taxAmount);
                item.discrepancyRemarks = QString("Value Mismatch: Taxable Δ=₹%1, Tax Δ=₹%2").arg(item.taxableDiff).arg(item.taxDiff);

                item.bookVoucherId = book.voucherId;
                item.bookGstin = book.supplierGstin;
                item.bookSupplierName = book.supplierName;
                item.bookInvNo = book.invoiceNo;
                item.bookInvDate = book.invoiceDate;
                item.bookTaxable = book.taxableValue;
                item.bookTax = book.taxAmount;
                item.bookTotal = book.totalValue;

                item.portalGstin = portal.supplierGstin;
                item.portalSupplierName = portal.supplierName;
                item.portalInvNo = portal.invoiceNo;
                item.portalInvDate = portal.invoiceDate;
                item.portalTaxable = portal.taxableValue;
                item.portalTax = portal.taxAmount;
                item.portalTotal = portal.totalValue;
                item.portalSection = portal.section;
                item.itcEligibility = portal.itcEligibility;

                matchedPortalIndices.insert(pIdx);
                matchedBookIndices.insert(bIdx);
                summary.items.append(item);
                summary.valueMismatchCount++;
                summary.mismatchItcAmount += book.taxAmount;
                break;
            }
        }
    }

    // -------------------------------------------------------------
    // PASS 5: Probable Fuzzy Match (GSTIN + Amount match, typo in Inv#)
    // -------------------------------------------------------------
    for (int bIdx = 0; bIdx < bookRecords.size(); ++bIdx) {
        if (matchedBookIndices.contains(bIdx)) continue;
        const auto& book = bookRecords[bIdx];
        if (book.supplierGstin.isEmpty()) continue;
        QString cleanBookGstin = normalizeGstin(book.supplierGstin);

        for (int pIdx = 0; pIdx < portalRecords.size(); ++pIdx) {
            if (matchedPortalIndices.contains(pIdx)) continue;
            const auto& portal = portalRecords[pIdx];
            QString cleanPortalGstin = normalizeGstin(portal.supplierGstin);

            if (cleanBookGstin == cleanPortalGstin && isClose(book.taxAmount, portal.taxAmount, 0.05)) {
                if (book.invoiceDate.isValid() && portal.invoiceDate.isValid() &&
                    std::abs(book.invoiceDate.daysTo(portal.invoiceDate)) <= dateToleranceDays) 
                {
                    Gstr2ReconciledItem item;
                    item.status = MatchStatus::ProbableFuzzyMatch;
                    item.statusText = "PROBABLE MATCH (Typo in Inv#)";
                    item.discrepancyRemarks = QString("Books Inv# '%1' vs Portal Inv# '%2'").arg(book.invoiceNo).arg(portal.invoiceNo);

                    item.bookVoucherId = book.voucherId;
                    item.bookGstin = book.supplierGstin;
                    item.bookSupplierName = book.supplierName;
                    item.bookInvNo = book.invoiceNo;
                    item.bookInvDate = book.invoiceDate;
                    item.bookTaxable = book.taxableValue;
                    item.bookTax = book.taxAmount;
                    item.bookTotal = book.totalValue;

                    item.portalGstin = portal.supplierGstin;
                    item.portalSupplierName = portal.supplierName;
                    item.portalInvNo = portal.invoiceNo;
                    item.portalInvDate = portal.invoiceDate;
                    item.portalTaxable = portal.taxableValue;
                    item.portalTax = portal.taxAmount;
                    item.portalTotal = portal.totalValue;
                    item.portalSection = portal.section;
                    item.itcEligibility = portal.itcEligibility;

                    matchedPortalIndices.insert(pIdx);
                    matchedBookIndices.insert(bIdx);
                    summary.items.append(item);
                    summary.probableMatchCount++;
                    break;
                }
            }
        }
    }

    // -------------------------------------------------------------
    // Unmatched: Missing in Portal (Supplier Default)
    // -------------------------------------------------------------
    for (int bIdx = 0; bIdx < bookRecords.size(); ++bIdx) {
        if (!matchedBookIndices.contains(bIdx)) {
            const auto& book = bookRecords[bIdx];
            // Only flag as NotInPortal if party has a valid registered GSTIN
            if (!book.supplierGstin.isEmpty() && book.supplierGstin.length() == 15) {
                Gstr2ReconciledItem item;
                item.status = MatchStatus::NotInPortal;
                item.statusText = "NOT IN PORTAL (Supplier Default)";
                item.discrepancyRemarks = "Supplier has not uploaded invoice in GSTR-1";

                item.bookVoucherId = book.voucherId;
                item.bookGstin = book.supplierGstin;
                item.bookSupplierName = book.supplierName;
                item.bookInvNo = book.invoiceNo;
                item.bookInvDate = book.invoiceDate;
                item.bookTaxable = book.taxableValue;
                item.bookTax = book.taxAmount;
                item.bookTotal = book.totalValue;
                item.taxDiff = book.taxAmount;
                item.taxableDiff = book.taxableValue;

                summary.items.append(item);
                summary.notInPortalCount++;
                summary.missingPortalItcAmount += book.taxAmount;
            }
        }
    }

    // -------------------------------------------------------------
    // Unmatched: Missing in Books (Unclaimed ITC)
    // -------------------------------------------------------------
    for (int pIdx = 0; pIdx < portalRecords.size(); ++pIdx) {
        if (!matchedPortalIndices.contains(pIdx)) {
            const auto& portal = portalRecords[pIdx];
            Gstr2ReconciledItem item;
            item.status = MatchStatus::NotInBooks;
            item.statusText = "NOT IN BOOKS (Unclaimed ITC)";
            item.discrepancyRemarks = "Invoice present in 2B, but missing in purchase register";

            item.portalGstin = portal.supplierGstin;
            item.portalSupplierName = portal.supplierName;
            item.portalInvNo = portal.invoiceNo;
            item.portalInvDate = portal.invoiceDate;
            item.portalTaxable = portal.taxableValue;
            item.portalTax = portal.taxAmount;
            item.portalTotal = portal.totalValue;
            item.portalSection = portal.section;
            item.itcEligibility = portal.itcEligibility;
            item.taxDiff = -portal.taxAmount;
            item.taxableDiff = -portal.taxableValue;

            summary.items.append(item);
            summary.notInBooksCount++;
            summary.unclaimedPortalItcAmount += portal.taxAmount;
        }
    }

    return summary;
}

bool Gstr2Reconciler::saveReconciliationStatus(int voucherId, MatchStatus status, const QString& remarks) {
    DatabaseManager& db = DatabaseManager::instance();
    QString statusStr = "UNMATCHED";
    if (status == MatchStatus::Matched || status == MatchStatus::ExactMatch || status == MatchStatus::RoundingMatch) {
        statusStr = "MATCHED";
    } else if (status == MatchStatus::ValueMismatch || status == MatchStatus::TaxValueMismatch) {
        statusStr = "MISMATCH";
    }

    QString sql = "UPDATE vouchers SET narration = COALESCE(narration, '') || ' [GST: ' || ? || ']' WHERE id = ?";
    return db.executeNonQuery(sql, { statusStr + " " + remarks, voucherId });
}

} // namespace MahadevERP
