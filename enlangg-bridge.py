#!/usr/bin/env python3
"""
Enlangg Sovereign Native Daemon & Studio Bridge Server
Connects Enlangg Studio (Desktop & Web) directly to installed ~/.enlangg/bin compilers.
Zero third-party dependencies - standard library exclusively.
"""

import http.server
import json
import os
import platform
import shutil
import subprocess
import sys
import tempfile
import time

if hasattr(sys.stdout, 'reconfigure'):
    sys.stdout.reconfigure(encoding='utf-8', errors='replace')
if hasattr(sys.stderr, 'reconfigure'):
    sys.stderr.reconfigure(encoding='utf-8', errors='replace')

DEFAULT_PORT = 5999
BASE_DIR = os.path.dirname(os.path.abspath(__file__))
WEBSITE_DIR = os.path.join(BASE_DIR, "website")
HOME_DIR = os.path.expanduser("~")
DEFAULT_ENLANG_BIN = os.path.join(HOME_DIR, ".enlangg", "bin")

def find_binary(name):
    target_ext = ".exe" if platform.system() == "Windows" else ""
    local_bin = os.path.join(DEFAULT_ENLANG_BIN, f"{name}{target_ext}")
    repo_bin = os.path.join(BASE_DIR, f"{name}{target_ext}")

    # Prioritize whichever binary is newer between repo build and installed build
    if os.path.exists(repo_bin) and os.path.exists(local_bin):
        try:
            if os.path.getmtime(repo_bin) >= os.path.getmtime(local_bin):
                return repo_bin
            return local_bin
        except Exception:
            return repo_bin
    if os.path.exists(repo_bin):
        return repo_bin
    if os.path.exists(local_bin):
        return local_bin

    # 3. Check system PATH
    which_bin = shutil.which(name)
    if which_bin:
        return which_bin

    return None

def detect_installed_binaries():
    binaries = ["enlangg", "enlng", "enlngdb", "enlngf", "enlngd", "enlngs", "enlngm"]
    detected = {}
    for b in binaries:
        path = find_binary(b)
        detected[b] = {
            "found": path is not None,
            "path": path
        }
    return detected

