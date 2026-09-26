#include "cashbook_options_dialog.h"
#include <QKeyEvent>
#include <QCursor>

namespace MahadevERP {

CashBookOptionsDialog::CashBookOptionsDialog(QWidget* parent)
    : QDialog(parent, Qt::Dialog | Qt::FramelessWindowHint)
{
    setModal(true);
    setAttribute(Qt::WA_StyledBackground, true);
    setObjectName("cashBookOptionsDialog");
    setupUi();
}

void CashBookOptionsDialog::setupUi() {
    setFixedWidth(460);
    setStyleSheet(
        "#cashBookOptionsDialog { background-color: #FFFFFF; border: 2.5px solid #2563EB; border-radius: 12px; }"
        "QLabel { border: none; background: transparent; }"
    );

    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(16, 14, 16, 14);
    root->setSpacing(6);

    // Header
    auto* header = new QHBoxLayout();
    auto* titleLabel = new QLabel("CASH BOOK TYPE...", this);
    titleLabel->setStyleSheet("color: #2563EB; font-size: 13.5px; font-weight: 800; letter-spacing: 0.8px; border: none; background: transparent;");
    header->addWidget(titleLabel);

    header->addStretch(1);
    auto* hint = new QLabel("Press Key or ↑ / ↓ & Enter", this);
    hint->setStyleSheet("color: #64748B; font-size: 11px; font-weight: 700; border: none; background: transparent;");
    header->addWidget(hint);
    root->addLayout(header);

    // Build Option list
    m_options = {
        {"Flat Type", "F", Qt::Key_F, CashBookOptionResult::FlatType, false},
        {"Other Type", "O", Qt::Key_O, CashBookOptionResult::OtherType, false},
        {"Day-Wise Balance", "D", Qt::Key_D, CashBookOptionResult::DayWiseBalance, false},
        {"Find Negative Bal.", "N", Qt::Key_N, CashBookOptionResult::FindNegativeBal, false},
        {"Find Maximum Bal.", "X", Qt::Key_X, CashBookOptionResult::FindMaximumBal, false},
        {"Find Minimum Bal.", "B", Qt::Key_B, CashBookOptionResult::FindMinimumBal, false},
        {"Cash & Bank Flow Statements", "", 0, CashBookOptionResult::Cancelled, true},
        {"Cash Flow Statement", "A", Qt::Key_A, CashBookOptionResult::CashFlowStatement, false},
        {"Bank Flow Statement", "N", Qt::Key_N, CashBookOptionResult::BankFlowStatement, false},
        {"Cash & Bank Joint Flow", "J", Qt::Key_J, CashBookOptionResult::JointFlowStatement, false}
    };

    m_itemFrames.clear();

    for (int i = 0; i < m_options.size(); ++i) {
        const auto& opt = m_options[i];
        if (opt.isSeparator) {
            auto* sepBox = new QVBoxLayout();
            sepBox->setContentsMargins(4, 6, 4, 4);
            sepBox->setSpacing(4);

            auto* sepLine = new QFrame(this);
            sepLine->setFixedHeight(1);
            sepLine->setStyleSheet("background-color: #E2E8F0; border: none;");
            sepBox->addWidget(sepLine);

            auto* secTitle = new QLabel(opt.title, this);
            secTitle->setStyleSheet("color: #7C3AED; font-size: 11.5px; font-weight: 800; text-decoration: underline;");
            sepBox->addWidget(secTitle);

            root->addLayout(sepBox);
            m_itemFrames.append(nullptr); // Placeholder
            continue;
        }

        auto* row = new QFrame(this);
        row->setObjectName("menuRow");
        row->setFixedHeight(38);
        row->setCursor(Qt::PointingHandCursor);
        row->installEventFilter(this);

        auto* rowLayout = new QHBoxLayout(row);
        rowLayout->setContentsMargins(12, 0, 12, 0);

        auto* label = new QLabel(opt.title, row);
        label->setAttribute(Qt::WA_TransparentForMouseEvents, true);
        label->setStyleSheet("font-size: 13px; font-weight: 700; color: #0F172A; border: none; background: transparent;");
        rowLayout->addWidget(label, 1);

        if (!opt.hotkeyLetter.isEmpty()) {
            auto* sc = new QLabel(opt.hotkeyLetter, row);
            sc->setObjectName("scLabel");
            sc->setAttribute(Qt::WA_TransparentForMouseEvents, true);
            sc->setStyleSheet(
                "#scLabel { background-color: #F1F5F9; border: 1px solid #CBD5E1; border-radius: 4px; padding: 2px 8px; font-size: 11px; font-weight: 800; color: #2563EB; }"
            );
            rowLayout->addWidget(sc);
        }

        m_itemFrames.append(row);
        root->addWidget(row);
    }

    // Set initial selection to first selectable row (e.g. Cash Flow Statement at index 7 or Flat Type at index 0)
    m_selectedIndex = 7; // Highlight Cash Flow Statement by default
    updateSelection(m_selectedIndex);
}

void CashBookOptionsDialog::showEvent(QShowEvent* event) {
    QDialog::showEvent(event);
    QWidget* topWin = parentWidget() ? parentWidget()->window() : nullptr;
    if (topWin) {
        QPoint center = topWin->mapToGlobal(topWin->rect().center());
        move(center.x() - width() / 2, center.y() - height() / 2);
    }
    updateSelection(m_selectedIndex);
}

bool CashBookOptionsDialog::eventFilter(QObject* watched, QEvent* event) {
    for (int i = 0; i < m_itemFrames.size(); ++i) {
        if (m_itemFrames[i] && m_itemFrames[i] == watched) {
            if (event->type() == QEvent::MouseButtonPress) {
                m_selectedIndex = i;
                triggerCurrent();
                return true;
            } else if (event->type() == QEvent::MouseMove || event->type() == QEvent::Enter) {
                updateSelection(i);
                return true;
            }
        }
    }
    return QDialog::eventFilter(watched, event);
}

void CashBookOptionsDialog::updateSelection(int idx) {
    if (idx < 0 || idx >= m_itemFrames.size() || m_itemFrames[idx] == nullptr) return;
    m_selectedIndex = idx;

    for (int i = 0; i < m_itemFrames.size(); ++i) {
        if (!m_itemFrames[i]) continue;
        if (i == m_selectedIndex) {
            m_itemFrames[i]->setStyleSheet(
                "#menuRow { background-color: #EFF6FF; border: 1.5px solid #2563EB; border-radius: 6px; }"
                "QLabel { border: none; background: transparent; }"
            );
        } else {
            m_itemFrames[i]->setStyleSheet(
                "#menuRow { background-color: #FFFFFF; border: 1px solid #E2E8F0; border-radius: 6px; }"
                "QLabel { border: none; background: transparent; }"
            );
        }
    }
}

void CashBookOptionsDialog::triggerCurrent() {
    if (m_selectedIndex >= 0 && m_selectedIndex < m_options.size() && !m_options[m_selectedIndex].isSeparator) {
        m_result = m_options[m_selectedIndex].result;
        accept();
    }
}

void CashBookOptionsDialog::keyPressEvent(QKeyEvent* event) {
    int key = event->key();

    if (key == Qt::Key_Up) {
        int next = m_selectedIndex;
        do {
            next = (next > 0) ? next - 1 : m_itemFrames.size() - 1;
        } while (m_itemFrames[next] == nullptr);
        updateSelection(next);
        event->accept();
        return;
    } else if (key == Qt::Key_Down) {
        int next = m_selectedIndex;
        do {
            next = (next < m_itemFrames.size() - 1) ? next + 1 : 0;
        } while (m_itemFrames[next] == nullptr);
        updateSelection(next);
        event->accept();
        return;
    } else if (key == Qt::Key_Return || key == Qt::Key_Enter) {
        triggerCurrent();
        event->accept();
        return;
    } else if (key == Qt::Key_Escape) {
        m_result = CashBookOptionResult::Cancelled;
        reject();
        event->accept();
        return;
    }

    // Direct Letter hotkey checks
    for (int i = 0; i < m_options.size(); ++i) {
        if (!m_options[i].isSeparator && m_options[i].hotkeyKey == key) {
            m_selectedIndex = i;
            triggerCurrent();
            event->accept();
            return;
        }
    }

    QDialog::keyPressEvent(event);
}

} // namespace MahadevERP
