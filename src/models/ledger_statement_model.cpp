#include "ledger_statement_model.h"
#include "../database_manager.h"
#include "../engine/accounting_engine.h"
#include "../engine/fiscal_year_helper.h"
#include <QDate>
#include <algorithm>
#include <cmath>

static std::pair<QString, QString> parseDateIsoAndFmt(const QString& dStr) {
    if (dStr.trimmed().isEmpty()) return {"9999-12-31", ""};
    QString s = dStr.trimmed();
    if (s.contains('-')) {
        QStringList p = s.split('-');
        if (p.size() == 3) {
            if (p[0].length() == 4) {
                int y = p[0].toInt();
                int m = p[1].toInt();
                int d = p[2].toInt();
                return {
                    QString("%1-%2-%3").arg(y, 4, 10, QChar('0')).arg(m, 2, 10, QChar('0')).arg(d, 2, 10, QChar('0')),
                    QString("%1-%2-%3").arg(d, 2, 10, QChar('0')).arg(m, 2, 10, QChar('0')).arg(y, 4, 10, QChar('0'))
                };
            } else if (p[2].length() == 4) {
                int d = p[0].toInt();
                int m = p[1].toInt();
                int y = p[2].toInt();
                return {
                    QString("%1-%2-%3").arg(y, 4, 10, QChar('0')).arg(m, 2, 10, QChar('0')).arg(d, 2, 10, QChar('0')),
                    QString("%1-%2-%3").arg(d, 2, 10, QChar('0')).arg(m, 2, 10, QChar('0')).arg(y, 4, 10, QChar('0'))
                };
            }
        }
    } else if (s.contains('/')) {
        QStringList p = s.split('/');
        if (p.size() == 3 && p[2].length() == 4) {
            int d = p[0].toInt();
            int m = p[1].toInt();
            int y = p[2].toInt();
            return {
                QString("%1-%2-%3").arg(y, 4, 10, QChar('0')).arg(m, 2, 10, QChar('0')).arg(d, 2, 10, QChar('0')),
                QString("%1-%2-%3").arg(d, 2, 10, QChar('0')).arg(m, 2, 10, QChar('0')).arg(y, 4, 10, QChar('0'))
            };
        }
    }
    return {s, s};
}

static QString computeFyForIso(const QString& isoDate, const QString& explicitFy = "") {
    if (!explicitFy.trimmed().isEmpty()) return explicitFy.trimmed();
    FiscalYearInfo fy = FiscalYearHelper::getFiscalYearForDate(isoDate);
    return fy.name;
}

// ======================== LedgerStatementSideModel ========================

LedgerStatementSideModel::LedgerStatementSideModel(QObject* parent)
    : QAbstractListModel(parent)
{
}

int LedgerStatementSideModel::rowCount(const QModelIndex& parent) const {
    if (parent.isValid()) return 0;
    return m_entries.size();
}

QVariant LedgerStatementSideModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || index.row() < 0 || index.row() >= m_entries.size()) {
        return QVariant();
    }

    const LedgerStatementEntry& e = m_entries.at(index.row());
    switch (role) {
    case IdRole: return e.id;
    case IsSelectedRole: return e.isSelected;
    case VIsoRole: return e.vIso;
    case VDateRole: return e.vDate;
    case RefNoRole: return e.refNo;
    case VoucherNoRole: return e.voucherNo;
    case InvoiceNoRole: return e.invoiceNo;
    case VoucherTypeRole: return e.voucherType;
    case LegacyTypeRole: return e.legacyType;
    case TransTypeRole: return e.transType;
    case ParticularsRole: return e.particulars;
    case AmountRole: return e.amount;
    case AmountFmtRole: return e.amountFmt;
    case FinancialYearRole: return e.financialYear;
    case SideRole: return e.side;
    default: return QVariant();
    }
}

