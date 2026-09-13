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

struct DebitCreditNoteItem {
    QString itemName;
    QString hsnCode = "1006";
    QString unit = "QTL";
    int bags = 0;
    double weightQtl = 0.0;
    double rate = 0.0;
    double taxableAmount = 0.0;
    double gstPct = 5.0;
};

class DebitCreditNoteItemsModel : public QAbstractListModel {
    Q_OBJECT
    Q_PROPERTY(int count READ count NOTIFY countChanged)

public:
    enum Roles {
        ItemNameRole = Qt::UserRole + 1,
        HsnCodeRole,
        UnitRole,
        BagsRole,
        WeightQtlRole,
        RateRole,
        TaxableAmountRole,
        GstPctRole
    };

    explicit DebitCreditNoteItemsModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;
    QHash<int, QByteArray> roleNames() const override;

    const QVector<DebitCreditNoteItem>& items() const { return m_items; }
    void setItems(const QVector<DebitCreditNoteItem> &items);
    Q_INVOKABLE void addItem(const QString &itemName = QString(), const QString &hsn = "1006",
                             int bags = 0, double weight = 0.0, double rate = 0.0,
                             double amount = 0.0, double gst = 5.0);
    Q_INVOKABLE void removeItem(int index);
    Q_INVOKABLE void updateItem(int index, const QString &field, const QVariant &value);
    Q_INVOKABLE void clear();
    Q_INVOKABLE int count() const { return m_items.size(); }
    Q_INVOKABLE QVariantMap get(int index) const;
    Q_INVOKABLE QVariantList toVariantList() const;

signals:
    void countChanged();
    void itemDataChanged();

private:
    QVector<DebitCreditNoteItem> m_items;
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
    Q_PROPERTY(DebitCreditNoteItemsModel* itemsModel READ itemsModel CONSTANT)

    // Register / Summary Stats
    Q_PROPERTY(int totalCount READ totalCount NOTIFY summaryChanged)
    Q_PROPERTY(double totalCreditAmount READ totalCreditAmount NOTIFY summaryChanged)
    Q_PROPERTY(double totalDebitAmount READ totalDebitAmount NOTIFY summaryChanged)
    Q_PROPERTY(double totalGstAdjusted READ totalGstAdjusted NOTIFY summaryChanged)
    Q_PROPERTY(double netAdjustmentValue READ netAdjustmentValue NOTIFY summaryChanged)
    Q_PROPERTY(QString totalCreditAmountFmt READ totalCreditAmountFmt NOTIFY summaryChanged)
    Q_PROPERTY(QString totalDebitAmountFmt READ totalDebitAmountFmt NOTIFY summaryChanged)
    Q_PROPERTY(QString totalGstAdjustedFmt READ totalGstAdjustedFmt NOTIFY summaryChanged)
    Q_PROPERTY(QString netAdjustmentValueFmt READ netAdjustmentValueFmt NOTIFY summaryChanged)

    // Draft Form Properties
    Q_PROPERTY(int draftId READ draftId WRITE setDraftId NOTIFY draftChanged)
    Q_PROPERTY(QString draftNoteType READ draftNoteType WRITE setDraftNoteType NOTIFY draftChanged)
    Q_PROPERTY(QString draftNoteNo READ draftNoteNo WRITE setDraftNoteNo NOTIFY draftChanged)
    Q_PROPERTY(QString draftNoteDate READ draftNoteDate WRITE setDraftNoteDate NOTIFY draftChanged)
    Q_PROPERTY(QString draftNoteTime READ draftNoteTime WRITE setDraftNoteTime NOTIFY draftChanged)
    Q_PROPERTY(QString draftOrigInvNo READ draftOrigInvNo WRITE setDraftOrigInvNo NOTIFY draftChanged)
    Q_PROPERTY(QString draftOrigInvDate READ draftOrigInvDate WRITE setDraftOrigInvDate NOTIFY draftChanged)
    Q_PROPERTY(QString draftOrigInvType READ draftOrigInvType WRITE setDraftOrigInvType NOTIFY draftChanged)
    Q_PROPERTY(QString draftPartyName READ draftPartyName WRITE setDraftPartyName NOTIFY draftChanged)
    Q_PROPERTY(QString draftPartyGstin READ draftPartyGstin WRITE setDraftPartyGstin NOTIFY draftChanged)
    Q_PROPERTY(bool draftIsInterstate READ draftIsInterstate WRITE setDraftIsInterstate NOTIFY draftChanged)
    Q_PROPERTY(double draftGstPct READ draftGstPct WRITE setDraftGstPct NOTIFY draftChanged)
    Q_PROPERTY(QString draftReasonCode READ draftReasonCode WRITE setDraftReasonCode NOTIFY draftChanged)
    Q_PROPERTY(QString draftAdjType READ draftAdjType WRITE setDraftAdjType NOTIFY draftChanged)
    Q_PROPERTY(QString draftNarration READ draftNarration WRITE setDraftNarration NOTIFY draftChanged)

