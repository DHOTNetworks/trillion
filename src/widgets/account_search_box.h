#pragma once

#include <QLineEdit>
#include <QListWidget>
#include <QFrame>
#include <functional>
#include <QVariantList>

class AccountSearchBox : public QLineEdit {
    Q_OBJECT

public:
    explicit AccountSearchBox(QWidget* parent = nullptr);
    ~AccountSearchBox() override;

    void setSearchFunction(std::function<QVariantList(const QString&)> fn);
    void setPartyName(const QString& partyName);
    void setParty(const QString& partyName, int partyId = 0);
    void setSelectedPartyId(int partyId);
    void clearParty();
    void clearSelection() { clearParty(); }
    QString currentPartyName() const;
    int currentPartyId() const;
    int selectedPartyId() const { return currentPartyId(); }
    QVariantMap selectedPartyData() const { return m_selectedPartyData; }
    void openSearchPopup();
    void closeSearchPopup();
    void selectCurrentListItem();
    bool isPopupVisible() const { return m_popupFrame && m_popupFrame->isVisible(); }

signals:
    void partySelected(const QString& partyName);
    void partyDataSelected(const QVariantMap& partyData);
    void partySelectedWithId(const QString& partyName, int partyId);

protected:
    void focusInEvent(QFocusEvent* event) override;
    void focusOutEvent(QFocusEvent* event) override;
    void hideEvent(QHideEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    void moveEvent(QMoveEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    bool eventFilter(QObject* watched, QEvent* event) override;

private slots:
    void onTextChanged(const QString& text);
    void onListItemClicked(QListWidgetItem* item);

private:
    void positionPopup();
    void updateResults();

    QFrame* m_popupFrame = nullptr;
    QListWidget* m_listWidget = nullptr;
    std::function<QVariantList(const QString&)> m_searchFn;
    int m_selectedPartyId = 0;
    QVariantMap m_selectedPartyData;
    bool m_programmaticChange = false;
    bool m_isUpdatingPopup = false;
};
