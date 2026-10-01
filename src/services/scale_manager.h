#pragma once

#include <QObject>
#include <QString>
#include <QVector>

struct UiScaleOption {
    int percent;
    QString label;
    QString recommendation;
};

class ScaleManager : public QObject {
    Q_OBJECT

public:
    static ScaleManager& instance();

    // Fast static methods for early pre-QApplication startup
    static int getSavedScalePercent();
    static void saveScaleConfigFile(int percent);

    void init();
    
    int currentScalePercent() const { return m_scalePercent; }
    double currentScaleFactor() const { return m_scaleFactor; }
    
    QVector<UiScaleOption> availableScaleOptions() const;

    void setScalePercent(int percent, bool persist = true);
    void zoomIn();
    void zoomOut();
    void resetScale();
    int getRecommendedScalePercent() const;

    // Helper functions for responsive metrics
    int scaleValue(int basePixel) const;
    int scaleFontPt(int basePt) const;

signals:
    void scaleChanged(double factor, int percent);

private:
    ScaleManager();
    ~ScaleManager() override = default;
    ScaleManager(const ScaleManager&) = delete;
    ScaleManager& operator=(const ScaleManager&) = delete;

    QString generateGlobalStyleSheet(double factor) const;
    void applyLiveScaleToWidgetTree();

    int m_scalePercent = 100;
    double m_scaleFactor = 1.0;
};
