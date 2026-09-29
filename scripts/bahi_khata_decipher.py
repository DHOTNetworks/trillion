#!/usr/bin/env python3
"""
Bahi-Khata MDB Password Decipher & Company Info Extractor
=========================================================
Reads MS Access database files (.mdb / Data.***) produced by Bahi-Khata ERP,
extracts company information, and deciphers stored passwords using Bahi-Khata's
linear progressive shift cipher.

Cipher Specification:
  Encryption:  CipherChar[i] = PlainChar[i] + (i + 1) * 5
  Decryption:  PlainChar[i] = CipherChar[i] - (i + 1) * 5
"""

import sys
import os
import csv
import io
import json
import argparse
import subprocess
from typing import List, Dict, Any, Optional


def decipher_password(cipher_text: str) -> str:
    """Deciphers a Bahi-Khata password string."""
    if not cipher_text:
        return ""
    plain = []
    for i, ch in enumerate(cipher_text):
        shift = (i + 1) * 5
        plain.append(chr(ord(ch) - shift))
    return "".join(plain)


def encipher_password(plain_text: str) -> str:
    """Enciphers a plaintext password using Bahi-Khata cipher."""
    if not plain_text:
        return ""
    cipher = []
    for i, ch in enumerate(plain_text):
        shift = (i + 1) * 5
        cipher.append(chr(ord(ch) + shift))
    return "".join(cipher)


def get_mdb_tables(db_path: str) -> List[str]:
    """Lists all user tables in the MDB database."""
    try:
        out = subprocess.check_output(["mdb-tables", "-1", db_path], stderr=subprocess.DEVNULL)
        return [t.strip() for t in out.decode("utf-8", errors="ignore").splitlines() if t.strip()]
    except Exception:
        return []


def export_mdb_table(db_path: str, table_name: str) -> List[Dict[str, str]]:
    """Exports an MDB table as a list of dictionary rows."""
    try:
        out = subprocess.check_output(["mdb-export", db_path, table_name], stderr=subprocess.DEVNULL)
        text = out.decode("utf-8", errors="ignore")
        reader = csv.DictReader(io.StringIO(text))
        return list(reader)
    except Exception:
        return []


def inspect_file(db_path: str) -> Dict[str, Any]:
    """Inspects an MDB / Data file for company info and deciphers any passwords."""
    result: Dict[str, Any] = {
        "file_path": os.path.abspath(db_path),
        "file_name": os.path.basename(db_path),
        "file_size": os.path.getsize(db_path) if os.path.exists(db_path) else 0,
        "valid": False,
        "company_info": [],
        "passwords_found": []
    }

    if not os.path.exists(db_path):
        result["error"] = "File not found"
        return result

    tables = get_mdb_tables(db_path)
    if not tables:
        result["error"] = "Unable to read MDB catalog (ensure mdbtools is installed and file is a valid Access MDB)"
        return result

    result["valid"] = True
    result["table_count"] = len(tables)

    # 1. Inspect CompanyInfo table
    if "CompanyInfo" in tables:
        rows = export_mdb_table(db_path, "CompanyInfo")
        for row in rows:
            raw_pwd = row.get("Passwd", "") or row.get("Password", "") or row.get("Pass", "")
            decrypted = decipher_password(raw_pwd) if raw_pwd else "(No password set / Open)"

            entry = {
                "company_name": row.get("CompanyName", ""),
                "acc_year_from": row.get("AccYearFrom", ""),
                "acc_year_to": row.get("AccYearTo", ""),
                "books_beginning": row.get("BooksBeginingFrom", ""),
                "firm_type": row.get("FirmType", ""),
                "business": row.get("MyBUSINESS", "") or row.get("Business", ""),
                "station": row.get("MyStation", ""),
                "state": row.get("MySTATE", ""),
                "gstin": row.get("GSTIN", ""),
                "pan": row.get("PAN_No", ""),
                "address": row.get("Address", ""),
                "mobile": row.get("Mobile1", "") or row.get("Phone_F", "") or row.get("Phone_O", ""),
                "raw_password": raw_pwd,
                "deciphered_password": decrypted
            }
            result["company_info"].append(entry)

            if raw_pwd:
                result["passwords_found"].append({
                    "table": "CompanyInfo",
                    "column": "Passwd",
                    "raw_cipher": raw_pwd,
                    "plain_password": decrypted
                })

    # 2. Check any other potential password columns across all tables
    for t in tables:
        if t == "CompanyInfo":
            continue
        # Only inspect settings/auth tables if present
        if any(k in t.lower() for k in ["user", "login", "auth", "passwd", "password", "setting"]):
            rows = export_mdb_table(db_path, t)
            if not rows:
                continue
            for r in rows:
                for col_name, val in r.items():
                    if any(p in col_name.lower() for p in ["pass", "pwd"]) and val:
                        dec = decipher_password(val)
                        result["passwords_found"].append({
                            "table": t,
                            "column": col_name,
                            "raw_cipher": val,
                            "plain_password": dec
                        })

    return result


