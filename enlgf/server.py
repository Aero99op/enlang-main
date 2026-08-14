"""enlgf Web Server.

Serves compiled .enlgf HTML pages live over HTTP on any specified port (default 3000).
"""

import sys
import os
import http.server
import socketserver
from .lexer import ENLGFLexer
from .parser import ENLEGFPParser
from .emitter import ENLGFEmitter

def compile_enlgf_source(source: str) -> str:
    """Compiles .enlgf markup source to HTML5 string."""
    tokens = ENLGFLexer(source).tokenize()
    ast = ENLEGFPParser(tokens).parse()
    html = ENLGFEmitter(ast).emit()
    return html

def compile_enlgf_file(filepath: str, style_path: str = None, script_path: str = None) -> str:
    """Reads and compiles a .enlgf file into HTML5, optionally injecting .enlgd stylesheets and .enlgs scripts."""
    with open(filepath, "r", encoding="utf-8") as f:
        source = f.read()
    html = compile_enlgf_source(source)
    
    # 1. Auto-detect matching .enlgd stylesheet if not explicitly provided
    if style_path is None:
        auto_style = f"{os.path.splitext(filepath)[0]}.enlgd"
        if os.path.exists(auto_style):
            style_path = auto_style

    if style_path and os.path.exists(style_path):
        try:
            if style_path.endswith(".enlgd"):
                from enlgd.compiler import compile_enlgd_file
                css_content = compile_enlgd_file(style_path)
            else:
                with open(style_path, "r", encoding="utf-8") as sf:
                    css_content = sf.read()
            style_tag = f"    <style>\n{css_content}\n    </style>\n  </head>"
            html = html.replace("  </head>", style_tag, 1)
        except Exception as e:
            print(f"[enlgf Server Warning] Failed to compile stylesheet '{style_path}': {e}", file=sys.stderr)

    # 2. Auto-detect matching .enlgs script if not explicitly provided
    if script_path is None:
        auto_script = f"{os.path.splitext(filepath)[0]}.enlgs"
        if os.path.exists(auto_script):
            script_path = auto_script

    if script_path and os.path.exists(script_path):
        try:
            if script_path.endswith(".enlgs"):
                from enlgs.compiler import compile_enlgs_file
                js_content = compile_enlgs_file(script_path)
            else:
                with open(script_path, "r", encoding="utf-8") as scf:
                    js_content = scf.read()
            script_tag = f"    <script>\n{js_content}\n    </script>\n  </body>"
            if "  </body>" in html:
                html = html.replace("  </body>", script_tag, 1)
            elif "</body>" in html:
                html = html.replace("</body>", script_tag, 1)
            else:
                html += f"\n<script>\n{js_content}\n</script>"
        except Exception as e:
            print(f"[enlgf Server Warning] Failed to compile script '{script_path}': {e}", file=sys.stderr)

    # 3. Auto-detect and compile referenced <link ... href="*.enlgd"> in HTML
    import re
    link_matches = re.findall(r'<link[^>]*href=["\']([^"\']+\.enlgd)["\'][^>]*>', html)
    for enlgd_ref in link_matches:
        base_dir = os.path.dirname(os.path.abspath(filepath)) if os.path.exists(filepath) else "."
        full_ref_path = os.path.join(base_dir, enlgd_ref)
        if os.path.exists(full_ref_path):
            try:
                from enlgd.compiler import compile_enlgd_file
                css_content = compile_enlgd_file(full_ref_path)
                pattern = re.compile(rf'<link[^>]*href=["\']{re.escape(enlgd_ref)}["\'][^>]*>')
                html = pattern.sub(f"<style>\n{css_content}\n    </style>", html)
            except Exception as e:
                print(f"[enlgf Server Warning] Failed to compile referenced stylesheet '{enlgd_ref}': {e}", file=sys.stderr)

    # 4. Auto-detect and compile referenced <script ... src="*.enlgs"> in HTML
    script_matches = re.findall(r'<script[^>]*src=["\']([^"\']+\.enlgs)["\'][^>]*>\s*</script>', html)
    for enlgs_ref in script_matches:
        base_dir = os.path.dirname(os.path.abspath(filepath)) if os.path.exists(filepath) else "."
        full_ref_path = os.path.join(base_dir, enlgs_ref)
        if os.path.exists(full_ref_path):
            try:
                from enlgs.compiler import compile_enlgs_file
                js_content = compile_enlgs_file(full_ref_path)
                pattern = re.compile(rf'<script[^>]*src=["\']{re.escape(enlgs_ref)}["\'][^>]*>\s*</script>')
                html = pattern.sub(f"<script>\n{js_content}\n    </script>", html)
            except Exception as e:
                print(f"[enlgf Server Warning] Failed to compile referenced script '{enlgs_ref}': {e}", file=sys.stderr)

    return html

