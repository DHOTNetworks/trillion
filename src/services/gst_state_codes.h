#pragma once

#include <QMap>
#include <QString>

/// @brief Canonical GST state-code table — the ONLY place state names/codes live.
///
/// Replaces three divergent copies (PartiesModel 37-code map, GstTaxEngine
/// 18-name chain, NewFirmDialog 5-name chain). Codes are GST state codes
/// ("06" = Haryana). Unknown codes yield "" — callers decide the fallback
/// explicitly instead of printing "Other State" from inside a lookup.
namespace GstStateCodes {

inline const QMap<QString, QString>& codeToState() {
    static const QMap<QString, QString> kMap = {
        {"01", "Jammu & Kashmir"}, {"02", "Himachal Pradesh"}, {"03", "Punjab"},
        {"04", "Chandigarh"}, {"05", "Uttarakhand"}, {"06", "Haryana"},
        {"07", "Delhi"}, {"08", "Rajasthan"}, {"09", "Uttar Pradesh"},
        {"10", "Bihar"}, {"11", "Sikkim"}, {"12", "Arunachal Pradesh"},
        {"13", "Nagaland"}, {"14", "Manipur"}, {"15", "Mizoram"},
        {"16", "Tripura"}, {"17", "Meghalaya"}, {"18", "Assam"},
        {"19", "West Bengal"}, {"20", "Jharkhand"}, {"21", "Odisha"},
        {"22", "Chhattisgarh"}, {"23", "Madhya Pradesh"}, {"24", "Gujarat"},
        {"26", "Dadra & Nagar Haveli and Daman & Diu"}, {"27", "Maharashtra"},
        {"29", "Karnataka"}, {"30", "Goa"}, {"31", "Lakshadweep"},
        {"32", "Kerala"}, {"33", "Tamil Nadu"}, {"34", "Puducherry"},
        {"36", "Telangana"}, {"37", "Andhra Pradesh"}, {"38", "Ladakh"},
    };
    return kMap;
}

inline QString stateForCode(const QString& code) {
    QString c = code.trimmed();
    if (c.length() == 1 && c.at(0).isDigit()) c.prepend('0');
    return codeToState().value(c, QString());
}

inline QString codeForState(const QString& state) {
    const QString want = state.trimmed().toLower();
    if (want.isEmpty()) return QString();
    for (auto it = codeToState().cbegin(); it != codeToState().cend(); ++it) {
        if (it.value().toLower() == want) return it.key();
    }
    // Common aliases accepted at mandi counters.
    static const QMap<QString, QString> kAliases = {
        {"jammu and kashmir", "01"}, {"uttaranchal", "05"}, {"up", "09"},
        {"orissa", "21"}, {"mp", "23"}, {"pondicherry", "34"},
    };
    return kAliases.value(want, QString());
}

inline QString stateForGstin(const QString& gstin) {
    const QString g = gstin.trimmed();
    if (g.length() >= 2 && g.at(0).isDigit() && g.at(1).isDigit())
        return stateForCode(g.left(2));
    return QString();
}

} // namespace GstStateCodes
