"""enlg / enlang Command Line Interface (CLI), REPL & Version Manager.

Provides script execution, web server hosting, production compilation,
and multi-version management commands.
"""

import sys
import os
import argparse
import subprocess
import json
import urllib.request
from pathlib import Path

from enlg.lexer.lexer import Lexer
from enlg.parser.block_parser import BlockParser
from enlg.compiler.generator import CIRGenerator
from enlg.runtime.vm import VirtualMachine

# Force UTF-8 output so logs and special characters render correctly on all platforms
if sys.stdout.encoding and sys.stdout.encoding.lower() != "utf-8":
    sys.stdout.reconfigure(encoding="utf-8", errors="replace")

VERSION = "1.0.14"
PYPI_PACKAGE_NAME = "enlang"
ENLANG_HOME = Path.home() / ".enlang"
VERSIONS_DIR = ENLANG_HOME / "versions"

def _get_pypi_latest_version(pkg_name=PYPI_PACKAGE_NAME) -> str:
    """Fetches the latest released version from PyPI JSON API."""
    try:
        url = f"https://pypi.org/pypi/{pkg_name}/json"
        req = urllib.request.Request(url, headers={"User-Agent": f"enlang-cli/{VERSION}"})
        with urllib.request.urlopen(req, timeout=3) as resp:
            data = json.loads(resp.read().decode("utf-8"))
            return data.get("info", {}).get("version", VERSION)
    except Exception:
        return VERSION

def cmd_check_version(args):
    """Checks current installed version and queries PyPI for updates."""
    print("================================================================")
    print(f"  EnLang Programming Language & Engine v{VERSION}")
    print("================================================================")
    print(f"  Installed Version:  {VERSION}")
    
    print("  Checking PyPI for latest version...", end="", flush=True)
    latest = _get_pypi_latest_version()
    print(f" [{latest}]")
    
    if latest != VERSION:
        print(f"\n  👉 Update Available: v{latest}")
        print(f"     Run: enlang update -v latest")
    else:
        print("  ✅ You are on the latest version!")
    print("================================================================")

def cmd_update(args):
    """Updates Enlang to the latest PyPI release or specified version."""
    target_ver = args.v or "latest"
    print(f"Updating EnLang to '{target_ver}' via pip...")
    
    pkg = f"{PYPI_PACKAGE_NAME}=={target_ver}" if target_ver != "latest" else f"--upgrade {PYPI_PACKAGE_NAME}"
    cmd = [sys.executable, "-m", "pip", "install"] + pkg.split()
    
    res = subprocess.run(cmd)
    if res.returncode == 0:
        print(f"\n✅ Successfully updated EnLang!")
    else:
        print(f"\n❌ Update failed with exit code {res.returncode}", file=sys.stderr)
        sys.exit(res.returncode)

def cmd_install_coexisting(args):
    """Installs a specific version alongside the current version (co-existence)."""
    target_ver = args.v
    if not target_ver:
        print("Error: Specify version with -v <versionname> (e.g. enlang install -v 1.0.0)", file=sys.stderr)
        sys.exit(1)
        
    dest_dir = VERSIONS_DIR / target_ver
    dest_dir.mkdir(parents=True, exist_ok=True)
    
    print(f"Installing EnLang v{target_ver} into co-existing storage: {dest_dir}")
    cmd = [
        sys.executable, "-m", "pip", "install",
        f"{PYPI_PACKAGE_NAME}=={target_ver}",
        "--target", str(dest_dir)
    ]
    res = subprocess.run(cmd)
    if res.returncode == 0:
        print(f"\n✅ EnLang v{target_ver} installed successfully alongside other versions!")
        print(f"   Storage path: {dest_dir}")
        print(f"   To view all versions: enlang list -v")
        print(f"   To activate this version: enlang switch -v {target_ver}")
    else:
        print(f"\n❌ Installation failed with code {res.returncode}", file=sys.stderr)
        sys.exit(res.returncode)

