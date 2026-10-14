#pragma once

// Shared print chrome for ALL reports — one CSS + firm header + footer so
// every PDF/ODF/Excel export looks like it came from one product (Tally-grade
// determinism). Renderers in src/printing/reports/* compose with these; no
// renderer may inline its own page CSS.
//
// Two surfaces: screen widgets (reportCss, keep) and PRINT documents
// (printCss + letterhead/title/signatory below) — print is styled like a
// Big-Four CA-firm statement: letterhead + double rule, centered title,
// bordered tables with navy headers, ruled totals, signature blocks.
//
// QtCore-only (QString builders) — no QPrinter dependency, headless-testable.

#include <QString>
#include <QStringList>
#include <QVariantMap>
#include <QDateTime>

namespace ErpPrint {

/// Single page stylesheet for every report document.
inline QString reportCss() {
    return QString(
        "body{font-family:'Segoe UI',-apple-system,Roboto,sans-serif;color:#0F172A;"
        "font-size:12px;margin:0;padding:16px;}"
        "h1{font-size:16px;font-weight:800;margin:0 0 2px 0;color:#0F172A;}"
        ".sub{font-size:11px;color:#64748B;margin:0 0 12px 0;}"
        ".meta{font-size:11px;color:#475569;margin:0 0 12px 0;}"
        "table{width:100%;border-collapse:collapse;}"
        "th{background:#0F172A;color:#fff;font-size:12px;font-weight:700;"
        "text-align:left;padding:8px 10px;}"
        "th.num,td.num{text-align:right;font-variant-numeric:tabular-nums;}"
        "th.ctr,td.ctr{text-align:center;}"
        "td{padding:7px 10px;border-bottom:1px solid #E2E8F0;}"
        "tr:nth-child(even) td{background:#F8FAFC;}"
        "tr.total td{background:#F1F5F9;font-weight:800;border-top:2px solid #CBD5E1;}"
        ".footer{margin-top:12px;font-size:10px;color:#94A3B8;}"
        ".badge{display:inline-block;padding:2px 8px;border-radius:6px;font-weight:700;font-size:11px;}"
        ".ok{background:#F0FDF4;color:#16A34A;border:1px solid #86EFAC;}"
        ".bad{background:#FEF2F2;color:#DC2626;border:1px solid #FECACA;}");
}

/// Firm letterhead block. firm keys: name, address, gstin, phone.
inline QString firmHeaderHtml(const QString& firmName, const QString& firmMeta,
                              const QString& title, const QString& subtitle) {
    return QString("<h1>%1</h1><div class=\"sub\">%2</div>"
                   "<h1>%3</h1><div class=\"sub\">%4</div>")
        .arg(firmName.toHtmlEscaped(), firmMeta.toHtmlEscaped(),
             title.toHtmlEscaped(), subtitle.toHtmlEscaped());
}

inline QString metaLineHtml(const QString& text) {
    return QString("<div class=\"meta\">%1</div>").arg(text.toHtmlEscaped());
}

/// Column header row from a header list + per-column css class list
/// ("", "num", "ctr").
inline QString theadHtml(const QStringList& headers, const QStringList& classes = {}) {
    QString h = "<thead><tr>";
    for (int i = 0; i < headers.size(); ++i) {
        const QString c = (i < classes.size() && !classes[i].isEmpty())
            ? QString(" class=\"%1\"").arg(classes[i]) : QString();
        h += QString("<th%1>%2</th>").arg(c, headers[i].toHtmlEscaped());
    }
    return h + "</tr></thead>";
}

inline QString footerHtml(const QString& note) {
    return QString("<div class=\"footer\">%1</div>").arg(note.toHtmlEscaped());
}

/// Full document wrapper: doctype + css + body.
inline QString wrapDocument(const QString& body) {
    return QString("<!DOCTYPE html><html><head><meta charset=\"utf-8\">"
                   "<style>%1</style></head><body>%2</body></html>")
        .arg(reportCss(), body);
}

// ---- CA-firm print stationery (QTextDocument-safe subset) --------------------

/// Print stylesheet: bordered tables, navy headers, ruled totals.
inline QString printCss() {
    return QString(
        "body{font-family:'Segoe UI','Helvetica Neue',Arial,sans-serif;color:#0F172A;"
        "font-size:9pt;margin:0;padding:10px;}"
        ".lh{text-align:center;margin:0 0 2px 0;}"
        ".lh-name{font-size:17pt;font-weight:800;letter-spacing:0.3px;}"
        ".lh-line{font-size:9pt;color:#334155;}"
        ".rule{border:none;border-top:2px solid #0F172A;margin:4px 0 1px 0;}"
        ".rule2{border:none;border-top:1px solid #0F172A;margin:0 0 8px 0;}"
        ".title{text-align:center;font-size:13pt;font-weight:800;margin:6px 0 1px 0;}"
        ".period{text-align:center;font-size:9.5pt;color:#334155;margin:0 0 8px 0;}"
        "table.grid{width:100%;border-collapse:collapse;margin-bottom:8px;}"
        "table.grid th{background:#0F172A;color:#fff;font-size:9pt;font-weight:700;"
        "text-align:center;padding:5px 6px;border:0.5pt solid #0F172A;}"
        "table.grid td{font-size:9pt;padding:4px 6px;border:0.5pt solid #64748B;}"
        "td.num{text-align:right;font-variant-numeric:tabular-nums;white-space:nowrap;}"
        "td.ctr{text-align:center;}"
        "tr.total td{font-weight:800;background:#F1F5F9;"
        "border-top:1.5pt solid #0F172A;border-bottom:3px double #0F172A;}"
        "tr.diffr td{font-weight:800;color:#DC2626;}"
        ".dayband{background:#0F172A;color:#fff;font-size:9.5pt;font-weight:700;"
        "padding:4px 8px;margin:10px 0 0 0;border:0.5pt solid #0F172A;}"
        ".notes{font-size:8.5pt;color:#334155;margin:6px 0;}"
        ".sign{width:100%;margin-top:18px;border-collapse:collapse;}"
        ".sign td{width:33%;text-align:center;font-size:9pt;vertical-align:bottom;"
        "padding-top:28px;border:none;border-top:0.5pt solid #0F172A;}"
        ".genfoot{margin-top:10px;font-size:7.5pt;color:#94A3B8;text-align:right;}"
        ".grade{font-size:7.5pt;color:#475569;}"
        ".copytag{display:inline-block;border:1px solid #0F172A;padding:2px 10px;"
        "font-size:9pt;font-weight:700;margin:4px 0;}"
        "table.meta{width:100%;border-collapse:collapse;margin-bottom:8px;}"
        "table.meta td{border:0.5pt solid #64748B;font-size:9pt;padding:5px 7px;vertical-align:top;}"
        ".words{font-size:9pt;}");
}

/// Firm letterhead from a getFirmProfile()-shaped map (skips empty lines,
// dedupes city/state already present inside the address line).
inline QString letterheadHtml(const QVariantMap& firm) {
    auto esc = [](const QVariant& v) { return v.toString().trimmed().toHtmlEscaped(); };
    QString h = QString("<div class=\"lh\"><div class=\"lh-name\">%1</div>")
                    .arg(esc(firm.value("firm_name")));
    QString addr = esc(firm.value("address"));
    addr.replace(",", ", ");
    while (addr.contains(",  ")) addr.replace(",  ", ", ");
    const QString city = esc(firm.value("city"));
    const QString stateCore = esc(firm.value("state")).split('(').first().trimmed();
    if (!city.isEmpty() && !addr.contains(city, Qt::CaseInsensitive))
        addr += (addr.isEmpty() ? "" : ", ") + city;
    if (!stateCore.isEmpty() && !addr.contains(stateCore, Qt::CaseInsensitive))
        addr += (addr.isEmpty() ? "" : ", ") + stateCore;
    if (!addr.isEmpty()) h += QString("<div class=\"lh-line\">%1</div>").arg(addr);
    QString contact;
    if (!esc(firm.value("phone")).isEmpty()) contact += "Phone: " + esc(firm.value("phone"));
    if (!esc(firm.value("email")).isEmpty()) contact += (contact.isEmpty() ? QString() : QString(" &nbsp;|&nbsp; ")) + "Email: " + esc(firm.value("email"));
    if (!contact.isEmpty()) h += QString("<div class=\"lh-line\">%1</div>").arg(contact);
    QString reg;
    if (!esc(firm.value("gstin")).isEmpty()) reg += "GSTIN: " + esc(firm.value("gstin"));
    if (!esc(firm.value("pan")).isEmpty()) reg += (reg.isEmpty() ? QString() : QString(" &nbsp;|&nbsp; ")) + "PAN: " + esc(firm.value("pan"));
    if (!reg.isEmpty()) h += QString("<div class=\"lh-line\">%1</div>").arg(reg);
    return h + "</div><hr class=\"rule\"/><hr class=\"rule2\"/>";
}

inline QString titleBlockHtml(const QString& title, const QString& periodLine) {
    return QString("<div class=\"title\">%1</div><div class=\"period\">%2</div>")
        .arg(title.toHtmlEscaped(), periodLine.toHtmlEscaped());
}

/// Signature block for STATEMENTS/REGISTERS only (trial, daybook, ledger,
/// registers, P&L, balance sheet...). Underscore rules (not CSS borders) so
/// the lines survive every QTextDocument/Qt version. Vouchers and invoices
/// must NOT use this — they get contextual signatures (voucherSignHtml) or
/// the Busy-style invoice footer (invoiceFooterHtml) instead.
inline QString signatoryHtml(const QString& firmName = "") {
    QString forLine;
    if (!firmName.trimmed().isEmpty())
        forLine = QString("For %1<br>").arg(firmName.trimmed().toHtmlEscaped());
    return QString("<br><table width=\"100%\" border=\"0\" cellspacing=\"0\" cellpadding=\"4\"><tr>"
                   "<td width=\"33%\" align=\"center\" valign=\"bottom\">___________________<br><b>Prepared By</b></td>"
                   "<td width=\"33%\" align=\"center\" valign=\"bottom\">___________________<br><b>Checked By</b></td>"
                   "<td width=\"34%\" align=\"center\" valign=\"bottom\">%1___________________<br><b>Authorised Signatory</b></td>"
                   "</tr></table>").arg(forLine);
}

/// "For <firm>" voucher label, with fallback when the profile is empty.
inline QString forFirmLabel(const QVariantMap& firm) {
    const QString name = firm.value("firm_name").toString().trimmed();
    return name.isEmpty() ? QString("Authorised Signatory") : QString("For %1").arg(name);
}

/// Contextual voucher signature strip (transaction documents only):
/// e.g. {"Prepared By", "Passed By", "Receiver's Signature"} or
/// {"Buyer's Signature", "For X"}. No Generated footer on vouchers.
inline QString voucherSignHtml(const QStringList& labels) {
    QString t = "<br><table width=\"100%\" border=\"0\" cellspacing=\"0\" cellpadding=\"4\"><tr>";
    const int n = qMax(1, labels.size());
    const int w = 100 / n;
    for (int i = 0; i < labels.size(); ++i) {
        t += QString("<td width=\"%1%\" align=\"center\" valign=\"bottom\">___________________<br><b>%2</b></td>")
                 .arg(w).arg(labels[i].toHtmlEscaped());
    }
    return t + "</tr></table>";
}

/// Top document bar for invoices (Busy standard):
/// Page No. 1 of 1 | TITLE | copy tag. Bordered so it survives QTextDocument.
inline QString docBarHtml(const QString& title, const QString& copyTag) {
    return QString("<table width=\"100%\" border=\"0\" cellspacing=\"1\" cellpadding=\"4\" bgcolor=\"#94A3B8\">"
                   "<tr><td width=\"25%\" align=\"left\" bgcolor=\"#FFFFFF\">Page No. 1 of 1</td>"
                   "<td width=\"50%\" align=\"center\" bgcolor=\"#FFFFFF\"><b>%1</b></td>"
                   "<td width=\"25%\" align=\"right\" bgcolor=\"#FFFFFF\">%2</td></tr></table>")
        .arg(title.toHtmlEscaped(), copyTag.toHtmlEscaped());
}

/// Busy-style invoice footer: Terms & bank & For-firm signature.
/// No Prepared/Checked triple, no Generated stamp — invoices are governed
/// by their Terms + authorised signature, like the reference Bill of Supply.
inline QString invoiceFooterHtml(const QVariantMap& firm) {
    auto esc = [](const QVariant& v) { return v.toString().trimmed().toHtmlEscaped(); };
    const QString city = esc(firm.value("city")).split(",").first().trimmed();
    const QString jurisdiction = city.isEmpty() ? "Sirsa" : city;
    QString bank;
    if (!esc(firm.value("bank_ac")).isEmpty())
        bank += QString("<b>Account Number:</b> %1<br>").arg(esc(firm.value("bank_ac")));
    if (!esc(firm.value("bank_name")).isEmpty())
        bank += QString("<b>Bank:</b> %1<br>").arg(esc(firm.value("bank_name")));
    if (!esc(firm.value("bank_ifsc")).isEmpty())
        bank += QString("<b>IFSC:</b> %1<br>").arg(esc(firm.value("bank_ifsc")));
    if (!esc(firm.value("bank_branch")).isEmpty())
        bank += QString("<b>Branch:</b> %1<br>").arg(esc(firm.value("bank_branch")));
    if (bank.isEmpty()) bank = "&nbsp;";
    else bank.chop(4); // trailing <br>
    const QString firmName = esc(firm.value("firm_name"));
    return QString(
        "<table width=\"100%\" border=\"0\" cellspacing=\"1\" cellpadding=\"6\" bgcolor=\"#94A3B8\">"
        "<tr>"
        "<td width=\"40%\" valign=\"top\" bgcolor=\"#FFFFFF\"><b>Terms and Conditions</b><br>E. &amp; O.E.<br>"
        "1. Goods once sold will not be taken back.<br>"
        "2. Interest @ 18% p.a. will be charged if the payment is not made within the stipulated time.<br>"
        "3. Subject to '%1' Jurisdiction only.</td>"
        "<td width=\"30%\" valign=\"top\" bgcolor=\"#FFFFFF\">%2</td>"
        "<td width=\"30%\" valign=\"top\" align=\"right\" bgcolor=\"#FFFFFF\"><b>For %3</b><br><br><br><br><br><b>Signature</b></td>"
        "</tr></table>").arg(jurisdiction, bank, firmName);
}

/// Sales-invoice footer (Busy standard): Bank Details + T&C stacked on the
/// left, For-firm + Authorised Signatory on the right with the same spacing
/// as the balance-sheet signatory. No triple, no Generated stamp.
inline QString salesInvoiceFooterHtml(const QVariantMap& firm) {
    auto esc = [](const QVariant& v) { return v.toString().trimmed().toHtmlEscaped(); };
    const QString city = esc(firm.value("city")).split(",").first().trimmed();
    const QString jurisdiction = city.isEmpty() ? "Sirsa" : city;
    QString bank;
    if (!esc(firm.value("bank_ac")).isEmpty())
        bank += QString("<b>Account Number:</b> %1<br>").arg(esc(firm.value("bank_ac")));
    if (!esc(firm.value("bank_name")).isEmpty())
        bank += QString("<b>Bank:</b> %1<br>").arg(esc(firm.value("bank_name")));
    if (!esc(firm.value("bank_ifsc")).isEmpty())
        bank += QString("<b>IFSC:</b> %1<br>").arg(esc(firm.value("bank_ifsc")));
    if (!esc(firm.value("bank_branch")).isEmpty())
        bank += QString("<b>Branch:</b> %1<br>").arg(esc(firm.value("bank_branch")));
    if (bank.isEmpty()) bank = "&nbsp;";
    else bank.chop(4); // trailing <br>
    const QString firmName = esc(firm.value("firm_name"));
    return QString(
        "<table width=\"100%\" border=\"0\" cellspacing=\"1\" cellpadding=\"6\" bgcolor=\"#94A3B8\">"
        "<tr>"
        "<td width=\"55%\" valign=\"top\" bgcolor=\"#FFFFFF\"><b>Bank Details</b><br>%1<br>"
        "<b>Terms and Conditions</b><br>E. &amp; O.E.<br>"
        "1. Goods once sold will not be taken back.<br>"
        "2. Interest @ 18% p.a. will be charged if the payment is not made within the stipulated time.<br>"
        "3. Subject to '%2' Jurisdiction only.</td>"
        "<td width=\"45%\" valign=\"top\" align=\"right\" bgcolor=\"#FFFFFF\"><b>For %3</b><br><br><br><br>Authorised Signatory</td>"
        "</tr></table>").arg(bank, jurisdiction, firmName);
}

inline QString generatedFooterHtml() {
    return QString("<div class=\"genfoot\">Generated %1 &nbsp;|&nbsp; E. &amp; O.E.</div>")
        .arg(QDateTime::currentDateTime().toString("dd-MM-yyyy hh:mm AP").toHtmlEscaped());
}

/// Full print document wrapper.
inline QString wrapPrintDocument(const QString& body) {
    return QString("<!DOCTYPE html><html><head><meta charset=\"utf-8\">"
                   "<style>%1</style></head><body>%2</body></html>")
        .arg(printCss(), body);
}

} // namespace ErpPrint
