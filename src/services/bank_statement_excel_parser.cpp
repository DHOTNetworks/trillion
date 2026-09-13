#include "bank_statement_excel_parser.h"
#include <QFile>
#include <QFileInfo>
#include <QTextStream>
#include <QRegularExpression>
#include <QXmlStreamReader>
#include <QDebug>
#include <cmath>

#include <xls.h>
#include <miniz.h>

QString BankStatementExcelParser::parseDateToIso(const QString& rawDate) {
    QString s = rawDate.trimmed();
    if (s.isEmpty()) return "";

    static const QMap<QString, QString> monthMap = {
        {"JAN", "01"}, {"FEB", "02"}, {"MAR", "03"}, {"APR", "04"},
        {"MAY", "05"}, {"JUN", "06"}, {"JUL", "07"}, {"AUG", "08"},
        {"SEP", "09"}, {"OCT", "10"}, {"NOV", "11"}, {"DEC", "12"}
    };

    // Format: 01-APR-2025 or 01-Apr-2025
    QStringList parts = s.split('-');
    if (parts.size() == 3) {
        if (parts[0].length() == 4) {
            // YYYY-MM-DD
            return s;
        }
        int day = parts[0].toInt();
        QString monStr = parts[1].toUpper().left(3);
        QString mon = monthMap.value(monStr, "");
        if (mon.isEmpty()) {
            mon = QString("%1").arg(parts[1].toInt(), 2, 10, QChar('0'));
        }
        int yr = parts[2].toInt();
        if (yr < 100) yr += 2000;
        return QString("%1-%2-%3").arg(yr, 4, 10, QChar('0')).arg(mon).arg(day, 2, 10, QChar('0'));
    }

    // Format: 01/04/2025
    parts = s.split('/');
    if (parts.size() == 3) {
        int day = parts[0].toInt();
        int mon = parts[1].toInt();
        int yr = parts[2].toInt();
        if (yr < 100) yr += 2000;
        return QString("%1-%2-%3").arg(yr, 4, 10, QChar('0')).arg(mon, 2, 10, QChar('0')).arg(day, 2, 10, QChar('0'));
    }

    // Format: 01.04.2025
    parts = s.split('.');
    if (parts.size() == 3) {
        int day = parts[0].toInt();
        int mon = parts[1].toInt();
        int yr = parts[2].toInt();
        if (yr < 100) yr += 2000;
        return QString("%1-%2-%3").arg(yr, 4, 10, QChar('0')).arg(mon, 2, 10, QChar('0')).arg(day, 2, 10, QChar('0'));
    }

    return s;
}

double BankStatementExcelParser::parseAmount(const QString& rawAmount) {
    QString s = rawAmount.trimmed();
    if (s.isEmpty()) return 0.0;

    bool negative = false;
    if (s.startsWith('-') || (s.startsWith('(') && s.endsWith(')'))) {
        negative = true;
    }

    QString clean;
    for (QChar ch : s) {
        if (ch.isDigit() || ch == '.') {
            clean += ch;
        }
    }

    if (clean.isEmpty()) return 0.0;
    double val = clean.toDouble();
    return negative ? -val : val;
}

