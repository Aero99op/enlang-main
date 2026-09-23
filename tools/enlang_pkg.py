#!/usr/bin/env python3
"""
tools/enlang_pkg.py - The Sovereign Enlang Package Manager (enlangg pkg / init / add / install)

Provides:
  - Manifest management via enlang.json
  - Standard library resolution and linking into .enlang_modules/
  - Git and local package dependency management
  - Clean, zero-brittle import resolution
"""

import sys
import os
import json
import shutil
import subprocess
import argparse

STDLIB_MAPPING = {
    "math": "lib_math.enlng",
    "strings": "lib_strings.enlng",
    "ds": "lib_ds.enlng",
    "file": "lib_file.enlng",
    "fs": "lib_fs.enlng",
    "ml": "lib_ml.enlng",
    "net": "lib_net.enlng",
    "db": "lib_db.enlng",
    "concurrency": "lib_concurrency.enlng",
    "std": "lib_std.enlng",
    "enlngdb": "lib_enlngdb.enlng"
}

def find_enlang_root():
    """Finds the Enlang standard library / engine root directory."""
    if "ENLANG_HOME" in os.environ and os.path.exists(os.environ["ENLANG_HOME"]):
        return os.path.abspath(os.environ["ENLANG_HOME"])
    
    # Path relative to tools/enlang_pkg.py
    candidate = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
    if os.path.exists(os.path.join(candidate, "lib_math.enlng")):
        return candidate
    
    if os.path.exists("D:\\enlangg\\lib_math.enlng"):
        return "D:\\enlangg"
        
    return candidate

def load_manifest(cwd=None):
    """Loads enlang.json manifest from current or specified directory."""
    cwd = cwd or os.getcwd()
    manifest_path = os.path.join(cwd, "enlang.json")
    if not os.path.exists(manifest_path):
        return None, manifest_path
    try:
        with open(manifest_path, "r", encoding="utf-8") as f:
            data = json.load(f)
            return data, manifest_path
    except Exception as e:
        print(f"[ENLANGG PKG ERROR] Failed to parse {manifest_path}: {e}", file=sys.stderr)
        return None, manifest_path

def save_manifest(data, manifest_path):
    """Saves formatted enlang.json."""
    with open(manifest_path, "w", encoding="utf-8", newline="\n") as f:
        json.dump(data, f, indent=2)
        f.write("\n")

def cmd_init(args):
    """Initializes a new Enlang project with enlang.json and main.enlng."""
    cwd = os.getcwd()
    manifest_path = os.path.join(cwd, "enlang.json")
    if os.path.exists(manifest_path):
        print(f"[ENLANGG PKG] Project manifest already exists at: {manifest_path}")
        return 0

    proj_name = args.name or os.path.basename(os.path.abspath(cwd))
    manifest = {
        "name": proj_name,
        "version": "1.0.0",
        "description": "Sovereign Enlang Project",
        "main": "main.enlng",
        "dependencies": {}
    }
    save_manifest(manifest, manifest_path)
    print(f"[SUCCESS] Created project manifest: {manifest_path}")

    main_file = os.path.join(cwd, "main.enlng")
    if not os.path.exists(main_file):
        with open(main_file, "w", encoding="utf-8", newline="\n") as f:
            f.write("# Sovereign Enlang Application Entrypoint\n\n")
            f.write(f"freeze APP_NAME as \"{proj_name}\"\n")
            f.write("show \"[START] Running \" + APP_NAME\n")
            f.write("show \"Sovereign Enlang 5.0 ready.\"\n")
        print(f"[SUCCESS] Generated entrypoint: {main_file}")

    # Ensure .gitignore ignores .enlang_modules/
    gitignore_path = os.path.join(cwd, ".gitignore")
    ignore_entry = ".enlang_modules/\n"
    if os.path.exists(gitignore_path):
        with open(gitignore_path, "r", encoding="utf-8", errors="replace") as f:
            lines = f.read()
        if ".enlang_modules" not in lines:
            with open(gitignore_path, "a", encoding="utf-8") as f:
                f.write("\n# Enlang Sovereign Modules\n.enlang_modules/\n")
    else:
        with open(gitignore_path, "w", encoding="utf-8") as f:
            f.write("# Enlang Sovereign Modules\n.enlang_modules/\n")

    return 0

