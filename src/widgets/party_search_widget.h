#pragma once

#include <QLineEdit>
#include <QListWidget>
#include <QFrame>
#include <QVariantMap>
#include <QVariantList>
#include <functional>
#include "../models/parties_model.h"

class PartySearchWidget : public QLineEdit {
    Q_OBJECT

public:
    explicit PartySearchWidget(QWidget* parent = nullptr);
    ~PartySearchWidget() override;

    void setPartyName(const QString& name);
    QString currentPartyName() const { return text(); }
    int selectedPartyId() const { return m_selectedPartyId; }
    QVariantMap selectedPartyData() const { return m_selectedPartyData; }
    void clearSelection();
    void openSearchPopup();
    void closeSearchPopup();
    void selectCurrentListItem();
    bool isPopupVisible() const { return m_popupFrame && m_popupFrame->isVisible(); }

signals:
    void partySelected(const QVariantMap& partyData);
    void partySelectionCancelled();

protected:
    void focusInEvent(QFocusEvent* event) override;
    void focusOutEvent(QFocusEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    void moveEvent(QMoveEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    bool eventFilter(QObject* watched, QEvent* event) override;

private slots:
    void onTextChanged(const QString& text);
    void onListItemClicked(QListWidgetItem* item);

private:
    void setupUi();
    void positionPopup();
    void updateResults();

    QFrame* m_popupFrame = nullptr;
    QListWidget* m_listWidget = nullptr;
    PartiesModel m_partiesModel;
    int m_selectedPartyId = -1;
    QVariantMap m_selectedPartyData;
    bool m_programmaticChange = false;
    bool m_isUpdatingPopup = false;
};
