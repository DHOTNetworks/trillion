#include "scale_settings_dialog.h"
#include "../services/scale_manager.h"
#include <QFrame>
#include <QGridLayout>
#include <QScrollArea>
#include <QGuiApplication>
#include <QScreen>
#include <QFont>

ScaleSettingsDialog::ScaleSettingsDialog(QWidget* parent)
    : QDialog(parent)
{
    m_initialPercent = ScaleManager::instance().currentScalePercent();
    m_selectedPercent = m_initialPercent;

    setWindowTitle("Display & UI Scaling Settings");
    setFixedWidth(560);
    setModal(true);

    setupUi();
    updateScaleDisplay(m_selectedPercent);
}

void ScaleSettingsDialog::setupUi() {
    setAttribute(Qt::WA_StyledBackground, true);
    setStyleSheet("ScaleSettingsDialog { background-color: #F8FAFC; }");

    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(18, 16, 18, 16);
    mainLayout->setSpacing(12);

    // ========================================================================
    // 1. HEADER CARD (Compact Title & Auto-Detect Button)
    // ========================================================================
    QFrame* headerCard = new QFrame(this);
    headerCard->setStyleSheet("background-color: #FFFFFF; border: 1.5px solid #E2E8F0; border-radius: 8px;");
    QHBoxLayout* headerLayout = new QHBoxLayout(headerCard);
    headerLayout->setContentsMargins(14, 10, 14, 10);
    headerLayout->setSpacing(10);

    QVBoxLayout* titleBox = new QVBoxLayout();
    titleBox->setSpacing(2);

    QLabel* titleLabel = new QLabel("🖥️ Display & UI Scaling Settings", headerCard);
    titleLabel->setStyleSheet("font-size: 16px; font-weight: 800; color: #0F172A; border: none; background: transparent;");
    titleBox->addWidget(titleLabel);

    QLabel* subLabel = new QLabel("Adjust interface size live for 27\" Desktop or 4K monitors.", headerCard);
    subLabel->setStyleSheet("font-size: 11px; font-weight: 500; color: #64748B; border: none; background: transparent;");
    titleBox->addWidget(subLabel);
    headerLayout->addLayout(titleBox);

    headerLayout->addStretch(1);

    m_autoDetectBtn = new QPushButton("🎯 Auto-Detect", headerCard);
    m_autoDetectBtn->setFixedHeight(32);
    m_autoDetectBtn->setCursor(Qt::PointingHandCursor);
    m_autoDetectBtn->setStyleSheet(
        "QPushButton { background-color: #EFF6FF; border: 1.5px solid #3B82F6; border-radius: 6px; padding: 0px 12px; font-weight: 700; color: #1D4ED8; font-size: 11.5px; }"
        "QPushButton:hover { background-color: #DBEAFE; border-color: #2563EB; }"
    );
    connect(m_autoDetectBtn, &QPushButton::clicked, this, &ScaleSettingsDialog::onAutoDetectClicked);
    headerLayout->addWidget(m_autoDetectBtn);

    mainLayout->addWidget(headerCard);

    // ========================================================================
    // 2. INTERACTIVE SLIDER & STEPPERS
    // ========================================================================
    QFrame* sliderCard = new QFrame(this);
    sliderCard->setStyleSheet("background-color: #FFFFFF; border: 1.5px solid #E2E8F0; border-radius: 8px;");
    QVBoxLayout* sliderCardLayout = new QVBoxLayout(sliderCard);
    sliderCardLayout->setContentsMargins(14, 12, 14, 12);
    sliderCardLayout->setSpacing(10);

    QHBoxLayout* sliderTopRow = new QHBoxLayout();
    QLabel* sliderTitle = new QLabel("Fine Adjustment Slider", sliderCard);
    sliderTitle->setStyleSheet("font-size: 13px; font-weight: 700; color: #334155; border: none; background: transparent;");
    sliderTopRow->addWidget(sliderTitle);

    sliderTopRow->addStretch(1);

    m_currentPercentBadge = new QLabel(QString("%1%").arg(m_selectedPercent), sliderCard);
    m_currentPercentBadge->setStyleSheet(
        "font-size: 15px; font-weight: 800; color: #2563EB; background-color: #EFF6FF; "
        "border: 1.5px solid #BFDBFE; border-radius: 6px; padding: 2px 12px;"
    );
    sliderTopRow->addWidget(m_currentPercentBadge);
    sliderCardLayout->addLayout(sliderTopRow);

    // Stepper + Slider Row
    QHBoxLayout* stepperRow = new QHBoxLayout();
    stepperRow->setSpacing(8);

    m_minusBtn = new QPushButton("−", sliderCard);
    m_minusBtn->setFixedSize(32, 32);
    m_minusBtn->setCursor(Qt::PointingHandCursor);
    m_minusBtn->setStyleSheet(
        "QPushButton { background-color: #F1F5F9; border: 1px solid #CBD5E1; border-radius: 6px; font-size: 18px; font-weight: 800; color: #334155; }"
        "QPushButton:hover { background-color: #E2E8F0; color: #0F172A; }"
    );
    connect(m_minusBtn, &QPushButton::clicked, this, &ScaleSettingsDialog::onStepDownClicked);
    stepperRow->addWidget(m_minusBtn);

    m_scaleSlider = new QSlider(Qt::Horizontal, sliderCard);
    m_scaleSlider->setRange(80, 200);
    m_scaleSlider->setSingleStep(5);
    m_scaleSlider->setPageStep(10);
    m_scaleSlider->setValue(m_selectedPercent);
    m_scaleSlider->setStyleSheet(
        "QSlider::groove:horizontal { height: 6px; background: #E2E8F0; border-radius: 3px; }"
        "QSlider::sub-page:horizontal { background: #3B82F6; border-radius: 3px; }"
        "QSlider::handle:horizontal { background: #2563EB; border: 2px solid #FFFFFF; width: 18px; height: 18px; margin-top: -6px; margin-bottom: -6px; border-radius: 9px; }"
        "QSlider::handle:horizontal:hover { background: #1D4ED8; }"
    );
    connect(m_scaleSlider, &QSlider::valueChanged, this, &ScaleSettingsDialog::onSliderValueChanged);
    stepperRow->addWidget(m_scaleSlider);

    m_plusBtn = new QPushButton("+", sliderCard);
    m_plusBtn->setFixedSize(32, 32);
    m_plusBtn->setCursor(Qt::PointingHandCursor);
    m_plusBtn->setStyleSheet(
        "QPushButton { background-color: #F1F5F9; border: 1px solid #CBD5E1; border-radius: 6px; font-size: 18px; font-weight: 800; color: #334155; }"
        "QPushButton:hover { background-color: #E2E8F0; color: #0F172A; }"
    );
    connect(m_plusBtn, &QPushButton::clicked, this, &ScaleSettingsDialog::onStepUpClicked);
    stepperRow->addWidget(m_plusBtn);

    sliderCardLayout->addLayout(stepperRow);

    // Live Sample Text Preview
    QFrame* previewBox = new QFrame(sliderCard);
    previewBox->setStyleSheet("background-color: #F8FAFC; border: 1px dashed #CBD5E1; border-radius: 6px;");
    QHBoxLayout* previewLayout = new QHBoxLayout(previewBox);
    previewLayout->setContentsMargins(10, 6, 10, 6);
    previewLayout->setSpacing(8);

    QLabel* previewTag = new QLabel("PREVIEW:", previewBox);
    previewTag->setStyleSheet("font-size: 10px; font-weight: 800; color: #64748B; border: none; background: transparent;");
    previewLayout->addWidget(previewTag);

    m_previewSampleLabel = new QLabel("M/s Balaji Rice Mill  •  ₹ 1,42,85,690.00 Dr", previewBox);
    m_previewSampleLabel->setStyleSheet("font-weight: 700; color: #0F172A; border: none; background: transparent;");
    previewLayout->addWidget(m_previewSampleLabel);
    previewLayout->addStretch(1);

    sliderCardLayout->addWidget(previewBox);

    mainLayout->addWidget(sliderCard);

    // ========================================================================
    // 3. PRESET TILES GRID (2 Columns of Clean Modern Cards)
    // ========================================================================
    QLabel* presetHeader = new QLabel("Quick Presets", this);
    presetHeader->setStyleSheet("font-size: 12px; font-weight: 800; color: #475569; padding-left: 2px;");
    mainLayout->addWidget(presetHeader);

    QFrame* presetsFrame = new QFrame(this);
    presetsFrame->setStyleSheet("background-color: transparent; border: none;");
    QGridLayout* grid = new QGridLayout(presetsFrame);
    grid->setContentsMargins(0, 0, 0, 0);
    grid->setSpacing(8);

    struct PresetInfo {
        int percent;
        QString title;
        QString desc;
    };
    QVector<PresetInfo> presets = {
        {100, "100% Standard", "Laptop & 1080p screens"},
        {110, "110% Comfortable", "Slightly enlarged text"},
        {125, "125% Large", "Recommended for 27\" (1440p)"},
        {135, "135% High Clarity", "Large desktop displays"},
        {150, "150% 4K UHD", "Recommended for 4K displays"},
        {175, "175% Ultra Large", "Maximum visibility"}
    };

    for (int i = 0; i < presets.size(); ++i) {
        const auto& p = presets.at(i);
        QPushButton* btn = new QPushButton(presetsFrame);
        btn->setFixedHeight(46);
        btn->setCursor(Qt::PointingHandCursor);

        QVBoxLayout* bLayout = new QVBoxLayout(btn);
        bLayout->setContentsMargins(10, 4, 10, 4);
        bLayout->setSpacing(1);

        QLabel* tLbl = new QLabel(p.title, btn);
        tLbl->setObjectName("title");
        tLbl->setStyleSheet("font-size: 12px; font-weight: 800; border: none; background: transparent;");
        bLayout->addWidget(tLbl);

        QLabel* dLbl = new QLabel(p.desc, btn);
        dLbl->setObjectName("desc");
        dLbl->setStyleSheet("font-size: 10px; font-weight: 500; border: none; background: transparent;");
        bLayout->addWidget(dLbl);

        connect(btn, &QPushButton::clicked, this, [this, pct = p.percent]() {
            onPresetButtonClicked(pct);
        });

        m_presetButtons.insert(p.percent, btn);
        grid->addWidget(btn, i / 2, i % 2);
    }

    mainLayout->addWidget(presetsFrame);

    // ========================================================================
    // 4. ACTION BUTTONS FOOTER
    // ========================================================================
    QHBoxLayout* footerLayout = new QHBoxLayout();
    footerLayout->setContentsMargins(0, 4, 0, 0);
    footerLayout->setSpacing(10);

    m_resetBtn = new QPushButton("Reset (100%)", this);
    m_resetBtn->setFixedHeight(36);
    m_resetBtn->setCursor(Qt::PointingHandCursor);
    m_resetBtn->setStyleSheet(
        "QPushButton { background-color: #FFFFFF; border: 1.5px solid #CBD5E1; border-radius: 6px; padding: 0px 14px; font-weight: 700; color: #475569; font-size: 11.5px; }"
        "QPushButton:hover { background-color: #F1F5F9; border-color: #94A3B8; }"
    );
    connect(m_resetBtn, &QPushButton::clicked, this, &ScaleSettingsDialog::onResetClicked);
    footerLayout->addWidget(m_resetBtn);

    footerLayout->addStretch(1);

    m_cancelBtn = new QPushButton("Cancel", this);
    m_cancelBtn->setFixedHeight(36);
    m_cancelBtn->setCursor(Qt::PointingHandCursor);
    m_cancelBtn->setStyleSheet(
        "QPushButton { background-color: #FFFFFF; border: 1.5px solid #CBD5E1; border-radius: 6px; padding: 0px 16px; font-weight: 700; color: #64748B; font-size: 12px; }"
        "QPushButton:hover { background-color: #F1F5F9; border-color: #94A3B8; color: #334155; }"
    );
    connect(m_cancelBtn, &QPushButton::clicked, this, &ScaleSettingsDialog::onCancelClicked);
    footerLayout->addWidget(m_cancelBtn);

    m_saveBtn = new QPushButton("✓ Apply & Save", this);
    m_saveBtn->setFixedHeight(36);
    m_saveBtn->setCursor(Qt::PointingHandCursor);
    m_saveBtn->setStyleSheet(
        "QPushButton { background-color: #16A34A; border: none; border-radius: 6px; padding: 0px 20px; font-weight: 800; color: #FFFFFF; font-size: 12px; }"
        "QPushButton:hover { background-color: #15803D; }"
    );
    connect(m_saveBtn, &QPushButton::clicked, this, &ScaleSettingsDialog::onApplyAndSaveClicked);
    footerLayout->addWidget(m_saveBtn);

    mainLayout->addLayout(footerLayout);

    updatePresetButtonStyles();
}

