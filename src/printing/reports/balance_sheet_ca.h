#pragma once

// CA-audited balance-sheet stationery (reference: audited 31.03.2025 report).
// Serif, underline-ruled, borderless two-sided layout with full ledger lists
// (never "As Per List Attached"); firm-only footer (no CA block, no UDIN,
// no Generated stamp, no Prepared/Checked triple).
//
// QTextDocument-safe: <u> text rules + table border/cellpadding attributes
// only. No CSS borders anywhere (proven not to render in this app's
// QTextDocument). Amounts are plain 2-decimal (no Rs, no commas), like the
// audited reference. Values come straight from BalanceSheetCalculator, so
// figures match the widget and the module byte-for-byte.

#include <QString>
#include <QStringList>
#include <QVector>
#include "../../engine/balance_sheet_calculator.h"

namespace ErpPrint {

/// Plain 2-decimal amount, audited-report style (61053466.20).
inline QString caAmount(double v) {
    return QString::number(v, 'f', 2);
}

/// One side's rows as 3-cell fragments (no <tr>), so both sides can be
/// zipped into a single flat 6-column table. Flat tables paginate cleanly
/// row-by-row; nested side tables get clipped mid-row at page breaks by
/// QTextDocument (ledger names cut off like "...Private Limited [Ka").
/// Layout per group:
/// - header fragment: group name + group total, BOTH underlined+bold;
/// - below it: that group's ledgers in NORMAL text (plain, never
///   underlined). Net profit/loss is a header fragment, so it is bold too.
inline QStringList balanceSheetHalfRows(const QVector<BalanceSheetItem>& groups) {
    QStringList rows;
    for (const BalanceSheetItem& g : groups) {
        rows.append(
            QString("<td valign='top'><u><b>%1</b></u></td><td></td>"
                    "<td valign='top' align='right' nowrap style='white-space:nowrap;'>"
                    "<u><b>%2</b></u></td>")
                .arg(g.name.toHtmlEscaped(), caAmount(g.amount)));
        for (const BalanceSheetItem& c : g.children) {
            rows.append(
                QString("<td valign='top'>%1</td>"
                        "<td valign='top' align='right' nowrap style='white-space:nowrap;'>%2</td>"
                        "<td></td>")
                    .arg(c.name.toHtmlEscaped(), caAmount(c.amount)));
        }
    }
    return rows;
}

/// "M/S MAHADEV RICE INDUSTRY" title form (reference header, all caps).
inline QString balanceSheetFirmTitle(const QString& firmName) {
    QString f = firmName.trimmed();
    if (f.startsWith("M/S", Qt::CaseInsensitive)) {
        f = f.mid(3).trimmed();
        while (f.startsWith(".") || f.startsWith(",")) f = f.mid(1).trimmed();
        if (f.isEmpty()) f = firmName.trimmed();
    }
    if (!f.isEmpty() && !f.startsWith("M/S", Qt::CaseInsensitive)) f = "M/S " + f;
    return f.toUpper().toHtmlEscaped();
}

/// "For M/s. Mahadev Rice Industry" footer form (reference signatory).
inline QString balanceSheetFirmForLine(const QString& firmName) {
    QString f = firmName.trimmed();
    if (f.startsWith("M/S", Qt::CaseInsensitive))
        f = f.mid(3).trimmed();
    while (f.startsWith(".") || f.startsWith(",")) f = f.mid(1).trimmed();
    if (f.isEmpty()) return QString("For the Firm");
    QStringList words = f.toLower().split(' ', Qt::SkipEmptyParts);
    for (int i = 0; i < words.size(); ++i) {
        if (!words[i].isEmpty())
            words[i][0] = words[i][0].toUpper();
    }
    return QString("For M/s. %1").arg(words.join(' ')).toHtmlEscaped();
}

/// Full CA balance-sheet document. asOnDots = "31.03.2025".
/// Single flat 6-column table (name/detail/total per side): rows break
/// across pages cleanly, long ledger names wrap in full and are never cut.
/// Grand totals share one bottom row, each under its side's total column,
/// visibly underlined. Columns 22/14/14 per side fit a full 12-digit figure
/// on one A4-portrait line.
inline QString balanceSheetCaHtml(const QString& firmName,
                                  const QString& asOnDots,
                                  const QVector<BalanceSheetItem>& liab,
                                  const QVector<BalanceSheetItem>& assets,
                                  double totLiab, double totAsset) {
    const QStringList L = balanceSheetHalfRows(liab);
    const QStringList A = balanceSheetHalfRows(assets);
    const int n = qMax(L.size(), A.size());
    const QString emptyHalf = "<td></td><td></td><td></td>";

    QString rows =
        "<tr><td valign='top' width='22%'><u><b>Liabilities</b></u></td>"
        "<td valign='top' width='28%' colspan='2' align='center'><u><b>Amount</b></u></td>"
        "<td valign='top' width='22%'><u><b>Assets</b></u></td>"
        "<td valign='top' width='28%' colspan='2' align='center'><u><b>Amount</b></u></td></tr>";
    rows.reserve(rows.size() + n * 220);
    for (int i = 0; i < n; ++i) {
        rows += QString("<tr>%1%2</tr>")
                    .arg(i < L.size() ? L.at(i) : emptyHalf,
                         i < A.size() ? A.at(i) : emptyHalf);
    }
    rows += QString("<tr><td width='22%'></td><td width='14%'></td>"
                    "<td valign='top' align='right' nowrap style='white-space:nowrap;'>"
                    "<b><u>%1</u></b></td>"
                    "<td width='22%'></td><td width='14%'></td>"
                    "<td valign='top' align='right' nowrap style='white-space:nowrap;'>"
                    "<b><u>%2</u></b></td></tr>")
                .arg(caAmount(totLiab), caAmount(totAsset));

    const QString body =
        QString("<div align='center' style='font-size:14pt;'><b><u>%1</u></b></div>"
                "<div align='center' style='font-size:12pt;'><b><u>BALANCE SHEET FOR THE PERIOD ENDING AS ON %2</u></b></div>"
                "<br>"
                "<table width='100%' border='0' cellspacing='0' cellpadding='2'>"
                "%3"
                "</table>"
                "<br><br>"
                "<table width='100%' border='0' cellspacing='0' cellpadding='2'>"
                "<tr>"
                "<td width='50%' valign='top'><b>%4</b><br><br><br><br>Authorised Signatory</td>"
                "<td width='50%'></td>"
                "</tr>"
                "</table>")
            .arg(balanceSheetFirmTitle(firmName),
                 asOnDots.toHtmlEscaped(),
                 rows,
                 balanceSheetFirmForLine(firmName));

    return QString("<!DOCTYPE html><html><head><meta charset=\"utf-8\">"
                   "<style>"
                   "body{font-family:'Times New Roman',Times,serif;color:#000000;"
                   "font-size:10pt;margin:0;padding:10px;}"
                   "table{width:100%;border-collapse:collapse;}"
                   "td{font-size:10pt;vertical-align:top;}"
                   "</style></head><body>%1</body></html>")
        .arg(body);
}

} // namespace ErpPrint
