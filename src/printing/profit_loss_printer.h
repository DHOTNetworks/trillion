#pragma once

#include <QString>
#include <QWidget>
#include <QPrinter>
#include "../engine/profit_loss_calculator.h"

class ProfitLossPrinter {
public:
    // Pure QPainter on QPrinter vector renderer for exact A4 Two-Column layout (1-2 pages max)
    static bool render(QPrinter& printer, const ProfitLossData& data);

    // Export Statement to PDF file (returns absolute file path)
    static QString exportPdf(const ProfitLossData& data, const QString& customPath = "");

    // Export Statement to CSV file (returns absolute file path)
    static QString exportCsv(const ProfitLossData& data, const QString& customPath = "");

    // Print Statement directly to physical/system printer
    static bool print(const ProfitLossData& data, QWidget* parent = nullptr);

    // Legacy HTML preview helper if needed
    static QString generateHtml(const ProfitLossData& data);
};
