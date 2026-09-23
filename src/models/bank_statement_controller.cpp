#include "bank_statement_controller.h"
#include "../database_manager.h"
#include "../engine/fiscal_year_helper.h"
#include "../services/financial_math_service.h"
#include "../services/accounting_date_service.h"
#include "../services/bank_statement_excel_parser.h"
#include "vouchers_model.h"
#include <QRegularExpression>
#include <QDate>
#include <QDebug>
#include <QFileDialog>
#include <QDir>
#include <cmath>

// ---------------------------------------------------------------------------
// BankStatementRowsModel Implementation
// ---------------------------------------------------------------------------

BankStatementRowsModel::BankStatementRowsModel(QObject *parent)
    : QAbstractListModel(parent) {
}

int BankStatementRowsModel::rowCount(const QModelIndex &parent) const {
    if (parent.isValid()) return 0;
    return m_visibleIndices.size();
}

QVariant BankStatementRowsModel::data(const QModelIndex &index, int role) const {
    if (!index.isValid() || index.row() < 0 || index.row() >= m_visibleIndices.size()) {
        return QVariant();
    }

    int rawIdx = m_visibleIndices.at(index.row());
    const auto &r = m_allRows.at(rawIdx);
    switch (role) {
    case IndexRole: return r.index;
    case DateRole: return r.dateStr;
    case IsoDateRole: return r.isoDate;
    case NarrationRole: return r.rawNarration;
    case UtrRefRole: return r.utrRef;
    case WithdrawalRole: return r.withdrawal;
    case DepositRole: return r.deposit;
    case BalanceRole: return r.balance;
    case AmountRole: return r.deposit > 0.001 ? r.deposit : r.withdrawal;
    case CategoryRole: return r.category;
    case ExtractedPartyRole: return r.extractedParty;
    case DrAccountRole: return r.suggestedDrAccount;
    case CrAccountRole: return r.suggestedCrAccount;
    case VoucherTypeRole: return r.voucherType;
    case ConfidenceRole: return r.confidence;
    case IsDuplicateRole: return r.isDuplicate;
    case IsSelectedRole: return r.isSelected;
    default: return QVariant();
    }
}

bool BankStatementRowsModel::setData(const QModelIndex &index, const QVariant &value, int role) {
    if (!index.isValid() || index.row() < 0 || index.row() >= m_visibleIndices.size()) {
        return false;
    }

    int rawIdx = m_visibleIndices.at(index.row());
    auto &r = m_allRows[rawIdx];
    bool changed = false;

    switch (role) {
    case IsSelectedRole:
        if (r.isSelected != value.toBool()) {
            r.isSelected = value.toBool();
            changed = true;
            emit selectionChanged();
        }
        break;
    case DrAccountRole:
        if (r.suggestedDrAccount != value.toString()) {
            r.suggestedDrAccount = value.toString();
            r.confidence = "HIGH";
            r.isSelected = true;
            changed = true;
            emit selectionChanged();
        }
        break;
    case CrAccountRole:
        if (r.suggestedCrAccount != value.toString()) {
            r.suggestedCrAccount = value.toString();
            r.confidence = "HIGH";
            r.isSelected = true;
            changed = true;
            emit selectionChanged();
        }
        break;
    case VoucherTypeRole:
        if (r.voucherType != value.toString()) {
            r.voucherType = value.toString();
            changed = true;
        }
        break;
    default:
        break;
    }

    if (changed) {
        emit dataChanged(index, index);
        emit rowsChanged();
        return true;
    }
    return false;
}

QHash<int, QByteArray> BankStatementRowsModel::roleNames() const {
    QHash<int, QByteArray> roles;
    roles[IndexRole] = "rowIndex";
    roles[DateRole] = "vDate";
    roles[IsoDateRole] = "isoDate";
    roles[NarrationRole] = "narration";
    roles[UtrRefRole] = "utrRef";
    roles[WithdrawalRole] = "withdrawal";
    roles[DepositRole] = "deposit";
    roles[BalanceRole] = "balance";
    roles[AmountRole] = "amount";
    roles[CategoryRole] = "category";
    roles[ExtractedPartyRole] = "extractedParty";
    roles[DrAccountRole] = "drAccount";
    roles[CrAccountRole] = "crAccount";
    roles[VoucherTypeRole] = "voucherType";
    roles[ConfidenceRole] = "confidence";
    roles[IsDuplicateRole] = "isDuplicate";
    roles[IsSelectedRole] = "isSelected";
    return roles;
}

void BankStatementRowsModel::applyFilter() {
    beginResetModel();
    m_visibleIndices.clear();

    QString q = m_searchQuery.toLower();
    QString ft = m_filterType.toUpper().trimmed();

    for (int i = 0; i < m_allRows.size(); ++i) {
        const auto &r = m_allRows[i];

        if ((ft.contains("RECEIPT") || ft.contains("DEPOSIT")) && r.deposit <= 0.001) continue;
        if ((ft.contains("PAYMENT") || ft.contains("WITHDRAWAL")) && r.withdrawal <= 0.001) continue;
        if (ft.contains("CHARGE") && r.category != "BANK_CHARGES") continue;
        if (ft.contains("INTEREST") && r.category != "INTEREST_DEBIT") continue;
        if (ft.contains("UNMATCH") && r.confidence != "UNMATCHED") continue;
        if (ft.contains("DUPLICATE") && !r.isDuplicate) continue;

        if (!q.isEmpty()) {
            bool matches = r.cleanNarration.toLower().contains(q) ||
                           r.rawNarration.toLower().contains(q) ||
                           r.utrRef.toLower().contains(q) ||
                           r.suggestedDrAccount.toLower().contains(q) ||
                           r.suggestedCrAccount.toLower().contains(q) ||
                           r.extractedParty.toLower().contains(q) ||
                           r.dateStr.toLower().contains(q) ||
                           r.isoDate.toLower().contains(q);
            if (!matches) continue;
        }

        m_visibleIndices.append(i);
    }
    endResetModel();
    emit rowsChanged();
}

void BankStatementRowsModel::setFilter(const QString &filterType, const QString &searchQuery) {
    if (m_filterType != filterType || m_searchQuery != searchQuery) {
        m_filterType = filterType;
        m_searchQuery = searchQuery;
        applyFilter();
    }
}

