#pragma once

#include <QString>
#include <QStringList>
#include <QVariantMap>
#include <QVariantList>
#include <string>

/**
 * @brief Reusable Data Preprocessor for Bahi-Khata migration and application data feeding.
 * 
 * Provides unified sanitization, multi-format date parsing, currency/numeric conversions,
 * banking detail parsing, and text normalization without hardcoding.
 */
class DataPreprocessor {
public:
    // Text and String Sanitization
    static std::string cleanText(const std::string& s);
    static QString cleanString(const QString& s);
    static std::string toLowerStr(const std::string& s);
    static QString toTitleCase(const QString& s);
    static QString sanitizeForSql(const QString& s);

    // Numeric & Financial Parsing
    static double parseDouble(const std::string& s, double def = 0.0);
    static double parseDouble(const QString& s, double def = 0.0);
    static int parseInt(const std::string& s, int def = 0);
    static int parseInt(const QString& s, int def = 0);
    static qint64 parseLong(const QString& s, qint64 def = 0);
    static double roundTo(double value, int decimalPlaces = 2);

    // Date & Time Normalization (formats any legacy string to YYYY-MM-DD)
    static QString parseDateFormatted(const QString& raw);
    static QString parseDateTimeFormatted(const QString& raw);
    static bool isValidDate(const QString& dateStr);

    // Banking & Financial Entity Parsing
    static void cleanBankingDetails(
        const QString& rawMemo,
        const QString& rawBank1,
        const QString& rawBank2,
        QString& outBankName,
        QString& outAccountNo,
        QString& outIfscCode
    );

    // GSTIN & PAN Extraction & Validation
    static QString extractGstin(const QString& text);
    static QString extractPan(const QString& text);
    static bool isValidGstin(const QString& gstin);
    static bool isValidPan(const QString& pan);

    // Narration & Invoice Details Unpacker
    static QVariantMap parseInvoiceDetailsString(const QString& detailsStr);
};