void ScaleSettingsDialog::updateScaleDisplay(int percent) {
    m_selectedPercent = percent;
    if (m_currentPercentBadge) {
        m_currentPercentBadge->setText(QString("%1%").arg(percent));
    }

    if (m_scaleSlider && m_scaleSlider->value() != percent) {
        m_scaleSlider->blockSignals(true);
        m_scaleSlider->setValue(percent);
        m_scaleSlider->blockSignals(false);
    }

    updatePresetButtonStyles();

    // Live preview applied dynamically across the app
    ScaleManager::instance().setScalePercent(percent, false);
}

void ScaleSettingsDialog::updatePresetButtonStyles() {
    for (auto it = m_presetButtons.begin(); it != m_presetButtons.end(); ++it) {
        int pct = it.key();
        QPushButton* btn = it.value();
        QLabel* tLbl = btn->findChild<QLabel*>("title");
        QLabel* dLbl = btn->findChild<QLabel*>("desc");

        if (pct == m_selectedPercent) {
            btn->setStyleSheet("background-color: #EFF6FF; border: 2px solid #2563EB; border-radius: 6px; text-align: left;");
            if (tLbl) tLbl->setStyleSheet("color: #1D4ED8; font-size: 12px; font-weight: 800; border: none; background: transparent;");
            if (dLbl) dLbl->setStyleSheet("color: #3B82F6; font-size: 10px; font-weight: 600; border: none; background: transparent;");
        } else {
            btn->setStyleSheet("background-color: #FFFFFF; border: 1.5px solid #E2E8F0; border-radius: 6px; text-align: left;");
            if (tLbl) tLbl->setStyleSheet("color: #0F172A; font-size: 12px; font-weight: 700; border: none; background: transparent;");
            if (dLbl) dLbl->setStyleSheet("color: #64748B; font-size: 10px; font-weight: 500; border: none; background: transparent;");
        }
    }
}

void ScaleSettingsDialog::onSliderValueChanged(int value) {
    updateScaleDisplay(value);
}

void ScaleSettingsDialog::onPresetButtonClicked(int percent) {
    updateScaleDisplay(percent);
}

void ScaleSettingsDialog::onStepDownClicked() {
    int next = m_selectedPercent - 5;
    if (next < 80) next = 80;
    updateScaleDisplay(next);
}

void ScaleSettingsDialog::onStepUpClicked() {
    int next = m_selectedPercent + 5;
    if (next > 200) next = 200;
    updateScaleDisplay(next);
}

void ScaleSettingsDialog::onAutoDetectClicked() {
    int rec = ScaleManager::instance().getRecommendedScalePercent();
    updateScaleDisplay(rec);
}

void ScaleSettingsDialog::onResetClicked() {
    updateScaleDisplay(100);
}

void ScaleSettingsDialog::onApplyAndSaveClicked() {
    ScaleManager::instance().setScalePercent(m_selectedPercent, true);
    accept();
}

void ScaleSettingsDialog::onCancelClicked() {
    ScaleManager::instance().setScalePercent(m_initialPercent, false);
    reject();
}