void BankStatementRowsModel::setRows(const QVector<CanaraBankTransaction> &rows) {
    m_allRows = rows;
    applyFilter();
    emit selectionChanged();
}

void BankStatementRowsModel::clear() {
    beginResetModel();
    m_allRows.clear();
    m_visibleIndices.clear();
    endResetModel();
    emit rowsChanged();
    emit selectionChanged();
}

void BankStatementRowsModel::setRowSelected(int visibleIndex, bool selected) {
    if (visibleIndex >= 0 && visibleIndex < m_visibleIndices.size()) {
        setData(this->index(visibleIndex), selected, IsSelectedRole);
    }
}

void BankStatementRowsModel::setRowDrAccount(int visibleIndex, const QString &acc) {
    if (visibleIndex >= 0 && visibleIndex < m_visibleIndices.size()) {
        setData(this->index(visibleIndex), acc, DrAccountRole);
    }
}

void BankStatementRowsModel::setRowCrAccount(int visibleIndex, const QString &acc) {
    if (visibleIndex >= 0 && visibleIndex < m_visibleIndices.size()) {
        setData(this->index(visibleIndex), acc, CrAccountRole);
    }
}

void BankStatementRowsModel::setRowVoucherType(int visibleIndex, const QString &vType) {
    if (visibleIndex >= 0 && visibleIndex < m_visibleIndices.size()) {
        setData(this->index(visibleIndex), vType, VoucherTypeRole);
    }
}

void BankStatementRowsModel::selectAll(bool selected) {
    for (int visibleIdx : m_visibleIndices) {
        m_allRows[visibleIdx].isSelected = selected;
    }
    if (!m_visibleIndices.isEmpty()) {
        emit dataChanged(index(0), index(m_visibleIndices.size() - 1), {IsSelectedRole});
    }
    emit selectionChanged();
}

void BankStatementRowsModel::deselectDuplicates() {
    for (auto &r : m_allRows) {
        if (r.isDuplicate) {
            r.isSelected = false;
        }
    }
    if (!m_visibleIndices.isEmpty()) {
        emit dataChanged(index(0), index(m_visibleIndices.size() - 1), {IsSelectedRole});
    }
    emit selectionChanged();
}

QVariantMap BankStatementRowsModel::getRow(int visibleIndex) const {
    if (visibleIndex < 0 || visibleIndex >= m_visibleIndices.size()) return QVariantMap();
    int rawIdx = m_visibleIndices.at(visibleIndex);
    const auto &r = m_allRows.at(rawIdx);
    QVariantMap map;
    map["rowIndex"] = r.index;
    map["vDate"] = r.dateStr;
    map["isoDate"] = r.isoDate;
    map["date"] = !r.dateStr.isEmpty() ? r.dateStr : r.isoDate;
    map["narration"] = r.rawNarration;
    map["utrRef"] = r.utrRef;
    map["withdrawal"] = r.withdrawal;
    map["deposit"] = r.deposit;
    map["balance"] = r.balance;
    map["amount"] = r.deposit > 0.001 ? r.deposit : r.withdrawal;
    map["category"] = r.category;
    map["extractedParty"] = r.extractedParty;
    map["drAccount"] = r.suggestedDrAccount;
    map["crAccount"] = r.suggestedCrAccount;
    map["voucherType"] = r.voucherType;
    map["confidence"] = r.confidence;
    map["isDuplicate"] = r.isDuplicate;
    map["isSelected"] = r.isSelected;
    return map;
}

// ---------------------------------------------------------------------------
// BankStatementController Implementation
// ---------------------------------------------------------------------------

BankStatementController::BankStatementController(QObject *parent)
    : QObject(parent) {
    connect(&m_rowsModel, &BankStatementRowsModel::selectionChanged, this, &BankStatementController::onModelSelectionChanged);
    connect(&m_rowsModel, &BankStatementRowsModel::rowsChanged, this, &BankStatementController::recalculateTotalsAndCounts);
}

QString BankStatementController::fileName() const {
    if (m_statementPath.isEmpty()) return "";
    return QFileInfo(m_statementPath).fileName();
}

QString BankStatementController::dateRange() const {
    if (m_header.fromDate.isEmpty() && m_header.toDate.isEmpty()) return "";
    return QString("%1 to %2").arg(m_header.fromDate, m_header.toDate);
}

void BankStatementController::setBankLedger(const QString &name) {
    if (m_bankLedgerName != name) {
        m_bankLedgerName = name;
        emit bankLedgerChanged();

        // Update default dr/cr account across rows if needed
        auto rows = m_rowsModel.rows();
        for (auto &r : rows) {
            if (r.deposit > 0.001) {
                r.suggestedDrAccount = m_bankLedgerName;
            } else {
                r.suggestedCrAccount = m_bankLedgerName;
            }
        }
        m_rowsModel.setRows(rows);
    }
}

QString BankStatementController::totalWithdrawalsFmt() const {
    return FinancialMathService::instance().formatInr(m_totalWithdrawals);
}

QString BankStatementController::totalDepositsFmt() const {
    return FinancialMathService::instance().formatInr(m_totalDeposits);
}

QString BankStatementController::selectedWithdrawalsFmt() const {
    return FinancialMathService::instance().formatInr(m_selectedWithdrawals);
}

QString BankStatementController::selectedDepositsFmt() const {
    return FinancialMathService::instance().formatInr(m_selectedDeposits);
}

void BankStatementController::detectBankLedger() {
    QString acNo = m_header.accountNo.trimmed();
    QVariantMap foundLedger;

    if (!acNo.isEmpty()) {
        // Search by account number in parties table
        QVariantList rows = DatabaseManager::instance().executeQuery(
            "SELECT name FROM parties WHERE bank_account = ? OR name LIKE ? OR alias LIKE ? LIMIT 1;",
            {acNo, "%" + acNo + "%", "%" + acNo + "%"}
        );
        if (!rows.isEmpty()) {
            m_bankLedgerName = rows.first().toMap().value("name").toString();
            emit bankLedgerChanged();
            return;
        }
    }

    // Fallback search for matching bank account
    QVariantList bankRows = DatabaseManager::instance().executeQuery(
        "SELECT name FROM parties WHERE (group_name LIKE '%Bank%' OR group_name LIKE '%Secured Loans%') "
        "ORDER BY id ASC LIMIT 1;"
    );
    if (!bankRows.isEmpty()) {
        m_bankLedgerName = bankRows.first().toMap().value("name").toString();
    } else {
        m_bankLedgerName = "Bank Account";
    }
    emit bankLedgerChanged();
}

