#pragma once

#include <QDialog>
#include <QListWidget>
#include <QLabel>
#include <QPushButton>
#include <QVector>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QKeyEvent>
#include "../engine/fiscal_year_helper.h"

struct DateColumnItem {
    QString displayDate; // "01-04-2026" or "31-03-2027"
    QString isoDate;     // "2026-04-01" or "2027-03-31"
    QString fyName;      // "FY 2026-27"
    bool isStart = true;
};

class DateListWidget : public QListWidget {
    Q_OBJECT
public:
    explicit DateListWidget(bool isStartColumn, QWidget* parent = nullptr);

signals:
    void switchColumnRequested();

protected:
    void keyPressEvent(QKeyEvent* event) override;
    void focusInEvent(QFocusEvent* event) override;

private:
    bool m_isStartColumn = true;
};

struct DateRowWidgetInfo {
    QWidget* container = nullptr;
    QLabel* radioLabel = nullptr;
    QLabel* dateLabel = nullptr;
    QLabel* fyBadgeLabel = nullptr;
};

class AccountingPeriodDialog : public QDialog {
    Q_OBJECT

public:
    explicit AccountingPeriodDialog(QWidget* parent = nullptr);
    ~AccountingPeriodDialog() override;

    QString selectedFromIso() const { return m_selectedFromIso; }
    QString selectedToIso() const { return m_selectedToIso; }
    QString selectedFyLabel() const { return m_selectedFyLabel; }

    // Static helper to execute dialog and immediately apply selection globally
    static bool selectAndApplyGlobalPeriod(QWidget* parent = nullptr,
                                           QString* outFromIso = nullptr,
                                           QString* outToIso = nullptr,
                                           QString* outFyLabel = nullptr);

public slots:
    void applyPeriod();
    void setPreset(const QString& fyName);

protected:
    void keyPressEvent(QKeyEvent* event) override;
    bool eventFilter(QObject* obj, QEvent* event) override;

private slots:
    void onFromRowChanged(int row);
    void onToRowChanged(int row);
    void updatePreview();

private:
    void setupUi();
    void loadYearsFromDatabase();
    void updateCheckedStates();
    DateRowWidgetInfo createDateRowWidget(const DateColumnItem& item);

    QList<FiscalYearInfo> m_availableFys;
    QVector<DateColumnItem> m_fromItems;
    QVector<DateColumnItem> m_toItems;
    QVector<DateRowWidgetInfo> m_fromRowWidgets;
    QVector<DateRowWidgetInfo> m_toRowWidgets;

    QString m_selectedFromIso;
    QString m_selectedToIso;
    QString m_selectedFyLabel;

    QHBoxLayout* m_presetsLayout = nullptr;
    QList<QPushButton*> m_presetButtons;
    DateListWidget* m_fromList = nullptr;
    DateListWidget* m_toList = nullptr;

    QLabel* m_previewDateRangeLabel = nullptr;
    QLabel* m_previewFyBadge = nullptr;
    QPushButton* m_applyBtn = nullptr;
    QPushButton* m_cancelBtn = nullptr;

    bool m_updating = false;
};
