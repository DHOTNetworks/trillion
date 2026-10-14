#!/usr/bin/env python3
"""Carve print_export_controller.cpp into per-report modules.

Moves every render*Html body verbatim into src/printing/reports/<base>_report.cpp
(namespace PrintReports, scope prefix stripped), creates <base>_excel.cpp with the
per-target Excel variant, and rewrites the controller methods as thin delegates.
Public API in print_export_controller.h is UNCHANGED (tests stay green).

Run from repo root:  python3 tools/carve_print_reports.py [--check]
"""
import re, sys

CHECK = "--check" in sys.argv
SRC = "src/services/print_export_controller.cpp"
OUT = "src/printing/reports"

GROUPS = [  # (match-key, file base) — longest keys first
    ("sales_invoice", "sales_invoice"), ("purchase_invoice", "purchase_invoice"),
    ("cash_voucher", "cash_voucher"), ("cheque_voucher", "cheque_voucher"),
    ("journal_voucher", "journal_voucher"), ("jform_voucher", "jform_voucher"),
    ("iform_voucher", "iform_voucher"), ("tds_voucher", "tds_voucher"),
    ("debit_credit_note", "debit_credit_note"), ("transport_dispatch", "transport_dispatch"),
    ("ledger_statement", "ledger_statement"), ("ledger_csv", "ledger_statement"),
    ("day_book", "day_book"), ("trial_balance", "trial_balance"),
    ("profit_loss", "profit_loss"), ("balance_sheet", "balance_sheet"),
    ("capital_accounts", "capital_accounts"), ("cash_bank_flow", "cash_bank_flow"),
    ("interestcalculation", "interest_statement"),
    ("interest_statement", "interest_statement"), ("depreciation_chart", "depreciation_chart"),
    ("sales_register", "sales_register"), ("purchase_register", "purchase_register"),
    ("stock_register", "stock_register"), ("stock_csv", "stock_register"),
    ("item_movement", "item_movement"), ("closing_stock", "closing_stock"),
    ("milling_statement", "milling_statement"), ("mandi_report", "mandi_report"),
    ("gstr_report", "gstr_report"),
]

PLUMBING = {"open_file_in_os", "get_default_reports_dir", "printHtml",
            "exportHtmlToPdf", "exportHtmlToOdf", "exportHtmlToExcel",
            "exportTableToCsv"}
SHARED = ["getFirmProfile", "getPartyProfile", "numberToWords", "amountInWords"]
CSV_BUILDERS = {
    "export_ledger_csv": "buildLedgerCsvDocument",
    "export_sales_register_csv": "buildSalesRegisterCsvDocument",
    "export_purchase_register_csv": "buildPurchaseRegisterCsvDocument",
    "export_stock_csv": "buildStockCsvDocument",
}

# Defaults that lived on the controller header decls (definitions never repeat
# them) — re-applied to the module headers so every old call site compiles.
# Map: file base -> [(bare tail, full tail)] (each appears on render + excel decls).
DEFAULT_TAILS = {
    "cash_voucher": [("(const QString& voucherNo, const QString& voucherType);",
                      "(const QString& voucherNo, const QString& voucherType = \"Cash Payment\");")],
    "cheque_voucher": [("(const QString& voucherNo, const QString& voucherType);",
                        "(const QString& voucherNo, const QString& voucherType = \"Cheque Payment\");")],
    "sales_invoice": [("(const QString& invoiceNo, const QString& copyType);",
                       "(const QString& invoiceNo, const QString& copyType = \"ORIGINAL FOR RECIPIENT\");")],
    "trial_balance": [("(const QString& asOnDate, int mode);",
                       "(const QString& asOnDate, int mode = 0);")],
}

MODULE_CPP_PREAMBLE = """#include "%(base)s_report.h"
#include "report_format.h"
#include "report_data.h"
#include "../../database_manager.h"
#include "../../engine/accounting_engine.h"
#include "../../engine/fiscal_year_helper.h"
#include "../../engine/profit_loss_calculator.h"
#include "../../engine/balance_sheet_calculator.h"
#include "../../engine/stock_valuation_engine.h"
#include "../../models/account_classifier.h"
#include <QString>
#include <QVariant>
#include <QVariantMap>
#include <QVariantList>
#include <QDate>
#include <QDateTime>
#include <QHash>
#include <QRegularExpression>
#include <QDebug>
#include <cmath>
#include <algorithm>

namespace PrintReports {
"""


def group_of(fn):
    flat = fn.lower().replace("_", "")
    for key, base in GROUPS:
        if key.replace("_", "") in flat:
            return base
    return None


def params_to_args(params):
    inner = params.strip()[1:-1].strip()
    if not inner:
        return ""
    return ", ".join(p.strip().split("=")[0].strip().split()[-1].lstrip("*&")
                     for p in inner.split(","))


