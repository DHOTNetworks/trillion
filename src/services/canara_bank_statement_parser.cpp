#include "canara_bank_statement_parser.h"
#include "../database_manager.h"
#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>
#include <QDebug>
#include "miniz.h"
#include <algorithm>

CanaraBankStatementParser::CanaraBankStatementParser() {
}

QString CanaraBankStatementParser::decompressStream(const QByteArray &compressedData) {
    if (compressedData.isEmpty()) return QString();

    z_stream strm;
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;
    strm.avail_in = compressedData.size();
    strm.next_in = reinterpret_cast<Bytef*>(const_cast<char*>(compressedData.data()));

    // Window size 15 + 32 enables automatic zlib and gzip header detection
    if (inflateInit2(&strm, 15 + 32) != Z_OK) {
        // Fallback standard inflateInit
        if (inflateInit(&strm) != Z_OK) {
            return QString();
        }
    }

    QByteArray outBuffer;
    char tempBuf[16384];

    int ret = Z_OK;
    while (ret == Z_OK) {
        strm.avail_out = sizeof(tempBuf);
        strm.next_out = reinterpret_cast<Bytef*>(tempBuf);
        ret = inflate(&strm, Z_NO_FLUSH);
        if (ret != Z_OK && ret != Z_STREAM_END && ret != Z_BUF_ERROR) {
            break;
        }
        int have = sizeof(tempBuf) - strm.avail_out;
        if (have > 0) {
            outBuffer.append(tempBuf, have);
        }
    }

    inflateEnd(&strm);

    return QString::fromLatin1(outBuffer);
}

struct TextRun {
    double x = 0.0;
    double y = 0.0;
    QString text;
};