QString BankStatementController::findMatchingParty(const QString &extractedName, const QString &narration, QString &outConfidence) {
    if (extractedName.isEmpty() && narration.isEmpty()) {
        outConfidence = "UNMATCHED";
        return QString();
    }

    // 1. Check Bank Narration Aliases Table (Learned memory)
    if (!narration.isEmpty() || !extractedName.isEmpty()) {
        QVariantList aliasRows = DatabaseManager::instance().executeQuery(
            "SELECT mapped_party_name FROM bank_narration_aliases "
            "WHERE (? LIKE ('%' || narration_pattern || '%') OR narration_pattern = ? COLLATE NOCASE) "
            "  AND LENGTH(narration_pattern) >= 4 "
            "ORDER BY LENGTH(narration_pattern) DESC LIMIT 1;",
            {narration, extractedName}
        );
        if (!aliasRows.isEmpty()) {
            outConfidence = "ALIAS_MATCH";
            return aliasRows.first().toMap().value("mapped_party_name").toString();
        }
    }

    // 2. Direct exact and smart normalized search on parties table
    if (!extractedName.isEmpty()) {
        QString clean = extractedName.trimmed();

        // Exact match on Name or Alias
        QVariant directRow = DatabaseManager::instance().executeScalar(
            "SELECT name FROM parties WHERE name = ? COLLATE NOCASE OR alias = ? COLLATE NOCASE LIMIT 1;",
            {clean, clean}
        );
        if (directRow.isValid()) {
            outConfidence = "HIGH";
            return directRow.toString();
        }

        // Handle variations of AND <-> &
        QString varAnd = clean;
        varAnd.replace(QRegularExpression("\\bAND\\b", QRegularExpression::CaseInsensitiveOption), "&");
        varAnd = varAnd.simplified();

        QString varAmp = clean;
        varAmp.replace("&", "AND");
        varAmp = varAmp.simplified();

        for (const QString &v : {varAnd, varAmp}) {
            QVariant varRow = DatabaseManager::instance().executeScalar(
                "SELECT name FROM parties WHERE name LIKE ? COLLATE NOCASE OR alias LIKE ? COLLATE NOCASE LIMIT 1;",
                {"%" + v + "%", "%" + v + "%"}
            );
            if (varRow.isValid()) {
                outConfidence = "HIGH";
                return varRow.toString();
            }
        }

        // Strip corporate suffixes (LIMITED, LTD, PRIVATE, PVT, CORP, COMPANY, CO, LLP)
        QString core = varAnd;
        core.remove(QRegularExpression("\\b(LIMITED|LTD|PRIVATE|PVT|CORPORATION|CORP|COMPANY|CO|LLP|INC)\\b", QRegularExpression::CaseInsensitiveOption));
        core.remove(QRegularExpression("[^A-Za-z0-9& ]"));
        core = core.simplified();

        if (core.length() >= 3) {
            QVariant coreRow = DatabaseManager::instance().executeScalar(
                "SELECT name FROM parties WHERE name LIKE ? COLLATE NOCASE OR alias LIKE ? COLLATE NOCASE LIMIT 1;",
                {"%" + core + "%", "%" + core + "%"}
            );
            if (coreRow.isValid()) {
                outConfidence = "HIGH";
                return coreRow.toString();
            }

            // Keyword fuzzy match with meaningful keywords (>= 3 chars)
            QStringList rawWords = core.split(' ', Qt::SkipEmptyParts);
            QStringList sigWords;
            for (const QString &w : rawWords) {
                if (w.length() >= 3 || w == "&" || w.contains('&')) {
                    sigWords.append(w);
                }
            }

            if (!sigWords.isEmpty()) {
                QString query = "SELECT name FROM parties WHERE ";
                QVariantList params;
                for (int i = 0; i < sigWords.size(); ++i) {
                    if (i > 0) query += " AND ";
                    query += "(name LIKE ? OR alias LIKE ?)";
                    params.append("%" + sigWords[i] + "%");
                    params.append("%" + sigWords[i] + "%");
                }
                query += " LIMIT 1;";
                QVariant fuzzyRow = DatabaseManager::instance().executeScalar(query, params);
                if (fuzzyRow.isValid()) {
                    outConfidence = "HIGH";
                    return fuzzyRow.toString();
                }
            }
        }
    }

    // 3. Category Fallbacks
    if (narration.contains(" SC", Qt::CaseInsensitive) || narration.contains("SERVICE CHARGE", Qt::CaseInsensitive) || narration.contains("MORTGAGE CHARGES", Qt::CaseInsensitive) || narration.contains("CHG", Qt::CaseInsensitive)) {
        QVariant bgRow = DatabaseManager::instance().executeScalar("SELECT name FROM parties WHERE name LIKE '%Bank Charges%' LIMIT 1;");
        outConfidence = "HIGH";
        return bgRow.isValid() ? bgRow.toString() : "Bank Charges";
    }

    if (narration.contains("QOS PERIOD", Qt::CaseInsensitive) || narration.contains("INTEREST", Qt::CaseInsensitive)) {
        QVariant intRow = DatabaseManager::instance().executeScalar("SELECT name FROM parties WHERE name LIKE '%Interest Bank%' OR name LIKE '%Bank Interest%' LIMIT 1;");
        outConfidence = "HIGH";
        return intRow.isValid() ? intRow.toString() : "Interest Bank A/c";
    }

    outConfidence = "UNMATCHED";
    return QString();
}

struct CachedAlias {
    QString pattern;
    QString mappedParty;
};

struct CachedParty {
    QString name;
    QString alias;
    QString cleanName;
    QString cleanAlias;
    QStringList tokens;
};

struct CachedTxn {
    qint64 id = 0;
    QString voucherNo;
    QString voucherType;
    QString date;
    QString opposingAccount;
    QString partyName;
    double amount = 0.0;
    QString drCr;
    bool matched = false;
};

