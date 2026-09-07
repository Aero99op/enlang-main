"""enlgs CLI Entry Point.
Supports:
  enlngs <file.enlngs> [-o <out.js>]
  enlngs compile <file.enlngs> [-o <out.js>]
  enlngs --version
"""
import sys
import os

def main():
    args = sys.argv[1:]
    if not args or "--help" in args or "-h" in args:
        print("enlngs - Enlangg Reactive Fullstack Script Engine v5.0.0")
        print("\nUsage:")
        print("  enlngs <file.enlngs> [-o <out.js>]        Compile reactive script to JavaScript")
        print("  enlngs compile <file.enlngs> [-o <out.js>] Compile reactive script to JavaScript")
        print("  enlngs --version                           Display version information")
        sys.exit(0)

    if "--version" in args or "-v" in args:
        print("enlngs version 5.0.0 (Enlangg Reactive Fullstack Script Engine)")
        sys.exit(0)

    src = args[0]
    if src == "compile" and len(args) > 1:
        src = args[1]

    out = None
    if "-o" in args:
        idx = args.index("-o")
        if idx + 1 < len(args):
            out = args[idx + 1]

    from enlgs.compiler import build_enlgs_file
    build_enlgs_file(src, out)

if __name__ == "__main__":
    main()
