#include "migration_utils.h"
#include <QRegularExpression>
#include <cmath>

namespace MahadevERP {

QString MigrationUtils::cleanText(const QString& s) {
    QString res = s.trimmed();
    res.remove('\0');
    return res;
}

QString MigrationUtils::parseNormalizedDate(const QString& rawDate) {
    QString dt = rawDate.trimmed();
    if (dt.isEmpty()) return QDate::currentDate().toString("yyyy-MM-dd");

    if (dt.contains("T")) {
        dt = dt.split("T").first().trimmed();
    } else if (dt.contains(" ")) {
        dt = dt.split(" ").first().trimmed();
    }

    dt.replace(".", "-");
    dt.replace("/", "-");

    QStringList pts = dt.split("-");
    if (pts.size() == 3) {
        // YYYY-MM-DD
        if (pts[0].length() == 4) {
            int y = pts[0].toInt();
            int m = pts[1].toInt();
            int d = pts[2].toInt();
            QDate qd(y, m, d);
            if (qd.isValid()) return qd.toString("yyyy-MM-dd");
        }
        // DD-MM-YYYY
        else if (pts[2].length() == 4) {
            int d = pts[0].toInt();
            int m = pts[1].toInt();
            int y = pts[2].toInt();
            QDate qd(y, m, d);
            if (qd.isValid()) return qd.toString("yyyy-MM-dd");
        }
    }

    // YYYYMMDD
    if (dt.length() == 8 && dt.toInt() > 0) {
        int y = dt.left(4).toInt();
        int m = dt.mid(4, 2).toInt();
        int d = dt.right(2).toInt();
        QDate qd(y, m, d);
        if (qd.isValid()) return qd.toString("yyyy-MM-dd");
    }

    QDate parsed = QDate::fromString(dt, "yyyy-MM-dd");
    if (parsed.isValid()) return parsed.toString("yyyy-MM-dd");

    parsed = QDate::fromString(dt, "dd-MM-yyyy");
    if (parsed.isValid()) return parsed.toString("yyyy-MM-dd");

    return dt;
}

QString MigrationUtils::resolveFinancialYear(const QString& isoDate) {
    QDate d = QDate::fromString(isoDate, "yyyy-MM-dd");
    if (!d.isValid()) return "FY 2025-26";
    int y = d.year();
    if (d.month() >= 4) {
        return QString("FY %1-%2").arg(y).arg(QString::number((y + 1) % 100).rightJustified(2, '0'));
    } else {
        return QString("FY %1-%2").arg(y - 1).arg(QString::number(y % 100).rightJustified(2, '0'));
    }
}

QString MigrationUtils::resolveFirmTypeFromPan(const QString& rawType, const QString& pan, const QString& companyName) {
    QString cleanPan = pan.trimmed().toUpper();
    if (cleanPan.length() == 15) {
        cleanPan = cleanPan.mid(2, 10);
    }
    if (cleanPan.length() >= 4) {
        QChar c = cleanPan.at(3);
        if (c == 'P') return "Proprietorship Firm";
        if (c == 'F') return "Partnership Firm";
        if (c == 'C') return "Private Limited Company";
        if (c == 'H') return "Hindu Undivided Family (HUF)";
        if (c == 'T') return "Trust";
        if (c == 'A' || c == 'B') return "Association of Persons (AOP/BOI)";
        if (c == 'G') return "Government Agency";
        if (c == 'J') return "Artificial Juridical Person";
        if (c == 'L') return "Local Authority";
    }

    QString lowerName = companyName.trimmed().toLower();
    if (lowerName.contains("pvt ltd") || lowerName.contains("private limited")) return "Private Limited Company";
    if (lowerName.contains("llp")) return "Limited Liability Partnership (LLP)";
    if (lowerName.contains("ltd") || lowerName.contains("limited")) return "Public Limited Company";

    QString lowerType = rawType.trimmed().toLower();
    if (lowerType.contains("prop") || lowerType.contains("proprietor")) return "Proprietorship Firm";
    if (lowerType.contains("partner")) return "Partnership Firm";
    if (lowerType.contains("company") || lowerType.contains("corporate")) return "Private Limited Company";
    if (lowerType.contains("huf")) return "Hindu Undivided Family (HUF)";
    if (lowerType.contains("trust")) return "Trust";

    return "Partnership Firm";
}

void MigrationUtils::cleanBankingDetails(const QString& m2, const QString& b2, const QString& b3,
                                       QString& outBankName, QString& outAccountNo, QString& outIfsc) {
    outBankName.clear();
    outAccountNo.clear();
    outIfsc.clear();

    QString fullText = QString("%1 %2 %3").arg(m2, b2, b3).trimmed();
    if (fullText.isEmpty()) return;

    static const QRegularExpression ifscRe(QStringLiteral("\\b[A-Z]{4}0[A-Z0-9]{6}\\b"), QRegularExpression::CaseInsensitiveOption);
    QRegularExpressionMatch ifscMatch = ifscRe.match(fullText);
    if (ifscMatch.hasMatch()) {
        outIfsc = ifscMatch.captured(0).toUpper();
    }

    static const QRegularExpression acNoRe(QStringLiteral("\\b\\d{9,18}\\b"));
    QRegularExpressionMatch acMatch = acNoRe.match(fullText);
    if (acMatch.hasMatch()) {
        outAccountNo = acMatch.captured(0);
    }

    QString bankCandidate = b2.trimmed();
    if (bankCandidate.isEmpty() || bankCandidate == outAccountNo || bankCandidate == outIfsc) {
        bankCandidate = b3.trimmed();
    }
    if (bankCandidate.isEmpty() || bankCandidate == outAccountNo || bankCandidate == outIfsc) {
        bankCandidate = m2.trimmed();
    }

    QStringList bankKeywords = {"Bank", "SBI", "PNB", "HDFC", "ICICI", "Axis", "Canara", "Union", "BOB", "OBC", "Punjab", "State", "Baroda", "Kotak", "IndusInd"};
    for (const auto& kw : bankKeywords) {
        if (fullText.contains(kw, Qt::CaseInsensitive)) {
            if (outBankName.isEmpty() || !outBankName.contains(kw, Qt::CaseInsensitive)) {
                outBankName = kw;
                if (!outBankName.contains("Bank", Qt::CaseInsensitive)) outBankName += " Bank";
                break;
            }
        }
    }
    if (outBankName.isEmpty() && !bankCandidate.isEmpty() && bankCandidate != outAccountNo && bankCandidate != outIfsc) {
        outBankName = bankCandidate;
    }
}

QString MigrationUtils::extractFssai(const QString& business, const QString& address) {
    QString combined = business + " " + address;
    static const QRegularExpression fssaiRe(QStringLiteral("\\b\\d{14}\\b"));
    QRegularExpressionMatch match = fssaiRe.match(combined);
    if (match.hasMatch()) return match.captured(0);
    return "";
}

QString MigrationUtils::extractStateCode(const QString& gstin, const QString& state) {
    QString cleanGstin = gstin.trimmed().toUpper();
    if (cleanGstin.length() >= 2 && cleanGstin.left(2).toInt() > 0) {
        return cleanGstin.left(2);
    }

    QString s = state.trimmed().toLower();
    if (s.contains("haryana")) return "06";
    if (s.contains("punjab")) return "03";
    if (s.contains("delhi")) return "07";
    if (s.contains("rajasthan")) return "08";
    if (s.contains("uttar pradesh") || s.contains("u.p.") || s == "up") return "09";
    if (s.contains("himachal")) return "02";
    if (s.contains("chandigarh")) return "04";
    if (s.contains("uttarakhand")) return "05";
    if (s.contains("jammu") || s.contains("kashmir")) return "01";
    if (s.contains("bihar")) return "10";
    if (s.contains("west bengal")) return "19";
    if (s.contains("gujarat")) return "24";
    if (s.contains("maharashtra")) return "27";
    if (s.contains("madhya pradesh") || s == "mp") return "23";
    if (s.contains("andhra")) return "37";
    if (s.contains("telangana")) return "36";
    if (s.contains("karnataka")) return "29";
    if (s.contains("tamil")) return "33";
    if (s.contains("kerala")) return "32";
    return "06";
}

QString MigrationUtils::extractPincode(const QString& address, const QString& fallback) {
    static const QRegularExpression pinRe(QStringLiteral("\\b\\d{6}\\b"));
    QRegularExpressionMatch match = pinRe.match(address);
    if (match.hasMatch()) return match.captured(0);
    return fallback;
}

QString MigrationUtils::formatFyVoucherNo(const QString& isoDate, const QString& typeCode, const QString& rawNo) {
    QString fy = resolveFinancialYear(isoDate);
    QString fyShort = fy.mid(3, 4); // e.g. "2025" -> "25"
    if (fy.length() >= 9) {
        fyShort = fy.mid(5, 2) + fy.mid(8, 2); // e.g. "FY 2025-26" -> "2526"
    }
    QString cleanNo = rawNo.trimmed();
    static const QRegularExpression pRe(QStringLiteral("^(Sale|Sales|Purc|Purchase|Pur|Jrnl|Journal|ChPt|ChRt|Pymt|Rcpt|TDS|JFrm|J-Form)[-\\s#]*"), QRegularExpression::CaseInsensitiveOption);
    cleanNo.remove(pRe);
    if (cleanNo.isEmpty()) cleanNo = rawNo.trimmed();
    return QString("%1/%2-%3").arg(fyShort, typeCode, cleanNo);
}

} // namespace MahadevERP