QHash<int, QByteArray> LedgerStatementSideModel::roleNames() const {
    QHash<int, QByteArray> roles;
    roles[IdRole] = "id";
    roles[IsSelectedRole] = "isSelected";
    roles[VIsoRole] = "vIso";
    roles[VDateRole] = "vDate";
    roles[RefNoRole] = "refNo";
    roles[VoucherNoRole] = "voucher_no";
    roles[InvoiceNoRole] = "invoice_no";
    roles[VoucherTypeRole] = "voucher_type";
    roles[LegacyTypeRole] = "legacy_type";
    roles[TransTypeRole] = "trans_type";
    roles[ParticularsRole] = "particulars";
    roles[AmountRole] = "amount";
    roles[AmountFmtRole] = "amountFmt";
    roles[FinancialYearRole] = "financial_year";
    roles[SideRole] = "side";
    return roles;
}

void LedgerStatementSideModel::setEntries(const QVector<LedgerStatementEntry>& entries) {
    beginResetModel();
    m_entries = entries;
    endResetModel();
    emit countChanged();
    recalculateTotals();
}

void LedgerStatementSideModel::clear() {
    beginResetModel();
    m_entries.clear();
    endResetModel();
    emit countChanged();
    recalculateTotals();
}

void LedgerStatementSideModel::toggleSelection(int row) {
    if (row < 0 || row >= m_entries.size()) return;
    m_entries[row].isSelected = !m_entries[row].isSelected;
    QModelIndex idx = index(row);
    emit dataChanged(idx, idx, {IsSelectedRole});
    recalculateTotals();
}

void LedgerStatementSideModel::setSelection(int row, bool selected) {
    if (row < 0 || row >= m_entries.size()) return;
    if (m_entries[row].isSelected == selected) return;
    m_entries[row].isSelected = selected;
    QModelIndex idx = index(row);
    emit dataChanged(idx, idx, {IsSelectedRole});
    recalculateTotals();
}

void LedgerStatementSideModel::selectAll(bool selected) {
    if (m_entries.isEmpty()) return;
    for (int i = 0; i < m_entries.size(); ++i) {
        m_entries[i].isSelected = selected;
    }
    emit dataChanged(index(0), index(m_entries.size() - 1), {IsSelectedRole});
    recalculateTotals();
}

QVariantMap LedgerStatementSideModel::get(int row) const {
    if (row < 0 || row >= m_entries.size()) return QVariantMap();
    const LedgerStatementEntry& e = m_entries.at(row);
    QVariantMap m;
    m["id"] = e.id;
    m["isSelected"] = e.isSelected;
    m["vIso"] = e.vIso;
    m["vDate"] = e.vDate;
    m["refNo"] = e.refNo;
    m["voucherNo"] = e.voucherNo;
    m["voucher_no"] = e.voucherNo;
    m["invoiceNo"] = e.invoiceNo;
    m["invoice_no"] = e.invoiceNo;
    m["voucherType"] = e.voucherType;
    m["voucher_type"] = e.voucherType;
    m["legacyType"] = e.legacyType;
    m["legacy_type"] = e.legacyType;
    m["transType"] = e.transType;
    m["trans_type"] = e.transType;
    m["particulars"] = e.particulars;
    m["amount"] = e.amount;
    m["amountFmt"] = e.amountFmt;
    m["financialYear"] = e.financialYear;
    m["financial_year"] = e.financialYear;
    m["side"] = e.side;
    return m;
}

void LedgerStatementSideModel::recalculateTotals() {
    double tot = 0.0;
    double selTot = 0.0;
    for (const auto& e : m_entries) {
        tot += e.amount;
        if (e.isSelected) selTot += e.amount;
    }
    m_totalAmount = tot;
    m_totalAmountFmt = AccountingEngine::formatIndianCurrency(tot, true);
    m_selectedTotal = selTot;
    m_selectedTotalFmt = AccountingEngine::formatIndianCurrency(selTot, true);
    emit totalsChanged();
}

// ======================== LedgerStatementController ========================

LedgerStatementController::LedgerStatementController(QObject* parent)
    : QObject(parent)
{
    connect(&m_drModel, &LedgerStatementSideModel::totalsChanged, this, &LedgerStatementController::onSideTotalsChanged);
    connect(&m_crModel, &LedgerStatementSideModel::totalsChanged, this, &LedgerStatementController::onSideTotalsChanged);
}

void LedgerStatementController::setCurrentPartyName(const QString& name) {
    if (m_currentPartyName == name) return;
    m_currentPartyName = name.trimmed();
    emit currentPartyNameChanged();
    reloadData();
}

