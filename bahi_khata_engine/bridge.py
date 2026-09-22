"""
Bahi-Khata JetDB Engine - Core Python Bridge
Communicates with UCanAccess / Jackcess JDBC Bridge for 100% safe read/write operations
on Bahi-Khata MS Access / JetDB (.002, .018, .mdb) database files.
"""

import os
import sys
import json
import subprocess
from typing import List, Dict, Any, Optional, Tuple

class BahiKhataBridgeClient:
    def __init__(self, db_path: str, java_bin: Optional[str] = None):
        self.db_path = os.path.abspath(db_path)
        if not os.path.exists(self.db_path):
            raise FileNotFoundError(f"Bahi-Khata database file not found: {self.db_path}")

        # Resolve Java binary
        self.java_bin = java_bin or self._find_java()
        if not self.java_bin:
            raise RuntimeError("Java runtime (OpenJDK) not found. Please install openjdk.")

        # Resolve classpath
        base_dir = os.path.dirname(os.path.abspath(__file__))
        project_root = os.path.abspath(os.path.join(base_dir, ".."))
        ucanaccess_dir = os.path.join(project_root, "tools", "ucanaccess", "*")
        bridge_classes = os.path.join(base_dir, "bridge")
        
        self.classpath = f"{ucanaccess_dir}:{bridge_classes}"
        self._ensure_bridge_compiled(bridge_classes, ucanaccess_dir)

    def _find_java(self) -> Optional[str]:
        candidates = [
            "/opt/homebrew/opt/openjdk/bin/java",
            "/usr/local/opt/openjdk/bin/java",
            "/opt/homebrew/bin/java",
            "/usr/bin/java",
        ]
        for c in candidates:
            if os.path.exists(c):
                return c
        try:
            out = subprocess.check_output(["which", "java"], text=True).strip()
            if out:
                return out
        except Exception:
            pass
        return None

    def _ensure_bridge_compiled(self, bridge_dir: str, ucanaccess_dir: str):
        class_file = os.path.join(bridge_dir, "BahiKhataBridge.class")
        java_file = os.path.join(bridge_dir, "BahiKhataBridge.java")
        if not os.path.exists(class_file) or os.path.getmtime(java_file) > os.path.getmtime(class_file):
            javac_bin = self.java_bin.replace("java", "javac") if "bin/java" in self.java_bin else "javac"
            cmd = [javac_bin, "-cp", ucanaccess_dir, java_file]
            res = subprocess.run(cmd, capture_output=True, text=True)
            if res.returncode != 0:
                raise RuntimeError(f"Failed to compile Java bridge: {res.stderr}")

    def query(self, sql: str, limit: int = -1) -> List[Dict[str, Any]]:
        """Executes a SELECT query and returns a list of dictionaries."""
        cmd = [self.java_bin, "-cp", self.classpath, "BahiKhataBridge", "query", self.db_path, sql]
        if limit > 0:
            cmd.append(str(limit))
            
        res = subprocess.run(cmd, capture_output=True, text=True)
        # Filter output for JSON line
        for line in res.stdout.splitlines():
            line = line.strip()
            if line.startswith('{"status":"OK"') or line.startswith('{"status":"ERROR"'):
                data = json.loads(line)
                if data.get("status") == "ERROR":
                    raise RuntimeError(f"JetDB Query Error: {data.get('error')}\nSQL: {sql}")
                cols = data.get("columns", [])
                rows = data.get("rows", [])
                return [dict(zip(cols, row)) for row in rows]
                
        if res.returncode != 0:
            raise RuntimeError(f"Bridge invocation failed: {res.stderr}\nStdout: {res.stdout}")
        return []

    def execute(self, sql: str) -> int:
        """Executes an INSERT / UPDATE / DELETE statement in an ACID transaction."""
        cmd = [self.java_bin, "-cp", self.classpath, "BahiKhataBridge", "execute", self.db_path, sql]
        res = subprocess.run(cmd, capture_output=True, text=True)
        for line in res.stdout.splitlines():
            line = line.strip()
            if line.startswith('{"status":"OK"') or line.startswith('{"status":"ERROR"'):
                data = json.loads(line)
                if data.get("status") == "ERROR":
                    raise RuntimeError(f"JetDB Execution Error: {data.get('error')}\nSQL: {sql}")
                return data.get("affectedRows", 0)
                
        if res.returncode != 0:
            raise RuntimeError(f"Bridge invocation failed: {res.stderr}\nStdout: {res.stdout}")
        return 0

    def get_tables(self) -> List[str]:
        """Lists all non-system tables in the Bahi Khata database."""
        cmd = [self.java_bin, "-cp", self.classpath, "BahiKhataBridge", "tables", self.db_path]
        res = subprocess.run(cmd, capture_output=True, text=True)
        for line in res.stdout.splitlines():
            line = line.strip()
            if line.startswith('{"status":"OK"'):
                data = json.loads(line)
                return data.get("tables", [])
        return []

    def get_schema(self, table: Optional[str] = None) -> List[Dict[str, Any]]:
        """Returns column names and types for a table or all tables."""
        cmd = [self.java_bin, "-cp", self.classpath, "BahiKhataBridge", "schema", self.db_path]
        if table:
            cmd.append(table)
        res = subprocess.run(cmd, capture_output=True, text=True)
        for line in res.stdout.splitlines():
            line = line.strip()
            if line.startswith('{"status":"OK"'):
                data = json.loads(line)
                return data.get("columns", [])
        return []

    def compact_and_repair(self) -> bool:
        """Fully compacts the database and rebuilds all B-tree indexes from scratch."""
        cmd = [self.java_bin, "-cp", self.classpath, "BahiKhataBridge", "compact", self.db_path]
        res = subprocess.run(cmd, capture_output=True, text=True)
        for line in res.stdout.splitlines():
            line = line.strip()
            if line.startswith('{"status":"OK"'):
                return True
            elif line.startswith('{"status":"ERROR"'):
                data = json.loads(line)
                raise RuntimeError(f"Database Compact Error: {data.get('error')}")
        if res.returncode != 0:
            raise RuntimeError(f"Bridge invocation failed: {res.stderr}\nStdout: {res.stdout}")
        return True


