#include "scale_manager.h"
#include "../database_manager.h"
#include <QApplication>
#include <QGuiApplication>
#include <QScreen>
#include <QFont>
#include <QFile>
#include <QDir>
#include <QTextStream>
#include <QTableView>
#include <QTableWidget>
#include <QHeaderView>
#include <QStyle>
#include <QDebug>
#include <cmath>

static QString getScaleConfigFilePath() {
    QDir dataDir("data");
    if (!dataDir.exists()) {
        dataDir.mkpath(".");
    }
    return dataDir.filePath("ui_scale.cfg");
}

int ScaleManager::getSavedScalePercent() {
    QString cfgPath = getScaleConfigFilePath();
    if (QFile::exists(cfgPath)) {
        QFile file(cfgPath);
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QTextStream in(&file);
            QString content = in.readAll().trimmed();
            bool ok = false;
            int val = content.toInt(&ok);
            if (ok && val >= 80 && val <= 250) {
                return val;
            }
        }
    }
    return 0;
}

void ScaleManager::saveScaleConfigFile(int percent) {
    QString cfgPath = getScaleConfigFilePath();
    QFile file(cfgPath);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
        QTextStream out(&file);
        out << percent << "\n";
    }
}

ScaleManager::ScaleManager() {
}

ScaleManager& ScaleManager::instance() {
    static ScaleManager s_instance;
    return s_instance;
}

void ScaleManager::init() {
    int percent = getSavedScalePercent();
    if (percent < 80 || percent > 250) {
        QString dbSetting = DatabaseManager::instance().getSetting("ui_scale_percent", "");
        if (!dbSetting.trimmed().isEmpty()) {
            percent = dbSetting.trimmed().toInt();
        }
    }
    if (percent < 80 || percent > 250) {
        percent = getRecommendedScalePercent();
    }
    setScalePercent(percent, false);
}

int ScaleManager::getRecommendedScalePercent() const {
    QScreen* screen = QGuiApplication::primaryScreen();
    if (screen) {
        int width = screen->geometry().width();
        int dpi = qRound(screen->logicalDotsPerInch());
        if (width >= 3400 || dpi >= 160) {
            return 150; // 4K UHD Display
        } else if (width >= 2400 || dpi >= 120) {
            return 125; // 27" QHD (2560x1440) Display
        }
    }
    return 100;
}

QVector<UiScaleOption> ScaleManager::availableScaleOptions() const {
    return {
        {100, "100% Standard", "Default size for 1080p & laptop screens"},
        {110, "110% Comfortable", "Slightly enlarged text & controls"},
        {125, "125% Large", "Recommended for 27\" (2560x1440) monitors"},
        {135, "135% High Clarity", "Optimized for large desktop displays"},
        {150, "150% 4K UHD", "Recommended for 4K displays & TVs"},
        {175, "175% Ultra Large", "Maximum visibility for distance viewing"},
        {200, "200% Double Scale", "2x magnification across interface"}
    };
}

void ScaleManager::setScalePercent(int percent, bool persist) {
    if (percent < 80) percent = 80;
    if (percent > 250) percent = 250;

    m_scalePercent = percent;
    m_scaleFactor = percent / 100.0;

    if (persist) {
        saveScaleConfigFile(percent);
        DatabaseManager::instance().setSetting("ui_scale_percent", QString::number(percent));
    }

    // 1. Universal Cross-Platform Scaled Font Configuration
    QFont appFont;
#ifdef Q_OS_WIN
    appFont.setFamilies({"Segoe UI", "Calibri", "Arial"});
    int basePt = 10;
#elif defined(Q_OS_MACOS)
    QFont::insertSubstitution("Segoe UI", "Helvetica Neue");
    appFont.setFamilies({"Helvetica Neue", "Arial"});
    int basePt = 12;
#else
    QFont::insertSubstitution("Segoe UI", "Noto Sans");
    appFont.setFamilies({"Noto Sans", "Liberation Sans", "DejaVu Sans", "Arial"});
    int basePt = 10;
#endif
    appFont.setStyleHint(QFont::SansSerif);
    int newPointSize = qMax(8, qRound(basePt * m_scaleFactor));
    appFont.setPointSize(newPointSize);
    QApplication::setFont(appFont);

    // 2. Scaled Global Stylesheet
    QString appStyle = generateGlobalStyleSheet(m_scaleFactor);
    if (qApp) {
        qApp->setStyleSheet(appStyle);
    }

    // 3. Dynamic Live Traversal of Widget Tree (Table Row Heights & Headers)
    applyLiveScaleToWidgetTree();

    emit scaleChanged(m_scaleFactor, m_scalePercent);
}

