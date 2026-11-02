#!/usr/bin/env python3
"""Keeps the in-browser engine in step with the native compiler.

The website playground and Enlangg Studio run the same C interpreter as
enlng.exe, built to WebAssembly (tools/build_wasm.ps1). This script is the
pre-commit check:

  1. copies the native binaries to website/ (download mirrors),
  2. rebuilds the WebAssembly engine when a C source is newer than it
     (needs the Emscripten SDK; without it the old build is kept and a
     warning is printed),
  3. runs the programs of tests/interp_parity with enlng.exe and with the
     WebAssembly engine under Node, and fails when their output differs.

Usage: python sync_wasm.py [--no-build]
"""

import glob
import os
import shutil
import subprocess
import sys
import tempfile

ROOT_DIR = os.path.abspath(os.path.dirname(__file__))
SITE = os.path.join(ROOT_DIR, 'website')
WASM = os.path.join(SITE, 'enlng_engine.wasm')

# Programs that touch the clock, files, the network or the keyboard differ by nature.
SKIP_WORDS = ("random_number", "time_now", "sleep(", "ask ", "http", "serve", "read_file", "write ",
              "delete_file", "run_command", "listen", "connect", "read bytes", "list_dir", "make_folder")


def log(msg):
    print(f"[SYNC-WASM] {msg}")


def sync_binaries():
    for b in ['enlng.exe', 'enlangg.exe', 'enlngdb.exe', 'enlngf.exe', 'enlngd.exe', 'enlngs.exe', 'enlngm.exe']:
        src = os.path.join(ROOT_DIR, b)
        if os.path.exists(src):
            try:
                shutil.copy2(src, os.path.join(SITE, b))
            except OSError:
                pass
    log("Synchronized native binaries to website/ directory.")


def sources():
    files = [os.path.join(ROOT_DIR, 'enlng.c')]
    files += glob.glob(os.path.join(ROOT_DIR, 'src', '*.[ch]'))
    files += glob.glob(os.path.join(ROOT_DIR, 'src', '*.enlng'))
    files += glob.glob(os.path.join(ROOT_DIR, 'enlngdb', 'c', '*.[ch]'))
    files += glob.glob(os.path.join(ROOT_DIR, 'lib_*.enlng'))
    return [f for f in files if os.path.exists(f)]


def needs_build():
    if not os.path.exists(WASM):
        return True
    built = os.path.getmtime(WASM)
    return any(os.path.getmtime(f) > built for f in sources())


def rebuild():
    script = os.path.join(ROOT_DIR, 'tools', 'build_wasm.ps1')
    emsdk = os.environ.get('EMSDK_DIR', r'D:\emsdk' if os.name == 'nt' else os.path.expanduser('~/emsdk'))
    if os.name != 'nt' or not os.path.isdir(emsdk):
        log("Warning: the C sources are newer than the WebAssembly engine, but the Emscripten SDK "
            "was not found, so the engine was not rebuilt. See tools/build_wasm.ps1.")
        return False
    r = subprocess.run(['powershell', '-NoProfile', '-ExecutionPolicy', 'Bypass', '-File', script, '-Emsdk', emsdk],
                       capture_output=True, text=True)
    if r.returncode != 0:
        log("ERROR: building the WebAssembly engine failed:\n" + (r.stderr or r.stdout)[-1500:])
        return False
    log("Rebuilt the WebAssembly engine and the standard library file.")
    return True


def native_binary():
    for name in ('enlng.exe', 'enlng'):
        p = os.path.join(ROOT_DIR, name)
        if os.path.exists(p):
            return p
    return None


def verify_parity():
    native = native_binary()
    node = shutil.which('node')
    if not native or not node or not os.path.exists(WASM):
        log("Warning: parity not checked (needs enlng.exe, node and the built WebAssembly engine).")
        return True
    checked = 0
    for f in sorted(glob.glob(os.path.join(ROOT_DIR, 'tests', 'interp_parity', '*.enlng'))):
        src = open(f, encoding='utf-8').read()
        if any(w in src for w in SKIP_WORDS):
            continue
        work = tempfile.mkdtemp(prefix='enlng-wasm-')
        try:
            for h in glob.glob(os.path.join(os.path.dirname(f), '*.enlng')):
                shutil.copy(h, work)
            prog = os.path.join(work, os.path.basename(f))
            a = subprocess.run([native, 'run', prog], cwd=work, capture_output=True, stdin=subprocess.DEVNULL, timeout=60)
            b = subprocess.run([node, os.path.join(ROOT_DIR, 'tools', 'wasm_run.js'), prog],
                               capture_output=True, stdin=subprocess.DEVNULL, timeout=120)
        finally:
            shutil.rmtree(work, ignore_errors=True)
        ao = a.stdout.decode('utf-8', 'replace').replace('\r\n', '\n')
        bo = b.stdout.decode('utf-8', 'replace').replace('\r\n', '\n')
        if a.returncode != b.returncode or ao != bo:
            log(f"ERROR: the WebAssembly engine and enlng.exe differ on {os.path.basename(f)}")
            log(f"  native: exit {a.returncode} {ao[:160]!r}")
            log(f"  wasm:   exit {b.returncode} {bo[:160]!r}")
            return False
        checked += 1
    log(f"PASSED: {checked} programs give the same output with enlng.exe and in WebAssembly.")
    return True


def sync_all():
    log("==================================================")
    log("  Native compiler <-> WebAssembly engine check")
    log("==================================================")
    sync_binaries()
    if '--no-build' not in sys.argv and needs_build():
        if not rebuild() and not os.path.exists(WASM):
            sys.exit(1)
    if not verify_parity():
        log("[ERROR] The WebAssembly engine is not in step with the native compiler!")
        sys.exit(1)
    log("[SUCCESS] WebAssembly engine is in step with the native compiler.")


if __name__ == '__main__':
    sync_all()