static void processMatrix(const QVector<QVector<QString>>& matrix,
                          BankStatementMetadata& outMeta,
                          QVector<BankStatementTransaction>& outTxns) {
    if (matrix.isEmpty()) return;

    // 1. Scan metadata from top rows (e.g. Canara Bank, SBI, Axis)
    int headerRowIdx = -1;
    for (int r = 0; r < std::min(25, static_cast<int>(matrix.size())); ++r) {
        const auto& row = matrix[r];
        for (int c = 0; c < row.size(); ++c) {
            QString cell = row[c].trimmed();
            if (cell.contains("Canara Bank", Qt::CaseInsensitive)) outMeta.bankName = "Canara Bank";
            else if (cell.contains("Axis Bank", Qt::CaseInsensitive)) outMeta.bankName = "Axis Bank";
            else if (cell.contains("State Bank of India", Qt::CaseInsensitive) || cell.contains("SBI", Qt::CaseInsensitive)) outMeta.bankName = "State Bank of India";
            else if (cell.contains("HDFC", Qt::CaseInsensitive)) outMeta.bankName = "HDFC Bank";
            else if (cell.contains("ICICI", Qt::CaseInsensitive)) outMeta.bankName = "ICICI Bank";
            else if (cell.contains("YES BANK", Qt::CaseInsensitive)) outMeta.bankName = "YES Bank";
            else if (cell.contains("Punjab National Bank", Qt::CaseInsensitive) || cell.contains("PNB", Qt::CaseInsensitive)) outMeta.bankName = "Punjab National Bank";

            if (cell.contains("Account Number", Qt::CaseInsensitive) && c + 1 < row.size() && !row[c + 1].trimmed().isEmpty()) {
                outMeta.accountNumber = row[c + 1].trimmed();
            } else if (cell.contains("Customer ID", Qt::CaseInsensitive) && c + 1 < row.size()) {
                outMeta.customerId = row[c + 1].trimmed();
            } else if (cell.contains("IFSC Code", Qt::CaseInsensitive) && c + 1 < row.size()) {
                outMeta.ifscCode = row[c + 1].trimmed();
            } else if (cell.contains("Branch Name", Qt::CaseInsensitive) && c + 1 < row.size()) {
                outMeta.branchName = row[c + 1].trimmed();
            } else if (cell.contains("Name", Qt::CaseInsensitive) && !cell.contains("Branch", Qt::CaseInsensitive) && c + 1 < row.size() && outMeta.accountName.isEmpty()) {
                outMeta.accountName = row[c + 1].trimmed();
            } else if (cell.contains("Address", Qt::CaseInsensitive) && c + 1 < row.size() && outMeta.address.isEmpty()) {
                outMeta.address = row[c + 1].trimmed();
            } else if (cell.contains("Statement for Account from", Qt::CaseInsensitive)) {
                // e.g. Statement for Account from  01-Apr-2025 to 31-Mar-2026
                QRegularExpression rePeriod(R"(from\s+([0-9A-Za-z\-]+)\s+to\s+([0-9A-Za-z\-]+))", QRegularExpression::CaseInsensitiveOption);
                auto m = rePeriod.match(cell);
                if (m.hasMatch()) {
                    outMeta.periodFrom = BankStatementExcelParser::parseDateToIso(m.captured(1));
                    outMeta.periodTo = BankStatementExcelParser::parseDateToIso(m.captured(2));
                }
            }
        }

        // Check if this row is the column header row
        QString rowText = row.join(" ").toLower();
        if ((rowText.contains("date") || rowText.contains("txn date") || rowText.contains("value date")) &&
            (rowText.contains("balance") || rowText.contains("withdraw") || rowText.contains("debit") || rowText.contains("deposit") || rowText.contains("credit"))) {
            headerRowIdx = r;
            break;
        }
    }

    if (headerRowIdx < 0) {
        headerRowIdx = 0; // Fallback
    }

    // 2. Identify Column Mapping
    int dateCol = -1;
    int idCol = -1;
    int debitCol = -1;
    int creditCol = -1;
    int balCol = -1;
    int remarksCol = -1;

    const auto& hRow = matrix[headerRowIdx];
    for (int c = 0; c < hRow.size(); ++c) {
        QString h = hRow[c].trimmed().toLower();
        if (h.contains("date") && dateCol == -1) dateCol = c;
        else if ((h.contains("trasnaction id") || h.contains("transaction id") || h.contains("txn id") || h.contains("chq") || h.contains("cheque") || h.contains("ref")) && idCol == -1) idCol = c;
        else if ((h.contains("withdrawal") || h.contains("debit") || h.contains("dr")) && debitCol == -1) debitCol = c;
        else if ((h.contains("deposit") || h.contains("credit") || h.contains("cr")) && creditCol == -1) creditCol = c;
        else if (h.contains("balance") && balCol == -1) balCol = c;
        else if ((h.contains("remark") || h.contains("narration") || h.contains("desc") || h.contains("particular")) && remarksCol == -1) remarksCol = c;
    }

    // Defaults if standard Canara layout (Date=0, TxnId=1, Withdraw=2, Deposit=3, Balance=4, Remarks=5)
    if (dateCol == -1) dateCol = 0;
    if (idCol == -1) idCol = 1;
    if (debitCol == -1) debitCol = 2;
    if (creditCol == -1) creditCol = 3;
    if (balCol == -1) balCol = 4;
    if (remarksCol == -1) remarksCol = 5;

    // 3. Process Transaction Rows
    double totalW = 0.0;
    double totalD = 0.0;

    for (int r = headerRowIdx + 1; r < matrix.size(); ++r) {
        const auto& row = matrix[r];
        if (row.isEmpty()) continue;

        QString c0 = (dateCol < row.size()) ? row[dateCol].trimmed() : "";
        QString c1 = (idCol < row.size()) ? row[idCol].trimmed() : "";

        // Check for Opening / Closing balance markers
        if (c1.contains("Opening Balance", Qt::CaseInsensitive) || c0.contains("Opening Balance", Qt::CaseInsensitive)) {
            if (balCol < row.size()) outMeta.openingBalance = BankStatementExcelParser::parseAmount(row[balCol]);
            continue;
        }
        if (c1.contains("Closing Balance", Qt::CaseInsensitive) || c0.contains("Closing Balance", Qt::CaseInsensitive)) {
            if (balCol < row.size()) outMeta.closingBalance = BankStatementExcelParser::parseAmount(row[balCol]);
            continue;
        }

        if (c0.isEmpty()) continue;

        QString isoDate = BankStatementExcelParser::parseDateToIso(c0);
        // Verify valid date format
        if (isoDate.length() < 8) continue;

        BankStatementTransaction tx;
        tx.rawDate = c0;
        tx.date = isoDate;
        tx.txnId = c1;

        if (debitCol < row.size()) tx.withdrawal = BankStatementExcelParser::parseAmount(row[debitCol]);
        if (creditCol < row.size()) tx.deposit = BankStatementExcelParser::parseAmount(row[creditCol]);
        if (balCol < row.size()) tx.balance = BankStatementExcelParser::parseAmount(row[balCol]);
        if (remarksCol < row.size()) tx.remarks = row[remarksCol].trimmed();

        // Reversals / Negative Amount Normalization
        if (tx.withdrawal < 0.0) {
            tx.deposit = std::abs(tx.withdrawal);
            tx.withdrawal = 0.0;
        }
        if (tx.deposit < 0.0) {
            tx.deposit = std::abs(tx.deposit);
        }

        // Determine initial voucher type
        if (tx.withdrawal > 0.0 && tx.deposit == 0.0) {
            tx.voucherType = "Payment";
        } else if (tx.deposit > 0.0 && tx.withdrawal == 0.0) {
            tx.voucherType = "Receipt";
        } else {
            tx.voucherType = "Contra";
        }

        totalW += tx.withdrawal;
        totalD += tx.deposit;
        outTxns.append(tx);
    }

    outMeta.totalWithdrawals = totalW;
    outMeta.totalDeposits = totalD;
    outMeta.totalTransactions = outTxns.size();

    if (outMeta.bankName.isEmpty()) outMeta.bankName = "Bank Statement";
}

