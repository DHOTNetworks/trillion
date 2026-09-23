#pragma once

#include <QWidget>
#include <QLineEdit>
#include <QComboBox>
#include <QCheckBox>
#include <QPushButton>
#include <QLabel>
#include <QFrame>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QScrollArea>
#include "account_search_box.h"
#include "../models/parties_model.h"

class ModifyLedgerWidget : public QWidget {
    Q_OBJECT

public:
    explicit ModifyLedgerWidget(QWidget* parent = nullptr);
    ~ModifyLedgerWidget() override = default;

    void resetForm();
    void focusSearch();
    void loadParty(int partyId);
    void loadPartyByName(const QString& name);

signals:
    void backRequested();
    void savedSuccess();

protected:
    void keyPressEvent(QKeyEvent* event) override;
    bool eventFilter(QObject* watched, QEvent* event) override;
    void showEvent(QShowEvent* event) override;

private slots:
    void onPartySelected(const QVariantMap& partyData);
    void onUpdateClicked();
    void onDeleteClicked();
    void executeUpdate();
    void executeDelete();
    void updateTotalOpeningBalDisplay();
    void onNameEditingFinished();
    void onGstinChanged(const QString& text);
    void onStateChanged(const QString& stateName);

private:
    void setupUi();
    QWidget* createHeaderSection();
    QWidget* createSearchCard();
    QWidget* createSlateSection();
    void populateDropdowns();
    void setupSearchableCombo(QComboBox* combo, const QStringList& items);
    void setComboText(QComboBox* combo, const QString& text);
    QString getComboText(const QComboBox* combo) const;

    PartiesModel m_partiesModel;
    int m_selectedPartyId = -1;

    // Top Search Card
    AccountSearchBox* m_partySearchWidget = nullptr;

    // Top Header & Live Balance Summary Monitor
    QComboBox* m_prefixCombo = nullptr;
    QPushButton* m_createPrefixBtn = nullptr;
    QLabel* m_totalDrLbl = nullptr;
    QLabel* m_totalCrLbl = nullptr;
    QLabel* m_diffBalLbl = nullptr;

    // Section 1: Identification, Station, State & Opening Balance
    QLineEdit* m_nameInput = nullptr;
    QLineEdit* m_aliasInput = nullptr;
    QComboBox* m_groupCombo = nullptr;
    QLabel* m_parentGroupLbl = nullptr;

    QComboBox* m_stationCombo = nullptr;
    QComboBox* m_stateCombo = nullptr;
    QLineEdit* m_stateCodeInput = nullptr;
    QLineEdit* m_pincodeInput = nullptr;
    QCheckBox* m_useRoutesCheck = nullptr;
    QWidget* m_routeContainer = nullptr;
    QComboBox* m_routeCombo = nullptr;

    QLineEdit* m_booksFromInput = nullptr;
    QLineEdit* m_opBalInput = nullptr;
    QComboBox* m_balTypeCombo = nullptr;
    QLineEdit* m_openFromInput = nullptr;

    // Section 2: Statutory, GST, Address & Banking (3 Columns)
    // Col 1: GST & Tax
    QLineEdit* m_gstinInput = nullptr;
    QComboBox* m_gstPartyTypeCombo = nullptr;
    QLineEdit* m_panInput = nullptr;
    QLineEdit* m_tinInput = nullptr;
    QLineEdit* m_urnInput = nullptr;

    // Col 2: Address & Contact
    QLineEdit* m_addressInput = nullptr;
    QLineEdit* m_phoneInput = nullptr;
    QLineEdit* m_whatsappInput = nullptr;
    QLineEdit* m_emailInput = nullptr;
    QLineEdit* m_aadhaarInput = nullptr;

    // Col 3: Banking & Credit Terms
    QLineEdit* m_bankAccountInput = nullptr;
    QLineEdit* m_ifscInput = nullptr;
    QLineEdit* m_bankNameInput = nullptr;
    QLineEdit* m_shopNoInput = nullptr;
    QLineEdit* m_creditLimitInput = nullptr;

    // Section 3: Behavioral Rules Checkboxes
    QCheckBox* m_applyTcsCheck = nullptr;
    QCheckBox* m_tcsExemptCheck = nullptr;
    QCheckBox* m_stockNotCalcCheck = nullptr;
    QCheckBox* m_calcDirectExpenseCheck = nullptr;
    QCheckBox* m_showDateTotalsCheck = nullptr;
    QCheckBox* m_useCreditLimitCheck = nullptr;
    QCheckBox* m_setTitleCaseCheck = nullptr;

    // Action Buttons
    QPushButton* m_cancelBtn = nullptr;
    QPushButton* m_deleteBtn = nullptr;
    QPushButton* m_updateBtn = nullptr;

    QList<QWidget*> m_navOrder;
};
