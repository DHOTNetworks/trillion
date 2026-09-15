#pragma once

#include <QPushButton>
#include <QString>
#include <QColor>

class KbdBadgeButton : public QPushButton {
    Q_OBJECT

public:
    explicit KbdBadgeButton(const QString& text, const QString& shortcut = "",
                           const QColor& bgColor = QColor("#2563EB"),
                           const QColor& hoverColor = QColor("#1D4ED8"),
                           QWidget* parent = nullptr);

    void setButtonColors(const QColor& bg, const QColor& hover, const QColor& badgeBg = QColor());

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    QString m_shortcut;
    QColor m_bgColor;
    QColor m_hoverColor;
    QColor m_badgeBg;
};
