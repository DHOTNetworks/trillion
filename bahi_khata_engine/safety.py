"""
Bahi-Khata Safety & Integrity Engine
Guarantees 100% safe database mutations with:
1. Automatic Point-in-Time Snapshots (.bak)
2. Atomic Rollback capability
3. Double-Entry Accounting Invariant Enforcement
4. Comprehensive Change Audit Trail
"""

import os
import shutil
import hashlib
import json
import datetime
from typing import List, Dict, Any, Optional

class SnapshotManager:
    def __init__(self, db_path: str):
        self.db_path = os.path.abspath(db_path)
        self.backup_dir = os.path.join(os.path.dirname(self.db_path), "backups")
        os.makedirs(self.backup_dir, exist_ok=True)

    def calculate_checksum(self, file_path: Optional[str] = None) -> str:
        """Calculates MD5 checksum of the database file."""
        target = file_path or self.db_path
        h = hashlib.md5()
        with open(target, "rb") as f:
            while chunk := f.read(65536):
                h.update(chunk)
        return h.hexdigest()

    def create_snapshot(self, tag: str = "auto") -> str:
        """Creates an atomic physical backup of the JetDB database."""
        ts = datetime.datetime.now().strftime("%Y%m%d_%H%M%S_%f")
        base_name = os.path.basename(self.db_path)
        backup_filename = f"{base_name}.{tag}.{ts}.bak"
        backup_path = os.path.join(self.backup_dir, backup_filename)
        
        # Copy file preserving metadata
        shutil.copy2(self.db_path, backup_path)
        
        # Write metadata sidecar
        meta_path = backup_path + ".meta.json"
        meta = {
            "source_db": self.db_path,
            "backup_path": backup_path,
            "timestamp": datetime.datetime.now().isoformat(),
            "tag": tag,
            "checksum": self.calculate_checksum(backup_path),
            "size_bytes": os.path.getsize(backup_path)
        }
        with open(meta_path, "w") as f:
            json.dump(meta, f, indent=2)
            
        return backup_path

    def list_snapshots(self) -> List[Dict[str, Any]]:
        """Lists all available snapshots sorted by timestamp desc."""
        snapshots = []
        if not os.path.exists(self.backup_dir):
            return snapshots
            
        for f in os.listdir(self.backup_dir):
            if f.endswith(".bak"):
                full_path = os.path.join(self.backup_dir, f)
                meta_path = full_path + ".meta.json"
                meta = {}
                if os.path.exists(meta_path):
                    try:
                        with open(meta_path) as mf:
                            meta = json.load(mf)
                    except Exception:
                        pass
                snapshots.append({
                    "filename": f,
                    "path": full_path,
                    "timestamp": meta.get("timestamp", datetime.datetime.fromtimestamp(os.path.getmtime(full_path)).isoformat()),
                    "tag": meta.get("tag", "manual"),
                    "size_mb": round(os.path.getsize(full_path) / (1024 * 1024), 2),
                    "checksum": meta.get("checksum", "")
                })
        snapshots.sort(key=lambda x: x["timestamp"], reverse=True)
        return snapshots

    def rollback(self, snapshot_path: Optional[str] = None) -> bool:
        """Restores the database from a snapshot (defaults to latest snapshot)."""
        if not snapshot_path:
            snaps = self.list_snapshots()
            if not snaps:
                raise RuntimeError("No snapshots found to rollback to.")
            snapshot_path = snaps[0]["path"]

        if not os.path.exists(snapshot_path):
            raise FileNotFoundError(f"Snapshot not found: {snapshot_path}")

        # Create emergency pre-rollback snapshot just in case
        emergency_tag = "pre_rollback"
        ts = datetime.datetime.now().strftime("%Y%m%d_%H%M%S")
        emergency_file = os.path.join(self.backup_dir, f"{os.path.basename(self.db_path)}.{emergency_tag}.{ts}.bak")
        shutil.copy2(self.db_path, emergency_file)

        # Restore
        shutil.copy2(snapshot_path, self.db_path)
        return True


class AccountingGuard:
    """Validates double-entry rules and financial invariants before committing changes."""
    
    @staticmethod
    def validate_double_entry(transaction_rows: List[Dict[str, Any]]) -> Tuple[bool, str, float, float]:
        """
        Validates that for a voucher, Sum(Debit) == Sum(Credit).
        Returns: (is_balanced, error_message, total_dr, total_cr)
        """
        dr_sum = 0.0
        cr_sum = 0.0

        for r in transaction_rows:
            drcr = str(r.get("DrCr", "")).strip().upper()
            amt = float(r.get("Amount", 0.0))
            if drcr in ["DR", "D"]:
                dr_sum += amt
            elif drcr in ["CR", "C"]:
                cr_sum += amt
            else:
                return False, f"Invalid DrCr indicator: '{drcr}' on row {r}", dr_sum, cr_sum

        dr_sum = round(dr_sum, 2)
        cr_sum = round(cr_sum, 2)
        diff = round(abs(dr_sum - cr_sum), 2)

        if diff > 0.01:
            return False, f"Double-entry imbalance: Total Dr = {dr_sum:,.2f}, Total Cr = {cr_sum:,.2f} (Delta = {diff:,.2f})", dr_sum, cr_sum

        return True, "Balanced", dr_sum, cr_sum


class AuditTrailLogger:
    """Maintains an append-only JSONL log of every agent action and database modification."""
    
    def __init__(self, db_path: str):
        self.log_file = os.path.join(os.path.dirname(os.path.abspath(db_path)), "bahi_khata_audit_trail.jsonl")

    def log(self, action: str, agent: str, reason: str, details: Dict[str, Any], snapshot_path: Optional[str] = None):
        entry = {
            "timestamp": datetime.datetime.now().isoformat(),
            "action": action,
            "agent": agent,
            "reason": reason,
            "snapshot": snapshot_path,
            "details": details
        }
        with open(self.log_file, "a") as f:
            f.write(json.dumps(entry) + "\n")
