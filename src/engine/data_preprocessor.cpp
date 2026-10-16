#include "data_preprocessor.h"
#include <QMap>
#include <QRegularExpression>
#include <QDate>
#include <QDateTime>
#include <algorithm>
#include <cmath>

std::string DataPreprocessor::cleanText(const std::string& s) {
    std::string temp;
    temp.reserve(s.size());
    for (size_t i = 0; i < s.size(); ++i) {
        if (static_cast<unsigned char>(s[i]) == 0xC2 && i + 1 < s.size() && static_cast<unsigned char>(s[i+1]) == 0xA0) {
            temp += ' ';
            i++;
        } else if (static_cast<unsigned char>(s[i]) == 0xA0) {
            temp += ' ';
        } else {
            temp += s[i];
        }
    }
    size_t first = temp.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) return "";
    size_t last = temp.find_last_not_of(" \t\r\n");
    std::string sub = temp.substr(first, last - first + 1);
    if (sub == "none" || sub == "null" || sub == "None" || sub == "NULL") return "";
    return sub;
}

QString DataPreprocessor::cleanString(const QString& s) {
    return QString::fromStdString(cleanText(s.toStdString()));
}

std::string DataPreprocessor::toLowerStr(const std::string& s) {
    std::string out = s;
    std::transform(out.begin(), out.end(), out.begin(), [](unsigned char c) { return std::tolower(c); });
    return out;
}

QString DataPreprocessor::toTitleCase(const QString& s) {
    QString trimmed = s.trimmed();
    if (trimmed.isEmpty()) return "";
    QStringList words = trimmed.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
    for (QString& word : words) {
        if (!word.isEmpty()) {
            word = word.left(1).toUpper() + word.mid(1).toLower();
        }
    }
    return words.join(" ");
}

QString DataPreprocessor::sanitizeForSql(const QString& s) {
    QString res = s;
    return res.replace("'", "''");
}

double DataPreprocessor::parseDouble(const std::string& s, double def) {
    std::string clean = cleanText(s);
    if (clean.empty()) return def;
    std::string num;
    for (char c : clean) {
        if (c != ',' && c != '%') num += c;
    }
    try {
        return std::stod(num);
    } catch (...) {
        return def;
    }
}

double DataPreprocessor::parseDouble(const QString& s, double def) {
    return parseDouble(s.toStdString(), def);
}

int DataPreprocessor::parseInt(const std::string& s, int def) {
    std::string clean = cleanText(s);
    if (clean.empty()) return def;
    std::string num;
    for (char c : clean) {
        if (c != ',') num += c;
    }
    try {
        return static_cast<int>(std::stod(num));
    } catch (...) {
        return def;
    }
}

int DataPreprocessor::parseInt(const QString& s, int def) {
    return parseInt(s.toStdString(), def);
}

qint64 DataPreprocessor::parseLong(const QString& s, qint64 def) {
    std::string clean = cleanText(s.toStdString());
    if (clean.empty()) return def;
    std::string num;
    for (char c : clean) {
        if (c != ',') num += c;
    }
    try {
        return static_cast<qint64>(std::stoll(num));
    } catch (...) {
        return def;
    }
}

double DataPreprocessor::roundTo(double value, int decimalPlaces) {
    double factor = std::pow(10.0, decimalPlaces);
    return std::round(value * factor) / factor;
}

QString DataPreprocessor::parseDateFormatted(const QString& raw) {
    QString s = raw.trimmed();
    if (s.isEmpty()) return "";
    QString firstPart = s.split(' ').first();
    
    // Check DD/MM/YYYY or MM/DD/YYYY
    QStringList parts = firstPart.split('/');
    if (parts.size() == 3) {
        int m = parts[0].toInt();
        int d = parts[1].toInt();
        int y = parts[2].toInt();
        if (y < 100) y += 2000;
        
        // If first number > 12, it must be DD/MM/YYYY
        if (m > 12 && d <= 12) {
            std::swap(m, d);
        }
        return QString("%1-%2-%3")
            .arg(y, 4, 10, QChar('0'))
            .arg(m, 2, 10, QChar('0'))
            .arg(d, 2, 10, QChar('0'));
    }

    // Check YYYY-MM-DD or DD-MM-YYYY
    parts = firstPart.split('-');
    if (parts.size() == 3) {
        if (parts[0].length() == 4) return firstPart;
        int d = parts[0].toInt();
        int m = parts[1].toInt();
        int y = parts[2].toInt();
        if (y < 100) y += 2000;
        return QString("%1-%2-%3")
            .arg(y, 4, 10, QChar('0'))
            .arg(m, 2, 10, QChar('0'))
            .arg(d, 2, 10, QChar('0'));
    }

    return firstPart;
}

QString DataPreprocessor::parseDateTimeFormatted(const QString& raw) {
    QString s = raw.trimmed();
    if (s.isEmpty()) return "";
    QStringList parts = s.split(' ');
    QString datePart = parseDateFormatted(parts.first());
    if (parts.size() > 1) {
        return datePart + " " + parts.mid(1).join(" ");
    }
    return datePart;
}

