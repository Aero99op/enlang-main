"""Sovereign Multi-Threaded HTTP REST Server Daemon for enlngdb (ZERO EXTERNAL DEPENDENCIES).

Allows Cloud Web Apps (Next.js, Cloudflare Pages, Vercel, Python, Node.js, Go)
to query EnlngDB remotely over HTTP / JSON with zero configuration and full CORS support.

Endpoints:
- POST /api/query     : Executes conversational EnlngDB natural queries
- POST /api/batch     : Executes a batch of statements in an atomic transaction
- GET  /api/health    : Engine health check, memory stats, table count, uptime
- GET  /api/tables    : Schema overview of all tables in the active database
- GET  /api/table/:name: Preview rows and schema for a specific table
"""

import os
import sys
import json
import time
import argparse
from http.server import HTTPServer, ThreadingHTTPServer, BaseHTTPRequestHandler
from typing import Optional, Dict, Any, List
from pathlib import Path

from enlngdb.compiler import compile_enlngdb_source
from enlngdb.engine import NativeExecutionEngine
from enlngdb.storage import NativeStorageEngine, StorageError
from enlngdb.locking import DatabaseLock
from enlngdb.paging import estimate_memory_bytes


SERVER_START_TIME = time.time()


class EnlngDBRequestHandler(BaseHTTPRequestHandler):
    """Multi-threaded HTTP request handler for conversational EnlngDB queries."""

    server_version = "EnlngDB-Sovereign-HTTP/2.0"

    def _set_cors_headers(self):
        self.send_header("Access-Control-Allow-Origin", "*")
        self.send_header("Access-Control-Allow-Methods", "GET, POST, OPTIONS")
        self.send_header("Access-Control-Allow-Headers", "Content-Type, Authorization, X-Requested-With")

    def do_OPTIONS(self):
        """Handles CORS preflight requests."""
        self.send_response(204)
        self._set_cors_headers()
        self.end_headers()

    def _send_json(self, data: Dict[str, Any], status: int = 200):
        """Sends a JSON response with CORS headers."""
        response_bytes = json.dumps(data, default=str).encode("utf-8")
        self.send_response(status)
        self._set_cors_headers()
        self.send_header("Content-Type", "application/json; charset=utf-8")
        self.send_header("Content-Length", str(len(response_bytes)))
        self.end_headers()
        self.wfile.write(response_bytes)

    def do_GET(self):
        """Handles GET requests (health, status, tables, inspection)."""
        engine: NativeExecutionEngine = self.server.engine  # type: ignore

        if self.path in ("/api/health", "/api/status", "/health", "/status"):
            total_records = sum(len(t.rows) for t in engine.storage.tables.values())
            mem_bytes = sum(estimate_memory_bytes(t.to_dict()) for t in engine.storage.tables.values())
            uptime = round(time.time() - SERVER_START_TIME, 2)

            self._send_json({
                "status": "healthy",
                "engine": "EnlngDB Sovereign Engine v2.0",
                "active_database": engine.db_path or ":memory:",
                "table_count": len(engine.storage.tables),
                "total_records": total_records,
                "memory_bytes": mem_bytes,
                "uptime_seconds": uptime
            })
            return

        elif self.path in ("/api/tables", "/tables"):
            tables_info = []
            for name, tbl in engine.storage.tables.items():
                tables_info.append({
                    "name": name,
                    "columns": [c.name for c in tbl.columns],
                    "primary_key": tbl.primary_key,
                    "row_count": len(tbl.rows),
                    "hints": tbl.hints
                })
            self._send_json({
                "success": True,
                "database": engine.db_path or ":memory:",
                "tables": tables_info
            })
            return

        elif self.path.startswith("/api/table/"):
            tbl_name = self.path.replace("/api/table/", "").strip()
            if tbl_name in engine.storage.tables:
                tbl = engine.storage.tables[tbl_name]
                self._send_json({
                    "success": True,
                    "table": tbl_name,
                    "columns": [c.name for c in tbl.columns],
                    "primary_key": tbl.primary_key,
                    "row_count": len(tbl.rows),
                    "rows_preview": tbl.rows[:50]
                })
            else:
                self._send_json({"success": False, "error": f"Table '{tbl_name}' does not exist."}, status=404)
            return

        self._send_json({
            "error": "Not Found",
            "message": "EnlngDB Server is running. Use POST /api/query to execute database statements."
        }, status=404)

    def do_POST(self):
        """Handles POST requests (query execution and batch execution)."""
        engine: NativeExecutionEngine = self.server.engine  # type: ignore

        content_length = int(self.headers.get("Content-Length", 0))
        if content_length <= 0:
            self._send_json({"success": False, "error": "Empty request body."}, status=400)
            return

        try:
            raw_body = self.rfile.read(content_length).decode("utf-8")
            body = json.loads(raw_body)
        except Exception as e:
            self._send_json({"success": False, "error": f"Invalid JSON payload: {str(e)}"}, status=400)
            return

        if self.path in ("/api/query", "/query"):
            query = body.get("query", "").strip()
            if not query:
                self._send_json({"success": False, "error": "Field 'query' is required."}, status=400)
                return

            start_t = time.perf_counter()
            try:
                ast = compile_enlngdb_source(query)
                reports = engine.execute_program(ast)
                duration_ms = round((time.perf_counter() - start_t) * 1000, 2)

                # Persist to disk if file-backed
                if engine.db_path and engine.db_path != ":memory:":
                    engine.storage.save_to_disk(engine.db_path)

                # Consolidate response
                first_report = reports[0] if reports else {}
                self._send_json({
                    "success": True,
                    "database": engine.db_path or ":memory:",
                    "reports": reports,
                    "count": first_report.get("count", len(first_report.get("rows", []))),
                    "rows": first_report.get("rows", []),
                    "columns": first_report.get("columns", []),
                    "duration_ms": duration_ms
                })
            except Exception as e:
                duration_ms = round((time.perf_counter() - start_t) * 1000, 2)
                self._send_json({
                    "success": False,
                    "error": str(e),
                    "duration_ms": duration_ms
                }, status=400)
            return

        elif self.path in ("/api/batch", "/batch"):
            queries = body.get("queries", [])
            if not isinstance(queries, list) or not queries:
                self._send_json({"success": False, "error": "Field 'queries' must be a non-empty array of statements."}, status=400)
                return

            start_t = time.perf_counter()
            all_reports = []
            try:
                for q in queries:
                    ast = compile_enlngdb_source(q)
                    reps = engine.execute_program(ast)
                    all_reports.extend(reps)

                if engine.db_path and engine.db_path != ":memory:":
                    engine.storage.save_to_disk(engine.db_path)

                duration_ms = round((time.perf_counter() - start_t) * 1000, 2)
                self._send_json({
                    "success": True,
                    "executed_queries": len(queries),
                    "reports": all_reports,
                    "duration_ms": duration_ms
                })
            except Exception as e:
                duration_ms = round((time.perf_counter() - start_t) * 1000, 2)
                self._send_json({
                    "success": False,
                    "error": str(e),
                    "duration_ms": duration_ms
                }, status=400)
            return

        self._send_json({"error": "Endpoint not found."}, status=404)

    def log_message(self, format, *args):
        # Override to suppress default noisy console logs during tests unless debug
        if os.environ.get("ENLNGDB_DEBUG"):
            super().log_message(format, *args)