void ScaleManager::applyLiveScaleToWidgetTree() {
    if (!qApp) return;

    QWidgetList allWidgets = QApplication::allWidgets();
    int rowHeight = qMax(22, qRound(28 * m_scaleFactor));
    int headerHeight = qMax(24, qRound(32 * m_scaleFactor));

    for (QWidget* w : allWidgets) {
        if (!w) continue;

        if (QTableView* tv = qobject_cast<QTableView*>(w)) {
            if (tv->verticalHeader()) {
                tv->verticalHeader()->setDefaultSectionSize(rowHeight);
            }
            if (tv->horizontalHeader()) {
                tv->horizontalHeader()->setDefaultSectionSize(qRound(90 * m_scaleFactor));
            }
        }

        w->updateGeometry();
        w->update();
    }
}

void ScaleManager::zoomIn() {
    int next = m_scalePercent + 10;
    if (next > 200) next = 200;
    setScalePercent(next, true);
}

void ScaleManager::zoomOut() {
    int prev = m_scalePercent - 10;
    if (prev < 80) prev = 80;
    setScalePercent(prev, true);
}

void ScaleManager::resetScale() {
    setScalePercent(100, true);
}

int ScaleManager::scaleValue(int basePixel) const {
    return qMax(1, qRound(basePixel * m_scaleFactor));
}

int ScaleManager::scaleFontPt(int basePt) const {
    return qMax(6, qRound(basePt * m_scaleFactor));
}

QString ScaleManager::generateGlobalStyleSheet(double factor) const {
    int labelFontSize = qMax(11, qRound(13 * factor));
    int btnFontSize = qMax(10, qRound(12 * factor));
    int btnPaddingV = qMax(4, qRound(7 * factor));
    int btnPaddingH = qMax(8, qRound(14 * factor));
    int btnRadius = qMax(4, qRound(6 * factor));
    int inputPaddingV = qMax(3, qRound(6 * factor));
    int inputPaddingH = qMax(6, qRound(10 * factor));
    int comboItemHeight = qMax(20, qRound(26 * factor));

    return QString(
        "QDialog, QMessageBox, QInputDialog {"
        "  background-color: #FFFFFF;"
        "  color: #0F172A;"
        "}"
        "QDialog QLabel, QMessageBox QLabel, QInputDialog QLabel {"
        "  color: #0F172A;"
        "  background: transparent;"
        "  font-size: %1px;"
        "  font-weight: 600;"
        "}"
        "QDialog QPushButton, QMessageBox QPushButton, QInputDialog QPushButton {"
        "  background-color: #F1F5F9;"
        "  color: #0F172A;"
        "  border: 1px solid #CBD5E1;"
        "  border-radius: %2px;"
        "  padding: %3px %4px;"
        "  font-size: %5px;"
        "  font-weight: 700;"
        "  min-width: %6px;"
        "}"
        "QDialog QPushButton:hover, QMessageBox QPushButton:hover, QInputDialog QPushButton:hover {"
        "  background-color: #E2E8F0;"
        "  border-color: #94A3B8;"
        "}"
        "QDialog QPushButton:pressed, QMessageBox QPushButton:pressed, QInputDialog QPushButton:pressed {"
        "  background-color: #CBD5E1;"
        "}"
        "QComboBox QAbstractItemView {"
        "  background-color: #FFFFFF;"
        "  border: 1.5px solid #2563EB;"
        "  selection-background-color: #2563EB;"
        "  selection-color: #FFFFFF;"
        "  outline: none;"
        "} "
        "QComboBox QAbstractItemView::item, QComboBox QListView::item {"
        "  color: #0F172A;"
        "  padding: %7px %8px;"
        "  min-height: %9px;"
        "  font-size: %10px;"
        "} "
        "QComboBox QAbstractItemView::item:hover, QComboBox QListView::item:hover {"
        "  background-color: #EFF6FF;"
        "  color: #1D4ED8;"
        "} "
        "QComboBox QAbstractItemView::item:selected, QComboBox QListView::item:selected {"
        "  background-color: #2563EB;"
        "  color: #FFFFFF;"
        "}"
    )
    .arg(labelFontSize)
    .arg(btnRadius)
    .arg(btnPaddingV)
    .arg(btnPaddingH)
    .arg(btnFontSize)
    .arg(qRound(60 * factor))
    .arg(inputPaddingV)
    .arg(inputPaddingH)
    .arg(comboItemHeight)
    .arg(labelFontSize);
}