void BankStatementController::matchPartiesAndDuplicates(QVector<CanaraBankTransaction> &txns) {
    // 1. Pre-load all learned narration aliases in one batch
    QVector<CachedAlias> cachedAliases;
    QVariantList aliasRows = DatabaseManager::instance().executeQuery(
        "SELECT narration_pattern, mapped_party_name FROM bank_narration_aliases "
        "WHERE LENGTH(narration_pattern) >= 4 "
        "ORDER BY LENGTH(narration_pattern) DESC;"
    );
    for (const auto &ar : aliasRows) {
        auto map = ar.toMap();
        cachedAliases.append({
            map.value("narration_pattern").toString().trimmed(),
            map.value("mapped_party_name").toString().trimmed()
        });
    }

    // 2. Pre-load all parties into high-speed memory maps
    QVector<CachedParty> cachedParties;
    QHash<QString, QString> exactPartyMap;
    QHash<QString, QString> basePartyMap;
    QHash<QString, QString> phoneticPartyMap;
    QHash<QString, QString> spacelessPartyMap;
    QHash<QString, QString> accountPartyMap;

    // Also index any numeric account patterns from learned aliases
    static QRegularExpression acDigitsRegex(R"(\b\d{9,18}\b)");
    for (const auto &al : cachedAliases) {
        auto m = acDigitsRegex.match(al.pattern);
        if (m.hasMatch()) {
            accountPartyMap.insert(m.captured(0), al.mappedParty);
        }
    }

    auto normalizeBase = [](const QString &s) -> QString {
        QString res = s.toLower();
        res.remove(QRegularExpression(R"(\[.*?\])"));
        res.remove(QRegularExpression(R"(\(.*?\))"));
        res.remove(QRegularExpression(R"(^(?:m/s\.?|ms\.?|shri\b|sh\.|sh\b|smt\.|smt\b|mr\.|mr\b|mrs\.|mrs\b|c/o)\s*)", QRegularExpression::CaseInsensitiveOption));
        res.remove(QRegularExpression(R"(\b(limited|ltd|private|pvt|corporation|corp|company|co|llp|inc)\b)", QRegularExpression::CaseInsensitiveOption));
        res.replace(QRegularExpression(R"(\b(son of|s/o|s/0|d/o|w/o)\b)"), "s/o");
        res.replace(QRegularExpression(R"(\bindustries\b)"), "industry");
        res.replace(QRegularExpression(R"(\benterprises\b)"), "enterprise");
        res.replace(QRegularExpression(R"(\btraders\b)"), "trader");
        res.replace(QRegularExpression(R"(\bproducts\b)"), "product");
        res.replace(QRegularExpression(R"(\bfoods\b)"), "food");
        res.replace(QRegularExpression(R"(\bstores\b)"), "store");
        res.replace(QRegularExpression(R"(\band\b|\bant\b)"), "&");
        res.replace(QRegularExpression(R"(\bprotien\b)"), "protein");
        res.remove(QRegularExpression("[^a-z0-9& ]"));

        QStringList words = res.split(' ', Qt::SkipEmptyParts);
        QStringList joined;
        QString acro;
        for (const auto &w : words) {
            if (w.length() == 1 && w != "&") {
                acro += w;
            } else {
                if (!acro.isEmpty()) {
                    joined.append(acro);
                    acro.clear();
                }
                joined.append(w);
            }
        }
        if (!acro.isEmpty()) joined.append(acro);
        return joined.join(" ").trimmed();
    };

    auto phoneticKey = [&](const QString &s) -> QString {
        QString res = normalizeBase(s);
        res.replace("bh", "b").replace("kh", "k").replace("ph", "f").replace("dh", "d").replace("th", "t").replace("gh", "g").replace("sh", "s").replace("ch", "c");
        res.replace("ee", "i").replace("oo", "u").replace("aa", "a").replace("w", "v").replace("ie", "i").replace("ei", "i");
        res.replace("gg", "g").replace("ll", "l").replace("mm", "m").replace("nn", "n").replace("pp", "p").replace("rr", "r").replace("ss", "s").replace("tt", "t");
        res.remove(QRegularExpression("[^a-z0-9& ]"));

        QStringList words = res.split(' ', Qt::SkipEmptyParts);
        QStringList joined;
        QString acro;
        for (const auto &w : words) {
            if (w.length() == 1 && w != "&") {
                acro += w;
            } else {
                if (!acro.isEmpty()) {
                    joined.append(acro);
                    acro.clear();
                }
                joined.append(w);
            }
        }
        if (!acro.isEmpty()) joined.append(acro);
        return joined.join(" ").trimmed();
    };

    auto spacelessKey = [&](const QString &s) -> QString {
        QString b = normalizeBase(s);
        b.remove(QRegularExpression("[^a-z0-9]"));
        return b;
    };

    QVariantList partyRows = DatabaseManager::instance().executeQuery("SELECT name, alias, bank_account FROM parties;");
    for (const auto &pr : partyRows) {
        auto map = pr.toMap();
        QString name = map.value("name").toString().trimmed();
        QString alias = map.value("alias").toString().trimmed();
        QString bankAcc = map.value("bank_account").toString().trimmed();
        if (name.isEmpty()) continue;

        exactPartyMap.insert(name.toLower(), name);
        if (!alias.isEmpty()) {
            exactPartyMap.insert(alias.toLower(), name);
        }

        QString baseName = normalizeBase(name);
        if (!baseName.isEmpty() && !basePartyMap.contains(baseName)) {
            basePartyMap.insert(baseName, name);
        }

        QString phon = phoneticKey(name);
        if (!phon.isEmpty() && !phoneticPartyMap.contains(phon)) {
            phoneticPartyMap.insert(phon, name);
        }

        QString sk = spacelessKey(name);
        if (!sk.isEmpty() && !spacelessPartyMap.contains(sk)) {
            spacelessPartyMap.insert(sk, name);
        }

        // Index bank account numbers (9-18 digits)
        if (!bankAcc.isEmpty()) {
            auto it = acDigitsRegex.globalMatch(bankAcc);
            while (it.hasNext()) {
                accountPartyMap.insert(it.next().captured(0), name);
            }
            QString digitsOnly = bankAcc;
            digitsOnly.remove(QRegularExpression(R"(\D)"));
            if (digitsOnly.length() >= 9 && digitsOnly.length() <= 18) {
                accountPartyMap.insert(digitsOnly, name);
            }
        }

        CachedParty cp;
        cp.name = name;
        cp.alias = alias;
        cp.cleanName = baseName;
        cp.cleanAlias = normalizeBase(alias);

        QStringList words = (cp.cleanName + " " + cp.cleanAlias).split(' ', Qt::SkipEmptyParts);
        for (const auto &w : words) {
            if (w.length() >= 3 || w == "&" || w.contains('&')) {
                if (!cp.tokens.contains(w)) cp.tokens.append(w);
            }
        }
        cachedParties.append(cp);
    }

    // 3. Pre-load all existing transactions for this bank account
    QVector<CachedTxn> existingBankTxns;
    QVariantList dbTxnRows = DatabaseManager::instance().executeQuery(
        "SELECT id, voucher_no, voucher_type, voucher_date, bank_date, opposing_account, party_name, narration, amount, dr_cr "
        "FROM transactions "
        "WHERE party_name = ? OR opposing_account = ?;",
        {m_bankLedgerName, m_bankLedgerName}
    );

    for (const auto &tr : dbTxnRows) {
        auto map = tr.toMap();
        CachedTxn ct;
        ct.id = map.value("id").toLongLong();
        ct.voucherNo = map.value("voucher_no").toString();
        ct.voucherType = map.value("voucher_type").toString();
        ct.date = map.value("bank_date").toString();
        if (ct.date.isEmpty()) ct.date = map.value("voucher_date").toString();
        ct.opposingAccount = map.value("opposing_account").toString();
        ct.partyName = map.value("party_name").toString();
        ct.amount = map.value("amount").toDouble();
        ct.drCr = map.value("dr_cr").toString();
        ct.matched = false;
        existingBankTxns.append(ct);
    }

    // Fast in-memory party matcher lambda
    auto matchPartyFast = [&](const QString &extractedName, const QString &narration, QString &outConfidence) -> QString {
        if (extractedName.isEmpty() && narration.isEmpty()) {
            outConfidence = "UNMATCHED";
            return QString();
        }

        // A. Learned Aliases
        for (const auto &al : cachedAliases) {
            if (!narration.isEmpty() && narration.contains(al.pattern, Qt::CaseInsensitive)) {
                outConfidence = "ALIAS_MATCH";
                return al.mappedParty;
            }
            if (!extractedName.isEmpty() && extractedName.compare(al.pattern, Qt::CaseInsensitive) == 0) {
                outConfidence = "ALIAS_MATCH";
                return al.mappedParty;
            }
        }

        // B. Exact & Normalized Base Maps
        if (!extractedName.isEmpty()) {
            QString cleanExtracted = extractedName.trimmed().toLower();
            if (exactPartyMap.contains(cleanExtracted)) {
                outConfidence = "HIGH";
                return exactPartyMap.value(cleanExtracted);
            }

            QString baseExtracted = normalizeBase(extractedName);
            if (!baseExtracted.isEmpty() && basePartyMap.contains(baseExtracted)) {
                outConfidence = "HIGH";
                return basePartyMap.value(baseExtracted);
            }

            QString phonExtracted = phoneticKey(extractedName);
            if (!phonExtracted.isEmpty() && phoneticPartyMap.contains(phonExtracted)) {
                outConfidence = "HIGH";
                return phoneticPartyMap.value(phonExtracted);
            }

            QString spaceExtracted = spacelessKey(extractedName);
            if (!spaceExtracted.isEmpty() && spacelessPartyMap.contains(spaceExtracted)) {
                outConfidence = "HIGH";
                return spacelessPartyMap.value(spaceExtracted);
            }

            // C. Normalized substring & token search
            if (baseExtracted.length() >= 3) {
                for (const auto &cp : cachedParties) {
                    if (cp.cleanName == baseExtracted || cp.cleanAlias == baseExtracted ||
                        cp.cleanName.contains(baseExtracted) || (cp.cleanAlias.length() >= 3 && cp.cleanAlias.contains(baseExtracted)) ||
                        (baseExtracted.length() >= 4 && cp.cleanName.length() >= 4 && baseExtracted.contains(cp.cleanName))) {
                        outConfidence = "HIGH";
                        return cp.name;
                    }
                }

                QStringList tokens = baseExtracted.split(' ', Qt::SkipEmptyParts);
                QStringList sigTokens;
                for (const auto &t : tokens) {
                    if (t.length() >= 3 || t == "&" || t.contains('&')) sigTokens.append(t);
                }

                if (!sigTokens.isEmpty()) {
                    for (const auto &cp : cachedParties) {
                        bool allFound = true;
                        for (const auto &st : sigTokens) {
                            if (!cp.cleanName.contains(st) && !cp.cleanAlias.contains(st)) {
                                allFound = false;
                                break;
                            }
                        }
                        if (allFound) {
                            outConfidence = "HIGH";
                            return cp.name;
                        }
                    }
                }
            }
        }

        // D. Category Fallbacks
        QString uNarr = narration.toUpper();
        if (uNarr.startsWith("SC ") || uNarr.contains(" SC") || uNarr.contains("SERVICE CHARGE") ||
            uNarr.contains("MORTGAGE CHARGES") || uNarr.contains("TRANSACTION CHARGES") ||
            uNarr.contains("SMS CHARGES") || uNarr.contains("SMS ALERT") || uNarr.contains("FOLIO AMT") ||
            uNarr.contains("PENALTY") || uNarr.contains("PASSHEETCHARGES") || uNarr.contains("PASSHEET") ||
            uNarr.contains("DOC CHGS") || uNarr.contains("PROC CHGS") || uNarr.contains("RTGS 00.00 TO") || 
            uNarr.contains("COMMERCIAL WITH SCORE") || uNarr.contains("RTN SC") || uNarr.contains("CHQ RETURN") ||
            uNarr.contains("AUDIT FEE") || uNarr.contains("STOCK AUDIT")) {
            outConfidence = "HIGH";
            for (const auto &cp : cachedParties) {
                if (cp.name.contains("Bank Charges", Qt::CaseInsensitive)) return cp.name;
            }
            return "Bank Charges";
        }

        if (uNarr.contains("QOS PERIOD") || uNarr.contains("INTEREST") ||
            uNarr.contains("CC INT") || uNarr.contains("OD INT") ||
            uNarr.contains("CASA DEBIT INTEREST") || uNarr.contains("INTEREST CAPITALIZED")) {
            outConfidence = "HIGH";
            for (const auto &cp : cachedParties) {
                if (cp.name.contains("Interest Bank", Qt::CaseInsensitive) || cp.name.contains("Bank Interest", Qt::CaseInsensitive)) return cp.name;
            }
            return "Interest Bank A/c";
        }

        if (uNarr.contains("DRAWDOWN FROM CASA") || uNarr.contains("TD PAYIN") || uNarr.contains("CASAXFER") ||
            (!m_header.firmName.isEmpty() && uNarr.contains(m_header.firmName.toUpper()))) {
            outConfidence = "HIGH";
            if (!m_header.firmName.isEmpty()) {
                for (const auto &cp : cachedParties) {
                    if (cp.name.contains(m_header.firmName, Qt::CaseInsensitive)) return cp.name;
                }
                return m_header.firmName;
            }
            return "Internal Transfer";
        }

        // E. Account Number Fallback Matching (9 to 18 digits)
        if (!accountPartyMap.isEmpty()) {
            static QRegularExpression acRegex(R"(\b\d{9,18}\b)");
            auto it = acRegex.globalMatch(narration);
            while (it.hasNext()) {
                QString acSeq = it.next().captured(0);
                if (accountPartyMap.contains(acSeq)) {
                    outConfidence = "ACCOUNT_MATCH";
                    return accountPartyMap.value(acSeq);
                }
            }

            for (auto acIt = accountPartyMap.constBegin(); acIt != accountPartyMap.constEnd(); ++acIt) {
                if (acIt.key().length() >= 9 && narration.contains(acIt.key())) {
                    outConfidence = "ACCOUNT_MATCH";
                    return acIt.value();
                }
            }
        }

        outConfidence = "UNMATCHED";
        return QString();
    };

    // 4. Match all transactions in memory
    for (auto &txn : txns) {
        QString confidence = "UNMATCHED";
        QString matchedParty = matchPartyFast(txn.extractedParty, txn.rawNarration, confidence);

        double amt = txn.deposit > 0.001 ? txn.deposit : txn.withdrawal;
        bool isDeposit = txn.deposit > 0.001;

        // Guaranteed Double-Entry Duplicate Check:
        // Match exact Bank Account, exact Transaction Date, exact Amount, and exact Double-Entry Leg
        CachedTxn *foundDup = nullptr;

        for (auto &dbTxn : existingBankTxns) {
            if (dbTxn.matched) continue;

            if (std::abs(dbTxn.amount - amt) >= 0.01) continue;
            if (dbTxn.date != txn.isoDate) continue;

            bool legMatches = false;
            if (isDeposit) {
                // Deposit in bank: Bank is Debited (Dr), Opposing account is Credited (Cr)
                if ((dbTxn.partyName == m_bankLedgerName && dbTxn.drCr == "Dr") ||
                    (dbTxn.opposingAccount == m_bankLedgerName && dbTxn.drCr == "Cr")) {
                    legMatches = true;
                }
            } else {
                // Withdrawal from bank: Bank is Credited (Cr), Opposing account is Debited (Dr)
                if ((dbTxn.partyName == m_bankLedgerName && dbTxn.drCr == "Cr") ||
                    (dbTxn.opposingAccount == m_bankLedgerName && dbTxn.drCr == "Dr")) {
                    legMatches = true;
                }
            }

            if (legMatches) {
                foundDup = &dbTxn;
                break;
            }
        }

        if (foundDup) {
            foundDup->matched = true; // 1-to-1 match against existing DB records
            txn.isDuplicate = true;
            txn.isSelected = false; // Never auto-select duplicates for posting
            txn.confidence = "DUPLICATE";

            // Recover authentic opposing party ledger from the existing database voucher
            QString existingOpposing = foundDup->opposingAccount;
            if (existingOpposing.isEmpty() || existingOpposing == m_bankLedgerName) {
                existingOpposing = foundDup->partyName;
            }
            if (!existingOpposing.isEmpty() && existingOpposing != m_bankLedgerName) {
                matchedParty = existingOpposing;
            }

            if (isDeposit) {
                txn.suggestedDrAccount = m_bankLedgerName;
                txn.suggestedCrAccount = matchedParty;
                txn.voucherType = "Cheque Receipt";
            } else {
                txn.suggestedDrAccount = matchedParty;
                txn.suggestedCrAccount = m_bankLedgerName;
                txn.voucherType = "Cheque Payment";
            }
        } else {
            txn.isDuplicate = false;
            txn.confidence = confidence;
            txn.isSelected = !matchedParty.isEmpty();

            if (isDeposit) {
                txn.suggestedDrAccount = m_bankLedgerName;
                txn.suggestedCrAccount = matchedParty;
                txn.voucherType = "Cheque Receipt";
            } else {
                txn.suggestedDrAccount = matchedParty;
                txn.suggestedCrAccount = m_bankLedgerName;
                txn.voucherType = "Cheque Payment";
            }
        }
    }
}