    // Draft Form Live Calculations
    Q_PROPERTY(double formTaxableAmount READ formTaxableAmount NOTIFY formTotalsChanged)
    Q_PROPERTY(double formCgstAmount READ formCgstAmount NOTIFY formTotalsChanged)
    Q_PROPERTY(double formSgstAmount READ formSgstAmount NOTIFY formTotalsChanged)
    Q_PROPERTY(double formIgstAmount READ formIgstAmount NOTIFY formTotalsChanged)
    Q_PROPERTY(double formTotalTaxAmount READ formTotalTaxAmount NOTIFY formTotalsChanged)
    Q_PROPERTY(double formRoundOff READ formRoundOff NOTIFY formTotalsChanged)
    Q_PROPERTY(double formGrandTotal READ formGrandTotal NOTIFY formTotalsChanged)
    Q_PROPERTY(int formTotalBags READ formTotalBags NOTIFY formTotalsChanged)
    Q_PROPERTY(double formTotalWeightQtl READ formTotalWeightQtl NOTIFY formTotalsChanged)

    Q_PROPERTY(QString formTaxableAmountFmt READ formTaxableAmountFmt NOTIFY formTotalsChanged)
    Q_PROPERTY(QString formCgstAmountFmt READ formCgstAmountFmt NOTIFY formTotalsChanged)
    Q_PROPERTY(QString formSgstAmountFmt READ formSgstAmountFmt NOTIFY formTotalsChanged)
    Q_PROPERTY(QString formIgstAmountFmt READ formIgstAmountFmt NOTIFY formTotalsChanged)
    Q_PROPERTY(QString formTotalTaxAmountFmt READ formTotalTaxAmountFmt NOTIFY formTotalsChanged)
    Q_PROPERTY(QString formRoundOffFmt READ formRoundOffFmt NOTIFY formTotalsChanged)
    Q_PROPERTY(QString formGrandTotalFmt READ formGrandTotalFmt NOTIFY formTotalsChanged)

public:
    explicit DebitCreditNoteController(QObject *parent = nullptr);

    DebitCreditNoteModel* model() { return &m_model; }
    DebitCreditNoteItemsModel* itemsModel() { return &m_itemsModel; }

    int totalCount() const { return m_totalCount; }
    double totalCreditAmount() const { return m_totalCreditAmount; }
    double totalDebitAmount() const { return m_totalDebitAmount; }
    double totalGstAdjusted() const { return m_totalGstAdjusted; }
    double netAdjustmentValue() const { return m_netAdjustmentValue; }

    QString totalCreditAmountFmt() const;
    QString totalDebitAmountFmt() const;
    QString totalGstAdjustedFmt() const;
    QString netAdjustmentValueFmt() const;

    // Draft Form Getters & Setters
    int draftId() const { return m_draftId; }
    void setDraftId(int val);

    QString draftNoteType() const { return m_draftNoteType; }
    void setDraftNoteType(const QString &val);

    QString draftNoteNo() const { return m_draftNoteNo; }
    void setDraftNoteNo(const QString &val);

    QString draftNoteDate() const { return m_draftNoteDate; }
    void setDraftNoteDate(const QString &val);

    QString draftNoteTime() const { return m_draftNoteTime; }
    void setDraftNoteTime(const QString &val);

    QString draftOrigInvNo() const { return m_draftOrigInvNo; }
    void setDraftOrigInvNo(const QString &val);

    QString draftOrigInvDate() const { return m_draftOrigInvDate; }
    void setDraftOrigInvDate(const QString &val);

    QString draftOrigInvType() const { return m_draftOrigInvType; }
    void setDraftOrigInvType(const QString &val);

    QString draftPartyName() const { return m_draftPartyName; }
    void setDraftPartyName(const QString &val);

    QString draftPartyGstin() const { return m_draftPartyGstin; }
    void setDraftPartyGstin(const QString &val);