def install_dependency(pkg_name, source, target_dir, enlang_root):
    """Installs a single dependency into target_dir (.enlang_modules)."""
    os.makedirs(target_dir, exist_ok=True)

    # 1. Standard Library
    if source == "stdlib" or pkg_name in STDLIB_MAPPING:
        lib_filename = STDLIB_MAPPING.get(pkg_name, f"lib_{pkg_name}.enlng")
        src_path = os.path.join(enlang_root, lib_filename)
        if not os.path.exists(src_path):
            print(f"[ENLANGG PKG WARN] Standard library file '{lib_filename}' not found at {src_path}")
            return False

        # Copy as lib_<name>.enlng and <name>.enlng
        dest_lib = os.path.join(target_dir, lib_filename)
        dest_short = os.path.join(target_dir, f"{pkg_name}.enlng")
        shutil.copyfile(src_path, dest_lib)
        shutil.copyfile(src_path, dest_short)
        print(f"  + Linked stdlib '{pkg_name}' -> .enlang_modules/{lib_filename}")
        return True

    # 2. Git Repository
    if source.startswith(("http://", "https://", "git@")):
        dest_git = os.path.join(target_dir, pkg_name)
        if os.path.exists(dest_git):
            print(f"  * Updating git package '{pkg_name}'...")
            subprocess.run(["git", "pull"], cwd=dest_git, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        else:
            print(f"  + Cloning git package '{pkg_name}' from {source}...")
            res = subprocess.run(["git", "clone", "--depth", "1", source, dest_git])
            if res.returncode != 0:
                print(f"[ENLANGG PKG ERROR] Failed to clone '{source}'", file=sys.stderr)
                return False
        return True

    # 3. Local file or directory
    if source.startswith("file:"):
        local_src = source[5:]
    else:
        local_src = source

    if os.path.exists(local_src):
        dest_local = os.path.join(target_dir, pkg_name)
        if os.path.isdir(local_src):
            if os.path.exists(dest_local):
                shutil.rmtree(dest_local)
            shutil.copytree(local_src, dest_local)
        else:
            shutil.copyfile(local_src, dest_local)
        print(f"  + Linked local package '{pkg_name}' from {local_src}")
        return True

    print(f"[ENLANGG PKG ERROR] Unknown source '{source}' for package '{pkg_name}'", file=sys.stderr)
    return False

def cmd_add(args):
    """Adds a dependency to enlang.json and installs it."""
    cwd = os.getcwd()
    manifest, manifest_path = load_manifest(cwd)
    if manifest is None:
        manifest = {
            "name": os.path.basename(os.path.abspath(cwd)),
            "version": "1.0.0",
            "dependencies": {}
        }

    pkg_name = args.package.strip()
    if not pkg_name:
        print("[ENLANGG PKG ERROR] Please specify package name.", file=sys.stderr)
        return 1

    source = "stdlib"
    if args.git:
        source = args.git
    elif args.path:
        source = f"file:{os.path.abspath(args.path)}"
    elif pkg_name in STDLIB_MAPPING:
        source = "stdlib"
    elif pkg_name.startswith(("http://", "https://", "git@")):
        source = pkg_name
        pkg_name = os.path.basename(pkg_name.rstrip("/")).replace(".git", "")

    if "dependencies" not in manifest:
        manifest["dependencies"] = {}

    manifest["dependencies"][pkg_name] = source
    save_manifest(manifest, manifest_path)

    enlang_root = find_enlang_root()
    target_dir = os.path.join(cwd, ".enlang_modules")
    success = install_dependency(pkg_name, source, target_dir, enlang_root)
    if success:
        print(f"[SUCCESS] Added '{pkg_name}' ({source}) to {manifest_path}")
        return 0
    return 1

def cmd_install(args):
    """Installs all dependencies from enlang.json."""
    cwd = os.getcwd()
    manifest, manifest_path = load_manifest(cwd)
    if manifest is None:
        print(f"[ENLANGG PKG ERROR] No enlang.json found in {cwd}. Run 'enlangg init' first.", file=sys.stderr)
        return 1

    deps = manifest.get("dependencies", {})
    if not deps:
        print("[ENLANGG PKG] No dependencies declared in enlang.json.")
        return 0

    print("==============================================================")
    print(f"       ENLANGG PACKAGE INSTALLER ({len(deps)} dependencies)  ")
    print("==============================================================")

    enlang_root = find_enlang_root()
    target_dir = os.path.join(cwd, ".enlang_modules")
    os.makedirs(target_dir, exist_ok=True)

    installed = 0
    for pkg_name, source in deps.items():
        if install_dependency(pkg_name, source, target_dir, enlang_root):
            installed += 1

    print("--------------------------------------------------------------")
    print(f"[SUCCESS] {installed}/{len(deps)} dependencies installed in .enlang_modules/")
    return 0

def cmd_list(args):
    """Lists installed dependencies."""
    cwd = os.getcwd()
    manifest, manifest_path = load_manifest(cwd)
    if manifest is None:
        print("[ENLANGG PKG] No enlang.json in current directory.")
        return 0

    deps = manifest.get("dependencies", {})
    print("==============================================================")
    print(f"  Project: {manifest.get('name', 'unnamed')} v{manifest.get('version', '0.0.0')}")
    print("==============================================================")
    if not deps:
        print("  (No dependencies declared)")
    else:
        for pkg, src in deps.items():
            installed = os.path.exists(os.path.join(cwd, ".enlang_modules", pkg)) or \
                        os.path.exists(os.path.join(cwd, ".enlang_modules", f"lib_{pkg}.enlng")) or \
                        os.path.exists(os.path.join(cwd, ".enlang_modules", f"{pkg}.enlng"))
            status = "[INSTALLED]" if installed else "[MISSING - run enlangg install]"
            print(f"  - {pkg:<16} : {src}  {status}")
    print("--------------------------------------------------------------")
    return 0

def cmd_remove(args):
    """Removes a package from enlang.json and deletes it from .enlang_modules."""
    cwd = os.getcwd()
    manifest, manifest_path = load_manifest(cwd)
    if manifest is None or "dependencies" not in manifest:
        print("[ENLANGG PKG ERROR] No enlang.json found.", file=sys.stderr)
        return 1

    pkg_name = args.package.strip()
    if pkg_name not in manifest["dependencies"]:
        print(f"[ENLANGG PKG WARN] Package '{pkg_name}' not listed in dependencies.")
        return 0

    del manifest["dependencies"][pkg_name]
    save_manifest(manifest, manifest_path)

    # Remove files in .enlang_modules
    target_dir = os.path.join(cwd, ".enlang_modules")
    candidates = [
        os.path.join(target_dir, pkg_name),
        os.path.join(target_dir, f"{pkg_name}.enlng"),
        os.path.join(target_dir, f"lib_{pkg_name}.enlng")
    ]
    for c in candidates:
        if os.path.isdir(c):
            shutil.rmtree(c)
        elif os.path.isfile(c):
            os.remove(c)

    print(f"[SUCCESS] Removed package '{pkg_name}' from project.")
    return 0

def main():
    parser = argparse.ArgumentParser(
        prog="enlangg pkg",
        description="The Sovereign Universal Enlang Package Manager"
    )
    subparsers = parser.add_subparsers(dest="command", help="Package manager commands")

    # init
    init_parser = subparsers.add_parser("init", help="Initialize a new enlang.json project")
    init_parser.add_argument("name", nargs="?", default=None, help="Project name (optional)")

    # add
    add_parser = subparsers.add_parser("add", help="Add a dependency to enlang.json")
    add_parser.add_argument("package", help="Package or stdlib name (e.g. math, strings, fs, ds)")
    add_parser.add_argument("--git", help="Git repository URL")
    add_parser.add_argument("--path", help="Local directory or file path")

    # install
    subparsers.add_parser("install", help="Install all dependencies from enlang.json")

    # list
    subparsers.add_parser("list", help="List project dependencies")

    # remove
    rm_parser = subparsers.add_parser("remove", help="Remove a dependency")
    rm_parser.add_argument("package", help="Package name to remove")

    args = parser.parse_args()
    if not args.command:
        parser.print_help()
        sys.exit(0)

    handlers = {
        "init": cmd_init,
        "add": cmd_add,
        "install": cmd_install,
        "list": cmd_list,
        "remove": cmd_remove
    }

    ret = handlers[args.command](args)
    sys.exit(ret)

if __name__ == "__main__":
    main()
