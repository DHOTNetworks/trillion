#pragma once

#include <QObject>
#include <QAbstractListModel>
#include <QString>
#include <QVector>
#include <QVariantMap>
#include <QVariantList>

struct DebitCreditNoteRecord {
    int id = 0;
    int fyId = 0;
    QString financialYear;
    QString noteType = "Credit Note"; // "Credit Note" or "Debit Note"
    QString noteNo;
    QString noteDate;
    QString noteTime;
    int originalInvoiceId = 0;
    QString originalInvoiceNo;
    QString originalInvoiceDate;
    QString originalInvoiceType = "Sale"; // "Sale" or "Purchase"
    int partyId = 0;
    QString partyName;
    QString partyGstin;
    QString stateCode;
    bool isInterstate = false;
    QString reasonCode = "01-Sales Return";
    QString adjustmentType = "Sales Return"; // "Sales Return", "Rate Cut", "Purchase Return", "Purchase Rate Cut", "Discount", "Correction"
    QString itemName;
    QString hsnCode;
    int totalBags = 0;
    double totalWeightQtl = 0.0;
    double taxableAmount = 0.0;
    double gstPct = 5.0;
    double cgstAmount = 0.0;
    double sgstAmount = 0.0;
    double igstAmount = 0.0;
    double totalTaxAmount = 0.0;
    double roundOff = 0.0;
    double grandTotal = 0.0;
    QString narration;
    int voucherId = 0;
};

class DebitCreditNoteModel : public QAbstractListModel {
    Q_OBJECT
    Q_PROPERTY(int count READ count NOTIFY countChanged)

public:
    enum Roles {
        IdRole = Qt::UserRole + 1,
        NoteTypeRole,
        NoteNoRole,
        NoteDateRole,
        NoteTimeRole,
        OriginalInvoiceNoRole,
        OriginalInvoiceDateRole,
        OriginalInvoiceTypeRole,
        PartyNameRole,
        PartyGstinRole,
        IsInterstateRole,
        ReasonCodeRole,
        AdjustmentTypeRole,
        ItemNameRole,
        HsnCodeRole,
        TotalBagsRole,
        TotalWeightRole,
        TaxableAmountRole,
        GstPctRole,
        CgstAmountRole,
        SgstAmountRole,
        IgstAmountRole,
        TotalTaxAmountRole,
        RoundOffRole,
        GrandTotalRole,
        NarrationRole,
        TaxableAmountFmtRole,
        TotalTaxAmountFmtRole,
        GrandTotalFmtRole
    };

    explicit DebitCreditNoteModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    void setAllRecords(const QVector<DebitCreditNoteRecord> &records);
    const QVector<DebitCreditNoteRecord>& allRecords() const { return m_allRecords; }

    Q_INVOKABLE void setFilter(const QString &filterType, const QString &searchQuery);
    Q_INVOKABLE QVariantMap get(int visibleIndex) const;
    Q_INVOKABLE int count() const { return m_visibleIndices.size(); }

signals:
    void countChanged();

private:
    void applyFilter();

    QVector<DebitCreditNoteRecord> m_allRecords;
    QVector<int> m_visibleIndices;
    QString m_filterType = "ALL";
    QString m_searchQuery;
};

class DebitCreditNoteController : public QObject {
    Q_OBJECT

    Q_PROPERTY(DebitCreditNoteModel* model READ model CONSTANT)
    Q_PROPERTY(int totalCount READ totalCount NOTIFY summaryChanged)
    Q_PROPERTY(double totalCreditAmount READ totalCreditAmount NOTIFY summaryChanged)
    Q_PROPERTY(double totalDebitAmount READ totalDebitAmount NOTIFY summaryChanged)
    Q_PROPERTY(double totalGstAdjusted READ totalGstAdjusted NOTIFY summaryChanged)
    Q_PROPERTY(double netAdjustmentValue READ netAdjustmentValue NOTIFY summaryChanged)
    Q_PROPERTY(QString totalCreditAmountFmt READ totalCreditAmountFmt NOTIFY summaryChanged)
    Q_PROPERTY(QString totalDebitAmountFmt READ totalDebitAmountFmt NOTIFY summaryChanged)
    Q_PROPERTY(QString totalGstAdjustedFmt READ totalGstAdjustedFmt NOTIFY summaryChanged)
    Q_PROPERTY(QString netAdjustmentValueFmt READ netAdjustmentValueFmt NOTIFY summaryChanged)

public:
    explicit DebitCreditNoteController(QObject *parent = nullptr);

    DebitCreditNoteModel* model() { return &m_model; }

    int totalCount() const { return m_totalCount; }
    double totalCreditAmount() const { return m_totalCreditAmount; }
    double totalDebitAmount() const { return m_totalDebitAmount; }
    double totalGstAdjusted() const { return m_totalGstAdjusted; }
    double netAdjustmentValue() const { return m_netAdjustmentValue; }

    QString totalCreditAmountFmt() const;
    QString totalDebitAmountFmt() const;
    QString totalGstAdjustedFmt() const;
    QString netAdjustmentValueFmt() const;

    Q_INVOKABLE void reload();
    Q_INVOKABLE QVariantMap fetchOriginalInvoice(const QString &invType, const QString &invNo);
    Q_INVOKABLE QVariantMap calculateTotals(const QVariantList &items, double gstPct, bool isInterstate);
    Q_INVOKABLE QVariantMap saveNote(const QVariantMap &data);
    Q_INVOKABLE bool deleteNote(int id);
    Q_INVOKABLE QVariantMap getNote(int id);
    Q_INVOKABLE QString getNextNoteNo(const QString &noteType);
    Q_INVOKABLE QVariantList searchInvoices(const QString &invType, const QString &query);

signals:
    void summaryChanged();
    void noteSaved(int id, const QString &noteNo);
    void noteDeleted(int id);

private:
    void recalculateSummary();
    void postDoubleEntryVoucher(const DebitCreditNoteRecord &note);
    void deleteDoubleEntryVouchers(int noteId);

    DebitCreditNoteModel m_model;
    int m_totalCount = 0;
    double m_totalCreditAmount = 0.0;
    double m_totalDebitAmount = 0.0;
    double m_totalGstAdjusted = 0.0;
    double m_netAdjustmentValue = 0.0;
};
