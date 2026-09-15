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
    QString currentPartyName() const;
    void openSearchPopup();
    void closeSearchPopup();

signals:
    void partySelected(const QString& partyName);

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
    void positionPopup();
    void updateResults();

    QFrame* m_popupFrame = nullptr;
    QListWidget* m_listWidget = nullptr;
    std::function<QVariantList(const QString&)> m_searchFn;
    bool m_programmaticChange = false;
    bool m_isUpdatingPopup = false;
};
