"""enlgf CLI Entry Point.
Supports:
  enlngf <file.enlngf> [--port <port>]
  enlngf run <file.enlngf> [--port <port>]
  enlngf build <file.enlngf> [-o <out.html>]
  enlngf --version
"""
import sys
import os

def main():
    args = sys.argv[1:]
    if not args or "--help" in args or "-h" in args:
        print("enlngf - Enlangg Frontend & Web Studio Compiler v5.0.0")
        print("\nUsage:")
        print("  enlngf <file.enlngf> [--port <port>]       Serve live over HTTP with hot reload")
        print("  enlngf run <file.enlngf> [--port <port>]   Launch interactive Web Studio")
        print("  enlngf build <file.enlngf> [-o <out.html>] Compile frontend markup to standalone HTML")
        print("  enlngf --version                           Display version information")
        sys.exit(0)

    if "--version" in args or "-v" in args:
        print("enlngf version 5.0.0 (Enlangg Frontend & Web Studio Engine)")
        sys.exit(0)

    if args[0] == "build":
        if len(args) < 2:
            print("Usage: enlngf build <file.enlngf> [-o <out.html>]", file=sys.stderr)
            sys.exit(1)
        src = args[1]
        out = None
        if "-o" in args:
            idx = args.index("-o")
            if idx + 1 < len(args):
                out = args[idx + 1]
        if not out:
            base, _ = os.path.splitext(src)
            out = f"{base}.html"
        from enlgf.server import compile_enlgf_file
        html = compile_enlgf_file(src)
        with open(out, "w", encoding="utf-8") as f:
            f.write(html)
        print(f"[enlngf] Compiled HTML written to '{out}'")
        sys.exit(0)

    src = args[0]
    if src == "run" and len(args) > 1:
        src = args[1]
    
    port = 3000
    for flag in ["--port", "-p", "--p"]:
        if flag in args:
            idx = args.index(flag)
            if idx + 1 < len(args):
                port = int(args[idx + 1])
                break

    from enlgf.server import serve_enlgf
    serve_enlgf(src, port)

if __name__ == "__main__":
    main()