QVector<TextRun> extractRunsFromStream(const QString &streamContent) {
    QVector<TextRun> runs;
    if (streamContent.isEmpty()) return runs;

    double curX = 0.0;
    double curY = 0.0;
    bool inBT = false;

    const int len = streamContent.length();
    int i = 0;
    QStringList operandTokens;

    while (i < len) {
        const QChar ch = streamContent[i];

        if (ch.isSpace()) {
            i++;
            continue;
        }

        if (ch == '%') {
            while (i < len && streamContent[i] != '\r' && streamContent[i] != '\n') {
                i++;
            }
            continue;
        }

        if (ch == '(') {
            int start = i + 1;
            i++;
            int depth = 1;
            while (i < len && depth > 0) {
                if (streamContent[i] == '\\') {
                    i += 2;
                    continue;
                }
                if (streamContent[i] == '(') depth++;
                else if (streamContent[i] == ')') depth--;
                i++;
            }
            int strEnd = (depth == 0) ? (i - 1) : i;
            QString strVal = streamContent.mid(start, strEnd - start);
            strVal.replace("\\(", "(").replace("\\)", ")").replace("\\\\", "\\").replace("\\r", "\r").replace("\\n", "\n");

            // Lookahead for Tj operator
            int p = i;
            while (p < len && streamContent[p].isSpace()) p++;
            if (p + 1 < len && streamContent[p] == 'T' && streamContent[p + 1] == 'j' &&
                (p + 2 >= len || streamContent[p + 2].isSpace() || streamContent[p + 2] == '(' || streamContent[p + 2] == '[' || streamContent[p + 2] == '/')) {
                if (inBT && !strVal.trimmed().isEmpty()) {
                    runs.append({curX, curY, strVal.trimmed()});
                }
                i = p + 2;
                operandTokens.clear();
                continue;
            } else if (p < len && streamContent[p] == '\'' && (p + 1 >= len || streamContent[p + 1].isSpace())) {
                if (inBT && !strVal.trimmed().isEmpty()) {
                    runs.append({curX, curY, strVal.trimmed()});
                }
                i = p + 1;
                operandTokens.clear();
                continue;
            }

            operandTokens.append(strVal);
            continue;
        }

        if (ch == '[') {
            int start = i + 1;
            i++;
            QString combined;
            while (i < len && streamContent[i] != ']') {
                if (streamContent[i] == '(') {
                    int sStart = i + 1;
                    i++;
                    int depth = 1;
                    while (i < len && depth > 0) {
                        if (streamContent[i] == '\\') {
                            i += 2;
                            continue;
                        }
                        if (streamContent[i] == '(') depth++;
                        else if (streamContent[i] == ')') depth--;
                        i++;
                    }
                    int sEnd = (depth == 0) ? i - 1 : i;
                    QString sub = streamContent.mid(sStart, sEnd - sStart);
                    sub.replace("\\(", "(").replace("\\)", ")").replace("\\\\", "\\");
                    combined += sub;
                } else {
                    i++;
                }
            }
            if (i < len && streamContent[i] == ']') i++;

            // Lookahead for TJ operator
            int p = i;
            while (p < len && streamContent[p].isSpace()) p++;
            if (p + 1 < len && streamContent[p] == 'T' && streamContent[p + 1] == 'J' &&
                (p + 2 >= len || streamContent[p + 2].isSpace() || streamContent[p + 2] == '(' || streamContent[p + 2] == '[' || streamContent[p + 2] == '/')) {
                if (inBT && !combined.trimmed().isEmpty()) {
                    runs.append({curX, curY, combined.trimmed()});
                }
                i = p + 2;
                operandTokens.clear();
                continue;
            }

            operandTokens.append(combined);
            continue;
        }

        // Regular word / operator token
        int start = i;
        while (i < len && !streamContent[i].isSpace() && streamContent[i] != '(' && streamContent[i] != '[' && streamContent[i] != '%' && streamContent[i] != '<' && streamContent[i] != '>') {
            i++;
        }
        if (i == start) {
            i++;
            continue;
        }

        QString token = streamContent.mid(start, i - start);

        if (token == "BT") {
            inBT = true;
            operandTokens.clear();
        } else if (token == "ET") {
            inBT = false;
            operandTokens.clear();
        } else if (inBT) {
            if (token == "Tm") {
                if (operandTokens.size() >= 6) {
                    curX = operandTokens[operandTokens.size() - 2].toDouble();
                    curY = operandTokens[operandTokens.size() - 1].toDouble();
                }
                operandTokens.clear();
            } else if (token == "Td" || token == "TD") {
                if (operandTokens.size() >= 2) {
                    curX += operandTokens[operandTokens.size() - 2].toDouble();
                    curY += operandTokens[operandTokens.size() - 1].toDouble();
                }
                operandTokens.clear();
            } else if (token == "Tj") {
                if (!operandTokens.isEmpty()) {
                    QString text = operandTokens.takeLast().trimmed();
                    if (!text.isEmpty()) {
                        runs.append({curX, curY, text});
                    }
                }
                operandTokens.clear();
            } else if (token == "TJ") {
                if (!operandTokens.isEmpty()) {
                    QString text = operandTokens.takeLast().trimmed();
                    if (!text.isEmpty()) {
                        runs.append({curX, curY, text});
                    }
                }
                operandTokens.clear();
            } else {
                operandTokens.append(token);
                if (operandTokens.size() > 8) {
                    operandTokens.removeFirst();
                }
            }
        }
    }

    return runs;
}

QVector<QString> CanaraBankStatementParser::parseTextFromContentStream(const QString &streamContent) {
    QVector<QString> extractedLines;
    QVector<TextRun> runs = extractRunsFromStream(streamContent);
    if (runs.isEmpty()) return extractedLines;

    std::sort(runs.begin(), runs.end(), [](const TextRun &a, const TextRun &b) {
        if (std::abs(a.y - b.y) > 3.0) {
            return a.y > b.y;
        }
        return a.x < b.x;
    });

    QString currentLine;
    double currentY = -9999.0;

    for (const auto &run : runs) {
        if (currentY < -9000.0) {
            currentY = run.y;
            currentLine = run.text;
        } else if (std::abs(run.y - currentY) <= 3.0) {
            if (!currentLine.isEmpty() && !currentLine.endsWith(' ') && !run.text.startsWith(' ')) {
                currentLine += " ";
            }
            currentLine += run.text;
        } else {
            if (!currentLine.trimmed().isEmpty()) {
                extractedLines.append(currentLine.trimmed());
            }
            currentY = run.y;
            currentLine = run.text;
        }
    }

    if (!currentLine.trimmed().isEmpty()) {
        extractedLines.append(currentLine.trimmed());
    }

    return extractedLines;
}

