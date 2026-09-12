#include "canara_bank_statement_parser.h"
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

    static QRegularExpression btBlockRegex("BT(.*?)ET", QRegularExpression::DotMatchesEverythingOption);
    auto blockIt = btBlockRegex.globalMatch(streamContent);

    while (blockIt.hasNext()) {
        auto blockMatch = blockIt.next();
        QString block = blockMatch.captured(1);

        QStringList lines = block.split('\n', Qt::SkipEmptyParts);
        for (const QString &rawLine : lines) {
            QString line = rawLine.trimmed();
            if (line.isEmpty()) continue;

            if (line.endsWith("Tm")) {
                QStringList parts = line.split(' ', Qt::SkipEmptyParts);
                if (parts.size() >= 6) {
                    curX = parts[parts.size() - 3].toDouble();
                    curY = parts[parts.size() - 2].toDouble();
                }
            } else if (line.endsWith("Td") || line.endsWith("TD")) {
                QStringList parts = line.split(' ', Qt::SkipEmptyParts);
                if (parts.size() >= 3) {
                    curX += parts[parts.size() - 3].toDouble();
                    curY += parts[parts.size() - 2].toDouble();
                }
            }

            if (line.endsWith("Tj")) {
                int firstParen = line.indexOf('(');
                int lastParen = line.lastIndexOf(')');
                if (firstParen >= 0 && lastParen > firstParen) {
                    QString str = line.mid(firstParen + 1, lastParen - firstParen - 1);
                    str.replace("\\(", "(").replace("\\)", ")").replace("\\\\", "\\");
                    if (!str.trimmed().isEmpty()) {
                        runs.append({curX, curY, str.trimmed()});
                    }
                }
            } else if (line.endsWith("TJ")) {
                int firstBracket = line.indexOf('[');
                int lastBracket = line.lastIndexOf(']');
                if (firstBracket >= 0 && lastBracket > firstBracket) {
                    QString inner = line.mid(firstBracket + 1, lastBracket - firstBracket - 1);
                    QString combined;
                    int p = 0;
                    while (p < inner.length()) {
                        int o = inner.indexOf('(', p);
                        if (o < 0) break;
                        int c = inner.indexOf(')', o);
                        if (c < 0) break;
                        QString sub = inner.mid(o + 1, c - o - 1);
                        sub.replace("\\(", "(").replace("\\)", ")").replace("\\\\", "\\");
                        combined += sub;
                        p = c + 1;
                    }
                    if (!combined.trimmed().isEmpty()) {
                        runs.append({curX, curY, combined.trimmed()});
                    }
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

    // 1. Bank Service Charges
    if (u.contains(" SC") || u.contains("SERVICE CHARGE") || u.contains("MORTGAGE CHARGES") || 
        u.contains("EXPERIAN") || u.contains("CRIF HIGH MARK") || u.contains("SMS ALERT") ||
        u.contains("DD/TT ISS") || u.contains("COMMERCIAL WITH SCORE") || u.contains("RTGS 00.00 TO")) {
        txn.category = "BANK_CHARGES";
        txn.extractedParty = "Bank Charges";
        txn.suggestedDrAccount = "Bank Charges";
        txn.confidence = "HIGH";
        return;
    }

    // 2. Bank Interest / CC Interest
    if (u.contains("QOS PERIOD") || u.contains("INTEREST DEBIT") || u.contains("CC INT") || u.contains("OD INT")) {
        txn.category = "INTEREST_DEBIT";
        txn.extractedParty = "Interest Bank A/c";
        txn.suggestedDrAccount = "Interest Bank A/c";
        txn.confidence = "HIGH";
        return;
    }

    // 3. Cheque Return Charges
    if (u.contains("RTN SC") || u.contains("CHQ RETURN") || u.contains("INSUFFICIENT")) {
        txn.category = "CHEQUE_RETURN_CHARGES";
        txn.extractedParty = "Bank Charges";
        txn.suggestedDrAccount = "Bank Charges";
        txn.confidence = "HIGH";
        return;
    }

    // 4. Cheque Clearing
    if (u.contains("CHQ PAID") || u.contains("MICR INWARD") || u.contains("CLEARING")) {
        txn.category = "CHEQUE_CLEARING";
        txn.confidence = "MEDIUM";
        int dashIdx = narr.lastIndexOf('-');
        if (dashIdx > 0 && dashIdx < narr.length() - 2) {
            txn.extractedParty = narr.mid(dashIdx + 1).trimmed().remove('.');
        }
        return;
    }

    // 5. Internal / Partner Funds Transfer
    if (u.contains("FUNDS TRANSFER")) {
        txn.category = "INTERNAL_TRANSFER";
        static QRegularExpression ftRegex("FUNDS TRANSFER (?:DEBIT|CREDIT)\\s*-\\s*([^/\\-]+)");
        auto ftM = ftRegex.match(narr);
        if (ftM.hasMatch()) {
            txn.extractedParty = ftM.captured(1).trimmed();
        }
        // Normalize known aliases
        if (txn.extractedParty.contains("HARITAGE", Qt::CaseInsensitive)) {
            txn.extractedParty = "Haritage Harvestor Agro Products";
        }
        txn.confidence = "MEDIUM";
        return;
    }

    // 6. RTGS / NEFT / IMPS Customer Receipts
    if (u.contains("RTGS CR") || u.contains("NEFT CR") || u.contains("IMPS-CR") || u.contains("INET-IMPS-CR") || u.contains("CR-")) {
        txn.category = "CUSTOMER_RECEIPT";
        txn.voucherType = "Cheque Receipt";

        // Try extracting party name from: RTGS CR-UTR-IFSC-PARTY NAME--/FAST/FAST
        QStringList parts = narr.split('-', Qt::SkipEmptyParts);
        QString candidate;
        if (parts.size() >= 4) {
            candidate = parts[3].trimmed();
        } else if (parts.size() == 3) {
            candidate = parts[2].trimmed();
        } else if (u.contains("IMPS")) {
            static QRegularExpression impsRegex("IMPS-CR/[^/]+/([^/]+)");
            auto m = impsRegex.match(narr);
            if (m.hasMatch()) candidate = m.captured(1).trimmed();
        }

        // Clean candidate name
        candidate.remove(QRegularExpression("[/\\\\].*"));
        candidate = candidate.trimmed();

        // Check common aliases
        if (candidate.contains("L AND FINEEINC", Qt::CaseInsensitive) || candidate.contains("L & T", Qt::CaseInsensitive)) {
            candidate = "L & T Finance Ltd.";
        } else if (candidate.contains("HARITAGE", Qt::CaseInsensitive)) {
            candidate = "Haritage Harvestor Agro Products";
        }

        txn.extractedParty = candidate;
        txn.confidence = candidate.isEmpty() ? "UNMATCHED" : "HIGH";
        return;
    }

    // 7. RTGS / NEFT / IMPS Supplier / Vendor Payments
    if (u.contains("RTGS DR") || u.contains("NEFT DR") || u.contains("IMPS-DR") || u.contains("IB-IMPS-DR") || u.contains("DR-")) {
        txn.category = "SUPPLIER_PAYMENT";
        txn.voucherType = "Cheque Payment";

        QStringList parts = narr.split('-', Qt::SkipEmptyParts);
        QString candidate;
        if (parts.size() >= 4) {
            candidate = parts[3].trimmed();
        } else if (parts.size() == 3) {
            candidate = parts[2].trimmed();
        } else if (u.contains("IB NEFT DR") || u.contains("NEFT DR")) {
            static QRegularExpression ibRegex("(?:IB )?NEFT DR\\s+[A-Z0-9]+\\s+([A-Za-z0-9 &.,]+?)(?:\\s+[A-Z]{4}\\d{7}|\\s+\\d{10,}|$)");
            auto m = ibRegex.match(narr);
            if (m.hasMatch()) candidate = m.captured(1).trimmed();
        }

        // Clean candidate name
        candidate.remove(QRegularExpression("[/\\\\].*"));
        candidate = candidate.trimmed();

        // Check common aliases
        if (candidate.contains("L AND FINEEINC", Qt::CaseInsensitive) || candidate.contains("L & T", Qt::CaseInsensitive)) {
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
    int globalIndex = 1;

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
                if (r.text.contains("MAHADEV RICE INDUSTRY", Qt::CaseInsensitive)) {
                    header.firmName = "MAHADEV RICE INDUSTRY";
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

    return true;
}