bool BankStatementController::loadStatement(const QString &filePath) {
    m_isLoading = true;
    m_isError = false;
    m_statusMessage = "Loading and parsing bank statement...";
    emit loadingChanged();
    emit statusChanged();

    QString cleanPath = filePath;
    if (cleanPath.startsWith("file://")) {
        cleanPath = QUrl(cleanPath).toLocalFile();
    }

    m_statementPath = cleanPath;

    QString ext = QFileInfo(cleanPath).suffix().toLower();
    QVector<CanaraBankTransaction> transactions;
    QString errorMsg;
    bool ok = false;
    QString sourceFormat = "PDF";

    if (ext == "xls" || ext == "xlsx" || ext == "csv" || ext == "txt" || ext == "tsv") {
        sourceFormat = ext.toUpper();
        BankStatementMetadata meta;
        QVector<BankStatementTransaction> excelTxns;
        ok = BankStatementExcelParser::parseFile(cleanPath, meta, excelTxns, errorMsg);
        if (ok) {
            m_header.accountNo = meta.accountNumber;
            m_header.firmName = meta.accountName;
            m_header.ifscCode = meta.ifscCode;
            m_header.branchName = meta.branchName;
            m_header.fromDate = meta.periodFrom;
            m_header.toDate = meta.periodTo;
            m_header.openingBalance = meta.openingBalance;

            int idx = 1;
            CanaraBankStatementParser parser;
            for (const auto &et : excelTxns) {
                CanaraBankTransaction ct;
                ct.index = idx++;
                ct.dateStr = et.rawDate;
                ct.isoDate = et.date;
                ct.utrRef = et.txnId;
                ct.withdrawal = et.withdrawal;
                ct.deposit = et.deposit;
                ct.balance = et.balance;
                ct.rawNarration = et.remarks;
                parser.classifyAndExtractParty(ct);
                transactions.append(ct);
            }
        }
    } else {
        CanaraBankStatementParser parser;
        ok = parser.parsePdf(cleanPath, m_header, transactions, errorMsg);
    }

    if (!ok) {
        m_isLoading = false;
        m_isError = true;
        m_statusMessage = errorMsg;
        emit loadingChanged();
        emit statusChanged();
        return false;
    }

    detectBankLedger();
    matchPartiesAndDuplicates(transactions);

    m_rowsModel.setRows(transactions);
    emit statementLoaded();

    recalculateTotalsAndCounts();

    m_isLoading = false;
    m_isError = false;
    m_statusMessage = QString("Successfully parsed %1 transactions from %2 statement (%3).")
        .arg(transactions.size())
        .arg(m_header.firmName.isEmpty() ? "Bank" : m_header.firmName)
        .arg(sourceFormat);
    emit loadingChanged();
    emit statusChanged();

    return true;
}