QVector<QString> CanaraBankStatementParser::extractPdfTextLines(const QByteArray &pdfData) {
    QVector<QString> allLines;
    if (pdfData.isEmpty()) return allLines;

    int pos = 0;
    while (pos < pdfData.size()) {
        int streamStart = pdfData.indexOf("stream", pos);
        if (streamStart < 0) break;

        int contentStart = streamStart + 6;
        if (contentStart < pdfData.size() && pdfData[contentStart] == '\r') contentStart++;
        if (contentStart < pdfData.size() && pdfData[contentStart] == '\n') contentStart++;

        int streamEnd = pdfData.indexOf("endstream", contentStart);
        if (streamEnd < 0) break;

        int contentEnd = streamEnd;
        while (contentEnd > contentStart && (pdfData[contentEnd - 1] == '\r' || pdfData[contentEnd - 1] == '\n')) {
            contentEnd--;
        }

        QByteArray compressed = pdfData.mid(contentStart, contentEnd - contentStart);
        QString decompressed = decompressStream(compressed);

        if (!decompressed.isEmpty()) {
            QVector<QString> pageLines = parseTextFromContentStream(decompressed);
            allLines.append(pageLines);
        }

        pos = streamEnd + 9;
    }

    return allLines;
}

static QString cleanCandidateName(const QString &raw) {
    if (raw.trimmed().isEmpty()) return QString();
    QString c = raw.trimmed();

    // 1. Strip entity/honorific prefixes with word boundaries
    static QRegularExpression prefixRegex(R"(^(?:M/S\.?|MS\.?|SHRI\b|SH\.|SH\b|SMT\.|SMT\b|MR\.|MR\b|MRS\.|MRS\b|C/O)\s*)", QRegularExpression::CaseInsensitiveOption);
    c.remove(prefixRegex);
    c = c.trimmed();

    // 2. Strip trailing routing flags (e.g. --/FAST, //URGENT, /NONE, -//BT)
    static QRegularExpression trailingFlagsRegex(R"(\s*[-/\\].*$)");
    c.remove(trailingFlagsRegex);
    c = c.trimmed();

    return c;
}

