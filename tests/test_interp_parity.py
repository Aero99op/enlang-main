"""Differential test: the built-in interpreter must print what the gcc path prints.

    python tests/test_interp_parity.py <path-to-enlng> [--list-only-failures]

Runs every .enlng program under tests/ and examples/ twice (`enlng run` and
`enlng run --native`) with empty stdin, in a scratch copy of the working
directory, and compares exit code and stdout. Needs gcc for the native side.
"""
import glob
import os
import shutil
import subprocess
import sys
import tempfile

# Programs whose output is not deterministic or that need a network/UI.
SKIP_WORDS = ("random_number", "time_now", "sleep(", "ask ", "http", "serve")


def run(exe, args, cwd):
    try:
        p = subprocess.run([exe, "run"] + args, cwd=cwd, stdin=subprocess.DEVNULL,
                           capture_output=True, timeout=90)
        return p.returncode, p.stdout.decode("utf-8", "replace").replace("\r\n", "\n"), p.stderr.decode("utf-8", "replace")
    except subprocess.TimeoutExpired:
        return -999, "", "timeout"


def main():
    exe = os.path.abspath(sys.argv[1])
    root = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    files = sorted(glob.glob(os.path.join(root, "tests", "**", "*.enlng"), recursive=True)
                   + glob.glob(os.path.join(root, "examples", "**", "*.enlng"), recursive=True))
    checked = same = 0
    bad = []
    skipped = 0
    for path in files:
        try:
            src = open(path, encoding="utf-8", errors="replace").read()
        except OSError:
            continue
        if any(w in src for w in SKIP_WORDS) or not src.lstrip().startswith("type enlng"):
            skipped += 1
            continue
        work = tempfile.mkdtemp(prefix="enlng-parity-")
        try:
            # fresh copy of the folder's helper files so writes never touch the repo
            for f in glob.glob(os.path.join(os.path.dirname(path), "*.enlng")):
                shutil.copy(f, work)
            rel = os.path.join(work, os.path.basename(path))
            a = run(exe, [rel], work)
            b = run(exe, ["--native", rel], work)
        finally:
            shutil.rmtree(work, ignore_errors=True)
        checked += 1
        if a[0] == b[0] and a[1] == b[1]:
            same += 1
        else:
            bad.append((os.path.relpath(path, root), a, b))
    print(f"{checked} compared, {same} identical, {len(bad)} different, {skipped} skipped")
    for name, a, b in bad:
        print(f"\nDIFF {name}")
        print(f"  interp: exit {a[0]}  stdout {a[1][:200]!r}  stderr {a[2][:200]!r}")
        print(f"  native: exit {b[0]}  stdout {b[1][:200]!r}  stderr {b[2][:200]!r}")
    sys.exit(1 if bad else 0)


if __name__ == "__main__":
    main()