double LedgerStatementController::netBalance() const {
    return std::abs(m_drModel.totalAmount() - m_crModel.totalAmount());
}

QString LedgerStatementController::netBalanceFmt() const {
    return AccountingEngine::formatIndianCurrency(netBalance(), true);
}

QString LedgerStatementController::netBalanceType() const {
    double dr = m_drModel.totalAmount();
    double cr = m_crModel.totalAmount();
    if (std::abs(dr - cr) < 0.001) return "Nil Balance";
    if (dr > cr) return "Dr (Debit Balance)";
    return "Cr (Credit Balance)";
}

void LedgerStatementController::onSideTotalsChanged() {
    emit statementTotalsChanged();
}

void LedgerStatementController::loadPartyStatement(const QString& partyName, const QString& fromDate, const QString& toDate) {
    m_currentPartyName = partyName.trimmed();
    FiscalYearInfo fy = FiscalYearHelper::getActiveFiscalYear();
    QString fIso = FiscalYearHelper::normalizeToIso(fromDate);
    QString tIso = FiscalYearHelper::normalizeToIso(toDate);
    FiscalYearHelper::clampDateRangeToFiscalYear(fIso, tIso, fy);
    m_fromDate = fIso;
    m_toDate = tIso;
    emit currentPartyNameChanged();
    reloadData();
}

void LedgerStatementController::applyDateFilter(const QString& fromDate, const QString& toDate) {
    FiscalYearInfo fy = FiscalYearHelper::getActiveFiscalYear();
    QString fIso = FiscalYearHelper::normalizeToIso(fromDate);
    QString tIso = FiscalYearHelper::normalizeToIso(toDate);
    FiscalYearHelper::clampDateRangeToFiscalYear(fIso, tIso, fy);
    m_fromDate = fIso;
    m_toDate = tIso;
    reloadData();
}

void LedgerStatementController::refresh() {
    reloadData();
}

