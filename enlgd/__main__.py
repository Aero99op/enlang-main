"""enlgd CLI Entry Point.
Supports:
  enlngd <file.enlngd> [-o <out.css>]
  enlngd compile <file.enlngd> [-o <out.css>]
  enlngd --version
"""
import sys
import os

def main():
    args = sys.argv[1:]
    if not args or "--help" in args or "-h" in args:
        print("enlngd - Enlangg Design Tokens & Stylesheet Compiler v5.0.0")
        print("\nUsage:")
        print("  enlngd <file.enlngd> [-o <out.css>]        Compile design tokens to CSS")
        print("  enlngd compile <file.enlngd> [-o <out.css>] Compile design tokens to CSS")
        print("  enlngd --version                           Display version information")
        sys.exit(0)

    if "--version" in args or "-v" in args:
        print("enlngd version 5.0.0 (Enlangg Design Tokens & Stylesheet Engine)")
        sys.exit(0)

    src = args[0]
    if src == "compile" and len(args) > 1:
        src = args[1]

    out = None
    if "-o" in args:
        idx = args.index("-o")
        if idx + 1 < len(args):
            out = args[idx + 1]

    from enlgd.compiler import build_enlgd_file
    build_enlgd_file(src, out)

if __name__ == "__main__":
    main()