class EnlngDBHttpServer(ThreadingHTTPServer):
    """Threading HTTP server with embedded sovereign EnlngDB engine instance."""

    def __init__(self, server_address, RequestHandlerClass, db_path: Optional[str] = None):
        super().__init__(server_address, RequestHandlerClass)
        self.engine = NativeExecutionEngine(db_path=db_path, stream_output=False)


def serve_enlngdb(db_path: Optional[str] = None, host: str = "0.0.0.0", port: int = 8080) -> EnlngDBHttpServer:
    """Instantiates and starts an EnlngDB HTTP Server Daemon."""
    server = EnlngDBHttpServer((host, port), EnlngDBRequestHandler, db_path=db_path)
    return server


def main():
    parser = argparse.ArgumentParser(description="EnlngDB Sovereign Network HTTP Server Daemon")
    parser.add_argument("--db", default=None, help="Path to sovereign database file (.edb / .enlngdb)")
    parser.add_argument("--port", type=int, default=8080, help="Port to listen on (default: 8080)")
    parser.add_argument("--host", default="0.0.0.0", help="Host address to bind to (default: 0.0.0.0)")

    args = parser.parse_args()

    db_display = args.db or ":memory:"
    print(f"\n=======================================================")
    print(f"  ENLNGDB SOVEREIGN NETWORK SERVER DAEMON LIVE")
    print(f"  Host: http://{args.host}:{args.port}")
    print(f"  Active Database: {db_display}")
    print(f"  Zero SQL | Zero External Dependencies | Multi-Threaded")
    print(f"=======================================================\n")

    server = serve_enlngdb(db_path=args.db, host=args.host, port=args.port)
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        print("\nShutting down EnlngDB Server Daemon cleanly...")
        server.shutdown()
        server.server_close()
        print("EnlngDB Server stopped.")


if __name__ == "__main__":
    main()
