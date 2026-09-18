#pragma once

#include <QString>
#include <QWidget>
#include <QPrinter>
#include "../engine/balance_sheet_calculator.h"

class BalanceSheetPrinter {
public:
    // Pure QPainter on QPrinter vector renderer for exact A4 Two-Column layout (1-2 pages max)
    static bool render(QPrinter& printer, const BalanceSheetData& data);

    // Export Balance Sheet to crisp A4 PDF file using QPrinter (returns absolute file path)
    static QString exportPdf(const BalanceSheetData& data, const QString& customPath = "");

    // Export Balance Sheet to CSV file (returns absolute file path)
    static QString exportCsv(const BalanceSheetData& data, const QString& customPath = "");

    // Print Balance Sheet directly to physical/system printer via QPrintDialog & QPrinter
    static bool print(const BalanceSheetData& data, QWidget* parent = nullptr);

    // Legacy HTML preview helper if needed
    static QString generateHtml(const BalanceSheetData& data);
};