void CanaraBankStatementParser::classifyAndExtractParty(CanaraBankTransaction &txn) {
    QString narr = txn.rawNarration.trimmed();
    txn.cleanNarration = narr;

    // Extract UTR / Reference ID
    static QRegularExpression utrRegex("([A-Z]{4}[A-Z0-9]{10,22})");
    auto utrMatch = utrRegex.match(narr);
    if (utrMatch.hasMatch()) {
        txn.utrRef = utrMatch.captured(1);
    }

    QString u = narr.toUpper();

    // Default voucher type based on direction
    if (txn.deposit > 0.001) {
        txn.voucherType = "Cheque Receipt";
    } else {
        txn.voucherType = "Cheque Payment";
    }

    // 1. Bank Service Charges, Fees & Audit Fees
    if (u.startsWith("SC ") || u.contains(" SC") || u.contains("SERVICE CHARGE") || u.contains("MORTGAGE CHARGES") || 
        u.contains("TRANSACTION CHARGES") || u.contains("SMS CHARGES") || u.contains("SMS ALERT") ||
        u.contains("FOLIO AMT") || u.contains("PENALTY CHARGES") || u.contains("PENALTY") ||
        u.contains("DOC CHGS") || u.contains("PROC CHGS") || u.contains("PASSHEETCHARGES") || u.contains("PASSHEET") ||
        u.contains("EXPERIAN") || u.contains("CRIF HIGH MARK") || u.contains("DD/TT ISS") || 
        u.contains("COMMERCIAL WITH SCORE") || u.contains("RTGS 00.00 TO") || u.contains("RTN SC") || 
        u.contains("CHQ RETURN") || u.contains("AUDIT FEE") || u.contains("STOCK AUDIT")) {
        txn.category = "BANK_CHARGES";
        txn.extractedParty = "Bank Charges";
        txn.suggestedDrAccount = "Bank Charges";
        txn.confidence = "HIGH";
        return;
    }

    // 2. Bank Interest / CC Interest / OD Interest / Interest Capitalized
    if (u.contains("QOS PERIOD") || u.contains("INTEREST DEBIT") || u.contains("CC INT") || 
        u.contains("OD INT") || u.contains("CASA DEBIT INTEREST") || u.contains("INTEREST CAPITALIZED")) {
        txn.category = "INTEREST_DEBIT";
        txn.extractedParty = "Interest Bank A/c";
        txn.suggestedDrAccount = "Interest Bank A/c";
        txn.confidence = "HIGH";
        return;
    }

    // 3. Internal Account Transfers / Loan Drawdowns / Term Deposits
    if (u.contains("DRAWDOWN FROM CASA") || u.contains("TD PAYIN") || u.contains("CASAXFER")) {
        txn.category = "INTERNAL_TRANSFER";
        txn.extractedParty = "Internal Transfer";
        txn.confidence = "HIGH";
        return;
    }

    // 4. IB IFT / IB ITG (Internal Bank / Partner Funds Transfer)
    if (u.contains("IB IFT") || u.contains("IB ITG")) {
        static QRegularExpression iftRegex(R"(IB IFT\s+\d+\s+\d+\s+([A-Za-z0-9 &.,]+?)(?:\s+ONLINE TRANSACTION|\s+OTH|\s+BILL|\s+PF|\s+TRF|$))", QRegularExpression::CaseInsensitiveOption);
        auto m = iftRegex.match(narr);
        if (m.hasMatch()) {
            txn.extractedParty = cleanCandidateName(m.captured(1));
            txn.category = "SUPPLIER_PAYMENT";
            txn.confidence = txn.extractedParty.isEmpty() ? "UNMATCHED" : "HIGH";
            return;
        }
        static QRegularExpression itgRegex(R"(IB ITG\s+\d+\s+ONLINE TRANSACTION\s+([A-Za-z0-9 &.,\-]+))", QRegularExpression::CaseInsensitiveOption);
        auto m2 = itgRegex.match(narr);
        if (m2.hasMatch()) {
            txn.extractedParty = cleanCandidateName(m2.captured(1));
            txn.category = "SUPPLIER_PAYMENT";
            txn.confidence = txn.extractedParty.isEmpty() ? "UNMATCHED" : "HIGH";
            return;
        }
    }

    // 5. NACH Mandates (e.g. NACH LTFINANCELIMITED...)
    if (u.contains("NACH")) {
        static QRegularExpression nachRegex(R"(NACH\s+([A-Za-z0-9 &.,]+?)(?:\s+[A-Z0-9]{10,}|\s+\d{10,}|$))", QRegularExpression::CaseInsensitiveOption);
        auto m = nachRegex.match(narr);
        if (m.hasMatch()) {
            QString cand = cleanCandidateName(m.captured(1));
            if (cand.contains("LTFINANCE", Qt::CaseInsensitive) || cand.contains("L AND T", Qt::CaseInsensitive) || cand.contains("L & T", Qt::CaseInsensitive)) {
                cand = "L & T Finance Ltd.";
            }
            txn.extractedParty = cand;
            txn.category = "SUPPLIER_PAYMENT";
            txn.confidence = txn.extractedParty.isEmpty() ? "UNMATCHED" : "HIGH";
            return;
        }
    }

    // 6. Partner / Inter-Account Funds Transfer
    if (u.contains("FUNDS TRANSFER")) {
        txn.category = "INTERNAL_TRANSFER";
        static QRegularExpression ftRegex("FUNDS TRANSFER (?:DEBIT|CREDIT)\\s*(?:\\d+)?\\s*-\\s*([^/\\-]+)");
        auto ftM = ftRegex.match(narr);
        if (ftM.hasMatch()) {
            txn.extractedParty = cleanCandidateName(ftM.captured(1));
        }
        // Normalize known aliases
        if (txn.extractedParty.contains("HARITAGE", Qt::CaseInsensitive)) {
            txn.extractedParty = "Haritage Harvestor Agro Products";
        }
        txn.confidence = txn.extractedParty.isEmpty() ? "UNMATCHED" : "HIGH";
        return;
    }

    // 5. Cheque Clearing (BY CLG / TO CLG / MICR)
    if (u.contains("CLG:") || u.contains("CLEARING") || u.contains("MICR INWARD") || u.contains("CHQ PAID") || u.contains("CHEQUE WITHDRAWAL")) {
        txn.category = "CHEQUE_CLEARING";
        txn.confidence = "MEDIUM";
        if (narr.contains(',')) {
            txn.extractedParty = cleanCandidateName(narr.section(',', -1));
        } else if (narr.contains('-')) {
            txn.extractedParty = cleanCandidateName(narr.section('-', -1));
        }
        return;
    }

    // 6. UPI Receipts / Payments
    if (u.contains("UPI/")) {
        QStringList upiParts = narr.split('/', Qt::SkipEmptyParts);
        if (upiParts.size() >= 4) {
            txn.extractedParty = cleanCandidateName(upiParts[3]);
        }
        txn.category = (txn.deposit > 0.001) ? "CUSTOMER_RECEIPT" : "SUPPLIER_PAYMENT";
        txn.confidence = txn.extractedParty.isEmpty() ? "UNMATCHED" : "HIGH";
        return;
    }

    // 7. INET-IMPS Customer / Supplier Transfers
    if (u.contains("INET-IMPS-CR") || u.contains("INET-IMPS-DR")) {
        static QRegularExpression inetRegex("INET-IMPS-(?:CR|DR)/([^/]+)/");
        auto im = inetRegex.match(narr);
        if (im.hasMatch()) {
            QString cand = im.captured(1).trimmed();
            if (cand.startsWith("SUSHIL TRA", Qt::CaseInsensitive)) cand = "Sushil Trading Company";
            else if (cand.startsWith("RAMAYANA I", Qt::CaseInsensitive)) cand = "Ramayana Industries";
            txn.extractedParty = cleanCandidateName(cand);
        }
        txn.category = (txn.deposit > 0.001) ? "CUSTOMER_RECEIPT" : "SUPPLIER_PAYMENT";
        txn.confidence = txn.extractedParty.isEmpty() ? "UNMATCHED" : "HIGH";
        return;
    }

    // 8. RTGS / NEFT / IMPS Customer Receipts
    if (u.contains("RTGS CR") || u.contains("NEFT CR") || u.contains("IMPS-CR") || u.contains("CR-")) {
        txn.category = "CUSTOMER_RECEIPT";
        txn.voucherType = "Cheque Receipt";

        // Try extracting party name from: RTGS CR-UTR-IFSC-PARTY NAME--/FAST/FAST
        QStringList parts = narr.split('-', Qt::SkipEmptyParts);
        QString candidate;
        static QRegularExpression ifscRegex("^[A-Z]{4}[0-9A-Z]{7}$");

        if (parts.size() >= 4) {
            if (ifscRegex.match(parts[1].trimmed()).hasMatch()) {
                candidate = parts[2].trimmed();
            } else if (ifscRegex.match(parts[2].trimmed()).hasMatch()) {
                candidate = parts[3].trimmed();
            } else {
                candidate = parts[3].trimmed();
            }
        } else if (parts.size() == 3) {
            if (ifscRegex.match(parts[1].trimmed()).hasMatch()) {
                candidate = parts[2].trimmed();
            } else {
                candidate = parts[2].trimmed();
            }
        } else if (u.contains("IMPS")) {
            static QRegularExpression impsRegex("IMPS-CR/[^/]+/([^/]+)");
            auto m = impsRegex.match(narr);
            if (m.hasMatch()) candidate = m.captured(1).trimmed();
        }

        candidate = cleanCandidateName(candidate);

        // Check common aliases
        if (candidate.contains("L AND FINEEINC", Qt::CaseInsensitive) || 
            candidate.contains("L & T", Qt::CaseInsensitive) || 
            candidate.contains("L AND T", Qt::CaseInsensitive) || 
            candidate.contains("L ANT T", Qt::CaseInsensitive)) {
            candidate = "L & T Finance Ltd.";
        } else if (candidate.contains("HARITAGE", Qt::CaseInsensitive)) {
            candidate = "Haritage Harvestor Agro Products";
        }

        txn.extractedParty = candidate;
        txn.confidence = candidate.isEmpty() ? "UNMATCHED" : "HIGH";
        return;
    }

    // 9. RTGS / NEFT / IMPS Supplier / Vendor Payments
    if (u.contains("RTGS DR") || u.contains("NEFT DR") || u.contains("IMPS-DR") || u.contains("IB-IMPS-DR") || u.contains("DR-")) {
        txn.category = "SUPPLIER_PAYMENT";
        txn.voucherType = "Cheque Payment";

        QStringList parts = narr.split('-', Qt::SkipEmptyParts);
        QString candidate;
        static QRegularExpression ifscRegex("^[A-Z]{4}[0-9A-Z]{7}$");

        if (parts.size() >= 4) {
            if (ifscRegex.match(parts[1].trimmed()).hasMatch()) {
                candidate = parts[2].trimmed();
            } else if (ifscRegex.match(parts[2].trimmed()).hasMatch()) {
                candidate = parts[3].trimmed();
            } else {
                candidate = parts[3].trimmed();
            }
        } else if (parts.size() == 3) {
            if (ifscRegex.match(parts[1].trimmed()).hasMatch()) {
                candidate = parts[2].trimmed();
            } else {
                candidate = parts[2].trimmed();
            }
        } else if (u.contains("IB NEFT DR") || u.contains("NEFT DR")) {
            static QRegularExpression ibRegex("(?:IB )?NEFT DR\\s+[A-Z0-9]+\\s+([A-Za-z0-9 &.,]+?)(?:\\s+[A-Z]{4}[0-9A-Z]{7}|\\s+\\d{10,}|$)", QRegularExpression::CaseInsensitiveOption);
            auto m = ibRegex.match(narr);
            if (m.hasMatch()) candidate = m.captured(1).trimmed();
        }

        candidate = cleanCandidateName(candidate);

        // Check common aliases
        if (candidate.contains("L AND FINEEINC", Qt::CaseInsensitive) || 
            candidate.contains("L & T", Qt::CaseInsensitive) || 
            candidate.contains("L AND T", Qt::CaseInsensitive) || 
            candidate.contains("L ANT T", Qt::CaseInsensitive)) {
            candidate = "L & T Finance Ltd.";
        } else if (candidate.contains("HARITAGE", Qt::CaseInsensitive)) {
            candidate = "Haritage Harvestor Agro Products";
        }

        txn.extractedParty = candidate;
        txn.confidence = candidate.isEmpty() ? "UNMATCHED" : "HIGH";
        return;
    }

    txn.category = "OTHER";
    txn.confidence = "UNMATCHED";
}