class EnlangBridgeHandler(http.server.SimpleHTTPRequestHandler):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, directory=WEBSITE_DIR if os.path.exists(WEBSITE_DIR) else BASE_DIR, **kwargs)

    def end_headers(self):
        # Enable Full CORS for Web IDE connectivity
        self.send_header("Access-Control-Allow-Origin", "*")
        self.send_header("Access-Control-Allow-Methods", "GET, POST, OPTIONS")
        self.send_header("Access-Control-Allow-Headers", "Content-Type, X-Enlang-Source")
        self.send_header("Cache-Control", "no-cache, no-store, must-revalidate")
        super().end_headers()

    def do_OPTIONS(self):
        self.send_response(200)
        self.end_headers()

    def do_GET(self):
        if self.path in ("/health", "/api/health", "/status"):
            binaries = detect_installed_binaries()
            resp = {
                "status": "online",
                "version": "2.0.0-native-bridge",
                "system": platform.system(),
                "defaultBinDir": DEFAULT_ENLANG_BIN,
                "binaries": binaries,
                "activeInstalled": any(v["found"] for v in binaries.values()),
                "timestamp": int(time.time() * 1000)
            }
            body = json.dumps(resp, indent=2).encode("utf-8")
            self.send_response(200)
            self.send_header("Content-Type", "application/json")
            self.send_header("Content-Length", str(len(body)))
            self.end_headers()
            self.wfile.write(body)
            return

        # Fallback to serving Studio web assets locally
        return super().do_GET()

    def do_POST(self):
        if self.path in ("/api/run", "/run"):
            content_length = int(self.headers.get("Content-Length", 0))
            raw_body = self.rfile.read(content_length).decode("utf-8")
            
            try:
                data = json.loads(raw_body)
            except Exception as e:
                self.send_error_json(400, f"Invalid JSON payload: {str(e)}")
                return

            filename = data.get("filename", "scratch.enlng")
            content = data.get("content", "")
            target_lang = data.get("lang", "")

            # Execute code using the real installed binaries
            result = self.execute_code(filename, content, target_lang)
            
            body = json.dumps(result).encode("utf-8")
            self.send_response(200)
            self.send_header("Content-Type", "application/json")
            self.send_header("Content-Length", str(len(body)))
            self.end_headers()
            self.wfile.write(body)
            return

        self.send_error_json(404, "Endpoint not found")

    def send_error_json(self, status_code, message):
        self.send_response(status_code)
        self.send_header("Content-Type", "application/json")
        body = json.dumps({"success": False, "error": message}).encode("utf-8")
        self.send_header("Content-Length", str(len(body)))
        self.end_headers()
        self.wfile.write(body)

    def execute_code(self, filename, content, target_lang):
        t0 = time.perf_counter()
        ext = os.path.splitext(filename)[1].lower()
        if not ext and target_lang:
            ext = f".{target_lang.lstrip('.')}"

        # Create temporary execution file
        temp_dir = tempfile.mkdtemp(prefix="enlang_bridge_")
        temp_file = os.path.join(temp_dir, os.path.basename(filename) or f"run{ext}")
        
        try:
            with open(temp_file, "w", encoding="utf-8") as f:
                f.write(content)

            # Determine appropriate installed binary
            cmd = None
            executor_name = "enlangg"

            if ext in (".enlngdb", ".enlgdb") or "type enlngdb" in content.lower():
                enlngdb_bin = find_binary("enlngdb")
                enlangg_bin = find_binary("enlangg")
                if enlngdb_bin:
                    cmd = [enlngdb_bin, temp_file]
                    executor_name = enlngdb_bin
                elif enlangg_bin:
                    cmd = [enlangg_bin, "db", "run", temp_file]
                    executor_name = f"{enlangg_bin} db"
            elif ext in (".enlngf", ".enlgf"):
                enlngf_bin = find_binary("enlngf") or find_binary("enlangg")
                if enlngf_bin:
                    cmd = [enlngf_bin, temp_file]
                    executor_name = enlngf_bin
            elif ext in (".enlngd", ".enlgd"):
                enlngd_bin = find_binary("enlngd") or find_binary("enlangg")
                if enlngd_bin:
                    cmd = [enlngd_bin, temp_file]
                    executor_name = enlngd_bin
            elif ext in (".enlngs", ".enlgs"):
                enlngs_bin = find_binary("enlngs") or find_binary("enlangg")
                if enlngs_bin:
                    cmd = [enlngs_bin, temp_file]
                    executor_name = enlngs_bin
            elif ext in (".enlngm", ".enlgm"):
                enlngm_bin = find_binary("enlngm") or find_binary("enlangg")
                if enlngm_bin:
                    cmd = [enlngm_bin, temp_file]
                    executor_name = enlngm_bin
            else:
                # Default core Enlang (.enlng) - use pure C native compiler enlng
                enlng_bin = find_binary("enlng")
                enlangg_bin = find_binary("enlangg")
                if enlng_bin:
                    cmd = [enlng_bin, temp_file]
                    executor_name = enlng_bin
                elif enlangg_bin:
                    cmd = [enlangg_bin, "run", temp_file]
                    executor_name = enlangg_bin

            if not cmd:
                # Fallback to running via Python compiler in repo
                cmd = [sys.executable, "-m", "enlang", temp_file]
                executor_name = "Python fallback runner"

            # Execute with safe 10-second timeout
            proc = subprocess.run(
                cmd,
                capture_output=True,
                text=True,
                timeout=10,
                encoding="utf-8",
                errors="replace",
                cwd=temp_dir
            )

            dt = (time.perf_counter() - t0) * 1000
            return {
                "success": proc.returncode == 0,
                "exitCode": proc.returncode,
                "stdout": proc.stdout,
                "stderr": proc.stderr,
                "output": proc.stdout or proc.stderr,
                "timeMs": round(dt, 2),
                "executor": executor_name,
                "tempFile": temp_file
            }
        except subprocess.TimeoutExpired:
            return {
                "success": False,
                "exitCode": 124,
                "stdout": "",
                "stderr": "Execution timed out after 10.0 seconds.",
                "output": "Execution timed out after 10.0 seconds.",
                "timeMs": round((time.perf_counter() - t0) * 1000, 2),
                "executor": str(cmd)
            }
        except Exception as e:
            return {
                "success": False,
                "exitCode": 1,
                "stdout": "",
                "stderr": str(e),
                "output": str(e),
                "timeMs": round((time.perf_counter() - t0) * 1000, 2),
                "executor": str(cmd)
            }
        finally:
            try:
                shutil.rmtree(temp_dir, ignore_errors=True)
            except Exception:
                pass

def run_bridge_server(port=DEFAULT_PORT):
    server = http.server.ThreadingHTTPServer(("127.0.0.1", port), EnlangBridgeHandler)
    binaries = detect_installed_binaries()
    found_count = sum(1 for b in binaries.values() if b["found"])
    
    print("=" * 65)
    print(f"  [ENLANGG] NATIVE BRIDGE & STUDIO DAEMON LIVE ON PORT {port}")
    print(f"  Local Bridge API : http://127.0.0.1:{port}/health")
    print(f"  Local Studio UI  : http://127.0.0.1:{port}/studio.html")
    print(f"  Detected Binaries: {found_count}/7 detected in system / repo")
    for name, info in binaries.items():
        symbol = "+" if info["found"] else "-"
        path_str = info["path"] or "Not found in ~/.enlangg/bin or PATH"
        print(f"   [{symbol}] {name:<10} -> {path_str}")
    print("=" * 65)
    print("  Ready to accept execution requests from Enlangg Studio...", flush=True)
    
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        print("\nDaemon stopped.")
        server.server_close()

if __name__ == "__main__":
    p = DEFAULT_PORT
    if len(sys.argv) > 1 and sys.argv[1].isdigit():
        p = int(sys.argv[1])
    run_bridge_server(p)
