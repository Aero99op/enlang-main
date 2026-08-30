"""Enlang PyPI Build & Upload Automation Script.

Usage:
    python upload_pypi.py <pypi-token>
    # OR set PYPI_TOKEN environment variable
"""

import sys
import os
import subprocess
import shutil

def build_and_upload(token=None):
    if not token and len(sys.argv) > 1:
        token = sys.argv[1]
    
    if not token:
        token = os.environ.get("PYPI_TOKEN")

    print("================================================================")
    print("  ENLANG PYPI RELEASE AUTOMATION v1.0.1")
    print("================================================================")

    # 1. Clean old dist and build folders
    for d in ["dist", "build", "enlang.egg-info"]:
        if os.path.exists(d):
            print(f"Cleaning {d}...")
            shutil.rmtree(d, ignore_errors=True)

    # 2. Build sdist and wheel
    print("\nBuilding distribution packages (sdist + bdist_wheel)...")
    res_build = subprocess.run([sys.executable, "setup.py", "sdist", "bdist_wheel"])
    if res_build.returncode != 0:
        print("[ERROR] Build failed! Aborting upload.")
        sys.exit(1)

    print("\n[SUCCESS] Built distribution packages in dist/:")
    for f in os.listdir("dist"):
        print(f"  - dist/{f}")

    if not token:
        print("\n================================================================")
        print("  Packages built successfully!")
        print("  To upload to PyPI, provide your API token:")
        print("    python upload_pypi.py pypi-AgEI...your-token-here")
        print("================================================================")
        return

    # 3. Upload with twine
    print("\nUploading to PyPI with twine...")
    cmd = [
        "twine", "upload",
        "dist/*",
        "--skip-existing",
        "-u", "__token__",
        "-p", token
    ]
    res_upload = subprocess.run(cmd)
    if res_upload.returncode == 0:
        print("\n================================================================")
        print("  [SUCCESS] Successfully released Enlang v1.0.0 to PyPI!")
        print("  Install with: pip install --upgrade enlang")
        print("================================================================")
    else:
        print(f"\n[ERROR] Twine upload failed with return code {res_upload.returncode}")

if __name__ == "__main__":
    build_and_upload()
