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
    server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    
    try:
        server_socket.bind(("0.0.0.0", PORT))
    except Exception as e:
        print(f"Error binding to port {PORT}: {e}", file=sys.stderr)
        sys.exit(1)

    server_socket.listen(128)

    print("=" * 60)
    print(f"  PRAYAS 3D PORTFOLIO SERVER LIVE ON PORT {PORT}")
    print(f"  Live URL: http://localhost:{PORT}")
    print(f"  Alt URL:  http://127.0.0.1:{PORT}")
    print("=" * 60, flush=True)

    def handle_client(sock, addr):
        try:
            sock.settimeout(3.0)
            sock.setsockopt(socket.IPPROTO_TCP, socket.TCP_NODELAY, 1)
            request_data = sock.recv(4096).decode("utf-8", errors="ignore")
            if not request_data:
                sock.close()
                return

            req_line = request_data.split("\r\n")[0]
            parts = req_line.split(" ")
            path = parts[1] if len(parts) > 1 else "/"

            if path.startswith("/favicon.ico"):
                sock.sendall(b"HTTP/1.1 204 No Content\r\nConnection: close\r\n\r\n")
                sock.close()
                return

            html = compile_enlgf_file(FILEPATH)
            body = html.encode("utf-8")
            header = (
                f"HTTP/1.1 200 OK\r\n"
                f"Content-Type: text/html; charset=utf-8\r\n"
                f"Content-Length: {len(body)}\r\n"
                f"Cache-Control: no-cache, no-store, must-revalidate\r\n"
                f"Connection: close\r\n\r\n"
            ).encode("utf-8")

            sock.sendall(header + body)
            print(f"[Portfolio Server] 200 OK -> {path} ({len(body)} bytes)", flush=True)
        except Exception as e:
            pass
        finally:
            try:
                sock.shutdown(socket.SHUT_RDWR)
            except:
                pass
            sock.close()

    try:
        while True:
            client_sock, client_addr = server_socket.accept()
            t = threading.Thread(target=handle_client, args=(client_sock, client_addr), daemon=True)
            t.start()
    except KeyboardInterrupt:
        print("\nServer stopped.")
        server_socket.close()

if __name__ == "__main__":
    run_server()