def main():
    s = open(SRC, encoding="utf-8").read()
    fnpat = re.compile(r"^(QString|bool|void|QVariantMap) PrintExportController::(\w+)(\(.*?\)(?: const)?) \{", re.M)
    matches = list(fnpat.finditer(s))
    assert len(matches) >= 140, len(matches)
    prelude = s[:matches[0].start()]
    assert "PrintExportController::PrintExportController" in prelude
    if "static QString formatINR" not in prelude:
        print("Already carved (formatINR lives in report_format.h) — nothing to do.")
        return

    blocks, order = {}, []
    for i, m in enumerate(matches):
        end = matches[i + 1].start() if i + 1 < len(matches) else len(s)
        blocks[m.group(2)] = (m.group(1), m.group(3), s[m.start():end])
        order.append(m.group(2))

    render, wrappers = {}, []
    for n in order:
        if n in PLUMBING or n in SHARED:
            continue
        if n.startswith("render"):
            render[n] = blocks[n]
        else:
            wrappers.append(n)
    covered = set(render) | set(wrappers) | set(PLUMBING) | set(SHARED)
    assert covered == set(order), set(order) - covered
    print(f"render={len(render)} wrappers={len(wrappers)}")

    bygroup = {}
    for n in render:
        g = group_of(n)
        assert g, n
        bygroup.setdefault(g, []).append(n)
    assert len(bygroup) == 27, sorted(bygroup)
    for n in wrappers:
        if n in CSV_BUILDERS:
            continue
        _, _, body = blocks[n]
        assert re.search(r"render\w+Html\(|export_\w+\(|print_\w+\(", body), n
    print("classification OK")

    if CHECK:
        print("CHECK OK — no files written")
        return

    # ---- per-report modules ----
    for base in sorted(bygroup):
        names = bygroup[base]
        h = [f"#pragma once\n",
             f"// {base} — print/PDF/ODF renderer + per-target Excel + CSV builders.\n",
             "// One report, one contract: edit layout here without touching\n",
             "// any other report or the controller plumbing.\n",
             "#include <QString>\n", "#include <QStringList>\n",
             "#include <QVector>\n", "#include <QVariant>\n",
             "#include <QVariantMap>\n",
             "\nnamespace PrintReports {\n"]
        for n in names:
            rtype, params, _ = blocks[n]
            h.append(f"{rtype} {n}{params};\n")
            if n.startswith("render"):
                h.append(f"{rtype} {n.replace('Html', 'ExcelHtml', 1)}{params};\n")
        # re-apply declaration defaults (definitions never carry them)
        h = "".join(h)
        for tail, full in DEFAULT_TAILS.get(base, []):
            assert h.count(tail) == 2, (base, tail, h.count(tail))
            h = h.replace(tail, full)
        hdr = h
        for n in wrappers:
            if n in CSV_BUILDERS and group_of(n) == base:
                _, params, _ = blocks[n]
                inner = params.strip()[1:-1].strip()
                parts = [p.strip() for p in inner.split(",")]
                assert parts[-1].split()[-1] == "customPath", (n, parts)
                mod_params = ", ".join(parts[:-1])
                outs = "QStringList* headersOut, QVector<QStringList>* rowsOut, QString* fileNameOut"
                h.append(f"void {CSV_BUILDERS[n]}({mod_params}, {outs});\n")
        h.append("} // namespace PrintReports\n")

        c = [MODULE_CPP_PREAMBLE % {"base": base}]
        for n in names:
            rtype, params, body = blocks[n]
            head, rest = body.split("{", 1)
            assert "PrintExportController::" in head
            moved = head.replace("PrintExportController::", "") + "{" + rest
            # cross-group call: closing stock reuses the stock register table
            moved = moved.replace("renderStockRegisterHtml(",
                                  "StockRegister::renderStockRegisterHtml(") \
                if base == "closing_stock" else moved
            c.append("\n" + moved)
        for n in wrappers:
            if n in CSV_BUILDERS and group_of(n) == base:
                c.append("\n" + csv_builder(n, *blocks[n]))
        c.append("\n} // namespace PrintReports\n")

        x = [f'#include "{base}_report.h"\n', "\nnamespace PrintReports {\n"]
        for n in names:
            if not n.startswith("render"):
                continue
            rtype, params, _ = blocks[n]
            xn = n.replace("Html", "ExcelHtml", 1)
            x.append(
                f"\n{rtype} {xn}{params} {{\n"
                f"    // Excel-dedicated variant: same figures as print/PDF today.\n"
                f"    // Customize here per target (raw numbers + num/date classes)\n"
                f"    // without touching the print layout in {base}_report.cpp.\n"
                f"    return {n}({params_to_args(params)});\n}}\n")
        x.append("\n} // namespace PrintReports\n")

        open(f"{OUT}/{base}_report.h", "w", encoding="utf-8").write("".join(h))
        open(f"{OUT}/{base}_report.cpp", "w", encoding="utf-8").write("".join(c))
        open(f"{OUT}/{base}_excel.cpp", "w", encoding="utf-8").write("".join(x))
    print("modules written:", len(bygroup))

    # ---- rewritten controller.cpp ----
    new = [prelude_kept(prelude)]
    for n in order:
        if n in PLUMBING:
            new.append(blocks[n][2])
        elif n in SHARED:
            continue  # moved to report_data.h
        elif n.startswith("render"):
            rtype, params, _ = blocks[n]
            new.append(
                f"{rtype} PrintExportController::{n}{params} {{\n"
                f"    return PrintReports::{n}({params_to_args(params)});\n}}\n")
        elif n in CSV_BUILDERS:
            new.append(csv_delegate(n, *blocks[n]))
        else:
            new.append(wrapper_delegate(n, *blocks[n]))
    open(SRC, "w", encoding="utf-8").write("".join(new))
    print("controller rewritten")