def start_server(filepath: str, port: int = 3000, style_path: str = None, script_path: str = None):
    """Starts live high-performance HTTP web server serving compiled .enlgf file with instant response."""
    import socket
    import threading

    abs_filepath = os.path.abspath(filepath)
    if not os.path.exists(abs_filepath):
        print(f"Error: File '{filepath}' not found.", file=sys.stderr)
        sys.exit(1)

    server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    
    try:
        server_socket.bind(("0.0.0.0", port))
    except Exception as e:
        print(f"Error binding to port {port}: {e}", file=sys.stderr)
        sys.exit(1)

    server_socket.listen(128)

    print("=" * 60)
    print(f"  ENLANG FRONTEND WEB SERVER (.enlgf)")
    print(f"  Serving File: {os.path.basename(abs_filepath)}")
    print(f"  Live URL:     http://localhost:{port}")
    print(f"  Alt URL:      http://127.0.0.1:{port}")
    print("=" * 60)
    print("Press Ctrl+C to stop server.\n", flush=True)

    def handle_client(sock, client_addr):
        try:
            sock.settimeout(3.0)
            sock.setsockopt(socket.IPPROTO_TCP, socket.TCP_NODELAY, 1)
            request_data = sock.recv(4096).decode("utf-8", errors="ignore")
            if not request_data:
                sock.close()
                return

            req_line = request_data.split("\r\n")[0]
            parts = req_line.split(" ")
            method = parts[0] if len(parts) > 0 else "GET"
            path = parts[1] if len(parts) > 1 else "/"

            if path.startswith("/favicon.ico"):
                res = b"HTTP/1.1 204 No Content\r\nConnection: close\r\n\r\n"
                sock.sendall(res)
                sock.close()
                return

            # Recompile on every request for live developer updates
            try:
                html_content = compile_enlgf_file(abs_filepath, style_path=style_path, script_path=script_path)
                body = html_content.encode("utf-8")
                header = (
                    f"HTTP/1.1 200 OK\r\n"
                    f"Content-Type: text/html; charset=utf-8\r\n"
                    f"Content-Length: {len(body)}\r\n"
                    f"Cache-Control: no-cache, no-store, must-revalidate\r\n"
                    f"Connection: close\r\n\r\n"
                ).encode("utf-8")
                sock.sendall(header + body)
                print(f"[enlgf Server] 200 OK -> {path} ({len(body)} bytes)", flush=True)
            except Exception as comp_err:
                err_html = f"<html><body><h2>enlgf Compilation Error</h2><pre>{comp_err}</pre></body></html>".encode("utf-8")
                header = (
                    f"HTTP/1.1 500 Internal Server Error\r\n"
                    f"Content-Type: text/html; charset=utf-8\r\n"
                    f"Content-Length: {len(err_html)}\r\n"
                    f"Connection: close\r\n\r\n"
                ).encode("utf-8")
                sock.sendall(header + err_html)
                print(f"[enlgf Server Error] {comp_err}", flush=True)
        except Exception:
            pass
        finally:
            try:
                sock.shutdown(socket.SHUT_RDWR)
            except Exception:
                pass
            sock.close()

    try:
        while True:
            client_sock, client_addr = server_socket.accept()
            t = threading.Thread(target=handle_client, args=(client_sock, client_addr), daemon=True)
            t.start()
    except KeyboardInterrupt:
        print("\n[enlgf Server] Server stopped gracefully.")
        server_socket.close()
        sys.exit(0)