QString BankStatementController::browseStatementFile() {
    QString initialDir = m_statementPath.isEmpty() ? QDir::currentPath() : QFileInfo(m_statementPath).absolutePath();
    if (initialDir.isEmpty() || !QDir(initialDir).exists()) {
        initialDir = QDir::homePath();
    }
    QString path = QFileDialog::getOpenFileName(
        nullptr,
        "Select Bank Statement File (Excel, CSV, or PDF)",
        initialDir,
        "Bank Statement Files (*.xls *.xlsx *.csv *.pdf);;Excel Files (*.xls *.xlsx);;CSV Files (*.csv);;PDF Statements (*.pdf);;All Files (*)"
    );
    if (!path.isEmpty()) {
        loadStatement(path);
    }
    return path;
}

void BankStatementController::reloadFromCurrentStatement() {
    if (!m_statementPath.isEmpty()) {
        loadStatement(m_statementPath);
    }
}

void BankStatementController::onModelSelectionChanged() {
    int selCount = 0;
    double selW = 0.0;
    double selD = 0.0;

    const auto &rows = m_rowsModel.rows();
    for (const auto &r : rows) {
        if (r.isSelected) {
            selCount++;
            selW += r.withdrawal;
            selD += r.deposit;
        }
    }

    m_selectedCount = selCount;
    m_selectedWithdrawals = FinancialMathService::instance().round2(selW);
    m_selectedDeposits = FinancialMathService::instance().round2(selD);

    emit selectionChanged();
}