bool DataPreprocessor::isValidDate(const QString& dateStr) {
    QDate d = QDate::fromString(dateStr, "yyyy-MM-dd");
    return d.isValid();
}

void DataPreprocessor::cleanBankingDetails(
    const QString& m2, const QString& b2, const QString& b3,
    QString& outBankName, QString& outAccountNo, QString& outIfscCode
) {
    outBankName = "";
    outAccountNo = "";
    outIfscCode = "";

    QRegularExpression ifscRegex("[A-Z]{4}0[A-Z0-9]{6}", QRegularExpression::CaseInsensitiveOption);
    QRegularExpression accRegex("^(?:A/C\\s*(?:NO\\.?)?\\s*)?([0-9]{9,18})$", QRegularExpression::CaseInsensitiveOption);
    QRegularExpression pureAccRegex("^[0-9]{9,18}$");

    QStringList candidates = { m2.trimmed(), b2.trimmed(), b3.trimmed() };
    QStringList remainingCandidates;

    // 1. Identify IFSC
    for (const QString& cand : candidates) {
        if (cand.isEmpty()) continue;
        QRegularExpressionMatch match = ifscRegex.match(cand);
        if (match.hasMatch() && outIfscCode.isEmpty()) {
            outIfscCode = match.captured(0).toUpper();
            QString rem = cand;
            rem = rem.remove(match.captured(0)).trimmed();
            rem = rem.remove(QRegularExpression("IFSC\\s*(?:CODE)?\\s*:?", QRegularExpression::CaseInsensitiveOption)).trimmed();
            if (!rem.isEmpty()) {
                remainingCandidates.append(rem);
            }
        } else {
            remainingCandidates.append(cand);
        }
    }

    // 2. Identify Account Number
    QStringList finalBankCandidates;
    for (const QString& cand : remainingCandidates) {
        if (cand.isEmpty()) continue;
        QRegularExpressionMatch accMatch = accRegex.match(cand);
        QRegularExpressionMatch pureMatch = pureAccRegex.match(cand);
        if ((accMatch.hasMatch() || pureMatch.hasMatch()) && outAccountNo.isEmpty()) {
            if (accMatch.hasMatch() && !accMatch.captured(1).isEmpty()) {
                outAccountNo = accMatch.captured(1).trimmed();
            } else if (pureMatch.hasMatch()) {
                outAccountNo = pureMatch.captured(0).trimmed();
            } else {
                outAccountNo = cand;
            }
        } else {
            finalBankCandidates.append(cand);
        }
    }

    // 3. Identify Bank Name
    for (const QString& cand : finalBankCandidates) {
        if (cand.isEmpty()) continue;
        if (cand.contains("BANK", Qt::CaseInsensitive) || 
            cand.contains("PUNJAB", Qt::CaseInsensitive) || 
            cand.contains("HDFC", Qt::CaseInsensitive) || 
            cand.contains("ICICI", Qt::CaseInsensitive) || 
            cand.contains("SBI", Qt::CaseInsensitive) || 
            cand.contains("AXIS", Qt::CaseInsensitive) ||
            cand.contains("KOTAK", Qt::CaseInsensitive) ||
            cand.contains("OBC", Qt::CaseInsensitive) ||
            cand.contains("PNB", Qt::CaseInsensitive) ||
            cand.contains("CANARA", Qt::CaseInsensitive) ||
            cand.contains("UNION", Qt::CaseInsensitive) ||
            cand.contains("BARODA", Qt::CaseInsensitive)) {
            if (outBankName.isEmpty()) {
                outBankName = cand;
            } else {
                outBankName += " " + cand;
            }
        } else {
            if (outAccountNo.isEmpty() && cand.contains(QRegularExpression("[0-9]{6,}"))) {
                outAccountNo = cand;
            } else if (outBankName.isEmpty()) {
                outBankName = cand;
            }
        }
    }
}

QString DataPreprocessor::extractGstin(const QString& text) {
    QRegularExpression gstinRegex("\\b([0-9]{2}[A-Z]{5}[0-9]{4}[A-Z]{1}[1-9A-Z]{1}Z[0-9A-Z]{1})\\b", QRegularExpression::CaseInsensitiveOption);
    QRegularExpressionMatch match = gstinRegex.match(text);
    if (match.hasMatch()) {
        return match.captured(1).toUpper();
    }
    return "";
}

QString DataPreprocessor::extractPan(const QString& text) {
    QRegularExpression panRegex("\\b([A-Z]{5}[0-9]{4}[A-Z]{1})\\b", QRegularExpression::CaseInsensitiveOption);
    QRegularExpressionMatch match = panRegex.match(text);
    if (match.hasMatch()) {
        return match.captured(1).toUpper();
    }
    return "";
}

bool DataPreprocessor::isValidGstin(const QString& gstin) {
    QRegularExpression gstinRegex("^[0-9]{2}[A-Z]{5}[0-9]{4}[A-Z]{1}[1-9A-Z]{1}Z[0-9A-Z]{1}$", QRegularExpression::CaseInsensitiveOption);
    return gstinRegex.match(gstin.trimmed()).hasMatch();
}

