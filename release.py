"""Enlang Master Release & Update Automation Script.

Bumps versions across all files, packages the fresh VS Code VSIX extension,
installs it locally, builds distribution packages, and uploads to PyPI.

Usage:
    python release.py <new_version> [pypi_token]

Example:
    python release.py 1.0.2
"""

import sys
import os
import re
import subprocess
import shutil

PYPI_TOKEN_DEFAULT = "pypi-AgEIcHlwaS5vcmcCJDRkZmMzMGYwLTQzNTYtNDc0ZS04Y2FhLTAzZmFmMzVhNmViYwACKlszLCJkYjFlZTE4ZC02MmQzLTQxNWUtYjg2MC05YjIwZDlmNDA4MDEiXQAABiBJtst_tH5XQmJm7uVFhUXoAaWY09rWjyiHng_O5aMvZQ"

def update_file_version(filepath: str, pattern: str, replacement: str):
    if not os.path.exists(filepath):
        print(f"Warning: {filepath} not found.")
        return
    with open(filepath, "r", encoding="utf-8") as f:
        content = f.read()
    new_content = re.sub(pattern, replacement, content)
    with open(filepath, "w", encoding="utf-8") as f:
        f.write(new_content)
    print(f"  [UPDATED] {filepath} -> version updated")

def main():
    if len(sys.argv) < 2:
        print("Usage: python release.py <new_version> [pypi_token]")
        print("Example: python release.py 1.0.2")
        sys.exit(1)

    new_ver = sys.argv[1].strip()
    token = sys.argv[2].strip() if len(sys.argv) > 2 else PYPI_TOKEN_DEFAULT

    print("================================================================")
    print(f"  ENLANG MASTER RELEASE AUTOMATION -> v{new_ver}")
    print("================================================================")

    # 1. Bump versions in config files
    print("\n1. Bumping versions across project...")
    update_file_version("setup.py", r'version="[^"]+"', f'version="{new_ver}"')
    update_file_version("pyproject.toml", r'version = "[^"]+"', f'version = "{new_ver}"')
    update_file_version("enlg/cli.py", r'VERSION = "[^"]+"', f'VERSION = "{new_ver}"')
    update_file_version("vscode-enlang/package.json", r'"version":\s*"[^"]+"', f'"version": "{new_ver}"')

    # 2. Package fresh VSIX
    print("\n2. Packaging fresh VS Code VSIX extension...")
    subprocess.run([sys.executable, "scratch/package_vsix.py"], check=True)

    vsix_path = f"vscode-enlang/enlang-{new_ver}.vsix"
    if os.path.exists(vsix_path):
        print(f"  [SUCCESS] Packaged: {vsix_path}")
        # Copy to enlg/vscode to bundle with PyPI package
        import shutil
        os.makedirs("enlg/vscode", exist_ok=True)
        shutil.copy(vsix_path, "enlg/vscode/enlang-extension.vsix")
        shutil.copy(vsix_path, f"enlg/vscode/enlang-{new_ver}.vsix")
        print("  [BUNDLED] Copied VSIX to enlg/vscode/ for 1-command installer")
        print("\n3. Installing updated VSIX in VS Code...")
        subprocess.run(["code.cmd", "--install-extension", vsix_path, "--force"], shell=True)

    # 3. Clean old dist/build folders
    print("\n4. Cleaning dist/build folders...")
    for d in ["dist", "build", "enlang.egg-info"]:
        if os.path.exists(d):
            shutil.rmtree(d, ignore_errors=True)

    # 4. Build distribution packages
    print("\n5. Building PyPI distribution packages...")
    res_build = subprocess.run([sys.executable, "setup.py", "sdist", "bdist_wheel"])
    if res_build.returncode != 0:
        print("[ERROR] Build failed!")
        sys.exit(1)

    # 5. Upload to PyPI
    print("\n6. Uploading to PyPI with twine...")
    cmd_upload = [
        "twine", "upload",
        "dist/*",
        "--skip-existing",
        "-u", "__token__",
        "-p", token
    ]
    res_upload = subprocess.run(cmd_upload)
    if res_upload.returncode == 0:
        print("\n================================================================")
        print(f"  [SUCCESS] Enlang v{new_ver} released live to PyPI & VSIX updated!")
        print(f"  PyPI URL: https://pypi.org/project/enlang/{new_ver}/")
        print(f"  Install with: pip install --upgrade enlang")
        print("================================================================")
    else:
        print(f"\n[ERROR] Upload returned code {res_upload.returncode}")

if __name__ == "__main__":
    main()