def find_data_files(start_path: str) -> List[str]:
    """Finds all candidate Bahi-Khata MDB / Data.* files."""
    if os.path.isfile(start_path):
        return [start_path]

    found = []
    for root, _, files in os.walk(start_path):
        for f in sorted(files):
            if f.endswith(".ldb") or f.endswith(".db"):
                continue
            if f.startswith("Data.") or f.endswith(".mdb") or f.endswith(".accdb") or f.startswith("data."):
                found.append(os.path.join(root, f))
    return found


def format_cli_output(info: Dict[str, Any]) -> str:
    """Formats the inspection result as a clean terminal report."""
    lines = []
    lines.append("=" * 78)
    lines.append(f"📁 FILE: {info['file_name']} ({info['file_path']})")
    lines.append(f"   Size: {info['file_size']:,} bytes | Tables: {info.get('table_count', 0)}")
    lines.append("-" * 78)

    if not info.get("valid"):
        lines.append(f"❌ Error: {info.get('error', 'Unknown error')}")
        lines.append("=" * 78)
        return "\n".join(lines)

    companies = info.get("company_info", [])
    if not companies:
        lines.append("ℹ️  No CompanyInfo records found.")
    else:
        for idx, c in enumerate(companies, 1):
            lines.append(f"🏢 COMPANY #{idx}: {c['company_name'] or '(Unnamed)'}")
            lines.append(f"   • Entity Type    : {c['firm_type'] or '-'}")
            lines.append(f"   • Accounting FY  : {c['acc_year_from'][:10] if c['acc_year_from'] else '-'} To {c['acc_year_to'][:10] if c['acc_year_to'] else '-'}")
            lines.append(f"   • Location       : {c['station'] or '-'}, {c['state'] or '-'}")
            lines.append(f"   • Business       : {c['business'] or '-'}")
            lines.append(f"   • GSTIN / PAN    : {c['gstin'] or '-'} / {c['pan'] or '-'}")
            lines.append(f"   • Mobile / Phone : {c['mobile'] or '-'}")
            lines.append("   " + "-" * 72)
            lines.append(f"   🔐 STORED CIPHER : {c['raw_password'] if c['raw_password'] else '(Empty - No Password)'}")
            lines.append(f"   🔑 DECIPHERED PWD: \033[1;32m{c['deciphered_password']}\033[0m")

    passwords = info.get("passwords_found", [])
    if len(passwords) > len(companies):
        lines.append("-" * 78)
        lines.append("🔎 OTHER PASSWORDS FOUND:")
        for p in passwords:
            if p["table"] != "CompanyInfo":
                lines.append(f"   • Table [{p['table']}] Column [{p['column']}]: '{p['raw_cipher']}' -> \033[1;32m{p['plain_password']}\033[0m")

    lines.append("=" * 78)
    return "\n".join(lines)


def main():
    parser = argparse.ArgumentParser(
        description="Bahi-Khata MDB Password Decipher & Data Extractor",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  python3 scripts/bahi_khata_decipher.py Bahi-Khata-Data/Data.001
  python3 scripts/bahi_khata_decipher.py Bahi-Khata-Data/
  python3 scripts/bahi_khata_decipher.py --decode ">;HERO"
  python3 scripts/bahi_khata_decipher.py --encode "919191"
  python3 scripts/bahi_khata_decipher.py Bahi-Khata-Data/Data.001 --json
"""
    )
    parser.add_argument("paths", nargs="*", default=["Bahi-Khata-Data"], help="Path(s) to Data.* or .mdb files or directories (default: Bahi-Khata-Data)")
    parser.add_argument("--json", action="store_true", help="Output results in JSON format")
    parser.add_argument("--decode", metavar="CIPHER", help="Directly decode a cipher string")
    parser.add_argument("--encode", metavar="PLAIN", help="Directly encode a plaintext password")

    args = parser.parse_args()

    if args.decode:
        dec = decipher_password(args.decode)
        if args.json:
            print(json.dumps({"cipher": args.decode, "plain": dec}, indent=2))
        else:
            print(f"Cipher:  {args.decode}")
            print(f"Decoded: {dec}")
        return

    if args.encode:
        enc = encipher_password(args.encode)
        if args.json:
            print(json.dumps({"plain": args.encode, "cipher": enc}, indent=2))
        else:
            print(f"Plain:   {args.encode}")
            print(f"Encoded: {enc}")
        return

    all_files = []
    for p in args.paths:
        all_files.extend(find_data_files(p))

    if not all_files:
        print(f"No Data.* or .mdb files found in: {', '.join(args.paths)}", file=sys.stderr)
        sys.exit(1)

    results = []
    for f in all_files:
        info = inspect_file(f)
        results.append(info)
        if not args.json:
            print(format_cli_output(info))

    if args.json:
        print(json.dumps(results, indent=2))


if __name__ == "__main__":
    main()
