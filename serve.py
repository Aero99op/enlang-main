import os
import sys
import http.server

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from enlgf.server import compile_enlgf_file

PORT = 5000
FILEPATH = os.path.abspath("app.enlgf")

class Handler(http.server.BaseHTTPRequestHandler):
    protocol_version = "HTTP/1.0"
    
    def address_string(self):
        return self.client_address[0]
        
    def do_GET(self):
        print(f"[HTTP] Received GET request from {self.client_address}", flush=True)
        try:
            html = compile_enlgf_file(FILEPATH)
            data = html.encode("utf-8")
            self.send_response(200)
            self.send_header("Content-Type", "text/html; charset=utf-8")
            self.send_header("Content-Length", str(len(data)))
            self.send_header("Connection", "close")
            self.end_headers()
            self.wfile.write(data)
            self.wfile.flush()
            print(f"[HTTP] Sent {len(data)} bytes to {self.client_address}", flush=True)
        except Exception as e:
            print(f"[HTTP Error] {e}", flush=True)
            self.send_response(500)
            self.end_headers()

if __name__ == "__main__":
    http.server.HTTPServer.allow_reuse_address = True
    server = http.server.HTTPServer(("0.0.0.0", PORT), Handler)
    print(f"============================================================")
    print(f"  ENLANG SERVER LIVE ON:")
    print(f"  http://localhost:{PORT}")
    print(f"  http://127.0.0.1:{PORT}")
    print(f"============================================================", flush=True)
    server.serve_forever()
