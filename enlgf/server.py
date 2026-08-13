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

def compile_enlgf_file(filepath: str) -> str:
    """Reads and compiles a .enlgf file into HTML5."""
    with open(filepath, "r", encoding="utf-8") as f:
        source = f.read()
    return compile_enlgf_source(source)

def start_server(filepath: str, port: int = 3000):
    """Starts live HTTP web server serving compiled .enlgf file."""
    if not os.path.exists(filepath):
        print(f"Error: File '{filepath}' not found.", file=sys.stderr)
        sys.exit(1)

    base_dir = os.path.dirname(os.path.abspath(filepath))

    class ENLEGFHandler(http.server.SimpleHTTPRequestHandler):
        def __init__(self, *args, **kwargs):
            super().__init__(*args, directory=base_dir, **kwargs)

        def do_GET(self):
            if self.path in ("/", f"/{os.path.basename(filepath)}"):
                try:
                    html_content = compile_enlgf_file(filepath)
                    self.send_response(200)
                    self.send_header("Content-type", "text/html; charset=utf-8")
                    self.end_headers()
                    self.wfile.write(html_content.encode("utf-8"))
                except Exception as e:
                    self.send_response(500)
                    self.send_header("Content-type", "text/html; charset=utf-8")
                    self.end_headers()
                    err_html = f"<html><body><h2>enlgf Compilation Error</h2><pre>{e}</pre></body></html>"
                    self.wfile.write(err_html.encode("utf-8"))
            else:
                super().do_GET()

        def log_message(self, format, *args):
            # Clean server log format
            print(f"[enlgf Server] {args[0]} -> {args[1]}")

    print("=" * 60)
    print(f"  ENLANG FRONTEND WEB SERVER (.enlgf)")
    print(f"  Serving File: {os.path.basename(filepath)}")
    print(f"  Live URL:     http://localhost:{port}")
    print("=" * 60)
    print("Press Ctrl+C to stop server.\n")

    socketserver.ThreadingTCPServer.allow_reuse_address = True
    with socketserver.ThreadingTCPServer(("", port), ENLEGFHandler) as httpd:
        try:
            httpd.serve_forever()
        except KeyboardInterrupt:
            print("\n[enlgf Server] Server stopped gracefully.")
            sys.exit(0)