    bool draftIsInterstate() const { return m_draftIsInterstate; }
    void setDraftIsInterstate(bool val);

    double draftGstPct() const { return m_draftGstPct; }
    void setDraftGstPct(double val);

    QString draftReasonCode() const { return m_draftReasonCode; }
    void setDraftReasonCode(const QString &val);

    QString draftAdjType() const { return m_draftAdjType; }
    void setDraftAdjType(const QString &val);

    QString draftNarration() const { return m_draftNarration; }
    void setDraftNarration(const QString &val);

    // Form Live Calculations
    double formTaxableAmount() const { return m_formTaxableAmount; }
    double formCgstAmount() const { return m_formCgstAmount; }
    double formSgstAmount() const { return m_formSgstAmount; }
    double formIgstAmount() const { return m_formIgstAmount; }
    double formTotalTaxAmount() const { return m_formTotalTaxAmount; }
    double formRoundOff() const { return m_formRoundOff; }
    double formGrandTotal() const { return m_formGrandTotal; }
    int formTotalBags() const { return m_formTotalBags; }
    double formTotalWeightQtl() const { return m_formTotalWeightQtl; }

    QString formTaxableAmountFmt() const;
    QString formCgstAmountFmt() const;
    QString formSgstAmountFmt() const;
    QString formIgstAmountFmt() const;
    QString formTotalTaxAmountFmt() const;
    QString formRoundOffFmt() const;
    QString formGrandTotalFmt() const;

    // Invokables
    Q_INVOKABLE void reload();
    Q_INVOKABLE void resetDraft();
    Q_INVOKABLE bool loadNoteIntoDraft(int id);
    Q_INVOKABLE bool fetchAndLoadOriginalInvoice(const QString &invType, const QString &invNo);
    Q_INVOKABLE bool saveCurrentDraft();
    Q_INVOKABLE bool deleteNote(int id);
    Q_INVOKABLE QString getNextNoteNo(const QString &noteType);
    Q_INVOKABLE QString currentDateIso() const;
    Q_INVOKABLE QString currentTimeIso() const;

    // Legacy support / general methods
    Q_INVOKABLE QVariantMap fetchOriginalInvoice(const QString &invType, const QString &invNo);
    Q_INVOKABLE QVariantMap calculateTotals(const QVariantList &items, double gstPct, bool isInterstate);
    Q_INVOKABLE QVariantMap saveNote(const QVariantMap &data);
    Q_INVOKABLE QVariantMap getNote(int id);
    Q_INVOKABLE QVariantList searchInvoices(const QString &invType, const QString &query);

signals:
    void summaryChanged();
    void draftChanged();
    void formTotalsChanged();
    void noteSaved(int id, const QString &noteNo);
    void noteDeleted(int id);

private slots:
    void recalculateFormTotals();

private:
    void recalculateSummary();
    void postDoubleEntryVoucher(const DebitCreditNoteRecord &note);
    void deleteDoubleEntryVouchers(int noteId);

    DebitCreditNoteModel m_model;
    DebitCreditNoteItemsModel m_itemsModel;

    int m_totalCount = 0;
    double m_totalCreditAmount = 0.0;
    double m_totalDebitAmount = 0.0;
    double m_totalGstAdjusted = 0.0;
    double m_netAdjustmentValue = 0.0;

    // Draft State
    int m_draftId = 0;
    QString m_draftNoteType = "Credit Note";
    QString m_draftNoteNo;
    QString m_draftNoteDate;
    QString m_draftNoteTime;
    QString m_draftOrigInvNo;
    QString m_draftOrigInvDate;
    QString m_draftOrigInvType = "Sale";
    int m_draftOrigInvId = 0;
    QString m_draftPartyName;
    QString m_draftPartyGstin;
    QString m_draftStateCode;
    bool m_draftIsInterstate = false;
    double m_draftGstPct = 5.0;
    QString m_draftReasonCode = "01-Sales Return";
    QString m_draftAdjType = "Sales Return";
    QString m_draftNarration;

    // Form Live Calculations
    int m_formTotalBags = 0;
    double m_formTotalWeightQtl = 0.0;
    double m_formTaxableAmount = 0.0;
    double m_formCgstAmount = 0.0;
    double m_formSgstAmount = 0.0;
    double m_formIgstAmount = 0.0;
    double m_formTotalTaxAmount = 0.0;
    double m_formRoundOff = 0.0;
    double m_formGrandTotal = 0.0;
};

