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
    run_parser = subparsers.add_parser("run", help="Run an enlg source file")
    run_parser.add_argument("file", help="Path to .enlg file")
    
    # REPL command
    subparsers.add_parser("repl", help="Start the interactive REPL shell")
    
    args = parser.parse_args()
    
    if args.command == "run":
        run_file(args.file)
    elif args.command == "repl":
        start_repl()
    else:
        # Default behavior with no args: launch REPL
        start_repl()

if __name__ == "__main__":
    main()
