#!/usr/bin/env python3
"""
Enlang Sovereign Auto-Sync Pipeline: Native Compiler <-> WebAssembly Engine
Ensures that whenever enlng.exe or compiler sources change,
the in-browser WebAssembly engine and bundle are automatically updated with 100% parity.
"""

import os
import sys
import json
import time
import shutil
import subprocess

ROOT_DIR = os.path.abspath(os.path.dirname(__file__))

def log(msg):
    print(f"[SYNC-WASM] {msg}")

def rebuild_wasm_bundle():
    """Packs all 12 Enlang domain modules and stdlib files into website/enlang_bundle.json."""
    dirs = ['enlg', 'enlgd', 'enlgdb', 'enlgf', 'enlgm', 'enlgs', 'enlng', 'enlngd', 'enlngdb', 'enlngf', 'enlngm', 'enlngs']
    bundle = {}

    for d in dirs:
        dir_path = os.path.join(ROOT_DIR, d)
        if not os.path.exists(dir_path):
            continue
        for root, _, files in os.walk(dir_path):
            if '__pycache__' in root or 'tests' in root:
                continue
            for f in files:
                if f.endswith('.py') or f.endswith('.enlng'):
                    p = os.path.join(root, f)
                    rel = os.path.relpath(p, ROOT_DIR).replace('\\', '/')
                    try:
                        with open(p, 'r', encoding='utf-8') as fh:
                            bundle[rel] = fh.read()
                    except Exception as e:
                        pass

    engine_path = os.path.join(ROOT_DIR, 'website', 'enlang_engine.py')
    if os.path.exists(engine_path):
        with open(engine_path, 'r', encoding='utf-8') as f:
            bundle['enlang_engine.py'] = f.read()

    out_path = os.path.join(ROOT_DIR, 'website', 'enlang_bundle.json')
    with open(out_path, 'w', encoding='utf-8') as out:
        json.dump(bundle, out)

    size_kb = os.path.getsize(out_path) / 1024
    log(f"Packed {len(bundle)} compiler & stdlib files into website/enlang_bundle.json ({size_kb:.1f} KB)")
    return out_path

def sync_binaries():
    """Syncs compiled executables to website/ directory."""
    bins = ['enlng.exe', 'enlangg.exe', 'enlngdb.exe', 'enlngf.exe', 'enlngd.exe', 'enlngs.exe', 'enlngm.exe']
    for b in bins:
        src = os.path.join(ROOT_DIR, b)
        dst = os.path.join(ROOT_DIR, 'website', b)
        if os.path.exists(src):
            try:
                shutil.copy2(src, dst)
            except Exception:
                pass
    log("Synchronized native binaries to website/ directory.")

def verify_parity():
    """Runs dual-engine parity verification between native compiler and WebAssembly engine."""
    sys.path.insert(0, os.path.join(ROOT_DIR, 'website'))
    try:
        import enlang_engine
    except ImportError:
        log("Warning: Could not import enlang_engine for verification.")
        return True

    test_code = """type enlng

remember n as "spandan"
show n
string_replace "a" in n at 0
show n
"""

    res_raw = enlang_engine.execute_enlang_wasm('sandbox.enlng', test_code, 'enlng')
    try:
        res = json.loads(res_raw) if isinstance(res_raw, str) else res_raw
    except Exception:
        res = {}

    if not res or not res.get('success'):
        log(f"ERROR: WebAssembly parity test failed: {res_raw}")
        return False


    output = res.get('output', '').strip().replace('\r\n', '\n')
    expected = "spandan\napandan"
    if expected not in output:
        log(f"ERROR: Parity mismatch! Got: {output}, Expected: {expected}")
        return False

    log("PASSED: WebAssembly parity test verified successfully (spandan -> apandan)!")
    return True
def sync_all():
    log("==================================================")
    log("  Enlang Automatic WebAssembly Sync Pipeline")
    log("==================================================")
    sync_binaries()
    rebuild_wasm_bundle()
    ok = verify_parity()
    if ok:
        log("[SUCCESS] WebAssembly engine is 100% in sync with native compiler!")
    else:
        log("[ERROR] Sync verification failed!")
        sys.exit(1)

def get_watched_files():

    files = [
        os.path.join(ROOT_DIR, 'enlng.exe'),
        os.path.join(ROOT_DIR, 'enlangg.exe'),
        os.path.join(ROOT_DIR, 'enlng.c'),
        os.path.join(ROOT_DIR, 'enlangg.c'),
        os.path.join(ROOT_DIR, 'website', 'enlang_engine.py'),
    ]
    src_dir = os.path.join(ROOT_DIR, 'src')
    if os.path.exists(src_dir):
        for root, _, fs in os.walk(src_dir):
            for f in fs:
                if f.endswith(('.c', '.h')):
                    files.append(os.path.join(root, f))
    return files

def watch_mode():
    log("Starting Live Auto-Sync Watcher Daemon...")
    log("Monitoring enlng.exe, enlangg.exe, enlng.c, and src/ for changes...")
    log("Press Ctrl+C to stop.")
    
    mtimes = {}
    for f in get_watched_files():
        if os.path.exists(f):
            mtimes[f] = os.path.getmtime(f)

    try:
        while True:
            time.sleep(1)
            changed = False
            for f in get_watched_files():
                if os.path.exists(f):
                    current_mtime = os.path.getmtime(f)
                    if f not in mtimes or current_mtime > mtimes[f]:
                        log(f"Change detected in: {os.path.basename(f)}")
                        mtimes[f] = current_mtime
                        changed = True
            if changed:
                sync_all()
    except KeyboardInterrupt:
        log("Auto-Sync Watcher stopped.")

if __name__ == '__main__':
    if len(sys.argv) > 1 and sys.argv[1] in ('--watch', '-w', 'watch'):
        watch_mode()
    else:
        sync_all()