QVariantList LedgerStatementController::searchParties(const QString& query) const {
    QString q = query.trimmed().toLower();
    if (q.isEmpty()) {
        return DatabaseManager::instance().executeQuery(
            "SELECT id, legacy_id, name, group_name, party_type, phone, city, gstin, opening_balance, balance_type "
            "FROM parties ORDER BY name COLLATE NOCASE ASC LIMIT 50;"
        );
    }

    static const QRegularExpression splitRegex(QStringLiteral("[\\s\\-\\/\\.\\[\\]\\(\\)\\,\\&]+"));
    QStringList queryTokens = q.split(splitRegex, Qt::SkipEmptyParts);
    if (queryTokens.isEmpty()) {
        return DatabaseManager::instance().executeQuery(
            "SELECT id, legacy_id, name, group_name, party_type, phone, city, gstin, opening_balance, balance_type "
            "FROM parties ORDER BY name COLLATE NOCASE ASC LIMIT 50;"
        );
    }

    QVariantList allParties = DatabaseManager::instance().executeQuery(
        "SELECT id, legacy_id, name, group_name, party_type, phone, city, gstin, opening_balance, balance_type "
        "FROM parties;"
    );

    struct RankedItem {
        QVariantMap data;
        int tier;          // 0: 1st word prefix, 1: 2nd word prefix, 2: 3rd+ word prefix, 3: City prefix, 4: Group prefix
        int wordIndex;     // Index of matching word in name
        int nameLength;
        QString name;
    };

    QList<RankedItem> matches;
    matches.reserve(allParties.size());

    for (const QVariant& rowVar : allParties) {
        QVariantMap row = rowVar.toMap();
        QString name = row.value("name").toString().trimmed();
        QString city = row.value("city").toString().trimmed();
        QString group = row.value("group_name").toString().trimmed();

        QString nameLower = name.toLower();
        QString cityLower = city.toLower();
        QString groupLower = group.toLower();

        // Extract bracketed city/location text if any (e.g. [Sirsa])
        QString bracketLocation;
        int bStart = nameLower.indexOf('[');
        int bEnd = nameLower.indexOf(']', bStart);
        if (bStart != -1 && bEnd > bStart) {
            bracketLocation = nameLower.mid(bStart + 1, bEnd - bStart - 1).trimmed();
        }

        // Clean name without brackets for main word matching
        QString cleanNamePart = (bStart != -1) ? nameLower.left(bStart).trimmed() : nameLower;
        QStringList nameWords = cleanNamePart.split(splitRegex, Qt::SkipEmptyParts);
        QStringList locationWords = bracketLocation.split(splitRegex, Qt::SkipEmptyParts);
        if (!cityLower.isEmpty()) {
            locationWords.append(cityLower.split(splitRegex, Qt::SkipEmptyParts));
        }
        QStringList groupWords = groupLower.split(splitRegex, Qt::SkipEmptyParts);

        // Check matching of all query tokens
        bool allTokensMatched = true;
        int bestTier = 999;
        int bestWordIdx = 999;

        for (int t = 0; t < queryTokens.size(); ++t) {
            const QString& tok = queryTokens.at(t);
            bool tokenFound = false;

            // 1. Check Name words
            for (int w = 0; w < nameWords.size(); ++w) {
                if (nameWords.at(w).startsWith(tok)) {
                    tokenFound = true;
                    int curTier = (w == 0) ? 0 : ((w == 1) ? 1 : 2);
                    if (curTier < bestTier) {
                        bestTier = curTier;
                        bestWordIdx = w;
                    }
                    break;
                }
            }

            // 2. Check Bracket Location / City words
            if (!tokenFound) {
                for (int lw = 0; lw < locationWords.size(); ++lw) {
                    if (locationWords.at(lw).startsWith(tok)) {
                        tokenFound = true;
                        int curTier = 3;
                        if (curTier < bestTier) {
                            bestTier = curTier;
                            bestWordIdx = 100 + lw;
                        }
                        break;
                    }
                }
            }

            // 3. Check Group words
            if (!tokenFound) {
                for (int gw = 0; gw < groupWords.size(); ++gw) {
                    if (groupWords.at(gw).startsWith(tok)) {
                        tokenFound = true;
                        int curTier = 4;
                        if (curTier < bestTier) {
                            bestTier = curTier;
                            bestWordIdx = 200 + gw;
                        }
                        break;
                    }
                }
            }

            if (!tokenFound) {
                allTokensMatched = false;
                break;
            }
        }

        if (allTokensMatched && bestTier < 999) {
            RankedItem item;
            item.data = row;
            item.tier = bestTier;
            item.wordIndex = bestWordIdx;
            item.nameLength = name.length();
            item.name = name;
            matches.append(item);
        }
    }

    // Sort matching results: Tier -> WordIndex -> NameLength -> Name (A-Z)
    std::sort(matches.begin(), matches.end(), [](const RankedItem& a, const RankedItem& b) {
        if (a.tier != b.tier) {
            return a.tier < b.tier;
        }
        if (a.wordIndex != b.wordIndex) {
            return a.wordIndex < b.wordIndex;
        }
        if (a.nameLength != b.nameLength) {
            return a.nameLength < b.nameLength;
        }
        return QString::compare(a.name, b.name, Qt::CaseInsensitive) < 0;
    });

    QVariantList results;
    int limit = qMin(matches.size(), 50);
    for (int i = 0; i < limit; ++i) {
        results.append(matches.at(i).data);
    }

    return results;
}

void LedgerStatementController::reloadData() {
    m_rawDrEntries.clear();
    m_rawCrEntries.clear();

    QString cleanName = m_currentPartyName.trimmed();
    if (cleanName.isEmpty()) {
        m_drModel.clear();
        m_crModel.clear();
        emit statementTotalsChanged();
        emit statementLoaded();
        return;
    }

    PartitionedLedgerData partData = FiscalYearHelper::partitionPartyTransactions(cleanName, m_fromDate, m_toDate);
    m_fromDate = partData.effectiveFromDate;
    m_toDate = partData.effectiveToDate;
    m_rawDrEntries = partData.drEntries;
    m_rawCrEntries = partData.crEntries;

    m_drModel.setEntries(m_rawDrEntries);
    m_crModel.setEntries(m_rawCrEntries);
    emit statementTotalsChanged();
    emit statementLoaded();
}

void LedgerStatementController::applyFilterInternal() {
    reloadData();
}
