"""enlg Command Line Interface (CLI) & REPL.

Provides script execution and interactive shell capabilities.
"""

import sys
import os
import argparse
from enlg.lexer.lexer import Lexer
from enlg.parser.block_parser import BlockParser
from enlg.compiler.generator import CIRGenerator
from enlg.runtime.vm import VirtualMachine

# Force UTF-8 output so ML pipeline logs render correctly on all platforms
if sys.stdout.encoding and sys.stdout.encoding.lower() != "utf-8":
    sys.stdout.reconfigure(encoding="utf-8", errors="replace")

VERSION = "1.0.0"

def run_source(source: str, vm: VirtualMachine = None) -> VirtualMachine:
    """Executes enlg source code string through the entire compiler pipeline."""
    tokens = Lexer(source).tokenize()
    ast = BlockParser.parse(tokens)
    generator = CIRGenerator()
    cir = generator.generate(ast)
    
    if vm is None:
        vm = VirtualMachine()
        
    vm.execute(cir)
    return vm

def run_file(filepath: str):
    """Reads and executes an enlg source file."""
    if not os.path.exists(filepath):
        print(f"Error: File '{filepath}' not found.", file=sys.stderr)
        sys.exit(1)
        
    with open(filepath, "r", encoding="utf-8") as f:
        source = f.read()
        
    try:
        run_source(source)
    except Exception as e:
        print(f"Runtime Error: {e}", file=sys.stderr)
        sys.exit(1)

def start_repl():
    """Launches the interactive Read-Eval-Print Loop."""
    print(f"enlg Language REPL v{VERSION}")
    print("Type 'exit' or 'quit' to exit.\n")
    
    vm = VirtualMachine()
    
    while True:
        try:
            line = input("enlg> ")
            if line.strip() in ("exit", "quit"):
                break
            if not line.strip():
                continue
                
            run_source(line, vm)
            if len(vm.stack) > 0:
                val = vm.stack.pop()
                if val is not None:
                    print(val)
        except KeyboardInterrupt:
            print("\nExiting REPL.")
            break
        except Exception as e:
            print(f"Error: {e}")

def main():
    parser = argparse.ArgumentParser(description="enlg Programming Language Compiler & Runtime")
    parser.add_argument("--version", action="version", version=f"enlg v{VERSION}")
    
    subparsers = parser.add_subparsers(dest="command")
    
    # Run command
    run_parser = subparsers.add_parser("run", help="Run an enlg or enlgf source file")
    run_parser.add_argument("file", help="Path to .enlg or .enlgf file")
    run_parser.add_argument("--p", "--port", type=int, default=3000, help="Port to serve .enlgf web application (default 3000)")
    run_parser.add_argument("--style", type=str, default=None, help="Path to .enlgd stylesheet to inject into .enlgf web page")
    run_parser.add_argument("--script", type=str, default=None, help="Path to .enlgs script to inject into .enlgf web page")
    
    # Build command
    build_parser = subparsers.add_parser("build", help="Build/compile an .enlgd (CSS), .enlgs (JS), or .enlgf (HTML) file")
    build_parser.add_argument("file", help="Path to .enlgd, .enlgs, or .enlgf file")
    build_parser.add_argument("--out", "-o", type=str, default=None, help="Output destination file path")

    # REPL command
    subparsers.add_parser("repl", help="Start the interactive REPL shell")
    
    args = parser.parse_args()
    
    if args.command == "run":
        if args.file.endswith(".enlgf"):
            from enlgf.server import start_server
            start_server(args.file, port=args.p, style_path=args.style, script_path=args.script)
        else:
            run_file(args.file)
    elif args.command == "build":
        if args.file.endswith(".enlgd"):
            from enlgd.compiler import build_enlgd_file
            build_enlgd_file(args.file, output_path=args.out)
        elif args.file.endswith(".enlgs"):
            from enlgs.compiler import build_enlgs_file
            build_enlgs_file(args.file, output_path=args.out)
        elif args.file.endswith(".enlgf"):
            from enlgf.server import compile_enlgf_file
            html = compile_enlgf_file(
                args.file,
                style_path=args.style if hasattr(args, 'style') else None,
                script_path=args.script if hasattr(args, 'script') else None
            )
            out_file = args.out or f"{os.path.splitext(args.file)[0]}.html"
            with open(out_file, "w", encoding="utf-8") as f:
                f.write(html)
            print(f"[enlgf] Built HTML: {out_file}")
        else:
            print(f"Error: Unknown build file type '{args.file}'. Expected .enlgd, .enlgs, or .enlgf", file=sys.stderr)
            sys.exit(1)
    elif args.command == "repl":
        start_repl()
    else:
        # Default behavior with no args: launch REPL
        start_repl()

if __name__ == "__main__":
    main()
