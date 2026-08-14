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
    import subprocess
    import threading
    import time
    
    serve_dir = os.path.dirname(FILEPATH)
    if not serve_dir:
        serve_dir = "."

    html_out = os.path.splitext(FILEPATH)[0] + ".html"
    filename_html = os.path.basename(html_out)

    def do_compile():
        try:
            html = compile_enlgf_file(FILEPATH)
            with open(html_out, "w", encoding="utf-8") as f:
                f.write(html)
            return True
        except Exception as e:
            print(f"\n[enlgf Compile Error] {e}", file=sys.stderr)
            return False

    if not do_compile():
        sys.exit(1)

    print("=" * 60)
    print(f"  PRAYAS 3D PORTFOLIO SERVER LIVE ON PORT {PORT}")
    print(f"  Powered by: HTML Live Server (npx live-server)")
    print(f"  Watching:   {os.path.basename(FILEPATH)}")
    print(f"  Live URL:   http://localhost:{PORT}/{filename_html}")
    print("=" * 60, flush=True)

    def watch_files():
        base_name = os.path.splitext(FILEPATH)[0]
        files_to_watch = [FILEPATH]
        
        if os.path.exists(f"{base_name}.enlgd"):
            files_to_watch.append(f"{base_name}.enlgd")
        if os.path.exists(f"{base_name}.enlgs"):
            files_to_watch.append(f"{base_name}.enlgs")

        last_mtimes = {f: os.stat(f).st_mtime for f in files_to_watch if os.path.exists(f)}
        
        while True:
            time.sleep(0.5)
            changed = False
            for f in files_to_watch:
                if os.path.exists(f):
                    current_mtime = os.stat(f).st_mtime
                    if current_mtime > last_mtimes.get(f, 0):
                        last_mtimes[f] = current_mtime
                        changed = True
            
            if changed:
                print(f"[enlgf Watcher] Change detected. Recompiling -> {filename_html}")
                do_compile()

    watcher_t = threading.Thread(target=watch_files, daemon=True)
    watcher_t.start()
    
    cmd = [
        "npx", "live-server",
        f"--port={PORT}",
        f"--open={filename_html}",
        "--cors"
    ]

    try:
        import platform
        use_shell = platform.system() == "Windows"
        subprocess.run(cmd, cwd=serve_dir, check=True, shell=use_shell)
    except KeyboardInterrupt:
        print("\nServer stopped.")
    except Exception as e:
        print(f"\n[Server Error] Failed to start npx live-server: {e}", file=sys.stderr)


if __name__ == "__main__":
    run_server()