void BankStatementController::recalculateTotalsAndCounts() {
    double totalW = 0.0;
    double totalD = 0.0;
    int unmat = 0;
    int dups = 0;

    const auto &rows = m_rowsModel.rows();
    for (const auto &r : rows) {
        totalW += r.withdrawal;
        totalD += r.deposit;
        if (r.suggestedDrAccount.isEmpty() || r.suggestedCrAccount.isEmpty() || r.confidence == "UNMATCHED") {
            unmat++;
        }
        if (r.isDuplicate) {
            dups++;
        }
    }

    m_totalWithdrawals = FinancialMathService::instance().round2(totalW);
    m_totalDeposits = FinancialMathService::instance().round2(totalD);
    m_unmatchedCount = unmat;
    m_duplicateCount = dups;

    emit totalsChanged();
    emit countsChanged();

    onModelSelectionChanged();
}

void BankStatementController::saveAlias(const QString &narration, const QString &mappedParty) {
    if (narration.trimmed().isEmpty() || mappedParty.trimmed().isEmpty()) return;

    QString cleanMapped = mappedParty.trimmed();

    // 1. Extract core keyword from narration for robust matching
    QString pattern = narration.trimmed();
    QStringList parts = pattern.split('-');
    if (parts.size() >= 4) {
        static QRegularExpression ifscRegex("^[A-Z]{4}[0-9A-Z]{7}$");
        if (ifscRegex.match(parts[1].trimmed()).hasMatch()) {
            pattern = parts[2].trimmed();
        } else if (parts[3].contains("MAHADEV", Qt::CaseInsensitive)) {
            pattern = parts[2].trimmed();
        } else {
            pattern = parts[3].trimmed();
        }
        pattern.remove(QRegularExpression("[/\\\\-].*"));
    }

    if (!pattern.isEmpty() && pattern.length() >= 3) {
        DatabaseManager::instance().executeNonQuery(
            "INSERT OR REPLACE INTO bank_narration_aliases (bank_code, narration_pattern, mapped_party_name, created_at) "
            "VALUES ('CNRB', ?, ?, ?);",
            {pattern.trimmed(), cleanMapped, QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss")}
        );
    }

    // 2. Extract account numbers (9 to 18 digits) from narration and save account alias + enrich parties table
    static QRegularExpression acRegex(R"(\b\d{9,18}\b)");
    auto acMatch = acRegex.globalMatch(narration);
    while (acMatch.hasNext()) {
        QString acNo = acMatch.next().captured(0);
        if (acNo != m_header.accountNo && !acNo.isEmpty()) {
            DatabaseManager::instance().executeNonQuery(
                "INSERT OR REPLACE INTO bank_narration_aliases (bank_code, narration_pattern, mapped_party_name, created_at) "
                "VALUES ('CNRB', ?, ?, ?);",
                {acNo, cleanMapped, QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss")}
            );
            DatabaseManager::instance().executeNonQuery(
                "UPDATE parties SET bank_account = ? WHERE name = ? AND (bank_account IS NULL OR bank_account = '');",
                {acNo, cleanMapped}
            );
        }
    }
}

QVariantMap BankStatementController::postSelectedVouchers() {
    QVariantMap result;
    result["success"] = false;

    const auto &rows = m_rowsModel.rows();
    QVector<CanaraBankTransaction> toPost;

    for (const auto &r : rows) {
        if (r.isSelected) {
            if (r.suggestedDrAccount.isEmpty() || r.suggestedCrAccount.isEmpty()) {
                result["message"] = QString("Row %1 has missing Ledger mapping. Please select both Dr and Cr accounts.").arg(r.index);
                return result;
            }
            toPost.append(r);
        }
    }

    if (toPost.isEmpty()) {
        result["message"] = "No transactions selected for posting.";
        return result;
    }

    DatabaseManager::instance().beginTransaction();

    VouchersModel vModel;
    int postedCount = 0;
    double totalPostedAmt = 0.0;

    for (const auto &r : toPost) {
        double amt = r.deposit > 0.001 ? r.deposit : r.withdrawal;
        QString vchType = r.voucherType;
        QString drParty = r.suggestedDrAccount.trimmed();
        QString crParty = r.suggestedCrAccount.trimmed();
        QString chqNo = r.utrRef;
        QString narr = r.rawNarration.trimmed();
        QString dt = r.isoDate;

        // Financial Year Resolution
        FiscalYearInfo fy = FiscalYearHelper::getFiscalYearForDate(dt);
        int fyId = 1;
        QString fyLabel = fy.name;
        QVariant fyIdVar = DatabaseManager::instance().executeScalar(
            "SELECT id FROM financial_years WHERE year_name = ? LIMIT 1;",
            {fy.name}
        );
        if (fyIdVar.isValid() && !fyIdVar.isNull()) {
            fyId = fyIdVar.toInt();
        } else {
            QVariant anyFy = DatabaseManager::instance().executeScalar("SELECT id FROM financial_years WHERE is_active = 1 LIMIT 1;");
            if (anyFy.isValid() && !anyFy.isNull()) fyId = anyFy.toInt();
        }

        QString vchNo = vModel.get_next_voucher_no(vchType, fyLabel);
        QString fullNarr = chqNo.isEmpty() ? "" : ("Ref/UTR: " + chqNo);
        if (!narr.isEmpty()) {
            fullNarr += fullNarr.isEmpty() ? narr : (" | " + narr);
        }

        // Get Party ID
        QVariant partyRow = DatabaseManager::instance().executeScalar(
            "SELECT id FROM parties WHERE name = ? COLLATE NOCASE LIMIT 1;", 
            {vchType == "Cheque Receipt" ? crParty : drParty}
        );
        int partyId = partyRow.isValid() ? partyRow.toInt() : 1;

        bool ok = DatabaseManager::instance().executeNonQuery(
            "INSERT INTO vouchers (fy_id, financial_year, voucher_no, instrument_no, voucher_date, voucher_type, legacy_type, party_id, ledger_id, party_name, account_type, amount, narration) "
            "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);",
            {fyId, fyLabel, vchNo, chqNo, dt, vchType, vchType, partyId, partyId, drParty, crParty, amt, fullNarr}
        );

        if (!ok) {
            DatabaseManager::instance().rollback();
            result["message"] = QString("Failed to insert voucher for row %1 (%2)").arg(r.index).arg(r.rawNarration);
            return result;
        }

        // Double-entry debit leg
        DatabaseManager::instance().executeNonQuery(
            "INSERT INTO transactions (fy_id, financial_year, voucher_no, voucher_date, voucher_type, trans_type, party_id, party_name, opposing_account, dr_cr, amount, narration) "
            "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, 'Dr', ?, ?);",
            {fyId, fyLabel, vchNo, dt, vchType, vchType, partyId, drParty, crParty, amt, fullNarr}
        );

        // Double-entry credit leg
        DatabaseManager::instance().executeNonQuery(
            "INSERT INTO transactions (fy_id, financial_year, voucher_no, voucher_date, voucher_type, trans_type, party_id, party_name, opposing_account, dr_cr, amount, narration) "
            "VALUES (?, ?, ?, ?, ?, ?, 0, ?, ?, 'Cr', ?, ?);",
            {fyId, fyLabel, vchNo, dt, vchType, vchType, crParty, drParty, amt, fullNarr}
        );

        // Remember alias for confirmed party
        if (!r.extractedParty.isEmpty() && !r.confidence.contains("ALIAS")) {
            saveAlias(r.extractedParty, vchType == "Cheque Receipt" ? crParty : drParty);
        }

        postedCount++;
        totalPostedAmt += amt;
    }

    DatabaseManager::instance().commit();
    vModel.reload_data();

    // Reload rows to refresh duplicate status
    reloadFromCurrentStatement();

    result["success"] = true;
    result["count"] = postedCount;
    result["totalAmount"] = totalPostedAmt;
    result["message"] = QString("Successfully posted %1 bank vouchers totaling ₹%2 into the database.")
                            .arg(postedCount)
                            .arg(FinancialMathService::instance().formatInr(totalPostedAmt));

    emit vouchersPostedSuccess(postedCount, totalPostedAmt);
    return result;
}

QStringList BankStatementController::getAvailableBankLedgers() const {
    QStringList list;
    QVariantList rows = DatabaseManager::instance().executeQuery(
        "SELECT name FROM parties WHERE group_name LIKE '%Bank%' OR group_name LIKE '%Secured Loans%' ORDER BY name ASC;"
    );
    for (const auto &r : rows) {
        list.append(r.toMap().value("name").toString());
    }
    return list;
}

QStringList BankStatementController::getAllPartyLedgers() const {
    QStringList list;
    QVariantList rows = DatabaseManager::instance().executeQuery(
        "SELECT name FROM parties ORDER BY name ASC;"
    );
    for (const auto &r : rows) {
        list.append(r.toMap().value("name").toString());
    }
    return list;
}

QVariantList BankStatementController::searchPartyLedgers(const QString &query) const {
    QString q = query.trimmed();
    if (q.isEmpty()) {
        return DatabaseManager::instance().executeQuery(
            "SELECT id, legacy_id, name, group_name, party_type, phone, city, gstin, opening_balance, balance_type "
            "FROM parties ORDER BY name COLLATE NOCASE ASC LIMIT 50;"
        );
    }
    QString pattern = q;
    pattern.replace(QChar(0x00A0), '%');
    pattern.replace(' ', '%');
    QString wildcard = "%" + pattern + "%";

    return DatabaseManager::instance().executeQuery(
        "SELECT id, legacy_id, name, group_name, party_type, phone, city, gstin, opening_balance, balance_type "
        "FROM parties WHERE name LIKE ? OR alias LIKE ? OR phone LIKE ? OR city LIKE ? OR group_name LIKE ? "
        "ORDER BY name COLLATE NOCASE ASC LIMIT 50;",
        {wildcard, wildcard, wildcard, wildcard, wildcard}
    );
}