bool CanaraBankStatementParser::parsePdf(const QString &pdfPath, 
                                        CanaraBankStatementHeader &header, 
                                        QVector<CanaraBankTransaction> &transactions, 
                                        QString &errorMessage) {
    QFile file(pdfPath);
    if (!file.open(QIODevice::ReadOnly)) {
        errorMessage = QString("Cannot open PDF file: %1").arg(pdfPath);
        return false;
    }

    QByteArray data = file.readAll();
    file.close();

    return parsePdfData(data, header, transactions, errorMessage);
}

bool CanaraBankStatementParser::parsePdfData(const QByteArray &pdfData, 
                                            CanaraBankStatementHeader &header, 
                                            QVector<CanaraBankTransaction> &transactions, 
                                            QString &errorMessage) {
    transactions.clear();

    if (pdfData.isEmpty()) {
        errorMessage = "PDF data is empty.";
        return false;
    }

    static QRegularExpression datePattern("^\\d{2}-\\d{2}-\\d{4}$");
    static QRegularExpression partialDatePattern("^\\d{2}-\\d{2}-$");
    static QRegularExpression yearPattern("^\\d{4}$");

    int globalIndex = 1;
    QString pendingDatePrefix;
    QString pendingBalSuffix;

    // Search for all streams
    int pos = 0;
    while (pos < pdfData.size()) {
        int streamStart = pdfData.indexOf("stream", pos);
        if (streamStart < 0) break;

        int contentStart = streamStart + 6;
        if (contentStart < pdfData.size() && pdfData[contentStart] == '\r') contentStart++;
        if (contentStart < pdfData.size() && pdfData[contentStart] == '\n') contentStart++;

        int streamEnd = pdfData.indexOf("endstream", contentStart);
        if (streamEnd < 0) break;

        int contentEnd = streamEnd;
        while (contentEnd > contentStart && (pdfData[contentEnd - 1] == '\r' || pdfData[contentEnd - 1] == '\n')) {
            contentEnd--;
        }

        QByteArray compressed = pdfData.mid(contentStart, contentEnd - contentStart);
        QString decompressed = decompressStream(compressed);

        if (!decompressed.isEmpty()) {
            QVector<TextRun> runs = extractRunsFromStream(decompressed);

            // Extract Header Metadata from stream text
            for (const auto &r : runs) {
                if (r.text.contains("Statement for A/c", Qt::CaseInsensitive)) {
                    static QRegularExpression acRegex("Statement for A/c\\s*(\\w+)", QRegularExpression::CaseInsensitiveOption);
                    auto m = acRegex.match(r.text);
                    if (m.hasMatch()) header.accountNo = m.captured(1).trimmed();

                    static QRegularExpression dateRangeRegex("Between\\s*(\\d{2}-[A-Za-z]{3}-\\d{4})\\s*and\\s*(\\d{2}-[A-Za-z]{3}-\\d{4})", QRegularExpression::CaseInsensitiveOption);
                    auto dm = dateRangeRegex.match(r.text);
                    if (dm.hasMatch()) {
                        header.fromDate = dm.captured(1);
                        header.toDate = dm.captured(2);
                    }
                }
                if (r.text.contains("CNRB", Qt::CaseInsensitive) && header.ifscCode.isEmpty()) {
                    static QRegularExpression ifscRegex("(CNRB\\d{7})");
                    auto im = ifscRegex.match(r.text);
                    if (im.hasMatch()) header.ifscCode = im.captured(1);
                }
            }

            // Sort page runs by Y descending, then X ascending
            std::sort(runs.begin(), runs.end(), [](const TextRun &a, const TextRun &b) {
                if (std::abs(a.y - b.y) > 3.0) {
                    return a.y > b.y;
                }
                return a.x < b.x;
            });

            // Find year context on this page or from header
            QString pageYear = "2025";
            for (const auto &r : runs) {
                auto m = datePattern.match(r.text);
                if (m.hasMatch()) {
                    pageYear = r.text.right(4);
                    break;
                }
            }

            // If a run has partial date like "29-12-", complete it
            for (auto &r : runs) {
                if (r.x < 60.0 && partialDatePattern.match(r.text).hasMatch()) {
                    r.text = r.text + pageYear;
                }
            }

            // Find all date runs (X < 60 and DD-MM-YYYY)
            QVector<TextRun> dateRuns;
            for (const auto &r : runs) {
                if (r.x < 60.0 && datePattern.match(r.text).hasMatch()) {
                    dateRuns.append(r);
                }
            }

            // Group transaction runs per date row
            for (int i = 0; i < dateRuns.size(); ++i) {
                double curY = dateRuns[i].y;
                double nextY = (i + 1 < dateRuns.size()) ? dateRuns[i + 1].y : -999999.0;

                QVector<TextRun> txnRuns;
                for (const auto &r : runs) {
                    if (r.y > nextY && r.y <= curY + 4.0) {
                        txnRuns.append(r);
                    }
                }

                // Extract narration runs (60 <= X < 350)
                QVector<TextRun> narrRuns;
                for (const auto &r : txnRuns) {
                    if (r.x >= 60.0 && r.x < 350.0) {
                        narrRuns.append(r);
                    }
                }
                std::sort(narrRuns.begin(), narrRuns.end(), [](const TextRun &a, const TextRun &b) {
                    if (std::abs(a.y - b.y) > 3.0) return a.y > b.y;
                    return a.x < b.x;
                });

                QStringList narrParts;
                for (const auto &nr : narrRuns) {
                    if (!nr.text.isEmpty()) narrParts.append(nr.text);
                }
                QString narration = narrParts.join(" ");

                // Withdrawal (350 <= X < 420)
                double withdrawal = 0.0;
                for (const auto &r : txnRuns) {
                    if (r.x >= 350.0 && r.x < 420.0) {
                        QString cleanAmt = r.text;
                        cleanAmt.remove(',');
                        bool okVal = false;
                        double val = cleanAmt.toDouble(&okVal);
                        if (okVal) { withdrawal = val; break; }
                    }
                }

                // Deposit (420 <= X < 490)
                double deposit = 0.0;
                for (const auto &r : txnRuns) {
                    if (r.x >= 420.0 && r.x < 490.0) {
                        QString cleanAmt = r.text;
                        cleanAmt.remove(',');
                        bool okVal = false;
                        double val = cleanAmt.toDouble(&okVal);
                        if (okVal) { deposit = val; break; }
                    }
                }

                // Balance (X >= 490)
                double balance = 0.0;
                for (const auto &r : txnRuns) {
                    if (r.x >= 490.0) {
                        QString cleanAmt = r.text;
                        cleanAmt.remove(',');
                        bool okVal = false;
                        double val = cleanAmt.toDouble(&okVal);
                        if (okVal) { balance = val; break; }
                    }
                }

                // Reversals / Negative Amount Normalization
                if (withdrawal < 0.0) {
                    deposit = std::abs(withdrawal);
                    withdrawal = 0.0;
                }
                if (deposit < 0.0) {
                    deposit = std::abs(deposit);
                }

                // Ignore zero-amount transactions
                if (std::abs(withdrawal) < 0.005 && std::abs(deposit) < 0.005) {
                    continue;
                }

                CanaraBankTransaction txn;
                txn.index = globalIndex++;
                txn.dateStr = dateRuns[i].text;

                QDate d = QDate::fromString(txn.dateStr, "dd-MM-yyyy");
                if (d.isValid()) {
                    txn.isoDate = d.toString("yyyy-MM-dd");
                } else {
                    txn.isoDate = txn.dateStr;
                }

                txn.rawNarration = narration;
                txn.withdrawal = withdrawal;
                txn.deposit = deposit;
                txn.balance = balance;

                classifyAndExtractParty(txn);
                transactions.append(txn);
            }
        }

        pos = streamEnd + 9;
    }

    if (transactions.isEmpty()) {
        errorMessage = "No transaction records could be extracted from the PDF statement.";
        return false;
    }

    if (header.firmName.isEmpty()) {
        QVariant dbFirm = DatabaseManager::instance().executeScalar("SELECT company_name FROM company_info LIMIT 1;");
        if (dbFirm.isValid() && !dbFirm.toString().trimmed().isEmpty()) {
            QString name = dbFirm.toString().trimmed();
            name.remove(QRegularExpression(R"(^(?:M/S\.?|MS\.?)\s*)", QRegularExpression::CaseInsensitiveOption));
            header.firmName = name.trimmed();
        }
    }

    return true;
}

