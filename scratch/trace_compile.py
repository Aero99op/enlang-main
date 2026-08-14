import os
import sys

BASE_DIR = r"D:\enlangg"
sys.path.insert(0, BASE_DIR)

from enlgf.server import compile_enlgf_file
import threading
import traceback

def dump_threads():
    import faulthandler
    faulthandler.dump_traceback()

print("Setting timer to dump threads if frozen...")
timer = threading.Timer(2.0, dump_threads)
timer.start()

try:
    print("Starting compile...")
    html = compile_enlgf_file(os.path.join(BASE_DIR, "portfolio.enlgf"))
    print("Compile succeeded.")
except Exception as e:
    print(f"Error: {e}")
finally:
    timer.cancel()