bool DataPreprocessor::isValidPan(const QString& pan) {
    QRegularExpression panRegex("^[A-Z]{5}[0-9]{4}[A-Z]{1}$", QRegularExpression::CaseInsensitiveOption);
    return panRegex.match(pan.trimmed()).hasMatch();
}

QVariantMap DataPreprocessor::parseInvoiceDetailsString(const QString& detailsStr) {
    QVariantMap map;
    QStringList tokens = detailsStr.split('|', Qt::SkipEmptyParts);
    for (const QString& token : tokens) {
        QString t = token.trimmed();
        if (t.startsWith("Invoice No.", Qt::CaseInsensitive)) {
            map["invoice_no"] = t.mid(11).trimmed();
        } else if (t.contains("@")) {
            map["item_line"] = t;
        } else if (!t.isEmpty()) {
            if (!map.contains("remarks")) {
                map["remarks"] = t;
            } else {
                map["remarks"] = map["remarks"].toString() + " | " + t;
            }
        }
    }
    return map;
}

QString DataPreprocessor::getStateNameFromCode(const QString& code) {
    static const QMap<QString, QString> codeToState = {
        {"01", "Jammu & Kashmir"},
        {"02", "Himachal Pradesh"},
        {"03", "Punjab"},
        {"04", "Chandigarh"},
        {"05", "Uttarakhand"},
        {"06", "Haryana"},
        {"07", "Delhi"},
        {"08", "Rajasthan"},
        {"09", "Uttar Pradesh"},
        {"10", "Bihar"},
        {"11", "Sikkim"},
        {"12", "Arunachal Pradesh"},
        {"13", "Nagaland"},
        {"14", "Manipur"},
        {"15", "Mizoram"},
        {"16", "Tripura"},
        {"17", "Meghalaya"},
        {"18", "Assam"},
        {"19", "West Bengal"},
        {"20", "Jharkhand"},
        {"21", "Odisha"},
        {"22", "Chhattisgarh"},
        {"23", "Madhya Pradesh"},
        {"24", "Gujarat"},
        {"26", "Dadra & Nagar Haveli and Daman & Diu"},
        {"27", "Maharashtra"},
        {"29", "Karnataka"},
        {"30", "Goa"},
        {"31", "Lakshadweep"},
        {"32", "Kerala"},
        {"33", "Tamil Nadu"},
        {"34", "Puducherry"},
        {"36", "Telangana"},
        {"37", "Andhra Pradesh"},
        {"38", "Ladakh"}
    };
    return codeToState.value(code, "");
}

QString DataPreprocessor::extractStateCode(const QString& gstin, const QString& state) {
    QString g = gstin.trimmed();
    if (g.length() >= 2 && g.at(0).isDigit() && g.at(1).isDigit()) {
        return g.left(2);
    }
    QString s = state.trimmed().toLower();
    if (s.contains("jammu") || s.contains("kashmir")) return "01";
    if (s.contains("himachal")) return "02";
    if (s.contains("punjab")) return "03";
    if (s.contains("chandigarh")) return "04";
    if (s.contains("uttarakhand") || s.contains("uttaranchal")) return "05";
    if (s.contains("haryana")) return "06";
    if (s.contains("delhi")) return "07";
    if (s.contains("rajasthan")) return "08";
    if (s.contains("uttar pradesh") || s == "up") return "09";
    if (s.contains("bihar")) return "10";
    if (s.contains("sikkim")) return "11";
    if (s.contains("arunachal")) return "12";
    if (s.contains("nagaland")) return "13";
    if (s.contains("manipur")) return "14";
    if (s.contains("mizoram")) return "15";
    if (s.contains("tripura")) return "16";
    if (s.contains("meghalaya")) return "17";
    if (s.contains("assam")) return "18";
    if (s.contains("bengal")) return "19";
    if (s.contains("jharkhand")) return "20";
    if (s.contains("odisha") || s.contains("orissa")) return "21";
    if (s.contains("chhattisgarh")) return "22";
    if (s.contains("madhya pradesh") || s == "mp") return "23";
    if (s.contains("gujarat")) return "24";
    if (s.contains("maharashtra")) return "27";
    if (s.contains("karnataka")) return "29";
    if (s.contains("goa")) return "30";
    if (s.contains("kerala")) return "32";
    if (s.contains("tamil nadu") || s.contains("tamilnadu")) return "33";
    if (s.contains("puducherry") || s.contains("pondicherry")) return "34";
    if (s.contains("telangana")) return "36";
    if (s.contains("andhra")) return "37";
    if (s.contains("ladakh")) return "38";
    return "06";
}

QString DataPreprocessor::extractPincode(const QString& address, const QString& fallback) {
    QRegularExpression pinRegex(R"(\b(1\d{5}|[2-8]\d{5})\b)");
    auto match = pinRegex.match(address);
    if (match.hasMatch()) {
        return match.captured(1);
    }
    return fallback;
}
