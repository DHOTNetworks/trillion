"""
Bahi-Khata Safe Data Correction Engine
Applies validated, audited, and point-in-time snapshot protected corrections directly on JetDB.
"""

from typing import List, Dict, Any, Optional
from .bridge import BahiKhataBridgeClient
from .safety import SnapshotManager, AuditTrailLogger, AccountingGuard

class BahiKhataCorrector:
    def __init__(self, bridge: BahiKhataBridgeClient):
        self.bridge = bridge
        self.snapshot_mgr = SnapshotManager(bridge.db_path)
        self.audit_logger = AuditTrailLogger(bridge.db_path)

    def execute_safe_mutation(self, sql_statements: List[str], reason: str, agent_name: str = "AIAgent") -> Dict[str, Any]:
        """
        Executes a batch of mutation SQLs with automatic pre-write snapshot and rollback on failure.
        """
        # 1. Take Snapshot
        snapshot_path = self.snapshot_mgr.create_snapshot(tag="pre_correction")
        
        try:
            total_affected = 0
            for sql in sql_statements:
                affected = self.bridge.execute(sql)
                total_affected += affected

            # Log to Audit Trail
            self.audit_logger.log(
                action="MUTATION_BATCH",
                agent=agent_name,
                reason=reason,
                details={"statements": sql_statements, "affected_rows": total_affected},
                snapshot_path=snapshot_path
            )

            return {
                "status": "SUCCESS",
                "affected_rows": total_affected,
                "snapshot": snapshot_path,
                "reason": reason
            }
        except Exception as e:
            # Atomic rollback to snapshot
            self.snapshot_mgr.rollback(snapshot_path)
            self.audit_logger.log(
                action="MUTATION_FAILED_ROLLED_BACK",
                agent=agent_name,
                reason=f"Failed: {str(e)}",
                details={"statements": sql_statements, "error": str(e)},
                snapshot_path=snapshot_path
            )
            raise RuntimeError(f"Correction failed and was safely rolled back to {snapshot_path}. Error: {e}")

    def update_party_gstin(self, party_code: int, gstin: str, reason: str = "Update GSTIN", agent: str = "AIAgent") -> Dict[str, Any]:
        """Safely updates a party's GSTIN in the Ledgers table."""
        gstin = gstin.strip().upper()
        sql = f"UPDATE Ledgers SET RegNo = '{gstin}' WHERE Code1st = {party_code}"
        return self.execute_safe_mutation([sql], reason=f"{reason} for Party {party_code}", agent_name=agent)

    def update_transport_details(self, voucher_no: int, trans_type: str, vehicle_no: str, gr_no: str, agent: str = "AIAgent") -> Dict[str, Any]:
        """Safely updates dispatch logistics in SaleTransportationDetail."""
        sql = f"""
        UPDATE SaleTransportationDetail 
        SET VehicleNo = '{vehicle_no.strip().upper()}', GRNo = '{gr_no.strip()}'
        WHERE VoucherNumber = {voucher_no} AND TransType = '{trans_type.strip()}'
        """
        return self.execute_safe_mutation([sql], reason=f"Update Transport details for Vch #{voucher_no}", agent_name=agent)
