import os
import sys
import http.server

BASE_DIR = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, BASE_DIR)

from enlgf.server import compile_enlgf_file

PORT = 2222
FILEPATH = os.path.join(BASE_DIR, "portfolio.enlgf")

class PortfolioHandler(http.server.BaseHTTPRequestHandler):
    protocol_version = "HTTP/1.0"

    def address_string(self):
        return self.client_address[0]

    def do_HEAD(self):
        self.send_response(200)
        self.send_header("Content-Type", "text/html; charset=utf-8")
        self.send_header("Connection", "close")
        self.end_headers()
        self.close_connection = True

    def do_GET(self):
        req_path = self.path.split("?")[0]
        
        if req_path == "/favicon.ico":
            self.send_response(204)
            self.send_header("Connection", "close")
            self.end_headers()
            self.close_connection = True
            return

        try:
            html = compile_enlgf_file(FILEPATH)
            data = html.encode("utf-8")

            self.send_response(200)
            self.send_header("Content-Type", "text/html; charset=utf-8")
            self.send_header("Content-Length", str(len(data)))
            self.send_header("Cache-Control", "no-cache, no-store, must-revalidate")
            self.send_header("Connection", "close")
            self.end_headers()
            self.wfile.write(data)
            self.wfile.flush()
            self.close_connection = True
            print(f"[Portfolio Server] 200 OK -> {self.path} ({len(data)} bytes)", flush=True)
        except Exception as e:
            print(f"[Portfolio Server Error] {e}", flush=True)
            self.send_response(500)
            self.send_header("Content-Type", "text/html; charset=utf-8")
            self.send_header("Connection", "close")
            self.end_headers()
            self.wfile.write(f"<h1>500 Server Error</h1><pre>{e}</pre>".encode("utf-8"))
            self.wfile.flush()
            self.close_connection = True

    def log_message(self, format, *args):
        pass

if __name__ == "__main__":
    http.server.ThreadingHTTPServer.allow_reuse_address = True
    server = http.server.ThreadingHTTPServer(("0.0.0.0", PORT), PortfolioHandler)
    print("=" * 60)
    print(f"  PRAYAS 3D PORTFOLIO SERVER LIVE ON PORT {PORT}")
    print(f"  Live URL: http://localhost:{PORT}")
    print(f"  Alt URL:  http://127.0.0.1:{PORT}")
    print("=" * 60, flush=True)
    server.serve_forever()
