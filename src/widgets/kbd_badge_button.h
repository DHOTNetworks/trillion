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
                           const QColor& textColor = QColor("#FFFFFF"),
                           const QColor& borderColor = QColor(Qt::transparent),
                           QWidget* parent = nullptr);

    void setButtonColors(const QColor& bg, const QColor& hover, const QColor& text = QColor("#FFFFFF"),
                         const QColor& border = QColor(Qt::transparent), const QColor& badgeBg = QColor(),
                         const QColor& badgeText = QColor());

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    QString m_shortcut;
    QColor m_bgColor;
    QColor m_hoverColor;
    QColor m_textColor;
    QColor m_borderColor;
    QColor m_badgeBg;
    QColor m_badgeText;
};