bool BankStatementExcelParser::parseFile(const QString& filePath,
                                         BankStatementMetadata& outMeta,
                                         QVector<BankStatementTransaction>& outTxns,
                                         QString& outError) {
    QFileInfo fi(filePath);
    if (!fi.exists()) {
        outError = "File does not exist: " + filePath;
        return false;
    }

    QString ext = fi.suffix().toLower();
    if (ext == "xls") {
        return parseXls(filePath, outMeta, outTxns, outError);
    } else if (ext == "xlsx") {
        return parseXlsx(filePath, outMeta, outTxns, outError);
    } else if (ext == "csv" || ext == "txt" || ext == "tsv") {
        return parseCsv(filePath, outMeta, outTxns, outError);
    }

    outError = "Unsupported file format: ." + ext + ". Please select an .xls, .xlsx, or .csv file.";
    return false;
}

bool BankStatementExcelParser::parseXls(const QString& filePath,
                                        BankStatementMetadata& outMeta,
                                        QVector<BankStatementTransaction>& outTxns,
                                        QString& outError) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        outError = "Failed to open XLS file: " + file.errorString();
        return false;
    }
    QByteArray fileBytes = file.readAll();
    file.close();

    if (fileBytes.isEmpty()) {
        outError = "Excel XLS file is empty.";
        return false;
    }

    xls::xls_error_t err = xls::LIBXLS_OK;
    xls::xlsWorkBook* wb = xls::xls_open_buffer(
        reinterpret_cast<const unsigned char*>(fileBytes.constData()),
        static_cast<size_t>(fileBytes.size()),
        "UTF-8",
        &err
    );
    if (!wb) {
        outError = QString("Failed to open Excel XLS file. Error code: %1").arg(err);
        return false;
    }

    if (wb->sheets.count == 0) {
        xls::xls_close_WB(wb);
        outError = "Excel workbook contains no sheets.";
        return false;
    }

    xls::xlsWorkSheet* ws = xls::xls_getWorkSheet(wb, 0);
    if (!ws) {
        xls::xls_close_WB(wb);
        outError = "Unable to read first worksheet in XLS file.";
        return false;
    }

    xls::xls_parseWorkSheet(ws);

    QVector<QVector<QString>> matrix;
    matrix.reserve(ws->rows.lastrow + 1);

    for (xls::WORD r = 0; r <= ws->rows.lastrow; ++r) {
        xls::xlsRow* row = xls::xls_row(ws, r);
        if (!row) continue;

        QVector<QString> rowVec;
        rowVec.reserve(ws->rows.lastcol + 1);
        for (xls::WORD c = 0; c <= ws->rows.lastcol; ++c) {
            auto& cell = row->cells.cell[c];
            if (cell.str) {
                rowVec.append(QString::fromUtf8((char*)cell.str));
            } else if (std::abs(cell.d) > 1e-9) {
                rowVec.append(QString::number(cell.d, 'f', 2));
            } else if (cell.l != 0) {
                rowVec.append(QString::number(cell.l));
            } else {
                rowVec.append("");
            }
        }
        matrix.append(rowVec);
    }

    xls::xls_close_WS(ws);
    xls::xls_close_WB(wb);

    processMatrix(matrix, outMeta, outTxns);
    return true;
}

