import os
import sys
import socket
import threading

BASE_DIR = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, BASE_DIR)

from enlgf.server import compile_enlgf_file

PORT = 2222
FILEPATH = os.path.join(BASE_DIR, "portfolio.enlgf")

def run_server():
    import http.server
    import socketserver
    
    serve_dir = os.path.dirname(FILEPATH)
    if not serve_dir:
        serve_dir = "."

    class PortfolioHandler(http.server.SimpleHTTPRequestHandler):
        def __init__(self, *args, **kwargs):
            super().__init__(*args, directory=serve_dir, **kwargs)

        def do_GET(self):
            req_path = self.path.split("?")[0]
            
            # Serve the compiled .enlgf file on root or exact filename match
            if req_path == "/" or req_path == f"/{os.path.basename(FILEPATH)}":
                try:
                    html = compile_enlgf_file(FILEPATH)
                    encoded = html.encode("utf-8")
                    self.send_response(200)
                    self.send_header("Content-Type", "text/html; charset=utf-8")
                    self.send_header("Content-Length", str(len(encoded)))
                    self.send_header("Cache-Control", "no-cache, no-store, must-revalidate")
                    self.end_headers()
                    self.wfile.write(encoded)
                except Exception as e:
                    self.send_response(500)
                    self.send_header("Content-Type", "text/html; charset=utf-8")
                    self.end_headers()
                    err_html = f"<html><body><h2>enlgf Compilation Error</h2><pre>{e}</pre></body></html>"
                    self.wfile.write(err_html.encode("utf-8"))
                return
                
            # Fall back to standard serving for assets like images, js, etc.
            super().do_GET()

        def log_message(self, format, *args):
            pass

    print("=" * 60)
    print(f"  PRAYAS 3D PORTFOLIO SERVER LIVE ON PORT {PORT}")
    print(f"  Serving Dir: {os.path.abspath(serve_dir)}")
    print(f"  Live URL: http://localhost:{PORT}")
    print(f"  Alt URL:  http://127.0.0.1:{PORT}")
    print("=" * 60, flush=True)

    socketserver.TCPServer.allow_reuse_address = True
    
    class ThreadedHTTPServer(socketserver.ThreadingMixIn, socketserver.TCPServer):
        daemon_threads = True

    try:
        with ThreadedHTTPServer(("0.0.0.0", PORT), PortfolioHandler) as httpd:
            httpd.serve_forever()
    except KeyboardInterrupt:
        print("\nServer stopped.")
    except Exception as e:
        print(f"\n[Server Error] {e}", file=sys.stderr)


if __name__ == "__main__":
    run_server()