def cmd_install_extension(args):
    """Installs the official EnLang VS Code / Cursor / VSCodium extension directly into developer IDE."""
    import glob
    print("================================================================")
    print("  EnLang IDE Extension Installer")
    print("================================================================")

    # Locate bundled vsix file
    vsix_candidates = []
    
    # 1. Check inside installed package
    pkg_vscode = Path(__file__).parent / "vscode"
    if pkg_vscode.exists():
        vsix_candidates.extend(pkg_vscode.glob("*.vsix"))

    # 2. Check local workspace
    workspace_vscode = Path("vscode-enlang")
    if workspace_vscode.exists():
        vsix_candidates.extend(workspace_vscode.glob("*.vsix"))

    if not vsix_candidates:
        print("  [ERROR] No bundled extension package (.vsix) found!", file=sys.stderr)
        print("  Please run: python release.py <version> to generate one.")
        print("================================================================")
        sys.exit(1)

    # Pick the best vsix candidate
    vsix_candidates.sort(key=lambda p: p.stat().st_mtime, reverse=True)
    vsix_path = vsix_candidates[0]

    # Detect available IDE commands in system PATH
    ide_commands = [
        ("code", "Visual Studio Code"),
        ("cursor", "Cursor IDE"),
        ("codium", "VSCodium"),
        ("code-insiders", "VS Code Insiders")
    ]

    installed_count = 0
    for cmd_name, ide_title in ide_commands:
        try:
            # Test if IDE command is available
            test_cmd = f"{cmd_name} --version" if os.name != 'nt' else f"{cmd_name}.cmd --version"
            check_res = subprocess.run(test_cmd, shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
            if check_res.returncode == 0:
                print(f"  Detected IDE:     {ide_title} ({cmd_name})")
                print(f"  Extension VSIX:   {vsix_path.name}")
                
                install_cmd = f"{cmd_name} --install-extension \"{vsix_path.resolve()}\" --force" if os.name != 'nt' else f"{cmd_name}.cmd --install-extension \"{vsix_path.resolve()}\" --force"
                install_res = subprocess.run(install_cmd, shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
                
                if install_res.returncode == 0:
                    print(f"  Status:           [SUCCESS] Installed into {ide_title}!")
                    installed_count += 1
                else:
                    print(f"  Status:           [WARNING] {install_res.stderr.strip()[:80]}")
        except Exception:
            pass

    if installed_count > 0:
        print("----------------------------------------------------------------")
        print("  🎉 Extension installed! All 6 Enlang domains are now active:")
        print("     - .enlg   (Backend & VM)")
        print("     - .enlgf  (Web Frontend Markup)")
        print("     - .enlgd  (CSS & Design DSL)")
        print("     - .enlgs  (Client Reactive Scripting)")
        print("     - .enlgm  (Flutter Mobile DSL)")
        print("     - .enlgdb (Natural English SQL & Database)")
        print("================================================================")
    else:
        print("  [MANUAL INSTALLATION]:")
        print(f"  VSIX File Location: {vsix_path.resolve()}")
        print("  Open VS Code -> Extensions -> '...' -> 'Install from VSIX...'")
        print("================================================================")

def cmd_replace(args):
    """Replaces current active installation with specified version."""
    target_ver = args.v
    if not target_ver:
        print("Error: Specify version with -v <versionname> (e.g. enlang replace -v 1.0.0)", file=sys.stderr)
        sys.exit(1)
        
    print(f"Replacing active EnLang installation with v{target_ver}...")
    cmd = [
        sys.executable, "-m", "pip", "install",
        "--force-reinstall",
        f"{PYPI_PACKAGE_NAME}=={target_ver}"
    ]
    res = subprocess.run(cmd)
    if res.returncode == 0:
        print(f"\n✅ Active EnLang installation replaced with v{target_ver} successfully!")
    else:
        print(f"\n❌ Replacement failed with code {res.returncode}", file=sys.stderr)
        sys.exit(res.returncode)

def cmd_list_versions(args):
    """Lists all co-existing installed versions."""
    print("================================================================")
    print("  EnLang Co-Existing Versions:")
    print("================================================================")
    print(f"  * {VERSION} (Current System Active)")
    
    if VERSIONS_DIR.exists():
        for d in VERSIONS_DIR.iterdir():
            if d.is_dir():
                print(f"    - {d.name} ({d})")
    print("================================================================")

def run_source(source: str, vm: VirtualMachine = None) -> VirtualMachine:
    """Executes enlg source code string through the compiler pipeline."""
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
    print(f"EnLang Interactive REPL v{VERSION}")
    print("Type 'exit' or 'quit' to exit.\n")
    
    vm = VirtualMachine()
    
    while True:
        try:
            line = input("enlang> ")
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
    parser = argparse.ArgumentParser(description="EnLang Programming Language Compiler, Runtime & Package Manager")
    parser.add_argument("-v", "--version", action="version", version=f"enlang v{VERSION}")
    
    subparsers = parser.add_subparsers(dest="command")
    
    # Run command
    run_parser = subparsers.add_parser("run", help="Run an enlg, enlgf, html, enlgs, enlgm, js, or py source file")
    run_parser.add_argument("file", help="Path to source file (.enlg, .enlgf, .html, .enlgs, .enlgm, .js, .py)")
    run_parser.add_argument("-p", "--p", "--port", type=int, default=3000, help="Port to serve web application (default 3000)")
    run_parser.add_argument("--style", type=str, default=None, help="Path to .enlgd stylesheet to inject into .enlgf web page")
    run_parser.add_argument("--script", type=str, default=None, help="Path to .enlgs script to inject into .enlgf web page")
    
    # Build command
    build_parser = subparsers.add_parser("build", help="Build/translate .enlg (Python), .enlgd (CSS), .enlgs (JS), .enlgm (Dart), or .enlgf (HTML) file")
    build_parser.add_argument("file", help="Path to source file")
    build_parser.add_argument("--out", "-o", type=str, default=None, help="Output destination file path")

    # Check command
    check_parser = subparsers.add_parser("check", help="Check file syntax for errors or inspect version/PyPI status")
    check_parser.add_argument("file", nargs="?", default=None, help="Path to source file to check syntax for (.enlg, .enlgf, .enlgd, .enlgs, .enlgm)")
    check_parser.add_argument("-v", "--version-check", action="store_true", help="Detailed version check against PyPI")

    # AI command
    ai_parser = subparsers.add_parser("ai", help="Ask Enlang AI assistant or generate code snippets")
    ai_parser.add_argument("prompt", nargs="*", default=[], help="Question or coding task for Enlang AI")
    
    # Update command
    update_parser = subparsers.add_parser("update", help="Update EnLang to latest or specified release")
    update_parser.add_argument("-v", type=str, default="latest", help="Version to update to (default: latest)")
    
    # Install command
    install_parser = subparsers.add_parser("install", help="Install IDE extension or co-existing Enlang version")
    install_parser.add_argument("target", nargs="?", default=None, help="Target to install ('extension', 'vscode', 'cursor' or version with -v)")
    install_parser.add_argument("-v", type=str, default=None, help="Version name to install (e.g. 1.0.2)")
    
    # Replace command
    replace_parser = subparsers.add_parser("replace", help="Replace active version with specified version")
    replace_parser.add_argument("-v", type=str, required=True, help="Version name to replace active installation with")

    # List versions command
    list_parser = subparsers.add_parser("list", help="List all co-existing versions")
    list_parser.add_argument("-v", "--versions", action="store_true", help="List installed versions")

    # REPL command
    subparsers.add_parser("repl", help="Start the interactive REPL shell")
    
    args = parser.parse_args()
    
    if args.command == "run":
        file_lower = args.file.lower()
        if file_lower.endswith((".enlgf", ".html")):
            from enlgf.server import start_server
            start_server(args.file, port=args.p, style_path=args.style, script_path=args.script)
        elif file_lower.endswith(".enlgs"):
            import subprocess
            from enlgs.compiler import build_enlgs_file
            js_file = build_enlgs_file(args.file)
            subprocess.run(["node", js_file])
        elif file_lower.endswith(".enlgm"):
            import subprocess
            from enlgm.compiler import build_enlgm_file
            dart_file = build_enlgm_file(args.file)
            print(f"[enlgm] Running Flutter application...")
            subprocess.run(["flutter", "run"])
        elif file_lower.endswith(".enlgdb"):
            from enlgdb.compiler import run_enlgdb_file
            run_enlgdb_file(args.file)
        elif file_lower.endswith(".js"):
            import subprocess
            subprocess.run(["node", args.file])
        elif file_lower.endswith(".py"):
            import subprocess
            subprocess.run([sys.executable, args.file])
        elif file_lower.endswith(".enlg"):
            run_file(args.file)
        else:
            run_file(args.file)
            
    elif args.command == "build":
        file_lower = args.file.lower()
        if file_lower.endswith(".enlg"):
            from enlg.compiler.py_transpiler import build_enlg_file
            build_enlg_file(args.file, output_path=args.out)
        elif file_lower.endswith(".enlgdb"):
            from enlgdb.compiler import build_enlgdb_file
            build_enlgdb_file(args.file, output_path=args.out)
        elif file_lower.endswith(".enlgd"):
            from enlgd.compiler import build_enlgd_file
            build_enlgd_file(args.file, output_path=args.out)
        elif file_lower.endswith(".enlgs"):
            from enlgs.compiler import build_enlgs_file
            build_enlgs_file(args.file, output_path=args.out)
        elif file_lower.endswith(".enlgm"):
            from enlgm.compiler import build_enlgm_file
            build_enlgm_file(args.file, output_path=args.out)
        elif file_lower.endswith(".enlgf"):
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
            print(f"Error: Unknown build file type '{args.file}'. Expected .enlg, .enlgdb, .enlgd, .enlgs, .enlgm, or .enlgf", file=sys.stderr)
            sys.exit(1)
            
    elif args.command == "check":
        if args.file:
            from enlg.diagnostics.checker import print_diagnostic_report
            print_diagnostic_report(args.file)
        else:
            cmd_check_version(args)

    elif args.command == "ai":
        from enlg.ai.assistant import run_ai_cli
        user_prompt = " ".join(args.prompt) if args.prompt else None
        run_ai_cli(user_prompt)

    elif args.command == "update":
        cmd_update(args)
    elif args.command == "install":
        if args.target in ("extension", "vscode", "cursor", "codium", "ide") or (not args.v and not args.target):
            cmd_install_extension(args)
        elif args.v:
            cmd_install_coexisting(args)
        else:
            cmd_install_extension(args)
    elif args.command == "replace":
        cmd_replace(args)
    elif args.command == "list":
        cmd_list_versions(args)
    elif args.command == "repl":
        start_repl()
    else:
        start_repl()

if __name__ == "__main__":
    main()
