#include "kbd_badge_button.h"
#include <QPainter>
#include <QPaintEvent>
#include <QFontMetrics>

KbdBadgeButton::KbdBadgeButton(const QString& text, const QString& shortcut,
                               const QColor& bgColor, const QColor& hoverColor,
                               const QColor& textColor, const QColor& borderColor,
                               QWidget* parent)
    : QPushButton(parent)
    , m_shortcut(shortcut)
    , m_bgColor(bgColor)
    , m_hoverColor(hoverColor)
    , m_textColor(textColor)
    , m_borderColor(borderColor)
{
    setText(text);
    setFixedHeight(34);
    setCursor(Qt::PointingHandCursor);
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
}

void KbdBadgeButton::setButtonColors(const QColor& bg, const QColor& hover, const QColor& text,
                                     const QColor& border, const QColor& badgeBg, const QColor& badgeText) {
    m_bgColor = bg;
    m_hoverColor = hover;
    m_textColor = text;
    m_borderColor = border;
    m_badgeBg = badgeBg;
    m_badgeText = badgeText;
    update();
}

QSize KbdBadgeButton::sizeHint() const {
    QFont font = this->font();
    font.setPointSize(10);
    font.setBold(true);
    QFontMetrics fm(font);
    int textWidth = fm.horizontalAdvance(text());

    int badgeWidth = 0;
    int spacing = 0;
    if (!m_shortcut.isEmpty()) {
        QFont badgeFont = font;
        badgeFont.setPointSize(8);
        badgeFont.setBold(true);
        QFontMetrics bfm(badgeFont);
        badgeWidth = bfm.horizontalAdvance(m_shortcut) + 12; // 6px padding on each side
        spacing = 8;
    }

    int totalW = 16 + textWidth + spacing + badgeWidth + 16; // 16px left + 16px right margin
    return QSize(qMax(totalW, 80), 34);
}

QSize KbdBadgeButton::minimumSizeHint() const {
    return sizeHint();
}

void KbdBadgeButton::paintEvent(QPaintEvent* /*event*/) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    bool isHovered = underMouse() && isEnabled();
    QColor currentBg = isHovered ? m_hoverColor : m_bgColor;
    if (!isEnabled()) {
        currentBg = QColor("#E2E8F0");
    }

    // 1. Draw Background
    painter.setBrush(currentBg);
    if (m_borderColor.isValid() && m_borderColor != Qt::transparent) {
        painter.setPen(QPen(m_borderColor, 1));
    } else {
        painter.setPen(Qt::NoPen);
    }
    painter.drawRoundedRect(rect().adjusted(1, 1, -1, -1), 6, 6);

    // 2. Setup Fonts
    QFont font = painter.font();
    font.setFamily("Segoe UI");
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
        badgeWidth = bfm.horizontalAdvance(m_shortcut) + 12;
    }

    int totalContentWidth = textWidth + spacing + badgeWidth;
    int startX = (width() - totalContentWidth) / 2;

    // 3. Draw Main Text
    QColor txtColor = isEnabled() ? m_textColor : QColor("#94A3B8");
    painter.setPen(txtColor);
    painter.drawText(QRect(startX, 0, textWidth, height()), Qt::AlignVCenter | Qt::AlignLeft, text());

    // 4. Draw Shortcut Badge if present
    if (!m_shortcut.isEmpty()) {
        int badgeX = startX + textWidth + spacing;
        int badgeH = 18;
        int badgeY = (height() - badgeH) / 2;
        QRect badgeRect(badgeX, badgeY, badgeWidth, badgeH);

        QColor bBg;
        if (m_badgeBg.isValid()) {
            bBg = m_badgeBg;
        } else if (m_textColor == QColor("#FFFFFF")) {
            bBg = QColor(0, 0, 0, 50); // Dark translucent badge for bright buttons
        } else {
            bBg = QColor("#E2E8F0"); // Light gray badge for white/light buttons
        }

        painter.setBrush(bBg);
        painter.setPen(Qt::NoPen);
        painter.drawRoundedRect(badgeRect, 4, 4);

        painter.setFont(badgeFont);
        QColor bTxt;
        if (m_badgeText.isValid()) {
            bTxt = m_badgeText;
        } else if (m_textColor == QColor("#FFFFFF")) {
            bTxt = QColor("#FEF08A"); // Accent yellow for dark/colored buttons
        } else {
            bTxt = QColor("#475569"); // Slate gray for light buttons
        }
        painter.setPen(bTxt);
        painter.drawText(badgeRect, Qt::AlignCenter, m_shortcut);
    }
}

