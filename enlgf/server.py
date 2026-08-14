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
    """Starts a live server by wrapping Node.js 'live-server' with a background .enlgf watcher."""
    import subprocess
    import threading
    import time
    
    abs_filepath = os.path.abspath(filepath)
    if not os.path.exists(abs_filepath):
        print(f"Error: File '{filepath}' not found.", file=sys.stderr)
        sys.exit(1)
        
    serve_dir = os.path.dirname(abs_filepath)
    if not serve_dir:
        serve_dir = "."
        
    # The output HTML file that live-server will watch and serve
    html_out = os.path.splitext(abs_filepath)[0] + ".html"
    filename_html = os.path.basename(html_out)

    def do_compile():
        try:
            html = compile_enlgf_file(abs_filepath, style_path=style_path, script_path=script_path)
            with open(html_out, "w", encoding="utf-8") as f:
                f.write(html)
            return True
        except Exception as e:
            print(f"\n[enlgf Compile Error] {e}", file=sys.stderr)
            return False

    # Initial compile
    if not do_compile():
        sys.exit(1)

    print("=" * 60)
    print(f"  ENLANG FRONTEND WEB SERVER (.enlgf)")
    print(f"  Powered by:   HTML Live Server (npx live-server)")
    print(f"  Watching:     {os.path.basename(abs_filepath)}")
    print(f"  Live URL:     http://localhost:{port}/{filename_html}")
    print("=" * 60)
    print("Press Ctrl+C to stop server.\n", flush=True)

    # Watcher thread to auto-compile when .enlgf / .enlgd / .enlgs changes
    def watch_files():
        # Get related files
        base_name = os.path.splitext(abs_filepath)[0]
        files_to_watch = [abs_filepath]
        if style_path and os.path.exists(style_path):
            files_to_watch.append(style_path)
        elif os.path.exists(f"{base_name}.enlgd"):
            files_to_watch.append(f"{base_name}.enlgd")
            
        if script_path and os.path.exists(script_path):
            files_to_watch.append(script_path)
        elif os.path.exists(f"{base_name}.enlgs"):
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

    # Spawn standard node live-server
    cmd = [
        "npx", "live-server",
        f"--port={port}",
        f"--open={filename_html}",
        "--cors"
    ]
    
    try:
        # Run live-server in the foreground, letting it handle stdout/stderr
        import platform
        use_shell = platform.system() == "Windows"
        subprocess.run(cmd, cwd=serve_dir, check=True, shell=use_shell)
    except KeyboardInterrupt:
        print("\n[enlgf Server] Server stopped gracefully.")
        sys.exit(0)
    except Exception as e:
        print(f"\n[enlgf Server Error] Failed to start npx live-server. Is Node.js/npx installed? {e}", file=sys.stderr)
        sys.exit(1)
