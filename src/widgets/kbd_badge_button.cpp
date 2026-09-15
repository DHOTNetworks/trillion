#include "kbd_badge_button.h"
#include <QPainter>
#include <QPaintEvent>
#include <QFontMetrics>

KbdBadgeButton::KbdBadgeButton(const QString& text, const QString& shortcut,
                               const QColor& bgColor, const QColor& hoverColor,
                               QWidget* parent)
    : QPushButton(parent)
    , m_shortcut(shortcut)
    , m_bgColor(bgColor)
    , m_hoverColor(hoverColor)
{
    setText(text);
    setFixedHeight(32);
    setCursor(Qt::PointingHandCursor);
    setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);
}

void KbdBadgeButton::setButtonColors(const QColor& bg, const QColor& hover, const QColor& badgeBg) {
    m_bgColor = bg;
    m_hoverColor = hover;
    m_badgeBg = badgeBg;
    update();
}

void KbdBadgeButton::paintEvent(QPaintEvent* /*event*/) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    bool isHovered = underMouse() && isEnabled();
    QColor currentBg = isHovered ? m_hoverColor : m_bgColor;
    if (!isEnabled()) {
        currentBg = QColor("#94A3B8");
    }

    // Draw Rounded Button Background
    painter.setBrush(currentBg);
    painter.setPen(Qt::NoPen);
    painter.drawRoundedRect(rect(), 6, 6);

    // Calculate text and badge layout
    QFont font = painter.font();
    font.setPointSize(10);
    font.setBold(true);
    painter.setFont(font);

    QFontMetrics fm(font);
    int textWidth = fm.horizontalAdvance(text());
    int badgeWidth = 0;
    int spacing = m_shortcut.isEmpty() ? 0 : 8;

    QFont badgeFont = font;
    badgeFont.setPointSize(8);
    badgeFont.setBold(true);
    QFontMetrics bfm(badgeFont);

    if (!m_shortcut.isEmpty()) {
        badgeWidth = bfm.horizontalAdvance(m_shortcut) + 10;
    }

    int totalContentWidth = textWidth + spacing + badgeWidth;
    int startX = (width() - totalContentWidth) / 2;
    int centerY = height() / 2;

    // Draw Main Text
    painter.setPen(Qt::white);
    painter.drawText(QRect(startX, 0, textWidth, height()), Qt::AlignVCenter | Qt::AlignLeft, text());

    // Draw Shortcut Badge if present
    if (!m_shortcut.isEmpty()) {
        int badgeX = startX + textWidth + spacing;
        int badgeH = 18;
        int badgeY = (height() - badgeH) / 2;
        QRect badgeRect(badgeX, badgeY, badgeWidth, badgeH);

        QColor badgeBgColor = m_badgeBg.isValid() ? m_badgeBg : QColor(0, 0, 0, 40);
        painter.setBrush(badgeBgColor);
        painter.setPen(QColor(255, 255, 255, 60));
        painter.drawRoundedRect(badgeRect, 4, 4);

        painter.setFont(badgeFont);
        painter.setPen(QColor("#FEF08A")); // Soft yellow accent for keyboard shortcut
        painter.drawText(badgeRect, Qt::AlignCenter, m_shortcut);
    }
}
