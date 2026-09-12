#include "bank_statement_controller.h"
#include "../database_manager.h"
#include "../services/financial_math_service.h"
#include "../services/accounting_date_service.h"
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
    return m_rows.size();
}

QVariant BankStatementRowsModel::data(const QModelIndex &index, int role) const {
    if (!index.isValid() || index.row() < 0 || index.row() >= m_rows.size()) {
        return QVariant();
    }

    const auto &r = m_rows.at(index.row());
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
    if (!index.isValid() || index.row() < 0 || index.row() >= m_rows.size()) {
        return false;
    }

    auto &r = m_rows[index.row()];
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

void BankStatementRowsModel::setRows(const QVector<CanaraBankTransaction> &rows) {
    beginResetModel();
    m_rows = rows;
    endResetModel();
    emit rowsChanged();
    emit selectionChanged();
}

void BankStatementRowsModel::clear() {
    beginResetModel();
    m_rows.clear();
    endResetModel();
    emit rowsChanged();
    emit selectionChanged();
}

void BankStatementRowsModel::setRowSelected(int index, bool selected) {
    if (index >= 0 && index < m_rows.size()) {
        setData(this->index(index), selected, IsSelectedRole);
    }
}

void BankStatementRowsModel::setRowDrAccount(int index, const QString &acc) {
    if (index >= 0 && index < m_rows.size()) {
        setData(this->index(index), acc, DrAccountRole);
    }
}

void BankStatementRowsModel::setRowCrAccount(int index, const QString &acc) {
    if (index >= 0 && index < m_rows.size()) {
        setData(this->index(index), acc, CrAccountRole);
    }
}

void BankStatementRowsModel::setRowVoucherType(int index, const QString &vType) {
    if (index >= 0 && index < m_rows.size()) {
        setData(this->index(index), vType, VoucherTypeRole);
    }
}

void BankStatementRowsModel::selectAll(bool selected) {
    for (int i = 0; i < m_rows.size(); ++i) {
        m_rows[i].isSelected = selected;
    }
    emit dataChanged(index(0), index(m_rows.size() - 1), {IsSelectedRole});
    emit selectionChanged();
}

void BankStatementRowsModel::deselectDuplicates() {
    for (int i = 0; i < m_rows.size(); ++i) {
        if (m_rows[i].isDuplicate) {
            m_rows[i].isSelected = false;
        }
    }
    emit dataChanged(index(0), index(m_rows.size() - 1), {IsSelectedRole});
    emit selectionChanged();
}

QVariantMap BankStatementRowsModel::getRow(int index) const {
    if (index < 0 || index >= m_rows.size()) return QVariantMap();
    const auto &r = m_rows.at(index);
    QVariantMap map;
    map["rowIndex"] = r.index;
    map["vDate"] = r.dateStr;
    map["isoDate"] = r.isoDate;
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

    // Fallback search for Canara Bank account
    QVariantList canaraRows = DatabaseManager::instance().executeQuery(
        "SELECT name FROM parties WHERE (group_name LIKE '%Bank%' OR group_name LIKE '%Secured Loans%') "
        "AND name LIKE '%Canara%' ORDER BY id ASC LIMIT 1;"
    );
    if (!canaraRows.isEmpty()) {
        m_bankLedgerName = canaraRows.first().toMap().value("name").toString();
    } else {
        // Generic Bank search
        QVariantList anyBank = DatabaseManager::instance().executeQuery(
            "SELECT name FROM parties WHERE group_name LIKE '%Bank%' LIMIT 1;"
        );
        if (!anyBank.isEmpty()) {
            m_bankLedgerName = anyBank.first().toMap().value("name").toString();
        } else {
            m_bankLedgerName = "Canara Bank Cc 128001400717";
        }
    }
    emit bankLedgerChanged();
}

QString BankStatementController::findMatchingParty(const QString &extractedName, const QString &narration, QString &outConfidence) {
    if (extractedName.isEmpty() && narration.isEmpty()) {
        outConfidence = "UNMATCHED";
        return QString();
    }

    // 1. Check Bank Narration Aliases Table (Learned memory)
    QVariantList aliasRows = DatabaseManager::instance().executeQuery(
        "SELECT mapped_party_name FROM bank_narration_aliases WHERE ? LIKE ('%' || narration_pattern || '%') "
        "OR narration_pattern = ? COLLATE NOCASE LIMIT 1;",
        {narration, extractedName}
    );
    if (!aliasRows.isEmpty()) {
        outConfidence = "ALIAS_MATCH";
        return aliasRows.first().toMap().value("mapped_party_name").toString();
    }

    // 2. Direct exact or LIKE search on parties table
    if (!extractedName.isEmpty()) {
        QString clean = extractedName;
        clean.remove(QRegularExpression("[^A-Za-z0-9 ]"));
        clean = clean.trimmed();

        // Exact match
        QVariant directRow = DatabaseManager::instance().executeScalar(
            "SELECT name FROM parties WHERE name = ? COLLATE NOCASE OR alias = ? COLLATE NOCASE LIMIT 1;",
            {extractedName, extractedName}
        );
        if (directRow.isValid()) {
            outConfidence = "HIGH";
            return directRow.toString();
        }

        // Search prefix or contain with mandi/city bracket
        QVariant containRow = DatabaseManager::instance().executeScalar(
            "SELECT name FROM parties WHERE name LIKE ? COLLATE NOCASE LIMIT 1;",
            {"%" + clean + "%"}
        );
        if (containRow.isValid()) {
            outConfidence = "HIGH";
            return containRow.toString();
        }

        // Fuzzy match on multi-word party names
        QStringList words = clean.split(' ', Qt::SkipEmptyParts);
        if (words.size() >= 2) {
            QString query = "SELECT name FROM parties WHERE ";
            QVariantList params;
            for (int i = 0; i < words.size(); ++i) {
                if (i > 0) query += " AND ";
                query += "name LIKE ?";
                params.append("%" + words[i] + "%");
            }
            query += " LIMIT 1;";
            QVariant fuzzyRow = DatabaseManager::instance().executeScalar(query, params);
            if (fuzzyRow.isValid()) {
                outConfidence = "HIGH";
                return fuzzyRow.toString();
            }
        }
    }

    // 3. Category Fallbacks
    if (narration.contains(" SC", Qt::CaseInsensitive) || narration.contains("SERVICE CHARGE", Qt::CaseInsensitive) || narration.contains("MORTGAGE CHARGES", Qt::CaseInsensitive)) {
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

void BankStatementController::matchPartiesAndDuplicates(QVector<CanaraBankTransaction> &txns) {
    for (auto &txn : txns) {
        QString confidence = "UNMATCHED";
        QString matchedParty = findMatchingParty(txn.extractedParty, txn.rawNarration, confidence);

        txn.confidence = confidence;

        // Set suggested debit and credit accounts based on transaction direction
        if (txn.deposit > 0.001) {
            // Customer Receipt: Dr Bank Account, Cr Customer
            txn.suggestedDrAccount = m_bankLedgerName;
            txn.suggestedCrAccount = matchedParty;
            txn.voucherType = "Cheque Receipt";
        } else {
            // Supplier / Expense Payment: Dr Party / Expense, Cr Bank Account
            txn.suggestedDrAccount = matchedParty;
            txn.suggestedCrAccount = m_bankLedgerName;
            txn.voucherType = "Cheque Payment";
        }

        // Check duplicate entry in vouchers / transactions
        double amt = txn.deposit > 0.001 ? txn.deposit : txn.withdrawal;
        QString q = "SELECT id FROM vouchers WHERE voucher_date = ? AND ABS(amount - ?) < 0.01 LIMIT 1;";
        QVariant dupVoucher = DatabaseManager::instance().executeScalar(q, {txn.isoDate, amt});

        if (dupVoucher.isValid()) {
            txn.isDuplicate = true;
            txn.isSelected = false; // Deselect duplicate by default
        } else {
            txn.isDuplicate = false;
            txn.isSelected = !matchedParty.isEmpty(); // Select if mapped
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

    CanaraBankStatementParser parser;
    QVector<CanaraBankTransaction> transactions;
    QString errorMsg;

    bool ok = parser.parsePdf(cleanPath, m_header, transactions, errorMsg);

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
    m_statusMessage = QString("Successfully parsed %1 transactions from Canara Bank statement.").arg(transactions.size());
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
        "Select Canara Bank Statement PDF",
        initialDir,
        "PDF Files (*.pdf);;All Files (*)"
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

    // Extract core keyword from narration for robust matching
    QString pattern = narration.trimmed();
    QStringList parts = pattern.split('-');
    if (parts.size() >= 4) {
        pattern = parts[3].trimmed();
        pattern.remove(QRegularExpression("[/\\\\-].*"));
    }

    DatabaseManager::instance().executeNonQuery(
        "INSERT OR REPLACE INTO bank_narration_aliases (bank_code, narration_pattern, mapped_party_name, created_at) "
        "VALUES ('CNRB', ?, ?, ?);",
        {pattern.trimmed(), mappedParty.trimmed(), QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss")}
    );
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
        QVariantList fyRows = DatabaseManager::instance().executeQuery(
            "SELECT id, year_name FROM financial_years WHERE start_date <= ? AND end_date >= ? LIMIT 1;",
            {dt, dt}
        );
        int fyId = 28;
        QString fyLabel = "FY 2026-27";
        if (!fyRows.isEmpty()) {
            fyId = fyRows.first().toMap().value("id").toInt();
            fyLabel = fyRows.first().toMap().value("year_name").toString();
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