bool BankStatementExcelParser::parseCsv(const QString& filePath,
                                        BankStatementMetadata& outMeta,
                                        QVector<BankStatementTransaction>& outTxns,
                                        QString& outError) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        outError = "Failed to open CSV file: " + file.errorString();
        return false;
    }

    QTextStream in(&file);
    in.setEncoding(QStringConverter::Utf8);
    QVector<QVector<QString>> matrix;

    while (!in.atEnd()) {
        QString line = in.readLine();
        if (line.trimmed().isEmpty()) continue;

        QVector<QString> row;
        QString currentField;
        bool inQuotes = false;

        for (int i = 0; i < line.length(); ++i) {
            QChar c = line[i];
            if (c == '\"') {
                if (inQuotes && i + 1 < line.length() && line[i + 1] == '\"') {
                    currentField += '\"';
                    i++; // skip escaped quote
                } else {
                    inQuotes = !inQuotes;
                }
            } else if ((c == ',' || c == '\t') && !inQuotes) {
                row.append(currentField.trimmed());
                currentField.clear();
            } else {
                currentField += c;
            }
        }
        row.append(currentField.trimmed());
        matrix.append(row);
    }

    file.close();
    processMatrix(matrix, outMeta, outTxns);
    return true;
}

bool BankStatementExcelParser::parseXlsx(const QString& filePath,
                                         BankStatementMetadata& outMeta,
                                         QVector<BankStatementTransaction>& outTxns,
                                         QString& outError) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        outError = "Failed to open XLSX file: " + file.errorString();
        return false;
    }
    QByteArray fileBytes = file.readAll();
    file.close();

    if (fileBytes.isEmpty()) {
        outError = "XLSX file is empty.";
        return false;
    }

    mz_zip_archive zipArchive;
    memset(&zipArchive, 0, sizeof(zipArchive));

    if (!mz_zip_reader_init_mem(&zipArchive, fileBytes.constData(), static_cast<size_t>(fileBytes.size()), 0)) {
        outError = "Failed to open XLSX zip package.";
        return false;
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

    // 2. Read xl/worksheets/sheet1.xml
    int sheetIdx = mz_zip_reader_locate_file(&zipArchive, "xl/worksheets/sheet1.xml", nullptr, 0);
    if (sheetIdx < 0) {
        // Try case-insensitive lookup
        for (mz_uint i = 0; i < mz_zip_reader_get_num_files(&zipArchive); ++i) {
            mz_zip_archive_file_stat fileStat;
            if (mz_zip_reader_file_stat(&zipArchive, i, &fileStat)) {
                QString fn = QString::fromUtf8(fileStat.m_filename).toLower();
                if (fn.contains("sheet1.xml")) {
                    sheetIdx = i;
                    break;
                }
            }
        }
    }

    if (sheetIdx < 0) {
        mz_zip_reader_end(&zipArchive);
        outError = "No sheet1.xml found in XLSX file.";
        return false;
    }

    size_t sheetSize = 0;
    void* sheetData = mz_zip_reader_extract_to_heap(&zipArchive, sheetIdx, &sheetSize, 0);
    mz_zip_reader_end(&zipArchive);

    if (!sheetData) {
        outError = "Failed to extract sheet1.xml from XLSX.";
        return false;
    }

    QByteArray sheetXmlBytes(static_cast<const char*>(sheetData), static_cast<int>(sheetSize));
    mz_free(sheetData);

    QXmlStreamReader xml(sheetXmlBytes);
    QVector<QVector<QString>> matrix;
    QVector<QString> currentRow;
    QString currentCellType;
    QString currentCellValue;

    while (!xml.atEnd()) {
        xml.readNext();
        if (xml.isStartElement()) {
            if (xml.name() == QLatin1String("row")) {
                currentRow.clear();
            } else if (xml.name() == QLatin1String("c")) {
                currentCellType = xml.attributes().value("t").toString();
                currentCellValue.clear();
            } else if (xml.name() == QLatin1String("v") || xml.name() == QLatin1String("t")) {
                currentCellValue = xml.readElementText();
                if (currentCellType == "s") {
                    int sstId = currentCellValue.toInt();
                    if (sstId >= 0 && sstId < sharedStrings.size()) {
                        currentCellValue = sharedStrings[sstId];
                    }
                }
                currentRow.append(currentCellValue);
            }
        } else if (xml.isEndElement()) {
            if (xml.name() == QLatin1String("row")) {
                matrix.append(currentRow);
            }
        }
    }

    processMatrix(matrix, outMeta, outTxns);
    return true;
}