def prelude_kept(prelude):
    m = re.search(r"static QString formatINR\(double val\) \{.*?\n\}\n+", prelude, re.S)
    assert m, "formatINR block not found"
    kept = prelude.replace(m.group(0), "")
    assert "formatINR" not in kept
    kept += "\n// Per-report renderers (one module per report — see src/printing/reports/).\n"
    for base in ["sales_invoice", "purchase_invoice", "cash_voucher", "cheque_voucher",
                 "journal_voucher", "jform_voucher", "iform_voucher", "tds_voucher",
                 "debit_credit_note", "transport_dispatch", "ledger_statement", "day_book",
                 "trial_balance", "profit_loss", "balance_sheet", "capital_accounts",
                 "cash_bank_flow", "interest_statement", "depreciation_chart",
                 "sales_register", "purchase_register", "stock_register", "item_movement",
                 "closing_stock", "milling_statement", "mandi_report", "gstr_report"]:
        kept += f'#include "../printing/reports/{base}_report.h"\n'
    return kept


def wrapper_delegate(n, rtype, params, body):
    head, rest = body.split("{", 1)
    assert "PrintExportController::" in head  # keep method scope; only re-point the render call
    if not re.search(r"\brender\w+Html\(", rest):
        return head + "{" + rest  # sibling composition (duplicate/all-copies) — keep verbatim
    rest2 = re.sub(r"\brender(\w+Html)\(", r"PrintReports::render\1(", rest)
    if n.endswith("_excel"):
        # excel target uses the per-target variant
        rest2 = re.sub(r"PrintReports::(render\w+)Html\(", r"PrintReports::\1ExcelHtml(", rest2)
    assert rest2 != rest, n
    # excel wrappers must not leak print-html calls
    if n.endswith("_excel"):
        assert "PrintReports::render" in rest2 and "Html(" not in rest2.replace("ExcelHtml(", ""), n
    return head + "{" + rest2


def csv_delegate(n, rtype, params, body):
    args = params_to_args(params)
    assert args.endswith("customPath"), (n, args)
    barg = args.rsplit(",", 1)[0]
    head, _ = body.split("{", 1)
    assert "PrintExportController::" in head  # keep method scope
    return (head + "{\n"
            "    QStringList headers;\n"
            "    QVector<QStringList> rows;\n"
            "    QString defName;\n"
            f"    PrintReports::{CSV_BUILDERS[n]}({barg}, &headers, &rows, &defName);\n"
            "    return exportTableToCsv(headers, rows, defName, customPath);\n}\n")


def csv_builder(n, rtype, params, body):
    inner = params.strip()[1:-1].strip()
    parts = [p.strip() for p in inner.split(",")]
    assert parts[-1].split()[-1] == "customPath", (n, parts)
    mod_params = ", ".join(parts[:-1])
    outs = "QStringList* headersOut, QVector<QStringList>* rowsOut, QString* fileNameOut"
    stmts = body[body.find("{") + 1:body.rfind("}")]
    m = re.search(r"return exportTableToCsv\(([^;]+)\);", stmts)
    assert m, n
    callargs = [a.strip() for a in m.group(1).split(",")]
    assert len(callargs) == 4 and callargs[3] == "customPath", (n, callargs)
    hvar, rvar, fname = callargs[0], callargs[1], callargs[2]
    tail = (f"\n    *headersOut = {hvar};\n    *rowsOut = {rvar};\n"
            f"    *fileNameOut = {fname};\n    return;\n")
    stmts2 = stmts[:m.start()] + tail
    return (f"void {CSV_BUILDERS[n]}({mod_params}, {outs}) {{\n" +
            stmts2 + "}\n")


if __name__ == "__main__":
    main()
